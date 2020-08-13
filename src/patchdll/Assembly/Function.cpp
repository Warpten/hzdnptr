#include "Function.hpp"
#include "../MemoryHelpers.hpp"

#include <Zydis/Utils.h>

#include <functional>
#include <optional>
#include <unordered_set>

namespace Assembly
{
    Function::Function(uintptr_t address)
    {
        using NodePtr = std::shared_ptr<Node>;

        std::function<void(NodePtr)> collectNodes;
        collectNodes = [&collectNodes, this](NodePtr node) -> void {
            Instruction const& lastInstruction = node->Block.GetLastInstruction();

            if (lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_UNCOND_BR || lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_COND_BR)
            {
                auto immediateOperand = lastInstruction.GetOperand([](ZydisDecodedOperand const& operand) {
                    return operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
                });

                if (immediateOperand != nullptr && node->_branchTarget.expired())
                {
                    uintptr_t jumpTarget = 0;
                    ZydisCalcAbsoluteAddress(&lastInstruction.Decoded, immediateOperand, lastInstruction.Address, &jumpTarget);

                    NodePtr jumpTargetNode = this->TryGetNode(jumpTarget, node);
                    node->_branchTarget = jumpTargetNode;

                    collectNodes(jumpTargetNode);
                }
            }

            if (lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_RET)
                return;

            if (node->_next.expired())
            {
                NodePtr nextNode = this->TryGetNode(lastInstruction.Address + lastInstruction.Decoded.length, node);
                node->_next = nextNode;
                collectNodes(nextNode);
            }
        };

        _front = TryGetNode(address, nullptr);
        collectNodes(_front);
    }

    auto Function::TryGetNode(uintptr_t address, std::shared_ptr<Node> parent) -> std::shared_ptr<Node>
    {
        auto itr = _nodeCache.find(address);
        if (itr != _nodeCache.end())
            return itr->second;

        // Node not found, create it
        std::shared_ptr<Node> node = std::make_shared<Node>(InstructionBlock{ address });
        auto pair = this->_nodeCache.emplace(std::piecewise_construct,
            std::forward_as_tuple(address),
            std::forward_as_tuple(std::make_shared<Node>(InstructionBlock{ address })));

        if (pair.second)
            return pair.first->second;

        return nullptr;
    }

    std::optional<uintptr_t> Function::FindEpilogue() const
    {
        auto findEpilogueStart = [](std::shared_ptr<Node> currentBlock) -> std::optional<Instruction>
        {
            auto instructions = currentBlock->Block.GetInstructions();
            auto itr = instructions.rbegin();
            ++itr; // Skip last instruction
            for (; itr != instructions.rend(); ++itr)
            {
                Instruction const& instruction = *itr;

                // if we find add rsp it's the beginning of epilogue
                if (instruction.Decoded.mnemonic == ZYDIS_MNEMONIC_ADD)
                {
                    auto stackCleanupOperand = instruction.GetOperand([](ZydisDecodedOperand const& operand) {
                        return operand.type == ZYDIS_OPERAND_TYPE_REGISTER
                            && operand.visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT
                            && (operand.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
                            && operand.reg.value == ZYDIS_REGISTER_RSP;
                        }
                    );

                    if (stackCleanupOperand != nullptr)
                        return instruction;
                }
                else if (instruction.Decoded.mnemonic == ZYDIS_MNEMONIC_POP) // Register pops are valid
                    continue;
                else // But everything else is not
                    return {};
            }

            return {};
        };

        std::shared_ptr<Node> lastBlock;

        // In a loop situation (A -> B -> C -> A -> ...) the result of evaluating A
        // depends on the result of evaluating B. It's a cyclic dependency graph.
        // We stop cyclic analysis by maintaining a cache of explored nodes.
        // Each traversed node is marked as such even when the result is unknown.
        // In short, when evaluating C, A will not be reevaluated again, thus bypassing that branch.
        // For a more complex graph:
        // A -> B -> C ------\
        // ^          \-> D  |
        //  \----------------/
        // When hitting C, we won't analyze A again, and thus C's result depends on D, which means every
        // node in the cycle (A, B and C) depend on D, meaning that every node depending on A's output
        // actually depends on D, maintaining a dependency chain.
        std::unordered_set<std::shared_ptr<Node>> exploredNodes;
        exploredNodes.emplace(nullptr); // To avoid processing empty nodes, dirty trick

        std::function<std::pair<bool, std::shared_ptr<Node>>(std::shared_ptr<Node>)> findLastBlock;
        findLastBlock = [&findLastBlock, &exploredNodes, &findEpilogueStart](std::shared_ptr<Node> currentBlock) -> std::pair<bool, std::shared_ptr<Node>>
        {
            exploredNodes.insert(currentBlock);

            auto branchTargetBlock = currentBlock->GetBranchTarget();


            Instruction const& lastInstruction = currentBlock->Block.GetLastInstruction();
            // investigate the branch target if it exists and if it hasn't been processed yet
            if (lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_UNCOND_BR && exploredNodes.count(branchTargetBlock) == 0)
            {
                // -- -- -- -- -- -- -- SHORTCUT -- -- -- -- -- -- -- 
                // if the unconditional branch is followed by alignment padding (CC chains) we know this is the function end
                // thanks to how compilers generally generate code.
                if (Memory::ReadMemory<uint8_t>(lastInstruction.Address + lastInstruction.Decoded.length, false) == 0xCC)
                    return std::make_pair(true, currentBlock);

                // If unconditional jump this could still be an epilogue
                // Consider the following:
                //   push rbx
                //   sub rsp, 0x20
                //   ...
                //   add rsp, 0x20
                //   pop rdi
                //   jmp ...
                //
                //   sub rsp, 0x28 ; at jump target
                //   ...
                //   add rsp, 0x28
                //   retn
                // Here, jmp is the real end of function, thus we need to consider 
                //   add rsp, 0x20
                // as epilogue, not
                //   add rsp, 0x28
                Instruction const& branchTarget = branchTargetBlock->Block.GetFirstInstruction();

                if (branchTarget.Decoded.mnemonic == ZYDIS_MNEMONIC_SUB)
                {
                    auto registerOperand = branchTarget.GetOperand([](ZydisDecodedOperand const& operand) {
                        return operand.type == ZYDIS_OPERAND_TYPE_REGISTER
                            && operand.visibility == ZYDIS_OPERAND_VISIBILITY_EXPLICIT
                            && (operand.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
                            && operand.reg.value == ZYDIS_REGISTER_RSP;
                        }
                    );

                    if (registerOperand != nullptr)
                    {
                        // Adding to rsp! jmp is the real end
                        // ... if any previous instruction does stack cleanup and register fixing
                        
                        auto epilogueInstruction = findEpilogueStart(currentBlock);
                        if (epilogueInstruction)
                            return std::make_pair(true, currentBlock);
                    }
                }
            }
            else if (lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_RET)
            {
                return std::make_pair(true, currentBlock);
            }

            for (std::shared_ptr<Node> child : { branchTargetBlock, currentBlock->GetNext() })
            {
                // Recursively search through children if they haven't been processed yet
                if (child && exploredNodes.count(child) == 0)
                {
                    auto pair = findLastBlock(child);
                    if (pair.first)
                        return pair;
                }
            }
            
            return std::make_pair(false, nullptr);
        };

        auto [explorationSuccess, lastNode] = findLastBlock(_front);
        if (!explorationSuccess)
            return {}; // empty optional

        auto epilogueStartInstruction = findEpilogueStart(lastNode);
        if (!epilogueStartInstruction)
            return {}; // empty optional

        return epilogueStartInstruction->Address;
    }
}

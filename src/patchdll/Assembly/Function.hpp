#ifndef ASSEMBLY_FUNCTION_HPP__
#define ASSEMBLY_FUNCTION_HPP__

#include "InstructionBlock.hpp"

#include <memory>
#include <optional>
#include <unordered_map>

namespace Assembly
{
    struct Function final
    {
        struct Node
        {
            Node(InstructionBlock const& block, std::shared_ptr<Node> parent) : Block(block), Previous(parent) { }

            InstructionBlock Block;
            std::shared_ptr<Node> Next;
            std::shared_ptr<Node> BranchTarget;
            std::shared_ptr<Node> Previous;
        };

        Function(uintptr_t address, ZydisDecoder& decoder);

        std::optional<uintptr_t> FindEpilogue() const;

    private:

        std::shared_ptr<Node> TryGetNode(uintptr_t address, std::shared_ptr<Node> parent);

        ZydisDecoder& _decoder;

        std::shared_ptr<Node> _front;
        std::unordered_map<uintptr_t, std::shared_ptr<Node>> _nodeCache;
    };
}

#endif // ASSEMBLY_FUNCTION_HPP__
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
            Node(InstructionBlock const& block) : Block(block) { }

            InstructionBlock Block;
            std::weak_ptr<Node> _next;
            std::weak_ptr<Node> _branchTarget;

            std::shared_ptr<Node> GetNext() const {
                return _next.lock();
            }

            std::shared_ptr<Node> GetBranchTarget() const {
                return _branchTarget.lock();
            }
        };

        Function(uintptr_t address);

        std::optional<uintptr_t> FindEpilogue() const;

    private:

        std::shared_ptr<Node> TryGetNode(uintptr_t address, std::shared_ptr<Node> parent);

        std::shared_ptr<Node> _front;
        std::unordered_map<uintptr_t, std::shared_ptr<Node>> _nodeCache;
    };
}

#endif // ASSEMBLY_FUNCTION_HPP__

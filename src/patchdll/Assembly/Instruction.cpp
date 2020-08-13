#include "Instruction.hpp"
#include "Decoder.hpp"

namespace Assembly
{
    Instruction::Instruction(uintptr_t address)
        : Address(address)
    {
        Assembly::Decoder::Decode(address, Decoded);
    }

    tcb::span<ZydisDecodedOperand const> Instruction::GetOperands() const
    {
        return { Decoded.operands, Decoded.operands + Decoded.operand_count };
    }

    ZydisDecodedOperand const* Instruction::GetOperand(std::function<bool(ZydisDecodedOperand const&)> operand) const
    {
        auto operands = GetOperands();
        auto itr = std::find_if(operands.begin(), operands.end(), [](ZydisDecodedOperand const& operand) {
            return operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
        });

        if (itr == operands.end())
            return nullptr;

        return itr;
    }
}
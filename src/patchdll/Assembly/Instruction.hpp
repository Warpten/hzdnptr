#ifndef ASSEMBLY_INSTRUCTION_HPP__
#define ASSEMBLY_INSTRUCTION_HPP__

#include "../span.hpp"

#include <Zydis/DecoderTypes.h>

#include <cstdint>
#include <functional>

namespace Assembly
{
    struct Instruction final
    {
        Instruction(uintptr_t address);

        tcb::span<ZydisDecodedOperand const> GetOperands() const;

        ZydisDecodedOperand const* GetOperand(std::function<bool(ZydisDecodedOperand const&)> operand) const;


        ZydisDecodedInstruction Decoded;
        uintptr_t Address;
    };
}

#endif // ASSEMBLY_INSTRUCTION_HPP__
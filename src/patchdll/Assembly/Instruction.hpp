#ifndef ASSEMBLY_INSTRUCTION_HPP__
#define ASSEMBLY_INSTRUCTION_HPP__

#include "../span.hpp"

#include <Zydis/Decoder.h>

#include <cstdint>
#include <functional>

namespace Assembly
{
    struct Instruction final
    {
        Instruction(uintptr_t address, ZydisDecoder& decoder);

        tcb::span<ZydisDecodedOperand const> GetOperands() const;

        ZydisDecodedOperand const* GetOperand(std::function<bool(ZydisDecodedOperand const&)> operand) const;


        ZydisDecodedInstruction Decoded;
        uintptr_t Address;

    private:
        ZydisDecoder& _decoder;
    };
}

#endif // ASSEMBLY_INSTRUCTION_HPP__
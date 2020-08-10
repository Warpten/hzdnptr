#ifndef ASSEMBLY_INSTRUCTION_BLOCK_HPP__
#define ASSEMBLY_INSTRUCTION_BLOCK_HPP__

#include "Instruction.hpp"

#include <Zydis/Decoder.h>

#include <vector>

namespace Assembly
{
    struct InstructionBlock final
    {
        InstructionBlock(uintptr_t address, ZydisDecoder& decoder);
        InstructionBlock(InstructionBlock const&) = default;
        InstructionBlock(InstructionBlock&&) = default;

        Instruction const& GetFirstInstruction() const;
        Instruction const& GetLastInstruction() const;

        std::vector<Instruction> const& GetInstructions() const { return _instructions; }

    private:
        ZydisDecoder& _decoder;
        std::vector<Instruction> _instructions;
    };
}

#endif // ASSEMBLY_INSTRUCTION_BLOCK_HPP__
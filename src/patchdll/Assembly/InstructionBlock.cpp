#include "InstructionBlock.hpp"

namespace Assembly
{
    InstructionBlock::InstructionBlock(uintptr_t address)
    {
        while (true)
        {
            Instruction const& lastInstruction = _instructions.emplace_back(address);

            if (lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_RET)
                break;

            if (lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_UNCOND_BR || lastInstruction.Decoded.meta.category == ZYDIS_CATEGORY_COND_BR)
                break;

            address += lastInstruction.Decoded.length;
        }
    }

    Instruction const& InstructionBlock::GetFirstInstruction() const
    {
        return _instructions.front();
    }

    Instruction const& InstructionBlock::GetLastInstruction() const
    {
        return *_instructions.rbegin();
    }
}

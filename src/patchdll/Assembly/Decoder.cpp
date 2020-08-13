#include "Decoder.hpp"

namespace Assembly
{
    /* static */ Decoder& Decoder::Instance()
    {
        static Decoder instance;
        return instance;
    }

    Decoder::Decoder()
    {
        ZydisDecoderInit(&_instance, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
    }

    /* static */ void Decoder::Decode(uintptr_t address, ZydisDecodedInstruction& instruction)
    {
        ZydisDecoderDecodeBuffer(&Instance()._instance, reinterpret_cast<uint8_t*>(address), ZYDIS_MAX_INSTRUCTION_LENGTH, &instruction);
    }
}

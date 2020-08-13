#ifndef ASSEMBLY_DECODER_HPP__
#define ASSEMBLY_DECODER_HPP__

#include <Zydis/Decoder.h>

namespace Assembly
{
    struct Decoder final
    {
    private:
        static Decoder& Instance();

        Decoder();

    public:
        static void Decode(uintptr_t address, ZydisDecodedInstruction& instruction);

    private:
        ZydisDecoder _instance;
    };
}

#endif // ASSEMBLY_DECODER_HPP__

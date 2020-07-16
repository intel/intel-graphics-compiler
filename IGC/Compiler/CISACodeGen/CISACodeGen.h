/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once
#include "Compiler/CodeGenPublic.h"
#include "visaBuilder_interface.h"
#include "../../inc/common/UFO/portable_compiler.h"

namespace USC
{
    struct SShaderStageBTLayout;
}

namespace llvm
{
    class Function;
}

namespace IGC
{

    enum e_mask : unsigned char
    {
        EMASK_Q1,
        EMASK_Q2,
        EMASK_Q3,
        EMASK_Q4,
        EMASK_H1,
        EMASK_H2,
    };

    enum e_varType : unsigned char
    {
        EVARTYPE_GENERAL,
        EVARTYPE_ADDRESS,
        EVARTYPE_PREDICATE,
        EVARTYPE_SURFACE,
        EVARTYPE_SAMPLER
    };

    enum e_alignment : unsigned char
    {
        // In the increasing alignment order, except AUTO
        // (AUTO: naturally aligned)
        EALIGN_BYTE,
        EALIGN_WORD,
        EALIGN_DWORD,
        EALIGN_QWORD,
        EALIGN_OWORD,
        EALIGN_HWORD,
        EALIGN_32WORD,
        EALIGN_64WORD,
        EALIGN_AUTO
    };

#define EALIGN_GRF EALIGN_HWORD
#define EALIGN_2GRF EALIGN_32WORD

    static const std::array<unsigned int, 8> alignmentSize =
    {
        1,
        2,
        4,
        8,
        16,
        32,
        64,
        128
    };

    class calignmentSize
    {
    public:
        unsigned int operator[](unsigned int idx)
        {
            return alignmentSize[idx];
        }
    };

    // XMACRO defining the CISA opCode
    // need to move to the CISA encoding file when breaking down code
#define DECLARE_CISA_OPCODE(opCode, name, visaname) \
    opCode,
    enum e_opcode
    {
#include "isaDef.def"
    };
#undef DECLARE_CISA_OPCODE

#define DECLARE_CISA_OPCODE(opCode, name, visaname) \
    visaname,
    __attr_unused static ISA_Opcode ConvertOpcode[] =
    {
    #include "isaDef.def"
    };
#undef DECLARE_CISA_OPCODE

    enum e_modifier : unsigned char
    {
        EMOD_NONE,
        EMOD_SAT,
        EMOD_ABS,
        EMOD_NEG,
        EMOD_NEGABS,
        EMOD_NOT,
    };

    enum e_predMode
    {
        EPRED_NORMAL,
        EPRED_ALL,
        EPRED_ANY,
    };

    enum e_instType : unsigned char
    {
        EINSTTYPE_INVALID,
        EINSTTYPE_ALU,
        EINSTTYPE_BRANCH,
        EINSTTYPE_URBWRITE,
        EINSTTYPE_SEND,
        EINSTTYPE_RENDERTARGETWRITE,
        EINSTTYPE_SAMPLE,
        EINSTTYPE_LD,
        EINSTTYPE_INFO,
        EINSTTYPE_GATHER4,
        EINSTTYPE_TYPED4_READ,
        EINSTTYPE_OWLOAD,
        EINSTTYPE_GATHER,
        EINSTTYPE_SCATTER,
        EINSTTYPE_SCATTER4,
        EINSTTYPE_TYPED4_WRITE,
        EINSTTYPE_EOT,
        EINSTTYPE_ADDRADD,
        EINSTTYPE_LDMS,
        EINSTTYPE_BARRIER,
        EINSTTYPE_FENCE,
    };

    enum e_predicate : unsigned char
    {
        EPREDICATE_EQ,
        EPREDICATE_NE,
        EPREDICATE_GT,
        EPREDICATE_GE,
        EPREDICATE_LT,
        EPREDICATE_LE,
    };

    enum e_predefSurface : unsigned char
    {
        ESURFACE_NORMAL,
        ESURFACE_SLM,
        ESURFACE_STATELESS,
        ESURFACE_BINDLESS,
    };

    enum e_predefSampler : unsigned char
    {
        ESAMPLER_NORMAL,
        ESAMPLER_BINDLESS,
    };

    enum e_barrierKind : unsigned char
    {
        EBARRIER_NORMAL,  // default (signal+wait)
        EBARRIER_SIGNAL,
        EBARRIER_WAIT
    };

    enum e_instance : unsigned char
    {
        EINSTANCE_UNSPECIFIED,
        EINSTANCE_FIRST_HALF,
        EINSTANCE_SECOND_HALF
    };

    class CVariable;

    struct ResourceDescriptor
    {
        CVariable* m_resource;
        e_predefSurface m_surfaceType;
        ResourceDescriptor() : m_resource(nullptr), m_surfaceType(ESURFACE_NORMAL) {}
    };

    struct SamplerDescriptor
    {
        CVariable* m_sampler;
        e_predefSampler m_samplerType;
        SamplerDescriptor() : m_sampler(nullptr), m_samplerType(ESAMPLER_NORMAL) {}
    };

}

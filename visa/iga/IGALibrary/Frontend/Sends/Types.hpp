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
#ifndef IGA_FRONTEND_SENDS_TYPES
#define IGA_FRONTEND_SENDS_TYPES

#include "../Formatter.hpp"

#include <cstdint>

namespace iga {
// An IR for send messages


// A scattered message takes a block of addresses and looks up several
// elements (ElemsPerChannel) for each address.   (I think the values are
// still loaded SOA.)
//


struct VectorMessage
{
    enum SM {
        INVALID = 0,
        // SIMD4x2 = 4,
        SIMD8   = 8,
        SIMD16  = 16,
    } simdMode;

    bool hasStatusReturn; // bit 13
    bool packedData; // bit 30 (0 = 32b/No, 1=16b yes)
    bool packedAddresses; // bit 31 (0 = 32b, 1=16b yes)
};

struct ScatteredMessage : VectorMessage
{
    enum DataType {
        BYTE  = 1,
        WORD  = 2, // not used
        DWORD = 4,
        QWORD = 8,
    } dataType;
    enum ElemsPerChan { // "DE" comes from the BXML identifiers
        DE1 = 1,
        DE2 = 2,
        DE4 = 4,
        DE8 = 8
    } elemsPerChannel;

    bool isConstant; // DCRO

    size_t numAddrRegisters() const { return 0; }
    size_t numDataRegisters() const { return 0; }
};

// typed and untyped messages
struct SurfaceMessage : VectorMessage
{
    enum ChMask {
        INVALID = 0,

        RGBA,
        GBA,
        RBA,
        BA,
        RGA,
        GA,
        RA,
        A,
        RGB,
        GB,
        RB,
        B,
        RG,
        G,
        R,
    } chMask;
    // SM3 (0 = SIMD4x2, 1 = SIMD16, 2=SIMD8) (super class)
};

struct BlockMessage
{
    enum SubType {
        DEFAULT,           // blk
        ALIGNED,           // ablk
        CONST,             // cblk
        UNALIGNED,         // ublk
        CONST_UNALIGNED,   // cublk
        SAMPLER_UNALIGNED, // sublk
    } subType;
    enum Size {
        B128 = 128, // OctWords
        B256 = 256  // HexWords
    } blockSize;

    enum Elems {
        X1  = 0x01,
        X1L = 0x11, // low part of GRF: only relevant for Oword
        X1H = 0x21, // high part of GRF: only releavant for Oword
        X2  = 0x02,
        X4  = 0x04,
        X8  = 0x08,
    } elems;

    size_t numAddrRegisters() const;
    size_t numDataRegisters() const;
};


enum AddrModel
{
    INVALID    = 0x0000,

    A64_FAMILY = 0x0100,
    A64_C      = 0x01FF,
    A64_NC     = 0x01FD,
    A64_SSO    = 0x01FC,

    A32_FAMILY = 0x0200,
    A32_C      = 0x02FF,
    A32_NC     = 0x02FD,
    A32_SSO    = 0x02FC,

    BTI_FAMILY = 0x0400,

    SLM        = 0x08FE,

    SCRATCH    = 0x1000,
};


struct Message
{
    enum Family {
        INVALID = 0,

        SCATTERED = 1,
        BLOCK,
        SURFACE, // typed untyped
        ATOMIC_FLOAT,
        ATOMIC_INTEGRAL,
        // ...
    } family;

    union {
        ScatteredMessage         scattered;
        BlockMessage             block;
        SurfaceMessage           surface;
    };

    enum Mode {
        READ = 1,
        WRITE = 2
    } mode;

    bool                         isSplit;
    bool                         isConditional;
    bool                         headerPresent;
    bool                         invalidateOnRead;

    uint32_t                     addrOffset; // scratch or SSBO
    AddrModel                    addrModel;

    int                          src0Len, src1Len; // mlen, xlen
    int                          dstLen; // rlen

    const char                  *mnemonic;

    size_t numAddrRegisters() const;
    size_t numDataRegisters() const;

    void formatOpcode(const Instruction &i, BasicFormatter &bf) const;
    void formatExecInfo(const Instruction &i, BasicFormatter &bf) const;
    void formatDst(Platform p, const Instruction &i, BasicFormatter &bf) const;
    void formatSrcs(Platform p, const Instruction &i, BasicFormatter &bf) const;
    // void formatExtraInstOpts(
    //    const Instruction &i,
    //    BasicFormatter &bf,
    //    bool &somethingEmitted) const;
    void formatExtraInstOpts( // TODO: hack
        const Instruction &i,
        std::vector<const char *> &output) const;
};


} // namespace
#endif

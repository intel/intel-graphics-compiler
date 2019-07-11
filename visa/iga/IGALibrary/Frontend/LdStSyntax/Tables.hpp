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
#ifndef IGA_FRONTEND_LDSTSYNTAX_TABLES_HPP
#define IGA_FRONTEND_LDSTSYNTAX_TABLES_HPP

#include "MTypes.hpp"
#include "../../IR/Types.hpp"

#include <cstdint>
#include <ostream>
#include <string>

namespace iga
{
    // parses the token stream into a raw bit (unshifted) value
    // the parse will return false if the parse soft-fails (fails without
    // consuming input), and return true otherwise.  If an error is detected
    // token stream, then a parse exception is raised
    // TODO: can we use this GenParser &p)
    //       #include "../KernelParser.hpp"
    //       means we need a dummy definition for formatter only library
    //       (since the parsing functionality is absent there)
    typedef bool (*MParseFunction)(
        const std::string &token,
        uint32_t &val);
    typedef bool (*MFormatFunction)(
        std::ostream &os,
        uint32_t val);
    typedef bool (*MDecodeExecSizeFunction)(
        uint32_t val,
        ExecSize &execSize);
    typedef bool (*MEncodeExecSizeFunction)(
        ExecSize execSize,
        uint32_t &val);

    // syntax mapping
    enum MFieldMapping {
        MFieldMappingNONE = 0,
        MFieldMappingExecSize,   // mapping via the ExecSize field
        MFieldMappingSubOp,      // ld.sc8[.one.of.these]
        MFieldMappingInstOpt,    // mapping via an instruction option
        MFieldMappingAddrOffset, // mapping via an address offset (e.g. a32o)
        MFieldMappingAddrModel,  // e.g. "a64" or "surf[3]" or "scratch[0x100]"
        MFieldMappingHeader,     // bit 19 (if the message supports a header)
    };
    struct MFieldType
    {
        int                       length; // the number of bits in this type
        const char               *name; // e.g. "DE"
        MFieldMapping             mapping;

        // used for MFieldMappingInstOpt
        const char               *instOptSymbol;
        // used for MFieldMappingSubOp
        MParseFunction            subopParse;    // resolves a lexeme to a value
        MFormatFunction           subopFormat;   // resolves value to lexeme
        // used for MFieldMappingExecSize
        MDecodeExecSizeFunction   execSizeDecode;
        MEncodeExecSizeFunction   execSizeEncode;
    };
    struct MField
    {
        int off;
        const MFieldType *type;
    };
    enum MKind
    {
        INVALID_MKIND = 0,

        LOAD = 1,
        STORE = 2,
        ATOMIC = 3,
    };
    enum MAddrModel
    {
        // for this type the top bits signal the address model category
        //   e.g. BTS, A32, A64, etc...
        // and the lower bits get used to actually encode the BTS value
        // that we wish to use.  Hence, the enumeration does two things.
        //   1. We use it to sets of legal address models for some format.
        //      a) MFormat's might have something like BTS_SLM_A32
        //         meaning BTI's, SLM, and A32 are all appropriate address
        //         models for that message
        //      b) When we actually parse the BTI value, we'll encode the
        //         value in the lower word.   Hence
        //          "surf[0x4]" will translate to (0x40000|0x4)
        //          "scratch[0x11]" would be (SCRATCH_FAMILY|0x11)
        //
        // NOTE. we treat scratch ops as an "address model" of it's own.
        INVALID_ADDR_MODEL = 0x00000000,
        //
        ///////////////////////////////////////////////////////////////////////
        // constants for clearer definitions below
        STATELESS_COHERENT = 0xFF,
        STATELESS_NON_COHERENT = 0xFD,
        STATELESS_SURFACE_OFFSET = 0xFC,
        SLM_BTI = 0xFE,
        //
        ///////////////////////////////////////////////////////////////////////
        // A32 stateless address models
        A32_FAMILY = 0x00010000,
        A32_CO = A32_FAMILY | STATELESS_COHERENT, // a32c[..] (coherent)
        A32_NC = A32_FAMILY | STATELESS_NON_COHERENT, // a32[..]  (non-coherent)
        A32_SO = A32_FAMILY | STATELESS_SURFACE_OFFSET, // a32o[..] (surface offset model)
        ///////////////////////////////////////////////////////////////////////
        // A64 stateless address models
        A64_FAMILY = 0x00020000,
        A64_CO = A64_FAMILY | STATELESS_COHERENT, // a64c[...]
        A64_NC = A64_FAMILY | STATELESS_NON_COHERENT, // a64[...]
        A64_SO = A64_FAMILY | STATELESS_SURFACE_OFFSET, // a64o[... + <OFF>]
        //
        // BTI family surf[...][...]
        BTI_0 = 0x00040000 + 0,
        BTI_1 = BTI_0 + 1,
        BTI_2 = BTI_0 + 2,
        BTI_3 = BTI_0 + 3,
        BTI_4 = BTI_0 + 4,
        BTI_5 = BTI_0 + 5,
        BTI_6 = BTI_0 + 6,
        BTI_7 = BTI_0 + 7,
        // ... hundreds of others, but we just show resolve the first few
        // explicitly for debugging common code
        BTI_LAST   = BTI_0 + 0xEF,
        BTS_FAMILY = BTI_0, // put this after BTI_0 so debugger favors "BTI_0"
        // of family symbol
        //
        ///////////////////////////////////////////////////////////////////////
        // SLM
        //
        // there's only one valid value here (SLM)
        // SLM_FAMILY indicates an MFormat may accept SLM
        SLM_FAMILY = 0x00080000,           // slm[...] family
        SLM        = (SLM_FAMILY|SLM_BTI), // slm[...] (the specific value)
        //
        ///////////////////////////////////////////////////////////////////////
        // the scratch message "model"
        //
        // same deal as BTS, we count indices as
        // suffixed with "HW" since they are 32B per scratch entry ([16:5])
        // (hex words)
        SCRATCH_HW0 = 0x00100000,
        SCRATCH_HW1 = SCRATCH_HW0 + 1,
        SCRATCH_HW2 = SCRATCH_HW0 + 2,
        SCRATCH_HW3 = SCRATCH_HW0 + 3,
        SCRATCH_HW4 = SCRATCH_HW0 + 4,
        SCRATCH_HW5 = SCRATCH_HW0 + 5,
        SCRATCH_HW6 = SCRATCH_HW0 + 6,
        SCRATCH_HW7 = SCRATCH_HW0 + 7,
        // ... up to 0xFFF (12 bits for scratch values)
        SCRATCH_FAMILY = SCRATCH_HW0, // scratch[...]
        //
        ///////////////////////////////////////////////////////////////////////
        // aliases for table definitions
        //
        // An MFormat uses this to mean BTI, SLM, and A32
        BTS_SLM_A32 = BTS_FAMILY | SLM_FAMILY | A32_FAMILY,
    };
    static bool isSLMBTI(int x) {return x == SLM_BTI;}
    static bool isStatelessBTI(int x) {
        return x == STATELESS_COHERENT ||
            x == STATELESS_NON_COHERENT ||
            x == STATELESS_SURFACE_OFFSET;
    }
    struct MFormat {
        Platform       platform;      // introduced
        MType          type;          // MSD_.... FIXME: use #include an autogen
        MKind          kind;          // ld / st
        const char    *mnemonic;      // e.g. "sc8"
        const MField   fields[8];     // the fields
        const int8_t   argIndices[4]; // ld.[ARGS] fields[index] of subop #i (-1 for the array end)
        MAddrModel     addrModel;     // the address models that this format supports
        SFID           sfid;          // DC1, DC0, etc...
        uint32_t       opcodeMask;    // the descriptor match mask
        uint32_t       opcodeValue;   // the value to match
    };

    const MFormat *GetMFormatTable(size_t &len);
}

#endif // IGA_FRONTEND_LDSTSYNTAX_TABLES_HPP

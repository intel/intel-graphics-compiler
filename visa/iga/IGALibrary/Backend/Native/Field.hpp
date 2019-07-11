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
#ifndef IGA_BACKEND_NATIVE_FIELD_HPP
#define IGA_BACKEND_NATIVE_FIELD_HPP

#define FIELD(NM,OFF,LEN) {NM, OFF, LEN}

#include "../../asserts.hpp"
#include "../../api/iga_bxml_ops.hpp"

#include <cstdint>
#include <cstring>
#include <string>

namespace iga
{
    // a field in the binary format that gets encoded or decoded
    struct Field
    {
        const char *name;
        int offset;
        int length;

        bool overlaps(const Field &f) const {
            if (length < f.length)
                return f.overlaps(*this);
            // test the end points of the smaller interval within the larger
            return (offset >= f.offset && offset < (f.offset + f.length)) ||
                ((offset + length - 1) >= f.offset &&
                 (offset + length - 1) < (f.offset + f.length));
        }
    };

    static bool operator== (const Field &f1, const Field &f2)
    {
        return f1.offset == f2.offset &&
            f1.length == f2.length &&
            strcmp(f1.name, f2.name) == 0;
    }
    static bool operator< (const Field &f1, const Field &f2)
    {
        if (f1.offset < f2.offset) {
            return true;
        } else if (f1.offset > f2.offset) {
            return false;
        } else {
            if (f1.length < f2.length) {
                return true;
            } else if (f1.length > f2.length) {
                return false;
            } else {
                return strcmp(f1.name, f2.name) < 0;
            }
        }
    }

    struct OperandFields {
        const Field &fTYPE;
        const Field *pfSRCMODS;
        const Field &fREGFILE;

        const Field &fREG;
        const Field &fSUBREG;
        const Field &fSPCACC;

        const Field *pfRGNVT0;
        const Field *pfRGNVT1;
        const Field *pfRGNWI;
        const Field &fRGNHZ;

        const Field *pfADDRMODE;
        const Field *pfADDRREG;
        const Field *pfADDROFF;

        const Field *pfSRCISIMM;
        const Field *pfSRCIMM32L; // also IMM16
        const Field *pfSRCIMM32H;
    };

    enum OpIx {
        IX_DST      = 0x40, // must not overlap with other bits
        IX_SRC0     = 0x10,
        IX_SRC1     = 0x20,
        IX_SRC2     = 0x30,
        OP_IX_TYPE_MASK = 0xF0,

        OP_IX_MASK  = 0x0F, // masks table index (unique across bas. and trn.)

        BAS      = 0x100,
        BAS_DST  = BAS | IX_DST  | 0,
        BAS_SRC0 = BAS | IX_SRC0 | 1,
        BAS_SRC1 = BAS | IX_SRC1 | 2,

        TER      = 0x200,
        TER_DST  = TER | IX_DST  | 3,
        TER_SRC0 = TER | IX_SRC0 | 4,
        TER_SRC1 = TER | IX_SRC1 | 5,
        TER_SRC2 = TER | IX_SRC2 | 6
    };
    static inline bool IsTernary(OpIx IX) { return (IX & OpIx::TER) == OpIx::TER; }
    static inline bool IsDst(OpIx IX) { return (IX & OpIx::IX_DST) == OpIx::IX_DST; }
    static inline int ToSrcIndex(OpIx IX) {
        IGA_ASSERT(!IsDst(IX),"ToSrcIndex(OpIx) on dst index");
        return ((IX & 0xF0)>>4) - 1;
    }
    static inline int ToFieldOperandArrayIndex(OpIx IX) {
        return (IX & OpIx::OP_IX_MASK);
    }

    static ::std::string ToStringOpIx(OpIx ix)
    {
        switch (ix & OpIx::OP_IX_TYPE_MASK) {
        case OpIx::IX_DST:  return "dst";
        case OpIx::IX_SRC0: return "src0";
        case OpIx::IX_SRC1: return "src1";
        case OpIx::IX_SRC2: return "src2";
        default: return "?";
        }
    }


    // a grouping of compaction information
    struct CompactedField {
        const Field       &index;
        const uint64_t    *values;
        size_t             numValues;
        const Field      **mappings;
        size_t             numMappings;
        const char       **meanings; // length is numValues
        // this is used for "what would this compaction word mean"
        std::string      (*format)(Op,uint64_t);

        // sums up with width of all the fields mapped
        size_t countNumBitsMapped() const {
            size_t entrySizeBits = 0;
            for (size_t k = 0; k < numMappings; k++)
                entrySizeBits += mappings[k]->length;
            return entrySizeBits;
        }
        bool isSrcImmField() const {return mappings == nullptr;}
    };

// Assumes existence of:
//   std::string [namespace::]Format_##SYM (uint64_t val)
// E.g. SYM=CMP_CTRLIX_2SRC references
//   std::string iga::pstg12::Format_CMP_CTRLIX_2SRC(uint64_t val);
#define MAKE_COMPACTED_FIELD(SYM)\
    extern std::string Format_##SYM (Op, uint64_t val);\
    \
    static_assert(sizeof(SYM ## _VALUES)/sizeof(SYM ## _VALUES[0]) == \
        sizeof(SYM ## _MEANINGS)/sizeof(SYM ## _MEANINGS[0]), \
        "mismatch in table sizes");\
    static const CompactedField CIX_ ## SYM {\
        SYM, \
        SYM ## _VALUES, sizeof(SYM ## _VALUES)/sizeof(SYM ## _VALUES[0]),\
        SYM ## _MAPPINGS, sizeof(SYM ## _MAPPINGS)/sizeof(SYM ## _MAPPINGS[0]),\
        SYM ## _MEANINGS,\
        &(Format_ ## SYM),\
    }
} // namespace

#endif /* IGA_BACKEND_NATIVE_FIELD_HPP */

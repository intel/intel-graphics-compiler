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
#ifndef IGA_BACKEND_NATIVE_MINST_HPP
#define IGA_BACKEND_NATIVE_MINST_HPP

#include "Field.hpp"
#include "../../bits.hpp"

#include <cstdint>

namespace iga
{
    // machine instruction
    struct MInst {
        MInst() {
            dw0 = dw1 = dw2 = dw3 = 0;
        }

        union {
            struct {uint32_t dw0, dw1, dw2, dw3; };
            struct {uint32_t dws[4];};
            struct {uint64_t qw0, qw1;};
            struct {uint64_t qws[2];};
        };

        bool testBit(int off) const {
            return getField(off,1) != 0;
        }
        uint64_t getField(int off, int len) const {
            return getBits(&qws[0], off, len);
        }
        uint64_t getField(const Field &f) const {
            return getField(f.offset, f.length);
        }
        // gets a fragmented field
        uint64_t getField(const Field &fLo, const Field &fHi) const {
            uint64_t lo = getField(fLo.offset, fLo.length);
            uint64_t hi = getField(fHi.offset, fHi.length);
            return ((hi<<fLo.length) | lo);
        }
        // gets a fragmented field from an array of fields
        // the fields are ordered low bit to high bit
        // we stop when we find a field with length 0
        template <int N>
        uint64_t getFields(const Field ff[N]) const
        {
            uint64_t bits = 0;

            int off = 0;
            for (int i = 0; i < N; i++) {
                if (ff[i].length == 0) {
                    break;
                }
                auto frag = getBits(qws, ff[i].offset, ff[i].length);
                bits |= frag << off;
                off += ff[i].length;
            }

            return bits;
        }

        // returns false if field is too small to hold the value
        bool setField(const Field &f, uint64_t val) {
            return setBits(qws, f.offset, f.length, val);
        }
        bool isCompact() const {return testBit(29);}
    };
    static_assert(sizeof(MInst) == 16, "MInst must be 16 bytes");
}

#endif // IGA_BACKEND_NATIVE_MINST_HPP

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
#ifndef _IGA_IMMVAL_HPP_
#define _IGA_IMMVAL_HPP_

#include <stdint.h>

namespace iga
{
    struct ImmVal {
        enum Kind {
            UNDEF,
            F16,
            F32,
            F64,

            S8,
            S16,
            S32,
            S64,

            U8,
            U16,
            U32,
            U64
        };

        union {
            // TODO: may be able to reduce to 32-bits and 64-bits
            uint16_t    f16;
            float       f32;
            double      f64;

            int8_t       s8;
            int16_t     s16;
            int32_t     s32;
            int64_t     s64;

            uint8_t      u8;
            uint16_t    u16;
            uint32_t    u32;
            uint64_t    u64 = 0;
        };
        Kind            kind = Kind::UNDEF;

        ImmVal& operator=(uint8_t x);
        ImmVal& operator=(int8_t x);
        ImmVal& operator=(uint16_t x);
        ImmVal& operator=(int16_t x);
        ImmVal& operator=(uint32_t x);
        ImmVal& operator=(int32_t x);
        ImmVal& operator=(uint64_t x);
        ImmVal& operator=(int64_t x);
        ImmVal& operator=(float x);
        ImmVal& operator=(double x);

        void Abs();
        void Negate();
    }; // class ImmVal
} // namespace iga
#endif // _IGA_IMMVAL_HPP_

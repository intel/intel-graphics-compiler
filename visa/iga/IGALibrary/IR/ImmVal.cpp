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
#include "ImmVal.hpp"
#include "../Frontend/Floats.hpp"

#include <cmath>

using namespace iga;


ImmVal& ImmVal::operator=(uint8_t x) {
    kind = U8;
    u64 = 0;
    u8 = x;
    return *this;
}
ImmVal& ImmVal::operator=(int8_t x) {
    kind = S8;
    u64 = 0;
    s8 = x;
    return *this;
}
ImmVal& ImmVal::operator=(uint16_t x) {
    kind = U16;
    u64 = 0;
    u16 = x;
    return *this;
}
ImmVal& ImmVal::operator=(int16_t x) {
    kind = S16;
    u64 = 0;
    s16 = x;
    return *this;
}
ImmVal& ImmVal::operator=(uint32_t x) {
    kind = U32;
    u64 = 0;
    u32 = x;
    return *this;
}
ImmVal& ImmVal::operator=(int32_t x) {
    kind = S32;
    u64 = 0;
    s32 = x;
    return *this;
}
ImmVal& ImmVal::operator=(uint64_t x) {
    kind = U64;
    u64 = x;
    return *this;
}
ImmVal& ImmVal::operator=(int64_t x) {
    kind = S64;
    s64 = x;
    return *this;
}
ImmVal& ImmVal::operator=(float x) {
    kind = F32;
    u64 = 0;
    f32 = x;
    return *this;
}
ImmVal& ImmVal::operator=(double x) {
    kind = F64;
    f64 = x;
    return *this;
}


void ImmVal::Abs() {
    switch (kind) {
    case Kind::F16:
        // the sign bit manually so that we can "negate" NaN values
        u16 &= ~IGA_F16_SIGN_BIT;
        break;
    case Kind::F32:
        // see F16
        u32 &= ~IGA_F32_SIGN_BIT;
        break;
    case Kind::F64:
        // see F64
        u64 &= ~IGA_F64_SIGN_BIT;
        break;

    case Kind::S8:
        s8 = s8 < 0 ? -s8 : s8;
        break;
    case Kind::S16:
        s32 = s16 < 0 ? -s16 : s16;
        break;
    case Kind::S32:
        s32 = s32 < 0 ? -s32 : s32;
        break;
    case Kind::S64:
        s64 = s64 < 0 ? -s64 : s64;
        break;
    default:
        break;
    }
}

void ImmVal::Negate() {
    switch (kind) {
    case Kind::F16:
        // N.B. we manually flip the sign bit
        // so that -{s,q}nan gives the right bit pattern on all HW
        u16 = IGA_F16_SIGN_BIT ^ u16;
        break;
    case Kind::F32:
        // see above
        u32 = IGA_F32_SIGN_BIT ^ u32;
        break;
    case Kind::F64:
        // see above
        u64 = IGA_F64_SIGN_BIT ^ u64;
        break;

    case Kind::S8:
        s8 = -s8;
        break;
    case Kind::S16:
        s16 = -s16;
        break;
    case Kind::S32:
        s32 = -s32;
        break;
    case Kind::S64:
        s64 = -s64;
        break;
    default:
        break;
    }
}
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

// Bit Instructions

uchar __builtin_spirv_OpBitReverse_i8(uchar Base)
{
    return  __builtin_IB_bfrev( (uint)Base) >> 24;
}

ushort __builtin_spirv_OpBitReverse_i16(ushort Base)
{
    return  __builtin_IB_bfrev( (uint)Base) >> 16;
}

uint __builtin_spirv_OpBitReverse_i32(uint Base)
{
    return __builtin_IB_bfrev(Base);
}

ulong __builtin_spirv_OpBitReverse_i64(ulong Base)
{
    return (__builtin_IB_bfrev((uint)Base) << 32) | __builtin_IB_bfrev((Base >> 32));
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, uchar, uchar, i8 ) 
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, ushort, ushort, i16 ) 
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, uint, uint, i32 ) 
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, ulong, ulong, i64 ) 


uchar __builtin_spirv_OpBitCount_i8(uchar Base)
{
    return __builtin_IB_popcount_1u8(Base);
}

uchar __builtin_spirv_OpBitCount_i16(ushort Base)
{
    return (uchar)__builtin_IB_popcount_1u16(Base);
}

uchar __builtin_spirv_OpBitCount_i32(uint Base)
{
    return (uchar)__builtin_IB_popcount_1u32(Base);
}

uchar __builtin_spirv_OpBitCount_i64(ulong Base)
{
    return __builtin_spirv_OpBitCount_i32(Base >> 32) + __builtin_spirv_OpBitCount_i32((uint)Base);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitCount, uchar, uchar, i8 ) 
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitCount, uchar, ushort, i16 ) 
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitCount, uchar, uint, i32 ) 
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitCount, uchar, ulong, i64 ) 


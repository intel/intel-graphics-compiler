/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef __IMF_INCLUDE_H__
#define __IMF_INCLUDE_H__

typedef unsigned char _iml_uint8_t;
typedef unsigned short _iml_uint16_t;
typedef unsigned int _iml_uint32_t;
typedef unsigned long _iml_v2_uint64_t;
typedef char _iml_int8_t;
typedef short _iml_int16_t;
typedef int _iml_int32_t;
typedef long _iml_int64_t;

typedef union
{
    _iml_uint32_t w;
    float f;
} int_float;
typedef union
{
    _iml_v2_uint64_t w;
    _iml_uint32_t w32[2];
    _iml_int32_t s32[2];
    double f;
} int_double;
typedef struct tag_iml_v2_dpdwords_t
{
    _iml_uint32_t lo_dword;
    _iml_uint32_t hi_dword;
} _iml_v2_dpdwords_t;
typedef union
{
    _iml_uint32_t hex[2];

    _iml_v2_dpdwords_t dwords;
    double fp;
} _iml_v2_dp_union_t;

typedef union
{
    _iml_uint32_t hex[1];

    float fp;
} _iml_v2_sp_union_t;

#endif

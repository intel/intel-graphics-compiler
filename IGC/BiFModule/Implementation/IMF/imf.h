/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

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

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __BIF_DEFINITIONS_CL__
#define __BIF_DEFINITIONS_CL__

#define FLOAT_SIGN_MASK         (0x80000000)  // used to be FLT_SIGN_MASK
#define FLOAT_EXPONENT_MASK     (0x7F800000)  // used to be EXPONENT_MASK
#define FLOAT_MANTISSA_MASK     (0x007FFFFF)  // used to be MANTISSA_MASK
#define FLOAT_QUIET_NAN         (0x7FFFFFFF)  // used to be QUIET_NAN
#define FLOAT_NEG_ONE_EXP_MASK  (0x3F000000)
#define FLOAT_IMPLICIT_BIT      (0x00800000)
#define FLOAT_BIAS              (127)
#define FLOAT_MANTISSA_BITS     (23)

#define HALF_SIGN_MASK          ((short)(0x8000))
#define HALF_EXPONENT_MASK      ((short)(0x7C00))
#define HALF_MANTISSA_MASK      ((short)(0x03FF))
#define HALF_QUIET_NAN          ((short)(0x7FFF))
#define HALF_NEG_ONE_EXP_MASK   ((short)(0x3800))
#define HALF_IMPLICIT_BIT       ((short)(0x0800))

#define DOUBLE_SIGN_MASK        (0x8000000000000000L)
#define DOUBLE_EXPONENT_MASK    (0x7FF0000000000000L)
#define DOUBLE_MANTISSA_MASK    (0x000FFFFFFFFFFFFFL)
#define DOUBLE_QUIET_NAN        (0x7FFFFFFFFFFFFFFFL)
#define DOUBLE_NEG_ONE_EXP_MASK (0x3FE0000000000000L)
#define DOUBLE_BIAS             (1023)
#define DOUBLE_MANTISSA_BITS    (52)

#define HALF_BITS               (16)
#define HALF_SIGN_BITS          (1)
#define HALF_EXPONENT_BITS      (5)
#define HALF_MANTISSA_BITS      (10)
#define HALF_BIAS               (15)
#define HALF_EXPONENT_RANGE     (31)

#define FLOAT_BITS              (32)
#define FLOAT_SIGN_BITS         (1)
#define FLOAT_EXPONENT_BITS     (8)
#define FLOAT_MANTISSA_BITS     (23)
#define FLOAT_BIAS              (127)
#define FLOAT_EXPONENT_RANGE    (255)

#define DOUBLE_BITS             (64)
#define DOUBLE_SIGN_BITS        (1)
#define DOUBLE_EXPONENT_BITS    (11)
#define DOUBLE_MANTISSA_BITS    (52)
#define DOUBLE_BIAS             (1023)

#define ONE_EIGHTY_OVER_PI_DBL   (as_double(0x404CA5DC1A63C1F8)) // 57.295779513082320876798154814105
#define ONE_EIGHTY_OVER_PI_FLT   (as_float(0x42652EE1))          // 57.295779513082320876798154814105f
#define ONE_EIGHTY_OVER_PI_HLF   (as_half((ushort)0x5329))       // 57.295779513082320876798154814105h

#define PI_OVER_ONE_EIGHTY_DBL   (as_double(0x3F91DF46A2529D39)) // 0.01745329251994329576923690768489
#define PI_OVER_ONE_EIGHTY_FLT   (as_float(0x3C8EFA35))          // 0.01745329251994329576923690768489f
#define PI_OVER_ONE_EIGHTY_HLF   (as_half((ushort)0x2478))       // 0.01745329251994329576923690768489h


#define ONE_OVER_LOG_10         (as_float(0x3EDE5BD8))          // 4.3429448190325180e-1f
#define ONE_OVER_LOG_10_DBL     (as_double(0x3fdbcb7b1526e50e)) // 0.4342944819032518276511289

#define ONE_OVER_LOG2_10        (as_float(0x3e9a209b))          // 0.30103000998497009f
#define ONE_OVER_LOG2_10_DBL    (as_double(0x3fd34413509f79ff)) // 0.3010299956639811952137388

#define ONE_OVER_LOG_2_DBL      (as_double(0x3ff71547652b82fe)) // 1.4426950408889634073599246

#define ONE_OVER_LOG10_2        (as_float(0x40549A78))          // 3.321928094887362f
#define ONE_OVER_LOG10_2_DBL    (as_double(0x400a934f0979a371)) // 3.3219280948873623478703194

#define _TWO_25                 (0x4c000000)

#define MINNORM                 (0x00800000)
#define MAXNORM                 (0x7f7fffff)

#endif // __BIF_DEFINITIONS_CL__

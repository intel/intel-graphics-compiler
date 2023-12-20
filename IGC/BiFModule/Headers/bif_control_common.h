/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#ifndef __BIF_FLAG_CONTROL_COMMON_H__
#define __BIF_FLAG_CONTROL_COMMON_H__

#define _STR(x) #x
#define STR(x) _STR(x)

#define _CON(x, y) x ## y
#define CON(x, y) _CON(x, y)

#define BIF_FLAG_CTRL_PREFIX __bif_flag_

// BiF flag control token name
#define BIF_FLAG_CTRL_N(BIF_FLAG_N) \
   CON(BIF_FLAG_CTRL_PREFIX, BIF_FLAG_N)

// BiF flag control token name as string
#define BIF_FLAG_CTRL_N_S(BIF_FLAG_N_S) \
   STR(BIF_FLAG_CTRL_N(BIF_FLAG_N_S))

// BiF flag control token check with value
#define BIF_FLAG_CTRL_CHECK_EQ(BIF_FLAG_N, VALUE_CHECK) \
   ( BIF_FLAG_CTRL_N(BIF_FLAG_N) == VALUE_CHECK )

// Get BiF flag control token
#define BIF_FLAG_CTRL_GET(BIF_FLAG_N) \
   BIF_FLAG_CTRL_N(BIF_FLAG_N)

#ifdef BIF_COMPILATION
#pragma OPENCL EXTENSION __cl_clang_bitfields : enable
#include "stdint.h"

#include "../../../inc/common/igfxfmid.h"
#endif

#endif // __BIF_FLAG_CONTROL_COMMON_H__

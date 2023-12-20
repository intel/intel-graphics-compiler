/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#ifndef __BIF_CONTROL_H__
#define __BIF_CONTROL_H__

#define BIF_COMPILATION

#include "bif_control_common.h"

#define BIF_FLAG_CONTROL(BIF_FLAG_TYPE, BIF_FLAG_NAME)     \
    extern __constant BIF_FLAG_TYPE BIF_FLAG_CTRL_N(BIF_FLAG_NAME);


#include "bif_flag_controls.h"

#endif // __BIF_CONTROL_H__

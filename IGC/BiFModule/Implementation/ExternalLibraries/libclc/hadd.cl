/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../../../Headers/spirv.h"

INLINE OVERLOADABLE long libclc_hadd(long x, long y) {
    return (x >> (long)1) + (y >> (long)1) + (x & y & (long)1);
}

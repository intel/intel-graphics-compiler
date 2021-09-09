/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"
#include "mul_hi.cl"

INLINE OVERLOADABLE long libclc_mad_sat(long x, long y, long z) {
    long hi = libclc_mul_hi(x, y);
    ulong ulo = x * y;
    long  slo = x * y;
    /* Big overflow of more than 2 bits, add can't fix this */
    if (((x < 0) == (y < 0)) && hi != 0)
        return LONG_MAX;
    /* Low overflow in mul and z not neg enough to correct it */
    if (hi == 0 && ulo >= LONG_MAX && (z > 0 || (ulo + z) > LONG_MAX))
        return LONG_MAX;
    /* Big overflow of more than 2 bits, add can't fix this */
    if (((x < 0) != (y < 0)) && hi != -1)
        return LONG_MIN;
    /* Low overflow in mul and z not pos enough to correct it */
    if (hi == -1 && ulo <= ((ulong)LONG_MAX + 1UL) && (z < 0 || z < (LONG_MAX - ulo)))
        return LONG_MIN;
    /* We have checked all conditions, any overflow in addition returns
     * the correct value */
    return ulo + z;
}

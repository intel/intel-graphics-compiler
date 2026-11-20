/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Declarations of the functions defined in mul_hilo.cl.

#ifndef __MUL_HILO_H__
#define __MUL_HILO_H__

ulong ___intc_umul_hilo(ulong sourceA, ulong sourceB, __private ulong *destLow);

long ___intc_mul_hilo(long sourceA, long sourceB, __private ulong *destLow);

#endif // __MUL_HILO_H__

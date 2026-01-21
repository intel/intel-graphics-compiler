/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///
/// \file
/// \brief Implementation of BiF intrinsics for BFloat16 data type support.
///
/// This file follows the same structure as IBiF_Impl.cl, but provides
/// implementations for bfloat data type.
///
/// When the bfloat16 extension is not enabled, the builtin functions defined
/// in this file will not be available.
///
#if defined(IGC_SPV_KHR_bfloat16) && defined(IGC_SPV_INTEL_bfloat16_arithmetic)

#include "spirv_bfloat.h"
#include "spirv_math_bfloat.h"

// Generic Header
#include "IBiF_Header.cl"

// Math builtin functions
#include "IBiF_Math_Common.cl"

// Arithmetic opcodes
#include "arithmetic.cl"

// Atomic opcodes
#include "atomic.cl"

// Relational opcodes
#include "relational.cl"

// Vload opcodes
#include "vload.cl"

// Vstore opcodes
#include "vstore.cl"

#endif // defined(IGC_SPV_KHR_bfloat16) && defined(IGC_SPV_INTEL_bfloat16_arithmetic)

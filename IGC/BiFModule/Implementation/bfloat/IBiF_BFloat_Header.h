/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Common declarations for the BFloat16 BiF sources.

#ifndef _IBIF_BFLOAT_HEADER_
#define _IBIF_BFLOAT_HEADER_

#include "spirv_bfloat.h"
#include "spirv_math_bfloat.h"

// Generic Header
#include "IBiF_Header.h"

// Helpers shared across BiF modules.
OVERLOADABLE int __intel_relaxed_isnan(bfloat x);

#endif // _IBIF_BFLOAT_HEADER_

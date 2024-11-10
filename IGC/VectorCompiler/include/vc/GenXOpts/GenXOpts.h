/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_GENXOPTS_GENXOPTS_H
#define VC_GENXOPTS_GENXOPTS_H

#if LLVM_VERSION_MAJOR < 16
#include "GenXOptsLegacyPM.h"
#else
#include "GenXOptsNewPM.h"
#endif

#endif // VC_GENXOPTS_GENXOPTS_H

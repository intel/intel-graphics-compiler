/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

typedef struct _VISA_GenVar VISA_GenVar;
typedef struct _VISA_AddrVar VISA_AddrVar;
typedef struct _VISA_PredVar VISA_PredVar;
typedef struct _VISA_SamplerVar VISA_SamplerVar;
typedef struct _VISA_SurfaceVar VISA_SurfaceVar;
typedef struct _VISA_VMEVar VISA_VMEVar;
typedef struct _VISA_LabelVar VISA_LabelVar;
typedef struct _VISA_FileVar VISA_FileVar;

typedef struct _VISA_LabelOpnd VISA_LabelOpnd;
typedef struct _VISA_VectorOpnd VISA_VectorOpnd;
typedef struct _VISA_RawOpnd VISA_RawOpnd;
typedef struct _VISA_PredOpnd VISA_PredOpnd;
typedef struct _VISA_StateOpndHandle VISA_StateOpndHandle;

#include "JitterDataStruct.h"
#include "RelocationInfo.h"
#include "VISABuilderAPIDefinition.h"
#include "inc/common/sku_wa.h"
#include "visa_igc_common_header.h"
#include "visa_wa.h"

namespace vISA {
// Entry point to the vISA finalizer. Once a vISA builder is created, one can
// create kernels, add variables and append instructions, and compile it to
// native ISA.
int CreateVISABuilder(VISABuilder *&builder, vISABuilderMode mode,
                      VISA_BUILDER_OPTION builderOption,
                      TARGET_PLATFORM platform, int numArgs,
                      const char *flags[], const WA_TABLE *pWaTable);
// Destroy the vISA builder and release any internal resources used by it. Note
// that it does not free compilation output such as the kernel binary and debug
// info, as they may have longer life time than the builder itself.
int DestroyVISABuilder(VISABuilder *&builder);

// Interface to free the kernel ISA and debug info binary.
void freeBlock(void *ptr);
} // namespace vISA

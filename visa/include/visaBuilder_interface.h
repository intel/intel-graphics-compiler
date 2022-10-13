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

extern "C" int CreateVISABuilder(VISABuilder *&builder, vISABuilderMode mode,
                                 VISA_BUILDER_OPTION builderOption,
                                 TARGET_PLATFORM platform, int numArgs,
                                 const char *flags[], const WA_TABLE *pWaTable);
extern "C" int DestroyVISABuilder(VISABuilder *&builder);

/**
 *
 *  Interface to free the kernel binary allocated by the vISA finalizer
 */
extern "C" void freeBlock(void *ptr);

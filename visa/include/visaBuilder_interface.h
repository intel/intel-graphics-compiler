/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "JitterDataStruct.h"
#include "RelocationInfo.h"
#include "VISABuilderAPIDefinition.h"
#include "inc/common/sku_wa.h"
#include "visa_igc_common_header.h"
#include "visa_wa.h"

// Entry point to the vISA finalizer. Once a vISA builder is created, one can
// create kernels, add variables and append instructions, and compile it to
// native ISA.
extern "C" int CreateVISABuilder(VISABuilder *&builder, vISABuilderMode mode,
                                 VISA_BUILDER_OPTION builderOption,
                                 TARGET_PLATFORM platform, int numArgs,
                                 const char *flags[], const WA_TABLE *pWaTable);
// Destroy the vISA builder and release any internal resources used by it. Note
// that it does not free compilation output such as the kernel binary and debug
// info, as they may have longer life time than the builder itself.
extern "C" int DestroyVISABuilder(VISABuilder *&builder);

// Interface to free the kernel ISA and debug info binary.
extern "C" void freeBlock(void *ptr);
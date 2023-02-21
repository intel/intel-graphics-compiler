/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Common_ISA_framework.h"
#include "VISAKernel.h"
#include "inc/common/sku_wa.h"
#include "visa_igc_common_header.h"

extern "C" VISA_BUILDER_API int
CreateVISABuilder(VISABuilder *&builder, vISABuilderMode mode,
                  VISA_BUILDER_OPTION builderOption, TARGET_PLATFORM platform,
                  int numArgs, const char *flags[], const WA_TABLE *pWaTable) {
  if (builder)
    return VISA_FAILURE;
  CISA_IR_Builder *cisa_builder = nullptr;
  int status = CISA_IR_Builder::CreateBuilder(
      cisa_builder, mode, builderOption, platform, numArgs, flags, pWaTable);
  builder = static_cast<VISABuilder *>(cisa_builder);

  return status;
}

extern "C" VISA_BUILDER_API int DestroyVISABuilder(VISABuilder *&builder) {
  CISA_IR_Builder *cisa_builder = static_cast<CISA_IR_Builder *>(builder);
  if (!cisa_builder)
    return VISA_FAILURE;
  int status = CISA_IR_Builder::DestroyBuilder(cisa_builder);
  return status;
}
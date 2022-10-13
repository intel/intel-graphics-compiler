/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// PatchInfo linker.
//

#pragma once

#ifndef __CM_FC_PATCHINFO_LINKER_H__
#define __CM_FC_PATCHINFO_LINKER_H__

#include "cm_fc_ld.h"

#include "PatchInfoRecord.h"

bool linkPatchInfo(cm::patch::Collection &C, std::size_t NumKernels,
                   cm_fc_kernel_t *Kernels, const char *Options);

#endif // __CM_FC_PATCHINFO_LINKER_H__

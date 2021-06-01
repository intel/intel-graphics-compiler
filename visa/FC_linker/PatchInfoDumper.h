/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// PatchInfo dumper.
//

#pragma once

#ifndef __CM_FC_PATCHINFO_DUMPER_H__
#define __CM_FC_PATCHINFO_DUMPER_H__

#include <cstddef>
#include <cstdio>

void dumpPatchInfo(void *Buf, std::size_t Size, void *Bin, std::size_t BinSize,
                   std::FILE *fp = nullptr);

#endif // __CM_FC_PATCHINFO_DUMPER_H__

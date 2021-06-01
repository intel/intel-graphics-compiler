/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// PatchInfo reader.
//

#pragma once

#ifndef __CM_FC_PATCHINFO_READER_H__
#define __CM_FC_PATCHINFO_READER_H__

#include <cstddef>

#include "PatchInfoRecord.h"

bool readPatchInfo(const char *Buf, std::size_t Size, cm::patch::Collection &C);

#endif /* __CM_FC_PATCHINFO_READER_H__ */

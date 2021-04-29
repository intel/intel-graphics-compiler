/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"

namespace CIF {

namespace Build {

#if defined CIF_ULT
Version_t GetUltVersion();
#endif

inline Version_t GetBinaryVersion() {
#if defined GIT
#elif defined CIF_ULT
  return GetUltVersion();
#else
  return CIF::UnknownVersion;
#endif
}
}
}

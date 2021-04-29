/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

namespace SPIRVDebugInfo {

enum Entrypoints {
#define _OCL_EXT_OP(name, num) name = num,
#include "SPIRV.DebugInfofuncs.h"
#undef _OCL_EXT_OP
};

}; // end namespace


/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_IGCDEPS_SHADERDUMP_H
#define VC_IGCDEPS_SHADERDUMP_H

#include "vc/Support/ShaderDump.h"

#include <iStdLib/types.h>

#include <common/shaderHash.hpp>

#include <memory>

namespace vc {
std::unique_ptr<ShaderDumper> createVC_IGCFileDumper(const ShaderHash &Hash);
} // namespace vc

#endif

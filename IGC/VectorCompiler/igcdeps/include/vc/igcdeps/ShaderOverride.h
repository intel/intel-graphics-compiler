/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_IGCDEPS_SHADEROVERRIDE_H
#define VC_IGCDEPS_SHADEROVERRIDE_H

#include "vc/Support/ShaderOverride.h"
#include "inc/common/igfxfmid.h"

#include <memory>


namespace vc {
std::unique_ptr<ShaderOverrider>
createVC_IGCShaderOverrider(ShaderHash const &Hash, PLATFORM const &Platform);
} // namespace vc

#endif

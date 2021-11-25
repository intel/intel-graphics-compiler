/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_SUPPORT_SHADEROVERRIDE_H
#define VC_SUPPORT_SHADEROVERRIDE_H

#include <memory>

namespace llvm {
class StringRef;
} // namespace llvm

namespace vc {

class ShaderOverrider {
public:
  enum class Extensions { VISAASM, ASM, DAT, LL };

  virtual bool override(void *&GenXBin, int &GenXBinSize,
                        llvm::StringRef FunctionName, Extensions Ext) const = 0;

  virtual ~ShaderOverrider() = default;
};

std::unique_ptr<ShaderOverrider> createDefaultshaderOverrider();

} // namespace vc

#endif

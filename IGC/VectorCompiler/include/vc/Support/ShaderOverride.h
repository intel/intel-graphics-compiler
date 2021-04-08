/*========================== begin_copyright_notice ============================

Copyright (c) 2021-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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
                        llvm::StringRef ShaderName, Extensions Ext) const = 0;

  virtual ~ShaderOverrider() = default;
};

std::unique_ptr<ShaderOverrider> createDefaultshaderOverrider();

} // namespace vc

#endif

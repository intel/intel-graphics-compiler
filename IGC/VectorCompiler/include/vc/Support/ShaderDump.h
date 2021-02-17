/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

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

#ifndef VC_SUPPORT_SHADERDUMP_H
#define VC_SUPPORT_SHADERDUMP_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <string>

namespace llvm {
class Module;
}

namespace vc {

class ShaderDumper {
public:
  // Main methods, just dump either binary data or text.
  virtual void dumpBinary(llvm::ArrayRef<char> Binary,
                          llvm::StringRef DumpName) = 0;
  virtual void dumpText(llvm::StringRef Text, llvm::StringRef DumpName) = 0;

  // Convenience method to dump module.
  virtual void dumpModule(const llvm::Module &M, llvm::StringRef DumpName) = 0;

  // Hack required for finalizer dumps since it can dump only to
  // specified file and not to generic stream.
  // Return the same string by default.
  virtual std::string composeDumpPath(llvm::StringRef DumpName) const {
    return DumpName.str();
  }

  virtual ~ShaderDumper() = default;
};

std::unique_ptr<ShaderDumper> createDefaultShaderDumper();
} // namespace vc

#endif

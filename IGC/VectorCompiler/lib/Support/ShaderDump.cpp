/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "vc/Support/ShaderDump.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace {
class FileShaderDumper : public vc::ShaderDumper {
public:
  void dumpBinary(ArrayRef<char> Binary, StringRef DumpName) override {
    writeToFile(DumpName, [Binary](llvm::raw_fd_ostream &OS) {
      OS.write(Binary.data(), Binary.size());
    });
  }
  void dumpText(StringRef Text, StringRef DumpName) override {
    writeToFile(DumpName, [Text](llvm::raw_fd_ostream &OS) { OS << Text; });
  }
  void dumpModule(const Module &M, StringRef DumpName) override {
    writeToFile(DumpName,
                [&M](llvm::raw_fd_ostream &OS) { M.print(OS, nullptr); });
  }

private:
  template <typename F> void writeToFile(StringRef DumpName, F Writer) const {
    int FD;
    auto EC = llvm::sys::fs::openFileForWrite(DumpName, FD);
    // Silently return, nothing critical if debug dump fails.
    if (EC)
      return;
    llvm::raw_fd_ostream Stream{FD, /*shouldClose=*/true};
    Writer(Stream);
  }
};
} // namespace

std::unique_ptr<vc::ShaderDumper> vc::createDefaultShaderDumper() {
  return std::make_unique<FileShaderDumper>();
}

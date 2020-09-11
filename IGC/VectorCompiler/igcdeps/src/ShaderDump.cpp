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
//
// This file implements shader dumping facilities using standard IGC
// functionality like debug dumps and enviroment variables.
//
// IGC has debug dump type classification and some types are reused here
// to provide outputs. For now, these types are picked to produce correct
// output format for each type of dump:
//
//   ASM_BC -- general binary data;
//   DBG_MSG_TEXT -- general text data;
//   TRANSLATED_IR_TEXT -- LLVM IR text.
//
// XXX: above types can be revised to extend classification for general-
// purpose dump types without special semantics.
//
//===---------------------------------------------------------------------===//

#include "vc/igcdeps/ShaderDump.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <common/debug/Dump.hpp>

#include <iomanip>
#include <sstream>
#include <string>

namespace {
class VC_IGCFileDumper : public vc::ShaderDumper {
  // Partially filled dump name.
  IGC::Debug::DumpName DumpPrefix;

public:
  VC_IGCFileDumper(const ShaderHash &Hash);

  void dumpBinary(llvm::ArrayRef<char> Binary,
                  llvm::StringRef DumpName) override;
  void dumpText(llvm::StringRef Text, llvm::StringRef DumpName) override;

  void dumpModule(const llvm::Module &M, llvm::StringRef DumpName) override;

  std::string composeDumpPath(llvm::StringRef DumpName) const override;

private:
  template <IGC::Debug::DumpType DumpTy, typename F>
  void writeToFile(llvm::StringRef DumpName, F Writer) const;
};
} // namespace

VC_IGCFileDumper::VC_IGCFileDumper(const ShaderHash &Hash)
    : DumpPrefix{IGC::Debug::DumpName("VC").Hash(Hash)} {}

static IGC::Debug::DumpName addDumpPostfix(const IGC::Debug::DumpName &Name,
                                           llvm::StringRef Postfix) {
  return Name.PostFix(Postfix.str());
}

template <IGC::Debug::DumpType DumpTy, typename F>
void VC_IGCFileDumper::writeToFile(llvm::StringRef DumpName, F Writer) const {
  IGC::Debug::Dump Dumper{addDumpPostfix(DumpPrefix, DumpName), DumpTy};
  Writer(Dumper.stream());
}

void VC_IGCFileDumper::dumpBinary(llvm::ArrayRef<char> Binary,
                                  llvm::StringRef DumpName) {
  writeToFile<IGC::Debug::DumpType::ASM_BC>(
      DumpName, [Binary](llvm::raw_ostream &OS) {
        OS.write(Binary.data(), Binary.size());
      });
}

void VC_IGCFileDumper::dumpText(llvm::StringRef Text,
                                llvm::StringRef DumpName) {
  writeToFile<IGC::Debug::DumpType::DBG_MSG_TEXT>(
      DumpName, [Text](llvm::raw_ostream &OS) { OS << Text; });
}

void VC_IGCFileDumper::dumpModule(const llvm::Module &M,
                                  llvm::StringRef DumpName) {
  writeToFile<IGC::Debug::DumpType::TRANSLATED_IR_TEXT>(
      DumpName, [&M](llvm::raw_ostream &OS) { M.print(OS, nullptr); });
}

std::string VC_IGCFileDumper::composeDumpPath(llvm::StringRef DumpName) const {
  return addDumpPostfix(DumpPrefix, DumpName).str();
}

namespace vc {
std::unique_ptr<ShaderDumper> createVC_IGCFileDumper(const ShaderHash &Hash) {
  return std::make_unique<VC_IGCFileDumper>(Hash);
}
} // namespace vc

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Support/ShaderDump.h"
#include "vc/Support/BackendConfig.h"

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

  void dumpCos(StringRef Contents, StringRef DumpName) override {
    dumpText(Contents, DumpName);
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

static void writeContentToFile(const Twine &Name, llvm::ArrayRef<char> Blob) {
  std::error_code EC;
  // no error handling since this is debug output
  raw_fd_ostream OS(Name.str(), EC);
  OS.write(Blob.data(), Blob.size());
}

std::string vc::legalizeShaderDumpName(const llvm::Twine &FileName) {
  std::string Result = FileName.str();
  std::replace_if(Result.begin(), Result.end(),
                  [](auto c) { return (!std::isalnum(c) && c != '.'); }, '_');
  return Result;
}

void vc::produceAuxiliaryShaderDumpFile(const llvm::GenXBackendConfig &BC,
                                        const llvm::Twine &OutputName,
                                        llvm::ArrayRef<char> Blob) {
  std::string LegalizedName = legalizeShaderDumpName(OutputName);
  if (BC.hasShaderDumper())
    BC.getShaderDumper().dumpBinary(Blob, LegalizedName);
  else
    writeContentToFile(LegalizedName, Blob);
}

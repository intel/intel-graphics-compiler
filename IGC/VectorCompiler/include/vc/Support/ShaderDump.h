/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_SUPPORT_SHADERDUMP_H
#define VC_SUPPORT_SHADERDUMP_H

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <string>

namespace llvm {
class Module;
class GenXBackendConfig;
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

  virtual void dumpCos(llvm::StringRef Contents, llvm::StringRef DumpName) = 0;

  // Hack required for finalizer dumps since it can dump only to
  // specified file and not to generic stream.
  // Return the same string by default.
  virtual std::string composeDumpPath(llvm::StringRef DumpName) const {
    return DumpName.str();
  }

  virtual ~ShaderDumper() = default;
};

std::unique_ptr<ShaderDumper> createDefaultShaderDumper();

std::string legalizeShaderDumpName(const llvm::Twine &FileName);

void produceAuxiliaryShaderDumpFile(const llvm::GenXBackendConfig &BC,
                                    const llvm::Twine &OutputName,
                                    const llvm::ArrayRef<char> Blob);

inline void produceAuxiliaryShaderDumpFile(const llvm::GenXBackendConfig &BC,
                                           const llvm::Twine &OutputName,
                                           const llvm::StringRef Blob) {
  return produceAuxiliaryShaderDumpFile(BC, OutputName,
                                        {Blob.begin(), Blob.end()});
}

} // namespace vc

#endif

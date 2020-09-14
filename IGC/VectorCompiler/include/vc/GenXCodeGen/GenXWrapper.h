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

#pragma once

#include "vc/Support/ShaderDump.h"

#include <JitterDataStruct.h>
#include <RelocationInfo.h>

#include <inc/common/sku_wa.h>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace vc {

namespace ocl {

enum class ArgKind {
  General,
  LocalSize,  // IMPLICIT_LOCAL_SIZE
  GroupCount, // IMPLICIT_NUM_GROUPS
  Buffer,     // 1D buffer
  SVM,        // stateless global pointer
  Sampler,
  Image1d,
  Image2d,
  Image3d,
  PrintBuffer,
  PrivateBase
};

enum class ArgAccessKind { None, ReadOnly, WriteOnly, ReadWrite };

struct ArgInfo {
  ArgKind Kind;
  ArgAccessKind AccessKind;
  unsigned Index;
  unsigned Offset;
  unsigned SizeInBytes;
  unsigned BTI;
};

struct TableInfo {
  void *Buf = nullptr;
  uint32_t Size = 0;
  uint32_t NumEntries = 0;
};

// This data partially duplicates KernelInfo data.
// It exists due to OCLBinary to ZEBinary transition period.
struct ZEBinaryInfo {
  struct SymbolsInfo {
    using ZESymEntrySeq = std::vector<vISA::ZESymEntry>;
    ZESymEntrySeq Functions;
    ZESymEntrySeq Local;
    // for now only function and local symbols are used
  };
  using ZERelocEntrySeq = std::vector<vISA::ZERelocEntry>;
  ZERelocEntrySeq Relocations;
  SymbolsInfo Symbols;
};

// Mirror of cmc_kernel_info that owns its data.
struct KernelInfo {
  std::string Name;
  std::vector<ArgInfo> Args;
  std::vector<std::string> PrintStrings;
  bool HasGroupID;
  bool HasBarriers;
  bool HasReadWriteImages;
  unsigned SLMSize;
  unsigned ThreadPrivateMemSize;
  unsigned StatelessPrivateMemSize;
  unsigned GRFSizeInBytes;

  TableInfo RelocationTable;
  TableInfo SymbolTable;
  ZEBinaryInfo ZEBinInfo;
};

struct GTPinInfo {
  std::vector<char> GTPinBuffer;
};

struct CompileInfo {
  KernelInfo KernelInfo;
  FINALIZER_INFO JitInfo;
  GTPinInfo GtpinInfo;
  std::vector<char> GenBinary;
  std::vector<char> DebugInfo;
};

struct CompileOutput {
  std::vector<CompileInfo> Kernels;
  unsigned PointerSizeInBytes;
};

} // namespace ocl

namespace cm {
struct CompileOutput {
  std::string IsaBinary;
};
} // namespace cm

using CompileOutput = std::variant<cm::CompileOutput, ocl::CompileOutput>;

enum class FileType {
  SPIRV, SOURCE
};

enum class OptimizerLevel { None, Full };

enum class BinaryKind { CM, OpenCL, ZE };

struct CompileOptions {
  FileType FType = FileType::SPIRV;
  std::string CPUStr;
  std::unique_ptr<WA_TABLE> WATable = nullptr;
  // Optional shader dumper.
  std::unique_ptr<ShaderDumper> Dumper = nullptr;

  // Api accessible options.
  bool NoVecDecomp = false;
  OptimizerLevel OptLevel = OptimizerLevel::Full;
  llvm::Optional<unsigned> StackMemSize;

  // Internal options.
  std::string FeaturesString; // format is: [+-]<feature1>,[+-]<feature2>,...
  BinaryKind Binary = BinaryKind::OpenCL;
  bool DumpIsa = false;
  bool DumpIR = false;
  bool DumpAsm = false;
  bool TimePasses = false;
};

class ExternalData {
  std::unique_ptr<llvm::MemoryBuffer> OCLGenericBIFModule;

public:
  explicit ExternalData(std::unique_ptr<llvm::MemoryBuffer> GenericModule)
      : OCLGenericBIFModule{std::move(GenericModule)} {
    IGC_ASSERT_MESSAGE(OCLGenericBIFModule,
                       "wrong argument: no memory buffer was provided");
  }

  const llvm::MemoryBuffer &getOCLGenericBIFModule() const {
    return *OCLGenericBIFModule;
  }
};

llvm::Expected<CompileOutput> Compile(llvm::ArrayRef<char> Input,
                                      const CompileOptions &Opts,
                                      const ExternalData &ExtData);

llvm::Expected<CompileOptions> ParseOptions(llvm::StringRef ApiOptions,
                                            llvm::StringRef InternalOptions);
} // namespace vc

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

#include <JitterDataStruct.h>
#include <inc/common/sku_wa.h>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/Error.h>

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
};


struct CompileInfo {
  KernelInfo KernelInfo;
  FINALIZER_INFO JitInfo;
  std::string GenBinary;
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

enum class RuntimeKind { CM, OpenCL };

struct CompileOptions {
  FileType FType = FileType::SPIRV;
  std::string CPUStr;
  std::unique_ptr<WA_TABLE> WATable = nullptr;

  // Api accessible options.
  bool NoVecDecomp = false;
  OptimizerLevel OptLevel = OptimizerLevel::Full;

  // Internal options.
  std::string FeaturesString; // format is: [+-]<feature1>,[+-]<feature2>,...
  RuntimeKind Runtime = RuntimeKind::OpenCL;
  bool DumpIsa = false;
  bool DumpIR = false;
};

llvm::Expected<CompileOutput> Compile(llvm::ArrayRef<char> Input,
                                      const CompileOptions &Opts);

llvm::Expected<CompileOptions> ParseOptions(llvm::StringRef ApiOptions,
                                            llvm::StringRef InternalOptions);
} // namespace vc

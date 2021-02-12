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

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"
#include "vc/Support/ShaderDump.h"

#include "Probe/Assertion.h"

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
using CompileOutput = llvm::GenXOCLRuntimeInfo::CompiledModuleT;
} // namespace ocl

namespace cm {
struct CompileOutput {
  std::string IsaBinary;
};
} // namespace cm

using CompileOutput = std::variant<cm::CompileOutput, ocl::CompileOutput>;

enum class FileType { SPIRV, LLVM_TEXT, LLVM_BINARY };

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
  bool EmitDebugInfo = false;

  bool NoJumpTables = false;
  OptimizerLevel OptLevel = OptimizerLevel::Full;
  llvm::Optional<unsigned> StackMemSize;

  // Internal options.
  std::string FeaturesString; // format is: [+-]<feature1>,[+-]<feature2>,...
  BinaryKind Binary = BinaryKind::OpenCL;
  bool DumpIsa = false;
  bool DumpIR = false;
  bool DumpAsm = false;
  bool DumpDebugInfo = false;
  bool TimePasses = false;
};

class ExternalData {
  std::unique_ptr<llvm::MemoryBuffer> OCLGenericBIFModule;
  std::unique_ptr<llvm::MemoryBuffer> OCLFP64BIFModule;

public:
  explicit ExternalData(std::unique_ptr<llvm::MemoryBuffer> GenericModule,
                        std::unique_ptr<llvm::MemoryBuffer> FP64Module)
      : OCLGenericBIFModule{std::move(GenericModule)},
        OCLFP64BIFModule{std::move(FP64Module)} {
    IGC_ASSERT_MESSAGE(OCLGenericBIFModule && OCLFP64BIFModule,
                       "wrong argument: no memory buffer was provided");
  }

  const llvm::MemoryBuffer &getOCLGenericBIFModule() const {
    return *OCLGenericBIFModule;
  }

  const llvm::MemoryBuffer &getOCLFP64BIFModule() const {
    return *OCLFP64BIFModule;
  }
};

llvm::Expected<CompileOutput> Compile(llvm::ArrayRef<char> Input,
                                      const CompileOptions &Opts,
                                      const ExternalData &ExtData,
                                      llvm::ArrayRef<uint32_t> SpecConstIds,
                                      llvm::ArrayRef<uint64_t> SpecConstValues);

llvm::Expected<CompileOptions> ParseOptions(llvm::StringRef ApiOptions,
                                            llvm::StringRef InternalOptions,
                                            bool IsStrictMode);
} // namespace vc

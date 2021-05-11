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

#pragma once

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/ShaderDump.h"
#include "vc/Support/ShaderOverride.h"

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

enum class GlobalsLocalizationMode { All, No, Vector, Partial };

struct CompileOptions {
  FileType FType = FileType::SPIRV;
  std::string CPUStr;
  int RevId;
  // Non-owning pointer to WA table.
  const WA_TABLE *WATable = nullptr;
  // Optional shader dumper.
  std::unique_ptr<ShaderDumper> Dumper = nullptr;
  // Optional Shader Overrider
  std::unique_ptr<vc::ShaderOverrider> ShaderOverrider = nullptr;

  // Api accessible options.
  // -ze-no-vector-decomposition
  bool NoVecDecomp = false;
  // -g
  bool EmitDebugInformation = false;
  bool EmitDebuggableKernels = false;
  // -fno-jump-tables
  bool NoJumpTables = false;
  // -ftranslate-legacy-memory-intrinsics
  bool TranslateLegacyMemoryIntrinsics = false;

  OptimizerLevel OptLevel = OptimizerLevel::Full;
  llvm::Optional<unsigned> StackMemSize;
  bool ForceLiveRangesLocalizationForAccUsage = false;
  bool ForceDisableNonOverlappingRegionOpt = false;

  // Internal options.
  std::string FeaturesString; // format is: [+-]<feature1>,[+-]<feature2>,...
  BinaryKind Binary = BinaryKind::OpenCL;
  bool DumpIsa = false;
  bool DumpIR = false;
  bool DumpAsm = false;
  bool DumpDebugInfo = false;
  bool TimePasses = false;
  GlobalsLocalizationMode GlobalsLocalization = GlobalsLocalizationMode::Vector;
  std::string LLVMOptions;

  // from IGC_XXX env
  FunctionControl FCtrl = FunctionControl::Default;
};

struct ExternalData {
  std::unique_ptr<llvm::MemoryBuffer> OCLGenericBIFModule;
  std::unique_ptr<llvm::MemoryBuffer> VCPrintf32BIFModule;
  std::unique_ptr<llvm::MemoryBuffer> VCPrintf64BIFModule;
  std::unique_ptr<llvm::MemoryBuffer> VCEmulationBIFModule;
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

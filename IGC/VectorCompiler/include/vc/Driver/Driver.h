/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvmWrapper/ADT/Optional.h"
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
#include <llvm/Target/TargetOptions.h>

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace vc {

using CompileOutput = llvm::GenXOCLRuntimeInfo::CompiledModuleT;

enum class FileType { SPIRV, LLVM_TEXT, LLVM_BINARY };

enum class OptimizerLevel { None, Full };

enum class GlobalsLocalizationMode { All, No, Vector, Partial };

enum class DisableLRCoalescingControl { Default, Disable, Enable };

enum class DisableExtraCoalescingControl { Default, Disable, Enable };

enum class NoOptFinalizerControl { Default, Disable, Enable };

enum class DebugInfoStripControl { None, All, NonLine };

struct CompileOptions {
  FileType FType = FileType::SPIRV;
  std::string CPUStr;
  int RevId = -1;
  // Non-owning pointer to WA table.
  const WA_TABLE *WATable = nullptr;
  // Optional shader dumper.
  std::unique_ptr<ShaderDumper> Dumper = nullptr;
  // Optional Shader Overrider
  std::unique_ptr<vc::ShaderOverrider> ShaderOverrider = nullptr;

  std::string ApiOptions;

  // Output binary format
  // Internal options:
  //   -[cl,ze]-disable-zebin
  //   -binary-format=[ocl,ze,cm]
  // Debug keys:
  //   IGC_EnableZEBinary=[0,1]
  BinaryKind Binary = BinaryKind::Default;

  // Api accessible options.
  // -ze-no-vector-decomposition
  bool NoVecDecomp = false;
  // -g
  bool ExtendedDebuggingSupport = false;
  // emit kernels that can interact with debugger, can be disabled by
  // -vc-disable-debuggable-kernels internal option
  bool EmitDebuggableKernels = true;
  // -fno-jump-tables
  bool NoJumpTables = false;
  // -ftranslate-legacy-memory-intrinsics
  bool TranslateLegacyMemoryIntrinsics = false;
  // -disable-finalizer-msg
  bool DisableFinalizerMsg = false;
  // -fno-struct-splitting
  bool DisableStructSplitting = false;
  // IGC_DisableEuFusion
  bool DisableEUFusion = false;
  // IGC_VCSaveStackCallLinkage, -vc-save-stack-call-linkage
  bool SaveStackCallLinkage = false;

  OptimizerLevel IROptLevel = OptimizerLevel::Full;
  OptimizerLevel CodegenOptLevel = OptimizerLevel::Full;

  IGCLLVM::optional<unsigned> StackMemSize;
  IGCLLVM::optional<unsigned> GRFSize;

  bool EnableAutoLargeGRF = false;

  bool ForceLiveRangesLocalizationForAccUsage = false;
  bool ForceDisableNonOverlappingRegionOpt = false;
  bool ForceDisableIndvarsOpt = false;

  DisableLRCoalescingControl DisableLRCoalescingMode =
      DisableLRCoalescingControl::Default;
  DisableExtraCoalescingControl DisableExtraCoalescingMode =
      DisableExtraCoalescingControl::Default;

  NoOptFinalizerControl NoOptFinalizerMode = NoOptFinalizerControl::Default;
  bool ForceDebugInfoValidation = false;

  bool EnablePreemption = false;

  llvm::FPOpFusion::FPOpFusionMode AllowFPOpFusion = llvm::FPOpFusion::Standard;

  bool UsePlain2DImages = false;

  // Internal options.
  std::string FeaturesString; // format is: [+-]<feature1>,[+-]<feature2>,...
  bool DumpIsa = false;
  bool DumpIR = false;
  bool DumpAsm = false;
  bool DumpDebugInfo = false;
  bool DumpInstOffset = false;

  bool TimePasses = false;
  bool ShowStats = false;
  bool ResetTimePasses = false;
  bool ResetLLVMStats = false;

  std::string StatsFile;
  std::string LLVMOptions;
  bool UseBindlessBuffers = false;
  bool UseBindlessImages = false;
  bool EmitZebinVisaSections = false;
  bool HasL1ReadOnlyCache = false;
  bool HasLocalMemFenceSupress = false;
  bool HasMultiTile = false;
  bool HasL3CacheCoherentCrossTiles = false;
  bool HasL3FlushOnGPUScopeInvalidate = false;
  bool HasL3FlushForGlobal = false;
  bool HasGPUFenceScopeOnSingleTileGPUs = false;
  bool HasHalfSIMDLSC = false;
  bool EmitVisaOnly = false;

  // from IGC_XXX env
  FunctionControl FCtrl = FunctionControl::Default;
  bool DirectCallsOnly = false;
  DebugInfoStripControl StripDebugInfoCtrl = DebugInfoStripControl::None;
  unsigned ForceLoopUnrollThreshold = 0;
  bool IgnoreLoopUnrollThresholdOnPragma = false;
  unsigned InteropSubgroupSize = 16;

  bool CheckGVClobbering =
#ifdef NDEBUG
      false
#else
      true
#endif
      ;

  bool EnableHashMovs = false;
  bool EnableHashMovsAtPrologue = false;
  uint64_t AsmHash = 0;

  bool EnableOpaquePointers = false;
  bool CollectCostInfo = false;

  unsigned DepressurizerGRFThreshold = 2560;
  unsigned DepressurizerFlagGRFTolerance = 3840;

  bool ReportLSCStoresWithNonDefaultL1CacheControls = false;
};

struct ExternalData {
  std::unique_ptr<llvm::MemoryBuffer> VCPrintf32BIFModule;
  std::unique_ptr<llvm::MemoryBuffer> VCPrintf64BIFModule;
  std::unique_ptr<llvm::MemoryBuffer> VCBuiltinsBIFModule;
  std::unique_ptr<llvm::MemoryBuffer> VCSPIRVBuiltinsBIFModule;
  llvm::ArrayRef<const char*> VISALTOStrings;
  llvm::ArrayRef<const char*> DirectCallFunctions;
};

llvm::Expected<CompileOutput>
Compile(llvm::ArrayRef<char> Input, const CompileOptions &Opts,
        const ExternalData &ExtData, llvm::ArrayRef<uint32_t> SpecConstIds,
        llvm::ArrayRef<uint64_t> SpecConstValues, llvm::raw_ostream &Log);

llvm::Expected<CompileOptions> ParseOptions(llvm::StringRef ApiOptions,
                                            llvm::StringRef InternalOptions,
                                            bool IsStrictMode);
} // namespace vc

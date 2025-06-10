/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Definition of backend configuration options and immutable wrapper pass.
//
// This pass should be used to query all options that can affect backend
// behavior. Pass will always be available at least with default options.
// Default values are set using LLVM command line options that can be
// overridden, for example, in plugin mode.
//
// Online mode wrapper will provide its custom values for all options that
// should not be defaulted.
//
// Proposed usage in passes: just use "getAnalysis<GenXBackendConfig>()" and
// query all needed information.
//
//===----------------------------------------------------------------------===//

#ifndef VC_SUPPORT_BACKEND_CONFIG_H
#define VC_SUPPORT_BACKEND_CONFIG_H

#include "vc/Support/ShaderDump.h"
#include "vc/Support/ShaderOverride.h"

#include "Probe/Assertion.h"

#include "inc/common/sku_wa.h"

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/MemoryBuffer.h>

#include <limits>
#include <memory>

enum class FunctionControl { Default, StackCall };

namespace vc {
enum class BinaryKind { Default, CM, OpenCL, ZE };
} // namespace vc

namespace llvm {

void initializeGenXBackendConfigPass(PassRegistry &PR);

// Plain structure to be filled by users who want to create backend
// configuration. Some values are default-initialized from cl options.
struct GenXBackendOptions {
  // EmitDebuggable Kernels (allocate SIP Surface)
  bool DebuggabilityEmitDebuggableKernels = true;
  // Legacy path requires kernel to reserve BTI=0
  bool DebuggabilityForLegacyPath = false;
  // Emit breakpoints at the start of each kernel
  bool DebuggabilityEmitBreakpoints = false;
  // Enable emission of DWARF debug information
  bool DebuggabilityEmitDWARF = true;
  // Generate Debug Info in a format compatible with zebin
  bool DebuggabilityZeBinCompatibleDWARF = false;
  // Enable strict debug info validation
  bool DebuggabilityValidateDWARF = false;

  // Special mode of operation for BiF precompilation. It is expected that this
  // mode is set only by specialized tools and should not be touched during
  // standard compilation flow.
  bool BiFCompilation = false;

  // Enable/disable regalloc dump.
  bool DumpRegAlloc = false;
  // Maximum available memory for stack (in bytes).
  unsigned StackSurfaceMaxSize = 8 * 1024;

  // Non-owning pointer to abstract shader dumper for debug dumps.
  vc::ShaderDumper *Dumper = nullptr;
  // Non-owning pointer to ShaderOverride interface
  vc::ShaderOverrider *ShaderOverrider = nullptr;

  // Flag to turn off StructSpliter pass
  bool DisableStructSplitting = false;

  // Flag to disable EU fusion
  bool DisableEUFusion = false;

  // Whether to enable finalizer dumps.
  bool EnableAsmDumps = false;
  bool EnableIsaDumps = false;
  // Whether to enable dumps of kernel debug information
  bool EnableDebugInfoDumps = false;
  std::string DebugInfoDumpsNameOverride;
  // Add instruction offsets as comments in the final assembly
  bool EnableInstOffsetDumps = false;

  bool ForceArrayPromotion = false;

  // Localize live ranges to reduce accumulator usage
  bool LocalizeLRsForAccUsage = false;

  // Disable LR coalescing
  bool DisableLiveRangesCoalescing = false;

  // Disable extra coalescing
  bool DisableExtraCoalescing = false;

  // Disable non-overlapping region transformation (the case with undef
  // value in two-address operand)
  bool DisableNonOverlappingRegionOpt = false;

  // Disable induction variable simplification
  bool DisableIndvarsOpt = false;

  FunctionControl FCtrl = FunctionControl::Default;

  // Non-owning pointer to workaround table.
  const WA_TABLE *WATable = nullptr;

  // Number of general-purpose registers available for a kernel thread, 0 is to
  // use default value.
  unsigned GRFSize = 0;

  // Use compiler heuristics to determine number of GRF.
  bool AutoLargeGRF = false;

  // Use bindless mode for buffers.
  bool UseBindlessBuffers = false;

  // Use bindless mode for images.
  bool UseBindlessImages = false;

  // Output binary format
  vc::BinaryKind Binary = vc::BinaryKind::OpenCL;

  // Add vISA asm as sections in ZeBin
  bool EmitZebinVisaSections = false;

  // max private stateless memory size per thread
  unsigned StatelessPrivateMemSize = 8192;

  // Disable critical messages from CisaBuilder
  bool DisableFinalizerMsg = false;

  // Historically stack calls linkage is changed to internal in CMABI. This
  // option allows saving the original linkage type for such functions. This is
  // required for linking (e.g. invoke_simd).
  bool SaveStackCallLinkage = false;

  // Treat "image2d_t" as non-media 2d images.
  bool UsePlain2DImages = false;

  // Enable/disable flushing L3 cache for globals.
  bool L3FlushForGlobal = false;

  // Allow the use of `GPU` fence scope on single-tile GPUs.
  bool GPUFenceScopeOnSingleTileGPUs = false;

  // Enable preemption (to be switched on by default)
  bool EnablePreemption = false;

  // Temporary solution. When is set, code is generated under the assumption all
  // calls are direct. Extern call are still extern in LLVM IR.
  bool DirectCallsOnly = false;

  // Loop unroll threshold. Value 0 means to keep default threshold.
  unsigned LoopUnrollThreshold = 0;

  // Ignore unrolling threshold on loops with #pragma unroll.
  bool IgnoreLoopUnrollThresholdOnPragma = false;

  // Subgroup size used for cross-module calls/returns
  unsigned InteropSubgroupSize = 16;

  // Run auxiliary checker/fixup for GV access clobbering cases.
  bool CheckGVClobbering =
#ifdef NDEBUG
      false
#else
      true
#endif
      ;

  // Compile until vISA stage only.
  bool EmitVisaOnly = false;

  bool EnableHashMovs = false;
  bool EnableHashMovsAtPrologue = false;
  uint64_t AsmHash = 0;

  bool EnableCostModel = false;

  unsigned DepressurizerGRFThreshold = 2560;
  unsigned DepressurizerFlagGRFTolerance = 3840;

  // Report LSC stores with non default L1 cache controls.
  bool ReportLSCStoresWithNonDefaultL1CacheControls = false;

  // Calling enforceLLVMOptions queries the state of LLVM options and
  // updates BackendOptions accordingly.
  // Note: current implementation allows backend options to be configured by
  // both driver and LLVM command line. LLVM options take presedence over
  // driver-derived values.
  void enforceLLVMOptions();

  struct InitFromLLVMOpts {};

  GenXBackendOptions() = default;
  GenXBackendOptions(InitFromLLVMOpts) { enforceLLVMOptions(); }
};

enum BiFKind { VCPrintf, VCBuiltins, VCSPIRVBuiltins, Size };

class GenXBackendData {
  // The owner of OpenCL generic BiF module.
  // For now it is only required for llvm-lit/debugging,
  // in libigc mode this field always holds nullptr.
  std::array<std::unique_ptr<MemoryBuffer>, BiFKind::Size>  BiFModuleOwner;

public:
  std::array<MemoryBufferRef, BiFKind::Size> BiFModule;
  llvm::ArrayRef<const char*> VISALTOStrings;
  llvm::StringSet<> DirectCallFunctions;

  struct InitFromLLMVOpts {};

  GenXBackendData() {}
  GenXBackendData(InitFromLLMVOpts);

private:
  // ModuleBuffer cannot be nullptr.
  void setOwningBiFModule(BiFKind Kind,
                          std::unique_ptr<MemoryBuffer> ModuleBuffer);
  // Weak contract variant. Does nothing for nullptr.
  void setOwningBiFModuleIf(BiFKind Kind,
                            std::unique_ptr<MemoryBuffer> ModuleBuffer);
};

struct GenXBackendConfigResult {
protected:
  GenXBackendOptions Options;
  GenXBackendData Data;

  GenXBackendConfigResult(GenXBackendOptions &&OptionsIn,
                          GenXBackendData &&DataIn)
      : Options(std::move(OptionsIn)), Data(std::move(DataIn)){};

  GenXBackendConfigResult()
      : Options{GenXBackendOptions::InitFromLLVMOpts{}},
        Data{GenXBackendData::InitFromLLMVOpts{}} {};

public:
  // Return whether regalloc results should be printed.
  bool enableRegAllocDump() const { return Options.DumpRegAlloc; }

  // Return whether BiF compilation mode is enabled.
  bool isBiFCompilation() const { return Options.BiFCompilation; }

  // Return maximum available space in bytes for stack purposes.
  unsigned getStackSurfaceMaxSize() const {
    return Options.StackSurfaceMaxSize;
  }

  MemoryBufferRef getBiFModule(BiFKind Kind) const {
    return Data.BiFModule[Kind];
  }

  llvm::ArrayRef<const char*> getVISALTOStrings() const {
    return Data.VISALTOStrings;
  }

  llvm::StringSet<> getDirectCallFunctionsSet() const {
    return Data.DirectCallFunctions;
  }

  bool emitBreakpointAtKernelEntry() const {
    return Options.DebuggabilityEmitBreakpoints;
  }
  bool emitDebuggableKernels() const {
    return Options.DebuggabilityEmitDebuggableKernels;
  }
  bool emitDebuggableKernelsForLegacyPath() const {
    return Options.DebuggabilityForLegacyPath && emitDebuggableKernels();
  }
  bool emitDWARFDebugInfo() const { return Options.DebuggabilityEmitDWARF; }
  bool emitDWARFDebugInfoForZeBin() const {
    return Options.DebuggabilityZeBinCompatibleDWARF && emitDWARFDebugInfo();
  }
  bool enableDebugInfoValidation() const {
    return Options.DebuggabilityValidateDWARF;
  }

  // Return whether shader dumper is installed.
  bool hasShaderDumper() const { return Options.Dumper; }

  // Get reference to currently installed dumper.
  // Precondition: hasShaderDumper() == true.
  vc::ShaderDumper &getShaderDumper() const {
    IGC_ASSERT_MESSAGE(hasShaderDumper(),
                       "Attempt to query not installed dumper");
    return *Options.Dumper;
  }
  // Return whether shader overrider is installed.
  bool hasShaderOverrider() const { return Options.ShaderOverrider; }
  // Get reference to currently installed overrider.
  // Precondition: hasShaderOverrider() == true.
  vc::ShaderOverrider &getShaderOverrider() const {
    IGC_ASSERT_MESSAGE(hasShaderOverrider(),
                       "Attempt to query not installed overrider");
    return *Options.ShaderOverrider;
  }

  bool asmDumpsEnabled() const { return Options.EnableAsmDumps; }
  bool isaDumpsEnabled() const { return Options.EnableIsaDumps; }
  bool dbgInfoDumpsEnabled() const { return Options.EnableDebugInfoDumps; }
  const std::string &dbgInfoDumpsNameOverride() const {
    return Options.DebugInfoDumpsNameOverride;
  }

  bool emitInstOffsets() const { return Options.EnableInstOffsetDumps; }

  bool isArrayPromotionForced() const { return Options.ForceArrayPromotion; }

  bool localizeLiveRangesForAccUsage() const {
    return Options.LocalizeLRsForAccUsage;
  }

  bool disableLiveRangesCoalescing() const {
    return Options.DisableLiveRangesCoalescing;
  }

  bool disableExtraCoalescing() const {
    return Options.DisableExtraCoalescing;
  }

  bool disableNonOverlappingRegionOpt() const {
    return Options.DisableNonOverlappingRegionOpt;
  }

  bool disableIndvarsOpt() const { return Options.DisableIndvarsOpt; }

  unsigned getStatelessPrivateMemSize() const {
    return Options.StatelessPrivateMemSize;
  }

  bool isDisableFinalizerMsg() const {
    return Options.DisableFinalizerMsg;
  }

  bool isDisableEUFusion() const { return Options.DisableEUFusion; }

  FunctionControl getFCtrl() const { return Options.FCtrl; }

  unsigned getGRFSize() const { return Options.GRFSize; }

  bool isAutoLargeGRFMode() const { return Options.AutoLargeGRF; }

  // Return pointer to WA_TABLE. Can be null.
  const WA_TABLE *getWATable() const {
    return Options.WATable;
  }

  bool doStructSplitting() const { return !Options.DisableStructSplitting; }

  bool useBindlessBuffers() const { return Options.UseBindlessBuffers; }

  bool useBindlessImages() const { return Options.UseBindlessImages; }

  bool emitZebinVisaSections() const { return Options.EmitZebinVisaSections; }

  bool saveStackCallLinkage() const { return Options.SaveStackCallLinkage; }

  bool usePlain2DImages() const { return Options.UsePlain2DImages; }

  bool enablePreemption() const { return Options.EnablePreemption; }

  bool directCallsOnly(llvm::StringRef FunctionName = "") const {
      return Options.DirectCallsOnly || Data.DirectCallFunctions.count(FunctionName);
  }

  bool emitVisaOnly() const { return Options.EmitVisaOnly; }

  unsigned getLoopUnrollThreshold() const {
    return Options.LoopUnrollThreshold;
  }

  bool ignoreLoopUnrollThresholdOnPragma() const {
    return Options.IgnoreLoopUnrollThresholdOnPragma;
  }

  unsigned getInteropSubgroupSize() const {
    return Options.InteropSubgroupSize;
  }

  bool checkGVClobbering() const { return Options.CheckGVClobbering; }

  bool isHashMovsEnabled() const { return Options.EnableHashMovs; }
  bool isHashMovsAtPrologueEnabled() const {
    return Options.EnableHashMovsAtPrologue;
  }

  uint64_t getAsmHash() const { return Options.AsmHash; }

  bool isCostModelEnabled() const { return Options.EnableCostModel; }

  vc::BinaryKind getBinaryFormat() const { return Options.Binary; }

  unsigned getDepressurizerGRFThreshold() const {
    return Options.DepressurizerGRFThreshold;
  }
  unsigned getDepressurizerFlagGRFTolerance() const {
    return Options.DepressurizerFlagGRFTolerance;
  }

  bool reportLSCStoresWithNonDefaultL1CacheControls() const {
    return Options.ReportLSCStoresWithNonDefaultL1CacheControls;
  }
};

class GenXBackendConfig : public ImmutablePass, public GenXBackendConfigResult {
public:
  static char ID;

public:
  GenXBackendConfig();
  explicit GenXBackendConfig(GenXBackendOptions &&OptionsIn,
                             GenXBackendData &&DataIn);

  GenXBackendConfigResult &getResult() { return *this; };
};
} // namespace llvm

#if LLVM_VERSION_MAJOR >= 16

#include "llvm/IR/PassManager.h"

struct GenXBackendConfigPass
    : public llvm::AnalysisInfoMixin<GenXBackendConfigPass> {
  using Result = llvm::GenXBackendConfigResult;
  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::AnalysisKey Key;
};
#endif

#endif

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

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

#include "llvmWrapper/Support/MemoryBuffer.h"

#include "Probe/Assertion.h"

#include "inc/common/sku_wa.h"

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

#include <limits>
#include <memory>

enum class FunctionControl { Default, StackCall };

namespace llvm {

void initializeGenXBackendConfigPass(PassRegistry &PR);

// Plain structure to be filled by users who want to create backend
// configuration. Some values are default-initialized from cl options.
struct GenXBackendOptions {
  // EmitDebuggable Kernels (allocate SIP Surface and avoid using BTI=0)
  bool EmitDebuggableKernels = false;
  // Enable emission of DWARF debug information
  bool EmitDebugInformation = false;
  // Generate Debug Info in a format compatible with zebin
  bool DebugInfoForZeBin = false;
  // Enable strict debug info validation
  bool DebugInfoValidationEnable = false;

  // Enable/disable regalloc dump.
  bool DumpRegAlloc;
  // Maximum available memory for stack (in bytes).
  unsigned StackSurfaceMaxSize;

  // Non-owning pointer to abstract shader dumper for debug dumps.
  vc::ShaderDumper *Dumper = nullptr;
  // Non-owning pointer to ShaderOverride interface
  vc::ShaderOverrider *ShaderOverrider = nullptr;

  // Flag to turn off StructSpliter pass
  bool DisableStructSplitting = false;

  // Whether to enable finalizer dumps.
  bool EnableAsmDumps;
  // Whether to enable dumps of kernel debug information
  bool EnableDebugInfoDumps;
  std::string DebugInfoDumpsNameOverride;

  bool ForceArrayPromotion = false;

  // Localize live ranges to reduce accumulator usage
  bool LocalizeLRsForAccUsage;

  // Disable non-overlapping region transformation (the case with undef
  // value in two-address operand)
  bool DisableNonOverlappingRegionOpt;

  // Force passing "-debug" option to finalizer
  bool PassDebugToFinalizer = false;

  // Disables coalescing of live ranges
  bool DisableLiveRangesCoalescing = false;

  // use new Prolog/Epilog Insertion pass vs old CisaBuilder machinery
  bool UseNewStackBuilder = true;

  FunctionControl FCtrl;

  // Non-owning pointer to workaround table.
  const WA_TABLE *WATable = nullptr;

  bool IsLargeGRFMode = false;

  // Use bindless mode for buffers.
  bool UseBindlessBuffers;

  // max private stateless memory size per thread
  unsigned StatelessPrivateMemSize;

  // Disable critical messages from CisaBuilder
  bool DisableFinalizerMsg = false;

  // Historically stack calls linkage is changed to internal in CMABI. This
  // option allows saving the original linkage type for such functions. This is
  // required for linking (e.g. invoke_simd).
  bool SaveStackCallLinkage = false;

  // Treat "image2d_t" as non-media 2d images.
  bool UsePlain2DImages = false;


  // Enable preemption (to be switched on by default)
  bool EnablePreemption = false;

  GenXBackendOptions();
};

enum BiFKind {
  OCLGeneric,
  VCPrintf,
  VCEmulation,
  VCSPIRVBuiltins,
  Size
};

class GenXBackendData {
  // The owner of OpenCL generic BiF module.
  // For now it is only required for llvm-lit/debugging,
  // in libigc mode this field always holds nullptr.
  std::array<std::unique_ptr<MemoryBuffer>, BiFKind::Size>  BiFModuleOwner;

public:
  std::array<MemoryBufferRef, BiFKind::Size> BiFModule;

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

class GenXBackendConfig : public ImmutablePass {
public:
  static char ID;

private:
  GenXBackendOptions Options;
  GenXBackendData Data;

public:
  GenXBackendConfig();
  explicit GenXBackendConfig(GenXBackendOptions OptionsIn,
                             GenXBackendData DataIn);

  // Return whether regalloc results should be printed.
  bool enableRegAllocDump() const { return Options.DumpRegAlloc; }

  // Return maximum available space in bytes for stack purposes.
  unsigned getStackSurfaceMaxSize() const {
    return Options.StackSurfaceMaxSize;
  }

  MemoryBufferRef getBiFModule(BiFKind Kind) const {
    return Data.BiFModule[Kind];
  }

  bool emitDebugInformation() const { return Options.EmitDebugInformation; }
  bool emitDebuggableKernels() const { return Options.EmitDebuggableKernels; }
  bool emitDebugInfoForZeBin() const { return Options.DebugInfoForZeBin; }
  bool enableDebugInfoValidation() const { return Options.DebugInfoValidationEnable; }
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
  bool dbgInfoDumpsEnabled() const { return Options.EnableDebugInfoDumps; }
  const std::string &dbgInfoDumpsNameOverride() const {
    return Options.DebugInfoDumpsNameOverride;
  }

  bool isArrayPromotionForced() const { return Options.ForceArrayPromotion; }

  bool localizeLiveRangesForAccUsage() const {
    return Options.LocalizeLRsForAccUsage;
  }

  bool disableNonOverlappingRegionOpt() const {
    return Options.DisableNonOverlappingRegionOpt;
  }

  bool disableLiveRangesCoalescing() const {
    return Options.DisableLiveRangesCoalescing;
  }

  bool passDebugToFinalizer() const {
    return Options.PassDebugToFinalizer;
  }

  bool useNewStackBuilder() const { return Options.UseNewStackBuilder; }

  unsigned getStatelessPrivateMemSize() const {
    return Options.StatelessPrivateMemSize;
  }

  bool isDisableFinalizerMsg() const {
    return Options.DisableFinalizerMsg;
  }

  FunctionControl getFCtrl() const { return Options.FCtrl; }

  bool isLargeGRFMode() const { return Options.IsLargeGRFMode; }

  // Return pointer to WA_TABLE. Can be null.
  const WA_TABLE *getWATable() const {
    return Options.WATable;
  }

  bool doStructSplitting() const { return !Options.DisableStructSplitting; }

  bool useBindlessBuffers() const { return Options.UseBindlessBuffers; }

  bool saveStackCallLinkage() const { return Options.SaveStackCallLinkage; }

  bool usePlain2DImages() const { return Options.UsePlain2DImages; }

  bool enablePreemption() const { return Options.EnablePreemption; }
};
} // namespace llvm

#endif

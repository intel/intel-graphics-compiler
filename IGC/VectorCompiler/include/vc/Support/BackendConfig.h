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

struct GlobalsLocalizationConfig {
  using LimitT = std::size_t;
  static constexpr auto NoLimit = std::numeric_limits<LimitT>::max();

private:
  // Whether every global variable must be localized.
  bool IsForced = true;
  // How many GRF memory is allowed to be used for localization.
  LimitT Limit = NoLimit;

public:
  GlobalsLocalizationConfig(bool IsForcedIn, LimitT LimitIn)
      : IsForced{IsForcedIn}, Limit{LimitIn} {
    if (IsForced)
      IGC_ASSERT_MESSAGE(
          Limit == NoLimit,
          "there can be no localization limit when localization is forced");
  }

  GlobalsLocalizationConfig() {}

  // Every global variable must be localized.
  static GlobalsLocalizationConfig CreateForcedLocalization() { return {}; }

  // GlobalsLocalization is allowed to localize globals but it can use only
  // GlobalsLocalizationLimit bytes of GRF.
  static GlobalsLocalizationConfig
  CreateLocalizationWithLimit(LimitT GlobalsLocalizationLimitIn = NoLimit) {
    return {false, GlobalsLocalizationLimitIn};
  }

  bool isForced() const { return IsForced; }

  LimitT getLimit() const { return Limit; }
};

// Plain structure to be filled by users who want to create backend
// configuration. Some values are default-initialized from cl options.
struct GenXBackendOptions {
  // EmitDebuggable Kernels (allocate SIP Surface and avoid using BTI=0)
  bool EmitDebuggableKernels = false;
  // Enable emission of DWARF debug information
  bool EmitDebugInformation = false;
  // Enable/disable regalloc dump.
  bool DumpRegAlloc;
  // Maximum available memory for stack (in bytes).
  unsigned StackSurfaceMaxSize;

  // Non-owning pointer to abstract shader dumper for debug dumps.
  vc::ShaderDumper *Dumper = nullptr;
  // Non-owning pointer to ShaderOverride interface
  vc::ShaderOverrider *ShaderOverrider = nullptr;

  // Whether to enable finalizer dumps.
  bool EnableAsmDumps;
  // Whether to enable dumps of kernel debug information
  bool EnableDebugInfoDumps;
  std::string DebugInfoDumpsNameOverride;

  // Configuration for GlobalsLocalization pass
  // (part of CMABI pass by historical reasons).
  GlobalsLocalizationConfig GlobalsLocalization;
  bool ForceArrayPromotion = false;

  // Localize live ranges to reduce accumulator usage
  bool LocalizeLRsForAccUsage;

  // Disable non-overlapping region transformation (the case with undef
  // value in two-address operand)
  bool DisableNonOverlappingRegionOpt;

  // use new Prolog/Epilog Insertion pass vs old CisaBuilder machinery
  bool UseNewStackBuilder = true;

  FunctionControl FCtrl;

  // Non-owning pointer to workaround table.
  const WA_TABLE *WATable = nullptr;

  // max private stateless memory size per thread
  unsigned StatelessPrivateMemSize;

  GenXBackendOptions();
};

enum BiFKind {
  OCLGeneric,
  VCPrintf,
  VCEmulation,
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

  bool isGlobalsLocalizationForced() const {
    return Options.GlobalsLocalization.isForced();
  }
  GlobalsLocalizationConfig::LimitT getGlobalsLocalizationLimit() const {
    return Options.GlobalsLocalization.getLimit();
  }
  bool isArrayPromotionForced() const { return Options.ForceArrayPromotion; }

  bool localizeLiveRangesForAccUsage() const {
    return Options.LocalizeLRsForAccUsage;
  }

  bool disableNonOverlappingRegionOpt() const {
    return Options.DisableNonOverlappingRegionOpt;
  }

  bool useNewStackBuilder() const { return Options.UseNewStackBuilder; }

  unsigned getStatelessPrivateMemSize() const {
    return Options.StatelessPrivateMemSize;
  }

  FunctionControl getFCtrl() const { return Options.FCtrl; }

  // Return pointer to WA_TABLE. Can be null.
  const WA_TABLE *getWATable() const {
    return Options.WATable;
  }
};
} // namespace llvm

#endif

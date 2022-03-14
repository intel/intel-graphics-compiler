/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Support/BackendConfig.h"

#include <llvm/Support/CommandLine.h>

#include <string>

#define DEBUG_TYPE "GenXBackendConfig"

using namespace llvm;

//===----------------------------------------------------------------------===//
//
// All options that can control backend behavior should be here.
//
//===----------------------------------------------------------------------===//

static cl::opt<bool> GenerateDebugInfoOpt(
    "vc-emit-debug-info", cl::Hidden,
    cl::desc("Generate DWARF debug info for each compiled kernel"));

static cl::opt<bool> EmitDebuggableKernelsOpt(
    "vc-emit-debuggable-kernels", cl::Hidden,
    cl::desc("Emit kernels suitable for interaction with the debugger"));

static cl::opt<bool> DumpRegAllocOpt(
    "genx-dump-regalloc", cl::Hidden,
    cl::desc(
        "Enable dumping of GenX liveness and register allocation to a file."));

static cl::opt<unsigned> StackMemSizeOpt("stack-mem-size",
                                         cl::desc("Available space for stack"));

static cl::opt<bool>
    EnableAsmDumpsOpt("genx-enable-asm-dumps",
                      cl::desc("Enable finalizer assembly dumps"));
static cl::opt<bool>
    EnableDebugInfoDumpOpt("vc-enable-dbginfo-dumps",
                           cl::desc("Enable debug information-related dumps"));
static cl::opt<std::string> DebugInfoDumpsNameOverrideOpt(
    "vc-dbginfo-dumps-name-override",
    cl::desc("Override for 'suffix' part of debug info dump name"));

static cl::opt<std::string>
    OCLGenericBiFPath("vc-ocl-generic-bif-path",
                      cl::desc("full name (with path) of a BiF file with "
                               "precompiled OpenCL generic builtins"),
                      cl::init(""));

static cl::opt<std::string>
    VCEmulationBiFPath("vc-emulation-bif-path",
                       cl::desc("full name (with path) of a BiF file with "
                                "precompiled emulation routines"),
                       cl::init(""));

static cl::opt<std::string>
    VCSPIRVBuiltinsBiFPath("vc-spirv-builtins-bif-path",
                           cl::desc("full name (with path) of a BiF file with "
                                    "precompiled SPIR-V builtins"),
                           cl::init(""));

static cl::opt<std::string>
    VCPrintfBiFPath("vc-printf-bif-path",
                    cl::desc("full name (with path) of a BiF file with "
                             "precompiled printf implementation"),
                    cl::init(""));

static cl::opt<bool> LocalizeLRsForAccUsageOpt(
    "vc-acc-split", cl::Hidden,
    cl::desc("Localize arithmetic chain to reduce accumulator usages"));

static cl::opt<bool>
    DisableLRCoalescingOpt("vc-disable-coalescing", cl::Hidden,
                           cl::desc("disable coalescing of live ranges"));

static cl::opt<bool>
    DisableExtraCoalescingOpt("vc-disable-extra-coalescing", cl::Hidden,
                              cl::desc("disable extrac coalescing"));

static cl::opt<bool> DisableNonOverlappingRegionOptOpt(
    "vc-disable-non-overlapping-region-opt", cl::Hidden,
    cl::desc("Disable non-overlapping region optimization"));

static cl::opt<bool>
    UseNewStackBuilderOpt("vc-use-new-stack-builder",
                          cl::desc("Use prolog/epilog insertion pass"));

static cl::opt<unsigned>
    StatelessPrivateMemSizeOpt("dbgonly-enforce-privmem-stateless",
                               cl::desc("Enforce stateless privmem size"));

static cl::opt<FunctionControl> FunctionControlOpt(
    "vc-function-control", cl::desc("Force special calls (see supported enum)"),
    cl::values(clEnumValN(FunctionControl::Default, "default", "Default"),
               clEnumValN(FunctionControl::StackCall, "stackcall",
                          "Stackcall")));

static cl::opt<bool> LargeGRFModeOpt("vc-large-grf",
                                     cl::desc("Enable large GRF mode"));

static cl::opt<bool> UseBindlessBuffersOpt("vc-use-bindless-buffers",
                                           cl::desc("Use bindless buffers"));

static cl::opt<bool> EnablePreemptionOpt("vc-enable-preemption",
                                         cl::desc("Enable preemption"));

static cl::opt<bool> SaveStackCallLinkageOpt(
    "save-stack-call-linkage", cl::Hidden,
    cl::desc("Do not override stack calls linkage as internal"));

static cl::opt<bool> UsePlain2DImagesOpt(
    "vc-use-plain-2d-images", cl::Hidden,
    cl::desc("Treat \"image2d_t\" annotation as non-media image"));

static cl::opt<bool>
    L3FlushForGlobalOpt("vc-flush-l3-for-global", cl::Hidden,
                        cl::desc("Enable flushing L3 cache for globals"));

static cl::opt<bool> GPUFenceScopeOnSingleTileGPUsOpt(
    "vc-gpu-scope-fence-on-single-tile", cl::Hidden,
    cl::desc("Allow the use of \"GPU\" fence scope on single-tile GPUs."));

static cl::opt<bool> DirectCallsOnlyOpt(
    "direct-calls-only", cl::Hidden,
    cl::desc(
        "Generate code under the assumption all unknown calls are direct"));

static cl::opt<unsigned> VCLoopUnrollThreshold(
    "vc-loop-unroll-threshold", cl::Hidden,
    cl::desc("Threshold value for LLVM loop unroll pass"));

static cl::opt<bool> VCIgnoreLoopUnrollThresholdOnPragma(
    "vc-ignore-loop-unroll-threshold-on-pragma", cl::Hidden,
    cl::desc("Ignore threshold value for LLVM loop unroll pass when pragma is "
             "used"));

//===----------------------------------------------------------------------===//
//
// Backend config related stuff.
//
//===----------------------------------------------------------------------===//
char GenXBackendConfig::ID = 0;

template <typename DstT, typename OptT>
static void enforceOptionIfSpecified(DstT &Opt, OptT &OptValue) {
  if (OptValue.getNumOccurrences())
    Opt = OptValue;
}

void GenXBackendOptions::enforceLLVMOptions() {
  enforceOptionIfSpecified(DebuggabilityEmitDebuggableKernels,
                           EmitDebuggableKernelsOpt);
  enforceOptionIfSpecified(DebuggabilityEmitDWARF, GenerateDebugInfoOpt);
  enforceOptionIfSpecified(DumpRegAlloc, DumpRegAllocOpt);
  enforceOptionIfSpecified(StackSurfaceMaxSize, StackMemSizeOpt);
  enforceOptionIfSpecified(EnableAsmDumps, EnableAsmDumpsOpt);
  enforceOptionIfSpecified(EnableDebugInfoDumps, EnableDebugInfoDumpOpt);
  enforceOptionIfSpecified(DebugInfoDumpsNameOverride,
                           DebugInfoDumpsNameOverrideOpt);
  enforceOptionIfSpecified(UseNewStackBuilder, UseNewStackBuilderOpt);
  enforceOptionIfSpecified(LocalizeLRsForAccUsage, LocalizeLRsForAccUsageOpt);
  enforceOptionIfSpecified(DisableLiveRangesCoalescing, DisableLRCoalescingOpt);
  enforceOptionIfSpecified(DisableExtraCoalescing, DisableExtraCoalescingOpt);
  enforceOptionIfSpecified(DisableNonOverlappingRegionOpt,
                           DisableNonOverlappingRegionOptOpt);
  enforceOptionIfSpecified(FCtrl, FunctionControlOpt);
  enforceOptionIfSpecified(IsLargeGRFMode, LargeGRFModeOpt);
  enforceOptionIfSpecified(UseBindlessBuffers, UseBindlessBuffersOpt);
  enforceOptionIfSpecified(StatelessPrivateMemSize, StatelessPrivateMemSizeOpt);
  enforceOptionIfSpecified(SaveStackCallLinkage, SaveStackCallLinkageOpt);
  enforceOptionIfSpecified(UsePlain2DImages, UsePlain2DImagesOpt);
  enforceOptionIfSpecified(L3FlushForGlobal, L3FlushForGlobalOpt);
  enforceOptionIfSpecified(GPUFenceScopeOnSingleTileGPUs,
                           GPUFenceScopeOnSingleTileGPUsOpt);
  enforceOptionIfSpecified(EnablePreemption, EnablePreemptionOpt);
  enforceOptionIfSpecified(DirectCallsOnly, DirectCallsOnlyOpt);
  enforceOptionIfSpecified(LoopUnrollThreshold, VCLoopUnrollThreshold);
  enforceOptionIfSpecified(IgnoreLoopUnrollThresholdOnPragma,
                           VCIgnoreLoopUnrollThresholdOnPragma);
}

static std::unique_ptr<MemoryBuffer>
readBiFModuleFromFile(const cl::opt<std::string> &File) {
  if (File.getNumOccurrences() == 0)
    return nullptr;
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFileOrSTDIN(File);
  if (!FileOrErr)
    report_fatal_error("opening OpenCL BiF file failed: " +
                       FileOrErr.getError().message());
  return std::move(FileOrErr.get());
}

GenXBackendData::GenXBackendData(InitFromLLMVOpts) {
  setOwningBiFModuleIf(BiFKind::OCLGeneric,
                       readBiFModuleFromFile(OCLGenericBiFPath));
  setOwningBiFModuleIf(BiFKind::VCEmulation,
                       readBiFModuleFromFile(VCEmulationBiFPath));
  setOwningBiFModuleIf(BiFKind::VCSPIRVBuiltins,
                       readBiFModuleFromFile(VCSPIRVBuiltinsBiFPath));
  setOwningBiFModuleIf(BiFKind::VCPrintf, readBiFModuleFromFile(VCPrintfBiFPath));
}

void GenXBackendData::setOwningBiFModule(
    BiFKind Kind, std::unique_ptr<MemoryBuffer> ModuleBuffer) {
  IGC_ASSERT_MESSAGE(ModuleBuffer, "wrong argument");
  IGC_ASSERT(static_cast<size_t>(Kind) < BiFModuleOwner.size());
  BiFModuleOwner[Kind] = std::move(ModuleBuffer);
  BiFModule[Kind] = IGCLLVM::makeMemoryBufferRef(*BiFModuleOwner[Kind]);
}

void GenXBackendData::setOwningBiFModuleIf(
    BiFKind Kind, std::unique_ptr<MemoryBuffer> ModuleBuffer) {
  if (ModuleBuffer)
    setOwningBiFModule(Kind, std::move(ModuleBuffer));
}

GenXBackendConfig::GenXBackendConfig()
    : ImmutablePass(ID), Options{GenXBackendOptions::InitFromLLVMOpts{}},
      Data{GenXBackendData::InitFromLLMVOpts{}} {
  initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
}

GenXBackendConfig::GenXBackendConfig(GenXBackendOptions OptionsIn,
                                     GenXBackendData DataIn)
    : ImmutablePass(ID), Options(std::move(OptionsIn)),
      Data(std::move(DataIn)) {
  initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
}

INITIALIZE_PASS(GenXBackendConfig, DEBUG_TYPE, DEBUG_TYPE, false, true)

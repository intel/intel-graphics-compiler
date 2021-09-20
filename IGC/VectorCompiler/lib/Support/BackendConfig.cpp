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
    "vc-emit-debug-info", cl::init(false), cl::Hidden,
    cl::desc("Generate DWARF debug info for each compiled kernel"));

static cl::opt<bool> EmitDebuggableKernelsOpt(
    "vc-emit-debuggable-kernels", cl::init(false), cl::Hidden,
    cl::desc("Emit kernels suitable for interaction with the debugger"));

static cl::opt<bool> DumpRegAllocOpt(
    "genx-dump-regalloc", cl::init(false), cl::Hidden,
    cl::desc(
        "Enable dumping of GenX liveness and register allocation to a file."));

static cl::opt<unsigned> StackMemSizeOpt("stack-mem-size",
                                         cl::desc("Available space for stack"),
                                         cl::init(8 * 1024));

static cl::opt<bool>
    EnableAsmDumpsOpt("genx-enable-asm-dumps",
                      cl::desc("Enable finalizer assembly dumps"),
                      cl::init(false));
static cl::opt<bool>
    EnableDebugInfoDumpOpt("vc-enable-dbginfo-dumps",
                           cl::desc("Enable debug information-related dumps"),
                           cl::init(false));
static cl::opt<std::string> DebugInfoDumpNameOverride(
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
                                "precompiled divrem emulation routines"),
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
    "vc-acc-split", cl::init(false), cl::Hidden,
    cl::desc("Localize arithmetic chain to reduce accumulator usages"));

static cl::opt<bool> DisableNonOverlappingRegionOptOpt(
    "vc-disable-non-overlapping-region-opt", cl::init(false), cl::Hidden,
    cl::desc("Disable non-overlapping region optimization"));

static cl::opt<bool> PassDebugToFinalizerOpt(
    "vc-pass-debug-to-finalizer", cl::init(false), cl::Hidden,
    cl::desc("Pass -debug option to finalizer"));

static cl::opt<bool>
    UseNewStackBuilderOpt("vc-use-new-stack-builder",
                          cl::desc("Use prolog/epilog insertion pass"),
                          cl::init(true));

static cl::opt<unsigned>
    StatelessPrivateMemSizeOpt("dbgonly-enforce-privmem-stateless",
                               cl::desc("Enforce stateless privmem size"),
                               cl::init(8192));

static cl::opt<FunctionControl> FunctionControlOpt(
    "vc-function-control", cl::desc("Force special calls (see supported enum)"),
    cl::init(FunctionControl::Default),
    cl::values(clEnumValN(FunctionControl::Default, "default", "Default"),
               clEnumValN(FunctionControl::StackCall, "stackcall", "Default")));

static cl::opt<bool> LargeGRFModeOpt("vc-large-grf",
                                     cl::desc("Enable large GRF mode"),
                                     cl::init(false));

static cl::opt<bool> UseBindlessBuffersOpt("vc-use-bindless-buffers",
                                           cl::desc("Use bindless buffers"),
                                           cl::init(false));

static cl::opt<bool> SaveStackCallLinkageOpt(
    "save-stack-call-linkage", cl::init(false), cl::Hidden,
    cl::desc("Do not override stack calls linkage as internal"));

//===----------------------------------------------------------------------===//
//
// Backend config related stuff.
//
//===----------------------------------------------------------------------===//
char GenXBackendConfig::ID = 0;

GenXBackendOptions::GenXBackendOptions()
    : EmitDebugInformation(GenerateDebugInfoOpt),
      EmitDebuggableKernels(EmitDebuggableKernelsOpt),
      DumpRegAlloc(DumpRegAllocOpt), StackSurfaceMaxSize(StackMemSizeOpt),
      EnableAsmDumps(EnableAsmDumpsOpt),
      EnableDebugInfoDumps(EnableDebugInfoDumpOpt),
      DebugInfoDumpsNameOverride(DebugInfoDumpNameOverride),
      UseNewStackBuilder(UseNewStackBuilderOpt),
      LocalizeLRsForAccUsage(LocalizeLRsForAccUsageOpt),
      DisableNonOverlappingRegionOpt(DisableNonOverlappingRegionOptOpt),
      PassDebugToFinalizer(PassDebugToFinalizerOpt),
      FCtrl(FunctionControlOpt), IsLargeGRFMode(LargeGRFModeOpt),
      UseBindlessBuffers(UseBindlessBuffersOpt),
      StatelessPrivateMemSize(StatelessPrivateMemSizeOpt),
      SaveStackCallLinkage(SaveStackCallLinkageOpt) {}

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
    : ImmutablePass(ID), Data{GenXBackendData::InitFromLLMVOpts{}} {
  initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
}

GenXBackendConfig::GenXBackendConfig(GenXBackendOptions OptionsIn,
                                     GenXBackendData DataIn)
    : ImmutablePass(ID), Options(std::move(OptionsIn)),
      Data(std::move(DataIn)) {
  initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
}

INITIALIZE_PASS(GenXBackendConfig, DEBUG_TYPE, DEBUG_TYPE, false, true)

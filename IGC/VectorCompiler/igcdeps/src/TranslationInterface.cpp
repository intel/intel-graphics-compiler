/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/igcdeps/TranslationInterface.h"

#include "vc/igcdeps/ShaderDump.h"
#include "vc/igcdeps/ShaderOverride.h"
#include "vc/igcdeps/cmc.h"

#include "vc/BiF/Wrapper.h"
#include "vc/Driver/Driver.h"
#include "vc/Support/Status.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/ScopeExit.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/raw_ostream.h>

#include <llvmWrapper/ADT/Optional.h>

#include <AdaptorOCL/OCL/BuiltinResource.h>
#include <AdaptorOCL/OCL/LoadBuffer.h>
#include <AdaptorOCL/OCL/TB/igc_tb.h>
#include <common/igc_regkeys.hpp>
#include <iStdLib/utility.h>
#include <inc/common/secure_mem.h>

#include <algorithm>
#include <memory>
#include <system_error>

#include <cstring>

namespace {
struct VcPayloadInfo {
  bool IsValid = false;
  size_t IRSize = 0;
  llvm::StringRef VCApiOpts;
  llvm::StringRef VCInternalOpts;
};

class BuildDiag {
public:
  explicit BuildDiag(llvm::raw_ostream &Log) : Out(Log) {}
  void addWarning(llvm::StringRef Str) { Out << "warning: " << Str << "\n"; }

private:
  llvm::raw_ostream &Out;
};
} // namespace

static VcPayloadInfo tryExtractPayload(const char *Input, size_t InputSize) {
  // Payload format:
  // |-vc-payload|api opts|internal opts|i64(IR size)|i64(Payload size)|-vc-payload|
  // NOTE: <api/internal opts> are c-strings.
  //
  // Should be in sync with:
  //    Source/IGC/AdaptorOCL/ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.cpp

  // Check for availability of "-vc-payload" marker at the end.
  const char *PayloadMarker = "-vc-payload";
  const size_t PayloadMarkerSize = strlen(PayloadMarker);
  // Make sure that we also have a room for 2 i64 size items.
  if (InputSize < (PayloadMarkerSize + 2 * sizeof(uint64_t)))
    return {};
  const char *const InputEnd = Input + InputSize;
  if (std::memcmp(InputEnd - PayloadMarkerSize, PayloadMarker,
                  PayloadMarkerSize) != 0)
    return {};

  // Read IR and Payload sizes. We already ensured that we have the room.
  uint64_t IrSize;
  uint64_t PayloadSize;
  const char *IrSizeBuff =
      InputEnd - PayloadMarkerSize - 2 * sizeof(uint64_t);
  const char *PayloadSizeBuff =
      InputEnd - PayloadMarkerSize - 1 * sizeof(uint64_t);
  memcpy_s(&IrSize, sizeof(IrSize), IrSizeBuff, sizeof(IrSize));
  memcpy_s(&PayloadSize, sizeof(PayloadSize), PayloadSizeBuff,
           sizeof(PayloadSize));
  if (InputSize != (PayloadSize + IrSize))
    return {};

  // Search for the start of payload, it should start with "-vc-codegen" marker.
  const char *const IREnd = InputEnd - PayloadSize;
  if (std::memcmp(IREnd, PayloadMarker, PayloadMarkerSize) != 0)
    return {};

  // Make sure that we have a zero-terminated c-string (vc-options are encoded
  // as such).
  auto ApiOptsStart  = IREnd + PayloadMarkerSize;
  auto ApiOptsEnd = std::find(ApiOptsStart, InputEnd, 0);
  if (ApiOptsEnd == InputEnd)
    return {};
  auto IntOptStart = ApiOptsEnd + 1;
  auto IntOptEnd = std::find(IntOptStart, InputEnd, 0);
  // Consistency check, see the Payload format.
  if ((IntOptEnd + 1) != IrSizeBuff)
    return {};

  VcPayloadInfo Result;
  Result.VCApiOpts = llvm::StringRef(ApiOptsStart);
  Result.VCInternalOpts = llvm::StringRef(IntOptStart);
  Result.IRSize = IrSize;
  Result.IsValid = true;

  return Result;
}

template <enum vc::bif::RawKind Kind>
std::unique_ptr<llvm::MemoryBuffer>
getVCModuleBufferForArch(llvm::StringRef CPUStr = "") {
  return llvm::MemoryBuffer::getMemBuffer(
      vc::bif::getRawDataForArch<Kind>(CPUStr), "",
      false /* RequiresNullTerminator */);
}

template <enum vc::bif::RawKind Kind>
std::unique_ptr<llvm::MemoryBuffer> getVCModuleBuffer() {
  return llvm::MemoryBuffer::getMemBuffer(vc::bif::getRawData<Kind>(), "",
                                          false /* RequiresNullTerminator */);
}

static std::pair<std::string, unsigned>
getPlatformName(const PLATFORM &Platform) {
  constexpr unsigned ComputeTileMaskPVC = 7;
  auto Core = Platform.eRenderCoreFamily;
  auto Product = Platform.eProductFamily;
  unsigned DevId = Platform.usDeviceID;
  unsigned RevId = Platform.usRevId;

  switch (Core) {
  case IGFX_GEN8_CORE:
    return {"Gen8", RevId};
  case IGFX_GEN9_CORE:
    if (Product == IGFX_BROXTON || Product == IGFX_GEMINILAKE)
      return {"Gen9LP", RevId};
    return {"Gen9", RevId};
  case IGFX_GEN11_CORE:
  case IGFX_GEN11LP_CORE:
    return {"Gen11", RevId};
  case IGFX_GEN12_CORE:
  case IGFX_GEN12LP_CORE:
    return {"XeLP", RevId};
  case IGFX_XE_HP_CORE:
    return {"XeHP", RevId};
  case IGFX_XE_HPG_CORE:
    if (Product == IGFX_DG2)
      return {"XeHPG", RevId};
    if (Product == IGFX_METEORLAKE)
      return {"XeLPG", RevId};
    if (Product == IGFX_ARROWLAKE) {
      if (GFX_IS_ARL_S(DevId))
        return {"XeLPG", RevId};
      return {"XeLPGPlus", RevId};
    }
    break;
  case IGFX_XE_HPC_CORE:
    if (Product == IGFX_PVC) {
      if (GFX_IS_VG_CONFIG(DevId))
        return {"XeHPCVG", RevId & ComputeTileMaskPVC};
      return {"XeHPC", RevId & ComputeTileMaskPVC};
    }
    break;
  case IGFX_XE2_HPG_CORE:
    if (Product == IGFX_LUNARLAKE)
      return {"Xe2", RevId};
    if (Product == IGFX_BMG)
      return {"Xe2", RevId};
    break;
  case IGFX_XE3_CORE:
    if (Product == IGFX_PTL)
      return {"Xe3", RevId};
    LLVM_FALLTHROUGH;
  default:
    break;
  }
  IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported platform");
  return {"Invalid", -1};
}

static void adjustPlatform(const IGC::CPlatform &IGCPlatform,
                           vc::CompileOptions &Opts) {
  auto &PlatformInfo = IGCPlatform.getPlatformInfo();

  std::tie(Opts.CPUStr, Opts.RevId) = getPlatformName(PlatformInfo);

  Opts.HasL1ReadOnlyCache = IGCPlatform.hasL1ReadOnlyCache();
  Opts.HasLocalMemFenceSupress = IGCPlatform.localMemFenceSupress();
  Opts.HasMultiTile = IGCPlatform.hasMultiTile();
  Opts.HasL3CacheCoherentCrossTiles = IGCPlatform.L3CacheCoherentCrossTiles();
  Opts.HasL3FlushOnGPUScopeInvalidate =
      IGCPlatform.hasL3FlushOnGPUScopeInvalidate();
  Opts.HasHalfSIMDLSC = IGCPlatform.hasHalfSIMDLSC();
  Opts.WATable = &IGCPlatform.getWATable();
}

static void adjustFileType(TC::TB_DATA_FORMAT DataFormat,
                           vc::CompileOptions &Opts) {
  switch (DataFormat) {
  case TC::TB_DATA_FORMAT::TB_DATA_FORMAT_LLVM_TEXT:
    Opts.FType = vc::FileType::LLVM_TEXT;
    return;
  case TC::TB_DATA_FORMAT::TB_DATA_FORMAT_LLVM_BINARY:
      Opts.FType = vc::FileType::LLVM_BINARY;
      return;
  case TC::TB_DATA_FORMAT::TB_DATA_FORMAT_SPIR_V:
    Opts.FType = vc::FileType::SPIRV;
    return;
  default:
    llvm_unreachable("Data format is not supported yet");
  }
}

static void adjustOptLevel(vc::CompileOptions &Opts) {
  if (IGC_IS_FLAG_ENABLED(VCOptimizeNone))
    Opts.IROptLevel = vc::OptimizerLevel::None;
}

static void adjustStackCalls(vc::CompileOptions &Opts, BuildDiag &Diag) {
  int FCtrlFlag = IGC_GET_FLAG_VALUE(FunctionControl);
  switch (FCtrlFlag) {
  default:
    Opts.FCtrl = FunctionControl::Default;
    break;
  case FLAG_FCALL_FORCE_INLINE:
    Diag.addWarning("VC does not support always inline");
    break;
  case FLAG_FCALL_FORCE_SUBROUTINE:
    Diag.addWarning("VC does not support always subroutine");
    break;
  case FLAG_FCALL_FORCE_STACKCALL:
    Opts.FCtrl = FunctionControl::StackCall;
    break;
  case FLAG_FCALL_FORCE_INDIRECTCALL:
    Diag.addWarning("VC does not support always indirect calls");
    break;
  }
}

static void adjustDebugStrippingPolicy(vc::CompileOptions &Opts) {
  int DebugStripFlag = IGC_GET_FLAG_VALUE(StripDebugInfo);
  switch (DebugStripFlag) {
  default:
    Opts.StripDebugInfoCtrl = vc::DebugInfoStripControl::None;
    break;
  case FLAG_DEBUG_INFO_STRIP_ALL:
    Opts.StripDebugInfoCtrl = vc::DebugInfoStripControl::All;
    break;
  case FLAG_DEBUG_INFO_STRIP_NONLINE:
    Opts.StripDebugInfoCtrl = vc::DebugInfoStripControl::NonLine;
    break;
  }
}

template <typename T> T deriveDefaultableFlagValue(int Flag) {
  switch (Flag) {
  default:
    return T::Default;
  case DEFAULTABLE_FLAG_ENABLE:
    return T::Enable;
  case DEFAULTABLE_FLAG_DISABLE:
    return T::Disable;
  }
}

static void adjustTransformationsAndOptimizations(vc::CompileOptions &Opts) {
  if (IGC_IS_FLAG_ENABLED(VCLocalizeAccUsage))
    Opts.ForceLiveRangesLocalizationForAccUsage = true;
  if (IGC_IS_FLAG_ENABLED(VCDisableNonOverlappingRegionOpt))
    Opts.ForceDisableNonOverlappingRegionOpt = true;
  if (IGC_IS_FLAG_ENABLED(VCSaveStackCallLinkage))
    Opts.SaveStackCallLinkage = true;
  if (IGC_IS_FLAG_ENABLED(VCDirectCallsOnly))
    Opts.DirectCallsOnly = true;
  if (IGC_IS_FLAG_ENABLED(DisableEuFusion))
    Opts.DisableEUFusion = true;
  if (IGC_IS_FLAG_ENABLED(DebugInfoValidation))
    Opts.ForceDebugInfoValidation = true;
  if (IGC_IS_FLAG_ENABLED(EnableL3FlushForGlobal))
    Opts.HasL3FlushForGlobal = true;
  if (IGC_IS_FLAG_ENABLED(EnableGPUFenceScopeOnSingleTileGPUs))
    Opts.HasGPUFenceScopeOnSingleTileGPUs = true;

  if (unsigned LoopUnrollThreshold = IGC_GET_FLAG_VALUE(VCLoopUnrollThreshold))
    Opts.ForceLoopUnrollThreshold = LoopUnrollThreshold;
  if (IGC_IS_FLAG_ENABLED(VCIgnoreLoopUnrollThresholdOnPragma))
    Opts.IgnoreLoopUnrollThresholdOnPragma = true;

  unsigned SIMDWidth = IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth);
  if (SIMDWidth == 8 || SIMDWidth == 16 || SIMDWidth == 32)
    Opts.InteropSubgroupSize = SIMDWidth;

  Opts.NoOptFinalizerMode =
      deriveDefaultableFlagValue<vc::NoOptFinalizerControl>(
          IGC_GET_FLAG_VALUE(VCNoOptFinalizerControl));

  Opts.DisableLRCoalescingMode =
      deriveDefaultableFlagValue<vc::DisableLRCoalescingControl>(
          IGC_GET_FLAG_VALUE(VCDisableLRCoalescingControl));
  Opts.DisableExtraCoalescingMode =
      deriveDefaultableFlagValue<vc::DisableExtraCoalescingControl>(
          IGC_GET_FLAG_VALUE(VCDisableExtraCoalescing));
  if (__IGC_OPAQUE_POINTERS_API_ENABLED ||
      IGC_IS_FLAG_ENABLED(EnableOpaquePointersBackend))
    Opts.EnableOpaquePointers = true;
}

static void adjustKernelMetrics(vc::CompileOptions &Opts) {
  if (IGC_IS_FLAG_ENABLED(EnableKernelCostInfo))
    Opts.CollectCostInfo = true;
}

static void adjustDumpOptions(vc::CompileOptions &Opts) {
  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
    Opts.DumpIR = true;
    Opts.DumpIsa = true;
    Opts.DumpAsm = true;
    Opts.DumpDebugInfo = true;
  }
}

static void adjustHashOptions(vc::CompileOptions &Opts,
                              const ShaderHash &Hash) {
  Opts.EnableHashMovs =
      IGC_IS_FLAG_DISABLED(ForceDisableShaderDebugHashCodeInKernel) &&
      IGC_IS_FLAG_ENABLED(ShaderDebugHashCodeInKernel);
  Opts.EnableHashMovsAtPrologue = IGC_IS_FLAG_ENABLED(EnableHashMovsAtPrologue);
  Opts.AsmHash = Hash.getAsmHash();
}

static void adjustOptions(const IGC::CPlatform &IGCPlatform,
                          TC::TB_DATA_FORMAT DataFormat,
                          vc::CompileOptions &Opts, BuildDiag &Diag,
                          const ShaderHash &Hash) {
  adjustPlatform(IGCPlatform, Opts);
  adjustFileType(DataFormat, Opts);
  adjustOptLevel(Opts);
  adjustDumpOptions(Opts);
  adjustStackCalls(Opts, Diag);
  adjustDebugStrippingPolicy(Opts);
  adjustHashOptions(Opts, Hash);

  adjustTransformationsAndOptimizations(Opts);
  adjustKernelMetrics(Opts);
}

static void setErrorMessage(llvm::StringRef ErrorMessage,
                            TC::STB_TranslateOutputArgs &pOutputArgs) {
  pOutputArgs.pErrorString = new char[ErrorMessage.size() + 1];
  memcpy_s(pOutputArgs.pErrorString, ErrorMessage.size(), ErrorMessage.data(),
           ErrorMessage.size());
  pOutputArgs.pErrorString[ErrorMessage.size()] = '\0';
  pOutputArgs.ErrorStringSize = ErrorMessage.size() + 1;
}

static void outputBinary(llvm::StringRef Binary, llvm::StringRef DebugInfo,
                         TC::STB_TranslateOutputArgs *OutputArgs) {
  size_t BinarySize = Binary.size();
  char *BinaryOutput = new char[BinarySize];
  memcpy_s(BinaryOutput, BinarySize, Binary.data(), BinarySize);
  OutputArgs->OutputSize = static_cast<uint32_t>(BinarySize);
  OutputArgs->pOutput = BinaryOutput;
  if (DebugInfo.size()) {
    char *DebugInfoOutput = new char[DebugInfo.size()];
    memcpy_s(DebugInfoOutput, DebugInfo.size(), DebugInfo.data(),
             DebugInfo.size());
    OutputArgs->pDebugData = DebugInfoOutput;
    OutputArgs->DebugDataSize = DebugInfo.size();
  }
}

// Similar to ShaderHashOCL though reinterpretation is hidden inside
// iStdLib so probably it will be safer (to use more specialized things).
static ShaderHash getShaderHash(llvm::ArrayRef<char> Input) {
  ShaderHash Hash;
  Hash.asmHash = iSTD::HashFromBuffer(Input.data(), Input.size());
  return Hash;
}

static void dumpInputData(vc::ShaderDumper &Dumper, llvm::StringRef ApiOptions,
                          llvm::StringRef InternalOptions,
                          llvm::ArrayRef<char> Input, bool IsRaw) {
  if (!IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    return;

  Dumper.dumpText(ApiOptions, IsRaw ? "options_raw" : "options");
  Dumper.dumpText(InternalOptions,
                  IsRaw ? "internal_options_raw" : "internal_options");
  Dumper.dumpBinary(Input, IsRaw ? "igc_input_raw" : "igc_input", "spv");
}

static bool tryAddAuxiliaryOptions(llvm::StringRef AuxOpt,
                                   llvm::StringRef InOpt,
                                   std::string &Storage) {
  if (AuxOpt.empty())
    return false;

  Storage.clear();
  Storage.append(InOpt.data(), InOpt.size())
      .append(" ")
      .append(AuxOpt.data(), AuxOpt.size());
  return true;
}

// Parse initial options dumping all needed data.
// Return structure that describes compilation setup (CompileOptions).
// FIXME: payload decoding requires modification of input data so make
// it reference until the problem with option passing from FE to BE is
// solved in more elegant way.
static llvm::Expected<vc::CompileOptions>
parseOptions(vc::ShaderDumper &Dumper, llvm::StringRef ApiOptions,
             llvm::StringRef InternalOptions, llvm::ArrayRef<char> &Input) {
  auto RawInputDumper = llvm::make_scope_exit([=, &Dumper]() {
    dumpInputData(Dumper, ApiOptions, InternalOptions, Input, /*IsRaw=*/true);
  });

  std::string ApiOptionsHolder;
  std::string InternalOptionsHolder;
  auto LegacyPayload = tryExtractPayload(Input.data(), Input.size());
  if (LegacyPayload.IsValid) {
    ApiOptionsHolder = "-vc-codegen";
    ApiOptionsHolder.append(" ");
    ApiOptionsHolder.append(LegacyPayload.VCApiOpts.str());
    ApiOptions = ApiOptionsHolder;

    InternalOptionsHolder = InternalOptions.str();
    InternalOptionsHolder.append(" ");
    InternalOptionsHolder.append(LegacyPayload.VCInternalOpts.str());
    InternalOptions = InternalOptionsHolder;

    Input = Input.take_front(static_cast<size_t>(LegacyPayload.IRSize));
  }

  std::string AuxApiOptions;
  std::string AuxInternalOptions;
  if (tryAddAuxiliaryOptions(IGC_GET_REGKEYSTRING(VCApiOptions), ApiOptions,
                             AuxApiOptions))
    ApiOptions = {AuxApiOptions.data(), AuxApiOptions.size()};
  if (tryAddAuxiliaryOptions(IGC_GET_REGKEYSTRING(VCInternalOptions),
                             InternalOptions, AuxInternalOptions))
    InternalOptions = {AuxInternalOptions.data(), AuxInternalOptions.size()};

  auto InputDumper = llvm::make_scope_exit([=, &Dumper]() {
    dumpInputData(Dumper, ApiOptions, InternalOptions, Input, /*IsRaw=*/false);
  });

  const bool IsStrictParser = IGC_GET_FLAG_VALUE(VCStrictOptionParser);
  auto ExpOptions =
      vc::ParseOptions(ApiOptions, InternalOptions, IsStrictParser);
  if (ExpOptions.errorIsA<vc::NotVCError>()) {
    RawInputDumper.release();
    InputDumper.release();
  }
  if (ExpOptions)
    ExpOptions->ApiOptions = ApiOptions.str();
  return std::move(ExpOptions);
}

// Returns whether the modules were successfully obtained.
template <enum vc::bif::RawKind Printf32, enum vc::bif::RawKind Printf64>
bool fillPrintfData(vc::ExternalData &ExtData) {
  ExtData.VCPrintf32BIFModule = getVCModuleBuffer<Printf32>();
  if (!ExtData.VCPrintf32BIFModule)
    return false;
  ExtData.VCPrintf64BIFModule = getVCModuleBuffer<Printf64>();
  return static_cast<bool>(ExtData.VCPrintf64BIFModule);
}

static std::optional<vc::ExternalData>
fillExternalData(vc::BinaryKind Binary, llvm::StringRef CPUStr,
                 llvm::ArrayRef<const char *> VISALTOStrings,
                 llvm::ArrayRef<const char *> DirectCallFunctions) {
  vc::ExternalData ExtData;
  switch (Binary) {
  case vc::BinaryKind::CM:
    if (!fillPrintfData<vc::bif::RawKind::PrintfCM32,
                        vc::bif::RawKind::PrintfCM64>(ExtData))
      return {};
    break;
  case vc::BinaryKind::OpenCL:
    if (!fillPrintfData<vc::bif::RawKind::PrintfOCL32,
                        vc::bif::RawKind::PrintfOCL64>(ExtData))
      return {};
    break;
  case vc::BinaryKind::ZE:
    if (!fillPrintfData<vc::bif::RawKind::PrintfZE32,
                        vc::bif::RawKind::PrintfZE64>(ExtData))
      return {};
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unknown binary format");
  }
  ExtData.VCBuiltinsBIFModule =
      getVCModuleBufferForArch<vc::bif::RawKind::Builtins>(CPUStr);
  if (!ExtData.VCBuiltinsBIFModule)
    return {};
  ExtData.VCSPIRVBuiltinsBIFModule =
      getVCModuleBuffer<vc::bif::RawKind::SPIRVBuiltins>();
  if (!ExtData.VCSPIRVBuiltinsBIFModule)
    return {};

  ExtData.VISALTOStrings = VISALTOStrings;
  ExtData.DirectCallFunctions = DirectCallFunctions;

  return std::move(ExtData);
}

static void dumpPlatform(const vc::CompileOptions &Opts, PLATFORM Platform,
                         vc::ShaderDumper &Dumper) {
#if defined(_DEBUG) || defined(_INTERNAL)
  if (!IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    return;

  std::ostringstream Os;
  auto Core = Platform.eDisplayCoreFamily;
  auto RenderCore = Platform.eRenderCoreFamily;
  auto Product = Platform.eProductFamily;
  auto RevId = Platform.usRevId;

  Os << "NEO passed: DisplayCore = " << Core << ", RenderCore = " << RenderCore
     << ", Product = " << Product << ", Revision = " << RevId << "\n";
  Os << "IGC translated into: "
     << (Opts.CPUStr.empty() ? "(empty)" : Opts.CPUStr) << ", " << Opts.RevId
     << "\n";

  Dumper.dumpText(Os.str(), "platform.be");
#endif
}

static bool textRelocationsMatch(const vc::CMKernel &K) {
  auto &Info = K.getProgramOutput();
  return Info.m_relocs.size() == Info.m_funcRelocationTableEntries;
}

static void validateCMProgramForOCLBin(const vc::CGen8CMProgram &CMProgram) {
  IGC_ASSERT_MESSAGE(
      CMProgram.m_programInfo->m_GlobalPointerAddressRelocAnnotation.globalReloc
          .empty(),
      "global section relocations aren't supported for oclbin");
  IGC_ASSERT_MESSAGE(
      CMProgram.m_programInfo->m_GlobalPointerAddressRelocAnnotation
          .globalConstReloc.empty(),
      "constant section relocations aren't supported for oclbin");
  // FIXME: Relocations in indirect functions are unsupported for oclbin. They
  // are supported for zebin. So zebin and oclbin data is compared here to
  // diagnose the issue.
  // FIXME: It is possible to have a legal case where the number of relocations
  // for zebin and oclbin differs in future. The check must be updated in
  // this case. For now number of relocations is expected to match or to be 0
  // for oclbin and not 0 for zebin in case of relocations in indirect
  // functions.
  IGC_ASSERT_MESSAGE(llvm::all_of(CMProgram.m_kernels,
                                  [](const std::unique_ptr<vc::CMKernel> &K) {
                                    return textRelocationsMatch(*K);
                                  }),
                     "some text relocations are lost for oclbin");
}

static llvm::Expected<bool>
translateBuild(const TC::STB_TranslateInputArgs *InputArgs,
               TC::STB_TranslateOutputArgs *OutputArgs,
               TC::TB_DATA_FORMAT InputDataFormatTemp,
               const IGC::CPlatform &IGCPlatform,
               float ProfilingTimerResolution, llvm::raw_ostream &BuildLogOut) {
  llvm::StringRef ApiOptions{InputArgs->pOptions, InputArgs->OptionsSize};
  llvm::StringRef InternalOptions{InputArgs->pInternalOptions,
                                  InputArgs->InternalOptionsSize};
  llvm::ArrayRef<char> Input{InputArgs->pInput, InputArgs->InputSize};

  const auto Hash = getShaderHash(Input);
  auto Dumper = IGC_IS_FLAG_ENABLED(ShaderDumpEnable)
                    ? vc::createVC_IGCFileDumper(Hash)
                    : vc::createDefaultShaderDumper();
  BuildDiag Diag(BuildLogOut);

  // Parse and adjust VC options passed by a user and a runtime.
  auto ExpOptions = parseOptions(*Dumper, ApiOptions, InternalOptions, Input);
  if (!ExpOptions)
    return ExpOptions.takeError();
  vc::CompileOptions &Opts = ExpOptions.get();
  adjustOptions(IGCPlatform, InputDataFormatTemp, Opts, Diag, Hash);

  dumpPlatform(Opts, IGCPlatform.getPlatformInfo(), *Dumper);

  if (Opts.CPUStr.empty())
    return llvm::make_error<vc::OptionError>("Unknown target platform", false);

  if (IGC_IS_FLAG_ENABLED(ShaderOverride))
    Opts.ShaderOverrider =
        vc::createVC_IGCShaderOverrider(Hash, IGCPlatform.getPlatformInfo());

  Opts.Dumper = std::move(Dumper);

  llvm::ArrayRef<const char *> VISALTOStrings{InputArgs->pVISAAsmToLinkArray,
                                              InputArgs->NumVISAAsmsToLink};
  llvm::ArrayRef<const char *> DirectCallFunctions{
      InputArgs->pDirectCallFunctions, InputArgs->NumDirectCallFunctions};

  auto ExtData = fillExternalData(Opts.Binary, Opts.CPUStr, VISALTOStrings,
                                  DirectCallFunctions);
  if (!ExtData)
    return llvm::make_error<vc::BifLoadingError>();

  llvm::ArrayRef<uint32_t> SpecConstIds{InputArgs->pSpecConstantsIds,
                                        InputArgs->SpecConstantsSize};
  llvm::ArrayRef<uint64_t> SpecConstValues{InputArgs->pSpecConstantsValues,
                                           InputArgs->SpecConstantsSize};

  vc::ExternalData &extDataValue = ExtData.value();

  auto ExpOutput = vc::Compile(Input, Opts, extDataValue, SpecConstIds,
                               SpecConstValues, BuildLogOut);
  if (!ExpOutput)
    return ExpOutput.takeError();
  auto &CompileResult = ExpOutput.get();

  vc::CGen8CMProgram CMProgram{Opts, IGCPlatform.getPlatformInfo(),
                               IGCPlatform.getWATable(), Input};
  vc::createBinary(CMProgram, CompileResult);

  switch (Opts.Binary) {
  case vc::BinaryKind::OpenCL: {
    validateCMProgramForOCLBin(CMProgram);
    CMProgram.CreateKernelBinaries();
    Util::BinaryStream ProgramBinary;
    CMProgram.GetProgramBinary(ProgramBinary, CompileResult.PointerSizeInBytes);
    llvm::StringRef BinaryRef{ProgramBinary.GetLinearPointer(),
                              static_cast<std::size_t>(ProgramBinary.Size())};

    Util::BinaryStream ProgramDebugData;
    CMProgram.GetProgramDebugData(ProgramDebugData);
    llvm::StringRef DebugInfoRef{
        ProgramDebugData.GetLinearPointer(),
        static_cast<std::size_t>(ProgramDebugData.Size())};

    if (CMProgram.HasErrors())
      return CMProgram.GetError();

    outputBinary(BinaryRef, DebugInfoRef, OutputArgs);
    break;
  }
  case vc::BinaryKind::ZE: {
    llvm::SmallVector<char, 0> ProgramBinary;
    llvm::raw_svector_ostream ProgramBinaryOS{ProgramBinary};
    CMProgram.GetZEBinary(ProgramBinaryOS, CompileResult.PointerSizeInBytes);

    if (CMProgram.HasErrors())
      return CMProgram.GetError();

    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
      Opts.Dumper->dumpBinary(ProgramBinary, "", "progbin");

    llvm::StringRef BinaryRef{ProgramBinary.data(), ProgramBinary.size()};
    outputBinary(BinaryRef, {}, OutputArgs);
    break;
  }
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unknown binary format");
  }

  return true;
}

std::error_code vc::translateBuild(const TC::STB_TranslateInputArgs *InputArgs,
                                   TC::STB_TranslateOutputArgs *OutputArgs,
                                   TC::TB_DATA_FORMAT InputDataFormatTemp,
                                   const IGC::CPlatform &IGCPlatform,
                                   float ProfilingTimerResolution) {
  std::string BuildLog;
  llvm::raw_string_ostream BuildLogOut(BuildLog);

  auto R = ::translateBuild(InputArgs, OutputArgs, InputDataFormatTemp,
                            IGCPlatform, ProfilingTimerResolution, BuildLogOut);
  IGC_ASSERT(OutputArgs->pErrorString == nullptr);

  std::error_code Status;
  if (!R)
    llvm::handleAllErrors(
        R.takeError(), [&Status, &BuildLogOut](const llvm::ErrorInfoBase &EI) {
          Status = EI.convertToErrorCode();
          BuildLogOut << EI.message() << "\n";
        });

  if (!BuildLog.empty()) {
#ifndef NDEBUG
    llvm::errs() << BuildLog;
#endif // NDEBUG
    setErrorMessage(BuildLog, *OutputArgs);
  }

  return Status;
}

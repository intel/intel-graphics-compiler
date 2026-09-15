/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test the IGC (libigc) CIF interfaces

#include "OCLInterfaceTester.h"

#include "ocl_igc_interface/igc_ocl_device_ctx.h"

// Match one supported field by name: set it from the parsed value, read it back
// Uses key/value from the enclosing forEachStdinField lambda.
#define FIELD(h, NAME, TYPE)                                                                                           \
  if (key == #NAME) {                                                                                                  \
    uint64_t val = 0;                                                                                                  \
    if (!parseValue(value, val))                                                                                       \
      return false;                                                                                                    \
    h->Set##NAME((TYPE)val);                                                                                           \
    std::cout << #NAME "=" << h->Get##NAME() << "\n";                                                                  \
    return true;                                                                                                       \
  }

// Interface No. 1 - GTSystemInfo sub-interface
IGC_CHECK(gtsysinfo, "Interface v1: test that GTSystemInfo is accessible") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<1>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<1> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  auto h = deviceCtx->GetGTSystemInfoHandle();
  if (!h) {
    std::cerr << "error: failed to get GTSystemInfo handle\n";
    return ExitCode::FailedToGetInterface;
  }

  // Set/get each supported field from stdin. Add a FIELD line to support a new one.
  return forEachStdinField([&](std::string_view key, std::string_view value) {
    FIELD(h, EUCount, uint32_t)                     // v1
    FIELD(h, ThreadCount, uint32_t)                 // v1
    FIELD(h, SliceCount, uint32_t)                  // v1
    FIELD(h, SubSliceCount, uint32_t)               // v1
    FIELD(h, L3CacheSizeInKb, uint64_t)             // v1
    FIELD(h, LLCCacheSizeInKb, uint64_t)            // v1
    FIELD(h, EdramSizeInKb, uint64_t)               // v1
    FIELD(h, L3BankCount, uint32_t)                 // v1
    FIELD(h, MaxFillRate, uint32_t)                 // v1
    FIELD(h, EuCountPerPoolMax, uint32_t)           // v1
    FIELD(h, EuCountPerPoolMin, uint32_t)           // v1
    FIELD(h, TotalVsThreads, uint32_t)              // v1
    FIELD(h, TotalHsThreads, uint32_t)              // v1
    FIELD(h, TotalDsThreads, uint32_t)              // v1
    FIELD(h, TotalGsThreads, uint32_t)              // v1
    FIELD(h, TotalPsThreadsWindowerRange, uint32_t) // v1
    FIELD(h, CsrSizeInMb, uint32_t)                 // v1
    FIELD(h, MaxEuPerSubSlice, uint32_t)            // v1
    FIELD(h, MaxSlicesSupported, uint32_t)          // v1
    FIELD(h, MaxSubSlicesSupported, uint32_t)       // v1
    FIELD(h, IsL3HashModeEnabled, bool)             // v1
    FIELD(h, IsDynamicallyPopulated, bool)          // v1
    FIELD(h, MaxDualSubSlicesSupported, uint32_t)   // v3
    FIELD(h, DualSubSliceCount, uint32_t)           // v3
    FIELD(h, SLMSizeInKb, uint32_t)                 // v4
    return unknownField(key);
  });
}

// Interface No. 1 - IgcFeaturesAndWorkarounds sub-interface
IGC_CHECK(features, "Interface v1: test that IgcFeaturesAndWorkarounds is accessible") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<1>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<1> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  // v3 and v4 in IgcFeaturesAndWorkarounds are siblings (both derive from v2), and
  // FtrEfficient64BitAddressing exists in both. Request the handle at the same
  // version as H (Latest) so the object type matches the member functions below.
  using H = IGC::IgcFeaturesAndWorkaroundsLatest;
  auto h = deviceCtx->GetIgcFeaturesAndWorkaroundsHandle<H>();
  if (!h) {
    std::cerr << "error: failed to get IgcFeaturesAndWorkarounds handle\n";
    return ExitCode::FailedToGetInterface;
  }

  // Set/get each supported field from stdin. Add a FIELD line to support a new one.
  return forEachStdinField([&](std::string_view key, std::string_view value) {
    FIELD(h, FtrDesktop, bool)                    // v1
    FIELD(h, FtrChannelSwizzlingXOREnabled, bool) // v1
    FIELD(h, FtrGtBigDie, bool)                   // v1
    FIELD(h, FtrGtMediumDie, bool)                // v1
    FIELD(h, FtrGtSmallDie, bool)                 // v1
    FIELD(h, FtrGT1, bool)                        // v1
    FIELD(h, FtrGT1_5, bool)                      // v1
    FIELD(h, FtrGT2, bool)                        // v1
    FIELD(h, FtrGT3, bool)                        // v1
    FIELD(h, FtrGT4, bool)                        // v1
    FIELD(h, FtrIVBM0M1Platform, bool)            // v1
    FIELD(h, FtrGTL, bool)                        // v1
    FIELD(h, FtrGTM, bool)                        // v1
    FIELD(h, FtrGTH, bool)                        // v1
    FIELD(h, FtrSGTPVSKUStrapPresent, bool)       // v1
    FIELD(h, FtrGTA, bool)                        // v1
    FIELD(h, FtrGTC, bool)                        // v1
    FIELD(h, FtrGTX, bool)                        // v1
    FIELD(h, Ftr5Slice, bool)                     // v1
    FIELD(h, FtrGpGpuMidThreadLevelPreempt, bool) // v1
    FIELD(h, FtrIoMmuPageFaulting, bool)          // v1
    FIELD(h, FtrWddm2Svm, bool)                   // v1
    FIELD(h, FtrPooledEuEnabled, bool)            // v1
    FIELD(h, FtrResourceStreamer, bool)           // v1
    FIELD(h, MaxOCLParamSize, uint32_t)           // v2
    FIELD(h, FtrEfficient64BitAddressing, bool)   // v4
    return unknownField(key);
  });
}

// Interface No. 2
IGC_CHECK(system_routine, "Interface v2: test that system routine is returned correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<2>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<2> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  // SystemRoutineBuffer
  auto sysBuf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
  if (!sysBuf) {
    std::cerr << "error: failed to create output buffer\n";
    return ExitCode::FailedToGetInterface;
  }

  // StateSaveAreaHeaderInitBuffer
  auto ssaBuf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
  if (!ssaBuf) {
    std::cerr << "error: failed to create output buffer\n";
    return ExitCode::FailedToGetInterface;
  }

  // Platform: required to run system routine. GetSystemRoutine selects the SIP
  // binary from the render core family and product family, so both must be set
  auto platform = deviceCtx->GetPlatformHandle();
  if (!applyPlatform(platform.get())) {
    std::cerr << "error: check 'system_routine' requires --platform <name>\n";
    return ExitCode::MissingPlatform;
  }

  // GT system info: required to run system routine. The SIP state-save-area
  // header is populated from GT info, and MaxSlicesSupported must be > 0 (it is
  // asserted in populateSIPKernelInfo). It is the only gt field strictly required.
  auto gtsysinfo = deviceCtx->GetGTSystemInfoHandle();
  gtsysinfo->SetMaxSlicesSupported(8);

  // IGC::SystemRoutineType::contextSaveRestore
  bool ret = deviceCtx->GetSystemRoutine(IGC::SystemRoutineType::debug, /*bindless*/ true, sysBuf.get(), ssaBuf.get());
  if (!ret) {
    std::cerr << "error: failed to get system routine\n";
    return ExitCode::FailedToGetInterface;
  }

  std::cout << "supported=" << ret << "\n";
  std::cout << "systemRoutineBytes=" << sysBuf->GetSize<char>() << "\n";

  // The state-save-area header starts with an 8-byte magic: "tssarea".
  std::string_view ssa = readBuf(ssaBuf.get());
  std::string_view magic = ssa.substr(0, ssa.find('\0'));
  std::cout << "stateSaveAreaMagic=" << magic << "\n";

  return ExitCode::Success;
}

// Interface No. 3
IGC_CHECK(revision, "Interface v3: test that revision is returned correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<3>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<3> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  const char *rev = deviceCtx->GetIGCRevision();
  if (!rev || rev[0] == '\0') {
    std::cerr << "error: GetIGCRevision() returned empty string\n";
    return ExitCode::FailedToGetInterface;
  }

  std::cout << "revision=" << rev << "\n";
  return ExitCode::Success;
}

// Interface No. 4
IGC_CHECK(builtins, "Interface v4: test that builtin memory requirements are returned correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<4>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<4> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  auto builtins = deviceCtx->GetIgcBuiltinsHandle();
  if (!builtins) {
    std::cerr << "error: failed to get IgcBuiltins handle\n";
    return ExitCode::FailedToGetInterface;
  }

  // items/rangeSize/valueTypeSizeInBytes must all be provided via stdin.
  // Inputs start at -1 (a value stdin can never set, since negative values fail
  // to parse) so we can detect any that were not provided.
  long items = -1, rangeSize = -1, valueTypeSizeInBytes = -1;
  const std::map<std::string_view, long *> inputs = {
      {"items", &items},
      {"rangeSize", &rangeSize},
      {"valueTypeSizeInBytes", &valueTypeSizeInBytes},
  };

  // read stdin for FIELD=VALUE pairs
  int rc = forEachStdinField([&](std::string_view key, std::string_view value) {
    auto it = inputs.find(key);
    if (it == inputs.end())
      return unknownField(key);

    uint64_t val = 0;
    if (!parseValue(value, val))
      return false;

    *it->second = static_cast<long>(val);
    return true;
  });
  if (rc != ExitCode::Success)
    return rc;

  for (const auto &[name, ptr] : inputs) {
    if (*ptr < 0) {
      std::cerr << "error: missing required input '" << name << "'\n";
      return ExitCode::MissingInput;
    }
  }

  using B = IGC::IgcBuiltinsLatest;
  B::IGCBuiltinMemoryInfo memInfo = {};
  bool supported = builtins->GetBuiltinMemoryRequired(
      &memInfo, B::BuiltinAlgorithm::sort, B::SortAlgorithmVariant::defaultJointSort, B::BuiltinMemoryScope::workGroup,
      items, rangeSize, B::BuiltinDataType::uint32_type, valueTypeSizeInBytes);

  std::cout << "supported=" << supported << "\n";
  std::cout << "globalMemoryInBytes=" << memInfo.globalMemoryInBytes << "\n";
  std::cout << "sharedMemoryInBytes=" << memInfo.sharedMemoryInBytes << "\n";
  std::cout << "canMemoryBeUsedConcurrently=" << memInfo.canMemoryBeUsedConcurrently << "\n";
  return ExitCode::Success;
}

// Interface No. 5
IGC_CHECK(spirv_ext, "Interface v5: test that SPIR-V extensions supported by a platform are returned correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<5>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<5> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  // Apply the platform on the device ctx; the options-and-capabilities handle reads
  // it when computing the supported-extensions list
  auto platform = deviceCtx->GetPlatformHandle();
  if (!applyPlatform(platform.get())) {
    std::cerr << "error: check 'spirv_ext' requires --platform <name>\n";
    return ExitCode::MissingPlatform;
  }

  auto caps = deviceCtx->GetIgcOptionsAndCapabilitiesHandle();
  if (!caps) {
    std::cerr << "error: failed to get IgcOptionsAndCapabilities handle\n";
    return ExitCode::FailedToGetInterface;
  }

  auto buf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
  if (!buf) {
    std::cerr << "error: failed to create output buffer\n";
    return ExitCode::FailedToGetInterface;
  }

  caps->GetCompilerSupportedSPIRVExtensionsYAML(buf.get());

  std::cout << readBuf(buf.get()) << "\n";
  return ExitCode::Success;
}

// Translation path: the platform cross-check
IGC_CHECK(platform_check, "Translation: cross-check the render GMDID against the platform enum") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<1>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<1> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  // Platform<2> is the first version exposing SetRenderBlockID
  auto platform = deviceCtx->GetPlatformHandle<IGC::Platform<2>>();
  if (!platform) {
    std::cerr << "error: failed to get IGC::Platform<2> handle\n";
    return ExitCode::FailedToGetInterface;
  }
  if (!applyPlatform(platform.get())) {
    std::cerr << "error: check 'platform_check' requires --platform <name>\n";
    return ExitCode::MissingPlatform;
  }

  std::cout << "renderCoreFamily=" << getCoreName(getEffectiveCore()) << "\n";

  const uint32_t renderBlockID = applyRenderBlockID(platform.get());
  GFX_GMD_ID decoded = {};
  decoded.Value = renderBlockID;
  if (renderBlockID == 0)
    std::cout << "renderBlockID=not-reported\n";
  else
    std::cout << "renderBlockID=" << decoded.GmdID.GMDArch << "." << decoded.GmdID.GMDRelease << "."
              << decoded.GmdID.RevisionID << "\n";

  auto translationCtx = deviceCtx->CreateTranslationCtx(IGC::CodeType::spirV, IGC::CodeType::oclGenBin);
  if (!translationCtx) {
    std::cerr << "error: failed to create a SPIRV -> OCL_GEN_BIN translation context\n";
    return ExitCode::UnsupportedInterface;
  }

  // A bare SPIR-V header with no instructions: the five words every module starts
  // with, and nothing else. It parses, so IGC reaches the input and reports an
  // empty module through the normal channels instead of bailing out early.
  static const uint32_t emptySpirV[] = {
      0x07230203, // magic
      0x00010000, // version 1.0
      0x00000000, // generator
      0x00000001, // id bound
      0x00000000, // schema
  };
  auto srcBuf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, emptySpirV, sizeof(emptySpirV));
  auto optionsBuf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
  auto internalOptionsBuf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
  if (!srcBuf || !optionsBuf || !internalOptionsBuf) {
    std::cerr << "error: failed to create translation input buffers\n";
    return ExitCode::FailedToGetInterface;
  }

  auto out = translationCtx->Translate(srcBuf.get(), optionsBuf.get(), internalOptionsBuf.get(), nullptr, 0);
  if (!out) {
    std::cerr << "error: Translate() returned no output object\n";
    return ExitCode::FailedToGetInterface;
  }

  std::string_view log = readBuf(out->GetBuildLog<CIF::Builtins::BufferSimple>());
  while (!log.empty() && log.back() == '\0')
    log.remove_suffix(1);

  if (!log.empty())
    std::cout << log << "\n";

  return ExitCode::Success;
}

// Interface No. 6
IGC_CHECK(regkey, "Interface v6: test that regkey token is returned correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::IgcOclDeviceCtx<6>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::IgcOclDeviceCtx<6> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  auto buf = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
  if (!buf) {
    std::cerr << "error: failed to create output buffer\n";
    return ExitCode::FailedToGetInterface;
  }

  deviceCtx->GetIGCRegKeys(buf.get());

  std::cout << readBuf(buf.get()) << "\n";
  return ExitCode::Success;
}

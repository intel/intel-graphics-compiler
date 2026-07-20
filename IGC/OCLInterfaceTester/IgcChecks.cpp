/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test the IGC (libigc) CIF interfaces

#include "OCLInterfaceTester.h"

#include "ocl_igc_interface/igc_ocl_device_ctx.h"

// Match one supported field by name: set it from the parsed value, read it back
// Uses key/val from the enclosing forEachStdinField lambda.
#define FIELD(h, NAME, TYPE)                                                                                           \
  if (key == #NAME) {                                                                                                  \
    h->Set##NAME((TYPE)val);                                                                                           \
    std::cout << #NAME "=" << h->Get##NAME() << "\n";                                                                  \
    return true;                                                                                                       \
  }

// Interface No. 1 - GTSystemInfo sub-interface
CHECK(gtsysinfo, "Interface v1: test that GTSystemInfo is accessible") {
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
  return forEachStdinField([&](std::string_view key, uint64_t val) {
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
CHECK(features, "Interface v1: test that IgcFeaturesAndWorkarounds is accessible") {
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
  return forEachStdinField([&](std::string_view key, uint64_t val) {
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
CHECK(system_routine, "Interface v2: test that system routine is returned correctly") {
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

// Interface No. 4
CHECK(builtins, "Interface v4: test that builtin memory requirements are returned correctly") {
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
  int rc = forEachStdinField([&](std::string_view key, uint64_t val) {
    auto it = inputs.find(key);
    if (it == inputs.end())
      return unknownField(key);
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
CHECK(spirv_ext, "Interface v5: test that SPIR-V extensions supported by a platform are returned correctly") {
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

// Interface No. 6
CHECK(regkey, "Interface v6: test that regkey token is returned correctly") {
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

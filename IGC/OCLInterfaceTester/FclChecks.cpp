/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test the FCL (libigdfcl) CIF interfaces

#include "OCLInterfaceTester.h"

#include "ocl_igc_interface/fcl_ocl_device_ctx.h"

// Accepted code-type names
static constexpr struct {
  std::string_view name;
  IGC::CodeType::CodeType_t code;
} codeTypes[] = {
    {"OCL_C", IGC::CodeType::oclC},
    {"OCL_CPP", IGC::CodeType::oclCpp},
    {"ELF", IGC::CodeType::elf},
    {"SPIRV", IGC::CodeType::spirV},
    {"LLVM_BC", IGC::CodeType::llvmBc},
    {"LLVM_LL", IGC::CodeType::llvmLl},
    {"OCL_GEN_BIN", IGC::CodeType::oclGenBin},
};

static bool findCodeType(std::string_view name, IGC::CodeType::CodeType_t &code) {
  for (const auto &entry : codeTypes)
    if (name == entry.name) {
      code = entry.code;
      return true;
    }
  return false;
}

// Interface No. 2
FCL_CHECK(fcl_preferred_ir,
          "[FCL] Interface v2: test that the preferred intermediate representation is returned correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::FclOclDeviceCtx<2>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::FclOclDeviceCtx<2> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  const auto ir = deviceCtx->GetPreferredIntermediateRepresentation();
  std::cout << "preferredIR=" << IGC::CodeType::CodeTypeCoder::Dec(ir) << "\n";
  return ExitCode::Success;
}

FCL_CHECK(fcl_versions, "[FCL] Test that the supported FclOclDeviceCtx version range is returned correctly") {
  CIF::Version_t verMin = 0;
  CIF::Version_t verMax = 0;
  if (!cif->GetSupportedVersions(IGC::FclOclDeviceCtx<CIF::BaseVersion>::GetInterfaceId(), verMin, verMax)) {
    std::cerr << "error: failed to query supported FclOclDeviceCtx versions\n";
    return ExitCode::UnsupportedInterface;
  }

  std::cout << "versionMin=" << verMin << "\n";
  std::cout << "versionMax=" << verMax << "\n";
  return ExitCode::Success;
}

// Interface No. 3
FCL_CHECK(fcl_translation_pairs,
          "[FCL] Interface v3: test that the supported translation code-type pairs are reported correctly") {
  auto deviceCtx = cif->CreateInterface<IGC::FclOclDeviceCtx<3>>();
  if (!deviceCtx) {
    std::cerr << "error: failed to create IGC::FclOclDeviceCtx<3> interface\n";
    return ExitCode::UnsupportedInterface;
  }

  return forEachStdinField([&](std::string_view inName, std::string_view outName) {
    IGC::CodeType::CodeType_t in = IGC::CodeType::undefined;
    IGC::CodeType::CodeType_t out = IGC::CodeType::undefined;
    if (!findCodeType(inName, in) || !findCodeType(outName, out)) {
      std::cerr << "error: expected two known code-type names in line '" << inName << " " << outName << "'\n";
      return false;
    }

    auto err = CIF::Builtins::CreateConstBuffer<CIF::Builtins::BufferSimple>(cif, nullptr, 0);
    if (!err) {
      std::cerr << "error: failed to create error buffer\n";
      return false;
    }

    auto translationCtx = deviceCtx->CreateTranslationCtx(in, out, err.get());
    std::cout << IGC::CodeType::CodeTypeCoder::Dec(in) << " -> " << IGC::CodeType::CodeTypeCoder::Dec(out) << " = "
              << (translationCtx ? 1 : 0) << "\n";
    return true;
  });
}

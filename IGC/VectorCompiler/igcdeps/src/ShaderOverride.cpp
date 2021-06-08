/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Class provides methods needed for construct kernel names
// and represents state of overriding process.
//

#include <common/debug/Dump.hpp>
#include <common/shaderHash.hpp>
#include <common/shaderOverride.hpp>

#include "vc/igcdeps/ShaderOverride.h"

using Extensions = vc::ShaderOverrider::Extensions;
namespace {

class VC_IGCShaderOverrider final : public vc::ShaderOverrider {
  IGC::Debug::DumpName NamePrefix;
  PLATFORM const &Platform;
  IGC::Debug::OutputFolderName Folder;

  // Gets full kernel name:
  // VC_<HASH>_<ShaderName>
  IGC::Debug::DumpName composeShaderName(std::string const &ShaderName) const;

  // Gets full path to file with extension:
  // "/path/to/shaderOverride/VC_<HASH>_<ShaderName>.Ext"
  std::string path(std::string const &ShaderName,
                   vc::ShaderOverrider::Extensions Ext) const;

public:
  VC_IGCShaderOverrider(ShaderHash const &Hash, PLATFORM const &Platform);

  // Overrides .asm or .dat files
  bool override(void *&GenXBin, int &GenXBinSize, llvm::StringRef ShaderName,
                Extensions Ext) const override;
};

} // namespace

VC_IGCShaderOverrider::VC_IGCShaderOverrider(ShaderHash const &InHash,
                                             PLATFORM const &InPlatform)
    : NamePrefix{IGC::Debug::DumpName("VC").Hash(InHash)}, Platform{InPlatform},
      Folder{IGC::Debug::GetShaderOverridePath()} {}

// Returns appropriate Name with no special symbols. They are replaced with
// '_'.
// Name = gjdn&85Lg -> return =  gjdn_85Lg
static std::string legalizeName(std::string Name) {
  std::replace_if(Name.begin(), Name.end(),
                  [](unsigned char c) { return (!isalnum(c) && c != '_'); },
                  '_');
  return Name;
}

bool VC_IGCShaderOverrider::override(void *&GenXBin, int &GenXBinSize,
                                     llvm::StringRef ShaderName,
                                     Extensions Ext) const {
  std::string const LegalizedShaderName = legalizeName(ShaderName.str());
  std::string const FullPath = path(LegalizedShaderName, Ext);
  bool Status = false;

  switch (Ext) {
  case Extensions::ASM:
    overrideShaderIGA(Platform, GenXBin, GenXBinSize, FullPath, Status);
    break;
  case Extensions::DAT:
    overrideShaderBinary(GenXBin, GenXBinSize, FullPath, Status);
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unexpected extension");
  }
  return Status;
}

// Returns name with added postfix
static IGC::Debug::DumpName addPostfix(IGC::Debug::DumpName const &Name,
                                       std::string const &Postfix) {
  return Name.PostFix(Postfix);
}

IGC::Debug::DumpName
VC_IGCShaderOverrider::composeShaderName(std::string const &ShaderName) const {
  return addPostfix(NamePrefix, ShaderName);
}

std::string
VC_IGCShaderOverrider::path(std::string const &ShaderName,
                            vc::ShaderOverrider::Extensions Ext) const {
  IGC::Debug::DumpName FullPath{composeShaderName(ShaderName)};

  switch (Ext) {
  case Extensions::VISAASM:
    FullPath = FullPath.Extension("visaasm");
    break;
  case Extensions::ASM:
    FullPath = FullPath.Extension("asm");
    break;
  case Extensions::DAT:
    FullPath = FullPath.Extension("dat");
    break;
  case Extensions::LL:
    FullPath = FullPath.Extension("ll");
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unexpected extension");
  }

  return FullPath.AbsolutePath(Folder);
}

namespace vc {
std::unique_ptr<ShaderOverrider>
createVC_IGCShaderOverrider(ShaderHash const &Hash, PLATFORM const &Platform) {
  return std::make_unique<VC_IGCShaderOverrider>(Hash, Platform);
}
} // namespace vc

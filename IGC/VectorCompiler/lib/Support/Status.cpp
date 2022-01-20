/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Support/Status.h"
#include "vc/Support/StatusCode.h"
#include "vc/Support/StatusTraits.h"
#include <string>
#include <system_error>
#include "Probe/Assertion.h"

namespace {
class vc_error_category : public std::error_category {
public:
  const char *name() const noexcept override;
  std::string message(int condition) const override;
};
} // namespace

const char *vc_error_category::name() const noexcept {
  return "vector compiler";
}

std::string vc_error_category::message(int condition) const {
  using namespace vc;

  switch (static_cast<errc>(condition)) {
  case errc::dynamic_load_fail:
    return std::string(ErrorTraits<errc::dynamic_load_fail>::getMessage());
  case errc::symbol_not_found:
    return std::string(ErrorTraits<errc::symbol_not_found>::getMessage());
  case errc::bad_spirv:
    return std::string(ErrorTraits<errc::bad_spirv>::getMessage());
  case errc::bad_bitcode:
    return std::string(ErrorTraits<errc::bad_bitcode>::getMessage());
  case errc::invalid_module:
    return std::string(ErrorTraits<errc::invalid_module>::getMessage());
  case errc::target_machine_not_created:
    return std::string(ErrorTraits<errc::target_machine_not_created>::getMessage());
  case errc::not_vc_codegen:
    return std::string(ErrorTraits<errc::not_vc_codegen>::getMessage());
  case errc::invalid_api_option:
    return std::string(ErrorTraits<errc::invalid_api_option>::getMessage());
  case errc::invalid_internal_option:
    return std::string(ErrorTraits<errc::invalid_internal_option>::getMessage());
  case errc::bif_load_fail:
    return std::string(ErrorTraits<errc::bif_load_fail>::getMessage());
  case errc::output_not_created:
    return std::string(ErrorTraits<errc::output_not_created>::getMessage());
  }
  IGC_ASSERT_EXIT_MESSAGE(0, "Unknown error code");
}

static const vc_error_category vc_err_category;

namespace vc {

const std::error_category &err_category() noexcept { return vc_err_category; }

// DynLoadError {{
char DynLoadError::ID = 0;

void DynLoadError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::dynamic_load_fail>::getMessage() << ": " << Message;
}
// }}

// SymbolLookupError {{
char SymbolLookupError::ID = 0;

void SymbolLookupError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::symbol_not_found>::getMessage() << ": symbol '"
     << Symbol << "' was not found in '" << Library << "'";
}
// }}

// BadSpirvError {{
char BadSpirvError::ID = 0;

void BadSpirvError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::bad_spirv>::getMessage() << ": " << Message;
}
// }}

// BadBitcodeError {{
char BadBitcodeError::ID = 0;

void BadBitcodeError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::bad_bitcode>::getMessage() << ": " << Message;
}
// }}

// InvalidModuleError {{
char InvalidModuleError::ID = 0;

void InvalidModuleError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::invalid_module>::getMessage();
}
// }}

// TargetMachineError {{
char TargetMachineError::ID = 0;

void TargetMachineError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::target_machine_not_created>::getMessage();
}
// }}

// NotVCError {{
char NotVCError::ID = 0;

void NotVCError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::not_vc_codegen>::getMessage();
}
// }}

// OptionErrorCommon {{
char OptionError::ID = 0;

void OptionError::log(llvm::raw_ostream &OS) const {
  if (IsInternal)
    OS << ErrorTraits<errc::invalid_internal_option>::getMessage();
  else
    OS << ErrorTraits<errc::invalid_api_option>::getMessage();
  OS << ": " << BadOption;
}
// }}

// OutputBinaryCreationError {{
char OutputBinaryCreationError::ID = 0;
void OutputBinaryCreationError::log(llvm::raw_ostream &OS) const {
  OS << ErrorTraits<errc::output_not_created>::getMessage();
  OS << ": " << Message;
}
// }}

} // namespace vc

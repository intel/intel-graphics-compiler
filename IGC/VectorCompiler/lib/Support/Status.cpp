/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "vc/Support/Status.h"

#include "vc/Support/StatusCode.h"
#include "vc/Support/StatusTraits.h"

#include "llvm/Support/ErrorHandling.h"

#include <string>
#include <system_error>

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
    return ErrorTraits<errc::dynamic_load_fail>::getMessage();
  case errc::symbol_not_found:
    return ErrorTraits<errc::symbol_not_found>::getMessage();
  case errc::bad_spirv:
    return ErrorTraits<errc::bad_spirv>::getMessage();
  case errc::bad_bitcode:
    return ErrorTraits<errc::bad_bitcode>::getMessage();
  case errc::invalid_module:
    return ErrorTraits<errc::invalid_module>::getMessage();
  case errc::target_machine_not_created:
    return ErrorTraits<errc::target_machine_not_created>::getMessage();
  case errc::not_vc_codegen:
    return ErrorTraits<errc::not_vc_codegen>::getMessage();
  case errc::invalid_api_option:
    return ErrorTraits<errc::invalid_api_option>::getMessage();
  case errc::invalid_internal_option:
    return ErrorTraits<errc::invalid_internal_option>::getMessage();
  }
  llvm_unreachable("Unknown error code");
}

static vc_error_category vc_err_category;

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

} // namespace vc

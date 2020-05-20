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

#ifndef VC_SUPPORT_STATUS_H
#define VC_SUPPORT_STATUS_H

#include "vc/Support/StatusCode.h"

#include <llvm/Support/Error.h>

#include <system_error>

namespace vc {

class DynLoadError final : public llvm::ErrorInfo<DynLoadError> {
public:
  static char ID;

private:
  std::string Message;

public:
  DynLoadError(llvm::StringRef Msg) : Message(Msg.str()) {}

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::dynamic_load_fail);
  }
};

class SymbolLookupError final : public llvm::ErrorInfo<SymbolLookupError> {
public:
  static char ID;

private:
  std::string Library;
  std::string Symbol;

public:
  SymbolLookupError(llvm::StringRef Lib, llvm::StringRef Sym)
      : Library(Lib.str()), Symbol(Sym.str()) {}

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::symbol_not_found);
  }
};

class BadSpirvError final : public llvm::ErrorInfo<BadSpirvError> {
public:
  static char ID;

private:
  std::string Message;

public:
  BadSpirvError(llvm::StringRef Msg) : Message(Msg.str()) {}

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::bad_spirv);
  }
};

class BadBitcodeError final : public llvm::ErrorInfo<BadBitcodeError> {
public:
  static char ID;

private:
  std::string Message;

public:
  BadBitcodeError(llvm::StringRef Msg) : Message(Msg.str()) {}

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::bad_bitcode);
  }
};

class InvalidModuleError final : public llvm::ErrorInfo<InvalidModuleError> {
public:
  static char ID;

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::invalid_module);
  }
};

class TargetMachineError final : public llvm::ErrorInfo<TargetMachineError> {
public:
  static char ID;

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::target_machine_not_created);
  }
};

class NotVCError final : public llvm::ErrorInfo<NotVCError> {
public:
  static char ID;

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::not_vc_codegen);
  }
};

class OptionError final : public llvm::ErrorInfo<OptionError> {
public:
  static char ID;

private:
  std::string BadOption;
  bool IsInternal;

public:
  OptionError(llvm::StringRef BadOpt, bool IsInternal_)
      : BadOption(BadOpt.str()), IsInternal(IsInternal_) {}

  bool isInternal() const { return IsInternal; }

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    const errc c =
        IsInternal ? errc::invalid_internal_option : errc::invalid_api_option;
    return make_error_code(c);
  }
};

} // namespace vc

#endif

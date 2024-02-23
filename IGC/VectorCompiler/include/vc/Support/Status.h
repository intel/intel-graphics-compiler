/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

class BifLoadingError final : public llvm::ErrorInfo<BifLoadingError> {
public:
  static char ID;

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::bif_load_fail);
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

class OutputBinaryCreationError final
    : public llvm::ErrorInfo<OutputBinaryCreationError> {
public:
  static char ID;

private:
  std::string Message;

public:
  OutputBinaryCreationError(llvm::StringRef Msg) : Message{Msg.str()} {}

  void log(llvm::raw_ostream &OS) const override;
  std::error_code convertToErrorCode() const override {
    return make_error_code(errc::output_not_created);
  }
};

} // namespace vc

#endif

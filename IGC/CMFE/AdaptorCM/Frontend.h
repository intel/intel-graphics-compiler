/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LLVM_CLANG_FRONTENDWRAPPER_FRONTEND_H
#define LLVM_CLANG_FRONTENDWRAPPER_FRONTEND_H

#include "Interface.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Process.h>

#include <memory>
#include <sstream>

namespace IGC {
namespace AdaptorCM {
namespace Frontend {

struct InputArgs final : public Intel::CM::ClangFE::IInputArgs {
  StrT InputText;
  SeqT<StrT> CompilationOpts;
  SeqT<FileT<StrT>> ExtraFiles;

  const StrT &getSrc() const override { return InputText; }
  const SeqT<StrT> &getCompOpts() const override { return CompilationOpts; }
  const SeqT<FileT<StrT>> &getExtraFiles() const override { return ExtraFiles; }
};

using StringVect_t = std::vector<std::string>;
inline std::pair<StringVect_t, StringVect_t>
convertBackendArgsToVcAndFinalizerOpts(const StringVect_t &BackendArgs) {
  StringVect_t VcOpts;
  StringVect_t FinalizerOpts;

  const std::string FinalizerOptPrefix = "-finalizer-opts=";
  auto IsFinalizerOpts = [&FinalizerOptPrefix](const std::string &Opt) {
    return Opt.find(FinalizerOptPrefix) == 0;
  };
  std::partition_copy(BackendArgs.begin(), BackendArgs.end(),
                      std::back_inserter(FinalizerOpts),
                      std::back_inserter(VcOpts), IsFinalizerOpts);
  std::transform(FinalizerOpts.begin(), FinalizerOpts.end(),
                 FinalizerOpts.begin(),
                 [&FinalizerOptPrefix](const std::string &Opt) {
                   return Opt.substr(FinalizerOptPrefix.size());
                 });
  return {VcOpts, FinalizerOpts};
}

using IOutputArgs = Intel::CM::ClangFE::IOutputArgs;
using IDriverInvocation = Intel::CM::ClangFE::IDriverInvocation;
using IDriverInvocationPtr =
    std::unique_ptr<IDriverInvocation, void (*)(IDriverInvocation *)>;
using IOutputArgsPtr = std::unique_ptr<IOutputArgs, void (*)(IOutputArgs *)>;

// Main helper class for interaction with FEWrapper library.
// FEWrapper is parametrized with error handling function
// that should have signature compatible with "void (std::string)".
// FEWrapper can be created using makeFEWrapper helper function.
// After creation, FEWrapper provides two methods to forward
// arguments to library functions.
template <typename Fn> class FEWrapper {
#if __cplusplus >= 201703L
  static_assert(std::is_invocable_r_v<void, Fn, std::string>,
                "Invalid error handler signature");
#endif

  using DyLibTy = llvm::sys::DynamicLibrary;

  Fn ErrHandler;
  DyLibTy Lib;

  template <typename SymTy> SymTy getLibSymbol(const char *Symbol) {
    auto *Addr = reinterpret_cast<SymTy>(Lib.getAddressOfSymbol(Symbol));
    if (!Addr) {
      std::string Err = "AdaptorCM: symbol is missing from CMFEWrapper: ";
      Err += Symbol;
      ErrHandler(Err);
    }
    return Addr;
  }

// Convenience macro to request symbols from library.
// x -- symbol identifier.
#define CMFE_WRAPPER_GET_SYMBOL_IMPL(x) (this->getLibSymbol<decltype(&x)>(#x))
#define CMFE_WRAPPER_GET_SYMBOL(x) CMFE_WRAPPER_GET_SYMBOL_IMPL(x)

  bool isCompatibleABI() {
    auto *GetInterfaceVersion =
        CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEGetInterfaceVersion);
    if (!GetInterfaceVersion)
      return false;

    const auto LoadedVersion = GetInterfaceVersion();
    const auto SupportedVersion = Intel::CM::ClangFE::InterfaceVersion;
    const bool IsCompatible = LoadedVersion == SupportedVersion;
    if (!IsCompatible) {
      std::ostringstream ErrMsg;
      ErrMsg << "AdaptorCM: incompatible clangFEWrapper interface: expected = "
             << SupportedVersion << ", loaded = " << LoadedVersion;
      ErrHandler(ErrMsg.str());
    }
    return IsCompatible;
  }

  DyLibTy loadLibrary(const std::string &DefaultDir) {
    constexpr auto CustomPathEnv = "CM_FE_DIR";

    std::string LibDir;
    if (auto EnvDir = llvm::sys::Process::GetEnv(CustomPathEnv))
      LibDir = EnvDir.getValue();
    else
      LibDir = DefaultDir;

    llvm::SmallString<32> LibPath;
    llvm::sys::path::append(LibPath, LibDir, CMFE_WRAPPER_NAME);

    std::string FError;
    auto DL = llvm::sys::DynamicLibrary::getPermanentLibrary(LibPath.c_str(),
                                                             &FError);

    if (!DL.isValid()) {
      std::ostringstream os;
      os << "AdaptorCM: could not load FEWrapper: <" << LibPath.c_str()
         << ">: " << FError;
      ErrHandler(os.str());
    }
    return DL;
  }

  // Provide extra argument to guarantee correct overload resolution
  // for public copy/move ctors.
  template <typename ErrFn>
  FEWrapper(ErrFn &&ErrH, const std::string &DefaultPath)
      : ErrHandler(std::forward<ErrFn>(ErrH)), Lib(loadLibrary(DefaultPath)) {}

  template <typename ErrFn, typename ErrFnTy>
  friend llvm::Optional<FEWrapper<ErrFnTy>>
  makeFEWrapper(ErrFn &&ErrH, const std::string &DefaultDir);

public:
  FEWrapper(FEWrapper &&) = default;
  FEWrapper(const FEWrapper &) = default;

  IDriverInvocationPtr buildDriverInvocation(int Argc, const char **Argv) {
    auto *BuildDriverInvocation =
        CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEBuildDriverInvocation);
    auto Del = [](IDriverInvocation *Ptr) { Ptr->discard(); };
    if (!BuildDriverInvocation)
      return {nullptr, Del};
    IDriverInvocation *Drv = BuildDriverInvocation(Argc, Argv);
    return {Drv, Del};
  }

  IOutputArgsPtr translate(const InputArgs &Input) {
    auto *Compile = CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFECompile);
    auto Del = [](IOutputArgs *Ptr) { Ptr->discard(); };
    if (!Compile)
      return {nullptr, Del};
    IOutputArgs *Args = Compile(&Input);
    return {Args, Del};
  }

  bool isDriverShowVersionInvocation(IDriverInvocation *DriverInvoc) {
    auto *isShowVersionInvocation =
        CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEIsShowVersionInvocation);
    return isShowVersionInvocation(DriverInvoc);
  }

  int getRevIdSymInvocation(IDriverInvocation *DriverInvoc) {
    auto *RevIdSym = CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEGetRevId);
    return RevIdSym(DriverInvoc);
  }

  DyLibTy &LibInfo() { return Lib; }

  bool getPrintStats(IDriverInvocation *DriverInvoc) {
    auto *PrintStats = CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEGetPrintStats);
    return PrintStats(DriverInvoc);
  }

  std::string getStatsFile(IDriverInvocation *DriverInvoc) {
    auto *StatsFile = CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEGetStatsFile);
    return StatsFile(DriverInvoc);
  }

  std::string getVCApiOptions(IDriverInvocation *DriverInvoc) {
    auto *getVCOptions =
        CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEGetVCApiOptions);
    const char *vcOptions = getVCOptions(DriverInvoc);
    if (!vcOptions)
      return "";
    return vcOptions;
  }

#undef CMFE_WRAPPER_GET_SYMBOL
#undef CMFE_WRAPPER_GET_SYMBOL_IMPL
};

// Create FEWrapper with given error handler ErrH.
// DefaultDir parameter allows to override search order by providing
// absolute path to directory with FE wrapper. Defaults to empty string
// that is expanded to plain FE wrapper name.
// Return Optional as it can fail during loading.
template <typename ErrFn,
          typename ErrFnTy = typename std::decay<ErrFn>::type>
inline llvm::Optional<FEWrapper<ErrFnTy>>
makeFEWrapper(ErrFn &&ErrH, const std::string &DefaultDir = std::string{}) {
  FEWrapper<ErrFnTy> IFace{std::forward<ErrFn>(ErrH), DefaultDir};

  if (!IFace.Lib.isValid())
    return llvm::None;
  if (!IFace.isCompatibleABI())
    return llvm::None;
  return IFace;
}

} // namespace Frontend
} // namespace AdaptorCM
} // namespace IGC

#endif // LLVM_ADAPTOR_CM_FRONTEND_H

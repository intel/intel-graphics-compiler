#ifndef LLVM_ADAPTOR_CM_FRONTEND_H
#define LLVM_ADAPTOR_CM_FRONTEND_H

#include "Interface.h"

#include <llvm/ADT/Optional.h>
#include <llvm/Support/DynamicLibrary.h>

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
inline StringVect_t
convertBackendArgsToVcOpts(const StringVect_t &BackendArgs) {
  std::vector<std::string> VcOpts;
  std::vector<std::string> AllFinalizerOpts;

  const std::string FinalizerOptPrefix = "-finalizer-opts=";
  auto IsFinalizerOpts = [&FinalizerOptPrefix](const std::string &Opt) {
    return Opt.find(FinalizerOptPrefix) == 0;
  };
  std::partition_copy(BackendArgs.begin(), BackendArgs.end(),
                      std::back_inserter(AllFinalizerOpts),
                      std::back_inserter(VcOpts), IsFinalizerOpts);
  std::transform(AllFinalizerOpts.begin(), AllFinalizerOpts.end(),
                 AllFinalizerOpts.begin(),
                 [&FinalizerOptPrefix](const std::string &Opt) {
                   return Opt.substr(FinalizerOptPrefix.size());
                 });
  if (!AllFinalizerOpts.empty()) {
    // TODO: we should escape the joined finalizer options
    VcOpts.emplace_back((llvm::StringRef(FinalizerOptPrefix) + "\"" +
                         llvm::join(AllFinalizerOpts, " ") + "\"")
                            .str());
  }

  return VcOpts;
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

  DyLibTy loadLibrary() {
    constexpr auto CustomPathEnv = "CM_FE_DIR";

    std::string FELibName = CMFE_WRAPPER_NAME;
    auto EnvFE = llvm::sys::Process::GetEnv(CustomPathEnv);
    if (EnvFE)
      FELibName =
          llvm::sys::path::convert_to_slash(EnvFE.getValue() + "/" + FELibName);

    std::string FError;
    auto DL = llvm::sys::DynamicLibrary::getPermanentLibrary(FELibName.c_str(),
                                                             &FError);

    if (!DL.isValid()) {
      std::ostringstream os;
      os << "AdaptorCM: could not load FEWrapper: <" << FELibName
         << ">: " << FError;
      ErrHandler(os.str());
    }
    return DL;
  }

  // Provide extra argument to guarantee correct overload resolution
  // for public copy/move ctors.
  template <typename ErrFn>
  FEWrapper(ErrFn &&ErrH, bool)
      : ErrHandler(std::forward<ErrFn>(ErrH)), Lib(loadLibrary()) {}

  template <typename ErrFn, typename ErrFnTy>
  friend llvm::Optional<FEWrapper<ErrFnTy>> makeFEWrapper(ErrFn &&ErrH);

public:
  FEWrapper(FEWrapper &&) = default;
  FEWrapper(const FEWrapper &) = default;

  IDriverInvocationPtr buildDriverInvocation(std::vector<const char *> &Args) {
    auto *BuildDriverInvocation =
        CMFE_WRAPPER_GET_SYMBOL(IntelCMClangFEBuildDriverInvocation);
    auto Del = [](IDriverInvocation *Ptr) { Ptr->discard(); };
    if (!BuildDriverInvocation)
      return {nullptr, Del};
    IDriverInvocation *Drv =
        BuildDriverInvocation(static_cast<int>(Args.size()), Args.data());
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

#undef CMFE_WRAPPER_GET_SYMBOL
#undef CMFE_WRAPPER_GET_SYMBOL_IMPL
};

namespace detail {
// Metafunction to remove cvref and create function pointer if needed.
template <typename Ty> struct CleanFunctor {
  using NoCV = typename std::remove_cv<Ty>::type;
  using NoRef = typename std::remove_reference<NoCV>::type;
  static constexpr bool IsFunc = std::is_function<NoRef>::value;
  using type = typename std::conditional<IsFunc, NoRef *, NoRef>::type;
};
} // namespace detail

// Create FEWrapper with given error handler ErrH.
// Return Optional as it can fail during loading.
template <typename ErrFn,
          typename ErrFnTy = typename detail::CleanFunctor<ErrFn>::type>
inline llvm::Optional<FEWrapper<ErrFnTy>> makeFEWrapper(ErrFn &&ErrH) {
  FEWrapper<ErrFnTy> IFace{std::forward<ErrFn>(ErrH), false};

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

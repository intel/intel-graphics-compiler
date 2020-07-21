#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/ADT/StringExtras.h>

#include <algorithm>
#include <cassert>

#include "Frontend.h"
#include "InputArgsWrapper.h"

#if defined(__linux__)
#include <dlfcn.h>
#endif

using namespace IGC::AdaptorCM::Frontend;

namespace {
const char *getClangFELibName() {
  return CMFE_WRAPPER_NAME;
}

llvm::sys::DynamicLibrary getFELibrary() {

  static auto DL =
    llvm::sys::DynamicLibrary::getPermanentLibrary(getClangFELibName());

  if (!DL.isValid()) {
    llvm::report_fatal_error(
      std::string("AdaptorCM: could not load FEWrapper: <")
        .append(getClangFELibName()).append(">"), false);
  }
  return DL;
}

} // namespace

void IGC::AdaptorCM::Frontend::validateABICompatibility() {

  auto DL = getFELibrary();
  auto GetInterfaceVersion =
    reinterpret_cast<decltype(&IntelCMClangFEGetInterfaceVersion)>(
      DL.getAddressOfSymbol("IntelCMClangFEGetInterfaceVersion"));

  auto LoadedInterfaceVersion = GetInterfaceVersion();
  auto SupportedInterfaceVersion = Intel::CM::ClangFE::InterfaceVersion;
  if (SupportedInterfaceVersion != LoadedInterfaceVersion) {
    llvm::report_fatal_error(
      llvm::StringRef("AdaptorCM: incompatible clangFEWrapper interface: ") +
        "expected = " + llvm::Twine(SupportedInterfaceVersion) +
        ", loaded = " + llvm::Twine(LoadedInterfaceVersion));
  }
}

IDriverInvocation *
IGC::AdaptorCM::Frontend::getDriverInvocation(int argc, const char * argv[]) {

  if (!argv)
    llvm::report_fatal_error("no input arguments specified", false);

  if (argc < 0)
    llvm::report_fatal_error("invalid number of arguments specified", false);


  auto DL = getFELibrary();
  auto BuildCommandLine =
    reinterpret_cast<decltype(&IntelCMClangFEBuildDriverInvocation)>(
      DL.getAddressOfSymbol("IntelCMClangFEBuildDriverInvocation"));

  if (!BuildCommandLine) {
    llvm::report_fatal_error(
      "haven't found IntelCMClangFEBuildDriverInvocation function "
      "in the dynamic library", false);
  }

  return BuildCommandLine(argc, argv);
}

IOutputArgs *IGC::AdaptorCM::Frontend::translate(const IInputArgs *Input) {

  if (!Input) {
    llvm::report_fatal_error("empty input passed to AdaptorCM::Translate", false);
  }

  auto DL = getFELibrary();

  auto Compile = reinterpret_cast<decltype(&IntelCMClangFECompile)>(
      DL.getAddressOfSymbol("IntelCMClangFECompile"));
  if (!Compile) {
    llvm::report_fatal_error(
      "haven't found IntelCMClangFECompile function in the dynamic library", false);
  }

  InputArgsWrapper::ErrorType Error;
  InputArgsWrapper WrappedInput(*Input, Error);
  if (!Error.empty()) {
    llvm::report_fatal_error(Error, false);
  }

  return Compile(&WrappedInput);
}

using StringVect_t = IGC::AdaptorCM::Frontend::StringVect_t;

StringVect_t IGC::AdaptorCM::Frontend::convertBackendArgsToVcOpts(
        const StringVect_t& BeArgs) {

  std::vector<std::string> VcOpts;

  std::vector<std::string> AllFinalizerOpts;

  const std::string FinalizerOptPrefix = "-finalizer-opts=";
  auto IsFinalizerOpts = [&FinalizerOptPrefix](const std::string& Opt) {
    return Opt.find(FinalizerOptPrefix) == 0;
  };
  std::partition_copy(BeArgs.begin(), BeArgs.end(),
                      std::back_inserter(AllFinalizerOpts),
                      std::back_inserter(VcOpts),
                      IsFinalizerOpts);
  std::transform(AllFinalizerOpts.begin(), AllFinalizerOpts.end(),
                 AllFinalizerOpts.begin(),
                 [&FinalizerOptPrefix](const std::string& Opt) {
                    return Opt.substr(FinalizerOptPrefix.size());
                 });
  if (!AllFinalizerOpts.empty()) {
    //TODO: we should escape the joined finalizer options
    VcOpts.emplace_back((llvm::StringRef(FinalizerOptPrefix) + "\"" +
                         llvm::join(AllFinalizerOpts, " ") + "\"").str());
  }

  return VcOpts;
}

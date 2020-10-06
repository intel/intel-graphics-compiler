#include "llvm/Support/Process.h"
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Path.h>

#include <algorithm>
#include <cassert>
#include <sstream>

#include "Frontend.h"
#include "InputArgsWrapper.h"

#if defined(__linux__)
#include <dlfcn.h>
#endif

using namespace IGC::AdaptorCM::Frontend;

namespace {
llvm::sys::DynamicLibrary getFELibrary() {
  std::string FELibName = CMFE_WRAPPER_NAME;
  std::string FError;

  auto EnvFE = llvm::sys::Process::GetEnv("CM_FE_DIR");
  if (EnvFE)
    FELibName =
        llvm::sys::path::convert_to_slash(EnvFE.getValue() + "/" + FELibName);

  static auto DL = llvm::sys::DynamicLibrary::getPermanentLibrary(
      FELibName.c_str(), &FError);

  if (!DL.isValid()) {
    std::ostringstream os;
    os << "AdaptorCM: could not load FEWrapper: <" << FELibName
       << ">: " << FError;
    llvm::report_fatal_error(os.str(), false);
  }
  return DL;
}

} // namespace

bool IGC::AdaptorCM::Frontend::validateABICompatibility(
    IGC::AdaptorCM::Frontend::AbiCompatibilityInfo *AbiInfo) {

  if (AbiInfo) {
    AbiInfo->RequestedVersion = -1;
    AbiInfo->AvailableVersion = -1;
  }

  auto DL = getFELibrary();
  auto GetInterfaceVersion =
    reinterpret_cast<decltype(&IntelCMClangFEGetInterfaceVersion)>(
      DL.getAddressOfSymbol("IntelCMClangFEGetInterfaceVersion"));

  if (!GetInterfaceVersion)
    return false;

  const auto LoadedInterfaceVersion = GetInterfaceVersion();
  const auto SupportedInterfaceVersion = Intel::CM::ClangFE::InterfaceVersion;

  if (AbiInfo) {
    AbiInfo->RequestedVersion = SupportedInterfaceVersion;
    AbiInfo->AvailableVersion = LoadedInterfaceVersion;
  }
  return LoadedInterfaceVersion == SupportedInterfaceVersion;
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

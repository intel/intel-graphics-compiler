/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LLVM_FRONTEND_WRAPPER_INTERFACE_H
#define LLVM_FRONTEND_WRAPPER_INTERFACE_H

#include <string>
#include <vector>

#if defined _WIN32 || defined __CYGWIN__
#ifdef INTEL_CM_CLANGFE_LIB
#define INTEL_CM_CLANGFE_DLL_DECL __declspec(dllexport)
#else
#define INTEL_CM_CLANGFE_DLL_DECL __declspec(dllimport)
#endif
#else
#define INTEL_CM_CLANGFE_DLL_DECL
#endif

namespace Intel {
namespace CM {
namespace ClangFE {

struct IInputArgs {
  using StrT = std::string;
  template <typename T> struct FileT {
    T Name;
    T Src;
  };
  template <typename T> using SeqT = std::vector<T>;

  virtual const StrT &getSrc() const = 0;
  virtual const SeqT<StrT> &getCompOpts() const = 0;
  virtual const SeqT<FileT<StrT>> &getExtraFiles() const = 0;
  virtual ~IInputArgs(){};
};

struct IOutputArgs {
  using StrT = std::string;
  using BufferT = std::vector<char>;
  // numbers were taken from OpenCL
  enum class ErrT { SUCCESS = 0, COMPILE_PROGRAM_FAILURE = -15 };

  virtual ErrT getStatus() const = 0;
  virtual const BufferT &getIR() const = 0;
  virtual const StrT &getLog() const = 0;
  virtual void discard() = 0;
  virtual ~IOutputArgs(){};
};

struct IDriverInvocation {
  using StrT = std::string;
  using SeqStrT = std::vector<std::string>;

  enum class InputTypeT { SourceCM, LLVM_IR, SPIRV, NONE, OTHER };
  enum class OutputTypeT { S, SPIRV, LLVM_IR, LLVM_BC, PREPROC, OTHER };
  enum class TargetRuntimeT { OCL, L0 };
  enum class BinaryFormatT { DEFAULT, OCL, ZE };

  virtual const SeqStrT& getFEArgs() const = 0;
  virtual const SeqStrT& getBEArgs() const = 0;

  virtual const InputTypeT& getInputType() const = 0;
  virtual const OutputTypeT& getOutputType() const = 0;
  virtual const TargetRuntimeT& getTargetRuntime() const = 0;
  virtual BinaryFormatT getBinaryFormat() const = 0;
  virtual bool getTimePasses() const = 0;

  virtual const StrT& getTargetArch() const = 0;
  virtual const StrT& getInputFilename() const = 0;
  virtual const StrT& getOutputFilename() const = 0;
  virtual const StrT& getTargetFeaturesStr() const = 0;

  virtual bool isHelp() const = 0;

  virtual void discard() = 0;
  virtual ~IDriverInvocation() {};
};

// this number should be increased whenever the public interface changes
static const int InterfaceVersion = 10;

} // namespace ClangFE
} // namespace CM
} // namespace Intel

extern "C" INTEL_CM_CLANGFE_DLL_DECL bool IntelCMClangFEIsShowVersionInvocation(
    Intel::CM::ClangFE::IDriverInvocation const *DriverInvocPtr);

extern "C" INTEL_CM_CLANGFE_DLL_DECL int
IntelCMClangFEGetInterfaceVersion();

extern "C" INTEL_CM_CLANGFE_DLL_DECL int IntelCMClangFEGetRevId(
    Intel::CM::ClangFE::IDriverInvocation const *DriverInvocPtr);

extern "C" INTEL_CM_CLANGFE_DLL_DECL bool IntelCMClangFEGetPrintStats(
    Intel::CM::ClangFE::IDriverInvocation const *DriverInvocPtr);

extern "C" INTEL_CM_CLANGFE_DLL_DECL const char *IntelCMClangFEGetStatsFile(
    Intel::CM::ClangFE::IDriverInvocation const *DriverInvocPtr);

extern "C" INTEL_CM_CLANGFE_DLL_DECL const char *IntelCMClangFEGetVCApiOptions(
    Intel::CM::ClangFE::IDriverInvocation const *DriverInvocPtr);

extern "C" INTEL_CM_CLANGFE_DLL_DECL Intel::CM::ClangFE::IDriverInvocation *
IntelCMClangFEBuildDriverInvocation(int argc, const char * argv[]);

// Invokes clang with source text, options etc. passed in IInputArgs,
// returns result of compilation (status, log, IR, if there is such)
// in IOutputArgs.
//
// This function doesn't take ownership of InputArg, so it is user's
// responsibility to delete both IInputArgs and IOutputArgs.
extern "C" INTEL_CM_CLANGFE_DLL_DECL Intel::CM::ClangFE::IOutputArgs *
IntelCMClangFECompile(const Intel::CM::ClangFE::IInputArgs *InputArg);

#endif // LLVM_FRONTEND_WRAPPER_INTERFACE_H

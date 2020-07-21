#ifndef LLVM_ADAPTOR_CM_INPUT_ARGS_WRAPPER_H
#define LLVM_ADAPTOR_CM_INPUT_ARGS_WRAPPER_H

#include <cassert>

#include "Interface.h"

#include "Frontend.h"

namespace IGC {
namespace AdaptorCM {
namespace Frontend {

// It's a class to hold input arguments for IntelCMClangFECompile
// function. It takes arguments from IGC::AdaptorCM::Frontend::IInputArgs
// and extend them.
// IGC::AdaptorCM::Frontend::IInputArgs must outlive object of this class,
// or we'll get dangling reference.
class InputArgsWrapper final : public Intel::CM::ClangFE::IInputArgs {

public:
  using ErrorType = std::string;

private:
  using FEIInput = IGC::AdaptorCM::Frontend::IInputArgs;

  const StrT &Src;
  SeqT<StrT> CompOpts;
  SeqT<FileT<StrT>> ExtraFiles;

  void processSupportDirectories(const FEIInput &In, ErrorType &Err);

public:

  InputArgsWrapper(const FEIInput &In, ErrorType &Err);

  const StrT &getSrc() const override { return Src; }
  const SeqT<StrT> &getCompOpts() const override { return CompOpts; }
  const SeqT<FileT<StrT>> &getExtraFiles() const override {
    return ExtraFiles;
  }
};

} // namespace Frontend
} // namespace AdaptorCM
} // namespace IGC

#endif // LLVM_ADAPTOR_CM_INPUT_ARGS_WRAPPER_H

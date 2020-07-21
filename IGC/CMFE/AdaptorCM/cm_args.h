#ifndef ADAPTOR_CM_FE_ARGS_H
#define ADAPTOR_CM_FE_ARGS_H

#include "Frontend.h"

namespace IGC {
namespace CM {

struct FeInputArgsImpl final : public IGC::AdaptorCM::Frontend::IInputArgs {

  StrT       InputText;
  SeqT<StrT> CompilationOpts;
  SeqT<StrT> ExtraFiles;
  SeqT<StrT> SupportDirs;

  using OutputFormatT = IGC::AdaptorCM::Frontend::IInputArgs::FormatT;

  OutputFormatT OutputFormat = FormatT::LLVM_IR_BC;

  const StrT &getSrc() const override { return InputText; }
  const SeqT<StrT> &getCompOpts() const override { return CompilationOpts; }
  const SeqT<StrT> &getExtraFiles() const override { return ExtraFiles; }
  const SeqT<StrT> &getSupportDirs() const override { return SupportDirs; }
  OutputFormatT getOutputFormat() const override { return OutputFormat; }

  ~FeInputArgsImpl() {}
};

} // namespace CM
} // namespace IGC

#endif // ADAPTOR_CM_FE_ARGS_H

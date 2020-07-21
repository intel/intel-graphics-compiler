#ifndef LLVM_ADAPTOR_CM_FRONTEND_H
#define LLVM_ADAPTOR_CM_FRONTEND_H

#include "Interface.h"

namespace IGC {
namespace AdaptorCM {
namespace Frontend {

struct IInputArgs {
  using StrT = Intel::CM::ClangFE::IInputArgs::StrT;
  template <typename T> using SeqT = Intel::CM::ClangFE::IInputArgs::SeqT<T>;
  // output formats
  enum class FormatT { LLVM_IR, LLVM_IR_BC, SPIR_V_BC, PREPROC };

  virtual const StrT &getSrc() const = 0;
  virtual const SeqT<StrT> &getCompOpts() const = 0;
  virtual const SeqT<StrT> &getExtraFiles() const = 0;
  virtual const SeqT<StrT> &getSupportDirs() const = 0;

  virtual FormatT getOutputFormat() const = 0;
  virtual ~IInputArgs() {}
};

using StringVect_t = std::vector<std::string>;
StringVect_t convertBackendArgsToVcOpts(const StringVect_t& BackendArgs);

using IOutputArgs = Intel::CM::ClangFE::IOutputArgs;
using IDriverInvocation = Intel::CM::ClangFE::IDriverInvocation;

void validateABICompatibility();

IOutputArgs *translate(const IInputArgs *);

IDriverInvocation *getDriverInvocation(int argc, const char * argv[]);

} // namespace Frontend
} // namespace AdaptorCM
} // namespace IGC

#endif // LLVM_ADAPTOR_CM_FRONTEND_H

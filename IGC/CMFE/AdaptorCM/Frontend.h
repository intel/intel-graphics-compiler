#ifndef LLVM_ADAPTOR_CM_FRONTEND_H
#define LLVM_ADAPTOR_CM_FRONTEND_H

#include "Interface.h"

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
StringVect_t convertBackendArgsToVcOpts(const StringVect_t& BackendArgs);

using IOutputArgs = Intel::CM::ClangFE::IOutputArgs;
using IDriverInvocation = Intel::CM::ClangFE::IDriverInvocation;

struct AbiCompatibilityInfo {
  int RequestedVersion = -1;
  int AvailableVersion = -1;
};
bool validateABICompatibility(AbiCompatibilityInfo *AbiInfo = nullptr);

IOutputArgs *translate(const InputArgs &);

IDriverInvocation *getDriverInvocation(int argc, const char * argv[]);

} // namespace Frontend
} // namespace AdaptorCM
} // namespace IGC

#endif // LLVM_ADAPTOR_CM_FRONTEND_H

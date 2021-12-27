/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Legacy SPIRVDLL-like interface for translation of SPIRV to LLVM IR.
//
//===----------------------------------------------------------------------===//

#include "SPIRVWrapper.h"

#include "vc/Support/Status.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Verifier.h"

#include "LLVMSPIRVLib.h"

#include "Probe/Assertion.h"

#include <sstream>

using namespace llvm;

namespace {

using SpirvReadVerifyType = int(
    const char *pIn, size_t InSz, const uint32_t *SpecConstIds,
    const uint64_t *SpecConstVals, unsigned SpecConstSz,
    void (*OutSaver)(const char *pOut, size_t OutSize, void *OutUserData),
    void *OutUserData, void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
    void *ErrUserData);

int spirvReadVerify(const char *pIn, size_t InSz, const uint32_t *SpecConstIds,
                    const uint64_t *SpecConstVals, unsigned SpecConstSz,
                    void (*OutSaver)(const char *pOut, size_t OutSize,
                                     void *OutUserData),
                    void *OutUserData,
                    void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
                    void *ErrUserData) {
  llvm::LLVMContext Context;
  llvm::StringRef SpirvInput = llvm::StringRef(pIn, InSz);
  std::istringstream IS(SpirvInput.str());
  std::unique_ptr<llvm::Module> M;
  {
    llvm::Module *SpirM;
    std::string ErrMsg;
    SPIRV::TranslatorOpts Opts;
    Opts.enableAllExtensions();
    Opts.setFPContractMode(SPIRV::FPContractMode::On);
    Opts.setDesiredBIsRepresentation(SPIRV::BIsRepresentation::SPIRVFriendlyIR);
    // Add specialization constants
    for (unsigned i = 0; i < SpecConstSz; ++i)
      Opts.setSpecConst(SpecConstIds[i], SpecConstVals[i]);

    // This returns true on success...
    bool Status = llvm::readSpirv(Context, Opts, IS, SpirM, ErrMsg);
    if (!Status) {
      std::ostringstream OSS;
      OSS << "spirv_read_verify: readSpirv failed: " << ErrMsg;
      ErrSaver(OSS.str().c_str(), ErrUserData);
      return -1;
    }
    Status = llvm::verifyModule(*SpirM);
    if (Status) {
      ErrSaver("spirv_read_verify: verify Module failed", ErrUserData);
      return -1;
    }
    M.reset(SpirM);
  }

  llvm::SmallVector<char, 16> CloneBuffer;
  llvm::raw_svector_ostream CloneOstream(CloneBuffer);
  WriteBitcodeToFile(*M, CloneOstream);
  IGC_ASSERT(CloneBuffer.size() > 0);

  OutSaver(CloneBuffer.data(), CloneBuffer.size(), OutUserData);
  return 0;
}

Expected<SpirvReadVerifyType *> getSpirvReadVerifyFunction() {
  return &spirvReadVerify;
}

} // namespace

Expected<std::vector<char>>
vc::translateSPIRVToIR(ArrayRef<char> Input, ArrayRef<uint32_t> SpecConstIds,
                       ArrayRef<uint64_t> SpecConstValues) {
  IGC_ASSERT(SpecConstIds.size() == SpecConstValues.size());
  auto OutSaver = [](const char *pOut, size_t OutSize, void *OutData) {
    auto *Vec = reinterpret_cast<std::vector<char> *>(OutData);
    Vec->assign(pOut, pOut + OutSize);
  };
  auto ErrSaver = [](const char *pErrMsg, void *ErrData) {
    auto *ErrStr = reinterpret_cast<std::string *>(ErrData);
    *ErrStr = pErrMsg;
  };
  std::string ErrMsg;
  std::vector<char> Result;
  auto SpirvReadVerifyFunctionExp = getSpirvReadVerifyFunction();
  if (!SpirvReadVerifyFunctionExp)
    return SpirvReadVerifyFunctionExp.takeError();
  auto *SpirvReadVerifyFunction = SpirvReadVerifyFunctionExp.get();

  int Status = SpirvReadVerifyFunction(
      Input.data(), Input.size(), SpecConstIds.data(), SpecConstValues.data(),
      SpecConstValues.size(), OutSaver, &Result, ErrSaver, &ErrMsg);

  if (Status != 0)
    return make_error<vc::BadSpirvError>(ErrMsg);
  return {std::move(Result)};
}

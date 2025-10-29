/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Legacy SPIRVDLL-like interface for translation of SPIRV to LLVM IR.
//
//===----------------------------------------------------------------------===//

#include "SPIRVWrapper.h"

#include "vc/Support/Status.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Verifier.h"
#include <llvm/IR/IRBuilder.h>
#include "LLVMSPIRVLib.h"

#include "Probe/Assertion.h"

#include <sstream>

using namespace llvm;

namespace {

using SpirvReadVerifyType = int(
    const char *pIn, size_t InSz, const uint32_t *SpecConstIds,
    const uint64_t *SpecConstVals, unsigned SpecConstSz, LLVMContext &Context,
    void (*OutSaver)(const char *pOut, size_t OutSize, void *OutUserData),
    void *OutUserData, void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
    void *ErrUserData);

void PrepareModuleStructs(Module &M) {
  // Delete return type structs from function definitions
  SmallVector<Instruction *, 16> ToRemoveExtract;
  SmallVector<Instruction *, 4> ToRemoveGotoJoin;
  SmallVector<Function *, 4> ToRemoveFunc;

  for (auto &F : M.functions()) {
    if (!F.isIntrinsic())
      continue;
    auto IID = vc::getAnyIntrinsicID(&F);
    // It is necessary to correct all returned structures
    if (IID != GenXIntrinsic::genx_simdcf_goto &&
        IID != GenXIntrinsic::genx_simdcf_join &&
        IID != GenXIntrinsic::genx_addc && IID != GenXIntrinsic::genx_simad &&
        IID != GenXIntrinsic::genx_uimad)
      continue;
    auto *ST = cast<StructType>(F.getReturnType());
    if (!ST->isLiteral() || ST->isPacked()) {
      ToRemoveFunc.push_back(&F);
      // Replace return type with literal non-packed struct.
      auto *FT = F.getFunctionType();
      auto *NewST = StructType::get(ST->getContext(), ST->elements());
      auto *NewFT = FunctionType::get(NewST, FT->params(), FT->isVarArg());
      /// We expect *.goto.*.* name, not *.goto.*.*.[123...]
      std::string Name = F.getName().str();
      F.setName(F.getName() + ".old");
      auto *NewFn = Function::Create(NewFT, F.getLinkage(), F.getAddressSpace(),
                                     Name, F.getParent());
      for (auto &U : F.uses()) {
        auto *Call = cast<CallInst>(U.getUser());
        // Replace current instructions operands and uses
        llvm::IRBuilder<> IRB(Call);
        ToRemoveGotoJoin.push_back(Call);
        // 1. Get operands from call
        // 2. Generate new `calls` with operands
        SmallVector<Value *, 8> Args(Call->args());
        CallInst *NewCall = IRB.CreateCall(NewFn, Args, Call->getName());
        if (Call->isConvergent())
          NewCall->setConvergent();
        // Check all uses of call is extract
        // 3. Generate `extracts` for each exist extract
        for (auto &UC : Call->uses()) {
          auto *Extract = cast<ExtractValueInst>(UC.getUser());
          llvm::IRBuilder<> Builder(Extract);
          SmallVector<unsigned, 8> Idxs(Extract->indices());
          auto *NewExtract =
              Builder.CreateExtractValue(NewCall, Idxs, Extract->getName());
          // 4. Replace uses for extracts
          Extract->replaceAllUsesWith(NewExtract);
          ToRemoveExtract.push_back(Extract);
        }
      }
    }
  }
  // 5. Remove extracts and calls
  for (auto *Vals : ToRemoveExtract)
    Vals->eraseFromParent();
  for (auto *Vals : ToRemoveGotoJoin)
    Vals->eraseFromParent();
  for (auto *Func : ToRemoveFunc)
    Func->eraseFromParent();
}

void fixAlignmentAttributes(Module &M) {
  for (auto &F : M.functions()) {
    for (auto &Arg : F.args()) {
      // Remove align attribute if the argument type is not a pointer
      if (!Arg.getType()->isPointerTy())
        F.removeParamAttr(Arg.getArgNo(), Attribute::Alignment);
    }
  }
}

int spirvReadVerify(
    const char *pIn, size_t InSz, const uint32_t *SpecConstIds,
    const uint64_t *SpecConstVals, unsigned SpecConstSz, LLVMContext &Context,
    void (*OutSaver)(const char *pOut, size_t OutSize, void *OutUserData),
    void *OutUserData, void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
    void *ErrUserData) {
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
    Opts.setEmitFunctionPtrAddrSpace(true);
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
    PrepareModuleStructs(*SpirM);
    fixAlignmentAttributes(*SpirM);
    // Bool-value need to separate functionality and debug errors
    bool BrokenDebugInfo = false;
    Status = llvm::verifyModule(*SpirM, nullptr, &BrokenDebugInfo);
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
                       ArrayRef<uint64_t> SpecConstValues, LLVMContext &Ctx) {
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
      SpecConstValues.size(), Ctx, OutSaver, &Result, ErrSaver, &ErrMsg);

  if (Status != 0)
    return make_error<vc::BadSpirvError>(ErrMsg);
  return {std::move(Result)};
}

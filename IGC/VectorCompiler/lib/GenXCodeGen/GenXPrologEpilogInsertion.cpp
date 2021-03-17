/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

///
/// GenXPrologEpilogInsertion
/// ---------------------
/// This pass generates IR for stack ABI we need to comply with on the IGC side
///
/// To make vISA emit accesses to its predefined registers we generate
/// genx.read/write.predef.reg intrinsics baled into rd/wrregions.
/// VISA regalloc makes sure that dests & sources of the generated instructions
/// are allocated to the proper predefined VISA regs
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/Support/BackendConfig.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <unordered_set>

using namespace llvm;
using namespace genx;

namespace {

inline unsigned calcPadding(unsigned Val, unsigned Align) {
  IGC_ASSERT(Align);
  if (Val % Align == 0)
    return 0;
  return Align - Val % Align;
}

class GenXPrologEpilogInsertion
    : public FunctionPass,
      public InstVisitor<GenXPrologEpilogInsertion> {

  const DataLayout *DL = nullptr;
  const GenXSubtarget *ST = nullptr;
  const GenXBackendConfig *BEConf = nullptr;

  std::unordered_set<Function *> HasStackCalls;
  std::unordered_map<Function *, unsigned> PrivMemSize;

  unsigned ArgRegSize;
  unsigned RetRegSize;

  bool ForceArgMem = false;
  bool ForceRetMem = false;

  void generateKernelProlog(Function &F);
  void generateFunctionProlog(Function &F);
  void generateFunctionEpilog(Function &F, ReturnInst &I);
  void generateStackCall(CallInst *CI);
  void generateAlloca(CallInst *CI);

  Value *push(Value *V, IRBuilder<> &IRB, Value *InitSP);
  std::pair<Instruction *, Value*> pop(Type *T, IRBuilder<> &IRB, Value *InitSP);

  // helper functions
  Instruction *buildReadPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                  Type *AffectingType,
                                  bool BuildTempVal = false,
                                  bool AllowScalar = true, unsigned Offset = 0,
                                  unsigned Width = 0);
  Instruction *buildReadPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                  Value *AffectingValue,
                                  bool BuildTempVal = false,
                                  bool AllowScalar = true, unsigned Offset = 0,
                                  unsigned Width = 0);
  Instruction *buildReadPredefRegNoRegion(PreDefined_Vars RegID,
                                          IRBuilder<> &IRB,
                                          Type *AffectingType);
  Instruction *buildReadPredefRegNoRegion(PreDefined_Vars RegID,
                                          IRBuilder<> &IRB,
                                          Value *AffectingValue);

  Instruction *buildWritePredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                   Value *Input);
  Instruction *buildWritePredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                   Value *Input, Value *Dep,
                                   Instruction *InsPoint, unsigned Offset = 0);

public:
  static char ID;
  explicit GenXPrologEpilogInsertion();
  StringRef getPassName() const override {
    return "GenX prolog & epilog insertion";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
  void visitCallInst(CallInst &I);
  void visitReturnInst(ReturnInst &I);
};

} // namespace

char GenXPrologEpilogInsertion::ID = 0;
namespace llvm {
void initializeGenXPrologEpilogInsertionPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXPrologEpilogInsertion, "GenXPrologEpilogInsertion",
                      "GenXPrologEpilogInsertion", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPrologEpilogInsertion, "GenXPrologEpilogInsertion",
                    "GenXPrologEpilogInsertion", false, false)

GenXPrologEpilogInsertion::GenXPrologEpilogInsertion() : FunctionPass(ID) {
  initializeGenXPrologEpilogInsertionPass(*PassRegistry::getPassRegistry());
}

FunctionPass *llvm::createGenXPrologEpilogInsertionPass() {
  return new GenXPrologEpilogInsertion();
}

void GenXPrologEpilogInsertion::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
}

bool GenXPrologEpilogInsertion::runOnFunction(Function &F) {
  BEConf = &getAnalysis<GenXBackendConfig>();
  DL = &F.getParent()->getDataLayout();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  ArgRegSize = visa::ArgRegSizeInGRFs * ST->getGRFWidth();
  RetRegSize = visa::RetRegSizeInGRFs * ST->getGRFWidth();
  if (!(BEConf->useNewStackBuilder() && ST->isOCLRuntime()))
    return false;
  if (!F.getParent()->getModuleFlag(ModuleMD::UseSVMStack))
    return false;
  visit(F);
  if (isKernel(&F)) {
    generateKernelProlog(F);
    // no epilog is required for kernels
  } else if (F.hasFnAttribute(genx::FunctionMD::CMStackCall)) {
    generateFunctionProlog(F);
    // function epilog is generated when RetInst is met
  }

  return true;
}

void GenXPrologEpilogInsertion::visitCallInst(CallInst &I) {
  if (I.isInlineAsm())
    return;
  if (IGCLLVM::isIndirectCall(I) ||
      I.getCalledFunction()->hasFnAttribute(genx::FunctionMD::CMStackCall)) {
    HasStackCalls.insert(I.getFunction());
    generateStackCall(&I);
  } else if (GenXIntrinsic::getGenXIntrinsicID(I.getCalledFunction()) ==
             GenXIntrinsic::genx_alloca)
    generateAlloca(&I);
}

void GenXPrologEpilogInsertion::visitReturnInst(ReturnInst &I) {
  if (I.getFunction()->hasFnAttribute(genx::FunctionMD::CMStackCall))
    generateFunctionEpilog(*I.getFunction(), I);
}

// FE_SP = PrivateBase + HWTID * StackPerThread
void GenXPrologEpilogInsertion::generateKernelProlog(Function &F) {
  IRBuilder<> IRB(&F.getEntryBlock().front());
  Function *HWID = GenXIntrinsic::getGenXDeclaration(
      F.getParent(), llvm::GenXIntrinsic::genx_get_hwid, {});
  auto *HWIDCall = IRB.CreateCall(HWID);
  auto *ThreadOffset = IRB.getInt32(visa::StackPerThreadSVM);
  auto *Mul = IRB.CreateMul(HWIDCall, ThreadOffset);
  auto *MulCasted = IRB.CreateZExt(Mul, IRB.getInt64Ty());

  KernelMetadata KM(&F);
  auto *PrivBase =
      std::find_if(F.arg_begin(), F.arg_end(), [&KM](Argument &Arg) {
        return KernelArgInfo(KM.getArgKind(Arg.getArgNo())).isPrivateBase();
      });
  IGC_ASSERT_EXIT_MESSAGE(PrivBase != F.arg_end(), "No PrivBase arg found");

  auto *Add = IRB.CreateAdd(PrivBase, MulCasted);
  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, Add);
}

// foreach arg:
//  if fits in %ARG:
//    %arg_reg = genx.read.predef.reg %ARG
//    %arg = rdregion from %arg_reg
//  else:
//    read from stackmem
void GenXPrologEpilogInsertion::generateFunctionProlog(Function &F) {
  IRBuilder<> IRB(&F.getEntryBlock().front());
  unsigned Offset = 0;
  Value *Sp = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                 IRB.getInt64Ty(), true);
  for (auto &Arg : F.args()) {
    if (Arg.use_empty())
      continue;
    auto *ArgScalarType = Arg.getType()->getScalarType();
    auto NumElements = 1;
    bool AllowScalar = true;
    if (auto *VT = dyn_cast<VectorType>(Arg.getType()))
      NumElements = VT->getNumElements();
    if (ArgScalarType->isIntegerTy(1)) {
      NumElements = 1;
      ArgScalarType = IRB.getInt32Ty();
      AllowScalar = false;
    }
    Offset += calcPadding(Offset, ST->getGRFWidth());
    unsigned ArgSize = DL->getTypeSizeInBits(Arg.getType()) / genx::ByteBits;
    if (ForceArgMem || Offset + ArgSize > ArgRegSize) {
      auto ReadVal = pop(Arg.getType(), IRB, Sp);
      Sp = ReadVal.second;
      Arg.replaceAllUsesWith(ReadVal.first);
    } else {
      auto *ArgRegType = VectorType::get(
          ArgScalarType,
          ArgRegSize / (DL->getTypeSizeInBits(ArgScalarType) / genx::ByteBits));
      auto *ArgRead = buildReadPredefReg(PreDefined_Vars::PREDEFINED_ARG, IRB,
                                         ArgRegType, HasStackCalls.count(&F),
                                         AllowScalar, Offset, NumElements);
      if (Arg.getType()->getScalarType()->isIntegerTy(1)) {
        IGC_ASSERT(isa<VectorType>(Arg.getType()));
        ArgRead = cast<Instruction>(IRB.CreateTruncOrBitCast(
            ArgRead, VectorType::get(
                         IRB.getIntNTy(
                             cast<VectorType>(Arg.getType())->getNumElements()),
                         1)));
        ArgRead = cast<Instruction>(IRB.CreateBitCast(ArgRead, Arg.getType()));
      }
      Offset += ArgSize;
      Arg.replaceAllUsesWith(ArgRead);
    }
  }
  F.setMetadata(
      InstMD::FuncArgSize,
      MDNode::get(F.getContext(),
                  ConstantAsMetadata::get(IRB.getInt32(
                      (Offset + ST->getGRFWidth() - 1) / ST->getGRFWidth()))));
}

//  if fits in %ARG:
//    %ret = wrregion (%retval)
//    genx.write.predef.reg %RET %ret
//  else:
//    write to stackmem
void GenXPrologEpilogInsertion::generateFunctionEpilog(Function &F,
                                                       ReturnInst &I) {
  IRBuilder<> IRB(&I);
  unsigned RetSize = 0;
  if (!F.getReturnType()->isVoidTy()) {
    auto *RetVal = I.getReturnValue();
    if (RetVal->getType()->getScalarType()->isIntegerTy(1))
      return;
    if (auto *StructTy = dyn_cast<StructType>(RetVal->getType())) {
      IGC_ASSERT_EXIT_MESSAGE(StructTy->getNumElements() == 1,
                              "No structures passing supported yet");
      auto *InsValue = cast<InsertValueInst>(RetVal);
      RetVal = InsValue->getOperand(1);
    }
    RetSize = DL->getTypeSizeInBits(RetVal->getType());
    Value *RetBase = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                        IRB.getInt64Ty());
    // restore SP
    if (PrivMemSize[&F]) {
      auto *RestoredSP =
          IRB.CreateSub(RetBase, IRB.getInt64(PrivMemSize[&F]), "");
      buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, RestoredSP);
    }
    if (ForceRetMem ||
        DL->getTypeSizeInBits(RetVal->getType()) / genx::ByteBits >
            RetRegSize) {
      RetBase = push(RetVal, IRB, RetBase);
    } else {
      auto *RetRegType = VectorType::get(
          RetVal->getType()->getScalarType(),
          RetRegSize /
              (DL->getTypeSizeInBits(RetVal->getType()->getScalarType()) /
               genx::ByteBits));
      auto *RetRegWrite =
          buildWritePredefReg(PreDefined_Vars::PREDEFINED_RET, IRB, RetVal,
                              UndefValue::get(RetRegType), &I);
      I.replaceUsesOfWith(RetVal, RetRegWrite);
    }
  }
  F.setMetadata(InstMD::FuncRetSize,
                MDNode::get(F.getContext(),
                            ConstantAsMetadata::get(
                                IRB.getInt32(RetSize / ST->getGRFWidth()))));
}

void GenXPrologEpilogInsertion::generateStackCall(CallInst *CI) {
  IRBuilder<> IRB(CI);
  unsigned Offset = 0;
  Value *OrigSp = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                     IRB.getInt64Ty(), true);
  auto *SpArgs = OrigSp;
  // write args
  for (auto &Arg : CI->arg_operands()) {
    if (Arg->getType()->getScalarType()->isIntegerTy(1)) {
      IGC_ASSERT(isa<VectorType>(Arg->getType()));
      Arg = IRB.CreateBitOrPointerCast(
          Arg,
          VectorType::get(
              IRB.getIntNTy(cast<VectorType>(Arg->getType())->getNumElements()),
              1));
    }
    Offset += calcPadding(Offset, ST->getGRFWidth());
    auto ArgSize = DL->getTypeSizeInBits(Arg->getType()) / genx::ByteBits;
    auto *ArgRegType = VectorType::get(
        Arg->getType()->getScalarType(),
        ArgRegSize / (DL->getTypeSizeInBits(Arg->getType()->getScalarType()) /
                      genx::ByteBits));
    if (ForceArgMem || Offset + ArgSize > ArgRegSize) {
      // to stack
      SpArgs = push(Arg, IRB, SpArgs);
    } else {
      // to %ARG
      auto *FakeArg = buildReadPredefRegNoRegion(
          PreDefined_Vars::PREDEFINED_ARG, IRB, ArgRegType);
      auto *ArgRegWrite = buildWritePredefReg(PreDefined_Vars::PREDEFINED_ARG,
                                              IRB, Arg, FakeArg, CI, Offset);
      CI->replaceUsesOfWith(Arg, ArgRegWrite);
      Offset += ArgSize;
    }
  }

  CI->setMetadata(
      InstMD::FuncArgSize,
      MDNode::get(CI->getContext(),
                  ConstantAsMetadata::get(IRB.getInt32(
                      (Offset + ST->getGRFWidth() - 1) / ST->getGRFWidth()))));
  bool isVoidCall = CI->getType()->isVoidTy();
  CI->setMetadata(
      InstMD::FuncRetSize,
      MDNode::get(CI->getContext(),
                  ConstantAsMetadata::get(IRB.getInt32(
                      (isVoidCall ? 0
                                  : (DL->getTypeSizeInBits(CI->getType())) /
                                        genx::ByteBits) /
                      ST->getGRFWidth()))));
  if (isVoidCall)
    return;
  // read retvalue
  Instruction *ActualRet = CI;
  if (auto *StructTy = dyn_cast<StructType>(ActualRet->getType())) {
    IGC_ASSERT_EXIT_MESSAGE(StructTy->getNumElements() == 1,
                            "No structures passing supported yet");
    IGC_ASSERT(ActualRet->getNumUses() == 1 &&
               isa<ExtractValueInst>(ActualRet->user_back()));
    ActualRet = cast<ExtractValueInst>(ActualRet->user_back());
  }
  IRB.SetInsertPoint(ActualRet->getNextNode());
  if (ForceRetMem ||
      DL->getTypeSizeInBits(ActualRet->getType()) / genx::ByteBits >
          RetRegSize) {
    // from stack
    Region R(OrigSp->getType());
    Value *SpRet =
        R.createRdRegion(cast<Instruction>(OrigSp)->getOperand(0), "",
                         &*IRB.GetInsertPoint(), DebugLoc(), true);
    auto ReadVal = pop(ActualRet->getType(), IRB, SpRet);
    SpRet = ReadVal.second;
    ActualRet->replaceAllUsesWith(ReadVal.first);
  } else {
    // from %RET arg
    auto *RetRead =
        buildReadPredefReg(PreDefined_Vars::PREDEFINED_RET, IRB, ActualRet);
    ActualRet->replaceAllUsesWith(RetRead);
    cast<Instruction>(RetRead->getOperand(0))->replaceUsesOfWith(RetRead, ActualRet);
  }
}

// alloca_base = FE_SP
// FE_SP += sizeof(alloca)
void GenXPrologEpilogInsertion::generateAlloca(CallInst *CI) {
  IRBuilder<> IRB(CI);

  auto *AllocaBase = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                        CI->getType(), true);

  auto *AllocaOff = CI->getOperand(0);
  auto *AllocaOffTy = AllocaOff->getType();

  unsigned AllocaOffset = DL->getTypeSizeInBits(AllocaOffTy) / genx::ByteBits;

  // padd the current alloca the comply with gather/scatter alignment rules
  auto *AllocaEltTy = AllocaOffTy->getScalarType();
  if (AllocaOffTy->isArrayTy())
    AllocaEltTy = AllocaOffTy->getArrayElementType();
  AllocaOffset += calcPadding(AllocaOffset, visa::BytesPerSVMPtr);

  auto *Add =
      IRB.CreateAdd(AllocaBase, ConstantInt::get(CI->getType(), AllocaOffset));
  IGC_ASSERT(AllocaOffset % visa::BytesPerSVMPtr == 0);
  PrivMemSize[CI->getFunction()] += AllocaOffset;

  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, Add);
  CI->replaceAllUsesWith(AllocaBase);
  CI->eraseFromParent();
}

Value *GenXPrologEpilogInsertion::push(Value *V, IRBuilder<> &IRB, Value *InitSP) {
  if (!isa<VectorType>(V->getType()))
    V = IRB.CreateBitCast(V, VectorType::get(V->getType()->getScalarType(), 1));
  if (V->getType()->getScalarType()->isIntegerTy(1)) {
    IGC_ASSERT(isa<VectorType>(V->getType()));
    V = IRB.CreateBitOrPointerCast(
        V, VectorType::get(IRB.getInt8Ty(),
                           cast<VectorType>(V->getType())->getNumElements() /
                               genx::ByteBits));
  }
  unsigned BytesLeft = DL->getTypeSizeInBits(V->getType()) / genx::ByteBits;
  unsigned Offset = 0;
  auto *SP = InitSP;
  auto Copy = [&](unsigned Width) {
    unsigned ByteSize = Width * visa::BytesPerOword;
    if (ByteSize > BytesLeft && BytesLeft && Width == 1)
      ByteSize = BytesLeft;
    while (BytesLeft >= ByteSize) {
      auto *CurReadType = VectorType::get(
          V->getType()->getScalarType(),
          ByteSize / (DL->getTypeSizeInBits(V->getType()->getScalarType()) /
                      genx::ByteBits));
      Region R(CurReadType);
      R.Offset = Offset;
      auto *Read = R.createRdRegion(V, "", &*IRB.GetInsertPoint(), DebugLoc());

      auto *Fn = llvm::GenXIntrinsic::getGenXDeclaration(
          IRB.GetInsertPoint()->getModule(),
          llvm::GenXIntrinsic::genx_svm_block_st,
          {SP->getType(), Read->getType()});

      IRB.CreateCall(Fn, {SP, Read});
      Offset += ByteSize;
      BytesLeft -= ByteSize;
      SP = IRB.CreateAdd(SP, IRB.getInt64(ByteSize));
      Offset += calcPadding(Offset, visa::BytesPerOword);
    }
  };
  Copy(8);
  Copy(4);
  Copy(2);
  Copy(1);
  return SP;
}

std::pair<Instruction *, Value *>
GenXPrologEpilogInsertion::pop(Type *Ty, IRBuilder<> &IRB, Value *InitSP) {
  auto *OrigTy = Ty;
  if (!isa<VectorType>(Ty))
    Ty = VectorType::get(Ty, 1);
  unsigned BytesLeft = DL->getTypeSizeInBits(Ty) / genx::ByteBits;
  unsigned Offset = 0;
  if (Ty->getScalarType()->isIntegerTy(1)) {
    IGC_ASSERT(isa<VectorType>(Ty));
    Ty = VectorType::get(IRB.getInt8Ty(),
                         cast<VectorType>(Ty)->getNumElements() /
                             genx::ByteBits);
  }
  auto *SP = InitSP;
  Value *RetVal = UndefValue::get(Ty);
  auto Copy = [&](unsigned Width) {
    unsigned ByteSize = Width * visa::BytesPerOword;
    if (ByteSize > BytesLeft && BytesLeft && Width == 1)
      ByteSize = BytesLeft;
    while (BytesLeft >= ByteSize) {
      auto *CurLdType = VectorType::get(
          Ty->getScalarType(),
          ByteSize /
              (Ty->getScalarType()->getPrimitiveSizeInBits() / genx::ByteBits));
      auto *Fn = llvm::GenXIntrinsic::getGenXDeclaration(
          IRB.GetInsertPoint()->getModule(),
          llvm::GenXIntrinsic::genx_svm_block_ld, {CurLdType, SP->getType()});

      auto *Ld = IRB.CreateCall(Fn, {SP});
      Region R(Ld->getType());
      R.Offset = Offset;
      RetVal =
          R.createWrRegion(RetVal, Ld, "", &*IRB.GetInsertPoint(), DebugLoc());
      Offset += ByteSize;
      BytesLeft -= ByteSize;
      SP = IRB.CreateAdd(SP, IRB.getInt64(ByteSize));
      Offset += calcPadding(Offset, visa::BytesPerOword);
    }
  };
  Copy(8);
  Copy(4);
  Copy(2);
  Copy(1);
  if (!isa<VectorType>(OrigTy)) {
    Region R(Ty);
    RetVal =
        R.createRdRegion(RetVal, "", &*IRB.GetInsertPoint(), DebugLoc(), true);
  }
  if (OrigTy->getScalarType()->isIntegerTy(1))
    RetVal = IRB.CreateBitCast(RetVal, OrigTy);
  return {cast<Instruction>(RetVal), SP};
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *AffectingType,
    bool BuildTempVal, bool AllowScalar, unsigned Offset, unsigned Width) {
  return buildReadPredefReg(RegID, IRB, UndefValue::get(AffectingType),
                            BuildTempVal, AllowScalar, Offset, Width);
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Value *AffectingValue,
    bool BuildTempVal, bool AllowScalar, unsigned Offset, unsigned Width) {
  auto *RegRead = buildReadPredefRegNoRegion(RegID, IRB, AffectingValue);

  auto *Result = RegRead;
  if (BuildTempVal) {
    Region RWrite(RegRead);
    Result = RWrite.createWrRegion(UndefValue::get(RegRead->getType()), RegRead,
                                   "", &*IRB.GetInsertPoint(), DebugLoc());
  }
  Region RRead(RegRead);
  RRead.Offset = Offset;
  if (Width) {
    RRead.NumElements = Width;
    RRead.Width = Width;
  }
  Result = RRead.createRdRegion(Result, "", &*IRB.GetInsertPoint(), DebugLoc(),
                                AllowScalar);
  return Result;
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefRegNoRegion(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *AffectingType) {
  return buildReadPredefRegNoRegion(RegID, IRB, UndefValue::get(AffectingType));
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefRegNoRegion(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Value *AffectingValue) {
  auto *AffectingType = AffectingValue->getType();
  Function *RegReadIntr = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertPoint()->getModule(),
      llvm::GenXIntrinsic::genx_read_predef_reg,
      {(isa<VectorType>(AffectingType) ? AffectingType
                                       : VectorType::get(AffectingType, 1)),
       AffectingType});
  auto *RegRead =
      IRB.CreateCall(RegReadIntr, {IRB.getInt32(RegID), AffectingValue});
  return RegRead;
}

Instruction *
GenXPrologEpilogInsertion::buildWritePredefReg(PreDefined_Vars RegID,
                                               IRBuilder<> &IRB, Value *Input) {
  return buildWritePredefReg(RegID, IRB, Input,
                             UndefValue::get(Input->getType()),
                             cast<Instruction>(Input)->getNextNode());
}

Instruction *GenXPrologEpilogInsertion::buildWritePredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Value *Input, Value *Dep,
    Instruction *InsPoint, unsigned Offset) {
  Region RWrite(Input);
  RWrite.Offset = Offset;
  auto *Wrr = RWrite.createWrRegion(Dep, Input, "", InsPoint, DebugLoc());
  Function *RegWriteIntr = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertPoint()->getModule(),
      llvm::GenXIntrinsic::genx_write_predef_reg,
      {Input->getType(), Wrr->getType()});
  auto *RegWrite = IRB.CreateCall(RegWriteIntr, {IRB.getInt32(RegID), Wrr});
  return RegWrite;
}

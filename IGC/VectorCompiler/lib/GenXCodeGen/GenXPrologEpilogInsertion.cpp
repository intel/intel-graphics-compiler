/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

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
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/MathExtras.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <unordered_set>

using namespace llvm;
using namespace genx;

static cl::opt<bool>
    ForceArgMemPassing("vc-stack-force-arg-mem",
                       cl::desc("Pass all stackcall args via stackmem"),
                       cl::init(false));
static cl::opt<bool>
    ForceRetMemPassing("vc-stack-force-ret-mem",
                       cl::desc("Pass all stackcall retval via stackmem"),
                       cl::init(false));
static cl::opt<bool> HandleMaskArgs("vc-stack-handle-mask-args",
                                    cl::desc("Pass i1 arguments of stackcalls"),
                                    cl::init(true));

namespace {

inline unsigned calcPadding(unsigned Val, unsigned Align) {
  IGC_ASSERT(Align);
  if (Val % Align == 0)
    return 0;
  return Align - Val % Align;
}

// Diagnostic information for error/warning relating prolog/epilog insertion.
class DiagnosticInfoPrologEpilogInsertion : public DiagnosticInfo {
private:
  std::string Description;

public:
  // Initialize from description
  DiagnosticInfoPrologEpilogInsertion(const Twine &Desc,
                                      DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(llvm::getNextAvailablePluginDiagnosticKind(), Severity),
        Description(Desc.str()) {}

  void print(DiagnosticPrinter &DP) const override {
    DP << "GenXPrologEpilogInsertion: " << Description;
  }
};

class GenXPrologEpilogInsertion
    : public FunctionPass,
      public InstVisitor<GenXPrologEpilogInsertion> {

  const DataLayout *DL = nullptr;
  const GenXSubtarget *ST = nullptr;
  const GenXBackendConfig *BEConf = nullptr;

  unsigned NumCalls = 0;
  unsigned PrivMemSize = 0;

  unsigned ArgRegSize = 0;
  unsigned RetRegSize = 0;

  bool UseGlobalMem = true;

  void generateKernelProlog(Function &F);
  void generateFunctionProlog(Function &F);
  void generateFunctionEpilog(Function &F, ReturnInst &I);
  void generateStackCall(CallInst *CI);
  void generateAlloca(CallInst *CI);

  Value *push(Value *V, IRBuilder<> &IRB, Value *InitSP);
  std::pair<Instruction *, Value*> pop(Type *T, IRBuilder<> &IRB, Value *InitSP);

  // *** Helper functions ***
  // All params should be easy to understand
  // except for BuildTempVal. It is necessary to generate
  // a temp copy of predef reg when we have >1 calls within a function,
  // which copes with code motion done by baling (or other passes).
  // Temp value is also required when instructions operating by predef
  // regs directly have >1 uses, but it's easier to handle that in baling.
  Instruction *buildReadPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                  Type *Ty,
                                  bool BuildTempVal = false,
                                  bool AllowScalar = true, unsigned Offset = 0,
                                  unsigned Width = 0);
  Instruction *buildReadPredefReg(PreDefined_Vars RegID, IRBuilder<> &IRB,
                                  Type *Ty,
                                  Value *Dep,
                                  bool BuildTempVal = false,
                                  bool AllowScalar = true, unsigned Offset = 0,
                                  unsigned Width = 0);
  Instruction *buildReadPredefRegNoRegion(PreDefined_Vars RegID,
                                          IRBuilder<> &IRB,
                                          Type *Ty);
  Instruction *buildReadPredefRegNoRegion(PreDefined_Vars RegID,
                                          IRBuilder<> &IRB,
                                          Type *Ty,
                                          Value *Dep);

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

class CallsCalculator : public InstVisitor<CallsCalculator> {
  unsigned NumCalls = 0;

public:
  void visitCallInst(CallInst &CI) {
    if (CI.isInlineAsm())
        return;
    if (IGCLLVM::isIndirectCall(CI) ||
        !GenXIntrinsic::isAnyNonTrivialIntrinsic(CI.getCalledFunction()))
      NumCalls++;
  }

  unsigned getNumCalls(Function &F) {
    NumCalls = 0;
    visit(F);
    return NumCalls;
  }
};

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
  NumCalls = CallsCalculator().getNumCalls(F);
  UseGlobalMem =
      F.getParent()->getModuleFlag(ModuleMD::UseSVMStack) != nullptr;
  visit(F);
  if (genx::isKernel(&F)) {
    generateKernelProlog(F);
    // no epilog is required for kernels
  } else if (genx::requiresStackCall(&F)) {
    generateFunctionProlog(F);
    // function epilog is generated when RetInst is met
  }

  return true;
}

void GenXPrologEpilogInsertion::visitCallInst(CallInst &I) {
  if (I.isInlineAsm())
    return;
  bool IsIndirectCall = IGCLLVM::isIndirectCall(I);
  bool IsStackCall =
      IsIndirectCall || genx::requiresStackCall(I.getCalledFunction());
  if (IsStackCall)
    generateStackCall(&I);
  if (!IsIndirectCall) {
    auto IID = GenXIntrinsic::getAnyIntrinsicID(I.getCalledFunction());
    if (IID == GenXIntrinsic::genx_alloca)
      generateAlloca(&I);
    // TODO: conformance fails when we pass i1 args in presence of SIMDCF. Funny
    // thing is that ISPC doesn't use goto/join in its recursion tests so
    // they're fine (i.e. they're not affected by this option) unlike CM
    else if (IID == GenXIntrinsic::genx_simdcf_goto)
      HandleMaskArgs = false;
  }
}

void GenXPrologEpilogInsertion::visitReturnInst(ReturnInst &I) {
  if (genx::requiresStackCall(I.getFunction()))
    generateFunctionEpilog(*I.getFunction(), I);
}

// FE_SP = PrivateBase + HWTID * PrivMemPerThread
void GenXPrologEpilogInsertion::generateKernelProlog(Function &F) {
  IRBuilder<> IRB(&F.getEntryBlock().front());
  Function *HWID = GenXIntrinsic::getGenXDeclaration(
      F.getParent(), llvm::GenXIntrinsic::genx_get_hwid, {});
  auto *HWIDCall = IRB.CreateCall(HWID);
  // TODO: revisit offset for scratch if it will be needed.
  auto *ThreadOffset =
      IRB.getInt32(UseGlobalMem ? BEConf->getStatelessPrivateMemSize()
                                : visa::StackPerThreadScratch);
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
    auto *ArgScalarType = Arg.getType()->getScalarType();
    auto NumElements = 1;
    bool AllowScalar = true;
    if (auto *VT = dyn_cast<VectorType>(Arg.getType()))
      NumElements = VT->getNumElements();
    if (ArgScalarType->isIntegerTy(1)) {
      if (!HandleMaskArgs)
        continue;
      NumElements = 1;
      ArgScalarType = IRB.getInt32Ty();
      AllowScalar = false;
    }
    Offset += calcPadding(Offset, ST->getGRFWidth());
    unsigned ArgSize = DL->getTypeSizeInBits(Arg.getType()) / genx::ByteBits;
    if (ForceArgMemPassing || Offset + ArgSize > ArgRegSize) {
      auto ReadVal = pop(Arg.getType(), IRB, Sp);
      Sp = ReadVal.second;
      Arg.replaceAllUsesWith(ReadVal.first);
    } else {
      auto *ArgRegType = IGCLLVM::FixedVectorType::get(
          ArgScalarType,
          ArgRegSize / (DL->getTypeSizeInBits(ArgScalarType) / genx::ByteBits));
      auto *ArgRead = buildReadPredefReg(PreDefined_Vars::PREDEFINED_ARG, IRB,
                                         ArgRegType, NumCalls > 0,
                                         AllowScalar, Offset, NumElements);
      if (Arg.getType()->getScalarType()->isIntegerTy(1)) {
        IGC_ASSERT(isa<VectorType>(Arg.getType()));
        ArgRead = cast<Instruction>(IRB.CreateTruncOrBitCast(
            ArgRead, IGCLLVM::FixedVectorType::get(
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
    // restore SP
    Value *RetBase = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                        IRB.getInt64Ty());
    if (PrivMemSize) {
      auto *RestoredSP =
          IRB.CreateSub(RetBase, IRB.getInt64(PrivMemSize), "");
      buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, RestoredSP);
    }
    RetSize = DL->getTypeSizeInBits(RetVal->getType()) / genx::ByteBits;
    std::vector<Value *> Worklist;
    if (isa<StructType>(RetVal->getType())) {
      while (auto *InsValue = dyn_cast<InsertValueInst>(RetVal)) {
        RetVal = InsValue->getOperand(0);
        if (InsValue->getOperand(1)->getType()->getScalarType()->isIntegerTy(1))
          continue;
        Worklist.push_back(InsValue);
      }
    } else
      Worklist.push_back(RetVal);
    IGC_ASSERT(!Worklist.empty());
    for (auto *Ins : Worklist) {
      Instruction *InstToReplaceIn = &I;
      unsigned Offset = 0;
      if (auto *InsVal = dyn_cast<InsertValueInst>(Ins)) {
        IRB.SetInsertPoint(InsVal);
        InstToReplaceIn = InsVal;
        Ins = InsVal->getOperand(1);

        IGC_ASSERT(InsVal->getIndices().size() == 1);
        auto IdxVal = InsVal->getIndices()[0];
        if (auto *StructTy =
                dyn_cast<StructType>(InsVal->getOperand(0)->getType()))
          // FIXME: consider calculating offset manually to not waste
          // GRF space on struct padding
          Offset = DL->getStructLayout(StructTy)->getElementOffset(IdxVal);
        else if (auto *ArrTy =
                     dyn_cast<ArrayType>(InsVal->getOperand(0)->getType()))
          Offset =
              (DL->getTypeSizeInBits(ArrTy->getScalarType()) / genx::ByteBits) *
              IdxVal;
        else if (auto* VecTy =
            dyn_cast<VectorType>(InsVal->getOperand(0)->getType()))
            Offset =
            (DL->getTypeSizeInBits(VecTy->getScalarType()) / genx::ByteBits) *
            IdxVal;
        else
          IGC_ASSERT_MESSAGE(0, "Unsupported type to extract from stackcall");
      }
      RetBase = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                   IRB.getInt64Ty());
      if (ForceRetMemPassing ||
          DL->getTypeSizeInBits(RetVal->getType()) / genx::ByteBits >
              RetRegSize) {
        RetBase = push(Ins, IRB, RetBase);
      } else {
        auto *RetRegType = IGCLLVM::FixedVectorType::get(
            Ins->getType()->getScalarType(),
            RetRegSize /
                (DL->getTypeSizeInBits(Ins->getType()->getScalarType()) /
                 genx::ByteBits));
        auto *RetRegWrite = buildWritePredefReg(
            PreDefined_Vars::PREDEFINED_RET, IRB, Ins,
            UndefValue::get(RetRegType), InstToReplaceIn, Offset);
        InstToReplaceIn->replaceUsesOfWith(Ins, RetRegWrite);
      }
    }
  }
  F.setMetadata(InstMD::FuncRetSize,
                MDNode::get(F.getContext(),
                            ConstantAsMetadata::get(IRB.getInt32(
                                divideCeil(RetSize, ST->getGRFWidth())))));
}

void GenXPrologEpilogInsertion::generateStackCall(CallInst *CI) {
  IRBuilder<> IRB(CI);
  unsigned Offset = 0;
  Value *OrigSp = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                     IRB.getInt64Ty(), true);
  auto *SpArgs = OrigSp;
  // write args
  for (auto &Arg : CI->arg_operands()) {
    auto *OrigTy = Arg->getType();
    if (OrigTy->getScalarType()->isIntegerTy(1)) {
      if (!HandleMaskArgs)
        continue;
      IGC_ASSERT(isa<VectorType>(Arg->getType()));
      Arg = IRB.CreateBitOrPointerCast(
          Arg,
          IGCLLVM::FixedVectorType::get(
              IRB.getIntNTy(cast<VectorType>(Arg->getType())->getNumElements()),
              1));
    }
    Offset += calcPadding(Offset, ST->getGRFWidth());
    auto ArgSize = DL->getTypeSizeInBits(Arg->getType()) / genx::ByteBits;
    auto *ArgRegType = IGCLLVM::FixedVectorType::get(
        Arg->getType()->getScalarType(),
        ArgRegSize / (DL->getTypeSizeInBits(Arg->getType()->getScalarType()) /
                      genx::ByteBits));
    if (ForceArgMemPassing || Offset + ArgSize > ArgRegSize) {
      // to stack
      SpArgs = push(Arg, IRB, SpArgs);
    } else {
      // to %ARG
      auto *FakeArg = buildReadPredefRegNoRegion(
          PreDefined_Vars::PREDEFINED_ARG, IRB, ArgRegType);
      auto *ArgRegWrite = buildWritePredefReg(PreDefined_Vars::PREDEFINED_ARG,
                                              IRB, Arg, FakeArg, CI, Offset);
      if (OrigTy->getScalarType()->isIntegerTy(1))
        ArgRegWrite = cast<Instruction>(
            IRB.CreateBitOrPointerCast(ArgRegWrite,OrigTy));
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
                  ConstantAsMetadata::get(IRB.getInt32(divideCeil(
                      (isVoidCall ? 0
                                  : (DL->getTypeSizeInBits(CI->getType())) /
                                        genx::ByteBits),
                      ST->getGRFWidth())))));
  if (isVoidCall)
    return;
  IRB.SetInsertPoint(CI->getNextNode());
  bool UseMemForRet =
      ForceRetMemPassing ||
      DL->getTypeSizeInBits(CI->getType()) / genx::ByteBits > RetRegSize;
  if (UseMemForRet)
    OrigSp = buildReadPredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB,
                                IRB.getInt64Ty(), CI, true);
  // read retvalue
  std::vector<std::pair<Instruction *, Instruction *>> Worklist;
  if (isa<StructType>(CI->getType())) {
    for (auto *U : CI->users()) {
      auto *EV = dyn_cast<ExtractValueInst>(U);
      IGC_ASSERT(EV && EV->getIndices().size() == 1);
      if (!U->getType()->getScalarType()->isIntegerTy(1))
        Worklist.push_back({cast<Instruction>(U), cast<Instruction>(U)});
    }
  } else
    Worklist.push_back({CI, UseMemForRet ? cast<Instruction>(OrigSp) : CI});
  for (auto &I : Worklist) {
    auto *ActualRet = I.first;
    IRB.SetInsertPoint(I.second->getNextNode());
    Value *ValToReplaceWith = nullptr;
    Instruction *InstToFix = nullptr;
    bool FixReplacement = true;
    if (UseMemForRet) {
      // from stack
      Region R(OrigSp->getType());
      Value *SpRet =
          R.createRdRegion(cast<Instruction>(OrigSp)->getOperand(0), "",
                           &*IRB.GetInsertPoint(), DebugLoc(), true);
      auto ReadVal = pop(ActualRet->getType(), IRB, SpRet);
      SpRet = ReadVal.second;

      InstToFix = cast<Instruction>(
          cast<Instruction>(cast<Instruction>(OrigSp)->getOperand(0))
              ->getOperand(1));
      ValToReplaceWith = ReadVal.first;
    } else {
      // from %RET arg
      unsigned Offset = 0;
      if (auto *EV = dyn_cast<ExtractValueInst>(ActualRet)) {
        // do not fix anything as we gonna replace extractValue insts,
        // not the call itself
        FixReplacement = false;
        auto IdxVal = EV->getIndices()[0];
        if (auto *StructTy = dyn_cast<StructType>(CI->getType()))
          // FIXME: consider calculating offset manually to not waste
          // GRF space on struct padding
          Offset = DL->getStructLayout(StructTy)->getElementOffset(IdxVal);
        else if (auto *ArrTy = dyn_cast<ArrayType>(CI->getType()))
          Offset =
              (DL->getTypeSizeInBits(ArrTy->getScalarType()) / genx::ByteBits) *
              IdxVal;
        else if (auto* VecTy = dyn_cast<VectorType>(CI->getType()))
            Offset =
            (DL->getTypeSizeInBits(VecTy->getScalarType()) / genx::ByteBits) *
            IdxVal;
        else
          IGC_ASSERT_MESSAGE(0, "Unsupported type to extract from stackcall");
      }
      unsigned NumElems = 1;
      if (auto *VT = dyn_cast<VectorType>(ActualRet->getType()))
        NumElems = VT->getNumElements();
      auto *RetRegType = IGCLLVM::FixedVectorType::get(
          ActualRet->getType()->getScalarType(),
          RetRegSize /
              (DL->getTypeSizeInBits(ActualRet->getType()->getScalarType()) /
               genx::ByteBits));
      auto *RetRead =
          buildReadPredefReg(PreDefined_Vars::PREDEFINED_RET, IRB, RetRegType,
                             CI, NumCalls > 1, true, Offset, NumElems);
      // here RetRead is rdr in a sequence
      // read_predef_reg->wrr->rdr where wrr exists iff NumCalls>1
      // and we want InstToFix to be read_predef_reg
      InstToFix = cast<Instruction>(RetRead->getOperand(0));
      if (NumCalls > 1)
        InstToFix = cast<Instruction>(InstToFix->getOperand(1));
      ValToReplaceWith = RetRead;
    }
    ActualRet->replaceAllUsesWith(ValToReplaceWith);
    // after replacing all of the uses we have to restore one of
    // in read_predef_reg to create a dependency from the call result
    // hence prevent its removal
    // InstToFix has to be read_predef_reg (we guarantee that when setting it)
    if (FixReplacement)
      InstToFix->replaceUsesOfWith(ValToReplaceWith, ActualRet);
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
  PrivMemSize += AllocaOffset;

  if (PrivMemSize > (UseGlobalMem ? BEConf->getStatelessPrivateMemSize()
                                  : visa::StackPerThreadScratch)) {
    DiagnosticInfoPrologEpilogInsertion Warn{
        CI->getFunction()->getName() +
            " frame allocation size is too big for TPM",
        DS_Warning};
    CI->getContext().diagnose(Warn);
  }

  buildWritePredefReg(PreDefined_Vars::PREDEFINED_FE_SP, IRB, Add);
  CI->replaceAllUsesWith(AllocaBase);
  CI->eraseFromParent();
}

Value *GenXPrologEpilogInsertion::push(Value *V, IRBuilder<> &IRB, Value *InitSP) {
  if (!isa<VectorType>(V->getType()))
    V = IRB.CreateBitCast(V, IGCLLVM::FixedVectorType::get(V->getType()->getScalarType(), 1));
  if (V->getType()->getScalarType()->isIntegerTy(1)) {
    IGC_ASSERT(isa<VectorType>(V->getType()));
    V = IRB.CreateBitOrPointerCast(
        V, IGCLLVM::FixedVectorType::get(IRB.getInt8Ty(),
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
      auto *CurReadType = IGCLLVM::FixedVectorType::get(
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
    Ty = IGCLLVM::FixedVectorType::get(Ty, 1);
  unsigned BytesLeft = DL->getTypeSizeInBits(Ty) / genx::ByteBits;
  unsigned Offset = 0;
  if (Ty->getScalarType()->isIntegerTy(1)) {
    IGC_ASSERT(isa<VectorType>(Ty));
    Ty = IGCLLVM::FixedVectorType::get(IRB.getInt8Ty(),
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
      auto *CurLdType = IGCLLVM::FixedVectorType::get(
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
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty,
    bool BuildTempVal, bool AllowScalar, unsigned Offset, unsigned Width) {
  return buildReadPredefReg(RegID, IRB, Ty, UndefValue::get(Ty),
                            BuildTempVal, AllowScalar, Offset, Width);
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefReg(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty, Value *Dep,
    bool BuildTempVal, bool AllowScalar, unsigned Offset, unsigned Width) {
  auto *RegRead = buildReadPredefRegNoRegion(RegID, IRB, Ty, Dep);

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
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty) {
  return buildReadPredefRegNoRegion(RegID, IRB, Ty, UndefValue::get(Ty));
}

Instruction *GenXPrologEpilogInsertion::buildReadPredefRegNoRegion(
    PreDefined_Vars RegID, IRBuilder<> &IRB, Type *Ty, Value *Dep) {
  Function *RegReadIntr = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertPoint()->getModule(),
      llvm::GenXIntrinsic::genx_read_predef_reg,
      {(isa<VectorType>(Ty) ? Ty : IGCLLVM::FixedVectorType::get(Ty, 1)), Dep->getType()});
  auto *RegRead =
      IRB.CreateCall(RegReadIntr, {IRB.getInt32(RegID), Dep});
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

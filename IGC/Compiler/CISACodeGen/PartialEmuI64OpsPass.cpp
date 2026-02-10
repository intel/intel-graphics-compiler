/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/TargetFolder.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"
#include "common/LLVMUtils.h"
#include "common/IGCIRBuilder.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/PartialEmuI64OpsPass.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

using std::ldexp;

namespace {

typedef llvm::IGCIRBuilder<TargetFolder> BuilderType;
typedef std::pair<Value *, Value *> ValuePair;

class InstExpander;
class Preprocessor;

class PartialEmuI64Ops : public FunctionPass {
  friend class InstExpander;
  friend class Preprocessor;

  const DataLayout *DL;
  IGC::CodeGenContext *CGC;
  llvm::DominatorTree *DT;

  BuilderType *IRB;
  InstExpander *Expander;

  LLVMContext *TheContext;
  Module *TheModule;
  Function *TheFunction;

  typedef DenseMap<Value *, ValuePair> ValueMapTy;
  ValueMapTy ValueMap;

  // Special bitcasts of 64-bit arguments, which need special handling as we
  // cannot replace argument type.
  SmallPtrSet<BitCastInst *, 8> Arg64Casts;

  SmallPtrSet<Instruction *, 32> DeadInsts;

public:
  static char ID;

  PartialEmuI64Ops()
      : FunctionPass(ID), DL(nullptr), CGC(nullptr), IRB(nullptr), Expander(nullptr), TheContext(nullptr),
        TheModule(nullptr), TheFunction(nullptr) {
    initializePartialEmuI64OpsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "PartialEmuI64Ops"; }

private:
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
  }

  LLVMContext *getContext() const { return TheContext; }
  Module *getModule() const { return TheModule; }
  Function *getFunction() const { return TheFunction; }
  bool hasPtr64() const {
    return (DL->getPointerSizeInBits() == 64 || DL->getPointerSizeInBits(ADDRESS_SPACE_GLOBAL) == 64 ||
            DL->getPointerSizeInBits(ADDRESS_SPACE_CONSTANT) == 64);
  }
  bool isPtr64(const PointerType *PtrTy) const {
    return CGC->getRegisterPointerSizeInBits(PtrTy->getAddressSpace()) == 64;
  }
  bool isInt64(const Type *Ty) const { return Ty->isIntegerTy(64); }
  bool isInt64(const Value *V) const { return isInt64(V->getType()); }

  bool isArg64Cast(BitCastInst *BC) const { return Arg64Casts.count(BC) != 0; }

  Type *getV2Int32Ty(unsigned NumElts = 1) const {
    return IGCLLVM::FixedVectorType::get(IRB->getInt32Ty(), NumElts * 2);
  }

  ValuePair getExpandedValues(Value *V);
  void setExpandedValues(Value *V, Value *Lo, Value *Hi);
  bool valueNotStored(Value *V);

  alignment_t getAlignment(LoadInst *LD) const {
    auto Align = IGCLLVM::getAlignmentValue(LD);
    if (Align == 0)
      Align = DL->getABITypeAlign(LD->getType()).value();
    return Align;
  }

  alignment_t getAlignment(StoreInst *ST) const {
    auto Align = IGCLLVM::getAlignmentValue(ST);
    if (Align == 0)
      Align = DL->getABITypeAlign(ST->getType()).value();
    return Align;
  }

  void copyKnownMetadata(Instruction *NewI, Instruction *OldI) const {
    unsigned LscCacheCtrlID = OldI->getContext().getMDKindID("lsc.cache.ctrl");
    SmallVector<std::pair<unsigned, MDNode *>, 8> MD;
    OldI->getAllMetadata(MD);
    for (const auto &MDPair : MD) {
      unsigned ID = MDPair.first;
      MDNode *N = MDPair.second;
      if (ID == LscCacheCtrlID)
        NewI->setMetadata(ID, N);
    }
    // Nothing needed yet
  }

  void dupMemoryAttribute(LoadInst *NewLD, LoadInst *RefLD, unsigned Off) const {
    auto alignment = getAlignment(RefLD);

    NewLD->setVolatile(RefLD->isVolatile());
    NewLD->setAlignment(IGCLLVM::getAlign(MinAlign(alignment, Off)));
    NewLD->setOrdering(RefLD->getOrdering());
    NewLD->setSyncScopeID(RefLD->getSyncScopeID());
    copyKnownMetadata(NewLD, RefLD);
  }

  void dupMemoryAttribute(StoreInst *NewST, StoreInst *RefST, unsigned Off) const {
    auto alignment = getAlignment(RefST);

    NewST->setVolatile(RefST->isVolatile());
    NewST->setAlignment(IGCLLVM::getAlign(MinAlign(alignment, Off)));
    NewST->setOrdering(RefST->getOrdering());
    NewST->setSyncScopeID(RefST->getSyncScopeID());
    copyKnownMetadata(NewST, RefST);
  }

  bool expandArguments(Function &F);
  bool preparePHIs(Function &F);
  bool expandInsts(Function &F);
  bool populatePHIs(Function &F);
  bool removeDeadInsts();
  bool hasNoInt64HWSupport(Instruction *instr);
};

class InstExpander : public InstVisitor<InstExpander, bool> {
  friend class InstVisitor<InstExpander, bool>;

  PartialEmuI64Ops *Emu;
  BuilderType *IRB;
  Instruction *m_CurrentInstr;

public:
  InstExpander(PartialEmuI64Ops *E, BuilderType *B) : Emu(E), IRB(B), m_CurrentInstr(nullptr) {}

  bool expand(Instruction *I);
  ValuePair getExpandedValues(Value *V);
  void setCurrentInstruction(Instruction *I) { m_CurrentInstr = I; }

private:
  bool visitInstruction(Instruction &);

  // Not I64 HW supported
  bool visitAdd(BinaryOperator &);
  bool visitSub(BinaryOperator &);
  bool visitMul(BinaryOperator &);
  bool visitAnd(BinaryOperator &);
  bool visitOr(BinaryOperator &);
  bool visitXor(BinaryOperator &);
  bool visitICmp(ICmpInst &);
  bool visitSelect(SelectInst &);

  bool visitRet(ReturnInst &) { return false; }
  bool visitBr(BranchInst &) { return false; }
  bool visitSwitch(SwitchInst &) { return false; }
  bool visitIndirectBr(IndirectBrInst &) { return false; }
  bool visitInvoke(InvokeInst &) { return false; }
  bool visitResume(ResumeInst &) { return false; }
  bool visitUnreachable(UnreachableInst &) { return false; }

  bool visitFNeg(UnaryOperator &) { return false; }
  bool visitFAdd(BinaryOperator &) { return false; }
  bool visitFSub(BinaryOperator &) { return false; }
  bool visitFMul(BinaryOperator &) { return false; }
  bool visitFDiv(BinaryOperator &) { return false; }
  bool visitSDiv(BinaryOperator &);
  bool visitUDiv(BinaryOperator &);
  bool visitSRem(BinaryOperator &);
  bool visitURem(BinaryOperator &);
  bool visitFRem(BinaryOperator &) { return false; }

  bool visitShl(BinaryOperator &) { return false; }
  bool visitLShr(BinaryOperator &) { return false; }
  bool visitAShr(BinaryOperator &) { return false; }

  bool visitAlloca(AllocaInst &) { return false; }
  bool visitLoad(LoadInst &) { return false; }
  bool visitStore(StoreInst &) { return false; }
  bool visitGetElementPtr(GetElementPtrInst &) { return false; }
  bool visitFence(FenceInst &) { return false; }
  bool visitAtomicCmpXchg(AtomicCmpXchgInst &);
  bool visitAtomicRMW(AtomicRMWInst &);

  bool visitTrunc(TruncInst &) { return false; }
  bool visitSExt(SExtInst &) { return false; }
  bool visitZExt(ZExtInst &) { return false; }
  bool visitFPToUI(FPToUIInst &) { return false; }
  bool visitFPToSI(FPToSIInst &) { return false; }
  bool visitUIToFP(UIToFPInst &) { return false; }
  bool visitSIToFP(SIToFPInst &) { return false; }
  bool visitFPTrunc(FPTruncInst &) { return false; }
  bool visitFPExt(FPExtInst &) { return false; }
  bool visitPtrToInt(PtrToIntInst &) { return false; }
  bool visitIntToPtr(IntToPtrInst &) { return false; }
  bool visitBitCast(BitCastInst &) { return false; }
  bool visitAddrSpaceCast(AddrSpaceCastInst &) { return false; }

  bool visitFCmp(FCmpInst &) { return false; }
  bool visitPHI(PHINode &) { return false; }
  bool visitCall(CallInst &) { return false; }

  bool visitVAArg(VAArgInst &);
  bool visitExtractElement(ExtractElementInst &) { return false; }
  bool visitInsertElement(InsertElementInst &) { return false; }
  bool visitShuffleVector(ShuffleVectorInst &) { return false; }
  bool visitExtractValue(ExtractValueInst &);
  bool visitInsertValue(InsertValueInst &);
  bool visitLandingPad(LandingPadInst &);

  void convert2xi32OutputBackToi64(Instruction &instr, Value *Lo, Value *Hi);
  bool isCombine2xi32Toi64Required(Instruction &instr);
};

class Preprocessor {
  PartialEmuI64Ops *Emu;
  BuilderType *IRB;

public:
  Preprocessor(PartialEmuI64Ops *E, BuilderType *B) : Emu(E), IRB(B) {}

  bool preprocess(Function &F) {
    bool Changed = false;
    // Preprocess additions with overflow.
    for (auto &BB : F) {
      for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
        IntrinsicInst *II = dyn_cast<IntrinsicInst>(BI);
        if (!II || II->getIntrinsicID() != Intrinsic::uadd_with_overflow ||
            !II->getArgOperand(0)->getType()->isIntegerTy(64)) {
          ++BI;
          continue;
        }

        IRB->SetInsertPoint(II);

        Value *LHS = II->getArgOperand(0);
        Value *RHS = II->getArgOperand(1);
        Value *Res = IRB->CreateAdd(LHS, RHS);
        Value *Overflow = IRB->CreateICmpULT(Res, LHS);

        for (auto UI = II->user_begin(), UE = II->user_end(); UI != UE; /*EMPTY*/) {
          User *U = *UI++;
          ExtractValueInst *Ex = cast<ExtractValueInst>(U);
          IGC_ASSERT(nullptr != Ex);
          IGC_ASSERT(Ex->getNumIndices() == 1);

          unsigned Idx = *Ex->idx_begin();
          IGC_ASSERT(Idx == 0 || Idx == 1);
          Ex->replaceAllUsesWith((Idx == 0) ? Res : Overflow);
          Ex->eraseFromParent();
        }
        IGC_ASSERT(II->user_empty());
        ++BI;
        II->eraseFromParent();
        Changed = true;
      }
    }
    // Preprocess non-LOAD/-STORE pointer usage if there's 64-bit pointer.
    IGC_ASSERT(nullptr != Emu);
    if (Emu->hasPtr64()) {
      for (auto &BB : F) {
        SmallVector<Instruction *, 16> LocalDeadInsts;
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE; ++BI) {
          switch (BI->getOpcode()) {
          default: // By default, NOTHING!
            break;
          case Instruction::ICmp: {
            ICmpInst *Cmp = cast<ICmpInst>(BI);
            IGC_ASSERT(nullptr != Cmp);
            PointerType *PtrTy = dyn_cast<PointerType>(Cmp->getOperand(0)->getType());
            if (!PtrTy || !Emu->isPtr64(PtrTy))
              continue;

            IRB->SetInsertPoint(Cmp);

            Value *LHS = Cmp->getOperand(0);
            Value *RHS = Cmp->getOperand(1);
            LHS = IRB->CreatePtrToInt(LHS, IRB->getInt64Ty());
            RHS = IRB->CreatePtrToInt(RHS, IRB->getInt64Ty());
            Cmp->setOperand(0, LHS);
            Cmp->setOperand(1, RHS);

            Changed = true;
            break;
          }
          case Instruction::Select: {
            SelectInst *SI = cast<SelectInst>(BI);
            PointerType *PtrTy = dyn_cast<PointerType>(SI->getType());
            if (!PtrTy || !Emu->isPtr64(PtrTy))
              continue;

            IRB->SetInsertPoint(SI);

            Value *TVal = SI->getTrueValue();
            Value *FVal = SI->getFalseValue();
            TVal = IRB->CreatePtrToInt(TVal, IRB->getInt64Ty());
            FVal = IRB->CreatePtrToInt(FVal, IRB->getInt64Ty());
            Value *NewPtr = IRB->CreateSelect(SI->getCondition(), TVal, FVal);
            NewPtr = IRB->CreateIntToPtr(NewPtr, PtrTy);
            SI->replaceAllUsesWith(NewPtr);
            LocalDeadInsts.push_back(SI);

            Changed = true;
            break;
          }
          case Instruction::Load: {
            LoadInst *LD = cast<LoadInst>(BI);
            PointerType *PtrTy = dyn_cast<PointerType>(LD->getType());
            if (!PtrTy || !Emu->isPtr64(PtrTy))
              continue;

            IRB->SetInsertPoint(LD);

            // Cast the original pointer to pointer to pointer to i64.
            Value *OldPtr = LD->getPointerOperand();
            PointerType *OldPtrTy = cast<PointerType>(OldPtr->getType());
            PointerType *NewPtrTy = IRB->getInt64Ty()->getPointerTo(OldPtrTy->getAddressSpace());
            Value *NewPtr = IRB->CreateBitCast(OldPtr, NewPtrTy);
            // Create new load.
            LoadInst *NewLD = IRB->CreateLoad(IRB->getInt64Ty(), NewPtr);
            Emu->dupMemoryAttribute(NewLD, LD, 0);
            // Cast the load i64 back to pointer.
            Value *NewVal = IRB->CreateIntToPtr(NewLD, PtrTy);
            LD->replaceAllUsesWith(NewVal);
            LocalDeadInsts.push_back(LD);

            Changed = true;
            break;
          }
          case Instruction::Store: {
            StoreInst *ST = cast<StoreInst>(BI);
            PointerType *PtrTy = dyn_cast<PointerType>(ST->getValueOperand()->getType());
            if (!PtrTy || !Emu->isPtr64(PtrTy))
              continue;

            IRB->SetInsertPoint(ST);

            // Cast the pointer to pointer to pointer to i64.
            Value *OldPtr = ST->getPointerOperand();
            PointerType *OldPtrTy = cast<PointerType>(OldPtr->getType());
            PointerType *NewPtrTy = IRB->getInt64Ty()->getPointerTo(OldPtrTy->getAddressSpace());
            Value *NewPtr = IRB->CreateBitCast(OldPtr, NewPtrTy);
            // Cast the pointer to be stored into i64.
            Value *OldVal = ST->getValueOperand();
            Value *NewVal = IRB->CreatePtrToInt(OldVal, IRB->getInt64Ty());
            // Create new store.
            StoreInst *NewST = IRB->CreateStore(NewVal, NewPtr);
            Emu->dupMemoryAttribute(NewST, ST, 0);
            LocalDeadInsts.push_back(ST);

            Changed = true;
            break;
          }
          case Instruction::IntToPtr: {
            IntToPtrInst *I2P = cast<IntToPtrInst>(BI);
            Value *Src = I2P->getOperand(0);
            PointerType *PtrTy = cast<PointerType>(I2P->getType());
            if (!Emu->isPtr64(PtrTy) && !Emu->isInt64(Src))
              continue;

            IRB->SetInsertPoint(I2P);
            unsigned int ptrSize = Emu->CGC->getRegisterPointerSizeInBits(PtrTy->getAddressSpace());

            Src = IRB->CreateZExtOrTrunc(Src, IRB->getIntNTy(ptrSize));
            I2P->setOperand(0, Src);

            Changed = true;
            break;
          }
          case Instruction::PtrToInt: {
            PtrToIntInst *P2I = cast<PtrToIntInst>(BI);
            Value *Src = P2I->getOperand(0);
            PointerType *PtrTy = cast<PointerType>(Src->getType());
            if (!Emu->isPtr64(PtrTy) || Emu->isInt64(Src))
              continue;

            IRB->SetInsertPoint(P2I);

            Value *NewVal = IRB->CreatePtrToInt(Src, IRB->getInt64Ty());
            NewVal = IRB->CreateZExtOrTrunc(NewVal, P2I->getType());
            P2I->replaceAllUsesWith(NewVal);
            LocalDeadInsts.push_back(P2I);

            Changed = true;
            break;
          }
          }
        }
        // Remove dead instructions.
        for (auto I : LocalDeadInsts)
          I->eraseFromParent();
      }
    }

    return Changed;
  }
};

char PartialEmuI64Ops::ID = 0;

} // End anonymous namespace

FunctionPass *createPartialEmuI64OpsPass() { return new PartialEmuI64Ops(); }

#define PASS_FLAG "igc-PartialEmuI64Ops"
#define PASS_DESC "IGC Partial I64-bit ops emulation"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PartialEmuI64Ops, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(PartialEmuI64Ops, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

bool PartialEmuI64Ops::runOnFunction(Function &F) {
  // Skip non-kernel function.
  MetaDataUtils *MDU = nullptr;
  MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  IGC_ASSERT(nullptr != MDU);
  auto FII = MDU->findFunctionsInfoItem(&F);
  if (FII == MDU->end_FunctionsInfo())
    return false;

  DL = &F.getParent()->getDataLayout();
  CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  BuilderType TheBuilder(F.getContext(), TargetFolder(*DL));
  InstExpander TheExpander(this, &TheBuilder);
  Preprocessor ThePreprocessor(this, &TheBuilder);
  IRB = &TheBuilder;
  Expander = &TheExpander;

  TheContext = &F.getContext();
  TheModule = F.getParent();
  TheFunction = &F;

  ValueMap.clear();
  Arg64Casts.clear();
  DeadInsts.clear();

  bool Changed = false;
  Changed |= ThePreprocessor.preprocess(F);
  Changed |= expandArguments(F);
  Changed |= expandInsts(F);
  Changed |= removeDeadInsts();

  return Changed;
}

ValuePair PartialEmuI64Ops::getExpandedValues(Value *V) {
  auto [VMI, New] = ValueMap.insert(std::make_pair(V, ValuePair()));
  if (!New)
    return VMI->second;

  if (dyn_cast<ConstantInt>(V)) {
    Value *Lo = IRB->CreateTrunc(V, IRB->getInt32Ty());
    Value *Hi = IRB->CreateTrunc(IRB->CreateLShr(V, 32), IRB->getInt32Ty());
    VMI->second = std::make_pair(Lo, Hi);
    return VMI->second;
  }

  if (dyn_cast<UndefValue>(V)) {
    Value *Lo = UndefValue::get(IRB->getInt32Ty());
    Value *Hi = UndefValue::get(IRB->getInt32Ty());
    VMI->second = std::make_pair(Lo, Hi);
    return VMI->second;
  }

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
    Value *Lo = nullptr;
    Value *Hi = nullptr;
    if (isa<PtrToIntOperator>(CE)) {
      V = IRB->CreateBitCast(V, getV2Int32Ty());
      Lo = IRB->CreateExtractElement(V, IRB->getInt32(0));
      Hi = IRB->CreateExtractElement(V, IRB->getInt32(1));
    } else {
      Lo = IRB->CreateTrunc(V, IRB->getInt32Ty());
      Hi = IRB->CreateTrunc(IRB->CreateLShr(V, 32), IRB->getInt32Ty());
    }
    VMI->second = std::make_pair(Lo, Hi);
    return VMI->second;
  }
  IGC_ASSERT_UNREACHABLE(); // TODO: NOT IMPLEMENTED!
}

void PartialEmuI64Ops::setExpandedValues(Value *V, Value *Lo, Value *Hi) {
  ValuePair Pair = std::make_pair(Lo, Hi);
  ValueMap.insert(std::make_pair(V, Pair));
}

bool PartialEmuI64Ops::valueNotStored(Value *V) {
  // returns true if the key is not in the map yet
  return (ValueMap.count(V) == 0);
}

bool PartialEmuI64Ops::expandArguments(Function &F) {
  Instruction *Pos = &F.getEntryBlock().front();
  IRB->SetInsertPoint(Pos);

  bool Changed = false;
  for (auto &Arg : F.args()) {
    if (!isInt64(&Arg))
      continue;
    bool expandArg = false;
    for (auto *U : Arg.users()) {
      if (Instruction *instr = dyn_cast<Instruction>(U)) {
        if (hasNoInt64HWSupport(instr)) {
          expandArg = true;
          break;
        }
      }
    }
    if (expandArg) {
      Value *V = IRB->CreateBitCast(&Arg, getV2Int32Ty());
      Value *Lo = IRB->CreateExtractElement(V, IRB->getInt32(0));
      Value *Hi = IRB->CreateExtractElement(V, IRB->getInt32(1));
      setExpandedValues(&Arg, Lo, Hi);
      Arg64Casts.insert(cast<BitCastInst>(V));
      Changed = true;
    }
  }

  return Changed;
}

ValuePair InstExpander::getExpandedValues(Value *V) {
  auto instrOp = dyn_cast<Instruction>(V);
  if (instrOp != nullptr) {

    Value *L = nullptr;
    Value *H = nullptr;

    bool insertNewPair = false;
    bool insertPointChanged = false;
    Instruction *currentIP = nullptr;

    IGC_ASSERT(nullptr != m_CurrentInstr);
    currentIP = &*IRB->GetInsertPoint();
    if (currentIP != nullptr && Emu->DT->dominates(llvm::cast<llvm::Instruction>(instrOp), currentIP)) {
      // insert the bitcast,extract instructions level up than the current instruction if needed
      auto *I = &*std::next(instrOp->getIterator());
      if (isa<PHINode>(instrOp))
        // insert the bitcast,extract instructions just after the PHI instructions block
        I = &*(instrOp->getParent()->getFirstNonPHI()->getIterator());
      IRB->SetInsertPoint(I);
      insertPointChanged = true;
    }

    switch (instrOp->getOpcode()) {
    case llvm::Instruction::Add:
    case llvm::Instruction::Sub:
      if (Emu->CGC->platform.hasInt64Add()) {
        if (Emu->valueNotStored(V)) {
          Value *_V = IRB->CreateBitCast(instrOp, Emu->getV2Int32Ty());
          L = IRB->CreateExtractElement(_V, IRB->getInt32(0));
          H = IRB->CreateExtractElement(_V, IRB->getInt32(1));
          insertNewPair = true;
        }
      }
      break;
    case llvm::Instruction::Mul:
    case llvm::Instruction::Xor:
    case llvm::Instruction::And:
    case llvm::Instruction::Or:
    case llvm::Instruction::ICmp:
    case llvm::Instruction::Select:
      // for the emulated opcode get the expanded values from the ValueMap directly
      break;

    case llvm::Instruction::PtrToInt:
      if (Emu->valueNotStored(V)) {
        Value *Ptr = instrOp->getOperand(0);
        GenISAIntrinsic::ID GIID = GenISAIntrinsic::GenISA_ptr_to_pair;
        Function *IFunc = GenISAIntrinsic::getDeclaration(Emu->getModule(), GIID, Ptr->getType());
        Value *_V = IRB->CreateCall(IFunc, Ptr);
        L = IRB->CreateExtractValue(_V, 0);
        H = IRB->CreateExtractValue(_V, 1);
        insertNewPair = true;
      }
      break;
    case llvm::Instruction::ZExt:
      if (Emu->valueNotStored(V)) {
        Value *Src = instrOp->getOperand(0);
        L = IRB->CreateZExt(Src, IRB->getInt32Ty());
        H = IRB->getInt32(0);
        insertNewPair = true;
      }
      break;
    case llvm::Instruction::SExt:
      if (Emu->valueNotStored(V)) {
        Value *Src = instrOp->getOperand(0);
        L = IRB->CreateSExt(Src, IRB->getInt32Ty());
        H = IRB->CreateAShr(L, IRB->getInt32(31));
        insertNewPair = true;
      }
      break;
    default:
      if (Emu->valueNotStored(V)) {
        Value *_V = IRB->CreateBitCast(instrOp, Emu->getV2Int32Ty());
        L = IRB->CreateExtractElement(_V, IRB->getInt32(0));
        H = IRB->CreateExtractElement(_V, IRB->getInt32(1));
        insertNewPair = true;
      }
      break;
    }

    if (insertPointChanged)
      IRB->SetInsertPoint(currentIP);

    if (insertNewPair) {
      ValuePair Pair = std::make_pair(L, H);
      Emu->ValueMap.insert(std::make_pair(V, Pair));
      return Pair;
    }
  }

  return Emu->getExpandedValues(V);
}

bool InstExpander::isCombine2xi32Toi64Required(Instruction &instr) {
  for (auto *U : instr.users()) {
    if (Instruction *instr = dyn_cast<Instruction>(U)) {
      if (!Emu->hasNoInt64HWSupport(instr))
        // there is at least one I64 HW instruction that is using this operand
        return true;
    }
  }
  return false;
}

void InstExpander::convert2xi32OutputBackToi64(Instruction &instr, Value *Lo, Value *Hi) {
  if (isCombine2xi32Toi64Required(instr)) {
    // combine 2xi32 to i64
    Type *V2I32Ty = Emu->getV2Int32Ty();
    Value *Op2xi32Toi64 = UndefValue::get(V2I32Ty);
    Op2xi32Toi64 = IRB->CreateInsertElement(Op2xi32Toi64, Lo, IRB->getInt32(0));
    Op2xi32Toi64 = IRB->CreateInsertElement(Op2xi32Toi64, Hi, IRB->getInt32(1));
    Op2xi32Toi64 = IRB->CreateBitCast(Op2xi32Toi64, IRB->getInt64Ty());
    instr.replaceAllUsesWith(Op2xi32Toi64);
    Emu->setExpandedValues(Op2xi32Toi64, Lo, Hi);
  }
}

bool PartialEmuI64Ops::hasNoInt64HWSupport(Instruction *instr) {
  if (
  // list of the instructions without Int64 HW support on PVC-B0+
      (instr->getOpcode() == llvm::Instruction::Mul)
      || (instr->getOpcode() == llvm::Instruction::Add && !CGC->platform.hasInt64Add()) ||
      (instr->getOpcode() == llvm::Instruction::Sub && !CGC->platform.hasInt64Add()) ||
      (instr->getOpcode() == llvm::Instruction::Xor) || (instr->getOpcode() == llvm::Instruction::And) ||
      (instr->getOpcode() == llvm::Instruction::Or) || (instr->getOpcode() == llvm::Instruction::ICmp) ||
      (instr->getOpcode() == llvm::Instruction::Select))
    return true;
  return false;
}

bool PartialEmuI64Ops::expandInsts(Function &F) {
  ReversePostOrderTraversal<Function *> RPOT(&F);
  bool Changed = false;
  for (auto &BB : RPOT) {
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /*EMPTY*/) {
      Instruction *I = &(*BI++);
      bool LocalChanged = Expander->expand(I);
      Changed |= LocalChanged;

      if (LocalChanged) {
        BI = std::next(BasicBlock::iterator(I));
        BE = I->getParent()->end();
        DeadInsts.insert(I);
      }
    }
  }

  return Changed;
}

bool PartialEmuI64Ops::removeDeadInsts() {
  bool Changed = false;
  for (auto *I : DeadInsts) {
    Type *Ty = I->getType();
    if (!Ty->isVoidTy())
      I->replaceAllUsesWith(UndefValue::get(Ty));
    I->eraseFromParent();
    Changed = true;
  }
  return Changed;
}

bool InstExpander::expand(Instruction *I) {
  IRB->SetInsertPoint(I);

  if (!visit(*I))
    return false;

  return true;
}

bool InstExpander::visitInstruction(Instruction &I) {
  [[maybe_unused]] bool isKnown = IGCLLVM::isFreezeInst(&I);
  IGC_ASSERT_MESSAGE(isKnown, "UNKNOWN INSTRUCTION is BEING EXPANDED!");
  return false;
}

bool InstExpander::visitAdd(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&BinOp))
    return false;

  if (Emu->CGC->platform.hasInt64Add())
    return false;

  setCurrentInstruction(&BinOp);
  auto [L0, H0] = getExpandedValues(BinOp.getOperand(0));
  auto [L1, H1] = getExpandedValues(BinOp.getOperand(1));

  GenISAIntrinsic::ID GIID = GenISAIntrinsic::GenISA_add_pair;
  Function *IFunc = GenISAIntrinsic::getDeclaration(Emu->getModule(), GIID);
  IGC_ASSERT(nullptr != IRB);
  Value *V = IRB->CreateCall4(IFunc, L0, H0, L1, H1);
  Value *Lo = IRB->CreateExtractValue(V, 0);
  Value *Hi = IRB->CreateExtractValue(V, 1);

  Emu->setExpandedValues(&BinOp, Lo, Hi);
  convert2xi32OutputBackToi64(BinOp, Lo, Hi);
  return true;
}

bool InstExpander::visitSub(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&BinOp))
    return false;

  if (Emu->CGC->platform.hasInt64Add())
    return false;

  setCurrentInstruction(&BinOp);
  auto [L0, H0] = getExpandedValues(BinOp.getOperand(0));
  auto [L1, H1] = getExpandedValues(BinOp.getOperand(1));

  GenISAIntrinsic::ID GIID = GenISAIntrinsic::GenISA_sub_pair;
  Function *IFunc = GenISAIntrinsic::getDeclaration(Emu->getModule(), GIID);
  IGC_ASSERT(nullptr != IRB);
  Value *V = IRB->CreateCall4(IFunc, L0, H0, L1, H1);
  Value *Lo = IRB->CreateExtractValue(V, 0);
  Value *Hi = IRB->CreateExtractValue(V, 1);

  Emu->setExpandedValues(&BinOp, Lo, Hi);
  convert2xi32OutputBackToi64(BinOp, Lo, Hi);
  return true;
}

bool InstExpander::visitMul(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&BinOp))
    return false;

  setCurrentInstruction(&BinOp);
  auto [L0, H0] = getExpandedValues(BinOp.getOperand(0));
  auto [L1, H1] = getExpandedValues(BinOp.getOperand(1));

  GenISAIntrinsic::ID GIID = GenISAIntrinsic::GenISA_mul_pair;
  Function *IFunc = GenISAIntrinsic::getDeclaration(Emu->getModule(), GIID);
  IGC_ASSERT(nullptr != IRB);
  Value *V = IRB->CreateCall4(IFunc, L0, H0, L1, H1);
  Value *Lo = IRB->CreateExtractValue(V, 0);
  Value *Hi = IRB->CreateExtractValue(V, 1);

  Emu->setExpandedValues(&BinOp, Lo, Hi);
  convert2xi32OutputBackToi64(BinOp, Lo, Hi);
  return true;
}

bool InstExpander::visitAnd(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&BinOp))
    return false;

  setCurrentInstruction(&BinOp);
  auto [L0, H0] = getExpandedValues(BinOp.getOperand(0));
  auto [L1, H1] = getExpandedValues(BinOp.getOperand(1));

  Value *Lo = IRB->CreateAnd(L0, L1);
  Value *Hi = IRB->CreateAnd(H0, H1);

  Emu->setExpandedValues(&BinOp, Lo, Hi);
  convert2xi32OutputBackToi64(BinOp, Lo, Hi);
  return true;
}

bool InstExpander::visitOr(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&BinOp))
    return false;

  setCurrentInstruction(&BinOp);
  auto [L0, H0] = getExpandedValues(BinOp.getOperand(0));
  auto [L1, H1] = getExpandedValues(BinOp.getOperand(1));

  Value *Lo = IRB->CreateOr(L0, L1);
  Value *Hi = IRB->CreateOr(H0, H1);

  Emu->setExpandedValues(&BinOp, Lo, Hi);
  convert2xi32OutputBackToi64(BinOp, Lo, Hi);
  return true;
}

bool InstExpander::visitXor(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&BinOp))
    return false;

  setCurrentInstruction(&BinOp);
  auto [L0, H0] = getExpandedValues(BinOp.getOperand(0));
  auto [L1, H1] = getExpandedValues(BinOp.getOperand(1));

  Value *Lo = IRB->CreateXor(L0, L1);
  Value *Hi = IRB->CreateXor(H0, H1);

  Emu->setExpandedValues(&BinOp, Lo, Hi);
  convert2xi32OutputBackToi64(BinOp, Lo, Hi);
  return true;
}

bool InstExpander::visitICmp(ICmpInst &Cmp) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(Cmp.getOperand(0)))
    return false;

  setCurrentInstruction(&Cmp);
  auto Pred = Cmp.getPredicate();
  auto [L0, H0] = getExpandedValues(Cmp.getOperand(0));
  auto [L1, H1] = getExpandedValues(Cmp.getOperand(1));

  Value *T0 = nullptr, *T1 = nullptr, *T2 = nullptr, *T3 = nullptr, *Res = nullptr;
  switch (Pred) {
  case CmpInst::ICMP_EQ:
    T0 = IRB->CreateICmpEQ(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    Res = IRB->CreateAnd(T1, T0);
    break;
  case CmpInst::ICMP_NE:
    T0 = IRB->CreateICmpNE(L0, L1), T1 = IRB->CreateICmpNE(H0, H1);
    Res = IRB->CreateOr(T1, T0);
    break;
  case CmpInst::ICMP_UGT:
    T0 = IRB->CreateICmpUGT(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpUGT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_UGE:
    T0 = IRB->CreateICmpUGE(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpUGT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_ULT:
    T0 = IRB->CreateICmpULT(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpULT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_ULE:
    T0 = IRB->CreateICmpULE(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpULT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_SGT:
    T0 = IRB->CreateICmpUGT(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpSGT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_SGE:
    T0 = IRB->CreateICmpUGE(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpSGT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_SLT:
    T0 = IRB->CreateICmpULT(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpSLT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  case CmpInst::ICMP_SLE:
    T0 = IRB->CreateICmpULE(L0, L1);
    T1 = IRB->CreateICmpEQ(H0, H1);
    T2 = IRB->CreateAnd(T1, T0);
    T3 = IRB->CreateICmpSLT(H0, H1);
    Res = IRB->CreateOr(T2, T3);
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Invalid ICmp predicate");
    break;
  }

  IGC_ASSERT(nullptr != Res);

  Cmp.replaceAllUsesWith(Res);
  return true;
}

bool InstExpander::visitSelect(SelectInst &SI) {
  IGC_ASSERT(nullptr != Emu);
  if (!Emu->isInt64(&SI))
    return false;

  setCurrentInstruction(&SI);
  Value *Cond = SI.getOperand(0);
  auto [L0, H0] = getExpandedValues(SI.getOperand(1));
  auto [L1, H1] = getExpandedValues(SI.getOperand(2));

  IGC_ASSERT(nullptr != IRB);
  Value *Lo = IRB->CreateSelect(Cond, L0, L1);
  Value *Hi = IRB->CreateSelect(Cond, H0, H1);

  Emu->setExpandedValues(&SI, Lo, Hi);
  convert2xi32OutputBackToi64(SI, Lo, Hi);
  return true;
}

bool InstExpander::visitSDiv(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&BinOp),
                     "There should not be `sdiv` which is already emulated by library call.");
  return false;
}

bool InstExpander::visitUDiv(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&BinOp),
                     "There should not be `udiv` which is already emulated by library call.");
  return false;
}

bool InstExpander::visitSRem(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&BinOp),
                     "There should not be `srem` which is already emulated by library call.");
  return false;
}

bool InstExpander::visitURem(BinaryOperator &BinOp) {
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&BinOp),
                     "There should not be `urem` which is already emulated by library call.");
  return false;
}

bool InstExpander::visitVAArg(VAArgInst &VAAI) {
  // TODO: Add i64 emulation support.
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&VAAI), "TODO: NOT IMPLEMENTED YET!");
  return false;
}

bool InstExpander::visitExtractValue(ExtractValueInst &EVI) {
  // TODO: Add i64 emulation support.
  IGC_ASSERT(nullptr != Emu);

  if (Emu->CGC->platform.hasPartialInt64Support())
    return false;

  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&EVI), "TODO: NOT IMPLEMENTED YET!");
  return false;
}

bool InstExpander::visitInsertValue(InsertValueInst &IVI) {
  // TODO: Add i64 emulation support.
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT(1 < IVI.getNumOperands());

  if (Emu->CGC->platform.hasPartialInt64Support())
    return false;

  IGC_ASSERT_MESSAGE(false == Emu->isInt64(IVI.getOperand(1)), "TODO: NOT IMPLEMENTED YET!");
  return false;
}

bool InstExpander::visitLandingPad(LandingPadInst &LPI) {
  // TODO: Add i64 emulation support.
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT(1 < LPI.getNumOperands());
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(LPI.getOperand(1)), "TODO: NOT IMPLEMENTED YET!");
  return false;
}

bool InstExpander::visitAtomicCmpXchg(AtomicCmpXchgInst &ACXI) {
  [[maybe_unused]] Value *V = ACXI.getCompareOperand();
  IGC_ASSERT(nullptr != V);
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(V), "TODO: NOT IMPLEMENTED YET!");

  return false;
}

bool InstExpander::visitAtomicRMW(AtomicRMWInst &RMW) {
  IGC_ASSERT(nullptr != Emu);
  IGC_ASSERT_MESSAGE(false == Emu->isInt64(&RMW), "TODO: NOT IMPLEMENTED YET!");
  return false;
}

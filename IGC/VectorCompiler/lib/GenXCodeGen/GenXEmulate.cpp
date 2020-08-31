/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
/// GenXEmulate
/// -----------
///
/// GenXEmulate is a mudule pass that emulates certain LLVM IR instructions.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "GENX_EMULATION"

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

static cl::opt<bool> OptIcmpEnable("genx-i64emu-icmp-enable", cl::init(true),
                                   cl::Hidden,
                                   cl::desc("enable icmp emulation"));
using IRBuilder = IRBuilder<TargetFolder>;

class GenXEmulate : public ModulePass {
  // Maps <opcode, type> to its corresponding emulation function.
  using OpType = std::pair<unsigned, Type *>;
  std::vector<Instruction *> ToErase;
  std::map<OpType, Function *> EmulationFuns;
  const GenXSubtarget *ST = nullptr;

  class Emu64Expander : public InstVisitor<Emu64Expander, Value *> {

    friend InstVisitor<Emu64Expander, Value *>;

    const GenXSubtarget &ST;
    IVSplitter SplitBuilder;
    Instruction &Inst;

    Value *expandBitwiseOp(BinaryOperator &);
    Value *visitAdd(BinaryOperator &);
    Value *visitSub(BinaryOperator &);
    Value *visitAnd(BinaryOperator &);
    Value *visitOr(BinaryOperator &);
    Value *visitXor(BinaryOperator &);
    Value *visitSelectInst(SelectInst &I);
    Value *visitICmp(ICmpInst &);

    Value *visitInstruction(Instruction &I) { return nullptr; }

    static bool isI64Cmp(const Instruction &I);
    static Value *detectBitwiseNot(BinaryOperator &);

    bool needsEmulation() const {
      return (SplitBuilder.IsI64Operation() || isI64Cmp(Inst));
    }

    IRBuilder getIRBuilder() {
      return IRBuilder(Inst.getParent(), BasicBlock::iterator(&Inst),
                       TargetFolder(Inst.getModule()->getDataLayout()));
    }

  public:
    Emu64Expander(const GenXSubtarget &ST, Instruction &I)
        : ST(ST), SplitBuilder(I), Inst(I) {}

    Value *tryExpand() {
      if (!needsEmulation())
        return nullptr;
      LLVM_DEBUG(dbgs() << "i64-emu: trying " << Inst << "\n");
      auto *Result = visit(Inst);

      if (Result)
        LLVM_DEBUG(dbgs() << "i64-emu: emulated with " << *Result << "\n");

      return Result;
    }
    using LHSplit = IVSplitter::LoHiSplit;
    Value *buildTernaryAddition(IRBuilder &Builder, Value &A, Value &B,
                                Value &C, const Twine &Name) const;
    static Value *buildGeneralICmp(IRBuilder &B, CmpInst::Predicate P,
                                   const LHSplit &L, const LHSplit &R);
    static Value *buildICmpEQ(IRBuilder &B, const LHSplit &L, const LHSplit &R);
    static Value *buildICmpNE(IRBuilder &B, const LHSplit &L, const LHSplit &R);
  };

public:
  static char ID;
  explicit GenXEmulate() : ModulePass(ID) {}
  virtual StringRef getPassName() const { return "GenX emulation"; }
  void getAnalysisUsage(AnalysisUsage &AU) const;
  bool runOnModule(Module &M);
  void runOnFunction(Function &F);

private:
  Value *emulateInst(Instruction *Inst);
  Function *getEmulationFunction(Instruction *Inst);
  // Check if a function is to emulate instructions.
  static bool isEmulationFunction(const Function* F) {
    if (F->empty())
      return false;
    if (F->hasFnAttribute("CMBuiltin"))
      return true;
    // FIXME: The above attribute is lost during SPIR-V translation.
    if (F->getName().contains("__cm_intrinsic_impl_"))
      return true;
    return false;
  }
};

} // end namespace
bool GenXEmulate::Emu64Expander::isI64Cmp(const Instruction &I) {
  if (Instruction::ICmp != I.getOpcode())
    return false;
  return I.getOperand(0)->getType()->getScalarType()->isIntegerTy(64);
}

Value *GenXEmulate::Emu64Expander::detectBitwiseNot(BinaryOperator &Op) {
  if (Instruction::Xor != Op.getOpcode())
    return nullptr;

  auto isAllOnes = [](const Value *V) {
    if (auto *C = dyn_cast<Constant>(V))
      return C->isAllOnesValue();
    return false;
  };

  if (isAllOnes(Op.getOperand(1)))
    return Op.getOperand(0);
  if (isAllOnes(Op.getOperand(0)))
    return Op.getOperand(1);

  return nullptr;
}
Value *GenXEmulate::Emu64Expander::expandBitwiseOp(BinaryOperator &Op) {
  auto Src0 = SplitBuilder.splitOperandHalf(0);
  auto Src1 = SplitBuilder.splitOperandHalf(1);

  auto Builder = getIRBuilder();

  Value *Part1 = Builder.CreateBinOp(Op.getOpcode(), Src0.Left, Src1.Left,
                                     Inst.getName() + ".part1");
  Value *Part2 = Builder.CreateBinOp(Op.getOpcode(), Src0.Right, Src1.Right,
                                     Inst.getName() + ".part2");
  return SplitBuilder.combineHalfSplit({Part1, Part2}, Op.getOpcodeName(),
                                       Inst.getType()->isIntegerTy());
}
Value *GenXEmulate::Emu64Expander::visitAdd(BinaryOperator &Op) {
  auto Src0 = SplitBuilder.splitOperandLoHi(0);
  auto Src1 = SplitBuilder.splitOperandLoHi(1);

  auto *AddcFunct = GenXIntrinsic::getGenXDeclaration(
      Inst.getModule(), GenXIntrinsic::genx_addc,
      {Src0.Lo->getType(), Src1.Lo->getType()});

  auto Builder = getIRBuilder();
  // add64 transforms as:
  //    [add_lo, carry] = genx_addc(src0.l0, src1.lo)
  //    add_hi = add(carry, add(src0.hi, src1.hi))
  //    add64  = combine(add_lo,add_hi)
  using namespace GenXIntrinsic::GenXResult;
  auto *AddcVal = Builder.CreateCall(AddcFunct, {Src0.Lo, Src1.Lo}, "addc");
  auto *AddLo = Builder.CreateExtractValue(AddcVal, {IdxAddc_Add}, "addc.add");
  auto *Carry =
      Builder.CreateExtractValue(AddcVal, {IdxAddc_Carry}, "addc.carry");
  auto *AddHi =
      buildTernaryAddition(Builder, *Carry, *Src0.Hi, *Src1.Hi, "add_hi");
  return SplitBuilder.combineLoHiSplit({AddLo, AddHi}, Op.getOpcodeName(),
                                       Inst.getType()->isIntegerTy());
}
Value *GenXEmulate::Emu64Expander::visitSub(BinaryOperator &Op) {
  auto Src0 = SplitBuilder.splitOperandLoHi(0);
  auto Src1 = SplitBuilder.splitOperandLoHi(1);

  auto *SubbFunct = GenXIntrinsic::getGenXDeclaration(
      Inst.getModule(), GenXIntrinsic::genx_subb,
      {Src0.Lo->getType(), Src1.Lo->getType()});

  auto Builder = getIRBuilder();
  // sub64 transforms as:
  //    [sub_lo, borrow] = genx_subb(src0.l0, src1.lo)
  //    sub_hi = add(src0.hi, add(-borrow, -src1.hi))
  //    sub64  = combine(sub_lo, sub_hi)
  using namespace GenXIntrinsic::GenXResult;
  auto *SubbVal = Builder.CreateCall(SubbFunct, {Src0.Lo, Src1.Lo}, "subb");
  auto *SubLo = Builder.CreateExtractValue(SubbVal, {IdxSubb_Sub}, "subb.sub");
  auto *Borrow =
      Builder.CreateExtractValue(SubbVal, {IdxSubb_Borrow}, "subb.borrow");
  auto *MinusBorrow = Builder.CreateNeg(Borrow, "borrow.negate");
  auto *MinusS1Hi = Builder.CreateNeg(Src1.Hi, "negative.src1_hi");
  auto *SubHi = buildTernaryAddition(Builder, *Src0.Hi, *MinusBorrow,
                                     *MinusS1Hi, "sub_hi");
  return SplitBuilder.combineLoHiSplit({SubLo, SubHi}, Op.getOpcodeName(),
                                       Inst.getType()->isIntegerTy());
}
Value *GenXEmulate::Emu64Expander::visitAnd(BinaryOperator &Op) {
  return expandBitwiseOp(Op);
}
Value *GenXEmulate::Emu64Expander::visitOr(BinaryOperator &Op) {
  return expandBitwiseOp(Op);
}
Value *GenXEmulate::Emu64Expander::visitXor(BinaryOperator &Op) {
  if (auto *NotOperand = detectBitwiseNot(Op)) {
    unsigned OperandIdx = NotOperand == Op.getOperand(0) ? 0 : 1;
    auto Src0 = SplitBuilder.splitOperandHalf(OperandIdx);
    auto *Part1 = BinaryOperator::CreateNot(Src0.Left, ".part1_not", &Inst);
    auto *Part2 = BinaryOperator::CreateNot(Src0.Right, ".part2_not", &Inst);
    return SplitBuilder.combineHalfSplit({Part1, Part2}, ".not",
                                         Op.getType()->isIntegerTy());
  }
  return expandBitwiseOp(Op);
}
Value *GenXEmulate::Emu64Expander::visitSelectInst(SelectInst &I) {
  auto SrcTrue = SplitBuilder.splitOperandLoHi(1);
  auto SrcFalse = SplitBuilder.splitOperandLoHi(2);
  auto *Cond = I.getCondition();

  auto Builder = getIRBuilder();
  // sel from 64-bit values transforms as:
  //    split TrueVal and FalseVal on lo/hi parts
  //    lo_part = self(cond, src0.l0, src1.lo)
  //    hi_part = self(cond, src0.hi, src1.hi)
  //    result  = combine(lo_part, hi_part)
  auto *SelLo = Builder.CreateSelect(Cond, SrcTrue.Lo, SrcFalse.Lo, "sel.lo");
  auto *SelHi = Builder.CreateSelect(Cond, SrcTrue.Hi, SrcFalse.Hi, "sel.hi");
  return SplitBuilder.combineLoHiSplit({SelLo, SelHi}, I.getOpcodeName(),
                                       I.getType()->isIntegerTy());
}
Value *GenXEmulate::Emu64Expander::visitICmp(ICmpInst &Cmp) {
  if (!OptIcmpEnable)
    return nullptr;

  auto Builder = getIRBuilder();

  unsigned BaseOperand = 0;
  IVSplitter Splitter(Cmp, &BaseOperand);
  auto Src0 = Splitter.splitOperandLoHi(0);
  auto Src1 = Splitter.splitOperandLoHi(1);

  LLVM_DEBUG(dbgs() << "processing: " << Cmp << "\n");

  Value *Result = buildGeneralICmp(Builder, Cmp.getPredicate(), Src0, Src1);

  if (Cmp.getType()->isIntegerTy() && !Result->getType()->isIntegerTy()) {
    // we expect this cast to be possible
    IGC_ASSERT(Cmp.getType() == Result->getType()->getScalarType());
    Result = Builder.CreateBitCast(Result, Cmp.getType(),
                                   Result->getName() + ".toi");
  }
  return Result;
}
Value *GenXEmulate::Emu64Expander::buildTernaryAddition(
    IRBuilder &Builder, Value &A, Value &B, Value &C, const Twine &Name) const {
  auto *SubH = Builder.CreateAdd(&A, &B, Name + ".part");
  return Builder.CreateAdd(SubH, &C, Name);
}
Value *GenXEmulate::Emu64Expander::buildICmpEQ(IRBuilder &Builder,
                                               const LHSplit &Src0,
                                               const LHSplit &Src1) {
  auto *T0 = Builder.CreateICmpEQ(Src0.Lo, Src1.Lo);
  auto *T1 = Builder.CreateICmpEQ(Src0.Hi, Src1.Hi);
  return Builder.CreateAnd(T0, T1, "emulated_icmp_eq");
}
Value *GenXEmulate::Emu64Expander::buildICmpNE(IRBuilder &Builder,
                                               const LHSplit &Src0,
                                               const LHSplit &Src1) {
  auto *T0 = Builder.CreateICmpNE(Src0.Lo, Src1.Lo);
  auto *T1 = Builder.CreateICmpNE(Src0.Hi, Src1.Hi);
  return Builder.CreateOr(T1, T0, "emulated_icmp_ne");
}
Value *GenXEmulate::Emu64Expander::buildGeneralICmp(IRBuilder &Builder,
                                                    CmpInst::Predicate P,
                                                    const LHSplit &Src0,
                                                    const LHSplit &Src1) {

  auto getEmulateCond1 = [](const CmpInst::Predicate P) {
    // For the unsigned predicate the first condition stays the same
    if (CmpInst::isUnsigned(P))
      return P;
    switch (P) {
    // transform signed predicate to an unsigned one
    case CmpInst::ICMP_SGT:
      return CmpInst::ICMP_UGT;
    case CmpInst::ICMP_SGE:
      return CmpInst::ICMP_UGE;
    case CmpInst::ICMP_SLT:
      return CmpInst::ICMP_ULT;
    case CmpInst::ICMP_SLE:
      return CmpInst::ICMP_ULE;
    default:
      llvm_unreachable("unexpected ICMP predicate for first condition");
    }
  };
  auto getEmulateCond2 = [](const CmpInst::Predicate P) {
    // discard EQ part
    switch (P) {
    case CmpInst::ICMP_SGT:
    case CmpInst::ICMP_SGE:
      return CmpInst::ICMP_SGT;
    case CmpInst::ICMP_SLT:
    case CmpInst::ICMP_SLE:
      return CmpInst::ICMP_SLT;
    case CmpInst::ICMP_UGT:
    case CmpInst::ICMP_UGE:
      return CmpInst::ICMP_UGT;
    case CmpInst::ICMP_ULT:
    case CmpInst::ICMP_ULE:
      return CmpInst::ICMP_ULT;
    default:
      llvm_unreachable("unexpected ICMP predicate for second condition");
    }
  };

  switch (P) {
  case CmpInst::ICMP_EQ:
    return buildICmpEQ(Builder, Src0, Src1);
  case CmpInst::ICMP_NE:
    return buildICmpNE(Builder, Src0, Src1);
  default: {
    CmpInst::Predicate EmuP1 = getEmulateCond1(P);
    CmpInst::Predicate EmuP2 = getEmulateCond2(P);
    auto *T0 = Builder.CreateICmp(EmuP1, Src0.Lo, Src1.Lo);
    auto *T1 = Builder.CreateICmpEQ(Src0.Hi, Src1.Hi);
    auto *T2 = Builder.CreateAnd(T1, T0);
    auto *T3 = Builder.CreateICmp(EmuP2, Src0.Hi, Src1.Hi);
    return Builder.CreateOr(T2, T3, "emu_" + CmpInst::getPredicateName(P));
  }
  }
}

char GenXEmulate::ID = 0;
namespace llvm {
void initializeGenXEmulatePass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXEmulate, "GenXEmulate", "GenXEmulate", false, false)
INITIALIZE_PASS_END(GenXEmulate, "GenXEmulate", "GenXEmulate", false, false)

ModulePass *llvm::createGenXEmulatePass() {
  initializeGenXEmulatePass(*PassRegistry::getPassRegistry());
  return new GenXEmulate;
}

void GenXEmulate::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
}

bool GenXEmulate::runOnModule(Module &M) {
  bool Changed = false;
  EmulationFuns.clear();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  // TODO: consider just an iteration over instructions

  // Process non-builtin functions.
  for (auto &F : M.getFunctionList()) {
    if (!isEmulationFunction(&F))
      runOnFunction(F);
  }
  Changed |= !ToErase.empty();
  for (auto *I : ToErase)
    I->eraseFromParent();
  ToErase.clear();

  // Delete unuse builtins or make used builtins internal.
  for (auto I = M.begin(); I != M.end();) {
    Function &F = *I++;
    if (isEmulationFunction(&F)) {
      Changed = true;
      if (F.use_empty())
        F.eraseFromParent();
      else
        F.setLinkage(GlobalValue::InternalLinkage);
    }
  }

  return Changed;
}

void GenXEmulate::runOnFunction(Function &F) {
  for (auto &BB : F.getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end(); ++I) {

      Instruction *Inst = &*I;
      auto *NewVal = emulateInst(Inst);
      if (NewVal) {
        Inst->replaceAllUsesWith(NewVal);
        ToErase.push_back(Inst);
      }
    }
  }
  return;
}

Function *GenXEmulate::getEmulationFunction(Instruction *Inst) {
  unsigned Opcode = Inst->getOpcode();
  Type *Ty = Inst->getType();
  OpType OpAndType = std::make_pair(Opcode, Ty);

  // Check if this emulation function has been cached.
  auto Iter = EmulationFuns.find(OpAndType);
  if (Iter != EmulationFuns.end())
    return Iter->second;

  IGC_ASSERT(ST && "subtarget expected");
  StringRef EmuFnName = ST->getEmulateFunction(Inst);
  if (EmuFnName.empty())
    return nullptr;

  Module *M = Inst->getParent()->getParent()->getParent();
  for (auto &F : M->getFunctionList()) {
    if (!isEmulationFunction(&F))
      continue;
    if (F.getReturnType() != Inst->getType())
      continue;
    StringRef FnName = F.getName();
    if (FnName.contains(EmuFnName)) {
      EmulationFuns[OpAndType] = &F;
      return &F;
    }
  }

  return nullptr;
}

Value *GenXEmulate::emulateInst(Instruction *Inst) {
  Function *EmuFn = getEmulationFunction(Inst);
  if (EmuFn) {
    IGC_ASSERT(!isa<CallInst>(Inst) && "call emulation not supported yet");
    llvm::IRBuilder<> Builder(Inst);
    SmallVector<Value *, 8> Args(Inst->operands());
    return Builder.CreateCall(EmuFn, Args);
  }
  IGC_ASSERT(ST);
  if (ST->emulateLongLong())
    return Emu64Expander(*ST, *Inst).tryExpand();
  return nullptr;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/Types.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

#include <queue>

#define DEBUG_TYPE "genx-count-indirect-stateless"

using namespace llvm;

namespace {
class GenXCountIndirectStateless : public ModulePass {
public:
  static char ID;
  GenXCountIndirectStateless() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CallGraphWrapperPass>();
    AU.setPreservesAll();
  }

  StringRef getPassName() const override {
    return "GenX count indirect stateless";
  }

  bool runOnModule(Module &M) override;

private:
  void countIndirectStateless(Function &F);
  unsigned getIndirectStatelessCount(Function &F) const;

  SmallDenseMap<Function *, unsigned> IndirectStatelessCount;
};

class CountIndirectStatelessVisitor
    : public InstVisitor<CountIndirectStatelessVisitor> {
public:
  unsigned getCount() const { return Count; }

  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &CI) {
    analyzePointer(CI.getPointerOperand());
  }
  void visitAtomicRMWInst(AtomicRMWInst &AI) {
    analyzePointer(AI.getPointerOperand());
  }
  void visitLoadInst(LoadInst &LI) { analyzePointer(LI.getPointerOperand()); }
  void visitStoreInst(StoreInst &SI) { analyzePointer(SI.getPointerOperand()); }

  void visitCallInst(CallInst &CI);

private:
  bool analyzePointer(Value *Ptr);
  static bool isMemLoadIntrinsic(Value *V);

  SmallDenseMap<Instruction *, bool> IsPointer;
  unsigned Count = 0;
};
} // namespace

namespace llvm {
void initializeGenXCountIndirectStatelessPass(PassRegistry &);
} // namespace llvm

char GenXCountIndirectStateless::ID = 0;

INITIALIZE_PASS_BEGIN(GenXCountIndirectStateless, "GenXCountIndirectStateless",
                      "GenXCountIndirectStateless", false, false)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(GenXCountIndirectStateless, "GenXCountIndirectStateless",
                    "GenXCountIndirectStateless", false, false)

ModulePass *llvm::createGenXCountIndirectStatelessPass() {
  initializeGenXCountIndirectStatelessPass(*PassRegistry::getPassRegistry());
  return new GenXCountIndirectStateless();
}

bool GenXCountIndirectStateless::runOnModule(Module &M) {
  for (auto &F : M)
    countIndirectStateless(F);

  if (IndirectStatelessCount.empty())
    return false;

  auto Kernels = make_filter_range(M, [](auto &F) { return vc::isKernel(F); });

  for (auto &F : Kernels) {
    const unsigned Count = getIndirectStatelessCount(F);

    vc::KernelMetadata MD(&F);
    MD.updateIndirectCountMD(Count);
  }

  return true;
}

void GenXCountIndirectStateless::countIndirectStateless(Function &F) {
  if (F.isDeclaration())
    return;

  CountIndirectStatelessVisitor Visitor;
  Visitor.visit(F);
  auto Count = Visitor.getCount();

  if (Count > 0)
    IndirectStatelessCount.try_emplace(&F, Count);
}

unsigned
GenXCountIndirectStateless::getIndirectStatelessCount(Function &F) const {
  const auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

  std::queue<const Function *> WorkList;
  SmallPtrSet<const Function *, 8> Visited;
  WorkList.push(&F);
  unsigned Count = 0;

  while (!WorkList.empty()) {
    auto *Curr = WorkList.front();
    WorkList.pop();

    if (Curr->isDeclaration() || !Visited.insert(Curr).second)
      continue;

    Count += IndirectStatelessCount.lookup(Curr);

    const auto *Node = CG[Curr];
    for (const auto &Edge : *Node) {
      const auto *F = Edge.second->getFunction();
      if (F && !Visited.contains(F))
        WorkList.push(F);
    }
  }

  return Count;
}

void CountIndirectStatelessVisitor::visitCallInst(CallInst &CI) {
  if (vc::InternalIntrinsic::isStatelessIntrinsic(&CI)) {
    analyzePointer(vc::InternalIntrinsic::getMemoryAddressOperand(&CI));
    return;
  }

  auto IID = vc::getAnyIntrinsicID(&CI);
  switch (IID) {
  default:
    return;
  case Intrinsic::masked_gather:
  case Intrinsic::masked_load:
    analyzePointer(CI.getArgOperand(0));
    break;
  case Intrinsic::masked_scatter:
  case Intrinsic::masked_store:
    analyzePointer(CI.getArgOperand(1));
    break;
  case GenXIntrinsic::genx_svm_block_ld:
  case GenXIntrinsic::genx_svm_block_ld_unaligned:
  case GenXIntrinsic::genx_svm_block_st:
    analyzePointer(CI.getArgOperand(0));
    break;
  case GenXIntrinsic::genx_svm_gather:
  case GenXIntrinsic::genx_svm_scatter:
    analyzePointer(CI.getArgOperand(2));
    break;
  case GenXIntrinsic::genx_svm_gather4_scaled:
  case GenXIntrinsic::genx_svm_scatter4_scaled:
    analyzePointer(CI.getArgOperand(3));
    analyzePointer(CI.getArgOperand(4));
    break;

  case GenXIntrinsic::genx_svm_atomic_add:
  case GenXIntrinsic::genx_svm_atomic_and:
  case GenXIntrinsic::genx_svm_atomic_cmpxchg:
  case GenXIntrinsic::genx_svm_atomic_dec:
  case GenXIntrinsic::genx_svm_atomic_imax:
  case GenXIntrinsic::genx_svm_atomic_imin:
  case GenXIntrinsic::genx_svm_atomic_inc:
  case GenXIntrinsic::genx_svm_atomic_max:
  case GenXIntrinsic::genx_svm_atomic_min:
  case GenXIntrinsic::genx_svm_atomic_or:
  case GenXIntrinsic::genx_svm_atomic_sub:
  case GenXIntrinsic::genx_svm_atomic_xchg:
  case GenXIntrinsic::genx_svm_atomic_xor:
    analyzePointer(CI.getArgOperand(1));
    break;
  }
}

bool CountIndirectStatelessVisitor::analyzePointer(Value *Ptr) {
  auto *Inst = dyn_cast<Instruction>(Ptr);
  if (!Inst)
    return false;

  if (auto It = IsPointer.find(Inst); It != IsPointer.end()) {
    if (It->second)
      ++Count;
    return It->second;
  }

  if (auto [It, Inserted] = IsPointer.try_emplace(Inst, false); !Inserted)
    return It->second;

  auto *Ty = Inst->getType();
  const bool IsInt64 = Ty->isIntOrIntVectorTy(64);

  if (Ty->isFPOrFPVectorTy()) {
    return false;
  }

  if (auto *PTy = dyn_cast<PointerType>(Ty)) {
    const auto AS = PTy->getAddressSpace();
    if (AS == vc::AddrSpace::Local || AS == vc::AddrSpace::CodeSectionINTEL ||
        AS == vc::AddrSpace::GlobalA32)
      return false;
  }

  if (isa<LoadInst>(Inst) || isMemLoadIntrinsic(Inst) ||
      (isa<CallInst>(Inst) && !vc::isAnyNonTrivialIntrinsic(Inst))) {
    IsPointer[Inst] = true;
    ++Count;
    return true;
  }

  if (isa<AtomicRMWInst>(Inst)) {
    IsPointer[Inst] = IsInt64;
    Count += IsInt64;
    return IsInt64;
  }

  if (isa<AtomicCmpXchgInst>(Inst)) {
    auto *ValTy = cast<StructType>(Ty)->getElementType(0);
    const auto IsInt64 = ValTy->isIntOrIntVectorTy(64);
    IsPointer[Inst] = IsInt64;
    Count += IsInt64;
    return IsInt64;
  }

  const auto Opcode = Inst->getOpcode();
  switch (Opcode) {
  default:
    break;
  case Instruction::Trunc:
  case Instruction::SExt:
  case Instruction::ZExt:
    // Trunc, ZExt and SExt instructions cannot produce a pointer value.
  case Instruction::Mul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    // Mul-like and div-like instructions cannot produce a pointer value.
    return false;
  }

  if (isa<GetElementPtrInst>(Inst) || isa<ExtractElementInst>(Inst)) {
    const bool IsIndirect = analyzePointer(Inst->getOperand(0));
    IsPointer[Inst] = IsIndirect;
    return IsIndirect;
  }

  for (auto &Op : Inst->operands()) {
    if (analyzePointer(Op.get())) {
      IsPointer[Inst] = true;
      return true;
    }
  }

  return false;
}

bool CountIndirectStatelessVisitor::isMemLoadIntrinsic(Value *V) {
  if (vc::InternalIntrinsic::isInternalMemoryIntrinsic(V))
    return true;

  auto IID = vc::getAnyIntrinsicID(V);
  switch (IID) {
  default:
    break;
  case Intrinsic::masked_gather:
  case Intrinsic::masked_load:
  case GenXIntrinsic::genx_svm_block_ld:
  case GenXIntrinsic::genx_svm_block_ld_unaligned:
  case GenXIntrinsic::genx_svm_gather:
    return true;
  case GenXIntrinsic::genx_svm_atomic_add:
  case GenXIntrinsic::genx_svm_atomic_cmpxchg:
  case GenXIntrinsic::genx_svm_atomic_dec:
  case GenXIntrinsic::genx_svm_atomic_inc:
  case GenXIntrinsic::genx_svm_atomic_sub:
  case GenXIntrinsic::genx_svm_atomic_xchg:
    return V->getType()->isIntOrIntVectorTy(64);
  }

  return false;
}

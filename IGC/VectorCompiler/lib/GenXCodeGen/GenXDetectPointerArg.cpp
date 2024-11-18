/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/Types.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Pass.h"

#include <stack>

#define DEBUG_TYPE "genx-detect-pointer-arg"

using namespace llvm;

namespace {
class GenXDetectPointerArg : public ModulePass,
                             public InstVisitor<GenXDetectPointerArg> {
public:
  static char ID;
  GenXDetectPointerArg() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

  StringRef getPassName() const override { return "GenX detect pointer arg"; }

  bool runOnModule(Module &M) override;

  void visitCallInst(CallInst &CI);
  void visitLoadInst(LoadInst &LI);
  void visitStoreInst(StoreInst &SI);
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &CI);
  void visitAtomicRMWInst(AtomicRMWInst &AI);

private:
  bool handleKernel(Function &F);
  void analyzeValue(Value *V);

  std::pair<AllocaInst *, GetElementPtrInst *> findAllocaWithOffset(Value *Ptr);
  bool findStoredArgs(LoadInst &LI);

  Function *CurrentFunc = nullptr;

  SmallPtrSet<Argument *, 8> PointerArgs;
  SmallPtrSet<Value *, 8> Visited;
  DenseMap<std::pair<AllocaInst *, uint64_t>, Argument *> AllocaArgs;
};
} // namespace

namespace llvm {
void initializeGenXDetectPointerArgPass(PassRegistry &);
}

char GenXDetectPointerArg::ID = 0;

INITIALIZE_PASS_BEGIN(GenXDetectPointerArg, "GenXDetectPointerArg",
                      "GenXDetectPointerArg", false, false)
INITIALIZE_PASS_END(GenXDetectPointerArg, "GenXDetectPointerArg",
                    "GenXDetectPointerArg", false, false)

ModulePass *llvm::createGenXDetectPointerArgPass() {
  initializeGenXDetectPointerArgPass(*PassRegistry::getPassRegistry());
  return new GenXDetectPointerArg();
}

bool GenXDetectPointerArg::runOnModule(Module &M) {
  bool Changed = false;
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    if (!vc::isKernel(F))
      continue;

    Changed |= handleKernel(F);
  }

  return Changed;
}

bool GenXDetectPointerArg::handleKernel(Function &F) {
  CurrentFunc = &F;
  PointerArgs.clear();
  Visited.clear();
  AllocaArgs.clear();

  LLVM_DEBUG(dbgs() << "Processing kernel: " << F.getName() << "\n");

  visit(F);

  if (PointerArgs.empty())
    return false;

  vc::KernelMetadata MD(&F);
  auto Descs = MD.getArgTypeDescs();
  SmallVector<StringRef, 8> NewDescs(Descs.begin(), Descs.end());

  if (NewDescs.size() < F.arg_size())
    NewDescs.resize(F.arg_size(), "");

  for (auto *Arg : PointerArgs) {
    if (!Arg->getType()->isIntegerTy(64) && !Arg->getType()->isPointerTy())
      continue;

    auto ArgNo = Arg->getArgNo();
    auto Kind = MD.getArgKind(ArgNo);

    if (!vc::isNormalCategoryArgKind(Kind))
      continue;

    auto Desc = Descs.size() > ArgNo ? Descs[ArgNo] : "";
    if (!vc::isDescSvmPtr(Desc)) {
      LLVM_DEBUG(dbgs() << "Updating arg type desc for arg " << ArgNo << ": '"
                        << Desc << "' is updated to '" << vc::OCLAttributes::SVM
                        << "'\n");
      NewDescs[ArgNo] = vc::OCLAttributes::SVM;
    }
  }

  MD.updateArgTypeDescsMD(std::move(NewDescs));

  return true;
}

static Value *getSvmPointerOperand(const CallInst &CI) {
  auto IID = vc::getAnyIntrinsicID(&CI);

  switch (IID) {
  default:
    break;
  case GenXIntrinsic::genx_svm_block_ld:
  case GenXIntrinsic::genx_svm_block_ld_unaligned:
  case GenXIntrinsic::genx_svm_block_st:
    return CI.getArgOperand(0);
  case GenXIntrinsic::genx_svm_gather:
  case GenXIntrinsic::genx_svm_scatter:
    return CI.getArgOperand(2);
  case GenXIntrinsic::genx_svm_gather4_scaled:
  case GenXIntrinsic::genx_svm_scatter4_scaled:
    return CI.getArgOperand(3);
  case GenXIntrinsic::genx_svm_atomic_add:
  case GenXIntrinsic::genx_svm_atomic_sub:
  case GenXIntrinsic::genx_svm_atomic_min:
  case GenXIntrinsic::genx_svm_atomic_max:
  case GenXIntrinsic::genx_svm_atomic_xchg:
  case GenXIntrinsic::genx_svm_atomic_and:
  case GenXIntrinsic::genx_svm_atomic_or:
  case GenXIntrinsic::genx_svm_atomic_xor:
  case GenXIntrinsic::genx_svm_atomic_imin:
  case GenXIntrinsic::genx_svm_atomic_imax:
  case GenXIntrinsic::genx_svm_atomic_inc:
  case GenXIntrinsic::genx_svm_atomic_dec:
  case GenXIntrinsic::genx_svm_atomic_cmpxchg:
    return CI.getArgOperand(1);
  }

  return nullptr;
}

void GenXDetectPointerArg::visitCallInst(CallInst &CI) {
  if (vc::InternalIntrinsic::isStatelessIntrinsic(&CI)) {
    LLVM_DEBUG(dbgs() << "Found stateless intrinsic: " << CI << "\n");
    if (auto *Addr = vc::InternalIntrinsic::getMemoryAddressOperand(&CI))
      analyzeValue(Addr);
    if (auto *Base = vc::InternalIntrinsic::getMemoryBaseOperand(&CI))
      analyzeValue(Base);
  }

  if (auto *Ptr = getSvmPointerOperand(CI)) {
    LLVM_DEBUG(dbgs() << "Found SVM intrinsic: " << CI << "\n");
    analyzeValue(Ptr);
  }

  if (vc::isAnyNonTrivialIntrinsic(&CI))
    return;

  // Non-intrinsic functions
  for (auto &Op : CI.args()) {
    auto *OpTy = Op->getType();
    // TODO: support 64-bit integers as well.
    if (OpTy->isPointerTy())
      analyzeValue(Op);
  }
}

void GenXDetectPointerArg::visitLoadInst(LoadInst &LI) {
  LLVM_DEBUG(dbgs() << "Found load: " << LI << "\n");
  analyzeValue(LI.getPointerOperand());
}

static Optional<int64_t> accumulateConstantOffset(GetElementPtrInst &GEPI) {
  int64_t Offset = 0;
  auto &DL = GEPI.getModule()->getDataLayout();

  for (auto GTI = gep_type_begin(GEPI), PrevGTI = gep_type_end(GEPI);
       GTI != gep_type_end(GEPI); PrevGTI = GTI++) {
    auto *C = dyn_cast<ConstantInt>(GTI.getOperand());
    if (!C)
      return {};

    if (C->isZero())
      continue;

    auto IndexV = C->getZExtValue();
    if (auto *STy = GTI.getStructTypeOrNull())
      Offset += DL.getStructLayout(STy)->getElementOffset(
          static_cast<unsigned>(IndexV));
    else
      Offset += IndexV * DL.getTypeAllocSize(GTI.getIndexedType());
  }

  return {Offset};
}

void GenXDetectPointerArg::visitStoreInst(StoreInst &SI) {
  LLVM_DEBUG(dbgs() << "Found store: " << SI << "\n");
  analyzeValue(SI.getPointerOperand());

  // Only track stores of kernel arguments.
  auto *V = dyn_cast<Argument>(SI.getValueOperand());
  if (!V)
    return;

  auto *Ty = V->getType()->getScalarType();
  if (!Ty->isPointerTy() && !Ty->isIntegerTy(64))
    return;

  auto [AI, GEPI] = findAllocaWithOffset(SI.getPointerOperand());
  if (!AI)
    return;

  uint64_t Offset = 0;

  if (GEPI) {
    auto MaybeOffset = accumulateConstantOffset(*GEPI);
    if (!MaybeOffset || *MaybeOffset < 0)
      return;
    Offset = *MaybeOffset;
  }

  LLVM_DEBUG(dbgs() << "Storing arg: " << *V << " to alloca: " << *AI
                    << " with offset: " << Offset << "\n");
  AllocaArgs.try_emplace({AI, Offset}, V);
}

void GenXDetectPointerArg::visitAtomicCmpXchgInst(AtomicCmpXchgInst &CI) {
  LLVM_DEBUG(dbgs() << "Found atomic cmpxchg: " << CI << "\n");
  analyzeValue(CI.getPointerOperand());
}

void GenXDetectPointerArg::visitAtomicRMWInst(AtomicRMWInst &AI) {
  LLVM_DEBUG(dbgs() << "Found atomic rmw: " << AI << "\n");
  analyzeValue(AI.getPointerOperand());
}

std::pair<AllocaInst *, GetElementPtrInst *>
GenXDetectPointerArg::findAllocaWithOffset(Value *Ptr) {
  // Only support pointer to alloca
  if (auto *PTy = dyn_cast<PointerType>(Ptr->getType());
      PTy->getAddressSpace() != vc::AddrSpace::Private)
    return {};

  AllocaInst *AI = nullptr;
  GetElementPtrInst *GEPI = nullptr;

  while (!AI) {
    if (auto *BCI = dyn_cast<BitCastInst>(Ptr)) {
      Ptr = BCI->getOperand(0);
      continue;
    }

    if (auto *GEP = dyn_cast<GetElementPtrInst>(Ptr)) {
      // Only one getelementptr instruction is supported.
      if (GEPI)
        return {};
      GEPI = GEP;
      Ptr = GEP->getPointerOperand();
      continue;
    }

    AI = dyn_cast<AllocaInst>(Ptr);
    if (!AI)
      return {};
  }

  return {AI, GEPI};
}

bool GenXDetectPointerArg::findStoredArgs(LoadInst &LI) {
  LLVM_DEBUG(dbgs() << "Looking for stored args for load: " << LI << "\n");

  auto [AI, GEPI] = findAllocaWithOffset(LI.getPointerOperand());
  if (!AI)
    return false;

  auto &DL = LI.getModule()->getDataLayout();
  SmallVector<uint64_t, 4> Offsets = {0};

  if (GEPI) {
    for (auto GTI = gep_type_begin(GEPI), PrevGTI = gep_type_end(GEPI);
         GTI != gep_type_end(GEPI); PrevGTI = GTI++) {
      if (auto *C = dyn_cast<ConstantInt>(GTI.getOperand())) {
        if (C->isZero())
          continue;

        uint64_t Offset = 0;
        auto IndexV = C->getZExtValue();

        if (auto *STy = GTI.getStructTypeOrNull())
          Offset = DL.getStructLayout(STy)->getElementOffset(
              static_cast<unsigned>(IndexV));
        else
          Offset = IndexV * DL.getTypeAllocSize(GTI.getIndexedType());

        for (auto &O : Offsets)
          O += Offset;
      } else {
        if (PrevGTI == gep_type_end(GEPI))
          return false;

        auto *ATy = dyn_cast<ArrayType>(PrevGTI.getIndexedType());
        if (!ATy)
          return false;

        uint64_t ArrayElements = ATy->getNumElements();
        uint64_t ByteSize = DL.getTypeAllocSize(GTI.getIndexedType());

        SmallVector<uint64_t, 4> Tmp;
        auto Insert = std::back_inserter(Tmp);
        for (auto I = 0; I < ArrayElements; ++I)
          transform(Offsets, Insert,
                    [I, ByteSize](uint64_t O) { return O + I * ByteSize; });

        Offsets = std::move(Tmp);
      }
    }
  }

  bool Found = false;
  for (auto Offset : Offsets) {
    auto It = AllocaArgs.find({AI, Offset});
    if (It != AllocaArgs.end()) {
      LLVM_DEBUG(dbgs() << "Found stored arg for load: " << *It->second
                        << "\n");
      PointerArgs.insert(It->second);
      Found = true;
    }
  }

  return Found;
}

void GenXDetectPointerArg::analyzeValue(Value *V) {
  std::stack<Value *> WorkList;

  WorkList.push(V);

  while (!WorkList.empty()) {
    auto *Curr = WorkList.top();
    WorkList.pop();

    if (isa<Constant>(Curr) || Visited.contains(Curr))
      continue;

    auto *Ty = Curr->getType()->getScalarType();
    if (Ty->isPointerTy()) {
      auto AS = Ty->getPointerAddressSpace();
      if (AS == vc::AddrSpace::Local || AS == vc::AddrSpace::Program ||
          AS == vc::AddrSpace::GlobalA32)
        continue;
    } else if (!Ty->isIntegerTy(64))
      continue;

    LLVM_DEBUG(dbgs() << "The value can be a pointer: " << *Curr << "\n");

    if (auto *Arg = dyn_cast<Argument>(Curr)) {
      PointerArgs.insert(Arg);
      continue;
    }

    auto *Inst = dyn_cast<Instruction>(Curr);
    if (!Inst || vc::InternalIntrinsic::isInternalMemoryIntrinsic(Inst))
      continue;

    if (auto *LI = dyn_cast<LoadInst>(Inst); LI && findStoredArgs(*LI))
      continue;

    Visited.insert(Inst);

    if (GenXIntrinsic::isRdRegion(Inst)) {
      WorkList.push(Inst->getOperand(0));
      continue;
    }
    if (GenXIntrinsic::isWrRegion(Inst)) {
      WorkList.push(Inst->getOperand(0));
      WorkList.push(Inst->getOperand(1));
      continue;
    }
    if (auto *GEP = dyn_cast<GetElementPtrInst>(Inst)) {
      WorkList.push(GEP->getPointerOperand());
      continue;
    }

    // Skip function calls and intrinsics.
    if (isa<CallInst>(Inst))
      continue;

    switch (Inst->getOpcode()) {
    default:
      break;
    case Instruction::Mul:
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
      // Mul-like and div-like instructions cannot produce a pointer.
      continue;
    }

    for (auto &Op : Inst->operands())
      WorkList.push(Op.get());
  }
}

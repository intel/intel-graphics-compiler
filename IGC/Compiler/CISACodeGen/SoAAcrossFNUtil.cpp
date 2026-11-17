/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SoAAcrossFNUtil.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/Analysis/ValueTracking.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "Probe/Assertion.h"

#include <optional>

using namespace llvm;

namespace IGC::SoAAcrossFNUtil {
void collectSoAUnsupportedFns(Module &M, CallGraph &CG, GenXFunctionGroupAnalysis *FGA, IGCMD::MetaDataUtils *MdUtils,
                              SmallPtrSetImpl<Function *> &UnsupportedFns) {
  auto collectRecursiveFns = [&] {
    for (auto I = scc_begin(&CG); !I.isAtEnd(); ++I) {
      if (!I.hasCycle())
        continue;

      for (CallGraphNode *N : *I)
        if (Function *F = N->getFunction())
          UnsupportedFns.insert(F);
    }
  };

  auto hasUnsupportedAttr = [](Function &F) {
    return F.hasFnAttribute("visaStackCall") || F.hasFnAttribute("hasRecursion") || F.hasFnAttribute("hasVLA") ||
           F.hasFnAttribute("referenced-indirectly");
  };

  auto hasUnsupportedFGInfo = [&](Function &F) {
    auto *FG = FGA ? FGA->getGroup(&F) : nullptr;
    return (FG && (FG->hasStackCall() || FG->hasVariableLengthAlloca() || FGA->isIndirectCallGroup(FG) ||
                   FG->hasInlineAsm() || FG->hasRecursion()));
  };

  collectRecursiveFns();

  for (auto &F : M) {
    if (F.isDeclaration() || F.hasAddressTaken() || F.getFunctionType()->isVarArg() || hasUnsupportedAttr(F) ||
        hasUnsupportedFGInfo(F) || MdUtils->findFunctionsInfoItem(&F) == MdUtils->end_FunctionsInfo()) {
      UnsupportedFns.insert(&F);
    }
  }
}

// Trace the uses of an alloca or a function arg to all args it reaches.
std::optional<SmallVector<Argument *, 8>>
traceUsesToArgs(Value *Root, const SmallPtrSetImpl<Function *> &UnsupportedFns,
                DenseMap<Argument *, std::optional<SmallVector<Argument *, 8>>> &ArgToArgCache, bool AllowMerges) {
  SmallSet<Argument *, 8> RootArgs;
  SmallVector<Value *, 8> Worklist;
  SmallPtrSet<Value *, 8> Seen;
  Worklist.push_back(Root);

  while (!Worklist.empty()) {
    llvm::Value *V = Worklist.pop_back_val();
    if (!Seen.insert(V).second) {
      continue;
    }
    for (User *U : V->users()) {
      if (auto *II = llvm::dyn_cast<llvm::IntrinsicInst>(U)) {
        llvm::Intrinsic::ID IID = II->getIntrinsicID();
        if (IID == llvm::Intrinsic::lifetime_start || IID == llvm::Intrinsic::lifetime_end || isDebugInst(II))
          continue;
      }
      if (llvm::isa<llvm::GetElementPtrInst, llvm::BitCastInst, llvm::AddrSpaceCastInst>(U)) {
        Worklist.push_back(U);
        continue;
      }
      if (PHINode *PN = dyn_cast<PHINode>(U)) {
        if (!AllowMerges && PN->getNumIncomingValues() != 1) {
          return std::nullopt;
        }
        Worklist.push_back(PN);
        continue;
      }
      if (AllowMerges && isa<SelectInst>(U)) {
        Worklist.push_back(U);
        continue;
      }
      if (llvm::isa<llvm::LoadInst>(U)) {
        continue;
      }
      if (auto *PLI = llvm::dyn_cast<llvm::PredicatedLoadIntrinsic>(U)) {
        if (PLI->getMergeValue() == V)
          return std::nullopt;
        continue;
      }
      if (auto *SI = llvm::dyn_cast<llvm::StoreInst>(U)) {
        if (SI->getValueOperand() == V) {
          return std::nullopt;
        }
        continue;
      }
      if (auto *CI = llvm::dyn_cast<llvm::CallInst>(U)) {
        auto *Callee = CI->getCalledFunction();
        if (!Callee || UnsupportedFns.contains(Callee) || !CI->getFunctionType() ||
            CI->getFunctionType() != Callee->getFunctionType())
          return std::nullopt;
        for (unsigned I = 0; I < CI->arg_size(); ++I) {
          if (CI->getArgOperand(I) != V) {
            continue;
          }
          auto *Arg = Callee->getArg(I);
          RootArgs.insert(Arg);

          // Use memoized result if available
          if (auto It = ArgToArgCache.find(Arg); It != ArgToArgCache.end()) {
            if (!It->second.has_value())
              return std::nullopt;
            RootArgs.insert(It->second->begin(), It->second->end());
            continue;
          }
          auto CalleeArgs = traceUsesToArgs(Arg, UnsupportedFns, ArgToArgCache, AllowMerges);
          ArgToArgCache[Arg] = CalleeArgs;
          if (!CalleeArgs.has_value())
            return std::nullopt;
          RootArgs.insert(CalleeArgs->begin(), CalleeArgs->end());
        }
        continue;
      }
      return std::nullopt;
    }
  }
  return SmallVector<Argument *, 8>(RootArgs.begin(), RootArgs.end());
}

// Only target large scalar array allocas that stand to benefit from SoA promotion.
// This avoids the IR explosion that would result from splitting every pointer merge.
bool isWorthwhileForAlloca(AllocaInst *AI, const DataLayout &DL) {
  IGC_ASSERT(AI != nullptr);
  Type *AllocTy = AI->getAllocatedType();
  // Only flat [N x T] of arithmetic T qualifies for the target pattern.
  auto *ArrTy = dyn_cast<ArrayType>(AllocTy);
  if (!ArrTy) {
    return false;
  }
  Type *ETy = ArrTy->getElementType();
  if (!ETy->isFloatingPointTy() && !ETy->isIntegerTy()) {
    return false;
  }
  uint64_t Bytes = DL.getTypeAllocSize(AllocTy).getFixedValue();
  uint64_t MinBytes = (uint64_t)IGC_GET_FLAG_VALUE(PHIOfAllocaPtrSplitMinSize);
  return Bytes >= MinBytes;
}

void collectRootArgs(Module &M, GenXFunctionGroupAnalysis *FGA, IGCMD::MetaDataUtils *MdUtils, bool VerifyAlloca,
                     DenseSet<const Value *> &RootArgs) {
  CallGraph CG(M);
  SmallPtrSet<Function *, 8> UnsupportedFns;
  collectSoAUnsupportedFns(M, CG, FGA, MdUtils, UnsupportedFns);
  const DataLayout &DL = M.getDataLayout();
  DenseMap<Argument *, std::optional<SmallVector<Argument *, 8>>> ArgToArgCache;
  for (Function &F : M) {
    if (UnsupportedFns.contains(&F)) {
      continue;
    }
    for (Instruction &I : F.getEntryBlock()) {
      auto *AI = dyn_cast<AllocaInst>(&I);
      if (!AI || (VerifyAlloca && !isWorthwhileForAlloca(AI, DL))) {
        continue;
      }
      if (auto Args = traceUsesToArgs(AI, UnsupportedFns, ArgToArgCache, true)) {
        RootArgs.insert(Args->begin(), Args->end());
      }
    }
  }
}

namespace {
// Returns Ty moved into address space AS if Ty is a pointer outside AS;
// otherwise (or if AS is unset) returns Ty unchanged.
Type *clampAddrSpace(Type *Ty, std::optional<unsigned> AS) {
  auto *PT = dyn_cast<PointerType>(Ty);
  if (!AS || !PT || PT->getAddressSpace() == *AS) {
    return Ty;
  }
  return IGCLLVM::PointerType::get(PT, *AS);
}

// Casts V to DestTy, or returns V if it already has that type. The opcode is
// derived from the types rather than copied from an original cast: after
// stripping and clamping, source and destination may share an address space,
// where an addrspacecast would be invalid.
Value *castTo(IRBuilderBase &B, Value *V, Type *DestTy) {
  if (V->getType() == DestTy) {
    return V;
  }
  return B.CreateCast(CastInst::getCastOpcode(V, false, DestTy, false), V, DestTy);
}

// Re-creates GEP on top of Base, preserving `inbounds`. Under typed pointers
// Base may no longer have the pointee type GEP indexes (its leading casts were
// stripped), so restore it first with a bitcast in Base's own address space.
Value *replayGEP(IRBuilderBase &B, Value *Base, GetElementPtrInst *GEP) {
  Type *SrcElemTy = GEP->getSourceElementType();
  auto *BasePtrTy = cast<PointerType>(Base->getType());
  if (!IGCLLVM::isOpaqueOrPointeeTypeMatches(BasePtrTy, SrcElemTy)) {
    Base = B.CreateBitCast(Base, IGCLLVM::PointerType::get(SrcElemTy, BasePtrTy->getAddressSpace()));
  }
  SmallVector<Value *, 4> Idxs(GEP->idx_begin(), GEP->idx_end());
  return GEP->isInBounds() ? B.CreateInBoundsGEP(SrcElemTy, Base, Idxs) : B.CreateGEP(SrcElemTy, Base, Idxs);
}
} // anonymous namespace

Value *replayAddressChain(IRBuilderBase &B, Value *Incoming, ArrayRef<Instruction *> Chain, const DataLayout &DL,
                          const DenseSet<const Value *> &ExtraRoots) {
  Value *Cur = Incoming;
  std::optional<unsigned> ClampAS;
  Value *Obj = IGCLLVM::getUnderlyingObject(Incoming, DL);
  if (isa<AllocaInst>(Obj) || ExtraRoots.contains(Obj)) {
    // Stripping bitcast/addrspacecast preserves the underlying object.
    while (isa<BitCastInst, AddrSpaceCastInst>(Cur)) {
      Cur = cast<Instruction>(Cur)->getOperand(0);
    }
    ClampAS = Cur->getType()->getPointerAddressSpace();
  }

  for (Instruction *I : Chain) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
      Cur = replayGEP(B, Cur, GEP);
    } else {
      Cur = castTo(B, Cur, clampAddrSpace(cast<CastInst>(I)->getType(), ClampAS));
    }
  }
  // Give the result the type the original chain produced (clamped). This only
  // matters under typed pointers, when stripping changed the pointee type and
  // the chain ends without a cast restoring it (e.g. an empty chain).
  Type *OrigTy = Chain.empty() ? Incoming->getType() : Chain.back()->getType();
  return castTo(B, Cur, clampAddrSpace(OrigTy, ClampAS));
}
} // namespace IGC::SoAAcrossFNUtil

/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/BdpasScaleCoalescing.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/VectorUtils.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/debug/DebugMacros.hpp"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "bdpas-scale-coalescing"

namespace {

// Four ordinary FP4 BDPAS calls often form a 2x2 scale rectangle:
//
//                       B0                  B1
//               +----------------+  +----------------+
//        A0     | C00: A0 + B0   |  | C01: A0 + B1   |
//               +----------------+  +----------------+
//        A1     | C10: A1 + B0   |  | C11: A1 + B1   |
//               +----------------+  +----------------+
//
// Ordinary lowering creates private strided scale operands for every call:
// two MOVs for A and two for B, or 16 MOVs for the rectangle. This pass builds
// one shared packed A region and one shared packed B region, then gives each
// call offsets 0 or 16 into those regions. Per-call striding then disappears.
// The generic region construction needs eight shared MOVs. When each scale is
// a bitcast from i16, byte-swizzle lowering can build both regions with four
// SIMD32 MOVs, two for each region:
//
//   mov (M1_NM, 32) packed_a(0,0)<1>:b  responses_a(0,0)<4;1,0>:b
//   mov (M1_NM, 32) packed_a(0,32)<1>:b responses_a(0,1)<4;1,0>:b
//   mov (M1_NM, 32) packed_b(0,0)<1>:b  responses_b(0,0)<4;1,0>:b
//   mov (M1_NM, 32) packed_b(0,32)<1>:b responses_b(0,1)<4;1,0>:b
//
using CallList = SmallVector<GenIntrinsicInst *, 4>;

struct CandidateIndex {
  DenseMap<Value *, CallList> ByA;
  DenseMap<Value *, CallList> ByB;
  DenseMap<Value *, DenseMap<Value *, CallList>> ByPair;
};

struct ScaleRectangle {
  GenIntrinsicInst *Calls[2][2] = {};
  Value *ScaleA[2] = {};
  Value *ScaleB[2] = {};
};

class BdpasScaleCoalescing : public FunctionPass {
public:
  static char ID;

  BdpasScaleCoalescing();

  StringRef getPassName() const override { return "BdpasScaleCoalescing"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<WIAnalysis>();
  }

  bool runOnFunction(Function &F) override;

private:
  CodeGenContext *Ctx = nullptr;
  DominatorTree *DT = nullptr;

  bool processBlock(BasicBlock &BB, const WIAnalysis &WI);
  bool isCandidate(GenIntrinsicInst *GII, const WIAnalysis &WI) const;
  bool findRectangle(GenIntrinsicInst *Seed, const CandidateIndex &Index, const SmallPtrSetImpl<Instruction *> &Used,
                     ScaleRectangle &Match) const;
  bool operandsDominate(const ScaleRectangle &Match, Instruction *InsertBefore) const;
  Value *buildRegion(IRBuilder<> &Builder, Value *First, Value *Second, StringRef Name) const;
  void rewrite(GenIntrinsicInst *Bdpas, Value *ScaleA, Value *ScaleB, unsigned AOffset, unsigned BOffset) const;
};

} // namespace

char BdpasScaleCoalescing::ID = 0;

#define PASS_FLAG "igc-bdpas-scale-coalescing"
#define PASS_DESCRIPTION "Coalesce FP4 BDPAS block scaling operands into shared packed regions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BdpasScaleCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(BdpasScaleCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

BdpasScaleCoalescing::BdpasScaleCoalescing() : FunctionPass(ID) {
  initializeBdpasScaleCoalescingPass(*PassRegistry::getPassRegistry());
}

FunctionPass *IGC::createBdpasScaleCoalescingPass() { return new BdpasScaleCoalescing(); }

bool BdpasScaleCoalescing::isCandidate(GenIntrinsicInst *GII, const WIAnalysis &WI) const {
  if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_sub_group_bdpas)
    return false;

  auto *PrecisionA = dyn_cast<ConstantInt>(GII->getArgOperand(5));
  auto *PrecisionB = dyn_cast<ConstantInt>(GII->getArgOperand(6));
  if (!PrecisionA || !PrecisionB)
    return false;
  if (PrecisionA->getZExtValue() != static_cast<uint64_t>(PrecisionType::E2M1) ||
      PrecisionB->getZExtValue() != static_cast<uint64_t>(PrecisionType::E2M1))
    return false;
  auto IsCanonicalScaleType = [](Value *V) {
    auto *Ty = dyn_cast<FixedVectorType>(V->getType());
    return Ty && Ty->getNumElements() == 2 && Ty->getElementType()->isIntegerTy(8);
  };
  Value *ScaleA = GII->getArgOperand(3);
  Value *ScaleB = GII->getArgOperand(4);
  return IsCanonicalScaleType(ScaleA) && IsCanonicalScaleType(ScaleB) && !WI.isUniform(ScaleA) && !WI.isUniform(ScaleB);
}

bool BdpasScaleCoalescing::findRectangle(GenIntrinsicInst *Seed, const CandidateIndex &Index,
                                         const SmallPtrSetImpl<Instruction *> &Used, ScaleRectangle &Match) const {
  GenIntrinsicInst *C00 = Seed;
  if (Used.contains(C00))
    return false;

  Value *A0 = C00->getArgOperand(3);
  Value *B0 = C00->getArgOperand(4);
  auto SameA = Index.ByA.find(A0);
  auto SameB = Index.ByB.find(B0);
  if (SameA == Index.ByA.end() || SameB == Index.ByB.end())
    return false;

  // Starting from C00=(A0,B0), walk three indexes to close the rectangle:
  //
  //   ByA[A0]       -> C01=(A0,B1)
  //   ByB[B0]       -> C10=(A1,B0)
  //   ByPair[A1][B1]-> C11=(A1,B1)
  //
  // Each loop rejects reused calls and equal scales before Match is written.
  // Therefore a true return always describes four distinct calls and two
  // distinct A and B scales; an incomplete candidate is skipped here.
  for (GenIntrinsicInst *C01 : SameA->second) {
    if (C01 == C00 || Used.contains(C01) || C01->getArgOperand(3) != A0 || C01->getArgOperand(4) == B0)
      continue;
    Value *B1 = C01->getArgOperand(4);
    for (GenIntrinsicInst *C10 : SameB->second) {
      if (C10 == C00 || C10 == C01 || Used.contains(C10) || C10->getArgOperand(3) == A0 || C10->getArgOperand(4) != B0)
        continue;
      Value *A1 = C10->getArgOperand(3);
      auto SameA1 = Index.ByPair.find(A1);
      if (SameA1 == Index.ByPair.end())
        continue;
      auto Pair = SameA1->second.find(B1);
      if (Pair == SameA1->second.end())
        continue;
      for (GenIntrinsicInst *C11 : Pair->second) {
        if (C11 == C00 || C11 == C01 || C11 == C10 || Used.contains(C11) || C11->getArgOperand(3) != A1 ||
            C11->getArgOperand(4) != B1)
          continue;

        Match.Calls[0][0] = C00;
        Match.Calls[0][1] = C01;
        Match.Calls[1][0] = C10;
        Match.Calls[1][1] = C11;
        Match.ScaleA[0] = A0;
        Match.ScaleA[1] = A1;
        Match.ScaleB[0] = B0;
        Match.ScaleB[1] = B1;
        return true;
      }
    }
  }
  return false;
}

bool BdpasScaleCoalescing::operandsDominate(const ScaleRectangle &Match, Instruction *InsertBefore) const {
  for (Value *Scale : {Match.ScaleA[0], Match.ScaleA[1], Match.ScaleB[0], Match.ScaleB[1]}) {
    if (auto *Def = dyn_cast<Instruction>(Scale)) {
      if (!DT->dominates(Def, InsertBefore))
        return false;
    }
  }
  return true;
}

Value *BdpasScaleCoalescing::buildRegion(IRBuilder<> &Builder, Value *First, Value *Second, StringRef Name) const {
  Type *I8 = Builder.getInt8Ty();
  auto *RegionTy = FixedVectorType::get(I8, 4);
  auto *FirstBitCast = dyn_cast<BitCastInst>(First);
  auto *SecondBitCast = dyn_cast<BitCastInst>(Second);
  // Prefer byte_swizzle when both <2 x i8> values are bitcasts of i16. Its
  // lowering can read the two bytes of each i16 directly. For eligible paired
  // d16u32 loads, <0,2,1,3> becomes two vISA MOVs per packed region:
  //
  //   mov (M1_NM, 32) region(0,0)<1>:b  responses(0,0)<4;1,0>:b
  //   mov (M1_NM, 32) region(0,32)<1>:b responses(0,1)<4;1,0>:b
  //
  // Other producers use the default extract/insert path below. It accepts any
  // canonical <2 x i8> values and preserves the same element order.
  if (FirstBitCast && SecondBitCast && FirstBitCast->getSrcTy()->isIntegerTy(16) &&
      SecondBitCast->getSrcTy()->isIntegerTy(16)) {
    Function *Swizzle =
        GenISAIntrinsic::getDeclaration(Builder.GetInsertBlock()->getModule(), GenISAIntrinsic::GenISA_byte_swizzle);
    Constant *Indices =
        ConstantVector::get({Builder.getInt32(0), Builder.getInt32(2), Builder.getInt32(1), Builder.getInt32(3)});
    return Builder.CreateCall(Swizzle, {FirstBitCast->getOperand(0), SecondBitCast->getOperand(0), Indices},
                              VALUE_NAME(Name + ".swizzle"));
  }

  auto GetElement = [&Builder, Name](Value *Vector, unsigned Index, StringRef Suffix) {
    if (Value *Element = llvm::findScalarElement(Vector, Index))
      return Element;
    return Builder.CreateExtractElement(Vector, Builder.getInt32(Index), VALUE_NAME(Name + Suffix));
  };
  Value *Elements[4] = {
      GetElement(First, 0, ".first.low"),
      GetElement(Second, 0, ".second.low"),
      GetElement(First, 1, ".first.high"),
      GetElement(Second, 1, ".second.high"),
  };

  Value *Region = PoisonValue::get(RegionTy);
  for (unsigned I = 0; I != 4; ++I)
    Region = Builder.CreateInsertElement(Region, Elements[I], Builder.getInt32(I), VALUE_NAME(Name + ".insert"));
  return Region;
}

void BdpasScaleCoalescing::rewrite(GenIntrinsicInst *Bdpas, Value *ScaleA, Value *ScaleB, unsigned AOffset,
                                   unsigned BOffset) const {
  SmallVector<Value *, 12> Args(Bdpas->args());
  Args[3] = ScaleA;
  Args[4] = ScaleB;
  Type *I32 = Type::getInt32Ty(Bdpas->getContext());
  Args.push_back(ConstantInt::get(I32, AOffset));
  Args.push_back(ConstantInt::get(I32, BOffset));

  Type *Overloads[6] = {Bdpas->getType(),   Args[0]->getType(), Args[1]->getType(),
                        Args[2]->getType(), Args[3]->getType(), Args[4]->getType()};
  Function *NewDeclaration =
      GenISAIntrinsic::getDeclaration(Bdpas->getModule(), GenISAIntrinsic::GenISA_sub_group_bdpas_packed, Overloads);
  CallInst *NewCall = CallInst::Create(NewDeclaration, Args, Bdpas->getName(), IGCLLVM::insertPosition(Bdpas));
  NewCall->setAttributes(Bdpas->getAttributes());
  NewCall->setCallingConv(Bdpas->getCallingConv());
  NewCall->setTailCallKind(Bdpas->getTailCallKind());
  NewCall->copyMetadata(*Bdpas);
  NewCall->setDebugLoc(Bdpas->getDebugLoc());
  Bdpas->replaceAllUsesWith(NewCall);
}

bool BdpasScaleCoalescing::processBlock(BasicBlock &BB, const WIAnalysis &WI) {
  SmallVector<GenIntrinsicInst *, 16> Calls;
  for (Instruction &I : BB) {
    auto *GII = dyn_cast<GenIntrinsicInst>(&I);
    if (isCandidate(GII, WI))
      Calls.push_back(GII);
  }
  LLVM_DEBUG(dbgs() << "bdpas-scale-coalescing: found " << Calls.size() << " candidates\n");

  CandidateIndex Index;
  for (GenIntrinsicInst *Call : Calls) {
    Value *ScaleA = Call->getArgOperand(3);
    Value *ScaleB = Call->getArgOperand(4);
    Index.ByA[ScaleA].push_back(Call);
    Index.ByB[ScaleB].push_back(Call);
    Index.ByPair[ScaleA][ScaleB].push_back(Call);
  }

  SmallPtrSet<Instruction *, 16> Used;
  SmallVector<Instruction *, 16> Dead;
  // A scale root can occur in more than one accepted rectangle. Recursive
  // deletion through the first occurrence can erase it before a later cleanup
  // entry is visited. WeakTrackingVH follows RAUW and becomes null when its
  // value is deleted, so repeated entries never leave dangling pointers.
  SmallVector<WeakTrackingVH, 16> ScaleRoots;
  bool Changed = false;
  for (GenIntrinsicInst *Call : Calls) {
    ScaleRectangle Match;
    if (!findRectangle(Call, Index, Used, Match))
      continue;

    Instruction *InsertBefore = Match.Calls[0][0];
    for (auto &Row : Match.Calls)
      for (GenIntrinsicInst *RectangleCall : Row)
        if (RectangleCall->comesBefore(InsertBefore))
          InsertBefore = RectangleCall;
    if (!operandsDominate(Match, InsertBefore))
      continue;

    IRBuilder<> Builder(InsertBefore);
    Builder.SetCurrentDebugLocation(InsertBefore->getDebugLoc());
    Value *PackedA = buildRegion(Builder, Match.ScaleA[0], Match.ScaleA[1], "bdpas.scale.a");
    Value *PackedB = buildRegion(Builder, Match.ScaleB[0], Match.ScaleB[1], "bdpas.scale.b");
    for (Value *Scale : {Match.ScaleA[0], Match.ScaleA[1], Match.ScaleB[0], Match.ScaleB[1]})
      if (auto *Root = dyn_cast<Instruction>(Scale))
        ScaleRoots.emplace_back(Root);
    for (unsigned A = 0; A != 2; ++A) {
      for (unsigned B = 0; B != 2; ++B) {
        GenIntrinsicInst *OldCall = Match.Calls[A][B];
        const unsigned AOffset = A * BdpasPackedScaleLayout::TileScaleBytes;
        const unsigned BOffset = B * BdpasPackedScaleLayout::TileScaleBytes;
        IGC_ASSERT_MESSAGE(AOffset + BdpasPackedScaleLayout::AReadExtent <= BdpasPackedScaleLayout::PackedRegionBytes &&
                               BOffset + BdpasPackedScaleLayout::BReadExtent <=
                                   BdpasPackedScaleLayout::PackedRegionBytes,
                           "Packed BDPAS scale offsets must stay inside the region");
        rewrite(OldCall, PackedA, PackedB, AOffset, BOffset);
        Used.insert(OldCall);
        Dead.push_back(OldCall);
      }
    }
    Changed = true;
  }

  for (Instruction *I : Dead)
    I->eraseFromParent();
  for (WeakTrackingVH &ScaleRoot : ScaleRoots) {
    auto *Root = dyn_cast_or_null<Instruction>(ScaleRoot);
    if (Root && isInstructionTriviallyDead(Root))
      RecursivelyDeleteTriviallyDeadInstructions(Root);
  }
  return Changed;
}

bool BdpasScaleCoalescing::runOnFunction(Function &F) {
  if (skipFunction(F)) {
    LLVM_DEBUG(dbgs() << "bdpas-scale-coalescing: skipped function " << F.getName() << "\n");
    return false;
  }

  Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto *ModuleMD = Ctx->getModuleMetaData();
  // Code generation chooses SIMD width for the function-group head. Restricting
  // reconstruction to entry functions prevents a callee-local requirement from
  // rewriting IR that may later be emitted for another SIMD width.
  if (!isEntryFunc(Ctx->getMetaDataUtils(), &F) && F.getCallingConv() != CallingConv::SPIR_KERNEL) {
    LLVM_DEBUG(dbgs() << "bdpas-scale-coalescing: non-entry function " << F.getName() << "\n");
    return false;
  }

  if (IGC::getSIMDSize(ModuleMD, &F) != 16) {
    LLVM_DEBUG(dbgs() << "bdpas-scale-coalescing: non-SIMD16 function " << F.getName() << "\n");
    return false;
  }

  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  const WIAnalysis &WI = getAnalysis<WIAnalysis>();
  bool Changed = false;
  for (BasicBlock &BB : F)
    Changed |= processBlock(BB, WI);
  return Changed;
}

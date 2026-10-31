/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ResourceLoopAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include "common/ResourceAddrSpace.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Debug.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;

char ResourceLoopAnalysis::ID = 0;

#define PASS_FLAG "resource-loop-analysis"
#define PASS_DESCRIPTION "Analyze the begin and end of a resource loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(ResourceLoopAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResourceLoopAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

llvm::FunctionPass *IGC::createResourceLoopAnalysisPass() { return new ResourceLoopAnalysis; }

ResourceLoopAnalysis::ResourceLoopAnalysis() : FunctionPass(ID) {
  initializeResourceLoopAnalysisPass(*PassRegistry::getPassRegistry());
}

static bool ValueOnlyUsedByEEI(Value *V) {
  for (Value::user_iterator UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
    ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(*UI);
    if (!EEI || (EEI->getOperand(0) != V) || !isa<ConstantInt>(EEI->getOperand(1))) {
      return false;
    }
  }
  return true;
}

// Instructions that can never sit inside a resource loop.
static bool isResourceLoopBarrier(const Instruction *Inst) {
  return isa<CallInst>(Inst) || Inst->isTerminator() || Inst->mayReadOrWriteMemory();
}

// An instruction that is not part of a send ends the resource loop that is being built. Shared by
// ResourceLoopAnalysis and ResourceLoopDecodeSink, so that the sink never builds a run the analysis would split.
static bool endsResourceLoop(const Instruction *Inst, WIAnalysis *WI) {
  // avoid uniform in ballot loop, and avoid flag modification
  return isResourceLoopBarrier(Inst) || WI->isUniform(Inst) || Inst->getType()->getScalarType()->isIntegerTy(1);
}

// Check if two resource values are semantically equivalent,
// even if they are different SSA values (e.g., after heap offset rewriting).
static bool isSameResource(Value *A, Value *B) {
  if (A == B)
    return true;
  if (!A || !B)
    return false;

  // Strip pointer casts and compare underlying values
  A = A->stripPointerCasts();
  B = B->stripPointerCasts();
  if (A == B)
    return true;

  // Compare inttoptr(add(..., heapBase)) patterns:
  // If both are inttoptr instructions with the same operand structure,
  // they refer to the same resource.
  auto *IA = dyn_cast<IntToPtrInst>(A);
  auto *IB = dyn_cast<IntToPtrInst>(B);
  if (IA && IB) {
    auto *AddA = dyn_cast<BinaryOperator>(IA->getOperand(0));
    auto *AddB = dyn_cast<BinaryOperator>(IB->getOperand(0));
    if (AddA && AddB && AddA->getOpcode() == Instruction::Add && AddB->getOpcode() == Instruction::Add) {
      // add is commutative - normalize: find the shared heap base operand
      // and compare the resource index operand.
      Value *ResA = nullptr, *ResB = nullptr;
      Value *BaseA = nullptr;

      // Try to identify which operand is the heap base (shared between both)
      // and which is the resource index.
      for (unsigned i = 0; i < 2; ++i) {
        for (unsigned j = 0; j < 2; ++j) {
          if (AddA->getOperand(i) == AddB->getOperand(j)) {
            BaseA = AddA->getOperand(i);
            ResA = AddA->getOperand(1 - i);
            ResB = AddB->getOperand(1 - j);
            break;
          }
        }
        if (BaseA)
          break;
      }

      if (BaseA && ResA && ResB) {
        // Direct match on the resource index operand
        if (ResA == ResB)
          return true;
        // Compare through ptrtoint wrappers
        auto *P2IA = dyn_cast<PtrToIntInst>(ResA);
        auto *P2IB = dyn_cast<PtrToIntInst>(ResB);
        if (P2IA && P2IB && P2IA->getOperand(0)->stripPointerCasts() == P2IB->getOperand(0)->stripPointerCasts())
          return true;
      }
    }
  }

  return false;
}

// Classify Inst as a lane-varying resource access that can be wrapped in a resource loop.
// Returns true and fills OpTy/Res/Samp if so.
static bool classifyLaneVaryingSend(Instruction *Inst, WIAnalysis *WI, unsigned &OpTy, Value *&Res, Value *&Samp) {
  OpTy = 0;
  Res = nullptr;
  Samp = nullptr;
  // There are more types of lane-varying resource access than what are
  // listed below. Limited the optimization to get most of performance and
  // to reduce debugging scope.
  if (auto *LI = dyn_cast<SamplerLoadIntrinsic>(Inst)) {
    if (!WI->isUniform(LI->getTextureValue())) {
      // need extra restrictions:
      // no half-type because it may need extra op for packing
      // no return of the number of elements >= 5, that is
      // sampler-feedback
      unsigned NumElt = 1;
      if (auto vecTy = dyn_cast<IGCLLVM::FixedVectorType>(LI->getType())) {
        NumElt = (unsigned)vecTy->getNumElements();
      }
      if (LI->getType()->getScalarSizeInBits() >= 32 && NumElt <= 4) {
        Res = LI->getTextureValue();
        OpTy = 3;
      }
    }
  } else if (auto *LI = dyn_cast<LdRawIntrinsic>(Inst)) {
    if (!WI->isUniform(LI->getResourceValue())) {
      // need extra restrictions:
      // no half-type because it may need extra op for packing
      if (LI->getType()->getScalarSizeInBits() >= 32) {
        Res = LI->getResourceValue();
        OpTy = 4;
      }
    }
  }
  return (Res || Samp) && OpTy;
}

// The limits on how many sends ResourceLoopAnalysis fuses into one resource loop. Shared with ResourceLoopDecodeSink,
// so that the sink never prepares a run longer than the analysis accepts. Returns false if this shader is not fused at
// all.
static bool getResourceLoopFusionLimits(CodeGenContext *Ctx, unsigned &MaxSends, unsigned &MaxALU, unsigned &MinSize) {
  if (IGC_IS_FLAG_DISABLED(FuseResourceLoop) || !Ctx->platform.allowFuseResourceLoop(MaxSends, MaxALU, MinSize)) {
    // not supported or no fuse resource loop needed
    return false;
  }
  // When CS is retried, the previous compilation may have spilled due to register pressure.
  // In that case, we should not fuse resource loops because it may make register pressure worse and cause more spills.
  if (Ctx->type == ShaderType::COMPUTE_SHADER && !Ctx->m_retryManager->IsFirstTry()) {
    return false;
  }
  // RAYTRACING_SHADER: at most 2 instructions between two sends of a loop
  if (Ctx->type == ShaderType::RAYTRACING_SHADER) {
    MaxALU = 2;
  }
  return true;
}

bool ResourceLoopAnalysis::runOnFunction(Function &F) {
  auto WI = &(getAnalysis<WIAnalysis>());
  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  LoopMap.clear();
  MergedInstsMap.clear();

  // should return false if not supported or no fuse resource loop needed
  if (!getResourceLoopFusionLimits(CTX, MaxResourceLoopSize, MaxALUBetweenMemOps, MinFuseResourceLoopSize))
    return false;
  for (BasicBlock &BBRef : F) {
    Instruction *currentLoopStart = nullptr;
    BasicBlock *BB = &BBRef;
    // give every instruction a seq-no in order to check the location of uses
    llvm::DenseMap<Instruction *, unsigned> InstOrder;
    unsigned SeqNo = 0;
    for (auto &Inst : *BB) {
      InstOrder[&Inst] = SeqNo++;
    }
    // find and mark resource-loops
    unsigned loopOpTy = 0;
    Value *loopRes = nullptr;
    Value *loopSamp = nullptr;
    auto prevMemIter = BB->end();   // last memory-inst in the loop
    SmallPtrSet<Value *, 8> DefSet; // all memory-inst in the loop
    SmallPtrSet<Value *, 8> DefOnly4EEI;
    auto closeResourceLoop = [&]() {
      if (prevMemIter != BB->end()) {
        auto PI = &*prevMemIter;
        IGC_ASSERT(LoopMap.find(PI) != LoopMap.end());

        bool tooSmall = !currentLoopStart;
        if (!tooSmall) {
          auto mergedIt = MergedInstsMap.find(currentLoopStart);
          const size_t mergedCount = (mergedIt != MergedInstsMap.end()) ? mergedIt->second.size() : 0;
          const size_t totalSendCount = 1 + mergedCount; // include loop-start send
          const bool prepared = currentLoopStart->getMetadata(ResourceLoopPreparedRunMD) != nullptr;
          tooSmall = (totalSendCount < (prepared ? std::min(MinFuseResourceLoopSize, 2u) : MinFuseResourceLoopSize));
        }

        if (tooSmall) {
          // Walk from currentLoopStart to prevMemIter and erase all markers
          if (currentLoopStart) {
            auto III = BasicBlock::iterator(currentLoopStart);
            auto endIter = std::next(prevMemIter);
            while (III != endIter) {
              LoopMap.erase(&*III);
              ++III;
            }
            MergedInstsMap.erase(currentLoopStart);
          }
        } else {
          LoopMap[PI] |= MarkResourceLoopEnd;
        }
        prevMemIter = BB->end();
        loopRes = nullptr;
        loopSamp = nullptr;
        loopOpTy = 0;
        DefSet.clear();
        DefOnly4EEI.clear();
        currentLoopStart = nullptr;
      }
    };
    for (BasicBlock::iterator II = BB->begin(), EI = BB->end(); II != EI; ++II) {
      Instruction *I = &*II;
      unsigned curOpTy = 0;
      Value *curRes = nullptr;
      Value *curSamp = nullptr;
      classifyLaneVaryingSend(I, WI, curOpTy, curRes, curSamp);
      // check data-dependence from mem-ops in the current set
      bool HasDeps = false;
      for (Value *opnd : I->operands()) {
        if (DefSet.count(opnd)) {
          // special handling on extract-element in the loop
          bool SkipEEI = false;
          if (auto EEI = dyn_cast<ExtractElementInst>(I)) {
            if (opnd == EEI->getVectorOperand()) {
              if (DefOnly4EEI.count(opnd))
                SkipEEI = true;
              else if (ValueOnlyUsedByEEI(opnd)) {
                DefOnly4EEI.insert(opnd);
                SkipEEI = true;
              }
            }
          }
          if (SkipEEI)
            DefSet.insert(I); // add EEI to the def-set
          else {
            HasDeps = true;
            break;
          }
        }
      }
      if ((curRes || curSamp) && curOpTy) {
        // this is a lane-varying-resource-access
        bool LoopEnd = HasDeps;
        if (!LoopEnd && curOpTy == loopOpTy && isSameResource(curRes, loopRes) &&
            (curSamp == loopSamp || isSameResource(curSamp, loopSamp))) {
          // need to check ALU instruction in between
          // all those instructions should only be used
          // inside the loop
          IGC_ASSERT(prevMemIter != BB->end());
          auto III = prevMemIter;
          unsigned aluInstCount = 0;
          ++III;
          unsigned curInstOrder = InstOrder[I];
          while (!LoopEnd && III != II) {
            auto defInst = &*III;
            if (isa<ExtractElementInst>(defInst) && DefSet.count(defInst)) {
              ++III;
              aluInstCount++;
              continue;
            }
            unsigned defInstOrder = InstOrder[defInst];
            for (auto UI = defInst->user_begin(), UE = defInst->user_end(); !LoopEnd && UI != UE; ++UI) {
              // Determine the block of the use.
              Instruction *useInst = cast<Instruction>(*UI);
              auto it = InstOrder.find(useInst);
              if (it == InstOrder.end())
                LoopEnd = true;
              else if (it->second > curInstOrder || it->second <= defInstOrder)
                LoopEnd = true;
            }
            ++III;
            aluInstCount++;
          }

          // Compute the prospective number of fused resource-access sends if we
          // were to include the current memory operation I in the loop:
          //   1 for the loop-start send
          // + already merged sends in MergedInstsMap[currentLoopStart]
          // + 1 for the current candidate send.
          unsigned fusedSendCount = 1;
          auto MergedIt = MergedInstsMap.find(currentLoopStart);
          if (MergedIt != MergedInstsMap.end())
            fusedSendCount += static_cast<unsigned>(MergedIt->second.size());
          unsigned prospectiveSendCount = fusedSendCount + 1;
          if (!LoopEnd && prospectiveSendCount > MaxResourceLoopSize) {
            LoopEnd = true;
          }
          if (!LoopEnd && aluInstCount > MaxALUBetweenMemOps) {
            LoopEnd = true;
          }
          // Check for interleaved EEI+ALU pattern between sends that causes
          // excessive register pressure. The pattern is:
          //   %load = loads(...)
          //   %eei  = extractelement %load,...  <== EEI from DefSet
          //   %alu  = add/mul/...               <== non-EEI ALU after EEI
          // When an EEI appears between sends followed by additional ALU,
          // both the full vector result and extracted scalar remain live
          // across the ALU and subsequent sends, increasing register pressure.
          if (!LoopEnd && !DefOnly4EEI.empty()) {
            auto scanIter = prevMemIter;
            ++scanIter;
            bool sawEEI = false;
            while (!LoopEnd && scanIter != II) {
              auto *scanInst = &*scanIter;
              if (isa<ExtractElementInst>(scanInst) && DefSet.count(scanInst)) {
                sawEEI = true;
              } else if (sawEEI) {
                // Non-EEI instruction after an in-loop EEI between sends:
                // this creates the interleaved pattern that inflates liveness.
                LoopEnd = true;
              }
              ++scanIter;
            }
          }
        } else {
          LoopEnd = true; // mismatch resource/sampler/op-type
        }

        if (!LoopEnd) {
          if (prevMemIter != BB->end()) {
            // mark instructions in between two mem-op as inside
            auto III = prevMemIter;
            ++III;
            while (III != II) {
              auto InBetween = &*III;
              LoopMap[InBetween] = MarkResourceLoopInside;
              ++III;
            }
          }
          LoopMap[I] = MarkResourceLoopInside;
          prevMemIter = II;
          DefSet.insert(I);
          MergedInstsMap[currentLoopStart].push_back(I);
        } else {
          // mark the end of the previous loop
          if (prevMemIter != BB->end()) {
            auto PI = &*prevMemIter;
            IGC_ASSERT(LoopMap.find(PI) != LoopMap.end());
            LoopMap[PI] |= MarkResourceLoopEnd;
          }
          LoopMap[I] = MarkResourceLoopStart;
          currentLoopStart = I;
          loopRes = curRes;
          loopSamp = curSamp;
          loopOpTy = curOpTy;
          prevMemIter = II;
          DefSet.clear();
          DefOnly4EEI.clear();
          DefSet.insert(I);
        }
      } else {
        // the other instructions
        bool LoopEnds = HasDeps || endsResourceLoop(I, WI);

        if (LoopEnds) {
          closeResourceLoop();
        }
      }
    }
    // Close any open resource loop at the end of the basic block
    closeResourceLoop();
  }

  if (IGC_IS_FLAG_ENABLED(DumpResourceLoop)) {
    auto name = Debug::DumpName(Debug::GetShaderOutputName())
                    .Hash(CTX->hash)
                    .Type(CTX->type)
                    .Pass("ResourceLoop")
                    .PostFix(F.getName().str())
                    .Extension("txt");
    printResourceLoops(Debug::Dump(name, Debug::DumpType::DBG_MSG_TEXT).stream(), &F);
  }
  // RLA is an analysis pass, it should return false.
  return false;
}

void ResourceLoopAnalysis::printResourceLoops(raw_ostream &OS, Function *F) const {
  if (F) {
    // All instructions
    for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
      Instruction *I = &*II;
      if (LoopMap.find(I) != LoopMap.end()) {
        unsigned marker = LoopMap.find(I)->second;
        OS << "  [";
        if (marker & MarkResourceLoopStart)
          OS << "S";
        if (marker & MarkResourceLoopInside)
          OS << "I";
        if (marker & MarkResourceLoopEnd)
          OS << "E";
        OS << "]  " << *I << "\n";
      }
    }
  }
  OS << "\n";
}

namespace {
// See createResourceLoopDecodeSinkPass in the header.
class ResourceLoopDecodeSink : public FunctionPass {
public:
  static char ID;
  ResourceLoopDecodeSink() : FunctionPass(ID) {
    initializeResourceLoopDecodeSinkPass(*PassRegistry::getPassRegistry());
  }
  StringRef getPassName() const override { return "Resource Loop Decode Sink"; }
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG(); // instructions only move within their block
    AU.addRequired<WIAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
  }

private:
  bool processBlock(BasicBlock &BB, WIAnalysis *WI, unsigned MaxSends, unsigned MaxALU);
};
} // namespace

char ResourceLoopDecodeSink::ID = 0;

#define PASS_FLAG "resource-loop-decode-sink"
#define PASS_DESCRIPTION "Sink load decode code to let same-resource sends share a resource loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResourceLoopDecodeSink, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResourceLoopDecodeSink, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

llvm::FunctionPass *IGC::createResourceLoopDecodeSinkPass() { return new ResourceLoopDecodeSink; }

bool ResourceLoopDecodeSink::runOnFunction(Function &F) {
  auto *WI = &getAnalysis<WIAnalysis>();
  auto *CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  unsigned MaxSends = 0, MaxALU = 0, MinSize = 0;
  if (!getResourceLoopFusionLimits(CTX, MaxSends, MaxALU, MinSize))
    return false;

  bool Changed = false;
  for (BasicBlock &BB : F)
    Changed |= processBlock(BB, WI, MaxSends, MaxALU);
  return Changed;
}

bool ResourceLoopDecodeSink::processBlock(BasicBlock &BB, WIAnalysis *WI, unsigned MaxSends, unsigned MaxALU) {
  if (MaxSends < 2)
    return false;

  // sequence number of every instruction, to check the position of uses
  DenseMap<Instruction *, unsigned> InstOrder;
  auto renumber = [&]() {
    InstOrder.clear();
    unsigned SeqNo = 0;
    for (auto &I : BB)
      InstOrder[&I] = SeqNo++;
  };
  renumber();

  // The search for the next send of a run stops after this many instructions since the previous send of the run, which
  // keeps the pass cheap on long blocks.
  constexpr unsigned MaxScanSinceSend = 128;

  bool Changed = false;
  for (auto It = BB.begin(); It != BB.end(); ++It) {
    unsigned OpTy = 0;
    Value *Res = nullptr, *Samp = nullptr;
    if (!classifyLaneVaryingSend(&*It, WI, OpTy, Res, Samp))
      continue;

    SmallVector<Instruction *, 4> Sends{&*It};
    SmallPtrSet<Value *, 16> Dependent; // results of the sends and of the code that consumes them
    Dependent.insert(&*It);
    SmallVector<Instruction *, 16> Decode;  // code that consumes the sends, in program order
    SmallVector<Instruction *, 16> Between; // all other code between the sends, in program order

    // Instructions looked at since the first send of this run or the last send that joined it. It belongs to this
    // run only: the outer loop steps over instructions that are not sends without scanning.
    unsigned Scanned = 0;
    for (auto Scan = std::next(It); Scan != BB.end() && Scanned++ < MaxScanSinceSend; ++Scan) {
      Instruction *J = &*Scan;
      unsigned JTy = 0;
      Value *JRes = nullptr, *JSamp = nullptr;
      bool IsSend = classifyLaneVaryingSend(J, WI, JTy, JRes, JSamp);
      bool DependsOnSends = any_of(J->operands(), [&](Value *Op) { return Dependent.contains(Op); });

      if (IsSend) {
        if (JTy != OpTy || !isSameResource(JRes, Res) || !(JSamp == Samp || isSameResource(JSamp, Samp)) ||
            DependsOnSends)
          break;
        Sends.push_back(J);
        Dependent.insert(J);
        Scanned = 0;
        if (Sends.size() == MaxSends)
          break;
        continue;
      }

      // anything that could not be moved down or reordered ends the run
      if (isResourceLoopBarrier(J) || isa<PHINode>(J))
        break;
      if (DependsOnSends) {
        Decode.push_back(J);
        Dependent.insert(J);
      } else {
        Between.push_back(J);
      }
    }

    // Whether the first Count sends can be fused by ResourceLoopAnalysis once the code that does not have to stay
    // between them is moved below the last one. The code that the later sends need (their operands, transitively)
    // has to stay, and then it has to satisfy the rules ResourceLoopAnalysis applies to the code inside a loop.
    // Everything else, whether it consumes a send or not, moves.
    SmallPtrSet<Instruction *, 16> Needed;
    auto fusable = [&](unsigned Count) {
      Needed.clear();
      Instruction *Last = Sends[Count - 1];
      SmallPtrSet<Instruction *, 16> InBetween(Between.begin(), Between.end());
      SmallVector<Instruction *, 8> Work;
      auto addNeeded = [&](Value *V) {
        auto *Inst = dyn_cast<Instruction>(V);
        if (Inst && InBetween.contains(Inst) && InstOrder[Inst] < InstOrder[Last] && Needed.insert(Inst).second)
          Work.push_back(Inst);
      };
      for (unsigned S = 1; S < Count; ++S)
        for (Value *Op : Sends[S]->operands())
          addNeeded(Op);
      while (!Work.empty()) {
        Instruction *Inst = Work.pop_back_val();
        for (Value *Op : Inst->operands())
          addNeeded(Op);
      }

      for (unsigned S = 0; S + 1 < Count; ++S) {
        unsigned Prev = InstOrder[Sends[S]], Next = InstOrder[Sends[S + 1]];
        unsigned InGap = 0;
        for (Instruction *K : Needed) {
          unsigned Pos = InstOrder[K];
          if (Pos <= Prev || Pos >= Next)
            continue;
          if (++InGap > MaxALU || endsResourceLoop(K, WI))
            return false;
          // like ResourceLoopAnalysis: the code between two sends is used only up to the send that follows it
          for (User *U : K->users()) {
            auto *UI = cast<Instruction>(U);
            auto UIt = InstOrder.find(UI);
            if (UIt == InstOrder.end() || UIt->second > Next || UIt->second <= Pos ||
                !(UI == Sends[S + 1] || Needed.contains(UI)))
              return false;
          }
        }
      }
      return true;
    };

    unsigned Count = Sends.size();
    while (Count >= 2 && !fusable(Count))
      --Count;
    if (Count < 2)
      continue;
    Sends.resize(Count);
    // the successful call of fusable above left the code that has to stay between the sends in Needed

    // Move the code that does not have to stay between the sends below the last send, keeping its order.
    Instruction *Last = Sends.back();
    SmallVector<Instruction *, 16> ToMove;
    for (Instruction *D : Decode)
      if (InstOrder[D] < InstOrder[Last])
        ToMove.push_back(D);
    for (Instruction *K : Between)
      if (InstOrder[K] < InstOrder[Last] && !Needed.contains(K))
        ToMove.push_back(K);
    llvm::sort(ToMove, [&](Instruction *A, Instruction *B) { return InstOrder[A] < InstOrder[B]; });

    if (!ToMove.empty()) {
      Instruction *InsertPt = Last->getNextNode();
      for (Instruction *M : ToMove)
        IGCLLVM::moveBefore(M, InsertPt);
      Changed = true;
      renumber();
      // Tell ResourceLoopAnalysis that this run is prepared, so FuseResourceLoopMinSize does not discard it.
      // A run that was already adjacent is left alone: FuseResourceLoopMinSize was tuned for those.
      for (Instruction *S : Sends)
        S->setMetadata(ResourceLoopPreparedRunMD, MDNode::get(BB.getContext(), {}));
    }
    // continue after the last send of the run
    It = Last->getIterator();
  }
  return Changed;
}

namespace {
// See createResourceLoopSamplerSelectPass in the header.
class ResourceLoopSamplerSelect : public FunctionPass {
public:
  static char ID;
  ResourceLoopSamplerSelect() : FunctionPass(ID) {
    initializeResourceLoopSamplerSelectPass(*PassRegistry::getPassRegistry());
  }
  StringRef getPassName() const override { return "Resource Loop Sampler Select"; }
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG(); // samples move into the dominating block, no block is added or removed
    AU.addRequired<WIAnalysis>();
  }

private:
  bool processBranch(IGCLLVM::CondBrInst &Br, WIAnalysis *WI);
};
} // namespace

char ResourceLoopSamplerSelect::ID = 0;

#define PASS_FLAG "resource-loop-sampler-select"
#define PASS_DESCRIPTION "Merge if/else samples that differ only in the sampler into one sample with a sampler select"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResourceLoopSamplerSelect, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(ResourceLoopSamplerSelect, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

llvm::FunctionPass *IGC::createResourceLoopSamplerSelectPass() { return new ResourceLoopSamplerSelect; }

bool ResourceLoopSamplerSelect::runOnFunction(Function &F) {
  auto *WI = &getAnalysis<WIAnalysis>();
  bool Changed = false;
  for (BasicBlock &BB : F) {
    if (auto *Br = dyn_cast<IGCLLVM::CondBrInst>(BB.getTerminator()))
      Changed |= processBranch(*Br, WI);
  }
  return Changed;
}

// Only samples with an explicit LOD or gradient: they do not depend on the neighbouring lanes, so executing them for
// the lanes of both arms cannot change a result.
static bool isExplicitLodSample(const Instruction *Inst) {
  auto *GII = dyn_cast<GenIntrinsicInst>(Inst);
  if (!GII)
    return false;
  switch (GII->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_sampleLptr:
  case GenISAIntrinsic::GenISA_sampleLCptr:
  case GenISAIntrinsic::GenISA_sampleDptr:
  case GenISAIntrinsic::GenISA_sampleDCptr:
    return true;
  default:
    return false;
  }
}

// An arm of a diamond whose only instructions that touch memory are explicit-LOD samples. Collects those samples.
static bool collectArmSamples(BasicBlock *Arm, BasicBlock *Head, BasicBlock *Join,
                              SmallVectorImpl<SampleIntrinsic *> &Samples) {
  if (Arm->getSinglePredecessor() != Head)
    return false;
  Instruction *Term = Arm->getTerminator();
  if (!isa<IGCLLVM::UncondBrInst>(Term) || Term->getSuccessor(0) != Join)
    return false;
  for (Instruction &I : *Arm) {
    if (I.isTerminator())
      break;
    if (isExplicitLodSample(&I))
      Samples.push_back(cast<SampleIntrinsic>(&I));
    else if (I.mayHaveSideEffects() || I.mayReadFromMemory())
      return false;
  }
  return !Samples.empty();
}

// The i64 address of a sampler pointer. Samplers are an inttoptr of a computed address, use that address directly.
static Value *getSamplerAddress(Value *Sampler, IRBuilder<> &B) {
  if (auto *ITP = dyn_cast<IntToPtrInst>(Sampler))
    if (ITP->getOperand(0)->getType()->isIntegerTy(64))
      return ITP->getOperand(0);
  return B.CreatePtrToInt(Sampler, B.getInt64Ty());
}

// True if the two samplers can be told apart by their address only: both are indirect (or bindless, where the address
// is the pointer value) and their address spaces differ in no more than the bufId tag, which identifies the access.
// A direct sampler keeps its index in the address space, so it cannot be selected per lane.
static bool areInterchangeableSamplers(Value *A, Value *B) {
  auto *PA = dyn_cast<PointerType>(A->getType());
  auto *PB = dyn_cast<PointerType>(B->getType());
  if (!PA || !PB)
    return false;
  unsigned ASA = PA->getAddressSpace(), ASB = PB->getAddressSpace();
  bool DirectA = false, DirectB = false;
  unsigned BufIdA = 0, BufIdB = 0;
  BufferType TypeA = DecodeAS4GFXResource(ASA, DirectA, BufIdA);
  BufferType TypeB = DecodeAS4GFXResource(ASB, DirectB, BufIdB);
  if (TypeA != TypeB || (TypeA != BINDLESS_SAMPLER && (DirectA || DirectB)))
    return false;
  GFXResourceAddrSpace MaskedA, MaskedB;
  MaskedA.u32Val = ASA;
  MaskedB.u32Val = ASB;
  if (!DirectA) // for an indirect sampler the bufId only tags the access
    MaskedA.bits.bufId = MaskedB.bits.bufId = 0;
  return MaskedA.u32Val == MaskedB.u32Val;
}

// The longest chain of pure instructions in an arm that is compared or hoisted with a sample (descriptor address
// computation: zext, add, add, inttoptr).
constexpr unsigned MaxArmChainDepth = 6;

// True if V is defined outside of the arm, or by pure code of the arm that can be hoisted to the head.
static bool isHoistable(Value *V, BasicBlock *Arm, unsigned Depth = 0) {
  auto *I = dyn_cast<Instruction>(V);
  if (!I || I->getParent() != Arm)
    return true;
  if (Depth > MaxArmChainDepth || isa<PHINode>(I) || isa<CallInst>(I) || !isSafeToSpeculativelyExecute(I))
    return false;
  return all_of(I->operands(), [&](Value *Op) { return isHoistable(Op, Arm, Depth + 1); });
}

// Moves the pure code of the arm that V depends on, then V, before InsertPt. It now runs for the lanes of both arms, so
// it does not keep the source location of one of them.
static void hoistInto(Value *V, BasicBlock *Arm, Instruction *InsertPt) {
  auto *I = dyn_cast<Instruction>(V);
  if (!I || I->getParent() != Arm)
    return;
  for (Value *Op : I->operands())
    hoistInto(Op, Arm, InsertPt);
  IGCLLVM::moveBefore(I, InsertPt);
  I->dropLocation();
}

// True if A and B are the same value, or the same computation (with the same nsw/nuw/exact/fast-math flags, as only the
// then arm's copy is kept) in the then and else arm. Hoisting one copy then makes the other one dead, which the later
// passes (no CSE across the arms) cannot do.
static bool isSameComputation(Value *A, Value *B, BasicBlock *ThenBB, BasicBlock *ElseBB, unsigned Depth = 0) {
  if (A == B)
    return true;
  auto *IA = dyn_cast<Instruction>(A);
  auto *IB = dyn_cast<Instruction>(B);
  if (!IA || !IB || IA->getParent() != ThenBB || IB->getParent() != ElseBB || Depth > MaxArmChainDepth ||
      !IA->isSameOperationAs(IB) || isa<PHINode>(IA) || isa<CallInst>(IA))
    return false;
  // the nsw/nuw/exact/fast-math flags are not part of isSameOperationAs
  if (IA->getRawSubclassOptionalData() != IB->getRawSubclassOptionalData())
    return false;
  for (unsigned Op = 0; Op < IA->getNumOperands(); ++Op)
    if (!isSameComputation(IA->getOperand(Op), IB->getOperand(Op), ThenBB, ElseBB, Depth + 1))
      return false;
  return true;
}

bool ResourceLoopSamplerSelect::processBranch(IGCLLVM::CondBrInst &Br, WIAnalysis *WI) {
  BasicBlock *Head = Br.getParent();
  BasicBlock *ThenBB = Br.getSuccessor(0);
  BasicBlock *ElseBB = Br.getSuccessor(1);
  if (ThenBB == ElseBB)
    return false;
  Instruction *ThenTerm = ThenBB->getTerminator();
  if (!isa<IGCLLVM::UncondBrInst>(ThenTerm))
    return false;
  BasicBlock *Join = ThenTerm->getSuccessor(0);

  SmallVector<SampleIntrinsic *, 4> ThenSamples, ElseSamples;
  if (!collectArmSamples(ThenBB, Head, Join, ThenSamples) || !collectArmSamples(ElseBB, Head, Join, ElseSamples) ||
      ThenSamples.size() != ElseSamples.size())
    return false;

  // Pair the samples in order: same intrinsic, the same computation of every operand except the sampler, and a
  // non-uniform texture, so a resource loop is needed anyway and the per-lane sampler adds none. The code in the arms
  // that the merged sample needs must be pure, to move it to the head.
  for (unsigned I = 0; I < ThenSamples.size(); ++I) {
    SampleIntrinsic *T = ThenSamples[I], *E = ElseSamples[I];
    if (T->getIntrinsicID() != E->getIntrinsicID() || T->getType() != E->getType() ||
        T->getNumOperands() != E->getNumOperands())
      return false;
    unsigned SamplerIdx = T->getSamplerIndex();
    // the callee is not compared: it is overloaded on the sampler type
    for (unsigned Op = 0; Op < IGCLLVM::getNumArgOperands(T); ++Op) {
      if (Op != SamplerIdx && !isSameComputation(T->getOperand(Op), E->getOperand(Op), ThenBB, ElseBB))
        return false;
      if (!isHoistable(T->getOperand(Op), ThenBB))
        return false;
    }
    // Identical samplers: nothing to select, leave the pair alone. Different samplers must also be interchangeable.
    if (T->getSamplerValue() == E->getSamplerValue() ||
        !areInterchangeableSamplers(T->getSamplerValue(), E->getSamplerValue()) ||
        WI->isUniform(T->getTextureValue()) || !isHoistable(E->getSamplerValue(), ElseBB))
      return false;
  }

  for (unsigned I = 0; I < ThenSamples.size(); ++I) {
    SampleIntrinsic *T = ThenSamples[I], *E = ElseSamples[I];
    for (unsigned Op = 0; Op < IGCLLVM::getNumArgOperands(T); ++Op)
      hoistInto(T->getOperand(Op), ThenBB, &Br);
    hoistInto(E->getSamplerValue(), ElseBB, &Br);
    IRBuilder<> B(&Br);
    Value *Addr = B.CreateSelect(Br.getCondition(), getSamplerAddress(T->getSamplerValue(), B),
                                 getSamplerAddress(E->getSamplerValue(), B), "samplerSelect");
    // The indirect sampler address spaces of the two arms only tell the accesses apart: keep the one of the then arm.
    Value *Sampler = B.CreateIntToPtr(Addr, T->getSamplerValue()->getType());
    auto *Merged = cast<CallInst>(T->clone());
    Merged->setOperand(T->getSamplerIndex(), Sampler);
    IGCLLVM::insertBefore(Merged, &Br);
    Merged->dropLocation(); // it stands for the sample of both arms
    Merged->takeName(T);
    T->replaceAllUsesWith(Merged);
    E->replaceAllUsesWith(Merged);
    // Weak handles: deleting one candidate can free another one (a repeated operand, or an operand chain).
    SmallVector<WeakTrackingVH, 16> DeadCandidates(T->op_begin(), T->op_end());
    DeadCandidates.append(E->op_begin(), E->op_end());
    T->eraseFromParent();
    E->eraseFromParent();
    // the else arm's copies of the hoisted computation, and the then arm's sampler, are dead now
    RecursivelyDeleteTriviallyDeadInstructionsPermissive(DeadCandidates);
  }
  return true;
}

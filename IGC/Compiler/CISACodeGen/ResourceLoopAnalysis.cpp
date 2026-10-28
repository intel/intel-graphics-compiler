/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ResourceLoopAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
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

  bool Changed = false;
  for (auto It = BB.begin(); It != BB.end(); ++It) {
    unsigned OpTy = 0;
    Value *Res = nullptr, *Samp = nullptr;
    if (!classifyLaneVaryingSend(&*It, WI, OpTy, Res, Samp))
      continue;

    SmallVector<Instruction *, 4> Sends{&*It};
    SmallPtrSet<Value *, 16> Dependent; // results of the sends and of the code that consumes them
    Dependent.insert(&*It);
    SmallVector<Instruction *, 16> Decode;     // dependent code found so far, in program order
    SmallVector<Instruction *, 4> Independent; // code since the last send that stays in the loop

    for (auto Scan = std::next(It); Scan != BB.end(); ++Scan) {
      Instruction *J = &*Scan;
      unsigned JTy = 0;
      Value *JRes = nullptr, *JSamp = nullptr;
      bool IsSend = classifyLaneVaryingSend(J, WI, JTy, JRes, JSamp);
      bool DependsOnSends = any_of(J->operands(), [&](Value *Op) { return Dependent.contains(Op); });

      if (IsSend) {
        if (JTy != OpTy || !isSameResource(JRes, Res) || !(JSamp == Samp || isSameResource(JSamp, Samp)) ||
            DependsOnSends)
          break;
        // The code staying between two sends has to be used only up to the send
        // that follows it, as in ResourceLoopAnalysis.
        bool UsesOk = true;
        for (Instruction *K : Independent) {
          for (User *U : K->users()) {
            auto UIt = InstOrder.find(cast<Instruction>(U));
            if (UIt == InstOrder.end() || UIt->second > InstOrder[J] || UIt->second <= InstOrder[K] ||
                Dependent.contains(U))
              UsesOk = false;
          }
        }
        if (!UsesOk)
          break;
        Sends.push_back(J);
        Dependent.insert(J);
        Independent.clear();
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
        // ResourceLoopAnalysis would end the loop here, so the run could not be fused.
        if (endsResourceLoop(J, WI))
          break;
        Independent.push_back(J);
        if (Independent.size() > MaxALU)
          break;
      }
    }

    if (Sends.size() < 2)
      continue;

    // Only the decode code that sits before the last send has to move.
    Instruction *Last = Sends.back();
    Instruction *InsertPt = Last->getNextNode();
    bool Moved = false;
    for (Instruction *D : Decode) {
      if (InstOrder[D] > InstOrder[Last])
        break;
      IGCLLVM::moveBefore(D, InsertPt);
      Moved = true;
    }
    if (Moved) {
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

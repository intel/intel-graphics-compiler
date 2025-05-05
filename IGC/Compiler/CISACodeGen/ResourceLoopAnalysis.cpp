/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ResourceLoopAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/Debug.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char ResourceLoopAnalysis::ID = 0;

#define PASS_FLAG "resource-loop-analysis"
#define PASS_DESCRIPTION "Analyze the begin and end of a resource loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(ResourceLoopAnalysis, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResourceLoopAnalysis, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

llvm::FunctionPass *IGC::createResourceLoopAnalysisPass() {
  return new ResourceLoopAnalysis;
}

ResourceLoopAnalysis::ResourceLoopAnalysis() : FunctionPass(ID) {
  initializeResourceLoopAnalysisPass(*PassRegistry::getPassRegistry());
}

static bool ValueOnlyUsedByEEI(Value *V) {
  for (Value::user_iterator UI = V->user_begin(), UE = V->user_end(); UI != UE;
       ++UI) {
    ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(*UI);
    if (!EEI || (EEI->getOperand(0) != V) ||
        !isa<ConstantInt>(EEI->getOperand(1))) {
      return false;
    }
  }
  return true;
}

bool ResourceLoopAnalysis::runOnFunction(Function &F) {
  auto WI = &(getAnalysis<WIAnalysis>());
  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  if (!IGC_IS_FLAG_ENABLED(FuseResourceLoop) ||
      CTX->platform.GetPlatformFamily() < IGFX_XE_HPG_CORE) {
    return true;
  }
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
    BasicBlock *BB = &*BI;
    // give every instruction a seq-no in order to check the location of uses
    std::map<Instruction *, unsigned> InstOrder;
    unsigned SeqNo = 0;
    for (BasicBlock::iterator II = BB->begin(), EI = BB->end(); II != EI;
         ++II) {
      Instruction *I = &*II;
      InstOrder[I] = SeqNo++;
    }
    // find and mark resource-loops
    unsigned loopOpTy = 0;
    Value *loopRes = nullptr;
    Value *loopSamp = nullptr;
    auto prevMemIter = BB->end();   // last memory-inst in the loop
    SmallPtrSet<Value *, 8> DefSet; // all memory-inst in the loop
    SmallPtrSet<Value *, 8> DefOnly4EEI;
    for (BasicBlock::iterator II = BB->begin(), EI = BB->end(); II != EI;
         ++II) {
      Instruction *I = &*II;
      unsigned curOpTy = 0;
      Value *curRes = nullptr;
      Value *curSamp = nullptr;
      // There are more types of lane-varying resource access than what are
      // listed below. Limited the optimization to get most of performance and
      // to reduce debugging scope.
      if (auto *LI = dyn_cast<SamplerLoadIntrinsic>(I)) {
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
            curRes = LI->getTextureValue();
            curOpTy = 3;
          }
        }
      } else if (auto *LI = dyn_cast<LdRawIntrinsic>(I)) {
        if (!WI->isUniform(LI->getResourceValue())) {
          // need extra restrictions:
          // no half-type because it may need extra op for packing
          if (LI->getType()->getScalarSizeInBits() >= 32) {
            curRes = LI->getResourceValue();
            curOpTy = 4;
          }
        }
      }
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
        if (!LoopEnd && curOpTy == loopOpTy && curRes == loopRes &&
            curSamp == loopSamp) {
          // need to check ALU instruction in between
          // all those instructions should only be used
          // inside the loop
          IGC_ASSERT(prevMemIter != BB->end());
          auto III = prevMemIter;
          ++III;
          while (!LoopEnd && III != II) {
            auto defInst = &*III;
            if (isa<ExtractElementInst>(defInst) && DefSet.count(defInst)) {
              ++III;
              continue;
            }
            for (auto UI = defInst->user_begin(), UE = defInst->user_end();
                 !LoopEnd && UI != UE; ++UI) {
              // Determine the block of the use.
              Instruction *useInst = cast<Instruction>(*UI);
              if (InstOrder.find(useInst) == InstOrder.end())
                LoopEnd = true;
              else if (InstOrder[useInst] > InstOrder[I] ||
                       InstOrder[useInst] <= InstOrder[defInst])
                LoopEnd = true;
            }
            ++III;
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
        } else {
          // mark the end of the previous loop
          if (prevMemIter != BB->end()) {
            auto PI = &*prevMemIter;
            IGC_ASSERT(LoopMap.find(PI) != LoopMap.end());
            LoopMap[PI] |= MarkResourceLoopEnd;
          }
          LoopMap[I] = MarkResourceLoopStart;
          loopRes = curRes;
          loopSamp = curSamp;
          loopOpTy = curOpTy;
          prevMemIter = II;
          DefSet.clear();
          DefSet.insert(I);
        }
      } else {
        // the other instructions
        bool LoopEnds = HasDeps;
        if (isa<CallInst>(I) || I->isTerminator())
          LoopEnds = true;
        else if (I->mayReadOrWriteMemory())
          LoopEnds = true;
        else if (WI->isUniform(I))
          LoopEnds = true; // avoid uniform in ballot loop
        else if (I->getType()->getScalarType()->isIntegerTy(1))
          LoopEnds = true; // avoid flag modification

        if (LoopEnds && prevMemIter != BB->end()) {
          auto PI = &*prevMemIter;
          IGC_ASSERT(LoopMap.find(PI) != LoopMap.end());
          LoopMap[PI] |= MarkResourceLoopEnd;
          prevMemIter = BB->end();
          loopRes = nullptr;
          loopSamp = nullptr;
          loopOpTy = 0;
          DefSet.clear();
        }
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(DumpResourceLoop)) {
    auto name = Debug::DumpName(Debug::GetShaderOutputName())
                    .Hash(CTX->hash)
                    .Type(CTX->type)
                    .Pass("ResourceLoop")
                    .PostFix(F.getName().str())
                    .Extension("txt");
    printResourceLoops(
        Debug::Dump(name, Debug::DumpType::DBG_MSG_TEXT).stream(), &F);
  }
  return false;
}

void ResourceLoopAnalysis::printResourceLoops(raw_ostream &OS,
                                              Function *F) const {
  if (F) {
    // All instructions
    for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
      Instruction *I = &*II;
      if (LoopMap.find(I) != LoopMap.end()) {
        unsigned marker = LoopMap.find(I)->second;
        OS << "  [" << marker << "]  " << *I << "\n";
      }
    }
  }
  OS << "\n";
}

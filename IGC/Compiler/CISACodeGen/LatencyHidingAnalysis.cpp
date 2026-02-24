/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// LatencyHidingAnalysis - Collects functional metrics about how effectively
// 2D block loads are placed ahead of DPAS consumers and if the order is optimal.
//
// Writes a YAML report for performance debug purposes.
// Main useful per-BB metrics include hiding sums (how many non-noop instructions
// are between 2D block loads and their DPAS consumers), load ordering penalty (
// how many inversions are there between actual load order and ideal load order based
// on the actual DPAS consumer positions), and shuffle classification (whether the load
// feeds DPAS directly or through a non-contiguous shuffle).

#include "Compiler/CISACodeGen/LatencyHidingAnalysis.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include "common/LLVMWarningsPop.hpp"

#include <fstream>

using namespace llvm;
using namespace IGC::Debug;

namespace IGC {

// ---- Pass registration ----

#define PASS_FLAG "igc-latency-hiding-analysis"
#define PASS_DESCRIPTION "Latency Hiding Analysis"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(LatencyHidingAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(VectorShuffleAnalysis)
IGC_INITIALIZE_PASS_END(LatencyHidingAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LatencyHidingAnalysis::ID = 0;

LatencyHidingAnalysis::LatencyHidingAnalysis() : FunctionPass(ID) {
  initializeLatencyHidingAnalysisPass(*PassRegistry::getPassRegistry());
}

LatencyHidingAnalysis::LatencyHidingAnalysis(const std::string &Tag, int Verbose)
    : FunctionPass(ID), DumpTag(Tag), VerboseLevel(Verbose), ShouldDump(true) {
  initializeLatencyHidingAnalysisPass(*PassRegistry::getPassRegistry());
}

// ---- Static helpers ----

static bool is2dBlockRead(Instruction *I) {
  if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I)) {
    switch (Intr->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_LSC2DBlockRead:
    case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
      return true;
    default:
      break;
    }
  }
  return false;
}

static bool isDPAS(Value *V) {
  GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(V);
  if (!Intr)
    return false;
  switch (Intr->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_dpas:
  case GenISAIntrinsic::GenISA_sub_group_dpas:
  case GenISAIntrinsic::GenISA_sub_group_bdpas:
    return true;
  default:
    break;
  }
  return false;
}

static bool isSetFieldInst(Instruction *I) {
  if (auto *Intr = dyn_cast<GenIntrinsicInst>(I))
    return Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField;
  return false;
}

static bool isCreatePayloadInst(Instruction *I) {
  if (auto *Intr = dyn_cast<GenIntrinsicInst>(I))
    return Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload;
  return false;
}

// Instructions that can hide load latency besides DPAS and other 2D block loads:
// vector fmul, fadd, fsub, fneg, fptrunc, fpext, maxnum, minnum, fabs, exp2,
// and WaveAll.
static bool isOtherHidingInst(Instruction *I) {
  if (!I->getType()->isVectorTy())
    return false;
  switch (I->getOpcode()) {
  case Instruction::FMul:
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FNeg:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
    return true;
  default:
    break;
  }
  if (auto *II = dyn_cast<IntrinsicInst>(I)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::maxnum:
    case Intrinsic::minnum:
    case Intrinsic::fabs:
    case Intrinsic::exp2:
      return true;
    default:
      break;
    }
  }
  if (auto *GII = dyn_cast<GenIntrinsicInst>(I)) {
    if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveAll)
      return true;
  }
  return false;
}

static std::string getValueName(Value *V) {
  if (!V)
    return "<null>";
  if (V->hasName())
    return "%" + V->getName().str();
  if (V->getType()->isVoidTy())
    return "<void>";
  return "%" + std::to_string(V->getValueID());
}

static std::string getInstString(Instruction *I) {
  if (!I)
    return "<null>";
  std::string Str;
  raw_string_ostream OS(Str);
  I->print(OS);
  OS.flush();
  return Str;
}

static std::string yamlEscape(const std::string &S) {
  std::string Result;
  Result.reserve(S.size());
  for (char C : S) {
    switch (C) {
    case '\\':
      Result += "\\\\";
      break;
    case '"':
      Result += "\\\"";
      break;
    case '\n':
      Result += "\\n";
      break;
    case '\r':
      Result += "\\r";
      break;
    case '\t':
      Result += "\\t";
      break;
    default:
      Result += C;
      break;
    }
  }
  return Result;
}

// ---- Core analysis ----

LatencyHidingAnalysis::BBAnalysisResult LatencyHidingAnalysis::analyzeBB(BasicBlock &BB) {
  BBAnalysisResult Result;
  Result.BBName = BB.hasName() ? BB.getName().str() : "unnamed";
  Result.NumLoads = 0;
  Result.NumDPAS = 0;
  Result.TotalInstructions = 0;
  Result.NumNonContiguousShuffleLoads = 0;
  Result.DpasHidingSum = 0;
  Result.LoadHidingSum = 0;
  Result.OtherHidingSum = 0;
  Result.LoadOrderPenalty = 0;

  // Phase 1: Build position map and collect DPAS/load/other-hiding positions
  DenseMap<Instruction *, int> Position;
  SmallVector<int, 64> DpasPositions;
  SmallVector<int, 32> LoadPositions;
  SmallVector<int, 32> OtherHidingPositions;
  int Pos = 0;
  for (auto &I : BB) {
    Position[&I] = Pos;
    if (isDPAS(&I)) {
      DpasPositions.push_back(Pos);
      Result.NumDPAS++;
    }
    if (is2dBlockRead(&I))
      LoadPositions.push_back(Pos);
    if (isOtherHidingInst(&I))
      OtherHidingPositions.push_back(Pos);
    Pos++;
  }
  Result.TotalInstructions = Pos;

  // Phase 2: Analyze each 2D block load
  for (auto &I : BB) {
    if (!is2dBlockRead(&I))
      continue;

    Result.NumLoads++;
    LoadAnalysisInfo LI;
    LI.LoadInst = &I;
    LI.SetupInsts = 0;
    LI.Distance = -1;
    LI.DpasBetween = -1;
    LI.LoadsBetween = -1;
    LI.OtherBetween = -1;
    LI.TotalConsumers = 0;
    LI.HasNonContiguousShuffle = false;
    LI.HasNonDPASConsumers = false;
    LI.ShuffleInsts = 0;

    // 2a. Classify decomposed vs direct
    GenIntrinsicInst *Intr = cast<GenIntrinsicInst>(&I);
    LI.IsDecomposed = (Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload);

    // 2b. Count setup instructions (decomposed only)
    if (LI.IsDecomposed) {
      Value *Payload = Intr->getOperand(0);
      for (auto It = BasicBlock::iterator(&I); It != BB.begin();) {
        --It;
        Instruction *Prev = &*It;
        if (isSetFieldInst(Prev) && Prev->getOperand(0) == Payload)
          LI.SetupInsts++;
        else if (isCreatePayloadInst(Prev) && Prev == Payload) {
          LI.SetupInsts++;
          break; // CreatePayload is the start of the setup chain
        }
      }
    }

    // 2c. Find all consumer DPAS and detect shuffle type
    SmallPtrSet<Instruction *, 16> Consumers;
    SmallPtrSet<Instruction *, 32> ShuffleInstSet;

    // Path A: Through vector shuffle patterns
    auto DVs = VSA->getDestVectorsForSourceVector(&I);
    for (auto *DV : DVs) {
      // Track all IE/EE instructions in this shuffle chain
      for (auto *IE : DV->getIEs())
        ShuffleInstSet.insert(IE);
      for (auto *EE : DV->EEs)
        ShuffleInstSet.insert(EE);

      if (!DV->isNoOp()) {
        // Non-contiguous shuffle: real MOVs, worst case for this load
        LI.HasNonContiguousShuffle = true;
      }

      // Find DPAS users of the last IE (the shuffle output)
      Instruction *LastIE = DV->getLastIE();
      for (auto *User : LastIE->users()) {
        if (auto *UserI = dyn_cast<Instruction>(User)) {
          if (isDPAS(UserI) && UserI->getParent() == &BB)
            Consumers.insert(UserI);
        }
      }
    }

    // Path B: Direct DPAS users
    for (auto *User : I.users()) {
      if (auto *UserI = dyn_cast<Instruction>(User)) {
        if (isDPAS(UserI) && UserI->getParent() == &BB)
          Consumers.insert(UserI);
      }
    }

    LI.ShuffleInsts = ShuffleInstSet.size();
    LI.TotalConsumers = Consumers.size();

    // Detect non-DPAS, non-shuffle consumers (e.g. VectorToScalars patterns)
    for (auto *User : I.users()) {
      if (auto *UserI = dyn_cast<Instruction>(User)) {
        if (!isDPAS(UserI) && !isa<ExtractElementInst>(UserI) && !isa<InsertElementInst>(UserI)) {
          LI.HasNonDPASConsumers = true;
          break;
        }
      }
    }

    // 2d. Compute dpas_between, loads_between, other_between
    if (LI.HasNonContiguousShuffle) {
      LI.DpasBetween = 0;
      LI.LoadsBetween = 0;
      LI.OtherBetween = 0;
      // Still compute distance and closest consumer for reporting
      int ClosestPos = INT_MAX;
      for (auto *C : Consumers) {
        if (Position[C] < ClosestPos) {
          ClosestPos = Position[C];
          LI.ClosestConsumer = C;
        }
      }
      LI.Distance = (ClosestPos < INT_MAX) ? (ClosestPos - Position[&I]) : -1;
    } else if (Consumers.empty()) {
      LI.DpasBetween = -1;
      LI.LoadsBetween = -1;
      LI.OtherBetween = -1;
      LI.Distance = -1;
    } else {
      // Find closest consumer DPAS
      int LoadPos = Position[&I];
      int ClosestConsumerPos = INT_MAX;
      for (auto *C : Consumers) {
        if (Position[C] < ClosestConsumerPos) {
          ClosestConsumerPos = Position[C];
          LI.ClosestConsumer = C;
        }
      }
      LI.Distance = ClosestConsumerPos - LoadPos;

      // Count DPAS at positions strictly between load and closest consumer
      int DpasCount = 0;
      for (int DpasPos : DpasPositions) {
        if (DpasPos > LoadPos && DpasPos < ClosestConsumerPos)
          DpasCount++;
      }
      LI.DpasBetween = DpasCount;

      // Count 2D block loads at positions strictly between load and closest consumer
      int LoadCount = 0;
      for (int LdPos : LoadPositions) {
        if (LdPos > LoadPos && LdPos < ClosestConsumerPos)
          LoadCount++;
      }
      LI.LoadsBetween = LoadCount;

      // Count other hiding insts between load and closest consumer
      int OtherCount = 0;
      for (int OtherPos : OtherHidingPositions) {
        if (OtherPos > LoadPos && OtherPos < ClosestConsumerPos)
          OtherCount++;
      }
      LI.OtherBetween = OtherCount;
    }

    Result.Loads.push_back(LI);
  }

  // Phase 3: Aggregate metrics
  int DpasHidingSum = 0;
  int LoadHidingSum = 0;
  int OtherHidingSum = 0;
  int NonContiguousShuffleLoads = 0;
  for (const auto &LI : Result.Loads) {
    if (LI.HasNonContiguousShuffle)
      NonContiguousShuffleLoads++;
    if (LI.DpasBetween >= 0 && !LI.HasNonContiguousShuffle) {
      DpasHidingSum += LI.DpasBetween;
      LoadHidingSum += LI.LoadsBetween;
      OtherHidingSum += LI.OtherBetween;
    }
  }
  Result.DpasHidingSum = DpasHidingSum;
  Result.LoadHidingSum = LoadHidingSum;
  Result.OtherHidingSum = OtherHidingSum;
  Result.NumNonContiguousShuffleLoads = NonContiguousShuffleLoads;

  // Phase 4: Compute load ordering penalty.
  // Ideal load order follows the DPAS consumption stream:
  //   Primary key:   earliest consumer DPAS position (ascending)
  //   Secondary key: shared loads (2+ consumers) before unique loads
  // This ensures each DPAS group's shared load and unique loads are
  // clustered together rather than all shared loads being hoisted first.
  // Example: DPAS(A,X), DPAS(A,Y), DPAS(B,K), DPAS(B,M)
  //   Ideal: A, X, Y, B, K, M  (NOT: A, B, X, Y, K, M)
  // Penalty = number of inversions between actual and ideal order.
  {
    struct LoadRankInfo {
      int LoadIdx;          // index in Result.Loads
      int IdealGroup;       // 0 = shared (multi-consumer), 1 = unique
      int EarliestConsumer; // position of earliest consumer DPAS
    };

    SmallVector<LoadRankInfo, 32> RankInfos;
    for (size_t k = 0; k < Result.Loads.size(); k++) {
      const auto &LI = Result.Loads[k];
      if (LI.TotalConsumers == 0)
        continue;
      LoadRankInfo RI;
      RI.LoadIdx = k;
      RI.IdealGroup = (LI.TotalConsumers > 1) ? 0 : 1;
      RI.EarliestConsumer = LI.ClosestConsumer ? Position[LI.ClosestConsumer] : INT_MAX;
      RankInfos.push_back(RI);
    }

    int Penalty = 0;
    for (size_t i = 0; i < RankInfos.size(); i++) {
      for (size_t j = i + 1; j < RankInfos.size(); j++) {
        // Load i appears before load j in actual code.
        // If load j should ideally come before load i, that's an inversion.
        auto IdealI = std::make_pair(RankInfos[i].EarliestConsumer, RankInfos[i].IdealGroup);
        auto IdealJ = std::make_pair(RankInfos[j].EarliestConsumer, RankInfos[j].IdealGroup);
        if (IdealJ < IdealI) {
          Penalty++;

          std::string Reason;
          if (RankInfos[j].EarliestConsumer < RankInfos[i].EarliestConsumer) {
            Reason = "feeds earlier DPAS (consumer pos " + std::to_string(RankInfos[j].EarliestConsumer) + " < " +
                     std::to_string(RankInfos[i].EarliestConsumer) + ")";
          } else {
            Reason = "shared load (consumers: " + std::to_string(Result.Loads[RankInfos[j].LoadIdx].TotalConsumers) +
                     ") should precede unique load at same DPAS";
          }
          Result.LoadOrderInversions.push_back({RankInfos[i].LoadIdx, RankInfos[j].LoadIdx, Reason});
        }
      }
    }
    Result.LoadOrderPenalty = Penalty;
  }

  return Result;
}

// ---- Dump ----

void LatencyHidingAnalysis::dumpResults(Function &F) {
  std::string Out;
  Out.reserve(8192);

  Out += "---\n";
  Out += "tag: \"" + yamlEscape(DumpTag) + "\"\n";
  Out += "function: \"" + yamlEscape(F.getName().str()) + "\"\n";
  Out += "basic_blocks:\n";

  for (const auto &R : FunctionResults) {
    Out += "  - name: \"" + yamlEscape(R.BBName) + "\"\n";
    Out += "    loads: " + std::to_string(R.NumLoads) + "\n";
    Out += "    dpas: " + std::to_string(R.NumDPAS) + "\n";
    Out += "    total_insts: " + std::to_string(R.TotalInstructions) + "\n";
    Out += "    dpas_hiding_sum: " + std::to_string(R.DpasHidingSum) + "\n";
    Out += "    load_hiding_sum: " + std::to_string(R.LoadHidingSum) + "\n";
    Out += "    other_hiding_sum: " + std::to_string(R.OtherHidingSum) + "\n";
    Out += "    load_order_penalty: " + std::to_string(R.LoadOrderPenalty) + "\n";
    Out += "    non_contiguous_shuffle_loads: " + std::to_string(R.NumNonContiguousShuffleLoads) + "\n";
    Out += "    load_details:\n";

    for (size_t i = 0; i < R.Loads.size(); i++) {
      const auto &LI = R.Loads[i];

      std::string ShuffleType = "none";
      if (LI.HasNonContiguousShuffle)
        ShuffleType = "non_contiguous";
      else if (LI.ShuffleInsts > 0)
        ShuffleType = "noop";

      Out += "      - index: " + std::to_string(i) + "\n";
      Out += "        name: \"" + yamlEscape(getValueName(LI.LoadInst)) + "\"\n";
      Out += "        type: \"" + std::string(LI.IsDecomposed ? "decomposed" : "direct") + "\"\n";
      Out += "        setup: " + std::to_string(LI.SetupInsts) + "\n";
      Out += "        distance: " + std::to_string(LI.Distance) + "\n";
      Out += "        dpas_between: " + std::to_string(LI.DpasBetween) + "\n";
      Out += "        loads_between: " + std::to_string(LI.LoadsBetween) + "\n";
      Out += "        other_between: " + std::to_string(LI.OtherBetween) + "\n";
      Out += "        consumers: " + std::to_string(LI.TotalConsumers) + "\n";
      Out += "        has_non_dpas_consumers: " + std::string(LI.HasNonDPASConsumers ? "true" : "false") + "\n";
      Out += "        shuffle: \"" + ShuffleType + "\"\n";
      Out += "        shuffle_insts: " + std::to_string(LI.ShuffleInsts) + "\n";

      if (VerboseLevel >= 2) {
        Out += "        ir: \"" + yamlEscape(getInstString(LI.LoadInst)) + "\"\n";
        if (LI.ClosestConsumer)
          Out += "        first_consumer: \"" + yamlEscape(getInstString(LI.ClosestConsumer)) + "\"\n";
      }
    }

    if (VerboseLevel >= 2 && !R.LoadOrderInversions.empty()) {
      Out += "    load_order_inversions:\n";
      for (const auto &Inv : R.LoadOrderInversions) {
        const auto &Early = R.Loads[Inv.EarlyLoadIdx];
        const auto &Late = R.Loads[Inv.LateLoadIdx];
        Out += "      - placed: \"" + yamlEscape(getValueName(Early.LoadInst)) + "\"\n";
        Out += "        before: \"" + yamlEscape(getValueName(Late.LoadInst)) + "\"\n";
        Out += "        reason: \"" + yamlEscape(Inv.Reason) + "\"\n";
        Out += "        placed_ir: \"" + yamlEscape(getInstString(Early.LoadInst)) + "\"\n";
        Out += "        before_ir: \"" + yamlEscape(getInstString(Late.LoadInst)) + "\"\n";
      }
    }
  }

  // Output to console or file
  if (IGC_IS_FLAG_ENABLED(PrintToConsole)) {
    llvm::outs() << Out;
  } else {
    auto Name = DumpName(IGC::Debug::GetShaderOutputName())
                    .Hash(CTX->hash)
                    .Type(CTX->type)
                    .Retry(CTX->m_retryManager->GetRetryId())
                    .Pass(("latency_hiding_" + DumpTag).c_str())
                    .Extension("yaml");
    IGC::Debug::DumpLock();
    auto OpenMode = FirstDump ? std::ios_base::out : std::ios_base::app;
    FirstDump = false;
    std::ofstream OutputFile(Name.str(), OpenMode);
    if (OutputFile.is_open()) {
      OutputFile << Out;
    }
    OutputFile.close();
    IGC::Debug::DumpUnlock();
  }
}

// ---- runOnFunction ----

bool LatencyHidingAnalysis::runOnFunction(Function &F) {
  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  VSA = &getAnalysis<VectorShuffleAnalysis>();

  FunctionResults.clear();
  BBToResultIdx.clear();

  for (auto &BB : F) {
    // Skip BBs without both 2D block loads and DPAS
    bool Has2DLoad = false;
    bool HasDPAS = false;
    for (auto &I : BB) {
      if (is2dBlockRead(&I))
        Has2DLoad = true;
      if (isDPAS(&I))
        HasDPAS = true;
      if (Has2DLoad && HasDPAS)
        break;
    }
    if (!Has2DLoad || !HasDPAS)
      continue;

    BBToResultIdx[&BB] = FunctionResults.size();
    FunctionResults.push_back(analyzeBB(BB));
  }

  if (!FunctionResults.empty() && (ShouldDump || IGC_IS_FLAG_ENABLED(PrintToConsole)))
    dumpResults(F);

  return false; // analysis pass, never modifies IR
}

} // namespace IGC

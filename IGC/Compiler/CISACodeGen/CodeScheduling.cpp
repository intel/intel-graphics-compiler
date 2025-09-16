/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <fstream>

#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
// #include "common/Stats.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Verifier.h"
#include "common/LLVMWarningsPop.hpp"

// #include "llvm/ADT/PostOrderIterator.h"
#include "Compiler/CISACodeGen/CodeScheduling.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Value.h"
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>

using namespace llvm;
using namespace IGC::Debug;

namespace IGC {

typedef enum VerbosityLevel { None = 0, Low, Medium, High } VerbosityLevel;

// Static functions

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
    return true;
  default:
    break;
  }
  return false;
};

// Get Value name as string for debug purposes
// Can have side effect of assigning a name to the value if it has no name
// Under a debug flag CodeSchedulingRenameAll
static std::string getName(Value *V) {
  if (!V)
    return "<null>";
  if (V->hasName())
    return "%" + V->getName().str();

  if (V->getType()->isVoidTy()) {
    return "<void>";
  }
  if (IGC_IS_FLAG_ENABLED(CodeSchedulingRenameAll)) {
    // If the value has no name, we can assign a name to it
    // to make debugging easier.
    std::string Name = "x" + std::to_string(V->getValueID());
    V->setName(Name);
    return "%" + Name;
  }
  return "%" + std::to_string(V->getValueID());
}

// Helper functions for debug dumps
#define PrintDumpLevel(Level, Contents)                                                                                \
  if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling) && (Level <= IGC_GET_FLAG_VALUE(CodeSchedulingDumpLevel))) {             \
    *LogStream << Contents;                                                                                            \
  }
#define PrintInstructionDumpLevel(Level, Inst)                                                                         \
  if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling) && (Level <= IGC_GET_FLAG_VALUE(CodeSchedulingDumpLevel))) {             \
    (Inst)->print(*LogStream, false);                                                                                  \
    *LogStream << "\n";                                                                                                \
  }
// default level is low
#define PrintDump(Contents) PrintDumpLevel(VerbosityLevel::Low, Contents)
#define PrintInstructionDump(Inst) PrintInstructionDumpLevel(VerbosityLevel::Low, Inst)

// Register pass to igc-opt
#define PASS_FLAG "igc-code-scheduling"
#define PASS_DESCRIPTION "Code Scheduling"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CodeScheduling, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(VectorShuffleAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(RematChainsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
IGC_INITIALIZE_PASS_END(CodeScheduling, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CodeScheduling::ID = 0;
CodeScheduling::CodeScheduling() : FunctionPass(ID), LogStringStream(Log) {
  if (IGC_IS_FLAG_ENABLED(PrintToConsole)) {
    LogStream = &IGC::Debug::ods();
  } else {
    LogStream = &LogStringStream;
  }
  initializeCodeSchedulingPass(*PassRegistry::getPassRegistry());
}

// Helper class to hold configuration options for code scheduling
class SchedulingConfig {
private:
  std::vector<int> OptionValues;
  std::vector<std::string> OptionNames;

public:
#define DECLARE_SCHEDULING_OPTION(option, defaultValue, description) option,

  enum Option {
#include "CodeSchedulingOptionsDef.h"
  };

#undef DECLARE_SCHEDULING_OPTION
#define DECLARE_SCHEDULING_OPTION(option, defaultValue, description)                                                   \
  OptionValues.push_back(defaultValue);                                                                                \
  OptionNames.push_back(#option);

  SchedulingConfig() {
#include "CodeSchedulingOptionsDef.h"

    if (IGC_IS_FLAG_SET(CodeSchedulingConfig)) {
      std::string ConfigString = IGC_GET_REGKEYSTRING(CodeSchedulingConfig);
      updateFromString(ConfigString);
    }
  }

#undef DECLARE_SCHEDULING_OPTION

  int operator[](Option key) { return OptionValues[key]; }

  int get(Option key) { return OptionValues[key]; }

  std::string toString() {
    std::string Str;
    for (const auto &Option : OptionValues) {
      Str += std::to_string(Option) + ";";
    }
    // return Str without the last ;
    return Str.substr(0, Str.size() - 1);
  }

  // Update the configuration from a string in the format "1;2;3;4",
  // where each number corresponds to the value of an option in the order defined in CodeSchedulingOptionsDef.h.
  // Used with the CodeSchedulingConfig debug IGC flag
  void updateFromString(std::string ConfigString) {
    // ConfigString contains only values
    std::vector<int> Values;
    size_t Pos = 0;
    std::string Token;
    while ((Pos = ConfigString.find(";")) != std::string::npos) {
      Token = ConfigString.substr(0, Pos);
      Values.push_back(std::stoi(Token));
      ConfigString.erase(0, Pos + 1);
    }
    if (!ConfigString.empty()) {
      Values.push_back(std::stoi(ConfigString));
    }
    IGC_ASSERT(Values.size() == OptionValues.size());
    OptionValues = std::move(Values);
  }

  void printOptions(llvm::raw_ostream *LogStream) {
    PrintDump("IGC_CodeSchedulingConfig=\"" << toString() << "\"\n");
    for (size_t i = 0; i < OptionValues.size(); i++) {
      PrintDump("  " << OptionNames[i] << ": " << OptionValues[i] << "\n");
    }
  }
};

// Class to track register pressure within a basic block
// It is stateful, tracking the register pressure as instructions are added using 'update' method
// Object of this class are copyable so the current state can be saved, but they don't have the whole information about
// the order of the instructions added, only the estimated regpressure. Preserving the order of instructions would be a
// responsibility of the user class

class RegisterPressureTracker {
public:
  RegisterPressureTracker(BasicBlock *BB, IGCLivenessAnalysis *RPE, IGCFunctionExternalRegPressureAnalysis *FRPE,
                          VectorShuffleAnalysis *VSA, RematChainsAnalysis *RCA, WIAnalysisRunner *WI, CodeGenContext *CTX,
                          SchedulingConfig *Config, llvm::raw_ostream *LogStream)
      : BB(BB), RPE(RPE), FRPE(FRPE), VSA(VSA), RCA(RCA), WI(WI), CTX(CTX), C(Config), LogStream(LogStream) {
    F = BB->getParent();
    SIMD = C->get(SchedulingConfig::Option::ForceSIMDSize) > 0 ? C->get(SchedulingConfig::Option::ForceSIMDSize)
                                                               : numLanes(RPE->bestGuessSIMDSize(F));
    PrintDump("SIMD: " << SIMD << "\n");
    DL = &(F->getParent()->getDataLayout());

    reset();
  }

  RegisterPressureTracker(const RegisterPressureTracker &RPT) {
    BB = RPT.BB;
    RPE = RPT.RPE;
    FRPE = RPT.FRPE;
    VSA = RPT.VSA;
    RCA = RPT.RCA;
    WI = RPT.WI;
    CTX = RPT.CTX;
    C = RPT.C;
    LogStream = RPT.LogStream;

    F = BB->getParent();
    SIMD = C->get(SchedulingConfig::Option::ForceSIMDSize) > 0 ? C->get(SchedulingConfig::Option::ForceSIMDSize)
                                                               : numLanes(RPE->bestGuessSIMDSize(F));
    DL = &(F->getParent()->getDataLayout());

    // copy the state
    BBIn = RPT.BBIn;
    BBOut = RPT.BBOut;
    BBCurrent = RPT.BBCurrent;
    CurrentPressure = RPT.CurrentPressure;
    EstimationCache = RPT.EstimationCache;
    RealUsesCache = RPT.RealUsesCache;
    ValueSizeCache = RPT.ValueSizeCache;

    CurrentNumOf2dLoads = RPT.CurrentNumOf2dLoads;
    TotalNumOf2dLoads = RPT.TotalNumOf2dLoads;

    // deepcopy HangingLiveVarsVec and HangingLiveVars
    HangingLiveVarsVec.clear();
    HangingLiveVarsVec.reserve(RPT.HangingLiveVarsVec.size());
    for (const auto &HangingLiveVar : RPT.HangingLiveVarsVec) {
      HangingLiveVarsVec.push_back(std::make_unique<HangingLiveVarsInfo>(HangingLiveVar->Size, HangingLiveVar->Type));
      HangingLiveVarsVec.back()->LiveVars = HangingLiveVar->LiveVars;
      for (auto *V : HangingLiveVar->LiveVars) {
        HangingLiveVars[V] = HangingLiveVarsVec.back().get();
      }
    }
  }

  RegisterPressureTracker &operator=(const RegisterPressureTracker &) = delete;
  RegisterPressureTracker() = delete;
  ~RegisterPressureTracker() = default;

  int getNumGRF() {
    int NGRF = static_cast<int>(CTX->getNumGRFPerThread(false));
    if (NGRF == 0) { // GRF info is not set, using the default value
      if (CTX->isAutoGRFSelectionEnabled()) {
        NGRF = C->get(SchedulingConfig::Option::DefaultNumGRFAuto);
      } else {
        NGRF = C->get(SchedulingConfig::Option::DefaultNumGRF);
      }
    }
    return NGRF;
  }

  unsigned int computeSizeInBytes(Value *V, unsigned int SIMD, WIAnalysisRunner *WI, const DataLayout &DL) {
    auto It = ValueSizeCache.find({V, SIMD});
    if (It != ValueSizeCache.end()) {
      return It->second;
    }
    unsigned int Size = computeSizeInBytesImpl(V, SIMD, WI, DL);
    ValueSizeCache[{V, SIMD}] = Size;
    return Size;
  }

  unsigned int computeSizeInBytesImpl(Value *V, unsigned int SIMD, WIAnalysisRunner *WI, const DataLayout &DL) {
    auto Type = V->getType();

    bool NoRetVal = Type->isVoidTy();
    if (NoRetVal)
      return 0;

    if (auto *Intr = dyn_cast<GenIntrinsicInst>(V)) {
      switch (Intr->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_ftobf:
        // use the size of the input type, because bf is GRF-aligned
        Type = Intr->getOperand(0)->getType();
        break;
      default:
        break;
      }
    }

    auto TypeSizeInBits = static_cast<int>(DL.getTypeSizeInBits(Type));

    int Multiplier = static_cast<int>(SIMD);
    if (WI && WI->isUniform(V))
      Multiplier = 1;
    int SizeInBytes = TypeSizeInBits * Multiplier / 8;
    return SizeInBytes;
  }

  // Set the initial state using RPE and FRPE
  void reset() {
    BBIn = RPE->getInSet()[BB];
    BBOut = RPE->getOutSet()[BB];

    BBCurrent.clear();
    for (auto *V : BBIn) {
      if (isa<Argument>(V)) {
        BBCurrent.insert(V);
        continue;
      }

      auto *I = dyn_cast<Instruction>(V);
      if (!I)
        continue;

      IGC_ASSERT(!IGCLLVM::isDebugOrPseudoInst(*I));

      auto *DV = VSA->getDestVector(I);
      if (DV && DV->isVectorShuffle()) {
        BBCurrent.insert(DV->getSourceVec());
      } else {
        BBCurrent.insert(I);
      }
    }

    // Add all Phi instructions from BB to BBCurrent
    for (auto &Phi : BB->phis()) {
      BBCurrent.insert(&Phi);
      // add all the Phi Values to BBIn
      for (auto &Op : Phi.operands()) {
        Value *V = Op.get();
        BBIn.insert(V);
      }
    }

    PrintDumpLevel(VerbosityLevel::Medium, "Initial BBIn: " << BBIn.size() << "\n");
    for (auto *V : BBIn) {
      PrintInstructionDumpLevel(VerbosityLevel::Medium, V);
    }

    PrintDumpLevel(VerbosityLevel::Medium, "Initial BBCurrent: " << BBCurrent.size() << "\n");
    for (auto *V : BBCurrent) {
      PrintInstructionDumpLevel(VerbosityLevel::Medium, V);
    }

    PrintDump("\n\n");
    const int ReservedRegisters = C->get(SchedulingConfig::Option::ReservedRegisters);
    const int RegisterSize = static_cast<int>(RPE->registerSizeInBytes());
    CurrentPressure =
        static_cast<int32_t>(RPE->estimateSizeInBytes(BBCurrent, *F, SIMD, WI)) + ReservedRegisters * RegisterSize;
    PrintDump("Initial CurrentPressure: " << CurrentPressure << "\n");
    int32_t CurrentPressureInRegisters = static_cast<int32_t>(RPE->bytesToRegisters(CurrentPressure));
    PrintDump("Initial CurrentPressure in registers: " << CurrentPressureInRegisters << "\n\n");

    CurrentNumOf2dLoads = 0;
    TotalNumOf2dLoads = std::count_if(BB->begin(), BB->end(), [](Instruction &I) { return is2dBlockRead(&I); });
  }

  bool isRegpressureLow(Instruction *I = nullptr) {
    return compareRPWithThreshold<false>(C->get(SchedulingConfig::Option::LowRPThresholdDelta), I);
  }

  bool isRegpressureHigh(Instruction *I = nullptr) {
    return compareRPWithThreshold<true>(C->get(SchedulingConfig::Option::GreedyRPThresholdDelta) +
                                            static_cast<int>(IGC_GET_FLAG_VALUE(CodeSchedulingRPMargin)),
                                        I);
  }

  bool isRegpressureCritical(Instruction *I = nullptr) {
    int AdjustmentForFragmentation = 0;
    if (I && is2dBlockRead(I) && (getNumGRF() >= C->get(SchedulingConfig::Option::FragmentationAdjustmentsMinGRF))) {
      if (!C->get(SchedulingConfig::Option::IgnoreFragmentationForLastLoad) ||
          (CurrentNumOf2dLoads < (TotalNumOf2dLoads - 1))) {
        auto *VectorType = dyn_cast<IGCLLVM::FixedVectorType>(I->getType());
        if (VectorType) {
          if (static_cast<int>(VectorType->getNumElements()) >=
              C->get(SchedulingConfig::Option::LargeLoadSizeForFragmentationAdjustment)) {
            AdjustmentForFragmentation = C->get(SchedulingConfig::Option::RPMarginIncreaseForFragmentationAdjustment);
          }
        }
      }
    }
    return compareRPWithThreshold<true>(
        static_cast<int>(IGC_GET_FLAG_VALUE(CodeSchedulingRPMargin)) + AdjustmentForFragmentation, I);
  }

  template <bool checkIfHigher> bool compareRPWithThreshold(int Threshold, Instruction *I = nullptr) {
    if constexpr (checkIfHigher) {
      return getCurrentPressure(I) > getNumGRF() - Threshold;
    } else {
      return getCurrentPressure(I) <= getNumGRF() - Threshold;
    }
  }

  int32_t getCurrentPressure(Instruction *I = nullptr) {
    auto CurrentPressureAdjusted = CurrentPressure;
    if (I != nullptr)
      CurrentPressureAdjusted += estimate(I);
    auto ExternalPressure = static_cast<int32_t>(FRPE->getExternalPressureForFunction(F));
    auto CurrentPressureInRegisters =
        static_cast<int32_t>(RPE->bytesToRegisters(CurrentPressureAdjusted)) + ExternalPressure;
    return CurrentPressureInRegisters;
  }

  int32_t estimate(Instruction *I) { return estimateOrUpdate(I, false); }

  int32_t update(Instruction *I) { return estimateOrUpdate(I, true); }

  llvm::DenseSet<Value *> getRealUses(Value *I) {
    auto It = RealUsesCache.find(I);
    if (It != RealUsesCache.end()) {
      return It->second;
    }

    llvm::DenseSet<Value *> &Uses = RealUsesCache.try_emplace(I).first->second;

    std::function<void(Value *)> collectRealUses = [&](Value *V) {
      for (auto *U : V->users()) {
        if (Instruction *UI = dyn_cast<Instruction>(U)) {
          if (isDbgIntrinsic(UI))
            continue;

          if (isNoOpInst(UI, CTX)) {
            collectRealUses(UI);
          } else {
            Uses.insert(UI);
          }
        }
      }
    };

    collectRealUses(I);

    return Uses;
  }

  bool inBBCurrent(Value *V) { return BBCurrent.count(V); }

  Value *getRealOp(Value *V) {
    if (BBIn.count(V))
      return V;

    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
      return V;

    bool IsAddrSpaceCast = isa<AddrSpaceCastInst>(I);

    if (isNoOpInst(I, CTX) || IsAddrSpaceCast) {
      return getRealOp(I->getOperand(0));
    }
    return V;
  }

  DenseSet<Instruction *> getHangingS2VInstructions() {
    // return all the vectors that are created of scalars, but not fully populated yet
    DenseSet<Instruction *> HangingInstructions;
    for (const auto &HangingLiveVar : HangingLiveVarsVec) {
      if (HangingLiveVar->Type == HangingLiveVarsType::HANGING_SCALARS_TO_VECTOR) {
        for (auto *V : HangingLiveVar->LiveVars) {
          if (Instruction *I = dyn_cast<Instruction>(V)) {
            HangingInstructions.insert(I);
          }
        }
      }
    }
    return HangingInstructions;
  }

private:
  BasicBlock *BB;
  Function *F;
  IGCLivenessAnalysis *RPE;
  IGCFunctionExternalRegPressureAnalysis *FRPE;
  VectorShuffleAnalysis *VSA;
  RematChainsAnalysis *RCA;
  WIAnalysisRunner *WI;
  CodeGenContext *CTX;
  const DataLayout *DL;
  SchedulingConfig *C;
  llvm::raw_ostream *LogStream;

  int32_t SIMD;
  int32_t CurrentPressure = 0;

  int32_t TotalNumOf2dLoads = 0;
  int32_t CurrentNumOf2dLoads = 0;

  ValueSet BBIn;
  ValueSet BBOut;
  ValueSet BBCurrent;

  llvm::DenseMap<Value *, int32_t> EstimationCache;
  llvm::DenseMap<Value *, DenseSet<Value *>> RealUsesCache;
  llvm::DenseMap<std::pair<Value *, int32_t>, int32_t> ValueSizeCache;

  typedef enum { HANGING_SCALARS_TO_VECTOR, HANGING_VECTOR_TO_SCALARS, HANGING_VECTORS, HANGING_NOOP_VECTORS } HangingLiveVarsType;

  // POD structure to keep information about hanging values
  struct HangingLiveVarsInfo {
    ValueSet LiveVars;
    uint32_t Size;
    HangingLiveVarsType Type;

    HangingLiveVarsInfo(uint32_t SizeInBytes, HangingLiveVarsType Type) : LiveVars(), Size(SizeInBytes), Type(Type) {};
  };
  std::vector<std::unique_ptr<HangingLiveVarsInfo>> HangingLiveVarsVec;
  DenseMap<Value *, HangingLiveVarsInfo *> HangingLiveVars;

  // Check if the value dies on the instruction CurrentI. Looks through no-op instructions,
  // but doesn't check if the value "hangs". Handling the value that looks dead is in fact "hangs"
  // is the responsibility of the user function.
  bool operandDies(Value *V, Instruction *CurrentI) {
    if (BBOut.count(V))
      return false;

    if (isa<Argument>(V))
      return false;

    for (auto *U : getRealUses(V)) {
      if (Instruction *UI = dyn_cast<Instruction>(U)) {
        if (UI->getParent() != BB) {
          continue;
        }
        if (IGCLLVM::isDebugOrPseudoInst(*UI))
          continue;

        if (!BBCurrent.count(UI) && UI != CurrentI) {
          // found a use of the value that is not in BBCurrent (that means not "placed" in the BB yet)
          // and it is not the CurrentI instruction. So it is still alive
          return false;
        }
      }
    }

    return true;
  };

  // Main function of the RegisterPressureTracker class
  // It estimates the register pressure in case we add instruction I to the basic block
  // Or updates the state to reflect that we add the instruction I (if Update is true)
  // Returns the estimated or updated register pressure in bytes
  int32_t estimateOrUpdate(Instruction *I, bool Update) {
    if (Update) {
      EstimationCache.clear();
      return estimateOrUpdateImpl(I, Update);
    }
    auto It = EstimationCache.find(I);
    if (It != EstimationCache.end()) {
      return It->second;
    }
    int32_t Result = estimateOrUpdateImpl(I, Update);
    EstimationCache[I] = Result;
    return Result;
  }

  int32_t estimateOrUpdateImpl(Instruction *I, bool Update) {
    auto *Intr = dyn_cast<GenIntrinsicInst>(I);
    bool IsNoOpIntr = Intr && (Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_ptr_to_pair);

    if (IGCLLVM::isDebugOrPseudoInst(*I) || I->isLifetimeStartOrEnd() || isNoOpInst(I, CTX) || IsNoOpIntr) {
      // NoOp instructions do not change register pressure
      if (Update)
        PrintDumpLevel(VerbosityLevel::High, "NoOp instruction: " << getName(I) << "\n");
      return 0;
    }

    // Check for remat chain patterns
    if (RCA && !Update) {
      RematChainPattern *RCP = RCA->getRematChainPattern(I);
      if (RCP && (RCP->getFirstInst() == I)) {
        // if it's a remat chain we are going to use the remat target instruction (if it's load or store)
        Instruction *TargetInst = RCP->getRematTargetInst();
        return estimateOrUpdateImpl(TargetInst, false);
      }
    }

    if (Update)
      PrintDumpLevel(VerbosityLevel::High, getName(I));

    int32_t ResultSizeInBytes = 0;

    // First check how does the instruction increase the register pressure
    // It takes the register for the output value...
    int RPIncrease = computeSizeInBytes(I, SIMD, WI, *DL);

    if (!Update && isShuffled2dBlockRead(I)) {
      RPIncrease *= 2;
    }

    // ... if is not a special case

    // There are 4 special cases when dealing with InsertElement/ExtractElement instructions:

    auto *DTI = VSA->getDestVector(I);
    auto *V2SP = VSA->getVectorToScalarsPattern(I);

    if (DTI) {
      if (DTI->isNoOp()) {
        // InsertElement and ExtractElement sequences that result in no operations in the assembly do not
        // increase register pressure
        RPIncrease = 0;
      }

      if (DTI->isVectorShuffle() && !DTI->isNoOp()) {
        // IE and EE instructions perform a transformation
        // The first IE increases the regpressure (we allocate subsequent register space for the subvector)
        // The other instructions don't
        if (DTI->getFirstIE() != I) {
          RPIncrease = 0;
        }
      }
      if (!DTI->isVectorShuffle()) {
        // Composing the vector out of scalars
        // First IE increases the repressure (we allocate subsequent register space for the vector)
        // The other instructions don't
        if (DTI->getFirstIE() != I) {
          RPIncrease = 0;
        }
      }
    } else {
      if (V2SP) { // VectorToScalarsPattern
        // ExtractElement instruction that extracts a scalar from a vector
        // Doesn't increase pressure
        RPIncrease = 0;
      }
    }

    if (Update)
      PrintDumpLevel(VerbosityLevel::High, ": +" << RPIncrease << "   ");

    ResultSizeInBytes += RPIncrease;

    // Function to create a HangingLiveVarsInfo for a two of vector cases
    auto createHLVForVector = [&](HangingLiveVarsType Type, Value *SourceVec) {
      // Create a HangingLiveVarsInfo for the vector
      auto SourceVecSize = computeSizeInBytes(SourceVec, SIMD, WI, *DL);
      HangingLiveVarsVec.emplace_back(std::make_unique<HangingLiveVarsInfo>(SourceVecSize, Type));
      auto *HLV = HangingLiveVarsVec.back().get();
      for (auto *DT : VSA->getDestVectorsForSourceVector(SourceVec)) {
        auto *CurrentLastIE = DT->getLastIE();
        auto *CurrentLastEE = DT->getLastEE();
        if (Type == HANGING_VECTORS) {
          HLV->LiveVars.insert(CurrentLastEE);
          HangingLiveVars[CurrentLastEE] = HLV;
        } else {
          if (Type == HANGING_NOOP_VECTORS) {
            // If we are creating a HangingLiveVarsInfo for no-op vectors, we use LastIE
            // because it is the last instruction that kills the whole vector
            HLV->LiveVars.insert(CurrentLastIE);
            HangingLiveVars[CurrentLastIE] = HLV;
          }
        }
      }
      if (Update)
        PrintDumpLevel(VerbosityLevel::High,
                       " (populating HLV with "
                           << HLV->LiveVars.size()
                           << (Type == HANGING_NOOP_VECTORS ? " IEs, vector size " : " EEs, vector size ") << HLV->Size
                           << ")");
      return HLV;
    };

    if (Update) {
      // If we place the instruction it's possible that it prolongs the live interval of some instructions
      // So that they will take space in the registers when the associated SSA value dies and is not used anymore

      // We call it "hanging" instructions. Currently 4 patterns are supported:

      // 1. "NoOp" shuffle
      // IE and EE just create smaller vector out of a larger one and the indices are sequential
      // This means that the instruction is a no-op and does not change the register pressure
      // But the source vector is going to die only when all the subvectors die

      if (DTI && DTI->isNoOp()) {
        auto *LastIE = DTI->getLastIE();
        if (!HangingLiveVars.count(LastIE)) {
          auto *HLV = createHLVForVector(HANGING_NOOP_VECTORS, DTI->getSourceVec());
          IGC_ASSERT(HangingLiveVars[LastIE] == HLV);
          IGC_ASSERT(HangingLiveVars.count(LastIE));
        }
      }

      // 2. Vector shuffle
      // Every First IE of a subvector increases pressure, because there will be MOVs in the asm
      // Last IE of all the transforms kills the whole SourceVector

      // To model that we populate the HangingLiveVars with the last EEs.

      // Then last usage of every subvector kills the corresponding subvector, so they behave as normal values
      else if (DTI && DTI->isVectorShuffle()) {
        auto *LastEE = DTI->getLastEE();
        if (!HangingLiveVars.count(LastEE)) {
          auto *HLV = createHLVForVector(HANGING_VECTORS, DTI->getSourceVec());
          IGC_ASSERT(HangingLiveVars[LastEE] == HLV);
          IGC_ASSERT(HangingLiveVars.count(LastEE));
        }
      }

      // 3. Vector is creating out of scalars
      // These scalars will have a common live interval, so first IE increases pressure: the vector is created
      // and the scalars are not dead, even the first will live further.

      // The last InsertElement will decrease pressure only if there are no more uses of the initial scalar
      // values. If there are, the values "hang" and register pressure will decrease only when all the scalars are
      // dead.

      // Populating the HangingLiveVars with all the scalars and the size of the vector
      else if (DTI) {
        IGC_ASSERT(isa<InsertElementInst>(I));

        auto *FirstIE = DTI->getFirstIE();
        auto *FirstScalar = FirstIE->getOperand(1);
        if (!HangingLiveVars.count(FirstScalar)) {
          HangingLiveVarsVec.emplace_back(std::make_unique<HangingLiveVarsInfo>(0, HANGING_SCALARS_TO_VECTOR));
          auto *HLV = HangingLiveVarsVec.back().get();

          for (Value *V : DTI->getSourceScalars()) {
            if (HLV->LiveVars.count(V)) {
              // If the scalar is already in the HLV, we don't need to add it again
              continue;
            }
            HLV->Size += computeSizeInBytes(V, SIMD, WI, *DL);
            HLV->LiveVars.insert(V);
            HangingLiveVars[V] = HLV;
          }

          Value *CurrentInstructionScalarOp = I->getOperand(1);
          bool CurrentScalarDies = operandDies(CurrentInstructionScalarOp, I);
          if (CurrentScalarDies) {
            HLV->LiveVars.erase(CurrentInstructionScalarOp);
          }

          if (HLV->LiveVars.empty()) {
            // If there are no live vars, we don't need to keep the HLV
            HangingLiveVarsVec.pop_back();
            HangingLiveVars.erase(FirstScalar);
            PrintDumpLevel(VerbosityLevel::High, " (no live vars, removing HLV as soon as it's created)");
          } else {
            PrintDumpLevel(VerbosityLevel::High, " (populating HLV with "
                                                     << HLV->LiveVars.size() << (CurrentScalarDies ? " remaining" : "")
                                                     << " scalars, vector size " << HLV->Size << ")");
            IGC_ASSERT(HangingLiveVars.count(FirstScalar));
          }
        }
      }

      else if (V2SP) {
        // 4. ExtractElement from a vector to scalars
        // The vector is not dead on the last EE, it will die on the last usage of the last EE
        // If the vector has uses apart from the ExtractElement instructions we also add it to the
        // HangingLiveVars
        auto *EE = cast<ExtractElementInst>(I);
        if (!HangingLiveVars.count(I)) {
          IGC_ASSERT(V2SP->getSourceVec() == EE->getVectorOperand());
          HangingLiveVarsVec.emplace_back(std::make_unique<HangingLiveVarsInfo>(
              computeSizeInBytes(V2SP->getSourceVec(), SIMD, WI, *DL), HANGING_VECTOR_TO_SCALARS));
          auto *HLV = HangingLiveVarsVec.back().get();
          for (Value *V : V2SP->getEEs()) {
            IGC_ASSERT(!HLV->LiveVars.count(V));
            if (V->hasNUndroppableUsesOrMore(1)) {
              HLV->LiveVars.insert(V);
              HangingLiveVars[V] = HLV;
            }
          }
          if (!V2SP->areAllUsesScalars()) {
            HangingLiveVars[V2SP->getSourceVec()] = HLV;
            HLV->LiveVars.insert(V2SP->getSourceVec());
            PrintDumpLevel(VerbosityLevel::High, " (adding vector " << getName(V2SP->getSourceVec()) << " to HLV)");
          }
          PrintDumpLevel(VerbosityLevel::High,
                         " (populating HLV with " << HLV->LiveVars.size() << " EEs, vector size " << HLV->Size << ")");
        }
      }
    }

    // Now we check the operands of the instruction
    // and see if they die on this instruction, decreasing the register pressure

    if (Update)
      PrintDumpLevel(VerbosityLevel::High, " | ");

    SmallSet<Value *, 8> SeenRealOps; // "Real" refer to that they are not no-ops.
                                      // We make sure we don't count the same op twice on the same instruction

    for (auto &Op : I->operands()) {
      Value *V = Op.get();
      Instruction *OpI = dyn_cast<Instruction>(V);

      if (!OpI && !isa<Argument>(V))
        continue;

      if (OpI && (IGCLLVM::isDebugOrPseudoInst(*OpI)))
        continue;

      Value *RealOp = getRealOp(V);

      if (Update)
        PrintDumpLevel(VerbosityLevel::High, getName(V) << " -> " << getName(RealOp));

      if (!SeenRealOps.count(RealOp) && operandDies(RealOp, I)) {
        int RPDecrease = computeSizeInBytes(RealOp, SIMD, WI, *DL);

        if (Update)
          PrintDumpLevel(VerbosityLevel::High, " (X)");

        if ((DTI && DTI->getSourceVec() == RealOp) || (V2SP && V2SP->getSourceVec() == RealOp)) {
          // This operand is the source vector of the instruction
          // It "hangs" - we'll check if it dies later

          if (Update)
            PrintDumpLevel(VerbosityLevel::High, " (source vector hangs)");
          RPDecrease = 0;
        }

        auto *DT = VSA->getDestVector(RealOp);
        if (DT) {
          if (DT->getLastIE() != RealOp) {
            // This op is not the last IE so it can't kill the hanging values

            if (Update)
              PrintDumpLevel(VerbosityLevel::High, " (not last IE, vector doesn't die)");
            RPDecrease = 0;
          }
          if (DT->isNoOp()) {
            // If the operand is part of No-Op vector shuffle
            // it can't neither increase nor decrease the regpressure
            // and can't kill the hanging vector

            if (Update)
              PrintDumpLevel(VerbosityLevel::High, " (no-op)");
            RPDecrease = 0;
          }
        }

        if (!Update) {
          if (DTI && !DTI->isVectorShuffle()) {
            // Creating vector out of scalars
            if ((DTI->getFirstIE() == I) && (I->getOperand(1) == V)) {
              // Hack: Only for estimation (non-update) we assume that
              // The scalar in the FirstIE doesn't die
              // Because it usually happens this way when we create a vector of size >1 from different
              // values

              // For Update case it will be estimated properly using the hanging live vars information
              RPDecrease = 0;
            }
          }
        }

        // Check if this operand also kills the "hanging" values
        if (HangingLiveVars.count(RealOp)) {
          auto HLV = HangingLiveVars[RealOp];

          if (HLV->LiveVars.count(RealOp) && HLV->LiveVars.size() == 1) // This op is the only live var left
          {
            if (Update)
              PrintDumpLevel(VerbosityLevel::High, " (hanging vector dies)");
            if (HLV->Type == HANGING_SCALARS_TO_VECTOR ||
                HLV->Type == HANGING_VECTOR_TO_SCALARS) {
              // only scalars die
              RPDecrease = HLV->Size;
            } else {
              // in the vector shuffle case it's possible that the subvector also dies
              RPDecrease += HLV->Size;
            }
          } else {
            if (Update)
              PrintDumpLevel(VerbosityLevel::High,
                             " (hanging vector, left vars: "
                                 << (HLV->LiveVars.count(RealOp) ? HLV->LiveVars.size() - 1 : HLV->LiveVars.size())
                                 << ")");
            if (HLV->Type == HANGING_SCALARS_TO_VECTOR ||
                HLV->Type == HANGING_VECTOR_TO_SCALARS) {
              RPDecrease = 0; // We don't decrease pressure, because the vector is still alive
            }
          }
          if (Update) {
            HLV->LiveVars.erase(RealOp);
          }
        }

        if (Update)
          PrintDumpLevel(VerbosityLevel::High, ": -" << RPDecrease << "   ");

        ResultSizeInBytes -= RPDecrease;
      } else {
        if (Update)
          PrintDumpLevel(VerbosityLevel::High, "   ");
      }

      SeenRealOps.insert(RealOp);
    }

    if (Update) {
      // Updating state if needed

      BBCurrent.insert(I);
      CurrentPressure += ResultSizeInBytes;

      if (is2dBlockRead(I)) {
        CurrentNumOf2dLoads++;
      }

      // Print log dump only on Update in order not to output duplicating information
      PrintDumpLevel(VerbosityLevel::High, "\n\n");
    }

    return ResultSizeInBytes;
  }

  bool isShuffled2dBlockRead(Instruction *I) {
    if (!is2dBlockRead(I)) {
      return false;
    }
    auto RealUses = getRealUses(I);
    for (auto *U : RealUses) {
      Instruction *UI = dyn_cast<Instruction>(U);
      if (!UI || (UI->getParent() != BB))
        return false;
      auto *DV = VSA->getDestVector(UI);
      if (!DV)
        return false;
      if (!DV->isVectorShuffle())
        return false;
      if (DV->isNoOp()) {
        // No-op vector shuffle does not increase register pressure
        return false;
      }
    }
    return true;
  }
};

// Main class for the local code scheduling

// Builds a dependency graph (DepGraph) representing instruction dependencies within the basic block.
// Uses a RegisterPressureTracker to estimate and track register usage as instructions are scheduled.
// Can perform multiple scheduling attempts with backtracking to find a schedule that avoids spills.

// Internal classes:
// - InstructionNode: Represents a node in the dependency graph for an instruction.
// - DepEdge: Represents a dependency edge between instructions.
// - DepGraph: Manages the dependency graph construction and traversal.
// - Schedule: Encapsulates a candidate instruction schedule and its state.

class BBScheduler {
  class DepEdge;
  class InstructionNode;
  class DepGraph;

public:
  using Option = SchedulingConfig::Option;

  static const int WEIGHT_NOT_SPECIFIED = std::numeric_limits<int>::min();

  typedef llvm::DenseMap<Instruction *, InstructionNode *> InstToNodeMap;
  typedef std::vector<std::unique_ptr<DepEdge>> DepEdgeList;
  typedef std::vector<InstructionNode> InstNodeList;
  typedef std::vector<InstructionNode *> InstNodePtrList;

  BBScheduler(BasicBlock *BB, IGCLivenessAnalysis *RPE, IGCFunctionExternalRegPressureAnalysis *FRPE, AAResults *AA,
              VectorShuffleAnalysis *VSA, RematChainsAnalysis *RCA, CodeGenContext *CTX, SchedulingConfig *Config, llvm::raw_ostream *LogStream)
      : BB(BB), RPE(RPE), FRPE(FRPE), AA(AA), VSA(VSA), RCA(RCA), CTX(CTX), C(*Config), LogStream(LogStream) {
    F = BB->getParent();
    WI = &FRPE->getWIAnalysis(F);
  }

  // Main function to schedule the instructions in a BB
  bool schedule() {
    bool Changed = false;

    std::string BBName = BB->getName().str();
    if (BBName.empty()) {
      BBName = "Unnamed";
    }
    PrintDump("Scheduling basic block " << BBName << "\n");

    // Check if the original schedule can have spills
    // Do nothing if the original schedule can not have spills and rescheduling is not forced

    RegisterPressureTracker RPT(BB, RPE, FRPE, VSA, RCA, WI, CTX, &C, LogStream);

    int32_t MaxOriginalRegpressure = 0;
    bool OriginalScheduleCanHaveSpills = false;

    PrintDump("Original schedule: " << BBName << "\n");
    for (auto &I : *BB) {
      std::string Info;
      if (isa<PHINode>(&I)) {
        // PHIs are already included in the initial regpressure
        Info = formatDebugInfo(RPT.getCurrentPressure(), 0, "Phi", getVectorShuffleString(&I, VSA, RCA));
      } else {
        int32_t Estimate = RPT.update(&I);
        Info = formatDebugInfo(RPT.getCurrentPressure(), Estimate, "OG", getVectorShuffleString(&I, VSA, RCA));
      }
      PrintDump(Info);
      PrintInstructionDump(&I);

      MaxOriginalRegpressure = std::max(MaxOriginalRegpressure, RPT.getCurrentPressure());
      if (RPT.isRegpressureCritical()) {
        OriginalScheduleCanHaveSpills = true;
      }
    }
    PrintDump("Max original regpressure: " << MaxOriginalRegpressure << "\n");

    if (!OriginalScheduleCanHaveSpills && !IGC_IS_FLAG_ENABLED(EnableCodeSchedulingIfNoSpills)) {
      PrintDump("Original schedule can not have spills, skipping scheduling\n");
      PrintDump("Schedule is not changed" << "\n");
      return false;
    }

    int NumGRF = RPT.getNumGRF();
    int ThresholdValue = NumGRF - static_cast<int>(IGC_GET_FLAG_VALUE(CodeSchedulingRPMargin)) +
                         static_cast<int>(IGC_GET_FLAG_VALUE(CodeSchedulingRPThreshold));
    if (MaxOriginalRegpressure < ThresholdValue) {
      PrintDump("Max original regpressure is below threshold: " << MaxOriginalRegpressure << " < " << ThresholdValue
                                                                << ", skipping scheduling\n");
      PrintDump("Schedule is not changed" << "\n");
      return false;
    }

    // Create a schedules stack and an initial empty schedule. It'll create a DepGraph.
    // Schedule is a copyable object, so we can make a copy to save a "checkpoint".

    std::vector<std::unique_ptr<Schedule>> Schedules;

    std::unique_ptr<Schedule> DefaultSchedule = std::make_unique<Schedule>(BB, RPE, FRPE, VSA, RCA, WI, CTX, &C, LogStream);

    // First try if "GreedyMW" scheduling can be applied
    // This approach prioritizes scheduling by the edge weights
    // To maximize hiding the instructions latency.

    // We'll commit it if it has no spills

    std::unique_ptr<Schedule> GreedyMWSchedule = std::make_unique<Schedule>(*DefaultSchedule);
    GreedyMWSchedule->setGreedyMW(true);

    if (!IGC_IS_FLAG_ENABLED(CodeSchedulingForceRPOnly)) {
      std::vector<std::unique_ptr<Schedule>> NewSchedules;
      PrintDump("Greedy MW attempt\n");

      while (!GreedyMWSchedule->isComplete()) {
        std::unique_ptr<Schedule> Checkpoint = GreedyMWSchedule->scheduleNextInstruction();
        if (Checkpoint) {
          NewSchedules.push_back(std::move(Checkpoint));
        }
      }

      if (IGC_IS_FLAG_ENABLED(CodeSchedulingForceMWOnly) || !GreedyMWSchedule->canEverHaveSpills()) {
        PrintDump("Greedy MW schedule is forced or has no spills.\n");
        if (((GreedyMWSchedule->getMaxRegpressure() > MaxOriginalRegpressure)) &&
          IGC_IS_FLAG_DISABLED(CodeSchedulingMWOptimizedHigherRPCommit))
        {
          PrintDump("Greedy MW schedule has higher regpressure that the original (" <<
                    GreedyMWSchedule->getMaxRegpressure() << " > " << MaxOriginalRegpressure <<
                    "), skipping commit\n");
          PrintDump("Schedule is not changed" << "\n");
          return false;
        }
        GreedyMWSchedule->commit();
        return true;
      }

      // push NewSchedules to Schedules in the reverse order
      for (auto It = NewSchedules.rbegin(); It != NewSchedules.rend(); ++It) {
        It->get()->setGreedyMW(false); // Reset the GreedyMW flag for the new schedules
        Schedules.push_back(std::move(*It));
      }
    }

    // Then try to apply "GreedyRP" scheduling
    // Schedule only for the pressure minimization
    // If it still has spills or is forced, we will commit it

    std::unique_ptr<Schedule> GreedyRPSchedule = nullptr;

    if(!IGC_IS_FLAG_ENABLED(CodeSchedulingForceRPOnly) && GreedyMWSchedule->isComplete() && GreedyMWSchedule->isEqualGreedyRP()) {
      PrintDump("Greedy MW schedule is equal to Greedy RP schedule, skipping Greedy RP attempt\n");
      GreedyRPSchedule = std::make_unique<Schedule>(*GreedyMWSchedule);
    } else {
      PrintDump("Greedy RP attempt\n");
      GreedyRPSchedule = std::make_unique<Schedule>(*DefaultSchedule);
      GreedyRPSchedule->setGreedyRP(true);
    }

    // PrintDump("DepGraph dump\n");
    // DepGraph G(BB, RPE, FRPE, VSA, RCA, WI, CTX, C, LogStream);
    // G.print(*LogStream);

    while (!GreedyRPSchedule->isComplete()) {
      GreedyRPSchedule->scheduleNextInstruction();
    }

    bool CanCompileWithNoSpills = !GreedyRPSchedule->canEverHaveSpills();

    if (IGC_IS_FLAG_ENABLED(CodeSchedulingForceRPOnly)) {
      PrintDump("Greedy RP schedule is forced\n");
      if (((GreedyRPSchedule->getMaxRegpressure() > MaxOriginalRegpressure)) &&
          IGC_IS_FLAG_DISABLED(CodeSchedulingGreedyRPHigherRPCommit)) {
        PrintDump("Greedy RP schedule has higher regpressure that the original (" <<
                  GreedyRPSchedule->getMaxRegpressure() << " > " << MaxOriginalRegpressure <<
                  "), skipping commit\n");
        PrintDump("Schedule is not changed" << "\n");
        return false;
      }
      PrintDump("Commiting RP schedule and stopping.\n")
      PrintDump("Schedule is changed" << "\n");
      GreedyRPSchedule->commit();
      return true;
    }

    // Try several attempts with backtracking to find the best schedule with no spills
    for (auto &S : Schedules) {
      S->setRefLiveIntervals(GreedyMWSchedule->getMaxLiveIntervals());
    }

    PrintDump("Schedules left in the queue: " << Schedules.size() << "\n");

    uint Attempt = 1;
    while (!Schedules.empty()) {
      Schedule *S = Schedules.back().get();
      PrintDump("Attempt #" << Attempt << "\n");

      std::vector<std::unique_ptr<Schedule>> NewSchedules;

      while (!S->isComplete()) {
        // Schedule the next instruction and add the checkpoint if it
        // returns the previous state
        std::unique_ptr<Schedule> Checkpoint = S->scheduleNextInstruction();
        if (Checkpoint) {
          NewSchedules.push_back(std::move(Checkpoint));
        }
        if (CanCompileWithNoSpills && S->canEverHaveSpills()) {
          break;
        }
      }

      bool Success = S->isComplete() && !S->canEverHaveSpills();
      if (Success) {
        PrintDump("Schedule is complete\n");
        if (((S->getMaxRegpressure() > MaxOriginalRegpressure)) &&
            IGC_IS_FLAG_DISABLED(CodeSchedulingMWOptimizedHigherRPCommit)) {
          PrintDump("Completed schedule on attempt #" << Attempt << " has higher regpressure that the original (" <<
                    S->getMaxRegpressure() << " > " << MaxOriginalRegpressure <<
                    "), skipping commit\n");
          PrintDump("Schedule is not changed" << "\n");
          return false;
        }
        S->commit();
        Changed = true;
        break;
      } else {
        PrintDump("Schedule of attempt #" << Attempt << " is not complete\n");
        PrintDump("Can ever have spills? " << S->canEverHaveSpills() << "\n");
        PrintDump("Can compile with no spills? " << CanCompileWithNoSpills << "\n");
        Schedules.pop_back();

        // push NewSchedules to Schedules in the reverse order
        for (auto It = NewSchedules.rbegin(); It != NewSchedules.rend(); ++It) {
          Schedules.push_back(std::move(*It));
        }

        PrintDump("Schedules left in the queue: " << Schedules.size() << "\n");
      }
      if (Attempt > static_cast<int>(IGC_GET_FLAG_VALUE(CodeSchedulingAttemptsLimit))) {
        PrintDump("Attempts limit reached\n");
        break;
      }
      Attempt++;
    };

    if (!Changed && IGC_IS_FLAG_ENABLED(CodeSchedulingCommitGreedyRP) && OriginalScheduleCanHaveSpills) {
      PrintDump("No schedule is complete, so GreedyRP schedule is the best.\n");
      if (((GreedyRPSchedule->getMaxRegpressure() > MaxOriginalRegpressure)) &&
          IGC_IS_FLAG_DISABLED(CodeSchedulingGreedyRPHigherRPCommit)) {
        PrintDump("Greedy RP schedule has higher regpressure that the original (" <<
                  GreedyRPSchedule->getMaxRegpressure() << " > " << MaxOriginalRegpressure <<
                  "), skipping commit\n");
        PrintDump("Schedule is not changed" << "\n");
        return false;
      }
      PrintDump("Commiting Greedy RP schedule as the best one.\n");
      PrintDump("Schedule is changed" << "\n");
      GreedyRPSchedule->commit();
      Changed = true;
    }

    PrintDump("Schedule is " << (Changed ? "changed" : "not changed") << "\n");

    return Changed;
  }

private:
  BasicBlock *BB;
  Function *F;
  IGCFunctionExternalRegPressureAnalysis *FRPE;
  IGCLivenessAnalysis *RPE;
  WIAnalysisRunner *WI;
  AAResults *AA;
  VectorShuffleAnalysis *VSA;
  CodeGenContext *CTX;
  RematChainsAnalysis *RCA;
  SchedulingConfig &C;
  llvm::raw_ostream *LogStream;

  // Helper function to format debug information string
  static std::string formatDebugInfo(int32_t CurrentPressure, int32_t Estimate, const std::string Type,
                                     const std::string AddString = "") {
    const int ESTIMATION_NUMBERS_WIDTH = 12;
    const int INFO_WIDTH = 20;
    std::string Info = std::to_string(CurrentPressure) + ", " + std::to_string(Estimate);
    Info.resize(ESTIMATION_NUMBERS_WIDTH, ' ');
    Info = "(" + Info + ") " + Type + ": ";
    Info.resize(INFO_WIDTH, ' ');

    if (!AddString.empty()) {
      Info += AddString;
    }

    return Info;
  }

  // Helper function to get vector shuffle string
  static std::string getVectorShuffleString(Instruction *I, VectorShuffleAnalysis *VSA, RematChainsAnalysis *RCA) {
    auto *DT = VSA->getDestVector(I);
    auto *V2SP = VSA->getVectorToScalarsPattern(I);
    auto *RCP = RCA->getRematChainPattern(I);

    std::string VS_String = "    ";
    if (RCP) {
      VS_String = "REM ";
    } else if (DT && DT->isNoOp()) {
      VS_String = "NOP ";
    } else if (DT && DT->isVectorShuffle()) {
      VS_String = "VS  ";
    } else if (DT && !DT->isVectorShuffle()) {
      VS_String = "SCA ";
    } else if (V2SP) {
      VS_String = "V2S ";
    }

    return VS_String;
  }

  class InstructionNode {
  public:
    InstructionNode(Instruction *I, uint32_t N) : I(I), OriginalPosition(N) {
      MaxWeight = WEIGHT_NOT_SPECIFIED;
      MaxWeightHighRP = WEIGHT_NOT_SPECIFIED;
    }

    InstructionNode(Instruction *I, uint32_t N, int32_t MW, int32_t MWHighRP)
        : I(I), OriginalPosition(N), MaxWeight(MW), MaxWeightHighRP(MWHighRP) {}

    Instruction *I;
    uint32_t OriginalPosition;
    int32_t MaxWeight;
    int32_t MaxWeightHighRP;
    llvm::DenseSet<DepEdge *> Preds;
    llvm::DenseSet<DepEdge *> Succs;
    llvm::SmallSetVector<Instruction *, 8> RealUses;

    void print(llvm::raw_ostream &LogStream) {
      if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling)) {
        const int INFO_WIDTH = 16;
        std::string Info = "#" + std::to_string(OriginalPosition) + ", MW: " + std::to_string(MaxWeight) + " ";
        Info.resize(INFO_WIDTH, ' ');
        LogStream << Info;
        I->print(LogStream);
        LogStream << "\n";
      }
    }

    void printSuccessors(llvm::raw_ostream &LogStream) {
      if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling)) {
        if (Succs.size() > 0) {
          LogStream << "Successors: \n";
          for (const auto &Succ : Succs) {
            Succ->print(LogStream);
          }
        }
      }
    }
  };

  class DepEdge {
  public:
    DepEdge(InstructionNode *Src, InstructionNode *Dst, int32_t Weight, bool ForceSubsequent)
        : Src(Src), Dst(Dst), Weight(Weight), WeightHighRP(Weight), ForceSubsequent(ForceSubsequent), Deleted(false) {}

    DepEdge(InstructionNode *Src, InstructionNode *Dst, int32_t Weight, int32_t WeightHighRP, bool ForceSubsequent)
        : Src(Src), Dst(Dst), Weight(Weight), WeightHighRP(WeightHighRP), ForceSubsequent(ForceSubsequent),
          Deleted(false) {}

    InstructionNode *Src;
    InstructionNode *Dst;
    int32_t Weight;
    int32_t WeightHighRP;
    bool ForceSubsequent;
    bool Deleted;

    void print(llvm::raw_ostream &LogStream) {
      if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling)) {
        if (!Deleted) {
          LogStream << "  ";
          Src->print(LogStream);
          LogStream << "  ";
          LogStream << " ->(" << Weight << ")-> ";
          LogStream << "  ";
          Dst->print(LogStream);
          LogStream << "\n";
        }
      }
    }
  };

  // The DepGraph builds in the constructor
  // Then its fields can be used directly
  class DepGraph {
  public:
    InstToNodeMap InstToNode;
    InstNodeList InstNodes;
    DepEdgeList DepEdges;

    DepGraph() {}

    DepGraph(const DepGraph &) = delete;
    DepGraph &operator=(const DepGraph &) = delete;

    DepGraph(BasicBlock *BB, IGCLivenessAnalysis *RPE, IGCFunctionExternalRegPressureAnalysis *FRPE,
             VectorShuffleAnalysis *VSA, RematChainsAnalysis *RCA, WIAnalysisRunner *WI, CodeGenContext *CTX, SchedulingConfig &C,
             llvm::raw_ostream *LogStream) {
      InstNodes.reserve(BB->size() * sizeof(InstructionNode));
      InstToNode.reserve(BB->size() * sizeof(InstToNodeMap));

      // Create InstNodes and InstToNode from BB instructions
      auto N = 0;
      for (auto &I : *BB) {
        if (isa<PHINode>(&I)) {
          continue;
        }

        InstNodes.emplace_back(&I, N++);
        InstToNode[&I] = &InstNodes.back();
      }

      auto addEdge = [&](Instruction *Src, Instruction *Dst, int Weight = WEIGHT_NOT_SPECIFIED,
                         int WeightHighRP = WEIGHT_NOT_SPECIFIED, bool ForceSubsequent = false) {
        IGC_ASSERT(Src && Dst);
        if (Src == Dst) {
          return;
        }
        if (Weight == WEIGHT_NOT_SPECIFIED) {
          Weight = C[Option::DefaultWeight];
        }
        if (WeightHighRP == WEIGHT_NOT_SPECIFIED) {
          WeightHighRP = Weight;
        }
        if (InstToNode.count(Src) && InstToNode.count(Dst)) {
          DepEdges.emplace_back(
              std::make_unique<DepEdge>(InstToNode[Src], InstToNode[Dst], Weight, WeightHighRP, ForceSubsequent));
          InstToNode[Src]->Succs.insert(DepEdges.back().get());
          InstToNode[Dst]->Preds.insert(DepEdges.back().get());
        }
      };

      auto isNoOpSingleElementVectorEE = [&](Instruction *I) -> bool {
        if (auto *EE = dyn_cast<ExtractElementInst>(I)) {
          if (auto *VectorType = dyn_cast<IGCLLVM::FixedVectorType>(EE->getVectorOperand()->getType())) {
            if (VectorType->getNumElements() == 1 && VectorType->getElementType()->isSingleValueType()) {
              return true;
            }
          }
        }
        return false;
      };

      std::vector<Instruction *> UnknownStores;
      std::vector<Instruction *> AllMemoryAccesses;

      // Structures to track non-ssa dependencies of the decomposed loads
      DenseMap<Instruction *, llvm::SmallVector<Instruction *, 32>> Prev2DBlockReadPayloads;
      DenseMap<Instruction *, DenseMap<uint32_t, Instruction *>> Last2DBlockSetAddrPayloadField;

      // Returns the size of the load in bytes for simple cases (vector of
      // single value type)
      // TODO handle more complex cases
      auto getLoadSize = [&](GenIntrinsicInst *Intr) -> uint32_t {
        auto VectorType = dyn_cast<IGCLLVM::FixedVectorType>(Intr->getType());
        if (!VectorType)
          return 0;
        auto ElemType = VectorType->getElementType();
        if (!ElemType->isSingleValueType())
          return 0;
        uint32_t ElemSize = ElemType->getPrimitiveSizeInBits() / 8;
        uint32_t NumElements = VectorType->getNumElements();
        return NumElements * ElemSize;
      };

      auto getSSAEdgeWeight = [&](Instruction *Src, Instruction *Dst, bool HighRP = false) {
        if (IsExtendedMathInstruction(Src)) {
          return C[Option::WeightExtendedMathDstDep];
        }
        if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(Src)) {
          if (isDPAS(Src)) {
            return HighRP ? C[Option::WeightDPASDstDepHighRP] : C[Option::WeightDPASDstDep];
          }
          switch (Intr->getIntrinsicID()) {
          case GenISAIntrinsic::GenISA_LSC2DBlockRead:
          case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload: {
            int AdditionalWeight =
                C[Option::LoadSizeAdditionalWeight] * C[Option::LoadSizeWeightFactor] * getLoadSize(Intr);
            return (HighRP ? C[Option::Weight2dBlockReadDstDepHighRP] : C[Option::Weight2dBlockReadDstDep]) +
                   AdditionalWeight;
          }
          case GenISAIntrinsic::GenISA_WaveAll:
            return HighRP ? C[Option::WeightWaveAllDstDepHighRP] : C[Option::WeightWaveAllDstDep];
          default:
            break;
          }
        }
        if (Src->mayReadFromMemory()) {
          return C[Option::WeightUnknownMemoryReadDstDep];
        }
        return C[Option::DefaultWeight];
      };

      // Stage 1. Creating the dependencies
      for (auto &I : *BB) {
        if (isa<PHINode>(&I)) {
          continue;
        }

        // 1.1. Tracking the SSA dependencies
        for (auto &Op : I.operands()) {
          if (Instruction *OpI = dyn_cast<Instruction>(Op)) {
            auto *Src = OpI;
            auto *Dst = &I;

            int Weight = getSSAEdgeWeight(Src, Dst, false);
            int WeightHighRP = getSSAEdgeWeight(Src, Dst, true);
            bool ForceSubsequent = false;

            // Place Noop instructions right after the source
            // Look through them to find the latency of the real source
            if (isNoOpInst(Src, CTX)) {
              if (Src->getNumOperands() == 1) {
                if (Instruction *SrcOp = dyn_cast<Instruction>(Src->getOperand(0))) {
                  Weight = getSSAEdgeWeight(SrcOp, Dst, false);
                  WeightHighRP = getSSAEdgeWeight(SrcOp, Dst, true);
                }
              }
            }

            DestVector *SrcDV = VSA->getDestVector(Src);
            if (SrcDV && (Src == cast<Instruction>(SrcDV->getLastIE()))) {
              // Edge from the last IE of the vector shuffle to the real user
              if (SrcDV->isNoOp()) {
                // Use weight from the source vec instruction
                Instruction *SourceVecInstruction = dyn_cast<Instruction>(SrcDV->getSourceVec());
                Weight = SourceVecInstruction == nullptr ? 0 : getSSAEdgeWeight(SourceVecInstruction, Dst, false);
                WeightHighRP = SourceVecInstruction == nullptr ? 0 : getSSAEdgeWeight(SourceVecInstruction, Dst, true);
              } else {
                // Use the default weight for the vector shuffle
                Weight = C[Option::WeightUnknownVectorShuffleDstDep];
                WeightHighRP = C[Option::WeightUnknownVectorShuffleDstDep];
              }
            }

            RematChainPattern *RCP = RCA->getRematChainPattern(Src);
            if (RCP) {
              if (RCP->isRematInst(Dst) || (RCP->getRematTargetInst() == Dst)) {
                ForceSubsequent = true;
              }
            }

            // Edge from some instruction TO the no-op or vector shuffle
            // Weight is 0 and it makes sense to place it right after the source

            // Note: for the case of transforming vector shuffle the transforming movs should not always
            // follow the source (which is usually a block load). Proper handling of this case is
            // unsupported, for now we'l always place it right away. The induces register pressure should be
            // tracked by the RegisterPressureTracker correctly.

            DestVector *DstDV = VSA->getDestVector(Dst);
            VectorToScalarsPattern *V2SP = VSA->getVectorToScalarsPattern(Dst);
            if (IGCLLVM::isDebugOrPseudoInst(*Dst) || Dst->isLifetimeStartOrEnd() || isNoOpInst(Dst, CTX) ||
                (DstDV && (DstDV->isNoOp())) || (DstDV && (DstDV->isVectorShuffle()) && !DstDV->isNoOp()) ||
                (DstDV && !DstDV->isVectorShuffle()) || V2SP || isNoOpSingleElementVectorEE(Dst)) {
              Weight = 0;
              WeightHighRP = 0;
              ForceSubsequent = true;
            }

            addEdge(OpI, &I, Weight, WeightHighRP, ForceSubsequent);
          }
        }

        // 1.2. Tracking the non-SSA dependencies: decomposed loads, memory dependencies and so on

        // For now it's not needed to track if the memory can alias, we don't use AliasAnalysis
        // We just don't move loads across stores and don't change the order of the stores.

        // The mechanism for that is adding "fake" edges:
        // - from any memory access to the unknown store
        // - from the unknown store to any memory access

        // Unknown stores: some of the instructions, like GenISA_LSC2DBlockSetAddrPayloadField are marked as
        // stores in order to be handled by LLVM passes conservatively, but they are essentially not stores, we
        // know we can move them. We only restrict moving around of the "unknown" stores.

        bool isUnknownStore =
            I.mayWriteToMemory(); // first set the flag then it may be revoked if moving of the store is safe
        bool isPrefetch = false;

        if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(&I)) {
          switch (Intr->getIntrinsicID()) {

          case GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField: {
            Instruction *Payload = cast<Instruction>(Intr->getOperand(0));
            uint32_t Field = cast<ConstantInt>(Intr->getOperand(1))->getZExtValue();
            Last2DBlockSetAddrPayloadField[Payload][Field] = &I;
            // Every 2DBlockSetAddrPayloadField depends on the previous 2DBlockReads with the same payload
            for (auto &PrevBlockRead : Prev2DBlockReadPayloads[Payload]) {
              addEdge(PrevBlockRead, &I, C[Option::Weight2dBlockReadSrcDep], C[Option::Weight2dBlockReadSrcDep]);
            }
            isUnknownStore = false;
            break;
          }

          case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch:
            isPrefetch = true;
            isUnknownStore = false;
            break;

          case GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload:
            isPrefetch = true;
            // -- no break intentionally --
          case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload: {
            Instruction *Payload = cast<Instruction>(Intr->getOperand(0));
            // Every 2dBlockReadPayload depends on all the previous SetAddrPayloadField for every payload
            // field number
            for (auto &Field : Last2DBlockSetAddrPayloadField[Payload]) {
              addEdge(Field.second, &I, C[Option::Weight2dBlockSetPayloadFieldDstDep],
                      C[Option::Weight2dBlockSetPayloadFieldDstDep]);
            }
            Prev2DBlockReadPayloads[Payload].push_back(&I);
            isUnknownStore = false;
            break;
          }

          case GenISAIntrinsic::GenISA_LSC2DBlockRead:
          case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload: {
            isUnknownStore = false;
            break;
          }

          case GenISAIntrinsic::GenISA_WaveAll:
          case GenISAIntrinsic::GenISA_ftobf:
            isUnknownStore = false;
            break;

          default:
            break;
          }
        }

        if (isDPAS(&I)) {
          isUnknownStore = false;
        }

        if (isUnknownStore || isPrefetch) {
          if (isUnknownStore) {
            PrintDumpLevel(VerbosityLevel::High, "Unknown store:\n");
          } else {
            PrintDumpLevel(VerbosityLevel::High, "Prefetch:\n");
          }
          PrintInstructionDumpLevel(VerbosityLevel::High, &I);

          UnknownStores.push_back(&I);

          // Every unknown store depends on all the memory accesses
          // We also assume the same for the prefetch in order to preserve its place
          for (auto &MemAccess : AllMemoryAccesses) {
            if (isDPAS(MemAccess) && isPrefetch) {
              // Don't add the edge from the DPAS to the prefetch, prefetch benefits from being
              // executed earlier
              continue;
            }
            addEdge(MemAccess, &I, 0, 0);
          }
        }

        Instruction *Terminator = BB->getTerminator();

        // Terminator "depends" on all the instructions - they need to
        // be placed before
        if ((&I != Terminator) && (!isPrefetch)) {
          addEdge(&I, Terminator, C[Option::AddWeightToTerminatorEdge] ? getSSAEdgeWeight(&I, Terminator, false) : 0,
                  C[Option::AddWeightToTerminatorEdge] ? getSSAEdgeWeight(&I, Terminator, true) : 0);
        }

        if (isPrefetch) {
          // Prefetch should be placed before terminator and in advance, so use its weight
          addEdge(&I, Terminator, C[Option::WeightPrefetch], C[Option::WeightPrefetch]);

          // And for now we preserve the position of the prefetch, so let's say it depends on all the known
          // memory accesses
          for (auto &MemAccess : AllMemoryAccesses) {
            addEdge(MemAccess, &I, C[Option::WeightPrefetch], C[Option::WeightPrefetch]);
          }
        }

        if (I.mayReadOrWriteMemory() && !isPrefetch) {
          // Every memory access depends on all the unknown stores
          // Can be further relaxed with checking alias information
          for (auto &UnknownStore : UnknownStores) {
            addEdge(UnknownStore, &I, 0, 0);
          }

          AllMemoryAccesses.push_back(&I);
        }
      }

      PrintDumpLevel(VerbosityLevel::Medium, "Total nodes: " << InstNodes.size() << "\n");
      PrintDumpLevel(VerbosityLevel::Medium, "Total edges: " << DepEdges.size() << "\n");

      // Stage 2. Calculating MaxWeight for every node
      // iterate over the nodes in the backward order
      for (auto &Node : llvm::reverse(InstNodes)) {
        if (Node.Succs.empty()) {
          Node.MaxWeight = 0;
          Node.MaxWeightHighRP = 0;
        } else {
          int32_t MW = 0;
          int32_t MWHighRP = 0;
          for (const auto &Succ : Node.Succs) {
            MW = std::max(MW, Succ->Weight + Succ->Dst->MaxWeight);
            MWHighRP = std::max(MWHighRP, Succ->WeightHighRP + Succ->Dst->MaxWeightHighRP);
          }
          Node.MaxWeight = MW;
          Node.MaxWeightHighRP = MWHighRP;
        }
      }

      if (IGC_GET_FLAG_VALUE(CodeSchedulingDumpLevel) >= VerbosityLevel::High) {
        PrintDumpLevel(VerbosityLevel::High, "Dependency graph dump:\n");
        this->print(*LogStream);
      }
    }

    void print(llvm::raw_ostream &LogStream) {
      if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling)) {
        for (auto &Node : InstNodes) {
          Node.print(LogStream);
          Node.printSuccessors(LogStream);
          LogStream << "\n";
        }
      }
    }
  };

  // The Schedule class represents a candidate schedule for the instructions in a basic block.
  // It is copyable.
  // Function "scheduleNextInstruction" selects the next instruction to schedule based on the current state of the
  // schedule. It may return the old Schedule (before adding this instruction) that can be used as a checkpoint for
  // backtracking.

  class Schedule {
  public:
    Schedule(BasicBlock *BB, IGCLivenessAnalysis *RPE, IGCFunctionExternalRegPressureAnalysis *FRPE,
             VectorShuffleAnalysis *VSA, RematChainsAnalysis *RCA, WIAnalysisRunner *WI, CodeGenContext *CTX, SchedulingConfig *C,
             llvm::raw_ostream *LogStream)
        : BB(BB), C(*C), CTX(CTX), VSA(VSA), RCA(RCA), LogStream(LogStream),
          G(DepGraph(BB, RPE, FRPE, VSA, RCA, WI, CTX, *C, LogStream)),
          RT(RegisterPressureTracker(BB, RPE, FRPE, VSA, RCA, WI, CTX, C, LogStream)) {
      // init ready list
      for (auto &Node : G.InstNodes) {
        if (Node.Preds.empty()) {
          ReadyList.push_back(&Node);
        }
      }

      IGC_ASSERT(this->VSA->getDestVector(BB->getTerminator()) == nullptr);
    }

    Schedule &operator=(const Schedule &) = delete;
    ~Schedule() = default;

    // Copy constructor for Schedule
    Schedule(const Schedule &S)
        : LogStream(S.LogStream), RT(S.RT), // RT is copyable
          BB(S.BB), C(S.C), CTX(S.CTX), VSA(S.VSA), RCA(S.RCA), Handicapped(S.Handicapped), GreedyRP(S.GreedyRP),
          GreedyMW(S.GreedyMW), RegpressureWasCritical(S.RegpressureWasCritical), RefLiveIntervals(S.RefLiveIntervals) {
      G.InstNodes.reserve(S.G.InstNodes.size());
      G.DepEdges.reserve(S.G.DepEdges.size());

      // Deep clone G and remap the nodes
      llvm::DenseMap<const InstructionNode *, InstructionNode *> NodeMap;
      for (auto &Node : S.G.InstNodes) {
        G.InstNodes.emplace_back(Node.I, Node.OriginalPosition, Node.MaxWeight, Node.MaxWeightHighRP);
        G.InstToNode[Node.I] = &G.InstNodes.back();
        NodeMap[&Node] = &G.InstNodes.back();
      }

      for (auto &Edge : S.G.DepEdges) {
        if (Edge->Deleted) {
          continue;
        }
        G.DepEdges.emplace_back(std::make_unique<DepEdge>(NodeMap[Edge->Src], NodeMap[Edge->Dst], Edge->Weight,
                                                          Edge->WeightHighRP, Edge->ForceSubsequent));
        NodeMap[Edge->Src]->Succs.insert(G.DepEdges.back().get());
        NodeMap[Edge->Dst]->Preds.insert(G.DepEdges.back().get());
      }

      for (InstructionNode *Node : S.ReadyList) {
        ReadyList.push_back(NodeMap[Node]);
      }

      for (InstructionNode *Node : S.ImmediateReadyList) {
        ImmediateReadyList.push_back(NodeMap[Node]);
      }

      for (InstructionNode *Node : S.ScheduledList) {
        ScheduledList.push_back(NodeMap[Node]);
      }

      IGC_ASSERT(VSA->getDestVector(BB->getTerminator()) == nullptr);
    }

    // Schedule next instruction and maybe return the previous checkpoint
    std::unique_ptr<Schedule> scheduleNextInstruction() {
      std::unique_ptr<Schedule> Checkpoint = nullptr;

      auto ChosenNode = chooseReadyInstruction();

      InstructionNode *Node = std::get<0>(ChosenNode);
      bool CanClone = std::get<1>(ChosenNode);
      if (CanClone) {
        bool NeedToClone = needToClone(Node, !GreedyMW);
        if (NeedToClone) {
          Checkpoint = std::make_unique<Schedule>(*this);
          Checkpoint->addHandicapped(Node->I, RT.getCurrentPressure());
        }
      }

      ImmediateReadyList.erase(std::remove(ImmediateReadyList.begin(), ImmediateReadyList.end(), Node),
                               ImmediateReadyList.end());
      ReadyList.erase(std::remove(ReadyList.begin(), ReadyList.end(), Node), ReadyList.end());
      Handicapped.erase(Node->I);

      ScheduledList.push_back(Node);
      RT.update(Node->I);
      MaxRegpressure = std::max(MaxRegpressure, RT.getCurrentPressure());
      if (RT.isRegpressureCritical()) {
        RegpressureWasCritical = true;
      }

      std::vector<DepEdge *> ToErase;
      for (const auto &Succ : Node->Succs) {
        Succ->Deleted = true;
        Succ->Dst->Preds.erase(Succ);
        if (Succ->Dst->Preds.empty()) {
          if (Succ->ForceSubsequent) {
            ImmediateReadyList.push_back(Succ->Dst);
          } else {
            ReadyList.push_back(Succ->Dst);
          }
        }
      }


      return std::move(Checkpoint);
    }

    bool isComplete() { return ScheduledList.size() == G.InstNodes.size(); }

    bool canHaveSpills() { return RT.isRegpressureCritical(); }

    bool canEverHaveSpills() { return RegpressureWasCritical; }

    int32_t getMaxRegpressure() { return MaxRegpressure; }

    bool isEqualGreedyRP() { return GreedyRP || AllInstructionsScheduledByRP; }

    void setGreedyRP(bool Greedy) { GreedyRP = Greedy; }

    void setGreedyMW(bool Greedy) { GreedyMW = Greedy; }

    void addHandicapped(Instruction *I, int RP) { Handicapped[I] = RP; }

    void setRefLiveIntervals(const DenseMap<Instruction *, int32_t> &Intervals) { RefLiveIntervals = Intervals; }

    void commit() {
      // Reorder the real LLVM instructions
      Instruction *InsertPoint = nullptr;
      for (auto &Node : ScheduledList) {
        if (!InsertPoint) {
          Node->I->moveBefore(&*BB->getFirstInsertionPt());
        } else {
          Node->I->moveAfter(InsertPoint);
        }
        InsertPoint = Node->I;
      }
      PrintDump("Commited the schedule\n");
    }

    void print() {
      if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling)) {
        for (auto &Node : ScheduledList) {
          Node->print(*LogStream);
          Node->printSuccessors(*LogStream);
        }
      }
    }

    DenseMap<Instruction *, int32_t> getMaxLiveIntervals() {
      DenseMap<Instruction *, int32_t> NewPositions;
      int32_t CurrentPos = 0;
      for (auto &Node : ScheduledList) {
        NewPositions[Node->I] = CurrentPos;

        if (isa<InsertElementInst>(Node->I) || isa<ExtractElementInst>(Node->I)) {
          continue;
        }
        if (isNoOpInst(Node->I, CTX)) {
          continue;
        }
        if (isDbgIntrinsic(Node->I)) {
          continue;
        }

        CurrentPos++;
      }

      DenseMap<Instruction *, int32_t> MaxLiveIntervals;
      for (auto &Node : ScheduledList) {
        for (auto *U : RT.getRealUses(Node->I)) {
          Instruction *UI = dyn_cast<Instruction>(U);
          if (!UI) {
            continue;
          }
          InstructionNode *UNode = G.InstToNode[UI];
          if (!UNode) {
            continue;
          }
          int32_t NewLiveInterval = NewPositions[UI] - NewPositions[Node->I];
          MaxLiveIntervals[Node->I] = std::max(MaxLiveIntervals[Node->I], NewLiveInterval);
        }
      }

      return std::move(MaxLiveIntervals);
    }

  private:
    llvm::raw_ostream *LogStream;
    DepGraph G;
    RegisterPressureTracker RT;
    BasicBlock *BB;
    SchedulingConfig &C;
    VectorShuffleAnalysis *VSA;
    RematChainsAnalysis *RCA;
    CodeGenContext *CTX;

    InstNodePtrList ScheduledList;
    InstNodePtrList ReadyList;
    InstNodePtrList ImmediateReadyList; // Immediate ready list is a list of ready instruction that should be
                                        // scheduled immediately Not the list of constant values.

    llvm::DenseMap<Instruction *, int>
        Handicapped; // Handicapped instructions that should be scheduled as late as possible

    bool GreedyRP = false;
    bool GreedyMW = false;
    bool RegpressureWasCritical = false;
    bool AllInstructionsScheduledByRP = true;
    int32_t MaxRegpressure = 0;

    DenseMap<Instruction *, int32_t> RefLiveIntervals;

    // Returns the chosen instruction and if it's possible to clone the schedule
    std::tuple<InstructionNode *, bool> chooseReadyInstruction() {
      auto getLowestRegpressureNodes = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        IGC_ASSERT(Nodes.size() > 0);
        if (Nodes.size() == 1) {
          return Nodes;
        }
        // Sort in ascending order using RT->estimate(Node->I) as a key
        std::sort(Nodes.begin(), Nodes.end(),
                  [&](InstructionNode *A, InstructionNode *B) { return RT.estimate(A->I) < RT.estimate(B->I); });
        int32_t LowestRP = RT.estimate(Nodes.front()->I);
        InstNodePtrList LowestRPNodes;
        if (C[Option::AllowLargerRPWindowRPThreshold] > 0 &&
            LowestRP >= static_cast<int32_t>(C[Option::AllowLargerRPWindowRPThreshold])) {
            // If the lowest RP is larger than the threshold, we can allow larger RP window
            LowestRP += static_cast<int32_t>(C[Option::AllowLargerRPWindowSize]);
        }
        for (InstructionNode *Node : Nodes) {
          if (RT.estimate(Node->I) <= LowestRP) {
            LowestRPNodes.push_back(Node);
          } else {
            break;
          }
        }
        Nodes = std::move(LowestRPNodes);
        return Nodes;
      };

      auto getMaxWeightNodes = [&](InstNodePtrList &Nodes, bool UseHighRPWeight = false) -> InstNodePtrList & {
        IGC_ASSERT(Nodes.size() > 0);
        if (Nodes.size() == 1) {
          return Nodes;
        }
        // Sort in descending order of MaxWeight
        std::sort(Nodes.begin(), Nodes.end(), [&](InstructionNode *A, InstructionNode *B) {
          return UseHighRPWeight ? A->MaxWeightHighRP > B->MaxWeightHighRP : A->MaxWeight > B->MaxWeight;
        });
        auto MaxWeight = UseHighRPWeight ? Nodes.front()->MaxWeightHighRP : Nodes.front()->MaxWeight;
        InstNodePtrList MaxWeightNodes;
        for (InstructionNode *Node : Nodes) {
          if (UseHighRPWeight ? Node->MaxWeightHighRP == MaxWeight : Node->MaxWeight == MaxWeight) {
            MaxWeightNodes.push_back(Node);
          } else {
            break;
          }
        }
        Nodes = std::move(MaxWeightNodes);
        return Nodes;
      };

      auto getFirstNode = [&](InstNodePtrList &Nodes) {
        IGC_ASSERT(Nodes.size() > 0);
        if (Nodes.size() == 1) {
          return Nodes.front();
        }
        // return the node with the lowest OriginalPosition
        auto FirstNode = Nodes.front();
        for (InstructionNode *Node : Nodes) {
          if (Node->OriginalPosition < FirstNode->OriginalPosition) {
            FirstNode = Node;
          }
        }
        return FirstNode;
      };

      auto getLargeBlockLoadsIfExist = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        InstNodePtrList LargeBlockLoads;
        for (InstructionNode *Node : Nodes) {
          if (is2dBlockRead(Node->I)) {
            auto *VectorType = dyn_cast<IGCLLVM::FixedVectorType>(Node->I->getType());
            if (VectorType) {
              if ((C[Option::PrioritizeLargeBlockLoadsInRP] > 0) &&
                  (static_cast<int>(VectorType->getNumElements()) >= C[Option::PrioritizeLargeBlockLoadsInRP])) {
                LargeBlockLoads.push_back(Node);
              }
            }
          }
        }
        if (LargeBlockLoads.size() > 0) {
          Nodes = std::move(LargeBlockLoads);
        }
        return Nodes;
      };

      auto getRealOpThroughVS = [&](Instruction *I) -> Instruction * {
        Instruction *OpI = dyn_cast<Instruction>(RT.getRealOp(I));
        if (!OpI) {
          return nullptr;
        }
        auto *DV = VSA->getDestVector(OpI);
        if (DV && DV->isVectorShuffle()) {
          auto *SourceVec = dyn_cast<Instruction>(DV->getSourceVec());
          if (!SourceVec) {
            return nullptr;
          }
          return dyn_cast<Instruction>(RT.getRealOp(SourceVec));
        }
        return OpI;
      };

      std::function<llvm::DenseSet<Value *>(Instruction *)> getRealUsesThroughVS;
      getRealUsesThroughVS = [&](Instruction *I) -> llvm::DenseSet<Value *> {
        llvm::DenseSet<Value *> Uses;

        std::function<void(Value *)> collectUses = [&](Value *V) {
          for (auto *U : RT.getRealUses(V)) {
            auto *DV = VSA->getDestVector(U);
            if (DV && DV->isVectorShuffle()) {
              collectUses(DV->getLastIE());
            } else {
              Uses.insert(U);
            }
          }
        };

        collectUses(I);
        return Uses;
      };

      std::function<llvm::DenseSet<Value *>(Instruction *)> getRealUsesThroughRematChains;
      getRealUsesThroughRematChains = [&](Instruction *I) -> llvm::DenseSet<Value *> {
        llvm::DenseSet<Value *> Uses;

        std::function<void(Value *)> collectUses = [&](Value *V) {
          for (auto *U : RT.getRealUses(V)) {
            auto *RematChainPattern = RCA->getRematChainPattern(U);
            if (RematChainPattern) {
              // If the use is a remat chain, collect the last instruction in the chain
              Uses.insert(RematChainPattern->getRematTargetInst());
            } else {
              Uses.insert(U);
            }
          }
        };

        collectUses(I);
        return Uses;
      };

      auto getLoadsThatUnlockDPASes = [&](InstNodePtrList &Nodes, uint MaxLoadSize) -> InstNodePtrList & {
        // We first prioritize the DPASes that don't increase regpressure
        // if there are loads that unlock these DPASes - filter out all ther instructions
        // But if there are no DPASes that don't increase regpressure
        // - we can also consider the ones that do increase

        auto getLoadWidth = [&](Instruction *I) -> uint {
          if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I)) {
            if (Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead ||
                Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload) {
              auto VectorType = dyn_cast<IGCLLVM::FixedVectorType>(Intr->getType());
              if (VectorType) {
                return VectorType->getNumElements();
              }
            }
          }
          return 0;
        };

        InstNodePtrList LoadsThatUnlockDPASes;
        InstNodePtrList LoadsThatUnlockDPASesNoRPIncreasing;

        for (InstructionNode *Node : Nodes) {
          if (!is2dBlockRead(Node->I) || getLoadWidth(Node->I) > MaxLoadSize) {
            continue;
          }
          for (auto *U : getRealUsesThroughVS(Node->I)) {
            auto *I = dyn_cast<Instruction>(U);
            if (!I) {
              continue;
            }

            if (isDPAS(I)) {

              bool OneOpIsDPAS = false;
              bool FirstOpIsZero = false;

              auto *FirstOp = dyn_cast<Constant>(I->getOperand(0));
              if (FirstOp && (isa<UndefValue>(FirstOp) || FirstOp->isNullValue())) {
                FirstOpIsZero = true;
              }

              int NumOps = static_cast<int>(I->getNumOperands());
              for (auto &Op : I->operands()) {
                Instruction *OpI = dyn_cast<Instruction>(Op.get());
                if (!OpI) {
                  NumOps--;
                  continue;
                }
                if (RT.inBBCurrent(OpI)) {
                  NumOps--;
                  if (OpI && isDPAS(OpI)) {
                    OneOpIsDPAS = true;
                  }
                } else if (getRealOpThroughVS(OpI) == Node->I) {
                  NumOps--;
                }
              }
              if (NumOps == 0) {
                LoadsThatUnlockDPASes.push_back(Node);
                if (!FirstOpIsZero) {
                  LoadsThatUnlockDPASesNoRPIncreasing.push_back(Node);
                }
                break;
              }
            }
          }
        }

        if (LoadsThatUnlockDPASesNoRPIncreasing.size() > 0) {
          Nodes = std::move(LoadsThatUnlockDPASesNoRPIncreasing);
        } else if (LoadsThatUnlockDPASes.size() > 0) {
          Nodes = std::move(LoadsThatUnlockDPASes);
        }
        return Nodes;
      };

      auto getDPASIfExist = [&](InstNodePtrList &Nodes, bool ForceDPAS = false) -> InstNodePtrList & {
        InstNodePtrList DPASNodes;
        for (InstructionNode *Node : Nodes) {
          if (isDPAS(Node->I)) {
            DPASNodes.push_back(Node);
          }
        }
        if (DPASNodes.size() > 0 || ForceDPAS) { // is ForceDPAS we can also return empty list
          Nodes = std::move(DPASNodes);
        }
        return Nodes;
      };

      auto isLargeLoad = [&](Instruction *I) -> bool {
        if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I)) {
          if (Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead ||
              Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload) {
            auto VectorType = dyn_cast<IGCLLVM::FixedVectorType>(Intr->getType());
            if (VectorType) {
              return static_cast<int>(VectorType->getNumElements()) >= static_cast<int>(C[Option::LargeBlockLoadSize]);
            }
          }
        }
        return false;
      };

      auto filterOutNotReadyRematInstructions = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        InstNodePtrList NonFilteredNodes;
        for (InstructionNode *Node : Nodes) {
          auto *RCP = RCA->getRematChainPattern(Node->I);
          if (!RCP || (RCP->getLastInst() == Node->I)) {
            NonFilteredNodes.push_back(Node);
          } else {
            // if the target instruction is not ready, we need to filter out the first remated instruction
            bool IsReady = true;
            Instruction *TargetInst = RCP->getRematTargetInst();
            InstructionNode *TargetNode = G.InstToNode[TargetInst];
            for (const auto &PN : TargetNode->Preds) {
              IGC_ASSERT(!PN->Deleted);
              if (PN->Src->I == RCP->getLastInst()) {
                continue;
              }
              IsReady = false;
              break;
            }
            if (IsReady) {
              NonFilteredNodes.push_back(Node);
            } else {
              PrintDumpLevel(VerbosityLevel::High, "Filtering out not ready remat instruction: ");
              PrintInstructionDumpLevel(VerbosityLevel::High, Node->I);
            }
          }
        }
        if (NonFilteredNodes.size() > 0) {
          Nodes = std::move(NonFilteredNodes);
        }
        return Nodes;
      };

      auto filterOutNotReadyIcmp = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        // Heuristic in order not to put ICMP that is used by a select too early.
        // Schedule it only when the select is ready

        InstNodePtrList NonFilteredNodes;
        for (InstructionNode *Node : Nodes) {
          if (isa<ICmpInst>(Node->I)) {
            bool IsReady = true;
            User *U = IGCLLVM::getUniqueUndroppableUser(Node->I);
            if (!U) {
              NonFilteredNodes.push_back(Node);
              continue;
            }
            SelectInst *SI = dyn_cast<SelectInst>(U);
            if (!SI) {
              NonFilteredNodes.push_back(Node);
              continue;
            }
            // If the select instruction is not ready, we need to filter out the icmp instruction
            InstructionNode *SelectNode = G.InstToNode[SI];
            for (const auto &PN : SelectNode->Preds) {
              if (PN->Src->I == Node->I) {
                continue;
              }
              if (isa<Constant>(PN->Src->I) || isa<PHINode>(PN->Src->I)) {
                continue;
              }
              Instruction *OpI = dyn_cast<Instruction>(PN->Src->I);
              if (!OpI) {
                continue;
              }

              if (!RT.inBBCurrent(OpI)) {
                // if the instruction is in BBCurrent, then it is ready
                IsReady = false;
                break;
              }
            }
            if (IsReady) {
              NonFilteredNodes.push_back(Node);
            }
            // else it's filtered out, until the operand of the select is ready
          }
          else {
            NonFilteredNodes.push_back(Node);
          }
        }
        if (NonFilteredNodes.size() > 0) {
          Nodes = std::move(NonFilteredNodes);
        }
        return Nodes;
      };

      auto focusLoadsOnOneDPAS = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        // If all Nodes are 2d block loads, choose the dpas user with the lowest initial number and filter out
        // all the remaining loads. This is needed to avoid a situation when we schedule a lot of small loads first,
        // but all the DPASes wait for some load that is in the end
        if (Nodes.size() == 1) {
          return Nodes;
        }

        InstNodePtrList NonFilteredNodes;
        if (std::all_of(Nodes.begin(), Nodes.end(),
                        [&](InstructionNode *Node) { return is2dBlockRead(Node->I); })) {

          // Get the first DPAS user
          InstructionNode *FirstDPASUser = nullptr;
          for (InstructionNode *Node : Nodes) {
            for (auto *U : getRealUsesThroughVS(Node->I)) {
              auto *I = dyn_cast<Instruction>(U);
              if (!I) {
                continue;
              }

              if (isDPAS(I)) {
                if (!FirstDPASUser || (G.InstToNode[I]->OriginalPosition < FirstDPASUser->OriginalPosition)) {
                  FirstDPASUser = G.InstToNode[I];

                  NonFilteredNodes = {Node};
                } else if (G.InstToNode[I] == FirstDPASUser) {
                  NonFilteredNodes.push_back(Node);
                }
              }
            }
          }

          if (NonFilteredNodes.size() > 0) {
            Nodes = std::move(NonFilteredNodes);
          }
        }

        return Nodes;
      };

      auto filterOutNotUnblockingExistingVectorInst = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        // If some values are currently hanging because of creating a vector instruction out of scalars
        // we prioritize the candidates that unblock the other elements of the vector

        // This helps to resolve the issue when we schedule several IEs to the 0th element of different vectors
        // increasing the regpressure, because the GRF space for the other elements is immediately reserved
        // but the vectors are not fully populated and we can't use them

        DenseSet<Instruction *> HangingElements = RT.getHangingS2VInstructions();
        if (HangingElements.empty()) {
          // If there are no hanging elements, we don't need to filter out anything
          return Nodes;
        }

        InstNodePtrList NonFilteredNodes;
        for (InstructionNode *Node : Nodes) {
          if (HangingElements.count(Node->I) > 0) {
            // If the instruction is already hanging, we don't need to filter it out
            NonFilteredNodes.push_back(Node);
            continue;
          }
          for (Value *V : getRealUsesThroughRematChains(Node->I)) {
            if (Instruction *I = dyn_cast<Instruction>(V)) {
              if (HangingElements.count(I) > 0) {
                NonFilteredNodes.push_back(Node);
                break; // No need to check other uses, we already found a use that unblocks the vector
              }
            }
          }
        }
        if (NonFilteredNodes.size() > 0) {
          Nodes = std::move(NonFilteredNodes);
        }
        return Nodes;
      };

      auto getMaxNumWaveAll = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
        // Experimental heuristic: Add only maxnum (llvm.maxnum) and waveall instructions to the list
        // The idea is that maxnum->waveall(max) is a common pattern
        // that usually leads to decreasing the register pressure
        // because all the lanes converge to the same value

        InstNodePtrList NonFilteredNodes;
        for (InstructionNode *Node : Nodes) {
          if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(Node->I)) {
            if (Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveAll) {
              NonFilteredNodes.push_back(Node);
            }
          }
          else if (IntrinsicInst *Intr = llvm::dyn_cast<IntrinsicInst>(Node->I)) {
            if (Intr->getIntrinsicID() == Intrinsic::maxnum) {
              NonFilteredNodes.push_back(Node);
            }
          }
        }
        if (NonFilteredNodes.size() > 0) {
          Nodes = std::move(NonFilteredNodes);
        }
        return Nodes;
      };

      // ===                                                          ===
      // === Choosing if we have instructions to schedule immediately ===
      // ===                                                          ===

      if (!ImmediateReadyList.empty()) {
        InstructionNode *Node = getFirstNode(ImmediateReadyList);

        auto *DT = VSA->getDestVector(Node->I);
        std::string VS_String = "   ";

        // PrioritizeDPASOverImmediateVS heuristic: if we have an immediate ready instruction that is a DPAS,
        // prioritize it over the immediate ready vector shuffle
        // The idea is to put the DPAS in between the load and the load shuffle to hide latency
        // because the vector shuffle forces waiting for the load to finish
        if (C[Option::PrioritizeDPASAndOtherOverImmediateVS]) {
          auto isAllowedInstruction = [&](Instruction *I) {
            if (isa<BinaryOperator>(I)) {
              return true;
            }
            if (isNoOpInst(I, CTX)) {
              return true;
            }
            GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I);
            if (!Intr) {
              return false;
            }
            switch (Intr->getIntrinsicID()) {
            case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch:
            case GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload:
            case GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField:
              return true;
            default:
              return isDPAS(I);
            }
          };
          auto getAllowedInstructions = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
            InstNodePtrList AllowedNodes;
            for (InstructionNode *Node : Nodes) {
              if (isAllowedInstruction(Node->I)) {
                AllowedNodes.push_back(Node);
              }
            }
            Nodes = std::move(AllowedNodes);
            return Nodes;
          };

          if (DT && DT->isVectorShuffle() && !DT->isNoOp() && !ReadyList.empty() && !ScheduledList.empty() &&
              (is2dBlockRead(ScheduledList.back()->I) || isAllowedInstruction(ScheduledList.back()->I))) {
            InstructionNode *OriginalImmediateNode = Node;

            // Try to put a DPAS in between the load and the load shuffle
            InstNodePtrList TempReadyList = ReadyList;
            TempReadyList = getAllowedInstructions(TempReadyList);
            if (!TempReadyList.empty()) {
              TempReadyList = getLowestRegpressureNodes(TempReadyList);
              TempReadyList = getMaxWeightNodes(TempReadyList, RT.isRegpressureHigh() || GreedyRP);
              Node = getFirstNode(TempReadyList);
              if (RT.estimate(Node->I) > C[Option::PrioritizeOverImmediateVSMaxRPInBytes]) {
                Node = OriginalImmediateNode;
              }
              if (Node != OriginalImmediateNode) {
                DT = nullptr;
                VS_String = "DPH"; // DPAS heuristic
              }
            }
          }
        }

        std::string Info = formatDebugInfo(
          RT.getCurrentPressure(), RT.estimate(Node->I), "Im", getVectorShuffleString(Node->I, VSA, RCA));

        PrintDump(Info);
        Node->print(*LogStream);

        return std::make_tuple(Node, false);
      } else {
        // If we have no immediate ready instructions, choose the one from the ready list

        InstructionNode *Node = nullptr;

        IGC_ASSERT(ReadyList.size() > 0);

        PrintDumpLevel(VerbosityLevel::Medium, "Choosing from the ready list:\n");
        for (InstructionNode *N : ReadyList) {
          PrintInstructionDumpLevel(VerbosityLevel::Medium, N->I);
        }

        // Filter ReadyList so that only if the instruction is Handicapped
        // It will remain only if the current regpressure is lower that the Handicapped value
        InstNodePtrList FilteredReadyList;
        for (InstructionNode *Node : ReadyList) {
          IGC_ASSERT(Node->I);
          if (Handicapped.count(Node->I) == 0 || RT.getCurrentPressure() < Handicapped[Node->I]) {
            FilteredReadyList.push_back(Node);
          }
        }

        bool CanClone = true;

        // If the filtered list is empty, use the original list
        if (FilteredReadyList.empty()) {
          FilteredReadyList = ReadyList;
          CanClone = false;
        }

        FilteredReadyList = filterOutNotReadyRematInstructions(FilteredReadyList);
        FilteredReadyList = filterOutNotReadyIcmp(FilteredReadyList);

        IGC_ASSERT(FilteredReadyList.size() > 0);

        bool ChooseByRP = RT.isRegpressureHigh() || GreedyRP;

        InstNodePtrList OrigFilteredReadyList = FilteredReadyList;
        if (!ChooseByRP) {
          // ===                                       ===
          // === Choosing when the regpressure is OK   ===
          // ===                                       ===

          // Choose the Node with the highest MaxWeight, if several, choose the one with the lowest
          // regpressure, if several, choose the one with the least OriginalPosition
          FilteredReadyList = getMaxWeightNodes(FilteredReadyList);
          FilteredReadyList = getLowestRegpressureNodes(FilteredReadyList);
          if (C[Option::FocusLoadsOnOneDPAS]) {
            FilteredReadyList = focusLoadsOnOneDPAS(FilteredReadyList);
          }
          Node = getFirstNode(FilteredReadyList);
          bool IsRegpressureCritical = RT.isRegpressureCritical(Node->I);
          CanClone = RT.isRegpressureHigh(Node->I) || isLargeLoad(Node->I);
          ChooseByRP = IsRegpressureCritical;
          FilteredReadyList = OrigFilteredReadyList;
        }

        if (ChooseByRP) {
          // ===                                       ===
          // === Choosing when the regpressure is HIGH ===
          // ===                                       ===

          // Choose the Node with the lowest regpressure estimate, if several, choose the one with the highest
          // MaxWeight, if several, choose the one with the least OriginalPosition
          if (GreedyRP && !RT.isRegpressureHigh() && (C[Option::PrioritizeLargeBlockLoadsInRP] > 0)) {
            // Experimental heuristic: prioritize large block loads
            FilteredReadyList = getLargeBlockLoadsIfExist(FilteredReadyList);
          }

          if (C[Option::PrioritizeMaxnumWaveallHighRP]) {
            FilteredReadyList = getMaxNumWaveAll(FilteredReadyList);
          }
          if (C[Option::PrioritizeDPASHighRP]) {
            // Experimental heuristic: prioritize DPAS and the instructions that make it possible to
            // schedule DPAS earlier
            FilteredReadyList = getDPASIfExist(FilteredReadyList, false);
          }
          if (C[Option::PrioritizeLoadsThatUnlockDPASesHighRP]) {
            // Experimental heuristic: prioritize loads that unlock
            // DPASes
            FilteredReadyList = getLoadsThatUnlockDPASes(FilteredReadyList,
                                                         C[Option::PrioritizeLoadsThatUnlockDPASesHighRP_MaxLoadSize]);
          }
          if (C[Option::PrioritizePopulatingOneVectorHighRP]) {
            FilteredReadyList = filterOutNotUnblockingExistingVectorInst(FilteredReadyList);
          }

          FilteredReadyList = getLowestRegpressureNodes(FilteredReadyList);

          if (C[Option::FocusLoadsOnOneDPAS]) {
            FilteredReadyList = focusLoadsOnOneDPAS(FilteredReadyList);
          }

          // If we have several nodes with the same regpressure, choose the one with the highest MaxWeight
          FilteredReadyList = getMaxWeightNodes(FilteredReadyList, C[Option::UseHighRPWeight] == 1);

          Node = getFirstNode(FilteredReadyList);

          // Don't clone if we are choosing by RP
          CanClone = false;
        }

#ifdef _DEBUG
        IGC_ASSERT(std::find(ReadyList.begin(), ReadyList.end(), Node) != ReadyList.end());
#endif
        IGC_ASSERT(Node != nullptr);

        if (!ChooseByRP) {
          AllInstructionsScheduledByRP = false;
        }

        std::string ChoosingMode = ChooseByRP ? "RP" : "MW";
        ChoosingMode += CanClone ? "*" : "";
        std::string Info = formatDebugInfo(RT.getCurrentPressure(), RT.estimate(Node->I),
                           ChoosingMode,
                           getVectorShuffleString(Node->I, VSA, RCA));
        PrintDump(Info);
        Node->print(*LogStream);

        return std::make_tuple(Node, CanClone);
      }
    }

    bool needToClone(InstructionNode *Node, bool checkMinInterval = true) {
      if (!is2dBlockRead(Node->I)) {
        return false;
      }
      auto Uses = RT.getRealUses(Node->I);
      if (Uses.size() == 0) {
        return false;
      }

      if (checkMinInterval) {
        return RefLiveIntervals[Node->I] > C[Option::MinLiveIntervalForCloning];
      }

      return true;
    }
  };
};

bool CodeScheduling::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (CTX->type != ShaderType::OPENCL_SHADER)
    return false;

  if (IGC_IS_FLAG_ENABLED(DisableCodeScheduling))
    return false;

  SchedulingConfig Config;

  if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling)) {
    auto printGlobalSettings = [](llvm::raw_ostream &LogStream) {
      LogStream << "CodeSchedulingForceMWOnly: " << IGC_GET_FLAG_VALUE(CodeSchedulingForceMWOnly) << "\n";
      LogStream << "CodeSchedulingForceRPOnly: " << IGC_GET_FLAG_VALUE(CodeSchedulingForceRPOnly) << "\n";
      LogStream << "CodeSchedulingAttemptsLimit: " << IGC_GET_FLAG_VALUE(CodeSchedulingAttemptsLimit) << "\n";
      LogStream << "CodeSchedulingRPMargin: " << IGC_GET_FLAG_VALUE(CodeSchedulingRPMargin) << "\n";
      LogStream << "CodeSchedulingRenameAll: " << IGC_GET_FLAG_VALUE(CodeSchedulingRenameAll) << "\n";
      LogStream << "CodeSchedulingDumpLevel: " << IGC_GET_FLAG_VALUE(CodeSchedulingDumpLevel) << "\n";
      LogStream << "EnableCodeSchedulingIfNoSpills: " << IGC_GET_FLAG_VALUE(EnableCodeSchedulingIfNoSpills) << "\n";
      LogStream << "-----\n";
    };

    Log.clear();

    printGlobalSettings(*LogStream);
    Config.printOptions(LogStream);

    PrintDump("=====================================\n");
    PrintDump("Function " << F.getName() << "\n");
  }

  // Might be needed soon for heuristics

  // DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  // LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  // AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
  AA = nullptr; // using alias information is not supported yet
  VSA = &getAnalysis<VectorShuffleAnalysis>();
  RCA = &getAnalysis<RematChainsAnalysis>();
  RPE = &getAnalysis<IGCLivenessAnalysis>();
  FRPE = &getAnalysis<IGCFunctionExternalRegPressureAnalysis>();
  WI = &FRPE->getWIAnalysis(&F);

  bool Changed = false;

  for (auto &BB : F) {
    if (!std::any_of(BB.begin(), BB.end(), [](Instruction &I) { return isDPAS(&I); }))
      continue;

    BBScheduler Scheduler(&BB, RPE, FRPE, AA, VSA, RCA, CTX, &Config, LogStream);
    Changed |= Scheduler.schedule();
  }

  if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling) && IGC_IS_FLAG_DISABLED(PrintToConsole))
    dumpToFile(Log);

  IGC_ASSERT(false == verifyFunction(F, &dbgs()));

  return Changed;
}

void CodeScheduling::dumpToFile(const std::string &Log) {
  auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                  .Hash(CTX->hash)
                  .Type(CTX->type)
                  .Retry(CTX->m_retryManager.GetRetryId())
                  .Pass("scheduling")
                  .Extension("txt");
  IGC::Debug::DumpLock();
  std::ofstream OutputFile(Name.str(), std::ios_base::app);
  if (OutputFile.is_open()) {
    OutputFile << Log;
  }
  OutputFile.close();
  IGC::Debug::DumpUnlock();
}

} // namespace IGC

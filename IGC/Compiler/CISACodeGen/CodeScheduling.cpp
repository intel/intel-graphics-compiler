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
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Verifier.h"

// #include "llvm/ADT/PostOrderIterator.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/CodeScheduling.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

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
    if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling) && (Level <= IGC_GET_FLAG_VALUE(CodeSchedulingDumpLevel))) {           \
        *LogStream << Contents;                                                                                        \
    }
#define PrintInstructionDumpLevel(Level, Inst)                                                                         \
    if (IGC_IS_FLAG_ENABLED(DumpCodeScheduling) && (Level <= IGC_GET_FLAG_VALUE(CodeSchedulingDumpLevel))) {           \
        (Inst)->print(*LogStream, false);                                                                              \
        *LogStream << "\n";                                                                                            \
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
    OptionValues.push_back(defaultValue);                                                                              \
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
        VectorShuffleAnalysis *VSA, WIAnalysisRunner *WI, CodeGenContext *CTX, SchedulingConfig *Config,
        llvm::raw_ostream *LogStream)
        : BB(BB), RPE(RPE), FRPE(FRPE), VSA(VSA), WI(WI), CTX(CTX), C(Config), LogStream(LogStream) {
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
        PressureMap = RPT.PressureMap;

        // deepcopy HangingLiveVarsVec and HangingLiveVars
        HangingLiveVarsVec.clear();
        HangingLiveVarsVec.reserve(RPT.HangingLiveVarsVec.size());
        for (const auto &HangingLiveVar : RPT.HangingLiveVarsVec) {
            HangingLiveVarsVec.push_back(
                std::make_unique<HangingLiveVarsInfo>(HangingLiveVar->Size, HangingLiveVar->Type));
            HangingLiveVarsVec.back()->LiveVars = HangingLiveVar->LiveVars;
            for (auto *V : HangingLiveVar->LiveVars) {
                HangingLiveVars[V] = HangingLiveVarsVec.back().get();
            }
        }
    }

    RegisterPressureTracker& operator=(const RegisterPressureTracker&) = delete;
    RegisterPressureTracker() = delete;
    ~RegisterPressureTracker() = default;

    // TODO reuse instead of copy from IGCLivenessAnalysis.cpp
    unsigned int computeSizeInBytes(Value *V, unsigned int SIMD, WIAnalysisRunner *WI, const DataLayout &DL) {
        // when we check size of operands, this check is redundant
        // but allows for a nicer code
        bool NoRetVal = V->getType()->isVoidTy();
        if (NoRetVal)
            return 0;

        auto Type = V->getType();
        unsigned int TypeSizeInBits = (unsigned int)DL.getTypeSizeInBits(Type);
        unsigned int Multiplier = SIMD;
        if (WI && WI->isUniform(V))
            Multiplier = 1;
        unsigned int SizeInBytes = TypeSizeInBits * Multiplier / 8;
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
            BBCurrent.insert(I);
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
        const int RegisterSize = RPE->registerSizeInBytes();
        CurrentPressure = RPE->estimateSizeInBytes(BBCurrent, *F, SIMD, WI) + ReservedRegisters * RegisterSize;
        PrintDump("Initial CurrentPressure: " << CurrentPressure << "\n");
        int32_t CurrentPressureInRegisters = int32_t(RPE->bytesToRegisters(CurrentPressure));
        PrintDump("Initial CurrentPressure in registers: " << CurrentPressureInRegisters << "\n\n");

        PressureMap.clear();
        for (auto *V : BBCurrent) {
            PressureMap[V] = CurrentPressure;
        }
    }

    bool isRegpressureLow(Instruction *I = nullptr) {
        return compareRPWithThreshold<false>(C->get(SchedulingConfig::Option::LowRPThresholdDelta), I);
    }

    bool isRegpressureHigh(Instruction *I = nullptr) {
        return compareRPWithThreshold<true>(
            C->get(SchedulingConfig::Option::GreedyRPThresholdDelta) + IGC_GET_FLAG_VALUE(CodeSchedulingRPMargin), I);
    }

    bool isRegpressureCritical(Instruction *I = nullptr) {
        return compareRPWithThreshold<true>(IGC_GET_FLAG_VALUE(CodeSchedulingRPMargin), I);
    }

    int32_t getCurrentPressure(Instruction *I = nullptr) {
        auto CurrentPressureAdjusted = CurrentPressure;
        if (I != nullptr)
            CurrentPressureAdjusted += estimate(I);
        auto ExternalPressure = int32_t(FRPE->getExternalPressureForFunction(F));
        auto CurrentPressureInRegisters = int32_t(RPE->bytesToRegisters(CurrentPressureAdjusted)) + ExternalPressure;
        return CurrentPressureInRegisters;
    }

    int32_t estimate(Instruction *I) { return estimateOrUpdate(I, false); }

    int32_t update(Instruction *I) { return estimateOrUpdate(I, true); }

    int32_t getMaxPressure(Value *V) { return RPE->bytesToRegisters(PressureMap[V]); }

    llvm::SmallSet<Value *, 8> getRealUses(Value *I) {
        llvm::SmallSet<Value *, 8> Uses;
        for (auto *U : I->users()) {
            if (Instruction *UI = dyn_cast<Instruction>(U)) {
                if (isDbgIntrinsic(UI))
                    continue;

                if (isNoOpInst(UI, CTX)) {
                    for (auto *UU : getRealUses(UI)) {
                        Uses.insert(UU);
                    }
                } else {
                    Uses.insert(UI);
                }
            }
        }
        return std::move(Uses);
    }

    bool inBBCurrent(Value *V) { return BBCurrent.count(V); }

    Value *getRealOp(Value *V) {
        if (BBIn.count(V))
            return V;

        Instruction *I = dyn_cast<Instruction>(V);
        if (!I)
            return V;

        if (isNoOpInst(I, CTX)) {
            return getRealOp(I->getOperand(0));
        }
        return V;
    }

  private:
    BasicBlock *BB;
    Function *F;
    IGCLivenessAnalysis *RPE;
    IGCFunctionExternalRegPressureAnalysis *FRPE;
    VectorShuffleAnalysis *VSA;
    WIAnalysisRunner *WI;
    CodeGenContext *CTX;
    const DataLayout *DL;
    SchedulingConfig *C;
    llvm::raw_ostream *LogStream;

    int32_t SIMD;
    int32_t CurrentPressure = 0;

    ValueSet BBIn;
    ValueSet BBOut;
    ValueSet BBCurrent;

    llvm::DenseMap<Value *, int32_t> PressureMap;

    typedef enum { HANGING_SCALARS, HANGING_VECTORS, HANGING_NOOP_VECTORS } HangingLiveVarsType;

    // POD structure to keep information about hanging values
    struct HangingLiveVarsInfo {
        ValueSet LiveVars;
        uint32_t Size;
        HangingLiveVarsType Type;

        HangingLiveVarsInfo(uint32_t SizeInBytes, HangingLiveVarsType Type)
            : LiveVars(), Size(SizeInBytes), Type(Type){};
    };
    std::vector<std::unique_ptr<HangingLiveVarsInfo>> HangingLiveVarsVec;
    DenseMap<Value *, HangingLiveVarsInfo *> HangingLiveVars;

    template <bool checkIfHigher> bool compareRPWithThreshold(int Threshold, Instruction *I = nullptr) {
        int NGRF = (int)CTX->getNumGRFPerThread(false);
        if (NGRF == 0) { // GRF info is not set, using the default value
            if (CTX->isAutoGRFSelectionEnabled()) {
                NGRF = C->get(SchedulingConfig::Option::DefaultNumGRFAuto);
            } else {
                NGRF = C->get(SchedulingConfig::Option::DefaultNumGRF);
            }
        }
        if constexpr (checkIfHigher) {
            return getCurrentPressure(I) > NGRF - Threshold;
        } else {
            return getCurrentPressure(I) <= NGRF - Threshold;
        }
    }

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
        if (IGCLLVM::isDebugOrPseudoInst(*I) || I->isLifetimeStartOrEnd() || isNoOpInst(I, CTX)) {
            // NoOp instructions do not change register pressure
            if (Update)
                PrintDumpLevel(VerbosityLevel::High, "NoOp instruction: " << getName(I) << "\n");
            return 0;
        }

        if (Update)
            PrintDumpLevel(VerbosityLevel::High, getName(I));

        int32_t ResultSizeInBytes = 0;

        // First check how does the instruction increase the register pressure
        // It takes the register for the output value...
        int RPIncrease = computeSizeInBytes(I, SIMD, WI, *DL);

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
                    HangingLiveVarsVec.emplace_back(std::make_unique<HangingLiveVarsInfo>(0, HANGING_SCALARS));
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
                        PrintDumpLevel(VerbosityLevel::High,
                            " (populating HLV with " << HLV->LiveVars.size() << (CurrentScalarDies ? " remaining" : "")
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
                        computeSizeInBytes(V2SP->getSourceVec(), SIMD, WI, *DL), HANGING_SCALARS));
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
                        PrintDumpLevel(
                            VerbosityLevel::High, " (adding vector " << getName(V2SP->getSourceVec()) << " to HLV)");
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
                        if (HLV->Type == HANGING_SCALARS) {
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
                        if (HLV->Type == HANGING_SCALARS) {
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

            for (auto *V : BBCurrent) {
                PressureMap[V] = std::max(PressureMap[V], CurrentPressure);
            }

            // Print log dump only on Update in order not to output duplicating information
            PrintDumpLevel(VerbosityLevel::High, "\n\n");
        }

        return ResultSizeInBytes;
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
        VectorShuffleAnalysis *VSA, CodeGenContext *CTX, SchedulingConfig *Config, llvm::raw_ostream *LogStream)
        : BB(BB), RPE(RPE), FRPE(FRPE), AA(AA), VSA(VSA), CTX(CTX), C(*Config), LogStream(LogStream) {
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

        RegisterPressureTracker RPT(BB, RPE, FRPE, VSA, WI, CTX, &C, LogStream);

        bool OriginalScheduleCanHaveSpills = false;
        for (auto &I : *BB) {
            RPT.update(&I);
            if (RPT.isRegpressureCritical()) {
                OriginalScheduleCanHaveSpills = true;
                PrintDump("Original schedule achieved the critical regpressure: " << RPT.getCurrentPressure() << "\n");
                break;
            }
        }

        if (!OriginalScheduleCanHaveSpills && !IGC_IS_FLAG_ENABLED(EnableCodeSchedulingIfNoSpills)) {
            PrintDump("Original schedule can not have spills, skipping scheduling\n");
            PrintDump("Schedule is not changed" << "\n");
            return false;
        }

        // Create a schedules stack and an initial empty schedule. It'll create a DepGraph.
        // Schedule is a copyable object, so we can make a copy to save a "checkpoint".

        std::vector<std::unique_ptr<Schedule>> Schedules;
        Schedules.push_back(std::make_unique<Schedule>(BB, RPE, FRPE, VSA, WI, CTX, &C, LogStream));

        // First try if "GreedyMW" scheduling can be applied
        // This approach prioritizes scheduling by the edge weights
        // To maximize hiding the instructions latency.

        // We'll commit it if it has no spills

        std::unique_ptr<Schedule> GreedyMWSchedule = std::make_unique<Schedule>(*Schedules.front());
        GreedyMWSchedule->setGreedyMW(true);

        if (!IGC_IS_FLAG_ENABLED(CodeSchedulingForceRPOnly)) {
            PrintDump("Greedy MW attempt\n");
            while (!GreedyMWSchedule->isComplete()) {
                GreedyMWSchedule->scheduleNextInstruction();
            }

            if (IGC_IS_FLAG_ENABLED(CodeSchedulingForceMWOnly) || !GreedyMWSchedule->canEverHaveSpills()) {
                PrintDump("Greedy MW schedule is forced or has no spills.\n");
                GreedyMWSchedule->commit();
                return true;
            }
        }

        // Then try to apply "GreedyRP" scheduling
        // Schedule only for the pressure minimization
        // If it still has spills or is forced, we will commit it

        std::unique_ptr<Schedule> GreedyRPSchedule = std::make_unique<Schedule>(*Schedules.front());
        GreedyRPSchedule->setGreedyRP(true);
        PrintDump("Greedy RP attempt\n");

        // PrintDump("DepGraph dump\n");
        // DepGraph G(BB, RPE, FRPE, VSA, WI, CTX, C, LogStream);
        // G.print(*LogStream);

        while (!GreedyRPSchedule->isComplete()) {
            GreedyRPSchedule->scheduleNextInstruction();
        }

        bool canCompileWithNoSpills = !GreedyRPSchedule->canEverHaveSpills();

        if (IGC_IS_FLAG_ENABLED(CodeSchedulingForceRPOnly) ||
            ((IGC_GET_FLAG_VALUE(CodeSchedulingAttemptsLimit) <= 0 || !canCompileWithNoSpills) &&
                OriginalScheduleCanHaveSpills)) {
            PrintDump("Greedy RP schedule can have spills or is forced, commiting it and stopping.\n");
            GreedyRPSchedule->commit();
            return true;
        }

        IGC_ASSERT(Schedules.size() == 1);

        // Try several attempts with backtracking to find the best schedule with no spills
        Schedules.front()->setRefLiveIntervals(GreedyMWSchedule->getMaxLiveIntervals());

        uint attempt = 1;
        do {
            Schedule *S = Schedules.back().get();
            PrintDump("Attempt #" << attempt << "\n");

            std::vector<std::unique_ptr<Schedule>> NewSchedules;

            while (!S->isComplete()) {
                // Schedule the next instruction and add the checkpoint if it returns the previous state
                std::unique_ptr<Schedule> Checkpoint = S->scheduleNextInstruction();
                if (Checkpoint) {
                    NewSchedules.push_back(std::move(Checkpoint));
                }
                if (canCompileWithNoSpills && S->canEverHaveSpills()) {
                    break;
                }
            }

            bool Success = S->isComplete();
            if (Success) {
                PrintDump("Schedule is complete\n");
                S->commit();
                Changed = true;
                break;
            } else {
                PrintDump("Schedule of attempt #" << attempt << " is not complete\n");
                PrintDump("Can ever have spills? " << S->canEverHaveSpills() << "\n");
                PrintDump("Can compile with no spills? " << canCompileWithNoSpills << "\n");
                Schedules.pop_back();

                // push NewSchedules to Schedules in the reverse order
                for (auto It = NewSchedules.rbegin(); It != NewSchedules.rend(); ++It) {
                    Schedules.push_back(std::move(*It));
                }

                PrintDump("Schedules left in the queue: " << Schedules.size() << "\n");
            }
            if (attempt > int(IGC_GET_FLAG_VALUE(CodeSchedulingAttemptsLimit))) {
                PrintDump("Attempts limit reached\n");
                break;
            }
            attempt++;
        } while (!Schedules.empty());

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
    SchedulingConfig &C;
    llvm::raw_ostream *LogStream;

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
                std::string Info =
                    "Node #" + std::to_string(OriginalPosition) + ", MW: " + std::to_string(MaxWeight) + " ";
                Info.resize(23, ' ');
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
            : Src(Src), Dst(Dst), Weight(Weight), WeightHighRP(Weight), ForceSubsequent(ForceSubsequent),
              Deleted(false) {}

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
            VectorShuffleAnalysis *VSA, WIAnalysisRunner *WI, CodeGenContext *CTX, SchedulingConfig &C,
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
                    DepEdges.emplace_back(std::make_unique<DepEdge>(
                        InstToNode[Src], InstToNode[Dst], Weight, WeightHighRP, ForceSubsequent));
                    InstToNode[Src]->Succs.insert(DepEdges.back().get());
                    InstToNode[Dst]->Preds.insert(DepEdges.back().get());
                }
            };

            std::vector<Instruction *> UnknownStores;
            std::vector<Instruction *> AllMemoryAccesses;

            // Structures to track non-ssa dependencies of the decomposed loads
            DenseMap<Instruction *, llvm::SmallVector<Instruction *, 32>> Prev2DBlockReadPayloads;
            DenseMap<Instruction *, DenseMap<uint32_t, Instruction *>> Last2DBlockSetAddrPayloadField;

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
                    case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
                        return HighRP ? C[Option::Weight2dBlockReadDstDepHighRP] : C[Option::Weight2dBlockReadDstDep];

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
                                Weight = SourceVecInstruction == nullptr
                                             ? 0
                                             : getSSAEdgeWeight(SourceVecInstruction, Dst, false);
                                WeightHighRP = SourceVecInstruction == nullptr
                                                   ? 0
                                                   : getSSAEdgeWeight(SourceVecInstruction, Dst, true);
                            } else {
                                // Use the default weight for the vector shuffle
                                Weight = C[Option::WeightUnknownVectorShuffleDstDep];
                                WeightHighRP = C[Option::WeightUnknownVectorShuffleDstDep];
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
                            V2SP) {
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
                            addEdge(PrevBlockRead, &I, C[Option::Weight2dBlockReadSrcDep],
                                C[Option::Weight2dBlockReadSrcDep]);
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

                    default:
                        break;
                    }
                }

                if (isDPAS(&I)) {
                    isUnknownStore = false;
                }

                if (isUnknownStore || isPrefetch) {
                    PrintDumpLevel(VerbosityLevel::High, "Unknown store:\n");
                    PrintInstructionDumpLevel(VerbosityLevel::High, &I);

                    UnknownStores.push_back(&I);

                    // Every unknown store depends on all the memory accesses
                    // We also assume the same for the prefetch in order to preserve its place
                    for (auto &MemAccess : AllMemoryAccesses) {
                        addEdge(MemAccess, &I, 0, 0);
                    }
                }

                Instruction *Terminator = BB->getTerminator();

                // Terminator "depends" on all the instruction - they need to be placed before
                // TODO consider if we need to add a weight to this edge
                if ((&I != Terminator) && (!isPrefetch)) {
                    addEdge(&I, Terminator, 0, 0);
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
            VectorShuffleAnalysis *VSA, WIAnalysisRunner *WI, CodeGenContext *CTX, SchedulingConfig *C,
            llvm::raw_ostream *LogStream)
            : BB(BB), C(*C), CTX(CTX), VSA(VSA), LogStream(LogStream),
              G(DepGraph(BB, RPE, FRPE, VSA, WI, CTX, *C, LogStream)),
              RT(RegisterPressureTracker(BB, RPE, FRPE, VSA, WI, CTX, C, LogStream)) {
            // init ready list
            for (auto &Node : G.InstNodes) {
                if (Node.Preds.empty()) {
                    ReadyList.push_back(&Node);
                }
            }

            IGC_ASSERT(this->VSA->getDestVector(BB->getTerminator()) == nullptr);
        }

        Schedule& operator=(const Schedule&) = delete;
        ~Schedule() = default;

        // Copy constructor for Schedule
        Schedule(const Schedule &S)
            : LogStream(S.LogStream), RT(S.RT), // RT is copyable
              BB(S.BB), C(S.C), CTX(S.CTX), VSA(S.VSA), Handicapped(S.Handicapped), GreedyRP(S.GreedyRP),
              GreedyMW(S.GreedyMW), RegpressureWasCritical(S.RegpressureWasCritical),
              RefLiveIntervals(S.RefLiveIntervals) {
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
                G.DepEdges.emplace_back(std::make_unique<DepEdge>(
                    NodeMap[Edge->Src], NodeMap[Edge->Dst], Edge->Weight, Edge->WeightHighRP, Edge->ForceSubsequent));
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
            if (!GreedyMW && CanClone) {
                bool NeedToClone = needToClone(Node);
                if (NeedToClone) {
                    Checkpoint = std::make_unique<Schedule>(*this);
                    Checkpoint->addHandicapped(Node->I, RT.getCurrentPressure());
                }
            }

            ImmediateReadyList.erase(
                std::remove(ImmediateReadyList.begin(), ImmediateReadyList.end(), Node), ImmediateReadyList.end());
            ReadyList.erase(std::remove(ReadyList.begin(), ReadyList.end(), Node), ReadyList.end());
            Handicapped.erase(Node->I);

            ScheduledList.push_back(Node);
            RT.update(Node->I);
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

            for (auto &Node : G.InstNodes) {
                if (Node.Preds.empty()) {
                    bool IsInReadyList = std::find(ReadyList.begin(), ReadyList.end(), &Node) != ReadyList.end();
                    bool IsInImmediateReadyList = std::find(ImmediateReadyList.begin(), ImmediateReadyList.end(),
                                                      &Node) != ImmediateReadyList.end();
                    if (!IsInReadyList && !IsInImmediateReadyList) {
                        IGC_ASSERT(std::find(ScheduledList.begin(), ScheduledList.end(), &Node) != ScheduledList.end());
                    }
                }
            }

            IGC_ASSERT(ReadyList.size() + ImmediateReadyList.size() > 0 || ScheduledList.size() == G.InstNodes.size());

            return std::move(Checkpoint);
        }

        bool isComplete() { return ScheduledList.size() == G.InstNodes.size(); }

        bool canHaveSpills() { return RT.isRegpressureCritical(); }

        bool canEverHaveSpills() { return RegpressureWasCritical; }

        void setGreedyRP(bool Greedy) { GreedyRP = Greedy; }

        void setGreedyMW(bool Greedy) { GreedyMW = Greedy; }

        void addHandicapped(Instruction *I, int RP) { Handicapped[I] = RP; }

        void setRefLiveIntervals(const DenseMap<Instruction *, int32_t>& Intervals) { RefLiveIntervals = Intervals; }

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
                auto LowestRP = RT.estimate(Nodes.front()->I);
                InstNodePtrList LowestRPNodes;
                for (InstructionNode *Node : Nodes) {
                    if (RT.estimate(Node->I) == LowestRP) {
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
                            if (VectorType->getNumElements() >= uint(C[Option::PrioritizeLargeBlockLoadsInRP])) {
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

            // experimental heuristic
            auto getLoadsThatUnlockDPASes = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
                InstNodePtrList LoadsThatUnlockDPASes;
                for (InstructionNode *Node : Nodes) {
                    if (!is2dBlockRead(Node->I)) {
                        continue;
                    }
                    for (auto *U : RT.getRealUses(Node->I)) {
                        auto *I = dyn_cast<Instruction>(U);
                        if (!I) {
                            continue;
                        }

                        if (isDPAS(I)) {

                            bool OneOpIsDPAS = false;
                            uint NumOps = I->getNumOperands();
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
                                } else if (RT.getRealOp(OpI) == Node->I) {
                                    NumOps--;
                                }
                            }
                            if (NumOps == 0) {
                                LoadsThatUnlockDPASes.push_back(Node);
                                break;
                            }
                        }
                    }
                }
                if (LoadsThatUnlockDPASes.size() > 0) {
                    Nodes = std::move(LoadsThatUnlockDPASes);
                }
                return Nodes;
            };

            auto getDPASIfExist = [&](InstNodePtrList &Nodes) -> InstNodePtrList & {
                InstNodePtrList DPASNodes;
                for (InstructionNode *Node : Nodes) {
                    if (isDPAS(Node->I)) {
                        DPASNodes.push_back(Node);
                    }
                }
                if (DPASNodes.size() > 0) {
                    Nodes = std::move(DPASNodes);
                }
                return Nodes;
            };

            // ===                                                          ===
            // === Choosing if we have instructions to schedule immediately ===
            // ===                                                          ===

            if (!ImmediateReadyList.empty()) {
                InstructionNode *Node = getFirstNode(ImmediateReadyList);
                std::string Info =
                    std::to_string(RT.getCurrentPressure()) + ", " + std::to_string(RT.estimate(Node->I));
                Info.resize(11, ' ');
                Info = "(" + Info + ") Im: ";
                Info.resize(20, ' ');

                auto *DT = VSA->getDestVector(Node->I);
                auto *V2SP = VSA->getVectorToScalarsPattern(Node->I);

                std::string VS_String = "   ";
                if (DT && DT->isVectorShuffle()) {
                    VS_String = "VS ";
                }
                if (DT && !DT->isVectorShuffle()) {
                    VS_String = "SCA";
                }
                if (DT && DT->isNoOp()) {
                    VS_String = "NOP";
                }
                if (V2SP) {
                    VS_String = "V2S";
                }

                Info += VS_String + "   ";

                PrintDump(Info);
                Node->print(*LogStream);

                return std::make_tuple(Node, false);
            } else {
                // If we have no immediate ready instructions, choose the one from the ready list

                InstructionNode *Node = nullptr;

                IGC_ASSERT(ReadyList.size() > 0);

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
                    Node = getFirstNode(FilteredReadyList);
                    ChooseByRP = RT.isRegpressureHigh(Node->I);
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

                    if (C[Option::PrioritizeDPASHighRP]) {
                        // Experimental heuristic: prioritize DPAS and the instructions that make it possible to
                        // schedule DPAS earlier
                        FilteredReadyList = getDPASIfExist(FilteredReadyList);
                        FilteredReadyList = getLoadsThatUnlockDPASes(FilteredReadyList);
                    }

                    FilteredReadyList = getLowestRegpressureNodes(FilteredReadyList);
                    // If we have several nodes with the same regpressure, choose the one with the highest MaxWeight
                    FilteredReadyList = getMaxWeightNodes(FilteredReadyList, C[Option::UseHighRPWeight] == 1);

                    Node = getFirstNode(FilteredReadyList);

                    // Don't clone if we are choosing by RP
                    CanClone = false;
                }
                IGC_ASSERT(std::find(ReadyList.begin(), ReadyList.end(), Node) != ReadyList.end());
                IGC_ASSERT(Node != nullptr);

                // Dump the info
                std::string Info =
                    std::to_string(RT.getCurrentPressure()) + ", " + std::to_string(RT.estimate(Node->I));
                Info.resize(11, ' ');
                Info = "(" + Info + ") " + (ChooseByRP ? "RP" : "MW") + ": ";
                Info.resize(20, ' ');

                auto *DT = VSA->getDestVector(Node->I);

                std::string VS_String = "   ";
                if (DT && DT->isVectorShuffle()) {
                    VS_String = "VS ";
                }
                if (DT && !DT->isVectorShuffle()) {
                    VS_String = "SCA";
                }
                if (DT && DT->isNoOp()) {
                    VS_String = "NOP";
                }

                Info += VS_String + "   ";

                PrintDump(Info);
                Node->print(*LogStream);

                return std::make_tuple(Node, CanClone);
            }
        }

        bool needToClone(InstructionNode *Node) {
            if (!is2dBlockRead(Node->I)) {
                return false;
            }
            auto Uses = RT.getRealUses(Node->I);
            if (Uses.size() == 0) {
                return false;
            }

            return RefLiveIntervals[Node->I] > C[Option::MinLiveIntervalForCloning];
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
            LogStream << "EnableCodeSchedulingIfNoSpills: " << IGC_GET_FLAG_VALUE(EnableCodeSchedulingIfNoSpills)
                      << "\n";
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
    RPE = &getAnalysis<IGCLivenessAnalysis>();
    FRPE = &getAnalysis<IGCFunctionExternalRegPressureAnalysis>();
    WI = &FRPE->getWIAnalysis(&F);

    bool Changed = false;

    for (auto &BB : F) {
        if (!std::any_of(BB.begin(), BB.end(), [](Instruction &I) { return isDPAS(&I); }))
            continue;

        BBScheduler Scheduler(&BB, RPE, FRPE, AA, VSA, CTX, &Config, LogStream);
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

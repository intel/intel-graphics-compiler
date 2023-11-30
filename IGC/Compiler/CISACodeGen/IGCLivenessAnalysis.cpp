/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "IGCLivenessAnalysis.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"

#include <fstream>
#include <queue>

char IGCLivenessAnalysis::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "igc-df-liveness"
#define PASS_DESCRIPTION2                                                      \
    "computes full dataflow liveness analysis & and register pressure "        \
    "estimator"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 true
IGC_INITIALIZE_PASS_BEGIN(IGCLivenessAnalysis, PASS_FLAG2, PASS_DESCRIPTION2,
                          PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(IGCLivenessAnalysis, PASS_FLAG2, PASS_DESCRIPTION2,
                        PASS_CFG_ONLY2, PASS_ANALYSIS2)
#define PRINT(str) llvm::errs() << str


IGCLivenessAnalysis::IGCLivenessAnalysis() : FunctionPass(ID) { UseWIAnalysis = true; };

IGCLivenessAnalysis::IGCLivenessAnalysis(bool UseWiAnalysisVar /* = false */,
                                         bool DumpToFileVar /* = false */,
                                         const std::string &DumpFileNameVar)
    : FunctionPass(ID) {
    UseWIAnalysis = UseWiAnalysisVar;
    DumpToFile = DumpToFileVar;
    DumpFileName = DumpFileNameVar;
    initializeIGCLivenessAnalysisPass(*PassRegistry::getPassRegistry());
}

unsigned int IGCLivenessAnalysis::registerSizeInBytes() {
    if (CGCtx->platform.isProductChildOf(IGFX_PVC))
        return 64;
    return 32;
}

SIMDMode IGCLivenessAnalysis::bestGuessSIMDSize() {
    switch (IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth)) {
    case 0:
        break;
    case 8:
        return SIMDMode::SIMD8;
    case 16:
        return SIMDMode::SIMD16;
    case 32:
        return SIMDMode::SIMD32;
    }

    if (CGCtx->platform.isProductChildOf(IGFX_PVC))
        return SIMDMode::SIMD32;
    return SIMDMode::SIMD8;
}

void IGCLivenessAnalysis::printNames(const ValueSet &Set, std::string &Name) {
    for (auto *val : Set) {
        llvm::raw_string_ostream Rso(Name);
        // false means don't print type
        val->printAsOperand(Rso, false);
        Rso << ", ";
    }
}

int getAmountOfBB(llvm::Value *Val) {
    llvm::SmallPtrSet<llvm::BasicBlock *, 16> BBSet;
    for (auto U : Val->users()) {

        auto UserInst = llvm::dyn_cast<Instruction>(U);
        if (!UserInst)
            continue;
        auto UserBB = UserInst->getParent();
        BBSet.insert(UserBB);
    }
    return BBSet.size();
}

void IGCLivenessAnalysis::printDefNames(const ValueSet &Set,
                                        std::string &Name) {

    for (auto *Val : Set) {
        llvm::raw_string_ostream Rso(Name);
        // false means don't print type
        Val->printAsOperand(Rso, false);
        int HowManyBB = getAmountOfBB(Val);
        Rso << "(" << std::to_string(Val->getNumUses())
            << ")[" + std::to_string(HowManyBB) << "], ";
    }
}

void IGCLivenessAnalysis::printPhi(const PhiSet &Set, std::string &Output) {

    if (Set.empty())
        return;

    Output += "PHIS:\t[\t" + std::to_string(Set.size()) + "\t] ";
    for (auto Elem : Set) {
        auto &OneBBSet = Elem.second;
        unsigned int Size = OneBBSet.size();
        if (PrinterType > 1) {
            Output += "{ ";
            printNames(OneBBSet, Output);
            Output += "} ";
            Output += "(" + std::to_string(Size) + ")";
        }
    }
    Output += "\n";
}

ValueSet IGCLivenessAnalysis::getDefs(llvm::BasicBlock &BB) {

    ValueSet &BBIn = In[&BB];
    ValueSet &BBOut = Out[&BB];

    ValueSet Difference;
    for (auto *elem : BBOut)
        if (!BBIn.count(elem))
            Difference.insert(elem);

    return Difference;
}

void IGCLivenessAnalysis::printDefs(const ValueSet &In, const ValueSet &Out,
                                    std::string &Output) {
    ValueSet Difference, Common, Kills;

    for (auto *elem : Out)
        if (!In.count(elem))
            Difference.insert(elem);

    for (auto *elem : Out)
        if (In.count(elem))
            Common.insert(elem);

    for (auto *elem : In)
        if (!Common.count(elem))
            Kills.insert(elem);

    if (!Kills.empty()) {
        Output += "KILL:\t[\t" + std::to_string(Kills.size()) + "\t] ";
        if (PrinterType > 2)
            printNames(Kills, Output);
        Output += "\n";
    }

    if (!Difference.empty()) {
        Output += "DEF:\t[\t" + std::to_string(Difference.size()) + "\t] ";
        if (PrinterType > 2)
            printDefNames(Difference, Output);
        Output += "\n";
    }
}

void IGCLivenessAnalysis::printName(llvm::Value *Val, std::string &String) {
    llvm::raw_string_ostream Rso(String);
    // false means don't print type
    Val->printAsOperand(Rso, false);
}

void IGCLivenessAnalysis::printSets(llvm::BasicBlock *BB, std::string &Output,
                                    unsigned int SIMD) {
    if (PrinterType <= 0)
        return;

    Output += "block: " + BB->getName().str() + " ";
    Output += "function: " + BB->getParent()->getName().str() + "\n";

    ValueSet &PtrSetIn = In[BB];
    ValueSet &PtrSetOut = Out[BB];
    PhiSet &PtrSetPhi = InPhi[BB];

    printPhi(PtrSetPhi, Output);

    Output += "IN: \t[\t" + std::to_string(PtrSetIn.size()) + "\t] ";
    if (PrinterType > 2)
        printNames(PtrSetIn, Output);
    Output += "\n";

    printDefs(PtrSetIn, PtrSetOut, Output);

    Output += "OUT:\t[\t" + std::to_string(PtrSetOut.size()) + "\t] ";
    if (PrinterType > 2)
        printNames(PtrSetOut, Output);
    Output += "\n";

    intraBlock(*BB, Output, SIMD);
}

// for every successor we take all of it's IN values
// and the PHI values that are coming from our BB
void IGCLivenessAnalysis::mergeSets(ValueSet *OutSet, llvm::BasicBlock *Succ) {
    for (auto elem : In[Succ])
        OutSet->insert(elem);
}

// we scan through all successors and merge their INSETs as our OUTSET
void IGCLivenessAnalysis::combineOut(llvm::BasicBlock *BB, ValueSet *Set) {
    ValueSet *OutSet = &Out[BB];
    for (llvm::succ_iterator SI = llvm::succ_begin(BB), SE = llvm::succ_end(BB);
         SI != SE; ++SI) {
        llvm::BasicBlock *Successor = *SI;
        mergeSets(OutSet, Successor);
    }
}

void addToSet(llvm::Instruction *Inst, ValueSet &Set) {
    for (auto &Op : Inst->operands()) {
        llvm::Value *V = Op.get();
        // We are counting only instructions right now
        // potetntially we should also count globals, but
        // we defintely shouldn't count:
        // br label %bb1 (basic block names)
        // call %functionName (function names)
        // add %a, 1 (constants)
        if (!(llvm::isa<llvm::Instruction>(V) || llvm::isa<llvm::Argument>(V)))
            continue;
        Set.insert(V);
    }
}

// idea is simple, each predecessor block that converges into our block
// has its own set of PHI values, that it has to deliver
// so we take values that are coming from each block
// and add them to their OUT set directly
void IGCLivenessAnalysis::addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet) {
    for (auto BB : Phi->blocks()) {
        auto ValueFromOurBlock = Phi->getIncomingValueForBlock(BB);
        auto *OutSet = &Out[BB];
        if (llvm::isa<llvm::Constant>(ValueFromOurBlock))
            continue;
        OutSet->insert(ValueFromOurBlock);
        // for visualization purposes also add them to INPHI set
        // this way it will be easier to scan them and print
        (*InPhiSet)[BB].insert(ValueFromOurBlock);
    }
}
// scan through block in reversed order and add each operand
// into IN block while deleting defined values
void IGCLivenessAnalysis::processBlock(llvm::BasicBlock *BB, ValueSet &Set,
                                       PhiSet *PhiSet) {
    for (auto RI = BB->rbegin(), RE = BB->rend(); RI != RE; ++RI) {
        llvm::Instruction *Inst = &(*RI);
        Set.erase(Inst);

        auto Phi = llvm::dyn_cast<llvm::PHINode>(Inst);
        if (Phi) {
            addToPhiSet(Phi, PhiSet);
            continue;
        }
        addToSet(Inst, Set);
    }
}

void IGCLivenessAnalysis::livenessAnalysis(llvm::Function &F) {
    std::queue<llvm::BasicBlock *> Worklist;
    for (BasicBlock &BB : F)
        Worklist.push(&BB);

    while (!Worklist.empty()) {

        llvm::BasicBlock *BB = Worklist.front();
        Worklist.pop();

        ValueSet *InSet = &In[BB];
        ValueSet *OutSet = &Out[BB];
        PhiSet *InPhiSet = &InPhi[BB];

        combineOut(BB, OutSet);

        unsigned int SizeBefore = InSet->size();
        unsigned int SizeBeforePhi = InPhiSet->size();

        // this should be a copy, everything that should go OUT
        // minus what is defined and what was intermediate, should go IN
        // so we are feeding copy of the OUT as IN, and delete everything
        // that was defined in the block while processing it
        *InSet = *OutSet;
        processBlock(BB, *InSet, InPhiSet);

        // sets can only grow monotonically, updated set is a bigger set
        bool IsSetChanged = InSet->size() != SizeBefore;
        bool IsPhiSetChanged = InPhiSet->size() != SizeBeforePhi;
        if (IsSetChanged || IsPhiSetChanged)
            for (auto *Pred : predecessors(BB))
                Worklist.push(Pred);
    }
}

unsigned int IGCLivenessAnalysis::estimateSizeInBytes(ValueSet &Set,
                                                      const DataLayout &DL,
                                                      unsigned int SIMD) {
    unsigned int Result = 0;
    for (auto El : Set) {
        auto Type = El->getType();
        auto TypeSizeInBits = DL.getTypeSizeInBits(Type);
        unsigned int Multiplier = SIMD;
        if (UseWIAnalysis && WI->isUniform(El))
            Multiplier = 1;
        uint64_t SizeInBytes = TypeSizeInBits * Multiplier / 8;
        Result += SizeInBytes;
    }

    return Result;
}

void IGCLivenessAnalysis::printInstruction(llvm::Instruction *Inst,
                                           std::string &Str) {
    llvm::raw_string_ostream rso(Str);
    Inst->print(rso, false);
    rso << "\n";
}

void IGCLivenessAnalysis::printIntraBlock(llvm::BasicBlock &BB,
                                          std::string &Output,
                                          InsideBlockPressureMap &BBListing) {
    unsigned int RegisterSizeInBytes = registerSizeInBytes();
    for (auto &I : BB) {
        llvm::Instruction *Inst = &I;
        if (UseWIAnalysis && WI->isUniform(Inst)) {
            Output += "U: ";
        } else {
            Output += "N: ";
        }
        unsigned int SizeInBytes = BBListing[Inst];
        unsigned int AmountOfRegistersRoundUp =
            (SizeInBytes + RegisterSizeInBytes - 1) / RegisterSizeInBytes;
        Output += std::to_string(SizeInBytes) + " (" +
                  std::to_string(AmountOfRegistersRoundUp) + ")" + "    \t";
        printInstruction(Inst, Output);
    }
}

void IGCLivenessAnalysis::collectPressureForBB(
    llvm::BasicBlock &BB, InsideBlockPressureMap &BBListing,
    unsigned int SIMD) {
    const DataLayout &DL = BB.getParent()->getParent()->getDataLayout();
    ValueSet &BBOut = Out[&BB];
    // this should be a copy
    ValueSet BBSet = BBOut;

    for (auto RI = BB.rbegin(), RE = BB.rend(); RI != RE; ++RI) {

        llvm::Instruction *Inst = &(*RI);

        unsigned int Size = estimateSizeInBytes(BBSet, DL, SIMD);

        auto Phi = llvm::dyn_cast<llvm::PHINode>(Inst);
        if (!Phi) {
            addToSet(Inst, BBSet);
        }

        BBListing[Inst] = Size;
        BBSet.erase(Inst);
    }
}

void IGCLivenessAnalysis::intraBlock(llvm::BasicBlock &BB, std::string &Output,
                                     unsigned int SIMD) {
    InsideBlockPressureMap BBListing;
    collectPressureForBB(BB, BBListing, SIMD);
    printIntraBlock(BB, Output, BBListing);
}

void IGCLivenessAnalysis::dumpRegPressure(llvm::Function &F,
                                          unsigned int SIMD) {
    // force print all
    PrinterType = 5;

    std::string Output;
    Output.reserve(32768);

    IGC::Debug::DumpLock();
    {
        std::stringstream ss;
        ss << F.getName().str() << DumpFileName << "_RegEst";
        auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                        .Hash(CGCtx->hash)
                        .Type(CGCtx->type)
                        .Retry(CGCtx->m_retryManager.GetRetryId())
                        .Pass(ss.str().c_str())
                        .Extension("ll");

        std::ofstream OutputFile(Name.str());
        for (BasicBlock &BB : F) {
            // prints information for one BB
            printSets(&BB, Output, SIMD);
            if (OutputFile.is_open())
                OutputFile << Output;
            Output.clear();
        }

        OutputFile.close();
    }
    IGC::Debug::DumpUnlock();
}

bool IGCLivenessAnalysis::runOnFunction(llvm::Function &F) {
    if (UseWIAnalysis)
        WI = &getAnalysis<WIAnalysis>();
    CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    livenessAnalysis(F);

    unsigned int SIMD = numLanes(bestGuessSIMDSize());

    if (DumpToFile) {
        dumpRegPressure(F, SIMD);
    } else {
        // basically only for LIT testing
        std::string Output;
        // no particular reason behind this, just big enough power of 2
        // helps to reduce printing time, by preemptively allocating
        // memory
        Output.reserve(32768);
        for (BasicBlock &BB : F) {
            // why 16? because test is written for 16
            printSets(&BB, Output, 16);
        }
        PRINT(Output);
        Output.clear();
    }

    return true;
}

FunctionPass *IGC::createIGCEarlyRegEstimator(bool UseWIAnalysis /*= false*/,
                                              bool DumpToFile /* = false */,
                                              std::string DumpFileName) {
    return new IGCLivenessAnalysis(UseWIAnalysis, DumpToFile, DumpFileName);
}

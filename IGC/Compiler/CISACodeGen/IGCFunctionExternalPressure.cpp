/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCFunctionExternalPressure.h"


char IGCFunctionExternalRegPressureAnalysis::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "igc-external-pressure-analysis"
#define PASS_DESCRIPTION2                                                      \
    "computes full dataflow liveness analysis & and register pressure "        \
    "estimator"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 true
IGC_INITIALIZE_PASS_BEGIN(IGCFunctionExternalRegPressureAnalysis, PASS_FLAG2, PASS_DESCRIPTION2,
                          PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(IGCFunctionExternalRegPressureAnalysis, PASS_FLAG2, PASS_DESCRIPTION2,
                        PASS_CFG_ONLY2, PASS_ANALYSIS2)
#define PRINT(str) llvm::errs() << str


IGCFunctionExternalRegPressureAnalysis::IGCFunctionExternalRegPressureAnalysis() : ModulePass(ID) {
    initializeIGCFunctionExternalRegPressureAnalysisPass(*PassRegistry::getPassRegistry());
};


std::unique_ptr<WIAnalysisRunner> IGCFunctionExternalRegPressureAnalysis::runWIAnalysis(Function &F) {

    TranslationTable TT;
    TT.run(F);

    auto *DT = &getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
    auto *PDT = &getAnalysis<PostDominatorTreeWrapperPass>(F).getPostDomTree();
    auto *LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

    auto WI = std::make_unique<WIAnalysisRunner>(&F, LI, DT, PDT, MDUtils, CGCtx, ModMD, &TT);
    WI->run();
    return WI;
}


unsigned int IGCFunctionExternalRegPressureAnalysis::estimateSizeInBytes(ValueSet &Set,
                                                      Function &F,
                                                      unsigned int SIMD, WIAnalysisRunner& WI) {
    const DataLayout &DL = F.getParent()->getDataLayout();

    unsigned int Result = 0;
    for (auto El : Set) {
        auto Type = El->getType();
        unsigned int TypeSizeInBits = (unsigned int) DL.getTypeSizeInBits(Type);
        unsigned int Multiplier = SIMD;
        if (WI.isUniform(El))
            Multiplier = 1;
        unsigned int SizeInBytes = TypeSizeInBits * Multiplier / 8;
        Result += SizeInBytes;
    }

    return Result;
}

// for every successor we take all of it's IN values
// and the PHI values that are coming from our BB
void IGCFunctionExternalRegPressureAnalysis::mergeSets(ValueSet *OutSet, llvm::BasicBlock *Succ) {
    for (auto elem : In[Succ])
        OutSet->insert(elem);
}

// we scan through all successors and merge their INSETs as our OUTSET
void IGCFunctionExternalRegPressureAnalysis::combineOut(llvm::BasicBlock *BB, ValueSet *Set) {
    ValueSet *OutSet = &Out[BB];
    for (llvm::succ_iterator SI = llvm::succ_begin(BB), SE = llvm::succ_end(BB);
         SI != SE; ++SI) {
        llvm::BasicBlock *Successor = *SI;
        mergeSets(OutSet, Successor);
    }
}

void IGCFunctionExternalRegPressureAnalysis::addOperandsToSet(llvm::Instruction *Inst, ValueSet &Set) {
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

void IGCFunctionExternalRegPressureAnalysis::addNonLocalOperandsToSet(llvm::Instruction *Inst, ValueSet &Set) {
    for (auto &Op : Inst->operands()) {
        llvm::Value *V = Op.get();
        // We are counting only instructions right now
        // potetntially we should also count globals, but
        // we defintely shouldn't count:
        // br label %bb1 (basic block names)
        // call %functionName (function names)
        // add %a, 1 (constants)
        Instruction *I = dyn_cast<Instruction>(V);
        bool IsInstruction = I != nullptr;
        bool OperandInDifferentBB = IsInstruction && (I->getParent() != Inst->getParent());
        bool IsArgument = !IsInstruction && llvm::isa<llvm::Argument>(V);
        if (OperandInDifferentBB || IsArgument)
        {
            Set.insert(V);
        }
    }
}


void IGCFunctionExternalRegPressureAnalysis::collectPressureForBB(
    llvm::BasicBlock &BB, InsideBlockPressureMap &BBListing,
    unsigned int SIMD, WIAnalysisRunner& WI) {

    ValueSet &BBOut = Out[&BB];
    // this should be a copy
    ValueSet BBSet = BBOut;

    for (auto RI = BB.rbegin(), RE = BB.rend(); RI != RE; ++RI) {

        llvm::Instruction *Inst = &(*RI);

        unsigned int Size = estimateSizeInBytes(BBSet, *BB.getParent(), SIMD, WI);

        auto Phi = llvm::dyn_cast<llvm::PHINode>(Inst);
        if (!Phi) {
            addOperandsToSet(Inst, BBSet);
        }

        BBListing[Inst] = Size;
        BBSet.erase(Inst);
    }
}

// idea is simple, each predecessor block that converges into our block
// has its own set of PHI values, that it has to deliver
// so we take values that are coming from each block
// and add them to their OUT set directly
void IGCFunctionExternalRegPressureAnalysis::addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet) {
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
void IGCFunctionExternalRegPressureAnalysis::processBlock(llvm::BasicBlock *BB, ValueSet &Set,
                                       PhiSet *PhiSet) {
    for (auto RI = BB->rbegin(), RE = BB->rend(); RI != RE; ++RI) {
        llvm::Instruction *Inst = &(*RI);
        Set.erase(Inst);

        auto Phi = llvm::dyn_cast<llvm::PHINode>(Inst);
        if (Phi) {
            addToPhiSet(Phi, PhiSet);
            continue;
        }
        addNonLocalOperandsToSet(Inst, Set);
    }
}

void IGCFunctionExternalRegPressureAnalysis::livenessAnalysis(llvm::Function &F) {
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


void IGCFunctionExternalRegPressureAnalysis::generateTableOfPressure(llvm::Module &M, unsigned int SIMD) {

    // basic preprocessing, scan all of the functions,
    // collect callsite pressure for eash callsite
    for (auto &F : M) {
        if(F.isDeclaration()) continue;

        livenessAnalysis(F);
        std::unique_ptr<WIAnalysisRunner> WI = runWIAnalysis(F);

        for (auto &BB : F) {

            std::unique_ptr<InsideBlockPressureMap> PressureMap;
            for (auto &I : BB) {

                auto *Call = llvm::dyn_cast<CallInst>(&I);

                if (!Call) continue;
                if (Call->getCallingConv() != CallingConv::SPIR_FUNC) continue;

                if(!PressureMap)
                    PressureMap = getPressureMapForBB(BB, SIMD, *WI);

                CallSitePressure[Call] = (*PressureMap)[&I];
            }
        }
    }

    llvm::SmallVector<llvm::Function *, 16> PostOrder;

    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    auto ExtNode = CG.getExternalCallingNode();
    for (auto I = po_begin(ExtNode), E = po_end(ExtNode); I != E; ++I) {
        auto CGNode = *I;

        if (auto F = CGNode->getFunction()) {
            if (F->isDeclaration())
                continue;
            // Ignore externally linked functions and stackcall functions
            if (F->hasFnAttribute("referenced-indirectly") ||
                F->hasFnAttribute("invoke_simd_target") ||
                F->hasFnAttribute("hasRecursion") ||
                F->hasFnAttribute("visaStackCall"))
                continue;

            PostOrder.push_back(F);
        }
    }

    // now it's REVERSE POST ORDER
    // we will use it to process external pressure for a function
    std::reverse(PostOrder.begin(), PostOrder.end());

    for (auto El : PostOrder) {
        unsigned int MaxPressure = 0;
        // top level functions won't go inside this cycle
        // noone is calling them
        for (auto U : El->users()) {
            CallInst *Callsite = llvm::dyn_cast<CallInst>(U);
            if(!Callsite) continue;
            // at this point, because we process in a specific order, every function
            // that could potentially call our function, should be processed already
            // and we know its external pressure, for a top level function it will be 0
            // and we will process them first
            unsigned int ExternalPressure = CallSitePressure[Callsite] + ExternalFunctionPressure[Callsite->getFunction()];
            MaxPressure = std::max(MaxPressure, ExternalPressure);
        }
        ExternalFunctionPressure[El] = MaxPressure;
    }
}

bool IGCFunctionExternalRegPressureAnalysis::runOnModule(llvm::Module &M) {

    CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    unsigned int SIMD = numLanes(bestGuessSIMDSize());
    generateTableOfPressure(M, SIMD);

    return true;
}

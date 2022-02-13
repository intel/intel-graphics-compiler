/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VariableReuseAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/Debug.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include <algorithm>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

namespace
{
    // If V is scalar, return 1.
    // if V is vector, return the number of elements.
    inline int getNumElts(Value* V) {
        IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
        return VTy ? (int)VTy->getNumElements() : 1;
    }

    inline int getTypeSizeInBits(Type* Ty) {
        int scalarBits = Ty->getScalarSizeInBits();
        IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
        return scalarBits * (VTy ? (int)VTy->getNumElements() : 1);
    }
}

char VariableReuseAnalysis::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(VariableReuseAnalysis, "VariableReuseAnalysis",
    "VariableReuseAnalysis", false, true)
    // IGC_INITIALIZE_PASS_DEPENDENCY(RegisterEstimator)
    IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
    IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch)
    IGC_INITIALIZE_PASS_DEPENDENCY(DeSSA)
    IGC_INITIALIZE_PASS_DEPENDENCY(CoalescingEngine)
    IGC_INITIALIZE_PASS_DEPENDENCY(BlockCoalescing)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_END(VariableReuseAnalysis, "VariableReuseAnalysis",
        "VariableReuseAnalysis", false, true)

    llvm::FunctionPass* IGC::createVariableReuseAnalysisPass() {
    return new VariableReuseAnalysis;
}

VariableReuseAnalysis::VariableReuseAnalysis()
    : FunctionPass(ID),
    m_pCtx(nullptr), m_WIA(nullptr), m_LV(nullptr), m_DeSSA(nullptr),
    m_PatternMatch(nullptr), m_coalescingEngine(nullptr),
    m_RPE(nullptr), m_SimdSize(0), m_IsFunctionPressureLow(Status::Undef),
    m_IsBlockPressureLow(Status::Undef) {
    initializeVariableReuseAnalysisPass(*PassRegistry::getPassRegistry());
}

bool VariableReuseAnalysis::runOnFunction(Function& F)
{
    m_F = &F;

    m_WIA = &(getAnalysis<WIAnalysis>());
    if (IGC_IS_FLAG_DISABLED(DisableDeSSA))
    {
        m_DeSSA = &getAnalysis<DeSSA>();
    }
    m_LV = &(getAnalysis<LiveVarsAnalysis>().getLiveVars());
    m_PatternMatch = &getAnalysis<CodeGenPatternMatch>();
    m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_coalescingEngine = &getAnalysis<CoalescingEngine>();
    m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    m_DL = &F.getParent()->getDataLayout();

    // FIXME: enable RPE.
    // m_RPE = &getAnalysis<RegisterEstimator>();

    // Nothing but cleanup data from previous runs.
    reset();

    if (IGC_IS_FLAG_ENABLED(EnableVariableAlias) &&
        m_DeSSA &&
        !m_pCtx->getModuleMetaData()->compOpt.OptDisable &&
        m_pCtx->platform.GetPlatformFamily() >= IGFX_GEN9_CORE)
    {
        // Setup ArgDeSSARoot (for subroutine, it might be conservative,
        // but it should work.).
        m_ArgDeSSARoot.clear();
        for (auto II = F.arg_begin(), IE = F.arg_end(); II != IE; ++II)
        {
            Value* A = II;
            if (Value * R = m_DeSSA->getRootValue(A)) {
                m_ArgDeSSARoot.push_back(R);
            }
        }

        // 0. Merge Variables
        //    Merge two different variables into a single one.
        //    The two vars that will be merged should have the same
        //    size/type and normally are defined with different values.
        //    Once merged, they are put in the same DeSSA congruent class
        mergeVariables(&F);

        // 1. SubVector aliasing
        //    Two variables alias each other if they have the same values.
        //    Although they have different names, the two variables share
        //    the same values over their live ranges. The cases such as
        //    extractElement/insertElement, etc. Once aliasing is identified,
        //    the liveness of the alias root is updated to be the sum of both.
        //    This is the same as DeSSA alias.
        InsertElementAliasing(&F);

        // 2. Handle extractElement, etc that handles a single instruction or
        //    a few instruction, not invovled in a complicated patterns like
        //    InsertElement.
        visitLiveInstructions(&F);

        postProcessing();

        if (IGC_IS_FLAG_ENABLED(DumpVariableAlias))
        {
            auto name =
                Debug::DumpName(Debug::GetShaderOutputName())
                .Hash(m_pCtx->hash)
                .Type(m_pCtx->type)
                .Pass("VariableAlias")
                .PostFix(F.getName().str())
                .Extension("txt");
            printAlias(Debug::Dump(name, Debug::DumpType::DBG_MSG_TEXT).stream(), m_F);
        }
    }

    m_F = nullptr;
    return false;
}

static unsigned getMaxReuseDistance(uint16_t size) {
    return (size == 8) ? 10 : 5;
}

bool VariableReuseAnalysis::checkUseInst(Instruction* UseInst, LiveVars* LV) {
    BasicBlock* CurBB = UseInst->getParent();
    if (UseInst->isUsedOutsideOfBlock(CurBB))
        return false;

    // This situation can occur:
    //
    //     ,------.
    //     |      |
    //     |      v
    //     |   t2 = phi ... t1 ...
    //     |      |
    //     |      v
    //     |   t1 = ...
    //     |  ... = ... t1 ...
    //     |      |
    //     `------'
    //
    // Disallow reuse if t1 has a phi use.
    // Disallow reuse if t1 has a far away use when the pressure is not low.
    unsigned DefLoc = LV->getDistance(UseInst);
    unsigned FarUseLoc = 0;
    for (auto UI : UseInst->users()) {
        if (isa<PHINode>(UI))
            return false;

        auto Inst = dyn_cast<Instruction>(UI);
        if (!Inst)
            return false;
        unsigned UseLoc = LV->getDistance(Inst);
        FarUseLoc = std::max(FarUseLoc, UseLoc);
    }

    // When the whole function or block pressure is low, skip the distance check.
    if (isCurFunctionPressureLow() || isCurBlockPressureLow())
        return true;

    // Use distance to limit reuse.
    const unsigned FarUseDistance = getMaxReuseDistance(m_SimdSize);
    return FarUseLoc <= DefLoc + FarUseDistance;
}

bool VariableReuseAnalysis::checkDefInst(Instruction* DefInst,
    Instruction* UseInst, LiveVars* LV) {
    IGC_ASSERT(nullptr != DefInst);
    IGC_ASSERT(nullptr != UseInst);
    if (isa<PHINode>(DefInst))
        return false;

    if (auto CI = dyn_cast<CallInst>(DefInst)) {
        Function* F = CI->getCalledFunction();
        // Do not reuse the return symbol of subroutine/stack calls.
        if (!F || !F->isDeclaration())
            return false;

        if (isa<GenIntrinsicInst>(DefInst)) {
            // Just skip all gen intrinsic calls. Some intrinsic calls may have
            // special meaning.
            return false;
        }
    }

    // This is a block level reuse.
    BasicBlock* CurBB = UseInst->getParent();
    if (DefInst->getParent() != CurBB || DefInst->isUsedOutsideOfBlock(CurBB))
        return false;

    // Check whether UseInst is the last use of DefInst. If not, this source
    // variable cannot be reused.
    Instruction* LastUse = LV->getLVInfo(DefInst).findKill(CurBB);
    if (LastUse != UseInst)
        return false;

    // When the whole function or block pressure is low, skip the distance check.
    if (isCurFunctionPressureLow() || isCurBlockPressureLow())
        return true;

    // Use distance to limit far reuses.
    unsigned DefLoc = LV->getDistance(DefInst);
    unsigned UseLoc = LV->getDistance(UseInst);
    const unsigned FarDefDistance = getMaxReuseDistance(m_SimdSize);
    return UseLoc <= DefLoc + FarDefDistance;
}

void VariableReuseAnalysis::mergeVariables(Function* F)
{
    for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II)
    {
        Instruction* I = &*II;
        if (!m_PatternMatch->NeedInstruction(*I))
            continue;
        if (GenIntrinsicInst * CI = dyn_cast<GenIntrinsicInst>(I))
        {
            switch (CI->getIntrinsicID()) {
            case GenISAIntrinsic::GenISA_sub_group_dpas:
            case GenISAIntrinsic::GenISA_dpas:
            {
                if (!m_DeSSA) {
                    // Skip if no DeSSA
                    break;
                }

                Value* out = CI;
                Value* input = CI->getOperand(0);

                if (!(isa<Instruction>(input) || isa<Argument>(input)))
                {
                    // input may be a constant for example
                    break;
                }
                Type* OTy = out->getType();
                Type* ITy = input->getType();
                if (getTypeSizeInBits(OTy) != getTypeSizeInBits(ITy))
                {
                    // If out and input are different size, skip
                    break;
                }

                // For now, coalescing out and input if at least one of them
                // is local, and input is the last use.
                if ((m_WIA && m_WIA->whichDepend(out) == m_WIA->whichDepend(input)) &&
                    !hasBeenPayloadCoalesced(input) &&
                    !hasBeenPayloadCoalesced(out) &&
                    !m_DeSSA->interfere(out, input))
                {
                    // For dpas, alignment for out/input are the same
                    e_alignment align = EALIGN_AUTO;
                    if (m_WIA) {
                        align = GetPreferredAlignment(out, m_WIA, m_pCtx);
                    }
                    // Make sure that nodes have been created before doing union
                    m_DeSSA->addReg(out, align);
                    m_DeSSA->addReg(input, align);
                    m_DeSSA->unionRegs(out, input);
                }
                break;
            }
            default:
                break;
            }  // End of switch
        }
    }
}

void VariableReuseAnalysis::visitLiveInstructions(Function* F)
{
    for (auto BI = F->begin(), BE = F->end(); BI != BE; ++BI)
    {
        BasicBlock* BB = &*BI;
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
        {
            Instruction& I = *II;
            if (!m_PatternMatch->NeedInstruction(I))
                continue;
            visit(I);
        }
    }
}

// Given a root Value RootVal, all its values that are coalesced
// with it are in AllVals. This function finds the place to insert
// the lifeTimeStart for RootVal, which is either at the end of a
// BB or right before the first definition. If any value is argument,
// no lifeTimeStart is needed.
// (For assisting visa for liveness analysis.)
void VariableReuseAnalysis::setLifeTimeStartPos(
    Value* RootVal,
    ValueVectorTy& AllVals,
    BlockCoalescing* theBC)
{
    SmallSet<BasicBlock*, 8> defBBSet;
    SmallSet<BasicBlock*, 8> phiSrcMovBBSet;
    for (int i = 0, sz = (int)AllVals.size(); i < sz; ++i)
    {
        Value* V = AllVals[i];
        Instruction* I = dyn_cast<Instruction>(V);
        if (!I) {
            // For arg, global etc., its start is on entry.
            // Thus, no need to insert lifetime start.
            defBBSet.clear();
            phiSrcMovBBSet.clear();
            break;
        }

        if (PHINode * PHI = dyn_cast<PHINode>(I)) {
            Value* PHI_root = m_DeSSA->getRootValue(PHI);
            int sz1 = (int)PHI->getNumIncomingValues();
            for (int i1 = 0; i1 < sz1; ++i1)
            {
                Value* Src = PHI->getIncomingValue(i1);
                Value* Src_root = m_DeSSA->getRootValue(Src);
                if (!Src_root || PHI_root != Src_root) {
                    // Need Src-side phi mov
                    BasicBlock* BB = PHI->getIncomingBlock(i1);
                    phiSrcMovBBSet.insert(BB);
                }
            }
        }
        else {
            BasicBlock* BB = I->getParent();
            defBBSet.insert(BB);
        }
    }

    if (defBBSet.size() == 0 && phiSrcMovBBSet.size() == 0) {
        return;
    }

    auto BSI = defBBSet.begin();
    auto BSE = defBBSet.end();
    BasicBlock* NearestDomBB = *BSI;
    for (++BSI; BSI != BSE; ++BSI)
    {
        BasicBlock* aB = *BSI;
        NearestDomBB = m_DT->findNearestCommonDominator(NearestDomBB, aB);
    }

    // phiSrcMovBBSet
    for (auto II = phiSrcMovBBSet.begin(), IE = phiSrcMovBBSet.end();
        II != IE; ++II)
    {
        BasicBlock* aB = *II;
        NearestDomBB = m_DT->findNearestCommonDominator(NearestDomBB, aB);
    }

    // Skip emptry BBs that are going to be skipped in codegen emit.
    while (theBC->IsEmptyBlock(NearestDomBB))
    {
        auto Node = m_DT->getNode(NearestDomBB);
        NearestDomBB = Node->getIDom()->getBlock();
    }

    if (defBBSet.count(NearestDomBB))
    {
        // lifeTimeStart insert pos is in a BB where a def exists
        m_LifetimeAt1stDefOfBB[RootVal] = NearestDomBB;
    }
    else
    {
        // No def in the bb, it must be at the end of BB
        // (must be before phiSrcMov too).
        m_LifetimeAtEndOfBB[NearestDomBB].push_back(RootVal);
    }
}

void VariableReuseAnalysis::postProcessing()
{
    // BlockCoalescing : check if a BB is a to-be-skipped empty BB.
    // It is used for selecting BB to add lifetime start
    BlockCoalescing* theBC = &getAnalysis<BlockCoalescing>();
    if (!m_DeSSA || m_pCtx->getVectorCoalescingControl() < 3)
        return;

    DenseMap<Value*, int> dessaRootVisited;
    auto IS = m_aliasMap.begin();
    auto IE = m_aliasMap.end();
    for (auto II = IS; II != IE; ++II)
    {
        SSubVecDesc* SV = II->second;
        Value* aliasee = SV->BaseVector;
        if (aliasee != SV->Aliaser)
            continue;

        // An alias set of an aliasee (base) :
        //     The aliasee and all its aliasers; and for each of them, all values
        //     in its dessa CC.
        //
        // For each Aliasee, record its lifetime start, which is the
        // nearest dominator that dominates all value defs in an alias set.
        // This BB is either one that has no defintion of values in the set;
        // or one that has a defintion to a value in the set. For the former,
        // m_LifetimeAtEndOfBB is used to keep track of it; for the latter,
        // m_LifetimeAt1stDefOfBB is used.
        ValueVectorTy AllVals;
        SmallVector<Value*, 8> valInCC;
        m_DeSSA->getAllValuesInCongruentClass(aliasee, valInCC);
        AllVals.insert(AllVals.end(), valInCC.begin(), valInCC.end());

        // update visited for aliasee
        Value* aliaseeRoot = m_DeSSA->getRootValue(aliasee);
        aliaseeRoot = aliaseeRoot ? aliaseeRoot : aliasee;
        dessaRootVisited[aliaseeRoot] = 1;
        for (int i = 0, sz = (int)SV->Aliasers.size(); i < sz; ++i)
        {
            SSubVecDesc* aSV = SV->Aliasers[i];
            Value* aliaser = aSV->Aliaser;
            valInCC.clear();
            m_DeSSA->getAllValuesInCongruentClass(aliaser, valInCC);
            AllVals.insert(AllVals.end(), valInCC.begin(), valInCC.end());

            // update visited for aliaser
            Value* aRoot = m_DeSSA->getRootValue(aliaser);
            aRoot = aRoot ? aRoot : aliaser;
            dessaRootVisited[aRoot] = 1;
        }

        setLifeTimeStartPos(aliaseeRoot, AllVals, theBC);
    }

    // For other vector values
    if (m_pCtx->getVectorCoalescingControl() < 4)
        return;

    for (auto II = inst_begin(*m_F), IE = inst_end(*m_F); II != IE; ++II)
    {
        Instruction* I = &*II;
        if (!m_PatternMatch->NeedInstruction(*I))
            continue;
        if (!I->getType()->isVectorTy())
            continue;
        Value* rootV = m_DeSSA->getRootValue(I);
        rootV = rootV ? rootV : I;
        if (dessaRootVisited.find(rootV) != dessaRootVisited.end()) {
            // Already handled by sub-vector aliasing, skip
            continue;
        }

        ValueVectorTy AllVals;
        SmallVector<Value*, 8> valInCC;
        m_DeSSA->getAllValuesInCongruentClass(rootV, valInCC);
        AllVals.insert(AllVals.end(), valInCC.begin(), valInCC.end());

        setLifeTimeStartPos(rootV, AllVals, theBC);
    }
}

Value* VariableReuseAnalysis::getRootValue(Value* V)
{
    Value* dessaRV = nullptr;
    if (m_DeSSA) {
        dessaRV = m_DeSSA->getRootValue(V);
    }
    return dessaRV ? dessaRV : V;
}

Value* VariableReuseAnalysis::getAliasRootValue(Value* V)
{
    Value* V_nv = m_DeSSA ? m_DeSSA->getNodeValue(V) : V;
    auto II = m_aliasMap.find(V_nv);
    if (II == m_aliasMap.end()) {
        return V_nv;
    }
    return II->second->BaseVector;
}

// Returns true for the following pattern:
//   a = extractElement <vectorType> EEI_Vec, <constant EEI_ix>
//   b = insertElement  <vectorType> V1,  a,  <constant IEI_ix>
// where EEI_ix and IEI_ix are constants; Return false otherwise.
bool VariableReuseAnalysis::getVectorIndicesIfConstant(
    InsertElementInst* IEI, int& IEI_ix, Value*& EEI_Vec, int& EEI_ix)
{
    // Check if I has constant index, skip if not.
    ConstantInt* CI = dyn_cast<ConstantInt>(IEI->getOperand(2));
    if (!CI) {
        return false;
    }
    IEI_ix = (int)CI->getZExtValue();

    // Check that the elements inserted are from extractElement
    // Also, special-handling of insertelement itself.
    Value* elem = IEI->getOperand(1);
    ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(elem);
    if (!EEI) {
        // Just insertelement itself
        EEI_ix = 0;
        EEI_Vec = elem;
        return true;
    }
    ConstantInt* CI1 = dyn_cast<ConstantInt>(EEI->getIndexOperand());
    if (!CI1) {
        return false;
    }
    EEI_ix = (int)CI1->getZExtValue();
    EEI_Vec = EEI->getVectorOperand();
    return true;
}

void VariableReuseAnalysis::visitExtractElementInst(ExtractElementInst& I)
{
    if (m_pCtx->getVectorCoalescingControl() == 0) {
        return;
    }

    ExtractElementInst* EEI = &I;
    Value* vecVal = EEI->getVectorOperand();

    // Before doing extractMask explicitly, don't do aliasing
    // for extractElement whose vector operand are the candidate
    // of the existing extractMask optimization, as doing so will
    // disable the existing extractMask optimization, which will
    // cause perf regression.
    if (Instruction * Inst = dyn_cast<Instruction>(vecVal))
    {
        if (IGC_IS_FLAG_DISABLED(EnableExtractMask) &&
            (isSampleInstruction(Inst) || isLdInstruction(Inst)))
        {
            // OCL can have sample (image read), not ld. For 3d/mac,
            // need to check more
            return;
        }
    }

    // If inst is dead, EEI is an argument, or EEI & vecVal have
    // different uniformness, skip it. (Current igc & visa interface
    // requires any argument value to be a root value, not alias.)
    if (m_HasBecomeNoopInsts.count(EEI) ||
        m_DeSSA->isNoopAliaser(EEI) ||
        isOrCoalescedWithArg(EEI) ||
        (m_WIA && m_WIA->whichDepend(EEI) != m_WIA->whichDepend(vecVal))) {
        return;
    }

    Value* EEI_nv = m_DeSSA->getNodeValue(EEI);
    Value* vec_nv = m_DeSSA->getNodeValue(vecVal);

    // If EEI has been payload-coalesced or has been vec-aliased,
    // skip it for now (implementation choice).
    // Note that payload-coalescing does not use node value yet.
    if (hasBeenPayloadCoalesced(EEI) ||
        hasAnotherInDCCAsAliasee(vec_nv) ||
        hasAnyOfDCCAsAliaser(EEI_nv)) {
        return;
    }

    // Can only do alias if idx is a known constant.
    Value* IdxVal = EEI->getIndexOperand();
    ConstantInt* Idx = dyn_cast<ConstantInt>(IdxVal);
    if (!Idx) {
        return;
    }

    int iIdx = (int)Idx->getZExtValue();
    if (aliasInterfere(EEI_nv, vec_nv, iIdx)) {
        return;
    }

    // Valid vec alias and add it into alias map
    addVecAlias(EEI_nv, vec_nv, iIdx);

    // Mark this inst as noop inst
    m_HasBecomeNoopInsts[EEI] = 1;
}

void VariableReuseAnalysis::printAlias(raw_ostream& OS, const Function* F) const
{
    // Assign each inst/arg a unique integer so that the output
    // would be in order. It is useful when doing comparison.
    DenseMap<const Value*, int> Val2IntMap;
    int id = 0;
    if (F) {
        // All arguments
        for (auto AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
            const Value* aVal = AI;
            Val2IntMap[aVal] = (++id);
        }
        // All instructions
        for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
            const Instruction* Inst = &*II;
            Val2IntMap[(Value*)Inst] = (++id);
        }
    }

    auto SubVecCmp = [&](const SSubVecDesc* SV0, const SSubVecDesc* SV1) -> bool {
        int n0 = Val2IntMap[SV0->Aliaser];
        int n1 = Val2IntMap[SV1->Aliaser];
        return n0 < n1;
    };

    OS << "\nSummary of Variable Alias Info: "
        << (F ? F->getName().str() : "Function")
        << "\n";

    SmallVector<SSubVecDesc*, 64> sortedAlias;
    for (auto& MI : m_aliasMap) {
        SSubVecDesc* SV = MI.second;
        sortedAlias.push_back(SV);
    }
    std::sort(sortedAlias.begin(), sortedAlias.end(), SubVecCmp);

    for (int i = 0, sz = (int)sortedAlias.size(); i < sz; ++i)
    {
        SSubVecDesc* SV = sortedAlias[i];
        Value* aliasee = SV->BaseVector;
        if (SV->Aliaser != aliasee) {
            // Not alias root
            continue;
        }
        OS << "Aliasee : " << *aliasee << "\n";
        std::sort(SV->Aliasers.begin(), SV->Aliasers.end(), SubVecCmp);
        for (auto VI : SV->Aliasers)
        {
            SSubVecDesc* aSV = VI;
            Value* aliaser = aSV->Aliaser;
            Value* dessaRoot = m_DeSSA ? m_DeSSA->getRootValue(aliaser) : nullptr;
            const char* inCC = dessaRoot ? ".inDessaCC" : "";
            OS << "    " << *aliaser
                << "  [" << aSV->StartElementOffset << "]"
                << inCC << "\n";
        }
        OS << "\n";
    }
    OS << "\n";
}

void VariableReuseAnalysis::dumpAlias() const
{
    printAlias(dbgs(), m_F);
}

// Add alias Aliaser ->Aliasee[Idx]
void VariableReuseAnalysis::addVecAlias(
    Value* Aliaser, Value* Aliasee, int Idx)
{
    SSubVecDesc* aliaserSV = getOrCreateSubVecDesc(Aliaser);
    SSubVecDesc* aliaseeSV = getOrCreateSubVecDesc(Aliasee);
    Value* aliaseeRoot = aliaseeSV->BaseVector;
    aliaserSV->BaseVector = aliaseeRoot;
    aliaserSV->StartElementOffset = Idx + aliaseeSV->StartElementOffset;

    // If Aliaser exists as a root (aliasee), re-alias all its
    // aliasers to the new root 'aliaseeRoot'.
    SSubVecDesc* rootSV = getOrCreateSubVecDesc(aliaseeRoot);
    if (aliaserSV->Aliasers.size() > 0)
    {
        for (int i = 0, sz = (int)aliaserSV->Aliasers.size(); i < sz; ++i)
        {
            SSubVecDesc* SV = aliaserSV->Aliasers[i];
            SV->BaseVector = aliaseeRoot;
            SV->StartElementOffset += Idx;
            rootSV->Aliasers.push_back(SV);
        }

        // Clear aliaser's Aliasers as it is no longer a root
        aliaserSV->Aliasers.clear();
    }

    // Finally, add aliaserSV into root's Aliaser vector and
    // update aliaser to its root map if aliaser's not isolated.
    rootSV->Aliasers.push_back(aliaserSV);

    // aliaser
    Value* rv0 = m_DeSSA ? m_DeSSA->getRootValue(Aliaser) : nullptr;
    if (rv0) {
        m_root2AliasMap[rv0] = Aliaser;
    }
    // aliasee, note that re-defining it does not matter.
    Value* rv1 = m_DeSSA ? m_DeSSA->getRootValue(Aliasee) : nullptr;
    if (rv1) {
        m_root2AliasMap[rv1] = Aliasee;
    }
}

SSubVecDesc* VariableReuseAnalysis::getOrCreateSubVecDesc(Value* V)
{
    if (m_aliasMap.count(V) == 0) {
        SSubVecDesc* SV = new(Allocator) SSubVecDesc(V);
        m_aliasMap.insert(std::make_pair(V, SV));
    }
    return m_aliasMap[V];
}

// Return true if V itself is sub-vector aliased.
// Note that other values in V's DeSSA CC are not checked.
bool VariableReuseAnalysis::isAliased(Value* V) const
{
    Value* V_nv = m_DeSSA ? m_DeSSA->getNodeValue(V) : V;
    return m_aliasMap.count(V_nv) > 0;
}

// DCC: DeSSA Congruent Class
// If any value in V's DCC is aliaser, return true.
bool VariableReuseAnalysis::hasAnyOfDCCAsAliaser(Value* V) const
{
    auto II = m_aliasMap.find(V);
    if (II != m_aliasMap.end()) {
        // If it is in the map, all of its DCC should
        // be either aliaser or aliasee, never have the
        // mix of aliaser and aliasee (implementation
        // must guarantee this).
        SSubVecDesc* SV = II->second;
        return SV->Aliaser != SV->BaseVector;
    }

    // If V is not in the map, check others in its DCC
    Value* rv = m_DeSSA ? m_DeSSA->getRootValue(V) : nullptr;
    if (rv) {
        auto II = m_root2AliasMap.find(rv);
        if (II != m_root2AliasMap.end()) {
            Value* aV = II->second;
            auto MI = m_aliasMap.find(aV);
            IGC_ASSERT(MI != m_aliasMap.end());
            SSubVecDesc* SV = MI->second;
            return (SV->Aliaser != SV->BaseVector);
        }
    }
    return false;
}

// DCC: DeSSA Congruent Class
// If there is another value in V's DCC that is aliasee, return true.
bool VariableReuseAnalysis::hasAnotherInDCCAsAliasee(Value* V) const
{
    // Check if any value of its dessa CC has been aliased already.
    Value* rv = m_DeSSA ? m_DeSSA->getRootValue(V) : nullptr;
    if (rv) {
        auto II = m_root2AliasMap.find(rv);
        if (II != m_root2AliasMap.end()) {
            Value* aV = II->second;
            auto MI = m_aliasMap.find(aV);
            IGC_ASSERT(MI != m_aliasMap.end());
            SSubVecDesc* SV = MI->second;
            const Value* tV = SV->Aliaser;
            return (tV == SV->BaseVector && tV != V);
        }
    }
    return false;
}

// A chain of IEIs is used to define a vector. If all elements of this vector
// are inserted via this chain IEI that has a constant index, populate AllIEIs.
//   input:  FirstIEI (first IEI, usually with index = 0)
//   output: AllIEIs (collect all values used to initialize the vector)
// Return value:
//   true :  if all elements are inserted with IEI of constant index
//   false:  otherwise.
bool VariableReuseAnalysis::getAllInsEltsIfAvailable(
    InsertElementInst* FirstIEI, VecInsEltInfoTy& AllIEIs)
{
    int nelts = getNumElts(FirstIEI);

    // Sanity
    if (nelts < 2)
        return false;

    // AllIEIs are fixed to the number of elements of the vector.
    AllIEIs.resize(nelts);

    InsertElementInst* LastIEI = FirstIEI;
    InsertElementInst* I = FirstIEI;
    Value* dessaRoot = m_DeSSA->getRootValue(FirstIEI);
    while (I)
    {
        LastIEI = I;

        // For insertElement, it should be in the same dessa CC
        // already, as dessa special-handles it. Make sure they
        // are indeed in the same CC, otherwise, skip.
        if (hasBeenPayloadCoalesced(I) ||
            m_DeSSA->getRootValue(I) != dessaRoot)
            return false;

        Value* V = nullptr;
        Value* E = nullptr;
        int IEI_ix = 0, V_ix = 0;
        if (!getElementValue(I, IEI_ix, E, V, V_ix)) {
            return false;
        }

        IGC_ASSERT_MESSAGE(IEI_ix < nelts, "ICE: IEI's index out of bound!");
        SVecInsEltInfo& InsEltInfo = AllIEIs[IEI_ix];
        if (InsEltInfo.IEI) {
            // One element is inserted more than once, skip.
            return false;
        }
        InsEltInfo.IEI = I;
        InsEltInfo.Elt = E;
        InsEltInfo.FromVec = V;
        InsEltInfo.FromVec_eltIx = V_ix;
        if (E) {
            InsEltInfo.EEI = dyn_cast<ExtractElementInst>(E);
        }

        if (!I->hasOneUse()) {
            break;
        }

        I = dyn_cast<InsertElementInst>(I->user_back());
    }

    // Special cases.
    if (AllIEIs.empty() || LastIEI->use_empty()) {
        return false;
    }

    // Make sure all elements are present
    for (int i = 0; i < nelts; ++i) {
        if (AllIEIs[i].IEI == nullptr)
            return false;
    }
    return true;
}

Value* VariableReuseAnalysis::traceAliasValue(Value* V)
{
    if (CastInst * CastI = dyn_cast_or_null<CastInst>(V))
    {
        Value* Src = CastI->getOperand(0);
        if (isa<Constant>(Src))
            return CastI;

        Value* NV0 = m_DeSSA->getNodeValue(CastI);
        Value* NV1 = m_DeSSA->getNodeValue(Src);
        if (NV0 == NV1)
        {
            // Meaning they are aliased already by dessa
            return traceAliasValue(Src);
        }
    }
    return V;
}

//
// Returns true if the following is true
//     IEI = insertElement  <vectorType> Vec,  A,  <constant IEI_ix>
// Return false, otherwise.
//
// When the above condition is true, S, V, V_ix are used for the
// following cases:
//     1. sub-vector (V, V_ix),  S = A
//        A = extractElement <vectorType> V, <constant V_ix>
//        A is the element denoted by (V, V_ix)
//     2. non-sub-vector: V=nullptr, V_ix=0,  S = A
//        A is a candidate inserted and can be alias to Vec
//
//  Input: IEI
//  Output: IEI_ix, S, V, V_ix
bool VariableReuseAnalysis::getElementValue(
    InsertElementInst* IEI, int& IEI_ix, Value*& S, Value*& V, int& V_ix)
{
    // Return value: S or (V, V_ix)
    S = nullptr;
    V = nullptr;
    V_ix = 0;
    IEI_ix = 0;

    // Check if I has constant index, skip if not.
    ConstantInt* CI = dyn_cast<ConstantInt>(IEI->getOperand(2));
    if (!CI) {
        return false;
    }

    // From now on, this func must return true.
    IEI_ix = (int)CI->getZExtValue();

    // Check that the elements inserted are from extractElement.
    // Also, if no ExtractELement, get IEI's element value as S.
    Value* elem0 = IEI->getOperand(1);
    if (hasBeenPayloadCoalesced(elem0) ||
        isa<Constant>(elem0) ||
        isOrCoalescedWithArg(elem0))
    {
        // If elem0 has been payload-coalesced, is constant,
        // or it has been aliased to an argument, it cannot
        // be aliased to IEI.
        return false;
    }

    Value* elem = traceAliasValue(elem0);
    ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(elem);
    S = elem;
    if (!EEI) {
        // case 2. No sub-vector alias, but it is okay
        //         to use non-sub-vector aliasing.
        return true;
    }
    ConstantInt* CI1 = dyn_cast<ConstantInt>(EEI->getIndexOperand());
    if (!CI1 ||
        !m_DeSSA->isSingleValued(elem))
    {
        // case 2
        //   1. EEI's index isn't constant, or
        //   2. EEI is not single-valued (implementation)
        // No sub-vector aliasing, but non-sub-vector aliasing
        // is okay.
        return true;
    }

    V = EEI->getVectorOperand();
    if (isa<Constant>(V) ||
        hasBeenPayloadCoalesced(V))
    {
        // case 2 again, just non-sub-vector aliasing
        V = nullptr;
        return true;
    }

    // case 1.
    V_ix = (int)CI1->getZExtValue();
    return true;
}

void VariableReuseAnalysis::InsertElementAliasing(Function* F)
{
    // Do it if VATemp >= 2 and for ocl only for now
    if (m_pCtx->getVectorCoalescingControl() < 2) {
        return;
    }

    for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II)
    {
        Instruction* I = &*II;
        if (!m_PatternMatch->NeedInstruction(*I))
            continue;

        InsertElementInst* IEI = dyn_cast<InsertElementInst>(I);
        if (!IEI)
            continue;

        // Two cases for sub-vector aliasing:
        //   1. extractFrom: sub-vector is created from a base vector.
        //      For example:
        //         given base: int8 b;  a sub-vector s (int4) can be:
        //         s = (int4)(b.s4, b.s5, b.s6, b.s7)
        //      In this case, 's' becomes a part of 'b'. In LLVM IR,
        //      there are a chain of extElt and insElt instructions for
        //      doing so.
        //   2. insertTo: sub-vector is used to create a base vector.
        //      For example:
        //         given sub-vector int4 s0, s1;  int8 vector b is created like:
        //           b = (int8) (s0, s1)
        //      In this case,  both s0 and s1 become part of b.

        // Start insertElement pattern from the first InsertElement (one
        // with UndefValue. Note that that this's also the dessa insElt root.
        if (!isa<UndefValue>(IEI->getOperand(0)))
            continue;

        // First, collect all insertElementInst and extractElementInst.
        VecInsEltInfoTy AllIEIs;
        if (!getAllInsEltsIfAvailable(IEI, AllIEIs)) {
            continue;
        }

        // Check if this is an extractFrom pattern.
        // If so, add alias and return true.
        if (processExtractFrom(AllIEIs)) {
            continue;
        }

        // Check if this is an insertTo pattern.
        // If so, add alias and return true.
        if (processInsertTo(AllIEIs)) {
            continue;
        }
    }
}

// Check if the vector value of InsertElement is
// a sub-vector of another one, return true if so.
bool VariableReuseAnalysis::processExtractFrom(VecInsEltInfoTy& AllIEIs)
{
    int nelts = (int)AllIEIs.size();
    Value* BaseVec = AllIEIs[0].FromVec;
    int BaseStartIx = AllIEIs[0].FromVec_eltIx;
    if (!BaseVec) {
        // Base is not a vector, so IEI cannot be
        // a subvector of another vector!
        return false;
    }
    int base_nelts = getNumElts(BaseVec);

    // If Base's size is smaller than IEI's, IEI cannot be sub-vector
    if (base_nelts < nelts) {
        return false;
    }

    for (int i = 1; i < nelts; ++i)
    {
        if (AllIEIs[i].FromVec != BaseVec ||
            AllIEIs[i].FromVec_eltIx != (BaseStartIx + i))
            return false;
    }

    // Interference checking
    Value* Sub = AllIEIs[0].IEI;
    Value* Sub_nv = m_DeSSA->getNodeValue(Sub);
    Value* Base_nv = m_DeSSA->getNodeValue(BaseVec);

    // If Sub is an arg of function, skip (Base is okay to be an arg)
    if (isOrCoalescedWithArg(Sub)) {
        return false;
    }

    // Implementation restriction
    if (hasAnyOfDCCAsAliaser(Sub_nv) ||
        hasAnotherInDCCAsAliasee(Base_nv)) {
        return false;
    }

    if (aliasInterfere(Sub_nv, Base_nv, BaseStartIx)) {
        return false;
    }

    // add alias
    addVecAlias(Sub_nv, Base_nv, BaseStartIx);

    // Make sure noop insts are in the map.
    for (int i = 0, sz = (int)AllIEIs.size(); i < sz; ++i)
    {
        InsertElementInst* IEI = AllIEIs[i].IEI;
        if (m_DeSSA->isNoopAliaser(IEI))
            continue;
        m_HasBecomeNoopInsts[IEI] = 1;

        ExtractElementInst* EEI = AllIEIs[i].EEI;
        IGC_ASSERT(EEI);
        if (m_DeSSA->isNoopAliaser(EEI))
            continue;
        m_HasBecomeNoopInsts[EEI] = 1;
    }
    return true;
}

// Check if IEI is a base vector created by other sub-vectors
// or scalars. If it is, create alias and return true.
bool VariableReuseAnalysis::processInsertTo(VecInsEltInfoTy& AllIEIs)
{
    int nelts = (int)AllIEIs.size();
    Value* Sub = AllIEIs[0].FromVec;
    int SubStartIx = 0;
    SmallVector<std::pair<Value*, int>, 8> SubVecs;

    auto IsInSubVecs = [&](Value* Val) -> bool {
        for (int j = 0, sz = (int)SubVecs.size(); j < sz; ++j) {
            if (SubVecs[j].first == Val)
                return true;
        }
        return false;
    };

    // Check alias interference
    InsertElementInst* FirstIEI = AllIEIs[0].IEI;
    Value* Base_nv = m_DeSSA->getNodeValue(FirstIEI);
    // Early check to see if Base_nv could be used as Base.
    if (hasAnotherInDCCAsAliasee(Base_nv)) {
        return false;
    }

    bool isSubCandidate = true;
    for (int i = 0; i < nelts; ++i)
    {
        // On entry to the iteration, AllIEIs[i].FromVec must be the
        // same as Sub.  If the next Sub is different from the current
        // one, the current element (AllIEIs[i]) is the last one element
        // for the Sub.
        //
        // Note
        //   case 1:  if Elt == nullptr, no aliasing
        //   case 2:  if Elt != nullptr && Fromvec == nullptr, Elt aliasing
        //   case 3:  if Elt != nullptr && FromVec != nullptr,
        //            (FromVec, FromVec_eltIx) sub-vector aliasing
        //
        Value* Elt = AllIEIs[i].Elt;
        if (!Elt ||
            (Sub && (i - SubStartIx) != AllIEIs[i].FromVec_eltIx)) {
            isSubCandidate = false;
            continue;
        }

        // If Sub == nullptr or NextSub != Sub, this is the last element
        // of the current Sub (it is a scalar in case of sub == nullpr).
        Value* NextSub = (i < (nelts - 1)) ? AllIEIs[i + 1].FromVec : nullptr;
        if (!Sub || Sub != NextSub)
        {
            // End of the current Sub
            if (isSubCandidate)
            {
                Value* aliaser = Sub ? Sub : Elt;
                int sub_nelts = getNumElts(aliaser);
                // If Sub's size is not smaller than IEI's, or not all sub's
                // elements are used, skip.
                if (sub_nelts < nelts && (i - SubStartIx) == (sub_nelts - 1))
                {
                    SubVecs.push_back(std::make_pair(aliaser, SubStartIx));
                }
            }

            // NextSub should be the new sub-vector.
            // Make sure it is not used yet.
            // Note this works for special case in which NextSub = nullptr.
            isSubCandidate = true;
            Value* NextElt = (i < (nelts - 1)) ? AllIEIs[i + 1].Elt : nullptr;
            if (!NextElt ||
                (NextSub && IsInSubVecs(NextSub)) ||
                (!NextSub && IsInSubVecs(NextElt))) {
                isSubCandidate = false;
            }
            Sub = NextSub;
            SubStartIx = i + 1;
        }
    }

    // Check alias interference
    bool hasAlias = false;
    for (int i = 0, sz = (int)SubVecs.size(); i < sz; ++i)
    {
        std::pair<Value*, int>& aPair = SubVecs[i];
        Value* V = aPair.first;

        // If V is an arg, skip it
        if (isOrCoalescedWithArg(V)) {
            continue;
        }

        int V_ix = aPair.second;
        Value* V_nv = m_DeSSA->getNodeValue(V);
        if (hasAnyOfDCCAsAliaser(V_nv)) {
            continue;
        }
        if (aliasInterfere(V_nv, Base_nv, V_ix)) {
            continue;
        }
        addVecAlias(V_nv, Base_nv, V_ix);

        int V_sz = getNumElts(V);
        if (V_sz > 1)
        {
            // set up Noop inst
            // Make sure noop insts are in the map.
            for (int j = V_ix, sz = V_ix + V_sz; j < sz; ++j)
            {
                InsertElementInst* IEI = AllIEIs[j].IEI;
                if (m_DeSSA->isNoopAliaser(IEI))
                    continue;
                m_HasBecomeNoopInsts[IEI] = 1;

                ExtractElementInst* EEI = AllIEIs[j].EEI;
                IGC_ASSERT(EEI);
                // Sub-vector
                if (m_DeSSA->isNoopAliaser(EEI))
                    continue;
                m_HasBecomeNoopInsts[EEI] = 1;

                Value* EEI_nv = m_DeSSA->getNodeValue(EEI);
                addVecAlias(EEI_nv, Base_nv, j);
            }
        }
        hasAlias = true;
    }
    return hasAlias;
}

// Return all aliased values of VecAliasee, given the alias:
//           Aliaser->(VecAliasee, Idx)
void VariableReuseAnalysis::getAllAliasVals(
    ValueVectorTy& AliasVals,
    Value* Aliaser,
    Value* VecAliasee,
    int    Idx)
{
    AliasVals.clear();
    auto II = m_aliasMap.find(VecAliasee);
    AliasVals.push_back(VecAliasee);
    if (II != m_aliasMap.end())
    {
        SSubVecDesc* aliaseeSV = II->second;
        int nelts = getNumElts(Aliaser);
        int Idx_end = Idx + nelts - 1;
        for (int i = 0, sz = (int)(aliaseeSV->Aliasers.size()); i < sz; ++i)
        {
            SSubVecDesc* SV = aliaseeSV->Aliasers[i];
            int start = SV->StartElementOffset;
            int end = start + SV->NumElts - 1;
            if ((start > Idx_end) || (end < Idx))
                continue;
            AliasVals.push_back(SV->Aliaser);
        }
    }
}


// Check if two potentially-aliased values (must be dessa node
// values) interfere each other.
bool VariableReuseAnalysis::aliasInterfere(Value* Sub, Value* Base, int BaseIdx)
{
    ValueVectorTy Vec0, Vec1;
    Vec0.push_back(Sub);
    getAllAliasVals(Vec1, Sub, Base, BaseIdx);
    auto II0 = m_aliasMap.find(Sub);
    if (II0 != m_aliasMap.end()) {
        SSubVecDesc* SV0 = II0->second;
        for (int i = 0, sz = (int)SV0->Aliasers.size(); i < sz; ++i) {
            SSubVecDesc* tSV = SV0->Aliasers[i];
            Vec0.push_back(tSV->Aliaser);
        }
    }

    for (int i0 = 0, sz0 = (int)Vec0.size(); i0 < sz0; ++i0)
    {
        Value* V0 = Vec0[i0];
        for (int i1 = 0, sz1 = (int)Vec1.size(); i1 < sz1; ++i1) {
            Value* V1 = Vec1[i1];
            if (m_DeSSA->aliasInterfere(V0, V1))
                return true;
        }
    }
    return false;
}

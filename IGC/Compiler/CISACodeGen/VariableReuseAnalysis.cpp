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
using namespace IGC::IGCMD;

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

    e_alignment getMinAlignment(Value* V, WIAnalysis* WIA, CodeGenContext* pContext)
    {
        auto grfAlignment = [pContext]() {
            return pContext->platform.getGRFSize() == 64
                ? EALIGN_32WORD : EALIGN_HWORD;
        };

        auto getSendPayloadAlignment = [pContext](const bool Is64BitTy) {
            if (pContext->platform.getGRFSize() == 64 /*bytes*/)
                return Is64BitTy ? EALIGN_64WORD : EALIGN_32WORD;
            return Is64BitTy ? EALIGN_32WORD : EALIGN_HWORD;
        };

        //Type* eltTy = V->getType()->getScalarType();
        bool is64BitTy = false; // (eltTy->getPrimitiveSizeInBits() > 32);

        // GRF-aligned for send operands
        // check if V is defined by send
        if (GenIntrinsicInst* CI = dyn_cast<GenIntrinsicInst>(V))
        {
            switch (CI->getIntrinsicID()) {
            case GenISAIntrinsic::GenISA_sub_group_dpas:
                return grfAlignment();
            case GenISAIntrinsic::GenISA_simdBlockRead:
            case GenISAIntrinsic::GenISA_LSC2DBlockRead:
                return getSendPayloadAlignment(is64BitTy);
            default:
                break;
            }
        }

        // Check if V is used in send
        bool isSend = false;
        for (auto UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
            User* U = *UI;
            if (isa<LoadInst>(U) || isa<StoreInst>(U))
                isSend = true;
            if (GenIntrinsicInst* CI = dyn_cast<GenIntrinsicInst>(V))
            {
                switch (CI->getIntrinsicID()) {
                    // GetPreferredAlilgnment() will handle dpas
                    //case GenISAIntrinsic::GenISA_sub_group_dpas:
                case GenISAIntrinsic::GenISA_simdBlockWrite:
                case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
                    isSend = true;
                    break;
                default:
                    break;
                }
            }
        }
        if (isSend)
            return getSendPayloadAlignment(is64BitTy);
        return GetPreferredAlignment(V, WIA, pContext);
    }
}

char VariableReuseAnalysis::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(VariableReuseAnalysis, "VariableReuseAnalysis",
    "VariableReuseAnalysis", false, true)
    // IGC_INITIALIZE_PASS_DEPENDENCY(RegisterEstimator)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
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
    m_IsBlockPressureLow(Status::Undef),
    m_BBSizeThreshold(IGC_GET_FLAG_VALUE(ScalarAliasBBSizeThreshold)) {
    initializeVariableReuseAnalysisPass(*PassRegistry::getPassRegistry());
}

bool VariableReuseAnalysis::runOnFunction(Function& F)
{
    m_F = &F;

    m_WIA = &(getAnalysis<WIAnalysis>());
    if (IGC_IS_FLAG_ENABLED(EnableDeSSA))
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

        sortAliasResult();

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
    // If dessa is disabled (LV = null), skip.
    if (UseInst->isUsedOutsideOfBlock(CurBB) || LV == nullptr)
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
    // If dessa is disabled (LV = null), skip
    if (isa<PHINode>(DefInst) || LV == nullptr)
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
    const auto control = ((m_pCtx->getVectorCoalescingControl() >> 2) & 0x3);
    if (control == 0) {
        return;
    }

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
    const auto control = ((m_pCtx->getVectorCoalescingControl() >> 4) & 0x3);
    if (!m_DeSSA || control == 0)
        return;

    // VectorAlias = 0x10
    DenseMap<Value*, int> dessaRootVisited;
    auto IS = m_baseVecMap.begin();
    auto IE = m_baseVecMap.end();
    for (auto II = IS; II != IE; ++II)
    {
        SBaseVecDesc* BV = II->second;
        Value* aliasee = BV->BaseVector;
        // An alias set of an aliasee:
        //     The aliasee and all its aliasers; and for each of them, all values
        //     in their dessa CC.
        //
        // For each Aliasee, record its lifetime start, which is the
        // nearest dominator that dominates all value defs in an alias set.
        // This BB is either one that has no defintion of values in the set;
        // or one that has a defintion to a value in the set. For the former,
        // m_LifetimeAtEndOfBB is used to keep track of it; for the latter,
        // m_LifetimeAt1stDefOfBB is used.
        ValueVectorTy AllVals;
        SmallVector<Value*, 16> valInCC;
        m_DeSSA->getAllValuesInCongruentClass(aliasee, valInCC);
        AllVals.insert(AllVals.end(), valInCC.begin(), valInCC.end());

        // update visited for aliasee
        Value* aliaseeRoot = getRootValue(aliasee);
        dessaRootVisited[aliaseeRoot] = 1;
        for (int i = 0, sz = (int)BV->Aliasers.size(); i < sz; ++i)
        {
            SSubVecDesc* aSV = BV->Aliasers[i];
            Value* aliaser = aSV->Aliaser;
            valInCC.clear();
            m_DeSSA->getAllValuesInCongruentClass(aliaser, valInCC);
            AllVals.insert(AllVals.end(), valInCC.begin(), valInCC.end());

            // update visited for aliaser
            Value* aRoot = getRootValue(aliaser);
            dessaRootVisited[aRoot] = 1;
        }

        setLifeTimeStartPos(aliaseeRoot, AllVals, theBC);
    }

    // VectorAlias = 0x20, for other vector values.
    if (control < 2)
        return;

    for (auto II = inst_begin(*m_F), IE = inst_end(*m_F); II != IE; ++II)
    {
        Instruction* I = &*II;
        if (!m_PatternMatch->NeedInstruction(*I))
            continue;
        if (!I->getType()->isVectorTy())
            continue;
        Value* I_nd = m_DeSSA->getNodeValue(I);
        Value* rootV = getRootValue(I_nd);
        if (dessaRootVisited.find(rootV) != dessaRootVisited.end()) {
            // Already handled by sub-vector aliasing, skip
            continue;
        }
        dessaRootVisited[rootV] = 1;

        ValueVectorTy AllVals;
        SmallVector<Value*, 16> valInCC;
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
    auto II0 = m_baseVecMap.find(V_nv);
    if (II0 != m_baseVecMap.end())
        return II0->second->BaseVector;

    auto II1 = m_aliasMap.find(V_nv);
    if (II1 == m_aliasMap.end()) {
        return V_nv;
    }
    return II1->second->Aliasee->BaseVector;
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
    const auto control = ((m_pCtx->getVectorCoalescingControl() >> 2) & 0x3);
    // VectorAlias=0x4 : for isolated values
    //            =0x8 : for both isolated and non-isolated values
    if (control == 0) {
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
    // If EEI has been payload-coalesced or has been an aliaser, skip
    if (hasBeenPayloadCoalesced(EEI) || isAliaser(EEI_nv)) {
        return;
    }

    if (!m_DeSSA->isSingleValued(EEI_nv) || !m_DeSSA->isSingleValued(vec_nv)) {
        if (control < 2) {
            return;
        }

        if (hasAnyDCCAsAliaser(EEI_nv) || hasAnotherDCCAsAliasee(vec_nv)) {
            return;
        }
    }

    // Can only do alias if idx is a known constant.
    Value* IdxVal = EEI->getIndexOperand();
    ConstantInt* Idx = dyn_cast<ConstantInt>(IdxVal);
    if (!Idx) {
        return;
    }

    int iIdx = (int)Idx->getZExtValue();
    if (!m_DeSSA->isSingleValued(EEI_nv) || !m_DeSSA->isSingleValued(vec_nv)) {
        if (control < 2)
            return;

        // case 3: DeSSA CC not empty
        if (aliasInterfere(EEI_nv, vec_nv, iIdx))
            return;
    }

    // Valid vec alias and add it into alias map
    addVecAlias(EEI_nv, vec_nv, vecVal, iIdx);

    // Mark this inst as noop inst
    m_HasBecomeNoopInsts[EEI] = 1;
}

void VariableReuseAnalysis::printAlias(raw_ostream& OS, const Function* F) const
{
    auto toString = [](e_alignment A) -> const char* {
        switch (A) {
        case EALIGN_BYTE: return "Byte";
        case EALIGN_WORD: return "word";
        case EALIGN_DWORD: return "dword";
        case EALIGN_QWORD: return "qword";
        case EALIGN_OWORD: return "oword";
        case EALIGN_HWORD: return "hword";
        case EALIGN_32WORD: return "32word";
        case EALIGN_64WORD: return "64word";
        default: break;
        }
        return "auto";
    };

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

    auto SubVecCmp = [&](const SSubVecDesc* SV0, const SSubVecDesc* SV1) {
        int n0 = Val2IntMap[SV0->Aliaser];
        int n1 = Val2IntMap[SV1->Aliaser];
        return n0 < n1;
    };

    auto BaseVecCmp = [&](const SBaseVecDesc* BV0, const SBaseVecDesc* BV1) {
        int n0 = Val2IntMap[BV0->BaseVector];
        int n1 = Val2IntMap[BV1->BaseVector];
        return n0 < n1;
    };

    OS << "\nSummary of Variable Alias Info: "
        << (F ? F->getName().str() : "Function")
        << "\n";

    SmallVector<SBaseVecDesc*, 64> sortedAlias;
    for (auto& MI : m_baseVecMap) {
        SBaseVecDesc* BV = MI.second;
        sortedAlias.push_back(BV);
    }
    std::sort(sortedAlias.begin(), sortedAlias.end(), BaseVecCmp);

    for (int i = 0, sz = (int)sortedAlias.size(); i < sz; ++i)
    {
        SBaseVecDesc* BV = sortedAlias[i];
        Value* aliasee = BV->BaseVector;

        OS << "Aliasee : " << *aliasee << "  align: "
           << toString(BV->Align) << "\n";
        std::sort(BV->Aliasers.begin(), BV->Aliasers.end(), SubVecCmp);
        for (auto VI : BV->Aliasers)
        {
            SSubVecDesc* aSV = VI;
            Value* aliaser = aSV->Aliaser;
            bool isSinglVal = m_DeSSA ? m_DeSSA->isSingleValued(aliaser) : true;
            const char* inCC = !isSinglVal ? ".inDessaCC" : "";
            OS << "    " << *aliaser
                << "  [" << aSV->StartElementOffset << "]"
                << inCC << "\n";
        }
        OS << "\n";
    }
    OS << "\n";
}

// Sort the final aliase info (baseVecmap) so that its order is deterministic.
// CreateAliasVars() relies on this to generate cvariables in order.
// (todo: use vector instead of map in the algorithm to avoid sorting.)
void VariableReuseAnalysis::sortAliasResult()
{
    if (m_baseVecMap.empty()) {
        return;
    }

    Function* F = m_F;
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

    auto SubVecCmp = [&](const SSubVecDesc* SV0, const SSubVecDesc* SV1) {
        int n0 = Val2IntMap[SV0->Aliaser];
        int n1 = Val2IntMap[SV1->Aliaser];
        return n0 < n1;
        };

    auto BaseVecCmp = [&](const SBaseVecDesc* BV0, const SBaseVecDesc* BV1) {
        int n0 = Val2IntMap[BV0->BaseVector];
        int n1 = Val2IntMap[BV1->BaseVector];
        return n0 < n1;
        };


    m_sortedBaseVec.clear();
    for (auto& MI : m_baseVecMap) {
        SBaseVecDesc* BV = MI.second;
        std::sort(BV->Aliasers.begin(), BV->Aliasers.end(), SubVecCmp);
        m_sortedBaseVec.push_back(BV);
    }
    std::sort(m_sortedBaseVec.begin(), m_sortedBaseVec.end(), BaseVecCmp);
}

void VariableReuseAnalysis::dumpAlias() const
{
    printAlias(dbgs(), m_F);
}

// Add alias Aliaser -> Aliasee[Idx]
void VariableReuseAnalysis::addVecAlias(
    Value* Aliaser, Value* Aliasee, Value* OrigBaseVec,
    int Idx, e_alignment AliaseeAlign)
{
    auto getLargerAlign = [](e_alignment A0, e_alignment A1) -> e_alignment {
        if (A0 == EALIGN_AUTO)
            return A1;
        if (A1 == EALIGN_AUTO)
            return A0;
        return A0 > A1 ? A0 : A1;
    };

    int StartIx = Idx;
    // if Aliasee is an aliaser now, get its aliasee and it will be the new aliasee
    SBaseVecDesc* aliaseeBV;
    auto SMI = m_aliasMap.find(Aliasee);
    if (SMI != m_aliasMap.end()) {
        SSubVecDesc* SV = SMI->second;
        aliaseeBV = SV->Aliasee;
        StartIx += SV->StartElementOffset;
    }
    else {
        aliaseeBV = getOrCreateBaseVecDesc(Aliasee, OrigBaseVec, AliaseeAlign);
    }
    // update align
    aliaseeBV->Align = getLargerAlign(aliaseeBV->Align, AliaseeAlign);

    SSubVecDesc* aliaserSV = getOrCreateSubVecDesc(Aliaser);
    aliaserSV->Aliasee = aliaseeBV;
    aliaserSV->StartElementOffset = StartIx;

    // If Aliaser exists as aliasee, must re-alias its aliasers.
    auto BMI = m_baseVecMap.find(Aliaser);
    if (BMI != m_baseVecMap.end()) {
        SBaseVecDesc* BVD = BMI->second;
        for (int i = 0, sz = (int)BVD->Aliasers.size(); i < sz; ++i)
        {
            SSubVecDesc* SV = BVD->Aliasers[i];
            SV->Aliasee = aliaseeBV;
            SV->StartElementOffset += StartIx;
            aliaseeBV->Aliasers.push_back(SV);
        }

        aliaseeBV->Align = getLargerAlign(aliaseeBV->Align, BVD->Align);

        // Delete BMI as it is no longer a base vector.
        m_baseVecMap.erase(BMI);
    }

    // Finally, add aliaserSV into Aliasee's Aliaser vector
    aliaseeBV->Aliasers.push_back(aliaserSV);

    const auto control1 = (m_pCtx->getVectorCoalescingControl() & 0x3);
    const auto control2 = ((m_pCtx->getVectorCoalescingControl() >> 2) & 0x3);
    if (control1 > 1 || control2 > 1)
    {
        // If aliaser isn't single-valued, add it to its root map.
        if (!m_DeSSA->isSingleValued(Aliaser)) {
            Value* rv0 = m_DeSSA->getRootValue(Aliaser);
            m_root2AliasMap[rv0] = Aliaser;
        }
        if (!m_DeSSA->isSingleValued(Aliasee)) {
            // If it isn't isolated, add it to its root map
            Value* rv1 = m_DeSSA->getRootValue(Aliasee);
            m_root2AliasMap[rv1] = Aliasee;
        }
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

SBaseVecDesc* VariableReuseAnalysis::getOrCreateBaseVecDesc(Value* V,
    Value* OV, e_alignment A)
{
    if (m_baseVecMap.count(V) == 0) {
        SBaseVecDesc* BV = new(Allocator) SBaseVecDesc(V, OV, A);
        m_baseVecMap.insert(std::make_pair(V, BV));
    }
    return m_baseVecMap[V];
}

// Return true if V itself is sub-vector aliased.
// Note that other values in V's DeSSA CC are not checked.
bool VariableReuseAnalysis::isAliased(Value* V) const
{
    Value* V_nv = m_DeSSA ? m_DeSSA->getNodeValue(V) : V;
    return (m_aliasMap.count(V_nv) > 0 || m_baseVecMap.count(V_nv) > 0);
}

// Return true if V is aliased to a vector as an aliaser
bool VariableReuseAnalysis::isAliaser(Value* V) const
{
    Value* V_nv = m_DeSSA ? m_DeSSA->getNodeValue(V) : V;
    return m_aliasMap.count(V_nv) > 0;
}

// DCC: DeSSA Congruent Class
// If V has been coalesced by DeSSA and any value in V's DCC has been aliased
// as an aliaser, return true.
bool VariableReuseAnalysis::hasAnyDCCAsAliaser(Value* V) const
{
    // If V is not in the map, check others in its DCC
    Value* rv = m_DeSSA ? m_DeSSA->getRootValue(V) : nullptr;
    if (rv) {
        auto II = m_root2AliasMap.find(rv);
        if (II != m_root2AliasMap.end()) {
            Value* aV = II->second;
            auto MI = m_aliasMap.find(aV);
            if (MI != m_aliasMap.end())
                return true;
        }
    }
    return false;
}

// DCC: DeSSA Congruent Class
// If there is another value (different from V) in V's DCC that is aliasee,
// return true.
bool VariableReuseAnalysis::hasAnotherDCCAsAliasee(Value* V) const
{
    // Check if any value of its dessa CC has been an aliasee.
    Value* rv = m_DeSSA ? m_DeSSA->getRootValue(V) : nullptr;
    if (rv) {
        auto II = m_root2AliasMap.find(rv);
        if (II != m_root2AliasMap.end()) {
            Value* aV = II->second;
            auto MI = m_baseVecMap.find(aV);
            if (MI != m_baseVecMap.end() && aV != V)
                return true;
        }
    }
    return false;
}

// A chain of IEIs is used to define a vector. If all elements of this vector
// are inserted via this chain IEIs with constant indices, populate AllIEIs.
//   input:  FirstIEI (first IEI, usually with index = 0)
//   output: AllIEIs (collect all values used to initialize the vector)
// Return value:
//   true :  if all elements are inserted with IEI of constant index
//   false:  otherwise.
bool VariableReuseAnalysis::getAllInsEltsIfAvailable(
    InsertElementInst* FirstIEI, VecInsEltInfoTy& AllIEIs, bool OnlySameBB)
{
    int nelts = getNumElts(FirstIEI);

    // Sanity
    if (nelts < 2)
        return false;

    AllIEIs.resize(nelts);

    InsertElementInst* LastIEI = FirstIEI;
    InsertElementInst* I = FirstIEI;
    Value* dessaRoot = m_DeSSA->getRootValue(FirstIEI);
    while (I)
    {
        LastIEI = I;

        if (OnlySameBB && LastIEI->getParent() != FirstIEI->getParent())
            return false;

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

    // Make sure all elements are present, and they should have same uniform.
    Value* V = AllIEIs[0].IEI;
    if (V == nullptr) {
        return false;
    }
    Value* V_nv = m_DeSSA->getNodeValue(V);
    Value* V_root = getRootValue(V_nv);
    auto V_dep = m_WIA->whichDepend(V);
    for (int i = 0; i < nelts; ++i) {
        Value* tV = AllIEIs[i].IEI;
        if (tV == nullptr)
            return false;

        // Expect node values for all IEIs are identical. In general, if they
        // are in the same DeSSA CC, that would be fine.
        Value* tV_nv = m_DeSSA->getNodeValue(tV);
        if (V_root != getRootValue(tV_nv))
            return false;

        Value* E = AllIEIs[i].Elt;
        Value* FromVec = AllIEIs[i].FromVec;
        Value* FromVec_nv = m_DeSSA->getNodeValue(FromVec);
        // check if FromVec has been coalesced with IEI already by DeSSA.
        // (Wouldn't happen under current DeSSA, but might happen in future)
        if (V_root == getRootValue(FromVec_nv))
            return false;

        // Make sure FromVec or E have the same uniformness as V.
        if ((E && V_dep != m_WIA->whichDepend(E)) ||
            (FromVec && V_dep != m_WIA->whichDepend(FromVec)))
            return false;
    }
    return true;
}

Value* VariableReuseAnalysis::traceAliasValue(Value* V)
{
    if (CastInst * CastI = dyn_cast_or_null<CastInst>(V))
    {
        // Only handle Noop cast inst. For example,
        //    dst = bitcast <3 x i32> src to <3 x float>,
        // it is okay, but the following isn't.
        //    dst = bitcast <3 x i64> src to <6 x i32>
        if (!isNoOpInst(CastI, m_pCtx)) {
            return V;
        }

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
//     IEI = insertElement  <vectorType> Vec,  S,  <constant IEI_ix>
// Return false, otherwise.
//
// When the above condition is true, V and V_ix are used for the
// following cases:
//     1. S is from another vector V.
//        S = extractElement <vectorType> V, <constant V_ix>
//        S is the element denoted by (V, V_ix)
//     2. otherwise, V=nullptr, V_ix=0.
//        S is a candidate inserted and could be alias to the vector.
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

    IEI_ix = (int)CI->getZExtValue();

    Value* elem0 = IEI->getOperand(1);
    if (hasBeenPayloadCoalesced(elem0) ||
        isa<Constant>(elem0) ||
        isOrCoalescedWithArg(elem0))
    {
        // If elem0 has been payload-coalesced, is constant,
        // or it has been aliased to an argument, skip it.
        return false;
    }

    Value* elem = traceAliasValue(elem0);
    ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(elem);
    S = elem;
    if (!EEI) {
        // case 2.
        return true;
    }
    ConstantInt* CI1 = dyn_cast<ConstantInt>(EEI->getIndexOperand());
    if (!CI1 ||
        !m_DeSSA->isSingleValued(elem))
    {
        // case 2
        return true;
    }

    V = EEI->getVectorOperand();
    if (isa<Constant>(V) ||
        hasBeenPayloadCoalesced(V))
    {
        // case 2 again
        V = nullptr;
        return true;
    }

    // case 1.
    V_ix = (int)CI1->getZExtValue();
    return true;
}

void VariableReuseAnalysis::InsertElementAliasing(Function* F)
{
    // There are dead blocks that are still not removed, don't count them
    // Should use F->size() once dead BBs are removed
    auto getNumBBs = [](Function* aF) {
        int32_t i = 1;  // count entry
        for (BasicBlock &aBB  : *aF) {
            if (aBB.hasNPredecessors(0)) {
                continue;
            }
            ++i;
        }
        return i;
    };

    // Do it if VectorAlias != 0.
    // VectorAlias=0x1: subvec aliasing for isolated values (getRootValue()=null)
    //            =0x2: subvec aliasing for both isolated and non-isolated value)
    const auto control = (m_pCtx->getVectorCoalescingControl() & 0x3);
    // To avoid increasing GRF pressure, skip if F is too large or not an entry
    const int32_t NumBBThreshold = IGC_GET_FLAG_VALUE(VectorAliasBBThreshold);
    bool OnlySameBB = getNumBBs(F) > NumBBThreshold;
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (control == 0 || !isEntryFunc(pMdUtils, F)) {
        return;
    }
    for (auto BI = F->begin(), BE = F->end(); BI != BE; ++BI)
    {
        BasicBlock* BB = &*BI;
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
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
            // with UndefValue. Note that this's also the dessa insElt root.
            if (!isa<UndefValue>(IEI->getOperand(0)))
                continue;

            // First, collect all insertElementInst and extractElementInst.
            VecInsEltInfoTy AllIEIs;
            if (!getAllInsEltsIfAvailable(IEI, AllIEIs, OnlySameBB)) {
                continue;
            }

            // Check if this is an extractFrom pattern, if so, add alias.
            if (processExtractFrom(AllIEIs)) {
                continue;
            }

            // Check if this is an insertTo pattern, if so add alias.
            if (processInsertTo(BB, AllIEIs)) {
                continue;
            }
        }
    }
}

// Return true if vector formed by IEI chain is a sub-vector of another one.
bool VariableReuseAnalysis::processExtractFrom(VecInsEltInfoTy& AllIEIs)
{
    const int nelts = (int)AllIEIs.size();
    Value* BaseVec = AllIEIs[0].FromVec;
    int BaseStartIx = AllIEIs[0].FromVec_eltIx;
    if (!BaseVec) {
        return false;
    }
    int base_nelts = getNumElts(BaseVec);

    if (base_nelts < nelts) {
        return false;
    }

    for (int i = 1; i < nelts; ++i)
    {
        if (AllIEIs[i].FromVec != BaseVec ||
            AllIEIs[i].FromVec_eltIx != (BaseStartIx + i))
            return false;
    }

    // DPAS unlikely uses smaller vector, favor extractMask
    if (base_nelts <= 4 && isExtractMaskCandidate(BaseVec)) {
        return false;
    }

    Value* lastIEI = AllIEIs[nelts - 1].IEI;
    auto S_use = getCandidateStateUse(lastIEI);
    auto B_def = getCandidateStateDef(BaseVec);
    auto B_use = getCandidateStateUse(BaseVec);
    if (!aliasOkay(S_use, B_def, B_use))
        return false;

    Value* Sub = AllIEIs[0].IEI;
    // If Sub is coalesced with an arg of function, skip.
    if (isOrCoalescedWithArg(Sub)) {
        return false;
    }

    Value* Sub_nv = m_DeSSA->getNodeValue(Sub);
    Value* Base_nv = m_DeSSA->getNodeValue(BaseVec);

    // If Sub_nv has been aliased to another vector already, skip
    if (isAliaser(Sub_nv)) {
        return false;
    }

    e_alignment BaseAlign;
    if (!checkSubAlign(BaseAlign, Sub, BaseVec, BaseStartIx)) {
        return false;
    }

    // Skip if they are not singled valued
    bool isSub_singleVal = m_DeSSA->isSingleValued(Sub_nv);
    bool isBase_singleVal = m_DeSSA->isSingleValued(Base_nv);
    if (!isSub_singleVal || !isBase_singleVal) {
        if ((m_pCtx->getVectorCoalescingControl() & 0x3) < 2)
            return false;

        // Skip if they are already coalesced by DeSSA
        Value* rootBase_nv = m_DeSSA->getRootValue(Base_nv);
        if (rootBase_nv && rootBase_nv == m_DeSSA->getRootValue(Sub_nv))
            return false;

        // Skip
        //   1) if Sub_nv has been vector-aliased to another one already; or
        //   2) if a different one from Base_nv's CC (not Base_nv) has been
        //      aliased by anotehr one already
        // Both cases need a complicated interference checking.
        if (hasAnyDCCAsAliaser(Sub_nv) ||
            hasAnotherDCCAsAliasee(Base_nv)) {
            return false;
        }

        if (aliasInterfere(Sub_nv, Base_nv, BaseStartIx)) {
            return false;
        }
    }

    // add alias
    addVecAlias(Sub_nv, Base_nv, BaseVec, BaseStartIx, BaseAlign);

    // Make sure noop insts are in the map.
    for (int i = 0, sz = nelts; i < sz; ++i)
    {
        // IEI chain is coalesced by DeSSA, so it's safe to mark it as noop
        InsertElementInst* IEI = AllIEIs[i].IEI;
        if (!m_DeSSA->isNoopAliaser(IEI)) {
          m_HasBecomeNoopInsts[IEI] = 1;
        }

        ExtractElementInst* EEI = AllIEIs[i].EEI;
        IGC_ASSERT(EEI);
        if (!m_DeSSA->isNoopAliaser(EEI)) {
          // Set EEI as an aliser, thus it become noop.
          Value *EEI_nv = m_DeSSA->getNodeValue(EEI);
          addVecAlias(EEI_nv, Base_nv, BaseVec, AllIEIs[i].FromVec_eltIx, EALIGN_AUTO);
          m_HasBecomeNoopInsts[EEI] = 1;
        }
    }
    return true;
}

// Check if IEI is a base vector created by other sub-vectors
// or scalars. If it is, create alias and return true.
bool VariableReuseAnalysis::processInsertTo(BasicBlock* BB, VecInsEltInfoTy& AllIEIs)
{
    const auto control = (m_pCtx->getVectorCoalescingControl() & 0x3);
    SmallVector<std::pair<Value*, int>, 8> SubVecs;
    auto IsInSubVecs = [&SubVecs](Value* Val) {
        for (int j = 0, sz = (int)SubVecs.size(); j < sz; ++j) {
            if (SubVecs[j].first == Val)
                return true;
        }
        return false;
    };

    InsertElementInst* FirstIEI = AllIEIs[0].IEI;
    Value* Base_nv = m_DeSSA->getNodeValue(FirstIEI);
    if (!m_DeSSA->isSingleValued(Base_nv)) {
        if (control < 2)
            return false;

        if (hasAnotherDCCAsAliasee(Base_nv)) {
            return false;
        }
    }

    // Find all subvec that are part of BaseVec. AllIEIs may be formed from
    // multiple subvec and scalar.
    bool isSubCandidate = true;
    int nelts = (int)AllIEIs.size();
    Value* Sub = AllIEIs[0].FromVec;
    int SubStartIx = 0;
    for (int i = 0; i < nelts; ++i)
    {
        // On entry to the iteration, AllIEIs[i].FromVec must be the same as
        // Sub. If the next Sub is different from the current one, the current
        // element (AllIEIs[i]) is the last one element for the Sub.
        //
        // Note
        //   case 1:  if Elt == nullptr, no aliasing
        //   case 2:  if Elt != nullptr && Fromvec == nullptr, scalar aliasing
        //   case 3:  if Elt != nullptr && FromVec != nullptr,
        //            (FromVec, FromVec_eltIx) sub-vector aliasing
        //
        Value* Elt = AllIEIs[i].Elt;
        if (!Elt ||
            (Sub && (i - SubStartIx) != AllIEIs[i].FromVec_eltIx)) {
            isSubCandidate = false;
        }

        if (Elt && Sub == nullptr && skipScalarAliaser(BB, Elt)) {
            // Skip scalar coalescing
            isSubCandidate = false;
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


    InsertElementInst* LastIEI = AllIEIs.back().IEI;
    auto B_use = getCandidateStateUse(LastIEI);
    bool hasAlias = false;
    for (int i = 0, sz = (int)SubVecs.size(); i < sz; ++i)
    {
        std::pair<Value*, int>& aPair = SubVecs[i];
        Value* V = aPair.first;
        int V_ix = aPair.second;

        // If V is an arg, skip it
        if (isOrCoalescedWithArg(V)) {
            continue;
        }

        // If V has been an aliaser, skip.
        if (isAliaser(V)) {
            continue;
        }

        auto S_use = getCandidateStateUse(V);
        auto S_def = getCandidateStateDef(V);
        if (!aliasOkay(S_use, S_def, B_use))
            continue;

        e_alignment BaseAlign;
        if (!checkSubAlign(BaseAlign, V, LastIEI, V_ix))
            continue;

        Value* V_nv = m_DeSSA->getNodeValue(V);
        if (!m_DeSSA->isSingleValued(V_nv)) {
            if (control < 2)
                continue;

            if (hasAnyDCCAsAliaser(V_nv))
                continue;

            if (aliasInterfere(V_nv, Base_nv, V_ix)) {
                continue;
            }
        }
        addVecAlias(V_nv, Base_nv, FirstIEI, V_ix, BaseAlign);

        int V_sz = getNumElts(V);
        if (V_sz > 1)
        {
            // set up Noop inst
            // Make sure noop insts are in the map.
            for (int j = V_ix, sz = V_ix + V_sz; j < sz; ++j)
            {
                // Safe to mark IEI as noop as IEI chain's coalesced by DeSSA
                InsertElementInst* IEI = AllIEIs[j].IEI;
                if (!m_DeSSA->isNoopAliaser(IEI)) {
                    m_HasBecomeNoopInsts[IEI] = 1;
                }

                ExtractElementInst* EEI = AllIEIs[j].EEI;
                IGC_ASSERT(EEI);
                // Sub-vector
                if (!m_DeSSA->isNoopAliaser(EEI)) {
                    // EEI should be in alias map so it can be marked as noop
                    Value *EEI_nv = m_DeSSA->getNodeValue(EEI);
                    addVecAlias(EEI_nv, Base_nv, FirstIEI, j);
                    m_HasBecomeNoopInsts[EEI] = 1;
                }
            }
        }
        else {
            // scalar
            // Safe to mark IEI as noop as IEI chain's coalesced by DeSSA
            InsertElementInst* IEI = AllIEIs[V_ix].IEI;
            if (m_DeSSA->isNoopAliaser(IEI))
                continue;
            m_HasBecomeNoopInsts[IEI] = 1;
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
        SBaseVecDesc* baseV = aliaseeSV->Aliasee;
        int nelts = getNumElts(Aliaser);
        int Idx_end = Idx + nelts - 1;
        for (int i = 0, sz = (int)(baseV->Aliasers.size()); i < sz; ++i)
        {
            SSubVecDesc* SV = baseV->Aliasers[i];
            int start = SV->StartElementOffset;
            int end = start + SV->NumElts - 1;
            if ((start > Idx_end) || (end < Idx))
                continue;
            AliasVals.push_back(SV->Aliaser);
        }
    }
}


// Given two values : Sub and (Base, BaseIdx), check if theses two value
// interfere each other. Assume these two values are dessa node values.
bool VariableReuseAnalysis::aliasInterfere(Value* Sub, Value* Base, int BaseIdx)
{
    // Vec0 : set of values aliased to Sub (as aliasee). Sub cannot be aliaser
    //        as algo does not make an aliaser twice.
    // Vec1 : set of values aliased to Sub if Sub aliases to (Base, BaseIdx).
    ValueVectorTy Vec0, Vec1;
    Vec0.push_back(Sub);
    getAllAliasVals(Vec1, Sub, Base, BaseIdx);
    auto II0 = m_baseVecMap.find(Sub);
    if (II0 != m_baseVecMap.end()) {
        SBaseVecDesc* BV0 = II0->second;
        for (int i = 0, sz = (int)BV0->Aliasers.size(); i < sz; ++i) {
            SSubVecDesc* tSV = BV0->Aliasers[i];
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

// Check if a value is used in instructions that we handle.
VariableReuseAnalysis::AState VariableReuseAnalysis::getCandidateStateUse(
    Value* V) const
{
    // If any of its use is used as func arg, skip
    AState retSt = AState::OK;
    for (User* U : V->users()) {
        Value* Val = U;
        CastInst* CI = dyn_cast<CastInst>(Val);
        if (CI && isNoOpInst(CI, m_pCtx)) {
            Val = CI->getOperand(0);
        }
        if (GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(Val)) {
            switch (GII->getIntrinsicID()) {
            case GenISAIntrinsic::GenISA_sub_group_dpas:
            case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
            case GenISAIntrinsic::GenISA_simdBlockWrite:
                retSt = AState::TARGET;
                break;
            default:
                break;
            }
        }
        else if (StoreInst* SI = dyn_cast<StoreInst>(Val)) {
            retSt =  AState::TARGET;
        }
        else if (isa<CallInst>(Val)) {
            return AState::SKIP;
        }
    }
    return retSt;
}

// Check if a value is defined by instructions that we handle.
VariableReuseAnalysis::AState VariableReuseAnalysis::getCandidateStateDef(
    Value* V) const
{
    Value* Val = V;
    CastInst* CI = dyn_cast<CastInst>(Val);
    if (CI && isNoOpInst(CI, m_pCtx)) {
        Val = CI->getOperand(0);
    }

    // skip if V is defined by a function.
    if (GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(Val)) {
        switch (GII->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_sub_group_dpas:
        case GenISAIntrinsic::GenISA_LSC2DBlockRead:
        case GenISAIntrinsic::GenISA_simdBlockRead:
            return AState::TARGET;
        default:
            break;
        }
    }
    else if (LoadInst* SI = dyn_cast<LoadInst>(Val)) {
        return AState::TARGET;
    }
    else if (isa<CallInst>(Val)) {
        return AState::SKIP;
    }
    return AState::OK;
}

// Vector alias disables extractMask optimization. This function
// checks if extractMask optim can be applied. And the caller
// will decide whether to favor extractMask optimization.
bool VariableReuseAnalysis::isExtractMaskCandidate(Value* V) const
{
    auto BIT = [](int n) { return (uint32_t)(1 << n); };

    if (!isa<VectorType>(V->getType()))
        return false;

    uint32_t nelts = getNumElts(V);
    // Using 31 to be consistent with extractMash.
    if (nelts > 31) {
        return false;
    }
    uint32_t mask = 0;
    for (auto II = V->user_begin(), IE = V->user_end(); II != IE; ++II)
    {
        Value* V = *II;
        if (ExtractElementInst* EEI = dyn_cast<llvm::ExtractElementInst>(V))
        {
            if (ConstantInt* CI = dyn_cast<ConstantInt>(EEI->getIndexOperand()))
            {
                uint32_t indexBit = BIT(static_cast<uint>(CI->getZExtValue()));
                mask |= indexBit;
                continue;
            }
        }
        return false;
    }
    uint32_t fullMask = maskTrailingOnes<uint32_t>(nelts);
    return fullMask > mask;
}

// Check if SubVec is aligned if it becomes a sub-vector at Base_ix of
// BaseVec. If so, return true with SubVec alignment in BaseAlign.
bool VariableReuseAnalysis::checkSubAlign(e_alignment& BaseAlign,
    Value* SubVec, Value* BaseVec, int Base_ix)
{
    auto maxAlign = [](e_alignment A, e_alignment B) {
        if (A == EALIGN_AUTO)
            return B;
        if (B == EALIGN_AUTO)
            return A;
        return A > B ? A : B;
    };

    auto toBytes = [](e_alignment A) {
        switch (A) {
        case EALIGN_BYTE: return 1;
        case EALIGN_WORD: return 2;
        case EALIGN_DWORD: return 4;
        case EALIGN_QWORD: return 8;
        case EALIGN_OWORD: return 16;
        case EALIGN_HWORD: return 32;
        case EALIGN_32WORD: return 64;
        case EALIGN_64WORD: return 128;
        default: break;
        }
        return 0;
    };

    BaseAlign = EALIGN_AUTO;

    // Get element bytes from original base vector
    Type* eltTy = BaseVec->getType()->getScalarType();
    uint32_t eltBytes = (uint32_t)m_DL->getTypeStoreSize(eltTy);

    // get all coalesced values for subvec and find the max alignment
    SmallVector<Value*, 16> allVals;
    m_DeSSA->getAllCoalescedValues(SubVec, allVals);

    e_alignment sub_align = EALIGN_AUTO;
    for (auto II : allVals) {
        Value* V = II;
        e_alignment thisAlign = getMinAlignment(V, m_WIA, m_pCtx);
        sub_align = maxAlign(sub_align, thisAlign);
    }
    int sub_alignBytes = toBytes(sub_align);
    if (sub_alignBytes == 0) {
        // AUTO align is fine.
        return true;
    }

    // m_SimdSize is unavailable, using smallest simdsize for now.
    int simdsize = numLanes(m_pCtx->platform.getMinDispatchMode());
    int uLanes = (m_WIA->isUniform(BaseVec) ? 1 : simdsize);
    // If base is an aliaser at this time, must check its aliasee

    Value* BaseVec_nd = m_DeSSA->getNodeValue(BaseVec);
    int ix1 = 0;
    auto MII = m_aliasMap.find(BaseVec_nd);
    if (MII != m_aliasMap.end()) {
        SSubVecDesc* SV = MII->second;
        ix1 = SV->StartElementOffset;
    }
    int ix = Base_ix + ix1;
    int startOffset = eltBytes * uLanes * ix;
    if ((startOffset % sub_alignBytes) != 0) {
        // cannot be correctly aligned, skip
        return false;
    }
    BaseAlign = sub_align;
    return true;
}

bool VariableReuseAnalysis::skipScalarAliaser(BasicBlock* BB, Value* ScalarVal) const
{
    Instruction* I = dyn_cast<Instruction>(ScalarVal);
    // Don't count dbg instructions in BB
    unsigned InstCountInBB = BB->sizeWithoutDebug();
    return ((InstCountInBB > m_BBSizeThreshold) || !I || I->getParent() != BB);
}

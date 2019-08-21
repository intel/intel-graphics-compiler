/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "VariableReuseAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/Debug.h>
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>

using namespace llvm;
using namespace IGC;

namespace
{
    // If V is scalar, return 1.
    // if V is vector, return the number of elements.
    inline int getNumElts(Value* V) {
        VectorType* VTy = dyn_cast<VectorType>(V->getType());
        return VTy ? (int)VTy->getNumElements() : 1;
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
                .PostFix(F.getName())
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
    assert(DefInst && UseInst);
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

bool VariableReuseAnalysis::isAliaser_tbd(Value* V)
{
    return m_ValueAliasMap.count(V) > 0;
}
bool VariableReuseAnalysis::isAliasee_tbd(Value* V)
{
    return m_AliasRootMap.count(V) > 0;
}

// insertAliasPair:
//    insert alias pair (NewAliaser -> RootSV) into alias map. It
//    also updates root map. Note that alias map has a property
//    that no chain alias relation (like a --> b, b-->c) will exist.
//    Therefore, if 'NewAliaser' is already an alias root, that it,
//    it has been in root map, this funtion will remove it from the
//    alias root map by adjusting all its aliasers to be aliased to
//    the 'RootSV' (new root). Once the adjustment is done, it is
//    removed from root map.
//
// Assumption:
//    NewAliaser : must not in alias map; maybe in root map.
//    SVRoot     : must not in alias map. maybe in root map.
//
void VariableReuseAnalysis::insertAliasPair(Value* NewAliaser, SSubVecDesc& RootSV)
{
    Value* Aliasee = RootSV.BaseVector;
    assert(m_ValueAliasMap.count(NewAliaser) == 0 &&
        m_ValueAliasMap.count(Aliasee) == 0 &&
        "ICE: Aliaser already in map");
    m_ValueAliasMap.insert(std::make_pair(NewAliaser, RootSV));

    // Update aliasRoot map
    TinyPtrVector<Value*>& TPV = m_AliasRootMap[Aliasee];
#if defined( _DEBUG ) || defined( _INTERNAL )
    for (int i = 0, sz = (int)TPV.size(); i < sz; ++i) {
        assert(TPV[i] != NewAliaser && "Alias Root already has this aliaser!");
    }
#endif
    TPV.push_back(NewAliaser);

    // If 'NewAliaser' is a root, remove it as it is no longer a root.
    // All its aliasers are moved to be aliasers of 'Aliasee'.
    auto II = m_AliasRootMap.find(NewAliaser);
    if (II != m_AliasRootMap.end())
    {
        TinyPtrVector<Value*>& TPV1 = II->second;
        for (int i = 0, sz = (int)TPV1.size(); i < sz; ++i) {
            Value* oldAliaser = TPV1[i];
            auto II1 = m_ValueAliasMap.find(oldAliaser);
            assert(II1 != m_ValueAliasMap.end() &&
                "ICE: alias not in value alias map");
            II1->second.BaseVector = Aliasee;
            II1->second.StartElementOffset += RootSV.StartElementOffset;

            TPV.push_back(oldAliaser);
        }
        m_AliasRootMap.erase(II);
    }
}

int VariableReuseAnalysis::getCongruentClassSize(Value* V)
{
    SmallVector<Value*, 8> cc;    // S's congruent class
    m_DeSSA->getAllValuesInCongruentClass(V, cc);
    return (int)cc.size();
}

bool VariableReuseAnalysis::isSameSizeValue(Value* V0, Value* V1)
{
    Type* Ty0 = V0->getType();
    Type* Ty1 = V1->getType();
    if (Ty0 == Ty1) {
        return true;
    }
    VectorType* VTy0 = dyn_cast<VectorType>(Ty0);
    VectorType* VTy1 = dyn_cast<VectorType>(Ty1);
    int nelts0 = (VTy0 ? (int)VTy0->getNumElements() : 1);
    int nelts1 = (VTy1 ? (int)VTy1->getNumElements() : 1);
    if (nelts0 != nelts1) {
        return false;
    }

    Type* eTy0 = (VTy0 ? VTy0->getElementType() : Ty0);
    Type* eTy1 = (VTy1 ? VTy1->getElementType() : Ty1);
    PointerType* ePTy0 = dyn_cast<PointerType>(eTy0);
    PointerType* ePTy1 = dyn_cast<PointerType>(eTy1);
    uint32_t eBits0 =
        ePTy0 ? m_pCtx->getRegisterPointerSizeInBits(ePTy0->getAddressSpace())
        : (uint32_t)m_DL->getTypeSizeInBits(eTy0);
    uint32_t eBits1 =
        ePTy1 ? m_pCtx->getRegisterPointerSizeInBits(ePTy1->getAddressSpace())
        : (uint32_t)m_DL->getTypeSizeInBits(eTy1);
    if (eBits0 == 0 || eBits1 == 0 || eBits0 != eBits1) {
        return false;
    }
    return true;
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

    // to be deleted
    if (IGC_GET_FLAG_VALUE(VATemp) == 0) {
        auto IS = m_AliasRootMap.begin();
        auto IE = m_AliasRootMap.end();
        for (auto II = IS; II != IE; ++II)
        {
            Value* aliasee = II->first;

            //  m_ValueAliasMap's entry is a pair <aliaser, aliasee>, where
            //  aliaser is the key and aliasee is the value of the entry.
            //
            //  During creating aliases, it is guaranteed that the alias chain
            //  relation will not happen.
            //      alias chain relation
            //          a0 alias_to b0
            //          b0 alias_to b1
            assert(m_ValueAliasMap.count(aliasee) == 0 &&
                "ICE: alias chain relation exists!");

            // For each alias set, record its lifetime start, which is the
            // nearest dominator that dominates all value defs in an alias set.
            // This BB is either one that has no defintion of values in the set;
            // or one that has a defintion to a value in the set. For the former,
            // m_LifetimeAtEndOfBB is used to keep track of it; for the latter,
            // m_LifetimeAt1stDefOfBB is used.
            SmallVector<Value*, 8> allVals;
            getAllValues(allVals, aliasee);
            SmallSet<BasicBlock*, 8> defBBSet;
            SmallSet<BasicBlock*, 4> phiDefBBSet;

            for (int i = 0, sz = (int)allVals.size(); i < sz; ++i)
            {
                Value* V = allVals[i];
                if (Instruction * I = dyn_cast<Instruction>(V))
                {

                    BasicBlock* BB = I->getParent();
                    defBBSet.insert(BB);
                    if (dyn_cast<PHINode>(I)) {
                        phiDefBBSet.insert(BB);
                    }
                }
                else
                {
                    // For arg, global etc., its start is on entry.
                    // Thus, no need to insert lifetime start.
                    defBBSet.clear();
                    break;
                }
            }

            if (defBBSet.size() == 0) {
                continue;
            }

            auto BSI = defBBSet.begin();
            auto BSE = defBBSet.end();
            BasicBlock* NearestDomBB = *BSI;
            for (++BSI; BSI != BSE; ++BSI)
            {
                BasicBlock* aB = *BSI;
                NearestDomBB = m_DT->findNearestCommonDominator(NearestDomBB, aB);
            }

            // Skip emptry BBs that are going to be skipped in codegen emit.
            while (theBC->IsEmptyBlock(NearestDomBB))
            {
                auto Node = m_DT->getNode(NearestDomBB);
                NearestDomBB = Node->getIDom()->getBlock();
            }

            if (phiDefBBSet.count(NearestDomBB)) {
                // PHI's source mov are in its predecessor and could
                // be moved somewhere else. So, don't insert lifetime start.
                continue;
            }
            if (defBBSet.count(NearestDomBB))
            {
                m_LifetimeAt1stDefOfBB[aliasee] = NearestDomBB;
            }
            else
            {
                m_LifetimeAtEndOfBB[NearestDomBB].push_back(aliasee);
            }
        }
        return;
    }

    if (!m_DeSSA || IGC_GET_FLAG_VALUE(VATemp) < 3)
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
        //     All values that alias to the aliasee and their values
        //     in their dessa CC.
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
    if (IGC_GET_FLAG_VALUE(VATemp) < 4)
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
    if (IGC_GET_FLAG_VALUE(VATemp) == 0)
    {
        Value* ARV = getRootValue(V);
        auto AliaseeII = m_ValueAliasMap.find(ARV);
        if (AliaseeII != m_ValueAliasMap.end())
        {
            SSubVecDesc& svd = AliaseeII->second;
            ARV = svd.BaseVector;
        }
        return ARV;
    }

    Value* V_nv = m_DeSSA ? m_DeSSA->getNodeValue(V) : V;
    auto II = m_aliasMap.find(V_nv);
    if (II == m_aliasMap.end()) {
        return V_nv;
    }
    return II->second->BaseVector;
}

bool VariableReuseAnalysis::addAlias(Value* Aliaser, SSubVecDesc& SVD)
{
    // In case, aliasee is an "aliaser" to the other a root value,
    // the other value shall be the new aliasee.
    SSubVecDesc tSVD = SVD;
    auto AliaseeII = m_ValueAliasMap.find(tSVD.BaseVector);
    if (AliaseeII != m_ValueAliasMap.end()) {
        SSubVecDesc& svd0 = AliaseeII->second;
        tSVD.BaseVector = svd0.BaseVector;
        tSVD.StartElementOffset += svd0.StartElementOffset;
    }
    Value* Aliasee = tSVD.BaseVector;
    if (Aliasee == Aliaser) {
        // Circular aliasing relation. Skip
        return false;
    }

    // Now, 'Aliasee' may be in root map, it must not in value map.
    auto II = m_ValueAliasMap.find(Aliaser);
    if (II == m_ValueAliasMap.end())
    {
        // both 'Aliaser' and 'Aliasee' are not in value map.
        insertAliasPair(Aliaser, tSVD);
    }
    else {
        SSubVecDesc& existingRSV = II->second;
        Value* existingRV = existingRSV.BaseVector;
        if (isSameSizeValue(Aliaser, Aliasee) &&
            tSVD.StartElementOffset == 0) {
            // add alias(Aliasee, existingRV)
            insertAliasPair(Aliasee, existingRSV);
        }
        else if (isSameSizeValue(Aliaser, existingRV) &&
            existingRSV.StartElementOffset == 0)
        {
            insertAliasPair(existingRV, tSVD);
        }
        else {
            return false;
        }
    }
    return true;
}

// Return all values in the set rooted at Aliasee
void VariableReuseAnalysis::getAllValues(
    SmallVector<Value*, 8> & AllValues,
    Value* Aliasee)
{
    SmallVector<Value*, 8> valInCC;
    m_DeSSA->getAllValuesInCongruentClass(Aliasee, valInCC);
    AllValues.insert(AllValues.end(), valInCC.begin(), valInCC.end());
    valInCC.clear();

    // If it has aliasers, add their values in the set
    if (m_AliasRootMap.count(Aliasee) > 0)
    {
        TinyPtrVector<Value*>& aliasSet = m_AliasRootMap[Aliasee];
        for (int i = 0, sz = (int)aliasSet.size(); i < sz; ++i)
        {
            Value* V = aliasSet[i];
            m_DeSSA->getAllValuesInCongruentClass(V, valInCC);
            AllValues.insert(AllValues.end(), valInCC.begin(), valInCC.end());
            valInCC.clear();
        }
    }
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

// If all elements inserted by this chain IEI are from EEI, populate
// AllElts and return the last IEI as RootIEI.
bool VariableReuseAnalysis::checkAndGetAllInsertElements(
    InsertElementInst* FirstIEI, ValueVectorTy& AllIEIs, VecEltTy& AllElts)
{
    VectorType* VTy = cast<VectorType>(FirstIEI->getType());
    int nelts = (int)VTy->getNumElements();
    assert(nelts == (int)AllElts.size() && "AllElts's size is set up correctly!");

    InsertElementInst* I = FirstIEI;
    while (I)
    {
        Value* EEI_Vec;
        int IEI_ix, EEI_ix;
        if (!getVectorIndicesIfConstant(I, IEI_ix, EEI_Vec, EEI_ix)) {
            return false;
        }
        if (AllElts[IEI_ix].Vec) {
            // Assume one element is inserted exactly once.
            // Return false if  not.
            return false;
        }
        AllElts[IEI_ix].Vec = EEI_Vec;
        AllElts[IEI_ix].EltIx = EEI_ix;

        AllIEIs.push_back(I);
        if (!I->hasOneUse()) {
            break;
        }
        I = dyn_cast<InsertElementInst>(I->user_back());
    }

    if (AllIEIs.empty() || AllIEIs.back()->use_empty()) {
        return false;
    }

    // Make sure all elements are present
    for (int i = 0; i < nelts; ++i) {
        if (!AllElts[i].Vec)
            return false;
    }
    return true;
}

void VariableReuseAnalysis::visitCallInst(CallInst& I)
{
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 2) {
        return;
    }

    if (GenIntrinsicInst * GII = dyn_cast<GenIntrinsicInst>(&I))
    {
        GenISAIntrinsic::ID id = GII->getIntrinsicID();
        if (id == GenISAIntrinsic::GenISA_Copy)
        {
            Value* Src = GII->getArgOperand(0);
            if (hasBeenPayloadCoalesced(GII) ||
                hasBeenPayloadCoalesced(Src) ||
                getCongruentClassSize(GII) != 1 ||
                getCongruentClassSize(Src) != 1)
            {
                return;
            }

            SSubVecDesc SV;
            SV.BaseVector = Src;
            SV.StartElementOffset = 0;
            if (addAlias(GII, SV)) {
                m_HasBecomeNoopInsts[&I] = 1;
            }
        }
        else if (id == GenISAIntrinsic::GenISA_WaveShuffleIndex) {
            Value* Src = GII->getArgOperand(0);
            Value* Index = GII->getArgOperand(1);
            if (hasBeenPayloadCoalesced(GII) ||
                hasBeenPayloadCoalesced(Src) ||
                getCongruentClassSize(GII) != 1 ||
                getCongruentClassSize(Src) != 1)
            {
                return;
            }

            if (ConstantInt * CST = dyn_cast<ConstantInt>(Index))
            {
                int ix = (int)CST->getZExtValue();
                SSubVecDesc SV;
                SV.BaseVector = Src;
                SV.StartElementOffset = ix;
                if (addAlias(GII, SV)) {
                    m_HasBecomeNoopInsts[&I] = 1;
                }
            }
        }
    }
}

void VariableReuseAnalysis::visitCastInst(CastInst& I)
{
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 1 ||
        IGC_GET_FLAG_VALUE(EnableDeSSAAlias) > 1)
        return;

    if (!canBeAlias(&I)) {
        return;
    }

    // Set alias of dst to CastInst's src
    // As CastInst is noop, its definition is dropped and
    // only its uses are merged to src's liveness info.
    Value* D = &I;
    Value* S = I.getOperand(0);

    SSubVecDesc SV;
    SV.BaseVector = S;
    SV.StartElementOffset = 0;
    if (addAlias(D, SV)) {
        m_HasBecomeNoopInsts[&I] = 1;
    }
    else {
        // If D is aliased to another value already, it cannot
        // alias to S again. But we can check if S can be aliased
        // to D.
        SV.BaseVector = D;
        if (addAlias(S, SV)) {
            m_HasBecomeNoopInsts[&I] = 1;
        }
    }
    // This is probably not needed!
    // Extend S's liveness to contain D's
    // m_LV->mergeUseFrom(S, D);
}

// to be deleted
void VariableReuseAnalysis::visitInsertElementInst(InsertElementInst& I)
{
    if (IGC_GET_FLAG_VALUE(VATemp) == 0) {
        // old code
        visitInsertElementInst_toBeDeleted(I);
        return;
    }
}

void VariableReuseAnalysis::visitInsertElementInst_toBeDeleted(InsertElementInst& I)
{
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 2)
        return;

    // Two cases for sub-vector aliasing:
    //   1. extractFrom: sub-vector is created from a base vector.
    //      For example:
    //         given base: int8 b;  a sub-vector s (int4) can be:
    //         s = (int4)(b.s4, b.s5, b.s6, b.s7) // extract and insert in llvm
    //      In this case, 's' becomes a part of 'b'.
    //   2. insertTo: sub-vector is used to create a base vector.
    //      For example:
    //         given sub-vector int4 s0, s1;  int8 vector b is created like:
    //           b = (int8) (s0, s1)  // extract and insert
    //      In this case,  both s0 and s1 ecome part of b.

    // Start insertElement pattern from the first InsertElement, ie, one with UndefValue.
    if (!isa<UndefValue>(I.getOperand(0)))
        return;

    VectorType* VTy = cast<VectorType>(I.getType());
    int nelts = (int)VTy->getNumElements();

    // Sanity
    if (nelts < 2)
        return;

    VecEltTy  AllElts(nelts);
    ValueVectorTy AllIEIs;
    if (!checkAndGetAllInsertElements(&I, AllIEIs, AllElts)) {
        return;
    }

    assert(AllIEIs.size() == nelts && "ICE: wrong the number of IEIs!");

    InsertElementInst* LastIEI = cast<InsertElementInst>(AllIEIs.back());
    SSubVecDesc SV;
    SmallVector<SSubVecDesc, 4> SVs;
    if (IsExtractFrom(AllElts, &I, LastIEI, SV))
    {
        if (addAlias(LastIEI, SV))
        {
            for (int i = 0, sz = (int)AllIEIs.size(); i < sz; ++i)
            {
                Instruction* Inst = cast<Instruction>(AllIEIs[i]);
                m_HasBecomeNoopInsts[Inst] = 1;
            }
        }
        return;
    }

    SVs.clear();
    if (IsInsertTo(AllElts, &I, LastIEI, SVs))
    {
        for (int i = 0, sz = (int)SVs.size(); i < sz; ++i)
        {
            SSubVecDesc& subvec = SVs[i];
            Value* aliaser = subvec.BaseVector;
            subvec.BaseVector = LastIEI;
            if (addAlias(aliaser, subvec)) {
                VectorType* Ty = dyn_cast<VectorType>(aliaser->getType());
                int sz = Ty ? (int)Ty->getNumElements() : 1;
                int startIx = (int)subvec.StartElementOffset;
                for (int i = startIx, e = startIx + sz; i < e; ++i) {
                    Instruction* Inst = cast<Instruction>(AllIEIs[i]);
                    m_HasBecomeNoopInsts[Inst] = 1;
                }
            }
        }
        return;
    }
}

void VariableReuseAnalysis::visitExtractElementInst(ExtractElementInst& I)
{
    if (IGC_GET_FLAG_VALUE(VATemp) == 0) {
        visitExtractElementInst_toBeDeleted(I);
        return;
    }

    // Do it for OCL only (todo enable it for other api)
    if (!m_pCtx->m_DriverInfo.EnableVecAliasing())
    {
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

void VariableReuseAnalysis::visitExtractElementInst_toBeDeleted(ExtractElementInst& I)
{
    // Only handles ExtractElements whose indexes are all known constants.
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 2)
        return;

    if (m_HasBecomeNoopInsts.count(&I))
        return;

    // Process all Extract elements of the vector operand.
    // Do it when the function sees "e0 = EEI V, 0"
    ConstantInt* Idx = dyn_cast<ConstantInt>(I.getIndexOperand());
    if (!Idx || Idx->getZExtValue() != 0) {
        return;
    }

    Value* Vec = I.getVectorOperand();
    if (isAliasedValue(Vec))
    {
        return;
    }

    VectorType* VTy = cast<VectorType>(Vec->getType());
    int nelts = (int)VTy->getNumElements();
    SmallVector<ExtractElementInst*, 8> allEEs(nelts, nullptr);
    allEEs[0] = &I;
    for (auto user : Vec->users())
    {
        ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(user);
        if (!EEI || EEI == &I)
        {
            continue;
        }
        ConstantInt* Index = dyn_cast<ConstantInt>(EEI->getIndexOperand());
        if (!Index)
        {
            continue;
        }
        int ix = (int)Index->getZExtValue();
        if (ix < nelts && allEEs[ix] == nullptr)
        {
            allEEs[ix] = EEI;
        }
    }

    for (int i = 0; i < nelts; ++i)
    {
        ExtractElementInst* EEI = allEEs[i];
        if (!EEI || isAliasedValue(EEI) ||
            (m_WIA && m_WIA->whichDepend(EEI) != m_WIA->whichDepend(Vec)))
        {
            continue;
        }

        if (aliasHasInterference(EEI, Vec)) {
            continue;
        }

        SSubVecDesc SV;
        SV.BaseVector = Vec;
        SV.StartElementOffset = i;
        if (addAlias(EEI, SV)) {
            m_HasBecomeNoopInsts[EEI] = 1;
        }
    }
}

bool VariableReuseAnalysis::isLocalValue(Value* V)
{
    Instruction* I = dyn_cast<Instruction>(V);
    if (!I)
        return false;
    BasicBlock* BB = I->getParent();
    return !m_LV->isLiveIn(I, *BB) && !m_LV->isLiveOut(I, *BB);
}

// Return true if live ranges of aliaser and aliasee overlap. The
// difference between this one and hasInterference() is that both aliaser
// and aliasee in this function are excluded from congruent class
// as they are with the identical value (that is why they are aliased).
bool VariableReuseAnalysis::aliasHasInterference(Value* Aliaser, Value* Aliasee)
{
    // As two aliased variables have the identical value, their interference
    // does not prevent merging them into a single variable (overlapping or not,
    // they still have the same value).
    if (!m_DeSSA) {
        return false;
    }

    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 3)
    {
        // should have this check ?
        if (getCongruentClassSize(Aliaser) != 1 ||
            getCongruentClassSize(Aliasee) != 1) {
            return true;
        }
        return false;
    }

    // THis is for EnableVATemp =3
    // [todo] find a better way to handle alias with values whose
    // congruent class has more than one values.
    SmallVector<Value*, 8> Aliasercc;  // Aliaser's congruent class
    SmallVector<Value*, 8> Aliaseecc;  // Aliasee's congruent class
    m_DeSSA->getAllValuesInCongruentClass(Aliaser, Aliasercc);
    m_DeSSA->getAllValuesInCongruentClass(Aliasee, Aliaseecc);

    // Check every pair of values in two congruent classes
    // and exclude Aliaser and Aliasee.
    Value* AliaserInsEltRoot = m_DeSSA->getInsEltRoot(Aliaser);
    Value* AliaseeInsEltRoot = m_DeSSA->getInsEltRoot(Aliasee);
    for (int i = 0, sz0 = (int)Aliasercc.size(); i < sz0; ++i)
    {
        Value* val0 = Aliasercc[i];
        for (int j = 0, sz1 = (int)Aliaseecc.size(); j < sz1; ++j)
        {
            Value* val1 = Aliaseecc[j];
            if (val0 == AliaserInsEltRoot && val1 == AliaseeInsEltRoot)
                continue;

            if (m_LV->hasInterference(val0, val1))
                return true;
        }
    }
    return false;
}

// Return true if V0 and V1's live ranges overlap, return false otherwise.
bool VariableReuseAnalysis::hasInterference(Value* V0, Value* V1)
{
    // Key assumption about Congruent class (dessa)/LVInfo(LiveVars):
    //   1. Single-definition liveness
    //      LVInfo has liveness info for each llvm value, which has a single
    //      definition throughout its live ranges. This is what LLVM's value means.
    //   2. Multiple-definition congruent class
    //      If two values are combined, they should be in the same congruent class,
    //      not by extending liveness to reflect both.
    //
    //   For example:
    //          1:   v0 = 10
    //          2:      = v0 (last use)
    //          3:   v1 = 20
    //          4:      = v1 (last use)
    //   Assume v0 and v1 are combined, they will be put in the same congruent class.
    //   We will not extend  v0's liveness to "4" (same for v1).
    //
    //   Another example:
    //          1:   v0 = 10
    //          2:      = v0 (last use)
    //          3:   v1 = bitcast v0
    //          4:      = v1 (last use) 
    //   then we can just extend v0's liveness to "4", and v1 to be alias to v0.
    //        
    SmallVector<Value*, 8> V0cc;  // V0's congruent class
    SmallVector<Value*, 8> V1cc;  // V1's congruent class
    if (m_DeSSA) {
        m_DeSSA->getAllValuesInCongruentClass(V0, V0cc);
        m_DeSSA->getAllValuesInCongruentClass(V1, V1cc);
    }
    else {
        V0cc.push_back(V0);
        V1cc.push_back(V1);
    }

    // Check every pair of values in two congruent classes
    for (int i = 0, sz0 = (int)V0cc.size(); i < sz0; ++i)
    {
        Value* val0 = V0cc[i];
        for (int j = 0, sz1 = (int)V1cc.size(); j < sz1; ++j)
        {
            Value* val1 = V1cc[j];
            if (m_LV->hasInterference(val0, val1))
                return true;
        }
    }

    return false;
}

// Check if src and dst of CastInst can be an alias to each other. It is used
// for checking alias-possible instructions such as bitcast/inttoptr/ptrtoint.
//
// This is trivial for LLVM IR, as LLVM IR is SSA. But after DeSSA,
// need to check other values in their congruent classes.
//
bool VariableReuseAnalysis::canBeAlias(CastInst* I)
{
    if (hasBeenPayloadCoalesced(I)) {
        return false;
    }
    if (!isNoOpInst(I, m_pCtx)) {
        return false;
    }

    // Set alias of dst to CastInst's src
    // As CastInst is noop, its definition is dropped and
    // only its uses are merged to src's liveness info.
    Value* D = I;
    Value* S = I->getOperand(0);
    if (isa<Constant>(S)) {
        return false;
    }

    if (hasBeenPayloadCoalesced(S)) {
        return false;
    }

    if (!m_DeSSA) {
        // No congruent class, so it can be alias!
        return true;
    }

    if (getCongruentClassSize(D) != 1 ||
        getCongruentClassSize(S) != 1 ||
        (m_WIA && m_WIA->whichDepend(D) != m_WIA->whichDepend(S))) {
        return false;
    }
    return true;
}

// Check if the vector value of InsertElement is
// a sub-vector of another one, return true if so.
bool VariableReuseAnalysis::IsExtractFrom(
    VecEltTy& AllElts, InsertElementInst* FirstIEI,
    InsertElementInst* LastIEI, SSubVecDesc& SV)
{
    int nelts = (int)AllElts.size();
    Value* BaseVec = AllElts[0].Vec;
    int BaseStartIx = AllElts[0].EltIx;
    VectorType* BVTy = dyn_cast<VectorType>(BaseVec->getType());
    if (BVTy == nullptr) {
        // Base is not a vector, so IEI cannot be
        // a subvector of another vector!
        return false;
    }
    int base_nelts = (int)BVTy->getNumElements();

    // If Base's size is smaller than IEI's, IEI cannot be sub-vector
    if (base_nelts < nelts) {
        return false;
    }

    for (int i = 1; i < nelts; ++i)
    {
        if (AllElts[i].Vec != BaseVec ||
            AllElts[i].EltIx != (BaseStartIx + i))
            return false;
    }

    // Don't do it if any of them is payload-coalesced
    if (hasBeenPayloadCoalesced(LastIEI) ||
        hasBeenPayloadCoalesced(BaseVec) ||
        (m_WIA && m_WIA->whichDepend(LastIEI) != m_WIA->whichDepend(BaseVec)))
    {
        return false;
    }

    if (aliasHasInterference(LastIEI, BaseVec)) {
        return false;
    }

    SV.StartElementOffset = BaseStartIx;
    SV.BaseVector = BaseVec;
    return true;
}

bool VariableReuseAnalysis::IsInsertTo(
    VecEltTy& AllElts, InsertElementInst* FirstIEI,
    InsertElementInst* LastIEI, SmallVector<SSubVecDesc, 4> & SVs)
{
    int nelts = (int)AllElts.size();
    Value* SubVec = AllElts[0].Vec;
    int SubStartIx = 0;

    for (int i = 0; i < nelts; ++i)
    {
        // 1. AllElts[i].Vec must be SubVec.
        // 2. Check the next SubVec, if it change, the current
        //    element is the last one of the crrent SubVec;
        //    and SSubVecDesc will be created if the SubVec meets
        //    the condition.
        if ((i - SubStartIx) != AllElts[i].EltIx) {
            return false;
        }

        Value* NextSub = (i < (nelts - 1)) ? AllElts[i + 1].Vec : nullptr;
        if (SubVec != NextSub)
        {
            // NextSub should be the new sub vector. Make sure it is not in SVs
            // Note this works for speical case in which NextSub = nullptr.
            for (int j = 0, sz = (int)SVs.size(); j < sz; ++j) {
                if (SVs[j].BaseVector == NextSub) {
                    return false;
                }
            }

            // End of the SubVec.
            VectorType* BVTy = dyn_cast<VectorType>(SubVec->getType());
            int sub_nelts = BVTy ? (int)BVTy->getNumElements() : 1;
            // If Sub's size is not smaller than IEI's, or not all sub's
            // elements are used, skip.
            if (sub_nelts >= nelts || (i - SubStartIx) != (sub_nelts - 1)) {
                return false;
            }

            SSubVecDesc sv;
            sv.BaseVector = SubVec;  // note that this is sub, not base!
            sv.StartElementOffset = SubStartIx;
            SVs.push_back(sv);

            SubVec = NextSub;
            SubStartIx = i + 1;
        }
    }

    // If IEI or any of its sub vector has been payload-coalesced, skip.
    if (hasBeenPayloadCoalesced(LastIEI)) {
        return false;
    }

    for (int i = 0, sz = (int)SVs.size(); i < sz; ++i)
    {
        Value* SubVec = SVs[i].BaseVector;
        if (hasBeenPayloadCoalesced(SubVec) ||
            (m_WIA && m_WIA->whichDepend(LastIEI) != m_WIA->whichDepend(SubVec))) {
            return false;
        }
    }

    for (int i = 0, sz = (int)SVs.size(); i < sz; ++i)
    {
        Value* SubVec = SVs[i].BaseVector;
        if (aliasHasInterference(SubVec, LastIEI)) {
            return false;
        }
    }
    return true;
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

    if (IGC_GET_FLAG_VALUE(VATemp) == 0)
    {   // tobedeleted
        for (auto& MI : m_AliasRootMap)
        {
            Value* aliasee = MI.first; // root value
            TinyPtrVector<llvm::Value*> aliasers = MI.second;
            OS << "Aliasee : " << *aliasee << "\n";
            for (auto VI : aliasers)
            {
                Value* aliaser = VI;
                auto II = m_ValueAliasMap.find(aliaser);
                if (II == m_ValueAliasMap.end()) {
                    OS << "    " << *aliaser << "  [Wrong Value Alias]\n";
                    assert(false && "ICE VariableAlias: wrong Value alias map!");
                }
                else {
                    const SSubVecDesc& SV = II->second;
                    OS << "    " << *aliaser << "  [" << SV.StartElementOffset << "]\n";
                }
            }
            OS << "\n";
        }
        OS << "\n";
        return;
    }

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
// If any value in V's DCC is aliaer, return true.
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
            assert(MI != m_aliasMap.end());
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
            assert(MI != m_aliasMap.end());
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

        Value* V;
        Value* E;
        int IEI_ix, V_ix;
        if (!getElementValue(I, IEI_ix, E, V, V_ix)) {
            return false;
        }

        assert(IEI_ix < nelts && "ICE: IEI's index out of bound!");
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
    if (IGC_GET_FLAG_VALUE(VATemp) < 2 ||
        !m_pCtx->m_DriverInfo.EnableVecAliasing()) {
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
        assert(EEI);
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
                assert(EEI);
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
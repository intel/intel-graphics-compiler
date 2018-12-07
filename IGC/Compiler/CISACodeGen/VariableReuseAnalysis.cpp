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

using namespace llvm;
using namespace IGC;

char VariableReuseAnalysis::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(VariableReuseAnalysis, "VariableReuseAnalysis",
                          "VariableReuseAnalysis", false, true)
// IGC_INITIALIZE_PASS_DEPENDENCY(RegisterEstimator)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch)
IGC_INITIALIZE_PASS_DEPENDENCY(DeSSA)
IGC_INITIALIZE_PASS_DEPENDENCY(CoalescingEngine)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(VariableReuseAnalysis, "VariableReuseAnalysis",
                        "VariableReuseAnalysis", false, true)

llvm::FunctionPass *IGC::createVariableReuseAnalysisPass() {
  return new VariableReuseAnalysis;
}

VariableReuseAnalysis::VariableReuseAnalysis()
    : FunctionPass(ID),
      m_WIA(nullptr), m_LV(nullptr), m_DeSSA(nullptr),
      m_PatternMatch(nullptr), m_coalescingEngine(nullptr),
      m_pCtx(nullptr), m_RPE(nullptr), m_SimdSize(0),
      m_IsFunctionPressureLow(Status::Undef),
      m_IsBlockPressureLow(Status::Undef) {
  initializeVariableReuseAnalysisPass(*PassRegistry::getPassRegistry());
}

bool VariableReuseAnalysis::runOnFunction(Function &F)
{
  m_WIA = &(getAnalysis<WIAnalysis>());
  if (IGC_IS_FLAG_DISABLED(DisableDeSSA))
  {
	m_DeSSA = &getAnalysis<DeSSA>();
  }
  m_LV = &(getAnalysis<LiveVarsAnalysis>().getLiveVars());
  m_PatternMatch = &getAnalysis<CodeGenPatternMatch>();
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_coalescingEngine = &getAnalysis<CoalescingEngine>();
  m_DL = &F.getParent()->getDataLayout();

  // FIXME: enable RPE.
  // m_RPE = &getAnalysis<RegisterEstimator>();

  // Nothing but cleanup data from previous runs.
  reset();

  if (IGC_IS_FLAG_ENABLED(EnableVariableAlias) &&
      m_pCtx->platform.GetPlatformFamily() >= IGFX_GEN9_CORE)
  {
      // 0. Special handling.
      //    Here, try to merge two different variables into a single one.
      //    The two vars that will be merged should have the same
      //    size and normally are defined with different values.
      //    Nonetheless, their live ranges do not interfere with each
      //    other. They are added into the same congruent class (as they
      //    are actually different variables).
      mergeVariables(&F);

      // 1. Do variable aliasing, such as sub-vector to vector, some cast
      //    instructions (bitcast, ptrtoint, etc.). The aliasing relation
      //    does not involve more than one value. Thus, we add aliasing
      //    variables into a different map, they don't fit well into congruent
      //    class (doing so will be over-conservative).
      visitLiveInstructions(&F);

      postProcessing();
  }

  return false;
}

static unsigned getMaxReuseDistance(uint16_t size) {
  return (size == 8) ? 10 : 5;
}

bool VariableReuseAnalysis::checkUseInst(Instruction *UseInst, LiveVars *LV) {
  BasicBlock *CurBB = UseInst->getParent();
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

bool VariableReuseAnalysis::checkDefInst(Instruction *DefInst,
                                         Instruction *UseInst, LiveVars *LV) {
  assert(DefInst && UseInst);
  if (isa<PHINode>(DefInst))
    return false;

  if (auto CI = dyn_cast<CallInst>(DefInst)) {
    Function *F = CI->getCalledFunction();
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
  BasicBlock *CurBB = UseInst->getParent();
  if (DefInst->getParent() != CurBB || DefInst->isUsedOutsideOfBlock(CurBB))
    return false;

  // Check whether UseInst is the last use of DefInst. If not, this source
  // variable cannot be reused.
  Instruction *LastUse = LV->getLVInfo(DefInst).findKill(CurBB);
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

bool VariableReuseAnalysis::isAliaser(llvm::Value* V)
{
    return m_ValueAliasMap.count(V) > 0;
}
bool VariableReuseAnalysis::isAliasee(llvm::Value*V)
{
    return m_AliasRootMap.count(V) > 0;
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
    PointerType *ePTy0 = dyn_cast<PointerType>(eTy0);
    PointerType *ePTy1 = dyn_cast<PointerType>(eTy1);
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
        if (GenIntrinsicInst *CI = dyn_cast<GenIntrinsicInst>(I))
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

void VariableReuseAnalysis::postProcessing()
{
    //  m_ValueAliasMap's entry is a pair <aliaser, aliasee>, where
    //  aliaser is the key and aliasee is the value of the entry.
    // 
    // Normalizing alias map so that the following will be handled:
    //  1) alias chain relation
    //        a0 alias_to b0
    //        b0 alias_to b1
    //     Change to
    //        a0 alias_to b1
    //        b0 alias_to b1
    //    This make sure that any map value will not be a map key
    //  2) circular alias relation
    //     It might be possible to generate a circular alias relation like:
    //        a0 alias_to b0
    //        b0 alias_to b1
    //        b1 alias_to a0
    //     Change to the following by removing one of alias pair.
    //        a0 alias_to b1
    //        b0 alias_to b1
    //
    int sz = (int)m_ValueAliasMap.size();
    auto NI = m_ValueAliasMap.begin();
    auto IE = m_ValueAliasMap.end();
    for (auto II = NI; II != IE; II = NI)
    {
        ++NI;
        SSubVector& SV = II->second;
        Value* aliasee = SV.BaseVector;
        int off = SV.StartElementOffset;

        // Checking a circular aliasing relation.
        //   With map's size = sz, it can do max looping of (sz -1)
        //   trip count without revisiting an entry twice. If the loop
        //   revisit an entry, it must have a circular alias relation.
        int k = 0;
        while (m_ValueAliasMap.count(aliasee) > 0 && k < sz)
        {
            ++k;
            SSubVector& tSV = m_ValueAliasMap[aliasee];
            off += tSV.StartElementOffset;
            aliasee = tSV.BaseVector;
        }
        if (k == sz)
        {
            // circular alias relation
            m_ValueAliasMap.erase(II);
            --sz;
            continue;
        }
        SV.BaseVector = aliasee;
        SV.StartElementOffset = off;

        Value* aliaser = II->first;
        if (m_AliasRootMap.count(aliasee) == 0) {
            TinyPtrVector<Value*> TPV;
            TPV.push_back(aliaser);
            m_AliasRootMap.insert(std::make_pair(aliasee, TPV));
        }
        else {
            m_AliasRootMap[aliasee].push_back(aliaser);
        }
    }
}

// [Todo] rename SSubVector to SSubVecDesc (sub-vector descriptor)
bool VariableReuseAnalysis::addAlias(Value* Aliaser, SSubVector& SVD)
{
    // In case, aliasee is an "aliaser" to the other value.
    // the other value shall be the final baseVector.
    SSubVector tSVD = SVD;
    auto AliaseeII = m_ValueAliasMap.find(tSVD.BaseVector);
    if (AliaseeII != m_ValueAliasMap.end()) {
        SSubVector& svd0 = AliaseeII->second;
        tSVD.BaseVector = svd0.BaseVector;
        tSVD.StartElementOffset += svd0.StartElementOffset;
    }
    Value* Aliasee = tSVD.BaseVector;
    if (Aliasee == Aliaser) {
        // Circular aliasing relation. Skip
        return false;
    }

    auto II = m_ValueAliasMap.find(Aliaser);
    if (II == m_ValueAliasMap.end())
    {
        m_ValueAliasMap.insert(std::make_pair(Aliaser, tSVD));
    }
    else {
        // todo: might merge alias(aliaser, aliasee) with alias(aliaser, existingV)
        // if size(aliaser) == size(existingV) or size(aliaser) == size(aliasee)
        // do merging
        // Note that it's possible to have a conflicting alias !
        SSubVector& existingSV = II->second;
        Value *existingV = existingSV.BaseVector;
        if (isSameSizeValue(Aliaser, Aliasee) &&
            tSVD.StartElementOffset == 0) {
            // add alias(Aliasee, existingV)
            Aliaser = Aliasee;
            Aliasee = existingV;
            m_ValueAliasMap.insert(std::make_pair(Aliasee, existingSV));
        }
        else if (isSameSizeValue(Aliaser, existingV) &&
                 existingSV.StartElementOffset == 0)
        {
            m_ValueAliasMap.insert(std::make_pair(existingV, tSVD));
            Aliaser = existingV;
        }
        else {
            return false;
        }
    }

    // Update aliasRoot map
    auto II0 = m_AliasRootMap.find(Aliasee);
    if (II0 == m_AliasRootMap.end())
    {
        TinyPtrVector<Value*> TPV;
        TPV.push_back(Aliaser);
        m_AliasRootMap.insert(std::make_pair(Aliasee, TPV));
    }
    else {
        II0->second.push_back(Aliaser);
    }

    // Aliaser is no longer a root anymore
    auto II1 = m_AliasRootMap.find(Aliaser);
    if (II1 != m_AliasRootMap.end())
    {
        TinyPtrVector<Value*>& TPV = II1->second;
        for (int i = 0, sz = TPV.size(); i < sz; ++i) {
            m_AliasRootMap[Aliasee].push_back(TPV[i]);
        }
        m_AliasRootMap.erase(II1);
    }
    return true;
}

// Returns true for the following pattern:
//   a = extractElement <vectorType> EEI_Vec, <constant EEI_ix>
//   b = insertElement  <vectorType> V1,  a,  <constant IEI_ix>
// where EEI_ix and IEI_ix are constants; Return false otherwise.
bool VariableReuseAnalysis::getVectorIndicesIfConstant(
    InsertElementInst* IEI, int& IEI_ix, Value*& EEI_Vec, int&EEI_ix)
{
    // Check if I has constant index, skip if not.
    ConstantInt* CI = dyn_cast<ConstantInt>(IEI->getOperand(2));
    if (!CI) {
        return false;
    }
    IEI_ix = (int)CI->getZExtValue();

    // Check that the elements inserted are from extractElement
    // Also, special-handling of insertelement itself.
    Value *elem = IEI->getOperand(1);
    ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(elem);
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

    InsertElementInst *I = FirstIEI;
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
    if (GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(&I))
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

            SSubVector SV;
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

            if (ConstantInt* CST = dyn_cast<ConstantInt>(Index))
            {
                int ix = (int)CST->getZExtValue();
                SSubVector SV;
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
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 1)
        return;

    if (!canBeAlias(&I)) {
        return;
    }

    // Set alias of dst to CastInst's src
    // As CastInst is noop, its definition is dropped and
    // only its uses are merged to src's liveness info.
    Value* D = &I;
    Value* S = I.getOperand(0);

#if 0
    // temporary
    if (GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(S))
    {
        Type* Ty = GII->getType();
        if (VectorType* VTy = dyn_cast<VectorType>(Ty))
        {
            if (VTy->getNumElements() > 1) {
                D = S;
                S = &I;
            }
        }
    }
#endif
    SSubVector SV;
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

void VariableReuseAnalysis::visitInsertElementInst(llvm::InsertElementInst& I)
{
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 2)
        return;

    // Two cases for sub-vector aliasing:
    //   1. extractFrom: sub-vector is created from a base vector.
    //      For example:
    //         given base: int8 b;  a sub-vector s (int4) can be:
    //         s = (int4)(b.s4, b.s5, b.s6, b.s7) // extract and insert in llvm
    //      In this case, 's' becomes a part of 'b'. And 'b' liveVar info gets
    //      updated by adding this insertElement as use (not *def*) to 'b'.
    //   2. insertTo: sub-vector is used to create a base vector.
    //      For example:
    //         given sub-vector int4 s0, s1;  int8 vector b is created like:
    //           b = (int8) (s0, s1)  // extract and insert
    //      In this case,  s0 and s1 are added into b's LiveVars info. If either s0
    //      or s1 is live at the other's def, the other is only added as use (not def);
    //      otherwise both are added as def. The original definition of b should be
    //      changed to use from def. In doing so, the live range is still accurate.

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

    InsertElementInst *LastIEI = cast<InsertElementInst>(AllIEIs.back());
    SSubVector SV;
    SmallVector<SSubVector, 4> SVs;
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
            SSubVector& subvec = SVs[i];
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

void VariableReuseAnalysis::visitExtractElementInst(llvm::ExtractElementInst& I)
{
    // Only handles ExtractElements whose indexes are all known constants.
    if (IGC_GET_FLAG_VALUE(EnableVATemp) < 3)
        return;

    if (m_HasBecomeNoopInsts.count(&I))
        return;

    // Process all Extract elements (could handle one EEI at a time)
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
        if (!EEI || isAliasedValue(EEI)) {
            continue;
        }

        if (aliasHasInterference(EEI, Vec)) {
            continue;
        }

        SSubVector SV;
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

#if 0
    if (getCongruentClassSize(Aliaser) != 1 ||
        getCongruentClassSize(Aliasee) != 1) {
        return true;
    }
    return false;
#else
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
        Value *val0 = Aliasercc[i];
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
#endif
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
        Value *val0 = V0cc[i];
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

#if 0
    // If D is in a congruent class or both D and S have different
    // uniform property, give up.   
    if (m_DeSSA->getRootValue(D) ||
        (m_WIA && m_WIA->whichDepend(D) != m_WIA->whichDepend(S))) {
        return false;
    }

    SmallVector<Value*, 8> Scc;    // S's congruent class
    m_DeSSA->getAllValuesInCongruentClass(S, Scc);
    for (int i = 0, sz0 = (int)Scc.size(); i < sz0; ++i)
    {
        Value *v0 = Scc[i];
        if (v0 != S && m_LV->hasInterference(D, v0)) {
            return false;
        }
    }
    return true;
#endif
    if (getCongruentClassSize(D) != 1 ||
        getCongruentClassSize(S) != 1 ||
        (m_WIA && m_WIA->whichDepend(D) != m_WIA->whichDepend(S))) {
        return false;
    }
    return true;
}

bool VariableReuseAnalysis::IsExtractFrom(
    VecEltTy& AllElts, InsertElementInst* FirstIEI,
    InsertElementInst* LastIEI, SSubVector& SV)
{
    int nelts = (int)AllElts.size();
    Value* BaseVec = AllElts[0].Vec;
    int BaseStartIx = AllElts[0].EltIx;
    VectorType *BVTy = dyn_cast<VectorType>(BaseVec->getType());
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
    InsertElementInst* LastIEI, SmallVector<SSubVector, 4>& SVs)
{
    int nelts = (int)AllElts.size();
    Value* SubVec = AllElts[0].Vec;
    int SubStartIx = 0;

    for (int i = 0; i < nelts; ++i)
    {
        // 1. AllElts[i].Vec must be SubVec.
        // 2. Check the next SubVec, if it change, the current
        //    element is the last one of the crrent SubVec;
        //    and SSubVector will be created if the SubVec meets
        //    the condition.
        if ((i - SubStartIx) != AllElts[i].EltIx) {
            return false;
        }

        Value *NextSub = (i < (nelts - 1)) ? AllElts[i+1].Vec : nullptr;
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
            VectorType *BVTy = dyn_cast<VectorType>(SubVec->getType());
            int sub_nelts = BVTy ? (int)BVTy->getNumElements() : 1;
            // If Sub's size is not smaller than IEI's, or not all sub's
            // elements are used, skip.
            if (sub_nelts >= nelts || (i - SubStartIx) != (sub_nelts - 1)) {
                return false;
            }

            SSubVector sv;
            sv.BaseVector = SubVec;  // note that this is sub, not base!
            sv.StartElementOffset = SubStartIx;
            SVs.push_back(sv);

            SubVec = NextSub;
            SubStartIx = i+1;
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

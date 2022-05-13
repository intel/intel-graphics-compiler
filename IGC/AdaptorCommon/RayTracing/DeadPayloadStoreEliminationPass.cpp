/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass eliminates redundant payload stores in AHS (AnyHit Shader) which will be overwritten by CHS (ClosetHit Shader).
/// To simplify the problem, right now, we only handle the case where, at compile time, we know AHS is followed by CHS immediately
/// (Theoretically, we might be able to enhance this pass to handle other cases as well which needs further analysis of IS/AHS).
/// Given this kind of case, we only need to find RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH or AcceptHitAndEndSearch().
/// To find above two flags/intrinsic, we do some analysis in LowerIntersectionAnyHit
/// and mark the BTD_CHS intrinsic with func metadata: "btd.target"(CHS_FuncName).
/// And then, find any store which is
/// 1) this store is post-dominated by "btd.target" inst.
/// 2) this store doesn't alias with any load which are between this store and "btd.target" inst.
/// 3) this store alias with a store in CHS.
/// 4) the store in CHS is not preceded by a load which alias it.
/// One example is like this:
///-------------------------------------------------
/// void MyRaygenShader(){
/// ...
/// TraceRay(AccelerationStructure, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, payload);
/// }
///
/// void MyAnyHitShader(...){
/// payload.value1 = 100;       //AHS.SI_0. Not dead because AHS.LI_1 will use it.
/// if (payload.test > 1)       //AHS.LI_0.
///     payload.value2 += payload.value1; //AHS.SI_1, AHS.LI_1. AHS.SI_1 is dead.
/// payload.value2 += 101;   //AHS.SI_2. Dead (will be overwriten by CHS.SI_1)
/// }
///
/// void MyClosestHitShader(...){
/// payload.value1 = 110; //CHS.SI_0
/// payload.value2 = 111; //CHS.SI_1
/// }
///-------------------------------------------------
/// Besides, we need to SimplifyCFG(AHS) in this pass because, under this case, LowerIntersectionAnyHit's output looks like below IR
/// where AHS.BTD_CHS intrinsic doesn't PDT store. SimplifyCFG will merge BB0, BTDBB, BTDClosestHitBB.
/// BB0:
///     store ...., %payload    ;to be deleted by DPSE
///     br i1 true, label %BTDBB, label %CommitBB
/// BTDBB:
///     br i1 false, label% SkipClosestHit, label% BTDClosestHitBB
/// BTDClosestHitBB:
///     call void @"llvm.genx.GenISA.BindlessThreadDispatch... !AHS.BTD_CHS
/// SkipClosestHit:
/// CommitBB:
///     call void @"llvm.genx.GenISA.TraceRayAsync...
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "StackFrameInfo.h"
#include "ContinuationUtils.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "iStdLib/utility.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Transforms/Scalar.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "Utils.h"

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

class DeadPayloadStoreEliminationPass : public ModulePass
{
public:
    DeadPayloadStoreEliminationPass()
        : ModulePass(ID),
          m_module(nullptr)
    {
        initializeDeadPayloadStoreEliminationPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "DeadPayloadStoreEliminationPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
private:
    void GetAHS_CHPair(vector<pair<Function*, Function*>>& AHS_CHS_pairs);
    bool DeleteDeadPayloadStores(pair<Function*, Function*>& AHS_CHS_pair);
    void CollectAnyHitPayloadCandidates(
        const pair<Function*, Function*>& AHS_CHS_pair,
        SmallVectorImpl<PayloadUse>& AHS_PayloadStores,
        SmallVectorImpl<PayloadUse>& CHS_PayloadStores);
    void FilterAHSPayloadCandidates(
        Function* AHS_F,
        StringRef CHS_FName,
        SmallVectorImpl<PayloadUse>& AHS_PayloadUses,
        SmallVectorImpl<PayloadUse>& AHS_PayloadStores);
    void FilterCHSPayloadCandidates(
        Function* CHS_F,
        SmallVectorImpl<PayloadUse>& CHS_PayloadUses,
        SmallVectorImpl<PayloadUse>& CHS_PayloadStores);

    Module* m_module;
    RayDispatchShaderContext *m_CGCtx = nullptr;
};

char DeadPayloadStoreEliminationPass::ID = 0;

#define PASS_FLAG "Dead-PayloadStore-Elimination"
#define PASS_DESCRIPTION "Deleate dead payloadstore in AnyHitShader"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DeadPayloadStoreEliminationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(DeadPayloadStoreEliminationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool DeadPayloadStoreEliminationPass::runOnModule(Module &M)
{
    m_module = &M;
    m_CGCtx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
    vector<pair<Function*, Function*>> AHS_CHS_pairs;
    GetAHS_CHPair(AHS_CHS_pairs);
    if (AHS_CHS_pairs.empty())
        return false;

    bool changed = false;
    for (auto& AHS_CHS_pair : AHS_CHS_pairs)
    {
        bool ret = DeleteDeadPayloadStores(AHS_CHS_pair);
        changed = ret || changed;
    }

    return changed;
}

void DeadPayloadStoreEliminationPass::GetAHS_CHPair(vector<pair<Function*, Function*>>& AHS_CHS_pairs)
{
    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto& FuncMD = modMD->FuncMD;

    std::unordered_map<string, Function*> AHS_Fs;
    std::unordered_map<string, Function*> CHS_Fs;
    for (auto& F : *m_module)
    {
        if (F.isDeclaration())
            continue;
        auto MD = FuncMD.find(&F);
        if (MD == FuncMD.end())
            continue;
        auto CallableShaderType = MD->second.rtInfo.callableShaderType;
        if (CallableShaderType == ClosestHit)
        {
            CHS_Fs[F.getName().str()] = &F;
        }
        else if (CallableShaderType == AnyHit)
        {
            AHS_Fs[F.getName().str()] = &F;
        }
    }

    if (CHS_Fs.empty() || AHS_Fs.empty())
    {
        return;
    }

    //AHS and CHS could be reused in multi HitGroups, and we call the relationship to be: (m)AHSes TO (n)CHSes.
    //which means, we might have m AHS shaders, and n CHS shaders used in all HitGroups.
    //Two cases as examples:
    //Case 1: m = 2, n = 1
    //which means, HitGroup0 has AHS0, CHS0; HitGroup1 has AHS1, CHS0;
    //Case 2: m = 1, n = 2
    //which means, HitGroup0 has AHS0, CHS0; HitGroup1 has AHS0, CHS1;
    //To simplify problem, we support only case 1 above because for case 2, to delete any stores in AHS0, we have to check both CHS0 and CHS1.
    //Note that this doesn't mean AHS has to be in one single HitGroup because we could have cases like below. So, we DO need to traverse all hitgroups.
    //HitGroup0 has AHS0, CHS0; HitGroup1 has AHS0, has no CHS; HitGroup2 has IS0, AHS0, CHS0.
    //FPM below is used to simplifyCFG as mentioned above in this file.
    legacy::FunctionPassManager FPM(m_module);
    FPM.add(createCFGSimplificationPass());
    for (auto& pair : AHS_Fs)
    {
        const string& AHS_FName = pair.first;
        llvm::Optional<std::string> CHS_FName;
        if (auto* Refs = m_CGCtx->hitgroupRefs(AHS_FName))
        {
            auto LeaderCHS = (*Refs)[0]->ClosestHit;
            if (llvm::all_of(*Refs, [&](HitGroupInfo* H) { return LeaderCHS == H->ClosestHit; }))
                CHS_FName = LeaderCHS;
        }
        if (CHS_FName)
        {
            Function* AHS_F = AHS_Fs[AHS_FName];
            FPM.run(*AHS_F);
            AHS_CHS_pairs.push_back({ AHS_F, CHS_Fs[*CHS_FName]});
        }
    }
}

void DeadPayloadStoreEliminationPass::FilterAHSPayloadCandidates(
    Function* AHS_F,
    StringRef CHS_FName,
    SmallVectorImpl<PayloadUse>& AHS_PayloadUses,
    SmallVectorImpl<PayloadUse>& AHS_PayloadStores)
{
    PostDominatorTree PDT(*AHS_F);
    for (auto& I : instructions(*AHS_F))
    {
        if (auto* GII = dyn_cast<GenIntrinsicInst>(&I))
        {
            if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_BindlessThreadDispatch)
            {
                if (MDNode* node = GII->getMetadata(RTBuilder::BTDTarget))
                {
                    auto metaFName =
                        cast<MDString>(node->getOperand(0).get())->getString();
                    if (!metaFName.equals(CHS_FName))
                    {
                        return;
                    }

                    for (auto& PUS : AHS_PayloadUses)
                    {
                        if (PUS.I->getOpcode() == Instruction::Store &&
                            PDT_dominates(PDT, GII, PUS.I))
                        {
                            if (llvm::any_of(AHS_PayloadUses, [&](auto& PUL) {
                                //find a load which is post dominated by btd.target(CHS)
                                //          and this load alias with current store
                                //if we can find it, that means, this store might not be a candidate
                                bool ret = (PUL.I->getOpcode() == Instruction::Load &&
                                    PDT_dominates(PDT, GII, PUL.I) &&
                                    Intervals::overlap(PUS.MemInterval, PUL.MemInterval)
                                    );
                                return ret;
                                }))
                            {
                                continue;
                            }
                            AHS_PayloadStores.push_back(PUS);
                        }
                    }
                }
            }
        }
    }
}

//TODO: In case multi AHS pair with single CHS, we might want to filterCHS only once.
void DeadPayloadStoreEliminationPass::FilterCHSPayloadCandidates(
    Function* CHS_F,
    SmallVectorImpl<PayloadUse>& CHS_PayloadUses,
    SmallVectorImpl<PayloadUse>& CHS_PayloadStores)
{
    DominatorTree DT(*CHS_F);
    PostDominatorTree PDT(*CHS_F);
    SmallVector<PayloadUse, 4> CHS_PayloadLoads;
    for (auto& PU : CHS_PayloadUses)
    {
        if (PU.I->getOpcode() == Instruction::Load)
            CHS_PayloadLoads.push_back(PU);
    }
    if (CHS_PayloadLoads.empty()) {
        CHS_PayloadStores = CHS_PayloadUses;
        return;
    }

    for (auto& PUS : CHS_PayloadUses)
    {
        if (PUS.I->getOpcode() == Instruction::Store &&
            PDT.dominates(PUS.I->getParent(), &CHS_F->getEntryBlock())) {
            if (llvm::any_of(CHS_PayloadLoads, [&](PayloadUse& PUL) {
                //find a load which is not dominated by current store
                //          and this load alias with current store
                //if we can find it, that means, this store is not a candidate
                bool ret = (!DT.dominates(PUS.I, PUL.I) &&
                    Intervals::overlap(PUS.MemInterval, PUL.MemInterval));
                return ret;
            }))
            {
                continue;
            }
            CHS_PayloadStores.push_back(PUS);
        }
    }
}

void DeadPayloadStoreEliminationPass::CollectAnyHitPayloadCandidates(
    const pair<Function*, Function*>& AHS_CHS_pair,
    SmallVectorImpl<PayloadUse>& AHS_PayloadStores,
    SmallVectorImpl<PayloadUse>& CHS_PayloadStores)
{
    Function* AHS_F = AHS_CHS_pair.first;
    Function* CHS_F = AHS_CHS_pair.second;
    ArgQuery AHS_ArgQ(*AHS_F, *m_CGCtx), CHS_ArgQ(*CHS_F, *m_CGCtx);

    Argument* AHS_Arg = AHS_ArgQ.getPayloadArg(AHS_F);
    Argument* CHS_Arg = CHS_ArgQ.getPayloadArg(CHS_F);
    if (!AHS_Arg || !CHS_Arg)
        return;

    auto& DL = AHS_F->getParent()->getDataLayout();
    SmallVector<PayloadUse, 4> AHS_PayloadUses, CHS_PayloadUses;
    bool found = collectAnalyzablePayloadUses(AHS_Arg, DL, AHS_PayloadUses, 0);
    if (!found || AHS_PayloadUses.empty())
        return;

    found = collectAnalyzablePayloadUses(CHS_Arg, DL, CHS_PayloadUses, 0);
    if (!found || CHS_PayloadUses.empty())
        return;

    FilterAHSPayloadCandidates(AHS_F, CHS_F->getName(), AHS_PayloadUses, AHS_PayloadStores);
    if (AHS_PayloadStores.empty())
        return;
    FilterCHSPayloadCandidates(CHS_F, CHS_PayloadUses, CHS_PayloadStores);
}

bool DeadPayloadStoreEliminationPass::DeleteDeadPayloadStores(pair<Function*, Function*>& AHS_CHS_pair)
{
    SmallVector<PayloadUse, 4> AHS_PayloadStores, CHS_PayloadStores;
    CollectAnyHitPayloadCandidates(AHS_CHS_pair, AHS_PayloadStores, CHS_PayloadStores);

    bool ret = false;
    SmallVector<PayloadUse, 4> Candidate_PayloadUses;
    for (auto& Any_PU : AHS_PayloadStores) {
        if (llvm::any_of(CHS_PayloadStores, [&](PayloadUse& CHS_PU) {
            return fully_overlap(CHS_PU.MemInterval, Any_PU.MemInterval);
            }))
        {
            Candidate_PayloadUses.push_back(Any_PU);
        }
    }
    for (auto& candidatePU : Candidate_PayloadUses)
    {
        candidatePU.I->eraseFromParent();
        ret = true;
    }

    return ret;
}

namespace IGC
{

Pass* createDeadPayloadStoreEliminationPass(void)
{
    return new DeadPayloadStoreEliminationPass();
}

} // namespace IGC

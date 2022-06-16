/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass splits shaders on continuation based calls (currently, TraceRay()
/// and CallShader()).  the TraceRay message without the sync bit set is
/// asynchronous: the shader does *not* block waiting for the return from the
/// send.rta.  This is in contrast to how it is used from an application
/// perspective:
///
/// [shader("raygeneration")]
/// void MyRaygenShader()
/// {
///     // do stuff before
///     TraceRay(..., payload);
///     RenderTarget[DispatchRaysIndex().xy] = payload.color;
/// }
///
/// The application invokes the trace which could involve an arbitrary depth
/// of further recursively traced rays (e.g., for reflection, refraction, or
/// shadow rays) up to maxTraceRecursionDepth.  The expectation is that the
/// payload can be read having incorporated all the processing from the ray
/// tree.
///
/// After this pass, 'MyRaygenShader' would look like:
///
/// [shader("raygeneration")]
/// void MyRaygenShader()
/// {
///     // do stuff before
///     spill(RTStack, payload);
///     TraceRay(..., payload);
///     return;
/// }
///
/// [shader("raygeneration")]
/// void __Continuation_0_of_MyRaygenShader()
/// {
///     payload = fill(RTStack);
///     RenderTarget[DispatchRaysIndex().xy] = payload.color;
/// }
///
/// Given that the payload is live across the TraceRay(), we need to spill it
/// to the RTStack and refill it in the continuation so we can pick up right
/// where we left off where the TraceRay() was called.
///
/// The continuation would later be invoked without UMD intervention by the
/// BTD unit.
///
/// Note that a shader can have any number of TraceRay() calls so it could be
/// carved up into an arbitrary number of continuations.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "RTArgs.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMUtils.h"
#include "CrossingAnalysis.h"
#include "SplitAsyncUtils.h"
#include "FuseContinuations.h"

#include <vector>
#include <set>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/ValueHandle.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/SSAUpdaterBulk.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

class SplitAsyncPass : public ModulePass
{
public:
    SplitAsyncPass() : ModulePass(ID)
    {
        initializeSplitAsyncPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "SplitAsyncPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
private:
    Module* m_module = nullptr;
    RayDispatchShaderContext *m_CGCtx = nullptr;

    struct FuncInfo
    {
        SmallVector<ContinuationHLIntrinsic*, 4> ContinuationPoints;
        SmallVector<AllocaInst*, 4> Allocas;
        MapVector<uint64_t, SmallVector<SpillValueIntrinsic*, 2>> Spills;
        MapVector<uint64_t, SmallVector<FillValueIntrinsic*, 2>> Fills;
        std::unique_ptr<DominatorTree> DT;
        Optional<uint32_t> ContinuationID;
    };
private:
    void markAsContinuationFunction(Function *F, CallableShaderTypeMD Ty) const
    {
        ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
        IGC::FunctionMetaData funcMd;
        funcMd.functionType = FunctionTypeMD::CallableShader;
        funcMd.rtInfo.callableShaderType = Ty;
        funcMd.rtInfo.isContinuation = true;
        modMD->FuncMD.insert(std::make_pair(F, funcMd));

        IGC::IGCMD::IGCMetaDataHelper::addFunction(
            *m_CGCtx->getMetaDataUtils(),
            F,
            FunctionTypeMD::KernelFunction);
    }

    MapVector<Function*, FuncInfo> processShader(
        Function& F,
        uint32_t& CurContinuationID);

    void injectSpills(
        Function& F,
        const std::vector<Instruction*>& SplitPoints) const;

    void injectSpills(
        ArrayRef<ContinuationHLIntrinsic*> ContinuationPoints) const;

    template <typename Fn>
    void splitShader(
        ArrayRef<ContinuationHLIntrinsic*> ContinuationPoints,
        uint32_t& CurContinuationID,
        Fn Notify) const;

    template <typename FnFunc, typename FnInst>
    bool setup(
        Function& F,
        uint32_t& CurContinuationID,
        FnFunc VisitFunc,
        FnInst VisitInst) const;

    void compactifySpills(MapVector<Function*, FuncInfo>& Info) const;
    void nonCompactifiedSpills(MapVector<Function*, FuncInfo>& Info) const;
    void hoistRematInstructions(MapVector<Function*, FuncInfo>& Info) const;

    SpillValueIntrinsic* updateOffset(
        RTBuilder& RTB,
        SpillValueIntrinsic* SI,
        uint64_t Offset) const;

    FillValueIntrinsic* updateOffset(
        RTBuilder& RTB,
        FillValueIntrinsic* FI,
        uint64_t Offset) const;

    void updateOffsets(RTBuilder& RTB, BasicBlock& BB) const;

    void remat(Function& F, SuspendCrossingInfo& Checker) const;

    void rewriteMaterializableInstructions(
        const std::vector<Instruction*>& Users,
        const std::vector<Instruction*>& InstSeq) const;

    static constexpr char* RematTrackingDefMD = "val.tracker.def";
    static constexpr char* RematTrackingCloneMD = "val.tracker.clone";

    void setDefTrackingNumber(Instruction& I, uint32_t Num) const;
    void setCloneTrackingNumber(Instruction& I, uint32_t Num) const;
    uint32_t getTrackingNumber(const MDNode* MD) const;
};

char SplitAsyncPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "split-async-new"
#define PASS_DESCRIPTION "Decompose shader into continuations split on async calls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitAsyncPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SplitAsyncPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

void SplitAsyncPass::rewriteMaterializableInstructions(
    const std::vector<Instruction*>& Users,
    const std::vector<Instruction*>& InstSeq) const
{
    BasicBlock* CurrentBlock = nullptr;
    Instruction* CurrentMaterialization = nullptr;
    Instruction* const CurrentDef = InstSeq[InstSeq.size() - 1];

    for (auto *User : Users)
    {
        // If we have not seen this block, materialize the value.
        if (CurrentBlock != User->getParent())
        {
            CurrentBlock = User->getParent();
            auto* InsertPt = &*CurrentBlock->getFirstInsertionPt();

            Instruction* NewI = nullptr;
            ValueToValueMapTy VM;
            for (auto* I : InstSeq)
            {
                NewI = I->clone();
                NewI->setName(I->getName());
                NewI->insertBefore(InsertPt);
                if (auto *MD = NewI->getMetadata(RematTrackingDefMD))
                {
                    uint32_t DefNum = getTrackingNumber(MD);
                    NewI->setMetadata(RematTrackingDefMD, nullptr);
                    setCloneTrackingNumber(*NewI, DefNum);
                }

                RemapInstruction(NewI, VM,
                    RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

                VM[I] = NewI;
            }

            CurrentMaterialization = NewI;
        }

        if (auto* PN = dyn_cast<PHINode>(User))
        {
            IGC_ASSERT_MESSAGE(PN->getNumIncomingValues() == 1,
                "unexpected number of incoming values in the PHINode");
            PN->replaceAllUsesWith(CurrentMaterialization);
            PN->eraseFromParent();
            continue;
        }

        // Replace all uses of CurrentDef in the current instruction with the
        // CurrentMaterialization for the block.
        User->replaceUsesOfWith(CurrentDef, CurrentMaterialization);
    }
}

void SplitAsyncPass::remat(Function& F, SuspendCrossingInfo& Checker) const
{
    std::vector<std::pair<WeakTrackingVH, std::vector<Instruction*>>> DefToUsers;
    for (Instruction& I : instructions(F))
    {
        std::vector<Instruction*> Users;
        for (User* U : I.users())
        {
            if (Checker.isDefinitionAcrossSuspend(I, U))
                Users.push_back(cast<Instruction>(U));
        }

        if (!Users.empty())
            DefToUsers.push_back(std::make_pair(&I, Users));
    }

    uint32_t ValID = 0;
    const bool MarkInstructions = m_CGCtx->opts().HoistRemat;
    const uint32_t Threshold    = m_CGCtx->opts().RematThreshold;
    RematChecker RMChecker{ *m_CGCtx, RematStage::MID };
    for (auto& [VH, Users] : DefToUsers)
    {
        auto* I = cast<Instruction>(VH);
        auto Insts = RMChecker.canFullyRemat(I, Threshold);
        if (!Insts)
            continue;

        auto& Seq = *Insts;
        if (MarkInstructions)
        {
            for (auto* NewI : Seq)
            {
                if (NewI->getMetadata(RematTrackingDefMD))
                    continue;

                setDefTrackingNumber(*NewI, ValID++);
            }
        }

        rewriteMaterializableInstructions(Users, Seq);
    }
}

void SplitAsyncPass::setCloneTrackingNumber(Instruction& I, uint32_t Num) const
{
    auto& C = I.getContext();
    MDNode* node = MDNode::get(
        C,
        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), Num)));
    I.setMetadata(RematTrackingCloneMD, node);
}

void SplitAsyncPass::setDefTrackingNumber(Instruction& I, uint32_t Num) const
{
    auto& C = I.getContext();
    MDNode* node = MDNode::get(
        C,
        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), Num)));
    I.setMetadata(RematTrackingDefMD, node);
}

uint32_t SplitAsyncPass::getTrackingNumber(const MDNode* MD) const
{
    auto* CMD = cast<ConstantAsMetadata>(MD->getOperand(0));
    auto* C = cast<ConstantInt>(CMD->getValue());
    return static_cast<uint32_t>(C->getZExtValue());
}

void SplitAsyncPass::injectSpills(
    Function& F, const std::vector<Instruction*>& SplitPoints) const
{
    rewritePHIs(F);

    SuspendCrossingInfo Checker(F, SplitPoints);

    remat(F, Checker);

    SmallVector<Spill, 8> Spills;
    for (Instruction& I : instructions(F))
    {
        for (User* U : I.users())
        {
            if (Checker.isDefinitionAcrossSuspend(I, U))
            {
                Spills.emplace_back(&I, U);
            }
        }
    }

    insertSpills(m_CGCtx, F, Spills);
}

void SplitAsyncPass::injectSpills(
    ArrayRef<ContinuationHLIntrinsic*> ContinuationPoints) const
{
    if (ContinuationPoints.empty())
        return;

    auto& F = *ContinuationPoints[0]->getFunction();
    auto& C = F.getContext();
    // make a temporary "split point" that will immediately follow
    // TraceRay(). If we were to split on TraceRay() calls themselves, we
    // would end up spilling if the TraceRay() used a given definition. That
    // is, `isDefinitionAcrossSuspend` also *includes* the suspend point
    // itself. In reality, we're really suspending execution right after the
    // lowering of the TraceRay().
    auto* FnTy = FunctionType::get(Type::getVoidTy(C), false);
    auto* SplitFunc = Function::Create(
        FnTy,
        GlobalValue::InternalLinkage,
        "",
        *F.getParent());

    IRBuilder<> IRB(C);
    std::vector<Instruction*> SplitPoints;
    for (auto* CHLI : ContinuationPoints)
    {
        IRB.SetInsertPoint(CHLI->getNextNode());
        auto* CI = IRB.CreateCall(SplitFunc);
        SplitPoints.push_back(CI);
        splitAround(CI, VALUE_NAME("Cont"));
    }

    injectSpills(F, SplitPoints);

    for (auto* SPI : SplitPoints)
        SPI->eraseFromParent();
    SplitFunc->eraseFromParent();
}

template <typename Fn>
void SplitAsyncPass::splitShader(
    ArrayRef<ContinuationHLIntrinsic*> ContinuationPoints,
    uint32_t& CurContinuationID,
    Fn Notify) const
{
    if (ContinuationPoints.empty())
        return;

    auto& F = *ContinuationPoints[0]->getFunction();
    auto& C = F.getContext();

    IRBuilder IRB(C);
    SmallVector<BasicBlock*, 4> NextBlocks;
    // Now, we need to split the shader into continuations. Steps:
    for (auto* CHLI : ContinuationPoints)
    {
        auto* CurBB = CHLI->getParent();
        NextBlocks.push_back(CurBB->getUniqueSuccessor());
        // 1. Give every TraceRay() a unique ID
        CHLI->setContinuationID(CurContinuationID++);
        // 2. Inject a return after each one
        auto* TI = CurBB->getTerminator();
        TI->eraseFromParent();
        IRB.SetInsertPoint(CurBB);
        IRB.CreateRetVoid();
    }
    // 3. Create a continuation starting at each one
    for (uint32_t i = 0; i < ContinuationPoints.size(); i++)
    {
        auto* CHLI = ContinuationPoints[i];
        auto* NextBB = NextBlocks[i];

        uint32_t CurContID = CHLI->getContinuationID();

        ValueToValueMapTy VMap;
        Function* Continuation = CloneFunction(&F, VMap);
        Continuation->setName(
            "__Continuation_" + Twine(CurContID) + "_of_" +
            F.getName());

        auto* CloneCLHI = cast<ContinuationHLIntrinsic>(VMap.find(CHLI)->second);
        Notify(CloneCLHI);

        auto* ContEntryBB = cast<BasicBlock>(VMap.find(NextBB)->second);
        ContEntryBB->moveBefore(&Continuation->getEntryBlock());

        llvm::removeUnreachableBlocks(*Continuation);
    }
    llvm::removeUnreachableBlocks(F);
}

template <typename FnFunc, typename FnInst>
bool SplitAsyncPass::setup(
    Function& F,
    uint32_t& CurContinuationID,
    FnFunc VisitFunc,
    FnInst VisitInst) const
{
    std::vector<ContinuationHLIntrinsic*> ContinuationPoints;

    for (auto& I : instructions(F))
    {
        if (auto* CHLI = dyn_cast<ContinuationHLIntrinsic>(&I))
            ContinuationPoints.push_back(CHLI);
    }

    if (ContinuationPoints.empty())
        return false;

    injectSpills(ContinuationPoints);

    MapVector<uint32_t, Function*> IdxContMap;
    splitShader(ContinuationPoints, CurContinuationID,
        [&](ContinuationHLIntrinsic* CHLI) {
            uint32_t CurContID = CHLI->getContinuationID();
            auto* Continuation = CHLI->getFunction();
            IdxContMap[CurContID] = Continuation;
        });

    std::vector<Function*> Shaders{ &F };
    for (auto& [_, Cont] : IdxContMap)
        Shaders.push_back(Cont);

    auto updateContFn = [&](ContinuationHLIntrinsic* CHLI)
    {
        uint32_t ContID = CHLI->getContinuationID();
        auto I = IdxContMap.find(ContID);
        IGC_ASSERT(I != IdxContMap.end());
        CHLI->setContinuationFn(I->second);
    };

    for (auto* Shader : Shaders)
    {
        VisitFunc(Shader);
        for (auto& I : instructions(*Shader))
        {
            // 4. update the continuation argument on each TraceRay()
            if (auto* CHLI = dyn_cast<ContinuationHLIntrinsic>(&I))
                updateContFn(CHLI);

            VisitInst(&I);
        }
    }

    return true;
}

MapVector<Function*, SplitAsyncPass::FuncInfo>
SplitAsyncPass::processShader(
    Function& F,
    uint32_t& CurContinuationID)
{
    MapVector<Function*, FuncInfo> Info;
    bool Changed = setup(F, CurContinuationID,
        [&](Function* Fn) {
            Info[Fn];
        },
        [&](Instruction* I) {
            if (auto* CHLI = dyn_cast<ContinuationHLIntrinsic>(I))
            {
                Info[I->getFunction()].ContinuationPoints.push_back(CHLI);
                uint32_t ID = CHLI->getContinuationID();
                auto* ContFn = CHLI->getContinuationFn();
                Info[ContFn].ContinuationID = ID;
            }
            else if (auto* AI = dyn_cast<AllocaInst>(I))
                Info[I->getFunction()].Allocas.push_back(AI);
            else if (auto* SI = dyn_cast<SpillValueIntrinsic>(I))
                Info[I->getFunction()].Spills[SI->getOffset()].push_back(SI);
            else if (auto* FI = dyn_cast<FillValueIntrinsic>(I))
                Info[I->getFunction()].Fills[FI->getOffset()].push_back(FI);
        });

    if (!Changed)
        return Info;

    if (IGC_IS_FLAG_DISABLED(DisableCompactifySpills))
        compactifySpills(Info);
    else
        nonCompactifiedSpills(Info);

    if (m_CGCtx->opts().HoistRemat)
        hoistRematInstructions(Info);

    return Info;
}

void SplitAsyncPass::hoistRematInstructions(
    MapVector<Function*, FuncInfo>& Info) const
{
    for (auto& [F, FI] : Info)
    {
        if (!FI.DT)
            FI.DT = std::make_unique<DominatorTree>(*F);

        auto* DT = FI.DT.get();

        std::unordered_set<Instruction*> ToDelete;
        std::unordered_map<uint32_t, Instruction*> Defs;
        SmallVector<Instruction*, 32> Clones;
        for (auto& I : instructions(*F))
        {
            if (auto* MD = I.getMetadata(RematTrackingDefMD))
                Defs[getTrackingNumber(MD)] = &I;
            if (I.getMetadata(RematTrackingCloneMD))
                Clones.push_back(&I);
        }

        auto* InsertPt = F->getEntryBlock().getTerminator();
        std::unordered_set<Instruction*> Lifted;
        BasicBlock* CurBB = nullptr;
        for (auto* I : Clones)
        {
            if (I->getParent() != CurBB)
            {
                CurBB = I->getParent();
                Lifted.clear();
            }

            auto* MD = I->getMetadata(RematTrackingCloneMD);
            uint32_t Num = getTrackingNumber(MD);
            auto DefI = Defs.find(Num);

            I->setMetadata(RematTrackingCloneMD, nullptr);

            if (DefI != Defs.end())
            {
                auto* Def = DefI->second;
                if (DT->dominates(Def, I))
                {
                    I->replaceAllUsesWith(Def);
                    ToDelete.insert(I);
                }
                else if (DT->dominates(I, Def))
                {
                    Def->replaceAllUsesWith(I);
                    ToDelete.insert(Def);
                }
            }
            else
            {
                bool CanLift = true;
                for (auto& Op : I->operands())
                {
                    auto* CurI = dyn_cast<Instruction>(Op);
                    if (!CurI)
                        continue;

                    if (Lifted.count(CurI) == 0)
                    {
                        IGC_ASSERT_MESSAGE(0, "what?");
                        CanLift = false;
                        break;
                    }
                }

                if (CanLift)
                {
                    I->moveBefore(InsertPt);
                    Lifted.insert(I);
                }
            }
        }

        for (auto& [_, I] : Defs)
            I->setMetadata(RematTrackingDefMD, nullptr);

        for (auto* I : ToDelete)
        {
            if (I->use_empty())
                I->eraseFromParent();
        }
    }
}

void SplitAsyncPass::compactifySpills(
    MapVector<Function*, FuncInfo>& Info) const
{
    if (Info.empty())
        return;

    auto* M = Info.begin()->first->getParent();
    auto& C = M->getContext();

    RTBuilder RTB(C, *m_CGCtx);

    // 1. add an entry block to place the fills
    for (auto& [F, FI] : Info)
    {
        auto& CurEntryBB = F->getEntryBlock();
        auto* NewEntryBB = BasicBlock::Create(
            C, VALUE_NAME("NewEntry"), F, &CurEntryBB);
        RTB.SetInsertPoint(NewEntryBB);
        auto *Br = RTB.CreateBr(&CurEntryBB);
        for (auto* AI : FI.Allocas)
            AI->moveBefore(Br);
    }

    // 2. Generate new fills
    for (auto& [F, FI] : Info)
    {
        for (auto& [_, Spills] : FI.Spills)
        {
            for (auto *SI : Spills)
                splitAround(SI, "");
        }
        RTB.SetInsertPoint(F->getEntryBlock().getTerminator());
        DenseMap<uint64_t, FillValueIntrinsic*> NewFills;
        SSAUpdaterBulk Updater;
        bool NeedsUpdate = false;
        for (auto& [Idx, Fills] : FI.Fills)
        {
            auto* Ty = Fills[0]->getType();
            auto* NewFill = RTB.getFillValue(Ty, Idx);
            NewFills[Idx] = NewFill;
        }
        for (auto& [Idx, Fills] : FI.Fills)
        {
            auto* NewFill = NewFills.find(Idx)->second;
            auto SpillI = FI.Spills.find(Idx);
            if (SpillI == FI.Spills.end())
            {
                for (auto* Fill : Fills)
                    Fill->replaceAllUsesWith(NewFill);
            }
            else
            {
                NeedsUpdate = true;
                unsigned VarHandle = Updater.AddVariable("", NewFill->getType());
                Updater.AddAvailableValue(
                    VarHandle, NewFill->getParent(), NewFill);
                auto& Spills = SpillI->second;
                for (auto* SI : Spills)
                {
                    Value* Val = SI->getData();
                    if (auto* FI = dyn_cast<FillValueIntrinsic>(Val))
                    {
                        Val = NewFills.find(FI->getOffset())->second;
                    }
                    Updater.AddAvailableValue(
                        VarHandle, SI->getParent(), Val);
                }

                for (auto* Fill : Fills)
                {
                    for (auto& Use : Fill->uses())
                        Updater.AddUse(VarHandle, &Use);
                }
            }
        }

        if (NeedsUpdate)
        {
            if (!FI.DT)
                FI.DT = std::make_unique<DominatorTree>(*F);
            Updater.RewriteAllUses(FI.DT.get());
        }

        for (auto& [Idx, Fills] : FI.Fills)
        {
            for (auto* Fill : Fills)
                Fill->eraseFromParent();
            Fills.clear();
            auto* NewFill = NewFills[Idx];
            if (!NewFill->use_empty())
                Fills.push_back(NewFill);
            else
                NewFill->eraseFromParent();
        }
        FI.Fills.remove_if([](auto& P) { return P.second.empty(); });
    }

    // 3. propagate fills to predecessors
    bool Changed = false;
    do {
        Changed = false;
        for (auto& [F, FI] : Info)
        {
            for (auto* CHLI : FI.ContinuationPoints)
            {
                auto* CurChild = CHLI->getContinuationFn();
                auto& ChildFI = Info.find(CurChild)->second;
                for (auto& [Idx, Fills] : ChildFI.Fills)
                {
                    if (FI.Spills.count(Idx) != 0)
                        continue;

                    if (FI.Fills.count(Idx) == 0)
                    {
                        auto* Ty = Fills[0]->getType();
                        RTB.SetInsertPoint(F->getEntryBlock().getTerminator());
                        FI.Fills[Idx].push_back(RTB.getFillValue(Ty, Idx));
                        Changed = true;
                    }
                }
            }
        }
    } while (Changed);

    for (auto& [F, FI] : Info)
    {
        SSAUpdaterBulk Updater;
        bool NeedsUpdate = false;
        for (auto* CHLI : FI.ContinuationPoints)
        {
            auto* CurChild = CHLI->getContinuationFn();
            auto& ChildFI = Info.find(CurChild)->second;
            for (auto& [Idx, _] : ChildFI.Fills)
            {
                auto SpillI = FI.Spills.find(Idx);
                auto FillI  = FI.Fills.find(Idx);
                if (SpillI != FI.Spills.end() && FillI != FI.Fills.end())
                {
                    NeedsUpdate = true;
                    auto& Fills = FillI->second;
                    auto& Spills = SpillI->second;
                    auto* Ty = Fills[0]->getType();
                    unsigned VarHandle = Updater.AddVariable("", Ty);

                    for (auto* Fill : Fills)
                    {
                        Updater.AddAvailableValue(
                            VarHandle, Fill->getParent(), Fill);
                    }

                    for (auto* SI : Spills)
                    {
                        Updater.AddAvailableValue(
                            VarHandle, SI->getParent(), SI->getData());

                        auto* NewSI = cast<SpillValueIntrinsic>(SI->clone());
                        NewSI->insertBefore(CHLI);
                        auto& Use = NewSI->getArgOperandUse(NewSI->getDataIdx());

                        Updater.AddUse(VarHandle, &Use);
                    }
                }
                else if (SpillI != FI.Spills.end())
                {
                    // just spill
                    IGC_ASSERT(SpillI->second.size() == 1);
                    auto* NewSI = SpillI->second[0]->clone();
                    NewSI->insertBefore(CHLI);
                }
                else if (FillI != FI.Fills.end())
                {
                    // just fill
                    auto* Val = FillI->second[0];
                    RTB.SetInsertPoint(CHLI);
                    RTB.getSpillValue(Val, Idx);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "shouldn't be possible!");
                }
            }
        }

        if (NeedsUpdate)
        {
            if (!FI.DT)
                FI.DT = std::make_unique<DominatorTree>(*F);
            Updater.RewriteAllUses(FI.DT.get());
        }

        for (auto* CHLI : FI.ContinuationPoints)
            updateOffsets(RTB, *CHLI->getParent());

        for (auto [_, Spills] : FI.Spills)
            Spills[0]->eraseFromParent();

        updateOffsets(RTB, F->getEntryBlock());
    }
}

SpillValueIntrinsic* SplitAsyncPass::updateOffset(
    RTBuilder& RTB,
    SpillValueIntrinsic* SI,
    uint64_t Offset) const
{
    auto* Ty = SI->getData()->getType();
    SpillValueIntrinsic* NewSI = nullptr;
    if (Ty->isIntegerTy() || Ty->isHalfTy())
    {
        if (Ty->getPrimitiveSizeInBits() < 32)
        {
            RTB.SetInsertPoint(SI);
            auto* NewVal = SI->getData();
            if (Ty->isHalfTy())
                NewVal = RTB.CreateBitCast(NewVal, RTB.getInt16Ty());
            NewVal =
                RTB.CreateZExt(NewVal, RTB.getInt32Ty());
            NewSI = RTB.getSpillValue(NewVal, SI->getOffset());
            SI->eraseFromParent();
            SI = NewSI;
        }
    }
    SI->setOffset(Offset);
    return NewSI;
}

FillValueIntrinsic* SplitAsyncPass::updateOffset(
    RTBuilder& RTB,
    FillValueIntrinsic* FI,
    uint64_t Offset) const
{
    auto* Ty = FI->getType();
    FillValueIntrinsic* NewFI = nullptr;
    if (Ty->isIntegerTy() || Ty->isHalfTy())
    {
        if (Ty->getPrimitiveSizeInBits() < 32)
        {
            RTB.SetInsertPoint(FI);
            NewFI = RTB.getFillValue(RTB.getInt32Ty(), FI->getOffset());
            Value* NewVal = nullptr;
            if (Ty->isHalfTy())
            {
                NewVal = RTB.CreateTrunc(NewFI, RTB.getInt16Ty());
                NewVal = RTB.CreateBitCast(NewVal, RTB.getHalfTy());
            }
            else
            {
                NewVal = RTB.CreateTrunc(NewFI, Ty);
            }
            FI->replaceAllUsesWith(NewVal);
            NewFI->takeName(FI);
            FI->eraseFromParent();
            FI = NewFI;
        }
    }
    FI->setOffset(Offset);
    return NewFI;
}

void SplitAsyncPass::updateOffsets(RTBuilder& RTB, BasicBlock& BB) const
{
    auto& DL = BB.getModule()->getDataLayout();
    uint64_t CurOffset = 0;

    for (auto II = BB.begin(), IE = BB.end(); II != IE; /* empty */)
    {
        auto* I = &*II++;
        if (auto* SI = dyn_cast<SpillValueIntrinsic>(I))
        {
            if (auto* NewSI = updateOffset(RTB, SI, CurOffset))
                SI = NewSI;
            CurOffset += DL.getTypeAllocSize(SI->getData()->getType());
        }
        else if (auto* FI = dyn_cast<FillValueIntrinsic>(I))
        {
            if (auto* NewFI = updateOffset(RTB, FI, CurOffset))
                FI = NewFI;
            CurOffset += DL.getTypeAllocSize(FI->getType());
        }
    }
}

void SplitAsyncPass::nonCompactifiedSpills(
    MapVector<Function*, FuncInfo>& Info) const
{
    if (Info.empty())
        return;

    std::map<uint64_t, std::vector<Instruction*>> IdxToInst;
    for (auto& [_, FnInfo] : Info)
    {
        for (auto& [_, Spills] : FnInfo.Spills)
        {
            for (auto *SI : Spills)
                IdxToInst[SI->getOffset()].push_back(SI);
        }

        for (auto& [_, Fills] : FnInfo.Fills)
        {
            for (auto *FI : Fills)
                IdxToInst[FI->getOffset()].push_back(FI);
        }
    }
    // 5. Update the spill/fill intrinsic indices to be byte offsets. If
    //    types are small integers, promote them to enable better vectorization.
    auto* M = Info.begin()->first->getParent();
    auto& DL = M->getDataLayout();
    uint64_t CurOffset = 0;
    RTBuilder IRB(M->getContext(), *m_CGCtx);
    for (auto &[_, Insts] : IdxToInst)
    {
        for (auto& I : Insts)
        {
            if (auto* SI = dyn_cast<SpillValueIntrinsic>(I))
            {
                if (auto* NewSI = updateOffset(IRB, SI, CurOffset))
                    I = NewSI;
            }
            else if (auto* FI = dyn_cast<FillValueIntrinsic>(I))
            {
                if (auto* NewFI = updateOffset(IRB, FI, CurOffset))
                    I = NewFI;
            }
        }
        uint64_t TypeSize = 0;
        if (auto* SI = dyn_cast<SpillValueIntrinsic>(Insts[0]))
            TypeSize = DL.getTypeAllocSize(SI->getData()->getType());
        else if (auto* FI = dyn_cast<FillValueIntrinsic>(Insts[0]))
            TypeSize = DL.getTypeAllocSize(FI->getType());
        else
            IGC_ASSERT_MESSAGE(0, "should be spill/fill only!");

        CurOffset += TypeSize;
    }
}

bool SplitAsyncPass::runOnModule(Module &M)
{
    m_module = &M;
    m_CGCtx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());

    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto& FuncMD = modMD->FuncMD;

    SmallVector<Function*, 4> Shaders;
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        Shaders.push_back(&F);
    }

    /// Map the continuation function to its associated info.
    MapVector<Function*, ContinuationFusing::FuncInfo> ContinuationMap;

    bool Changed = false;
    uint32_t CurContinuationID = 0;
    for (auto* F : Shaders)
    {
        auto MD = FuncMD.find(F);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");
        auto ShaderTy = MD->second.rtInfo.callableShaderType;

        auto Info = processShader(*F, CurContinuationID);
        Changed |= !Info.empty();

        for (auto& [Fn, FI] : Info)
        {
            if (Fn != F) // skip the root
                ContinuationMap[Fn] = { *FI.ContinuationID, F, ShaderTy };
        }
    }

    if (IGC_IS_FLAG_DISABLED(DisableFuseContinuations))
        fuseContinuations(M, ContinuationMap);

    // Set this prior to calling requiresIndirectContinuationHandling().
    modMD->rtInfo.NumContinuations = ContinuationMap.size();

    bool RequiresIndirect = m_CGCtx->requiresIndirectContinuationHandling();

    for (auto& [Cont, FI] : ContinuationMap)
    {
        Changed = true;
        if (RequiresIndirect)
        {
            markAsContinuationFunction(Cont, FI.ShaderTy);
        }
        else
        {
            Cont->setLinkage(GlobalValue::InternalLinkage);
            Cont->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
        }
        RTBuilder::markAsContinuation(*Cont);
    }

    DumpLLVMIR(m_CGCtx, "SplitAsyncPass");
    return Changed;
}

namespace IGC
{

Pass* createSplitAsyncPass(void)
{
    return new SplitAsyncPass();
}

} // namespace IGC

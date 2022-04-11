/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// After shader splitting, we have a collection of spill/fill intrinsics
/// that represent the live values across a continuation.
/// The goal of this pass is to do some analysis to determine cases where
/// spills can be rematerialized (recomputed) in the continuation so we don't
/// have to spill it.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "iStdLib/utility.h"
#include "common/LLVMUtils.h"
#include "ContinuationUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

using ComputeChain = SmallVector<Value*, 4>;

// Compare two values to see if they are equivalent.  We include a value map
// to morph the RHS to the left so isIdenticalTo() will return the right result.
static bool compareEntry(
    Value* LHS, Value* RHS, ValueToValueMapTy &VM)
{
    auto* LHSI = dyn_cast<Instruction>(LHS);
    auto* RHSI = dyn_cast<Instruction>(RHS);

    if (!LHSI || !RHSI)
        return false;

    if (LHSI->getOpcode() != RHSI->getOpcode())
        return false;

    VM[RHSI] = LHSI;

    if (isa<AllocaInst>(LHSI))
    {
        if (LHSI == RHSI)
            return true;
        else
            return false;
    }

    RemapInstruction(RHSI, VM, RF_NoModuleLevelChanges);

    return LHSI->isIdenticalTo(RHSI);
}

// Walk both chains to check if they are equivalent.
static bool areChainsEqual(ArrayRef<Value*> LHS, ArrayRef<Value*> RHS)
{
    if (LHS.size() != RHS.size())
        return false;

    ValueToValueMapTy VM;

    for (uint32_t i = 0; i < LHS.size(); i++)
    {
        if (!compareEntry(LHS[i], RHS[i], VM))
            return false;
    }

    return true;
}

class LateRematPass : public ModulePass
{
public:
    LateRematPass() : ModulePass(ID)
    {
        initializeLateRematPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &F) override;
    StringRef getPassName() const override
    {
        return "LateRematPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
private:
    using MkComputeChainFunc =
        std::function<Optional<ComputeChain>(SpillValueIntrinsic*)>;
    using Thunk = std::function<void()>;
    bool Changed;
    Optional<ComputeChain> getAddressComputation(
        ContinuationInfo &CI,
        Value* V);
    Optional<ComputeChain> unify(
        ArrayRef<SpillValueIntrinsic*> Spills,
        MkComputeChainFunc fn);
    void remat(
        ArrayRef<Value*> Chain,
        ValueToValueMapTy& VM,
        FillValueIntrinsic* FI);
    SmallVector<Thunk, 4> Thunks;
    bool tryRematAllocas(
        ContinuationInfo &CI,
        const DenseMap<const AllocaInst*, uint32_t> &AllocaMap,
        FillValueIntrinsic* Fill,
        ContinuationInfo::SpillColl& Spills);
    bool tryMemRayOpt(
        FillValueIntrinsic* Fill,
        ContinuationInfo::SpillColl& Spills);
    bool tryRematLocalPointer(
        ContinuationInfo& CI,
        FillValueIntrinsic* Fill,
        ContinuationInfo::SpillColl& Spills);
    RayDispatchShaderContext* m_CGCtx = nullptr;
    void shrinkSpillFill(
        const DataLayout &DL,
        ContinuationInfo& ContInfo,
        bool Sort);
    bool rearrange(ContinuationInfo& ContInfo);
    std::vector<uint32_t> computeNewOrder(const BasicBlock& BB) const;
    void removeDeadSpills(
        BasicBlock& BB, const std::vector<uint32_t> &Order, bool Remove) const;
    void rearrangeFills(
        BasicBlock& BB, const std::vector<uint32_t> &Order) const;
    static bool justRespill(const SpillValueIntrinsic* SI);
};

char LateRematPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "late-remat"
#define PASS_DESCRIPTION "Do more involved remat after splitting"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LateRematPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LateRematPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool LateRematPass::runOnModule(Module &M)
{
    m_CGCtx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
    Changed = false;

    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto &FuncMD = modMD->FuncMD;

    auto RootFuncs = getRootFunctions(m_CGCtx, M);

    const bool CanPromoteContinuations =
        m_CGCtx->requiresIndirectContinuationHandling() &&
        m_CGCtx->m_DriverInfo.supportsRaytracingContinuationPromotion();

    for (auto* Root : RootFuncs)
    {
        auto MD = FuncMD.find(Root);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");
        auto ShaderTy = MD->second.rtInfo.callableShaderType;

        Thunks.clear();

        ContinuationInfo ContInfo{ ShaderTy };
        // We calculate info about spills/fills here to be queried by the below
        // loop.  We build up a collection of functions at the end to actually
        // execute the transformations so we don't have to update the internal
        // data structures here.
        ContMap Group = getFuncGroup(Root);
        ContInfo.calculate(*Root, Group);
        // We expect that allocas will only be in the root function.  If there
        // was a need between shader splitting and late remat to add allocas
        // in the continuations then we need to add those here as well.
        auto AllocaMap = RTBuilder::getAllocaNumberMap(*Root);

        bool DidPromote = false;

        for (auto& [Fill, Spills] : ContInfo.spillfills())
        {
            if (tryRematAllocas(ContInfo, AllocaMap, Fill, Spills))
                Changed = true;
            else if (tryMemRayOpt(Fill, Spills))
                Changed = true;
            else if (CanPromoteContinuations &&
                     tryRematLocalPointer(ContInfo, Fill, Spills))
            {
                DidPromote = true;
                Changed = true;
            }
        }

        if (DidPromote)
        {
            uint32_t CurSlot = RTStackFormat::ShaderIdentifier::RaygenFirstOpenSlot;
            for (auto& [_, ContFn] : Group)
            {
                auto MD = FuncMD.find(ContFn);
                auto& rtInfo = MD->second.rtInfo;
                rtInfo.SlotNum = CurSlot++;
            }
        }

        // Now execute the actions at the end.
        for (auto Thunk : Thunks)
            Thunk();

        auto& DL = M.getDataLayout();
        bool ChangedOrder = false;
        if (IGC_IS_FLAG_DISABLED(DisableSpillReorder))
            ChangedOrder = rearrange(ContInfo);
        shrinkSpillFill(DL, ContInfo, !ChangedOrder);
    }

    DumpLLVMIR(m_CGCtx, "LateRematPass");
    return Changed;
}

template <typename T, typename FnTy>
auto static filter(BasicBlock* BB, FnTy Fn) -> SmallVector<T*, 8>
{
    SmallVector<T*, 8> Insts;
    for (auto& I : *BB)
    {
        if (auto* II = Fn(&I))
            Insts.push_back(II);
    }
    return Insts;
}

// Sort the spills/fills in descending order of size to ensure alignment for
// the larger types. For example, if SplitAsync generated code like:
//
// SpillValue(i1 %x, i64 0)
// SpillValue(i64 %y, i64 1)
//
// The %y spill would just be byte-aligned leading to suboptimal code.
// This will generate:
//
// SpillValue(i64 %y, i64 0)
// SpillValue(i1 %x, i64 8)
template<typename T, typename _ValFn>
static void
sortInstsBySize(const DataLayout &DL, SmallVectorImpl<T*> &Insts, _ValFn ValFn)
{
    if (!Insts.empty())
    {
        IRBuilder<> IRB(Insts[0]);
        bool SchedFill = isa<FillValueIntrinsic>(Insts[0]);

        // If we're scheduling fills, create a temporary anchor for the insert
        // point and delete at the end.
        Instruction* InsertPt = SchedFill ?
            IRB.CreateRetVoid() :
            Insts[Insts.size() - 1]->getNextNode();
        std::stable_sort(Insts.begin(), Insts.end(),
            [&](auto* A, auto* B) {
                return DL.getTypeAllocSize(ValFn(A)->getType()) >
                       DL.getTypeAllocSize(ValFn(B)->getType());
        });

        for (auto* I : Insts)
            I->moveBefore(InsertPt);

        if (SchedFill)
            InsertPt->eraseFromParent();
    }
}

// Is the data coming into the spill just a fill from the same memory location?
bool LateRematPass::justRespill(const SpillValueIntrinsic* SI)
{
    if (auto* FI = dyn_cast<FillValueIntrinsic>(SI->getData()))
        return (SI->getOffset() == FI->getOffset());

    return false;
}

void LateRematPass::rearrangeFills(
    BasicBlock& BB, const std::vector<uint32_t>& Order) const
{
    if (Order.empty())
        return;

    std::vector<FillValueIntrinsic*> CurFills;
    CurFills.reserve(Order.size());
    for (auto& I : BB)
    {
        if (auto* FI = dyn_cast<FillValueIntrinsic>(&I))
            CurFills.push_back(FI);
    }

    IGC_ASSERT_MESSAGE(Order.size() == CurFills.size(),
        "fills going to same continuation should match!");

    std::vector<FillValueIntrinsic*> Fills{ Order.size() };
    for (uint32_t i = 0; i < Order.size(); i++)
        Fills[i] = CurFills[Order[i]];

    // temporary anchor
    IRBuilder<> IRB(CurFills[0]);

    auto* InsertPt = IRB.CreateRetVoid();

    for (auto* Fill : Fills)
        Fill->moveBefore(InsertPt);

    InsertPt->eraseFromParent();
}

void LateRematPass::removeDeadSpills(
    BasicBlock& BB, const std::vector<uint32_t>& Order, bool Remove) const
{
    if (Order.empty())
        return;

    std::vector<SpillValueIntrinsic*> CurSpills;
    CurSpills.reserve(Order.size());
    for (auto& I : BB)
    {
        if (auto* SI = dyn_cast<SpillValueIntrinsic>(&I))
            CurSpills.push_back(SI);
    }

    IGC_ASSERT_MESSAGE(Order.size() == CurSpills.size(),
        "spills going to same continuation should match!");

    std::vector<SpillValueIntrinsic*> Spills{ Order.size() };
    for (uint32_t i = 0; i < Order.size(); i++)
        Spills[i] = CurSpills[Order[i]];

    Instruction* LastSpill = CurSpills[CurSpills.size() - 1];
    IGC_ASSERT(LastSpill);
    auto* InsertPt = LastSpill->getNextNode();

    for (auto* Spill : Spills)
    {
        if (justRespill(Spill) && Remove)
            Spill->eraseFromParent();
        else
            Spill->moveBefore(InsertPt);
    }
}

std::vector<uint32_t> LateRematPass::computeNewOrder(const BasicBlock& BB) const
{
    std::vector<std::pair<const SpillValueIntrinsic*, uint32_t>> Spills;

    uint32_t Cnt = 0;
    for (auto& I : BB)
    {
        if (auto* SI = dyn_cast<SpillValueIntrinsic>(&I))
            Spills.push_back({ SI, Cnt++ });
    }

    llvm::stable_sort(Spills, [](const auto &A, const auto &B) {
        return !justRespill(A.first) && justRespill(B.first);
    });

    std::vector<uint32_t> Indices;
    for (auto [_, Idx] : Spills)
        Indices.push_back(Idx);

    return Indices;
}

// If we encounter TraceRay() with a loop, try to rearrange the fields of
// spills that loop back to the continuation such that the fields that are only
// filled just to be re-spilled are moved to the bottom so they can be skipped
// in the loop.
bool LateRematPass::rearrange(ContinuationInfo& ContInfo)
{
    bool Changed = false;
    for (auto& Pair : ContInfo.spillfillblocks())
    {
        auto* FillBB = Pair.first;
        auto& SpillBBs = Pair.second;

        auto I = ContInfo.SuspendPoints.find(FillBB->getParent());
        if (I == ContInfo.SuspendPoints.end())
            continue;

        auto& Suspends = I->second;

        auto II = llvm::find_if(Suspends, [&](ContinuationHLIntrinsic* I) {
            return I->getContinuationFn() == FillBB->getParent();
        });

        // If there isn't a self-loop, then there is nothing to do.
        if (II == Suspends.end())
            continue;

        Changed = true;

        auto* SpillLoopBB = (*II)->getParent();

        auto Order = computeNewOrder(*SpillLoopBB);
        for (auto* BB : SpillBBs)
            removeDeadSpills(*BB, Order, SpillLoopBB == BB);
        rearrangeFills(*FillBB, Order);
    }
    return Changed;
}

void LateRematPass::shrinkSpillFill(
    const DataLayout &DL,
    ContinuationInfo& ContInfo,
    bool Sort)
{
    // Now, shrink wrap the spills/fills so they are contiguous again.
    for (auto& Pair : ContInfo.spillfillblocks())
    {
        auto* FillBB = Pair.first;
        {
            auto Fills = filter<FillValueIntrinsic>(
                FillBB,
                [](auto* I) { return dyn_cast<FillValueIntrinsic>(I); });
            if (Sort)
                sortInstsBySize(DL, Fills, [](auto* I) { return I; });

            uint32_t CurLoc = 0;
            for (auto* FI : Fills)
            {
                uint32_t Size = (uint32_t)DL.getTypeAllocSize(FI->getType());
                FI->setOffset(CurLoc);
                CurLoc += Size;
            }
        }

        auto& SpillBBs = Pair.second;
        for (auto* SpillBB : SpillBBs)
        {
            auto Spills = filter<SpillValueIntrinsic>(
                SpillBB,
                [](auto* I) { return dyn_cast<SpillValueIntrinsic>(I); });
            if (Sort)
                sortInstsBySize(DL, Spills, [](auto* I) { return I->getData(); });

            uint32_t CurLoc = 0;
            for (auto* SI : Spills)
            {
                uint32_t Size = (uint32_t)DL.getTypeAllocSize(
                    SI->getData()->getType());
                SI->setOffset(CurLoc);
                CurLoc += Size;
            }
        }
    }
}

static int32_t getMemRayIndex(
    const Value* V,
    const TraceRayAsyncHLIntrinsic* TRI)
{
    // Skip Tmax
    for (uint32_t i = 0; i < RTStackFormat::RayInfoSize - 1; i++)
    {
        if (V == TRI->getRayInfo(i))
            return i;
    }

    return -1;
}

bool LateRematPass::tryMemRayOpt(
    FillValueIntrinsic* Fill,
    ContinuationInfo::SpillColl& Spills)
{
    // When maxTraceRecursionDepth <= 1, we know there won't be any recursive
    // traces.  If we spill MemRay fields (obj, dir, and Tmin but *not*
    // Tmax) we know that HW doesn't update those entries.  In that case, we
    // can just read from the corresponding memory slot instead of the spill
    // slot.

    if (m_CGCtx->pipelineConfig.maxTraceRecursionDepth > 1)
        return false;

    if (!Fill->getType()->isFloatTy())
        return false;

    int32_t CurVal = -1;
    for (auto *SI : Spills)
    {
        TraceRayAsyncHLIntrinsic* TRI = nullptr;
        for (auto& I : reverse(*SI->getParent()))
        {
            if (auto* GII = dyn_cast<TraceRayAsyncHLIntrinsic>(&I))
            {
                TRI = GII;
                break;
            }
        }

        // If we didn't find a trace the spills must be due to a CallShader().
        if (!TRI)
            return false;

        int32_t Ret = getMemRayIndex(SI->getData(), TRI);
        if (Ret < 0)
            return false;

        if (CurVal < 0)
            CurVal = Ret;

        if (Ret != CurVal)
            return false;
    }

    auto go = [=]()
    {
        RTBuilder RTB(Fill, *m_CGCtx);
        auto* StackPointer = RTB.getAsyncStackPointer();
        Value* RematVal = RTB.getRayInfo(StackPointer, CurVal);
        Fill->replaceAllUsesWith(RematVal);
        for (auto* SI : Spills)
            SI->eraseFromParent();

        Fill->eraseFromParent();
    };
    Thunks.push_back(go);

    return true;
}

bool LateRematPass::tryRematLocalPointer(
    ContinuationInfo& CI,
    FillValueIntrinsic* Fill,
    ContinuationInfo::SpillColl& Spills)
{
    if (!CI.canPromoteContinuations())
        return false;

    if (!Fill->getType()->isPointerTy())
        return false;

    if (Fill->getType()->getPointerAddressSpace() != ADDRESS_SPACE_CONSTANT)
        return false;

    if (auto* Root = CI.findUniqueSpillRoot(Fill))
    {
        if (auto* LP = dyn_cast<LocalBufferPointerIntrinsic>(Root))
        {
            auto go = [=]()
            {
                auto* NewLP = LP->clone();
                NewLP->insertBefore(Fill);
                Fill->replaceAllUsesWith(NewLP);
                for (auto* SI : Spills)
                    SI->eraseFromParent();

                Fill->eraseFromParent();
            };

            Thunks.push_back(go);
            return true;
        }
    }

    return false;
}

bool LateRematPass::tryRematAllocas(
    ContinuationInfo& CI,
    const DenseMap<const AllocaInst*, uint32_t> &AllocaMap,
    FillValueIntrinsic* Fill,
    ContinuationInfo::SpillColl& Spills)
{
    // Remat alloca address calculations when possible

    if (!Fill->getType()->isPointerTy())
        return false;

    // Bail out if it's not a global pointer: it couldn't be a product of an
    // alloca.
    if (!RTBuilder::isNonLocalAlloca(Fill->getType()->getPointerAddressSpace()))
        return false;

    auto addrComputation = [&](SpillValueIntrinsic* SI)
    {
        return getAddressComputation(CI, SI->getData());
    };

    if (auto Chain = unify(Spills, addrComputation))
    {
        auto NewChain = *Chain;
        // We capture some of these by value as they change from function to
        // function.
        auto go = [&,NewChain,Fill,Spills]()
        {
            ValueToValueMapTy VM;
            RTBuilder RTB(Fill, *m_CGCtx);
            for (auto* V : NewChain)
            {
                if (auto* AI = dyn_cast<AllocaInst>(V))
                {
                    // Actually do the replacement calculation now.
                    auto I = AllocaMap.find(AI);
                    IGC_ASSERT_MESSAGE((I != AllocaMap.end()), "missing alloca?");
                    Value* stackBufferPtr = RTB.createAllocaNumber(AI, I->second);
                    // Given that we've examined the stack layout at this point,
                    // don't add additional allocas before intrinsic lowering
                    // (it's okay to add them to the end of the alloca list
                    // if needed).
                    VM[AI] = stackBufferPtr;
                    break;
                }
            }
            // We just handled the alloca above (the first instruction).  Skip
            // it and pass in the rest.
            remat(makeArrayRef(NewChain).drop_front(1), VM, Fill);
            Value* NewVal = VM[NewChain.back()];
            Fill->replaceAllUsesWith(NewVal);
            for (auto* SI : Spills)
                SI->eraseFromParent();

            Fill->eraseFromParent();
        };
        Thunks.push_back(go);
        return true;
    }

    return false;
}

// Walk the chain and inject the instructions at the given point.
void LateRematPass::remat(
    ArrayRef<Value*> Chain,
    ValueToValueMapTy& VM,
    FillValueIntrinsic* FI)
{
    for (auto* V : Chain)
    {
        IGC_ASSERT_MESSAGE(!isa<Argument>(V), "can't remat an arg!");
        if (isa<Constant>(V))
            continue;

        auto* I = cast<Instruction>(V);
        I->insertBefore(FI);
        VM[I] = I;
        RemapInstruction(I, VM, RF_NoModuleLevelChanges);
        I->setDebugLoc(FI->getDebugLoc());
    }
}

static void freeChain(ComputeChain* Chain)
{
    // remove all references
    for (auto* V : *Chain)
    {
        if (auto* I = dyn_cast<Instruction>(V))
        {
            if (!I->getParent())
                I->dropAllReferences();
        }
    }

    // now actually delete them
    for (auto* V : *Chain)
    {
        if (auto* I = dyn_cast<Instruction>(V))
        {
            if (!I->getParent())
                I->deleteValue();
        }
    }
}

// Ensure that all spills write the same value.  For each spill, we try to
// compute a sequence of instructions without side-effects that represents that
// value.  If we fail to do so (e.g., some pattern that we don't yet understand
// or side-effects that we couldn't remat) then we can't unify the spills.  If
// we find that they all would, in fact, spill the same value then the returned
// computation is a candidate to rematerialize.
Optional<ComputeChain> LateRematPass::unify(
    ArrayRef<SpillValueIntrinsic*> Spills,
    MkComputeChainFunc fn)
{
    if (Spills.empty())
        return None;

    auto First = fn(Spills[0]);
    if (!First.hasValue())
        return None;

    for (auto* Spill : Spills.drop_front(1))
    {
        if (auto Curr = fn(Spill))
        {
            if (!areChainsEqual(*First, *Curr))
            {
                freeChain(&*First);
                freeChain(&*Curr);
                return None;
            }
            else
            {
                freeChain(&*Curr);
            }
        }
        else
        {
            freeChain(&*First);
            return None;
        }
    }

    return First;
}

// See if we can trivially trace back to the originating alloca if 'V' is
// a stack address.
Optional<ComputeChain> LateRematPass::getAddressComputation(
    ContinuationInfo &CI,
    Value* V)
{
    auto lift = [](ComputeChain& Chain, ValueToValueMapTy& VM)
    {
        for (auto* V : Chain)
        {
            if (auto* I = dyn_cast<Instruction>(V))
            {
                RemapInstruction(I, VM, RF_NoModuleLevelChanges);
            }
        }
    };

    ComputeChain Chain;

    auto* I = dyn_cast<Instruction>(V);

    if (!I)
        return None;

    std::unique_ptr<ComputeChain, decltype(&freeChain)> Guard(&Chain, &freeChain);
    ValueToValueMapTy VM;

    auto push = [&](Instruction* I)
    {
        auto* NewI = I->clone();
        Chain.push_back(NewI);
        VM[I] = NewI;
    };

    // Can add more analysis here if necessary though this does a good job
    // on all workloads seen so far.
    while (I)
    {
        switch (I->getOpcode())
        {
        case Instruction::GetElementPtr:
            if (!cast<GetElementPtrInst>(I)->hasAllConstantIndices())
                return None;
            push(I);
            I = dyn_cast<Instruction>(I->getOperand(0));
            break;
        case Instruction::BitCast:
            push(I);
            I = dyn_cast<Instruction>(I->getOperand(0));
            break;
        case Instruction::Call:
            if (auto* FI = dyn_cast<FillValueIntrinsic>(I))
            {
                if (auto* Root = CI.findUniqueSpillRoot(FI))
                {
                    I = dyn_cast<Instruction>(Root);
                    VM[FI] = I;
                }
                else
                {
                    return None;
                }
            }
            else
            {
                return None;
            }
            break;
        case Instruction::Alloca:
            Chain.push_back(I);
            VM[I] = I;
            // reverse back to dominance order.
            std::reverse(std::begin(Chain), std::end(Chain));
            lift(Chain, VM);
            Guard.release();
            return Chain;
        default:
            return None;
        }
    }

    return None;
}

namespace IGC
{

Pass* createLateRematPass(void)
{
    return new LateRematPass();
}

} // namespace IGC

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass coalesces reads from the RTGlobals and global root signature
/// into block reads.  It may be extended for the local root signature in the
/// future.
///
/// Rather than doing individual loads such as:
///
/// %&stackSizePerRay = getelementptr %RayDispatchGlobalData addrspace(1)* %globalPtr, i64 0, i32 2, i32 0
/// %stackSizePerRay = load i32 addrspace(1)* %&stackSizePerRay
///
/// %&rtMemBasePtr = getelementptr %RayDispatchGlobalData addrspace(1)* %globalPtr, i64 0, i32 0
/// %bc = bitcast i64 addrspace(1)* %&rtMemBasePtr to <2 x i32> addrspace(1)*
/// %rtMemBasePtr = load <2 x i32> addrspace(1)* %bc
///
/// ...
///
/// We can do a block load then extract out the pieces:
///
/// %BlockLoad = load <8 x i32> addrspace(1)* %globalPtr
///
/// %stackSizePerRay = extractelement <8 x i32> %BlockLoad, i64 4
///
/// %9 = extractelement <8 x i32> %BlockLoad, i64 0
/// %10 = extractelement <8 x i32> % BlockLoad, i64 1
/// %11 = insertelement <2 x i32> undef, i32 % 9, i64 0
/// %rtMemBasePtr = insertelement <2 x i32> % 11, i32 % 10, i64 1
///
/// ...
///
/// We trade some increased register pressure for fewer memory accesses.
///
/// We currently decompose the region of memory to access into blocks of at most
/// MAX_BLOCK_SIZE bytes.  Currently, VectorPreProcess will decompose these
/// further to 128-byte block reads.
///
/// There is a regkey "RayTracingConstantCoalescingMinBlockSize" that can be
/// used to tune register pressure vs. load tradeoff.  In units of OWords, you
/// may select how fine grained of an access to do.  For example, suppose we use
/// a block size of 2 OWords = 32 bytes and there are three DW accesses
/// at offsets 4, 20, and 96 bytes from the base of the global pointer.
///
/// The first two accesses will be grouped into a 32-byte message and the last
/// access will go in its own message (broken down to just a scalar read as it
/// was before).  This is because there were intervening empty blocks of
/// 32-bytes with no data used.  If we had set
/// RayTracingConstantCoalescingMinBlockSize = 8 (128 bytes), all three accesses
/// would be coalesced into the same read.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CISACodeGen/PrepareLoadsStoresUtils.h"
#include "MemRegionAnalysis.h"
#include "Interval.h"

#include <vector>
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Support/MathExtras.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;
using namespace Intervals;

class RayTracingConstantCoalescingPass : public FunctionPass
{
public:
    RayTracingConstantCoalescingPass(): FunctionPass(ID)
    {
        initializeRayTracingConstantCoalescingPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<DominatorTreeWrapperPass>();
    }

    bool runOnFunction(Function &M) override;
    StringRef getPassName() const override
    {
        return "RayTracingConstantCoalescingPass";
    }

    static char ID;
};

char RayTracingConstantCoalescingPass::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "raytracing-constant-coalescing-pass"
#define PASS_DESCRIPTION2 "Coalesce loads from RTGlobals"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(RayTracingConstantCoalescingPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(RayTracingConstantCoalescingPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

// A load and its associated access range in bytes.
// The first uint64_t is the offset in bytes from the base of the global pointer.
// The second uint64_t is also the offset but the last byte (inclusive) accessed
// by the load.
using LoadInfo        = std::tuple<LoadInst*, uint64_t, uint64_t>;
// Same as LoadInfo but for the newly created block loads.
using BlockLoadInfo   = std::tuple<LoadInst*, uint64_t, uint64_t>;

static constexpr uint32_t MinBlockAlign = 16;

// We currently are doing the vector loads with a DW element type.  Change
// this and getVectorEltTy() to try other types (QW is the only one of interest
// probably).
static constexpr uint32_t getVectorEltSize()
{
    return 4;
}

static Type* getVectorEltTy(LLVMContext& C)
{
    static_assert(getVectorEltSize() == 4, "mismatch?");
    return Type::getInt32Ty(C);
}

// This class is responsible for computing the extraction indices across loads
// so we can replace the given load.  'LI' could straddle two contiguous loads
// so this should generate extractelements from the end of the left
// block load and the start of the right block load.
class Stitcher
{
public:
    Stitcher(Module& M, ArrayRef<BlockLoadInfo> Loads) :
        Loads(Loads), DL(M.getDataLayout()),
        IRB(M.getContext()), EltSize(getVectorEltSize()) {}

    Value* getReplacement(LoadInst* LI, const Interval& I)
    {
        Elts.clear();
        auto* LoadEltTy = LI->getType()->getScalarType();
        uint32_t LoadEltSize = uint32_t(DL.getTypeSizeInBits(LoadEltTy) / 8);
        IGCLLVM::FixedVectorType* vecType = llvm::dyn_cast<IGCLLVM::FixedVectorType>(LI->getType());
        // How many indices `LI` demands.
        uint32_t NumEltsRem =
            isa<VectorType>(LI->getType()) ?
            (uint32_t)vecType->getNumElements() : 1;

        auto& [LoadStart, LoadEnd] = I;
        // Expand the range out so it is aligned with the element type of
        // the block load.
        auto [NewLoadStart, NewLoadEnd] =
            expandRange(EltSize, EltSize, LoadStart, LoadEnd);

        IGC_ASSERT(NewLoadStart % EltSize == 0);
        IGC_ASSERT((NewLoadEnd - NewLoadStart + 1) % EltSize == 0);
        // This is the element index if we were to break up the entire range
        // into `EltSize` chunks without regards to any gaps in between loads.
        uint32_t StartIdx = uint32_t(NewLoadStart / EltSize);

        IRB.SetInsertPoint(LI);

        // Look for block loads that overlap with `LI` so we can extract
        // the needed data.
        for (auto& [BlockLI, BlockStart, BlockEnd] : Loads)
        {
            if (overlap(I, std::make_pair(BlockStart, BlockEnd)))
            {
                IGC_ASSERT(BlockStart % EltSize == 0);
                IGC_ASSERT((BlockEnd - BlockStart + 1) % EltSize == 0);
                uint32_t BlockStartIdx = uint32_t(BlockStart / EltSize);
                uint32_t BlockNumIdx = uint32_t((BlockEnd - BlockStart + 1) / EltSize);
                IGC_ASSERT(StartIdx >= BlockStartIdx);

                uint32_t MaxIdx = 0;

                for (uint32_t i = StartIdx - BlockStartIdx; i < BlockNumIdx; i++)
                {
                    auto* CurElt = IRB.CreateExtractElement(BlockLI, i);
                    MaxIdx = std::max(MaxIdx, i);

                    if (LoadEltSize > EltSize)
                    {
                        // we currently don't need to handle larger loads
                        // because we break them down in runOnFunction() (as
                        // long as they are no larger than 64-bit which
                        // shouldn't happen).
                        IGC_ASSERT_MESSAGE(0, "shouldn't happen!");
                    }
                    else if (LoadEltSize < EltSize)
                    {
                        // This could be needed in the future.
                        IGC_ASSERT_MESSAGE(0, "not yet!");
                    }
                    else
                    {
                        Elts.push_back(CurElt);
                        IGC_ASSERT(NumEltsRem > 0);
                        NumEltsRem--;
                        StartIdx++;
                        if (NumEltsRem == 0)
                            break;
                    }
                }

                auto Iter = MaxEltIdx.find(BlockLI);

                if (Iter != MaxEltIdx.end())
                    Iter->second = std::max(Iter->second, MaxIdx);
                else
                    MaxEltIdx.insert(std::make_pair(BlockLI, MaxIdx));

                if (NumEltsRem == 0)
                    break;
            }
        }

        if (Elts.empty())
        {
            IGC_ASSERT_MESSAGE(0, "couldn't find data?");
            return nullptr;
        }

        IGC_ASSERT_MESSAGE(NumEltsRem == 0, "Couldn't patch all values!");

        // Now that `Elts` has been populated, we just need to build the final
        // value to replace `LI` with.

        if (!isa<VectorType>(LI->getType()))
        {
            IGC_ASSERT(Elts.size() == 1);
            auto *Res = IRB.CreateBitOrPointerCast(Elts[0], LI->getType());
            Res->takeName(LI);
            return Res;
        }
        else
        {
            IGCLLVM::FixedVectorType* vecType = llvm::dyn_cast<IGCLLVM::FixedVectorType>(LI->getType());
            IGC_ASSERT(Elts.size() == vecType->getNumElements());
            Value* Vec = UndefValue::get(LI->getType());
            for (uint32_t i = 0; i < Elts.size(); i++)
            {
                auto* Elt = IRB.CreateBitOrPointerCast(Elts[i], LoadEltTy);
                Vec = IRB.CreateInsertElement(Vec, Elt, i);
            }

            Vec->takeName(LI);
            return Vec;
        }
    }
public:
    DenseMap<LoadInst*, uint32_t>& getMaxEltMap()
    {
        return MaxEltIdx;
    }
private:
    // Track the maximum element index that was accessed from `LI`.  This will
    // be used to shrink-wrap loads that only use part of their range.
    DenseMap<LoadInst*, uint32_t> MaxEltIdx;
    // The block loads.  The array is sorted and disjoint.
    ArrayRef<BlockLoadInfo> Loads;
    const DataLayout& DL;
    IRBuilder<> IRB;
    // Size of the element of the block loads (currently, all block loads
    // have the same type).  For example, if the block load is an <8 x i32>,
    // EltSize = 4.
    uint32_t EltSize = 0;
    // `Elts` will be populated with integer sized types corresponding to the
    // elements in `LI`.
    SmallVector<Value*, 16> Elts;
};

// returns true iff the interval `I` associated with some block is referenced
// by at least one load.
static bool needBlock(const Interval& I, ArrayRef<LoadInfo> Loads)
{
    for (auto& [LI, Start, End] : Loads)
    {
        if (overlap(I, std::make_pair(Start, End)))
            return true;
    }

    return false;
}

// Precondition: should have called expandRange on [StartOffset, EndOffset]
// prior to this.
static SortedIntervals getRanges(
    ArrayRef<LoadInfo> Loads,
    uint32_t MinBlockSize,
    uint64_t StartOffset,
    uint64_t EndOffset)
{
    SortedIntervals Intervals;

    // Get first set of needed blocks
    uint64_t NumMinBlocks = (EndOffset - StartOffset + 1) / MinBlockSize;

    uint64_t Start = StartOffset;
    for (uint32_t i = 0; i < NumMinBlocks; i++)
    {
        Interval CurInterval = std::make_pair(Start, Start + MinBlockSize - 1);
        if (needBlock(CurInterval, Loads))
            Intervals.push_back(CurInterval);
        Start += MinBlockSize;
    }

    // Given the intervals that have at least some part read by a load, do a
    // bottom up merging into bigger intervals.
    mergeIntervals(Intervals, [](uint64_t NewSize) {
        return llvm::isPowerOf2_64(NewSize) && NewSize <= MAX_BLOCK_SIZE;
    });

    return Intervals;
}

static SmallVector<BlockLoadInfo, 4> getBlockLoads(
    CodeGenContext &Ctx, Instruction* InsertPt, ArrayRef<Interval> Ranges)
{
    SmallVector<BlockLoadInfo, 4> BlockLoads;
    RTBuilder RTB(InsertPt, Ctx);

    auto* EltTy = getVectorEltTy(*Ctx.getLLVMContext());
    constexpr uint64_t EltSize = getVectorEltSize();

    auto* GlobalPtr = RTB.getGlobalBufferPtr();
    uint32_t AddrSpace = GlobalPtr->getType()->getPointerAddressSpace();

    GlobalPtr = RTB.CreateBitCast(GlobalPtr, RTB.getInt8PtrTy(AddrSpace));

    for (auto &R : Ranges)
    {
        auto &[Start, End] = R;
        uint64_t Size = End - Start + 1;
        IGC_ASSERT(Size % EltSize == 0);
        uint32_t NumElts = static_cast<uint32_t>(Size / EltSize);

        auto* VectorTy = IGCLLVM::FixedVectorType::get(EltTy, NumElts);
        auto* Ptr = RTB.CreateGEP(GlobalPtr, RTB.getInt32((uint32_t)Start));
        Ptr = RTB.CreateBitCast(Ptr, VectorTy->getPointerTo(AddrSpace));
        auto *LI = RTB.CreateAlignedLoad(
            Ptr, IGCLLVM::getAlign(MinBlockAlign), VALUE_NAME("BlockLoad"));
        RTB.setInvariantLoad(LI);

        BlockLoads.push_back(std::make_tuple(LI, Start, End));
    }

    return BlockLoads;
}

// It is safe to insert the block loads at the top of the block that
// dominates all loads.
static Instruction* getInsertPt(DominatorTree& DT, ArrayRef<LoadInfo> Loads)
{
    BasicBlock* InsertBB = nullptr;
    for (auto &[LI, Start, End] : Loads)
    {
        if (!InsertBB)
            InsertBB = LI->getParent();
        else
            InsertBB = DT.findNearestCommonDominator(InsertBB, LI->getParent());
    }

    auto* InsertPt = &*InsertBB->getFirstInsertionPt();
    return InsertPt;
}

bool RayTracingConstantCoalescingPass::runOnFunction(Function &F)
{
    auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    if (!Ctx->m_DriverInfo.supportsExpandedRTGlobals())
        return false;

    auto& DL = F.getParent()->getDataLayout();

    SmallVector<LoadInfo, 8> Loads;

    uint64_t StartOffset = std::numeric_limits<uint64_t>::max();
    uint64_t EndOffset   = 0;

    IRBuilder<> IRB(F.getContext());

    for (auto II = inst_begin(&F), EI = inst_end(&F); II != EI; /* empty */)
    {
        auto* LI = dyn_cast<LoadInst>(&*II++);
        // All accesses to the globals should have invariant.load attached
        // but we'll check just to be sure here in case that changes one day.
        if (!LI || !LI->getMetadata(LLVMContext::MD_invariant_load))
            continue;

        uint64_t Offset = 0;
        auto Region = getRegionOffset(LI->getPointerOperand(), &DL, &Offset);

        if (!Region || *Region != RTMemRegion::RTGlobals)
            continue;

        // This really shouldn't happen but just to be safe.  Only loads that
        // are int/float/pointer/vector should go through.
        if (LI->getType()->isAggregateType())
            continue;

        // make all loads 32-bit (makes vectorization easier).
        IRB.SetInsertPoint(LI);
        if (auto [NewVal, NewLI] = expand64BitLoad(IRB, DL, LI); NewVal)
        {
            NewVal->takeName(LI);
            LI->replaceAllUsesWith(NewVal);
            LI->eraseFromParent();
            LI = NewLI;
        }

        uint64_t Size = DL.getTypeSizeInBits(LI->getType()) / 8;

        uint64_t Start = Offset;
        uint64_t End   = Offset + Size - 1;

        Loads.push_back(std::make_tuple(LI, Start, End));

        StartOffset = std::min(StartOffset, Start);
        EndOffset   = std::max(EndOffset, End);
    }

    if (Loads.empty())
        return false;

    auto& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    auto* InsertPt = getInsertPt(DT, Loads);

    // Minimum size of a load in bytes
    uint32_t MinBlockSize =
        IGC_GET_FLAG_VALUE(RayTracingConstantCoalescingMinBlockSize);
    MinBlockSize = (MinBlockSize == 0) ?
        4 * MinBlockAlign :
        MinBlockSize * MinBlockAlign;

    auto [NewStartOffset, NewEndOffset] =
        expandRange(MinBlockSize, MinBlockAlign, StartOffset, EndOffset);

    auto Ranges = getRanges(Loads, MinBlockSize, NewStartOffset, NewEndOffset);
    auto NewLoads = getBlockLoads(*Ctx, InsertPt, Ranges);

    Stitcher S{ *InsertPt->getModule(), NewLoads };

    for (auto& [LI, Start, End] : Loads)
    {
        if (auto* NewVal = S.getReplacement(LI, std::make_pair(Start, End)))
        {
            LI->replaceAllUsesWith(NewVal);
            LI->eraseFromParent();
        }
    }

    // After creating the block loads, shrink the ones that don't access
    // elements beyond what a smaller vector length would manage.
    // %v = load <8 x i32> addrspace(1)* %p
    // %x = extractelement <8 x i32> %v, i32 0
    // %y = extractelement <8 x i32> %v, i32 2
    // %z = extractelement <8 x i32> %v, i32 3
    // ==>
    // %v = load <4 x i32> addrspace(1)* %p
    // %x = extractelement <4 x i32> %v, i32 0
    // %y = extractelement <4 x i32> %v, i32 2
    // %z = extractelement <4 x i32> %v, i32 3
    for (auto& [LI, Idx] : S.getMaxEltMap())
    {
        IGCLLVM::FixedVectorType* vecType = llvm::dyn_cast<IGCLLVM::FixedVectorType>(LI->getType());
        uint32_t NumElts =
            isa<VectorType>(LI->getType()) ?
            (uint32_t)vecType->getNumElements() : 1;

        uint64_t NewSize = llvm::PowerOf2Ceil(Idx + 1);

        if (NewSize >= NumElts)
            continue;

        IRB.SetInsertPoint(LI);

        auto* EltTy = getVectorEltTy(F.getContext());
        auto* NewRetTy = IGCLLVM::FixedVectorType::get(EltTy, int_cast<uint32_t>(NewSize));
        auto* NewPtr = IRB.CreateBitCast(
            LI->getPointerOperand(),
            NewRetTy->getPointerTo(LI->getPointerAddressSpace()));

        auto *NewLoad = IRB.CreateAlignedLoad(
            NewPtr->getType()->getPointerElementType(),
            NewPtr,
            IGCLLVM::getAlign(LI->getAlignment()));
        NewLoad->takeName(LI);

        Value* V2 = UndefValue::get(LI->getType());
        for (uint32_t i = 0; i <= Idx; i++)
        {
            auto* Val = IRB.CreateExtractElement(NewLoad, i);
            V2 = IRB.CreateInsertElement(V2, Val, i);
        }

        LI->replaceAllUsesWith(V2);
        LI->eraseFromParent();
    }

    return true;
}

namespace IGC
{

Pass* createRayTracingConstantCoalescingPass(void)
{
    return new RayTracingConstantCoalescingPass();
}

} // namespace IGC

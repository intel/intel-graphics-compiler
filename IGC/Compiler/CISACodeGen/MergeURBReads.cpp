/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "common/IGCIRBuilder.h"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"
#include "IGCPassSupport.h"
#include "MergeURBReads.hpp"

namespace IGC
{
using namespace llvm;
/// This pass merges URB read instructions to reduce the number of sends.
///
/// Assumptions and limitations:
///  1 Current implementation does not merge instructions from different basic
///    blocks.
///  2 Only URBRead instructions are merged.
///  3 This pass assumes that in the input LLVM IR only the first 4 DWORDs
///    of the URBRead result will be used. If this assumption is not true the
///    result of the pass will still be correct but less optimal.
///  4 This pass will not merge instructions if results of both instructions
///    are indexed dynamically and it is not possible to find the maximum value
///    of the index used.
///
/// Merging is done using the following approach: for every basic block:
/// - gather all URBRead messages and sort in ascending order so that the
///   reads from lower URB offsets go first
/// - iterate over the URBRead instructions in ascending order
/// - for given URBRead instruction look for an URBRead instructions that read
///   the same or the next 4-DWORD chunk of URB (see assumption 3) and merge
///   with the current one if possible
///
class MergeURBReads : public FunctionPass
{
public:
    static char ID;

    MergeURBReads() :
        FunctionPass(ID)
    {
        initializeMergeURBReadsPass(*PassRegistry::getPassRegistry());
    }

    virtual bool runOnFunction(Function& F);

    virtual void getAnalysisUsage(AnalysisUsage& AU) const
    {
        AU.setPreservesAll();
    }

    virtual llvm::StringRef getPassName() const { return "MergeURBReads"; }

private:
    void GatherReads(BasicBlock& BB);
    bool MergeReads();

private:
    // member 0 - vertex index
    // member 1 - runtime URB offset
    // member 2 - constant URB offset
    // member 3 - URBRead intrinsic
    using URBReadEntry = std::tuple<Value*, Value*, uint, GenIntrinsicInst*>;
    // Ordered set, ascending order, for given pair of <vertex index, dynamic
    // offset> URBReads from smaller constant URB offsets are before those with
    // larger offsets.
    std::set<URBReadEntry> m_URBReads;
};

char MergeURBReads::ID = 0;

#define PASS_FLAG "igc-merge-urb-reads"
#define PASS_DESCRIPTION "Merges URB Read instructions."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MergeURBReads, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MergeURBReads, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

llvm::FunctionPass* createMergeURBReadsPass()
{
    return new MergeURBReads();
}

bool MergeURBReads::runOnFunction(Function& F)
{
    // For each basic block in the function:
    // - gather all calls to URBRead intrinsic
    // - merge URBRead calls
    // - remove unused URBRead calls
    bool modified = false;
    for (auto& BB : F)
    {
        m_URBReads.clear();
        GatherReads(BB);
        modified |= MergeReads();
        for (auto& entry : m_URBReads)
        {
            GenIntrinsicInst* intr = std::get<3>(entry);
            if (intr->getNumUses() == 0)
            {
                intr->eraseFromParent();
            }
        }
    }
    return modified;
}

// Dynamic cast to an Instruction with specific opcode.
template <uint Opcode>
Instruction* dyn_cast(Value* v)
{
    if (isa<Instruction>(v) &&
        Operator::getOpcode(v) == Opcode)
    {
        return cast<Instruction>(v);
    }
    return nullptr;
}


// Returns the channel mask for a URBRead instruction based on the input
// ExtractElement instruction index operand.
static inline uint GetChannelMask(ExtractElementInst* ee)
{
    if (isa<ConstantInt>(ee->getIndexOperand()))
    {
        return (1 << getImmValueU32(ee->getIndexOperand()));
    }
    // Non-constant index:
    // Pattern 1:
    //  %155 = and i32 %153, 3
    //  %156 = shl i32 1, %155
    if (Instruction* shlInst = dyn_cast<Instruction::Shl>(ee->getIndexOperand()))
    {
        Instruction* andInst = dyn_cast<Instruction::And>(shlInst->getOperand(1));
        if (andInst &&
            isa<ConstantInt>(shlInst->getOperand(0)) &&
            isa<ConstantInt>(andInst->getOperand(1)) &&
            getImmValueU32(andInst->getOperand(1)) <= 3)
        {
            if (getImmValueU32(shlInst->getOperand(0)) == 1)
            {
                return 0x0F;
            }
            else if (getImmValueU32(shlInst->getOperand(0)) == 8)
            {
                return 0xF0;
            }
        }
    }
    // Pattern 2:
    //  %155 = and i32 %153, 3
    else if (Instruction* andInst = dyn_cast<Instruction::And>(ee->getIndexOperand()))
    {
        if (isa<ConstantInt>(andInst->getOperand(1)) &&
            getImmValueU32(andInst->getOperand(1)) <= 3)
        {
            return 0x0F;
        }
    }
    return 0xFF;
}

// Returns the channel mask for a URBRead instruction. This function checks all
// ExtractElement instructions to produce the mask of (potentially) used data.
static inline uint GetChannelMask(GenIntrinsicInst* inst)
{
    IGC_ASSERT(inst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_URBRead);
    uint mask = 0;
    for (auto iit = inst->user_begin(), eit = inst->user_end();
        iit != eit && mask != 0xFF;
        ++iit)
    {
        ExtractElementInst* ee = dyn_cast<ExtractElementInst>(*iit);
        if (!ee)
        {
            mask = 0xFF;
            break;
        }
        mask |= GetChannelMask(ee);
    }
    return mask;
}

// Gets all URBRead instruction in a basic block.
void MergeURBReads::GatherReads(BasicBlock& BB)
{
    for (auto II = BB.begin(), IE = BB.end();
        II != IE;
        ++II)
    {
        GenIntrinsicInst* intr = dyn_cast<GenIntrinsicInst>(&*II);
        if (intr &&
            intr->isGenIntrinsic(GenISAIntrinsic::GenISA_URBRead))
        {
            std::pair<Value*, uint> baseAndOffset =
                GetURBBaseAndOffset(intr->getOperand(1));
            m_URBReads.insert(std::make_tuple(
                intr->getOperand(0),
                baseAndOffset.first,
                baseAndOffset.second,
                intr));
        }
    }
}

// Moves a URB read instruction before the user instruction if needed.
static inline void MoveDefBeforeUse(
    GenIntrinsicInst* def,
    Instruction* use)
{
    if (use->getParent() == def->getParent())
    {
        // Check if use is before def
        for (BasicBlock::const_iterator iit = use->getParent()->begin(), eit = def->getIterator();
            iit != eit; ++iit)
        {
            if (iit == use->getIterator())
            {
                def->moveBefore(use);
                return;
            }
        }
    }
}

enum class MergeWith
{
    Prev, // merge candidate starts at offset 16B smaller
    Equal, // merge candidate starts at the same offset
    Next // merge candidate starts at offset 16B larger, not used in current code
};
/// Merges the `curr` URBRead in to the `other`.
/// Depending on the `op` the `other` URBRead may be reading the previous,
/// the same or next URB offset (in 4-DWORD units),
/// e.g. the following IR (Prev):
///   %other = call <8 x float> @llvm.genx.GenISA.URBRead(i32 %0, i32 2)
///   %8 = extractelement <8 x float> %other, i32 0
///   %curr = call <8 x float> @llvm.genx.GenISA.URBRead(i32 %0, i32 3)
///   %8 = extractelement <8 x float> %curr, i32 0
/// is changed to:
///   %other = call <8 x float> @llvm.genx.GenISA.URBRead(i32 %0, i32 2)
///   %8 = extractelement <8 x float> %other, i32 0
///   %curr = call <8 x float> @llvm.genx.GenISA.URBRead(i32 %0, i32 3)
///   %8 = extractelement <8 x float> %other, i32 4
template<MergeWith op>
bool Merge(
    GenIntrinsicInst* curr,
    GenIntrinsicInst* other)
{
    bool modified = false;
    IGC_ASSERT(other->getType() == curr->getType());
    IGC_ASSERT(other->getType()->isVectorTy() &&
        cast<IGCLLVM::FixedVectorType>(other->getType())->getNumElements() == 8);

    std::vector<User*> users(curr->user_begin(), curr->user_end());
    for (auto user : users)
    {
        if (op == MergeWith::Equal)
        {
            if (Instruction* inst = dyn_cast<Instruction>(user))
            {
                MoveDefBeforeUse(other, inst);
            }
            user->replaceUsesOfWith(curr, other);
            modified = true;
            continue;
        }
        ExtractElementInst* ee = dyn_cast<ExtractElementInst>(user);
        if (!ee)
        {
            continue;
        }
        Value* newIdx = nullptr;
        uint mask = GetChannelMask(ee);
        Value* four = ConstantInt::get(ee->getIndexOperand()->getType(), 4);
        IGCIRBuilder<> builder(ee);
        if (op == MergeWith::Prev && (mask & 0xF0) == 0)
        {
            newIdx = builder.CreateAdd(ee->getIndexOperand(), four);
        }
        else if (op == MergeWith::Next && (mask & 0xF0) == 0)
        {
            newIdx = builder.CreateSub(ee->getIndexOperand(), four);
        }
        if (newIdx)
        {
            MoveDefBeforeUse(other, ee);
            ee->setOperand(0, other);
            ee->setOperand(1, newIdx);
            modified = true;
        }
    }
    return modified;
}

bool MergeURBReads::MergeReads()
{
    bool modified = false;
    // Iterate over all URBRead instructions starting from the smallest offset in URB.
    for (auto cit = m_URBReads.begin(), eit = m_URBReads.end(); cit != eit; ++cit)
    {
        const URBReadEntry& curr = *cit;
        uint currConstOffset = std::get<2>(curr);
        GenIntrinsicInst* currIntr = std::get<3>(curr);
        if (currIntr->getNumUses() == 0) // already merged
        {
            continue;
        }
        // Look for an URBRead instruction that reads the same or next chunk of
        // URB and try to merge to current one.
        for (auto oit = std::next(cit); oit != eit; ++oit)
        {
            const URBReadEntry& other = *oit;
            uint otherConstOffset = std::get<2>(other);
            GenIntrinsicInst* otherIntr = std::get<3>(other);
            if (otherIntr->getNumUses() == 0 || // all uses merged
                std::get<0>(curr) != std::get<0>(other) || // different vertex index
                std::get<1>(curr) != std::get<1>(other) || // different runtime offset
                otherConstOffset > currConstOffset + 1) // too far in the URB
            {
                // no more candidates for merging exist
                break;
            }
            IGC_ASSERT(otherConstOffset >= currConstOffset);
            if (otherConstOffset == currConstOffset)
            {
                // This case should not be seen for "user" inputs but it is
                // possible to appear after SGV lowering, e.g. base vertex and
                // base instance in vertex shader can occupy the same input
                // 4-DWORD location.
                modified |= Merge<MergeWith::Equal>(otherIntr, currIntr);
            }
            else
            {
                IGC_ASSERT(otherConstOffset == currConstOffset + 1);
                modified |= Merge<MergeWith::Prev>(otherIntr, currIntr);
            }
        }
    }
    return modified;
}
} // namespace IGC

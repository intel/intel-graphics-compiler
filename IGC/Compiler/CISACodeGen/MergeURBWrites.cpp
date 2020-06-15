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

#include "MergeURBWrites.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"

namespace
{
    using namespace llvm;
    using namespace IGC;

    /// Used to store information about instructions and their positions in the current BB.
    class InstWithIndex
    {
    public:
        // Initialize pointer to null and place to -1 to recognize "empty" entries
        InstWithIndex() : m_inst(nullptr), m_place(-1)
        {
        }

        InstWithIndex(CallInst* inst, int place) :
            m_inst(inst),
            m_place(place)
        {
        }

        int GetPlace() const { return m_place; }
        CallInst* GetInst() const { return m_inst; }

    private:
        CallInst* m_inst;
        int m_place;
    };

    class MergeURBWrites : public FunctionPass
    {
    public:
        MergeURBWrites() :
            FunctionPass(ID)
        { }

        virtual bool runOnFunction(Function& F);

        virtual void getAnalysisUsage(AnalysisUsage& AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual llvm::StringRef getPassName() const { return "MergeURBWrites"; }

    private:
        /// Stores all URB write instructions in a vector.
        /// Also merges partial (channel granularity) writes to the same offset.
        void FillWriteList(BasicBlock& BB);

        // Tries to merge two writes to adjacent offsets into a single instruction.
        // Does this by going through write list containing stored write instructions
        // in order of offsets and merging adjacent writes.
        void MergeInstructions();

        // Returns the dynamic URB base offset and an immediate const offset
        // from the dynamic base. The function calculates the result by walking
        // the use-def chain of pUrbOffset.
        // If pUrbOffset is an immediate constant (==offset) then
        // <nullptr, offset> is returned.
        // In all other cases <pUrbOffset, 0> is returned.
        std::pair<Value*, unsigned int> GetBaseAndOffset(Value* pUrbOffset);

        // represents the map (urb index) --> (instruction, instruction index in BB)
        // The key consists of a dynamic URB base offset (key.first) and
        // an immediate offset from this dynamic base
        // Dynamic URB base offset is null if URB offset is constant.
        std::map<std::pair<Value*, unsigned int>, InstWithIndex> m_writeList;

        bool m_bbModified;
        static char ID;
    };

    char MergeURBWrites::ID = 0;

    unsigned int GetChannelMask(CallInst* inst)
    {
        return int_cast<unsigned int>(cast<ConstantInt>(inst->getOperand(1))->getZExtValue());
    }

} // end of unnamed namespace to contain class definition and auxiliary functions


/// This optimization merges shorter writes to URB to get a smaller number of longer writes
/// which is more efficient.
/// Current implementation can:
/// 1) merge consecutive writes of length 4 to a single write of length 8
/// 2) merge writes at the same offset with channel masks
///
/// The implementation works as follows: we maintain a map (urb offset) --> instruction
/// (kept as a vector) that gets filled with information taken from URBWrite instructions.
/// If we encounter two instructions writing at the same offset, we merge channel mask and
/// overwrite the earlier channel writes for repeated writes.
/// In the second phase, we go through the index list and replace two writes at adjacent
/// locations with one.
///
/// for now, we don't handle the following cases:
/// 1) channel mask is a runtime value
/// 2) handling of writes of size >4
///    so e.g. we don't handle |aaaa|bbbbbbbb|cccc| -> |aaaabbbb|bbbbcccc|
/// this will be addressed in the future.
///
bool MergeURBWrites::runOnFunction(Function& F)
{
    bool fModified = false;
    for (auto& BB : F)
    {
        FillWriteList(BB);
        MergeInstructions();
        fModified |= m_bbModified;
    }
    return fModified;
}

void MergeURBWrites::FillWriteList(BasicBlock& BB)
{
    m_bbModified = false;
    m_writeList.clear();
    int instCounter = 0; // counts the instruction in the BB
    for (auto iit = BB.begin(); iit != BB.end(); ++iit, ++instCounter)
    {
        auto intrinsic = dyn_cast<GenIntrinsicInst>(iit);
        if (intrinsic == nullptr) continue;

        GenISAIntrinsic::ID IID = intrinsic->getIntrinsicID();
        if ((IID == GenISAIntrinsic::GenISA_URBReadOutput) ||
            (IID == GenISAIntrinsic::GenISA_threadgroupbarrier))
        {
            MergeInstructions();
            m_writeList.clear();
        }
        // if not URBWrite intrinsic, we are not interested
        if (IID != GenISAIntrinsic::GenISA_URBWrite)
        {
            continue;
        }

        // intrinsic has the format: URB_write (%offset, %mask, %data0, ... , %data7)
        ConstantInt* pImmediateMask = dyn_cast<ConstantInt>(iit->getOperand(1));
        if (pImmediateMask == nullptr || (GetChannelMask(intrinsic) > 0x0F))
        {
            // for now, we don't handle the following cases:
            // 1) mask is a runtime value
            // 2) handling of writes of size >4
            //    so e.g. we don't handle |aaaa|bbbbbbbb|cccc| -> |aaaabbbb|bbbbcccc|
            // this will be addressed in the future
            continue;
        }

        std::pair<Value*, unsigned int> baseAndOffset =
            GetBaseAndOffset(iit->getOperand(0));

        auto it = m_writeList.find(baseAndOffset);
        // we encountered an instruction writing at the same offset,
        // most likely we write RTAI, VAI or PSIZE to vertex header
        // or we overwrite the old value
        if (it != m_writeList.end())
        {
            const InstWithIndex& instWithIndex = it->second;
            auto oldMask = GetChannelMask(instWithIndex.GetInst());
            auto newMask = GetChannelMask(intrinsic);
            // assume the write lengths are <=4
            // if we have writes to the same channel, we retain the later one,
            // discarding the earlier one
            if (oldMask <= 0x0F && newMask <= 0x0F)
            {
                // get difference oldMask-newMask to determine if we need to take any old operands
                auto takeFromOlderMask = oldMask & (~newMask);
                // update the mask stored in operand #1 of the second instruction
                auto mergedMask = oldMask | newMask;
                auto mergedMaskValue = llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(BB.getContext()),
                    mergedMask);
                intrinsic->setOperand(1, mergedMaskValue);
                // move data operands from the older instruction to the newer one
                unsigned int opIndex = 0;
                while (takeFromOlderMask != 0)
                {
                    if (takeFromOlderMask & 1)
                    {
                        intrinsic->setOperand(
                            opIndex + 2,
                            instWithIndex.GetInst()->getOperand(opIndex + 2));
                    }
                    ++opIndex;
                    takeFromOlderMask = takeFromOlderMask >> 1;
                }
                // after transferring the operands, remove the old instruction and store the new one
                instWithIndex.GetInst()->eraseFromParent();
                m_bbModified = true;
                m_writeList[baseAndOffset] = InstWithIndex(intrinsic, instCounter);
            }
        }
        else
        {
            // adding new write at this offset
            m_writeList[baseAndOffset] = InstWithIndex(intrinsic, instCounter);
        }
    }
} // FillWriteList()

void MergeURBWrites::MergeInstructions()
{
    // nothing to do for an empty list
    if (m_writeList.size() == 0)
    {
        return;
    }

    // vector of URBWrite8 in order of offsets
    llvm::SmallVector<InstWithIndex, 16> URBWrite8;

    auto last = std::prev(m_writeList.end());
    for (auto ii = m_writeList.begin(); ii != m_writeList.end() && ii != last; ++ii)
    {
        auto next = std::next(ii);
        if (ii->second.GetInst() == nullptr || next->second.GetInst() == nullptr)
        {
            //nothing to do, no write at current or next offset
            continue;
        }

        // ii->first.first is the dynamic URB base offset, may be nullptr
        // ii->first.second is the immediate constant offset from ii->first.first
        if (ii->first.first != next->first.first)
        {
            // nothing to do, different base URB offset
            continue;
        }
        if (ii->first.second + 1 != next->first.second)
        {
            // nothing to do, not a consecutive URB access
            continue;
        }
        // We have two instructions, merge them by moving operands from the one appearing
        // earlier in the BB to the one appearing later and increasing write length.
        //
        // From this (instructions "in order"):
        // earlierInst : URBWrite(offset,   mask1, d0, d1, d2, d3, xx, xx, xx, xx)
        // laterInst   : URBWrite(offset+1, mask2, g0, g1, g2, g3, xx, xx, xx, xx)
        //
        // we need to get this
        // laterInst   : URBWrite(offset, mask1 | mask2<<4, d0, d1, d2, d3, g0, g1, g2, g3)
        //
        // and from this ("reversed order"):
        // earlierInst : URBWrite(offset+1,   mask1, d0, d1, d2, d3, xx, xx, xx, xx)
        // laterInst   : URBWrite(offset, mask2, g0, g1, g2, g3, xx, xx, xx, xx)
        //
        // we need to get this:
        // laterInst   : URBWrite(offset, mask2 | mask1<<4, g0, g1, g2, g3, d0, d1, d2, d3)
        //
        // Note that earlier, later refers to the placement of the instruction
        // in the basic block while ii, next iterators refer to the placement
        // in the vector indexed by urb offsets, so 'ii' corresponds to 'offset'
        // and 'next' corresponds to 'offset+1'.
        //
        // determine which instruction is appearing earlier in the BB
        const bool inOrder = ii->second.GetPlace() < next->second.GetPlace();
        CallInst* earlierInst = inOrder ? ii->second.GetInst() : next->second.GetInst();
        CallInst* laterInst = !inOrder ? ii->second.GetInst() : next->second.GetInst();

        // merge per-channel write masks
        auto lowWriteMask = GetChannelMask(ii->second.GetInst());
        auto highWriteMask = GetChannelMask(next->second.GetInst());
        IGC_ASSERT(lowWriteMask <= 0x0F);
        IGC_ASSERT(highWriteMask <= 0x0F);
        auto mergedMask = lowWriteMask | (highWriteMask << 4);

        // Move the data operands from the earlier instruction to the later instruction.
        // If instructions are in order, we need to add new operands to positions 2..5 while
        // moving the existing data operands from 2...5 to the higher positions 6...9.
        // If in reversed order, we just need to add new operands to positions 6...9.
        const unsigned int displacement = inOrder ? 4 : 0;
        const unsigned int displacement2 = inOrder ? 0 : 4;
        for (unsigned int k = 2; k < 6; ++k)
        {
            // move existing operand if necessary
            laterInst->setOperand(k + displacement, laterInst->getOperand(k));
            // get the new one from the other instruction
            laterInst->setOperand(k + displacement2, earlierInst->getOperand(k));
        }

        // now take the smaller of the two offsets from the instruction in the current slot
        laterInst->setOperand(0, ii->second.GetInst()->getOperand(0));
        // and update the mask operand
        auto mergedMaskVal = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(laterInst->getParent()->getContext()),
            mergedMask);
        laterInst->setOperand(1, mergedMaskVal);

        // earlier instruction is no longer needed
        earlierInst->eraseFromParent();
        m_bbModified = true;
        ++ii; // skip the next slot since we just considered it as 'next'
        if (nullptr == ii->first.first) // if URB offset is immediate const
        {
            URBWrite8.push_back(laterInst == ii->second.GetInst() ? ii->second : next->second);
        }
    } // for

} // MergeInstructions


std::pair<Value*, unsigned int> MergeURBWrites::GetBaseAndOffset(Value* pUrbOffset)
{
    Value* pBase = pUrbOffset;
    unsigned int offset = 0;

    auto GetConstant = [](Value* pVal)->unsigned int
    {
        IGC_ASSERT(isa<ConstantInt>(pVal));
        ConstantInt* pConst = cast<ConstantInt>(pVal);
        return int_cast<unsigned int>(pConst->getZExtValue());
    };

    if (isa<ConstantInt>(pUrbOffset))
    {
        Value* pNullBase = nullptr;
        return std::make_pair(
            pNullBase,
            GetConstant(pUrbOffset));
    }
    else if (isa<Instruction>(pUrbOffset))
    {
        Instruction* pInstr = cast<Instruction>(pUrbOffset);
        if (pInstr->getOpcode() == Instruction::Add)
        {
            Value* src0 = pInstr->getOperand(0);
            Value* src1 = pInstr->getOperand(1);
            if (isa<ConstantInt>(src1))
            {
                auto baseAndOffset = GetBaseAndOffset(src0);
                pBase = baseAndOffset.first;
                offset = GetConstant(src1) + baseAndOffset.second;
            }
            else if (isa<ConstantInt>(src0))
            {
                auto baseAndOffset = GetBaseAndOffset(src1);
                pBase = baseAndOffset.first;
                offset = GetConstant(src0) + baseAndOffset.second;
            }
        }
    }

    return std::make_pair(pBase, offset);
}

llvm::FunctionPass* IGC::createMergeURBWritesPass()
{
    return new MergeURBWrites();
}

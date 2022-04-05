/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IntrinsicInst.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/IGCIRBuilder.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"
#include "IGCPassSupport.h"

#include "URBPartialWrites.hpp"

using namespace llvm;
namespace IGC
{
// This pass converts URB partial writes into full writes. Depending on platform
// type the minimum URB full-write access granularity (referred as `full-write
// granularity` for the rest of this file) is 32 or 16 bytes.
// A URB partial write is a URB write that writes chunks of data smaller than
// `full-write granularity` or writes data at address not aligned to `full-write
// granularity`.
//
// A URB partial write instruction can be changed to a full-mask write if it can
// be proved that writing additional URB channels does not overwrite data that
// is observable by URBReadOutput instruction in current shader stage or read
// in the next shader stage (i.e. written by other URBWrite instructions in
// current stage).
//
// High-level algorithm:
// For every partial write instruction (A):
//  - check if there exists a different URB write instruction (B) that
//    potentially writes to the same chunk of URB and that B is not strictly
//    dominated by A and does not strictly post-dominates A
//    The dominance/post-dominance condition is not valid in mesh and task
//    shaders. In mesh and task shaders a single URB location may be (partially)
//    written from multiple HW threads.
//  - check if there exists a URB read instruction (C) that potentially reads
//    the same chunk of URB accessed by A and is reachable from A
// If both conditions above are `false` the full mask can be set.
class URBPartialWrites : public FunctionPass
{
public:
    static char ID;

    URBPartialWrites() :
        m_SafeToExtend(std::make_pair(true, true)),
        m_SharedURB(false),
        FunctionPass(ID)
    {
        initializeURBPartialWritesPass(*PassRegistry::getPassRegistry());
    }

    virtual bool runOnFunction(Function& F);

    virtual void getAnalysisUsage(AnalysisUsage& AU) const
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<DominatorTreeWrapperPass>();
        AU.addRequired<PostDominatorTreeWrapperPass>();
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addPreserved<DominatorTreeWrapperPass>();
        AU.addPreserved<PostDominatorTreeWrapperPass>();
        AU.addPreserved<LoopInfoWrapperPass>();
    }

    virtual llvm::StringRef getPassName() const { return "URBPartialWrites"; }

private:
    // URB access descriptor
    // member 0 - runtime URB offset
    // member 1 - constant URB offset
    // member 2 - URBReadOutput or URBWrite intrinsic
    // member 3 - intrinsic instruction number within its basic block
    typedef std::tuple<Value*, uint, GenIntrinsicInst*, uint> URBAccess;

    void GetURBWritesAndReads(Function& F);
    bool ResolvePartialWrites16B();
    bool ResolvePartialWrites32B();
    bool Dominates(
        const URBAccess& access0,
        const URBAccess& access1) const;
    bool PostDominates(
        const URBAccess& access0,
        const URBAccess& access1) const;
    std::pair<bool, bool> IsSafeToExtendChannelMask(
        const URBAccess& write0,
        const URBAccess& access1) const;
    std::pair<bool, bool> CheckURBWrites(
        const URBAccess& write0) const;
    std::pair<bool, bool> CheckURBReads(
        const URBAccess& write0) const;
private:
    const std::pair<bool, bool> m_SafeToExtend;
    bool m_SharedURB; // multiple HW threads may write the same URB location
    std::vector<URBAccess> m_UrbWrites;
    std::vector<URBAccess> m_UrbReads;
};
char URBPartialWrites::ID = 0;

static inline uint GetZExtValue(Value* v)
{
    IGC_ASSERT(isa<ConstantInt>(v));
    return int_cast<uint>(cast<ConstantInt>(v)->getZExtValue());
}

// Returns true if `v` is a llvm::Constant* and is equal to `a`.
static inline bool IsConstAndEqualTo(Value* v, uint a)
{
    if (isa<ConstantInt>(v) && a == GetZExtValue(v))
    {
        return true;
    }
    return false;
}

// Returns `v` casted to llvm::Instruction* if `v` is an llvm::Instruction* with
// opcode equal to the template argument.
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

// Returns the channel mask for a URBReadOutput or URBWrite instruction. For
// URBWrite, if channel mask operand is not a constant the function returns the
// mask of channels that are potentially written by the instruction.
// For URBReadOutput, if all usages are not ExtractElement instruction with
// constant indices the function returns the mask of channels that are
// potentially required.
static inline uint GetChannelMask(GenIntrinsicInst* inst)
{
    if (inst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_URBReadOutput)
    {
        uint mask = 0;
        for (auto iit = inst->user_begin(), eit = inst->user_end();
            iit != eit;
            ++iit)
        {
            ExtractElementInst* ee = dyn_cast<ExtractElementInst>(*iit);
            if (ee && isa<ConstantInt>(ee->getIndexOperand()))
            {
                mask |= 1 << GetZExtValue(ee->getIndexOperand());
            }
            else
            {
                mask = 0xFF;
                break;
            }
        }
        return mask;
    }
    IGC_ASSERT(inst->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_URBWrite);
    Value* channelMask = inst->getOperand(1);
    uint mask = 0xFF;
    if (isa<ConstantInt>(channelMask))
    {
        mask = GetZExtValue(channelMask);
    }
    else
    {
        // Non-constant mask:
        // Pattern 1:
        //  %155 = and i32 %153, 3
        //  %156 = shl i32 1, %155
        // Pattern 2:
        //  %155 = and i32 %153, 3
        //  %156 = shl i32 8, %155
        Instruction* shlInst = dyn_cast<Instruction::Shl>(channelMask);
        Instruction* andInst = shlInst ?
            dyn_cast<Instruction::And>(shlInst->getOperand(1)) : nullptr;
        if (andInst && IsConstAndEqualTo(andInst->getOperand(1), 3))
        {
            if (IsConstAndEqualTo(shlInst->getOperand(0), 1))
            {
                mask = 0x0F;
            }
            if (IsConstAndEqualTo(shlInst->getOperand(0), 8))
            {
                mask = 0xF0;
            }
        }
    }
    // disable channels corresponding to the undef operands
    for (uint i = 0; i < 8; ++i)
    {
        if (isa<UndefValue>(inst->getOperand(2 + i)))
        {
            mask &= ~(1 << i);
        }
    }
    return mask;
}

// Returns true if the `val` is even. If the function returns `false`
// `val` can be even or odd.
static inline bool IsEven(Value* val)
{
    if (val == nullptr)
    {
        return true;
    }
    Instruction* inst = dyn_cast<Instruction>(val);
    if (inst &&
        inst->getNumOperands() == 2 &&
        isa<ConstantInt>(inst->getOperand(1)))
    {
        const uint src1 = GetZExtValue(inst->getOperand(1));
        if (Operator::getOpcode(inst) == Instruction::Shl &&
            src1 > 0)
        {
            return true;
        }
        if (Operator::getOpcode(inst) == Instruction::Mul &&
            (src1 % 2) == 0)
        {
            return true;
        }
    }
    return false;
}

// Collects all URBWrite and URBReadOutput instructions in a function.
void URBPartialWrites::GetURBWritesAndReads(Function& F)
{
    for (auto& BB : F)
    {
        uint instCounter = 0;
        for (auto II = BB.begin(), IE = BB.end();
            II != IE;
            ++II, ++instCounter)
        {
            GenIntrinsicInst* intr = dyn_cast<GenIntrinsicInst>(&(*II));
            if (intr &&
                intr->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_URBWrite)
            {
                std::pair<Value*, uint> baseAndOffset =
                    GetURBBaseAndOffset(intr->getOperand(0));
                m_UrbWrites.push_back(std::make_tuple(
                    baseAndOffset.first,
                    baseAndOffset.second,
                    intr,
                    instCounter));
            }
            else if (intr &&
                intr->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_URBReadOutput)
            {
                std::pair<Value*, uint> baseAndOffset =
                    GetURBBaseAndOffset(intr->getOperand(0));
                m_UrbReads.push_back(std::make_tuple(
                    baseAndOffset.first,
                    baseAndOffset.second,
                    intr,
                    instCounter));
            }
        }
    }
}

// Checks if extending the channel mask of the `write0` may potentially
// overwrite data accessed by `access1`. The returned value is a pair of `bool`
// values, the first member corresponds to the lower 16-byte chunk and the
// second member corresponds to the higher 16-byte chunk. If true is returned it
// means that the channel mask of corresponding 16-byte chunk can be extended
// to full mask.
std::pair<bool, bool> URBPartialWrites::IsSafeToExtendChannelMask(
    const URBAccess& write0,
    const URBAccess& access1) const
{
    Value* const offset0 = std::get<0>(write0);
    const uint constOffset0 = std::get<1>(write0);
    GenIntrinsicInst* const intr0 = std::get<2>(write0);
    IGC_ASSERT(intr0->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_URBWrite);
    const bool immMask0 = isa<ConstantInt>(intr0->getOperand(1));
    const uint mask0 = GetChannelMask(intr0);
    Value* const offset1 = std::get<0>(access1);
    const uint constOffset1 = std::get<1>(access1);
    GenIntrinsicInst* const intr1 = std::get<2>(access1);
    const uint mask1 = GetChannelMask(intr1);

    // Get lower 4 bits of channel mask
    auto Lo = [](uint mask)->uint
    {
        return mask & 0xF;
    };
    // Get higher 4 bits of channel mask
    auto Hi = [](uint mask)->uint
    {
        return (mask >> 4) & 0xF;
    };
    // Returns true if full mask can be set.
    auto SafeToSetFullMask = [this, immMask0](
        uint mask0,
        uint mask1)->bool
    {
        if (mask1 == 0)
        {
            // not overlapping accesses
            return true;
        }
        if (immMask0 &&
            !m_SharedURB &&
            (mask0 | mask1) == mask0)
        {
            // The second URB access is for a subset of channels accessed by the
            // first access.
            return true;
        }
        return false;
    };

    bool overlapLo = false;
    bool overlapHi = false;
    if ((offset0 == nullptr && offset1 == nullptr) ||
        offset0 == offset1)
    {
        if (constOffset0 == constOffset1)
        {
            // e.g.:
            // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 1, ...)
            // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 18, ...)
            if (!SafeToSetFullMask(Lo(mask0), Lo(mask1)))
            {
                overlapLo = true;
            }
            // e.g.:
            // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 1, ...)
            // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 18, ...)
            if (!SafeToSetFullMask(Hi(mask0), Hi(mask1)))
            {
                overlapHi = true;
            }
        }
        // e.g.:
        // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 16, ...)
        // call void @llvm.genx.GenISA.URBWrite(i32 2, i32 2, ...)
        else if (constOffset0 + 1 == constOffset1 &&
                !SafeToSetFullMask(Hi(mask0), Lo(mask1)))
        {
            overlapHi = true;
        }
        // e.g.:
        // call void @llvm.genx.GenISA.URBWrite(i32 2, i32 2, ...)
        // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 16, ...)
        else if (constOffset1 + 1 == constOffset0 &&
                 !SafeToSetFullMask(Lo(mask0), Hi(mask1)))
        {
            overlapLo = true;
        }
    }
    // e.g.:
    // %offset0 = and i32 %153, 2
    // call void @llvm.genx.GenISA.URBWrite(i32 offset0, i32 2, ...)
    // call void @llvm.genx.GenISA.URBWrite(i32 0, i32 16, ...)
    else if ((offset0 != nullptr && offset1 == nullptr) &&
        (constOffset1 < constOffset0 && constOffset1 + 2 <= constOffset0))
    {
        // no ovelap
    }
    // e.g.:
    // call void @llvm.genx.GenISA.URBWrite(i32 0, i32 2, ...)
    // %offset1 = and i32 %153, 2
    // call void @llvm.genx.GenISA.URBWrite(i32 offset1, i32 16, ...)
    else if ((offset0 == nullptr && offset1 != nullptr) &&
        (constOffset0 < constOffset1 && constOffset0 + 2 <= constOffset1))
    {
        // no ovelap
    }
    // e.g.:
    // %offset0 = and i32 %153, 2
    // call void @llvm.genx.GenISA.URBWrite(i32 offset0, i32 2, ...)
    // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 16, ...)
    else if ((offset0 != nullptr && offset1 == nullptr) &&
        constOffset1 + 1 == constOffset0 &&
        !SafeToSetFullMask(Lo(mask0), Hi(mask1)))
    {
        overlapLo = true;
    }
    // e.g.:
    // call void @llvm.genx.GenISA.URBWrite(i32 1, i32 32, ...)
    // %offset1 = and i32 %153, 2
    // call void @llvm.genx.GenISA.URBWrite(i32 offset1, i32 2, ...)
    else if ((offset0 == nullptr && offset1 != nullptr) &&
        constOffset0 + 1 == constOffset1 &&
        !SafeToSetFullMask(Hi(mask0), Lo(mask1)))
    {
        overlapHi = true;
    }
    else
    {
        overlapLo = true;
        overlapHi = true;
    }
    return std::make_pair(!overlapLo, !overlapHi);
}

// Checks if there exists a URBWrite instruction (different than `write0`) such
// that:
// - it is not strictly dominated by `write0` and does not strictly
//   post-dominates `write0`
// - writes to URB channels that are not written by `write0` but belong to the
//   32-byte chunk accessed by `write0`
// The returned value is a pair of `bool` values, the first member corresponds
// to the lower 16-byte chunk and the second member corresponds to the higher
// 16-byte chunk. If true is returned it means that the channel mask of
// corresponding 16-byte chunk can be extended to full mask.
std::pair<bool, bool> URBPartialWrites::CheckURBWrites(
    const URBAccess& write0) const
{
    std::pair<bool, bool> safeToExtend = m_SafeToExtend;
    for (uint k = 0; k < m_UrbWrites.size(); ++k)
    {
        const URBAccess& write1 = m_UrbWrites[k];
        std::pair<bool, bool> extendMask = IsSafeToExtendChannelMask(write0, write1);
        if (extendMask == m_SafeToExtend)
        {
            continue;
        }
        if (write0 != write1 &&
            !Dominates(write0, write1) &&
            !PostDominates(write1, write0))
        {
            safeToExtend.first &= extendMask.first;
            safeToExtend.second &= extendMask.second;
        }
        // In Mesh and Task shaders URB may be written from multiple HW threads.
        else if (write0 != write1 && m_SharedURB)
        {
            safeToExtend.first &= extendMask.first;
            safeToExtend.second &= extendMask.second;
        };
        if (!safeToExtend.first && !safeToExtend.second)
        {
            break;
        }
    }
    return safeToExtend;
}

// Checks if there exists a URBReadOutput instruction such that:
// - URBReadOutput is reachable from `write0`
// - reads URB channels that are not written by `write0` but belong to the
//   32-byte chunk accessed by `write0`
// The returned value is a pair of `bool` values, the first member corresponds
// to the lower 16-byte chunk and the second member corresponds to the higher
// 16-byte chunk. If true is returned it means that the channel mask of
// corresponding 16-byte chunk can be extended to full mask.
std::pair<bool, bool> URBPartialWrites::CheckURBReads(
    const URBAccess& write) const
{
    DominatorTree* DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    GenIntrinsicInst* const writeIntr = std::get<2>(write);
    const bool immMask = isa<ConstantInt>(writeIntr->getOperand(1));
    const uint writeMask = GetChannelMask(writeIntr);
    std::pair<bool, bool> safeToExtend = m_SafeToExtend;
    for (uint i = 0; i < m_UrbReads.size(); ++i)
    {
        const URBAccess& read = m_UrbReads[i];
        GenIntrinsicInst* const readIntr = std::get<2>(read);
        const uint readMask1 = GetChannelMask(readIntr);
        std::pair<bool, bool> extendMask = IsSafeToExtendChannelMask(write, read);
        if (extendMask == m_SafeToExtend)
        {
            continue;
        }
        // Note: this method can potentially be refined by checking if between
        // the `write` and the `read` exists a different URBWrite that
        // overwrites the URB chunk written by `write`.
        if (isPotentiallyReachable(writeIntr, readIntr, nullptr, DT))
        {
            safeToExtend.first &= extendMask.first;
            safeToExtend.second &= extendMask.second;
            if (!safeToExtend.first && !safeToExtend.second)
            {
                break;
            }
        }
    }
    return safeToExtend;
}

// Returns true if `access0` strictly dominates `access1`
bool URBPartialWrites::Dominates(
    const URBAccess& access0,
    const URBAccess& access1) const
{
    GenIntrinsicInst* const intr0 = std::get<2>(access0);
    GenIntrinsicInst* const intr1 = std::get<2>(access1);
    if (intr0 == intr1)
    {
        return false;
    }
    if (intr0->getParent() == intr1->getParent())
    {
        return std::get<3>(access0) < std::get<3>(access1);
    }
    DominatorTree* DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    return DT->dominates(intr0->getParent(), intr1->getParent());
}

// Returns true if `access0` strictly post-dominates `access1`
bool URBPartialWrites::PostDominates(
    const URBAccess& access0,
    const URBAccess& access1) const
{
    GenIntrinsicInst* const intr0 = std::get<2>(access0);
    GenIntrinsicInst* const intr1 = std::get<2>(access1);
    if (intr0 == intr1)
    {
        return false;
    }
    if (intr0->getParent() == intr1->getParent())
    {
        return std::get<3>(access0) > std::get<3>(access1);
    }
    PostDominatorTree* PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    return PDT->properlyDominates(intr0->getParent(), intr1->getParent());
}

// Converts URB partial writes into full writes on platforms with `full-write
// granularity` of 32 bytes.
bool URBPartialWrites::ResolvePartialWrites32B()
{
    bool modified = false;
    LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    for (uint i = 0; i < m_UrbWrites.size(); ++i)
    {
        const URBAccess& write0 = m_UrbWrites[i];
        Value* const offset0 = std::get<0>(write0);
        const uint constOffset0 = std::get<1>(write0);
        GenIntrinsicInst* const intr0 = std::get<2>(write0);
        const bool immMask0 = isa<ConstantInt>(intr0->getOperand(1));
        const uint mask0 = GetChannelMask(intr0);
        if (immMask0 && mask0 == 0xFF)
        {
            // not a partial write
            continue;
        }
        if (!immMask0 && LI->getLoopFor(intr0->getParent()))
        {
            // not immediate mask and in a loop
            continue;
        }
        if (!IsEven(offset0) || (constOffset0 % 2) == 1)
        {
            // unaligned URB write
            continue;
        }
        if (m_SafeToExtend == CheckURBWrites(write0) &&
            m_SafeToExtend == CheckURBReads(write0))
        {
            Value* fullMask = ConstantInt::get(
                intr0->getOperand(1)->getType(),
                0xFF);
            intr0->setOperand(1, fullMask);
            modified = true;
        }
    }
    return modified;
}

// Converts URB partial writes into full writes on platforms with `full-write
// granularity` of 16 bytes.
bool URBPartialWrites::ResolvePartialWrites16B()
{
    bool modified = false;
    LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    for (uint i = 0; i < m_UrbWrites.size(); ++i)
    {
        const URBAccess& write0 = m_UrbWrites[i];
        GenIntrinsicInst* const intr0 = std::get<2>(write0);
        const bool immMask0 = isa<ConstantInt>(intr0->getOperand(1));
        const uint mask0 = GetChannelMask(intr0);
        if (immMask0 &&
            (mask0 == 0xFF || mask0 == 0x0F || mask0 == 0xF0))
        {
            // not a partial write
            continue;
        }
        if (!immMask0 && LI->getLoopFor(intr0->getParent()))
        {
            // not immediate mask and in a loop
            continue;
        }
        std::pair<bool, bool> extendMask = CheckURBWrites(write0);
        std::pair<bool, bool> extendMask1 = CheckURBReads(write0);
        bool extendLo = extendMask.first && extendMask1.first;
        bool extendHi = extendMask.second && extendMask1.second;
        if (extendLo || extendHi)
        {
            uint newMask0 = mask0;
            if (extendLo && (mask0 & 0x0F) != 0)
            {
                newMask0 |= 0x0F;
            }
            if (extendHi && (mask0 & 0xF0) != 0)
            {
                newMask0 |= 0xF0;
            }
            Type* maskType = intr0->getOperand(1)->getType();
            Value* newMask = ConstantInt::get(maskType, newMask0);
            if ((!immMask0 && extendLo && extendHi) ||
                (!immMask0 && extendLo && (mask0 & 0xF0) == 0) ||
                (!immMask0 && extendHi && (mask0 & 0x0F) == 0) ||
                (immMask0 && newMask0 != mask0))
            {
                intr0->setOperand(1, newMask);
                modified = true;
            }
            else if (!immMask0)
            {
                IGCIRBuilder<> builder(intr0);
                uint orMask = extendHi ? 0xF0 : 0x0F;
                if ((orMask & mask0) == orMask)
                {
                    newMask = builder.CreateOr(
                        intr0->getOperand(1),
                        ConstantInt::get(maskType, orMask));
                    intr0->setOperand(1, newMask);
                    modified = true;
                }
            }
        }
    }
    return modified;
}

bool URBPartialWrites::runOnFunction(Function& F)
{
    bool modified = false;
    m_UrbWrites.clear();
    m_UrbReads.clear();

    GetURBWritesAndReads(F);

    const CodeGenContext* const ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (ctx->type == ShaderType::MESH_SHADER ||
        ctx->type == ShaderType::TASK_SHADER)
    {
        // In Mesh and Task shaders a single URB location may be written from
        // multiple HW threads or different lanes inside a single HW thread.

        //TODO if we can prove uniformity across urb inputs then we can enable
        //this for future uniform urb writes
        m_SharedURB = true;
    }
    const uint fullWriteGranularity = ctx->platform.getURBFullWriteMinGranularity();
    if (fullWriteGranularity == 16)
    {
        modified = ResolvePartialWrites16B();
    }
    else
    {
        IGC_ASSERT(fullWriteGranularity == 32);
        modified = ResolvePartialWrites32B();
    }
    return modified;
}

llvm::FunctionPass* createURBPartialWritesPass()
{
    return new URBPartialWrites();
}

#define PASS_FLAG "igc-urb-partial-writes"
#define PASS_DESCRIPTION "Convert URB partial writes to full-mask writes."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(URBPartialWrites, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
    IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(URBPartialWrites, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

} // namespace IGC

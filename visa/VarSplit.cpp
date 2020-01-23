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

#include "VarSplit.h"
#include "GraphColor.h"

namespace vISA
{

VarSplitPass::VarSplitPass(G4_Kernel& k) : kernel(k)
{
}

void VarSplitPass::run()
{
    findSplitCandidates();

    split();
}

void VarSplitPass::findSplitCandidates()
{
    auto canSplit = [](G4_INST* inst)
    {
        // Insert any new split candidates here
        return (inst->isSend() && inst->getMsgDesc()->isSampler() && inst->getMsgDesc()->ResponseLength() > 2);
    };

    // Find all dcls that can be split in to smaller chunks
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (canSplit(inst))
            {
                auto dstDcl = inst->getDst()->getTopDcl();
                if (dstDcl && dstDcl->getRegFile() == G4_RegFileKind::G4_GRF)
                {
                    auto& prop = splitVars[dstDcl];
                    prop.numDefs++;
                    prop.def = std::make_pair(inst->getDst(), bb);
                }
            }

            for (unsigned int s = 0; s != G4_MAX_SRCS; s++)
            {
                auto src = inst->getSrc(s);
                if (!src || !src->isSrcRegRegion() || !src->asSrcRegRegion()->getTopDcl())
                    continue;

                auto srcRgn = src->asSrcRegRegion();
                auto dcl = srcRgn->getTopDcl();

                if (dcl->getNumRows() < 2)
                    continue;

                // It is possible that splitVars map doesnt have an entry for dcl
                // yet as in CFG, a use may appear lexically earlier than its def.
                auto& prop = splitVars[dcl];
                prop.srcs.push_back(std::make_pair(srcRgn,bb));
            }
        }
    }

    // Now filter out real candidates
    for (auto itemIt = splitVars.begin(); itemIt != splitVars.end(); itemIt++)
    {
        auto& item = (*itemIt);
        if (item.second.numDefs != 1)
        {
            item.second.legitCandidate = false;
            continue;
        }

        // Check whether each src operand is independent and compute size
        for (auto& srcpair : item.second.srcs)
        {
            auto src = srcpair.first;
            auto numRows = (src->getRightBound() - src->getLeftBound() + G4_GRF_REG_NBYTES - 1) / G4_GRF_REG_NBYTES;
            auto regOff = src->getRegOff();

            if (numRows == 1)
            {
                if (item.second.ag == VarProperties::AccessGranularity::Unknown)
                    item.second.ag = VarProperties::AccessGranularity::OneGrf;
                else if (item.second.ag == VarProperties::AccessGranularity::TwoGrf)
                {
                    item.second.legitCandidate = false;
                    break;
                }
            }
            else if (numRows == 2)
            {
                if (item.second.ag == VarProperties::AccessGranularity::Unknown)
                    item.second.ag = VarProperties::AccessGranularity::TwoGrf;
                else if (item.second.ag == VarProperties::AccessGranularity::OneGrf)
                {
                    item.second.legitCandidate = false;
                    break;
                }
                else if (regOff % 2 != 0)
                {
                    item.second.legitCandidate = false;
                    break;
                }
            }
        }
    }

    for (auto itemIt = splitVars.begin(); itemIt != splitVars.end();)
    {
        auto item = (*itemIt);
        if (!item.second.legitCandidate ||
            item.second.ag == VarProperties::AccessGranularity::Unknown ||
            item.second.numDefs > 1)
        {
            itemIt = splitVars.erase(itemIt);
            continue;
        }
        itemIt++;
    }

    // Each entry in splitVars map is a split candidate
}

void VarSplitPass::split()
{
    auto getIter = [](G4_INST* inst, G4_BB* bb)
    {
        for(auto iter = bb->begin(); iter != bb->end(); iter++)
        {
            if (*iter == inst)
                return iter;
        }
        return bb->end();
    };

#ifdef DEBUG_VERBOSE_ON
    unsigned int numIntrinsicsInserted = 0;
#endif
    // Do actual splitting
    for (auto& item : splitVars)
    {
        MUST_BE_TRUE(item.second.legitCandidate, "Cannot split non-candidate");

        // Insert intrinsics
        auto it = getIter(item.second.def.first->getInst(), item.second.def.second);
        MUST_BE_TRUE(it != item.second.def.second->end(), "Instruction not found in BB");
        it++;

        auto dstDcl = item.second.def.first->getTopDcl();
        unsigned int numIntrinInsts = 0;
        if (item.second.ag == VarProperties::AccessGranularity::OneGrf)
            numIntrinInsts = dstDcl->getNumRows();
        else if (item.second.ag == VarProperties::AccessGranularity::TwoGrf)
            numIntrinInsts = dstDcl->getNumRows() / 2;
        else
            MUST_BE_TRUE(false, "unsupported access granularity");

        // lb, rb, split dcl
        std::vector<std::tuple<unsigned int, unsigned int, G4_Declare*>> splitDcls;
        for (unsigned int i = 0; i != numIntrinInsts; i++)
        {
            unsigned int numRows = item.second.ag == VarProperties::AccessGranularity::TwoGrf ? 2 : 1;
            unsigned int lb = getGRFSize() * i*numRows;
            unsigned int rb = lb + (getGRFSize()*numRows)-1;

            auto name = kernel.fg.builder->getNameString(kernel.fg.mem, 50, "%s_%d_%d_%d", dstDcl->getName(), i, lb, rb);
            auto splitDcl = kernel.fg.builder->createDeclareNoLookup((const char*)name,
                G4_RegFileKind::G4_GRF, getGRFSize() / G4_Type_Table[Type_UD].byteSize, numRows, Type_UD);
            splitParentDcl.insert(std::make_pair(splitDcl, dstDcl));
            splitChildren[dstDcl].push_back(splitDcl);

            // If this part of dcl is never used in code, then dont create split intrinsic inst for it
            if (!item.second.isPartDclUsed(lb, rb))
            {
                unusedDcls.insert(splitDcl);
                continue;
            }

            auto dstRgn = kernel.fg.builder->Create_Dst_Opnd_From_Dcl(splitDcl, 1);
            auto srcRgn = kernel.fg.builder->createSrcRegRegion(Mod_src_undef, Direct, dstDcl->getRegVar(),
                item.second.def.first->getRegOff() + (i * numRows), item.second.def.first->getSubRegOff(), kernel.fg.builder->getRegionStride1(), Type_UD);
            unsigned int esize = (getGRFSize() / G4_Type_Table[Type_UD].byteSize) * numRows;
            auto intrin = kernel.fg.builder->createIntrinsicInst(nullptr, Intrinsic::Split, esize, dstRgn, srcRgn, nullptr, nullptr, item.second.def.first->getInst()->getOption(), item.second.def.first->getInst()->getLineNo());
            item.second.def.second->insert(it, intrin);
            splitDcls.push_back(std::make_tuple(lb, rb, splitDcl));
#ifdef DEBUG_VERBOSE_ON
            numIntrinsicsInserted++;
#endif
        }

        auto getSplitDcl = [&splitDcls](unsigned int lb, unsigned int rb)
        {
            for (auto& item : splitDcls)
            {
                if (std::get<0>(item) == lb &&
                    std::get<1>(item) == rb)
                    return std::get<2>(item);
            }
            return (G4_Declare*)nullptr;
        };

        auto getOpndNum = [](G4_SrcRegRegion* src)
        {
            auto inst = src->getInst();
            for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
            {
                if (inst->getSrc(i) == src)
                    return i;
            }
            MUST_BE_TRUE(false, "src operand not found");
            return 0xffffffff;
        };

        // Replace all src operands
        for (auto& s : item.second.srcs)
        {
            auto srcRgn = s.first;

            auto lb = srcRgn->getLeftBound();
            auto rb = srcRgn->getRightBound();

            auto dcl = getSplitDcl(lb, rb);
            MUST_BE_TRUE(dcl, "Didnt find split dcl");

            auto newSrc = kernel.fg.builder->createSrcRegRegion(srcRgn->getModifier(), srcRgn->getRegAccess(), dcl->getRegVar(), 0, srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
            auto opndNum = getOpndNum(srcRgn);
            srcRgn->getInst()->setSrc(newSrc, opndNum);
        }
    }

    DEBUG_VERBOSE("Number of split intrinsincs inserted " << numIntrinsicsInserted << std::endl);
}

void VarSplitPass::replaceIntrinsics()
{
#ifdef DEBUG_VERBOSE_ON
    unsigned int numSplitMovs = 0;
#endif
    // Replace intrinsic.split with mov
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (inst->isSplitIntrinsic())
            {
                inst->setOpcode(G4_mov);
#ifdef DEBUG_VERBOSE_ON
                if(inst->getDst()->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum() !=
                    inst->getSrc(0)->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum())
                    numSplitMovs++;
#endif
            }
        }
    }

    DEBUG_VERBOSE("Number of split movs left behind " << numSplitMovs << std::endl);
}

G4_Declare* VarSplitPass::getParentDcl(G4_Declare* splitDcl)
{
    auto it = splitParentDcl.find(splitDcl);
    if (it != splitParentDcl.end())
        return (*it).second;
    return nullptr;
}

std::vector<G4_Declare*>* VarSplitPass::getChildren(G4_Declare* parent)
{
    auto it = splitChildren.find(parent);
    if (it == splitChildren.end())
        return nullptr;
    return &(*it).second;
}

std::vector<G4_Declare*> VarSplitPass::getSiblings(G4_Declare* s)
{
    // Return all siblings of s, excluding s
    std::vector<G4_Declare*> siblings;

    // First lookup parent
    auto parentDcl = getParentDcl(s);
    if (!parentDcl)
        return siblings;

    // Lookup all children of parent
    auto children = getChildren(parentDcl);
    if (!children)
        return siblings;

    for (auto item : *children)
    {
        if (item != s)
            siblings.push_back(item);
    }

    return siblings;
}

bool VarSplitPass::isSplitDcl(G4_Declare* d)
{
    auto it = splitChildren.find(d);
    if (it == splitChildren.end())
        return false;
    return true;
}

bool VarSplitPass::isPartialDcl(G4_Declare* d)
{
    auto it = splitParentDcl.find(d);
    if (it == splitParentDcl.end())
        return false;
    return true;
}

unsigned int VarSplitPass::getSiblingNum(G4_Declare* d)
{
    // This function must be invoked assuming d is a partial dcl.
    // Otherwise behavior is unpredictable.
    if (!isPartialDcl(d))
        return 0;

    unsigned int siblingNum = 0;
    // First lookup parent
    auto parentDcl = getParentDcl(d);

    // Lookup all children of parent
    auto children = getChildren(parentDcl);

    for (auto s : *children)
    {
        if (s == d)
            break;
        siblingNum++;
    }
    return siblingNum;
}

unsigned int VarSplitPass::getIdealAllocation(G4_Declare* dcl, LiveRange** lrs)
{
    // This function is invoked when assigning GRFs to parent or child dcl.
    // If parent is already allocated a GRF then check which child dcl is,
    // and return GRF number that will allow coalescing.
    unsigned int idealGRF = 0;
    if (isSplitDcl(dcl))
    {
        // This is parent dcl
        auto children = getChildren(dcl);
        // Try coalescing with first child that is allocated
        unsigned int childNo = 0;
        for (auto child : *children)
        {
            if (child->getRegVar()->isRegAllocPartaker())
            {
                auto id = child->getRegVar()->getId();
                auto lr = lrs[id];
                auto phyReg = lr->getPhyReg();
                if (phyReg && !isChildDclUnused(child))
                {
                    auto regNum = phyReg->asGreg()->getRegNum();
                    if (regNum >= (childNo * child->getNumRows()))
                    {
                        idealGRF = regNum - (childNo * child->getNumRows());
                        break;
                    }
                }
            }
            childNo++;
        }
    }
    else if (isPartialDcl(dcl))
    {
        // First try coalescing with parent, if it already allocated
        auto parent = getParentDcl(dcl);
        auto lr = lrs[parent->getRegVar()->getId()];
        auto phyRegParent = lr->getPhyReg();
        if (phyRegParent)
        {
            idealGRF = phyRegParent->asGreg()->getRegNum() + (getSiblingNum(dcl) * dcl->getNumRows());
        }
        else
        {
            // Check whether any other sibling is allocated
            auto siblings = getSiblings(dcl);
            unsigned int i = 0;
            for (auto sibling : siblings)
            {
                if (sibling->getRegVar()->isRegAllocPartaker())
                {
                    auto phyRegSibling = lrs[sibling->getRegVar()->getId()]->getPhyReg();
                    if (phyRegSibling)
                    {
                        unsigned int parentGRF = phyRegSibling->asGreg()->getRegNum() - (i * sibling->getNumRows());
                        idealGRF = parentGRF + (getSiblingNum(dcl) * sibling->getNumRows());
                    }
                }
                i++;
            }
        }
    }

    idealGRF = idealGRF >= getGRFSize() ? 0 : idealGRF;

    return idealGRF;
}

bool VarSplitPass::isChildDclUnused(G4_Declare* dcl)
{
    auto it = unusedDcls.find(dcl);
    if (it == unusedDcls.end())
        return false;
    return true;
}

void VarSplitPass::writeHints(G4_Declare* dcl, LiveRange** lrs)
{
    bool isParent = isSplitDcl(dcl);
    bool isChild = isPartialDcl(dcl);

    // If parent, then set hints in all children
    if (isParent)
    {
        auto regNum = lrs[dcl->getRegVar()->getId()]->getPhyReg()->asGreg()->getRegNum();
        auto children = getChildren(dcl);
        unsigned int i = 0;
        for (auto child : *children)
        {
            if (child->getRegVar()->isRegAllocPartaker())
            {
                auto id = child->getRegVar()->getId();
                lrs[id]->setAllocHint(regNum + (i * child->getNumRows()));
            }
            i++;
        }
    }
    else if (isChild)
    {
        // Write hint to parent if its empty
        auto parentDcl = getParentDcl(dcl);
        auto idealParentGRF = getIdealAllocation(parentDcl, lrs);
        if (idealParentGRF != 0)
        {
            lrs[parentDcl->getRegVar()->getId()]->setAllocHint(idealParentGRF);
            auto siblings = getSiblings(dcl);
            unsigned int i = 0;
            for (auto sibling : siblings)
            {
                if (sibling->getRegVar()->isRegAllocPartaker())
                {
                    lrs[sibling->getRegVar()->getId()]->setAllocHint(idealParentGRF + (i * sibling->getNumRows()));
                }
                i++;
            }
        }
    }
}

};
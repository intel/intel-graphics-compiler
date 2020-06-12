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

void VarSplitPass::buildPreVerify()
{
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (inst->getDst())
            {
                splitVerify[inst].dst = inst->getDst();
                splitVerify[inst].dstLb = inst->getDst()->getLeftBound();
                splitVerify[inst].dstRb = inst->getDst()->getRightBound();
            }
            for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
            {
                if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion())
                {
                    splitVerify[inst].src[i] = inst->getSrc(i);
                    splitVerify[inst].srcLb[i] = inst->getSrc(i)->getLeftBound();
                    splitVerify[inst].srcRb[i] = inst->getSrc(i)->getRightBound();
                }
            }
        }
    }
}

void VarSplitPass::verify()
{
    // verify
    // parent, <child, <lb, rb>>
    std::unordered_map<G4_Declare*, std::unordered_map<G4_Declare*, std::pair<unsigned int, unsigned int>>> parentSplit;
    std::unordered_set<G4_Declare*> splitDcls;
    unsigned int numSplitIntrinsics = 0;

    auto getChildData = [&](G4_Declare* child)
    {
        std::pair<unsigned int, unsigned int> childLbRb = { 0,0 };
        bool found = false;
        for (auto& item : parentSplit)
        {
            for (auto& itemCh : (item).second)
            {
                if (itemCh.first == child)
                {
                    MUST_BE_TRUE(!found, "Duplicate lb/rb entry found");
                    childLbRb = itemCh.second;
                    found = true;
                }
            }
        }

        MUST_BE_TRUE(found, "Didnt find child dcl");
        return childLbRb;
    };

    // create parent->child mapping
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (inst->isSplitIntrinsic())
            {
                // ensure this is split mov instruction
                MUST_BE_TRUE(inst->isSplitIntrinsic(), "Didnt expect new non-split intrinsic instruction");

                // verify that split instruction's dst, src(0) is correct
                splitDcls.insert(inst->getDst()->getTopDcl());

                MUST_BE_TRUE(!inst->getSrc(0)->getTopDcl()->getAddressed(), "Shouldnt split indirectly addressed variable");

                auto origSrc0 = inst->getSrc(0)->asSrcRegRegion();
                auto origLb = origSrc0->getLeftBound();
                auto origRb = origSrc0->getRightBound();
                auto itemToInsert = std::make_pair(inst->getDst()->getTopDcl(), std::make_pair((unsigned int)origLb, (unsigned int)origRb));
                parentSplit[origSrc0->getTopDcl()].insert(itemToInsert);

                numSplitIntrinsics++;
            }
        }
    }

    // now check whether usage of child is correct
    std::unordered_map<G4_Declare*, unsigned int> parentDefCount;
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            auto dst = inst->getDst();

            if (dst && splitDcls.find(dst->getTopDcl()) != splitDcls.end())
            {
                MUST_BE_TRUE(inst->isSplitIntrinsic(), "Found split dcl as dst in non-split intrinsic instruction");
            }

            if (dst && parentSplit.find(dst->getTopDcl()) != parentSplit.find(dst->getTopDcl()))
            {
                auto oldDefCount = parentDefCount[dst->getTopDcl()];
                parentDefCount[dst->getTopDcl()] = oldDefCount + 1;
                MUST_BE_TRUE(oldDefCount == 0, "Found second def of parent of split variable");
            }

            for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
            {
                auto src = inst->getSrc(i);
                if (!src || !src->asSrcRegRegion())
                    continue;

                if (parentSplit.find(src->asSrcRegRegion()->getTopDcl()) != parentSplit.end())
                {
                    MUST_BE_TRUE(inst->isSplitIntrinsic(), "Found src opnd using pre-split parent");
                }

                if (splitDcls.find(src->asSrcRegRegion()->getTopDcl()) == splitDcls.end())
                    continue; // not a split dcl

                // src is a split dcl, verify its usage is consistent with pre-transformation data structure
                auto lb = src->getLeftBound();
                auto rb = src->getRightBound();

                auto childData = getChildData(src->asSrcRegRegion()->getTopDcl());
                auto childLb = childData.first;
                //auto childRb = childData.second;

                auto totalLb = childLb + lb;
                auto totalRb = childLb + rb;

                auto origInstData = splitVerify[inst];
                auto origLb = origInstData.srcLb[i];
                auto origRb = origInstData.srcRb[i];

                MUST_BE_TRUE(origLb == totalLb, "Mismatch in lb");
                MUST_BE_TRUE(origRb == totalRb, "Mismatch in rb");
            }
        }
    }

    printf("Split verification passed successfully - %d split!\n", numSplitIntrinsics);
}

void VarSplitPass::verifyOverlap()
{
    // For defs, map GRF written with G4_Declare of dst rgn.
    // Verify that src reg# used in intrinsic_split comes from
    // parent dcl of the split dst.
    std::unordered_map<unsigned int, G4_Declare*> regToDcl;
    unsigned int numSplitLeft = 0;
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (inst->isSplitIntrinsic())
            {
                auto dst = inst->getDst();
                auto src = inst->getSrc(0);
                auto dstTopDcl = dst->getTopDcl();
                auto srcTopDcl = src->getTopDcl();
                MUST_BE_TRUE(dstTopDcl->getRegVar()->getPhyReg() &&
                    dstTopDcl->getRegVar()->getPhyReg()->isGreg(), "Unexpected assignment condition on split dst");
                MUST_BE_TRUE(srcTopDcl->getRegVar()->getPhyReg() &&
                    srcTopDcl->getRegVar()->getPhyReg()->isGreg(), "Unexpected assignment condition on split src");

                auto dstLS = dst->getLinearizedStart();
                auto dstLE = dst->getLinearizedEnd();

                auto srcLS = src->getLinearizedStart();
                auto srcLE = src->getLinearizedEnd();

                if (dstLS == srcLS &&
                    dstLE == srcLE)
                {
                    // assignment is ok and will be coalesced away
                    continue;
                }

                numSplitLeft++;

                auto srcDclRegNum = srcLS / G4_GRF_REG_NBYTES;
                auto srcDclNumRows = ((srcLE + G4_GRF_REG_NBYTES - 1) / G4_GRF_REG_NBYTES) - srcDclRegNum;

                // check whether src GRF# is written by parent of dst split dcl
                for (unsigned int i = srcDclRegNum; i != (srcDclRegNum + srcDclNumRows); i++)
                {
                    auto dcl = regToDcl[i];
                    if (dcl != getParentDcl(dstTopDcl))
                    {
                        if (!dstTopDcl->getRegVar()->isRegVarTransient() &&
                            !dstTopDcl->getRegVar()->isRegVarCoalesced())
                        {
                            MUST_BE_TRUE(false, "split src uses GRF value from non-parent");
                        }
                        else if(dstTopDcl->getRegVar()->isRegVarTransient())
                        {
                            // dst is spilled
                            auto dstOrigDcl = dstTopDcl->getRegVar()->getNonTransientBaseRegVar()->getDeclare()->getRootDeclare();
                            if (dcl != getParentDcl(dstOrigDcl))
                            {
                                MUST_BE_TRUE(false, "split src uses GRF value from non-parent. dst spilled.");
                            }
                        }
                        else
                        {
                            // do nothing for coalesced ranges
                        }
                    }
                }
            }
            else
            {
                auto dstRgn = inst->getDst();
                if (dstRgn && dstRgn->getTopDcl() &&
                    dstRgn->getTopDcl()->getRegVar()->getPhyReg() &&
                    dstRgn->getTopDcl()->getRegVar()->getPhyReg()->isGreg())
                {
                    auto grf = dstRgn->getTopDcl()->getRegVar()->getPhyReg()->asGreg()->getRegNum();
                    auto numRows = (dstRgn->getLinearizedEnd() - dstRgn->getLinearizedStart() + G4_GRF_REG_NBYTES - 1) / G4_GRF_REG_NBYTES;
                    for (unsigned int i = grf; i != (grf + numRows); i++)
                    {
                        regToDcl[i] = dstRgn->getTopDcl();
                    }
                }
            }
        }
    }

    printf("Split assignment overlap passed successfully - %d splits left\n", numSplitLeft);
}

void VarSplitPass::run()
{
    if (kernel.getOption(vISA_VerifyExplicitSplit))
        buildPreVerify();

    findSplitCandidates();

    split();

    if (kernel.getOption(vISA_VerifyExplicitSplit))
        verify();
}

void VarSplitPass::findSplitCandidates()
{
    auto canSplit = [](G4_INST* inst)
    {
        // Insert any new split candidates here
        return (inst->isSend() && inst->getMsgDesc()->isSampler() && inst->getMsgDesc()->ResponseLength() > 2 &&
            !inst->getDst()->getTopDcl()->getRegVar()->isRegVarTransient());
    };

    // Find all dcls that can be split in to smaller chunks
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (inst->getDst() && inst->getDst()->getTopDcl())
            {
                auto dstDcl = inst->getDst()->getTopDcl();

                if (dstDcl &&
                    !dstDcl->getAddressed() &&
                    dstDcl->getRegFile() == G4_RegFileKind::G4_GRF)
                {
                    auto& prop = splitVars[dstDcl];
                    prop.numDefs++;
                    prop.def = std::make_pair(inst->getDst(), bb);
                    if (canSplit(inst))
                    {
                        prop.candidateDef = true;
                    }
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
        if (item.second.numDefs != 1 || !item.second.candidateDef)
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

            if (item.first->getByteSize() < src->getRightBound())
            {
                item.second.legitCandidate = false;
                break;
            }

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
            else if (numRows > 2)
            {
                // use as send src
                item.second.legitCandidate = false;
                break;
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

    // Apply split cost heuristic
    std::unordered_map<G4_INST*, unsigned int> instId;
    for (auto bb : kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            instId[inst] = instId.size();
        }
    }
    for (auto itemIt = splitVars.begin(); itemIt != splitVars.end();)
    {
        auto item = (*itemIt);

        if (item.second.srcs.size() > 0)
        {
            // Dont emit split if all uses are closeby
            unsigned int idx = instId[item.second.srcs.front().first->getInst()];
            bool split = true;
            if (item.second.srcs.size() > 1)
            {
                for (auto src : item.second.srcs)
                {
                    if ((instId[src.first->getInst()] - idx) < 2)
                    {
                        split = false;
                    }
                    else
                    {
                        split = true;
                        break;
                    }
                    idx = instId[src.first->getInst()];
                }
            }
            else
            {
                if ((idx - instId[item.second.def.first->getInst()]) < 2)
                {
                    split = false;
                }
            }

            // dont split if def-last first use distance <= 8
            if (split &&
                (instId[item.second.srcs.back().first->getInst()] - instId[item.second.def.first->getInst()]) <= 8)
                split = false;

            if (!split)
            {
                itemIt = splitVars.erase(itemIt);
                continue;
            }
        }
        ++itemIt;
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
    for(auto curDcl : kernel.Declares)
    {
        auto isCandidate = splitVars.find(curDcl);
        if (isCandidate == splitVars.end())
            continue;

        auto item = *isCandidate;

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
            auto intrin = kernel.fg.builder->createIntrinsicInst(nullptr, Intrinsic::Split, esize, dstRgn, srcRgn, nullptr, nullptr,
                item.second.def.first->getInst()->getOption() | G4_InstOption::InstOpt_WriteEnable, item.second.def.first->getInst()->getLineNo());
            intrin->setCISAOff(item.second.def.first->getInst()->getCISAOff());
            item.second.def.second->insert(it, intrin);
            splitDcls.push_back(std::make_tuple(lb, rb, splitDcl));
            IRchanged = true;
#ifdef DEBUG_VERBOSE_ON
            numIntrinsicsInserted++;
#endif
        }

        auto getSplitDcl = [&splitDcls](unsigned int lb, unsigned int rb)
        {
            for (auto& item : splitDcls)
            {
                if (std::get<0>(item) <= lb &&
                    std::get<1>(item) >= rb)
                    return std::tuple(std::get<0>(item),
                        std::get<1>(item), std::get<2>(item));
            }
            return std::tuple(0u,0u,(G4_Declare*)nullptr);
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

            auto item = getSplitDcl(lb, rb);
            auto item_lb = std::get<0>(item);
            auto dcl = std::get<2>(item);
            MUST_BE_TRUE(dcl, "Didnt find split dcl");

            unsigned int regNum = (lb - item_lb)/G4_GRF_REG_NBYTES;

            auto newSrc = kernel.fg.builder->createSrcRegRegion(srcRgn->getModifier(), srcRgn->getRegAccess(), dcl->getRegVar(), regNum, srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
            auto opndNum = getOpndNum(srcRgn);
            auto tup = std::make_tuple(srcRgn->getInst(), srcRgn, opndNum);
            preSplit.insert(std::make_pair(newSrc, tup));
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

    if (kernel.getOption(vISA_VerifyExplicitSplit))
    {
        // Check whether intrinsic assignments overlap with other
        // rows of original variable
        verifyOverlap();
    }

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
    // This function is invoked when assigning GRFs to parent.
    unsigned int idealGRF = 0;
    if (isSplitDcl(dcl))
    {
        // <ideal reg num for parent, num children with this assignment for coalescing>
        std::unordered_map<unsigned int, unsigned int> idealParentReg;
        // This is parent dcl
        auto children = getChildren(dcl);
        // Try coalescing with first child that is allocated
        unsigned int numRowsOffset = 0;
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
                    if (regNum >= numRowsOffset)
                    {
                        idealGRF = regNum - numRowsOffset;
                        idealParentReg[idealGRF] = idealParentReg[idealGRF] + 1;
                    }
                }
            }
            numRowsOffset += child->getNumRows();
        }
        std::pair<unsigned int, unsigned int> state;
        if (idealParentReg.size() > 0)
        {
            state.first = idealParentReg.begin()->first;
            state.second = idealParentReg.begin()->second;
        }

        for(auto arg : idealParentReg)
        {
            if (arg.second > state.second)
            {
                state.first = arg.first;
                state.second = arg.second;
            }
        }
        idealGRF = state.first;
    }

    idealGRF = idealGRF >= kernel.getNumRegTotal() ? 0 : idealGRF;

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
        if (auto parentDcl = getParentDcl(dcl))
        {
            auto idealParentGRF = getIdealAllocation(parentDcl, lrs);

            if (idealParentGRF != 0 && parentDcl->getRegVar()->isRegAllocPartaker())
            {
                lrs[parentDcl->getRegVar()->getId()]->setAllocHint(idealParentGRF);
                auto children = getChildren(getParentDcl(dcl));
                unsigned int i = 0;
                for (auto child : *children)
                {
                    if (child->getRegVar()->isRegAllocPartaker())
                    {
                        lrs[child->getRegVar()->getId()]->setAllocHint(idealParentGRF + (i * child->getNumRows()));
                    }
                    i++;
                }
            }
        }
    }
}

void VarSplitPass::undo(G4_Declare* parentDcl)
{
    // Undo split for parentDcl

    if (!isSplitDcl(parentDcl))
        return;

    auto children = getChildren(parentDcl);

    // Skip doing anything if any child was spilled
    for (auto child : *children)
    {
        if (child->isSpilled())
            return;
    }

    for (auto& split : preSplit)
    {
        auto srcRgn = split.first;

        if (!srcRgn->getTopDcl() ||
            getParentDcl(srcRgn->getTopDcl()) != parentDcl)
            continue;

        // parentDcl is parent of split
        auto inst = std::get<0>(split.second);
        auto opnd = std::get<1>(split.second);
        auto srcNum = std::get<2>(split.second);

        inst->setSrc(opnd, srcNum);
    }

    // remove explicit variable split instructions
    for (auto child : *children)
    {
        unusedDcls.erase(child);
    }

    for (auto bb : kernel.fg.getBBList())
    {
        for (auto instIt = bb->begin(); instIt != bb->end();)
        {
            auto inst = (*instIt);
            if (inst->isSplitIntrinsic() &&
                inst->getSrc(0)->getTopDcl() == parentDcl)
            {
                instIt = bb->erase(instIt);
                continue;
            }
            ++instIt;
        }
    }

    // fix other data structures
    for (auto childIt = splitParentDcl.begin(); childIt != splitParentDcl.end();)
    {
        auto child = (*childIt);
        if (child.second == parentDcl)
        {
            childIt = splitParentDcl.erase(childIt);
            continue;
        }
        ++childIt;
    }

    splitChildren.erase(parentDcl);
}

bool VarSplitPass::reallocParent(G4_Declare* child, LiveRange** lrs)
{
    // Given a child, lookup all siblings. If all children
    // are assigned consecutive GRFs but parent isnt then
    // return true.
    bool ret = true;

    if (!child->getRegVar()->isRegAllocPartaker())
        return false;

    auto parent = getParentDcl(child);

    if (parent == nullptr)
        return false;

    auto children = getChildren(parent);

    auto baseRegNum = 0;
    const unsigned int UnInit = 0xffffffff;
    unsigned int nextRegNumExpected = UnInit;
    for (auto child : *children)
    {
        if (!child->getRegVar()->isRegAllocPartaker() ||
            isChildDclUnused(child))
        {
            if (nextRegNumExpected != UnInit)
            {
                nextRegNumExpected += child->getNumRows();
            }
            continue;
        }

        auto id = child->getRegVar()->getId();
        auto phyReg = lrs[id]->getPhyReg();
        if (phyReg)
        {
            if (nextRegNumExpected == UnInit)
            {
                baseRegNum = phyReg->asGreg()->getRegNum();
                nextRegNumExpected = baseRegNum + (child->getByteSize() / G4_GRF_REG_NBYTES);
                continue;
            }
            else if (nextRegNumExpected != phyReg->asGreg()->getRegNum())
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        nextRegNumExpected += child->getNumRows();
    }

    G4_VarBase* parentPhyReg = nullptr;

    if (parent->getRegVar()->isRegAllocPartaker())
        parentPhyReg = lrs[parent->getRegVar()->getId()]->getPhyReg();

    if (!parentPhyReg)
        return false;

    if (parentPhyReg->asGreg()->getRegNum() == baseRegNum)
        return false;

    return ret;
}

bool VarSplitPass::isParentChildRelation(G4_Declare* parent, G4_Declare* child)
{
    if (!isSplitDcl(parent))
        return false;

    auto children = getChildren(parent);

    for(auto c : *children)
    {
        if (c == child)
            return true;
    }

    return false;
}
bool VarSplitPass::isSplitVarLocal(G4_Declare* dcl)
{
    auto it = splitVars.find(dcl);
    MUST_BE_TRUE(it != splitVars.end(), "Not a split dcl");

    return (*it).second.isDefUsesInSameBB();
}

};
/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VarSplit.h"
#include "GraphColor.h"

namespace vISA
{

VarSplitPass::VarSplitPass(G4_Kernel& k) : kernel(k)
{
}

void VarSplitPass::buildPreVerify()
{
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
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
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
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
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
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
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
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

                auto srcDclRegNum = srcLS / kernel.numEltPerGRF<Type_UB>();
                auto srcDclNumRows = ((srcLE + kernel.numEltPerGRF<Type_UB>() - 1) / kernel.numEltPerGRF<Type_UB>()) - srcDclRegNum;

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
                        else if (dstTopDcl->getRegVar()->isRegVarTransient())
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
                    auto numRows = (dstRgn->getLinearizedEnd() - dstRgn->getLinearizedStart() + kernel.numEltPerGRF<Type_UB>() - 1) / kernel.numEltPerGRF<Type_UB>();
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
        return (inst->isSend() &&
            inst->getMsgDesc()->isSampler() &&
            inst->getMsgDesc()->getDstLenRegs() > 2 &&
            !inst->getDst()->getTopDcl()->getRegVar()->isRegVarTransient());
    };

    // Find all dcls that can be split in to smaller chunks
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
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
        if (item.second.numDefs != 1 || !item.second.candidateDef ||
            !item.second.isDefUsesInSameBB())
        {
            item.second.legitCandidate = false;
            continue;
        }

        // Check whether each src operand is independent and compute size
        for (auto& srcpair : item.second.srcs)
        {
            auto src = srcpair.first;
            auto numRows = (src->getRightBound() - src->getLeftBound() + kernel.numEltPerGRF<Type_UB>() - 1) / kernel.numEltPerGRF<Type_UB>();
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
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
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
        for (auto iter = bb->begin(); iter != bb->end(); iter++)
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
    for(unsigned int i = 0; i != kernel.Declares.size(); ++i)
    {
        auto curDcl = kernel.Declares[i];
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
            unsigned int lb = kernel.getGRFSize() * i*numRows;
            unsigned int rb = lb + (kernel.getGRFSize()*numRows)-1;

            auto name = kernel.fg.builder->getNameString(kernel.fg.mem, 50, "%s_%d_%d_%d", dstDcl->getName(), i, lb, rb);
            auto splitDcl = kernel.fg.builder->createDeclareNoLookup(name,
                G4_RegFileKind::G4_GRF, kernel.numEltPerGRF<Type_UD>(), numRows, Type_UD);
            splitParentDcl.insert(std::make_pair(splitDcl, dstDcl));
            splitChildren[dstDcl].push_back(splitDcl);

            // If this part of dcl is never used in code, then dont create split intrinsic inst for it
            if (!item.second.isPartDclUsed(lb, rb))
            {
                unusedDcls.insert(splitDcl);
                continue;
            }

            auto dstRgn = kernel.fg.builder->createDstRegRegion(splitDcl, 1);
            auto srcRgn = kernel.fg.builder->createSrc(dstDcl->getRegVar(),
                item.second.def.first->getRegOff() + (i * numRows), item.second.def.first->getSubRegOff(),
                kernel.fg.builder->getRegionStride1(), Type_UD);
            G4_ExecSize execSize {(kernel.getGRFSize() / TypeSize(Type_UD)) * numRows};
            auto intrin = kernel.fg.builder->createIntrinsicInst(
                nullptr, Intrinsic::Split, execSize, dstRgn, srcRgn, nullptr, nullptr,
                item.second.def.first->getInst()->getOption() | G4_InstOption::InstOpt_WriteEnable, true);
            intrin->inheritDIFrom(item.second.def.first->getInst());
            item.second.def.second->insertBefore(it, intrin);
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

            unsigned int regNum = (lb - item_lb) / kernel.numEltPerGRF<Type_UB>();

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
    for (auto bb : kernel.fg)
    {
        for (auto inst :*bb)
        {
            if (inst->isSplitIntrinsic())
            {
                inst->setOpcode(G4_mov);
#ifdef DEBUG_VERBOSE_ON
                if (inst->getDst()->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum() !=
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

        for (auto arg : idealParentReg)
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

    for (auto bb : kernel.fg)
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
                nextRegNumExpected = baseRegNum + (child->getByteSize() / kernel.numEltPerGRF<Type_UB>());
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

    for (auto c : *children)
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

LoopVarSplit::LoopVarSplit(G4_Kernel& k, GraphColor* c, RPE* r) : kernel(k), coloring(c), rpe(r), references(k)
{
    for (auto spill : coloring->getSpilledLiveRanges())
    {
        spilledDclSet.insert(spill->getDcl());
    }
    auto numVars = coloring->getNumVars();
    auto lrs = coloring->getLiveRanges();
    for (unsigned int i = 0; i != numVars; ++i)
    {
        auto rootDcl = lrs[i]->getDcl();
        dclSpillCost[rootDcl] = lrs[i]->getSpillCost();
    }
}

void LoopVarSplit::run()
{
    // 1. consider list of spilled variables sorted in
    //       descending order of spill cost
    // 2. iterate over each spilled variable from sorted list
    //    a. run cost heuristic to decide if split makes sense
    //    b. if decision is to split, then check which loop(s) to split around

    std::list<std::pair<G4_Declare*, float>> sortedDcls;
    for (auto item : dclSpillCost)
    {
        if(spilledDclSet.find(item.first) !=spilledDclSet.end())
            sortedDcls.push_back(std::pair(item.first, item.second));
    }

    auto SpillCostDesc = [](const std::pair<G4_Declare*, float>& first,
        const std::pair<G4_Declare*, float>& second)
    {
        return first.second > second.second;
    };

    sortedDcls.sort(SpillCostDesc);

    // TODO: Iterating sequence based on spill cost may not be very accurate.
    // Thats because long living variables typically have lower spill cost.
    // But they may be referenced in some nested loop multiple times. In such
    // cases, it is beneficial to split the variable around such loops. To
    // accommodate such cases, we should be looking at reference count of
    // variables in loops and iterate in an order based on that.
    for (auto var : sortedDcls)
    {
        const auto& loops = getLoopsToSplitAround(var.first);
        for (auto& loop : loops)
        {
            split(var.first, *loop);
        }
    }
}

std::vector<G4_SrcRegRegion*> LoopVarSplit::getReads(G4_Declare* dcl, Loop& loop)
{
    std::vector<G4_SrcRegRegion*> reads;

    const auto& uses = references.getUses(dcl);
    for (auto use : *uses)
    {
        auto bb = std::get<1>(use);
        if (loop.contains(bb))
        {
            auto inst = std::get<0>(use);
            for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
            {
                auto opnd = inst->getSrc(i);
                if (!opnd || !opnd->isSrcRegRegion())
                    continue;
                auto topdcl = opnd->asSrcRegRegion()->getTopDcl();
                if (topdcl == dcl)
                    reads.push_back(opnd->asSrcRegRegion());
            }
        }
    }

    return reads;
}

std::vector<G4_DstRegRegion*> LoopVarSplit::getWrites(G4_Declare* dcl, Loop& loop)
{
    std::vector<G4_DstRegRegion*> writes;

    const auto& defs = references.getDefs(dcl);
    for (auto def : *defs)
    {
        auto bb = std::get<1>(def);
        if (loop.contains(bb))
        {
            auto dst = std::get<0>(def)->getDst();
            writes.push_back(dst);
        }
    }

    return writes;
}

unsigned int LoopVarSplit::getMaxRegPressureInLoop(Loop& loop)
{
    auto it = maxRegPressureCache.find(&loop);
    if (it != maxRegPressureCache.end())
        return it->second;

    unsigned int maxRPE = 0;
    auto& bbs = loop.getBBs();

    for (auto bb : bbs)
    {
        for (auto inst : bb->getInstList())
        {
            maxRPE = std::max(maxRPE, rpe->getRegisterPressure(inst));
        }
    }

    maxRegPressureCache[&loop] = maxRPE;

    return maxRPE;
}

void LoopVarSplit::dump(std::ostream& of)
{
    for (auto dcl : splitResults)
    {
        of << "Spilled dcl: " << dcl.first->getName() << std::endl;
        for (auto split : dcl.second)
        {
            of << "\tSplit dcl: " << split.first->getName() << ", for loop L" << split.second->id << std::endl;
        }
        of << std::endl;
    }
}

void LoopVarSplit::removeSplitInsts(GlobalRA* gra, G4_Declare* spillDcl, G4_BB* bb)
{
    auto it = gra->splitResults.find(spillDcl);

    if (it == gra->splitResults.end())
        return;

    auto& bbInstsToRemove = (*it).second.insts;
    for (auto entry : bbInstsToRemove)
    {
        auto currBB = entry.first;

        if (currBB == bb)
        {
            auto& instsToRemoveFromBB = entry.second;
            for (auto instIt = bb->begin(); instIt != bb->end();)
            {
                auto inst = (*instIt);
                if (instsToRemoveFromBB.find(inst) == instsToRemoveFromBB.end())
                {
                    ++instIt;
                    continue;
                }

                instIt = bb->erase(instIt);
            }
        }
    }
}

bool LoopVarSplit::removeFromPreheader(GlobalRA* gra, G4_Declare* spillDcl, G4_BB* bb, INST_LIST_ITER filledInstIter)
{
    auto it = gra->splitResults.find(spillDcl);
    if (it != gra->splitResults.end() &&
        (*it).second.insts.find(bb) != (*it).second.insts.end())
    {
        auto inst = *filledInstIter;
        if (inst->isRawMov())
        {
            auto dstTopDcl = inst->getDst()->getTopDcl();
            auto it = gra->splitResults.find(dstTopDcl);
            if (it == gra->splitResults.end())
                return false;

            removeSplitInsts(gra, spillDcl, bb);
            return true;
        }
    }
    return false;
}

bool LoopVarSplit::removeFromLoopExit(GlobalRA* gra, G4_Declare* spillDcl, G4_BB* bb, INST_LIST_ITER filledInstIter)
{
    return removeFromPreheader(gra, spillDcl, bb, filledInstIter);
}

const std::unordered_set<G4_INST*> LoopVarSplit::getSplitInsts(GlobalRA* gra, G4_BB* bb)
{
    std::unordered_set<G4_INST*> ret;

    for (auto& splitVar : gra->splitResults)
        for (auto& insts : splitVar.second.insts)
            if (insts.first == bb)
            {
                for (auto item : insts.second)
                    ret.insert(item);
            }

    return ret;
}

bool LoopVarSplit::split(G4_Declare* dcl, Loop& loop)
{
    // Split dcl in given loop. Return true if split was successful.
    // It is assumed that dcl is spilled. This method inserts a
    // copy in preheader of loop. Dst of this copy is a new tmp and
    // source is dcl. All uses/defs of dcl in the loop are replaced
    // with the tmp. If dcl is ever written in the loop, a copy from
    // tmp to dcl is inserted in loop exit bb.
    //
    // This transformation requires a valid preheader be present for
    // loop. If dcl is written in the loop then a valid exit bb is also
    // needed.
    if (!loop.preHeader)
        return false;

    const auto& dsts = getWrites(dcl, loop);
    if (dsts.size() > 0)
    {
        // evaluate if it makes sense to insert the copy if loop has
        // multiple exits.
        if (loop.getLoopExits().size() != 1)
            return false;
        // TODO: Handle creation of loop preheader and loop exit in
        // presence of SIMD CF. this requires changing JIP, UIP
        // in source goto instructions and fix (if) any data structures
        // in VISA that rely on those JIP/UIP.
        if (!loop.preHeader->dominates(loop.getLoopExits().front()))
            return false;
    }

    const auto& srcs = getReads(dcl, loop);

    auto splitDcl = kernel.fg.builder->createTempVar(dcl->getTotalElems(),
        dcl->getElemType(), coloring->getGRA().getSubRegAlign(dcl),
        "LOOPSPLIT", true);

    auto& splitData = coloring->getGRA().splitResults[splitDcl];
    splitData.origDcl = dcl;

    // emit TMP = dcl in preheader
    copy(loop.preHeader, splitDcl, dcl, &splitData);

    // emit dcl = TMP in loop exit
    if (dsts.size() > 0)
    {
        copy(loop.getLoopExits().front(), dcl, splitDcl, &splitData, false);
    }

    // replace all occurences of dcl in loop with TMP
    for (auto src : srcs)
        replaceSrc(src, splitDcl);

    for (auto dst : dsts)
        replaceDst(dst, splitDcl);

    splitResults[dcl].push_back(std::make_pair(splitDcl, &loop));

    return true;
}

void LoopVarSplit::copy(G4_BB* bb, G4_Declare* dst, G4_Declare* src, SplitResults* splitData, bool pushBack)
{
    // create mov instruction to copy dst->src
    // multiple mov instructions may be created depending on size of dcls
    // all mov instructions are appended to inst list of bb
    // when pushBack argument = true, append to BB (happens in pre-header)
    // when pushBack argument = false, insert in bb after label (happens at exit bb)

    dst = dst->getRootDeclare();
    src = src->getRootDeclare();
    unsigned int numRows = dst->getNumRows();
    unsigned int bytesRemaining = dst->getByteSize();

    auto insertCopy = [&](G4_INST* inst)
    {
        if (pushBack || bb->size() == 0)
        {
            bb->push_back(inst);
            splitData->insts[bb].insert(inst);
        }
        else
        {
            // insert immediately after label instruction, if one exists
            for (auto it = bb->begin(); it != bb->end(); ++it)
            {
                auto inst = (*it);
                if (inst->isLabel())
                    continue;
                bb->insertAfter(it, inst);
                splitData->insts[bb].insert(inst);
                break;
            }
        }
        if (inst->isWriteEnableInst() && coloring->getGRA().EUFusionNoMaskWANeeded())
        {
            coloring->getGRA().addEUFusionNoMaskWAInst(bb, inst);
        }
    };

    // first copy full GRF rows
    if (numRows > 1 || (dst->getTotalElems() * dst->getElemSize() == kernel.getGRFSize()))
    {
        // dcls are GRF sized so emit max SIMD size possible and copy 2 rows at
        // a time
        for (unsigned int i = 0; i < numRows; i++)
        {
            const RegionDesc* rd = kernel.fg.builder->getRegionStride1();
            G4_ExecSize execSize{ kernel.numEltPerGRF<Type_UD>() };

            if ((i + 1) < numRows)
                execSize = G4_ExecSize(kernel.numEltPerGRF<Type_UD>() * 2);

            auto dstRgn = kernel.fg.builder->createDst(dst->getRegVar(), (short)i, 0, 1, Type_F);
            auto srcRgn = kernel.fg.builder->createSrc(src->getRegVar(), (short)i, 0, rd, Type_F);
            auto inst = kernel.fg.builder->createMov(execSize, dstRgn, srcRgn, InstOpt_WriteEnable, false);

            insertCopy(inst);
            bytesRemaining -= (execSize.value * G4_Type_Table[Type_F].byteSize);

            if (bytesRemaining < kernel.numEltPerGRF<Type_UB>())
                break;
        }
    }

    while (bytesRemaining > 0)
    {
        G4_Type type = Type_W;
        G4_ExecSize execSize = g4::SIMD16;

        if (bytesRemaining >= 16)
            execSize = g4::SIMD8;
        else if (bytesRemaining >= 8 && bytesRemaining < 16)
            execSize = g4::SIMD4;
        else if (bytesRemaining >= 4 && bytesRemaining < 8)
            execSize = g4::SIMD2;
        else if (bytesRemaining >= 2 && bytesRemaining < 4)
            execSize = g4::SIMD1;
        else if (bytesRemaining == 1)
        {
            // If a region has odd number of bytes, copy last byte in final iteration
            execSize = g4::SIMD1;
            type = Type_UB;
        }
        else
        {
            MUST_BE_TRUE(false, "Unexpected condition");
        }

        const RegionDesc* rd = kernel.fg.builder->getRegionStride1();

        unsigned int row = (dst->getByteSize() - bytesRemaining) / kernel.numEltPerGRF<Type_UB>();
        unsigned int col = (dst->getByteSize() - bytesRemaining) % kernel.numEltPerGRF<Type_UB>();
        if (G4_Type_Table[type].byteSize > 1)
        {
            MUST_BE_TRUE(col % 2 == 0, "Unexpected condition");
            col /= 2;
        }

        G4_DstRegRegion* dstRgn = kernel.fg.builder->createDst(dst->getRegVar(), row, col, 1, type);
        G4_SrcRegRegion* srcRgn = kernel.fg.builder->createSrc(src->getRegVar(), row, col, rd, type);

        auto inst = kernel.fg.builder->createMov(execSize, dstRgn, srcRgn, InstOpt_WriteEnable, false);

        insertCopy(inst);

        bytesRemaining -= (execSize.value * G4_Type_Table[type].byteSize);
    };
}

void LoopVarSplit::replaceSrc(G4_SrcRegRegion* src, G4_Declare* dcl)
{
    auto srcDcl = src->getBase()->asRegVar()->getDeclare();
    dcl = getNewDcl(srcDcl, dcl);

    auto newSrcRgn = kernel.fg.builder->createSrc(dcl->getRegVar(), src->getRegOff(),
        src->getSubRegOff(), src->getRegion(), src->getType(), src->getAccRegSel());

    auto inst = src->getInst();
    for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
    {
        if (inst->getSrc(i) == src)
        {
            inst->setSrc(newSrcRgn, i);
            break;
        }
    }
}

void LoopVarSplit::replaceDst(G4_DstRegRegion* dst, G4_Declare* dcl)
{
    auto dstDcl = dst->getBase()->asRegVar()->getDeclare();
    dcl = getNewDcl(dstDcl, dcl);

    auto newDstRgn = kernel.fg.builder->createDst(dcl->getRegVar(), dst->getRegOff(),
        dst->getSubRegOff(), dst->getHorzStride(), dst->getType(), dst->getAccRegSel());

    auto inst = dst->getInst();
    inst->setDest(newDstRgn);
}

G4_Declare* LoopVarSplit::getNewDcl(G4_Declare* dcl1, G4_Declare* dcl2)
{
    // this method gets args dcl1, dcl2. this method is invoked
    // when the transformation replaces existing src/dst rgn with
    // equivalent one but using split variable.
    //
    // dcl1 is a dcl used to construct some src or dst rgn.
    // dcl2 is a new dcl that splits dcl1. dcl2 is always root dcl.
    // dcl1 may or may not be aliased of another dcl.
    // if dcl1 is also root dcl, then return dcl2.
    // if dcl1 is an alias dcl, then construct new dcl that aliases
    //   dcl2 at similar offset.
    // mapping from old dcl to new dcl is stored for future invocations.

    MUST_BE_TRUE(!dcl2->getAliasDeclare(), "Expecting to see root dcl for dcl2");

    auto it = oldNewDcl.find(dcl1);
    if (it != oldNewDcl.end())
        return (*it).second;

    if (!dcl1->getAliasDeclare())
    {
        oldNewDcl[dcl1] = dcl2;
        return dcl2;
    }

    auto newDcl = kernel.fg.builder->createTempVar(dcl1->getTotalElems(), dcl1->getElemType(),
        dcl1->getSubRegAlign());
    newDcl->setAliasDeclare(getNewDcl(dcl1->getRootDeclare(), dcl2), dcl1->getOffsetFromBase());

    oldNewDcl[dcl1] = newDcl;

    return newDcl;
}

std::vector<Loop*> LoopVarSplit::getLoopsToSplitAround(G4_Declare* dcl)
{
    // return a list of Loop* around which variable dcl should be split
    std::vector<Loop*> loopsToSplitAround;

    // first make list of all loops where dcl is ever referenced
    auto uses = references.getUses(dcl);
    auto defs = references.getDefs(dcl);

    auto StableOrder = [](const G4_BB* first, const G4_BB* second)
    {
        return first->getId() < second->getId();
    };
    std::set<G4_BB*, decltype(StableOrder)> bbsWithRefToDcl(StableOrder);

    for (auto& use : *uses)
    {
        auto bb = std::get<1>(use);
        bbsWithRefToDcl.insert(bb);
    }

    for (auto& def : *defs)
    {
        auto bb = std::get<1>(def);
        bbsWithRefToDcl.insert(bb);
    }

    auto OrderByRegPressure = [&](Loop* loop1, Loop* loop2)
    {
        return getMaxRegPressureInLoop(*loop1) >
            getMaxRegPressureInLoop(*loop2);
    };

    std::set<Loop*, decltype(OrderByRegPressure)> innerMostLoops(OrderByRegPressure);

    // now collect innermost loop for each referenced BB
    for (auto bb : bbsWithRefToDcl)
    {
        auto innerMost = kernel.fg.getLoops().getInnerMostLoop(bb);
        if (innerMost)
            innerMostLoops.insert(innerMost);
    }

    // prune list of loops
    // 1. loops are stored in descending order of max reg pressure
    // 2. apply cost heuristic to decide if variable should be split at a loop
    // 3. once split at a loop, dont split at any parent or nested loop
    //
    // Example:
    //
    // -----
    // |
    // | Loop A
    // | =X
    // -----
    //
    // -----
    // |
    // | Loop B
    // | =X
    // | -----
    // | | =X
    // | | Loop C
    // | -----
    // |
    // -----
    //
    // Assume variable X is spilled and is referenced in Loop A, Loop B, Loop C.
    // Loop C is nested in Loop B.
    // The algorithm below decides whether it is better to spill X around Loop C
    // or Loop B. X is spilled around only 1 of these 2 loops since they've a
    // parent-nested relationship. Independently, the algorithm can also decide to
    // split X around Loop A as it is not a parent or nested loop of other loops.
    //


    for (auto loop : innerMostLoops)
    {
        bool dontSplit = false;
        for (auto splitLoop : loopsToSplitAround)
        {
            if (loop->fullSubset(splitLoop) ||
                loop->fullSuperset(splitLoop))
            {
                // variable already split in nested or parent loop
                dontSplit = true;
                break;
            }
        }
        if (dontSplit)
            continue;

        // apply cost heuristic
        if (dcl->getNumRows() <= 2)
        {
            if (getMaxRegPressureInLoop(*loop) <= (unsigned int)(1.5f * (float)kernel.getNumRegTotal()))
                loopsToSplitAround.push_back(loop);
        }
        else if (dcl->getNumRows() <= 4)
        {
            if (getMaxRegPressureInLoop(*loop) <= (unsigned int)(0.95f * (float)kernel.getNumRegTotal()))
                loopsToSplitAround.push_back(loop);
        }
        else if (dcl->getNumRows() > 4)
        {
            // splitting dcls with > 4 rows should be very rare
            if (getMaxRegPressureInLoop(*loop) <= (unsigned int)(0.6f * (float)kernel.getNumRegTotal()))
                loopsToSplitAround.push_back(loop);
        }
    }

    return loopsToSplitAround;
}

};

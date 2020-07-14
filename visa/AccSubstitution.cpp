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

#include <cmath>
#include "AccSubstitution.h"

using namespace vISA;

struct AccInterval
{
    G4_INST* inst;
    int lastUse;
    bool mustBeAcc0 = false;
    bool isPreAssigned = false;
    int assignedAcc = -1;
    int bundleConflictTimes = 0;
    int bankConflictTimes = 0;
    int suppressionTimes = 0;

    AccInterval(G4_INST* inst_, int lastUse_, bool preAssigned = false) :
        inst(inst_), lastUse(lastUse_), isPreAssigned(preAssigned)
    {
        if (isPreAssigned)
        {
            mustBeAcc0 = true;
            assignedAcc = 0;
        }
    }

    double getSpillCost()
    {
        if (isPreAssigned)
        {
            // don't spill pre-assigned
            return (double)1000000;
        }
        int dist = lastUse - inst->getLocalId();

        return std::pow((double)inst->use_size(), 3) / dist;
    }

    // see if this interval needs both halves of the acc
    bool needBothAcc(IR_Builder& builder) const
    {
        switch (inst->getDst()->getType())
        {
        case Type_F:
            return inst->getExecSize() == (builder.getNativeExecSize() * 2);
        case Type_HF:
            return false;
        case Type_DF:
            return inst->getExecSize() > (builder.getNativeExecSize() / 2);
        default:
            return true;
        }
    }

    void dump()
    {
        std::cerr << "Interval: [" << inst->getLocalId() << ", " << lastUse << "]\n";
        std::cerr << "\t";
        inst->dump();
        if (assignedAcc != -1)
        {
            std::cerr << "\tAssigned to Acc" << assignedAcc << "\n";
        }
        std::cerr << "\n";
    }
};


// returns true if the inst is a candidate for acc substitution
// lastUse is also update to point to the last use id of the inst
bool AccSubPass::isAccCandidate(G4_INST* inst, int& lastUse, bool& mustBeAcc0)

{
    mustBeAcc0 = false;
    G4_DstRegRegion* dst = inst->getDst();
    if (!dst || kernel.fg.globalOpndHT.isOpndGlobal(dst) || !inst->canDstBeAcc())
    {
        return false;
    }

    if (inst->getCondMod())
    {
        // since our du-chain is on inst instead of operand, the presence of conditional modifier complicates the checks later.
        // This is somewhat conservative but shouldn't matter too much as inst with both dst and conditional modifiers are rare.
        return false;
    }

    // check that every use may be replaced with acc
    int lastUseId = 0;
    std::vector<G4_INST*> madSrc0Use;
    std::vector<G4_INST*> threeSrcUses; //3src inst that use this dst
    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto&& use = *I;
        G4_INST* useInst = use.first;
        Gen4_Operand_Number opndNum = use.second;
        lastUseId = std::max(lastUseId, useInst->getLocalId());
        // acc may be src0 of two-source inst or src1 of three-source inst
        // ToDo: may swap source here
        if (useInst->getNumSrc() == 3)
        {

            if (!kernel.fg.builder->relaxedACCRestrictions() &&
                std::find(threeSrcUses.begin(), threeSrcUses.end(), useInst) != threeSrcUses.end())
            {
                // don't allow acc to appear twice in a 3-src inst
                return false;
            }
            threeSrcUses.push_back(useInst);
            switch (opndNum)
            {
            case Opnd_src1:
                break;  //OK

            case Opnd_src0:

                if (kernel.fg.builder->canMadHaveSrc0Acc())
                {
                    // OK
                }
                else if (useInst->opcode() == G4_mad)
                {
                    // we can turn this mad into a mac
                    mustBeAcc0 = true;
                    if (useInst->getSrc(0)->getType() == Type_HF && useInst->getMaskOffset() == 16)
                    {
                        // we must use acc1, and need to check that inst does not have an acc0 source
                        // so that dst and src won't have different acc source
                        if (inst->isAccSrcInst())
                        {
                            bool hasAcc0Src = false;
                            auto isAcc0 = [](G4_SrcRegRegion* src)
                            {
                                return src->getBase()->asAreg()->getArchRegType() == AREG_ACC0;
                            };
                            if (inst->getSrc(0)->isSrcRegRegion() &&
                                inst->getSrc(0)->asSrcRegRegion()->getBase()->isAccReg())
                            {
                                hasAcc0Src = isAcc0(inst->getSrc(0)->asSrcRegRegion());
                            }
                            else if (inst->getSrc(1)->isSrcRegRegion() &&
                                inst->getSrc(1)->asSrcRegRegion()->getBase()->isAccReg())
                            {
                                hasAcc0Src = isAcc0(inst->getSrc(1)->asSrcRegRegion());
                            }
                            if (hasAcc0Src)
                            {
                                return false;
                            }
                        }
                    }
                    madSrc0Use.push_back(useInst);
                }
                else
                {
                    return false;
                }
                break;
            default:
                return false;
            }
        }
        else if (!builder.relaxedACCRestrictions() && opndNum != Opnd_src0)
        {
            return false;
        }

        if (useInst->getSingleDef(opndNum) == nullptr)
        {
            // def must be the only define for this use
            return false;
        }

        int srcId = opndNum == Opnd_src0 ? 0 : 1;
        G4_Operand* src = useInst->getSrc(srcId);
        if (dst->getType() != src->getType() || kernel.fg.globalOpndHT.isOpndGlobal(src) ||
            dst->compareOperand(src) != Rel_eq)
        {
            return false;
        }
        if (!useInst->canSrcBeAcc(opndNum))
        {
            return false;
        }
    }

    // we have to avoid the case where the dst is used as both src0 and src1 of a mad
    for (auto madUse : madSrc0Use)
    {
        for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
        {
            auto&& use = *I;
            G4_INST* useInst = use.first;
            Gen4_Operand_Number opndNum = use.second;
            if (madUse == useInst && opndNum == Opnd_src1)
            {
                return false;
            }
        }
    }

    if (lastUseId == 0)
    {
        // no point using acc for a dst without local uses
        return false;
    }

    lastUse = lastUseId;
    return true;
}

// replace an inst's dst and all of its (local) uses with acc
// note that this may fail due to HW restrictions on acc
bool AccSubPass::replaceDstWithAcc(G4_INST* inst, int accNum)
{
    G4_DstRegRegion* dst = inst->getDst();
    bool useAcc1 = (accNum & 0x1) != 0;
    accNum &= ~0x1;

    if (!builder.relaxedACCRestrictions())
    {
        auto myAcc = useAcc1 ? AREG_ACC1 : AREG_ACC0;
        // check that dst and src do not have different accumulator
        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
        {
            if (inst->getSrc(i)->isAccReg())
            {
                auto base = inst->getSrc(i)->asSrcRegRegion()->getBase();
                if (base->isPhyAreg())
                {
                    if (base->asAreg()->getArchRegType() != myAcc)
                    {
                        return false;
                    }
                }
            }
        }
    }

    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto&& use = *I;
        G4_INST* useInst = use.first;
        if (!builder.canMadHaveSrc0Acc() && useInst->opcode() == G4_mad && use.second == Opnd_src0)
        {
            // if we are replacing mad with mac, additionally check if acc1 needs to be used
            if (useInst->getMaskOffset() == 16 && dst->getType() == Type_HF)
            {
                if (builder.doMultiAccSub())
                {
                    // this is not legal since acc1 may be taken by another interval already
                    return false;
                }
                useAcc1 = true;
            }
        }

        if (builder.relaxedACCRestrictions())
        {
            // mul/mac can't have both sources be acc
            // Note that we only need to check for explicit mac here since we will not change mad to mac
            if (useInst->opcode() == G4_mul || useInst->opcode() == G4_mac)
            {
                if (useInst->getSrc(0)->isAccReg() || useInst->getSrc(1)->isAccReg())
                {
                    return false;
                }
            }
        }
        else
        {
            // do not allow an inst to have multiple acc source operands
            if (useInst->getNumSrc() == 3)
            {
                if (useInst->getSrc(0)->isAccReg() || useInst->getSrc(1)->isAccReg())
                {
                    return false;
                }
            }
            else if (useInst->opcode() == G4_mac)
            {
                // this can happen if we have to convert mad into mac (some platforms don't allow
                // src0 acc for mad), and the mad's src1 is also an acc candidate.
                return false;
            }
        }
    }

    // at this point acc substitution must succeed

    G4_Areg* accReg = useAcc1 ? builder.phyregpool.getAcc1Reg() : builder.phyregpool.getAcc0Reg();
    G4_DstRegRegion* accDst = builder.createDst(accReg,
        (short)accNum, 0, 1, dst->getType());
    accDst->setAccRegSel(inst->getDst()->getAccRegSel());
    inst->setDest(accDst);
    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto&& use = *I;
        G4_INST* useInst = use.first;
        int srcId = use.second == Opnd_src0 ? 0 : 1;
        G4_SrcRegRegion* oldSrc = useInst->getSrc(srcId)->asSrcRegRegion();
        G4_SrcRegRegion* accSrc = builder.createSrcRegRegion(oldSrc->getModifier(), Direct,
            accReg, (short)accNum, 0, builder.getRegionStride1(), dst->getType());
        accSrc->setAccRegSel(oldSrc->getAccRegSel());
        if (useInst->opcode() == G4_mad && srcId == 0 && !builder.canMadHaveSrc0Acc())
        {
            // change mad to mac as src0 of 3-src does not support acc
            auto updateDefSrcPos = [](G4_INST* useInst, Gen4_Operand_Number origPos)
            {
                for (auto DI = useInst->def_begin(), DE = useInst->def_end(); DI != DE; ++DI)
                {
                    auto&& def = *DI;
                    if (def.second == origPos)
                    {
                        for (auto UI = def.first->use_begin(), UE = def.first->use_end(); UI != UE; ++UI)
                        {
                            auto& use = *UI;
                            if (use.first == useInst && use.second == origPos)
                            {
                                switch (use.second)
                                {
                                case Opnd_src1:
                                    use.second = Opnd_src0;
                                    break;
                                case Opnd_src2:
                                    use.second = Opnd_src1;
                                    break;
                                default:
                                    assert(false && "unexpectd src pos");
                                }
                            }
                        }
                    }
                }
            };
            assert(accNum == 0 && "mad src0 may only use acc0");
            G4_Operand* macSrc0 = useInst->getSrc(1);
            updateDefSrcPos(useInst, Opnd_src1);
            G4_Operand* macSrc1 = useInst->getSrc(2);
            updateDefSrcPos(useInst, Opnd_src2);
            useInst->setSrc(macSrc0, 0);
            useInst->setSrc(macSrc1, 1);
            useInst->setOpcode(G4_mac);
            useInst->setImplAccSrc(accSrc);
        }
        else
        {
            useInst->setSrc(accSrc, srcId);
        }
    }

    return true;
}

struct AccAssignment
{
    std::vector<bool> freeAccs;
    std::list<AccInterval*> activeIntervals;
    IR_Builder& builder;

    AccAssignment(int numGeneralAcc, IR_Builder& m_builder) : builder(m_builder)
    {
        freeAccs.resize(numGeneralAcc, true);
    }

    // expire all intervals that end before the given interval
    void expireIntervals(AccInterval* interval)
    {
        for (auto iter = activeIntervals.begin(), iterEnd = activeIntervals.end(); iter != iterEnd;)
        {
            AccInterval* active = *iter;
            if (active->lastUse <= interval->inst->getLocalId())
            {
                assert(!freeAccs[active->assignedAcc] && "active interval's acc should not be free");
                freeAccs[active->assignedAcc] = true;
                if (active->needBothAcc(builder))
                {
                    assert(!freeAccs[active->assignedAcc + 1] && "active interval's acc should not be free");
                    freeAccs[active->assignedAcc + 1] = true;
                }
                iter = activeIntervals.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // spill interval that is assigned to accID and remove it from active list
    void spillInterval(int accID)
    {
        auto acc0Iter = std::find_if(activeIntervals.begin(), activeIntervals.end(),
            [accID](AccInterval* interval) { return interval->assignedAcc == accID; });
        assert(acc0Iter != activeIntervals.end() && "expect to find interval with acc0");
        auto spillInterval = *acc0Iter;
        assert(!spillInterval->isPreAssigned && "overlapping pre-assigned acc0");
        spillInterval->assignedAcc = -1;
        activeIntervals.erase(acc0Iter);
        freeAccs[accID] = true;
        if (spillInterval->needBothAcc(builder))
        {
            assert(accID % 2 == 0 && "accID must be even-aligned in this case");
            freeAccs[accID + 1] = true;
        }
    }

    // pre-assigned intervals (e.g., mach, addc) must use acc0 (and acc1 depending on inst type/size)
    // we have to spill active intervals that occupy acc0/acc1.
    // the pre-assigned interavl is also pushed to active list
    void handlePreAssignedInterval(AccInterval* interval)
    {
        if (!freeAccs[interval->assignedAcc])
        {
            spillInterval(interval->assignedAcc);
        }
        freeAccs[interval->assignedAcc] = false;

        if (interval->needBothAcc(builder))
        {
            assert(interval->assignedAcc == 0 && "Total 2 acc support right now");
            if (!freeAccs[interval->assignedAcc + 1]) // && activeIntervals.size()
            {
                spillInterval(interval->assignedAcc + 1);
            }
            freeAccs[interval->assignedAcc + 1] = false;
        }

        activeIntervals.push_back(interval);
    }

    // pick a free acc for this interval
    // returns true if a free acc is found, false otherwise
    bool assignAcc(AccInterval* interval)
    {
        if (interval->isPreAssigned)
        {
            handlePreAssignedInterval(interval);
            return true;
        }

        int step = interval->needBothAcc(builder) ? 2 : 1;
        for (int i = 0, end = interval->mustBeAcc0 ? 1 : (int)freeAccs.size(); i < end; i += step)
        {
            if (freeAccs[i] && (!interval->needBothAcc(builder) || freeAccs[i + 1]))
            {
                interval->assignedAcc = i;
                freeAccs[i] = false;
                if (interval->needBothAcc(builder))
                {
                    freeAccs[i + 1] = false;
                }

                activeIntervals.push_back(interval);
                return true;
            }
        }
        return false;
    }
};


void AccSubPass::multiAccSub(G4_BB* bb)
{
    int numGeneralAcc = kernel.getNumAcc();

    std::vector<AccInterval*> intervals;


    //build intervals for potential acc candidates as well as pre-existing acc uses from mac/mach/addc/etc
    for (auto instIter = bb->begin(), instEnd = bb->end(); instIter != instEnd; ++instIter)
    {
        G4_INST* inst = *instIter;
        if (inst->defAcc())
        {
            // we should only have single def/use acc at this point, so any use would kill the def
            auto iter = instIter;
            auto useIter = std::find_if(++iter, instEnd, [](G4_INST* inst) { return inst->useAcc(); });
            int lastUseId = useIter == instEnd ? bb->back()->getLocalId() : (*useIter)->getLocalId();
            AccInterval* newInterval = new AccInterval(inst, lastUseId, true);
            intervals.push_back(newInterval);
        }
        else
        {
            int lastUseId = 0;
            bool mustBeAcc0 = false;
            int bundleBCTimes = 0;
            int bankBCTimes = 0;
            int readSuppressionSrcs = 0;
            if (isAccCandidate(inst, lastUseId, mustBeAcc0))
            {
                // this is a potential candidate for acc substitution
                AccInterval* newInterval = new AccInterval(inst, lastUseId);
                newInterval->mustBeAcc0 = mustBeAcc0;
                newInterval->bankConflictTimes = bankBCTimes;
                newInterval->bundleConflictTimes = bundleBCTimes;
                newInterval->suppressionTimes = readSuppressionSrcs;

                intervals.push_back(newInterval);
            }
        }
    }


        //modified linear scan to assign free accs to intervals
        AccAssignment accAssign(numGeneralAcc, builder);

        for (auto interval : intervals)
        {
            // expire intervals
            accAssign.expireIntervals(interval);

            // assign interval
            bool foundFreeAcc = accAssign.assignAcc(interval);

            //Spill
            if (!foundFreeAcc && accAssign.activeIntervals.size() != 0)
            {
                // check if we should spill one of the active intervals
                auto spillCostCmp = [interval](AccInterval* intv1, AccInterval* intv2)
                {
                    if (!interval->mustBeAcc0)
                    {
                        return intv1->getSpillCost() < intv2->getSpillCost();
                    }

                    // different compr function if interval must use acc0
                    if (intv1->assignedAcc == 0 && intv2->assignedAcc == 0)
                    {
                        return intv1->getSpillCost() < intv2->getSpillCost();
                    }
                    else if (intv1->assignedAcc == 0)
                    {
                        return true;
                    }
                    return false;
                };
                auto spillIter = std::min_element(accAssign.activeIntervals.begin(), accAssign.activeIntervals.end(),
                    spillCostCmp);
                auto spillCandidate = *spillIter;
                if (interval->getSpillCost() > spillCandidate->getSpillCost() &&
                    !spillCandidate->isPreAssigned &&
                    !(interval->mustBeAcc0 && spillCandidate->assignedAcc != 0))
                {
                    bool tmpAssignValue[2];

                    tmpAssignValue[0] = accAssign.freeAccs[spillCandidate->assignedAcc];
                    accAssign.freeAccs[spillCandidate->assignedAcc] = true;
                    if (spillCandidate->needBothAcc(builder))
                    {
                        tmpAssignValue[1] = accAssign.freeAccs[spillCandidate->assignedAcc + 1];
                        accAssign.freeAccs[spillCandidate->assignedAcc + 1] = true;
                    }

                    if (accAssign.assignAcc(interval))
                    {
                        spillCandidate->assignedAcc = -1;
                        accAssign.activeIntervals.erase(spillIter);
                    }
                    else
                    {
                        accAssign.freeAccs[spillCandidate->assignedAcc] = tmpAssignValue[0];
                        if (spillCandidate->needBothAcc(builder))
                        {
                            accAssign.freeAccs[spillCandidate->assignedAcc + 1] = tmpAssignValue[1];
                        }
                    }
                }
            }
        }

        for (auto interval : intervals)
        {
            if (!interval->isPreAssigned && interval->assignedAcc != -1)
            {
                G4_INST* inst = interval->inst;
                replaceDstWithAcc(inst, interval->assignedAcc);

                numAccSubDef++;
                numAccSubUse += (int)inst->use_size();
#if 0
                std::cout << "Acc sub def inst: \n";
                inst->emit(std::cout);
                std::cout << "[" << inst->getLocalId() << "]\n";
                std::cout << "Uses:\n";
                for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
                {
                    auto&& use = *I;
                    std::cout << "\t";
                    use.first->emit(std::cout);
                    std::cout << "[" << use.first->getLocalId() << "]\n";
                }
#endif
            }
        }


    for (int i = 0, end = (int)intervals.size(); i < end; ++i)
    {
        delete intervals[i];
    }

    return;
}

// substitute local operands with acc when possible
void AccSubPass::accSub(G4_BB* bb)
{
    bb->resetLocalId();

    if (builder.doMultiAccSub())
    {
        multiAccSub(bb);
        return;
    }

    for (auto instIter = bb->begin(), instEnd = bb->end(); instIter != instEnd; ++instIter)
    {
        bool canDoAccSub = true;
        G4_INST* inst = *instIter;

        if (inst->defAcc())
        {
            // skip ahead till its single use
            // we should only have single def/use acc at this point, so any use would
            // kill the def
            auto iter = instIter;
            auto useIter = std::find_if(++iter, instEnd, [](G4_INST* inst) { return inst->useAcc(); });
            if (useIter == instEnd)
            {
                return;
            }
            instIter = --useIter; // start at the use inst next time
            continue;
        }

        int lastUseId = 0;
        bool mustBeAcc0 = false; //ignored
        if (!isAccCandidate(inst, lastUseId, mustBeAcc0))
        {
            continue;
        }

        // don't attempt acc sub if def and last use are too far apart
        // this is a crude way to avoid a long running life range from blocking
        // other acc sub opportunities
        const int accWindow = 25;
        if (lastUseId == 0 || lastUseId - inst->getLocalId() > accWindow)
        {
            continue;
        }

        // check for intervening acc usage between inst and its last use
        auto subIter = instIter;
        ++subIter;
        for (int instId = inst->getLocalId() + 1; instId != lastUseId; ++subIter, ++instId)
        {
            G4_INST* anInst = *subIter;
            if (anInst->useAcc() || anInst->mayExpandToAccMacro())
            {
                canDoAccSub = false;
                break;
            }
        }

        if (!canDoAccSub)
        {
            continue;
        }
        else
        {
            replaceDstWithAcc(inst, 0);
            // advance iter to the last use of the acc
            instIter = subIter;
            --instIter;

            numAccSubDef++;
            numAccSubUse += (int)inst->use_size();

#if 0
            std::cout << "Acc sub def inst: \n";
            inst->emit(std::cout);
            std::cout << "[" << inst->getLocalId() << "]\n";
            std::cout << "Uses:\n";
            for (auto&& use : inst->useInstList)
            {
                std::cout << "\t";
                use.first->emit(std::cout);
                std::cout << "[" << use.first->getLocalId() << "]\n";
            }
#endif
        }
    }
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Mem_Manager.h"
#include "FlowGraph.h"
#include "RegAlloc.h"
#include "GraphColor.h"
#include "Timer.h"
#include "DebugInfo.h"
#include "VarSplit.h"

#include <bitset>
#include <climits>
#include <cmath>
#include <fstream>
#include <optional>
#include <vector>

using namespace vISA;

#define GRAPH_COLOR

PointsToAnalysis::PointsToAnalysis(const DECLARE_LIST &declares, unsigned int numBB) :
    numBBs(numBB), numAddrs(0), indirectUses(std::make_unique<REGVAR_VECTOR[]>(numBB))
{
    for (auto decl : declares)
    {
        //add alias check, For Alias Dcl
        if ((decl->getRegFile() == G4_ADDRESS || decl->getRegFile() == G4_SCALAR) &&
            decl->getAliasDeclare() == NULL)  // It is a base declaration, not alias
        {
            // participate liveness analysis
            decl->getRegVar()->setId(numAddrs++);
        }
        else
        {
            decl->getRegVar()->setId(UNDEFINED_VAL);
        }
    }

    // assign all addr aliases the same ID as its root
    for (auto decl : declares)
    {
        if ((decl->getRegFile() == G4_ADDRESS || decl->getRegFile() == G4_SCALAR) &&
            decl->getAliasDeclare() != NULL)
        {
            // participate liveness analysis
            decl->getRegVar()->setId(decl->getRootDeclare()->getRegVar()->getId());
        }
    }

    if (numAddrs > 0)
    {
        for (unsigned int i = 0; i < numAddrs; i++)
            regVars.push_back(NULL);

        for (auto decl : declares)
        {
            if ((decl->getRegFile() == G4_ADDRESS || decl->getRegFile() == G4_SCALAR) &&
                decl->getAliasDeclare() == NULL &&
                decl->getRegVar()->getId() != UNDEFINED_VAL)
            {
                regVars[decl->getRegVar()->getId()] = decl->getRegVar();
            }
        }

        pointsToSets.resize(numAddrs);
        addrExpSets.resize(numAddrs);
        addrPointsToSetIndex.resize(numAddrs);
        // initially each address variable has its own points-to set
        for (unsigned i = 0; i < numAddrs; i++)
        {
            addrPointsToSetIndex[i] = i;
        }
    }
}

// This function is intended to be invoked only in GRF RA. As that ensures points-to
// data structures are well populated and no new entry would be added to points-to
// table. If this condition is no longer true, then this function should be modified.
const std::unordered_map<G4_Declare*, std::vector<G4_Declare*>>& PointsToAnalysis::getPointsToMap() {
    // return map computed earlier
    // assume no updates are made to points-to analysis table since first update
    if (addrTakenMap.size() > 0)
        return addrTakenMap;

    unsigned idx = 0;

    // populate map from each addr reg -> addr taken targets
    for (auto& RV : regVars)
    {
        auto ptsToIdx = addrPointsToSetIndex[idx];
        for(auto& item : pointsToSets[ptsToIdx])
            addrTakenMap[RV->getDeclare()->getRootDeclare()].push_back(item.var->getDeclare()->getRootDeclare());
        ++idx;
    }

    return addrTakenMap;
}

const std::unordered_map<G4_Declare*, std::vector<G4_Declare*>>& PointsToAnalysis::getRevPointsToMap()
{
    if (revAddrTakenMap.size() > 0)
        return revAddrTakenMap;

    // call the function instead of using direct member to guarantee the map is populated
    auto& forwardMap = getPointsToMap();

    for (auto& entry : forwardMap)
    {
        for (auto& var : entry.second)
        {
            revAddrTakenMap[var].push_back(entry.first);
        }
    }

    return revAddrTakenMap;
}

//
//  A flow-insensitive algroithm to compute the register usage for indirect accesses.
//  The algorithm is divided into two parts:
//  1. We go through every basic block computing the points-to set for each adddress
//     variable.  This happens when we see an instruction like
//     mov (8) A0 &R0
//
//  2. We go through each basic block again, and for each r[A0] expression
//     we mark variables in A0's points-to set as used in the block
//
//  The algorithm is conservative but should work well for our inputs since
//  the front end pretty much always uses a fresh address variable when taking
//  the address of a GRF variable, wtih the exception of call-by-reference parameters
//  It's performed only once at the beginning of RA, at the point where all variables
//  are virtual and no spill code (either for address or GRF) has been inserted.
//
void PointsToAnalysis::doPointsToAnalysis(FlowGraph& fg)
{
    if (numAddrs == 0)
    {
        return;
    }

    // keep a list of address taken variables
    std::vector<G4_RegVar*> addrTakenDsts;
    std::map<G4_RegVar*, std::vector<std::pair<G4_AddrExp*, unsigned char>> > addrTakenMapping;
    std::vector< std::pair<G4_AddrExp*, unsigned char>> addrTakenVariables;

    for (G4_BB* bb : fg)
    {
        for (const G4_INST* inst : *bb)
        {
            G4_DstRegRegion* dst = inst->getDst();
            if (dst != NULL && dst->getRegAccess() == Direct && dst->getType() != Type_UD)
            {
                G4_VarBase* ptr = dst->getBase();

                for (int i = 0; i < G4_MAX_SRCS; i++)
                {
                    G4_Operand* src = inst->getSrc(i);
                    if (src != NULL && src->isAddrExp())
                    {
                        int offset = 0;
                        if (dst && !dst->isNullReg() && dst->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR)
                        {
                            offset = src->asAddrExp()->getOffset();
                        }
                        addrTakenMapping[ptr->asRegVar()].push_back(std::make_pair(src->asAddrExp(), offset));
                        addrTakenDsts.push_back(ptr->asRegVar());
                        addrTakenVariables.push_back(std::make_pair(src->asAddrExp(), offset));
                    }
                }
            }
        }
    }

    // first compute the points-to set for each address variable
    for (G4_BB* bb : fg)
    {
        for (G4_INST* inst : *bb)
        {
            if (inst->isPseudoKill() || inst->isLifeTimeEnd())
            {
                // No need to consider these lifetime placeholders for points2analysis
                continue;
            }

            G4_DstRegRegion* dst = inst->getDst();
            if (dst != NULL && dst->getRegAccess() == Direct && dst->getType() != Type_UD)
            {
                G4_VarBase* ptr = dst->getBase();
                //Dst is address variable
                if (ptr->isRegVar() && (ptr->asRegVar()->getDeclare()->getRegFile() == G4_ADDRESS || ptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR) &&
                    !ptr->asRegVar()->getDeclare()->isMsgDesc())
                {

                    // dst is an address variable.  ExDesc A0 may be ignored since they are never used in indirect access
                    if (inst->isMov() || inst->isPseudoAddrMovIntrinsic())
                    {
                        for (int i = 0; i < inst->getNumSrc(); i++)
                        {
                            G4_Operand* src = inst->getSrc(i);
                            if (!src || src->isNullReg())
                            {
                                continue;
                            }
                            if (src->isAddrExp())
                            {
                                // case 1:  mov A0 &GRF
                                G4_RegVar* addrTaken = src->asAddrExp()->getRegVar();

                                if (addrTaken != NULL)
                                {
                                    unsigned char offset = 0;
                                    if (ptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR)
                                    {
                                        offset = src->asAddrExp()->getOffset();
                                    }
                                    addToPointsToSet(ptr->asRegVar(), src->asAddrExp(), offset);
                                }
                            }
                            else
                            {
                                //G4_Operand* srcPtr = src->isSrcRegRegion() ? src->asSrcRegRegion()->getBase() : src;
                                G4_VarBase* srcPtr = src->isSrcRegRegion() ? src->asSrcRegRegion()->getBase() : nullptr;

                                if (srcPtr && srcPtr->isRegVar() && (srcPtr->asRegVar()->getDeclare()->getRegFile() == G4_ADDRESS))
                                {
                                    // case 2:  mov A0 A1
                                    // merge the two addr's points-to set together
                                    if (ptr->asRegVar()->getId() != srcPtr->asRegVar()->getId())
                                    {
                                        mergePointsToSet(srcPtr->asRegVar(), ptr->asRegVar());
                                    }
                                }
                                else
                                {
                                    // case ?: mov v1 A0
                                    // case ?: mov A0 v1
                                    if (srcPtr &&
                                        srcPtr->isRegVar() &&
                                        addrTakenMapping[srcPtr->asRegVar()].size() != 0)
                                    {
                                        for (int i = 0; i < (int)addrTakenMapping[srcPtr->asRegVar()].size(); i++)
                                        {
                                            addToPointsToSet(ptr->asRegVar(), addrTakenMapping[srcPtr->asRegVar()][i].first, addrTakenMapping[srcPtr->asRegVar()][i].second);
                                        }
                                    }
                                    else
                                    {
                                        // case 3: mov A0 0
                                        // Initial of address register, igore the point to analysis
                                        // FIXME: currently, vISA don't expect mov imm value to the address register. So, 0 is treated as initialization.
                                        // If support mov A0 imm in future, 0 may be R0.
                                        if (!(src->isImm() && (src->asImm()->getImm() == 0)))
                                        {
                                            // case 4:  mov A0 V2
                                            // conservatively assume address can point to anything
                                            DEBUG_MSG("unexpected addr move for pointer analysis:\n");
                                            DEBUG_EMIT(inst);
                                            DEBUG_MSG("\n");
                                            for (int i = 0, size = (int)addrTakenVariables.size(); i < size; i++)
                                            {
                                                addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first, addrTakenVariables[i].second);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (inst->isArithmetic())
                    {
                        G4_Operand* src0 = inst->getSrc(0);
                        G4_Operand* src1 = inst->getSrc(1);
                        bool src0addr = false;
                        if (src0->isAddrExp())
                        {
                            src0addr = true;
                        }
                        else if (src0->isSrcRegRegion() && src0->getRegAccess() == Direct)
                        {
                            if (src0->isAddress())
                            {
                                src0addr = true;
                            }
                        }

                        bool src1addr = false;
                        if (src1->isAddrExp())
                        {
                            src1addr = true;
                        }
                        else if (src1->isSrcRegRegion() && src1->getRegAccess() == Direct)
                        {
                            if (src1->isAddress())
                            {
                                src1addr = true;
                            }
                        }

                        if (src0addr ^ src1addr)
                        {
                            G4_Operand* src = src0addr ? src0 : src1;

                            if (src->isAddrExp())
                            {
                                // case 5:  add/mul A0 &GRF src1
                                addToPointsToSet(ptr->asRegVar(), src->asAddrExp(), 0);
                            }
                            else
                            {
                                G4_VarBase* srcPtr = src->isSrcRegRegion() ? src->asSrcRegRegion()->getBase() : nullptr;
                                // case 6:  add/mul A0 A1 src1
                                // merge the two addr's points-to set together
                                if (srcPtr && (ptr->asRegVar()->getId() != srcPtr->asRegVar()->getId()))
                                {
                                    mergePointsToSet(srcPtr->asRegVar(), ptr->asRegVar());
                                }
                            }
                        }
                        else if (ptr->isRegVar() && ptr->asRegVar()->isPhyRegAssigned())
                        {
                            // OK, using builtin a0 or a0.2 directly.
                        }
                        else
                        {
                            // case 7:  add/mul A0 V1 V2
                            DEBUG_MSG("unexpected addr add/mul for pointer analysis:\n");
                            DEBUG_EMIT(inst);
                            DEBUG_MSG("\n");
                            for (int i = 0; i < (int)addrTakenVariables.size(); i++)
                            {
                                addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first, 0);
                            }
                        }
                    }
                    else
                    {
                        // case 8: A0 = ???
                        DEBUG_MSG("unexpected instruction with address destination:\n");
                        DEBUG_EMIT(inst);
                        DEBUG_MSG("\n");
                        for (int i = 0; i < (int)addrTakenVariables.size(); i++)
                        {
                            addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first, addrTakenVariables[i].second);
                        }
                    }
                }
                else if (ptr->isRegVar() && !ptr->asRegVar()->getDeclare()->isMsgDesc())
                {
                    for (int i = 0; i < G4_MAX_SRCS; i++)
                    {
                        G4_Operand* src = inst->getSrc(i);
                        G4_VarBase* srcPtr = (src && src->isSrcRegRegion()) ? src->asSrcRegRegion()->getBase() : nullptr;
                        //We don't support using "r[a0.0]" as address expression.
                        //For instructions like following, it's not point-to propagation for simdShuffle and add64_i_i_i_i.
                        //(W) mov (1)              simdShuffle(0,0)<1>:d  r[A0(0,0), 0]<0;1,0>:d
                        //    pseudo_mad (16)      add64_i_i_i_i(0,0)<1>:d  0x6:w  simdShuffle(0,0)<0;0>:d  rem_i_i_i_i(0,0)<1;0>:d // $470:
                        //    shl (16)             add64_i_i_i_i(0,0)<1>:d  add64_i_i_i_i(0,0)<1;1,0>:d  0x2:w // $472:
                        if (srcPtr != nullptr && srcPtr->isRegVar() && ptr != srcPtr && src->getRegAccess() != IndirGRF)
                        {
                            std::vector<G4_RegVar*>::iterator addrDst = std::find(addrTakenDsts.begin(), addrTakenDsts.end(), srcPtr->asRegVar());
                            if (addrDst != addrTakenDsts.end())
                            {
                                addrTakenDsts.push_back(ptr->asRegVar());
                                for (int i = 0; i < (int)addrTakenMapping[srcPtr->asRegVar()].size(); i++)
                                {
                                    addrTakenMapping[ptr->asRegVar()].push_back(addrTakenMapping[srcPtr->asRegVar()][i]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

#ifdef DEBUG_VERBOSE_ON
    DEBUG_VERBOSE("Results of points-to analysis:\n");
    for (unsigned int i = 0; i < numAddrs; i++)
    {
        DEBUG_VERBOSE("Addr " << i);
        for (G4_RegVar *grf : pointsToSets[addrPointsToSetIndex[i]])
        {
            DEBUG_EMIT(grf);
            DEBUG_VERBOSE("\t");
        }
        DEBUG_VERBOSE("\n");
    }
#endif

    // mark GRF that may be used indirect access as live in the block
    // This includes all GRFs in the address's points-to set
    for (auto bb : fg)
    {
        for (G4_INST* inst : *bb)
        {
            G4_DstRegRegion* dst = inst->getDst();

            if (dst != NULL &&
                dst->getRegAccess() == IndirGRF)
            {
                G4_VarBase* dstptr = dst->getBase();
                MUST_BE_TRUE(dstptr->isRegVar() && (dstptr->asRegVar()->getDeclare()->getRegFile() == G4_ADDRESS || dstptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR),
                    "base must be address");
                addPointsToSetToBB(bb->getId(), dstptr->asRegVar());
            }

            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                //
                // look for indirect reg access r[ptr] which refers addrTaken reg var
                //
                if (inst->getSrc(j) == NULL || !inst->getSrc(j)->isSrcRegRegion()) {
                    continue;
                }

                G4_SrcRegRegion* src = inst->getSrc(j)->asSrcRegRegion();

                if (src->getRegAccess() == IndirGRF)
                {
                    G4_VarBase* srcptr = src->getBase();
                    MUST_BE_TRUE(srcptr->isRegVar() && (srcptr->asRegVar()->getDeclare()->getRegFile() == G4_ADDRESS || srcptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR),
                        "base must be address");
                    addPointsToSetToBB(bb->getId(), srcptr->asRegVar());
                }
            }
        }
    }

#ifndef NDEBUG
    for (unsigned i = 0; i < numAddrs; i++)
    {
        REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[i]];
        for (const pointInfo cur : vec)
        {
            unsigned indirectVarSize = cur.var->getDeclare()->getByteSize();
            assert((indirectVarSize <= fg.builder->getGRFSize()* fg.getKernel()->getNumRegTotal()) && "indirected variables' size is larger than GRF file size");
        }
    }
#endif

#ifdef DEBUG_VERBOSE_ON
    for (unsigned int i = 0; i < numBBs; i++)
    {
        DEBUG_VERBOSE("Indirect uses for BB" << i << "\t");
        const REGVAR_VECTOR &grfVec = getIndrUseVectorForBB(i);
        for (G4_RegVar* grf : grfVec)
        {
            DEBUG_EMIT(grf);
            DEBUG_VERBOSE("\t");
        }
        DEBUG_VERBOSE("\n");
    }
#endif

}

bool LivenessAnalysis::isLocalVar(G4_Declare* decl) const
{
    if ((decl->isInput() == true &&
        !(fg.builder->getFCPatchInfo() &&
            fg.builder->getFCPatchInfo()->getFCComposableKernel() &&
            !decl->isLiveIn())) &&
        !(fg.builder->isPreDefArg(decl) &&
            (fg.builder->getIsKernel() ||
                (fg.getIsStackCallFunc() &&
                    fg.builder->getArgSize() == 0))))
        return false;

    if (fg.builder->getOption(vISA_enablePreemption) &&
        decl == fg.builder->getBuiltinR0())
        return false;


    if (decl->isOutput() == true &&
        !(fg.builder->isPreDefRet(decl) &&
            (fg.builder->getIsKernel() ||
                (fg.getIsStackCallFunc() &&
                    fg.builder->getRetVarSize() == 0))))
        return false;

    LocalLiveRange* dclLR = gra.getLocalLR(decl);
    if (decl &&
        dclLR &&
        dclLR->isLiveRangeLocal()&&
        decl->getRegFile() != G4_INPUT)
    {
        return true;
    }

    // Since variable split is done only for local variables, there is no need to analysis for global variables.
    if (decl->getIsSplittedDcl() || decl->getIsPartialDcl())
    {
        return true;
    }

    return false;
}

bool LivenessAnalysis::setGlobalVarIDs(bool verifyRA, bool areAllPhyRegAssigned)
{
    bool phyRegAssigned = areAllPhyRegAssigned;

    for (G4_Declare* decl : gra.kernel.Declares)
    {
        if (!isLocalVar(decl))
        {
            /* Note that we only do local split, so there is no need to handle partial declare and splitted declare */
            if (livenessCandidate(decl, verifyRA) && decl->getAliasDeclare() == NULL)
            {
                decl->getRegVar()->setId(numGlobalVarId++);
                if (decl->getRegVar()->getPhyReg() == NULL && !decl->getIsPartialDcl())
                    numUnassignedVarId++;
                if (decl->getRegVar()->isPhyRegAssigned() == false)
                {
                    phyRegAssigned = false;
                }
            }
            else
            {
                decl->getRegVar()->setId(UNDEFINED_VAL);
            }
        }
    }

    return phyRegAssigned;
}

bool LivenessAnalysis::setLocalVarIDs(bool verifyRA, bool areAllPhyRegAssigned)
{
    numVarId = numGlobalVarId;
    bool phyRegAssigned = areAllPhyRegAssigned;

    for (G4_Declare* decl : gra.kernel.Declares)
    {
        if (isLocalVar(decl))
        {
            if (livenessCandidate(decl, verifyRA) && decl->getAliasDeclare() == NULL)
            {
                if (decl->getIsSplittedDcl())
                {
                    decl->setSplitVarStartID(0);
                }
                if (decl->getIsPartialDcl())
                {
                    auto declSplitDcl = gra.getSplittedDeclare(decl);
                    if (declSplitDcl && declSplitDcl->getIsSplittedDcl())
                    {
                        if (numSplitStartID == 0)
                        {
                            numSplitStartID = numVarId;
                        }

                        if (declSplitDcl->getSplitVarStartID() == 0)
                        {
                            declSplitDcl->setSplitVarStartID(numVarId);
                        }
                        numSplitVar++;
                    }
                    else
                    {
                        assert(0 && "Found child declare without parent");
                    }
                }

                decl->getRegVar()->setId(numVarId++);
                if (decl->getRegVar()->getPhyReg() == NULL && !decl->getIsPartialDcl())
                    numUnassignedVarId++;
                if (decl->getRegVar()->isPhyRegAssigned() == false)
                {
                    phyRegAssigned = false;
                }
            }
            else
            {
                decl->getRegVar()->setId(UNDEFINED_VAL);
            }
        }
    }

    return phyRegAssigned;
}

bool LivenessAnalysis::setVarIDs(bool verifyRA, bool areAllPhyRegAssigned)
{
    bool phyRegAssigned = areAllPhyRegAssigned;
    for (G4_Declare* decl : gra.kernel.Declares)
    {

        if (livenessCandidate(decl, verifyRA) && decl->getAliasDeclare() == NULL)
        {
            if (decl->getIsSplittedDcl())
            {
                decl->setSplitVarStartID(0);
            }
            if (decl->getIsPartialDcl())
            {
                auto declSplitDcl = gra.getSplittedDeclare(decl);
                if (declSplitDcl->getIsSplittedDcl())
                {
                    if (numSplitStartID == 0)
                    {
                        numSplitStartID = numVarId;
                    }

                    if (declSplitDcl->getSplitVarStartID() == 0)
                    {
                        declSplitDcl->setSplitVarStartID(numVarId);
                    }
                    numSplitVar++;
                }
                else
                {
                    assert(0 && "Found child declare without parent");
                }
            }

            // participate liveness analysis
            decl->getRegVar()->setId(numVarId++);

            if (decl->getRegVar()->getPhyReg() == NULL && !decl->getIsPartialDcl())
                numUnassignedVarId++;

            //
            // dump Reg Var info for debugging
            //

            if (decl->getRegVar()->isPhyRegAssigned() == false)
            {
                phyRegAssigned = false;
            }
#ifdef DEBUG_VERBOSE_ON
            DEBUG_EMIT(decl->getRegVar());
            DEBUG_VERBOSE(" id = " << decl->getRegVar()->getId() << std::endl);
#endif
        }
        //
        // those reg vars that are not candidates, set their id to
        // undefined value
        //
        else
        {
            decl->getRegVar()->setId(UNDEFINED_VAL);
        }
    }

    numGlobalVarId = numVarId;

    return phyRegAssigned;
}

LivenessAnalysis::LivenessAnalysis(
        GlobalRA& g,
        unsigned char kind,
        bool verifyRA,
        bool forceRun) :
        selectedRF(kind),
        pointsToAnalysis(g.pointsToAnalysis), m(4096), gra(g), fg(g.kernel.fg)
{
    //
    // NOTE:
    // The maydef sets are simply aliases to the mayuse sets, since their uses are
    // mutually exclusive.
    //
    // Go over each reg var if it's a liveness candidate, assign id for bitset.
    //
    bool areAllPhyRegAssigned = !forceRun;
    bool hasStackCall = fg.getHasStackCalls() || fg.getIsStackCallFunc();

    if ((selectedRF & G4_GRF) &&
        !fg.builder->getOption(vISA_GlobalSendVarSplit) && !hasStackCall)
    {
        areAllPhyRegAssigned = setGlobalVarIDs(verifyRA, areAllPhyRegAssigned);
        areAllPhyRegAssigned = setLocalVarIDs(verifyRA, areAllPhyRegAssigned);
    }
    else
    {
        areAllPhyRegAssigned = setVarIDs(verifyRA, areAllPhyRegAssigned);
    }

    // For Alias Dcl
    for (auto decl : gra.kernel.Declares)
    {
        if (livenessCandidate(decl, verifyRA) && decl->getAliasDeclare() != NULL)
        {
            // It is an alias declaration. Set its id = base declaration id
            decl->getRegVar()->setId(decl->getAliasDeclare()->getRegVar()->getId());
        }
#ifdef DEBUG_VERBOSE_ON
        DEBUG_EMIT(decl->getRegVar());
        DEBUG_VERBOSE(" id = " << decl->getRegVar()->getId() << std::endl);
#endif
    }

    //
    // if no chosen candidate for reg allocation return
    //
    if (numVarId == 0 ||
        (verifyRA == false &&
         areAllPhyRegAssigned == true))
    {
        // If all variables have physical register assignments
        // there are no candidates for allocation
        numVarId = 0;
        return;
    }

    //
    // put selected reg vars into vars[]
    //
    vars.resize(numVarId);
    for (auto dcl : gra.kernel.Declares)
    {
        if (livenessCandidate(dcl, verifyRA) &&
            dcl->getAliasDeclare() == NULL)
        {
            G4_RegVar* var = dcl->getRegVar();
            vars[var->getId()] = var;
        }
    }

    addr_taken = BitSet(numVarId, false);

    numBBId = (unsigned) fg.size();

    def_in.resize(numBBId);
    def_out.resize(numBBId);
    use_in.resize(numBBId);
    use_out.resize(numBBId);
    use_gen.resize(numBBId);
    use_kill.resize(numBBId);
    indr_use.resize(numBBId);

    for (unsigned i = 0; i < numBBId; i++)
    {
        def_in[i]  = SparseBitSet(numVarId);
        def_out[i] = SparseBitSet(numVarId);
        use_in[i]  = SparseBitSet(numVarId);
        use_out[i] = SparseBitSet(numVarId);
        use_gen[i] = SparseBitSet(numVarId);
        use_kill[i]= SparseBitSet(numVarId);
        indr_use[i]= SparseBitSet(numVarId);
    }
}

LivenessAnalysis::~LivenessAnalysis()
{
    //
    // if no chosen candidate for reg allocation return
    //
    if (numVarId == 0)
    {
        return;
    }

    // Remove liveness inserted pseudo kills
    for (auto bb : fg)
    {
        for (auto instIt = bb->begin(); instIt != bb->end();)
        {
            auto inst = (*instIt);
            if (inst->isPseudoKill())
            {
                auto src0 = inst->getSrc(0);
                MUST_BE_TRUE(src0 && src0->isImm(), "expecting src0 immediate for pseudo kill");
                if (src0->asImm()->getImm() == PseudoKillType::FromLiveness)
                {
                    instIt = bb->erase(instIt);
                    continue;
                }
            }
            ++instIt;
        }
    }
}

bool LivenessAnalysis::livenessCandidate(const G4_Declare* decl, bool verifyRA) const
{
    const LocalLiveRange* declLR = nullptr;
    if (verifyRA == false && (declLR = gra.getLocalLR(decl)) && declLR->getAssigned() && !declLR->isEOT())
    {
        return false;
    }
    else if ((selectedRF & decl->getRegFile()))
    {
        if (selectedRF & G4_GRF)
        {
            if ((decl->getRegFile() & G4_INPUT) && decl->getRegVar()->isPhyRegAssigned() && !decl->getRegVar()->isGreg())
            {
                return false;
            }
            if (decl->getByteSize() == 0)
            {
                // regrettably, this can happen for arg/retval pre-defined variable
                return false;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

void LivenessAnalysis::updateKillSetForDcl(G4_Declare* dcl, SparseBitSet* curBBGen, SparseBitSet* curBBKill, G4_BB* curBB, SparseBitSet* entryBBGen, SparseBitSet* entryBBKill, G4_BB* entryBB, unsigned scopeID)
{
    if (scopeID != 0 &&
        scopeID != UINT_MAX &&
        dcl->getScopeID() == scopeID)
    {
        entryBBKill->set(dcl->getRegVar()->getId(), true);
        entryBBGen->set(dcl->getRegVar()->getId(), false);
#ifdef DEBUG_VERBOSE_ON
        DEBUG_VERBOSE("Killed sub-routine scope " << dcl->getName() << " at bb with id = " << entryBB->getId() << std::endl);
#endif
    }
}

// Scoping info is stored per decl. A variable can be either global scope (default),
// sub-routine scope, or basic block scope. This function iterates over all
// instructions and their operands in curBB and if scoping for it is set in symbol
// table then it marks kills accordingly. A bb scope variable is killed in the bb it appears
// and a sub-routine local variable is killed in entry block of the sub-routine. No
// error check is performed currently so if variable scoping information is incorrect
// then generated code will be so too.
void LivenessAnalysis::performScoping(SparseBitSet* curBBGen, SparseBitSet* curBBKill, G4_BB* curBB, SparseBitSet* entryBBGen, SparseBitSet* entryBBKill, G4_BB* entryBB)
{
    unsigned scopeID = curBB->getScopeID();
    for (G4_INST* inst : *curBB)
    {
        G4_DstRegRegion* dst = inst->getDst();

        if (dst &&
            dst->getBase()->isRegAllocPartaker())
        {
            G4_Declare* dcl = GetTopDclFromRegRegion(dst);
            updateKillSetForDcl(dcl, curBBGen, curBBKill, curBB, entryBBGen, entryBBKill, entryBB, scopeID);
        }

        for (int i = 0; i < G4_MAX_SRCS; i++)
        {
            G4_Operand* src = inst->getSrc(i);

            if (src)
            {
                if (src->isSrcRegRegion() &&
                    src->asSrcRegRegion()->getBase()->isRegAllocPartaker())
                {
                    G4_Declare* dcl = GetTopDclFromRegRegion(src);
                    updateKillSetForDcl(dcl, curBBGen, curBBKill, curBB, entryBBGen, entryBBKill, entryBB, scopeID);
                }
                else if (src->isAddrExp() &&
                    src->asAddrExp()->getRegVar()->isRegAllocPartaker())
                {
                    G4_Declare* dcl = src->asAddrExp()->getRegVar()->getDeclare()->getRootDeclare();

                    updateKillSetForDcl(dcl, curBBGen, curBBKill, curBB, entryBBGen, entryBBKill, entryBB, scopeID);
                }
            }
        }
    }
}

void LivenessAnalysis::detectNeverDefinedVarRows()
{
    // This function records variables and its rows that are never defined
    // in the kernel. This information helps detect kills for partial
    // writes when VISA optimizer optimizes away some rows of a variable.
    // In interest of compile time we only look for full rows that are
    // not defined rather than sub-regs.
    std::unordered_map<G4_Declare*, BitSet> largeDefs;

    // Populate largeDefs map with dcls > 1 GRF size
    for (auto dcl : gra.kernel.Declares)
    {
        if (dcl->getAliasDeclare() || dcl->getIsPartialDcl() || dcl->getAddressed())
            continue;

        if (dcl->getRegFile() != G4_GRF)
            continue;

        unsigned int dclNumRows = dcl->getNumRows();

        if (dclNumRows < 2)
            continue;

        BitSet bitset(dclNumRows, false);

        largeDefs.insert(std::make_pair(dcl, std::move(bitset)));
    }

    if (largeDefs.empty())
        return;

    const unsigned bytesPerGRF = fg.builder->numEltPerGRF<Type_UB>();

    // Update row usage of each dcl in largeDefs
    for (auto bb : gra.kernel.fg)
    {
        for (auto inst : *bb)
        {
            auto dst = inst->getDst();
            if (!dst)
                continue;

            auto dstTopDcl = dst->getTopDcl();

            if (dstTopDcl)
            {
                auto it = largeDefs.find(dstTopDcl);

                if (it == largeDefs.end())
                {
                    continue;
                }

                unsigned int rowStart = dst->getLeftBound() / bytesPerGRF;
                unsigned int rowEnd = dst->getRightBound() / bytesPerGRF;

                it->second.set(rowStart, rowEnd);
            }
        }
    }

    // Propagate largeDefs to neverDefinedRows bit vector to later bitwise OR it
    for (auto it : largeDefs)
    {
        unsigned int numRows = it.first->getNumRows();
        BitSet* undefinedRows = nullptr;
        for (unsigned int i = 0; i < numRows; i++)
        {
            if (!it.second.isSet(i))
            {
                if (undefinedRows == nullptr)
                {
                    undefinedRows = &neverDefinedRows.emplace(it.first, BitSet(it.first->getByteSize(), false)).first->second;
                }
                undefinedRows->set(i * bytesPerGRF, i * bytesPerGRF + bytesPerGRF - 1);
            }
        }
    }
}

//
// compute liveness of reg vars
// Each reg var indicates a region within the register file. As such, the case in which two consecutive defs
// of a reg region without any use in between does not mean the second def overwrites the first one because the two defs
// may write different parts of the region. Def vectors are used to track which definitions of reg vars reach
// the entry and the end of a basic block, which tell us the first definitions of reg vars. Use vectors track which
// uses of reg vars are anticipated, which tell use the uses of reg vars.Def and Use vectors encapsulate the liveness
// of reg vars.
//
void LivenessAnalysis::computeLiveness()
{
    //
    // no reg var is selected, then no need to compute liveness
    //
    if (getNumSelectedVar() == 0)
    {
        return;
    }

    startTimer(TimerID::LIVENESS);

#ifdef DEBUG_VERBOSE_ON
    std::vector<FuncInfo*>& fns = fg.funcInfoTable;
#endif
    //
    // mark input arguments live at the entry of kernel
    // mark output arguments live at the exit of kernel
    //
    SparseBitSet inputDefs(numVarId);
    SparseBitSet outputUses(numVarId);

    for (unsigned i = 0; i < numVarId; i++)
    {
        bool setLiveIn = false;
        if (!vars[i])
            continue;

        G4_Declare *decl = vars[i]->getDeclare();

        if ((decl->isInput() == true &&
            !(fg.builder->getFCPatchInfo() &&
                fg.builder->getFCPatchInfo()->getFCComposableKernel() &&
                !decl->isLiveIn())) &&
            !(fg.builder->isPreDefArg(decl) &&
                (fg.builder->getIsKernel() ||
                    (fg.getIsStackCallFunc() &&
                        fg.builder->getArgSize() == 0))))
            setLiveIn = true;

        if (fg.builder->getOption(vISA_enablePreemption) &&
            decl == fg.builder->getBuiltinR0())
            setLiveIn = true;

        if(setLiveIn)
        {
            inputDefs.set(i, true);
#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("First def input = " << decl->getName() << std::endl);
#endif
        }

        bool setLiveOut = false;
        if (decl->isOutput() == true &&
            !(fg.builder->isPreDefRet(decl) &&
                (fg.builder->getIsKernel() ||
                    (fg.getIsStackCallFunc() &&
                        fg.builder->getRetVarSize() == 0))))
            setLiveOut = true;

        if (fg.builder->getOption(vISA_enablePreemption) &&
            decl == fg.builder->getBuiltinR0())
            setLiveOut = true;

        if(setLiveOut)
        {
            outputUses.set(i, true);
#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("First def output    = " << decl->getName() << std::endl);
#endif
        }
    }

    //
    // clean up def_in & def_out that are used in markFirstDef
    //
    for (unsigned i = 0; i < numBBId; i++)
    {
        def_in[i].clear();
        def_out[i].clear();
    }

    if (livenessClass(G4_GRF))
        detectNeverDefinedVarRows();

    //
    // compute def_out and use_in vectors for each BB
    //
    for (G4_BB * bb  : fg)
    {
        unsigned id = bb->getId();

        computeGenKillandPseudoKill(bb, def_out[id], use_in[id], use_gen[id], use_kill[id]);

        //
        // exit block: mark output parameters live
        //
        if (bb->Succs.empty())
        {
            use_out[id] = outputUses;
        }
    }

    G4_BB* subEntryBB = NULL;
    SparseBitSet* subEntryKill = NULL;
    SparseBitSet* subEntryGen = NULL;

    if (fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM)
    {
        //
        // Top-down order of BB list iteration guarantees that
        // entry BB of each sub-routine will be seen before any other
        // BBs belonging to that sub-routine. This assumes that BBs of
        // a sub-routine are laid out back to back in bb list.
        //
        for (auto bb : fg)
        {
            unsigned id = bb->getId();

            if (bb->getScopeID() != 0 &&
                bb->getScopeID() != UINT_MAX)
            {
                subEntryBB = fg.sortedFuncTable[bb->getScopeID() - 1]->getInitBB();
                unsigned entryBBID = subEntryBB->getId();
                subEntryKill = &use_kill[entryBBID];
                subEntryGen = &use_gen[entryBBID];
            }

            //
            // Mark explicitly scoped variables as kills
            //
            performScoping(&use_gen[id], &use_kill[id], bb, subEntryGen, subEntryKill, subEntryBB);
        }
    }

    //
    // compute indr accesses
    //
    if (selectedRF & G4_GRF)
    {
        // only GRF variables can have their address taken
        for (auto bb : fg)
        {
            const REGVAR_VECTOR& grfVec = pointsToAnalysis.getIndrUseVectorForBB(bb->getId());
            for (const pointInfo addrTaken : grfVec)
            {
                indr_use[bb->getId()].set(addrTaken.var->getId(), true);
                addr_taken.set(addrTaken.var->getId(), true);
            }
        }
    }
    //
    // Perform inter-procedural context-sensitive flow analysis.
    // This is required when the CFG involves function calls with multiple calling
    // contexts for the same function, as peforming just a context-insensitive
    // analysis results in uses being propgated along paths that are not feasible
    // in the actual program.
    //
    if (performIPA())
    {
        hierarchicalIPA(inputDefs, outputUses);
        stopTimer(TimerID::LIVENESS);
        return;
    }


    if (fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
        (selectedRF & G4_GRF || selectedRF & G4_FLAG) &&
        (numFnId > 0))
    {
        // compute the maydef for each subroutine
        maydefAnalysis();

        //
        // dump vectors for debugging
        //
#ifdef DEBUG_VERBOSE_ON
        dump_bb_vector("MAYDEF IN", maydef_in);
        dump_bb_vector("MAYDEF OUT", maydef_out);
        dump_fn_vector("MAYDEF", fns, maydef);
#endif
    }

    auto getPostOrder = [](G4_BB *S, std::vector<G4_BB *> &PO) {
      std::stack<std::pair<G4_BB *, BB_LIST_ITER>> Stack;
      std::set<G4_BB *> Visited;

      Stack.push({S, S->Succs.begin()});
      Visited.insert(S);
      while (!Stack.empty()) {
        G4_BB *Curr = Stack.top().first;
        BB_LIST_ITER It = Stack.top().second;

        if (It != Curr->Succs.end()) {
          G4_BB *Child = *Stack.top().second++;
          if (Visited.insert(Child).second) {
            Stack.push({Child, Child->Succs.begin()});
          }
          continue;
        }
        PO.push_back(Curr);
        Stack.pop();
      }
    };

    std::vector<G4_BB *> PO;
    getPostOrder(fg.getEntryBB(), PO);

    bool change;

    //
    // backward flow analysis to propagate uses (locate last uses)
    //
    do {
        change = false;
        for (auto I = PO.begin(), E = PO.end(); I != E; ++I)
            change |= contextFreeUseAnalyze(*I, change);
    } while (change);

    //
    // initialize entry block with payload input
    //
    def_in[fg.getEntryBB()->getId()] = inputDefs;

    //
    // forward flow analysis to propagate defs (locate first defs)
    //
    do {
        change = false;
        for (auto I = PO.rbegin(), E = PO.rend(); I != E; ++I)
            change |= contextFreeDefAnalyze(*I, change);
    } while (change);

#if 0
    // debug code to compare old v. new IPA
    {
        std::vector<G4_Declare*> idToDecl;
        idToDecl.resize(numVarId);
        for (auto dcl : fg.getKernel()->Declares)
        {
            auto id = dcl->getRegVar()->getId();
            if (id < numVarId)
            {
                idToDecl[id] = dcl;
            }
        }

        fg.getKernel()->dump(std::cerr);

        auto printLive = [this, &idToDecl](int id)
        {
            std::cerr << "Liveness for " << idToDecl[id]->getName() << "\n";
            std::cerr << "Use In: ";
            for (int i = 0; i < (int) useInCopy.size(); ++i)
            {
                if (useInCopy[i].isSet(id))
                {
                    std::cerr << "BB" << i << " ";
                }
            }
            std::cerr << "\n";
            std::cerr << "Use Out: ";
            for (int i = 0; i < (int) useOutCopy.size(); ++i)
            {
                if (useOutCopy[i].isSet(id))
                {
                    std::cerr << "BB" << i << " ";
                }
            }
            std::cerr << "\n";
        };

        auto printSetDiff = [&idToDecl, this](const std::vector<BitSet>& set1,
            const std::vector<BitSet>& set2)
        {
            for (int i = 0, size = (int) set1.size(); i < size; ++i)
            {
                bool printBB = true;
                for (int j = 0; j < (int)numVarId; ++j)
                {
                    if (set1[i].isSet(j) ^ set2[i].isSet(j))
                    {
                        if (printBB)
                        {
                            std::cerr << "BB" << i << ": ";
                            printBB = false;
                        }
                        std::cerr << idToDecl[j]->getName() << "(" << j << "):" <<
                            (set1[i].isSet(j) ? 1 : 0) << " ";
                    }
                }
                if (!printBB)
                {
                    std::cerr << "\n";
                }
            }
        };

        std::cerr << "use-in comparison:\n";
        printSetDiff(use_in, useInCopy);

        std::cerr << "use-out comparison:\n";
        printSetDiff(use_out, useOutCopy);
    }
#endif

    //
    // dump vectors for debugging
    //
#if 0
    {
        dump_bb_vector("DEF IN", def_in);
        dump_bb_vector("DEF OUT", def_out);
        dump_bb_vector("USE IN", use_in);
        dump_bb_vector("USE OUT", use_out);
    }
#endif

    stopTimer(TimerID::LIVENESS);
}

//
// compute the maydef set for every subroutine
// This includes recursively all the variables that are defined by the
// subroutine, but does not include defs in the caller
// This means this must be called before we do fix-point on def_in/def_out
// and destroy their original values
// This is used by augmentation later to model all variables that may be defined by a call
// FIXME: we should use a separate def set to represent declares defined in each BB
//
void LivenessAnalysis::maydefAnalysis()
{
    for (auto func : fg.sortedFuncTable)
    {
        unsigned fid = func->getId();
        if (fid == UINT_MAX)
        {
            // entry kernel
            continue;
        }

        auto& BV = subroutineMaydef[func];
        for (auto&& bb : func->getBBList())
        {
            BV |= def_out[bb->getId()];
        }
        for (auto&& callee : func->getCallees())
        {
            BV |= subroutineMaydef[callee];
        }
    }
}

//
// Use analysis for this subroutine only
// use_out[call-BB] = use_in[ret-BB]
// use_out[exit-BB] should be initialized by the caller
//
void LivenessAnalysis::useAnalysis(FuncInfo* subroutine)
{
    bool changed = false;
    do
    {
        changed = false;
        for (auto BI = subroutine->getBBList().rbegin(), BE = subroutine->getBBList().rend(); BI != BE; ++BI)
        {
            //
            // use_out = use_in(s1) + use_in(s2) + ...
            // where s1 s2 ... are the successors of bb
            // use_in  = use_gen + (use_out - use_kill)
            //
            G4_BB* bb = *BI;
            unsigned bbid = bb->getId();
            if (bb->getBBType() & G4_BB_EXIT_TYPE)
            {
                // use_out is set by caller
            }
            else if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                use_out[bbid] |= use_in[bb->getPhysicalSucc()->getId()];
            }
            else
            {
                for (auto succ : bb->Succs)
                {
                    use_out[bbid] |= use_in[succ->getId()];
                }
            }

            if (changed)
            {
                // no need to update changed, save a copy
                use_in[bbid] = use_out[bbid];
                use_in[bbid] -= use_kill[bbid];
                use_in[bbid] |= use_gen[bbid];
            }
            else
            {
                SparseBitSet oldUseIn = use_in[bbid];

                use_in[bbid] = use_out[bbid];
                use_in[bbid] -= use_kill[bbid];
                use_in[bbid] |= use_gen[bbid];

                if (!(bb->getBBType() & G4_BB_INIT_TYPE) && oldUseIn != use_in[bbid])
                {
                    changed = true;
                }
            }
        }
    } while (changed);
}

//
// Use analysis for this subroutine only, considering both arg/retval of its callees
// use_out[call-BB] = (use_in[ret-BB] | arg[callee]) - retval[callee]
//
void LivenessAnalysis::useAnalysisWithArgRetVal(FuncInfo* subroutine,
    const std::unordered_map<FuncInfo*, SparseBitSet>& args, const std::unordered_map<FuncInfo*, SparseBitSet>& retVal)
{
    bool changed = false;
    do
    {
        changed = false;
        for (auto BI = subroutine->getBBList().rbegin(), BE = subroutine->getBBList().rend(); BI != BE; ++BI)
        {
            //
            // use_out = use_in(s1) + use_in(s2) + ...
            // where s1 s2 ... are the successors of bb
            // use_in  = use_gen + (use_out - use_kill)
            //
            G4_BB* bb = *BI;
            unsigned bbid = bb->getId();
            if (bb->getBBType() & G4_BB_EXIT_TYPE)
            {
                // use_out is set by previous analysis
            }
            else if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                use_out[bbid] = use_in[bb->getPhysicalSucc()->getId()];
                auto callee = bb->getCalleeInfo();
                auto BVIt = args.find(callee);
                MUST_BE_TRUE(BVIt != args.end(), "Missing entry in map");
                use_out[bbid] |= (*BVIt).second;
                BVIt = retVal.find(callee);
                MUST_BE_TRUE(BVIt != retVal.end(), "Missing entry in map");
                use_out[bbid] -= (*BVIt).second;
            }
            else
            {
                for (auto succ : bb->Succs)
                {
                    use_out[bbid] |= use_in[succ->getId()];
                }
            }

            if (changed)
            {
                // no need to update changed, save a copy
                use_in[bbid] = use_out[bbid];
                use_in[bbid] -= use_kill[bbid];
                use_in[bbid] |= use_gen[bbid];
            }
            else
            {
                SparseBitSet oldUseIn = use_in[bbid];

                use_in[bbid] = use_out[bbid];
                use_in[bbid] -= use_kill[bbid];
                use_in[bbid] |= use_gen[bbid];

                if (!(bb->getBBType() & G4_BB_INIT_TYPE) && oldUseIn != use_in[bbid])
                {
                    changed = true;
                }
            }
        }
    } while (changed);
}

//
// Def analysis for each subroutine only
// at a call site, we do
// def_in[ret-BB] = def_out[call-BB] U def_out[callee exit-BB]
// callee's def_in/def_out is not modified
//
void LivenessAnalysis::defAnalysis(FuncInfo* subroutine)
{

    //def_in[bb] = null (inputs for entry BB)
    //def_out[bb] is initialized to all defs in the bb
    bool changed = false;
    do
    {
        changed = false;
        for (auto&& bb : subroutine->getBBList())
        {
            uint32_t bbid = bb->getId();
            std::optional<SparseBitSet> defInOrNull = std::nullopt;
            if (!changed)
            {
                defInOrNull = def_in[bbid];
            }
            auto phyPredBB = (bb == fg.getEntryBB()) ? nullptr : bb->getPhysicalPred();
            if (phyPredBB && (phyPredBB->getBBType() & G4_BB_CALL_TYPE))
            {
                // this is the return BB, we take the def_out of the callBB + the predecessors
                G4_BB* callBB = bb->getPhysicalPred();
                def_in[bbid] |= def_out[callBB->getId()];
                for (auto&& pred : bb->Preds)
                {
                    def_in[bbid] |= def_out[pred->getId()];
                }
            }
            else if (bb->getBBType() & G4_BB_INIT_TYPE)
            {
                // do nothing as we don't want to propagate caller defs yet
            }
            else
            {
                for (auto&& pred : bb->Preds)
                {
                    def_in[bbid] |= def_out[pred->getId()];
                }
            }

            if (!changed)
            {
                if (def_in[bbid] != defInOrNull.value())
                {
                    changed = true;
                }
            }
            def_out[bbid] |= def_in[bbid];
        }
    } while (changed);
}

void LivenessAnalysis::hierarchicalIPA(const SparseBitSet& kernelInput, const SparseBitSet& kernelOutput)
{

    assert (fg.sortedFuncTable.size() > 0 && "topological sort must already be performed");
    std::unordered_map<FuncInfo*, SparseBitSet> args;
    std::unordered_map<FuncInfo*, SparseBitSet> retVal;

#ifdef _DEBUG
    auto verifyFuncTable = [&]()
    {
        auto accountedBBs = 0;
        for (auto& sub : fg.sortedFuncTable)
        {
            accountedBBs += sub->getBBList().size();
        }
        MUST_BE_TRUE(fg.getBBList().size() == accountedBBs, "unaccounted bbs");
    };
    verifyFuncTable();
#endif

    auto initKernelLiveOut = [this, &kernelOutput]()
    {
        for (auto&& bb : fg.kernelInfo->getBBList())
        {
            if (bb->Succs.empty())
            {
                // EOT BB
                use_out[bb->getId()] = kernelOutput;
            }
        }
    };
    // reset all live-in/out sets except for the kernel live-out
    auto clearLiveSets = [this]()
    {
        for (auto subroutine : fg.sortedFuncTable)
        {
            for (auto bb : subroutine->getBBList())
            {
                use_in[bb->getId()].clear();
                use_out[bb->getId()].clear();
            }
        }
    };

    // top-down traversal to compute retval for each subroutine
    // retval[s] = live_out[s] - live_in[s],
    // where live_out[s] is the union of the live-in of the ret BB at each call site (hence top-down traversal).
    // this is not entirely accurate since we may have pass-through retVals
    // (e.g., A call B call C, C's retVal is pass-through in B and used in A, which
    //  means it won't be killed in B if we do top-down)
    // But for now let's trade some loss of accuracy to save one more round of fix-point
    initKernelLiveOut();
    for (auto FI = fg.sortedFuncTable.rbegin(), FE = fg.sortedFuncTable.rend(); FI != FE; ++FI)
    {
        auto subroutine = *FI;
        useAnalysis(subroutine);
        if (subroutine != fg.kernelInfo)
        {
            retVal[subroutine] = use_out[subroutine->getExitBB()->getId()];
            retVal[subroutine] -= use_in[subroutine->getInitBB()->getId()];
        }
        for (auto&& bb : subroutine->getBBList())
        {
            if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                G4_BB* retBB = bb->getPhysicalSucc();
                G4_BB* exitBB = bb->getCalleeInfo()->getExitBB();
                assert((exitBB->getBBType() & G4_BB_EXIT_TYPE) &&
                    "should be a subroutine's exit BB");
                use_out[exitBB->getId()] |= use_in[retBB->getId()];
            }
        }
    }

    // bottom-up traversal to compute arg for each subroutine
    // arg[s] = live-in[s], except retval of its callees are excluded as by definition they will not be live-in
    // The live-out of each subroutine is initialized to null so that
    // args are limited to variables actually used in this subroutine (and its callees)
    clearLiveSets();
    initKernelLiveOut();
    for (auto subroutine : fg.sortedFuncTable)
    {
        useAnalysisWithArgRetVal(subroutine, args, retVal);
        if (subroutine != fg.kernelInfo)
        {
            args[subroutine] = use_in[subroutine->getInitBB()->getId()];
            args[subroutine] -= use_out[subroutine->getExitBB()->getId()];
        }
    }

    // the real deal -- top-down traversal taking arg/retval/live-through all into consideration
    // again top-down traversal is needed to compute the live-out of each subroutine.
    clearLiveSets();
    initKernelLiveOut();
    for (auto FI = fg.sortedFuncTable.rbegin(), FE = fg.sortedFuncTable.rend(); FI != FE; ++FI)
    {
        auto subroutine = *FI;
        useAnalysisWithArgRetVal(subroutine, args, retVal);
        for (auto&& bb : subroutine->getBBList())
        {
            if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                G4_BB* retBB = bb->getPhysicalSucc();
                G4_BB* exitBB = bb->getCalleeInfo()->getExitBB();
                assert((exitBB->getBBType() & G4_BB_EXIT_TYPE) &&
                    "should be a subroutine's exit BB");
                use_out[exitBB->getId()] |= use_in[retBB->getId()];
            }
        }
    }

    maydefAnalysis();   // must be done before defAnalysis!

    // algorithm sketch for def-in/def-out:
    // In reverse topological order :
    //  -- Run def analysis on subroutine
    //  -- at each call site:
    //       def_in[ret-BB] |= def_out[call-BB] U def_out[exit-BB]
    // In topological order :
    //  -- At each call site:
    //       add def_out[call-BB] to all of callee's BBs
    def_in[fg.getEntryBB()->getId()] = kernelInput;
    for (auto subroutine : fg.sortedFuncTable)
    {
        defAnalysis(subroutine);
    }


    // FIXME: I assume we consider all caller's defs to be callee's defs too?
    for (auto FI = fg.sortedFuncTable.rbegin(), FE = fg.sortedFuncTable.rend(); FI != FE; ++FI)
    {
        auto subroutine = *FI;
        if (subroutine->getCallees().size() == 0)
        {
            continue;
        }
        for (auto&& bb : subroutine->getBBList())
        {
            if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                auto callee = bb->getCalleeInfo();
                for (auto&& calleeBB : callee->getBBList())
                {
                    def_in[calleeBB->getId()] |= def_out[bb->getId()];
                    def_out[calleeBB->getId()] |= def_out[bb->getId()];
                }
            }
        }
    }

#if 0
    std::vector<G4_Declare*> idToDecl;
    idToDecl.resize(numVarId);
    for (auto dcl : fg.getKernel()->Declares)
    {
        auto id = dcl->getRegVar()->getId();
        if (id < numVarId)
        {
            idToDecl[id] = dcl;
        }
    }
    for (auto subroutine : fg.sortedFuncTable)
    {
        auto printVal = [&idToDecl](const BitSet& bs)
        {
            for (int i = 0, size = (int)bs.getSize(); i < size; ++i)
            {
                if (bs.isSet(i))
                {
                    std::cerr << idToDecl[i]->getName() << "(" << i << ") ";
                }
            }
        };

        subroutine->dump();

        if (subroutine != fg.kernelInfo)
        {
            std::cerr << "\tArgs: ";
            printVal(args[subroutine->getId()]);
            std::cerr << "\n";

            std::cerr << "\tRetVal: ";
            printVal(retVal[subroutine->getId()]);
            std::cerr << "\n";
            std::cerr << "\tLiveThrough: ";
            BitSet liveThrough = use_in[subroutine->getInitBB()->getId()];
            liveThrough &= use_out[subroutine->getExitBB()->getId()];
            printVal(liveThrough);
            //std::cerr << "\n";
            //std::cerr << "\tDef in: ";
            //printVal(def_in[subroutine->getInitBB()->getId()]);
            //std::cerr << "\n";
            //std::cerr << "\tDef out: ";
            //printVal(def_out[subroutine->getInitBB()->getId()]);
            std::cerr << "\n";
        }
    }
#endif
 }

//
// determine if the dst writes the whole region of target declare
//
bool LivenessAnalysis::writeWholeRegion(const G4_BB* bb,
                                        const G4_INST* inst,
                                        G4_DstRegRegion* dst,
                                        const Options *opt) const
{
    unsigned execSize = inst->getExecSize();
    MUST_BE_TRUE(dst->getBase()->isRegVar(), ERROR_REGALLOC);

    if (!bb->isAllLaneActive() && !inst->isWriteEnableInst() &&
        fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D)
    {
        // conservatively assume non-nomask instructions in simd control flow
        // may not write the whole region
        return false;
    }

    if (inst->getPredicate())
    {
        return false;
    }

    if (inst->isFCall())
        return true;

    // Flags may be partially written when used as the destination
    // e.g., setp (M5_NM, 16) P11 V97(8,0)<0;1,0>
    // It can be only considered as a complete kill
    // if the computed bound diff matches with the number of flag elements
    if (dst->isFlag() == true)
    {
        if ((dst->getRightBound() - dst->getLeftBound() + 1) ==
            dst->getBase()->asRegVar()->getDeclare()->getNumberFlagElements())
        {
        return true;
    }
        else
        {
            return false;
        }
    }

    //
    // Find Primary Variable Declare
    //

    const G4_Declare* decl = ((const G4_RegVar*)dst->getBase())->getDeclare();
    const G4_Declare* primaryDcl = decl->getRootDeclare();

    //
    //  Cannot write whole register if
    //     * alias offset in non zero
    //     * reg or sub-reg offset is non zero
    //     * horiz stride is non zero
    //     * predicate is non null
    //
    if (decl->getAliasOffset() != 0 ||
        dst->getRegAccess() != Direct ||
        dst->getRegOff() != 0 ||
        dst->getSubRegOff() != 0 ||
        dst->getHorzStride() != 1 ||
        inst->isPartialWrite()) {
        return false;
    }

     //
    // For CISA3, pseudo-callee-save and pseudo-caller-save insts
    // are kills
    //
    if (fg.isPseudoDcl(primaryDcl))
    {
        return true;
    }

    //
    // If the region does not cover the whole declare then it does not write the whole region.
    //
    if (!primaryDcl->getRegVar()->isRegVarTransient() && (dst->getTypeSize() * execSize !=
        primaryDcl->getElemSize() * primaryDcl->getNumElems() * primaryDcl->getNumRows())) {
           return false;
    }

    return true;
}

//
// determine if the dst writes the whole region of target declare
//
bool LivenessAnalysis::writeWholeRegion(const G4_BB* bb,
                                        const G4_INST* inst,
                                        const G4_VarBase* flagReg) const
{
    if (!bb->isAllLaneActive() && !inst->isWriteEnableInst() && gra.kernel.getKernelType() != VISA_3D)
    {
        // conservatively assume non-nomask instructions in simd control flow
        // may not write the whole region
        return false;
    }

    const G4_Declare* decl = flagReg->asRegVar()->getDeclare();
    if (inst->getExecSize() != G4_ExecSize(decl->getNumberFlagElements()))
    {
        return false;
    }

    return true;
}

// Set bits in dst footprint based on dst region's left/right bound
void LivenessAnalysis::footprintDst(const G4_BB* bb,
    const G4_INST* i,
    G4_Operand* opnd,
    BitSet* dstfootprint) const
{
    if (dstfootprint &&
        !(i->isPartialWrite()) &&
        ((bb->isAllLaneActive() ||
            i->isWriteEnableInst() == true) ||
            gra.kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D))
    {
        // Bitwise OR left-bound/right-bound with dst footprint to indicate
        // bytes that are written in to
        opnd->updateFootPrint(*dstfootprint, true, *fg.builder);
    }
}

// Reset bits in srcfootprint based on src region's left/right bound
void LivenessAnalysis::footprintSrc(const G4_INST* i,
    G4_Operand *opnd,
    BitSet* srcfootprint)
{
    // Reset bits in kill map footprint
    opnd->updateFootPrint(*srcfootprint, false, i->getBuilder());
}

void LivenessAnalysis::computeGenKillandPseudoKill(G4_BB* bb,
                                                   SparseBitSet& def_out,
                                                   SparseBitSet& use_in,
                                                   SparseBitSet& use_gen,
                                                   SparseBitSet& use_kill) const
{
    //
    // Mark each fcall as using all globals and arg pre-defined var
    //
    if (bb->isEndWithFCall() && (selectedRF & G4_GRF))
    {
        const G4_Declare* arg = fg.builder->getStackCallArg();
        const G4_Declare* ret = fg.builder->getStackCallRet();

        const G4_FCALL* fcall = fg.builder->getFcallInfo(bb->back());
        MUST_BE_TRUE(fcall != NULL, "fcall info not found");

        if (arg->getByteSize() != 0)
        {
            // arg var is a use and a kill at each fcall
            if (fcall->getArgSize() != 0)
            {
                use_gen.set(arg->getRegVar()->getId(), true);
            }
            use_kill.set(arg->getRegVar()->getId(), true);
        }

        if (ret->getByteSize() != 0)
        {
            // ret var is a kill at each fcall
            use_kill.set(ret->getRegVar()->getId(), true);
            if (fcall->getRetSize() != 0)
            {
                def_out.set(ret->getRegVar()->getId(), true);
            }
        }
    }

    std::map<unsigned, BitSet> footprints;
    std::vector<std::pair<G4_Declare*, INST_LIST_RITER>> pseudoKills;

    for (INST_LIST::reverse_iterator rit = bb->rbegin(), rend = bb->rend(); rit != rend; ++rit)
    {
        G4_INST* i = (*rit);
        if (i->isLifeTimeEnd())
        {
            continue;
        }

        G4_DstRegRegion* dst = i->getDst();
        if (dst)
        {
            G4_DstRegRegion* dstrgn = dst;

            if (dstrgn->getBase()->isRegAllocPartaker())
            {
                G4_Declare* topdcl = GetTopDclFromRegRegion(dstrgn);
                unsigned id = topdcl->getRegVar()->getId();
                if (i->isPseudoKill())
                {
                    // Mark kill, reset gen
                    use_kill.set(id, true);
                    use_gen.set(id, false);

                    continue;
                }

                BitSet* dstfootprint = &footprints[id];

                if (dstfootprint->getSize() == 0)
                {
                    // Write for dst was not seen before, so insert in to map
                    // bitsetSize is in bytes
                    unsigned int bitsetSize = dstrgn->isFlag() ? topdcl->getNumberFlagElements() : topdcl->getByteSize();

                    BitSet newBitSet(bitsetSize, false);

                    auto it = neverDefinedRows.find(topdcl);
                    if (it != neverDefinedRows.end())
                    {
                        // Bitwise OR new bitset with never defined rows
                        newBitSet |= it->second;
                    }

                    footprints[id] = std::move(newBitSet);
                    if (gra.isBlockLocal(topdcl) &&
                        topdcl->getAddressed() == false &&
                        dstrgn->getRegAccess() == Direct)
                    {
                        // Local live ranges are never live-out of the only
                        // basic block they are defined. So in top-down order
                        // the first lexical definition is a kill irrespective
                        // of the footprint. In cases when local live-range
                        // def and use have h-stride != 1, the footprint at this
                        // lexically first definition will not have all bits set.
                        // This prevents that def to be seen as a kill. A simple
                        // solution to this is to set all bits when initializing
                        // the bitvector while iterating in bottom-up order. As
                        // we traverse further up uses will reset bits and defs
                        // will set bits. So when we encounter the lexically first
                        // def, we will be guaranteed to find all bits set, thus
                        // interpreting that def as a kill.
                        dstfootprint->setAll();
                    }
                }

                if (dstrgn->getRegAccess() == Direct)
                {
                    def_out.set(id, true);
                    //
                    // if the inst writes the whole region the var declared, we set use_kill
                    // so that use of var will not pass through (i.e., var's interval starts
                    // at this instruction.
                    //
                    if (writeWholeRegion(bb, i, dstrgn, fg.builder->getOptions()))
                    {
                        use_kill.set(id, true);
                        use_gen.set(id, false);

                        dstfootprint->setAll();
                    }
                    else
                    {
                        footprintDst(bb, i, dstrgn, dstfootprint);

                        use_gen.set(id, true);
                    }
                }
                else
                {
                    use_gen.set(id, true);
                }
            }
            else if ((selectedRF & G4_GRF) && dst->isIndirect())
            {
                // conservatively add each variable potentially accessed by dst to gen
                const REGVAR_VECTOR& pointsToSet = pointsToAnalysis.getAllInPointsToOrIndrUse(dst, bb);
                for (auto pt : pointsToSet)
                {
                    if (pt.var->isRegAllocPartaker())
                    {
                        use_gen.set(pt.var->getId(), true);
                    }
                }
            }
        }

        //
        // process each source operand
        //
        for (unsigned j = 0; j < G4_MAX_SRCS; j++)
        {
            G4_Operand* src = i->getSrc(j);

            if (!src)
            {
                continue;
            }
            if (src->isSrcRegRegion())
            {
                G4_Declare* topdcl = GetTopDclFromRegRegion(src);
                const G4_VarBase* base = (topdcl != nullptr ? topdcl->getRegVar() :
                    src->asSrcRegRegion()->getBase());
                if (base->isRegAllocPartaker())
                {
                    unsigned id = topdcl->getRegVar()->getId();
                    BitSet* srcfootprint = &footprints[id];

                    if (srcfootprint->getSize() != 0)
                    {
                        footprintSrc(i, src->asSrcRegRegion(), srcfootprint);
                    }
                    else
                    {
                        unsigned int bitsetSize = (src->asSrcRegRegion()->isFlag()) ? topdcl->getNumberFlagElements() : topdcl->getByteSize();

                        BitSet newBitSet(bitsetSize, false);

                        auto it = neverDefinedRows.find(topdcl);
                        if (it != neverDefinedRows.end())
                        {
                            // Bitwise OR new bitset with never defined rows
                            newBitSet |= it->second;
                        }

                        footprints[id] = std::move(newBitSet);
                        if (gra.isBlockLocal(topdcl) &&
                            topdcl->getAddressed() == false &&
                            (topdcl->getRegFile() == G4_ADDRESS ||
                                src->asSrcRegRegion()->getRegAccess() == Direct))
                        {
                            srcfootprint->setAll();
                        }
                        footprintSrc(i, src->asSrcRegRegion(), srcfootprint);
                    }

                    use_gen.set(static_cast<const G4_RegVar*>(base)->getId(), true);
                }

                if ((selectedRF & G4_GRF) && src->getRegAccess() == IndirGRF)
                {
                    int idx = 0;
                    G4_RegVar* grf;
                    G4_Declare* topdcl = GetTopDclFromRegRegion(src);

                    while ((grf = pointsToAnalysis.getPointsTo(topdcl->getRegVar(), idx++)) != NULL)
                    {
                        // grf is a variable that src potentially points to
                        // since we dont know exactly which part of grf is sourced
                        // assume entire grf is sourced
                        // Also add grf to the gen set as it may be potentially used
                        unsigned int id = grf->getId();
                        use_gen.set(id, true);
                        BitSet* srcfootprint = &footprints[id];

                        if (srcfootprint->getSize() != 0)
                        {
                            srcfootprint->clear();

                            DEBUG_VERBOSE("Found potential indirect use of " << grf->getDeclare()->getName() <<
                                " so resetting its footprint" << std::endl);
                        }
                    }
                }
            }
            //
            // treat the addr expr as both a use and a partial def
            //
            else if (src->isAddrExp())
            {
                G4_RegVar* reg = static_cast<G4_AddrExp*>(src)->getRegVar();
                if (reg->isRegAllocPartaker() &&  !reg->isRegVarTmp())
                {
                    unsigned srcId = reg->getId();
                    use_gen.set(srcId, true);
                    def_out.set(srcId, true);
                }
            }
        }

        //
        // Process condMod
        //
        G4_CondMod* mod = i->getCondMod();
        if (mod) {
            G4_VarBase *flagReg = mod->getBase();
            if (flagReg)
            {
                if (flagReg->asRegVar()->isRegAllocPartaker())
                {
                    G4_Declare* topdcl = flagReg->asRegVar()->getDeclare();
                    MUST_BE_TRUE(topdcl->getAliasDeclare() == nullptr, "Invalid alias flag decl.");
                    unsigned id = topdcl->getRegVar()->getId();

                    BitSet* dstfootprint = &footprints[id];

                    if (dstfootprint->getSize() == 0)
                    {
                        // Write for dst was not seen before, so insert in to map
                        // bitsetSize is in bits for flag
                        unsigned int bitsetSize = topdcl->getNumberFlagElements();

                        BitSet newBitSet(bitsetSize, false);
                        footprints[id] = std::move(newBitSet);

                        if (gra.isBlockLocal(topdcl))
                        {
                            dstfootprint->setAll();
                        }
                    }

                    def_out.set(id, true);

                    if (writeWholeRegion(bb, i, flagReg))
                    {
                        use_kill.set(id, true);
                        use_gen.set(id, false);

                        dstfootprint->setAll();
                    }
                    else
                    {
                        footprintDst(bb, i, mod, dstfootprint);
                        use_gen.set(id, true);
                    }
                }
            }
            else
            {
                MUST_BE_TRUE((i->opcode() == G4_sel ||
                    i->opcode() == G4_csel) &&
                    i->getCondMod() != nullptr,
                    "Invalid CondMod");
            }
        }

        //
        // Process predicate
        //
        G4_Predicate* predicate = i->getPredicate();
        if (predicate) {
            G4_VarBase *flagReg = predicate->getBase();
            MUST_BE_TRUE(flagReg->asRegVar()->getDeclare()->getAliasDeclare() == nullptr, "Invalid alias flag decl.");
            if (flagReg->asRegVar()->isRegAllocPartaker())
            {
                const G4_Declare* topdcl = flagReg->asRegVar()->getDeclare();
                unsigned id = topdcl->getRegVar()->getId();
                auto srcfootprint = &footprints[id];

                if (srcfootprint->getSize() != 0)
                {
                    footprintSrc(i, predicate, srcfootprint);
                }
                else
                {
                    unsigned int bitsetSize = topdcl->getNumberFlagElements();

                    BitSet newBitSet(bitsetSize, false);
                    footprints[id] = std::move(newBitSet);
                    if (gra.isBlockLocal(topdcl))
                    {
                        srcfootprint->setAll();
                    }
                    footprintSrc(i, predicate, srcfootprint);
                }

                use_gen.set(static_cast<const G4_RegVar*>(flagReg)->getId(), true);
            }
        }

        //
        // Check whether dst can be killed at this point
        // A block of code is said to kill a variable when union
        // of all partial writes causes all elements to be written
        // into and any reads in the block can be sourced from
        // writes within that block itself
        //
        if (dst && dst->getBase()->isRegAllocPartaker())
        {
            G4_Declare* topdcl = GetTopDclFromRegRegion(dst);
            unsigned id = topdcl->getRegVar()->getId();
            auto dstfootprint = &footprints[id];

            if (dstfootprint->getSize() != 0)
            {
                // Found dst in map
                // Check whether all bits set
                // pseudo_kill for this dst was not found in this BB yet
                unsigned int first;
                LocalLiveRange* topdclLR = nullptr;

                if ((dstfootprint->isAllset() ||
                    // Check whether local RA marked this range
                    (topdcl &&
                    (topdclLR = gra.getLocalLR(topdcl)) &&
                    topdclLR->isLiveRangeLocal() &&
                    (!topdcl->isInput()) &&
                    topdclLR->getFirstRef(first) == i)) &&
                    // If single inst writes whole region then dont insert pseudo_kill
                    writeWholeRegion(bb, i, dst, fg.builder->getOptions()) == false)
                {
                    bool foundKill = false;
                    INST_LIST::reverse_iterator nextIt = rit;
                    ++nextIt;
                    if (nextIt != bb->rend())
                    {
                        const G4_INST* nextInst = (*nextIt);
                        if (nextInst->isPseudoKill())
                        {
                            G4_DstRegRegion* nextDst = nextInst->getDst();

                            if (nextDst != NULL &&
                                nextDst->isDstRegRegion() &&
                                nextDst->getBase()->isRegAllocPartaker() &&
                                topdcl == GetTopDclFromRegRegion(nextDst))
                            {
                                foundKill = true;
                            }
                        }
                    }
                    if (!foundKill)
                    {
                        // All bytes of dst written at this point, so this is a good place to insert
                        // pseudo kill inst
                        pseudoKills.emplace_back(topdcl, rit);
                    }

                    // Reset gen
                    use_gen.set(dst->getBase()->asRegVar()->getId(), false);

                    // Set kill
                    use_kill.set(dst->getBase()->asRegVar()->getId(), true);
#ifdef DEBUG_VERBOSE_ON
                    DEBUG_VERBOSE("Found kill at inst ");
                    INST_LIST_ITER fwdIter = rit.base();
                    fwdIter--;
                    (*fwdIter)->emit_inst(std::cout, false, NULL);
                    DEBUG_VERBOSE(" // $" << (*fwdIter)->getCISAOff());
                    DEBUG_VERBOSE(std::endl);
#endif
                }
            }
        }

        if (mod && mod->getBase() && mod->getBase()->asRegVar()->isRegAllocPartaker())
        {
            G4_VarBase *flagReg = mod->getBase();
            G4_Declare* topdcl = flagReg->asRegVar()->getDeclare();
            unsigned id = topdcl->getRegVar()->getId();
            auto dstfootprint = &footprints[id];

            if (dstfootprint->getSize() != 0)
            {
                unsigned int first;
                const LocalLiveRange* topdclLR = nullptr;
                if ((dstfootprint->isAllset() ||
                    // Check whether local RA marked this range
                    // This may not be necessary as currently local RA is not performed for flags.
                    (topdcl &&
                    (topdclLR = gra.getLocalLR(topdcl)) &&
                        topdclLR->isLiveRangeLocal() &&
                        topdclLR->getFirstRef(first) == i)) &&
                        // If single inst writes whole region then dont insert pseudo_kill
                        writeWholeRegion(bb, i, flagReg) == false)
                {
                    // All bytes of dst written at this point, so this is a good place to insert
                    // pseudo kill inst
                    pseudoKills.emplace_back(topdcl, rit);

                    // Reset gen
                    use_gen.set(flagReg->asRegVar()->getId(), false);

                    // Set kill
                    use_kill.set(flagReg->asRegVar()->getId(), true);
#ifdef DEBUG_VERBOSE_ON
                    DEBUG_VERBOSE("Found kill at inst ");
                    INST_LIST_ITER fwdIter = rit.base();
                    fwdIter--;
                    (*fwdIter)->emit_inst(std::cout, false, NULL);
                    DEBUG_VERBOSE(" // $" << (*fwdIter)->getCISAOff());
                    DEBUG_VERBOSE(std::endl);
#endif
                }
            }
        }
    }

    //
    // Insert pseudo_kill nodes in BB
    //
    for (auto&& pseudoKill : pseudoKills)
    {
        INST_LIST_ITER iterToInsert = pseudoKill.second.base();
        do
        {
            --iterToInsert;
        } while ((*iterToInsert)->isPseudoKill());
        G4_INST* killInst = fg.builder->createPseudoKill(pseudoKill.first, PseudoKillType::FromLiveness);
        bb->insertBefore(iterToInsert, killInst);
    }

    //
    // initialize use_in
    //
    use_in = use_gen;
}

//
// use_out = use_in(s1) + use_in(s2) + ... where s1 s2 ... are the successors of bb
// use_in  = use_gen + (use_out - use_kill)
//
bool LivenessAnalysis::contextFreeUseAnalyze(G4_BB* bb, bool isChanged)
{
    bool changed;

    unsigned bbid = bb->getId();

    if (bb->Succs.empty()) // exit block
    {
        changed = false;
    }
    else if (isChanged)
    {
        // no need to update changed. This saves a memcpy
        for (auto succBB : bb->Succs)
        {
            use_out[bbid] |= use_in[succBB->getId()];
        }
        changed = true;
    }
    else
    {
        SparseBitSet old = use_out[bbid];
        for (auto succBB : bb->Succs)
        {
            use_out[bbid] |= use_in[succBB->getId()];
        }

        changed = (old != use_out[bbid]);
    }

    //
    // in = gen + (out - kill)
    //
    use_in[bbid] = use_out[bbid];
    use_in[bbid] -= use_kill[bbid];
    use_in[bbid] |= use_gen[bbid];

    return changed;
}

//
// def_in = def_out(p1) + def_out(p2) + ... where p1 p2 ... are the predecessors of bb
// def_out |= def_in
//
bool LivenessAnalysis::contextFreeDefAnalyze(G4_BB* bb, bool isChanged)
{
    bool changed  = false;
    unsigned bbid = bb->getId();

    if (bb->Preds.empty())
    {
        changed = false;
    }
    else if (isChanged)
    {
        // no need to update changed. This saves a memcpy
        for (auto predBB : bb->Preds)
        {
            def_in[bbid] |= def_out[predBB->getId()];
        }
        changed = true;
    }
    else
    {
        SparseBitSet old = def_in[bbid];
        for (auto predBB : bb->Preds)
        {
            def_in[bbid] |= def_out[predBB->getId()];
        }
        changed = (old != def_in[bbid]);
    }

     def_out[bb->getId()] |= def_in[bb->getId()];

     return changed;
}

void LivenessAnalysis::dump_bb_vector(char* vname, std::vector<BitSet>& vec)
{
    std::cerr << vname << "\n";
    for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++)
    {
        G4_BB* bb = (*it);
        std::cerr << "    BB" << bb->getId() << "\n";
        const BitSet& in = vec[bb->getId()];
        std::cerr << "        ";
        for (unsigned i = 0; i < in.getSize(); i+= 10)
        {
            //
            // dump 10 bits a group
            //
            for (unsigned j = i; j < in.getSize() && j < i+10; j++)
            {
                std::cerr << (in.isSet(j) ? "1" : "0");
            }
            std::cerr << " ";
        }
        std::cerr << "\n";
    }
}

void LivenessAnalysis::dump_fn_vector(char* vname, std::vector<FuncInfo*>& fns, std::vector<BitSet>& vec)
{
    DEBUG_VERBOSE(vname << std::endl);
    for (std::vector<FuncInfo*>::iterator it = fns.begin(); it != fns.end(); it++)
    {
        FuncInfo* funcInfo = (*it);

        DEBUG_VERBOSE("    FN" << funcInfo->getId() << std::endl);
        const BitSet& in = vec[funcInfo->getId()];
        DEBUG_VERBOSE("        ");
        for (unsigned i = 0; i < in.getSize(); i += 10)
        {
            //
            // dump 10 bits a group
            //
            for (unsigned j = i; j < in.getSize() && j < i + 10; j++)
            {
                DEBUG_VERBOSE(in.isSet(j) ? "1" : "0");
            }
            DEBUG_VERBOSE(" ");
        }
        DEBUG_VERBOSE(std::endl);
    }
}

//
// dump which vars are live at the entry of BB
//
void LivenessAnalysis::dump() const
{
    for (auto bb : fg)
    {
        std::cerr << "BB" << bb->getId() << "'s live in: ";
        unsigned total_size = 0;
        auto dumpVar = [&total_size](G4_RegVar* var)
        {
            int size = var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
            std::cerr << var->getName() << "(" << size << "), ";
            total_size += size;
        };

        unsigned count = 0;
        for (auto var : vars)
        {
            if (var->isRegAllocPartaker() && isLiveAtEntry(bb, var->getId()))
            {
                if (count++ % 10 == 0) std::cerr << "\n";
                dumpVar(var);
            }
        }
        std::cerr << "\nBB" << bb->getId() << "'s live in size: " << total_size / fg.builder->numEltPerGRF<Type_UB>() << "\n\n";
        std::cerr << "BB" << bb->getId() << "'s live out: ";
        total_size = 0;
        count = 0;
        for (auto var : vars)
        {
            if (var->isRegAllocPartaker() && isLiveAtExit(bb, var->getId()))
            {
                if (count++ % 10 == 0) std::cerr << "\n";
                dumpVar(var);
            }
        }
        std::cerr << "\nBB" << bb->getId() << "'s live out size: " << total_size / fg.builder->numEltPerGRF<Type_UB>()<< "\n\n";
    }
}

void LivenessAnalysis::dumpBB(G4_BB *bb) const
{
    std::cerr << "\n\nBB" << bb->getId() << "'s live in: ";
    unsigned total_size = 0;
    auto dumpVar = [&total_size](G4_RegVar* var)
    {
        int size = var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
        std::cerr << var->getName() << "(" << size << ")" << "[" << var->getRegAllocPartaker() << "], ";
        total_size += size;
    };

    unsigned count = 0;
    for (auto var : vars)
    {
        if (var->isRegAllocPartaker() && isLiveAtEntry(bb, var->getId()))
        {
            if (count++ % 10 == 0) std::cerr << "\n";
            dumpVar(var);
        }
    }
    std::cerr << "\n\nBB" << bb->getId() << "'s live out: ";
    total_size = 0;
    count = 0;
    for (auto var : vars)
    {
        if (var->isRegAllocPartaker() && isLiveAtExit(bb, var->getId()))
        {
            if (count++ % 10 == 0) std::cerr << "\n";
            dumpVar(var);
        }
    }
    std::cerr << "\n\nBB" << bb->getId() << "'s use through: ";
    total_size = 0;
    count = 0;
    for (auto var : vars)
    {
        if (var->isRegAllocPartaker() && isUseThrough(bb, var->getId()))
        {
            if (count++ % 10 == 0) std::cerr << "\n";
            dumpVar(var);
        }
    }
    std::cerr << "\n\nBB" << bb->getId() << "'s def through: ";
    total_size = 0;
    count = 0;
    for (auto var : vars)
    {
        if (var->isRegAllocPartaker() && isDefThrough(bb, var->getId()))
        {
            if (count++ % 10 == 0) std::cerr << "\n";
            dumpVar(var);
        }
    }
}

void LivenessAnalysis::dumpLive(BitSet& live) const
{
    auto dumpVar = [](G4_RegVar* var)
    {
        int size = var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
        std::cerr << var->getName() << "(" << size << ")" << "[" << var->getRegAllocPartaker() << "], ";
    };

    unsigned count = 0;
    for (auto var : vars)
    {
        if (live.isSet(var->getId()))
        {
            if (count++ % 10 == 0) std::cerr << "\n";
            dumpVar(var);
        }
    }
}

//
// dump which vars are live at the entry of BB
//
void LivenessAnalysis::dumpGlobalVarNum() const
{
    SparseBitSet global_def_out(numVarId);
    SparseBitSet global_use_in(numVarId);

    for (auto bb : fg)
    {
        SparseBitSet global_in = use_in[bb->getId()];
        SparseBitSet global_out = def_out[bb->getId()];
        global_in &= def_in[bb->getId()];
        global_use_in |= global_in;
        global_out &= use_out[bb->getId()];
        global_def_out |= global_out;
    }

    int global_var_num = 0;
    for (auto var : vars)
    {
        if (var->isRegAllocPartaker())
        {
            if (global_use_in.isSet(var->getId()) || global_def_out.isSet(var->getId()))
            {
                global_var_num ++;
            }
        }
    }
    std::cerr << "total var num: " << numVarId << " global var num: " << global_var_num << "\n";
}

void LivenessAnalysis::reportUndefinedUses() const
{
    auto dumpVar = [](G4_RegVar* var)
    {
        int size = var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
        std::cerr << var->getName() << "(" << size << "), ";
    };

    std::cerr << "\nPossible undefined uses in kernel " << fg.getKernel()->getName() << ":\n";
    unsigned count = 0;
    for (auto var : vars)
    {
        // Skip if the var is not involved in RA.
        if (!var->isRegAllocPartaker())
            continue;
        // Skip if the var is a AddrSpillLoc.
        if (var->isRegVarAddrSpillLoc())
            continue;
        // Skip if the var is not in use_in of BB0
        if (!isUseIn(fg.getEntryBB(), var->getId()))
            continue;
        // Skip if the var is in def_in of BB0
        if (def_in[fg.getEntryBB()->getId()].isSet(var->getId()))
            continue;

        if (count++ % 10 == 0) std::cerr << "\n";
            dumpVar(var);
    }
    std::cerr << "\n";
}

bool LivenessAnalysis::isEmptyLiveness() const
{
    return numBBId == 0;
}

//
// return true if var is live at the entry of bb
// check both use_in and def_in, if one condition fails then var is not in the live range
//
bool LivenessAnalysis::isLiveAtEntry(const G4_BB* bb, unsigned var_id) const
{
    return use_in[bb->getId()].isSet(var_id) && def_in[bb->getId()].isSet(var_id);
}
//
// return true if var is live at the exit of bb
//
bool LivenessAnalysis::isLiveAtExit(const G4_BB* bb, unsigned var_id) const
{
    return use_out[bb->getId()].isSet(var_id) && def_out[bb->getId()].isSet(var_id);
}

//
// return true if var is user through the bb
//
bool LivenessAnalysis::isUseOut(const G4_BB* bb, unsigned var_id) const
{
    return use_out[bb->getId()].isSet(var_id);
}

//
// return true if var is user through the bb
//
bool LivenessAnalysis::isUseIn(const G4_BB* bb, unsigned var_id) const
{
    return use_in[bb->getId()].isSet(var_id);
}

//
// return true if var is user through the bb
//
bool LivenessAnalysis::isUseThrough(const G4_BB* bb, unsigned var_id) const
{
    return use_in[bb->getId()].isSet(var_id) && use_out[bb->getId()].isSet(var_id);
}
//
// return true if var is live at the exit of bb
//
bool LivenessAnalysis::isDefThrough(const G4_BB* bb, unsigned var_id) const
{
    return def_in[bb->getId()].isSet(var_id) && def_out[bb->getId()].isSet(var_id);
}


void GlobalRA::markBlockLocalVar(G4_RegVar* var, unsigned bbId)
{
    G4_Declare* dcl = var->getDeclare()->getRootDeclare();

    if (dcl->isInput() || dcl->isOutput())
    {
        setBBId(dcl, UINT_MAX - 1);
    }
    else
    {
        if (getBBId(dcl) == bbId)
        {
            // Do nothing.
        }
        else if (getBBId(dcl) == UINT_MAX)
        {
            setBBId(dcl, bbId);
        }
        else {
            setBBId(dcl, UINT_MAX - 1);
        }
    }
}

void GlobalRA::markBlockLocalVars()
{
    for (auto bb : kernel.fg)
    {
        for (std::list<G4_INST*>::iterator it = bb->begin(); it != bb->end(); it++)
        {
            G4_INST* inst = *it;

            // Track direct dst references.

            G4_DstRegRegion* dst = inst->getDst();

            if (dst != NULL)
            {
                G4_DstRegRegion* dstRgn = dst->asDstRegRegion();

                if (dstRgn->getBase()->isRegVar()) {
                    markBlockLocalVar(dstRgn->getBase()->asRegVar(), bb->getId());

                    G4_Declare* topdcl = GetTopDclFromRegRegion(dst);
                    if (topdcl)
                    {
                        if (inst->isSend())
                        {
                            topdcl->setIsRefInSendDcl(true);
                        }

                        LocalLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);
                        unsigned int startIdx;
                        if (lr->getFirstRef(startIdx) == NULL)
                        {
                            lr->setFirstRef(inst, 0);
                        }
                        lr->recordRef(bb);
                        recordRef(topdcl);
                    }
                }
            }

            G4_CondMod* condMod = inst->getCondMod();

            if (condMod != NULL &&
                condMod->getBase() != NULL)
            {
                if (condMod->getBase() && condMod->getBase()->isRegVar())
                {
                    markBlockLocalVar(condMod->getBase()->asRegVar(), bb->getId());

                    G4_Declare* topdcl = condMod->getBase()->asRegVar()->getDeclare();
                    if (topdcl)
                    {
                        LocalLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);
                        unsigned int startIdx;
                        if (lr->getFirstRef(startIdx) == NULL)
                        {
                            lr->setFirstRef(inst, 0);
                        }
                        lr->recordRef(bb);
                        recordRef(topdcl);
                    }
                }
            }

            // Track direct src references.
            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* src = inst->getSrc(j);

                if (src == NULL)
                {
                    // Do nothing.
                }
                else if (src->isSrcRegRegion() && src->asSrcRegRegion()->getBase()->isRegVar())
                {
                    G4_SrcRegRegion* srcRgn = src->asSrcRegRegion();

                    if (srcRgn->getBase()->isRegVar()) {
                        markBlockLocalVar(src->asSrcRegRegion()->getBase()->asRegVar(), bb->getId());

                        G4_Declare* topdcl = GetTopDclFromRegRegion(src);
                        if (topdcl)
                        {
                            if (inst->isSend())
                            {
                                topdcl->setIsRefInSendDcl(true);
                            }

                            LocalLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);

                            lr->recordRef(bb);
                            recordRef(topdcl);
                            if (inst->isEOT())
                            {
                                lr->markEOT();
                            }
                        }
                    }
                }
                else if (src->isAddrExp())
                {
                    G4_RegVar* addExpVar = src->asAddrExp()->getRegVar();
                    markBlockLocalVar(addExpVar, bb->getId());

                    G4_Declare* topdcl = addExpVar->getDeclare()->getRootDeclare();
                    MUST_BE_TRUE(topdcl != NULL, "Top dcl was null for addr exp opnd");

                    LocalLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);
                    lr->recordRef(bb);
                    lr->markIndirectRef();
                    recordRef(topdcl);
                }
            }

            G4_Operand* pred = inst->getPredicate();

            if (pred != NULL)
            {
                if (pred->getBase() && pred->getBase()->isRegVar())
                {
                    markBlockLocalVar(pred->getBase()->asRegVar(), bb->getId());
                    G4_Declare* topdcl = pred->getBase()->asRegVar()->getDeclare();
                    if (topdcl)
                    {
                        LocalLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);
                        lr->recordRef(bb);
                        recordRef(topdcl);
                    }
                }
            }

            // Track all indirect references.
            const REGVAR_VECTOR& grfVec = pointsToAnalysis.getIndrUseVectorForBB(bb->getId());
            for (pointInfo grf : grfVec)
            {
                markBlockLocalVar(grf.var, bb->getId());
            }
        }
    }
}

void GlobalRA::resetGlobalRAStates()
{
    if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA))
    {
        // remove partial decls
        auto isPartialDcl = [](G4_Declare* dcl)
        {
            return dcl->getIsPartialDcl();
        };

        kernel.Declares.erase(
            std::remove_if (kernel.Declares.begin(), kernel.Declares.end(), isPartialDcl),
            kernel.Declares.end());
    }

    for (auto dcl : kernel.Declares)
    {
        //Reset all the local live ranges
        resetLocalLR(dcl);

        if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA))
        {
            //Remove the split declares
            if (dcl->getIsSplittedDcl())
            {
                dcl->setIsSplittedDcl(false);
                clearSubDcl(dcl);
            }
        }

        //Remove the bank assignment
        if (builder.getOption(vISA_LocalBankConflictReduction) &&
            builder.hasBankCollision())
        {
            setBankConflict(dcl, BANK_CONFLICT_NONE);
        }
        clearBundleConflictDcl(dcl);
    }

    return;
}

//
// Mark block local (temporary) variables.
//
void GlobalRA::markGraphBlockLocalVars()
{
    // Clear stale LocalLiveRange* first to avoid double ref counting
    clearStaleLiveRanges();

    //Create live ranges and record the reference info
    markBlockLocalVars();

#ifdef DEBUG_VERBOSE_ON
    std::cout << "\t--LOCAL VARIABLES--\n";
    for (auto dcl : kernel.Declares)
    {
        LocalLiveRange* topdclLR = getLocalLR(dcl);

        if (topdclLR != nullptr &&
            topdclLR->isLiveRangeLocal())
        {
            std::cout << dcl->getName() << ",\t";
        }
    }
    std::cout << "\n";
#endif
}

//
// Pre-assign phy regs to stack call function return variable as per ABI.
//
void FlowGraph::setABIForStackCallFunctionCalls()
{
    // For each G4_pseudo_fcall inst, create dst of GRF type
    // with physical register 1.0 pre-assigned to it.
    // Similarly, for G4_pseudo_fret create src of GRF type
    // with physical register 1.0 pre-assigned to it.
    // Each will use 2 dwords of r1.0.
    int call_id = 0, ret_id = 0;

    for (auto bb : *this)
    {
        if (bb->isEndWithFCall())
        {
            const char* n = builder->getNameString(mem, 25, "FCALL_RET_LOC_%d", call_id++);

            G4_INST* fcall = bb->back();
            // Set call dst to r125.0
            G4_Declare* r1_dst = builder->createDeclareNoLookup(n, G4_GRF, builder->numEltPerGRF<Type_UD>(), 1, Type_UD);
            r1_dst->getRegVar()->setPhyReg(builder->phyregpool.getGreg(builder->kernel.getFPSPGRF()), IR_Builder::SubRegs_Stackcall::Ret_IP);
            G4_DstRegRegion* dstRgn = builder->createDst(r1_dst->getRegVar(), 0, 0, 1, Type_UD);
            fcall->setDest(dstRgn);
        }

        if (bb->isEndWithFRet())
        {
            const char* n = builder->getNameString(mem, 25, "FRET_RET_LOC_%d", ret_id++);
            G4_INST* fret = bb->back();
            const RegionDesc* rd = builder->createRegionDesc(2, 2, 1);
            G4_Declare* r1_src = builder->createDeclareNoLookup(n, G4_INPUT, builder->numEltPerGRF<Type_UD>(), 1, Type_UD);
            r1_src->getRegVar()->setPhyReg(builder->phyregpool.getGreg(builder->kernel.getFPSPGRF()), IR_Builder::SubRegs_Stackcall::Ret_IP);
            G4_Operand* srcRgn = builder->createSrc(r1_src->getRegVar(), 0, 0, rd, Type_UD);
            fret->setSrc(srcRgn, 0);
            if (fret->getExecSize() == g4::SIMD1)
            {
                // due to <2;2,1> regioning we must update exec size as well
                fret->setExecSize(g4::SIMD2);
            }
            if (builder->getOption(vISA_GenerateDebugInfo))
            {
                pKernel->getKernelDebugInfo()->setFretVar(GetTopDclFromRegRegion(fret->getSrc(0)));
            }
        }
    }
}

// Function to verify RA results
void GlobalRA::verifyRA(LivenessAnalysis & liveAnalysis)
{
    for (auto bb : kernel.fg)
    {
        // Verify PREG assignment
        for (auto inst : *bb)
        {
            G4_DstRegRegion* dst = inst->getDst();
            if (dst != NULL &&
                dst->getBase()->isRegAllocPartaker())
            {
                MUST_BE_TRUE(dst->getBase()->asRegVar()->getPhyReg(), "RA verification error: No PREG assigned for variable " << GetTopDclFromRegRegion(dst)->getName() << "!");
            }

            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* src = inst->getSrc(j);
                if (src != NULL &&
                    src->isSrcRegRegion() &&
                    src->asSrcRegRegion()->getBase()->isRegAllocPartaker())
                {
                    MUST_BE_TRUE(src->asSrcRegRegion()->getBase()->asRegVar()->getPhyReg(),
                        "RA verification error: No PREG assigned for variable " << GetTopDclFromRegRegion(src->asSrcRegRegion())->getName() << "!");
                }
            }
        }

        int numGRF = kernel.getNumRegTotal();
        // Verify Live-in
        std::map<uint32_t, G4_Declare*> LiveInRegMap;
        std::map<uint32_t, G4_Declare*>::iterator LiveInRegMapIt;
        std::vector<uint32_t> liveInRegVec(numGRF * kernel.numEltPerGRF<Type_UW>(), UINT_MAX);

        for (G4_Declare* dcl : kernel.Declares)
        {
            if (dcl->getAliasDeclare() != nullptr)
                continue;

            if (dcl->getRegVar()->isRegAllocPartaker())
            {
                G4_RegVar* var = dcl->getRegVar();
                uint32_t varID = var->getId();
                if (liveAnalysis.isLiveAtEntry(bb, dcl->getRegVar()->getId()))
                {
                    MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid preg assignment for variable " << dcl->getName() << "!");

                    uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                    uint32_t regOff = var->getPhyRegOff();

                    uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                        (regOff * dcl->getElemSize()) / G4_WSIZE;
                    for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                    {
                        LiveInRegMapIt = LiveInRegMap.find(idx);
                        if (liveInRegVec[idx] != UINT_MAX)
                        {
                            MUST_BE_TRUE(LiveInRegMapIt != LiveInRegMap.end(), "RA verification error: Invalid entry in LiveInRegMap!");
                            if (dcl->isInput())
                            {
                                DEBUG_MSG("RA verification warning: Found conflicting input variables: " << dcl->getName()
                                    << " and " << (*LiveInRegMapIt).second->getName() << " assigned to r" << regNum
                                    << "." << regOff << "!\n");
                                liveInRegVec[idx] = varID;
                                LiveInRegMapIt->second = dcl;
                            }
                            else
                            {
                                DEBUG_MSG("RA verification warning: Found conflicting live-in variables: " << dcl->getName()
                                    << " and " << LiveInRegMapIt->second->getName() << " assigned to r" <<
                                    regNum << "." << regOff << "!\n");
                            }

                        }
                        else
                        {
                            liveInRegVec[idx] = varID;
                            MUST_BE_TRUE(LiveInRegMapIt == LiveInRegMap.end(), "RA verification error: Invalid entry in LiveInRegMap!");
                            LiveInRegMap.emplace(idx, dcl);
                        }
                    }
                }
            }
        }

        // Verify Live-out
        G4_Declare *ret = kernel.fg.builder->getStackCallRet();
        std::map<uint32_t, G4_Declare*> liveOutRegMap;
        std::map<uint32_t, G4_Declare*>::iterator liveOutRegMapIt;
        std::vector<uint32_t> liveOutRegVec(numGRF * kernel.numEltPerGRF<Type_UW>(), UINT_MAX);

        for (G4_Declare* dcl : kernel.Declares)
        {
            if (dcl->getAliasDeclare() != NULL)
                continue;
            if (dcl->getRegVar()->isRegAllocPartaker())
            {
                G4_RegVar* var = dcl->getRegVar();
                uint32_t varID = var->getId();
                if (liveAnalysis.isLiveAtExit(bb, varID))
                {
                    MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid preg assignment for variable " << dcl->getName() << "!");

                    uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                    uint32_t regOff = var->getPhyRegOff();

                    uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                        (regOff * dcl->getElemSize()) / G4_WSIZE;
                    for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                    {
                        liveOutRegMapIt = liveOutRegMap.find(idx);
                        if (liveOutRegVec[idx] != UINT_MAX)
                        {
                            MUST_BE_TRUE(liveOutRegMapIt != liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                            if (dcl->isInput())
                            {
                                DEBUG_MSG("RA verification warning: Found conflicting input variables: " << dcl->getName()
                                    << " and " << liveOutRegMapIt->second->getName() << " assigned to r" << regNum
                                    << "." << regOff << "!\n");
                                liveOutRegVec[idx] = varID;
                                liveOutRegMapIt->second = dcl;
                            }
                            else
                            {
                                DEBUG_MSG("RA verification warning: Found conflicting live-out variables: " << dcl->getName()
                                    << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                    regNum << "." << regOff << "!\n");
                            }

                        }
                        else
                        {
                            liveOutRegVec[idx] = varID;
                            MUST_BE_TRUE(liveOutRegMapIt == liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                            liveOutRegMap.emplace(idx, dcl);
                        }
                    }
                }
            }
        }

        for (INST_LIST::reverse_iterator rit = bb->rbegin(); rit != bb->rend(); ++rit)
        {
            G4_INST* inst = (*rit);
            INST_LIST_RITER ritNext = rit;
            ritNext++;
            G4_INST* rNInst = nullptr;
            if (ritNext != bb->rend())
            {
                rNInst = (*ritNext);
            }

            if (inst->isPseudoKill())
            {
                continue;
            }
            G4_DstRegRegion* dst = inst->getDst();
            G4_DstRegRegion* rNDst = nullptr;

            if (rNInst && rNInst->isPseudoKill())
            {
                rNDst = rNInst->getDst();
            }

            //
            // verify dst operand
            //
            if (dst != NULL)
            {
                if (dst->getBase()->isRegAllocPartaker())
                {
                    G4_DstRegRegion* dstrgn = dst;
                    G4_RegVar* var = dstrgn->getBase()->asRegVar();
                    uint32_t varID = var->getId();
                    G4_Declare* dcl = GetTopDclFromRegRegion(dstrgn);
                    G4_Declare* rNDcl = nullptr;
                    if (rNDst != nullptr)
                    {
                        rNDcl = GetTopDclFromRegRegion(rNDst);
                    }
                    MUST_BE_TRUE(dcl != nullptr, "Null declare found");
                    var = dcl->getRegVar();

                    MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
                    MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

                    uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                    uint32_t regOff = var->getPhyRegOff();

                    uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                        (regOff * dcl->getElemSize()) / G4_WSIZE;
                    for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                    {
                        liveOutRegMapIt = liveOutRegMap.find(idx);
                        if (liveOutRegVec[idx] == UINT_MAX)
                        {
                            if (!inst->isPseudoKill())
                            {
                                MUST_BE_TRUE(liveOutRegMapIt == liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                DEBUG_MSG("RA verification warning: Found unused variable " << dcl->getName() << "!\n");
                            }
                        }
                        else
                        {
                            MUST_BE_TRUE(liveOutRegMapIt != liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                            if (liveOutRegVec[idx] != varID)
                            {
                                const SparseBitSet& indr_use = liveAnalysis.indr_use[bb->getId()];

                                if (strstr(dcl->getName(), GlobalRA::StackCallStr) != NULL)
                                {
                                    DEBUG_MSG("RA verification warning: Found conflicting stackCall variable: " << dcl->getName()
                                        << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                        regNum << "." << regOff << "!\n");
                                }
                                else if (indr_use.isSet(liveOutRegVec[idx]) == true)
                                {
                                    MUST_BE_TRUE(false, "RA verification warning: Found conflicting indirect variables: " << dcl->getName()
                                        << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                        regNum << "." << regOff << "!\n");
                                }
                                else
                                {
                                    if (!inst->isPseudoKill())
                                    {
                                        DEBUG_MSG("RA verification error: Found conflicting variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                }
                            }

                            if (liveAnalysis.writeWholeRegion(bb, inst, dstrgn, kernel.getOptions()) ||
                                inst->isPseudoKill() || rNDcl == dcl) {
                                liveOutRegVec[idx] = UINT_MAX;
                                MUST_BE_TRUE(liveOutRegMapIt != liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                liveOutRegMap.erase(liveOutRegMapIt);
                            }
                        }
                    }
                }
                else if (dst->getRegAccess() == IndirGRF)
                {
                    G4_DstRegRegion* dstrgn = dst;
                    G4_Declare* addrdcl = GetTopDclFromRegRegion(dstrgn);
                    G4_RegVar* ptvar = NULL;
                    int vid = 0;

                    while ((ptvar = pointsToAnalysis.getPointsTo(addrdcl->getRegVar(), vid++)) != NULL)
                    {
                        uint32_t varID = ptvar->getId();
                        G4_Declare* dcl = ptvar->getDeclare();
                        MUST_BE_TRUE(dcl != nullptr, "Null declare found");
                        while (dcl->getAliasDeclare())
                        {
                            dcl = dcl->getAliasDeclare();
                        }
                        G4_RegVar* var = dcl->getRegVar();

                        MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
                        MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

                        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                        uint32_t regOff = var->getPhyRegOff();

                        uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                            (regOff * dcl->getElemSize()) / G4_WSIZE;
                        for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                        {
                            liveOutRegMapIt = liveOutRegMap.find(idx);
                            if (liveOutRegVec[idx] == UINT_MAX)
                            {
                                MUST_BE_TRUE(liveOutRegMapIt == liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                DEBUG_MSG("RA verification warning: Found unused variable " << dcl->getName() << "!\n");
                            }
                            else
                            {
                                MUST_BE_TRUE(liveOutRegMapIt != liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                if (liveOutRegVec[idx] != varID)
                                {
                                    const SparseBitSet& indr_use = liveAnalysis.indr_use[bb->getId()];

                                    if (strstr(dcl->getName(), GlobalRA::StackCallStr) != NULL)
                                    {
                                        DEBUG_MSG("RA verification warning: Found conflicting stackCall variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                    else if (indr_use.isSet(liveOutRegVec[idx]) == true)
                                    {
                                        MUST_BE_TRUE(false, "RA verification warning: Found conflicting indirect variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                    else
                                    {
                                        MUST_BE_TRUE(false, "RA verification error: Found conflicting variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (inst->opcode() == G4_pseudo_fcall)
            {
                if (ret != NULL && ret->getRegVar() != NULL)
                {
                    G4_RegVar* var = ret->getRegVar();
                    uint32_t varID = var->getId();
                    MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
                    MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

                    uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                    uint32_t regOff = var->getPhyRegOff();

                    uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                        regOff * ret->getElemSize() / G4_WSIZE;
                    for (uint32_t i = 0; i < ret->getWordSize(); ++i, ++idx)
                    {
                        liveOutRegMapIt = liveOutRegMap.find(idx);
                        liveOutRegVec[idx] = UINT_MAX;
                        MUST_BE_TRUE(liveOutRegMapIt != liveOutRegMap.end(),
                            "RA verification error: Invalid entry in liveOutRegMap!");
                        liveOutRegMap.erase(liveOutRegMapIt);
                    }
                }
            }

            //
            // verify each source operand
            //
            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* src = inst->getSrc(j);
                if (src == NULL)
                {
                    continue;
                }
                if (src->isAddrExp() && src->asAddrExp()->isRegAllocPartaker())
                {
                    G4_RegVar* var = src->asAddrExp()->getRegVar();
                    uint32_t varID = UINT_MAX;
                    G4_Declare* dcl = NULL;

                    varID = var->getId();
                    dcl = var->getDeclare();

                    uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                    uint32_t regOff = var->getPhyRegOff();

                    uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                        (regOff * dcl->getElemSize()) / G4_WSIZE;
                    for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                    {
                        liveOutRegMapIt = liveOutRegMap.find(idx);
                        if (liveOutRegVec[idx] == UINT_MAX)
                        {
                            liveOutRegVec[idx] = varID;
                            MUST_BE_TRUE(liveOutRegMapIt == liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                            liveOutRegMap.emplace(idx, dcl);
                        }
                        else
                        {
                            if (liveOutRegVec[idx] != varID)
                            {
                                const SparseBitSet& indr_use = liveAnalysis.indr_use[bb->getId()];

                                if (dcl->isInput())
                                {
                                    DEBUG_MSG("RA verification warning: Found conflicting input variables: " << dcl->getName()
                                        << " and " << liveOutRegMapIt->second->getName() << " assigned to r" << regNum
                                        << "." << regOff << "!\n");
                                    liveOutRegVec[idx] = varID;
                                    liveOutRegMapIt->second = dcl;
                                }
                                else if (strstr(dcl->getName(), GlobalRA::StackCallStr) != NULL)
                                {
                                    DEBUG_MSG("RA verification warning: Found conflicting stackCall variables: " << dcl->getName()
                                        << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                        regNum << "." << regOff << "!\n");
                                }
                                else if (indr_use.isSet(liveOutRegVec[idx]) == true)
                                {
                                    MUST_BE_TRUE(false, "RA verification warning: Found conflicting indirect variables: " << dcl->getName()
                                        << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                        regNum << "." << regOff << "!\n");
                                }
                                else
                                {
                                    INST_LIST::reverse_iterator succ = rit;
                                    ++succ;
                                    bool idMismatch = false;
                                    G4_Declare* topdcl = GetTopDclFromRegRegion((*succ)->getDst());
                                    if (topdcl != nullptr &&
                                        liveOutRegVec[idx] != topdcl->getRegVar()->getId())
                                    {
                                        idMismatch = true;
                                    }
                                    if (succ == bb->rend() ||
                                        !(*succ)->isPseudoKill()  ||
                                        (*succ)->getDst() == NULL ||
                                        idMismatch)
                                    {
                                        MUST_BE_TRUE(false, "RA verification error: Found conflicting variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                }
                            }
                        }
                    }
                }
                else if (src->isSrcRegRegion() && src->asSrcRegRegion()->getBase()->isRegAllocPartaker())
                {
                    G4_SrcRegRegion* srcrgn = src->asSrcRegRegion();
                    G4_RegVar* var = srcrgn->getBase()->asRegVar();
                    uint32_t varID = var->getId();
                    G4_Declare* dcl = GetTopDclFromRegRegion(srcrgn);
                    var = dcl->getRegVar();
                    MUST_BE_TRUE(var->getId() == varID, "RA verification error: Invalid regVar ID!");
                    MUST_BE_TRUE(var->getPhyReg()->isGreg(), "RA verification error: Invalid dst reg!");

                    if (!inst->isLifeTimeEnd())
                    {
                        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                        uint32_t regOff = var->getPhyRegOff();

                        uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                            (regOff * dcl->getElemSize()) / G4_WSIZE;
                        for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                        {
                            liveOutRegMapIt = liveOutRegMap.find(idx);
                            if (liveOutRegVec[idx] == UINT_MAX)
                            {
                                liveOutRegVec[idx] = varID;
                                MUST_BE_TRUE(liveOutRegMapIt == liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                liveOutRegMap.emplace(idx, dcl);
                            }
                            else
                            {
                                if (liveOutRegVec[idx] != varID)
                                {
                                    const SparseBitSet& indr_use = liveAnalysis.indr_use[bb->getId()];

                                    if (dcl->isInput())
                                    {
                                        DEBUG_MSG("RA verification warning: Found conflicting input variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" << regNum
                                            << "." << regOff << "!\n");
                                        liveOutRegVec[idx] = varID;
                                        liveOutRegMapIt->second = dcl;
                                    }
                                    else if (strstr(dcl->getName(), GlobalRA::StackCallStr) != NULL)
                                    {
                                        DEBUG_MSG("RA verification warning: Found conflicting stackCall variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                    else if (indr_use.isSet(liveOutRegVec[idx]) == true)
                                    {
                                        MUST_BE_TRUE(false, "RA verification warning: Found conflicting indirect variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                    else
                                    {
                                        INST_LIST::reverse_iterator succ = rit;
                                        ++succ;
                                        bool idMismatch = false;
                                        G4_Declare* topdcl = GetTopDclFromRegRegion((*succ)->getDst());
                                        if (topdcl != nullptr &&
                                            liveOutRegVec[idx] != topdcl->getRegVar()->getId())
                                        {
                                            idMismatch = true;
                                        }
                                        if (succ == bb->rbegin() ||
                                            !(*succ)->isPseudoKill() ||
                                            (*succ)->getDst() == NULL ||
                                            idMismatch)
                                        {
                                            DEBUG_MSG("RA verification error: Found conflicting variables: " << dcl->getName()
                                                << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                                regNum << "." << regOff << "!\n");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                        uint32_t regOff = var->getPhyRegOff();

                        uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                            (regOff * dcl->getElemSize()) / G4_WSIZE;
                        for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                        {
                            if (liveOutRegVec[idx] != UINT_MAX)
                            {
                                liveOutRegMapIt = liveOutRegMap.find(idx);
                                MUST_BE_TRUE(liveOutRegMapIt != liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                MUST_BE_TRUE(false, "RA verification error: Found live variable: " << dcl->getName()
                                    << " after lifetime_end " << " assigned to r" << regNum << "." << regOff << "!\n");
                            }
                        }
                    }

                    // verify EOT source
                    if (inst->isEOT() && kernel.fg.builder->hasEOTGRFBinding())
                    {
                        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                        MUST_BE_TRUE(regNum >= 112,
                            "RA verification error: EOT source: " << dcl->getName()
                            << " is assigned to r." << regNum << "\n");
                    }
                }
                else if (src->isSrcRegRegion() && src->getRegAccess() == IndirGRF)
                {
                    G4_SrcRegRegion* srcrgn = src->asSrcRegRegion();
                    G4_Declare* addrdcl = GetTopDclFromRegRegion(srcrgn);
                    G4_RegVar* ptvar = NULL;
                    int vid = 0;

                    while ((ptvar = pointsToAnalysis.getPointsTo(addrdcl->getRegVar(), vid++)) != NULL)
                    {
                        uint32_t varID = ptvar->getId();
                        G4_Declare* dcl = ptvar->getDeclare()->getRootDeclare();
                        G4_RegVar* var = dcl->getRegVar();

                        uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
                        uint32_t regOff = var->getPhyRegOff();

                        uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                            (regOff * dcl->getElemSize()) / G4_WSIZE;
                        for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx)
                        {
                            liveOutRegMapIt = liveOutRegMap.find(idx);
                            if (liveOutRegVec[idx] == UINT_MAX)
                            {
                                liveOutRegVec[idx] = varID;
                                MUST_BE_TRUE(liveOutRegMapIt == liveOutRegMap.end(), "RA verification error: Invalid entry in liveOutRegMap!");
                                liveOutRegMap.emplace(idx, dcl);
                            }
                            else
                            {
                                if (liveOutRegVec[idx] != varID)
                                {
                                    const SparseBitSet& indr_use = liveAnalysis.indr_use[bb->getId()];

                                    if (dcl->isInput())
                                    {
                                        DEBUG_MSG("RA verification warning: Found conflicting input variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" << regNum
                                            << "." << regOff << "!\n");
                                        liveOutRegVec[idx] = varID;
                                        liveOutRegMapIt->second = dcl;
                                    }
                                    else if (indr_use.isSet(liveOutRegVec[idx]) == true)
                                    {
                                        MUST_BE_TRUE(false, "RA verification warning: Found conflicting indirect variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                    else
                                    {
                                        MUST_BE_TRUE(false, "RA verification error: Found conflicting variables: " << dcl->getName()
                                            << " and " << liveOutRegMapIt->second->getName() << " assigned to r" <<
                                            regNum << "." << regOff << "!\n");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static void recordRAStats(IR_Builder& builder,
                          G4_Kernel& kernel,
                          int RAStatus)
{
#if COMPILER_STATS_ENABLE
    CompilerStats &Stats = builder.getcompilerStats();
    int SimdSize = kernel.getSimdSize();
    if (RAStatus == VISA_SUCCESS)
    {
        Stats.SetFlag("IsRAsuccessful", SimdSize);
        switch (kernel.getRAType())
        {
        case RA_Type::TRIVIAL_BC_RA:
        case RA_Type::TRIVIAL_RA:
            Stats.SetFlag("IsTrivialRA", SimdSize);
            break;
        case RA_Type::LOCAL_ROUND_ROBIN_BC_RA:
        case RA_Type::LOCAL_ROUND_ROBIN_RA:
        case RA_Type::LOCAL_FIRST_FIT_BC_RA:
        case RA_Type::LOCAL_FIRST_FIT_RA:
            Stats.SetFlag("IsLocalRA", SimdSize);
            break;
        case RA_Type::HYBRID_BC_RA:
        case RA_Type::HYBRID_RA:
            Stats.SetFlag("IsHybridRA", SimdSize);
            break;
        case RA_Type::GRAPH_COLORING_RR_RA:
        case RA_Type::GRAPH_COLORING_FF_RA:
        case RA_Type::GRAPH_COLORING_RR_BC_RA:
        case RA_Type::GRAPH_COLORING_FF_BC_RA:
        case RA_Type::GRAPH_COLORING_SPILL_RR_RA:
        case RA_Type::GRAPH_COLORING_SPILL_FF_RA:
        case RA_Type::GRAPH_COLORING_SPILL_RR_BC_RA:
        case RA_Type::GRAPH_COLORING_SPILL_FF_BC_RA:
        case RA_Type::GLOBAL_LINEAR_SCAN_RA:
        case RA_Type::GLOBAL_LINEAR_SCAN_BC_RA:
            Stats.SetFlag("IsGlobalRA", SimdSize);
            break;
        case RA_Type::UNKNOWN_RA:
            break;
        default:
            assert(0 && "Incorrect RA type");
        }
    }
#endif // COMPILER_STATS_ENABLE
}

static void replaceSSO(G4_Kernel& kernel)
{
    // Invoke function only for XeHP_SDV and later
    // Replace SSO with r126.7 (scratch reg)

    auto dst = kernel.fg.builder->createDst(
        kernel.fg.getScratchRegDcl()->getRegVar(), 0, 7, 1, Type_UD);
    for (auto bb : kernel.fg)
    {
        for (auto instIt = bb->begin(); instIt != bb->end(); instIt++)
        {
            auto inst = (*instIt);
            if (inst->getDst() &&
                inst->getDst()->getTopDcl() == kernel.fg.builder->getSpillSurfaceOffset())
            {
                if (kernel.fg.getIsStackCallFunc())
                {
                    instIt = bb->erase(instIt);
                    --instIt;
                }
                else
                    inst->setDest(dst);

                // if an earlier pass inserted pseudokill for SSO dcl, remove it
                // but our final target is the instruction actually defining SSO.
                if (inst->isPseudoKill())
                    continue;

                // Also update scratch msg dcl to be an alias
                kernel.fg.builder->getSpillSurfaceOffset()->setAliasDeclare(
                    kernel.fg.getScratchRegDcl(), 7 * TypeSize(Type_UD));

                return;
            }
        }
    }
}


int regAlloc(IR_Builder& builder, PhyRegPool& regPool, G4_Kernel& kernel)
{
    kernel.fg.callerSaveAreaOffset = kernel.fg.calleeSaveAreaOffset = kernel.fg.frameSizeInOWord = 0;

    // This must be done before Points-to analysis as it may modify CFG and add new BB!
    if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
    {
        kernel.fg.setABIForStackCallFunctionCalls();
        kernel.fg.addFrameSetupDeclares(builder, regPool);
        kernel.fg.normalizeFlowGraph();
        if (builder.getPlatform() >= Xe_XeHPSDV)
            replaceSSO(kernel);
    }

    if (kernel.getOption(vISA_DoSplitOnSpill))
    {
        // loop computation is done here because we may need to add
        // new preheader BBs. later parts of RA assume no change
        // to CFG structure.
        kernel.fg.getLoops().computePreheaders();
    }

    kernel.fg.reassignBlockIDs();
    // do it once for whole RA pass. Assumption is RA should not modify CFG at all
    kernel.fg.setPhysicalPredSucc();

    if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D)
    {
        kernel.fg.findNaturalLoops();

#ifdef DEBUG_VERBOSE_ON
        for (auto backedge : kernel.fg.backEdges)
        {
            DEBUG_VERBOSE("\n===> Found natural loop: ");
            for (auto block : kernel.fg.naturalLoops[backedge])
            {
                DEBUG_VERBOSE("\tBB" << block->getId());
            }
            DEBUG_VERBOSE(std::endl);
        }
#endif
    }

    //
    // Perform flow-insensitive points-to-analysis.
    //
    PointsToAnalysis pointsToAnalysis(kernel.Declares, kernel.fg.getNumBB());
    pointsToAnalysis.doPointsToAnalysis(kernel.fg);
    GlobalRA gra(kernel, regPool, pointsToAnalysis);

    //
    // insert pseudo save/restore return address so that reg alloc
    // can assign registers to hold the return addresses
    //
    gra.assignLocForReturnAddr();

    //FIXME: here is a temp WA
    bool hybridWithSpill = builder.getOption(vISA_HybridRAWithSpill) && !(kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc());
    if (kernel.fg.funcInfoTable.size() > 0 &&
        kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D && !hybridWithSpill)
    {
        kernel.getOptions()->setOption(vISAOptions::vISA_LocalRA, false);
    }

    //
    // Mark block local variables for the whole graph prior to performing liveness analysis.
    // 1. Is required for flag/address register allocation
    // 2. We must make sure the reference number, reference BB(which will be identified in local RA as well) happens only one time.
       // Otherwise, there will be correctness issue
    gra.markGraphBlockLocalVars();

    //Remove the un-referenced declares
    gra.removeUnreferencedDcls();

    if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM)
    {
        kernel.fg.markScope();
    }

    //
    // perform graph coloring for whole program
    //

    if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
    {
        kernel.fg.addSaveRestorePseudoDeclares(builder);
        gra.fixSrc0IndirFcall();
    }

    int status = gra.coloringRegAlloc();

    if (auto jitInfo = builder.getJitInfo())
    {
        jitInfo->numBytesScratchGtpin = kernel.getGTPinData()->getNumBytesScratchUse();
    }

    if (!gra.isReRAPass())
    {
        // propagate address takens to gtpin info
        const auto& addrTakenMap = pointsToAnalysis.getPointsToMap();
        auto gtpinData = kernel.getGTPinData();
        for (auto& indirRef : addrTakenMap)
        {
            for (auto target : indirRef.second)
                gtpinData->addIndirRef(indirRef.first, target);
        }
    }

    recordRAStats(builder, kernel, status);
    if (status != VISA_SUCCESS)
    {
        return status;
    }

    if (auto sp = kernel.getVarSplitPass())
    {
        sp->replaceIntrinsics();
    }
    if (builder.getOption(vISA_VerifyRA))
    {
        LivenessAnalysis liveAnalysis(gra,
            G4_GRF | G4_INPUT, true);
        liveAnalysis.computeLiveness();
        gra.verifyRA(liveAnalysis);
    }

    // printf("EU Fusion WA insts for func: %s\n", kernel.getName());
    for (auto inst : gra.getEUFusionCallWAInsts())
    {
        kernel.setMaskOffset(inst, InstOpt_M16);
        // inst->dump();
    }

#if defined(_DEBUG)
    for (auto inst : gra.getEUFusionNoMaskWAInsts())
    {
        if (inst->getPredicate() != nullptr ||
            inst->getCondMod() != nullptr)
        {
            assert(false && "Don't expect either predicate nor condmod for WA insts!");
        }
    }
#endif

    return status;
}

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

#include <vector>
#include "Mem_Manager.h"
#include "FlowGraph.h"
#include "BuildIR.h"
#include "SpillCode.h"

#ifdef _DEBUG
#define _DEBUG_SPILL
#endif

using namespace vISA;

//
// create a declare to hold the spill value of var
//
G4_Declare* SpillManager::createNewSpillLocDeclare(G4_Declare* dcl)
{

    if (dcl->getRegFile() == G4_FLAG)
    {
        MUST_BE_TRUE(dcl->getElemType() == Type_UW || dcl->getElemType() == Type_W, "flag reg's type should be UW");
        MUST_BE_TRUE(dcl->getNumElems() <= builder.getNumFlagRegisters(), "Flag reg Spill size exceeds limit");
    }
    else
    {
        // if we are dealing with type other than UW, e.g., B, then we need to
        // take care different data type reg moves of spill code. For now, just
        // assume data types of addr reg are UW
        //
        G4_Type type = dcl->getElemType();
        MUST_BE_TRUE(type == Type_UW ||
                     type == Type_W ||
                     type == Type_UD ||
                     type == Type_D, "addr reg's type should be UW or UD");
        MUST_BE_TRUE(dcl->getNumElems() <= getNumAddrRegisters(), "Addr reg Spill size exceeds 16 bytes");
    }

    G4_Declare* sp = dcl->getSpilledDeclare();
    if (sp == NULL) // not yet created
    {
        sp = builder.createAddrFlagSpillLoc(dcl);
        gra.setBBId(sp, bbId);
    }

    return sp;
}

//
// replicate dcl for temporary use (loading value from SPILL location)
//
G4_Declare* SpillManager::createNewTempAddrDeclare(G4_Declare* dcl)
{
    const char* name = builder.getNameString(builder.mem, 16, "Temp_ADDR_%d", tempDclId++);

    MUST_BE_TRUE(dcl->getElemType() == Type_UW || dcl->getElemType() == Type_W, "addr reg's type should be UW");
    MUST_BE_TRUE(dcl->getNumRows() == 1, "Temp_ADDR should be only 1 row");
    MUST_BE_TRUE(dcl->getNumElems() <= getNumAddrRegisters(), "Temp_ADDR exceeds 16 bytes");
    G4_Declare* sp = builder.createDeclareNoLookup(name,
        G4_ADDRESS,
        dcl->getNumElems(),
        1, // 1 row
        Type_UW);
    gra.setBBId(sp, bbId);
    // Live range of new temp addrs is short so that there is no point spilling them.
    // indicate this is for newly created addr temp so that RA won't spill it
    // in subsequent RA allocation
    gra.addAddrFlagSpillDcl(sp);

    return sp;
}

G4_Declare* SpillManager::createNewTempFlagDeclare(G4_Declare* dcl)
{
    const char* name = builder.getNameString(builder.mem, 32, "Temp_FSPILL_%d", tempDclId++);

    assert(dcl->getRegFile() == G4_FLAG && "dcl should be a flag");
    G4_Declare* sp = builder.createFlag(dcl->getNumberFlagElements(), name);
    gra.setBBId(sp, bbId);
    sp->copyAlign(dcl);
    gra.copyAlignment(sp, dcl);
    gra.addAddrFlagSpillDcl(sp);

    return sp;
}

//
// replicate dcl for temporary use (loading value from SPILL location)
//
G4_Declare* SpillManager::createNewTempAddrDeclare(G4_Declare* dcl, uint16_t num_reg)
{
    const char* name = builder.getNameString(builder.mem, 16, "Temp_ADDR_%d", tempDclId++);

    G4_Type type = dcl->getElemType();
    MUST_BE_TRUE(type == Type_UW ||
                 type == Type_W ||
                 type == Type_UD ||
                 type == Type_D, "addr reg's type should be UW or UD");
    MUST_BE_TRUE(dcl->getNumRows() == 1, "Temp_ADDR should be only 1 row");
    MUST_BE_TRUE(dcl->getNumElems() <= getNumAddrRegisters(), "Temp_ADDR exceeds 16 bytes");
    G4_Declare* sp = builder.createDeclareNoLookup( name,
                                            G4_ADDRESS,
                                            num_reg,
                                            1, // 1 row
                                            type);
    gra.setBBId(sp, bbId);
    // Live range of new temp addrs is short so that there is no point spilling them.
    // indicate this is for newly created addr temp so that RA won't spill it
    // in subsequent RA allocation
    gra.addAddrFlagSpillDcl(sp);

    return sp;
}

//
// generate a reg to reg mov inst for addr/flag spill
// mov  dst(dRegOff,dSubRegOff)<1>  src(sRegOff,sSubRegOff)<nRegs;nRegs,1>
//
void SpillManager::genRegMov(G4_BB* bb,
                             INST_LIST_ITER it,
                             G4_VarBase*    src,
                             unsigned short sSubRegOff,
                             G4_VarBase*    dst,
                             unsigned       nRegs,
                             bool           useNoMask = true)
{
    builder.instList.clear();

    uint16_t dSubRegOff = 0;
    for (uint16_t i = 16; i != 0 && nRegs != 0; i >>= 1)  // 16, 8, 4, 2, 1
    {
        if (nRegs >= i)
        {
            //
            // create loc(0,locOff)<nRegs;nRegs,1> operand
            //
            /*
                Flag registers should always be scalar regions
            */
            G4_Type type = Type_W;
            const RegionDesc* srcRgn = NULL;
            unsigned execSize = i;
            if(src->isFlag() || dst->isFlag())
            {

                type = Type_UW;
                if(i == 2)
                {
                    type = Type_UD;
                    execSize = 1;
                }
                else if(i > 2)
                {
                    ASSERT_USER(false, "unsupported flag width");
                }

                srcRgn = builder.getRegionScalar();
            }
            else
            {
                srcRgn = (i== 1) ? builder.getRegionScalar() : builder.getRegionStride1();
            }

            G4_SrcRegRegion* s = builder.createSrcRegRegion(Mod_src_undef,
                    Direct,
                    src,
                    0,
                    sSubRegOff,
                    srcRgn,
                    type);
            //
            // create a0.aOff<1>
            //
            G4_DstRegRegion* d = builder.createDst(
                                dst,
                                0,
                                dSubRegOff,
                                1,
                                type);

            if (execSize != kernel.getSimdSize())
            {
                // NoMask must be used in this case
                useNoMask = true;
            }
            // mov (nRegs)  a0.aOff<1>  loc(0,locOff)<4;4,1>
            builder.createMov((uint8_t) execSize, d, s,
                useNoMask ? InstOpt_WriteEnable : InstOpt_NoOpt, true);

            sSubRegOff += i;
            dSubRegOff += i;

            nRegs -= i;
        }
    }
    MUST_BE_TRUE(nRegs == 0, ERROR_SPILLCODE);

    //
    // insert newly created insts from builder to instList
    //
    bb->splice(it,builder.instList);
}
//
// check if dst is spilled & insert spill code
//
void SpillManager::replaceSpilledDst(G4_BB* bb,
                                     INST_LIST_ITER it, // where new insts will be inserted
                                     G4_INST*       inst,
                                     PointsToAnalysis& pointsToAnalysis,
                                     G4_Operand ** operands_analyzed,
                                     G4_Declare ** declares_created)
{
    G4_DstRegRegion* dst = inst->getDst();
    if (dst == NULL)
        return;

    if (dst->getBase()->isRegAllocPartaker() &&
        dst->getBase()->asRegVar()->getDeclare()->getSpilledDeclare() != NULL) // spilled base
    {
        // create a dst region with spill loc
        // original dst region  V100_uw(0,0)<1>:uw ==>
        // new dst region SP_uw(0,0)<1>:uw
        G4_Declare* spDcl = dst->getBase()->asRegVar()->getDeclare()->getSpilledDeclare();
        if (dst->getRegAccess() == Direct)
        {

            G4_DstRegRegion rgn(*dst, spDcl->getRegVar()); // using spDcl as new base
            if( rgn.getHorzStride() == UNDEFINED_SHORT &&
                dst->isFlag() )
            {
                // Flag as destination has undefined hstride
                // For replacing it with spill range, make hstride 1
                rgn.setHorzStride( 1 );
            }
            G4_DstRegRegion* d = builder.createDstRegRegion(rgn);
            inst->setDest(d);
        }
        else if (dst->getRegAccess() == IndirGRF)
        {
            // add (1)  r[V100_uw(0,0),0]<1>:f V124_f(0,0)<0;1,0>:f  1
            // indirect access' base must be addr reg so we need to create a temp addr live range
            // to load value from V100's spill loc
            // e.g.,   mov (1) T_uw(0,0)<1>:uw SPILL_LOC_V100_uw(0,0)<0;1,0>:uw
            //         add (1)  r[T_uw(0,0),0]<1>:f V124_f(0,0)<0;1,0>:f   1
            //
            // create declare for temp addr live range
            //
            G4_Declare* tmpDcl = NULL;
            bool match_found = false;

            for(unsigned int j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_SrcRegRegion* analyzed_src = (G4_SrcRegRegion*) operands_analyzed[j];
                if( analyzed_src != NULL &&
                    analyzed_src->getBase()->asRegVar()->getDeclare() == dst->getBase()->asRegVar()->getDeclare() &&
                    analyzed_src->getSubRegOff() == dst->getSubRegOff() &&
                    !analyzed_src->getRegion()->isRegionWH() )
                {
                    tmpDcl = declares_created[j];
                    match_found = true;
                }
            }

            if( !match_found )
            {
                tmpDcl = createNewTempAddrDeclare(spDcl);
                //
                // generate mov Tmp(0,0)<1>  SPILL_LOC_V100(0,0)
                //
                genRegMov(bb, it,
                    spDcl->getRegVar(), 0,
                    tmpDcl->getRegVar(),
                    tmpDcl->getNumElems());
            }

            G4_DstRegRegion rgn(*dst, tmpDcl->getRegVar()); // using tmpDcl as new base
            G4_DstRegRegion* d = match_found ? builder.createDstWithNewSubRegOff(&rgn, 0) : builder.createDstRegRegion(rgn);
            inst->setDest(d);

            if (!match_found)
            {
                pointsToAnalysis.insertAndMergeFilledAddr(dst->getBase()->asRegVar(), tmpDcl->getRegVar());
            }
        }
        else
            MUST_BE_TRUE(false, "Unknown reg access");
    }
}
//
// check if src is spilled and insert spill code to load spilled value
//
void SpillManager::replaceSpilledSrc(G4_BB* bb,
                                     INST_LIST_ITER it, // where new insts will be inserted
                                     G4_INST*       inst,
                                     unsigned       i,
                                     PointsToAnalysis& pointsToAnalysis,
                                     G4_Operand ** operands_analyzed,
                                     G4_Declare ** declares_created)
{
    G4_Operand* src = inst->getSrc(i);
    if (src == NULL)
        return;
    //
    // go ahead replace src (addr reg) with spill GRF
    //

    if (src->isSrcRegRegion() &&
        src->asSrcRegRegion()->getBase()->isRegAllocPartaker() &&
        src->asSrcRegRegion()->getBase()->asRegVar()->getDeclare()->getSpilledDeclare() != NULL) // spilled base
    {
        // create a src region with spill loc
        // original src region  V100_uw(0,0)<0;1,0>:uw
        // new src region SP_uw(0,0)<0;1,0>:uw
        G4_SrcRegRegion* ss = src->asSrcRegRegion();
        G4_Declare* spDcl = ss->getBase()->asRegVar()->getDeclare()->getSpilledDeclare();
        if (ss->getRegAccess() == Direct)
        {
            G4_SrcRegRegion* s = NULL;
            if (inst->isSplitSend() && i == 3)
            {
                G4_Declare* tmpDcl = createNewTempAddrDeclare(spDcl, 1);
                tmpDcl->setSubRegAlign(Four_Word);
                gra.setSubRegAlign(tmpDcl, Four_Word);
                // (W) mov (1) tmpDcl<1>:ud spDcl<0;1,0>:ud
                auto movSrc = builder.Create_Src_Opnd_From_Dcl(spDcl, builder.getRegionScalar());
                auto movDst = builder.Create_Dst_Opnd_From_Dcl(tmpDcl, 1);
                G4_INST* movInst = builder.createMov(1, movDst, movSrc, InstOpt_WriteEnable, false);
                bb->insertBefore(it, movInst);

                s = builder.createSrcRegRegion(
                    Mod_src_undef,
                    Direct,
                    tmpDcl->getRegVar(),
                    0,
                    0,
                    ss->getRegion(),
                    spDcl->getElemType());
                inst->setSrc(s, i);
            }
            else
            {
                s = builder.createSrcWithNewBase(ss, spDcl->getRegVar()); // using spDcl as new base
            }
            inst->setSrc(s,i);
        }
        else if (ss->getRegAccess() == IndirGRF)
        {
            // add (2)  V124_f(0,0)<1>:f  r[V100_uw(0,0),0]<4;2,2>:f 1
            // indirect access' base must be addr reg so we need to create a temp addr live range
            // to load value from V100's spill loc
            // e.g.,   mov (1) T(0,0)<1>:uw SPILL_LOC_V100_uw(0,0)<0;1,0>:uw
            //         add (2)  V124_f(0,0)<1>:f  r[T(0,0),0]<4;2,2>:f 1
            //
            // create declare for temp addr live range
            //

            uint16_t num_reg = 1;
            //if access is VxH copy number of addresses based on execution size of instruction
            if (ss->getRegion()->isRegionWH())
            {
                num_reg = inst->getExecSize();
            }

            G4_Declare* tmpDcl = NULL;
            bool match_found = false;

            for (unsigned int j = 0; j < i; j++)
            {
                G4_SrcRegRegion* analyzed_src = (G4_SrcRegRegion*)operands_analyzed[j];
                if (analyzed_src != NULL &&
                    analyzed_src->getBase()->asRegVar()->getDeclare() == ss->getBase()->asRegVar()->getDeclare() &&
                    analyzed_src->getSubRegOff() == ss->getSubRegOff())
                {
                    tmpDcl = declares_created[j];
                    match_found = true;
                }
            }

            if (!match_found)
            {
                tmpDcl = createNewTempAddrDeclare(spDcl, num_reg);
                operands_analyzed[i] = ss;
                declares_created[i] = tmpDcl;

                //
                // generate mov Tmp(0,0)<1>  SPILL_LOC_V100(0,0)
                //
                genRegMov(bb, it,
                    spDcl->getRegVar(), ss->getSubRegOff(),
                    tmpDcl->getRegVar(),
                    tmpDcl->getNumElems(), builder.getPlatform() >= GENX_ICLLP ? false : true);
            }

            // create new src from the temp address variable, with offset 0
            auto s = builder.createIndirectSrc(ss->getModifier(), tmpDcl->getRegVar(), ss->getRegOff(), 0,
                ss->getRegion(), ss->getType(), ss->getAddrImm());
            inst->setSrc(s, i);
            if (!match_found)
            {
                pointsToAnalysis.insertAndMergeFilledAddr(ss->getBase()->asRegVar(), tmpDcl->getRegVar());
            }
        }
        else
            MUST_BE_TRUE(false, "Unknown reg access");
    }
}

//
// check if predicate is spilled & insert fill code
//
void SpillManager::replaceSpilledPredicate(G4_BB* bb,
                                           INST_LIST_ITER it, // where new insts will be inserted
                                           G4_INST*       inst)
{
    G4_Predicate* predicate = inst->getPredicate();
    if (predicate == NULL)
        return;

    G4_VarBase *flagReg = predicate->getBase();
    if (flagReg->asRegVar()->isRegAllocPartaker())
    {
        G4_Declare* flagDcl = flagReg->asRegVar()->getDeclare();
        G4_Declare* spDcl = flagDcl->getSpilledDeclare();
        if (spDcl != NULL)
        {
            G4_Declare* tmpDcl = createNewTempFlagDeclare(flagDcl);
            genRegMov(bb, it,
                      spDcl->getRegVar(), 0,
                      tmpDcl->getRegVar(),
                      tmpDcl->getNumElems());
            G4_Predicate *new_pred = builder.createPredicate(predicate->getState(), tmpDcl->getRegVar(), 0, predicate->getControl());
            inst->setPredicate(new_pred);
            ++numFlagSpillLoad;
        }
    }
}
//
// check if flag dst is spilled and insert spill code
//
void SpillManager::replaceSpilledFlagDst(G4_BB*         bb,
                                         INST_LIST_ITER it, // where new insts will be inserted
                                         G4_INST*       inst)
{
    G4_CondMod* mod = inst->getCondMod();
    if (mod == NULL)
        return;

    G4_VarBase *flagReg = mod->getBase();
    if (flagReg != NULL && flagReg->asRegVar()->isRegAllocPartaker())
    {
        G4_Declare* flagDcl = flagReg->asRegVar()->getDeclare();
        G4_Declare* spDcl = flagDcl->getSpilledDeclare();
        if (spDcl != NULL)
        {
            G4_Declare* tmpDcl;
            G4_Predicate* predicate = inst->getPredicate();

            if (predicate != NULL)
            {
                G4_VarBase *flagReg = predicate->getBase();
                tmpDcl = flagReg->asRegVar()->getDeclare();
            }
            else
            {
                tmpDcl = createNewTempFlagDeclare(flagDcl);
            }

            // Need to pre-load the spill GRF if the inst isn't going to write the full
            // spilled GRF variable.
            if (flagDcl->getNumberFlagElements() > inst->getExecSize() ||
                (!bb->isAllLaneActive() && !inst->isWriteEnableInst()))
            {
                genRegMov(bb, it,
                    spDcl->getRegVar(), 0,
                    tmpDcl->getRegVar(),
                    tmpDcl->getNumElems());
                ++numFlagSpillLoad;
            }

            G4_CondMod *newCondMod = builder.createCondMod(mod->getMod(), tmpDcl->getRegVar(), 0);

            inst->setCondMod(newCondMod);

            genRegMov(bb, ++it,
                      tmpDcl->getRegVar(), 0,
                      spDcl->getRegVar(),
                      tmpDcl->getNumElems());
            ++numFlagSpillStore;
        }
    }
}

//
// go over all declares and allocate spill locations
//
void SpillManager::createSpillLocations(G4_Kernel& kernel)
{
    // set spill flag to indicate which vars are spilled
    for (LIVERANGE_LIST::const_iterator lt = spilledLRs.begin (); lt != spilledLRs.end (); ++lt)
    {
        LiveRange* lr = *lt;
        G4_Declare* dcl = lr->getVar()->getDeclare();
        dcl->setSpillFlag();
        MUST_BE_TRUE(lr->getPhyReg() == NULL, "Spilled Live Range shouldn't have physical reg");
        MUST_BE_TRUE(lr->getSpillCost() < MAXSPILLCOST, "ERROR: spill live range with inifinite spill cost");
        // create spill loc for holding spilled addr regs
        createNewSpillLocDeclare(dcl);
    }
    // take care of alias declares
    DECLARE_LIST& declares = kernel.Declares;
    for (DECLARE_LIST_ITER it = declares.begin(); it != declares.end(); it++)
    {
        G4_Declare* dcl = (*it);
        if (!dcl->getRegVar()->isRegAllocPartaker()) // skip non reg alloc candidate
            continue;

        if (dcl->getAliasDeclare() != NULL &&    // dcl is not a representative declare
            dcl->getAliasDeclare()->isSpilled()) // dcl's representative decl is spilled
        {
            G4_Declare* sp = createNewSpillLocDeclare(dcl);
            // when doing RA multiple times (due to spill code), we don't want to set alias
            // information more than once
            if (sp->getAliasDeclare() == NULL)
            {
                sp->setAliasDeclare(dcl->getAliasDeclare()->getSpilledDeclare(), dcl->getAliasOffset());
            }
        }
    }
}

bool isSpillCandidateForLifetimeOpRemoval(G4_INST* inst)
{
    if(inst->isPseudoKill())
    {
        return inst->getDst()->isSpilled();
    }
    else if(inst->isLifeTimeEnd())
    {
        return inst->getSrc(0)->asSrcRegRegion()->isSpilled();
    }

    return false;
}

void SpillManager::insertSpillCode()
{
    //
    // create spill locations
    //
    createSpillLocations(kernel);

    FlowGraph& fg = kernel.fg;
    for (BB_LIST_ITER bb_it = fg.begin(); bb_it != fg.end(); bb_it++)
    {
        G4_BB* bb = *bb_it;
        bbId = bb->getId();
        //
        // handle spill code for the current BB
        //

        // In one iteration remove all spilled lifetime.start/end
        // ops.
        bb->erase(
            std::remove_if(bb->begin(), bb->end(), isSpillCandidateForLifetimeOpRemoval),
            bb->end());

        for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();)
        {
            G4_INST* inst = *inst_it;


            G4_Operand * operands_analyzed[G4_MAX_SRCS] = {NULL, NULL, NULL};
            G4_Declare * declares_created[G4_MAX_SRCS] = {NULL, NULL, NULL};
            // insert spill inst for spilled srcs
            for (unsigned i = 0; i < G4_MAX_SRCS; i++)
            {
                replaceSpilledSrc(bb, inst_it, inst, i, pointsToAnalysis, operands_analyzed, declares_created);
            }
            // insert spill inst for spilled dst
            replaceSpilledDst(bb, inst_it, inst, pointsToAnalysis, operands_analyzed, declares_created);

            //
            // Process predicate
            //
            G4_Predicate* predicate = inst->getPredicate();
            if(predicate != NULL) {
                replaceSpilledPredicate(bb, inst_it, inst);
            }

            //
            // Process condMod
            //
            G4_CondMod* mod = inst->getCondMod();
            if( mod != NULL &&
                mod->getBase() != NULL ) {
                replaceSpilledFlagDst(bb, inst_it, inst);
            }
            inst_it++;
        }
        bbId = UINT_MAX;
    }

}

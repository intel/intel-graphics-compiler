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

#include "Optimizer.h"
#include <fstream>
#include <sstream>
#include "G4_Opcode.h"
#include "Timer.h"
#include "G4Verifier.h"
#include <map>
#include <algorithm>
#include "LVN.h"
#include "ifcvt.h"
#include <random>
#include <chrono>
#include "FlowGraph.h"
#include "SendFusion.h"
#include "Common_BinaryEncoding.h"
#include <tuple>
#include "DebugInfo.h"

using namespace std;
using namespace vISA;

void Optimizer::LVN()
{
    // Run a simple LVN pass that replaces redundant
    // immediate loads in current BB. Also this pass
    // does not optimize operations like a
    // conventional VN pass because those require
    // more compile time, and are presumably already
    // done by FE generating VISA. This pass catches
    // redundancies that got introduced mainly by HW
    // conformity or due to VISA lowering.
    int numInstsRemoved = 0;
    Mem_Manager mem(1024);
    PointsToAnalysis p(kernel.Declares, kernel.fg.getNumBB());
    p.doPointsToAnalysis(kernel.fg);
    for (auto bb : kernel.fg)
    {
        ::LVN lvn(fg, bb, mem, *fg.builder, p);
        lvn.doLVN();

        numInstsRemoved += lvn.getNumInstsRemoved();

        numInstsRemoved += ::LVN::removeRedundantSamplerMovs(kernel, bb);
    }

    if(kernel.getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, kernel.getOptions());
        optreport << "===== LVN =====" << std::endl;
        optreport << "Number of instructions removed: " << numInstsRemoved << std::endl << std::endl;
        closeOptReportStream(optreport);
    }
}

// helper functions

static int getDstSubReg( G4_DstRegRegion *dst )
{
    int dstSubReg;
    if (dst->getBase()->isPhyReg())
    {
        dstSubReg = dst->getSubRegOff();
    }
    else
    {
        dstSubReg = dst->getSubRegOff() +
            static_cast<G4_RegVar*>(dst->getBase())->getPhyRegOff();
    }

    return dstSubReg;
}

static int getSrcSubReg( G4_Operand *src )
{
    MUST_BE_TRUE( src->isSrcRegRegion(), "expect Src Reg Region" );
    int srcSubReg;
    if (src->asSrcRegRegion()->getBase()->isPhyReg())
    {
        srcSubReg = src->asSrcRegRegion()->getSubRegOff();
    }
    else
    {
        srcSubReg = src->asSrcRegRegion()->getSubRegOff() +
            static_cast<G4_RegVar*>(src->asSrcRegRegion()->getBase())->getPhyRegOff();
    }
    return srcSubReg;
}

//
// determine if fall-through jump is needed
//
// also remove redundant jumps
//   if there is no predicate applied to a jump and its target is its fall though BB,
//   remove the jump instruction.

void Optimizer::insertFallThroughJump()
{

    for (BB_LIST_ITER it = fg.begin(); it != fg.end();)
    {
        G4_BB* bb = *it;
        BB_LIST_ITER next = ++it;
        //
        // determine if the current bb needs a fall through jump
        // check if the fall-through bb follows the current bb
        //
        G4_BB* fb = bb->fallThroughBB();
        if (fb && (next == fg.end() || // bb is the last bb
            fb != (*next)))
        {
            // This is bogus in SIMD CF, as bad things happen when you randomly insert jumps
            // in the middle of SIMD CF
        }
        else if (next != fg.end())
        {
            // do not remove a jmpi if it's the target of an indirect jmp
            // this makes the code more readable
            if (!(*next)->empty() && (*next)->front()->isLabel() &&
                !bb->empty() && bb->back()->opcode() == G4_jmpi &&
                bb->back()->getPredicate() == NULL &&
                !fg.isIndirectJmpTarget(bb->back()))
                {
                    if ((*next)->front()->getSrc(0) == bb->back()->getSrc(0))
                    {
                        std::list<G4_INST*>::iterator it = bb->end();
                        it--;
                        bb->erase(it);
                    }
            }
        }
        it = next;
    }
}

//
// Check if any GRF is out-of-boundary
//
void Optimizer::chkRegBoundary()
{
    for (auto bb : fg)
    {
        for (INST_LIST_ITER i = bb->begin(), end = bb->end(); i != end; i++)
        {
            G4_INST *inst = *i;
            if (!chkOpndBoundary(inst, inst->getDst())) {
                MUST_BE_TRUE(false, "region boundary check failure");
            }
            else
            {
                for (int src=0; src<G4_MAX_SRCS; src++)
                    if (!chkOpndBoundary(inst, inst->getSrc(src)))
                    {
                        MUST_BE_TRUE(false, "region boundary check failure");
                    }
            }
        }
    }
}

//
// Check if GRF (if we know the phy reg num) is out-of-boundary
// If the operand is not GRF, we return "true" without any check
//
bool Optimizer::chkOpndBoundary(G4_INST *inst, G4_Operand *opnd)
{
    if (opnd == NULL || (!opnd->isDstRegRegion() && !opnd->isSrcRegRegion()))
        return true;

    // we can not check indirect addressing now, leave it to address propagation
    if (opnd->isDstRegRegion() && opnd->asDstRegRegion()->getRegAccess() != Direct)
        return true;
    if (opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getRegAccess() != Direct)
        return true;

    return true;
}

void Optimizer::regAlloc()
{
    if (builder.getOption(vISA_DumpDotAll))
    {
        kernel.dumpDotFile("insertCallReturnVar");
    }

    fg.prepareTraversal();

    if (builder.getOption(vISA_DumpDotAll))
    {
        kernel.dumpDotFile("callReturn");
    }

    //
    // assign registers
    //
    int status = ::regAlloc(builder, builder.phyregpool, kernel);
    if (status != VISA_SUCCESS)
    {
        RAFail = true;
    }
}

void Optimizer::countBankConflicts()
{
    std::list<G4_INST*> conflicts;
    unsigned int numLocals = 0, numGlobals = 0;

    for (auto curBB : kernel.fg)
    {
        for(INST_LIST_ITER inst_it = curBB->begin();
            inst_it != curBB->end();
            inst_it++)
        {
            G4_INST* curInst = (*inst_it);
            G4_Operand* src0 = curInst->getSrc(0);
            G4_Operand* src1 = curInst->getSrc(1);
            G4_Operand* src2 = curInst->getSrc(2);

            if (src0 == NULL || src1 == NULL || src2 == NULL)
                continue;

            if (!src0->isSrcRegRegion() || !src1->isSrcRegRegion() || !src2->isSrcRegRegion())
                continue;

            if (!src0->asSrcRegRegion()->getBase()->asRegVar()->getPhyReg()->isGreg() ||
                !src1->asSrcRegRegion()->getBase()->asRegVar()->getPhyReg()->isGreg() ||
                !src1->asSrcRegRegion()->getBase()->asRegVar()->getPhyReg()->isGreg())
                continue;

            // We have a 3 src instruction with each src operand a GRF register region
            unsigned int src0grf = 0, src1grf = 0, src2grf = 0;

            src0grf = src0->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum() +
                src0->asSrcRegRegion()->getRegOff();
            src1grf = src1->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum() +
                src1->asSrcRegRegion()->getRegOff();
            src2grf = src2->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum() +
                src2->asSrcRegRegion()->getRegOff();

            bool isConflict = false;

            unsigned int src0partition = 0, src1partition = 0, src2partition = 0;

            if (src0grf < 64)
            {
                src0partition = 1;
                if (src0grf % 2 == 0)
                    src0partition = 0;
            }
            else
            {
                src0partition = 3;
                if (src0grf % 2 == 0)
                    src0partition = 2;
            }

            if (src1grf < 64)
            {
                src1partition = 1;
                if (src1grf % 2 == 0)
                    src1partition = 0;
            }
            else
            {
                src1partition = 3;
                if (src1grf % 2 == 0)
                    src1partition = 2;
            }

            if (src2grf < 64)
            {
                src2partition = 1;
                if (src2grf % 2 == 0)
                    src2partition = 0;
            }
            else
            {
                src2partition = 3;
                if (src2grf % 2 == 0)
                    src2partition = 2;
            }

            if (src0partition == src1partition && src1partition == src2partition)
                isConflict = true;

            if (curInst->getExecSize() == 16)
                isConflict = true;

            if (isConflict == true)
            {
                conflicts.push_back(curInst);
                numBankConflicts++;

                auto isGlobal = [](G4_Operand* opnd, FlowGraph& fg)
                    -> bool {
                    if (fg.globalOpndHT.isOpndGlobal(opnd))
                        return true;
                    return false;
                };

                if (!isGlobal(curInst->getSrc(0), kernel.fg) &&
                    curInst->getSrc(0)->getTopDcl()->getRegVar()->getPhyReg())
                    numLocals++;
                else
                    numGlobals++;

                if (!isGlobal(curInst->getSrc(1), kernel.fg) &&
                    curInst->getSrc(1)->getTopDcl()->getRegVar()->getPhyReg())
                    numLocals++;
                else
                    numGlobals++;

                if (!isGlobal(curInst->getSrc(2), kernel.fg) &&
                    curInst->getSrc(2)->getTopDcl()->getRegVar()->getPhyReg())
                    numLocals++;
                else
                    numGlobals++;
            }
        }
    }

    if (numBankConflicts > 0)
    {
        std::ofstream optreport;
        getOptReportStream(optreport, builder.getOptions());

        optreport << std::endl << std::endl;

        optreport << "===== Bank conflicts =====" << std::endl;
        optreport << "Found " << numBankConflicts << " conflicts (" << numLocals << " locals, " << numGlobals << " globals) in kernel: " << kernel.getName() << std::endl;
        for (std::list<G4_INST*>::iterator it = conflicts.begin();
            it != conflicts.end();
            it++)
        {
            G4_INST* i = (*it);
            i->emit(optreport);
            optreport << " // $" << i->getCISAOff() << ":#" << i->getLineNo() << std::endl;
        }

        optreport << std::endl << std::endl;

        closeOptReportStream(optreport);
    }
}

void Optimizer::insertDummyCompactInst()
{
    // Only for SKL+ and compaction is enabled.
    if (builder.getPlatform() < GENX_SKL || !builder.getOption(vISA_Compaction))
        return;

    // Insert mov (1) r0 r0 at the beginning of this kernel.
    G4_Declare *dcl = builder.getBuiltinR0();
    auto src = builder.createSrcRegRegion(Mod_src_undef, Direct,
        dcl->getRegVar(), 0, 0, builder.getRegionScalar(), Type_F);
    auto dst = builder.createDst(dcl->getRegVar(), 0, 0, 1, Type_F);
    G4_INST *movInst = builder.createMov(1, dst, src, InstOpt_WriteEnable, false);

    auto bb = fg.getEntryBB();
    for (auto it = bb->begin(), ie = bb->end(); it != ie; ++it)
    {
        if ((*it)->opcode() != G4_label)
        {
            bb->insert(it, movInst);
            return;
        }
    }

    // The entry block is empty or only contains a label.
    bb->push_back(movInst);
}

void Optimizer::insertHashMovs()
{
    // As per request from IGC team, we want to conditionally insert
    // two mov instructions like following:
    //
    // send ... {EOT}
    // mov (16) null<1>:d        lo32 {NoMask}
    // mov (16) null<1>:d        hi32 {NoMask}
    //
    for(BB_LIST_ITER bb_it = kernel.fg.begin();
        bb_it != kernel.fg.end();
        bb_it++)
    {
        G4_BB* bb = (*bb_it);

        for(INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++)
        {
            G4_INST* inst = (*inst_it);

            if(inst->isEOT())
            {
                // We have to insert new instructions after EOT.
                // Lexically, EOT could even be in the middle
                // of the program.
                auto insertHashMovInsts = [&](uint64_t hashVal)
                {
                    if (hashVal == 0)
                        return;

                    G4_INST* lo;
                    G4_INST* hi;
                    lo = kernel.fg.builder->createMov(
                        16,
                        kernel.fg.builder->createNullDst(Type_UD),
                        kernel.fg.builder->createImm((unsigned int)(hashVal & 0xffffffff), Type_UD),
                        InstOpt_WriteEnable, false)->InheritLLVMInst(inst);

                    hi = kernel.fg.builder->createMov(
                        16,
                        kernel.fg.builder->createNullDst(Type_UD),
                        kernel.fg.builder->createImm((unsigned int)((hashVal >> 32) & 0xffffffff), Type_UD),
                        InstOpt_WriteEnable, false)->InheritLLVMInst(inst);

                    bb->push_back(lo);
                    bb->push_back(hi);
                };
                uint64_t hashVal1 = builder.getOptions()->getuInt64Option(vISA_HashVal);
                uint64_t hashVal2 = builder.getOptions()->getuInt64Option(vISA_HashVal1);
                insertHashMovInsts(hashVal1);
                insertHashMovInsts(hashVal2);
                break;
            }
        }
    }
}

bool isLifetimeOpRemovalCandidate(G4_INST* inst)
{
    if( inst->isPseudoKill() || inst->isLifeTimeEnd() )
    {
        return true;
    }

    return false;

}

bool isPseudoUse(G4_INST* inst)
{
    if (inst->isPseudoUse())
    {
        return true;
    }
    return false;
}

void Optimizer::removeLifetimeOps()
{
    // Call this function after RA only.

    // Remove all pseudo_kill and lifetime.end
    // instructions.
    // Also remove pseudo_use instructions.
    for( BB_LIST_ITER bbs = kernel.fg.begin();
        bbs != fg.end();
        bbs++ )
    {
        G4_BB* bb = *bbs;
        bb->erase(
            std::remove_if(bb->begin(), bb->end(),
            [](G4_INST* inst) { return inst->isPseudoKill() || inst->isLifeTimeEnd() || inst->isPseudoUse(); }),
            bb->end());
    }
}

void Optimizer::runPass(PassIndex Index)
{
    const PassInfo &PI = Passes[Index];

    // Do not execute.
    if (PI.Option != vISA_EnableAlways && !builder.getOption(PI.Option))
        return;

    std::string Name = PI.Name;

    if (builder.getOption(vISA_DumpDotAll))
        kernel.dumpDotFile(("before." + Name).c_str());

    if (PI.Timer != TIMER_NUM_TIMERS)
        startTimer(PI.Timer);

    // Execute pass.
    (this->*(PI.Pass))();

    if (PI.Timer != TIMER_NUM_TIMERS)
        stopTimer(PI.Timer);

    if (builder.getOption(vISA_DumpDotAll))
        kernel.dumpDotFile(("after." + Name).c_str());

#ifdef _DEBUG
    bool skipVerify = Index == PI_regAlloc && RAFail == true;
    if (!skipVerify)
    {
        verifyG4Kernel(kernel, Index, true, G4Verifier::VC_ASSERT);
    }
#endif
}

void Optimizer::initOptimizations()
{
#define INITIALIZE_PASS(Name, Option, Timer) \
    Passes[PI_##Name] = PassInfo(&Optimizer::Name, ""#Name, Option, Timer)

    // To initialize a pass, the member function name is the first argument.
    // This member function must return void and take no argument.
    //
    // The second argument is the corresponding option to enable this pass.
    // If it always runs then use vISA_EnableAlways.
    //
    // The third argument is the intended timer for this pass. If no timing
    // is necessary, then TIMER_NUM_TIMERS can be used.
    //
    INITIALIZE_PASS(cleanMessageHeader,      vISA_LocalCleanMessageHeader, TIMER_OPTIMIZER);
    INITIALIZE_PASS(sendFusion,              vISA_EnableSendFusion,        TIMER_OPTIMIZER);
    INITIALIZE_PASS(renameRegister,          vISA_LocalRenameRegister,     TIMER_OPTIMIZER);
    INITIALIZE_PASS(newLocalDefHoisting,     vISA_LocalDefHoist,           TIMER_OPTIMIZER);
    INITIALIZE_PASS(newLocalCopyPropagation, vISA_LocalCopyProp,           TIMER_OPTIMIZER);
    INITIALIZE_PASS(cselPeepHoleOpt,         vISA_enableCSEL,              TIMER_OPTIMIZER);
    INITIALIZE_PASS(optimizeLogicOperation,  vISA_EnableAlways,            TIMER_OPTIMIZER);
    INITIALIZE_PASS(HWConformityChk,         vISA_EnableAlways,            TIMER_HW_CONFORMITY);
    INITIALIZE_PASS(preRA_Schedule,          vISA_preRA_Schedule,          TIMER_PRERA_SCHEDULING);
    INITIALIZE_PASS(preRA_HWWorkaround,      vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(regAlloc,                vISA_EnableAlways,            TIMER_TOTAL_RA);
    INITIALIZE_PASS(removeLifetimeOps,       vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(countBankConflicts,      vISA_OptReport,               TIMER_MISC_OPTS);
    INITIALIZE_PASS(removeRedundMov,         vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(removeEmptyBlocks,       vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(insertFallThroughJump,   vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(reassignBlockIDs,        vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(evalAddrExp,             vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(FoldAddrImmediate,       vISA_FoldAddrImmed,           TIMER_MISC_OPTS);
    INITIALIZE_PASS(chkRegBoundary,          vISA_EnableAlways,            TIMER_NUM_TIMERS);
    INITIALIZE_PASS(localSchedule,           vISA_LocalScheduling,         TIMER_SCHEDULING);
    INITIALIZE_PASS(NoDD,                    vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(HWWorkaround,            vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(NoSrcDepSet,             vISA_EnableNoSrcDep,          TIMER_MISC_OPTS);
    INITIALIZE_PASS(insertInstLabels,        vISA_EnableAlways,            TIMER_NUM_TIMERS);
    INITIALIZE_PASS(insertHashMovs,          vISA_InsertHashMovs,          TIMER_NUM_TIMERS);
    INITIALIZE_PASS(insertDummyCompactInst,  vISA_InsertDummyCompactInst,  TIMER_NUM_TIMERS);
    INITIALIZE_PASS(mergeScalarInst,         vISA_MergeScalar,             TIMER_OPTIMIZER);
    INITIALIZE_PASS(lowerMadSequence,        vISA_EnableMACOpt,            TIMER_OPTIMIZER);
    INITIALIZE_PASS(LVN,                     vISA_LVN,                     TIMER_OPTIMIZER);
    INITIALIZE_PASS(ifCvt,                   vISA_ifCvt,                   TIMER_OPTIMIZER);
    INITIALIZE_PASS(dumpPayload,             vISA_dumpPayload,             TIMER_MISC_OPTS);
    INITIALIZE_PASS(normalizeRegion,         vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(collectStats,            vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(createR0Copy,            vISA_enablePreemption,        TIMER_MISC_OPTS);
    INITIALIZE_PASS(initializePayload,       vISA_InitPayload,             TIMER_NUM_TIMERS);
    INITIALIZE_PASS(cleanupBindless,         vISA_enableCleanupBindless,   TIMER_OPTIMIZER);
    INITIALIZE_PASS(countGRFUsage,           vISA_PrintRegUsage,           TIMER_MISC_OPTS);
    INITIALIZE_PASS(changeMoveType,          vISA_ChangeMoveType,          TIMER_MISC_OPTS);
    INITIALIZE_PASS(reRAPostSchedule,        vISA_ReRAPostSchedule,        TIMER_OPTIMIZER);
    INITIALIZE_PASS(accSubPostSchedule,      vISA_accSubstitution,         TIMER_OPTIMIZER);
    INITIALIZE_PASS(dce,                     vISA_EnableDCE,               TIMER_OPTIMIZER);
    INITIALIZE_PASS(reassociateConst,        vISA_reassociate,             TIMER_OPTIMIZER);
    INITIALIZE_PASS(split4GRFVars,           vISA_split4GRFVar,            TIMER_OPTIMIZER);
    INITIALIZE_PASS(addFFIDProlog,           vISA_addFFIDProlog,           TIMER_MISC_OPTS);
    INITIALIZE_PASS(loadThreadPayload,       vISA_loadThreadPayload,       TIMER_MISC_OPTS);
    INITIALIZE_PASS(insertFenceBeforeEOT,    vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(insertScratchReadBeforeEOT, vISA_clearScratchWritesBeforeEOT, TIMER_MISC_OPTS);
    INITIALIZE_PASS(mapOrphans,              vISA_EnableAlways,            TIMER_MISC_OPTS);
    INITIALIZE_PASS(varSplit,                vISA_EnableAlways,            TIMER_OPTIMIZER);

    // Verify all passes are initialized.
#ifdef _DEBUG
    for (unsigned i = 0; i < PI_NUM_PASSES; ++i)
    {
        ASSERT_USER(Passes[i].Pass, "uninitialized pass");
    }
#endif
}

void replaceAllSpilledRegions(G4_Kernel& kernel, G4_Declare* oldDcl, G4_Declare* newDcl)
{
    // Iterate fg and replace all references to oldDcl with newDcl.
    // This requires creating new operands since changing dcl in
    // existing operands is disallowed.
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            auto dst = inst->getDst();
            if (dst &&
                !dst->isNullReg())
            {
                if (dst->getTopDcl() == oldDcl)
                {
                    G4_DstRegRegion* newDstRgn = kernel.fg.builder->createDstRegRegion(
                        dst->getRegAccess(), newDcl->getRegVar(), dst->getRegOff(),
                        dst->getSubRegOff(), dst->getHorzStride(), dst->getType());
                    inst->setDest(newDstRgn);
                }
            }

            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                auto src = inst->getSrc(i);

                if (src &&
                    src->isSrcRegRegion())
                {
                    auto srcRgn = inst->getSrc(i)->asSrcRegRegion();

                    if (srcRgn->getTopDcl() == oldDcl)
                    {
                        G4_SrcRegRegion* newSrcRgn = kernel.fg.builder->createSrcRegRegion(
                            srcRgn->getModifier(), srcRgn->getRegAccess(), newDcl->getRegVar(),
                            srcRgn->getRegOff(), srcRgn->getSubRegOff(), srcRgn->getRegion(),
                            srcRgn->getType());
                        inst->setSrc(newSrcRgn, i);
                    }
                }
            }
        }
    }
}

void getPhyRegs(G4_Operand* opnd, unsigned int& start, unsigned int& end)
{
    // start/end are inclusive, ie both offsets are referenced by the variable
    start = 0;
    end = 0;

    auto b = opnd->getBase();
    if (opnd->isAddrExp())
    {
        b = opnd->asAddrExp()->getRegVar();
    }

    if (b && b->isRegVar())
    {
        auto r = b->asRegVar();
        if (r->getPhyReg() &&
            r->getPhyReg()->isGreg())
        {
            if (!opnd->isAddrExp())
            {
                start = opnd->getLinearizedStart();
                end = opnd->getLinearizedEnd();
            }
            else
            {
                r = r->getDeclare()->getRootDeclare()->getRegVar();
                auto phyReg = r->getPhyReg()->asGreg();
                auto subRegOff = r->getPhyRegOff();
                start = phyReg->getRegNum() * G4_GRF_REG_NBYTES;
                start += subRegOff * r->getDeclare()->getElemSize();
                end = start + r->getDeclare()->getRootDeclare()->getByteSize() - 1;
            }
        }
    }
}

void computeGlobalFreeGRFs(G4_Kernel& kernel)
{
    auto gtpin = kernel.getGTPinData();
    gtpin->clearFreeGlobalRegs();
    std::vector<bool> freeGRFs(kernel.getNumRegTotal() * G4_GRF_REG_NBYTES, true);
    unsigned int start = 0, end = 0;

    // Mark r0 as busy. Done explicitly because move from r0 is inserted
    // after reRA pass.
    for (unsigned int i = 0; i < G4_GRF_REG_NBYTES; i++)
    {
        freeGRFs[i] = false;
    }

    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            auto dst = inst->getDst();
            if (dst &&
                dst->getBase() &&
                dst->getBase()->isRegVar())
            {
                getPhyRegs(dst, start, end);
                for (unsigned int i = start; i <= end; i++)
                {
                    freeGRFs[i] = false;
                }
            }

            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                auto src = inst->getSrc(i);

                if (!src)
                    continue;

                if (src->isSrcRegRegion() &&
                    src->asSrcRegRegion()->getBase()->isRegVar())
                {
                    getPhyRegs(src, start, end);
                    for (unsigned int i = start; i <= end; i++)
                    {
                        freeGRFs[i] = false;
                    }
                }
                else if (src->isAddrExp())
                {
                    getPhyRegs(src, start, end);
                    for (unsigned int i = start; i <= end; i++)
                    {
                        freeGRFs[i] = false;
                    }
                }
            }
        }
    }

    for (unsigned int i = 0; i < freeGRFs.size(); i++)
    {
        if (freeGRFs[i])
        {
            gtpin->addFreeGlobalReg(i);
        }
    }
}

typedef struct Assignment
{
    G4_Declare* dcl = nullptr;
    AssignedReg reg;
    unsigned int GRFBaseOffset = 0;
} Assignment;

void storeGRFAssignments(DECLARE_LIST& declares, std::vector<Assignment>& assignments)
{
    for (auto dcl : declares)
    {
        auto regVar = dcl->getRegVar();
        if (regVar &&
            regVar->isPhyRegAssigned() &&
            regVar->getPhyReg()->isGreg())
        {
            Assignment a;
            AssignedReg assignedPhyReg;
            assignedPhyReg.phyReg = regVar->getPhyReg();
            assignedPhyReg.subRegOff = regVar->getPhyRegOff();
            a.dcl = dcl;
            a.reg = assignedPhyReg;
            a.GRFBaseOffset = dcl->getGRFBaseOffset();
            assignments.push_back(a);

        }
    }
}

void restoreGRFAssignments(std::vector<Assignment>& assignments)
{
    for (auto& a : assignments)
    {
        auto regVar = a.dcl->getRegVar();
        regVar->setPhyReg(a.reg.phyReg, a.reg.subRegOff);
        a.dcl->setGRFBaseOffset(a.GRFBaseOffset);
    }
}

// simple heuristics to decide if it's profitable to do copy propagation for the move
// add more as necessary
bool Optimizer::isCopyPropProfitable(G4_INST* movInst) const
{
    assert(movInst->opcode() == G4_mov && "execpt move instruction");

    // if inst is a simd16 W/HF packing, we don't want to optimize it if
    // there are >=2 simd16 mad uses, since it will slow down the mad.
    // for gen9 additionally check for simd8 mad as it doesn't support strided regions
    auto dst = movInst->getDst();
    auto src0 = movInst->getSrc(0);
    auto hasStrideSource = dst->getHorzStride() == 1 &&
        src0->isSrcRegRegion() &&
        !(src0->asSrcRegRegion()->getRegion()->isContiguous(movInst->getExecSize()) ||
            src0->asSrcRegRegion()->getRegion()->isScalar());

    hasStrideSource &= movInst->getExecSize() == 16 || (!builder.hasAlign1Ternary() && movInst->getExecSize() == 8);

    auto hasNSIMD16or8MadUse = [](G4_INST* movInst, int N, bool checkSIMD8)
    {
        int numMadUses = 0;
        for (auto iter = movInst->use_begin(), iterEnd = movInst->use_end(); iter != iterEnd; ++iter)
        {
            auto use = *iter;
            auto inst = use.first;
            if (inst->opcode() == G4_pseudo_mad &&
                (inst->getExecSize() == 16 || (checkSIMD8 && inst->getExecSize() == 8)))
            {
                ++numMadUses;
                if (numMadUses == N)
                {
                    return true;
                }
            }
        }
        return false;
    };

    if (hasStrideSource)
    {
        if (hasNSIMD16or8MadUse(movInst, 2, true))
        {
            return false;
        }
    }

    // another condition where copy prop may be not profitable:
    // mov is from HF to F and dst is used in simd16 mad.
    // copy propagating away the move results in mix mode mad which is bad for bank conflicts
    if (dst->getType() == Type_F && src0->getType() == Type_HF)
    {
        if (hasNSIMD16or8MadUse(movInst, 4, false))
        {
            return false;
        }
    }
    return true;
}

void Optimizer::reRAPostSchedule()
{
    // No rera for stackcall functions
    if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
        return;

    std::vector<Assignment> assignments;
    auto finalizerInfo = *builder.getJitInfo();
    auto oldRAType = kernel.getRAType();
    auto gtpin = kernel.getGTPinData();
    computeGlobalFreeGRFs(kernel);
    auto freeGRFsBeforeReRA = gtpin->getNumFreeGlobalRegs();

    storeGRFAssignments(kernel.Declares, assignments);
    // This pass is run for gtpin since they need re-allocation after
    // scheduling pass to get as many free registers for instrumentation.

    // Store dcls referenced by srcs of 3-src instructions. These will
    // not be participate in re-allocation because that could potentially
    // alter bank conflict profile which might affect kernel performance.
    std::set<G4_Declare*> threeSrcDcl;
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            if (inst->isMovAddr() &&
                inst->getDst()->getTopDcl()->getRegFile() == G4_RegFileKind::G4_GRF)
            {
                // inst's dst is a spilled address register from previous RA pass.
                // Addr/flags are spilled to GRF. When address regs are spilled to GRF,
                // points2 analysis will miss those entries since it expects only addr
                // reg when src is G4_AddrExp type. So we replace such spilled addr
                // operands with new address operands.
                G4_Declare* dstDcl = inst->getDst()->getTopDcl();
                auto name = builder.getNameString(fg.mem, 16, "ADDR%d", kernel.Declares.size());
                G4_Declare* newDstAddrDcl = kernel.fg.builder->createDeclareNoLookup(name,
                    G4_RegFileKind::G4_ADDRESS, dstDcl->getNumElems(),
                    dstDcl->getNumRows(), dstDcl->getElemType());
                replaceAllSpilledRegions(kernel, dstDcl, newDstAddrDcl);
            }

            if (inst->getNumSrc() == 3 && !inst->isSend())
            {
                for (auto i = 0; i < inst->getNumSrc(); i++)
                {
                    auto srcOpnd = inst->getSrc(i);
                    if (srcOpnd->isSrcRegRegion())
                    {
                        auto dcl = srcOpnd->getTopDcl();
                        threeSrcDcl.insert(dcl);
                    }
                }
            }
        }
    }

    for (auto dcl : kernel.Declares)
    {
        if (threeSrcDcl.find(dcl->getRootDeclare()) == threeSrcDcl.end())
        {
            dcl->prepareForRealloc(&kernel);
        }
    }

    kernel.fg.clearBBLRASummaries();

    gtpin->setRAPass(gtPinData::RAPass::ReRAPass);

    gtpin->removeUnmarkedInsts();

    // This time we want to try only first-fit and no localra
    regAlloc();

    removeLifetimeOps();

    computeGlobalFreeGRFs(kernel);

    if (RAFail ||
        gtpin->getNumFreeGlobalRegs() < freeGRFsBeforeReRA)
    {
        restoreGRFAssignments(assignments);
        kernel.setRAType(oldRAType);

        computeGlobalFreeGRFs(kernel);
    }

    *builder.getJitInfo() = finalizerInfo;
}

void Optimizer::accSubPostSchedule()
{
    if (!builder.doAccSub() || !builder.getOption(vISA_doAccSubAfterSchedule))
    {
        return;
    }

    kernel.fg.resetLocalDataFlowData();
    kernel.fg.localDataFlowAnalysis();

    HWConformity hwConf(builder, kernel, mem);

    if (builder.getOption(vISA_localizationForAccSub))
    {
        for (auto bb : kernel.fg)
        {
            hwConf.localizeForAcc(bb);
        }

        kernel.fg.resetLocalDataFlowData();
        kernel.fg.localDataFlowAnalysis();
    }

    for (auto bb : kernel.fg)
    {
        hwConf.accSubstitution(bb);
    }
}

void* gtPinData::getFreeGRFInfo(unsigned int& size)
{
    // Here is agreed upon format for reporting free GRFs:
    //struct freeBytes
    //{
    //    unsigned short startByte;
    //    unsigned short numConsecutiveBytes;
    //};

    // Added magic 0xDEADD00D at start and
    // magic 0xDEADBEEF at the end of buffer
    // on request of gtpin team.
    //
    //struct freeGRFInfo
    //{
    //    unsigned short numItems;
    //
    //    freeBytes data[numItems];
    //};
    typedef struct freeBytes
    {
        unsigned short startByte;
        unsigned short numConsecutiveBytes;
    } freeBytes;

    typedef struct freeGRFInfo
    {
        unsigned int magicStart;
        unsigned int numItems;
        freeBytes* data;
    } freeGRFInfo;

    // Compute free register information using vector for efficiency,
    // then convert to POS for passing back to gtpin.
    std::vector<std::pair<unsigned short, unsigned short>> vecFreeBytes;

    for (auto byte : globalFreeRegs)
    {
        if (vecFreeBytes.size() > 0)
        {
            auto& lastFree = vecFreeBytes.back();
            if (byte == (lastFree.first + lastFree.second))
            {
                lastFree.second += 1;
            }
            else
            {
                vecFreeBytes.push_back(std::make_pair(byte, 1));
            }
        }
        else
        {
            vecFreeBytes.push_back(std::make_pair(byte, 1));
        }
    }

    // Now convert vector to POS
    unsigned int numItems = (unsigned int)vecFreeBytes.size();
    freeGRFInfo* buffer = (freeGRFInfo*)malloc(numItems * sizeof(freeBytes) + sizeof(unsigned int)
        + sizeof(unsigned int) + sizeof(unsigned int));
    if (buffer)
    {
        buffer->numItems = numItems;
        buffer->magicStart = 0xDEADD00D;
        memcpy_s((unsigned char*)buffer + sizeof(unsigned int) + sizeof(unsigned int),
            numItems * sizeof(freeBytes), vecFreeBytes.data(), numItems * sizeof(freeBytes));
        unsigned int magicEnd = 0xDEADBEEF;
        memcpy_s((unsigned char*)buffer + sizeof(unsigned int) + sizeof(unsigned int) + (numItems * sizeof(freeBytes)),
            sizeof(magicEnd), &magicEnd, sizeof(magicEnd));

        // numItems - unsigned int
        // magicStart - unsigned int
        // magicEnd - unsigned int
        // data - numItems * sizeof(freeBytes)
        size = sizeof(unsigned int) + sizeof(unsigned int) + sizeof(unsigned int) + (numItems * sizeof(freeBytes));
    }

    return (void*)buffer;
}

int Optimizer::optimization()
{
    // remove redundant message headers.
    runPass(PI_cleanMessageHeader);


    runPass(PI_sendFusion);

    // rename registers.
    runPass(PI_renameRegister);

    runPass(PI_newLocalDefHoisting);

    // remove redundant movs
    runPass(PI_newLocalCopyPropagation);

    runPass(PI_mergeScalarInst);

    runPass(PI_cselPeepHoleOpt);

    // this must be run after copy prop cleans up the moves
    runPass(PI_cleanupBindless);

    runPass(PI_reassociateConst);

    runPass(PI_lowerMadSequence);

    // optimize logic operantions
    runPass(PI_optimizeLogicOperation);

    // Dead code elimination
    runPass(PI_dce);

    // HW conformity check
    runPass(PI_HWConformityChk);

    // Local Value Numbering
    runPass(PI_LVN);

    runPass(PI_split4GRFVars);

    runPass(PI_insertFenceBeforeEOT);

    runPass(PI_varSplit);

    // PreRA scheduling
    runPass(PI_preRA_Schedule);

    // HW workaround before RA (assume no pseudo inst)
    runPass(PI_preRA_HWWorkaround);

    // perform register allocation
    runPass(PI_regAlloc);
    if (RAFail)
    {
        return VISA_SPILL;
    }

    runPass(PI_removeLifetimeOps);

    runPass(PI_countBankConflicts);

    // some passes still rely on G4_Declares and their def-use even after RA,
    // and removeRedundantMove will break them since it deletes moves solely based on GRF assignment
    // without maintaining def-use of G4_Declares.
    // so when these passes are active we have to defer removeRedundMov until after the passes
    // ToDo: study the perf impact of moving this post scheduling
    bool preserveVirtualDefUse = kernel.getOption(vISA_ReRAPostSchedule) || builder.doAccSub();

    if (!preserveVirtualDefUse)
    {
        runPass(PI_removeRedundMov);
    }

    //
    // if a fall-through BB does not immediately follow its predecessor
    // in the code layout, then insert a jump-to-fall-through in the predecessor
    //
    runPass(PI_insertFallThroughJump);

    // Run if-conversion to convert short if-blocks.
    runPass(PI_ifCvt);

    //
    // re-assign block ID so that we can use id to determine the ordering of
    // two blocks in the code layout
    //
    runPass(PI_reassignBlockIDs);

    runPass(PI_FoldAddrImmediate);

    //
    // Check if any GRF is out-of-boundary
    // FIXME: Now we can only check part of the out-of-boundary cases. We can not find out-of-boundary errors in:
    // (1). <post_dst>/<curr_dst> in send inst; (2). cross 256-bit bar case in compressed instruction
    //
    runPass(PI_chkRegBoundary);

    runPass(PI_localSchedule);

    runPass(PI_accSubPostSchedule);

    runPass(PI_changeMoveType);

    // NoDD optimization
    runPass(PI_NoDD);

    runPass(PI_reRAPostSchedule);

    if (preserveVirtualDefUse
        )
    {
        runPass(PI_removeRedundMov);
    }

    // Insert NoSrcDepSet flag to improve performance
    runPass(PI_NoSrcDepSet);

    // remove any placeholders blocks inserted to aid regalloc
    // run this pass after reRA pass otherwise CFG can become
    // invalid (funcInfo, calleeInfo may point to bad initBB).
    runPass(PI_removeEmptyBlocks);

    runPass(PI_insertScratchReadBeforeEOT);

    // HW workaround
    runPass(PI_HWWorkaround);

    runPass(PI_normalizeRegion);

    runPass(PI_countGRFUsage);

    runPass(PI_dumpPayload);

    // this must be the last step of the optimization so as to not violate
    // the CFG assumption
    runPass(PI_insertInstLabels);

    runPass(PI_insertHashMovs);

    runPass(PI_collectStats);

    // Create a copy of R0 at the top of kernel.
    // This must be done after all other optimizer
    // passes, except initializePayload, to avoid
    // potential code ordering issue.
    runPass(PI_createR0Copy);

    runPass(PI_initializePayload);

    runPass(PI_loadThreadPayload);

    runPass(PI_addFFIDProlog);

    // Insert a dummy compact instruction if requested for SKL+
    runPass(PI_insertDummyCompactInst);

    runPass(PI_mapOrphans);

    return VISA_SUCCESS;
}

//  When constructing CFG we have the assumption that a label must be the first
//  instruction in a bb.  During structure analysis, however, we may end up with a bb that
//  starts with multiple endifs if the bb is the target of multiple gotos that have been
//  converted to an if. Instead of creating a BB for each of the endif, we associate each endif with a label
//  and emit them only at the very end.
//
//  For break and continue, UIP must be the lable directly attached to the while
//  op. If not, create such a label
//
//  DO
//    IF
//      P =
//      CONT L1
//    ENDIF L1
//    IF
//      BREAK L2
//    ENDIF L1
//  L1
//  (P) WHILE
//  L2
//
//  will be transfered into
//
//  DO
//    IF
//      P =
//      Spill <- P
//      CONT L3         // UIP becomes L3
//    ENDIF L1
//    IF
//      BREAK L3        // UIP becomes L3
//    ENDIF L1
//  L1                  // existing label
//  P <- fill
//  L3                  // new label
//  (P) WHILE
//  L2
//
void Optimizer::insertInstLabels()
{
    for( BB_LIST_CITER iter = fg.cbegin(), bend = fg.cend(); iter != bend; ++iter )
    {
        G4_BB* bb = *iter;
        INST_LIST_ITER iter2 = bb->begin();
        INST_LIST_ITER iend  = bb->end();
        while( iter2 != iend )
        {
            INST_LIST_ITER currIter = iter2;
            ++iter2;

            G4_INST* inst = *currIter;
            G4_Label* endifLabel = fg.getLabelForEndif(inst);
            if (endifLabel)
            {
                G4_INST* labelInst = fg.createNewLabelInst( endifLabel, inst->getLineNo(), inst->getCISAOff() );
                bb->insert( currIter, labelInst );
            }
        }
    }

    // Patch labels if necessary.
    for(auto iter = fg.begin(), iend = fg.end(); iter != iend; ++iter)
    {
        G4_BB *bb = *iter;
        if (bb->empty())
            continue;

        G4_INST *inst = bb->back();
        G4_opcode opc = inst->opcode();
        if (opc != G4_cont && opc != G4_break)
            continue;

        // The matching while BB.
         G4_BB *whileBB = nullptr;
         if (opc == G4_cont)
         {
             // The whileBB is the first successor bb, if this is continue.
             whileBB = bb->Succs.front();
         }
         else
         {
             // For break, the whileBB should be the physical predecessor of
             // break's first successor bb.
             BB_LIST_ITER iter = bb->Succs.begin();
             while (iter != bb->Succs.end())
             {
                 G4_BB * succBB = (*iter);
                 if (succBB->getPhysicalPred() &&
                     (!succBB->getPhysicalPred()->empty()) &&
                     (succBB->getPhysicalPred()->back()->opcode() == G4_while))
                 {
                     whileBB = succBB->getPhysicalPred();
                     break;
                 }
                 iter++;
             }
         }

         if (whileBB == nullptr ||
             whileBB->empty() ||
             whileBB->back()->opcode() != G4_while)
         {
             ASSERT_USER(false, "can not find while BB");
             continue;
         }

         // If while instruction is following the label, then no need
         // to insert a new uip label, just use the existing one.
         G4_InstCF *instCF = inst->asCFInst();
         auto whileIter = std::prev(whileBB->end());
         G4_INST *prevInst = *std::prev(whileIter);
         if (prevInst->isLabel())
         {
             instCF->setUip(prevInst->getLabel());
         }
         else
         {
             std::string NewUipName = instCF->getUipLabelStr();
             NewUipName += "_UIP";
             G4_Label *label = builder.createLabel(NewUipName, LABEL_BLOCK);
             instCF->setUip(label);

             G4_INST *newInst = fg.createNewLabelInst(label, inst->getLineNo(),
                                                      inst->getCISAOff());

             whileBB->insert(whileIter, newInst);
         }
    }
}

//Fold address register into address register offset, such that we can same one instruction
//that computes:
//add a0.0 a0.0 immed;
//mul dst r[a0.0, 0] src2
//-->
//mul dst r[a0.0, immed] src2

//The condition is that immed is in range [-512..511] and it is dividable by 32.
//This is a local OPT.
//For simplicity, only execsize 1 is considered.
//Since physical registers are alreasy assigned, we use the info directly here without check.

void Optimizer::reverseOffsetProp(
    AddrSubReg_Node addrRegInfo[8],
    int subReg,
    unsigned int srcNum,
    INST_LIST_ITER lastIter,
    INST_LIST_ITER iend
    )
{
    if( addrRegInfo[subReg].usedImmed && addrRegInfo[subReg].canUseImmed )
    {
        INST_LIST_ITER iter;
        G4_INST *inst;
        G4_Operand *inst_src;
        G4_DstRegRegion *inst_dst;
        for (iter = addrRegInfo[subReg].iter; iter != lastIter; ++iter)
        {
            if( iter == lastIter )
                break;
            inst = *iter;
            if( inst->isDead())
                continue;
            inst_dst = inst->getDst();
            if( inst_dst &&
                inst_dst->getRegAccess() != Direct )
            {
                int subReg1 = getDstSubReg( inst_dst );

                short currOff = inst_dst->getAddrImm();
                if( subReg1 == subReg )
                {
                    // create a new dst
                    G4_DstRegRegion tmpRgn( *inst_dst );
                    G4_DstRegRegion *newDst = &tmpRgn;
                    newDst->setImmAddrOff( short(currOff - addrRegInfo[subReg].immAddrOff) );
                    inst->setDest( builder.createDstRegRegion(*newDst) );
                }
            }
            for (int i = 0; i < inst->getNumSrc(); i++)
            {
                inst_src = inst->getSrc(i);
                if( inst_src && inst_src->isSrcRegRegion() &&
                    inst_src->asSrcRegRegion()->getRegAccess() != Direct )
                {
                    int subReg1 = getSrcSubReg( inst_src );

                    short currOff = inst_src->asSrcRegRegion()->getAddrImm();
                    if( subReg1 == subReg )
                    {
                        G4_SrcRegRegion tmpRgn(*inst_src->asSrcRegRegion());
                        G4_SrcRegRegion *newSrc = &tmpRgn;
                        newSrc->setImmAddrOff( short(currOff - addrRegInfo[subReg].immAddrOff) );
                        inst->setSrc( builder.createSrcRegRegion(*newSrc) , i );
                    }
                }
            }
        }
        // Immed has been propagated to srcs before this src in *ii, also reverse this
        if( srcNum > 0 )
        {
            inst = *lastIter;
            for (unsigned i = 0; i < srcNum; i++)
            {
                inst_src = inst->getSrc(i);
                if( inst_src && inst_src->isSrcRegRegion() &&
                    inst_src->asSrcRegRegion()->getRegAccess() != Direct )
                {
                    int subReg1 = getSrcSubReg( inst_src );

                    short currOff = inst_src->asSrcRegRegion()->getAddrImm();
                    if( subReg1 == subReg )
                    {
                        G4_SrcRegRegion tmpRgn(*inst_src->asSrcRegRegion());
                        G4_SrcRegRegion *newSrc = &tmpRgn;
                        newSrc->setImmAddrOff( short(currOff - addrRegInfo[subReg].immAddrOff) );
                        inst->setSrc( builder.createSrcRegRegion(*newSrc) , i );
                    }
                }
            }
        }
    }

    addrRegInfo[subReg].immAddrOff = 0;
    addrRegInfo[subReg].iter = iend;
    addrRegInfo[subReg].canRemoveInst = false;
    addrRegInfo[subReg].canUseImmed = false;
    addrRegInfo[subReg].usedImmed = false;
}

void Optimizer::FoldAddrImmediate()
{
    AddrSubReg_Node* addrRegInfo = new AddrSubReg_Node[getNumAddrRegisters()];
    BB_LIST_ITER ib, bend(fg.end());
    int dst_subReg = 0, src0_subReg = 0;
    G4_DstRegRegion *dst;
    G4_Operand *src0, *src1;
    unsigned num_srcs;

    for(ib = fg.begin(); ib != bend; ++ib)
    {
        G4_BB* bb = (*ib);
        INST_LIST_ITER ii, iend(bb->end());
        // reset address offset info
        for(unsigned i = 0; i < getNumAddrRegisters(); i++)
        {
            addrRegInfo[i].subReg = 0;
            addrRegInfo[i].immAddrOff = 0;
            addrRegInfo[i].iter = iend;
            addrRegInfo[i].canRemoveInst = false;
            addrRegInfo[i].canUseImmed = false;
            addrRegInfo[i].usedImmed = false;
        }
        for (ii = bb->begin(); ii != iend; ii++)
        {
            G4_INST *inst = *ii;
            if(inst->isDead())
            {
                continue;
            }
            num_srcs = inst->getNumSrc();
            dst = inst->getDst();
            if (dst)
            {
                dst_subReg = getDstSubReg( dst );
            }
            src0 = inst->getSrc(0);
            if( src0 && src0->isSrcRegRegion() )
            {
                src0_subReg = getSrcSubReg( src0 );
            }
            src1 = inst->getSrc(1);

            if (dst != NULL && dst->getRegAccess() == Direct &&
                src0 != NULL && src0->isSrcRegRegion() && src0->asSrcRegRegion()->getRegAccess() == Direct &&
                dst->isAreg() && dst->isA0() &&
                src0->isAreg() && src0->asSrcRegRegion()->isA0() &&
                src1==NULL)
            {
                continue;
            }

            if (inst->opcode() == G4_add && inst->getExecSize() == 1 &&
                inst->getPredicate() == NULL &&
                (src1->isImm() && !src1->isRelocImm()) &&
                dst && dst->getRegAccess() == Direct &&
                src0 && src0->isSrcRegRegion() && src0->asSrcRegRegion()->getRegAccess() == Direct &&
                dst->isAreg() && dst->isA0() &&
                src0->isAreg() && src0->asSrcRegRegion()->isA0() &&
                dst_subReg == src0_subReg )
            {
                // since there is use of a0.x here, we can not remove the former def of a0.x
                // reverse immed offset propagation
                reverseOffsetProp( addrRegInfo, dst_subReg, 0, ii, iend );

                int64_t offset = src1->asImm()->getImm();
                if( offset >= -512 && offset <= 511 && offset%0x20 == 0)
                {
                    // this kills the previous def on a0.x
                    if( addrRegInfo[dst_subReg].canRemoveInst && addrRegInfo[dst_subReg].iter != iend )
                    {
                        // mark dead
                        (*(addrRegInfo[dst_subReg].iter))->markDead();
                    }
                    addrRegInfo[dst_subReg].subReg = dst_subReg;
                    addrRegInfo[dst_subReg].immAddrOff = (short)offset;
                    addrRegInfo[dst_subReg].iter = ii;
                    addrRegInfo[dst_subReg].canRemoveInst = true;
                    addrRegInfo[dst_subReg].canUseImmed = true;
                    addrRegInfo[dst_subReg].usedImmed = false;
                }
            }
            else
            {
                G4_Operand *src;
                // if there is any direct use of addr reg, the ADD inst can not be removed
                for (unsigned i = 0; i < num_srcs; i++)
                {
                    src = inst->getSrc(i);
                    if( src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == Direct &&
                        src->asSrcRegRegion()->isAreg() && src->asSrcRegRegion()->isA0())
                    {
                        // TODO: show if an inst is generated for spill code
                        // if there is no regVar for this srcRegion, the physical register is hard-wired in input or generated by spillCode.
                        // in this case, the subregister info is in the subRegOff of G4_SrcRegRegion
                        // this also applies to dst register
                        int subReg = getSrcSubReg( src );

                        // it is possible that several elements are used
                        int width, hstride, vstride, outerloop = 1;
                        width = src->asSrcRegRegion()->getRegion()->width;
                        hstride = src->asSrcRegRegion()->getRegion()->horzStride;
                        vstride = src->asSrcRegRegion()->getRegion()->vertStride;
                        if( vstride != 0 )
                        {
                            outerloop = inst->getExecSize() / vstride;
                        }

                        for( int k = 0; k < outerloop; k++ )
                        {
                            for( int j = 0; j < width; j++ )
                            {
                                int currSubreg = subReg + k * vstride + j * hstride;
                                // there may be inst whose src or dst addr immediate offset are already changed
                                // reverse the change
                                reverseOffsetProp( addrRegInfo, currSubreg, i, ii, iend );
                            }
                        }
                    }
                }
                // use of address register in index region
                for(unsigned i = 0; i < num_srcs; i++)
                {
                    src = inst->getSrc(i);
                    if( src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() != Direct)
                    {
                        int subReg = getSrcSubReg( src );

                        // if VxH is used and more than one sub registers are used in addressing
                        // do not fold the immediate even though they have the same immediate value
                        unsigned short vertStride = src->asSrcRegRegion()->getRegion()->vertStride;
                        if( vertStride == UNDEFINED_SHORT ||
                            (vertStride > 0 && (unsigned short)inst->getExecSize() / vertStride > 1 ))
                        {
                            int numSubReg = 0;
                            if( vertStride == UNDEFINED_SHORT )
                            {
                                numSubReg = inst->getExecSize()/src->asSrcRegRegion()->getRegion()->width;
                            }
                            else
                            {
                                numSubReg = 1; //inst->getExecSize()/vertStride;
                            }
                            for( int j = subReg; j < subReg + numSubReg; j++ )
                            {
                                reverseOffsetProp( addrRegInfo, j, i, ii, iend );
                            }
                        }
                        else
                        {
                            // we check the existing address reg imm offset.
                            short currOff = src->asSrcRegRegion()->getAddrImm();
                            if( addrRegInfo[subReg].canUseImmed )
                            {
                                if( currOff % 0x20 == 0 &&
                                    (currOff + addrRegInfo[subReg].immAddrOff) <= 511 &&
                                    (currOff + addrRegInfo[subReg].immAddrOff) >= -512)
                                {
                                    G4_SrcRegRegion tmpRgn(*src->asSrcRegRegion());
                                    G4_SrcRegRegion* newSrc = &tmpRgn;
                                    newSrc->setImmAddrOff( short(currOff + addrRegInfo[subReg].immAddrOff) );
                                    inst->setSrc( builder.createSrcRegRegion(*newSrc), i );

                                    addrRegInfo[subReg].usedImmed = true;
                                }
                                else
                                {
                                    // if the offset can not be folded into all uses of a0.0, reverse the former folding
                                    reverseOffsetProp( addrRegInfo, subReg, i, ii, iend );
                                }
                            }
                        }
                    }
                }
                if (dst)
                {
                    // make sure the addr reg is not redefined
                    // direct access to a0.x
                    if( dst->isAreg() && dst->getRegAccess() == Direct && dst->isA0() )
                    {
                        int width, hstride;
                        width = inst->getExecSize();
                        hstride = dst->getHorzStride();

                        for( int j = 0; j < width; j++ )
                        {
                            int currSubreg = dst_subReg + j * hstride;
                            // this kills the previous def on a0.x
                            if( addrRegInfo[currSubreg].iter != iend && addrRegInfo[currSubreg].canRemoveInst )
                            {
                                // mark dead
                                (*(addrRegInfo[currSubreg].iter))->markDead();
                            }
                            addrRegInfo[currSubreg].immAddrOff = 0;
                            addrRegInfo[currSubreg].iter = iend;
                            addrRegInfo[currSubreg].canRemoveInst = false;
                            addrRegInfo[currSubreg].canUseImmed = false;
                            addrRegInfo[currSubreg].usedImmed = false;
                        }
                    }
                    // check if dst is indirectly addressed
                    else if( dst->getRegAccess() != Direct )
                    {
                        short currOff = dst->getAddrImm();
                        if( addrRegInfo[dst_subReg].canUseImmed )
                        {
                            if( currOff % 0x20 == 0 &&
                                (currOff + addrRegInfo[dst_subReg].immAddrOff) <= 511 &&
                                (currOff + addrRegInfo[dst_subReg].immAddrOff) >= -512 )
                            {
                                // create a new dst
                                G4_DstRegRegion tmpRgn( *dst );
                                G4_DstRegRegion *newDst = &tmpRgn;
                                newDst->setImmAddrOff( short(currOff + addrRegInfo[dst_subReg].immAddrOff) );
                                inst->setDest( builder.createDstRegRegion( *newDst ) );
                                addrRegInfo[dst_subReg].usedImmed = true;
                            }
                            else
                            {
                                // if the offset can not be folded into all uses of a0.0, reverse the former folding
                                reverseOffsetProp( addrRegInfo, dst_subReg, 0, ii, iend );
                            }
                        }
                    }
                }
            }
        }

        // if a def lives out of this BB, we can not delete the defining inst
        for (unsigned i = 0; i < getNumAddrRegisters(); i++)
        {
            // reverse immed offset propagation
            reverseOffsetProp( addrRegInfo, i, 0, iend, iend );
        }
        // remove the ADD instructions that marked as dead
        for(ii = bb->begin(); ii != bb->end();)
        {
            G4_INST *inst = *ii;
            INST_LIST_ITER curr = ii++;
            if( inst->isDead() )
            {
                bb->erase(curr);
            }
        }
    }

    delete [] addrRegInfo;
}
G4_SrcModifier Optimizer::mergeModifier( G4_Operand *src, G4_Operand *use )
{
    if( ( src == NULL || !src->isSrcRegRegion() )  && use && use->isSrcRegRegion() ){
        return use->asSrcRegRegion()->getModifier();
    }else if( ( use == NULL || !use->isSrcRegRegion() ) && src && src->isSrcRegRegion() ){
        return src->asSrcRegRegion()->getModifier();
    }else if( src && src->isSrcRegRegion() && use && use->isSrcRegRegion() ){
        G4_SrcModifier mod1 = src->asSrcRegRegion()->getModifier(), mod2 = use->asSrcRegRegion()->getModifier();
        if( mod2 == Mod_Abs || mod2 == Mod_Minus_Abs ){
            return mod2;
        }else if( mod2 == Mod_src_undef ){
            return mod1;
        }else{
            // mod2 == Minus
            if( mod1 == Mod_Minus ){
                return Mod_src_undef;
            }else if( mod1 == Mod_Abs ){
                return Mod_Minus_Abs;
            }else if( mod1 == Mod_Minus_Abs ){
                return Mod_Abs;
            }else{
                return mod2;
            }
        }
    }else{
        return Mod_src_undef;
    }
}

// Prevent sinking in presence of lifetime.end for any src op.
// For eg,
// add V33, V32, 0x1
// ...
// lifetime.end V32 <-- This prevents sinking of add to mov
// ...
// pseudo_kill V34 <-- This prevents hoisting of V34 to add dst
// mov V34, V33
//
static bool checkLifetime(G4_INST *defInst, G4_INST *inst)
{
    // Check whether current instruction ends any src opnd of op
    if (!inst->isLifeTimeEnd())
        return true;

    G4_RegVar *Var = GetTopDclFromRegRegion(inst->getSrc(0))->getRegVar();
    // Check whether lifetime op corresponds to any operand of current inst.
    if (defInst->getPredicate())
    {
        G4_RegVar *opndVar = defInst->getPredicate()->asPredicate()->getBase()->asRegVar();
        if (opndVar == Var)
            return false;
    }
    if (defInst->getCondMod())
    {
        G4_RegVar *opndVar = defInst->getCondMod()->asCondMod()->getBase()->asRegVar();
        if (opndVar == Var)
            return false;
    }
    if (defInst->getDst() &&
        !defInst->getDst()->isNullReg())
    {
        G4_RegVar *opndVar = GetTopDclFromRegRegion(defInst->getDst())->getRegVar();
        if (opndVar == Var)
            return false;
    }
    for (unsigned int srcOpnd = 0; srcOpnd < G4_MAX_SRCS; srcOpnd++)
    {
        G4_Operand *src = defInst->getSrc(srcOpnd);
        if (src && src->isSrcRegRegion())
        {
            G4_RegVar *opndVar = GetTopDclFromRegRegion(src)->getRegVar();
            if (opndVar == Var)
                return false;
        }
    }

    return true;
}

//
// Sink definition towards its use.
//
// For example, without sinking the def instruction once, use cannot
// be hoisted, since there is a data dependency between the middle
// instruction and the last move.
//
// def: shr (1) V39(0,0)<1>:ud V38(0,0)<0;1,0>:d 0x4:w {Align1, Q1}
//      mov (8) V68(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
// use: mov (1) V68(0,2)<1>:ud V39(0,0)<0;1,0>:ud {Align1, NoMask}
//
// after sinking, it becomes
//
//      mov (8) V68(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
// def: shr (1) V39(0,0)<1>:ud V38(0,0)<0;1,0>:d 0x4:w {Align1, Q1}
// use: mov (1) V68(0,2)<1>:ud V39(0,0)<0;1,0>:ud {Align1, NoMask}
//
// which makes local def hoisting possible.
//
// The third argument 'other' points to the first instruction (upwards) that
// has data-dependency with the use instruction.
//
static bool canSink(G4_BB *bb, INST_LIST_RITER revIter, INST_LIST_RITER other)
{
    // The use instruction.
    G4_INST *inst = *revIter;

    // Currently we do not handle multiple definition for this optimization.
    if (inst->def_size() != 1)
        return false;

    // Find its def instruction.
    G4_INST *defInst = inst->def_back().first;

    ASSERT_USER(*other != defInst, "iterator points to def already");

    // Walk up to check if sinking is safe.
    INST_LIST_RITER it = other;

    while (*it != defInst)
    {
       if ((*it)->isWAWdep(defInst) ||
           (*it)->isRAWdep(defInst) ||
           (*it)->isWARdep(defInst))
           return false;

       if (!checkLifetime(defInst, *it))
           return false;

       // move towards to defInst.
       ++it;
    }

    // At this point, there is no data dependency and sinking is safe.
    //
    // We do sinking right here.
    //
    ASSERT_USER(*it == defInst, "iterator out of sync");

    // Both 'other' and 'it' are reverse iterators, and sinking is through
    // forward iterators. The fisrt base should not be decremented by 1,
    // otherwise, the instruction will be inserted before not after.
    bb->insert(other.base(), defInst);
    bb->erase(--it.base());

    return true;
}

static bool canHoist(FlowGraph &fg, G4_BB *bb, INST_LIST_RITER revIter)
{
    G4_INST *inst = *revIter;

    if (inst->isMixedMode() && fg.builder->getOption(vISA_DisableleHFOpt))
        return false;
    // Cannot hoist if this is not a move, or it is a global operand.
    if  (inst->opcode() != G4_mov ||
         fg.globalOpndHT.isOpndGlobal(inst->getSrc(0)) ||
         !inst->canHoist(!bb->isAllLaneActive(), fg.builder->getOptions()))
    {
         return false;
    }

    // Do not do def-hoisting for seeting flags which is likely to increase flag
    // register pressure.
    if (auto Dst = inst->getDst())
    {
        G4_Declare *Dcl = Dst->getTopDcl();
        if (Dcl && Dcl->getRegFile() == G4_RegFileKind::G4_FLAG)
        {
            return false;
        }
    }

    // Now check each definition of src(0)
    for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I)
    {
        ASSERT_USER(I->second == Opnd_src0, "invalid use-def chain");
        if (!inst->canHoistTo(I->first, !bb->isAllLaneActive()))
            return false;

        auto defInst = I->first;
        if (fg.globalOpndHT.isOpndGlobal(defInst->getDst()))
        {
            return false;
        }

        // Further check data-dependency, that is, no other instruction
        // should have no WAR or WAW dependency.
        //
        //   defInst
        //
        //   other inst
        //
        //   inst <-- revIter
        //
        INST_LIST_RITER other = revIter;
        ++other;

        // Measure the distance in between
        unsigned distance = 0;

        // Walkup until hits its defining instruction.
        while (*other != I->first)
        {
            // FIXME: remove duplicate computations for multiple definitions.
            if (inst->isWAWdep(*other) || inst->isWARdep(*other))
            {
                break;
            }
            ++other;
            ++distance;
        }

        // Check the distance first, if this is too far then the following
        // sinking optimization is very expensive.
        #define MAX_DEF_HOIST_DIST 160
        if (distance > MAX_DEF_HOIST_DIST)
            return false;

        // There is a data dependency.
        if (*other != I->first)
        {
            // check if sinking is possible.
            if (!canSink(bb, revIter, other))
                return false;
        }
    }
    return true;
}

static G4_DstRegRegion *buildNewDstOperand(FlowGraph &fg, G4_INST *inst, G4_INST *defInst)
{
    G4_Operand *src = inst->getSrc(0);
    G4_DstRegRegion *dst = inst->getDst();

    G4_Type srcType = src->getType();
    G4_Type dstType = dst->getType();
    G4_DstRegRegion *dstRegion = dst;
    bool indirectDst = (dstRegion->getRegAccess() != Direct);
    unsigned char srcElSize = (unsigned char)G4_Type_Table[srcType].byteSize;

    G4_DstRegRegion *defDstRegion = defInst->getDst();
    G4_DstRegRegion *newDstOpnd = dst;

    unsigned char defDstElSize = (unsigned char)G4_Type_Table[defDstRegion->getType()].byteSize;
    G4_CmpRelation rel = src->compareOperand(defDstRegion);
    G4_Type defDstType = defDstRegion->getType();

    unsigned char dstElSize = (unsigned char)G4_Type_Table[dstType].byteSize;
    unsigned short dstHS = dst->getHorzStride();

    if (rel == Rel_gt || srcElSize != defDstElSize ||
        (defInst->getSaturate() && srcType != defDstType) || inst->isRawMov() ||
        (dstType != defDstType &&
         (IS_FTYPE(defDstType) || (IS_FTYPE(dstType) && defDstType != srcType))))
    {
        unsigned short regOff = 0, subRegOff = 0;
        if (rel == Rel_gt)
        {
            // compute new dst for defInst
            // get dst portion based on src region
            unsigned defDstLB = defDstRegion->getLeftBound();

            unsigned srcLB = src->getLeftBound();
            const RegionDesc *srcRegionDesc = src->asSrcRegRegion()->getRegion();
            bool contRegion = srcRegionDesc->isContiguous(inst->getExecSize());

            uint32_t dist = defDstLB - srcLB, dstDist = 0, tempLen = 0;
            if (src->asSrcRegRegion()->isScalar() || contRegion)
            {
                // mov (1) V18(0,0)[1]:b 0x73:w [Align1]
                // mov (1) V18(0,1)[1]:b 0x61:w [Align1]
                // mov (1) V18(0,2)[1]:b 0x70:w [Align1]
                // mov (1) V18(0,3)[1]:b 0:w [Align1]
                // mov (1) V20(1,0)[1]:ud V18(0,0)[0;1,0]:ud [Align1]
                // length of subregoff part
                tempLen = dstRegion->getSubRegOff() * dstElSize + dist * dstHS;

                if (tempLen >= GENX_GRF_REG_SIZ)
                {
                    regOff = dst->getRegOff() + 1;
                    subRegOff = (unsigned short)((tempLen - GENX_GRF_REG_SIZ) / defDstElSize);
                }
                else
                {
                    regOff = dst->getRegOff();
                    subRegOff = (unsigned short)tempLen / defDstElSize;
                }
            }
            else
            {
                // mov (16) V18(0,0)[1]:b 0x73:w [Align1]
                // mov (16) V18(0,16)[1]:b 0x61:w [Align1]
                // mov (16) V18(1,0)[1]:b 0x70:w [Align1]
                // mov (16) V18(1,16)[1]:b 0:w [Align1]
                // mov (32) V20(1,0)[1]:b V18(0,0)[32;16,1]:b [Align1]
                // mov (32) V20(2,0)[1]:b V18(0,16)[32;16,1]:b [Align1]

                // Compute the linear index of the first element from defInst's dst
                // in useInst's src.
                //
                // mov <2> V50(0, 14)<1>:b 0xa:w                  <- defInst
                // mov <16> V51(0, 9)<2>:b V50(0, 0)<0; 16, 2>:b  <- useInst
                //
                // Starting from left bound difference, dist = 14.
                //
                // FirstEltIndex is 7 = 14 / 2. With this index, we can compute
                // the register offset and sub-register offset in useInst's dst.
                //
                // In the above example, there is only a single row. In general
                // there may have multiple rows in useInst's src region.
                //
                // (1) convert difference in number of elements.
                MUST_BE_TRUE(dist % srcElSize == 0, "unexpected difference");
                dist = dist / srcElSize;

                // (2) compute row and column index, by default a single row.
                unsigned rowIndex = 0, colIndex = dist;
                if (srcRegionDesc->vertStride > 0)
                {
                    rowIndex = dist / srcRegionDesc->vertStride;
                    colIndex = dist % srcRegionDesc->vertStride;
                }

                // (3) compute the final linear index.
                MUST_BE_TRUE(srcRegionDesc->horzStride == 0 || colIndex % srcRegionDesc->horzStride == 0, "invalid region");
                unsigned FirstEltIndex = rowIndex * srcRegionDesc->width +
                                         (srcRegionDesc->horzStride == 0 ? colIndex : (colIndex / srcRegionDesc->horzStride));

                // (4) compute the register and subregister offet in useInst's dst.
                dstDist = FirstEltIndex * dstElSize * dstHS;
                tempLen = dstDist + dst->getSubRegOff() * dstElSize;
                regOff = (unsigned short)(dst->getRegOff() +
                    tempLen / GENX_GRF_REG_SIZ);

                subRegOff = (unsigned short)(tempLen % GENX_GRF_REG_SIZ) / defDstElSize;
            }

            unsigned short defDstHS = defDstRegion->getHorzStride();
            newDstOpnd = fg.builder->createDstRegRegion(
                dst->getRegAccess(),
                dst->getBase(),
                indirectDst ? dst->getRegOff() : regOff,
                indirectDst ? dst->getSubRegOff() : subRegOff,
                dstHS * defDstHS, defDstRegion->getType());
            newDstOpnd->setImmAddrOff(dst->getAddrImm() +
                (indirectDst ? (short)tempLen : 0));
        }
        else
        {
            unsigned char scale = dstElSize / defDstElSize;

            // If instruction that gets def hoisted is just re-interpretation of bits,
            // doesn't do conversion, then I think original type of the defInst
            // should be used. This preserves the original behavior.
            //
            // mov (8) V57(0,0)[1]:hf V52(0,0)[8;8,1]:f [Align1, Q1] %21
            // mov (8) V58(0,0)[1]:w  V59(0,0)[8;8,1]:w [Align1, Q1] %22
            if (dst->getType() == src->getType())
            {
                newDstOpnd = fg.builder->createDstRegRegion(
                    dst->getRegAccess(),
                    dst->getBase(),
                    dst->getRegOff(),
                    indirectDst
                    ? dst->getSubRegOff()
                    : (scale == 0
                    ? dst->getSubRegOff() /
                    (defDstElSize / dstElSize)
                    : dst->getSubRegOff() * scale),
                    dstHS, defDstRegion->getType());
                newDstOpnd->setImmAddrOff(dst->getAddrImm());
            }
            else
            {
                newDstOpnd = fg.builder->createDstRegRegion(dst->getRegAccess(),
                    dst->getBase(),
                    dst->getRegOff(),
                    dst->getSubRegOff(), dstHS,
                    dst->getType());
                newDstOpnd->setImmAddrOff(dst->getAddrImm());

            }
        }
    }
    return newDstOpnd;
}

//
//  inst0:   op0  rx<2>(0), ry0, ry1
//  inst1:   op1  rx<2>(1), ry2, ry3
//  inst2:   mov  rz<1>(0), rx<0, 0>(8; 8, 1)
//
// ==>
//
//  inst0:   op0, rz<2>(0), ry0, ry1
//  inst1:   op1, rz<2>(1), ry2, ry3
//  inst2:   mov  rz<1>(0), rx<0, 0>(8; 8, 1)  (to be deleted)
//
// Def-use/use-def chains will be updated as follows:
//
// (0) all use-defs remain the same for inst0 and inst1.
//
// (1) remove all use-defs of inst2. (They must be from inst0 and inst1,
//     which is the pre-condition of doHoisting.)
//
// (2) remove all def-uses of inst0 and inst1 from dst.
//
// (3) remove all def-uses of inst2.
//
// (4) add new def-uses to inst0 and inst1.
//
static void doHoisting(FlowGraph &fg, G4_BB *bb, INST_LIST_RITER revIter)
{
    G4_INST *inst = *revIter;

    for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I)
    {
        G4_INST *defInst = I->first;

        // Build a new dst operand for each def instruction.
        G4_DstRegRegion *newDst = buildNewDstOperand(fg, inst, defInst);

        // Update the defInst with a new operand and set attributes properly.
        if (inst->def_size() == 1)
        {
            defInst->setDest(newDst);
        }
        else
        {
            defInst->setDest(fg.builder->duplicateOperand(newDst)->asDstRegRegion());
        }

        // (4) for each def-use of inst, add it to defInst, if it is
        // an effective use.
        for (auto UI = inst->use_begin(), UE = inst->use_end(); UI != UE; ++UI)
        {
            G4_Operand *UseOpnd = UI->first->getOperand(UI->second);
            // this comparison is necessary, since uses of inst's dst may be
            // different from those from defInst's dst.
            G4_CmpRelation rel = defInst->getDst()->compareOperand(UseOpnd);
            if (rel != Rel_disjoint)
            {
                defInst->addDefUse(UI->first, UI->second);
            }
        }

        if (inst->getPredicate())
        {
            ASSERT_USER(inst->def_size() == 1, "multiple defs not implemented");
            // Remove existing definitions on defInst[opnd_pred].
            defInst->removeDefUse(Opnd_pred);

            defInst->setPredicate(inst->getPredicate());

            // (4) Transfer definitions of inst[opnd_pred] to definitions of
            // defInst[opnd_pred].
            inst->transferDef(defInst, Opnd_pred, Opnd_pred);
        }
        if (inst->getSrc(0)->asSrcRegRegion()->isScalar() &&
            inst->getExecSize() > 1)
        {
            defInst->setExecSize(defInst->getExecSize() * inst->getExecSize());
        }
        defInst->setSaturate(inst->getSaturate() || defInst->getSaturate());
        if (!bb->isAllLaneActive())
        {
            // set writeEnable of dstInst to be off
            defInst->setOptions((defInst->getOption() & ~0xFFF000C) |
                                (inst->getMaskOption()));
        }
    }

    // (1), (2), (3) Remove all defs/uses and it is ready to be deleted.
    inst->removeAllDefs();
    inst->removeAllUses();
}

void Optimizer::newLocalDefHoisting()
{
    unsigned numDefHoisted = 0;
    for (auto bb : fg)
    {
        for (auto I = bb->rbegin(); I != bb->rend(); /* empty */)
        {
            if (canHoist(fg, bb, I))
            {
                doHoisting(fg, bb, I);
                ++numDefHoisted;

                // list::erase does not take a reverse_iterator.
                //
                // The base iterator is an iterator of the same type as the one
                // used to construct the reverse_iterator, but pointing to the
                // element next to the one that the reverse_iterator is currently
                // pointing to (a reverse_iterator has always an offset of -1
                // with respect to its base iterator).
                I = INST_LIST::reverse_iterator(bb->erase(--I.base()));
            }
            else
            {
                ++I;
            }
        }
    }

    if (builder.getOption(vISA_OptReport))
    {
        std::ofstream optReport;
        getOptReportStream(optReport, builder.getOptions());
        optReport << "             === Local Definition Hoisting Optimization ===\n";
        optReport << "Number of defs hoisted: " << numDefHoisted << "\n";
        closeOptReportStream(optReport);
    }
}

//  find a common (integer) type for constant folding.  The rules are:
//  -- both types must be int
//  -- Q and UQ are not folded
//  -- UD if one of the type is UD
//  -- D otherwise
//
//  returns Type_UNDEF if no appropriate type can be found
//
static G4_Type findConstFoldCommonType( G4_Type type1, G4_Type type2 )
{
    if (IS_TYPE_INT(type1) && IS_TYPE_INT(type2))
    {
        if (G4_Type_Table[type1].byteSize == 8 || G4_Type_Table[type2].byteSize == 8)
        {
            return Type_UNDEF;
        }
        if (type1 == Type_UD || type2 == Type_UD)
        {
            return Type_UD;
        }
        else
        {
            return Type_D;
        }
    }
    return Type_UNDEF;
}

// Do the following algebraic simplification:
// - mul v, src0, 0 ==> 0, commutative
// - and v, src0, 0 ==> 0, commutative
// - mul v, src0, 1 ==> src0, commutative
// - shl v, src0, 0 ==> src0
// - shr v, src0, 0 ==> src0
// - asr v, src0, 0 ==> src0
// - add v, src0, 0 ==> src0, commutative
void Optimizer::doSimplification(G4_INST *inst)
{
    // Just handle following commonly used ops for now.
    if (inst->opcode() != G4_mul && inst->opcode() != G4_and &&
        inst->opcode() != G4_add && inst->opcode() != G4_shl &&
        inst->opcode() != G4_shr && inst->opcode() != G4_asr &&
        inst->opcode() != G4_mov)
    {
        return;
    }


    // Perform 'mov' to 'movi' transform when it's a 'mov' of
    // - simd8
    // - it's a raw mov
    // - dst is within a single GRF.
    // - src uses VxH indirect access.
    // - src is within one GRF.
    // - indices to src are all within src.
    if (inst->opcode() == G4_mov && inst->getExecSize() == 8 &&
        inst->isRawMov() && inst->getDst() &&
        !inst->getDst()->asDstRegRegion()->isCrossGRFDst() &&
        inst->getSrc(0) && inst->getSrc(0)->isSrcRegRegion() &&
        inst->getSrc(0)->asSrcRegRegion()->isIndirect() &&
        inst->getSrc(0)->asSrcRegRegion()->getRegion()->isRegionWH() &&
        inst->getSrc(0)->asSrcRegRegion()->getRegion()->width == 1 &&
        // destination stride in bytes must be equal to the source element size
        // in bytes.
        getTypeSize(inst->getSrc(0)->getType()) ==
        getTypeSize(inst->getDst()->getType()) *
        inst->getDst()->asDstRegRegion()->getHorzStride()) {
        // Convert 'mov' to 'movi' if the following conditions are met.

        auto getSingleDefInst = [](G4_INST *UI,
                                   Gen4_Operand_Number OpndNum)
            -> G4_INST * {
            G4_INST *Def = nullptr;
            for (auto I = UI->def_begin(), E = UI->def_end(); I != E; ++I) {
                if (I->second != OpndNum)
                    continue;
                if (Def) {
                    // Not single defined, bail out
                    Def = nullptr;
                    break;
                }
                Def = I->first;
            }
            return Def;
        };

        unsigned SrcSizeInBytes = inst->getExecSize() *
                getTypeSize(inst->getSrc(0)->getType());
        if (SrcSizeInBytes == G4_GRF_REG_NBYTES/2 ||
            SrcSizeInBytes == G4_GRF_REG_NBYTES)
        {
            G4_INST *LEA = getSingleDefInst(inst, Opnd_src0);
            if (LEA && LEA->opcode() == G4_add &&
                LEA->getExecSize() == inst->getExecSize()) {
                G4_Operand *Op0 = LEA->getSrc(0);
                G4_Operand *Op1 = LEA->getSrc(1);
                G4_Declare *Dcl = nullptr;
                int Offset = 0;
                if (Op0->isAddrExp()) {
                    G4_AddrExp *AE = Op0->asAddrExp();
                    Dcl = AE->getRegVar()->getDeclare();
                    Offset = AE->getOffset();
                }
                if (Dcl && (Offset % SrcSizeInBytes) == 0 &&
                    Op1->isImm() && Op1->getType() == Type_UV) {
                    // Immeidates in 'uv' ensures each element is a
                    // byte-offset within half-GRF.
                    G4_SubReg_Align SubAlign = GRFALIGN;
                    if (SrcSizeInBytes <= G4_GRF_REG_NBYTES/2u)
                        SubAlign = (G4_SubReg_Align)(NUM_WORDS_PER_GRF/2);
                    inst->setOpcode(G4_movi);
                    if (!Dcl->isEvenAlign() && Dcl->getSubRegAlign() != GRFALIGN)
                    {
                        Dcl->setSubRegAlign(SubAlign);
                    }
                    const RegionDesc *rd = builder.getRegionStride1();
                    inst->getSrc(0)->asSrcRegRegion()->setRegion(rd);
                    // Set subreg alignment for the address variable.
                    Dcl =
                        LEA->getDst()->getBase()->asRegVar()->getDeclare();
                    assert(Dcl->getRegFile() == G4_ADDRESS &&
                           "Address variable is required.");
                    Dcl->setSubRegAlign(Eight_Word);
                }
            }
        }
    }

    auto isInteger = [](G4_Operand *opnd, int64_t val)
    {
        if (opnd && IS_TYPE_INT(opnd->getType()) && !opnd->isRelocImm())
        {
            return opnd->isImm() && opnd->asImm()->getInt() == val;
        }
        return false;
    };

    G4_Operand *src0 = inst->getSrc(0);
    G4_Operand *src1 = inst->getSrc(1);
    G4_Operand *newSrc = nullptr;
    if (inst->opcode() == G4_mul || inst->opcode() == G4_and)
    {
        if (isInteger(src1, 0))
        {
            inst->removeDefUse(Opnd_src0);
            newSrc = builder.createImm(0, Type_W);
        }
        else if (isInteger(src0, 0))
        {
            inst->removeDefUse(Opnd_src1);
            newSrc = builder.createImm(0, Type_W);
        }
        else if (inst->opcode() == G4_mul)
        {
            if (isInteger(src1, 1))
            {
                newSrc = src0;
            }
            else if (isInteger(src0, 1))
            {
                inst->swapDefUse();
                newSrc = src1;
            }
        }
    }
    else if (inst->opcode() == G4_shl || inst->opcode() == G4_shr ||
             inst->opcode() == G4_asr || inst->opcode() == G4_add)
    {
        if (isInteger(src1, 0))
        {
            newSrc = src0;
        }
        else if (inst->opcode() == G4_add && isInteger(src0, 0))
        {
            inst->swapDefUse();
            newSrc = src1;
        }
    }

    if (newSrc != nullptr)
    {
        inst->setOpcode(G4_mov);
        if (newSrc != src0)
        {
            inst->setSrc(newSrc, 0);
        }
        inst->setSrc(nullptr, 1);
    }
}

//
// Do very simple const only reassociation to fold const values
// e.g.,
// V2 = V1 + K1
// V3 = V2 + K2
// -->
// V3 = V1 + (K1 + K2)
// we only search one level (+ and +, * and *) for now as more complex reassociation should be taken
// care of by IGC earlier.
// also only do it for integer type for now
//
void Optimizer::reassociateConst()
{
    for (auto BB : fg)
    {
        for (auto iter = BB->begin(), iterEnd = BB->end(); iter != iterEnd; ++iter)
        {
            G4_INST* inst = *iter;
            if (inst->opcode() != G4_add && inst->opcode() != G4_mul)
            {
                continue;
            }
            auto isSrc1Const = [](G4_INST* inst)
            {
                if (!IS_INT(inst->getDst()->getType()))
                {
                    return false;
                }
                if (!inst->getSrc(0)->isImm() && inst->getSrc(1)->isImm())
                {
                    return true;
                }
                else if (inst->getSrc(0)->isImm() && !inst->getSrc(1)->isImm())
                {
                    inst->swapSrc(0, 1);
                    inst->swapDefUse(); // swap def/use for src0 and src1
                    return true;
                }
                return false;
            };
            if (!isSrc1Const(inst))
            {
                continue;
            }
            auto src0Def = inst->getSingleDef(Opnd_src0);
            if (!src0Def)
            {
                continue;
            }

            auto isGoodSrc0Def = [isSrc1Const](G4_INST* def, G4_INST* use)
            {
                assert(use->getSrc(0)->isSrcRegRegion() && "expect src0 to be src region");
                if (def->opcode() != use->opcode())
                {
                    return false;
                }
                if (def->getSaturate() || def->getPredicate() || def->getCondMod() ||
                    def->getMaskOffset() != use->getMaskOffset())
                {
                    return false;
                }
                if (!isSrc1Const(def))
                {
                    return false;
                }
                auto useSrc = use->getSrc(0)->asSrcRegRegion();
                if (useSrc->hasModifier() || getTypeSize(def->getDst()->getType()) != getTypeSize(useSrc->getType()) ||
                    def->getDst()->compareOperand(useSrc) != Rel_eq)
                {
                    // make sure def fully defines use and have the same integer type size (signed-ness should not matter)
                    return false;
                }
                if (def->getDst()->compareOperand(def->getSrc(0)) != Rel_disjoint)
                {
                    // can't sink source if def overwrites it
                    return false;
                }
                // additionally check for the use inst that dst type size is >= src type size
                // otherwise the first add may truncate upper bits due to overflow,
                // which makes reassociation unsafe
                if (getTypeSize(useSrc->getType()) < getTypeSize(use->getDst()->getType()))
                {
                    return false;
                }

                return true;
            };

            if (isGoodSrc0Def(src0Def, inst) && !chkFwdOutputHazard(src0Def, iter))
            {
                //std::cout << "reassociate: \n";
                //src0Def->dump();
                //inst->dump();
                G4_Imm* constOne = src0Def->getSrc(1)->asImm();
                G4_Imm* constTwo = inst->getSrc(1)->asImm();
                G4_Imm* resultImm = foldConstVal(constOne, constTwo, inst->opcode());

                if (resultImm)
                {
                    inst->setSrc(builder.duplicateOperand(src0Def->getSrc(0)), 0);
                    inst->setSrc(resultImm, 1);
                    inst->removeDefUse(Opnd_src0);
                    src0Def->copyDef(inst, Opnd_src0, Opnd_src0);
                    //ToDo: remove this when DCE pass is enabled
                    if (src0Def->use_size() == 0 && !fg.globalOpndHT.isOpndGlobal(src0Def->getDst()) &&
                        !src0Def->getDst()->isIndirect())
                    {
                        src0Def->markDead();
                        src0Def->removeAllDefs();
                    }
                    //std::cout << "--> new inst:\t";
                    //inst->dump();
                }
            }
        }
        BB->erase(std::remove_if(BB->begin(), BB->end(), [](G4_INST* inst) { return inst->isDead(); }),
            BB->end());
    }

}

// helper function to fold BinOp with two immediate operands
// supported opcodes are given below in doConsFolding
// returns nullptr if the two constants may not be folded
G4_Imm* Optimizer::foldConstVal(G4_Imm* const1, G4_Imm* const2, G4_opcode op)
{
    bool isNonQInt = IS_TYPE_INT(const1->getType()) && IS_TYPE_INT(const2->getType()) &&
        !IS_QTYPE(const1->getType()) && !IS_QTYPE(const2->getType());

    if (!isNonQInt)
    {
        return nullptr;
    }

    G4_Type src0T = const1->getType(), src1T = const2->getType(), resultType = src0T;

    if (op == G4_mul || op == G4_add || op == G4_and || op == G4_xor || op == G4_or)
    {
        resultType = findConstFoldCommonType(src0T, src1T);
        if (resultType == Type_UNDEF)
        {
            return nullptr;
        }

        int64_t res;
        switch (op)
        {
        case G4_and:
            res = (int64_t)(const1->getInt()) & (int64_t)(const2->getInt());
            break;

        case G4_xor:
            res = (int64_t)(const1->getInt()) ^ (int64_t)(const2->getInt());
            break;

        case G4_or:
            res = (int64_t)(const1->getInt()) | (int64_t)(const2->getInt());
            break;

        case G4_add:
            res = (int64_t)(const1->getInt()) + (int64_t)(const2->getInt());
            break;

        case G4_mul:
            res = (int64_t)(const1->getInt()) * (int64_t)(const2->getInt());
            break;

        default:
            return nullptr;
        }

        // result type is either D or UD
        // don't fold if the value overflows D/UD
        if (!G4_Imm::isInTypeRange(res, resultType))
        {
            return nullptr;
        }
        return builder.createImmWithLowerType(res, resultType);
    }
    else
    {
        uint32_t shift = const2->getInt() % 32;

        if (op == G4_shl || op == G4_shr)
        {
            uint32_t value = (uint32_t)const1->getInt();
            // set result type to D/UD as it may overflow W. If the value fits the type will be lowered later
            // source type matters here since it affects sign extension
            resultType = IS_SIGNED_INT(resultType) ? Type_D : Type_UD;
            int64_t res = op == G4_shl ?
                ((int64_t)value) << shift :
                value >> shift;
            if (!G4_Imm::isInTypeRange(res, resultType))
            {
                return nullptr;
            }

            return builder.createImmWithLowerType(res, resultType);
        }
        else if (op == G4_asr)
        {
            if (IS_SIGNED_INT(resultType))
            {
                int64_t value = const1->getInt();
                int64_t res = value >> shift;
                return builder.createImmWithLowerType(res, resultType);
            }
            else
            {
                uint64_t value = const1->getInt();
                uint64_t res = value >> shift;
                return builder.createImmWithLowerType(res, resultType);
            }
        }
    }
    return nullptr;
}



// Currently constant folding is done for the following code patterns:
//
// - op v, imm, imm
//    where op is shl, shr, asr, or, xor, and, add, mul
// Restrictions:
// - operand type cannot be float or Q/UQ
// - saturation is not allowed
void Optimizer::doConsFolding( G4_INST *inst )
{
    G4_Operand *src0, *src1;

    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);

    G4_Imm *newSrc = nullptr;

    if (src0 && src0->isImm() && !src0->isRelocImm() &&
        src1 && src1->isImm() && !src1->isRelocImm() &&
        inst->getSaturate() == false)
    {
        newSrc = foldConstVal(src0->asImm(), src1->asImm(), inst->opcode());
    }

    if (newSrc != NULL)
    {
        // change instruction into a MOV
        inst->setOpcode( G4_mov );
        inst->setSrc( newSrc, 0 );
        inst->setSrc( NULL, 1 );
    }
}

static void hoistUseInst(G4_BB *bb, G4_INST *inst , INST_LIST_ITER forwardIter, bool &canRemove)
{
    // check if we can move the use inst up.
    // currently we do not handle multiple use for this optimization
    G4_INST* useInst = inst->use_front().first;
    if( inst->hasOneUse() )
    {
        forwardIter--;
        INST_LIST_ITER backwardIter = forwardIter;
        INST_LIST_ITER instListEnd = bb->end();
        while( backwardIter != instListEnd &&
               *backwardIter != useInst )
        {
            backwardIter++;
        }

        INST_LIST_ITER useInstIter = backwardIter;
        backwardIter--;
        while( backwardIter != forwardIter )
        {
            if( useInst->isWAWdep(*backwardIter) ||
                useInst->isRAWdep(*backwardIter) ||
                useInst->isWARdep(*backwardIter) )
            {
                break;
            }
            backwardIter--;
        }
        if( backwardIter != forwardIter )
        {
            canRemove = false;
        }
        else
        {
            // hoisting
            backwardIter++;
            bb->insert( backwardIter, useInst );
            bb->erase( useInstIter );
        }
    }
    else
    {
        canRemove = false;
    }
}

// conver modifier(imm)
template<class T>
static
typename std::enable_if<std::is_floating_point<T>::value, T>::type
getImmValue(T imm, G4_SrcModifier modifier)
{
    switch (modifier)
    {
    case Mod_Minus:
        return -imm;
    case Mod_Abs:
        return std::abs(imm);
    case Mod_Minus_Abs:
        return -(std::abs(imm));
    case Mod_Not:
        MUST_BE_TRUE(false, "unexpected not modifier for floating types");
        return imm;
    default:
        return imm;
    }
}

template<class T>
static
typename std::enable_if<std::is_integral<T>::value, T>::type
getImmValue(T imm, G4_SrcModifier modifier)
{
    switch (modifier)
    {
    case Mod_Minus:
        return -imm;
    case Mod_Abs:
        return std::llabs(imm);
    case Mod_Minus_Abs:
        return -(std::llabs(imm));
    case Mod_Not:
        return ~imm;
    default:
        return imm;
    }
}

// Source operand of the MOV instruction is already known being able to be
// propagated into all its uses. But, due to the dependency issue, it cannot be
// propagated. Try to propagate the type if a narrower type could be used.
static bool propagateType(IR_Builder &Builder, G4_BB *BB, G4_INST *Mov, G4_INST::MovType MT) {
    // Only propagate type if a narrower type could be used.
    if (MT != G4_INST::ZExt && MT != G4_INST::SExt)
        return false;

    G4_DstRegRegion *Dst = Mov->getDst();
    if (Dst->isIndirect())
        return false;

    // Check all propagation types are the same.
    G4_Type PT = Type_UNDEF;
    for (auto UI = Mov->use_begin(), UE = Mov->use_end(); UI != UE; ++UI) {
        auto Use = UI->first;
        auto OpndNum = UI->second;
        auto Opnd = Use->getOperand(OpndNum);
        if (!Opnd->isSrcRegRegion())
          return false;
        if (Opnd->asSrcRegRegion()->isIndirect())
          return false;
        G4_Type PropType = Use->getPropType(OpndNum, MT, Mov);
        if (PropType == Type_UNDEF)
          return false;
        if (PT != Type_UNDEF && PT != PropType)
          return false;
        PT = PropType;
    }
    if (PT == Type_UNDEF)
        return false;
    // Create a new destination of MOV of the propagation type.
    unsigned NumElt = Mov->getExecSize();
    auto NewDcl = Builder.createTempVar(NumElt, PT, Any);
    auto NewDst = Builder.Create_Dst_Opnd_From_Dcl(NewDcl, Dst->getHorzStride());
    Mov->setDest(NewDst);
    // Propagate type
    for (auto UI = Mov->use_begin(), UE = Mov->use_end(); UI != UE; ++UI) {
        auto Use = UI->first;
        auto OpndNum = UI->second;
        auto Opnd = Use->getOperand(OpndNum)->asSrcRegRegion();
        auto NewOpnd = Builder.Create_Src_Opnd_From_Dcl(NewDcl, Opnd->getRegion());
        Use->setSrc(NewOpnd, OpndNum - 1);
    }
    return true;
}

void Optimizer::newLocalCopyPropagation()
{
    BB_LIST_ITER ib, bend(fg.end());

    for (ib = fg.begin(); ib != bend; ++ib)
    {
        G4_BB* bb = *ib;

        bb->resetLocalId();

        INST_LIST_ITER ii = bb->begin(), iend(bb->end());
        while (ii != iend)
        {
            G4_INST *inst = *ii;
            G4_Operand *dst = inst->getDst();
            if (!dst)
            {
                ii++;
                continue;
            }

            doConsFolding(inst);
            doSimplification(inst);

            G4_INST::MovType MT = inst->canPropagate();
            // Skip super mov.
            if (MT == G4_INST::SuperMov) {
                ii++;
                continue;
            }

            if (!isCopyPropProfitable(inst))
            {
                ++ii;
                continue;
            }
            bool canRemove = true;

            // check if each use may be copy propagated.
            USE_EDGE_LIST_ITER iter, iend1(inst->use_end());
            for (iter = inst->use_begin(); iter != iend1; iter++)
            {
                G4_INST *useInst = iter->first;
                Gen4_Operand_Number opndNum = iter->second;

                if (inst->getDst()->isA0() && useInst->isSplitSend() && VISA_WA_CHECK(builder.getPWaTable(), WaSendSEnableIndirectMsgDesc))
                {
                    canRemove = false;
                    break;
                }

                if (!inst->canPropagateTo(useInst, opndNum, MT, !bb->isAllLaneActive()))
                {
                    canRemove = false;
                    break;
                }

                // Make sure there is no lifetime.end for src0 of the move inst
                INST_LIST_ITER cpIter = ii;
                cpIter++;
                while (*cpIter != useInst)
                {
                    // Detect patterns like:
                    //
                    // mov A, B
                    // ...
                    // lifetime.end B
                    // op C, A, D
                    //
                    // Because of presence of lifetime.end B, copy propagation for inst
                    // mov A, B
                    // cannot be done

                    if ((*cpIter)->isLifeTimeEnd())
                    {
                        // Check whether lifetime end is for same opnd
                        G4_Declare* lifetimeEndTopDcl = GetTopDclFromRegRegion((*cpIter)->getSrc(0));
                        G4_Declare* curInstDstTopDcl = GetTopDclFromRegRegion((*ii)->getDst());

                        if (lifetimeEndTopDcl == curInstDstTopDcl)
                        {
                            canRemove = false;
                            break;
                        }
                    }

                    cpIter++;
                }
            }

            if (canRemove && inst->getSrc(0)->isSrcRegRegion())
            {
                // check for anti-dependencies for src0 of the move instruction
                bool def_use_in_between = false;

                G4_INST* lastUse = inst->use_front().first;
                for (USE_EDGE_LIST_ITER
                        iter = inst->use_begin(),
                        uend = inst->use_end(); iter != uend; ++iter)
                {
                    G4_INST* useInst = iter->first;
                    if (useInst->getLocalId() > lastUse->getLocalId())
                    {
                        lastUse = useInst;
                    }
                }

                INST_LIST_ITER forwardIter = ii;
                forwardIter++;
                INST_LIST_ITER instListEnd = bb->end();

                while (!def_use_in_between &&
                       forwardIter != instListEnd &&
                       *forwardIter != lastUse)
                {
                    if ((*forwardIter)->isWARdep(inst))
                    {
                        def_use_in_between = true;
                        break;
                    }
                    forwardIter++;
                }

                // check if hoisting is possible
                if (def_use_in_between)
                {
                    hoistUseInst(bb, inst, forwardIter, canRemove);
                }

                if (!canRemove) {
                    // Check whether the type could be propagated instead to demote
                    // type if possible.
                    propagateType(builder, bb, inst, MT);
                }
            }

            if (!canRemove)
            {
                ii++;
                continue;
            }

            G4_Operand *src = inst->getSrc(0);
            // do propagation
            for (iter = inst->use_begin(); iter != iend1; /* empty */)
            {
                G4_INST *useInst = (*iter).first;
                Gen4_Operand_Number opndNum = (*iter).second;
                G4_Operand *use = useInst->getOperand(opndNum);
                G4_Type propType = useInst->getPropType(opndNum, MT, inst);

                // replace use with def
                if (src->isImm())
                {
                    auto newImmVal = G4_Imm::typecastVals(src->asImm()->getImm(),
                        propType);
                    G4_Imm* newImm = builder.createImm(newImmVal,
                        propType);
                    G4_SrcModifier modifier = use->asSrcRegRegion()->getModifier();
                    if (modifier != Mod_src_undef)
                    {
                        if (IS_TYPE_FLOAT_ALL(propType))
                        {
                            if (propType == Type_DF)
                            {
                                double imm = getImmValue(newImm->getDouble(), modifier);
                                newImm = builder.createDFImm(imm);
                            }
                            else
                            {
                                float imm = getImmValue(newImm->getFloat(), modifier);
                                newImm = builder.createImm(imm);
                            }
                        }
                        else
                        {
                            int64_t imm = getImmValue(newImm->getImm(), modifier);
                            newImm = builder.createImm(imm, propType);
                        }
                    }
                    useInst->setSrc(newImm,
                                    opndNum - 1);
                }
                else
                {
                    if (use == NULL)
                    {
                        break;
                    }
                    G4_SrcModifier new_mod = mergeModifier( src, use );

                    unsigned use_elsize = G4_Type_Table[use->getType()].byteSize;
                    unsigned dstElSize = G4_Type_Table[inst->getDst()->getType()].byteSize;
                    const RegionDesc *rd = src->asSrcRegRegion()->getRegion();
                    G4_Operand *new_src_opnd = NULL;
                    bool new_src = false;
                    unsigned char scale = 1, newExecSize = useInst->getExecSize();

                    // Compute the composed region if exists.
                    auto getComposedRegion = [=](
                        unsigned dStride, unsigned ex1,
                        const RegionDesc *rd1, unsigned ex2,
                        const RegionDesc *rd2) -> const RegionDesc *
                    {
                        // Easy cases.
                        if (rd1->isScalar())
                            return rd1;
                        else if (rd2->isScalar())
                            return rd2;
                        else if (dStride == 1 && rd1->isContiguous(ex1))
                            return rd2;
                        else if (dStride == 1 && rd2->isContiguous(ex2))
                            return rd1;

                        // rd1 and rd2 must be single strided. Use a non-zero
                        // invalid stride value as the initial value, which
                        // simplifies and unifies the checking.
                        uint16_t stride1 = 64;
                        if (rd1->isContiguous(ex1))
                            stride1 = 1;
                        else
                            rd1->isSingleNonUnitStride(ex1, stride1);

                        uint16_t stride2 = 64;
                        if (rd2->isContiguous(ex2))
                            stride2 = 1;
                        else
                            rd2->isSingleNonUnitStride(ex2, stride2);

                        // All are single strided; the composition is the product of strides.
                        if (stride1 * stride2 * dStride <= 32)
                            return builder.createRegionDesc((uint16_t)ex2, stride1 * stride2 * dStride, 1, 0);

                        // Should be unreachable, since the legality check
                        // before should reject cases that are difficult to do
                        // composition. Assert?
                        return nullptr;
                    };

                    if (MT == G4_INST::Trunc)
                    {
                        G4_DstRegRegion *dst = inst->getDst();
                        G4_SrcRegRegion *src0 = src->asSrcRegRegion();
                        unsigned typeSizeRatio = G4_Type_Table[src0->getType()].byteSize / G4_Type_Table[dst->getType()].byteSize;
                        unsigned numElt = src0->isScalar() ? 1 : inst->getExecSize() * typeSizeRatio;
                        // src0 region is guaranteed to be scalar/contiguous due to canPropagate() check earlier
                        const RegionDesc* region = src0->isScalar() ?
                            builder.getRegionScalar() :
                            builder.createRegionDesc(useInst->getExecSize(), (uint16_t)inst->getExecSize()
                            * typeSizeRatio, inst->getExecSize(),
                            (uint16_t)typeSizeRatio);
                        if (src0->isIndirect())
                        {
                            new_src_opnd = builder.createIndirectSrc(new_mod,
                                src0->getBase(), src0->getRegOff(), src0->getSubRegOff() * typeSizeRatio,
                                region, propType, src0->getAddrImm());
                        }
                        else
                        {
                            G4_Declare* newDcl = builder.createTempVar(numElt, inst->getDst()->getType(), Any);
                            newDcl->setAliasDeclare(src0->getBase()->asRegVar()->getDeclare(), 0);

                            new_src_opnd =
                                builder.createSrcRegRegion(new_mod, Direct,
                                newDcl->getRegVar(), src0->getRegOff(),
                                src0->getSubRegOff() * typeSizeRatio,
                                region, propType);
                        }
                        new_src = true;
                    }
                    else if (dstElSize < use_elsize)
                    {
                        // FIXME: How could this happen? Revisit later if
                        // NoMask is guaranteed.
                        // TODO!!! src is aligned to use type. -- should check this.
                        new_src = true;
                        scale = use_elsize/dstElSize;
                        unsigned short vs = rd->vertStride, wd = rd->width;
                        // packed word/byte
                        if (use->asSrcRegRegion()->isScalar())
                        {
                            rd = builder.getRegionScalar();
                        }
                        else if (inst->isComprInst() && vs == wd)
                        {
                            rd = builder.getRegionStride1();
                        }
                        else
                        {
                            rd = builder.createRegionDesc(vs/scale, wd/scale, 1);
                        }
                    }
                    else if (inst->getExecSize() < useInst->getExecSize() &&
                               rd && use->isSrcRegRegion())
                    {
                        unsigned dStride = inst->getDst()->getHorzStride();
                        const RegionDesc *rd2 = use->asSrcRegRegion()->getRegion();
                        if (auto compRd = getComposedRegion(dStride, inst->getExecSize(),
                                                            rd, newExecSize, rd2))
                        {
                            new_src = true;
                            rd = compRd;
                        }
                    }

                    if (new_mod != Mod_src_undef || new_src)
                    {
                        // For truncation case, new src operand is already built.
                        if (MT != G4_INST::Trunc) {
                            new_src_opnd = builder.createSrcRegRegion(
                                new_mod,
                                src->asSrcRegRegion()->getRegAccess(),
                                src->asSrcRegRegion()->getBase(),
                                src->asSrcRegRegion()->getRegOff(),
                                src->asSrcRegRegion()->getSubRegOff()/scale,
                                rd,
                                propType);
                            if (src->asSrcRegRegion()->getRegAccess() != Direct)
                            {
                                new_src_opnd->asSrcRegRegion()->setImmAddrOff(src->asSrcRegRegion()->getAddrImm());
                            }
                        }
                    }
                    else if (inst->hasOneUse())
                    {
                        new_src_opnd = src;
                        new_src_opnd->asSrcRegRegion()->setModifier(new_mod);
                        new_src_opnd->asSrcRegRegion()->setType(propType);
                    }
                    else
                    {
                        new_src_opnd = builder.duplicateOperand(src);
                        new_src_opnd->asSrcRegRegion()->setModifier(new_mod);
                        new_src_opnd->asSrcRegRegion()->setType(propType);
                    }
                    useInst->setSrc(new_src_opnd, opndNum - 1);
                }

                iter = inst->eraseUse(iter);
                // due to truncation a (partial) def of the move may no longer be a def of the use
                inst->copyDef(useInst, Opnd_src0, opndNum, true);

                doConsFolding(useInst);
            }
            // remove decl corresponding to this def
            // TODO!!! what if there is some alias to this decl?
            // remove MOV inst

            // remove it from the use list of its deflists
            inst->removeDefUse(Opnd_src0);

            INST_LIST_ITER tmp = ii;
            ii++;
            bb->erase(tmp);
        }
    }
}

void Optimizer::cselPeepHoleOpt()
{
    if (!builder.hasCondModForTernary())
    {
        return;
    }
    BB_LIST_ITER ib, bend(fg.end());
    G4_SrcRegRegion *cmpSrc0 = NULL;
    G4_Operand *cmpSrc1 = NULL;
    for(ib = fg.begin(); ib != bend; ++ib)
    {
        G4_BB* bb = (*ib);
        INST_LIST_ITER ii;
        INST_LIST_ITER nextIter;
        INST_LIST_ITER iiEnd;
        if (bb->empty())
        {
            continue;
        }

        bb->resetLocalId();
        ii = bb->begin();
        iiEnd = bb->end();

        nextIter = ii;

        do
        {
            ii = nextIter;
            ++nextIter;
            G4_INST *inst = *ii;
            G4_opcode op = inst->opcode();
            bool hasGRFDst = inst->getDst() && !inst->hasNULLDst();
            /*
            csel doesn't have the same symantics for destination
            as cmp instruction
            */
            if (op != G4_cmp || hasGRFDst || inst->getPredicate() ||
                inst->isDead() || !inst->getSrc(0)->isSrcRegRegion())
            {
                continue;
            }

            cmpSrc0 = inst->getSrc(0)->asSrcRegRegion();
            cmpSrc1 = inst->getSrc(1);

            G4_CondMod *cModifier = inst->getCondMod();

                // check if dst is global
            if( fg.globalOpndHT.isOpndGlobal( cModifier ) )
            {
                continue;
            }

            /*
            csel instruction implicitly compares src2 to 0
            only supports floats
            no predication
            */

            if(!cmpSrc1->isImm()                                                                            ||
                (cmpSrc1->asImm()->getImm() != 0 &&
                    (cmpSrc1->asImm()->getType() != Type_F || cmpSrc1->asImm()->getFloat()!= -0.0f))        ||
                cmpSrc0->getType() != Type_F                                                                ||
                cmpSrc0->isImm())
                continue;

            if (inst->getSrc(0)->isRelocImm() ||
                inst->getSrc(1)->isRelocImm())
            {
                continue;
            }

            // Only allow single strided regions.
            uint16_t src0Stride = 0;
            if (!cmpSrc0->getRegion()->isSingleStride(inst->getExecSize(), src0Stride))
                continue;

            /*
                Can do scan until use instruction to see if src0 is modified
                but I think for general case this will suffice. If we are
                not capturing opportunities can revisit.
            */

            if(inst->useEmpty())
                continue;

            int execSize = inst->getExecSize();
            if( execSize == 2)
                continue;

            USE_EDGE_LIST_ITER iter = inst->use_begin();
            USE_EDGE_LIST_ITER endUseList = inst->use_end();

            bool canOpt = true;
            int maxInstID = 0;

            for(; iter != endUseList; ++iter)
            {
                G4_INST* useInst = (*iter).first;

                if (useInst->getNumSrc() != 2)
                {
                    canOpt = false;
                    break;
                }

                maxInstID = std::max(useInst->getLocalId(), maxInstID);
                G4_Operand *dstUse = useInst->getDst();
                G4_Operand *selSrc0 = useInst->getSrc(0);
                G4_Operand *selSrc1 = useInst->getSrc(1);

                if( useInst->opcode() != G4_sel     ||
                    selSrc0->isImm()                ||
                    selSrc1->isImm()                ||
                    selSrc0->getType() != Type_F    ||
                    selSrc1->getType() != Type_F    ||
                    dstUse->getType()   != Type_F   ||
                    //3-src restriction
                    !builder.isOpndAligned(dstUse, 16)  ||
                    !builder.isOpndAligned(selSrc0, 16) ||
                    !builder.isOpndAligned(selSrc1, 16))
                {
                    canOpt = false;
                    break;
                }

                //   if inst is NoMask use inst can be anything.
                //   if inst is not NoMask then useInst needs to be subset of inst.
                if(!(inst->getMaskOption() & InstOpt_WriteEnable))
                {
                    auto isInclusive = [](int lb1, int rb1, int lb2, int rb2)
                    {
                        return lb1 <= lb2 && rb1 >= rb2;
                    };
                    if (!isInclusive(inst->getMaskOffset(), inst->getMaskOffset() + inst->getExecSize(),
                        useInst->getMaskOffset(), useInst->getMaskOffset() + useInst->getExecSize()))
                    {
                        canOpt = false;
                        break;
                    }
                }

                uint8_t numPredDefs = 0;
                DEF_EDGE_LIST_ITER useIter = useInst->def_begin();
                DEF_EDGE_LIST_ITER iterEnd = useInst->def_end();

                //    Just in case some weird code is generated with partial writes to predicate
                for(; useIter != iterEnd; ++useIter)
                {
                    if((*useIter).second == Opnd_pred)
                        ++numPredDefs;

                    // Check whether pseudo_kill for dst exists between cmp and sel
                    // cmp.xx.fx.0 (8) ... src0 $0
                    // ...
                    // pseudo_kill dst
                    // (f0) sel (8) dst src1 src2
                    //
                    // These two cannot be merged because pseudo_kill is in between them

                    INST_LIST_ITER cselOptIt = ii;
                    cselOptIt++;
                    while((*cselOptIt) != useInst)
                    {
                        if((*cselOptIt)->isLifeTimeEnd())
                        {
                            if(GetTopDclFromRegRegion((*cselOptIt)->getDst()) ==
                                GetTopDclFromRegRegion(useInst->getDst()))
                            {
                                canOpt = false;
                                break;
                            }
                        }

                        cselOptIt++;
                    }
                }

                if(numPredDefs > 1)
                {
                    canOpt = false;
                    break;
                }
            }

            INST_LIST_ITER tempInstIter = nextIter;
            //explicit check that cmp sr0 is not over written or partially writen to
            //between cmp and sel.
            for( ;tempInstIter != iiEnd; ++tempInstIter)
            {
                G4_INST *tempInst = *tempInstIter;

                if (tempInst->getLocalId() == maxInstID)
                {
                    break;
                }

                if(!tempInst->getDst())
                    continue;

                //also checks for indirect, will return inerference.
                G4_CmpRelation rel = tempInst->getDst()->compareOperand(cmpSrc0);
                if(rel != Rel_disjoint)
                {
                    canOpt = false;
                    break;
                }
            }

            if(canOpt)
            {
                for (auto iter = inst->use_begin(); iter != inst->use_end(); /*empty*/)
                {
                    G4_INST* useInst = (*iter).first;
                    G4_CondMod *mod = inst->getCondMod();
                    useInst->setOpcode(G4_csel);
                    useInst->setSrc(builder.duplicateOperand(inst->getSrc(0)), 2);
                    useInst->setCondMod(builder.duplicateOperand(mod));
                    useInst->setPredicate(NULL);

                    G4_SrcRegRegion* opnd2 = useInst->getSrc(2)->asSrcRegRegion();

                    if (!opnd2->isScalar() && inst->getExecSize() > useInst->getExecSize())
                    {
                        //earlier check establishes that useInst mask is equivalent or subset
                        //sel instruction
                        /*
                            case which considering:
                            cmp (16)
                            sel (8)
                        */
                        if (useInst->getMaskOffset() != inst->getMaskOffset())
                        {
                            //check elsewhere guarantees this is float.
                            G4_Type type = opnd2->getType();
                            unsigned short typeSize = (unsigned short)G4_Type_Table[type].byteSize;
                            unsigned offset = opnd2->getRegOff() * G4_GRF_REG_NBYTES + opnd2->getSubRegOff() * typeSize;
                            offset += useInst->getExecSize() * src0Stride * typeSize;

                            auto newSrc2 = builder.createSrcRegRegion(opnd2->getModifier(), Direct, opnd2->getBase(),
                                offset / G4_GRF_REG_NBYTES, (offset % G4_GRF_REG_NBYTES) / typeSize, opnd2->getRegion(),
                                opnd2->getType());
                            useInst->setSrc(newSrc2, 2);
                        }
                    }
                    //
                    // Modifying useDef links
                    //
                    // cmp.xx.f0.0 (8) ... src2 $0  <- inst (to be deleted)
                    // (f0) sel (8) dst src0 src1   <- useInst
                    // =>
                    // csel.xx.f0.0 (8) dst src0 src1 src2

                    // useInst's predicate becomes NULL.
                    iter = inst->eraseUse(iter);

                    // inst's src0 becomes useInst's src2.
                    inst->copyDef(useInst, Opnd_src0, Opnd_src2);
                }
                ASSERT_USER(inst->useEmpty(), "all predicate uses are removed.");
                inst->removeAllDefs();
                bb->erase( ii );
            }
        }while( nextIter != iiEnd );
    }
}

// helper function to convert
// and/or p3 p1 p2
// ==>
// (p1) sel t1 1 0
// (p2) sel t2 1 0
// and/or.nz.p3 t1 t2
// if the original inst is NoMask and Q1/H1, we do
// and/or p3 p1 p2
// ==>
// and/or (1) p3 p1 p2
// p3's type is uw for simd8/16 and ud for simd32
static void expandPseudoLogic(IR_Builder& builder,
                              G4_BB* bb,
                              INST_LIST_ITER& iter)

{
    G4_INST* inst = *iter;
    MUST_BE_TRUE(inst->isPseudoLogic(),
        "inst must be either pseudo_and/or/xor/not");
    INST_LIST_ITER newIter = iter;

    bool isFirstInst = iter == bb->begin();
    if (!isFirstInst)
    {
        --iter;
    }

    auto canFoldOnSIMD1 = [=, &builder]()
    {
        if (inst->isWriteEnableInst() &&
            (inst->getMaskOffset() == 0 || inst->getMaskOffset() == 16) &&
            // we can't do this for simd8 inst in simd16 kernels as it will overwrite upper flag bits
            (inst->getExecSize() > 8 || inst->getExecSize() == builder.kernel.getSimdSize()))
        {
            return true;
        }

        // Dst operand has a single flag element.
        if (inst->isWriteEnableInst() && inst->getMaskOffset() == 0 && inst->getExecSize() == 1)
        {
            G4_Operand *Dst = inst->getDst();
            if (Dst && Dst->getTopDcl() != nullptr)
            {
                G4_Declare *Dcl = Dst->getTopDcl();
                if (Dcl->getNumberFlagElements() == 1)
                    return true;
            }
        }

        return false;
    };

    if (canFoldOnSIMD1())
    {
        G4_opcode newOpcode = G4_illegal;
        if (inst->getMaskOffset() == 16)
        {
            MUST_BE_TRUE(inst->getExecSize() == 16, "Only support simd16 pseudo-logic instructions");
            // we have to use the upper flag bits (.1) instead
            MUST_BE_TRUE(inst->getSrc(0)->isSrcRegRegion() && inst->getSrc(0)->isFlag(),
                "expect src0 to be flag");
            auto newSrc0 = builder.createSrcWithNewSubRegOff(inst->getSrc(0)->asSrcRegRegion(), 1);
            inst->setSrc(newSrc0, 0);
            if (inst->getSrc(1) != nullptr)
            {
                MUST_BE_TRUE(inst->getSrc(1)->isSrcRegRegion() && inst->getSrc(1)->isFlag(),
                    "expect src1 to be flag");
                auto newSrc1 = builder.createSrcWithNewSubRegOff(inst->getSrc(1)->asSrcRegRegion(), 1);
                inst->setSrc(newSrc1, 1);
            }
            inst->getDst()->setSubRegOff(1);
        }

        switch (inst->opcode())
        {
            case G4_pseudo_and:
                newOpcode = G4_and;
                break;
            case G4_pseudo_or:
                newOpcode = G4_or;
                break;
            case G4_pseudo_xor:
                newOpcode = G4_xor;
                break;
            case G4_pseudo_not:
                newOpcode = G4_not;
                break;
            default:
                MUST_BE_TRUE(false, "unexpected opcode for pseudo-logic instructions");
        }

        inst->setOpcode(newOpcode);
        inst->setExecSize(1);
    }
    else
    {
        uint8_t tmpSize = inst->getExecSize();
        auto LowerOpnd = [=, &builder](Gen4_Operand_Number opNum, G4_INST* &SI) -> G4_Operand *
        {
            G4_Operand *Opnd = inst->getOperand(opNum);
            if (Opnd)
            {
                auto src = Opnd->asSrcRegRegion();
                auto newDcl = builder.createTempVar(tmpSize, Type_UW, Any);
                auto newDst = builder.createDst(newDcl->getRegVar(), 0, 0, 1, Type_UW);
                auto newPred = builder.createPredicate(PredState_Plus, src->getBase(), src->getSubRegOff());
                auto newSel = builder.createInternalInst(newPred, G4_sel, nullptr, false, tmpSize, newDst,
                                                         builder.createImm(1, Type_UW),
                                                         builder.createImm(0, Type_UW),
                                                         inst->getOption(),
                                                         inst->getLineNo(),
                                                         inst->getCISAOff(),
                                                         inst->getSrcFilename());
                inst->transferDef(newSel, opNum, Gen4_Operand_Number::Opnd_pred);
                bb->insert(newIter, newSel);
                SI = newSel;
                const RegionDesc *rd = (tmpSize == 1) ? builder.getRegionScalar() : builder.getRegionStride1();
                return builder.Create_Src_Opnd_From_Dcl(newDcl, rd);
            }
            return Opnd;
        };

        G4_INST *Sel0 = nullptr;
        G4_Operand *logicSrc0 = LowerOpnd(Gen4_Operand_Number::Opnd_src0, Sel0);

        G4_INST *Sel1 = nullptr;
        G4_Operand *logicSrc1 = LowerOpnd(Gen4_Operand_Number::Opnd_src1, Sel1);

        if (logicSrc1  == nullptr)
        {
            MUST_BE_TRUE(inst->opcode() == G4_pseudo_not, "Must be a pseudo-not instruction");
            // for not P1 P0
            // we generate
            // (P0) sel V1 1 0
            // xor.P1 null V1 1
            // so that the upper bits would stay zero
            logicSrc1 = builder.createImm(1, Type_UW);
        }

        auto nullDst = builder.createNullDst(Type_UW);
        auto newCondMod = builder.createCondMod(Mod_nz, inst->getDst()->getBase()->asRegVar(), 0);
        G4_opcode newOpcode = G4_illegal;
        switch (inst->opcode())
        {
            case G4_pseudo_and:
                newOpcode = G4_and;
                break;
            case G4_pseudo_or:
                newOpcode = G4_or;
                break;
            case G4_pseudo_xor:
                newOpcode = G4_xor;
                break;
            case G4_pseudo_not:
                // see comment above
                newOpcode = G4_xor;
                break;
            default:
                MUST_BE_TRUE(false, "unexpected opcode for pseudo-logic instructions");
        }

        G4_INST* newLogicOp = builder.createInternalInst(
            NULL,
            newOpcode,
            newCondMod,
            false,
            tmpSize,
            nullDst,
            logicSrc0,
            logicSrc1,
            inst->getOption(),  // keep the original instruction emask
            inst->getLineNo(),
            inst->getCISAOff(),
            inst->getSrcFilename() );

        // Fix def-use
        if (Sel0 != nullptr)
        {
            Sel0->addDefUse(newLogicOp, Gen4_Operand_Number::Opnd_src0);
        }
        if (Sel1 != nullptr)
        {
            Sel1->addDefUse(newLogicOp, Gen4_Operand_Number::Opnd_src1);
        }
        inst->transferUse(newLogicOp);
        bb->insert(newIter, newLogicOp);
        bb->erase(newIter);
    }

    // iter either points to the start or the first expanded instruction.  Caller will advance it to the previous instruction
    if (isFirstInst)
    {
        iter = bb->begin();
    }
    else
    {
        ++iter;
    }
}

// mov(1) P0 Imm(NoMask)
// (P0) mov(esize) r[A0, 0] src0 src1
// == >
// smov(esize) r[A0, 0] src0 src1 Imm
//
// esize is either 8 or 16
bool Optimizer::createSmov(G4_BB *bb, G4_INST* flagMove, G4_INST* next_inst)
{
    if ((next_inst->getExecSize() != 8 && next_inst->getExecSize() != 16) ||
        next_inst->getPredicate() == NULL ||
        next_inst->getCondMod() != NULL ||
        next_inst->getSaturate() == true ||
        next_inst->getDst()->getRegAccess() == Direct ||
        getTypeSize(next_inst->getDst()->getType()) == 1 ||
        getTypeSize(next_inst->getSrc(0)->getType()) == 1 ||
        (builder.getPlatform() < GENX_SKL && builder.getPlatform() != GENX_BDW) ||
        getTypeSize(next_inst->getDst()->getType()) < getTypeSize(next_inst->getSrc(0)->getType()))
    {
        return false;
    }

    if (next_inst->getSrc(0)->isSrcRegRegion() &&
        next_inst->getSrc(0)->asSrcRegRegion()->getModifier() != Mod_src_undef)
    {
        return false;
    }

    if (flagMove->use_size() != 1 || flagMove->use_front().first != next_inst)
    {
        return false;
    }

    G4_CmpRelation rel = flagMove->getDst()->compareOperand(next_inst->getPredicate());
    if (rel != Rel_eq && !(rel == Rel_gt && next_inst->getMaskOffset() == 0))
    {
        return false;
    }

    if (builder.getOptions()->getTarget() == VISA_3D || !bb->isAllLaneActive())
    {
        if (!flagMove->isWriteEnableInst())
        {
            return false;
        }
    }

    next_inst->setOpcode(G4_smov);
    next_inst->setSrc(flagMove->getSrc(0), 1);
    next_inst->setPredicate(nullptr);

    flagMove->removeUseOfInst();
    return true;
}

// Returns true if *iter has an use that is a cmp and we can fold that cmp
// into *iter as a conditional modifier. The cmp instruction is deleted as part of folding.
// Note that iter may be modified to point to the next inst if we decide to sync *iter to
// where the cmp was to work around dependencies
bool Optimizer::foldCmpToCondMod(G4_BB* bb, INST_LIST_ITER& iter)
{
    // find a cmp that uses inst dst
    G4_INST* inst = *iter;
    G4_INST* cmpInst = nullptr;
    bool canFold = false;

    if (inst->getCondMod())
    {
        return false;
    }

    for (auto UI = inst->use_begin(), UE = inst->use_end(); UI != UE; ++UI)
    {
        cmpInst = (*UI).first;

        // cmp instruction must be of the form
        // cmp [<op> P0] null src 0
        // where src is singly defined by inst
        if (cmpInst->opcode() == G4_cmp && cmpInst->getExecSize() == inst->getExecSize() &&
            cmpInst->hasNULLDst() &&
            cmpInst->getSrc(0)->isSrcRegRegion() &&
            cmpInst->getSrc(0)->asSrcRegRegion()->getModifier() == Mod_src_undef &&
            cmpInst->def_size() == 1 && !cmpInst->getPredicate() &&
            cmpInst->getSrc(1)->isImm() && cmpInst->getSrc(1)->asImm()->isZero())
        {
            canFold = true;
            break;
        }
    }

    if (!canFold)
    {
       return false;
    }

    // floating point cmp may flush denorms to zero, but mov may not.
    //
    // mov(1|M0) (lt)f0.0 r6.2<1>:f r123.2<0;1,0>:f
    // may not be the same as
    // mov(1|M0)          r6.2<1>:f r123.2<0;1,0>:f
    // cmp(1|M0) (lt)f0.0 null<1>:f 6.2<0;1,0>:f 0x0:f
    // for denorm inputs.
    if (inst->opcode() == G4_mov && IS_TYPE_FLOAT_ALL(cmpInst->getSrc(0)->getType()))
    {
        return false;
    }

    auto cmpIter = std::find(iter, bb->end(), cmpInst);

    // check if cmp instruction is close enough
    constexpr int DISTANCE = 60;
    if (std::distance(iter, cmpIter) > DISTANCE)
    {
        return false;
    }

    auto isSupportedCondMod = [](G4_CondModifier mod)
    {
        return mod == Mod_g || mod == Mod_ge || mod == Mod_l || mod == Mod_le ||
            mod == Mod_e || mod == Mod_ne;
    };
    G4_CondModifier mod = cmpInst->getCondMod()->getMod();
    if (!isSupportedCondMod(mod))
    {
        return false;
    }

    if (builder.getOptions()->getTarget() == VISA_3D || !bb->isAllLaneActive())
    {
        // Make sure masks of both instructions are same
        if (inst->getMaskOption() != cmpInst->getMaskOption())
        {
            return false;
        }
    }

    auto getUnsignedTy = [](G4_Type Ty)
    {
        switch (Ty)
        {
        case Type_D:
            return Type_UD;
        case Type_W:
            return Type_UW;
        case Type_B:
            return Type_UB;
        case Type_Q:
            return Type_UQ;
        case Type_V:
            return Type_UV;
        default:
            break;
        }
        return Ty;
    };

    G4_Type T1 = inst->getDst()->getType();
    G4_Type T2 = cmpInst->getSrc(0)->getType();
    if (getUnsignedTy(T1) != getUnsignedTy(T2))
    {
        return false;
    }
    // Skip if the dst needs saturating but it's used as different sign.
    if (inst->getSaturate() && T1 != T2)
    {
        return false;
    }

    if (chkBwdOutputHazard(iter, cmpIter))
    {
        return false;
    }

    auto isSafeToSink = [](INST_LIST_ITER defIter,
        INST_LIST_ITER beforeIter, int maxDist)
    {
        G4_INST *inst = *defIter;
        int dist = 0;
        for (auto it = std::next(defIter); it != beforeIter; ++it)
        {
            if (dist++ >= maxDist)
                return false;
            if (inst->isWAWdep(*it) || inst->isRAWdep(*it) ||
                inst->isWARdep(*it))
                return false;
            if (!checkLifetime(inst, *it))
                return false;
        }
        return true;
    };

    // Merge two instructions
    // If legal, use the cmp location as new insert position.
    bool sinkInst = false;

    if (inst->getDst()->compareOperand(cmpInst->getSrc(0)) == Rel_eq)
    {
        if (inst->use_size() == 1)
        {
            // see if we can replace dst with null
            if (inst->supportsNullDst() && !fg.globalOpndHT.isOpndGlobal(inst->getDst()))
            {
                inst->setDest(builder.createDst(
                    builder.phyregpool.getNullReg(),
                    0,
                    0,
                    inst->getDst()->getHorzStride(),
                    inst->getDst()->getType()));
            }
            // Check if it is safe to sink inst right before cmp inst, which lowers flag pressure in general.
            const int MAX_DISTANCE = 20;
            sinkInst = isSafeToSink(iter, cmpIter, MAX_DISTANCE);
        }
        inst->setCondMod(cmpInst->getCondMod());
        inst->setOptions((inst->getOption() & ~InstOpt_Masks) | (cmpInst->getMaskOption()));
        // The sign of dst should follow its use instead of its
        // def. The later is meaningless from how hardware works.
        auto honorSignedness = [](G4_CondModifier mod) {
            switch (mod) {
            case Mod_g:
            case Mod_ge:
            case Mod_l:
            case Mod_le:
                return true;
            default:
                break;
            }
            return false;
        };
        if (honorSignedness(inst->getCondMod()->getMod()))
            inst->getDst()->setType(T2);

        // update def-use
        // since cmp is deleted, we have to
        // -- transfer cmp's use to inst
        // -- remove cmp from its definition's use list
        cmpInst->transferUse(inst, true);
        cmpInst->removeUseOfInst();
        if (!sinkInst)
        {
            bb->erase(cmpIter);
        }
        else
        {
            // Before and <- ii
            //        cmp <- next_iter
            // After  cmp <- ii
            //        and <- next
            std::iter_swap(iter, cmpIter);
            auto nextii = std::next(iter);
            bb->erase(iter);
            iter = nextii;
        }
        return true;
    }
    return false;
}

// Return true  : folding cmp and sel is performed
//        false : no folding is performed.
bool Optimizer::foldCmpSel(G4_BB *BB, G4_INST *selInst, INST_LIST_ITER &selInst_II)
{
    MUST_BE_TRUE( (selInst->opcode() == G4_sel &&
                   selInst->getPredicate() &&
                   selInst->getCondMod() == NULL),
                  "foldCmpSel: Inst should be a sel with predicate and without cmod." );
    G4_Predicate *pred = selInst->getPredicate();

    // global predicates are not eligible since folding the cmp removes the def
    if (fg.globalOpndHT.isOpndGlobal(pred))
    {
        return false;
    }

    // Needs to find cmp instruction that defines predicate of sel. The cmp is
    // not necessarily right before the sel. To be able to fold the cmp to the
    // sel, we have to check if the cmp can be moved right before the sel. It not,
    // no folding is performed.
    G4_INST *cmpInst = nullptr;
    for (auto DI = selInst->def_begin(), DE = selInst->def_end(); DI != DE; ++DI)
    {
        if ((*DI).second == Opnd_pred)
        {
            if (cmpInst)
            {   // only handle single def.
                cmpInst = nullptr;
                break;
            }
            cmpInst = (*DI).first;
            if (cmpInst && cmpInst->opcode() != G4_cmp)
            {
                cmpInst = nullptr;
                break;
            }
        }
    }
    if (!cmpInst)
    {
        // G4_cmp is not found, skip optimization.
        return false;
    }

    // Do a fast check first. Note that sel w/ cmod
    // does not allow predication. So, we will give up if cmp has predicate.
    bool isSubExecSize = (selInst->getExecSize() < cmpInst->getExecSize());
    bool isSameExecSize = (selInst->getExecSize() == cmpInst->getExecSize());
    if ((!isSameExecSize && !isSubExecSize) ||
        (cmpInst->getDst() && !cmpInst->hasNULLDst()) ||
        cmpInst->use_size() != 1 ||
        cmpInst->getPredicate() != nullptr)
    {
        return false;
    }

    // Check if two source operands have the same type and value.
    auto IsEqual = [](G4_Operand *opnd1, G4_Operand *opnd2)
    {
        if (opnd1->isImm() && opnd2->isImm())
            return opnd1->asImm()->isEqualTo(opnd2->asImm());
        if (opnd1->compareOperand(opnd2) != Rel_eq)
            return false;
        // footprint does not imply equality.
        // (1) region difference:  r10.0<1;4,2>:f vs r10.0<8;8,1>
        // (2) source modifier: r10.0<0;1,0>:f vs -r10.0<0;1,0>
        //
        if (opnd1->isSrcRegRegion() && opnd2->isSrcRegRegion())
            return opnd1->asSrcRegRegion()->sameSrcRegRegion(*opnd2->asSrcRegRegion());

        // Common cases should be covered.
        return false;
    };

    G4_Operand *sel_src0 = selInst->getSrc(0);
    G4_Operand *sel_src1 = selInst->getSrc(1);
    G4_Operand *cmp_src0 = cmpInst->getSrc(0);
    G4_Operand *cmp_src1 = cmpInst->getSrc(1);

    // Normalize SEL's predicate state to Plus.
    if (G4_PredState::PredState_Minus == pred->getState())
    {
        selInst->swapSrc(0, 1);
        selInst->swapDefUse();
        pred->setState(G4_PredState::PredState_Plus);
        std::swap(sel_src0, sel_src1);
    }

    // source operands of SEL and CMP are reversed or not.
    bool reversed = false;
    G4_CondMod* condMod = cmpInst->getCondMod();

    auto canFold = [=, &reversed]() {
        G4_CmpRelation condRel = pred->asPredicate()->compareOperand(condMod);
        if (!(condRel == Rel_eq && isSameExecSize) &&
            !(condRel == Rel_lt && isSubExecSize))
            return false;

        if (!cmpInst->isWriteEnableInst() &&
            cmpInst->getMaskOption() != selInst->getMaskOption())
            return false;

        // P = cmp.gt A, B
        // C = (+P) sel A, B  => C = sel.gt A, B
        //
        // P = cmp.ne A, B
        // C = (+P) sel A, B  => C = sel.ne A, B
        //
        if (IsEqual(sel_src0, cmp_src0) && IsEqual(sel_src1, cmp_src1))
            return true;

        // Sel operands are reversed.
        // P = cmp.gt A, B
        // C = (+P) sel B, A  => C = sel.le B, A
        //
        // P = cmp.ne A, B
        // C = (+P) sel B, A  => C = sel.ne B, A
        //
        if (IsEqual(sel_src0, cmp_src1) && IsEqual(sel_src1, cmp_src0))
        {
            reversed = true;
            return true;
        }
        return false;
    };

    if (!canFold())
    {
        return false;
    }

    // check if cmpInst can be legally moved right before selInst;
    // if it cannot, we cannot fold cmp to sel!.
    INST_LIST_ITER cmpInst_II = selInst_II;
    while (cmpInst_II != BB->begin())
    {
        cmpInst_II--;
        if (*cmpInst_II == cmpInst)
        {
            break;
        }
    }

    // No local def (possible?)
    if (cmpInst_II == BB->begin())
    {
        return false;
    }

    // If cmpInst has no WAR harzard b/w cmpInst and selInst, cmpInst
    // can be moved right before selInst.
    if (chkFwdOutputHazard(cmpInst_II, selInst_II))
    {
        return false;
    }

    G4_CondModifier mod = condMod->getMod();
    if (reversed)
        mod = G4_CondMod::getReverseCondMod(mod);
    G4_CondMod* cmod = builder.createCondMod(mod, condMod->getBase(),
                                             condMod->getSubRegOff());
    selInst->setCondMod(cmod);
    selInst->setPredicate(nullptr);

    // update def-use
    // since cmp is deleted, we have to
    // -- remove def-use between cmp and sel
    // -- transfer cmp's remaining use to sel
    // -- remove cmp and its definitions' use
    selInst->removeDefUse(Opnd_pred);
    cmpInst->transferUse(selInst, true);
    cmpInst->removeUseOfInst();
    BB->erase(cmpInst_II);
    return true;
}

// try to fold a pseudo not instruction into its use(s)
// return true if successful
bool Optimizer::foldPseudoNot(G4_BB* bb, INST_LIST_ITER& iter)
{
    G4_INST* notInst = *iter;
    assert(notInst->opcode() == G4_pseudo_not && "expect not instruction");
    if (notInst->getPredicate() || notInst->getCondMod())
    {
        return false;
    }

    G4_DstRegRegion* dst = notInst->getDst();
    if (fg.globalOpndHT.isOpndGlobal(dst))
    {
        return false;
    }
    if (!notInst->getSrc(0)->isSrcRegRegion())
    {
        return false;
    }

    // unfortunately, def-use chain is not always properly maintained, so we have to skip opt
    // even if we can't find a use
    bool canFold = notInst->use_size() > 0;
    for (auto uses = notInst->use_begin(), end = notInst->use_end(); uses != end; ++uses)
    {
        auto&& use = *uses;
        G4_INST* useInst = use.first;
        Gen4_Operand_Number opndPos = use.second;
        if (!useInst->isLogic() || !G4_INST::isSrcNum(opndPos))
        {
            canFold = false;
            break;
        }

        // check the case where flag is partially used
        G4_SrcRegRegion* opnd = useInst->getSrc(G4_INST::getSrcNum(opndPos))->asSrcRegRegion();
        if (dst->compareOperand(opnd) != Rel_eq)
        {
            canFold = false;
            break;
        }
    }

    if (canFold)
    {
        G4_SrcRegRegion* origUse = notInst->getSrc(0)->asSrcRegRegion();
        if (notInst->getMaskOffset() == 16)
        {
            // Fold upper bits
            assert(notInst->getExecSize() == 16);
            origUse = builder.createSrcWithNewSubRegOff(origUse, 1);
            notInst->setSrc(origUse, 0);
        }
        for (auto uses = notInst->use_begin(), uend = notInst->use_end(); uses != uend; ++uses)
        {
            auto use = *uses;
            G4_INST* useInst = use.first;
            Gen4_Operand_Number opndPos = use.second;
            G4_SrcRegRegion* opnd = useInst->getSrc(G4_INST::getSrcNum(opndPos))->asSrcRegRegion();
            int numNot = 1 + (origUse->getModifier() == Mod_Not ? 1 : 0) +
                (opnd->getModifier() == Mod_Not ? 1 : 0);
            G4_SrcModifier newModifier = numNot & 0x1 ? Mod_Not : Mod_src_undef;
            G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(*origUse);
            newSrc->setModifier(newModifier);
            useInst->setSrc(newSrc, G4_INST::getSrcNum(opndPos));

        }

        for (auto defs = notInst->def_begin(); defs != notInst->def_end(); ++defs)
        {
            auto def = *defs;
            G4_INST* defInst = def.first;
            notInst->copyUsesTo(defInst, false);
        }
        notInst->removeAllDefs();
        notInst->removeAllUses();
        bb->erase(iter);
        return true;
    }
    return false;
}

/***
this function optmize the following cases:

case 1:
cmp.gt.P0 s0 s1
(P0) sel d s0 s1
==>
sel.gt.P0 d s0 s1

case 2:
and dst src0 src1   -- other OPs are also optimized: or, xor...
cmp.nz.P0 NULL dst 0
(P0) ...
==>
and.nz.P0 dst src0 src1
(P0) ...
add/addc instructions also handled in case 2. Few more cond
modifiers supported to such arithmetic instructions.

case 3:
cmp.l.P0 NULL src0 src1
cmp.l.P1 NULL src2 src3
and P2 P0 P1   -- other OPs are also optimized: or, xor...
==>
cmp.l.P2 NULL src0 src1
(P2)cmp.l.P2 NULL src2 src3

case 4:
mov (1) P0 Imm  (NoMask)
(P0) mov (8) r[A0, 0] src0 src1
==>
smov (8) r[A0, 0] src0 src1 Imm

case 5:
psuedo_not (1) P2 P1
and (1) P4 P3 P2
==>
and (1) P4 P3 ~P1
*/

void Optimizer::optimizeLogicOperation()
{
    BB_LIST_ITER ib, bend(fg.end());
    G4_Operand *dst = NULL;
    bool resetLocalIds =  false;
    bool doLogicOpt = builder.getOption(vISA_LocalFlagOpt);

    if (!doLogicOpt)
    {
        // we still need to expand the pseudo logic ops
        for (auto bb : fg)
        {
            for (auto I = bb->begin(), E = bb->end(); I != E; ++I)
            {
                auto inst = *I;
                if (inst->isPseudoLogic())
                {
                    expandPseudoLogic(builder, bb, I);
                }
            }
        }
        return;
    }

    for(ib = fg.begin(); ib != bend; ++ib)
    {
        G4_BB* bb = (*ib);
        INST_LIST_ITER ii;
        if( ( bb->begin() == bb->end() ) ){
            continue;
        }
        resetLocalIds =  false;
        ii = bb->end();
        do
        {
            ii--;
            G4_INST *inst = *ii;
            G4_opcode op = inst->opcode();
            dst = inst->getDst();
            bool nullDst = inst->hasNULLDst();
            G4_Declare *dcl = nullptr;
            if (dst)
            {
                dcl = dst->getTopDcl();
            }

            if ((!Opcode_can_use_cond_mod(op) && !Opcode_define_cond_mod(op) &&
                !inst->isPseudoLogic()) ||
                !dst || nullDst || (dcl && dcl->isOutput()))
            {
                continue;
            }

            INST_LIST_ITER next_iter = ii, pred_iter = ii;
            next_iter++;
            if( ii != bb->begin() )
            {
                pred_iter--;
            }

            if(!resetLocalIds)
            {
                bb->resetLocalId();
                resetLocalIds =  true;
            }

            // case 5
            if (inst->opcode() == G4_pseudo_not)
            {
                bool folded = foldPseudoNot(bb, ii);
                if (folded)
                {
                    ii = next_iter;
                    continue;
                }
            }

            // case 1
            if (pred_iter != ii && op == G4_sel && inst->getPredicate() && !inst->getCondMod())
            {
                if (!foldCmpSel(bb, inst, ii)) {
                    // Compare two operands with special simple checking on
                    // indirect access. That checking could be simplified as
                    // only dst/src of the same instruction are checked.
                    auto compareOperand =
                        [](G4_DstRegRegion *A, G4_Operand *B, unsigned ExecSize)
                        -> G4_CmpRelation {
                        G4_CmpRelation Res = A->compareOperand(B);
                        if (Res != Rel_interfere)
                            return Res;
                        if (A->getRegAccess() != IndirGRF ||
                            B->getRegAccess() != IndirGRF)
                            return Res;
                        if (A->getHorzStride() != 1)
                            return Res;
                        // Extra check if both are indirect register accesses.
                        G4_VarBase *BaseA = A->getBase();
                        G4_VarBase *BaseB = B->getBase();
                        if (!BaseA || !BaseB || BaseA != BaseB || !BaseA->isRegVar())
                            return Res;
                        if (!B->isSrcRegRegion())
                            return Res;
                        G4_SrcRegRegion *S = B->asSrcRegRegion();
                        if (!S->getRegion()->isContiguous(ExecSize))
                            return Res;
                        if (A->getRegOff() != S->getRegOff() ||
                            A->getSubRegOff() != S->getSubRegOff())
                            return Res;
                        if (A->getAddrImm() != S->getAddrImm())
                            return Res;
                        return Rel_eq;
                    };

                    unsigned ExSz = inst->getExecSize();
                    G4_DstRegRegion *Dst = inst->getDst();
                    G4_Operand *Src0 = inst->getSrc(0);
                    G4_Operand *Src1 = inst->getSrc(1);
                    int OpndIdx = -1;
                    if (compareOperand(Dst, Src0, ExSz) == Rel_eq &&
                        Src0->isSrcRegRegion() &&
                        Src0->asSrcRegRegion()->getModifier() == Mod_src_undef)
                        OpndIdx = 0;
                    else if (compareOperand(Dst, Src1, ExSz) == Rel_eq &&
                             Src1->isSrcRegRegion() &&
                             Src1->asSrcRegRegion()->getModifier() == Mod_src_undef)
                        OpndIdx = 1;
                    if (OpndIdx >= 2) {
                        // If dst is equal to one of operands of 'sel', that
                        // 'sel' could be transformed into a predicated 'mov',
                        // i.e.,
                        //
                        // transforms
                        //
                        //  (+p) sel dst, src0, src1
                        //
                        // into
                        //
                        //  (+p) mov dst, src0   if dst == src1
                        //
                        // or
                        //
                        //  (-p) mov dst, src1   if dst == src0
                        //
                        if (OpndIdx == 0) {
                            // Inverse predicate.
                            G4_Predicate *Pred = inst->getPredicate();
                            G4_PredState State = Pred->getState();
                            State = (State == PredState_Plus) ? PredState_Minus
                                                              : PredState_Plus;
                            Pred->setState(State);
                            // Swap source operands.
                            inst->swapSrc(0, 1);
                            inst->swapDefUse();
                        }
                        inst->setOpcode(G4_mov);
                        inst->setSrc(nullptr, 1);
                        inst->removeDefUse(Opnd_src1);
                    }
                }
            }
            else if (builder.hasSmov() && inst->opcode() == G4_mov &&
                inst->getPredicate() == NULL &&
                inst->getCondMod()  == NULL &&
                inst->getExecSize() == 1 &&
                inst->getSrc(0)->isImm() &&
                inst->getDst()->isFlag() &&
                next_iter != bb->end() &&
                (*next_iter)->opcode() == G4_mov )
            {
                // case 4
                G4_INST *next_inst = *next_iter;
                if (createSmov(bb, inst, next_inst))
                {
                    bb->erase(ii);
                    ii = next_iter;
                }
                continue;
            }
            else if (inst->getPredicate() == NULL && inst->canSupportCondMod())
            {
                // FIXME: why this condition?
                if (op == G4_pseudo_mad && inst->getExecSize()==1)
                {
                    continue;
                }

                // case 2
                foldCmpToCondMod(bb, ii);
            }
            else if (inst->getPredicate() == NULL && inst->isPseudoLogic() )
            {
                bool merged = false;

                if (op == G4_pseudo_and || op == G4_pseudo_or)
                {
                    merged = foldPseudoAndOr(bb, ii);
                }

                // translate the psuedo op
                if (!merged)
                {
                    expandPseudoLogic(builder, bb, ii);
                }
            }
        }
        while( ii != bb->begin() );
    }
}

// see if we can fold a pseudo and/or instruction with previous cmp
// returns true if successful, and ii (inst-list-iter) is also updated
// to the next inst
bool Optimizer::foldPseudoAndOr(G4_BB* bb, INST_LIST_ITER& ii)
{
    // case 3

    // optimization should apply even when the dst of the pseudo-and/pseudo-or is global,
    // since we are just hoisting it up, and WAR/WAW checks should be performed as we search
    // for the src0 and src1 inst.

    G4_INST* inst = *ii;
    // look for def of srcs
    G4_Operand* src0 = inst->getSrc(0);
    G4_Operand* src1 = inst->getSrc(1);

    /*
    Old logic would scan from inst (and/or) up until it encountered
    def instructions that did full write.
    It would scan from def instruction down to inst looking for WAW, WAR
    conflicts between def instruction and intermediate instructions.
    Basically insuring that there were no partial writes in to flag registers
    use by the inst.

    The new code uses defInstList directly, and aborts if there are more then are
    two definitions. Which means there is more then one instruction writing to source.
    Disadvantage of that is that it is less precisise. For example if we are folding in to
    closest definition then before it was OK, but now will be disallowed.
    */
    int32_t maxSrc1 = 0;
    int32_t maxSrc2 = 0;
    G4_INST *defInstructions[2] = { nullptr, nullptr };
    G4_INST* src0DefInst = nullptr;
    G4_INST* src1DefInst = nullptr;

    //Picks the latest two instructions to compare.
    //local IDs are reset at the beginning of this function.
    if (inst->def_size() < 2)
    {
        return false;
    }
    //trying to find latest instructions that define src0 and src1
    for (auto I = inst->def_begin(), E= inst->def_end(); I != E; ++I)
    {
        G4_INST* srcInst = I->first;
        if ((I->second == Opnd_src0) && (srcInst->getLocalId() >= maxSrc1))
        {
            maxSrc1 = srcInst->getLocalId();
            defInstructions[0] = srcInst;
            src0DefInst = srcInst;
        }
        else if ((I->second == Opnd_src1) && (srcInst->getLocalId() >= maxSrc2))
        {
            maxSrc2 = srcInst->getLocalId();
            defInstructions[1] = srcInst;
            src1DefInst = srcInst;
        }
    }

    // Making sure that dst of pseudo instruction is not used or defined between pseudo instruction
    // and the first definition of the source
    if (defInstructions[0] && defInstructions[1])
    {
        // make defInst[0] the closer def to the pseudo-and/or
        if (maxSrc2 > maxSrc1)
        {
            std::swap(defInstructions[0], defInstructions[1]);
            std::swap(maxSrc1, maxSrc2);
        }
        //Doing backward scan until earlist src to make sure dst of and/or is not being written to
        //or being read
        /*
        handling case like in spmv_csr
        cmp.lt (M1, 1) P15 V40(0,0)<0;1,0> 0x10:w                                    /// $191
        cmp.lt (M1, 1) P16 V110(0,0)<0;1,0> V34(0,0)<0;1,0>                          /// $192
        and (M1, 1) P16 P16 P15                                                      /// $193
        */
        if (chkBwdOutputHazard(defInstructions[1], ii, defInstructions[0]))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // check if the defInst can be folded into the pseudo and/or for a given source
    // folding is legal if
    // -- src is the only use of defInst
    // -- def completely defines the src
    // -- def inst does not have predicate
    // -- the defInst closer to the pseudo inst is a cmp without pred
    // -- def inst is not global operand
    // the last condition can be relaxed if defInst is the same as the inst's dst,
    // as they will be the same flag
    if (!(defInstructions[0]->opcode() == G4_cmp && defInstructions[0]->getPredicate() == nullptr))
    {
        return false;
    }

    auto checkSource = [this, inst](G4_INST* defInst, G4_Operand* src)
    {
        if (defInst == nullptr)
        {
            return false;
        }

        if (defInst->use_size() > 1)
        {
            return false;
        }
        G4_Operand* curr_dst = defInst->getCondMod() ?
            defInst->getCondMod() : (G4_Operand*) defInst->getDst();

        G4_CmpRelation rel = curr_dst->compareOperand(src);
        if (rel != Rel_eq ||
            (defInst->getPredicate() && defInst->opcode() != G4_sel))
        {
            return false;
        }

        if (fg.globalOpndHT.isOpndGlobal(curr_dst))
        {
            G4_DstRegRegion* dst = inst->getDst();
            if (dst->compareOperand(curr_dst) != Rel_eq)
            {
                return false;
            }
        }

        return true;
    };

    if (!checkSource(src0DefInst, src0) || !checkSource(src1DefInst, src1))
    {
        return false;
    }

    // do the case 3 optimization

    G4_PredState ps = (inst->opcode() == G4_pseudo_or) ? PredState_Minus : PredState_Plus;
    G4_INST *first_inst = defInstructions[1];
    G4_INST *second_inst = defInstructions[0];

    // unify subregister according to logic op dst
    G4_VarBase *curr_base = inst->getDst()->getBase()->asRegVar();
    unsigned short curr_subreg = 0;

    G4_Operand *first_def, *second_def;
    G4_VarBase *first_def_base, *second_def_base;
    int first_def_subreg, second_def_subreg;

    // change condmod and predicate of second inst

    if (first_inst->getCondMod())
    {
        first_def = first_inst->getCondMod();
        first_def_base = first_def->asCondMod()->getBase();
        first_def_subreg = first_def->asCondMod()->getSubRegOff();
    }
    else
    {
        first_def = first_inst->getDst();
        first_def_base = first_def->asDstRegRegion()->getBase()->asRegVar();
        first_def_subreg = 0;
    }

    if (second_inst->getCondMod())
    {
        second_def = second_inst->getCondMod();
        second_def_base = second_def->asCondMod()->getBase();
        second_def_subreg = second_def->asCondMod()->getSubRegOff();
    }
    else
    {
        second_def = second_inst->getDst();
        second_def_base = second_def->asDstRegRegion()->getBase()->asRegVar();
        second_def_subreg = 0;
    }

    bool change_flag = false;
    if (second_inst->getCondMod() &&
        (second_def_base != curr_base || second_def_subreg != curr_subreg))
    {
        change_flag = true;
        G4_CondMod *new_condMod = builder.createCondMod(
            second_inst->getCondMod()->getMod(),
            curr_base,
            curr_subreg);

        second_inst->setCondMod(new_condMod);
    }
    // create a predicate

    G4_Predicate *new_pred = builder.createPredicate(
        ps,
        curr_base,
        curr_subreg);

    second_inst->setPredicate(new_pred);

    if (change_flag)
    {
        for (USE_EDGE_LIST_ITER iter = second_inst->use_begin();
            iter != second_inst->use_end();
            ++iter)
        {
            G4_INST *curr_inst = (*iter).first;
            if (curr_inst == inst)
            {
                continue;
            }
            if (curr_inst->getPredicate() &&
                (curr_inst->getPredicate()->getBase() != curr_base ||
                    curr_inst->getPredicate()->getSubRegOff() != curr_subreg) &&
                curr_inst->getPredicate()->compareOperand(second_def) == Rel_eq)
            {
                curr_inst->setPredicate(builder.duplicateOperand(new_pred));
            }

            for (int k = 0; k < curr_inst->getNumSrc(); k++)
            {
                G4_Operand *curr_src = curr_inst->getSrc(k);
                if (curr_src->isSrcRegRegion() && !(curr_inst->isMath() && k == 1 && curr_src->isNullReg()))
                {
                    if (curr_src->asSrcRegRegion()->compareOperand(second_def) == Rel_eq)
                    {
                        G4_SrcRegRegion *new_src_opnd = builder.createSrcRegRegion(
                            curr_src->asSrcRegRegion()->getModifier(),
                            curr_src->asSrcRegRegion()->getRegAccess(),
                            inst->getDst()->getBase(),
                            0,
                            0,
                            curr_src->asSrcRegRegion()->getRegion(),
                            curr_src->asSrcRegRegion()->getType());

                        curr_inst->setSrc(new_src_opnd, k);
                    }
                }
            }
        }
    }

    if (first_def_base != curr_base || first_def_subreg != curr_subreg)
    {
        if (first_inst->getCondMod() && first_def->isCondMod())
        {
            G4_CondMod *new_condMod = builder.createCondMod(
                first_inst->getCondMod()->getMod(),
                curr_base,
                curr_subreg);
            first_inst->setCondMod(new_condMod);
        }
        else
        {
            first_inst->setDest(builder.duplicateOperand(inst->getDst()));
        }
        for (USE_EDGE_LIST_ITER iter = first_inst->use_begin();
            iter != first_inst->use_end();
            ++iter)
        {
            G4_INST *curr_inst = (*iter).first;
            if (curr_inst == inst)
            {
                continue;
            }
            if (curr_inst->getPredicate() &&
                (curr_inst->getPredicate()->getBase() != curr_base ||
                    curr_inst->getPredicate()->getSubRegOff() != curr_subreg) &&
                curr_inst->getPredicate()->compareOperand(first_def) == Rel_eq)
            {
                curr_inst->setPredicate(builder.duplicateOperand(new_pred));
            }

            for (int k = 0; k < curr_inst->getNumSrc(); k++)
            {
                G4_Operand *curr_src = curr_inst->getSrc(k);
                if (curr_src->isSrcRegRegion() && !(curr_inst->isMath() && k == 1 && curr_src->isNullReg()))
                {
                    if (curr_src->asSrcRegRegion()->compareOperand(first_def) == Rel_eq)
                    {
                        G4_SrcRegRegion *new_src_opnd = builder.createSrcRegRegion(
                            curr_src->asSrcRegRegion()->getModifier(),
                            curr_src->asSrcRegRegion()->getRegAccess(),
                            curr_base,
                            0,
                            curr_subreg,
                            curr_src->asSrcRegRegion()->getRegion(),
                            curr_src->asSrcRegRegion()->getType());
                        curr_inst->setSrc(new_src_opnd, k);
                    }
                }
            }
        }
    }

    // update def-use
    // since inst (the pseudo-and/or) is deleted and the same flag is used for first
    // and second inst, we have to
    // -- transfer inst's use to the second_inst
    // -- add def-use between first_inst and second_inst
    // -- remove inst from first_inst and second_inst's use
    inst->transferUse(second_inst, true);
    first_inst->addDefUse(second_inst, Opnd_pred);
    inst->removeUseOfInst();

    INST_LIST_ITER new_iter = ii;
    ++ii;
    bb->erase(new_iter);
    return true;
}


    /***  The beginning of message header optimization  ***/

    /*
    * reuse the previous header which can save the redundant definitions.
    */
    void MSGTable::reusePreviousHeader(G4_INST *dest,
        G4_INST *source,
        G4_INST *mDot2,
        IR_Builder &builder)
    {
        if (dest==NULL) return;
        if (source != NULL)
        {
            dest->setDest( builder.duplicateOperand( source->getDst() ) );
        }
        else
        {
            short subRegOff = dest->getDst()->getSubRegOff();
            dest->setDest( builder.duplicateOperand( mDot2->getDst() ) );
            dest->getDst()->setSubRegOff( subRegOff );
        }
    }

    /*
    * insert a mov, from the previous header to current header
    * this is only required for the instruction,
    * whose payload size >1 so that we can't directly reuse
    * the previous header. keep a copy from the previous header,
    * so that we only need to update the fields that need to be changed.
    */
    void MSGTable::insertHeaderMovInst(G4_INST *source_send,
        IR_Builder& builder,
        G4_BB *bb)
    {
        G4_INST *inst = NULL;
        INST_LIST_ITER pos;

        switch (first)
        {
        case HEADER_FULL_REGISTER:
            inst = m;
            pos  = m_it;
            break;
        case HEADER_X:
            inst = mDot0;
            pos  = mDot0_it;
            break;
        case HEADER_Y:
            inst = mDot1;
            pos  = mDot1_it;
            break;
        case HEADER_SIZE:
            inst = mDot2;
            pos  = mDot2_it;
            break;
        default:
            MUST_BE_TRUE(false, "did not catch the first def instruction correctly");
            return;
        }

        // mov(8) m_new<1>, m_old<8;8:1>  {Align1}
        G4_Declare *srcDcl = source_send->getSrc(0)->getBase()->asRegVar()->getDeclare();
        G4_SrcRegRegion* newSrcOpnd = builder.Create_Src_Opnd_From_Dcl(srcDcl, builder.getRegionStride1());

        G4_INST* mov = builder.createMov(
            m->getExecSize(),
            builder.duplicateOperand(m->getDst()),
            newSrcOpnd,
            m->getOption(),
            false);
        bb->insert(pos, mov);

        // maintain def-use.
        //
        // (1) Uses. m is ready to be deleted.
        m->transferUse(mov);

        // (2) Defs
        // The defs should be from definitions for source->send's src(0).
        //
        // mov (8) V244(1,0)<1>:ud V88(0,0)<8;8,1>:ud {Align1, NoMask}
        // mov (8) V244(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
        // mov (1) V244(0,2)<1>:ud 0x1f:ud {Align1, NoMask}
        // mov (1) V244(0,0)<1>:ud 0:uw {Align1, NoMask}
        // mov (1) V244(0,1)<1>:ud 0:uw {Align1, NoMask}
        // add (1) a0.0<1>:ud r1.1<0;1,0>:ud 0x40a8000:ud {Align1, NoMask}
        // send (8) null<1>:ud V244(0,0)<8;8,1>:ud a0.0<0;1,0>:ud {Align1, NoMask}  <- source_send
        // mov (8) V89(0,0)<1>:d V34_histogram(1,0)<8;8,1>:d {Align1, Q1}
        // mov (8) V246(1,0)<1>:ud V89(0,0)<8;8,1>:ud {Align1, NoMask}
        // mov (8) V246(0,0)<1>:ud V244(0,0)<8;8,1>:ud {Align1, NoMask} <-- mov
        // mov (8) V246(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}      <-- m
        //
        // There are more than one defs here.
        //
        // Note that
        //
        // mov (8) V244(1,0)<1>:ud V88(0,0)<8;8,1>:ud {Align1, NoMask}
        //
        // is a definition of send but not for mov. We enable checked
        // while copying defs.
        source_send->copyDef(mov, Opnd_src0, Opnd_src0, /*checked*/true);
    }

    /*
    * compare the two instructions
    * they must have:
    * the same instruction opcode, predicate, condmodifier, optionString
    * the same dst, and number of src args
    */
    bool Optimizer::isHeaderOptCandidate(G4_INST *dst, G4_INST *src)
    {
        if ( !dst   ||
            !src )
        {
            return true;
        }

        // Compare instructions
        if ( dst->opcode() != src->opcode()             ||
            dst->getOption() != src->getOption()       ||
            dst->getExecSize() != src->getExecSize()   ||
            dst->getPredicate() != src->getPredicate() ||
            dst->getCondMod() != src->getCondMod())
        {
            return false;
        }

        if ( dst->getNumSrc() != src->getNumSrc() )
        {
            return false;
        }

        // Compare destination args
        G4_Operand *dst_dest = dst->getDst();
        G4_Operand *src_dest = src->getDst();
        if ( !dst_dest   ||
            !src_dest   )
        {
            return false;
        }

        return true;
    }

    /*
    * compare the two instructions to see if they are redundent
    * they must have:
    * the same src args
    * the same def
    */
    bool Optimizer::isHeaderOptReuse(G4_INST *dst, G4_INST *src)
    {
        if ( !dst   &&
            !src )
        {
            return true;
        }
        if ( !dst   ||
            !src   )
        {
            return false;
        }

        for ( unsigned int i = 0; i < G4_MAX_SRCS; i++)
        {
            G4_Operand* opnd = dst->getSrc(i);

            if ( opnd != NULL && opnd->compareOperand( src->getSrc(i) ) != Rel_eq )
            {
                return false;
            }
        }

        // The same number of def instructions
        if (dst->def_size() != src->def_size())
        {
            return false;
        }

        if (dst->def_size() == 0)
        {
            return true; // both have no def at all
        }

        for (auto ii = dst->def_begin(); ii != dst->def_end(); ii++)
        {
            bool sameDef = false;
            for (auto jj = src->def_begin(); jj != src->def_end(); jj++)
            {
                if ((*ii).first == (*jj).first &&
                    (*ii).second == (*jj).second)
                {
                    sameDef = true;
                    break; // break the inner jj loop
                }
            }
            if (sameDef == false)
            {
                return false;
            }
        }
        return true;
    }

    bool Optimizer::headerOptValidityCheck(MSGTable *dest, MSGTable *source)
    {
        if ( !isHeaderOptCandidate(dest->a0Dot0, source->a0Dot0)   ||
            !isHeaderOptCandidate(dest->mDot0,  source->mDot0)    ||
            !isHeaderOptCandidate(dest->m,      source->m)        ||
            !isHeaderOptCandidate(dest->mDot1,  source->mDot1)    ||
            !isHeaderOptCandidate(dest->mDot2, source->mDot2) )
        {
            return false;
        }

        if ( dest->m )
        {
            if ( !(dest->m->hasOneUse() && dest->m->use_front().first == dest->send) )
            {
                return false;
            }
        }
        if ( dest->mDot0 )
        {
            if ( !(dest->mDot0->hasOneUse() && dest->mDot0->use_front().first == dest->send) )
            {
                return false;
            }
        }
        if ( dest->mDot1 )
        {
            if ( !(dest->mDot1->hasOneUse() && dest->mDot1->use_front().first == dest->send) )
            {
                return false;
            }
        }
        if ( dest->mDot2 )
        {
            if ( !(dest->mDot2->hasOneUse() && dest->mDot2->use_front().first == dest->send) )
            {
                return false;
            }
        }

        if ( dest->send                         &&
            dest->send->getSrc(0)              &&
            dest->send->getSrc(0)->getTopDcl() &&
            source->send                       &&
            source->send->getSrc(0)            &&
            source->send->getSrc(0)->getTopDcl() )
        {
            unsigned short dstSize, sourceSize;
            dstSize    = dest->send->getSrc(0)->getTopDcl()->getTotalElems() *
                dest->send->getSrc(0)->getTopDcl()->getElemSize();
            sourceSize = source->send->getSrc(0)->getTopDcl()->getTotalElems() *
                source->send->getSrc(0)->getTopDcl()->getElemSize();
            if ( dstSize != sourceSize )
            {
                return false;
            }
        }

        return true;
    }

    // a class to store all presently valid values, each value is an instruction
    // if the number of values exceeds the max allowed, the oldest is removed
    class InstValues
    {
        const int maxNumVal;
        std::list<G4_INST*> values;

    public:
        InstValues(int maxCount) : maxNumVal(maxCount) {}

        void addValue(G4_INST* inst)
        {
            if (values.size() == maxNumVal)
            {
                values.pop_front();
            }
            values.push_back(inst);
        }

        // delete all values that may be invalid after inst
        void deleteValue(G4_INST* inst)
        {
            if (inst->isOptBarrier())
            {
                values.clear();
                return;
            }

            auto hasIndirectGather = [](G4_INST *inst)
            {
                for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                {
                    auto src = inst->getSrc(i);
                    if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->isIndirect()
                        && src->asSrcRegRegion()->getRegion()->isRegionWH())
                    {
                        return true;
                    }
                }
                return false;
            };

            if (hasIndirectGather(inst))
            {
                // optimization is likely unprofitable due to high address register pressure in this case.
                // more importantly, it may actually cause RA to fail since we don't spill physical a0.0
                values.clear();
                return;
            }

            G4_DstRegRegion* dst = inst->getDst();
            auto interferes = [dst](G4_INST* valInst)
            {
                G4_DstRegRegion* valDst = valInst->getDst();
                if (dst->compareOperand(valDst) != Rel_disjoint)
                {
                    return true;
                }
                for (int i = 0, numSrc = valInst->getNumSrc(); i < numSrc; ++i)
                {
                    G4_Operand* src = valInst->getSrc(i);
                    if (src != nullptr && dst->compareOperand(src) != Rel_disjoint)
                    {
                        return true;
                    }
                }
                return false;
            };
            if (dst != nullptr)
            {
                values.remove_if(interferes);
            }
        }

        G4_INST* findValue(G4_INST* inst)
        {
            for (auto valInst : values)
            {
                if (inst->opcode() != valInst->opcode() ||
                    inst->getExecSize() != valInst->getExecSize())
                {
                    continue;
                }
                // skip flags for now
                if ((inst->getPredicate() || valInst->getPredicate()) ||
                    (inst->getCondMod() || valInst->getCondMod()))
                {
                    continue;
                }
                // emask checks
                if (inst->getMaskOffset() != valInst->getMaskOffset() ||
                    inst->isWriteEnableInst() ^ valInst->isWriteEnableInst())
                {
                    continue;
                }
                // all source should be isomorphic (same type/shape)
                bool srcMatch = true;
                for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                {
                    G4_Operand* src = inst->getSrc(i);
                    G4_Operand* valSrc = valInst->getSrc(i);
                    if (src == nullptr || valSrc == nullptr || src->compareOperand(valSrc) != Rel_eq)
                    {
                        srcMatch = false;
                        break;
                    }
                }
                if (srcMatch)
                {
                    return valInst;
                }
            }
            return nullptr;
        }

        void clear()
        {
            values.clear();
        }
    };

    G4_Operand* Optimizer::updateSendsHeaderReuse(
        std::vector<std::vector<G4_INST*>> &instLookUpTable,
        std::vector<G4_INST*> &iVector, INST_LIST_ITER endIter)
    {
        int bSize = (int)iVector.size();
        for (auto & Cache : instLookUpTable)
        {
            if (Cache.size() == bSize)
            {
                bool match[8] = { false };
                bool anyMatch = false;
                for (int index = 0; index < bSize; ++index)
                {
                    G4_INST* cInst = Cache[index];
                    G4_INST* iInst = iVector[index];

                    // opcode check
                    if (cInst->opcode() != iInst->opcode() ||
                        cInst->getExecSize() != iInst->getExecSize())
                    {
                        continue;
                    }
                    // flag check
                    if (cInst->getPredicate() != iInst->getPredicate() ||
                        cInst->getCondMod() != iInst->getCondMod())
                    {
                        continue;
                    }
                    // emask check
                    if (cInst->getMaskOffset() != iInst->getMaskOffset() ||
                        cInst->isWriteEnableInst() ^ iInst->isWriteEnableInst())
                    {
                        continue;
                    }
                    // dst check
                    G4_DstRegRegion *cDstRgn = cInst->getDst();
                    G4_DstRegRegion *iDstRgn = iInst->getDst();
                    if (cDstRgn->getRegOff() != iDstRgn->getRegOff() ||
                        cDstRgn->getSubRegOff() != iDstRgn->getSubRegOff() ||
                        cDstRgn->getHorzStride() != iDstRgn->getHorzStride() ||
                        cDstRgn->getRegAccess() != iDstRgn->getRegAccess() ||
                        cDstRgn->getType() != iDstRgn->getType())
                    {
                        continue;
                    }

                    // all source should be isomorphic (same type/shape) and unaltered between declaration and reuse
                    bool srcMatch = true;

                    for (int iSrc = 0, numSrc = cInst->getNumSrc(); iSrc < numSrc; ++iSrc)
                    {
                        G4_Operand* cOpnd = cInst->getSrc(iSrc);
                        G4_Operand* iOpnd = iInst->getSrc(iSrc);
                        if (cOpnd == nullptr || iOpnd == nullptr || cOpnd->compareOperand(iOpnd) != Rel_eq)
                        {
                            srcMatch = false;
                            break;
                        }
                    }

                    if (chkBwdWARdep(cInst, endIter))
                        srcMatch = false;

                    match[index] = srcMatch;
                    anyMatch |= srcMatch;
                }

                if (anyMatch)
                {
                    // at least partial match

                    for (int index = 0; index < bSize; ++index)
                    {
                        G4_INST* cInst = Cache[index];
                        G4_INST* iInst = iVector[index];

                        // mark it if there is match
                        if (match[index])
                        {
                            iInst->markDead();
                            continue;
                        }

                        // create new dst region to replace one in iVector[i]
                        G4_DstRegRegion *cDst = cInst->getDst();
                        G4_DstRegRegion *iDst = iInst->getDst();
                        G4_DstRegRegion *newDstRegion = builder.createDst(cDst->getTopDcl()->getRegVar(),
                                iDst->getRegOff(), iDst->getSubRegOff(), iDst->getHorzStride(), iDst->getType());
                        iInst->setDest(newDstRegion);

                        // update the look-up table list
                        Cache[index] = iInst;
                    }
                    return Cache[0]->getDst();
                }
            }
        }
        return nullptr;
    }

    //
    // Perform value numbering on writes to the extended msg descriptor for bindless access of the form
    // op (1) a0.2<1>:ud src0 src1 src2 {NoMask}
    // and remove redundant instructions.  This is limited to within BB
    //
    void Optimizer::cleanupBindless()
    {
        // Perform send header cleanup for bindless sampler/surface
        for (auto bb : fg)
        {
            std::vector<std::vector<G4_INST*>> instLookUpTable;
            std::vector<G4_INST*> instVector;
            for (auto iter = bb->begin(), iterEnd = bb->end();
                iter != iterEnd; ++iter)
            {
                G4_INST *inst = *iter;
                G4_DstRegRegion *dst = inst->getDst();

                if (dst != nullptr && dst->getTopDcl() != nullptr &&
                    dst->getTopDcl()->getCapableOfReuse())
                {
                    // it is header definition instruction
                    instVector.push_back(inst);
                }

                if (inst->isSplitSend())
                {
                    G4_Operand *header = inst->getSrc(0);
                    G4_Operand *exDesc = inst->getSrc(3);

                    if (header->getTopDcl() && header->getTopDcl()->getCapableOfReuse() &&
                        exDesc->isSrcRegRegion())
                    {

                        if (instVector.size() != 0)
                        {
                            // check if we can reuse cached values
                            G4_Operand *value = updateSendsHeaderReuse(instLookUpTable, instVector, iter);
                            if (value == nullptr)
                            {
                                // no found, cache the header
                                instLookUpTable.push_back(instVector);
                            }
                            else
                            {
                                // update sends header src
                                G4_SrcRegRegion* newHeaderRgn = builder.createSrcRegRegion(
                                    Mod_src_undef, Direct, value->getBase(), 0, 0, builder.getRegionStride1(), Type_UD);
                                inst->setSrc(newHeaderRgn, 0);
                            }
                        }
                    }
                    // clear header def
                    instVector.clear();
                }
                else if (inst->isSend())
                {
                    instVector.clear();
                }
            }
            bb->erase(
                std::remove_if(bb->begin(), bb->end(), [](G4_INST* inst) { return inst->isDead(); }),
                bb->end());
        }

        for (auto bb : fg)
        {
            InstValues values(4);
            for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd;)
            {
                G4_INST* inst = *iter;

                auto isDstExtDesc = [](G4_INST* inst)
                {
                    G4_DstRegRegion* dst = inst->getDst();
                    if (dst && dst->getTopDcl() && dst->getTopDcl()->isMsgDesc())
                    {
                        // check that its single use is at src3 of split send
                        if (inst->use_size() != 1)
                        {
                            return false;
                        }
                        auto use = inst->use_front();
                        G4_INST* useInst = use.first;
                        if (useInst->isSend())
                        {
                            return true;
                        }
                    }
                    return false;
                };

                if (isDstExtDesc(inst))
                {
                    G4_INST* valInst = values.findValue(inst);
                    if (valInst != nullptr)
                    {
#if 0
                        std::cout << "can replace \n";
                        inst->emit(std::cout);
                        std::cout << "\n with \n";
                        valInst->emit(std::cout);
                        std::cout << "\n";
#endif
                        for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
                        {
                            // each use is in the form of A0(0,0)<0;1,0>:ud in a send
                            G4_INST* useInst = I->first;
                            Gen4_Operand_Number num = I->second;
                            assert(useInst->isSend() && "use inst must be a send");
                            G4_SrcRegRegion* newExDesc = builder.createSrcRegRegion(
                                Mod_src_undef, Direct, valInst->getDst()->getBase(), 0, 0,
                                builder.getRegionScalar(), Type_UD);
                            useInst->setSrc(newExDesc, useInst->getSrcNum(num));
                        }
                        iter = bb->erase(iter);
                        continue;
                    }
                    else
                    {
#ifdef DEBUG_VERBOSE_ON
                        std::cout << "add new value:\n";
                        inst->emit(std::cout);
                        std::cout << "\n";
#endif
                        // this is necessary since for msg desc we always the physical a0.0,
                        // so a new inst will invalidate the previous one
                        values.deleteValue(inst);
                        values.addValue(inst);
                    }
                }
                else
                {
                    values.deleteValue(inst);
                }
                ++iter;
            }
        }
    }

    /*
    * compare the two send and their defs
    * determine whether to remove the redundant mov inst
    * or reuse the previous header
    *
    * 1    mov (8) V152(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
    * 2    mov (1) V152(0,2)<1>:ud 0x7000f:ud {Align1, NoMask}
    * 3    mov (1) V152(0,0)<1>:ud 0:uw {Align1, NoMask}
    * 4    mov (1) V152(0,1)<1>:ud 0:uw {Align1, NoMask}
    * 5    add (1) a0.0<1>:ud r1.0<0;1,0>:ud 0x2490000:ud {Align1, NoMask}
    * 6    send (8) V32_in(0,0)<1>:ud V152(0,0)<8;8,1>:ud a0.0<0;1,0>:ud {Align1, NoMask}
    *
    * 7    mov (8) V154(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
    * 8    mov (1) V152(0,2)<1>:ud 0x1f:ud {Align1, NoMask}
    * 9    mov (1) V154(0,0)<1>:ud 0:uw {Align1, NoMask}
    * 10   mov (1) V154(0,1)<1>:ud 0:uw {Align1, NoMask}
    * 11   add (1) a0.0<1>:ud r1.1<0;1,0>:ud 0x2190000:ud {Align1, NoMask}
    * 12   send (8) V33(0,0)<1>:d V152(0,0)<8;8,1>:ud a0.0<0;1,0>:ud {Align1, NoMask}
    *
    * It is rather tricky to maintain def-use chains for this optimization.
    * The send instruction (Line 12) can reuse Inst[1, 3, 4] and need to
    * keep Inst[8, 11]. The defs for send at line 12 is Inst[1, 3, 4, 8, 11]
    * and the defs for send at line 6 is Inst[1, 2, 3, 4, 5].
    *
    * We take the following approach to maintain def-use.
    *
    * - Starting with initial valid defs, [7, 8, 9, 10, 11].
    *
    * - Each to be removed instruction transfers its use to a proper
    *   previous definition.
    *
    * - Each to be kept instruction remains, even there may have changes
    *   in its definition. For example, dst of move instruction at Line 8
    *   is changed from V154 to V152, but no def-use modification should
    *   be made for this instruction.
    *
    * - No def-use modification should be made to the final send, since
    *   all have been properly set.
    */
    void Optimizer::optMessageHeaders(MSGTableList& msgList,
        G4_BB* bb,
        DEFA0& myA0)
    {
        unsigned char redundancyCount = 0;
        bool isSameX, isSameY, isSameSize;
        bool replaceOldHeader = false;

        uint16_t payLoadSize;

        MSGTable *dest, *source;
        MSGTable_ITER iter=msgList.begin();

        if(iter == msgList.end())
        {
            return;
        }
        dest = *iter;   // dest is the front
        iter++;
        if(iter == msgList.end())
        {
            return;
        }
        source = *iter; // source is the cached one

        if (!headerOptValidityCheck(dest, source))
        {
            return;
        }

        if ( isHeaderOptReuse(dest->a0Dot0, myA0.pred)  &&
            !myA0.isA0Redef )
        {
            // Transfer uses of dstDot0 to myA0.pred. This removes uses from
            // dest->a0Dot0 and add to myA0.pred. dest->a0Dot0 to be deleted.
            dest->a0Dot0->transferUse(myA0.pred, /*keepExisting*/true);
            dest->a0Dot0->markDead();
        }

        payLoadSize = dest->send->getMsgDesc()->MessageLength();

        isSameX = isHeaderOptReuse(dest->mDot0, source->mDot0)      &&
            !source->isXRedef ;

        isSameY = isHeaderOptReuse(dest->mDot1, source->mDot1)      &&
            !source->isYRedef;

        isSameSize = isHeaderOptReuse(dest->mDot2, source->mDot2)   &&
            !source->isSizeRedef;

        if ( isSameX && dest->mDot0 )
        {
            redundancyCount++;
        }
        if ( isSameY && dest->mDot1 )
        {
            redundancyCount++;
        }
        if ( isSameSize && dest->mDot2 )
        {
            redundancyCount++;
        }

        if ( payLoadSize > 1                    &&
            redundancyCount < MESSAGE_HEADER_THRESHOLD )
        {
            return; // don't delete if redunant insts >=THRESHold
        };

        if (payLoadSize > 1 &&
            !(redundancyCount == 3 &&
                dest->send->getSrc(0)->compareOperand(source->send->getSrc(0))
                == Rel_eq))
        {
            dest->insertHeaderMovInst(source->send, builder, bb);
            replaceOldHeader = true;
        }

        {   // always remove "mov(8) Mx<1>, r0.0<8;8,1>:ud{Align1}"
            dest->m->markDead();
            if ( !replaceOldHeader )
            {
                dest->m->transferUse(source->m, /*keepExisting*/true);
                dest->m=source->m;
            }
        }

        if ( isSameX && dest->mDot0 )
        {
            dest->mDot0->markDead();
            if ( !replaceOldHeader )
            {
                dest->mDot0->transferUse(source->mDot0, /*keepExisting*/true);
                dest->mDot0 = source->mDot0;
            }
        }
        else if ( payLoadSize==1 && dest->mDot0 )
        {
            dest->reusePreviousHeader(dest->mDot0, source->mDot0, source->mDot2, builder);
            if ( !replaceOldHeader )
            {
                source->mDot0 = dest->mDot0;
            }
        }

        if ( isSameY && dest->mDot1 )
        {
            dest->mDot1->markDead();
            if ( !replaceOldHeader )
            {
                dest->mDot1->transferUse(source->mDot1, /*keepExisting*/true);
                dest->mDot1 = source->mDot1;
            }
        }
        else if ( payLoadSize==1 && dest->mDot1 )
        {
            dest->reusePreviousHeader(dest->mDot1, source->mDot1, source->mDot2, builder);
            if ( !replaceOldHeader )
            {
                source->mDot1 = dest->mDot1;
            }
        }

        if ( isSameSize && dest->mDot2 )
        {
            dest->mDot2->markDead();
            if ( !replaceOldHeader )
            {
                dest->mDot2->transferUse(source->mDot2, /*keepExisting*/true);
                dest->mDot2 = source->mDot2;
            }
        }
        else if ( payLoadSize==1 && dest->mDot2 )
        {
            dest->reusePreviousHeader(dest->mDot2, source->mDot2, source->mDot2, builder);
            if ( !replaceOldHeader )
            {
                source->mDot2 = dest->mDot2;
            }
        }

        if ( payLoadSize==1 )
        {
            // Check this function's comments for why no def-use changes
            // should be made on resetting src(0).
            G4_Operand *src0 = source->send->getSrc(0);
            dest->send->setSrc(builder.duplicateOperand(src0), 0);
        }

        dest->opt = true;

        return;
    }

    bool Optimizer::isHeaderCachingCandidate(G4_INST *inst)
    {
        if (inst->isSend())
        {
            return true;
        }

        if ( inst->useEmpty() )
        {
            return false;
        }

        for(USE_EDGE_LIST_ITER iter = inst->use_begin(), iend = inst->use_end(); iter != iend; ++iter )
        {
            if ((*iter).first->isSend())
            {
                G4_INST *send = (*iter).first;
                G4_Operand *header = send->getSrc(0);
                G4_DstRegRegion *dst = inst->getDst();

                //def to BuiltInA0 is part of header opt
                if (inst->getDst()                                            &&
                    inst->getDst()->getBase()                   &&
                    inst->getDst()->getBase()->isRegVar()       &&
                    inst->getDst()->getBase()->asRegVar() ==
                    builder.getBuiltinA0()->getRegVar()                         &&
                    inst->getDst()->getRegOff() == 0            &&
                    inst->getDst()->getSubRegOff() == 0         )
                {
                    return true;
                }

                // make sure that dst of the current inst is header, not payload
                // header is hard-coded to be 32 bytes
                if ( header->getTopDcl()    == dst->getTopDcl()         &&
                    dst->getLeftBound() >= header->getLeftBound()      &&
                    dst->getRightBound() <= header->getLeftBound() + GENX_GRF_REG_SIZ -1 )
                {
                    return true;
                }
                return false;
            }
        }

        return false;
    }

   /*
    * mark the below "and" instruction as redundant;
    * the "and" instruction is the header for a barrier:
    *   and (8) r1.0<1>:ud r0.2<0;1,0>:ud 0xf000000:ud {Align1, NoMask}
    *   send (1) null<1>:ud r1 0x3 0x2000004:ud{Align1}
    *   wait n0:ud {Align1}
    */
    void Optimizer::removeRedundantBarrierHeaders(G4_INST *sendInst,
                                          G4_SrcRegRegion* barrierSrc0,
                                          bool first)
    {
        bool barrier = false;
        G4_SrcRegRegion *src0 = NULL;
        if (!first) // skip the check as already done so for first barrier
        {
            barrier = isBarrierPattern(sendInst, src0);
        }
        if (barrier || first)
        {
            auto item = sendInst->def_begin();
            G4_INST *andInst = item->first;
            // delete all the uses of andInst
            // addInst and sendInst will have no def and no use
            andInst->removeAllUses();
            // sendInst.src0 (addInst.dst) will be replaced by barrierSend.src0
            // create a new G4_SrcRegRegion, which is a copy of barrierSend.src0
            G4_SrcRegRegion *src = builder.createSrcRegRegion(
                    Mod_src_undef,
                    Direct,
                    barrierSrc0->getTopDcl()->getRegVar(),
                    0,
                    0,
                    barrierSrc0->getRegion(),
                    barrierSrc0->getTopDcl()->getElemType() );
            sendInst->setSrc(src, 0);
            andInst->markDead();
        }
    }

    /*
    * pattern match of a code sequence for ISA_BARRIER:
    *   and (8) r1.0<1>:ud r0.2<0;1,0>:ud 0xf000000:ud {Align1, NoMask}
    *   send (1) null<1>:ud r1 0x3 0x2000004:ud{Align1}
    *   wait n0:ud {Align1}
    */
    bool Optimizer::isBarrierPattern(G4_INST *sendInst,
                                     G4_SrcRegRegion *& barrierSendSrc0)
    {
        /*
         * check G4_send
         */
        G4_SendMsgDescriptor *desc = sendInst->getMsgDesc();
        uint32_t descVal = desc->getDesc();
        if ((desc->getFuncId() == SFID::GATEWAY) &&
            (descVal == (0x1 << 25) + 0x4) && // 0x2000004
            (sendInst->def_size() == 1))
        {
            auto item = sendInst->def_begin();
            G4_INST *andInst = item->first; // getting "and" from send's def

           /*
            * check G4_and
            */
            if ((andInst) && (andInst->opcode() == G4_and) &&
                (item->second == Opnd_src0))
            {
                G4_Operand *src0 = andInst->getSrc(0);
                G4_Operand *src1 = andInst->getSrc(1);

                bool isSrc0 =
                    ((src0->isSrcRegRegion()) &&
                     (src0->asSrcRegRegion()->getBase()) &&
                     (src0->asSrcRegRegion()->getBase()->isRegVar()) &&
                     (src0->asSrcRegRegion()->getBase()->asRegVar() ==
                      builder.getBuiltinR0()->getRegVar()) &&
                     (src0->asSrcRegRegion()->getRegOff() == 0) &&
                     (src0->asSrcRegRegion()->getSubRegOff() == 2)); // r0.2

                bool isSrc1 = src1->isImm() && !src1->isRelocImm() &&
                    src1->asImm()->getInt() == (builder.getPlatform() >= GENX_SKL ? 0x8F000000 : 0x0F000000);

                if (isSrc0 && isSrc1 && sendInst->getSrc(0) &&
                    sendInst->getSrc(0)->isSrcRegRegion())
                {
                    barrierSendSrc0 = sendInst->getSrc(0)->asSrcRegRegion();
                    return true;
                }
            }
        }

        return false;
    }

    /*
     * add the the barrier header as the top instruction
     */
    void Optimizer::hoistBarrierHeaderToTop(G4_SrcRegRegion *barrierSendSrc0)
    {
        G4_Declare *dcl = barrierSendSrc0->getTopDcl();
        IR_Builder *mybuilder = &builder;

        // below code is copied from translateVISASyncInst() for ISA_BARRIER
        // all other dwords are ignored
        // and (8) r32.0:ud r0.2:ud 0x0F000000

        G4_SrcRegRegion* r0_src_opnd = builder.createSrcRegRegion(
            Mod_src_undef,
            Direct,
            mybuilder->getBuiltinR0()->getRegVar(),
            0,
            2,
            builder.getRegionScalar(),
            Type_UD);
        G4_DstRegRegion *dst1_opnd = builder.createDst(
            dcl->getRegVar(),
            0,
            0,
            1,
            Type_UD);

        G4_Imm * g4Imm = NULL;

        //for SKL+ there are 5 bits for barrierID
        //5th bit is stored in bit 31 of second dword
        if (builder.getPlatform() < GENX_SKL)
        {
            g4Imm = builder.createImm(0x0F000000, Type_UD);
        }
        else
        {
            g4Imm = builder.createImm(0x8F000000, Type_UD);
        }

        G4_INST *andInst = builder.createBinOp(G4_and, 8, dst1_opnd, r0_src_opnd, g4Imm, InstOpt_WriteEnable, false);
        for (auto bb : fg)
        {
            auto iter = std::find_if(bb->begin(), bb->end(), [](G4_INST* inst) { return !inst->isLabel();});
            if (iter != bb->end())
            {
                bb->insert(iter, andInst);
                return;
            }
        }
    }

    /*
     * check whether there are new definitions in order to determine redundancy
     */
    bool Optimizer::chkNewDefBetweenSends(G4_INST *inst, MSGTableList& msgList, DEFA0& myA0)
    {
        bool isDef = false;
        msgList.unique();

        // check SIMD8 VxH region everytime
        if ( inst->getDst()                         &&
            inst->getDst()->isAddress() )
        {
            isDef = myA0.isA0Redef = true;
        }
        else
        {
            for (int i = 0; i < inst->getNumSrc(); i++)
            {
                if (inst->getSrc(i)                     &&
                    inst->getSrc(i)->isAddress() )
                {
                    isDef = myA0.isA0Redef = true;
                    break;
                }
            }
        }

        if ( msgList.size() < 2 )
        {
            return false;
        }
        MSGTable_ITER ii = msgList.begin();
        if(ii == msgList.end())
        {
            return false;
        }

        ii++;

        if(ii == msgList.end())
        {
            return false;
        }

        MSGTable* last = *(ii);
        if ( last == NULL       ||
            last->send == NULL )
        {
            return false;
        }
        G4_Operand *def = inst->getDst();
        if ( def == NULL)
        {
            return false;
        }
        if ( last->mDot0                                        &&
             ( def->compareOperand(last->mDot0->getSrc(0)) == Rel_eq ||
               ( last->mDot0->getSrc(1) && def->compareOperand(last->mDot0->getSrc(1)) == Rel_eq ) ||
                ( last->mDot0->getSrc(2) && def->compareOperand(last->mDot0->getSrc(2)) == Rel_eq ) ) )
        {
            isDef = last->isXRedef = true;
        }
        else if ( last->mDot1                                   &&
             ( def->compareOperand(last->mDot1->getSrc(0)) == Rel_eq ||
               ( last->mDot1->getSrc(1) && def->compareOperand(last->mDot1->getSrc(1)) == Rel_eq ) ||
               ( last->mDot1->getSrc(2) && def->compareOperand(last->mDot1->getSrc(2)) == Rel_eq ) ) )
        {
            isDef = last->isYRedef = true;
        }
        else if ( last->mDot2                                   &&
             ( def->compareOperand(last->mDot2->getSrc(0)) == Rel_eq  ||
               ( last->mDot2->getSrc(1) && def->compareOperand(last->mDot2->getSrc(1)) == Rel_eq ) ||
               ( last->mDot2->getSrc(2) && def->compareOperand(last->mDot2->getSrc(2)) == Rel_eq ) ) )
        {
             isDef = last->isSizeRedef = true;
        }
        else if ( last->m                                       &&
            ( def->compareOperand(last->m->getSrc(0)) == Rel_eq ||
              ( last->m->getSrc(1) && def->compareOperand(last->m->getSrc(1)) == Rel_eq ) ||
              ( last->m->getSrc(2) && def->compareOperand(last->m->getSrc(2)) == Rel_eq ) ) )
        {
            isDef = last->isR0Dot0Redef = true;
        }
        return isDef;

    }

    /*
    * Cache the send and its def into a table for optimization
    */
    void Optimizer::addEntryToMessageTable(G4_INST *inst,
        MSGTableList& msgList,
        G4_BB* bb,
        INST_LIST_ITER ii,
        DEFA0 &myA0)
    {
        MSGTable *item = msgList.front();
        if ( inst->isSend() )
        {
            item->send = inst;
            item->opt           = false;
            item->isR0Dot0Redef = false;
            item->isXRedef      = false;
            item->isYRedef      = false;
            item->isSizeRedef   = false;

            if (item->invalid)
            {
                msgList.pop_front();
            }
            else if (item->a0Dot0 != NULL &&   // only def a0.0
                 (item->m == NULL  || item->mDot2 == NULL))
            {
                if ( isHeaderOptCandidate(item->a0Dot0, myA0.pred) )
                {
                    if ( isHeaderOptReuse(item->a0Dot0, myA0.pred)  &&
                        !myA0.isA0Redef )
                    {
                        // Transfer uses of a0Dot0 to myA0.pred. This removes uses from
                        // a0Dot0 and add to myA0.pred. item->a0Dot0 to be deleted.
                        item->a0Dot0->transferUse(myA0.pred, /*keepExisting*/true);
                        item->a0Dot0->markDead();
                    }
                }
                msgList.pop_front();
            }
            else if (item->a0Dot0 &&  item->m && item->mDot2) // complete header def
            {

                msgList.unique();
                if (msgList.size() >= 2)
                {
                    optMessageHeaders(msgList, bb, myA0);
                    if (msgList.front()->opt                                    &&
                        msgList.front()->send->getMsgDesc()->MessageLength() == 1 )
                    {
                        // keep the oldest send for subsequent read operations
                        // but the instruction to define a0.0 needs to be latest
                        //msgList.back()->a0Dot0 = msgList.front()->a0Dot0;
                        msgList.pop_front(); // delete first element
                    }
                    else if (msgList.front()->opt                               &&
                        msgList.front()->send->getMsgDesc()->MessageLength() >= 1 )
                    {
                        // keep the latest send for subsequent write operations
                        msgList.pop_back();
                    }
                    else
                    {
                        msgList.pop_back();
                    }
                    myA0.isA0Redef      = false;
                }
            }
            else
            {
                // not an optimization candidate
                msgList.pop_front();
            }
        }
        else if ( inst->getDst()                                          &&
            inst->getDst()->getBase()                   &&
            inst->getDst()->getBase()->isRegVar()       &&
            inst->getDst()->getBase()->asRegVar() ==
            builder.getBuiltinA0()->getRegVar()                           &&
            inst->getDst()->getRegOff() == 0            &&
            inst->getDst()->getSubRegOff() == 0         )
        {
            // is builtInA0.0
            item->a0Dot0 = inst;
            item->a0Dot0_it = ii;

            if ( myA0.curr == NULL )
            {
                myA0.pred = NULL;
                myA0.isA0Redef      = false;
            }
            else if (!myA0.curr->isDead())
            {
                // only update the a0 def when we didn't remove it
                myA0.pred = myA0.curr;
                myA0.predIt = myA0.currIt;
            }
            myA0.currIt = ii;
            myA0.curr = inst;
        }
       else if ( inst->getSrc(0) )
       {
            G4_DstRegRegion *dst = inst->getDst();
            if (dst)
            {
                if (dst->getRegOff() == 0)
                {
                    // mov(8) m.0, builtInR0.0
                    G4_Operand *src = inst->getSrc(0);
                    if( dst->getSubRegOff() == 0          &&
                        inst->getExecSize() == 8                            &&
                        src                                                 &&
                        src->isSrcRegRegion()                               &&
                        src->asSrcRegRegion()->getBase()                    &&
                        src->asSrcRegRegion()->getBase()->isRegVar()        &&
                        src->asSrcRegRegion()->getBase()->asRegVar()  ==
                        builder.getBuiltinR0()->getRegVar()            &&
                        src->asSrcRegRegion()->getRegOff() == 0             &&
                        src->asSrcRegRegion()->getSubRegOff() == 0 )
                    {
                        if (item->first == HEADER_UNDEF)
                            item->first = HEADER_FULL_REGISTER;
                        item->m = inst;
                        item->m_it = ii;
                    }
                    // mov(1) m.0
                    else if(dst->getSubRegOff() == 0  &&
                        inst->getExecSize() == 1                        )
                    {
                        if (item->first == HEADER_UNDEF)
                            item->first = HEADER_X;
                        item->mDot0 = inst;
                        item->mDot0_it = ii;
                    }
                    // mov(1) m.1
                    else if(dst->getSubRegOff() == 1  &&
                        inst->getExecSize() == 1                        )
                    {
                        if (item->first == HEADER_UNDEF)
                            item->first = HEADER_Y;
                        item->mDot1 = inst;
                        item->mDot1_it = ii;
                    }
                    // mov(1) m0.2
                    else if(dst->getSubRegOff() == 2  &&
                        inst->getExecSize() == 1                        )
                    {
                        if (item->first == HEADER_UNDEF)
                            item->first = HEADER_SIZE;
                        item->mDot2 = inst;
                        item->mDot2_it = ii;
                    }
                    else
                    {
                        // unrecognized update to header
                        item->invalid = true;
                    }
                }
            }
        }

    }

    void Optimizer::messageHeaderReport(size_t ic_before,
        size_t ic_after,
        G4_Kernel& kernel)
    {
        if ( builder.getOption(vISA_OptReport))
        {
            std::ofstream optReport;
            getOptReportStream(optReport, builder.getOptions());
            optReport << "             === Message Header Optimization ===" <<endl;
            optReport<< fixed << endl;
            optReport<< kernel.getName() <<" is reduced from "
                <<ic_before <<" to " <<ic_after <<" instructions. "<<endl;
            if (((float)(ic_before)) != 0.0)
            {
            optReport<< setprecision(0)
                << (float)((ic_before-ic_after)*100)/(float)(ic_before)
                << "% instructions of this kernel are removed."
                <<endl;
            }
            optReport<<endl;
            closeOptReportStream(optReport);
        }
    }

    /**
    *  optimizer for removal of redundant message header instructions
    */
    void Optimizer::cleanMessageHeader()
    {
        MSGTableList msgList;
        BB_LIST_ITER ib, bend(fg.end());
        size_t ic_before = 0;
        size_t ic_after = 0;

        std::stack<MSGTable*> toDelete;

        bool isRedundantBarrier = false;
        G4_SrcRegRegion *barrierSendSrc0 = NULL;

        for(ib = fg.begin(); ib != bend; ++ib)
        {

            msgList.clear();
            MSGTable *newItem   = (MSGTable *)mem.alloc(sizeof(MSGTable));
            toDelete.push(newItem);
            memset(newItem, 0, sizeof(MSGTable));
            newItem->first      = HEADER_UNDEF;

            msgList.push_front(newItem);
            G4_BB* bb = (*ib);
            INST_LIST_ITER ii = bb->begin();
            INST_LIST_ITER iend = bb->end();
            ic_before += bb->size();

            DEFA0 myA0;
            myA0.curr      = NULL;
            myA0.pred      = NULL;
            myA0.isA0Redef = false;

            for (; ii != iend; ii++ )
            {
                G4_INST *inst = *ii;
                if ( isHeaderCachingCandidate( inst ) )
                {
                    if ( inst->opcode() == G4_send && isRedundantBarrier )
                    {
                        removeRedundantBarrierHeaders(inst, barrierSendSrc0, false);
                    }
                    else if ( inst->opcode() == G4_send && !isRedundantBarrier )
                    {
                        isRedundantBarrier = isBarrierPattern(inst, barrierSendSrc0);
                        if (isRedundantBarrier)
                        {
                            removeRedundantBarrierHeaders(inst, barrierSendSrc0, true);
                        }
                    }

                    addEntryToMessageTable(inst, msgList, bb, ii, myA0);
                    if ( inst->isSend() )
                    {
                        MSGTable * item = (MSGTable *)mem.alloc(sizeof(MSGTable));
                        toDelete.push(item);
                        memset(item, 0, sizeof(MSGTable));
                        item->first  = HEADER_UNDEF;
                        msgList.push_front(item);
                    }
                }
                else
                {
                    chkNewDefBetweenSends(inst, msgList, myA0);
                }
            }

            // Dead code elimination
            for(ii = bb->begin(); ii != bb->end();)
            {
                G4_INST *inst = *ii;
                INST_LIST_ITER curr = ii++;
                if( inst->isDead() )
                {
                    inst->removeUseOfInst();
                    bb->erase(curr);
                }
            }

            ic_after += bb->size();
        }

        messageHeaderReport(ic_before, ic_after, kernel);

        if (isRedundantBarrier)
        {
            hoistBarrierHeaderToTop(barrierSendSrc0);
        }

        //
        // Destroy STL iterators allocated using mem manager
        //
        while( toDelete.size() > 0 )
        {
            toDelete.top()->~MSGTable();
            toDelete.pop();
        }
        msgList.clear();
    }
    /***  The end of message header optimization  ***/

    void getOptReportStream(std::ofstream& reportStream, const Options *opt)
    {
        char optReportFileName[MAX_OPTION_STR_LENGTH+20];
        const char *asmFileName;
        opt->getOption(VISA_AsmFileName, asmFileName);
        SNPRINTF(optReportFileName, MAX_OPTION_STR_LENGTH, "%s_optreport.txt", asmFileName);
        reportStream.open(optReportFileName, ios::out | ios::app  );
        MUST_BE_TRUE(reportStream, "Fail to open " << optReportFileName);
    }

    void closeOptReportStream(std::ofstream& reportStream)
    {
        reportStream.close();
    }

    void Optimizer::sendFusion()
    {
        (void) doSendFusion(&fg, &mem);
    }

    // For a subroutine, insert a dummy move with {Switch} option immediately
    // before the first non-label instruction in BB. Otherwie, for a following
    // basic block, insert a dummy move before *any* instruction to ensure that
    // no instruction should be placed between the targe jip/uip label and its
    // associated instruction.
    void Optimizer::addSwitchOptionToBB(G4_BB* bb, bool isSubroutine)
    {
        auto instIter = bb->begin();
        if (isSubroutine)
        {
            for (auto instEnd = bb->end(); instIter != instEnd; ++instIter)
            {
                G4_INST* bbInst = *instIter;
                if (bbInst->isLabel())
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        if (instIter != bb->end() && ((*instIter)->getOption() & InstOpt_Switch))
        {
            // this BB is already processed, skip
            return;
        }

        // mov (1) null<1>:ud r0.0<0;1,0>:ud {Switch}
        G4_DstRegRegion* movDst = builder.createNullDst(Type_UD);
        G4_SrcRegRegion* movSrc = builder.Create_Src_Opnd_From_Dcl(builder.getBuiltinR0(), builder.getRegionScalar());
        G4_INST* movInst = builder.createMov(1, movDst, movSrc, InstOpt_WriteEnable, false);
        movInst->setOptionOn(InstOpt_Switch);
        bb->insert(instIter, movInst);
    }

    void Optimizer::linePlaneWA(G4_INST* inst)
    {

        /*
        Putting it here instead of in HW confomrity because we need original src0 region
        in scheduler to calculate RB correctly. Otherwise setup moves for src0 get scheduled after instruction

        *  HW check #12: Check and correct the first operand for line instruction
        *  Actually it must be a replicated stream of 4 contiguous elements.
        *  That means <0;4,1> region. But in asm code it must be presented as
        *  replicated scalar - <0;1,0>.
        */
        if (inst->opcode() == G4_line || inst->opcode() == G4_pln)
        {
            G4_Operand *src = inst->getSrc(0);
            const RegionDesc *rd = src->isSrcRegRegion() ? src->asSrcRegRegion()->getRegion() : NULL;
            MUST_BE_TRUE( rd != NULL, " Src0 of line inst is not regregion. " );
            if (rd->isScalar())
            {
                return;
            }
            MUST_BE_TRUE(( rd->vertStride == 0 || rd->vertStride == 4 ) && rd->width == 4,
                "Unexpected region for the first line operand." );

            // create a new rd for src0
            const RegionDesc *new_rd = builder.getRegionScalar();
            src->asSrcRegRegion()->setRegion( new_rd );
        }
    }

    //
    // This inserts two dummy moves to clear flag dependencies before EOT:
    // mov(1) null:ud f0.0<0;1,0>:ud{ Align1, Q1, NoMask }
    // mov(1) null:ud f1.0<0;1,0>:ud{ Align1, Q1, NoMask }
    // This is done if f0/f1 is ever defined in a BB but not used in it, as we conservatively assume
    // that the flag may be undefined when the EOT is reached.
    // Note that USC only does this if EOT is inside control flow, i.e., EOT is an early exit
    //
    void Optimizer::clearARFDependencies()
    {
        auto flagToInt = [](G4_Areg* areg)
        {
            MUST_BE_TRUE(areg->isFlag(), "expect F0 or F1");
            return areg->getArchRegType() == AREG_F0 ? 0 : 1;
        };
        // see if F0 and F1 are ever defined but not used in the same BB
        bool unusedFlag[2]; // f0 and f1
        unusedFlag[0] = unusedFlag[1] = false;
        for (auto bb : fg)
        {
            bool unusedFlagLocal[2]; // f0 and f1
            unusedFlagLocal[0] = unusedFlagLocal[1] = false;

            for (auto inst : *bb)
            {
                // check predicate source
                if (inst->getPredicate())
                {
                    G4_VarBase* flag = inst->getPredicate()->getBase();
                    if (flag->isRegVar())
                    {
                        G4_Areg* areg = flag->asRegVar()->getPhyReg()->asAreg();
                        unusedFlagLocal[flagToInt(areg)] = false;
                    }
                }
                else
                {
                    // check explicit source
                    for (int i = 0; i < inst->getNumSrc(); ++i)
                    {
                        if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion() && inst->getSrc(i)->isFlag())
                        {
                            G4_SrcRegRegion* src = inst->getSrc(i)->asSrcRegRegion();
                            if (src->getBase()->isRegVar())
                            {
                                G4_Areg* flag = src->getBase()->asRegVar()->getPhyReg()->asAreg();
                                unusedFlagLocal[flagToInt(flag)] = false;
                            }
                        }
                    }
                }

                // check explicit dst
                if (inst->getDst() && inst->getDst()->isFlag())
                {
                    // flag is an explicit dst
                    G4_DstRegRegion* dst = inst->getDst();
                    if (dst->getBase()->isRegVar())
                    {
                        G4_Areg* flag = dst->getBase()->asRegVar()->getPhyReg()->asAreg();
                        unusedFlagLocal[flagToInt(flag)] = true;
                    }
                }
                // check cond mod
                else if (G4_VarBase* flag = inst->getCondModBase())
                {
                    if (flag->isRegVar())
                    {
                        G4_Areg* areg = flag->asRegVar()->getPhyReg()->asAreg();
                        unusedFlagLocal[flagToInt(areg)] = true;
                    }
                }
            }

            if (unusedFlagLocal[0] &&
                unusedFlag[0] == false)
            {
                unusedFlag[0] = true;
            }

            if (unusedFlagLocal[1] &&
                unusedFlag[1] == false)
            {
                unusedFlag[1] = true;
            }

            if (unusedFlag[0] && unusedFlag[1])
            {
                break;
            }
        }

        if (unusedFlag[0] || unusedFlag[1])
        {
            for (auto bb : fg)
            {
                if (bb->size() == 0)
                {
                    return;
                }
                G4_INST* inst = bb->back();
                if (inst->isEOT())
                {
                    auto instIter = bb->end();
                    --instIter;
                    if (unusedFlag[0])
                    {
                        G4_SrcRegRegion* flagSrc = builder.createSrcRegRegion(Mod_src_undef,
                            Direct, builder.phyregpool.getF0Reg(), 0, 0,
                            builder.getRegionScalar(), Type_UD);
                        G4_DstRegRegion* nullDst = builder.createNullDst(Type_UD);
                        G4_INST* inst = builder.createMov(1, nullDst, flagSrc, InstOpt_WriteEnable, false);
                        bb->insert(instIter, inst);
                    }
                    if (unusedFlag[1])
                    {
                        G4_SrcRegRegion* flagSrc = builder.createSrcRegRegion(Mod_src_undef,
                            Direct, builder.phyregpool.getF1Reg(), 0, 0,
                            builder.getRegionScalar(), Type_UD);
                        G4_DstRegRegion* nullDst = builder.createNullDst(Type_UD);
                        G4_INST* inst = builder.createMov(1, nullDst, flagSrc, InstOpt_WriteEnable, false);
                        bb->insert(instIter, inst);
                    }
                }
            }
        }
    }

    // change the send src0 region to be consistent with assembler expectation
    // We do it here instead of HW conformity since they only affect binary encoding
    // ToDo: this should not be necessary anymore, should see if we can remove
    void Optimizer::fixSendSrcRegion(G4_INST* inst)
    {
        if (inst->isSend() && inst->getSrc(0) != NULL)
        {
            const RegionDesc* newDesc = NULL;
            uint8_t execSize = inst->getExecSize();
            if (execSize == 1)
            {
                newDesc = builder.getRegionScalar();
            }
            else if (execSize > 8)
            {
                newDesc = builder.getRegionStride1();
            }
            else
            {
                newDesc = builder.createRegionDesc(execSize, execSize, 1);
            }
            inst->getSrc(0)->asSrcRegRegion()->setRegion(newDesc);
        }
    }


    /*
     *  Three sources only
     */
    bool Optimizer::hasGen12LPBundleConflict(G4_INST* inst)
    {
        int regs[3] = { -1 };
        int bundles[3] = { -1 };

        //Three source only
        if (inst->getNumSrc() != 3 || inst->isSend())
        {
            return false;
        }

        //SIMD16
        if (inst->getExecSize() < 16)
        {
            return false;
        }

        //Source 0 is scalar
        G4_Operand* srcOpnd = inst->getSrc(0);
        if (!srcOpnd || !srcOpnd->isSrcRegRegion() || !srcOpnd->asSrcRegRegion()->isScalar())
        {
            return false;
        }

        for (int i = 1; i < inst->getNumSrc(); i++)
        {
            srcOpnd = inst->getSrc(i);
            if (srcOpnd)
            {
                if (srcOpnd->isSrcRegRegion() &&
                    srcOpnd->asSrcRegRegion()->getBase() &&
                    srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
                {
                    G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                    if (baseVar->isGreg()) {
                        uint32_t byteAddress = srcOpnd->getLinearizedStart();
                        regs[i] = byteAddress / GENX_GRF_REG_SIZ;
                        bundles[i] = (regs[i] % 16) / 2;
                    }
                }
            }
        }

        if (bundles[1] == bundles[2] && bundles[1] != -1)
        {
            return true;
        }

        return false;
    }


    void Optimizer::swapSrc1(G4_INST* inst)
    {
        G4_Operand* src1Opnd = inst->getSrc(1);

        if (inst->getExecSize() < 2)
        {
            return;
        }

        if (!src1Opnd ||
            !src1Opnd->isSrcRegRegion() ||
            !src1Opnd->asSrcRegRegion()->isScalar())
        {
            return;
        }

        if (inst->isMixedMode())
        {
            return;
        }

        G4_Operand* src02Opnd = nullptr;

        if (inst->opcode() == G4_mad)
        {
            src02Opnd = inst->getSrc(2);
            if (src1Opnd->getType() == src02Opnd->getType())
            {
                inst->swapSrc(1, 2);
            }
        }

        if (inst->opcode() == G4_add ||
            inst->opcode() == G4_mul)
        {
            src02Opnd = inst->getSrc(0);
            if (src1Opnd->getType() == src02Opnd->getType())
            {
                inst->swapSrc(0, 1);
            }
        }

        return;
    }

    G4_SrcRegRegion* IR_Builder::createSubSrcOperand(
        G4_SrcRegRegion* src, uint16_t start, uint8_t size, uint16_t newVs, uint16_t newWd)
    {
        const RegionDesc* rd = NULL;
        uint16_t vs = src->getRegion()->vertStride, hs = src->getRegion()->horzStride, wd = src->getRegion()->width;
        // even if src has VxH region, it could have a width that is equal to the new exec_size,
        // meaning that it's really just a 1x1 region.
        auto isVxHRegion = src->getRegion()->isRegionWH() && wd < size;
        if (!isVxHRegion)
        {
            // r[a0.0,0]<4;2,1> and size is 4 or 1
            if (size < newWd)
            {
                newWd = size;
            }
            rd = size == 1 ? getRegionScalar() :
                createRegionDesc(size == newWd ? newWd * hs : newVs, newWd, hs);
            rd = getNormalizedRegion(size, rd);
        }

        if (src->getRegAccess() != Direct)
        {
            if (isVxHRegion)
            {
                // just handle <1,0>
                if (start > 0)
                {
                    // Change a0.N to a0.(N+start)
                    assert((start % wd == 0) && "illegal starting offset and width combination");
                    uint16_t subRegOff = src->getSubRegOff() + start / wd;
                    return createIndirectSrc(src->getModifier(), src->getBase(), src->getRegOff(), subRegOff,
                        src->getRegion(), src->getType(), src->getAddrImm());
                }
                else
                {
                    return duplicateOperand(src);
                }
            }

            if (start > 0)
            {
                short numRows = start / wd;
                short numCols = start % wd;
                short newOff = (numRows * vs + numCols * hs) * G4_Type_Table[src->getType()].byteSize;
                auto newSrc = createIndirectSrc(src->getModifier(), src->getBase(), src->getRegOff(), src->getSubRegOff(), rd,
                    src->getType(), src->getAddrImm() + newOff);
                return newSrc;
            }
            else
            {
                G4_SrcRegRegion* newSrc = duplicateOperand(src);
                newSrc->setRegion(rd);
                return newSrc;
            }
        }

        // direct access oprand
        uint16_t regOff, subRegOff;
        if (start > 0)
        {
            G4_Type srcType = src->getType();
            uint16_t newEleOff;
            uint16_t vs = src->getRegion()->vertStride, hs = src->getRegion()->horzStride, wd = src->getRegion()->width;

            if (src->isAccReg())
            {
                switch (srcType)
                {
                case Type_F:
                    // must be acc1.0 as result of simd16 -> 8 split
                    assert(size == 8 && "only support simd16->simd8 for now");
                    return createSrcRegRegion(src->getModifier(), Direct, phyregpool.getAcc1Reg(), 0, 0, src->getRegion(), srcType);
                case Type_HF:
                {
                    // can be one of acc0.8, acc1.0, acc1.8
                    if (src->getBase()->asAreg()->getArchRegType() == AREG_ACC1)
                    {
                        start += 16;
                    }
                    G4_Areg* accReg = start >= 16 ? phyregpool.getAcc1Reg() : phyregpool.getAcc0Reg();
                    return createSrcRegRegion(src->getModifier(), Direct, accReg, 0, start % 16, src->getRegion(), srcType);

                }
                default:
                    // Keep using acc0 for other types.
                    return duplicateOperand(src);
                }
            }

            newEleOff = start * hs +
                (start >= wd && vs != wd * hs ? (start / wd * (vs - wd * hs)) : 0);

            uint16_t newSubRegOff = src->getSubRegOff() + newEleOff;
            bool crossGRF = newSubRegOff * G4_Type_Table[srcType].byteSize >= G4_GRF_REG_NBYTES;
            if (crossGRF)
            {
                regOff = src->getRegOff() + 1;
                subRegOff = newSubRegOff - G4_GRF_REG_NBYTES / G4_Type_Table[srcType].byteSize;
            }
            else
            {
                regOff = src->getRegOff();
                subRegOff = newSubRegOff;
            }

            // create a new one
            return createSrcRegRegion(src->getModifier(), Direct, src->getBase(), regOff, subRegOff, rd,
                srcType, src->getAccRegSel());
        }
        else
        {
            G4_SrcRegRegion* newSrc = duplicateOperand(src);
            newSrc->setRegion(rd);
            return newSrc;
        }
    }

    G4_DstRegRegion* IR_Builder::createSubDstOperand(G4_DstRegRegion* dst, uint16_t start, uint8_t size)
    {
        if (dst->getRegAccess() != Direct)
        {
            if (start > 0)
            {
                // just change immediate offset
                uint16_t newOff = start * G4_Type_Table[dst->getType()].byteSize * dst->getHorzStride();
                G4_DstRegRegion* newDst = duplicateOperand(dst);
                newDst->setImmAddrOff(dst->getAddrImm() + newOff);
                return newDst;
            }
            else
            {
                return duplicateOperand(dst);
            }
        }

        uint16_t regOff, subRegOff;
        if (start > 0)
        {
            G4_Type dstType = dst->getType();
            uint16_t hs = dst->getHorzStride();
            if (dst->isAccReg())
            {
                switch (dstType)
                {
                case Type_F:
                    // must be acc1.0 as result of simd16 -> 8 split
                    assert(size == 8 && "only support simd16->simd8 for now");
                    return createDst(
                        phyregpool.getAcc1Reg(),
                        0,
                        0,
                        hs,
                        dstType);
                case Type_HF:
                {
                    // can be one of acc0.8, acc1.0, acc1.8
                    if (dst->getBase()->asAreg()->getArchRegType() == AREG_ACC1)
                    {
                        start += 16;
                    }
                    G4_Areg* accReg = start >= 16 ? phyregpool.getAcc1Reg() : phyregpool.getAcc0Reg();
                    return createDst(accReg, 0, start % 16, hs, dstType);
                }
                default:

                    // other types do not support acc1, we have to continue to use acc0
                    // whoever doing the split must fix the dependencies later by shuffling instructions
                    // so that acc0 does not get overwritten
                    return createDstRegRegion(*dst);
                }
            }

            uint16_t newSubRegOff = dst->getSubRegOff() + start * hs;
            bool crossGRF = newSubRegOff * G4_Type_Table[dstType].byteSize >= G4_GRF_REG_NBYTES;
            if (crossGRF)
            {
                regOff = dst->getRegOff() + 1;
                subRegOff = newSubRegOff - G4_GRF_REG_NBYTES / G4_Type_Table[dstType].byteSize;
            }
            else
            {
                regOff = dst->getRegOff();
                subRegOff = newSubRegOff;
            }
            // create a new one
            return createDst(dst->getBase(), regOff, subRegOff, hs, dst->getType(),
                dst->getAccRegSel());
        }
        else
        {
            G4_DstRegRegion* newDst = duplicateOperand(dst);
            return newDst;
        }
    }

    G4_INST* IR_Builder::makeSplittingInst(G4_INST* inst, uint8_t ExSize)
    {
        // Instruction's option is reused. Call sites should reset this field
        // properly. FIXME: fix all call sites.
        G4_INST* newInst = NULL;
        G4_opcode op = inst->opcode();
        if (inst->isMath())
        {
            newInst = createMathInst(NULL, inst->getSaturate(), ExSize,
                NULL, NULL, NULL, inst->asMathInst()->getMathCtrl(),
                inst->getOption(), inst->getLineNo());
            newInst->setCISAOff(inst->getCISAOff());
            newInst->setSrcFilename(inst->getSrcFilename());
        }
        else if (inst->getNumSrc() < 3)
        {
            newInst = createInternalInst(
                NULL, op, NULL, inst->getSaturate(), ExSize, NULL, NULL, NULL,
                inst->getOption(), inst->getLineNo(), inst->getCISAOff(),
                inst->getSrcFilename());
        }
        else
        {
            newInst = createInternalInst(
                NULL, op, NULL, inst->getSaturate(), ExSize, NULL, NULL, NULL,
                NULL, inst->getOption(), inst->getLineNo(), inst->getCISAOff(),
                inst->getSrcFilename());
        }

        return newInst;
    }

    // HW WAs that are done before RA.
    void Optimizer::preRA_HWWorkaround()
    {
        // -forceNoMaskWA : to force running this WA pass on platform other than TGLLP.
        // noMaskWA:  only apply on TGLLP
        //   bit[1:0]:  0 - off
        //              1 - on, replacing nomask in any divergent BB (conservative)
        //              2 - on, replacing nomask in nested divergent BB (aggressive)
        //              3 - not used, will behave the same as 2
        //     bit[2]:  0 - optimized. "emask flag" is created once per each BB
        //              1 - simple insertion of "emask flag". A new flag is created
        //                  each time it is needed, that is, created per each inst.
        //  (See comments for more details at doNoMaskWA().
        if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) != VISA_CM &&
            ((builder.getuint32Option(vISA_noMaskWA) & 0x3) > 0 ||
             builder.getOption(vISA_forceNoMaskWA)))
        {
            doNoMaskWA();
        }

        insertFenceAtEntry();
    }

    /*
    some workaround for HW restrictions.  We apply them here so as not to affect optimizations, RA, and scheduling
    [DevBDW:A]: A goto instruction must not be followed by any instruction requiring register indirect access on source operands.
    [DevBDW:A]: A join instruction must not be followed by any instruction requiring register indirect access on source operands.
    [DevBDW]: A POW/FDIV operation must not be followed by an instruction that requires two destination registers.
    For BDW A-stepping, we need a WA to prevent the following instructions:
    Send r10  (read from data port to r10)
    Send null r10 (write dataport from r10)
    Insert a dummy mov from r10
    [BDW+]: call/return's execution size must be set to 16/32 so that the emask will be passed correctly to the callee
    [BDW,CHV,SKL]: A call instruction must be followed by an instruction that supports Switch.
                   When call takes a jump, the first instruction must have a Switch.
    */
    void Optimizer::HWWorkaround()
    {
        // Ensure the first instruction of a stack function has switch option.
        if (fg.getIsStackCallFunc() &&
            VISA_WA_CHECK(builder.getPWaTable(), WaThreadSwitchAfterCall) &&
            !builder.getOption(vISA_enablePreemption))
        {
            addSwitchOptionToBB(fg.getEntryBB(), true);
        }

        // set physical pred/succ as it's needed for the call WA
        fg.setPhysicalPredSucc();
        BB_LIST_ITER ib, bend(fg.end());
        for(ib = fg.begin(); ib != bend; ++ib)
        {
            G4_BB* bb = (*ib);
            INST_LIST_ITER ii = bb->begin();

            while (ii != bb->end())
            {
                G4_INST *inst = *ii;

                G4_InstSend* sendInst = inst->asSendInst();
                if (sendInst && sendInst->isFence())
                {
                    // ToDo: replace with fence.wait intrinsic so we could hide fence latency by scheduling them apart
                    if (sendInst->getMsgDesc()->ResponseLength() > 0)
                    {
                        // commit is enabled for the fence, need to generate a move after to make sure the fence is complete
                        // mov (8) r1.0<1>:ud r1.0<8;8,1>:ud {NoMask}
                        INST_LIST_ITER nextIter = ii;
                        nextIter++;
                        G4_DstRegRegion* dst = inst->getDst();
                        G4_Declare* fenceDcl = dst->getBase()->asRegVar()->getDeclare();
                        G4_DstRegRegion* movDst = builder.createDst(
                            builder.phyregpool.getNullReg(), 0, 0, 1, fenceDcl->getElemType());
                        G4_SrcRegRegion* movSrc = builder.Create_Src_Opnd_From_Dcl(fenceDcl, builder.createRegionDesc(8, 8, 1));
                        G4_INST* movInst = builder.createMov(8, movDst, movSrc, InstOpt_WriteEnable, false);
                        bb->insert(nextIter, movInst);
                    }
                }

                if (inst->isCall() || inst->isFCall())
                {
                    if (VISA_WA_CHECK(builder.getPWaTable(), WaThreadSwitchAfterCall))
                    {
                        // WA:
                        // A call instruction must be followed by an instruction that supports Switch.
                        // When call takes a jump, the first instruction must have a Switch.
                        BB_LIST_ITER nextBBIter = ib;
                        ++nextBBIter;
                        if (nextBBIter != bend)
                        {
                            addSwitchOptionToBB(*nextBBIter, false);
                        }
                        // also do this for call target
                        addSwitchOptionToBB(bb->Succs.front(), true);
                    }
                }

                // we must set {Switch} if the instruction updates ARF with no scoreboard
                {
                    G4_DstRegRegion* dst = inst->getDst();
                    if (dst != nullptr && dst->getBase()->noScoreBoard())
                    {
                        inst->setOptionOn(InstOpt_Switch);
                    }
                }

                if (inst->isSend() && !inst->isNoPreemptInst() && builder.needsNoPreemptR2ForSend())
                {
                    G4_Operand *Src0 = inst->getSrc(0);
                    if (Src0 && Src0->isGreg())
                    {
                        unsigned LB = Src0->getLinearizedStart();
                        if (LB == 2 * GENX_GRF_REG_SIZ)
                        {
                            inst->setOptionOn(InstOpt_NoPreempt);
                        }
                    }
                }

                if (builder.hasFdivPowWA() &&
                    inst->isMath() &&
                    (inst->asMathInst()->getMathCtrl() == MATH_FDIV || inst->asMathInst()->getMathCtrl() == MATH_POW))
                {
                    INST_LIST_ITER nextIter = ii;
                    nextIter++;
                    if (nextIter == bb->end())
                    {
                        break;
                    }
                    // check next inst
                    G4_INST *nextInst = *nextIter;
                    if (!nextInst->isSend() && nextInst->getDst() && !nextInst->hasNULLDst() && nextInst->getDst()->crossGRF())
                    {
                        // insert a nop
                        G4_INST *nopInst = builder.createNop(inst->getOption());
                        bb->insert(nextIter, nopInst);
                    }
                }

                if (inst->isCall() || inst->isReturn())
                {
                    inst->setExecSize((unsigned char)kernel.getSimdSize());
                }

                // HW Workaround: for platforms without 64-bit regioning, change send src/dst type from QWord to DWord
                if (builder.no64bitRegioning() && inst->isSend())
                {
                    G4_DstRegRegion* dst = inst->getDst();
                    if (dst != NULL &&
                        G4_Type_Table[dst->getType()].byteSize == 8)
                    {
                        dst->setType(Type_D);
                    }

                    G4_Operand *src0 = inst->getSrc(0);
                    if (src0 != NULL &&
                        G4_Type_Table[src0->getType()].byteSize == 8)
                    {
                        src0->asSrcRegRegion()->setType(Type_D);
                    }

                    if (inst->isSplitSend())
                    {
                        G4_Operand *src1 = inst->getSrc(1);
                        if (src1 != NULL &&
                            G4_Type_Table[src1->getType()].byteSize == 8)
                        {
                            src1->asSrcRegRegion()->setType(Type_D);
                        }
                    }
                }

                if (inst->isEOT() && VISA_WA_CHECK(builder.getPWaTable(), WaClearTDRRegBeforeEOTForNonPS))
                {
                    // insert
                    // mov(8) tdr0:uw 0x0:uw {NoMask}
                    G4_DstRegRegion* tdrDst = builder.createDst(builder.phyregpool.getTDRReg(),
                        0, 0, 1, Type_UW);
                    G4_Imm* src = builder.createImm(0, Type_UW);
                    G4_INST* movInst = builder.createMov(8, tdrDst, src, InstOpt_WriteEnable | InstOpt_Switch, false);
                    bb->insert(ii, movInst);
                }

                if (inst->isEOT() && VISA_WA_CHECK(builder.getPWaTable(), Wa_14010017096))
                {
                    // insert "(W) mov(16) acc0.0:f 0x0:f" before EOT
                    G4_INST* movInst = builder.createMov(16,
                        builder.createDst(builder.phyregpool.getAcc0Reg(),0, 0, 1, Type_F),
                        builder.createImm(0, Type_F), InstOpt_WriteEnable, false)->InheritLLVMInst(inst);
                    // insert mov before contiguous send, in case that there are instruction combined set on continuous
                    // two send
                    INST_LIST_ITER insert_point = ii;
                    for (; insert_point != bb->begin(); --insert_point)
                        if (!(*insert_point)->isSend())
                            break;

                    if (!(*insert_point)->isEOT())
                        ++insert_point;
                    bb->insert(insert_point, movInst);
                }

                if (VISA_WA_CHECK(builder.getPWaTable(), WaResetN0BeforeGatewayMessage) &&
                    inst->isSend() && inst->getMsgDesc()->isBarrierMsg())
                {
                    // mov (1) n0.0 0x0 {Switch}
                    G4_DstRegRegion* n0Dst = builder.createDst(
                        builder.phyregpool.getN0Reg(), 0, 0, 1, Type_UD);
                    auto movInst = builder.createMov(1, n0Dst,
                        builder.createImm(0, Type_UD), InstOpt_WriteEnable | InstOpt_Switch, false);
                    bb->insert(ii, movInst);
                }

                linePlaneWA(inst);
                fixSendSrcRegion(inst);
                ii++;
            }
        }

        if (VISA_WA_CHECK(builder.getPWaTable(), WaClearArfDependenciesBeforeEot))
        {
            clearARFDependencies();
        }
        if (VISA_WA_CHECK(builder.getPWaTable(), Wa_2201674230))
        {
            clearSendDependencies();
        }

        if (builder.needResetA0forVxHA0())
        {
            // reset a0 to 0 at the beginning of a shader.
            resetA0();
        }

        if (builder.getOption(vISA_setA0toTdrForSendc))
        {
            // set A0 to tdr0 before sendc/sendsc. TGL WA
            setA0toTdrForSendc();
        }

        if (builder.needReplaceIndirectCallWithJmpi() &&
            kernel.getIntKernelAttribute(Attributes::ATTR_Extern) != 0)
        {
            // replace ret in the external functions with jmpi. That we will
            // also return the call with jmpi in VISAKernelImpl::compilePostOptimize
            replaceRetWithJmpi();
        }

        if (kernel.hasIndirectCall())
        {
            // If the indirect call has regiser src0, the register must be a
            // ip-based address of the call target. Insert a add before call to
            // calculate the relative offset from call to the target
            expandIndirectCallWithRegTarget();
        }
    }

class NSDS {
    int GRF_BUCKET;
    int TOTAL_BUCKETS;

    struct InstrDescr;
    // Used to get the InstrDescr for a given instruction.
    // We use instr->local_id as the index into IDvec[]
    std::vector<InstrDescr *> IDvec;
    int idCnt;
    const int IDCNT_UNINIT = 0;
    const int IDCNT_INIT = 1;

    class Bucket;
    // The main data structure for the buckets
    std::vector<Bucket> bucketVec;

public:
    struct BucketDescr;
    typedef std::vector<BucketDescr> BDVec_t;
    typedef std::set<Bucket *> BSet_t;

private:
    enum RW {
        RW_UNINIT = 0,
        READ = 1 << 0,
        WRITE = 1 << 1,
        RW_MAX
    };

    // The IDs which are covered by other instructions
    std::set<InstrDescr *> coveredIDs;

    // This is the instruction descriptor
    struct InstrDescr {
        G4_INST *instr;
        // The buckets Read by this instruction and whether it is covered or not
        std::map<Bucket *, bool> bucketStatusR;
        // The buckets Written by this instr and whether it is covered or not
        std::map<Bucket *, bool> bucketStatusW;
        int coveredBucketsR;
        int coveredBucketsW;
        std::set<InstrDescr *> &coveredIDs;

        // Attach bucket to instruction descriptor (into bucketStatus{R,W})
        void addBucket(Bucket *bucket, RW rwType) {
            if (rwType & READ) {
                if (! bucketStatusR.count(bucket)) {
                    bucketStatusR.insert(std::pair<Bucket *, bool>(bucket, false));
                }
            }
            else if (rwType & WRITE) {
                if (! bucketStatusW.count(bucket)) {
                    bucketStatusW.insert(std::pair<Bucket *, bool>(bucket, false));
                }
            }
        }

        // Cover instructions reading BUCKET
        void coverR(Bucket *bucket) {
            // READ: If BUCKET has not been marked as covered, mark it now
            auto itR = bucketStatusR.find(bucket);
            if (itR != bucketStatusR.end() && itR->second == false) {
                coveredBucketsR++;
                itR->second = true;
            }

            // If there is at least one send instruction that reads at least
            // one of the buckets redy by this ID, collect it.
            if (coveredBucketsR) {
                coveredIDs.insert(this);
            }
        }

        // Cover instructions writing to BUCKET
        void coverW(Bucket *bucket) {
            // WRITE: If BUCKET has not been marked as covered, mark it now
            auto itW = bucketStatusW.find(bucket);
            if (itW != bucketStatusW.end() && itW->second == false) {
                coveredBucketsW++;
                itW->second = true;
            }

            // If there is at least one instruction that reads at least
            // one of the buckets written to by this ID, collect it.
            if (coveredBucketsW) {
                coveredIDs.insert(this);
            }
        }

        // Cover BUCKET for this instruction
        void coverRW(Bucket *bucket) {
            // READ: If BUCKET has not been marked as covered, mark it now
            auto itR = bucketStatusR.find(bucket);
            if (itR != bucketStatusR.end() && itR->second == false) {
                coveredBucketsR++;
                itR->second = true;
            }

            // WRITE: If BUCKET has not been marked as covered, mark it now
            auto itW = bucketStatusW.find(bucket);
            if (itW != bucketStatusW.end() && itW->second == false) {
                coveredBucketsW++;
                itW->second = true;
            }

            // If ID is fully covered from the read or write side, collect it.
            if ((coveredBucketsR && coveredBucketsR == bucketStatusR.size())
                || (coveredBucketsW && coveredBucketsW == bucketStatusW.size())) {
                coveredIDs.insert(this);
            }
        }

        InstrDescr(G4_INST *Instr, NSDS *nsds)
            : instr(Instr), coveredBucketsR(0), coveredBucketsW(0),
              coveredIDs(nsds->coveredIDs) {
            ;
        }

        void dump(void) {
            std::cerr << "  InstrDescr:\n";
            std::cerr << " ";
            instr->dump();

            std::cerr << "    BucketStatusR: ";
            for (auto pair : bucketStatusR) {
                Bucket *bucket = pair.first;
                bool covered = pair.second;
                std::cerr << bucket->getIndex()
                          << "[" << ((covered) ? "T" : "F") << "]"
                          << ", ";
            }
            std::cerr << "\n";

            std::cerr << "    BucketStatusW: ";
            for (auto pair : bucketStatusW) {
                Bucket *bucket = pair.first;
                bool covered = pair.second;
                std::cerr << bucket->getIndex()
                          << "[" << ((covered) ? "T" : "F") << "]"
                          << ", ";
            }
            std::cerr << "\n";

            std::cerr << "    coveredBucketsR: " << coveredBucketsR << "\n";
            std::cerr << "    coveredBucketsW: " << coveredBucketsW << "\n";
            std::cerr << "    ----\n";
        }
    };

    // Each bucket points to all instructions that read the register that
    // corresponds to this bucket.
    class Bucket {
        int index;                 // For debugging
        std::set<InstrDescr *> IDSet;
    public:
        Bucket(void) {
            index = 0;
        }
        void setIndex(int Index) { index = Index; }
        int getIndex(void) { return index; }
        std::set<InstrDescr *> &getIDSet(void) { return IDSet; }

        void addID(InstrDescr *ID) {
            IDSet.insert(ID);
        }
        void removeID(InstrDescr *ID) {
            IDSet.erase(ID);
        }

        // Cover instructions that read this bucket
        void coverReadIDs(SFID sfid = SFID::NULL_SFID) {
            // Go through all the instructions that read this instruction.
            // Notify the instruction that one of its registers is covered
            for (InstrDescr *ID : IDSet) {
                assert(ID->instr->isSend());
                // Only sends with the same SFID can be covered
                if (sfid == SFID::NULL_SFID
                    || ID->instr->getMsgDesc()->getFuncId() == sfid)
                {
                    ID->coverR(this);
                }
            }
        }

        // Cover instructions that write this bucket
        void coverWrittenIDs(SFID sfid = SFID::NULL_SFID) {
            // Go through all the instructions that read this instruction.
            // Notify the instruction that one of its registers is covered
            for (InstrDescr *ID : IDSet) {
                assert(ID->instr->isSend());
                // Only sends with the same SFID can be covered
                if (sfid == SFID::NULL_SFID
                    || ID->instr->getMsgDesc()->getFuncId() == sfid) {
                    ID->coverW(this);
                }
            }
        }

        // This bucket is reported as being written to.
        // Therefore kill the instructions that read this BD.
        // Each instruction may be reading from other buckets too.
        // Remove it from all those buckets.
        void kill(void) {
            // Temporarily cache the IDs to be removed
            std::set<InstrDescr *> removeIDs = IDSet;
            for (InstrDescr *ID : removeIDs) {
                // Remove ID from all the buckets it is reading
                for (auto &bucketStatus
                         : {ID->bucketStatusR, ID->bucketStatusW}) {
                    for (auto pair : bucketStatus) {
                        Bucket *B = pair.first;
                        B->removeID(ID);
                    }
                }
            }
        }

        // Return TRUE if no IDs are connected to this bucket
        int empty(void) {
            return IDSet.empty();
        }

        void dump(void) {
            std::cerr << "Bucket: " << index << "\n";
            for (InstrDescr *ID : IDSet) {
                ID->dump();
            }
        }
    };

    // Parse INSTR's OPND and generate the Bucket Descriptors.
    // Insert them into BDVec.
    void getBucketsForOperand(G4_INST* inst, RW rwType, G4_Operand* opnd,
                              BDVec_t& BDVec) {
        if (opnd->isLabel() || opnd->isImm()) {
            return;
        }
#define UNINIT_BUCKET -1
        int startingBucket = UNINIT_BUCKET;
        G4_VarBase* base = opnd->getBase();
        bool canSpanMultipleBuckets = false;

        assert(base && "If no base, then the operand is not touched by instr.");

        // If a register allocated regvar, then get the physical register
        G4_VarBase *phyReg = (base->isRegVar()) ?
            base->asRegVar()->getPhyReg() : base;

        switch (phyReg->getKind()) {
        case G4_VarBase::VK_phyGReg:
            // Handle GRFs - fast path
            startingBucket = opnd->getLinearizedStart()/G4_GRF_REG_NBYTES;
            canSpanMultipleBuckets = true;
            break;
        case G4_VarBase::VK_phyAReg:
            break;
        default:
            assert(0 && "Bad kind");
            break;
        }

        // Create one or more buckets and push them into the vector
        if (startingBucket != UNINIT_BUCKET) {
            if (canSpanMultipleBuckets) {
                unsigned int divisor;
                int baseBucket;
                assert(base->isGreg());
                divisor = G4_GRF_REG_NBYTES;
                baseBucket = GRF_BUCKET;
                int endingBucket = baseBucket
                    + opnd->getLinearizedEnd() / divisor;
                MUST_BE_TRUE(endingBucket >= startingBucket,
                             "Ending bucket less than starting bucket");
                int numBuckets = endingBucket - startingBucket + 1;
                for (int j = startingBucket;
                     j < (startingBucket + numBuckets); j++ ) {
                    BDVec.push_back(BucketDescr(j, rwType));
                }
            }
            else {
                BDVec.push_back(BucketDescr(startingBucket, rwType));
            }
        }
    }


    // Add INST insto the bucketVec
void genBucket(G4_INST *send, Bucket *bucket, RW rwType) {
        assert(send->isSend());
        // 1. Create Instruction Descriptor for SEND, or reuse existing one
        InstrDescr *ID;
        if (! send->getLocalId()) {
            ID = new InstrDescr(send, this);
            send->setLocalId(idCnt);
            IDvec[idCnt] = ID;
            idCnt++;
        } else {
            ID = IDvec[send->getLocalId()];
        }
        // 2. Add the bucket read by INST to the Instr Descriptor
        ID->addBucket(bucket, rwType);
        // 3. Insert the ID into the bucket
        bucket->addID(ID);
    }

    void IDvecInit(size_t bbSize) {
        assert(IDvec.size() == 0);
        IDvec.resize(bbSize + IDCNT_INIT);
        idCnt = IDCNT_INIT;
    }

    void IDvecFree(void) {
        for (InstrDescr *ID : IDvec) {
            if (ID) {
                delete ID;
            }
        }
        IDvec.clear();
        idCnt = IDCNT_INIT;
    }


public:

    // Debug print
    void dump(void) {
        for (size_t i = 0, e = bucketVec.size(); i != e; ++i) {
            Bucket *bucket = &bucketVec[i];
            if (bucket->empty()) {
                continue;
            }
            bucket->dump();
        }
    }

    struct BucketDescr {
        int bucketIdx;
        RW type;
        BucketDescr(int BucketIdx, RW rwType)
            : bucketIdx(BucketIdx), type(rwType) { ; }
    };

    // We pack all information we have collected for the instruction
    struct BucketDescrBox {
        BucketDescrBox() : hasIndirW(false) {
            BDVec.reserve(16);
        }
        // The vector of buckets read/writen by the instruction
        BDVec_t BDVec;
        // The instruction has an indirect write
        bool hasIndirW;
    };

    // Given an inst with physical register assignment,
    // return all bucket descriptors that the physical register can map
    // to. This requires taking in to account exec size, data
    // type, and whether inst is a send
    BucketDescrBox getBucketDescrs(G4_INST* inst, BucketDescrBox &instrBuckets) {
        // Iterate over all WRITTEN operands and create buckets.
        for (Gen4_Operand_Number opndNum : {Opnd_dst}) {
            G4_Operand *opnd = inst->getOperand(opndNum);
            if (! opnd || ! opnd->getBase()) {
                continue;
            }
            getBucketsForOperand(inst, WRITE, opnd, instrBuckets.BDVec);
            instrBuckets.hasIndirW |= opnd->asDstRegRegion()->isIndirect();
        }
        for (Gen4_Operand_Number opndNum
                 : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_src3}) {
            G4_Operand *opnd = inst->getOperand(opndNum);
            if (! opnd || ! opnd->getBase()) {
                continue;
            }
            getBucketsForOperand(inst, READ, opnd, instrBuckets.BDVec);
        }
        return instrBuckets;
    }

    // Kill the write buckets of the bucket descriptors
    void killWriteBuckets(BucketDescrBox &instrBuckets) {
        // An indirect write kills all active buckets.
        if (instrBuckets.hasIndirW) {
            // FIXME: make this faster by keeping track of active buckets
            for (Bucket &bucket : bucketVec) {
                bucket.kill();
            }
        } else {
            for (const BucketDescr &BD : instrBuckets.BDVec) {
                if (BD.type & WRITE) {
                    Bucket *bucket = &bucketVec[BD.bucketIdx];
                    bucket->kill();
                }
            }
        }
    }

    // // Kill the buckets that instr reasd from
    // void killReadBuckets(BucketDescrBox &instrBuckets) {
    //     for (const BucketDescr &BD : instrBuckets.BDVec) {
    //         if (BD.type & READ) {
    //             Bucket *bucket = &bucketVec[BD.bucketIdx];
    //             bucket->kill();
    //         }
    //     }
    // }

    // Kill the buckets of this INSTR only
    void killInstrBuckets(G4_INST *instr) {
        int IDidx = instr->getLocalId();
        assert(IDidx);
        InstrDescr *ID = IDvec[IDidx];
        // Go through the buckets related to this INSTR and
        // remove this ID from them all.
        for (auto &bucketStatus : {ID->bucketStatusR, ID->bucketStatusW}) {
            for (const auto &pair : bucketStatus) {
                Bucket *bucket = pair.first;
                bucket->removeID(ID);
            }
        }
        // We no longer carry an InstrDescr
        instr->setLocalId(IDCNT_UNINIT);
    }

    // Create new buckets for the bucket descriptors of the instruction
    void genReadBuckets(G4_INST *instr, const BucketDescrBox &instrBuckets) {
        for (const auto &BD : instrBuckets.BDVec) {
            if (BD.type & READ) {
                Bucket *bucket = &bucketVec[BD.bucketIdx];
                genBucket(instr, bucket, READ);
            }
        }
    }

    // Create new bucket for any registers written by the instruction
    void genWriteBuckets(G4_INST *instr, const BucketDescrBox &instrBuckets) {
        for (const auto &BD : instrBuckets.BDVec) {
            if (BD.type & WRITE) {
                Bucket *bucket = &bucketVec[BD.bucketIdx];
                genBucket(instr, bucket, WRITE);
            }
        }
    }

    // Cover the instructions that read the values read by instrBuckets
    void coverInstrReadingBucketsReadBy(const BucketDescrBox &instrBuckets,
                                        SFID sfid)
    {
        for (const auto &BD : instrBuckets.BDVec) {
            if (BD.type == READ) {
                bucketVec[BD.bucketIdx].coverReadIDs(sfid);
            }
        }
    }

    // Cover the instructions that read the values read by instrBuckets
    void coverInstrWritingBucketsReadBy(const BucketDescrBox &instrBuckets) {
        for (const auto &BD : instrBuckets.BDVec) {
            if (BD.type == READ) {
                bucketVec[BD.bucketIdx].coverWrittenIDs();
            }
        }
    }

    // Return a vector of all fully covered instructions and clear the
    // set where these are held.
    std::vector<G4_INST *> getCoveredInstrsAndClear(void) {
        std::vector<G4_INST *> instrs;
        for (InstrDescr *ID : coveredIDs) {
            instrs.push_back(ID->instr);
        }
        coveredIDs.clear();
        return instrs;
    }

    NSDS(const Options *options, G4_BB *bb) {
        int totalGRFNum = bb->getKernel().getNumRegTotal();
        int TOTAL_BUCKETS = 0;
        GRF_BUCKET = TOTAL_BUCKETS;
        TOTAL_BUCKETS += totalGRFNum;

        bucketVec.resize(TOTAL_BUCKETS);
        // If not in debugging mode don't bother initializing ids
#if (defined(_DEBUG) || defined(_INTERNAL))
        for (size_t i = 0, e = bucketVec.size(); i != e; ++i) {
            bucketVec[i].setIndex((int)i);
        }
#endif
        IDvecInit(bb->size());
    };

    ~NSDS(void) {
        IDvecFree();
    }

    void verify(G4_BB *bb, G4_INST *fromInstr, G4_INST *toInstr) {
        NSDS::BucketDescrBox FromInstrBuckets;
        (void) getBucketDescrs(fromInstr, FromInstrBuckets);

        bool foundFrom = false;
        for (auto instr : *bb) {
            if (instr != fromInstr && !foundFrom) {
                continue;
            }
            foundFrom = true;
            if (instr == fromInstr) {
                continue;
            }
            if (foundFrom && instr == toInstr) {
                break;
            }


            NSDS::BucketDescrBox InstrInstrBuckets;
            (void) getBucketDescrs(instr, InstrInstrBuckets);
            assert(!InstrInstrBuckets.hasIndirW);
            for (const BucketDescr &BD : InstrInstrBuckets.BDVec) {
                if (BD.type & WRITE) {
                    // Look for this BD in FromInstrBuckets
                    for (const BucketDescr &FromBD : FromInstrBuckets.BDVec) {
                        if (FromBD.bucketIdx == BD.bucketIdx) {
                            assert(0);
                        }
                    }
                }
            }


        }
    }
};

    // Scan the code and insert the NoSrcDepSet flag if possible to remove some
    // scoreboard delays. The NoSrcDepSet flag allows the instruction that
    // follows to ignore the scoreboard dependence check for the registers read
    // and issue right away.
    //
    // Scenario 1: A send without the NoSrcDepSet can cover all sends with a
    //             RAR dependence against the registers they read.
    //    ... = send R1 SFID0 {NoSrcDepSet} (covered by the send below)
    //    ... = send R1 SFID2      -        (different SFID)
    //    ... = send R1 SFID0 {NoSrcDepSet} (covered by the send below)
    //    ... = send R1 SFID0
    //     ...
    //    ... = R1
    //     R1 = ...
    //
    // Scenario 2: A RAW dependence between an instruction and the destination
    //             register of a send allows it to be marked {NoSrcDepSet}
    //    send R1, Rx  SFID0 {NoSrcDepSet}
    //    R2 = send R1 SFID0 {NoSrcDepSet} (covered by RAW dependence)
    //    ... = R2
    //    R1 = ...
    //
    // Scenario 3: A covered send can also cover other sends of the same SFID
    //             if the sources of these sends are only accessed after the
    //             instruction that covered the first send
    //    send R2 SFID0 {NoSrcDepSet}
    //    send R1 SIFD0
    //    ...
    //    ... = R1
    //    R2 = ...

    // We can only set the flag if the registers read by the send are all read
    // by following sends before any write to that register.
    //
    // We visit the instructions top-down in a single pass.
    // For each instruction we collect the register buckets read and written.
    //
    // If the instruction is a send, we link it with the corresponding buckets
    // that are read by it.
    // Each instruction is wrapped in an IstrDescr object which keeps track of
    // the bucket status: that is whether they are covered or not by succeeding
    // instructions that read those registers.
    /*
          InstrDescr
          +--------+ +--------+
          | sendA  | | sendB  |
          |r0,r1,r2| |r2,r3,r4|  bucketStatusR,W
          | T, F, T| | F, F, F| (covered flag T/F)
          +--------+ +--------+
            /  |  \  /   |   \
       +----+----+----+----+----+....+----+----+----+...
       | r0 | r1 | r2 | r3 | r4 |    |r128| p0 | p1 |
       +----+----+----+----+----+....+----+----+----+...
       Bucket                                           bucketVec
    */
    // For each register read we check the bucket tha that corresponds to it.
    // We iterate over all the sends connected to it and mark that bucket
    // as covered. If the send instruction has all its buckets covered,
    // then it is legal to mark it with the NoSrcDepSet flag.
    // The marked sends are removed from the bucket array.
    //
    // Upon an instruction that writes to a register, we access the
    // corresponding bucket in bucketVec, we go through all the instructions
    // that access it and we remove those instructions from the data structure.
    // For example upon an instruction r2=... we will remove both sendA and
    // sendB from the data structure.
    //

    // The entry point for the code that adds the NoSrcDepSet flag to
    // specific send isntructions for better performance.
    // For a description of the algorithm check the comments of the NSDS class.
    void Optimizer::NoSrcDepSet(void)
    {
        #define NSDS_RAW        // Enable NoSrcDepSet for RAW
        // Early return if invalid platform
        if (! builder.noSrcDepSetAllowedPlatform()
            || ! builder.getOption(vISA_EnableNoSrcDep)) {
            return;
        }
        #if (defined(_DEBUG) || defined(_INTERNAL))
        int dbgCnt = 0;
        #endif
        for (auto bb : fg) {
            NSDS nsds(builder.getOptions(), bb);
            for (auto instr : *bb) {
                if (instr->isLabel()) {
                    continue;
                }
                instr->setLocalId(0); // We are storing the IDs in it

                NSDS::BucketDescrBox instrBuckets;
                (void) nsds.getBucketDescrs(instr, instrBuckets);

                // Cover any registers that the current instruction reads
                // To be safe, predicated instrs can kill but cannot cover.
                if (! instr->getPredicate()) {
                    // This is for Scenario 1.
                    // A send without the nosrcdepset flag can cover other sends
                    // that read the same register
                    if (instr->isSend()) {
                        if (builder.getOption(vISA_EnableNoSrcDepScen1)) {
                            auto SFID = instr->getMsgDesc()->getFuncId();
                            nsds.coverInstrReadingBucketsReadBy(instrBuckets, SFID);
                        }
                    }
                    // This is for Scenario 2.
                    // A non-send instruction reading from Rx
                    // can cover sends that write to Rx
                    else {
                        if (builder.getOption(vISA_EnableNoSrcDepScen2)) {
                            nsds.coverInstrWritingBucketsReadBy(instrBuckets);
                        }
                    }

                    // Check if all registers read by the instruction cover
                    // a send in NSDS. If so, this instruction gets marked.
                    for (G4_INST *coveredSend : nsds.getCoveredInstrsAndClear()) {
                        assert(coveredSend->isSend());
                        #if (defined(_DEBUG) || defined(_INTERNAL))
                        // Disable marking the instruction
                        int stopAt = builder.getOptions()->getuInt32Option(vISA_stopNoSrcDepSetAt);
                        if (stopAt == UINT_MAX || dbgCnt++ < abs(stopAt))
                        #endif
                            // MARK the instruction
                            coveredSend->setOptionOn(InstOpt_NoSrcDepSet);
                        // Kill the buckets associated to this instruction
                        nsds.killInstrBuckets(coveredSend);

                        // Verifier for NSDS
                        // #define NSDS_VERIFY
                        #ifdef NSDS_VERIFY
                        nsds.verify(bb, coveredSend, instr);
                        #endif
                    }
                }

                // Kill any bucket this instruction writes to.
                nsds.killWriteBuckets(instrBuckets);

                // Populate the buckets read by this send
                if (instr->isSend()) {
                    // RAR
                    nsds.genReadBuckets(instr, instrBuckets);
                    #ifdef NSDS_RAW
                    // RAW
                    nsds.genWriteBuckets(instr, instrBuckets);
                    #endif
                }
                #if (defined(_DEBUG) || defined(_INTERNAL))
                if (builder.getOptions()->getOption(vISA_DumpNoSrcDep)) {
                    nsds.dump();
                }
                #endif
            }
        }
    }

    //
    // rewrite source regions to satisfy various HW requirements.  This pass will not modify the instrppuctions otherwise
    // -- rewrite <1;1,0> to <2;2,1> when possible (exec size > 1, width is not used to cross GRF)
    //    as HW doesn't allow <1;1,0> to be co-issued
    //
    void Optimizer::normalizeRegion()
    {
        for (auto bb : fg)
        {
            for (auto inst : *bb)
            {
                if (inst->isCall() ||
                    inst->isReturn())
                {
                    // Do not rewrite region for call or return,
                    // as the effective execution size is 2.
                    continue;
                }

                if (inst->getExecSize() == 1)
                {
                    // Replace: mov (1) r64.0<4>:df  r3.0<0;1,0>:df
                    // with:    mov (1) r64.0<1>:df  r3.0<0;1,0>:df
                    // otherwise, will get incorrect results for HSW, HW mode
                    G4_Operand* dst = inst->getDst();
                    if (dst != NULL &&
                        dst->asDstRegRegion()->getHorzStride() > 1 &&
                        G4_Type_Table[dst->getType()].byteSize == 8)
                    {
                        dst->asDstRegRegion()->setHorzStride(1);
                    }
                }
                else
                {
                    for (int i = 0; i < inst->getNumSrc(); ++i)
                    {
                        G4_Operand* src = inst->getSrc(i);
                        // Only rewrite direct regions.
                        if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == Direct)
                        {
                            G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
                            if (srcRegion->getRegion()->isContiguous(inst->getExecSize()))
                            {
                                srcRegion->rewriteContiguousRegion(builder, i);
                            }
                            else if (inst->isAlign1Ternary())
                            {
                                // special checks for 3src inst with single non-unit stride region
                                // rewrite it as <s*2;s>
                                uint16_t stride = 0;
                                if (srcRegion->getRegion()->isSingleNonUnitStride(inst->getExecSize(), stride))
                                {
                                    MUST_BE_TRUE(stride <= 4, "illegal stride for align1 ternary region");
                                    srcRegion->setRegion(kernel.fg.builder->createRegionDesc(stride * 2, 2, stride));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void Optimizer::countGRFUsage()
    {
        unsigned int maxGRFNum = kernel.getNumRegTotal();
        int count = 0;
        bool *GRFUse = (bool *) builder.mem.alloc(sizeof(bool) * maxGRFNum);
        for (unsigned int i = 0; i < maxGRFNum; ++i)
        {
            GRFUse[i] = false;
        }
        for (auto dcl : kernel.Declares)
        {
            if (dcl->getRegVar()->isGreg())
            {
                int GRFStart = dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum();
                int numRows = dcl->getNumRows();
                MUST_BE_TRUE(GRFStart >= 0 && (GRFStart + numRows) <= (int)maxGRFNum,
                    "illegal GRF assignment");
                for (int i = GRFStart; i < GRFStart + numRows; ++i)
                {
                    GRFUse[i] = true;
                }
            }
        }

        for (unsigned int i = 0; i < maxGRFNum; ++i)
        {
            if (GRFUse[i])
            {
                count++;
            }
        }
        fg.builder->getJitInfo()->numGRFUsed = count;
        fg.builder->criticalMsgStream() << "\tKernel " << kernel.getName() << " : " <<
            count << " registers\n";
    }

    //
    //  Dump the input payload to start of scratch space.
    //  this is strictly for debugging and we do not care if this gets overwritten by
    //  other usage of the scratch space (private memory, spill, etc.)
    //
    void Optimizer::dumpPayload()
    {
        int inputEnd = 0;
        for (int i = 0, numInput = kernel.fg.builder->getInputCount(); i < numInput; ++i)
        {
            input_info_t* input_info = kernel.fg.builder->getInputArg(i);
            if (inputEnd < input_info->size + input_info->offset)
            {
                inputEnd = input_info->size + input_info->offset;
            }
        }

        G4_BB* bb = kernel.fg.getEntryBB();
        // iter points to the first non-label inst
        auto iter = bb->begin(), bbEnd = bb->end();
        while (iter != bbEnd)
        {
            if (!(*iter)->isLabel())
            {
                break;
            }
            ++iter;
        }

        int regOffset = (inputEnd + GENX_GRF_REG_SIZ - 1) / GENX_GRF_REG_SIZ;

        static const unsigned SCRATCH_MSG_DESC_CATEGORY = 18;
        static const unsigned SCRATCH_MSG_DESC_OPERATION_MODE = 17;
        static const unsigned SCRATCH_MSG_DESC_CHANNEL_MODE = 16;
        static const unsigned SCRATCH_MSG_DESC_BLOCK_SIZE = 12;

        // write 8 GRFs at a time
        int msgSize = 8;
        for (int i = 1; i < regOffset; i += msgSize)
        {
            uint16_t extFuncCtrl = 0;
            // both scratch and block read use DC
            SFID funcID = SFID::DP_DC;

            uint32_t headerPresent = 0x80000;
            uint32_t msgDescImm = headerPresent;
            uint32_t msgLength = 1;
            uint32_t blocksizeEncoding = 0x3; // 8 GRF
            msgDescImm |= (msgLength << getSendMsgLengthBitOffset());
            msgDescImm |= (1 << SCRATCH_MSG_DESC_CATEGORY);
            msgDescImm |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
            msgDescImm |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);

            msgDescImm |= (blocksizeEncoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
            msgDescImm |= i;

            G4_SendMsgDescriptor* desc = kernel.fg.builder->createSendMsgDesc(
                msgDescImm, 0, 1, funcID, msgSize, extFuncCtrl, SendAccess::WRITE_ONLY);
            const RegionDesc* region = kernel.fg.builder->getRegionStride1();
            G4_SrcRegRegion* headerOpnd = kernel.fg.builder->Create_Src_Opnd_From_Dcl(kernel.fg.builder->getBuiltinR0(), region);
            G4_Declare* tempDcl = builder.createHardwiredDeclare(msgSize * 8, Type_UD, i, 0);
            G4_SrcRegRegion* srcOpnd = kernel.fg.builder->Create_Src_Opnd_From_Dcl(tempDcl, region);
            G4_DstRegRegion* dstOpnd = kernel.fg.builder->createNullDst(Type_UD);

            G4_INST* sendInst = kernel.fg.builder->createSplitSendInst(
                nullptr, G4_sends, 16, dstOpnd, headerOpnd, srcOpnd,
                kernel.fg.builder->createImm(msgDescImm, Type_UD), InstOpt_WriteEnable, desc, nullptr, 0);
            bb->insert(iter, sendInst);
        }
    }

    // perform simple stat collection (e.g., numBarriers, numSends)
    // IR is not modified
    void Optimizer::collectStats()
    {
        builder.getJitInfo()->usesBarrier = false;
        uint32_t numSends = 0;
        for (auto bb : fg)
        {
            for (auto inst : *bb)
            {
                if (inst->isSend())
                {
                    numSends++;
                    if (inst->asSendInst()->getMsgDesc()->isBarrierMsg())
                    {
                        // ToDo: remove this at some point as only legacy CMRT needs this information
                        builder.getJitInfo()->usesBarrier = true;
                    }

                }
            }
        }
        builder.getcompilerStats().SetI64(CompilerStats::numSendStr(), numSends, builder.kernel.getSimdSize());
    }

    // Create a copy of R0 at top of kernel,
    // to support midthread preemption.
    // This must be called before all other optimizer
    // passes, except PI_initializePayload.
    void Optimizer::createR0Copy()
    {
        if (!builder.getIsKernel())
        {
            return;
        }
        bool needSwitch = (fg.getIsStackCallFunc() && VISA_WA_CHECK(builder.getPWaTable(), WaThreadSwitchAfterCall));

        // Skip copying of ``copy of R0'' if it's never assigned, a case where
        // ``copy of R0'' is never used. As EOT always use ``copy of R0'', that
        // case only happens for synthetic tests where no practical code is
        // generated.
        if (!builder.getBuiltinR0()->getRegVar()->isPhyRegAssigned() && !needSwitch)
            return;

        G4_Declare *R0Dcl = builder.getRealR0();
        G4_SrcRegRegion* R0Opnd = builder.createSrcRegRegion(
            Mod_src_undef,
            Direct,
            R0Dcl->getRegVar(),
            0,
            0,
            builder.getRegionStride1(),
            Type_UD);

        G4_DstRegRegion* R0CopyOpnd = builder.createDst(
            builder.getBuiltinR0()->getRegVar(),
            0,
            0,
            1,
            Type_UD);
        R0CopyOpnd->computePReg();

        unsigned int options = InstOpt_WriteEnable;
        if (needSwitch)
            options |= InstOpt_Switch;
        G4_INST *movInst =
            builder.createMov(8, R0CopyOpnd, R0Opnd, options, false);

        BB_LIST_ITER ib = kernel.fg.begin();
        G4_INST *inst = NULL;
        BB_LIST_ITER bend(kernel.fg.end());
        INST_LIST_ITER ii;

        for (ib = kernel.fg.begin(); ib != bend; ++ib)
        {
            G4_BB* bb = (*ib);
            ii = bb->begin();
            INST_LIST_ITER iend = bb->end();
            for (; ii != iend; ii++)
            {
                inst = *ii;
                if (inst->opcode() != G4_label)
                {
                    bb->insert(ii, movInst);
                    return;
                }
            }
        }
    }

    void Optimizer::initializePayload()
    {
        if (!kernel.fg.builder->getIsKernel())
        {
            return;
        }

        unsigned int inputEnd = 32;
        unsigned int inputCount = kernel.fg.builder->getInputCount();
        for (unsigned int id = 0; id < inputCount; id++)
        {
            input_info_t* input_info = kernel.fg.builder->getInputArg(id);
            if (inputEnd < (unsigned)(input_info->size + input_info->offset))
            {
                inputEnd = input_info->size + input_info->offset;
            }
        }

        int reg_offset = (inputEnd + 31) / 32;
        //bytes whithin grf that need to be initialized
        int remainder_bytes = 32 - (inputEnd % 32);
        //number of full grfs that need to be initialized
        int num_reg_init = 128 - reg_offset;
        //beginning execution size for byte remainder initialization
        uint16_t exec_size = 16;
        //offset within GRF from which to start to initialize
        int sub_offset = (inputEnd % 32);
        G4_BB* bb = kernel.fg.getEntryBB();

        // iter points to the first non-label inst
        auto iter = bb->begin(), bbEnd = bb->end();
        while (iter != bbEnd)
        {
            if (!(*iter)->isLabel())
            {
                break;
            }
            ++iter;
        }

        //inits builk of GRFS, two at a time
        for (int i = 0; i < num_reg_init - (num_reg_init % 2); i += 2)
        {
            int reg_init = reg_offset + i;
            G4_Declare* tempDcl = builder.createHardwiredDeclare(16, Type_UD, reg_init, 0);
            G4_DstRegRegion* dst = builder.createDst(tempDcl->getRegVar(), 0, 0, 1, Type_UD);
            G4_Imm * src0 = builder.createImm(0, Type_UD);
            G4_INST* initInst = builder.createMov(16, dst, src0, InstOpt_WriteEnable, false);
            bb->insert(iter, initInst);
        }

        //init last register if bulk of GRFs was odd
        if ((num_reg_init % 2))
        {
            G4_Declare* tempDcl = builder.createHardwiredDeclare(8, Type_UD, 128 - (num_reg_init % 2), 0);
            G4_DstRegRegion* dst = builder.createDst(tempDcl->getRegVar(), 0, 0, 1, Type_UD);
            G4_Imm * src0 = builder.createImm(0, Type_UD);
            G4_INST* spIncInst = builder.createMov(8, dst, src0, InstOpt_WriteEnable, false);
            G4_BB* bb = kernel.fg.getEntryBB();
            bb->insert(iter, spIncInst);
        }

        //inits remainder GRF
        //loops untill all bytes within GRF are initialized
        //on each iteration goes down by execution size
        //There was a small bug if inputEnd offset is GRF aligned it would think all of
        //last payload register is the "remainder" and will initialize it.
        while (remainder_bytes && (inputEnd % 32))
        {
            for (int i = 0; i < remainder_bytes - (remainder_bytes % exec_size); i += exec_size, sub_offset += exec_size)
            {
                G4_Declare* tempDcl = builder.createHardwiredDeclare(exec_size, Type_UB, reg_offset - 1, sub_offset);
                G4_DstRegRegion* dst = builder.createDst(tempDcl->getRegVar(), 0, 0, 1, Type_UB);

                G4_Declare* tempDclSrc = builder.createHardwiredDeclare(1, Type_UD, 126, 0);
                G4_SrcRegRegion *src0 = builder.createSrcRegRegion(Mod_src_undef, Direct, tempDclSrc->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UB);

                G4_INST* initInst = builder.createMov((uint8_t)exec_size, dst, src0, InstOpt_WriteEnable, false);
                bb->insert(iter, initInst);
            }
            //caluclates bytes that remain to be initialized
            remainder_bytes = remainder_bytes % exec_size;
            //next lowest execution size
            exec_size = (uint16_t)max(1, exec_size / 2);
        }

        //Initializaing Flag register
        int num32BitFlags = builder.getNumFlagRegisters() / 2;
        for (int i = 0; i < num32BitFlags; ++i)
        {
            G4_Declare* tmpFlagDcl = builder.createTempFlag(2);
            tmpFlagDcl->getRegVar()->setPhyReg(builder.phyregpool.getFlagAreg(i), 0);
            G4_DstRegRegion *tempPredVar = builder.createDst(tmpFlagDcl->getRegVar(), 0, 0, 1, Type_UD);
            G4_INST *predInst = builder.createMov(1, tempPredVar, builder.createImm(0, Type_UW), InstOpt_WriteEnable, false);
            bb = kernel.fg.getEntryBB();
            bb->insert(iter, predInst);
        }
    }

    // create prolog to set sr0 to FFID. TGL WA.
    // Do only when there is cr0 write inside the kernel
    void Optimizer::addFFIDProlog()
    {
        if (!builder.getIsKernel())
            return;

        FFID ffid = static_cast<FFID>(builder.getOptions()->getuInt32Option(vISA_setFFID));
        // return if FFID is not given
        if (ffid == FFID_INVALID)
            return;

        // get r127.0 decl
        G4_Declare* rtail =
            builder.createHardwiredDeclare(8, Type_UD, kernel.getNumRegTotal() - 1, 0);

        // (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud  0xF0FFFFFF:ud
        auto createAnd = [this, &rtail]()
        {
            auto src0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, builder.phyregpool.getSr0Reg(), 0, 0,
                builder.getRegionScalar(), Type_UD);
            auto src1 = builder.createImm(0xF0FFFFFF, Type_UD);
            auto dst = builder.createDst(rtail->getRegVar(), 0, 0, 1, Type_UD);

            return builder.createBinOp(G4_and, 1,
                dst, src0, src1, InstOpt_WriteEnable, false);
        };

        // (W) or  (1|M0)  sr0.0<1>:ud   127.0<0;1,0>:ud    imm:ud
        auto createOr = [this, &rtail](uint32_t imm)
        {
            auto src0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, rtail->getRegVar(), 0, 0,
                builder.getRegionScalar(), Type_UD);
            auto src1 = builder.createImm(imm, Type_UD);
            auto dst = builder.createDst(
                builder.phyregpool.getSr0Reg(), 0, 0, 1, Type_UD);

            return builder.createBinOp(G4_or, 1,
                dst, src0, src1, InstOpt_WriteEnable, false);
        };

        // (W) jmpi (1|M0) label
        auto createJmpi = [this](G4_Label* label)
        {
            return builder.createInternalInst(nullptr, G4_jmpi, nullptr, false, 1,
                nullptr, label, nullptr, InstOpt_WriteEnable);
        };

        auto createLabelInst = [this](G4_Label* label)
        {
            return kernel.fg.createNewLabelInst(label);
        };

        // for compute shader, create two entris
        if (ffid == FFID_GP || ffid == FFID_GP1)
        {
            // Entry0: Set sr0 to FFID_GP (0x7)
            //     (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud   0xF0FFFFFF:ud
            //     (W) or  (1|M0)  sr0.0<1>:ud     127.0<0;1,0>:ud    0x07000000:ud
            //     jmpi ffid_prolog_end
            // Entry1: Set sr0 to FFID_GP1 (0x8)
            //     (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud   0xF0FFFFFF:ud
            //     (W) or  (1|M0)  sr0.0<1>:ud     127.0<0;1,0>:ud    0x08000000:ud
            //     ffid_prolog_end:

            // Put the entry0 block into a new BB, so that we can make it 64-bit
            // aligned in BinaryEncodingIGA
            G4_BB* entry_0_bb = kernel.fg.createNewBB();
            entry_0_bb->push_back(createAnd());
            entry_0_bb->push_back(createOr(0x07000000));

            // get jmp target label. If the next bb has no label, create one and insert it
            // at the beginning
            G4_Label* jmp_label = nullptr;
            assert(kernel.fg.begin() != kernel.fg.end());
            G4_BB* next_bb = *kernel.fg.begin();
            if (next_bb->front()->isLabel())
            {
                jmp_label = next_bb->front()->getSrc(0)->asLabel();
            }
            else
            {
                std::string label_name("ffid_prolog_end");
                jmp_label = builder.createLabel(label_name, LABEL_BLOCK);
                next_bb->insert(next_bb->begin(), createLabelInst(jmp_label));
            }
            entry_0_bb->push_back(createJmpi(jmp_label));

            // Put the rest in another BB
            G4_BB* entry_1_bb = kernel.fg.createNewBB();
            entry_1_bb->push_back(createAnd());
            entry_1_bb->push_back(createOr(0x08000000));

            // add these two BB to be the first two in the shader
            kernel.fg.addPrologBB(entry_1_bb);
            kernel.fg.addPrologBB(entry_0_bb);
            builder.setHasComputeFFIDProlog();
        }
        else
        {
            // for other shaders, set the FFID
            //     (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud   0xF0FFFFFF:ud
            //     (W) or  (1|M0)  sr0.0<1>:ud     127.0<0;1,0>:ud    (FFID << 24):ud
            G4_BB* bb = kernel.fg.createNewBB();
            bb->push_back(createAnd());
            bb->push_back(createOr(ffid << 24));
            kernel.fg.addPrologBB(bb);
        }
    }

    void Optimizer::loadThreadPayload()
    {
    }

    // some platform/shaders require a memory fence before the end of thread
    // ToDo: add fence only when the writes can reach EOT without a fence in between
    void Optimizer::insertFenceBeforeEOT()
    {
        if (!builder.getOption(vISA_clearHDCWritesBeforeEOT))
        {
            return;
        }

        if (!kernel.fg.builder->getIsKernel())
        {
            // we dont allow a function to exit
            return;
        }

        bool hasUAVWrites = false;
        bool hasSLMWrites = false;
        bool hasTypedWrites = false;


        for (auto bb : kernel.fg)
        {
            if (bb->isEndWithFCall())
            {
                // conservatively assume we need a fence
                // ToDo: we don't need a SLM fence if kernel doesnt use SLM, since function can't allocate SLM on its own
                // We can move this W/A to IGC for more precise analysis
                hasUAVWrites = true;
                hasSLMWrites = true;
                hasTypedWrites = true;
                break;
            }

            for (auto inst : *bb)
            {
                if (inst->isSend())
                {
                    auto msgDesc = inst->asSendInst()->getMsgDesc();
                    if (msgDesc->isDataPortWrite())
                    {
                        if (msgDesc->isHDC())
                        {
                            if (msgDesc->isSLMMessage())
                            {
                                hasSLMWrites = true;
                            }
                            else if (msgDesc->isHdcTypedSurfaceWrite())
                            {
                                hasTypedWrites = true;
                            }
                            else
                            {
                                hasUAVWrites = true;
                            }
                        }

                    }
                }
            }
        }

        if (!hasUAVWrites && !hasSLMWrites && !hasTypedWrites)
        {
            return;
        }

        for (auto bb : kernel.fg)
        {
            if (bb->isLastInstEOT())
            {
                auto iter = std::prev(bb->end());

                {
                    if (hasUAVWrites)
                    {
                        auto fenceInst = builder.createFenceInstruction(0, true, true, false);
                        bb->insert(iter, fenceInst);
                    }
                    if (hasSLMWrites)
                    {
                        auto fenceInst = builder.createFenceInstruction(0, true, false, false);
                        bb->insert(iter, fenceInst);
                    }
                }
                builder.instList.clear();
            }
        }
    }

    // some platform/shaders require a memory fence at kernel entry
    // this needs to be called before RA since fence may have a (dummy) destination.
    void Optimizer::insertFenceAtEntry()
    {
    }


    void Optimizer::mapOrphans()
    {
        auto catchAllCISAOff = builder.debugInfoPlaceholder;
        if (catchAllCISAOff == UNMAPPABLE_VISA_INDEX)
            return;

        for (auto bb : kernel.fg)
        {
            for (auto inst : bb->getInstList())
            {
                if (inst->getCISAOff() == UNMAPPABLE_VISA_INDEX)
                {
                    inst->setCISAOff(catchAllCISAOff);
                }
            }
        }
    }

    void Optimizer::varSplit()
    {
        VarSplitPass* splitPass = kernel.getVarSplitPass();
        // Run explicit variable split pass
        if (kernel.getOption(vISA_IntrinsicSplit))
        {
            splitPass->run();
        }
    }

    // some platforms require extra instruction before an EOT to
    // ensure that all outstanding scratch writes are globally observed
    void Optimizer::insertScratchReadBeforeEOT()
    {
        int globalScratchOffset = kernel.getIntKernelAttribute(Attributes::ATTR_SpillMemOffset);
        if (builder.needFenceBeforeEOT() ||
            (globalScratchOffset == 0 && builder.getJitInfo()->spillMemUsed == 0))
        {
            return;
        }

        struct ScratchReadDesc
        {
            uint32_t addrOffset : 12;
            uint32_t dataElements : 2;
            uint32_t reserved : 3;
            uint32_t opType : 2;
            uint32_t header : 1;
            uint32_t resLen : 5;
            uint32_t msgLen : 4;
            uint32_t reserved2: 3;
        };

        union {
            uint32_t value;
            ScratchReadDesc layout;
        } desc;

        // msg desc for 1GRF scratch block read
        desc.value = 0;
        desc.layout.opType = 2;
        desc.layout.header = 1;
        desc.layout.resLen = 1;
        desc.layout.msgLen = 1;

        for (auto bb : kernel.fg)
        {
            if (bb->isLastInstEOT())
            {
                auto iter = std::prev(bb->end());
                if (getPlatformGeneration(builder.getPlatform()) >= PlatformGen::GEN10)
                {
                    // an HDC fence is more efficient in this case
                    // fence with commit enable
                    int fenceDesc = G4_SendMsgDescriptor::createDesc((0x7 << 14) | (1 << 13), true, 1, 1);
                    auto msgDesc = builder.createSyncMsgDesc(SFID::DP_DC, fenceDesc);
                    auto src = builder.Create_Src_Opnd_From_Dcl(builder.getBuiltinR0(), builder.getRegionStride1());
                    auto dst = builder.Create_Dst_Opnd_From_Dcl(builder.getBuiltinR0(), 1);
                    G4_INST* inst = builder.createSendInst(nullptr, G4_send, 8, dst, src,
                        builder.createImm(fenceDesc, Type_UD), InstOpt_WriteEnable, msgDesc);
                    bb->insert(iter, inst);
                }
                else
                {
                    // insert a dumy scratch read
                    auto msgDesc = builder.createReadMsgDesc(SFID::DP_DC, desc.value);
                    auto src = builder.Create_Src_Opnd_From_Dcl(builder.getBuiltinR0(), builder.getRegionStride1());
                    // We can use any dst that does not conflcit with EOT src, which must be between r112-r127
                    auto dstDcl = builder.createHardwiredDeclare(8, Type_UD, 1, 0);
                    auto dst = builder.Create_Dst_Opnd_From_Dcl(dstDcl, 1);
                    G4_INST* sendInst = builder.createSendInst(nullptr, G4_send, 8, dst, src,
                        builder.createImm(desc.value, Type_UD), InstOpt_WriteEnable, msgDesc);
                    bb->insert(iter, sendInst);
                }

                builder.instList.clear();
            }
        }
    }

    // Reset A0 to 0 at the beginning of the shader if the shader use VxH a0
    void Optimizer::resetA0()
    {
        // check all instructions to see if VxH a0 src is used
        // only reset A0 when it's used
        bool hasA0 = false;
        for (auto bb : kernel.fg)
        {
            for (auto inst : *bb)
            {
                // VxH must be in src0
                if (inst->getSrc(0) != nullptr &&
                    inst->getSrc(0)->isSrcRegRegion() &&
                    inst->getSrc(0)->asSrcRegRegion()->getBase()->isA0() &&
                    inst->getSrc(0)->asSrcRegRegion()->getRegion()->isRegionWH())
                {
                    hasA0 = true;
                    break;
                }
            }
            if (hasA0)
                break;
        }

        if (!hasA0)
            return;

        // insert "mov (16) a0.0:uw 0x0:uw" at the beginning of the shader
        if (kernel.fg.begin() != kernel.fg.end()) {
            G4_BB* bb = *kernel.fg.begin();
            auto insertIt = std::find_if(bb->begin(), bb->end(),
                [](G4_INST* inst) { return !inst->isLabel(); });
            bb->insert(insertIt,
                builder.createMov(16, builder.createDst(
                    builder.phyregpool.getAddrReg(), 0, 0, 1, Type_UW),
                    builder.createImm(0, Type_UW), InstOpt_WriteEnable, false));
        }
    }

    G4_Declare* Optimizer::createInstsForCallTargetOffset(
        InstListType& insts, G4_INST* fcall, int64_t adjust_off)
    {
        // create instruction sequence:
        //       add  r2.0  -IP   call_target
        //       add  r2.0  r2.0  adjust_off

        // call's dst must be r1.0, which is reserved at
        // GlobalRA::setABIForStackCallFunctionCalls. It must not be overlapped with
        // r2.0, that is hardcoded as the new jump target
        assert(fcall->getDst()->isGreg());
        assert((fcall->getDst()->getLinearizedStart() / GENX_GRF_REG_SIZ) != 2);

        // hardcoded add's dst to r2.0
        G4_Declare* add_dst_decl =
            builder.createHardwiredDeclare(1, fcall->getDst()->getType(), 2, 0);

        // create the first add instruction
        // add  r2.0  -IP   call_target
        G4_INST* add_inst = builder.createBinOp(
            G4_add, 1,
            builder.Create_Dst_Opnd_From_Dcl(add_dst_decl, 1),
            builder.createSrcRegRegion(
                Mod_Minus, Direct, builder.phyregpool.getIpReg(), 0, 0,
                builder.getRegionScalar(), Type_UD),
            fcall->getSrc(0), InstOpt_WriteEnable | InstOpt_NoCompact, false);

        // create the second add to add the -ip to adjust_off, adjust_off dependes
        // on how many instructions from the fist add to the jmp instruction, and
        // if it's post-increment (jmpi) or pre-increment (call)
        // add  r2.0  r2.0  adjust_off
        G4_INST* add_inst2 = builder.createBinOp(
            G4_add, 1,
            builder.Create_Dst_Opnd_From_Dcl(add_dst_decl, 1),
            builder.Create_Src_Opnd_From_Dcl(
                add_dst_decl, builder.getRegionScalar()),
            builder.createImm(adjust_off, Type_D),
            InstOpt_WriteEnable | InstOpt_NoCompact, false);

        insts.push_back(add_inst);
        insts.push_back(add_inst2);

        return add_dst_decl;
    }

    void Optimizer::createInstForJmpiSequence(InstListType& insts, G4_INST* fcall)
    {
        // SKL workaround for indirect call
        // r1.0 is the return IP (the instruction right after jmpi)
        // r1.1 is the return mask. While we'll replace the ret in calee to jmpi as well,
        // we do not need to consider the return mask here.

        // Do not allow predicate call on jmpi WA
        assert(fcall->getPredicate() == nullptr);

        // calculate the reserved register's num from fcall's dst register (shoud be r1)
        assert(fcall->getDst()->isGreg());
        uint32_t reg_num = fcall->getDst()->getLinearizedStart() / GENX_GRF_REG_SIZ;

        G4_Declare* new_target_decl = createInstsForCallTargetOffset(insts, fcall, -64);

        // add  r1.0   IP   32
        G4_Declare* r1_0_decl =
            builder.createHardwiredDeclare(1, fcall->getDst()->getType(), reg_num, 0);
        insts.push_back(builder.createBinOp(
            G4_add, 1,
            builder.Create_Dst_Opnd_From_Dcl(r1_0_decl, 1),
            builder.createSrcRegRegion(
                Mod_src_undef, Direct, builder.phyregpool.getIpReg(), 0, 0,
                builder.getRegionScalar(), Type_UD),
            builder.createImm(32, Type_UD),
            InstOpt_WriteEnable | InstOpt_NoCompact, false));

        // jmpi r2.0
        // update jump target (src0) to add's dst
        G4_SrcRegRegion* jump_target = builder.Create_Src_Opnd_From_Dcl(
            new_target_decl, builder.getRegionScalar());
        jump_target->setType(Type_D);
        insts.push_back(builder.createJmp(nullptr, jump_target, InstOpt_NoCompact, false));
    }

    void Optimizer::expandIndirectCallWithRegTarget()
    {
        // check every fcall
        for (auto bb : kernel.fg)
        {
            // At this point G4_pseudo_fcall may be converted to G4_call,
            // check all call (???)
            if (bb->back()->isFCall()) //|| bb->back()->isCall())
            {
                G4_INST* fcall = bb->back();
                if (fcall->getSrc(0)->isGreg() || fcall->getSrc(0)->isA0()) {
                    // at this point the call instruction's src0 has the target_address
                    // and the call dst is the reserved register for ret
                    // All the caller save register should be saved. We usd r2.0 directly
                    // here to calculate the new call's target. We picked r2.0 due to the
                    // HW's limitation that call/calla's src and dst offset (the subreg num)
                    // must be 0.
                    //
                    // expand call
                    // From:
                    //       call r1.0 call_target
                    // To:
                    //       add  r2.0  -IP   call_target
                    //       add  r2.0  r2.0  -32
                    //       call r1.0  r2.0

                    // For SKL workaround, expand call
                    // From:
                    //       call r1.0 call_target
                    // To:
                    //       add  r2.0   -IP    call_target
                    //       add  r2.0   r2.0   -64
                    //       add  r1.0   IP   32              // set the return IP
                    //       jmpi r2.0

                    InstListType expanded_insts;
                    if (builder.needReplaceIndirectCallWithJmpi()) {
                        createInstForJmpiSequence(expanded_insts, fcall);
                    }
                    else {
                        G4_Declare* jmp_target_decl =
                            createInstsForCallTargetOffset(expanded_insts, fcall, -32);
                        // Updated call's target to the new target
                        G4_SrcRegRegion* jump_target = builder.Create_Src_Opnd_From_Dcl(
                            jmp_target_decl, builder.getRegionScalar());
                        fcall->setSrc(jump_target, 0);
                        fcall->setNoCompacted();
                    }
                    // then insert the expaneded instructions right before the call
                    INST_LIST_ITER insert_point = bb->end();
                    --insert_point;
                    for (auto inst_to_add : expanded_insts) {
                        bb->getInstList().insert(insert_point, inst_to_add);
                        inst_to_add->setCISAOff(fcall->getCISAOff());
                    }

                    // remove call from the instlist for Jmpi WA
                    if (builder.needReplaceIndirectCallWithJmpi())
                        bb->getInstList().erase(--bb->end());
                }
            }
        }

    }

    // Replace ret with jmpi, must be single return
    void Optimizer::replaceRetWithJmpi()
    {
        size_t num_ret = 0;

        for (G4_BB* bb : kernel.fg)
        {
            if (bb->empty())
                continue;
            if (bb->isEndWithFRet())
            {
                ++num_ret;
                G4_INST* ret_inst = bb->back();

                // ret dst's decl
                G4_Declare* r_1_0 = ret_inst->getSrc(0)->getTopDcl();

                // calculate the jmpi target offset
                // expand the original ret from:
                //     ret r1.0
                // To:
                //     add   r1.0  -ip   r1.0
                //     add   r1.0  r1.0  -48
                //     jmpi  r1.0

                // add   r1.0  -ip   r1.0
                G4_INST* add0 = builder.createBinOp(
                    G4_add, 1,
                    builder.Create_Dst_Opnd_From_Dcl(r_1_0, 1),
                    builder.createSrcRegRegion(
                        Mod_Minus, Direct, builder.phyregpool.getIpReg(), 0, 0,
                        builder.getRegionScalar(), Type_UD),
                    builder.Create_Src_Opnd_From_Dcl(r_1_0, builder.getRegionScalar()),
                    InstOpt_WriteEnable | InstOpt_NoCompact, false);

                // add   r1.0  r1.0  -48
                G4_INST* add1 = builder.createBinOp(
                    G4_add, 1,
                    builder.Create_Dst_Opnd_From_Dcl(r_1_0, 1),
                    builder.Create_Src_Opnd_From_Dcl(r_1_0, builder.getRegionScalar()),
                    builder.createImm(-48, Type_D), InstOpt_WriteEnable | InstOpt_NoCompact, false);

                // jmpi r1.0
                G4_SrcRegRegion* jmpi_target = builder.Create_Src_Opnd_From_Dcl(
                    r_1_0, builder.getRegionScalar());
                jmpi_target->setType(Type_D);
                G4_INST* jmpi = builder.createJmp(nullptr, jmpi_target, InstOpt_NoCompact, false);

                // remove the ret
                bb->pop_back();
                // add the ret
                bb->push_back(add0);
                bb->push_back(add1);
                bb->push_back(jmpi);
            }
        }

        // there should be exactly one ret in a external function. We did not try
        // to restore the CallMask. We rely on single return of a function to make
        // sure the CallMask before and after calling this function is the same.
        assert(num_ret == 1);
    }

    // Set a0 to tdr0 before snedc/sendsc
    void Optimizer::setA0toTdrForSendc()
    {
        // check for the last inst of each BB, if it's sendc/sendsc, insert
        // "(W) mov(8) a0.0:uw tdr0.0:uw" right before it
        for (G4_BB* bb : kernel.fg)
        {
            if (bb->empty())
                continue;
            if (bb->back()->opcode() == G4_opcode::G4_sendc ||
                bb->back()->opcode() == G4_opcode::G4_sendsc)
            {
                // "(W) mov(8) a0.0:uw tdr0.0:uw"
                bb->insert(--bb->end(), builder.createMov(8,
                    builder.createDst(builder.phyregpool.getAddrReg(), 0, 0, 1, Type_UW),
                    builder.createSrcRegRegion(Mod_src_undef, Direct, builder.phyregpool.getTDRReg(), 0,
                                               0, builder.getRegionScalar(), Type_UW),
                    InstOpt_WriteEnable, false));
            }
        }
    }

    /*
    * Check if there is WAR/WAW dependency between end inst and the preceding instruction
    */
    bool Optimizer::chkFwdOutputHazard(INST_LIST_ITER& startIter, INST_LIST_ITER& endIter)
    {
        G4_INST *startInst = *startIter;

        INST_LIST_ITER forwardIter = startIter;
        forwardIter++;
        while( forwardIter != endIter )
        {
            if( (*forwardIter)->isWAWdep(startInst) ||
                (*forwardIter)->isWARdep(startInst) )
            {
                break;
            }
            forwardIter++;
        }

        if( forwardIter != endIter )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // check if startInst has any WAR/WAW conflicts with subsequent insts up till endIter (excluding endIter)
    // precondition: startInst must be before endIter in the same BB
    // this is used to sink an inst (or its sources) to the endIter location
    bool Optimizer::chkFwdOutputHazard(G4_INST* startInst, INST_LIST_ITER endIter)
    {
        INST_LIST_ITER backIter = std::prev(endIter, 1);
        while (*backIter != startInst)
        {
            G4_INST* inst = *backIter;
            if (inst->isWARdep(startInst) || inst->isWAWdep(startInst))
            {
                return true;
            }
            --backIter;
        }
        return false;
    }

    bool Optimizer::chkBwdOutputHazard(INST_LIST_ITER& startIter, INST_LIST_ITER& endIter)
    {
        G4_INST *endInst = *endIter;

        INST_LIST_ITER backwardIter = endIter;
        backwardIter--;
        while( backwardIter != startIter )
        {
            if( endInst->isWAWdep(*backwardIter) ||
                endInst->isWARdep(*backwardIter) )
            {
                break;
            }
            backwardIter--;
        }

        if( backwardIter != startIter )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Optimizer::chkBwdOutputHazard(G4_INST* startInst, INST_LIST_ITER& endIter)
    {
        G4_INST *endInst = *endIter;

        INST_LIST_ITER backwardIter = endIter;
        backwardIter--;
        while (*backwardIter != startInst)
        {
            if( endInst->isWAWdep(*backwardIter) ||
                /*
                    Makes sure there is not WAR conflict between this instruction and instruction preceding it:
                    ... grf1(use preceding inst)
                    grf1 <---- def this inst
                */
                endInst->isWARdep(*backwardIter) )
            {
                break;
            }
            backwardIter--;
        }

        if (*backwardIter != startInst)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /*
    Skips WAW check for the skipInst
    */
    bool Optimizer::chkBwdOutputHazard(G4_INST* startInst, INST_LIST_ITER& endIter, G4_INST *skipInst)
    {
        G4_INST *endInst = *endIter;

        INST_LIST_ITER backwardIter = endIter;
        backwardIter--;
        while (*backwardIter != startInst)
        {
            if (skipInst == *backwardIter)
            {
                --backwardIter;
                continue;
            }

            /*
            Makes sure there is not WAR conflict between this instruction and instruction preceding it:
            ... grf1(use preceding inst)
            grf1 <---- def this inst
            */
            if (endInst->isWARdep(*backwardIter) ||
                endInst->isWAWdep(*backwardIter))
            {
                break;
            }
            --backwardIter;
        }

        if (*backwardIter != startInst)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Optimizer::chkBwdWARdep(G4_INST* startInst, INST_LIST_ITER endIter)
    {
        while (*endIter != startInst)
        {
            G4_INST* inst = *endIter;
            if (inst->isWARdep(startInst))
            {
                return true;
            }
            --endIter;
        }
        return false;
    }

    // This function performs the following renaming to enable further optimization opportunities:
    //
    // op   v3, v1, v2
    // mov  v4, v3
    // mov  v5, v3
    // mov  v6, v3
    // ======>
    // op   v3, v1, v2
    // mov  v4, v3
    // mov  v5, v4
    // mov  v6, v4
    void Optimizer::renameRegister()
    {
        const int MAX_REG_RENAME_DIST = 250;
        const int MAX_REG_RENAME_SIZE = 2;

        BB_LIST_ITER ib, bend(fg.end());

        for(ib = fg.begin(); ib != bend; ++ib)
        {
            G4_BB* bb = (*ib);

            bb->resetLocalId();
            std::unordered_set<G4_INST *> Seen;

            INST_LIST_ITER ii = bb->begin(), iend(bb->end());
            while ( ii != iend )
            {
                G4_INST *inst = *ii;

                if( !inst->isRawMov() ||
                    inst->getPredicate() ||
                    Seen.count(inst) > 0 ||
                    inst->def_size() != 1 ||
                    !inst->canHoist(!bb->isAllLaneActive(), fg.builder->getOptions()) )
                {
                    ii++;
                    continue;
                }

                G4_Operand *src = inst->getSrc(0);
                G4_DstRegRegion *dst = inst->getDst();

                G4_Declare *srcDcl = src->isRegRegion() ? GetTopDclFromRegRegion(src) : nullptr;
                G4_Declare *dstDcl = GetTopDclFromRegRegion(dst);

                if (!srcDcl || !dstDcl)
                {
                    ++ii;
                    continue;
                }

                // If this move is between two different register files, then
                // do not do register renaming.
                if (srcDcl && dstDcl && srcDcl->getRegFile() != dstDcl->getRegFile())
                {
                    ++ii;
                    continue;
                }

                G4_INST *defInst = inst->def_front().first;
                G4_Declare *defDstDcl = GetTopDclFromRegRegion(defInst->getDst());
                if( ( dstDcl && dstDcl->getAddressed() ) ||
                    ( defDstDcl && defDstDcl->getAddressed() ) )
                {
                    ii++;
                    continue;
                }

                G4_DstRegRegion* defDstRegion = defInst->getDst();
                if (Seen.count(defInst) > 0 ||
                    src->compareOperand( defDstRegion ) != Rel_eq )
                {
                    ii++;
                    continue;
                }

                unsigned int instMaskOption = inst->getMaskOption();
                bool canRename = true;

                if (defInst->use_size() == 1)
                {
                    ii++;
                    continue;
                }

                int32_t sizeRatio = dstDcl->getByteSize() / srcDcl->getByteSize();

                G4_INST* lastUse = defInst->use_front().first;
                for (auto iter = defInst->use_begin(), E = defInst->use_end(); iter != E; ++iter)
                {
                    G4_INST* useInst = (*iter).first;

                    if( useInst == inst ||
                        ( (useInst->getLocalId() - inst->getLocalId()) > MAX_REG_RENAME_DIST &&
                           sizeRatio > MAX_REG_RENAME_SIZE ) )
                    {
                        continue;
                    }

                    /*
                        it incorrectly renames in this case, because it doesn't consider
                        defInst mask.
                        BEFORE:
                        <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (1) V44(0,0)[1]:d V46(0,0)[0;1,0]:d [Align1, NoMask] %11</FONT></TD></TR>
                        <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (8) V48(0,0)[1]:d V44(0,0)[0;1,0]:d [Align1, Q1] %12</FONT></TD></TR>
                        <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (8) V56(0,0)[1]:d V44(0,0)[0;1,0]:d [Align1, Q1] %22</FONT></TD></TR>

                        AFTER:
                        <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (1) V44(0,0)[1]:d V46(0,0)[0;1,0]:d [Align1, NoMask] %11</FONT></TD></TR>
                        <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (8) V48(0,0)[1]:d V44(0,0)[0;1,0]:d [Align1, Q1] %12</FONT></TD></TR>
                        <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (8) V56(0,0)[1]:d V48(0,0)[0;1,0]:d [Align1, Q1] %22</FONT></TD></TR>

                        Fix: BB in SIMD control flow && (inst not NoMask) !(instMask == defFask == useMask)

                        Disallow replication?
                    */
                    if( useInst->getLocalId() < inst->getLocalId() ||
                        !useInst->isRawMov() ||
                        inst->getExecSize() != useInst->getExecSize() ||
                        (useInst->getSrc(0))->compareOperand( defDstRegion ) != Rel_eq ||
                        useInst->def_size() > 1 ||
                        (!(inst->isWriteEnableInst()) &&
                        useInst->getMaskOption() != instMaskOption) ||
                        //fix described above
                        (!bb->isAllLaneActive() &&
                        !inst->isWriteEnableInst() &&
                        !(inst->getExecSize() == defInst->getExecSize() &&
                                inst->getExecSize() == useInst->getExecSize())
                        )
                      )
                    {
                        canRename = false;
                        break;
                    }

                    if( useInst->getLocalId() > lastUse->getLocalId() )
                    {
                        lastUse = useInst;
                    }
                }

                for (auto iter = defInst->use_begin(), E = defInst->use_end(); iter != E; ++iter)
                {
                    G4_INST* useInst = (*iter).first;
                    Seen.insert(useInst);
                }

                if( !canRename )
                {
                    ii++;
                    continue;
                }

                INST_LIST_ITER forwardIter = ii;
                forwardIter++;
                while( canRename &&
                       *forwardIter != lastUse &&
                       ( ((*forwardIter)->getLocalId() - inst->getLocalId()) <= MAX_REG_RENAME_DIST ||
                          sizeRatio <= MAX_REG_RENAME_SIZE ) )
                {
                    if( (*forwardIter)->isWAWdep( inst ) )
                    {
                        canRename = false;
                        break;
                    }
                    forwardIter++;
                }

                if( !canRename )
                {
                    ii++;
                    continue;
                }

                for (auto useIter = defInst->use_begin(); useIter != defInst->use_end(); /*empty*/)
                {
                    G4_INST* useInst = (*useIter).first;

                    if( useInst == inst ||
                        ( (useInst->getLocalId() - inst->getLocalId()) > MAX_REG_RENAME_DIST &&
                          sizeRatio > MAX_REG_RENAME_SIZE ) )
                    {
                        useIter++;
                        continue;
                    }

                    G4_Operand *useSrc = useInst->getSrc(0);
                    unsigned char execSize = useInst->getExecSize();
                    unsigned short dstHS = dst->getHorzStride();
                    const RegionDesc *newSrcRd;

                    if( useSrc->asSrcRegRegion()->isScalar() )
                    {
                        newSrcRd = builder.getRegionScalar();
                    }
                    else
                    {
                        unsigned tExecSize = (execSize > 8) ? 8 : execSize;
                        if (RegionDesc::isLegal(tExecSize * dstHS, execSize, dstHS))
                        {
                            newSrcRd = builder.createRegionDesc((uint16_t)tExecSize*dstHS, execSize, dstHS);
                        }
                        else
                        {
                            // Skip this use. TODO: normalize this region.
                            ++useIter;
                            continue;
                        }
                    }

                    G4_SrcRegRegion *newSrcOpnd = builder.createSrcRegRegion(
                        Mod_src_undef,
                        dst->getRegAccess(),
                        dst->getBase(),
                        dst->getRegOff(),
                        dst->getSubRegOff(),
                        newSrcRd,
                        useSrc->getType() );
                    if (dst->getRegAccess() != Direct)
                    {
                        newSrcOpnd->asSrcRegRegion()->setImmAddrOff( dst->getAddrImm() );
                    }

                    useInst->getSrc(0)->~G4_Operand();
                    useInst->setSrc(newSrcOpnd, 0);

                    // Maintain def-use for this change:
                    // - remove this use from defInst
                    // - add a new use to inst
                    useIter = defInst->eraseUse(useIter);
                    inst->addDefUse(useInst, Opnd_src0);
                }

                ii++;
            }
        }
    }

    // use by mergeScalar
#define OPND_PATTERN_ENUM(DO) \
    DO(UNKNOWN) \
    DO(IDENTICAL) \
    DO(CONTIGUOUS) \
    DO(DISJOINT)

enum OPND_PATTERN
{
    OPND_PATTERN_ENUM(MAKE_ENUM)
};

static const char* patternNames[] =
{
    OPND_PATTERN_ENUM(STRINGIFY)
};

#define MAX_BUNDLE_SIZE 16
#define MAX_NUM_SRC 3

struct BUNDLE_INFO
{
    int size;
    int sizeLimit;
    G4_BB* bb;
    INST_LIST_ITER startIter;
    G4_INST* inst[MAX_BUNDLE_SIZE];
    OPND_PATTERN dstPattern;
    OPND_PATTERN srcPattern[MAX_NUM_SRC];

    BUNDLE_INFO(G4_BB* instBB, INST_LIST_ITER& instPos, int limit) : sizeLimit(limit), bb(instBB)
    {

        inst[0] = *instPos;
        startIter = instPos;
        dstPattern = OPND_PATTERN::UNKNOWN;
        for (int i = 0; i < MAX_NUM_SRC; i++)
        {
            srcPattern[i] = OPND_PATTERN::UNKNOWN;
        }
        size = 1;
    }

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    void appendInst(G4_INST* lastInst)
    {
        MUST_BE_TRUE(size < MAX_BUNDLE_SIZE, "max bundle size exceeded");
        inst[size++] = lastInst;
    }

    void deleteLastInst()
    {
        assert(size > 0 && "empty bundle");
        inst[--size] = nullptr;
    }

    bool canMergeDst(G4_DstRegRegion* dst);
    bool canMergeSource(G4_Operand* src, int srcPos);
    bool canMerge(G4_INST* inst);

    bool doMerge(IR_Builder& builder,
        std::unordered_set<G4_Declare*>& modifiedDcl,
        std::vector<G4_Declare*>& newInputs);

    void print(std::ostream& output) const
    {
        output << "Bundle:\n";
        output << "Dst pattern:\t" << patternNames[dstPattern] << "\n";
        output << "Src Pattern:\t";
        for (int i = 0; i < inst[0]->getNumSrc(); ++i)
        {
            output << patternNames[srcPattern[i]] << " ";
        }
        output << "\n";
        for (int i = 0; i < size; ++i)
        {
            inst[i]->emit(output);
            output << "\n";
        }
    }

};

// This will create a new dcl or reuse the existing one if available.
// The returned dcl will have
//    its Reg's byte address == input's
//    its type == srcType
//    itss numElem == bundleSize.
static G4_Declare* getInputDeclare(IR_Builder& builder,
                                   std::vector<G4_Declare*>& declares,
                                   G4_Declare* input,
                                   G4_Type eltType,
                                   int bundleSize,
                                   int firstEltOffset)
{
    MUST_BE_TRUE(input->isInput() && input->getAliasDeclare() == NULL, "Expect root input variable");
    for (auto dcl : declares)
    {
        MUST_BE_TRUE(dcl->isInput() && dcl->getAliasDeclare() == NULL, "declare must be root input variable");
        if (dcl->getRegVar()->getByteAddr() == input->getRegVar()->getByteAddr() &&
            dcl->getElemType() == eltType &&
            dcl->getTotalElems() >= bundleSize)
        {
            return dcl;
        }
    }

    // Now, create a new dcl.
    // As the new dcl's offset is in unit of element type, we will convert offset in bytes to
    // offset in elementType.  As the previous check guarantees that the byte offset must be
    // multiple of elementType, here just add additional checks to make sure this is the case.
    uint32_t offset = input->getRegVar()->getPhyRegOff() * G4_Type_Table[input->getElemType()].byteSize + firstEltOffset;
    uint32_t eltBytes = G4_Type_Table[eltType].byteSize;
    MUST_BE_TRUE((offset % eltBytes) == 0, "Offset shoule be mutiple of element size");
    offset = offset / eltBytes;
    const char *name = builder.getNameString(builder.mem, 16, "InputR%d.%d", input->getRegVar()->getPhyReg()->asGreg()->getRegNum(), offset);
    G4_Declare* newInputDcl = builder.createDeclareNoLookup(name, G4_INPUT, (uint16_t) bundleSize, 1,
        eltType);
    newInputDcl->getRegVar()->setPhyReg(input->getRegVar()->getPhyReg(), offset);
    declares.push_back(newInputDcl);
    return newInputDcl;
}

//#define DEBUG_VERBOSE_ON
//
// merge all instructions in the bundle into a single one, by modifying the first instruction
// in the bundle and delete the rest
// returns true if merge is successful
// merge can fail if a source/dst appears in multiple bundles and has a conflicing alignment
// (this only happens when the operand is in DISJOINT pattern)
//
bool BUNDLE_INFO::doMerge(IR_Builder& builder,
                          std::unordered_set<G4_Declare*>& modifiedDcl,
                          std::vector<G4_Declare*>& newInputs)
{
    if (size == 1)
    {
        return false;
    }

    if (size == 3)
    {
        for (int pos = 0, numSrc = inst[0]->getNumSrc(); pos < numSrc; ++pos)
        {
            if (srcPattern[pos] == OPND_PATTERN::CONTIGUOUS)
            {
                // since we are rounding up esize to 4, we have to make sure the source is
                // not out of bound. If it is we merge the first two inst instead
                G4_SrcRegRegion* lastSrc = inst[size - 1]->getSrc(pos)->asSrcRegRegion();
                if (lastSrc->getLeftBound() + G4_Type_Table[lastSrc->getType()].byteSize >=
                    lastSrc->getTopDcl()->getByteSize())
                {
                    deleteLastInst();
                    break;
                }
            }
        }
    }

    if (!builder.hasAlign1Ternary() && size == 2 && inst[0]->getNumSrc() == 3)
    {
        return false;
    }

    // for distinct patterns (both src and dst), we need to check the variable is only used once in the entire bundle,
    // so that the new variable created for the operand does not conflict with another.
    // this is to prevent coalescing two variables with conflicting life range, e.g.,
    //  add (M1_NM, 1) V66(0,0)<1> V66(0,0)<0;1,0> 0x1:d                             /// $36
    //  add (M1_NM, 1) V67(0,0)<1> V65(0,0)<0;1,0> 0x1:d                             /// $37
    //  or make a variable with two root aliases:
    //  mul (M1_NM, 1) V186(0,0)<1> V184(0,0)<0;1,0> V185(0,0)<0; 1, 0>                /// $206
    //  mul (M1_NM, 1) V187(0,0)<1> V182(0,0)<0;1,0> V182(0,0)<0; 1, 0>                /// $207
    std::set<G4_Declare*> uniqueDeclares;

    // check if merging is legal
    if (dstPattern == OPND_PATTERN::DISJOINT)
    {
        G4_Declare* rootDcl = inst[0]->getDst()->getTopDcl()->getAliasDeclare();
        for (int instId = 0; instId < size; ++instId)
        {
            G4_DstRegRegion* dst = inst[instId]->getDst();
            G4_Declare* dcl = dst->getTopDcl();
            if (dcl->getAliasDeclare() != rootDcl)
            {
                // all dsts in the bundle should have the same base variable (or NULL)
                // this is to avoid the situation where {V1, V2} is in one bundle and {V1, V3}
                // is in another bundle, and we can't optimize both since V2 and V3 may be both live
                return false;
            }
            if (dcl->getAliasDeclare() != NULL)
            {
                if (dcl->getAliasOffset() !=
                    G4_Type_Table[dst->getType()].byteSize * instId)
                {
                    return false;
                }
            }
            uniqueDeclares.insert(dcl);
        }
    }
    for (int i = 0; i < inst[0]->getNumSrc(); ++i)
    {
        if (srcPattern[i] == OPND_PATTERN::DISJOINT)
        {
            G4_Declare* rootDcl = inst[0]->getSrc(i)->getTopDcl()->getAliasDeclare();
            for (int instId = 0; instId < size; ++instId)
            {
                G4_Operand* src = inst[instId]->getSrc(i);
                G4_Declare* dcl = src->getTopDcl();
                if (dcl->getAliasDeclare() != rootDcl)
                {
                    // all srcs in the bundle, if aliased, should have the same alias variable
                    // see comments from above for dst
                    return false;
                }
                if (dcl->getAliasDeclare() != NULL)
                {
                    if (dcl->getAliasOffset() !=
                        G4_Type_Table[src->getType()].byteSize * instId)
                    {
                        return false;
                    }
                }
                if (uniqueDeclares.find(dcl) != uniqueDeclares.end())
                {
                    return false;
                }
                uniqueDeclares.insert(dcl);
            }
        }
    }

    uint8_t execSize = (uint8_t) Round_Up_Pow2(size);
    G4_INST* newInst = inst[0]; //reuse inst[0] as the new merged inst

    // at this point merge will definitely succeed
    // handle merged dst
    if (dstPattern == OPND_PATTERN::DISJOINT)
    {
        G4_Type dstType = newInst->getDst()->getType();
        // create a new declare with bundle size and have the original dsts alias to it
        G4_Declare* newDcl = NULL;
        if (newInst->getDst()->getTopDcl()->getAliasDeclare() != NULL)
        {
            newDcl = newInst->getDst()->getTopDcl()->getAliasDeclare();
        }
        else
        {
            newDcl = builder.createTempVar(execSize, dstType, Eight_Word, "Merged");
        }
        for (int i = 0; i < size; ++i)
        {
            G4_Declare* dstDcl = inst[i]->getDst()->getTopDcl();
            if (dstDcl->getAliasDeclare() == NULL)
            {
                dstDcl->setAliasDeclare(newDcl, i * G4_Type_Table[dstType].byteSize);
                modifiedDcl.insert(dstDcl);
#ifdef DEBUG_VERBOSE_ON
                cout << "Dcl " << dstDcl->getName() << "--> (" << newDcl->getName() << ", "
                    << i * G4_Type_Table[dstType].byteSize << ")\n";
#endif
            }
        }
        G4_DstRegRegion* newDst = builder.createDst(newDcl->getRegVar(), 0, 0, 1, dstType);
        newInst->setDest(newDst);
    }
    else
    {
        MUST_BE_TRUE(dstPattern == OPND_PATTERN::CONTIGUOUS, "unexpected dst pattern");
    }

    for (int i = 0; i < newInst->getNumSrc(); ++i)
    {
        if (srcPattern[i] == OPND_PATTERN::DISJOINT)
        {
            G4_Operand* oldSrc = newInst->getSrc(i);
            G4_Type srcType = oldSrc->getType();
            // create a new declare with bundle size and have the original dsts alias to it
            G4_Declare* newDcl = NULL;
            if (oldSrc->getTopDcl()->getAliasDeclare() != NULL)
            {
                newDcl = oldSrc->getTopDcl()->getAliasDeclare();
            }
            else
            {
                newDcl = builder.createTempVar(execSize, srcType, Eight_Word, "Merged");
            }
            for (int j = 0; j < size; ++j)
            {
                MUST_BE_TRUE(inst[j]->getSrc(i)->isSrcRegRegion(), "Src must be a region");
                G4_SrcRegRegion* src = inst[j]->getSrc(i)->asSrcRegRegion();
                G4_Declare* srcDcl = src->getTopDcl();
                if (srcDcl->getAliasDeclare() == NULL)
                {
                    srcDcl->setAliasDeclare(newDcl, j * G4_Type_Table[srcType].byteSize);
                    modifiedDcl.insert(srcDcl);
#ifdef DEBUG_VERBOSE_ON
                    cout << "Dcl " << srcDcl->getName() << "--> (" << newDcl->getName() << ", "
                        << i * G4_Type_Table[srcType].byteSize << ")\n";
#endif
                }
            }
            G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(
                oldSrc->asSrcRegRegion()->getModifier(), Direct,
                newDcl->getRegVar(), 0, 0, builder.getRegionStride1(), srcType);
            newInst->setSrc(newSrc, i);
        }
        else if (srcPattern[i] == OPND_PATTERN::CONTIGUOUS)
        {
            // update region
            G4_SrcRegRegion* src = newInst->getSrc(i)->asSrcRegRegion();
            G4_Declare* rootDcl = src->getTopDcl()->getRootDeclare();
            if (rootDcl->isInput())
            {
                //check if the existing input is good enough
                bool sameInput = true;
                for (int instId = 1; instId < size; ++instId)
                {
                    auto dcl = inst[instId]->getSrc(i)->getTopDcl()->getRootDeclare();
                    if (rootDcl != dcl)
                    {
                        sameInput = false;
                        break;
                    }
                }

                if (sameInput)
                {
                    src->setRegion(builder.getRegionStride1());
                }
                else
                {
                    // we need a new input variable that covers all inputs used in the bundle
                    // Since rootDcl may have a different type than the operands', need to use
                    // operand's type instead of rootDcl, so we pass srcType into getInputDeclare().
                    G4_Type srcType = newInst->getSrc(i)->getType();
                    int firstEltOffset = src->getRegOff() * GENX_GRF_REG_SIZ + src->getSubRegOff() * G4_Type_Table[srcType].byteSize;
                    G4_Declare* newInputDcl = getInputDeclare(builder, newInputs, rootDcl, srcType, size, firstEltOffset);
                    src = builder.createSrcRegRegion(
                        src->getModifier(), Direct, newInputDcl->getRegVar(),
                        0, 0, builder.getRegionStride1(), src->getType());
                    newInst->setSrc(src, i);
                }
            }
            else
            {
                src->setRegion(builder.getRegionStride1());
            }
        }
        else
        {
            MUST_BE_TRUE(srcPattern[i] == OPND_PATTERN::IDENTICAL, "unexpected source pattern");
        }
    }

    newInst->setExecSize(execSize);

    auto iter = startIter;
    ++iter;
    for (int i = 1; i < size; ++i)
    {
        G4_INST* instToDelete = *iter;
        instToDelete->transferUse(newInst, true);
        for (int srcNum = 0, numSrc = instToDelete->getNumSrc(); srcNum < numSrc; ++srcNum)
        {
            Gen4_Operand_Number opndPos = instToDelete->getSrcOperandNum(srcNum);
            instToDelete->transferDef( newInst, opndPos, opndPos );
        }

        iter = bb->erase(iter);
    }

#ifdef DEBUG_VERBOSE_ON
    cout << "new inst after merging:\n";
    newInst->emit(cout);
    cout << "\n";
    newInst->emitDefUse(cout);
    cout << "\n";
#endif

    return true;
}


//
// check to see if offset1 + type size == offset2
// if so and pattern is either unknown or contiguous, return true
// if pattern is unknown also change it to contiguous
//
// also check to see if distance between offset2 and origOffset
// (offset of the first instruction found in current bundle)
// exceeds GRF size. If so returns false.
//
//
static bool checkContiguous(unsigned offset1, unsigned offset2, G4_Type type, OPND_PATTERN& pattern, unsigned origOffset)
{
    if (origOffset + GENX_GRF_REG_SIZ <= offset2)
        return false;
    if (offset1 + G4_Type_Table[type].byteSize == offset2)
    {
        switch (pattern)
        {
        case OPND_PATTERN::UNKNOWN:
            pattern = OPND_PATTERN::CONTIGUOUS;
            return true;
        case OPND_PATTERN::CONTIGUOUS:
            return true;
        default:
            return false;
        }
    }
    return false;
}

// check if this instruction is eligbile to be merged into a vector instruction
// the conditions are:
// -- arithmetic, logic, mov, math, or pseudo_mad instructions
// -- simd1, NoMask, no predicates or conditional modifier
// -- dst must be direct GRF whose declare is a single element without alias
// -- all sources must be either direct GRF or immediates
static bool isMergeCandidate(G4_INST* inst, const IR_Builder& builder, bool isInSimdFlow)
{
    if (inst->isMixedMode() && builder.getOption(vISA_DisableleHFOpt))
        return false;

    if (!inst->isArithmetic() && !inst->isLogic() && !inst->isMath() && !inst->isMov() &&
        inst->opcode() != G4_pseudo_mad)
    {
        return false;
    }

    if (inst->getExecSize() != 1 ||
        (!inst->isWriteEnableInst() && isInSimdFlow))
    {
        return false;
    }

    if (inst->getPredicate() || inst->getCondMod())
    {
        return false;
    }

    MUST_BE_TRUE(inst->getDst() != NULL, "dst must not be NULL");
    if (inst->getDst()->isIndirect())
    {
        return false;
    }

    if (builder.no64bitRegioning() && getTypeSize(inst->getDst()->getType()) == 8)
    {
        // applying merge scalar makes code quality worse since we need to insert moves
        // to compensate for the lack of 64-bit regions later
        return false;
    }

    G4_VarBase *dstBase = inst->getDst()->getBase();
    G4_Declare* dstDcl = dstBase->isRegVar() ? dstBase->asRegVar()->getDeclare() : nullptr;
    if (dstDcl != nullptr &&
        (dstDcl->getAliasDeclare() != nullptr ||
         dstDcl->getTotalElems() != 1 ||
         G4_Type_Table[inst->getDst()->getType()].byteSize != dstDcl->getElemSize() ||
         !dstDcl->useGRF()))
    {
        return false;
    }

    if (dstDcl != nullptr && (dstDcl->isOutput() || dstDcl->isInput()))
    {
        return false;
    }

    for (int i = 0; i < inst->getNumSrc(); ++i)
    {
        G4_Operand* src = inst->getSrc(i);
        if (src->isSrcRegRegion())
        {
            if (src->asSrcRegRegion()->isIndirect())
            {
                return false;
            }
            G4_Declare* srcDcl = src->getTopDcl();
            if (srcDcl == nullptr)
            {
                return false;
            }

            if (!srcDcl->useGRF() /* || srcDcl->getTotalElems() != 1 */)
            {
                return false;
            }
            // can't do opt if source decl type is inconsistent with its use
            if (G4_Type_Table[srcDcl->getElemType()].byteSize != G4_Type_Table[src->getType()].byteSize)
            {
                return false;
            }
        }
        else if (src->isImm() && !src->isRelocImm())
        {
            // ok
        }
        else
        {
            return false;
        }
    }

    return true;
}

// returns true if dcl is a scalar root variable that is naturally aligned
// Note that this also rules out input
static bool isScalarNaturalAlignedVar(G4_Declare* dcl)
{
    return dcl->getTotalElems() == 1 && dcl->getAliasDeclare() == NULL &&
        dcl->getByteAlignment() == G4_Type_Table[dcl->getElemType()].byteSize && !dcl->isInput();
}

//
// Check if the dst can be combined into the bundle
// There are 2 scenarios:
// CONTIGUOUS: the dsts together form a contiguous region
// DISJOINT: all dsts are distinct scalar, naturally aligned root variables
//
bool BUNDLE_INFO::canMergeDst(G4_DstRegRegion* dst)
{
    // src must be either Imm or SrcRegRegion

    G4_DstRegRegion* firstDst = inst[0]->getDst();
    G4_DstRegRegion* prevDst = inst[size-1]->getDst();
    if (prevDst->getType() != dst->getType())
    {
        return false;
    }

    G4_Declare* prevDstDcl = prevDst->getTopDcl();
    G4_Declare* dstDcl = dst->getTopDcl();

    if ((dstDcl == nullptr) || (prevDstDcl == nullptr))
    {
        return false;
    }

    if (prevDstDcl == dstDcl)
    {
        if (dstPattern == OPND_PATTERN::DISJOINT)
        {
            return false;
        }

        if (!checkContiguous(prevDst->getLeftBound(), dst->getLeftBound(),
            dst->getType(), dstPattern, firstDst->getLeftBound()))
        {
            return false;
        }
    }
    else if (prevDstDcl->isInput() && dstDcl->isInput())
    {
        unsigned firstDstGRFOffset = firstDst->getLeftBound() + firstDst->getTopDcl()->getRegVar()->getByteAddr();
        unsigned prevDstGRFOffset = prevDst->getLeftBound() + prevDstDcl->getRegVar()->getByteAddr();
        unsigned dstGRFOffset = dst->getLeftBound() + dstDcl->getRegVar()->getByteAddr();
        if (!checkContiguous(prevDstGRFOffset, dstGRFOffset, dst->getType(), dstPattern, firstDstGRFOffset))
        {
            return false;
        }
    }
    else
    {
        switch (dstPattern)
        {
        case OPND_PATTERN::UNKNOWN:
            //allow if both sources are size 1 root variables with no alignment
            if (isScalarNaturalAlignedVar(prevDstDcl) && isScalarNaturalAlignedVar(dstDcl))
            {
                dstPattern = OPND_PATTERN::DISJOINT;
            }
            else
            {
                return false;
            }
            break;
        case OPND_PATTERN::DISJOINT:
            if (!isScalarNaturalAlignedVar(dstDcl))
            {
                return false;
            }
            //also check to see if dst is the same as any other previous dst
            for (int i = 0; i < size - 1; i++)
            {
                G4_INST* bundleInst = inst[i];
                if (dstDcl == bundleInst->getDst()->getTopDcl())
                {
                    return false;
                }
            }
            break;
        default:
            return false;
        }
    }

    // additionally check if dst has a WAR dependence with one of the source in earlier instructions
    // it may seem that WAR dependencies should not interfere with vectorization since src is always read
    // before the dst; however, the problem is that if src and dst in different instructions refer to
    // the same variable we can't get them to align properly.
    for (int i = 0; i < size; i++)
    {
        for (int srcPos = 0; srcPos < inst[i]->getNumSrc(); ++srcPos)
        {
            G4_Operand* src = inst[i]->getSrc(srcPos);
            // since both are scalar, check is as simple as comparing the dcl
            if (src->getTopDcl() != NULL && src->getTopDcl() == dst->getTopDcl())
            {
                return false;
            }
        }
    }

    return true;
}

//
// Check if this src at srcPos can be combined into the bundle
// There are 3 scenarios:
// IDENTICAL: all sources in the bundle are the same scalar variable
// CONTIGUOUS: the sources together form a contiguous region
// DISJOINT: all sources are distinct scalar, naturally aligned root variables
//
bool BUNDLE_INFO::canMergeSource(G4_Operand* src, int srcPos)
{
    // src must be either Imm or SrcRegRegion

    MUST_BE_TRUE(srcPos < MAX_NUM_SRC, "invalid srcPos");

    if (src->isRelocImm())
    {
        return false;
    }

    if ((inst[0]->isMath() && inst[0]->asMathInst()->isOneSrcMath()) && srcPos == 1)
    {
        // null source is always legal
        srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
        return true;
    }

    G4_Operand* firstSrc = inst[0]->getSrc(srcPos);
    G4_Operand* prevSrc = inst[size-1]->getSrc(srcPos);
    if (prevSrc->getType() != src->getType())
    {
        return false;
    }
    if (prevSrc->isImm())
    {
        // ToDo: can add packed vector support later
        if (!src->isImm() ||
            prevSrc->asImm()->getImm() != src->asImm()->getImm())
        {
            return false;
        }
        srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
    }
    else
    {
        if (!src->isSrcRegRegion())
        {
            return false;
        }
        G4_SrcRegRegion* prevSrcRegion = prevSrc->asSrcRegRegion();
        G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
        if (prevSrcRegion->getModifier() != srcRegion->getModifier())
        {
            return false;
        }

        G4_Declare* prevSrcDcl = prevSrc->getTopDcl();
        G4_Declare* srcDcl = src->getTopDcl();
        if (prevSrcDcl == srcDcl)
        {
            if (srcPattern[srcPos] == OPND_PATTERN::DISJOINT)
            {
                return false;
            }

            if (prevSrc->getLeftBound() == src->getLeftBound())
            {
                // the case where we have identical source for each instruction in the bundle
                switch (srcPattern[srcPos])
                {
                case OPND_PATTERN::UNKNOWN:
                    srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
                    break;
                case OPND_PATTERN::IDENTICAL:
                    // do nothing
                    break;
                default:
                    return false;
                }
            }
            else if (!checkContiguous(prevSrc->getLeftBound(), src->getLeftBound(),
                src->getType(), srcPattern[srcPos], firstSrc->getLeftBound()))
            {
                return false;
            }
        }
        else if (prevSrcDcl->isInput() && srcDcl->isInput())
        {
            unsigned firstSrcGRFOffset = firstSrc->getLeftBound() + firstSrc->getTopDcl()->getRegVar()->getByteAddr();
            unsigned prevSrcGRFOffset = prevSrc->getLeftBound() + prevSrcDcl->getRegVar()->getByteAddr();
            unsigned srcGRFOffset = src->getLeftBound() + srcDcl->getRegVar()->getByteAddr();
            if (!checkContiguous(prevSrcGRFOffset, srcGRFOffset, src->getType(), srcPattern[srcPos], firstSrcGRFOffset))
            {
                return false;
            }
            else if (prevSrcGRFOffset / GENX_GRF_REG_SIZ != srcGRFOffset / 32)
            {
                // resulting input would cross GRF boundary, and our RA does not like it one bit
                return false;
            }

            if (inst[0]->getNumSrc() == 3)
            {
                // for 3src inst, we can't merge if inst0's src is not oword-aligned
                if ((prevSrcGRFOffset & 0xF) != 0)
                {
                    return false;
                }
            }
        }
        else
        {
            switch (srcPattern[srcPos])
            {
            case OPND_PATTERN::UNKNOWN:
                //allow if both sources are size 1 root variables with no alignment
                if (isScalarNaturalAlignedVar(prevSrcDcl) && isScalarNaturalAlignedVar(srcDcl))
                {
                    srcPattern[srcPos] = OPND_PATTERN::DISJOINT;
                }
                else
                {
                    return false;
                }
                break;
            case OPND_PATTERN::DISJOINT:
                if (!isScalarNaturalAlignedVar(srcDcl))
                {
                    return false;
                }
                //also check to see if src is the same as any other previous sources
                for (int i = 0; i < size - 1; i++)
                {
                    G4_INST* bundleInst = inst[i];
                    if (srcDcl == bundleInst->getSrc(srcPos)->getTopDcl())
                    {
                        return false;
                    }
                }
                break;
            default:
                return false;
            }
        }
    }

    if (src->isSrcRegRegion())
    {
        // we additionally have to check if src has a RAW dependency with one of the
        // previous dst in the bundle
        for (int i = 0; i < size; i++)
        {
            G4_DstRegRegion* dst = inst[i]->getDst();
            // since both are scalar, check is as simple as comparing the dcl
            if (src->getTopDcl() == dst->getTopDcl())
            {
                return false;
            }
        }
    }

    return true;
}


//
// check if inst can be successfully appended to the bundle
// For this to be successful:
// -- inst must itself be a merge candidate (checked by caller)
// -- inst must have same opcode/dst modifier/src modifier as all other instructions in the bundle
// -- dst and src operand for the inst must form one of the legal patterns with the instructions in the bundle
//
bool BUNDLE_INFO::canMerge(G4_INST* inst)
{
    G4_INST* firstInst = this->inst[0];
    if (firstInst->opcode() != inst->opcode())
    {
        return false;
    }

    if (inst->isMath())
    {
        G4_MathOp firstMathOp = MATH_RESERVED;
        if (firstInst->isMath())
        {
            firstMathOp = firstInst->asMathInst()->getMathCtrl();
        }

        if (firstMathOp != inst->asMathInst()->getMathCtrl())
        {
            return false;
        }
    }

    if (firstInst->getSaturate() != inst->getSaturate())
    {
        return false;
    }

    if (!canMergeDst(inst->getDst()))
    {
        return false;
    }


    for (int i = 0; i < inst->getNumSrc(); ++i)
    {
        if (!canMergeSource(inst->getSrc(i), i))
        {
            return false;
        }
    }

    // append instruction to bundle
    appendInst(inst);
    return true;

}

//
// iter is advanced to the next instruction not belonging to the handle
//
static void findInstructionToMerge(BUNDLE_INFO* bundle, INST_LIST_ITER& iter, const INST_LIST_ITER& lastInst, const IR_Builder& builder, bool isInSimdFlow)
{

    for (; iter != lastInst && bundle->size < bundle->sizeLimit; ++iter)
    {
        G4_INST* nextInst = *iter;
        if (!isMergeCandidate(nextInst, builder, isInSimdFlow))
        {
            break;
        }

        if (!bundle->canMerge(nextInst))
        {
            break;
        }
    }

#ifdef DEBUG_VERBOSE_ON
    if (bundle->size > 1)
    {
        cout << "Find vectorization candidates\n";
        bundle->print(std::cout);
    }
#endif

}

//
// recompute bounds for declares in the given unordered_set
// this only affects DstRegRegion and SrcRegRegion
// If the operand is global, we have to update the global operand table as well
// as the bounds may have changed
//
void Optimizer::recomputeBound(std::unordered_set<G4_Declare*>& declares)
{

    for (auto bb : fg)
    {
        for (auto ii = bb->begin(), iiEnd = bb->end(); ii != iiEnd; ++ii)
        {
            G4_INST* inst = *ii;
            if (inst->getDst() != NULL)
            {
                G4_DstRegRegion* dst = inst->getDst();
                if (dst->getTopDcl() != NULL && declares.find(dst->getTopDcl()) != declares.end())
                {
                    bool isGlobal = builder.kernel.fg.globalOpndHT.isOpndGlobal(dst);
                    dst->computeLeftBound();
                    inst->computeRightBound(dst);
                    if (isGlobal)
                    {
                        builder.kernel.fg.globalOpndHT.addGlobalOpnd(dst);
                    }
                }
            }
            for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
            {
                if (inst->getSrc(i) != NULL && inst->getSrc(i)->isSrcRegRegion())
                {
                    G4_SrcRegRegion* src = inst->getSrc(i)->asSrcRegRegion();
                    if (src->getTopDcl() != NULL && declares.find(src->getTopDcl()) != declares.end())
                    {
                        bool isGlobal = builder.kernel.fg.globalOpndHT.isOpndGlobal(src);
                        src->computeLeftBound();
                        inst->computeRightBound(src);
                        if (isGlobal)
                        {
                            builder.kernel.fg.globalOpndHT.addGlobalOpnd(src);
                        }
                    }
                }
            }
        }
    }
}

//
// Given a sequence of simd1 instructions (max 4), try to merge them into a single instruction
// e.g.,
// mul (1) r0.4<1>:f r0.0<0;1,0>:f r6.5<0;1,0>:f {NoMask}
// mul (1) r0.5<1>:f r0.1<0;1,0>:f r6.5<0;1,0>:f {NoMask}
// mul (1) r0.6<1>:f r0.2<0;1,0>:f r6.5<0;1,0>:f {NoMask}
// mul (1) r0.7<1>:f r0.3<0;1,0>:f r6.5<0;1,0>:f {NoMask}
// becomes
// mul (4) r0.4<1>:f r0.0<1;1,0>:f r6.5<0;1,0>:f {NoMask}
// A bunch of conditions have to be satisified; check BUNDLE_INFO for more details.
// This is only performed for 3D input as CM is very unlikely to benefit from this
// (put another way, if this succeeds for CM our FE is doing something wrong)
//
void Optimizer::mergeScalarInst()
{

    int bundleSizeLimit = MAX_BUNDLE_SIZE;
    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) == VISA_3D)
    {
        bundleSizeLimit = 4;
    }

    Mem_Manager mergeManager(1024);
    // set of declares that have been changed to alias to another declare
    std::unordered_set<G4_Declare*> modifiedDcl;
    std::vector<G4_Declare*> newInputs;

    //stats
    int numBundles = 0;
    int numDeletedInst = 0;

    for (G4_BB* bb : fg)
    {
        std::vector<BUNDLE_INFO*> bundles;
        INST_LIST_ITER ii = bb->begin(), iiEnd = bb->end();
        while (ii != iiEnd)
        {
            G4_INST *inst = *ii;
            auto nextIter = ii;
            ++nextIter;
            if (nextIter != iiEnd && isMergeCandidate(inst, builder, !bb->isAllLaneActive()))
            {
                BUNDLE_INFO* bundle = new (mergeManager) BUNDLE_INFO(bb, ii, bundleSizeLimit);
                findInstructionToMerge(bundle, nextIter, bb->end(), builder, !bb->isAllLaneActive());
                if (bundle->size > 1)
                {
                    bundles.push_back(bundle);
                }
                else
                {
                    bundle->~BUNDLE_INFO();
                }
                ii = nextIter;
            }
           else
            {
                ++ii;
            }
        }

        for (auto bundle : bundles)
        {
            bool success = bundle->doMerge(builder, modifiedDcl, newInputs);
            if (success)
            {
                numBundles++;
                numDeletedInst += bundle->size - 1;
            }
            bundle->~BUNDLE_INFO();
        }
    }

    // we have to reset the bound for all operands whose declares have been modified
    recomputeBound(modifiedDcl);

    if (builder.getOption(vISA_OptReport))
    {
        std::ofstream optReport;
        getOptReportStream(optReport, builder.getOptions());
        optReport << "             === Merge Scalar Optimization ===\n";
        optReport << "Number of optimized bundles:\t" << numBundles << "\n";
        optReport << "Number of instructions saved:\t" << numDeletedInst << "\n";
        closeOptReportStream(optReport);
    }
}

static bool isMad(G4_INST *I) { return I->opcode() == G4_pseudo_mad; }

static inline bool isWBTypeAndNotNull(G4_Operand *opnd)
{
    // Requires opnd to be not null.
    return opnd && (IS_BTYPE(opnd->getType()) || IS_WTYPE(opnd->getType()));
}

namespace {

/// Class to describe a mad sequence that can be turned into a sequence of MAC
/// instructions.
class MadSequenceInfo {
public:
    // IR builder object.
    IR_Builder &builder;

    // The basic block being examined.
    G4_BB *bb;

    enum MadSeqKind { MK_unknown, MK_isSafe, MK_isNotSafe};

private:
    // Flag indicates if ACC transformation is safe.
    MadSeqKind kind;

    // The single definition that defines the first mad's src2. If there are
    // multiple defintions, it is nullptr.
    G4_INST *src2Def;

    // The sequence of mad instruction to be examined.
    std::vector<G4_INST *> madSequence;

    // The use chain of the last mad. This use chain ended with an instruction
    // that has *B/*W type, and the chain length is limited by a predefined
    // constant.
    std::vector<G4_INST *> lastMadUserChain;

public:
    MadSequenceInfo(IR_Builder &builder, G4_BB *bb)
        : builder(builder), bb(bb), kind(MK_unknown), src2Def(nullptr) {}

    bool isSafe() const { return kind == MK_isSafe; }
    bool isNotSafe() const { return kind == MK_isNotSafe; }
    void setNotSafe() { kind = MK_isNotSafe ; }
    void setSafe() { kind = MK_isSafe; }

    G4_INST *getSrc2Def() { return src2Def; }
    G4_INST *getFirstMad() const { return madSequence.front(); }
    G4_INST *getLastMad() const { return madSequence.back(); }
    void appendMad(INST_LIST_ITER begin, INST_LIST_ITER end)
    {
        madSequence.insert(madSequence.end(), begin, end);
    }
    typedef std::vector<G4_INST *>::iterator mad_iter;
    mad_iter mad_begin() { return madSequence.begin(); }
    mad_iter mad_end() { return madSequence.end(); }

    void appendUser(G4_INST *inst) { lastMadUserChain.push_back(inst); }
    G4_INST *getLastUser() { return lastMadUserChain.back(); }

    void reset()
    {
        kind = MK_unknown;
        src2Def = nullptr;
        madSequence.clear();
        lastMadUserChain.clear();
    }

    // Collect all candidate instructions and perform minimal checks.
    INST_LIST_ITER populateCandidates(INST_LIST_ITER iter);

    // Make changes to all the candidates collected. This is the only
    // function that makes changes to the IR.
    void processCandidates();

private:
    void populateUserChain(G4_INST *defInst, int level);
    void populateSrc2Def();

    // Check whether this mad sequence can be turned into a MAC sequence.
    bool checkMadSequence();

    // Check whether the user chain blocks this transformation or not.
    bool checkUserChain();

    // Check if other instructions between defInst and useInst are also updating
    // ACC registers, which may block this transformation.
    bool checkACCDependency(G4_INST *defInst, G4_INST *useInst);

    // The common type for accumulator operands.
    G4_Type getCommonACCType()
    {
        G4_Type T = Type_UNDEF;

        // If there is no user chain to check. Use the last mad's destionation
        // operand type as the common type. Otherwise, use the last user's
        // destionation type.
        if (lastMadUserChain.empty())
            T = getLastMad()->getDst()->getType();
        else
            T = getLastUser()->getDst()->getType();

        return IS_FTYPE(T) ? Type_F : (IS_SIGNED_INT(T) ? Type_W : Type_UW);
    }
};

class AccRestriction {
    unsigned encoding;

public:
    // Accumulator Restriction Kind:
    enum Accumulator_RK {
      ARK_NoRestriction = 0x01,              // No restrictions.
      ARK_NoAccess = 0x02,                   // No accumulator access, implicit or explicit.
      ARK_NoSourceOperand = 0x04,            // Source operands cannot be accumulators.
      ARK_NoModifier = 0x08,                 // Source modifier is not allowed if source is an accumulator.
      ARK_NoExplicitSource = 0x010,          // Accumulator is an implicit source and thus cannot be an explicit source operand.
      ARK_NoDst = 0x20,                      // Accumulator cannot be destination, implicit or explicit.
      ARK_AccWrEnRequired = 0x40,            // AccWrEn is required. The accumulator is an implicit
                                             // destination and thus cannot be an explicit destination operand.
      ARK_NoIntegerSource = 0x80,            // Integer source operands cannot be accumulators.
      ARK_NoExplicitSrcAllowAccWrEn = 0x100, // No explicit accumulator access because this is a three-source instruction.
                                             // AccWrEn is allowed for implicitly updating the accumulator.
      ARK_NoBothSrcAndDst = 0x200            // An accumulator can be a source or destination operand but not both.
    };

    AccRestriction(unsigned val) : encoding(val) {}

    bool useAccAsSrc(G4_SrcRegRegion *opnd, bool isExplicit = true,
                     bool isAlreadyDst = false) const
    {
        if (!opnd)
            return false;

        if (encoding & ARK_NoAccess)
            return false;

        if (encoding & ARK_NoRestriction)
            return true;

        if (encoding & ARK_NoSourceOperand)
            return false;

        if (encoding & ARK_NoIntegerSource)
            return !IS_TYPE_INT(opnd->getType());

        if (encoding & ARK_NoExplicitSource)
            return !isExplicit;

        if (encoding & ARK_NoExplicitSrcAllowAccWrEn)
            return false;

        if (encoding & ARK_NoBothSrcAndDst)
            return !isAlreadyDst;

        if (encoding & ARK_NoModifier)
            return opnd->getModifier() == Mod_src_undef;

        return true;
    }

    bool useAccAsDst(G4_DstRegRegion *opnd, bool isExplicit = true,
                     bool isAlreadySrc = false) const
    {
        if (!opnd)
            return false;

        if (encoding & ARK_NoAccess)
            return false;

        if (encoding & ARK_NoRestriction)
            return true;

        if (encoding & ARK_NoDst)
            return false;

        if (encoding & ARK_AccWrEnRequired)
            return !isExplicit;

        if (encoding & ARK_NoBothSrcAndDst)
            return !isAlreadySrc;

        return true;
    }

    static AccRestriction getRestrictionKind(G4_INST *inst);
};

} // namespace

AccRestriction AccRestriction::getRestrictionKind(G4_INST *inst)
{
    switch (inst->opcode()) {
    default:
        break;
    case G4_add:
    case G4_asr:
    case G4_avg:
    case G4_csel:
    case G4_frc:
    case G4_sel:
    case G4_shr:
    case G4_smov:
        return ARK_NoRestriction;
    case G4_addc:
    case G4_subb:
        return ARK_AccWrEnRequired;
    case G4_and:
    case G4_not:
    case G4_or:
    case G4_xor:
        return ARK_NoModifier;
    case G4_cmp:
    case G4_cmpn:
    case G4_lzd:
        return ARK_NoRestriction;
    case G4_dp2:
    case G4_dp3:
    case G4_dp4:
    case G4_dph:
    case G4_line:
    case G4_movi:
    case G4_pln:
    case G4_sad2:
    case G4_sada2:
        return ARK_NoSourceOperand;
    case G4_lrp:
    case G4_mac:
        return ARK_NoExplicitSource;
    case G4_madm:
        return ARK_NoExplicitSrcAllowAccWrEn;
    case G4_mach:
        return ARK_NoExplicitSource | ARK_AccWrEnRequired;
    case G4_mov:
        return ARK_NoBothSrcAndDst;
    case G4_mul:
        return ARK_NoIntegerSource;
    case G4_rndd:
    case G4_rnde:
    case G4_rndu:
    case G4_rndz:
        return ARK_NoRestriction;
    case G4_shl:
        return ARK_NoDst;
    }

    return ARK_NoAccess;
}

/// Check this pseudo-mad's dst operand. Returns false if there is anything
/// blocking acc's usage.
static bool checkMadDst(G4_INST *inst, IR_Builder &builder)
{
    G4_DstRegRegion *dst = inst->getDst();
    if (!dst || builder.kernel.fg.globalOpndHT.isOpndGlobal(dst))
        return false;

    if (dst->getRegAccess() != Direct)
        return false;

    // Only acc0 is available for w/uw destination.
    // FIXME: This acc type size is only for simd 16.
    unsigned Sz = G4_Type_Table[Type_W].byteSize;
    Sz *= dst->getHorzStride() * inst->getExecSize();
    return Sz <= G4_GRF_REG_NBYTES;
}

// Check whether this mad sequence can be turned into a MAC sequence.
bool MadSequenceInfo::checkMadSequence()
{
    unsigned int maskOffset = getFirstMad()->getMaskOffset();

    // First check each individual mad.
    for (auto inst : madSequence)
    {
        ASSERT_USER(isMad(inst), "not a mad");

        // Only for simd 16. TODO: support simd8.
        if (inst->getExecSize() != 16)
            return false;

        if (inst->getMaskOffset() != maskOffset)
            return false;

        // Do not handle predicate yet.
        if (inst->getPredicate() != nullptr)
            return false;

        // Do not handle cond modifier yet.
        if (inst->getCondMod() != nullptr)
            return false;

        if (!checkMadDst(inst, builder))
            return false;

        G4_Operand *src0 = inst->getSrc(0);
        G4_Operand *src1 = inst->getSrc(1);
        G4_Operand *src2 = inst->getSrc(2);

        if (!src0 || !src1 || !src2)
            return false;

        if (IS_FTYPE(src0->getType()) && IS_FTYPE(src1->getType()) &&
            IS_FTYPE(src2->getType()))
        {
            // ok
        }
        else if ((!IS_BTYPE(src0->getType()) && !IS_WTYPE(src0->getType())) ||
            (!IS_BTYPE(src1->getType()) && !IS_WTYPE(src1->getType())))
        {
            // Only when src0 and src1 are of Byte/Word types.
            return false;
        }
        else if (!IS_BTYPE(src2->getType()) && !IS_WTYPE(src2->getType()) &&
                !IS_DTYPE(src2->getType()))
        {
            // Only when src2 is of Byte/Word/DWord types.
            return false;
        }

        // If there is a modifier for src2, or src2 is accessed somewhere
        // indirectly then we will not generate a MAC.
        if (!src2->isSrcRegRegion())
            return false;

        if (src2->asSrcRegRegion()->getModifier() != Mod_src_undef ||
            src2->asSrcRegRegion()->getRegAccess() != Direct ||
            (src2->getTopDcl() && src2->getTopDcl()->getAddressed()))
            return false;
    }

    // Now check instructions in pairs.
    G4_INST *defInst = getSrc2Def();
    G4_INST *lastMad = getLastMad();
    for (auto I = mad_begin(); defInst != lastMad; ++I)
    {
        G4_INST *useInst = *I;
        G4_Operand *dst = defInst->getDst();
        G4_Operand *src2 = useInst->getSrc(2);
        ASSERT_USER(dst && dst->isDstRegRegion(), "invalid dst");
        ASSERT_USER(src2 && src2->isSrcRegRegion(), "invalid src2");
        if (dst->compareOperand(src2) != Rel_eq)
            return false;

        // Move the next pair.
        defInst = useInst;
    }

    return true;
}

bool MadSequenceInfo::checkACCDependency(G4_INST *defInst, G4_INST *useInst)
{
    auto iter = std::find(bb->begin(), bb->end(), defInst);
    ASSERT_USER(iter != bb->end(), "no instruction found?");

    for (++iter; (*iter) != useInst; ++iter) {
        if ((*iter)->defAcc() || (*iter)->useAcc() || (*iter)->mayExpandToAccMacro())
            return false;
    }
    return true;
}

// Check whether this user chain is safe to use ACC.
bool MadSequenceInfo::checkUserChain()
{
    // skip if there is no user to be analyzed.
    if (lastMadUserChain.empty())
        return true;

    G4_INST *defInst = getLastMad();
    G4_INST *useInst = defInst->use_back().first;

    while (true)
    {
        // TODO: enable simd8.
        if (useInst->getExecSize() != 16)
            return false;

        // Only when used as a source operand.
        Gen4_Operand_Number opndNum = defInst->use_back().second;
        if (!G4_INST::isSrcNum(opndNum))
            return false;

        G4_Operand *useOpnd = useInst->getSrc(G4_INST::getSrcNum(opndNum));
        if (!useOpnd || !useOpnd->isSrcRegRegion())
            return false;

        if (useOpnd->asSrcRegRegion()->isIndirect())
            return false;

        // check other source type.
        for (int i = 0, e = useInst->getNumSrc(); i < e; ++i)
        {
            G4_Operand *otherSrc = useInst->getSrc(i);
            if (otherSrc == useOpnd)
                continue;

            if (!isWBTypeAndNotNull(otherSrc))
                return false;
        }

        bool isLastUser = (useInst == getLastUser());

        // The last user does not use ACC as its dst.
        AccRestriction AR = AccRestriction::getRestrictionKind(useInst);
        if (!AR.useAccAsSrc(useOpnd->asSrcRegRegion(), true, !isLastUser))
            return false;

        // Now check between defInst and useInst, no ACC will be written by
        // other instructions.
        if (!checkACCDependency(defInst, useInst))
            return false;

        if (isLastUser)
            // No extra check for the last user.
            break;

        // This is not the last user. We check its dst too.
        G4_Operand *useDst = useInst->getDst();
        if (!useDst)
            return false;

        // check type, no support for *Q types yet.
        if (!IS_DTYPE(useDst->getType()))
            return false;

        // For each inner user, need to use ACC as its explicit dst.
        if (!AR.useAccAsDst(useDst->asDstRegRegion(), true, true))
            return false;

        if (defInst->getDst()->compareOperand(useOpnd) != Rel_eq)
            return false;

        // move to next pair.
        defInst = useInst;
        useInst = defInst->use_back().first;
    }

    // nothing wrong.
    return true;
}

void MadSequenceInfo::populateSrc2Def()
{
    G4_INST *firstMad = getFirstMad();
    ASSERT_USER(firstMad && isMad(firstMad), "invalid mad");

    src2Def = firstMad->getSingleDef(Opnd_src2, true);
    if (src2Def == nullptr)
        // Cannot find a single definition.
        return setNotSafe();

    // Check it right here.
    // Only support splats or simd16 initialization.
    if (src2Def->getExecSize() != 16 && src2Def->getExecSize() != 1)
    {
        return setNotSafe();
    }

    G4_Operand *Dst = src2Def->getDst();
    if (!Dst || builder.kernel.fg.globalOpndHT.isOpndGlobal(Dst))
        return setNotSafe();

    if (Dst->asDstRegRegion()->getRegAccess() != Direct)
        return setNotSafe();

    if (src2Def->getPredicate() || src2Def->getSaturate() ||
        !src2Def->hasOneUse())
        return setNotSafe();

    if (IS_DTYPE(src2Def->getExecType()))
    {
        // since we use <1>:w region for our acc temp, due to alignment requirements
        // we can't allow dword source types
        return setNotSafe();
    }

    // Check if there is any ACC dependency.
    if (!checkACCDependency(src2Def, firstMad))
        return setNotSafe();

    // Check restrictions on compression to ensure that changing the destination
    // type will not change the source region meaning due to instruction
    // compression.
    //
    // If both instructions are compressed or both non-compressed then it is
    // safe. Otherwise, check whether source regions are compression invariants.
    if (src2Def->isComprInst() ^ firstMad->isComprInst())
    {
        auto checkCompression = [](G4_INST *inst)
        {
            for (int i = 0; i < inst->getNumSrc(); ++i)
            {
                G4_Operand *opnd = inst->getSrc(i);
                if (!opnd || !opnd->isSrcRegRegion())
                    continue;
                if (!inst->isComprInvariantSrcRegion(opnd->asSrcRegRegion(), i))
                    return false;
                if (IS_DTYPE(opnd->getType()))
                    return false;
            }
            return true;
        };

        if (src2Def->isComprInst())
        {
            if (!checkCompression(src2Def))
               return setNotSafe();
        }
        else
        {
            if (!checkCompression(firstMad))
               return setNotSafe();
        }
    }

    AccRestriction AR = AccRestriction::getRestrictionKind(src2Def);
    if (!AR.useAccAsDst(Dst->asDstRegRegion()))
        return setNotSafe();
}

void MadSequenceInfo::populateUserChain(G4_INST *defInst, int level)
{
    // Failed.
    if (level <= 0)
        return setNotSafe();

    // Only for a single use.
    if (!defInst->hasOneUse())
        return setNotSafe();

    // Only when used as a source operand.
    Gen4_Operand_Number opndNum = defInst->use_back().second;
    if (!G4_INST::isSrcNum(opndNum))
        return setNotSafe();

    G4_INST *useInst = defInst->use_back().first;
    G4_Operand *useDst = useInst->getDst();

    // Should support a null dst?
    if (useDst == nullptr)
        return setNotSafe();

    appendUser(useInst);

    // If this user has *W/*B types then candidate found and stop the search.
    if (IS_BTYPE(useDst->getType()) || IS_WTYPE(useDst->getType()))
        return;

    // Search to the next level.
    return populateUserChain(useInst, level - 1);
}

// Currently, we assume that MAD instructions are back-to-back. This avoids
// dependency checking among mad and non-mad instructions.
//
// TODO: hoist or sink instructions.
//
INST_LIST_ITER MadSequenceInfo::populateCandidates(INST_LIST_ITER iter)
{
    // Find the first pseudo-mad instruction.
    iter = std::find_if(iter, bb->end(), isMad);

    // No mad found
    if (iter == bb->end())
        return iter;

    // Find the first non-mad instruction following this sequence.
    auto end = std::find_if_not(iter, bb->end(), isMad);

    ASSERT_USER(iter != end, "out of sync");
    appendMad(iter, end);

    // Checking the first mad's src2.
    populateSrc2Def();

    // If the mad sequence has *W/*B types then it is safe to use ACC regardless
    // of the destionation operand type in its use.
    // ..
    // mac (16) acc0.0<1>:w r10.7<0;1,0>:w r62.16<16;16,1>:ub {Align1, H1}
    // mac (16) acc0.0<1>:w r11.6<0;1,0>:w r63.0<16;16,1>:ub {Align1, H1}
    // mac (16) r24.0<1>:w r11.7<0;1,0>:w r63.16<16;16,1>:ub {Align1, H1}
    // add (16) r14.0<1>:d r14.0<8;8,1>:d r24.0<8;8,1>:w {Align1, H1}
    //
    // could be generated.
    G4_Type MadDstType = getLastMad()->getDst()->getType();
    if (IS_DTYPE(MadDstType))
    {
        // Populate the user chain up to some predetermined level.
        const int level = 4;
        populateUserChain(getLastMad(), level);
    }

    // We have gathered all candidates for this optimization. Now we make
    // comprehensive checks on the mad sequence and the user chain.
    if (isNotSafe())
        return end;

    if (!checkMadSequence())
        return end;

    if (!checkUserChain())
        return end;

    // everything is OK, preceed to do transformation.
    setSafe();
    return end;
}

// Makes changes to the mad sequence and its users.
void MadSequenceInfo::processCandidates()
{
    ASSERT_USER(isSafe(), "not safe for ACC");
    ASSERT_USER(getSrc2Def(), "null src");
    ASSERT_USER(!madSequence.empty(), "no mad");

    // In this function we replace all src2 to implicit ACC registers and
    // update operands in its use chain.
    G4_Type AdjustedType = getCommonACCType();

    // Fix src2Def
    G4_INST *src2Def = getSrc2Def();
    {
        // change dst of the last MAD
        G4_DstRegRegion *accDstOpnd = builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, AdjustedType);
        src2Def->setDest(accDstOpnd);

        // Convert splat.
        if (src2Def->getExecSize() == 1)
        {
            src2Def->setExecSize(getFirstMad()->getExecSize());
        }
    }

    // update use-chain
    if (!lastMadUserChain.empty())
    {
        G4_INST *defInst = getLastMad();
        G4_INST *useInst = defInst->use_back().first;
        Gen4_Operand_Number opndNum = defInst->use_back().second;
        ASSERT_USER(defInst->hasOneUse(), "bad candidate");

        while (true)
        {
            const RegionDesc *rd = builder.getRegionStride1();
            auto mod = useInst->getOperand(opndNum)->asSrcRegRegion()->getModifier();
            G4_SrcRegRegion *accSrcOpnd = builder.createSrcRegRegion(
                mod, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, rd, AdjustedType);
            useInst->setSrc(accSrcOpnd, G4_INST::getSrcNum(opndNum));

            // The last use, only update source, and exit.
            if (useInst == getLastUser())
                break;

            // Also update the destination.
            G4_DstRegRegion *accDstOpnd = builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, AdjustedType);
            useInst->setDest(accDstOpnd);

            // move to the next pair.
            defInst = useInst;
            useInst = defInst->use_back().first;
            opndNum = defInst->use_back().second;
            ASSERT_USER(defInst->hasOneUse(), "bad candidate");
        }
    }

    // update mad sequence
    for (auto I = mad_begin(), E = mad_end(); I != E; ++I) {
        G4_INST *inst = *I;
        ASSERT_USER(isMad(inst), "not a mad");

        const RegionDesc *rd = builder.getRegionStride1();
        G4_SrcRegRegion *accSrcOpnd = builder.createSrcRegRegion(
            Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, rd,
            AdjustedType);

        inst->setImplAccSrc(accSrcOpnd);
        inst->setSrc(nullptr, 2);
        // For the last mad, if it has *B/*W type, then no user will be modified
        // and do not change its destination operand. Otherwise, use acc as the
        // destination.
        if (getLastMad() != inst || !lastMadUserChain.empty())
        {
            G4_DstRegRegion *accDstOpnd = builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, AdjustedType);
            inst->setDest(accDstOpnd);
        }
        inst->setOpcode(G4_mac);
        inst->fixMACSrc2DefUse();
    }
}

// Do any kind of proprocessing in this basic block to help MAC transformation.
// Returns false if we can easily detect this optimization is not possible.
// Otherwise, returns true.
static bool preprocessMadInBlock(IR_Builder &builder, G4_BB *bb)
{
    bool hasMad = false;
    for (auto inst : *bb)
    {
        if (isMad(inst))
        {
            hasMad = true;
            HWConformity::tryEliminateMadSrcModifier(builder, inst);
        }
    }

    // nothing to do if there is no mad.
    return hasMad;
}

//
// mul (16) V48(0,0)<1>:d r0.1<0;1,0>:w V42_in(7,2)<16;16,1>:ub {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r1.0<0;1,0>:w V42_in(7,1)<16;16,1>:ub V48(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.2<0;1,0>:w V42_in(7,3)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.3<0;1,0>:w V42_in(8,1)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.4<0;1,0>:w V42_in(8,2)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.5<0;1,0>:w V42_in(8,3)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.6<0;1,0>:w V42_in(9,1)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.7<0;1,0>:w V42_in(9,2)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.8<0;1,0>:w V42_in(9,3)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// add (16) V51_tempConvolve(0,0)<1>:d V51_tempConvolve(0,0)<16;16,1>:d 0x4000:w {Align1, H1}
// shr.sat (16) V52(0,0)<1>:ub V51_tempConvolve(0,0)<16;16,1>:d 0xf:w {Align1, H1}
//
void Optimizer::lowerMadSequence()
{

    // Only enable CM for now.
    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) != VISA_CM)
        return;

    for (G4_BB *bb : fg)
    {
        // Preprocess this basic block. If no mad sequence found then skip to
        // the next basic block right away.
        if (!preprocessMadInBlock(builder, bb))
            continue;

        // Object to gather information for ACC optimization.
        MadSequenceInfo madInfo(builder, bb);

        auto iter = bb->begin();
        while (iter != bb->end())
        {
            // Returns an iterator to the next non-mad instruction after the mad
            // sequence. It is safe to insert/delete instructions before it.
            iter = madInfo.populateCandidates(iter);

            // Perform transformation. The resulted IR may still need to be
            // fixed by HWConformity, e.g. the dst may still have *B type.
            if (madInfo.isSafe())
                madInfo.processCandidates();

            // Cleanup immediate results, whether change has been made or not.
            madInfo.reset();
        }
    }
}

void Optimizer::ifCvt()
{
    runIfCvt(fg);
}



namespace {

enum SplitBounds : unsigned {
    LoLBound = 0,
    LoRBound = 63,
    HiLBound = 64,
    HiRBound = 127
};

static bool isCandidateDecl(G4_Declare *Dcl, const IR_Builder& builder)
{
    G4_Declare *RootDcl = Dcl->getRootDeclare();
    if (RootDcl->getRegFile() != G4_GRF)
        return false;

    // Only split 4GRF variables. We should be able to split > 4GRF variables,
    // but this should have been done in FE.
    if (RootDcl->getByteSize() != 4 * GENX_GRF_REG_SIZ)
        return false;

    if (RootDcl->getAddressed())
        return false;

    if (builder.isPreDefArg(RootDcl) || builder.isPreDefRet(RootDcl))
    {
        return false;
    }

    // ToDo: add more special declares to exclude list

    return true;
}

// Associated declarations for splitting.
struct DclMapInfo {
    // The low part of the splitted variable.
    G4_Declare *DclLow;

    // The high part of the splitted variable.
    G4_Declare *DclHigh;

    // Aliases of the low part. Created if needed for different types.
    std::vector<G4_Declare *> AliasLow;

    // Aliases of the high part. Created if needed for different types.
    std::vector<G4_Declare *> AliasHigh;

    DclMapInfo(G4_Declare *Lo, G4_Declare *Hi) : DclLow(Lo), DclHigh(Hi) {}

    // Return an appropriate declaration/alias for low or high part.
    G4_Declare *getDcl(IR_Builder &Builder, G4_Type Ty, bool IsLow)
    {
        return IsLow ? getDcl(Builder, Ty, DclLow, AliasLow)
                     : getDcl(Builder, Ty, DclHigh, AliasHigh);
    }

private:
    G4_Declare *getDcl(IR_Builder &Builder, G4_Type Ty, G4_Declare *RootDcl,
                       std::vector<G4_Declare *> &Aliases)
    {
        if (Ty == RootDcl->getElemType())
            return RootDcl;

        for (auto AL : Aliases)
        {
            if (Ty == AL->getElemType())
                return AL;
        }

        // Create such an alias if it does not exist yet.
        unsigned NElts = RootDcl->getByteSize() / G4_Type_Table[Ty].byteSize;
        auto Alias = Builder.createTempVar(NElts, Ty, Any,
            (std::string(RootDcl->getName()) + "_" + G4_Type_Table[Ty].str).c_str(), false);
        Alias->setAliasDeclare(RootDcl, 0);
        Aliases.push_back(Alias);
        return Alias;
    }
};

}  // namespace

//
// mov (M1, 8) V45(0,0)<1>:q V42(0,0)<8;8,1>:d                                      /// $7
// mov (M3, 8) V45(2,0)<1>:q V42(1,0)<8;8,1>:d                                      /// $8
// shl (M1, 8) V46(0,0)<1>:q V45(0,0)<8;8,1>:q 0x2:q                                /// $9
// shl (M3, 8) V46(2,0)<1>:q V45(2,0)<8;8,1>:q 0x2:q                                /// $10
//
// into
//
// mov (M1, 8) V45L(0,0)<1>:q V42(0,0)<8;8,1>:d                                      /// $7
// mov (M3, 8) V45H(0,0)<1>:q V42(1,0)<8;8,1>:d                                      /// $8
// shl (M1, 8) V46(0,0)<1>:q V45L(0,0)<8;8,1>:q 0x2:q                                /// $9
// shl (M3, 8) V46(2,0)<1>:q V45H(0,0)<8;8,1>:q 0x2:q                                /// $10
//
void Optimizer::splitVariables()
{
    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) != VISA_3D)
    {
        return;
    }

    if (builder.getOption(vISA_Debug))
    {
        return;
    }

    // Only for simd16 and simd32.
    if (kernel.getSimdSize() == 8)
    {
        return;
    }

    // All declarations in this map are candidates for variable splitting.
    std::map<const G4_Declare *, DclMapInfo *> DclMap;

    // All instructions to be updated, and the flag indicates if this
    // instruction defines low part or ont.
    std::vector<std::pair<G4_INST *, bool>> InstsToUpdate;

    for (G4_BB *bb : fg)
    {
        for (G4_INST *inst : *bb)
        {
            // Only for variables defined by non-send instructions.
            if (inst->opcode() == G4_label || inst->isSend())
                continue;

            // TODO: Allow global variables.
            auto Dst = inst->getDst();
            if (!Dst || Dst->getHorzStride() != 1 ||
                fg.globalOpndHT.isOpndGlobal(Dst))
                continue;
            auto Dcl = Dst->getTopDcl();
            if (!Dcl || !isCandidateDecl(Dcl, builder))
                continue;

            unsigned LBound = Dst->getLeftBound();
            unsigned RBound = Dst->getRightBound();
            if ((LBound == LoLBound && RBound == LoRBound) ||
                (LBound == HiLBound && RBound == HiRBound))
            {
                // OK, only defines low or high part.
                // Bound constraints imply that def and uses are in two grfs.
            }
            else
            {
                continue;
            }

            if (inst->useEmpty())
            {
                continue;
            }

            // Check all uses.
            bool DoSplitting = true;
            for (auto UI = inst->use_begin(), UE = inst->use_end(); UI != UE; ++UI)
            {
                G4_Operand *Op = UI->first->getOperand(UI->second);
                if (Op && Op->compareOperand(Dst) != Rel_eq)
                {
                    DoSplitting = false;
                    break;
                }

                // The def-use chain is broken in the following code:
                //
                // mov (8, M1_NM) V33(0,0):df 0  <- not def
                // mov (8, M3_NM) V33(2,0):df 0  <- not def
                // mov (8, M1) V33(0,0):df 1     <- def
                // mov (8, M3_NM) V33(2,0):df 1  <- def
                //
                // use of V33(0,0) and V33(2,0)
                //
                // FIXME: remove this if def-use chain is accurate.
                if (UI->first->isWriteEnableInst() && !inst->isWriteEnableInst())
                {
                    DoSplitting = false;
                    break;
                }

                // Only allow single definition:
                // mov (8, M1_NM) V33:q 0
                // mov (8, M1)    V33:q 1
                // add (8, M1_NM  V34:q V33:q V32:q
                //
                if (UI->first->getSingleDef(UI->second) != inst)
                {
                    DoSplitting = false;
                    break;
                }
            }

            if (!DoSplitting)
                continue;

            G4_Type Ty = Dcl->getElemType();
            auto Iter = DclMap.find(Dcl);
            if (Iter == DclMap.end())
            {
                unsigned NElts = Dcl->getTotalElems();
                auto DclLow = builder.createTempVar(NElts / 2, Ty, GRFALIGN, "Lo");
                auto DclHi = builder.createTempVar(NElts / 2, Ty, GRFALIGN, "Hi");
                DclMap[Dcl] = new DclMapInfo(DclLow, DclHi);
            }
            bool IsLow = LBound == LoLBound;
            InstsToUpdate.push_back(std::make_pair(inst, IsLow));
        }
    }

    // Iterate instructions that define hi or low parts. Update their defs and uses.
    for (auto IPair : InstsToUpdate)
    {
        G4_INST *Inst = IPair.first;
        bool IsLow = IPair.second;

        // Update Inst's Dst.
        {
            G4_DstRegRegion *Dst = Inst->getDst();
            G4_Type Ty = Dst->getType();
            auto NewDcl = DclMap[Dst->getTopDcl()]->getDcl(builder, Ty, IsLow);
            auto NewDst = builder.createDst(NewDcl->getRegVar(), 0, 0, 1, Ty);
            Inst->setDest(NewDst);
        }

        // Update Inst's uses.
        for (auto UI = Inst->use_begin(), UE = Inst->use_end(); UI != UE; ++UI)
        {
            G4_INST *UseInst = UI->first;
            G4_SrcRegRegion *UseOpnd = UseInst->getOperand(UI->second)->asSrcRegRegion();
            G4_Type UseTy = UseOpnd->getType();
            auto NewUseDcl = DclMap[UseOpnd->getTopDcl()]->getDcl(builder, UseTy, IsLow);
            auto NewUseOpnd = builder.createSrcRegRegion(
                UseOpnd->getModifier(), UseOpnd->getRegAccess(),
                NewUseDcl->getRegVar(), 0, 0, UseOpnd->getRegion(), UseTy);
            UseInst->setSrc(NewUseOpnd, G4_INST::getSrcNum(UI->second));
        }
    }

    // Cleanup.
    for (auto DI : DclMap)
    {
        delete DI.second;
    }
}

//
// replacement of the above that can handle global variables
// basically we split any 4GRF variables (they typically result from
// simd16 64-bit vars) into two half if
// -- they are not address taken or used in send
// -- none of the operands cross from the 2nd to the 3rd GRF
// This is intended to give RA more freedom as the split variables do
// not have to be allocated contiguously.
// Note that this invalidates existing def-use chains
//
void Optimizer::split4GRFVars()
{
    std::unordered_set<G4_Declare*> varToSplit;
    std::vector<G4_Declare*> varToSplitOrdering;
    // map each split candidate to their replacement split variables
    std::unordered_map<const G4_Declare *, DclMapInfo *> DclMap;

    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) != VISA_3D)
    {
        return;
    }

    if (builder.getOption(vISA_Debug))
    {
        return;
    }

    // Only for simd16 and simd32.
    if (kernel.getSimdSize() == 8)
    {
        return;
    }

    // first scan the decl list
    for (auto dcl : kernel.Declares)
    {
        if (dcl->getAliasDeclare() == nullptr)
        {
            if (isCandidateDecl(dcl, builder))
            {
                if (varToSplit.find(dcl) == varToSplit.end())
                {
                    varToSplitOrdering.push_back(dcl);
                }

                varToSplit.emplace(dcl);
            }
        }
        else
        {
            // strictly speaking this condition is not necesary, but having
            // no aliases that could point into a middle of the split candidate
            // makes replacing the split var much easier. By construction the root
            // must appear before its alias decls
            uint32_t offset = 0;
            G4_Declare* rootDcl = dcl->getRootDeclare(offset);
            if (offset != 0 && isCandidateDecl(rootDcl, builder))
            {
                varToSplit.erase(rootDcl);
            }
        }
    }

    if (varToSplit.empty())
    {
        // early exit if there are no split candidate
        return;
    }

    // first pass is to make sure the validity of all split candidates
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            auto removeCandidate = [&varToSplit](G4_Declare* dcl)
            {
                if (dcl)
                {
                    dcl = dcl->getRootDeclare();
                    varToSplit.erase(dcl);
                }
            };

            if (inst->isSend())
            {
                removeCandidate(inst->getDst()->getTopDcl());
                removeCandidate(inst->getSrc(0)->getTopDcl());
                if (inst->isSplitSend())
                {
                    removeCandidate(inst->getSrc(1)->getTopDcl());
                }
            }
            else
            {
                auto cross2GRF = [](G4_Operand* opnd)
                {
                    uint32_t lb = opnd->getLeftBound();
                    uint32_t rb = opnd->getRightBound();
                    return (lb < 2u * GENX_GRF_REG_SIZ) && (rb >= 2u * GENX_GRF_REG_SIZ);
                };
                // check and remove decls with operands that cross 2GRF boundary
                if (inst->getDst())
                {
                    G4_Declare* dstDcl = inst->getDst()->getTopDcl();
                    if (dstDcl && cross2GRF(inst->getDst()))
                    {
                        removeCandidate(dstDcl);
                    }
                }
                for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                {
                    G4_Operand* src = inst->getSrc(i);
                    if (src && src->getTopDcl() && cross2GRF(src))
                    {
                        removeCandidate(src->getTopDcl());
                    }
                }
            }
        }
    }

    if (varToSplit.empty())
    {
        // early exit if there are no split candidate
        return;
    }

    // create Lo/Hi for each variable being split
    for (auto splitDcl : varToSplitOrdering)
    {
        // varToSplitOrdering may have extra elements since we never delete any inserted dcl from it.
        if (varToSplit.find(splitDcl) == varToSplit.end())
            continue;
        G4_Type Ty = splitDcl->getElemType();
        unsigned NElts = splitDcl->getTotalElems();
        std::string varName(splitDcl->getName());
        auto DclLow = builder.createTempVar(NElts / 2, Ty, GRFALIGN,
            (varName + "Lo").c_str(), false);
        auto DclHi = builder.createTempVar(NElts / 2, Ty, GRFALIGN,
            (varName + "Hi").c_str(), false);
        DclMap[splitDcl] = new DclMapInfo(DclLow, DclHi);
        //std::cerr << "split " << splitDcl->getName() << " into (" <<
        //   DclLow->getName() << ", " << DclHi->getName() << ")\n";
    }

    // second pass actually does the replacement
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            auto dst = inst->getDst();
            if (dst && dst->getTopDcl())
            {
                G4_Declare* dstRootDcl = dst->getTopDcl()->getRootDeclare();
                if (DclMap.count(dstRootDcl))
                {
                    bool isLow = dst->getLeftBound() < 2u * GENX_GRF_REG_SIZ;
                    auto NewDcl = DclMap[dstRootDcl]->getDcl(builder, dst->getType(), isLow);
                    auto NewDst = builder.createDst(NewDcl->getRegVar(),
                        dst->getRegOff() - (isLow ? 0 : 2), dst->getSubRegOff(),
                        dst->getHorzStride(), dst->getType(), dst->getAccRegSel());
                    inst->setDest(NewDst);
                }
            }

            for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
            {
                G4_Operand* src = inst->getSrc(i);
                if (src && src->getTopDcl())
                {
                    G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
                    G4_Declare* srcRootDcl = src->getTopDcl()->getRootDeclare();
                    if (DclMap.count(srcRootDcl))
                    {
                        bool isLow = src->getLeftBound() < 2u * GENX_GRF_REG_SIZ;
                        auto NewSrcDcl = DclMap[srcRootDcl]->getDcl(builder, src->getType(), isLow);
                        auto NewSrc = builder.createSrcRegRegion(
                            srcRegion->getModifier(), src->getRegAccess(),
                            NewSrcDcl->getRegVar(), srcRegion->getRegOff() - (isLow ? 0 : 2),
                            srcRegion->getSubRegOff(), srcRegion->getRegion(), src->getType(), src->getAccRegSel());
                        inst->setSrc(NewSrc, i);
                    }
                }
            }
        }
    }

    for (auto DI : DclMap)
    {
        delete DI.second;
    }
}

//
// for some platforms int has half throughout compared to float,
// so for copy moves we should change their type
// from D/UD to F or W/UW to HF when possible
//
void Optimizer::changeMoveType()
{
    if (!builder.favorFloatMov() && !builder.balanceIntFloatMoves())
    {
        return;
    }

    auto changeType = [this](G4_INST* movInst, G4_Type newTy)
    {
        movInst->getDst()->setType(newTy);
        auto src0 = movInst->getSrc(0);
        if (src0->isImm())
        {
            uint32_t mask = getTypeSize(newTy) == 4 ? 0xFFFFFFFF : 0xFFFF;
            movInst->setSrc(fg.builder->createImm(src0->asImm()->getImm() & mask, newTy), 0);
        }
        else
        {
            movInst->getSrc(0)->asSrcRegRegion()->setType(newTy);
        }
    };




    for (auto bb : fg)
    {
        for (auto inst : *bb)
        {
            if (inst->opcode() != G4_mov)
            {
                continue;
            }
            // copy move means dst and src has identical bits (implies same type width), and there are no
            // sat/conditional modifier as well as src modifier
            // ToDo: we should probably change isRawMov() to include mov UD D
            auto src0 = inst->getSrc(0);

            // FIXME: This is a quick WA to bypass RelocImm, so that it won't create a new src0 and
            // overwrite the original RelocImm
            // While this optimization should still be able to apply to RelocImm. Once we turn on this optimization
            // for RelocImm, we should update assert in VISAKernelImpl::GetGenRelocEntryBuffer to allow float type
            if (src0->isRelocImm())
                continue;

            G4_Type dstTy = inst->getDst()->getType();
            G4_Type src0Ty = src0->getType();
            bool hasNoModifier = !inst->getSaturate() && !inst->getCondMod() &&
               (src0->isImm() || (src0->isSrcRegRegion() && src0->asSrcRegRegion()->getModifier() == Mod_src_undef));

            // it may be unsafe to change the move type for acc as it has higher precision
            if (inst->getDst()->isGreg() && hasNoModifier)
            {
                if (src0->isGreg())
                {
                    bool isIntCopyMove = IS_TYPE_INT(dstTy) && IS_TYPE_INT(src0Ty) &&
                        getTypeSize(dstTy) == getTypeSize(src0Ty);
                    if (isIntCopyMove)
                    {
                        if (dstTy == Type_D || dstTy == Type_UD)
                        {
                            changeType(inst, Type_F);
                        }
                        else if (dstTy == Type_W || dstTy == Type_UW)
                        {
                            changeType(inst, Type_HF);
                        }
                    }
                }
                else if (src0->isImm())
                {
                    // allow sext and zext imm moves
                    int64_t immVal = src0->asImm()->getImm();
                    bool isIntImmMove = IS_TYPE_INT(dstTy) && IS_TYPE_INT(src0Ty) &&
                        G4_Imm::isInTypeRange(immVal, dstTy);
                    if (isIntImmMove)
                    {
                        if (dstTy == Type_D || dstTy == Type_UD)
                        {
                            changeType(inst, Type_F);
                        }
                        else if (dstTy == Type_W || dstTy == Type_UW)
                        {
                            changeType(inst, Type_HF);
                        }
                    }
                }
            }
        }
    }
}


// NoDD
namespace {
    // This describes the part of the GRF accessed.
    struct AccessMask {
        uint64_t mask;
        // int from;
        // int to;
        void setMask(int From, int To) {
            mask = (((uint64_t) 1L << (To + 1L)) - 1L)
                & ~(((uint64_t) 1L << From) - 1L);
        }
        AccessMask(void) {
            mask = 0;
        }
        AccessMask(int From, int To) {
            setMask(From, To);
        }
        AccessMask(G4_Operand *opnd, int bucket) {
            int linearStart = opnd->getLinearizedStart();
            int linearEnd = opnd->getLinearizedEnd();
            int buLinearStart = bucket * G4_GRF_REG_NBYTES;
            int buLinearEnd = buLinearStart + G4_GRF_REG_NBYTES - 1;

            // If the OPND is accessing more than 1 GRF
            // get the addresses that fit within the bucket
            int fittedLinearStart = std::max(linearStart, buLinearStart);
            int fittedLinearEnd = std::min(linearEnd, buLinearEnd);

            int from = fittedLinearStart - buLinearStart;
            assert(from >= 0);
            int to = fittedLinearEnd - buLinearStart;
            setMask(from, to);
            assert(to < G4_GRF_REG_NBYTES);
        }

        // Return TRUE if this mask and AM have some overlap
        bool hasOverlapWith(const AccessMask &AM) const {
            // if (AM.from >= from) {
            //     return AM.from <= to;
            // } else {
            //     return AM.to >= from;
            // }
            return (mask & AM.mask) != 0;
        }
        // Return TRUE if this mask fully covers the AM mask
        bool fullyCovers(const AccessMask &AM) const {
            // return (from <= AM.from && to >= AM.to);
            return (mask | AM.mask) == mask;
        }
        // Return TRUE if the mask covers the full GRF
        bool coversFullGRF(void) const {
            // return (to > from
            //         && (from % G4_GRF_REG_NBYTES) == 0
            //         && (to & G4_GRF_REG_NBYTES) == 0);
            return mask == ((uint64_t) 1 << G4_GRF_REG_NBYTES) - (uint64_t) 1;
        }
        void setBits(const AccessMask &AM) {
            mask |= AM.mask;
        }
        void clearBits(void) {
            mask = 0;
        }
        void clearBits(const AccessMask &AM) {
            mask &= ~AM.mask;
        }
        // Return true if all bits set in AM are also set in this mask
        bool areBitsSet(const AccessMask &AM) const {
            return (AM.mask & mask) == AM.mask;
        }
        // Debug print
        void dump(void) const {
            for (int i = 0; i < G4_GRF_REG_NBYTES; ++i) {
                if (((uint64_t) 1 << i) & mask)
                    std::cerr << "1";
                else
                    std::cerr << "0";
            }
            fprintf(stderr, "    0x%lX\n", (unsigned long)mask);
        }
    };




    enum RW {
        RW_UNINIT = 0,
        READ = 1 << 0,
        WRITE = 1 << 1,
        RW_MAX
    };


    // Points to the bucket
    struct BucketDescr {
        int bucketIdx;
        RW type;
        AccessMask mask;
        bool isARF;
        G4_Type dataType;

        BucketDescr(int BI, G4_Type DT, RW RWT, AccessMask AM, bool IsARF)
            : bucketIdx(BI), type(RWT), mask(AM), isARF(IsARF), dataType(DT)
        {
            assert(dataType != Type_UNDEF && "Bad data type ?");
        }
        bool sameReg(const BucketDescr &BD) const {
            return bucketIdx == BD.bucketIdx;
        }
        bool sameDataType(const BucketDescr &BD) const {
            return dataType == BD.dataType;
        }
        bool hasOverlapWith(const BucketDescr &BD) const {
            return sameReg(BD) && mask.hasOverlapWith(BD.mask);
        }
        bool accessesARF(void) const {
            return isARF;
        }
    };

    typedef std::vector<BucketDescr> BDVec_t;

    // Parse INSTR's OPND and generate the Bucket Descriptors.
    // Insert them into BDVec.
    void getBucketsForOperand(G4_INST* inst, RW rwType, G4_Operand* opnd,
                              BDVec_t& BDVec) {
        if (opnd->isLabel() || opnd->isImm()) {
            return;
        }
#define UNINIT_BUCKET -1
        int startingBucket = UNINIT_BUCKET;
        G4_VarBase* base = opnd->getBase();
        bool canSpanMultipleBuckets = false;
        bool isARF = false;

        assert(base && "If no base, then the operand is not touched by instr.");

        // If a register allocated regvar, then get the physical register
        G4_VarBase *phyReg = (base->isRegVar()) ?
            base->asRegVar()->getPhyReg() : base;

        switch (phyReg->getKind()) {
        case G4_VarBase::VK_phyGReg:
            // Handle GRFs - fast path
            startingBucket = opnd->getLinearizedStart()/G4_GRF_REG_NBYTES;
            canSpanMultipleBuckets = true;
            break;
        case G4_VarBase::VK_phyAReg:
            isARF = true;
            break;
        default:
            assert(0 && "Bad kind");
            break;
        }

        // Create one or more buckets and push them into the vector
        if (startingBucket != UNINIT_BUCKET) {
            if (canSpanMultipleBuckets) {
                assert(! isARF);
                unsigned int divisor;
                int baseBucket;
                assert(base->isGreg());
                divisor = G4_GRF_REG_NBYTES;
                baseBucket = 0;
                int endingBucket = baseBucket
                    + opnd->getLinearizedEnd() / divisor;
                MUST_BE_TRUE(endingBucket >= startingBucket,
                             "Ending bucket less than starting bucket");
                int numBuckets = endingBucket - startingBucket + 1;
                for (int bucket_j = startingBucket;
                     bucket_j < (startingBucket + numBuckets); bucket_j++ ) {
                    AccessMask AM(opnd, bucket_j);
                    BDVec.push_back(BucketDescr(bucket_j, opnd->getType(),
                                                rwType, AM, isARF));
                }
            }
            else {
                AccessMask AM(opnd, startingBucket);
                BDVec.push_back(BucketDescr(startingBucket, opnd->getType(),
                                            rwType, AM, isARF));
            }
        }
    }

    // We pack all information we have collected for the instruction
    struct BucketDescrWrapper {
        BucketDescrWrapper() : hasIndirW(false) {
            BDVec.reserve(16);
        }
        // The vector of buckets read/writen by the instruction
        BDVec_t BDVec;
        // The instruction has an indirect write
        bool hasIndirW;
        // Return TRUE if there is a RAW dependence between this and prevBDW
        // even without any overlap: e.g:
        // math (8) r86.0<1>:hf r86.0<8;8,1>:hf null:ud
        // math (8) r86.8<1>:hf r86.8<8;8,1>:hf null:ud
        // Although there is no a real data movement between
        // r86.0<1>:hf -> r86.8<8;8,1>:hf, adding noDDclr and noDDChck would
        // lead to a hang
        bool hasRAW(const BucketDescrWrapper &prevBDW) const {
            for (const auto &prevBD : prevBDW.BDVec) {
                if (prevBD.type == WRITE) {
                    for (const auto &currBD : BDVec) {
                        if (currBD.type == READ) {
                            // if (currBD.hasOverlapWith(prevBD)) {
                            if (currBD.sameReg(prevBD)) {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

        // Return TRUE if there is a subreg WAW dependence between
        // this and prevBDW
        // This is TRUE only if the data types match.
        bool hasSubregWAW(const BucketDescrWrapper &prevBDW) const {
            bool noWAW = false;
            for (const auto &prevBD : prevBDW.BDVec) {
                if (prevBD.type == WRITE) {
                    for (const auto &currBD : BDVec) {
                        if (currBD.type == WRITE) {
                            if (currBD.sameReg(prevBD)) {
                                if (! currBD.hasOverlapWith(prevBD)
                                    && currBD.sameDataType(prevBD)) {
                                    noWAW = true;
                                } else {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
            return noWAW;
        }

        // Return TRUE if writing to more than 1 GRF
        bool touchesManyGRFs(void) const {
            std::set<int> bucketsTouched;
            for (const auto &BD : BDVec) {
                if (BD.type == WRITE) {
                    bucketsTouched.insert(BD.bucketIdx);
                }
            }
            return bucketsTouched.size() > 1;
        }

        // Return TRUE if we are accessing an ARF
        bool accessesARF(void) const {
            for (const auto &BD : BDVec) {
                if (BD.accessesARF()) {
                    return true;
                }
            }
            return false;
        }
    };

    // Given an inst with physical register assignment,
    // return all bucket descriptors that the physical register can map
    // to. This requires taking in to account exec size, data
    // type, and whether inst is a send
    BucketDescrWrapper getBucketDescrsForInst(G4_INST* inst,
                                              BucketDescrWrapper &instrBuckets) {
        // Iterate over all WRITTEN operands and create buckets.
        for (Gen4_Operand_Number opndNum : {Opnd_dst}) {
            G4_Operand *opnd = inst->getOperand(opndNum);
            if (! opnd || ! opnd->getBase()) {
                continue;
            }
            getBucketsForOperand(inst, WRITE, opnd, instrBuckets.BDVec);
            instrBuckets.hasIndirW |= opnd->asDstRegRegion()->isIndirect();
        }
        for (Gen4_Operand_Number opndNum
                 : {Opnd_src0, Opnd_src1, Opnd_src2, Opnd_src3}) {
            G4_Operand *opnd = inst->getOperand(opndNum);
            if (! opnd || ! opnd->getBase()) {
                continue;
            }
            getBucketsForOperand(inst, READ, opnd, instrBuckets.BDVec);
        }
        return instrBuckets;
    }


    bool mustAddNoDD(const BucketDescrWrapper &prevBDW,
                     const BucketDescrWrapper &currBDW) {
        if (currBDW.hasRAW(prevBDW)) {
            return false;
        }
        // This also confirms that the data types are equal
        if (currBDW.hasSubregWAW(prevBDW)) {
            return true;
        }
        return false;
    }

    static bool areOnSamePipeline(G4_INST *prevInstr, G4_INST *currInstr) {
        // Forbid different opcodes
        if (prevInstr->opcode() != currInstr->opcode()) {
            return false;
        }
        // Math check type
        if (prevInstr->isMath() && currInstr->isMath()) {
            auto prevMathTy = prevInstr->asMathInst()->getMathCtrl();
            auto currMathTy = currInstr->asMathInst()->getMathCtrl();
            return prevMathTy == currMathTy;
        }

        return true;
    }

}

// Scan the code and insert NoDDclr and NoDDchk flags to avoid unnecessary
// latencies when accessing sub-registers.
// This happens because the scoreboard works with a GRF granularity.
// Therefore a sub-reg is considered to access the whole GRF and dependencies
// are over-estimated.
//
// NoDDclr: Do not mark the destination registers in the scoreboard
// NoDDchk: Do not check the scoreboard for the source registers
//
// Example:
//     A: add(8) R1.0 = ...
//     B: add(8) R1.8 = ...
//    Instruction B will wait until A has written back to the RF because
//    R1.0 and R1.8 alias in the scoreboard
//   Solution:
//     A: add(8) R1.0 = ... {NoDDclr}
//     B: add(8) R1.8 = ... {NoDDchk}
//    Now B will issue right after A.
//
static bool tryToAddNoDD(G4_INST *currInstr, G4_INST *prevInstr,
                         BucketDescrWrapper &prevBDW,
                         BucketDescrWrapper &currBDW,
                         const Options *options) {
    // 1. To be safe, predicated instrs can kill but cannot cover.
    // 2. Must be on same pipeline
    // 3. If subreg WAW and no RAW dep, insert NoDD flags
    if (prevInstr
        && ! prevBDW.touchesManyGRFs()
        && ! prevBDW.hasIndirW
        && ! prevInstr->isSend()
        && ! currInstr->isSend()
        && ! currInstr->getPredicate()
        && ! prevInstr->getPredicate()
        && ! prevBDW.accessesARF()
        && ! currBDW.accessesARF()
        && areOnSamePipeline(prevInstr, currInstr)
        && mustAddNoDD(prevBDW, currBDW)) {

        // Tag the instructions with the NoDD flag
        prevInstr->setOptionOn(InstOpt_NoDDClr);
        currInstr->setOptionOn(InstOpt_NoDDChk);

        // Sanity check
        G4_Operand *dst = prevInstr->getOperand(Opnd_dst);
        G4_Operand *src = currInstr->getOperand(Opnd_src0);
        if (dst && src)
        {
            G4_VarBase *dstBase = dst->getBase();
            G4_VarBase *srcBase = src->getBase();
            if (dstBase && srcBase && dstBase->isRegVar() && srcBase->isRegVar())
            {
                G4_VarBase *dstReg = (dstBase->isRegVar()) ?
                    dstBase->asRegVar()->getPhyReg() : dstBase;
                G4_VarBase *srcReg = (srcBase->isRegVar()) ?
                    srcBase->asRegVar()->getPhyReg() : srcBase;
                if (srcReg && srcReg->isGreg() && dstReg && dstReg->isGreg())
                {
                    auto dstPhyReg = dstReg->asGreg();
                    int dstNum = dstPhyReg->getRegNum();
                    auto srcPhyReg = srcReg->asGreg();
                    int srcNum = srcPhyReg->getRegNum();
                    assert(dstNum != srcNum);
                }
            }
        }
        // DEBUGGING: dump NoDD pairs into a file
        if (options->getOption(vISA_DebugNoDD)) {
            fstream ofile("C:\\Intel\\nodd.txt",
                          std::fstream::out | std::fstream::app);
            ofile << *prevInstr << std::endl;
            ofile << *currInstr << std::endl << std::endl;
        }
        return true;
    }
    return false;
}

// NOTE: It is currently disabled (vISA_EnableNoDD == false).
void Optimizer::NoDD(void) {
    const Options *options = builder.getOptions();
    if (! builder.noDDAllowedPlatform()
        || ! options->getOption(vISA_EnableNoDD)) {
        return;
    }
    const int MAX_LOOK_BACK = options->getuInt32Option(vISA_NoDDLookBack);

    for (auto bb : fg) {
        // Keep the last few instructions
        std::vector<G4_INST *> prevInstrs;
        std::vector<BucketDescrWrapper> prevBDWs;
        // G4_INST *prevInstr = nullptr;
        // BucketDescrWrapper prevBDW;
        for (auto currInstr : *bb) {
            if (currInstr->isLabel()) {
                continue;
            }
            BucketDescrWrapper currBDW;
            (void) getBucketDescrsForInst(currInstr, currBDW);

            // Try to insert NoDD in MAX_LOOK_BACK previous intructions
            bool succ = false;
            int prevIdx = 0;
            while (! succ && prevIdx < (int)prevInstrs.size()) {
                G4_INST *prevInstr = prevInstrs[prevIdx];
                BucketDescrWrapper &prevBDW = prevBDWs[prevIdx];
                succ = tryToAddNoDD(currInstr, prevInstr,
                                    prevBDW, currBDW, options);
                prevIdx++;
            }
            prevInstrs.insert(prevInstrs.begin(), currInstr);
            prevBDWs.insert(prevBDWs.begin(), currBDW);
            if (prevInstrs.size() == MAX_LOOK_BACK) {
                prevInstrs.pop_back();
                prevBDWs.pop_back();
            }
        }
    }
}

static bool isDeadInst(FlowGraph& fg, G4_INST* Inst)
{
    if (Inst->isMov() || Inst->isLogic() || Inst->isCompare() ||
        Inst->isArithmetic() || Inst->isVector()) {

        // Check side-effects.
        // - no global
        // - no indirect
        // - not physically assigned (including ARF)
        auto checkOpnd = [&](G4_Operand *Opnd) {
            if (Opnd == nullptr || Opnd->isNullReg())
                return true;
            if (fg.globalOpndHT.isOpndGlobal(Opnd))
                return false;
            if (Opnd->isDstRegRegion() && Opnd->asDstRegRegion()->isIndirect())
                return false;
            G4_VarBase *Base = Opnd->getBase();
            if (!Base->isRegVar())
                return false;
            if (Base->asRegVar()->isPhyRegAssigned())
                return false;
            return true;
        };

        // Should have no use.
        if (Inst->use_size() > 0)
            return false;

        // Skip instructions with special attributes.
        if (Inst->isYieldInst() || Inst->isBreakPointInst())
            return false;

        // Check defs. Assuming acc operands are all locally defined
        // and def-use are correctly maintained.
        if (!checkOpnd(Inst->getDst()) ||
            !checkOpnd(Inst->getCondMod()))
            return false;

        // At this point, this instruction is dead.
        return true;
    }

    // By default it is not dead.
    return false;
}

void Optimizer::dce()
{
    for (auto bb : fg) {
        for (auto I = bb->rbegin(), E = bb->rend(); I != E; ++I) {
            G4_INST* Inst = *I;
            if (isDeadInst(fg, Inst)) {
                Inst->removeAllDefs();
                Inst->markDead();
            }
        }
        bb->erase(
            std::remove_if(bb->begin(), bb->end(), [](G4_INST* Inst) { return Inst->isDead(); }),
            bb->end());
    }
}

static bool retires(G4_Operand* Opnd, G4_INST* SI)
{
    assert(Opnd && Opnd->isGreg());
    unsigned LB = Opnd->getLinearizedStart() / GENX_GRF_REG_SIZ;
    unsigned RB = Opnd->getLinearizedEnd() / GENX_GRF_REG_SIZ;

    auto overlaps = [=](G4_Operand* A) {
        if (A == nullptr || A->isNullReg() || !A->isGreg())
            return false;
        unsigned LB1 = A->getLinearizedStart() / GENX_GRF_REG_SIZ;
        unsigned RB1 = A->getLinearizedEnd() / GENX_GRF_REG_SIZ;
        return (RB >= LB1 && RB1 >= LB);
    };

    // RAW or WAW
    if (overlaps(SI->getDst()))
        return true;

    if (Opnd->isSrcRegRegion())
        return false;

    // WAR.
    if (overlaps(SI->getSrc(0)))
        return true;
    if (SI->isSplitSend() && overlaps(SI->getSrc(1)))
        return true;

    // Do not retire this send.
    return false;
}

// Emit a self-move to retire this send.
static G4_INST* emitRetiringMov(IR_Builder& builder, G4_BB* BB, G4_INST* SI,
                                INST_LIST_ITER InsertBefore)
{
    assert(SI && SI->isSend());
    G4_Operand* Src0 = SI->getSrc(0);

    unsigned RegNum = Src0->getLinearizedStart() / GENX_GRF_REG_SIZ;
    G4_Declare* Dcl = builder.createTempVar(16, Type_F, Any);
    Dcl->setGRFBaseOffset(RegNum * G4_GRF_REG_NBYTES);
    Dcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(RegNum), 0);

    G4_DstRegRegion* MovDst = builder.createDst(Dcl->getRegVar(), 0, 0, 1, Type_F);
    G4_SrcRegRegion* MovSrc = builder.createSrcRegRegion(
        Mod_src_undef, Direct, Dcl->getRegVar(), 0, 0,
        builder.getRegionStride1(), Type_F);
    G4_INST* MovInst = builder.createMov(8, MovDst, MovSrc, InstOpt_M0 | InstOpt_WriteEnable, false);
    BB->insert(InsertBefore, MovInst);
    return MovInst;
}

// Use this instruction to retire live sends.
static void retireSends(std::vector<G4_INST*>& LiveSends, G4_INST* Inst)
{
    if (LiveSends.empty())
        return;

    // Predicated instructions may not retire a send.
    if (Inst->getPredicate() != nullptr && Inst->opcode() != G4_sel)
        return;

    // Collect operands for dependency checking.
    std::vector<G4_Operand*> Opnds;
    if (G4_DstRegRegion* Dst = Inst->getDst()) {
        if (!Dst->isNullReg() && !Dst->isIndirect() && Dst->isGreg())
            Opnds.push_back(Dst);
    }
    for (int i = 0; i < Inst->getNumSrc(); ++i) {
        G4_Operand* Opnd = Inst->getSrc(i);
        if (Opnd == nullptr || !Opnd->isSrcRegRegion() || Opnd->isNullReg())
            continue;
        G4_SrcRegRegion* Src = Opnd->asSrcRegRegion();
        if (!Src->isIndirect() && Src->isGreg())
            Opnds.push_back(Opnd);
    }

    // WRA, RAW or WAW dependency retires a live send.
    bool Changed = false;
    for (auto Opnd : Opnds) {
        for (auto& SI : LiveSends) {
            if (SI && retires(Opnd, SI)) {
                SI = nullptr;
                Changed = true;
            }
        }
    }
    // Remove nullptr values when there are changes.
    if (Changed) {
        auto Iter = std::remove(LiveSends.begin(), LiveSends.end(), (G4_INST*)0);
        LiveSends.erase(Iter, LiveSends.end());
    }
}

// Limit the number of live sends and clear all sends at the end of a block.
void Optimizer::clearSendDependencies()
{
    for (auto BB : fg) {
        // Live send instructions. This vector will only have MAX_SENDS
        // or less instructions.
        const unsigned MAX_SENDS = 3;
        std::vector<G4_INST*> LiveSends;

        for (auto I = BB->begin(); I != BB->end(); /*empty*/) {
            auto CurI = I++;
            G4_INST* Inst = *CurI;

            // Try to retire live sends.
            retireSends(LiveSends, Inst);
            if (!Inst->isSend())
                continue;

            // This is a send.
            if (LiveSends.size() >= MAX_SENDS) {
                // OK, too many live sends. Retire the earliest live send.
                G4_INST* SI = LiveSends.front();
                G4_INST* MovInst = emitRetiringMov(builder, BB, SI, CurI);
retireSends(LiveSends, MovInst);
assert(LiveSends.size() < MAX_SENDS);
            }

            // If this is EOT and send queue is not full, then nothing to do.
            // Otherwise a new send becomes live.
            if (Inst->isEOT())
                LiveSends.clear();
            else
                LiveSends.push_back(Inst);
        }

        // Retire remainig live sends in this block, if any.
        for (auto SI : LiveSends) {
            assert(SI && SI->isSend());
            auto InsertBefore = BB->end();
            G4_INST* LastInst = BB->back();
            if (LastInst->isFlowControl())
                InsertBefore = std::prev(InsertBefore);
            emitRetiringMov(builder, BB, SI, InsertBefore);
        }
    }
}

// [HW WA]
// Fused Mask cannot change from 01 to 00, therefore, EU will go through
// insts that it should skip, causing incorrect result if NoMask instructions
// that has side-effect (send, or modifying globals, etc) are executed.
// A WA is to change any NoMask instruction by adding a predicate to it.
// And the predicated is equivalent to NoMask. For example, the following
// instruction
//
//    (W) add (8|M0)  r10.0<1>:d  r11.0<1;1,0>:d  r12.0<1;1,0>:d
//
//  will be changed to
//
//    (W)  mov (1|M0) f0.0<1>:w  0
//         cmp (8|M0) (eq)f0.0 r0:uw  r0:uw
//    (W&f0.0.any8h) add (8|M0)  r10.0<1>:d  r11.0<1;1,0>:d  r12.0<1;1,0>:d
//
//  Note that f0.0 is called "emask flag".
//
// Even with this HW bug, the HW still have the correct CE mask so that the
// above mov&cmp sequence still works, that is, f0.0 will be all zero if no
// active lanes and will not be zero if there is at least one active lane.
//
// Nested Divergence
//   For a fused mask to be 01,  the control-flow must be divergent
//   at that point. Furthermore, changing 01 to 00 happens only if a further
//   divergence happens within a already-divergent path. This further
//   divergence is called nested divergence here.
//
//   As changing from 01 to 00 never happens with backward goto, backward
//   goto is treated as divergent, but not nested divergent for the purpose
//   of this WA.
//
// This function first finds out which BB are in nested divergent branch and
// then add predicates to those NoMask instructions.
//
void Optimizer::doNoMaskWA()
{
    std::unordered_map<G4_BB*, int> nestedDivergentBBs;

    // Identify BBs that need WA
    fg.reassignBlockIDs();
    fg.findNestedDivergentBBs(nestedDivergentBBs);

    std::vector<INST_LIST_ITER> NoMaskCandidates;
    uint32_t simdsize = fg.getKernel()->getSimdSize();

    // When using cmp to generate emask, the default is to create
    // all-one flag so it applies to all execution size and quarter
    // control combination. Doing so needs 3 instructions for each BB.
    // On the other hand, if anyh can be used, 2 insts would be needed
    // so that we save 1 insts for each BB. The condition that anyh
    // can be used is that M0 is used for all NoMask insts that needs
    // WA and all its execsize is no larger than simdsize.
    bool enableAnyh = true; // try to use anyh if possible
    bool  useAnyh = enableAnyh;  // Set for each BB/inst.

    auto getPredCtrl = [&](bool isUseAnyh) -> G4_Predicate_Control
    {
        if (isUseAnyh)
        {
            return simdsize == 8 ? PRED_ANY8H
                                 : (simdsize == 16 ? PRED_ANY16H : PRED_ANY32H);
        }
        return PRED_DEFAULT;
    };

    // Return condMod if a flag register is used. Since sel
    // does not update flag register, return null for sel.
    auto getFlagModifier = [](G4_INST* I) -> G4_CondMod* {
        if (I->opcode() == G4_sel) {
            return nullptr;
        }
        return I->getCondMod();
    };

    // Return true if a NoMask inst is either send or global
    auto isCandidateInst = [&](G4_INST* Inst, FlowGraph& cfg) -> bool {
        // pseudo should be gone at this time [skip all pseudo].
        if (!Inst->isWriteEnableInst() ||
            Inst->isCFInst() ||
            Inst->isPseudoLogic() ||
            Inst->isPseudoKill())
        {
            return false;
        }

        G4_DstRegRegion* dst = Inst->getDst();
        G4_CondMod* condmod = getFlagModifier(Inst);
        bool dstGlb = (dst && !dst->isNullReg() && cfg.globalOpndHT.isOpndGlobal(dst));
        bool condmodGlb = (condmod && cfg.globalOpndHT.isOpndGlobal(condmod));
        if (Inst->isSend() || dstGlb || condmodGlb)
        {
            if (Inst->isSend() && Inst->getPredicate() &&
                Inst->getExecSize() > simdsize)
            {
                // fused send, already correctly predicated, skip
                return false;
            }
            return true;
        }
        return false;
    };

    // Use cmp to calculate "emask flag(or flag)" (see comment at entry of this function)
    auto createFlagFromCmp = [&](G4_INST*& flagDefInst,
        uint32_t flagBits, G4_BB* BB, INST_LIST_ITER& II)->G4_RegVar*
    {
        //  Ty  (for flag):  big enough to hold flag for either anyh or all one flag.
        //                   if useAnyh
        //                      Ty = (simdsize > 16 ? UD : UW
        //                   else
        //                      Ty = max(simdsize, flagBits) > 16 ? UD : UW
        //
        //  if useAnyh
        //    I0:               (W) mov (1|M0)  flag:Ty,  0
        //    flagDefInst:          cmp (simdsize|M0) (eq)flag  r0:uw  r0:uw
        //  else
        //    I0:               (W) mov (1|M0)  flag:Ty,  0
        //    I1:                   cmp (simdsize|M0) (eq)flag  r0:uw  r0:uw
        //    flagDefInst: (W&flag.anyh) mov flag:Ty 0xFFFFFFFF:Ty
        //
        G4_Type Ty = (simdsize > 16) ? Type_UD : Type_UW;
        if (!useAnyh && flagBits == 32 && flagBits > simdsize)
        {
            Ty = Type_UD;
        }
        G4_Declare* flagDecl = builder.createTempFlag((Ty == Type_UW ? 1 : 2), "cmpFlag");
        G4_RegVar* flagVar = flagDecl->getRegVar();
        G4_DstRegRegion* flag = builder.createDst(flagVar, 0, 0, 1, Ty);
        G4_INST* I0 = builder.createMov(1, flag,
            builder.createImm(0, Ty), InstOpt_WriteEnable, false);
        BB->insert(II, I0);

        G4_SrcRegRegion* r0_0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct,
            builder.getRealR0()->getRegVar(), 0, 0,
            builder.getRegionScalar(), Type_UW);
        G4_SrcRegRegion* r0_1 = builder.createSrcRegRegion(
            Mod_src_undef, Direct,
            builder.getRealR0()->getRegVar(), 0, 0,
            builder.getRegionScalar(), Type_UW);
        G4_CondMod* flagCM = builder.createCondMod(Mod_e, flagVar, 0);
        G4_DstRegRegion* nullDst = builder.createNullDst(Type_UW);
        G4_INST* I1 = builder.createInternalInst(
            NULL, G4_cmp, flagCM, false, simdsize, nullDst, r0_0, r0_1, InstOpt_M0);
        BB->insert(II, I1);

        if (useAnyh)
        {
            flagDefInst = I1;
            return flagVar;
        }

        G4_Imm* allone = builder.createImm(0xFFFFFFFF, Ty);
        G4_DstRegRegion* tFlag = builder.createDst(flagVar, 0, 0, 1, Ty);
        flagDefInst = builder.createMov(1, tFlag, allone, InstOpt_WriteEnable, false);
        G4_Predicate* tP = builder.createPredicate(
            PredState_Plus, flagVar, 0,
            (simdsize == 8 ? PRED_ANY8H : (simdsize == 16 ? PRED_ANY16H : PRED_ANY32H)));
        flagDefInst->setPredicate(tP);
        tP->setSameAsNoMask(true);
        BB->insert(II, flagDefInst);

        // update DefUse
        flagDefInst->addDefUse(I0, Opnd_pred);
        flagDefInst->addDefUse(I1, Opnd_pred);
        return flagVar;
    };

    // flagVar : emask flag for this BB:
    // currII:  iter to I
    //   positive predicate:
    //        I :  (W&P) <inst> (8|M0) ...
    //      to:
    //        I0:  (W&flagVar) sel (1|M0) tP P  0
    //        I :  (W&tP) <inst> (8|M0) ...
    //
    //   negative predicate:
    //        I :  (W&-P) <inst> (8|M0) ...
    //      to:
    //        I0:  (W&flagVar) sel  (1|M0) tP  P  0xFF
    //        I :  (W&-tP) <inst> (8|M0) ...
    //
    // where the predCtrl of tP at 'I' shall be the same as the original
    // predCtrl /(anyh, anyv, etc) of P at 'I'
    //
    auto doPredicateInstWA = [&](
        G4_INST* flagVarDefInst,  // inst that defines flagVar
        G4_RegVar* flagVar,
        G4_BB* currBB,
        INST_LIST_ITER& currII) -> void
    {
        G4_INST* I = *currII;
        G4_Predicate* P = I->getPredicate();
        assert((P && !I->getCondMod()) && "ICE: expect predicate and no flagModifier!");

        uint32_t flagBits = P->getRightBound() + 1;
        assert((16 * flagVar->getDeclare()->getRootDeclare()->getWordSize()) >= flagBits &&
            "ICE[vISA]: WA's flagVar should not be smaller!");

        G4_Type Ty = (flagBits > 16) ? Type_UD : Type_UW;
        G4_Declare* tPDecl = builder.createTempFlag(
            (Ty == Type_UD) ? 2 : 1, "tFlag");
        G4_RegVar* tPVar = tPDecl->getRegVar();
        G4_SrcRegRegion* Src0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, P->getTopDcl()->getRegVar(),
            0, 0, builder.getRegionScalar(), Ty);
        G4_Imm* Src1;
        if (P->getState() == PredState_Plus) {
            Src1 = builder.createImm(0, Ty);
        }
        else {
            Src1 = builder.createImm(I->getExecLaneMask(), Ty);
        }
        G4_DstRegRegion* tDst = builder.createDst(tPVar, 0, 0, 1, Ty);
        G4_Predicate* flag0 = builder.createPredicate(
            PredState_Plus, flagVar, 0, getPredCtrl(useAnyh));
        G4_INST* I0 = builder.createInternalInst(
            flag0, G4_sel, nullptr, false,
            1, tDst, Src0, Src1, InstOpt_WriteEnable);
        currBB->insert(currII, I0);
        flag0->setSameAsNoMask(true);

        flagVarDefInst->addDefUse(I0, Opnd_pred);
        if (!fg.globalOpndHT.isOpndGlobal(P)) {
            I->transferDef(I0, Opnd_pred, Opnd_src0);
        }

        G4_Predicate* tP = builder.createPredicate(P->getState(), tPVar, 0, P->getControl());
        I->setPredicate(tP);

        // update defUse
        I0->addDefUse(I, Opnd_pred);
    };

    // flagVar : emask for this BB
    //    Note that sel does not update flag register. When condMod is
    //    used, predicate is not allowed.
    // Before:
    //     I:  (W) sel.ge.f0.0  (1|M0)   r10.0<1>:f  r20.0<0;1,0>:f  0:f
    // After
    //     I:  (W) sel.ge.f0.0  (1|M0)  t:f  r20.0<0;1,0>:f   0:f
    //     I0: (W&flagVar) mov (1|M0)  r10.0<1>:f t:f
    //
    auto doFlagModifierSelInstWA = [&](
        G4_INST* flagVarDefInst,  // inst that defines flagVar
        G4_RegVar* flagVar,
        G4_BB* currBB,
        INST_LIST_ITER& currII) -> void
    {
        G4_INST* I = *currII;
        G4_CondMod* P = I->getCondMod();
        assert((P && !I->getPredicate()) && "ICE [sel]: expect flagModifier and no predicate!");

        G4_DstRegRegion* dst = I->getDst();
        assert((dst && !dst->isNullReg()) && "ICE: expect dst to be non-null!");

        // Create a temp that's big enough to hold data and possible gap
        // b/w data due to alignment/hw restriction.
        G4_Declare* saveDecl = builder.createTempVar(
            I->getExecSize() * dst->getHorzStride(), dst->getType(), Any, "saveTmp");

        G4_DstRegRegion* tDst = builder.createDst(
            saveDecl->getRegVar(), 0, 0, dst->getHorzStride(), dst->getType());
        I->setDest(tDst);

        const RegionDesc* regionSave;
        if (I->getExecSize() == 1) {
            regionSave = builder.getRegionScalar();
        }
        else {
            switch (dst->getHorzStride())
            {
            case 1: regionSave = builder.getRegionStride1(); break;
            case 2: regionSave = builder.getRegionStride2(); break;
            case 4: regionSave = builder.getRegionStride4(); break;
            default:
                assert(false && "ICE: unsupported dst horz stride!");
            }
        }

        G4_SrcRegRegion* tSrc = builder.createSrcRegRegion(
            Mod_src_undef, Direct, saveDecl->getRegVar(), 0, 0, regionSave, dst->getType());
        G4_INST* I0 = builder.createMov(
            I->getExecSize(), dst, tSrc, InstOpt_WriteEnable, false);
        G4_Predicate* flag0 = builder.createPredicate(
            PredState_Plus, flagVar, 0, getPredCtrl(useAnyh));
        I0->setPredicate(flag0);
        flag0->setSameAsNoMask(true);

        auto nextII = currII;
        ++nextII;
        currBB->insert(nextII, I0);

        flagVarDefInst->addDefUse(I0, Opnd_pred);
        // As I's dst must be global (otherwise, WA is not needed),
        // no need to do "I->transferUse(I0)".
        I->addDefUse(I0, Opnd_src0);
    };

    //  Non-predicated inst with flagModifier.
    //    flagVar : emask for this BB.
    //    Note that if 32-bit flag is used, flagVar and this instruction I's condMod
    //    take two flag registers, leaving no flag for temporary. In this case, we
    //    will do manual spill, ie,  save and restore the original flag (case 1 and 3).
    //
    //    Before:
    //       I:  (W)  cmp (16|M16) (ne)P  D ....   // 32-bit flag
    //         or
    //           (W)  cmp (16|M0)  (ne)P  D ....   // 16-bit flag
    //
    //    After:
    //      (1) D = null (common)
    //           I0: (W)           mov (1|M0) save:ud  P<0;1,0>:ud
    //           I:  (W)           cmp (16|M16) (ne)P  ....
    //           I1: (W&-flagVar)  mov (1|M0)  P   save:ud
    //      (2) 'I' uses 16-bit flag (common)
    //           I0: (W)           mov  (1)  nP<1>:uw    flagVar.0<0;1,0>:uw
    //           I:  (W&nP)        cmp (16|M0) (ne)nP  ....
    //           I1: (W&flagVar)   mov (1|M0)  P<1>:uw  nP<0;1,0>:uw
    //      (3) otherwise(less common)
    //           I0: (W)           mov (1|M0) save:ud  P<0;1,0>:ud
    //           I1: (W)           mov (16|M16) (nz)P  null flagVar
    //           I:  (W&P)         cmp (16|M16) (ne)P  D ...
    //           I2: (W&-flagVar)  mov (1|M0)  P   save:ud
    //
    auto doFlagModifierInstWA = [&](
        G4_INST* flagVarDefInst,  // inst that defines flagVar
        G4_RegVar* flagVar,
        G4_BB* currBB,
        INST_LIST_ITER& currII) -> void
    {
        G4_INST* I = *currII;
        G4_CondMod* P = I->getCondMod();
        assert((P && !I->getPredicate()) && "ICE: expect flagModifier and no predicate!");

        if (I->opcode() == G4_sel)
        {
            // Special handling of sel inst
            doFlagModifierSelInstWA(flagVarDefInst, flagVar, currBB, currII);
            return;
        }

        bool condModGlb = fg.globalOpndHT.isOpndGlobal(P);
        G4_Declare* modDcl = P->getTopDcl();
        G4_Type Ty = (modDcl->getWordSize() > 1) ? Type_UD : Type_UW;
        if (I->hasNULLDst())
        {   // case 1
            G4_Declare* saveDecl = builder.createTempVar(1, Ty, Any, "saveTmp");
            G4_RegVar* saveVar = saveDecl->getRegVar();
            G4_SrcRegRegion* I0S0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, modDcl->getRegVar(),
                0, 0, builder.getRegionScalar(), Ty);
            G4_DstRegRegion* D0 = builder.createDst(saveVar, 0, 0, 1, Ty);
            G4_INST* I0 = builder.createMov(1, D0, I0S0, InstOpt_WriteEnable, false);
            currBB->insert(currII, I0);

            auto nextII = currII;
            ++nextII;
            G4_SrcRegRegion* I1S0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, saveVar,
                0, 0, builder.getRegionScalar(), Ty);
            G4_DstRegRegion* D1 = builder.createDst(
                modDcl->getRegVar(), 0, 0, 1, Ty);
            G4_INST* I1 = builder.createMov(1, D1, I1S0, InstOpt_WriteEnable, false);
            G4_Predicate* flag = builder.createPredicate(
                PredState_Minus, flagVar, 0, getPredCtrl(useAnyh));
            I1->setPredicate(flag);
            currBB->insert(nextII, I1);

            flagVarDefInst->addDefUse(I1, Opnd_pred);
            I0->addDefUse(I1, Opnd_src0);

            if (!condModGlb)
            {
                // Copy condMod uses to I1.
                I->copyUsesTo(I1, false);
            }
            return;
        }

        if (I->getExecSize() == 16 && Ty == Type_UW)
        {   // case 2
            G4_Declare* nPDecl = builder.createTempFlag(1, "nP");
            G4_RegVar* nPVar = nPDecl->getRegVar();
            G4_SrcRegRegion* I0S0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, flagVar,
                0, 0, builder.getRegionScalar(), Ty);
            G4_DstRegRegion* D0 = builder.createDst(nPVar, 0, 0, 1, Ty);
            G4_INST* I0 = builder.createMov(1, D0, I0S0, InstOpt_WriteEnable, false);
            currBB->insert(currII, I0);

            // Use the new flag
            // Note that if useAny is true, nP should use anyh
            G4_Predicate* nP = builder.createPredicate(
                PredState_Plus, nPVar, 0, getPredCtrl(useAnyh));
            G4_CondMod* nM = builder.createCondMod(P->getMod(), nPVar, 0);
            I->setPredicate(nP);
            nP->setSameAsNoMask(true);
            I->setCondMod(nM);

            G4_SrcRegRegion* I1S0 = builder.createSrcRegRegion(
                Mod_src_undef, Direct, nPVar,
                0, 0, builder.getRegionScalar(), Ty);
            G4_DstRegRegion* D1 = builder.createDst(
                modDcl->getRegVar(), 0, 0, 1, Ty);
            G4_Predicate* flag1 = builder.createPredicate(
                PredState_Plus, flagVar, 0, getPredCtrl(useAnyh));
            G4_INST* I1 = builder.createMov(1, D1, I1S0, InstOpt_WriteEnable, false);
            I1->setPredicate(flag1);
            flag1->setSameAsNoMask(true);

            auto nextII = currII;
            ++nextII;
            currBB->insert(nextII, I1);

            flagVarDefInst->addDefUse(I0, Opnd_src0);
            flagVarDefInst->addDefUse(I1, Opnd_pred);
            I0->addDefUse(I, Opnd_pred);
            I->addDefUse(I1, Opnd_src0);

            if (!condModGlb)
            {
                // Need to transfer condMod uses to I1 only. Here, use
                // copyUsesTo() to achieve that, which is conservative.
                I->copyUsesTo(I1, false);
            }
            return;
        }

        // case 3
        G4_Declare* saveDecl = builder.createTempVar(1, Ty, Any, "saveTmp");
        G4_RegVar* saveVar = saveDecl->getRegVar();
        G4_SrcRegRegion* I0S0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, modDcl->getRegVar(),
            0, 0, builder.getRegionScalar(), Ty);
        G4_DstRegRegion* D0 = builder.createDst(
            saveVar, 0, 0, 1, Ty);
        G4_INST* I0 = builder.createMov(1, D0, I0S0, InstOpt_WriteEnable, false);
        currBB->insert(currII, I0);

        G4_DstRegRegion* D1 = builder.createNullDst(Ty);
        G4_SrcRegRegion* I1S0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, flagVar,
            0, 0, builder.getRegionScalar(), Ty);
        G4_INST* I1 = builder.createMov(I->getExecSize(),
            D1, I1S0, (InstOpt_WriteEnable | I->getMaskOption()), false);
        G4_CondMod* nM0 = builder.createCondMod(Mod_nz, modDcl->getRegVar(), 0);
        I1->setCondMod(nM0);
        currBB->insert(currII, I1);

        G4_Predicate* nP = builder.createPredicate(
            PredState_Plus, modDcl->getRegVar(), 0, PRED_DEFAULT);
        I->setPredicate(nP);
        // keep I's condMod unchanged

        auto nextII = currII;
        ++nextII;
        G4_SrcRegRegion* I2S0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, saveVar,
            0, 0, builder.getRegionScalar(), Ty);
        G4_DstRegRegion* D2 = builder.createDst(
            modDcl->getRegVar(), 0, 0, 1, Ty);
        G4_INST* I2 = builder.createMov(1, D2, I2S0, InstOpt_WriteEnable, false);
        G4_Predicate* flag2 = builder.createPredicate(
            PredState_Minus, flagVar, 0, getPredCtrl(useAnyh));
        I2->setPredicate(flag2);
        currBB->insert(nextII, I2);

        flagVarDefInst->addDefUse(I1, Opnd_src0);
        flagVarDefInst->addDefUse(I2, Opnd_pred);
        I0->addDefUse(I2, Opnd_src0);

        if (!condModGlb)
        {
            I1->addDefUse(I, Opnd_pred);
            // Need to copy uses of I's condMod to I2 only, here
            // conservatively use copyUsesTo().
            I->copyUsesTo(I2, false);
        }
    };

    //  Predicated inst with flagModifier.
    //  flagVar : emask for this BB:
    //
    //    Before:
    //       I:  (W&[-]P)  and (16|M0) (ne)P  ....
    //
    //    After:
    //       I0: (W)           mov (1|M0) save:uw  P
    //       I1: (W&-flagVar)  mov (1|M0) P<1>:uw  0:uw | 0xFFFF (for -P)
    //       I:  (W&P)         and (16|M0) (ne)P  ....
    //       I2: (W&-flagVar)  mov (1|M0)  P   save:uw
    //
    auto doPredicateAndFlagModifierInstWA = [&](
        G4_INST* flagVarDefInst,  // inst that defines flagVar
        G4_RegVar* flagVar,
        G4_BB* currBB,
        INST_LIST_ITER& currII) -> void
    {
        G4_INST* I = *currII;
        G4_Predicate* P = I->getPredicate();
        G4_CondMod* M = I->getCondMod();
        assert((P && M) && "ICE: expect both predicate and flagModifier!");
        assert(P->getTopDcl() == M->getTopDcl() &&
            "ICE: both predicate and flagMod must be the same flag!");

        bool condModGlb = fg.globalOpndHT.isOpndGlobal(M);

        G4_Declare* modDcl = M->getTopDcl();
        G4_Type Ty = (modDcl->getWordSize() > 1) ? Type_UD : Type_UW;

        G4_Declare* saveDecl = builder.createTempVar(1, Ty, Any, "saveTmp");
        G4_RegVar* saveVar = saveDecl->getRegVar();
        G4_SrcRegRegion* I0S0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, modDcl->getRegVar(),
            0, 0, builder.getRegionScalar(), Ty);
        G4_DstRegRegion* D0 = builder.createDst(
            saveVar, 0, 0, 1, Ty);
        G4_INST* I0 = builder.createMov(1, D0, I0S0, InstOpt_WriteEnable, false);
        currBB->insert(currII, I0);

        G4_DstRegRegion* D1 = builder.createDst(
            modDcl->getRegVar(), 0, 0, 1, Ty);
        G4_Imm* immS0;
        if (P->getState() == PredState_Plus) {
            immS0 = builder.createImm(0, Ty);
        }
        else {
            immS0 = builder.createImm(I->getExecLaneMask(), Ty);
        }
        G4_INST* I1 = builder.createMov(1, D1, immS0, InstOpt_WriteEnable, false);
        G4_Predicate* flag1 = builder.createPredicate(
            PredState_Minus, flagVar, 0, getPredCtrl(useAnyh));
        I1->setPredicate(flag1);
        currBB->insert(currII, I1);

        // No change to I

        auto nextII = currII;
        ++nextII;
        G4_SrcRegRegion* I2S0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, saveVar,
            0, 0, builder.getRegionScalar(), Ty);
        G4_DstRegRegion* D2 = builder.createDst(
            modDcl->getRegVar(), 0, 0, 1, Ty);
        G4_INST* I2 = builder.createMov(1, D2, I2S0, InstOpt_WriteEnable, false);
        G4_Predicate* flag2 = builder.createPredicate(
            PredState_Minus, flagVar, 0, getPredCtrl(useAnyh));
        I2->setPredicate(flag2);
        currBB->insert(nextII, I2);

        flagVarDefInst->addDefUse(I1, Opnd_pred);
        flagVarDefInst->addDefUse(I2, Opnd_pred);
        I0->addDefUse(I2, Opnd_src0);

        if (!condModGlb)
        {
            I1->addDefUse(I, Opnd_pred);
            // Need to copy uses of I's condMod to I2 only, here
            // conservatively use copyUsesTo().
            I->copyUsesTo(I2, false);
        }
    };

    // Scan all insts and apply WA on NoMask candidate insts
    for (auto BI : fg)
    {
        G4_BB* BB = BI;
        if (nestedDivergentBBs.count(BB) == 0)
        {
            continue;
        }
        if (((builder.getuint32Option(vISA_noMaskWA) & 0x3) >= 2) &&
            nestedDivergentBBs[BB] < 2)
        {
            continue;
        }

        if ((builder.getuint32Option(vISA_noMaskWA) & 0x4) != 0)
        {
            // simple flag insertion: every time a flag is needed, calcalate it.
            for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
            {
                G4_INST* I = *II;
                if (!isCandidateInst(I, fg))
                {
                    continue;
                }

                uint32_t flagbits = 16;
                if ((I->getExecSize() + I->getMaskOffset()) > 16)
                {
                    flagbits = 32;
                }
                useAnyh = enableAnyh;
                if (enableAnyh)
                {
                    if (I->getExecSize() > simdsize || I->getMaskOffset() != 0)
                    {
                        useAnyh = false;
                    }
                }

                G4_INST* flagDefInst = nullptr;
                G4_RegVar* flagVar = createFlagFromCmp(flagDefInst, flagbits, BB, II);

                G4_Predicate* pred = I->getPredicate();
                G4_CondMod* condmod = I->getCondMod();
                if (!condmod && !pred)
                {
                    // case 1: no predicate, no flagModifier (common case)
                    G4_Predicate* newPred = builder.createPredicate(
                        PredState_Plus, flagVar, 0, getPredCtrl(useAnyh));
                    newPred->setSameAsNoMask(true);
                    I->setPredicate(newPred);

                    // update defUse
                    flagDefInst->addDefUse(I, Opnd_pred);
                }
                else if (pred && !condmod)
                {
                    // case 2: has predicate, no flagModifier
                    doPredicateInstWA(flagDefInst, flagVar, BB, II);
                }
                else if (!pred && condmod)
                {
                    // case 3: has flagModifier, no predicate
                    doFlagModifierInstWA(flagDefInst, flagVar, BB, II);
                }
                else
                {
                    // case 4: both predicate and flagModifier are present
                    //         (rare or never happen)
                    doPredicateAndFlagModifierInstWA(flagDefInst, flagVar, BB, II);
                }
            }
            continue;
        }

        // For each BB that needs to modify NoMask, create a flag once right
        // prior to the first NoMask inst to be modified like the following:
        //
        //    BB:
        //      Before:
        //             (W&f0.0) inst0 (16|M0) ...
        //             ......
        //             (W&f0.0) inst0 (16|M0) ...
        //      After:
        //         (W) mov (1|M0)           f0.0:uw   0:uw  // or ud
        //             cmp (16|M0) (eq)f0.0 null:uw  r0.0<0;1,0::uw  r0.0<0;1,0>:uw
        //         if (!useAnyh)
        //             (W&f0.0.any16h)  mov (1|M0) f0.0  0xFFFF:uw
        //
        //             (W&f0.0) inst0 (16|M0) ...
        //             ......
        //             (W&f0.0) inst0 (16|M0) ...
        //         else
        //             (W&f0.0.any16h) inst0 (16|M0) ...
        //             ......
        //             (W&f0.0.any16h) inst0 (16|M0) ...
        //

        //  1. Collect all candidates and check if 32 bit flag is needed
        //     and if useAnyh can be set to true.
        bool need32BitFlag = false;
        useAnyh = enableAnyh; // need to reset for each BB
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
        {
            G4_INST* I = *II;
            if (isCandidateInst(I, fg))
            {
                NoMaskCandidates.push_back(II);
                if ((I->getExecSize() + I->getMaskOffset()) > 16)
                {
                    need32BitFlag = true;
                }
                if (enableAnyh)
                {
                    if (I->getExecSize() > simdsize || I->getMaskOffset() != 0)
                    {
                        useAnyh = false;
                    }
                }
            }
        }
        if (NoMaskCandidates.empty())
        {
            continue;
        }

        // 2. Do initialization for per-BB flag.
        INST_LIST_ITER& II0 = NoMaskCandidates[0];

        G4_INST* flagDefInst = nullptr;
        uint32_t flagBits = need32BitFlag ? 32 : 16;
        G4_RegVar* flagVarForBB = createFlagFromCmp(flagDefInst, flagBits, BB, II0);

        // 3. Do WA by adding predicate to each candidate
        for (int i = 0, sz = (int)NoMaskCandidates.size(); i < sz; ++i)
        {
            INST_LIST_ITER& II = NoMaskCandidates[i];
            G4_INST* I = *II;

            G4_CondMod* condmod = I->getCondMod();
            G4_Predicate* pred = I->getPredicate();
            if (!condmod && !pred)
            {
                // case 1: no predicate, no flagModifier (common case)
                G4_Predicate* newPred = builder.createPredicate(
                    PredState_Plus, flagVarForBB, 0, getPredCtrl(useAnyh));
                newPred->setSameAsNoMask(true);
                I->setPredicate(newPred);

                // update defUse
                flagDefInst->addDefUse(I, Opnd_pred);
            }
            else if (pred && !condmod)
            {
                // case 2: has predicate, no flagModifier
                doPredicateInstWA(flagDefInst, flagVarForBB, BB, II);
            }
            else if (!pred && condmod)
            {
                // case 3: has flagModifier, no predicate
                doFlagModifierInstWA(flagDefInst, flagVarForBB, BB, II);
            }
            else
            {
                // case 4: both predicate and flagModifier are present
                //         (rare or never happen)
                doPredicateAndFlagModifierInstWA(flagDefInst, flagVarForBB, BB, II);
            }
        }
        // Clear it to prepare for the next BB
        NoMaskCandidates.clear();
    }
}

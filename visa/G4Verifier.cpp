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

#include "G4Verifier.h"
using namespace vISA;

void verifyG4Kernel(G4_Kernel &k, Optimizer::PassIndex index, bool alwaysOn, G4Verifier::VerifyControl ctrl)
{
    if (alwaysOn || k.fg.builder->getOption(VISA_FullIRVerify))
    {
        G4Verifier verifier(k, ctrl, index);
        verifier.verify();
    }
}

void verifyG4Inst(G4_Kernel &kernel, G4_INST *inst, Optimizer::PassIndex index)
{
    G4Verifier verifier(kernel, G4Verifier::VC_ASSERT, index);
    verifier.verifyInst(inst);
}

std::atomic<int> G4Verifier::index(0);

G4Verifier::G4Verifier(G4_Kernel &k, VerifyControl ctrl, Optimizer::PassIndex index)
    : kernel(k), verifyCtrl(ctrl), passIndex(index)
{
    if (ctrl == VC_AppendDump || ctrl == VC_NewDump)
    {
        const char* buf = nullptr;
        k.getOptions()->getOption(VISA_AsmFileName, buf);
        std::string dumpName;
        if (buf != nullptr)
        {
            dumpName = std::string(buf);
        }
        dumpName += ".g4verify.dump.txt";
        if (ctrl == VC_AppendDump)
            dumpText.open(dumpName, std::ofstream::app);
        else
            dumpText.open(dumpName, std::ofstream::trunc);
    }
}

void G4Verifier::verify()
{
    // For each instruction do verification.
    for (auto BBI = kernel.fg.cbegin(), BBE = kernel.fg.cend(); BBI != BBE; ++BBI)
    {
       auto bb = *BBI;
       for (auto I = bb->begin(), E = bb->end(); I != E; ++I)
       {
           G4_INST *inst = *I;
           verifyInst(inst);
       }
    }
}

bool G4Verifier::verifyInst(G4_INST *inst)
{
    ASSERT_USER(inst != NULL, "null instruction upexpected");
    verifyOpcode(inst);
    verifyOpnd(inst->getDst(), inst);
    verifyOpnd(inst->getSrc(0), inst);
    verifyOpnd(inst->getSrc(1), inst);
    verifyOpnd(inst->getSrc(2), inst);
    verifyOpnd(inst->getPredicate(), inst);
    verifyOpnd(inst->getCondMod(), inst);
    verifyOpnd(inst->getImplAccDst(), inst);
    verifyOpnd(inst->getImplAccSrc(), inst);

    if (inst->isSend())
    {
        verifySend(inst);
    }

    verifyDstSrcOverlap(inst);

    if (passIndex == Optimizer::PI_cleanMessageHeader ||
        passIndex == Optimizer::PI_renameRegister ||
        passIndex == Optimizer::PI_newLocalDefHoisting ||
        passIndex == Optimizer::PI_newLocalCopyPropagation ||
        passIndex == Optimizer::PI_cselPeepHoleOpt)
    {
        // def-use chain should be valid after these passes
        return verifyDefUseChain(inst);
    }
    return true;
}

// Returns true if this use is defined by the defInst (dst, condMod, or acc)
// Otherwise returns false.
static bool checkDefUse(G4_INST* defInst, G4_Operand *use)
{
    if (!use)
        return false;

    G4_Operand *dst = defInst->getOperand(Opnd_dst);
    G4_Operand *condMod = defInst->getOperand(Opnd_condMod);

    if (use->isAccReg())
    {
        // use is acc
        // ToDo: we should check if acc is re-defined in between as well
        if (defInst->getImplAccDst() != NULL || dst->isAccReg())
        {
            return true;
        }
    }

    if (dst && Rel_disjoint != use->compareOperand(dst))
        return true;

    if (condMod && Rel_disjoint != use->compareOperand(condMod))
        return true;

    return false;
}

bool G4Verifier::verifyDefUseChain(G4_INST *inst)
{
    bool isValid = true;

    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto DU = *I;
        // A valid def-use satisfies
        //
        // inst[dst/condMod] defines DU.first[DU.second]
        //
        G4_Operand *use = (DU.first)->getOperand(DU.second);
        if (!checkDefUse(inst, use))
        {
            isValid = false;
            printDefUse(inst, DU.first, DU.second);
            assertIfEnable();
        }
    }

    for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I)
    {
        auto UD = *I;
        // A valid use-def satisfies
        //
        // UD.first[dst/condMod] defines inst[UD.second]
        //
        G4_Operand *use = inst->getOperand(UD.second);
        if (!checkDefUse(UD.first, use))
        {
            isValid = false;
            printDefUse(UD.first, inst, UD.second);
            assertIfEnable();
        }
    }

    return isValid;
}

void G4Verifier::printDefUseImpl(std::ostream &os, G4_INST *def, G4_INST *use,
                                 Gen4_Operand_Number pos)
{
    os << "\n  def: ";
    def->emit(os);
    os << "\n user: ";
    use->emit(os);
    os << "\n opnd: ";
    use->getOperand(pos)->emit(os);
}

/// Dump or warn def-use.
void G4Verifier::printDefUse(G4_INST *def, G4_INST *use, Gen4_Operand_Number pos)
{
    if (dumpText.is_open() && dumpText.good())
    {
        dumpText << "\n\nIndex: " << index++;
        printDefUseImpl(dumpText, def, use, pos);
    }
    else if (verifyCtrl == VC_WARN)
    {
        std::cerr << "\n\nInvalid def-use pair detected!!\n";
        printDefUseImpl(std::cerr, def, use, pos);
    }
}

void G4Verifier::assertIfEnable() const
{
#ifdef _DEBUG
    if (verifyCtrl == VC_ASSERT)
    {
        int *ptr = 0;
        *ptr = 0;
    }
#endif
}

bool G4Verifier::dataHazardCheck(G4_Operand *dst, G4_Operand *src)
{
    G4_RegVar* dstVar = static_cast<G4_RegVar*>(dst->asDstRegRegion()->getBase());
    G4_RegVar* srcVar = static_cast<G4_RegVar*>(src->asSrcRegRegion()->getBase());
    if (!dstVar->isRegVar() || !dstVar->isGreg() || !srcVar->isRegVar() || !srcVar->isGreg())
    {
        return false;
    }

    int dstStart = dst->getLinearizedStart();
    int dstEnd = dst->getLinearizedEnd();
    int srcStart = src->getLinearizedStart();
    int srcEnd = src->getLinearizedEnd();

    if (dstEnd < srcStart ||
        srcEnd < dstStart)
    {
        return false;
    }

    int dstReg = dstStart / GENX_GRF_REG_SIZ;
    int dstRegNum = (dstEnd - dstStart + GENX_GRF_REG_SIZ) / GENX_GRF_REG_SIZ;
    int srcReg = srcStart / GENX_GRF_REG_SIZ;
    int srcRegNum = (srcEnd - srcStart + GENX_GRF_REG_SIZ) / GENX_GRF_REG_SIZ;
    int srcReg2 = -1;

    if (srcRegNum > 1)
    {
        srcReg2 = srcReg + 1;
    }

    if (dstRegNum >= 2 && srcRegNum == 1)
    {
        srcReg2 = srcReg;
    }

    if (dstReg == srcReg2)
    {
        return true;
    }

    return false;
}

void G4Verifier::verifyDstSrcOverlap(G4_INST* inst)
{
    if (passIndex == Optimizer::PI_regAlloc && kernel.fg.builder->avoidDstSrcOverlap())
    {
        G4_DstRegRegion* dst = inst->getDst();

        if (inst->isSend() || dst == NULL || dst->isNullReg() || inst->opcode() == G4_madm)
        {
            return;
        }

        if (!inst->isComprInst())
        {
            return;
        }

        int dstStart = dst->getLinearizedStart() / GENX_GRF_REG_SIZ;
        int dstEnd = dst->getLinearizedEnd() / GENX_GRF_REG_SIZ;

        for (int i = 0; i < inst->getNumSrc(); i++)
        {
            G4_Operand* src = inst->getSrc(i);
            if (src != NULL && !src->isNullReg() && src->getTopDcl() &&
                (src->getTopDcl()->getRegFile() == G4_GRF || src->getTopDcl()->getRegFile() == G4_INPUT))
            {
                bool noOverlap = dataHazardCheck(dst, src);

                int srcStart = src->getLinearizedStart() / GENX_GRF_REG_SIZ;
                int srcEnd = src->getLinearizedEnd() / GENX_GRF_REG_SIZ;
                if (dstEnd != dstStart ||
                    srcStart != srcEnd)  //Any operand is more than 2 GRF
                {
                    MUST_BE_TRUE(!noOverlap, "dst and src0 overlap");
                }
            }
        }
    }
}

void G4Verifier::verifySend(G4_INST* inst)
{
    MUST_BE_TRUE(inst->isSend(), "expect send inst");
    if (passIndex == Optimizer::PI_regAlloc)
    {
        G4_DstRegRegion* dst = inst->getDst();
        G4_SrcRegRegion* src0 = inst->getSrc(0)->asSrcRegRegion();
        G4_SrcRegRegion* src1 = inst->isSplitSend() ? inst->getSrc(1)->asSrcRegRegion() : nullptr;

        if (inst->isEOT() && kernel.fg.builder->hasEOTGRFBinding())
        {
            auto checkEOTSrc = [](G4_SrcRegRegion* src) {
                const unsigned int EOTStart = 112 * GENX_GRF_REG_SIZ;
                if (src->isNullReg())
                {
                    return true;
                }
                return src->getLinearizedStart() >= EOTStart;
            };
            if (kernel.getNumRegTotal() >= 128)
            {
                MUST_BE_TRUE(checkEOTSrc(src0), "src0 for EOT send is not in r112-r127");
                if (src1 != nullptr)
                {
                    MUST_BE_TRUE(checkEOTSrc(src1), "src1 for EOT sends is not in r112-r127");
                }
            }
        }

        if (inst->isSplitSend())
        {
            if (src0->getBase()->isGreg() && src1 && src1->getBase()->isGreg())
            {
                int src0Start = src0->getLinearizedStart() / GENX_GRF_REG_SIZ;
                int src0End = src0Start + inst->getMsgDesc()->MessageLength() - 1;
                int src1Start = src1->getLinearizedStart() / GENX_GRF_REG_SIZ;
                int src1End = src1Start + inst->getMsgDesc()->extMessageLength() - 1;
                bool noOverlap = src0End < src1Start ||
                    src1End < src0Start;
                MUST_BE_TRUE(noOverlap, "split send src0 and src1 overlap");
            }
        }

        if (kernel.fg.builder->WaDisableSendSrcDstOverlap())
        {
            if (!dst->isNullReg())
            {
                if (src0->getBase()->isGreg())
                {
                    bool noOverlap = dst->getLinearizedEnd() < src0->getLinearizedStart() ||
                        src0->getLinearizedEnd() < dst->getLinearizedStart();
                    MUST_BE_TRUE(noOverlap, "send dst and src0 overlap");
                }
                if (src1 && !src1->isNullReg())
                {
                    bool noOverlap = dst->getLinearizedEnd() < src1->getLinearizedStart() ||
                        src1->getLinearizedEnd() < dst->getLinearizedStart();
                    MUST_BE_TRUE(noOverlap, "split send dst and src1 overlap");
                }
            }
        }
    }
}


void G4Verifier::verifyOpnd(G4_Operand* opnd, G4_INST* inst)
{

    uint8_t execSize = inst->getExecSize();

    if (opnd == NULL)
    {
        return;
    }

    if (inst->opcode() == G4_sel && opnd->isCondMod())
    {
        // conditional modifier for sel is a don't care, so we can skip verification
        return;
    }

    // FIXME: If isImm() condition is removed then some assertions are hit.
    // This means somewhere in Jitter operand sharing is happening for
    // immediate type operands. This should be fixed.
    // For Imm, AddrExp, AddrExpList, Labels, hashtable lookup is
    // performed at creation time unline SrcRegion, DstRegion,
    // Predicate, CondMod. This means former type of operands
    // can be shared across instructions.
    if (opnd->getInst() != inst &&
        opnd->isLabel() == false &&
        opnd->isImm() == false &&
        opnd->isNullReg() == false &&
        opnd->isAddrExp() == false)
    {
        DEBUG_VERBOSE("Inst not set correctly for opnd ");

        std::cerr << "operand: ";
        opnd->emit(std::cerr);
        std::cerr << " in instruction:\n  ";
        inst->emit(std::cerr);
        std::cerr << "\n";

        if (opnd->getInst() == NULL)
        {
            DEBUG_VERBOSE("Inst set to NULL");
            MUST_BE_TRUE(false, "Inst pointer set to NULL in opnd");
        }
        else
        {
            opnd->getInst()->emit(std::cerr);
            std::cerr << "\n";
            DEBUG_VERBOSE("Inst ptr set to incorrect inst");
            MUST_BE_TRUE(false, "Inst pointer incorrectly set in opnd");
        }
        DEBUG_VERBOSE(std::endl);
    }

    if (inst->isSend())
    {
        // send dst/src may not be GRF-aligned before HW conformity,
        // so we only check their bound in RA
        if (passIndex != Optimizer::PI_regAlloc)
        {
            return;
        }

        if (opnd == inst->getDst())
        {
            if (opnd->isRightBoundSet() && !opnd->isNullReg())
            {
                unsigned int correctRB = ((inst->getMsgDesc()->ResponseLength() + opnd->asDstRegRegion()->getRegOff()) * G4_GRF_REG_NBYTES) - 1;

                if (inst->getMsgDesc()->isScratchRW() == false &&
                    inst->getMsgDesc()->isOwordLoad() &&
                    inst->getMsgDesc()->isValidFuncCtrl() &&
                    (inst->getMsgDesc()->getFuncCtrl() & 0x700) == 0)
                {
                    correctRB = opnd->getLeftBound() + 15;
                }
                else if (opnd->getTopDcl()->getByteSize() < G4_GRF_REG_NBYTES)
                {
                    correctRB = opnd->getLeftBound() + opnd->getTopDcl()->getByteSize() - 1;
                }

                G4_Declare* parentDcl = opnd->getBase()->asRegVar()->getDeclare();
                while (parentDcl != NULL)
                {
                    correctRB += parentDcl->getAliasOffset();
                    parentDcl = parentDcl->getAliasDeclare();
                }

                correctRB = std::min(correctRB, opnd->getTopDcl()->getByteSize() - 1);

                if (opnd->getRightBound() != correctRB)
                {
                    DEBUG_VERBOSE("Right bound mismatch for send inst dst. Orig rb = " <<
                        opnd->getRightBound() << ", correct rb = " << correctRB << std::endl);

                    inst->emit(std::cerr);
                    DEBUG_VERBOSE(std::endl);
                    MUST_BE_TRUE(false, "Right bound mismatch!");
                }
            }
        }
        else if (opnd == inst->getSrc(0) || opnd == inst->getSrc(1))
        {
            if (opnd->isRightBoundSet())
            {
                int msgLength = (opnd == inst->getSrc(0)) ? inst->getMsgDesc()->MessageLength() : inst->getMsgDesc()->extMessageLength();
                unsigned int numBytes = opnd->getTopDcl()->getByteSize();
                unsigned int correctRB = 0;
                if (numBytes < G4_GRF_REG_NBYTES)
                {
                    correctRB = opnd->asSrcRegRegion()->getRegOff() * G4_GRF_REG_NBYTES + numBytes - 1;
                }
                else
                {
                    correctRB = ((msgLength + opnd->asSrcRegRegion()->getRegOff()) * G4_GRF_REG_NBYTES) - 1;
                }

                G4_Declare* parentDcl = opnd->getBase()->asRegVar()->getDeclare();
                while (parentDcl != NULL)
                {
                    correctRB += parentDcl->getAliasOffset();
                    parentDcl = parentDcl->getAliasDeclare();
                }

                correctRB = std::min(correctRB, opnd->getTopDcl()->getByteSize() - 1);

                if (opnd->getRightBound() != correctRB)
                {
                    DEBUG_VERBOSE("Right bound mismatch for send inst src0. Orig rb = " <<
                        opnd->getRightBound() << ", correct rb = " << correctRB << std::endl);

                    inst->emit(std::cerr);
                    DEBUG_VERBOSE(std::endl);
                    MUST_BE_TRUE(false, "Right bound mismatch!");
                }
            }
        }
    }
    else
    {
        if (opnd->isSrcRegRegion() && opnd->isRightBoundSet())
        {
            G4_SrcRegRegion newRgn(*(opnd->asSrcRegRegion()));

            newRgn.setInst(inst);
            newRgn.computeLeftBound();
            newRgn.computeRightBound(execSize);

            if (inst->isPseudoUse())
            {
                G4_Declare* topdcl = newRgn.getBase()->asRegVar()->getDeclare();

                while (topdcl->getAliasDeclare() != NULL)
                {
                    topdcl = topdcl->getAliasDeclare();
                }

                newRgn.setLeftBound(0);
                newRgn.setRightBound(topdcl->getByteSize() - 1);
            }

            if ((opnd->getRightBound() - opnd->getLeftBound()) > (2u * G4_GRF_REG_NBYTES) &&
                (inst->isPseudoUse() == false))
            {
                if (!(inst->opcode() == G4_pln && inst->getSrc(1) == opnd))
                {
                    DEBUG_VERBOSE("Difference between left/right bound is greater than 2 GRF for src region. Single non-send opnd cannot span 2 GRFs. lb = " <<
                        opnd->getLeftBound() << ", rb = " << opnd->getRightBound() << std::endl);
                    inst->emit(std::cerr);
                    DEBUG_VERBOSE(std::endl);
                    MUST_BE_TRUE(false, "Left/right bound span incorrect!");
                }
            }

            if (inst->opcode() == G4_pln &&
                inst->getSrc(1) == opnd)
            {
                // For pln, src1 uses 2 GRFs if exec size <= 8
                // and 4 GRFs if exec size == 16
                newRgn.computeRightBound(inst->getExecSize() > 8 ? inst->getExecSize() : inst->getExecSize() * 2);

                if (inst->getExecSize() > 8)
                {
                    newRgn.setRightBound(newRgn.getRightBound() * 2 - newRgn.getLeftBound() + 1);
                }
            }

            if (inst->getMaskOffset() > 0 &&
                opnd == inst->getImplAccSrc())
            {
                // Update left/right bound as per inst mask offset, eg Q2
                // has offset 8
                G4_Type extype;
                int extypesize;
                unsigned int multiplicationFactor = 1;
                if (opnd->isAccReg())
                {
                    // Right bound granularity is in terms of
                    // bytes for Acc registers
                    multiplicationFactor = 4;
                }

                extype = inst->getOpExecType(extypesize);
                if ((IS_WTYPE(extype) || IS_DTYPE(extype)))
                {
                    // This condition is a result of HW Conformity requirement
                    // that for exec type = D/DW, only acc0 is used even when
                    // qtr control is set to Q2/H2
                    newRgn.setLeftBound(0);
                    newRgn.setRightBound(31);
                }
                else
                {
                    newRgn.setLeftBound(newRgn.getLeftBound() + (inst->getMaskOffset() * multiplicationFactor));
                    newRgn.setRightBound(newRgn.getRightBound() + (inst->getMaskOffset() * multiplicationFactor));
                }
            }

            if (opnd->getLeftBound() != newRgn.getLeftBound())
            {
                DEBUG_VERBOSE("Left bound mismatch for src opnd for following inst. Orig lb = " <<
                    opnd->getLeftBound() << ", recomputed lb = " << newRgn.getLeftBound() << std::endl);
                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Left bound mismatch!");
            }

            if (opnd->getRightBound() != newRgn.getRightBound())
            {
                DEBUG_VERBOSE("Right bound mismatch for src opnd for following inst. Orig rb = " <<
                    opnd->getRightBound() << ", recomputed rb = " << newRgn.getRightBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Right bound mismatch!");
            }
        }
        else if (opnd->isDstRegRegion() && opnd->isRightBoundSet() && !opnd->isNullReg())
        {
            G4_DstRegRegion newRgn(*(opnd->asDstRegRegion()));
            newRgn.setInst(inst);
            newRgn.computeLeftBound();
            newRgn.computeRightBound(execSize);

            if (inst->isPseudoKill())
            {
                G4_Declare* topdcl = newRgn.getBase()->asRegVar()->getDeclare();

                while (topdcl->getAliasDeclare() != NULL)
                {
                    topdcl = topdcl->getAliasDeclare();
                }

                newRgn.setLeftBound(0);
                newRgn.setRightBound(topdcl->getByteSize() - 1);
            }

            if ((opnd->getRightBound() - opnd->getLeftBound()) > (2u * G4_GRF_REG_NBYTES) &&
                (inst->isPseudoKill() == false))
            {
                DEBUG_VERBOSE("Difference between left/right bound is greater than 2 GRF for dst region. Single non-send opnd cannot span 2 GRFs. lb = " <<
                    opnd->getLeftBound() << ", rb = " << opnd->getRightBound() << std::endl);
                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Left/right bound span incorrect!");
            }

            if (inst->getMaskOffset() > 0 &&
                opnd == inst->getImplAccDst())
            {
                // Update left/right bound as per inst mask offset, eg Q2
                // has offset 8
                G4_Type extype;
                int extypesize;
                unsigned int multiplicationFactor = 1;
                if (opnd->isAccReg())
                {
                    // Right bound granularity is in terms of
                    // bytes for Acc registers
                    multiplicationFactor = 4;
                }

                extype = inst->getOpExecType(extypesize);

                if ((IS_WTYPE(extype) || IS_DTYPE(extype)))
                {
                    // This condition is a result of HW Conformity requirement
                    // that for exec type = D/DW, only acc0 is used even when
                    // qtr control is set to Q2/H2
                    newRgn.setLeftBound(0);
                    newRgn.setRightBound(31);
                }
                else
                {
                    newRgn.setLeftBound(newRgn.getLeftBound() + (inst->getMaskOffset() * multiplicationFactor));
                    newRgn.setRightBound(newRgn.getRightBound() + (inst->getMaskOffset() * multiplicationFactor));
                }
            }

            if (opnd->getLeftBound() != newRgn.getLeftBound())
            {
                DEBUG_VERBOSE("Left bound mismatch for dst opnd for following inst. Orig lb = " <<
                    opnd->getLeftBound() << ", recomputed lb = " << newRgn.getLeftBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Left bound mismatch");
            }

            if (opnd->getRightBound() != newRgn.getRightBound())
            {
                DEBUG_VERBOSE("Right bound mismatch for dst opnd for following inst. Orig rb = " <<
                    opnd->getRightBound() << ", recomputed rb = " << newRgn.getRightBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Right bound mismatch!");
            }
        }
        else if (opnd->isPredicate() && opnd->isRightBoundSet())
        {
            G4_Predicate newRgn(*(opnd->asPredicate()));

            newRgn.setLeftBound(0);
            newRgn.computeRightBound(execSize);

            if (inst->getMaskOffset() > 0)
            {
                // Update left/right bound as per inst mask offset, eg Q2
                // has offset 8
                newRgn.setLeftBound(newRgn.getLeftBound() + inst->getMaskOffset());
                newRgn.setRightBound(newRgn.getRightBound() + inst->getMaskOffset());
            }

            if (opnd->getLeftBound() != newRgn.getLeftBound())
            {
                DEBUG_VERBOSE("Left bound mismatch for pred opnd for following inst. Orig lb = " <<
                    opnd->getLeftBound() << ", recomputed lb = " << newRgn.getLeftBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Left bound mismatch");
            }

            if (opnd->getRightBound() != newRgn.getRightBound())
            {
                DEBUG_VERBOSE("Right bound mismatch for pred opnd for following inst. Orig rb = " <<
                    opnd->getRightBound() << ", recomputed rb = " << newRgn.getRightBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Right bound mismatch!");
            }
        }
        else if (opnd->isCondMod() && opnd->isRightBoundSet())
        {
            G4_CondMod newRgn(*(opnd->asCondMod()));

            newRgn.setLeftBound(0);
            newRgn.computeRightBound(execSize);

            if (inst->getMaskOffset() > 0)
            {
                // Update left/right bound as per inst mask offset, eg Q2
                // has offset 8
                newRgn.setLeftBound(newRgn.getLeftBound() + inst->getMaskOffset());
                newRgn.setRightBound(newRgn.getRightBound() + inst->getMaskOffset());
            }

            if (opnd->getLeftBound() != newRgn.getLeftBound())
            {
                DEBUG_VERBOSE("Left bound mismatch for cond mod opnd for following inst. Orig lb = " <<
                    opnd->getLeftBound() << ", recomputed lb = " << newRgn.getLeftBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Left bound mismatch");
            }

            if (opnd->getRightBound() != newRgn.getRightBound())
            {
                DEBUG_VERBOSE("Right bound mismatch for cond mod opnd for following inst. Orig rb = " <<
                    opnd->getRightBound() << ", recomputed rb = " << newRgn.getRightBound() << std::endl);

                inst->emit(std::cerr);
                DEBUG_VERBOSE(std::endl);
                MUST_BE_TRUE(false, "Right bound mismatch!");
            }
        }
        else
        {
            // Not implemented
        }

        if (passIndex == Optimizer::PI_regAlloc)
        {
            // alignment checks that can only be performed post RA
            bool threeSrcAlign16 = (inst->getNumSrc() == 3) && !inst->isSend() && !kernel.fg.builder->hasAlign1Ternary();
            bool nonScalar = (opnd->isSrcRegRegion() && !opnd->asSrcRegRegion()->isScalar()) ||
                (opnd->isDstRegRegion() && inst->getExecSize() > 2);
            bool isAssigned = opnd->isRegRegion() && opnd->getBase()->isRegVar() &&
                opnd->getBase()->asRegVar()->isPhyRegAssigned();
            // allow replicated DF source opnd with <2;2,0> region
            bool isReplicated = (opnd->getType() == Type_DF) &&
                opnd->isSrcRegRegion() &&
                (opnd->asSrcRegRegion()->getRegion()->width == 2) &&
                (opnd->asSrcRegRegion()->getRegion()->horzStride == 0) &&
                (opnd->asSrcRegRegion()->getRegion()->vertStride == 2);
            if (threeSrcAlign16 && nonScalar && isAssigned &&
                opnd->getLinearizedStart() % 16 != 0 &&
                !isReplicated)
            {
                MUST_BE_TRUE(false, "dp2/dp3/dp4/dph and non-scalar 3src op must be align16!");
            }

            // check acc source alignment
            // for explicit acc source, it and the inst's dst should both be oword-aligned
            // for implicit acc source, its subreg offset should be identical to that of the dst
            if (opnd->isAccReg())
            {
                uint32_t offset = opnd->getLinearizedStart() % 32;
                if (inst->getDst())
                {
                    uint32_t dstOffset = inst->getDst()->getLinearizedStart() % 32;
                    if (opnd == inst->getImplAccSrc())
                    {
                        assert(offset == dstOffset && "implicit acc source must have identical offset as dst");
                    }
                    else if (opnd->isSrcRegRegion())
                    {
                        assert((offset % 16 == 0 && dstOffset % 16 == 0) &&
                            "explicit acc source and its dst must be oword-aligned");
                    }
                }
            }
        }
    }
}

void verifyLifetimeConsistency(G4_BB* bb)
{
    // Verify whether misplaced pseudo_kill/lifetime.end is seen in BB
    // Following code patterns are incorrect:
    // mov (1) A,
    // ...
    // pseudo_kill A
    // As per VISA spec, we allow a single instance of pseudo_kill per
    // variable. Later RA's liveness may insert multiple. This will
    // not be invoked after RA anyway. As a precaution, we return
    // if no unassigned register is found.
    //
    // Similarly for lifetime.end
    // lifetime.end A
    // ...
    // mov (1) A,
    // This is illegal because lifetime.end appears before last use
    // in BB
    bool unassignedFound = false;

    for (INST_LIST_ITER it = bb->begin(), end = bb->end();
        it != end;
        it++)
    {
        G4_INST* curInst = (*it);

        std::stack<G4_Operand*> opnds;
        opnds.push(curInst->getDst());
        opnds.push(curInst->getSrc(0));
        opnds.push(curInst->getSrc(1));
        opnds.push(curInst->getSrc(2));
        opnds.push(curInst->getPredicate());
        opnds.push(curInst->getCondMod());

        while (!opnds.empty())
        {
            G4_Operand* curOpnd = opnds.top();
            opnds.pop();

            if (curOpnd != NULL && curOpnd->getTopDcl() != NULL)
            {
                G4_Declare* topdcl = curOpnd->getTopDcl();

                if (topdcl->getRegVar() &&
                    !topdcl->getRegVar()->isPhyRegAssigned())
                {
                    unassignedFound = true;
                }
            }
        }
    }

    if (unassignedFound == true)
    {
        typedef std::map<G4_Declare*, std::pair<G4_INST*, unsigned int>> dclInstMap;
        typedef dclInstMap::iterator dclInstMapIter;
        dclInstMap pseudoKills;
        dclInstMap lifetimeEnd;

        unsigned int instId = 0;

        // First populate all pseudo_kills and lifetime.end instructions
        // in BB's inst list. Later run second loop to check whether
        // lifetime rules are flouted.
        for (INST_LIST_ITER it = bb->begin(), end = bb->end();
            it != end;
            it++, instId++)
        {
            G4_INST* curInst = (*it);
            std::pair<G4_INST*, unsigned int> instPair;

            instPair.first = curInst;
            instPair.second = instId;

            if (curInst->isPseudoKill())
            {
                pseudoKills.insert(make_pair(GetTopDclFromRegRegion(curInst->getDst()), instPair));
            }

            if (curInst->isLifeTimeEnd())
            {
                lifetimeEnd.insert(make_pair(GetTopDclFromRegRegion(curInst->getSrc(0)), instPair));
            }
        }

        instId = 0;
        for (INST_LIST_ITER it = bb->begin(), end = bb->end();
            it != end;
            it++, instId++)
        {
            G4_INST* curInst = (*it);

            if (curInst->isPseudoKill() ||
                curInst->isLifeTimeEnd())
            {
                continue;
            }

            std::stack<G4_Operand*> opnds;
            opnds.push(curInst->getDst());
            opnds.push(curInst->getSrc(0));
            opnds.push(curInst->getSrc(1));
            opnds.push(curInst->getSrc(2));
            opnds.push(curInst->getPredicate());
            opnds.push(curInst->getCondMod());

            while (!opnds.empty())
            {
                G4_Operand* curOpnd = opnds.top();
                opnds.pop();

                if (curOpnd != NULL && curOpnd->getTopDcl() != NULL)
                {
                    G4_Declare* topdcl = curOpnd->getTopDcl();

                    // Check whether topdcl has been written to map
                    dclInstMapIter killsIt = pseudoKills.find(topdcl);

                    if (killsIt != pseudoKills.end())
                    {
                        unsigned int foundAtId = (*killsIt).second.second;

                        if (foundAtId > instId)
                        {
                            DEBUG_VERBOSE("Found a definition before pseudo_kill.");
                            (*killsIt).second.first->emit(std::cerr);
                            DEBUG_VERBOSE(std::endl);
                            curInst->emit(std::cerr);
                            DEBUG_VERBOSE(std::endl);
                        }
                    }

                    dclInstMapIter lifetimeEndIter = lifetimeEnd.find(topdcl);

                    if (lifetimeEndIter != lifetimeEnd.end())
                    {
                        unsigned int foundAtId = (*lifetimeEndIter).second.second;

                        if (foundAtId < instId)
                        {
                            DEBUG_VERBOSE("Found a use after lifetime.end.");
                            (*lifetimeEndIter).second.first->emit(std::cerr);
                            DEBUG_VERBOSE(std::endl);
                            curInst->emit(std::cerr);
                            DEBUG_VERBOSE(std::endl);
                        }
                    }
                }
            }
        }
    }
}

void G4Verifier::verifyOpcode(G4_INST* inst)
{
    switch (inst->opcode())
    {
    case G4_dp2:
    case G4_dp3:
    case G4_dp4:
        assert(kernel.fg.builder->hasDotProductInst() && "unsupported opcode");
        break;
    case G4_lrp:
        assert(kernel.fg.builder->hasLRP() && "unsupported opcode");
        break;
    case G4_madm:
        assert(kernel.fg.builder->hasMadm() && "unsupported opcode");
        break;
    default:
        break;
    }

    if (passIndex == Optimizer::PI_regAlloc)
    {
        //ToDo: add more checks for psuedo inst after RA
        assert(!inst->isPseudoLogic() && "pseudo logic inst should be lowered before RA");
    }

}


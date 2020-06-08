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

#include "BinaryEncodingIGA.h"
#include "GTGPU_RT_ASM_Interface.h"
#include "iga/IGALibrary/api/igaEncoderWrapper.hpp"
#include "Timer.h"
#include "BuildIR.h"

using namespace iga;
using namespace vISA;


Platform BinaryEncodingIGA::getIGAInternalPlatform(TARGET_PLATFORM genxPlatform)
{
    Platform platform = Platform::INVALID;
    switch (genxPlatform)
    {
    case GENX_BDW:
        platform = Platform::GEN8;
        break;
    case GENX_CHV:
        platform = Platform::GEN8;
        break;
    case GENX_SKL:
    case GENX_BXT:
        platform = Platform::GEN9;
        break;
    case GENX_ICLLP:
        platform = Platform::GEN11;
        break;
    case GENX_TGLLP:
        platform = Platform::GEN12P1;
        break;
    default:
        break;
    }

    return platform;
}

BinaryEncodingIGA::BinaryEncodingIGA(
    vISA::Mem_Manager &m,
    vISA::G4_Kernel& k,
    std::string fname)
    : mem(m), kernel(k), fileName(fname), m_kernelBuffer(nullptr),
    m_kernelBufferSize(0), platform(k.fg.builder->getPlatform())
{
    platformModel = iga::Model::LookupModel(getIGAInternalPlatform(getGenxPlatform()));
    IGAKernel = new iga::Kernel(*platformModel);
}

iga::InstOptSet BinaryEncodingIGA::getIGAInstOptSet(G4_INST* inst) const
{
    iga::InstOptSet options;

    if (inst->isAccWrCtrlInst() && kernel.fg.builder->encodeAccWrEn())
    {
        options.add(iga::InstOpt::ACCWREN);
    }
    if (inst->isAtomicInst())
    {
        options.add(iga::InstOpt::ATOMIC);
    }
    if (inst->isBreakPointInst())
    {
        options.add(iga::InstOpt::BREAKPOINT);
    }
    if (inst->isNoDDChkInst())
    {
        options.add(iga::InstOpt::NODDCHK);
    }
    if (inst->isNoDDClrInst())
    {
        options.add(iga::InstOpt::NODDCLR);
    }
    if (inst->isNoPreemptInst())
    {
        options.add(iga::InstOpt::NOPREEMPT);
    }
    if (inst->isYieldInst())
    {
        options.add(iga::InstOpt::SWITCH);
    }
    if (inst->isSend())
    {
        if (inst->isEOT())
        {
            options.add(iga::InstOpt::EOT);
        }
        if (inst->isNoSrcDepSet())
        {
            options.add(iga::InstOpt::NOSRCDEPSET);
        }
        if (inst->asSendInst()->isSerializedInst())
        {
            options.add(iga::InstOpt::SERIALIZE);
        }
    }
    if (inst->isNoCompactedInst())
    {
        options.add(iga::InstOpt::NOCOMPACT);
    }

    return options;
}


void BinaryEncodingIGA::FixInst()
{
    for (auto bb : kernel.fg)
    {
        for (auto iter = bb->begin(); iter != bb->end();)
        {
            G4_INST* inst = *iter;
            if (inst->isIntrinsic())
            {
                // WA for simulation:  remove any intrinsics that should be lowered before binary encoding
                MUST_BE_TRUE(inst->asIntrinsicInst()->getLoweredByPhase() == Phase::BinaryEncoding,
                    "Unexpected intrinsics in binary encoding");
                iter = bb->erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
}

iga::SFID BinaryEncodingIGA::getSFID(const G4_INST *inst)
{
    ASSERT_USER(inst->isSend(), "Only send has SFID");

    G4_SendMsgDescriptor *msgDesc = inst->getMsgDesc();
    auto funcID = msgDesc->getFuncId();

    iga::SFID sfid = iga::SFID::INVALID;
    switch (funcID)
    {
    case vISA::SFID::NULL_SFID: sfid = iga::SFID::NULL_; break;
    case vISA::SFID::SAMPLER:   sfid = iga::SFID::SMPL; break;
    case vISA::SFID::GATEWAY:   sfid = iga::SFID::GTWY; break;
    case vISA::SFID::DP_DC2:    sfid = iga::SFID::DC2; break;
    case vISA::SFID::DP_WRITE:  sfid = iga::SFID::RC; break;
    case vISA::SFID::URB:       sfid = iga::SFID::URB; break;
    case vISA::SFID::SPAWNER:   sfid = iga::SFID::TS; break;
    case vISA::SFID::VME:       sfid = iga::SFID::VME; break;
    case vISA::SFID::DP_CC:     sfid = iga::SFID::DCRO; break;
    case vISA::SFID::DP_DC:     sfid = iga::SFID::DC0; break;
    case vISA::SFID::DP_PI:     sfid = iga::SFID::PIXI; break;
    case vISA::SFID::DP_DC1:    sfid = iga::SFID::DC1; break;
    case vISA::SFID::CRE:       sfid = iga::SFID::CRE; break;
    default:
        ASSERT_USER(false, "Unknow SFID generated from vISA");
        break;
    }

    return sfid;
}
iga::MathFC BinaryEncodingIGA::getMathFC(const G4_INST *inst)
{
    G4_MathOp mathControlValue = inst->asMathInst()->getMathCtrl();

    switch (mathControlValue)
    {
    case MATH_INV:          return iga::MathFC::INV;
    case MATH_LOG:          return iga::MathFC::LOG;
    case MATH_EXP:          return iga::MathFC::EXP;
    case MATH_SQRT:         return iga::MathFC::SQT;
    case MATH_RSQ:          return iga::MathFC::RSQT;
    case MATH_SIN:          return iga::MathFC::SIN;
    case MATH_COS:          return iga::MathFC::COS;
    case MATH_FDIV:         return iga::MathFC::FDIV;
    case MATH_POW:          return iga::MathFC::POW;
    case MATH_INT_DIV:      return iga::MathFC::IDIV;
    case MATH_INT_DIV_QUOT: return iga::MathFC::IQOT;
    case MATH_INT_DIV_REM:  return iga::MathFC::IREM;
    case MATH_INVM:         return iga::MathFC::INVM;
    case MATH_RSQRTM:       return iga::MathFC::RSQTM;
    default:
        ASSERT_USER(false, "invalid math subfunction");
        return iga::MathFC::INVALID;
    }
}

std::pair<iga::Op,iga::Subfunction> BinaryEncodingIGA::getIgaOpInfo(
    G4_opcode op, const G4_INST *inst, iga::Platform p)
{
    iga::Op igaOp = iga::Op::INVALID;
    iga::Subfunction sf = iga::InvalidFC::INVALID;
    switch (op)
    {
    case G4_illegal: igaOp = iga::Op::ILLEGAL; break;
    case G4_mov:     igaOp = iga::Op::MOV; break;
    case G4_sel:     igaOp = iga::Op::SEL; break;
    case G4_movi:    igaOp = iga::Op::MOVI; break;
    case G4_not:     igaOp = iga::Op::NOT; break;
    case G4_and:     igaOp = iga::Op::AND; break;
    case G4_or:      igaOp = iga::Op::OR; break;
    case G4_xor:     igaOp = iga::Op::XOR; break;
    case G4_shr:     igaOp = iga::Op::SHR; break;
    case G4_shl:     igaOp = iga::Op::SHL; break;
    case G4_smov:    igaOp = iga::Op::SMOV; break;
    case G4_asr:     igaOp = iga::Op::ASR; break;
    case G4_ror:     igaOp = iga::Op::ROR; break;
    case G4_rol:     igaOp = iga::Op::ROL; break;
    case G4_cmp:     igaOp = iga::Op::CMP; break;
    case G4_cmpn:    igaOp = iga::Op::CMPN; break;
    case G4_csel:    igaOp = iga::Op::CSEL; break;
    case G4_bfrev:   igaOp = iga::Op::BFREV; break;
    case G4_bfe:     igaOp = iga::Op::BFE; break;
    case G4_bfi1:    igaOp = iga::Op::BFI1; break;
    case G4_bfi2:    igaOp = iga::Op::BFI2; break;
    case G4_jmpi:    igaOp = iga::Op::JMPI; break;
    case G4_brd:     igaOp = iga::Op::BRD; break;
    case G4_if:      igaOp = iga::Op::IF; break;
    case G4_brc:     igaOp = iga::Op::BRC; break;
    case G4_else:    igaOp = iga::Op::ELSE; break;
    case G4_endif:   igaOp = iga::Op::ENDIF; break;
    case G4_while:   igaOp = iga::Op::WHILE; break;
    case G4_break:   igaOp = iga::Op::BREAK; break;
    case G4_cont:    igaOp = iga::Op::CONT; break;
    case G4_halt:    igaOp = iga::Op::HALT; break;
    case G4_call:    igaOp = iga::Op::CALL; break;
    case G4_return:  igaOp = iga::Op::RET; break;
    case G4_goto:    igaOp = iga::Op::GOTO; break;
    case G4_join:    igaOp = iga::Op::JOIN; break;
    case G4_wait:
        if (p >= iga::Platform::GEN12P1)
        {
            igaOp = iga::Op::SYNC;
            sf = iga::SyncFC::BAR;
        }
        else
        {
            igaOp = iga::Op::WAIT;
        }
        break;
    case G4_send:
        igaOp = iga::Op::SEND;
        sf = getSFID(inst);
        break;
    case G4_sendc:
        igaOp = iga::Op::SENDC;
        sf = getSFID(inst);
        break;
    case G4_sends:
        sf = getSFID(inst);
        if (p >= iga::Platform::GEN12P1) {
            // G4 IR still calls send sends after GEN12
            igaOp = iga::Op::SEND;
        } else {
            igaOp = iga::Op::SENDS;
        }
        break;
    case G4_sendsc:
        sf = getSFID(inst);
        if (p >= iga::Platform::GEN12P1) {
            // G4 IR still calls send sends after GEN12
            igaOp = iga::Op::SENDC;
        } else {
            igaOp = iga::Op::SENDSC;
        }
        break;
    case G4_math:
        sf = getMathFC(inst);
        igaOp = iga::Op::MATH;
        break;
    case G4_add:     igaOp = iga::Op::ADD; break;
    case G4_mul:     igaOp = iga::Op::MUL; break;
    case G4_avg:     igaOp = iga::Op::AVG; break;
    case G4_frc:     igaOp = iga::Op::FRC; break;
    case G4_rndu:    igaOp = iga::Op::RNDU; break;
    case G4_rndd:    igaOp = iga::Op::RNDD; break;
    case G4_rnde:    igaOp = iga::Op::RNDE; break;
    case G4_rndz:    igaOp = iga::Op::RNDZ; break;
    case G4_mac:     igaOp = iga::Op::MAC; break;
    case G4_mach:
        igaOp = iga::Op::MACH;
        break;
    case G4_lzd:     igaOp = iga::Op::LZD; break;
    case G4_fbh:     igaOp = iga::Op::FBH; break;
    case G4_fbl:     igaOp = iga::Op::FBL; break;
    case G4_cbit:    igaOp = iga::Op::CBIT; break;
    case G4_addc:    igaOp = iga::Op::ADDC; break;
    case G4_subb:    igaOp = iga::Op::SUBB; break;
    case G4_sad2:    igaOp = iga::Op::SAD2; break;
    case G4_sada2:   igaOp = iga::Op::SADA2; break;
    case G4_dp4:     igaOp = iga::Op::DP4; break;
    case G4_dph:     igaOp = iga::Op::DPH; break;
    case G4_dp3:     igaOp = iga::Op::DP3; break;
    case G4_dp2:     igaOp = iga::Op::DP2; break;
    case G4_dp4a:    igaOp = iga::Op::DP4A; break;
    case G4_line:    igaOp = iga::Op::LINE; break;
    case G4_pln:     igaOp = iga::Op::PLN; break;
    case G4_mad:     igaOp = iga::Op::MAD; break;
    case G4_lrp:     igaOp = iga::Op::LRP; break;
    case G4_madm:    igaOp = iga::Op::MADM; break;
    case G4_nop:     igaOp = iga::Op::NOP; break;
    case G4_label:
        break;
    case G4_pseudo_mad: igaOp = iga::Op::MAD; break;
    case G4_do:
        ASSERT_USER(false, "G4_do is not GEN ISA OPCODE.");
        break;
    case G4_mulh:
        ASSERT_USER(false, "G4_mulh is not GEN ISA OPCODE.");
        break;
    case G4_pseudo_and:   igaOp = iga::Op::AND; break;
    case G4_pseudo_or:    igaOp = iga::Op::OR; break;
    case G4_pseudo_xor:   igaOp = iga::Op::XOR; break;
    case G4_pseudo_not:   igaOp = iga::Op::NOT; break;
    case G4_pseudo_fcall: igaOp = iga::Op::CALL; break;
    case G4_pseudo_fret:  igaOp = iga::Op::RET; break;
    case G4_pseudo_sada2: igaOp = iga::Op::SADA2; break;
    case G4_pseudo_exit:
        ASSERT_USER(false, "G4_pseudo_exit not GEN ISA OPCODE.");
        break;
    case G4_pseudo_fc_call: igaOp = iga::Op::CALL; break;
    case G4_pseudo_fc_ret:  igaOp = iga::Op::RET; break;
    case G4_intrinsic:
        ASSERT_USER(false, "G4_intrinsic not GEN ISA OPCODE.");
        break;
    case G4_sync_nop:
        igaOp = iga::Op::SYNC;
        sf = iga::SyncFC::NOP;
        break;
    case G4_sync_allrd:
        igaOp = iga::Op::SYNC;
        sf = iga::SyncFC::ALLRD;
        break;
    case G4_sync_allwr:
        igaOp = iga::Op::SYNC;
        sf = iga::SyncFC::ALLWR;
        break;
    case G4_NUM_OPCODE:
    default:
        ASSERT_USER(false, "INVALID opcode.");
        break;
    }

    return std::pair(igaOp, sf);
}

void BinaryEncodingIGA::SetSWSB(G4_INST *inst, iga::SWSB &sw)
{
    // Set token, e.g. $0
    if (inst->tokenHonourInstruction() && (inst->getToken() != (unsigned short)-1))
    {
        sw.tokenType = SWSB::TokenType::SET;
        sw.sbid = inst->getToken();
    }

    if ((unsigned)inst->getDistance())
    {
        {
            // there is only one pipe on single-dist-pipe platform,
            // must be REG_DIST
            sw.distType = SWSB::DistType::REG_DIST;
        }
        sw.minDist = (uint32_t)inst->getDistance();
    }

    // Set token dependency, e.g. $1.src
    if (inst->getDepTokenNum())
    {
        assert(sw.tokenType != SWSB::TokenType::SET &&
               "unexpect SWSB dependence type");
        assert(inst->getDepTokenNum() == 1 &&
            "More than one token dependence in one instruction");

        using SWSBTokenType = vISA::G4_INST::SWSBTokenType;

        for (int i = 0; i < (int)inst->getDepTokenNum(); i++)
        {
            SWSBTokenType type = SWSBTokenType::TOKEN_NONE;
            uint8_t token = (uint8_t)inst->getDepToken(i, type);
            if (type == SWSBTokenType::AFTER_READ)
            {
                sw.tokenType = SWSB::TokenType::SRC;
            }
            else if (type == SWSBTokenType::AFTER_WRITE)
            {
                sw.tokenType = SWSB::TokenType::DST;
            }
            sw.sbid = token;
        }
    }
    return;
}

void BinaryEncodingIGA::getIGAFlagInfo(
    G4_INST* inst,
    const OpSpec* opSpec,
    iga::Subfunction sf,
    Predication& pred,
    FlagModifier& condMod,
    RegRef& flagReg)
{
    G4_Predicate* predG4 = inst->getPredicate();
    G4_CondMod* condModG4 = inst->getCondMod();
    iga::RegRef predFlag;
    bool hasPredFlag = false;

    if (opSpec->supportsPredication() && predG4 != nullptr)
    {
        pred = getIGAPredication(predG4);
        predFlag = getIGAFlagReg(predG4->getBase());
        flagReg = predFlag;
        hasPredFlag = true;
    }

    bool hasImplicitModifier =
        opSpec->is(iga::Op::MATH) && iga::IsMacro(sf.math);

    if ((opSpec->supportsFlagModifier() || hasImplicitModifier) &&
        condModG4 != nullptr)
    {
        condMod = getIGAFlagModifier(condModG4);
        // in case for min/max sel instruction, it could have CondMod but has no flag registers
        if (condModG4->getBase() != nullptr) {
            flagReg = getIGAFlagReg(condModG4->getBase());
            // pred and condMod Flags must be the same
            assert(!hasPredFlag || predFlag == flagReg);
        }
    }
}

void BinaryEncodingIGA::Encode()
{
    FixInst();
    Block* currBB = nullptr;

    auto isFirstInstLabel = [this]()
    {
        for (auto bb : kernel.fg)
        {
            for (auto inst : *bb)
            {
                return inst->isLabel();
            }
        }
        return false;
    };

    // Make the size of the first BB be multiple of 4 instructions, and do not compact
    // any instructions in it, so that the size of the first BB is multiple of 64 bytes
    if (kernel.fg.builder->getHasPerThreadProlog() ||
        kernel.fg.builder->getHasComputeFFIDProlog())
    {
        G4_BB* first_bb = *kernel.fg.begin();
        size_t num_inst = first_bb->getInstList().size();
        assert(num_inst != 0 && "the first BB must not be empty");
        // label instructions don't count. Only the first instruction could be a label
        if (first_bb->getInstList().front()->isLabel())
            --num_inst;

        if (num_inst % 4 != 0) {
            size_t num_nop = 4 - (num_inst % 4);
            for (size_t i = 0; i < num_nop; ++i)
                first_bb->getInstList().push_back(
                    kernel.fg.builder->createNop(InstOpt_NoCompact));
        }
        // set all instruction to be NoCompact
        for (auto inst : *first_bb)
        {
            inst->setOptionOn(InstOpt_NoCompact);
        }
    }

    if (!isFirstInstLabel())
    {
        // create a new BB if kernel does not start with label
        currBB = IGAKernel->createBlock();
        IGAKernel->appendBlock(currBB);
    }

    std::list<std::pair<Instruction*, G4_INST*>> encodedInsts;
    iga::Block *bbNew = nullptr;
    for (auto bb : this->kernel.fg)
    {
        for (auto inst : *bb)
        {
            bbNew = nullptr;
            if (inst->isLabel())
            {
                // note that we create a new IGA BB per label instead of directly mapping vISA BB to IGA BB,
                // as some vISA BBs can have multiple labels (e.g., multiple endifs)
                G4_Label* label = inst->getLabel();
                currBB = lookupIGABlock(label, *IGAKernel);
                IGAKernel->appendBlock(currBB);
                continue;
            }

            Instruction *igaInst = translateInstruction(inst, bbNew);
            if (!igaInst) {
                // assertion failure already reported
                continue;
            }

            igaInst->addInstOpts(getIGAInstOptSet(inst));

            if (getPlatformGeneration(platform) >= PlatformGen::GEN12) {
                iga::SWSB sw;
                SetSWSB(inst, sw);

                SWSB::InstType instTy = SWSB::InstType::UNKNOWN;
                if (inst->isMath())
                    instTy = SWSB::InstType::MATH;
                else if (inst->isSend())
                    instTy = SWSB::InstType::SEND;
                else
                    instTy = SWSB::InstType::OTHERS;

                // Verify if swsb is in encode-able dist and token combination
                if(!sw.verify(getIGASWSBEncodeMode(*kernel.fg.builder), instTy))
                    IGA_ASSERT_FALSE("Invalid swsb dist and token combination");
                igaInst->setSWSB(sw);
            }

#if _DEBUG
            igaInst->validate();
#endif
            currBB->appendInstruction(igaInst);

            if (bbNew)
            {
                // Fall through block is created.
                // So the new block needs to become current block
                // so that jump offsets can be calculated correctly
                currBB = bbNew;
            }
            // If, in future, we generate multiple binary inst
            // for a single G4_INST, then it should be safe to
            // make pair between the G4_INST and first encoded
            // binary inst.
            encodedInsts.push_back(std::make_pair(igaInst, inst));
        }
    }

    kernel.setAsmCount(IGAInstId);


    if (m_kernelBuffer)
    {
        m_kernelBufferSize = 0;
        delete static_cast<uint8_t*>(m_kernelBuffer);
        m_kernelBuffer = nullptr;
    }

    //Will compact only if Compaction flag is present
    startTimer(TIMER_IGA_ENCODER);
    bool autoCompact = true;

    if (kernel.getOption(vISA_Compaction) == false)
    {
        autoCompact = false;
    }

    KernelEncoder encoder(IGAKernel, autoCompact);
    encoder.setSWSBEncodingMode(getIGASWSBEncodeMode(*kernel.fg.builder));

    if (kernel.getOption(vISA_EnableIGASWSB))
    {
        encoder.enableIGAAutoDeps();
    }

    encoder.encode();

    stopTimer(TIMER_IGA_ENCODER);
    m_kernelBufferSize = encoder.getBinarySize();
    m_kernelBuffer = allocCodeBlock(m_kernelBufferSize);
    memcpy_s(m_kernelBuffer, m_kernelBufferSize, encoder.getBinary(), m_kernelBufferSize);

    // encodedPC is available after encoding
    for (auto&& inst : encodedInsts)
    {
        inst.second->setGenOffset(inst.first->getPC());
    }
    if (kernel.fg.builder->getHasPerThreadProlog())
    {
        // per thread data load is in the first BB
        assert(kernel.fg.getNumBB() > 1 && "expect at least one prolog BB");
        auto secondBB = *(std::next(kernel.fg.begin()));
        auto iter = std::find_if(secondBB->begin(), secondBB->end(),
            [](G4_INST* inst) { return !inst->isLabel();});
        assert(iter != secondBB->end() && "execpt at least one non-label inst in second BB");
        kernel.fg.builder->getJitInfo()->offsetToSkipPerThreadDataLoad =
            (uint32_t)(*iter)->getGenOffset();
    }
    if (kernel.fg.builder->getHasComputeFFIDProlog())
    {
        // something weird will happen if both HasPerThreadProlog and HasComputeFFIDProlog
        assert(!kernel.fg.builder->getHasPerThreadProlog());

        // set offsetToSkipSetFFIDGP to the second entry's offset
        // the first instruction in the second BB is the start of the sencond entry
        assert(kernel.fg.getNumBB() > 1 && "expect at least one prolog BB");
        auto secondBB = *(std::next(kernel.fg.begin()));
        assert(!secondBB->empty() && !secondBB->front()->isLabel());
        kernel.fg.builder->getJitInfo()->offsetToSkipSetFFIDGP =
            (uint32_t)secondBB->front()->getGenOffset();
    }
}

iga::Instruction *BinaryEncodingIGA::translateInstruction(
    G4_INST *g4inst, iga::Block*& bbNew)
{
    iga::Instruction *igaInst = nullptr;
    auto opinfo = getIgaOpInfo(g4inst->opcode(), g4inst, platformModel->platform);
    iga::Op igaOp = opinfo.first;
    iga::Subfunction sf = opinfo.second;
    // common fields: op, predicate, flag reg, exec size, exec mask offset,
    // mask ctrl, conditional modifier
    const OpSpec* opSpec = &platformModel->lookupOpSpec(igaOp);
    if (!opSpec->isValid())
    {
        std::cerr << "INVALID opcode " << G4_Inst_Table[g4inst->opcode()].str << "\n";
        ASSERT_USER(false, "INVALID OPCODE.");
        return nullptr;
    }
    Predication pred;
    RegRef flagReg {0, 0};
    ExecSize execSize = getIGAExecSize(g4inst->getExecSize());
    ChannelOffset chOff = getIGAChannelOffset(g4inst->getMaskOffset());
    MaskCtrl maskCtrl =
        getIGAMaskCtrl(g4inst->opcode() == G4_jmpi || g4inst->isWriteEnableInst());
    FlagModifier condModifier = FlagModifier::NONE;

    getIGAFlagInfo(g4inst, opSpec, sf, pred, condModifier, flagReg);

    if (opSpec->isBranching())
    {
        BranchCntrl brnchCtrl = getIGABranchCntrl(g4inst->asCFInst()->isBackward());
        igaInst = IGAKernel->createBranchInstruction(
            *opSpec,
            pred,
            flagReg,
            execSize,
            chOff,
            maskCtrl,
            brnchCtrl);
    }
    else if (opSpec->isSendOrSendsFamily())
    {
        SendDesc desc = getIGASendDesc(g4inst);
        InstOptSet extraOpts; // empty set
        int xlen = -1;
        SendDesc exDesc = getIGASendExDesc(g4inst, xlen, extraOpts);
        igaInst =
            IGAKernel->createSendInstruction(
                *opSpec,
                sf.send,
                pred,
                flagReg,
                execSize,
                chOff,
                maskCtrl,
                exDesc,
                desc);

        ASSERT_USER(igaInst, "Instruction is NULL");
        if (!igaInst) {
            return nullptr;
        }

        igaInst->setSrc1Length(xlen);
        igaInst->addInstOpts(extraOpts);
    }
    else if (opSpec->op == Op::NOP)
    {
        igaInst = IGAKernel->createNopInstruction();
    }
    else if (opSpec->op == Op::ILLEGAL)
    {
        igaInst = IGAKernel->createIllegalInstruction();
    }
    else
    {
        igaInst =
            IGAKernel->createBasicInstruction(
                *opSpec,
                pred,
                flagReg,
                execSize,
                chOff,
                maskCtrl,
                condModifier,
                sf);
    }

    ASSERT_USER(igaInst, "Instruction is NULL");
    if (!igaInst) {
        // only on asserts; this should only happen if memory allocation
        // fails for some reason
        return nullptr;
    }

    igaInst->setID(IGAInstId++);
    igaInst->setLoc(g4inst->getCISAOff()); // make IGA src off track CISA id

    if (opSpec->supportsDestination())
    {
        translateInstructionDst(g4inst, igaInst);
    }

    if (opSpec->isBranching() &&
        igaOp != iga::Op::JMPI &&
        igaOp != iga::Op::RET &&
        igaOp != iga::Op::CALL &&
        igaOp != iga::Op::BRC &&
        igaOp != iga::Op::BRD)
    {
        translateInstructionBranchSrcs(g4inst, igaInst, bbNew);
    }
    else
    {
        translateInstructionSrcs(g4inst, igaInst);
    }

    return igaInst;
}

void BinaryEncodingIGA::translateInstructionDst(
    G4_INST *g4inst, iga::Instruction *igaInst)
{
    assert(g4inst->getDst() && "dst must not be null");
    G4_DstRegRegion* dst = g4inst->getDst();
    DstModifier dstModifier = getIGADstModifier(g4inst->getSaturate());
    Region::Horz hstride = getIGAHorz(dst->getHorzStride());
    Type type = getIGAType(dst->getType());

    // workaround for SKL bug
    // not all bits are copied from immediate descriptor
    if (g4inst->isSend() && platform >= GENX_SKL && platform < GENX_ICLLP)
    {
        G4_SendMsgDescriptor* msgDesc = g4inst->getMsgDesc();
        G4_Operand* descOpnd = g4inst->isSplitSend() ?
            g4inst->getSrc(2) : g4inst->getSrc(1);
        if (!descOpnd->isImm() && msgDesc->is16BitReturn())
        {
            type = Type::HF;
        }
    }

    if (igaInst->isMacro())
    {
        RegRef regRef = getIGARegRef(dst);
        Region::Horz hstride = getIGAHorz(dst->getHorzStride());
        igaInst->setMacroDestination(
            dstModifier,
            getIGARegName(dst),
            regRef,
            getIGAImplAcc(dst->getAccRegSel()),
            hstride,
            type);
    }
    else if (dst->getRegAccess() == Direct)
    {

        igaInst->setDirectDestination(
            dstModifier,
            getIGARegName(dst),
            getIGARegRef(dst),
            hstride,
            type);
    }
    else
    { // Operand::Kind::INDIRECT
        RegRef regRef {0, 0};
        bool valid;
        regRef.subRegNum = (uint8_t)dst->ExIndSubRegNum(valid);
        igaInst->setInidirectDestination(
            dstModifier,
            regRef,
            dst->getAddrImm(),
            hstride,
            type);
    }
}

void BinaryEncodingIGA::translateInstructionBranchSrcs(
    G4_INST *inst, iga::Instruction *igaInst, iga::Block*& bbNew)
{
    if (inst->asCFInst()->getJip())
    {
        // encode jip/uip for branch inst
        // note that it does not apply to jmpi/call/ret/brc/brd,
        // which may have register sources. Their label appears directly as
        // source operand instead.
        G4_Operand* uip = inst->asCFInst()->getUip();
        G4_Operand* jip = inst->asCFInst()->getJip();
        //iga will take care off
        if (uip)
        {
            igaInst->setLabelSource(SourceIndex::SRC1,
                lookupIGABlock(uip->asLabel(), *IGAKernel), iga::Type::UD);
        }

        igaInst->setLabelSource(SourceIndex::SRC0,
            lookupIGABlock(jip->asLabel(), *IGAKernel), iga::Type::UD);
    }
    else
    {
        // Creating a fall through block
        bbNew = IGAKernel->createBlock();
        igaInst->setLabelSource(SourceIndex::SRC0, bbNew, iga::Type::UD);
        IGAKernel->appendBlock(bbNew);
    }
}

void BinaryEncodingIGA::translateInstructionSrcs(
    G4_INST *inst, iga::Instruction *igaInst)
{
    // set source operands
    int numSrcToEncode = inst->getNumSrc();
    if (inst->isSend())
    {
        // skip desc/exdesc as they are handled separately
        numSrcToEncode = inst->isSplitSend() ? 2 : 1;

        if (numSrcToEncode == 1 && platformModel->platform >= Platform::GEN12P1)
        {
            RegRef regTemp(0, 0);
            Region rgnTemp = iga::Region::SRC010;

            igaInst->setDirectSource(
                SourceIndex::SRC1,
                SrcModifier::NONE,
                RegName::ARF_NULL,
                regTemp,
                rgnTemp,
                Type::INVALID);
        }
    }
    if (platform >= GENX_ICLLP
        && inst->opcode() == G4_movi && numSrcToEncode == 1)
    {
        // From ICL, 'movi' becomes a binary instruction with an
        // optional immediate operand, which needs encoding as null
        // or imm32. So far, within vISA jitter, 'movi' is still
        // modeled as unary instruction, setting src1 to null for
        // platforms >= CNL.
        RegRef regTemp(0, 0);
        Region rgnTemp = iga::Region::SRC110;
        igaInst->setDirectSource(SourceIndex::SRC1,
            SrcModifier::NONE,
            RegName::ARF_NULL,
            regTemp, rgnTemp,
            Type::UB);
    }
    for (int i = 0; i < numSrcToEncode; i++)
    {
        SourceIndex opIx = (SourceIndex)((int)SourceIndex::SRC0 + i);
        G4_Operand* src = inst->getSrc(i);

        if (src->isSrcRegRegion())
        {
            G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
            SrcModifier srcMod = getIGASrcModifier(srcRegion->getModifier());
            Region region = getIGARegion(srcRegion, i);
            Type type = Type::INVALID;

            // let IGA take care of types for send/s instructions
            if (!igaInst->getOpSpec().isSendOrSendsFamily())
            {
                type = getIGAType(src->getType());
            }
            else if (i == 0 && platform >= GENX_SKL && platform < GENX_ICLLP)
            {
                // work around for SKL bug
                // not all bits are copied from immediate descriptor
                G4_SendMsgDescriptor* msgDesc = inst->getMsgDesc();
                G4_Operand* descOpnd = inst->isSplitSend() ?
                    inst->getSrc(2) : inst->getSrc(1);
                if (!descOpnd->isImm() && msgDesc->is16BitInput())
                {
                    type = Type::HF;
                }
            }

            if (igaInst->isMacro())
            {
                auto accRegSel =
                    srcRegion->isNullReg() ? NOACC : srcRegion->getAccRegSel();
                RegRef regRef = getIGARegRef(srcRegion);
                igaInst->setMacroSource(
                    opIx,
                    srcMod,
                    getIGARegName(srcRegion),
                    regRef,
                    getIGAImplAcc(accRegSel),
                    region,
                    type);
            }
            else if (srcRegion->getRegAccess() == Direct)
            {
                igaInst->setDirectSource(
                    opIx,
                    srcMod,
                    getIGARegName(srcRegion),
                    getIGARegRef(srcRegion),
                    region,
                    type);
            }
            else
            {
                RegRef regRef {0, 0};
                bool valid;
                regRef.subRegNum = (uint8_t)srcRegion->ExIndSubRegNum(valid);
                igaInst->setInidirectSource(
                    opIx,
                    srcMod,
                    regRef,
                    srcRegion->getAddrImm(),
                    region,
                    type);
            }
        }
        else if (src->isLabel())
        {
            igaInst->setLabelSource(opIx,
                lookupIGABlock(src->asLabel(), *IGAKernel), iga::Type::UD);
        }
        else if (src->isImm())
        {
            Type type = getIGAType(src->getType());
            ImmVal val;
            val = src->asImm()->getImm();
            val.kind = getIGAImmType(src->getType());
            igaInst->setImmediateSource(opIx, val, type);
        }
        else
        {
            IGA_ASSERT_FALSE("unexpected src kind");
        }
    } // for
}

SWSB_ENCODE_MODE BinaryEncodingIGA::getIGASWSBEncodeMode(const IR_Builder& builder) {
    if (getPlatformGeneration(builder.getPlatform()) < PlatformGen::GEN12)
        return SWSB_ENCODE_MODE::SWSBInvalidMode;


    return SWSB_ENCODE_MODE::SingleDistPipe;
}

SendDesc BinaryEncodingIGA::getIGASendDesc(G4_INST* sendInst) const
{
    SendDesc desc;
    assert(sendInst->isSend() && "expect send inst");
    G4_Operand* msgDesc = sendInst->isSplitSend() ?
        sendInst->getSrc(2) : sendInst->getSrc(1);
    if (msgDesc->isImm())
    {
        desc.type = SendDesc::Kind::IMM;
        desc.imm = (uint32_t)msgDesc->asImm()->getImm();
    }
    else
    {
        desc.type = SendDesc::Kind::REG32A;
        desc.reg.regNum = 0; // must be a0
        bool valid = false;
        desc.reg.subRegNum = (uint8_t)msgDesc->asSrcRegRegion()->ExSubRegNum(valid);
        assert(valid && "invalid subreg");
    }

    return desc;
}

SendDesc BinaryEncodingIGA::getIGASendExDesc(
    G4_INST* sendInst, int& xlen, iga::InstOptSet& extraOpts) const
{
    SendDesc exDescArg;

    if (sendInst->isEOT())
        extraOpts.add(InstOpt::EOT);

    xlen = -1;

    assert(sendInst->isSend() && "expect send inst");
    if (sendInst->isSplitSend())
    {
        G4_Operand* exDesc = sendInst->getSrc(3);
        if (exDesc->isImm())
        {
            G4_SendMsgDescriptor* g4SendMsg = sendInst->getMsgDesc();
            xlen = (int)g4SendMsg->extMessageLength();
            //
            exDescArg.type = SendDesc::Kind::IMM;
            uint32_t tVal = (uint32_t)exDesc->asImm()->getImm();
            //We must clear the funcID in the extended message for Gen12+
            //It's because the explicit encoding is applied, no mapping anymore.
            //ditto for the EOT bit which is moved out of extDesc
            //The extended message format
            //struct ExtendedMsgDescLayout {
            //    uint32_t funcID : 4;       // bit 0:3 << not part of ExDesc
            //    uint32_t unnamed1 : 1;     // bit 4
            //    uint32_t eot : 1;          // bit 5 << not part of ExDesc
            //    uint32_t extMsgLength : 5; // bit 6:10
            //    uint32_t unnamed2 : 5;     // bit 11:15
            //    uint32_t extFuncCtrl : 16; // bit 16:31
            //};
            if (getPlatformGeneration(sendInst->getPlatform()) >= PlatformGen::GEN12)
            {
                tVal &= 0xFFFFFFC0;
            }
            exDescArg.imm = tVal;
        }
        else
        {
            exDescArg.type = SendDesc::Kind::REG32A;
            exDescArg.reg.regNum = 0; // must be a0
            bool valid = false;
            exDescArg.reg.subRegNum =
                (uint8_t)exDesc->asSrcRegRegion()->ExSubRegNum(valid);
            assert(valid && "invalid subreg");
        }
    }
    else // old unary packed send
    {
        // exDesc is stored in SendMsgDesc and must be IMM
        G4_SendMsgDescriptor* sendDesc = sendInst->getMsgDesc();
        assert(sendDesc != nullptr && "null msg desc");
        exDescArg.type = SendDesc::Kind::IMM;
        uint32_t tVal = sendDesc->getExtendedDesc();

        // We must clear the funcID in the extended message
        if (getPlatformGeneration(sendInst->getPlatform()) >= PlatformGen::GEN12)
        {
            tVal = tVal & 0xFFFFFFF0;
        }
        exDescArg.imm = tVal;
        // non-split send implies Src1.Length == 0
        xlen = 0;
    }

    return exDescArg;
}

void *BinaryEncodingIGA::EmitBinary(uint32_t& binarySize)
{
    binarySize = m_kernelBufferSize;


    if (kernel.getOption(vISA_GenerateBinary))
    {
        std::string binFileName = fileName + ".dat";
        std::string errStr;
        ofstream os(binFileName.c_str(), ios::binary);
        if (!os)
        {
            errStr = "Can't open " + binFileName + ".\n";
            MUST_BE_TRUE(0, errStr);
            return nullptr;
        }
        os.write((const char*) m_kernelBuffer, binarySize);
    }

    return m_kernelBuffer;
}

iga::ExecSize BinaryEncodingIGA::getIGAExecSize(int execSize)
{
    switch (execSize)
    {
    case 1:     return iga::ExecSize::SIMD1;
    case 2:     return iga::ExecSize::SIMD2;
    case 4:     return iga::ExecSize::SIMD4;
    case 8:     return iga::ExecSize::SIMD8;
    case 16:    return iga::ExecSize::SIMD16;
    case 32:    return iga::ExecSize::SIMD32;
    default:
        assert(false && "illegal simd size");
        return iga::ExecSize::INVALID;
    }
}

iga::ChannelOffset BinaryEncodingIGA::getIGAChannelOffset(int offset)
{
    switch (offset)
    {
    case 0:     return iga::ChannelOffset::M0;
    case 4:     return iga::ChannelOffset::M4;
    case 8:     return iga::ChannelOffset::M8;
    case 12:    return iga::ChannelOffset::M12;
    case 16:    return iga::ChannelOffset::M16;
    case 20:    return iga::ChannelOffset::M20;
    case 24:    return iga::ChannelOffset::M24;
    case 28:    return iga::ChannelOffset::M28;
    default:
        assert(false && "illegal mask offset");
        return iga::ChannelOffset::M0;
    }
}

iga::MaskCtrl BinaryEncodingIGA::getIGAMaskCtrl(bool noMask)
{
    return noMask ? iga::MaskCtrl::NOMASK : iga::MaskCtrl::NORMAL;
}

iga::RegName BinaryEncodingIGA::getIGAARFName(G4_ArchRegKind areg)
{
    switch (areg)
    {
    case AREG_NULL:     return iga::RegName::ARF_NULL;
    case AREG_A0:       return iga::RegName::ARF_A;
    case AREG_ACC0:
    case AREG_ACC1:     return iga::RegName::ARF_ACC;
    case AREG_MASK0:    return iga::RegName::ARF_CE;
    case AREG_MS0:      return iga::RegName::ARF_MSG;
    case AREG_DBG:      return iga::RegName::ARF_DBG;
    case AREG_SR0:      return iga::RegName::ARF_SR;
    case AREG_CR0:      return iga::RegName::ARF_CR;
    case AREG_N0:
    case AREG_N1:       return iga::RegName::ARF_N;
    case AREG_IP:       return iga::RegName::ARF_IP;
    case AREG_F0:
    case AREG_F1:
        return iga::RegName::ARF_F;
    case AREG_TM0:      return iga::RegName::ARF_TM;
    case AREG_TDR0:     return iga::RegName::ARF_TDR;
    case AREG_SP:       return iga::RegName::ARF_SP;
    default:
        assert(false && "illegal ARF");
        return iga::RegName::INVALID;
    }
}

iga::Type BinaryEncodingIGA::getIGAType(G4_Type type)
{
    switch (type)
    {
    case Type_UB:   return iga::Type::UB;
    case Type_B:    return iga::Type::B;
    case Type_UW:   return iga::Type::UW;
    case Type_W:    return iga::Type::W;
    case Type_UD:   return iga::Type::UD;
    case Type_D:    return iga::Type::D;
    case Type_UQ:   return iga::Type::UQ;
    case Type_Q:    return iga::Type::Q;
    case Type_HF:   return iga::Type::HF;
    case Type_F:    return iga::Type::F;
    case Type_DF:   return iga::Type::DF;
    case Type_UV:   return iga::Type::UV;
    case Type_V:    return iga::Type::V;
    case Type_VF:   return iga::Type::VF;
    case Type_NF:   return iga::Type::NF;
    default:
        assert(false && "illegal type");
        return iga::Type::INVALID;
    }
}

iga::PredCtrl BinaryEncodingIGA::getIGAPredCtrl(G4_Predicate_Control g4PrCtl)
{
    switch (g4PrCtl)
    {
    case PRED_DEFAULT:      return iga::PredCtrl::SEQ;
    case PRED_ANY2H:        return iga::PredCtrl::ANY2H;
    case PRED_ANY4H:        return iga::PredCtrl::ANY4H;
    case PRED_ANY8H:        return iga::PredCtrl::ANY8H;
    case PRED_ANY16H:       return iga::PredCtrl::ANY16H;
    case PRED_ANY32H:       return iga::PredCtrl::ANY32H;
    case PRED_ALL2H:        return iga::PredCtrl::ALL2H;
    case PRED_ALL4H:        return iga::PredCtrl::ALL4H;
    case PRED_ALL8H:        return iga::PredCtrl::ALL8H;
    case PRED_ALL16H:       return iga::PredCtrl::ALL16H;
    case PRED_ALL32H:       return iga::PredCtrl::ALL32H;
    case PRED_ANYV:         return iga::PredCtrl::ANYV;
    case PRED_ALLV:         return iga::PredCtrl::ALLV;
    default:
        assert(false && "illegal predicate control");
        return iga::PredCtrl::NONE;
    }
}

iga::Predication BinaryEncodingIGA::getIGAPredication(G4_Predicate* predG4)
{
    iga::Predication pred;
    if (predG4)
    {
        pred.function = getIGAPredCtrl(predG4->getControl());
        pred.inverse = predG4->getState() != PredState_Plus;
    }
    return pred;
}

iga::SrcModifier BinaryEncodingIGA::getIGASrcModifier(G4_SrcModifier srcMod)
{
    switch (srcMod)
    {
    case Mod_Minus:         return iga::SrcModifier::NEG;
    case Mod_Abs:           return iga::SrcModifier::ABS;
    case Mod_Minus_Abs:     return iga::SrcModifier::NEG_ABS;
    case Mod_Not:           return iga::SrcModifier::NEG;
    case Mod_src_undef:     return iga::SrcModifier::NONE;
    default:
        assert(false && "illegal source modifier");
        return iga::SrcModifier::NONE;
    }
}

iga::Region::Vert BinaryEncodingIGA::getIGAVert(int vstride)
{
    switch (vstride)
    {
    case 0:                 return iga::Region::Vert::VT_0;
    case 1:                 return iga::Region::Vert::VT_1;
    case 2:                 return iga::Region::Vert::VT_2;
    case 4:                 return iga::Region::Vert::VT_4;
    case 8:                 return iga::Region::Vert::VT_8;
    case 16:                return iga::Region::Vert::VT_16;
    case 32:                return iga::Region::Vert::VT_32;
    case UNDEFINED_SHORT:   return iga::Region::Vert::VT_VxH;
    default:
        assert(false && "illegal vstride");
        return iga::Region::Vert::VT_INVALID;
    }
}

iga::Region::Width BinaryEncodingIGA::getIGAWidth(int width)
{
    switch (width)
    {
    case 1:     return iga::Region::Width::WI_1;
    case 2:     return iga::Region::Width::WI_2;
    case 4:     return iga::Region::Width::WI_4;
    case 8:     return iga::Region::Width::WI_8;
    case 16:    return iga::Region::Width::WI_16;
    default:
        assert(false && "illegal width");
        return iga::Region::Width::WI_INVALID;
    }
}

iga::Region::Horz BinaryEncodingIGA::getIGAHorz(int hstride)
{
    switch (hstride)
    {
    case 0:     return iga::Region::Horz::HZ_0;
    case 1:     return iga::Region::Horz::HZ_1;
    case 2:     return iga::Region::Horz::HZ_2;
    case 4:     return iga::Region::Horz::HZ_4;
    default:
        assert(false && "illegal hstride");
        return iga::Region::Horz::HZ_INVALID;
    }
}

iga::Region BinaryEncodingIGA::getIGARegion(
    G4_SrcRegRegion* srcRegion, int srcPos)
{
    iga::Region igaRegion;
    const RegionDesc* region = srcRegion->getRegion();
    if ((srcRegion->getInst()->getNumSrc() == 3 &&
        !srcRegion->getInst()->isSend()))
    {
        // special handling for 3src instructions
        if (srcPos != 2)
        {
            // for src0 and src1, IGA/GED does not like width to be set
            igaRegion.set(
                getIGAVert(region->vertStride),
                iga::Region::Width::WI_INVALID,
                getIGAHorz(region->horzStride));
        }
        else
        {
            // for src2, IGA expects both VS and W to be invalid
            igaRegion.set(
                iga::Region::Vert::VT_INVALID,
                iga::Region::Width::WI_INVALID,
                getIGAHorz(region->horzStride));
        }
    }
    else
    {
        igaRegion.set(
            getIGAVert(region->vertStride),
            getIGAWidth(region->width),
            getIGAHorz(region->horzStride));
    }
    return igaRegion;
}

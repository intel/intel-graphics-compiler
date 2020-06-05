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
#include "IGAToGEDTranslation.hpp"
#include "Encoder.hpp"
#include "../../IR/Kernel.hpp"
#include "../../Frontend/IRToString.hpp"
#include "../../Models/Models.hpp"
#include "../../Timer/Timer.hpp"
#include "../../IR/SWSBSetter.hpp"
#include <cstring>

using namespace iga;

//
// NOTE: if we have it to do again I would have:
//     Encoder::encodeInstruction(Instruction &)
//  instantiate a
//     BasicInstructionEncoder (derives from InstructionEncoder)
// And call run on that op (pass the GEDInstruction).
// This localizes the encoding complexity per format.
// The various template functions below would be inherited from
// InstructionEncoder


static const char *gedReturnValueToString(GED_RETURN_VALUE rv)
{
    switch(rv) {
    case GED_RETURN_VALUE_SUCCESS: return "success";
    case GED_RETURN_VALUE_CYCLIC_DEPENDENCY: return "cyclic dependency";
    case GED_RETURN_VALUE_NULL_POINTER: return "null pointer";
    case GED_RETURN_VALUE_OPCODE_NOT_SUPPORTED: return "unsupported opcode";
    case GED_RETURN_VALUE_NO_COMPACT_FORM: return "no compact form";
    case GED_RETURN_VALUE_INVALID_FIELD: return "invalid field";
    case GED_RETURN_VALUE_INVALID_VALUE: return "invalid value";
    case GED_RETURN_VALUE_INVALID_INTERPRETER: return "invalid interpreter";
    default: return "other error";
    }
}
void Encoder::handleGedError(
    int line, const char *setter, GED_RETURN_VALUE status)
{
    error("encoder line %d: unsupported GED setter %s (%s)",
        line, setter, gedReturnValueToString(status), setter);
}



Encoder::Encoder(
    const Model &model,
    ErrorHandler &errHandler,
    const EncoderOpts &opts)
    : GEDBitProcessor(model, errHandler)
    , m_opts(opts)
    , m_numberInstructionsEncoded(0)
    , m_mem(nullptr)
{
    // derive the swsb encoding mode from platform if not set
    if (opts.swsbEncodeMode == SWSB_ENCODE_MODE::SWSBInvalidMode) {
        m_opts.swsbEncodeMode = model.getSWSBEncodeMode();
    }
}

void Encoder::encodeKernelPreProcess(Kernel &k)
{
    doEncodeKernelPreProcess(k);
}

void Encoder::doEncodeKernelPreProcess(Kernel &k)
{
    if (m_opts.autoDepSet && platform() >= Platform::GEN12P1) {
        SWSBAnalyzer swsb_analyzer(k, errorHandler(), m_opts.swsbEncodeMode, m_opts.sbidCount);
        swsb_analyzer.run();
    }
}

double Encoder::getElapsedTimeMicros(unsigned int idx)
{
    return getIGATimerUS(idx);
}

int64_t Encoder::getElapsedTimeTicks(unsigned int idx)
{
    return getIGATimerTicks(idx);
}

std::string Encoder::getTimerName(unsigned int idx)
{
    return getIGATimerNames(idx);
}

size_t Encoder::getNumInstructionsEncoded() const
{
    return m_numberInstructionsEncoded;
}

void Encoder::encodeKernel(
    Kernel &k,
    MemManager &mem,
    void *&bits,
    uint32_t &bitsLen)
{
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
    try {
#endif
        initIGATimer();
        setIGAKernelName("test");
        IGA_ASSERT(k.getModel().platform == platform(),
            "kernel/encoder model mismatch");

        encodeKernelPreProcess(k);
        m_needToPatch.clear();
        m_mem = &mem;
        m_numberInstructionsEncoded = k.getInstructionCount();
        size_t allocLen = m_numberInstructionsEncoded * UNCOMPACTED_SIZE;
        if (allocLen == 0) // for empty kernel case
            allocLen = 4;
        m_instBuf = (uint8_t *)mem.alloc(allocLen);
        if (!m_instBuf) {
            fatalAt(0, "failed to allocate memory for kernel binary");
            return;
        }

        for (auto blk : k.getBlockList()) {
            START_ENCODER_TIMER()
            encodeBlock(blk);
            STOP_ENCODER_TIMER();
            if (hasFatalError()) {
                return;
            }
        }
        START_ENCODER_TIMER()
        patchJumpOffsets();
        STOP_ENCODER_TIMER()

        // setting actual size
        bitsLen = currentPc();
        bits = m_instBuf;

        applyGedWorkarounds(k, currentPc());

        // clear any padding
        memset(m_instBuf + bitsLen, 0, allocLen - bitsLen);
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
    } catch (const iga::FatalError&) {
        // error is already reported
    }
#endif
}

void Encoder::encodeBlock(Block *blk)
{
    m_blockToOffsetMap[blk] = currentPc();
    for (const auto inst : blk->getInstList()) {
        setCurrInst(inst);
        encodeInstruction(*inst);
        if (hasFatalError()) {
            return;
        }
        setEncodedPC(inst, currentPc());

        GED_RETURN_VALUE status = GED_RETURN_VALUE_SIZE;
        bool mustCompact = inst->hasInstOpt(InstOpt::COMPACTED);
        bool mustNotCompact =
            inst->hasInstOpt(InstOpt::NOCOMPACT);
        int32_t iLen = 16;
        if (mustCompact || (!mustNotCompact && m_opts.autoCompact)) {
            // try compact first
            status = GED_EncodeIns(&m_gedInst, GED_INS_TYPE_COMPACT, m_instBuf + currentPc());
            if (status == GED_RETURN_VALUE_SUCCESS) {
                //If auto compation is turned on, in case we need to patch later.
                inst->addInstOpt(InstOpt::COMPACTED);
                iLen = 8;
            } else if (status == GED_RETURN_VALUE_NO_COMPACT_FORM) {
                if (mustCompact) {
                    if (m_opts.explicitCompactMissIsWarning) {
                        warningAt(inst->getLoc(), "GED unable to compact instruction");
                    } else {
                        errorAt(inst->getLoc(), "GED unable to compact instruction");
                    }
                }
            } // else: some other error (unreachable?)
        }

        // try native encoding
        if (status != GED_RETURN_VALUE_SUCCESS) {
            inst->removeInstOpt(InstOpt::COMPACTED);
            status = GED_EncodeIns(&m_gedInst, GED_INS_TYPE_NATIVE, m_instBuf + currentPc());
            if (status != GED_RETURN_VALUE_SUCCESS) {
                errorAt(inst->getLoc(), "GED unable to encode instruction: %s",
                    gedReturnValueToString(status));
            }
        }

        advancePc(iLen);
    }
}

bool Encoder::getBlockOffset(const Block *b, uint32_t &pc)
{
    auto iter = m_blockToOffsetMap.find(b);
    if (iter != m_blockToOffsetMap.end())
    {
        pc = iter->second;
        return true;
    }
    return false;
}

void Encoder::setEncodedPC(Instruction *inst, int32_t encodedPC)
{
#if 0
    auto iter = m_instPcs.find(inst);
    IGA_ASSERT(iter == m_instPcs.end(), "resetting encode PC");
    ((void)iter); // dummy use where ASSERT_USER compiles out
    m_instPcs[inst] = encodedPC;
#else
    inst->setPC(encodedPC);
#endif
}

int32_t Encoder::getEncodedPC(const Instruction *inst) const
{
#if 0
    auto iter = m_instPcs.find(inst);
    if (iter == m_instPcs.end()) {
        IGA_ASSERT_FALSE("inst PC not found");
        return 0;
    }
    return iter->second;
#else
    return inst->getPC();
#endif
}

void Encoder::encodeFC(const Instruction &i)
{
    const OpSpec &os = i.getOpSpec();

    if (os.is(Op::MATH)) {
        GED_MATH_FC mfc = lowerMathFC(i.getMathFc());
        GED_ENCODE(MathFC, mfc);
    } else if (os.isSendOrSendsFamily()) {
        if (platform() >= Platform::GEN12P1) {
            auto sfid = lowerSFID(i.getSendFc());
            GED_ENCODE(SFID, sfid);
        }
    } else if (os.is(Op::SYNC)) {
        GED_SYNC_FC wfc = lowerSyncFC(i.getSyncFc());
        GED_ENCODE(SyncFC, wfc);
    } else if (os.supportsBranchCtrl()) {
        GED_ENCODE(BranchCtrl,
            lowerBranchCntrl(i.getBranchCtrl()));
    } else if (os.supportsSubfunction()) {
        IGA_ASSERT_FALSE("encoder needs to encode subfunction for this op");
    }
}

void Encoder::encodeInstruction(Instruction& inst)
{
    m_opcode = inst.getOp();
    const auto gedPlat = lowerPlatform(platform());
    const auto gedOp = lowerOpcode(m_opcode);
    GED_RETURN_VALUE status = GED_InitEmptyIns(
        gedPlat,
        &m_gedInst,
        gedOp);
    if (status != GED_RETURN_VALUE_SUCCESS) {
        fatalAt(inst.getLoc(), "GED failed to create instruction template");
        return;
    }

    if (m_opcode == Op::ILLEGAL) {
        // GED does all the work for this instruction
        return;
    } else if (m_opcode == Op::NOP) {
        // nop supports {Breakpoint}
        encodeOptions(inst);
        return;
    }
    const OpSpec &os = inst.getOpSpec();

    // Dwindling cases where we must use Align16
    // Pre-GEN10 ternary ops are all align16
    bool isTernary = platform() < Platform::GEN10 && os.isTernary();
    bool contextSaveRestoreNeedsAlign16 =
        isAlign16MathMacroRegisterCsrOperand(inst.getDestination()) ||
        isAlign16MathMacroRegisterCsrOperand(inst.getSource(0));
    // IEEE macro instructions (math.invm and math.rsqrtm)
    bool align16MacroInst = m_model.supportsAlign16MacroInst() && inst.isMacro();
    m_encodeAlign16 = isTernary || contextSaveRestoreNeedsAlign16 || align16MacroInst;
    GED_ACCESS_MODE accessMode = m_encodeAlign16 ?
        GED_ACCESS_MODE_Align16 : GED_ACCESS_MODE_Align1;
    if (m_model.supportsAccessMode()) {
        GED_ENCODE(AccessMode, accessMode);
    } // else GED will crash given this call (even given Align1)

    ExecSize execSize = inst.getExecSize();
    if (os.isTernary() &&
        m_model.supportsAlign16Ternary() &&
        inst.getExecSize() == ExecSize::SIMD1)
    {
        // scalar ternary workaround for Align16
        // (c.f. Encoder::encodeTernaryDestinationAlign16)
        execSize = inst.getDestination().getType() == Type::DF ?
            ExecSize::SIMD2 : ExecSize::SIMD4;
    }
    GED_ENCODE(ExecSize, lowerExecSize(execSize));

    encodeFC(inst);

    if (os.supportsQtrCtrl()) {
        // use ExecSize from above since it may  have been modified
        GED_CHANNEL_OFFSET qtrCtrl = lowerQtrCtrl(inst.getChannelOffset());
        GED_ENCODE(ChannelOffset, qtrCtrl);
    }

    GED_ENCODE(MaskCtrl, lowerEmask(inst.getMaskCtrl()));

    // Predicate
    const Predication &pred = inst.getPredication();
    if (os.supportsPredication()) {
        GED_ENCODE(PredCtrl, lowerPredCtrl(pred.function));
    } else {
        GED_ENCODE(PredCtrl, GED_PRED_CTRL_Normal);
    }

    bool isImm64Src0Overlap = false;
    isImm64Src0Overlap =
        platform() >= Platform::GEN12P1 &&
        inst.getSource(0).getKind() == Operand::Kind::IMMEDIATE &&
        TypeIs64b(inst.getSource(0).getType());

    if (!isImm64Src0Overlap && inst.getOpSpec().supportsFlagModifier()) {
            GED_ENCODE(CondModifier, lowerCondModifier(inst.getFlagModifier()));
    }

    bool hasFlagRegField = true;

    if (os.supportsPredication() && hasFlagRegField)
        GED_ENCODE(PredInv, pred.inverse ? GED_PRED_INV_Invert : GED_PRED_INV_Normal);

    // GED_ExecutionDataType
    RegRef flagReg = inst.getFlagReg();
    if (hasFlagRegField && (flagReg != REGREF_INVALID)) {
        GED_ENCODE(FlagRegNum, static_cast<uint32_t>(inst.getFlagReg().regNum));
        GED_ENCODE(FlagSubRegNum, inst.getFlagReg().subRegNum);
    }

    // set AccWrEn where supported
    if (inst.hasInstOpt(InstOpt::ACCWREN)) {
        GED_ENCODE(AccWrCtrl, GED_ACC_WR_CTRL_AccWrEn);
    }

    if (os.isBranching()) {
        if (m_model.supportsSimplifiedBranches()) {
            encodeBranchingInstructionSimplified(inst);
        } else {
            encodeBranchingInstruction(inst);
        }
        // options encoded internally
    } else if (os.isTernary()) {
        encodeTernaryInstruction(inst, accessMode);
    } else if (os.isSendOrSendsFamily()) {
        encodeSendInstruction(inst);
    } else if (os.is(Op::SYNC)) {
        encodeSyncInstruction(inst);
    } else {
        encodeBasicInstruction(inst, accessMode);
    }

    if (!hasFatalError()) {
        encodeOptions(inst);

        // setup for back patching on branching ops
        if (os.isBranching() || inst.isMovWithLabel()) {
            bool src0IsLabel = inst.getSource(0).isImm();
            bool src1IsLabel = inst.getSourceCount() > 1 && inst.getSource(1).isImm();
            if (src0IsLabel || src1IsLabel) {
                m_needToPatch.emplace_back(&inst, m_gedInst, m_instBuf + currentPc());
            }
        }
    }
}

void Encoder::encodeBasicInstruction(
    const Instruction& inst,
    GED_ACCESS_MODE accessMode)
{
    const OpSpec& os = inst.getOpSpec();
    if (os.supportsDestination()) {
        encodeBasicDestination(inst, inst.getDestination(), accessMode);
    } else if (os.op == Op::WAIT ) {
        // wait has an implicit destination (same as first source)
        // but with dst region of <1>
        Operand copy = inst.getSource(0);
        copy.setRegion(Region::DST1);
        encodeBasicDestination(inst, copy);
    }

    switch (inst.getSourceCount())
    {
    case 2:
        encodeBasicSource<SourceIndex::SRC1>(inst, inst.getSource(1), accessMode);
        // vvvv fall through vvvv
    case 1:
        encodeBasicSource<SourceIndex::SRC0>(inst, inst.getSource(0), accessMode);
    }
}

void Encoder::encodeTernaryDestinationAlign1(const Instruction& inst)
{
    const Operand& dst = inst.getDestination();

    if (inst.getOpSpec().supportsSaturation()) {
        GED_ENCODE(Saturate, lowerSaturate(dst.getDstModifier()));
    }
    GED_ENCODE(DstDataType, lowerDataType(dst.getType()));
    GED_ENCODE(DstRegFile, lowerRegFile(dst.getDirRegName()));
    encodeDstReg(dst.getDirRegName(), dst.getDirRegRef().regNum);

    if (inst.isMacro()) {
        GED_ENCODE(DstMathMacroExt, lowerMathMacroReg(dst.getMathMacroExt()));
        // GED_ENCODE(DstHorzStride, 1);
    } else {
        encodeDstSubRegNum(subRegNumToBinNum(
            dst.getDirRegRef().subRegNum, dst.getDirRegName(), dst.getType()), true);
        bool hasDstRgnHz = true;
        if (hasDstRgnHz) {
            GED_ENCODE(DstHorzStride, static_cast<int>(dst.getRegion().getHz()));
        }
    }
}

template <SourceIndex S>
void Encoder::encodeTernarySourceAlign1(const Instruction& inst)
{
    // CNL+ align1 ternary
    if (platform() < Platform::GEN10) {
        fatal("src%d: align1 ternary is not supported on this platform.", (int)S);
        return;
    }

    const Operand& src = inst.getSource(S);
    Type srcType = src.getType();


    // GED will catch any mismatch between float and int (illegal mixed mode)
    encodeSrcType<S>(srcType); // GED dependency requires type before reg file

    switch (src.getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::MACRO: {
        encodeSrcRegFile<S>(lowerRegFile(src.getDirRegName()));

        if (platform() <= Platform::GEN11) {
            encodeSrcAddrMode<S>(GED_ADDR_MODE_Direct);
        }

        // source modifiers
        if (inst.getOpSpec().supportsSourceModifiers()) {
            encodeSrcModifier<S>(src.getSrcModifier());
        }

        // regioning
        //
        // ternary align1 puts SpcAcc into subreg, so regions may be set
        // in all cases
        auto rgn = src.getRegion();
        // * madm doesn't have a region in GEN9 ...
        //   it does in GEN10+, but we haven't supported it in syntax yet
        //   and leave it to GED to set it
        // * src0 and src1 only has <w;h>, src2 only has <h>
        bool hasSrcRgnHz = !inst.isMacro();
        bool hasSrcRgnVt = !inst.isMacro() && S < SourceIndex::SRC2;
        if (hasSrcRgnHz) {
            encodeSrcRegionHorz<S>(rgn.getHz());
        }
        if (hasSrcRgnVt) {
            encodeTernarySrcRegionVert(S, rgn.getVt());
        }
        // register and subregister
        encodeSrcReg<S>(src.getDirRegName(), src.getDirRegRef().regNum);
        if (inst.isMacro()) {
            if (platform() < Platform::GEN11) {
                fatal("src%d: math macro operands require Align16", (int)S);
                return;
            }
            encodeSrcMathMacroReg<S>(src.getMathMacroExt());
            if (S != SourceIndex::SRC2) {
                encodeTernarySrcRegionVert(S, Region::Vert::VT_4);
            }
            encodeSrcRegionHorz<S>(Region::Horz::HZ_1);

        } else {
            auto subReg = subRegNumToBinNum(
                src.getDirRegRef().subRegNum, src.getDirRegName(), src.getType());
            encodeSrcSubRegNum<S>(subReg, true);
        }
        break;
    }
    case Operand::Kind::IMMEDIATE:
        if (S == SourceIndex::SRC1) {
            fatal("src1: immediate operand in ternary align1 must be src0 or src2");
            return;
        }
        encodeSrcRegFile<S>(GED_REG_FILE_IMM);
        if (platform() < Platform::GEN10) {
            encodeImmVal(src.getImmediateValue(), src.getType());
        } else {
            encodeTernaryImmVal<S>(src.getImmediateValue(), src.getType());
        }
        break;
    default:
        fatal("src%d: invalid operand kind", (int)S);
        return;
    }
}


void Encoder::encodeTernaryInstruction(
    const Instruction& inst,
    GED_ACCESS_MODE accessMode)
{
    if (accessMode == GED_ACCESS_MODE_Align1) {
        encodeTernaryAlign1Instruction(inst);
    } else {
        encodeTernaryAlign16Instruction(inst);
    }
}
void Encoder::encodeTernaryAlign16Instruction(const Instruction& inst)
{
    if (inst.getOpSpec().supportsDestination()) {
        encodeTernaryDestinationAlign16(inst);
    }
    encodeTernarySourceAlign16<SourceIndex::SRC0>(inst);
    encodeTernarySourceAlign16<SourceIndex::SRC1>(inst);
    encodeTernarySourceAlign16<SourceIndex::SRC2>(inst);
}
void Encoder::encodeTernaryAlign1Instruction(const Instruction& inst)
{
    // set ExecutionDataType (integral or floating)
    // the operands must be part of the same type set
    Type src0Type = inst.getSource(0).getType();
    GED_EXECUTION_DATA_TYPE execDataType;
    if (isTernaryAlign1Floating(src0Type)) {
        execDataType = GED_EXECUTION_DATA_TYPE_Float;
    } else if (isTernaryAlign1Integral(src0Type)) {
        execDataType = GED_EXECUTION_DATA_TYPE_Integer;
    } else {
        fatal("src0: unsupported type for ternary align1 encoding");
        return;
    }
    GED_ENCODE(ExecutionDataType, execDataType);

    if (inst.getOpSpec().supportsDestination()) {
        encodeTernaryDestinationAlign1(inst);
    }
    encodeTernarySourceAlign1<SourceIndex::SRC0>(inst);
    encodeTernarySourceAlign1<SourceIndex::SRC1>(inst);
    encodeTernarySourceAlign1<SourceIndex::SRC2>(inst);
}

void Encoder::encodeBranchingInstruction(const Instruction& inst)
{
    // the destination stride is always 1 for all control flow
    GED_ENCODE(DstHorzStride, 1);

    // control flow instructions require patching later if any operand is a label
    bool src0IsLabel = inst.getSource(0).getKind() == Operand::Kind::LABEL;

    // break up instructions into various classes
    //   - stuff with implicit operands: jmpi
    //   - stuff that can take register operands: call, calla, return
    //   - everything else: if, else, while, ..., goto, join, ...
    if (m_opcode == Op::JMPI)
    {
        // jmpi encodes the syntax
        //   jmpi (1) LABEL
        //   jmpi (1) reg32
        // as
        //   jmpi (1) ip ip LABEL
        //   jmpi (1) ip ip reg32
        //
        // "Restriction: The index data type must be D (Signed DWord Integer)."
        //
        // implicit IP ...
        encodeBasicDestination(inst, Operand::DST_REG_IP_UD);
        encodeBasicSource<SourceIndex::SRC0>(inst, Operand::SRC_REG_IP_UD);
        GED_ENCODE(Src1DataType, GED_DATA_TYPE_d);
        if (src0IsLabel) {
            // jmpi (1) LABEL   (encodes into Src1)
            GED_ENCODE(Src1RegFile, GED_REG_FILE_IMM);
        } else {
            // jmpi (1) reg32   (encodes into Src1)
            encodeBasicSource<SourceIndex::SRC1>(inst, inst.getSource(0));
        }
    }
    else if (m_opcode == Op::CALL ||
        m_opcode == Op::CALLA ||
        m_opcode == Op::RET)
    {
        // e.g. call, calla, ret
        //   call  (..)   imm32
        //   call  (..)   reg32
        //   calla (..)   imm32
        //   ret   (...)  reg32       => encodes as ret (...) null reg
        //
        if (m_opcode == Op::CALL || m_opcode == Op::CALLA) {
            encodeBasicDestination(inst, inst.getDestination());
        } else if (m_opcode == Op::RET) {
            encodeBasicDestination(inst, Operand::DST_REG_NULL_UD);
            encodeBasicSource<SourceIndex::SRC0>(inst, inst.getSource(0));
        }

        if (m_opcode == Op::CALL || m_opcode == Op::CALLA) {
            if (src0IsLabel) {
                // op == CALL (since it's a label), hence we have
                // call (..) imm32 => which uses src1
                GED_ENCODE(Src1RegFile, GED_REG_FILE_IMM);
                GED_ENCODE(Src1DataType, GED_DATA_TYPE_d);
            } else {
                // call  (..) reg32
                // calla (..) imm32
                encodeBasicSource<SourceIndex::SRC1>(inst, inst.getSource(0));
            }

            // <2;2,1> restriction for CALL and CALLA restriction is only for
            // IVB+HSW, but simulator has it until CNL.  So we have to support it
            // until we get CNL HW validation moves to it
            if (callNeedsSrcRegion221(inst)) {
                GED_ENCODE(Src0VertStride, 2);
                GED_ENCODE(Src0Width,      2);
                GED_ENCODE(Src0HorzStride, 1);
            }
        }
    } else if (m_opcode == Op::BRD || m_opcode == Op::BRC) {
        // [brd/brc]: The ip register must be used (for example, by the assembler) as dst.
        encodeBasicDestination(inst, Operand::DST_REG_IP_D);
        if (!src0IsLabel) {
            encodeBasicSource<SourceIndex::SRC0>(inst, inst.getSource(0));
        }
        GED_DATA_TYPE ty =
            platform() < Platform::GEN8 ? GED_DATA_TYPE_w : GED_DATA_TYPE_d;
        GED_ENCODE(Src0RegFile,
            src0IsLabel ? GED_REG_FILE_IMM : GED_REG_FILE_GRF);
        GED_ENCODE(Src0DataType, ty);
        // GED automatically sets?
        // if (m_opcode == Op::BRC && src0IsLabel) {
        //    GED_ENCODE(Src1DataType, ty);
        // }
        // if (m_opcode == Op::BRD && m_inst->getOpSpec().hasImplicitSrcRegion(0)) {
        //     encodeSrcRegion(SourceIndex::SRC0,
        //         m_inst->getOpSpec().implicitSrcRegion(0));
        // }
    } else {
        // regular control flow that only accepts immediate values
        // e.g. if, else, endif, while, cont, break, goto, join, halt

        // Apparently, the implicit destination on these instructions
        // is null instead of ip (unlike jmpi etc)
        // destination is ip<1>:ud
        //
        // encodeDestination(&Operand::DST_IP);
        encodeBasicDestination(inst, Operand::DST_REG_NULL_UD);

        //UIP
        if (m_opcode != Op::ENDIF &&
            m_opcode != Op::WHILE &&
            m_opcode != Op::JOIN)
        {
            // if/else/halt/brk/cont.... all require :d on operands
            GED_DATA_TYPE ty =
                platform() < Platform::GEN8 ? GED_DATA_TYPE_w : GED_DATA_TYPE_d;
            GED_ENCODE(Src0RegFile, GED_REG_FILE_IMM);
            GED_ENCODE(Src0DataType, ty);
        }
        //before gen12 don't need to set JIP for control flow instructions that have UIP
        //JIP
        if (m_opcode == Op::WHILE ||
            m_opcode == Op::ENDIF ||
            m_opcode == Op::JOIN)
        {
            GED_ENCODE(Src1RegFile, GED_REG_FILE_IMM);
            GED_ENCODE(Src1DataType, GED_DATA_TYPE_d);
        }
    }
}

void Encoder::encodeBranchingInstructionSimplified(const Instruction& inst)
{
    const OpSpec& instSpec = inst.getOpSpec();

    // set branch control
    if (instSpec.supportsBranchCtrl()) {
        GED_ENCODE(BranchCtrl, lowerBranchCntrl(inst.getBranchCtrl()));
    }

    // control flow instructions require patching later if any operand is a label
    const Operand &src0 = inst.getSource(0);
    bool src0IsLabel = src0.getKind() == Operand::Kind::LABEL;

    // for jmpi HW will take care of IP so don't need to encode it for dst/src0
    if (inst.getOpSpec().supportsDestination()) {
        encodeBranchDestination(inst.getDestination());
    } else {
        encodeBranchDestination(Operand::DST_REG_NULL_UD);
    }
    // regualar control flow that only accepts immediate values
    // e.g. if, else, endif, while, cont, break, goto, join, halt

    // encoding JIP
    if (src0IsLabel) {
        GED_ENCODE(Src0RegFile, GED_REG_FILE_IMM);
    } else {
        if (inst.getSource(0).getKind() == Operand::Kind::INDIRECT)
            error("Branch instructions do not support indirect register mode");
        encodeBranchSource(inst.getSource(0));
    }

    if (inst.getSourceCount() == 2) {
        // encoding UIP always IMM except for brc with a register argument
        if (inst.getOp() != Op::BRC || inst.getSource(0).isImm()) {
            GED_ENCODE(Src1RegFile, GED_REG_FILE_IMM);
        }
    }
}

void Encoder::encodeSendInstructionProcessSFID(const Instruction& inst)
{
    if (platform() >= Platform::GEN12P1) {
        auto gedSfid = lowerSFID(inst.getSendFc());
        GED_ENCODE(SFID, gedSfid);
    }
}

void Encoder::encodeSendInstruction(const Instruction& inst)
{
    const OpSpec& os = inst.getOpSpec();
    if (os.isSendFamily()) {
        encodeSendDestination(inst.getDestination());
        encodeSendSource0(inst.getSource(0));
        if (m_model.supportsUnifiedSend()) {
            encodeSendsSource1(inst.getSource(1));
        }
    } else if (os.isSendsFamily()) {
        encodeSendDestination(inst.getDestination());
        encodeSendsSource0(inst.getSource(0));
        encodeSendsSource1(inst.getSource(1));
    }

    const bool supportsExDescReg =
        os.isSendsFamily() ||
        m_model.supportsUnifiedSend();

    encodeSendInstructionProcessSFID(inst);

    // ex_desc
    SendDesc extMsgDesc = inst.getExtMsgDescriptor();
    if (extMsgDesc.isImm()) { // Exdesc.IsReg == false
        if (supportsExDescReg) {
            GED_ENCODE(ExDescRegFile, GED_REG_FILE_IMM);
        }
        bool stripEOT = m_model.supportsUnifiedSend();
        if (stripEOT) {
            extMsgDesc.imm = extMsgDesc.imm & ~(1 << 5);
        }

        GED_ENCODE(ExMsgDesc, extMsgDesc.imm);

    } else {  // ExDesc.IsReg == true
        if (!supportsExDescReg) {
            error("ex_desc register not supported on this platform for "
                "this instruction");
        }

        // Underneath GED API converts ExDescRegFile to
        // SetSelReg32ExDesc for sends
        GED_ENCODE(ExDescRegFile, GED_REG_FILE_ARF);
        // double the subregister because it's encoded as [3:1]
        GED_ENCODE(ExDescAddrSubRegNum, 2 * extMsgDesc.reg.subRegNum);

    }

    // desc
    bool hasImmDescRegFile = true;
    SendDesc msgDesc = inst.getMsgDescriptor();
    if (msgDesc.isImm()) {
        if (hasImmDescRegFile)
            GED_ENCODE(DescRegFile, GED_REG_FILE_IMM);
        GED_ENCODE(MsgDesc, msgDesc.imm);
    } else {
        if (platform() == Platform::GEN9) {
            uint32_t msgDescriptor = 0;
            // There is a HW bug on SKL where HW will only copy bits 0-28 from the
            // address register (descriptor register) and will miss bit 30 of
            // the descriptor.  Hence, even in the case of an register descriptor
            // we must program bit 30 as immediate (it will be taken from the
            // encoding and OR'd in correctly)
            // E.g. (old syntax)
            //   sends (8) r74:hf r16 r73 0x42:ud a0.0 {Align1, Q1, NoMask}
            //   // #??:$10:%13 // sampler, resLen=3, msgLen=1, extMsgLen=1
            // On SKL, HW will copy bits 29-31 from the actual immediate descriptor
            // Bits.  Hence, we must set immediate descriptor bit 30 even in the case
            // of a register descriptor! (For SKL).
            //
            // For 3D sampler bit 30 indicates HF/F return format.
            // For render target write bit 30 indicates HF/F input...
            // "Thankfully" for SKL 3D sampler doesn't support HF input.
            // For CNL it does, and that will be bit 29.
            // But this bug should be fixed in CNL.
            if (platform() == Platform::GEN9 && msgDesc.isReg()) {
                if (inst.getDestination().getType() == Type::HF ||
                    inst.getSource(0).getType() == Type::HF)
                {
                    msgDescriptor |= (1 << 30);
                }
            }
            GED_ENCODE(DescRegFile, GED_REG_FILE_IMM);
            GED_ENCODE(MsgDesc, msgDescriptor);
        }
        // underneath GED API converts this to SetSelReg32Desc for sends
        if (hasImmDescRegFile)
            GED_ENCODE(DescRegFile, GED_REG_FILE_ARF);
        // only send/sendc support a register here
        if (m_model.supportsUnarySend()) {
            uint8_t regNumBits;
            const RegInfo *ri = m_model.lookupRegInfoByRegName(RegName::ARF_A);
            IGA_ASSERT(ri, "failed to find a0 register");
            ri->encode((int)msgDesc.reg.regNum, regNumBits);
            GED_ENCODE(DescRegNum, regNumBits);
        }
    } // end else: desc


    bool hasFusion = platform() >= Platform::GEN12P1;
    if (hasFusion) {
        GED_ENCODE(FusionCtrl,
            inst.hasInstOpt(InstOpt::SERIALIZE) ?
                GED_FUSION_CTRL_Serialized : GED_FUSION_CTRL_Normal);
    }
} //end: encodeSendInstruction

void Encoder::encodeSyncInstruction(const Instruction& inst)
{
    // Set the Dst.HorStride to 1 so that "sync.bar null" can be compacted
    GED_ENCODE(DstHorzStride, 1);

    const Operand &src = inst.getSource(0);
    if (src.getKind() == Operand::Kind::IMMEDIATE) {
        encodeSrcRegFile<SourceIndex::SRC0>(GED_REG_FILE_IMM);
        encodeSrcType<SourceIndex::SRC0>(src.getType());
        encodeImmVal(src.getImmediateValue(), src.getType());
    } else {
            encodeSrcRegFile<SourceIndex::SRC0>(GED_REG_FILE_ARF);
    }
}

void Encoder::encodeBranchDestination(const Operand& dst) {
    GED_ENCODE(DstRegFile,
        lowerRegFile(dst.getDirRegName()));
    encodeDstReg(dst.getDirRegName(), dst.getDirRegRef().regNum);
    encodeDstSubRegNum(subRegNumToBinNum(
                         dst.getDirRegRef().subRegNum,
                         dst.getDirRegName(),
                         dst.getType()),
                       true);
}

void Encoder::encodeBasicDestination(
    const Instruction& inst,
    const Operand& dst,
    GED_ACCESS_MODE accessMode)
{
    IGA_ASSERT(accessMode != GED_ACCESS_MODE_Align16 ||
        m_model.supportsAlign16(),
        "Align16 not supported on this platform.");

    GED_ENCODE(DstRegFile,
        lowerRegFile(dst.getDirRegName()));
    switch (dst.getKind())
    {
    case Operand::Kind::DIRECT:
    case Operand::Kind::MACRO:
        GED_ENCODE(DstAddrMode, GED_ADDR_MODE_Direct);
        GED_ENCODE(DstDataType,
            lowerDataType(dst.getType()));
        if (inst.getOpSpec().supportsSaturation()) {
            GED_ENCODE(Saturate,
                lowerSaturate(dst.getDstModifier()));
        }
        // VVVVV   fallthrough  VVVVV
    default: break;
    }

    switch (dst.getKind())
    {
    case Operand::Kind::DIRECT:
        if (accessMode == GED_ACCESS_MODE_Align16) {
            if (dst.getRegion() != Region::DST1) {
                fatal("dst has inconvertible region for Align16 encoding");
                return;
            }
            if (isAlign16MathMacroRegisterCsrOperand(dst)) {
                // acc2.XXXX on BDW .. SKL is context save and restore
                // This is really mme0
                encodeDstReg(RegName::ARF_MME, 0);
                // on GEN8 and GEN9 all encode as acc2, but the mux varies
                // to distinguish which acc it really is.
                GED_DST_CHAN_EN chEn;
                switch (dst.getDirRegRef().regNum) {
                /// case 0: ... acc2 actually uses Align1!
                // old-style for acc2 would be:
                // mov(8) r113:ud acc2:ud  {NoMask} // acc2
                //
                // acc3-9 are Align16
                case 1: chEn = GED_DST_CHAN_EN_x;    break; // mme0/acc3 -> acc2.x (0001b)
                case 2: chEn = GED_DST_CHAN_EN_y;    break; // mme1/acc4 -> acc2.y (0010b)
                case 3: chEn = GED_DST_CHAN_EN_xy;   break;
                case 4: chEn = GED_DST_CHAN_EN_z;    break;
                case 5: chEn = GED_DST_CHAN_EN_xz;   break;
                case 6: chEn = GED_DST_CHAN_EN_yz;   break;
                case 7: chEn = GED_DST_CHAN_EN_xyzw; break; // mme7/acc9 -> acc2.xyzw (0111b)
                default: IGA_ASSERT_FALSE("unreachable");
                }
                GED_ENCODE(DstChanEn, chEn);
            } else {
                // normal align16 destination (this still might be a
                // CSR work around op if the src is "acc2")
                encodeDstReg(dst.getDirRegName(), dst.getDirRegRef().regNum);
                GED_ENCODE(DstChanEn, GED_DST_CHAN_EN_xyzw);
            }
            encodeDstSubRegNum(
                subRegNumToBinNum(dst.getDirRegRef().subRegNum, dst.getDirRegName(), dst.getType()),
                inst.getOpSpec().isTernary() || inst.getOpSpec().isBranching());
        } else { // Align1
            encodeDstReg(dst.getDirRegName(), dst.getDirRegRef().regNum);
            encodeDstSubRegNum(
                subRegNumToBinNum(dst.getDirRegRef().subRegNum, dst.getDirRegName(), dst.getType()),
                inst.getOpSpec().isTernary() || inst.getOpSpec().isBranching());
        }
        break;
    case Operand::Kind::MACRO:
        encodeDstReg(dst.getDirRegName(), dst.getDirRegRef().regNum);
        GED_ENCODE(DstMathMacroExt,
            lowerSpecialAcc(dst.getMathMacroExt()));
        if (accessMode == GED_ACCESS_MODE_Align1 &&
            m_model.supportsAlign16ImplicitAcc())
        {
            fatal("Align1 dst math macro unsupported on this platform.");
            return;
        }
        break;
    case Operand::Kind::INDIRECT:
        GED_ENCODE(DstAddrMode, GED_ADDR_MODE_Indirect);
        GED_ENCODE(DstDataType,
            lowerDataType(dst.getType()));
        if (inst.getOpSpec().supportsSaturation()) {
            GED_ENCODE(Saturate,
                lowerSaturate(dst.getDstModifier()));
        }
        GED_ENCODE(DstAddrImm, dst.getIndImmAddr());
        GED_ENCODE(DstAddrSubRegNum, dst.getIndAddrReg().subRegNum);
        break;
    default:
        IGA_ASSERT_FALSE("unsupported operand kind");
        break;
    }

    if (accessMode == GED_ACCESS_MODE_Align1) {
        auto dstRgn = dst.getRegion();
        if (inst.getOpSpec().hasImplicitDstRegion(inst.isMacro())) {
            auto dstRgnImpl = inst.getOpSpec().implicitDstRegion(inst.isMacro());
            if (dstRgn != dstRgnImpl) {
                warning("dst region should be %s", ToSyntax(dstRgnImpl).c_str());
            }
        }
        GED_ENCODE(DstHorzStride, lowerRegionHorz(dstRgn.getHz()));
    }
}

static void createChSelForCtxSavRst(
    GED_SWIZZLE *chSel,
    GED_SWIZZLE x,
    GED_SWIZZLE y)
{
    // following IsaAsm rules here
    // reg.ab expands to reg.abbb
    chSel[0] = x;
    chSel[1] = chSel[2] = chSel[3] = y;
}


void Encoder::encodeBranchSource(const Operand& src)
{
    encodeSrcRegFile<SourceIndex::SRC0>(lowerRegFile(src.getDirRegName()));
    encodeSrcReg<SourceIndex::SRC0>(src.getDirRegName(),src.getDirRegRef().regNum);
    auto subReg = subRegNumToBinNum(
        src.getDirRegRef().subRegNum, src.getDirRegName(), Type::D);
    encodeSrcSubRegNum<SourceIndex::SRC0>(subReg, true);
}

template <SourceIndex S>
void Encoder::encodeBasicSource(
    const Instruction& inst,
    const Operand& src,
    GED_ACCESS_MODE accessMode)
{
    // setting the reg file must precede  must precede setting the type in GED
    switch (src.getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::MACRO:
    case Operand::Kind::INDIRECT:
        encodeSrcRegFile<S>(
            lowerRegFile(src.getDirRegName()));
        if (inst.getOpSpec().supportsSourceModifiers()) {
            encodeSrcModifier<S>(src.getSrcModifier());
        } else if (src.getSrcModifier() != SrcModifier::NONE) {
            // better be invalid in the IR if unsupported
            error("src%d source modifier not supported (invalid IR)",  (int)S);
        }
        break;
    case Operand::Kind::IMMEDIATE:
        encodeSrcRegFile<S>(GED_REG_FILE_IMM);
        break;
    default:
        break;
    }

    encodeSrcType<S>(src.getType());

    switch (src.getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::MACRO: {
        encodeSrcAddrMode<S>(GED_ADDR_MODE_Direct);
        if (src.getKind() == Operand::Kind::DIRECT) {
            if (isAlign16MathMacroRegisterCsrOperand(src)) {
                // BDW..SKL context save and restore of acc3...acc9
                // encode as acc2.####, ChSel will be changed in regioning code
                // recall acc2 is remapped to mme0
                encodeSrcReg<S>(RegName::ARF_MME, 0);
            } else {
                encodeSrcReg<S>(src.getDirRegName(), src.getDirRegRef().regNum);
                auto subReg = subRegNumToBinNum(
                    src.getDirRegRef().subRegNum,
                    src.getDirRegName(),
                    src.getType());
                encodeSrcSubRegNum<S>(subReg,
                    inst.getOpSpec().isTernary() || inst.getOpSpec().isBranching());
            }
        } else { // (src.getKind() == Operand::Kind::MACRO)
            encodeSrcReg<S>(RegName::GRF_R,src.getDirRegRef().regNum);
            encodeSrcMathMacroReg<S>(src.getMathMacroExt());
            if (accessMode == GED_ACCESS_MODE_Align16) {
                // vertical stride has to be halved for 8B types
                if (src.getType() == Type::DF) {
                    encodeSrcRegionVert<S>(Region::Vert::VT_2);
                } else {
                    encodeSrcRegionVert<S>(Region::Vert::VT_4);
                }
            }
        }
        break;
    }
    case Operand::Kind::INDIRECT:
        encodeSrcAddrMode<S>(GED_ADDR_MODE_Indirect);
        encodeSrcAddrImm<S>(src.getIndImmAddr());
        encodeSrcAddrSubRegNum<S>(src.getIndAddrReg().subRegNum);
        break;
    case Operand::Kind::IMMEDIATE:
        encodeImmVal(src.getImmediateValue(), src.getType());
        break;
    default:
        // support mov label
        if (static_cast<int>(S) == 0 && inst.isMovWithLabel()) {
            GED_ENCODE(Src0RegFile, GED_REG_FILE_IMM);
        } else {
            fatal("src%d: unsupported source operand kind (malformed IR)", (int)S);
            return;
        }
        break;
    }

    // sets stuff found in all register accesses (not macros)
    //   - region
    switch (src.getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::INDIRECT:
        if (accessMode == GED_ACCESS_MODE_Align16) {
            // r13.0<4>.xyzw is the only supported ChEn
            //      ^^^
            encodeSrcRegionVert<S>(Region::Vert::VT_4);
            GED_SWIZZLE chSel[4] =
                {GED_SWIZZLE_x, GED_SWIZZLE_y, GED_SWIZZLE_z, GED_SWIZZLE_w};
            if (isAlign16MathMacroRegisterCsrOperand(src)) {
                // context save and restore workaround on GEN8 and GEN9
                switch (src.getDirRegRef().regNum) {
                case 1: // acc2.yx = mme1 (acc3)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_y, GED_SWIZZLE_x);
                    break;
                case 2: // acc2.zx = mme2 (acc4)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_z, GED_SWIZZLE_x);
                    break;
                case 3: // acc2.wx = mme3 (acc5)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_w, GED_SWIZZLE_x);
                    break;
                case 4: // acc2.xy = mme4 (acc6)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_x, GED_SWIZZLE_y);
                    break;
                case 5: // acc2.yy = mme5 (acc7)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_y, GED_SWIZZLE_y);
                    break;
                case 6: // acc2.zy = mme6 (acc8)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_z, GED_SWIZZLE_y);
                    break;
                case 7: // acc2.wy = mme7 (acc9)
                    createChSelForCtxSavRst(chSel, GED_SWIZZLE_w, GED_SWIZZLE_y);
                    break;
                }
            } else {
                // normal Align16 that we are converting to Align1
                if (src.getRegion() != Region::SRC110 &&
                    // supports legacy bits that may use <K;K,1> for "block"
                    // access; this allows us to assemble/reassemble similar bits
                    src.getRegion() != Region::SRC221 &&
                    src.getRegion() != Region::SRC441 &&
                    src.getRegion() != Region::SRC881 &&
                    src.getRegion() != Region::SRCFF1)
                {
                    fatal("src%d: unsupported region for translation to align16 encoding", (int)S);
                    return;
                }
                // TODO: we could permit SIMD4 with .x to mean broadcast read
                // of subreg 0, but I don't think any System Routine code uses
                // this.
                //
                // NOTE: technically we could convert
                //   r13.0<0>.xxxx to r13.0<0;1,0>
                //   r13.0<0>.yyyy to r13.1<0;1,0>
                //   r13.0<0>.zzzz to r13.2<0;1,0>
                //   r13.0<0>.wwww to r13.3<0;1,0>
                // Also be sure to handle stuff like:
                //   r13.4<0>.zzzz (would be r13.7<0;1,0>)
                //
                // Let's wait until we need this though.
            }
            encodeSrcChanSel<S>(chSel[0], chSel[1], chSel[2], chSel[3]);
        } else { // Align1
            bool hasRgnWi = true;
            encodeSrcRegion<S>(src.getRegion(), hasRgnWi);
        }
        break;
    case Operand::Kind::MACRO:
        if (accessMode == GED_ACCESS_MODE_Align1) {
            encodeSrcRegion<S>(src.getRegion());
        } // else {align16 macros use the regioning bits, don't clobber them}
        break;
    default:
        break;
    }
}

void Encoder::encodeSendDirectDestination(const Operand& dst)
{
    if (platform() >= Platform::GEN12P1) {
        //auto t = dst.getType() == Type::INVALID ? Type::UD : dst.getType();
        //GED_ENCODE(DstDataType, lowerDataType(t));
        GED_ENCODE(DstRegNum, dst.getDirRegRef().regNum);
    } else {
        auto t = dst.getType() == Type::INVALID ? Type::UD : dst.getType();
        GED_ENCODE(DstDataType, lowerDataType(t));

        //GED_ENCODE(Saturate, lowerSaturate(dst->getDstModifier()));
        if (m_opcode != Op::SENDS && m_opcode != Op::SENDSC) {
            GED_ENCODE(DstHorzStride, static_cast<uint32_t>(dst.getRegion().getHz())); // not used for sends
        }

        GED_ENCODE(DstRegNum, dst.getDirRegRef().regNum);
        // GED_ENCODE(DstSubRegNum,
        //    SubRegToBytesOffset(dst.getDirRegRef().subRegNum, RegName::GRF_R, dst.getType()));
    }
}

void Encoder::encodeSendDestinationDataType(const Operand& dst)
{
    if (platform() >= Platform::GEN12P1)
        return;

    auto t = dst.getType() == Type::INVALID ? Type::UD : dst.getType();
    GED_ENCODE(DstDataType, lowerDataType(t));
}

void Encoder::encodeSendDestination(const Operand& dst)
{
    if (m_model.supportsUnarySend()) {
        switch (dst.getKind())
        {
        case Operand::Kind::DIRECT:
            GED_ENCODE(DstAddrMode, GED_ADDR_MODE_Direct);
            break;
        case Operand::Kind::INDIRECT:
            GED_ENCODE(DstAddrMode, GED_ADDR_MODE_Indirect);
            break;
        default:
            fatal("dst: unsupported destination operand kind/addrMode (malformed IR)");
            return;
        }
    }

    GED_ENCODE(DstRegFile,
        lowerRegFile(dst.getDirRegName()));

    if (dst.getKind() ==  Operand::Kind::DIRECT) {
        encodeSendDirectDestination(dst);
    } else if (dst.getKind() ==  Operand::Kind::INDIRECT) {
        encodeSendDestinationDataType(dst);
        if (m_opcode != Op::SENDS && m_opcode != Op::SENDSC) {
            GED_ENCODE(DstHorzStride, static_cast<uint32_t>(dst.getRegion().getHz())); // not used for sends
        }
        GED_ENCODE(DstAddrImm, dst.getIndImmAddr());
        GED_ENCODE(DstAddrSubRegNum, dst.getIndAddrReg().subRegNum);
    }
}

void Encoder::encodeSendSource0(const Operand& src)
{
    if (m_model.supportsUnarySend()) {
        switch( src.getKind() )
        {
        case Operand::Kind::DIRECT:
            GED_ENCODE(Src0AddrMode, GED_ADDR_MODE_Direct);
            break;
        case Operand::Kind::INDIRECT:
            GED_ENCODE(Src0AddrMode, GED_ADDR_MODE_Indirect);
            break;
        default:
            fatal("src%d: unsupported source operand kind/addrMode (malformed IR)", (int)0);
            return;
            break;
        }
    }

    GED_REG_FILE gedRegFile = lowerRegFile(src.getDirRegName());
    GED_ENCODE(Src0RegFile, gedRegFile);

    auto t = src.getType() == Type::INVALID ? Type::UD : src.getType();

    if (src.getKind() ==  Operand::Kind::DIRECT)
    {
        if (m_model.supportsUnifiedSend()){
            GED_ENCODE(Src0RegNum, src.getDirRegRef().regNum);
        } else {
            GED_ENCODE(Src0DataType, lowerDataType(t));
            GED_ENCODE(Src0RegNum,    src.getDirRegRef().regNum);
            GED_ENCODE(Src0SubRegNum, src.getDirRegRef().subRegNum);
        }
    }
    else if (src.getKind() ==  Operand::Kind::INDIRECT)
    {
        GED_ENCODE(Src0DataType, lowerDataType(t));
        GED_ENCODE(Src0AddrSubRegNum, src.getIndAddrReg().subRegNum);
        GED_ENCODE(Src0AddrImm, src.getIndImmAddr());
    }
}

// The sends opCode exists on gen9+.  There is no sends opcode on pre-gen9.
// Starting from gen12, send opcode can have two sources, so the sends opcode
// is not needed.

void Encoder::encodeSendsSource0(const Operand& src)
{
    // "...for sends/sendsc instructions Src0.SrcMod, ... and Src0.SrcType are not used."
    // "Src0.RegFile[1], Src1.RegFile[1] are implicitly set to 0,
    //  and Src0.RegFile[0] is implicitly set as 1 for sends/sendsc instructions."
    switch (src.getKind())
    {
    case Operand::Kind::DIRECT:
        GED_ENCODE(Src0AddrMode, GED_ADDR_MODE_Direct);
        break;
    case Operand::Kind::INDIRECT:
        GED_ENCODE(Src0AddrMode, GED_ADDR_MODE_Indirect);
        break;
    default:
        fatal("src%d: unsupported source operand kind/addrMode (malformed IR)", (int)0);
        return;
        break;
    }

    if (src.getKind() ==  Operand::Kind::DIRECT)
    {
        GED_ENCODE(Src0RegNum,    src.getDirRegRef().regNum);
        GED_ENCODE(Src0SubRegNum, src.getDirRegRef().subRegNum);
    }
    else if (src.getKind() ==  Operand::Kind::INDIRECT)
    {
        auto immAddr = src.getIndImmAddr();
        GED_ENCODE(Src0AddrImm, immAddr);
        GED_ENCODE(Src0AddrSubRegNum, src.getIndAddrReg().subRegNum);
    }
}


void Encoder::encodeSendsSource1(const Operand& src)
{
    //GED_ENCODE(Src1AddrMode, GED_ADDR_MODE_Direct);
    GED_REG_FILE gedRegFile = lowerRegFile(src.getDirRegName());
    GED_ENCODE(Src1RegFile, gedRegFile);
    GED_ENCODE(Src1RegNum, src.getDirRegRef().regNum);
}

void Encoder::encodeSendsDestination(const Operand& dst)
{
    GED_ENCODE(DstAddrMode, GED_ADDR_MODE_Direct);
    GED_ENCODE(DstRegFile, lowerRegFile(dst.getDirRegName()));
    // send types use :ud where possible
    auto t = dst.getType() == Type::INVALID ? Type::UD : dst.getType();
    GED_ENCODE(DstDataType, lowerDataType(t));

    //GED_ENCODE(Saturate, lowerSaturate(dst->getDstModifier()));
    //GED_ENCODE(DstHorzStride, static_cast<uint32_t>(dst->getHz()));

    GED_ENCODE(DstRegNum, dst.getDirRegRef().regNum);
    // TODO: set correct regType
    encodeDstSubRegNum(
        subRegNumToBinNum(
            dst.getDirRegRef().subRegNum, RegName::GRF_R, dst.getType()), true);
}

template <SourceIndex S>
void Encoder::encodeTernarySourceAlign16(const Instruction& inst)
{
    // PreCNL Align16
    // GRF-only
    encodeSrcAddrMode<S>(GED_ADDR_MODE_Direct);

    const Operand& src = inst.getSource(S);

    if (inst.getOpSpec().supportsSourceModifiers()) {
        encodeSrcModifier<S>(src.getSrcModifier());
    }

    // set the data type
    GED_DATA_TYPE gedType = lowerDataType(src.getType());
    if (S == SourceIndex::SRC0) {
        GED_ENCODE(SrcDataType, gedType);
    } else {
        const Operand &src0 = inst.getSource(SourceIndex::SRC0);
        bool src0IsFloating = src0.getType() == Type::F || src0.getType() == Type::HF;
        if (platform() >= Platform::GEN8LP && src0IsFloating) {
            bool srcNIsFloating = src.getType() == Type::F || src.getType() == Type::HF;
            if (src0IsFloating && srcNIsFloating) {
                encodeSrcType<S>(src.getType());
            } else {
                fatal("src%d: mixed types require :f and :hf (or vice versa)", (int)S);
                return;
            }
        }
    }

    if (!inst.isMacro()) {
        const Region& rgn = src.getRegion();
        const RegRef& reg = src.getDirRegRef();
        //Adjusting sub register when going from align1 to align 16 representation.
        //in align 16 subregister is always 16 byte alligned, but we can play with swizzle to access none aligned sub register
        uint16_t subRegNumber = reg.subRegNum;
        //mad (8) r46.0.xyzw:df r46.0.xyzw:df r50.0.xyzw:df r48.0.xyzw:df {Align16, Q1} // #??:$66:%66 {0=EL, 1=EL, 2=EL, BC=BAD}
        //mad (2) r5.0.xy:df r5.0.xyxy:df r92.2.xyxy:df r93.0.xyxy:df {Align16, Q1, NoMask} // #??:$988:%988 {0=OL, 1=EH, 2=OH, BC=GOOD} BDW,SKL
        if (S != SourceIndex::SRC2) {
            if (rgn == Region::SRC8X1 ||
                rgn == Region::SRC4X1 ||
                rgn == Region::SRC2X1) {
                encodeSrcRepCtrl<S>(GED_REP_CTRL_NoRep);
                encodeSrcChanSel<S>(GED_SWIZZLE_x, GED_SWIZZLE_y, GED_SWIZZLE_z, GED_SWIZZLE_w);
            } else if (rgn == Region::SRC0X0) {
                if (src.getType() == Type::DF) {
                    if (reg.subRegNum % 2 == 0) {
                        encodeSrcChanSel<S>(GED_SWIZZLE_x, GED_SWIZZLE_y, GED_SWIZZLE_x, GED_SWIZZLE_y);
                    } else {
                        encodeSrcChanSel<S>(GED_SWIZZLE_z, GED_SWIZZLE_w, GED_SWIZZLE_z, GED_SWIZZLE_w);
                        subRegNumber -= 1;
                    }
                } else {
                    encodeSrcRepCtrl<S>(GED_REP_CTRL_Rep);
                }
            } else {
                fatal("src%d: unsupported region for Align16 encoding", (int)S);
                return;
            }
        } else {
            if (rgn == Region::SRCXX1) {
                encodeSrcRepCtrl<S>(GED_REP_CTRL_NoRep);
                encodeSrcChanSel<S>(GED_SWIZZLE_x, GED_SWIZZLE_y, GED_SWIZZLE_z, GED_SWIZZLE_w);
            } else if (rgn == Region::SRCXX0) {
                if (src.getType() == Type::DF) {
                    if (src.getDirRegRef().subRegNum % 2 == 0) {
                        encodeSrcChanSel<S>(GED_SWIZZLE_x, GED_SWIZZLE_y, GED_SWIZZLE_x, GED_SWIZZLE_y);
                    } else {
                        encodeSrcChanSel<S>(GED_SWIZZLE_z, GED_SWIZZLE_w, GED_SWIZZLE_z, GED_SWIZZLE_w);
                        subRegNumber -= 1;
                    }
                } else {
                    encodeSrcRepCtrl<S>(GED_REP_CTRL_Rep);
                }
            }
            else if (rgn == Region::SRC0X0 && src.getType() == Type::DF) {
                encodeSrcChanSel<S>(GED_SWIZZLE_x, GED_SWIZZLE_y, GED_SWIZZLE_x, GED_SWIZZLE_y);
            }
            else {
                fatal("src%d: unsupported region for Align16 encoding", (int)S);
                return;
            }
        }
        uint32_t regNum = reg.regNum;
        encodeSrcReg<S>(RegName::GRF_R, (uint16_t)regNum);
        auto subReg =
            subRegNumToBinNum(subRegNumber, src.getDirRegName(), src.getType());
        encodeSrcSubRegNum<S>(subReg, true);
    } else {
        // implicit operand accumulator
        // e.g. madm (4) ... -r14.acc3
        encodeSrcReg<S>(RegName::GRF_R,src.getDirRegRef().regNum);
        encodeSrcMathMacroReg<S>(src.getMathMacroExt());
    }
}

void Encoder::encodeTernaryDestinationAlign16(const Instruction& inst)
{
    const Operand& dst = inst.getDestination();
    if (inst.getOpSpec().supportsSaturation()) {
        GED_ENCODE(Saturate,
            lowerSaturate(dst.getDstModifier()));
    }
    GED_ENCODE(DstDataType, lowerDataType(dst.getType()));
    if (dst.getDirRegName() != RegName::GRF_R) {
        fatal("align16 ternary dst must be to GRF");
        return;
    }

    // register / info (must be GRF)
    GED_ENCODE(DstRegFile, lowerRegFile(dst.getDirRegName()));
    uint32_t regNum = dst.getDirRegRef().regNum;
    GED_ENCODE(DstRegNum, regNum);
    if (inst.isMacro()) {
        // macro only
        GED_DST_CHAN_EN chanEn = mathMacroRegToChEn(dst.getMathMacroExt());
        GED_ENCODE(DstChanEn, chanEn);
    } else {
        // Align16 instruction (we must convert from Align1)
        //
        // As long as the Align1 sequences are packed (.xyzw), this is
        // straightforward.  However, "scalar" (braoadcast) sequences are
        // a bit harder as we must carefully choose the ChEn based on the
        // subregister that would be used in Align1
        // (See also Decoder::decodeDestinationTernaryAlign16)
        GED_DST_CHAN_EN chanEn = GED_DST_CHAN_EN_xyzw;
        auto reg = dst.getDirRegRef();
        if (inst.getExecSize() == ExecSize::SIMD1) {
            // SIMD1 MAD is not allowed, so MDF (and IGC) are generating use
            // SIMD4 and SIMD2 with specific channel masks to selectively
            // enable just the bottom channel.
            if (dst.getType() == Type::DF) {
                // For 64-bit types we use a mad (2) ...
                // Note, only :df is needed since :q and :uq are not supported
                //
                // e.g. mad (2)  r5.0.xy:df     ... {Align16, Q1, NoMask} //
                if (dst.getDirRegRef().subRegNum % 2 == 0) {
                    chanEn = GED_DST_CHAN_EN_xy;
                } else {
                    // e.g. mad (1) r5.1<1>:df
                    //  encodes as
                    //      mad (2) r5.0.zw:df
                    //           ^ SIMD2 and .zw (~= .1)
                    chanEn = GED_DST_CHAN_EN_zw;
                    reg.subRegNum -= 1;
                }
            } else {
                // 32-bit or 16-bit type (:hf).  We use a SIMD4
                //
                // one channel enabled. E.g. we'll parse
                //   mad (1|M0)  r53.6<1>:f  ...
                // and encode it as
                //   mad (4)     r53.4.z:f
                //        ^ SIMD4    ^^^ aligned subreg .4.z == subreg .6:f Align1
                switch (reg.subRegNum % 4) {
                case 0: chanEn = GED_DST_CHAN_EN_x; break;
                case 1: chanEn = GED_DST_CHAN_EN_y; break;
                case 2: chanEn = GED_DST_CHAN_EN_z; break;
                case 3: chanEn = GED_DST_CHAN_EN_w; break;
                }
                // align the subregister
                reg.subRegNum -= (reg.subRegNum % 4);
            }
        }
        GED_ENCODE(DstChanEn, chanEn);
        encodeDstSubRegNum(subRegNumToBinNum(
            reg.subRegNum, dst.getDirRegName(), dst.getType()), true);
    }
}

void Encoder::encodeDstReg(RegName regName, uint16_t regNum)
{
    // encodes ARF or GRF
    uint32_t gedBits = translateRegNum(-1, regName, regNum);
    GED_ENCODE(DstRegNum, gedBits);
}


void Encoder::encodeImmVal(const ImmVal &val, Type type) {
    GED_ENCODE(Imm, typeConvesionHelper(val, type));
}

template <SourceIndex S>
void Encoder::encodeSrcRepCtrl(GED_REP_CTRL rep)
{
    if (S == SourceIndex::SRC0) {
        GED_ENCODE(Src0RepCtrl, rep);
    } else if (S == SourceIndex::SRC1) {
        GED_ENCODE(Src1RepCtrl, rep);
    } else {
        GED_ENCODE(Src2RepCtrl, rep);
    }
}

template <SourceIndex S> void Encoder::encodeSrcChanSel(
    GED_SWIZZLE chSelX,
    GED_SWIZZLE chSelY,
    GED_SWIZZLE chSelZ,
    GED_SWIZZLE chSelW)
{
    uint32_t chSelBits =
        createChanSel(chSelX, chSelY, chSelZ, chSelW);
    if (S == SourceIndex::SRC0) {
        GED_ENCODE(Src0ChanSel, chSelBits);
    } else if (S == SourceIndex::SRC1) {
        GED_ENCODE(Src1ChanSel, chSelBits);
    } else {
        GED_ENCODE(Src2ChanSel, chSelBits);
    }
}

uint32_t Encoder::translateRegNum(
    int opIx, RegName regName, uint16_t regNum)
{
    uint8_t regNumBits = 0;

    const char *whichOp =
        opIx == 0 ? "src0" :
            opIx == 1 ? "src1" :
            opIx == 2 ? "src2" :
            "dst";

    const RegInfo *ri = m_model.lookupRegInfoByRegName(regName);
    if (ri == nullptr) {
        error("%s: invalid register name for this platform", whichOp);
    } else if (!ri->isRegNumberValid((int)regNum)) {
        error("%s: %s%d number out of range", whichOp, ri->syntax, regNum);
    } else {
        ri->encode((int)regNum, regNumBits);
    }
    return regNumBits; // widen for GED
}

uint32_t Encoder::mathMacroRegToBits(int src, MathMacroExt implAcc) {
    uint32_t bits = 8; // NOACC
    switch (implAcc) {
    /// or 00000000b (GEN11)
    case MathMacroExt::MME0:  bits = 0; break; // 0000b
    case MathMacroExt::MME1:  bits = 1; break;
    case MathMacroExt::MME2:  bits = 2; break;
    case MathMacroExt::MME3:  bits = 3; break;
    case MathMacroExt::MME4:  bits = 4; break;
    case MathMacroExt::MME5:  bits = 5; break;
    case MathMacroExt::MME6:  bits = 6; break;
    case MathMacroExt::MME7:  bits = 7; break;
    /// or 00008000b (GEN11)
    case MathMacroExt::NOMME: bits = 8; break; // 1000b
    default:
        if (src < 0) {
            fatal("dst operand has invalid math macro register");
        } else {
            fatal("src%d operand has invalid math macro register", src);
        }
        return bits;
    }
    return bits;
}
GED_DST_CHAN_EN Encoder::mathMacroRegToChEn(MathMacroExt implAcc) {
    GED_DST_CHAN_EN bits = GED_DST_CHAN_EN_w; // NOACC
    switch (implAcc) {
    case MathMacroExt::MME0:   bits = GED_DST_CHAN_EN_None; break; // 0000b
    case MathMacroExt::MME1:   bits = GED_DST_CHAN_EN_x;    break;
    case MathMacroExt::MME2:   bits = GED_DST_CHAN_EN_y;    break;
    case MathMacroExt::MME3:   bits = GED_DST_CHAN_EN_xy;   break;
    case MathMacroExt::MME4:   bits = GED_DST_CHAN_EN_z;    break; // 0100b
    case MathMacroExt::MME5:   bits = GED_DST_CHAN_EN_xz;   break;
    case MathMacroExt::MME6:   bits = GED_DST_CHAN_EN_yz;   break;
    case MathMacroExt::MME7:   bits = GED_DST_CHAN_EN_xyz;  break;
    case MathMacroExt::NOMME:  bits = GED_DST_CHAN_EN_w;    break; // 1000b
    default: fatal("operand has invalid math macro register");
    }
    return bits;
}

void Encoder::encodeOptionsThreadControl(const Instruction& inst)
{
    if (inst.hasInstOpt(InstOpt::NOPREEMPT)) {
        if (m_model.supportsNoPreempt()) {
            GED_ENCODE(ThreadCtrl, GED_THREAD_CTRL_NoPreempt);
        }
        else {
            warning("NoPreempt not supported on this platform (dropping)");
        }
    }
}

// Translate from subRegNum to num represented in binary encoding
std::pair<bool, uint32_t> Encoder::subRegNumToBinNum(
    int subRegNum, RegName regName, Type type)
{
    return std::make_pair(true, SubRegToBytesOffset(subRegNum, regName, type));
}

void Encoder::encodeDstSubRegNum(
    std::pair<bool, uint32_t> subReg, bool isTernaryOrBranch)
{
    GED_ENCODE(DstSubRegNum, subReg.second);
}

void Encoder::encodeOptions(const Instruction& inst)
{
    GED_ENCODE(DebugCtrl,
        inst.hasInstOpt(InstOpt::BREAKPOINT) ?
            GED_DEBUG_CTRL_Breakpoint : GED_DEBUG_CTRL_Normal);

    if (inst.hasInstOpt(InstOpt::EOT))
    {
        GED_ENCODE(EOT, GED_EOT_EOT);
    }

    auto &os = inst.getOpSpec();
    if (os.supportsDepCtrl()) {
        if (inst.hasInstOpt(InstOpt::NODDCHK) &&
            !inst.hasInstOpt(InstOpt::NODDCLR))
        {
            GED_ENCODE(DepCtrl, GED_DEP_CTRL_NoDDChk);
        }
        else if (!inst.hasInstOpt(InstOpt::NODDCHK) &&
                  inst.hasInstOpt(InstOpt::NODDCLR))
        {
            GED_ENCODE(DepCtrl, GED_DEP_CTRL_NoDDClr);
        }
        else if (inst.hasInstOpt(InstOpt::NODDCHK) &&
                 inst.hasInstOpt(InstOpt::NODDCLR))
        {
            GED_ENCODE(DepCtrl, GED_DEP_CTRL_NoDDClr_NoDDChk);
        }
        else if (!inst.getOpSpec().isSendOrSendsFamily() && inst.getOp() != Op::NOP)
        {
            GED_ENCODE(DepCtrl, GED_DEP_CTRL_Normal);
        }
    }

    if (inst.hasInstOpt(InstOpt::ATOMIC))
    {
        GED_ENCODE(ThreadCtrl, GED_THREAD_CTRL_Atomic);
    }

    if (inst.hasInstOpt(InstOpt::SWITCH) && m_model.supportsHwDeps())
    {
        if (inst.getOp() == Op::NOP) {
            warning("nop doesn't support Switch option (dropping)");
        } else {
            GED_ENCODE(ThreadCtrl, GED_THREAD_CTRL_Switch);
        }
    }
    encodeOptionsThreadControl(inst);

    if (!inst.hasInstOpt(InstOpt::ATOMIC) &&
        !inst.hasInstOpt(InstOpt::SWITCH) &&
        !inst.hasInstOpt(InstOpt::NOPREEMPT) &&
        !inst.getOpSpec().isSendOrSendsFamily() &&
        inst.getOp() != Op::NOP)
    {
        GED_ENCODE(ThreadCtrl, GED_THREAD_CTRL_Normal);
    }

    if (inst.hasInstOpt(InstOpt::NOSRCDEPSET))
    {
        GED_ENCODE(NoSrcDepSet, GED_NO_SRC_DEP_SET_NoSrcDepSet);
    }
    else if (inst.getOpSpec().isSendOrSendsFamily() &&
             m_model.supportNoSrcDepSet())
    {
        GED_ENCODE(NoSrcDepSet, GED_NO_SRC_DEP_SET_Normal);
    }

    if (platform() >= Platform::GEN12P1 && m_opcode != Op::ILLEGAL) {
        SWSB::InstType inst_type = SWSB::InstType::OTHERS;

        if (inst.getOpSpec().isSendOrSendsFamily())
            inst_type = SWSB::InstType::SEND;
        else if (inst.is(Op::MATH))
            inst_type = SWSB::InstType::MATH;
        uint32_t swsb_binary = inst.getSWSB().encode(
                                    m_opts.swsbEncodeMode, inst_type);

        assert(inst.getSWSB().verify(m_opts.swsbEncodeMode, inst_type));
        GED_ENCODE(SWSB, swsb_binary);
    }

}


void Encoder::patchJumpOffsets()
{
    for (JumpPatch &jp : m_needToPatch)
    {
        const Instruction *inst = jp.inst;
        IGA_ASSERT(
            inst->getOpSpec().isBranching() || inst->isMovWithLabel(),
            "patching non-control-flow/non-mov instruction");

        // on some platforms jmpi os post-increment
        uint32_t jmpiExtraOffset = 0;
        bool isPostIncrementJmpi =
            inst->getOp() == Op::JMPI && !m_model.supportsSimplifiedBranches();
        if (isPostIncrementJmpi) {
            // jmpi is relative to the incremented PC, hence we must add
            // the size of the instruction here.  jmpi probably will never
            // compact, but we'll be careful here
            jmpiExtraOffset = inst->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
            IGA_ASSERT(inst->getSource(0).getKind() == Operand::Kind::LABEL,
                "patching non label op");
            // skip registers
        }

        // calla and mov is an absolute offset
        uint32_t encodePC =
            (inst->getOpSpec().isJipAbsolute()) || (inst->getOp() == Op::MOV) ?
            0 : getEncodedPC(inst);

        uint32_t jumpPC = 0;
        const Block *jipBlk = inst->getJIP();
        if (jipBlk == nullptr) {
            // immediate offset: we have to treat this as a relative offset
            jumpPC = inst->getSource(0).getImmediateValue().s32 + encodePC;
        } else if (!getBlockOffset(jipBlk, jumpPC)) {
            // For call, its target symbol may not be resolvable until in the
            // link stage when other kernels are available.
            if (inst->getOp() != Op::CALL && inst->getOp() != Op::CALLA) {
                fatalAt(inst->getLoc(), "jip label invalid");
            }
        }

        int32_t jip = jumpPC - encodePC - jmpiExtraOffset;
        // JIP and UIP are in QWORDS for most ops on PreBDW
        int32_t pcUnscale = arePcsInQWords(inst->getOpSpec()) ? 8 : 1;

        if (inst->isMovWithLabel()) {
            // encode mov label
            GED_DATA_TYPE src0_ty = lowerDataType(inst->getSource(0).getType());
            GED_ENCODE_TO(Src0DataType, src0_ty, &jp.gedInst);
            GED_ENCODE_TO(Imm, jip, &jp.gedInst);
        } else {
            // encode other branch instructions
            GED_ENCODE_TO(JIP, jip / pcUnscale, &jp.gedInst);
        }

        if (inst->getSourceCount() == 2 &&
            (inst->getOp() != Op::BRC || inst->getSource(1).isImm()))
        {
            // No need to set src1 regFile and type,
            // it will be over written by UIP
            const Block *uipBlk = inst->getUIP();
            if (uipBlk == nullptr) {
                jumpPC = inst->getSource(1).getImmediateValue().s32 + encodePC;
            } else if (!getBlockOffset(uipBlk, jumpPC)) {
                fatalAt(inst->getLoc(), "uip label invalid");
            }
            encodePC = getEncodedPC(inst);
            int32_t uip = jumpPC - encodePC;
            GED_ENCODE_TO(UIP, uip/pcUnscale, &jp.gedInst);
        }

        // re-encode branch
        START_GED_TIMER()
        GED_RETURN_VALUE status = GED_EncodeIns(&jp.gedInst,
            inst->hasInstOpt(InstOpt::COMPACTED) ?
                GED_INS_TYPE_COMPACT : GED_INS_TYPE_NATIVE,
            jp.bits);
        STOP_GED_TIMER()
        if (status != GED_RETURN_VALUE_SUCCESS) {
            fatalAt(inst->getLoc(),
                "GED_EncodeIns failed: %s",
                gedReturnValueToString(status));
        }
    }
}


bool Encoder::arePcsInQWords(const OpSpec &os) const
{
    // everything is in bytes except:
    // HSW calla, call, and jmpi
    return platform() < Platform::GEN8 &&
        os.op != Op::JMPI &&
        os.op != Op::CALL &&
        os.op != Op::CALLA;
}

bool Encoder::callNeedsSrcRegion221(const Instruction &inst) const
{
    // [call]: "Restriction: The src0 regioning control must be <2;2,1>" [IVB,HSW]
    // [calla]: "Restriction: The src0 regioning control must be <2;2,1>"
    return (inst.getOp() == Op::CALL && platform() < Platform::GEN8) ||
        (inst.getOp() == Op::CALLA && platform() <= Platform::GEN10);
}


void Encoder::encodeTernarySrcRegionVert(SourceIndex S, Region::Vert v) {
    if (S == SourceIndex::SRC0) {
        GED_ENCODE(Src0VertStride, lowerRegionVert(v));
    } else { // (S == SourceIndex::SRC1)
        GED_ENCODE(Src1VertStride, lowerRegionVert(v));
    } // S != SRC2 since ternary Align1 doesn't have bits for that
}

// fixes stuff where GED just ignores or where it refuses to allow us to
// set bits.  This should be empty unless GED fixes are in flight.
void Encoder::applyGedWorkarounds(
    const Kernel& /* k */, size_t /* bitsLen */)
{
    // NOTE: there should be a GED raw bits setter (we can use this for
    // workarounds...)
}

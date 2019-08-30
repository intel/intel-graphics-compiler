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
#include "Decoder.hpp"
#include "../../MemManager/MemManager.hpp"
#include "IGAToGEDTranslation.hpp"
#include "GEDToIGATranslation.hpp"
#include "../../Frontend/Formatter.hpp"
#include "../../Frontend/IRToString.hpp"
#include "../../IR/IRChecker.hpp"
#include "../../asserts.hpp"
#include "../../IR/SWSBSetter.hpp"

#include <sstream>
#include <cstring>

// Used to label expressions that need to be removed once GED is fixed
#define GED_WORKAROUND(X) (X)

using namespace ::iga;

DEFINE_GED_SOURCE_ACCESSORS_01(GED_ADDR_MODE, AddrMode)

DEFINE_GED_SOURCE_ACCESSORS_012(GED_REG_FILE, RegFile)
DEFINE_GED_SOURCE_ACCESSORS_01(int32_t, AddrImm)
DEFINE_GED_SOURCE_ACCESSORS_01(uint32_t, AddrSubRegNum)
DEFINE_GED_SOURCE_ACCESSORS_01(uint32_t, Width)

//        DEFINE_GED_SOURCE_ACCESSORS_INLINE_012(GED_REG_FILE, RegFile)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_DATA_TYPE, DataType)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, RegNum)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, SubRegNum)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_MATH_MACRO_EXT, MathMacroExt)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, VertStride)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, ChanSel)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_REP_CTRL, RepCtrl)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_SRC_MOD, SrcMod)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, HorzStride)

static void setImmValKind(Type t, ImmVal &val)
{
    switch (t) {
    case Type::B:  val.kind = ImmVal::Kind::S8; break;
    case Type::UB: val.kind = ImmVal::Kind::U8; break;
    case Type::W:  val.kind = ImmVal::Kind::S16; break;
    case Type::UW: val.kind = ImmVal::Kind::U16; break;
    case Type::D:  val.kind = ImmVal::Kind::S32; break;
    case Type::UD: val.kind = ImmVal::Kind::U32; break;
    case Type::Q:  val.kind = ImmVal::Kind::S64; break;
    case Type::UQ: val.kind = ImmVal::Kind::U64; break;
    case Type::HF: val.kind = ImmVal::Kind::F16; break;
    case Type::F:  val.kind = ImmVal::Kind::F32; break;
    case Type::DF: val.kind = ImmVal::Kind::F64; break;
    case Type::V: // fallthrough for the packed vector types
    case Type::UV:
    case Type::VF:
        val.kind = ImmVal::Kind::U32;
        break;
    default:
        break;
    }
}

static Region macroDefaultSourceRegion(
    int srcOpIx, const OpSpec &os, Platform p, ExecSize execSize)
{
    if (os.hasImplicitSrcRegion(srcOpIx, p, execSize)) {
        return os.implicitSrcRegion(srcOpIx, p, execSize);
    } else if (srcOpIx == 2) {
        return Region::SRCXX1;
    } else {
        if (p >= Platform::GEN12P1) {
            return os.isTernary() ?
                Region::SRC1X0 : // GEN12 ternary packed region
                Region::SRC110;
        }
        return os.isTernary() ?
            Region::SRC2X1 :
            Region::SRC441;
    }
}


DecoderBase::DecoderBase(const Model &model, ErrorHandler &errHandler) :
    GEDBitProcessor(model,errHandler),
    m_gedModel(IGAToGEDTranslation::lowerPlatform(model.platform)),
    m_kernel(nullptr),
    m_opSpec(nullptr),
    m_binary(nullptr)
{
    // Derived the swsb mode from plaform
    if (m_SWSBEncodeMode == SWSB_ENCODE_MODE::SWSBInvalidMode)
        m_SWSBEncodeMode = model.getSWSBEncodeMode();
    IGA_ASSERT(m_gedModel != GED_MODEL_INVALID, "invalid GED model");
}

void DecoderBase::decodeSWSB(Instruction* inst)
{
    if (m_model.platform >= Platform::GEN12P1) {
        uint32_t swsbBits = 0;
        if (inst->getOp() != Op::INVALID &&
            inst->getOp() != Op::ILLEGAL)
        {
            GED_DECODE_RAW_TO(SWSB, swsbBits);
        }
        // must convert the raw encoding bits to our SWSB IR
        SWSB::InstType inst_type = SWSB::InstType::OTHERS;
        if (inst->getOpSpec().isSendOrSendsFamily())
            inst_type = SWSB::InstType::SEND;
        else if (inst->getOpSpec().isMathSubFunc())
            inst_type = SWSB::InstType::MATH;
        SWSB sw;
        SWSB_STATUS status = sw.decode(swsbBits, m_SWSBEncodeMode, inst_type);

        switch (status)
        {
        case iga::SWSB_STATUS::ERROR_SET_ON_VARIABLE_LENGTH_ONLY:
            error("SBID set is only allowed on variable latency ops");
            break;
        case iga::SWSB_STATUS::ERROR_INVALID_SBID_VALUE:
            error("invalid SBID value 0x%x", swsbBits);
            break;
        default:
            break;
        }
        inst->setSWSB(sw);
    }
}

Kernel *DecoderBase::decodeKernelBlocks(
    const void *binary,
    size_t binarySize)
{
    return decodeKernel(binary, binarySize, false);
}


Kernel *DecoderBase::decodeKernelNumeric(
    const void *binary,
    size_t binarySize)
{
    return decodeKernel(binary, binarySize, true);
}


Kernel *DecoderBase::decodeKernel(
    const void *binary,
    size_t binarySize,
    bool numericLabels)
{
    m_binary = binary;
    if (binarySize == 0) {
        // edge case: empty kernel is okay
        return new Kernel(m_model);
    }
    if (binarySize < 8) {
        // bail if we don't have at least a compact instruction
        error("binary size is too small");
        return nullptr;
    }
    Kernel *kernel = new Kernel(m_model);

    InstList insts;
    // NOTE: we could pre-allocate instruction list here
    // (this would block allocate everything)
    // insts.reserve(binarySize / 8 + 1);

    // Pass 1. decode them all into Instruction objects
    decodeInstructions(
        *kernel,
        binary,
        binarySize,
        insts);

    if (numericLabels) {
        Block *block = kernel->createBlock();
        block->setPC(0);
        for (Instruction *inst : insts) {
            block->appendInstruction(inst);
        }
        kernel->appendBlock(block);
    } else {
        auto blockStarts = Block::inferBlocks(
            errorHandler(),
            kernel->getMemManager(),
            insts);
        int id = 1;
        for (auto bitr : blockStarts) {
            bitr.second->setID(id++);
            kernel->appendBlock(bitr.second);
        }
    }
    return kernel;
}

unsigned DecoderBase::decodeOpGroup(Op op)
{
    unsigned fcBits = 0xFFFFFFFF;
    switch (op) {
        case Op::MATH: {
            GED_DECODE(MathFC, GED_MATH_FC, mathFc, MathFC);
            fcBits = static_cast<unsigned>(mathFc);
            break;
        }
        case Op::SYNC: {
            GED_DECODE(SyncFC, GED_SYNC_FC, syncFC, SyncFC);
            fcBits = static_cast<unsigned>(syncFC);
            break;
        }
        case Op::SENDC:
        case Op::SEND: {
            GED_DECODE(SFID, GED_SFID, sfid, SFID);
            fcBits = static_cast<unsigned>(sfid);
            break;
        }
        default:
            break; // fallthrough with invalid fc_bits value
    }
    return fcBits;
}

const OpSpec *DecoderBase::decodeOpSpec(Op op)
{
    auto os = &m_model.lookupOpSpec(op);
    if (os->format != OpSpec::GROUP) {
        // simple op like add, mov, etc..
        return os;
    }
    unsigned fcBits = decodeOpGroup(op);

    // an op group such as math or wdep
    return &m_model.lookupGroupSubOp(op, fcBits);
}

// Pass 1. decode all instructions in Instruction*
void DecoderBase::decodeInstructions(
    Kernel &kernel,
    const void *binaryStart,
    size_t binarySize,
    InstList &insts)
{
    restart();
    uint32_t nextId = 1;
    const unsigned char *binary = (const unsigned char *)binaryStart;

    int32_t bytesLeft =  (int32_t)binarySize;
    while (bytesLeft > 0)
    {
        // need at least 4 bytes to check compaction control
        if (bytesLeft < 4) {
            warning("unexpected padding at end of kernel");
            break;
        }
        // ensure there's enough buffer left
        int32_t iLen = getBitField(COMPACTION_CONTROL,1) != 0 ?
            COMPACTED_SIZE :
            UNCOMPACTED_SIZE;
        if (bytesLeft < iLen) {
            warning("unexpected padding at end of kernel");
            break;
        }
        memset(&m_currGedInst, 0, sizeof(m_currGedInst));
        GED_RETURN_VALUE status =
            GED_DecodeIns(m_gedModel, binary, (uint32_t)binarySize, &m_currGedInst);
        Instruction *inst = nullptr;
        if (status == GED_RETURN_VALUE_NO_COMPACT_FORM) {
            error("error decoding instruction (no compacted form)");
            inst = createErrorInstruction(
                kernel,
                "unable to decompact",
                binary,
                iLen);
            // fall through: GED can sort of decode some things here
        } else if (status != GED_RETURN_VALUE_SUCCESS) {
            error("error decoding instruction");
            inst = createErrorInstruction(
                kernel,
                "GED error decoding instruction",
                binary,
                iLen);
        } else {
            Op op = GEDToIGATranslation::translate(GED_GetOpcode(&m_currGedInst));
            m_opSpec = decodeOpSpec(op);
            if (m_opSpec->op == Op::INVALID) {
                // figure out if we failed to resolve the primary op
                // or if it's an unmapped subfunction (e.g. math function)
                auto os = m_model.lookupOpSpec(op);
                std::stringstream ss;
                if (os.format == OpSpec::GROUP) {
                    ss << "unsupported pseudo op (sub function of " <<
                        os.mnemonic << ")";
                } else {
                    ss << std::hex << "0x" << (unsigned)op <<
                        ": unsupported opcode on this platform";
                }
                std::string str = ss.str();
                error("%s", str.c_str());
                inst = createErrorInstruction(
                    kernel,
                    str.c_str(),
                    binary,
                    iLen);
            } else {
                try {
                    inst = decodeNextInstruction(kernel);
                } catch (const FatalError &fe) {
                    // error is already logged
                    inst = createErrorInstruction(
                        kernel,
                        fe.what(),
                        binary,
                        iLen);
                }
            }
        }
        inst->setPC(currentPc());
        inst->setID(nextId++);
        insts.emplace_back(inst);
        inst->setLoc(currentPc());
#if _DEBUG
        if (!errorHandler().hasErrors()) {
            // only validate if there weren't errors
            inst->validate();
        }
#endif
        advancePc(iLen);
        binary += iLen;
        bytesLeft -= iLen;
    }

}

void DecoderBase::decodeNextInstructionEpilog(Instruction *inst)
{
    decodeSWSB(inst);
}

// Decodes a GED instruction to IGA IR and appends it to a given block
Instruction *DecoderBase::decodeNextInstruction(Kernel &kernel)
{
    Instruction *inst = nullptr;

    switch (m_opSpec->format)
    {
    case OpSpec::NULLARY:
        if (m_opSpec->op == Op::ILLEGAL) {
            inst = kernel.createIllegalInstruction();
        } else if (m_opSpec->op == Op::NOP) {
            inst = kernel.createNopInstruction();
        } else {
            std::stringstream ss;
            ss << "at pc " << currentPc() << ": invalid operation format";
            IGA_ASSERT_FALSE(ss.str().c_str());
            return kernel.createIllegalInstruction();
        }
        break;
    case OpSpec::BASIC_UNARY_REG:
    case OpSpec::BASIC_UNARY_REGIMM:
    case OpSpec::BASIC_BINARY_REG_IMM:
    case OpSpec::BASIC_BINARY_REG_REG:
    case OpSpec::BASIC_BINARY_REG_REGIMM:
    case OpSpec::MATH_UNARY_REGIMM: // math also supported via this path
    case OpSpec::MATH_BINARY_REG_REGIMM:
    case OpSpec::MATH_MACRO_UNARY_REG:
    case OpSpec::MATH_MACRO_BINARY_REG_REG:
        inst = decodeBasicInstruction(kernel);
        break;
    case OpSpec::JUMP_UNARY_REG:
    case OpSpec::JUMP_UNARY_IMM:
    case OpSpec::JUMP_UNARY_REGIMM:
    case OpSpec::JUMP_UNARY_CALL_REGIMM:
    case OpSpec::JUMP_BINARY_BRC:
    case OpSpec::JUMP_BINARY_IMM_IMM:
        if (m_model.supportsSimplifiedBranches()) {
            inst = decodeBranchSimplifiedInstruction(kernel);
        } else {
            inst = decodeBranchInstruction(kernel);
        }
        break;
    case OpSpec::TERNARY_REGIMM_REG_REGIMM:
    case OpSpec::TERNARY_MACRO_REG_REG_REG:
        inst = decodeTernaryInstruction(kernel);
        break;
    case OpSpec::SEND_UNARY:
    case OpSpec::SEND_BINARY:
        inst = decodeSendInstruction(kernel);
        break;
    case OpSpec::SYNC_UNARY:
        if (m_model.platform < Platform::GEN12P1) {
            inst = decodeWaitInstruction(kernel);
        } else {
            inst = decodeSyncInstruction(kernel);
        }
        break;
    default: {
        std::stringstream ss;
        ss << "at pc " << currentPc() << ": invalid operation format\n";
        ss << FormatOpBits(m_model, (const uint8_t*)m_binary + currentPc());

        IGA_ASSERT_FALSE(ss.str().c_str());
        return kernel.createIllegalInstruction();
    } // default:
    } // switch

    decodeOptions(inst);
    decodeNextInstructionEpilog(inst);

    return inst;
}

bool DecoderBase::hasImm64Src0Overlap()
{
    if (m_model.platform < Platform::GEN12P1)
        return false;

    // SWSB overlaps the flag modifier with src0
    // check if it's 64 bit imm
    GED_REG_FILE regFile = decodeSrcRegFile<SourceIndex::SRC0>();
    Type t = decodeSrcType<SourceIndex::SRC0>();
    return (TypeIs64b(t) && (regFile == GED_REG_FILE_IMM));
}

///////////////////////////////////////////////////////////////////////
// BASIC INSTRUCTIONS
///////////////////////////////////////////////////////////////////////

Instruction *DecoderBase::decodeBasicInstruction(Kernel &kernel)
{
    FlagRegInfo fri = decodeFlagRegInfo(hasImm64Src0Overlap());
    Instruction *inst = kernel.createBasicInstruction(
            *m_opSpec,
            fri.pred,
            fri.reg,
            decodeExecSize(),
            decodeChannelOffset(),
            decodeMaskCtrl(),
            fri.modifier);

    GED_ACCESS_MODE accessMode = decodeAccessMode();
    if (m_opSpec->supportsDestination()) {
        decodeBasicDestination(inst, accessMode);
    }
    switch (m_opSpec->format) {
    case OpSpec::BASIC_UNARY_REG:
    case OpSpec::BASIC_UNARY_REGIMM:
    case OpSpec::MATH_UNARY_REGIMM:
    case OpSpec::MATH_MACRO_UNARY_REG:
        decodeBasicUnaryInstruction(inst, accessMode);
        break;
    case OpSpec::BASIC_BINARY_REG_IMM:
    case OpSpec::BASIC_BINARY_REG_REG:
    case OpSpec::BASIC_BINARY_REG_REGIMM:
    case OpSpec::MATH_BINARY_REG_REGIMM:
    case OpSpec::MATH_MACRO_BINARY_REG_REG:
        decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
        decodeSourceBasic<SourceIndex::SRC1>(inst, accessMode);
        break;
    default:
        std::stringstream ss;
        ss << "IGA INTERNAL ERROR: ";
        ss << FormatOpBits(m_model, (const char *)m_binary + currentPc());
        ss << ": unexpected format for basic instruction";
        IGA_ASSERT_FALSE(ss.str().c_str());
        error(ss.str().c_str());
        inst = kernel.createIllegalInstruction();
    }

    return inst;
}

void DecoderBase::decodeBasicUnaryInstruction(Instruction *inst, GED_ACCESS_MODE accessMode)
{
    decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
    if (m_opSpec->op == Op::MOVI && m_model.platform >= Platform::GEN10) {
        // movi can takes two parameters on on this platform
        // movi (..) reg  reg  (imm|null)
        decodeSourceBasic<SourceIndex::SRC1>(inst, accessMode);
    }
}

void DecoderBase::decodeBasicDestinationAlign16(Instruction *inst)
{
    GED_DECODE_RAW(GED_ADDR_MODE, addrMode, DstAddrMode);

    DstModifier dstMod = DstModifier::NONE;
    if (inst->getOpSpec().supportsSaturation()) {
        GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
        dstMod = GEDToIGATranslation::translate(mod);
    }

    GED_DECODE(Type, GED_DATA_TYPE, type, DstDataType);

    switch (addrMode)
    {
    case GED_ADDR_MODE_Direct: {
        DirRegOpInfo dri = decodeDstDirRegInfo();
        if (inst->isMacro()) {
            MathMacroExt MathMacroReg = decodeDestinationMathMacroRegFromChEn();
            inst->setMacroDestination(dstMod,
                dri.regName, dri.regRef, MathMacroReg, Region::Horz::HZ_1, type);
        } else {
            // normal Align16 destination
            uint32_t hStride = 1;
            GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
            if (dri.regName == RegName::ARF_MME &&
                isAlign16MathMacroRegisterCsrPlatform())
            {
                // special access to acc2-acc9 via ChEn encoding
                // (for context save and restore)
                dri.regRef.regNum = (uint8_t)decodeDestinationRegNumAccBitsFromChEn();
            } else if (chEn == GED_DST_CHAN_EN_xyzw) {
                hStride = 1;
            } else {
                fatal("dst: unsupported Align16 ChEn; only <1> (.xyzw) supported");
            }

            GED_DECODE_RAW(uint32_t, subRegNum, DstSubRegNum);
            inst->setDirectDestination(
                dstMod, dri.regName, dri.regRef,
                GEDToIGATranslation::translateRgnH(hStride), type);
        }
        break;
    }
    case GED_ADDR_MODE_Indirect: {
        GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
        if (chEn == GED_DST_CHAN_EN_xyzw) {
            warning("converting unary/binary Align16 dst to equivalent Align1");
        } else {
            fatal("unsupported Align16 Dst.ChEn (only .xyzw supported)");
        }

        GED_DECODE_RAW(int32_t, addrImm, DstAddrImm);

        GED_DECODE_RAW(uint32_t, subRegNum, DstAddrSubRegNum);
        RegRef a0 = RegRef(0, (uint8_t)subRegNum);
        inst->setInidirectDestination(
            dstMod, a0, (uint16_t)addrImm, Region::Horz::HZ_1, type);
        break;
    }
    default:
        fatal("invalid addressing mode on dst");
        break;
    } // switch
}

void DecoderBase::decodeBasicDestinationAlign1(Instruction *inst) {
    GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;
    GED_DECODE_RAW(GED_ADDR_MODE, taddrMode, DstAddrMode);
    addrMode = taddrMode;

    DstModifier dstMod = DstModifier::NONE;
    if (inst->getOpSpec().supportsSaturation()) {
        GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
        dstMod = GEDToIGATranslation::translate(mod);
    }

    GED_DECODE_RAW(uint32_t, hStride, DstHorzStride);
    Region::Horz rgnHzDec = GEDToIGATranslation::translateRgnH(hStride);
    if (inst->getOpSpec().hasImplicitDstRegion()) {
        Region::Horz rgnHzImpl = inst->getOpSpec().implicitDstRegion().getHz();
        if (rgnHzImpl != rgnHzDec) {
            warning("dst has wrong region for binary normal form");
        }
    }

    GED_DECODE(Type, GED_DATA_TYPE, type, DstDataType);

    switch (addrMode)
    {
    case GED_ADDR_MODE_Direct: {
        GED_DECODE_RAW(GED_REG_FILE, regFile, DstRegFile);
        if (regFile != GED_REG_FILE_ARF && regFile != GED_REG_FILE_GRF) {
            error("invalid reg file on dst");
        }

        DirRegOpInfo dri = decodeDstDirRegInfo();
        if (inst->isMacro()) {
            GED_DECODE(MathMacroExt, GED_MATH_MACRO_EXT, mme, DstMathMacroExt);
            inst->setMacroDestination(
                dstMod, dri.regName, dri.regRef, mme, rgnHzDec, type);
        } else {
            // normal Align1 destination
            // it's a normal Align1 destination
            GED_DECODE_RAW(uint32_t, subRegNum, DstSubRegNum);
            inst->setDirectDestination(
                dstMod, dri.regName, dri.regRef, rgnHzDec, type);
        }
        break;
    }
    case GED_ADDR_MODE_Indirect: {
        GED_DECODE_RAW(int32_t, addrImm, DstAddrImm);
        GED_DECODE_RAW(uint32_t, subRegNum, DstAddrSubRegNum);
        RegRef a0 = {0, (uint8_t)subRegNum};
        inst->setInidirectDestination(
            dstMod, a0, (uint16_t)addrImm, rgnHzDec, type);
        break;
    }
    default:
        fatal("invalid addressing mode on dst");
        break;
    } // switch
}



///////////////////////////////////////////////////////////////////////
// TERNARY INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *DecoderBase::decodeTernaryInstruction(Kernel& kernel)
{
    FlagRegInfo fri = decodeFlagRegInfo();
    Instruction *inst = kernel.createBasicInstruction(
        *m_opSpec,
        fri.pred,
        fri.reg,
        decodeExecSize(),
        decodeChannelOffset(),
        decodeMaskCtrl(),
        fri.modifier);

    GED_ACCESS_MODE accessMode = decodeAccessMode();
    decodeTernaryInstructionOperands(kernel, inst, accessMode);

    return inst;
}

void DecoderBase::decodeTernaryInstructionOperands(
    Kernel& kernel, Instruction *inst, GED_ACCESS_MODE accessMode)
{
    if (accessMode == GED_ACCESS_MODE_Align16) {
        if (m_opSpec->supportsDestination()) {
            decodeTernaryDestinationAlign16(inst);
        }
        decodeTernarySourceAlign16<SourceIndex::SRC0>(inst);
        decodeTernarySourceAlign16<SourceIndex::SRC1>(inst);
        decodeTernarySourceAlign16<SourceIndex::SRC2>(inst);
    } else {
        if (m_model.platform >= Platform::GEN10) {
            if (m_opSpec->supportsDestination()) {
                decodeTernaryDestinationAlign1(inst);
            }
            decodeTernarySourceAlign1<SourceIndex::SRC0>(inst);
            decodeTernarySourceAlign1<SourceIndex::SRC1>(inst);
            decodeTernarySourceAlign1<SourceIndex::SRC2>(inst);
        } else {
            error("unexpected Align1Ternary in current platform");
            inst = kernel.createIllegalInstruction();
        }
    }
}

void DecoderBase::decodeTernaryDestinationAlign16(Instruction *inst)
{
    GED_DECODE_RAW(uint32_t, regNumBits, DstRegNum);
    DstModifier dstMod = DstModifier::NONE;
    if (m_opSpec->supportsSaturation()) {
        GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
        dstMod = GEDToIGATranslation::translate(mod);
    }
    GED_DECODE(Type, GED_DATA_TYPE, type, DstDataType);
    GED_DECODE_RAW(GED_REG_FILE, regFile, DstRegFile);
    GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);

    RegName regName = RegName::INVALID;
    RegRef regRef;
    decodeReg(-1, regFile, regNumBits, regName, regRef);

    if (inst->isMacro()) {
        MathMacroExt MathMacroReg = decodeDestinationMathMacroRegFromChEn();
        inst->setMacroDestination(
            dstMod, regName, regRef, MathMacroReg, Region::Horz::HZ_1, type);
    } else {
        // We have to translate Align16 ternary instructions to equivalent
        // Align1 where posssible.  The goal of these translations is to
        // capture everything the IGC compiler generates.  There will be
        // valid Align16 sequences that we choose not to represent in Align1.
        //
        // CASES:
        // SIMD1: illegal (hardware disallows this, we use a SIMD4 with ChEn to emulate)
        // SIMD2: We accept .xy, .zw only if the type is :df. E.g.
        //   mad (2) r5.0.xy:df ... // means r5.0<1>
        //   mad (2) r5.0.zw:df ... // means r5.1<1>
        //  *** we convert the exec size to SIMD1 ***
        // SIMD4: This can be a true SIMD4 or an emulation of a SIMD1 scalar.
        //   We decide based on the ChEn mask.
        //   .xyzw is a SIMD4, but .{x,y,z,w} means scalar SIMD1
        //   (since other lanes are masked out)
        //   *** we convert the exec size to SIMD1 for the scalar case ***
        //  I.e.
        //    mad (4) r5.0.xyzw:f ... // gets converted cleanly
        //  to
        //    mad (4) r5.0<1>:f
        //  but
        //    mad (4) r5.0.x:f ... {NoMask) // gets treated as scalar and
        //  translates to (W) mad (1) r5.0.x
        //
        // SIMD8, SIMD16: we only accept .xyzw and .r as packed and scalar
        //   this seems to capture everything used in practice
        //
        // NOTE: this is an appalling hack, but creates the least technical
        // debt for the project.  These problems all go away in GEN10 when
        // we get Align1 ternary and Align16 fades into the sunset.
        uint8_t subregOffAlign16Elems = 0; // in elements not bytes (add after conversion)
        switch (inst->getExecSize()) {
        case ExecSize::SIMD2:
            if (chEn != GED_DST_CHAN_EN_xyzw) {
                // this is a special case of below for DF and Q
                inst->setExecSize(ExecSize::SIMD1);
                if (chEn == GED_DST_CHAN_EN_xy && type == Type::DF) {
                    subregOffAlign16Elems = 0; // dst.k.xy:df => dst.(k+0)<1>:df
                } else if (chEn == GED_DST_CHAN_EN_zw && type == Type::DF) {
                    subregOffAlign16Elems = 1; // dst.k.xy:df => dst.(k+1)<1>:df
                } else {
                    error("unsupported Align16 ternary destination for SIMD2 "
                        "(must be .xywz or .{xy,zw} for :df)");
                }
            }
            break;
        case ExecSize::SIMD4:
            if (chEn != GED_DST_CHAN_EN_xyzw) {
                // with Align16, we emulate a scalar (SIMD1) by masking out
                // all, but one of the channels.
                // we must translate a SIMD4
                //         mad (4) r5.0.x:f ... {NoMask}
                // to the following
                //     (W) mad (1) r5.0<1>:f ...
                // we have to twiddle the execution size here too
                inst->setExecSize(ExecSize::SIMD1);
                switch (chEn) {
                case GED_DST_CHAN_EN_x:
                    subregOffAlign16Elems = 0; // subregister is already aligned
                    break;
                case GED_DST_CHAN_EN_y:
                    subregOffAlign16Elems = 1; // dst.k.y => dst.(k+1)<1> (e.g. dst.4.y => dst.5<1>)
                    break;
                case GED_DST_CHAN_EN_z:
                    subregOffAlign16Elems = 2; // dst.k.z => dst.(k+2)<1>
                    break;
                case GED_DST_CHAN_EN_w:
                    subregOffAlign16Elems = 3; // dst.k.w => dst.(k+3)<1>
                    break;
                default:
                    error("unsupported Align16 ternary destination for SIMD4 "
                        "(must be .xywz or .{x,y,z,w})");
                    break;
                }
            } // else { it's an .xyzw ChEn: we can leave the subregister alone }
            break;
        case ExecSize::SIMD8:
        case ExecSize::SIMD16:
        case ExecSize::SIMD32: // can appear for things like :hf, :w, or :b
            if (chEn != GED_DST_CHAN_EN_xyzw) {
                error("unsupported Align16 ternary destination for SIMD{8,16} "
                    "(must be .xywz)");
            }
            // the access must already be aligned for .xyzw
            break;
        default:
            // SIMD1 is illegal
            error("unsupported Align16 ternary destination (unsupported SIMD)");
            break;
        }

        GED_DECODE_RAW(uint32_t, subRegNumBytes, DstSubRegNum);
        uint8_t subRegNumber = type == Type::INVALID ?
            0 : binNumToSubRegNum((uint8_t)subRegNumBytes, regName, type);
        regRef.subRegNum = (uint8_t)(subRegNumber + subregOffAlign16Elems);
        inst->setDirectDestination(
            dstMod,
            regName,
            regRef,
            Region::Horz::HZ_1,
            type);
    }
}

template <SourceIndex S>
void DecoderBase::decodeTernarySourceAlign16(Instruction *inst)
{
    bool isMacro = inst->isMacro(); // madm or math.invm or math.rsqrt

    if (!isMacro && m_model.supportsAlign16MacroOnly()) {
        warning("src%d: converting Align16 to Align1 "
            "(bits will re-assemble to Align1)", (int)S);
    }

    SrcModifier srcMod = decodeSrcModifier<S>();

    ///////////////////////////////////////////////////////////////////////////
    // register name, number and and register file
    // (will be GRF)
    uint32_t regNum = decodeSrcRegNum<S>();

    ///////////////////////////////////////////////////////////////////////////
    // swizzling / region
    GED_DATA_TYPE gedType;
    GED_DECODE_RAW_TO(SrcDataType, gedType);
    if (m_model.platform >= Platform::GEN8LP &&
        (gedType == GED_DATA_TYPE_f || gedType == GED_DATA_TYPE_hf) &&
        S > SourceIndex::SRC0)
    {
        // CHV+ mixed mode
        gedType = decodeSrcDataType<S>();
    }
    Type type = GEDToIGATranslation::translate(gedType);

    if (isMacro) {
        MathMacroExt MathMacroReg = decodeSrcMathMacroReg<S>();
        RegRef rr = RegRef((uint8_t)regNum, 0);
        Region macroDftSrcRgn = macroDefaultSourceRegion(
            (int)S, inst->getOpSpec(), m_model.platform, inst->getExecSize());
        inst->setMacroSource(
            S,
            srcMod,
            RegName::GRF_R,
            rr,
            MathMacroReg,
            macroDftSrcRgn,
            type);
    } else {
        int subReg = type == Type::INVALID ?
            0 : binNumToSubRegNum(decodeSrcSubRegNum<S>(), RegName::GRF_R, type);
        RegRef reg = RegRef((uint8_t)regNum, (uint8_t)subReg);
        Region rgn;
        if (decodeSrcRepCtrl<S>() == GED_REP_CTRL_NoRep) {
            GED_SWIZZLE swizzle[4];
            decodeChSelToSwizzle(decodeSrcChanSel<S>(), swizzle);
            bool isFullSwizzle  =  (swizzle[0] == GED_SWIZZLE_x && swizzle[1] == GED_SWIZZLE_y  &&
                swizzle[2] == GED_SWIZZLE_z && swizzle[3] == GED_SWIZZLE_w);

            bool isXYSwizzle    = (swizzle[0] == GED_SWIZZLE_x && swizzle[1] == GED_SWIZZLE_y  &&
                swizzle[2] == GED_SWIZZLE_x && swizzle[3] == GED_SWIZZLE_y);

            bool isYZSwizzle = (swizzle[0] == GED_SWIZZLE_z && swizzle[1] == GED_SWIZZLE_w  &&
                swizzle[2] == GED_SWIZZLE_z && swizzle[3] == GED_SWIZZLE_w);

            bool invalidSwizzle = false;
            if (TypeIs64b(type)) {
                invalidSwizzle = !isFullSwizzle && !isXYSwizzle && !isYZSwizzle;
            } else {
                invalidSwizzle = !isFullSwizzle;
            }

            if (invalidSwizzle) {
                fatal("unconvertible ternary align16 operand");
            }

            // mad (8) r46.0.xyzw:df r46.0.xyzw:df r50.0.xyzw:df r48.0.xyzw:df {Align16, Q1} // #??:$66:%66 {0=EL, 1=EL, 2=EL, BC=BAD}
            // mad (2) r5.0.xy:df r5.0.xyxy:df r92.2.xyxy:df r93.0.xyxy:df {Align16, Q1, NoMask} // #??:$988:%988 {0=OL, 1=EH, 2=OH, BC=GOOD} BDW,SKL
            // a HW hack for scalar operation on DF
            if (type == Type::DF && (isXYSwizzle || isYZSwizzle)) {
                if (S == SourceIndex::SRC2) {
                    rgn = Region::SRCXX0;
                } else {
                    rgn = Region::SRC0X0;
                }
                if (isYZSwizzle) {
                    reg.subRegNum += 1;
                }
            } else {
                // we accept r#.xyzw as r#<2;1>
                if (S == SourceIndex::SRC2) {
                    rgn = Region::SRCXX1;
                } else {
                    rgn = Region::SRC2X1;
                }
            }
        } else {
            // r#.r is the same as r#<0;0>
            if (S == SourceIndex::SRC2) {
                rgn = Region::SRCXX0;
            } else {
                rgn = Region::SRC0X0;
            }
        }
        inst->setDirectSource(S, srcMod, RegName::GRF_R, reg, rgn, type);
    }
}

static bool ternaryDstOmitsHzStride(const OpSpec &os) {
    return false;
}

void DecoderBase::decodeTernaryDestinationAlign1(Instruction *inst)
{
    const OpSpec &os = inst->getOpSpec();

    DstModifier dstMod = DstModifier::NONE;
    if (os.supportsSaturation()) {
        GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
        dstMod = GEDToIGATranslation::translate(mod);
    }

    DirRegOpInfo dri = decodeDstDirRegInfo();

    if (inst->isMacro()) {
        GED_DECODE(MathMacroExt, GED_MATH_MACRO_EXT, mme, DstMathMacroExt);
        inst->setMacroDestination(
            dstMod, dri.regName, dri.regRef, mme, Region::Horz::HZ_1, dri.type);
    } else {
        if (ternaryDstOmitsHzStride(inst->getOpSpec())) {
            if (dri.regRef.subRegNum != 0) {
                error("Invalid subreg number (expected 0)");
            }
            Region::Horz dftRgnHz = os.hasImplicitDstRegion() ?
                os.implicitDstRegion().getHz() : Region::Horz::HZ_1;
            inst->setDirectDestination(dstMod,
                dri.regName,
                dri.regRef,
                dftRgnHz,
                dri.type);
        } else {
            GED_DECODE_RAW(uint32_t, hStride, DstHorzStride);

            inst->setDirectDestination(dstMod,
                dri.regName,
                dri.regRef,
                GEDToIGATranslation::translateRgnH(hStride),
                dri.type);
        }
    }
}

template <SourceIndex S>
Region DecoderBase::decodeSrcRegionTernaryAlign1()
{
    if (S == SourceIndex::SRC2) {
        return GEDToIGATranslation::transateGEDtoIGARegion(
            static_cast<uint32_t>(Region::Vert::VT_INVALID),
            static_cast<uint32_t>(Region::Width::WI_INVALID),
            decodeSrcHorzStride<S>());
    } else {
        return GEDToIGATranslation::transateGEDtoIGARegion(
            decodeSrcVertStride<S>(),
            static_cast<uint32_t>(Region::Width::WI_INVALID),
            decodeSrcHorzStride<S>());
    }
}

template <SourceIndex S>
void DecoderBase::decodeTernarySourceAlign1(Instruction *inst)
{
    if (m_model.platform < Platform::GEN10) {
        fatal("Align1 not available on this platform");
    }

    GED_REG_FILE regFile = decodeSrcRegFile<S>();

    // regular ternary align1 source operand
    if (regFile == GED_REG_FILE_IMM) {
        Type type = decodeSrcType<S>();

        ImmVal val;
        if (m_model.platform < Platform::GEN10) {
            val = decodeSrcImmVal(type);
        } else {
            val = decodeTernarySrcImmVal<S>(type);
        }

        inst->setImmediateSource(S, val, type);
    } else if (regFile == GED_REG_FILE_GRF || regFile == GED_REG_FILE_ARF) {
        // addressing mode is always direct in Align1 ternary
        if (inst->isMacro()) {
            if (m_model.supportsAlign16ImplicitAcc()) {
                fatal("src%d: macro instructions must be Align16 for this platform.", (int)S);
            }
            RegRef regRef;
            RegName regName = decodeSourceReg<S>(regRef);
            Region macroDftSrcRgn = macroDefaultSourceRegion(
                (int)S, inst->getOpSpec(), m_model.platform, inst->getExecSize());
            inst->setMacroSource(
                S,
                decodeSrcModifier<S>(),
                regName,
                regRef,
                decodeSrcMathMacroReg<S>(),
                macroDftSrcRgn,
                decodeSrcType<S>());
        } else {
            // normal access
            Region rgn = decodeSrcRegionTernaryAlign1<S>();
            DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<S>();

            inst->setDirectSource(
                S,
                decodeSrcModifier<S>(),
                opInfo.regName,
                opInfo.regRef,
                rgn,
                opInfo.type);
        }
    } else { // GED_REG_FILE_INVALID
        fatal("invalid register file in src%d", (int)S);
    }
}


///////////////////////////////////////////////////////////////////////
// SEND INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *DecoderBase::decodeSendInstruction(Kernel& kernel)
{
    // desc
    GED_DECODE_RAW(GED_REG_FILE, descRegFile, DescRegFile);
    SendDescArg msgDesc {};
    if (descRegFile == GED_REG_FILE_IMM) {
        msgDesc.type = SendDescArg::IMM;
        GED_DECODE_RAW_TO(MsgDesc, msgDesc.imm);
    } else {
        // desc register is hardwired to a0.0 (ex-desc below can vary)
        msgDesc.type = SendDescArg::REG32A;
        msgDesc.reg.regNum = 0;
        msgDesc.reg.subRegNum = 0;
    }

    // ex_desc
    GED_REG_FILE exDescRegFile = GED_REG_FILE_IMM;
    if (m_opSpec->format & OpSpec::Format::SEND_BINARY) {
        // only sends has ExDescRegFile
        GED_DECODE_RAW_TO(ExDescRegFile, exDescRegFile);
    }

    SendDescArg extMsgDesc {};
    if (exDescRegFile == GED_REG_FILE_IMM) {
        extMsgDesc.type = SendDescArg::IMM;
        GED_DECODE_RAW_TO(ExMsgDesc, extMsgDesc.imm);
    } else {
        // For sends GED interprets SelReg32ExDesc and returns default values

        extMsgDesc.type = SendDescArg::REG32A;
        extMsgDesc.reg.regNum = 0; // a0 is implied
        GED_DECODE_RAW(uint32_t, subRegNum, ExDescAddrSubRegNum);
        extMsgDesc.reg.subRegNum = subRegNum / 2;
    }

    FlagRegInfo fri = decodeFlagRegInfo();
    Instruction *inst = kernel.createSendInstruction(
        *m_opSpec,
        fri.pred,
        fri.reg,
        decodeExecSize(),
        decodeChannelOffset(),
        decodeMaskCtrl(),
        extMsgDesc,
        msgDesc);
    if ((m_opSpec->format & OpSpec::Format::SEND_BINARY) == OpSpec::Format::SEND_BINARY) { // send is binary
        decodeSendDestination(inst);
        decodeSendSource0(inst);
        decodeSendSource1(inst);
    } else { // if (m_opSpec->isSendFamily()) {
        decodeSendDestination(inst);
        decodeSendSource0(inst);
    }

    decodeSendInstructionOptions(inst);

    return inst;
}

void DecoderBase::decodeSendDestination(Instruction *inst)
{
    GED_ACCESS_MODE accessMode = decodeAccessMode();
    GED_DECODE_RAW(GED_REG_FILE, regFile, DstRegFile);
    GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;

    if (m_model.platform <= Platform::GEN11) {
        GED_DECODE_RAW_TO(DstAddrMode, addrMode);
    }

    if (addrMode == GED_ADDR_MODE_Indirect) {
        if( regFile == GED_REG_FILE_GRF ) {
            decodeBasicDestination(inst, accessMode);
        } else {
            error("error decoding instruction: SEND dst ARF");
        }
    } else {
        DirRegOpInfo dri = decodeDstDirRegInfo();

        Region::Horz rgnHz = Region::Horz::HZ_1;
        if (m_opSpec->hasImplicitDstRegion()) {
            rgnHz = m_opSpec->implicitDstRegion().getHz();
        }

        inst->setDirectDestination(
            DstModifier::NONE,
            dri.regName,
            dri.regRef,
            rgnHz,
            dri.type);
    }
}

GED_ADDR_MODE DecoderBase::decodeSendSource0AddressMode()
{
    GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;
    if (m_model.platform <= Platform::GEN11) {
        addrMode = decodeSrcAddrMode<SourceIndex::SRC0>();
    }
    return addrMode;
}

void DecoderBase::decodeSendSource0(Instruction *inst)
{
    GED_ACCESS_MODE accessMode = decodeAccessMode();
    GED_REG_FILE regFile = decodeSrcRegFile<SourceIndex::SRC0>();

    GED_ADDR_MODE addrMode = decodeSendSource0AddressMode();

    if( regFile == GED_REG_FILE_GRF && addrMode == GED_ADDR_MODE_Indirect) {
        decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
    } else {
        DirRegOpInfo dri = decodeSrcDirRegOpInfo<SourceIndex::SRC0>();

        Region rgn = inst->getOpSpec().implicitSrcRegion(
            0,
            m_model.platform,
            inst->getExecSize());
        bool hasSrcRgnEncoding = inst->getOpSpec().isSendFamily()
            && m_model.platform < Platform::GEN9;

        hasSrcRgnEncoding &= m_model.platform <= Platform::GEN11;

        if (hasSrcRgnEncoding) {
            // these bits are implicitly set by GED on SKL, and they disallow access
            rgn = decodeSrcRegionVWH<SourceIndex::SRC0>();
        }

        inst->setDirectSource(
            SourceIndex::SRC0,
            SrcModifier::NONE,
            dri.regName,
            dri.regRef,
            rgn,
            dri.type);
    }
}


void DecoderBase::decodeSendSource1(Instruction *inst)
{
    RegRef regRef;
    RegName regName = decodeSourceReg<SourceIndex::SRC1>(regRef);
    const OpSpec &os = inst->getOpSpec();
    Region rgn = os.implicitSrcRegion(1, m_model.platform, inst->getExecSize());
    Type implSrcType = os.implicitSrcType(1, false, m_model.platform);
    inst->setDirectSource(
        SourceIndex::SRC1,
        SrcModifier::NONE,
        regName,
        regRef,
        rgn,
        implSrcType);
}

void DecoderBase::decodeSendInstructionOptions(Instruction *inst)
{

    if (m_model.platform >= Platform::GEN12P1) {
        GED_FUSION_CTRL fusionCtrl = GED_FUSION_CTRL_Normal;
        GED_DECODE_RAW_TO(FusionCtrl, fusionCtrl);
        if (fusionCtrl == GED_FUSION_CTRL_Serialized) {
            inst->addInstOpt(InstOpt::SERIALIZE);
        }
    }
}

///////////////////////////////////////////////////////////////////////
// BRANCH INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *DecoderBase::decodeBranchInstruction(Kernel& kernel)
{
    if (decodeAccessMode() == GED_ACCESS_MODE_Align16) {
        error("Align16 branches not supported");
        return kernel.createIllegalInstruction();
    }

    BranchCntrl branchCtrl = BranchCntrl::OFF;
    if (m_opSpec->supportsBranchCtrl()) {
        GED_DECODE_TO(BranchCtrl, GEDToIGATranslation::translate, branchCtrl);
    }
    FlagRegInfo fri = decodeFlagRegInfo();
    Instruction *inst = kernel.createBranchInstruction(
        *m_opSpec,
        fri.pred,
        fri.reg,
        decodeExecSize(),
        decodeChannelOffset(),
        decodeMaskCtrl(),
        branchCtrl);

    if (m_opSpec->op == Op::JMPI) {
        //   jmpi (1) JIP
        // is encoded as:
        //   jmpi (1) ip ip JIP
        GED_REG_FILE regFile = GED_REG_FILE_INVALID;

        GED_DECODE_RAW(GED_REG_FILE, regTFile, Src1RegFile);
        regFile = regTFile;

        // TODO: make and use m_opSpec->hasImplicit{Source,Destination}()
        if (regFile != GED_REG_FILE_IMM) {
            if (m_model.supportsSrc1CtrlFlow()) {
                decodeSourceBasic<SourceIndex::SRC1>(
                    inst, SourceIndex::SRC0, GED_ACCESS_MODE_Align1);
            } else {
                Region rgn = decodeSrcRegionVWH<SourceIndex::SRC0>();
                DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<SourceIndex::SRC0>();
                inst->setDirectSource(
                    SourceIndex::SRC0, SrcModifier::NONE, opInfo.regName, opInfo.regRef, rgn, opInfo.type);
            }
        } else {
            GED_DECODE_RAW(int32_t, jip, JIP);
            // jmpi is stored post-increment; normalize it to pre-increment
            jip += GED_InsSize(&m_currGedInst);
            Type dataType = Type::INVALID;
            if (m_model.supportsSrc1CtrlFlow()) {
                dataType = decodeSrcType<SourceIndex::SRC1>();
            } else {
                dataType = decodeSrcType<SourceIndex::SRC0>();
            }
            inst->setLabelSource(
                SourceIndex::SRC0,
                jip,
                dataType);
        }
    } else if(m_opSpec->op == Op::RET) {
        // ret encodes as:
        //   ret (..) null src0
        // we leave then null implicit
        decodeSourceBasicAlign1<SourceIndex::SRC0>(inst);
    } else if (m_opSpec->op == Op::CALL || m_opSpec->op == Op::CALLA) {
        // calla (..)  reg  imm32
        // call  (..)  reg  imm32
        // call  (..)  reg  reg32
        //
        // call can take register or immediate (jip)
        // call stores register info in src1
        decodeBasicDestinationAlign1(inst);
        GED_REG_FILE regFile = GED_REG_FILE_INVALID;
        Type srcType = Type::INVALID;

        GED_DECODE_RAW(GED_REG_FILE, regTFile, Src1RegFile);
        regFile = regTFile;
        srcType = decodeSrcType<SourceIndex::SRC1>();

        if (regFile == GED_REG_FILE_IMM) {
            // calla (..)  reg  imm32
            // call  (..)  reg  imm32
            decodeJipToSrc(inst,
                SourceIndex::SRC0,
                srcType);
        } else {
            // call (..)  reg  reg32

            decodeSourceBasicAlign1<SourceIndex::SRC1>(
                inst,
                SourceIndex::SRC0);
        }
    } else if (m_opSpec->op == Op::BRC || m_opSpec->op == Op::BRD) {
        // brc (..) lbl16 lbl16    [PreBDW]
        // brc (..) lbl32 lbl32    [BDW+]
        // brc (..) reg32          [PreHSW]
        // brc (..) reg64          [HSW]
        // brc (..) reg64          [BDW+]
        //
        // brd (..) imm16          [IVB,HSW]
        // brd (..) reg32          [IVB,HSW]
        // brd (..) lbl32          [BDW+]
        // brd (..) reg32          [BDW+]
        GED_DECODE_RAW(GED_REG_FILE, regFile, Src0RegFile);
        if (regFile == GED_REG_FILE_IMM) {
            Type type = decodeSrcType<SourceIndex::SRC0>();
            decodeJipToSrc(inst,
                SourceIndex::SRC0,
                type);
            if (m_opSpec->op == Op::BRC) {
                decodeUipToSrc1(inst, type);
            }
        } else {
            // register argument
            decodeSourceBasicAlign1<SourceIndex::SRC0>(inst);
            if (m_opSpec->op == Op::BRC) {
                // add an implicit null parameter
                inst->setSource(SourceIndex::SRC1, Operand::SRC_REG_NULL_UD);
            }
        }
    } else {
        // e.g. if, else, endif, while, cont, break, ...
        decodeJipToSrc(inst);
        if (m_opSpec->format != OpSpec::Format::JUMP_UNARY_IMM) {
            decodeUipToSrc1(inst, Type::INVALID);
        }
    }

    return inst;
}

Instruction *DecoderBase::decodeBranchSimplifiedInstruction(Kernel& kernel)
{
    BranchCntrl branchCtrl = BranchCntrl::OFF;
    if (m_opSpec->supportsBranchCtrl()) {
        GED_DECODE_TO(BranchCtrl, GEDToIGATranslation::translate, branchCtrl);
    }
    FlagRegInfo fri = decodeFlagRegInfo();
    Instruction *inst = kernel.createBranchInstruction(
        *m_opSpec,
        fri.pred,
        fri.reg,
        decodeExecSize(),
        decodeChannelOffset(),
        decodeMaskCtrl(),
        branchCtrl);

    if (inst->getOpSpec().supportsDestination()) {
        decodeBranchDestination(inst);
    }

    GED_DECODE_RAW(GED_REG_FILE, regFile, Src0RegFile);
    if (regFile != GED_REG_FILE_IMM) {
        Region rgn =
            m_opSpec->implicitSrcRegion(0, m_model.platform, inst->getExecSize());
        DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<SourceIndex::SRC0>();
        inst->setDirectSource(
            SourceIndex::SRC0,
            SrcModifier::NONE,
            opInfo.regName,
            opInfo.regRef,
            rgn,
            opInfo.type);
    } else {
        decodeJipToSrc(inst,
            SourceIndex::SRC0,
            m_opSpec->implicitSrcType(SourceIndex::SRC0, false, m_model.platform));
    }
    // brc/brd read both UIP and JIP from one register (64-bits)
    bool isReg64 = ((m_opSpec->op == Op::BRC || m_opSpec->op == Op::BRD) &&
        regFile == GED_REG_FILE_GRF);
    bool isUnary = (m_opSpec->format & OpSpec::Format::UNARY) != 0;
    if (!isReg64 && !isUnary) {
        decodeUipToSrc1(inst, Type::INVALID);
    }
    return inst;
}

void DecoderBase::decodeBranchDestination(Instruction *inst)
{
    DirRegOpInfo dri = decodeDstDirRegInfo();
    inst->setDirectDestination(
        DstModifier::NONE, dri.regName, dri.regRef, Region::Horz::HZ_1, Type::UD);
}

///////////////////////////////////////////////////////////////////////
// OTHER INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *DecoderBase::decodeWaitInstruction(Kernel &kernel)
{
    // wait encodes as
    //   wait (..) nreg  nreg  null
    GED_ACCESS_MODE accessMode = decodeAccessMode();
    FlagRegInfo fri = decodeFlagRegInfo();
    Instruction *inst = kernel.createBasicInstruction(
            *m_opSpec,
            fri.pred,
            fri.reg,
            decodeExecSize(),
            decodeChannelOffset(),
            decodeMaskCtrl(),
            fri.modifier);
    decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
    return inst;
}

Instruction *DecoderBase::decodeSyncInstruction(Kernel &kernel)
{
    FlagRegInfo fri = decodeFlagRegInfo();
    Instruction *inst = kernel.createBasicInstruction(
            *m_opSpec,
            fri.pred,
            fri.reg,
            decodeExecSize(),
            decodeChannelOffset(),
            decodeMaskCtrl(),
            fri.modifier);
    GED_REG_FILE regFile = decodeSrcRegFile<SourceIndex::SRC0>();
    if (regFile == GED_REG_FILE_ARF) {
        // e.g.
        //   sync.nop     null
        //   sync.allrd   null
        //   ...
        inst->setSource(SourceIndex::SRC0, Operand::SRC_REG_NULL_UB);
    } else {
        // e.g.
        //   sync.allrd   0x15
        //   ...
        decodeSourceBasic<SourceIndex::SRC0>(inst, GED_ACCESS_MODE_Align1);
    }
    return inst;
}

Predication DecoderBase::decodePredication()
{
    Predication pred = {PredCtrl::NONE, false};
    GED_DECODE_RAW(GED_PRED_CTRL, pc, PredCtrl);
    pred.function = GEDToIGATranslation::translate(pc);
    return pred;
}

void DecoderBase::decodePredInv(Predication& pred)
{
    GED_DECODE_RAW(GED_PRED_INV, pi, PredInv);
    pred.inverse = (pi == GED_PRED_INV_Invert);
}

MaskCtrl DecoderBase::decodeMaskCtrl()
{
    GED_DECODE(MaskCtrl, GED_MASK_CTRL, ctrl, MaskCtrl);
    return ctrl;
}

FlagRegInfo DecoderBase::decodeFlagRegInfo(bool imm64Src0Overlaps) {

    FlagRegInfo fri = {
        {PredCtrl::NONE, false},
        FlagModifier::NONE,
        REGREF_ZERO_ZERO};
    if (m_opSpec->supportsPredication()) {
        fri.pred = decodePredication();
    }
    if (m_opSpec->supportsFlagModifier() && !imm64Src0Overlaps) {
        // GEN12 SWSB overlaps CondModifier and Imm64 values
        GED_DECODE_RAW(GED_COND_MODIFIER, condMod, CondModifier);
        fri.modifier = GEDToIGATranslation::translate(condMod);
    } else if (m_opSpec->isMathSubFunc() && m_opSpec->isMacro()) {
        // math.inv and math.rsqrtm both implicitly support EO
        fri.modifier = FlagModifier::EO;
    }

    if (m_opSpec->supportsPredication())
        decodePredInv(fri.pred);

    if (fri.pred.function != PredCtrl::NONE ||
        fri.modifier != FlagModifier::NONE)
    {
        GED_DECODE_RAW(uint32_t, flagRegNum, FlagRegNum);
        fri.reg.regNum = static_cast<uint8_t>(flagRegNum);
        GED_DECODE_RAW(uint32_t, flagSubRegNum, FlagSubRegNum);
        fri.reg.subRegNum = static_cast<uint8_t>(flagSubRegNum);
    }

    return fri;
}

ExecSize DecoderBase::decodeExecSize()
{
    GED_DECODE_RAW(uint32_t, execSize, ExecSize);
    return GEDToIGATranslation::translateExecSize(execSize);
}

ChannelOffset DecoderBase::decodeChannelOffset()
{
    if (m_opSpec->supportsQtrCtrl()) {
        GED_DECODE(ChannelOffset, GED_CHANNEL_OFFSET, em, ChannelOffset);
        return em;
    } else {
        return ChannelOffset::M0;
    }
}

GED_ACCESS_MODE DecoderBase::decodeAccessMode()
{
    if (m_model.supportsAccessMode()) {
        GED_DECODE_RAW(GED_ACCESS_MODE, accessMode, AccessMode);
        return accessMode;
    }
    return GED_ACCESS_MODE_Align1;
}

void DecoderBase::decodeJipToSrc(Instruction *inst, SourceIndex s, Type type) {
    inst->setLabelSource(s, decodeJip(), type);
}
void DecoderBase::decodeUipToSrc1(Instruction *inst, Type type) {
    inst->setLabelSource(SourceIndex::SRC1, decodeUip(), type);
}

// PreBDW JIP and UIP are in QWORDS in <GEN8 except for a few
// exceptions for the above instructions
#define PC_SCALE \
        ((m_model.platform < Platform::GEN8 && \
        m_opSpec->op != Op::CALL && \
        m_opSpec->op != Op::CALLA && \
        m_opSpec->op != Op::JMPI) ? 8 : 1)
int32_t DecoderBase::decodeJip() {
    GED_DECODE_RAW(int32_t, jip, JIP);
    return jip * PC_SCALE;
}
int32_t DecoderBase::decodeUip() {
    GED_DECODE_RAW(int32_t, uip, UIP);
    return uip * PC_SCALE;
}

int DecoderBase::decodeDestinationRegNumAccBitsFromChEn()
{
    // this is used by the math macro register (implicit accumulator access)
    // and for context save and restore access to those registers
    GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
    switch (chEn) {
    case GED_DST_CHAN_EN_None: return 0; // 0000b => mme0 (acc2)
    case GED_DST_CHAN_EN_x:    return 1; // 0001b => mme1 (acc3)
    case GED_DST_CHAN_EN_y:    return 2; // 0010b => mme2 (acc4)
    case GED_DST_CHAN_EN_xy:   return 3; // 0011b => mme3 (acc5)
    case GED_DST_CHAN_EN_z:    return 4; // 0100b => mme4 (acc6)
    case GED_DST_CHAN_EN_xz:   return 5; // 0101b => mme5 (acc7)
    case GED_DST_CHAN_EN_yz:   return 6; // 0110b => mme6 (acc8)
    case GED_DST_CHAN_EN_xyz:  return 7; // 0111b => mme7 (acc9)
    //
    // every thing else unreachable because this is an explicit operand
    // not an implicit math macro acc reference
    //
    case GED_DST_CHAN_EN_w:    return 0; // 1000b => noacc
    //
    // HACK: for context save and restore, acc9 encodes as .xyzw
    // I think this is because of
    //   mov(8) acc2:ud r103:ud        {NoMask, Align16}   //acc9
    //              ^.xyzw implied
    // Seems like it should just be .xyz
    case GED_DST_CHAN_EN_xyzw: return 7; // 1111b => mme7 (acc9)
    default:
        error("dst: invalid math macro register (from ChEn)");
        return -1;
    }
}


MathMacroExt DecoderBase::decodeDestinationMathMacroRegFromChEn()
{
    // this is used by the math macro register (implicit accumulator) access
    // and for context save and restore access to those registers
    GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
    switch (chEn) {
    case GED_DST_CHAN_EN_None: return MathMacroExt::MME0; // 0000b => mme0 (acc2)
    case GED_DST_CHAN_EN_x:    return MathMacroExt::MME1; // 0001b => mme1 (acc3)
    case GED_DST_CHAN_EN_y:    return MathMacroExt::MME2; // 0010b => mme2 (acc4)
    case GED_DST_CHAN_EN_xy:   return MathMacroExt::MME3; // 0011b => mme3 (acc5)
    case GED_DST_CHAN_EN_z:    return MathMacroExt::MME4; // 0100b => mme4 (acc6)
    case GED_DST_CHAN_EN_xz:   return MathMacroExt::MME5; // 0101b => mme5 (acc7)
    case GED_DST_CHAN_EN_yz:   return MathMacroExt::MME6; // 0110b => mme6 (acc8)
    case GED_DST_CHAN_EN_xyz:  return MathMacroExt::MME7; // 0111b => mme7 (acc9)
    case GED_DST_CHAN_EN_w:    return MathMacroExt::NOMME; // 1000b => nomme (noacc)
    default:
        error("invalid dst implicit accumulator reference (in ChEn)");
        return MathMacroExt::INVALID;
    }
}

void DecoderBase::decodeDstDirSubRegNum(DirRegOpInfo& dri)
{
    if (m_opSpec->isMacro() || m_opSpec->isSendOrSendsFamily()) {
        dri.regRef.subRegNum = 0;
    } else {
        Type scalingType = dri.type;
        if (scalingType == Type::INVALID)
            scalingType = m_opSpec->isBranching() ? Type::D : Type::UB;

        GED_DECODE_RAW(uint32_t, subRegNum, DstSubRegNum);
        dri.regRef.subRegNum =
            binNumToSubRegNum((uint8_t)subRegNum, dri.regName, scalingType);
    }
}

void DecoderBase::decodeReg(
    int opIx,
    GED_REG_FILE regFile,
    uint32_t regNumBits,
    RegName &regName,
    RegRef &regRef) // works for src or dst
{
    const char *opName =
        opIx == 0 ? "src0" :
        opIx == 1 ? "src1" :
        opIx == 2 ? "src2" :
        "dst";
    if (regFile == GED_REG_FILE_GRF) {
        regName = RegName::GRF_R;
        regRef.regNum = (uint8_t)regNumBits;
    } else if (regFile == GED_REG_FILE_ARF) { // ARF
        regName = RegName::INVALID;
        int arfRegNum = 0;
        const RegInfo *ri = m_model.lookupArfRegInfoByRegNum(regNumBits);
        if (ri == nullptr) {
            error("%s: 0x%02X: invalid arf register", opName, regNumBits);
        } else {
            regName = ri->regName;
            if (!ri->decode(regNumBits, arfRegNum)) {
                error("%s: %s%d: invalid register number ",
                    opName, ri->syntax, arfRegNum);
            }
        }
        regRef.regNum = (uint8_t)arfRegNum;
    } else { // e.g. 10b
        error("%s: invalid register file", opName);
    }
}

DirRegOpInfo DecoderBase::decodeDstDirRegInfo() {
    DirRegOpInfo dri;
    dri.type = m_opSpec->implicitDstType(m_model.platform);
    bool hasDstType = true;
    if (m_model.platform >= Platform::GEN12P1) {
        hasDstType &= !m_opSpec->isSendOrSendsFamily();
        hasDstType &= !m_opSpec->isBranching();
    }
    if (hasDstType) {
        dri.type = decodeDstType();
    }

    GED_DECODE_RAW(GED_REG_FILE, gedRegFile, DstRegFile);
    GED_DECODE_RAW(uint32_t, regNumBits, DstRegNum);
    decodeReg(-1,gedRegFile,regNumBits,dri.regName,dri.regRef);
    decodeDstDirSubRegNum(dri);

    return dri;
}

Type DecoderBase::decodeDstType() {
    GED_DECODE(Type, GED_DATA_TYPE, t, DstDataType);
    return t;
}

bool DecoderBase::hasImplicitScalingType(Type& type, DirRegOpInfo& dri)
{
    // FIXME: when entering this function, assuming it MUST NOT be imm or label src
    if (m_model.platform >= Platform::GEN12P1 &&
        (m_opSpec->isSendFamily() || m_opSpec->isBranching()))
    {
        dri.type = m_opSpec->implicitSrcType(SourceIndex::SRC0, false, m_model.platform);
        type = Type::D;
        return true;
    }
    return false;
}

ImmVal DecoderBase::decodeSrcImmVal(Type t) {
    ImmVal val;
    val.kind = ImmVal::Kind::UNDEF;
    memset(&val, 0, sizeof(val)); // zero value in case GED only sets bottom bits

    GED_DECODE_RAW_TO(Imm, val.u64);
    setImmValKind(t, val);
    return val;
}

template <SourceIndex S>
void DecoderBase::decodeSourceBasicAlign1(Instruction *inst, SourceIndex toSrcIx)
{
    GED_REG_FILE regFile = decodeSrcRegFile<S>();
    if (regFile == GED_REG_FILE_IMM) {
        // immediate operand
        Type type = decodeSrcType<S>();
        inst->setImmediateSource(toSrcIx, decodeSrcImmVal(type), type);
    } else if (regFile == GED_REG_FILE_ARF || regFile == GED_REG_FILE_GRF) {
        // register operand
        GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;
        addrMode = decodeSrcAddrMode<S>();

        SrcModifier srcMod = decodeSrcModifier<S>();
        // region (implicit accumulator if Align16 and <GEN11)
        Region implRgn = Region::INVALID;
        if (inst->getOpSpec().hasImplicitSrcRegion(
                toSrcIx, m_model.platform, inst->getExecSize()))
        {
            implRgn = inst->getOpSpec().implicitSrcRegion(
                toSrcIx, m_model.platform, inst->getExecSize());
        }
        Region decRgn = Region::INVALID;
        if (!m_opSpec->isSendOrSendsFamily()) {
            decRgn = decodeSrcRegionVWH<S>();
        } else {
            decRgn = implRgn;
        }
        // ensure the region matches any implicit region rules
        if (!m_opSpec->isSendOrSendsFamily() &&
            inst->getOpSpec().hasImplicitSrcRegion(
                toSrcIx, m_model.platform, inst->getExecSize()))
        {
            if (implRgn != decRgn) {
                warning("src%d.Rgn should have %s for binary normal form" ,
                    (int)S,
                    ToSyntax(implRgn).c_str());
            }
        }

        if (addrMode == GED_ADDR_MODE_Direct) {
            if (inst->isMacro()) {
                // GEN11 macros are stored in the subregister
                if (m_model.supportsAlign16()) {
                    fatal("src%d: macro instructions must be Align16 for this platform.", (int)S);
                }
                MathMacroExt mme = decodeSrcMathMacroReg<S>();
                RegRef regRef{0,0};
                RegName regName = decodeSourceReg<S>(regRef);
                inst->setMacroSource(
                    toSrcIx, srcMod, regName, regRef, mme, decRgn, decodeSrcType<S>());
            } else {
                // normal access
                DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<S>();
                inst->setDirectSource(
                    toSrcIx,
                    srcMod,
                    opInfo.regName,
                    opInfo.regRef,
                    decRgn,
                    opInfo.type);
            }
        } else if (addrMode == GED_ADDR_MODE_Indirect) {
            RegRef a0 = {0, (uint8_t)decodeSrcAddrSubRegNum<S>()};
            int16_t addr_imm = decodeSrcAddrImm<S>();
            inst->setInidirectSource(
                toSrcIx,
                srcMod,
                a0,
                addr_imm,
                decRgn,
                decodeSrcType<S>());
        } else { // == GED_ADDR_MODE_INVALID
            fatal("invalid addressing mode in src%d", (int)S);
        }
    } else { // GED_REG_FILE_INVALID
        fatal("invalid register file in src%d", (int)S);
    }
}


template <SourceIndex S>
void DecoderBase::decodeSourceBasicAlign16(
    Instruction *inst, SourceIndex toSrcIx)
{
    GED_REG_FILE regFile = decodeSrcRegFile<S>();
    if (regFile == GED_REG_FILE_IMM) {
        // immediate operand
        Type type = decodeSrcType<S>();
        inst->setImmediateSource(toSrcIx, decodeSrcImmVal(type), type);
    } else if (regFile == GED_REG_FILE_ARF || regFile == GED_REG_FILE_GRF) {
        // register operand (direct or indirect)
        SrcModifier srcMod = decodeSrcModifier<S>();

        // reg and subreg (if direct)
        GED_ADDR_MODE addrMode = decodeSrcAddrMode<S>();

        // special context save/restore access to acc2-acc9
        uint32_t vs = decodeSrcVertStride<S>();

        if (addrMode == GED_ADDR_MODE_Direct) {
            DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<S>();
            if (inst->isMacro()) { // math macro operand (macro inst)
                if (!((vs == 2 && opInfo.type == Type::DF) ||
                    (vs == 4 && opInfo.type != Type::DF)))
                {
                    fatal("src%d: inconvertible align16 operand", (int)S);
                }
                // <GEN11 macros are stored in swizzle bits
                MathMacroExt MathMacroReg = decodeSrcMathMacroReg<S>();
                Region macroDftSrcRgn = macroDefaultSourceRegion(
                    (int)S,
                    inst->getOpSpec(),
                    m_model.platform,
                    inst->getExecSize());
                inst->setMacroSource(
                    toSrcIx,
                    srcMod,
                    opInfo.regName,
                    opInfo.regRef,
                    MathMacroReg,
                    macroDftSrcRgn,
                    opInfo.type);
            } else {
                if (vs != 4) {
                    fatal("src%d: inconvertible align16 operand", (int)S);
                }
                Region rgn = Region::SRC110;
                if (opInfo.regName == RegName::ARF_MME &&
                    isAlign16MathMacroRegisterCsrPlatform())
                {
                    // GEN8-9: context save and restore of acc3-9 hack
                    // (remember acc2 is Align1 and misses this path)
                    // So if we are here, it'll look like we just
                    // decoded "mme0" (acc2), but really we need to consult
                    // ChSel for the real mme# (acc#+2).
                    uint32_t chanSel = decodeSrcChanSel<S>() & 0xF;
                    // We do have to strip off the top bits of ChSel since
                    // it's really only ChSel[3:0].
                    //
                    // the ChSel[3:0] value is taken as interpreted as the
                    // mme register (e.g. we used to map 0 to 2 for acc2)
                    // mme's start at 0 (mme0 is acc2).
                    opInfo.regRef.regNum = (uint8_t)chanSel;
                    opInfo.regRef.subRegNum = 0;
                    // In GEN10, this all gets cleaned up and acc3 really is
                    //   RegName[7:4] = 0101b ("acc")
                    //   RegName[3:0] = 0011b ("3")
                    // (Which we map back to mme1.)  Moreover, that'll be
                    // Align1 code and this path won't be hit.
                } else {
                    // conversion of some other Align16 (e.g math macros)
                    if (isChanSelPacked<S>()) {
                        fatal("src%d: inconvertible align16 operand", (int)S);
                    }
                }
                inst->setDirectSource(
                    toSrcIx, srcMod, opInfo.regName, opInfo.regRef, rgn, opInfo.type);
            }
        } else if (addrMode == GED_ADDR_MODE_Indirect) {
            uint32_t vs = decodeSrcVertStride<S>();
            if (!isChanSelPacked<S>() && vs == 4) {
                fatal("src%d: inconvertible align16 operand", (int)S);
            }
            int32_t subRegNum = decodeSrcAddrSubRegNum<S>();
            int32_t addrImm = decodeSrcAddrImm<S>();
            RegRef indReg = {0, (uint8_t)subRegNum};
            inst->setInidirectSource(
                toSrcIx, srcMod, indReg,
                addrImm, Region::SRC110, decodeSrcType<S>());
        } else {
             // == GED_ADDR_MODE_INVALID
            fatal("src%d: invalid addressing mode", (int)S);
        }
    } else { // GED_REG_FILE_INVALID
        fatal("invalid register file in src%d", (int)S);
    }
}


void DecoderBase::decodeChSelToSwizzle(uint32_t chanSel, GED_SWIZZLE swizzle[4])
{
    GED_RETURN_VALUE status = GED_RETURN_VALUE_INVALID_FIELD;

    swizzle[0] = GED_GetSwizzleX(chanSel, m_gedModel, &status);
    if (status != GED_RETURN_VALUE_SUCCESS) {
        fatal("swizzle X could not be retrieved");
    }
    swizzle[1] = GED_GetSwizzleY(chanSel, m_gedModel, &status);
    if (status != GED_RETURN_VALUE_SUCCESS) {
        fatal("swizzle Y could not be retrieved");
    }
    swizzle[2] = GED_GetSwizzleZ(chanSel, m_gedModel, &status);
    if (status != GED_RETURN_VALUE_SUCCESS) {
        fatal("swizzle Z could not be retrieved");
    }
    swizzle[3] = GED_GetSwizzleW(chanSel, m_gedModel, &status);
    if (status != GED_RETURN_VALUE_SUCCESS) {
        fatal("swizzle W could not be retrieved");
    }
}

template <SourceIndex S>
bool DecoderBase::isChanSelPacked()
{
    uint32_t chanSel = decodeSrcChanSel<S>();
    GED_SWIZZLE swizzle[4];
    decodeChSelToSwizzle(chanSel, swizzle);
    return swizzle[0] != GED_SWIZZLE_x && swizzle[1] != GED_SWIZZLE_y &&
           swizzle[2] != GED_SWIZZLE_z && swizzle[3] != GED_SWIZZLE_w;
}

void DecoderBase::decodeThreadOptions(Instruction *inst, GED_THREAD_CTRL trdCntrl)
{
    switch (trdCntrl) {
        case GED_THREAD_CTRL_Atomic:
            inst->addInstOpt(InstOpt::ATOMIC);
            break;
        case GED_THREAD_CTRL_Switch:
            inst->addInstOpt(InstOpt::SWITCH);
            break;
        case GED_THREAD_CTRL_NoPreempt:
            inst->addInstOpt(InstOpt::NOPREEMPT);
            break;
        case GED_THREAD_CTRL_INVALID:
        default:
            break;
    }
}

template <SourceIndex S> ImmVal
DecoderBase::decodeTernarySrcImmVal(Type t)
{
    ImmVal val;
    val.kind = ImmVal::Kind::UNDEF;
    memset(&val, 0, sizeof(val)); // zero value in case GED only sets bottom bits

    if (S == SourceIndex::SRC0) {
        GED_DECODE_RAW_TO(Src0TernaryImm, val.u64);
    } else if (S == SourceIndex::SRC2) {
        GED_DECODE_RAW_TO(Src2TernaryImm, val.u64);
    } else {
        error("src1: immediate supported here on ternary instruction.");
    }

    setImmValKind(t, val);

    return val;
}

uint32_t DecoderBase::binNumToSubRegNum(
    uint32_t binNum, RegName regName, Type type)
{

    return BytesOffsetToSubReg(binNum, regName, type);
}

void DecoderBase::decodeOptions(Instruction *inst)
{
    const OpSpec &os = inst->getOpSpec();
    if (os.supportsAccWrEn(m_model.platform)) {
        // * GED doesn't allow AccWrEn on send's
        // * BrnchCtrl overlaps AccWrEn, so anything using that is out
        GED_ACC_WR_CTRL accWrEn = GED_ACC_WR_CTRL_Normal;
        GED_DECODE_RAW_TO(AccWrCtrl, accWrEn);
        if (accWrEn == GED_ACC_WR_CTRL_AccWrEn) {
            inst->addInstOpt(InstOpt::ACCWREN);
        }
    }

    if (os.supportsDebugCtrl()) {
        GED_DEBUG_CTRL debugCtrl = GED_DEBUG_CTRL_Normal;
        GED_DECODE_RAW_TO(DebugCtrl, debugCtrl);
        if (debugCtrl == GED_DEBUG_CTRL_Breakpoint) {
            inst->addInstOpt(InstOpt::BREAKPOINT);
        }
    }

    if (os.isSendOrSendsFamily()) {
        GED_EOT eot = GED_EOT_None;
        GED_DECODE_RAW_TO(EOT, eot);
        if (eot == GED_EOT_EOT) {
            inst->addInstOpt(InstOpt::EOT);
        }
    }

    if (os.supportsDepCtrl(m_model.platform)) {
        GED_DEP_CTRL dpCtrl = GED_DEP_CTRL_Normal;
        GED_DECODE_RAW_TO(DepCtrl, dpCtrl);
        if (dpCtrl == GED_DEP_CTRL_NoDDClr) {
            inst->addInstOpt(InstOpt::NODDCLR);
        } else if (dpCtrl == GED_DEP_CTRL_NoDDChk) {
            inst->addInstOpt(InstOpt::NODDCHK);
        } else if (dpCtrl == GED_DEP_CTRL_NoDDClr_NoDDChk) {
            inst->addInstOpt(InstOpt::NODDCLR);
            inst->addInstOpt(InstOpt::NODDCHK);
        }
    }

    if (GED_WORKAROUND(
        /* really need to get GED to support ThrCtrl on GEN7-8 send's */
            (!os.isSendOrSendsFamily() && os.supportsThreadCtrl()) ||
            (os.isSendOrSendsFamily() && m_model.platform >= Platform::GEN9)))
    {
        GED_THREAD_CTRL trdCntrl = GED_THREAD_CTRL_Normal;
        GED_DECODE_RAW_TO(ThreadCtrl, trdCntrl);
        decodeThreadOptions(inst, trdCntrl);
    }

    if (m_model.supportNoSrcDepSet() &&
        os.isSendOrSendsFamily())
    {
        GED_NO_SRC_DEP_SET srcDep;
        GED_DECODE_RAW_TO(NoSrcDepSet, srcDep);
        if (srcDep == GED_NO_SRC_DEP_SET_NoSrcDepSet) {
            inst->addInstOpt(InstOpt::NOSRCDEPSET);
        }
    }

    if (GED_IsCompact(&m_currGedInst)) {
        inst->addInstOpt(InstOpt::COMPACTED);
    }
}


Instruction *DecoderBase::createErrorInstruction(
    Kernel& kernel,
    const char *message,
    const void *binary,
    int32_t iLen)
{
    Instruction *inst = kernel.createIllegalInstruction();

    std::stringstream ss;
    ss << FormatOpBits(m_model, binary);
    if (*message) {
        ss << ": " << message;
    }
    size_t bufLen = (size_t)ss.tellp() + 1;
    char *buf = (char *)kernel.getMemManager().alloc(bufLen);
    ss.read(buf, bufLen - 1);
    buf[bufLen - 1] = 0;

    if (iLen == 8) {
        inst->addInstOpt(InstOpt::COMPACTED);
    }
    inst->setComment(buf);
    return inst;
}


uint32_t DecoderBase::getBitField(int ix, int len) const {
    const uint32_t *ws = (const uint32_t *)((const char *)m_binary + currentPc());
    // shift is only well-defined for values <32, use 0xFFFFFFFF
    uint32_t mask = len >= 32 ? 0xFFFFFFFF : (1<<(uint32_t)len) - 1;
    IGA_ASSERT(len <= 32 && ((ix + len - 1)/32 == ix/32),
        "getBitField: bitfield spans DWord");
    return (ws[ix / 32] >> (ix % 32)) & mask;
}


void DecoderBase::handleGedDecoderError(
    int line,
    const char *field,
    GED_RETURN_VALUE status)
{
      std::stringstream ss;
      if (status == GED_RETURN_VALUE_INVALID_VALUE) {
          // bad user bits -> report a warning
          ss << "GED reports invalid value for " << field;
      } else if (status == GED_RETURN_VALUE_INVALID_FIELD) {
          // our bad -> take it seriously
          ss << "GED reports invalid field for " << field << "(line " << line << ")";
      } else if (status != GED_RETURN_VALUE_SUCCESS) {
          // some other error -> our bad -> take it seriously and assert!
          ss << "GED reports invalid field for " << field << "(line " << line << ")";
          ss << "GED reports error (" << (int)status << ") accessing GED_" <<
              field << " (line " << line << ")";
      }
      ss << "\n";
      ss << FormatOpBits(m_model, (const char *)m_binary + currentPc());

      //std::cout << "pc[" << currentPc() << "] " << ss.str() << std::endl;
      if (status == GED_RETURN_VALUE_INVALID_VALUE) {
          error("%s", ss.str().c_str());
      } else {
          fatal("%s", ss.str().c_str());
      }
}

// These template class member functions are not defined in header (only
// declared in header), thus their definitions are not available to other
// .cpp.  We need to explicitly instantiate those template functions so
// the other .cpp can reference them.
template
void DecoderBase::decodeSourceBasicAlign16<SourceIndex::SRC0>(Instruction *inst, SourceIndex toSrcIx);
template
void DecoderBase::decodeSourceBasicAlign16<SourceIndex::SRC1>(Instruction *inst, SourceIndex toSrcIx);
template
void DecoderBase::decodeSourceBasicAlign1<SourceIndex::SRC0>(Instruction *inst, SourceIndex toSrcIx);
template
void DecoderBase::decodeSourceBasicAlign1<SourceIndex::SRC1>(Instruction *inst, SourceIndex toSrcIx);

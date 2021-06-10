/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BinaryEncodingIGA.h"
#include "GTGPU_RT_ASM_Interface.h"
#include "iga/IGALibrary/api/igaEncoderWrapper.hpp"
#include "Timer.h"
#include "BuildIR.h"

#include <map>
#include <utility>


using namespace iga;
using namespace vISA;

class BinaryEncodingIGA
{
    int               IGAInstId = 0;
    Mem_Manager&      mem;
    G4_Kernel&        kernel;
    std::string       fileName;
    iga::Kernel*      IGAKernel = nullptr;
    const iga::Model* platformModel;
    const TARGET_PLATFORM   platform;

public:
    BinaryEncodingIGA(vISA::Mem_Manager &m, vISA::G4_Kernel& k, std::string fname);
    ~BinaryEncodingIGA() {delete IGAKernel;}

    void SetSWSB(G4_INST * inst, iga::SWSB & sw);

    // translates and encodes (formerly "DoAll")
    void Encode();

    ///////////////////////////////////////////////////////////////////////////
    // these function translate G4 IR to IGA IR
    iga::Instruction *translateInstruction(G4_INST *g4inst, iga::Block*& bbNew);
    void translateInstructionDst(G4_INST *g4inst, iga::Instruction *igaInst);
    void translateInstructionBranchSrcs(G4_INST *g4inst, iga::Instruction *igaInst, iga::Block*& bbNew);
    void translateInstructionSrcs(G4_INST *g4inst, iga::Instruction *igaInst);

    void FixInst();
    void *EmitBinary(size_t& binarySize);

private:
    BinaryEncodingIGA(const BinaryEncodingIGA& other);
    BinaryEncodingIGA& operator=(const BinaryEncodingIGA& other);

    std::map<G4_Label*, iga::Block*> labelToBlockMap;

public:
    static iga::ExecSize       getIGAExecSize(int execSize);
    static iga::ChannelOffset  getIGAChannelOffset(int offset);
    static iga::MaskCtrl       getIGAMaskCtrl(bool noMask);
    static iga::RegName        getIGAARFName(G4_ArchRegKind areg);
    static iga::Type           getIGAType(G4_Type type, TARGET_PLATFORM genxPlatform);

    /// getIGAInternalPlatform - a helper function to transform visa platform to iga platform
    static iga::Platform       getIGAInternalPlatform(TARGET_PLATFORM genxPlatform);

    static std::pair<const iga::OpSpec *,iga::Subfunction> getIgaOpInfo(
        const G4_INST *inst, const iga::Model *m, bool allowUnknownOp, const IR_Builder& builder);
private:
    static iga::PredCtrl getIGAPredCtrl(G4_Predicate_Control g4PredCntrl);
    static iga::Predication getIGAPredication(G4_Predicate* predG4);
    static iga::BranchCntrl getIGABranchCntrl(bool isOn)
    {
        return isOn ? iga::BranchCntrl::ON : iga::BranchCntrl::OFF;
    }
    static iga::DstModifier getIGADstModifier(bool sat)
    {
        return sat ? iga::DstModifier::SAT : iga::DstModifier::NONE;
    }
    static iga::SrcModifier   getIGASrcModifier(G4_SrcModifier srcMod);
    static iga::Region::Vert  getIGAVert(int vstride);
    static iga::Region::Width getIGAWidth(int width);
    static iga::Region::Horz  getIGAHorz(int hstride);
    static iga::Region        getIGARegion(G4_SrcRegRegion* srcRegion, int srcPos);

    iga::MathMacroExt getIGAImplAcc(G4_AccRegSel accSel) const
    {
        switch (accSel)
        {
        case ACC2:      return iga::MathMacroExt::MME0;
        case ACC3:      return iga::MathMacroExt::MME1;
        case ACC4:      return iga::MathMacroExt::MME2;
        case ACC5:      return iga::MathMacroExt::MME3;
        case ACC6:      return iga::MathMacroExt::MME4;
        case ACC7:      return iga::MathMacroExt::MME5;
        case ACC8:      return iga::MathMacroExt::MME6;
        case ACC9:      return iga::MathMacroExt::MME7;
        case NOACC:     return iga::MathMacroExt::NOMME;
        default:
            assert(false && "illegal acc (mme) channel select");
            return iga::MathMacroExt::INVALID;
        }
    }
    iga::ImmVal::Kind getIGAImmType(G4_Type type)
    {
        switch (type)
        {
        case Type_UW:       return iga::ImmVal::Kind::U16;
        case Type_W:        return iga::ImmVal::Kind::S16;
        case Type_UD:       return iga::ImmVal::Kind::U32;
        case Type_D:        return iga::ImmVal::Kind::S32;
        case Type_UQ:       return iga::ImmVal::Kind::U64;
        case Type_Q:        return iga::ImmVal::Kind::S64;
        case Type_HF:       return iga::ImmVal::Kind::F16;
        case Type_F:        return iga::ImmVal::Kind::F32;
        case Type_DF:       return iga::ImmVal::Kind::F64;
        case Type_UV:
        case Type_V:
        case Type_VF:       return iga::ImmVal::Kind::U32;
        default:
            assert(false && "invalid immediate type");
            return iga::ImmVal::Kind::UNDEF;
        }
    }
    iga::InstOptSet getIGAInstOptSet(G4_INST* inst) const;

    iga::SendDesc getIGASendDesc(G4_INST* sendInst) const;
    iga::SendDesc getIGASendExDesc(
        G4_INST* sendInst, int& xlen, iga::InstOptSet& extraOpts) const;
    iga::SendDesc encodeExDescImm(
        G4_INST* sendInst, int& xlen, iga::InstOptSet& extraOpts) const;
    iga::SendDesc encodeExDescRegA0(
        G4_INST* sendInst, int& xlen, iga::InstOptSet& extraOpts) const;

    iga::RegName getIGARegName(G4_Operand* opnd) const
    {
        G4_VarBase *base = opnd->getBase();
        assert(base != nullptr && "base should not be null");
        if (base->isRegVar())
        {
            G4_VarBase *phyReg = base->asRegVar()->getPhyReg();
            return phyReg->isAreg() ? getIGAARFName(phyReg->asAreg()->getArchRegType()) : iga::RegName::GRF_R;
        }
        else
        {
            return base->isAreg() ? getIGAARFName(base->asAreg()->getArchRegType()) : iga::RegName::GRF_R;
        }
    }
    iga::RegRef getIGARegRef(G4_Operand* opnd) const
    {
        iga::RegRef regRef;
        G4_VarBase *base = opnd->getBase();
        assert(base != nullptr && "base should not be null");
        if (base->isGreg())
        {
            uint32_t byteAddress = opnd->getLinearizedStart();
            regRef.regNum = byteAddress / numEltPerGRF<Type_UB>();
            regRef.subRegNum =
                (byteAddress % numEltPerGRF<Type_UB>()) / opnd->getTypeSize();
        }
        else if (opnd->isSrcRegRegion())
        {
            bool valid, subvalid;
            regRef.regNum = (uint8_t)opnd->asSrcRegRegion()->ExRegNum(valid);
            regRef.subRegNum = (uint8_t)opnd->asSrcRegRegion()->ExSubRegNum(subvalid);
        }
        else
        {
            assert(opnd->isDstRegRegion() && "expect DstRegRegion");
            bool valid, subvalid;
            regRef.regNum = (uint8_t)opnd->asDstRegRegion()->ExRegNum(valid);
            regRef.subRegNum = (uint8_t)opnd->asDstRegRegion()->ExSubRegNum(subvalid);
        }
        return regRef;
    }

    iga::Block *lookupIGABlock(G4_Label* label, iga::Kernel& IGAKernel)
    {
        iga::Block *b = nullptr;
        auto itr = labelToBlockMap.find(label);
        if (itr == labelToBlockMap.end())
        {
            b = IGAKernel.createBlock();
            labelToBlockMap[label] = b;
        }
        else {
            b = itr->second;
        }
        return b;
    }

    void getIGAFlagInfo(
        G4_INST* inst,
        const iga::OpSpec* opSpec,
        iga::Subfunction sf,
        iga::Predication& pred,
        iga::FlagModifier& condMod,
        iga::RegRef& flagReg);

    iga::RegRef getIGAFlagReg(G4_VarBase* g4Base) const
    {
        iga::RegRef reg = iga::REGREF_INVALID;
        bool flagRegNumValid = true;
        reg.regNum = (uint8_t)g4Base->ExRegNum(flagRegNumValid);
        ASSERT_USER(flagRegNumValid, "Unable to retrieve flag Reg Num for predicate or conditional modifier.");
        reg.subRegNum = (uint8_t)g4Base->asRegVar()->getPhyRegOff();
        return reg;
    }

    iga::FlagModifier getIGAFlagModifier(G4_CondMod* cMod) const
    {
        if (cMod == nullptr)
        {
            return iga::FlagModifier::NONE;
        }

        G4_CondModifier mod = cMod->getMod();
        switch (mod)
        {
        case Mod_z:     // fallthrough
        case Mod_e:     return iga::FlagModifier::EQ;
        case Mod_nz:    // fallthrough
        case Mod_ne:    return iga::FlagModifier::NE;
        case Mod_g:     return iga::FlagModifier::GT;
        case Mod_ge:    return iga::FlagModifier::GE;
        case Mod_l:     return iga::FlagModifier::LT;
        case Mod_le:    return iga::FlagModifier::LE;
        case Mod_o:     // fallthrough
        case Mod_r:     return iga::FlagModifier::OV;
        case Mod_u:     return iga::FlagModifier::UN;
        default:
            ASSERT_USER(false, "Invalid FlagModifier.");
            return iga::FlagModifier::NONE;
        }
    }

    // get IGA type from GenPrecision
    iga::Type getIGAPrecisionType(GenPrecision p) const
    {
        switch (p)
        {
        case GenPrecision::U1:   return iga::Type::U1;
        case GenPrecision::U2:   return iga::Type::U2;
        case GenPrecision::U4:   return iga::Type::U4;
        case GenPrecision::U8:   return iga::Type::UB;
        case GenPrecision::S2:   return iga::Type::S2;
        case GenPrecision::S4:   return iga::Type::S4;
        case GenPrecision::S8:   return iga::Type::B;
        case GenPrecision::FP16: return iga::Type::HF;
        case GenPrecision::BF16: return iga::Type::BF;
        default:
            assert(false && "illegal Operand Precision");
            return iga::Type::INVALID;
        }
    }

    iga::Type getIGADpasType(G4_InstDpas* DpasInst, int SrcOprdIx) const
    {
        iga::Type ty;
        switch (SrcOprdIx) {
        default:
            MUST_BE_TRUE(false, "Invalid SrcOprdIx!");
            break;
        case 0:
        {
            G4_Operand* src0 = DpasInst->getSrc(0);
            if (src0->isNullReg()) {
                ty = getIGAType(DpasInst->getDst()->getType(), platform);
            }
            else
            {
                ty = getIGAType(src0->getType(), platform);
            }
            break;
        }
        case 1:
            ty = getIGAPrecisionType(DpasInst->getSrc1Precision());
            break;
        case 2:
            ty = getIGAPrecisionType(DpasInst->getSrc2Precision());
            break;
        }
        return ty;
    }

    iga::RegRef getIGADpasRegRef(G4_InstDpas* DpasInst, int SrcOprdIx) const
    {
        G4_Operand* src = DpasInst->getSrc(SrcOprdIx);
        iga::RegRef regref = getIGARegRef(src);
        if (SrcOprdIx == 2) {
            // By default, subRegNum is in terms of operand's type (D/UD for
            // dpas's src1/2). IGA needs it to be in terms of precision type.
            // Note that no need to do it for src1 as it must be grf-aligned!
            assert((regref.subRegNum % 2) == 0 &&
                "Minimum alignemnt of dpas's src2 must be QW");
            uint32_t bitOffsets = regref.subRegNum * src->getTypeSize() * 8;
            uint32_t PBits = G4_InstDpas::GetPrecisionSizeInBits(DpasInst->getSrc2Precision());
            regref.subRegNum = bitOffsets / PBits;
        }
        return regref;
    }

    static iga::BfnFC getBfnFC(const G4_INST *inst)
    {
        uint8_t funcCtrl = inst->asBfnInst()->getBooleanFuncCtrl();
        return iga::BfnFC(funcCtrl);
    }
    static iga::SFID getSFID(const G4_INST* inst);
    static iga::MathFC getMathFC(const G4_INST *inst);
    iga::Type getIGAType(const G4_INST* I, Gen4_Operand_Number O, TARGET_PLATFORM P);

    void *m_kernelBuffer;
    uint32_t m_kernelBufferSize;
}; // class BinaryEncodingIGA


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
        platform = Platform::XE;
        break;
    case XE_HP:
        platform = Platform::XE_HP;
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

    // Force instruction to be compacted if InstOpt::COMPACTED is given
    // even if autoCompact is not given to IGA
    if (inst->isCompactedInst())
    {
        options.add(iga::InstOpt::COMPACTED);
    }

    // Force instruction to be nocompacted
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

    G4_SendDesc *msgDesc = inst->getMsgDesc();
    auto funcID = msgDesc->getSFID();

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
    case vISA::SFID::DP_DC0:    sfid = iga::SFID::DC0; break;
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


//
// Return the IGA op for the given vISA instruction <op> for platform p.
// <inst> is sometimes necessary to compute the subopcode (e.g., send)
// As vISA may call this function to access instruction properties such as
// saturation and conditional modifier and this may happen before all pseudo
// opperations are lowered, <allowUnknownOp> may be used to suppress assert;
// an invalid will be returned in this case.
//
std::pair<const iga::OpSpec *,iga::Subfunction> BinaryEncodingIGA::getIgaOpInfo(
    const G4_INST *inst, const iga::Model *m, bool allowUnknownOp, const IR_Builder& builder)
{
    iga::Platform p = m->platform;
    iga::Op igaOp = iga::Op::INVALID;
    iga::Subfunction sf = iga::InvalidFC::INVALID;
    switch (inst->opcode())
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
    case G4_call:
    {
        igaOp = iga::Op::CALL;
        // check if we're using calla for indirect call
        if (builder.supportCallaRegSrc()) {
            // check if we're doing indiret call
            if (inst->getSrc(0)->isGreg() || inst->getSrc(0)->isA0())
                igaOp = iga::Op::CALLA;
        }
        break;
    }
    case G4_return:  igaOp = iga::Op::RET; break;
    case G4_goto:    igaOp = iga::Op::GOTO; break;
    case G4_join:    igaOp = iga::Op::JOIN; break;
    case G4_wait:
        if (p >= iga::Platform::XE)
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
        if (p >= iga::Platform::XE) {
            // G4 IR still calls send sends after Xe
            igaOp = iga::Op::SEND;
        } else {
            igaOp = iga::Op::SENDS;
        }
        break;
    case G4_sendsc:
        sf = getSFID(inst);
        if (p >= iga::Platform::XE) {
            // G4 IR still calls send sends after Xe
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
    case G4_dpas:
    case G4_dpasw:
    {
        igaOp = inst->opcode() == G4_dpasw ? iga::Op::DPASW : iga::Op::DPAS;
        G4_InstDpas* dpasInst = inst->asDpasInst();
        uint8_t D = dpasInst->getSystolicDepth();
        uint8_t C = dpasInst->getRepeatCount();
        sf = iga::GetDpasFC(D, C);
        break;
    }
    case G4_add3:    igaOp = iga::Op::ADD3; break;
    case G4_bfn:
        igaOp = iga::Op::BFN;
        sf = getBfnFC(inst);
        break;
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
        ASSERT_USER(!allowUnknownOp, "G4_do is not GEN ISA OPCODE.");
        break;
    case G4_pseudo_and:   igaOp = iga::Op::AND; break;
    case G4_pseudo_or:    igaOp = iga::Op::OR; break;
    case G4_pseudo_xor:   igaOp = iga::Op::XOR; break;
    case G4_pseudo_not:   igaOp = iga::Op::NOT; break;
    case G4_pseudo_fcall: igaOp = iga::Op::CALL; break;
    case G4_pseudo_fret:  igaOp = iga::Op::RET; break;
    case G4_pseudo_sada2: igaOp = iga::Op::SADA2; break;
    case G4_pseudo_exit:
        ASSERT_USER(!allowUnknownOp, "G4_pseudo_exit not GEN ISA OPCODE.");
        break;
    case G4_pseudo_fc_call: igaOp = iga::Op::CALL; break;
    case G4_pseudo_fc_ret:  igaOp = iga::Op::RET; break;
    case G4_intrinsic:
        ASSERT_USER(!allowUnknownOp, "G4_intrinsic not GEN ISA OPCODE.");
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
        assert(false);
        break;
    case G4_mulh:
        ASSERT_USER(!allowUnknownOp, "G4_mulh is not GEN ISA OPCODE.");
        break;
    case G4_madw:
        ASSERT_USER(!allowUnknownOp, "G4_madw not GEN ISA OPCODE.");
        break;
    default:
        ASSERT_USER(!allowUnknownOp, "INVALID opcode.");
        break;
    }
    const iga::OpSpec *os = &m->lookupOpSpec(igaOp);
    return std::pair<const iga::OpSpec *,iga::Subfunction>(os, sf);
}

void BinaryEncodingIGA::SetSWSB(G4_INST *inst, iga::SWSB &sw)
{
    // Set token, e.g. $0
    using SWSBTokenType = vISA::G4_INST::SWSBTokenType;
    if (inst->tokenHonourInstruction() && (inst->getTokenType() == SWSBTokenType::SB_SET))
    {
        sw.tokenType = SWSB::TokenType::SET;
        sw.sbid = inst->getToken();
    }

    // Set distance, e.g. A@1
    using DistanceType = vISA::G4_INST::DistanceType;
    if ((unsigned)inst->getDistance())
    {
        // check the distance type for multi-dist-pipes
        if (kernel.fg.builder->hasThreeALUPipes() ||
            kernel.fg.builder->hasFourALUPipes()) {
            switch (inst->getDistanceTypeXe())
            {
            case DistanceType::DIST:
                sw.distType = SWSB::DistType::REG_DIST;
                break;
            case DistanceType::DISTALL:
                sw.distType = SWSB::DistType::REG_DIST_ALL;
                break;
            case DistanceType::DISTINT:
                sw.distType = SWSB::DistType::REG_DIST_INT;
                break;
            case DistanceType::DISTFLOAT:
                sw.distType = SWSB::DistType::REG_DIST_FLOAT;
                break;
            case DistanceType::DISTLONG:
                sw.distType = SWSB::DistType::REG_DIST_LONG;
                break;
            default:
                break;
            }
        }
        else
        {
            // there is only one pipe on single-dist-pipe platform,
            // must be REG_DIST
            sw.distType = SWSB::DistType::REG_DIST;
        }
        sw.minDist = (uint32_t)inst->getDistance();
    }

    // Set token dependency, e.g. $1.src
    if (inst->getTokenType() == SWSBTokenType::AFTER_READ ||
        inst->getTokenType() == SWSBTokenType::AFTER_WRITE)
    {
        uint8_t token = (uint8_t)inst->getToken();
        if (inst->getTokenType() == SWSBTokenType::AFTER_READ)
        {
            sw.tokenType = SWSB::TokenType::SRC;
        }
        else if (inst->getTokenType() == SWSBTokenType::AFTER_WRITE)
        {
            sw.tokenType = SWSB::TokenType::DST;
        }
        sw.sbid = token;
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
        size_t num_inst = first_bb->size();
        assert(num_inst != 0 && "the first BB must not be empty");
        // label instructions don't count. Only the first instruction could be a label
        if (first_bb->front()->isLabel())
            --num_inst;

        if (num_inst % 4 != 0) {
            size_t num_nop = 4 - (num_inst % 4);
            for (size_t i = 0; i < num_nop; ++i)
                first_bb->push_back(
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

            if (getPlatformGeneration(platform) >= PlatformGen::XE) {
                iga::SWSB sw;
                SetSWSB(inst, sw);

                SWSB::InstType instTy = SWSB::InstType::UNKNOWN;
                if (inst->isMath())
                    instTy = SWSB::InstType::MATH;
                else if (inst->isDpas())
                    instTy = SWSB::InstType::DPAS;
                else if (inst->isSend())
                    instTy = SWSB::InstType::SEND;
                else
                    instTy = SWSB::InstType::OTHERS;

                // Verify if swsb is in encode-able dist and token combination
                if (!sw.verify(GetIGASWSBEncodeMode(*kernel.fg.builder), instTy))
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


    { // time the encoding
        TIME_SCOPE(IGA_ENCODER);
        bool autoCompact = kernel.getOption(vISA_Compaction);

        KernelEncoder encoder(IGAKernel, autoCompact);
        encoder.setSWSBEncodingMode(GetIGASWSBEncodeMode(*kernel.fg.builder));

        if (kernel.getOption(vISA_EnableIGASWSB))
        {
            encoder.enableIGAAutoDeps();
        }

        encoder.encode();

        m_kernelBufferSize = encoder.getBinarySize();
        m_kernelBuffer = allocCodeBlock(m_kernelBufferSize);
        memcpy_s(m_kernelBuffer, m_kernelBufferSize, encoder.getBinary(), m_kernelBufferSize);
    }

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
        assert(iter != secondBB->end() && "expect at least one non-label inst in second BB");
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
    auto opinfo = getIgaOpInfo(g4inst, platformModel, false, *kernel.fg.builder);
    // common fields: op, predicate, flag reg, exec size, exec mask offset,
    // mask ctrl, conditional modifier
    const OpSpec* opSpec = opinfo.first;
    if (opSpec == nullptr || !opSpec->isValid())
    {
        std::cerr << "INVALID opcode " << G4_Inst_Table[g4inst->opcode()].str << "\n";
        ASSERT_USER(false, "INVALID OPCODE.");
        return nullptr;
    }
    Op igaOp = opSpec->op;
    Subfunction sf = opinfo.second;
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
        igaOp != iga::Op::CALLA &&
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
    Type type = getIGAType(g4inst, Opnd_dst, platform);

    // workaround for SKL bug
    // not all bits are copied from immediate descriptor
    if (g4inst->isSend() && platform >= GENX_SKL && platform < GENX_ICLLP)
    {
        const G4_SendDescRaw* msgDesc = g4inst->getMsgDescRaw();
        assert(msgDesc && "expected raw descriptor");
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

        if (numSrcToEncode == 1 && platformModel->platform >= Platform::XE)
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
        SourceIndex opIx = SourceIndex::SRC0;
        switch (i) {
        case 0: opIx = SourceIndex::SRC0; break;
        case 1: opIx = SourceIndex::SRC1; break;
        case 2: opIx = SourceIndex::SRC2; break;
        default:
            assert(0 && "invalid source index number");
            break;
        }

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
                type = getIGAType(inst, inst->getSrcOperandNum(i), platform);
            }
            else if (i == 0 && platform >= GENX_SKL && platform < GENX_ICLLP)
            {
                // work around for SKL bug
                // not all bits are copied from immediate descriptor
                G4_SendDescRaw* msgDesc = inst->getMsgDescRaw();
                assert(msgDesc && "expected raw descriptor");
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
            else if (inst->isDpas())
            {
                assert(srcRegion->getRegAccess() == Direct &&
                    "dpas does not support indirect GRF operands");
                G4_InstDpas* dpasInst = inst->asDpasInst();
                RegRef regRef = getIGADpasRegRef(dpasInst, i);
                type = getIGADpasType(dpasInst, i);

                igaInst->setDirectSource(
                    opIx,
                    srcMod,
                    getIGARegName(srcRegion),
                    regRef,
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
            Type type = getIGAType(src->getType(), platform);
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

///////////////////////////////////////////////////////////////////////////////
// ExDesc encoding
static SendDesc encodeExDescSendUnary(
    G4_INST* sendInst, int& xlen, InstOptSet& extraOpts)
{
    SendDesc exDescIga;

    // old unary packed send
    // exDesc is stored in SendMsgDesc and must be IMM
    G4_SendDescRaw* descG4 = sendInst->getMsgDescRaw();
    assert(descG4 != nullptr && "expected raw send");

    exDescIga.type = SendDesc::Kind::IMM;
    uint32_t tVal = descG4->getExtendedDesc();

    if (getPlatformGeneration(sendInst->getPlatform()) >= PlatformGen::XE)
    {
        // We must clear the funcID in the extended message.
        // In Xe+ this is part of the EU encoding, not the descriptor.
        // vISA/G4IR still treat it as part of the descriptor.
        tVal = tVal & 0xFFFFFFF0;

        // clear the EOT bit which is not part of exDesc
        tVal &= ~(1 << 5);
    }
    exDescIga.imm = tVal;

    // non-split send implies Src1.Length == 0
    xlen = 0;

    return exDescIga;
}

////////////////////////////////////////////////////////////////////
// these handle binary sends (old "sends" and Xe+ "send"
//
SendDesc BinaryEncodingIGA::encodeExDescImm(
    G4_INST* sendInst,
    int& xlen,
    InstOptSet& extraOpts) const
{
    SendDesc exDescIga;

    G4_Operand* exDescG4 = sendInst->getSrc(3);
    G4_SendDescRaw* descG4 = sendInst->getMsgDescRaw();
    assert(descG4 != nullptr && "expected raw descriptor");

    xlen = (int)descG4->extMessageLength();
    //
    exDescIga.type = SendDesc::Kind::IMM;
    exDescIga.imm = (uint32_t)exDescG4->asImm()->getImm();
    // We must clear the funcID in the extended message for Xe+
    // because it is part of the EU instruction, not the descriptor,
    // and, vISA/G4-IR still thinks of it as part of the descriptor.
    //
    // Ditto for the EOT bit which is moved out of extDesc
    //
    // The extended message format
    // struct ExtendedMsgDescLayout {
    //    uint32_t funcID : 4;       // bit 0:3 << not part of ExDesc
    //    uint32_t unnamed1 : 1;     // bit 4
    //    uint32_t eot : 1;          // bit 5 << not part of ExDesc
    //    uint32_t extMsgLength : 5; // bit 6:10
    //    uint32_t unnamed2 : 5;     // bit 11:15
    //    uint32_t extFuncCtrl : 16; // bit 16:31
    // };
    if (getPlatformGeneration(sendInst->getPlatform()) >= PlatformGen::XE)
    {
        exDescIga.imm &= 0xFFFFFFC0;
    }

    // clear the EOT bit which is not part of exDesc on XE+
    if (getPlatformGeneration(sendInst->getPlatform()) >= PlatformGen::XE)
        exDescIga.imm &= ~(1 << 5);

    return exDescIga;
}

iga::SendDesc BinaryEncodingIGA::encodeExDescRegA0(
    G4_INST* sendInst, int& xlen, iga::InstOptSet& extraOpts) const
{
    SendDesc exDescIga;

    G4_Operand* exDescG4 = sendInst->getSrc(3);
    const G4_SendDescRaw* descG4 = sendInst->getMsgDescRaw();
    assert(descG4 != nullptr && "expected raw descriptor");

    exDescIga.type = SendDesc::Kind::REG32A;
    exDescIga.reg.regNum = 0; // must be a0
    bool valid = false;
    exDescIga.reg.subRegNum =
        (uint16_t)exDescG4->asSrcRegRegion()->ExSubRegNum(valid);
    assert(valid && "invalid subreg");

    if (kernel.fg.builder->useNewExtDescFormat() && descG4->isCPSEnabled()) {
        // CPS is an instruction option if using RegDesc+ExBSO
        extraOpts.add(InstOpt::CPS);
    }

    // By default all RegDesc in the new descriptor format will use
    // the ExBSO model if at all possible
    bool encodeExBso = kernel.fg.builder->useNewExtDescFormat();
    if (encodeExBso)
        extraOpts.add(InstOpt::EXBSO);

    // G4 IR keeps Src1.Length (xlen) separate.  So it's known,
    // (even with a reg desc in nonExBSO mode)
    xlen = (int)descG4->extMessageLength();

    return exDescIga;
}

SendDesc BinaryEncodingIGA::getIGASendExDesc(
    G4_INST* sendInst, int& xlen, iga::InstOptSet& extraOpts) const
{
    assert(sendInst->isSend() && "expect send inst");

    if (sendInst->isEOT())
        extraOpts.add(InstOpt::EOT);

    xlen = -1;

    if (sendInst->isSplitSend())
    {
        const G4_Operand* exDesc = sendInst->getSrc(3);
        return exDesc->isImm() ?
            encodeExDescImm(sendInst, xlen, extraOpts) :
            encodeExDescRegA0(sendInst, xlen, extraOpts);
    }
    else
    {
        return encodeExDescSendUnary(sendInst, xlen, extraOpts);
    }
}

void *BinaryEncodingIGA::EmitBinary(size_t& binarySize)
{
    binarySize = m_kernelBufferSize;

    if (kernel.getOption(vISA_GenerateBinary))
    {
        std::string binFileName = fileName + ".dat";
        std::string errStr;
        std::ofstream os(binFileName.c_str(), std::ios::binary);
        if (!os)
        {
            errStr = "Can't open " + binFileName + ".\n";
            MUST_BE_TRUE(0, errStr);
            return nullptr;
        }
        os.write((const char*)m_kernelBuffer, binarySize);
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

iga::Type BinaryEncodingIGA::getIGAType(const G4_INST* I, Gen4_Operand_Number O, TARGET_PLATFORM P)
{

    G4_Type Ty = I->getOperand(O)->getType();
    return getIGAType(Ty, P);
}

iga::Type BinaryEncodingIGA::getIGAType(G4_Type type, TARGET_PLATFORM genxPlatform)
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
    case Type_BF:   return iga::Type::BF;
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

EncodeResult vISA::EncodeKernelIGA(
    vISA::Mem_Manager &m,
    vISA::G4_Kernel& k,
    const std::string &fname)
{
    EncodeResult r;
    BinaryEncodingIGA encoder(m, k, fname);
    encoder.Encode();
    r.binary = encoder.EmitBinary(r.binaryLen);
    return r;
}

SWSB_ENCODE_MODE vISA::GetIGASWSBEncodeMode(const IR_Builder& builder) {
    if (getPlatformGeneration(builder.getPlatform()) < PlatformGen::XE)
        return SWSB_ENCODE_MODE::SWSBInvalidMode;


    return SWSB_ENCODE_MODE::SingleDistPipe;
}

static const iga::Model *GetModel(TARGET_PLATFORM p)
{
    const iga::Model *m = iga::Model::LookupModel(
        BinaryEncodingIGA::getIGAInternalPlatform(p));
    return m;
}

bool vISA::InstSupportsSaturationIGA(TARGET_PLATFORM p, const G4_INST &i, const IR_Builder& builder)
{
    const iga::Model *m = GetModel(p);
    if(m)
    {
        auto oi = BinaryEncodingIGA::getIgaOpInfo(&i, m, true, builder);
        return oi.first && oi.first->isValid() && oi.first->supportsSaturation();
    }
    else
    {
        return false;
    }
}

bool vISA::InstSupportsSrcModifierIGA(TARGET_PLATFORM p, const G4_INST &i, const IR_Builder& builder)
{
    const iga::Model *m = GetModel(p);
    if(m)
    {
        auto oi = BinaryEncodingIGA::getIgaOpInfo(&i, m, true, builder);
        return oi.first && oi.first->isValid() && oi.first->supportsSourceModifiers();
    }
    else
    {
        return false;
    }
}


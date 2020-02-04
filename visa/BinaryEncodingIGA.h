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

#ifndef _BINARYENCODINGIGA_H_
#define _BINARYENCODINGIGA_H_

#include <map>
#include "Gen4_IR.hpp"
#include "iga/IGALibrary/IR/Kernel.hpp"
#include "iga/IGALibrary/Models/Models.hpp"
#include "FlowGraph.h"
#include "iga/IGALibrary/IR/Instruction.hpp"
#include "BinaryEncodingIGA.h"
#include "iga/IGALibrary/IR/Types.hpp"
#include "iga/IGALibrary/api/iga.h"

using namespace vISA;

class BinaryEncodingIGA
{
    int IGAInstId = 0;
    Mem_Manager     &mem;
    G4_Kernel&      kernel;
    std::string fileName;
    iga::Kernel*    IGAKernel;
    const iga::Model* platformModel;

public:
    BinaryEncodingIGA(vISA::Mem_Manager &m, vISA::G4_Kernel& k, std::string fname);
    virtual ~BinaryEncodingIGA()
    {
        delete IGAKernel;
    }

    void SetSWSB(G4_INST * inst, iga::SWSB & sw);

    void DoAll();
    void FixInst();
    void *EmitBinary(uint32_t& binarySize);

private:
    BinaryEncodingIGA(const BinaryEncodingIGA& other);
    BinaryEncodingIGA& operator=(const BinaryEncodingIGA& other);

    iga::Instruction *encodeMathInstruction(G4_INST *inst);
    iga::Instruction *encodeBranchInstruction(G4_INST *inst);
    iga::Instruction *encodeTernaryInstruction(G4_INST *inst);
    iga::Instruction *encodeSendInstruction(G4_INST *inst);
    iga::Instruction *encodeSplitSendInstruction(G4_INST *inst);

    std::map<G4_Label*, iga::Block*> labelToBlockMap;

    iga::ExecSize getIGAExecSize(int execSize) const
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
    iga::ChannelOffset getIGAChannelOffset(int offset) const
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
    iga::MaskCtrl getIGAMaskCtrl(bool noMask) const
    {
        return noMask ? iga::MaskCtrl::NOMASK : iga::MaskCtrl::NORMAL;
    }
    iga::RegName getIGAARFName(G4_ArchRegKind areg) const
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
        case AREG_F1:       return iga::RegName::ARF_F;
        case AREG_TM0:      return iga::RegName::ARF_TM;
        case AREG_TDR0:     return iga::RegName::ARF_TDR;
        case AREG_SP:       return iga::RegName::ARF_SP;
        default:
            assert(false && "illegal ARF");
            return iga::RegName::INVALID;
        }
    }

public:
    static iga::Type getIGAType(G4_Type type)
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

    /// getIGAInternalPlatform - a helper function to transform visa platform to iga platform
    static iga::Platform getIGAInternalPlatform(TARGET_PLATFORM genxPlatform);

    static iga::SWSB_ENCODE_MODE getIGASWSBEncodeMode(const IR_Builder& builder);

    static iga::Op getIGAOp(G4_opcode op, const G4_INST* inst, iga::Platform iga_platform);
private:

    iga::PredCtrl getIGAPredCtrl(G4_Predicate_Control g4PredCntrl) const
    {
        switch (g4PredCntrl)
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

    iga::Predication getIGAPredication(G4_Predicate* predG4) const
    {
        iga::Predication pred;
        if (predG4)
        {
            pred.function = getIGAPredCtrl(predG4->getControl());
            pred.inverse = (predG4->getState() == PredState_Plus) ? false : true;
        }
        return pred;
    }
    iga::BranchCntrl getIGABranchCntrl(bool isOn) const
    {
        return isOn ? iga::BranchCntrl::ON : iga::BranchCntrl::OFF;
    }
    iga::DstModifier getIGADstModifier(bool sat) const
    {
        return sat ? iga::DstModifier::SAT : iga::DstModifier::NONE;
    }
    iga::SrcModifier getIGASrcModifier(G4_SrcModifier srcMod) const
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
    iga::Region::Vert getIGAVert(int vstride) const
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
    iga::Region::Width getIGAWidth(int width) const
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
    iga::Region::Horz getIGAHorz(int hstride) const
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
    iga::Region getIGARegion(G4_SrcRegRegion* srcRegion, int srcPos) const
    {
        iga::Region igaRegion;
        const RegionDesc* region = srcRegion->getRegion();
        if ((srcRegion->getInst()->getNumSrc() == 3 && !srcRegion->getInst()->isSend()))
        {
            // special handling for 3src instructions
            if (srcPos != 2)
            {
                // for src0 and src1, IGA/GED does not like width to be set
                igaRegion.set(getIGAVert(region->vertStride), iga::Region::Width::WI_INVALID, getIGAHorz(region->horzStride));
            }
            else
            {
                // for src2, IGA expects both VS and W to be invalid
                igaRegion.set(iga::Region::Vert::VT_INVALID, iga::Region::Width::WI_INVALID, getIGAHorz(region->horzStride));
            }
        }
        else
        {
            igaRegion.set(getIGAVert(region->vertStride), getIGAWidth(region->width), getIGAHorz(region->horzStride));
        }
        return igaRegion;
    }
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
        case Type_UW:       return iga::ImmVal::U16;
        case Type_W:        return iga::ImmVal::S16;
        case Type_UD:       return iga::ImmVal::U32;
        case Type_D:        return iga::ImmVal::S32;
        case Type_UQ:       return iga::ImmVal::U64;
        case Type_Q:        return iga::ImmVal::S64;
        case Type_HF:       return iga::ImmVal::F16;
        case Type_F:        return iga::ImmVal::F32;
        case Type_DF:       return iga::ImmVal::F64;
        case Type_UV:
        case Type_V:
        case Type_VF:       return iga::ImmVal::U32;
        default:
            assert(false && "invalid immediate type");
            return iga::ImmVal::UNDEF;
        }
    }
    iga::InstOptSet getIGAInstOptSet(G4_INST* inst) const;

    iga::SendDescArg getIGASendDescArg(G4_INST* sendInst) const;
    iga::SendDescArg getIGASendExDescArg(G4_INST* sendInst) const;
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
            regRef.regNum = byteAddress / GENX_GRF_REG_SIZ;
            regRef.subRegNum = (byteAddress % GENX_GRF_REG_SIZ) / G4_Type_Table[opnd->getType()].byteSize;
        }
        else if (opnd->isSrcRegRegion())
        {
            bool valid, subvalid;
            regRef.regNum = (uint8_t) opnd->asSrcRegRegion()->ExRegNum(valid);
            regRef.subRegNum = (uint8_t) opnd->asSrcRegRegion()->ExSubRegNum(subvalid);
        }
        else
        {
            assert(opnd->isDstRegRegion() && "expect DstRegRegion");
            bool valid, subvalid;
            regRef.regNum = (uint8_t) opnd->asDstRegRegion()->ExRegNum(valid);
            regRef.subRegNum = (uint8_t) opnd->asDstRegRegion()->ExSubRegNum(subvalid);
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
        G4_INST* inst, const iga::OpSpec* opSpec, iga::Predication& pred,
        iga::FlagModifier& condMod, iga::RegRef& flagReg);

    iga::RegRef getIGAFlagReg(G4_VarBase* g4Base)
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
        case Mod_z:     //fallthru
        case Mod_e:     return iga::FlagModifier::EQ;
        case Mod_nz:    //fallthru
        case Mod_ne:    return iga::FlagModifier::NE;
        case Mod_g:     return iga::FlagModifier::GT;
        case Mod_ge:    return iga::FlagModifier::GE;
        case Mod_l:     return iga::FlagModifier::LT;
        case Mod_le:    return iga::FlagModifier::LE;
        case Mod_o:     // fallthru
        case Mod_r:     return iga::FlagModifier::OV;
        case Mod_u:     return iga::FlagModifier::UN;
        default:
            ASSERT_USER(false, "Invalid FlagModifier.");
            return iga::FlagModifier::NONE;
        }
    }

    static iga::Op getIGAOpFromSFIDForSend(G4_opcode op, const G4_INST* inst);

    static iga::Op getIGAMathOp(const G4_INST *inst)
    {

        G4_MathOp mathControlValue = inst->asMathInst()->getMathCtrl();

        switch (mathControlValue)
        {
        case MATH_INV:          return iga::Op::MATH_INV;
        case MATH_LOG:          return iga::Op::MATH_LOG;
        case MATH_EXP:          return iga::Op::MATH_EXP;
        case MATH_SQRT:         return iga::Op::MATH_SQT;
        case MATH_RSQ:          return iga::Op::MATH_RSQT;
        case MATH_SIN:          return iga::Op::MATH_SIN;
        case MATH_COS:          return iga::Op::MATH_COS;
        case MATH_FDIV:         return iga::Op::MATH_FDIV;
        case MATH_POW:          return iga::Op::MATH_POW;
        case MATH_INT_DIV:      return iga::Op::MATH_IDIV;
        case MATH_INT_DIV_QUOT: return iga::Op::MATH_IQOT;
        case MATH_INT_DIV_REM:  return iga::Op::MATH_IREM;
        case MATH_INVM:         return iga::Op::MATH_INVM;
        case MATH_RSQRTM:       return iga::Op::MATH_RSQTM;
        default:
            ASSERT_USER(false, "Invalid MathControl.");
            return iga::Op::INVALID;
        }
    }

    void *m_kernelBuffer;
    uint32_t m_kernelBufferSize;
};

#endif //_BINARYENCODINGIGA_H_

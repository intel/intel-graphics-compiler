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

#include "Gen4_IR.hpp"
#include "iga/IGALibrary/IR/Kernel.hpp"
#include "iga/IGALibrary/Models/Models.hpp"
#include "FlowGraph.h"
#include "iga/IGALibrary/IR/Instruction.hpp"
#include "BinaryEncodingIGA.h"
#include "iga/IGALibrary/IR/Types.hpp"
#include "iga/IGALibrary/api/iga.h"

#include <map>
#include <utility>


using namespace vISA;

class BinaryEncodingIGA
{
    int               IGAInstId = 0;
    Mem_Manager&      mem;
    G4_Kernel&        kernel;
    std::string       fileName;
    iga::Kernel*      IGAKernel;
    const iga::Model* platformModel;
    const TARGET_PLATFORM   platform;

public:
    BinaryEncodingIGA(vISA::Mem_Manager &m, vISA::G4_Kernel& k, std::string fname);
    virtual ~BinaryEncodingIGA()
    {
        delete IGAKernel;
    }

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
    void *EmitBinary(uint32_t& binarySize);

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

    static iga::SWSB_ENCODE_MODE getIGASWSBEncodeMode(const IR_Builder& builder);

    static std::pair<iga::Op,iga::Subfunction> getIgaOpInfo(
        G4_opcode op, const G4_INST *inst, iga::Platform p, bool allowUnknownOp);
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

    iga::SendDesc getIGASendDesc(G4_INST* sendInst) const;
    iga::SendDesc getIGASendExDesc(
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
            regRef.regNum = byteAddress / numEltPerGRF(Type_UB);
            regRef.subRegNum = (byteAddress % numEltPerGRF(Type_UB)) / G4_Type_Table[opnd->getType()].byteSize;
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

    static iga::SFID getSFID(const G4_INST* inst);
    static iga::MathFC getMathFC(const G4_INST *inst);

    void *m_kernelBuffer;
    uint32_t m_kernelBufferSize;
}; // class BinaryEncodingIGA

#endif //_BINARYENCODINGIGA_H_

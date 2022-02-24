/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BinaryEncoding.h"
#include "BuildIR.h"

using namespace vISA;

// Initlaize the platform dependent bit positions to some illegal value
// They will be set to the correct value by the Init() function
// Technically these should be TLS, but I'm counting on the fact that
// these values will be fixed for a given platform, and an application
// should never invoke the jitter twice with different platform strings.
// Initlaize the platform dependent bit positions to some illegal value
// They will be set to the correct value by the Init() function
// Technically these should be TLS, but I'm counting on the fact that
// these values will be fixed for a given platform, and an application
// should never invoke the jitter twice with different platform strings.
unsigned long bitsFlagSubRegNum[] = {128, 128};
unsigned long bitsNibCtrl[] = {128, 128};
unsigned long bits3SrcFlagSubRegNum[] = {128, 128};
unsigned long bits3SrcSrcType[] = {128, 128};
unsigned long bits3SrcDstType[] = {128, 128};
unsigned long bits3SrcNibCtrl[] = {128, 128};
unsigned long bits3SrcDstRegFile[] = {128, 128};

unsigned long bitsDepCtrl[] = {128, 128};
unsigned long bitsWECtrl[] = {128, 128};
unsigned long bitsDstRegFile[] = {128, 128};
unsigned long bitsDstType[] = {128, 128};
unsigned long bitsDstIdxRegNum[] = {128, 128};
unsigned long bitsDstIdxImmOWord[] = {128, 128};
unsigned long bitsDstIdxImmByte[]   = {128, 128};
unsigned long bitsDstIdxImmMSB[] = {128, 128};
unsigned long bitsSrcType[] = {128, 128, 128, 128};
unsigned long bitsSrcIdxRegNum[] = {128, 128, 128, 128};
unsigned long bitsSrcIdxImmOWord[]  = {128, 128, 128, 128};
unsigned long bitsSrcIdxImmByte[] = {128, 128, 128, 128};
unsigned long bitsSrcIdxImmMSB[]    = {128, 128, 128, 128};
unsigned long bitsJIP[] = {128, 128};
unsigned long bitsUIP[] = {128, 128};
unsigned long bits3SrcSrcMod[]  = {128, 128, 128, 128, 128, 128};

/************************************************************************/
/* Auxiliary                                                            */
/************************************************************************/

inline void SetOpCode(BinInst *mybin, uint32_t value)
{
    mybin->SetBits(bitsOpCode_0, bitsOpCode_1, value);
}

inline void BinaryEncoding::EncodeOpCode(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    G4_opcode opcode = inst->opcode();
    uint32_t euopc = getEUOpcode(opcode);
    SetOpCode(mybin, euopc);
}

inline void BinaryEncoding::EncodeExecSize(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    mybin->SetBits(bitsExecSize_0, bitsExecSize_1, GetEncodeExecSize(inst));
}

inline void BinaryEncoding::EncodeAccessMode(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();

    if (inst->isAligned1Inst()) {
        mybin->SetBits(bitsAccessMode_0, bitsAccessMode_1, ACCESS_MODE_ALIGN1);
    }
    else if (inst->isAligned16Inst()) {
        mybin->SetBits(bitsAccessMode_0, bitsAccessMode_1, ACCESS_MODE_ALIGN16);
    }
    else
    {
    }
}

static const unsigned PREDICATE_STATE[3] =
{
    (unsigned)PREDICATE_STATE_NORMAL,   // plus and undef are normal
    (unsigned)PREDICATE_STATE_INVERT << 4,    // minus is invert
    (unsigned)PREDICATE_STATE_NORMAL    // plus and undef are normal
};

inline void BinaryEncoding::EncodeFlagRegPredicate(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    G4_Predicate *pred = inst->getPredicate();
    uint32_t flagState = 0, flagSwizzle;
    if (pred)
    {
        unsigned pState = (unsigned)pred->getState();
        MUST_BE_TRUE(pState <= (unsigned)PredState_undef,
                     "BinaryEncoding: invalid prediate state");
        flagState = PREDICATE_STATE[pState];

        flagSwizzle = inst->isAligned16Inst() ?
            PREDICATE_ALIGN16_SEQUENTIAL : PREDICATE_ALIGN1_SEQUENTIAL;

        if (inst->isAligned16Inst())
        {
            flagSwizzle = pred->getAlign16PredicateControl();
        }
        else
        {
            auto predCtrl = pred->getControl();
            if (predCtrl != PRED_DEFAULT)
                flagSwizzle = (uint32_t) GetAlign1PredCtrl(predCtrl);
        }

        mybin->SetBits(bitsPredicate_0, bitsPredicate_1, flagState | flagSwizzle);
    }
}

inline void SetFlagSubRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcFlagSubRegNum[0], bits3SrcFlagSubRegNum[1], value);
    else
        mybin->SetBits(bitsFlagSubRegNum[0], bitsFlagSubRegNum[1], value);
}


inline void BinaryEncoding::EncodeFlagReg(G4_INST* inst)
{
    bool flagRegNumValid = false;
    unsigned FlagRegNumValue = 0;
    unsigned FlagRegSubNumValue = 0;
    BinInst *mybin = inst->getBinInst();

    G4_Predicate *pred = inst->getPredicate();
    if (pred) {
        FlagRegNumValue = pred->getBase()->ExRegNum(flagRegNumValid);
        FlagRegSubNumValue = pred->getBase()->asRegVar()->getPhyRegOff();
    }

    G4_CondMod *cModifier = inst->getCondMod();
    if (cModifier)    { // cond modifier
        G4_VarBase* flagReg = cModifier->getBase();
        if (flagReg != NULL)
        {
            FlagRegNumValue = flagReg->ExRegNum(flagRegNumValid);
            FlagRegSubNumValue = flagReg->asRegVar()->getPhyRegOff();
        }
        else
        {
            FlagRegNumValue = 0;
            FlagRegSubNumValue = 0;
        }
    }

    if (pred || cModifier) {
        if (flagRegNumValid) {
            switch (FlagRegNumValue)    {
                case 0:
                    // flag reg num is always zero prior to Gen7
                    break;
                case 1:
                    SetFlagRegNum(mybin, FlagRegNumValue);
                    break;
                default:
                    MUST_BE_TRUE(false, "Invalid flag register number");
                    break;
            }
        }
        if (FlagRegSubNumValue != UNDEFINED_SHORT) {
            switch (FlagRegSubNumValue) {
                case 0:
                case 1:
                    SetFlagSubRegNum(mybin, FlagRegSubNumValue);
                    break;
                default:
                    MUST_BE_TRUE(false, "invalid flag register sub-number");
            }
        }
    }
}

static const unsigned CONDITION_MODIFIER[11] =
{
    (unsigned)COND_CODE_Z,
    (unsigned)COND_CODE_Z,
    (unsigned)COND_CODE_NZ,
    (unsigned)COND_CODE_NZ,
    (unsigned)COND_CODE_G,
    (unsigned)COND_CODE_GE,
    (unsigned)COND_CODE_L,
    (unsigned)COND_CODE_LE,
    (unsigned)COND_CODE_ANY,
    (unsigned)COND_CODE_O,
    (unsigned)COND_CODE_ALL
};

inline void BinaryEncoding::EncodeCondModifier(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    G4_CondMod *cModifier = inst->getCondMod();
    uint32_t value;
    if (cModifier)    {
        value = (unsigned)cModifier->getMod();
        MUST_BE_TRUE(value < Mod_cond_undef,
                     "[Verifying]:[ERR]: Invalid conditional modifier:\t");
        mybin->SetBits(bitsCondModifier_0, bitsCondModifier_1, CONDITION_MODIFIER[value]);
    }
}

inline void BinaryEncoding::EncodeQtrControl(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();

    unsigned int emaskOffset = inst->getMaskOffset();
    mybin->SetBits(bitsQtrCtrl_0, bitsQtrCtrl_1, emaskOffset / 8);
    mybin->SetBits(bitsNibCtrl[0], bitsNibCtrl[1], (emaskOffset % 8 != 0) ? 1 : 0);
}

inline void BinaryEncoding::EncodeInstModifier(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    if (inst->getSaturate()) {
        mybin->SetBits(bitsInstModifier_0, bitsInstModifier_1, INST_MOD_SAT);
    }
    else {
        mybin->SetBits(bitsInstModifier_0, bitsInstModifier_1, INST_MOD_NONE);
    }
}

inline void BinaryEncoding::EncodeMathControl(G4_INST* inst)
{
    MUST_BE_TRUE(inst->isMath(), "BinaryEncoding::EncodeMathControl called on non-math instruction.");
    BinInst *mybin = inst->getBinInst();

    unsigned int MathControlValue = inst->asMathInst()->getMathCtrl();
    unsigned MathFunction = MathControlValue & 0xf;
    unsigned MathPartPrec = (MathControlValue >> 4) & 1;

    if (!mybin->GetIs3Src())     {
        mybin->SetBits(bitsMathFunction_0, bitsMathFunction_1, MathFunction);
        mybin->SetBits(bitsMathPartPrec_0, bitsMathPartPrec_1, MathPartPrec);
    }

 }

inline void EncodeAccWrCtrlInst(G4_INST *inst)
{
    BinInst *mybin = inst->getBinInst();
    //Jmpi does not support BranchControl
    if (inst->isAccWrCtrlInst() || (inst->isFlowControl() && inst->asCFInst()->isBackward() && inst->opcode() != G4_jmpi))
    {
        mybin->SetBits(bitsAccWrCtrl_0, bitsAccWrCtrl_1, 1);
        return;
    }
}

// set bit 29/30 (FP16 input/return) if message descriptor is indirect
// This is to work around a HW bug where only bit[0:28] of a0 is copied
inline void BinaryEncoding::EncodeSendMsgDesc29_30(G4_INST* inst)
{
    MUST_BE_TRUE(inst->isSend(), "must be a send inst");

    BinInst *mybin = inst->getBinInst();
    G4_SendDescRaw* msgDesc = inst->getMsgDescRaw();
    MUST_BE_TRUE(msgDesc, "expected raw descriptor");
    G4_Operand* descOpnd = inst->isSplitSend() ? inst->getSrc(2) : inst->getSrc(1);
    if (!descOpnd->isImm())
    {
        uint32_t bitValue = (msgDesc->is16BitReturn() << 1) + msgDesc->is16BitInput();
        mybin->SetBits(bitsSendDesc_30, bitsSendDesc_29, bitValue);
    }
}

inline void BinaryEncoding::EncodeInstOptionsString(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();

    EncodeAccessMode(inst);
    EncodeQtrControl(inst);
    EncodeAccWrCtrlInst(inst);

    mybin->SetBits(bitsCompactCtrl_0, bitsCompactCtrl_1,
        inst->isCompactedInst());

    if (inst->opcode() == G4_if  ||
         inst->opcode() == G4_else  ||
         inst->opcode() == G4_endif)
    {

    }
    else
    {
        mybin->SetBits(bitsThreadCtrl_0, bitsThreadCtrl_1,
            inst->isAtomicInst()? THREAD_CTRL_ATOMIC :
            // CHAI: Add Switch InstOpt support
            (inst->isYieldInst()? THREAD_CTRL_SWITCH : THREAD_CTRL_NORMAL));
    }


    if (inst->isNoDDChkInst())
    {
        if (inst->isNoDDClrInst())
        {
            mybin->SetBits(bitsDepCtrl[0], bitsDepCtrl[1],
                DEP_CTRL_DIS_CHECK_CLEAR_DEST);
        }   else   {
            mybin->SetBits(bitsDepCtrl[0], bitsDepCtrl[1],
                DEP_CTRL_DIS_CHECK);
        }
    }
    else    {
        if (inst->isNoDDClrInst())
        {
            mybin->SetBits(bitsDepCtrl[0], bitsDepCtrl[1],
                DEP_CTRL_DIS_CLEAR);
    }   else        {
            mybin->SetBits(bitsDepCtrl[0], bitsDepCtrl[1],
                DEP_CTRL_NORMAL);
        }
    }

    if (inst->isWriteEnableInst())
        mybin->SetBits(bitsWECtrl[0], bitsWECtrl[1], 1);

    if (inst->opcode()==G4_jmpi)
        mybin->SetBits(bitsWECtrl[0], bitsWECtrl[1], 1);

    if (inst->isBreakPointInst())
        mybin->SetBits(bitsDebugCtrl_0, bitsDebugCtrl_1, 1);

    if (inst->isNoSrcDepSet())
    {
        mybin->SetBits(bitsNoSrcDepSet_0, bitsNoSrcDepSet_1, 1);
    }

    if (!mybin->GetIs3Src())
    {
        if (inst->isEOT())
            mybin->SetBits(bitsEndOfThread_0, bitsEndOfThread_1, 1);
    }

}

///////////////////////////////////////////////////////////////////////////////
//                       Generic Src Utilities
///////////////////////////////////////////////////////////////////////////////

inline uint32_t GetOperandSrcType(G4_Operand *src)
{
    uint32_t type;
    G4_Type regType;

    if (src->isSrcRegRegion())
    {
        regType = src->asSrcRegRegion()->getType();
    }
    else
    {
        regType = src->getType();
    }

    switch (regType) {
        case Type_UD:
        case Type_D:
        case Type_UW:
        case Type_W:
        case Type_UB:
        case Type_B:
            type = regType;
            break;
        case Type_DF:
            type = SRC_TYPE_DF;
            break;
        case Type_F:
            type = SRC_TYPE_F;
            break;
        case Type_UQ:
            type = SRC_TYPE_UQ;
            break;
        case Type_Q:
            type = SRC_TYPE_Q;
            break;
        case Type_HF:
            type = SRC_TYPE_HF;
            break;
        default:
            type = SRC_TYPE_UNDEF;
            MUST_BE_TRUE(false, "Binary code emission error: unknown type");
    }
    return type;
}

inline uint32_t GetOperandSrcImmType(G4_Operand *src)
{
    uint32_t type;
    switch (src->getType()) {
        case Type_UD:
        case Type_D:
        case Type_UW:
        case Type_W:
            type = src->getType();
            break;
        case Type_UV:
            type = SRC_IMM_TYPE_UV;
            break;
        case Type_VF:
            type = SRC_IMM_TYPE_VF;
            break;
        case Type_V:
            type = SRC_IMM_TYPE_V;
            break;
        case Type_F:
            type = SRC_IMM_TYPE_F;
            break;
        case Type_UQ:
            type = SRC_IMM_TYPE_UQ;
            break;
        case Type_Q:
            type = SRC_IMM_TYPE_Q;
            break;
        case Type_DF:
            type = SRC_IMM_TYPE_DF;
            break;
        case Type_HF:
            type = SRC_IMM_TYPE_HF;
            break;
        default:
            type = SRC_TYPE_UNDEF;
            break;
    }
    return type;
}

static const unsigned SOURCE_MODIFIER[5] =
{
    (unsigned)SRC_MOD_NEGATE,
    (unsigned)SRC_MOD_ABSOLUTE,
    (unsigned)SRC_MOD_NEGATE_OF_ABSOLUTE,
    (unsigned)SRC_MOD_NEGATE,
    (unsigned)SRC_MOD_NONE
};

inline uint32_t GetSrcMod(G4_SrcRegRegion *srcRegion)
{
    unsigned mod = (unsigned)srcRegion->getModifier();
    MUST_BE_TRUE(mod <= (unsigned)Mod_src_undef,
                 "BinaryEncoding: Unexpected source modifier");
    return SOURCE_MODIFIER[mod];
}


///////////////////////////////////////////////////////////////////////////////
//                       Dst
///////////////////////////////////////////////////////////////////////////////



inline void SetDstRegFile(BinInst *mybin, uint32_t value)
{
    if (!mybin->GetIs3Src())
    {
        mybin->SetBits(bitsDstRegFile[0], bitsDstRegFile[1], value);
    }
}

inline void SetDstType(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        // handled by Set3SrcDstType
        return;
    else
        mybin->SetBits(bitsDstType[0], bitsDstType[1], value);
}

inline void SetDstAddrMode(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstAddrMode_0, bitsDstAddrMode_1, value);
}

inline void SetDstRegNumOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcDstRegNumOWord_0,
        bits3SrcDstRegNumOWord_1, value);
    else
        mybin->SetBits(bitsDstRegNumOWord_0,
        bitsDstRegNumOWord_1, value);
}

inline void SetDstRegNumByte(BinInst *mybin,  uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcDstRegNumDWord_0, bits3SrcDstRegNumDWord_1, (value >> 2));
    else
        mybin->SetBits(bitsDstRegNumByte_0, bitsDstRegNumByte_1, value);
}

inline void SetDstChanEn(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcDstChanEn_0,
         bits3SrcDstChanEn_1, value);
    else
        mybin->SetBits(bitsDstChanEn_0,
         bitsDstChanEn_1, value);
}

/**
 *  Below three functions set dst's indirect register number
 */
inline void SetDstIdxRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstIdxRegNum[0], bitsDstIdxRegNum[1], value);
}

inline void SetDstIdxImmOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;

    mybin->SetBits(bitsDstIdxImmOWord[0], bitsDstIdxImmOWord[1], value);
    mybin->SetBits(bitsDstIdxImmMSB[0], bitsDstIdxImmMSB[1], (value >> 5));
}

inline void SetDstIdxImmByte(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;

    mybin->SetBits(bitsDstIdxImmByte[0], bitsDstIdxImmByte[1], value);
    mybin->SetBits(bitsDstIdxImmMSB[0], bitsDstIdxImmMSB[1], (value >> 9));
}

inline void SetDstArchRegFile(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstArchRegFile_0, bitsDstArchRegFile_1, value);
}

inline void SetDstArchRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstArchRegNum_0, bitsDstArchRegNum_1, value);
}

inline void SetDstArchSubRegNumOWord(BinInst *mybin,  uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstArchSubRegNumOWord_0, bitsDstArchSubRegNumOWord_1, value);
}

inline void SetDstArchSubRegNumWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstArchSubRegNumWord_0, bitsDstArchSubRegNumWord_1, value);
}

inline void SetDstArchSubRegNumByte(BinInst *mybin,  uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstArchSubRegNumByte_0, bitsDstArchSubRegNumByte_1, value);
}

inline void SetOperandDstType(BinInst *mybin, G4_DstRegRegion *dst)
{
    G4_Type regType;

    regType = dst->getType();

    switch (regType)
    {
        case Type_UD:
        case Type_D:
        case Type_UW:
        case Type_W:
        case Type_UB:
        case Type_B:
            SetDstType(mybin, regType);
            break;
        case Type_DF:
            SetDstType(mybin, DST_TYPE_DF);
            break;
        case Type_F:
            SetDstType(mybin, DST_TYPE_F);
            break;
       case Type_UQ:
            SetDstType(mybin, DST_TYPE_UQ);
            break;
        case Type_Q:
            SetDstType(mybin, DST_TYPE_Q);
            break;
        case Type_HF:
            SetDstType(mybin, DST_TYPE_HF);
            break;
        default:
            MUST_BE_TRUE(false, "Encoding error: destination type unknown");
            break;
    }
}


inline void EncodeDstRegFile(BinInst *mybin, G4_DstRegRegion *dst)
{
    SetDstRegFile(mybin, EncodingHelper::GetDstRegFile(dst));
    if (EncodingHelper::GetDstRegFile(dst) == REG_FILE_A)
    {
        SetDstArchRegFile(mybin, EncodingHelper::GetDstArchRegType(dst));
    }
}


inline void EncodeDstAddrMode(BinInst *mybin, G4_DstRegRegion *dst)
{
    SetDstAddrMode(mybin, EncodingHelper::GetDstAddrMode(dst));
}

inline void SetDstHorzStride(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsDstHorzStride_0, bitsDstHorzStride_1, value);
}

inline void EncodeDstHorzStride(G4_INST *inst, BinInst *mybin, G4_DstRegRegion *dst)
{
    switch (dst->getHorzStride())
    {
        case 1:
            if (inst->isAligned16Inst())
            {
                SetDstHorzStride(mybin, HORZ_STRIDE_0);
            }
            else
            {
                SetDstHorzStride(mybin, HORZ_STRIDE_1);
            }
            break;
        case 2: SetDstHorzStride(mybin, HORZ_STRIDE_2); break;
        case 4: SetDstHorzStride(mybin, HORZ_STRIDE_4); break;
        case UNDEFINED_SHORT: SetDstHorzStride(mybin, HORZ_STRIDE_1); break;
        default:  MUST_BE_TRUE(false, "wrong dst horizontal stride " << dst->getHorzStride()); break;
    }
}


inline void EncodeDstChanEn(G4_INST* inst, BinInst *mybin, G4_DstRegRegion *dst)
{

    if (dst->isAccRegValid())
    {
        SetDstChanEn(mybin, dst->getAccRegSel());
        return;
    }

    SetDstChanEn(mybin, dst->getWriteMask());

}

inline void BinaryEncoding::EncodeDstRegNum(G4_INST* inst, BinInst *mybin, G4_DstRegRegion *dst)
{
    if (EncodingHelper::GetDstRegFile(dst) != REG_FILE_A &&
         EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_IMMED)
    {
        uint32_t byteAddress = dst->getLinearizedStart();

        MUST_BE_TRUE(byteAddress < kernel.getNumRegTotal() * kernel.numEltPerGRF<Type_UB>(), "dst exceeds total GRF number");

        if (inst->isAligned1Inst())
        {
            SetDstRegNumByte(mybin, byteAddress);
        } else {
            SetDstRegNumOWord(mybin, byteAddress/BYTES_PER_OWORD);
        }
    }
}

inline void EncodeDstArchRegNum(G4_INST* inst, BinInst *mybin, G4_DstRegRegion *dst)
{
    unsigned short RegNumValue = 0, RegSubNumValue = 0;
    bool valid, subValid;
    uint32_t regOffset;

    if (EncodingHelper::GetDstRegFile(dst) == REG_FILE_A  &&
         EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_IMMED)
    {
        if (EncodingHelper::GetDstArchRegType(dst) != ARCH_REG_FILE_NULL)
        {
            RegNumValue = dst->ExRegNum(valid);
            RegSubNumValue = dst->ExSubRegNum(subValid);

            SetDstArchRegNum(mybin, RegNumValue);

            unsigned short ElementSizeValue = EncodingHelper::GetElementSizeValue(dst);
            regOffset = RegSubNumValue * ElementSizeValue;
            if (inst->isAligned1Inst()) {
                SetDstArchSubRegNumByte(mybin, regOffset);
            } else {
                SetDstArchSubRegNumOWord(mybin, regOffset/BYTES_PER_OWORD);
            }
        }
    }
}

inline void EncodeDstIndirectRegNum(G4_INST* inst, BinInst *mybin, G4_DstRegRegion *dst)
{
    bool subValid;
    unsigned short IndAddrRegSubNumValue = 0;
    short IndAddrImmedValue = 0;

    if (EncodingHelper::GetDstRegFile(dst)==REG_FILE_R ||
         EncodingHelper::GetDstRegFile(dst)==REG_FILE_M)
    {
        if (EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_INDIR)
        { // Indirect
            IndAddrRegSubNumValue = dst->ExIndSubRegNum(subValid);
            IndAddrImmedValue = dst->ExIndImmVal();

            SetDstIdxRegNum(mybin, IndAddrRegSubNumValue);
            /* Set the indirect address immediate value. */
            if (inst->isAligned1Inst())
            {
                SetDstIdxImmByte(mybin, IndAddrImmedValue);
            }  else  {
                SetDstIdxImmOWord(mybin, IndAddrImmedValue / BYTES_PER_OWORD);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//                       Src0
///////////////////////////////////////////////////////////////////////////////


inline void SetSrc0RegFile(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcRegFile[0],bitsSrcRegFile[1],value);
}

inline void SetSrc0Type(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        // handled by Set3SrcSrcType
        return;
    else
        mybin->SetBits(bitsSrcType[0],bitsSrcType[1],value);
}

inline void SetSrc0AddrMode(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcAddrMode_0,bitsSrcAddrMode_1,value);
}

inline void SetSrc0SrcMod(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcMod[0], bits3SrcSrcMod[1], value);
    else
        mybin->SetBits(bitsSrcSrcMod_0, bitsSrcSrcMod_1,value);
}

inline void SetSrc0RepCtrl(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcRepCtrl_0, bits3SrcRepCtrl_1, value);
    else
        return;
}

inline void SetSrc0Swizzle(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSwizzle_0, bits3SrcSwizzle_1, value);
    else
        return;
}

inline void SetSrc0RegNumOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcRegNumOWord_0,
            bits3SrcSrcRegNumOWord_1, value);
    else
        mybin->SetBits(bitsSrcRegNumOWord_0,
            bitsSrcRegNumOWord_1, value);
}

inline void SetSrc0RegNumByte(BinInst *mybin, uint32_t value)
{
    mybin->SetBits(bitsSrcRegNumByte_0,
        bitsSrcRegNumByte_1, value);
}

inline void SetSrc0ChanSel_0(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_0_0,
            bits3SrcChanSel_0_1, value);
    else
        mybin->SetBits(bitsSrcChanSel_0_0,
            bitsSrcChanSel_0_1,value);
}


inline void SetSrc0ChanSel_1(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_1_0,
            bits3SrcChanSel_1_1, value);
    else
        mybin->SetBits(bitsSrcChanSel_1_0,
         bitsSrcChanSel_1_1,value);
}

inline void SetSrc0ChanSel_2(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_2_0,
            bits3SrcChanSel_2_1, value);
    else
        mybin->SetBits(bitsSrcChanSel_2_0,
            bitsSrcChanSel_2_1,value);
}

inline void SetSrc0ChanSel_3(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_3_0,
        bits3SrcChanSel_3_1, value);
    else
        mybin->SetBits(bitsSrcChanSel_3_0,
            bitsSrcChanSel_3_1, value);
}

inline void SetSrc0VertStride(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcVertStride_0,bitsSrcVertStride_1,value);
    }

inline void SetSrc0Width(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcWidth_0,bitsSrcWidth_1,value);
}

inline void SetSrc0HorzStride(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcHorzStride_0,bitsSrcHorzStride_1,value);
}

/**
 *  Below three functions set src0's indirect register number
 */
inline void SetSrc0IdxRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcIdxRegNum[0],bitsSrcIdxRegNum[1],value);
}

inline void SetSrc0IdxImmOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    mybin->SetBits(bitsSrcIdxImmOWord[0], bitsSrcIdxImmOWord[1], value);
    mybin->SetBits(bitsSrcIdxImmMSB[0], bitsSrcIdxImmMSB[1], (value >> 5));
}

inline void SetSrc0IdxImmByte(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    mybin->SetBits(bitsSrcIdxImmByte[0], bitsSrcIdxImmByte[1], value);
    mybin->SetBits(bitsSrcIdxImmMSB[0], bitsSrcIdxImmMSB[1], (value >> 9));

}

inline void SetSrc0ArchRegFile(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchRegFile_0,bitsSrcArchRegFile_1,value);
}

inline void SetSrc0ArchRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchRegNum_0,bitsSrcArchRegNum_1,value);
}

inline void SetSrc0ArchSubRegNumOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchSubRegNumOWord_0,bitsSrcArchSubRegNumOWord_1,value);
}

inline void SetSrc0ArchSubRegNumWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchSubRegNumWord_0,bitsSrcArchSubRegNumWord_1,value);
}

inline void SetSrc0ArchSubRegNumByte(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchSubRegNumByte_0,bitsSrcArchSubRegNumByte_1,value);
}

inline void SetSrc0Imm32(BinInst *mybin, uint32_t value, G4_Operand* src)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcImm32_0,bitsSrcImm32_1,value);
}

inline void SetSrc0Imm64(BinInst *mybin, uint64_t value, G4_Operand* src)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    mybin->SetBits(bitsSrcImm64_2, bitsSrcImm64_3, low);
    mybin->SetBits(bitsSrcImm64_0, bitsSrcImm64_1, high);
}

inline void EncodeSrc0RegFile(BinInst *mybin, G4_Operand *src0)
{
    RegFile refFile = EncodingHelper::GetSrcRegFile(src0);
    SetSrc0RegFile(mybin, refFile);
    if (refFile == REG_FILE_A) {
        SetSrc0ArchRegFile(mybin, EncodingHelper::GetSrcArchRegType(src0));
    }
}

inline void EncodeSrc0Type(G4_INST *inst, BinInst *mybin, G4_Operand *src0)
{
    if (src0->isImm())
    {
        SetSrc0Type(mybin, GetOperandSrcImmType(src0));
    } else {
        //no need to force to F anymore.
        //Also need actual type for IGA.
        //So that through binary to binary path I can figure out
        //whether I need to set bits 29/30 in msgDescriptor
        //due to HW Bug on SKL.
        if (inst->getPlatform() >= GENX_CHV)
        {
            SetSrc0Type(mybin, GetOperandSrcType(src0));
        }
        else
        {
            if (inst->isSend())
                SetSrc0Type(mybin, SRC_TYPE_F);
            else
                SetSrc0Type(mybin, GetOperandSrcType(src0));
        }
    }
}

inline void SetSrc1Imm32(BinInst *mybin, uint32_t value, G4_Operand* src)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcImm32_2,bitsSrcImm32_3,value);
}

inline void EncodeSrcImmData(BinInst *mybin, G4_Operand *src)
{
    G4_Imm *isrc = (G4_Imm *)src->asImm();
    if (IS_WTYPE(src->getType()))
    {
        uint32_t val = (uint32_t) isrc->getInt();
        uint32_t data = (val << 16) | (val & 0xffff);
        SetSrc1Imm32(mybin, data, src);
    }
    else if (src->getType() == Type_F)
    {
        // Cannot use getFloat() as it will change sNAN to qNAN (bug1892)
        SetSrc1Imm32(mybin, (uint32_t)isrc->getImm(), src);
    }
    else if (src->getType() == Type_DF)
    {
        SetSrc0Imm64(mybin, (uint64_t)isrc->getImm(), src);
    }
    else if (src->getType() == Type_Q  || src->getType() == Type_UQ)
    {
        int64_t val = isrc->getInt();
        SetSrc0Imm64(mybin, *(uint64_t*)&(val), src);
    }
    else {
        SetSrc1Imm32(mybin, (uint32_t)isrc->getInt(), src);
    }
}

inline void EncodeSrc0AddrMode(BinInst *mybin, G4_Operand *src0)
{
    SetSrc0AddrMode(mybin, EncodingHelper::GetSrcAddrMode(src0));
}

inline void EncodeSrc0ChanSelect(G4_INST *inst,
                                  BinInst *mybin,
                                  G4_Operand *src0,
                                  G4_SrcRegRegion *srcRegion)
{
    // encode acc2~acc9 if it is valid
    if (src0->isAccRegValid())
    {
        if (inst->opcode() == G4_madm ||
            (inst->isMath() && inst->asMathInst()->getMathCtrl() == MATH_INVM) ||
            (inst->isMath() && inst->asMathInst()->getMathCtrl() == MATH_RSQRTM))
        {
            uint32_t value = src0->getAccRegSel();
            SetSrc0ChanSel_0(mybin, value & 0x3);
            SetSrc0ChanSel_1(mybin, value >> 2 & 0x3);
            SetSrc0ChanSel_2(mybin, 0);
            SetSrc0ChanSel_3(mybin, 0);
            return;
        }
        ASSERT_USER(false, "acc2~acc7 were set on wrong instruction");
    }

    const char *swizzle = srcRegion->getSwizzle();
    if (swizzle[0] != '\0' && swizzle[0] != 'r') {
        ChanSel ch0 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 0);
        ChanSel ch1 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 1);
        ChanSel ch2 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 2);
        ChanSel ch3 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 3);
        if (ch0 != CHAN_SEL_UNDEF)
            SetSrc0ChanSel_0(mybin, ch0);
        if (ch1 != CHAN_SEL_UNDEF)
            SetSrc0ChanSel_1(mybin, ch1);
        if (ch2 != CHAN_SEL_UNDEF)
            SetSrc0ChanSel_2(mybin, ch2);
        if (ch3 != CHAN_SEL_UNDEF)
            SetSrc0ChanSel_3(mybin, ch3);
    }

    if (swizzle[0] == '\0')
    {
        if (inst->isAligned16Inst())
        {
            SetSrc0ChanSel_0(mybin, CHAN_SEL_X);
            SetSrc0ChanSel_1(mybin, CHAN_SEL_Y);
            SetSrc0ChanSel_2(mybin, CHAN_SEL_Z);
            SetSrc0ChanSel_3(mybin, CHAN_SEL_W);
        }
    }
    else if (inst->getNumSrc() == 3 && !inst->isSend())
    {
        if (swizzle[0] == 'r')
        {
            if (inst->isAligned16Inst())
            {
                SetSrc0ChanSel_0(mybin, CHAN_SEL_X);
                SetSrc0ChanSel_1(mybin, CHAN_SEL_Y);
                SetSrc0ChanSel_2(mybin, CHAN_SEL_Z);
                SetSrc0ChanSel_3(mybin, CHAN_SEL_W);
            }
        }
    }
}

inline void EncodeSrc0RepCtrl(BinInst *mybin, G4_SrcRegRegion *srcRegion)
{
    const char *swizzle = srcRegion->getSwizzle();
    if (swizzle[0] == 'r')
        SetSrc0RepCtrl(mybin, 0x1);
    else
        SetSrc0RepCtrl(mybin, 0x0);
}

inline void EncodeSrc0Modifier(BinInst *mybin, G4_Operand *src0, G4_SrcRegRegion *srcRegion)
{
    if (EncodingHelper::GetSrcRegFile(src0) != REG_FILE_M)
    {
        SetSrc0SrcMod(mybin, GetSrcMod(srcRegion));
    }
};

static const uint32_t EXEC_CHANNELS[6] =
{
     WIDTH_1,
     WIDTH_2,
     WIDTH_4,
     WIDTH_8,
     WIDTH_8,
     WIDTH_16
};

inline bool EncodeSrc0Width(
    G4_INST *inst, BinInst *mybin, const RegionDesc *rd, G4_Operand *src0)
{
    bool WidthValid = false;
    if (inst->isAligned16Inst()) return false;

    if (rd)
    {
        if (rd->width != UNDEFINED_SHORT)
        {
            WidthValid = true;
        }
        switch (rd->width)
        {
            case 1:  SetSrc0Width(mybin, WIDTH_1); break;
            case 2:  SetSrc0Width(mybin, WIDTH_2); break;
            case 4:  SetSrc0Width(mybin, WIDTH_4); break;
            case 8:  SetSrc0Width(mybin, WIDTH_8); break;
            case 16: SetSrc0Width(mybin, WIDTH_16); break;
            case UNDEFINED_SHORT: break;
            default: MUST_BE_TRUE(false, "wrong width for src0!"); break;
        }

    }

    // apply default width
    if (!WidthValid)
    {
        if (EncodingHelper::isSrcSubRegNumValid(src0))
        {
            SetSrc0Width(mybin, WIDTH_1);
        }
        else
        {
            uint32_t execSize = GetEncodeExecSize(inst);
            MUST_BE_TRUE(execSize <= (uint32_t)ES_32_CHANNELS,
                         "BinaryEncoding: Invalid exeuction channels");
            SetSrc0Width(mybin, EXEC_CHANNELS[execSize]);
        }
    }

    return WidthValid;
 }

static const unsigned HORIZONTAL_STRIDE[6] =
{
    (unsigned)HORZ_STRIDE_0,
    (unsigned)HORZ_STRIDE_1,
    (unsigned)HORZ_STRIDE_1,
    (unsigned)HORZ_STRIDE_1,
    (unsigned)HORZ_STRIDE_1,
    (unsigned)HORZ_STRIDE_1,
};

inline bool EncodeSrc0HorzStride(
    G4_INST *inst, BinInst *mybin, const RegionDesc *rd, G4_Operand *src0)
{
    // For Align16 instruction (SIMD4), treat <HorzStride> as <VertStride>
    // For Align16 source operand disable HorzStride
    bool HorzStrideValid = false;  // undef
    if (inst->isAligned16Inst()) return false;

    if (rd)  {
        if (rd->horzStride != UNDEFINED_SHORT)
        {
            HorzStrideValid = true;
        }
        switch (rd->horzStride)
        {
            case 0: SetSrc0HorzStride(mybin, HORZ_STRIDE_0); break;
            case 1: SetSrc0HorzStride(mybin, HORZ_STRIDE_1); break;
            case 2: SetSrc0HorzStride(mybin, HORZ_STRIDE_2); break;
            case 4: SetSrc0HorzStride(mybin, HORZ_STRIDE_4); break;
            case UNDEFINED_SHORT: break;
            default: MUST_BE_TRUE(false, "wrong horizontal stride for src0!"); break;
        }
    }
    // apply default horizontal stride
    if (!HorzStrideValid)
    {
        if (EncodingHelper::isSrcSubRegNumValid(src0))
             SetSrc0HorzStride(mybin, HORZ_STRIDE_0);
        else
        {
            uint32_t execSize = GetEncodeExecSize(inst);
            if (execSize <= (uint32_t)ES_32_CHANNELS)
                SetSrc0HorzStride(mybin, HORIZONTAL_STRIDE[execSize]);
        }
    } // end of valid horz stride
    return HorzStrideValid;
}

static const unsigned VERTICAL_STRIDE[6] =
{
     (unsigned)VERT_STRIDE_0,
     (unsigned)VERT_STRIDE_2,
     (unsigned)VERT_STRIDE_4,
     (unsigned)VERT_STRIDE_8,
     (unsigned)VERT_STRIDE_8,
     (unsigned)VERT_STRIDE_16
};

inline void EncodeSrc0VertStride(
    G4_INST *inst,
    BinInst *mybin,
    const RegionDesc *rd,
    G4_Operand *src0,
    bool WidthValid,
    bool HorzStrideValid)
{
    bool VertStrideValid = false; // undef
    unsigned short VertStrideValue = UNDEFINED_SHORT, HorzStrideValue = 0;

    if (rd)
    {
        VertStrideValue = rd->vertStride;
        HorzStrideValue = rd->horzStride;
        if (VertStrideValue != UNDEFINED_SHORT)
        {
            VertStrideValid = true;
        }
        switch (VertStrideValue)
        {
            case 0:   SetSrc0VertStride(mybin, VERT_STRIDE_0); break;
            case 1:   SetSrc0VertStride(mybin, VERT_STRIDE_1); break;
            case 2:   SetSrc0VertStride(mybin, VERT_STRIDE_2); break;
            case 4:   SetSrc0VertStride(mybin, VERT_STRIDE_4); break;
            case 8:   SetSrc0VertStride(mybin, VERT_STRIDE_8); break;
            case 16:  SetSrc0VertStride(mybin, VERT_STRIDE_16); break;
            case 32:  SetSrc0VertStride(mybin, VERT_STRIDE_32); break;
            case UNDEFINED_SHORT: break;
            default: MUST_BE_TRUE(false, "wrong vertical stride for src0!"); break;
        }
    }

    //apply default vertical stride below
    if (!WidthValid             &&
        !HorzStrideValid        &&
        !VertStrideValid        &&
        src0)
    {
        VertStrideValid = true;
        if (EncodingHelper::isSrcSubRegNumValid(src0))
        {
            SetSrc0VertStride(mybin, VERT_STRIDE_0);
        }
        else
        {
            if (inst->isAligned1Inst())
            {
                uint32_t value = GetEncodeExecSize(inst);
                MUST_BE_TRUE(value <= (uint32_t)ES_32_CHANNELS,
                             "BinaryEncoding: Invalid execution size");
                SetSrc0VertStride(mybin, VERTICAL_STRIDE[value]);
            }
            else
            {
                SetSrc0VertStride(mybin, VERT_STRIDE_4);
            }
        }
    }

    if (VertStrideValid) { }
    else if (inst->isAligned16Inst())
    {
        if (HorzStrideValid  && HorzStrideValue == 0)
        {
            SetSrc0HorzStride(mybin, HORZ_STRIDE_1);
            SetSrc0VertStride(mybin, VERT_STRIDE_0);
        }
        else if (HorzStrideValid  && HorzStrideValue == 4)
        {
            SetSrc0HorzStride(mybin, HORZ_STRIDE_1);
            SetSrc0VertStride(mybin, VERT_STRIDE_4);
        }
    }
    else if (src0 != NULL && EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_INDIR) { //indirect
        SetSrc0VertStride(mybin, VERT_STRIDE_ONE_DIMEN);
    }

}

inline void EncodeSrc0ArchRegNum(G4_INST* inst, BinInst *mybin, G4_SrcRegRegion *src0)
{
    unsigned short RegNumValue = 0, RegSubNumValue = 0;
    bool valid, subValid;
    uint32_t regOffset;

    if (EncodingHelper::GetSrcRegFile(src0)==REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src0)==ADDR_MODE_IMMED)
    {
        if (EncodingHelper::GetSrcArchRegType(src0) != ARCH_REG_FILE_NULL)
        {
            RegNumValue = src0->ExRegNum(valid);
            RegSubNumValue = src0->ExSubRegNum(subValid);

            SetSrc0ArchRegNum(mybin, RegNumValue);

            unsigned short ElementSizeValue = EncodingHelper::GetElementSizeValue(src0);
            regOffset = RegSubNumValue * ElementSizeValue;
            if (inst->isAligned1Inst()) {
                SetSrc0ArchSubRegNumByte(mybin, regOffset);
            } else {
                SetSrc0ArchSubRegNumOWord(mybin, regOffset/BYTES_PER_OWORD);
            }
        }
    }

}


inline void BinaryEncoding::EncodeSrc0RegNum(G4_INST* inst, BinInst *mybin, G4_Operand *src0)
{
    if (EncodingHelper::GetSrcRegFile(src0) != REG_FILE_A &&
         EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_IMMED)
    {
        bool repControl = EncodingHelper::GetRepControl(src0);
        uint32_t byteAddress = src0->getLinearizedStart();
        MUST_BE_TRUE(byteAddress < kernel.getNumRegTotal() * kernel.numEltPerGRF<Type_UB>(), "src0 exceeds total GRF number");

        if (mybin->GetIs3Src())
        {
            // encode dwords
            mybin->SetBits(bits3SrcSrc0RegDWord_H, bits3SrcSrc0RegDWord_L, byteAddress >> 2);
            // encode word (for word type)
            mybin->SetBits(bits3SrcSrc0SubRegNumW_H, bits3SrcSrc0SubRegNumW_L, (byteAddress >> 1) & 0x1);

        }
        else if (inst->isAligned1Inst() || repControl)
        {
            SetSrc0RegNumByte(mybin, byteAddress);
        }
        else
        {
            SetSrc0RegNumOWord(mybin, byteAddress/BYTES_PER_OWORD);
        }
    }
}

inline void EncodeSrc0IndirectRegNum(G4_INST* inst, BinInst *mybin, G4_SrcRegRegion *src0)
{
    bool subValid;
    unsigned short IndAddrRegSubNumValue = 0;
    short IndAddrImmedValue = 0;

    if (EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_INDIR)
    {
        if (!(EncodingHelper::GetSrcRegFile(src0)==REG_FILE_A &&
              EncodingHelper::GetSrcArchRegType(src0)==ARCH_REG_FILE_NULL))
        {
            IndAddrRegSubNumValue = src0->ExIndSubRegNum(subValid);
            IndAddrImmedValue = src0->ExIndImmVal();

            SetSrc0IdxRegNum(mybin, IndAddrRegSubNumValue);
            /* Set the indirect address immediate value. */
            if (inst->isAligned1Inst())
            {
                SetSrc0IdxImmByte(mybin, IndAddrImmedValue);
            }  else  {
                SetSrc0IdxImmOWord(mybin, IndAddrImmedValue / BYTES_PER_OWORD);
            }
        }
    }
}

inline void Set3SrcSrcType(BinInst *mybin, G4_INST *inst)
{
    if (inst->getSrc(0) == NULL) return;
    G4_Type type = inst->getSrc(0)->getType();
    ThreeSrcType sType = THREE_SRC_TYPE_F;
    switch (type)
    {
        case Type_F:
            sType = THREE_SRC_TYPE_F;
            break;
        case Type_D:
            sType = THREE_SRC_TYPE_D;
            break;
        case Type_UD:
            sType = THREE_SRC_TYPE_UD;
            break;
        case Type_DF:
            sType = THREE_SRC_TYPE_DF;
            break;
        case Type_HF:
            sType = THREE_SRC_TYPE_HF;
            break;
        default:
            break;
    }
     mybin->SetBits(bits3SrcSrcType[0], bits3SrcSrcType[1], (uint32_t)sType);
     if (inst->getPlatform() >= GENX_CHV)
     {
         if (inst->getSrc(1)->getType() == Type_HF)
         {
             mybin->SetBits(bits3SrcSrc1Type, bits3SrcSrc1Type, 1);
         }
         if (inst->getSrc(2)->getType() == Type_HF)
         {
             mybin->SetBits(bits3SrcSrc2Type, bits3SrcSrc2Type, 1);
         }
     }
}

inline void Set3SrcDstType(BinInst *mybin, G4_Type type)
{
    ThreeSrcType dType = THREE_SRC_TYPE_F;
    switch (type)
    {
        case Type_F:
            dType = THREE_SRC_TYPE_F;
            break;
        case Type_D:
            dType = THREE_SRC_TYPE_D;
            break;
        case Type_UD:
            dType = THREE_SRC_TYPE_UD;
            break;
        case Type_DF:
            dType = THREE_SRC_TYPE_DF;
            break;
        case Type_HF:
            dType = THREE_SRC_TYPE_HF;
            break;
        default:
            break;
    }
     mybin->SetBits(bits3SrcDstType[0], bits3SrcDstType[1], (uint32_t)dType);
}

BinaryEncoding::Status BinaryEncoding::EncodeOperandDst(G4_INST* inst)
{
    Status myStatus = SUCCESS;
    BinInst *mybin = inst->getBinInst();
    G4_DstRegRegion *dst = inst->getDst();

    if (inst->isSplitSend())
    {
        return EncodeSplitSendDst(inst);
    }

    if (mybin->GetIs3Src())
    {
        MUST_BE_TRUE(EncodingHelper::GetDstRegFile(dst) == REG_FILE_R, "Dst for 3src instruction must be GRF");
        Set3SrcDstType(mybin, dst->getType());
        Set3SrcSrcType(mybin, inst);
    }

    if (inst->opcode() == G4_wait) return SUCCESS;
    if (inst->opcode() == G4_jmpi)
    {
        SetDstRegFile(mybin, REG_FILE_A);
        SetDstArchRegFile(mybin, ARCH_REG_FILE_IP);
        //SetDstRegNumByte(mybin, 0);
        SetDstArchSubRegNumByte(mybin, 0);
        SetDstAddrMode(mybin, ADDR_MODE_IMMED);
        SetDstType(mybin, DST_TYPE_UD);
        SetDstHorzStride(mybin, 1);

        if (inst->getSrc(0))
        {
            SetSrc0RegFile(mybin, REG_FILE_A);
            SetSrc0ArchRegFile(mybin, ARCH_REG_FILE_IP);
            SetSrc0Type(mybin, DST_TYPE_UD);
            SetSrc0AddrMode(mybin, ADDR_MODE_IMMED);
            //SetSrc0RegNumByte(mybin, 0);
            SetSrc0ArchSubRegNumByte(mybin, 0);
            if (!(inst->getSrc(0)->isLabel()))
            {
                SetSrc0Width(mybin, WIDTH_1);
                SetDstHorzStride(mybin, 1);
            }
            else
            {
                SetSrc0Width(mybin, WIDTH_1);
                SetSrc0VertStride(mybin, VERT_STRIDE_0);
                SetSrc0HorzStride(mybin, HORZ_STRIDE_0);
                SetSrc0SrcMod(mybin, SRC_MOD_NONE);
            }
        }
    }

    if (dst == NULL)
    {
        SetDstHorzStride(mybin, HORZ_STRIDE_1);
        return FAILURE;
    }

    SetOperandDstType(mybin, dst);
    EncodeDstRegFile(mybin, dst);
    EncodeDstAddrMode(mybin, dst);
    // Note: dst doesn't have the vertical stride and width
    EncodeDstHorzStride(inst, mybin, dst);
    EncodeDstChanEn(inst, mybin, dst);
    EncodeDstRegNum(inst, mybin, dst);
    EncodeDstIndirectRegNum(inst, mybin, dst);
    EncodeDstArchRegNum(inst, mybin, dst);
    return myStatus;
}


inline BinaryEncoding::Status BinaryEncoding::EncodeOperandSrc0(G4_INST* inst)
{
    Status myStatus = SUCCESS;
    if (inst->isLabel()  ||
         inst->isCall())
        return myStatus;

    if (inst->isSplitSend())
    {
        return EncodeSplitSendSrc0(inst);
    }

    BinInst *mybin = inst->getBinInst();
    G4_Operand *src0 = inst->getSrc(0);

    if (src0 == NULL ||src0->isLabel()) return FAILURE;
    if (inst->opcode() == G4_jmpi && src0->isSrcRegRegion())
    {
        // will treat this src0 as src1 in EncodeOperandSrc1
        return SUCCESS;
    }

    EncodeSrc0Type(inst, mybin, src0);
    EncodeSrc0RegFile(mybin, src0);
    if (src0->isImm())
    {
        if (inst->opcode() != G4_mov && src0->getTypeSize() == 8)
        {
            MUST_BE_TRUE(false, "only Mov is allowed for 64bit immediate");
        }
        EncodeSrcImmData(mybin, src0);
    }
    else
    {
        G4_SrcRegRegion* src0Region = src0->asSrcRegRegion();
        EncodeSrc0AddrMode(mybin, src0);
        EncodeSrc0ChanSelect(inst, mybin, src0, src0Region);
        EncodeSrc0RepCtrl(mybin, src0Region);
        EncodeSrc0Modifier(mybin, src0, src0Region);

        const RegionDesc *rd = src0Region->getRegion();
        bool WidthValid = EncodeSrc0Width(inst, mybin, rd, src0);
        bool HorzStrideValid = EncodeSrc0HorzStride(inst, mybin, rd, src0);
        EncodeSrc0VertStride(inst, mybin, rd, src0, WidthValid, HorzStrideValid);

        EncodeSrc0ArchRegNum(inst, mybin, src0Region);
        EncodeSrc0RegNum(inst, mybin, src0);
        EncodeSrc0IndirectRegNum(inst, mybin, src0Region);
    }
    return myStatus;
}

///////////////////////////////////////////////////////////////////////////////
//                       Src1
///////////////////////////////////////////////////////////////////////////////

inline void SetSrc1RegFile(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcRegFile[2],bitsSrcRegFile[3],value);
}

inline void SetSrc1Type(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
    // handled by Set3SrcSrcType
        return;
    else
     mybin->SetBits(bitsSrcType[2],bitsSrcType[3],value);
}

inline void SetSrc1AddrMode(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcAddrMode_2,bitsSrcAddrMode_3,value);
}
inline void SetSrc1SrcMod(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcMod[2],
            bits3SrcSrcMod[3],value);
    else
        mybin->SetBits(bitsSrcSrcMod_2,
            bitsSrcSrcMod_3,value);
}

inline void SetSrc1RepCtrl(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcRepCtrl_2, bits3SrcRepCtrl_3, value);
    else
        return;
}

inline void SetSrc1Swizzle(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSwizzle_2, bits3SrcSwizzle_3, value);
    else
        return;
}

inline void SetSrc1RegNumOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcRegNumOWord_2,
            bits3SrcSrcRegNumOWord_3, value);
    else
        mybin->SetBits(bitsSrcRegNumOWord_2,
            bitsSrcRegNumOWord_3, value);
}

inline void SetSrc1RegNumByte(BinInst *mybin, uint32_t value)
{
    mybin->SetBits(bitsSrcRegNumByte_2,bitsSrcRegNumByte_3,value);
}

inline void SetSrc1ChanSel_0(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_0_2, bits3SrcChanSel_0_3, value);
    else
        mybin->SetBits(bitsSrcChanSel_0_2,bitsSrcChanSel_0_3,value);
}

void SetSrc1ChanSel_1(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_1_2, bits3SrcChanSel_1_3, value);
    else
        mybin->SetBits(bitsSrcChanSel_1_2,bitsSrcChanSel_1_3,value);
}

inline void SetSrc1ChanSel_2(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_2_2, bits3SrcChanSel_2_3, value);
    else
        mybin->SetBits(bitsSrcChanSel_2_2,bitsSrcChanSel_2_3,value);
}

inline void SetSrc1ChanSel_3(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_3_2, bits3SrcChanSel_3_3, value);
    else
        mybin->SetBits(bitsSrcChanSel_3_2,bitsSrcChanSel_3_3,value);
}

inline void SetSrc1VertStride(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcVertStride_2,bitsSrcVertStride_3,value);
}

inline void SetSrc1Width(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcWidth_2,bitsSrcWidth_3,value);
}

inline void SetSrc1HorzStride(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcHorzStride_2,bitsSrcHorzStride_3,value);
}

/**
 *  Below three functions set src1's indirect register number
 */
inline void SetSrc1IdxRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcIdxRegNum[2],bitsSrcIdxRegNum[3],value);
}

inline void SetSrc1IdxImmOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    mybin->SetBits(bitsSrcIdxImmOWord[2], bitsSrcIdxImmOWord[3], value);
    mybin->SetBits(bitsSrcIdxImmMSB[2], bitsSrcIdxImmMSB[3], (value >> 5));
}

inline void SetSrc1IdxImmByte(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    mybin->SetBits(bitsSrcIdxImmByte[2], bitsSrcIdxImmByte[3], value);
    mybin->SetBits(bitsSrcIdxImmMSB[2], bitsSrcIdxImmMSB[3], (value >> 9));
}

inline void SetSrc1ArchRegFile(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchRegFile_2,bitsSrcArchRegFile_3,value);
}

inline void SetSrc1ArchRegNum(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchRegNum_2,bitsSrcArchRegNum_3,value);
}

inline void SetSrc1ArchSubRegNumOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchSubRegNumOWord_2,bitsSrcArchSubRegNumOWord_3,value);
}

inline void SetSrc1ArchSubRegNumWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchSubRegNumWord_2,bitsSrcArchSubRegNumWord_3,value);
}

inline void SetSrc1ArchSubRegNumByte(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        return;
    else
        mybin->SetBits(bitsSrcArchSubRegNumByte_2,bitsSrcArchSubRegNumByte_3,value);
}

inline void EncodeSrc1RegFile(BinInst *mybin, G4_Operand *src1)
{
    RegFile regFile = EncodingHelper::GetSrcRegFile(src1);
    SetSrc1RegFile(mybin, regFile);
    if (regFile == REG_FILE_A)
    {
        SetSrc1ArchRegFile(mybin, EncodingHelper::GetSrcArchRegType(src1));
    }
}

inline void EncodeSrc1Type(BinInst *mybin, G4_Operand *src1)
{
    if (src1->isImm())
    {
        SetSrc1Type(mybin, GetOperandSrcImmType(src1));
    } else {
        SetSrc1Type(mybin, GetOperandSrcType(src1));
    }
}

inline void EncodeSrc1AddrMode(BinInst *mybin, G4_Operand *src1)
{
    SetSrc1AddrMode(mybin, EncodingHelper::GetSrcAddrMode(src1));
}

inline void EncodeSrc1ChanSelect(G4_INST *inst, BinInst *mybin, G4_SrcRegRegion *srcRegion)
{
    // encode acc2~acc9 if it is valid
    if (srcRegion->isAccRegValid())
    {
        if (inst->opcode() == G4_madm ||
            (inst->isMath() && inst->asMathInst()->getMathCtrl() == MATH_INVM) ||
            (inst->isMath() && inst->asMathInst()->getMathCtrl() == MATH_RSQRTM))
        {
            uint32_t value = srcRegion->getAccRegSel();
            SetSrc1ChanSel_0(mybin, value & 0x3);
            SetSrc1ChanSel_1(mybin, value >> 2 & 0x3);
            SetSrc1ChanSel_2(mybin, 0);
            SetSrc1ChanSel_3(mybin, 0);
            return;
        }
        ASSERT_USER(false, "acc2~acc7 were set on wrong instruction");
    }

    const char *swizzle = srcRegion->getSwizzle();
    if (swizzle[0] != '\0' && swizzle[0] != 'r') {
        ChanSel ch0 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 0);
        ChanSel ch1 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 1);
        ChanSel ch2 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 2);
        ChanSel ch3 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 3);
        if (ch0 != CHAN_SEL_UNDEF)
            SetSrc1ChanSel_0(mybin, ch0);
        if (ch1 != CHAN_SEL_UNDEF)
            SetSrc1ChanSel_1(mybin, ch1);
        if (ch2 != CHAN_SEL_UNDEF)
            SetSrc1ChanSel_2(mybin, ch2);
        if (ch3 != CHAN_SEL_UNDEF)
            SetSrc1ChanSel_3(mybin, ch3);
    }

    if (swizzle[0] == '\0')
    {
        if (inst->isAligned16Inst())
        {
            SetSrc1ChanSel_0(mybin, CHAN_SEL_X);
            SetSrc1ChanSel_1(mybin, CHAN_SEL_Y);
            SetSrc1ChanSel_2(mybin, CHAN_SEL_Z);
            SetSrc1ChanSel_3(mybin, CHAN_SEL_W);
        }
    }
    else if (inst->getNumSrc() == 3 && !inst->isSend())
    {
        if (swizzle[0] == 'r')
        {
            if (inst->isAligned16Inst())
            {
                SetSrc1ChanSel_0(mybin, CHAN_SEL_X);
                SetSrc1ChanSel_1(mybin, CHAN_SEL_Y);
                SetSrc1ChanSel_2(mybin, CHAN_SEL_Z);
                SetSrc1ChanSel_3(mybin, CHAN_SEL_W);
            }
        }
    }
}

inline void EncodeSrc1RepCtrl(BinInst *mybin, G4_SrcRegRegion *srcRegion)
{
    const char *swizzle = srcRegion->getSwizzle();
    if (swizzle[0] == 'r')
        SetSrc1RepCtrl(mybin, 0x1);
    else
        SetSrc1RepCtrl(mybin, 0x0);
}

inline void EncodeSrc1Modifier(BinInst *mybin, G4_SrcRegRegion *srcRegion)
{
    SetSrc1SrcMod(mybin, GetSrcMod(srcRegion));
}

inline bool EncodeSrc1Width(
    G4_INST *inst, BinInst *mybin, const RegionDesc *rd, G4_Operand *src1)
{
    bool WidthValid = false;
    if (inst->isAligned16Inst()) return false;

    if (rd)
    {
        if (rd->width != UNDEFINED_SHORT)
        {
            WidthValid = true;
        }
        switch (rd->width)
        {
            case 1:  SetSrc1Width(mybin, WIDTH_1); break;
            case 2:  SetSrc1Width(mybin, WIDTH_2); break;
            case 4:  SetSrc1Width(mybin, WIDTH_4); break;
            case 8:  SetSrc1Width(mybin, WIDTH_8); break;
            case 16: SetSrc1Width(mybin, WIDTH_16); break;
            case UNDEFINED_SHORT:break;
            default: MUST_BE_TRUE(false, "wrong width for src1!"); break;
        }
    }

    // apply default width
    if (!WidthValid)
    {
        if (EncodingHelper::isSrcSubRegNumValid(src1))
        {
            SetSrc1Width(mybin, WIDTH_1);
        }
        else
        {
            uint32_t width = GetEncodeExecSize(inst);
            if (width <= (uint32_t)ES_32_CHANNELS)
            {
                 SetSrc1Width(mybin, EXEC_CHANNELS[width]);
            }
        }
    }

    return WidthValid;
 }

inline bool EncodeSrc1HorzStride(
    G4_INST *inst, BinInst *mybin, const RegionDesc *rd, G4_Operand *src1)
{
    // For Align16 instruction (SIMD4), treat <HorzStride> as <VertStride>
    // For Align16 source operand disable HorzStride
    bool HorzStrideValid = false;
    if (inst->isAligned16Inst()) return false;

    if (rd)
    {
        if (rd->horzStride != UNDEFINED_SHORT)
        {
            HorzStrideValid = true;
        }
        switch (rd->horzStride)
        {
            case 0: SetSrc1HorzStride(mybin, HORZ_STRIDE_0); break;
            case 1: SetSrc1HorzStride(mybin, HORZ_STRIDE_1); break;
            case 2: SetSrc1HorzStride(mybin, HORZ_STRIDE_2); break;
            case 4: SetSrc1HorzStride(mybin, HORZ_STRIDE_4); break;
            case UNDEFINED_SHORT: break;
            default: MUST_BE_TRUE(false, "wrong horizontal stride for src1!"); break;
        }

    }
    // apply default horizontal stride
    if (!HorzStrideValid)
    {
        if (EncodingHelper::isSrcSubRegNumValid(src1))
            SetSrc1HorzStride(mybin, HORZ_STRIDE_0);
        else
        {
            uint32_t exeSize = GetEncodeExecSize(inst);
            MUST_BE_TRUE(exeSize <= (uint32_t)ES_32_CHANNELS,
                         "Binary Encoding: Invalid execution size");
            {
                SetSrc1HorzStride(mybin, HORIZONTAL_STRIDE[exeSize]);
            }
        }
    }

    return HorzStrideValid;
}

inline void EncodeSrc1VertStride(G4_INST *inst,
                                 BinInst *mybin,
                                 const RegionDesc *rd,
                                 G4_Operand *src1,
                                 bool WidthValid,
                                 bool HorzStrideValid)
{
    bool VertStrideValid = false;
    unsigned short VertStrideValue= UNDEFINED_SHORT;
    unsigned short HorzStrideValue= UNDEFINED_SHORT;

    if (rd)  {
        VertStrideValue = rd->vertStride;
        HorzStrideValue = rd->horzStride;
        if (VertStrideValue != UNDEFINED_SHORT)
        {
            VertStrideValid = true;
        }
        switch (VertStrideValue)
        {
            case 0:   SetSrc1VertStride(mybin, VERT_STRIDE_0); break;
            case 1:   SetSrc1VertStride(mybin, VERT_STRIDE_1); break;
            case 2:   SetSrc1VertStride(mybin, VERT_STRIDE_2); break;
            case 4:   SetSrc1VertStride(mybin, VERT_STRIDE_4); break;
            case 8:   SetSrc1VertStride(mybin, VERT_STRIDE_8); break;
            case 16:  SetSrc1VertStride(mybin, VERT_STRIDE_16); break;
            case 32:  SetSrc1VertStride(mybin, VERT_STRIDE_32); break;
            case UNDEFINED_SHORT: break;
            default: MUST_BE_TRUE(false, "wrong verical stride for src1!"); break;
        }

    }

        //apply default vertical stride below
    if (!WidthValid             &&
         !HorzStrideValid        &&
         !VertStrideValid        &&
         src1)
    {
        VertStrideValid = true;
        if (EncodingHelper::isSrcSubRegNumValid(src1))
        {
            SetSrc1VertStride(mybin, VERT_STRIDE_0);
        }
        else
        {
            if (inst->isAligned1Inst())
            {
                uint32_t execSize = GetEncodeExecSize(inst);
                MUST_BE_TRUE(execSize <= (uint32_t)ES_32_CHANNELS,
                             "BinaryEncoding: Invalid execution size");
                SetSrc1VertStride(mybin, VERTICAL_STRIDE[execSize]);
            }
            else
            {
                SetSrc1VertStride(mybin, VERT_STRIDE_4);
            }
        } // fi
    } // fi

    if (VertStrideValid) {}
    else if (inst->isAligned16Inst())
    {
        if (HorzStrideValid == 1   &&
             HorzStrideValue == 4)
        {
            SetSrc1HorzStride(mybin, HORZ_STRIDE_1);
            SetSrc1VertStride(mybin, VERT_STRIDE_4);
        }
    }
    else if (src1 != NULL && EncodingHelper::GetSrcAddrMode(src1) == ADDR_MODE_INDIR)
    { //indirect
        SetSrc1VertStride(mybin, VERT_STRIDE_ONE_DIMEN);
    }
}

inline void BinaryEncoding::EncodeSrc1RegNum(G4_INST *inst, BinInst *mybin, G4_Operand *src1)
{
    if (EncodingHelper::GetSrcRegFile(src1) != REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src1)==ADDR_MODE_IMMED)
    {
        bool repControl = EncodingHelper::GetRepControl(src1);
        uint32_t byteAddress = src1->getLinearizedStart();
        MUST_BE_TRUE(byteAddress < kernel.getNumRegTotal() * kernel.numEltPerGRF<Type_UB>(), "src1 exceeds total GRF number");

        if (mybin->GetIs3Src())
        {
            // src1 subregnum crosses dword boundary, which SetBits can't handle, so we have to break it into two (sigh)
            mybin->SetBits(bits3SrcSrc1RegDWord1_H, bits3SrcSrc1RegDWord1_L, byteAddress >> 4);
            mybin->SetBits(bits3SrcSrc1RegDWord2_H, bits3SrcSrc1RegDWord2_L, (byteAddress >> 2) & 0x3);
            mybin->SetBits(bits3SrcSrc1SubRegNumW_H, bits3SrcSrc1SubRegNumW_L, (byteAddress >> 1) & 0x1);
        }
        else if (inst->isAligned1Inst() || repControl)
        {
            SetSrc1RegNumByte(mybin, byteAddress);
        }
        else
        {
            SetSrc1RegNumOWord(mybin, byteAddress/BYTES_PER_OWORD);
        }
    }
}

inline void EncodeSrc1ArchRegNum(G4_INST *inst, BinInst *mybin, G4_SrcRegRegion *src1)
{
    unsigned short RegNumValue = 0, RegSubNumValue = 0;
    bool valid, subValid;
    uint32_t regOffset;

    if (EncodingHelper::GetSrcRegFile(src1)==REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src1) == ADDR_MODE_IMMED)
    {
        if (EncodingHelper::GetSrcArchRegType(src1) != ARCH_REG_FILE_NULL)
        {
            RegNumValue = src1->ExRegNum(valid);
            RegSubNumValue = src1->ExSubRegNum(subValid);

            SetSrc1ArchRegNum(mybin, RegNumValue);

            unsigned short ElementSizeValue = EncodingHelper::GetElementSizeValue(src1);
            regOffset = RegSubNumValue * ElementSizeValue;
            if (inst->isAligned1Inst())
            {
                SetSrc1ArchSubRegNumByte(mybin, regOffset);
            } else {
                SetSrc1ArchSubRegNumOWord(mybin, regOffset/BYTES_PER_OWORD);
            }
        }
    }
}

inline void EncodeSrc1IndirectRegNum(G4_INST* inst, BinInst *mybin, G4_SrcRegRegion *src1)
{
    bool subValid;
    unsigned short IndAddrRegSubNumValue = 0;
    short IndAddrImmedValue = 0;

    if (EncodingHelper::GetSrcAddrMode(src1)==ADDR_MODE_INDIR)
    {
        if (! (EncodingHelper::GetSrcRegFile(src1)==REG_FILE_A &&
              EncodingHelper::GetSrcArchRegType(src1)==ARCH_REG_FILE_NULL))
        {
            IndAddrRegSubNumValue = src1->ExIndSubRegNum(subValid);
            IndAddrImmedValue = src1->ExIndImmVal();

            SetSrc1IdxRegNum(mybin, IndAddrRegSubNumValue);
            /* Set the indirect address immediate value. */
            if (inst->isAligned1Inst())  {
                SetSrc1IdxImmByte(mybin, IndAddrImmedValue);
            }  else  {
                SetSrc1IdxImmOWord(mybin, IndAddrImmedValue / BYTES_PER_OWORD);
            }
        }
    }
}

BinaryEncoding::Status BinaryEncoding::EncodeSplitSendDst(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    if (inst->getDst() == NULL)
    {
        return FAILURE;
    }
    G4_DstRegRegion* dst = inst->getDst();

    EncodeDstRegFile(mybin, dst);
    SetOperandDstType(mybin, dst);
    if (EncodingHelper::GetDstAddrMode(dst)==ADDR_MODE_INDIR)
    {
        bool subValid;
        uint16_t IndAddrRegSubNumValue = dst->ExIndSubRegNum(subValid);
        int16_t IndAddrImmedValue = dst->ExIndImmVal();

        mybin->SetBits(bitsSendsDstAddrMode_0, bitsSendsDstAddrMode_1, ADDR_MODE_INDIR);
        mybin->SetBits(bitsSendsDstAddrSubRegNum_0, bitsSendsDstAddrSubRegNum_1, IndAddrRegSubNumValue);
        mybin->SetBits(bitsSendsDstAddrImm8_4_0, bitsSendsDstAddrImm8_4_1, (IndAddrImmedValue >> 4) & 0x1F);
        mybin->SetBits(bitsSendsDstAddrImmSign_0, bitsSendsDstAddrImmSign_1, (IndAddrImmedValue >> 9) & 0x1);
    }
    else
    {
        mybin->SetBits(bitsSendsDstAddrMode_0, bitsSendsDstAddrMode_1, ADDR_MODE_IMMED);
        if (EncodingHelper::GetDstRegFile(dst) == REG_FILE_A)
        {
            // must be NULL, do nothing
        }
        else
        {
            // must be GRF
            uint32_t byteAddress = dst->getLinearizedStart();
            MUST_BE_TRUE(byteAddress % 16 == 0, "dst for sends/sendsc must be oword-aligned");
            mybin->SetBits(bitsSendsDstRegNum_0, bitsSendsDstRegNum_1, byteAddress >> 5);
            mybin->SetBits(bitsSendsDstSubRegNum_0, bitsSendsDstSubRegNum_1, (byteAddress >> 4) & 0x1);
        }
    }

    return SUCCESS;
}

// src2 is the message descriptor (either a0.0 or imm)
BinaryEncoding::Status BinaryEncoding::EncodeSplitSendSrc2(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    if (inst->getSrc(2) == NULL)
    {
        return FAILURE;
    }
    G4_Operand* src2 = inst->getSrc(2);

    if (src2->isImm())
    {
        mybin->SetBits(bitsSendsSelReg32Desc_0, bitsSendsSelReg32Desc_0, 0);
        mybin->SetBits(bitsSrcImm32_2,bitsSrcImm32_3, (uint32_t) src2->asImm()->getInt());
    }
    else if (src2->isSrcRegRegion() && src2->asSrcRegRegion()->isA0())
    {
        mybin->SetBits(bitsSendsSelReg32Desc_0, bitsSendsSelReg32Desc_0, 1);
    }

    return SUCCESS;
}

BinaryEncoding::Status BinaryEncoding::EncodeSplitSendSrc1(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    if (inst->getSrc(1) == NULL || !inst->getSrc(1)->isSrcRegRegion())
    {
        return FAILURE;
    }
    G4_SrcRegRegion* src1 = inst->getSrc(1)->asSrcRegRegion();

    if (src1->isNullReg())
    {
        mybin->SetBits(bitsSendsSrc1RegFile_0, bitsSendsSrc1RegFile_1, REG_FILE_A);
    }
    else
    {
        mybin->SetBits(bitsSendsSrc1RegFile_0, bitsSendsSrc1RegFile_1, REG_FILE_R);
        if (EncodingHelper::GetSrcAddrMode(src1) == ADDR_MODE_INDIR)
        {
            bool subValid;
            uint16_t IndAddrRegSubNumValue = src1->ExIndSubRegNum(subValid);
            int16_t IndAddrImmedValue = src1->ExIndImmVal();

            mybin->SetBits(bitsSendsSrc1AddrMode_0, bitsSendsSrc1AddrMode_1, ADDR_MODE_INDIR);
            mybin->SetBits(bitsSendsSrc1AddrSubRegNum_0, bitsSendsSrc1AddrSubRegNum_1, IndAddrRegSubNumValue);
            mybin->SetBits(bitsSendsSrc1AddrImm8_4_0, bitsSendsSrc1AddrImm8_4_1, (IndAddrImmedValue >> 4) & 0x1F);
            mybin->SetBits(bitsSendsSrc1AddrImmSign_0, bitsSendsSrc1AddrImmSign_1, (IndAddrImmedValue >> 9) & 0x1);
        }
        else
        {
            uint32_t byteAddress = src1->getLinearizedStart();
            MUST_BE_TRUE(byteAddress % 32 == 0, "src1 for sends/sendsc must be GRF-aligned");
            mybin->SetBits(bitsSendsSrc1RegNum_0, bitsSendsSrc1RegNum_1, byteAddress >> 5);
        }
    }

    return SUCCESS;
}

BinaryEncoding::Status BinaryEncoding::EncodeSplitSendSrc0(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    if (inst->getSrc(0) == NULL || !inst->getSrc(0)->isSrcRegRegion())
    {
        return FAILURE;
    }
    G4_SrcRegRegion* src0 = inst->getSrc(0)->asSrcRegRegion();

    if (EncodingHelper::GetSrcAddrMode(src0)==ADDR_MODE_INDIR)
    {
        bool subValid;
        uint16_t IndAddrRegSubNumValue = src0->ExIndSubRegNum(subValid);
        int16_t IndAddrImmedValue = src0->ExIndImmVal();

        mybin->SetBits(bitsSendsSrc0AddrMode_0, bitsSendsSrc0AddrMode_1, ADDR_MODE_INDIR);
        mybin->SetBits(bitsSendsSrc0AddrSubRegNum_0, bitsSendsSrc0AddrSubRegNum_1, IndAddrRegSubNumValue);
        mybin->SetBits(bitsSendsSrc0AddrImm8_4_0, bitsSendsSrc0AddrImm8_4_1, (IndAddrImmedValue >> 4) & 0x1F);
        mybin->SetBits(bitsSendsSrc0AddrImmSign_0, bitsSendsSrc0AddrImmSign_1, (IndAddrImmedValue >> 9) & 0x1);
    }
    else
    {
        mybin->SetBits(bitsSendsSrc0AddrMode_0, bitsSendsSrc0AddrMode_1, ADDR_MODE_IMMED);
        uint32_t byteAddress = src0->getLinearizedStart();
        MUST_BE_TRUE(byteAddress % 32 == 0, "src1 for sends/sendsc must be GRF-aligned");
        mybin->SetBits(bitsSendsSrc0RegNum_0, bitsSendsSrc0RegNum_1, byteAddress >> 5);
    }

    return SUCCESS;
}

// Gen encodes target for call as src1, though internally we store it in src0
BinaryEncoding::Status BinaryEncoding::EncodeIndirectCallTarget(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    G4_Operand* funcAddr = inst->getSrc(0);
    EncodeSrc1RegFile(mybin, funcAddr);
    EncodeSrc1Type(mybin, funcAddr);
    if (funcAddr->isImm())
    {
        EncodeSrcImmData(mybin, funcAddr);
    }
    else
    {
        G4_SrcRegRegion *srcRegion = funcAddr->asSrcRegRegion();
        EncodeSrc1RegNum(inst, mybin, srcRegion);
        EncodeSrc1ArchRegNum(inst, mybin, srcRegion);
        EncodeSrc1IndirectRegNum(inst, mybin, srcRegion);
        EncodeSrc1AddrMode(mybin, srcRegion);
        EncodeSrc1RepCtrl(mybin, srcRegion);
        EncodeSrc1Modifier(mybin, srcRegion);
        EncodeSrc1ChanSelect(inst, mybin, srcRegion);
        const RegionDesc *rd = srcRegion->getRegion();
        bool WidthValid = EncodeSrc1Width(inst, mybin, rd, srcRegion);
        bool HorzStrideValid = EncodeSrc1HorzStride(inst, mybin, rd, srcRegion);
        EncodeSrc1VertStride(inst, mybin, rd, srcRegion, WidthValid, HorzStrideValid);
    }

    return SUCCESS;
}


BinaryEncoding::Status BinaryEncoding::EncodeOperandSrc1(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    G4_Operand *src1 = NULL;

    if (inst->isSplitSend())
    {
        return EncodeSplitSendSrc1(inst);
    }

    G4_Operand *src0 = inst->getSrc(0);
    bool isSrc0Region = false;
    if (src0)
        isSrc0Region = src0->isSrcRegRegion();

    if (inst->opcode() == G4_jmpi &&
         isSrc0Region)
    {
        src1 = src0;
    }
    else
    {
        src1 = inst->getSrc(1);
    }

    if (inst->isLabel())  {
        if (isSrc0Region)
        {
            //Gen4_IR has src1 operand on src0's place
            src1 = src0;
        }
        else
            return SUCCESS;
    }

    if (src1==NULL ||src1->isLabel()) return FAILURE;

    EncodeSrc1RegFile(mybin, src1);
    EncodeSrc1Type(mybin, src1);
    if (src1->isImm())
    {
        MUST_BE_TRUE(src1->getTypeSize() != 8, "64-bit immediate must be src0");
        EncodeSrcImmData(mybin, src1);
    }
    else
    {
        G4_SrcRegRegion *srcRegion = src1->asSrcRegRegion();
        EncodeSrc1RegNum(inst, mybin, src1);
        EncodeSrc1ArchRegNum(inst, mybin, srcRegion);
        EncodeSrc1IndirectRegNum(inst, mybin, srcRegion);
        EncodeSrc1AddrMode(mybin, src1);
        EncodeSrc1RepCtrl(mybin, srcRegion);
        EncodeSrc1Modifier(mybin, srcRegion);
        EncodeSrc1ChanSelect(inst, mybin, srcRegion);
        const RegionDesc *rd = srcRegion->getRegion();
        bool WidthValid = EncodeSrc1Width(inst, mybin, rd, src1);
        bool HorzStrideValid = EncodeSrc1HorzStride(inst, mybin, rd, src1);
        EncodeSrc1VertStride(inst, mybin, rd, src1, WidthValid, HorzStrideValid);
    }

    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//                       Src2
///////////////////////////////////////////////////////////////////////////////

inline void SetSrc2SrcMod(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcMod[4], bits3SrcSrcMod[5], value);
    else
        return;
}

inline void SetSrc2RepCtrl(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcRepCtrl_4, bits3SrcRepCtrl_5, value);
    else
        return;

}

inline void SetSrc2Swizzle(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSwizzle_4, bits3SrcSwizzle_5, value);
    else
        return;
}

inline void SetSrc2ChanSel_0(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_0_4, bits3SrcChanSel_0_5, value);
    else
        return;
}

inline void SetSrc2ChanSel_1(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_1_4, bits3SrcChanSel_1_5, value);
    else
        return;
}

inline void SetSrc2ChanSel_2(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_2_4, bits3SrcChanSel_2_5, value);
    else
        return;
}

inline void SetSrc2ChanSel_3(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcChanSel_3_4, bits3SrcChanSel_3_5, value);
    else
        return;
}

inline void SetSrc2RegNumHWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcRegNumHWord_4, bits3SrcSrcRegNumHWord_5, value);
    else
        return;
}

inline void SetSrc2RegNumOWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcRegNumOWord_4, bits3SrcSrcRegNumOWord_5, value);
    else
        return;
}

inline void SetSrc2RegNumDWord(BinInst *mybin, uint32_t value)
{
    if (mybin->GetIs3Src())
        mybin->SetBits(bits3SrcSrcRegNumDWord_4, bits3SrcSrcRegNumDWord_5, value);
    else
        return;
}

inline void EncodeSrc2ChanSelect(G4_INST *inst,
                                  BinInst *mybin,
                                  G4_SrcRegRegion *srcRegion,
                                  G4_Operand *src2)
{
    if (src2->isAccRegValid())
    {
        if (inst->opcode() == G4_madm)
        {
            uint32_t value = src2->getAccRegSel();
            SetSrc2ChanSel_0(mybin, value & 0x3);
            SetSrc2ChanSel_1(mybin, value >> 2 & 0x3);
            SetSrc2ChanSel_2(mybin, 0);
            SetSrc2ChanSel_3(mybin, 0);
            return;
        }
        ASSERT_USER(false, "acc2~acc7 were set on wrong instruction");
    }

    const char *swizzle = srcRegion->getSwizzle();
    {
        if (swizzle[0] != '\0' && swizzle[0] != 'r')
        {
            ChanSel ch0 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 0);
            ChanSel ch1 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 1);
            ChanSel ch2 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 2);
            ChanSel ch3 = EncodingHelper::GetSrcChannelSelectValue(srcRegion, 3);
            if (ch0 != CHAN_SEL_UNDEF)
                SetSrc2ChanSel_0(mybin, ch0);
            if (ch1 != CHAN_SEL_UNDEF)
                SetSrc2ChanSel_1(mybin, ch1);
            if (ch2 != CHAN_SEL_UNDEF)
                SetSrc2ChanSel_2(mybin, ch2);
            if (ch3 != CHAN_SEL_UNDEF)
                SetSrc2ChanSel_3(mybin, ch3);
        }
    }

    //Following ISAASM example which encodes it to xyzw and is able to compact as a result.
    if (swizzle[0] == '\0' || swizzle[0] == 'r')
    {
        if (inst->isAligned16Inst())
        {
            SetSrc2ChanSel_0(mybin, CHAN_SEL_X);
            SetSrc2ChanSel_1(mybin, CHAN_SEL_Y);
            SetSrc2ChanSel_2(mybin, CHAN_SEL_Z);
            SetSrc2ChanSel_3(mybin, CHAN_SEL_W);
        }
    }
}

inline void EncodeSrc2RepCtrl(BinInst *mybin, G4_SrcRegRegion *srcRegion)
{
    const char *swizzle = srcRegion->getSwizzle();
    if (swizzle[0] == 'r')
        SetSrc2RepCtrl(mybin, 0x1);
    else
        SetSrc2RepCtrl(mybin, 0x0);
}

inline void EncodeSrc2Modifier(BinInst *mybin, G4_SrcRegRegion *srcRegion, G4_Operand *src2)
{
    if (EncodingHelper::GetSrcRegFile(src2) != REG_FILE_M)
    {
        SetSrc2SrcMod(mybin, GetSrcMod(srcRegion));
    }
}

inline void BinaryEncoding::EncodeSrc2RegNum(G4_INST* inst, BinInst *mybin, G4_Operand *src2)
{
    if (EncodingHelper::GetSrcRegFile(src2) != REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src2) == ADDR_MODE_IMMED)
    {
        uint32_t byteAddress = src2->getLinearizedStart();
        MUST_BE_TRUE(byteAddress < kernel.getNumRegTotal() * kernel.numEltPerGRF<Type_UB>(), "src2 exceeds total GRF number");

        // encode dwords
        mybin->SetBits(bits3SrcSrc2RegDWord_H, bits3SrcSrc2RegDWord_L, byteAddress >> 2);
        // encode word (for word type)
        mybin->SetBits(bits3SrcSrc2SubRegNumW_H, bits3SrcSrc2SubRegNumW_L, (byteAddress >> 1) & 0x1);
    }
}

inline BinaryEncoding::Status BinaryEncoding::EncodeOperandSrc2(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    G4_Operand *src2 = inst->getSrc(2);

    if (src2==NULL || src2->isLabel())
    {
        return SUCCESS;
    }

    if (inst->isSplitSend())
    {
        return EncodeSplitSendSrc2(inst);
    }

    if (!src2->isImm())
    {
        G4_SrcRegRegion *srcRegion = src2->asSrcRegRegion();
        EncodeSrc2ChanSelect(inst, mybin, srcRegion, src2);
        EncodeSrc2RepCtrl(mybin, srcRegion);
        EncodeSrc2Modifier(mybin, srcRegion, src2);
        EncodeSrc2RegNum(inst, mybin, src2);
    }

    return SUCCESS;
}

void SetExtMsgDescr(G4_INST *inst, BinInst *mybin, uint32_t value)
{

    EncExtMsgDescriptor emd;
    emd.ulData = value;


    if (inst->isSplitSend())
    {
        G4_Operand *src3 = inst->getSrc(3);
        // additional extended msg desc to be encoded
        if (src3 && src3->isSrcRegRegion() && src3->asSrcRegRegion()->isA0())
        {
            mybin->SetBits(bitsSendsSelReg32ExDesc_0, bitsSendsSelReg32ExDesc_1, 1);
            mybin->SetBits(bitsSendsExDescRegNum_0, bitsSendsExDescRegNum_1,
                src3->asSrcRegRegion()->getBase()->asRegVar()->getPhyRegOff());
            return;
        }
    }

    // immediate extended message descriptor
    mybin->SetBits(bitsSharedFunctionID_0, bitsSharedFunctionID_1, emd.ExtMsgDescriptor.TargetUnitId);
    mybin->SetBits(bitsEndOfThread_0, bitsEndOfThread_1, emd.ExtMsgDescriptor.EndOfThread);
    mybin->SetBits(bitsExDescCPSLOD_0, bitsExDescCPSLOD_1, emd.ExtMsgDescriptor.CPSLODCompensation);


    if (inst->isSplitSend())
    {
        mybin->SetBits(bitsExMsgLength_0, bitsExMsgLength_1, emd.ExtMsgDescriptor.ExtMessageLength);
        mybin->SetBits(bitsSendsExDescFuncCtrl_0, bitsSendsExDescFuncCtrl_1, emd.ExtMsgDescriptor.ExtFunctionControl);
    }
    else if (inst->getPlatform() >= GENX_SKL)
    {
        // needs to encode extended message desc function control as well for SKL+
        uint32_t val = emd.ExtMsgDescriptor.ExtFunctionControl & 0xF;
        mybin->SetBits(bitsSendExDesc19, bitsSendExDesc16, val);
        val = (emd.ExtMsgDescriptor.ExtFunctionControl >> 4) & 0xF;
        mybin->SetBits(bitsSendExDesc23, bitsSendExDesc20, val);
        val = (emd.ExtMsgDescriptor.ExtFunctionControl >> 8) & 0xF;
        mybin->SetBits(bitsSendExDesc27, bitsSendExDesc24, val);
        val = (emd.ExtMsgDescriptor.ExtFunctionControl >> 12) & 0xF;
        mybin->SetBits(bitsSendExDesc31, bitsSendExDesc28, val);
    }
}

inline
BinaryEncoding::Status BinaryEncoding::EncodeExtMsgDescr(G4_INST* inst)
{
    BinInst *mybin = inst->getBinInst();
    MUST_BE_TRUE(inst->getMsgDescRaw(), "expected raw descriptor");
    uint32_t msgDesc = inst->getMsgDescRaw()->getExtendedDesc();
    SetExtMsgDescr(inst, mybin, msgDesc);
    return SUCCESS;
}

BinaryEncoding::Status BinaryEncoding::EncodeOperands(G4_INST* inst)
{
    Status myStatus = SUCCESS;

    //Don't need to encode src1 if math only has one src.
    int i = ((inst->isMath() && inst->asMathInst()->isOneSrcMath())?1:inst->getNumSrc());
    switch (i)
    {
        case 4: // split send's ext msg descriptor encoding is skipped here
        case 3:
            EncodeOperandSrc2(inst);
        case 2:
            EncodeOperandSrc1(inst);
        case 1:
            EncodeOperandSrc0(inst);
        default:
            break;
    }

    if (inst->opcode() == G4_jmpi      &&
         inst->getSrc(0)                &&
         inst->getSrc(0)->isSrcRegRegion())
    {
        EncodeOperandSrc1(inst);
    }

    if (inst->isSend())
        EncodeExtMsgDescr(inst); // all the ext msg descriptor is handled here

    return myStatus;
}

void BinaryEncoding::insertWaitDst(G4_INST *inst)
{
    BinInst *mybin = inst->getBinInst();
    if (inst->opcode()==G4_wait)
    {  // The wait instruction needs src0 inserted as dst operand
        G4_Operand *src0 = inst->getSrc(0);

        RegFile regFile = EncodingHelper::GetSrcRegFile(src0);
        SetDstRegFile(mybin, regFile);
        if (regFile == REG_FILE_A)
            SetDstArchRegFile(mybin, EncodingHelper::GetSrcArchRegType(src0));

        SetDstHorzStride(mybin, HORZ_STRIDE_1);
        SetDstAddrMode(mybin, EncodingHelper::GetSrcAddrMode(src0));

        // set dst's RegNum and RegSubNum according to its src0's.
        if (EncodingHelper::GetSrcRegFile(src0)!=REG_FILE_A &&
            EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_IMMED)
        {
            bool repControl = EncodingHelper::GetRepControl(src0);
            uint32_t byteAddress = src0->getLinearizedStart();

            if (inst->isAligned1Inst() || repControl)
            {
                SetDstRegNumByte(mybin, byteAddress);
            } else {
                SetDstRegNumOWord(mybin, byteAddress/BYTES_PER_OWORD);
            }
        }
    }
}

void BinaryEncoding::SetCompactCtrl (BinInst *mybin, uint32_t value)
{
    mybin->SetBits(bitsCompactCtrl_0, bitsCompactCtrl_1, value);
}

uint32_t BinaryEncoding::GetCompactCtrl(BinInst *mybin)
{
    return mybin->GetBits(bitsCompactCtrl_0, bitsCompactCtrl_1);
}

inline uint32_t GetComprCtrl(BinInst *mybin)
{
    return mybin->GetBits(bitsComprCtrl_0,  bitsComprCtrl_1);
}

bool isIndirectJmp(G4_INST *inst)
{
    return (inst                                &&
            inst->opcode() == G4_jmpi           &&
            inst->getSrc(0)->isSrcRegRegion()   &&
            ((G4_SrcRegRegion *)(inst->getSrc(0)))->getBase()->isRegVar());
}

bool isIndirectJmpTarget(G4_INST *inst)
{
    return (inst                                &&
            inst->opcode() == G4_jmpi           &&
            inst->getSrc(0)->isLabel());
}

BinaryEncoding::Status BinaryEncoding::DoAllEncoding(G4_INST* inst)
{
    Status myStatus = SUCCESS;
    bool isFCCall = false, isFCRet = false;

    if (inst->opcode() == G4_label) return myStatus;

    if (inst->opcode() == G4_illegal)
         return FAILURE;

    EncodingHelper::mark3Src(inst);

    insertWaitDst(inst);

    if (inst->opcode() == G4_pseudo_fc_call)
    {
        inst->asCFInst()->pseudoCallToCall();
        isFCCall = true;
    }

    if (inst->opcode() == G4_pseudo_fc_ret)
    {
        inst->asCFInst()->pseudoRetToRet();
        isFCRet = true;
    }

    EncodeOpCode(inst);

    if (inst->opcode() == G4_nop)
    {
        return myStatus;
    }

    EncodeExecSize(inst);
    EncodeFlagReg(inst);
    EncodeFlagRegPredicate(inst);
    EncodeCondModifier(inst);
    EncodeInstModifier(inst);
    EncodeInstOptionsString(inst);

    if (inst->isSend())
    {
        EncodeSendMsgDesc29_30(inst);
    }

    if (inst->opcode() == G4_math)
    {
        EncodeMathControl(inst);
    }

    if (isFCCall == true) {
        BinInst *mybin = inst->getBinInst();
        SetSrc1RegFile(mybin, REG_FILE_I);
        SetSrc1Type(mybin, SRC_IMM_TYPE_D);
        inst->setOpcode(G4_pseudo_fc_call);
    }

    if (isFCRet == true) {
        inst->setOpcode(G4_pseudo_fc_ret);
    }

    return myStatus;
}

//inline void dumpOptReport(int totalInst,
//                          int numCompactedInst,
//                          int numCompacted3SrcInst,
//                          G4_Kernel& kernel)
//{
//    if (kernel.getOption(vISA_OptReport))
//    {
//        std::ofstream optReport;
//        getOptReportStream(optReport, kernel.fg.builder->getOptions());
//        optReport << "             === Binary Compaction ===" <<endl;
//        optReport<< fixed << endl;
//        optReport<< kernel.getName() <<": "
//            << numCompactedInst <<" out of " <<totalInst <<" instructions are compacted."<<endl;
//        if (numCompacted3SrcInst>0)
//        {
//            optReport<< kernel.getName() <<": "
//            << numCompacted3SrcInst <<" instructions of 3 src (mad/pln) are compacted."<<endl;
//        }
//        optReport<< setprecision(0)
//            << (float)(numCompactedInst*100)/(float)(totalInst)
//            << "% instructions of this kernel are compacted."
//            <<endl;
//        optReport<<endl;
//        closeOptReportStream(optReport);
//    }
//}

inline BinaryEncoding::Status BinaryEncoding::ProduceBinaryInstructions()
{
    Status myStatus = FAILURE;
    BB_LIST_ITER ib, bend(kernel.fg.end());

    int globalInstNum = 0;
    int globalHalfInstNum = 0;
    int numCompactedInst = 0;
    int numCompacted3SrcInst = 0;
    //define offsetVector to record forward jumps/calls
    std::vector<ForwardJmpOffset> offsetVector;

    if (doCompaction())
    {
        for (uint8_t i=0; i<(int)COMPACT_TABLE_SIZE; i++)
        {
            BDWCompactControlTable.AddIndex(IVBCompactControlTable[i], i);
            BDWCompactSourceTable.AddIndex(IVBCompactSourceTable[i], i);
            BDWCompactSubRegTable.AddIndex(IVBCompactSubRegTable[i], i);
            BDWCompactSubRegTable.AddIndex1(IVBCompactSubRegTable[i] & 0x1F, i);
            BDWCompactSubRegTable.AddIndex2(IVBCompactSubRegTable[i] & 0x3FF, i);
            BDWCompactDataTypeTableStr.AddIndex(BDWCompactDataTypeTable[i], i);
        }
    }

     /**
     * Traverse the flow graph basic block
     */
    for (ib = kernel.fg.begin(); ib != bend; ++ib)
    {
        G4_BB *bb = *ib;
        int localInstNum = 0;
        int localHalfInstNum = 0;

        /**
         * Traverse the instruction lists
         */
        INST_LIST_ITER ii, iend(bb->end());
        //indirectJump = false;
        for (ii = bb->begin(); ii != iend; ++ii)
        {
            /* do detailed encoding here */
            G4_INST *inst = *ii;

            if (inst->opcode() == G4_label)
            {
                inst->setBinInst(NULL);
            }
            else
            {
                BinInst *bin = new (mem) BinInst();
                inst->setBinInst(bin);

                myStatus = DoAllEncoding(inst);


                if (inst->opcode() == G4_nop)
                {
                    binInstList.push_back(inst->getBinInst());
                    BuildLabelMap(inst, localHalfInstNum, localInstNum,
                        globalHalfInstNum, globalInstNum);
                    continue;
                }


                EncodeOperandDst(inst);

                if (!EncodingHelper::hasLabelString(inst))
                {
                    EncodeOperands(inst);
                }

                if (inst->opcode() == G4_pseudo_fc_call ||
                    inst->opcode() == G4_pseudo_fc_ret)
                {
                    inst->getBinInst()->SetDontCompactFlag(true);
                }

                if (doCompaction())
                {
                    // do not compact the instruction that mark as NoCompact
                    inst->getBinInst()->SetMustCompactFlag(false);
                    inst->getBinInst()->SetDontCompactFlag(inst->isNoCompactedInst());

                    /**
                     * handling switch/case for gen6: jump table should not be compacted
                     */
                    bool compacted = false;
                    {
                        TIME_SCOPE(ENCODE_COMPACTION);
                        compacted = compactOneInstruction(inst);
                    }
                    if (compacted)
                    {
                        if (kernel.getOption(vISA_OptReport))
                        {
                            numCompactedInst++;
                            if (inst->getBinInst()->GetIs3Src())
                                numCompacted3SrcInst++;
                        }
                        inst->setCompacted();
                    }
                }
                binInstList.push_back(inst->getBinInst());

                if (inst->opcode() >= G4_jmpi && inst->opcode() <= G4_join)
                {
                    if (!EncodeConditionalBranches(inst, globalHalfInstNum))
                    {
                        offsetVector.push_back(ForwardJmpOffset(inst, globalHalfInstNum));
                    }
                }
            } // else if

            BuildLabelMap(inst, localHalfInstNum, localInstNum,
                globalHalfInstNum, globalInstNum);

        } // for inst
    } // for bb
    kernel.setAsmCount(globalInstNum);
    SetInstCounts((uint32_t)globalHalfInstNum);

    EncodingHelper::dumpOptReport(globalInstNum, numCompactedInst, numCompacted3SrcInst, kernel);
    for (auto x = offsetVector.begin(), vEnd = offsetVector.end(); x != vEnd; x++)
    {
        // calculate offsets again since labels for forward jumps/calls
        // are available now
        if (!EncodeConditionalBranches(x->inst, x->offset))
        {
            MUST_BE_TRUE(false, "invalid label!");
        }
    }

    return myStatus;
}

void BinaryEncoding::DoAll()
{
    InitPlatform(kernel.getPlatform());
    FixInst();
    ProduceBinaryInstructions();
}

inline void SetBranchJIP(BinInst *mybin, uint32_t JIP)
{
    if (mybin->GetIs3Src())
        return;
    else
    {
        mybin->SetBits(bitsJIP[0], bitsJIP[1], JIP);
    }
}


inline void SetBranchJIPUIP(BinInst *mybin, uint32_t JIP, uint32_t UIP)
{
    if (mybin->GetIs3Src())
        return;
    else
    {
        mybin->SetBits(bitsJIP[0], bitsJIP[1], JIP);
        mybin->SetBits(bitsUIP[0], bitsUIP[1], UIP);
    }
}

void BinaryEncoding::SetBranchOffsets(G4_INST* inst,
                                      uint32_t JIP,
                                      uint32_t UIP)
{
    BinInst *mybin = inst->getBinInst();
    G4_opcode opc = inst->opcode();

    {
        //JIP and UIP are at src1
        //No need to set type and srcReg for src1
        //Not needed for instructions with JIP only
        //and over written by UIP

        if (opc == G4_if            ||
            opc == G4_break         ||
            opc == G4_cont          ||
            opc == G4_halt          ||
            opc == G4_goto          ||
            opc == G4_else)
        {
            SetSrc0RegFile(mybin, REG_FILE_I);
            SetSrc0Type(mybin, SRC_IMM_TYPE_D);
            SetBranchJIPUIP(mybin, JIP, UIP);
        }
        else
        {
            //JIP is at location src1 and must be of type D (signed dword integer).
            if ((opc == G4_while ||
                opc == G4_endif  ||
                opc == G4_join))
            {
                SetSrc1RegFile(mybin, REG_FILE_I);
                SetSrc1Type(mybin, SRC_IMM_TYPE_D);
            }
            SetBranchJIP(mybin, JIP);
        }
    }
}

inline bool isValidIPOffset(int32_t ipOffset)
{
    return true;
}

void BinaryEncoding::SetCmpSrc1Imm32(BinInst *mybin, uint32_t immediateData, G4_Operand* src)
{
    if (GetCompactCtrl(mybin))
    {
        SetCmpSrc1RegNum(mybin, immediateData & 0xff);          // 63:56
        SetCmpSrc1Index(mybin, (immediateData >> 8)& 0x1f);
    }
    else
    {
        SetSrc1Imm32(mybin, immediateData, src);
    }
}

/* EncodeConditionalBranches() returns true for backward jmp/call/jip/uip,
 * returns false for forward jmp/call/jip/uip.
 */
bool BinaryEncoding::EncodeConditionalBranches(G4_INST *inst,
                                   uint32_t insOffset)
{
    std::string jipLabel;
    std::string uipLabel;
    int32_t jipOffset = 0;
    int32_t uipOffset = 0;
    G4_opcode op = inst->opcode();

    // while and case only have JIP for all platforms
    // break, cont and halt have both JIP and UIP for all platforms
    if (op == G4_if    ||
         op == G4_while ||
         op == G4_else  ||
         op == G4_break ||
         op == G4_cont  ||
         op == G4_halt ||
         op == G4_goto ||
         op == G4_endif ||
         op == G4_join)
    {
        G4_Label* jip = inst->asCFInst()->getJip();
        if (jip)
        {
            int32_t info = GetLabelInfo(jip);
            if (info == -1)
            {
                return false;
            }
            jipOffset = info - insOffset;
            if (!isValidIPOffset(jipOffset))
            {
                MUST_BE_TRUE(false, "invalid IP offset");
            }
            // BDW+: in unit of bytes
            jipOffset *= (int32_t)JUMP_INST_COUNT_SIZE;
        }
        //Setting RegFile and Type in case JIP is 0.
        else if (op == G4_while ||
            op == G4_endif ||
            op == G4_join)
        {
            BinInst * mybin = inst->getBinInst();
            SetSrc1RegFile(mybin, REG_FILE_I);
            SetSrc1Type(mybin, SRC_IMM_TYPE_D);
        }
    }

    // halt has both JIP and UIP on all platforms
    // else has JIP for all platforms; else has UIP for BDW only
    if (op == G4_break                 ||
         op == G4_cont                  ||
         op == G4_halt                  ||
         op == G4_if                    ||
         op == G4_else                  ||
         op == G4_goto)
    {
        G4_Label* uip = inst->asCFInst()->getUip();
        if (uip)
        {
            int32_t info = GetLabelInfo(uip);
            if (info == -1)
            {
                return false;
            }
            uipOffset = info - insOffset;
            if (!isValidIPOffset(uipOffset))
            {
                MUST_BE_TRUE(false, "invalid IP offset");
            }
            uipOffset *= (uint32_t)JUMP_INST_COUNT_SIZE;
        }
    }

    if (op == G4_endif && jipOffset == 0)
    {
        jipOffset = INST_SIZE; // Next instruction ...
    }

    if (jipOffset != 0 || uipOffset != 0)
    {
        SetBranchOffsets(inst, jipOffset, uipOffset);
    }

    if (op == G4_jmpi              &&
         inst->getSrc(0)            &&
         inst->getSrc(0)->isLabel())
    {
        // find the label's IP count
        G4_Label *opnd = inst->getSrc(0)->asLabel();
        BinInst * mybin = inst->getBinInst();
        // Calculate the address offset
        // Label has the same IP count as the following instruction,
        // "break 1" is to the fall through instruction
        int32_t info = GetLabelInfo(opnd);
        if (info == -1)
        {
            return false;
        }
        int32_t jmpOffset = info - insOffset;
        if (GetCompactCtrl(mybin))
            jmpOffset -= 1;
        else
            jmpOffset -= 2;

        jmpOffset *= (int32_t)JUMP_INST_COUNT_SIZE;

        if (!isValidIPOffset(jmpOffset))
        {
            // generate
            // add (1) ip ip jmpOffset+16
            // to work around the 16-bit jump offset limit
            // jmpOffset is incremented by 1 since jmpi implicitly performs it
            // (e.g., jmpi 0 jumps to the next instruction)
            // everything else about the instruction should stay the same
            // (dst and src0 are both ip for example)
            SetOpCode(mybin, G4_add);
            jmpOffset += 16;
        }

        if (GetCompactCtrl(mybin))
        {
            SetCmpSrc1RegNum(mybin, jmpOffset & 0xff);          // 63:56
            SetCmpSrc1Index(mybin, (jmpOffset >> 8)& 0x1f);
        }
        else
        {
            SetSrc1RegFile(mybin, REG_FILE_I);
            SetSrc1Type(mybin, SRC_IMM_TYPE_D);
            SetSrc1Imm32(mybin, (uint32_t)jmpOffset, opnd);
        }
    }

    if (op == G4_call && inst->getSrc(0))
    {

        if (inst->getSrc(0)->isLabel())
        {
            G4_Label *opnd = inst->getSrc(0)->asLabel();
            int32_t info = GetLabelInfo(opnd);
            if (info == -1)
            {
                return false;
            }
            int32_t jmpOffset = info - insOffset;
            if (!isValidIPOffset(jmpOffset))
            {
                MUST_BE_TRUE(false, "invalid IP offset for call");
            }

            jmpOffset *= (int32_t)JUMP_INST_COUNT_SIZE;

            BinInst *mybin = inst->getBinInst();

            SetSrc0VertStride(mybin, 2);
            SetSrc0Width(mybin, 2);
            SetSrc0HorzStride(mybin, 1);


            SetSrc1RegFile(mybin, REG_FILE_I);
            SetSrc1Type(mybin, SRC_IMM_TYPE_D);

            SetCmpSrc1Imm32(mybin, jmpOffset, opnd);
        }
        else
        {
            // indirect call
            BinInst *mybin = inst->getBinInst();

            SetSrc0VertStride(mybin, 2);
            SetSrc0Width(mybin, 2);
            SetSrc0HorzStride(mybin, 1);
            EncodeIndirectCallTarget(inst);
        }
    }
    return true;
}

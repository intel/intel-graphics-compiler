/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "../Timer.h"

using namespace vISA;

//
// convert src into a direct packed region
//
static G4_SrcRegRegion* operandToDirectSrcRegRegion(
    IR_Builder& builder, G4_Operand* src, G4_ExecSize newSize, G4_ExecSize oldSize)
{
    if (src->isSrcRegRegion())
    {
        G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
        if (srcRegion->getRegAccess() == IndirGRF)
        {
            G4_Declare* dcl = builder.createTempVarWithNoSpill(newSize, src->getType(), Any);
            if (srcRegion->getRegion()->isRegionWH() && newSize > oldSize)
            {
                // for VxH regions we can't directly broadcast if new exec size is wider
                if (oldSize == g4::SIMD1)
                {
                    srcRegion->setRegion(builder, builder.getRegionScalar());
                    builder.createMov(newSize, builder.createDstRegRegion(dcl, 1), srcRegion,
                        InstOpt_WriteEnable, true);
                }
                else
                {
                    // ToDo: i think this is needed for all regions
                    auto tmpDst = builder.createDstRegRegion(dcl, 1);
                    builder.createMov(oldSize, tmpDst, src, InstOpt_WriteEnable, true);
                    auto tmpSrc = builder.createSrc(dcl->getRegVar(), 0, 0,
                        builder.createRegionDesc(0, oldSize, 1), src->getType());
                    builder.createMov(newSize, builder.createDstRegRegion(dcl, 1), tmpSrc,
                        InstOpt_WriteEnable, true);
                }
            }
            else
            {
                builder.createMovInst(dcl, 0, 0, newSize, nullptr, nullptr, src, true);
            }
            return builder.createSrcRegRegion(dcl, builder.getRegionStride1());
        }
        return src->asSrcRegRegion();
    }
    else
    {
        // src is an immediate
        MUST_BE_TRUE(src->isImm(), "expected an immediate operand");
        G4_Declare *tmpSrc = builder.createTempVarWithNoSpill(newSize, G4_Operand::GetNonVectorImmType(src->getType()), Any);
        builder.createMovInst(tmpSrc, 0, 0, newSize, nullptr, nullptr, src, true);
        return builder.createSrcRegRegion(tmpSrc, builder.getRegionStride1());
    }
}

void IR_Builder::expandFdiv(
    G4_ExecSize exsize, G4_Predicate *predOpnd, G4_Sat saturate,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd,
    uint32_t instOpt)
{
    // math.fdiv dst src0 src1
    // -->
    // math.inv tmp src1
    // mul dst src0 tmp
    G4_MathOp mathOp = MATH_INV;
    G4_Type invType = src1Opnd->getType();
    if (IS_VFTYPE(invType))
    {
        invType = Type_F;
    }
    G4_Declare* invResult = createTempVar(exsize, invType, Any);
    G4_DstRegRegion* invDst = createDstRegRegion(invResult, 1);
    createMathInst(predOpnd, g4::NOSAT, exsize, invDst, src1Opnd, createNullSrc(invType), mathOp, instOpt, true);
    G4_SrcRegRegion* invSrc = createSrcRegRegion(invResult, getRegionStride1());
    createInst(duplicateOperand(predOpnd), G4_mul, nullptr, saturate, exsize, dstOpnd, src0Opnd, invSrc, instOpt, true);
}

void IR_Builder::expandPow(
    G4_ExecSize exsize, G4_Predicate *predOpnd, G4_Sat saturate,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd,
    uint32_t instOpt)
{
    // math.pow dst src0 src1
    // -->
    // math.log tmp abs(src0)
    // mul dst tmp tmp src1
    // math.exp dst tmp
    G4_Type mathType = G4_Operand::GetNonVectorImmType(src0Opnd->getType());
    G4_Declare* tmpVar = createTempVar(exsize, mathType, Any);
    G4_DstRegRegion* logDst = createDstRegRegion(tmpVar, 1);
    G4_Operand* logSrc = src0Opnd;
    // make sure log source is positive
    if (src0Opnd->isSrcRegRegion())
    {
        G4_SrcRegRegion* srcRegion = src0Opnd->asSrcRegRegion();
        switch (srcRegion->getModifier())
        {
        case Mod_src_undef:
            srcRegion->setModifier(Mod_Abs);
            break;
        case Mod_Abs:
            // do nothing
            break;
        default:
        {
            G4_Declare* tmpLogSrc = createTempVar(exsize, src0Opnd->getType(), Any);
            createMovInst(tmpLogSrc, 0, 0, exsize, nullptr, nullptr, src0Opnd, false, instOpt);
            logSrc = createSrcRegRegion(tmpLogSrc, getRegionStride1());
            logSrc->asSrcRegRegion()->setModifier(Mod_Abs);
        }
        }
    }
    else
    {
        switch (src0Opnd->getType())
        {
        case Type_F:
        {
            float val = src0Opnd->asImm()->getFloat();
            if (val < 0)
            {
                logSrc = createImm(std::abs(val));
            }
            break;
        }
        case Type_HF:
        {
            uint16_t val = (uint16_t) src0Opnd->asImm()->getImm();
            if (val & 0x8000)
            {
                logSrc = createImm(val & 0x7FFFF, Type_HF);
            }
            break;
        }
        case Type_VF:
            // ToDo: what if VF contains negative values?
            break;
        default:
            assert(false && "unexpected src0 type for pow");
        }
    }
    createMathInst(predOpnd, g4::NOSAT, exsize,
        logDst, logSrc, createNullSrc(mathType), MATH_LOG, instOpt, true);
    G4_SrcRegRegion* mulSrc = createSrcRegRegion(tmpVar, getRegionStride1());
    G4_DstRegRegion* mulDst = createDstRegRegion(tmpVar, 1);
    createInst(duplicateOperand(predOpnd), G4_mul,
        nullptr, g4::NOSAT, exsize, mulDst, mulSrc, src1Opnd, instOpt, true);
    G4_SrcRegRegion* expSrc = createSrcRegRegion(tmpVar, getRegionStride1());
    createMathInst(duplicateOperand(predOpnd), saturate, exsize, dstOpnd, expSrc, createNullSrc(mathType), MATH_EXP, instOpt, true);
}

static void setDefaultRoundDenorm(
    IR_Builder& builder,
    bool hasDefaultRoundDenorm,
    G4_Declare* regCR0)
{
    if (!hasDefaultRoundDenorm)
    {
        // save cr0.0: mov (1) r116.2<1>:ud cr0.0<0;1,0>:ud {NoMask}
        G4_SrcRegRegion* cr0SrcRegOpndForSaveInst = builder.createSrc(builder.phyregpool.getCr0Reg(), 0, 0, builder.getRegionScalar(), Type_UD);
        builder.createMov(g4::SIMD1, builder.createDstRegRegion(regCR0, 1),
            cr0SrcRegOpndForSaveInst, InstOpt_WriteEnable, true);

        // set rounding mod in CR0 to RNE: and (1) cr0.0<1>:ud cr0.0<0;1,0>:ud 0xffffffcf:ud {NoMask}
        G4_DstRegRegion* cr0DstRegOpndForAndInst = builder.createDst(builder.phyregpool.getCr0Reg(), 0, 0, 1, Type_UD);
        G4_SrcRegRegion* cr0SrcRegOpndForAndInst = builder.createSrc(builder.phyregpool.getCr0Reg(), 0, 0, builder.getRegionScalar(), Type_UD);
        builder.createBinOp(nullptr, G4_and, g4::SIMD1, cr0DstRegOpndForAndInst, cr0SrcRegOpndForAndInst,
            builder.createImm(0xffffffcf, Type_UD), InstOpt_WriteEnable, true);
    }
}

static void restoreCR0_0(
    IR_Builder& builder,
    bool hasDefaultRoundDenorm,
    G4_Declare* regCR0)
{
    if (!hasDefaultRoundDenorm)
    {
        G4_DstRegRegion* cr0DstRegOpndForRestoreIfInst = builder.createDst(builder.phyregpool.getCr0Reg(), 0, 0, 1, Type_UD);
        auto tmpSrcOpndForCR0OnIf = builder.createSrcRegRegion(regCR0, builder.getRegionScalar());

        // restore cr0.0
        builder.createMov(g4::SIMD1, cr0DstRegOpndForRestoreIfInst, tmpSrcOpndForCR0OnIf,
            InstOpt_WriteEnable, true);
    }
}

int IR_Builder::translateVISAArithmeticDoubleInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize,
    VISA_EMask_Ctrl emask, G4_Predicate *predOpnd, G4_Sat saturate,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_INST* inst = nullptr;
    G4_ExecSize instExecSize = toExecSize(executionSize);
    G4_ExecSize exsize = (G4_ExecSize)(getNativeExecSize() / 2);
    const RegionDesc *srcRegionDesc = getRegionStride1();
    const RegionDesc *rdAlign16 = getRegionStride1();
    uint8_t element_size = exsize;       // element_size is set according to instExecSize
    unsigned int loopCount = 1;

    G4_Imm *dbl_constant_0 = createDFImm(0.0);
    G4_Imm *dbl_constant_1 = createDFImm(1.0);

    {
        if (instExecSize > exsize)
        {
            element_size = instExecSize;
            exsize = std::min(instExecSize, G4_ExecSize(getFP64MadmExecSize()));
            loopCount = instExecSize / exsize;
        }
    }

    G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);

    // pred and conModifier
    G4_Declare *tmpFlag = createTempFlag(1);
    G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

    // temp registers
    G4_Declare *t6  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t7  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t8  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t9  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t12 = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t13 = createTempVarWithNoSpill(element_size, Type_DF, Any);

    // r0 = 0.0:df, r1 = 1.0:df
    G4_Declare *t0  = getImmDcl(dbl_constant_0, element_size);
    G4_Declare *t1  = getImmDcl(dbl_constant_1, element_size);

    inst = createPseudoKills({ t6, t7, t8, t9, t10, t11, t12, t13, tmpFlag }, PseudoKillType::Src);

    G4_SrcRegRegion tsrc0(*this, Mod_src_undef, Direct, t0->getRegVar(), 0, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion tsrc1(*this, Mod_src_undef, Direct, t1->getRegVar(), 0, 0, srcRegionDesc, Type_DF);

    if (!src0Opnd)
    {
        // those are for drcp
        G4_SrcRegRegion valueOneScalarReg(*this, Mod_src_undef, Direct, t1->getRegVar(), 0, 0, getRegionScalar(), Type_DF);
        G4_Operand* valueOneOpnd = createSrcRegRegion(valueOneScalarReg); // it is used in drcp
        src0Opnd = valueOneOpnd;
    }

    bool noDstMove = exsize == 8 && !saturate && !predOpnd && isOpndAligned(dstOpnd, getGRFSize()) &&
        dstOpnd->getRegAccess() == Direct && dstOpnd->getHorzStride() == 1 && instExecSize == exsize;
    if (noDstMove && (dstOpnd->getTopDcl() == src0Opnd->getTopDcl() || dstOpnd->getTopDcl() == src1Opnd->getTopDcl()))
    {
        noDstMove = false;
    }

    G4_SrcRegRegion* src0RR = operandToDirectSrcRegRegion(*this, src0Opnd, G4_ExecSize(element_size), instExecSize);
    G4_SrcRegRegion* src1RR = operandToDirectSrcRegRegion(*this, src1Opnd, G4_ExecSize(element_size), instExecSize);

    // src operand registers
    G4_DstRegRegion tdst_src0(*this, Direct, t6->getRegVar(), 0, 0, 1, Type_DF);
    G4_DstRegRegion tdst_src1(*this, Direct, t7->getRegVar(), 0, 0, 1, Type_DF);

    bool needsSrc0Move = src0RR->isScalar() || src0RR->getModifier() != Mod_src_undef;
    if (needsSrc0Move)
    {
        if (opcode == ISA_DIV || opcode == ISA_DIVM)
        {
            G4_DstRegRegion *t6_dst_src0_opnd = createDstRegRegion(tdst_src0);
            inst = createMov(G4_ExecSize(element_size), t6_dst_src0_opnd, src0RR,
                instOpt, true); // mov (element_size) t6_dst_src0_opnd, src0RR {Q1/N1}
        }
    }
    bool needsSrc1Move = src1RR->isScalar() || src1RR->getModifier() != Mod_src_undef;
    if (needsSrc1Move)
    {
        G4_DstRegRegion *t7_dst_src1_opnd = createDstRegRegion(tdst_src1);
        inst = createMov(G4_ExecSize(element_size), t7_dst_src1_opnd, src1RR,
            instOpt, true); // mov (element_size) t7_dst_src1_opnd, src1RR {Q1/N1}
    }

    bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
    // cr0.0 register
    G4_Declare* regCR0 = createTempVarWithNoSpill(1, Type_UD, Any);

    // each madm only handles 4 channel double data
    VISA_EMask_Ctrl currEMask = emask;
    uint16_t splitInstGRFSize = (uint16_t)((TypeSize(Type_DF) * exsize + getGRFSize() - 1) / getGRFSize());
    for (uint16_t loopIndex = 0; currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
        ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize))
    {

        uint16_t regIndex = loopIndex * splitInstGRFSize;
        instOpt = Get_Gen4_Emask(currEMask, exsize);
        instOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0; // setting channels for non-mad insts
        unsigned int madmInstOpt = instOpt; // setting channels for mad insts

        G4_DstRegRegion tdst6(*this, Direct, t6->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst7(*this, Direct, t7->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst8(*this, Direct, noDstMove ? dstOpnd->getBase() : t8->getRegVar(),
            noDstMove ? dstOpnd->getRegOff() + regIndex : regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst12(*this, Direct, t12->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst13(*this, Direct, t13->getRegVar(), regIndex, 0, 1, Type_DF);

        /* below 2 are prepared for G4_math with Align16, so the region 2;2,1 is used not 4;4,1.*/
        G4_SrcRegRegion tsrc6_0(*this, Mod_src_undef, Direct, t6->getRegVar(), regIndex, 0, rdAlign16, Type_DF);
        G4_SrcRegRegion tsrc7_0(*this, Mod_src_undef, Direct, t7->getRegVar(), regIndex, 0, rdAlign16, Type_DF);

        G4_SrcRegRegion tsrc6(*this, Mod_src_undef, Direct, t6->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc7(*this, Mod_src_undef, Direct, t7->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, noDstMove ? dstOpnd->getBase() : t8->getRegVar(),
            noDstMove ? dstOpnd->getRegOff() + regIndex : regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc12(*this, Mod_src_undef, Direct, t12->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc13(*this, Mod_src_undef, Direct, t13->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);

        G4_DstRegRegion *t8DstOpnd0 = createDstRegRegion(tdst8);
        G4_DstRegRegion *t8DstOpnd1 = createDstRegRegion(tdst8);
        G4_DstRegRegion *t8DstOpnd2 = createDstRegRegion(tdst8);
        G4_DstRegRegion *t9DstOpnd0 = createDstRegRegion(tdst9);
        G4_DstRegRegion *t9DstOpnd1 = createDstRegRegion(tdst9);
        G4_DstRegRegion *t10DstOpnd0 = createDstRegRegion(tdst10);
        G4_DstRegRegion *t11DstOpnd0 = createDstRegRegion(tdst11);
        G4_DstRegRegion *t11DstOpnd1 = createDstRegRegion(tdst11);
        G4_DstRegRegion *t12DstOpnd0 = createDstRegRegion(tdst12);
        G4_DstRegRegion *t12DstOpnd1 = createDstRegRegion(tdst12);
        G4_DstRegRegion *t13DstOpnd0 = createDstRegRegion(tdst13);


        // src oprands passed by function calls
        // for INV instruction, src0 should be 1, contant value.
        /* below 2 are prepared for G4_math with Align16, so the region 2;2,1 is used not 4;4,1.*/
        G4_SrcRegRegion fsrc0_0(*this, Mod_src_undef, Direct, src0RR->getBase(), src0RR->getRegOff() + ((opcode == ISA_INV) ? 0 : regIndex), 0, rdAlign16, Type_DF);
        G4_SrcRegRegion fsrc1_0(*this, Mod_src_undef, Direct, src1RR->getBase(), src1RR->getRegOff() + regIndex, 0, rdAlign16, Type_DF);

        G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(), src0RR->getRegOff() + ((opcode == ISA_INV) ? 0 : regIndex), 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion fsrc1(*this, Mod_src_undef, Direct, src1RR->getBase(), src1RR->getRegOff() + regIndex, 0, srcRegionDesc, Type_DF);

        G4_SrcRegRegion *t6SrcOpnd0 = NULL;
        G4_SrcRegRegion *t6SrcOpnd1 = NULL;
        G4_SrcRegRegion *t6SrcOpnd2 = NULL;
        G4_SrcRegRegion *t6SrcOpnd3 = NULL;
        G4_SrcRegRegion *t7SrcOpnd0 = NULL;
        G4_SrcRegRegion *t8SrcOpnd0x0 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x1 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x2 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x3 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x4 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd1 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t9SrcOpnd0x0 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd0x1 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd1x0 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd1x1 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t10SrcOpnd0 = createSrcRegRegion(tsrc10);
        G4_SrcRegRegion *t10SrcOpnd1 = createSrcRegRegion(tsrc10);
        G4_SrcRegRegion *t11SrcOpnd0 = createSrcRegRegion(tsrc11);
        G4_SrcRegRegion *t11SrcOpnd1 = createSrcRegRegion(tsrc11);
        G4_SrcRegRegion *t12SrcOpnd0x0 = createSrcRegRegion(tsrc12);
        G4_SrcRegRegion *t12SrcOpnd0x1 = createSrcRegRegion(tsrc12);
        G4_SrcRegRegion *t12SrcOpnd0x2 = createSrcRegRegion(tsrc12);
        G4_SrcRegRegion *t12SrcOpnd0x3 = createSrcRegRegion(tsrc12);
        G4_SrcRegRegion *t12SrcOpnd1 = createSrcRegRegion(tsrc12);
        G4_SrcRegRegion *t13SrcOpnd0 = createSrcRegRegion(tsrc13);

        // save CR, and then set rounding mode to RNE if hasDefaultRoundDenorm is false
        setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, regCR0);

        if (needsSrc0Move)
        {
            if (opcode == ISA_INV)
            {
                t6SrcOpnd0 = createSrcRegRegion(fsrc0_0);
                t6SrcOpnd1 = createSrcRegRegion(fsrc0);
                t6SrcOpnd2 = createSrcRegRegion(fsrc0);
                t6SrcOpnd3 = createSrcRegRegion(fsrc0);
            }
            else
            {
                t6SrcOpnd0 = createSrcRegRegion(tsrc6_0);
                t6SrcOpnd1 = createSrcRegRegion(tsrc6);
                t6SrcOpnd2 = createSrcRegRegion(tsrc6);
                t6SrcOpnd3 = createSrcRegRegion(tsrc6);
            }
        }
        else
        {
            t6SrcOpnd0 = createSrcRegRegion(fsrc0_0);
            t6SrcOpnd1 = createSrcRegRegion(fsrc0);
            t6SrcOpnd2 = createSrcRegRegion(fsrc0);
            t6SrcOpnd3 = createSrcRegRegion(fsrc0);
        }

        if (needsSrc1Move)
        {
            t7SrcOpnd0 = createSrcRegRegion(tsrc7_0);
        }
        else
        {
            t7SrcOpnd0 = createSrcRegRegion(fsrc1_0);
        }

        // create -r7.noacc
        G4_SrcRegRegion tsrc7_neg(
            *this,
            Mod_Minus,
            t7SrcOpnd0->getRegAccess(),
            t7SrcOpnd0->getBase(),
            t7SrcOpnd0->getRegOff(),
            t7SrcOpnd0->getSubRegOff(),
            t7SrcOpnd0->getRegion(),
            t7SrcOpnd0->getType());
        G4_SrcRegRegion *t7SrcOpndNeg0 = createSrcRegRegion(tsrc7_neg);
        G4_SrcRegRegion *t7SrcOpndNeg1 = createSrcRegRegion(tsrc7_neg);
        G4_SrcRegRegion *t7SrcOpndNeg2 = createSrcRegRegion(tsrc7_neg);
        G4_SrcRegRegion *t7SrcOpndNeg3 = createSrcRegRegion(tsrc7_neg);

        // math.e0.f0.0 (4) r8.acc2 r6.noacc r7.noacc 0xe {Align16, N1/N2}
        t8DstOpnd0->setAccRegSel(ACC2);
        t6SrcOpnd0->setAccRegSel(NOACC);
        t7SrcOpnd0->setAccRegSel(NOACC);
        inst = createMathInst(NULL, g4::NOSAT, exsize, t8DstOpnd0, t6SrcOpnd0,
            t7SrcOpnd0, MATH_INVM, madmInstOpt, true);
        G4_CondMod *condModOverflow = createCondMod(Mod_o, tmpFlag->getRegVar(), 0);
        inst->setCondMod(condModOverflow);

        bool generateIf = true;

        if (generateIf)
        {
            // if
            if (isNoMask(emask))
            {
                G4_Declare *tmpFlag2 = createTempFlag(2);
                inst = createBinOp(G4_and, g4::SIMD1, createDstRegRegion(Direct, tmpFlag2->getRegVar(),
                    0, 0, 1, Type_UD, ACC_UNDEFINED),
                    createSrcRegRegion(tmpFlag, getRegionScalar()),
                    createImm(0x1, Type_UD), InstOpt_WriteEnable, true);

                G4_Predicate *predicateFlagReg = createPredicate(PredState_Minus, tmpFlag2->getRegVar(),
                    0, getPlatform() >= Xe_PVC ? PRED_DEFAULT : PRED_ANY32H);
                inst = createIf(predicateFlagReg, G4_ExecSize(32), instOpt);
            }
            else
            {
                G4_Predicate *predicateFlagReg = createPredicate(PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
                inst = createIf(predicateFlagReg, exsize, instOpt);
            }
        }
        {
            // madm (4) r9.acc3 r0.noacc r6.noacc r8.acc2 {Align16, N1/N2}
            G4_SrcRegRegion *t0SrcOpnd = createSrcRegRegion(tsrc0);
            t9DstOpnd0->setAccRegSel(ACC3);
            t0SrcOpnd->setAccRegSel(NOACC);
            t6SrcOpnd1->setAccRegSel(NOACC);
            t8SrcOpnd0x0->setAccRegSel(ACC2);
            inst = createMadm(exsize,
                t9DstOpnd0, t0SrcOpnd,
                t6SrcOpnd1, t8SrcOpnd0x0, madmInstOpt);

            // madm (4) r10.acc4 r1.noacc -r7.noacc r8.acc2 {Align16, N1/N2}
            G4_SrcRegRegion *t1SrcOpnd0 = createSrcRegRegion(tsrc1);
            t10DstOpnd0->setAccRegSel(ACC4);
            t1SrcOpnd0->setAccRegSel(NOACC);
            t7SrcOpndNeg0->setAccRegSel(NOACC);
            t8SrcOpnd0x1->setAccRegSel(ACC2);
            inst = createMadm(exsize,
                t10DstOpnd0, t1SrcOpnd0,
                t7SrcOpndNeg0, t8SrcOpnd0x1, madmInstOpt);

            // madm (4) r11.acc5 r6.noacc -r7.noacc r9.acc3 {Align16, N1/N2}
            t11DstOpnd0->setAccRegSel(ACC5);
            t6SrcOpnd2->setAccRegSel(NOACC);
            t7SrcOpndNeg1->setAccRegSel(NOACC);
            t9SrcOpnd0x0->setAccRegSel(ACC3);
            inst = createMadm(exsize,
                t11DstOpnd0, t6SrcOpnd2,
                t7SrcOpndNeg1, t9SrcOpnd0x0, madmInstOpt);

            // madm (4) r12.acc6 r8.acc2 r10.acc4 r8.acc2 {Align16, N1/N2}
            t12DstOpnd0->setAccRegSel(ACC6);
            t8SrcOpnd0x2->setAccRegSel(ACC2);
            t10SrcOpnd0->setAccRegSel(ACC4);
            t8SrcOpnd0x3->setAccRegSel(ACC2);
            inst = createMadm(exsize,
                t12DstOpnd0, t8SrcOpnd0x2,
                t10SrcOpnd0, t8SrcOpnd0x3, madmInstOpt);

            // madm (4) r13.acc7 r1.noacc -r7.noacc r12.acc6 {Align16, N1/N2}
            G4_SrcRegRegion *t1SrcOpnd1 = createSrcRegRegion(tsrc1);
            t13DstOpnd0->setAccRegSel(ACC7);
            t1SrcOpnd1->setAccRegSel(NOACC);
            t7SrcOpndNeg2->setAccRegSel(NOACC);
            t12SrcOpnd0x0->setAccRegSel(ACC6);
            inst = createMadm(exsize,
                t13DstOpnd0, t1SrcOpnd1,
                t7SrcOpndNeg2, t12SrcOpnd0x0, madmInstOpt);

            // madm (4) r8.acc8 r8.acc2 r10.acc4 r12.acc6 {Align16, N1/N2}
            t8DstOpnd1->setAccRegSel(ACC8);
            t8SrcOpnd0x4->setAccRegSel(ACC2);
            t10SrcOpnd1->setAccRegSel(ACC4);
            t12SrcOpnd0x1->setAccRegSel(ACC6);
            inst = createMadm(exsize,
                t8DstOpnd1, t8SrcOpnd0x4,
                t10SrcOpnd1, t12SrcOpnd0x1, madmInstOpt);

            // madm (4) r9.acc9 r9.acc3 r11.acc5 r12.acc6 {Align16, N1/N2}
            t9DstOpnd1->setAccRegSel(ACC9);
            t9SrcOpnd0x1->setAccRegSel(ACC3);
            t11SrcOpnd0->setAccRegSel(ACC5);
            t12SrcOpnd0x2->setAccRegSel(ACC6);
            inst = createMadm(exsize,
                t9DstOpnd1, t9SrcOpnd0x1,
                t11SrcOpnd0, t12SrcOpnd0x2, madmInstOpt);

            // madm (4) r12.acc2 r12.acc6 r8.acc8 r13.acc7 {Align16, N1/N2}
            t12DstOpnd1->setAccRegSel(ACC2);
            t12SrcOpnd0x3->setAccRegSel(ACC6);
            t8SrcOpnd1->setAccRegSel(ACC8);
            t13SrcOpnd0->setAccRegSel(ACC7);
            inst = createMadm(exsize, t12DstOpnd1, t12SrcOpnd0x3,
                t8SrcOpnd1, t13SrcOpnd0, madmInstOpt);

            // madm (4) r11.acc3 r6.noacc -r7.noacc r9.acc9 {Align16, N1/N2}
            t11DstOpnd1->setAccRegSel(ACC3);
            t6SrcOpnd3->setAccRegSel(NOACC);
            t7SrcOpndNeg3->setAccRegSel(NOACC);
            t9SrcOpnd1x0->setAccRegSel(ACC9);
            inst = createMadm(exsize, t11DstOpnd1, t6SrcOpnd3,
                t7SrcOpndNeg3, t9SrcOpnd1x0, madmInstOpt);

            // restore Rounding Mode in CR if hasDefaultRoundDenorm is false
            restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);

            // madm (4) r8.noacc r9.acc9 r11.acc3 r12.acc2 {Align16, N1/N2}
            t8DstOpnd2->setAccRegSel(NOACC);
            t9SrcOpnd1x1->setAccRegSel(ACC9);
            t11SrcOpnd1->setAccRegSel(ACC3);
            t12SrcOpnd1->setAccRegSel(ACC2);
            inst = createMadm(exsize,
                t8DstOpnd2, t9SrcOpnd1x1,
                t11SrcOpnd1, t12SrcOpnd1, madmInstOpt);
        }

        if (generateIf)
        {
            if (!hasDefaultRoundDenorm)
            {
                // else (8) {Q1/Q2}
                createElse(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);

                // restore Rounding Mode in CR
                restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);
            }

            // endif (8) {Q1/Q2}
            inst = createEndif(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);
        }
    }; //for loop

    if (!noDstMove)
    {
        // make final copy to dst
        // dst = r8:df     mov (instExecSize) dstOpnd, t8_src_opnd_final {Q1/N1}
        // final result is at r8.noacc
        G4_SrcRegRegion tsrc8_final(*this, Mod_src_undef, Direct, t8->getRegVar(), 0, 0, getRegionStride1(), Type_DF);
        G4_SrcRegRegion *t8_src_opnd_final = createSrcRegRegion(tsrc8_final);
        t8_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
        inst = createInst(
            predOpnd, G4_mov, nullptr, saturate, instExecSize,
            dstOpnd, t8_src_opnd_final, NULL,
            Get_Gen4_Emask(emask, instExecSize), true);
    }

    return VISA_SUCCESS;
}

int IR_Builder::translateVISAArithmeticSingleDivideIEEEInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize,
    VISA_EMask_Ctrl emask, G4_Predicate *predOpnd,
    G4_Sat saturate, G4_CondMod* condMod,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_INST* inst;
    G4_ExecSize instExecSize = toExecSize(executionSize);
    uint8_t element_size = 8; // element_size is changed according to insstExecSize
    G4_ExecSize exsize = getNativeExecSize(); // exsize is a constant and never changed
    unsigned int loopCount = 1;
    G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize); // for those execution size: element_size before the loop
    G4_InstOpts madmInstOpt = Get_Gen4_Emask(emask, exsize); // only used in the loop
    const RegionDesc *srcRegionDesc = getRegionStride1();

    G4_Imm *flt_constant_0 = createImm(float(0.0));
    G4_Imm *flt_constant_1 = createImm(float(1.0));
    if (instExecSize <= exsize)
    {
        element_size = exsize;
        loopCount = 1;
    }
    else
    {
        element_size = instExecSize;
        instOpt = Get_Gen4_Emask(emask, instExecSize);
        loopCount = instExecSize / exsize;
    }

    // pred and conModifier
    G4_Declare *tmpFlag = createTempFlag(1);
    G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

    // temp registers
    G4_Declare *t1  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t4  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t6  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t8  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t9  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_F, Any);

    // 0.0:f and 1.0:f constants
    G4_Declare *t2 = getImmDcl(flt_constant_0, exsize);
    G4_Declare *t5 = getImmDcl(flt_constant_1, exsize);

    inst = createPseudoKills({ t1, t4, t6, t8, t9, t10, t11, tmpFlag }, PseudoKillType::Src);

    // those are for drcp
    G4_SrcRegRegion valueOneScalarReg(*this, Mod_src_undef, Direct, t2->getRegVar(), 0, 0, getRegionScalar(), Type_F);
    G4_Operand *valueOneOpnd = createSrcRegRegion(valueOneScalarReg); // it is used in drcp

    if (src0Opnd == NULL)
    {
        src0Opnd = valueOneOpnd;
    }

    G4_SrcRegRegion* src0RR = operandToDirectSrcRegRegion(*this, src0Opnd, G4_ExecSize(element_size), instExecSize);
    G4_SrcRegRegion* src1RR = operandToDirectSrcRegRegion(*this, src1Opnd, G4_ExecSize(element_size), instExecSize);

    if (src0RR->isScalar() || src0RR->getModifier() != Mod_src_undef)
    {
        G4_DstRegRegion *tmp = createDstRegRegion(t6, 1);
        inst = createMov(G4_ExecSize(element_size), tmp, src0RR,
            instOpt, true); // mov (element_size) t6, src0RR {Q1/H1}
        src0RR = createSrcRegRegion(t6, getRegionStride1());
    }
    if (src1RR->isScalar() || src1RR->getModifier() != Mod_src_undef)
    {
        G4_DstRegRegion *tmp = createDstRegRegion(t4, 1);
        inst = createMov(G4_ExecSize(element_size), tmp, src1RR,
            instOpt, true); // mov (element_size) t4, src1RR {Q1/H1}
        src1RR = createSrcRegRegion(t4, getRegionStride1());
    }

    // t2 and t5 are constants
    G4_SrcRegRegion tsrc2(*this, Mod_src_undef, Direct, t2->getRegVar(), 0, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc5(*this, Mod_src_undef, Direct, t5->getRegVar(), 0, 0, srcRegionDesc, Type_F);

    G4_SrcRegRegion tsrc8_final(*this, Mod_src_undef, Direct, t8->getRegVar(), 0, 0,
        getRegionStride1(), t8->getElemType());
    G4_SrcRegRegion *t8_src_opnd_final = createSrcRegRegion(tsrc8_final);

    bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
    // cr0.0 register
    G4_Declare* regCR0 = createTempVarWithNoSpill(1, Type_UD, Any);

    VISA_EMask_Ctrl currEMask = emask;
    uint16_t splitInstGRFSize = (uint16_t)((TypeSize(Type_F) * exsize + getGRFSize() - 1) / getGRFSize());
    for (uint16_t loopIndex = 0; currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
        ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize))
    {
        uint16_t regIndex = loopIndex * splitInstGRFSize;
        // set Q1 for insts within the 1st loop and Q2 for the 2nd, if inside SIMD CF
        instOpt = Get_Gen4_Emask(currEMask, exsize);
        instOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0; // setting channels for non-mad insts
        madmInstOpt = instOpt; // setting channels for mad insts

                               //1, 6, 8, 9, 10, 11
        G4_DstRegRegion tdst1(*this, Direct, t1->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst6(*this, Direct, t6->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst8(*this, Direct, t8->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1, Type_F);

        G4_SrcRegRegion tsrc1(*this, Mod_src_undef, Direct, t1->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc4(*this, Mod_src_undef, Direct, t4->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc6(*this, Mod_src_undef, Direct, t6->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, t8->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);

        G4_DstRegRegion *t8DstOpnd0 = createDstRegRegion(tdst8);
        G4_DstRegRegion *t9DstOpnd0 = createDstRegRegion(tdst9);
        G4_DstRegRegion *t10DstOpnd0 = createDstRegRegion(tdst10);
        G4_DstRegRegion *t1DstOpnd0 = createDstRegRegion(tdst1);
        G4_DstRegRegion *t11DstOpnd0 = createDstRegRegion(tdst11);
        G4_DstRegRegion *t9DstOpnd1 = createDstRegRegion(tdst9);
        G4_DstRegRegion *t6DstOpnd0 = createDstRegRegion(tdst6);
        G4_DstRegRegion *t8DstOpnd1 = createDstRegRegion(tdst8);

        // src oprands passed by function calls
        G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(), src0RR->getRegOff() + regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion fsrc1(*this, Mod_src_undef, Direct, src1RR->getBase(), src1RR->getRegOff() + regIndex, 0, srcRegionDesc, Type_F);

        G4_SrcRegRegion *t4SrcOpnd0 = NULL;
        G4_SrcRegRegion *t6SrcOpnd0 = NULL;
        G4_SrcRegRegion *t6SrcOpnd1 = NULL;
        G4_SrcRegRegion *t6SrcOpnd2 = NULL;
        G4_SrcRegRegion *t6SrcOpnd3 = NULL;

        G4_SrcRegRegion *t1SrcOpnd0 = createSrcRegRegion(tsrc1);
        G4_SrcRegRegion *t1SrcOpnd1 = createSrcRegRegion(tsrc1);
        G4_SrcRegRegion *t6SrcOpnd4 = createSrcRegRegion(tsrc6);
        G4_SrcRegRegion *t8SrcOpnd0x0 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x1 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x2 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t8SrcOpnd0x3 = createSrcRegRegion(tsrc8);
        G4_SrcRegRegion *t9SrcOpnd0x0 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd0x1 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd1x0 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd1x1 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t10SrcOpnd0 = createSrcRegRegion(tsrc10);
        G4_SrcRegRegion *t11SrcOpnd0 = createSrcRegRegion(tsrc11);

        // save CR, and then set rounding mode to RNE if hasDefaultRoundDenorm is false
        setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, regCR0);

        t6SrcOpnd0 = createSrcRegRegion(fsrc0);
        t6SrcOpnd1 = createSrcRegRegion(fsrc0);
        t6SrcOpnd2 = createSrcRegRegion(fsrc0);
        t6SrcOpnd3 = createSrcRegRegion(fsrc0);

        t4SrcOpnd0 = createSrcRegRegion(fsrc1);


        // create -r4.noacc
        G4_SrcRegRegion tsrc4_neg(
            *this,
            Mod_Minus,
            t4SrcOpnd0->getRegAccess(),
            t4SrcOpnd0->getBase(),
            t4SrcOpnd0->getRegOff(),
            t4SrcOpnd0->getSubRegOff(),
            t4SrcOpnd0->getRegion(),
            t4SrcOpnd0->getType());
        G4_SrcRegRegion *t4SrcOpndNeg0 = createSrcRegRegion(tsrc4_neg);
        G4_SrcRegRegion *t4SrcOpndNeg1 = createSrcRegRegion(tsrc4_neg);
        G4_SrcRegRegion *t4SrcOpndNeg2 = createSrcRegRegion(tsrc4_neg);

        // math.e0.f0.0 (8) r8.acc2 r6.noacc r4.noacc 0xe {Align16, Q1/Q2}
        t8DstOpnd0->setAccRegSel(ACC2);
        t6SrcOpnd0->setAccRegSel(NOACC);
        t4SrcOpnd0->setAccRegSel(NOACC);
        inst = createMathInst(nullptr, g4::NOSAT, exsize,
            t8DstOpnd0, t6SrcOpnd0,
            t4SrcOpnd0, MATH_INVM, madmInstOpt, true);
        G4_CondMod *condModOverflow = createCondMod(Mod_o, tmpFlag->getRegVar(), 0);
        inst->setCondMod(condModOverflow);

        if (isNoMask(emask))
        {
            G4_Declare *tmpFlag2 = createTempFlag(2);
            inst = createBinOp(G4_and, g4::SIMD1, createDstRegRegion(Direct, tmpFlag2->getRegVar(),
                0, 0, 1, Type_UD, ACC_UNDEFINED),
                createSrcRegRegion(tmpFlag, getRegionScalar()),
                createImm(0x1, Type_UD), InstOpt_WriteEnable, true);

            G4_Predicate *predicateFlagReg = createPredicate(PredState_Minus, tmpFlag2->getRegVar(),
                0, getPlatform() >= Xe_PVC ? PRED_DEFAULT : PRED_ANY32H);
            inst = createIf(predicateFlagReg, G4_ExecSize(32), instOpt);
        }
        else
        {
            // (-f0.1) if (8) k0__AUTO_GENERATED_IF_LABEL__0 k0__AUTO_GENERATED_ELSE_LABEL__1 {Q1/Q2}
            G4_Predicate *predicateFlagReg = createPredicate(PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
            inst = createIf(predicateFlagReg, exsize, instOpt);
        }

        // madm (8) r9.acc3 r2.noacc r6.noacc r8.acc2 {Align16, Q1/Q2}
        G4_SrcRegRegion *t2SrcOpnd = createSrcRegRegion(tsrc2);
        t9DstOpnd0->setAccRegSel(ACC3);
        t2SrcOpnd->setAccRegSel(NOACC);
        t6SrcOpnd1->setAccRegSel(NOACC);
        t8SrcOpnd0x0->setAccRegSel(ACC2);
        inst = createMadm(exsize,
            t9DstOpnd0, t2SrcOpnd,
            t6SrcOpnd1, t8SrcOpnd0x0, madmInstOpt);

        // madm (8) r10.acc4 r5.noacc -r4.noacc r8.acc2 {Align16, Q1/Q2}
        G4_SrcRegRegion *t5SrcOpnd0 = createSrcRegRegion(tsrc5);
        t10DstOpnd0->setAccRegSel(ACC4);
        t5SrcOpnd0->setAccRegSel(NOACC);
        t4SrcOpndNeg0->setAccRegSel(NOACC);
        t8SrcOpnd0x1->setAccRegSel(ACC2);
        inst = createMadm(exsize,
            t10DstOpnd0, t5SrcOpnd0,
            t4SrcOpndNeg0, t8SrcOpnd0x1, madmInstOpt);

        // madm (8) r1.acc5 r8.acc2 r10.acc4 r8.acc2 {Align16, Q1/Q2}
        t1DstOpnd0->setAccRegSel(ACC5);
        t8SrcOpnd0x2->setAccRegSel(ACC2);
        t10SrcOpnd0->setAccRegSel(ACC4);
        t8SrcOpnd0x3->setAccRegSel(ACC2);
        inst = createMadm(exsize,
            t1DstOpnd0, t8SrcOpnd0x2,
            t10SrcOpnd0, t8SrcOpnd0x3, madmInstOpt);


        // madm (8) r11.acc6 r6.noacc -r4.noacc r9.acc3 {Align16, Q1/Q2}
        t11DstOpnd0->setAccRegSel(ACC6);
        t6SrcOpnd2->setAccRegSel(NOACC);
        t4SrcOpndNeg1->setAccRegSel(NOACC);
        t9SrcOpnd0x0->setAccRegSel(ACC3);
        inst = createMadm(exsize,
            t11DstOpnd0, t6SrcOpnd2,
            t4SrcOpndNeg1, t9SrcOpnd0x0, madmInstOpt);


        // madm (8) r9.acc7 r9.acc3 r11.acc6 r1.acc5 {Align16, Q1/Q2}
        t9DstOpnd1->setAccRegSel(ACC7);
        t9SrcOpnd0x1->setAccRegSel(ACC3);
        t11SrcOpnd0->setAccRegSel(ACC6);
        t1SrcOpnd0->setAccRegSel(ACC5);
        inst = createMadm(exsize,
            t9DstOpnd1, t9SrcOpnd0x1,
            t11SrcOpnd0, t1SrcOpnd0, madmInstOpt);


        // madm (8) r6.acc8 r6.noacc -r4.noacc r9.acc7 {Align16, Q1/Q2}
        t6DstOpnd0->setAccRegSel(ACC8);
        t6SrcOpnd3->setAccRegSel(NOACC);
        t4SrcOpndNeg2->setAccRegSel(NOACC);
        t9SrcOpnd1x0->setAccRegSel(ACC7);
        inst = createMadm(exsize,
            t6DstOpnd0, t6SrcOpnd3,
            t4SrcOpndNeg2, t9SrcOpnd1x0, madmInstOpt);

        // restore Rounding Mode in CR if hasDefaultRoundDenorm is false
        restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);

        // madm (8) r8.noacc r9.acc7 r6.acc8 r1.acc5 {Align16, Q1/Q2}
        t8DstOpnd1->setAccRegSel(NOACC);
        t9SrcOpnd1x1->setAccRegSel(ACC7);
        t6SrcOpnd4->setAccRegSel(ACC8);
        t1SrcOpnd1->setAccRegSel(ACC5);
        inst = createMadm(exsize,
            t8DstOpnd1, t9SrcOpnd1x1,
            t6SrcOpnd4, t1SrcOpnd1, madmInstOpt);

        if (!hasDefaultRoundDenorm)
        {
            // else (8) {Q1/Q2}
            createElse(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);

            // restore Rounding Mode in CR
            restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);
        }

        // endif (8) {Q1/Q2}
        inst = createEndif(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);
    };

    // make final copy to dst
    // dst = r8:f  mov (instExecSize) r20.0<1>:f r110.0<8;8,1>:f {Q1/H1}
    t8_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
    inst = createInst(predOpnd, G4_mov, condMod, saturate, instExecSize, dstOpnd, t8_src_opnd_final,
        NULL, Get_Gen4_Emask(emask, instExecSize), true);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISAArithmeticSingleSQRTIEEEInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize,
    VISA_EMask_Ctrl emask, G4_Predicate *predOpnd,
    G4_Sat saturate, G4_CondMod* condMod, G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_INST* inst;
    unsigned madmInstOpt = 0;
    G4_ExecSize instExecSize = toExecSize(executionSize);
    G4_ExecSize element_size = g4::SIMD8; // element_size is dynamic, changed according to instExecSize
    const G4_ExecSize exsize = getNativeExecSize();
    unsigned int loopCount = 1;
    G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize); // for those insts of execution size of element_size
    const RegionDesc *srcRegionDesc = getRegionStride1();

    G4_Imm *flt_constant_0 = createImm(float(0.0));
    G4_Imm *flt_constant_05 = createImm(float(0.5));
    if (instExecSize <= exsize)
    {
        element_size = exsize;
        loopCount = 1;
    }
    else
    {
        element_size = instExecSize;
        instOpt = Get_Gen4_Emask(emask, instExecSize);
        loopCount = instExecSize / exsize;
    }

    // pred and conModifier
    G4_Declare *tmpFlag = createTempFlag(instExecSize == 32 ? 2 : 1);
    G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

    // temp registers
    G4_Declare *t6  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t7  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t9  = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_F, Any);
    G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_F, Any);

    // 0.0:f and 0.5:f constants
    G4_Declare* t0 = getImmDcl(flt_constant_0, exsize);
    G4_Declare *t8 = getImmDcl(flt_constant_05, exsize);

    inst = createPseudoKills ({ t6, t7, t9, t10, t11, tmpFlag }, PseudoKillType::Src);

    G4_SrcRegRegion* src0RR = operandToDirectSrcRegRegion(*this, src0Opnd, element_size, instExecSize);

    if (src0RR->isScalar() || src0RR->getModifier() != Mod_src_undef)
    {
        // expand src0 to vector src
        G4_DstRegRegion *t6_dst_src0_opnd = createDstRegRegion(t6, 1);
        inst = createMov(element_size, t6_dst_src0_opnd, src0RR,
            instOpt, true); // mov (element_size) t6, src0RR {Q1/H1}

        src0RR = createSrcRegRegion(t6, getRegionStride1());
    }

    G4_SrcRegRegion tsrc0(*this, Mod_src_undef, Direct, t0->getRegVar(), 0, 0, srcRegionDesc, Type_F);
    G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, t8->getRegVar(), 0, 0, srcRegionDesc, Type_F);


    G4_SrcRegRegion tsrc7_final(*this, Mod_src_undef, Direct, t7->getRegVar(), 0, 0,
        getRegionStride1(), t7->getElemType());
    G4_SrcRegRegion *t7_src_opnd_final = createSrcRegRegion(tsrc7_final);

    bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
    // cr0.0 register
    G4_Declare* regCR0 = createTempVarWithNoSpill(1, Type_UD, Any);

    VISA_EMask_Ctrl currEMask = emask;
    uint16_t splitInstGRFSize = (uint16_t)((TypeSize(Type_F) * exsize + getGRFSize() - 1) / getGRFSize());
    for (uint16_t loopIndex = 0; currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
        ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize))
    {
        uint16_t regIndex = splitInstGRFSize * loopIndex;
        instOpt = Get_Gen4_Emask(currEMask, exsize);
        instOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0;
        madmInstOpt = instOpt;

        //7, 9, 10, 11
        G4_DstRegRegion tdst7(*this, Direct, t7->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1, Type_F);
        G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1, Type_F);

        G4_SrcRegRegion tsrc7(*this, Mod_src_undef, Direct, t7->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);
        G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(), regIndex, 0, srcRegionDesc, Type_F);

        G4_DstRegRegion *t7DstOpnd0 = createDstRegRegion(tdst7);
        G4_DstRegRegion *t7DstOpnd1 = createDstRegRegion(tdst7);
        G4_DstRegRegion *t7DstOpnd2 = createDstRegRegion(tdst7);
        G4_DstRegRegion *t9DstOpnd0 = createDstRegRegion(tdst9);
        G4_DstRegRegion *t9DstOpnd1 = createDstRegRegion(tdst9);
        G4_DstRegRegion *t10DstOpnd0 = createDstRegRegion(tdst10);
        G4_DstRegRegion *t10DstOpnd1 = createDstRegRegion(tdst10);
        G4_DstRegRegion *t11DstOpnd0 = createDstRegRegion(tdst11);

        // src oprands passed by function calls
        G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(), src0RR->getRegOff() + regIndex, 0, srcRegionDesc, Type_F);

        G4_SrcRegRegion *t6SrcOpnd0 = NULL;
        G4_SrcRegRegion *t6SrcOpnd1 = NULL;
        G4_SrcRegRegion *t6SrcOpnd2 = NULL;

        G4_SrcRegRegion *t7SrcOpnd0 = createSrcRegRegion(tsrc7);
        G4_SrcRegRegion *t7SrcOpnd1 = createSrcRegRegion(tsrc7);
        G4_SrcRegRegion *t7SrcOpnd2x0 = createSrcRegRegion(tsrc7);
        G4_SrcRegRegion *t7SrcOpnd2x1 = createSrcRegRegion(tsrc7);
        G4_SrcRegRegion *t7SrcOpnd3 = createSrcRegRegion(tsrc7);

        G4_SrcRegRegion *t9SrcOpnd0 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd1x0 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd1x1 = createSrcRegRegion(tsrc9);
        G4_SrcRegRegion *t9SrcOpnd2 = createSrcRegRegion(tsrc9);

        G4_SrcRegRegion *t10SrcOpnd0 = createSrcRegRegion(tsrc10);
        G4_SrcRegRegion *t10SrcOpnd1 = createSrcRegRegion(tsrc10);
        G4_SrcRegRegion *t10SrcOpnd2 = createSrcRegRegion(tsrc10);

        G4_SrcRegRegion *t11SrcOpnd0 = createSrcRegRegion(tsrc11);
        G4_SrcRegRegion *t11SrcOpnd1x0 = createSrcRegRegion(tsrc11);
        G4_SrcRegRegion *t11SrcOpnd1x1 = createSrcRegRegion(tsrc11);

        // save CR, and then set rounding mode to RNE if hasDefaultRoundDenorm is false
        setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, regCR0);

        t6SrcOpnd0 = createSrcRegRegion(fsrc0);
        t6SrcOpnd1 = createSrcRegRegion(fsrc0);
        t6SrcOpnd2 = createSrcRegRegion(fsrc0);

        //math.eo.f0.0 (8) r7.acc2 r6.noacc null 0xF {Aligned16, Q1/Q2}
        t7DstOpnd0->setAccRegSel(ACC2);
        t6SrcOpnd0->setAccRegSel(NOACC);

        G4_SrcRegRegion *null_src_opnd = createNullSrc(Type_F);

        inst = createMathInst(NULL, g4::NOSAT, exsize, t7DstOpnd0, t6SrcOpnd0,
            null_src_opnd, MATH_RSQRTM, madmInstOpt, true);
        G4_CondMod *condModOverflow = createCondMod(Mod_o, tmpFlag->getRegVar(), 0);
        inst->setCondMod(condModOverflow);

        if (isNoMask(emask))
        {
            G4_Declare *tmpFlag2 = createTempFlag(2);
            inst = createBinOp(G4_and, g4::SIMD1, createDstRegRegion(Direct, tmpFlag2->getRegVar(),
                0, 0, 1, Type_UD, ACC_UNDEFINED),
                createSrcRegRegion(tmpFlag, getRegionScalar()),
                createImm(0x1, Type_UD), InstOpt_WriteEnable, true);

            G4_Predicate *predicateFlagReg = createPredicate(PredState_Minus, tmpFlag2->getRegVar(),
                0, getPlatform() >= Xe_PVC ? PRED_DEFAULT : PRED_ANY32H);
            inst = createIf(predicateFlagReg, G4_ExecSize(32), instOpt);
        }
        else
        {
            // (-f1.0) if (8) k0__AUTO_GENERATED_IF_LABEL__0 k0__AUTO_GENERATED_IF_LABEL__0 {Q1/Q2}
            G4_Predicate *predicateFlagReg = createPredicate(PredState_Minus, tmpFlag->getRegVar(), 0, predCtrlValue);
            inst = createIf(predicateFlagReg, exsize, instOpt);
        }

        //madm (8) r9.acc3 r0.noacc r8.noacc r7.acc2 {Aligned16, Q1/Q2}
        G4_SrcRegRegion *t0SrcOpnd0 = createSrcRegRegion(tsrc0);
        G4_SrcRegRegion *t8SrcOpnd0 = createSrcRegRegion(tsrc8);
        t9DstOpnd0->setAccRegSel(ACC3);
        t0SrcOpnd0->setAccRegSel(NOACC);
        t8SrcOpnd0->setAccRegSel(NOACC);
        t7SrcOpnd0->setAccRegSel(ACC2);
        inst = createMadm( exsize, t9DstOpnd0, t0SrcOpnd0,
            t8SrcOpnd0, t7SrcOpnd0, madmInstOpt);


        //madm (8) r11.acc4 r0.noacc r6.noacc r7.acc2 {Aligned16, Q1/Q2}
        G4_SrcRegRegion *t0SrcOpnd1 = createSrcRegRegion(tsrc0);
        t11DstOpnd0->setAccRegSel(ACC4);
        t0SrcOpnd1->setAccRegSel(NOACC);
        t6SrcOpnd1->setAccRegSel(NOACC);
        t7SrcOpnd1->setAccRegSel(ACC2);
        inst = createMadm(exsize, t11DstOpnd0, t0SrcOpnd1,
            t6SrcOpnd1, t7SrcOpnd1, madmInstOpt);


        // create -r11.noacc
        G4_SrcRegRegion tsrc11_neg(
            *this,
            Mod_Minus,
            t11SrcOpnd0->getRegAccess(),
            t11SrcOpnd0->getBase(),
            t11SrcOpnd0->getRegOff(),
            t11SrcOpnd0->getSubRegOff(),
            t11SrcOpnd0->getRegion(),
            t11SrcOpnd0->getType());
        G4_SrcRegRegion *t11SrcOpndNeg0 = createSrcRegRegion(tsrc11_neg);
        //madm (8) r10.acc5 r8.noacc -r11.acc4 r9.acc3 {Aligned16, Q1/Q2}
        G4_SrcRegRegion *t8SrcOpnd1 = createSrcRegRegion(tsrc8);
        t10DstOpnd0->setAccRegSel(ACC5);
        t8SrcOpnd1->setAccRegSel(NOACC);
        t11SrcOpndNeg0->setAccRegSel(ACC4);
        t9SrcOpnd0->setAccRegSel(ACC3);
        inst = createMadm(exsize, t10DstOpnd0, t8SrcOpnd1,
            t11SrcOpndNeg0, t9SrcOpnd0, madmInstOpt);


        //madm (8) r9.acc6 r9.acc3 r10.acc5 r9.acc3 {Aligned16, Q1/Q2}
        t9DstOpnd1->setAccRegSel(ACC6);
        t9SrcOpnd1x0->setAccRegSel(ACC3);
        t10SrcOpnd0->setAccRegSel(ACC5);
        t9SrcOpnd1x1->setAccRegSel(ACC3);
        inst = createMadm( exsize, t9DstOpnd1, t9SrcOpnd1x0,
            t10SrcOpnd0, t9SrcOpnd1x1, madmInstOpt);


        //madm (8) r7.acc7 r11.acc4 r10.acc5 r11.acc4 {Aligned16, Q1/Q2}
        t7DstOpnd1->setAccRegSel(ACC7);
        t11SrcOpnd1x0->setAccRegSel(ACC4);
        t10SrcOpnd1->setAccRegSel(ACC5);
        t11SrcOpnd1x1->setAccRegSel(ACC4);
        inst = createMadm(exsize, t7DstOpnd1, t11SrcOpnd1x0,
            t10SrcOpnd1, t11SrcOpnd1x1, madmInstOpt);


        // create -r7.noacc
        G4_SrcRegRegion tsrc7_neg(
            *this,
            Mod_Minus,
            t7SrcOpnd2x0->getRegAccess(),
            t7SrcOpnd2x0->getBase(),
            t7SrcOpnd2x0->getRegOff(),
            t7SrcOpnd2x0->getSubRegOff(),
            t7SrcOpnd2x0->getRegion(),
            t7SrcOpnd2x0->getType());
        G4_SrcRegRegion *t7SrcOpndNeg0 = createSrcRegRegion(tsrc7_neg);
        //madm (8) r10.acc8 r6.noacc -r7.acc7 r7.acc7 {Aligned16, Q1/Q2}
        t10DstOpnd1->setAccRegSel(ACC8);
        t6SrcOpnd2->setAccRegSel(NOACC);
        t7SrcOpndNeg0->setAccRegSel(ACC7);
        t7SrcOpnd2x1->setAccRegSel(ACC7);
        inst = createMadm( exsize, t10DstOpnd1, t6SrcOpnd2,
            t7SrcOpndNeg0, t7SrcOpnd2x1, madmInstOpt);

        // restore Rounding Mode in CR if hasDefaultRoundDenorm is false
        restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);

        //madm (8) r7.noacc r7.acc7 r9.acc6 r10.acc8 {Aligned16, Q1/Q2}
        t7DstOpnd2->setAccRegSel(NOACC);
        t7SrcOpnd3->setAccRegSel(ACC7);
        t9SrcOpnd2->setAccRegSel(ACC6);
        t10SrcOpnd2->setAccRegSel(ACC8);
        inst = createMadm(exsize, t7DstOpnd2, t7SrcOpnd3,
            t9SrcOpnd2, t10SrcOpnd2, madmInstOpt);

        if (!hasDefaultRoundDenorm)
        {
            // else (8) {Q1/Q2}
            createElse(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);

            // restore Rounding Mode in CR
            restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);
        }

        // endif (exsize) {Q1/Q2}
        inst = createEndif(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);
    };

    // make final copy to dst
    // dst = r8:df   mov (instExecSize) r86.0<1>:f r8.0<8;8,1>:f {Q1/H1}
    t7_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
    inst = createInst(predOpnd, G4_mov, condMod, saturate, instExecSize, dstOpnd, t7_src_opnd_final,
        NULL, Get_Gen4_Emask(emask, instExecSize), true);

    return VISA_SUCCESS;
}


int IR_Builder::translateVISAArithmeticDoubleSQRTInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize,
    VISA_EMask_Ctrl emask, G4_Predicate *predOpnd,
    G4_Sat saturate, G4_CondMod* condMod,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_INST* inst = nullptr;
    G4_ExecSize instExecSize = toExecSize(executionSize);
    G4_ExecSize exsize = (G4_ExecSize)(getNativeExecSize() / 2);

    const RegionDesc *srcRegionDesc = getRegionStride1();
    const RegionDesc *rdAlign16 = getRegionStride1();
    unsigned int loopCount = 1;
    G4_ExecSize element_size = exsize;   // element_size is set according to instExecSize

    G4_DstRegRegion *dst0 = nullptr;
    G4_SrcRegRegion *src0 = nullptr;
    G4_SrcRegRegion *src1 = nullptr;
    G4_SrcRegRegion *src2 = nullptr;
    G4_SrcRegRegion *neg_src1 = nullptr;

    {
        if (instExecSize > exsize)
        {
            element_size = instExecSize;
            exsize = std::min(instExecSize, G4_ExecSize(getFP64MadmExecSize()));
            loopCount = instExecSize / exsize;
        }
    }

    bool noDstMove = exsize == 8 && !saturate && !predOpnd && isOpndAligned(dstOpnd, getGRFSize()) &&
        dstOpnd->getRegAccess() == Direct && dstOpnd->getHorzStride() == 1 && instExecSize == exsize;
    if (noDstMove && dstOpnd->getTopDcl() == src0Opnd->getTopDcl())
    {
        noDstMove = false;
    }

    unsigned int instOpt = Get_Gen4_Emask(emask, exsize);

    // pred and conModifier
    G4_Declare *flagReg = createTempFlag(instExecSize == 32 ? 2 : 1);
    G4_Predicate_Control predCtrlValue = PRED_DEFAULT;

    // temp registers
    G4_Declare *t0 = getImmDcl(createDFImm(0.0), element_size);
    G4_Declare *t1 = getImmDcl(createDFImm(1.0), element_size);
    G4_Declare *t2 = getImmDcl(createDFImm(0.5), element_size);
    G4_Declare* t3 = getImmDcl(createDFImm(1.5), element_size);
    G4_Declare *t6  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t7  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t8  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t9  = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t10 = createTempVarWithNoSpill(element_size, Type_DF, Any);
    G4_Declare *t11 = createTempVarWithNoSpill(element_size, Type_DF, Any);

    inst = createPseudoKills({t6, t7, t8, t9, t10, t11, flagReg }, PseudoKillType::Src);

    G4_SrcRegRegion* src0RR = operandToDirectSrcRegRegion(*this, src0Opnd, element_size, instExecSize);

    bool IsSrc0Moved = src0RR->getRegion()->isScalar() || src0RR->getModifier() != Mod_src_undef;
    if (IsSrc0Moved)
    {
        // expand scale src0 to vector src
        dst0 = createDstRegRegion(t6, 1);
        // mov (element_size) t6_dst_src0_opnd, src0RR {Q1/H1}
        inst = createMov(element_size, dst0, src0RR, instOpt, true);
    }

    // constants

    // r0 = 0.0:df, r1 = 1.0:df, r2(r8) = 0.5:df, r3 = 1.5:df
    // NOTE: 'NoMask' is required as constants are required for splitting
    // parts. Once they are in diverged branches, it won't be properly
    // initialized without 'NoMask'.

    // one GRF
    G4_SrcRegRegion csrc0(*this, Mod_src_undef, Direct, t0->getRegVar(), 0, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion csrc1(*this, Mod_src_undef, Direct, t1->getRegVar(), 0, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion csrc2(*this, Mod_src_undef, Direct, t2->getRegVar(), 0, 0, srcRegionDesc, Type_DF);
    G4_SrcRegRegion csrc3(*this, Mod_src_undef, Direct, t3->getRegVar(), 0, 0, srcRegionDesc, Type_DF);

    bool hasDefaultRoundDenorm = getOption(vISA_hasRNEandDenorm);
    // cr0.0 register
    G4_Declare* regCR0 = createTempVarWithNoSpill(1, Type_UD, Any);

    // each madm only handles 4 channel double data
    VISA_EMask_Ctrl currEMask = emask;
    uint16_t splitInstGRFSize = (uint16_t)((TypeSize(Type_DF) * exsize + getGRFSize() - 1) / getGRFSize());
    for (uint16_t loopIndex = 0; currEMask != vISA_NUM_EMASK && loopIndex < loopCount;
        ++loopIndex, currEMask = Get_Next_EMask(currEMask, exsize))
    {
        uint16_t regIndex = splitInstGRFSize * loopIndex;
        instOpt = Get_Gen4_Emask(currEMask, exsize);
        instOpt |= isNoMask(emask) ? InstOpt_WriteEnable : 0; // setting channels for non-mad insts
        unsigned int madmInstOpt = instOpt; // setting channels for mad insts

                                            // dst : 7, 8, 9, 10 11
        G4_DstRegRegion tdst7(*this, Direct, noDstMove ? dstOpnd->getBase() : t7->getRegVar(),
            noDstMove ? dstOpnd->getRegOff() + regIndex : regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst8(*this, Direct, t8->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst9(*this, Direct, t9->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst10(*this, Direct, t10->getRegVar(), regIndex, 0, 1, Type_DF);
        G4_DstRegRegion tdst11(*this, Direct, t11->getRegVar(), regIndex, 0, 1, Type_DF);

        // source of inst.
        G4_SrcRegRegion fsrc0_math(*this, Mod_src_undef, Direct, src0RR->getBase(), src0RR->getRegOff() + regIndex, 0, rdAlign16, Type_DF);


        // src : 6, 7, 8, 9, 10, 11
        G4_SrcRegRegion fsrc0(*this, Mod_src_undef, Direct, src0RR->getBase(), src0RR->getRegOff() + regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc6(*this, Mod_src_undef, Direct, t6->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc7(*this, Mod_src_undef, Direct, noDstMove ? dstOpnd->getBase() : t7->getRegVar(),
            noDstMove ? dstOpnd->getRegOff() + regIndex : regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc8(*this, Mod_src_undef, Direct, t8->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc9(*this, Mod_src_undef, Direct, t9->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc10(*this, Mod_src_undef, Direct, t10->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);
        G4_SrcRegRegion tsrc11(*this, Mod_src_undef, Direct, t11->getRegVar(), regIndex, 0, srcRegionDesc, Type_DF);

        // save CR, and then set rounding mode to RNE if hasDefaultRoundDenorm is false
        setDefaultRoundDenorm(*this, hasDefaultRoundDenorm, regCR0);

        // math.e0.f0.0 (4) r7.acc2 r6.noacc NULL 0xf {Align16, N1/N2}
        dst0 = createDstRegRegion(tdst7); dst0->setAccRegSel(ACC2);
        if (IsSrc0Moved)
        {
            // use t6, but need to adjust the offset since unlike the orig source t6 is zero-based.
            src0 = createSrc(t6->getRegVar(), regIndex, 0, rdAlign16, Type_DF);
        }
        else
        {
            src0 = createSrcRegRegion(fsrc0_math);
        }
        src0->setAccRegSel(NOACC);
        src1 = createNullSrc(Type_DF);
        G4_CondMod* condModOverflow = createCondMod(Mod_o, flagReg->getRegVar(), 0);
        inst = createMathInst(NULL, g4::NOSAT, exsize, dst0, src0, src1, MATH_RSQRTM, madmInstOpt, true);
        inst->setCondMod(condModOverflow);

        bool generateIf = true;

        if (generateIf)
        {
            // if
            if (isNoMask(emask))
            {
                G4_Declare* tmpFlag = createTempFlag(2);
                inst = createBinOp(G4_and, g4::SIMD1, createDstRegRegion(Direct, tmpFlag->getRegVar(),
                    0, 0, 1, Type_UD, ACC_UNDEFINED),
                    createSrcRegRegion(flagReg, getRegionScalar()),
                    createImm(0x1, Type_UD), InstOpt_WriteEnable, true);

                G4_Predicate* predicateFlagReg = createPredicate(PredState_Minus, tmpFlag->getRegVar(),
                    0, getPlatform() >= Xe_PVC ? PRED_DEFAULT : PRED_ANY32H);
                inst = createIf(predicateFlagReg, G4_ExecSize(32), instOpt);
            }
            else
            {
                G4_Predicate* predicateFlagReg = createPredicate(PredState_Minus, flagReg->getRegVar(), 0, predCtrlValue);
                inst = createIf(predicateFlagReg, exsize, instOpt);
            }
        }

        {
            // madm (4) r9.acc3 r0.noacc r2(r8).noacc r7.acc2 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst9); dst0->setAccRegSel(ACC3);
            src0 = createSrcRegRegion(csrc0); src0->setAccRegSel(NOACC);
            src1 = createSrcRegRegion(csrc2); src1->setAccRegSel(NOACC);
            src2 = createSrcRegRegion(tsrc7); src2->setAccRegSel(ACC2);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r11.acc4 r0.noacc r6.noacc r7.acc2 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst11); dst0->setAccRegSel(ACC4);
            src0 = createSrcRegRegion(csrc0); src0->setAccRegSel(NOACC);
            if (IsSrc0Moved)
            {
                src1 = createSrcRegRegion(tsrc6);
            }
            else
            {
                src1 = createSrcRegRegion(fsrc0);
            }
            src1->setAccRegSel(NOACC);
            src2 = createSrcRegRegion(tsrc7); src2->setAccRegSel(ACC2);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r10.acc5 r2(r8).noacc -r11.acc4 r9.acc3 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst10); dst0->setAccRegSel(ACC5);
            src0 = createSrcRegRegion(csrc2); src0->setAccRegSel(NOACC);
            src1 = createSrcRegRegion(tsrc11); src1->setAccRegSel(ACC4);
            src2 = createSrcRegRegion(tsrc9); src2->setAccRegSel(ACC3);
            G4_SrcRegRegion neg_srcRegion(
                *this,
                Mod_Minus,
                src1->getRegAccess(),
                src1->getBase(),
                src1->getRegOff(),
                src1->getSubRegOff(),
                src1->getRegion(),
                src1->getType());
            neg_src1 = createSrcRegRegion(neg_srcRegion);
            neg_src1->setAccRegSel(src1->getAccRegSel());
            inst = createMadm(exsize,
                dst0, src0, neg_src1, src2, madmInstOpt);

            // madm (4) r8.acc7 r1.noacc r3.noacc r10.acc5 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst8); dst0->setAccRegSel(ACC7);
            src0 = createSrcRegRegion(csrc1); src0->setAccRegSel(NOACC);
            src1 = createSrcRegRegion(csrc3); src1->setAccRegSel(NOACC);
            src2 = createSrcRegRegion(tsrc10); src2->setAccRegSel(ACC5);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r7.acc8 r0.noacc r10.acc5 r11.acc4 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst7); dst0->setAccRegSel(ACC8);
            src0 = createSrcRegRegion(csrc0); src0->setAccRegSel(NOACC);
            src1 = createSrcRegRegion(tsrc10); src1->setAccRegSel(ACC5);
            src2 = createSrcRegRegion(tsrc11); src2->setAccRegSel(ACC4);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r10.acc9 r0.noacc r10.acc5 r9.acc3 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst10); dst0->setAccRegSel(ACC9);
            src0 = createSrcRegRegion(csrc0); src0->setAccRegSel(NOACC);
            src1 = createSrcRegRegion(tsrc10); src1->setAccRegSel(ACC5);
            src2 = createSrcRegRegion(tsrc9); src2->setAccRegSel(ACC3);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r7.acc8 r11.acc4 r8.acc7 r7.acc8 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst7); dst0->setAccRegSel(ACC8);
            src0 = createSrcRegRegion(tsrc11); src0->setAccRegSel(ACC4);
            src1 = createSrcRegRegion(tsrc8); src1->setAccRegSel(ACC7);
            src2 = createSrcRegRegion(tsrc7); src2->setAccRegSel(ACC8);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r8.acc7 r9.acc3 r8.acc7 r10.acc9 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst8); dst0->setAccRegSel(ACC7);
            src0 = createSrcRegRegion(tsrc9); src0->setAccRegSel(ACC3);
            src1 = createSrcRegRegion(tsrc8); src1->setAccRegSel(ACC7);
            src2 = createSrcRegRegion(tsrc10); src2->setAccRegSel(ACC9);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

            // madm (4) r9.acc3 r6.noacc -r7.acc8 r7.acc8 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst9); dst0->setAccRegSel(ACC3);
            if (IsSrc0Moved)
            {
                src0 = createSrcRegRegion(tsrc6);
            }
            else
            {
                src0 = createSrcRegRegion(fsrc0);
            }
            src0->setAccRegSel(NOACC);
            src1 = createSrcRegRegion(tsrc7); src1->setAccRegSel(ACC8);
            src2 = createSrcRegRegion(tsrc7); src2->setAccRegSel(ACC8);
            G4_SrcRegRegion neg_srcRegion1(
                *this,
                Mod_Minus,
                src1->getRegAccess(),
                src1->getBase(),
                src1->getRegOff(),
                src1->getSubRegOff(),
                src1->getRegion(),
                src1->getType());
            neg_src1 = createSrcRegRegion(neg_srcRegion1);
            neg_src1->setAccRegSel(src1->getAccRegSel());
            inst = createMadm(exsize,
                dst0, src0, neg_src1, src2, madmInstOpt);

            // restore Rounding Mode in CR if hasDefaultRoundDenorm is false
            restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);

            // madm (4) r7.noacc r7.acc8 r9.acc3 r8.acc7 {Align16, N1/N2}
            dst0 = createDstRegRegion(tdst7); dst0->setAccRegSel(NOACC);
            src0 = createSrcRegRegion(tsrc7); src0->setAccRegSel(ACC8);
            src1 = createSrcRegRegion(tsrc9); src1->setAccRegSel(ACC3);
            src2 = createSrcRegRegion(tsrc8); src2->setAccRegSel(ACC7);
            inst = createMadm(exsize,
                dst0, src0, src1, src2, madmInstOpt);

        }

        if (generateIf)
        {
            if (!hasDefaultRoundDenorm)
            {
                // else (8) {Q1/Q2}
                createElse(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);

                // restore Rounding Mode in CR
                restoreCR0_0(*this, hasDefaultRoundDenorm, regCR0);
            }

            // endif (8) {Q1/Q2}
            inst = createEndif(isNoMask(emask) ? G4_ExecSize(32) : exsize, instOpt);
        }
    };

    if (!noDstMove)
    {
        // make final copy to dst
        // src = r7:df
        // final result is at r7.noacc
        G4_SrcRegRegion tsrc7_final(*this, Mod_src_undef, Direct, t7->getRegVar(), 0, 0, getRegionStride1(), t7->getElemType());
        G4_SrcRegRegion *t7_src_opnd_final = createSrcRegRegion(tsrc7_final); t7_src_opnd_final->setAccRegSel(ACC_UNDEFINED);
        // mov (instExecSize) r20.0<1>:df r7.0<8;8,1>:df {Q1/H1}
        inst = createInst(
            predOpnd, G4_mov, condMod, saturate, instExecSize,
            dstOpnd, t7_src_opnd_final, nullptr, Get_Gen4_Emask(emask, instExecSize), true);
    }

    return VISA_SUCCESS;
}

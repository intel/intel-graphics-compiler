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

#include <cmath>

#include "HWConformity.h"
#include "Optimizer.h"
#include "visa_wa.h"
#include "DebugInfo.h"
#include "G4Verifier.h"

using namespace vISA;

static G4_CondModifier getReverseCondMod(G4_CondModifier mod)
{
    switch (mod)
    {
    case Mod_z:
        return Mod_z;
    case Mod_e:
        return Mod_e;
    case Mod_nz:
        return Mod_nz;
    case Mod_ne:
        return Mod_ne;
    case Mod_g:
        return Mod_l;
    case Mod_ge:
        return Mod_le;
    case Mod_l:
        return Mod_g;
    case Mod_le:
        return Mod_ge;
    default:
        MUST_BE_TRUE(0, "Invalid conditional modifier input for reversed conditional modifier.");
    }
    return Mod_cond_undef;
}

static bool isCompressedInst(G4_INST* inst) {
    return inst->isComprInst();
}

#define isUnitRegionRow( opnd, exec_size )      \
        ( opnd->isImm() ||      \
        opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getRegion()->width == exec_size || \
        opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getRegion()->vertStride == 0 )

G4_SubReg_Align HWConformity::getDclAlignment(int opndBytes, G4_INST* inst, bool isScalar)
{
    auto subAlign = Get_G4_SubRegAlign_From_Size((uint16_t)opndBytes);
    bool hasAccSrc = inst->hasACCSrc();

    if (hasAccSrc && subAlign < GRFALIGN)
    {
        subAlign = GRFALIGN;
    }

    if (!isScalar)
    {
        // certain instructions have additional alignment requirements for non-scalar sources, FIXME: what about 64 bytes?
        if (!builder.hasAlign1Ternary() && inst->getNumSrc() == 3 && !inst->isSend() && subAlign < Eight_Word)
        {
            subAlign = Eight_Word;
        }
        if (inst->isMath())  //FIXME: need confirm, used to be GRFALIGN
        {
            subAlign = GRFALIGN;
        }
    }

    return subAlign;
}
/*
 *  create a new mov instruction and insert it before iter
 *  mov (esize) dst tmp:type
 *  where esize is "inst"'s execution size and insert it after "inst"
 *  return value is the new temp variable as a dst operand
 *  If dstAlign is specified, the new temp will at least be aligend to that size
 */
G4_DstRegRegion* HWConformity::insertMovAfter(INST_LIST_ITER& it, G4_DstRegRegion* dst, G4_Type type, G4_BB* bb, G4_SubReg_Align dstAlign)
{
    G4_INST* inst = *it;

    if (!dst)
    {
        return dst;
    }

    if (inst->hasNULLDst())
    {
        return builder.createDst(
            dst->getBase(),
            0,
            0,
            1,
            type);
    }

    INST_LIST_ITER iter = it;
    iter++;
    unsigned char exec_size = inst->getExecSize();
    G4_Type execType = inst->isRawMov() ? dst->getType() : inst->getExecType();
    bool scalarSrc = true;

    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++)
    {
        G4_Operand* src = inst->getSrc(i);
        if (!src->isImm())
        {
            if (!(inst->isMath() && i == 1 && src->isNullReg()) &&
                (src->isSrcRegRegion() && !src->asSrcRegRegion()->isScalar()))
            {
                scalarSrc = false;
            }
        }
        else if (IS_VINTTYPE(src->getType()) || IS_VFTYPE(src->getType()))
        {
            scalarSrc = false;
        }
    }

    uint8_t newExecSize = ((inst->opcode() == G4_sel || inst->getImplAccSrc() || !scalarSrc) ? exec_size : 1);

    uint32_t opExecWidthBytes = newExecSize * G4_Type_Table[execType].byteSize;
    if (execType == Type_DF && IS_BTYPE(type))
    {
        type = (type == Type_UB ? Type_UW : Type_W);
    }
    uint16_t dstWidthBytes = newExecSize * G4_Type_Table[type].byteSize;
    uint16_t scale = G4_Type_Table[execType].byteSize / G4_Type_Table[type].byteSize;
    /*   so according to comments in function that call it MAD needs to have packed format.
        It ends up with hStride 2, due to DefHoisting.
        So it is trying to undo it.
        For every other type if srcType > dstCype we need to adjust regions.
        This is not necessary for HF. It's already packed.

        The src region of move is wrong. Since for HF it is packed, unlike other data types.
        mad (8) r56.0.xyzw:hf -r37.0.xyzw:f r59.0.xyzw:hf r58.0.xyzw:hf {Align16, NoMask}
        mov (16) r44.0<2>:hf r56.0<16;8,2>:hf {Align1, H1} // #??:$39:%66
    */
    if (scale == 0 || (builder.getPlatform() >= GENX_CHV && execType == Type_F && type == builder.getMixModeType()))
    {
        scale = 1;
    }

    G4_SubReg_Align subAlign = getDclAlignment(opExecWidthBytes > dstWidthBytes ? opExecWidthBytes : dstWidthBytes,
        inst, newExecSize == 1);

    if (subAlign < dstAlign)
    {
        subAlign = dstAlign;
    }

    const RegionDesc* region = newExecSize > 1 ? builder.createRegionDesc(scale, 1, 0) : builder.getRegionScalar();

    G4_Declare* dcl = builder.createTempVar(newExecSize == 1 ? 1 : newExecSize * scale, type, subAlign);

    G4_SrcRegRegion* srcRegion = builder.Create_Src_Opnd_From_Dcl(dcl, region);
    G4_Predicate* pred = NULL;

    if (inst->opcode() != G4_sel)
    {
        pred = inst->getPredicate();
        inst->setPredicate(NULL);
        // maintainDU4TempMov will update def-use
    }

    unsigned int new_option = inst->getMaskOption();
    G4_INST* newInst = builder.createMov(exec_size, dst, srcRegion, new_option, false);
    newInst->setPredicate(pred);
    newInst->setSaturate(inst->getSaturate());
    bb->insert(iter, newInst);

    // update propagation info
    maintainDU4TempMov(inst, newInst);

    if (type == dst->getType())
    {
        newInst->setSaturate(false);
    }
    else if (type == Type_F || type == Type_DF)
    {
        inst->setSaturate(false);
    }

    inst->setExecSize(newExecSize);
    if (newExecSize == 1)
    {
        inst->setNoMask(true);
    }

    return builder.Create_Dst_Opnd_From_Dcl(dcl, scale);
}

//
// replace instruction (*it)' source srcPos, which must be a scalar/immediate,
// with a temp variable after inserting
// mov (esize) tmp<1>:type imm/scalar {options}
// before the instruction
// This is like insertMovBefore(), except that the latter will always use
// simd1 move for scalar/imm values, which may not be what we want
// NOTE: This does not check for redundant moves.  We are counting on a later LVN pass
// to clean them up
//
void HWConformity::broadcast(
    G4_BB* bb, INST_LIST_ITER it, int srcPos, G4_SubReg_Align align)
{
    G4_INST* inst = *it;
    G4_Operand* src = inst->getSrc(srcPos);
    MUST_BE_TRUE(src->isImm() ||
        (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar()),
        "source must be an immediate or scalar");
    G4_Type type = src->getType();

    uint8_t execSize = inst->getExecSize();
    uint32_t instMask = inst->getMaskOption();

    // avoid simd16 Qword moves
    MUST_BE_TRUE(execSize * G4_Type_Table[type].byteSize <= 2u * GENX_GRF_REG_SIZ,
        "move can't exceed 2 GRFs");

    G4_Declare* dcl = builder.createTempVar(execSize, type, align);
    G4_DstRegRegion* dst = builder.createDst(
        dcl->getRegVar(),
        0,
        0,
        1,
        type);
    G4_INST* newInst = builder.createMov(execSize, dst, src, instMask, false);

    bb->insert(it, newInst);

    const RegionDesc* srcRegion = builder.getRegionStride1();
    G4_SrcRegRegion* newSrc = builder.Create_Src_Opnd_From_Dcl(dcl, srcRegion);
    inst->setSrc(newSrc, srcPos);
    newInst->addDefUse(inst, inst->getSrcOperandNum(srcPos));

}

//
// A simplified version of insertMovBefore(), this copies raw bytes from source to a temp
// and replaces the original source with tmp.  This is primarily used to ensure operand alignment and region restrictions
// op (esize) ... (mod) src<region>:type
// -->
// mov (esize) tmp<1>:type src<region>:type
// op (esize) ... (mod) tmp<1;1,0>:type
//
// source must be a G4_SrcRegRegion (direct or indirect), immediates are not supported
// note that modifier is propagated from source to tmp, but region is not
//
//
G4_SrcRegRegion* HWConformity::insertCopyBefore(INST_LIST_ITER it, uint32_t srcNum,
    G4_SubReg_Align tmpAlign, G4_BB* bb)
{
    G4_INST* inst = *it;
    G4_Operand* src = inst->getSrc(srcNum);
    MUST_BE_TRUE(src != nullptr && src->isSrcRegRegion(), "source must be a SrcRegRegion");
    G4_SrcRegRegion* origSrc = src->asSrcRegRegion();

    uint8_t newExecSize = origSrc->isScalar() ? 1 : inst->getExecSize();
    G4_Declare* dcl = builder.createTempVar(newExecSize, origSrc->getType(), tmpAlign);
    G4_SrcModifier modifier = origSrc->getModifier();
    origSrc->setModifier(Mod_src_undef);
    G4_DstRegRegion* dst = builder.Create_Dst_Opnd_From_Dcl(dcl, 1);

    G4_INST* movInst = builder.createMov(newExecSize, dst, origSrc, InstOpt_WriteEnable, false);

    bb->insert(it, movInst);
    G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(modifier, Direct, dcl->getRegVar(),
        0, 0, newExecSize == 1 ? builder.getRegionScalar() : builder.getRegionStride1(),
        dcl->getElemType());

    return newSrc;
}

G4_SrcRegRegion* HWConformity::insertCopyAtBBEntry(G4_BB* bb, uint8_t execSize, G4_Operand* src)
{
    MUST_BE_TRUE(src != nullptr && src->isSrcRegRegion(), "source must be a SrcRegRegion");
    G4_SrcRegRegion* origSrc = src->asSrcRegRegion();
    auto lb = src->getLinearizedStart();
    auto rb = src->getLinearizedEnd();

    unsigned int regNum = lb / G4_GRF_REG_NBYTES;
    unsigned int numRegs = (rb + G4_GRF_REG_NBYTES - 1 - lb) / G4_GRF_REG_NBYTES;
    if (regNum == -1 || numRegs == 0)
    {
        return nullptr;
    }

    G4_Declare* dcl = builder.createTempVar(execSize, origSrc->getType(), GRFALIGN);
    dcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(regNum), 0);
    G4_SrcModifier modifier = origSrc->getModifier();
    origSrc->setModifier(Mod_src_undef);
    G4_DstRegRegion* dst = builder.Create_Dst_Opnd_From_Dcl(dcl, 1);
    dst->computePReg();

    G4_INST* movInst = builder.createMov(execSize, dst, origSrc, InstOpt_WriteEnable, false);

    for (auto it = bb->begin();
        it != bb->end();
        it++)
    {
        if (!(*it)->isLabel())
        {
            bb->insert(it, movInst);
            break;
        }
    }

    G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(modifier, Direct, dcl->getRegVar(),
        0, 0, execSize == 1 ? builder.getRegionScalar() : builder.getRegionStride1(),
        dcl->getElemType());
    newSrc->asSrcRegRegion()->computePReg();
    return newSrc;
}

/*
 *  create a new mov instruction
 *  mov (esize) tmp<1>:type src
 *  where esize is "inst"'s execution size and insert it before "inst"
 *  return value is the new temp variable as a source operand
 */
G4_Operand* HWConformity::insertMovBefore(
    INST_LIST_ITER it, uint32_t srcNum, G4_Type type, G4_BB* bb,
    G4_SubReg_Align tmpAlign)
{
    G4_INST* inst = *it;
    G4_SubReg_Align subAlign;
    const RegionDesc* region = nullptr;
    unsigned char exec_size = inst->getExecSize();
    G4_Operand* src = inst->getSrc(srcNum);
    unsigned short scale = IS_BTYPE(src->getType()) && src->getType() == type ? 2 : 1;

    uint8_t newExecSize = (src->isImm() && !IS_VTYPE(src->getType())) ||
        (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar())
        ? 1 : exec_size;

    if (newExecSize > 1)
    {
        if (scale == 1 && !IS_VTYPE(src->getType()))
        {
            scale = (uint16_t)(getTypeSize(src->getType()) / getTypeSize(type));
        }
        if (scale == 0)
        {
            scale = 1;
        }
        region = builder.createRegionDesc(scale, 1, 0);
    }
    else
    {
        scale = (uint16_t)(getTypeSize(src->getType()) / getTypeSize(type));
        if (scale == 0)
        {
            scale = 1;
        }
        region = builder.getRegionScalar();
    }

    int opExecWidthBytes = IS_VINTTYPE(src->getType()) ?
        G4_GRF_REG_NBYTES / 2 * (exec_size > 8 ? exec_size / 8 : 1) :
        (src->getType() == Type_VF ?
            G4_GRF_REG_NBYTES / 2 * (exec_size > 4 ? exec_size / 4 : 1) :
            newExecSize * getTypeSize(type)* scale);

    subAlign = getDclAlignment(opExecWidthBytes, inst, newExecSize == 1);

    if (subAlign < tmpAlign)
    {
        subAlign = tmpAlign;
    }

    uint32_t newInstEMask = newExecSize == 1 ? InstOpt_WriteEnable : inst->getMaskOption();

    // due to old BDW regioning rule we need NoMask inst here so they can be split
    if (builder.getOptions()->isTargetCM() && builder.getPlatform() == GENX_BDW)
    {
        if (!bb->isAllLaneActive())
        {
            newInstEMask = InstOpt_WriteEnable;
        }
    }

    G4_Declare* dcl = builder.createTempVar(newExecSize == 1 ? 1 : newExecSize * scale, type, subAlign);
    G4_DstRegRegion* dstRegion = builder.Create_Dst_Opnd_From_Dcl(dcl, scale);
    G4_INST* newInst = builder.createMov(newExecSize, dstRegion, builder.duplicateOperand(src), newInstEMask, false);
    bb->insert(it, newInst);
    inst->transferDef(newInst, Gen4_Operand_Number(srcNum + 1), Opnd_src0);
    newInst->addDefUse(inst, Gen4_Operand_Number(srcNum + 1));

    G4_SrcModifier modifier = Mod_src_undef;
    if (src->isSrcRegRegion() && src->asSrcRegRegion()->getModifier() == Mod_Not)
    {
        // mov doesn't support logic modifiers, so we keep it on the new source
        modifier = Mod_Not;
        newInst->getSrc(0)->asSrcRegRegion()->setModifier(Mod_src_undef);
    }

    return builder.createSrcRegRegion(
        modifier,
        Direct,
        dcl->getRegVar(),
        0,
        0,
        region,
        dcl->getElemType());
}

void HWConformity::fixPackedSource(INST_LIST_ITER it, G4_BB* bb, G4_Type extype)
{
    G4_INST* inst = *it;

    bool nonTypeWFound = false, nonTypeFFound = false, incompatibleTypeFound = false;

    for (int i = 0; i < inst->getNumSrc(); i++)
    {
        G4_Operand* src = inst->getSrc(i);
        if (!src || !(IS_VTYPE(src->getType())))
        {
            // Make sure other src operands are of word type only as this is a HW requirement
            if (src &&
                (src->getType() != Type_W &&
                    src->getType() != Type_UW))
            {
                nonTypeWFound = true;
            }
            if (src &&
                (src->getType() != Type_F))
            {
                nonTypeFFound = true;
            }
            continue;
        }
        G4_Type target_type = Type_W;
        if (src->getType() == Type_VF)
        {
            target_type = Type_F;
        }

        if (target_type == Type_W && nonTypeWFound == true)
        {
            // non-word type src is not allowed to co-exist with :v src
            incompatibleTypeFound = true;
        }
        else if (target_type == Type_F && nonTypeFFound == true)
        {
            // non-float type src is not allowed to co-exist with :vf src
            incompatibleTypeFound = true;
        }

        // Insert a move only if immediate operand is not
        // last src operand
        if (i != inst->getNumSrc() - 1 ||
            incompatibleTypeFound == true)
        {
            inst->setSrc(insertMovBefore(it, i, target_type, bb), i);
        }
    }
}
/*
 * fixMathInst() checks the following:
 * The math instruction can only use GRF registers as source(s) and destination.
 * The math instruction does not support indirect addressing modes.
 * source horizontal stride must be 1 with the exception of scalar sources and destination horizontal stride must be always 1.
 * Source and destination offset must be the same, except the case of scalar source
 * DW and UD is the only source format supported for INT DIV, FP16/FP32 is the only source format supported for all the other functions.
 * Mixed DW and UD sources are not allowed for the INT DIV function.
 * For single source math function, <src1> must be programmed as ARF-NULL register.
 */
bool HWConformity::fixMathInst(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;
    G4_DstRegRegion* dst = inst->getDst();
    G4_Operand* src0 = inst->getSrc(0), * src1 = inst->getSrc(1);
    bool mov_dst = false;

    MUST_BE_TRUE(inst->isMath(), "Expect math instruction");
    G4_InstMath* mathInst = inst->asMathInst();

    if (mathInst->getMathCtrl() == MATH_INVM || mathInst->getMathCtrl() == MATH_RSQRTM)
    {
        // split two GRF math macros. This should only happen for FP64
        if (!builder.hasTwoGRFMathMacro() &&
            IS_DFTYPE(inst->getDst()->getType()) && ((uint32_t)(inst->getExecSize() * 2)) > builder.getNativeExecSize())
        {
            evenlySplitInst(it, bb);
            return true;
        }
        // math macros are constructed internally and should already conform to all other HW rules
        return false;
    }

    // covers MATH_INT_DIV, MATH_INT_DIV_QUOT, MATH_INT_DIV_REM
    bool isIntDivide = inst->asMathInst()->isMathIntDiv();
    bool hasSameOffset = hasSameSubregOffset(inst);

    // check if the source needs a move and if so the new move type
    auto needsMove = [this, inst, isIntDivide, hasSameOffset](int srcID, G4_Type& newType)
    {
        assert((srcID == 0 || srcID == 1) && "math can have at most two sources");
        G4_Operand* src = inst->getSrc(srcID);
        newType = src->getType();
        if (isIntDivide)
        {
            G4_Type divType = IS_UNSIGNED_INT(inst->getSrc(0)->getType()) && IS_UNSIGNED_INT(inst->getSrc(1)->getType()) ?
                Type_UD : Type_D;
            if (newType != divType)
            {
                newType = divType;
                return true;
            }
        }
        else if ((src->getType() != Type_F && src->getType() != Type_VF) &&
            (builder.getPlatform() == GENX_BDW || src->getType() != Type_HF))
        {
            // CHV+ supports F/HF math, while BDW only supports F math
            // mix mode math is handled in fixMixedHFInst()
            newType = Type_F;
            return true;
        }

        if (src->isImm())
        {
            if (srcID == 0 && inst->asMathInst()->getMathCtrl() >= MATH_FDIV)
            {
                return true;
            }
        }
        else if (src->isSrcRegRegion())
        {
            G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
            const RegionDesc* rd = srcRegion->getRegion();
            if (srcRegion->getModifier() != Mod_src_undef && isIntDivide)
            {
                // no source modifer for int divide
                return true;
            }
            else if (srcRegion->getRegAccess() != Direct)
            {
                return true;
            }
            else if (!srcRegion->isScalar())
            {
                if (!hasSameOffset && !builder.isOpndAligned(srcRegion, GENX_GRF_REG_SIZ))
                {
                    return true;
                }
                else if (!rd->isContiguous(inst->getExecSize()))
                {
                    return true;
                }
            }
        }
        else
        {
            ASSERT_USER(false, "Unexpected math source!");
        }
        return false;
    };

    if (src0)
    {
        G4_Type src0_type = src0->getType();
        bool needsSrc0Mov = needsMove(0, src0_type);
        if (needsSrc0Mov)
        {
            inst->setSrc(insertMovBefore(it, 0, src0->isImm() ? G4_Operand::GetNonVectorImmType(src0_type) : src0_type, bb), 0);
            src0 = inst->getSrc(0);
        }
    }

    bool nullSrc1 = src1 && src1->isNullReg();
    if (!nullSrc1 && src1)
    {
        G4_Type src1_type = src1->getType();
        bool needsSrc1Move = needsMove(1, src1_type);

        if (needsSrc1Move)
        {
            if (isIntDivide && src1->isImm() && !IS_VINTTYPE(src1->getType()))
            {
                // just change the immediate's type
                uint32_t immVal = (uint32_t)src1->asImm()->getImm();
                inst->setSrc(builder.createImm(immVal, src1_type), 1);
            }
            else
            {
                inst->setSrc(insertMovBefore(it, 1, src1->isImm() ? G4_Operand::GetNonVectorImmType(src1_type) : src1_type, bb), 1);
            }
            src1 = inst->getSrc(1);
        }
    }

    if (nullSrc1 && src0 && src1->getType() != src0->getType())
    {
        G4_SrcRegRegion* src1_opnd = builder.createNullSrc(inst->getSrc(0)->getType());
        inst->setSrc(src1_opnd, 1);
    }

    // recompute as src0 and src1 may have been modified
    hasSameOffset = hasSameSubregOffset(inst);
    G4_Type extype = inst->getExecType2();
    bool cond1 = (dst->getType() != extype && !(dst->getType() == Type_UD && extype == Type_D));
    if (dst->getRegAccess() != Direct || dst->getHorzStride() != 1 || cond1 ||
        (!hasSameOffset && inst->getExecSize() != 1 && !builder.isOpndAligned(dst, GENX_GRF_REG_SIZ)))
    {
        mov_dst = true;
        G4_DstRegRegion* new_dst = insertMovAfter(it, dst, extype, bb);
        inst->setDest(new_dst);
    }
    return mov_dst;
}

//  find a common (integer) type for constant folding.  The rules are:
//  -- both types must be int
//  -- Q and UQ are not folded
//  -- UD if one of the type is UD
//  -- D otherwise
//
//  returns Type_UNDEF if no appropriate type can be found
//
static G4_Type findConstFoldCommonType(G4_Type type1, G4_Type type2)
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

//
// returns true if all sources and dst in this inst have the same fixed subreg offset
// null src/dst, scalar sources and immediates are excluded from the check
//
bool HWConformity::hasSameSubregOffset(G4_INST* inst) const
{
    bool anyOffset = true; // true means offset is not fixed yet
    uint32_t byteOffset = 0;
    if (inst->getDst())
    {
        G4_DstRegRegion* dst = inst->getDst();
        if (dst->isNullReg())
        {
            // do nothing
        }
        else if (dst->hasFixedSubregOffset(byteOffset))
        {
            anyOffset = false;
        }
        else
        {
            return false;
        }
    }

    for (int i = 0; i < inst->getNumSrc(); ++i)
    {
        G4_Operand* src = inst->getSrc(i);
        if (src->isSrcRegRegion())
        {
            uint32_t srcOffset = 0;
            G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
            if (srcRegion->isNullReg() || srcRegion->isScalar())
            {
                continue;
            }
            else if (srcRegion->hasFixedSubregOffset(srcOffset))
            {
                if (anyOffset)
                {
                    byteOffset = srcOffset;
                    anyOffset = false;
                }
                else if (srcOffset != byteOffset)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

// Check the following rules
// -- src0 in 2 source instructions may not be immediate.  We try to swap for src0 and src1 for
//    commutative instructions in such cases
// -- ARF may not be in src1
void HWConformity::fixImmAndARFSrc(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;
    if (inst->mayExceedTwoGRF())
    {
        return;
    }

    G4_Operand* src0, * src1, * src2;
    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
    src2 = inst->getSrc(2);

    /* Check for usage of two constants in binary operations */
    if (src0 && (src0->isImm() || src0->isAddrExp()) && inst->getNumSrc() == 2)
    {
        if (INST_COMMUTATIVE(inst->opcode()) && !src1->isImm())
        {
            //all commutative inst must have 2 sources
            if (inst->opcode() == G4_mul)
            {
                bool needConstMov;
                //for DW and W mul, src0 must be DW and src1 W
                needConstMov = IS_DTYPE(src0->getType()) && !IS_DTYPE(src1->getType());

                if (needConstMov)
                {
                    G4_Type tmpType = G4_Operand::GetNonVectorImmType(src0->getType());

                    G4_Operand* newSrc0 = insertMovBefore(it, 0, tmpType, bb);
                    inst->setSrc(newSrc0, 0);
                }
                else
                {
                    // swap operands
                    inst->swapSrc(0, 1);
                    inst->swapDefUse();
                }
            }
            else
            {
                // swap operands
                inst->swapSrc(0, 1);
                inst->swapDefUse();
            }
        }
        /*
        * A select operation isn't commutative, but we may commute the
        * operands provided we perform a predicate inversion as well.
        * (v0)  sel ... const V1
        *    =>
        * (-v0) sel ... V1 const
        */
        else if (inst->opcode() == G4_sel && !src1->isImm())
        {
            G4_CondMod* cond = inst->getCondMod();
            if (cond)
            {
                switch (cond->getMod())
                {
                case Mod_ne:
                    inst->setCondMod(builder.createCondMod(Mod_e, cond->getBase(), 0));
                    break;
                case Mod_e:
                    inst->setCondMod(builder.createCondMod(Mod_ne, cond->getBase(), 0));
                    break;
                default:
                    break;
                }
            }
            else
            {
                G4_Predicate* pred = inst->getPredicate();
                MUST_BE_TRUE(pred != NULL, "predicate must not be null");
                G4_PredState reverse = pred->getState() == PredState_Minus ? PredState_Plus : PredState_Minus;
                inst->setPredicate(builder.createPredicate(
                    reverse, pred->getBase(), pred->getSubRegOff(), pred->getControl()));
            }
            inst->swapSrc(0, 1);
            inst->swapDefUse();
        }
        else if (!inst->isMath())
        {
            // math immediate src0 is handled separately in fixMathInst()
            if ((inst->opcode() == G4_add || inst->opcode() == G4_mul) &&
                src0->isImm() && src1->isImm() &&
                IS_TYPE_INT(src0->getType()) && IS_TYPE_INT(src1->getType()) &&
                inst->getSaturate() == false)
            {
                // FIXME: this is duplicating the functionality of Optimizer::doConsFolding.
                G4_Type src0T = src0->getType(), src1T = src1->getType(), resultType = src0T;

                resultType = findConstFoldCommonType(src0T, src1T);
                if (resultType != Type_UNDEF)
                {
                    G4_Imm* newSrc = NULL;
                    int64_t res = inst->opcode() == G4_add ?
                        ((int64_t)(src0->asImm()->getInt()) + (int64_t)(src1->asImm()->getInt())) :
                        ((int64_t)(src0->asImm()->getInt()) * (int64_t)(src1->asImm()->getInt()));

                    // don't fold if the value overflows D/UD
                    if (G4_Imm::isInTypeRange(res, resultType))
                    {
                        newSrc = builder.createImmWithLowerType(res, resultType);

                        // change instruction into a MOV
                        // change instruction into a MOV
                        inst->setOpcode(G4_mov);
                        inst->setSrc(newSrc, 0);
                        inst->setSrc(nullptr, 1);
                        return;
                    }
                }
            }
            // If src0 is not 64-bit, src1 is 64-bit, swap them to save one move.
            if (INST_COMMUTATIVE(inst->opcode()) && src0->isImm() && src1->isImm() &&
                G4_Type_Table[src0->getType()].byteSize != 8 &&
                G4_Type_Table[src1->getType()].byteSize == 8)
            {
                inst->swapSrc(0, 1);
                inst->swapDefUse();
                src0 = inst->getSrc(0);
                src1 = inst->getSrc(1);
                // this needs to fall through as we still need move for src0
            }

            if (INST_COMMUTATIVE(inst->opcode()) && src0->isAddrExp() && src1->isImm())
            {
                // The original IR has both addr expr and immediate
                //   add(8) A0(0, 0)<1>:uw &V36 + 0 0xeca86420 : uv{ Align1, Q1 }
                // We insert a move for src1 which is an immediate
                //   mov(8) TV0(0, 0)<1> : uw 0xeca86420 : uv{ Align1 }
                //   add(8) A0(0, 0)<1> : uw &V36 + 0 TV0(0, 0)<8; 8, 1> : uw{ Align1, Q1 }
                G4_Type type = src1->getType();
                inst->setSrc(insertMovBefore(it, 1, G4_Operand::GetNonVectorImmType(type), bb), 1);
                // And we swap addr expr and the new variable
                //   add(8) A0(0, 0)<1> : uw TV0(0, 0)<8; 8, 1> : uw &V36 + 0 {Align1, Q1}
                // The final code sequence is
                //   mov(8) r13.0<1>:uw 0xeca86420 : uv{ Align1 } // #26:$9:%79
                //   add(8) a0.0<1> : uw r13.0<8; 8, 1> : uw 0x60 : uw{ Align1, Q1 }
                inst->setSrc(inst->getSrc(1), 0);
                inst->setSrc(src0, 1);
                inst->swapDefUse();
            }
            else
            {
                G4_Type newSrcType = inst->needsDWType() ? (IS_UNSIGNED_INT(src0->getType()) ? Type_UD : Type_D) :
                    G4_Operand::GetNonVectorImmType(src0->getType());
                inst->setSrc(insertMovBefore(it, 0, newSrcType, bb), 0);
            }
        }
    }

    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
    src2 = inst->getSrc(2);

    // check for non-mad 3src inst
    if (inst->getNumSrc() == 3 && src1->isImm())
    {
        inst->setSrc(insertMovBefore(it, 1, INST_FLOAT_SRC_ONLY(inst->opcode()) ? Type_F : src1->getType(), bb), 1);
    }

    // Architecture registers may not appear as src1.
    auto isARF = [](G4_Operand* opnd) { return opnd->isAreg() || opnd->isFlag(); };
    if (src1 != nullptr && isARF(src1) && !src1->isNullReg())
    {
        /* See if we can swap the src1 */
        if (INST_COMMUTATIVE(inst->opcode()) && !isARF(src0))
        {
            inst->swapSrc(0, 1);
            inst->swapDefUse();
        }
        else
        {
            /* Otherwise introduce a tmp */
            inst->setSrc(insertMovBefore(it, 1, INST_FLOAT_SRC_ONLY(inst->opcode()) ? Type_F : src1->getType(), bb), 1);
        }
    }

    src2 = inst->getSrc(2);

    /* 3 src instructions can't have any constants */
    if (!builder.hasAlign1Ternary() && src2 != nullptr && src2->isImm())
    {
        inst->setSrc(insertMovBefore(it, 2, src2->getType(), bb), 2);
    }
}

bool HWConformity::fixLine(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;

    if (inst->opcode() == G4_line)
    {
        bool badRegion = false;
        G4_Operand* src0 = inst->getSrc(0);
        // assumption: there are 4 elements in src0
        if (src0->isSrcRegRegion())
        {
            const RegionDesc* rd = src0->asSrcRegRegion()->getRegion();
            badRegion = (rd->vertStride != 0 || rd->width != 4 || rd->horzStride != 1);
        }
        if (!IS_FTYPE(src0->getType()) || src0->isImm() || badRegion ||
            !builder.isOpndAligned(src0, G4_GRF_REG_NBYTES / 2))
        {
            // insertMovBefore()  is not used here
            // due to the special region <0;4,1> of src0 of line
            G4_Declare* src0_dcl;
            G4_DstRegRegion* new_dst_opnd;
            G4_SrcRegRegion* new_src0_opnd;
            unsigned char mov_size = 4;

            src0_dcl = builder.createTempVar(mov_size, Type_F, Eight_Word);
            /* Create temporary variable */
            // Actully we set region to be <0;4,1> directly here.
            const RegionDesc* rd = builder.createRegionDesc(0, 4, 1);
            new_src0_opnd = builder.Create_Src_Opnd_From_Dcl(src0_dcl, rd);
            new_dst_opnd = builder.Create_Dst_Opnd_From_Dcl(src0_dcl, 1);

            G4_INST* newInst = builder.createMov(mov_size, new_dst_opnd, src0, InstOpt_NoOpt, false);
            newInst->setNoMask(true);

            bb->insert(it, newInst);
            inst->setSrc(new_src0_opnd, 0);
            return true;
        }
    }
    return false;
}

bool HWConformity::fixOpndType(INST_LIST_ITER it, G4_BB* bb)
{
    /*
    * Check for instruction that only accept float/int operands, as well as
    * instruction with mixed operand types.  Even though vISA itself forbids
    * mixed type instructions, optimizations such as copy propagation
    * may reintroduce them and so we do the checks here
    */
    G4_INST* inst = *it;
    bool changed = false;
    int numSrc = inst->getNumSrc();
    bool has_float = false;
    bool has_int = false;

    if (inst->mayExceedTwoGRF() || inst->opcode() == G4_smov)
    {
        // skip special instructions
        return false;
    }

    for (int i = 0; i < numSrc; i++)
    {
        if (!inst->getSrc(i))
        {
            continue;
        }
        G4_Type ty = inst->getSrc(i)->getType();
        if (IS_TYPE_FLOAT_ALL(ty))
        {
            has_float = true;
        }
        else
        {
            has_int = true;
        }
    }
    if (has_float && has_int)
    {
        for (int i = 0; i < numSrc; i++)
        {
            if (inst->getSrc(i) && !IS_FTYPE(inst->getSrc(i)->getType()) && !IS_DFTYPE(inst->getSrc(i)->getType()))
            {
                // FIXME: we should probably either get rid of this or assert,
                // it's unlikely that blinding casting int to float is the right thing here
                inst->setSrc(insertMovBefore(it, i, Type_F, bb), i);
                changed = true;
            }
        }
    }

    if (builder.noSrc1Byte())
    {
        if (numSrc > 1)
        {
            G4_Operand* src0 = inst->getSrc(0);
            G4_Operand* src1 = inst->getSrc(1);
            if (src0 != nullptr && src1 != nullptr && IS_BTYPE(src1->getType()))
            {
                if (!IS_BTYPE(src0->getType()) && inst->canSwapSource())
                {
                    inst->swapSrc(0, 1);
                }
                else
                {
                    inst->setSrc(insertMovBefore(it, 1, Type_W, bb), 1);
                    changed = true;
                }
            }
        }
    }
    return changed;
}

/*
 * fixOpnds() looks for operands conformity:
 * 1. checks can operand be a constant.
 * 2. checks if operand's type is conformant to operation.
 * 3. check if only src0 uses VxH
 * 4. check if indirect scalar is used in compressed inst
 * It tries to fix these cases by changing operands order if possible
 * or by insertion if temporary location with appropriate conversion.
 */
void HWConformity::fixOpnds(INST_LIST_ITER it, G4_BB* bb, G4_Type& exType)
{
    G4_INST* inst = *it;
    if (inst->isSend())
    {
        return;
    }

    G4_Operand* src0, * src1, * src2;

    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
    src2 = inst->getSrc(2);

    if (inst->opcode() == G4_mul)
    {
        if (IS_DTYPE(src1->getType()) &&
            !(IS_DTYPE(src0->getType()) || IS_FTYPE(src0->getType())))
        {
            // check if src0 uses VxH
            bool src0_use_VxH = false;

            if (src0->isSrcRegRegion() && src0->asSrcRegRegion()->getRegAccess() != Direct &&
                src0->asSrcRegRegion()->getRegion()->isRegionWH()) // is this safe?
            {
                src0_use_VxH = true;
            }
            if (src0_use_VxH)
            {
                src0 = insertMovBefore(it, 0, src0->getType(), bb);
            }
            inst->setSrc(src0, 1);
            inst->setSrc(src1, 0);
            inst->swapDefUse();
            src0 = inst->getSrc(0);
            src1 = inst->getSrc(1);
        }

        if (src1->isSrcRegRegion() && src1->asSrcRegRegion()->getRegAccess() != Direct &&
            src1->asSrcRegRegion()->getRegion()->isRegionWH())
        {
            if (IS_DTYPE(src0->getType()) &&
                !(IS_DTYPE(src1->getType()) || IS_FTYPE(src1->getType())))
            {
                inst->setSrc(insertMovBefore(it, 1, src1->getType(), bb), 1);
            }
            else
            {
                inst->swapSrc(0, 1);
                inst->swapDefUse();
            }
            src0 = inst->getSrc(0);
            src1 = inst->getSrc(1);
        }
    }

    fixImmAndARFSrc(it, bb);

    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
    src2 = inst->getSrc(2);

    // Vx1 and VxH can only be used for src0
    bool src0_use_VxH = false, src1_use_VxH = false;

    if (src2 &&
        src2->isSrcRegRegion() &&
        src2->asSrcRegRegion()->getRegion()->isRegionWH())
    {
        inst->setSrc(insertMovBefore(it, 2, exType, bb), 2);
    }

    if (src0 != NULL &&
        src0->isSrcRegRegion() &&
        src0->asSrcRegRegion()->getRegion()->isRegionWH())
    {
        src0_use_VxH = true;
    }

    if (src1 != NULL &&
        src1->isSrcRegRegion() &&
        src1->asSrcRegRegion()->getRegion()->isRegionWH())
    {
        src1_use_VxH = true;
    }

    if (src1_use_VxH)
    {
        if ((INST_COMMUTATIVE(inst->opcode()) || inst->opcode() == G4_cmp)
            && !src0_use_VxH &&
            !(inst->opcode() == G4_mul && IS_DTYPE(src0->getType())))
        {
            inst->swapSrc(0, 1);
            if (inst->opcode() == G4_cmp)
            {
                // change condMod
                G4_CondMod* condMod = inst->getCondMod();
                if (condMod)
                {
                    G4_CondMod* newCondModOpnd = builder.createCondMod(
                        getReverseCondMod(condMod->getMod()), condMod->getBase(), condMod->getSubRegOff());
                    inst->setCondMod(newCondModOpnd);
                }
            }
        }
        else
        {
            inst->setSrc(insertMovBefore(it, 1, exType, bb), 1);
        }
    }

    // at this point only src0 may be VxH
    // VxH regioning and conditional modifiers may not co-exist
    if (builder.getPlatform() >= GENX_ICLLP)
    {
        src0 = inst->getSrc(0);
        if (src0 && src0->isSrcRegRegion() && src0->asSrcRegRegion()->getRegion()->isRegionWH())
        {
            if (inst->getCondMod())
            {
                inst->setSrc(insertMovBefore(it, 0, src0->getType(), bb), 0);
            }
        }
    }
}

void HWConformity::fixAlign13SrcInst(INST_LIST_ITER iter, G4_BB* bb)
{
    // again mad should already conform by construction
    G4_INST* inst = *iter;
    MUST_BE_TRUE(inst->getNumSrc() == 3 && !inst->isSend(), "expect 3src inst");

    if (inst->opcode() != G4_mad)
    {
        G4_DstRegRegion* dst = inst->getDst();
        if (!isGoodAlign1TernaryDst(inst))
        {
            auto alignment = builder.noSrc2Regioning() ? GRFALIGN : Four_Word;
            G4_DstRegRegion* tmpDst = insertMovAfter(iter, dst, dst->getType(), bb, alignment);
            inst->setDest(tmpDst);
        }

        bool canBeImm = true;
        for (int i = 0; i < inst->getNumSrc(); ++i)
        {
            if (!isGoodAlign1TernarySrc(inst, i, canBeImm))
            {
                if (i == 2 && builder.noSrc2Regioning())
                {
                    // some additional handling for src2 when src2 regioning is not available
                    fixSrc2(iter, bb, false);
                }
                else
                {
                    G4_SubReg_Align subalign = (i == 2) ? Four_Word : Any;
                    inst->setSrc(insertMovBefore(iter, i, inst->getSrc(i)->getType(), bb, subalign), i);
                }
            }
            else
            {
                if (inst->getSrc(i)->isImm())
                {
                    canBeImm = false;
                }
            }
        }
    }
}

void HWConformity::fix3SrcInst(INST_LIST_ITER iter, G4_BB* bb)
{
    G4_INST* inst = *iter;
    if (inst->getNumSrc() != 3 || inst->mayExceedTwoGRF() || inst->opcode() == G4_madm)
    {
        return;
    }

    if (builder.hasAlign1Ternary())
    {
        fixAlign13SrcInst(iter, bb);
        return;
    }

    if (inst->opcode() != G4_mad)
    {
        // check that dst and srcs are legal for 3src.  We do not check
        // mad since they should already conform by construction
        uint8_t execSize = inst->getExecSize();
        G4_DstRegRegion* dst = inst->getDst();
        if (dst->getRegAccess() != Direct || dst->getHorzStride() != 1 ||
            !builder.isOpndAligned(dst, (execSize >= 8) ? 32 : execSize * 4))
        {
            G4_DstRegRegion* tmpDst = insertMovAfter(iter, dst, dst->getType(), bb);
            inst->setDest(tmpDst);
        }
        for (int i = 0; i < 3; i++)
        {
            if (!isGoodAlign16Src(inst, i))
            {
                inst->setSrc(
                    insertMovBefore(iter, i, inst->getSrc(i)->getType(), bb),
                    i);
            }
        }
    }

    //When it is set (Align16), the instruction uses 16-byte-aligned addressing for source and destination operands.
    if ((inst->getExecSize() == 1))
    {
        if (inst->getDst() &&
            inst->getDst()->getBase()->isRegVar())
        {
            if (!builder.isOpndAligned(inst->getDst(), 16))
            {
                G4_DstRegRegion* new_dst = insertMovAfter(iter, inst->getDst(), inst->getDst()->getType(), bb);
                G4_Declare* tmpDstDcl = new_dst->getTopDcl();
                tmpDstDcl->setSubRegAlign(Eight_Word);
                inst->setDest(new_dst);
            }
        }
    }

    if (inst->getExecSize() == 16)
    {
        bool wa3rc = (VISA_WA_CHECK(builder.getPWaTable(), WaDisableSIMD16On3SrcInstr) &&
            !(inst->getExecType() == Type_HF &&
                inst->getOperand(Opnd_src1)->isSrcRegRegion() &&
                inst->getOperand(Opnd_src1)->getType() == Type_HF &&
                !inst->getOperand(Opnd_src1)->asSrcRegRegion()->crossGRF()));

        if (wa3rc)
        {
            evenlySplitInst(iter, bb);
        }
    }
}

void HWConformity::fixCompareInst(
    INST_LIST_ITER i,
    G4_BB* bb,
    G4_Type exType,
    int dst_elsize)
{
    G4_INST* inst = *i;
    G4_Operand* dst = inst->getDst();

    if (dst && dst->isNullReg())
    {
        // change dst hstride if necessary
        if (G4_Type_Table[exType].byteSize != G4_Type_Table[dst->getType()].byteSize)
        {
            // create a new dst with new stride
            G4_DstRegRegion* new_null = builder.createNullDst(exType);
            inst->setDest(new_null);
        }
    }
}

// For integer packing moves, we can replace the src type with the dst type instead of inserting
// a new move to satisfy dst alignment, since integer down conversion is based on truncation
// an inst has to satisfy the following properties:
// -- is a move (duh) and does not have conditional modifiers or saturation
// -- dst must be a direct DstRegRegion that is GRF-aligned
// -- src must be a direct SrcRegRegion with GRF base, no modifiers, and packed/scalar region
// -- both dst and src have integer type, with source stride > dst stride
// returns true if we have successfully down cast the src type
static bool canReplaceMovSrcType(IR_Builder& builder, G4_INST* inst, uint32_t extypesize)
{

    if (inst->opcode() != G4_mov || inst->getCondMod() != NULL || inst->getSaturate())
    {
        return false;
    }
    if (!inst->getSrc(0)->isSrcRegRegion())
    {
        return false;
    }

    G4_DstRegRegion* dst = inst->getDst();
    G4_SrcRegRegion* src0 = inst->getSrc(0)->asSrcRegRegion();
    int dstByteOffset = dst->getByteOffset();
    if (dstByteOffset % extypesize != 0 ||
        dst->getRegAccess() != Direct)
    {
        // don't do this if dst is not GRF aligned, since we have to fix it later anyway
        return false;
    }

    if (src0->getRegAccess() != Direct || src0->getModifier() != Mod_src_undef ||
        (src0->getTopDcl() == NULL || src0->getTopDcl()->getRegFile() != G4_GRF))
    {
        return false;
    }

    bool isIntPackingMove = false;
    if (IS_TYPE_INT(dst->getType()) && IS_TYPE_INT(src0->getType()))
    {
        uint32_t dstAlign = G4_Type_Table[dst->getType()].byteSize * dst->getHorzStride();
        if (dstAlign < G4_Type_Table[src0->getType()].byteSize)
        {
            isIntPackingMove = true;
        }
    }

    if (!isIntPackingMove)
    {
        return false;
    }

    // we only handle direct contiguous and scalar source region for now,
    // as VxH and strided regions are a bit harder to update
    if (src0->getRegion()->isContiguous(inst->getExecSize()))
    {
        uint16_t newHS = extypesize / G4_Type_Table[dst->getType()].byteSize;
        if (newHS > 4)
        {
            // rule out Q -> B moves if Q is not scalar
            return false;
        }
    }
    else if (!src0->isScalar())
    {
        // only handle scalar and contiguous regions for now
        return false;
    }

    // instead of inserting a move, we change src's type to be same as dst type
    // e.g.,
    // mov (8) r1.0<1>:b r2.4<8;8,1>:d
    // becomes
    // mov (8) r1.0<1>:b r2.16<32;8,4>:b
    // This is safe since integer down conversion is based on truncation
    uint32_t typeSizeRatio = extypesize / G4_Type_Table[dst->getType()].byteSize;
    uint32_t numElt = src0->isScalar() ? 1 : inst->getExecSize() * typeSizeRatio;
    G4_Declare* newDcl = builder.createTempVar(numElt, dst->getType(), Any);
    newDcl->setAliasDeclare(src0->getBase()->asRegVar()->getDeclare(), 0);
    const RegionDesc* region = src0->isScalar() ? builder.getRegionScalar() :
        builder.createRegionDesc((uint16_t)inst->getExecSize(), (uint16_t)inst->getExecSize() * typeSizeRatio,
            inst->getExecSize(),
            (uint16_t)typeSizeRatio);
    G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(
        Mod_src_undef,
        Direct,
        newDcl->getRegVar(),
        src0->getRegOff(),
        src0->getSubRegOff() * typeSizeRatio,
        region,
        dst->getType());
    inst->setSrc(newSrc, 0);
    return true;
}

// implement HW restrictions on mov
// -- There is no direct conversion from B/UB to DF or DF to B/UB.
//    Use two instructions and a word or DWord intermediate type.
// -- There is no direct conversion from B/UB to Q/UQ or Q/UQ to B/UB.
//    Use two instructions and a word or DWord intermediate integer type.
// -- There is no direct conversion from HF to DF or DF to HF.
//    Use two instructions and F (Float) as an intermediate type.
// -- There is no direct conversion from HF to Q/UQ or Q/UQ to HF.
//    Use two instructions and F (Float) or a word integer type or a DWord integer type as an intermediate type.
// -- There is no direct scalar conversion from B/UB to HF or F.
//    Use two instructions and a WORD or DWORD intermediate type respectively.
// returns true if a move is inserted
bool HWConformity::fixMov(INST_LIST_ITER i, G4_BB* bb)
{
    G4_INST* inst = *i;

    if (inst->opcode() != G4_mov)
    {
        return false;
    }

    G4_Type dstType = inst->getDst()->getType();
    G4_Type srcType = inst->getSrc(0)->getType();
    auto src = inst->getSrc(0);

    bool scalarByteToFloat = builder.noScalarByteToFloat() &&
        IS_BTYPE(srcType) &&
        (IS_FTYPE(dstType) || IS_HFTYPE(dstType)) &&
        (src->isSrcRegRegion() && src->asSrcRegRegion()->isScalar());
    bool dstByteSrc64b = IS_BTYPE(dstType) && (IS_DFTYPE(srcType) || IS_QTYPE(srcType));

    if (scalarByteToFloat || dstByteSrc64b)
    {
        inst->setDest(insertMovAfter(i, inst->getDst(), Type_W, bb));
        return true;
    }
    if (IS_BTYPE(srcType) && (IS_DFTYPE(dstType) || IS_QTYPE(dstType)))
    {
        // mov Q/DF B
        inst->setDest(insertMovAfter(i, inst->getDst(), Type_W, bb));
        return true;
    }
    if (isLowPrecisionFloatTy(dstType) && (IS_DFTYPE(srcType) || IS_QTYPE(srcType)))
    {
        // mov HF Q/DF
        inst->setDest(insertMovAfter(i, inst->getDst(), Type_F, bb));
        return true;
    }
    if (isLowPrecisionFloatTy(srcType) && (IS_DFTYPE(dstType) || IS_QTYPE(dstType)))
    {
        // mov Q/DF HF
        inst->setDest(insertMovAfter(i, inst->getDst(), Type_F, bb));
        return true;
    }

    return false;
}

bool HWConformity::fixRotate(INST_LIST_ITER i, G4_BB* bb)
{

    // rotate requires src0 and dst to have the same datatype precision
    // It also does not support *B/*Q types, but that should be enforced at the vISA level
    // returns true if new instruction is inserted
    bool changed = false;
    G4_INST* inst = *i;
    if (inst->opcode() != G4_rol && inst->opcode() != G4_ror)
    {
        return false;
    }
    G4_DstRegRegion* dst = inst->getDst();
    G4_SrcRegRegion* src = inst->getSrc(0)->asSrcRegRegion();

    MUST_BE_TRUE(IS_WTYPE(dst->getType()) || IS_DTYPE(dst->getType()), "dst type must be *W or *D");
    MUST_BE_TRUE(IS_WTYPE(src->getType()) || IS_DTYPE(src->getType()), "src type must be *W or *D");

    if (G4_Type_Table[dst->getType()].byteSize != G4_Type_Table[src->getType()].byteSize)
    {
        // keep exec type same and change dst to be same type as src
        inst->setDest(insertMovAfter(i, dst, src->getType(), bb));
        changed = true;
    }

    if (dst->getType() == Type_W)
    {
        dst->setType(Type_UW);
    }
    else if (dst->getType() == Type_D)
    {
        dst->setType(Type_UD);
    }

    if (src->getType() == Type_W)
    {
        src->setType(Type_UW);
    }
    else if (src->getType() == Type_D)
    {
        src->setType(Type_UD);
    }
    return changed;
}

bool HWConformity::fixDstAlignment(INST_LIST_ITER i, G4_BB* bb, G4_Type extype, unsigned int dst_elsize)
{
    G4_INST* inst = *i;
    bool insertMOV = false;

    unsigned char exec_size = inst->getExecSize();
    G4_DstRegRegion* dst = inst->getDst();
    G4_Operand* src0 = inst->getSrc(0);
    unsigned h_stride = dst->getHorzStride();
    unsigned int extypesize = G4_Type_Table[extype].byteSize;

    if (inst->hasNULLDst())
    {
        if (dst_elsize * h_stride < extypesize)
        {
            uint16_t newHStride = extypesize / dst_elsize;
            if (newHStride == 8)
            {
                // dst is a null byte, this can be produced by logical optimization
                // we chagne the type to W here; this should be safe since the conditional modifier
                // is either .ez or .nz
                MUST_BE_TRUE(dst_elsize == 1, "expect B/UB dst");
                dst->setType(dst->getType() == Type_B ? Type_W : Type_UW);
                dst->setHorzStride(4);
            }
            else
            {
                MUST_BE_TRUE(newHStride <= 4, "horizontal stride must be <=4");
                dst->setHorzStride(newHStride);
            }
        }

        return insertMOV;
    }

    // optimize initialization instructions
    if (inst->opcode() == G4_mov && src0->isImm() &&
        (bb->isAllLaneActive() || inst->isWriteEnableInst()) &&
        !inst->getPredicate() &&
        dst->getRegAccess() == Direct &&
        dst->getHorzStride() == 1 &&
        inst->getSaturate() == false &&
        IS_BTYPE(dst->getType()) &&
        !IS_TYPE_F32_F64(src0->getType()) &&
        builder.isOpndAligned(dst, getTypeSize(src0->getType())))
    {
        // inst is a mov with packed byte dst and int imm source
        int64_t value = src0->asImm()->getInt();
        uint64_t new_value = (value & 0xFF) | (value << 0x8);
        int scale = 2;

        if (IS_DTYPE(src0->getType()))
        {
            scale = 4;
            new_value = (new_value & 0xFFFF) | (new_value << 0x10);
        }

        if (exec_size >= scale)
        {
            G4_Type new_type = (scale == 2) ? Type_UW : Type_UD;
            dst->setHorzStride(1);
            dst->setSubRegOff((short)(dst->getSubRegOff() / scale));
            dst->setType(new_type);
            inst->setSrc(builder.createImm(new_value, new_type), 0);
            inst->setExecSize((unsigned char)(exec_size / scale));
            return insertMOV;
        }
    }

    bool byteDst = IS_BTYPE(dst->getType());

    // Byte can not be used as dstination of INT*INT
    if ((byteDst && inst->opcode() == G4_mul &&
        IS_TYPE_INT(inst->getSrc(0)->getType()) && IS_TYPE_INT(inst->getSrc(1)->getType())))
    {
        // change dst type to W
        inst->setDest(insertMovAfter(i, dst, Type_W, bb));
        return true;
    }

    if (byteDst && extypesize == 8)
    {
        // Gen doesn't support hstride 8, so we add a W move here
        inst->setDest(insertMovAfter(i, dst, Type_W, bb));
        return true;
    }

    bool dstHFMixModeInst = inst->getDst()->getType() == builder.getMixModeType() && extype == Type_F;
    bool dstNotAlignedToExecType = exec_size > 1 && (dst_elsize * h_stride) < extypesize &&
        !(builder.hasMixMode() && dstHFMixModeInst);
    unsigned short dst_byte_offset;
    builder.isOpndAligned(dst, dst_byte_offset, extypesize);
    if (!((dst_byte_offset % extypesize == 0) ||
        (byteDst &&
        (dst_byte_offset % extypesize == 1))
        ) ||
        /*
         * Dynamic offset can be odd for serialized instructions
         * or when horizontal offset is dynamic.
         * Probably we need the same for any dst with dynamic offsets.
         */
        (dst_elsize < extypesize &&
            dst->getRegAccess() != Direct &&
            !(byteDst && extypesize == 2 && exec_size == 1)
            ) ||
        dstNotAlignedToExecType)
    {
        /*
         * 10.3
         * For byte dst type:
         * 1. no 1 horstride
         * 2. no odd start subreg
         * There is only one excpetion - raw mov op
         * Raw means src operand has no attribute.
         *
         * Note: Actually all these cases are now controlled
         *       by extypesize value.
         */

        if (inst->isRawMov() &&
            (dst_byte_offset % extypesize == 0 ||
            (byteDst && dst_byte_offset % extypesize == 1)))
        {
            return insertMOV;
        }

        if (canReplaceMovSrcType(builder, inst, extypesize))
        {
            return false;
        }

        if (inst->opcode() == G4_mov)
        {
            bool intHFConversion = false;
            G4_Operand* src0 = inst->getSrc(0);
            if (isLowPrecisionFloatTy(dst->getType()) && IS_TYPE_INT(src0->getType()))
            {
                intHFConversion = true;
            }
            else if (isLowPrecisionFloatTy(src0->getType()) && IS_TYPE_INT(dst->getType()))
            {
                intHFConversion = true;
            }
            // we allow packed destination for F to HF.
            if (builder.getPlatform() >= GENX_CHV && !intHFConversion && inst->isMixedMode())
            {
                return insertMOV;
            }
        }


        if (splitInstListForByteDst(i, bb, (uint16_t)extypesize))
        {
            return true;
        }

        inst->setDest(insertMovAfter(i, dst, dst->getType(), bb));
        insertMOV = true;
    }

    return insertMOV;
}

/*
 * This function checks to see if the instruction's indirect operands
 * potentially require totally more than 8 distinct addr reg sub-registers, and
 * then determines which of the operands to spill into temporary GRFs so
 * as to limit total number of distinct sub-registers used by the instruction
 * to 8. This is a requirement imposed by the CM register allocator.
 */

bool HWConformity::fixIndirectOpnd(INST_LIST_ITER i, G4_BB* bb)
{
    G4_INST* inst = *i;

    G4_Operand* src0 = inst->getSrc(0), * src1 = inst->getSrc(1);
    G4_DstRegRegion* dst = inst->getDst();
    bool null_dst = (!dst || inst->hasNULLDst());

    bool null_src0 = !src0;
    bool null_src1 = !src1 || (inst->isMath() && src1->isNullReg());

    const int addr_reg_max_count = 16;
    const int addr_reg_size = G4_Type_Table[Type_UW].byteSize;
    int src_uniq_count = 0;
    int src1_count = 0;
    int src0_count = 0;
    int dst_uniq_count = 0;
    int dst_count = 0;
    bool nospill_src1 = false;
    bool nospill_src0 = false;
    bool nospill_dst = false;
    bool spill_src1 = false;
    bool spill_src0 = false;
    bool spill_dst = false;
    G4_Declare* addr_dcl0 = NULL, * addr_dcl1 = NULL, * addr_dcl2 = NULL;
    if (!null_src0 && src0->isSrcRegRegion() &&
        src0->getRegAccess() != Direct && src0->asSrcRegRegion()->getBase()->isRegVar()) {
        addr_dcl0 = src0->asSrcRegRegion()->getBase()->asRegVar()->getDeclare()->getRootDeclare();
        // is the following precise?
        src0_count = addr_dcl0->getTotalElems();
        MUST_BE_TRUE(src0_count <= addr_reg_max_count, "More than 8 address subregisters required for one operand.");
        src_uniq_count += src0_count;
    }

    if (!null_src1 && src1->isSrcRegRegion() &&
        src1->getRegAccess() != Direct && src1->asSrcRegRegion()->getBase()->isRegVar()) {
        addr_dcl1 = src1->asSrcRegRegion()->getBase()->asRegVar()->getDeclare()->getRootDeclare();
        src1_count = addr_dcl1->getTotalElems();
        MUST_BE_TRUE(src1_count <= addr_reg_max_count, "More than 8 address subregisters required for one operand.");
        if (addr_dcl1 != addr_dcl0) {
            // should we use top level dcl here?
            src_uniq_count += src1_count;
        }
        else {
            nospill_src1 = true;
            nospill_src0 = true;
        }
    }

    if (!null_dst &&
        dst->getRegAccess() != Direct && dst->getBase()->isRegVar())
    {
        addr_dcl2 = dst->getBase()->asRegVar()->getDeclare()->getRootDeclare();
        dst_count = addr_dcl2->getTotalElems();
        MUST_BE_TRUE(dst_count <= addr_reg_max_count, "More than 8 address subregisters required for one operand.");
        if (addr_dcl2 != addr_dcl0 && addr_dcl2 != addr_dcl1) {
            dst_uniq_count += dst_count;
        }
        else if (addr_dcl2 != addr_dcl0) {
            nospill_dst = true;
            nospill_src0 = true;
        }
        else {
            nospill_dst = true;
            nospill_src1 = true;
        }
    }

    if (src_uniq_count > addr_reg_max_count) {
        if (src0_count > src1_count || nospill_src1) {
            MUST_BE_TRUE(nospill_src0 == false, "Address of source0 should be spilled.");
            spill_src0 = true;
            src_uniq_count -= src0_count;
        }
        else {
            MUST_BE_TRUE(nospill_src1 == false, "Address of source1 should be spilled.");
            spill_src1 = true;
            src_uniq_count -= src1_count;
        }
    }

    if (src_uniq_count + dst_uniq_count > addr_reg_max_count) {
        MUST_BE_TRUE(nospill_dst == false, "Address of dst should be spilled.");

        if (nospill_src1 && nospill_src0) {
            spill_dst = true;
            dst_uniq_count = 0;
        }
        else if (dst_uniq_count > src0_count&& dst_uniq_count > src1_count) {
            spill_dst = true;
            dst_uniq_count = 0;
        }
        else if (spill_src0) {
            spill_src1 = true;
            src_uniq_count -= src1_count;
        }
        else if (spill_src1) {
            spill_src0 = true;
            src_uniq_count -= src0_count;
        }
        else if (src0_count > src1_count) {
            spill_src0 = true;
            src_uniq_count -= src0_count;
        }
        else {
            spill_src1 = true;
            src_uniq_count -= src1_count;
        }
    }

    MUST_BE_TRUE(src_uniq_count + dst_uniq_count <= addr_reg_max_count,
        "Remianed number of address registers should be no more than 8 after spill.");

    // Is this only for iselect?
    // What if a scalar with indirect addressing is used?
    if (spill_src0) {
        G4_Operand* new_src0 = insertMovBefore(i, 0, src0->getType(), bb);
        inst->setSrc(new_src0, 0);
    }

    if (spill_src1 && src1) {
        G4_Operand* new_src1 = insertMovBefore(i, 1, src1->getType(), bb);
        inst->setSrc(new_src1, 1);
    }

    if (spill_dst && dst)
    {
        G4_DstRegRegion* new_dst = insertMovAfter(i, dst, dst->getType(), bb);
        inst->setDest(new_dst);
        if (dst != new_dst &&
            (IS_FTYPE(dst->getType()) || IS_DFTYPE(dst->getType())))
        {
            inst->setSaturate(false);
        }
    }
    return spill_dst;
}

// If an accumulator is a source operand, its register region must match that of the
// destination register (which means GRF-aligned since we always GRF-align Acc)
// also check for restrictions on explicit acc dst
bool HWConformity::fixAcc(INST_LIST_ITER iter, G4_BB* bb)
{
    G4_INST* inst = *iter;

    bool changed = false;
    auto dst = inst->getDst();
    if ((dst && dst->isAccReg()) || inst->opcode() == G4_mach)
    {
        if (!builder.accDstforIndirectSrc())
        {
            if (inst->getSrc(0)->isSrcRegRegion() && inst->getSrc(0)->asSrcRegRegion()->getRegAccess() == IndirGRF)
            {
                inst->setSrc(insertMovBefore(iter, 0, inst->getSrc(0)->getType(), bb), 0);
                changed = true;
            }
        }
    }

    // implicit acc src/dst get its offset from dst
    bool useAcc = inst->hasImplicitAccSrc() || inst->hasImplicitAccDst();
    if (!useAcc)
    {
        for (int i = 0; i < inst->getNumSrc(); ++i)
        {
            G4_Operand* src = inst->getSrc(i);
            if (src && src->isAccReg())
            {
                useAcc = true;
                break;
            }
        }
    }

    if (useAcc &&
        dst &&
        dst->getBase() &&
        dst->getBase()->isRegVar())
    {
        if (!builder.isOpndAligned(dst, GENX_GRF_REG_SIZ))
        {
            inst->setDest(insertMovAfter(iter, dst, dst->getType(), bb, GRFALIGN));
            changed = true;
        }
    }

    return changed;
}

/*
 * When operation execution size is 1, destination horizontal stride is set
 * according to rule 10.2:
 *
 * 10.1.2. If ExecSize is greater than 1, dst.HorzStride*sizeof(dst.Type) must
 *         be equal to or greater than the size of the execution data type.
 * 10.2. If ExecSize is 1, dst.HorzStride must not be 0. Note that this is
 *       relaxed from rule 10.1.2. Also note that this rule for destination
 *       horizontal stride is different from that for source as stated
 *       in rule #7.
 *
 * There are some instructions which work unpredictably if both ExecSize
 * and dst.HorzStride are 1. But they work fine if dst.HorzStride is set
 * according to rule 10.1.2. So we have to correct all such cases.
 *
 * This supposed to be the last operation before emitting final assembly code.
 */
void HWConformity::fixDstHstride(INST_LIST_ITER i, int extypesize)
{
    G4_INST* inst = *i;
    G4_DstRegRegion* dst = inst->getDst();
    int dst_elsize = G4_Type_Table[dst->getType()].byteSize;

    if (dst)
    {
        unsigned short hs = dst->getHorzStride();
        if (hs * dst_elsize < extypesize)
        {
            dst->setHorzStride((unsigned short)(extypesize / dst_elsize));
        }
    }
}

template<class T>
bool isPreAssignedRegOffsetNonZero(T* region)
{
    // T is non-NULL and either
    // G4_SrcRegRegion or G4_DstRegRegion
    bool ret = false;

    if ((region->isSrcRegRegion() || region->isDstRegRegion()) &&
        region->getBase() &&
        region->getBase()->isRegVar() &&
        region->getBase()->asRegVar()->isPhyRegAssigned() &&
        region->getBase()->asRegVar()->getPhyRegOff() != 0)
    {
        ret = true;
    }

    return ret;
}

void HWConformity::generateMacl(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* mulInst = *it;
    MUST_BE_TRUE(mulInst->opcode() == G4_mul, "expect mul instruction");
    if (mulInst->getExecSize() > builder.getNativeExecSize())
    {
        auto startIter = it;
        bool isFirstInst = startIter == bb->begin();
        if (!isFirstInst)
        {
            --startIter;
        }
        evenlySplitInst(it, bb);
        if (!isFirstInst)
        {
            ++startIter;
        }
        // startIter now points to first mul created by split
        auto endIter = it;
        ++endIter;
        // endIter points to the first inst after the original mul
        for (auto iter = startIter; iter != endIter;)
        {
            auto nextIter = iter;
            ++nextIter;
            G4_INST* currInst = *iter;
            if (currInst->opcode() == G4_mul)
            {
                doGenerateMacl(iter, bb);
            }
            iter = nextIter;
        }
    }
    else
    {
        doGenerateMacl(it, bb);
    }
}

// convert vISA mul (8) dst src0 src1 into
// mul (8) acc0.0<1>:d src0:d src1:w
// mach (8) dst:d src0:d src1:d
//
void HWConformity::doGenerateMacl(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* mulInst = *it;
    MUST_BE_TRUE(mulInst->opcode() == G4_mul, "expect mul instruction");
    assert(mulInst->getExecSize() <= builder.getNativeExecSize() && "expect single register inst");

    G4_Operand* src0 = mulInst->getSrc(0);
    G4_Operand* src1 = mulInst->getSrc(1);
    MUST_BE_TRUE(IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()), "both sources must have dword type");

    if (src1->isSrcRegRegion())
    {
        G4_SrcRegRegion* src1Region = src1->asSrcRegRegion();
        if (src1Region->getModifier() != Mod_src_undef)
        {
            // need extra move for the modifier
            src1 = insertMovBefore(it, 1, src1->getType(), bb);
            mulInst->setSrc(src1, 1);
        }
    }

    // sat cannot be used at all in the macro sequence
    // this effectivly means sat is broken for mul D D D
    mulInst->setSaturate(false);

    G4_DstRegRegion* origDst = mulInst->getDst();
    G4_Type accType = (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType())) ? Type_UD : Type_D;
    G4_DstRegRegion* accDstOpnd = builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, accType);
    mulInst->setDest(accDstOpnd);

    uint32_t origOptions = mulInst->getOption();
    fixMulSrc1(it, bb);
    mulInst->setOptionOn(InstOpt_WriteEnable);

    G4_Predicate* predicate = mulInst->getPredicate();
    if (predicate != nullptr)
    {
        // move pred to mach
        mulInst->setPredicate(nullptr);
    }
    if (mulInst->getCondMod() != nullptr)
    {
        // conditional modifier cannot be used
        // when the MUL source operand is of dword type.
        MUST_BE_TRUE(false, "Dw multiply does not support conditional modifiers");
        mulInst->setCondMod(nullptr);
    }

    // create a mach inst
    G4_INST* machInst = builder.createBinOp(G4_mach, mulInst->getExecSize(),
        origDst, builder.duplicateOperand(src0), builder.duplicateOperand(src1), origOptions, false);
    machInst->setPredicate(predicate);

    // maintain du chain as fixAccDst uses it later
    G4_SrcRegRegion* accSrcOpnd = builder.createSrcRegRegion(Mod_src_undef, Direct,
        builder.phyregpool.getAcc0Reg(), 0, 0, builder.getRegionStride1(), accType);
    machInst->setImplAccSrc(accSrcOpnd);
    mulInst->addDefUse(machInst, Opnd_implAccSrc);

    INST_LIST_ITER machIter = it;
    machIter = bb->insert(++machIter, machInst);

    if (!IS_DTYPE(origDst->getType()) || origDst->getHorzStride() != 1 ||
        !builder.isOpndAligned(origDst, 32))
    {
        // mach dst must be grf-aligned, packed D/UD as it is also used for the implicit acc source's region
        G4_DstRegRegion* tmpDst = insertMovAfter(machIter, origDst, accType, bb);
        machInst->setDest(tmpDst);
    }

}

// get rid of source modifiers on this inst[srcPos]
bool HWConformity::checkSrcMod(INST_LIST_ITER it, G4_BB* bb, int srcPos)
{
    bool changed = false;
    G4_INST* inst = *it;
    assert(srcPos < inst->getNumSrc() && "invalid srcPos");
    auto src = inst->getSrc(srcPos);
    if (src->isSrcRegRegion())
    {
        G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
        if (srcRegion->getModifier() != Mod_src_undef)
        {
            G4_Type type = IS_DTYPE(src->getType()) ? src->getType() : Type_D;
            src = insertMovBefore(it, srcPos, type, bb);
            inst->setSrc(src, srcPos);
            changed = true;
        }
    }
    return changed;
}

// If both source operands of an MUL instruction are of dword integer type,
// only the lower 16 bits of data elements in src0 are used.
// The full precision multiplication results can be only produced together
// with the mach and mov instructions.

bool HWConformity::fixMULInst(INST_LIST_ITER& i, G4_BB* bb)
{
    bool insertedInst = false;
    G4_INST* inst = *i;
    G4_DstRegRegion* dst = inst->getDst();
    uint8_t exec_size = inst->getExecSize();
    bool srcExchanged = false;

    if (dst->isAccReg())
    {
        return false;
    }

    uint32_t inst_opt = inst->getOption();
    G4_Operand* src0 = inst->getSrc(0), * src1 = inst->getSrc(1);

    // MUL is commutative and only
    // allows src1 to be a constant.
    // If src1 is a constant and src1
    // is not, they are swapped here.
    // If both are constants, they
    // will be fixed in checking HW conformity.
    // this is fixed in fixOpnd.

    if (src0->isImm() && !src1->isImm())
    {
        inst->swapSrc(0, 1);
        srcExchanged = true;
    }

    if (!builder.supportSrcModforMul() &&
        (IS_DTYPE(src0->getType()) || IS_DTYPE(src1->getType())) &&
        ((getTypeSize(src0->getType()) < 4) || (getTypeSize(src1->getType()) < 4)))

    {
        checkSrcMod(i, bb, 0);
        checkSrcMod(i, bb, 1);
    }

    src0 = inst->getSrc(0);
    src1 = inst->getSrc(1);
    // Q dst needs 64-bit support regardless of src type
    bool isDMul = IS_QTYPE(dst->getType()) || (IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()));

    if (!isDMul)
    {
        return false;
    }

    if (builder.hasMacl() && !IS_QTYPE(dst->getType()) &&
        (builder.noDwDstForDwordMul() || inst->getExecSize() > 1))
    {
        // use macl for D = D x D. We use macl when possible
        // except on scalar inst on platforms that support native DMul
        generateMacl(i, bb);
        return true;
    }

    bool doNativeMul = false;
    if (!builder.no64bitRegioning())
    {
        // platform natively supports DW-DW multiply, no need to generate mul/mach/mov sequence
        doNativeMul = true;
    }
    else
    {
        if ((builder.getPlatform() == GENX_CHV || builder.getPlatform() == GENX_BXT))
        {
            if (inst->getExecSize() == 1)
            {
                // scalar insts are a-ok
                return false;
            }
            // ok if source is scalar or qword-aligned
            doNativeMul = (getTypeSize(dst->getType()) * dst->getHorzStride() == 8);
            auto isQWordStride = [inst, this](G4_SrcRegRegion* src)
            {
                const RegionDesc* region = src->getRegion();
                if (!region->isScalar())
                {
                    uint16_t stride = 0;
                    (void)region->isSingleNonUnitStride(inst->getExecSize(), stride);
                    if (stride != 2)
                    {
                        return false;
                    }
                    // check that source is GRF-aligned to ensure that every element is qword-aligned
                    return builder.isOpndAligned(src, 32);
                }
                return true;
            };
            if (doNativeMul && src0->isSrcRegRegion())
            {
                doNativeMul = isQWordStride(src0->asSrcRegRegion());
            }
            if (doNativeMul && src1->isSrcRegRegion())
            {
                doNativeMul = isQWordStride(src1->asSrcRegRegion());
            }
        }
    }

    if (doNativeMul)
    {
        // promote source to D type if necessary
        if (IS_QTYPE(dst->getType()))
        {
            G4_Type newTy;
            G4_Operand* newOpnd;
            if (!IS_DTYPE(src0->getType()))
            {
                newTy = IS_SIGNED_INT(src0->getType()) ? Type_D : Type_UD;
                newOpnd = insertMovBefore(i, 0, newTy, bb);
                inst->setSrc(newOpnd, 0);
                insertedInst = true;
            }

            if (!IS_DTYPE(src1->getType()))
            {
                newTy = IS_SIGNED_INT(src1->getType()) ? Type_D : Type_UD;
                if (src1->isImm())
                {
                    newOpnd = builder.createImm(src1->asImm()->getImm(), newTy);
                }
                else
                {
                    newOpnd = insertMovBefore(i, 1, newTy, bb);
                }
                inst->setSrc(newOpnd, 1);
                insertedInst = true;
            }
        }
        return insertedInst;
    }

    // both sources are dword, replace with mul/mach/mov sequence
    // At this point, src0 and src1 are both DW, so we simply make
    // acc's type (i.e. dst_type) be DW/UD

    G4_CondMod* condmod = builder.duplicateOperand(inst->getCondMod());
    G4_Predicate* pred = builder.duplicateOperand(inst->getPredicate());

    // check if the following inst is mulh and uses the same srcs as this mul.
    // if true, translate them into
    // mul acc src0 src1
    // mach dst_mulh src0 src1
    // mov mul_dst src0 src1
    INST_LIST_ITER next_i = i;
    next_i++;
    G4_Type tmp_type = (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType())) ? Type_UD : Type_D;
    bool isCompressed = isCompressedInst(inst);

    if (src1->isSrcRegRegion())
    {
        G4_SrcRegRegion* src1Region = src1->asSrcRegRegion();
        if (src1Region->getModifier() != Mod_src_undef)
        {
            // need extra move for the modifier
            src1 = insertMovBefore(i, 1, src1->getType(), bb);
            inst->setSrc(src1, 1);
        }
    }

    bool sat_mod = inst->getSaturate();
    inst->setSaturate(false);

    // see if we can combine this mul with a mulh following it
    if (next_i != bb->end())
    {
        G4_INST* next_inst = *next_i;

        if (next_inst->opcode() == G4_mulh &&
            next_inst->getExecSize() == exec_size &&
            inst->getPredicate() == next_inst->getPredicate() &&
            ((srcExchanged &&
                src0->getType() == next_inst->getSrc(1)->getType() &&
                src0->compareOperand(next_inst->getSrc(1)) == Rel_eq &&
                src1->getType() == next_inst->getSrc(0)->getType() &&
                src1->compareOperand(next_inst->getSrc(0)) == Rel_eq) ||
                (!srcExchanged &&
                    src0->getType() == next_inst->getSrc(0)->getType() &&
                    src0->compareOperand(next_inst->getSrc(0)) == Rel_eq &&
                    src1->getType() == next_inst->getSrc(1)->getType() &&
                    src1->compareOperand(next_inst->getSrc(1)) == Rel_eq)))
        {
            // change current mul inst
            G4_DstRegRegion* acc_dst_opnd = builder.createDst(
                builder.phyregpool.getAcc0Reg(),
                0,
                0,
                1,
                tmp_type);

            inst->setDest(acc_dst_opnd);

            fixMulSrc1(i, bb);

            inst->transferUse(next_inst, true);
            inst->addDefUse(next_inst, Opnd_implAccSrc);
            // change mulh inst
            next_inst->setOpcode(G4_mach);

            G4_DstRegRegion* next_dst = next_inst->getDst();
            if (next_dst != NULL &&
                (next_inst->getSaturate() ||
                    next_dst->getByteOffset() % GENX_GRF_REG_SIZ != 0 ||
                    (!bb->isAllLaneActive() && next_inst->isWriteEnableInst() == false) ||
                    (next_dst &&
                    ((next_dst->getExecTypeSize() > G4_Type_Table[Type_D].byteSize) ||
                        isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(next_dst)))))
            {
                // add a tmp mov
                G4_DstRegRegion* new_next_dst = insertMovAfter(next_i, next_dst, next_dst->getType(), bb);
                next_inst->setDest(new_next_dst);
            }

            // set implicit source/dst for MACH
            const RegionDesc* rd = exec_size == 1 ? builder.getRegionScalar() : builder.getRegionStride1();
            G4_SrcRegRegion* acc_src_opnd = builder.createSrcRegRegion(Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, rd, tmp_type);
            next_inst->setImplAccSrc(acc_src_opnd);
            next_inst->setImplAccDst(builder.createDstRegRegion(*acc_dst_opnd));

            // create mov inst
            G4_SrcRegRegion* movAccSrc = builder.createSrcRegRegion(Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, rd, tmp_type);
            G4_INST* newMov = builder.createMov(exec_size, dst, movAccSrc, inst_opt, false);
            newMov->setPredicate(pred);
            newMov->setCondMod(condmod);

            INST_LIST_ITER iter = next_i;
            iter++;
            bb->insert(iter, newMov);

            next_inst->addDefUse(newMov, Opnd_src0);

            INST_LIST_ITER last_iter = iter;
            last_iter--;

            if (dst != NULL &&
                (sat_mod ||
                (dst &&
                    ((dst->getExecTypeSize() > G4_Type_Table[Type_D].byteSize) ||
                    (isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst))))))
            {
                // add a tmp mov
                iter--;
                G4_DstRegRegion* new_next_dst = insertMovAfter(iter, dst, dst->getType(), bb);
                newMov->setDest(new_next_dst);
                if (new_next_dst != dst && sat_mod)
                {
                    MUST_BE_TRUE(iter != bb->end() && (*iter)->opcode() == G4_mov,
                        "Next instruciton should be the MOV generated for consistent Dst and ACC source region.");
                    (*iter)->setSaturate(false);
                }
            }

            next_inst->setOptionOn(InstOpt_AccWrCtrl);

            if (exec_size > builder.getNativeExecSize())
            {
                splitDWMULInst(i, last_iter, bb);
            }
            return true;
        }
    }

    G4_DstRegRegion* acc_dst_opnd = builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmp_type);
    inst->setDest(acc_dst_opnd);
    fixMulSrc1(i, bb);

    inst->setNoMask(true);

    if (pred != NULL) {
        // conditional modifier cannot be used
        // when the MUL source operand is of dword type.
        inst->setCondMod(NULL);
    }

    // Dst is either null, or a temp D if the original dst is Q/UQ
    G4_DstRegRegion* machDst = NULL;
    G4_Declare* high32BitDcl = NULL;
    if (IS_QTYPE(dst->getType()))
    {
        high32BitDcl = builder.createTempVar(exec_size, Type_D, Any);
        machDst = builder.Create_Dst_Opnd_From_Dcl(high32BitDcl, 1);
    }
    else
    {
        machDst = builder.createNullDst(Type_D);
    }

    // create a mach inst
    G4_INST* newInst = builder.createBinOp(G4_mach, exec_size, machDst,
        builder.duplicateOperand(src0), builder.duplicateOperand(src1), inst_opt, false);

    newInst->setOptionOn(InstOpt_AccWrCtrl);

    INST_LIST_ITER iter = i;
    iter++;
    bb->insert(iter, newInst);

    inst->setPredicate(NULL);

    inst->copyDef(newInst, Opnd_src0, Opnd_src0);
    inst->copyDef(newInst, Opnd_src1, Opnd_src1);
    inst->transferUse(newInst);
    inst->addDefUse(newInst, Opnd_implAccSrc);

    // create an implicit source for MACH
    const RegionDesc* rd = NULL;
    unsigned short vs = 0, wd = exec_size, hs = 0;
    if (exec_size > 1) {
        if (isCompressed) {
            wd = wd / 2;
        }
        hs = 1;
        vs = wd;
    }
    rd = builder.createRegionDesc(vs, wd, hs);
    G4_SrcRegRegion* acc_src_opnd = builder.createSrcRegRegion(Mod_src_undef, Direct,
        builder.phyregpool.getAcc0Reg(), 0, 0, rd, tmp_type);

    newInst->setImplAccSrc(acc_src_opnd);

    // set an implicit dst for MACH
    newInst->setImplAccDst(builder.createDstRegRegion(*acc_dst_opnd));

    insertedInst = true;

    if (IS_QTYPE(dst->getType()))
    {
        // we have to produce two additional moves to form the Q/UQ:
        // mul (8) acc0:d r2.0<8;8,1>:d r3.0<16;8,2>:uw
        // mach (8) r5.0<1>:d r2.0<8;8,1>:d r3.0<8;8,1>:d
        // mov (8) r6.0<1>:d acc0:d  // Low 32 bits.
        // mov (8) dst.0<2>:d r6.0<1>:d
        // mov (8) dst.1<2>:d r5.0<1>:d
        // Note that we don't try to combine the moves because of the HW restriction that
        // "If an accumulator is an explicit source operand, its register region must match that of the destination register"

        G4_Declare* low32BitDcl = builder.createTempVar(exec_size, Type_D, Any);
        G4_INST* movInst = builder.createMov(exec_size,
            builder.Create_Dst_Opnd_From_Dcl(low32BitDcl, 1),
            builder.createSrcRegRegion(*acc_src_opnd), inst_opt, false);
        bb->insert(iter, movInst);

        G4_DstRegRegion* origDst = dst;
        bool needsExtraMov = origDst->getHorzStride() > 1 || condmod != NULL || sat_mod;

        G4_Declare* dstAlias = builder.createTempVar(exec_size * 2, Type_D, Any);
        if (!needsExtraMov)
        {
            uint32_t aliasOffset = origDst->getRegOff() * GENX_GRF_REG_SIZ + origDst->getSubRegOff() * 8;
            dstAlias->setAliasDeclare(origDst->getBase()->asRegVar()->getDeclare(), aliasOffset);
        }
        G4_INST* lowMove = builder.createMov(exec_size,
            builder.Create_Dst_Opnd_From_Dcl(dstAlias, 2),
            builder.Create_Src_Opnd_From_Dcl(low32BitDcl, builder.getRegionStride1()),
            inst_opt, false);
        lowMove->setPredicate(pred);

        bb->insert(iter, lowMove);

        MUST_BE_TRUE(high32BitDcl != NULL, "mach dst must not be null");
        G4_INST* highMove = builder.createMov(exec_size,
            builder.createDst(dstAlias->getRegVar(), 0, 1, 2, dstAlias->getElemType()),
            builder.Create_Src_Opnd_From_Dcl(high32BitDcl, builder.getRegionStride1()),
            inst_opt, false);
        highMove->setPredicate(pred);
        bb->insert(iter, highMove);

        if (needsExtraMov)
        {
            // this will take care of non-packed dst/cond mod/saturate
            G4_Declare* dstAliasAsQ = builder.createTempVar(exec_size, Type_Q, Any);
            dstAliasAsQ->setAliasDeclare(dstAlias, 0);
            G4_INST* moveInst = builder.createMov(exec_size, dst, builder.Create_Src_Opnd_From_Dcl(dstAliasAsQ, builder.getRegionStride1()),
                inst_opt, false);
            moveInst->setCondMod(condmod);
            moveInst->setSaturate(sat_mod);
            bb->insert(iter, moveInst);
        }

        return true;
    }

    INST_LIST_ITER last_iter;
    // create a mov inst
    if (sat_mod == false)
    {
        bool extra_mov = dst &&
            dst->getExecTypeSize() > G4_Type_Table[Type_D].byteSize;
        extra_mov |= (isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst));

        G4_INST* movInst = builder.createMov(exec_size, dst, builder.createSrcRegRegion(*acc_src_opnd),
            inst_opt, false);
        movInst->setPredicate(pred);
        movInst->setCondMod(condmod);

        newInst->transferUse(movInst);
        newInst->addDefUse(movInst, Opnd_src0);

        bb->insert(iter, movInst);
        last_iter = iter;
        last_iter--;
        if (extra_mov)
        {
            // add a tmp mov
            iter--;
            G4_DstRegRegion* new_next_dst = insertMovAfter(iter, dst, dst->getType(), bb);
            movInst->setDest(new_next_dst);
            movInst->setPredicate(NULL);
        }
    }
    else
    {
        // create an extra mov inst
        G4_Declare* dcl = builder.createTempVar(
            exec_size,
            tmp_type,
            GRFALIGN);

        G4_DstRegRegion* tmp_dst_opnd = builder.createDst(
            dcl->getRegVar(),
            0,
            0,
            1,
            tmp_type);
        G4_INST* movInst = builder.createMov(exec_size, tmp_dst_opnd,
            builder.createSrcRegRegion(*acc_src_opnd), InstOpt_NoOpt, false);
        movInst->setCondMod(condmod);
        bb->insert(iter, movInst);

        last_iter = iter;
        last_iter--;

        G4_SrcRegRegion* tmp_src_opnd = builder.createSrcRegRegion(Mod_src_undef, Direct, dcl->getRegVar(), 0, 0, rd, tmp_type);

        G4_INST* newInst2 = builder.createInternalInst(pred, G4_mov, condmod, sat_mod, exec_size, dst, tmp_src_opnd, NULL, inst_opt,
            inst->getLineNo(), inst->getCISAOff(), inst->getSrcFilename());

        newInst->transferUse(newInst2);
        newInst->addDefUse(movInst, Opnd_src0);
        movInst->addDefUse(newInst2, Opnd_src0);
        bb->insert(iter, newInst2);
        iter++;
    }

    if (exec_size > builder.getNativeExecSize())
    {
        splitDWMULInst(i, last_iter, bb);
    }

    return insertedInst;
}


// Translate MULH into
// MUL acc src0 src1
// MACH dst src0 src1
void HWConformity::fixMULHInst(INST_LIST_ITER& i, G4_BB* bb)
{
    G4_INST* inst = *i;
    INST_LIST_ITER iter = i;
    uint8_t exec_size = inst->getExecSize();

    int inst_opt = inst->getOption();

    G4_Operand* src0 = inst->getSrc(0), * src1 = inst->getSrc(1);

    if (src0->isImm() && !src1->isImm())
    {
        inst->swapSrc(0, 1);
        src0 = inst->getSrc(0);
        src1 = inst->getSrc(1);
    }

    bool useMulQDD = false;
    if (exec_size <= builder.getNativeExecSize() && !builder.no64bitRegioning() &&
        builder.supportFloatOr64bRegioning())
    {
        useMulQDD = true;
        if (!IS_DTYPE(src0->getType()) || !IS_DTYPE(src1->getType()))
        {
            if (src1->isImm() &&
                IS_DTYPE(src0->getType()) &&
                (IS_WTYPE(src1->getType()) || IS_BTYPE(src1->getType())))
            {
                // Ensure src1 has the same type size as src0.
                const G4_Imm* oldImm = src1->asImm();
                G4_Imm* newImm = builder.createImm(oldImm->getInt(), src0->getType());
                inst->setSrc(newImm, 1);
            }
            else
            {
                useMulQDD = false;
            }
        }
    }
    if (useMulQDD)
    {
        // use mul Q D D to get the upper 32-bit
        // note that we don't do this for CHV/BXT due to the 64-bit type restrictions
        inst->setOpcode(G4_mul);
        G4_DstRegRegion* dst = inst->getDst();
        G4_Type dstType = dst->getType();

        if (dstType == Type_UD)
            dstType = Type_UQ;
        else
            dstType = Type_Q;
        G4_Declare* dstDcl = dst->getBase()->asRegVar()->getDeclare();
        G4_Declare* tmpDcl = builder.createTempVar(
            exec_size,
            dstType,
            Any,
            "TV");
        tmpDcl->copyAlign(dstDcl);

        G4_DstRegRegion* tmpDst = builder.Create_Dst_Opnd_From_Dcl(tmpDcl, 1);
        inst->setDest(tmpDst);

        //need move to cast back to D/UD type
        G4_SrcRegRegion* tmpSrc = builder.createSrcRegRegion(
            Mod_src_undef,
            Direct,
            tmpDcl->getRegVar(),
            0,
            1,
            exec_size > 1 ? builder.getRegionStride2() : builder.getRegionScalar(),
            dst->getType());

        ++iter;

        G4_INST* tmpMov = builder.createMov(exec_size, dst, tmpSrc, inst->getOption(), false);
        tmpMov->setPredicate(builder.duplicateOperand(inst->getPredicate()));

        bb->insert(iter, tmpMov);
        //it will decrement back to mov
        i = iter;

        /*
            Need to remove dst from uses list of mulh, and add them to movInst useList
            add movInst to uselist of mulh.
            Add mulh to def instruction list of movInst
        */
        inst->transferUse(tmpMov);
        inst->addDefUse(tmpMov, Opnd_src0);
        return;
    }

    if (!builder.supportSrcModforMul() &&
        (IS_DTYPE(src0->getType()) || IS_DTYPE(src1->getType())) &&
        ((getTypeSize(src0->getType()) < 4) || (getTypeSize(src1->getType()) < 4)))
    {
        checkSrcMod(i, bb, 0);
        src0 = inst->getSrc(0);
    }

    if (src1->isSrcRegRegion() && src1->asSrcRegRegion()->getModifier() != Mod_src_undef)
    {
        // src1 does not support modifiers
        checkSrcMod(i, bb, 1);
        src1 = inst->getSrc(1);
    }

    G4_Type tmp_type = (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType())) ? Type_UD : Type_D;

    assert(IS_DTYPE(src0->getType()) && "src0 must be DW type");

    G4_DstRegRegion* acc_dst_opnd = builder.createDst(
        builder.phyregpool.getAcc0Reg(),
        0,
        0,
        1,
        tmp_type);
    G4_INST* newMul = builder.createBinOp(G4_mul, exec_size,
        acc_dst_opnd, builder.duplicateOperand(src0), builder.duplicateOperand(src1), inst_opt, false);

    bb->insert(iter, newMul);
    inst->copyDefsTo(newMul, false);
    newMul->addDefUse(inst, Opnd_implAccSrc);

    iter = i;
    iter--;
    fixMulSrc1(iter, bb);

    newMul->setNoMask(true);

    inst->setOpcode(G4_mach);

    if (src1->isImm() && src0->getType() != src1->getType())
    {
        G4_Imm* oldImm = src1->asImm();
        // Ensure src1 has the same type as src0.
        G4_Imm* newImm = builder.createImm(oldImm->getInt(), src0->getType());
        inst->setSrc(newImm, 1);
    }
    else if (!IS_DTYPE(src1->getType()))
    {
        // this can happen due to vISA opt, convert them to src0 type which should be D/UD
        // We use D as the tmp type to make sure we can represent all src1 values
        auto isSrc1NonScalar = inst->getSrc(1)->isSrcRegRegion() && !inst->getSrc(1)->asSrcRegRegion()->isScalar();
        auto newSrc = insertMovBefore(i, 1, Type_D, bb);
        inst->setSrc(builder.createSrcRegRegion(Mod_src_undef, Direct, newSrc->getTopDcl()->getRegVar(), 0, 0,
            isSrc1NonScalar ? builder.getRegionStride1() : builder.getRegionScalar(), src0->getType()), 1);
    }

    //set implicit src/dst for mach
    const RegionDesc* rd = exec_size > 1 ? builder.getRegionStride1() : builder.getRegionScalar();
    G4_SrcRegRegion* acc_src_opnd = builder.createSrcRegRegion(Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, rd, tmp_type);
    inst->setImplAccSrc(acc_src_opnd);
    inst->setImplAccDst(builder.createDstRegRegion(*acc_dst_opnd));

    INST_LIST_ITER end_iter = i;
    // check if the ACC source is aligned to mach dst
    G4_DstRegRegion* dst = inst->getDst();
    if ((inst->getSaturate()) ||
        (dst &&
        ((dst->getExecTypeSize() > G4_Type_Table[Type_D].byteSize) ||
            (isPreAssignedRegOffsetNonZero<G4_DstRegRegion>(dst)))))
    {
        // add a tmp mov
        inst->setDest(insertMovAfter(i, dst, dst->getType(), bb));
        end_iter++;
    }

    inst->setOptionOn(InstOpt_AccWrCtrl);

    if (exec_size > builder.getNativeExecSize())
    {
        auto start_iter = std::prev(i);
        splitDWMULInst(start_iter, end_iter, bb);
        i = end_iter;
    }
}

//
// insert move instructions to copy numDwords dwords from src to dst at the specified location
// a NoMask UD move is used.
// dst and src must be dword-aligned.
// srcOffset and dstOffset are in bytes
// numDwords must be one of {1,2,4,8,16}
// ToDo: may want to generalize this into a copyBytes function that selects the appropriate move type
// based on dst and src type
//
void HWConformity::copyDwords(G4_Declare* dst,
    int dstOffset,
    G4_Declare* src,
    int srcOffset,
    int numDwords,
    G4_BB* bb,
    INST_LIST_ITER iter)
{

    MUST_BE_TRUE(numDwords == 1 || numDwords == 2 || numDwords == 4 ||
        numDwords == 8 || numDwords == 16 || numDwords == 32, "invalid number of dwords to copy");

    G4_Declare* newDst = dst;

    if (dst->getElemType() != Type_UD)
    {
        // create an alias with type UD
        newDst = builder.createTempVar(numDwords, Type_UD, Any);
        newDst->setAliasDeclare(dst, 0);
    }

    G4_Declare* newSrc = src;
    if (src->getElemType() != Type_UD)
    {
        // create an alias with type UD
        newSrc = builder.createTempVar(numDwords, Type_UD, Any);
        newSrc->setAliasDeclare(src, 0);
    }

    G4_SrcRegRegion* srcOpnd = builder.createSrcRegRegion(Mod_src_undef, Direct,
        newSrc->getRegVar(), srcOffset / GENX_GRF_REG_SIZ,
        (srcOffset % GENX_GRF_REG_SIZ) / G4_Type_Table[Type_UD].byteSize,
        builder.getRegionStride1(), Type_UD);
    G4_DstRegRegion* dstOpnd = builder.createDst(newDst->getRegVar(),
        dstOffset / GENX_GRF_REG_SIZ,
        (dstOffset % GENX_GRF_REG_SIZ) / G4_Type_Table[Type_UD].byteSize, 1, Type_UD);

    G4_INST* movInst = builder.createMov((uint8_t)numDwords, dstOpnd, srcOpnd, InstOpt_WriteEnable, false);

    INST_LIST_ITER movPos = bb->insert(iter, movInst);

    if (numDwords == NUM_DWORDS_PER_GRF * 2 &&
        ((dstOffset % GENX_GRF_REG_SIZ) != 0 || (srcOffset % GENX_GRF_REG_SIZ) != 0))
    {
        // move crosses 2 GRF boundary, needs splitting
        evenlySplitInst(movPos, bb);
    }
}

// like the above, but source is an indirect 64-bit source and dst offset is always 0
// If source is Indirect 1x1, we generate
//  mov (esize*2) tmp<1>:ud r[A0]<1;1,0>:ud
//  ...     tmpSrc<region>:q
// If source is VxH indirect, we have to generate instead
//  mov (esize*2) tmp<1>:ud r[A0]<2,1>:ud
//  ...     tmpSrc<1;1,0>:q
// as we can't have the indirect region on the 64-bit type operand
// A0 is not changed otherwise
void HWConformity::copyDwordsIndirect(G4_Declare* dst,
    G4_SrcRegRegion* src,
    int numDwords,
    G4_BB* bb,
    INST_LIST_ITER iter)
{
    MUST_BE_TRUE(G4_Type_Table[dst->getElemType()].byteSize >= 4 &&
        G4_Type_Table[src->getType()].byteSize >= 4, "dst and src must have dword or qword type");

    MUST_BE_TRUE(src->getRegAccess() == IndirGRF, "source must be indirect GRF");

    G4_Declare* newDst = dst;

    if (dst->getElemType() != Type_UD)
    {
        // create an alias with type UD
        newDst = builder.createTempVar(numDwords, Type_UD, Any);
        newDst->setAliasDeclare(dst, 0);
    }

    G4_SrcRegRegion* newSrc = builder.duplicateOperand(src);
    MUST_BE_TRUE(G4_Type_Table[newSrc->getType()].byteSize == 8, "only support 64-bit type source so far");
    newSrc->setType(Type_UD);
    newSrc->setModifier(Mod_src_undef);
    if (newSrc->getRegion()->isRegionWH())
    {
        MUST_BE_TRUE(newSrc->getRegion()->width == 1, "only handle <1,0> region for now");
        newSrc->setRegion(builder.createRegionDesc(UNDEFINED_SHORT, 2, 1));
    }
    else
    {
        newSrc->setRegion(builder.getRegionStride1());
    }

    G4_DstRegRegion* dstOpnd = builder.createDst(newDst->getRegVar(), 0, 0, 1, Type_UD);

    G4_INST* movInst = builder.createMov((uint8_t)numDwords, dstOpnd, newSrc, InstOpt_WriteEnable, false);

    bb->insert(iter, movInst);
}

// copy numRegs GRFs from src[srcOffset] to dst[dstOffset]
// dst[dstOffset] and src[srcOffset] are both GRF-aligned
void HWConformity::copyRegs(G4_Declare* dst,
    int dstOffset,
    G4_Declare* src,
    int srcOffset,
    int numRegs,
    G4_BB* bb,
    INST_LIST_ITER iter)
{
    int numByteCopied = 0;
    for (; numRegs >= 2; numRegs -= 2, numByteCopied += G4_GRF_REG_NBYTES * 2)
    {
        copyDwords(dst, dstOffset + numByteCopied, src, srcOffset + numByteCopied, NUM_DWORDS_PER_GRF * 2, bb, iter);
    }
    if (numRegs != 0)
    {
        copyDwords(dst, dstOffset + numByteCopied, src, srcOffset + numByteCopied, NUM_DWORDS_PER_GRF, bb, iter);
    }
}

bool HWConformity::emulate64bMov(INST_LIST_ITER iter, G4_BB* bb)
{
    auto inst = (*iter);
    auto origIter = iter;
    auto dst = inst->getDst();
    auto src0 = inst->getSrc(0);

    MUST_BE_TRUE(!inst->getCondMod(), "cant handle cond mod");
    auto dstHS = dst->getHorzStride();

    auto incrementVar = [&](G4_Operand* var, unsigned int width, unsigned int regOff, unsigned int sregOff, G4_INST* inst, short increment)
    {
        auto addrDst = builder.createDstRegRegion(Direct, var->getBase(), regOff, sregOff, 1, Type_UW);
        auto addrSrc = builder.createSrcRegRegion(Mod_src_undef, Direct, var->getBase(), regOff, sregOff,
            builder.getRegionStride1(), Type_UW);
        auto incrementImm = builder.createImm(increment, Type_W);
        auto addrAddInst = builder.createInternalInst(nullptr, G4_add, nullptr, false, inst->getExecSize()/width, addrDst, addrSrc, incrementImm, InstOpt_WriteEnable);
        return addrAddInst;
    };

    if (src0->isSrcRegRegion())
    {
        auto src0RR = src0->asSrcRegRegion();
        MUST_BE_TRUE(IS_INT(src0RR->getType()) && IS_INT(dst->getType()), "expecting int types on src, dst");
        MUST_BE_TRUE(src0RR->getModifier() == Mod_src_undef, "cannot handle saturation");

        const RegionDesc* rgnToUse = nullptr;

        if (src0RR->getRegion()->isScalar())
            rgnToUse = builder.getRegionScalar();
        else if(!src0RR->isIndirect())
        {
            uint16_t stride = 0;
            bool legal = src0RR->getRegion()->isSingleStride(inst->getExecSize(), stride);
            MUST_BE_TRUE(legal, "unsupported region");
            if (stride == 1)
                rgnToUse = builder.getRegionStride2();
            else if (stride == 2)
                rgnToUse = builder.getRegionStride4();
            else
                MUST_BE_TRUE(false, "unsupported stride");
        }
        else
        {
            if (G4_Type_Table[src0RR->getType()].byteSize < 8)
                rgnToUse = src0RR->getRegion();
            else
            {
                // this will be broken up in to 2 instructions
                auto factor = G4_Type_Table[src0RR->getType()].byteSize / G4_Type_Table[dst->getType()].byteSize;
                auto vs = src0RR->getRegion()->vertStride * factor;
                auto w = src0RR->getRegion()->width;
                auto hs = src0RR->getRegion()->horzStride * factor;
                rgnToUse = builder.createRegionDesc(vs, w, hs);
            }
        }

        if (G4_Type_Table[dst->getType()].byteSize == 8)
        {
            if (G4_Type_Table[src0->getType()].byteSize == 8)
            {
                // may be q->uq or uq->q or raw mov
                // safe to do raw copy for all 3 cases

                // mov (8) r10.0<1>:uq   r20.0<1;1,0>:uq
                // =>
                // mov (8) r10.0<2>:ud   r20.0<2;1,0>:ud
                // mov (8) r10.1<2>:ud   r20.1<2;1,0>:ud

                // 1st half
                auto newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_UD, dst->getAddrImm())) :
                    (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2, 2 * dstHS, Type_UD));
                auto newSrc = builder.createSrcRegRegion(Mod_src_undef, src0RR->getRegAccess(), src0RR->getBase(), src0RR->getRegOff(),
                    src0RR->isIndirect() ? src0RR->getSubRegOff() : (src0RR->getSubRegOff() * 2), rgnToUse, Type_UD);
                newSrc->setImmAddrOff(src0RR->getAddrImm());
                auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc, inst->getOption(), false);
                newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
                iter = bb->insert(origIter, newInst);

                // second half
                bool dstAddrIncremented = false, src0AddrIncremented = false;
                unsigned int immAddrOff = 4;
                if (dst->isIndirect() && (4 + dst->getAddrImm()) > 512)
                {
                    // increment dst address register by 4, later decrement it
                    dstAddrIncremented = true;
                    immAddrOff = 0;
                    iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, 4));
                }
                newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_UD, immAddrOff + dst->getAddrImm())) :
                    (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2 + 1, 2 * dstHS, Type_UD));
                newSrc = builder.createSrcRegRegion(Mod_src_undef, src0RR->getRegAccess(), src0RR->getBase(), src0RR->getRegOff(),
                    src0RR->isIndirect() ? src0RR->getSubRegOff() : (src0RR->getSubRegOff() * 2 + 1), rgnToUse, Type_UD);
                if (newSrc->isIndirect())
                {
                    // upper 4 bytes
                    if ((4 + src0RR->getAddrImm()) > 512)
                    {
                        src0AddrIncremented = true;
                        iter = bb->insert(origIter, incrementVar(src0RR, src0RR->getRegion()->width, src0RR->getRegOff(), src0RR->getSubRegOff(), inst, 4));
                        newSrc->setImmAddrOff(src0RR->getAddrImm());
                    }
                    else
                        newSrc->setImmAddrOff(4 + src0RR->getAddrImm());
                }
                newInst = builder.createMov(inst->getExecSize(), newDst, newSrc, inst->getOption(), false);
                newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
                iter = bb->insert(origIter, newInst);

                if (dstAddrIncremented)
                {
                    iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, -4));
                }

                if (src0AddrIncremented)
                {
                    iter = bb->insert(origIter, incrementVar(src0RR, src0RR->getRegion()->width, src0RR->getRegOff(), src0RR->getSubRegOff(), inst, -4));
                }

                bb->erase(origIter);

                return true;
            }
            else if (G4_Type_Table[dst->getType()].byteSize == 8 && G4_Type_Table[src0->getType()].byteSize < 8)
            {
                // d/ud/w/uw/b/ub -> q/uq
                if (IS_SIGNED_INT(src0->getType()))
                {
                    // when src is signed, sign extend
                    // b/w/d -> q/uq
                    //
                    // dst<2>.0:d = src:[d|w|b]
                    // dst<2>.1:d = asr dst<2>.0:d 31
                    auto newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_D, dst->getAddrImm())) :
                        (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2, 2 * dstHS, Type_D));
                    auto newSrc = builder.createSrcRegRegion(*src0RR);
                    auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc, inst->getOption(), false);
                    newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
                    iter = bb->insert(origIter, newInst);

                    bool dstAddrIncremented = false;
                    unsigned int immAddrOff = 4;
                    if (dst->isIndirect() && (4 + dst->getAddrImm()) > 512)
                    {
                        // increment dst address register by 4, later decrement it
                        dstAddrIncremented = true;
                        immAddrOff = 0;
                        iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, 4));
                    }

                    newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_D, immAddrOff + dst->getAddrImm())) :
                        (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2 + 1, 2 * dstHS, Type_D));
                    if (dst->isIndirect())
                    {
                        newSrc = builder.createSrcRegRegion(Mod_src_undef, IndirGRF, dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                            rgnToUse, Type_D);
                        newSrc->setImmAddrOff(newDst->getAddrImm());
                    }
                    else
                        newSrc = builder.createSrcRegRegion(Mod_src_undef, Direct, dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2,
                            builder.getRegionStride2(), Type_D);
                    auto imm31 = builder.createImm(31, Type_W);
                    newInst = builder.createBinOp(G4_asr, inst->getExecSize(), newDst, newSrc, imm31, inst->getOption(), false);
                    newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
                    iter = bb->insert(origIter, newInst);

                    if (dstAddrIncremented)
                    {
                        iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, -4));
                    }

                    bb->erase(origIter);

                    return true;
                }
                else
                {
                    // when src is unsigned, zero extend
                    // ub/uw/ud -> q/uq
                    //
                    // dst<2>.0:ud = src:[ud|uw|ub]
                    // dst<2>.1:ud = 0

                    auto newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_UD, dst->getAddrImm())) :
                        (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2, 2 * dstHS, Type_UD));
                    auto newSrc = builder.createSrcRegRegion(*src0RR);
                    auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc, inst->getOption(), false);
                    newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
                    iter = bb->insert(origIter, newInst);

                    bool dstAddrIncremented = false;
                    unsigned int immAddrOff = 4;
                    if (dst->isIndirect() && (4 + dst->getAddrImm()) > 512)
                    {
                        // increment dst address register by 4, later decrement it
                        dstAddrIncremented = true;
                        immAddrOff = 0;
                        iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, 4));
                    }
                    newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_UD, immAddrOff + dst->getAddrImm())) :
                        (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2 + 1, 2 * dstHS, Type_UD));
                    auto imm0 = builder.createImm(0);
                    newInst = builder.createMov(inst->getExecSize(), newDst, imm0, inst->getOption(), false);
                    newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
                    iter = bb->insert(origIter, newInst);

                    if (dstAddrIncremented)
                    {
                        iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, -4));
                    }

                    bb->erase(origIter);

                    return true;
                }
            }
        }
        else if (G4_Type_Table[dst->getType()].byteSize < 8 && G4_Type_Table[src0->getType()].byteSize == 8)
        {
            // truncate
            // q/uq -> d/ud/w/uw/b/ub
            // 1. mov(8) r10.0<1>:d   r20.0<1;1,0>:uq
            // =>
            // mov(8) r10.0<1>:d   r20.0<2;1,0>:d
            //
            // 2. mov(8) r10.0<1>:d   r20.1<2;1,0>:uq
            // =>
            // mov(8) r10.0<1>:d   r20.2<4;1,0>:d

            unsigned int factor = G4_Type_Table[src0->getType()].byteSize / G4_Type_Table[dst->getType()].byteSize;
            auto newDst = builder.createDstRegRegion(*dst);
            auto newSrc = builder.createSrcRegRegion(Mod_src_undef, src0RR->getRegAccess(), src0RR->getBase(), src0RR->getRegOff(),
                src0RR->isIndirect() ? src0RR->getSubRegOff() : (src0RR->getSubRegOff() * factor), rgnToUse, dst->getType());
            newSrc->setImmAddrOff(src0RR->getAddrImm());
            auto newInst = builder.createMov(inst->getExecSize(), newDst, newSrc, inst->getOption(), false);
            newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
            iter = bb->insert(origIter, newInst);

            bb->erase(origIter);

            return true;
        }
    }
    else if (src0->isImm())
    {
        auto imm = src0->asImm()->getInt();
        int low = imm & 0xffffffff;
        int high = (imm >> 32) & 0xffffffff;

        // low
        auto newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_D, dst->getAddrImm())) :
            (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2, 2 * dstHS, Type_D));
        auto immLowSrc = builder.createImm(low, Type_D);
        auto newInst = builder.createMov(inst->getExecSize(), newDst, immLowSrc, inst->getOption(), false);
        newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
        iter = bb->insert(origIter, newInst);

        // high
        bool dstAddrIncremented = false;
        unsigned int immAddrOff = 4;
        if (dst->isIndirect() && (4 + dst->getAddrImm()) > 512)
        {
            // increment dst address register by 4, later decrement it
            dstAddrIncremented = true;
            immAddrOff = 0;
            iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, 4));
        }
        newDst = dst->isIndirect() ? (builder.createIndirectDst(dst->getBase(), dst->getSubRegOff(), 2 * dstHS, Type_D, immAddrOff + dst->getAddrImm())) :
            (builder.createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2 + 1, 2 * dstHS, Type_D));
        auto immHigh = builder.createImm(high, Type_D);
        newInst = builder.createMov(inst->getExecSize(), newDst, immHigh, inst->getOption(), false);
        newInst->setPredicate(inst->getPredicate() ? builder.createPredicate(*inst->getPredicate()) : nullptr);
        iter = bb->insert(origIter, newInst);

        if (dstAddrIncremented)
        {
            iter = bb->insert(origIter, incrementVar(dst, inst->getExecSize(), dst->getRegOff(), dst->getSubRegOff(), inst, -4));
        }

        bb->erase(origIter);

        return true;
    }

    return false;
}

bool HWConformity::fix64bInst(INST_LIST_ITER iter, G4_BB* bb)
{

    // HW restrictions:
    // [DevCHV, DevBXT]: When source or destination datatype is 64b, indirect addressing must not be used.
    // the region rules are:
    // Source and Destination horizontal stride must be aligned to the execution datatype.
    // Example:
    // mov (4) r10.0:df r11.0<16;8,2>:f // Source stride must be 2 since datatype is smaller
    // move (4) r10.0<2>:f r11.0<4;4,1>:df // Destination stride must be 2 since datatype is smaller.
    // as this would require splitting in some cases
    // Regioning must ensure Src.Vstride = Src.Width * Src.Hstride
    // Source and Destination offset must be the same, except the case of scalar source
    // [DevCHV, DevBXT]: When source or destination datatype is 64b, indirect addressing must not be used.
    // [DevCHV, DevBXT]: ARF registers must never be used with 64b datatype.

    if (!builder.no64bitRegioning())
    {
        return false;
    }

    G4_INST* inst = *iter;
    bool uses64BitType = false;
    bool isDWMultiply = false;
    uint8_t execSize = inst->getExecSize();

    if (inst->mayExceedTwoGRF())
    {
        return false;
    }
    if (inst->getDst() && getTypeSize(inst->getDst()->getType()) == 8)
    {
        uses64BitType = true;
    }
    for (int i = 0, size = inst->getNumSrc(); !uses64BitType && i < size; i++)
    {
        G4_Operand* src = inst->getSrc(i);

        if (src && getTypeSize(src->getType()) == 8)
        {
            uses64BitType = true;
        }
    }
    if (inst->opcode() == G4_mul && IS_DTYPE(inst->getSrc(0)->getType()) &&
        IS_DTYPE(inst->getSrc(1)->getType()))
    {
        //WA: dw*dw multiply is considered to use 64bit data type since the result is 64-bit
        uses64BitType = true;
        isDWMultiply = true;
    }

    if (uses64BitType)
    {
        if (builder.noInt64())
        {
            // handle i64 mov/add/cmp/sel
            // ToDo: move it to its own pass
            if (inst->opcode() == G4_mov && IS_INT(inst->getDst()->getType()) && IS_INT(inst->getSrc(0)->getType()))
            {
                if (emulate64bMov(iter, bb))
                    return true;
            }
        }

        int numSrc = inst->getNumSrc();

        // handle indirect sources first
        for (int i = 0; i < numSrc; ++i)
        {
            G4_Operand* src = inst->getSrc(i);
            if (src != nullptr && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF)
            {
                G4_SrcRegRegion* srcAsRegion = src->asSrcRegRegion();
                const RegionDesc* region = srcAsRegion->getRegion();
                int byteSize = G4_Type_Table[srcAsRegion->getType()].byteSize;
                if (byteSize == 8)
                {
                    // right bound is not available for indirect operands
                    // FIXME: this code should be moved to getRightBound()
                    int rightBound = 0;
                    // we must change move type to UD

                    if (region->isScalar())
                    {
                        rightBound = byteSize;
                    }
                    else if (region->isRegionWH())
                    {
                        rightBound = inst->getExecSize() * byteSize;
                    }
                    else
                    {
                        int num_rows = inst->getExecSize() / region->width;
                        rightBound = (num_rows - 1) * region->vertStride * byteSize +
                            region->horzStride * (region->width - 1) * byteSize +
                            byteSize;
                    }

                    int numDwords = rightBound / G4_Type_Table[Type_UD].byteSize;
                    numDwords = Round_Up_Pow2(numDwords);
                    G4_Declare* tmpSrc = builder.createTempVar(numDwords / 2, src->getType(), GRFALIGN);
                    // new source's region varies depending on whether it's VxH or 1x1
                    const RegionDesc* newRegion = region->isRegionWH() ? builder.getRegionStride1() : region;
                    copyDwordsIndirect(tmpSrc, srcAsRegion, numDwords, bb, iter);
                    G4_SrcRegRegion* tmpSrcOpnd = builder.createSrcRegRegion(srcAsRegion->getModifier(),
                        Direct, tmpSrc->getRegVar(), 0, 0, newRegion, tmpSrc->getElemType());
                    inst->setSrc(tmpSrcOpnd, i);
                }
                else
                {
                    // use the good ol' insertMovBefore
                    G4_Operand* tmpSrc = insertMovBefore(iter, i, src->getType(), bb);
                    G4_Declare* tmpSrcDcl = tmpSrc->getTopDcl();
                    tmpSrcDcl->setSubRegAlign(GRFALIGN);
                    inst->setSrc(tmpSrc, i);
                }
            }
        }

        // now handle direct sources with bad region/alignment
        bool hasSameOffset = hasSameSubregOffset(inst);
        for (int i = 0; i < numSrc; i++)
        {
            G4_Operand* src = inst->getSrc(i);
            if (src != NULL && src->isSrcRegRegion())
            {
                G4_SrcRegRegion* srcAsRegion = src->asSrcRegRegion();
                const RegionDesc* region = srcAsRegion->getRegion();
                int byteSize = G4_Type_Table[srcAsRegion->getType()].byteSize;

                if (!isDWMultiply && !region->isScalar() &&
                    (byteSize != 8 && (byteSize * region->horzStride) < 8))
                {
                    // source is not 8 byte aligned
                    // this can happen e.g. for
                    // mov (8) r1.0<1>:df (mod)r3<8;8,1>:f
                    // which we'd need to change to
                    // mov (8) r10.0<2>:f (mod)r3.0<8;8,1>:f
                    // mov (8) r1.0<1>:df r10.0<8;4,2>:f
                    // to satisfy rule 1
                    uint8_t exSize = inst->getExecSize();
                    uint16_t multFactor = (uint16_t)(8 / byteSize);
                    G4_Type tmpType = srcAsRegion->getType();
                    if (multFactor == 8)
                    {
                        // byte type needs special handling since we can't have stride 8
                        tmpType = (tmpType == Type_B) ? Type_W : Type_UW;
                        multFactor = 4;
                    }
                    MUST_BE_TRUE(multFactor != 8, "does not support 64b operation with byte source");
                    G4_Declare* tmp = builder.createTempVar(exSize * multFactor,
                        tmpType, GRFALIGN);
                    G4_DstRegRegion* tmpDst = builder.Create_Dst_Opnd_From_Dcl(tmp, multFactor);
                    G4_INST* movInst = builder.createMov(inst->getExecSize(), tmpDst, src, inst->getOption(), false);
                    bb->insert(iter, movInst);
                    uint16_t width = exSize;
                    if (width * 8 > GENX_GRF_REG_SIZ)
                    {
                        // can't have width cross GRF
                        width = 4;
                    }
                    G4_SrcRegRegion* newSrc = builder.Create_Src_Opnd_From_Dcl(tmp,
                        builder.createRegionDesc((uint16_t)multFactor * width, width, multFactor));
                    inst->setSrc(newSrc, i);
                }
                else if (region->isScalar())
                {
#if 0
                    // scalar region still must be aligned to qword, though it can be any qword
                    if (byteSize < 8 && !builder.isOpndAligned(srcAsRegion, 8))
                    {
                        G4_Operand* tmpSrc = insertCopyBefore(iter, i, Four_Word, bb);
                        inst->setSrc(tmpSrc, i);
                    }
#endif
                }
                else if (!hasSameOffset)
                {
                    // we need a temp src that is GRF-aligned
                    if (byteSize == 8)
                    {
                        // the same src/dst offset restriction applies to move as well, so we have to generate
                        // a packed move with UD type to work around the restriction
                        // e.g., for
                        // add (2) ... r1.1<4;2,2>:q
                        // we turn it into
                        // mov (8) r10.0<1>:ud r1.2<1;1,0>:ud {NoMask}
                        // add (2) ... r10.0<4;2,2>:q
                        int numDwords = (src->getRightBound() - src->getLeftBound() + 1) / G4_Type_Table[Type_UD].byteSize;
                        numDwords = Round_Up_Pow2(numDwords);
                        G4_Declare* tmpSrc = builder.createTempVar(numDwords / 2, src->getType(), GRFALIGN);
                        copyDwords(tmpSrc, 0, src->getTopDcl(), src->getLeftBound(), numDwords, bb, iter);
                        G4_SrcRegRegion* tmpSrcOpnd = builder.createSrcRegRegion(srcAsRegion->getModifier(),
                            Direct, tmpSrc->getRegVar(), 0, 0, srcAsRegion->getRegion(), tmpSrc->getElemType());
                        inst->setSrc(tmpSrcOpnd, i);
                    }
                    else
                    {
                        // use the good ol' insertMovBefore
                        G4_Operand* tmpSrc = insertMovBefore(iter, i, src->getType(), bb);
                        G4_Declare* tmpSrcDcl = tmpSrc->getTopDcl();
                        tmpSrcDcl->setSubRegAlign(GRFALIGN);
                        inst->setSrc(tmpSrc, i);
                    }
                }
                    }
                }

        for (int i = 0; i < numSrc; i++)
        {
            // rewrite <1;1,0> to <2;2,1> since HW does not like the former
            G4_Operand* src = inst->getSrc(i);
            if (src != nullptr && src->isSrcRegRegion())
            {
                G4_SrcRegRegion* srcAsRegion = src->asSrcRegRegion();
                const RegionDesc* region = srcAsRegion->getRegion();
                if (!region->isRegionWH() && region->vertStride != region->horzStride * region->width)
                {
                    // see if we can fix the region to satisfy VS = W * HS
                    if (region->width == inst->getExecSize())
                    {
                        // vs is a don't care, change to <w*hs, w, hz>
                        srcAsRegion->setRegion(builder.createRegionDesc(region->width * region->horzStride, region->width, region->horzStride));
                    }
                    else if (region->width == 1)
                    {
                        // hs is a don't care, change it to <esize*vs, esize, vs>
                        MUST_BE_TRUE(region->vertStride <= 4, "illegal vertical stride");

                        uint16_t wd = inst->getExecSize();
                        uint16_t hs = region->vertStride;
                        if (src->crossGRF())
                        {
                            // Make sure the new hs does not cross GRF
                            uint32_t nbytesIn1stGRF = GENX_GRF_REG_SIZ - (src->getLeftBound() % GENX_GRF_REG_SIZ);
                            uint32_t eltBytes = G4_Type_Table[srcAsRegion->getType()].byteSize;
                            uint32_t neltsIn1stGRF = nbytesIn1stGRF / eltBytes;

                            MUST_BE_TRUE((nbytesIn1stGRF % eltBytes) == 0, "Bad region with element crossing GRF");
                            MUST_BE_TRUE((neltsIn1stGRF % hs) == 0, "hs cannot cross GRF");

                            wd = neltsIn1stGRF / hs;
                            // Get the largest powOfTwo that can divide wd
                            wd = wd & (-wd);
                            //MUST_BE_TRUE( wd > 1, "Cannot select non-1 width w/o crossing GRF");
                        }
                        srcAsRegion->setRegion(builder.createRegionDesc(wd * hs, wd, hs));
                    }

                    else
                    {
                        // FIXME: Both VS and HS are used by the region, so we have to either split inst or insert multiple moves to pack the source
                        // both are painful, so we assert for now and fix later if we encounter such a case
                        MUST_BE_TRUE(false, "Unhandled bad 64b region on CHV/BXT");
                    }

                }
            }
        }
        G4_DstRegRegion* dst = inst->getDst();
        if (dst != NULL && !dst->isNullReg())
        {
            bool needsTmpDst = dst->getRegAccess() != Direct ||
                (execSize > 1 && !hasSameOffset) ||
                dst->isAreg();
            if (needsTmpDst)
            {
                // we need to have a temp dst that is direct and GRF-aligned
                if (dst->getRegAccess() == Direct && G4_Type_Table[dst->getType()].byteSize == 8)
                {
                    // the same src/dst offset restriction applies to move as well, so we have to generate
                    // a move with UD type to work around the restriction
                    // e.g., for
                    // add (2) r1.2<1>:q ...
                    // we generate
                    // add (2) r3.0<1>:q ...
                    // mov (4) r1.4<1>:ud r3.0<1;1,0>:ud {NoMask}
                    // If dst is not contiguous, we additionally add a move to pre-load the old values:
                    // add (2) r1.2<2>:q ...
                    // becomes
                    // mov (8) r3.0<1>:ud r1.4<1;1,0>:ud {NoMask}
                    // add (2) r3.0<2>:q ...
                    // mov (8) r1.4<1>:ud r3.0<1;1,0>:ud {NoMask}
                    int numDwords = (dst->getRightBound() - dst->getLeftBound() + 1) / G4_Type_Table[Type_UD].byteSize;
                    numDwords = Round_Up_Pow2(numDwords);
                    G4_Declare* tmpDst = builder.createTempVar(numDwords / 2, dst->getType(), GRFALIGN);
                    if (numDwords > execSize * 2)
                    {
                        // dst is not packed, need a move to pre-load the dst value into tmp
                        copyDwords(tmpDst, 0, dst->getTopDcl(), dst->getLeftBound(), numDwords, bb, iter);
                    }
                    INST_LIST_ITER next = iter;
                    ++next;
                    copyDwords(dst->getTopDcl(), dst->getLeftBound(), tmpDst, 0, numDwords, bb, next);
                    inst->setDest(builder.Create_Dst_Opnd_From_Dcl(tmpDst, dst->getHorzStride()));
                }
                else
                {
                    // use the good ol' insertMoveAfter
                    G4_DstRegRegion* tmpDst = insertMovAfter(iter, dst, dst->getType(), bb);
                    G4_Declare* tmpDstDcl = tmpDst->getTopDcl();
                    tmpDstDcl->setSubRegAlign(GRFALIGN);
                    inst->setDest(tmpDst);
                    if (G4_Type_Table[dst->getType()].byteSize == 8)
                    {
                        // tmpDst is indirect and thus still does not conform
                        // we rewrite
                        // mov (e) r[a0.0]<1>:q src<1;1,0>:q
                        // into
                        // mov (e*2) r[a0.0]<1>:ud src<1;1,0>:ud {NoMask}
                        ++iter;
                        G4_INST* movInst = *iter;
                        MUST_BE_TRUE(movInst->opcode() == G4_mov && movInst->getDst() == dst &&
                            movInst->getSrc(0)->isSrcRegRegion(),
                            "unexpected instruction created by insertMovAfter");
                        MUST_BE_TRUE(dst->getHorzStride() == 1, "only stride 1 is supported for now");
                        dst->setType(Type_UD);
                        G4_SrcRegRegion* src = movInst->getSrc(0)->asSrcRegRegion();
                        G4_Declare* tmpAsUD = builder.createTempVar(tmpDstDcl->getNumElems() * 2, Type_UD, Any);
                        tmpAsUD->setAliasDeclare(tmpDstDcl, 0);
                        const RegionDesc* newRegion = src->getRegion()->isScalar() ?
                            builder.createRegionDesc(0, 2, 1) : builder.getRegionStride1();
                        G4_SrcRegRegion* srcAsUD = builder.createSrcRegRegion(src->getModifier(),
                            src->getRegAccess(), tmpAsUD->getRegVar(), src->getRegOff(),
                            src->getSubRegOff() * 2, newRegion, tmpAsUD->getElemType());
                        movInst->setSrc(srcAsUD, 0);
                        movInst->setExecSize(inst->getExecSize() * 2);

                        // NoMask is set on the mov instruction, but if we fall outside of the new execution size,
                        // it won't be executed fully
                        // e.g., we have to change
                        // (W) mov (16|M24) r[a0.0,64]<1>:ud r67.0<8;8,1>:ud
                        // into
                        // (W) mov (16|M0) r[a0.0,64]<1>:ud r67.0<8;8,1>:ud
                        movInst->setMaskOption(InstOpt_M0);


                        // mov saturate/pred to the original inst
                        movInst->setOptionOn(InstOpt_WriteEnable);
                        if (movInst->getSaturate())
                        {
                            movInst->setSaturate(false);
                            inst->setSaturate(true);
                        }
                        G4_Predicate* pred = movInst->getPredicate();
                        if (pred)
                        {
                            MUST_BE_TRUE(inst->getPredicate() == nullptr, "both inst and movInst have predicates");
                            movInst->setPredicate(nullptr);
                            inst->setPredicate(pred);
                        }
                    }
                }
            }
        }
            }
            return false;
        }

//------------------------------------------------------------------------------
//
//  For BDW, 32 bits integer multiply is implemented as the following macro
//
//  mul (8) acc0:d     r2.0<8;8,1>d   r3.0<16;8,2>:uw
//  mach (8) rTemp<1>:d r2.0<8;8,1>d   r3.0<8;8,1>:d
//  mov (8) r5.0<1>:d   rTemp:d // hi-32bits
//  mov (8) r6.0<1>:d acc0:d // lo-32bits
//
//  Note that this only changes the mul instruction's src1, mach and mov is generated elsewhere
//------------------------------------------------------------------------------
void HWConformity::fixMulSrc1(INST_LIST_ITER i, G4_BB* bb)
{
    G4_INST* inst = *i;
    G4_Operand* src1 = inst->getSrc(1);

    if (!IS_DTYPE(src1->getType()))
    {
        // this could happen if dst is Q
        return;
    }

    if (src1->isImm())
    {
        uint64_t truncVal = src1->asImm()->getImm() & 0xFFFF;
        G4_Imm* new_src1 = builder.createImm(truncVal, Type_UW);
        inst->setSrc(new_src1, 1);
    }
    else
    {
        assert(src1->isSrcRegRegion() && "region expected");
        G4_SrcRegRegion* srcRegion = src1->asSrcRegRegion();
        const RegionDesc* rd = srcRegion->getRegion();
        if (rd->horzStride >= 4)
        {
            G4_Operand* new_src1 = insertMovBefore(i, 1, Type_UW, bb);
            inst->setSrc(new_src1, 1);
        }
        else
        {
            // create a new opnd with type UW
            unsigned short scale = G4_Type_Table[Type_D].byteSize / G4_Type_Table[Type_UW].byteSize;
            unsigned short newHS = rd->horzStride * scale;
            unsigned short newVS = rd->vertStride * scale;
            const RegionDesc* new_rd = builder.createRegionDesc(newVS, rd->width, newHS);
            short subRegOff = srcRegion->getSubRegOff();
            if (srcRegion->getRegAccess() == Direct)
            {
                subRegOff *= scale;
            }
            auto new_src1 = builder.createSrcRegRegion(
                srcRegion->getModifier(), srcRegion->getRegAccess(),
                srcRegion->getBase(), srcRegion->getRegOff(), subRegOff, new_rd,
                Type_UW);
            inst->setSrc(new_src1, 1);
            if (srcRegion->getRegAccess() != Direct)
            {
                new_src1->setImmAddrOff(srcRegion->getAddrImm());
            }
        }
    }

    G4_Operand* src0 = inst->getSrc(0);
    if (!builder.supportSrcModforMul() && IS_DTYPE(src0->getType()))
    {
        checkSrcMod(i, bb, 0);
        checkSrcMod(i, bb, 1);
    }
}

/*
 *  only acc0 may be used in DWord operations, so we have to break a
 *  SIMD16 DWord multiply into two mul-mach-mov sequences.
 *
 *  Input:
 *  (f0) mul (16) dst:d  src0:d  src1:d
 *
 *  Output:
 *  mul (8) acc0:d  src0:d  src1:d
 *  mach    (8) null:d  src0:d  src1:d
 *  (f0) mov (8) dst:d acc0:d
 *  mul (8) acc0:d  src0+1:d  src1+1:d
 *  mach    (8) null:d  src0+1:d    src1+1:d
 *  (f1) mov (8) dst+1:d acc0:d
 *
 */
void HWConformity::splitDWMULInst(INST_LIST_ITER& start, INST_LIST_ITER& end, G4_BB* bb)
{
    // split simd16 inst into SIMD8 ones, since D is not supported for acc1
    INST_LIST_ITER iter = start, last_iter = end;
    //iter--;
    last_iter++;
    INST_LIST_ITER curr_iter;
    while (iter != end)
    {
        curr_iter = iter;
        evenlySplitInst(curr_iter, bb);
        // curr_iter points to the second half after instruction splitting
        G4_INST* expand_sec_half_op = *curr_iter;
        iter++;

        bb->insert(last_iter, expand_sec_half_op);
        if (curr_iter == start)
        {
            start--;
        }
        bb->erase(curr_iter);
    }
    // handle the last inst
    if (iter == end)
    {
        evenlySplitInst(iter, bb);
        G4_INST* expand_sec_half_op = *iter;
        bb->insert(last_iter, expand_sec_half_op);
        end--;
        bb->erase(iter);
    }
}

static bool isGoodMadType(G4_Type type)
{
    switch (type)
    {
    case Type_F:
    case Type_HF:
    case Type_DF:
        return true;
    default:
        return false;
    }
}

bool HWConformity::isGoodAlign1TernaryDst(G4_INST* inst) const
{
    // Align1 MAD requirements:
    // -- dst must be direct GRF/acc with horizontal stride 1 or 2
    G4_Type execType = inst->getExecType();
    G4_DstRegRegion* dst = inst->getDst();

    MUST_BE_TRUE(!IS_QTYPE(dst->getType()) && !IS_BTYPE(dst->getType()), "3Src inst don't support Q and B dst types");

    if (!builder.hasMixMode() &&
        isLowPrecisionFloatTy(dst->getType()) && !isLowPrecisionFloatTy(execType))
    {
        return false;
    }

    int alignInBytes = 8;
    // if src2 is not a scalar, then align it to 32 bytes.
    if (builder.noSrc2Regioning())
    {
        unsigned src2Pos = inst->opcode() == G4_pseudo_mad ? 0 : 2;
        auto src2 = inst->getSrc(src2Pos);
        if (src2->isSrcRegRegion() && !src2->asSrcRegRegion()->isScalar())
        {
            alignInBytes = 32;
        }
    }

    if (!builder.isOpndAligned(dst, alignInBytes))
    {
        // dst must be 8 byte aligned due to encoding issues
        return false;
    }

    uint32_t effectiveStride = dst->getHorzStride();
    if (G4_Type_Table[dst->getType()].byteSize < G4_Type_Table[execType].byteSize)
    {
        if (IS_TYPE_INT(dst->getType()))
        {
            effectiveStride *= G4_Type_Table[execType].byteSize / G4_Type_Table[dst->getType()].byteSize;
        }
        else
        {
            // we have mixed HF and F inst
            // dst can be packed HF, but then it must be oword aligned
            // this should be checked later for mixed mode inst
        }
    }

    return dst->getRegAccess() == Direct && effectiveStride <= 2;
}

//
// check for legal align1 ternary inst sources
//
bool HWConformity::isGoodAlign1TernarySrc(G4_INST* inst, int srcPos, bool canBeImm)
{
    MUST_BE_TRUE(srcPos >= 0 && srcPos < 3, "illegal source pos");

    uint8_t execSize = inst->getExecSize();
    G4_Operand* src = inst->getSrc(srcPos);
    // for pseudo_mad we have to swap src0 and src2
    bool isSrc2 = inst->opcode() == G4_pseudo_mad ? srcPos == 0 : srcPos == 2;

    if (!builder.hasMixMode())
    {
        G4_Type execType = inst->getExecType();
        if (isLowPrecisionFloatTy(src->getType()) && !isLowPrecisionFloatTy(execType))
        {
            return false;
        }
    }

    if (IS_QTYPE(src->getType()))
    {
        return false;
    }

    // mad specific checks
    if (inst->opcode() == G4_pseudo_mad)
    {
        if (isSrc2)
        {
            if (IS_DTYPE(src->getType()))
            {
                return false;
            }

            if (builder.noSrc2Regioning() && IS_BTYPE(src->getType()))
            {
                return false;
            }
        }
        else if (srcPos == 1)
        {
            if (IS_DTYPE(src->getType()) && src->isSrcRegRegion() &&
                src->asSrcRegRegion()->getModifier() != Mod_src_undef)
            {
                // no source modifier for DW multiply
                return false;
            }
        }
    }

    if (src->isImm())
    {
        // either src0 or src2 can be 16b imm, but not both
        // permanent WA: simd16 inst can't have src0 imm.
        // Instead of splitting, we just add a move

        if (canBeImm && (srcPos == 0 || srcPos == 2) && G4_Type_Table[src->getType()].byteSize <= 2)
        {
            if (VISA_WA_CHECK(builder.getPWaTable(), WaNoSimd16TernarySrc0Imm))
            {
                return !isSrc2 && inst->getExecSize() == 16 ? false : true;
            }
            return true;
        }
        return false;
    }
    else if (src->isSrcRegRegion())
    {
        if (src->asSrcRegRegion()->getRegAccess() != Direct)
        {
            return false;
        }

        auto checkSingleStrideRegion = [](G4_SrcRegRegion* src, int stride, uint8_t execSize, IR_Builder& builder)
        {
            const RegionDesc* srcRegion = src->getRegion();

            if (stride > 4)
            {
                return false;
            }
            else if (srcRegion->isContiguous(execSize))
            {
                // Normalize the region if it is not.
                if (srcRegion->width != 1)
                {
                    src->setRegion(builder.getRegionStride1(), /*invariant*/ true);
                }
                if (!builder.encodeUnitStrideTernary())
                {
                    // we have to make sure width is not being used to cross GRF, as <1;1,0>
                    // is not a legal region for align1 ternary source (vs 1 not supported)
                    // mad doesn't support <1;1,0>, the width is at least 2
                    int minAlignment = G4_Type_Table[src->getType()].byteSize * 2;
                    return builder.isOpndAligned(src, minAlignment);
                }
            }
            return true;
        };

        // the following regions are supported:
        // <N;N,0>
        // <0;1,0>
        // <W*H;W,H>
        const RegionDesc* srcRegion = src->asSrcRegRegion()->getRegion();
        if (srcRegion->isScalar())
        {
            return true;
        }

        // src0 and src1 (for psuedo-mad, it's src1 and src2) may use the <N;N,0> region
        // as they come with a vStride in encoding
        // TODO: we may consider swapping src1 and src2 to catch more regions
        if (!isSrc2)
        {
            uint16_t stride = 0;
            if (srcRegion->isSingleStride(execSize, stride))
            {
                return checkSingleStrideRegion(src->asSrcRegRegion(), stride, execSize, builder);
            }

            if (builder.encodeUnitStrideTernary())
            {
                // <4;4,0> and <8;8,0> are ok
                return srcRegion->vertStride == srcRegion->width &&
                    srcRegion->horzStride == 0 &&
                    (srcRegion->width == 4 || srcRegion->width == 8);
            }
            else
            {
                // <2;2,0>, <4;4,0> and <8;8,0> are ok
                return srcRegion->vertStride == srcRegion->width &&
                    srcRegion->horzStride == 0 &&
                    srcRegion->width <= 8;
            }
        }
        else
        {
            if (!builder.noSrc2Regioning())
            {
                // src2 (src0 for pseudo-mad) is without vstride, and its region must be
                // <esize*H;esize,H>, with vstride derived from exSize and hstride
                uint16_t stride = 0;
                if (srcRegion->isSingleStride(execSize, stride))
                {
                    return checkSingleStrideRegion(src->asSrcRegRegion(), stride, execSize, builder);
                }
            }
            else
            {
                // not a scalar, src2 must be GRF aligned.
                if (!builder.isOpndAligned(src, G4_GRF_REG_NBYTES))
                {
                    return false;
                }

                uint16_t stride = 0;
                if (srcRegion->isSingleStride(execSize, stride))
                {
                    unsigned short dstExecSize = inst->getDst()->getExecTypeSize();
                    unsigned short srcExecSize = stride * src->asSrcRegRegion()->getElemSize();
                    // Source 2 and destination stride must be aligned to the same execution type.
                    // E.g. mad (4) r10.0<1>:hf src0 src1 r13.0<1>:hf
                    //      mad (4) r10.0<2>:hf src0 src1 r13.0<1>:f
                    //      mad (4) r10.0<1>:f  src0 src1 r13.0<2>:hf
                    // this rule is relaxed if mix mode is enabled (packed HF ok)
                    if (dstExecSize == srcExecSize)
                    {
                        return true;
                    }
                    if (builder.hasPartialMixMode() && inst->isMixedMode())
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }

    return true;
}

//
// a source is good for align16 if:
// -- it is a direct srcRegRegion
// -- it has contiguous region and can be made either GRF-aligned (for exec size >= 8)
//    or oword aligned (for exec size == 4)
// -- or it has scalar region and is not non-simd1 double
bool HWConformity::isGoodAlign16Src(G4_INST* inst, int srcPos)
{
    MUST_BE_TRUE(srcPos >= 0 && srcPos < 3, "illegal source pos");

    uint8_t execSize = inst->getExecSize();
    G4_Operand* src = inst->getSrc(srcPos);
    G4_Type opnd_type = src->getType();

    // Constants are not allowed as MAD opnds.
    if (src->isSrcRegRegion())
    {
        const RegionDesc* region = src->asSrcRegRegion()->getRegion();
        G4_RegAccess regAcc = src->asSrcRegRegion()->getRegAccess();

        if (regAcc != Direct)
        {
            return false;
        }

        if (region->isContiguous(execSize))
        {
            if (builder.getPlatform() == GENX_BDW && getTypeSize(opnd_type) < 4)
            {
                // BDW HF has to be 32-byte aligned
                if (!builder.isOpndAligned(src, 32))
                {
                    return false;
                }
            }
            else
            {
                if (execSize >= 8)
                {
                    // operand must be GRF aligned, or oword aligned for HF/W
                    uint32_t align = std::min<uint32_t>(execSize * getTypeSize(src->getType()), 32);
                    if (!builder.isOpndAligned(src, align))
                    {
                        return false;
                    }
                }
                else if (execSize == 4 || execSize == 2)
                {
                    // operand must be oword-aligned
                    if (!builder.isOpndAligned(src, 16))
                    {
                        return false;
                    }
                }
            }
        }
        else if (src->asSrcRegRegion()->isScalar())
        {
            if (opnd_type == Type_DF && execSize != 1)
            {
                // scalar region is illegal for DF since replicate is not supported
                return false;
            }

            if (opnd_type == Type_HF && builder.getPlatform() == GENX_BDW)
            {
                return false;
            }
        }
        else
        {
            // all other regions are illegal
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }

}

//
// Move modifiers of src2 in pseudo_mad to its defining instruction.
//
// mul (16) V66(0,0)<1>:d V46(23,0)<16;16,1>:w 0x39db:w {Align1, H1}
// psuedo_mad (16) V67(0,0)<1>:d V469,0)<8;8,1>:w 0x1b5d:w -V66(0,0)<16;16,1>:d
//
// becomes
//
// mul (16) V66(0,0)<1>:d -V46(23,0)<16;16,1>:w 0x39db:w {Align1, H1}
// psuedo_mad (16) V67(0,0)<1>:d V469,0)<8;8,1>:w 0x1b5d:w V66(0,0)<16;16,1>:d
//
static void tryTransferSrcModifier(IR_Builder& builder, G4_INST* def,
    G4_Operand* src)
{
    // Only when def has no other users.
    if (!def->hasOneUse())
        return;

    // Only transfer for integer types.
    if (!IS_SIGNED_INT(src->getType()))
        return;

    // In case the use type is different from the def type.
    if (!def->getDst() || (def->getDst()->getType() != src->getType()))
        return;

    switch (def->opcode()) {
    default:
        break;

        // Probably this is the only interesting op, since G4_math will not be
        // used to generate mac.
    case G4_mul:
    {
        // Chances are src1 is an immediate.
        G4_Operand* defSrc1 = def->getSrc(1);
        if (!IS_SIGNED_INT(defSrc1->getType()))
            return;

        if (defSrc1->isImm())
        {
            G4_Imm* val = defSrc1->asImm();
            // Mod_Minus is assumed.
            G4_Imm* newVal = builder.createImm(-val->getInt(), val->getType());
            def->setSrc(newVal, 1);
            src->asSrcRegRegion()->setModifier(Mod_src_undef);
        }
        else if (defSrc1->isSrcRegRegion())
        {
            G4_SrcRegRegion* reg = defSrc1->asSrcRegRegion();
            if (reg->getModifier() == Mod_src_undef)
            {
                reg->setModifier(src->asSrcRegRegion()->getModifier());
                src->asSrcRegRegion()->setModifier(Mod_src_undef);
            }
            else if (reg->getModifier() == Mod_Minus)
            {
                reg->setModifier(Mod_src_undef);
                src->asSrcRegRegion()->setModifier(Mod_src_undef);
            }
        }
    } break;
    }
}

// Try to move source modifiers on MAD's src2 into its defintion. This allows
// pseudo_mad ops to be translated into mac ops.
void HWConformity::tryEliminateMadSrcModifier(IR_Builder& builder, G4_INST* inst)
{
    ASSERT_USER(inst->opcode() == G4_pseudo_mad, "not a speudo-mad");

    // For pseudo_mad, src2 is the major source operand to be examined later.
    // If there is no modifier on src2, then nothing to do.
    G4_Operand* src2 = inst->getSrc(2);
    if (!src2->isSrcRegRegion())
        return;

    // Currently, only handle modifier minus. To handle others, we may need
    // to insert extra instructions.
    if (src2->asSrcRegRegion()->getModifier() != Mod_Minus)
        return;

    // Only when src2 has a single definition.
    if (G4_INST* def = inst->getSingleDef(Opnd_src2, true))
    {
        tryTransferSrcModifier(builder, def, src2);
    }
}

/// Heuristic to decide whether this fp pseudo-mad should be lowered into a
/// GEN mad or not. Returns true if mad is preferred, false otherwise.
///
/// We flavor generating non-mad when this vISA mad is part of b2b mads that
/// share the same dst.
///
bool HWConformity::isFpMadPreferred(G4_BB* bb, INST_LIST_ITER iter)
{
    G4_INST* inst = *iter;
    G4_Operand* dst = inst->getDst();
    MUST_BE_TRUE(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");

    // Check whether test_inst is sharing the same dst.
    auto equal_mad_dst = [](G4_INST* test_inst, G4_Operand* dst)
    {
        if (test_inst->opcode() == G4_pseudo_mad)
        {
            G4_Operand* test_dst = test_inst->getDst();
            if (test_dst->compareOperand(dst) == Rel_eq)
                return true;
        }
        return false;
    };

    auto next_iter = std::next(iter);
    if (next_iter != bb->end())
    {
        G4_INST* next_inst = *next_iter;
        if (equal_mad_dst(next_inst, dst))
            return false;
    }
    if (iter != bb->begin())
    {
        auto prev_iter = std::prev(iter);
        G4_INST* prev_inst = *prev_iter;
        if (equal_mad_dst(prev_inst, dst))
            return false;
    }

    // FIXME: remove possile duplicate calls to isGoodAlign16Src, Cm only.
    // This will go away if we use an extra opcode to represent muladd.
    unsigned extraMov = 0;
    for (int k = 0; k < inst->getNumSrc(); k++)
    {
        if (!isGoodAlign16Src(inst, k))
        {
            // If need to insert >1 number of moves, then do not use mad.
            if (++extraMov > 1)
                return false;
        }
    }

    return true;
}

// generate align1 mad, inserting moves if necessary
// returns true if conversion is successful
// for floating point mad this must succeed due to precision requirements
bool HWConformity::generateAlign1Mad(G4_BB* bb, INST_LIST_ITER iter)
{

    G4_INST* inst = *iter;
    MUST_BE_TRUE(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");
    bool mustDoMad = IS_TYPE_FLOAT_ALL(inst->getDst()->getType());

    if (!isGoodAlign1TernaryDst(inst))
    {
        if (mustDoMad)
        {
            auto alignment = builder.noSrc2Regioning() ? GRFALIGN : Four_Word;
            inst->setDest(insertMovAfter(iter, inst->getDst(), inst->getDst()->getType(), bb, alignment));
        }
        else
        {
            return false;
        }
    }

    // try swapping src0 (really src2) and src1 to see if we can save a move
    // some conditions where swap may help:
    // -- if src0 is D, as MAD only supports D + D * W
    // -- if src1 is imm, as MAD src2 supports 16-bit imm
    // -- if src0 is HF in a mix mode MAD, as MAD src1 supports HF
    // -- if src1 is scalar, as MAD src2 has more region restrictions
    {
        G4_Operand* src0 = inst->getSrc(0);
        G4_Operand* src1 = inst->getSrc(1);
        if (IS_DTYPE(src0->getType()) && src0->isSrcRegRegion() && !IS_DTYPE(src1->getType()))
        {
            inst->swapSrc(0, 1);
        }
        else if (src1->isImm() && getTypeSize(src1->getType()) == 2)
        {
            //swap src0 and src1 as src0 supports imm
            inst->swapSrc(0, 1);
        }
        else if (!isGoodAlign1TernarySrc(inst, 0, true) &&
            src1->isSrcRegRegion() &&
            src1->asSrcRegRegion()->isScalar())
        {
            // Swap src0 and src1 if src1 is scalar but src0 is not a good Align1TernarySrc
            // when src2 regioning support is quite limited.
            inst->swapSrc(0, 1);
        }
        else if (isLowPrecisionFloatTy(src0->getType()) && src1->getType() == Type_F)
        {
            inst->swapSrc(0, 1);
        }
    }

    // check src
    bool canBeImm = true;
    for (int k = 0; k < inst->getNumSrc(); k++)
    {
        G4_Operand* src = inst->getSrc(k);
        if (!isGoodAlign1TernarySrc(inst, k, canBeImm))
        {
            if (mustDoMad)
            {
                bool isSrc2 = (k == 0);
                if (builder.noSrc2Regioning() && isSrc2)
                {
                    fixSrc2(iter, bb, true);
                }
                else
                {
                    inst->setSrc(insertMovBefore(iter, k, src->getType(), bb), k);
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (src->isImm())
            {
                canBeImm = false;
            }
        }
    }

    inst->setOpcode(G4_mad);
    //swap src0 and src2 (vISA MAD is src0*src1+src2, while GEN MAD is src1*src2+src0)
    inst->swapSrc(0, 2);

    return true;
}

// convert a FP (HF/F/DF) pseudo-mad into a GEN mad,
// inserting moves if necessary
// returns true if conversion is successful
// note that this must return true for IGC due to precision requirements
bool HWConformity::generateFPMad(G4_BB* bb, INST_LIST_ITER iter)
{
    G4_INST* inst = *iter;
    MUST_BE_TRUE(inst->opcode() == G4_pseudo_mad, "expect pseudo mad");
    uint8_t execSize = inst->getExecSize();
    G4_DstRegRegion* dst = inst->getDst();

    // Align16 MAD requirements:
    // -- dst and all 3 srcs have the same F/HF/DF type (mixed F/HF is allowed on CHV+)
    // -- dst and all 3 srcs have direct access
    // -- execution size is 16/8/4/1
    // -- dst and src must be packed
    // -- if src region is not scalar, its subregister must be 16 byte aligned

    // do not force fma for CM since it doesn't have precision requirements
    bool preferFpMad = builder.getOption(vISA_forceFPMAD) || builder.favorFpMad();
    if (!preferFpMad)
    {
        preferFpMad = isFpMadPreferred(bb, iter);
    }

    auto alignMent = execSize * G4_Type_Table[dst->getType()].byteSize;
    alignMent = (alignMent > 32) ? 32 : alignMent;
    alignMent = (alignMent < 16) ? 16 : alignMent;

    if (dst->getRegAccess() != Direct || dst->getHorzStride() != 1 ||
        !builder.isOpndAligned(dst, alignMent))
    {
        if (preferFpMad)
        {
            G4_DstRegRegion* tmpDst = insertMovAfter(iter, dst, dst->getType(), bb);
            inst->setDest(tmpDst);
        }
        else
        {
            return false;
        }
    }

    // check src
    for (int k = 0; k < inst->getNumSrc(); k++)
    {
        G4_Type type = inst->getSrc(k)->getType();
        bool goodSrc = isGoodAlign16Src(inst, k);
        if (!goodSrc && preferFpMad)
        {
            // insert moves if type is legal mad type
            if (isGoodMadType(type))
            {
                G4_Operand* src = inst->getSrc(k);
                bool isReplicated = (type == Type_DF) &&
                    src->isSrcRegRegion() &&
                    (src->asSrcRegRegion()->getRegion()->width == 2) &&
                    (src->asSrcRegRegion()->getRegion()->horzStride == 0) &&
                    (src->asSrcRegRegion()->getRegion()->vertStride == 2);
                if ((type == Type_DF ||
                    (type == Type_HF && builder.getPlatform() == GENX_BDW)) &&
                    execSize > 1 &&
                    (src->isImm() || src->asSrcRegRegion()->isScalar()))
                {
                    // MAD DF does not support .r, so we have to broadcast the value
                    // '.r' on MAD HF on BDW is not a replication of that
                    // scalar element but a pair of half.
                    auto align = type == Type_HF ? GRFALIGN : Eight_Word;
                    broadcast(bb, iter, k, align);
                }
                // No need to insert mov for replicated DF src with <2;2,0> region,
                // which can be encoded as "xyxy" or "zwzw" swizzle based on offfset
                else if (!isReplicated)
                {
                    inst->setSrc(insertMovBefore(iter, k, type, bb), k);
                }
                goodSrc = true;
            }
        }
        if (!goodSrc)
        {
            return false;
        }
    }

    inst->setOpcode(G4_mad);

    //swap src0 and src2 (vISA MAD is src0*src1+src2, while GEN MAD is src1*src2+src0)
    inst->swapSrc(0, 2);

    return true;
}

// If the LF MAD does not conform to Genx ISA semantics, then translate
// it into a valid GenX sequence - either an equivalent MUL/ADD sequence
// or an equivalent MAC.
// ASSUMPTION:
//    This phase must be called at the end of all other optimizations
//    phases and just prior to testing for ACC spilling.
void HWConformity::fixMADInst(G4_BB* bb)
{
    INST_LIST expand_list;
    // trace the MAD instrcutions that may be converted into MAC later
    std::vector<G4_INST*> madList;

    bool doAlign1Mad = builder.hasAlign1Ternary();

    bb->resetLocalId();
    INST_LIST_ITER i = bb->begin();

    for (auto iterEnd = bb->end(); i != iterEnd; ++i)
    {

        G4_INST* inst = *i;
        // predicated mad is not allowed?
        if (inst->opcode() != G4_pseudo_mad)
        {
            continue;
        }

        tryEliminateMadSrcModifier(builder, inst);

        G4_DstRegRegion* dst = inst->getDst();
        uint32_t exec_size = inst->getExecSize();
        G4_Operand* src0 = inst->getSrc(0), * src1 = inst->getSrc(1), * src2 = inst->getSrc(2);

        bool conforming_genx_mad = true;
        bool generate_genx_mac;

        if (exec_size > builder.getNativeExecSize() * 2)
        {
            conforming_genx_mad = false;
        }
        else
        {
            switch (dst->getType())
            {
            case Type_F:
            case Type_HF:
            case Type_DF:
                break;
            case Type_W:
            case Type_UW:
            case Type_D:
            case Type_UD:
                if (!doAlign1Mad)
                {
                    conforming_genx_mad = false;
                }
                break;
            default:
                conforming_genx_mad = false;
            }
        }

        if (conforming_genx_mad)
        {
            bool doMad = doAlign1Mad ?
                generateAlign1Mad(bb, i) : generateFPMad(bb, i);
            if (doMad)
            {
                // done with this pseudo-mad
                continue;
            }
        }

        // Translate the LF MAD to an equivalent GenX sequence.
        if (builder.getOption(vISA_LocalMACopt))
        {
            generate_genx_mac = true;
        }
        else
        {
            generate_genx_mac = false;
        }

        if (IS_TYPE_FLOAT_ALL(dst->getType()) && exec_size > 8)
        {
            // no float mac following the original code
            generate_genx_mac = false;
        }

        if (generate_genx_mac)
        {
            int emask = inst->getMaskOption();
            if (emask != InstOpt_WriteEnable && inst->getMaskOffset() != 0)
            {
                generate_genx_mac = false;
            }
            // If either src1 or src0 are DWORD then we cannot generate a MAC.
            // ACC does not support B type
            if (generate_genx_mac &&
                (IS_BTYPE(src2->getType()) ||
                    IS_DTYPE(src0->getType()) ||
                    IS_DTYPE(src1->getType())))
            {
                generate_genx_mac = false;
            }

            // If there is a modifier for src2, or src2 is accessed somewhere indirectly then we will
            // not generate a MAC.
            if (generate_genx_mac)
            {
                if (src2->isImm() ||
                    (src2->isSrcRegRegion() &&
                    (src2->asSrcRegRegion()->getModifier() != Mod_src_undef ||
                        src2->asSrcRegRegion()->getRegAccess() != Direct ||
                        (src2->getTopDcl() && src2->getTopDcl()->getAddressed()))) ||
                    src2->getType() == Type_DF)
                {
                    generate_genx_mac = false;
                }
            }
        }
        // we can't do mac if src2 is global or it has >1 def or its single def is global
        if (generate_genx_mac)
        {
            G4_INST* mad_src2_def_inst = inst->getSingleDef(Opnd_src2);
            if (!mad_src2_def_inst || kernel.fg.globalOpndHT.isOpndGlobal(src2) ||
                kernel.fg.globalOpndHT.isOpndGlobal(mad_src2_def_inst->getDst()))
            {
                generate_genx_mac = false;
            }


            if (madList.size() > 0 && mad_src2_def_inst != madList.back())
            {
                // terminate the last mad list as this mad has a different definition
                int32_t lastMadId = madList.back()->getLocalId();
                bool macGenerated = convertMAD2MAC(i, madList, bb);
                madList.clear();
                if (generate_genx_mac && macGenerated &&
                    mad_src2_def_inst->getLocalId() < lastMadId)
                {
                    // mad's definition is before the last use of acc
                    generate_genx_mac = false;
                }
            }

            if (generate_genx_mac &&
                (mad_src2_def_inst->getPredicate() ||
                    mad_src2_def_inst->getSaturate() ||
                    mad_src2_def_inst->isMath() ||
                    mad_src2_def_inst->opcode() == G4_shl ||
                    mad_src2_def_inst->opcode() == G4_mad ||
                    !mad_src2_def_inst->hasOneUse() ||
                    (isCompressedInst(mad_src2_def_inst) ^ isCompressedInst(inst))))
            {
                generate_genx_mac = false;
            }

            if (generate_genx_mac &&
                madList.size() == 0 &&
                IS_DTYPE(mad_src2_def_inst->getExecType()))
            {
                // We don't generate mac in this case since by default we use w type for acc,
                // and it would violate dst alignment restriction
                // if mad_src2_def_inst is itself a psuedo_mad, however, then it's ok
                // since both sources for mac must have word type.
                generate_genx_mac = false;
            }

            if (generate_genx_mac)
            {
                // We will try to generate a MAC if it is possible to hoist
                // the definition for src2 into ACC, otherwise we will need to
                // generate a MOV/MAC; in which case we might as well
                // generate a MUL/ADD sequence anyway.

                // If the src2_def_op does not immediately precede the
                // MAD then we will attempt to schedule backward op to
                // immediately after src2_def_op. This will increase
                // the MAC reduction opportunities as it has the
                // effect of keeping ACC live ranges to very
                // short intervals.
                // NOTE: We do not attempt to schedule the src2_def_op
                // to just before op, as src2_def_op may be a
                // previously scheduled MAD.

                INST_LIST_ITER mov_iter = i;
                mov_iter--;
                uint16_t movDist = 0;

                if ((*mov_iter) != mad_src2_def_inst) {
                    // check if src and dst of MAD are re-defined in between and
                    // if dst is used in between
                    if (!findHoistLocation(i, mov_iter, movDist, mad_src2_def_inst))
                    {
                        generate_genx_mac = false;
                    }
                    else
                    {
                        if (movDist > 0)
                        {
                            mov_iter++;
                            bb->insert(mov_iter, inst);
                            INST_LIST_ITER tmpIter = i;
                            i--;
                            bb->erase(tmpIter);
                        }
                    }
                }

                // if instruction moving is blocked by some re-def, we need to check if it is possible that the ACC def instruction
                // will be split later. If yes, we do not use ACC and MAC here.

                // push this decision to convertMAC2MAD

                if (generate_genx_mac)
                {
                    if (madList.size() == 0)
                    {
                        // push src2 def into list
                        madList.push_back(mad_src2_def_inst);
                    }
                    madList.push_back(inst);
                }
            }
        }

        // translate MAD into MUL/ADD
        if (!generate_genx_mac)
        {
            convertMAD2MulAdd(i, bb);
            i++;
        }
    }
    if (madList.size() > 0)
    {
        i--;
        convertMAD2MAC(i, madList, bb);
    }
}

struct AccInterval
{
    G4_INST* inst;
    int lastUse;
    bool mustBeAcc0 = false;
    bool isPreAssigned = false;
    int assignedAcc = -1;
    int bundleConflictTimes = 0;
    int bankConflictTimes = 0;
    int suppressionTimes = 0;

    AccInterval(G4_INST* inst_, int lastUse_, bool preAssigned = false) :
        inst(inst_), lastUse(lastUse_), isPreAssigned(preAssigned)
    {
        if (isPreAssigned)
        {
            mustBeAcc0 = true;
            assignedAcc = 0;
        }
    }

    double getSpillCost()
    {
        if (isPreAssigned)
        {
            // don't spill pre-assigned
            return (double)1000000;
        }
        int dist = lastUse - inst->getLocalId();

        return std::pow((double)inst->use_size(), 3) / dist;
    }

    // see if this interval needs both halves of the acc
    bool needBothAcc(IR_Builder& builder) const
    {
        switch (inst->getDst()->getType())
        {
        case Type_F:
            return inst->getExecSize() == (builder.getNativeExecSize() * 2);
        case Type_HF:
            return false;
        case Type_DF:
            return inst->getExecSize() > (builder.getNativeExecSize() / 2);
        default:
            return true;
        }
    }

    void dump()
    {
        std::cerr << "Interval: [" << inst->getLocalId() << ", " << lastUse << "]\n";
        std::cerr << "\t";
        inst->dump();
        if (assignedAcc != -1)
        {
            std::cerr << "\tAssigned to Acc" << assignedAcc << "\n";
        }
        std::cerr << "\n";
    }
};


// returns true if the inst is a candidate for acc substitution
// lastUse is also update to point to the last use id of the inst
static bool isAccCandidate(G4_INST* inst, G4_Kernel& kernel, int& lastUse, bool& mustBeAcc0)

{
    mustBeAcc0 = false;
    G4_DstRegRegion* dst = inst->getDst();
    if (!dst || kernel.fg.globalOpndHT.isOpndGlobal(dst) || !inst->canDstBeAcc())
    {
        return false;
    }

    // check that every use may be replaced with acc
    int lastUseId = 0;
    std::vector<G4_INST*> madSrc0Use;
    std::vector<G4_INST*> threeSrcUses; //3src inst that use this dst
    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto&& use = *I;
        G4_INST* useInst = use.first;
        Gen4_Operand_Number opndNum = use.second;
        lastUseId = std::max(lastUseId, useInst->getLocalId());
        // acc may be src0 of two-source inst or src1 of three-source inst
        // ToDo: may swap source here
        if (useInst->getNumSrc() == 3)
        {

            if (!kernel.fg.builder->relaxedACCRestrictions() &&
                std::find(threeSrcUses.begin(), threeSrcUses.end(), useInst) != threeSrcUses.end())
            {
                // don't allow acc to appear twice in a 3-src inst
                return false;
            }
            threeSrcUses.push_back(useInst);
            switch (opndNum)
            {
            case Opnd_src1:
                break;  //OK

            case Opnd_src0:

                if (kernel.fg.builder->canMadHaveSrc0Acc())
                {
                    // OK
                }
                else if (useInst->opcode() == G4_mad)
                {
                    // we can turn this mad into a mac
                    mustBeAcc0 = true;
                    if (useInst->getSrc(0)->getType() == Type_HF && useInst->getMaskOffset() == 16)
                    {
                        // we must use acc1, and need to check that inst does not have an acc0 source
                        // so that dst and src won't have different acc source
                        if (inst->isAccSrcInst())
                        {
                            bool hasAcc0Src = false;
                            auto isAcc0 = [](G4_SrcRegRegion* src)
                            {
                                return src->getBase()->asAreg()->getArchRegType() == AREG_ACC0;
                            };
                            if (inst->getSrc(0)->isSrcRegRegion() &&
                                inst->getSrc(0)->asSrcRegRegion()->getBase()->isAccReg())
                            {
                                hasAcc0Src = isAcc0(inst->getSrc(0)->asSrcRegRegion());
                            }
                            else if (inst->getSrc(1)->isSrcRegRegion() &&
                                inst->getSrc(1)->asSrcRegRegion()->getBase()->isAccReg())
                            {
                                hasAcc0Src = isAcc0(inst->getSrc(1)->asSrcRegRegion());
                            }
                            if (hasAcc0Src)
                            {
                                return false;
                            }
                        }
                    }
                    madSrc0Use.push_back(useInst);
                }
                else
                {
                    return false;
                }
                break;
            default:
                return false;
            }
        }
        else if (opndNum != Opnd_src0)
        {
            return false;
        }

        if (useInst->getSingleDef(opndNum) == nullptr)
        {
            // def must be the only define for this use
            return false;
        }

        int srcId = opndNum == Opnd_src0 ? 0 : 1;
        G4_Operand* src = useInst->getSrc(srcId);
        if (dst->getType() != src->getType() || kernel.fg.globalOpndHT.isOpndGlobal(src) ||
            dst->compareOperand(src) != Rel_eq)
        {
            return false;
        }
        if (!useInst->canSrcBeAcc(opndNum))
        {
            return false;
        }
    }

    // we have to avoid the case where the dst is used as both src0 and src1 of a mad
    for (auto madUse : madSrc0Use)
    {
        for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
        {
            auto&& use = *I;
            G4_INST* useInst = use.first;
            Gen4_Operand_Number opndNum = use.second;
            if (madUse == useInst && opndNum == Opnd_src1)
            {
                return false;
            }
        }
    }

    if (lastUseId == 0)
    {
        // no point using acc for a dst without local uses
        return false;
    }

    lastUse = lastUseId;
    return true;
}

// replace an inst's dst and all of its (local) uses with acc
// note that this may fail due to HW restrictions on acc
static bool replaceDstWithAcc(G4_INST* inst, int accNum, IR_Builder& builder)
{
    G4_DstRegRegion* dst = inst->getDst();
    bool useAcc1 = (accNum & 0x1) != 0;
    accNum &= ~0x1;

    if (!builder.relaxedACCRestrictions())
    {
        auto myAcc = useAcc1 ? AREG_ACC1 : AREG_ACC0;
        // check that dst and src do not have different accumulator
        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
        {
            if (inst->getSrc(i)->isAccReg())
            {
                auto base = inst->getSrc(i)->asSrcRegRegion()->getBase();
                if (base->isPhyAreg())
                {
                    if (base->asAreg()->getArchRegType() != myAcc)
                    {
                        return false;
                    }
                }
            }
        }
    }

    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto&& use = *I;
        G4_INST* useInst = use.first;
        if (!builder.canMadHaveSrc0Acc() && useInst->opcode() == G4_mad && use.second == Opnd_src0)
        {
            // if we are replacing mad with mac, additionally check if acc1 needs to be used
            if (useInst->getMaskOffset() == 16 && dst->getType() == Type_HF)
            {
                if (builder.doMultiAccSub())
                {
                    // this is not legal since acc1 may be taken by another interval already
                    return false;
                }
                useAcc1 = true;
            }
        }

        if (!builder.relaxedACCRestrictions())
        {
            // do not allow an inst to have multiple acc source operands
            if (useInst->getNumSrc() == 3)
            {
                if (useInst->getSrc(0)->isAccReg() || useInst->getSrc(1)->isAccReg())
                {

                    return false;
                }
            }
            else if (useInst->opcode() == G4_mac)
            {
                // this can happen if we have to convert mad into mac (some platforms don't allow
                // src0 acc for mad), and the mad's src1 is also an acc candidate.
                return false;
            }
        }
    }

    // at this point acc substitution must succeed

    G4_Areg* accReg = useAcc1 ? builder.phyregpool.getAcc1Reg() : builder.phyregpool.getAcc0Reg();
    G4_DstRegRegion* accDst = builder.createDst(accReg,
        (short)accNum, 0, 1, dst->getType());
    accDst->setAccRegSel(inst->getDst()->getAccRegSel());
    inst->setDest(accDst);
    for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
    {
        auto&& use = *I;
        G4_INST* useInst = use.first;
        int srcId = use.second == Opnd_src0 ? 0 : 1;
        G4_SrcRegRegion* oldSrc = useInst->getSrc(srcId)->asSrcRegRegion();
        G4_SrcRegRegion* accSrc = builder.createSrcRegRegion(oldSrc->getModifier(), Direct,
            accReg, (short)accNum, 0, builder.getRegionStride1(), dst->getType());
        accSrc->setAccRegSel(oldSrc->getAccRegSel());
        if (useInst->opcode() == G4_mad && srcId == 0 && !builder.canMadHaveSrc0Acc())
        {
            // change mad to mac as src0 of 3-src does not support acc
            auto updateDefSrcPos = [](G4_INST* useInst, Gen4_Operand_Number origPos)
            {
                for (auto DI = useInst->def_begin(), DE = useInst->def_end(); DI != DE; ++DI)
                {
                    auto&& def = *DI;
                    if (def.second == origPos)
                    {
                        for (auto UI = def.first->use_begin(), UE = def.first->use_end(); UI != UE; ++UI)
                        {
                            auto& use = *UI;
                            if (use.first == useInst && use.second == origPos)
                            {
                                switch (use.second)
                                {
                                case Opnd_src1:
                                    use.second = Opnd_src0;
                                    break;
                                case Opnd_src2:
                                    use.second = Opnd_src1;
                                    break;
                                default:
                                    assert(false && "unexpectd src pos");
                                }
                            }
                        }
                    }
                }
            };
            assert(accNum == 0 && "mad src0 may only use acc0");
            G4_Operand* macSrc0 = useInst->getSrc(1);
            updateDefSrcPos(useInst, Opnd_src1);
            G4_Operand* macSrc1 = useInst->getSrc(2);
            updateDefSrcPos(useInst, Opnd_src2);
            useInst->setSrc(macSrc0, 0);
            useInst->setSrc(macSrc1, 1);
            useInst->setOpcode(G4_mac);
            useInst->setImplAccSrc(accSrc);
        }
        else
        {
            useInst->setSrc(accSrc, srcId);
        }
    }

    return true;
}

static bool isAccCandidate(G4_INST* inst, Gen4_Operand_Number opndNum, G4_Kernel& kernel)

{
    if (!kernel.fg.builder->canMadHaveSrc0Acc())
    {
        return false;
    }

    switch (opndNum)
    {
    case Opnd_src0:
    case Opnd_src1:
        break;
    default:
        return false;
    }

    if (!inst->canSrcBeAcc(opndNum))
    {
        return false;
    }

    return true;
}

struct AccAssignment
{
    std::vector<bool> freeAccs;
    std::list<AccInterval*> activeIntervals;
    IR_Builder& builder;

    AccAssignment(int numGeneralAcc, IR_Builder& m_builder) : builder(m_builder)
    {
        freeAccs.resize(numGeneralAcc, true);
    }

    // expire all intervals that end before the given interval
    void expireIntervals(AccInterval* interval)
    {
        for (auto iter = activeIntervals.begin(), iterEnd = activeIntervals.end(); iter != iterEnd;)
        {
            AccInterval* active = *iter;
            if (active->lastUse <= interval->inst->getLocalId())
            {
                assert(!freeAccs[active->assignedAcc] && "active interval's acc should not be free");
                freeAccs[active->assignedAcc] = true;
                if (active->needBothAcc(builder))
                {
                    assert(!freeAccs[active->assignedAcc + 1] && "active interval's acc should not be free");
                    freeAccs[active->assignedAcc + 1] = true;
                }
                iter = activeIntervals.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // spill interval that is assigned to accID and remove it from active list
    void spillInterval(int accID)
    {
        auto acc0Iter = std::find_if(activeIntervals.begin(), activeIntervals.end(),
            [accID](AccInterval* interval) { return interval->assignedAcc == accID; });
        assert(acc0Iter != activeIntervals.end() && "expect to find interval with acc0");
        auto spillInterval = *acc0Iter;
        assert(!spillInterval->isPreAssigned && "overlapping pre-assigned acc0");
        spillInterval->assignedAcc = -1;
        activeIntervals.erase(acc0Iter);
        freeAccs[accID] = true;
        if (spillInterval->needBothAcc(builder))
        {
            assert(accID % 2 == 0 && "accID must be even-aligned in this case");
            freeAccs[accID + 1] = true;
        }
    }

    // pre-assigned intervals (e.g., mach, addc) must use acc0 (and acc1 depending on inst type/size)
    // we have to spill active intervals that occupy acc0/acc1.
    // the pre-assigned interavl is also pushed to active list
    void handlePreAssignedInterval(AccInterval* interval)
    {
        if (!freeAccs[interval->assignedAcc])
        {
            spillInterval(interval->assignedAcc);
        }
        freeAccs[interval->assignedAcc] = false;

        if (interval->needBothAcc(builder))
        {
            assert(interval->assignedAcc == 0 && "Total 2 acc support right now");
            if (!freeAccs[interval->assignedAcc + 1]) // && activeIntervals.size()
            {
                spillInterval(interval->assignedAcc + 1);
            }
            freeAccs[interval->assignedAcc + 1] = false;
        }

        activeIntervals.push_back(interval);
    }

    // pick a free acc for this interval
    // returns true if a free acc is found, false otherwise
    bool assignAcc(AccInterval* interval)
    {
        if (interval->isPreAssigned)
        {
            handlePreAssignedInterval(interval);
            return true;
        }

        int step = interval->needBothAcc(builder) ? 2 : 1;
        for (int i = 0, end = interval->mustBeAcc0 ? 1 : (int)freeAccs.size(); i < end; i += step)
        {
            if (freeAccs[i] && (!interval->needBothAcc(builder) || freeAccs[i + 1]))
            {
                interval->assignedAcc = i;
                freeAccs[i] = false;
                if (interval->needBothAcc(builder))
                {
                    freeAccs[i + 1] = false;
                }

                activeIntervals.push_back(interval);
                return true;
            }
        }
        return false;
    }
};


void HWConformity::multiAccSubstitution(G4_BB* bb)
{
    int numGeneralAcc = kernel.getNumAcc();

    std::vector<AccInterval*> intervals;


    //build intervals for potential acc candidates as well as pre-existing acc uses from mac/mach/addc/etc
    for (auto instIter = bb->begin(), instEnd = bb->end(); instIter != instEnd; ++instIter)
    {
        G4_INST* inst = *instIter;
        if (inst->defAcc())
        {
            // we should only have single def/use acc at this point, so any use would kill the def
            auto iter = instIter;
            auto useIter = std::find_if(++iter, instEnd, [](G4_INST* inst) { return inst->useAcc(); });
            int lastUseId = useIter == instEnd ? bb->back()->getLocalId() : (*useIter)->getLocalId();
            AccInterval* newInterval = new AccInterval(inst, lastUseId, true);
            intervals.push_back(newInterval);
        }
        else
        {
            int lastUseId = 0;
            bool mustBeAcc0 = false;
            int bundleBCTimes = 0;
            int bankBCTimes = 0;
            int readSuppressionSrcs = 0;
            if (isAccCandidate(inst, kernel, lastUseId, mustBeAcc0))
            {
                // this is a potential candidate for acc substitution
                AccInterval* newInterval = new AccInterval(inst, lastUseId);
                newInterval->mustBeAcc0 = mustBeAcc0;
                newInterval->bankConflictTimes = bankBCTimes;
                newInterval->bundleConflictTimes = bundleBCTimes;
                newInterval->suppressionTimes = readSuppressionSrcs;

                intervals.push_back(newInterval);
            }
    }
}


        //modified linear scan to assign free accs to intervals
        AccAssignment accAssign(numGeneralAcc, builder);

        for (auto interval : intervals)
        {
            // expire intervals
            accAssign.expireIntervals(interval);

            // assign interval
            bool foundFreeAcc = accAssign.assignAcc(interval);

            //Spill
            if (!foundFreeAcc && accAssign.activeIntervals.size() != 0)
            {
                // check if we should spill one of the active intervals
                auto spillCostCmp = [interval](AccInterval* intv1, AccInterval* intv2)
                {
                    if (!interval->mustBeAcc0)
                    {
                        return intv1->getSpillCost() < intv2->getSpillCost();
                    }

                    // different compr function if interval must use acc0
                    if (intv1->assignedAcc == 0 && intv2->assignedAcc == 0)
                    {
                        return intv1->getSpillCost() < intv2->getSpillCost();
                    }
                    else if (intv1->assignedAcc == 0)
                    {
                        return true;
                    }
                    return false;
                };
                auto spillIter = std::min_element(accAssign.activeIntervals.begin(), accAssign.activeIntervals.end(),
                    spillCostCmp);
                auto spillCandidate = *spillIter;
                if (interval->getSpillCost() > spillCandidate->getSpillCost() &&
                    !spillCandidate->isPreAssigned &&
                    !(interval->mustBeAcc0 && spillCandidate->assignedAcc != 0))
                {
                    bool tmpAssignValue[2];

                    tmpAssignValue[0] = accAssign.freeAccs[spillCandidate->assignedAcc];
                    accAssign.freeAccs[spillCandidate->assignedAcc] = true;
                    if (spillCandidate->needBothAcc(builder))
                    {
                        tmpAssignValue[1] = accAssign.freeAccs[spillCandidate->assignedAcc + 1];
                        accAssign.freeAccs[spillCandidate->assignedAcc + 1] = true;
                    }

                    if (accAssign.assignAcc(interval))
                    {
                        spillCandidate->assignedAcc = -1;
                        accAssign.activeIntervals.erase(spillIter);
                    }
                    else
                    {
                        accAssign.freeAccs[spillCandidate->assignedAcc] = tmpAssignValue[0];
                        if (spillCandidate->needBothAcc(builder))
                        {
                            accAssign.freeAccs[spillCandidate->assignedAcc + 1] = tmpAssignValue[1];
                        }
                    }
                }
            }
        }

        for (auto interval : intervals)
        {
            if (!interval->isPreAssigned && interval->assignedAcc != -1)
            {
                G4_INST* inst = interval->inst;
                replaceDstWithAcc(inst, interval->assignedAcc, builder);

                numAccSubDef++;
                numAccSubUse += (int)inst->use_size();
#if 0
                std::cout << "Acc sub def inst: \n";
                inst->emit(std::cout);
                std::cout << "[" << inst->getLocalId() << "]\n";
                std::cout << "Uses:\n";
                for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I)
                {
                    auto&& use = *I;
                    std::cout << "\t";
                    use.first->emit(std::cout);
                    std::cout << "[" << use.first->getLocalId() << "]\n";
                }
#endif
    }
}


    for (int i = 0, end = (int)intervals.size(); i < end; ++i)
    {
        delete intervals[i];
    }

    return;
}

struct LiveNode
{
    G4_INST* Inst;
    Gen4_Operand_Number OpNum;
    LiveNode(G4_INST* Inst, Gen4_Operand_Number OpNum)
        : Inst(Inst)
        , OpNum(OpNum)
    {
    }
};

#define GLOBAL_USE_NUM 15

static bool isSameOperand(G4_Operand* srcOpnd, struct LiveNode* ln)
{
    G4_Operand* opnd = ln->Inst->getOperand(ln->OpNum);

    if (opnd->compareOperand(srcOpnd) == Rel_eq)
    {
        return true;
    }

    return false;
}

// substitute local operands with acc when possible
void HWConformity::localizeForAcc(G4_BB* bb)
{
    std::map<const G4_Declare*, G4_Operand*> replacedOperand;
    std::unordered_map<const G4_Declare*, vector<struct LiveNode>> useNodes;
    std::vector<const G4_Declare*> erasedCandidates;

    for (auto instIter = bb->begin(), instEnd = bb->end(); instIter != instEnd; ++instIter)
    {
        G4_INST* inst = *instIter;

        //Not defined in current BB
        G4_Operand* dst = inst->getOperand(Opnd_dst);
        if (dst && dst->isGreg() && kernel.fg.globalOpndHT.isOpndGlobal(dst))
        {
            const G4_Declare* dcl = dst->getTopDcl();
            if (useNodes.find(dcl) != useNodes.end())
            {
                useNodes.erase(dcl);
                erasedCandidates.emplace_back(dcl);
            }
        }

        //Source operand
        for (auto OpNum :
            { Gen4_Operand_Number::Opnd_src0, Gen4_Operand_Number::Opnd_src1,
              Gen4_Operand_Number::Opnd_src2 })
        {
            G4_Operand* src = inst->getOperand(OpNum);
            if (src && src->isGreg() && kernel.fg.globalOpndHT.isOpndGlobal(src))
            {
                const G4_Declare* dcl = src->getTopDcl();
                if ((OpNum != Opnd_src0 &&  //Acc can be used only for src0 and src1
                    OpNum != Opnd_src1) ||
                    !isAccCandidate(inst, OpNum, kernel)) //The operand is can be replaced with ACC
                {
                    auto dclIter = std::find(erasedCandidates.begin(), erasedCandidates.end(), dcl);
                    if (dclIter == erasedCandidates.end())
                    {
                        erasedCandidates.emplace_back(dcl);
                    }
                }
                else
                {
                    if (useNodes[dcl].empty() ||
                        isSameOperand(src, &(useNodes[dcl][0])))
                    {
                        useNodes[dcl].emplace_back(inst, OpNum);
                    }
                }
            }
        }
    }

    for (auto& Nodes : useNodes)
    {
        const G4_Declare* dcl = Nodes.first;
        auto dclIter = std::find(erasedCandidates.begin(), erasedCandidates.end(), dcl);
        if (dclIter != erasedCandidates.end())
        {
            continue;
        }
        if (Nodes.second.size() >= GLOBAL_USE_NUM)
        {
            for (auto& LN : Nodes.second)
            {
                G4_INST* inst = LN.Inst;
                Gen4_Operand_Number opNum = LN.OpNum;
                int i = inst->getSrcNum(opNum);
                G4_Operand* src = inst->getSrc(i);
                G4_Operand* tmpOpnd = nullptr;

                auto itR = replacedOperand.find(dcl);
                if (itR != replacedOperand.end())
                {
                    tmpOpnd = builder.duplicateOperand(itR->second);
                }
                else
                {
                    tmpOpnd = insertCopyAtBBEntry(bb, inst->getExecSize(), src);
                    replacedOperand[dcl] = tmpOpnd;
                }
                inst->setSrc(tmpOpnd, i);
            }
        }
    }

    return;
}


// substitute local operands with acc when possible
void HWConformity::accSubstitution(G4_BB* bb)
{
    bb->resetLocalId();

    if (builder.doMultiAccSub())
    {
        multiAccSubstitution(bb);
        return;
    }

    for (auto instIter = bb->begin(), instEnd = bb->end(); instIter != instEnd; ++instIter)
    {
        bool canDoAccSub = true;
        G4_INST* inst = *instIter;

        if (inst->defAcc())
        {
            // skip ahead till its single use
            // we should only have single def/use acc at this point, so any use would
            // kill the def
            auto iter = instIter;
            auto useIter = std::find_if(++iter, instEnd, [](G4_INST* inst) { return inst->useAcc(); });
            if (useIter == instEnd)
            {
                return;
            }
            instIter = --useIter; // start at the use inst next time
            continue;
        }

        int lastUseId = 0;
        bool mustBeAcc0 = false; //ignored
        if (!isAccCandidate(inst, kernel, lastUseId, mustBeAcc0))
        {
            continue;
        }

        // don't attempt acc sub if def and last use are too far apart
        // this is a crude way to avoid a long running life range from blocking
        // other acc sub opportunities
        const int accWindow = 25;
        if (lastUseId == 0 || lastUseId - inst->getLocalId() > accWindow)
        {
            continue;
        }

        // check for intervening acc usage between inst and its last use
        auto subIter = instIter;
        ++subIter;
        for (int instId = inst->getLocalId() + 1; instId != lastUseId; ++subIter, ++instId)
        {
            G4_INST* anInst = *subIter;
            if (anInst->useAcc() || anInst->mayExpandToAccMacro())
            {
                canDoAccSub = false;
                break;
            }
        }

        if (!canDoAccSub)
        {
            continue;
        }
        else
        {
            replaceDstWithAcc(inst, 0, builder);
            // advance iter to the last use of the acc
            instIter = subIter;
            --instIter;

            numAccSubDef++;
            numAccSubUse += (int)inst->use_size();

#if 0
            std::cout << "Acc sub def inst: \n";
            inst->emit(std::cout);
            std::cout << "[" << inst->getLocalId() << "]\n";
            std::cout << "Uses:\n";
            for (auto&& use : inst->useInstList)
            {
                std::cout << "\t";
                use.first->emit(std::cout);
                std::cout << "[" << use.first->getLocalId() << "]\n";
            }
#endif
        }
    }
}

// find the location for hoisting the inst pointed to by start
// boundary is the upper limit for hoisting
// if there is any ACC def/use between start and end, return false;
// otherwise, return true.
bool HWConformity::findHoistLocation(
    INST_LIST_ITER start, INST_LIST_ITER& end, uint16_t& movDist, G4_INST* boundary)
{
    bool canMov = true;
    G4_INST* inst = *start;
    end = start;
    end--;
    movDist = 0;

    if ((*end) != boundary)
    {
        // check if src and dst of MAD are re-defined in between and
        // if dst is used in between
        while ((*end) != boundary)
        {
            G4_INST* curInst = *end;
            if (curInst->hasACCOpnd() || curInst->mayExpandToAccMacro())
            {
                canMov = false;
                break;
            }

            if (inst->isRAWdep(curInst) ||
                inst->isWAWdep(curInst) ||
                inst->isWARdep(curInst))
            {
                break;
            }
            movDist++;
            --end;
        }

        // check if acc is possibly updated between the new location and boundary
        if (canMov && ((*end) != boundary))
        {
            INST_LIST_ITER in_between_iter = end;
            ++in_between_iter;
            for (; (*in_between_iter) != boundary; --in_between_iter)
            {
                G4_INST* curInst = *in_between_iter;
                if (curInst->hasACCOpnd() || curInst->mayExpandToAccMacro())
                {
                    canMov = false;
                    break;
                }
            }
        }
    }
    return canMov;
}

// for mac code gen we use W as acc type for int since it has enough precision for int
G4_Type HWConformity::getAccType(G4_Type ty)
{
    if (ty == Type_D)
    {
        return Type_W;
    }
    else if (ty == Type_UD)
    {
        return Type_UW;
    }
    else
    {
        return ty;
    }
}
// convert MAD in madList to MAC instructions
// iter is either the next pseudo-mad that does not belong to this list, or the last inst in the BB
// return true if the mad list is converted to mac
bool HWConformity::convertMAD2MAC(INST_LIST_ITER iter, std::vector<G4_INST*>& madList, G4_BB* bb)
{
    if (madList.size() == 1)
    {
        // there is only one inst in list, it is not a MAD
        return false;
    }

    // find the iterator of the last mad in list
    G4_INST* lastMad = madList.back();
    INST_LIST_ITER movTarget, lastMadIter = iter;
    while ((*lastMadIter) != lastMad)
    {
        lastMadIter--;
    }
    movTarget = lastMadIter;

    bool changeType = false;
    bool dwDst = IS_TYPE_INT(lastMad->getDst()->getType());
    bool twoGRFDst = lastMad->hasNULLDst() ? false :
        ((lastMad->getDst()->getRightBound() - lastMad->getDst()->getLeftBound() + 1) > GENX_GRF_REG_SIZ);
    G4_Type newType = lastMad->getDst()->getType();
    // check if we can convert the type of MAC dst from DW to W,
    // such that we can avoid instruction splitting and improve code quality
    if (dwDst && lastMad->hasNULLDst())
    {
        // is this possible?
        changeType = true;
        lastMad->getDst()->setType(IS_SIGNED_INT(lastMad->getDst()->getType()) ? Type_W : Type_UW);
    }
    else if (dwDst && twoGRFDst &&
        lastMad->hasOneUse() &&
        !kernel.fg.globalOpndHT.isOpndGlobal(lastMad->getDst()))
    {
        // last mad has single use, see if we can replace the def-use pair with acc
        G4_INST* useInst = lastMad->use_front().first;
        if (useInst->getDst() &&
            (IS_BTYPE(useInst->getDst()->getType()) || IS_WTYPE(useInst->getDst()->getType())))
        {
            // check the use of last MAD dst
            INST_LIST_ITER useIter = lastMadIter;
            useIter++;
            while ((*useIter) != useInst)
            {
                useIter++;
            }

            uint16_t movDist, hs;
            if (lastMad->canUseACCOpt(false, true, hs, true, true) && hs == 1 &&
                findHoistLocation(useIter, movTarget, movDist, lastMad) &&
                (*movTarget) == lastMad)
            {
                changeType = true;
                if (movDist > 0)
                {
                    movTarget++;
                    bb->insert(movTarget, useInst);
                    bb->erase(useIter);
                }
                uint32_t dstStrideSize = G4_Type_Table[useInst->getDst()->getType()].byteSize * useInst->getDst()->getHorzStride();
                uint32_t useTypeSize = G4_Type_Table[Type_UW].byteSize;
                // insert a temp mov
                if (dstStrideSize > useTypeSize)
                {
                    movTarget--;
                    insertMovAfter(movTarget,
                        (uint16_t)(useTypeSize / G4_Type_Table[useInst->getDst()->getType()].byteSize),
                        bb);
                }

                newType = getAccType(newType);
                // change src of useInst to ACC
                Gen4_Operand_Number srcNum = lastMad->use_front().second;

                ASSERT_USER(useInst->getSrc((uint32_t)srcNum - 1)->isSrcRegRegion(),
                    "Unexpected src to be changed!");

                G4_SrcRegRegion* accSrcOpnd = builder.createSrcRegRegion(
                    useInst->getSrc((uint32_t)srcNum - 1)->asSrcRegRegion()->getModifier(),
                    Direct,
                    builder.phyregpool.getAcc0Reg(),
                    0,
                    0,
                    builder.getRegionStride1(),
                    newType);

                useInst->setSrc(accSrcOpnd, (uint32_t)srcNum - 1);

                // change dst of the last MAD
                G4_DstRegRegion* accDstOpnd = builder.createDst(
                    builder.phyregpool.getAcc0Reg(),
                    0,
                    0,
                    1,
                    newType);

                lastMad->setDest(accDstOpnd);
            }
        }
    }

    // if we can do type demotion or dst fits in 1GRF, we do not have to worry about inst splitting.
    if (!twoGRFDst || changeType)
    {
        // generate MAC directly
        auto madIter = madList.end();
        madIter--;
        G4_INST* curInst = (*madIter);

        G4_Type accType = getAccType(curInst->getSrc(2)->getType());
        uint32_t accTypeSize = getTypeSize(accType);
        // mac dst region has to match that of acc, which is always GRF-aligned
        // we also cannot have acc dst hstride > 4
        if (!builder.isOpndAligned(curInst->getDst(), GENX_GRF_REG_SIZ) ||
            (curInst->getDst()->getExecTypeSize() / accTypeSize) > 4)
        {
            // ToDo: store the iter in madInst?
            auto instIter = std::find(bb->begin(), bb->end(), curInst);
            auto newDst = insertMovAfter(instIter, curInst->getDst(), curInst->getDst()->getType(), bb, GRFALIGN);
            curInst->setDest(newDst);
        }
        uint32_t dstByteStride = curInst->getDst()->getExecTypeSize();
        uint16_t stride = (uint16_t)(dstByteStride > accTypeSize ? dstByteStride / accTypeSize : 1);
        const RegionDesc* region = builder.createRegionDesc(stride, 1, 0);

        G4_SrcRegRegion* accSrcOpnd = builder.createSrcRegRegion(
            Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(),
            0, 0, region, accType);

        curInst->setImplAccSrc(accSrcOpnd);
        curInst->setSrc(nullptr, 2);
        curInst->setOpcode(G4_mac);
        curInst->fixMACSrc2DefUse();

        do
        {
            // change all intermediate macs to use acc dst and src
            madIter--;
            curInst = (*madIter);
            bool changeSrc = curInst->opcode() == G4_pseudo_mad;
            addACCOpnd(curInst, changeSrc, stride, accType);
        } while (madIter != madList.begin());
        return true;
    }

    // just split them into mul/add
    // assumption: all pseudo_mads from lastMadIter back to the first inst should be on madList

    auto madIter = lastMadIter;
    for (G4_INST* inst = *madIter; inst != madList.front(); inst = *(--madIter))
    {
        if (inst->opcode() == G4_pseudo_mad)
        {
            convertMAD2MulAdd(madIter, bb);
        }
    }
    return false;
}

void HWConformity::convertComprInstSrcRegion(G4_INST* inst)
{
    for (int k = 0; k < 2; k++)
    {
        G4_Operand* src = inst->getSrc(k);

        if (!src || src->isImm() || (inst->isMath() && k == 1 && src->isNullReg()))
        {
            continue;
        }

        if (!src->isSrcRegRegion()) {
            continue;
        }

        int w = src->asSrcRegRegion()->getRegion()->width;
        int hs = src->asSrcRegRegion()->getRegion()->horzStride;
        int vs = src->asSrcRegRegion()->getRegion()->vertStride;

        if (w == 1 && hs == 0 && vs == 0)
        {
            continue;
        }

        if (inst->getExecSize() < w)
        {
            const RegionDesc* rd =
                builder.createRegionDesc((uint16_t)(vs / 2), (uint16_t)(w / 2), (uint16_t)(hs / 2));
            src->asSrcRegRegion()->setRegion(rd);
        }
    }
}

// replace src/dst with ACC
void HWConformity::addACCOpnd(
    G4_INST* curInst, bool needACCSrc, int dstStride, G4_Type accTy)
{

    if (needACCSrc)
    {
        // change src2 to implicit ACC src.
        const RegionDesc* region = nullptr;
        switch (dstStride)
        {
        case 1:
            region = builder.getRegionStride1();
            break;
        case 2:
            region = builder.getRegionStride2();
            break;
        case 4:
            region = builder.getRegionStride4();
            break;
        default:
            MUST_BE_TRUE(false, "unexpected stride value");
            break;
        }

        G4_SrcRegRegion* accSrcOpnd = builder.createSrcRegRegion(
            Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(),
            0, 0, region, accTy);

        curInst->setImplAccSrc(accSrcOpnd);
        curInst->setSrc(nullptr, 2);
        curInst->setOpcode(G4_mac);
        curInst->fixMACSrc2DefUse();
    }

    // change dst for all in between MAD
    G4_DstRegRegion* accDstOpnd = builder.createDst(builder.phyregpool.getAcc0Reg(), 0,
        0, (unsigned short)dstStride, accTy);
    curInst->setDest(accDstOpnd);

}

// convert a psuedo mad inst into mul/add
// return the iterator pointing to add
void HWConformity::convertMAD2MulAdd(INST_LIST_ITER iter, G4_BB* bb)
{
    G4_INST* inst = *iter;
    assert(inst->opcode() == G4_pseudo_mad && "expect pseudo-mad");

    G4_DstRegRegion* addOpDst = inst->getDst();
    G4_Operand* addOpnd2 = inst->getSrc(2);
    G4_Type mulOpDstType = addOpDst->getType();
    G4_Type mulOpExecType = inst->getExecType();
    // pick the widest type of mad's src and dst as the intermediate type
    if (G4_Type_Table[mulOpDstType].byteSize > G4_Type_Table[mulOpExecType].byteSize)
    {
        mulOpExecType = mulOpDstType;
    }

    mulOpDstType = mulOpExecType;

    G4_SubReg_Align     subAlign = Get_G4_SubRegAlign_From_Type(mulOpDstType);

    // Reuse the MAD op for MUL.
    inst->setOpcode(G4_mul);
    inst->setSrc(nullptr, 2);

    G4_Declare* mulDefDcl = builder.createTempVar(inst->getExecSize(), mulOpDstType, subAlign);

    G4_DstRegRegion* mulOpDst = builder.Create_Dst_Opnd_From_Dcl(mulDefDcl, 1);
    inst->setDest(mulOpDst);

    // Follow with an ADD.
    INST_LIST_ITER tIter = iter;
    tIter++;

    auto addOpnd1 = builder.Create_Src_Opnd_From_Dcl(mulDefDcl, builder.getRegionStride1());
    G4_INST* addOp = builder.createInternalInst(
        inst->getPredicate(),
        G4_add,
        inst->getCondMod(),
        inst->getSaturate(),
        inst->getExecSize(),
        addOpDst,
        addOpnd1,
        addOpnd2,
        nullptr,
        inst->getOption(),
        inst->getLineNo(),
        inst->getCISAOff(),
        inst->getSrcFilename());

    bb->insert(tIter, addOp);

    // predicate/condmod/saturate, if they exist, are propagated to the add instruction
    inst->setSaturate(false);
    inst->setPredicate(NULL);
    inst->setCondMod(nullptr);

    {
        inst->transferDef(addOp, Opnd_src2, Opnd_src1);
        if (addOp->getPredicate())
        {
            inst->transferDef(addOp, Opnd_pred, Opnd_pred);
        }
        inst->transferUse(addOp);
        inst->addDefUse(addOp, Opnd_src0);
    }
}

// See if we can convert the pseudo_sada2 instruction into an actual Gen sada2
// This can be done if the following conditions are met:
// -- We can find the definition of the pseudo sada2 instruction's source 2 in
//    the same basic block, and that
// -- it may be replaced by an acc (i.e., the src2 is its only use, the dst and
//    the src have identical regions, and there are no intervening instructions
//    that update acc)
//
// We additionally attempt to schedule up the sada2 instruction to be as close
// as possible to the src2 defining instruction (subject to the constraints of
// def-use chains for def, src0 and src1), so that more opportunites may be
// exposed for later sada2 instructions

void HWConformity::fixSADA2Inst(G4_BB* bb)
{

    INST_LIST_ITER i = bb->begin();
    while (i != bb->end())
    {

        G4_INST* inst = *i;
        if (inst->opcode() != G4_pseudo_sada2)
        {
            ++i;
            continue;
        }

        G4_Operand* src2 = inst->getSrc(2);

        bool canDoSada2 = true;
        G4_INST* src2Dst = NULL;

        int emask = inst->getMaskOption();
        if (!bb->isAllLaneActive() &&
            emask != InstOpt_WriteEnable &&
            inst->getMaskOffset() != 0)
        {
            canDoSada2 = false;
        }

        G4_DstRegRegion* dst = inst->getDst();
        if (canDoSada2)
        {
            if (src2->isSrcRegRegion() && src2->asSrcRegRegion()->getRegAccess() == Direct)
            {
                // check Src2
                if (kernel.fg.globalOpndHT.isOpndGlobal(src2))
                {
                    // no sada2 if operand is global
                    canDoSada2 = false;
                }
                else if (src2->asSrcRegRegion()->getModifier() != Mod_src_undef)
                {
                    // no sada2 if src2 has a modifier
                    canDoSada2 = false;
                }
                else
                {
                    for (auto defIter = inst->def_begin(), end = inst->def_end(); defIter != end; ++defIter)
                    {
                        if ((*defIter).second == Opnd_src2)
                        {
                            if (src2Dst != NULL)
                            {
                                // no sada2 if src2 has >1 definition
                                canDoSada2 = false;
                                break;
                            }
                            src2Dst = (*defIter).first;
                        }
                    }

                    if (!src2Dst)
                    {
                        canDoSada2 = false;
                    }
                    else
                    {
                        if (!src2Dst->hasOneUse())
                        {
                            // no sad2 if def has more than one use
                            canDoSada2 = false;
                        }
                        else
                        {
                            G4_DstRegRegion* src2DstOpnd = src2Dst->getDst();
                            G4_Type src2DstType = src2DstOpnd->getType();
                            if (src2DstOpnd->getRegAccess() != Direct
                                || (src2DstType != Type_W && src2DstType != Type_UW))
                            {
                                // no sada2 if def's dst is indirect, or it type is not W or UW
                                canDoSada2 = false;
                            }
                            else if (src2DstOpnd->compareOperand(src2) !=
                                Rel_eq)
                            {
                                // no sada2 if src2Dst and src2 are not equal
                                canDoSada2 = false;
                            }
                        }
                    }
                }
            }
            else
            {
                canDoSada2 = false;
            }
        }

        // The new location of the sada2 after the conversion
        INST_LIST_ITER newSada2Iter = i;
        --newSada2Iter;
        if (canDoSada2)
        {
            // try to schedule up the sada2 to be as close to the src2-defining instruction
            // as possible to expose more optmizaition opportunities
            for (; *newSada2Iter != src2Dst; --newSada2Iter)
            {
                if (inst->isRAWdep(*newSada2Iter) ||
                    inst->isWAWdep(*newSada2Iter) ||
                    inst->isWARdep(*newSada2Iter))
                {
                    break;
                }
            }

            // make sure there are no instructions between the sada2's new location
            // and the src2-defining instruction that updates acc
            for (auto iter = newSada2Iter; *iter != src2Dst; --iter)
            {
                G4_INST* aInst = *iter;
                if (aInst->hasACCOpnd())
                {
                    canDoSada2 = false;
                    break;
                }
            }
        }

        if (canDoSada2)
        {
            // We have verified all conditions and can convert this instruction to sada2.
            // replace the destination for src2Dst to be acc0.
            // The actual acc0 offset will be fixed in a later pass
            G4_DstRegRegion* accDstOpnd = builder.createDst(
                builder.phyregpool.getAcc0Reg(),
                0,
                0,
                1,
                src2->getType());
            src2Dst->setDest(accDstOpnd);
            if (src2Dst->getExecSize() == 1)
            {
                // This can happen for the first sada2 instruction if src2 is scalar
                // expand its execution size so that acc is fully defined
                src2Dst->setExecSize(inst->getExecSize());
            }

            // create an implicit acc parameter for sada2
            inst->setOpcode(G4_sada2);
            inst->setSrc(nullptr, 2);
            G4_SrcRegRegion* accSrcOpnd = builder.createSrcRegRegion(
                Mod_src_undef,
                Direct,
                builder.phyregpool.getAcc0Reg(),
                0,
                0,
                builder.getRegionStride1(),
                src2->getType());

            inst->setImplAccSrc(accSrcOpnd);

            ++newSada2Iter;
            bb->insert(newSada2Iter, inst);
            i = bb->erase(i);

            // maintain def-use

            for (auto tmpIter = src2Dst->use_begin(), end = src2Dst->use_end(); tmpIter != end; ++tmpIter)
            {
                if ((*tmpIter).first == inst && (*tmpIter).second == Opnd_src2)
                {
                    (*tmpIter).second = Opnd_implAccSrc;
                    break;
                }
            }

            for (auto tmpIter = inst->def_begin(), end = inst->def_end(); tmpIter != end; ++tmpIter)
            {
                if ((*tmpIter).first == src2Dst && (*tmpIter).second == Opnd_src2)
                {
                    (*tmpIter).second = Opnd_implAccSrc;
                    break;
                }
            }
        }
        else
        {
            // pseudo_sada2 (N) dst src0 src1 src2
            // becomes
            // sad2 (n) tmp<1>:w src0 src1
            // add (n) dst tmp<n;n,1>:w src2

            inst->setOpcode(G4_sad2);
            inst->setSrc(nullptr, 2);

            G4_SubReg_Align sad2TmpSubAlign = Get_G4_SubRegAlign_From_Type(dst->getType());

            if (inst->getExecSize() * G4_Type_Table[dst->getType()].byteSize > GENX_GRF_REG_SIZ)
            {
                // align to GRF
                sad2TmpSubAlign = GRFALIGN;
            }
            // create a new temp variable as sad2's destination
            G4_Declare* sad2Tmp = builder.createTempVar(inst->getExecSize(), dst->getType(), sad2TmpSubAlign);
            G4_DstRegRegion* sad2Dst = builder.Create_Dst_Opnd_From_Dcl(sad2Tmp, 1);
            inst->setDest(sad2Dst);

            uint16_t srcVertStride, srcWidth, srcHorzStride;
            srcWidth = inst->getExecSize() > 8 ? 8 : inst->getExecSize();
            srcHorzStride = 1;
            srcVertStride = srcWidth;

            // opnd 0 for add is the new temp we've just created
            const RegionDesc* rd = builder.createRegionDesc(srcVertStride, srcWidth, srcHorzStride);
            G4_Operand* addSrc0Opnd = builder.createSrcRegRegion(Mod_src_undef, Direct, sad2Dst->getBase(),
                0, 0, rd, sad2Dst->getType());

            // opnd 1 is src2 of the pseudo_sada2
            // dst is the same as the pseudo_sada2
            G4_INST* addInst = builder.createInternalInst(
                inst->getPredicate(),
                G4_add,
                inst->getCondMod(),
                inst->getSaturate(),
                inst->getExecSize(),
                dst,
                addSrc0Opnd,
                src2,
                NULL,
                inst->getOption(),
                inst->getLineNo(),
                inst->getCISAOff(),
                inst->getSrcFilename());

            INST_LIST_ITER addLoc = i;
            ++addLoc;
            bb->insert(addLoc, addInst);

            // FIXME: redundant?
            inst->addDefUse(addInst, Opnd_src0);

            // The sad2 op should not have the SAT attribute set,
            // as this is intended only for the final result of the
            // SADA2 (and thus the add op will keep the SAT attribute).
            inst->setSaturate(false);
            inst->setPredicate(NULL);

            {
                inst->transferDef(addInst, Opnd_src2, Opnd_src1);
                if (addInst->getPredicate())
                {
                    inst->transferDef(addInst, Opnd_pred, Opnd_pred);
                }
                inst->transferUse(addInst);
                inst->addDefUse(addInst, Opnd_src0);
            }
            ++i;
        }
    }
}

void HWConformity::fixSendInst(G4_BB* bb)
{

    for (INST_LIST_ITER i = bb->begin(), end = bb->end(); i != end; i++)
    {

        G4_INST* inst = *i;
        if (!inst->isSend())
        {
            continue;
        }

        if (inst->getExecSize() < builder.getNativeExecSize())
        {
            // A64 messages require a minimum msg len of two for address (src0), which is inconsistent
            // with our input IR as it allows <2 GRF address variables (e.g., simd1 A64 scatter r/w).
            // To avoid this causing overlap between send dst/src0/src1 (it is known to cause HW hang),
            // we have to ensure they are all 2GRF-aligned
            G4_Declare* src0Dcl = inst->getSrc(0)->getTopDcl();
            // ToDo: check if dst/src1 may also exhibit such size mismatch
            bool sizeMismatch = inst->getMsgDesc()->MessageLength() == 2 &&
                (src0Dcl && src0Dcl->getRootDeclare()->getByteSize() < 2u * GENX_GRF_REG_SIZ);
            auto doEvenAlign = [](G4_Declare* dcl)
            {
                if (dcl)
                {
                    dcl = dcl->getRootDeclare();
                    // variables >= 2 GRF don't need even alignment since they can't possibly overlap
                    if (dcl->getByteSize() < 2u * GENX_GRF_REG_SIZ)
                    {
                        dcl->setEvenAlign();
                    }
                }
            };
            if (sizeMismatch)
            {
                doEvenAlign(inst->getSrc(0)->getTopDcl());
                if (inst->isSplitSend())
                {
                    doEvenAlign(inst->getSrc(1)->getTopDcl());
                }
                if (builder.WaDisableSendSrcDstOverlap())
                {
                    doEvenAlign(inst->getDst()->getTopDcl());
                }
            }
        }

        uint16_t offset = 0;
        if (!builder.isOpndAligned(inst->getDst(), offset, GENX_GRF_REG_SIZ))
        {
            inst->setDest(insertMovAfter(i, inst->getDst(), inst->getDst()->getType(), bb, GRFALIGN));
        }

        G4_Operand* src0 = inst->getSrc(0);
        G4_Declare* src0TopDcl = src0->getTopDcl();

        // if src0 and src1 are hard-wired GRF, check that
        // they satisfy EOT and preemption restrictions
        auto needsTempSrc = [this](G4_INST* inst, G4_Declare* dcl)
        {
            return dcl->getRegVar() && dcl->getRegVar()->getPhyReg() &&
                ((inst->isEOT() && builder.hasEOTGRFBinding() &&
                    dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum() < 112) ||
                    (builder.getOption(vISA_enablePreemption) &&
                        dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum() < 2));
        };

        auto fixSrc = [&](G4_INST* inst, bool isSrc0)
        {
            auto sendSrc = isSrc0 ? inst->getSrc(0)->asSrcRegRegion() : inst->getSrc(1)->asSrcRegRegion();
            uint16_t rows = isSrc0 ? inst->getMsgDesc()->MessageLength() : inst->getMsgDesc()->extMessageLength();
            G4_Type type = sendSrc->getType();
            G4_Declare* dcl = builder.createTempVar(rows * builder.getNativeExecSize(), type, GRFALIGN);

            MUST_BE_TRUE(G4_Type_Table[type].byteSize == 4, "Invalid src opnd type for send.");

            const RegionDesc* region = builder.getRegionStride1();
            G4_VarBase* base = sendSrc->getBase();
            short baseOff = sendSrc->getRegOff();
            short baseSubOff = sendSrc->getSubRegOff();
            for (uint16_t idx = 0; idx != rows; ++idx) {
                G4_SrcRegRegion* src = builder.createSrcRegRegion(Mod_src_undef, Direct, base, baseOff + idx, baseSubOff + 0, region, type);
                G4_DstRegRegion* dst = builder.createDst(dcl->getRegVar(), idx, 0, 1, type);
                G4_INST* newInst = builder.createMov(builder.getNativeExecSize(), dst, src, InstOpt_WriteEnable, false);
                bb->insert(i, newInst);
            }

            G4_Operand* newSrc = builder.Create_Src_Opnd_From_Dcl(dcl, builder.getRegionStride1());
            inst->setSrc(newSrc, isSrc0 ? 0 : 1);
        };

        if (needsTempSrc(inst, src0TopDcl))
        {
            fixSrc(inst, true);
        }

        if (inst->isSplitSend() && !inst->getSrc(1)->isNullReg())
        {
            // src1 may be null because some messages (e.g., CPS) require split send
            if (!builder.isOpndAligned(inst->getSrc(1), GENX_GRF_REG_SIZ))
            {
                inst->setSrc(insertMovBefore(i, 1, inst->getSrc(1)->getType(), bb, GRFALIGN), 1);
            }
            G4_Operand* src1 = inst->getSrc(1);
            G4_Declare* src1TopDcl = src1->getTopDcl();

            if (needsTempSrc(inst, src1TopDcl))
            {
                fixSrc(inst, false);
            }
        }

        if (builder.getOption(vISA_enablePreemption))
        {
            G4_DstRegRegion* dst = inst->getDst();
            if (!dst->isNullReg())
            {
                G4_Declare* dstTopDcl = dst->getTopDcl();
                if (dstTopDcl != NULL &&
                    dstTopDcl->getRegVar() &&
                    dstTopDcl->getRegVar()->getPhyReg())
                {
                    MUST_BE_TRUE((dstTopDcl->getRegVar()->getPhyReg()->asGreg()->getRegNum() > 2), "Unexpected preg used for send destination.");
                }
            }
        }

        if (builder.WaDisableSendSrcDstOverlap())
        {
            // create copy if dst and src0/src1 overlap due to being the same variable
            bool src0Overlap = inst->getDst()->compareOperand(inst->getSrc(0)) != Rel_disjoint;
            bool src1Overlap = inst->isSplitSend() && inst->getDst()->compareOperand(inst->getSrc(1)) != Rel_disjoint;
            if (src0Overlap || src1Overlap)
            {
                int dstSize = inst->getMsgDesc()->ResponseLength();
                int src0Size = src0Overlap ? inst->getMsgDesc()->MessageLength() : 0;
                int src1Size = src1Overlap ? inst->getMsgDesc()->extMessageLength() : 0;
                if (inst->getPredicate() || (bb->isDivergent() && !inst->isWriteEnableInst()) || dstSize > src0Size + src1Size)
                {
                    //copy src0/src1 if inst does not update all channels
                    if (src0Overlap)
                    {
                        G4_Declare* copyDst = builder.createTempVar(src0Size * NUM_DWORDS_PER_GRF, Type_UD, Any);
                        copyRegs(copyDst, 0, inst->getSrc(0)->getBase()->asRegVar()->getDeclare(),
                            inst->getSrc(0)->asSrcRegRegion()->getRegOff() * getGRFSize(), src0Size, bb, i);
                        inst->setSrc(builder.Create_Src_Opnd_From_Dcl(copyDst, builder.getRegionStride1()), 0);
                    }
                    if (src1Overlap)
                    {
                        G4_Declare* copyDst = builder.createTempVar(src1Size * NUM_DWORDS_PER_GRF, Type_UD, Any);
                        copyRegs(copyDst, 0, inst->getSrc(1)->getBase()->asRegVar()->getDeclare(),
                            inst->getSrc(1)->asSrcRegRegion()->getRegOff() * getGRFSize(), src1Size, bb, i);
                        inst->setSrc(builder.Create_Src_Opnd_From_Dcl(copyDst, builder.getRegionStride1()), 1);
                    }
                }
                else
                {
                    // copy dst
                    auto copyIter = i;
                    ++copyIter;
                    G4_Declare* copySrc = builder.createTempVar(dstSize * NUM_DWORDS_PER_GRF, Type_UD, Any);
                    copyRegs(inst->getDst()->getBase()->asRegVar()->getDeclare(), inst->getDst()->getRegOff() * getGRFSize(),
                        copySrc, 0, dstSize, bb, copyIter);
                    inst->setDest(builder.Create_Dst_Opnd_From_Dcl(copySrc, 1));
                }
            }
        }

    }

}

void HWConformity::fixOverlapInst(G4_BB* bb)
{
    for (INST_LIST_ITER i = bb->begin(), end = bb->end(); i != end; i++)
    {
        G4_INST* inst = *i;

        if (inst->mayExceedTwoGRF() || inst->opcode() == G4_madm)
        {
            continue;
        }

        if (inst->getDst() != NULL)
        {
            // create copy if dst and src0/src1 overlap due to being the same variable
            G4_Operand* dst = inst->getDst();
            if (dst != NULL && dst->isDstRegRegion() && dst->getTopDcl() && dst->getTopDcl()->getRegFile() == G4_GRF)
            {
                int dstSize = (dst->getLinearizedEnd() - dst->getLinearizedStart() + 1) / G4_GRF_REG_NBYTES;
                int srcSize = 1;

                bool srcOverlap = false;
                for (int i = 0; i < inst->getNumSrc(); i++)
                {
                    G4_Operand* src = inst->getSrc(i);
                    if (src != NULL && !src->isNullReg() && src->getTopDcl() && src->getTopDcl()->getRegFile() == G4_GRF)
                    {
                        srcOverlap |= inst->getDst()->compareOperand(inst->getSrc(i)) == Rel_interfere;
                        if (srcOverlap)
                        {
                            srcSize = (src->getLinearizedEnd() - src->getLinearizedStart() + 1) / G4_GRF_REG_NBYTES;
                            break;
                        }
                    }
                }

                if (srcOverlap && (dstSize > 1 || srcSize > 1))
                {
                    G4_AccRegSel accSel = inst->getDst()->getAccRegSel();
                    G4_DstRegRegion* newDst = insertMovAfter(i, inst->getDst(), inst->getDst()->getType(), bb);
                    newDst->setAccRegSel(accSel);
                    inst->setDest(newDst);
                }
            }
        }
    }
}

//
// Fix sel and csel instructions:
//  -- set their cond mod to null as they don't modify it.  They will be hard-coded to f0.0 in Gen asm

void HWConformity::fixSelCsel(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;
    if (inst->opcode() == G4_sel || inst->opcode() == G4_csel)
    {
        G4_CondMod* condMod = inst->getCondMod();
        if (condMod)
        {
            condMod->setBase(nullptr);
        }
    }
}

//
//  Avoid the dst and src overlap when they are using the same variable by inserting a mov instruction
//  add(8)  var1<2>, var2, var1<0, 1, 0>
//
void HWConformity::avoidDstSrcOverlap(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;

    if (inst->isSend() ||
        inst->opcode() == G4_nop ||
        inst->isLabel())
    {
        return;
    }

    G4_DstRegRegion* dst = inst->getDst();

    if (dst && dst->getBase()->isRegVar())
    {
        G4_Declare* dstDcl = dst->getTopDcl();

        if (dstDcl != nullptr)
        {
            G4_DstRegRegion* dstRgn = dst;
            int dstOpndNumRows = (( dstRgn->getLinearizedEnd() - dstRgn->getLinearizedStart()) / G4_GRF_REG_NBYTES) + 1;
            int dstLeft = dstRgn->getLinearizedStart();
            int dstRight = dstOpndNumRows > 1 ? ((dstLeft / G4_GRF_REG_NBYTES + 1) * G4_GRF_REG_NBYTES - 1) :
                dstRgn->getLinearizedEnd();

            for (int i = 0, nSrcs = inst->getNumSrc(); i < nSrcs; i++)
            {
                G4_Operand* src = inst->getSrc(i);

                if (!src || !src->getTopDcl())
                {
                    continue;
                }

                G4_Declare* srcDcl = src->getTopDcl();
                G4_CmpRelation rel = dst->compareOperand(src);
                if ((rel != Rel_disjoint && rel != Rel_undef) &&
                    src->isSrcRegRegion() &&
                    src->asSrcRegRegion()->getBase()->isRegVar() &&
                    srcDcl == dstDcl)
                {
                    G4_SrcRegRegion* srcRgn = src->asSrcRegRegion();
                    int srcOpndNumRows = ((srcRgn->getLinearizedEnd() - srcRgn->getLinearizedStart()) / G4_GRF_REG_NBYTES) + 1;
                    int srcLeft = srcRgn->getLinearizedStart();
                    int srcRight = srcRgn->getLinearizedEnd();

                    if (!srcRgn->isScalar() && srcOpndNumRows > 1)
                    {
                        srcLeft = (srcRgn->getLinearizedStart() / G4_GRF_REG_NBYTES + 1) * G4_GRF_REG_NBYTES;
                    }

                    if (dstOpndNumRows > 1 || srcOpndNumRows > 1)
                    {
                        if (!(srcLeft > dstRight || dstLeft > srcRight))
                        {
                            inst->setSrc(insertMovBefore(it, i, src->getType(), bb), i);
                        }
                    }
                }
            }
        }
    }

    return;
}

void HWConformity::conformBB(G4_BB* bb)
{
    INST_LIST_ITER i = bb->begin(), iEnd = bb->end();
    INST_LIST_ITER next_iter = i;
    for (; i != iEnd; i = next_iter)
    {
        // by default we skip the newly inserted instructions as we assume they are already HW conformed
        // if a check may produce new instructions that violate HW rules, it must adjust the next_iter
        // to point to them
        ++next_iter;
        G4_INST* inst = *i;
        G4_opcode opcode = inst->opcode();

        if ((inst->mayExceedTwoGRF() && !inst->isSend()) ||
            opcode == G4_nop ||
            opcode == G4_label)
        {
            continue;
        }

        if(builder.avoidDstSrcOverlap() &&
            inst->getDst() != NULL)
        {
            avoidDstSrcOverlap(i, bb);
        }

        // do this early since otherwise the moves inserted by other passes may still
        // inherit bad regions from the original inst
        fixSrcRegion(inst);

        bool changed = fixMov(i, bb);
        if (changed)
        {
            next_iter = i;
            next_iter++;
        }

        fixOpndType(i, bb);

        fixSelCsel(i, bb);

        fixPredCtrl(i, bb);

        if (inst->getExecSize() > builder.getNativeExecSize())
        {
            if (inst->opcode() == G4_math &&
                inst->getDst()->getType() == Type_HF &&
                inst->getSrc(0)->getType() == Type_HF &&
                (!inst->getSrc(1) || inst->getSrc(1)->getType() == Type_HF))
            {
                // split pure HF math to simd8
                evenlySplitInst(i, bb);
            }
        }
        fix3SrcInst(i, bb);

        G4_Operand* dst = inst->getDst();

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        if (inst->isMath())
        {
            if (fixMathInst(i, bb))
            {
                // check the newly added insts later
                next_iter = i;
                next_iter++;
            }
        }

        inst = *i;

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        if (inst->opcode() == G4_mul)
        {
            if (fixMULInst(i, bb))
            {
                // inserted mach and mov
                // check the newly added insts later ( MUL, MACH, MOV )
                next_iter = i;
                next_iter++;
            }
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        if (inst->opcode() == G4_mulh)
        {
            fixMULHInst(i, bb);
            // inserted mul before
            // check the newly added MUL inst
            i--;
            next_iter = i;
            continue;
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        // HW check #6: indirect operand spilling
        fixIndirectOpnd(i, bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
        // HW check #8: unsigned dst with execution type F
        /* If the execution type is F and the destination type if either UD, UW
         * or UB and the detination is not saturated, then we need to add an
         * intermediate type conversion to D.
         */
        inst = *i;
        opcode = inst->opcode();

        if (opcode == G4_cmp || opcode == G4_cmpn)
        {
            dst = inst->getDst();
            int dst_elsize = 0;
            bool null_dst = !dst || inst->hasNULLDst();
            if (!null_dst)
            {
                dst_elsize = dst->isPredicate() ? G4_Type_Table[Type_UW].byteSize : G4_Type_Table[dst->getType()].byteSize;
            }
            int extypesize;
            G4_Type extype = inst->getOpExecType(extypesize);
            fixCompareInst(i, bb, extype, dst_elsize);
        }
        dst = inst->getDst();

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
        if (fixAcc(i, bb))
        {
            next_iter = i;
            next_iter++;
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        {
            dst = inst->getDst();
            G4_Type extype = inst->getExecType2();
            int extypesize = G4_Type_Table[extype].byteSize;
            int dst_elsize = 0;
            if (dst)
            {
                dst_elsize = G4_Type_Table[dst->getType()].byteSize;
            }

            if (dst &&
                inst->getExecSize() == 1 &&
                dst_elsize < extypesize &&
                !IS_VTYPE(extype) &&
                !inst->isMixedMode() &&
                !inst->isSend())
            {
                fixDstHstride(i, extypesize);
            }
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        bool planeDeleted = fixPlaneInst(i, bb);
        if (planeDeleted)
        {
            continue;
        }

        fixLine(i, bb);
        fixRotate(i, bb);

        if (!builder.hasVxHFloat64b())
        {
            fixVxHFloat64b(i, bb);
        }

        // CHV/BXT specific checks for 64b datatypes
        if (fix64bInst(i, bb))
        {
            continue;
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
        fixImm64(i, bb); // fixed immediates for DF4 in fixImm64()


        // FIXME: may be better to call fixDstAlign instead
        if (builder.getPlatform() == GENX_BDW)
        {
            fixPackedHFConversions(i, bb);
        }
    }
}

//
// SIMD16 addc/subb are illegal on GEN, since they write to acc and there are only 8 acc
// channels for D/UD type.  In vISA IR we should get something like
// addc (16) V0 V2 V3
// mov  (16) V1 acc0<8;8,1>:ud
// which needs to be translated to
// addc (8) V0(0) V2(0) V3(0) {Q1}
// mov (8) V1(0) acc0<8;8,1>:ud {Q1}
// addc (8) V0(1) V2(1) V3(1) {Q2}
// mov (8) V1(1) acc0<8;8,1>:ud {Q2}
//
// We do this first thing in HW conformity to avoid REXES from splitting addc/subb incorrectly
// We also count on previous opt to preserve the inst pair by not inserting any acc using inst in between;
// it should hopefully be the case since we generally don't optimize instructions with acc src/dst
//
// If exec size of addc is < 8, we also have to make sure both the addc's dst and the carry move's dst are
// GRF-aligned, since acc's channel is dependent on the dst's subreg offset.  In other words, we fix
// addc (1) r1.0 ...
// mov (1) r1.1 acc0.0<0;1,0>
// into
// addc (1) r1.0 ...
// mov (1) r2.0 acc0.0<0;1,0>
// mov (1) r1.1 r2.0
//
//
bool HWConformity::fixAddcSubb(G4_BB* bb)
{
    bool changed = false;
    for (auto iter = bb->begin(), iterEnd = bb->end();
        iter != iterEnd; ++iter)
    {
        G4_INST* inst = *iter;
        if ((inst->opcode() == G4_addc || inst->opcode() == G4_subb) &&
            inst->getExecSize() != builder.getNativeExecSize())
        {
            // find the matching carry move
            G4_INST* carryMov = nullptr;
            auto movIter = iter;
            for (++movIter; movIter != iterEnd; ++movIter)
            {
                G4_INST* inst2 = *movIter;
                if (inst2->opcode() == G4_mov && inst2->getExecSize() == inst->getExecSize() &&
                    inst2->getSrc(0)->isAccReg() && inst2->getSrc(0)->getType() == Type_UD)
                {
                    carryMov = inst2;
                    break;
                }
                else if (inst2->useAcc())
                {
                    break;
                }
            }

            if (carryMov == NULL)
            {
                // can't find the move using acc, skip this addc/subb
                assert(false && "expect a carry move instruction");
                continue;
            }

            if (inst->getExecSize() > builder.getNativeExecSize())
            {
                evenlySplitInst(iter, bb);
                evenlySplitInst(movIter, bb);

                // movIter now points to the second half of move, and we want to move the first move to be
                // before the second half of the addc/subb, which is pointed by iter
                --movIter;
                G4_INST* mov1 = *movIter;
                bb->erase(movIter);
                bb->insert(iter, mov1);

                changed = true;
            }
            else
            {
                // we will need to GRF-align addc's dst as well as the move dst,
                // so that the acc will have the correct offset
                // note that insertMovAfter will align the tmp since addc/subb has implicit acc use
                if (!builder.isOpndAligned(inst->getDst(), 32))
                {
                    inst->setDest(
                        insertMovAfter(iter, inst->getDst(), inst->getDst()->getType(), bb));
                    changed = true;
                }
                if (!builder.isOpndAligned(carryMov->getDst(), 32))
                {
                    carryMov->setDest(
                        insertMovAfter(movIter, carryMov->getDst(), carryMov->getDst()->getType(), bb));
                    changed = true;
                }
            }
        }
    }
    return changed;
}

void HWConformity::chkHWConformity()
{
    fixDataLayout();

    for (auto bb : kernel.fg)
    {
        fixIntToHFMove(bb);
#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
        fixAddcSubb(bb);
#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        fixMADInst(bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
        // fix source operand first to avoid redundant MOVs if this fix is done after
        // reducing execution size.
        // used by 3d. Mainly to fix sel with two imm sources
        fixOpndTypeAlign(bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        if (builder.getOption(vISA_accSubstitution) &&
            !builder.getOption(vISA_doAccSubAfterSchedule))
        {
            accSubstitution(bb);
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        fixInstExecSize(bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        fixMixedHFInst(bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
        fixSADA2Inst(bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        fixSendInst(bb);

        if (builder.avoidDstSrcOverlap())
        {
            fixOverlapInst(bb);
        }

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif

        conformBB(bb);

#ifdef _DEBUG
        verifyG4Kernel(kernel, Optimizer::PI_HWConformityChk, false);
#endif
    }
}

bool HWConformity::hasBadRegion(G4_INST* inst)
{
    if (inst->getImplAccDst() || inst->getImplAccSrc())
        return false;
    bool badRegion = false;
    for (unsigned int srcNum = 0, n_srcs = inst->getNumSrc(); srcNum < n_srcs; srcNum++)
    {
        if (!(inst->getSrc(srcNum)->isSrcRegRegion()))
        {
            continue;
        }
        const RegionDesc* rd = inst->getSrc(srcNum)->asSrcRegRegion()->getRegion();
        if (rd->isRegionWH())
        {
            badRegion = true;
            break;
        }
        if (rd->horzStride == GENX_MAX_H_STRIDE && rd->width > 1)
        {
            badRegion = true;
            break;
        }
        G4_SrcRegRegion* expandSrcRegion = inst->getSrc(srcNum)->asSrcRegRegion();
        if (expandSrcRegion->getRegAccess() != Direct)
        {
            const RegionDesc* origRegion = expandSrcRegion->getRegion();
            short secondSubRegOffDiff = 0, secondAddrImmedDiff = 0;

            if (origRegion->width == 1)
            {
                secondSubRegOffDiff = origRegion->vertStride;
            }
            else
            {
                secondSubRegOffDiff = origRegion->horzStride;
            }
            secondAddrImmedDiff = (short)(secondSubRegOffDiff * G4_Type_Table[expandSrcRegion->getType()].byteSize);
            if ((expandSrcRegion->getAddrImm() + secondAddrImmedDiff) > G4_MAX_ADDR_IMM)
            {
                badRegion = true;
                break;
            }
        }
    }
    return badRegion;
}

bool HWConformity::canSplitInst(G4_INST* inst, G4_INST* use_op)
{
    if ((inst->getPredicate() && inst->getExecSize() < 16) || hasBadRegion(inst))
        return false;

    G4_CondMod* condMod = inst->getCondMod();
    if (condMod)
    {
        return false;
    }

    for (int i = 0; i < inst->getNumSrc(); i++)
    {
        G4_Operand* src = inst->getSrc(i);
        if (src->isAccReg())
        {
            // don't split inst with explicit acc
            return false;
        }
        if (src->isSrcRegRegion() && src->asSrcRegRegion()->getRegion()->vertStride == 32 && src->asSrcRegRegion()->getRegion()->width == 1)
        {
            // don't split the source into even/odd since verstride can't exceed 32
            // ToDo: check for horizontal stride as well?
            return false;
        }
    }

    return true;
}

bool HWConformity::canSplitByteDst(G4_opcode op)
{
    switch (op)
    {
    case G4_mac:
    case G4_mach:
    case G4_cmp:
    case G4_mad:
    case G4_sad2:
    case G4_sada2:
    case G4_line:
    case G4_send:
    case G4_sendc:
        return false;
    default:
        return true;
    }
}
// split one instruction into 2 if its dstination is packed byte and execution type is W.
// for example:
// add <16> V1(0,0)<1>:b V1(0,0)<16;16,1>:w V2(0,0)<16;16,1>:w
// ==>
// add <8> V1(0,0)<2>:b V1(0,0)<16;8,2>:w V2(0,0)<16;8,2>:w
// add <8> V1(0,1)<2>:b V1(0,1)<16;8,2>:w V2(0,1)<16;8,2>:w

// if predicate is used for instruction, the definition of this predicate is tracked and the
// corresponding instruction is checked to see if it can do the same split.
bool HWConformity::splitInstListForByteDst(INST_LIST_ITER it, G4_BB* bb, uint16_t extypesize)
{
    G4_INST* inst = *it;
    G4_opcode inst_op = inst->opcode();
    G4_DstRegRegion* dst = inst->getDst();
    // check if we can split the inst
    if (!canSplitByteDst(inst_op) ||
        inst->getExecSize() == 1 ||
        (!bb->isAllLaneActive() && !inst->isWriteEnableInst()) ||
        dst->getByteOffset() % extypesize != 0 ||
        dst->getHorzStride() != 1 ||
        extypesize != G4_Type_Table[Type_W].byteSize)
    {
        return false;
    }

    if (inst->getPredicate() || inst->getCondMod())
    {
        return false;
    }

    // recursively the inst that defines its predicate can be split
    INST_LIST expandOpList;
    bool canSplit = canSplitInst(inst, NULL);
    if (canSplit)
    {
        expandOpList.push_back(inst);
    }

    G4_INST* currInst = inst;
    while (canSplit && currInst->getPredicate())
    {
        // look for predicate def inst
        uint16_t defNum = 0;
        G4_INST* defInst = NULL;

        // FIXME: should be currInst->defInstList.begin()?
        for (auto def_iter = inst->def_begin(), end = inst->def_end(); def_iter != end; def_iter++)
        {
            if ((*def_iter).second == Opnd_pred)
            {
                defNum++;
                defInst = (*def_iter).first;
            }
        }
        if (defNum != 1 || !defInst->getCondMod())
        {
            canSplit = false;
            break;
        }
        if (canSplit)
        {
            if (!bb->isAllLaneActive() && !defInst->isWriteEnableInst())
            {
                canSplit = false;
            }
            else
            {
                canSplit = canSplitInst(defInst, currInst);
            }
        }
        // check if def inst can be split
        if (!canSplit)
        {
            break;
        }
        else
        {
            expandOpList.push_back(defInst);
            currInst = defInst;
        }
    }

    // split inst into two
    INST_LIST_ITER new_iter = it;
    new_iter++;
    if (canSplit)
    {
        while (!expandOpList.empty())
        {
            G4_INST* expand_op = expandOpList.front();
            expandOpList.pop_front();
            // find location of expand_op in instruction list
            do
            {
                new_iter--;
                if ((*new_iter) == expand_op)
                {
                    break;
                }
            } while (new_iter != bb->begin());

            MUST_BE_TRUE(new_iter != bb->end(), "Cannot find predicate definition function in BB.");
            new_iter++;
            G4_INST* secondHalfOp = splitInstWithByteDst(expand_op);
            MUST_BE_TRUE(secondHalfOp, "Error in spliting instruction.");
            bb->insert(new_iter, secondHalfOp);
        }
    }


    return canSplit;
}

G4_INST* HWConformity::splitInstWithByteDst(G4_INST* expand_op)
{
    unsigned char newExecSize = expand_op->getExecSize() / 2;
    if (expand_op->getPredicate())
    {
        expand_op->getPredicate()->splitPred();
    }
    if (expand_op->getCondMod())
    {
        expand_op->getCondMod()->splitCondMod();
    }
    G4_INST* expand_sec_half_op = builder.createInternalInst(
        builder.duplicateOperand(expand_op->getPredicate()),
        expand_op->opcode(),
        builder.duplicateOperand(expand_op->getCondMod()),
        expand_op->getSaturate(),
        newExecSize,
        NULL,
        NULL,
        NULL,
        NULL,
        expand_op->getOption(),
        expand_op->getLineNo(),
        expand_op->getCISAOff(),
        expand_op->getSrcFilename());
    MUST_BE_TRUE(expand_sec_half_op != NULL, ERROR_MEM_ALLOC);

    expand_op->setExecSize(newExecSize);

    if (expand_op->getDst() && !expand_op->hasNULLDst())
    {
        G4_DstRegRegion* old_dst = expand_op->getDst();
        short secondSubRegOff = old_dst->getSubRegOff() + 1;

        G4_DstRegRegion* newDstOpnd = builder.createDstRegRegion(
            old_dst->getRegAccess(),
            old_dst->getBase(),
            old_dst->getRegOff(),
            old_dst->getSubRegOff(),
            old_dst->getHorzStride() * 2,
            old_dst->getType());
        if (old_dst->getRegAccess() != Direct)
        {
            newDstOpnd->setImmAddrOff(old_dst->getAddrImm());
            secondSubRegOff -= 1;
        }

        expand_op->setDest(newDstOpnd);

        G4_DstRegRegion* secondDstOpnd = builder.createDstRegRegion(
            old_dst->getRegAccess(),
            old_dst->getBase(),
            old_dst->getRegOff(),
            secondSubRegOff,
            old_dst->getHorzStride() * 2,
            old_dst->getType());

        if (old_dst->getRegAccess() != Direct)
        {
            secondDstOpnd->setImmAddrOff(old_dst->getAddrImm() + 1);
        }

        expand_sec_half_op->setDest(secondDstOpnd);
    }
    else
    {
        expand_sec_half_op->setDest(expand_op->getDst());
    }

    for (int k = 0, n_srcs = expand_op->getNumSrc(); k < n_srcs; k++)
    {
        G4_Operand* expand_src = expand_op->getSrc(k);

        if (!expand_src)
            continue;

        if ((expand_op->isMath() && k == 1 && expand_src->isNullReg()) ||
            expand_src->isImm()) {
            expand_sec_half_op->setSrc(expand_src, k);
        }
        else if (expand_src->isSrcRegRegion()) {
            G4_SrcRegRegion* expandSrcRegion = expand_src->asSrcRegRegion();

            if (expandSrcRegion->isScalar()) {
                expand_sec_half_op->setSrc(builder.duplicateOperand(expand_src), k);
            }
            else {
                short secondSubRegOffDiff = 0, secondAddrImmedDiff = 0;

                const RegionDesc* origRegion = expandSrcRegion->getRegion();
                const RegionDesc* newRegion = NULL;

                if (origRegion->width == 1)
                {
                    newRegion = builder.createRegionDesc(origRegion->vertStride * 2, origRegion->width, origRegion->horzStride);
                    secondSubRegOffDiff = origRegion->vertStride;
                }
                else
                {
                    unsigned short newWD = origRegion->width / 2;
                    secondSubRegOffDiff = origRegion->horzStride;
                    newRegion = builder.createRegionDesc(
                        (newWD == 1 && newExecSize == 1) ? 0 : origRegion->vertStride,
                        newWD, (newWD == 1) ? 0 : origRegion->horzStride * 2);
                }
                secondAddrImmedDiff = (short)(secondSubRegOffDiff * G4_Type_Table[expand_src->getType()].byteSize);
                expandSrcRegion->setRegion(newRegion);

                bool directSrc = (expandSrcRegion->getRegAccess() == Direct);
                if (secondAddrImmedDiff >= GENX_GRF_REG_SIZ)
                {
                    secondSubRegOffDiff =
                        (short)((secondAddrImmedDiff - GENX_GRF_REG_SIZ) / G4_Type_Table[expand_src->getType()].byteSize);
                }
                G4_SrcRegRegion* secondSrcOpnd = builder.createSrcRegRegion(
                    expandSrcRegion->getModifier(),
                    expandSrcRegion->getRegAccess(),
                    expandSrcRegion->getBase(),
                    expandSrcRegion->getRegOff() + ((directSrc && secondAddrImmedDiff >= GENX_GRF_REG_SIZ) ? 1 : 0),
                    expandSrcRegion->getSubRegOff() + (directSrc ? secondSubRegOffDiff : 0),
                    newRegion,
                    expandSrcRegion->getType());
                if (expandSrcRegion->getRegAccess() != Direct)
                {
                    secondSrcOpnd->setImmAddrOff(expandSrcRegion->getAddrImm() + secondAddrImmedDiff);
                }
                expand_sec_half_op->setSrc(secondSrcOpnd, k);
            }
        }
    }
    expand_sec_half_op->setLineNo(expand_op->getLineNo());

    if (expand_op->getPredicate() || expand_op->getCondMod())
    {
        if (expand_op->getMaskOffset() == 0)
        {
            expand_sec_half_op->setMaskOption(InstOpt_M8);
        }
        else if (expand_op->getMaskOffset() == 16)
        {
            expand_sec_half_op->setMaskOption(InstOpt_M24);
        }
        else if (!(expand_op->opcode() == G4_sel && !(expand_op->getPredicate()) && expand_op->getCondMod()))
        {
            expand_sec_half_op->setMaskOption(newExecSize > 8 ? InstOpt_M16 : InstOpt_M8);
        }
    }
    return expand_sec_half_op;
}

//  in addition, fix the source region to follow the region restriction:
//  1. ExecSize must be greater than or equal to Width.  -- no check for this one
//  2. If ExecSize = Width and HorzStride ? 0, VertStride must be set to Width * HorzStride.
//  3. If ExecSize = Width and HorzStride = 0, there is no restriction on VertStride.
//  4. If Width = 1, HorzStride must be 0 regardless of the values of ExecSize and VertStride.
//  5. If ExecSize = Width = 1, both VertStride and HorzStride must be 0. This defines a scalar.
//  6. If VertStride = HorzStride = 0, Width must be 1 regardless of the value of ExecSize.
//  7. Dst.HorzStride must not be 0.        -- this needs not to be checked.
//  8. VertStride must be used to cross GRF register boundaries. This rule implies that
//      elements within a 'Width' cannot cross GRF boundaries.

void HWConformity::fixSrcRegion(G4_INST* inst)
{

    bool comprInst = isCompressedInst(inst);
    for (int i = 0; i < G4_MAX_SRCS; i++)
    {
        if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion() && !inst->getSrc(i)->isNullReg())
        {
            G4_SrcRegRegion* src = inst->getSrc(i)->asSrcRegRegion();
            const RegionDesc* srcRegion = src->getRegion();
            if (srcRegion->isRegionWH() || srcRegion->isRegionV() || srcRegion->isRegionSW())
            {
                // normalize VxH regions if possible
                if (srcRegion->isRegionWH() && srcRegion->width == inst->getExecSize())
                {
                    // r[a0.0]<E, S> -> r[a0.0]<S;1,0>
                    src->setRegion(builder.createRegionDesc(srcRegion->horzStride, 1, 0));
                }
                // ToDo: add other legalization
                continue;
            }

            //ToDo: most of these checks should be obsolete at this point
            uint16_t vs = srcRegion->vertStride, wd = srcRegion->width, hs = srcRegion->horzStride;
            uint8_t exSize = inst->getExecSize();
            MUST_BE_TRUE(inst->isSend() || exSize >= wd, " Bad source region: Width is greater than execution size.");
            if (comprInst)
            {
                if (G4_Type_Table[inst->getSrc(i)->getType()].byteSize > G4_WSIZE&&
                    wd == exSize &&
                    vs == wd && hs == 1)
                {
                    vs = wd = exSize / 2;
                }
            }
            if (wd == exSize && hs != 0 && vs != wd * hs)
            {
                // <V;E,H> --> <V*H;E,H>
                vs = wd * hs;
            }
            if (wd == 1)
            {
                // <V;1,H> -> <V;1,0> or <0;1,0>
                hs = 0;
                if (1 == exSize)
                    vs = 0;
            }
            if (vs == 0 && hs == 0)
            {
                // <0;N,0> -> <0;1,0>
                wd = 1;
            }
            if (hs == 0 &&
                ((G4_Type_Table[inst->getSrc(i)->getType()].byteSize == G4_WSIZE &&
                    exSize == 32 && vs == 32 && wd == 32) ||
                    (G4_Type_Table[inst->getSrc(i)->getType()].byteSize == G4_DSIZE &&
                        exSize == 16 && vs == 16 && wd == 16)))
            {
                vs = 0;
                wd = 1;
            }

            // check cross GRF (rule 2H)
            // TODO! for the following two cases, split the instruction:
            // source region is like<8;4,1>
            // source region is like<2;4,1>
            if (src->getRegAccess() == Direct && src->crossGRF() && hs != 0)
            {
                // TODO: this is a temp fix
                if ((builder.getPlatform() == GENX_BDW || builder.getPlatform() == GENX_CHV) && vs < wd * hs)
                    continue;
                // check number of elements in first GRF.
                uint16_t execTypeSize = hs * src->getElemSize();
                uint16_t sizeInFirstGRF = GENX_GRF_REG_SIZ - src->getLeftBound() % GENX_GRF_REG_SIZ;
                uint16_t vertSize = vs * G4_Type_Table[src->getType()].byteSize;
                uint16_t numEle = (sizeInFirstGRF + execTypeSize - 1) / execTypeSize;
                uint16_t rowSize = wd * execTypeSize;

                if (sizeInFirstGRF <= vertSize)
                {
                    if (numEle >= wd)
                    {
                        numEle = wd;
                    }
                }
                else if (vs > wd)
                {
                    numEle = sizeInFirstGRF / vertSize * wd +
                        ((sizeInFirstGRF % vertSize > rowSize) ? wd : (sizeInFirstGRF % vertSize + execTypeSize - 1) / execTypeSize);
                }
                // wd is used to cross GRF, change to <vs;1,0>
                if (numEle < wd || (wd >= vs && numEle % wd != 0))
                {

                    wd = 1;
                    if (hs == 0)
                    {
                        vs = 1;
                    }
                    else
                    {
                        vs = hs;
                    }
                    hs = 0;
                }
            }

            if (vs != srcRegion->vertStride || wd != srcRegion->width || hs != srcRegion->horzStride)
            {
                G4_SrcRegRegion* origSrc = inst->getSrc(i)->asSrcRegRegion();
                origSrc->setRegion(builder.createRegionDesc(vs, wd, hs));
            }
        }
    }
    if (inst->getDst() && !inst->hasNULLDst())
    {
        MUST_BE_TRUE(inst->getDst()->getHorzStride() != 0,
            "Bad source region: Width is greater than execution size.");
    }
}

//
//single entry point for HW conformity checks
//
void HWConformityChk(IR_Builder& builder, G4_Kernel& kernel, Mem_Manager& mem)
{
    HWConformity conformity(builder, kernel, mem);
    conformity.chkHWConformity();
}

bool HWConformity::markPackedByteReference(G4_Kernel& kernel, G4_Operand* opnd, G4_INST* inst)
{
    G4_Declare* dcl = NULL, * topdcl = NULL;
    bool foundOptCandidate = false;

    if ((opnd->isSrcRegRegion() || opnd->isDstRegRegion()))
    {
        if (opnd->getBase() && opnd->getBase()->isRegVar())
        {
            dcl = opnd->getBase()->asRegVar()->getDeclare();
            topdcl = dcl->getRootDeclare();
        }
    }

    if (topdcl != NULL &&
        topdcl->getRegFile() == G4_GRF &&
        !(topdcl->getAddressed()))
    {
        if (topdcl->doNotWiden() || inst->mayExceedTwoGRF())
        {
            //send has no regioning so it is certainly illegal to change data layout
            setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
            return false;
        }

        if (opnd->isDstRegRegion() &&
            // check if the opnd has pre-assigned physical regsiter
            !(opnd->asDstRegRegion()->getBase()->asRegVar()->isPhyRegAssigned()) &&
            // check if the opnd is global
            !(kernel.fg.globalOpndHT.isOpndGlobal(opnd)) &&
            // check if the opnd is used as packed byte
            G4_Type_Table[opnd->getType()].byteSize == 1 &&
            dcl->getElemSize() == 1 &&
            opnd->asDstRegRegion()->getHorzStride() == 1 &&
            // check if the instruction is a raw mov
            !inst->isRawMov() &&
            // check if the instruction execution type is word
            // (This should be the most common case that can benefit
            //  from this optimization. It could be extended to other
            //  cases like D execution type).
            G4_Type_Table[inst->getExecType()].byteSize == 2)
        {
            unsigned int leftBound = opnd->asDstRegRegion()->getLeftBound();
            unsigned int rightBound = opnd->asDstRegRegion()->getRightBound();

            if (((rightBound * 2 / G4_GRF_REG_NBYTES - leftBound * 2 / G4_GRF_REG_NBYTES) > 1) ||
                (builder.getPlatform() == GENX_BDW &&
                (rightBound * 2 / G4_GRF_REG_NBYTES != leftBound * 2 / G4_GRF_REG_NBYTES)))
            {
                setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
            }
            else if (getAccessPattern(topdcl) == ACCESS_PATTERN_UNDEF)
            {
                setAccessPattern(topdcl, ACCESS_PATTERN_PACKED_BYTE);
                foundOptCandidate = true;
            }
        }
        else if (opnd->isSrcRegRegion() &&
            // check if the opnd has pre-assigned physical regsiter
            !(opnd->asSrcRegRegion()->getBase()->asRegVar()->isPhyRegAssigned()) &&
            // check if the opnd is global
            !(kernel.fg.globalOpndHT.isOpndGlobal(opnd)) &&
            // check if the opnd is used as packed byte
            G4_Type_Table[opnd->getType()].byteSize == 1 &&
            dcl->getElemSize() == 1 &&
            opnd->asSrcRegRegion()->getRegion()->isContiguous(inst->getExecSize()))
        {
            unsigned int leftBound = opnd->asSrcRegRegion()->getLeftBound();
            unsigned int rightBound = opnd->asSrcRegRegion()->getRightBound();

            if (((rightBound * 2 / G4_GRF_REG_NBYTES - leftBound * 2 / G4_GRF_REG_NBYTES) > 1) ||
                (builder.getPlatform() == GENX_BDW &&
                (rightBound * 2 / G4_GRF_REG_NBYTES != leftBound * 2 / G4_GRF_REG_NBYTES)))
            {
                setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
            }
        }
        else
        {
            setAccessPattern(topdcl, ACCESS_PATTERN_INVALID);
        }
    }

    return foundOptCandidate;
}

G4_Operand* HWConformity::fixPackedByteReference(IR_Builder& builder, G4_Operand* opnd)
{
    G4_Operand* newOpnd = NULL;
    G4_Declare* topdcl = NULL;

    if (opnd->isDstRegRegion() ||
        opnd->isSrcRegRegion())
    {
        topdcl = GetTopDclFromRegRegion(opnd);
    }

    if (topdcl != NULL &&
        getAccessPattern(topdcl) == ACCESS_PATTERN_PACKED_BYTE)
    {
        if (opnd->isDstRegRegion())
        {
            short dst_regoff = opnd->asDstRegRegion()->getRegOff();
            short dst_subregoff = opnd->asDstRegRegion()->getSubRegOff();
            short off = (dst_regoff * G4_GRF_REG_NBYTES + dst_subregoff) * 2;

            dst_regoff = off / G4_GRF_REG_NBYTES;
            dst_subregoff = off % G4_GRF_REG_NBYTES;

            G4_DstRegRegion* newDstOpnd = builder.createDst(
                opnd->getBase()->asRegVar(),
                dst_regoff,
                dst_subregoff,
                2,
                opnd->getType());
            newOpnd = newDstOpnd;
        }
        else if (opnd->isSrcRegRegion())
        {
            short src_regoff = opnd->asSrcRegRegion()->getRegOff();
            short src_subregoff = opnd->asSrcRegRegion()->getSubRegOff();
            short off = (src_regoff * G4_GRF_REG_NBYTES + src_subregoff) * 2;

            src_regoff = off / G4_GRF_REG_NBYTES;
            src_subregoff = off % G4_GRF_REG_NBYTES;

            const RegionDesc* rd = builder.getRegionStride2();
            G4_SrcRegRegion* newSrcOpnd = builder.createSrcRegRegion(opnd->asSrcRegRegion()->getModifier(),
                Direct,
                opnd->getBase()->asRegVar(),
                src_regoff,
                src_subregoff,
                rd,
                opnd->getType());
            newOpnd = newSrcOpnd;
        }
    }

    return newOpnd;
}

void HWConformity::fixDataLayout()
{
    bool changeDataLayout = false;

    for (auto& bb : kernel.fg)
    {
        for (auto& inst : *bb)
        {
            if (G4_Inst_Table[inst->opcode()].n_dst == 1)
            {
                G4_Operand* dst = inst->getDst();

                if (dst)
                {
                    bool foundOptCandidate = markPackedByteReference(kernel, dst, inst);
                    if (changeDataLayout == false && foundOptCandidate)
                    {
                        changeDataLayout = true;
                    }
                }
            }

            for (int i = 0; i < inst->getNumSrc(); i++)
            {
                G4_Operand* src = inst->getSrc(i);

                if (src)
                {
                    markPackedByteReference(kernel, src, inst);
                }
            }
        }
    }

    if (changeDataLayout)
    {
        for (auto& dcl : kernel.Declares)
        {
            G4_Declare* topdcl = dcl->getRootDeclare();

            if (getAccessPattern(topdcl) == ACCESS_PATTERN_PACKED_BYTE)
            {
                dcl->setTotalElems(dcl->getTotalElems() * 2);

                if (dcl != topdcl)
                {
                    G4_Declare* aliasDcl = dcl->getAliasDeclare();
                    unsigned int aliasOffset = dcl->getAliasOffset();
                    dcl->setAliasDeclare(aliasDcl, aliasOffset * 2);
                }
            }
        }

        for (auto& bb : kernel.fg)
        {
            for (auto& inst : *bb)
            {
                if (G4_Inst_Table[inst->opcode()].n_dst == 1)
                {
                    G4_Operand* dst = inst->getDst();
                    G4_Operand* newDst = NULL;

                    if (dst)
                    {
                        newDst = fixPackedByteReference(builder, dst);
                        if (newDst)
                        {
                            inst->setDest(newDst->asDstRegRegion());
                        }
                    }
                }

                for (int i = 0; i < inst->getNumSrc(); i++)
                {
                    G4_Operand* src = inst->getSrc(i);
                    G4_Operand* newSrc = NULL;

                    if (src)
                    {
                        newSrc = fixPackedByteReference(builder, src);
                        if (newSrc)
                        {
                            inst->setSrc(newSrc, i);
                        }
                    }
                }
            }
        }
    }
}

// maintain def-use chain for current inst and the MOV inst generated for its dst
void HWConformity::maintainDU4TempMov(G4_INST* inst, G4_INST* newInst)
{
    if (newInst->getPredicate())
    {
        inst->transferDef(newInst, Opnd_pred, Opnd_pred);
    }

    inst->transferUse(newInst);

    inst->addDefUse(newInst, Opnd_src0);
}

static void expandPlaneMacro(IR_Builder& builder, INST_LIST_ITER it, G4_BB* bb, bool secondHalf)
{
    G4_INST* inst = *it;
    G4_DstRegRegion* dst = inst->getDst();
    G4_SrcRegRegion* src0 = inst->getSrc(0)->asSrcRegRegion();
    G4_SrcRegRegion* src1 = inst->getSrc(1)->asSrcRegRegion();

    G4_SrcRegRegion* srcP = builder.createSrcRegRegion(src0->getModifier(), Direct, src0->getBase(),
        src0->getRegOff(), src0->getSubRegOff(), builder.getRegionScalar(), src0->getType());
    G4_SrcRegRegion* srcQ = builder.createSrcRegRegion(src0->getModifier(), Direct, src0->getBase(),
        src0->getRegOff(), src0->getSubRegOff() + 1, builder.getRegionScalar(), src0->getType());
    G4_SrcRegRegion* srcR = builder.createSrcRegRegion(src0->getModifier(), Direct, src0->getBase(),
        src0->getRegOff(), src0->getSubRegOff() + 3, builder.getRegionScalar(), src0->getType());

    auto u = builder.createSrcWithNewRegOff(src1, src1->getRegOff() + (secondHalf ? 2 : 0));
    auto v = builder.createSrcWithNewRegOff(src1, src1->getRegOff() + (secondHalf ? 3 : 1));

    uint32_t options = inst->getOption();
    if (inst->getExecSize() == 16)
    {
        options &= ~InstOpt_QuarterMasks;
        int maskOffset = inst->getMaskOffset() + (secondHalf ? 8 : 0);
        switch (maskOffset)
        {
        case 0:
            options |= InstOpt_M0;
            break;
        case 8:
            options |= InstOpt_M8;
            break;
        case 16:
            options |= InstOpt_M16;
            break;
        case 24:
            options |= InstOpt_M24;
            break;
        default:
            MUST_BE_TRUE(false, "unexpected offset value");
        }
    }

    G4_Declare* tmpVal = builder.hasNFType() ? nullptr : builder.createTempVar(8, Type_F, Any);
    G4_DstRegRegion* accDst = builder.hasNFType() ?
        builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, Type_NF) :
        builder.Create_Dst_Opnd_From_Dcl(tmpVal, 1);
    G4_INST* madInst = builder.createInternalInst(nullptr, G4_mad, nullptr, false, 8, accDst,
        srcR, u, srcP, options | InstOpt_WriteEnable)->InheritLLVMInst(inst);
    bb->insert(it, madInst);

    G4_Predicate* pred = inst->getPredicate() ? builder.duplicateOperand(inst->getPredicate()) : nullptr;
    G4_CondMod* condMod = inst->getCondMod() ? builder.duplicateOperand(inst->getCondMod()) : nullptr;
    G4_SrcRegRegion* accSrc = builder.hasNFType() ?
        builder.createSrcRegRegion(Mod_src_undef, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, builder.getRegionStride1(), Type_NF) :
        builder.Create_Src_Opnd_From_Dcl(tmpVal, builder.getRegionStride1());
    G4_DstRegRegion* newDst = builder.createDst(dst->getBase(),
        dst->getRegOff() + (secondHalf ? 1 : 0), dst->getSubRegOff(), dst->getHorzStride(), dst->getType());
    G4_INST* secondMadInst = builder.createInternalInst(pred, G4_mad, condMod, inst->getSaturate(), 8, newDst,
        accSrc, v, srcQ, options)->InheritLLVMInst(inst);
    bb->insert(it, secondMadInst);
}

// Replace plane with a macro sequence:
// pln dest:f src0:f src1:f
// -->
// mad acc0:nf src0.3:f src1:f src0.0:f
// mad dest:f acc0:nf src1+1:f src0.1:f
// simd16 pln also needs to be split as the macro is simd8 only

void HWConformity::expandPlaneInst(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;
    MUST_BE_TRUE(inst->opcode() == G4_pln, "expect a plane inst");
    MUST_BE_TRUE(inst->getSrc(0)->isSrcRegRegion(), "src0 must be source reg region");
    MUST_BE_TRUE(inst->getExecSize() == 8 || inst->getExecSize() == 16, " only size 8 and 16 are supported");

    G4_DstRegRegion* dst = inst->getDst();
    if (dst->getRegAccess() == IndirGRF || dst->getHorzStride() > 1)
    {
        inst->setDest(insertMovAfter(it, dst, dst->getType(), bb));
    }
    G4_SrcRegRegion* src0 = inst->getSrc(0)->asSrcRegRegion();
    if (src0->getRegAccess() == IndirGRF)
    {
        // insert move to make src0 direct
        inst->setSrc(insertMovBefore(it, 0, src0->getType(), bb), 0);
    }
    G4_SrcRegRegion* src1 = inst->getSrc(1)->asSrcRegRegion();
    if (src1->getRegAccess() == IndirGRF)
    {
        // insert move to make src1 direct
        inst->setSrc(insertMovBefore(it, 1, src1->getType(), bb), 1);
    }

    expandPlaneMacro(builder, it, bb, false);
    if (inst->getExecSize() == 16)
    {
        expandPlaneMacro(builder, it, bb, true);
    }

    it = bb->erase(it);
}

// plane does not support pln with non-packed dst.
// also fix up plane sources, which don't support modifiers
// returns true if the original plane is deleted
bool HWConformity::fixPlaneInst(INST_LIST_ITER it, G4_BB* bb)
{

    G4_INST* inst = *it;
    if (inst->opcode() == G4_pln)
    {
        if (!builder.doPlane())
        {
            expandPlaneInst(it, bb);
            return true;
        }
        G4_DstRegRegion* dst = inst->getDst();
        if (dst->getHorzStride() != 1)
        {
            G4_DstRegRegion* newDst = insertMovAfter(it, dst, dst->getType(), bb);
            inst->setDest(newDst);
        }

        G4_Operand* src0 = inst->getSrc(0);
        G4_Operand* src1 = inst->getSrc(1);

        // Source modifiers are not supported for pln instruction
        if (src0 &&
            ((src0->isSrcRegRegion() &&
                src0->asSrcRegRegion()->getModifier() != Mod_src_undef) ||
                !builder.isOpndAligned(src0, 16)))
        {
            // src0 needs a temp
            G4_Declare* tmpDcl = builder.createTempVar(4, Type_F,
                GRFALIGN);

            // Before:
            // pln (16) dst, (mod)src0, src1
            //
            // After:
            // mov (4) tmp(0,0):f (mod)src0(r)<4;4,1>:f
            // pln (16) dst, tmp(0,0)<0;1,0>, src1
            G4_DstRegRegion* dstRgn = builder.createDst(
                tmpDcl->getRegVar(),
                0,
                0,
                1,
                Type_F);

            const RegionDesc* rd = builder.createRegionDesc(4, 4, 1);
            G4_SrcRegRegion* srcRgn = builder.createSrcRegRegion(
                src0->asSrcRegRegion()->getModifier(),
                Direct,
                src0->asSrcRegRegion()->getBase(),
                src0->asSrcRegRegion()->getRegOff(),
                src0->asSrcRegRegion()->getSubRegOff(),
                rd,
                Type_F);

            G4_INST* newInst = builder.createMov(4, dstRgn, srcRgn, InstOpt_NoOpt, false);

            bb->insert(it, newInst);

            rd = builder.getRegionScalar();
            G4_SrcRegRegion* newSrcRgn = builder.createSrcRegRegion(
                Mod_src_undef,
                Direct,
                tmpDcl->getRegVar(),
                0,
                0,
                rd,
                Type_F);

            inst->setSrc(newSrcRgn, 0);
            inst->transferDef(newInst, Opnd_src0, Opnd_src0);
            newInst->addDefUse(inst, Opnd_src0);
        }

        if (src1 && src1->isSrcRegRegion() && src1->asSrcRegRegion()->getModifier() != Mod_src_undef)
        {
            // src1 needs a temp
            // For pln instruction src2 is implied from src1 and exec_size
            // When exec_size = 8, src2 is 1 GRF after src1 with size = 1 GRF
            // When exec_size = 16, src2 is 2 GRFs after src1 with size = 2 GRFs
            unsigned short numGRFsToCopy = inst->getExecSize() == 8 ? 2 : 4;

            G4_Declare* tmpDcl = builder.createTempVar((unsigned short)(G4_GRF_REG_NBYTES / G4_Type_Table[Type_F].byteSize * numGRFsToCopy), Type_F,
                Any);

            // Before:
            // pln (16) dst, src0, (mod)src1
            //
            // After:
            // mov (16) tmp(0,0):f (mod)src1(r)<8;8,1>:f
            // mov (16) tmp(2,0):f (mod)src1(r+2)<8;8,1>:f <-- only if exec_size = 16
            // pln (16) dst, src0, tmp(0,0)
            for (int i = 0; i < numGRFsToCopy; i += 2)
            {
                G4_DstRegRegion* dstRgn = builder.createDst(
                    tmpDcl->getRegVar(),
                    (short)i,
                    0,
                    1,
                    Type_F);

                const RegionDesc* rd = builder.createRegionDesc(8, 8, 1);
                G4_SrcRegRegion* srcRgn = builder.createSrcRegRegion(
                    src1->asSrcRegRegion()->getModifier(),
                    Direct,
                    src1->asSrcRegRegion()->getBase(),
                    src1->asSrcRegRegion()->getRegOff() + i,
                    0,
                    rd,
                    Type_F);

                G4_INST* newInst = builder.createMov(16, dstRgn, srcRgn, InstOpt_NoOpt, false);

                bb->insert(it, newInst);

                if (i == 0)
                {
                    G4_SrcRegRegion* newSrcRgn = builder.createSrcRegRegion(
                        Mod_src_undef,
                        Direct,
                        tmpDcl->getRegVar(),
                        0,
                        0,
                        rd,
                        Type_F);

                    inst->setSrc(newSrcRgn, 1);
                    inst->transferDef(newInst, Opnd_src1, Opnd_src0);
                }
                newInst->addDefUse(inst, Opnd_src1);
            }
        }
    }
    return false;
}

void HWConformity::fixImm64(INST_LIST_ITER i,
    G4_BB* bb)
{
    G4_INST* inst = *i;
    for (int j = 0, n_srcs = inst->getNumSrc(); j < n_srcs; j++)
    {
        G4_Operand* src = inst->getSrc(j);
        if (!src ||
            !(src->isImm()) ||
            G4_Type_Table[src->getType()].byteSize != 8)
        {
            continue;
        }
        // a 64bit immediate is supported ONLY for a MOV operation
        bool needsSplit = false;

        if (VISA_WA_CHECK(builder.getPWaTable(), WaDisallow64BitImmMov))
        {
            needsSplit = true;
        }
        if (needsSplit)
        {
            char* immPtr = NULL;
            double dfValue = 0.0f;
            int64_t qValue = 0;

            if (IS_DFTYPE(src->getType()))
            {
                dfValue = src->asImm()->getDouble();
                immPtr = (char*)&dfValue;
            }
            else
            {
                qValue = src->asImm()->getInt();
                immPtr = (char*)&qValue;
            }
            unsigned int lowValue = *((unsigned int*)(immPtr));
            unsigned int highValue = *((unsigned int*)(immPtr + 4));
            G4_Imm* lowImm = builder.createImm((int64_t)lowValue, Type_UD);
            G4_Imm* highImm = builder.createImm((int64_t)highValue, Type_UD);

            G4_Declare* defDcl = NULL;

            defDcl = builder.createTempVar(1, src->getType(), Eight_Word);
            G4_Declare* dcl = builder.createTempVar(2, Type_UD, Eight_Word);
            dcl->setAliasDeclare(defDcl, 0);

            G4_DstRegRegion* dstRegion = builder.Create_Dst_Opnd_From_Dcl(dcl, 1);
            G4_INST* lowMovInst = builder.createMov(1, dstRegion, lowImm, InstOpt_WriteEnable, false);

            bb->insert(i, lowMovInst);

            G4_DstRegRegion* dstRegionNext = builder.Create_Dst_Opnd_From_Dcl(dcl, 1);
            G4_INST* highMovInst = builder.createMov(1, dstRegionNext, highImm, InstOpt_WriteEnable, false);
            dstRegionNext->setSubRegOff(1);
            bb->insert(i, highMovInst);

            inst->transferDef(lowMovInst, Gen4_Operand_Number(j + 1), Opnd_src0);
            lowMovInst->addDefUse(inst, Gen4_Operand_Number(j + 1));
            inst->transferDef(highMovInst, Gen4_Operand_Number(j + 1), Opnd_src0);
            highMovInst->addDefUse(inst, Gen4_Operand_Number(j + 1));

            unsigned short vs = 0, hs = 0, wd = 1; // gen7_5: always 0;1,0
            G4_SrcRegRegion* new_src = builder.Create_Src_Opnd_From_Dcl(defDcl,
                builder.createRegionDesc(vs, wd, hs));
            inst->setSrc(new_src, j);
        }
        else
        {
            if (inst->opcode() != G4_mov)
            {
                inst->setSrc(insertMovBefore(i, j, src->getType(), bb), j);
            }
        }
    }
}

// Check if the source of def_inst is redefined before inst
G4_INST* HWConformity::checkSrcDefInst(G4_INST* inst,
    G4_INST* def_inst,
    uint32_t srcNum)
{
    G4_INST* valid_inst = def_inst;

    if (def_inst != NULL)
    {
        MUST_BE_TRUE(def_inst->opcode() == G4_mov, "def inst must be a mov instruction");

        G4_INST* def_inst1 = NULL;
        for (auto def_it1 = inst->def_begin(), end = inst->def_end(); def_it1 != end; def_it1++)
        {
            if ((*def_it1).second == srcNum + 1)
            {
                def_inst1 = (*def_it1).first;
            }
        }

        if (def_inst1 != NULL)
        {
            G4_INST* def_inst2 = NULL;
            for (auto def_it2 = def_inst->def_begin(), end2 = def_inst->def_end(); def_it2 != end2; def_it2++)
            {
                if ((*def_it2).second == Opnd_src0)
                {
                    def_inst2 = (*def_it2).first;
                }
            }

            if (def_inst1 != def_inst2)
            {
                valid_inst = NULL;
            }
        }
    }

    return valid_inst;
}

/*
    Helper function for fixMixedHFInst
    It assumes dst is not null and is of type DstRegRegion.
    This check must be done before this method is called.
*/
void HWConformity::helperGenerateTempDst(
    G4_BB* bb,
    INST_LIST_ITER instIter,
    G4_INST* inst,
    uint8_t hStride,
    G4_Type tempDstType,
    G4_SubReg_Align subAlign)
{
    G4_DstRegRegion* dst = inst->getDst();
    uint8_t execSize = inst->getExecSize();
    uint8_t dstSize = execSize * G4_Type_Table[tempDstType].byteSize;
    //create a new temp with horizontal stride of 1 (packed)
    //create a move to dst.

    uint32_t numElt = execSize == 1 ? 1 : execSize * hStride;
    if (numElt > 1 && isLowPrecisionFloatTy(tempDstType) && hStride == 1 && subAlign < Eight_Word)
        subAlign = Eight_Word;
    subAlign = getDclAlignment(dstSize, inst, execSize == 1);

    G4_Declare* dcl = builder.createTempVar(numElt, tempDstType, subAlign);


    G4_DstRegRegion* dstRegion = builder.Create_Dst_Opnd_From_Dcl(dcl, hStride);
    inst->setDest(dstRegion);

    const RegionDesc* region =
        execSize == 1 ?
        builder.getRegionScalar() :
        builder.createRegionDesc(execSize * hStride, execSize, hStride);
    G4_SrcRegRegion* srcRegion = builder.Create_Src_Opnd_From_Dcl(dcl, region);

    //creating a mov from temp dst to final destination using original options of fixed instruction
    G4_INST* movInst = builder.createMov(
        execSize, dst, srcRegion, inst->getMaskOption(), false);

    ++instIter;
    //inserting mov after fixed instruction
    bb->insert(instIter, movInst);

    /*
    Need to remove dst from uses list of mulh, and add them to movInst useList
    add movInst to uselist of mulh.
    Add mulh to def instruction list of movInst
    */
    inst->transferUse(movInst);
    inst->addDefUse(movInst, Opnd_src0);
}

/*
    Not Implemented rules:

    3:  (Does this mean align1 doesn't support replication?)
        In Align16 mode, replicate is supported and is coissueable.

    4: (handled in reduce execution size)
        No simd16 in mixed mode when destination is packed f16 for both Align1 and Align16.

            mad(8) r3.xyzw:hf r4.xyzw:f r6.xyzw:hf r7.xyzw:hf

            add(8) r20.0<1>:hf r3<8;8,1>:f r6.0<8;8,1>:hf {Q1}

    5: (we are not producing this type of code)
        No accumulator read access for align16 mixed float

    6: (we do not generate code like this)
        [DevCHV, DevSKL+]: When source is float from accumulator register and destination is half float with a stride of 1, the source must register aligned. i.e., source must have offset zero.

    7: (doesn't seem like it is applicable to our code)
        In Align16, vertical stride can never be zero for f16

    8.a: (handled by another check)
        Math operations for mixed mode,
            - In Align16, only packed format is supported

    11. (handled in reduce execution size)
        [DevCHV, DevSKL, DevBXT]: No simd16 in mixed mode when destination is f32. Instruction Execution size must be no more than 8.

*/
void HWConformity::fixMixedHFInst(G4_BB* bb)
{
    for (auto instIter = bb->begin(); instIter != bb->end(); ++instIter)
    {
        G4_INST* inst = *instIter;

        if (inst->mayExceedTwoGRF())
        {
            continue;
        }

        if (inst->isMath() && builder.getOption(vISA_DisableHFMath))
        {
            auto src0 = inst->getSrc(0);
            auto src1 = inst->getSrc(1);
            auto dst = inst->getDst();
            if (src0 && src0->getType() == Type_HF)
            {
                inst->setSrc(insertMovBefore(instIter, 0, Type_F, bb), 0);
            }

            if (src1 && src1->getType() == Type_HF)
            {
                inst->setSrc(insertMovBefore(instIter, 1, Type_F, bb), 1);
            }

            if (dst && dst->getType() == Type_HF)
            {
                inst->setDest(insertMovAfter(instIter, dst, inst->getExecType2(), bb));
            }
            continue;
        }

        if (VISA_WA_CHECK(builder.getPWaTable(), WaSrc1ImmHfNotAllowed))
        {
            G4_Operand* tSrc1 = inst->getSrc(1);
            if (tSrc1 && tSrc1->isImm() && tSrc1->getType() == Type_HF)
            {
                inst->setSrc(insertMovBefore(instIter, 1, Type_HF, bb), 1);
            }
        }

        if (builder.hasPartialMixMode() && inst->getNumSrc() > 1)
        {
            // no HF on mad src2 or mul src1
            if (inst->isMixedMode())
            {
                auto canSwapSource = [](G4_INST* inst)
                {
                    int srcPos = inst->opcode() == G4_mad ? 2 : 1;
                    G4_Operand* src = inst->getSrc(srcPos);
                    G4_Operand* otherSrc = inst->getSrc(srcPos - 1);
                    if (src->isImm() || otherSrc->getType() != Type_F)
                    {
                        // swapping won't work
                        return false;
                    }
                    if (inst->opcode() == G4_mad)
                    {
                        // src2 has more restrictive regioning, so we can swap only when
                        // src1 is scalar or has contiguous region
                        if (otherSrc->isSrcRegRegion())
                        {
                            G4_SrcRegRegion* other = otherSrc->asSrcRegRegion();
                            if (other->getRegion()->isScalar() ||
                                other->getRegion()->isContiguous(inst->getExecSize()))
                            {
                                return true;
                            }
                        }
                        return false;
                    }
                    else
                    {
                        // swapping is always legal for mul
                        return true;
                    }
                };
                if (inst->opcode() == G4_mad)
                {
                    if (isLowPrecisionFloatTy(inst->getSrc(2)->getType()))
                    {
                        if (canSwapSource(inst))
                        {
                            inst->swapSrc(1, 2);
                        }
                        else
                        {
                            inst->setSrc(insertMovBefore(instIter, 2, Type_F, bb), 2);
                        }
                    }
                    // at this point src2 must be F. If dst is HF, it must be aligned to
                    // same subreg as src2 if src2 is non-scalar
                    bool nonScalarSrc2 = inst->getSrc(2)->isSrcRegRegion() &&
                        !inst->getSrc(2)->asSrcRegRegion()->getRegion()->isScalar();
                    if (isLowPrecisionFloatTy(inst->getDst()->getType()) && nonScalarSrc2)
                    {
                        if (!builder.isOpndAligned(inst->getDst(), GENX_GRF_REG_SIZ))
                        {
                            inst->setDest(insertMovAfter(instIter, inst->getDst(), Type_F, bb, GRFALIGN));
                        }
                        if (!builder.isOpndAligned(inst->getSrc(2), GENX_GRF_REG_SIZ))
                        {
                            inst->setSrc(insertMovBefore(instIter, 2, inst->getSrc(2)->getType(), bb, GRFALIGN), 2);
                        }
                    }
                }
                else if (inst->opcode() == G4_mul && isLowPrecisionFloatTy(inst->getSrc(1)->getType()))
                {
                    if (canSwapSource(inst))
                    {
                        inst->swapSrc(0, 1);
                    }
                    else
                    {
                        inst->setSrc(insertMovBefore(instIter, 1, Type_F, bb), 1);
                    }
                }
            }
        }

        // The execution size must be no more than 8 when half-floats are used in source or destination operand.
        // ToDO: move this to fixmathinst
        if (inst->getExecSize() > builder.getNativeExecSize())
        {
            if (inst->opcode() == G4_math &&
                inst->getDst()->getType() == Type_HF &&
                inst->getSrc(0)->getType() == Type_HF &&
                (!inst->getSrc(1) || inst->getSrc(1)->getType() == Type_HF))
            {
                evenlySplitInst(instIter, bb);
            }
        }

        G4_DstRegRegion* dst = inst->getDst();
        if (INST_FLOAT_SRC_ONLY(inst->opcode()) && dst && !dst->isNullReg() && isLowPrecisionFloatTy(dst->getType()))
        {
            helperGenerateTempDst(bb, instIter, inst, 1, Type_F);
        }

        if (!inst->isMixedMode())
            continue;

        if (inst->getDst() && !inst->getDst()->isNullReg())
            dst = inst->getDst();

        if ((VISA_WA_CHECK(builder.getPWaTable(), WaMixModeSelInstDstNotPacked) ||
            VISA_WA_CHECK(builder.getPWaTable(), WaFloatMixedModeSelNotAllowedWithPackedDestination)) &&
            inst->opcode() == G4_sel &&
            dst &&
            (VISA_WA_CHECK(builder.getPWaTable(), WaMixModeSelInstDstNotPacked) || dst->getHorzStride() == 1) &&
            dst->getType() == Type_HF)
        {
            helperGenerateTempDst(bb, instIter, inst, 1, Type_F);
        }

        if (!inst->isMixedMode())
            continue;

        if (builder.getPlatform() >= GENX_CHV)
        {
            // no SIMD16 mix mode instruction
            if (inst->getExecSize() > builder.getNativeExecSize() && inst->isMixedMode())
            {
                evenlySplitInst(instIter, bb, false);
                //instruction was split, and new instruction inserted before
                //going back to previous instruction to double check it still confirms.
                --instIter;
                inst = *instIter;
            }
        }

        /*
            12: [DevCHV, DevSKL]: Indirect Addressing on source is not supported when source and destination data types are mixed float.
        */
        if (builder.getPlatform() == GENX_CHV || builder.getPlatform() == GENX_SKL)
        {
            for (uint8_t i = 0; i < inst->getNumSrc(); ++i)
            {
                G4_Operand* src = inst->getSrc(i);
                if (src == nullptr || !src->isSrcRegRegion() || !src->asSrcRegRegion()->isIndirect())
                {
                    continue;
                }
                inst->setSrc(insertMovBefore(instIter, i, src->getType(), bb), i);
            }
        }

        if (inst->getDst()->getBase()->isRegVar() &&
            inst->getDst()->getType() == Type_HF &&
            inst->getDst()->getHorzStride() == 1)
        {
            inst->getDst()->getBase()->asRegVar()->getDeclare()->setSubRegAlign(Eight_Word);
        }
    }
}

// Fix for packed half types on BDW.
// Conversions from F to packed HF are not supported on this platform,
// only unpacked HF is supported on destination.
// When we encounter an instruction with HF type on destination with <1> stride
// and float on source, add an additional mov that handles unpacking.
void HWConformity::fixPackedHFConversions(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;
    G4_DstRegRegion* dst = inst->getDst();
    if (dst && dst->getType() == Type_HF && dst->getHorzStride() == 1 &&
        getTypeSize(inst->getExecType()) > 2)
    {
        helperGenerateTempDst(bb, it, inst, 2, Type_HF);
    }
}

void HWConformity::fixSrc2(INST_LIST_ITER it, G4_BB* bb, bool swapSrc0and2)
{
    G4_INST* inst = *it;
    int srcPos = swapSrc0and2 ? 0 : 2; // unfortunate side effect of vISA mad and Gen mad having difference src order
    assert(inst->getNumSrc() == 3 && "expect 3-src inst");
    if (builder.noSrc2Regioning())
    {
        auto src = inst->getSrc(srcPos);
        // we have to make sure src2 and dst are aligned
        // Promote src2's type to f if mix mode is supported.
        // e.g.,
        // mad (4) r10.0<1>:f src0 src1 r12.0<1>:hf  --> f
        // mad (4) r10.0<2>:hf src0 src1 r12.0<1>:hf --> f
        // mad (4) r10.0<1>:hf src0 src1 r12.0<2>:hf --> hf
        // mad (4) r10.0<2>:hf src0 src1 r12.1<2>:hf --> f
        // ditto for 3-src inst with int types
        G4_Type srcTy = src->getType();
        unsigned short dstEltSz = inst->getDst()->getExecTypeSize();
        if (dstEltSz >= 4)
        {
            if (IS_SIGNED_INT(srcTy))
            {
                srcTy = Type_D;
            }
            else if (IS_UNSIGNED_INT(srcTy))
            {
                srcTy = Type_UD;
            }
            else if (builder.hasMixMode() && builder.getMixModeType() == srcTy)
            {
                // we can change operand type to F to save one move
                srcTy = Type_F;
            }
        }
        inst->setSrc(insertMovBefore(it, srcPos, srcTy, bb, GRFALIGN), srcPos);

        // Check if dst stride aligns with src2.
        if (dstEltSz != G4_Type_Table[srcTy].byteSize)
        {
            inst->setDest(insertMovAfter(it, inst->getDst(), inst->getDst()->getType(), bb, GRFALIGN));
        }
    }
}

void HWConformity::fixVxHFloat64b(INST_LIST_ITER it, G4_BB* bb)
{
    // at this point VxH region should only be on src0
    G4_INST* inst = *it;
    G4_SrcRegRegion* src0 = inst->getSrc(0) && inst->getSrc(0)->isSrcRegRegion() ?
        inst->getSrc(0)->asSrcRegRegion() : nullptr;

    if (src0 && src0->getRegAccess() == IndirGRF && src0->getRegion()->isRegionWH())
    {
        auto type = src0->getType();
        if (type == Type_HF || type == Type_F)
        {
            auto intType = type == Type_HF ? Type_UW : Type_UD;
            if (inst->isRawMov())
            {
                // directly change the dst/src type to int
                inst->getDst()->setType(intType);
                src0->setType(intType);
            }
            else
            {
                // generate a copy move using int type
                // FIXME: code is a bit hacky, may want to change insertMovBefore
                // so that we could specify the move type
                auto origType = src0->getType();
                auto origMod = src0->getModifier();
                src0->setType(intType);
                src0->setModifier(Mod_src_undef);
                auto newSrc = insertMovBefore(it, 0, intType, bb);
                newSrc->asSrcRegRegion()->setType(origType);
                newSrc->asSrcRegRegion()->setModifier(origMod);
                inst->setSrc(newSrc, 0);
            }
        }
        else if (getTypeSize(type) == 8)
        {
            int numDwords = inst->getExecSize() * 2;
            G4_Declare* tmpSrc = builder.createTempVar(numDwords / 2, src0->getType(), Any);
            const RegionDesc* newRegion = builder.getRegionStride1();
            copyDwordsIndirect(tmpSrc, src0, numDwords, bb, it);
            G4_SrcRegRegion* tmpSrcOpnd = builder.createSrcRegRegion(src0->getModifier(),
                Direct, tmpSrc->getRegVar(), 0, 0, newRegion, tmpSrc->getElemType());
            inst->setSrc(tmpSrcOpnd, 0);
        }
    }
}

bool HWConformity::fixIntToHFMove(G4_BB* bb)
{
    // int to HF move requires dst to have stride 2, which would result in
    // an illegal SIMD32 inst. So we split in this case
    // we put it in a separate pass so that the split instructions may be legalized later
    bool changed = false;
    for (auto I = bb->begin(), E = bb->end(); I != E; ++I)
    {
        auto inst = *I;
        if (inst->opcode() == G4_mov && inst->getDst()->getType() == Type_HF &&
            IS_INT(inst->getSrc(0)->getType()))
        {
            if (inst->getExecSize() * 2 * 2 > getGRFSize() * 2)
            {
                evenlySplitInst(I, bb);
                changed = true;
            }
        }
    }
    return changed;
}

void HWConformity::fixPredCtrl(INST_LIST_ITER it, G4_BB* bb)
{
    G4_INST* inst = *it;
    G4_Predicate* pred = inst->getPredicate();
    if (pred && (pred->getControl() == PRED_ANY_WHOLE || pred->getControl() == PRED_ALL_WHOLE))
    {
        // we need WA if pred's size is greater than inst's exec size
        // and the platform does not support predctrl group size (indicated by the fact we
        // have PRED_ANY_WHOLE and PRED_ALL_WHOLE)
        // The case where pred size is less than inst's exec size is already undefined
        // even with predCtrl group size..
        G4_Declare* flagDcl = pred->getTopDcl();
        if (flagDcl->getNumberFlagElements() > inst->getExecSize())
        {
            // convert
            // (f0.any32h) sel (1) ...
            // into
            // cmp (1) [ne] f1 f0 0
            // (f1) sel (1) ...
            // and
            // (f0.all32h) sel (1) ...
            // into
            // cmp (1) [e] f1 f0 0xFFFFFFFF
            //
            // if f0 happens to be < 16 elements we have to clear upper bits as well in case it has garbage values
            assert(!inst->getCondMod() && "currently don't handle an instruction with conditional modifier");
            assert((inst->isWriteEnableInst() || bb->isAllLaneActive()) && "don't handle instruction in SIMD CF for now");
            G4_Declare* tmpFlag = builder.createTempFlag(1);
            G4_Type flagType = flagDcl->getNumberFlagElements() == 32 ? Type_UD : Type_UW;
            uint32_t allOneMask = (uint32_t)((1ULL << flagDcl->getNumberFlagElements()) - 1);
            G4_Declare* cmpSrc0Flag = flagDcl;
            if (flagDcl->getNumberFlagElements() < 16)
            {
                // clear the upper bit of the flag
                auto andInst = builder.createBinOp(G4_and, 1, builder.Create_Dst_Opnd_From_Dcl(tmpFlag, 1),
                    builder.Create_Src_Opnd_From_Dcl(flagDcl, builder.getRegionScalar()),
                    builder.createImm(allOneMask, Type_UW), InstOpt_WriteEnable, false);
                bb->insert(it, andInst);
                cmpSrc0Flag = tmpFlag;
            }
            G4_CondMod* condMod = builder.createCondMod(pred->getControl() == PRED_ANY_WHOLE ? Mod_ne : Mod_e,
                tmpFlag->getRegVar(), 0);

            G4_Imm* immVal = builder.createImm(pred->getControl() == PRED_ANY_WHOLE ? 0 : allOneMask, flagType);
            // cmp needs to be as wide as the original inst but is uniform and NoMask otherwise
            auto cmpInst = builder.createInternalInst(nullptr, G4_cmp, condMod, false, inst->getExecSize(), builder.createNullDst(flagType),
                builder.createSrcRegRegion(Mod_src_undef, Direct, cmpSrc0Flag->getRegVar(), 0, 0, builder.getRegionScalar(), flagType),
                immVal, InstOpt_WriteEnable);
            bb->insert(it, cmpInst);
            inst->setPredicate(builder.createPredicate(pred->getState(), tmpFlag->getRegVar(), 0));
        }
    }
}


/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "../Timer.h"

#include <cmath>

using namespace vISA;

#define SET_DATAPORT_MESSAGE_TYPE(dest, value)\
    dest |= value << 14;

#define MESSAGE_SPECIFIC_CONTROL 8


static unsigned int getObjWidth(
    unsigned blockWidth, unsigned blockHeight, G4_Declare * dcl)
{
    // makes sure io_width is divisible by 4
    unsigned ioWidth = (blockWidth + TypeSize(Type_D) - 1) & (~(TypeSize(Type_D) - 1));
    // gets next power of 2 size
    return Round_Up_Pow2(ioWidth / dcl->getElemSize()) * dcl->getElemSize();
}


/*
* Translates Media Block read CISA inst.
*
* read(I, X, Y, matrix<int,C,R> M)
* Assume C = R = 8 then code shoud look like
*
* .declare  VX Base=m ElementSize=4 Type=ud Total=8
* .declare  VY Base=r ElementSize=4 Type=ud Total=8
*
* mov  (8)     VX(0,0)<1>,  r0.0:ud
* mov  (1)     VX(0,2)<1>,  0x0007001f   // 8 rows, 32 bytes
* mov  (1)     VX(0,1)<1>,  Y
* mov  (1)     VX(0,0)<1>,  X
* send (8)     VY(0,0)<1>,  VX(0,0),    null,  0x04186000
* mov  (8)     M(0,0)<1>,   VY(0,0)
*
* 0x0007001f == (R-1)<<16 + C * sizeof(el_type) - 1;
*
* 0x04186000 ==
*  (((ObjectSize - 1) / numEltPerGRF<Type_UB>() + 1)) << 16 +
*          0x4100000 + 0x6000 + I;
*
* ObjectSize = RoundUpPow2(C) * R * sizeof(el_type);
*/
int IR_Builder::translateVISAMediaLoadInst(
    MEDIA_LD_mod mod,
    G4_Operand* surface,
    unsigned planeID,
    unsigned blockWidth,
    unsigned blockHeight,
    G4_Operand* xOffOpnd,
    G4_Operand* yOffOpnd,
    G4_DstRegRegion* dstOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    unsigned temp;

    unsigned objWidth = 0;
    if (blockWidth != 0)
    {
        objWidth = getObjWidth(blockWidth, blockHeight, dstOpnd->getBase()->asRegVar()->getDeclare());
    }
    unsigned obj_size = objWidth * blockHeight;

    /* mov (8)      VX(0,0)<1>,  r0:ud  */
    // add dcl for VX
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    // create MOV inst
    createMovR0Inst(dcl, 0, 0, true);
    /* mov (1)      VX(0,2)<1>,    CONST[R,C]  */
    temp = (blockHeight - 1) << 16 | (blockWidth - 1);
    createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(temp, Type_UD), true);
    /* mov (1)     VX(0,0)<1>,    X  */
    createMovInst(dcl, 0, 0, g4::SIMD1, NULL, NULL, xOffOpnd, true);
    /* mov (1)     VX(0,1)<1>,   Y  */
    createMovInst(dcl, 0, 1, g4::SIMD1, NULL, NULL, yOffOpnd, true);

    // send's operands preparation
    // create a currDst for VX
    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

    // mediaread overwrites entire GRF
    bool via_temp = false;
    G4_Operand *original_dst = NULL;
    G4_Declare *new_dcl = NULL;

    if (obj_size < numEltPerGRF<Type_UB>())
    {
        via_temp = true;
    }
    else
    {
        unsigned byte_subregoff =
            dstOpnd->asDstRegRegion()->getSubRegOff() * dstOpnd->getTypeSize();
        G4_VarBase *base = dstOpnd->asDstRegRegion()->getBase();
        G4_Declare *dcl = base->asRegVar()->getDeclare();

        if (byte_subregoff  % numEltPerGRF<Type_UB>() != 0)
        {
            via_temp = true;
        }
        else
        {
            G4_Declare *aliasdcl = dcl;
            bool false_alias_align = false;
            while (aliasdcl->getAliasDeclare()) {
                if (aliasdcl->getAliasOffset() % numEltPerGRF<Type_UB>() != 0) {
                    false_alias_align = true;
                    break;
                }
                aliasdcl = aliasdcl->getAliasDeclare();
            }
            if (false_alias_align) {
                via_temp = true;
            }
        }
    }

    if (via_temp == true)
    {
        original_dst = dstOpnd;
        new_dcl = createTempVar(numEltPerGRF<Type_UB>()/TypeSize(Type_UD),
            Type_UD, getGRFAlign());
        G4_DstRegRegion* tmp_dst_opnd = createDst(
            new_dcl->getRegVar(),
            0,
            0,
            1,
            Type_UD);

        dstOpnd = tmp_dst_opnd;
    }

    G4_DstRegRegion* d = checkSendDst(dstOpnd->asDstRegRegion());

    temp = 0;
    if ((mod == MEDIA_LD_top) || (mod == MEDIA_LD_top_mod)) {
        temp += 0x6 << MESSAGE_SPECIFIC_CONTROL;    // Read top fields
    } else if ((mod == MEDIA_LD_bottom) || (mod == MEDIA_LD_bottom_mod)) {
        temp += 0x7 << MESSAGE_SPECIFIC_CONTROL;    // Read bottom fields
    }

    SET_DATAPORT_MESSAGE_TYPE(temp, DC1_MEDIA_BLOCK_READ)

    temp += planeID;

    G4_ExecSize send_exec_size(getGenxDataportIOSize());
    if (IS_WTYPE(d->getType()))
    {
        send_exec_size *= 2;
    }

    createSendInst(
        NULL,
        d,
        payload,
        1,
        (obj_size - 1) / numEltPerGRF<Type_UB>() + 1,
        send_exec_size,
        temp,
        SFID::DP_DC1,
        1,
        SendAccess::READ_ONLY,
        surface,
        NULL,
        InstOpt_WriteEnable,
        false);

    if (via_temp)
    {
        G4_Declare *new_dcl2 = createTempVar(
            numEltPerGRF<Type_UB>()/original_dst->getTypeSize(),
            original_dst->getType(), getGRFAlign());

        new_dcl2->setAliasDeclare(new_dcl, 0);

        unsigned short remained_ele = obj_size / original_dst->getTypeSize();
        // max execution size is 32
        G4_ExecSize curr_exec_size = G4_ExecSize(getNativeExecSize() * 2);
        unsigned char curr_offset = 0;

        G4_Type dstType = original_dst->getType();
        while (remained_ele >= 1)
        {
            short dst_regoff = original_dst->asDstRegRegion()->getRegOff();
            short dst_subregoff = original_dst->asDstRegRegion()->getSubRegOff();
            if (remained_ele >= curr_exec_size)
            {
                G4_SrcRegRegion *tmp_src_opnd = createSrc(
                    new_dcl2->getRegVar(),
                    0,
                    curr_offset,
                    curr_exec_size == g4::SIMD1 ? getRegionScalar() : getRegionStride1(),
                    original_dst->getType());

                dst_subregoff += curr_offset;
                short ele_per_grf = numEltPerGRF<Type_UB>()/TypeSize(dstType);
                if (dst_subregoff >= ele_per_grf)
                {
                    dst_regoff += 1;
                    dst_subregoff -= ele_per_grf;
                }
                G4_DstRegRegion* tmp_dst_opnd = createDst(
                    original_dst->asDstRegRegion()->getBase(),
                    dst_regoff,
                    dst_subregoff,
                    1,
                    original_dst->getType());

                createMov(curr_exec_size, tmp_dst_opnd, tmp_src_opnd, InstOpt_WriteEnable, true);
                curr_offset += curr_exec_size;
                remained_ele -= curr_exec_size;
            }
            curr_exec_size /= 2;
        }
    }

    return VISA_SUCCESS;
}

/*
* Translates Media Block write CISA inst.
*
* write(I, X, Y, matrix<int,C,R> M)
* Assume C = R = 8 then code shoud look like
*
* .declare  VX Base=m ElementSize=4 Type=ud Total=72
* .declare  VY Base=m ElementSize=4 Type=ud Total=64 ALIAS(VX,32)
*
* mov  (8)     VX(0,0)<1>,  r0.0:ud
* mov  (64)    VY(0,0)<1>,  M
* mov  (1)     VX(0,2)<1>,  0x0007001f   // 8 rows, 32 bytes
* mov  (1)     VX(0,1)<1>,  Y
* mov  (1)     VX(0,0)<1>,  X
* send (8)     null<1>,  VX(0,0),  null,   0x05902000
*
* 72 = 8 + C * R
* 0x0007001f is (R-1)<<16 + C * sizeof(el_type) - 1
*
* 0x05902000 ==
*  ((((ObjectSize - 1) / numEltPerGRF<Type_UB>() + 1)) + 1)<<20 +
*          0x5000000 + 0x2000 + I
* ObjectSize = RoundUpPow2(C) * R * sizeof(el_type)
*/
int IR_Builder::translateVISAMediaStoreInst(
    MEDIA_ST_mod mod,
    G4_Operand* surface,
    unsigned planeID,
    unsigned blockWidth,
    unsigned blockHeight,
    G4_Operand* xOffOpnd,
    G4_Operand* yOffOpnd,
    G4_SrcRegRegion* srcOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    int objWidth = 0;
    if (blockWidth != 0)
    {
        objWidth = getObjWidth(blockWidth, blockHeight, srcOpnd->getBase()->asRegVar()->getDeclare());
    }
    unsigned obj_size = objWidth * blockHeight;
    unsigned int new_obj_size = obj_size;

    auto setTopBottomForDesc = [](uint32_t desc, MEDIA_ST_mod mod)
    {
        if (mod == MEDIA_ST_top)
        {
            return desc + (0x6 << MESSAGE_SPECIFIC_CONTROL);    // Write top fields
        }
        else if (mod == MEDIA_ST_bottom)
        {
            return desc + (0x7 << MESSAGE_SPECIFIC_CONTROL);    // Write bottom fields
        }
        return desc;
    };

    bool forceSplitSend = shouldForceSplitSend(surface);
    if (forceSplitSend || useSends())
    {
        // use split send
        G4_Declare *headerDcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
        createMovR0Inst(headerDcl, 0, 0, true);
        /* mov (1)      VX(0,2)<1>,    CONST[R,C]  */
        uint32_t temp = (blockHeight - 1) << 16 | (blockWidth - 1);
        createMovInst(headerDcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(temp, Type_UD), true);

        /* mov (1)     VX(0,0)<1>,    X  */
        createMovInst(headerDcl, 0, 0, g4::SIMD1, NULL, NULL, xOffOpnd, true);

        /* mov (1)     VX(0,1)<1>,   Y  */
        createMovInst(headerDcl, 0, 1, g4::SIMD1, NULL, NULL, yOffOpnd, true);

        G4_SrcRegRegion* headerOpnd = createSrcRegRegion(headerDcl, getRegionStride1());

        unsigned msgDesc = setTopBottomForDesc(0, mod);
        SET_DATAPORT_MESSAGE_TYPE(msgDesc, DC1_MEDIA_BLOCK_WRITE)

            msgDesc += planeID;
        // message length = 1, response length = 0, header present = 1
        msgDesc += (1 << getSendMsgLengthBitOffset()) + (1 << getSendHeaderPresentBitOffset());
        G4_DstRegRegion *dstOpnd = createNullDst(Type_UD);

        unsigned extMsgLength = (obj_size - 1) / numEltPerGRF<Type_UB>() + 1;
        uint16_t extFuncCtrl = 0;

        G4_SendDescRaw * desc = createSendMsgDesc(msgDesc, 0, 1, SFID::DP_DC1,
            extMsgLength, extFuncCtrl, SendAccess::WRITE_ONLY, surface);

        createSplitSendInst(
            nullptr, dstOpnd, headerOpnd, srcOpnd, g4::SIMD8, desc, InstOpt_WriteEnable, false);
    }
    else
    {
        uint32_t temp =  new_obj_size/TypeSize(Type_UD) + getGenxDataportIOSize();

        G4_Declare *dcl = createSendPayloadDcl(temp, Type_UD);

        /* mov  (c*r)    VX(1,0)<1>,  M */
        /* decl for data to write */
        temp =  obj_size/TypeSize(Type_UD);

        createMovSendSrcInst(dcl, 1, 0, temp, srcOpnd, InstOpt_WriteEnable);

        createMovR0Inst(dcl, 0, 0, true);

        /* mov (1)      VX(0,2)<1>,    CONST[R,C]  */
        temp = (blockHeight - 1) << 16 | (blockWidth - 1);
        createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(temp, Type_UD), true);

        /* mov (1)     VX(0,0)<1>,    X  */
        createMovInst(dcl, 0, 0, g4::SIMD1, NULL, NULL, xOffOpnd, true);

        /* mov (1)     VX(0,1)<1>,   Y  */
        createMovInst(dcl, 0, 1, g4::SIMD1, NULL, NULL, yOffOpnd, true);

        // send's operands preparation
        /* Size of whole operand in UINT elements */
        G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

        uint32_t funcCtrl = setTopBottomForDesc(0, mod);
        SET_DATAPORT_MESSAGE_TYPE(funcCtrl, DC1_MEDIA_BLOCK_WRITE);

        funcCtrl += planeID;
        G4_DstRegRegion *post_dst_opnd = createNullDst(Type_UD);

        createSendInst(
            NULL,
            post_dst_opnd,
            payload,
            ((obj_size - 1) / numEltPerGRF<Type_UB>() + 1) + 1,
            0,
            G4_ExecSize(getGenxDataportIOSize()),
            funcCtrl,
            SFID::DP_DC1,
            1,
            SendAccess::WRITE_ONLY,
            surface,
            NULL,
            InstOpt_WriteEnable,
            false);
    }

    return VISA_SUCCESS;
}


int IR_Builder::translateVISAVmeImeInst(
    uint8_t stream_mode,
    uint8_t search_ctrl,
    G4_Operand* surfaceOpnd,
    G4_Operand* uniInputOpnd,
    G4_Operand* imeInputOpnd,
    G4_Operand* ref0Opnd,
    G4_Operand* ref1Opnd,
    G4_Operand* costCenterOpnd,
    G4_DstRegRegion* outputOpnd)

{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    // add dcl for VX
    unsigned input_size_dw;

    unsigned uni_input_size;

    uni_input_size = 4;

    if ((COMMON_ISA_VME_STREAM_MODE) stream_mode != VME_STREAM_IN &&
        (COMMON_ISA_VME_STREAM_MODE) stream_mode != VME_STREAM_IN_OUT) {
        input_size_dw = (uni_input_size + 2)*32/TypeSize(Type_UD);
    } else if ((COMMON_ISA_VME_SEARCH_CTRL) search_ctrl == VME_SEARCH_DUAL_REF_DUAL_REC) {
        input_size_dw = (uni_input_size + 6)*32/TypeSize(Type_UD);
    } else {
        input_size_dw = (uni_input_size + 4)*32/TypeSize(Type_UD);
    }

    G4_Declare *dcl = createSendPayloadDcl(input_size_dw, Type_UD);

    // mov  (96)    VX(0,0)<1>,  UNIInput
    createMovSendSrcInst(dcl, 0, 0,
        uni_input_size*32/TypeSize(Type_UD),
        uniInputOpnd, InstOpt_WriteEnable);

    // mov  (192)   VX(3,0)<1>,  IMEInput
    createMovSendSrcInst(dcl, (short) uni_input_size, 0,
        (input_size_dw - uni_input_size*32/TypeSize(Type_UD)),
        imeInputOpnd, InstOpt_WriteEnable);

    // and  (1)     VX(0,13)<1>, VX(0,13):ub, 0xF8
    G4_DstRegRegion *tmp_dst1_opnd = createDst(
        dcl->getRegVar(),
        0,
        13,
        1,
        Type_UB);

    G4_SrcRegRegion *tmp_src1_opnd = createSrc(
        dcl->getRegVar(),
        0,
        13,
        getRegionScalar(),
        Type_UB);

    createBinOp(G4_and, g4::SIMD1, tmp_dst1_opnd, tmp_src1_opnd,
        createImm(0xF8, Type_UW), InstOpt_WriteEnable, true);

    // or   (1)     VX(0,13)<1>, VX(0,13):ub, searchCtrl
    G4_DstRegRegion *tmp_dst2_opnd = createDst(
        dcl->getRegVar(),
        0,
        13,
        1,
        Type_UB);

    G4_SrcRegRegion *tmp_src2_opnd = createSrc(
        dcl->getRegVar(),
        0,
        13,
        getRegionScalar(),
        Type_UB);

    createBinOp(G4_or, g4::SIMD1, tmp_dst2_opnd, tmp_src2_opnd,
        createImm(search_ctrl, Type_UW), InstOpt_WriteEnable, true);

    // mov  (2)     VA(0,0)<1>,  ref0
    // since ref0 is converted from UW to UD, move it as 1 UD
    createMovSendSrcInst(dcl, 0, 0, 1, ref0Opnd, InstOpt_WriteEnable);

    createMovSendSrcInst(dcl, 0, 1, 1, ref1Opnd, InstOpt_WriteEnable);

    createMovSendSrcInst(dcl, 3, 0, 8, costCenterOpnd, InstOpt_WriteEnable);

    // send's operands preparation
    // create a currDst for VX
    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

    G4_DstRegRegion* d = checkSendDst(outputOpnd->asDstRegRegion());

    unsigned temp = 0;            // Bit 7-0 of message descriptor
    temp += 0x2 << 13;            // Bit 14-13 of message descriptor
    temp += stream_mode << 15;     // Bit 16-15 of message descriptor

    unsigned regs2rcv;

    if ((COMMON_ISA_VME_STREAM_MODE) stream_mode != VME_STREAM_OUT &&
        (COMMON_ISA_VME_STREAM_MODE) stream_mode != VME_STREAM_IN_OUT) {
        regs2rcv = 224/numEltPerGRF<Type_UB>();
    } else if ((COMMON_ISA_VME_SEARCH_CTRL) search_ctrl == VME_SEARCH_DUAL_REF_DUAL_REC) {
        regs2rcv = 352/numEltPerGRF<Type_UB>();
    } else {
        regs2rcv = 288/numEltPerGRF<Type_UB>();
    }

    createSendInst(
        NULL,
        d,
        payload,
        input_size_dw / getGenxDataportIOSize(),
        regs2rcv,
        G4_ExecSize(getGenxDataportIOSize()),
        temp,
        SFID::VME,
        true,
        SendAccess::READ_ONLY,
        surfaceOpnd,
        NULL,
        InstOpt_WriteEnable,
        false);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISAVmeSicInst(
    G4_Operand* surfaceOpnd,
    G4_Operand* uniInputOpnd,
    G4_Operand* sicInputOpnd,
    G4_DstRegRegion* outputOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    unsigned uni_input_size;

    uni_input_size = 4;

    // add dcl for VX
    unsigned input_size_dw = (uni_input_size + 4)*32/TypeSize(Type_UD);

    G4_Declare *dcl = NULL;
    G4_Declare *topDcl = uniInputOpnd->getTopDcl();

    // check if uniInputOpnd and sicInputOpnd are alias to the
    // same top level decl with consistent payload layout
    if ((topDcl == sicInputOpnd->getTopDcl()) &&
        (uniInputOpnd->getByteOffset() == 0) &&
        (sicInputOpnd->getByteOffset() == uni_input_size*32) &&
        (topDcl->getByteSize() >= uni_input_size*32 + 128))
    {
        dcl = topDcl;
    }
    else
    {
        dcl = createSendPayloadDcl(input_size_dw, Type_UD);
        // mov  (96)    VX(0,0)<1>,  UNIInput
        createMovSendSrcInst(dcl, 0, 0, uni_input_size*32/TypeSize(Type_UD), uniInputOpnd, InstOpt_WriteEnable);
        // mov  (128)   VX(3,0)<1>,  SICInput
        createMovSendSrcInst(dcl, (short) uni_input_size, 0, 128/TypeSize(Type_UD), sicInputOpnd, InstOpt_WriteEnable);
    }

    // send's operands preparation
    // create a currDst for VX
    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

    G4_DstRegRegion* d = checkSendDst(outputOpnd->asDstRegRegion());

    unsigned temp = 0;            // Bit 7-0 of message descriptor
    temp += 0x1 << 13;            // Bit 14-13 of message descriptor

    unsigned regs2rcv = 7;

    createSendInst(
        NULL,
        d,
        payload,
        input_size_dw / getGenxDataportIOSize(),
        regs2rcv,
        G4_ExecSize(getGenxDataportIOSize()),
        temp,
        SFID::CRE,
        true,
        SendAccess::READ_ONLY,
        surfaceOpnd,
        NULL,
        InstOpt_WriteEnable,
        false);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISAVmeFbrInst(
    G4_Operand* surfaceOpnd,
    G4_Operand* unitInputOpnd,
    G4_Operand* fbrInputOpnd,
    G4_Operand* fbrMbModOpnd,
    G4_Operand* fbrSubMbShapeOpnd,
    G4_Operand* fbrSubPredModeOpnd,
    G4_DstRegRegion* outputOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    unsigned uni_input_size;

    uni_input_size = 4;

    // add dcl for VX
    unsigned input_size_dw = (uni_input_size + 4)*32/TypeSize(Type_UD);

    G4_Declare *dcl = createSendPayloadDcl(input_size_dw, Type_UD);

    // mov  (96)    VX(0,0)<1>,  UNIInput
    createMovSendSrcInst(dcl, 0, 0, uni_input_size*32/TypeSize(Type_UD), unitInputOpnd, InstOpt_WriteEnable);

    // mov  (128)   VX(3,0)<1>,  FBRInput
    createMovSendSrcInst(dcl, (short) uni_input_size, 0, 128/TypeSize(Type_UD), fbrInputOpnd, InstOpt_WriteEnable);

    // mov  (1)     VX(2,20)<1>, FBRMbMode
    G4_DstRegRegion* tmp_dst1_opnd = createDst(
        dcl->getRegVar(),
        2,
        20,
        1,
        Type_UB);

    createMov(
        g4::SIMD1,
        tmp_dst1_opnd,
        fbrMbModOpnd,
        InstOpt_WriteEnable,
        true);

    // mov  (1)     VX(2,21)<1>, FBRSubMbShape
    G4_DstRegRegion* tmp_dst2_opnd = createDst(
        dcl->getRegVar(),
        2,
        21,
        1,
        Type_UB);

    createMov(
        g4::SIMD1,
        tmp_dst2_opnd,
        fbrSubMbShapeOpnd,
        InstOpt_WriteEnable,
        true);

    //  mov  (1)     VX(2,22)<1>, FBRSubPredMode
    G4_DstRegRegion* tmp_dst3_opnd = createDst(
        dcl->getRegVar(),
        2,
        22,
        1,
        Type_UB);

    createMov(
        g4::SIMD1,
        tmp_dst3_opnd,
        fbrSubPredModeOpnd,
        InstOpt_WriteEnable,
        true);

    // send's operands preparation
    // create a currDst for VX
    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

    G4_DstRegRegion* d = checkSendDst(outputOpnd->asDstRegRegion());

    unsigned temp = 0;            // Bit 7-0 of message descriptor
    temp += 0x3 << 13;            // Bit 14-13 of message descriptor

    unsigned regs2rcv = 7;

    createSendInst(
        NULL,
        d,
        payload,
        input_size_dw / getGenxDataportIOSize(),
        regs2rcv,
        G4_ExecSize(getGenxDataportIOSize()),
        temp,
        SFID::CRE,
        true,  //head_present?
        SendAccess::READ_ONLY,
        surfaceOpnd,
        NULL,
        InstOpt_WriteEnable,
        false);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISAVmeIdmInst(
    G4_Operand* surfaceOpnd,
    G4_Operand* unitInputOpnd,
    G4_Operand* idmInputOpnd,
    G4_DstRegRegion* outputOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    unsigned uni_input_size;

    uni_input_size = 4;

    // add dcl for VX
    unsigned input_size_dw = (uni_input_size + 1)*32/TypeSize(Type_UD);

    G4_Declare *dcl = createSendPayloadDcl(input_size_dw, Type_UD);

    // mov  (128)    VX(0,0)<1>,  UNIInput
    createMovSendSrcInst(dcl, 0, 0, uni_input_size*32/TypeSize(Type_UD), unitInputOpnd, InstOpt_WriteEnable);

    // mov  (32)   VX(3,0)<1>,  IDMInput
    createMovSendSrcInst(dcl, (short) uni_input_size, 0, 32/TypeSize(Type_UD), idmInputOpnd, InstOpt_WriteEnable);

    // send's operands preparation
    // create a currDst for VX
    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

    G4_DstRegRegion* d = checkSendDst(outputOpnd->asDstRegRegion());

    unsigned temp = 0;            // Bit 7-0 of message descriptor
                                  // temp += 0x0 << 13;            // Bit 14-13 of message descriptor

    unsigned regs2rcv = 16;

    // dst is already UW
    createSendInst(
        NULL,
        d,
        payload,
        input_size_dw / getGenxDataportIOSize(),
        regs2rcv,
        G4_ExecSize(getGenxDataportIOSize()),
        temp,
        SFID::VME,
        true,
        SendAccess::READ_ONLY,
        surfaceOpnd,
        NULL,
        InstOpt_WriteEnable,
        false);

    return VISA_SUCCESS;
}


int IR_Builder::translateVISASamplerVAGenericInst(
    G4_Operand*   surface,   G4_Operand*   sampler,
    G4_Operand*   uOffOpnd , G4_Operand*   vOffOpnd,
    G4_Operand*   vSizeOpnd, G4_Operand*   hSizeOpnd,
    G4_Operand*   mmfMode,   unsigned char cntrl,
    unsigned char msgSeq,    VA_fopcode    fopcode,
    G4_DstRegRegion*dstOpnd, G4_Type       dstType,
    unsigned      dstSize,
    bool isBigKernel)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_Declare* dcl  = createSendPayloadDcl(2 * getGenxSamplerIOSize(), Type_UD);
    G4_Declare *dcl1 = createSendPayloadDcl(8,                          Type_UD);
    G4_Declare *dclF = createSendPayloadDcl(8,                          Type_F);
    dcl1->setAliasDeclare (dcl, numEltPerGRF<Type_UB>());
    dclF->setAliasDeclare (dcl, numEltPerGRF<Type_UB>());

    /// Message Sequence Setup:
    /// When Functionality is MINMAX/BoolCentroid/Centroid, value is binary 1x.
    switch (fopcode)
    {
    case       MINMAX_FOPCODE:
    case     Centroid_FOPCODE:
    case BoolCentroid_FOPCODE:
        msgSeq = 0x2;
        break;
    default:
        break; // Prevent gcc warning
    }

    /// Message Header Setup
    /// 19:18 output control format | 15 Alpha Write Channel Mask ARGB = 1101 = 0xD for sampler8x8
    unsigned msg_header = (cntrl << 18) + (0xD << 12);

    /// Media Payload Setup
    /// M1.7: 31:28 (Functionality) | 27 (IEF) | 26:25 (MSG_SEQ) | 24:23 (MMF_MODE) | 22:0 (Group ID Number)
    G4_Operand* mediaPayld_var = createImm(0, Type_UD);
    G4_Operand* mediaPayld_imm = NULL;

    if (fopcode ==  Convolve_FOPCODE)
    {
        mediaPayld_imm = createImm(
            (((unsigned)fopcode) << 28) |
            (0 << 27) |
            (msgSeq << 25) |
            (isBigKernel << 23), Type_UD);

    }
    else if (fopcode == MINMAX_FOPCODE || fopcode == MINMAXFILTER_FOPCODE)
    {
        mediaPayld_imm = createImm(
            (((unsigned)fopcode) << 28) |
            (0 << 27) |
            (msgSeq << 25) |
            (((mmfMode && mmfMode->isImm()) ? mmfMode->asImm()->getInt()    : 0) << 23),
            Type_UD);

        /// Support non-constant MMF_ENABLE parameters.
        /// Reuse for non-constant exec/control modes.
        if (mmfMode && !mmfMode->isImm())
        {
            G4_DstRegRegion  media_payload_dst(*this, Direct, dcl1->getRegVar(), 0, 7, 1, Type_UD);
            mediaPayld_var = createSrc(dcl1->getRegVar(), 0, 7, getRegionScalar(), Type_UD);
            createBinOp(G4_shl, g4::SIMD1,
                createDstRegRegion(media_payload_dst), mmfMode, createImm(23, Type_UD), InstOpt_WriteEnable, true);
        }
    }
    else
    {
        mediaPayld_imm = createImm((((unsigned)fopcode) << 28)                   |
            (    0 << 27)                               |
            (msgSeq << 25)                               |
            (0x3 << 23), Type_UD);
    }

    /// Message Descriptor Setup
    unsigned msg_descriptor = (0x3 << 17) + (0xB  << 12);

    createMovR0Inst(dcl, 0, 0, true);
    createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(msg_header, Type_UD), true); /// mov msg_header
    if (hasBindlessSampler())
    {
        // clear M0.3 bit 0 (sampler state base address select)
        // and (1) M0.3<1>:ud M0.3<0;1,0>:ud 0xFFFFFFFE:ud
        G4_SrcRegRegion* src0 = createSrc(dcl->getRegVar(), 0, 3,
            getRegionScalar(), Type_UD);
        G4_Imm* src1 = createImm(0xFFFFFFFE, Type_UD);
        G4_DstRegRegion* dst = createDst(dcl->getRegVar(), 0, 3, 1, Type_UD);
        (void) createBinOp(G4_and, g4::SIMD1, dst, src0, src1, InstOpt_WriteEnable, true);
    }
    createMovInst(dcl1, 0, 0, g4::SIMD8, NULL, NULL, createImm(0, Type_UD), true); /// zero out
    createMovInst(dclF, 0, 2, g4::SIMD1, NULL, NULL, uOffOpnd, true); /// mov u opnd
    createMovInst(dclF, 0, 3, g4::SIMD1, NULL, NULL, vOffOpnd, true); /// mov v opnd
    createAddInst(dcl1, 0, 7, g4::SIMD1, NULL, NULL, mediaPayld_var, mediaPayld_imm, InstOpt_WriteEnable); /// store payload bits
    G4_SrcRegRegion* src = createSrc(dcl1->getRegVar(), 0, 7,
        getRegionScalar(), Type_UD);

    createAddInst(dcl1, 0, 7, g4::SIMD1, NULL, NULL, src,
        createSrcRegRegion(builtinHWTID, getRegionScalar()), InstOpt_WriteEnable);
    // later phases need FFTID
    preDefVars.setHasPredefined(PreDefinedVarsInternal::HW_TID, true);
    /// M1.0: [DevBDW+] Function = Centroid/BoolCentroid v/h direction size.
    if (vSizeOpnd)
    {
        G4_Operand* h_sz_shl_opnd = NULL;

        if (!hSizeOpnd || hSizeOpnd->isImm())
            h_sz_shl_opnd = createImm((hSizeOpnd ? (hSizeOpnd->asImm()->getInt() << 4) : 0), Type_UD);
        else
        {
            h_sz_shl_opnd = createSrc(dcl1->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
            G4_DstRegRegion* temp_dst = createDst(dcl1->getRegVar(), 0, 0, 1, Type_UD);
            createBinOp(G4_shl, g4::SIMD1, temp_dst, hSizeOpnd,
                createImm(4, Type_UD), InstOpt_WriteEnable, true);
        }
        createAddInst(dcl1, 0, 0, g4::SIMD1, NULL, NULL, vSizeOpnd, h_sz_shl_opnd, InstOpt_WriteEnable);
    }

    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());
    G4_DstRegRegion* post_dst = checkSendDst(dstOpnd->asDstRegRegion());
    int reg_receive = dstSize/numEltPerGRF<Type_UB>();
    if (reg_receive < 1)
        reg_receive = 1;
    createSendInst(NULL, post_dst, payload, 2, reg_receive, g4::SIMD8,
        msg_descriptor, SFID::SAMPLER, 1, SendAccess::READ_ONLY, surface, sampler, InstOpt_WriteEnable, false);

    return VISA_SUCCESS;
}

/*
* Translates Sampler API intrinsic.
*output matrix, ChannelMask, SurfaceIndex, SamplerIndex, u, v, deltaU, deltaV
*u2d, OutputFormatControl=0, v2d=0.0, AVSExecMode=0, EIFbypass=false
* sample8x8AVS(matrix<unsigned short, N, 64> &M, samplerType,  channelMask, surfIndex, samplerIndex, u, v, deltaU, deltaV, u2d,
OutputFormatControl=0, v2d, AVSExecMode, EIFbypass=false);
*
* Assuming: N = 4, channelMask=ABGR_ENABLE, surfIndex = 0x21, samplerIndex = 0x4,
*           then the generated code should look like the following for GT:
*
* .declare  VX Base=m ElementSize=4 Type=ud Total=16
* .declare  VA Base=m ElementSize=4 Type=f Total=8  ALIAS(VX,8)
* .declare  VY Base=r ElementSize=2 Type=uw Total=256
*
* mov  (8)     VX(0,0)<1>,  r0:ud
* mov  (1)     VX(0,2)<1>,  0 channel mask [12,15], output format control [16,17] 0
* mov  (1)     VA(0,0)<1>,  v2d
* mov  (1)     VA(0,1)<1>,  vertical block number
* mov  (1)     VA(0,2)<1>,  u
* mov  (1)     VA(0,3)<1>,  v
* mov  (1)     VA(0,4)<1>,  deltaU
* mov  (1)     VA(0,5)<1>,  deltaV
* mov  (1)     VA(0,6)<1>,  u2d
* mov  (1)     VA(0,7)<1>,
[0:22]  GroupID
[23:24] Reserved
[25:26] 1x - 16x8
0x - 16x4
[27]    EIF Bypass
[28:31] 0000 - AVS Scaling
* send (16)    VY(0,0)<1>,  VX(0,0),    0x2,   0x048bc421
* mov  (256)   M(0,0)<1>,   VY(0,0)
*
* VX: message header
*
* VA: SIMD32 media payload
*
* ex_desc: 0x2 == 0010 (Target Function ID: Sampling Engine)
*
* desc: 0x050EB000 == Bit 31-29: 000 (Reserved)
*                     Bit 28-25: 0010 (Message Length = 2)
*                     Bit 24-20: 10000 (Response Message Length = 16)
*                     Bit 19:    1 (Header present)
*                     Bit 18-17: 11 (SIMD Mode = SIMD32/64)
*                     Bit 16-12: 01011 (Message Type = sample8x8 Media layout)
*                     Bit 11-8:  0000 + samplerIndex  (Sampler Index)
*                     Bit 7-0:   00000000 + surfIndex (Binding Table Index)
*
*/
int IR_Builder::translateVISAAvsInst(
    G4_Operand* surface,
    G4_Operand* sampler,
    ChannelMask channel,
    unsigned numEnabledChannels,
    G4_Operand* deltaUOpnd,
    G4_Operand* uOffOpnd,
    G4_Operand* deltaVOpnd,
    G4_Operand* vOffOpnd,
    G4_Operand* u2dOpnd,
    G4_Operand* groupIDOpnd,
    G4_Operand* verticalBlockNumberOpnd,
    unsigned char cntrl,
    G4_Operand* v2dOpnd,
    unsigned char execMode,
    G4_Operand* eifbypass,
    G4_DstRegRegion* dstOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);


    {
        /*
        * mov  (8)     VX(0,0)<1>,  r0:ud
        * mov  (1)     VX(0,2)<1>,  0 channel mask [12,15], output format control [16,17] 0
        * mov  (1)     VA(0,0)<1>,  v2d
        * mov  (1)     VA(0,1)<1>,  vertical block number
        * mov  (1)     VA(0,2)<1>,  u
        * mov  (1)     VA(0,3)<1>,  v
        * mov  (1)     VA(0,4)<1>,  deltaU
        * mov  (1)     VA(0,5)<1>,  deltaV
        * mov  (1)     VA(0,6)<1>,  u2d
        * mov  (1)     VA(0,7)<1>,
        [0:22]  GroupID
        [23:24] Reserved
        [25:26] 1x - 16x8
        0x - 16x4
        [27]    EIF Bypass
        [28:31] 0000 - AVS Scaling
        */
        unsigned int number_elements_returned = 64;
        G4_Type output_type = Type_UW;

        if (cntrl > 1)
            output_type = Type_UB;


        if (execMode == AVS_16x8)
        {
            number_elements_returned = 128;
            numEnabledChannels *= 2;
        }

        if (execMode == AVS_8x4)
        {
            number_elements_returned = 32;
        }

        if (execMode == AVS_4x4)
        {
            number_elements_returned = 16;
        }

        unsigned obj_size = number_elements_returned*numEnabledChannels*TypeSize(output_type);
        // mov (8)      VX(0,0)<1>,  r0:ud
        // add dcl for VX
        G4_Declare *dcl = createSendPayloadDcl(2 * getGenxSamplerIOSize(), Type_UD);

        // mov  VX(0,0)<1>, r0
        createMovR0Inst(dcl, 0, 0, true);
        /* mov (1)     VX(0,2)<1>,   0  */
        unsigned cmask = channel.getHWEncoding() << 12;
        cmask += cntrl << 18;
        createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(cmask, Type_UD), true);

        G4_Declare *dcl1 = createSendPayloadDcl(getGenxDataportIOSize(), Type_F);
        dcl1->setAliasDeclare(dcl, numEltPerGRF<Type_UB>());

        /*
        Keeping destination type as UD, otherwise w-->f conversion happens,
        which affects the results.
        */
        G4_Declare *dcl1_ud = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
        dcl1_ud->setAliasDeclare(dcl, numEltPerGRF<Type_UB>());

        // mov  (1)     VA(0,0)<1>,  v2d
        createMovInst(dcl1, 0, 0, g4::SIMD1, NULL, NULL, v2dOpnd, true);

        // mov  (1)     VA(0,1)<1>,  vertical block number
        createMovInst(dcl1_ud, 0, 1, g4::SIMD1, NULL, NULL, verticalBlockNumberOpnd, true);
        // mov  (1)     VA(1,2)<1>,  u
        createMovInst(dcl1, 0, 2, g4::SIMD1, NULL, NULL, uOffOpnd, true);
        // mov  (1)     VA(1,3)<1>,  v
        createMovInst(dcl1, 0, 3, g4::SIMD1, NULL, NULL, vOffOpnd, true);
        // mov  (1)     VA(1,4)<1>,  deltaU
        createMovInst(dcl1, 0, 4, g4::SIMD1, NULL, NULL, deltaUOpnd, true);
        // mov  (1)     VA(1,5)<1>,  deltaV
        createMovInst(dcl1, 0, 5, g4::SIMD1, NULL, NULL, deltaVOpnd, true);
        // mov  (1)     VA(0,6)<1>,  U2d
        createMovInst(dcl1, 0, 6, g4::SIMD1, NULL, NULL, u2dOpnd, true);

        {
            /*
            [23:24] Reserved
            [25:26] 1x - 16x8
            0x - 16x4
            [27]    EIF Bypass
            [28:31] 0000 - AVS Scaling
            */
            unsigned int upper_bits = 0;
            upper_bits += execMode << 25;

            if (eifbypass->isImm())
            {
                upper_bits += (eifbypass->asImm()->getInt() & 1) << 27;

                G4_DstRegRegion* dst2_opnd = createDst(dcl1_ud->getRegVar(), 0, 7, 1, Type_UD);
                createBinOp(G4_add, g4::SIMD1, dst2_opnd, groupIDOpnd,
                    createImm(upper_bits, Type_UD), InstOpt_WriteEnable, true);
            }
            else
            {
                // extract lsb of eifbypass
                G4_DstRegRegion* dst2_opnd = createDst(dcl1_ud->getRegVar(), 0, 7, 1, Type_UD);
                createBinOp(G4_and, g4::SIMD1, dst2_opnd, eifbypass,
                    createImm(1, Type_UD), InstOpt_WriteEnable, true);

                // eifbypass << 27
                G4_SrcRegRegion* src2_opnd = createSrc(dcl1_ud->getRegVar(), 0, 7, getRegionScalar(), dcl1_ud->getElemType());
                G4_DstRegRegion* dst3_opnd = createDst(dcl1_ud->getRegVar(), 0, 7, 1, Type_UD);
                createBinOp(G4_shl, g4::SIMD1, dst3_opnd, src2_opnd,
                    createImm(27, Type_UD), InstOpt_WriteEnable, true);

                // upper_bits + (eifbypass << 27)
                G4_SrcRegRegion* src3_opnd = createSrc(dcl1_ud->getRegVar(), 0, 7, getRegionScalar(), dcl1_ud->getElemType());
                G4_DstRegRegion* dst4_opnd = createDst(dcl1_ud->getRegVar(), 0, 7, 1, Type_UD);
                createBinOp(G4_add, g4::SIMD1, dst4_opnd, src3_opnd,
                    createImm(upper_bits, Type_UD), InstOpt_WriteEnable, true);

                G4_DstRegRegion* dst5_opnd = createDst(dcl1_ud->getRegVar(), 0, 7, 1, Type_UD);
                G4_SrcRegRegion* src_opnd = createSrc(dcl1_ud->getRegVar(), 0, 7, getRegionScalar(), dcl1_ud->getElemType());
                createBinOp(G4_add, g4::SIMD1, dst5_opnd, groupIDOpnd, src_opnd, InstOpt_WriteEnable, true);

            }
        }

        /*
        * desc: 0x050EB000 == Bit 31-29: 000 (Reserved)
        *                     Bit 28-25: 0010 (Message Length = 2)
        *                     Bit 24-20: 10000 (Response Message Length = 16)
        *                     Bit 19:    1 (Header present)
        *                     Bit 18-17: 11 (SIMD Mode = SIMD32/64)
        *                     Bit 16-12: 01011 (Message Type = sample8x8 Media layout)
        *                     Bit 11-8:  0000 + samplerIndex  (Sampler Index)
        *                     Bit 7-0:   00000000 + surfIndex (Binding Table Index)
        */

        // Set bit 9-8 for the message descriptor
        unsigned temp = 0;
        temp += 0xB << 12;  // Bit 15-12 = 1100 for Sampler Message Type
        temp += 0x3 << 17;  // Bit 17-16 = 11 for SIMD32 mode

                            // send's operands preparation
                            // create a currDst for VX
        G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());

        G4_DstRegRegion* d = checkSendDst(dstOpnd->asDstRegRegion());

        createSendInst(
            NULL,
            d,
            payload,
            2,
            obj_size/numEltPerGRF<Type_UB>(),
            g4::SIMD16,
            temp,
            SFID::SAMPLER,
            1,
            SendAccess::READ_ONLY,
            surface,
            sampler,
            InstOpt_WriteEnable,
            false);
    }

    return VISA_SUCCESS;
}


int IR_Builder::translateVISAVaSklPlusGeneralInst(
    ISA_VA_Sub_Opcode sub_opcode,
    G4_Operand* surface, G4_Operand* sampler,
    unsigned char mode, unsigned char functionality,
    G4_Operand* uOffOpnd, G4_Operand* vOffOpnd ,

    //1pixel convolve
    G4_Operand * offsetsOpnd,

    //FloodFill
    G4_Operand* loopCountOpnd,             G4_Operand* pixelHMaskOpnd,
    G4_Operand* pixelVMaskLeftOpnd,        G4_Operand* pixelVMaskRightOpnd,

    //LBP Correlation
    G4_Operand* disparityOpnd,

    //Correlation Search
    G4_Operand* verticalOriginOpnd,        G4_Operand* horizontalOriginOpnd,
    G4_Operand* xDirectionSizeOpnd,        G4_Operand* yDirectionSizeOpnd,
    G4_Operand* xDirectionSearchSizeOpnd , G4_Operand* yDirectionSearchSizeOpnd,

    G4_DstRegRegion* dstOpnd, G4_Type dstType, unsigned dstSize,

    //HDC
    unsigned char pixelSize, G4_Operand* dstSurfaceOpnd,
    G4_Operand *dstXOpnd,    G4_Operand* dstYOpnd,
    bool hdcMode)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_Declare* dcl  = NULL;
    G4_Declare *dcl_offsets = NULL;

    unsigned int reg_to_send = 2;
    //for offsets
    if ((sub_opcode == VA_OP_CODE_1PIXEL_CONVOLVE && mode == VA_CONV_16x1) ||
        sub_opcode == ISA_HDC_1PIXELCONV)
    {
        dcl = createSendPayloadDcl(4 * getGenxSamplerIOSize() , Type_UD);
        //16 pairs of x,y coordinates
        dcl_offsets = createSendPayloadDcl(32                      , Type_W);
        dcl_offsets->setAliasDeclare(dcl, numEltPerGRF<Type_UB>() * 2);
        reg_to_send = 4;
    }
    else
        dcl = createSendPayloadDcl(2 * getGenxSamplerIOSize() , Type_UD);

    G4_Declare *dcl_payload_UD = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
    G4_Declare *dcl_payload_F = createSendPayloadDcl(getGenxDataportIOSize(), Type_F);
    G4_Declare *dcl_payload_UW = createSendPayloadDcl(getGenxDataportIOSize() * 2, Type_UW);

    dcl_payload_UD->setAliasDeclare (dcl,  numEltPerGRF<Type_UB>());
    dcl_payload_F->setAliasDeclare (dcl, numEltPerGRF<Type_UB>());
    dcl_payload_UW->setAliasDeclare (dcl, numEltPerGRF<Type_UB>());

    /// Message Header Setup
    /// 19:18 output control format | 15 Alpha Write Channel Mask ARGB = 1101 = 0xD for sampler8x8
    unsigned msg_header = (0xD << 12);

    //if MMF based on pixel size set output format control.
    if (sub_opcode == ISA_HDC_MMF && pixelSize)
    {
        msg_header = msg_header + (0x2 << 18);
    }

    //I guess this is still needed just to be sure payload is really initiazlied.
    //since full register initalization is conservative some registers
    //can still be not initialized and then used for payload
    if (m_options->getOption(vISA_InitPayload))
    {
        createMovInst(dcl_payload_UD, 0, 0, g4::SIMD8, NULL, NULL, createImm(0, Type_UD));
    }
    // mov  VX(0,0)<1>, r0
    createMovR0Inst(dcl, 0, 0);
    createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(msg_header, Type_UD));

    //set dst BTI, In M0.2 bits 24:31
    if (hdcMode)
    {
        G4_Declare *dcl_temp = createDeclareNoLookup(
            "tmp_shl_dst_bti",
            G4_GRF ,
            1,
            1,
            Type_UD);

        //Creating dst of the shift to be used in shift instruction
        //Creating src of src to use in the subsequent add instruction
        G4_Operand* shift_immed = createSrc(dcl_temp->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
        G4_DstRegRegion* temp_dst = createDst(dcl_temp->getRegVar(), 0, 0,1, Type_UD);

        //creating a src and for m0.2
        G4_SrcRegRegion* m0_2_src = createSrc(dcl->getRegVar(), 0, 2, getRegionScalar(), Type_UD);
        G4_DstRegRegion* m0_2_dst = createDst(dcl->getRegVar(), 0, 2, 1, Type_UD);

        createBinOp(G4_shl, g4::SIMD1, temp_dst, dstSurfaceOpnd, createImm(24, Type_UD), InstOpt_WriteEnable, true);
        createBinOp(G4_add, g4::SIMD1, m0_2_dst, m0_2_src, shift_immed, InstOpt_WriteEnable, true);
    }

    // set x_offset In M0.4 0:15
    // set y_offset In M0.4 16:31
    if (hdcMode)
    {
        G4_Declare *dcl_temp = createDeclareNoLookup(
            "tmp_shl_y_offset",
            G4_GRF ,
            1,
            1,
            Type_UD);

        // Creating dst of the shift to be used in shift instruction
        // Creating src of src to use in the subsequent add instruction
        G4_Operand * shift_immed = createSrc(dcl_temp->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
        G4_DstRegRegion* temp_dst = createDst(dcl_temp->getRegVar(), 0, 0,1, Type_UD);

        // creating a src and for m0.4
        G4_DstRegRegion* m0_4_dst = createDst(dcl->getRegVar(), 0, 4, 1, Type_UD);

        createBinOp(G4_shl, g4::SIMD1, temp_dst, dstYOpnd, createImm(16, Type_UD), InstOpt_WriteEnable, true);
        createBinOp(G4_add, g4::SIMD1, m0_4_dst, dstXOpnd, shift_immed, InstOpt_WriteEnable, true);
    }

    // set dst surface format based on pixel size M0.5 0:4
    if (hdcMode)
    {
        int surface_format = 0;
        if (pixelSize == 0) {
            surface_format = 6; // PLANAR_Y16_SNORM
        } else if (pixelSize == 1) {
            surface_format = 5; // PLANAR_Y8_UNORM
        } else {
            ASSERT_USER(false,
                "Invalid surface format for SKL+ VA HDC");
        }
        createMovInst(dcl, 0, 5, g4::SIMD1, NULL, NULL, createImm(surface_format, Type_UD));
    }

    // setting M2.1 vertical  block offset to 0
    // for LBP correlation setting M2.0 to 0, since only upper 16 bits are set
    // later by adding to shl result
    createMovInst(dcl_payload_UD, 0, 1, g4::SIMD1, NULL, NULL, createImm(0, Type_UD));

    // setting up M1.7
    unsigned int m1_7 = sampler8x8_group_id++;

    ISA_VA_Sub_Opcode originalSubOpcode = sub_opcode;

    // HDC uses the same sub opcodes as regular VA,
    // but with return register set to 0.
    switch (sub_opcode)
    {
    case ISA_HDC_CONV:
        sub_opcode = Convolve_FOPCODE;
        break;
    case ISA_HDC_MMF:
        sub_opcode = MINMAXFILTER_FOPCODE;
        break;
    case ISA_HDC_ERODE:
        sub_opcode = ERODE_FOPCODE;
        break;
    case ISA_HDC_DILATE:
        sub_opcode = Dilate_FOPCODE;
        break;
    case ISA_HDC_LBPCORRELATION:
        sub_opcode = VA_OP_CODE_LBP_CORRELATION;
        break;
    case ISA_HDC_LBPCREATION:
        sub_opcode = VA_OP_CODE_LBP_CREATION;
        break;
    case ISA_HDC_1DCONV_H:
        sub_opcode = VA_OP_CODE_1D_CONVOLVE_HORIZONTAL;
        break;
    case ISA_HDC_1DCONV_V:
        sub_opcode = VA_OP_CODE_1D_CONVOLVE_VERTICAL;
        break;
    case ISA_HDC_1PIXELCONV:
        sub_opcode = VA_OP_CODE_1PIXEL_CONVOLVE;
        break;
    default:
        break; // Prevent gcc warning
    }
    //setting VA operation
    m1_7 |= (unsigned int)sub_opcode<<28;

    //setting IEF bypass to 1
    m1_7 |= 0x1<<27;

    //setting message sequence
    m1_7 |= (mode & 0x3) << 25;

    //setting functionality
    m1_7 |= (functionality & 0x3) << 23;
    createMovInst(dcl_payload_UD, 0, 7, g4::SIMD1, NULL, NULL, createImm(m1_7, Type_UD));

    /*
    case VA_OP_CODE_1D_CONVOLVE_HORIZONTAL:
    case VA_OP_CODE_1D_CONVOLVE_VERTICAL:
    case VA_OP_CODE_1PIXEL_CONVOLVE:
    case VA_OP_CODE_FLOOD_FILL:
    case VA_OP_CODE_LBP_CREATION:
    case VA_OP_CODE_LBP_CORRELATION:
    case VA_OP_CODE_CORRELATION_SEARCH:
    */

    //setting m1_5 and m1_4
    if (sub_opcode == VA_OP_CODE_CORRELATION_SEARCH)
    {
        createMovInst(dcl_payload_F, 0, 5, g4::SIMD1, NULL, NULL, verticalOriginOpnd);
        createMovInst(dcl_payload_F, 0, 4, g4::SIMD1, NULL, NULL, horizontalOriginOpnd);
    }

    //setting m1_3
    if (vOffOpnd != NULL)
    {
        createMovInst(dcl_payload_F, 0, 3, g4::SIMD1, NULL, NULL, vOffOpnd);
    }

    //setting m1_2
    if (uOffOpnd != NULL)
    {
        createMovInst(dcl_payload_F, 0, 2, g4::SIMD1, NULL, NULL, uOffOpnd);
    }

    if (sub_opcode == VA_OP_CODE_FLOOD_FILL)
    {
        createMovSendSrcInst(dcl_payload_UD, 0, 2, 5, pixelHMaskOpnd, 0);
    }

    if ((sub_opcode == VA_OP_CODE_1PIXEL_CONVOLVE  && mode == VA_CONV_16x1) ||
        originalSubOpcode == ISA_HDC_1PIXELCONV)
    {
        const RegionDesc *rd = getRegionStride1();
        G4_Operand *offsets_opnd_temp = createSrc(
            offsetsOpnd->asSrcRegRegion()->getBase(),
            0,
            0,
            rd,
            Type_W);

        createMovInst(dcl_offsets, 0, 0, g4::SIMD32, NULL, NULL, offsets_opnd_temp);
    }

    //creating temp for intermediate computations
    G4_Declare *dcl_temp = createDeclareNoLookup(
        "tmp_shl",
        G4_GRF ,
        1,
        1,
        Type_UD);
    G4_SrcRegRegion temp_src(*this, Mod_src_undef,Direct,dcl_temp->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
    G4_DstRegRegion temp_dst(*this, Direct, dcl_temp->getRegVar(), 0, 0,1, Type_UD);

    //creating a src and for m1.0
    G4_SrcRegRegion m1_0_src(*this, Mod_src_undef,Direct,dcl_payload_UD->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
    G4_DstRegRegion m1_0_dst(*this, Direct, dcl_payload_UD->getRegVar(), 0, 0, 1, Type_UD);

    G4_Operand * shift_immed = NULL;

    //setting m1_0
    switch (sub_opcode)
    {
    case VA_OP_CODE_FLOOD_FILL:
    {
        createMovInst(dcl_payload_UD, 0, 0, g4::SIMD1, NULL, NULL, pixelVMaskLeftOpnd);

        if (pixelVMaskRightOpnd->isImm())
        {
            shift_immed = createImm(pixelVMaskRightOpnd->asImm()->getInt() << 10,Type_UD);
            createMov(g4::SIMD1, createDstRegRegion(m1_0_dst), shift_immed, InstOpt_NoOpt, true);
        } else {

            createBinOp(G4_shl, g4::SIMD1,
                createDstRegRegion(temp_dst), pixelVMaskRightOpnd, createImm(10, Type_UD), InstOpt_WriteEnable, true);
            shift_immed = createSrcRegRegion(temp_src);
            createBinOp(G4_add, g4::SIMD1,
                createDstRegRegion(m1_0_dst), createSrcRegRegion(m1_0_src), shift_immed, InstOpt_WriteEnable, true);
        }

        if (loopCountOpnd->isImm())
        {
            shift_immed = createImm(loopCountOpnd->asImm()->getInt() << 24, Type_UD);
        } else {
            createBinOp(G4_shl, g4::SIMD1,
                createDstRegRegion(temp_dst), loopCountOpnd, createImm(24, Type_UD), InstOpt_WriteEnable, true);
            shift_immed = createSrcRegRegion(temp_src);
        }
        createBinOp(G4_add, g4::SIMD1,
            createDstRegRegion(m1_0_dst), createSrcRegRegion(m1_0_src), shift_immed, InstOpt_WriteEnable, true);
        break;
    }
    case VA_OP_CODE_LBP_CORRELATION:
    {
        //setting disparity
        if (disparityOpnd->isImm())
        {
            shift_immed = createImm(disparityOpnd->asImm()->getInt() << 16, Type_UD);
            createMov(g4::SIMD1, createDstRegRegion(m1_0_dst), shift_immed, InstOpt_NoOpt, true);
        }
        else
        {
            createBinOp(G4_shl, g4::SIMD1,
                createDstRegRegion(m1_0_dst), disparityOpnd, createImm(16, Type_UD), InstOpt_WriteEnable, true);
        }

        break;
    }
    case VA_OP_CODE_CORRELATION_SEARCH:
    {
        /*
        G4_Operand* verticalOriginOpnd    , G4_Operand* horizontalOriginOpnd  ,
        G4_Operand* xDirectionSizeOpnd   , G4_Operand* yDirectionSizeOpnd   ,
        G4_Operand* xDirectionSearchSizeOpnd , G4_Operand* yDirectionSearchSizeOpnd ,
        */
        createMovInst(dcl_payload_UD, 0, 0, g4::SIMD1, NULL, NULL, xDirectionSizeOpnd);

        //setting y-direction size of the source for correlation.
        if (yDirectionSizeOpnd->isImm())
        {
            shift_immed = createImm(yDirectionSizeOpnd->asImm()->getInt() << 4, Type_UD);
            createMov(g4::SIMD1, createDstRegRegion(m1_0_dst), shift_immed, InstOpt_NoOpt, true);
        }
        else
        {
            createBinOp(G4_shl, g4::SIMD1, createDstRegRegion(temp_dst), yDirectionSizeOpnd, createImm(4, Type_UD), InstOpt_WriteEnable, true);
            shift_immed = createSrcRegRegion(temp_src);
            createBinOp(G4_add, g4::SIMD1, createDstRegRegion(m1_0_dst), createSrcRegRegion(m1_0_src), shift_immed, InstOpt_WriteEnable, true);
        }


        //31:16 reserved

        //setting x-direction search size
        if (xDirectionSearchSizeOpnd->isImm())
        {
            shift_immed = createImm(xDirectionSearchSizeOpnd->asImm()->getInt() << 8, Type_UD);
        } else {
            createBinOp(G4_shl, g4::SIMD1, createDstRegRegion(temp_dst), xDirectionSearchSizeOpnd, createImm(8, Type_UD), InstOpt_WriteEnable, true);
            shift_immed = createSrcRegRegion(temp_src);
        }
        createBinOp(G4_add, g4::SIMD1, createDstRegRegion(m1_0_dst), createSrcRegRegion(m1_0_src), shift_immed, InstOpt_WriteEnable, true);

        //setting y-direction search size.
        if (yDirectionSearchSizeOpnd->isImm())
        {
            shift_immed = createImm(yDirectionSearchSizeOpnd->asImm()->getInt() << 16, Type_UD);
        } else {
            createBinOp(G4_shl, g4::SIMD1, createDstRegRegion(temp_dst), yDirectionSearchSizeOpnd, createImm(16, Type_UD), InstOpt_WriteEnable, true);
            shift_immed = createSrcRegRegion(temp_src);
        }
        createBinOp(G4_add, g4::SIMD1, createDstRegRegion(m1_0_dst), createSrcRegRegion(m1_0_src), shift_immed, InstOpt_WriteEnable, true);

        break;
    }
    default:
        break; // Prevent gcc warning
    }

    G4_SrcRegRegion* payload = createSrcRegRegion(dcl, getRegionStride1());
    G4_DstRegRegion* post_dst = NULL;

    unsigned int reg_to_receive = 0;

    if (!hdcMode)
    {
        post_dst = checkSendDst(dstOpnd);
        if ((dstSize %  numEltPerGRF<Type_UB>()) != 0)
        {
            reg_to_receive = (unsigned int) std::ceil((double)dstSize/numEltPerGRF<Type_UB>());
        }
        else
        {
            reg_to_receive = dstSize/numEltPerGRF<Type_UB>();
        }
    } else {
        post_dst = createNullDst(Type_UD);
    }

    /// Message Descriptor Setup
    /// 18:17 SIMD Mode (SIMD32/64 = 3)  |  16:12 Message Type (sampler8x8 = 01011 = 0xB)
    unsigned msg_descriptor = (0x3 << 17) + (0xB  << 12);
    createSendInst(NULL, post_dst, payload, reg_to_send, reg_to_receive, g4::SIMD8,
        msg_descriptor, SFID::SAMPLER, 1, SendAccess::READ_ONLY, surface, sampler, 0, false);

    return VISA_SUCCESS;
}

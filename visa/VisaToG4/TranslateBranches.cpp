/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "../Timer.h"

using namespace vISA;

int IR_Builder::translateVISACFSwitchInst(
    G4_Operand *indexOpnd, uint8_t numLabels, G4_Label ** labels)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    // TGL+ uses pre-increment IP for jmp offset, i.e., offset 0 is infinite loop
    bool preIncIP = getPlatform() >= GENX_TGLLP;
    // offsets are in bytes so we have to multiply everything by 16
    // This requires that the jmpi instructions are never compacted.
    if (indexOpnd->isImm())
    {
        auto val = indexOpnd->asImm()->getInt();
        if (preIncIP)
        {
            val += 1;
        }
        indexOpnd = createImm(val * 16, Type_W);
    }
    else
    {
        G4_Declare *tmpVar = createTempVar(1, Type_D, Any);
        G4_DstRegRegion* dstOpnd = createDstRegRegion(tmpVar, 1);
        (void)createBinOp(G4_shl, g4::SIMD1, dstOpnd, indexOpnd,
            createImm(4, Type_UW), InstOpt_WriteEnable, true);
        if (preIncIP)
        {
            auto src0 = createSrcRegRegion(tmpVar, getRegionScalar());
            auto src1 = createImm(16, Type_UW);
            (void) createBinOp(G4_add, g4::SIMD1, createDstRegRegion(tmpVar, 1), src0, src1, InstOpt_WriteEnable, true);
        }

        indexOpnd = createSrcRegRegion(tmpVar, getRegionScalar());
    }
    G4_INST* indirectJmp = createJmp(nullptr, indexOpnd, InstOpt_NoOpt, true);

    for (int i = 0; i < numLabels; i++)
    {
        indirectJmp->asCFInst()->addIndirectJmpLabel(labels[i]);
    }

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFLabelInst(G4_Label* lab)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    createLabelInst(lab, true);

    if (lab->isFuncLabel())
    {
        subroutineId++;
    }

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFCallInst(
    VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Label* lab)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_opcode callOpToUse = GetGenOpcodeFromVISAOpcode(ISA_CALL);
    G4_DstRegRegion* dstOpndToUse = NULL;
    G4_ExecSize execSize = toExecSize(execsize);
    G4_Label* srcLabel = lab;

    if (lab->isFCLabel() == true)
    {
        callOpToUse = G4_pseudo_fc_call;
        getFCPatchInfo()->setHasFCCalls(true);

        input_info_t *RetIP = getRetIPArg();
        G4_Declare *FCRet = createTempVar(2, Type_UD, Four_Word);
        FCRet->setAliasDeclare(RetIP->dcl, 0);
        dstOpndToUse = createDstRegRegion(FCRet, 1);

        execSize = g4::SIMD2;
    }

    G4_InstOpts instOpts = Get_Gen4_Emask(emask, execSize);
    createInst(
        predOpnd,
        callOpToUse,
        nullptr,
        g4::NOSAT,
        execSize,
        dstOpndToUse,
        srcLabel,
        nullptr,
        instOpts,
        true);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFJumpInst(G4_Predicate *predOpnd, G4_Label* lab)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    createInst(
        predOpnd,
        GetGenOpcodeFromVISAOpcode(ISA_JMP),
        NULL,
        g4::NOSAT,
        g4::SIMD1,
        NULL,
        lab,
        NULL,
        InstOpt_NoOpt,
        true);

    return VISA_SUCCESS;
}


int IR_Builder::translateVISACFFCallInst(
    VISA_Exec_Size execsize, VISA_EMask_Ctrl emask, G4_Predicate *predOpnd,
    std::string funcName, uint8_t argSize, uint8_t returnSize)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    kernel.fg.setHasStackCalls();

    if (getArgSize() < argSize)
    {
        setArgSize(argSize);
    }

    if (getRetVarSize() < returnSize)
    {
        setRetVarSize(returnSize);
    }

    uint8_t exsize = (uint8_t)Get_VISA_Exec_Size(execsize);

    auto callLabel = getFcallLabel(funcName);

    auto fcall = createInst(
        predOpnd,
        G4_pseudo_fcall,
        nullptr,
        g4::NOSAT,
        exsize,
        nullptr,
        callLabel,
        nullptr,
        InstOpt_NoOpt,
        true);

    addFcallInfo(fcall, argSize, returnSize);
    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFIFCallInst(
    VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Operand* funcAddr, uint8_t argSize, uint8_t returnSize)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    kernel.fg.setHasStackCalls();
    kernel.setHasIndirectCall();

    if (getArgSize() < argSize)
    {
        setArgSize(argSize);
    }

    if (getRetVarSize() < returnSize)
    {
        setRetVarSize(returnSize);
    }

    uint8_t exsize = (uint8_t)Get_VISA_Exec_Size(execsize);

    auto src0 = funcAddr;
    if (!src0->isSrcRegRegion() ||
        (src0->isSrcRegRegion() && (src0->asSrcRegRegion()->getModifier() != Mod_src_undef)))
    {
        auto tmpSrc0 = createTempVar(1, Type_D, Any);
        createMov(g4::SIMD1, createDstRegRegion(tmpSrc0, 1), src0, InstOpt_WriteEnable, true);
        src0 = createSrcRegRegion(tmpSrc0, getRegionScalar());
    }

    auto fcall = createInst(predOpnd, G4_pseudo_fcall, nullptr, g4::NOSAT, exsize,
        nullptr, src0, nullptr, InstOpt_NoOpt, true);

    addFcallInfo(fcall, argSize, returnSize);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFSymbolInst(
    const std::string& symbolName, G4_DstRegRegion* dst)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    if (symbolName.compare("INTEL_PATCH_PRIVATE_MEMORY_SIZE") == 0)
    {
        // Relocation for runtime-calculated private memory size
        auto* privateMemPatch = createRelocImm(Type_UD);
        dst->setType(*this, Type_UD);
        G4_INST* mov = createMov(g4::SIMD1, dst, privateMemPatch, InstOpt_WriteEnable, true);
        RelocationEntry::createRelocation(kernel, *mov, 0, symbolName, GenRelocType::R_SYM_ADDR_32);
    }
    else if (Type_D == dst->getType())
    {
        auto patchImm = createRelocImm(dst->getType());
        auto movInst = createMov(g4::SIMD1, dst, patchImm, InstOpt_WriteEnable, true);

        RelocationEntry::createRelocation(kernel, *movInst, 0, symbolName, GenRelocType::R_GLOBAL_IMM_32);
    }
    else if (noInt64() || needSwap64ImmLoHi() || VISA_WA_CHECK(getPWaTable(), WaDisallow64BitImmMov))
    {
        auto* funcAddrLow = createRelocImm(Type_UD);
        auto* funcAddrHigh = createRelocImm(Type_UD);

        assert(!dst->isIndirect());
        // change type from uq to ud, adjust the subRegOff
        auto dstLo = createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2, 1, Type_UD);
        G4_INST* movLo = createMov(g4::SIMD1, dstLo, funcAddrLow, InstOpt_WriteEnable, true);
        // subRegOff will be right following dst's sub-reg
        auto dstHi = createDst(dst->getBase(), dst->getRegOff(), dst->getSubRegOff() * 2 + 1, 1, Type_UD);
        G4_INST* movHi = createMov(g4::SIMD1, dstHi, funcAddrHigh, InstOpt_WriteEnable, true);

        RelocationEntry::createRelocation(kernel, *movLo, 0, symbolName, GenRelocType::R_SYM_ADDR_32);
        RelocationEntry::createRelocation(kernel, *movHi, 0, symbolName, GenRelocType::R_SYM_ADDR_32_HI);
    }
    else
    {
        // symbolic imm representing symbol's address
        auto funcAddr = createRelocImm(Type_UQ);
        auto movInst = createMov(g4::SIMD1, dst, funcAddr, InstOpt_WriteEnable, true);

        RelocationEntry::createRelocation(kernel, *movInst, 0, symbolName, GenRelocType::R_SYM_ADDR);
    }

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFFretInst(
    VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask, G4_Predicate *predOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_ExecSize exsize = toExecSize(executionSize);
    G4_InstOpts instOpts = Get_Gen4_Emask(emask, exsize);

    kernel.fg.setIsStackCallFunc();

    createInst(
        predOpnd,
        G4_pseudo_fret,
        NULL,
        g4::NOSAT,
        exsize,
        NULL,
        NULL, //src0Opnd
        NULL,
        instOpts,
        true);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFRetInst(
    VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask, G4_Predicate *predOpnd)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_ExecSize exsize = toExecSize(executionSize);
    G4_InstOpts instOpts = Get_Gen4_Emask(emask, exsize);

    if (getFCPatchInfo()->getIsCallableKernel() == true)
    {
        if (tmpFCRet == nullptr) {
            input_info_t *RetIP = getRetIPArg();
            tmpFCRet = createTempVar(2, Type_UD, Four_Word);
            tmpFCRet->setAliasDeclare(RetIP->dcl, 0);
        }
        G4_SrcRegRegion* srcOpndToUse = createSrc(tmpFCRet->getRegVar(), 0, 0, getRegionStride1(),
            Type_UD);

        createInst(predOpnd,
            G4_pseudo_fc_ret,
            NULL,
            g4::NOSAT,
            g4::SIMD2,
            createNullDst(Type_UD),
            srcOpndToUse,
            NULL,
            instOpts,
            true);
    }
    else
    {
        createCFInst(predOpnd,
            subroutineId == 0 ? G4_pseudo_exit : GetGenOpcodeFromVISAOpcode(ISA_RET),
            exsize,
            nullptr, nullptr, instOpts, true);
    }

    return VISA_SUCCESS;
}

int IR_Builder::translateVISAGotoInst(
    G4_Predicate* predOpnd,
    VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask, G4_Label* label)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    G4_ExecSize exsize = toExecSize(executionSize);
    G4_InstOpts instOpts = Get_Gen4_Emask(emask, exsize);

    auto cfInst = createInst(
        predOpnd,
        G4_goto,
        nullptr,
        g4::NOSAT,
        exsize,
        nullptr,
        nullptr,
        nullptr,
        instOpts,
        true);
    cfInst->asCFInst()->setUip(label);

    return VISA_SUCCESS;
}

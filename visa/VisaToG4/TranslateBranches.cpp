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

#include "BuildIR.h"
#include "../Timer.h"


int IR_Builder::translateVISACFSwitchInst(
    G4_Operand *indexOpnd, uint8_t numLabels, G4_Label ** labels)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    // offsets are in bytes so we have to multiply everything by 16
    // FIXME: this assumes the jmpi instructions will never be compacted.
    if (indexOpnd->isImm())
    {
        indexOpnd = createImm(indexOpnd->asImm()->getInt() * 16, Type_W);
    }
    else
    {
        G4_Declare *tmpVar = createTempVar(1, Type_D, Any);
        G4_DstRegRegion* dstOpnd = Create_Dst_Opnd_From_Dcl(tmpVar, 1);
        createBinOp(G4_shl, g4::SIMD1, dstOpnd, indexOpnd,
            createImm(4, Type_UW), InstOpt_NoOpt, true);
        indexOpnd = Create_Src_Opnd_From_Dcl(tmpVar, getRegionScalar());
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
        dstOpndToUse = Create_Dst_Opnd_From_Dcl(FCRet, 1);

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
        0);

    return VISA_SUCCESS;
}

int IR_Builder::translateVISACFJumpInst(G4_Predicate *predOpnd, G4_Label* lab)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    createInst(
        predOpnd,
        GetGenOpcodeFromVISAOpcode((ISA_Opcode)ISA_JMP),
        NULL,
        g4::NOSAT,
        g4::SIMD1,
        NULL,
        lab,
        NULL,
        0,
        0);

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

    auto fcall = createInst(
        predOpnd,
        G4_pseudo_fcall,
        nullptr,
        g4::NOSAT,
        exsize,
        nullptr,
        createLabel(funcName, LABEL_FUNCTION),  //src0 is a fake label containing callee's name
        nullptr,
        0,
        0);

    m_fcallInfo[fcall] = new (mem) G4_FCALL(argSize, returnSize);

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
        createMov(g4::SIMD1, Create_Dst_Opnd_From_Dcl(tmpSrc0, 1), src0, InstOpt_WriteEnable, true);
        src0 = Create_Src_Opnd_From_Dcl(tmpSrc0, getRegionScalar());
    }
    // FIXME: Remove the "add" instruction here that this instruction must be right before
    //        call that the result PC will be corresponding to the call instruction.
    //        So may not create this "add" here
/*
    // Call target is (function_address - PC)
    G4_Declare* callOffset = createTempVar(1, Type_D, Any);
    createInst(nullptr, G4_add, nullptr, false, 1,
               Create_Dst_Opnd_From_Dcl(callOffset, 1), src0,
               createSrcRegRegion(Mod_Minus, Direct, phyregpool.getIpReg(), 0, 0,
                                  getRegionScalar(), Type_UD),
               InstOpt_WriteEnable);
    auto fcall = createInst(predOpnd, G4_pseudo_fcall, nullptr, false, exsize,
        nullptr, Create_Src_Opnd_From_Dcl(callOffset, getRegionScalar()), nullptr, 0, 0);
*/
    auto fcall = createInst(predOpnd, G4_pseudo_fcall, nullptr, g4::NOSAT, exsize,
        nullptr, src0, nullptr, 0, 0);

    m_fcallInfo[fcall] = new (mem) G4_FCALL(argSize, returnSize);

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
        dst->setType(Type_UD);
        G4_INST* mov = createMov(g4::SIMD1, dst, privateMemPatch, InstOpt_WriteEnable, true);
        RelocationEntry::createRelocation(kernel, *mov, 0, symbolName, GenRelocType::R_SYM_ADDR_32);
    }
    else if (noInt64() || needSwap64ImmLoHi())
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
        0);

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
        G4_SrcRegRegion* srcOpndToUse = createSrcRegRegion(Mod_src_undef,
            Direct, tmpFCRet->getRegVar(), 0, 0, getRegionStride1(),
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
            0);
    }
    else
    {
        createCFInst(predOpnd,
            subroutineId == 0 ? G4_pseudo_exit : GetGenOpcodeFromVISAOpcode(ISA_RET),
            exsize,
            nullptr, nullptr, instOpts);
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
        0);
    cfInst->asCFInst()->setUip(label);

    return VISA_SUCCESS;
}

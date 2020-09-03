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






// create a fence instruction to the data cache
// flushParam --
//              bit 0 -- commit enable
//              bit 1-4 -- L3 flush parameters
//              bit 5 -- global/SLM
//              bit 6 -- L1 flush
//              bit 7 -- SW fence only; a scheduling barrier but does not generate any code
// bit 7, if set, takes precedence over other bits
G4_INST* IR_Builder::createFenceInstruction(
    uint8_t flushParam, bool commitEnable, bool globalMemFence,
    bool isSendc = false)
{
#define L1_FLUSH_MASK 0x40

    int flushBits = (flushParam >> 1) & 0xF;

    bool L1Flush = (flushParam & L1_FLUSH_MASK) != 0 &&
        !(hasSLMFence() && !globalMemFence);

    int desc = 0x7 << 14 | ((commitEnable ? 1 : 0) << 13);

    desc |= flushBits << 9;

    if (L1Flush)
    {
#define L1_FLUSH_BIT_LOC 8
        desc |= 1 << L1_FLUSH_BIT_LOC;
    }

    G4_Declare *srcDcl = getBuiltinR0();
    G4_Declare *dstDcl = createTempVar(8, Type_UD, Any);
    G4_DstRegRegion *sendDstOpnd = commitEnable ? Create_Dst_Opnd_From_Dcl(dstDcl, 1) : createNullDst(Type_UD);
    G4_SrcRegRegion *sendSrcOpnd = Create_Src_Opnd_From_Dcl(srcDcl, getRegionStride1());
    uint8_t BTI = 0x0;

    if (hasSLMFence())
    {
        // we must choose either GLOBAL_MEM_FENCE or SLM_FENCE
        BTI = globalMemFence ? 0 : 0xfe;
    }

    // commitEnable = true: msg length = 1, response length = 1, dst == src
    // commitEnable = false: msg length = 1, response length = 0, dst == null
    return Create_Send_Inst_For_CISA(nullptr, sendDstOpnd, sendSrcOpnd, 1, (commitEnable ? 1 : 0), g4::SIMD8,
        desc, SFID::DP_DC, true, SendAccess::READ_WRITE, createImm(BTI, Type_UD), nullptr, InstOpt_WriteEnable, isSendc);
}

// create a default SLM fence (no flush)
G4_INST* IR_Builder::createSLMFence()
{
    bool commitEnable = needsFenceCommitEnable();
    return createFenceInstruction(0, commitEnable, false, false);
}


int IR_Builder::translateVISAWaitInst(G4_Operand* mask)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    // clear TDR if mask is not null and not zero
    if (mask != NULL && !(mask->isImm() && mask->asImm()->getInt() == 0))
    {
        // mov (1) f0.0<1>:uw <TDR_bits>:ub {NoMask}
        G4_Declare* tmpFlagDcl = createTempFlag(1);
        G4_DstRegRegion* newPredDef = Create_Dst_Opnd_From_Dcl(tmpFlagDcl, 1);
        createMov(g4::SIMD1, newPredDef, mask, InstOpt_WriteEnable, true);

        // (f0.0) and (8) tdr0.0<1>:uw tdr0.0<8;8,1>:uw 0x7FFF:uw {NoMask}
        G4_Predicate* predOpnd = createPredicate(PredState_Plus, tmpFlagDcl->getRegVar(), 0, PRED_DEFAULT);
        G4_DstRegRegion* TDROpnd = createDst(phyregpool.getTDRReg(), 0, 0, 1, Type_UW);
        G4_SrcRegRegion* TDRSrc = createSrcRegRegion(
            Mod_src_undef, Direct, phyregpool.getTDRReg(), 0, 0, getRegionStride1(), Type_UW);
        createInst(predOpnd, G4_and, NULL, g4::NOSAT, g4::SIMD8,
            TDROpnd, TDRSrc, createImm(0x7FFF, Type_UW), InstOpt_WriteEnable, 0);
    }

    createIntrinsicInst(nullptr, Intrinsic::Wait, g4::SIMD1,
        nullptr, nullptr, nullptr, nullptr, InstOpt_WriteEnable);

    return VISA_SUCCESS;
}


void IR_Builder::generateBarrierSend()
{


    // 1 message length, 0 response length, no header, no ack
    int desc = (0x1 << 25) + 0x4;

    //get barrier id
    G4_Declare *dcl = createSendPayloadDcl(GENX_DATAPORT_IO_SZ, Type_UD);

    G4_SrcRegRegion* r0_src_opnd = createSrcRegRegion(
        Mod_src_undef,
        Direct,
        builtinR0->getRegVar(),
        0,
        2,
        getRegionScalar(),
        Type_UD);

    G4_DstRegRegion *dst1_opnd = Create_Dst_Opnd_From_Dcl(dcl, 1);

    bool enableBarrierInstCounterBits = kernel.getOption(VISA_EnableBarrierInstCounterBits);
    int mask = getBarrierMask(enableBarrierInstCounterBits);

    G4_Imm *g4Imm = createImm(mask, Type_UD);

    createBinOp(
        G4_and,
        g4::SIMD8,
        dst1_opnd,
        r0_src_opnd,
        g4Imm,
        InstOpt_WriteEnable,
        true);

    // Generate the barrier send message
    auto msgDesc = createSyncMsgDesc(SFID::GATEWAY, desc);
    createSendInst(
        NULL,
        G4_send,
        g4::SIMD1,
        createNullDst(Type_UD),
        Create_Src_Opnd_From_Dcl(dcl, getRegionStride1()),
        createImm(desc, Type_UD),
        InstOpt_WriteEnable,
        msgDesc,
        0);
}

void IR_Builder::generateBarrierWait()
{
    G4_Operand* waitSrc = nullptr;
    if (!hasUnifiedBarrier()) {

        if (getPlatform() < GENX_TGLLP) {
            // before gen12: wait n0.0<0;1,0>:ud
            waitSrc = createSrcRegRegion(Mod_src_undef, Direct, phyregpool.getN0Reg(),
                0, 0, getRegionScalar(), Type_UD);
        } else {
            // gen12: sync.bar null
            waitSrc = createNullSrc(Type_UD);
        }
    }
    createInst(nullptr, G4_wait, nullptr, g4::NOSAT, g4::SIMD1,
        nullptr, waitSrc, nullptr, InstOpt_WriteEnable);
}

int IR_Builder::translateVISASyncInst(ISA_Opcode opcode, unsigned int mask)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    switch (opcode)
    {
    case ISA_BARRIER:
    {
        generateBarrierSend();
        generateBarrierWait();
    }
    break;
    case ISA_SAMPLR_CACHE_FLUSH:
    {
        // msg length = 1, response length = 1, header_present = 1,
        // Bit 16-12 = 11111 for Sampler Message Type
        // Bit 18-17 = 11 for SIMD32 mode
        int desc = (1 << 25) + (1 << 20) + (1 << 19) + (0x3 << 17) + (0x1F << 12);

        G4_Declare *dcl = getBuiltinR0();
        G4_Declare *dstDcl = createTempVar(8, Type_UD, Any);
        G4_DstRegRegion* sendDstOpnd = Create_Dst_Opnd_From_Dcl(dstDcl, 1);
        G4_SrcRegRegion* sendMsgOpnd = Create_Src_Opnd_From_Dcl(dcl, getRegionStride1());

        auto msgDesc = createSyncMsgDesc(SFID::SAMPLER, desc);
        createSendInst(nullptr, G4_send, g4::SIMD8, sendDstOpnd, sendMsgOpnd,
            createImm(desc, Type_UD), 0, msgDesc, 0);

        G4_SrcRegRegion* moveSrcOpnd = createSrcRegRegion(Mod_src_undef, Direct, dstDcl->getRegVar(), 0, 0, getRegionStride1(), Type_UD);
        Create_MOV_Inst(dstDcl, 0, 0, g4::SIMD8, NULL, NULL, moveSrcOpnd);
    }
    break;
    case ISA_WAIT:
    {
        //This should be handled by translateVISAWait() now
        MUST_BE_TRUE(false, "Should not reach here");
    }
    break;
    case ISA_YIELD:
    {
        G4_INST* lastInst = instList.empty() ? nullptr : instList.back();
        if (lastInst && lastInst->opcode() != G4_label)
        {
            lastInst->setOptionOn(InstOpt_Switch);
        }
        else
        {
            // dummy move to apply the {switch}
            G4_SrcRegRegion* srcOpnd = createSrcRegRegion(Mod_src_undef, Direct, getBuiltinR0()->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
            G4_DstRegRegion* dstOpnd = createDst(getBuiltinR0()->getRegVar(), 0, 0, 1, Type_UD);

            G4_INST* nop = createMov(g4::SIMD1, dstOpnd, srcOpnd, InstOpt_NoOpt, true);
            nop->setOptionOn(InstOpt_Switch);
        }
    }
    break;
    case ISA_FENCE:
    {
#define GLOBAL_MASK 0x20
        union fenceParam
        {
            VISAFenceMask mask;
            uint8_t data;
        };

        fenceParam fenceMask;
        fenceMask.data = mask & 0xFF;
        bool globalFence = (mask & GLOBAL_MASK) == 0;

        if (fenceMask.mask.SWFence)
        {
            createIntrinsicInst(
                nullptr, Intrinsic::MemFence, g4::SIMD1,
                nullptr, nullptr, nullptr, nullptr, InstOpt_NoOpt);
        }
        else if (VISA_WA_CHECK(m_pWaTable, WADisableWriteCommitForPageFault))
        {
            // write commit does not work under page fault
            // so we generate a fence without commit, followed by a read surface info to BTI 0
            createFenceInstruction((uint8_t) mask & 0xFF, false, globalFence);
            G4_Imm* surface = createImm(0, Type_UD);
            G4_Declare* zeroLOD = createTempVar(8, Type_UD, Any);
            Create_MOV_Inst(zeroLOD, 0, 0, g4::SIMD8, NULL, NULL, createImm(0, Type_UD));
            G4_SrcRegRegion* sendSrc = Create_Src_Opnd_From_Dcl(zeroLOD, getRegionStride1());
            G4_DstRegRegion* sendDst = Create_Dst_Opnd_From_Dcl(zeroLOD, 1);
            ChannelMask maskR = ChannelMask::createFromAPI(CHANNEL_MASK_R);
            translateVISAResInfoInst(EXEC_SIZE_8, vISA_EMASK_M1, maskR, surface, sendSrc, sendDst);
        }
        else
        {
            createFenceInstruction((uint8_t) mask & 0xFF, (mask & 0x1) == 0x1, globalFence);
            // The move to ensure the fence is actually complete will be added at the end of compilation,
            // in Optimizer::HWWorkaround()
        }
        break;
    }
    default:
        return VISA_FAILURE;
    }

    return VISA_SUCCESS;
}

int IR_Builder::translateVISASplitBarrierInst(bool isSignal)
{
    TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

    if (isSignal)
    {
        generateBarrierSend();
    }
    else
    {
        generateBarrierWait();
    }

    return VISA_SUCCESS;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LatencyTable.h"
#include "LocalScheduler_G4IR.h"
#include "../G4_IR.hpp"

using namespace vISA;

uint16_t LatencyTable::getLatency(G4_INST* Inst) const
{
    auto GEN = m_builder->getPlatformGeneration();
    if (GEN >= PlatformGen::XE)
        return getLatencyG12(Inst);

    return getLatencyLegacy(Inst);
}

uint16_t LatencyTable::getDPAS8x8Latency() const
{
    switch(m_builder->getPlatform())
    {
        case Xe_XeHPSDV:
        case Xe_PVC:
            return uint16_t(LatenciesXe::DPAS + 7); //28
        case Xe_PVCXT:
            return uint16_t(LatenciesXe::DPAS + 1 + 7); //29
        case Xe_DG2:
            return 32;
        default: //Not suppport platform
           return 46;
    }
}

// This calculates the node's pipeline occupancy (node delay)
uint16_t LatencyTable::getOccupancy(G4_INST* Inst) const
{
    auto GEN = m_builder->getPlatformGeneration();
    if (GEN >= PlatformGen::XE)
        return getOccupancyG12(Inst);

    return getOccupancyLegacy(Inst);
}


uint16_t LatencyTable::getLatencyLegacy(G4_INST* Inst) const
{
    if (Inst->isSend())
    {
        G4_SendDesc* MsgDesc = Inst->getMsgDesc();
        return LegacyFFLatency[SFIDtoInt(MsgDesc->getSFID())];
    } else if (Inst->isMath()) {
        if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
            Inst->asMathInst()->getMathCtrl() == MATH_POW)
            return LegacyLatencies::EDGE_LATENCY_MATH_TYPE2;
        return LegacyLatencies::EDGE_LATENCY_MATH;
    }
    return LegacyLatencies::IVB_PIPELINE_LENGTH;
}

uint16_t LatencyTable::getOccupancyLegacy(G4_INST* Inst) const
{
    int divisor = 8;
    int InstLatency = LegacyLatencies::UNCOMPR_LATENCY;
    if (Inst->isFastHFInstruction()) {
        divisor = 16;
    }

    // Number of n-wide passes in FPU0 or FPU1 (EM).
    // "n" is:
    //      16 for BDW+ HalfFloatDoublePerf instructions,
    //      8 for other instructions.
    int passes = std::max(1, Inst->getExecSize() / divisor);

    // InstLatency is:
    //      4 for EM/FPU1 POW and FDIV instrutions ( HSW; for BDW+ it is 2 times higher ),
    //      2 for other EM/FPU1 instructions ( HSW; for BDW+ it is 2 times higher ),
    //      2 for other instructions.
    // Update DagNode latency for math.
    G4_opcode opCode = Inst->opcode();
    switch (opCode) {
    case G4_math: {
        // Use EdgeLatencyMathType2 for FDIV, FPOW functions.
        if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
            Inst->asMathInst()->getMathCtrl() == MATH_POW) {
            InstLatency = 4;
        } else {
            // Used EdgeLatencyMath for other functions.
            InstLatency = 2;
        }

        // BDW+ platforms have lower math TPT and longer latency (all math functions).
        InstLatency *= 2;
        break;
    }
    case G4_bfe:
    case G4_bfi1:
    case G4_bfi2:
    case G4_bfrev:
    case G4_cbit:
    case G4_dp2:
    case G4_dp3:
    case G4_dp4:
    case G4_dph:
    case G4_fbh:
    case G4_fbl:
    case G4_lrp:
    case G4_mac:
    case G4_mach:
    case G4_pln:
        InstLatency *= 2;
        break;
    case G4_label:
        // Labels need special care. They should have a latency of 1.
        // But their execSize is 255, which sets passes=31.
        passes = 1;
        InstLatency = 1;
        break;
    default:
        break;
    }

    return uint16_t(passes * InstLatency);
}

uint16_t LatencyTable::getLatencyG12(const G4_INST* Inst) const
{
    int Sz = Inst->getExecSize();
    int Scale = (Sz <= 8) ? 0 : (Sz == 16) ? 1 : 3;
    auto Dst = Inst->getDst();

    if (Inst->isSend()) {
        G4_SendDesc* MsgDesc = Inst->getMsgDesc();
        if (MsgDesc->isLSC())
        {
            if (MsgDesc->isFence())
            {
                return MsgDesc->isTyped() ?
                    LatenciesXe::LSC_TYPED_FENCE : LatenciesXe::LSC_UNTYPED_FENCE;
            }
            else
            {
                bool isCachedInL1 = MsgDesc->getCachingL1() == Caching::CA ||
                    (MsgDesc->getCachingL1() != Caching::UC && m_builder->getOption(vISA_assumeL1Hit));
                if (MsgDesc->isLSC() && MsgDesc->isTyped())
                {
                    return isCachedInL1 ? LatenciesXe::LSC_TYPED_L1 : LatenciesXe::LSC_TYPED_L3;
                }
                else
                {
                    return isCachedInL1 ? LatenciesXe::LSC_UNTYPED_L1 : LatenciesXe::LSC_UNTYPED_L3;
                }
            }
        }
        if (MsgDesc->isSLM())
            return Inst->asSendInst()->isFence() ? LatenciesXe::SLM_FENCE : LatenciesXe::SLM;
        if (MsgDesc->isSampler())
            return LatenciesXe::SAMPLER_L3;
        if (MsgDesc->isHDC())
            return LatenciesXe::DP_L3;
        if (MsgDesc->isBarrier())
            return LatenciesXe::BARRIER;
         return LatenciesXe::SEND_OTHERS;
    }
    if (Inst->isMath())
    {
        return uint16_t(LatenciesXe::MATH + LatenciesXe::DELTA_MATH * Scale);
    }
    if (Inst->isFlowControl())
    {
        return LatenciesXe::BRANCH;
    }
    if (Inst->isDpas()) {

        if (m_builder->getPlatform() == Xe_PVC)
        {
            G4_InstDpas *dpas = Inst->asDpasInst();
            return uint16_t(LatenciesXe::DPAS + dpas->getRepeatCount() - 1);
        }

        if (m_builder->getPlatform() == Xe_PVCXT)
        {
            G4_InstDpas *dpas = Inst->asDpasInst();
            return uint16_t(LatenciesXe::DPAS + 1 + dpas->getRepeatCount() - 1); //22 ~29
        }

        if (m_builder->getPlatform() == Xe_DG2)
        {
            G4_InstDpas *dpas = Inst->asDpasInst();
            switch(dpas->getRepeatCount())
            {
            case 1:
                return 21;
            case 2:
                return 22;
            case 8:
                return 32;
            default:
                return 32;
            }
        }
        G4_InstDpas* dpas = Inst->asDpasInst();
        return uint16_t(LatenciesXe::DPAS + dpas->getRepeatCount() - 1);
    }
    if (Inst->writesFlag() || (Dst && Dst->isA0()))
    {
        return LatenciesXe::ARF;
    }
    if (Inst->isArithmetic()) {
        if (Dst->isAccReg())
            return uint16_t(LatenciesXe::FPU_ACC + LatenciesXe::DELTA * Scale);
        return uint16_t(LatenciesXe::FPU + LatenciesXe::DELTA * Scale);
    }

    // By default, use the FPU pipeline latency.
    return uint16_t(LatenciesXe::FPU);
}

uint16_t LatencyTable::getOccupancyG12(G4_INST* Inst) const
{
    enum OccupancyXe {
        G12_OC_MATH = 4,
        G12_OC_Others = 1
    };

    int Sz = Inst->getExecSize();
    int Scale = (Sz <= 8) ? 1 : (Sz == 16) ? 2 : 4;
    if (Inst->isMath())
        return uint16_t(G12_OC_MATH * Scale);
    if (Inst->isFastHFInstruction())
        Scale = (Sz <= 16) ? 1 : 2;
    else if (G4_DstRegRegion* Dst = Inst->getDst()) {
        if (Dst->getTypeSize() == 8)
            Scale = (Sz <= 4) ? 1 : 2;
    }
    return uint16_t(G12_OC_Others * Scale);
}

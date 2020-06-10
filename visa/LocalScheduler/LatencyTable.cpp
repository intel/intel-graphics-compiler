#include "LatencyTable.h"
#include "LocalScheduler_G4IR.h"
#include "../Gen4_IR.hpp"

using namespace vISA;

uint16_t LatencyTable::getLatency(G4_INST* Inst) const
{
    auto GEN = getPlatformGeneration(m_builder->getPlatform());
    if (GEN >= PlatformGen::GEN12)
        return getLatencyG12(Inst);

    return getLatencyLegacy(Inst);
}

// This calculates the node's pipeline occupancy (node delay)
uint16_t LatencyTable::getOccupancy(G4_INST* Inst) const
{
    auto GEN = getPlatformGeneration(m_builder->getPlatform());
    if (GEN >= PlatformGen::GEN12)
        return getOccupancyG12(Inst);

    return getOccupancyLegacy(Inst);
}



uint16_t LatencyTable::getLatencyLegacy(G4_INST* Inst) const
{
    if (Inst->isSend())
    {
        G4_SendMsgDescriptor* MsgDesc = Inst->getMsgDesc();
        return LegacyFFLatency[SFIDtoInt(MsgDesc->getFuncId())];
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

uint16_t LatencyTable::getLatencyG12(G4_INST* Inst) const
{
    int Sz = Inst->getExecSize();
    int Scale = (Sz <= 8) ? 0 : (Sz == 16) ? 1 : 3;

    if (Inst->isSend()) {
        G4_SendMsgDescriptor* MsgDesc = Inst->getMsgDesc();
        if (MsgDesc->isSLMMessage())
            return Inst->asSendInst()->isFence() ? GEN12Latencies::SLM_FENCE : GEN12Latencies::SLM;
        if (MsgDesc->isSampler())
            return GEN12Latencies::SAMPLER_L3;
        if (MsgDesc->isHDC())
            return GEN12Latencies::DP_L3;
        if (MsgDesc->isBarrierMsg())
            return GEN12Latencies::BARRIER;
         return GEN12Latencies::SEND_OTHERS;
    } else if (Inst->isMath()) {
        return uint16_t(GEN12Latencies::MATH + GEN12Latencies::DELTA_MATH * Scale);
    } else if (Inst->isFlowControl()) {
        return GEN12Latencies::BRANCH;
    }
    else if (Inst->isArithmetic()) {
        G4_DstRegRegion *Dst = Inst->getDst();
        if (Dst->isAccReg())
            return uint16_t(GEN12Latencies::FPU_ACC + GEN12Latencies::DELTA * Scale);
        return uint16_t(GEN12Latencies::FPU + GEN12Latencies::DELTA * Scale);
    }

    // By default, use the FPU pipeline latency.
    return uint16_t(GEN12Latencies::FPU);
}

uint16_t LatencyTable::getOccupancyG12(G4_INST* Inst) const
{
    enum GEN12Occupancy {
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
        if (G4_Type_Table[Dst->getType()].byteSize == 8)
            Scale = (Sz <= 4) ? 1 : 2;
    }
    return uint16_t(G12_OC_Others * Scale);
}

#include "LatencyTable.h"
#include "LocalScheduler_G4IR.h"
#include "../Gen4_IR.hpp"

using namespace vISA;

uint16_t LatencyTable::getLatencyPreRA(G4_INST* Inst) const
{
    return getLatencyLegacy(Inst);
}

uint16_t LatencyTable::getLatencyPostRA(G4_INST* Inst) const
{
    return getLatencyLegacy(Inst);
}

uint16_t LatencyTable::getLatencyLegacy(G4_INST* Inst) const
{
    if (Inst->isSend()) {
        G4_SendMsgDescriptor* MsgDesc = Inst->getMsgDesc();
        return MsgDesc->getFFLatency();
    } else if (Inst->isMath()) {
        if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
            Inst->asMathInst()->getMathCtrl() == MATH_POW)
            return EDGE_LATENCY_MATH_TYPE2;
        return EDGE_LATENCY_MATH;
    }
    return IVB_PIPELINE_LENGTH;
}

// This calculates the node's pipeline occupancy (node delay)
uint16_t LatencyTable::getOccupany(G4_INST* inst) const
{
    return getOccupanyLegacy(inst);
}

uint16_t LatencyTable::getOccupanyLegacy(G4_INST* inst) const
{
    int divisor = 8;
    int instLatency = UNCOMPR_LATENCY;
    if (inst->isFastHFInstruction()) {
        divisor = 16;
    }

    // Number of n-wide passes in FPU0 or FPU1 (EM).
    // "n" is:
    //      16 for BDW+ HalfFloatDoublePerf instructions,
    //      8 for other instructions.
    int passes = std::max(1, inst->getExecSize() / divisor);

    // InstLatency is:
    //      4 for EM/FPU1 POW and FDIV instrutions ( HSW; for BDW+ it is 2 times higher ),
    //      2 for other EM/FPU1 instructions ( HSW; for BDW+ it is 2 times higher ),
    //      2 for other instructions.
    // Update DagNode latency for math.
    G4_opcode opCode = inst->opcode();
    switch (opCode) {
    case G4_math: {
        // Use EdgeLatencyMathType2 for FDIV, FPOW functions.
        if (inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
            inst->asMathInst()->getMathCtrl() == MATH_POW) {
            instLatency = 4;
        } else {
            // Used EdgeLatencyMath for other functions.
            instLatency = 2;
        }

        // BDW+ platforms have lower math TPT and longer latency (all math functions).
        instLatency *= 2;
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
        instLatency *= 2;
        break;
    case G4_label:
        // Labels need special care. They should have a latency of 1.
        // But their execSize is 255, which sets passes=31.
        passes = 1;
        instLatency = 1;
        break;
    default:
        break;
    }

    return uint16_t(passes * instLatency);
}

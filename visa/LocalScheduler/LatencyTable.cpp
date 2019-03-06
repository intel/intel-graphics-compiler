#include "LatencyTable.h"
#include "LocalScheduler_G4IR.h"
#include "../Gen4_IR.hpp"

using namespace vISA;

uint16_t LatencyTable::getLatency(G4_INST* Inst) const
{
    return getLatencyLegacy(Inst);
}

// This calculates the node's pipeline occupancy (node delay)
uint16_t LatencyTable::getOccupancy(G4_INST* Inst) const
{
    return getOccupancyLegacy(Inst);
}

static const uint16_t LegacyFFLatency[] = {
    2,   // 0: SFID_NULL
    2,   // 1: Useless
    300, // 2: SFID_SAMPLER
    200, // 3: SFID_GATEWAY
    400, // 4: SFID_DP_READ, SFID_DP_DC2
    200, // 5: SFID_DP_WRITE
    50,  // 6: SFID_URB
    50,  // 7: SFID_SPAWNER
    50,  // 8: SFID_VME
    60,  // 9: SFID_DP_CC
    400, //10: SFID_DP_DC
    50,  //11: SFID_DP_PI
    400, //12: SFID_DP_DC1
    200, //13: SFID_CRE
    200  //14: unknown, SFID_NUM
};

uint16_t LatencyTable::getLatencyLegacy(G4_INST* Inst) const
{
    if (Inst->isSend()) {
        G4_SendMsgDescriptor* MsgDesc = Inst->getMsgDesc();
        return LegacyFFLatency[SFIDtoInt(MsgDesc->getFuncId())];
    } else if (Inst->isMath()) {
        if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
            Inst->asMathInst()->getMathCtrl() == MATH_POW)
            return EDGE_LATENCY_MATH_TYPE2;
        return EDGE_LATENCY_MATH;
    }
    return IVB_PIPELINE_LENGTH;
}

uint16_t LatencyTable::getOccupancyLegacy(G4_INST* Inst) const
{
    int divisor = 8;
    int InstLatency = UNCOMPR_LATENCY;
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


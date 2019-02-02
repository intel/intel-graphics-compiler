#include "LatencyTable.h"
#include "LocalScheduler_G4IR.h"
#include "../Gen4_IR.hpp"

using namespace vISA;

unsigned LatencyTable::getLatencyPreRA(G4_INST* Inst) const
{
    return getLatencyLegacy(Inst);
}

unsigned LatencyTable::getLatencyPostRA(G4_INST* Inst) const
{
    return getLatencyLegacy(Inst);
}

unsigned LatencyTable::getLatencyLegacy(G4_INST* Inst) const
{
    if (Inst->isSend()) {
        if (G4_SendMsgDescriptor* MsgDesc = Inst->getMsgDesc())
            return MsgDesc->getFFLatency();
        return G4_SendMsgDescriptor::getDefaultFFLatency();
    } else if (Inst->isMath()) {
        if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
            Inst->asMathInst()->getMathCtrl() == MATH_POW)
            return EDGE_LATENCY_MATH_TYPE2;
        return EDGE_LATENCY_MATH;
    }
    return IVB_PIPELINE_LENGTH;
}

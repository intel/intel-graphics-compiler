#ifndef __LATENCY_TABLE_H
#define __LATENCY_TABLE_H

#include "../BuildIR.h"
namespace vISA
{
    class LatencyTable {
    public:
        struct Latency {
            Latency(uint32_t EL, uint32_t ND, uint32_t UM)
                : latency(EL), occupancy(ND), occupancyMultiplier(UM) { ; }
            Latency(uint32_t EL, uint32_t ND)
                : latency(EL), occupancy(ND), occupancyMultiplier(1) { ; }
            Latency(void)
                : latency(0), occupancy(0), occupancyMultiplier(1) { ; }
            uint32_t getSum(void) const {
                return latency + occupancy * occupancyMultiplier;
            };
            uint32_t getSumOldStyle(void) const {
                return latency + occupancy;
            };
            uint32_t getLatencyOnly(void) const { return latency; }
            uint32_t getOccupancyOnly(void) const {
                return occupancy * occupancyMultiplier;
            }
            uint32_t latency;
            uint32_t occupancy;
            uint32_t occupancyMultiplier;
        };
    private:
        const Options *m_options;
        std::map<G4_opcode, Latency> InstLatTable;
        std::map<G4_MathOp, Latency> MathLatTable;
        std::map<CISA_SHARED_FUNCTION_ID, Latency> SendLatTable;
    public:
        LatencyTable(const Options *options) : m_options(options) {
#undef DEF_INSTR_LATENCY
#undef DEF_MATH_LATENCY
#undef DEF_SEND_LATENCY
#define DEF_INSTR_LATENCY(OP, LAT, DEL) InstLatTable[OP] = {LAT, DEL};
#define DEF_MATH_LATENCY(...)
#define DEF_SEND_LATENCY(...)
#include "SKL_latencies.def"

#undef DEF_INSTR_LATENCY
#undef DEF_MATH_LATENCY
#undef DEF_SEND_LATENCY
#define DEF_INSTR_LATENCY(...)
#define DEF_MATH_LATENCY(OP, LAT, DEL) MathLatTable[OP] = {LAT, DEL};
#define DEF_SEND_LATENCY(...)
#include "SKL_latencies.def"

#undef DEF_INSTR_LATENCY
#undef DEF_MATH_LATENCY
#undef DEF_SEND_LATENCY
#define DEF_INSTR_LATENCY(...)
#define DEF_MATH_LATENCY(...)
#define DEF_SEND_LATENCY(OP, LAT, DEL) SendLatTable[OP] = {LAT, DEL};
#include "SKL_latencies.def"
        }

        Latency getLatency(G4_INST *inst) const {
            uint32_t latency = 0;
            uint32_t occupancy = 0;

            int execSize = std::max(8, (int)inst->getExecSize());
            uint32_t occupancyMultiplier = execSize/8;
            // :hf instructions have half the occupancy
            if (inst->isFastHFInstruction()) {
                occupancyMultiplier /= 2;
            }

            // 1. MATH
            if (inst->isMath()) {
                G4_MathOp mop = inst->asMathInst()->getMathCtrl();
                latency = MathLatTable.at(mop).latency;
                occupancy = MathLatTable.at(mop).occupancy;
            }
            // 2. SEND
            else if (inst->isSend()) {
                G4_SendMsgDescriptor *msgDesc = inst->getMsgDesc();
                assert(msgDesc);
                CISA_SHARED_FUNCTION_ID sfid = msgDesc->getFuncId();
                latency = SendLatTable.at(sfid).latency;
                occupancy = SendLatTable.at(sfid).occupancy;
                // Force latency. FIXME: is this correct?
                uint32_t forceLatency
                    = m_options->getuInt32Option(vISA_UnifiedSendCycle);
                if (forceLatency) {
                    latency = forceLatency;
                }
            }
            // 3. OTHER INSTRUCTION
            else {
                G4_opcode opcode = inst->opcode();
                switch (opcode) {
                case G4_label:
                    latency = 1;
                    occupancy = 1;
                    occupancyMultiplier = 1;
                    break;
                case G4_mul: {
                    G4_DstRegRegion *dstRgn = inst->getDst();
                    assert(dstRgn);
                    G4_Type dstType = dstRgn->getType();
                    G4_Type src1Type
                        = inst->getSrc(0)->asSrcRegRegion()->getType();
                    G4_Type src2Type
                        = inst->getSrc(1)->asSrcRegRegion()->getType();
                    uint32_t extraLatency = 0;
                    if (IS_TYPE_INT(dstType)
                        && IS_DTYPE(src1Type)
                        && IS_DTYPE(src2Type)) {
                        extraLatency = MUL_INTEGER_EXTRA_LATENCY;
                    }
                    assert(InstLatTable.count(opcode));
                    latency = InstLatTable.at(opcode).latency + extraLatency;
                    occupancy = InstLatTable.at(opcode).occupancy;
                    break;
                }
                default:
                    auto it = InstLatTable.find(opcode);
                    // If opcode not defined, use the values for ADD
                    if (it == InstLatTable.end()) {
                        latency = InstLatTable.at(G4_add).latency;
                        occupancy = InstLatTable.at(G4_add).occupancy;
                    } else {
                        latency = it->second.latency;
                        occupancy = it->second.occupancy;
                    }
                    break;
                }
            }

            return Latency(latency, occupancy, occupancyMultiplier);
        }
    };
}

#endif

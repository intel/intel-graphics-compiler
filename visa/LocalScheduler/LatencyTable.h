#ifndef __LATENCY_TABLE_H
#define __LATENCY_TABLE_H

#include "../BuildIR.h"
namespace vISA {
class LatencyTable {
public:
    struct Latency {
        Latency(uint32_t EL, uint32_t ND)
            : latency(EL)
            , occupancy(ND)
        {
        }

        uint32_t getLatency() const
        {
            return latency;
        }
        uint32_t getOccupancy() const
        {
            return occupancy;
        }
        uint32_t latency = 0;
        uint32_t occupancy = 0;
    };

public:
    explicit LatencyTable(const IR_Builder* builder)
        : m_builder(builder)
    {
    }

    unsigned getLatencyPostRA(G4_INST* Inst) const;
    unsigned getLatencyPreRA(G4_INST* Inst) const;

private:
    unsigned getLatencyLegacy(G4_INST* inst) const;

    const IR_Builder* m_builder;
    std::map<G4_opcode, Latency> InstLatTable;
    std::map<G4_MathOp, Latency> MathLatTable;
    std::map<CISA_SHARED_FUNCTION_ID, Latency> SendLatTable;
};

} // namespace vISA

#endif

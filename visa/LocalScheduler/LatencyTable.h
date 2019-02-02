#ifndef __LATENCY_TABLE_H
#define __LATENCY_TABLE_H

#include "../BuildIR.h"
namespace vISA {
class LatencyTable {
public:
    struct Latency {
        Latency(uint16_t EL, uint16_t ND)
            : latency(EL)
            , occupancy(ND)
        {
        }

        uint16_t getLatency() const
        {
            return latency;
        }
        uint16_t getOccupancy() const
        {
            return occupancy;
        }
        uint16_t latency = 0;
        uint16_t occupancy = 0;
    };

public:
    explicit LatencyTable(const IR_Builder* builder)
        : m_builder(builder)
    {
    }

    uint16_t getOccupany(G4_INST* inst) const;
    uint16_t getLatencyPostRA(G4_INST* Inst) const;
    uint16_t getLatencyPreRA(G4_INST* Inst) const;

private:
    uint16_t getLatencyLegacy(G4_INST* inst) const;
    uint16_t getOccupanyLegacy(G4_INST* inst) const;

    const IR_Builder* m_builder;
    std::map<G4_opcode, Latency> InstLatTable;
    std::map<G4_MathOp, Latency> MathLatTable;
    std::map<CISA_SHARED_FUNCTION_ID, Latency> SendLatTable;
};

} // namespace vISA

#endif

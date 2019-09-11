#ifndef __LATENCY_TABLE_H
#define __LATENCY_TABLE_H

#include "../BuildIR.h"
namespace vISA {
class LatencyTable {
public:
    explicit LatencyTable(const IR_Builder* builder)
        : m_builder(builder)
    {
    }

    uint16_t getOccupancy(G4_INST* Inst) const;
    uint16_t getLatency(G4_INST* Inst) const;

private:
    uint16_t getLatencyLegacy(G4_INST* Inst) const;
    uint16_t getOccupancyLegacy(G4_INST* Inst) const;

    uint16_t getLatencyG12(G4_INST* Inst) const;
    uint16_t getOccupancyG12(G4_INST* Inst) const;

    const IR_Builder* m_builder;
};

} // namespace vISA

#endif

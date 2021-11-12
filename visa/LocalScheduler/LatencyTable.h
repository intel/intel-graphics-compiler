/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __LATENCY_TABLE_H
#define __LATENCY_TABLE_H

#include "../BuildIR.h"

namespace vISA
{

    enum LegacyLatencies : uint16_t
    {
        //
        //  General instruction latencies
        //
        // To be comptabile with send cycles, don't normalized them to 1
        UNCOMPR_LATENCY         = 2,    // Latency of an uncompressed instruction
        COMPR_LATENCY           = 4,    // Latency of a compressed instruction
        ACC_BUBBLE              = 4,    // Accumulator back-to-back stall
        IVB_PIPELINE_LENGTH     = 14,
        EDGE_LATENCY_MATH       = 22,
        EDGE_LATENCY_MATH_TYPE2 = 30,
        EDGE_LATENCY_SEND_WAR   = 36
    };

    //
    // Message latencies
    //
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


    enum LatenciesXe : uint16_t
    {
        //
        // General instruction latencies
        //
        FPU_ACC                 = 6,    // SIMD8 latency if dst is acc.
        FPU                     = 10,   // SIMD8 latency for general FPU ops.
        MATH                    = 17,   // Math latency.
        BRANCH                  = 23,   // Latency for SIMD16 branch.
        BARRIER                 = 30,   // Latency for barrier.
        DELTA                   = 1,    // Extra cycles for wider SIMD sizes, compute only.
        DELTA_MATH              = 4,
        ARF                     = 16,   // latency for ARF dependencies (flag, address, etc.)
        // Latency for dpas 8x1
        // Latency for dpas 8x8 is 21 + 7 = 28
        DPAS = 21,

        //
        // Message latencies
        //

        // Latency for SIMD16 SLM messages. If accessing
        // the same location, it takes 28 cycles. For the
        // sequential access pattern, it takes 26 cycles.
        SLM                     = 28,
        SEND_OTHERS             = 50,   // Latency for other messages.
        DP_L3                   = 146,  // Dataport L3 hit
        SAMPLER_L3              = 214,  // Sampler L3 hit
        SLM_FENCE               = 23,   // Fence SLM
        LSC_UNTYPED_L1          = 45,   // LSC untyped L1 cache hit
        LSC_UNTYPED_L3          = 200,  // LSC untyped L3 cache hit
        LSC_UNTYPED_FENCE       = 35,   // LSC untyped fence (best case)
        LSC_TYPED_L1            = 75,   // LSC typed L1 cache hit
        LSC_TYPED_L3            = 200,  // LSC typed L3 cache hit
        LSC_TYPED_FENCE         = 60,   // LSC typed fence
    };


    class LatencyTable
    {
    public:
        explicit LatencyTable(const IR_Builder* builder)
            : m_builder(builder)
        {
        }
        // Functions to get latencies/occupancy based on platforms
        uint16_t getOccupancy(G4_INST* Inst) const;
        uint16_t getLatency(G4_INST* Inst) const;
        uint16_t getDPAS8x8Latency() const;

    private:
        uint16_t getLatencyLegacy(G4_INST* Inst) const;
        uint16_t getOccupancyLegacy(G4_INST* Inst) const;

        uint16_t getLatencyG12(const G4_INST* Inst) const;

        uint16_t getOccupancyG12(G4_INST* Inst) const;

        const IR_Builder* m_builder;
    };

} // namespace vISA

#endif

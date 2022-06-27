/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __RPE_H__
#define __RPE_H__

#include "RegAlloc.h"
#include "Mem_Manager.h"
#include <unordered_map>

namespace vISA
{
    class RPE
    {
    public:
        RPE(const GlobalRA&, const LivenessAnalysis*);

        ~RPE()
        {

        }

        void run();
        void runBB(G4_BB*);
        unsigned int getRegisterPressure(G4_INST* inst)
        {
            auto it = rp.find(inst);
            if (it == rp.end())
                return 0;
            return it->second;
        }

        unsigned int getMaxRP() const
        {
            return maxRP;
        }

        const LivenessAnalysis* getLiveness() const { return liveAnalysis; }

        void recomputeMaxRP();

        void dump() const;

    private:
        Mem_Manager m;
        const GlobalRA& gra;
        const LivenessAnalysis* const liveAnalysis;
        std::unordered_map<G4_INST*, unsigned int> rp;
        double regPressure = 0;
        uint32_t maxRP = 0;
        const Options* options;
        SparseBitSet live;
        const std::vector<G4_RegVar*>& vars;

        void regPressureBBExit(G4_BB*);
        void updateRegisterPressure(unsigned int, unsigned int, unsigned int);
        void updateLiveness(SparseBitSet&, uint32_t, bool);
    };
}
#endif

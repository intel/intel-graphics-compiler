/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
        RPE(const GlobalRA&, LivenessAnalysis*);

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

        unsigned int getMaxRP()
        {
            return maxRP;
        }

        const LivenessAnalysis* getLiveness() { return liveAnalysis; }

        void recomputeMaxRP();

        void dump() const;

    private:
        Mem_Manager m;
        const GlobalRA& gra;
        const LivenessAnalysis* liveAnalysis = nullptr;
        std::unordered_map<G4_INST*, unsigned int> rp;
        double regPressure = 0;
        uint32_t maxRP = 0;
        const Options* options;
        BitSet live;
        const std::vector<G4_RegVar*>& vars;

        void regPressureBBExit(G4_BB*);
        void updateRegisterPressure(unsigned int, unsigned int, unsigned int);
        void updateLiveness(BitSet&, uint32_t, bool);
    };
}
#endif

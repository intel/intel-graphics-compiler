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

#ifndef __SPILLCLEANUP_H__
#define __SPILLCLEANUP_H__

#include "Gen4_IR.hpp"
#include "FlowGraph.h"
#include "RPE.h"

namespace vISA
{
    class CoalesceSpillFills
    {
    private:
        G4_Kernel& kernel;
        LivenessAnalysis& liveness;
        GraphColor& graphColor;
        SpillManagerGMRF& spill;
        unsigned int iterNo;
        // Store declares spilled by sends like sampler
        std::set<G4_Declare*> sendDstDcl;
        std::set<G4_Declare*> addrTakenSpillFillDcl;
        RPE& rpe;

        // Set window size to coalesce
        const unsigned int cWindowSize = 10;
        const unsigned int cMaxFillPayloadSize = 4;
        const unsigned int cMaxSpillPayloadSize = 4;
        const unsigned int cSpillFillCleanupWindowSize = 10;
        const unsigned int cFillWindowThreshold128GRF = 180;
        const unsigned int cSpillWindowThreshold128GRF = 120;

        unsigned int fillWindowSizeThreshold = 0;
        unsigned int spillWindowSizeThreshold = 0;

        // <Old fill declare*, std::pair<Coalesced Decl*, Row Off>>
        // This data structure is used to replaced old spill/fill operands
        // with coalesced operands with correct offset.
        std::map<G4_Declare*, std::pair<G4_Declare*, unsigned int>> replaceMap;

        void replaceCoalescedOperands(G4_INST*);
        void insertKill(G4_BB*, INST_LIST_ITER, std::set<G4_Declare*>&);

        void dumpKernel();
        void dumpKernel(unsigned int v1, unsigned int v2);

        void sendsInRange(std::list<INST_LIST_ITER>&,
            std::list<INST_LIST_ITER>&,
            unsigned int, unsigned int&, unsigned int&);
        void keepConsecutiveSpills(std::list<INST_LIST_ITER>&,
            std::list<INST_LIST_ITER>&,
            unsigned int, unsigned int&, unsigned int&, bool&,
            G4_InstOption&);
        void fills();
        void spills();
        INST_LIST_ITER analyzeFillCoalescing(std::list<INST_LIST_ITER>&, INST_LIST_ITER, INST_LIST_ITER, G4_BB*);
        INST_LIST_ITER analyzeSpillCoalescing(std::list<INST_LIST_ITER>&, INST_LIST_ITER, INST_LIST_ITER, G4_BB*);
        void removeWARFills(std::list<INST_LIST_ITER>&, std::list<INST_LIST_ITER>&);
        void coalesceFills(std::list<INST_LIST_ITER>&, unsigned int, unsigned int, G4_BB*, int);
        G4_DstRegRegion* generateCoalescedFill(unsigned int, unsigned int, unsigned int, G4_SendMsgDescriptor*, int, G4_Align);
        G4_SrcRegRegion* generateCoalescedSpill(unsigned int, unsigned int, G4_SendMsgDescriptor*, bool,
            G4_InstOption, int, G4_Declare*, unsigned int);
        void copyToOldFills(G4_DstRegRegion*, std::list<std::pair<G4_DstRegRegion*, std::pair<unsigned int, unsigned int>>>,
            INST_LIST_ITER, G4_BB*, int);
        bool fillHeuristic(std::list<INST_LIST_ITER>&,
            std::list<INST_LIST_ITER>&, const std::list<INST_LIST_ITER>&, unsigned int&, unsigned int&);
        bool overlap(G4_INST*, std::list<INST_LIST_ITER>&);
        bool overlap(G4_INST*, G4_INST*, bool&);
        void coalesceSpills(std::list<INST_LIST_ITER>&, unsigned int,
            unsigned int, bool, G4_InstOption, G4_BB*, int);
        bool allSpillsSameVar(std::list<INST_LIST_ITER>&);
        void fixSendsSrcOverlap();
        void removeRedundantSplitMovs();
        G4_Declare* createCoalescedSpillDcl(unsigned int);
        void populateSendDstDcl();
        void spillFillCleanup();
        void removeRedundantWrites();
        void computeAddressTakenDcls();

    public:
        CoalesceSpillFills(G4_Kernel& k, LivenessAnalysis& l, GraphColor& g,
            SpillManagerGMRF& s, unsigned int iterationNo, RPE& r) :
            kernel(k), liveness(l), graphColor(g), spill(s), rpe(r)
        {
            unsigned int numGRFs = k.getNumRegTotal();
            auto scale = [=](unsigned threshold) -> unsigned {
                float ratio = 1.0f - (128 - threshold) / 128.0f;
                return static_cast<unsigned>(numGRFs * ratio);
            };
            fillWindowSizeThreshold = scale(cFillWindowThreshold128GRF);
            spillWindowSizeThreshold = scale(cSpillWindowThreshold128GRF);

            iterationNo = iterNo;

            computeAddressTakenDcls();
        }

        void run();

        static void getScratchMsgInfo(G4_INST* inst, unsigned int& scratchOffset, unsigned int& size)
        {
            scratchOffset = inst->getMsgDesc()->getScratchRWOffset();
            size = inst->getMsgDesc()->getScratchRWSize();
        }
    };
}

#endif

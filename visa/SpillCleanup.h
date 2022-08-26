/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPILLCLEANUP_H__
#define __SPILLCLEANUP_H__

#include "G4_IR.hpp"
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
        GlobalRA& gra;
        SpillManagerGRF& spill;
        unsigned int iterNo;
        // Store declares spilled by sends like sampler
        std::set<G4_Declare*> sendDstDcl;
        RPE& rpe;

        // Set window size to coalesce
        const unsigned int cWindowSize = 10;
        const unsigned int cMaxFillPayloadSize = 4;
        const unsigned int cMaxSpillPayloadSize = 4;
        const unsigned int cSpillFillCleanupWindowSize = 10;
        const unsigned int cFillWindowThreshold128GRF = 180;
        const unsigned int cSpillWindowThreshold128GRF = 120;
        const unsigned int cHighRegPressureForCleanup = 100;

        unsigned int fillWindowSizeThreshold = 0;
        unsigned int spillWindowSizeThreshold = 0;
        unsigned int highRegPressureForCleanup = 0;

        // <Old fill declare*, std::pair<Coalesced Decl*, Row Off>>
        // This data structure is used to replaced old spill/fill operands
        // with coalesced operands with correct offset.
        std::map<G4_Declare*, std::pair<G4_Declare*, unsigned int>> replaceMap;

        bool replaceCoalescedOperands(G4_INST*);

        void dumpKernel();
        void dumpKernel(unsigned int v1, unsigned int v2);

        bool notOOB(unsigned int min, unsigned int max);
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
        G4_INST* generateCoalescedFill(G4_SrcRegRegion*, unsigned int, unsigned int, unsigned int, bool);
        G4_SrcRegRegion* generateCoalescedSpill(G4_SrcRegRegion*, unsigned int, unsigned int, bool,
            G4_InstOption, G4_Declare*, unsigned int);
        void copyToOldFills(G4_DstRegRegion*, std::list<std::pair<G4_DstRegRegion*, std::pair<unsigned int, unsigned int>>>,
            INST_LIST_ITER, G4_BB*, int);
        bool fillHeuristic(std::list<INST_LIST_ITER>&,
            std::list<INST_LIST_ITER>&, const std::list<INST_LIST_ITER>&, unsigned int&, unsigned int&);
        bool overlap(G4_INST*, std::list<INST_LIST_ITER>&);
        bool overlap(G4_INST*, G4_INST*, bool&);
        void coalesceSpills(std::list<INST_LIST_ITER>&, unsigned int, unsigned int, bool, G4_InstOption, G4_BB*);
        bool allSpillsSameVar(std::list<INST_LIST_ITER>&);
        void fixSendsSrcOverlap();
        void removeRedundantSplitMovs();
        G4_Declare* createCoalescedSpillDcl(unsigned int);
        void populateSendDstDcl();
        void spillFillCleanup();
        void removeRedundantWrites();

    public:
        CoalesceSpillFills(G4_Kernel& k, LivenessAnalysis& l, GraphColor& g,
            SpillManagerGRF& s, unsigned int iterationNo, RPE& r, GlobalRA& gr) :
            kernel(k), liveness(l), graphColor(g), gra(gr), spill(s), iterNo(iterationNo), rpe(r)
        {
            unsigned int numGRFs = k.getNumRegTotal();
            auto scale = [=](unsigned threshold) -> unsigned {
                float ratio = 1.0f - (128 - threshold) / 128.0f;
                return static_cast<unsigned>(numGRFs * ratio);
            };
            fillWindowSizeThreshold = scale(cFillWindowThreshold128GRF);
            spillWindowSizeThreshold = scale(cSpillWindowThreshold128GRF);
            highRegPressureForCleanup = scale(cHighRegPressureForCleanup);
        }

        void run();

        static void getScratchMsgInfo(G4_INST* inst, unsigned int& scratchOffset, unsigned int& size)
        {
            if (inst->isSpillIntrinsic())
            {
                scratchOffset = inst->asSpillIntrinsic()->getOffset();
                size = inst->asSpillIntrinsic()->getNumRows();
            }
            else if (inst->isFillIntrinsic())
            {
                scratchOffset = inst->asFillIntrinsic()->getOffset();
                size = inst->asFillIntrinsic()->getNumRows();
            }
            else
            {
                MUST_BE_TRUE(false, "unknown inst type");
            }
        }
    };
}

#endif

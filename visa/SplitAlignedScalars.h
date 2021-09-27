/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPLITALIGNEDSCALARS_H__
#define __SPLITALIGNEDSCALARS_H__

#include "visa/GraphColor.h"

namespace vISA
{
    class SplitAlignedScalars
    {
    private:
        const unsigned int MinOptDist = 200;
        // Constant trip count assume for each loop to estimate dynamic inst
        // count change due to splitting.
        const unsigned int EstimatedLoopTripCount = 4;
        // Threshold percent increase in estimated dynamic inst count allowed
        const float BloatAllowed = 1.0f / 100.0f;

        unsigned int numDclsReplaced = 0;
        unsigned int numMovsAdded = 0;
        GlobalRA& gra;
        GraphColor& coloring;
        G4_Kernel& kernel;
        bool changesMade = false;

        class Data
        {
        public:
            unsigned int firstDef = 0;
            unsigned int lastUse = 0;
            bool allowed = true;
            unsigned int getDUMaxDist()
            {
                return std::abs((int)lastUse - (int)firstDef);
            };
            std::vector<std::pair<G4_BB*, G4_INST*>> defs;
            // store vector of <bb, inst, src#>
            std::vector <std::tuple<G4_BB*, G4_INST*, unsigned int>> uses;
        };

        std::unordered_map<G4_Declare*, Data> dclData;
        std::unordered_map<G4_Declare*, G4_Declare*> oldNewDcls;

        bool static canReplaceDst(G4_INST* inst);
        bool static canReplaceSrc(G4_INST* inst, unsigned int idx);

        bool heuristic(G4_Declare* dcl, Data& d);
        bool isDclCandidate(G4_Declare* dcl);
        std::vector<G4_Declare*> gatherCandidates();
        void pruneCandidates(std::vector<G4_Declare*>& candidates);
        unsigned int computeNumMovs(G4_Declare* dcl);

        template<class T>
        G4_Declare* getDclForRgn(T* rgn, G4_Declare* newTopDcl);

        // store set of dcls marked as spill in current RA iteration
        std::unordered_set<G4_Declare*> spilledDclSet;
        // store spill cost for each dcl
        std::unordered_map<G4_Declare*, float> dclSpillCost;
        // store dcls that have callee save bias
        std::unordered_set<G4_Declare*> calleeSaveBiased;

    public:
        SplitAlignedScalars(GlobalRA& g, GraphColor& c) : gra(g), coloring(c), kernel(g.kernel)
        {
            for (auto spill : coloring.getSpilledLiveRanges())
            {
                spilledDclSet.insert(spill->getDcl());
            }
            auto numVars = coloring.getNumVars();
            auto lrs = coloring.getLiveRanges();
            for (unsigned int i = 0; i != numVars; ++i)
            {
                auto rootDcl = lrs[i]->getDcl();
                dclSpillCost[rootDcl] = lrs[i]->getSpillCost();
                if (lrs[i]->getCalleeSaveBias())
                    calleeSaveBiased.insert(rootDcl);
            }
        }

        void run();

        bool getChangesMade() { return changesMade; }

        void dump(std::ostream& of = std::cerr);
    };

}
#endif

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
        const unsigned int MinBBSize = 5;

        unsigned int numDclsReplaced = 0;
        unsigned int numMovsAdded = 0;
        GlobalRA& gra;
        G4_Kernel& kernel;
        bool changesMade = false;

        class Data
        {
        public:
            unsigned int firstDef = 0;
            unsigned int lastUse = 0;
            unsigned int numDefs = 0;
            unsigned int numSrcs = 0;
            bool allowed = true;
            unsigned int getDUMaxDist()
            {
                return std::abs((int)lastUse - (int)firstDef);
            };
        };

        std::unordered_map<G4_Declare*, Data> dclData;
        std::unordered_map<G4_Declare*, G4_Declare*> oldNewDcls;

        bool static canReplaceDst(G4_INST* inst);
        bool static canReplaceSrc(G4_INST* inst, unsigned int idx);

        bool heuristic(G4_Declare* dcl, Data& d);
        bool isDclCandidate(G4_Declare* dcl);
        void gatherCandidates();

        template<class T>
        G4_Declare* getDclForRgn(T* rgn, G4_Declare* newTopDcl);

    public:
        SplitAlignedScalars(GlobalRA& g) : gra(g), kernel(g.kernel)
        {
        }

        void run();

        bool getChangesMade() { return changesMade; }

        void dump(std::ostream& of = std::cerr);
    };

}
#endif

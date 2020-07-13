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

#ifndef _LOCAL_DATAFLOW_H
#define _LOCAL_DATAFLOW_H

#include "FlowGraph.h"
#include "BuildIR.h"
#include <unordered_map>

using namespace vISA;
using namespace std;

// ToDo: move Local dataflow here and make it a class

namespace vISA {

    // A simple local (within one BB) analysis that checks if a def escapes (i.e., reaches the end of BB without other defines killing it).
    // This information may be helpful for a number of optimizations by determining if a define has only local effect.
    // Destination may be GRF or Address (i.e., direct DstRegRegion).
    // Acc and other special ARFs are not considered as their defintions should always be local.
    // Flag dst (i.e., conditional modifier) is currently not handled; while adding it is possible it may lead to confusion for instructions with both dst and condMod.
    // To reduce compilation overhead and implementation complexity only a single killing defintion is considered for now;
    // this is conservative but should cover most pratical cases as our input is almost SSA.
    class DefEscapeBBAnalysis
    {

        const FlowGraph& fg;

        // all escaped inst for this BB
        // If BB is not in the map, it means we need to run analysis for this BB
        // ToDo: assumption is that there should not be many escaped inst. Can change to vector to set later if desired.
        std::unordered_map<G4_BB*, std::vector<G4_INST*> > escapedInsts;

#ifdef _DEBUG
        // for debugging only, record instructions that were killed in the BB
        std::unordered_map<G4_BB*, std::vector<G4_INST*> > killedDefs;
#endif

    public:

        DefEscapeBBAnalysis(const FlowGraph& cfg) : fg(cfg) {}

        DefEscapeBBAnalysis(const DefEscapeBBAnalysis& analysis) = delete;

        virtual ~DefEscapeBBAnalysis() = default;

        void run()
        {
            for (auto&& bb : const_cast<FlowGraph&>(fg))
            {
                analyzeBB(bb);
            }
        }

        void analyzeBB(G4_BB* bb);

        bool isBBValid(G4_BB* bb) const { return escapedInsts.count(bb); }
        void invalidate() { escapedInsts.clear(); }
        void invalidateBB(G4_BB* bb) { escapedInsts.erase(bb); }
        bool isEscaped(G4_BB* bb, G4_INST* inst)
        {
            if (!isBBValid(bb))
            {
                analyzeBB(bb);
            }
            auto&& vec = escapedInsts.find(bb)->second;
            return std::find(vec.begin(), vec.end(), inst) != vec.end();
        }
        void print(std::ostream& OS) const;
        void dump() const
        {
            print(std::cerr);
        }
    };
}
#endif // _LOCAL_DATAFLOW_H

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
#ifndef _VARSPLIT_H_
#define _VARSPLIT_H_

#include "FlowGraph.h"
#include "BuildIR.h"

namespace vISA
{
class LiveRange;

class VarProperties
{
public:
    enum class AccessGranularity
    {
        OneGrf = 1,
        TwoGrf = 2,
        Unknown = 3
    };

    AccessGranularity ag = AccessGranularity::Unknown;
    unsigned int numDefs = 0;
    std::pair<G4_DstRegRegion*, G4_BB*> def;
    std::vector<std::pair<G4_SrcRegRegion*, G4_BB*>> srcs;
    bool legitCandidate = true;

    bool isPartDclUsed(unsigned int lb, unsigned int rb)
    {
        // Return true if lb/rb is part of any src regions
        for (auto& src : srcs)
        {
            if (src.first->getLeftBound() >= lb &&
                src.first->getRightBound() <= rb)
                return true;
        }
        return false;
    }
};

class VarSplitPass
{
public:
    VarSplitPass(G4_Kernel&);

    void run();
    void replaceIntrinsics();
    G4_Declare* getParentDcl(G4_Declare*);
    std::vector<G4_Declare*>* getChildren(G4_Declare*);
    std::vector<G4_Declare*> getSiblings(G4_Declare*);
    bool isSplitDcl(G4_Declare*);
    bool isPartialDcl(G4_Declare*);
    unsigned int getSiblingNum(G4_Declare*);
    unsigned int getIdealAllocation(G4_Declare*, LiveRange**);
    bool isChildDclUnused(G4_Declare*);
    void writeHints(G4_Declare*, LiveRange**);
    void undo(G4_Declare*);
    bool reallocParent(G4_Declare*, LiveRange**);
    bool isParentChildRelation(G4_Declare*, G4_Declare*);

private:
    G4_Kernel& kernel;

    void findSplitCandidates();
    void split();
    std::unordered_map<G4_Declare*, VarProperties> splitVars;
    // split dcl, parent dcl
    std::unordered_map<G4_Declare*, G4_Declare*> splitParentDcl;
    // parent dcl, vector<children>
    std::unordered_map<G4_Declare*, std::vector<G4_Declare*>> splitChildren;
    // Store child dcls that are never referenced in CFG
    std::unordered_set<G4_Declare*> unusedDcls;
    // Store pre-split regions for undo
    // <new src/dst region, <old src inst, old src rgn, old src#>>
    std::unordered_map<G4_Operand*, std::tuple<G4_INST*, G4_Operand*, unsigned int>> preSplit;
};
};
#endif

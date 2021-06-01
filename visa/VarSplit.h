/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
    bool candidateDef = false;
    bool legitCandidate = true;

    // API to check whether variable is local or global
    bool isDefUsesInSameBB()
    {
        auto defBB = def.second;
        for (auto src : srcs)
        {
            if (src.second != defBB)
                return false;
        }
        return true;
    }

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
    bool isSplitVarLocal(G4_Declare*);
    bool splitOccured() { return IRchanged; }

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
    bool IRchanged = false;

private:
    // Split verification related declarations
    void buildPreVerify();
    void verify();
    void verifyOverlap();

    class InstData
    {
    public:
        G4_DstRegRegion* dst = nullptr;
        unsigned int dstLb = 0;
        unsigned int dstRb = 0;
        G4_Operand* src[G4_MAX_SRCS];
        unsigned int srcLb[G4_MAX_SRCS];
        unsigned int srcRb[G4_MAX_SRCS];

        InstData()
        {
            for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
            {
                src[i] = nullptr;
                srcLb[i] = 0;
                srcRb[i] = 0;
            }
        }
    };
    std::unordered_map<G4_INST*, InstData> splitVerify;
};
};
#endif

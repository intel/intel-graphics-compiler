/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _VARSPLIT_H_
#define _VARSPLIT_H_

#include "FlowGraph.h"
#include "BuildIR.h"
#include "RPE.h"

namespace vISA
{
class LiveRange;
class GraphColor;
class RPE;
class GlobalRA;

// store mapping of a split variable to original variable. if any split
// variable is spilled, we can reuse spill location of original variable.
// also store all instructions emitted in preheader, loop exit for each
// split variable. this helps eliminate those instruction in case the
// split variable itself spills.
class SplitResults
{
public:
    G4_Declare* origDcl = nullptr;
    std::unordered_map<G4_BB*, std::unordered_set<G4_INST*>> insts;
};

class LoopVarSplit
{
public:
    LoopVarSplit(G4_Kernel& k, GraphColor* c, RPE* r);

    void run();

    std::vector<G4_SrcRegRegion*> getReads(G4_Declare* dcl, Loop& loop);
    std::vector<G4_DstRegRegion*> getWrites(G4_Declare* dcl, Loop& loop);
    unsigned int getMaxRegPressureInLoop(Loop& loop);
    void dump(std::ostream& of = std::cerr);

    static void removeSplitInsts(GlobalRA* gra, G4_Declare* spillDcl, G4_BB* bb);
    static bool removeFromPreheader(GlobalRA* gra, G4_Declare* spillDcl, G4_BB* bb, INST_LIST_ITER filledInstIter);
    static bool removeFromLoopExit(GlobalRA* gra, G4_Declare* spillDcl, G4_BB* bb, INST_LIST_ITER filledInstIter);
    static const std::unordered_set<G4_INST*> getSplitInsts(GlobalRA* gra, G4_BB* bb);

private:
    bool split(G4_Declare* dcl, Loop& loop);
    void copy(G4_BB* bb, G4_Declare* dst, G4_Declare* src, SplitResults* splitData, bool pushBack = true);
    void replaceSrc(G4_SrcRegRegion* src, G4_Declare* dcl);
    void replaceDst(G4_DstRegRegion* dst, G4_Declare* dcl);
    G4_Declare* getNewDcl(G4_Declare* dcl1, G4_Declare* dcl2);
    std::vector<Loop*> getLoopsToSplitAround(G4_Declare* dcl);

    G4_Kernel& kernel;
    GraphColor* coloring = nullptr;
    RPE* rpe = nullptr;
    VarReferences references;

    // store set of dcls marked as spill in current RA iteration
    std::unordered_set<G4_Declare*> spilledDclSet;

    // store spill cost for each dcl
    std::map<G4_Declare*, float> dclSpillCost;

    std::unordered_map<G4_Declare*, G4_Declare*> oldNewDcl;

    std::unordered_map<Loop*, std::unordered_set<G4_Declare*>> splitsPerLoop;

    std::unordered_map<Loop*, unsigned int> maxRegPressureCache;

    // a spilled dcl may be split multiple times, once per loop
    // store this information to uplevel to GlobalRA class so
    // anytime we spill a split variable, we reuse spill location.
    // Orig dcl, vector<Tmp Dcl, Loop>
    std::unordered_map<G4_Declare*, std::vector<std::pair<G4_Declare*, Loop*>>> splitResults;
};

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

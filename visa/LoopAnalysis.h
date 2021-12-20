/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "G4_IR.hpp"
#include <vector>
#include <list>
#include <unordered_set>

namespace vISA
{
    class G4_BB;
    class FlowGraph;

    // Top level Analysis class that each analysis needs to inherit.
    // Each inherited class needs to implement their own reset() and
    // run() classes.
    class Analysis
    {
    public:
        void setStale() { stale = true; }
        void setValid() { stale = false; }

        bool isStale() const { return stale; }

        void recomputeIfStale();

        virtual void reset() = 0;
        virtual void run() = 0;
        virtual void dump(std::ostream& os = std::cerr) = 0;
    private:
        bool stale = true;
        // flag to avoid re-triggering of analysis run when run is already in progress
        bool inProgress = false;
    };

    class ImmDominator : public Analysis
    {
    public:
        ImmDominator(G4_Kernel& k) : kernel(k)
        {
        }

        ~ImmDominator()
        {

        }

        bool dominates(G4_BB *bb1, G4_BB *bb2);
        void dumpImmDom(std::ostream& os = std::cerr);
        const std::vector<G4_BB*>& getIDoms();

    private:
        G4_Kernel& kernel;
        G4_BB* entryBB = nullptr;
        std::vector<G4_BB*> iDoms;
        // TODO: Internal data to be removed.
        std::vector<std::vector<G4_BB*>> immDoms;

        void runIDOM();
        G4_BB* InterSect(G4_BB* bb, int i, int k);

        void reset() override;
        void run() override;
        void dump(std::ostream& os = std::cerr) override;
    };

    class PostDom : public Analysis
    {
    public:
        PostDom(G4_Kernel&);
        std::unordered_set<G4_BB*>& getPostDom(G4_BB*);
        std::vector<G4_BB*>& getImmPostDom(G4_BB*);
        void dumpImmDom(std::ostream& os = std::cerr);

        G4_BB* getCommonImmDom(std::unordered_set<G4_BB*>&);

    private:
        G4_Kernel& kernel;
        G4_BB* exitBB = nullptr;
        std::vector<std::unordered_set<G4_BB*>> postDoms;
        std::vector<std::vector<G4_BB*>> immPostDoms;

        void updateImmPostDom();

        void reset() override;
        void run() override;
        void dump(std::ostream& os = std::cerr) override { dumpImmDom(os); }
    };

    // Computes and stores direct references of variables.
    // Indirects references are not computed here.
    class VarReferences : public Analysis
    {
    public:
        VarReferences(G4_Kernel& k) : kernel(k) {}

        // Defs -> vector[tuple<inst, bb, lb, rb>]
        // Uses -> vector[tuple<inst, bb>]
        using Defs = std::vector<std::tuple<G4_INST*, G4_BB*, unsigned int, unsigned int>>;
        using Uses = std::vector<std::tuple<G4_INST*, G4_BB*>>;

        bool isUniqueDef(G4_Operand* dst);
        unsigned int getDefCount(G4_Declare* dcl);
        unsigned int getUseCount(G4_Declare* dcl);
        const Defs* getDefs(G4_Declare* dcl);
        const Uses* getUses(G4_Declare* dcl);

    private:
        // Dcl -> vector[<inst, bb, lb, rb>]
        // this data structure helps check whether a definition or part of it
        // has multiple definitions in the program.
        std::unordered_map<G4_Declare*, std::pair<Defs, Uses>> VarRefs;
        G4_Kernel& kernel;

        void reset() override;
        void run() override;
        void dump(std::ostream& os = std::cerr) override;
    };

    using BackEdge = std::pair<G4_BB*, G4_BB*>;
    using BackEdges = std::vector<BackEdge>;

    class Loop
    {
    public:
        Loop(BackEdge b) : be(b) {}

        Loop* parent = nullptr;
        G4_BB* preHeader = nullptr;
        std::vector<Loop*> immNested;

        void addBBToLoopHierarchy(G4_BB* bb);
        void addBBToLoop(G4_BB* bb);

        unsigned int id = 0;

        std::vector<Loop*> getAllSiblings(std::vector<Loop*>& topLoops);

        // BBs not in loop are considered to have nesting level of 0.
        // BBs in outermost loop report nesting level 1.
        // BB in loopn reports nesting level to be 1+it's parent nesting level.
        unsigned int getNestingLevel();

        void dump(std::ostream& os = std::cerr);

        bool contains(const G4_BB*);

        unsigned int getBBSize() { return BBs.size(); }

        G4_BB* getHeader() { return be.second; }

        bool fullSubset(Loop* other);
        bool fullSuperset(Loop* other);

        Loop* getInnerMostLoop(const G4_BB* bb);

        std::vector<G4_BB*>& getLoopExits();

        const std::vector<G4_BB*>& getBBs() { return BBs; }

    private:
        std::vector<G4_BB*> BBs;
        std::unordered_set<const G4_BB*> BBsLookup;
        BackEdge be;
        std::vector<G4_BB*> loopExits;
    };

    class FuncInfo;

    class LoopDetection : public Analysis
    {
    public:
        LoopDetection(G4_Kernel&);

        std::vector<Loop*> getTopLoops();
        Loop* getInnerMostLoop(const G4_BB*);
        void computePreheaders();

    private:
        std::vector<Loop*> topLoops;
        // list owns memory, so no need for dynamic allocation
        std::list<Loop> allLoops;

        // store G4_BB -> <preId, rpostId>
        std::unordered_map<const G4_BB*, std::pair<unsigned int, unsigned int>> PreIdRPostId;

        G4_Kernel& kernel;
        FlowGraph& fg;

        void reset() override;
        void run() override;
        void dump(std::ostream& os = std::cerr) override;

        void DFSTraverse(const G4_BB* startBB, unsigned& preId, unsigned& postId, BackEdges& bes);
        void findDominatingBackEdges(BackEdges& bes);
        void populateLoop(BackEdge&);
        void computeLoopTree();
        void addLoop(Loop* newLoop, Loop* aParent);
        G4_BB* getPreheader(Loop* loop);
    };
}


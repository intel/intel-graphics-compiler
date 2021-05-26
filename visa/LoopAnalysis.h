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

    class Dominator : public Analysis
    {
    public:
        Dominator(G4_Kernel& k) : kernel(k)
        {
        }

        ~Dominator()
        {
        }

        std::unordered_set<G4_BB*>& getDom(G4_BB*);
        std::vector<G4_BB*>& getImmDom(G4_BB*);
        G4_BB* getCommonImmDom(const std::unordered_set<G4_BB*>& bbs);
        G4_BB* InterSect(G4_BB* bb, int i, int k);
        void dumpImmDom(std::ostream& os = std::cerr);
        void dumpDom(std::ostream& os = std::cerr);
        bool dominates(G4_BB* bb1, G4_BB* bb2);
        const std::vector<G4_BB*>& getIDoms();

    private:
        G4_Kernel& kernel;
        G4_BB* entryBB = nullptr;
        std::vector<G4_BB*> iDoms;
        std::vector<std::unordered_set<G4_BB*>> Doms;
        std::vector<std::vector<G4_BB*>> immDoms;

        void runIDOM();
        void runDOM();

        void updateImmDom();

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

    using BackEdge = std::pair<G4_BB*, G4_BB*>;
    using BackEdges = std::vector<BackEdge>;

    class Loop
    {
    public:
        Loop(BackEdge b) : be(b) {}

        Loop* parent = nullptr;
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

    private:
        std::vector<G4_BB*> BBs;
        std::unordered_set<const G4_BB*> BBsLookup;
        BackEdge be;
    };

    class FuncInfo;

    class LoopDetection : public Analysis
    {
    public:
        LoopDetection(G4_Kernel&);

        std::vector<Loop*> getTopLoops();

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
    };
}


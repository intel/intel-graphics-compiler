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
#include <unordered_set>

namespace vISA
{
    class G4_BB;

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
        void dumpImmDom();

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
    };

    class PostDom : public Analysis
    {
    public:
        PostDom(G4_Kernel&);
        std::unordered_set<G4_BB*>& getPostDom(G4_BB*);
        std::vector<G4_BB*>& getImmPostDom(G4_BB*);
        void dumpImmDom();
        G4_BB* getCommonImmDom(std::unordered_set<G4_BB*>&);

    private:
        G4_Kernel& kernel;
        G4_BB* exitBB = nullptr;
        std::vector<std::unordered_set<G4_BB*>> postDoms;
        std::vector<std::vector<G4_BB*>> immPostDoms;

        void updateImmPostDom();

        void reset() override;
        void run() override;
    };
}


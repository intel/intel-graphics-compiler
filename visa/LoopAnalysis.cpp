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

#include "LoopAnalysis.h"
#include "G4_Kernel.hpp"

using namespace vISA;

G4_BB* Dominator::InterSect(G4_BB* bb, int i, int k)
{
    recomputeIfStale();

    G4_BB* finger1 = immDoms[bb->getId()][i];
    G4_BB* finger2 = immDoms[bb->getId()][k];

    while ((finger1 != finger2) &&
        (finger1 != nullptr) &&
        (finger2 != nullptr))
    {
        if (finger1->getPreId() == finger2->getPreId())
        {
            assert(finger1 == kernel.fg.getEntryBB() || finger2 == kernel.fg.getEntryBB());
            return kernel.fg.getEntryBB();
        }

        while ((iDoms[finger1->getId()] != nullptr) &&
            (finger1->getPreId() > finger2->getPreId()))
        {
            finger1 = iDoms[finger1->getId()];
            immDoms[bb->getId()][i] = finger1;
        }

        while ((iDoms[finger2->getId()] != nullptr) &&
            (finger2->getPreId() > finger1->getPreId()))
        {
            finger2 = iDoms[finger2->getId()];
            immDoms[bb->getId()][k] = finger2;
        }

        if ((iDoms[finger2->getId()] == nullptr) ||
            (iDoms[finger1->getId()] == nullptr))
        {
            break;
        }
    }

    if (finger1 == finger2)
    {
        return finger1;
    }
    else if (finger1->getPreId() > finger2->getPreId())
    {
        return finger2;
    }
    else
    {
        return finger1;
    }
}

/*
* An improvement on the algorithm from "A Simple, Fast Dominance Algorithm"
* 1. Single pred assginment.
* 2. To reduce the back trace in the intersect function, a temp buffer for predictor of each nodes is used to record the back trace result.
*/
void Dominator::runIDOM()
{
    iDoms.resize(kernel.fg.size());
    immDoms.resize(kernel.fg.size());

    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        iDoms[bb->getId()] = nullptr;
        immDoms[bb->getId()].resize(bb->Preds.size());

        size_t i = 0;
        for (auto pred : bb->Preds)
        {
            immDoms[bb->getId()][i] = pred;
            i++;
        }
    }

    entryBB = kernel.fg.getEntryBB();
    iDoms[entryBB->getId()] = { entryBB };

    // Actual dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
        {
            auto bb = *I;
            if (bb == entryBB)
                continue;

            if (bb->Preds.size() == 1)
            {
                if (iDoms[bb->getId()] == nullptr)
                {
                    iDoms[bb->getId()] = (*bb->Preds.begin());
                    change = true;
                }
                else
                {
                    assert(iDoms[bb->getId()] == (*bb->Preds.begin()));
                }
            }
            else
            {
                G4_BB* tmpIdom = nullptr;
                int i = 0;
                for (auto pred : bb->Preds)
                {
                    if (iDoms[pred->getId()] != nullptr)
                    {
                        tmpIdom = pred;
                        break;
                    }
                    i++;
                }

                if (tmpIdom != nullptr)
                {
                    int k = 0;
                    for (auto pred : bb->Preds)
                    {
                        if (k == i)
                        {
                            k++;
                            continue;
                        }

                        if (iDoms[pred->getId()] != nullptr)
                        {
                            tmpIdom = InterSect(bb, i, k);
                        }
                        k++;
                    }

                    if (iDoms[bb->getId()] == nullptr ||
                        iDoms[bb->getId()] != tmpIdom)
                    {
                        iDoms[bb->getId()] = tmpIdom;
                        change = true;
                    }
                }
            }
        }
    }
}

void Dominator::runDOM()
{
    Doms.resize(kernel.fg.size());
    entryBB = kernel.fg.getEntryBB();

    MUST_BE_TRUE(entryBB != nullptr, "Entry BB not found!");

    Doms[entryBB->getId()] = { entryBB };
    std::unordered_set<G4_BB*> allBBs;
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        allBBs.insert(bb);
    }

    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        if (bb != entryBB)
        {
            Doms[bb->getId()] = allBBs;
        }
    }

    // Actual dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
        {
            auto bb = *I;
            if (bb == entryBB)
                continue;

            std::unordered_set<G4_BB*> tmp = { bb };

            // Compute intersection of dom of preds
            std::unordered_map<G4_BB*, unsigned int> numInstances;

            //
            for (auto preds : bb->Preds)
            {
                auto& domPred = Doms[preds->getId()];
                for (auto domPredBB : domPred)
                {
                    auto it = numInstances.find(domPredBB);
                    if (it == numInstances.end())  //Not found
                        numInstances.insert(std::make_pair(domPredBB, 1));
                    else
                        it->second = it->second + 1;
                }
            }

            // Common BBs appear in numInstances map with second value == bb->Preds count
            for (auto commonBBs : numInstances)
            {
                if (commonBBs.second == bb->Preds.size()) //same size means the bb from all preds.
                    tmp.insert(commonBBs.first);
            }

            // Check if Dom set changed for bb in current iter
            if (tmp.size() != Doms[bb->getId()].size())  //Same size
            {
                Doms[bb->getId()] = tmp;
                change = true;
                continue;
            }
            else //Same
            {
                auto& domBB = Doms[bb->getId()];
                for (auto tmpBB : tmp)
                {
                    if (domBB.find(tmpBB) == domBB.end()) //Same BB
                    {
                        Doms[bb->getId()] = tmp;
                        change = true;
                        break;
                    }
                    if (change)
                        break;
                }
            }
        }
    }

    updateImmDom();
}


std::unordered_set<G4_BB*>& Dominator::getDom(G4_BB* bb)
{
    recomputeIfStale();

    return Doms[bb->getId()];
}

std::vector<G4_BB*>& Dominator::getImmDom(G4_BB* bb)
{
    recomputeIfStale();

    return immDoms[bb->getId()];
}

void Dominator::updateImmDom()
{
    std::vector<BitSet> domBits(kernel.fg.size());

    for (size_t i = 0; i < kernel.fg.size(); i++)
    {
        domBits[i] = BitSet(unsigned(kernel.fg.size()), false);
    }

    // Update immDom vector with correct ordering
    for (auto bb : kernel.fg)
    {
        auto& DomBBs = Doms[bb->getId()];

        for (auto domBB : DomBBs)
        {
            domBits[bb->getId()].set(domBB->getId(), true);
        }
    }

    iDoms.resize(kernel.fg.size());
    for (auto bb : kernel.fg)
    {
        auto& DomBBs = Doms[bb->getId()];
        BitSet tmpBits = domBits[bb->getId()];
        tmpBits.set(bb->getId(), false);
        iDoms[bb->getId()] = bb;

        for (auto domBB : DomBBs)
        {
            if (domBB == bb)
                continue;

            if (tmpBits == domBits[domBB->getId()])
            {
                iDoms[bb->getId()] = domBB;
            }
        }
    }
}

void vISA::Dominator::reset()
{
    iDoms.clear();
    Doms.clear();
    immDoms.clear();

    setStale();
}

void vISA::Dominator::run()
{
    // this function re-runs analysis. caller needs to check if
    // analysis is stale.
    entryBB = kernel.fg.getEntryBB();

    runDOM();
    runIDOM();

    setValid();
}

const std::vector<G4_BB*>& vISA::Dominator::getIDoms()
{
    recomputeIfStale();

    return iDoms;
}

G4_BB* Dominator::getCommonImmDom(const std::unordered_set<G4_BB*>& bbs)
{
    recomputeIfStale();

    if (bbs.size() == 0)
        return nullptr;

    unsigned int maxId = (*bbs.begin())->getId();

    auto commonImmDoms = getImmDom(*bbs.begin());
    for (auto bb : bbs)
    {
        maxId = std::max(maxId, bb->getId());

        const auto& DomBB = Doms[bb->getId()];
        for (G4_BB*& dom : commonImmDoms)
        {
            if (dom != nullptr && DomBB.find(dom) == DomBB.end())
            {
                dom = nullptr;
            }
        }
    }

    // Return first imm dom that is not a BB from bbs set
    for (G4_BB* dom : commonImmDoms)
    {
        if (dom &&
            // Common imm pdom must be lexically last BB
            dom->getId() >= maxId &&
            ((dom->size() > 1 && dom->front()->isLabel()) ||
                (dom->size() > 0 && !dom->front()->isLabel())))
        {
            return dom;
        }
    }

    return entryBB;
}

void Dominator::dumpImmDom()
{
    for (auto bb : kernel.fg)
    {
        printf("BB%d - ", bb->getId());
        auto& domBBs = immDoms[bb->getId()];
        for (auto domBB : domBBs)
        {
            printf("BB%d", domBB->getId());
            if (domBB->getLabel())
            {
                printf(" (%s)", domBB->getLabel()->getLabel());
            }
            printf(", ");
        }
        printf("\n");
    }
}

void vISA::Analysis::recomputeIfStale()
{
    if (!isStale() || inProgress)
        return;

    inProgress = true;
    reset();
    run();
    inProgress = false;
}

PostDom::PostDom(G4_Kernel& k) : kernel(k)
{
}

void vISA::PostDom::reset()
{
    postDoms.clear();
    immPostDoms.clear();

    setStale();
}

void PostDom::run()
{
    exitBB = nullptr;
    auto numBBs = kernel.fg.size();
    postDoms.resize(numBBs);
    immPostDoms.resize(numBBs);

    for (auto bb_rit = kernel.fg.rbegin(); bb_rit != kernel.fg.rend(); bb_rit++)
    {
        auto bb = *bb_rit;
        if (bb->size() > 0)
        {
            auto lastInst = bb->back();
            if (lastInst->isEOT())
            {
                exitBB = bb;
                break;
            }
        }
    }

    MUST_BE_TRUE(exitBB != nullptr, "Exit BB not found!");

    postDoms[exitBB->getId()] = { exitBB };
    std::unordered_set<G4_BB*> allBBs(kernel.fg.cbegin(), kernel.fg.cend());

    for (auto bb : kernel.fg)
    {
        if (bb != exitBB)
        {
            postDoms[bb->getId()] = allBBs;
        }
    }

    // Actual post dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto bb : kernel.fg)
        {
            if (bb == exitBB)
                continue;

            std::unordered_set<G4_BB*> tmp = { bb };
            // Compute intersection of pdom of successors
            std::unordered_map<G4_BB*, unsigned> numInstances;
            for (auto succs : bb->Succs)
            {
                auto& pdomSucc = postDoms[succs->getId()];
                for (auto pdomSuccBB : pdomSucc)
                {
                    auto it = numInstances.find(pdomSuccBB);
                    if (it == numInstances.end())
                        numInstances.insert(std::make_pair(pdomSuccBB, 1));
                    else
                        it->second = it->second + 1;
                }
            }

            // Common BBs appear in numInstances map with second value == bb->Succs count
            for (auto commonBBs : numInstances)
            {
                if (commonBBs.second == bb->Succs.size())
                    tmp.insert(commonBBs.first);
            }

            // Check if postDom set changed for bb in current iter
            if (tmp.size() != postDoms[bb->getId()].size())
            {
                postDoms[bb->getId()] = tmp;
                change = true;
                continue;
            }
            else
            {
                auto& pdomBB = postDoms[bb->getId()];
                for (auto tmpBB : tmp)
                {
                    if (pdomBB.find(tmpBB) == pdomBB.end())
                    {
                        postDoms[bb->getId()] = tmp;
                        change = true;
                        break;
                    }
                    if (change)
                        break;
                }
            }
        }
    }

    setValid();
    updateImmPostDom();
}

std::unordered_set<G4_BB*>& PostDom::getPostDom(G4_BB* bb)
{
    recomputeIfStale();

    return postDoms[bb->getId()];
}

void PostDom::dumpImmDom()
{
    recomputeIfStale();

    for (auto bb : kernel.fg)
    {
        printf("BB%d - ", bb->getId());
        auto& pdomBBs = immPostDoms[bb->getId()];
        for (auto pdomBB : pdomBBs)
        {
            printf("BB%d", pdomBB->getId());
            if (pdomBB->getLabel())
            {
                printf(" (%s)", pdomBB->getLabel()->getLabel());
            }
            printf(", ");
        }
        printf("\n");
    }
}

std::vector<G4_BB*>& PostDom::getImmPostDom(G4_BB* bb)
{
    recomputeIfStale();

    return immPostDoms[bb->getId()];
}

void PostDom::updateImmPostDom()
{
    // Update immPostDom vector with correct ordering
    for (auto bb : kernel.fg)
    {
        auto& postDomBBs = postDoms[bb->getId()];
        auto& immPostDomBB = immPostDoms[bb->getId()];
        immPostDomBB.resize(postDomBBs.size());
        immPostDomBB[0] = bb;

        for (auto pdomBB : postDomBBs)
        {
            if (pdomBB == bb)
                continue;

            immPostDomBB[postDomBBs.size() - postDoms[pdomBB->getId()].size()] = pdomBB;
        }
    }
}

G4_BB* PostDom::getCommonImmDom(std::unordered_set<G4_BB*>& bbs)
{
    recomputeIfStale();

    if (bbs.size() == 0)
        return nullptr;

    unsigned maxId = (*bbs.begin())->getId();

    auto commonImmDoms = getImmPostDom(*bbs.begin());
    for (auto bb : bbs)
    {
        if (bb->getId() > maxId)
            maxId = bb->getId();

        auto& postDomBB = postDoms[bb->getId()];
        for (unsigned i = 0, size = commonImmDoms.size(); i != size; i++)
        {
            if (commonImmDoms[i])
            {
                if (postDomBB.find(commonImmDoms[i]) == postDomBB.end())
                {
                    commonImmDoms[i] = nullptr;
                }
            }
        }
    }

    // Return first imm dom that is not a BB from bbs set
    for (G4_BB* commonImmDom : commonImmDoms)
    {
        if (commonImmDom &&
            // Common imm pdom must be lexically last BB
            commonImmDom->getId() >= maxId &&
            ((commonImmDom->size() > 1 && commonImmDom->front()->isLabel()) ||
                (commonImmDom->size() > 0 && !commonImmDom->front()->isLabel())))
        {
            return commonImmDom;
        }
    }

    return exitBB;
}

/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LoopAnalysis.h"
#include "G4_Kernel.hpp"
#include "G4_BB.hpp"
#include "BitSet.h"

using namespace vISA;

G4_BB* ImmDominator::InterSect(G4_BB* bb, int i, int k)
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
void ImmDominator::runIDOM()
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

    auto getPostOrder = [](G4_BB *S, std::vector<G4_BB *> &PO) {
      std::stack<std::pair<G4_BB *, BB_LIST_ITER>> Stack;
      std::set<G4_BB *> Visited;

      Stack.push({S, S->Succs.begin()});
      Visited.insert(S);
      while (!Stack.empty()) {
        G4_BB *Curr = Stack.top().first;
        BB_LIST_ITER It = Stack.top().second;

        if (It != Curr->Succs.end()) {
          G4_BB *Child = *Stack.top().second++;
          if (Visited.insert(Child).second) {
            Stack.push({Child, Child->Succs.begin()});
          }
          continue;
        }
        PO.push_back(Curr);
        Stack.pop();
      }
    };

    entryBB = kernel.fg.getEntryBB();
    iDoms[entryBB->getId()] = { entryBB };

    std::vector<G4_BB *> PO;
    getPostOrder(entryBB, PO);

    // Actual dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto I = PO.rbegin(), E = PO.rend(); I != E; ++I) {
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

void ImmDominator::reset()
{
    iDoms.clear();
    immDoms.clear();

    setStale();
}

void ImmDominator::run()
{
    runIDOM();
    setValid();
}

bool ImmDominator::dominates(G4_BB *bb1, G4_BB *bb2)
{
    recomputeIfStale();

    // A block always dominates itself.
    if (bb1 == bb2)
        return true;

    // If either of them is the root (entry) block, bb1 dominates bb2
    // if and only if bb1 is that root block.
    auto Root = kernel.fg.getEntryBB();
    if (bb1 == Root || bb2 == Root)
        return (bb1 == Root);

    // Track back from bb2 if neither of them is the root block.
    auto IDoms = kernel.fg.getImmDominator().getIDoms();
    G4_BB *idom = bb2;
    do {
        idom = IDoms[idom->getId()];
        // bb1 dominates bb2 if it's one of dominators of bb2.
        if (idom == bb1)
            return true;
    } while (idom != Root);

    return false;
}

void ImmDominator::dump(std::ostream& os)
{
    if (isStale())
        os << "Imm dominator data is stale.\n";

    os << "\n\nImm dom:\n";
    dumpImmDom(os);
}

const std::vector<G4_BB*>& ImmDominator::getIDoms()
{
    recomputeIfStale();

    return iDoms;
}

void ImmDominator::dumpImmDom(std::ostream& os)
{
    for (auto bb : kernel.fg)
    {
        os << "BB" << bb->getId() << " - ";
        auto& domBBs = immDoms[bb->getId()];
        for (auto domBB : domBBs)
        {
            os << "BB" << domBB->getId();
            if (domBB->getLabel())
            {
                os << " (" << domBB->getLabel()->getLabel() << ")";
            }
            os << ", ";
        }
        os << "\n";
    }
}

void Analysis::recomputeIfStale()
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

void PostDom::reset()
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

void PostDom::dumpImmDom(std::ostream& os)
{
    if (isStale())
        os << "PostDom data is stale.\n";

    for (auto bb : kernel.fg)
    {
        os << "BB" << bb->getId();
        auto& pdomBBs = immPostDoms[bb->getId()];
        for (auto pdomBB : pdomBBs)
        {
            os << "BB" << pdomBB->getId();
            if (pdomBB->getLabel())
            {
                os << " (" << pdomBB->getLabel()->getLabel() << ")";
            }
            os << ", ";
        }
        os << "\n";
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

LoopDetection::LoopDetection(G4_Kernel& k) : kernel(k), fg(k.fg)
{
}

std::vector<Loop*> LoopDetection::getTopLoops()
{
    recomputeIfStale();

    return topLoops;
}

Loop* Loop::getInnerMostLoop(const G4_BB* bb)
{
    // if current loop contains bb, recurse loop tree and return
    // most nested loop containing it.
    // if current loop doesnt contain bb then return nullptr.
    if (!contains(bb))
        return nullptr;

    for (auto& nested : immNested)
        if(auto innerMost = nested->getInnerMostLoop(bb))
            return innerMost;

    return this;
}

std::vector<G4_BB*>& Loop::getLoopExits()
{
    // already computed before, so return old list
    if (loopExits.size() > 0)
        return loopExits;

    // list all successors of loop BBs that are themselves not part of loop, ie loop exits.
    // this loop may add duplicate entries to exits. those are cleaned up later.
    std::list<G4_BB*> exits;
    for (auto bb : BBs)
    {
        for (auto succ : bb->Succs)
        {
            if (contains(succ))
                continue;
            exits.push_back(succ);
        }
    }

    // sort exits found by bbid
    exits.sort([](G4_BB* bb1, G4_BB* bb2) { return bb1->getId() < bb2->getId(); });
    // remove duplicates
    exits.unique();
    // transfer data to class member for future invocations
    std::for_each(exits.begin(), exits.end(), [&](G4_BB* bb) { loopExits.push_back(bb); });

    return loopExits;
}

G4_BB* LoopDetection::getPreheader(Loop* loop)
{
    if (loop->preHeader)
        return loop->preHeader;

    // return pre-header if one exists, otherwise create a new one and return it
    auto header = loop->getHeader();
    auto headerPreds = header->Preds;

    // for a BB to be valid pre-header, it needs to fulfill following criteria:
    // 1. BB should be a predecessor of loop header,
    // 2. BB should dominate header
    // 3. BB's only successor should be loop header
    // 4. Loop header should've no other predecessor outside the loop

    G4_BB* enteringNode = nullptr;
    bool found = false;
    for (auto pred : headerPreds)
    {
        if (loop->contains(pred))
            continue;

        if (!enteringNode)
        {
            enteringNode = pred;
            found = true;
        }
        else
        {
            found = false;
            break;
        }

        if (!enteringNode->dominates(header))
        {
            found = false;
            break;
        }

        if (enteringNode->Succs.size() > 1)
        {
            found = false;
            break;
        }
    }

    if (found && enteringNode)
    {
        // entering node is legal preheader for loop
        loop->preHeader = enteringNode;
        return enteringNode;
    }

    // a valid pre-header wasnt found, so create one and return it
    // unless any pred uses SIMD CF goto in to loop header
    if (header->getLabel())
    {
        auto headerLblStr = std::string(header->getLabel()->getLabel());
        for (auto pred : headerPreds)
        {
            if (pred->isEndWithGoto())
            {
                auto gotoInst = pred->back()->asCFInst();
                auto jipStr = std::string(gotoInst->getJipLabelStr());
                auto uipStr = std::string(gotoInst->getUipLabelStr());
                if (jipStr == headerLblStr ||
                    uipStr == headerLblStr)
                {
                    // dont create preheader for this loop as a predecessor
                    // of header has SIMD CF in to loop header.
                    return nullptr;
                }
            }
            else if (pred->size() > 0 &&
                pred->back()->opcode() == G4_jmpi)
            {
                // TODO: may be safe to alter CF by changing jmpi destination
                // to preheader instead of header.
                return nullptr;
            }
        }
    }

    G4_BB* preHeader = kernel.fg.createNewBBWithLabel("preHeader");
    for (auto pred : headerPreds)
    {
        if (loop->contains(pred))
            continue;

        kernel.fg.removePredSuccEdges(pred, header);
        kernel.fg.addPredSuccEdges(pred, preHeader);
    }
    kernel.fg.addPredSuccEdges(preHeader, header);

    for (auto bbIt = kernel.fg.begin(); bbIt != kernel.fg.end(); ++bbIt)
    {
        if (*bbIt == header)
        {
            kernel.fg.insert(bbIt, preHeader);
            break;
        }
    }

    loop->preHeader = preHeader;
    if (loop->parent)
        loop->parent->addBBToLoopHierarchy(preHeader);

    // adding/deleted CFG edges causes loop information to become
    // stale. we fix this by inserting preheader BB to all
    // parent loops. and then we set valid flag so no recomputation
    // is needed.
    setValid();

    return preHeader;
}

Loop* LoopDetection::getInnerMostLoop(const G4_BB* bb)
{
    recomputeIfStale();

    for (auto& loop : topLoops)
        if (auto innerMost = loop->getInnerMostLoop(bb))
            return innerMost;

    return nullptr;
}

void LoopDetection::computePreheaders()
{
    recomputeIfStale();

    for (auto& loop : allLoops)
    {
        (void)getPreheader(&loop);
    }
}

void LoopDetection::reset()
{
    allLoops.clear();
    topLoops.clear();

    PreIdRPostId.clear();

    setStale();
}

// Adapted from FlowGraph::DFSTraverse.
// No changes are made to any G4_BB member or to FlowGraph.
void LoopDetection::DFSTraverse(const G4_BB* startBB, unsigned& preId, unsigned& postId, BackEdges& bes)
{
    std::stack<const G4_BB*> traversalStack;
    traversalStack.push(startBB);

    auto getPreId = [&](const G4_BB* bb)
    {
        return PreIdRPostId[bb].first;
    };

    auto getRPostId = [&](const G4_BB* bb)
    {
        return PreIdRPostId[bb].second;
    };

    auto setPreId = [&](const G4_BB* bb, unsigned int id)
    {
        PreIdRPostId[bb].first = id;
    };

    auto setRPostId = [&](const G4_BB* bb, unsigned int id)
    {
        PreIdRPostId[bb].second = id;
    };

    while (!traversalStack.empty())
    {
        auto bb = traversalStack.top();
        if (getPreId(bb) != UINT_MAX)
        {
            // Pre-processed already and continue to the next one.
            // Before doing so, set postId if not set before.
            traversalStack.pop();
            if (getRPostId(bb) == UINT_MAX)
            {
            // All bb's succ has been visited (PreId is set) at this time.
            // if any of its succ has not been finished (RPostId not set),
            // bb->succ forms a backedge.
            //
            // Note: originally, CALL and EXIT will not check back-edges, here
            //       we skip checking for them as well. (INIT & RETURN should
            //       be checked as well ?)
            if (!(bb->getBBType() & (G4_BB_CALL_TYPE | G4_BB_EXIT_TYPE)))
            {
                for (auto succBB : bb->Succs)
                {
                    if (getRPostId(succBB) == UINT_MAX)
                    {
                        BackEdge be = std::make_pair(const_cast<G4_BB*>(bb), succBB);
                        bes.push_back(be);
                    }
                }
            }

            // Need to keep this after backedge checking so that self-backedge
            // (single-bb loop) will not be missed.
            setRPostId(bb, postId++);
            }
            continue;
        }

        setPreId(bb, preId++);

        if (bb->getBBType() & G4_BB_CALL_TYPE)
        {
            const G4_BB* returnBB = bb->BBAfterCall();

            if (getPreId(returnBB) == UINT_MAX)
            {
                traversalStack.push(returnBB);
            }
            else
            {
                MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
            }
        }
        else if (bb->getBBType() & G4_BB_EXIT_TYPE)
        {
            // Skip
        }
        else
        {
            // To be consistent with previous behavior, use reverse_iter.
            auto RIE = bb->Succs.rend();
            for (auto rit = bb->Succs.rbegin(); rit != RIE; ++rit)
            {
                const G4_BB* succBB = *rit;
                if (getPreId(succBB) == UINT_MAX)
                {
                    traversalStack.push(succBB);
                }
            }
        }
    }
}

void LoopDetection::findDominatingBackEdges(BackEdges& bes)
{
    const auto& BBs = fg.getBBList();

    for (auto& bb : BBs)
    {
        PreIdRPostId[bb] = std::make_pair(UINT_MAX, UINT_MAX);
    }

    unsigned preId = 0;
    unsigned postID = 0;

    DFSTraverse(fg.getEntryBB(), preId, postID, bes);

    for (auto fn : fg.funcInfoTable)
    {
        DFSTraverse(fn->getInitBB(), preId, postID, bes);
    }
}

void LoopDetection::populateLoop(BackEdge& backEdge)
{
    // check whether dst dominates src
    auto src = const_cast<G4_BB*>(backEdge.first);
    auto dst = const_cast<G4_BB*>(backEdge.second);

    if (fg.getImmDominator().dominates(dst, src))
    {
        // this is a natural loop back edge. populate all bbs in loop.
        Loop newLoop(backEdge);
        newLoop.id = allLoops.size() + 1;

        newLoop.addBBToLoop(src);
        newLoop.addBBToLoop(dst);

        std::stack<G4_BB*> traversal;
        traversal.push(src);
        while (!traversal.empty())
        {
            auto bb = traversal.top();
            traversal.pop();

            // check whether bb's preds are dominated by loop header.
            // if yes, add them to traversal.
            for (auto predIt = bb->Preds.begin(); predIt != bb->Preds.end(); ++predIt)
            {
                auto pred = (*predIt);
                // pred is loop header, which is already added to list of loop BBs
                if (pred == dst)
                    continue;

                if (dst->dominates(pred) &&
                    !newLoop.contains(pred))
                {
                    // pred is part of loop
                    newLoop.addBBToLoop(pred);
                    traversal.push(pred);
                }
            }
        }

        (void)newLoop.getLoopExits();
        allLoops.emplace_back(newLoop);
    }
}

void LoopDetection::computeLoopTree()
{
    // create loop tree by iterating over allLoops in descending order
    // of BB count.
    std::vector<Loop*> sortedLoops;
    std::for_each(allLoops.begin(), allLoops.end(), [&](Loop& l) { sortedLoops.push_back(&l); });

    // sorting loops by size of contained BBs makes it easy to create
    // tree structure relationship of loops.
    // 1. If loop A has more BBs than loop B then A is either some parent of B or no relationship exists.
    // 2. For loop A to be a parent of loop B, all BBs of loop B have to be contained in loop A as well.
    //
    // processing loops in sorted order of BB size guarantees that we'll create tree in top-down order.
    // we'll never encounter a situation where new loop to be added to tree is parent of an existing
    // loop already present in the tree.
    std::sort(sortedLoops.begin(), sortedLoops.end(), [](Loop* l1, Loop* l2) { return l2->getBBSize() < l1->getBBSize(); });

    for (auto loop : sortedLoops)
    {
        addLoop(loop, nullptr);
    }
}

void LoopDetection::addLoop(Loop* newLoop, Loop* aParent)
{
    if (topLoops.size() == 0)
    {
        topLoops.push_back(newLoop);
        return;
    }

    // find a place in existing loop tree to insert new loop passed in.
    // following scenarios exist:
    // a. loop is nested loop of an existing loop,
    // b. loop is not nested but is sibling of existing loop,
    // c. loop is top level parent loop of a certain tree

    // check if newLoop fits in to any existing current top level loop
    auto siblings = aParent ? aParent->getAllSiblings(topLoops) : topLoops;
    for (auto& sibling : siblings)
    {
        if (newLoop->fullSubset(sibling))
        {
            if (sibling->immNested.size() > 0)
            {
                addLoop(newLoop, sibling->immNested[0]);
            }
            else
            {
                sibling->immNested.push_back(newLoop);
                newLoop->parent = sibling;
            }
            return;
        }
        else if (newLoop->fullSuperset(sibling))
        {
            MUST_BE_TRUE(false, "Not expecting to see parent loop here");
            return;
        }
    }

    // add new sibling to current level
    newLoop->parent = siblings[0]->parent;
    if (newLoop->parent)
    {
        siblings[0]->parent->immNested.push_back(newLoop);
    }
    else
    {
        topLoops.push_back(newLoop);
    }
}

void LoopDetection::run()
{
    BackEdges backEdges;
    findDominatingBackEdges(backEdges);
    for (auto& be : backEdges)
    {
        populateLoop(be);
    }

    computeLoopTree();

    setValid();
}

void LoopDetection::dump(std::ostream& os)
{
    if(isStale())
        os << "Loop info is stale.\n";

    os << "\n\n\nLoop tree:\n";

    for (auto loop : topLoops)
    {
        loop->dump(os);
    }
}

// add bb to current loop and to all valid parents
void Loop::addBBToLoopHierarchy(G4_BB* bb)
{
    addBBToLoop(bb);

    if (parent)
        parent->addBBToLoopHierarchy(bb);
}

void vISA::Loop::addBBToLoop(G4_BB* bb)
{
    if (!contains(bb))
    {
        BBs.push_back(bb);
        BBsLookup.insert(bb);
    }
}

bool Loop::fullSubset(Loop* other)
{
    if (BBs.size() > other->BBs.size())
        return false;

    // to avoid O(N^2) lookup, use unordered set of other loop's BBs for lookup
    auto& otherBBs = other->BBsLookup;

    // check whether current loop's all BBs are fully contained in "other" loop
    for (auto bb : BBs)
    {
        if (otherBBs.find(bb) == otherBBs.end())
            return false;
    }

    return true;
}

bool Loop::fullSuperset(Loop* other)
{
    return other->fullSubset(this);
}

std::vector<Loop*> Loop::getAllSiblings(std::vector<Loop*>& topLoops)
{
    if (parent)
        return parent->immNested;

    return topLoops;
}

unsigned int Loop::getNestingLevel()
{
    if (!parent)
        return 1;

    return parent->getNestingLevel()+1;
}

void Loop::dump(std::ostream& os)
{
    auto nestingLevel = getNestingLevel();
    nestingLevel = nestingLevel > 0 ? nestingLevel : 1;
    for (unsigned int i = 0; i != nestingLevel - 1; ++i)
    {
        os << "\t";
    }
    os << "L" << id << ": - { ";
    for (auto bb : BBs)
    {
        os << bb->getId();
        if (bb != BBs.back())
            os << ", ";
    }
    os << " } ";

    auto labelStr = std::string("BB") + (preHeader ? std::to_string(preHeader->getId()) :
        std::string("--"));

    if (preHeader && preHeader->getLabel())
        labelStr += "(" + std::string(preHeader->getLabel()->getLabel()) + ")";

    std::string exitBBs = "{ ";
    for (auto bb : loopExits)
    {
        exitBBs += "BB" + std::to_string(bb->getId());
        if (bb != loopExits.back())
            exitBBs += ", ";
    }
    exitBBs += " }";

    os << " BE: {BB" << be.first->getId() << " -> BB" << be.second->getId() << "}, " <<
        "PreHeader: " << labelStr << ", " << "Loop exits: " << exitBBs << std::endl;

    for (auto& nested : immNested)
    {
        nested->dump(os);
    }
}

bool Loop::contains(const G4_BB* bb)
{
    return BBsLookup.find(bb) != BBsLookup.end();
}

bool VarReferences::isUniqueDef(G4_Operand* dst)
{
    recomputeIfStale();

    auto dcl = dst->getTopDcl();

    // return true if spilled variable has a single static def
    // and it is not live-in to current bb (eg, loop, sub).
    if (getDefCount(dcl) != 1)
    {
        // check whether multiple defs exist in program for current
        // lb, rb
        auto lb = dst->getLeftBound();
        auto rb = dst->getRightBound();

        unsigned int count = 0;
        const auto& defs = getDefs(dcl);
        for (auto& def : *defs)
        {
            if (std::get<2>(def) <= rb &&
                std::get<3>(def) >= lb)
                ++count;
        }

        if (count > 1)
            return false;
    }

    return true;
}

unsigned int VarReferences::getDefCount(G4_Declare* dcl)
{
    auto defs = getDefs(dcl);
    if (defs)
        return defs->size();
    return 0;
}

unsigned int VarReferences::getUseCount(G4_Declare* dcl)
{
    auto uses = getUses(dcl);
    if (uses)
        return uses->size();
    return 0;
}

const VarReferences::Defs* VarReferences::getDefs(G4_Declare* dcl)
{
    recomputeIfStale();

    auto it = VarRefs.find(dcl);
    if (it != VarRefs.end())
        return &(it->second.first);

    return nullptr;
}

const VarReferences::Uses* VarReferences::getUses(G4_Declare* dcl)
{
    recomputeIfStale();

    auto it = VarRefs.find(dcl);
    if (it != VarRefs.end())
        return &(it->second.second);

    return nullptr;
}

void VarReferences::reset()
{
    VarRefs.clear();

    setStale();
}

void VarReferences::run()
{
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            if (inst->isPseudoKill())
                continue;

            auto dst = inst->getDst();

            if (dst && !dst->isNullReg())
            {
                auto topdcl = dst->getTopDcl();

                if (topdcl)
                {
                    auto lb = dst->getLeftBound();
                    auto rb = dst->getRightBound();
                    auto& Defs = VarRefs[topdcl].first;
                    Defs.push_back(std::make_tuple(inst, bb, lb, rb));
                }
            }

            auto condMod = inst->getCondMod();
            if (condMod)
            {
                auto topdcl = condMod->getTopDcl();
                if (topdcl)
                {
                    auto lb = condMod->getLeftBound();
                    auto rb = condMod->getRightBound();
                    auto& Defs = VarRefs[topdcl].first;
                    Defs.push_back(std::make_tuple(inst, bb, lb, rb));
                }
            }

            for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
            {
                auto src = inst->getSrc(i);
                if (src && src->isSrcRegRegion())
                {
                    auto topdcl = src->asSrcRegRegion()->getTopDcl();
                    if (topdcl)
                    {
                        auto& Uses = VarRefs[topdcl].second;
                        Uses.push_back(std::make_tuple(inst, bb));
                    }
                }
            }

            auto pred = inst->getPredicate();
            if (pred)
            {
                auto topdcl = pred->getTopDcl();
                if (topdcl)
                {
                    auto& Uses = VarRefs[topdcl].second;
                    Uses.push_back(std::make_tuple(inst, bb));
                }
            }
        }
    }

    setValid();
}

void VarReferences::dump(std::ostream& os)
{
    if (isStale())
        os << "Data is stale.\n";

    os << "#Dcls with defs/uses: " << VarRefs.size();
}

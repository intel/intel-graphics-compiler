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


/*******************************************************
 * This performs a basic liveness/DU analysis (reverse) of the register files
 * (GRF and ARF).  The RegSet abstraction tracks bytes and words touched
 * by instructions.
 *
 * We choose a reverse (backwards) analysis because we want the algorithm to
 * infer kernel inputs (or use of uninitialized variables).
 *    ==> Hence, read examples bottom up.
 *
 *******************************************************
 * EXAMPLE SYNTAX and NOMANCLATURE:
 *   x <- 5 * a     #3          |    {#3/a}          subtract w; introduce a
 *  ^ inst syntax   ^ inst ID        ^ set of paths  ^ description
 *                                     {#inst/reg.}
 *  x[i] <- 5  indicates element i of x written
 *  x    <- 5  indicates all elements of x written (vector write)
 *
 *******************************************************
 *
 * EXAMPLE:  SIMPLE
 *                              {#5/b,#3/a}       FINAL LIVE-IN
 *   x <- 5 * a  #3        |    {#5/b,#3/a}       subtract x; introduce a
 *   ...                   ...
 *   z <- 2 + x  #4        |    {#5/b,#5/x,#4/x}  introduce #4/w
 *   w <- b * x  #5        |    {#5/b,#5/x}       introduce uses of b and x
 *
 * EXAMPLE: SPLIT USE (consumer uses multiple producers)
 *                              {#1/b}            FINAL LIVE-IN
 *   a <- 5 * b     #1        | {#1/b}            kills both #4 and #5 since
 *                                                write set is superset of both
 *                                                but introduces #b
 *   ...                   ...
 *   w <- 2 * a[0]  #4        | {#5/a[1],#4/a[0]} introduce #4/a[0]
 *   w <- 2 * a[1]  #5        | {#5/a[1]}         introduce #5/a[1]
 *
 * EXAMPLE: SPLIT DEFINITION (consumer uses multiple producers)
 *                              {#5/x[0]}     FINAL LIVE-IN
 *   x[1] <- 6   #2        |    {#5/x[0]}     splits path here (x[0] is an input)
 *   ...                   ...
 *   w <- 2 * x  #5        |    {#5/x}        introduce #5/x
 *
 * EXAMPLE: PREDICATION UNSOLVED 1 (due to missing pred half)
 *          mov  r0  // #0
 *   (f0.0) add  r0  // #1
 *          mul  r0  // #2: depends on #0 and #1
 *
 ///////////////////////////////////////////////
 // TODO: IMPLEMENT THESE CASES
 ///////////////////////////////////////////////
 * EXAMPLE: PREDICATION SOLVED
 *           mov  r0   1    // #0
 *   (f0.0)  add  r0 ...    // #1
 *   (~f0.0) add  r0 ... r0 // #2: depends on #0 and #1 (same as prev case)
 *           mul  ... r0    // #3: deps on #1 and #2; #0 definitely killed
 *
 * EXAMPLE: PREDICATION UNSOLVED 2 (due to flag redef)
 *           mov  r0   1      // #0
 *   (f0.0)  add  r0 ...      // #1
 *           cmp (le)f0.0 ... // #2  f0.0 clobbered!
 *   (~f0.0) add  r0 ...      // #3: deps. on #0 and #1
 *           mul  ... r0      // #4: deps. on #0, #1, and #2
 *
 * EXAMPLE: PREDICATION UNSOLVED 2 (due to crossing BB)
 *           mov  r0   1      // #0
 *   (f0.0)  add  r0 ...      // #1
 * LABEL:
 *   (~f0.0) add  r0 ...      // #2: ~f0.0 could be defined elsewhere
 *           mul  ... r0      // #3: deps. on #0, #1, and #2
 *
 *******************************************************
 * IMPLEMENTATION NOTES:
 *
 *  - We actually consider a write to a register a "use", just a WAW
 *    dependency (this is not illustrated in the examples above).
 *
 *  - The first passes just compute LIVE-IN for all blocks until those
 *    sets reach a fixed point.  Finally, we walk through each block
 *    and complete each path per-instruction.
 *
 * The following implementation is an iterative data flow analyis.
 * ... more details ... bottom up ..., the joining function is ...
 *
 */
#include "DUAnalysis.hpp"

#include <algorithm>
#include <cstdio>
#include <list>
#include <map>
#include <sstream>
#include <string>

using namespace iga;

#define TRACE(...)
// #define TRACE(...) printf(__VA_ARGS__)

// The map key is the instruction and a use type (READ or WRITE)
typedef std::pair<Dep::Type,Instruction*> DepKey;
typedef std::map<DepKey,Dep>              LiveDepMap;

static void EmitPaths(const LiveDepMap &defs)
{
    for (const auto &pair : defs) {
        const Dep &d = pair.second;
        if (d.use == nullptr)
            return;
        auto str = d.live.str();
        TRACE("    #%s\n", str.c_str());
    }
}

// Information pertaining to a given block.  We convert this to
// a public-facing BlockInfo that discards all the temporary and
// non-essential information from the algorithm.
struct BlockState
{
    Block                                     *block;
    // predecessor info (second coordinate is true if the node is *not*
    // a fallthrough (i.e. it's a jump)
    std::vector<std::pair<BlockState *,bool>>  pred;

    LiveDepMap                                 liveIn;
    LiveDepMap                                 liveOut;

    BlockState(Block *b) : block(b) { }
};


struct DepAnalysisComputer
{
    const Model                     &model;
    Kernel                          *k;
    std::vector<InstDsts>            instDsts;
    std::vector<InstSrcs>            instSrcs;

    // mid-state and then output
    std::vector<BlockState>          blockState;

    // outputs
    std::vector<Dep>           liveRangesResult;

    DepAnalysisComputer(Kernel *_k)
        : model(_k->getModel())
        , k(_k)
    {
        instSrcs.reserve(k->getInstructionCount());
        instDsts.reserve(k->getInstructionCount());

        blockState.reserve(k->getBlockList().size());
        int bIx = 0, iIx = 0;

        // pre-assign ID's we know to be valid
        TRACE("******** INITIAL CONDITIONS **********\n");
        for (Block *b : k->getBlockList()) {
            blockState.emplace_back(b);
            b->setID(bIx++);
            TRACE("  BLOCK #%d\n", b->getID());

            for (Instruction *i : b->getInstList()) {
                i->setID(iIx++);

                instSrcs.emplace_back(InstSrcs::compute(*i));
                instDsts.emplace_back(InstDsts::compute(*i));

                TRACE("    DEF #%03d |  %-96s ||  %s <== %s\n",
                    i->getID(),
                    i->str(model.platform).c_str(),
                    instDsts.back().str().c_str(),
                    instSrcs.back().str().c_str());
            }
        }

        // pre-calculate predecessor blocks
        for (size_t i = 0; i < blockState.size(); i++) {
            BlockState &b = blockState[i];

            const auto &il = b.block->getInstList();
            if (i < blockState.size() - 1) {
                // not the last => potentially link the next block back to us
                if (il.empty()) {
                    // empty block fallthrough
                    // E.g.
                    //
                    //  ...
                    // FOO: // empty block, but valid branch target
                    // BAR:
                    //  ...
                    blockState[i + 1].pred.emplace_back(&b, false);
                } else {
                    // at least one instruction in this block

                    // unpredicated JMPI and an EOT are the only op that
                    // stop the block hard
                    const Instruction *iTerm = il.back();
                    bool unconditionalJmp =
                        iTerm->getOp() != Op::JMPI || iTerm->hasPredication();
                    bool eotTerm =!iTerm->hasInstOpt(InstOpt::EOT);
                    if (!unconditionalJmp && !eotTerm) {
                        // normal fallthrough
                        blockState[i + 1].pred.emplace_back(&b, false);
                    }
                    // link any jump target blocks back to us
                    for (size_t srcIx = 0; srcIx < iTerm->getSourceCount(); srcIx++) {
                        const Operand &src = iTerm->getSource(srcIx);
                        if (src.getKind() == Operand::Kind::LABEL) {
                            BlockState &tb =
                                blockState[src.getTargetBlock()->getID()];
                            tb.pred.emplace_back(&b, true);
                        }
                    }
                }
            }

        }
    }

    void runAnalysis() {
        computeLiveInPaths();
        completePaths();
    }


    void computeLiveInPaths() {
        bool changed;
        int itr = 0;
        do {
            TRACE("******* STARTING LIVE-IN ITERATION %d\n", itr);
            changed = false;
            for (BlockState &bs : blockState) {
                TRACE("  *** BLOCK %d with ...\n", bs.block->getID());
                EmitPaths(bs.liveIn);
                changed |= iterateBlockLiveIn(bs, false);
            }
            itr++;
            TRACE("\n");
        } while (changed);
    }
    void completePaths() {
        // copy out the data
        for (BlockState &b : blockState) {
            iterateBlockLiveIn(b, true);
        }
    }

    bool iterateBlockLiveIn(
        BlockState &b,
        bool copyOut)
    {
        // all the live paths at the end of this block
        LiveDepMap rLiveDefs = b.liveOut;

        // FOR each instruction i
        //   extend paths backwards
        //   remove any paths killed off by a definition
        //   start any new paths induced by i's uses
        auto &il = b.block->getInstList();
        for (InstList::reverse_iterator
            iItr = il.rbegin(),
            iItrEnd = il.rend();
            iItr != iItrEnd;)
        {
            Instruction *i = *iItr;
            TRACE("      *** INSTRUCTION: %-96s (def #%d)\n",
                i->str(model.platform).c_str(),
                i->getID());

            // extend live ranges
            LiveDepMap::iterator
                dItr = rLiveDefs.begin(),
                dEnd = rLiveDefs.end();
            while (dItr != dEnd) {
                dItr = extenedDepBackwards(i, rLiveDefs, dItr, copyOut);
            } // live ranges while

            // any use of a variable starts a new live range
            startNewDepsBackwards(i, rLiveDefs);
            iItr++;
        }

        bool bLiveInChanged = updateLiveDefs(b.liveIn, rLiveDefs);
        bool changedAnyPred = false;
        if (bLiveInChanged) {
            // push information back to predecessor nodes
            for (auto &predEdge : b.pred) {
                BlockState *bPred = (BlockState *)predEdge.first;
                changedAnyPred |= joinBlocks(
                    bPred->liveOut,
                    b.liveIn,
                    predEdge.second);
            }
        }

        return changedAnyPred;
    }

    LiveDepMap::iterator extenedDepBackwards(
        Instruction *i,
        LiveDepMap &rLiveDefs,
        LiveDepMap::iterator &dItr,
        bool copyOut)
    {
        const InstDsts &iOups = instDsts[i->getID()];
        Dep &d = dItr->second;

        RegSet overlap;
        d.live.intersectInto(iOups.unionOf(), overlap);
        if (copyOut && !overlap.empty()) {
            liveRangesResult.push_back(d);
            Dep &d = liveRangesResult.back();
            d.def = i;
            d.live = overlap;
        }

        if (!i->hasPredication()) {
            // don't subtract if the instruction is predicated
            // since there could be another definition above this
            d.live.destructiveSubtract(overlap);
        }

        // lr.live.destructiveSubtract(iOups.destinations);
        // lr.live.destructiveSubtract(iOups.flagModifier);
        if (d.live.empty()) {
            TRACE("        %s: deleting range\n", d.str().c_str());
            dItr = rLiveDefs.erase(dItr);
        } else {
            if (i->getOpSpec().isFixedLatency()) {
                d.minInOrderDist++;
            }
            TRACE("        %s: extending range\n", d.str().c_str());
            dItr++;
        }
        return dItr;
    }


    void startNewDepsBackwards(
        Instruction *i,
        LiveDepMap &rLiveDefs)
    {
        const InstDsts &iOups = instDsts[i->getID()];
        const InstSrcs &iInps = instSrcs[i->getID()];
        startNewDepsBackwardsWithSets(
            i,
            rLiveDefs,
            Dep::READ,
            &iInps.predication,
            &iInps.sources);
        startNewDepsBackwardsWithSets(
            i,
            rLiveDefs,
            Dep::WRITE,
            &iOups.flagModifier,
            &iOups.destinations);
    }
    void startNewDepsBackwardsWithSets(
        Instruction *i,
        LiveDepMap &rLiveDefs,
        Dep::Type type,
        const RegSet *rs1,
        const RegSet *rs2)
    {
        // early out (no dependencies on this instruction)
        if ((rs1==nullptr || rs1->empty()) &&
            (rs2==nullptr || rs2->empty()))
        {
            return;
        }

        Dep *d;
        const auto &itr = rLiveDefs.find(DepKey(type,i));
        if (itr != rLiveDefs.end()) {
            d = &itr->second;
        } else {
            auto val = rLiveDefs.emplace(DepKey(type,i),Dep(type,i));
            d = &(*val.first).second;
            d->minInOrderDist = 1;
            d->crossesBranch = false;
        }

        // clobber the old value.
        d->live.reset();
        if (rs1) {
            d->live.destructiveUnion(*rs1);
        }
        if (rs2) {
            d->live.destructiveUnion(*rs2);
        }
        TRACE("        %s: spawning live range\n", d->str().c_str());
    }

    bool joinBlocks(
              LiveDepMap &predOUT, // predecessor block live OUT
        const LiveDepMap &succIN,  // successor   block live IN
        bool /* isJump */)         // as opposed to fallthrough
    {
        bool changed = false;
        for (const auto &lrElem : succIN) {
            const Dep &lrSuccIN = lrElem.second;
            LiveDepMap::iterator itr = predOUT.find(lrElem.first);
            if (itr == predOUT.end()) {
                // clean insertion
                Dep copy = lrSuccIN;
                copy.crossesBranch = true;
                predOUT[lrElem.first] = copy;
                changed = true;
            } else {
                // mutation of existing path
                Dep &lrPredOUT = itr->second;

                // update the min distance
                if (lrSuccIN.minInOrderDist < lrPredOUT.minInOrderDist) {
                    changed = true;
                    lrPredOUT.minInOrderDist = lrSuccIN.minInOrderDist;
                    lrPredOUT.crossesBranch = true;
                }

                // add any new dependencies
                changed |= lrPredOUT.live.destructiveUnion(lrSuccIN.live);
            } // end else existing path
        } // for all source paths
        return changed;
    }
    static bool updateLiveDefs(LiveDepMap &to, const LiveDepMap &from) {
        bool changed = from != to;
        if (changed)
            to = from;
        return changed;
    }

    void addDep(
        const Dep &lr,
        Instruction *use,
        Dep::Type type)
    {
        liveRangesResult.push_back(lr);
        Dep &lr1 = liveRangesResult.back();
        lr1.use = use;
        lr1.useType = type;


        std::stringstream ss;
        lr1.str(ss);
        TRACE("  => %s\n", ss.str().c_str());
    }
};

// RAR{@3, #5 <- ?, r13..r14}
void iga::Dep::str(std::ostream &os) const
{
    if (useType == Dep::READ) {
        os << "RAW"; // read after write
    } else {
        os << "WAW"; // write after read
    }

    auto emitId = [&] (const Instruction *i) {
        if (i) {
            os << "#" << i->getID();
        } else {
            os << "?";
        }
    };
    os << "{";
    os << "@" << minInOrderDist;
    os << ", ";
    emitId(use);
    os << "<=";
    emitId(def);
    os << ", ";
    live.str(os);
    os << "}";
}


std::string iga::Dep::str() const
{
    std::stringstream ss;
    str(ss);
    return ss.str();
}


bool iga::Dep::operator==(const Dep &p) const
{
    return
        def == p.def &&
        use == p.use &&
        useType == p.useType &&
        live == p.live &&
        crossesBranch == p.crossesBranch &&
        minInOrderDist == p.minInOrderDist;
}


DepAnalysis iga::ComputeDepAnalysis(Kernel *k)
{
    DepAnalysisComputer lac(k);
    lac.runAnalysis();

    DepAnalysis la;

    // copy out block information
    for (const auto &bs : lac.blockState) {
        TRACE("==== BLOCK %d LIVE-IN ====\n", bs.block->getID());
        EmitPaths(bs.liveIn);

        la.blockInfo.emplace_back(bs.block);
        BlockInfo &bi = la.blockInfo.back();
        auto addSet =
            [] (const LiveDepMap &map, std::vector<Dep> &out) {
                for (const auto &pair : map) {
                    const Dep &d = pair.second;
                    if (d.def != nullptr || d.useType != Dep::WRITE) {
                        // filter out any false WAW dependencies
                        out.push_back(d);
                    }
                }
            };
        addSet(bs.liveIn, bi.liveDefsIn);
        addSet(bs.liveOut, bi.liveDefsOut);
    }

    // copy out the per-instruction live sets
    for (const Dep &d : lac.liveRangesResult) {
        if (d.def != nullptr || d.useType != Dep::WRITE) {
            la.deps.push_back(d);
        }
    }

    TRACE("=========== END ===========\n");
    return la;
}

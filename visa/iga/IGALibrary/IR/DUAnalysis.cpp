/*========================== begin_copyright_notice ============================

Copyright (c) 2017-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/


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
 * EXAMPLE: SPLIT USE (mult. consumer uses different parts of a produced result)
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
 */
#include "DUAnalysis.hpp"

// #define ENABLE_TRACING

#include <algorithm>
#ifdef ENABLE_TRACING
#include <cstdio>
#include "../strings.hpp"
#endif
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace iga;

#ifdef ENABLE_TRACING
#define TRACE(...) std::cout << iga::format(__VA_ARGS__)
#else
#define TRACE(...)
#endif

// The map key is the use instruction and a use type (READ or WRITE)
using DepKey = std::pair<Dep::Type,Instruction*>;
using LiveDepMap = std::map<DepKey,Dep>;


static void EmitPaths(const LiveDepMap &defs)
{
    for (const auto &pair : defs) {
        const Dep &d = pair.second;
        (void)d;
        TRACE("    #", d.str());
    }
}
static std::string FormatLiveDepMap(const LiveDepMap &ldm)
{
    std::stringstream ss;
    bool first = true;
    ss << "{";
    for (const auto &pair : ldm) {
        if (first) first = false; else ss << ",";
        const Dep &d = pair.second;
        ss << d.str();
    }
    ss << "}";
    return ss.str();
}

enum class EdgeType {FALLTHROUGH, JUMP};

// Information pertaining to a given block.  We convert this to
// a public-facing BlockInfo that discards all the temporary and
// non-essential information from the algorithm when extracting
// and returning the results.
struct BlockState
{
    Block                                     *block;

    // predecessor info (second coordinate is true if the node is *not*
    // a fallthrough (i.e. it's a jump)
    // indexed by block ID
    std::vector<std::pair<BlockState *,EdgeType>>  pred;

    LiveDepMap                                     liveIn;
    LiveDepMap                                     liveOut;

    BlockState(Block *b) : block(b) { }

    std::string str() const {
        std::stringstream ss;
        ss << "B#" << block->getID() << " pred:{";
        bool first = true;
        for (const auto &pbse : pred) {
            if (first) first = false; else ss << ",";
            ss << "B#" << pbse.first->block->getID();
        }
        ss << "}";
        return ss.str();
    }
};


struct DepAnalysisComputer
{
    const Model                     &model;
    Kernel                          *k;

    // sources and destinations indexed by instruction ID
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
        sanityCheckIR(k); // should nop in release

        instSrcs.reserve(k->getInstructionCount());
        instDsts.reserve(k->getInstructionCount());

        // we must do this so the vector doesn't resize
        blockState.reserve(k->getBlockList().size());
        int bIx = 0, iIx = 0;

        // pre-assign ID's we know to be valid and
        // precompute instruction dependencies
        for (Block *b : k->getBlockList()) {
            blockState.emplace_back(b);
            b->setID(bIx++);

            for (Instruction *i : b->getInstList()) {
                i->setID(iIx++);

                instSrcs.emplace_back(InstSrcs::compute(*i));
                instDsts.emplace_back(InstDsts::compute(*i));
            }
        }

        // pre-calculate predecessor blocks by looking at the terminator op
        for (size_t i = 0; i < blockState.size(); i++) {
            BlockState &b = blockState[i];

            const auto &il = b.block->getInstList();
            bool isLast = i == blockState.size() - 1;
            // not the last => potentially link the next block back to us
            if (il.empty()) {
                // empty block fallthrough
                // E.g.
                //
                //  ...
                // FOO: // empty block, but valid branch target
                // BAR:
                //  ...
                if (!isLast)
                    blockState[i + 1].pred.emplace_back(
                        &b, EdgeType::FALLTHROUGH);
            } else {
                // at least one instruction in this block;
                // mark 'b' as a predecessor to all targets
                //
                // unpredicated JMPI and an EOT are the only op that
                // stop the block hard
                const Instruction *iTerm = il.back();
                if (!isLast && fallthroughPossible(iTerm)) {
                    // normal fallthrough
                    blockState[i + 1].pred.emplace_back(
                        &b, EdgeType::FALLTHROUGH);
                }
                for (unsigned srcIx = 0; srcIx < iTerm->getSourceCount();
                    srcIx++)
                {
                    const Operand &src = iTerm->getSource(srcIx);
                    if (src.getKind() == Operand::Kind::LABEL) {
                        BlockState &tb =
                            blockState[src.getTargetBlock()->getID()];
                        tb.pred.emplace_back(&b, EdgeType::JUMP);
                    }
                }
            } // non-empty block 'b'
        } // end for(block state)

        TRACE("******** INITIAL CONDITIONS **********");
        for (const Block *b : k->getBlockList()) {
            TRACE("  BLOCK ", blockState[b->getID()].str());
            for (const Instruction *i : b->getInstList()) {
                (void)i;
                TRACE("    DEF #", i->getID(), " |  ",
                    trimTrailingWs(i->str()), " ||  ",
                    instDsts[i->getID()].str(), " <== ",
                        instSrcs[i->getID()].str());
            }
        }
    } // DepAnalysisComputer::DepAnalysisComputer

    static bool fallthroughPossible(const Instruction *i) {
        bool unconditionalJmp = i->getOp() == Op::JMPI && !i->hasPredication();
        bool eotTerm = i->hasInstOpt(InstOpt::EOT);
        return !unconditionalJmp && !eotTerm;
    }

    // in debug builds this ensures:
    //  1. block and instruction IDs aren't huge random numbers
    //     (since we intend to index-map information)
    //  2. ensure IDs are unique
    //  3. ensure there are branch instructions hiding in the middle of
    //     a block
    static void sanityCheckIR(const Kernel *k) {
#ifdef _DEBUG
        // NOTE: this is redundant because we preset all the ids above;
        // it does add checking for illegal mid-block branches
        std::unordered_set<int> instIds, blockIds;
        auto instCount = k->getInstructionCount();
        for (const Block *b : k->getBlockList()) {
            auto bId = b->getID();
            IGA_ASSERT(bId < 2 * (int)k->getBlockList().size(),
                "instruction ID's should be small");
            IGA_ASSERT(blockIds.find(bId) == blockIds.end(),
                "duplicate block ID");
            for (const Instruction *i : b->getInstList()) {
                auto id = i->getID();
                IGA_ASSERT(id < 2 * (int)instCount,
                    "instruction ID's should be small");
                IGA_ASSERT(instIds.find(id) == instIds.end(),
                    "duplicate instruction ID");
                instIds.emplace(i->getID());

                for (size_t sIx = 0; sIx < i->getSourceCount(); sIx++) {
                    if (i->getSource(sIx).getKind() == Operand::Kind::LABEL) {
                        IGA_ASSERT(b->getInstList().back() == i,
                            "label in the middle of a block");
                    }
                }
            }
        }
#endif // _DEBUG
    }


    void runAnalysis() {
        computeLiveInPaths();
        completePaths();
    }


    void computeLiveInPaths() {
        bool changed;
        int itr = 0;
        do {
            TRACE("******* STARTING LIVE-IN ITERATION ", itr);
            changed = false;
            // TODO: iterate these first in reverse order, then via worklist
            for (BlockState &bs : blockState) {
                TRACE("  *** B#", bs.block->getID(), " with ...");
                EmitPaths(bs.liveIn);
                changed |= recomputeBlockLiveIn(bs, false);
            }
            TRACE("******* ENDING ITERATION ", itr);
            itr++;
        } while (changed);
    }

    // Starting with liveOut, walk back through the instructions
    // extending all paths backwards.  Kill off stuff definitely defined,
    // and start new paths.  At the end, update this block's 'liveIn'
    // and push it back across to predecessor block's 'liveOut'
    bool recomputeBlockLiveIn(BlockState &b, bool copyOut)
    {
        // all the live paths at the end of this block
        LiveDepMap rLiveDefs = b.liveOut;

        // FOR each instruction i
        //   extend paths backwards
        //   remove any paths killed off by a definition
        //   start any new paths induced by i's uses
        auto &il = b.block->getInstList();
        for (InstList::reverse_iterator iItr = il.rbegin(),
            iItrEnd = il.rend();
            iItr != iItrEnd;)
        {
            Instruction *i = *iItr;
            TRACE("      *** I#", i->getID(), ": ", trimTrailingWs(i->str()));

            // extend live ranges
            LiveDepMap::iterator
                dItr = rLiveDefs.begin(),
                dEnd = rLiveDefs.end();
            while (dItr != dEnd) {
                dItr = extendDepBackwards(i, rLiveDefs, dItr, copyOut);
            }

            // any use of a variable starts a new live range
            startNewDepsBackwards(i, rLiveDefs);

            iItr++;
        }

        bool bLiveInChanged = updateLiveDefs(b.liveIn, rLiveDefs);
        bool changedAnyPred = false;
        if (bLiveInChanged) {
            // push information back to predecessor nodes
            TRACE("     propagating B#", b.block->getID(),
                " liveIn ", FormatLiveDepMap(b.liveIn), " back to pred(s)");
            for (auto &predEdge : b.pred) {
                BlockState *bPred = (BlockState *)predEdge.first;
                TRACE("       pred B#", bPred->block->getID(),
                    " out has ", FormatLiveDepMap(bPred->liveOut));
                bool predChanged = joinBlocks(
                    bPred->liveOut,
                    b.liveIn,
                    predEdge.second);
                changedAnyPred |= predChanged;
                TRACE("       liveOut for B#", bPred->block->getID(),
                    " changed to ", FormatLiveDepMap(bPred->liveOut));
            }
        } else {
            TRACE("    liveIn didn't change for this block");
        }

        return changedAnyPred;
    }


    // extends a given dependency back one instruction,
    // the range can be killed off and deleted or extended back normally
    LiveDepMap::iterator extendDepBackwards(
        Instruction *i,
        LiveDepMap &rLiveDefs,
        LiveDepMap::iterator &dItr,
        bool copyOut)
    {
        const InstDsts &iOups = instDsts[i->getID()];
        Dep &d = dItr->second;

        RegSet overlap(model);
        bool notEmpty = d.values.intersectInto(iOups.unionOf(), overlap);
        if (notEmpty && copyOut) {
            // this instruction defines some values used by this range
            //  - subtract out those values in store it as a def-use
            //    pair if this is the copyOut phase (after convergence)
            liveRangesResult.push_back(d);
            Dep &dCopy = liveRangesResult.back();
            dCopy.def = i;
            dCopy.values = overlap;
        }

        if (notEmpty && (!i->hasPredication() || i->is(Op::SEL))) {
            // don't subtract if the instruction is predicated
            // since there could be another definition above this
            //
            // sel does kill every active lane
            //
            // TODO: a nice enhancement would be to see if someone
            // behind us somewhere kills the other half
            //   (f0.0)  x = ..
            //   ...
            //   (~f0.0) y = ..
            // at def x with (x == y) we can fully kill the range off
            d.values.destructiveSubtract(overlap);
        }

        // lr.live.destructiveSubtract(iOups.destinations);
        // lr.live.destructiveSubtract(iOups.flagModifier);
        if (d.values.empty()) {
            TRACE("        ", d.str(), ": deleting range");
            dItr = rLiveDefs.erase(dItr);
        } else {
            if (i->getOpSpec().isFixedLatency()) {
                // TODO: we should do a pipe comparison here
                // compare the use's pipe with the definition's pipe
                d.minInOrderDist++;
            }
            TRACE("        ", d.str(), ": extending range");
            dItr++;
        }
        return dItr;
    }


    void startNewDepsBackwards(Instruction *i, LiveDepMap &rLiveDefs)
    {
        const InstSrcs &iInps = instSrcs[i->getID()];
        startNewDepsBackwardsWithSets(
            i,
            rLiveDefs,
            Dep::RAW,
            &iInps.predication,
            &iInps.sources);
        /*
        // disable WaW tracking for now
        const InstDsts &iOups = instDsts[i->getID()];
        startNewDepsBackwardsWithSets(
            i,
            rLiveDefs,
            Dep::WAW,
            &iOups.flagModifier,
            &iOups.destinations);
        */
    }
    void startNewDepsBackwardsWithSets(
        Instruction *i,
        LiveDepMap &rLiveDefs,
        Dep::Type type,
        const RegSet *rs1,
        const RegSet *rs2)
    {
        // early out (no dependencies on this instruction)
        if ((rs1 == nullptr || rs1->empty()) &&
            (rs2 == nullptr || rs2->empty()))
        {
            return;
        }

        Dep *d;
        const auto &itr = rLiveDefs.find(DepKey(type, i));
        if (itr != rLiveDefs.end()) {
            d = &itr->second;
        } else {
            auto val = rLiveDefs.emplace(DepKey(type, i), Dep(type, i));
            d = &(*val.first).second;
            d->minInOrderDist = 1;
            d->crossesBranch = false;
        }

        // clobber the old value.
        d->values.reset();
        if (rs1) {
            d->values.destructiveUnion(*rs1);
        }
        if (rs2) {
            d->values.destructiveUnion(*rs2);
        }
        TRACE("        ", d->str(), ": starting live range");
    }

    bool joinBlocks(
              LiveDepMap &predOUT, // predecessor block live OUT
        const LiveDepMap &succIN,  // successor   block live IN
        EdgeType /* ... */)        // jump or fallthrough
    {
        bool changed = false;
        for (const auto &lrElem : succIN) {
            const Dep &lrSuccIN = lrElem.second;
            auto r = predOUT.emplace(lrElem.first, lrSuccIN);
            auto isNewValue = r.second;
            Dep &lrPredOUT = r.first->second;
            if (isNewValue) {
                // was a new path
                lrPredOUT.crossesBranch = true;
                changed = true;
            } else {
                // changes an existing path

                // update the min distance
                if (lrSuccIN.minInOrderDist < lrPredOUT.minInOrderDist) {
                    changed = true;
                    lrPredOUT.minInOrderDist = lrSuccIN.minInOrderDist;
                    lrPredOUT.crossesBranch = true;
                }
                // add any new dependencies
                changed |= lrPredOUT.values.destructiveUnion(lrSuccIN.values);
            }
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
        TRACE("  => ", ss.str());
    }

    void completePaths() {
        // copy out the data
        TRACE("************ COPYING OUT RESULTS");
        for (BlockState &b : blockState) {
            recomputeBlockLiveIn(b, true);
        }
    }
};

// RAR{@3, #5 <- ?, r13..r14}
void Dep::str(std::ostream &os) const
{
    auto emitId = [&] (const Instruction *i) {
        if (i) {
            os << "#" << i->getID();
        } else {
            os << "?";
        }
    };
//    os << "@" << minInOrderDist;
//    os << ", ";
    emitId(use);
    os << "<=";
    emitId(def);
    os << "/";
    values.str(os);
//    os << "}";
}


std::string Dep::str() const
{
    std::stringstream ss;
    str(ss);
    return ss.str();
}


bool Dep::operator==(const Dep &p) const
{
    return
        def == p.def &&
        use == p.use &&
        useType == p.useType &&
        values == p.values &&
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
        TRACE("==== BLOCK ", bs.block->getID(), " LIVE-IN ====");
        EmitPaths(bs.liveIn);

        la.blockInfo.emplace_back(bs.block);
        BlockInfo &bi = la.blockInfo.back();
        auto addSet =
            [] (const LiveDepMap &map, std::vector<Dep> &out) {
                for (const auto &pair : map) {
                    const Dep &d = pair.second;
                    if (d.def != nullptr || d.useType != Dep::WAW) {
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
        if (d.useType != Dep::WAW) {
            la.deps.push_back(d);
        }
    }

    TRACE("=========== END ===========");
    return la;
}

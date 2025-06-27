/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


/******************************************************************************
 * This performs a basic liveness/DU analysis (reverse walk) of the
 * register files (GRF and ARF).  The RegSet abstraction tracks bytes
 * touched by instructions.
 *
 * We choose a reverse (backwards) analysis because we want the algorithm to
 * infer kernel inputs (or use of uninitialized variables).
 *    ==> Hence, read examples bottom up.
 ******************************************************************************
 * IMPLEMENTATION NOTES:
 *
 *  - The analysis is conservative/approximate.
 *
 *  - The algorithm runs in three steps.
 *      (a) precompute the data flow sets
 *      (b) iterate the graph until the fix point is reached
 *      (c) execute a final pass through to copy out data
 *
 * See below for notation and formalism below.
 */
#include "DUAnalysis.hpp"

// Uncomment this for debug tracing of the algorithm
// #define ENABLE_TRACING

#include <algorithm>
#ifdef ENABLE_TRACING
#include "../strings.hpp"
#include <cstdio>
#endif
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
// #include <unordered_map>

using namespace iga;

#ifdef ENABLE_TRACING
#define TRACE(...) (std::cout << iga::format(__VA_ARGS__) << "\n")
#else
#define TRACE(...)
#endif
/***************************************************************************
 * Notes: below we adopt an extremely compact notation, but which still
 * captures all the absurd complexity this hardward induces.  Here are some
 * notes and examples on the notation in comments.
 *
 *    - The analysis is bottom up; so generally read examples bottom up since
 *      that's the order we process things in.
 *
 *    - This is standard reverse-order bottom up iterative data flow analysis
 *      with a few enhancements.  Each block has a liveIN and liveOUT set.
 *      We recompute these iterateively and join them with the typical
 *      monotonic 'meet' function until a fixed point is reached.  (The sets
 *      should form properly ordered latices and should converge to a fixed
 *      solution.)
 *
 *    - Predication adds complexity classic compiler texts don't consider.
 *      At the simplest level consider a predicated instruction a short branch.
 *        I.e. X = (P) add ... is the same as: if (P) X = add ...
 *      However, often compilers will reduce if/else or expand selects into
 *      a predicate assignment and complement predicate.  E.g.
 *         X = (P) add ...;
 *         X = (~P) add ... definitely kills X
 *      Without special help, regular data flow treats this as:
 *          if (P) add .. X
 *          if (~P) add .. X
 *      which fails to definitely kill off X.
 *      Worse, the scheduler might float these pairs far apart, and, if that
 *      wasn't bad enough, the writes may be chunked into smaller pieces due to
 *      hardware restriction (several predicated ops to create X);
 *       e.g.
 *          X[1]   = (P[1])    add ..  fragment #0 of P
 *          X[0]   = (P[0])    add ..  fragment #1 of P
 *          X[0,1] = (~P[0,1]) add ..  full part of ~P[0,1]
 *                             use X[0,1]
 *      This can happen since we must break things down to smaller instructions
 *      for hardware restrictions (e.g. DF's max ExecSize is half F's).
 *      This leads to a "Humpty Dumpty" dataflow analysis (a mess).
 *      Thus, to detect complement predicate pairs, we must track all
 *      predicated operations in a dataflow within a block.
 *      Nevertheless, if pairs are nearby and uncomplicated (not fragmented),
 *      it should be fairly efficient.  However, pathological cases exist.
 *
 *    - Notation.  Rather that showing full complicated instructions,
 *      most examples reduce the to general 'def' and 'use' instructions
 *      in comments below.
 *           X =   def ... // defines X
 *           X = P def ... // defines X under predicate P
 *           ...
 *           use X
 *
 *    - Since we are dealing with fully vectorized code we use array
 *      notation to illustrate fragmenting.  Formally, the values needn't
 *      be adjacent (X[0] may not be near X[1]), but in practice they will be.
 *        A:  X[0,1] =      def ... // defines X[0] and X[1] unconditionally
 *        B:  X[1]   = P[0] def ... // defines X[1] under predicate P[0]
 *        C:  use X[1] // uses X[1] // uses just X[1]
 *      The above yields DU relations {(A,C,X[1]),(A,B,X[1])}
 *
 *    - In some cases we give data flow graphs with ASCII characters.
 *          (read this bottom up)
 *          XXXX||||  X[0] = def ...  // live range fully partially killed off
 *          ||||||||  ...             // nothing changes (disj. insts.)
 *          ||||++++  X[1] = P[0] def // part of range conditionally killed
 *          XXXXXXXX         use X[0,1]
 *      Even if not explicitly stated in an example, one should assume there
 *      could be non-interfering instructions between each line above.
 */

// A predicated kill within a live path within a basic block.
// These are created and killed off during analysis within a block.
struct PredicatedKill {
  bool inverted;
  const RegSet::Bits *predicate; // ptr for implicit constructors etc...
  RegSet kills;
  int instId; // instruction ID for debugging only

  PredicatedKill(bool inv, const RegSet::Bits &pr, const RegSet &ks, int id)
      : inverted(inv), predicate(&pr), kills(ks), instId(id) {}
};

// A set of distances (other instructions covered between def and use).
struct Dists {
  int allPipes = 0;

  void incrementFor(const Instruction &i) {
    (void)i;
    allPipes++;
  }

  bool meet(const Dists &rhs) {
    bool changed = false;
    if (rhs.allPipes < allPipes) {
      allPipes = rhs.allPipes;
      changed = true;
    }
    return changed;
  }

  bool operator==(const Dists &rhs) const { return allPipes == rhs.allPipes; }
  bool operator!=(const Dists &rhs) const { return !(*this == rhs); }
};

// A live path is a use instruction paired with the bytes that are live
// at the given instruction's position.
struct LivePath {
  // The instruction using the set of values we are tracking.
  Instruction &use;
  int useId;

  // The current set of values still live within this path.  We reduce
  // this until live is empty, at which point the live the path is killed
  // off.
  RegSet live;
  bool usePredInv;
  RegSet::Bits usePred;

  // Predicated kills are instructions prior to our current location
  // that conditionally killed off some subset of 'live'; this is a
  // local (block-only) view of predicated kills in this data flow.
  // EXAMPLE: (read bottom up)
  //   X = !P def   << finishes the kill (since we now have P and !P)
  //   ...
  //   X =  P def ... << partial kill #2
  //   ...
  //   X =  P def ... << partial kill #1
  //   ...
  //   U =    use  X  (new LivePath)
  //
  // We do have to be careful about interference
  // (e.g. someone redefines P or part of it)
  // The easy solution there is to just clear interfering elements
  // from this set when someone clobbers P.
  //
  // (It gets worse when you start considering partial kills and
  //  partial predicates: a real possibility with this SIMD non-sense)
  //    X[0,1] = def ..
  //    ...
  //    X[0] = @P[0] def
  //    ...
  //    X[1] = @P[1] def
  //    ...
  //           use X
  //
  // These are typically dropped during propagation back to a successor,
  // though if the successor is unique (a unit edge), then we can safely
  // do so.
  std::vector<PredicatedKill> pKills;

  // the minimum number of branches this path has crossed
  unsigned minBlocksCrossed = (unsigned)-1;
  // Same as above but excludes fallthrough
  // (this includes fallthrough)
  unsigned minBranchesCrossed = (unsigned)-1;

  // the minimum number of instructions this path has traveled
  Dists minDists;

  LivePath(Instruction &u)
      : use(u), useId(u.getID()), live(u.model()),
        usePredInv(u.hasPredication() && u.getPredication().inverse),
        usePred(RegSet(u.model()).bitSetFor(RegName::ARF_F)) {}

  // State that should not be propagated across blocks
  void clearLocalState() {
    usePred.clear();
    pKills.clear();
  }

  // The update() is called during path extension within a block against
  // instruction 'i'.
  //
  // (read bottom to top)
  //
  //      ||||    (still have pending predicates on P[2,4])
  //      ^^^^
  //  XXXX||||    X[0] =      def  <<<<< YOU ARE HERE <<<<<
  //  ||||||||
  //  ||||++++    X[1] = P[4] def
  //
  //  ||||||||    P[2] = def // kills P[2]
  //
  //  ||||||||
  //  ++++++++    X[0,1] = P[3,4] def
  //  ||||----    X[1] = P[2] def
  //  ||||||||
  //  ----||||    X[0] = ~P[0] def
  //  ||||||||
  //  XXXXXXXX    use ... X[0,1]
  //
  // In the above, entering the example instruction pKills would be:
  //   X[0]|~P[0] def, X[0]|P[3], X[1]|P[4]
  //  (note: X[0,1]|P[3,4] split to X[0]|P[3] and X[1]|P4)
  // The output should be:
  //   X[1]|P[4]
  //  since all X[0] predicates got killed off unconditionally
  //
  // SPECIFY: how to handle interfering definitions of P
  //  ==> I think we just ditch all predicates related to those new regions
  //      This is the analog of how it would work if they were proper branches
  //      (It'd be a fresh D/U pair only)
  void update(int iId, const RegSet::Bits &iPred, bool iPredInv,
              const RegSet &iKills, const RegSet &iOverlapU) {
    if (!iOverlapU.empty()) {
      // this instruction impacts our dataflow
      // (without considering predication)
      if (iPred.empty() ||
          (iPredInv == usePredInv && usePred.intersects(iPred))) {
        // definitely removing some values from this live range
        live.destructiveSubtract(iOverlapU);
        // see if this def is a complementary predicate
        subtractComplPredKills(iOverlapU);
      } else {
        // see if we can combine this subtraction with complementary
        // predicated kills
        subtractComplPredKills(iOverlapU, iPred, iPredInv);
      }
      if (live.empty()) {
        // we're about to be deleted; no need to tidy up our state
        return;
      }
    }

    addNewPredicatedKills(iOverlapU, iPred, iPredInv, iId);
  }

  // Checks if an instruction's predication interferes with this path's
  // use predication.
  // E.g.
  //  ? =    def // everything matches unpred. def
  //      P*  use X      // YES
  //
  // OR
  //
  //  ? = P[0,1] def
  //     ...
  //                 use X // YES (conservative)
  //      P[0]       use X // YES (intersects)
  //      P[0,1]     use X // YES (intersects)
  //      P[0,1,2,3] use X // YES (intersects)
  //     ~*          use X // NO (sign mismatch)
  //      P[2]       use X // NO (disjoint predicate)
  //

  bool matchesPredication(const RegSet::Bits &iPred, bool iPredInv) const {
    return (usePred.empty() || iPred.empty() ||
            (iPredInv == usePredInv && usePred.intersects(iPred)));
  }

  // If the current instruction is predicated and overlaps, we must
  // record the new predicated kills.
  // EXAMPLE:
  //  ....
  //  ||||++++  X[1] =  P[0] def  << YOU ARE HERE
  //  ....
  //  XXXXXXXX       =       use X[0,1]
  //
  // We don't bother to see if this covers another similarly predicated
  // kill later; extra copies are fine for our purposes.
  void addNewPredicatedKills(const RegSet &overlap, const RegSet::Bits &pred,
                             bool predInverted, int instId) {
    if (pred.empty() || overlap.empty()) {
      return;
    }
    TRACE("         I#", use.getID(), ": inserting predicated kill ",
          (predInverted ? "~" : ""),
          RegSet::str(overlap.getModel(), RegName::ARF_F, pred), " against I#",
          instId);
    pKills.emplace_back(predInverted, pred, overlap, instId);
  }

  // Folds complementary predicate kills over the same bytes and
  // same predication together an promotes them to unpredicated kills.
  // EXAMPLE:
  //  ||||----  X[1] = ~P[0] def <<<< should kill off X[1]
  //  ++++||||  X[0] =  P[0] def  << don't match
  //  ||||++++  X[1] =  P[0] def  << do match
  //  XXXXXXXX       = use X[0,1]
  void subtractComplPredKills(const RegSet &overlap, // subset of kills
                              const RegSet::Bits &pred, bool predInverted) {
    IGA_ASSERT(!pred.empty(), "predication should be non-empty");
    for (int i = (int)pKills.size() - 1; i >= 0; --i) {
      PredicatedKill &pk = pKills[i];
      if (pk.inverted == predInverted) {
        // sign matches, so we can't subtract
        continue;
      }
      // RegSet::Bits prOverlap =
      //    RegSet::Bits::intersection(pred, *pk.predicate);
      if (!pred.intersects(*pk.predicate)) {
        continue;
      }
      RegSet killOverlap = RegSet::intersection(overlap, pk.kills);
      if (killOverlap.empty()) {
        continue;
      }
      // some part of kill and predicate overlap with mismatch of signs
      // this overlap is "definitely" killed by this instruction
      bool changedL = live.destructiveSubtract(killOverlap);
      if (changedL && live.empty()) {
        break; // early out: nothing live ==> no more work needed
      }
      bool changedP = pk.kills.destructiveSubtract(killOverlap);
      if (changedP && pk.kills.empty()) {
        TRACE("         I#", use.getID(), ": invalidating predicated write");
        pKills.erase(pKills.begin() + i);
      } else {
        TRACE("         I#", use.getID(), ": eroding predicated write");
      }
    } // for: predicated kills
  }   // subtractPredicatedKills

  // Remove all interfering predicated kills.
  //
  // We could merge this with the other, but one might argument it's a bit
  // harder to read.  (i.e. it's similar but easier.)
  //
  // EXAMPLE:
  //  ||||XXXX  X[1] =       def
  //  ++++||||  X[0] =  P[0] def  << mismatch on 'overlap'
  //  ||||++++  X[1] =  P[0] def  << erase this
  //  XXXXXXXX       = use X[0,1]
  void subtractComplPredKills(const RegSet &overlap) {
    for (int i = (int)pKills.size() - 1; i >= 0; --i) {
      PredicatedKill &pk = pKills[i];
      RegSet killOverlap = RegSet::intersection(overlap, pk.kills);
      if (killOverlap.empty()) {
        continue;
      }
      bool changedP = pk.kills.destructiveSubtract(killOverlap);
      if (changedP && pk.kills.empty()) {
        TRACE("         I#", use.getID(),
              ": invalidating predicated write (uncond.)");
        pKills.erase(pKills.begin() + i);
      } else {
        TRACE("         I#", use.getID(),
              ": eroding predicated write (uncond.)");
      }
    } // for: predicated kills
  }   // subtractPredicatedKills

  // Remove predicated kills that interfere with this kill set even if
  // the current instruction is also predicated; and even if the overlap
  // is only partial.
  // Theoretically, if it was an additive operation (e.g. OR, BFN.OR),
  // we could safely retain it, but that's rare enough to be ignored.
  //
  // e.g. unconditional kill of a predicated kill
  //    ||||++++  X[1] = P[0] def <<< different P[0] than at A:
  //    ||||||||  P[0] =      def <<<<<< YOU ARE HERE
  // A: ||||----  X[1] = P[0] def
  // B: ++++||||  X[0] = P[1] def
  //    XXXXXXXX  use X[0,1]
  //
  // e.g. even if the predicate kill is predicated itself
  //    ||||++++  X[1] = P[0] def <<< different P[0] than at A:
  //    ||||||||  P[0] = P[2] def <<<<<< YOU ARE HERE
  // A: ||||----  X[1] = ~P[0] def
  // B: ++++||||  X[0] = P[1] def
  //    XXXXXXXX  use X[0,1]
  //
  // e.g. partial kill of predicated kill still causes full removal
  //    ||||++++  X[1] = P[0] def <<< different P[0] than at A:
  //    ||||||||  P[0] = P[2] def <<<<<< YOU ARE HERE
  // B: --------  X[0] = ~P[0,1] def
  //    XXXXXXXX  use X[0,1]
  // Even though the partial kill only clobbers P[0] (P[1] is intact),
  // we don't track which bytes that covers (alignment) in the predicated
  // kill; consequently the easiest thing to do is punt and conservatively
  // forget about the entire predicated kill.  In practice, garbage like
  // this, shouldn't be generated.
  void updateForPredicateRedefs(const RegSet &kills) {
    const RegSet::Bits &killsF = kills.bitSetFor(RegName::ARF_F);
    if (killsF.empty()) {
      return; // this instruction doesn't write predicates ==> done
    }
    if (killsF.intersects(usePred)) {
      // this instruction fiddles writes the predicates we depend on
      // conservatively stop trying to constrain defs by same predicate
      usePred.clear();
    }
    // any predicated kills we are tracking
    for (int i = (int)pKills.size() - 1; i >= 0; --i) {
      PredicatedKill &pk = pKills[i];
      if (pk.predicate->intersects(killsF)) {
        pKills.erase(pKills.begin() + i);
      }
    } // for
  }

  // The meet operator
  //
  // This updates 'this' (implicit object / receiver) merging data from
  // 'rhs' and indicates if something changed
  bool meet(const LivePath &rhs) {
    bool changed = false;

    changed |= minDists.meet(rhs.minDists);

    if (minBlocksCrossed == (unsigned)-1 ||
        rhs.minBlocksCrossed < minBlocksCrossed) {
      minBlocksCrossed = rhs.minBlocksCrossed;
      changed = true;
    }
    if (minBranchesCrossed == (unsigned)-1 ||
        rhs.minBranchesCrossed < minBranchesCrossed) {
      minBranchesCrossed = rhs.minBranchesCrossed;
      changed = true;
    }

    changed |= live.destructiveUnion(rhs.live);

    return changed;
  }

  Dep toDep(Instruction *def, const RegSet &values) const {
    Dep d(def, values, &use);
    d.minInsts = minDists.allPipes;
    // cast to int because (unsigned)0xFFFFFFFF needed for
    // set reduction latice (min function)
    d.crossesBlock = (int)minBlocksCrossed > 0;
    d.crossesBranch = (int)minBranchesCrossed > 0;
    return d;
  }

  bool operator==(const LivePath &rhs) const {
    return &use == &rhs.use && live == rhs.live && minDists == rhs.minDists &&
           minBlocksCrossed == rhs.minBlocksCrossed &&
           minBranchesCrossed == rhs.minBranchesCrossed;
  }
  bool operator!=(const LivePath &rhs) const { return !(*this == rhs); }

  std::string str() const {
    std::stringstream ss;
    ss << "#" << use.getID() << " = " << live.str();
    return ss.str();
  }
  std::string strFull() const {
    std::stringstream ss;
    ss << str();
    if (!pKills.empty()) {
      ss << " with pkills ";
      bool first = true;
      for (const PredicatedKill &pk : pKills) {
        if (first)
          first = false;
        else
          ss << ", ";
        pk.kills.str(ss);
        ss << "=";
        if (pk.inverted) {
          ss << "~";
        }
        ss << RegSet::str(pk.kills.getModel(), RegName::ARF_F, *pk.predicate);
        ss << " def I#" << pk.instId;
      }
    }
    return ss.str();
  }
}; // LivePath

#ifdef ENABLE_TRACING
// forces certain debug functions to be compiled under trace
void fake(const void *vlp, std::ostream &os) {
  const LivePath *lp = (const LivePath *)vlp;
  os << lp->strFull();
  os << lp->pKills[0].predicate->str();
}
#endif

using LivePaths = std::map<Instruction *, LivePath>;

enum class EdgeType { FALLTHROUGH, JUMP };

// Information pertaining to a given block.  We convert this to
// a public-facing BlockInfo that discards all the temporary and
// non-essential information from the algorithm when extracting
// and returning the results.
struct BlockState {
  Block *block;

  // predecessor info (second coordinate is true if the node is *not*
  // a fallthrough (i.e. it's a jump)
  // indexed by block ID
  std::vector<std::pair<BlockState *, EdgeType>> pred;

  LivePaths liveIn;
  LivePaths liveOut;

  // if a worklist algorithm is used
  bool dirty = true;

  BlockState(Block *b) : block(b) {}

  std::string str() const {
    std::stringstream ss;
    ss << idstr() << " pred:{";
    bool first = true;
    for (const auto &pbse : pred) {
      if (first)
        first = false;
      else
        ss << ",";
      ss << pbse.first->idstr();
    }
    ss << "}";
    return ss.str();
  }

  std::string idstr() const { return "B#" + std::to_string(block->getID()); }
};

struct DepAnalysisComputer {
  const Model &model;
  const Kernel *k;

  // precomputed sources and destination sets indexed by instruction ID
  std::vector<RegSet> instDstsUnion;
  std::vector<InstSrcs> instSrcs;

  // mid-state and then output
  std::vector<BlockState> blockState;

  // outputs
  DepAnalysis &results;

  DepAnalysisComputer(const Kernel *_k, DepAnalysis &_results)
      : model(_k->getModel()), k(_k), results(_results) {
    sanityCheckIR(k); // should nop in release

    results.deps.clear();
    results.liveIn.clear();
    results.sums.resize(k->getInstructionCount());

    instSrcs.reserve(k->getInstructionCount());
    instDstsUnion.reserve(k->getInstructionCount());

    // we must do this so the vector doesn't resize
    blockState.reserve(k->getBlockList().size());

    // pre-assign ID's we know to be valid and
    // precompute instruction dependencies
    for (Block *b : k->getBlockList()) {
      IGA_ASSERT((size_t)b->getID() == blockState.size(),
                 "we assume block ids go from 0 ... n-1");
      blockState.emplace_back(b);

      auto iitr = b->getInstList().begin();
      while (iitr != b->getInstList().end()) {
        Instruction *i = *iitr;
        IGA_ASSERT((size_t)i->getID() == instSrcs.size(),
                   "we assume block ids go from 0 ... n-1");
        instSrcs.emplace_back(InstSrcs::compute(*i));
        IGA_ASSERT((size_t)i->getID() == instDstsUnion.size(),
                   "we assume block ids go from 0 ... n-1");
        instDstsUnion.emplace_back(InstDsts::compute(*i).unionOf());
        // Given Gather Send this attempts to resolve the registers accessed
        // indirectly; we expect the pattern:
        //
        //   (W) mov (1) s0.K<1>:{uq,q} 0x000000000105010A:{uq,q}
        //   ... maybe an instruction or two
        //   send ... r[s0.K]:4 ... // sends {r1,r5,r1,r10}
        //
        if (i->isGatherSend()) {
          int src0Len = i->getSrc0Length();
          auto gatheredGrfs =
              findGatherSendMov(iitr, b->getInstList().begin(),
                           i->getSource(0).getDirRegRef().subRegNum, src0Len);
          instSrcs.back().sources.destructiveUnion(gatheredGrfs);
        }
        iitr++;
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
          blockState[i + 1].pred.emplace_back(&b, EdgeType::FALLTHROUGH);
      } else {
        // at least one instruction in this block;
        // mark 'b' as a predecessor to all targets
        //
        // unpredicated JMPI and an EOT are the only op that
        // stop the block hard
        const Instruction *iTerm = il.back();
        if (!isLast && fallthroughPossible(iTerm)) {
          // normal fallthrough
          blockState[i + 1].pred.emplace_back(&b, EdgeType::FALLTHROUGH);
        }
        for (unsigned srcIx = 0; srcIx < iTerm->getSourceCount(); srcIx++) {
          const Operand &src = iTerm->getSource(srcIx);
          if (src.getKind() == Operand::Kind::LABEL) {
            BlockState &tb = blockState[src.getTargetBlock()->getID()];
            tb.pred.emplace_back(&b, EdgeType::JUMP);
          }
        }
      } // non-empty block 'b'
    }   // end for(block state)

    TRACE("******** INITIAL CONDITIONS **********");
    for (const Block *b : k->getBlockList()) {
      TRACE("  BLOCK ", blockState[b->getID()].str());
      for (const Instruction *i : b->getInstList()) {
        (void)i;
        TRACE("    DEF #", i->getID(), " |  ", trimTrailingWs(i->str()),
              " ||  ", instDsts[i->getID()].str(),
              " <== ", instSrcs[i->getID()].str());
      }
    }
  } // DepAnalysisComputer::DepAnalysisComputer

  RegSet findGatherSendMov(InstList::iterator iitr, InstList::iterator ibegin,
                      uint16_t s0Subreg, int src0Len) {
    RegSet grfs(model);
    // The local match allows for kills of s0.K
    //   (P) mov ... s0.K:T   IMM:T
    //   (P) mov ... s0.K:T   IMM:T
    //   send  r[s0.K]
    if (iitr == ibegin)
      return grfs; // first instruction of the block
    // const Instruction *iSendg = *iitr;
    //
    if (src0Len <= 0)
      return grfs;
    //
    RegSet rsPredFirstMov(model);
    bool predFirstMovSign = false;

    RegSet rsS0(model);
    if (src0Len <= 4)
      rsS0.add(RegName::ARF_S, RegRef((uint16_t)0, uint16_t(s0Subreg / 4)),
               Type::UD);
    else
      rsS0.add(RegName::ARF_S, RegRef((uint16_t)0, uint16_t(s0Subreg / 8)),
               Type::UQ);
    //
    do {
      iitr--;
      const Instruction *i = *iitr;
      const Operand &dst = i->getDestination();
      const Operand &src0 = i->getSource(0);

      bool isS0Def =
          i->is(Op::MOV) && dst.getKind() == Operand::Kind::DIRECT &&
          dst.getDirRegName() == RegName::ARF_S && src0.isImm() &&
          TypeSizeInBits(dst.getType()) >= 4 &&
          TypeSizeInBits(dst.getType()) / 8 * dst.getDirRegRef().subRegNum ==
              s0Subreg &&
          TypeSizeInBits(src0.getType()) >= 4;
      if (isS0Def) {
        // (W) mov (1) s0.K:{q,uq}  IMM64:{q,uq,d,ud,w,uw}
        // (W) mov (1) s0.K:{d,ud}  IMM64:{d,ud,w,uw}
        // (W) mov (1) s0.K:{f}     IMM:{f}
        uint64_t imm = src0.getImmediateValue().u64;
        for (int i = 0; i < src0Len; i++, imm >>= 8) {
          int grf = (imm & 0xFF);
          grfs.addReg(RegName::GRF_R, grf);
        }
        if (!i->hasPredication()) {
          break; // unpredicated is full def (hopfeully the normal case)
        } else {
          // we also allow for two predicated definitions
          //   (W &  fX) mov .. s0.K:q   0x0201:q
          //   (W & ~fX) mov .. s0.K:q   0x0304:q
          if (rsPredFirstMov.empty()) {
            rsPredFirstMov.add(RegName::ARF_F, i->getFlagReg(), Type::B);
            predFirstMovSign = i->getPredication().inverse;
          } else {
            RegSet rsSecondPred{model};
            rsSecondPred.add(RegName::ARF_F, i->getFlagReg(), Type::B);
            if (rsSecondPred.intersects(rsPredFirstMov)) {
              if (i->getPredication().inverse != predFirstMovSign) {
                break; // full def
              }
              // else: dead def (continue to look for second)
            }
          }
        }
      } else {
        // make sure it doesn't interfere with a partial definition's
        // scalar or predication
        const auto &rsDsts = instDstsUnion[i->getID()];
        if (rsDsts.intersects(rsS0)) {
          // some other jerk clobbered part of our s0.# with an
          // instruction we don't deal with
          break;
        } else if (rsDsts.intersects(rsPredFirstMov)) {
          // someone clobbered the predicate value we were tracking
          break;
        }
      }
    } while (iitr != ibegin);

    return grfs;
  } // findGatherSendMov

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
      IGA_ASSERT(blockIds.find(bId) == blockIds.end(), "duplicate block ID");
      for (const Instruction *i : b->getInstList()) {
        auto id = i->getID();
        IGA_ASSERT(id < 2 * (int)instCount, "instruction ID's should be small");
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

  enum class IterationStrategy { INORDER, REVERORD, WORKLIST };

  void computeLiveInPaths() {
    static const IterationStrategy strat = IterationStrategy::WORKLIST;

    if (strat == IterationStrategy::INORDER) {
      results.iterations = iterateLiveInSets<IterationStrategy::INORDER>();
    } else if (strat == IterationStrategy::REVERORD) {
      results.iterations = iterateLiveInSets<IterationStrategy::REVERORD>();
    } else if (strat == IterationStrategy::WORKLIST) {
      results.iterations = iterateLiveInSets<IterationStrategy::WORKLIST>();
    } else {
      IGA_ASSERT_FALSE("invalid walk strategy");
    }
  }

  template <IterationStrategy STRAT> int iterateLiveInSets() {
    int itr = 0;
    bool changed = false;
    do {
      TRACE("******* STARTING LIVE-IN ITERATION ", itr);

      changed = false;
      if (STRAT == IterationStrategy::INORDER) {
        for (BlockState &bs : blockState) {
          TRACE("  *** processing ", bs.idstr());
          changed |= recomputeBlockLiveIn(bs, false);
        }
      } else if (STRAT == IterationStrategy::REVERORD) {
        for (int bsi = (int)blockState.size() - 1; bsi >= 0; bsi--) {
          BlockState &bs = blockState[bsi];
          TRACE("  *** processing ", bs.idstr());
          changed |= recomputeBlockLiveIn(bs, false);
        }
      } else if (STRAT == IterationStrategy::WORKLIST) {
        for (int bsi = (int)blockState.size() - 1; bsi >= 0; bsi--) {
          BlockState &bs = blockState[bsi];
          if (bs.dirty) {
            TRACE("  *** processing ", bs.idstr());
            changed |= recomputeBlockLiveIn(bs, false);
          } else {
            TRACE("  *** skipping ", bs.idstr());
          }
        }
      } else {
        IGA_ASSERT_FALSE("INVALID WALK STRATEGY");
      }

      TRACE("******* ENDING ITERATION ", itr);
      itr++;
    } while (changed);
    return itr;
  }

  // Starting with liveOut, walk back through the instructions
  // extending all paths backwards.  Kill off stuff definitely defined,
  // and start new paths.  At the end, update this block's 'liveIn'
  // and push it back across to predecessor block's 'liveOut'
  bool recomputeBlockLiveIn(BlockState &bs, bool copyOut) {
#ifdef ENABLE_TRACING
    std::string bLiveInBefore = FormatLivePaths(b.liveIn);
#endif
    // all the live paths at the end of this block
    LivePaths lps = bs.liveOut;

    // FOR each instruction i
    //   extend paths backwards
    //   remove any paths killed off by a definition
    //   start any new paths induced by i's uses
    auto &il = bs.block->getInstList();
    for (auto iItr = il.rbegin(), iItrEnd = il.rend(); iItr != iItrEnd;) {
      Instruction &i = **iItr;
      TRACE("      *** I#", i.getID(), ": ", trimTrailingWs(i.str()));

      // extend live ranges for all active paths
      LivePaths::iterator lpsItr = lps.begin(), lpsEnd = lps.end();
      while (lpsItr != lpsEnd) {
        lpsItr = extendLivePathBackwards(i, lps, lpsItr, copyOut);
      }

      // any use of a variable starts a new live range
      startNewLivePathBackwards(i, lps);

      // after all paths are moved back we
      if (copyOut) {
        RegSet rs(model);
        for (const auto &lps : lps) {
          const LivePath &lp = lps.second;
          rs.destructiveUnion(lp.live);
        }
        LiveCount &lc = results.sums[i.getID()];
        lc.grfBytes = (unsigned)rs.bitSetFor(RegName::GRF_R).cardinality();
        lc.accBytes = (unsigned)rs.bitSetFor(RegName::ARF_ACC).cardinality();
        lc.scalarBytes = (unsigned)rs.bitSetFor(RegName::ARF_S).cardinality();
        lc.flagBytes = (unsigned)rs.bitSetFor(RegName::ARF_F).cardinality();
        lc.indexBytes = (unsigned)rs.bitSetFor(RegName::ARF_A).cardinality();
      }

      iItr++;
    } // for instructions in this block

    // clear all predicated kills before propagating the live path back
    for (auto &e : lps) {
      e.second.clearLocalState();
    }

    // mark this block as up to date; this must precede propagation since
    // this block might be a predecessor of itself
    bs.dirty = false;

    //////////////////////////////////////////////////////////////////
    // propagate live paths back into our liveIN and eventually across
    // to caller
    bool bLiveInChanged = updateLiveDefs(bs.liveIn, lps);
    bool changedAnyPred = false;
    if (bLiveInChanged) {
      // push information back to predecessor nodes
      TRACE("       ", bs.idstr(), ".IN changed from ", bLiveInBefore, " to ",
            FormatLivePaths(bs.liveIn));
      for (auto &predEdge : bs.pred) {
        BlockState &bsPred = *(BlockState *)predEdge.first;

#ifdef ENABLE_TRACING
        std::string predBefore = FormatLivePaths(bsPred.liveOut);
#endif
        bool predChanged =
            propagateLivePathsBack(bsPred.liveOut, bs.liveIn, predEdge.second);
        bsPred.dirty |= predChanged;
        changedAnyPred |= predChanged;
        TRACE("       ", bs.idstr(), ".OUT changed from ", predBefore, " to ",
              FormatLivePaths(bsPred.liveOut));
      }
    } else {
      TRACE("       ", bs.idstr(), ".IN didn't change");
    }

    return changedAnyPred;
  }

  // extends a given dependency back one instruction,
  // the range can be killed off and deleted or extended back normally
  LivePaths::iterator extendLivePathBackwards(Instruction &i, LivePaths &lps,
                                              LivePaths::iterator &lpItrs,
                                              bool copyOut) {
    const RegSet &iKills = instDstsUnion[i.getID()];

    const RegSet::Bits &iPred =
        instSrcs[i.getID()].predication.bitSetFor(RegName::ARF_F);
    bool iPredInv = i.hasPredication() && i.getPredication().inverse;

    LivePath &lp = lpItrs->second;

    // capture changes to predicates before considering this instruction
    //   X = (~P) def (le)P
    //       (P) use X
    // don't want to accidentially subtract out this def
    lp.updateForPredicateRedefs(iKills);

    RegSet iOverlap(model);

    bool matchesPredication = lp.matchesPredication(iPred, iPredInv);
    if (matchesPredication) {
      bool overlapNotEmpty = lp.live.intersectInto(iKills, iOverlap);
      // overlap are now the bytes we write that intersect with this live
      // range; if not empty, this constitutes a new D/U pair
      if (overlapNotEmpty && copyOut) {
        results.deps.push_back(lp.toDep(&i, iOverlap));
      }
    }

    lp.update(i.getID(), iPred, iPredInv, iKills, iOverlap);

    if (lp.live.empty()) {
      TRACE("        ", lp.str(), ": deleting range");
      lpItrs = lps.erase(lpItrs);
    } else {
      lp.minDists.incrementFor(i);
      TRACE("        ", lp.str(), ": extending range");
      lpItrs++;
    }

    return lpItrs;
  }

  void startNewLivePathBackwards(Instruction &i, LivePaths &lps) {
    const InstSrcs &iInps = instSrcs[i.getID()];
    const RegSet &rsPreds = iInps.predication;
    const RegSet &rsSrcs = iInps.sources;
    const RegSet::Bits usePred = rsPreds.bitSetFor(RegName::ARF_F);

    // early out (no dependencies on this instruction)
    if (rsPreds.empty() && rsSrcs.empty()) {
      return;
    }

    LivePath *lp;
    const auto &itr = lps.find(&i);
    if (itr != lps.end()) {
      lp = &itr->second;
    } else {
      auto val = lps.emplace(&i, i);
      lp = &val.first->second;
      // lp->minBranchesCrossed = 0;
      // lp->minBlocksCrossed = -1;
    }

    // clobber the old value
    lp->live.reset();
    lp->live.destructiveUnion(rsPreds);
    lp->live.destructiveUnion(rsSrcs);
    lp->usePred.clear();
    lp->usePred.add(usePred);
    TRACE("        ", lp->str(), ": starting live range");
  }

  // propagates a successor's liveIN set to its predecessors's liveOUT
  bool
  propagateLivePathsBack(LivePaths &predOUT,      // predecessor block live OUT
                         const LivePaths &succIN, // successor   block live IN
                         EdgeType et)             // jump or fallthrough
  {
    bool changed = false;
    for (const auto &lrElem : succIN) {
      const LivePath &lrSuccIN = lrElem.second;
      auto r = predOUT.emplace(lrElem.first, lrSuccIN);
      bool isNewValue = r.second;
      LivePath &lrPredOUT = r.first->second;
      if (isNewValue) {
        // a new path
        lrPredOUT.minBranchesCrossed = 0;
        lrPredOUT.minBlocksCrossed = 1;
        if (et == EdgeType::JUMP) {
          lrPredOUT.minBranchesCrossed = 1;
        }
        changed = true;
      } else {
        // changes an existing path
        changed |= lrPredOUT.meet(lrSuccIN);
        // TODO: if it's a unit branch, copy the live predicate info
      }
    } // for all source paths in succIN
    return changed;
  }

  static bool updateLiveDefs(LivePaths &to, const LivePaths &from) {
    bool changed = from != to;
    if (changed) {
#if defined(__ANDROID__)
      to.clear();
      for (auto iter : from) {
        to.insert(iter);
      }
#else
      to = LivePaths(from);
#endif
    }
    return changed;
  }

  void completePaths() {
    // copy out the data
    TRACE("************ COPYING OUT RESULTS");
    for (BlockState &b : blockState) {
      recomputeBlockLiveIn(b, true);
    }
    if (!blockState.empty()) {
      // any ranges left over on the entry block can be tagged as
      // program inputs
      for (const auto &lpe : blockState.front().liveIn) {
        const auto &lp = lpe.second;
        results.liveIn.push_back(lp.toDep(nullptr, lp.live));
      }
    }
  }
};

// #1 =*> #3 {r13..r14}
// ? =*> #3 {r13..r14}
void Dep::str(std::ostream &os) const {
  auto emitId = [&](const Instruction *i) {
    if (i) {
      os << "#" << i->getID();
    } else {
      os << "?";
    }
  };
  emitId(def);
  if (crossesBranch) {
    os << "=*>";
  } else {
    os << "=>";
  }
  emitId(use);
  os << " ";
  values.str(os);
}

std::string Dep::str() const {
  std::stringstream ss;
  str(ss);
  return ss.str();
}

bool Dep::operator==(const Dep &p) const {
  return def == p.def && use == p.use && values == p.values &&
         crossesBranch == p.crossesBranch && crossesBlock == p.crossesBlock &&
         minInsts == p.minInsts;
}

DepAnalysis iga::ComputeDepAnalysis(const Kernel *k) {
  DepAnalysis la;

  DepAnalysisComputer lac(k, la);
  lac.runAnalysis();

  TRACE("=========== END ===========");
  return la;
}

#ifdef _DEBUG
// to suppress compiler warning
std::string IGA_DUAnalysis_ForceCodeGen_ForDebug(const void *, const void *);

// dummy function to force codegen of certain internal functions for debug.
std::string IGA_DUAnalysis_ForceCodeGen_ForDebug(const void *plp,
                                                 const void *bs) {
  const LivePath *lp = (const LivePath *)plp;
  return lp->strFull() + ((const BlockState *)bs)->idstr();
}
#endif //

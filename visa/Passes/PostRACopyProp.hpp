/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _G4_POST_RA_COPY_PROP_H_
#define _G4_POST_RA_COPY_PROP_H_

#include "BuildIR.h"
#include "FlowGraph.h"
#include "G4_Kernel.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vISA {

    // Post-RA local copy propagation. Removes the GRF-to-GRF movs left behind
    // by spill/fill cleanup by forwarding their sources to the readers:
    //   (W) mov (16) r26 r42
    //       add (16) r28 r26 r24  ==>  add (16) r28 r42 r24
    // Hazards are tracked on physical GRF byte ranges. A mov is removed only if
    // every read of its dst is rewritten; otherwise its rewrites are rolled back.
    class PostRACopyProp {
    public:
        PostRACopyProp(G4_Kernel& k, IR_Builder& b)
            : kernel(k), builder(b), grfSize(k.numEltPerGRF<Type_UB>()) {
        }
        void run();

    private:
        // Inclusive linearized GRF byte range.
        struct Footprint {
            unsigned start = 0;
            unsigned end = 0;
            bool overlaps(const Footprint& o) const {
                return start <= o.end && o.start <= end;
            }
            bool contains(const Footprint& o) const {
                return start <= o.start && o.end <= end;
            }
        };

        struct Rewrite {
            G4_INST* inst;
            unsigned srcIdx;
            G4_Operand* oldOpnd;
            G4_Declare* oldDcl; // root declare of oldOpnd
        };

        struct Copy {
            INST_LIST_ITER movIt;
            G4_INST* mov;
            Footprint dst;
            Footprint src;
            G4_Declare* dstDcl; // root declare
            G4_Declare* srcDcl; // root declare
            bool canForward = true; // dst and src unmodified since the mov
            bool observed = false;  // dst read without rewrite; mov must stay
            std::vector<Rewrite> rewrites;
        };

        G4_Kernel& kernel;
        IR_Builder& builder;
        const unsigned grfSize;
        // Direct source reads per root declare in the kernel.
        std::unordered_map<G4_Declare*, unsigned> numReads;
        // In-flight copies of the current BB.
        std::vector<Copy> active;
        std::vector<INST_LIST_ITER> toErase;
        // Removed movs; skipped on rollback.
        std::unordered_set<G4_INST*> erased;

        void countReads();
        void runOnBB(G4_BB* bb);
        bool isCandidate(G4_INST* inst) const;
        bool isBarrier(G4_INST* inst) const;
        bool canRewriteInto(G4_INST* inst) const;
        bool isRewriteLegal(G4_INST* inst, unsigned srcIdx, const Footprint& newFp,
            const G4_Declare* oldDcl) const;
        bool getWriteFootprint(G4_INST* inst, Footprint& fp) const;
        Footprint getReadFootprint(G4_INST* inst, G4_Operand* src) const;
        bool isFullKill(G4_INST* inst, const Footprint& wFp,
            const Copy& c) const;
        bool canIgnoreLiveOut(G4_Declare* dcl) const;
        void processReads(G4_INST* inst);
        void processWrites(G4_INST* inst);
        void finalize(Copy& c, bool killed);
        void finalizeAll();
    };

} // namespace vISA

#endif // _G4_POST_RA_COPY_PROP_H_

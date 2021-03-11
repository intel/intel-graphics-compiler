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

#ifndef _IGA_IR_DUANALYSIS_HPP
#define _IGA_IR_DUANALYSIS_HPP

#include "../IR/RegSet.hpp"
#include "../IR/Kernel.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace iga
{
    // A live range is a path from a definition of some register values to
    // a use.  The use may be a read (typical case) or a write (since we need
    // to be able to track WAW dependencies).
    struct Dep
    {
        // If the use is a read, this will be READ.
        // WRITE indicates that bytes are being redefined.
        // E.g.
        //  mov r0 ...
        //  mul r1:q ..  r0 // RAW dependency (on the add)
        //  add r0 ...   // WAR dependency
        //  add r1:d ... // WAW
        enum Type {RAW, WAR, WAW}  useType = RAW;
        //
        // The producer instruction;
        // this can be nullptr if the value is a program input
        Instruction             *def = nullptr;
        //
        // The consumer instruction;
        // this can be nullptr if the value is a program output
        Instruction             *use = nullptr; // or write (redef)
        //
        // The register values that are live in this path
        RegSet                   values;
        //
        // This is number of in-order instructions this path covers
        // We determine in-orderness via OpSpec::isFixedLatency
        int                      minInOrderDist = 0;
        // indicates if the dependency crosses a branch (JEU)
        // N.b. this will false for fallthrough since that isn't a branch
        //        mov r1 ...
        // (f0.0) jmpi TARGET
        //        add ...  r1 // crossesBranch = false here
        // TARGET:
        //        mul ...  r1 // crossesBranch = true here
        bool                     crossesBranch = false;

        Dep(const Model &m) : values(m) { }
        Dep(Type t, Instruction *_use)
            : useType(t)
            , def(nullptr)
            , use(_use)
            , values(*Model::LookupModel(_use->platform()))
            , minInOrderDist(0)
            , crossesBranch(false)
        {
        }
        Dep(const Dep &) = default;
        Dep &operator=(const Dep &) = default;

        bool operator==(const Dep &p) const;
        bool operator!=(const Dep &p) const { return !(*this == p); }

        // for debug
        void str(std::ostream &os) const;
        std::string str() const;
    };

    struct BlockInfo
    {
        Block                   *block;
        std::vector<Dep>         liveDefsIn;
        std::vector<Dep>         liveDefsOut;

        BlockInfo(Block *blk) : block(blk) { }
        BlockInfo(const BlockInfo &) = default;
        // BlockInfo(BlockInfo &&) = default;
    };

    struct DepAnalysis
    {
        // information on each block
        std::vector<BlockInfo>   blockInfo;
        //
        // relation of definitions and uses
        std::vector<Dep>         deps;
    };

    // the primary entry point for the live analysis
    DepAnalysis ComputeDepAnalysis(Kernel *k);
} // namespace IGA

#endif // _IGA_IR_ANALYSIS_HPP

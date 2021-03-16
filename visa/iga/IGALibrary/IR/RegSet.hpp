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

#ifndef _IGA_IR_REGSET_HPP
#define _IGA_IR_REGSET_HPP

// Future replacement for RegDeps(.cpp/.hpp)
// Need to decouple machine state logic from the dependency set.

#include "BitSet.hpp"
#include "Instruction.hpp"
#include "Kernel.hpp"

#include <ostream>
#include <string>


namespace iga
{
    // A register set represent all the storage in the register files
    // at a certain granularity (currently typically byte).
    //
    class RegSet
    {
        BitSet<> *bitSetForPtr(RegName rn);
        BitSet<> &bitSetFor(RegName rn);
        const BitSet<> &bitSetFor(RegName rn) const;

        size_t offsetOf(RegName rn, int reg) const;
        size_t offsetOf(RegName rn, RegRef rr, size_t typeSize) const;
        size_t offsetOf(RegName rn, RegRef rr, Type t) const;
        bool isTrackedReg(RegName rn) const;
    public:
        RegSet(const Model &m);
        // RegSet(const RegSet &) = default;
        // RegSet &operator=(const RegSet &) = default;

        bool  operator==(const RegSet &rs) const;
        bool  operator!=(const RegSet &rs) const { return !(*this == rs); }

        ///////////////////////////////////////////////////////////////////////
        // set operations
        bool  empty() const;
        void  reset();
        bool  intersects(const RegSet &rhs) const;
        bool  destructiveUnion(const RegSet &inp);
        bool  destructiveSubtract(const RegSet &rhs);
        bool  intersectInto(const RegSet &rhs, RegSet &into) const;

        ///////////////////////////////////////////////////////////////////////
        // generic add functions
        bool addReg(RegName rn, int reg);
        bool addRegs(RegName rn, int reg, int n = 1);
        bool add(RegName rn, size_t regFileOffBits, size_t numBits);
        bool add(RegName rn, RegRef r, Type t);

        // for pred or condmod
        bool addFlagRegArf(RegRef fr, ExecSize es, ChannelOffset co);

        ///////////////////////////////////////////////////////////////////////
        // specialized add functions
        bool addSourceInputs(const Instruction &i);
        bool addPredicationInputs(const Instruction &i);
        bool addSourceOperandInput(const Instruction &i, int srcIx);
        bool addSourceImplicitAccumulator(const Instruction &i);
        //
        bool addDestinationOutputs(const Instruction &i);
        bool addFlagModifierOutputs(const Instruction &i);
        bool addDestinationImplicitAccumulator(const Instruction &i);
        //

        static RegSet unionOf(const RegSet &r1, const RegSet &r2) {
            RegSet r12(*r1.model);
            (void)r12.destructiveUnion(r1);
            (void)r12.destructiveUnion(r2);
            return r12;
        }

        bool setDstRegion(
            RegName rn,
            RegRef rr,
            Region r,
            size_t execSize,
            size_t typeSizeBits);
        bool setSrcRegion(
            RegName rn,
            RegRef rr,
            Region r,
            size_t execSize,
            size_t typeSizeBits);

        void        str(std::ostream &os) const;
        std::string str() const;

    private:

    private:
        const Model *model;
        BitSet<> bitsR;
        BitSet<> bitsA;
        BitSet<> bitsAcc;
        BitSet<> bitsF;
    };


    struct InstSrcs
    {
        RegSet predication;
        RegSet sources;
        InstSrcs(const Model &m) : predication(m), sources(m) { }

        RegSet unionOf() const {
            return RegSet::unionOf(predication, sources);
        }
        bool empty() const { return predication.empty() && sources.empty(); }
        std::string str() const { return unionOf().str(); }

        static InstSrcs compute(const Instruction &i);
    };
    struct InstDsts
    {
        RegSet flagModifier;
        RegSet destinations;

        InstDsts(const Model &m) : destinations(m), flagModifier(m) { }

        RegSet unionOf() const {
            return RegSet::unionOf(flagModifier, destinations);
        }
        bool subtractFrom(RegSet &rs) const {
            bool changed = false;
            changed |= rs.destructiveSubtract(flagModifier);
            changed |= rs.destructiveSubtract(destinations);
            return changed;
        }
        bool empty() const { return flagModifier.empty() && destinations.empty(); }
        std::string str() const { return unionOf().str(); }

        static InstDsts compute(const Instruction &i);
    };


    inline InstSrcs InstSrcs::compute(const Instruction &i)
    {
        InstSrcs iss(*Model::LookupModel(i.platform()));
        iss.predication.addPredicationInputs(i);
        iss.sources.addSourceInputs(i);
        return iss;
    }
    inline InstDsts InstDsts::compute(const Instruction &i)
    {
        InstDsts ids(*Model::LookupModel(i.platform()));
        ids.destinations.reset();
        ids.flagModifier.addFlagModifierOutputs(i);
        ids.destinations.addDestinationOutputs(i);
        return ids;
    }
} // namespace iga

#endif // _IGA_IR_REGSET_HPP

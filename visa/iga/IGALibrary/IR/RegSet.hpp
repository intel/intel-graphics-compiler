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

#ifndef _IGA_IR_REGSET_HPP
#define _IGA_IR_REGSET_HPP

// Future replacement for RegDeps(.cpp/.hpp)
// Need to decouple machine state logic from the dependency set.

#include "BitSet.hpp"
#include "Instruction.hpp"
#include "Kernel.hpp" // BlockList TODO: move to Blocks.hpp (elide Encoder def)

#include <ostream>


namespace iga
{
    struct RegSetInfo {
        RegName reg;
        const char *syntax;
        int numRegisters;
        int bytesPerRegister;
        int startOffset;
        int length;

        static const RegSetInfo *lookup(RegName rn);
        static const RegSetInfo *ALL[];
    };

    // TODO: merge this with RegInfo in Models.hpp
    //
    // schema for register files
    // NOTE: I have to replicate all the constants as scalars (not struct elements)
    // because the compiler refuses to accept that they are constant. :(
    //
    // #define REGSET_NEXTSTART(REG) \
        //  ALIGN_UP_TO(32,((REG).startOffset + (REG).numRegisters * (REG).bytesPerRegister))
#define RS_CREATE_SET(REGSYM,REGSYN,REGS,BPR,START) \
const static size_t RS_ ## REGSYM ##_REGS = (REGS), RS_ ## REGSYM ## _BPR = (BPR), RS_ ## REGSYM ## _START = (START); \
const static RegSetInfo RS_ ## REGSYM = {RegName::REGSYM, REGSYN, REGS, BPR, START, (RS_ ## REGSYM ## _REGS)*(RS_ ## REGSYM ## _BPR)}
#define RS_NEXT(FROMSYM) ALIGN_UP_TO(32,RS_ ## FROMSYM ## _START + (RS_ ## FROMSYM ## _REGS)*(RS_ ## FROMSYM ## _BPR))

    RS_CREATE_SET(GRF_R,     "r", 256, 32, 0);
    RS_CREATE_SET(ARF_A,    "a0",   1, 32, RS_NEXT(GRF_R));
    RS_CREATE_SET(ARF_ACC, "acc",   4, 32, RS_NEXT(ARF_A));
    // MMR: technically only a couple bits each,
    // but treat as full registers just for simplicity
    RS_CREATE_SET(ARF_MME, "mme",   8, 32, RS_NEXT(ARF_ACC));
    RS_CREATE_SET(ARF_F,     "f",   2,  4, RS_NEXT(ARF_MME));
    RS_CREATE_SET(ARF_CR,   "cr",   3,  4, RS_NEXT(ARF_F));
    RS_CREATE_SET(ARF_SR,   "sr",   3,  4, RS_NEXT(ARF_CR));
    RS_CREATE_SET(ARF_IP,   "ip",   1,  4, RS_NEXT(ARF_SR));
    RS_CREATE_SET(ARF_SP,   "sp",   1, 16, RS_NEXT(ARF_IP));
    const static size_t RS_MAX_BITS = RS_NEXT(ARF_SP);
#undef RS_CREATE_SET
#undef RS_NEXT

    inline const RegSetInfo *RegSetInfo::lookup(RegName rn)
    {
        switch (rn) {
        case RegName::GRF_R: return &RS_GRF_R;
        case RegName::ARF_A: return &RS_ARF_A;
        case RegName::ARF_ACC: return &RS_ARF_ACC;
        case RegName::ARF_F: return &RS_ARF_F;
        case RegName::ARF_CR: return &RS_ARF_CR;
        case RegName::ARF_SR: return &RS_ARF_SR;
        case RegName::ARF_IP: return &RS_ARF_IP;
        case RegName::ARF_SP: return &RS_ARF_SP;
        default: return nullptr;
        }
    }
    static size_t relativeAddressOf(
        const RegSetInfo &rsi, RegRef rr, size_t tySzBits)
    {
        return rsi.bytesPerRegister*rr.regNum + rr.subRegNum*tySzBits/8;
    }

    // A register set represent all the storage in the register files
    // at a certain granularity (currently typically byte).
    //
    struct RegSet
    {
        static RegSet emptySet() { RegSet rs; rs.reset(); return rs; }

        static bool addPredicationInputs(const Instruction &i, RegSet &rs);
        static bool addSourceInputs(const Instruction &i, RegSet &rs);
        static bool addFlagModifierOutputs(const Instruction &i, RegSet &rs);
        static bool addDestinationOutputs(const Instruction &i, RegSet &rs);
        static RegSet unionOf(RegSet r1, RegSet r2) {
            RegSet r12;
            r12.reset();
            (void)r12.destructiveUnion(r1);
            (void)r12.destructiveUnion(r2);
            return r12;
        }

        bool destructiveUnion(const RegSet &inp) { return bits.add(inp.bits); }
        bool destructiveSubtract(const RegSet &rhs) { return bits.andNot(rhs.bits); }
        bool intersectInto(const RegSet &rhs, RegSet &into) const {
            return bits.intersectInto(rhs.bits, into.bits);
        }
        bool add(const RegSetInfo &rs, size_t off, size_t len);
        bool addFullReg(const RegSetInfo &rs, int reg) {
            return add(rs, rs.bytesPerRegister*reg, rs.bytesPerRegister);
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

        bool        empty() const { return bits.empty(); }
        void        reset() { bits.reset(); }
        bool        intersects(const RegSet &rhs) const { return bits.intersects(rhs.bits); }
        void        str(std::ostream &os) const;
        std::string str() const;

        const BitSet<>& getBitSet() const { return bits; }
              BitSet<>& getBitSet()       { return bits; }

        bool operator==(const RegSet &rs) const { return bits == rs.bits; }
        bool operator!=(const RegSet &rs) const { return bits != rs.bits; }
    private:
        BitSet<> bits = BitSet<>(RS_MAX_BITS);
        void formatShortReg(
            std::ostream &os,
            bool &first,
            const char *reg_name,
            size_t reg_num,
            size_t reg_start,
            size_t reg_len) const;
    };


    struct InstSrcs
    {
        RegSet predication;
        RegSet sources;
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
        InstSrcs iss;
        iss.predication.reset();
        iss.sources.reset();
        RegSet::addPredicationInputs(i, iss.predication);
        RegSet::addSourceInputs(i, iss.sources);
        return iss;
    }
    inline InstDsts InstDsts::compute(const Instruction &i)
    {
        InstDsts ids;
        ids.flagModifier.reset();
        ids.destinations.reset();
        RegSet::addFlagModifierOutputs(i, ids.flagModifier);
        RegSet::addDestinationOutputs(i, ids.destinations);
        return ids;
    }
} // namespace iga

#endif // _IGA_IR_REGSET_HPP
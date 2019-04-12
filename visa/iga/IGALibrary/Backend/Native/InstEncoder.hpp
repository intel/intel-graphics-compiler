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
#ifndef IGA_BACKEND_NATIVE_INSTENCODER_HPP
#define IGA_BACKEND_NATIVE_INSTENCODER_HPP

#include "MInst.hpp"
#include "Field.hpp"
#include "../BitProcessor.hpp"
#include "../EncoderOpts.hpp"
#include "../../IR/Kernel.hpp"
#include "../../asserts.hpp"
#include "../../bits.hpp"
//
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
//
#ifdef _DEBUG
// IGA_VALIDATE_BITS adds extra structures and code to ensure that each bit
// the instruction encoded gets written at most once.  This can catch
// accidental field overlaps quite effectively.
//
// The neceessary assumption follows that encoders must not get lazy or sloppy
// and just clobber bits.  Set it once only.
#define IGA_VALIDATE_BITS
#endif


namespace iga
{
    // immLo <= offset < immHi
    static inline bool rangeContains(int immLo, int immHi, int offset) {
        return immLo <= offset && offset < immHi;
    }

    struct InstEncoderState
    {
        int                            instIndex;
        const Instruction             *inst;
#ifdef IGA_VALIDATE_BITS
        // All the bit fields set by some field during this instruction encoding
        // e.g. if a field with bits [127:96] is set to 00000000....0001b
        // this bit mask will contain 1111....111b's for that field
        // This allows us to detect writing to two overlapped fields, which
        // indicates an internal logical error in the encoder.
        MInst                          dirty;
        // This contains a list of all the fields we set during an instruction
        // encoding so we can run through the list and determine which fields
        // overlapped.
        std::vector<const Field *>    fieldsSet;
#endif
        InstEncoderState(
              int _instIndex
            , const Instruction *_inst
#ifdef IGA_VALIDATE_BITS
            , const MInst &_dirty
            , const std::vector<const Field*> &_fieldsSet
#endif
            )
            : instIndex(_instIndex)
            , inst(_inst)
#ifdef IGA_VALIDATE_BITS
            , dirty(_dirty)
            , fieldsSet(_fieldsSet)
#endif
        {
        }
        InstEncoderState() { }
    };

    struct Backpatch
    {
        InstEncoderState               state;
        const Block                   *target;
        const Field                   &field;
        enum Type {REL, ABS}           type;

        Backpatch(
            InstEncoderState &_state,
            const Block *_target,
            const Field &_field,
            Type _type = REL)
            : state(_state)
            , target(_target)
            , field(_field)
            , type(_type) { }
        // TODO: determine where copy construction used to eliminate
        // Backpatch(const Backpatch&) = delete;
    };
    typedef std::vector<Backpatch> BackpatchList;

    // can be passed into the encoding if we want to know the result
    struct CompactionDebugInfo {
        std::vector<Op>                      fieldOps; // the op we were trying to compact
        std::vector<const CompactedField *>  fieldMisses; // which indices missed
        std::vector<uint64_t>                fieldMapping; // what we tried to match (parallel)
    };
    enum CompactionResult {
        CR_SUCCESS,
        CR_NO_COMPACT, // {NoCompact} (or -Xnoautocompact and {}
        CR_NO_FORMAT, // e.g. send or jump
        CR_NO_AUTOCOMPACT, // not annotation and {Compacted} not set
        CR_MISS // fragment misses
    };

    //////////////////////////////////////////////////////////
    // generic encoder for all platforms
    // subclasses specialize the necessary functions
    class InstEncoder : public BitProcessor
    {
        // the encoder options (e.g. for auto-compaction)
        EncoderOpts                    opts;
        // The target bits to encode to
        MInst                         *bits = nullptr;
        // A backpatch list that we can add to.  A parent encoder is permitted
        // to copy or shadow this list.  Hence, this class may not assume the
        // state is preserved between the calls of encodeInstruction and
        // resolveBackpatch.
        BackpatchList                  backpatches;
        // state used by the encoder (can be saved for backpatches)
        InstEncoderState               state;
    public:
        InstEncoder(
            const EncoderOpts &_opts,
            BitProcessor &_parent)
            : BitProcessor(_parent)
            , opts(_opts) { }
        InstEncoder(const InstEncoder&) = delete;

        BackpatchList &getBackpatches() {return backpatches;}
        const OpSpec &getOpSpec() const {return state.inst->getOpSpec();}

//        void setBits(MInst mi) {*bits = mi;}
//        MInst getBits() const {return *bits;}

        ////////////////////////////////////////////////////////////////////////
        // external interface (called by parent encoder; e.g. SerialEncoder)  //
        ////////////////////////////////////////////////////////////////////////

        // Called externally by our parent encoder algorithm to start the encoding
        // of an instruction.  The instruction size is determined by the compaction
        // bit in the target instruction.
        template <Platform P>
        void encodeInstruction(
            int ix,
            const Instruction &i,
            MInst *_bits)
        {
            setCurrInst(&i);

            state.instIndex = ix;
            state.inst = &i;
#ifdef VALIDATE_BITS
            state.dirty.qw0 = 0;
            state.dirty.qw1 = 0;
            state.fieldsSet.clear();
#endif
            memset(_bits, 0, sizeof(*_bits));
            bits = _bits;
            encodeForPlatform<P>(i);
        }
        void resolveBackpatch(Backpatch &bp, MInst *_bits) {
            bits = _bits;
            state = bp.state;
            setCurrInst(bp.state.inst);
            if (bp.type == Backpatch::ABS) {
                encode(bp.field, bp.target->getPC());
            } else {
                encode(bp.field, bp.target->getPC() - bp.state.inst->getPC());
            }
        }

        //////////////////////////////////////////////////////
        // internal (to the encoder package, not private) instances of
        // encodeForPlatform<P> and their subtrees will call these methods.
        //////////////////////////////////////////////////////
        void registerBackpatch(
            const Field &f,
            const Block *b,
            Backpatch::Type t = Backpatch::Type::REL)
        {
            backpatches.emplace_back(state, b, f, t);
        }

        void encode(const Field &f, int32_t val) { encodeFieldBits(f, (uint32_t)val); }
        void encode(const Field &f, int64_t val) { encodeFieldBits(f, (uint64_t)val); }
        void encode(const Field &f, uint32_t val) { encodeFieldBits(f, (uint64_t)val); }
        void encode(const Field &f, uint64_t val) { encodeFieldBits(f, val); }
        void encode(const Field &f, bool val) { encodeFieldBits(f, val ? 1 : 0); }
        void encode(const Field &f, const Model &model, const OpSpec &os);
        void encode(const Field &f, ExecSize es);
        void encode(const Field &f, MathMacroExt acc);
        void encode(const Field &f, Region::Vert vt);
        void encode(const Field &f, Region::Width wi);
        void encode(const Field &f, Region::Horz hz);
        void encode(const Field &f, SrcModifier mods);
        template <OpIx IX>
        void encodeSubreg(
            const Field &f,
            RegName reg,
            RegRef rr,
            Type ty);
        void encodeReg(
            const Field &fREGFILE,
            const Field &fREG,
            RegName reg,
            int regNum);

        //////////////////////////////////////////////////////
        // Helper Functions
        //////////////////////////////////////////////////////
        void encodeFieldBits(const Field &f, uint64_t val) {
            checkFieldOverlaps(f);
#ifndef _DEBUG
            if (val == 0) { // short circuit since we start with zeros
                return;
            }
#endif
            uint64_t shiftedVal = val << f.offset % 64; // shift into position
            uint64_t mask = getFieldMask(f);
            if (shiftedVal & ~mask) {
                // either val has bits greater than what this field represents
                // or below the shift (e.g. smaller value than we accept)
                // e.g. Dst.Subreg[4:3] just has the high two bits of
                // the subregister and can't represent small values.
                //
                // Generally, this indicates an internal IGA problem:
                //  e.g. something we need to catch in the parser, IR checker or
                //       some other higher level (i.e. we're looking at bad IR)
                error("0x%x: value too large or too small for %s",
                    val, f.name);
                return;
            }
            bits->qws[f.offset / 64] |= shiftedVal;
        }

        uint64_t getFieldMask(const Field &f) const {
            return iga::getFieldMask<uint64_t>(f.offset, f.length);
        }
        uint64_t getFieldMaskUnshifted(const Field &f) const {
            return iga::getFieldMaskUnshifted<uint64_t>(f.length);
        }

        void encodingError(const std::string &msg) {
            encodingError("", msg);
        }
        void encodingError(const Field *f, const std::string &msg) {
            encodingError(f ? f->name : "", msg);
        }
        void encodingError(const Field &f, const std::string &msg) {
            encodingError(f.name, msg);
        }
        void encodingError(OpIx ix, const std::string &msg) {
            encodingError(ToStringOpIx(ix), msg);
        }
        void encodingError(const std::string &ctx, const std::string &msg) {
            if (ctx.empty()) {
                error(msg.c_str());
            } else {
                std::stringstream ss;
                ss << ctx << ": " << msg;
                error(ss.str().c_str());
            }
        }
        void internalErrorBadIR(const char *what) {
            error("INTERNAL ERROR: malformed IR: %s", what);
        }
        void internalErrorBadIR(const Field &f) {
            internalErrorBadIR(f.name);
        }


        template <typename T>
        void encodeEnum(const Field &f, T lo, T hi, T x) {
            if (x < lo || x > hi) {
                encodingError(f, "invalid value");
            }
            encodeFieldBits(f, static_cast<uint64_t>(x));
        }
    private:
#ifdef VALIDATE_BITS
        void checkFieldOverlaps(const Field &f); // real work
#else
        void checkFieldOverlaps(const Field &) { } // nop
#endif
    private:
        // instances get defined per platform
        template <Platform P> void encodeForPlatform(const Instruction &i);
    }; // end class InstEncoder


    inline void InstEncoder::encode(
        const Field &f, const Model &model, const OpSpec &os)
    {
        if (!os.isValid()) {
            encodingError(f, "invalid opcode");
        }
        encodeFieldBits(f, os.code);
        if (os.isSubop()) {
            int fcValue = os.functionControlValue;
            const OpSpec *parOp = model.lookupSubOpParent(os);
            IGA_ASSERT(parOp != nullptr, "cannot find SubOpParent");
            IGA_ASSERT(parOp->functionControlFields[0].length >= 0,
                "cannot find subop fields");
            for (int i = 0;
                i < sizeof(parOp->functionControlFields)/sizeof(parOp->functionControlFields[0]);
                i++)
            {
                if (parOp->functionControlFields[i].length == 0) {
                    break; // previous fragment was last
                }
                uint64_t fragMask = getFieldMaskUnshifted(parOp->functionControlFields[i]);
                encode(parOp->functionControlFields[i], fcValue & fragMask);
                fcValue >>= parOp->functionControlFields[i].length;
            }
        }
    }

#define ENCODING_CASE(X, V) case X: val = (V); break
    inline void InstEncoder::encode(const Field &f, ExecSize es)
    {
        uint64_t val = 0;
        switch (es) {
        ENCODING_CASE(ExecSize::SIMD1,  0);
        ENCODING_CASE(ExecSize::SIMD2,  1);
        ENCODING_CASE(ExecSize::SIMD4,  2);
        ENCODING_CASE(ExecSize::SIMD8,  3);
        ENCODING_CASE(ExecSize::SIMD16, 4);
        ENCODING_CASE(ExecSize::SIMD32, 5);
        default: internalErrorBadIR(f);
        }
        encodeFieldBits(f, val);
    }

    // encodes this as an math macro operand reference (e.g. r12.mme2)
    // *not* as an explicit operand (for context save and restore)
    inline void InstEncoder::encode(const Field &f, MathMacroExt mme) {
        uint64_t val = 0;
        switch (mme) {
        ENCODING_CASE(MathMacroExt::MME0, 0x0);
        ENCODING_CASE(MathMacroExt::MME1, 0x1);
        ENCODING_CASE(MathMacroExt::MME2, 0x2);
        ENCODING_CASE(MathMacroExt::MME3, 0x3);
        ENCODING_CASE(MathMacroExt::MME4, 0x4);
        ENCODING_CASE(MathMacroExt::MME5, 0x5);
        ENCODING_CASE(MathMacroExt::MME6, 0x6);
        ENCODING_CASE(MathMacroExt::MME7, 0x7);
        ENCODING_CASE(MathMacroExt::NOMME, 0x8);
        default: internalErrorBadIR(f);
        }
        encodeFieldBits(f, val);
    }

    inline void InstEncoder::encode(const Field &f, Region::Vert vt) {
        uint64_t val = 0;
        switch (vt) {
        ENCODING_CASE(Region::Vert::VT_0, 0);
        ENCODING_CASE(Region::Vert::VT_1, 1);
        ENCODING_CASE(Region::Vert::VT_2, 2);
        ENCODING_CASE(Region::Vert::VT_4, 3);
        ENCODING_CASE(Region::Vert::VT_8, 4);
        ENCODING_CASE(Region::Vert::VT_16, 5);
        ENCODING_CASE(Region::Vert::VT_32, 6);
        ENCODING_CASE(Region::Vert::VT_VxH, 0xF);
        default: internalErrorBadIR(f);
        }
        encodeFieldBits(f, val);
    }

    inline void InstEncoder::encode(const Field &f, Region::Width wi) {
        uint64_t val = 0;
        switch (wi) {
        ENCODING_CASE(Region::Width::WI_1, 0);
        ENCODING_CASE(Region::Width::WI_2, 1);
        ENCODING_CASE(Region::Width::WI_4, 2);
        ENCODING_CASE(Region::Width::WI_8, 3);
        ENCODING_CASE(Region::Width::WI_16, 4);
        default: internalErrorBadIR(f);
        }
        encodeFieldBits(f, val);
    }

    inline void InstEncoder::encode(const Field &f, Region::Horz hz) {
        uint64_t val = 0;
        switch (hz) {
        ENCODING_CASE(Region::Horz::HZ_0, 0);
        ENCODING_CASE(Region::Horz::HZ_1, 1);
        ENCODING_CASE(Region::Horz::HZ_2, 2);
        ENCODING_CASE(Region::Horz::HZ_4, 3);
        default: internalErrorBadIR(f);
        }
        encodeFieldBits(f, val);
    }

    inline void InstEncoder::encode(const Field &f, SrcModifier mods) {
        uint64_t val = 0;
        switch (mods) {
        ENCODING_CASE(SrcModifier::NONE, 0);
        ENCODING_CASE(SrcModifier::ABS, 1);
        ENCODING_CASE(SrcModifier::NEG, 2);
        ENCODING_CASE(SrcModifier::NEG_ABS, 3);
        default: internalErrorBadIR(f);
        }
        encodeFieldBits(f, val);
    }

    inline void InstEncoder::encodeReg(
        const Field &fREGFILE,
        const Field &fREG,
        RegName reg,
        int regNum)
    {
        uint64_t regFileVal = 0, val = 0; // regVal (ENCODING_CASE assumes "val")
        switch (reg) {
        ENCODING_CASE(RegName::ARF_NULL, 0);
        ENCODING_CASE(RegName::ARF_A,    1);
        ENCODING_CASE(RegName::ARF_ACC,  2);
        ENCODING_CASE(RegName::ARF_MME,  2);
        ENCODING_CASE(RegName::ARF_F,    3);
        ENCODING_CASE(RegName::ARF_CE,   4);
        ENCODING_CASE(RegName::ARF_MSG,  5);
        ENCODING_CASE(RegName::ARF_SP,   6);
        ENCODING_CASE(RegName::ARF_SR,   7);
        ENCODING_CASE(RegName::ARF_CR,   8);
        ENCODING_CASE(RegName::ARF_N,    9);
        ENCODING_CASE(RegName::ARF_IP,  10);
        ENCODING_CASE(RegName::ARF_TDR, 11);
        ENCODING_CASE(RegName::ARF_TM,  12);
        ENCODING_CASE(RegName::ARF_FC,  13);
        ENCODING_CASE(RegName::ARF_DBG, 15);
        case RegName::GRF_R:
            val = regNum;
            regFileVal = 1;
            break;
        default: internalErrorBadIR(fREG);
        }
        if (reg != RegName::GRF_R) {
            val = val << 4;
            val |= (regNum & 0xF);
        }
        encodeFieldBits(fREGFILE, regFileVal);
        encodeFieldBits(fREG, val);
    }

#undef ENCODING_CASE

    template <OpIx IX>
    inline void InstEncoder::encodeSubreg(
        const Field &f, RegName reg, RegRef rr, Type ty)
    {
        uint64_t val = (uint64_t)rr.subRegNum;
        // branches have implicit :d
        val = ty == Type::INVALID ? 4*val :
            SubRegToBytesOffset((int)val, reg, ty);
        if (IX == OpIx::TER_DST) {
            if (val & 0x7) {
                // Dst.SubReg[4:3]
                encodingError(f,
                    "field lacks the bits to encode this "
                    "subregister (low bits are implicitly 0)");
            }
            val >>= 3; // unscale
        }
        encodeFieldBits(f, val);
    }


    class InstCompactor : public BitProcessor {
        const OpSpec *os;

        MInst compactedBits;
        MInst uncompactedBits;
        CompactionDebugInfo *compactionDebugInfo;
        bool compactionMissed = false;

        // the compaction result (if compaction enabled)
        CompactionResult compactionResult = CompactionResult::CR_NO_COMPACT;

        // various platforms define this
        template <Platform P> CompactionResult tryToCompactImpl();
    public:
        InstCompactor(BitProcessor &_parent)
            : BitProcessor(_parent)
        {
        }

        template <Platform P> CompactionResult tryToCompact(
            const OpSpec *_os,
            MInst _uncompactedBits, // copy-in
            MInst *compactedBitsOutput, // output
            CompactionDebugInfo *cdbi)
        {
            os = _os;
            uncompactedBits = _uncompactedBits;
            compactedBits.qw0 = compactedBits.qw1 = 0;
            compactionDebugInfo = cdbi;

            auto cr = tryToCompactImpl<P>();
            if (cr == CompactionResult::CR_SUCCESS) { // only clobber their memory if we succeed
                *compactedBitsOutput = compactedBits;
            }
            return cr;
        }

        ///////////////////////////////////////////////////////////////////////
        // used by child implementations
        const OpSpec &getOpSpec() const {return *os;}
        uint64_t getUncompactedField(const Field &f) const {return uncompactedBits.getField(f);}
        void setCompactedField(const Field &f, uint64_t val) {
            compactedBits.setField(f, val);
        }
        bool getCompactionMissed() const {return compactionMissed;}
        void setCompactionResult(CompactionResult cr) {compactionResult = cr;}

        void transferField(const Field &compactedField, const Field &nativeField) {
            IGA_ASSERT(compactedField.length == nativeField.length,
                "native and compaction field length mismatch");
            compactedBits.setField(compactedField, uncompactedBits.getField(nativeField));
        }

        // returns false upon failure (so we can exit compaction early)
        //
        // the immLo and immHi values tells us the range of bits that hold
        // immediate value bits; the bounds are [lo,hi): inclusive/exclusive
        bool compactIndex(const CompactedField &ci, int immLo, int immHi) {
            // fragments are ordered from high bit down to 0
            // hence, we have to walk them in reverse order to build the
            // word in reverse order
            //
            // we build up the don't-care mask as we assemble the
            // desired mapping
            uint64_t relevantBits = 0xFFFFFFFFFFFFFFFFull;
            int indexOffset = 0; // offset into the compaction index value
            uint64_t mappedValue = 0;
            for (int i = (int)ci.numMappings - 1; i >= 0; i--) {
                const Field *mappedField = ci.mappings[i];
                if (rangeContains(immLo, immHi, mappedField->offset) ||
                    rangeContains(immLo, immHi, mappedField->offset + mappedField->length - 1))
                {
                    // this is a don't-care field since the imm value expands
                    // into this field; strip those bits out
                    relevantBits &= ~getFieldMask<uint64_t>(indexOffset,mappedField->length);
                } else {
                    // we omit the random bits in don't care fields to give us
                    // a normalized lookup value (for debugging)
                    // technically we can match any table entry for don't-care
                    // bits
                    uint64_t uncompactedValue = uncompactedBits.getField(*mappedField);
                    setBits(&mappedValue, indexOffset, mappedField->length, uncompactedValue);
                }
                indexOffset += mappedField->length;
            }

            // TODO: make lookup constant (could use a prefix tree or just a simple hash)
            for (size_t i = 0; i < ci.numValues; i++) {
                if ((ci.values[i] & relevantBits) == mappedValue) {
                    if (!compactedBits.setField(ci.index, (uint64_t)i)) {
                        IGA_ASSERT_FALSE("compaction index overruns field");
                    }
                    return true; // hit
                }
            }
            // compaction miss
            fail(ci, mappedValue);
            return compactionDebugInfo != nullptr;
        }
        bool compactIndex(const CompactedField &ci) {
            return compactIndex(ci, 0, 0);
        }

        void fail(const CompactedField &ci, uint64_t mappedValue) {
            if (compactionDebugInfo) {
                compactionDebugInfo->fieldOps.push_back(os->op);
                compactionDebugInfo->fieldMisses.push_back(&ci);
                compactionDebugInfo->fieldMapping.push_back(mappedValue);
            }
            compactionMissed = true;
        }
    };

} // end iga::*
#endif /* IGA_BACKEND_NATIVE_INSTENCODER_HPP */
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
#ifndef _IGA_BACKEND_GED_ENCODER_H_
#define _IGA_BACKEND_GED_ENCODER_H_

#include "EncoderCommon.hpp"
#include "GEDBitProcessor.hpp"
#include "IGAToGEDTranslation.hpp"
#include "../EncoderOpts.hpp"
#include "../../IR/Instruction.hpp"
#include "../../IR/Block.hpp"
#include "../../IR/Kernel.hpp"
#include "../../ErrorHandler.hpp"
#include "../../Timer/Timer.hpp"

#include <list>
#include <map>



namespace iga
{
    typedef list<Block *, std_arena_based_allocator<Block*>> BlockList;

    class EncoderBase : protected GEDBitProcessor
    {
    public:
        EncoderBase(
            const Model& model,
            ErrorHandler& errHandler,
            const EncoderOpts& eos);

        void encodeKernel(
            Kernel& k,
            MemManager &m,
            void*& bits,
            uint32_t& bitsLen);

        size_t getNumInstructionsEncoded() const;

        ///////////////////////////////////////////////////////////////////////
        // PROFILING API FOR TESTING (must be compiled into)
        //
        // compile with MEASURE_COMPILATION_TIME
        double getElapsedTimeMicros(unsigned int idx);
        int64_t getElapsedTimeTicks(unsigned int idx);
        std::string getTimerName(unsigned int idx);

    protected:

        // TODO: phase these out
                void encodeKernelPreProcess(Kernel &k);
        virtual void doEncodeKernelPreProcess(Kernel &k);
        virtual void encodeFC(const OpSpec &os);
        virtual void encodeTernaryInstruction(const Instruction& inst, GED_ACCESS_MODE accessMode);
                void encodeTernaryAlign1Instruction(const Instruction& inst);
                void encodeTernaryAlign16Instruction(const Instruction& inst);
        virtual void encodeSendInstructionProcessSFID(const OpSpec&);
        virtual void encodeSendDirectDestination(const Operand& dst);
        virtual void encodeSendDestinationDataType(const Operand& dst);
                void encodeOptionsThreadControl(const Instruction& inst);

    protected:
        void encodeDstReg(RegName regName, uint8_t regNum); // just the regnum

        void encodeImmVal(const ImmVal &val, Type type);

        template <SourceIndex S> void encodeSrcRegFile(GED_REG_FILE rf);
        template <SourceIndex S> void encodeSrcRegionVert(Region::Vert v);
        template <SourceIndex S> void encodeSrcType(Type t);
        template <SourceIndex S> void encodeSrcAddrMode(GED_ADDR_MODE x);
        template <SourceIndex S> void encodeSrcModifier(SrcModifier x);
        template <SourceIndex S> void encodeSrcSubRegNum(
            std::pair<bool, uint32_t> subReg, bool isTernaryOrBranch);

        template <SourceIndex S> void encodeSrcMathMacroReg(MathMacroExt a);

        template <SourceIndex S> void encodeSrcReg(RegName regName, uint8_t regNum);
        template <SourceIndex S> void encodeSrcAddrImm(int32_t addrImm);
        template <SourceIndex S> void encodeSrcAddrSubRegNum(uint32_t addrSubReg);
        template <SourceIndex S> void encodeSrcRegion(const Region& r);
        template <SourceIndex S> void encodeSrcRegionWidth(Region::Width w);
        template <SourceIndex S> void encodeSrcRepCtrl(GED_REP_CTRL rep);
        template <SourceIndex S> void encodeSrcChanSel(
            GED_SWIZZLE chSelX,
            GED_SWIZZLE chSelY,
            GED_SWIZZLE chSelZ,
            GED_SWIZZLE chSelW);

        template <SourceIndex S>
        void encodeTernarySourceAlign16(const Instruction& inst);
        void encodeTernaryDestinationAlign16(const Instruction& inst);

        template <SourceIndex S>
        void encodeTernarySourceAlign1(
            const Instruction& inst,
            GED_EXECUTION_DATA_TYPE execDataType);
        void encodeTernaryDestinationAlign1(
            const Instruction& inst,
            GED_EXECUTION_DATA_TYPE execDataType);

        virtual void encodeTernarySrcRegionVert(SourceIndex S, Region::Vert v);

        template <SourceIndex S>
        void encodeTernaryImmVal(const ImmVal &val, Type type);

        template <SourceIndex S> void encodeSrcRegionHorz(Region::Horz s);


        void handleGedError(int line, const char *setter, GED_RETURN_VALUE status);

        // state that is valid over instance life
        struct EncoderOpts                        m_opts;

        // state that is valid over encodeInst()
        ged_ins_t                                 m_gedInst;
        bool                                      m_encodeAlign16 = false;
        Op                                        m_opcode = Op::INVALID;
        size_t                                    m_numberInstructionsEncoded;

    private:
        void operator delete(void*foo, MemManager* m) { };
        void *operator new(size_t sz, MemManager* m) { return m->alloc(sz); };

        void encodeBlock(Block* blk);
        void encodeInstruction(Instruction& inst);
        void patchJumpOffsets();

        ///////////////////////////////////////////////////////////////////////
        // BASIC INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        void encodeBasicInstruction(const Instruction& inst, GED_ACCESS_MODE accessMode);
        void encodeBasicDestination(
            const Instruction& inst,
            const Operand& dst,
            GED_ACCESS_MODE accessMode = GED_ACCESS_MODE_Align1);
        template <SourceIndex S> void encodeBasicSource(
            const Instruction& inst,
            const Operand& src,
            GED_ACCESS_MODE accessMode = GED_ACCESS_MODE_Align1);
        ///////////////////////////////////////////////////////////////////////
        // BRANCH INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        void encodeBranchingInstruction(const Instruction& inst);
        void encodeBranchingInstructionSimplified(const Instruction& inst);
        void encodeBranchDestination(const Instruction& inst, const Operand& dst);
        void encodeBranchSource(const Instruction& inst, const Operand& src);
        ///////////////////////////////////////////////////////////////////////
        // SEND INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        void encodeSendInstruction(const Instruction& inst);
        void encodeSendSource0(const Operand& src);
        void encodeSendDestination(const Instruction& inst, const Operand& dst);
        void encodeSendsSource0(const Operand& src);
        void encodeSendsSource1(const Operand& src);
        void encodeSendsDestination(const Operand& dst);

        ///////////////////////////////////////////////////////////////////////
        // OTHER HELPER FUNCTIONS
        ///////////////////////////////////////////////////////////////////////
        void     setEncodedPC(Instruction *inst, int32_t encodePC);
        int32_t  getEncodedPC(const Instruction *inst) const;

        bool getBlockOffset(const Block *b, uint32_t &pc);

        // handles encoding ARF registers as well as the easy GRF case
        // caller actually pulls the trigger and encodes the bits, but this
        // call can raise the encoding error
        uint32_t         translateRegNum(int opIx, RegName reg, uint8_t regNum);
        uint32_t         mathMacroRegToBits(int src, MathMacroExt mme); // ChSel and SubReg
        GED_DST_CHAN_EN  mathMacroRegToChEn(MathMacroExt mme);

        void applyGedWorkarounds(const Kernel &k, size_t bitsLen);
        void encodeOptions(const Instruction& inst);

        // Translate from subRegNum to num represented in binary encoding.
        // Return std::pair<bool, uint32_t> the bool denotes if the given sub reg num is
        // aligned to binary offset representation
        std::pair<bool, uint32_t> subRegNumToBinNum(int subRegNum, RegName regName, Type type);
        void encodeDstSubRegNum(std::pair<bool, uint32_t> subReg, bool isTernaryOrBranch);

        //////////////////////////////////////////////////////////////////////
        // platform specific queries *but sometimes need the instruction too)
        //
        // GEN7p5 implicitly scales PC offsets by QW except for a few instructions
        bool arePcsInQWords(const OpSpec &os) const;
        // Older GENs need a <2;2,1> on the src operand.
        // Later GENs ignore the region completely
        bool callNeedsSrcRegion221(const Instruction &inst) const;

        /////////////////////////////////////////////////////////////
        // state valid over encodeKernel()
        MemManager                               *m_mem;
        uint8_t                                  *m_instBuf = nullptr; // the output bits
        struct JumpPatch { // JIP and UIP label patching
            Instruction    *inst; // the instruction
            ged_ins_t       gedInst; // the partially constructed GED instruction
            uint8_t        *bits; // where to encode it in the heap
            JumpPatch(Instruction *i, const ged_ins_t &gi, uint8_t *bs)
                : inst(i), gedInst(gi), bits(bs) { }
        };
        vector<JumpPatch>                         m_needToPatch;
        std::map<const Block *, int32_t>          m_blockToOffsetMap;
        std::map<const Instruction *, int32_t>    m_instPcs; // maps instruction ID to PC

    public:
        ////////////////////////////////////////////////////////////////
        static uint64_t typeConvesionHelper(const ImmVal &val, Type type)
        {
            uint64_t value = 0;
            switch (type) {
            case iga::Type::UD:
            case iga::Type::F:
            case iga::Type::V:
            case iga::Type::UV:
            case iga::Type::VF:
                value = (uint64_t)val.u32;
                break;
            case iga::Type::D:
                value = (uint64_t)val.s32;
                break;
            case iga::Type::W:
                value = (uint64_t)val.s16;
                break;
            case iga::Type::UW:
            case iga::Type::HF:
                value = (uint64_t)val.u16;
                break;
            case iga::Type::DF:
            case iga::Type::UQ:
            case iga::Type::Q:
                value = val.u64;
                break;
            case iga::Type::B:
            case iga::Type::UB:
                // technically not reachable since we don't permit byte moves
                // from immediates
                value = val.u64;
                break;
            default:
                break;
            }

            return value;
        }

    protected:
        ////////////////////////////////////////////////////////////////
        // allowable types for ternary Align1
        static bool isTernaryAlign1Floating(Type t) {
            switch (t) {
            case Type::HF:
            case Type::F:
            case Type::DF:
            case Type::NF:
                return true;
            default:
                return false;
            }
        }

        // allowable types for ternary Align1

        static bool isTernaryAlign1Integral(Type t) {
            switch (t) {
            case Type::UQ: // technically uq not allows today, but maybe in future
            case Type::Q: // same as :uq
            case Type::UD:
            case Type::D:
            case Type::UW:
            case Type::W:
            case Type::UB:
            case Type::B:
                return true;
            default:
                return false;
            }
        }


    }; //end: class definition EncoderBase

    template <SourceIndex S> void EncoderBase::encodeSrcRegFile(GED_REG_FILE rf) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0RegFile, rf);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1RegFile, rf);
        } else {
            GED_ENCODE(Src2RegFile, rf);
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegionVert(Region::Vert v) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0VertStride, IGAToGEDTranslation::lowerRegionVert(v));
        } else { // (S == SourceIndex::SRC1)
            GED_ENCODE(Src1VertStride, IGAToGEDTranslation::lowerRegionVert(v));
        } // S != SRC2 since ternary Align1 doesn't have bits for that
    }

    template <SourceIndex S> void EncoderBase::encodeSrcType(Type t) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0DataType, IGAToGEDTranslation::lowerDataType(t));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1DataType, IGAToGEDTranslation::lowerDataType(t));
        } else {
            GED_ENCODE(Src2DataType, IGAToGEDTranslation::lowerDataType(t));
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcAddrMode(GED_ADDR_MODE x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrMode, x);
        } else {
            GED_ENCODE(Src1AddrMode, x);
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcModifier(SrcModifier x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SrcMod, IGAToGEDTranslation::lowerSrcMod(x));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SrcMod, IGAToGEDTranslation::lowerSrcMod(x));
        } else {
            GED_ENCODE(Src2SrcMod, IGAToGEDTranslation::lowerSrcMod(x));
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcSubRegNum(
        std::pair<bool, uint32_t> subReg, bool isTernaryOrBranch) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SubRegNum, subReg.second);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SubRegNum, subReg.second);
        } else {
            GED_ENCODE(Src2SubRegNum, subReg.second);
        }
    }

    template <SourceIndex S>
    void EncoderBase::encodeSrcMathMacroReg(MathMacroExt a)
    {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0MathMacroExt, IGAToGEDTranslation::lowerSpecialAcc(a));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1MathMacroExt, IGAToGEDTranslation::lowerSpecialAcc(a));
        } else {
            GED_ENCODE(Src2MathMacroExt, IGAToGEDTranslation::lowerSpecialAcc(a));
        }
    }
    template <SourceIndex S> void EncoderBase::encodeSrcReg(
        RegName regName,
        uint8_t regNum)
    {
        uint32_t regBits = 0;
        if (regName == RegName::GRF_R) {
            regBits = regNum; // GRF fast path
        } else { // ARF slower path
            const RegInfo *ri = m_model.lookupRegInfoByRegName(regName);
            if (!ri) {
                error("src%d: unexpected register on this platform", (int)S);
            } else {
                ri->encode((int)regNum,regNum);
                regBits = regNum; // widen for GED
            }
        }
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0RegNum, regBits);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1RegNum, regBits);
        } else {
            GED_ENCODE(Src2RegNum, regBits);
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcAddrImm(int32_t addrImm) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrImm, addrImm);
        } else {
            GED_ENCODE(Src1AddrImm, addrImm);
        }
    }
    template <SourceIndex S> void EncoderBase::encodeSrcAddrSubRegNum(uint32_t addrSubReg) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrSubRegNum, addrSubReg);
        } else {
            GED_ENCODE(Src1AddrSubRegNum, addrSubReg);
        }
    }
    template <SourceIndex S>
    void EncoderBase::encodeSrcRegion(const Region &rgn)
    {
        uint32_t v = 0;
        if (rgn.getVt() == Region::Vert::VT_VxH) {
            v = 0x3;
        } else if (rgn.getVt() != Region::Vert::VT_INVALID) {
            v = static_cast<uint32_t>(rgn.v);
        } else {
            error(S == SourceIndex::SRC0 ?
                "invalid region vertical stride on src0" :
                "invalid region vertical stride on src1");
        }

        uint32_t w = static_cast<uint32_t>(rgn.getWi());
        if (rgn.getWi() == Region::Width::WI_INVALID) {
            error(S == SourceIndex::SRC0 ?
                "invalid region width on src0" :
                "invalid region width on src1");
        }

        uint32_t h = static_cast<uint32_t>(rgn.getHz());
        if (rgn.getHz() == Region::Horz::HZ_INVALID) {
            h = 1;
            error(S == SourceIndex::SRC0 ?
                "invalid region horizontal stride on src0" :
                "invalid region horizontal stride on src1");
        }

        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0VertStride, v);
            GED_ENCODE(Src0Width, w);
            GED_ENCODE(Src0HorzStride, h);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1VertStride, v);
            GED_ENCODE(Src1Width, w);
            GED_ENCODE(Src1HorzStride, h);
        } else {
            IGA_ASSERT_FALSE("EncoderBase::encodeSrcRegion only works on src0 and src1");
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegionWidth(Region::Width w) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0Width, IGAToGEDTranslation::lowerRegionWidth(w));
        } else { // (S == SourceIndex::SRC1)
            GED_ENCODE(Src1Width, IGAToGEDTranslation::lowerRegionWidth(w));
        } // S != SRC2 since ternary Align1 doesn't have bits for that
    }

    template <SourceIndex S>
    void EncoderBase::encodeTernaryImmVal(const ImmVal &val, Type type) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0TernaryImm, typeConvesionHelper(val, type));
        } else if (S == SourceIndex::SRC2) {
            GED_ENCODE(Src2TernaryImm, typeConvesionHelper(val, type));
        } else {
            error("Immediate is not supported in src1 of Ternary instruction.");
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegionHorz(Region::Horz s) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0HorzStride, IGAToGEDTranslation::lowerRegionHorz(s));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1HorzStride, IGAToGEDTranslation::lowerRegionHorz(s));
        } else {
            GED_ENCODE(Src2HorzStride, IGAToGEDTranslation::lowerRegionHorz(s));
        }
    }
} // end: namespace iga*

namespace iga
{
    typedef EncoderBase Encoder;
}

#endif

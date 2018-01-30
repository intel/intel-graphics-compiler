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
#ifndef _IGA_ENCODER_H_
#define _IGA_ENCODER_H_

#include "../EncoderOpts.hpp"
#include "../BitProcessor.hpp"
#include "../../IR/Instruction.hpp"
#include "../../IR/Block.hpp"
#include "../../IR/Kernel.hpp"
#include "../../ErrorHandler.hpp"
#include "../../Timer/Timer.hpp"
#include "ged.h"

#include <list>
#include <map>
#include "IGAToGEDTranslation.hpp"

#include "EncoderCommon.hpp"


namespace iga
{
    typedef list<Block *, std_arena_based_allocator<Block*>> BlockList;

    class EncoderBase : protected BitProcessor
    {
    public:
        EncoderBase(
            const Model& model,
            ErrorHandler& errHandler,
            const EncoderOpts& eos = EncoderOpts());

        void encodeKernel(
            const Kernel& k,
            MemManager &m,
            void*& bits,
            uint32_t& bitsLen);
        double getElapsedTimeUS(unsigned int idx);
        int64_t getElapsedTimeTicks(unsigned int idx);
        std::string getTimerName(unsigned int idx);
        size_t getNumInstructionsEncoded();

    protected:
        virtual void encodeKernelPreProcess(const Kernel &k);
        virtual void encodeFC(const OpSpec &os);
        virtual bool hasImm64Src0Overlap(const OpSpec &os, Instruction& inst);
        virtual void encodeBasicInstructionNonDest(const OpSpec&);
        virtual bool encodeBasicInstructionNoSrcsToProcess(const Instruction& inst);
        virtual void encodeTernaryInstruction(const Instruction& inst, GED_ACCESS_MODE accessMode);
        virtual void encodeSendInstructionProcessSFID(const OpSpec&);
        virtual void encodeSendDirectDestination(const Operand& dst);
        virtual void encodeSendDestinationDataType(const Operand& dst);
        virtual void encodeSendInstructionPostProcess(const Instruction& inst);
        virtual void encodeOptionsPostProcess(const Instruction& inst);
        void encodeOptionsThreadControl(const Instruction& inst);
        virtual bool isPostIncrementJmpi(const Instruction &inst) const;
        virtual void encodeTernaryDestinationAlign1(const Instruction& inst);


    protected:
        void encodeImmVal(const ImmVal &val, Type type);

        template <SourceIndex S> void encodeSrcRegFile(GED_REG_FILE rf);
        template <SourceIndex S> void encodeSrcRegionVert(Region::Vert v);
        template <SourceIndex S> void encodeSrcType(Type t);
        template <SourceIndex S> void encodeSrcAddrMode(GED_ADDR_MODE x);
        template <SourceIndex S> void encodeSrcModifier(SrcModifier x);
        template <SourceIndex S> void encodeSrcSubRegNum(uint32_t subReg);

        template <SourceIndex S>
        void encodeSrcImplAcc(ImplAcc a);

        template <SourceIndex S> void encodeSrcRegNum(uint32_t reg);
        template <SourceIndex S> void encodeSrcAddrImm(int32_t x);
        template <SourceIndex S> void encodeSrcAddrSubRegNum(uint32_t x);
        template <SourceIndex S> void encodeSrcRegion(const Region& r);
        template <SourceIndex S> void encodeSrcRegionWidth(Region::Width w);
        template <SourceIndex S> void encodeSrcRepCtrl(GED_REP_CTRL rep);
        template <SourceIndex S> void encodeSrcChanSel(uint32_t chSel);

        template <SourceIndex S>
        void encodeTernarySourceAlign16(const Instruction& inst);
        void encodeTernaryDestinationAlign16(const Instruction& inst);

        template <SourceIndex S>
        void encodeTernarySourceAlign1(const Instruction& inst);

        virtual void encodeTernarySrcRegionVert(SourceIndex S, Region::Vert v);

        template <SourceIndex S>
        void encodeTernaryImmVal(const ImmVal &val, Type type);

        template <SourceIndex S> void encodeSrcRegionHorz(Region::Horz s);


        void handleGedError(int line, const char *setter, GED_RETURN_VALUE status);

        // state that is valid over instance life
        const Model&                              m_model;
        struct EncoderOpts                        m_opts;

        // state that is valid over encodeInst()
        ged_ins_t                                 m_gedInst;
        bool                                      m_encodeAlign16;
        Op                                        m_opcode;
        size_t                                    m_numberInstructionsEncoded;

    private:
        void operator delete(void*foo, MemManager* m) { };
        void *operator new(size_t sz, MemManager* m) { return m->alloc(sz); };

        void encodeBlock(Block *blk);
        void encodeInstruction(Instruction& inst);
        void patchJumpOffsets();

        void encodeBasicInstruction(const Instruction& inst, GED_ACCESS_MODE accessMode);
        void encodeBranchingInstruction(const Instruction& inst);
        void encodeBranchingInstructionSimplified(const Instruction& inst);

        void encodeSendInstruction(const Instruction& inst);

        void     setEncodedPC(Instruction *inst, int32_t encodePC);
        int32_t  getEncodedPC(const Instruction *inst) const;

        bool getBlockOffset(const Block *b, uint32_t &pc);

        void encodeBasicDestination(
            const Instruction& inst,
            const Operand& dst,
            GED_ACCESS_MODE accessMode = GED_ACCESS_MODE_Align1);

        void encodeBasicControlFlowDestination(
            const Instruction& inst,
            const Operand& dst);

        template <SourceIndex S> void encodeBasicSource(
            const Instruction& inst,
            const Operand& src,
            GED_ACCESS_MODE accessMode = GED_ACCESS_MODE_Align1);
        void encodeBasicControlFlowSource(
            const Instruction& inst,
            const Operand& src);

        void encodeSendSource0(const Operand& src);
        void encodeSendDestination(const Instruction& inst, const Operand& dst);

        void encodeSendsSource0(const Operand& src);
        void encodeSendsSource1(const Operand& src);
        void encodeSendsDestination(const Operand& dst);

        uint32_t         implAccToBits(int src, ImplAcc implAcc); // ChSel and SubReg
        GED_DST_CHAN_EN  implAccToChEn(ImplAcc implAcc);

        void applyGedWorkarounds(const Kernel &k, size_t bitsLen);
        void encodeOptions(const Instruction& inst);

        /////////////////////////////////////////////////////////////
        // platform specific queries
        bool isSpecialContextSaveAndRestore(const Operand& op) const;
        bool arePcsInQWords(const Instruction &inst) const;
        bool hasSimplifiedBranchEncoding() const;
        bool callNeedsSrcRegion221(const Instruction &inst) const;
        bool srcIsImm(const Operand &op) const;

        /////////////////////////////////////////////////////////////
        // state valid over encodeKernel()
        MemManager                               *m_mem;
        uint8_t                                  *m_instBuf; // the output bits
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

    protected:
        ////////////////////////////////////////////////////////////////
        uint64_t typeConvesionHelper(const ImmVal &val, Type type)
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
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1RegFile, rf);
        }
        else {
            GED_ENCODE(Src2RegFile, rf);
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegionVert(Region::Vert v) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0VertStride, IGAToGEDTranslation::lowerRegionVert(v));
        }
        else { // (S == SourceIndex::SRC1)
            GED_ENCODE(Src1VertStride, IGAToGEDTranslation::lowerRegionVert(v));
        } // S != SRC2 since ternary Align1 doesn't have bits for that
    }

    template <SourceIndex S> void EncoderBase::encodeSrcType(Type t) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0DataType, IGAToGEDTranslation::lowerDataType(t));
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1DataType, IGAToGEDTranslation::lowerDataType(t));
        }
        else {
            GED_ENCODE(Src2DataType, IGAToGEDTranslation::lowerDataType(t));
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcAddrMode(GED_ADDR_MODE x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrMode, x);
        }
        else {
            GED_ENCODE(Src1AddrMode, x);
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcModifier(SrcModifier x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SrcMod, IGAToGEDTranslation::lowerSrcMod(x));
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SrcMod, IGAToGEDTranslation::lowerSrcMod(x));
        }
        else {
            GED_ENCODE(Src2SrcMod, IGAToGEDTranslation::lowerSrcMod(x));
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcSubRegNum(uint32_t subReg) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SubRegNum, subReg);
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SubRegNum, subReg);
        }
        else {
            GED_ENCODE(Src2SubRegNum, subReg);
        }
    }

    template <SourceIndex S>
    void EncoderBase::encodeSrcImplAcc(ImplAcc a)
    {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SpecialAcc, IGAToGEDTranslation::lowerSpecialAcc(a));
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SpecialAcc, IGAToGEDTranslation::lowerSpecialAcc(a));
        }
        else {
            GED_ENCODE(Src2SpecialAcc, IGAToGEDTranslation::lowerSpecialAcc(a));
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegNum(uint32_t reg) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0RegNum, reg);
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1RegNum, reg);
        }
        else {
            GED_ENCODE(Src2RegNum, reg);
        }
    }
    template <SourceIndex S> void EncoderBase::encodeSrcAddrImm(int32_t x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrImm, x);
        }
        else {
            GED_ENCODE(Src1AddrImm, x);
        }
    }
    template <SourceIndex S> void EncoderBase::encodeSrcAddrSubRegNum(uint32_t x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrSubRegNum, x);
        }
        else {
            GED_ENCODE(Src1AddrSubRegNum, x);
        }
    }
    template <SourceIndex S>
    void EncoderBase::encodeSrcRegion(const Region &rgn)
    {
        uint32_t v = 0;
        if (rgn.getVt() == Region::Vert::VT_VxH) {
            v = 0x3;
        }
        else if (rgn.getVt() != Region::Vert::VT_INVALID) {
            v = static_cast<uint32_t>(rgn.v);
        }
        else {
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
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1VertStride, v);
            GED_ENCODE(Src1Width, w);
            GED_ENCODE(Src1HorzStride, h);
        }
        else {
            IGA_ASSERT_FALSE("EncoderBase::encodeSrcRegion only works on src0 and src1");
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegionWidth(Region::Width w) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0Width, IGAToGEDTranslation::lowerRegionWidth(w));
        }
        else { // (S == SourceIndex::SRC1)
            GED_ENCODE(Src1Width, IGAToGEDTranslation::lowerRegionWidth(w));
        } // S != SRC2 since ternary Align1 doesn't have bits for that
    }

    template <SourceIndex S>
    void EncoderBase::encodeTernaryImmVal(const ImmVal &val, Type type) {
        if (S == SourceIndex::SRC0)
        {
            GED_ENCODE(Src0TernaryImm, typeConvesionHelper(val, type));
        }
        else if (S == SourceIndex::SRC2)
        {
            GED_ENCODE(Src2TernaryImm, typeConvesionHelper(val, type));
        }
        else
        {
            error("Immediate is not supported in src1 of Ternary instruction.");
        }
    }

    template <SourceIndex S> void EncoderBase::encodeSrcRegionHorz(Region::Horz s) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0HorzStride, IGAToGEDTranslation::lowerRegionHorz(s));
        }
        else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1HorzStride, IGAToGEDTranslation::lowerRegionHorz(s));
        }
        else {
            GED_ENCODE(Src2HorzStride, IGAToGEDTranslation::lowerRegionHorz(s));
        }
    }


} // end: namespace iga*

namespace iga
{
    typedef EncoderBase Encoder;
}

#endif

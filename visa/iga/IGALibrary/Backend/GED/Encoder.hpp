/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BACKEND_GED_ENCODER_H_
#define _IGA_BACKEND_GED_ENCODER_H_

#include "GEDBitProcessor.hpp"
#include "IGAToGEDTranslation.hpp"
#include "../EncoderOpts.hpp"
#include "../../ErrorHandler.hpp"
#include "../../IR/Instruction.hpp"
#include "../../IR/Block.hpp"
#include "../../IR/Kernel.hpp"
#include "../../Timer/Timer.hpp"

#include <list>
#include <map>
#include <vector>


namespace iga
{
#define GED_ENCODE(FUNC, ARG) \
    GED_ENCODE_TO(FUNC, ARG, &m_gedInst)
#if defined(GED_TIMER) || defined(_DEBUG)
#define START_GED_TIMER() startIGATimer(TIMER_GED)
#define STOP_GED_TIMER()  stopIGATimer(TIMER_GED)
#else
#define START_GED_TIMER()
#define STOP_GED_TIMER()
#endif

#if defined(TOTAL_ENCODE_TIMER) || defined(_DEBUG)
#define START_ENCODER_TIMER() startIGATimer(TIMER_TOTAL)
#define STOP_ENCODER_TIMER()  stopIGATimer(TIMER_TOTAL)
#else
#define START_ENCODER_TIMER()
#define STOP_ENCODER_TIMER()
#endif

// uncomment to emit all the GED calls
// #define TRACE_GED_CALLS

#ifdef TRACE_GED_CALLS
#define TRACE_GED_SETTER(FIELD, ARG, STATUS) \
     std::cout << "Encoder.cpp:" << __LINE__ << ": GED_Set" << #FIELD << \
        "(..," << (ARG) << ") results in " << (STATUS) << "\n"
#else
#define TRACE_GED_SETTER(FIELD, ARG, STATUS)
#endif


#define GED_ENCODE_TO(FIELD, ARG, GED) \
    do { \
        GED_RETURN_VALUE _status; \
        START_GED_TIMER(); \
        _status = GED_Set ## FIELD (GED, ARG); \
        STOP_GED_TIMER(); \
        TRACE_GED_SETTER(FIELD, ARG, _status); \
        if (_status != GED_RETURN_VALUE_SUCCESS) { \
            handleGedError(__LINE__, #FIELD, _status); \
        } \
    } while (0)


    using BlockList = std::list<Block *, std_arena_based_allocator<Block*>>;

    class Encoder : protected GEDBitProcessor
    {
    public:
        Encoder(const Model& model, ErrorHandler& eh, const EncoderOpts& eos);

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
        void doEncodeKernelPreProcess(Kernel &k);
        void encodeFC(const Instruction &i);
        void encodeTernaryInstruction(const Instruction& inst, GED_ACCESS_MODE accessMode);
        void encodeTernaryAlign1Instruction(const Instruction& inst);
        void encodeTernaryAlign16Instruction(const Instruction& inst);
        void encodeSendDirectDestination(const Operand& dst);
        void encodeSendDestinationDataType(const Operand& dst);
        void encodeOptionsThreadControl(const Instruction& inst);

    protected:
        void encodeDstReg(RegName regName, uint16_t regNum); // just the regnum

        void encodeImmVal(const ImmVal &val, Type type);

        template <SourceIndex S> void encodeSrcRegFile(GED_REG_FILE rf);
        template <SourceIndex S> void encodeSrcRegionVert(Region::Vert v);
        template <SourceIndex S> void encodeSrcType(Type t);
        template <SourceIndex S> void encodeSrcAddrMode(GED_ADDR_MODE x);
        template <SourceIndex S> void encodeSrcModifier(SrcModifier x);
        template <SourceIndex S> void encodeSrcSubRegNum(uint32_t subRegInByte);

        template <SourceIndex S> void encodeSrcMathMacroReg(MathMacroExt a);

        template <SourceIndex S> void encodeSrcReg(RegName regName, uint16_t regNum);
        template <SourceIndex S> void encodeSrcAddrImm(int32_t addrImm);
        template <SourceIndex S> void encodeSrcAddrSubRegNum(uint32_t addrSubReg);
        template <SourceIndex S> void encodeSrcRegion(const Region& r, bool hasRgnWi = true);
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
        void encodeTernarySourceAlign1(const Instruction& inst);
        void encodeTernaryDestinationAlign1(const Instruction& inst);

        void encodeTernarySrcRegionVert(SourceIndex S, Region::Vert v);

        template <SourceIndex S>
        void encodeTernaryImmVal(const ImmVal &val, Type type);

        template <SourceIndex S> void encodeSrcRegionHorz(Region::Horz s);


        void handleGedError(int line, const char *setter, GED_RETURN_VALUE status);

        // state that is valid over instance life
        EncoderOpts                               m_opts;

        // state that is valid over encodeInst()
        ged_ins_t                                 m_gedInst;
        bool                                      m_encodeAlign16 = false;
        Op                                        m_opcode = Op::INVALID;
        size_t                                    m_numberInstructionsEncoded;

    private:
        void operator delete(void*, MemManager*) { };
        void *operator new(size_t sz, MemManager* m) {return m->alloc(sz);};

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
        void encodeBranchDestination(const Operand& dst);
        void encodeBranchSource(const Operand& src);
        ///////////////////////////////////////////////////////////////////////
        // SEND INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        void encodeSendInstruction(const Instruction& inst);
        void encodeSendSource0(const Operand& src);
        void encodeSendDestination(const Operand& dst);
        void encodeSendsSource0(const Operand& src);
        void encodeSendsSource1(const Operand& src);
        void encodeSendsDestination(const Operand& dst);

        void encodeSendDescs(const Instruction& inst);
        void encodeSendDescsPreXe(const Instruction& inst);
        void encodeSendDescsXe(const Instruction& inst);
        void encodeSendDescsXeHP(const Instruction& inst);
        void encodeSendDescsXeHPG(const Instruction& inst);

        ///////////////////////////////////////////////////////////////////////
        // SYNC INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        void encodeSyncInstruction(const Instruction& inst);

        ///////////////////////////////////////////////////////////////////////
        // OTHER HELPER FUNCTIONS
        ///////////////////////////////////////////////////////////////////////
        void     setEncodedPC(Instruction *inst, int32_t encodePC);
        int32_t  getEncodedPC(const Instruction *inst) const;

        bool getBlockOffset(const Block *b, uint32_t &pc);

        // handles encoding ARF registers as well as the easy GRF case
        // caller actually pulls the trigger and encodes the bits, but this
        // call can raise the encoding error
        uint32_t         translateRegNum(int opIx, RegName reg, uint16_t regNum);
        uint32_t         mathMacroRegToBits(int src, MathMacroExt mme); // ChSel and SubReg
        GED_DST_CHAN_EN  mathMacroRegToChEn(MathMacroExt mme);

        void applyGedWorkarounds(const Kernel &k, size_t bitsLen);
        void encodeOptions(const Instruction& inst);

        //////////////////////////////////////////////////////////////////////
        // platform specific queries *but sometimes need the instruction too)
        //
        // GEN7p5 implicitly scales PC offsets by QW except for a few instructions
        bool arePcsInQWords(const OpSpec &os) const;

        // Call need to have src0 region be set to:
        // SKL and before: <2;2,1>
        // ICL: <2;4,1>
        // Later GENs ignore the region completely
        bool callNeedsSrc0Region221(const Instruction &inst) const;
        bool callNeedsSrc0Region241(const Instruction &inst) const;

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
        std::vector<JumpPatch>                    m_needToPatch;
        std::map<const Block *, int32_t>          m_blockToOffsetMap;

    public:
        ////////////////////////////////////////////////////////////////
        static uint64_t typeConvesionHelper(const ImmVal &val, Type type)
        {
            uint64_t value = 0;
            switch (type) {
            case Type::UD:
            case Type::F:
            case Type::V:
            case Type::UV:
            case Type::VF:
                value = (uint64_t)val.u32;
                break;
            case Type::D:
                value = (uint64_t)val.s32;
                break;
            case Type::W:
                value = (uint64_t)val.s16;
                break;
            case Type::UW:
            case Type::HF:
            case Type::BF:
                value = (uint64_t)val.u16;
                break;
            case Type::DF:
            case Type::UQ:
            case Type::Q:
                value = val.u64;
                break;
            case Type::B:
            case Type::UB:
                // technically not reachable since we don't permit byte moves
                // from immediates
                value = val.u64;
                break;
            case Type::BF8:
                value = (uint64_t)val.u8;
                break;
            case Type::TF32:
                value = (uint64_t)val.u32;
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
            case Type::BF:
            case Type::BF8:
            case Type::TF32:
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


    }; //end: class definition Encoder

    template <SourceIndex S> void Encoder::encodeSrcRegFile(GED_REG_FILE rf) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0RegFile, rf);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1RegFile, rf);
        } else {
            GED_ENCODE(Src2RegFile, rf);
        }
    }

    template <SourceIndex S> void Encoder::encodeSrcRegionVert(Region::Vert v) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0VertStride, lowerRegionVert(v));
        } else { // (S == SourceIndex::SRC1)
            GED_ENCODE(Src1VertStride, lowerRegionVert(v));
        } // S != SRC2 since ternary Align1 doesn't have bits for that
    }

    template <SourceIndex S> void Encoder::encodeSrcType(Type t) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0DataType, lowerDataType(t));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1DataType, lowerDataType(t));
        } else {
            GED_ENCODE(Src2DataType, lowerDataType(t));
        }
    }

    template <SourceIndex S> void Encoder::encodeSrcAddrMode(GED_ADDR_MODE x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrMode, x);
        } else {
            GED_ENCODE(Src1AddrMode, x);
        }
    }

    template <SourceIndex S> void Encoder::encodeSrcModifier(SrcModifier x) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SrcMod, lowerSrcMod(x));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SrcMod, lowerSrcMod(x));
        } else {
            GED_ENCODE(Src2SrcMod, lowerSrcMod(x));
        }
    }

    template <SourceIndex S> void Encoder::encodeSrcSubRegNum(uint32_t subRegInByte) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0SubRegNum, subRegInByte);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1SubRegNum, subRegInByte);
        } else {
            GED_ENCODE(Src2SubRegNum, subRegInByte);
        }
    }

    template <SourceIndex S>
    void Encoder::encodeSrcMathMacroReg(MathMacroExt a)
    {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0MathMacroExt, lowerSpecialAcc(a));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1MathMacroExt, lowerSpecialAcc(a));
        } else {
            GED_ENCODE(Src2MathMacroExt, lowerSpecialAcc(a));
        }
    }
    template <SourceIndex S> void Encoder::encodeSrcReg(
        RegName regName,
        uint16_t regNum)
    {
        uint32_t regBits = 0;
        if (regName == RegName::GRF_R) {
            regBits = regNum; // GRF fast path
        } else { // ARF slower path
            const RegInfo *ri = m_model.lookupRegInfoByRegName(regName);
            if (!ri) {
                errorT("src", (int)S, ": unexpected register on this platform");
            } else {
                uint8_t reg8;
                ri->encode((int)regNum, reg8);
                regBits = reg8; // widen for GED
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

    template <SourceIndex S> void Encoder::encodeSrcAddrImm(int32_t addrImm) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrImm, addrImm);
        } else {
            GED_ENCODE(Src1AddrImm, addrImm);
        }
    }
    template <SourceIndex S>
    void Encoder::encodeSrcAddrSubRegNum(uint32_t addrSubReg) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0AddrSubRegNum, addrSubReg);
        } else {
            GED_ENCODE(Src1AddrSubRegNum, addrSubReg);
        }
    }
    template <SourceIndex S>
    void Encoder::encodeSrcRegion(const Region &rgn, bool hasRgnWi) {
        uint32_t v = 0;
        if (rgn.getVt() == Region::Vert::VT_VxH) {
            v = 0x3;
        } else if (rgn.getVt() != Region::Vert::VT_INVALID) {
            v = static_cast<uint32_t>(rgn.v);
        } else {
            errorT(S == SourceIndex::SRC0 ?
                "invalid region vertical stride on src0" :
                "invalid region vertical stride on src1");
        }

        uint32_t w = static_cast<uint32_t>(rgn.getWi());
        if (rgn.getWi() == Region::Width::WI_INVALID) {
            errorT(S == SourceIndex::SRC0 ?
                "invalid region width on src0" :
                "invalid region width on src1");
        }

        uint32_t h = static_cast<uint32_t>(rgn.getHz());
        if (rgn.getHz() == Region::Horz::HZ_INVALID) {
            h = 1;
            errorT(S == SourceIndex::SRC0 ?
                "invalid region horizontal stride on src0" :
                "invalid region horizontal stride on src1");
        }

        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0VertStride, v);
            if (hasRgnWi) {
                GED_ENCODE(Src0Width, w);
            } else {
                // some ops have an implicit width region
                // (e.g. some specialized instructions poaches Src0.RgnWi)
                //
                // Within the IR we use 1 so logic that depends on regioning
                // gets the correct behavior (hardware assumes w=1).
                w = 1;
            }
            GED_ENCODE(Src0HorzStride, h);
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1VertStride, v);
            GED_ENCODE(Src1Width, w);
            GED_ENCODE(Src1HorzStride, h);
        } else {
            IGA_ASSERT_FALSE(
                "Encoder::encodeSrcRegion: only works on src0 and src1");
        }
    }

    template <SourceIndex S>
    void Encoder::encodeSrcRegionWidth(Region::Width w) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0Width, lowerRegionWidth(w));
        } else { // (S == SourceIndex::SRC1)
            GED_ENCODE(Src1Width, lowerRegionWidth(w));
        } // S != SRC2 since ternary Align1 doesn't have bits for that
    }

    template <SourceIndex S>
    void Encoder::encodeTernaryImmVal(const ImmVal &val, Type type) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0TernaryImm, typeConvesionHelper(val, type));
        } else if (S == SourceIndex::SRC2) {
            GED_ENCODE(Src2TernaryImm, typeConvesionHelper(val, type));
        } else {
            errorT("immediate operands not supported in src1 of ternary formats");
        }
    }

    template <SourceIndex S>
    void Encoder::encodeSrcRegionHorz(Region::Horz s) {
        if (S == SourceIndex::SRC0) {
            GED_ENCODE(Src0HorzStride, lowerRegionHorz(s));
        } else if (S == SourceIndex::SRC1) {
            GED_ENCODE(Src1HorzStride, lowerRegionHorz(s));
        } else {
            GED_ENCODE(Src2HorzStride, lowerRegionHorz(s));
        }
    }
} // end: namespace iga*

namespace iga
{
    typedef Encoder Encoder;
}
#endif

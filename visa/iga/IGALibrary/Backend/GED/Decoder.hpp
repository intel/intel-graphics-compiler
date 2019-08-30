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
#ifndef _IGA_BACKEND_GED_DECODER_HPP_
#define _IGA_BACKEND_GED_DECODER_HPP_

#include "DecoderCommon.hpp"
#include "GEDBitProcessor.hpp"
#include "GEDToIGATranslation.hpp"
#include "ged.h"

namespace iga
{
    struct FlagRegInfo {
        Predication    pred;
        FlagModifier   modifier;
        RegRef         reg;
    };
    struct DirRegOpInfo {
        RegName   regName = RegName::INVALID; // e.g. "r" or "acc"
        RegRef    regRef;  // 13
        Type      type = Type::INVALID;
    };

    class DecoderBase : public GEDBitProcessor
    {
    public:
        // Constructs a new decoder with an error handler and an empty kernel
        DecoderBase(const Model &model, ErrorHandler &errHandler);

        // the main entry point for decoding a kernel
        Kernel *decodeKernelBlocks(
            const void *binary,
            size_t binarySize);
        Kernel *decodeKernelNumeric(
            const void *binary,
            size_t binarySize);

        // Set the SWSB endcoding mode, if not set, derived from platform
        void setSWSBEncodingMode(SWSB_ENCODE_MODE mode)
        {
            m_SWSBEncodeMode = mode;
        }

    private:
        Kernel *decodeKernel(
            const void *binary,
            size_t binarySize,
            bool numericLabels);

        // pass 1 decodes instructions with numeric labels
        void decodeInstructions(
            Kernel &kernel,
            const void *binary,
            size_t binarySize,
            InstList &insts);
        const OpSpec *decodeOpSpec(Op op);

        Instruction *decodeNextInstruction(Kernel &kernel);

        void decodeSWSB(Instruction* inst);

    protected:
        GED_ACCESS_MODE    decodeAccessMode();
        MaskCtrl           decodeMaskCtrl();
        Predication        decodePredication();
        void               decodePredInv(Predication& pred);
        FlagRegInfo        decodeFlagRegInfo(bool imm64Src0Overlap = false); // pred, cond, ...
        ExecSize           decodeExecSize();
        ChannelOffset      decodeChannelOffset();

        void decodeJipToSrc(
            Instruction *inst,
            SourceIndex s = SourceIndex::SRC0,
            Type type = Type::INVALID);
        void               decodeUipToSrc1(Instruction *inst, Type type);
        int32_t            decodeJip();
        int32_t            decodeUip();

        // Reads a source from a the 'fromSrc'th source operand
        // and creates the operand in IR as 'toSrc'.
        // Typically this will be used directly as
        //   createSoureOp(inst, SourceIndex::SRC0, SourceIndex::SRC0);
        // However, for some operands with implicit operands such as
        // jmpi, ths will show up as
        //   createSoureOp(inst, SourceIndex::SRC1, SourceIndex::SRC0);
        // since the first syntactic source is stored in src1's bits
        // (src0 is ip for that op)

        ///////////////////////////////////////////////////////////////////////
        // BASIC INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        Instruction *decodeBasicInstruction(Kernel &kernel);
        void decodeBasicUnaryInstruction(Instruction *inst, GED_ACCESS_MODE accessMode);

        void decodeBasicDestination(Instruction *inst, GED_ACCESS_MODE a) {
            if (a == GED_ACCESS_MODE_Align16)
                decodeBasicDestinationAlign16(inst);
            else
                decodeBasicDestinationAlign1(inst);
        }
        void decodeBasicDestinationAlign16(Instruction *inst); // e.g. for math.invm and context save restore
        void decodeBasicDestinationAlign1(Instruction *inst);
        template <SourceIndex S>
        void decodeSourceBasic(Instruction *inst, GED_ACCESS_MODE a) {
            if (a == GED_ACCESS_MODE_Align16)
                decodeSourceBasicAlign16<S>(inst, S);
            else
                decodeSourceBasicAlign1<S>(inst, S);
        }
        template <SourceIndex S>
        void decodeSourceBasic(Instruction *inst, SourceIndex toSrcIx, GED_ACCESS_MODE a) {
            if (a == GED_ACCESS_MODE_Align16)
                decodeSourceBasicAlign16<S>(inst, toSrcIx);
            else
                decodeSourceBasicAlign1<S>(inst, toSrcIx);
        }
        template <SourceIndex S>
        void decodeSourceBasicAlign1(Instruction *inst) {
            decodeSourceBasicAlign1<S>(inst,S);
        }
        template <SourceIndex S>
        void decodeSourceBasicAlign16(Instruction *inst) {
            decodeSourceBasicAlign16<S>(inst,S);
        }
        template <SourceIndex S>
        void decodeSourceBasicAlign1(Instruction *inst, SourceIndex toSrcIx);
        template <SourceIndex S>
        void decodeSourceBasicAlign16(Instruction *inst, SourceIndex toSrcIx);

        ///////////////////////////////////////////////////////////////////////
        // TERNARY INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        Instruction *decodeTernaryInstruction(Kernel& kernel);
        void decodeTernaryInstructionOperands(Kernel& kernel, Instruction *inst, GED_ACCESS_MODE accessMode);
        // Align16
        void decodeTernaryDestinationAlign16(Instruction *inst);
        template <SourceIndex S> void decodeTernarySourceAlign16(Instruction *inst);
        // Align11
        void decodeTernaryDestinationAlign1(Instruction *inst);
        template <SourceIndex S> void decodeTernarySourceAlign1(Instruction *inst);

        ///////////////////////////////////////////////////////////////////////
        // SEND INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        Instruction *decodeSendInstruction(Kernel &kernel);
        void decodeSendDestination(Instruction *inst);
        GED_ADDR_MODE decodeSendSource0AddressMode();
        void decodeSendSource0(Instruction *inst);
        void decodeSendSource1(Instruction *inst);
        void decodeSendInstructionOptions(Instruction *inst);

        ///////////////////////////////////////////////////////////////////////
        // BRANCH INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        Instruction *decodeBranchInstruction(Kernel &kernel);
        Instruction *decodeBranchSimplifiedInstruction(Kernel &kernel);
        void decodeBranchDestination(Instruction *inst);

        ///////////////////////////////////////////////////////////////////////
        // OTHER INSTRUCTIONS
        ///////////////////////////////////////////////////////////////////////
        Instruction *decodeWaitInstruction(Kernel &kernel);
        Instruction *decodeSyncInstruction(Kernel &kernel);

        ///////////////////////////////////////////////////////////////////////
        // OTHER HELPERS
        ///////////////////////////////////////////////////////////////////////
        void decodeReg(
            int opIx, // dst => opIx<0
            GED_REG_FILE regFile,
            uint32_t regNumBits,
            RegName &regName,
            RegRef &regRef);


        DirRegOpInfo decodeDstDirRegInfo();
        Type decodeDstType();
        int decodeDestinationRegNumAccBitsFromChEn();
        MathMacroExt decodeDestinationMathMacroRegFromChEn();

        virtual unsigned decodeOpGroup(Op op);
                bool hasImm64Src0Overlap();

        virtual void decodeDstDirSubRegNum(DirRegOpInfo& dri);
        virtual bool hasImplicitScalingType(Type& type, DirRegOpInfo& dri);
        virtual void decodeNextInstructionEpilog(Instruction *inst);

        void decodeThreadOptions(Instruction *inst, GED_THREAD_CTRL trdCntrl);

        template <SourceIndex S> ImmVal decodeTernarySrcImmVal(Type t);

        // various GED source accessors
        //some definitions created in Decoder.cpp using macros
        //i.e. DEFINE_GED_SOURCE_ACCESSORS...
        template <SourceIndex S> GED_ADDR_MODE    decodeSrcAddrMode();

//#define DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, I)
        //template <> GED_ADDR_MODE decodeSrcAddrMode <SourceIndex::SRC0>() {
        //    RETURN_GED_DECODE_RAW_TO_SRC(GED_ADDR_MODE, Src0AddrMode);
        //}

        template <SourceIndex S> GED_REG_FILE     decodeSrcRegFile();
        template <SourceIndex S> GED_DATA_TYPE    decodeSrcDataType();

        // register direct fields
        template <SourceIndex S> uint32_t         decodeSrcRegNum();
        template <SourceIndex S> uint32_t         decodeSrcSubRegNum();
        // implicit accumulators
        template <SourceIndex S> GED_MATH_MACRO_EXT  decodeSrcMathMacroExt();

        // register indirect fields
        template <SourceIndex S> int32_t          decodeSrcAddrImm();
        template <SourceIndex S> uint32_t         decodeSrcAddrSubRegNum();
        // immediate fields
        ImmVal                                    decodeSrcImmVal(Type type);

        // register direct and indirect fields
        // yeah, the naming convention dictates this name
        //   (on for Src vs Dst) and one for the field name SrcMod
        template <SourceIndex S> GED_SRC_MOD      decodeSrcSrcMod();


        template <SourceIndex S> Type decodeSrcType() {
            return GEDToIGATranslation::translate(decodeSrcDataType<S>());
        }

        template <SourceIndex S> Region decodeSrcRegionVWH() {
            return GEDToIGATranslation::transateGEDtoIGARegion(
                decodeSrcVertStride<S>(),
                decodeSrcWidth<S>(),
                decodeSrcHorzStride<S>());
        }

        template <SourceIndex S> Region decodeSrcRegionTernaryAlign1();

        template <SourceIndex S> MathMacroExt decodeSrcMathMacroReg() {
            return GEDToIGATranslation::translate(decodeSrcMathMacroExt<S>());
        }
        template <SourceIndex S> SrcModifier decodeSrcModifier() {
            if (m_opSpec->supportsSourceModifiers()) {
                return GEDToIGATranslation::translate(decodeSrcSrcMod<S>());
            }
            return SrcModifier::NONE;
        }

        // decodes regname, regnum, and subreg num (if applicable)
        // does *not* scale the subreg num
        template <SourceIndex S> RegName decodeSourceReg(RegRef &regRef) {
            uint32_t regNumBits = decodeSrcRegNum<S>();
            RegName regName = RegName::INVALID;
            GED_REG_FILE regFile = decodeSrcRegFile<S>();
            decodeReg((int)S, regFile, regNumBits, regName, regRef);
            if (!m_opSpec->isSendOrSendsFamily() && !m_opSpec->isMacro()) {
                regRef.subRegNum = (uint8_t)decodeSrcSubRegNum<S>();
            } else {
                regRef.subRegNum = 0;
            }
            return regName;
        }

        template <SourceIndex S> DirRegOpInfo decodeSrcDirRegOpInfo() {
            DirRegOpInfo dri;
            dri.regName = decodeSourceReg<S>(dri.regRef);

            Type scalingType = Type::INVALID;
            // FIXME: not sure what "hasImplicitScalingType" for, this cause the in-consistent
            // between encoder and decoder. Still using it to keep it the same as before...
            if (!hasImplicitScalingType(scalingType, dri)) {
                scalingType = dri.type = decodeSrcType<S>();
            }

            if (scalingType == Type::INVALID) {
                scalingType = Type::UB;
                if (m_opSpec->isBranching())
                    scalingType = Type::D;
            }

            dri.regRef.subRegNum = binNumToSubRegNum(
                dri.regRef.subRegNum, dri.regName,
                scalingType);


            return dri;
        }

        template <SourceIndex S> uint32_t         decodeSrcVertStride();
        template <SourceIndex S> uint32_t         decodeSrcWidth();
        template <SourceIndex S> uint32_t         decodeSrcHorzStride();
        template <SourceIndex S> uint32_t         decodeSrcChanSel();
        template <SourceIndex S> GED_REP_CTRL     decodeSrcRepCtrl();
        template <SourceIndex S> uint8_t          decodeSrcCtxSvRstAccBitsToRegNum();
        void decodeChSelToSwizzle(uint32_t chanSel, GED_SWIZZLE swizzle[4]);
        template <SourceIndex S> bool isChanSelPacked();

#if GED_VALIDATION_API
        static const bool print_ged_debug = true;
#else
        static const bool print_ged_debug = false;
        static GED_RETURN_VALUE GED_PrintFieldBitLocation(const ged_ins_t* ins, const GED_INS_FIELD field) { return GED_RETURN_VALUE_SUCCESS; };
#endif


        void decodeOptions(Instruction *inst);

        // Translate Sub register from binary encoding number to asm number
        uint32_t binNumToSubRegNum(
            uint32_t binNum, RegName regName, Type type);

    protected:
        GED_MODEL                     m_gedModel;

        // decode-level state (valid below decodeKernel variants)
        Kernel                       *m_kernel;

        // state shared below decodeInstToBlock()
        // info about the instruction being converted to IGA IR
        ged_ins_t                     m_currGedInst;
        const OpSpec                 *m_opSpec;
        const void                   *m_binary;

        // SWSB encoding mode
        SWSB_ENCODE_MODE m_SWSBEncodeMode = SWSB_ENCODE_MODE::SWSBInvalidMode;

        // for GED workarounds: grab specific bits from the current instruction
        uint32_t getBitField(int ix, int len) const;

        // helper for inserting instructions for decode errors
        Instruction *createErrorInstruction(
            Kernel &kernel,
            const char *message,
            const void *binary,
            int32_t iLen);

        // used by the GED_DECODE macros
        void handleGedDecoderError(
            int line,
            const char *field,
            GED_RETURN_VALUE status);
    }; // end class Decoder



} //end: namespace iga*

namespace iga
{
    typedef DecoderBase Decoder;
}

#endif //end: _IGA_DECODER_H_

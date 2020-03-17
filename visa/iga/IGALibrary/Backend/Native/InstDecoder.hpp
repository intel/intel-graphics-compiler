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
#ifndef IGA_NATIVE_INSTDECODER_HPP
#define IGA_NATIVE_INSTDECODER_HPP

#include "../../Frontend/IRToString.hpp"
#include "../../Frontend/Floats.hpp"
#include "../../IR/InstBuilder.hpp"
#include "../../strings.hpp"
#include "../BitProcessor.hpp"
#include "Field.hpp"
#include "Interface.hpp"
#include "MInst.hpp"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace iga
{
    typedef std::function<void(uint64_t bits, std::stringstream &ss)> FormatFunction;

    static int nextPowerOfTwo(int v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }
    static PredCtrl decodePredCtrl(uint64_t bits) {
        return static_cast<PredCtrl>(bits);
    }
    static ExecSize decodeExecSizeBits(uint64_t val) {
        switch (val) {
        case 0: return ExecSize::SIMD1;
        case 1: return ExecSize::SIMD2;
        case 2: return ExecSize::SIMD4;
        case 3: return ExecSize::SIMD8;
        case 4: return ExecSize::SIMD16;
        case 5: return ExecSize::SIMD32;
        default: return ExecSize::INVALID;
        }
    }
    static ChannelOffset decodeChannelOffsetBits(uint64_t val) {
        return static_cast<ChannelOffset>(val);
    }
    static FlagModifier decodeFlagModifierBits(uint64_t val) {
        FlagModifier fm;
        switch (val) {
        case 0: fm = FlagModifier::NONE; break;
        case 1: fm = FlagModifier::EQ; break;
        case 2: fm = FlagModifier::NE; break;
        case 3: fm = FlagModifier::GT; break;
        case 4: fm = FlagModifier::GE; break;
        case 5: fm = FlagModifier::LT; break;
        case 6: fm = FlagModifier::LE; break;
        // case 7: reserved
        case 8: fm = FlagModifier::OV; break;
        case 9: fm = FlagModifier::UN; break;
        default: fm = static_cast<FlagModifier>(val); break;
        }
        return fm;
    }
    static SrcModifier decodeSrcModifierBits(uint64_t bits)
    {
        switch (bits) {
        case 0: return SrcModifier::NONE;
        case 1: return SrcModifier::ABS;
        case 2: return SrcModifier::NEG;
        case 3: return SrcModifier::NEG_ABS;
        default: return static_cast<SrcModifier>(bits);
        }
    }
    static FlagModifier decodeFlagModifier(uint64_t bits)
    {
        switch (bits) {
        case 0: return FlagModifier::NONE;
        case 1: return FlagModifier::EQ;
        case 2: return FlagModifier::NE;
        case 3: return FlagModifier::GT;
        case 4: return FlagModifier::GE;
        case 5: return FlagModifier::LT;
        case 6: return FlagModifier::LE;
        //
        case 8: return FlagModifier::OV;
        case 9: return FlagModifier::UN;
        default: return static_cast<FlagModifier>(bits);
        }
     }

    //
    // This is a generic template for instruction decoders for various GEN
    // platforms.  Users can use this via inheritance or composition equally.
    // This class also decodes field by field for various debugging routines.
    // E.g. iga64 -Xifs ... decodes each field
    //
    // The naming convention is as follows:
    //
    //  T  decodeXXXX(Field fXXXX)
    // decodes something (an XXXX) using field fXXXX and store this into the
    // field list.  This implies that fXXXX is properly part of the format and
    // thus will not overlap with any other field decoded.  It also might
    // commit the field to the instruction builder.
    //
    //  T  peekXXXX(Field fXXXX)
    // decoes something, but quietly (don't add it to the list of
    // fields decoded).  E.g. we might speculatively need to look at a field
    // that we might not want to add.
    //
    // TODO: template<typename P> where P is PlatformInfo
    //       for GEN-specific stuff use this var; e.g. P.decodeBasicRegType(...)
    struct InstDecoder
    {
        InstBuilder    &builder;
        ErrorHandler   &errorHandler;
        const Model    &model;
        const OpSpec   &os;
        MInst           bits;
        FragmentList   *fields;
        Loc             loc;

        // instruction state
        ExecSize        execSize;
        InstOptSet      instOptSet;

        InstDecoder(
            InstBuilder &_builder,
            ErrorHandler &_errorHandler,
            const Model &_model,
            const OpSpec &_os,
            MInst _bits,
            FragmentList *_fields,
            Loc _loc)
            :  builder(_builder)
            , errorHandler(_errorHandler)
            , model(_model)
            , os(_os)
            , bits(_bits)
            , fields(_fields)
            , loc(_loc)
            , execSize(ExecSize::INVALID)
            , instOptSet(0)
        {
        }


        void reportError(const char *msg) {
            errorHandler.reportError(loc, msg);
        }
        void reportError(const std::string &msg) {
            errorHandler.reportError(loc, msg);
        }
        void reportWarning(const std::string &msg) {
            errorHandler.reportWarning(loc, msg);
        }
        void reportFieldError(const Field &f, const char *msg) {
            std::stringstream ss;
            ss << f.name << ": " <<  msg;
            reportError(ss.str());
        }
        void reportFieldWarning(const Field &f, const char *msg) {
            std::stringstream ss;
            ss << f.name << ": " <<  msg;
            reportWarning(ss.str());
        }
        void reportFieldErrorInvalidValue(const Field &f) {
            std::stringstream ss;
            reportFieldError(f, "invalid value");
        }

        ///////////////////////////////////////////
        // PRIMITIVE FIELD ADDERS
        void addDecodedField(const Field &f, std::string meaning) {
            if (fields) {
                int nEncoded = 0;
                for (const Fragment &fr : f.fragments) {
                    if (fr.isInvalid())
                        break;
                    else if (fr.isEncoded())
                        if (++nEncoded >= 2)
                            break;
                }
                bool needsFragmentPrefix = nEncoded >= 2;
                int fragIx = 0;
                for (const Fragment &fr : f.fragments) {
                    if (fr.isInvalid())
                        break;
                    else if (fr.isEncoded()) {
                        std::string fragMeaning;
                        if (needsFragmentPrefix) {
                            std::stringstream ss;
                            ss << "[frag. " << fragIx << "]: " << meaning;
                            fragMeaning = ss.str();
                            fragIx++;
                        } else {
                            fragMeaning = meaning;
                        }
                        addDecodedFragment(fr, fragMeaning);
                    }
                }
            }
        }
        void addDecodedFragment(const Fragment &fr, std::string val) {
            if (fields)
                fields->emplace_back(fr, val);
        }
        uint64_t decodeFragment(const Fragment &f) {
            if (fields)
                fields->emplace_back(f, "");
            return bits.getFragment(f);
        }
        uint64_t decodeFragment(const Fragment &f, FormatFunction fmt) {
            auto val = bits.getFragment(f);
            if (fields) {
                std::stringstream ss;
                fmt(val,ss);
                fields->emplace_back(f, ss.str());
            }
            return val;
        }
        uint64_t decodeFieldWithFunction(const Fragment &f, FormatFunction fmt) {
            return decodeFragment(f, fmt);
        }

        void addFieldSubfunction(const Field &f, std::string val = "") {
            addDecodedField(f, val);
        }

        void addReserved(int off, int len, std::string errStr = "?") {
            Fragment fRSVD("Reserved", off, len);
            auto b = bits.getFragment(fRSVD);
            addDecodedFragment(fRSVD, b != 0 ? errStr : "");
        }
        void addReserved(const Fragment &f) {
            std::stringstream ss;
            ss << "? (shadows " << f.name << ")";
            addReserved(f.offset, f.length, ss.str().c_str());
        }
        void addReserved(const Field &f) {
            for (const Fragment &fr : f.fragments) {
                if (fr.isEncoded()) {
                    addReserved(fr);
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////
        // one bit fields
        ///////////////////////////////////////////////////////////////////////////
        uint32_t decodeFieldU32(const Field &f) {
            addDecodedField(f, "");
            return (uint32_t)bits.getField(f);
        }
        template <typename T>
        T decodeField(
            const Field &f,
            T val0, const char *str0,
            T val1, const char *str1)
        {
            IGA_ASSERT(f.length() == 1, "field is >1 bit");
            T val;
            const char *str = "";
            auto b = bits.getField(f);
            switch (b) {
            case 0: val = val0; str = str0; break;
            case 1: val = val1; str = str1; break;
            default:
                IGA_ASSERT_FALSE("Unreachable");
                break; // unreachable
            };
            if (fields) {
                fields->emplace_back(f, str);
            }
            return val;
        }
        bool decodeBoolField(
            const Field &f,
            const char *falseMeaning,
            const char *trueMeaning)
        {
            return decodeField<bool>(f, false, falseMeaning, true, trueMeaning);
        }

        ///////////////////////////////////////////////////////////////////////
        // two bit fields
        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        T decodeField(
            const Field &f,
            T val0, const char *str0,
            T val1, const char *str1,
            T val2, const char *str2,
            T val3, const char *str3)
        {
            // IGA_ASSERT(f.length == 2, "field is not a two bits");
            // we can have subset fields like Dst.RgnHz[0] in ternary
            IGA_ASSERT(f.length() <= 2, "field is not a two bits");
            T val;
            const char *str = "";
            auto b = bits.getField(f);
            switch (b) {
            case 0: val = val0; str = str0; break;
            case 1: val = val1; str = str1; break;
            case 2: val = val2; str = str2; break;
            case 3: val = val3; str = str3; break;
            default:
                IGA_ASSERT_FALSE("Unreachable");
                break; // unreachable
            };
            if (fields) {
                fields->emplace_back(f, str);
            }
            return val;
        }

        ///////////////////////////////////////////////////////////////////////
        // multi bit fields
        ///////////////////////////////////////////////////////////////////////
        uint64_t decodeField(
            const Field &f,
            FormatFunction fmt)
        {
            auto val = bits.getField(f);
            if (fields)
            {
                std::stringstream ss;
                fmt(val, ss);
                fields->emplace_back(f, ss.str());
            }
            return val;
        }

        template <typename T>
        T decodeField(
            const Field &f,
            T invalid,
            std::initializer_list<std::pair<T,const char *>> vals)
        {
            IGA_ASSERT(nextPowerOfTwo((int)vals.size()) != f.length(),
                "field is wrong number of bits");
            int i = 0;
            T retVal = invalid;
            const char *strVal = "?";
            auto b = bits.getField(f);
            for (const auto &val : vals) {
                if (i == (int)b) {
                    retVal = val.first;
                    strVal = val.second;
                    break;
                }
                i++;
            }
            if (i == vals.size() || retVal == invalid) { // didn't find it
                retVal = invalid;
                strVal = "?";
                reportFieldErrorInvalidValue(f);
            }

            if (fields) {
                fields->emplace_back(f, strVal);
            }
            return retVal;
        }


        ///////////////////////////////////////////////////////////////////////
        // generic GEN helpers
        ///////////////////////////////////////////////////////////////////////
        void decodeMaskCtrl(const Field &fMASKCTRL) {
            if (decodeBoolField(fMASKCTRL, "", "WrEn")) {
                builder.InstNoMask(loc);
            }
        }

        void decodePredication(
            const Field &fPREDINV,
            const Field &fPREDCTRL,
            const Field &fFLAGREG)
        {
            PredCtrl predCtrl =
                decodePredCtrl(bits.getField(fPREDCTRL));
            addDecodedField(fPREDCTRL, ToSyntax(predCtrl));
            bool predInv = decodeBoolField(fPREDINV, "", "~");
            if (predInv && predCtrl == PredCtrl::NONE) {
                reportFieldError(fPREDINV,
                    "PredCtrl is not set, but PredInv is set");
            }
            if (predCtrl != PredCtrl::NONE) {
                RegRef flagReg = peekFlagRegRef(fFLAGREG);
                builder.InstPredication(loc, predInv, flagReg, predCtrl);
            }
        }

        void decodeBrCtl(const Field &fBRCTL) {
            bool brCtl = decodeBoolField(fBRCTL, "", ".b");
            builder.InstBrCtl(brCtl ? BranchCntrl::ON : BranchCntrl::OFF);
        }

        RegRef peekFlagRegRef(const Field &fFLAGREG) const {
            auto val = bits.getField(fFLAGREG);
            RegRef rr{(uint8_t)(val >> 1),(uint8_t)(val & 0x1)};
            return rr;
        }

        RegRef decodeFlagReg(const Field &fFLAGREG) {
            // flag register
            decodeField(fFLAGREG, [](uint64_t bits, std::stringstream &ss) {
                ss << "f" << (bits >> 1) << "." << (bits & 0x1);
                });
            return peekFlagRegRef(fFLAGREG);
        }

        void decodeFlagModifierField(
            const Field &fFLAGMOD,
            const Field &fFLAGREG)
        {
            RegRef flagReg = peekFlagRegRef(fFLAGREG);
            FlagModifier invalidModifier = static_cast<FlagModifier>(-1);
            FlagModifier zeroValue = FlagModifier::NONE;
            const char *zeroString = "";
            if (os.isMathSubFunc() && os.isMacro()) {
                zeroValue = FlagModifier::EO;
                zeroString = "(eo) (early out)";
            }
            // TODO: use decodeFlagModifier() and ToSyntax
            FlagModifier flagMod = decodeField<FlagModifier>(
                fFLAGMOD, invalidModifier, {
                 {zeroValue, zeroString},
                 {FlagModifier::EQ, "(eq)"},
                 {FlagModifier::NE, "(ne)"},
                 {FlagModifier::GT, "(gt)"},
                 {FlagModifier::GE, "(ge)"},
                 {FlagModifier::LT, "(lt)"},
                 {FlagModifier::LE, "(le)"},
                 {invalidModifier, "?"},
                 {FlagModifier::OV, "(ov) (overflow)"},
                 {FlagModifier::UN, "(un) (unordered)"}});
            builder.InstFlagModifier(flagReg, flagMod);
        }

        ImmVal decodeImm32(const Field &fIMM32L, Type t) {
            std::stringstream ss;
            ImmVal immVal;
            uint64_t val = bits.getField(fIMM32L);
            //
            // TODO: determine if ImmVal should store everything as s64
            // otherwise I need to normalize what the GED parser and decoder
            // a specific exmaple:
            //  NATIVE:       0x0000000000008000
            //  GED:          0xFFFFFFFFFFFF8000
            //
            // NATIVE: src0: PC[0]:
            //   mov:        61 00 03 00 a0 45 05 01  00 00 00 00 00 80 00 80
            //   mov (8|M0)               r1.0<1>:f     -32768:w
            // GED takes a 64b value
            switch (t) {
            case Type::UW:
                immVal = (uint16_t)val;
                ss << std::hex << (uint16_t)val;
                break;
            case Type::UD:
                immVal = (uint32_t)val;
                ss << std::hex << "0x" << (uint32_t)val;
                break;
            case Type::W:
                immVal = (int16_t)val;
                immVal.s64 = immVal.s16;
                ss << std::dec << (int16_t)val;
                break;
            case Type::D:
                immVal = (int32_t)val;
                immVal.s64 = immVal.s32;
                ss << std::dec << (int32_t)val;
                break;
            case Type::HF:
                immVal = (uint16_t)val;
                immVal.kind = ImmVal::F16;
                FormatFloat(ss, FloatFromBits((uint16_t)val));
                break;
            case Type::F:
                immVal = FloatFromBits((uint32_t)val);
                FormatFloat(ss, FloatFromBits((uint32_t)val));
                break;
            case Type::V:
            case Type::UV:
                immVal = (uint32_t)val;
                ss << "(";
                for (int i = 7; i >= 0; i--) {
                    if (i < 7) ss << ',';
                    if (t == Type::V) {
                        ss << std::hex << getSignedBits<int64_t>((int64_t)val, i*4, 4);
                    } else {
                        ss << std::dec << getBits<uint64_t>(val, i*4, 4);
                    }
                }
                ss << ")";
                break;
            case Type::VF:
                immVal = (uint32_t)val;
                ss << "(";
                for (int i = 3; i >= 0; i--) {
                    if (i < 3) ss << ',';
                    FormatFloat(ss, (uint8_t)getBits(val, i*8, 8));
                }
                ss << ")";
                break;
                break;
            default:
                immVal = (uint32_t)0;
                reportFieldError(fIMM32L, "invalid type for 32b IMM");
                ss << "?";
            }

            addDecodedField(fIMM32L, ss.str());

            return immVal;
        }

        ImmVal decodeImm64(
            const Field &fIMM32L, const Field &fIMM32H, Type type)
        {
            uint64_t lo = bits.getField(fIMM32L);
            uint64_t hi = bits.getField(fIMM32H);
            ImmVal immVal;
            immVal.u64 = ((hi << 32) | lo);;
            std::stringstream ss;
            ss << "(";
            switch (type) {
            case Type::DF:
                immVal.kind = ImmVal::F64;
                FormatFloat(ss, FloatFromBits(immVal.u64));
                break;
            case Type::UQ:
                immVal.kind = ImmVal::U64;
                ss << immVal.u64;
                break;
            case Type::Q:
                immVal.kind = ImmVal::S64;
                ss << immVal.s64;
                break;
            default:
                ss << "ERROR: expected 64b type";
                reportError("ERROR: expected 64b type");
            }
            ss << ")";
            addDecodedField(fIMM32L, "LO32" + ss.str());
            addDecodedField(fIMM32H, "HI32" + ss.str());
            return immVal;
        }

        void decodeExecOffsetInfo(
            const Field &fEXECSIZE,
            const Field &fCHANOFF)
        {
            execSize = decodeExecSizeBits(bits.getField(fEXECSIZE));
            if (execSize == ExecSize::INVALID) {
                reportFieldErrorInvalidValue(fEXECSIZE);
            }
            std::stringstream ssEs;
            ssEs << "(" << ToSyntax(execSize) << "|...)";
            addDecodedField(fEXECSIZE, ssEs.str());

            auto chOffBits = bits.getField(fCHANOFF);
            ChannelOffset chOff = decodeChannelOffsetBits(chOffBits);
            if (chOff < ChannelOffset::M0 || chOff > ChannelOffset::M28) {
                reportFieldErrorInvalidValue(fCHANOFF);
            }
            std::stringstream ssCo;
            ssCo << "(..." << ToSyntax(chOff) << ")";
            addDecodedField(fCHANOFF, ssCo.str());

            builder.InstExecInfo(loc, execSize, loc, chOff);
        }

        DstModifier decodeDstModifier(const Field &fSATURATE) {
            return
                decodeField(fSATURATE,
                    DstModifier::NONE, "",
                    DstModifier::SAT, "(sat)");
        }

        SrcModifier decodeSrcMods(const Field &fSRCMODS) {
            return decodeField<SrcModifier>(
                fSRCMODS,
                SrcModifier::NONE,"",
                SrcModifier::ABS, "(abs)",
                SrcModifier::NEG,  os.isBitwise() ? "~" : "-",
                SrcModifier::NEG_ABS,"-(abs)");
        }

        MathMacroExt decodeMathMacroReg(const Field &fSPCACC) {
            addReserved(fSPCACC.fragments[0].offset + 4, 1);
            return decodeMathMacroRegField(fSPCACC);
        }

        void decodeSubReg(
            OpIx opIndex,
            OperandInfo &opInfo,
            const Field &fSUBREG)
        {
            decodeSubRegWithType(
                opIndex, opInfo, fSUBREG, opInfo.type, ToSyntax(opInfo.type));
        }

        // e.g. for subregisters without proper types
        void decodeSubRegWithImplicitType(
            OpIx opIndex,
            OperandInfo &opInfo,
            const Field &fSUBREG,
            Type t)
        {
            decodeSubRegWithType(opIndex, opInfo, fSUBREG, t, "");
        }

        void decodeSubRegWithType(
            OpIx opIndex,
            OperandInfo &opInfo,
            const Field &fSUBREG,
            Type type,
            std::string typeSyntax)
        {
            auto srb = (int)bits.getField(fSUBREG);
            auto scaled = BytesOffsetToSubReg(srb, opInfo.regOpName, type);
            auto unscaled =
                SubRegToBytesOffset((int)scaled, opInfo.regOpName, type);
            if (unscaled != srb) {
                reportFieldError(fSUBREG,
                    "subregister offset is misaligned for type size");
            }
            opInfo.regOpReg.subRegNum = scaled;

            std::stringstream ss;
            ss << "." << (int)opInfo.regOpReg.subRegNum << typeSyntax;
            addDecodedField(fSUBREG, ss.str());
        }

        void decodeRegDirectFields(
            OpIx opIndex,
            OperandInfo &opInfo,
            const Field &fREGFILE, // direct only
            const Field &fSPCACC,
            const Field &fSUBREG,
            const Field &fREG)
        {
            decodeRegFields(opInfo, fREGFILE, fREG);
            if (os.isMacro()) {
                opInfo.kind = Operand::Kind::MACRO;
                opInfo.regOpMathMacroExtReg = decodeMathMacroRegField(fSPCACC);
                addReserved(
                    fSPCACC.fragments[0].offset + fSPCACC.fragments[0].length,
                    1);
                if (!IsTernary(opIndex) &&
                    (IsDst(opIndex) || ToSrcIndex(opIndex) == 0) &&
                    fSUBREG.length() > 1 &&
                    fSUBREG.fragments[0].length == 1)
                {
                    // for binary macros, the low of subreg is stowed elsewhere
                    addReserved(fSUBREG.fragments[0]);
                }
            } else {
                opInfo.kind = Operand::Kind::DIRECT;
                decodeSubReg(opIndex, opInfo, fSUBREG);
            }
        }

        void decodeRegFields(
            OperandInfo &opInfo,
            const Field &fREGFILE,
            const Field &fREG)
        {
            bool isGrf = decodeBoolField(fREGFILE, "ARF", "GRF");
            std::stringstream ss;
            auto val = bits.getField(fREG);
            if (isGrf) {
                opInfo.regOpName = RegName::GRF_R;
                opInfo.regOpReg.regNum = (uint8_t)val;
                ss << "r" << val;
            } else {
                const RegInfo *regInfo =
                    model.lookupArfRegInfoByRegNum((uint8_t)val);
                if (regInfo == nullptr) {
                    ss << "ARF?";
                    std::stringstream ssErr;
                    ssErr << "invalid ARF Reg (" << fmtHex(val,2) << ")";
                    reportFieldError(fREG,  ssErr.str().c_str());
                } else {
                    opInfo.regOpName = regInfo->regName;
                    ss << regInfo->syntax;
                    int arfReg = 0;
                    if(val > 0xFF || !regInfo->decode((uint8_t)val, arfReg)) {
                        ss << arfReg << "?";
                        std::stringstream ss;
                        ss << regInfo->syntax << arfReg << " is out of bounds";
                        reportFieldError(fREG, ss.str().c_str());
                    } else {
                        if (regInfo->hasRegNum()) {
                            ss << arfReg;
                        }
                        opInfo.regOpReg.regNum = (int)arfReg;
                        if (regInfo->regNumBase > 0) {
                            // if the register covers to another, let's tell
                            // them which
                            uint8_t coverRegBits = (uint8_t)val & 0xF0;
                            const RegInfo *coverRegInfo =
                                model.lookupArfRegInfoByRegNum(coverRegBits);
                            if (coverRegInfo) {
                                // e.g. GEN12 acc10 is mme2
                                ss << " (" <<
                                    coverRegInfo->syntax << (val & 0xF) << ")";
                            }
                        }
                    }
                }
            }
            addDecodedField(fREG, ss.str());
        }

        MathMacroExt decodeMathMacroRegField(const Field &fSPCACC) {
            return decodeField<MathMacroExt>(
                fSPCACC, MathMacroExt::INVALID,
                {{MathMacroExt::MME0,".mme0"},
                 {MathMacroExt::MME1,".mme1"},
                 {MathMacroExt::MME2,".mme2"},
                 {MathMacroExt::MME3,".mme3"},
                 {MathMacroExt::MME4,".mme4"},
                 {MathMacroExt::MME5,".mme5"},
                 {MathMacroExt::MME6,".mme6"},
                 {MathMacroExt::MME7,".mme7"},
                 {MathMacroExt::NOMME,".nomme"}});
        }

        bool decodeInstOpt(const Field &fINSTOPT, InstOpt opt) {
            IGA_ASSERT(fINSTOPT.length() == 1, "inst opt field is >1 bit");

            std::stringstream ss;
            bool z = bits.getField(fINSTOPT) != 0;
            if (z) {
                instOptSet.add(opt);
                ss << "{" << ToSyntax(opt) << "}";
            }
            addDecodedField(fINSTOPT, ss.str());
            return z;
        }
    }; // InstDecoder
} // iga::


#endif // IGA_NATIVE_INSTDECODER_HPP

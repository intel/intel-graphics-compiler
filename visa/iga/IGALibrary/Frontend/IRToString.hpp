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
#ifndef _IGA_IRTYPES_TO_STRING_HPP
#define _IGA_IRTYPES_TO_STRING_HPP

#include "../IR/Types.hpp"
#include "../strings.hpp"

#include <string>
#include <sstream>
#include <ostream>

namespace iga {

static std::string MakeErrorString(const char *pfx, int t) {
    std::stringstream ss;
    ss << pfx << "::" << t << "?";
    return ss.str();
}

#define MAKE_CASE(C,S) case C::S: return #C "::" #S
#define MAKE_DEFAULT_CASE(C) default: return MakeErrorString(#C, (int)x)

static std::string ToSymbol(Platform x) {
    switch (x) {
    MAKE_CASE(Platform, INVALID);
    MAKE_CASE(Platform, GEN6);
    MAKE_CASE(Platform, GEN7);
    MAKE_CASE(Platform, GEN7P5);
    MAKE_CASE(Platform, GEN8);
    MAKE_CASE(Platform, GEN8LP);
    MAKE_CASE(Platform, GEN9);
    MAKE_CASE(Platform, GEN9LP);
    MAKE_CASE(Platform, GEN9P5);
    MAKE_CASE(Platform, GEN10);
    MAKE_CASE(Platform, GEN11);
    MAKE_CASE(Platform, GENNEXT);
    MAKE_DEFAULT_CASE(Platform);
    }
}


static std::string ToSymbol(PredCtrl x) {
    switch (x) {
    MAKE_CASE(PredCtrl, NONE);
    MAKE_CASE(PredCtrl, SEQ);
    MAKE_CASE(PredCtrl, ANYV);
    MAKE_CASE(PredCtrl, ALLV);
    MAKE_CASE(PredCtrl, ANY2H);
    MAKE_CASE(PredCtrl, ALL2H);
    MAKE_CASE(PredCtrl, ANY4H);
    MAKE_CASE(PredCtrl, ALL4H);
    MAKE_CASE(PredCtrl, ANY8H);
    MAKE_CASE(PredCtrl, ALL8H);
    MAKE_CASE(PredCtrl, ANY16H);
    MAKE_CASE(PredCtrl, ALL16H);
    MAKE_CASE(PredCtrl, ANY32H);
    MAKE_CASE(PredCtrl, ALL32H);
    MAKE_DEFAULT_CASE(PredCtrl);
    }
}


static std::string ToSymbol(Op op) {
    // TODO: expand this manually, this is a crude approximation
    std::stringstream ss;
    ss << "op#" << (int)op;
    return ss.str();
}


static std::string ToSymbol(bool x) {
    return x ? "true" : "false";
}

static std::string ToSymbol(BranchCntrl x) {
    switch (x) {
    MAKE_CASE(BranchCntrl, ON);
    MAKE_CASE(BranchCntrl, OFF);
    MAKE_DEFAULT_CASE(BranchCntrl);
    }
}


static std::string ToSymbol(ExecSize x) {
    switch (x) {
    MAKE_CASE(ExecSize, SIMD1);
    MAKE_CASE(ExecSize, SIMD2);
    MAKE_CASE(ExecSize, SIMD4);
    MAKE_CASE(ExecSize, SIMD8);
    MAKE_CASE(ExecSize, SIMD16);
    MAKE_CASE(ExecSize, SIMD32);
    MAKE_DEFAULT_CASE(ExecSize);
    }
}


static std::string ToSymbol(ChannelOffset x) {
    switch (x) {
    MAKE_CASE(ChannelOffset, M0);
    MAKE_CASE(ChannelOffset, M4);
    MAKE_CASE(ChannelOffset, M8);
    MAKE_CASE(ChannelOffset, M12);
    MAKE_CASE(ChannelOffset, M16);
    MAKE_CASE(ChannelOffset, M20);
    MAKE_CASE(ChannelOffset, M24);
    MAKE_CASE(ChannelOffset, M28);
    MAKE_DEFAULT_CASE(ChannelOffset);
    }
}


static std::string ToSymbol(MaskCtrl x) {
    switch (x) {
    MAKE_CASE(MaskCtrl, NORMAL);
    MAKE_CASE(MaskCtrl, NOMASK);
    MAKE_DEFAULT_CASE(MaskCtrl);
    }
}


static std::string ToSymbol(SrcModifier x) {
    switch (x) {
    MAKE_CASE(SrcModifier, NONE);
    MAKE_CASE(SrcModifier, ABS);
    MAKE_CASE(SrcModifier, NEG);
    MAKE_CASE(SrcModifier, NEG_ABS);
    MAKE_DEFAULT_CASE(SrcModifier);
    }
}


static std::string ToSymbol(DstModifier x) {
    switch (x) {
    MAKE_CASE(DstModifier, NONE);
    MAKE_CASE(DstModifier, SAT);
    MAKE_DEFAULT_CASE(DstModifier);
    }
}


static std::string ToSymbol(RegName x) {
    switch (x) {
    MAKE_CASE(RegName, INVALID);
    MAKE_CASE(RegName, ARF_NULL);
    MAKE_CASE(RegName, ARF_A);
    MAKE_CASE(RegName, ARF_ACC);
    MAKE_CASE(RegName, ARF_MME);
    MAKE_CASE(RegName, ARF_F);
    MAKE_CASE(RegName, ARF_CE);
    MAKE_CASE(RegName, ARF_MSG);
    MAKE_CASE(RegName, ARF_SP);
    MAKE_CASE(RegName, ARF_SR);
    MAKE_CASE(RegName, ARF_CR);
    MAKE_CASE(RegName, ARF_N);
    MAKE_CASE(RegName, ARF_IP);
    MAKE_CASE(RegName, ARF_TDR);
    MAKE_CASE(RegName, ARF_TM);
    MAKE_CASE(RegName, ARF_FC);
    MAKE_CASE(RegName, ARF_DBG);
    MAKE_CASE(RegName, GRF_R);
    MAKE_DEFAULT_CASE(RegName);
    }
}


static std::string ToSymbol(MathMacroExt x) {
    switch (x) {
    MAKE_CASE(MathMacroExt, INVALID);
    MAKE_CASE(MathMacroExt, MME0);
    MAKE_CASE(MathMacroExt, MME1);
    MAKE_CASE(MathMacroExt, MME2);
    MAKE_CASE(MathMacroExt, MME3);
    MAKE_CASE(MathMacroExt, MME4);
    MAKE_CASE(MathMacroExt, MME5);
    MAKE_CASE(MathMacroExt, MME6);
    MAKE_CASE(MathMacroExt, MME7);
    MAKE_CASE(MathMacroExt, NOMME);
    MAKE_DEFAULT_CASE(MathMacroExt);
    }
}


static std::string ToSymbol(Type x) {
    switch (x) {
    MAKE_CASE(Type, INVALID);

    MAKE_CASE(Type, UB);
    MAKE_CASE(Type, B);
    MAKE_CASE(Type, UW);
    MAKE_CASE(Type, W);
    MAKE_CASE(Type, UD);
    MAKE_CASE(Type, D);
    MAKE_CASE(Type, UQ);
    MAKE_CASE(Type, Q);

    MAKE_CASE(Type, HF);
    MAKE_CASE(Type, F);
    MAKE_CASE(Type, DF);
    MAKE_CASE(Type, NF);

    MAKE_CASE(Type, UV);
    MAKE_CASE(Type, V);
    MAKE_CASE(Type, VF);


    MAKE_DEFAULT_CASE(Type);
    }
}


static std::string ToSymbol(FlagModifier x) {
    switch (x) {
    MAKE_CASE(FlagModifier, NONE);
    MAKE_CASE(FlagModifier, EQ);
    MAKE_CASE(FlagModifier, NE);
    MAKE_CASE(FlagModifier, GT);
    MAKE_CASE(FlagModifier, GE);
    MAKE_CASE(FlagModifier, LT);
    MAKE_CASE(FlagModifier, LE);
    MAKE_CASE(FlagModifier, OV);
    MAKE_CASE(FlagModifier, UN);
    MAKE_CASE(FlagModifier, EO);
    MAKE_DEFAULT_CASE(FlagModifier);
    }
}


static std::string ToSymbol(Region x) {
    if (x == Region::INVALID)
        return "Region::INVALID";
    else if (x == Region::SRC010)
        return "Region::SRC010";
    else if (x == Region::SRC110)
        return "Region::SRC110";
    else if (x == Region::SRC221)
        return "Region::SRC221";
    else if (x == Region::SRC441)
        return "Region::SRC441";
    else if (x == Region::SRC881)
        return "Region::SRC881";
    else if (x == Region::SRCFF1)
        return "Region::SRCFF1";
    else if (x == Region::DST1)
        return "Region::DST1";
    else if (x == Region::SRC0X0)
        return "Region::SRC0X0";
    else if (x == Region::SRC2X1)
        return "Region::SRC2X1";
    else if (x == Region::SRC8X1)
        return "Region::SRC8X1";
    else if (x == Region::SRCXX0)
        return "Region::SRCXX0";
    else if (x == Region::SRCXX1)
        return "Region::SRCXX1";
    else {
        std::stringstream ss;
        ss << "{";
        switch (x.getVt()) {
        case Region::Vert::VT_0: ss << "Region::Vert::VT_0"; break;
        case Region::Vert::VT_1: ss << "Region::Vert::VT_1"; break;
        case Region::Vert::VT_2: ss << "Region::Vert::VT_2"; break;
        case Region::Vert::VT_4: ss << "Region::Vert::VT_4"; break;
        case Region::Vert::VT_8: ss << "Region::Vert::VT_8"; break;
        case Region::Vert::VT_16: ss << "Region::Vert::VT_16"; break;
        case Region::Vert::VT_32: ss << "Region::Vert::VT_32"; break;
        case Region::Vert::VT_VxH: ss << "Region::Vert::VT_VxH"; break;
        case Region::Vert::VT_INVALID: ss << "Region::Vert::VT_INVALID";
        default: ss << x.v << "?"; break;
        }
        ss << ",";

        switch (x.getWi()) {
        case Region::Width::WI_1: ss << "Region::Width::WI_1"; break;
        case Region::Width::WI_2: ss << "Region::Width::WI_2"; break;
        case Region::Width::WI_4: ss << "Region::Width::WI_4"; break;
        case Region::Width::WI_8: ss << "Region::Width::WI_8"; break;
        case Region::Width::WI_16: ss << "Region::Width::WI_16"; break;
        case Region::Width::WI_INVALID: ss << "Region::Width::WI_INVALID"; break;
        default: ss << x.v << "?"; break;
        }
        ss << ",";

        switch (x.getHz()) {
        case Region::Horz::HZ_0: ss << "Region::Horz::HZ_0"; break;
        case Region::Horz::HZ_1: ss << "Region::Horz::HZ_1"; break;
        case Region::Horz::HZ_2: ss << "Region::Horz::HZ_2"; break;
        case Region::Horz::HZ_4: ss << "Region::Horz::HZ_4"; break;
        case Region::Horz::HZ_INVALID: ss << "Region::Horz::HZ_INVALID"; break;
        default: ss << x.v << "?"; break;
        }
        ss << "}";
        return ss.str();
    }
}


static std::string ToSymbol(InstOpt x) {
    switch (x) {
    MAKE_CASE(InstOpt, ACCWREN);
    MAKE_CASE(InstOpt, ATOMIC);
    MAKE_CASE(InstOpt, BREAKPOINT);
    MAKE_CASE(InstOpt, COMPACTED);
    MAKE_CASE(InstOpt, EOT);
    MAKE_CASE(InstOpt, NODDCHK);
    MAKE_CASE(InstOpt, NODDCLR);
    MAKE_CASE(InstOpt, NOPREEMPT);
    MAKE_CASE(InstOpt, NOSRCDEPSET);
    MAKE_CASE(InstOpt, SWITCH);
    MAKE_DEFAULT_CASE(InstOpt);
    }
}


static std::string ToSymbol(const RegRef& x) {
    if (x == REGREF_INVALID) {
        return "RegRef::INVALID";
    } else {
        std::stringstream ss;
        ss << "{";
        ss << x.regNum;
        ss << ",";
        ss << x.subRegNum;
        ss << "}";
        return ss.str();
    }
}


static std::string ToSymbol(const SendDescArg& x) {
    std::stringstream ss;
    ss << "{";
    ss << (x.type == SendDescArg::IMM ? "IMM" : "REG32A");
    ss << ",";
    if (x.type == SendDescArg::IMM) {
        ss << ToSymbol(x.reg);
    } else {
        fmtHex(ss, x.imm);
    }
    ss << "}";
    return ss.str();
}


static std::string ToSyntax(Type ot) {
    switch (ot) {
    case Type::UV: return ":uv";
    case Type::UB: return ":ub";
    case Type::UW: return ":uw";
    case Type::UD: return ":ud";
    case Type::UQ: return ":uq";

    case Type::V: return ":v";
    case Type::B: return ":b";
    case Type::W: return ":w";
    case Type::D: return ":d";
    case Type::Q: return ":q";

    case Type::VF: return ":vf";
    case Type::F:  return ":f";
    case Type::NF: return ":nf";
    case Type::HF: return ":hf";
    case Type::DF: return ":df";


    case Type::INVALID: return ":Type::INVALID";
    default: return MakeErrorString("Type", (int)ot);
    }
}


static std::string ToSyntax(MaskCtrl mc) {
    return mc == MaskCtrl::NOMASK ? "W" : "";
}


static std::string ToSyntax(PredCtrl pc) {
    switch (pc) {
    case PredCtrl::NONE: return "";
    case PredCtrl::SEQ: return "";
    case PredCtrl::ANYV: return ".anyv";
    case PredCtrl::ALLV: return ".allv";
    case PredCtrl::ANY2H: return ".any2h";
    case PredCtrl::ALL2H: return ".all2h";
    case PredCtrl::ANY4H: return ".any4h";
    case PredCtrl::ALL4H: return ".all4h";
    case PredCtrl::ANY8H: return ".any8h";
    case PredCtrl::ALL8H: return ".all8h";
    case PredCtrl::ANY16H: return ".any16h";
    case PredCtrl::ALL16H: return ".all16h";
    case PredCtrl::ANY32H: return ".any32h";
    case PredCtrl::ALL32H: return ".all32h";
    default: return MakeErrorString("PredCtrl", (int)pc);
    }
}


static std::string ToSyntax(const Predication& pred) {
    std::stringstream ss;
    ss << (pred.inverse ? "~" : "");
    ss << ToSyntax(pred.function);
    return ss.str();
}


static std::string ToSyntax(ExecSize es) {
    switch (es) {
    case ExecSize::SIMD1: return "1";
    case ExecSize::SIMD2: return "2";
    case ExecSize::SIMD4: return "4";
    case ExecSize::SIMD8: return "8";
    case ExecSize::SIMD16: return "16";
    case ExecSize::SIMD32: return "32";
    default: return MakeErrorString("ExecSize", (int)es);
    }
}


static std::string ToSyntax(ChannelOffset es) {
    switch (es) {
    case ChannelOffset::M0: return "|M0";
    case ChannelOffset::M4: return "|M4";
    case ChannelOffset::M8: return "|M8";
    case ChannelOffset::M12: return "|M12";
    case ChannelOffset::M16: return "|M16";
    case ChannelOffset::M20: return "|M20";
    case ChannelOffset::M24: return "|M24";
    case ChannelOffset::M28: return "|M28";
    default: return MakeErrorString("|ChannelOffset", (int)es);
    }
}


static std::string ToSyntax(FlagModifier fm) {
    switch (fm) {
    case FlagModifier::NONE: return ""; // 0
    case FlagModifier::EQ: return "eq";
    case FlagModifier::NE: return "ne";
    case FlagModifier::LT: return "lt";
    case FlagModifier::LE: return "le";
    case FlagModifier::GE: return "ge";
    case FlagModifier::GT: return "gt";
    case FlagModifier::OV: return "ov"; // 8
    case FlagModifier::UN: return "un";
    case FlagModifier::EO: return "eo";
    default: return MakeErrorString("FlagModifier",(int)fm);
    }
}


static bool IsBitwise(Op op) {
    switch (op) {
    case Op::AND:
    case Op::OR:
    case Op::XOR:
    case Op::NOT:
      return true;
    default:
      return false;
    }
}

static std::string ToSyntax(BranchCntrl bc) {
    switch (bc) {
    case BranchCntrl::OFF: return "";
    case BranchCntrl::ON: return ".b";
    default: return MakeErrorString("BranchCntrl",(int)bc);
    }
}

static std::string ToSyntax(Op op, SrcModifier sm) {
    switch (sm) {
    case SrcModifier::NONE: return "";
    case SrcModifier::ABS: return "(abs)";
    case SrcModifier::NEG: return IsBitwise(op) ? "~" : "-";
    case SrcModifier::NEG_ABS: return "-(abs)";
    default: return MakeErrorString("SrcModifier",(int)sm);
    }
}


static std::string ToSyntax(RegName regName) {
    switch (regName) {
    case RegName::ARF_NULL:  return "null";  // null
    case RegName::ARF_A:     return "a";
    case RegName::ARF_ACC:   return "acc";
    case RegName::ARF_MME:   return "mme";
    case RegName::ARF_F:     return "f";
    case RegName::ARF_CE:    return "ce";
    case RegName::ARF_MSG:   return "msg";
    case RegName::ARF_SP:    return "sp";
    case RegName::ARF_SR:    return "sr";
    case RegName::ARF_CR:    return "cr";
    case RegName::ARF_N:     return "n";
    case RegName::ARF_IP:    return "ip";
    case RegName::ARF_TDR:   return "tdr";
    case RegName::ARF_TM:    return "tm";
    case RegName::ARF_FC:    return "fc";
    case RegName::ARF_DBG:   return "dbg";
    case RegName::GRF_R:     return "r";
    default: return MakeErrorString("RegName",(int)regName);
    }
}


static std::string ToSyntax(MathMacroExt MathMacroReg) {
    switch (MathMacroReg) {
    case MathMacroExt::MME0:  return ".mme0";
    case MathMacroExt::MME1:  return ".mme1";
    case MathMacroExt::MME2:  return ".mme2";
    case MathMacroExt::MME3:  return ".mme3";
    case MathMacroExt::MME4:  return ".mme4";
    case MathMacroExt::MME5:  return ".mme5";
    case MathMacroExt::MME6:  return ".mme6";
    case MathMacroExt::MME7:  return ".mme7";
    case MathMacroExt::NOMME:  return ".nomme";
    case MathMacroExt::INVALID: return "MathMacroExt::INVALID";
    default: return MakeErrorString("MathMacroExt",(int)MathMacroReg);
    }
}


static std::string ToSyntax(const Region &rgn) {
    std::stringstream ss;

    if (rgn.getVt() != Region::Vert::VT_INVALID &&
        rgn.getWi() != Region::Width::WI_INVALID &&
        rgn.getHz() != Region::Horz::HZ_INVALID)
    {
        if (rgn.getVt() == Region::Vert::VT_VxH) {
            ss << "<" << (int)rgn.w << "," << (int)rgn.h << ">";
        } else {
            ss << "<" << (int)rgn.v << ";" <<
                (int)rgn.w << "," << (int)rgn.h << ">";
        }
    } else if (
        rgn.getVt() != Region::Vert::VT_INVALID &&
        rgn.getWi() == Region::Width::WI_INVALID &&
        rgn.getHz() != Region::Horz::HZ_INVALID)
    {
        ss << "<" << (int)rgn.v << ";" << (int)rgn.h << ">";
    } else if (
        rgn.getVt() == Region::Vert::VT_INVALID &&
        rgn.getWi() == Region::Width::WI_INVALID &&
        rgn.getHz() != Region::Horz::HZ_INVALID)
    {
        ss << "<" << (int)rgn.h << ">";
    } else if (rgn == Region::INVALID) {
        ss << "Region::INVALID";
    } else {
        ss << "<0x" << std::hex << (int)rgn.bits << "?>";
    }
    return ss.str();
}


static std::string ToSyntax(const InstOpt &i) {
    switch (i) {
    case InstOpt::ACCWREN:     return "AccWrEn";
    case InstOpt::ATOMIC:      return "Atomic";
    case InstOpt::BREAKPOINT:  return "Breakpoint";
    case InstOpt::COMPACTED:   return "Compacted";
    case InstOpt::EOT:         return "EOT";
    case InstOpt::NOCOMPACT:   return "NoCompact";
    case InstOpt::NODDCHK:     return "NoDDChk";
    case InstOpt::NODDCLR:     return "NoDDClr";
    case InstOpt::NOPREEMPT:   return "NoPreempt";
    case InstOpt::NOSRCDEPSET: return "NoSrcDepSet";
    case InstOpt::SWITCH:      return "Switch";
    default: return MakeErrorString("InstOpt",(int)i);
    }
}


static void ToSyntaxNoBraces(
    std::ostream &os,
    const InstOptSet &instOpts)
{
    static const InstOpt ALL_INST_OPTS[] = {
        InstOpt::ACCWREN,
        InstOpt::ATOMIC,
        InstOpt::BREAKPOINT,
        InstOpt::COMPACTED,
        InstOpt::EOT,
        InstOpt::NOCOMPACT,
        InstOpt::NODDCHK,
        InstOpt::NODDCLR,
        InstOpt::NOPREEMPT,
        InstOpt::NOSRCDEPSET,
        InstOpt::SWITCH
      };


    bool first = true;
    for (size_t i = 0;
        i < sizeof(ALL_INST_OPTS)/sizeof(ALL_INST_OPTS[0]);
        i++)
    {
        if (instOpts.contains(ALL_INST_OPTS[i])) {
            if (first) {
                first = false;
            } else {
                os << ",";
            }
            os << ToSyntax(ALL_INST_OPTS[i]);
        }
    }
}

static std::string ToSyntax(const InstOptSet &inst_opts) {
    std::stringstream ss;

    ss << "{";
    ToSyntaxNoBraces(ss, inst_opts);
    ss << "}";

    return ss.str();
}

} // namespace

#endif // _IGA_IRTYPES_TO_STRING_HPP

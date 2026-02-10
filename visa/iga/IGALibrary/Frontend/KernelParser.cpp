/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "KernelParser.hpp"
#include "../IR/InstBuilder.hpp"
#include "../IR/Types.hpp"
#include "../strings.hpp"
#include "BufferedLexer.hpp"
#include "Floats.hpp"
#include "Lexemes.hpp"
#include "Parser.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

using namespace iga;

static const IdentMap<FlagModifier> FLAGMODS{
    {"lt", FlagModifier::LT},
    {"le", FlagModifier::LE},
    {"gt", FlagModifier::GT},
    {"ge", FlagModifier::GE},
    {"eq", FlagModifier::EQ},
    {"ne", FlagModifier::NE},
    {"ov", FlagModifier::OV},
    {"un", FlagModifier::UN},
    {"eo", FlagModifier::EO},
    // aliased by different operations
    {"ze", FlagModifier::EQ},
    {"nz", FlagModifier::NE},
};
static const IdentMap<FlagModifier> FLAGMODS_LEGACY{
    {"l", FlagModifier::LT}, {"g", FlagModifier::GT}, {"e", FlagModifier::EQ},
    {"o", FlagModifier::OV}, {"u", FlagModifier::UN}, {"z", FlagModifier::EQ},
};
static const IdentMap<Type> SRC_TYPES{
    // dpas types
    {"u1", Type::U1},
    {"u2", Type::U2},
    {"u4", Type::U4},
    {"s2", Type::S2},
    {"s4", Type::S4},
    {"b", Type::B},
    {"ub", Type::UB},
    {"w", Type::W},
    {"uw", Type::UW},
    {"d", Type::D},
    {"ud", Type::UD},
    {"q", Type::Q},
    {"uq", Type::UQ},
    {"hf", Type::HF},
    {"f", Type::F},
    {"df", Type::DF},
    {"v", Type::V},
    {"uv", Type::UV},
    {"vf", Type::VF},
    {"nf", Type::NF},
    {"bf", Type::BF},
    {"bf16", Type::BF},
    {"bf8", Type::BF8},
    {"hf8", Type::HF8},
    {"qf", Type::QF},
    {"tf32", Type::TF32},
    ///////////////////////////////////////////
    // non-standard names
    {"u8", Type::UB},
    {"u16", Type::UW},
    {"u32", Type::UD},
    {"u64", Type::UQ},
    {"s8", Type::B},
    {"s16", Type::W},
    {"s32", Type::D},
    {"s64", Type::Q},
    {"f16", Type::HF},
    {"f32", Type::F},
    {"f64", Type::DF},
    {"e2m1", Type::E2M1},
};
static const IdentMap<Type> DST_TYPES{
    {"b", Type::B},
    {"ub", Type::UB},
    {"w", Type::W},
    {"uw", Type::UW},
    {"d", Type::D},
    {"ud", Type::UD},
    {"q", Type::Q},
    {"uq", Type::UQ},
    //
    {"hf", Type::HF},
    {"bf", Type::BF},
    {"bf16", Type::BF},
    {"bf8", Type::BF8},
    {"hf8", Type::HF8},
    {"qf", Type::QF},
    {"tf32", Type::TF32},
    {"f", Type::F},
    {"df", Type::DF},
    {"nf", Type::NF},
    ///////////////////////////////////////////
    // non-standard names
    {"u8", Type::UB},
    {"u16", Type::UW},
    {"u32", Type::UD},
    {"u64", Type::UQ},
    {"s8", Type::B},
    {"s16", Type::W},
    {"s32", Type::D},
    {"s64", Type::Q},
    {"f16", Type::HF},
    {"f32", Type::F},
    {"f64", Type::DF},
};
static const IdentMap<MathMacroExt> MATHMACROREGS = {
    {"mme0", MathMacroExt::MME0},   {"mme1", MathMacroExt::MME1},
    {"mme2", MathMacroExt::MME2},   {"mme3", MathMacroExt::MME3},
    {"mme4", MathMacroExt::MME4},   {"mme5", MathMacroExt::MME5},
    {"mme6", MathMacroExt::MME6},   {"mme7", MathMacroExt::MME7},
    {"nomme", MathMacroExt::NOMME},
};
static const IdentMap<MathMacroExt> MATHMACROREGS_OLDSTYLE = {
    {"acc2", MathMacroExt::MME0},   {"acc3", MathMacroExt::MME1},
    {"acc4", MathMacroExt::MME2},   {"acc5", MathMacroExt::MME3},
    {"acc6", MathMacroExt::MME4},   {"acc7", MathMacroExt::MME5},
    {"acc8", MathMacroExt::MME6},   {"acc9", MathMacroExt::MME7},
    {"noacc", MathMacroExt::NOMME},
};

GenParser::GenParser(const Model &model, InstBuilder &handler,
                     const std::string &inp, ErrorHandler &eh,
                     const ParseOpts &pots)
    : Parser(inp, eh), m_model(model), m_builder(handler), m_opts(pots) {
  initSymbolMaps();
}

bool GenParser::LookupReg(const std::string &str, const RegInfo *&ri,
                          int &reg) {
  ri = nullptr;
  reg = 0;

  // given something like "r13", parse the "r"
  // "cr0" -> "cr"
  size_t len = 0;
  while (len < str.length() && !isdigit(str[len]))
    len++;
  if (len == 0)
    return false;
  auto itr = m_regmap.find(str.substr(0, len));
  if (itr == m_regmap.end()) {
    return false;
  }
  ri = itr->second;
  reg = 0;
  if (ri->numRegs > 0) {
    // if it's a numbered register like "r13" or "cr1", then
    // parse the number part
    size_t off = len;
    while (off < str.size() && isdigit(str[off])) {
      char c = str[off++];
      reg = 10 * reg + c - '0';
    }
    if (off < str.size()) {
      // we have something like "r13xyz"; we don't treat this
      // as a register, but fallback so it can be treated as an
      // identifier (an immediate reference)
      reg = 0;
      ri = nullptr;
      return false;
    }
  } // else it was something like "null" or "ip"
    // either way, we are done
  return true;
}

bool GenParser::PeekReg(const RegInfo *&regInfo, int &regNum) {
  const Token &tk = Next();
  if (tk.lexeme != IDENT) {
    return false;
  } else if (LookupReg(GetTokenAsString(tk), regInfo, regNum)) {
    // helpful translation that permits acc2-acc9 and translates them
    // to mme0-7 with a warning (or whatever they map to on the given
    // platform
    if (regInfo->regName == RegName::ARF_ACC && regNum >= regInfo->numRegs) {
      const RegInfo *mme = m_model.lookupRegInfoByRegName(RegName::ARF_MME);
      IGA_ASSERT(mme != nullptr, "unable to find MMR on this platform");
      if (mme) {
        // switch acc to mme
        regInfo = mme;
        // adjust the reg num (e.g. acc2 -> mme0 on GEN8)
        regNum -= mme->regNumBase;
        WarningAtT(tk.loc,
                   "old-style access to mme via acc "
                   "(use mme",
                   regNum, " for acc", regNum + mme->regNumBase, ")");
      }
    }
    if (!regInfo->isRegNumberValid(regNum)) {
      WarningS(tk.loc, "register number out of bounds");
    }
    return true;
  } else {
    return false;
  }
}

bool GenParser::ConsumeReg(const RegInfo *&regInfo, int &regNum) {
  const Token &tk = Next();
  if (tk.lexeme != IDENT) {
    return false;
  }
  if (PeekReg(regInfo, regNum)) {
    Skip();
    return true;
  } else {
    return false;
  }
}

static bool isFloating(const ImmVal &v) {
  switch (v.kind) {
  case ImmVal::Kind::F16:
  case ImmVal::Kind::F32:
  case ImmVal::Kind::F64:
    return true;
  default:
    return false;
  }
}
static bool isSignedInt(const ImmVal &v) {
  switch (v.kind) {
  case ImmVal::Kind::S8:
  case ImmVal::Kind::S16:
  case ImmVal::Kind::S32:
  case ImmVal::Kind::S64:
    return true;
  default:
    return false;
  }
}
static bool isUnsignedInt(const ImmVal &v) {
  switch (v.kind) {
  case ImmVal::Kind::U8:
  case ImmVal::Kind::U16:
  case ImmVal::Kind::U32:
  case ImmVal::Kind::U64:
    return true;
  default:
    return false;
  }
}
static bool isIntegral(const ImmVal &v) {
  return isSignedInt(v) || isUnsignedInt(v);
}

// expression parsing
// &,|
// <<,>>
// +,-
// *,/,%
// -(unary neg)
bool GenParser::TryParseConstExpr(ImmVal &v) {
  return TryParseConstExpr(ExprParseOpts(), v);
}
bool GenParser::TryParseConstExpr(const ExprParseOpts &pos, ImmVal &v) {
  return parseBitwiseExpr(pos, false, v);
}
//
bool GenParser::TryParseIntConstExpr(ImmVal &v, const char *forWhat) {
  return TryParseIntConstExpr(ExprParseOpts(), v, forWhat);
}
bool GenParser::TryParseIntConstExpr(const ExprParseOpts &pos, ImmVal &v,
                                     const char *forWhat) {
  // allow floats in the parse, but check the result (better error)
  Loc loc = NextLoc();
  ExprParseOpts epos = pos; // allow floats, throw tantrum at end
  epos.allowFloat = true;
  bool z = TryParseConstExpr(epos, v);
  if (!z) {
    return false;
  } else if (!isIntegral(v)) {
    std::stringstream ss;
    if (forWhat) {
      ss << forWhat << " must be a constant integer expression";
    } else {
      ss << "expected constant integer expression";
    }
    FailS(loc, ss.str());
  }
  return true;
}
bool GenParser::TryParseIntConstExprAdd(ImmVal &v, const char *forWhat) {
  return TryParseIntConstExprPrimary(ExprParseOpts(), v, forWhat);
}
bool GenParser::TryParseIntConstExprAdd(const ExprParseOpts &pos, ImmVal &v,
                                        const char *forWhat) {
  Loc loc = NextLoc();
  ExprParseOpts epos = pos; // allow floats, throw tantrum at end
  epos.allowFloat = true;
  bool z = parseAddExpr(epos, false, v);
  if (!z) {
    return false;
  } else if (!isIntegral(v)) {
    std::stringstream ss;
    if (forWhat) {
      ss << forWhat << " must be a constant integer expression";
    } else {
      ss << "expected constant integer expression";
    }
    FailS(loc, ss.str());
  }
  return true;
}
bool GenParser::TryParseIntConstExprPrimary(ImmVal &v, const char *forWhat) {
  return TryParseIntConstExprPrimary(ExprParseOpts(), v, forWhat);
}
bool GenParser::TryParseIntConstExprPrimary(const ExprParseOpts &pos, ImmVal &v,
                                            const char *forWhat) {
  Loc loc = NextLoc();
  ExprParseOpts epos = pos; // allow floats, throw tantrum at end
  epos.allowFloat = true;
  bool z = parsePrimaryExpr(epos, false, v);
  if (!z) {
    return false;
  } else if (!isIntegral(v)) {
    std::stringstream ss;
    if (forWhat) {
      ss << forWhat << " must be a constant integer expression";
    } else {
      ss << "expected constant integer expression";
    }
    FailS(loc, ss.str());
  }
  return true;
}
bool GenParser::TryParseIntConstExprAddChain(const ExprParseOpts &pos,
                                             ImmVal &v, const char *forWhat) {
  Loc loc = NextLoc();
  ExprParseOpts epos = pos; // allow floats, throw tantrum at end
  epos.allowFloat = true;

  // treat this as
  // 0 + .... (or 0 - ...)
  v = (int64_t)0;
  while (LookingAtAnyOf(ADD, SUB)) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseMulExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  if (!isIntegral(v)) {
    std::stringstream ss;
    if (forWhat) {
      ss << forWhat << " must be a constant integer expression";
    } else {
      ss << "expected constant integer expression";
    }
    FailS(loc, ss.str());
  }
  return true;
}
bool GenParser::TryParseIntConstExprAddChain(ImmVal &v, const char *forWhat) {
  return TryParseIntConstExprAddChain(ExprParseOpts(), v, forWhat);
}

void GenParser::ensureIntegral(const Token &t, const ImmVal &v) {
  if (!isIntegral(v)) {
    FailS(t.loc, "argument to operator must be integral");
  }
}
void GenParser::checkNumTypes(const ImmVal &v1, const Token &op,
                              const ImmVal &v2) {
  if (isFloating(v1) && !isFloating(v2)) {
    FailAtT(op.loc, "right operand to ", GetTokenAsString(op),
            " must be floating point");
  } else if (isFloating(v2) && !isFloating(v1)) {
    FailAtT(op.loc, "left operand to ", GetTokenAsString(op),
            " must be floating point");
  }
}
// target must be float
void GenParser::checkIntTypes(const ImmVal &v1, const Token &op,
                              const ImmVal &v2) {
  if (isFloating(v1)) {
    FailAtT(op.loc, "left operand to ", GetTokenAsString(op),
            " must be integral");
  } else if (isFloating(v2)) {
    FailAtT(op.loc, "right operand to ", GetTokenAsString(op),
            " must be integral");
  }
}

ImmVal GenParser::evalBinExpr(const ImmVal &v1, const Token &op,
                              const ImmVal &v2) {
  bool isF = isFloating(v1) || isFloating(v2);
  bool isU = isUnsignedInt(v1) || isUnsignedInt(v2);
  ImmVal result = v1;
  switch (op.lexeme) {
  case AMP:
  case CIRC:
  case PIPE:
  case LSH:
  case RSH:
  case MOD:
    // integral only operations
    checkIntTypes(v1, op, v2);
    switch (op.lexeme) {
    case AMP:
      if (isU) {
        result.u64 &= v2.u64;
      } else {
        result.s64 &= v2.s64;
      }
      break;
    case CIRC:
      if (isU) {
        result.u64 ^= v2.u64;
      } else {
        result.s64 ^= v2.s64;
      }
      break;
    case PIPE:
      if (isU) {
        result.u64 |= v2.u64;
      } else {
        result.s64 |= v2.s64;
      }
      break;
    case LSH:
      if (isU) {
        result.u64 <<= v2.u64;
      } else {
        result.s64 <<= v2.s64;
      }
      break;
    case RSH:
      if (isU) {
        result.u64 >>= v2.u64;
      } else {
        result.s64 >>= v2.s64;
      }
      break;
    case MOD:
      if (isU) {
        result.u64 %= v2.u64;
      } else {
        result.s64 %= v2.s64;
      }
      break;
    default:
      break;
    }
    break;
  case ADD:
  case SUB:
  case MUL:
  case DIV:
    checkNumTypes(v1, op, v2);
    switch (op.lexeme) {
    case ADD:
      if (isF) {
        result.f64 += v2.f64;
      } else if (isU) {
        result.u64 += v2.u64;
      } else {
        result.s64 += v2.s64;
      }
      break;
    case SUB:
      if (isF) {
        result.f64 -= v2.f64;
      } else if (isU) {
        result.u64 -= v2.u64;
      } else {
        result.s64 -= v2.s64;
      }
      break;
    case MUL:
      if (isF) {
        result.f64 *= v2.f64;
      } else if (isU) {
        result.u64 *= v2.u64;
      } else {
        result.s64 *= v2.s64;
      }
      break;
    case DIV:
      if (isF) {
        result.f64 /= v2.f64;
      } else {
        if (v2.u64 == 0) {
          FailS(op.loc, "(integral) division by zero");
        }
        if (isU) {
          result.u64 /= v2.u64;
        } else {
          result.s64 /= v2.s64;
        }
      }
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return result;
}

bool GenParser::parseBitwiseExpr(const ExprParseOpts &pos, bool consumed,
                                 ImmVal &v) {
  return parseBitwiseORExpr(pos, consumed, v);
}
// E -> E ('|' E)*
bool GenParser::parseBitwiseORExpr(const ExprParseOpts &pos, bool consumed,
                                   ImmVal &v) {
  if (!parseBitwiseXORExpr(pos, consumed, v)) {
    return false;
  }
  while (LookingAt(PIPE)) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseBitwiseXORExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  return true;
}
// E -> E ('^' E)*
bool GenParser::parseBitwiseXORExpr(const ExprParseOpts &pos, bool consumed,
                                    ImmVal &v) {
  if (!parseBitwiseANDExpr(pos, consumed, v)) {
    return false;
  }
  while (LookingAt(CIRC)) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseBitwiseANDExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  return true;
}
// E -> E ('&' E)*
bool GenParser::parseBitwiseANDExpr(const ExprParseOpts &pos, bool consumed,
                                    ImmVal &v) {
  if (!parseShiftExpr(pos, consumed, v)) {
    return false;
  }
  while (LookingAt(AMP)) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseShiftExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  return true;
}

// E -> E (('<<'|'>>') E)*
bool GenParser::parseShiftExpr(const ExprParseOpts &pos, bool consumed,
                               ImmVal &v) {
  if (!parseAddExpr(pos, consumed, v)) {
    return false;
  }
  while (LookingAtAnyOf(LSH, RSH)) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseAddExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  return true;
}
// E -> E (('+'|'-') E)*
bool GenParser::parseAddExpr(const ExprParseOpts &pos, bool consumed,
                             ImmVal &v) {
  if (!parseMulExpr(pos, consumed, v)) {
    return false;
  }
  while (LookingAtAnyOf(ADD, SUB)) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseMulExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  return true;
}

// E -> E (('*'|'/'|'%') E)*
bool GenParser::parseMulExpr(const ExprParseOpts &pos, bool consumed,
                             ImmVal &v) {
  if (!parseUnExpr(pos, consumed, v)) {
    return false;
  }
  while (LookingAtAnyOf({MUL, DIV, MOD})) {
    Token t = Next();
    Skip();
    ImmVal r;
    parseUnExpr(pos, true, r);
    v = evalBinExpr(v, t, r);
  }
  return true;
}

// E -> ('-'|'~') E
bool GenParser::parseUnExpr(const ExprParseOpts &pos, bool consumed,
                            ImmVal &v) {
  if (!LookingAtAnyOf(SUB, TILDE)) {
    if (!parsePrimaryExpr(pos, consumed, v)) {
      return false;
    }
  } else {
    Token t = Next();
    Skip();
    parsePrimaryExpr(pos, true, v);
    switch (t.lexeme) {
    case SUB:
      v.Negate();
      break;
    case TILDE:
      ensureIntegral(t, v);
      v.u64 = ~v.u64;
      break;
    default:
      break;
    }
  }
  return true;
}

// special symbol (e.g. nan, inf, ...)
// grouped expression (E)
// literal
bool GenParser::parsePrimaryExpr(const ExprParseOpts &pos, bool consumed,
                                 ImmVal &v) {
  Token t = Next();
  bool isQuietNaN = false;
  if (pos.allowFloat && LookingAtIdentEq(t, "nan")) {
    WarningT("nan is deprecated, us snan(...) or qnan(...)");
    v.kind = ImmVal::Kind::F64;
    v.f64 = std::numeric_limits<double>::signaling_NaN();
    Skip();
  } else if (pos.allowFloat && ((isQuietNaN = LookingAtIdentEq(t, "qnan")) ||
                                LookingAtIdentEq(t, "snan"))) {
    auto nanSymLoc = NextLoc();
    Skip();
    if (Consume(LPAREN)) {
      auto payloadLoc = NextLoc();
      ImmVal payload;
      payload.u64 = 0;
      parseBitwiseExpr(pos, true, payload);
      if (payload.u64 >= F64_QNAN_BIT) {
        FailS(payloadLoc, "NaN payload overflows");
      } else if (payload.u64 == 0 && !isQuietNaN) {
        // signaling NaN has a 0 in the high mantissa, so we must
        // ensure that at least one bit in the mantissa is set
        // otherwise the entire mantissa is 0's and the pattern
        // would be an "infinity" pattern, not a NaN
        FailS(payloadLoc, "NaN payload must be nonzero for snan");
      }
      ConsumeOrFail(RPAREN, "expected )");
      v.u64 = payload.u64 | F64_EXP_MASK;
      if (isQuietNaN) {
        v.u64 |= F64_QNAN_BIT;
      }
    } else {
      WarningS(nanSymLoc, "bare qnan and snan tokens deprecated"
                          " (pass in a valid payload)");
      v.u64 = F64_EXP_MASK;
      if (isQuietNaN) {
        v.u64 |= F64_QNAN_BIT; // the quiet bit suffices
      } else {
        v.u64 |= 1; // set something other than the quiet bit
                    // non-zero in the payload
      }
    }
    v.kind = ImmVal::Kind::F64;
  } else if (pos.allowFloat && LookingAtIdentEq(t, "inf")) {
    v.f64 = std::numeric_limits<double>::infinity();
    v.kind = ImmVal::Kind::F64;
    Skip();
  } else if (pos.allowFloat && LookingAt(FLTLIT)) {
    ParseFltFrom(t.loc, v.f64);
    v.kind = ImmVal::Kind::F64;
    Skip();
  } else if (LookingAtAnyOf({INTLIT02, INTLIT10, INTLIT16})) {
    // we parse as unsigned, but tag as signed for negation etc...
    ParseIntFrom<uint64_t>(t.loc, v.u64);
    v.kind = ImmVal::Kind::S64;
    Skip();
  } else if (Consume(LPAREN)) {
    // (E)
    parseBitwiseExpr(pos, true, v);
    Consume(RPAREN);
  } else if (LookingAt(IDENT)) {
    // TEST CASES
    // LABEL:
    // // jmpi  LABEL                    // passes
    // // goto (16) (2 + LABEL) LABEL    // fails
    // // goto (16) LABEL LABEL          // passes
    // // join (16) LABEL                // passes
    // // mov (1) r65:ud LABEL:ud        // fails
    // // mov (1) r65:ud (2 + LABEL):ud  // fails (poor diagnostic)
    if (!pos.symbols.empty()) {
      std::string str = GetTokenAsString();
      auto itr = pos.symbols.find(str);
      if (itr != pos.symbols.end()) {
        v = itr->second;
        return true;
      }
    }
    if (consumed)
      FailT("unbound identifier");
    return false;
  } else {
    // something else: error unless we haven't consumed anything
    if (consumed) {
      FailT("syntax error in constant expression");
    }
    return false;
  }
  return true;
} // parsePrimary

void GenParser::initSymbolMaps() {
  // map the register names
  // this maps just the non-number part.
  // e.g. with cr0, this maps "cr"; see LookupReg()
  int tableLen;
  const RegInfo *table = GetRegisterSpecificationTable(tableLen);
  for (int i = 0; i < tableLen; i++) {
    const RegInfo *ri = table + i;
    if (ri->supportedOn(platform())) {
      m_regmap[ri->syntax] = ri;
    }
  }
}

class KernelParser : GenParser {
  // maps mnemonics and registers for faster lookup
  std::unordered_map<std::string, const OpSpec *> opmap;

  ExecSize m_defaultExecutionSize;
  Type m_defaultRegisterType;

  // instruction state
  const OpSpec *m_opSpec = nullptr;
  bool m_hasWrEn = false;
  Type m_unifType = {};
  const Token *m_unifTypeTk = nullptr;
  RegRef m_flagReg;
  ExecSize m_execSize = {};
  ChannelOffset m_chOff = {};
  Loc m_execSizeLoc;
  Loc m_mnemonicLoc;
  Operand::Kind m_srcKinds[3] = {};
  Loc m_srcLocs[3];
  int m_sendSrcLens[2];    // send message lengths
  Loc m_sendSrcLenLocs[2]; // locations so we can referee
  bool m_implicitExBSO = false;    // send src1 suffixed with length implies exBSO, but
                           // ensure the inst opt is given

private:
  void addNullGatherSendSrc1() {
    bool checkForBadSrc1 = true;
    if (checkForBadSrc1 && LookingAt(IDENT)) {
      // friendly error if the user includes src1 on a gather send
      // send... ... r[s0..]  null:0  ExDesc Desc
      //                      ^^^^^^ oops
      // send... ... r[s0..]  a0.2         Desc
      //                      ^^^^ okay (a0.2 is the ExDesc not src1)
      // sendg... ... r[s0..]  s0.0  s0.16  Desc
      //                       ^^^^ okay (s0.0 is ID0 not src1)
      auto at = NextLoc();
      const RegInfo *regInfo = nullptr;
      int regNum = 0;
      m_lexer.Mark();
      if (ConsumeReg(regInfo, regNum) &&
        (regInfo->regName == RegName::GRF_R ||
          regInfo->regName == RegName::ARF_NULL))
      {
        FailAtT(at, "src1 operand forbidden for Gathering Sends");
      }
      m_lexer.Reset();
    }

    // For instructions those have implicit types, match the type to
    // the expected one. Otherwise, ARF_NULL operand should have Type::INVALID
    Type type = Type::INVALID;
    m_opSpec->implicitSrcTypeVal(1, false, type);
    m_builder.InstSrcOpRegDirect(1,
                                 m_srcLocs[0], // use src0 loc
                                 SrcModifier::NONE, RegName::ARF_NULL,
                                 REGREF_ZERO_ZERO, Region::INVALID, type);
  }

  // A helper function to check if the instruction under parsing is a gather
  // send. This function is only meaningful after opSpec and src0 is parsed
  bool isParsingGatherSend() {
    IGA_ASSERT(m_opSpec && m_srcKinds[0] != Operand::Kind::INVALID,
               "isParsingGatherSend cannot be called before src0 is parsed");
    return m_opSpec && m_opSpec->isAnySendFormat() &&
           m_srcKinds[0] == Operand::Kind::INDIRECT;
  }

public:
  KernelParser(const Model &model, InstBuilder &handler, const std::string &inp,
               ErrorHandler &eh, const ParseOpts &pots)
      : GenParser(model, handler, inp, eh, pots),
        m_defaultExecutionSize(ExecSize::SIMD1),
        m_defaultRegisterType(Type::INVALID) {
    initSymbolMaps();
  }

  void ParseListing() { ParseProgram(); }

  void initSymbolMaps() {
    // map mnemonics names to their ops
    // subops only get mapped by their fully qualified names in this pass
    for (const OpSpec *os : m_model.ops()) {
      if (os->isValid()) {
        opmap[os->mnemonic] = os;
      }
    }
  }

  // Program = (Label? Insts* (Label Insts))?
  void ParseProgram() {
    m_builder.ProgramStart();

    // parse default type directives
    if (m_opts.supportLegacyDirectives) {
      // e.g. .default_execution_size...
      ParseLegacyDirectives();
    }

    // first block doesn't need a label
    if (!LookingAtLabelDef() && !EndOfFile()) {
      ParseBlock(NextLoc(), ""); // unnamed
    }
    // successive blocks need a label
    std::string label;
    Loc lblLoc = NextLoc();
    while (ConsumeLabelDef(label)) {
      ParseBlock(lblLoc, label);
      lblLoc = NextLoc();
    }
    if (!EndOfFile()) {
      FailT("expected instruction, block, or EOF");
    }

    m_builder.ProgramEnd();
  }

  // fix to support .default_execution_size
  bool ParseLegacyDirectives() {
    int parsed = 0;
    try {
      while (LookingAtSeq(Lexeme::DOT, Lexeme::IDENT)) {
        Skip();
        parsed++;
        if (ConsumeIdentEq("default_execution_size")) {
          Consume(LPAREN);
          auto loc = NextLoc();
          int dftExecSize;
          if (!ConsumeIntLit<int>(dftExecSize)) {
            FailT("expected SIMD width (integral value)");
          }
          if (dftExecSize != 1 && dftExecSize != 2 && dftExecSize != 4 &&
              dftExecSize != 8 && dftExecSize != 16 && dftExecSize != 32) {
            FailS(loc, "invalid default execution size; "
                       "must be 1, 2, 4, 8, 16, 32");
          }
          m_defaultExecutionSize = (ExecSize)dftExecSize;
          Consume(RPAREN);
          Consume(NEWLINE);
        } else if (ConsumeIdentEq("default_register_type")) {
          m_defaultRegisterType = TryParseOpType(DST_TYPES);
          if (m_defaultRegisterType == Type::INVALID) {
            FailT("expected default register type");
          }
          Consume(NEWLINE);
        } else {
          FailT("unexpected directive name");
        }
      } // while
    } catch (const SyntaxError &s) {
      RecoverFromSyntaxError(s);
      // returning true will make ParseBlock restart the loop
      // (so more directives get another shot)
      return true;
    }
    return parsed > 0;
  }

  void RecoverFromSyntaxError(const SyntaxError &s) {
    // record the error in this instruction
    m_errorHandler.reportError(s.loc, s.message);
    // bail if we've reached the max number of errors
    if (m_errorHandler.getErrors().size() >= m_opts.maxSyntaxErrors) {
      throw s;
    }
    // attempt resync by skipping until we pass a newline
    // DumpLookaheads();
    while (!Consume(Lexeme::NEWLINE) &&
           // !Consume(Lexeme::SEMI) &&
           //   don't sync on ; since this is typically part of a
           //   region rather than an operand separator
           //  ...     r12.0<8;8,1>:d
           //  ^ error        ^ syncs here otherwise
           !LookingAt(Lexeme::END_OF_FILE)) {
      // DumpLookaheads();
      Skip();
    }
  }

  void ParseBlock(const Loc &lblLoc, const std::string &label) {
    m_builder.BlockStart(lblLoc, label);
    auto lastInst = NextLoc();
    while (true) {
      if (Consume(Lexeme::NEWLINE) || Consume(Lexeme::SEMI)) {
        continue;
      } else if (m_opts.supportLegacyDirectives && ParseLegacyDirectives()) {
        // .default_execution_size...
        continue;
      }
      // if we make it here, then we are not looking at a legacy
      // directive and we are not looking at a newline
      if (LookingAtLabelDef() || EndOfFile()) {
        break;
      }
      // not at EOF and not looking at the beginning of new block
      // so we better be looking at an instruction
      ParseInst();
      lastInst = NextLoc(-1); // peek at the end of the last instruction
    }

    m_builder.BlockEnd(ExtentTo(lblLoc, lastInst));
  }

  void ParseInst() {
    try {
      // parse one instruction
      // DumpLookaheads();
      ParseInstCanThrowException();
    } catch (SyntaxError &s) {
      // we resync here and return from ParseInst
      RecoverFromSyntaxError(s);
    }
  }

  // reset KernelParser information
  void reset() {
    m_flagReg = REGREF_INVALID;
    m_execSizeLoc = Loc::INVALID;
    m_opSpec = nullptr;
    m_unifType = Type::INVALID;
    m_unifTypeTk = nullptr;
    m_mnemonicLoc = Loc::INVALID;
    for (size_t i = 0; i < sizeof(m_srcKinds) / sizeof(m_srcKinds[0]); i++) {
      m_srcLocs[i] = Loc::INVALID;
      m_srcKinds[i] = Operand::Kind::INVALID;
    }
    for (size_t i = 0; i < sizeof(m_sendSrcLens) / sizeof(m_sendSrcLens[0]);
         i++) {
      m_sendSrcLens[i] = -1;
      m_sendSrcLenLocs[i] = Loc::INVALID;
    }
    m_implicitExBSO = false;
  }

  // try if the instruction is an inline-binary-instruction
  // return true if it is
  bool tryParseInlineBinaryInst() {
    if (!Consume(Lexeme::DOT))
      return false;
    if (!ConsumeIdentEq("inline_inst"))
      return false;

    // .inline_inst 0x123 0x123 0x123 0x123
    Instruction::InlineBinaryType val = {0};
    for (size_t i = 0; i < 4; ++i) {
      if (LookingAt(INTLIT16)) {
        ParseIntFrom<uint32_t>(NextLoc(), val[i]);
        Skip();
      } else {
        FailT("Inline binary instruction must contain four 32b hexadecimal "
              "values");
      }
    }
    m_builder.InstInlineBinary(val);
    // set ILLEGAL as inline-binary-instruction's opSpec so that other passes
    // such as SWSBSetter can bypass it
    m_opSpec = &m_model.lookupOpSpec(Op::ILLEGAL);
    m_builder.InstOp(m_opSpec);
    return true;
  }

  void ParseRegularInst() {
    // (W&~f0) mov (8|M0) r1 r2
    // ^
    ParseWrEnPred();

    // (W&~f0) mov (8|M0) r1 r2
    //         ^
    //
    //         math.sqrt (8|M0) ...
    //         ^
    m_mnemonicLoc = NextLoc();
    m_opSpec = ParseMnemonic();
    if (m_opSpec) {
      // looking at a regular instruction (non special-ld-st inst)
      m_builder.InstOp(m_opSpec);
      FinishNonLdStInstBody();
    } else if (!ParseLdStInst()) {
      FailS(m_mnemonicLoc, "invalid mnemonic");
    }

    // .... {...}
    ParseInstOpts();
  }

  // Instruction = RegularInst | LdStInst
  // RegularInst = Predication? Mnemonic UniformType? EMask
  //                 ConditionModifier? Operands InstOptions?
  // LdStInst = ... comes from Sends/Interface.hpp ...
  void ParseInstCanThrowException() {
    // ShowCurrentLexicalContext();
    const Loc startLoc = NextLoc();
    m_builder.InstStart(startLoc);
    reset();

    if (!tryParseInlineBinaryInst())
      ParseRegularInst();

    if (!LookingAt(Lexeme::NEWLINE) && !LookingAt(Lexeme::SEMI) &&
        !LookingAt(Lexeme::END_OF_FILE)) {
      FailAfterPrev("expected newline, ';', or EOF");
    }

    m_builder.InstEnd(ExtentToPrevEnd(startLoc));
  }

  // ExecInfo = '(' ExecSize EmOffNm? ')'
  //   where EmOffNm = '|' EmOff  (',' 'NM')?
  //                 | '|' 'NM'
  //         EmOff = 'M0' | 'M4' | ...
  //         ExecSize = '1' | '2' | ... | '32'
  void ParseExecInfo(ExecSize dftExecSize, ExecSize &execSize,
                     ChannelOffset &chOff) {
    Loc execSizeLoc = m_execSizeLoc = NextLoc(0);
    Loc execOffsetLoc = NextLoc(0);
    // we are careful here since we might have things like:
    //    jmpi        (1*16)
    //    jmpi (1|M0) ...
    //    jmpi (1)    ...
    // We resolve that by looking ahead two symbols
    int execSizeVal = 1;
    if (LookingAt(LPAREN) &&
        (LookingAtFrom(2, RPAREN) || LookingAtFrom(2, PIPE))) {
      Skip();
      execSizeLoc = NextLoc();
      ConsumeIntLitOrFail(execSizeVal, "expected SIMD width");

      if (Consume(PIPE)) {
        static const IdentMap<ChannelOffset> EM_OFFS{
            {"M0", ChannelOffset::M0},   {"M4", ChannelOffset::M4},
            {"M8", ChannelOffset::M8},   {"M12", ChannelOffset::M12},
            {"M16", ChannelOffset::M16}, {"M20", ChannelOffset::M20},
            {"M24", ChannelOffset::M24}, {"M28", ChannelOffset::M28}};
        execOffsetLoc = NextLoc();
        ConsumeIdentOneOfOrFail(EM_OFFS, chOff, "expected ChOff",
                                "invalid ChOff");
        // if (m_chOff % m_execSize != 0) {
        //     Fail(execOffsetLoc,
        //         "invalid execution mask offset for execution size");
        // } else if (m_chOff + m_execSize > 32) {
        //     Fail(execOffsetLoc,
        //         "invalid execution mask offset for execution size");
        // }
      } else {
        chOff = ChannelOffset::M0;
      }
      ConsumeOrFail(RPAREN, "expected )");
    } else {
      if (m_opSpec && m_opSpec->hasImpicitEm()) {
        chOff = ChannelOffset::M0;
        execSizeVal = 1;
      } else if (m_opts.supportLegacyDirectives) {
        chOff = ChannelOffset::M0;
        execSizeVal = (int)dftExecSize;
      } else {
        FailT("expected '(' (start of execution size info)");
      }
    }

    switch (execSizeVal) {
    case 1:
      execSize = ExecSize::SIMD1;
      break;
    case 2:
      execSize = ExecSize::SIMD2;
      break;
    case 4:
      execSize = ExecSize::SIMD4;
      break;
    case 8:
      execSize = ExecSize::SIMD8;
      break;
    case 16:
      execSize = ExecSize::SIMD16;
      break;
    case 32:
      execSize = ExecSize::SIMD32;
      break;
    default:
      FailT("invalid SIMD width");
    }

    m_builder.InstExecInfo(execSizeLoc, execSize, execOffsetLoc, chOff);
  }

  Type SendOperandDefaultType(int srcIx) const {
    auto t = srcIx == 1 ? Type::INVALID : Type::UD;
    if (srcIx < 0) {
      if (m_opSpec->hasImplicitDstType())
        t = m_opSpec->implicitDstType();
    } else {
      if (m_opSpec->hasImplicitSrcType(srcIx, false))
        t = m_opSpec->implicitSrcType(srcIx, false);
    }
    return t;
  }

  Type ParseSendOperandTypeWithDefault(int srcIx) {
    // sends's second parameter doesn't have a valid type
    Type t = Type::INVALID;
    if (Consume(COLON)) {
      if (!LookingAt(IDENT)) {
        FailT("expected a send operand type");
      }
      if (!IdentLookupFrom(0, DST_TYPES, t)) {
        FailT("unexpected operand type for send");
      }
      Skip();
    } else {
      t = SendOperandDefaultType(srcIx);
    }
    return t;
  }

  bool ParseLdStInst();
  void ParseLdStOpControls(Loc mneLoc, const SendOpDefinition &opInfo,
                           VectorMessageArgs &vma);

  // e.g.  add (8|M8) ...
  //           ^
  // e.g.  nop ...
  //           ^
  void FinishNonLdStInstBody() {
    // if (Consume(COLON)) {
    //    m_unifTypeTk = &Next(0);
    //    ConsumeIdentOneOfOrFail(
    //        dstTypes,
    //        m_unifType,
    //        "expected uniform type",
    //        "invalid uniform type");
    // }

    // (W&~f0) mov (8|M0) (le)f0.0  r1:f  r2:f
    //             ^
    ChannelOffset chOff;
    ParseExecInfo(m_defaultExecutionSize, m_execSize, chOff); // sets m_execSize

    ParseFlagModOpt();
    switch (m_opSpec->format) {
    case OpSpec::NULLARY:
      // nop ...
      // illegal ...
      // thryld ...
      break; // fallthrough to instruction options
    case OpSpec::BASIC_UNARY_REG:
    case OpSpec::BASIC_UNARY_REGIMM:
      ParseDstOp();
      ParseSrcOp(0);
      if (m_opSpec->format == OpSpec::BASIC_UNARY_REG &&
          m_srcKinds[0] != Operand::Kind::DIRECT &&
          m_srcKinds[0] != Operand::Kind::INDIRECT) {
        FailS(m_srcLocs[0], "src0 must be a register");
      }
      break;
    case OpSpec::SYNC_UNARY:
      // implicit destination
      ParseSyncSrc0Op();
      if (m_model.supportsWait() && m_srcKinds[0] != Operand::Kind::DIRECT) {
        FailS(m_srcLocs[0], "src0 must be a notification register");
      }
      break;
    case OpSpec::SEND_UNARY:
      // <=Gen11
      ParseSendDstOp();
      ParseSendSrcOp(0, false);
      ParseSendDescsLegacy();
      break;
    case OpSpec::SEND_BINARY:
      if (platform() <= Platform::XE) {
        ParseSendInstructionLegacy();
      } else if (m_opSpec->isSendgFormat()) {
        ParseSendgInstructionXe3p();
      } else if (platform() >= Platform::XE3) {
        ParseSendInstructionXe3();
      } else if (platform() == Platform::XE2) {
        ParseSendInstructionXe2();
      } else if (platform() >= Platform::XE_HP) {
        ParseSendInstructionXeHP();
      } else {
        IGA_ASSERT_FALSE("invalid format for platform");
      }
      break;
    case OpSpec::BASIC_BINARY_REG_IMM:
    case OpSpec::BASIC_BINARY_REG_REG:
    case OpSpec::BASIC_BINARY_REG_REGIMM:
      ParseDstOp();
      ParseSrcOp(0);
      ParseSrcOp(1);
      break;
    case OpSpec::MATH_BINARY_REG_REGIMM:
      ParseDstOp();
      ParseSrcOp(0);
      if (m_opSpec->getSourceCount(m_builder.getSubfunction()) > 1) {
        // math sometimes has only one operand
        ParseSrcOp(1);
      }
      break;
    case OpSpec::TERNARY_REGIMM_REG_REGIMM:
      if (m_opSpec->isDpasFormat()) {
        ParseDpasOp(-1, true); // dst
        ParseDpasOp(0, false); // src0
        ParseDpasOp(1, false); // src1
        ParseDpasOp(2, false); // src2
        break;
      }
      ParseDstOp();
      ParseSrcOp(0);
      ParseSrcOp(1);
      ParseSrcOp(2);
      break;
    case OpSpec::QUINARY_SRC:
      assert(m_opSpec->is(Op::BDPAS) && "only bdpas has 5 sources");
      ParseDpasOp(-1, true); // dst
      ParseDpasOp(0, false); // src0
      ParseDpasOp(1, false); // src1
      ParseDpasOp(2, false); // src2
      ParseDpasOp(3, false); // src3
      ParseDpasOp(4, false); // src4
      break;
    case OpSpec::JUMP_UNARY_IMM:
      ParseSrcOpLabel(0);
      break;
    case OpSpec::JUMP_UNARY_REGIMM:
      // e.g. jmpi or brd  (registers or labels)
      ParseSrcOp(0);
      break;
    case OpSpec::JUMP_UNARY_REG:
      // e.g. ret (..) r12
      ParseSrcOp(0);
      if (m_srcKinds[0] != Operand::Kind::DIRECT &&
          m_srcKinds[0] != Operand::Kind::INDIRECT) {
        FailS(m_srcLocs[0], "src0 must be a register");
      }
      break;
    case OpSpec::JUMP_BINARY_BRC: {
      //   brc (...) lbl lbl
      //   brc (...) reg null
      //
      // for legacy reasons we support a unary form
      //   brc (...) reg
      // we do add an explicit null parameter for src1
      // it's very possible there are some simulator tests floating
      // aroudn that require this old form.
      ParseSrcOp(0);
      if (m_srcKinds[0] == Operand::Kind::IMMEDIATE ||
          m_srcKinds[0] == Operand::Kind::LABEL || LookingAtIdentEq("null")) {
        ParseSrcOp(1);
      } else {
        // legacy format
        // add an explicit null operand
        //
        // trb: this is no longer a warning, but we still set the
        // parameter in the IR for the time being
        //
        // if (m_parseOpts.deprecatedSyntaxWarnings) {
        //    Warning(brcStart,
        //        "deprecated syntax: register version "
        //        "of brc takes an explicit null parameter");
        // }
        // m_handler.InstSrcOpRegDirect(
        //     1,
        //     brcStart,
        //     SrcModifier::NONE,
        //     RegName::ARF_NULL,
        //     REGREF_ZERO_ZERO,
        //     Region::SRC010,
        //     Type::UD);
      }
      break;
    }
    case OpSpec::JUMP_UNARY_CALL_REGIMM:
      // call (..) dst src
      // calla (..) dst src
      ParseDstOp();
      ParseSrcOp(0);
      break;
    case OpSpec::JUMP_BINARY_IMM_IMM:
      // e.g. else, cont, break, goto, halt
      ParseSrcOpLabel(0);
      ParseSrcOpLabel(1);
      break;
    default:
      IGA_ASSERT_FALSE("unhandled syntax class in parser");
    }
  }

  // Original binary send <=XE (sends, sendsc)
  void ParseSendInstructionLegacy() {
    ParseSendDstOp();
    ParseSendSrcOp(0, false);
    ParseSendSrcOp(1, m_model.supportsUnarySend() &&
                          m_opts.supportLegacyDirectives);
    ParseSendDescsLegacy();
  }

  // XE_HP offers new : syntax for separate src1Len for certain messages
  // XE_HPG, XE_HPC require : syntax
  //   We are flexible here and permit src1Len to come out of the immediate
  //   descriptor for certain cases (with a warning).
  void ParseSendInstructionXeHP() {
    ParseSendDstOp();
    ParseSendSrcOp(0, false);
    int src1Len = -1;
    ParseSendSrc1OpWithOptLen(src1Len);
    ParseSendDescsWithOptSrc1Len(src1Len);
  }

  // same as prior sends, but has ExDescImm (c.f. ParseSendDescsXe2)
  void ParseSendInstructionXe2() {
    ParseSendDstOp();
    ParseSendSrcOp(0, false);
    int src1Len = -1;
    ParseSendSrc1OpWithOptLen(src1Len);
    ParseSendDescsXe2(src1Len);
  }

  // backwards compatible send with Gather Send support
  // same as Xe2, but allows gather send (src0 indirect operand)
  void ParseSendInstructionXe3() {
    ParseSendDstOpXe3(); // use dst type INVALID on >=XE3
    bool isGathering = ParseSendSrc0OpXe3();
    int src1Len = -1;
    if (LookingAtSeq(COLON, IDENT)) {
      // send ... r0:ud ...
      //            ^^^ forbidden on XE3+
      // send ... r[s0.0]:ud ...
      //                 ^^^ forbidden on XE3
      FailS(NextLoc(), "types are forbidden on send operands on this platform");
    }
    if (isGathering) {
      if (LookingAtSeq(COLON, INTLIT10)) {
        // send ... r[s0.0]:4 ...
        //                 ^^ oops needs to be descriptor for this platform
        FailS(NextLoc(), "Src0.Len should come from the descriptor");
      }
      m_srcLocs[1] = NextLoc();
      // create null src1 for Gather Send (src1 won't be printed in asm)
      addNullGatherSendSrc1();
      // set src1Len to 0 to indicate ExBSO mode on
      src1Len = 0;
    } else {
      ParseSendSrc1OpWithOptLen(src1Len);
    }
    ParseSendDescsXe2(src1Len);
  }

  void ParseSendgInstructionXe3p() {
    ParseSendDstOpXe3();

    bool gatheringSrc0 = ParseSendgSrc0OpXe3p();

    if (gatheringSrc0) {
      // create null src1 for Gather Send (src1 won't be printed in asm)
      addNullGatherSendSrc1();
      m_builder.InstSendSrc1Length(0);
    } else {
      ParseSendgSrc1OpXe3();
    }
    ParseSendgDescsXe3();
  }


  // Predication = ('(' WrEnPred ')')?
  //   where WrEnPred = WrEn
  //                  | Pred
  //                  | WrEn '&' Pred
  //         WrEn = 'W'
  //         Pred = '~'? FlagReg ('.' PreCtrl?)
  void ParseWrEnPred() {
    m_hasWrEn = false;
    if (Consume(LPAREN)) {
      const Loc nmLoc = NextLoc(0);
      if (ConsumeIdentEq("W")) {
        m_hasWrEn = true;
        m_builder.InstNoMask(nmLoc);
        if (Consume(AMP)) {
          ParsePred();
        }
      } else {
        ParsePred();
      }
      ConsumeOrFail(RPAREN, "expected )");
    }
  }

  // Pred = '~'? FlagReg ('.' PreCtrl?)
  void ParsePred() {
    static const IdentMap<PredCtrl> PREDCTRLS{
        {"xyzw", PredCtrl::SEQ}, // lack of a specific pred control defaults to
                                 // SEQ
        {"anyv", PredCtrl::ANYV},     {"allv", PredCtrl::ALLV},
        {"any2h", PredCtrl::ANY2H},   {"all2h", PredCtrl::ALL2H},
        {"any4h", PredCtrl::ANY4H},   {"all4h", PredCtrl::ALL4H},
        {"any8h", PredCtrl::ANY8H},   {"all8h", PredCtrl::ALL8H},
        {"any16h", PredCtrl::ANY16H}, {"all16h", PredCtrl::ALL16H},
        {"any32h", PredCtrl::ANY32H}, {"all32h", PredCtrl::ALL32H},
        {"any", PredCtrl::ANY},       {"all", PredCtrl::ALL},
    };

    const Loc prLoc = NextLoc(0);
    bool predInv = Consume(TILDE);
    ParseFlagRegRef(m_flagReg);
    PredCtrl predCtrl = PredCtrl::NONE;
    if (Consume(DOT)) {
      ConsumeIdentOneOfOrFail(PREDCTRLS, predCtrl,
                              "expected predication control",
                              "invalid predication control");
    } else {
      predCtrl = PredCtrl::SEQ;
    }
    m_builder.InstPredication(prLoc, predInv, m_flagReg, predCtrl);
  }

  const OpSpec *TryConsumeMmenonic() {
    const Token &tk = Next();
    if (tk.lexeme != IDENT) {
      return nullptr;
    }
    const char *p = &m_lexer.GetSource()[tk.loc.offset];
    std::string s;
    s.reserve((size_t)tk.loc.extent + 1);
    for (size_t i = 0; i < tk.loc.extent; i++) {
      s += *p++;
    }
    auto itr = opmap.find(s);
    if (itr == opmap.end()) {
      return nullptr;
    } else {
      Skip();
      return itr->second;
    }
  }

#if 0
    // returns the number of characters off
    size_t similarityR(
        size_t mismatches, size_t shortestStrLen,
        const std::string &left, size_t li,
        const std::string &right, size_t ri)
    {
        if (mismatches >= shortestStrLen) { // everything mismatches
            return shortestStrLen;
        } else if (li == left.size() - 1) { // the rest of right mismatches
            return mismatches + right.size() - ri;
        } else if (ri == right.size() - 1) { // the rest of left mismatches
            return mismatches + left.size() - li;
        } else {
            if (left[li] != right[ri]) {
                mismatches++;
            }
            // 1. delete both
            auto db = similarityR(
                mismatches, shortestStrLen,
                left, li + 1,
                right, ri + 1);
            // 2. delete left
            auto dr = similarityR(
                mismatches, shortestStrLen,
                left, li,
                right, ri + 1);
            // 3. delete right
            auto dl = similarityR(
                mismatches, shortestStrLen,
                left, li + 1,
                right, ri);
            return std::min<size_t>(db, std::min<size_t>(dr, dl));
        }
    }
    // roughly the number of characters
    float similarity(const std::string &left, const std::string &right) {
        if (left.size() > 8 || right.size() > 8)
            return 0;
        auto shortestLen = std::min<size_t>(left.size(), right.size());
        size_t minEdits = similarityR(0, shortestLen, left, 0, right, 0);
        return 1.0f - (float)minEdits/(float)shortestLen;
    }
#endif
  void failWithUnexpectedSubfunction(const Loc &loc, const std::string &) {
    FailAtT(loc, "unexpected subfunction for op");
#if 0
        std::vector<std::pair<float,const char *>> matches;
        for (int i = (int)m_opSpec->op + 1;
            i < (int)m_opSpec->op + m_opSpec->subopsLength;
            i++)
        {
            const OpSpec &sf = m_model.lookupOpSpec((iga::Op)i);
            if (sf.isValid()) {
                auto sim = similarity(sfIdent, sf.mnemonic.str());
                std::cout << sf.mnemonic << ": " << sim << "\n";
                if (sim >= 0.66f) {
                    matches.emplace_back(sim, sf.mnemonic.str());
                }
            }
        }
        std::sort(matches.begin(), matches.end(),
            std::greater<std::pair<float,const char *>>());
        if (!matches.empty()) {
            ss << "; did you mean ";
            if (matches.size() > 2) {
                ss << "one of: ";
            }
            // only show the top five
            size_t len = std::min<size_t>(5, matches.size());
            for (size_t i = 0; i < len; i++) {
                if (i > 0 && i == len - 1) {
                    if (len > 2) {
                        ss << ", or ";
                    } else {
                        ss << " or ";
                    }
                } else {
                    ss << ", ";
                }
                ss << matches[i].second;
            }
        }
#endif
  }

  // Parse BFN expressions.
  //
  //   BFNExpr = BFNBinExpr
  //     where BFNBinExpr = (BFNUnExpr ('&'|'^'|'|') BFNUnExpr)*
  //                          regular boolean precedence: & > ^ > |
  //           BFNUnExpr = '~'? BFNPrimExpr
  //           BFNPrimExpr = '(' BFNExpr ')' | 's0' | 's1' | 's2'
  //
  // E.g.
  // bfn.(s0^(s1|~s2)) ...
  //     ^
  uint8_t ParseBfnMnemonicSubfuncExpr() {
    Skip(1); // skip LPAREN
    uint32_t val = ParseBfnMnemonicSubfuncExprOR();
    ConsumeOrFail(RPAREN, "expected )");
    return (uint8_t)val;
  }
  uint32_t ParseBfnMnemonicSubfuncExprOR() {
    uint32_t val = ParseBfnMnemonicSubfuncExprXOR();
    while (Consume(PIPE)) {
      val |= ParseBfnMnemonicSubfuncExprXOR();
    }
    return val;
  }
  uint32_t ParseBfnMnemonicSubfuncExprXOR() {
    uint32_t val = ParseBfnMnemonicSubfuncExprAND();
    while (Consume(CIRC)) {
      val ^= ParseBfnMnemonicSubfuncExprAND();
    }
    return val;
  }
  uint32_t ParseBfnMnemonicSubfuncExprAND() {
    uint32_t val = ParseBfnMnemonicSubfuncExprNEG();
    while (Consume(AMP)) {
      val &= ParseBfnMnemonicSubfuncExprNEG();
    }
    return val;
  }
  uint32_t ParseBfnMnemonicSubfuncExprNEG() {
    bool c = Consume(TILDE);
    uint32_t val = ParseBfnMnemonicSubfuncExprATOM();
    if (c) {
      val = (~val & 0xFF);
    }
    return val;
  }
  uint32_t ParseBfnMnemonicSubfuncExprATOM() {
    if (Consume(LPAREN)) {
      uint32_t val = ParseBfnMnemonicSubfuncExprOR();
      ConsumeOrFail(RPAREN, "expected )");
      return val;
    } else if (LookingAt(IDENT) || LookingAt(INTLIT10)) {
      auto symTkn = Next();
      auto sfIdent = GetTokenAsString(symTkn);
      Skip();
      if (sfIdent == "s0") {
        return 0xAA;
      } else if (sfIdent == "s1") {
        return 0xCC;
      } else if (sfIdent == "s2") {
        return 0xF0;
      } else if (sfIdent == "zeros" || sfIdent == "0") {
        return 0x00;
      } else if (sfIdent == "ones" || sfIdent == "1") {
        return 0xFF;
      } else {
        FailAtT(symTkn.loc, "syntax error in symbolic expression");
      }
    } else {
      FailT("syntax error in symbolic expression");
    }
    return 0;
  }

  //
  // Mnemoninc
  //     = Ident SubMnemonic?
  //     | Ident BrCtl
  //     | Ident '(' BFNExpr ')'
  //   SubMnemoninc
  //     = '.' SfIdent
  //     | '.' HEX_INT | '.' DEC_INT
  //
  //   SfIdent =
  //     | BFNExpr = see above
  //     | BrnchCtl
  //     | SFID
  //     | SyncFc
  //     | ... other subfunction identifier
  const OpSpec *ParseMnemonic() {
    const Loc mnemonicLoc = NextLoc();
    const OpSpec *pOs = TryConsumeMmenonic();
    if (!pOs) {
      return nullptr;
    }
    m_builder.InstOp(pOs);
    // GED will reject this otherwise
    if (!m_hasWrEn && pOs->op == Op::JMPI) {
      WarningAtT(mnemonicLoc,
                 "jmpi must have (W) specified (automatically adding)");
      m_builder.InstNoMask(mnemonicLoc);
    }

    // TODO: abstract this all into a something that's contained in
    // the IGA bxml enums files.
    //   e.g. use OpSpec::supportsSubfunction()
    //        bool Subfunction::FromString(Op, Subfunction&)
    if (pOs->supportsBranchCtrl()) {
      BranchCntrl brctl = BranchCntrl::OFF;
      if (Consume(DOT)) {
        if (!ConsumeIdentEq("b")) {
          FailT("expected 'b' (branch control)");
        }
        brctl = BranchCntrl::ON;
      }
      m_builder.InstSubfunction(brctl);
    } else if (pOs->is(Op::MATH)) {
      // e.g. math.*, send.*, etc...
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnum<MathFC>());
    } else if (pOs->is(Op::SYNC)) {
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnum<SyncFC>());
    } else if (platform() >= Platform::XE && pOs->isAnySendFormat()) {
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnum<SFID>());
      // else: it's part of ExDesc
    } else if (pOs->is(Op::BFN)) {
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnumBfnFC());
    } else if (pOs->isDpasFormat()) {
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnum<DpasFC>());
    } else if (pOs->is(Op::SHFL)) {
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnum<ShuffleFC>());
    } else if (pOs->is(Op::LFSR)) {
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnum<LfsrFC>());
    } else if (pOs->is(Op::DNSCL)){
      m_builder.InstSubfunction(ParseSubfunctionFromBxmlEnumDnsclFC());
    } else {
      // isOldSend: pre XE will have a subfunction,
      // but we pull it from ExDesc
      bool isOldSend = platform() < Platform::XE && pOs->isAnySendFormat();
      IGA_ASSERT(!pOs->supportsSubfunction() || isOldSend,
                 "INTERNAL ERROR: subfunction expected");
    }

    if (LookingAt(DOT)) {
      // maybe an old condition modifier or saturation
      FlagModifier fm;
      if (LookingAtIdentEq(1, "sat")) {
        FailT("saturation flag prefixes destination operand: "
              "e.g. op (..) (sat)dst ...");
      } else if (IdentLookupFrom(1, FLAGMODS, fm) ||
                 IdentLookupFrom(1, FLAGMODS_LEGACY, fm)) {
        FailT("conditional modifier follows execution mask "
              "info: e.g. op (16|M0)  (le)f0.0 ...");
      } else {
        // didn't match flag modifier
        FailT("unexpected . (expected execution size)");
      }
    }

    return pOs;
  }

  template <typename T> T ParseSubfunctionFromBxmlEnum() {
    if (!Consume(DOT)) {
      FailAfterPrev("expected operation subfunction");
    }

    auto sfLoc = NextLoc();
    if (LookingAt(IDENT)) {
      auto loc = NextLoc();
      const std::string sfIdent = GetTokenAsString();
      Skip();
      auto x = FromSyntax<T>(sfIdent);
      if (x == T::INVALID) {
        FailAtT(loc, "invalid subfunction");
      }
      return x;
    } else if (LookingAtAnyOf(INTLIT10, INTLIT16)) {
      uint32_t val;
      (void)ConsumeIntLit(val);
      return (T)val;
    }
    FailAtT(sfLoc, "invalid subfunction");
    return (T)-1;
  }

  // we treat BFN a little different since we permit a BFN expression
  // e.g.  bfn.(s0&~s1^s2)
  //
  // template <> BfnFC ParseSubfunctionFromBxmlEnum()
  // gcc: doesn't permit this specialization
  BfnFC ParseSubfunctionFromBxmlEnumBfnFC() {
    if (!Consume(DOT)) {
      FailAfterPrev("expected subfunction");
    }
    uint8_t fc = 0;
    if (LookingAt(LPAREN)) {
      fc = ParseBfnMnemonicSubfuncExpr();
    } else if (LookingAtAnyOf(INTLIT10, INTLIT16)) {
      auto loc = NextLoc();
      uint32_t val = 0;
      (void)ConsumeIntLit(val);
      if (val > 0xFF) {
        FailAtT(loc, "subfunction value is out of bounds");
      }
      fc = (uint8_t)val;
    } else {
      FailT("invalid subexpression");
    }
    return BfnFC(fc);
  }

  DnsclFC ParseSubfunctionFromBxmlEnumDnsclFC() {
    if (!Consume(DOT)) {
      FailAfterPrev("expected operation subfunction");
    }
    // DnsclFC is composed by 4 parts. dnscl op is in the syntax:
    // dnscl.<ConvSrcDataType>to<ConvDstDataType>.<DnsclMode>.<RoundingMode>
    // for example: dnscl.bftoE2M1.mode0.srnd
    ConvSrcDataType srcTy = ConvSrcDataType::INVALID;
    ConvDstDataType dstTy = ConvDstDataType::INVALID;
    DnsclMode dMode = DnsclMode::INVALID;
    RoundingMode rMode = RoundingMode::INVALID;

    // parse <ConvSrcDataType>to<ConvDstDataType>
    auto loc = NextLoc();
    if (LookingAt(IDENT)) {
      auto srcDstTyLoc = NextLoc();
      std::string str = GetTokenAsString();
      Skip();
      auto toPos = str.find("to");
      if (toPos == std::string::npos)
        FailAtT(srcDstTyLoc, "invalid subfunction");
      std::string srcStr = str.substr(0, toPos);
      srcTy = FromSyntax<ConvSrcDataType>(srcStr);
      std::string dstStr = str.substr(toPos + 2);
      dstTy = FromSyntax<ConvDstDataType>(dstStr);
      if (srcTy == ConvSrcDataType::INVALID ||
          dstTy == ConvDstDataType::INVALID) {
        FailAtT(srcDstTyLoc, "invalid conv type");
      }
    } else {
      FailAtT(loc, "expected operation subfunction");
    }

    // parse <DnsclMode>
    if (!Consume(DOT)) {
      // FIXME: vague error message to avoid IP issue
      FailAfterPrev("expected mode");
    }
    loc = NextLoc();
    if (LookingAt(IDENT)) {
      auto dmLoc = NextLoc();
      std::string str = GetTokenAsString();
      Skip();
      dMode = FromSyntax<DnsclMode>(str);
      if (dMode == DnsclMode::INVALID)
        FailAtT(dmLoc, "invalid mode");
    } else {
      // FIXME: vague error message to avoid IP issue
      FailAtT(loc, "expected mode");
    }

    // parse <RoundingMode>
    if (!Consume(DOT)) {
      FailAfterPrev("expected rounding mode");
    }
    loc = NextLoc();
    if (LookingAt(IDENT)) {
      auto rmLoc = NextLoc();
      std::string str = GetTokenAsString();
      Skip();
      rMode = FromSyntax<RoundingMode>(str);
      if (rMode == RoundingMode::INVALID)
        FailAtT(rmLoc, "invalid mode");
    } else {
      FailAtT(loc, "expected rounding mode");
    }

    return DnsclFC(srcTy, dstTy, dMode, rMode);
  }

  // FlagModifierOpt = '(' FlagModif ')' FlagReg
  void ParseFlagModOpt() {
    Loc loc = NextLoc();
    if (Consume(LBRACK)) {
      // try the full form [(le)f0.0]
      FlagModifier flagMod;
      if (!TryParseFlagModFlag(flagMod)) {
        FailT("expected flag modifier function");
      }
      ParseFlagModFlagReg();
      ConsumeOrFail(RBRACK, "expected ]");
      if (m_opts.deprecatedSyntaxWarnings) {
        WarningAtT(loc, "deprecated flag modifier syntax (omit the brackets)");
      }
      m_builder.InstFlagModifier(m_flagReg, flagMod);
    } else {
      // try the short form
      // (le)f0.0
      FlagModifier flagMod;
      if (!TryParseFlagModFlag(flagMod)) {
        // soft fail (we might be looking at "(sat)r0.0")
        return;
      }
      ParseFlagModFlagReg();
      m_builder.InstFlagModifier(m_flagReg, flagMod);
    }
  }
  // e.g. "(le)"
  bool TryParseFlagModFlag(FlagModifier &flagMod) {
    if (!LookingAt(LPAREN)) {
      return false;
    }
    if (!IdentLookupFrom(1, FLAGMODS, flagMod)) {
      Loc loc = NextLoc(1);
      if (!IdentLookupFrom(1, FLAGMODS_LEGACY, flagMod)) {
        return false;
      } else if (m_opts.deprecatedSyntaxWarnings) {
        // deprecated syntax
        WarningAtT(loc,
                   "deprecated flag modifier syntax: "
                   "use ",
                   ToSyntax(flagMod), " for this function");
      }
    }
    Skip(2);
    ConsumeOrFail(RPAREN, "expected )");
    return true;
  }
  // e.g. "f0.1"
  void ParseFlagModFlagReg() {
    const Loc flregLoc = NextLoc();
    RegRef fr;
    ParseFlagRegRef(fr);
    if (m_flagReg.regNum != REGREF_INVALID.regNum &&
        (m_flagReg.regNum != fr.regNum ||
         m_flagReg.subRegNum != fr.subRegNum)) {
      FailAtT(flregLoc, "flag register must be same for predication "
                        "and flag modifier");
    }
    m_flagReg = fr;
  }

  // Examples:
  //   r13.4<2>:hf
  //   (sat)r13.0<1>:f
  //   r[a0.3,16]<1>:ud
  void ParseDstOp() {
    // We want to track the beginning of the "operand", not just the register.
    // That includes the saturate flag prefix(sat) on the dst.
    // That is we need:
    // (sat)r13.3:ud
    // ^    ^
    // | | not here
    // |
    // | here
    const Loc opStart = NextLoc(0);
    // ParseSatOpt increments m_offset.
    if (ParseSatOpt()) {
      m_builder.InstDstOpSaturate();
    }
    // Note that, if there is SAT token, opStart != regStart.
    // This is because m_offset changed.
    const Loc regStart = NextLoc(0);
    if (ConsumeIdentEq("r")) {
      ParseDstOpRegInd(opStart, 0);
    } else {
      const RegInfo *regInfo;
      int regNum;
      if (!ConsumeReg(regInfo, regNum)) {
        FailT("invalid destination register");
      }
      if (regInfo && !regInfo->isRegNumberValid(regNum)) {
        FailT("invalid destination register number "
              "(",
              regInfo->syntax, " only has ", regInfo->numRegs,
              " registers on this platform)");
      }
      if (LookingAt(LBRACK)) {
        ParseDstOpRegInd(opStart, regNum * 32);
      } else {
        assert(regInfo != nullptr);
        FinishDstOpRegDirSubRegRgnTy(opStart, regStart, *regInfo, regNum);
      }
    }
  }

  // r13
  // null
  // r13:w (non-standard)
  void ParseSendDstOp() {
    const Loc regStart = NextLoc(0);
    if (ConsumeIdentEq("r")) {
      ParseDstOpRegInd(regStart, 0);
    } else {
      const RegInfo *ri = NULL;
      int regNum = 0;
      if (!ConsumeReg(ri, regNum)) {
        FailT("invalid send destination register");
      }
      if (!ri->isRegNumberValid(regNum)) {
        FailT("invalid destination register number "
              "(",
              ri->syntax, " only has ", ri->numRegs,
              " registers on this platform)");
      }
      if (LookingAt(LBRACK)) {
        FailT("this form of indirect (r3[a0.0,16]) is invalid for "
              "send dst operand; use regular form: r[a0.0,16]");
      } else {
        FinishDstOpRegDirSubRegRgnTy(regStart, regStart, *ri, regNum);
      }
    }
  }

  void ParseSendDstOpXe3() {
    OperandInfo opInfo;
    m_srcLocs[1] = NextLoc();
    m_srcKinds[1] = opInfo.kind = Operand::Kind::DIRECT;
    auto reg = ParseReg();
      // for sendgx, check if the destination is ARF_NULL
      // if ARF_NULL, then change to r511
    if ((m_opSpec->is(Op::SENDGX) || m_opSpec->is(Op::SENDGXC)) &&
        reg.first->regName == RegName::ARF_NULL) {
      opInfo.regOpName = RegName::ARF_NULL;
      opInfo.regOpReg.regNum = (uint16_t)0;
      opInfo.regOpReg.subRegNum = 0;
    }
    else {
      opInfo.regOpName = reg.first->regName;
      opInfo.regOpReg.regNum = reg.second;
      opInfo.regOpReg.subRegNum = 0;
    }
    m_builder.InstDstOp(opInfo);
    if (LookingAtSeq(COLON, IDENT)) {
      // send r10:ud ...
      //         ^^^ forbidden on XE3+
      FailS(NextLoc(), "types are forbidden on send operands on this platform");
    }
  }

  // (sat)
  bool ParseSatOpt() {
    // TODO: expand to tokens so (sat) can be imm expr
    return Consume(SAT);
  }

  // .mme2 or .nomme
  MathMacroExt ParseMathMacroReg() {
    auto loc = NextLoc();
    MathMacroExt mme = MathMacroExt::INVALID;
    const char *EXPECTED = "expected math macro register "
                           "(e.g. .mme0, ..., .mme7, or .nomme)";
    ConsumeOrFail(DOT, EXPECTED);
    if (ConsumeIdentOneOf<MathMacroExt>(MATHMACROREGS, mme)) {
      return mme;
    }
    // determine if we support the old-style
    bool supportOldStyleAcc2ToAcc7 = true;
    supportOldStyleAcc2ToAcc7 = platform() < Platform::XE;

    if (supportOldStyleAcc2ToAcc7) {
      if (ConsumeIdentOneOf<MathMacroExt>(MATHMACROREGS_OLDSTYLE, mme)) {
        // warn?
        WarningS(loc, "old-style math macro register (use mme)");
        return mme;
      }
    }

    // favor the new error message
    FailS(loc, EXPECTED);
    return mme;
  }

  // REG ('.' INT)? DstRgn (':' DstTy)?
  //   where DstRgn = '<' INT '>'
  //
  // E.g. r13.4<2>:t
  void FinishDstOpRegDirSubRegRgnTy(const Loc &opStart, const Loc &regnameLoc,
                                    const RegInfo &ri, int regNum) {
    // subregister or implicit accumulator operand
    // e.g. .4 or .acc3
    Loc subregLoc = NextLoc(0);
    int subregNum = 0;

    // <1>
    Region::Horz rgnHz = Region::Horz::HZ_1;

    MathMacroExt mmeReg = MathMacroExt::INVALID;
    if (m_opSpec->isAnySendFormat() && Consume(DOT)) {
      ConsumeIntLitOrFail(subregNum, "expected subregister");
      // whine about them using a subregister on a send operand
      if (m_opts.deprecatedSyntaxWarnings) {
        WarningAtT(subregLoc, "send operand subregisters have no effect"
                              " and are deprecated syntax");
      }
      if (m_opSpec->isAnySendFormat() && LookingAt(LANGLE)) {
        if (m_opts.deprecatedSyntaxWarnings) {
          // whine about them using a region
          WarningT("send operand region has no effect and is"
                   " deprecated syntax");
        }
      }
      rgnHz = ParseDstOpRegion();
    } else if (m_builder.isMacroOp()) {
      // implicit accumulator operand
      mmeReg = ParseMathMacroReg();
    } else {
      // non-send with subregister.
      // regular subregister
      if (Consume(DOT)) {
        ConsumeIntLitOrFail(subregNum, "expected subregister");
      } else {
        // implicit subregister
        //  E.g. r12:d  ...
        subregNum = 0;
      }
    }

    // <1>
    rgnHz = ParseDstOpRegion();

    // :t
    Type dty = Type::INVALID;
    if (m_opSpec->isAnySendFormat()) {
      // special handling for send types
      dty = ParseSendOperandTypeWithDefault(-1);
    } else {
      dty = ParseDstOpTypeWithDefault();
    }

    // ensure the subregister is not out of bounds
    if (dty != Type::INVALID) {
      int typeSize = TypeSizeInBits(dty) / 8;
      if (!ri.isSubRegByteOffsetValid(regNum, subregNum * typeSize,
                                      m_model.getGRFByteSize())) {
        if (ri.regName == RegName::GRF_R) {
          ErrorAtT(subregLoc, "subregister out of bounds for data type ",
                   ToSyntax(dty));
        } else {
          // Print warning for non-GRF register, in case for those
          // valid but strange case e.g. null0.20:f
          WarningAtT(subregLoc, "subregister out of bounds for data type");
        }
      } else if (typeSize < ri.accGran) {
        WarningAtT(regnameLoc, "access granularity too small for data type");
      }
    }

    if (m_builder.isMacroOp()) {
      m_builder.InstDstOpRegMathMacroExtReg(opStart, ri.regName, regNum, mmeReg,
                                            rgnHz, dty);
    } else {
      RegRef reg(regNum, subregNum);
      m_builder.InstDstOpRegDirect(opStart, ri.regName, reg, rgnHz, dty);
    }
  }

  // e.g. [a0.4,  16]
  //  or  [a0.4 + 16]
  //  or  [a0.4 - 16]
  void ParseIndOpArgs(RegRef &addrRegRef, int &addrOff, RegName &regName) {
    ConsumeOrFail(LBRACK, "expected [");
    if (!ParseAddrRegRefOpt(addrRegRef, regName)) {
      FailT("expected address subregister");
    }
    Loc addrOffLoc = NextLoc();
    if (Consume(COMMA)) {
      bool neg = Consume(SUB);
      ConsumeIntLitOrFail(addrOff, "expected indirect address offset");
      if (neg)
        addrOff *= -1;
    } else if (Consume(ADD)) {
      ConsumeIntLitOrFail(addrOff, "expected indirect address offset");
    } else if (Consume(SUB)) {
      ConsumeIntLitOrFail(addrOff, "expected indirect address offset");
      addrOff *= -1;
    } else {
      addrOff = 0;
    }

    // check if the imm offset out of bound
    int addroffMax = 511;
    int addroffMin = -512;
    if (platform() >= Platform::XE_HPC) {
      addroffMax = 1023;
      addroffMin = -1024;
    }
    if (addrOff < addroffMin || addrOff > addroffMax) {
      FailAtT(addrOffLoc,
              "immediate offset is out of range; must be in "
              "[",
              addroffMin, ",", std::to_string(addroffMax), "]");
    }

    ConsumeOrFail(RBRACK, "expected ]");
  }

  // '[' 'a0' '.' INT (',' INT)? ']' ('<' INT '>')?)? TY?
  // e.g [a0.3,16]<1>:t
  void ParseDstOpRegInd(const Loc &opStart, int baseAddr) {
    // [a0.4,16]
    int addrOff;
    RegRef addrRegRef;
    RegName regName = RegName::INVALID;
    ParseIndOpArgs(addrRegRef, addrOff, regName);
    addrOff += baseAddr;
    //
    // <1>
    Region::Horz rgnHz = ParseDstOpRegion();
    //
    // :t
    Type dty = ParseDstOpTypeWithDefault();
    m_builder.InstDstOpRegIndirect(opStart, addrRegRef, addrOff, rgnHz, dty);
  }

  // '<' INT '>'
  Region::Horz ParseDstOpRegion() {
    if (!LookingAt(LANGLE)) {
      if (m_opSpec->hasImplicitDstRegion(m_builder.isMacroOp())) {
        return m_opSpec->implicitDstRegion(m_builder.isMacroOp()).getHz();
      } else {
        return Region::Horz::HZ_1;
      }
    }

    Region::Horz rgnHz = Region::Horz::HZ_1;
    if (Consume(LANGLE)) {
      const Loc loc = NextLoc();
      int rgnHzInt;
      ConsumeIntLitOrFail(rgnHzInt, "destination region argument");
      switch (rgnHzInt) {
      case 1:
        rgnHz = Region::Horz::HZ_1;
        break;
      case 2:
        rgnHz = Region::Horz::HZ_2;
        break;
      case 4:
        rgnHz = Region::Horz::HZ_4;
        break;
      default:
        FailS(loc, "invalid destination region");
      }
      ConsumeOrFail(RANGLE, "expected >");
    }

    return rgnHz;
  }

  // E.g. 3 in "a0.3"
  bool ParseAddrRegRefOpt(RegRef &addrReg, RegName &regName) {
    const RegInfo *ri;
    int regNum;
    regName = RegName::INVALID;
    if (!ConsumeReg(ri, regNum)) {
      return false;
    }
    regName = ri->regName;
    if (regNum != 0 || (ri->regName != RegName::ARF_A
                        && ri->regName != RegName::ARF_S // r[s0.0]
                        )) {
      FailT("expected address register for indirect access (a0)");
    }
    if (!Consume(DOT)) {
      FailT("expected .");
    }
    addrReg.regNum = addrReg.subRegNum = 0;
    ConsumeIntLitOrFail(addrReg.subRegNum,
                        "expected address register subregister");
    // TODO: range-check against RegInfo for "a"
    return true;
  }

  // E.g. 3 in "s0.2"
  bool ParseScalarRegRefOpt(RegRef &addrReg) {
    const RegInfo *ri;
    int regNum;
    if (!ConsumeReg(ri, regNum)) {
      return false;
    }
    if (regNum != 0 || ri->regName != RegName::ARF_S) {
      FailT("expected scalar register for indirect access (s0.#)");
    }
    if (!Consume(DOT)) {
      FailT("expected .");
    }
    addrReg.regNum = addrReg.subRegNum = 0;
    ConsumeIntLitOrFail(addrReg.subRegNum,
                        "expected scalar register subregister");
    return true;
  }

  // same as ParseSrcOp, but semantically chekcs that it's a label
  void ParseSrcOpLabel(int srcOpIx) {
    ParseSrcOp(srcOpIx);
    if (m_srcKinds[srcOpIx] != Operand::Kind::LABEL &&
        m_srcKinds[srcOpIx] != Operand::Kind::IMMEDIATE) {
      FailAtT(m_srcLocs[srcOpIx], "src", srcOpIx,
              " must be an immediate label");
    }
  }

  void ParseSrcOp(int srcOpIx) {
    m_srcLocs[srcOpIx] = NextLoc();

    // TODO: sink this down to the immediate literal case specially
    // For now, we leave it here since TryParseConstExpr() will otherwise
    // fail on "-r12:f" would fail: - as unary negation then r12 fails to
    // resolve
    const Token regnameTk = Next();
    const RegInfo *regInfo;
    int regNum;

    // first try and parse as register
    m_lexer.Mark();
    bool pipeAbs = false;
    SrcModifier srcMods = ParseSrcModifierOpt(pipeAbs);
    if (ConsumeIdentEq("r")) {
      // canonical register indirect
      // r[a0.4,-32]
      m_srcKinds[srcOpIx] = Operand::Kind::INDIRECT;
      ParseSrcOpInd(srcOpIx, m_srcLocs[srcOpIx], srcMods, 0);
      // register, symbolic immediate, label, ...
      if (pipeAbs) {
        ConsumeOrFailAfterPrev(PIPE, "expected |");
      }
      if (!m_opSpec->supportsSourceModifiers() &&
          srcMods != SrcModifier::NONE) {
        FailS(m_srcLocs[srcOpIx], "source modifier not supported");
      }
    } else if (ConsumeReg(regInfo, regNum)) {
      // normal register access
      //   r13.3<0;1,0>
      //   acc3
      m_srcKinds[srcOpIx] = Operand::Kind::DIRECT;
      if (!m_opSpec->supportsSourceModifiers() &&
          srcMods != SrcModifier::NONE) {
        FailS(m_srcLocs[srcOpIx], "source modifier not supported");
      }
      FinishSrcOpRegDirSubRegRgnTy(srcOpIx, m_srcLocs[srcOpIx], regnameTk.loc,
                                   srcMods, *regInfo, regNum);
      if (pipeAbs) {
        ConsumeOrFailAfterPrev(PIPE, "expected |");
      }
    } else {
      // backtrack to before any "source modifier"
      m_lexer.Reset();
      if (pipeAbs) {
        Skip(1);
      }

      // try as raw label first
      if ((m_opSpec->isBranching() || m_opSpec->op == Op::MOV) &&
          LookingAt(IDENT) && !LookingAtIdentEq("snan") &&
          !LookingAtIdentEq("qnan") && !LookingAtIdentEq("nan") &&
          !LookingAtIdentEq("inf")) {
        if (pipeAbs) {
          FailS(regnameTk.loc, "unexpected |");
        }
        // e.g. LABEL64 (mustn't be an expression)
        m_srcKinds[srcOpIx] = Operand::Kind::LABEL;
        std::string str = GetTokenAsString();
        Skip(1);
        FinishSrcOpImmLabel(srcOpIx, m_srcLocs[srcOpIx], regnameTk.loc, str);
      } else {
        ImmVal immVal;
        if (TryParseConstExpr(immVal)) {
          // (does not match labels)
          m_srcKinds[srcOpIx] = Operand::Kind::IMMEDIATE;
          if (pipeAbs) {
            immVal.Abs();
          }
          FinishSrcOpImmValue(srcOpIx, m_srcLocs[srcOpIx], regnameTk, immVal);
          if (pipeAbs) {
            ConsumeOrFailAfterPrev(PIPE, "expected |");
          }
        }
      }
    }
  } // ParseSrcOp

  // .allrd/.allwr SBID set first
  //   "($11,$2,$7)" means ((1 << 11) | (1 << 2) | (1 << 7))
  // we do also permit
  //   "()" as the empty set
  void ParseSyncSrc0Op() {
    auto sf = m_builder.getSubfunction().sync;
    bool isSyncSetOp = sf == SyncFC::ALLRD || sf == SyncFC::ALLWR;
    bool lookingAtPfx =
        LookingAtSeq(LPAREN, DOLLAR) || LookingAtSeq(LPAREN, RPAREN);
    if (!isSyncSetOp || !lookingAtPfx) {
      // e.g. other sync op, or immediate literal, or null
      ParseSrcOp(0);
      return;
    }
    m_srcLocs[0] = NextLoc();
    Skip(); // LPAREN
    uint32_t sbidSet = 0x0;
    auto parseSbid = [&]() {
      uint32_t sbid = 0;
      ConsumeOrFail(DOLLAR, "expected SBID");
      auto nxt = NextLoc();
      ConsumeIntLitOrFail(sbid, "expected SBID");
      if (sbid >= 32)
        FailAtT(nxt, "SBID out of bounds");
      sbidSet |= 1 << sbid;
      return true;
    };
    while (LookingAt(DOLLAR)) {
      parseSbid();
      if (!Consume(COMMA)) {
        break;
      }
    }
    ConsumeOrFail(RPAREN, "expected )");
    ImmVal val;
    val = sbidSet;
    auto srcTy = ParseSrcOpTypeWithoutDefault(0, true);
    m_builder.InstSrcOpImmValue(0, m_srcLocs[0], val, srcTy);
  }

  void ParseSrcOpInd(int srcOpIx, const Loc &opStart,
                     const SrcModifier &srcMods, int baseOff) {
    m_srcKinds[srcOpIx] = Operand::Kind::INDIRECT;

    // [a0.4 + 16]
    int addrOff;
    RegRef addrRegRef;
    RegName regName = RegName::INVALID;
    ParseIndOpArgs(addrRegRef, addrOff, regName);
    addrOff += baseOff;

    // regioning... <V;W,H> or <V,H>
    Region rgn = ParseSrcOpRegionInd(srcOpIx, opStart);

    // :t
    Type sty = ParseSrcOpTypeWithDefault(srcOpIx, true);

    IGA_ASSERT(regName != RegName::INVALID,
               "SrcOpInd must not have INVALID register name");
    m_builder.InstSrcOpRegIndirect(srcOpIx, opStart, srcMods, regName,
                                   addrRegRef, addrOff, rgn, sty);
  }

  // REG ('.' INT)? SrcRgn? (':' SrcTy)?
  //    ^ HERE
  //
  // E.g. r13.4<2>:f
  // E.g. r13.4<0;1,0>:f
  // E.g. r13.4<8;8,1>:f
  // E.g. r13.acc2:f
  // E.g. ce:ud
  // E.g. ip:ud
  // E.g. a0.1
  void FinishSrcOpRegDirSubRegRgnTy(int srcOpIx, const Loc &opStart,
                                    const Loc &regnameLoc,
                                    const SrcModifier &srcMod,
                                    const RegInfo &ri, int regNum) {
    // subregister or implicit accumulator operand
    // e.g. .4 or .acc3
    int subregNum;
    Region rgn;
    Loc subregLoc = NextLoc(1);
    MathMacroExt mme = MathMacroExt::INVALID;
    bool hasExplicitSubreg = false;
    if (m_builder.isMacroOp()) {
      // implicit accumulator operand
      // r13.acc2:f
      subregNum = 0;
      mme = ParseMathMacroReg();
    } else {
      // regular src with subregister
      // r13.4....
      //    ^
      if (Consume(DOT)) {
        ConsumeIntLitOrFail(subregNum, "expected subregister");
        hasExplicitSubreg = true;
      } else { // default to 0
        subregLoc = NextLoc(0);
        subregNum = 0;
      }
    }

    // for ternary ops <V;H> or <H>
    // for other regions <V;W,H>
    if (m_opSpec->isTernary()) {
      if (srcOpIx < 2) {
        rgn = ParseSrcOpRegionVH(srcOpIx, hasExplicitSubreg);
      } else {
        rgn = ParseSrcOpRegionH(srcOpIx, hasExplicitSubreg);
      }
    } else {
      rgn = ParseSrcOpRegionVWH(ri, srcOpIx, hasExplicitSubreg);
    }

    // :t
    Type sty = Type::INVALID;
    if (m_opSpec->isAnySendFormat()) {
      sty = ParseSendOperandTypeWithDefault(srcOpIx);
    } else {
      sty = ParseSrcOpTypeWithDefault(srcOpIx, false);
    }

    if (sty != Type::INVALID) {
      // ensure the subregister is not out of bounds
      int typeSize = TypeSizeInBits(sty) / 8;
      if (ri.isRegNumberValid(regNum) &&
          !ri.isSubRegByteOffsetValid(regNum, subregNum * typeSize,
                                      m_model.getGRFByteSize())) {
        // don't add an extra error if the parent register is
        // already out of bounds
        WarningS(subregLoc, "subregister out of bounds");
      } else if (typeSize < ri.accGran) {
        WarningS(regnameLoc, "register access granularity too small type");
      }
    }

    if (m_builder.isMacroOp()) {
      m_builder.InstSrcOpRegMathMacroExtReg(srcOpIx, opStart, srcMod,
                                            ri.regName, regNum, mme, rgn, sty);
    } else {
      RegRef reg((uint16_t)regNum, (uint16_t)subregNum);
      m_builder.InstSrcOpRegDirect(srcOpIx, opStart, srcMod, ri.regName, reg,
                                   rgn, sty);
    }
  }

  // '<' INT ';' INT ',' INT '>'     (VxWxH)
  Region ParseSrcOpRegionVWH(const RegInfo &ri, int srcOpIx,
                             bool hasExplicitSubreg) {
    if (m_opSpec->hasImplicitSrcRegion(srcOpIx, m_execSize,
                                       m_builder.isMacroOp())) {
      if (!LookingAt(LANGLE)) {
        return m_opSpec->implicitSrcRegion(srcOpIx, m_execSize,
                                           m_builder.isMacroOp());
      } else {
        WarningT(m_opSpec->mnemonic.str(), ".Src", srcOpIx,
                 " region should be implicit");
      }
    }

    Region rgn = Region::SRC010;
    if (Consume(LANGLE)) {
      rgn.set(ParseRegionVert());
      ConsumeOrFailAfterPrev(SEMI, "expected ;");
      rgn.set(ParseRegionWidth());
      ConsumeOrFailAfterPrev(COMMA, "expected ,");
      rgn.set(ParseRegionHorz());
      ConsumeOrFailAfterPrev(RANGLE, "expected >");
    } else if (m_opSpec->isAnySendFormat()) {
      rgn = defaultSendOperandRegion(ri.regName, srcOpIx);
    } else if (ri.supportsRegioning()) {
      // N.B. <1;1,0> won't coissue on PreGEN11
      rgn = hasExplicitSubreg || m_execSize == ExecSize::SIMD1 ? Region::SRC010
                                                               : Region::SRC110;
    } else {
      rgn = Region::SRC010;
    }
    return rgn;
  }

  // '<' INT ';' INT '>'   (CNL Align1 ternary)
  Region ParseSrcOpRegionVH(int srcOpIx, bool hasExplicitSubreg) {
    if (m_opSpec->hasImplicitSrcRegion(srcOpIx, m_execSize,
                                       m_builder.isMacroOp())) {
      if (!LookingAt(LANGLE)) {
        return m_opSpec->implicitSrcRegion(srcOpIx, m_execSize,
                                           m_builder.isMacroOp());
      } else if (m_opts.deprecatedSyntaxWarnings) {
        WarningT(m_opSpec->mnemonic.str(), ".Src", srcOpIx,
                 " region should be implicit");
      }
    }
    Region rgn = Region::SRC010;
    if (Consume(LANGLE)) {
      rgn.set(ParseRegionVert());
      ConsumeOrFailAfterPrev(SEMI, "expected ;");
      rgn.set(Region::Width::WI_INVALID);
      rgn.set(ParseRegionHorz());
      ConsumeOrFailAfterPrev(RANGLE, "expected >");
    } else {
      bool scalarAccess = hasExplicitSubreg || m_execSize == ExecSize::SIMD1;
      if (scalarAccess) {
        rgn = Region::SRC0X0;
      } else {
        // packed access
        rgn = platform() >= Platform::XE
                  ? Region::SRC1X0
                  : Region::SRC2X1; // most conservative mux
      }
    }
    return rgn;
  }

  // '<' INT '>'   (CNL Align1 ternary src2)
  Region ParseSrcOpRegionH(int srcOpIx, bool hasExplicitSubreg) {
    if (m_opSpec->hasImplicitSrcRegion(srcOpIx, m_execSize,
                                       m_builder.isMacroOp())) {
      if (!LookingAt(LANGLE)) {
        return m_opSpec->implicitSrcRegion(srcOpIx, m_execSize,
                                           m_builder.isMacroOp());
      } else if (m_opts.deprecatedSyntaxWarnings) {
        WarningT(m_opSpec->mnemonic.str(), ".Src", srcOpIx,
                 " region should be implicit");
      }
    }

    Region rgn;
    rgn.bits = 0; // needed to clear _padding
    if (Consume(LANGLE)) {
      rgn.set(Region::Vert::VT_INVALID, Region::Width::WI_INVALID,
              ParseRegionHorz());
      ConsumeOrFailAfterPrev(RANGLE, "expected >");
    } else {
      rgn = hasExplicitSubreg || m_execSize == ExecSize::SIMD1 ? Region::SRCXX0
                                                               : Region::SRCXX1;
    }
    return rgn;
  }

  Region defaultSendOperandRegion(RegName rn, int opIx) const {
    Region r = Region::INVALID;
    if (opIx < 0) {
      if (m_opSpec->hasImplicitDstRegion(m_builder.isMacroOp())) {
        r = m_opSpec->implicitDstRegion(m_builder.isMacroOp());
      }
    } else {
      if (m_opSpec->hasImplicitSrcRegion(opIx, m_execSize,
                                         m_builder.isMacroOp())) {
        r = m_opSpec->implicitSrcRegion(0, m_execSize, m_builder.isMacroOp());
      } else if (rn == RegName::ARF_NULL) {
        r = Region::SRC010;
      } else {
        r = Region::SRC110;
      }
    }
    return r;
  }

  // '<' INT ',' INT '>'             (VxH mode)
  // '<' INT ';' INT ',' INT '>'
  Region ParseSrcOpRegionInd(int srcOpIx, const Loc &opStart) {
    if (m_opSpec->hasImplicitSrcRegion(srcOpIx, m_execSize,
                                       m_builder.isMacroOp())) {
      if (!LookingAt(LANGLE)) {
        return m_opSpec->implicitSrcRegion(srcOpIx, m_execSize,
                                           m_builder.isMacroOp());
      } else if (m_opts.deprecatedSyntaxWarnings) {
        WarningT(m_opSpec->mnemonic.str(), ".Src", srcOpIx,
                 " region should be implicit");
      }
    }

    Region rgn;
    rgn.bits = 0;
    if (Consume(LANGLE)) {
      Loc arg1Loc = NextLoc();
      int arg1;
      ConsumeIntLitOrFail(arg1, "syntax error in source region");
      if (Consume(COMMA)) {
        // <W,H>
        rgn.set(Region::Vert::VT_VxH);
        switch (arg1) {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
          rgn.w = arg1;
          break;
        default:
          FailAtT(arg1Loc, "invalid region width");
          rgn.set(Region::Width::WI_INVALID);
        }
        rgn.set(ParseRegionHorz());
      } else {
        // <V;W,H>
        ConsumeOrFailAfterPrev(SEMI, "expected ;");
        switch (arg1) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
          rgn.v = arg1;
          break;
        default:
          FailAtT(arg1Loc, "invalid region vertical stride");
          rgn.set(Region::Vert::VT_INVALID);
        }
        rgn.set(ParseRegionWidth());
        ConsumeOrFailAfterPrev(COMMA, "expected ,");
        rgn.set(ParseRegionHorz());
      }
      ConsumeOrFailAfterPrev(RANGLE, "expected >");
    } else {
      rgn = Region::SRC110;
    }
    return rgn;
  }

  Region::Vert ParseRegionVert() {
    Loc loc = NextLoc();
    int x;
    ConsumeIntLitOrFail(x, "syntax error in region (vertical stride)");
    Region::Vert vs;
    switch (x) {
    case 0:
      vs = Region::Vert::VT_0;
      break;
    case 1:
      vs = Region::Vert::VT_1;
      break;
    case 2:
      vs = Region::Vert::VT_2;
      break;
    case 4:
      vs = Region::Vert::VT_4;
      break;
    case 8:
      vs = Region::Vert::VT_8;
      break;
    case 16:
      vs = Region::Vert::VT_16;
      break;
    case 32:
      vs = Region::Vert::VT_32;
      break;
    default:
      FailAtT(loc, "invalid region vertical stride");
      vs = Region::Vert::VT_INVALID;
    }
    return vs;
  }

  Region::Width ParseRegionWidth() {
    Loc loc = NextLoc();
    int x;
    ConsumeIntLitOrFail(x, "syntax error in region (width)");
    Region::Width wi;
    switch (x) {
    case 1:
      wi = Region::Width::WI_1;
      break;
    case 2:
      wi = Region::Width::WI_2;
      break;
    case 4:
      wi = Region::Width::WI_4;
      break;
    case 8:
      wi = Region::Width::WI_8;
      break;
    case 16:
      wi = Region::Width::WI_16;
      break;
    default:
      FailAtT(loc, "invalid region width");
      wi = Region::Width::WI_INVALID;
    }
    return wi;
  }

  Region::Horz ParseRegionHorz() {
    Loc loc = NextLoc();
    int x;
    ConsumeIntLitOrFail(x, "syntax error in region (horizontal stride)");
    Region::Horz hz;
    switch (x) {
    case 0:
      hz = Region::Horz::HZ_0;
      break;
    case 1:
      hz = Region::Horz::HZ_1;
      break;
    case 2:
      hz = Region::Horz::HZ_2;
      break;
    case 4:
      hz = Region::Horz::HZ_4;
      break;
    default:
      FailAtT(loc, "invalid region horizontal stride");
      hz = Region::Horz::HZ_INVALID;
    }
    return hz;
  }

  void CheckLiteralBounds(const Loc &opStart, Type type, const ImmVal &val,
                          int64_t mn, int64_t mx) {
    if (val.s64 < mn || val.s64 > mx) {
      WarningAtT(opStart, "literal is out of bounds for type ", ToSyntax(type));
    }
  }

  void FinishSrcOpImmValue(int srcOpIx, const Loc &opStart,
                           const Token &valToken, ImmVal &val) {
    // convert to the underlying data type
    Type sty = ParseSrcOpTypeWithoutDefault(srcOpIx, true);

    if (sty == Type::BF) {
      // ensure there's no BF type imm value
      ErrorAtT(opStart, "Imm operand with BF type is not allowed");
    }

    switch (sty) {
    case Type::B:
    case Type::UB:
    case Type::W:
    case Type::UW:
    case Type::D:
    case Type::UD:
    case Type::Q:
    case Type::UQ:
    case Type::V:
    case Type::UV:
    case Type::VF:
      if (!val.isS64()) {
        ErrorAtT(opStart, "literal must be integral for type ", ToSyntax(sty));
      }
      break;
    case Type::HF:
      if (val.isS64()) { // The value was parsed as integral
        if (valToken.lexeme == INTLIT10 && val.u64 != 0) {
          // base10
          // examples: 2:f or (1+2):f
          FailAtT(opStart, "immediate integer floating point literals must be "
                           "in hex or binary (e.g. 0x7F800000:f)");
        }
        // base2 or base16
        // e.g. 0x1234:hf  (preserve it)
        if (~0xFFFFull & val.u64) {
          ErrorAtT(opStart, "hex literal too big for type");
        }
        // no copy needed, the half float is already encoded as such
      } else { // ==ImmVal::F64
               // it's an fp64 literal, we need to narrow to fp16
               //   e.g. "(1.0/10.0):hf"
        uint64_t DROPPED_PAYLOAD =
            ~((uint64_t)F16_MANT_MASK) & (F64_MANT_MASK >> 1);
        if (IS_NAN(val.f64) && (val.u64 & DROPPED_PAYLOAD)) {
          FailAtT(opStart, "NaN payload value overflows");
        }
        // uint64_t orginalValue = val.u64;
        val.u64 = ConvertDoubleToHalf(val.f64); // copy over u64 to clobber all
        val.kind = ImmVal::Kind::F16;
        // uint64_t newValue = FloatToBits(
        //    ConvertFloatToDouble(ConvertHalfToFloat(val.u16)));
        // if (orginalValue != newValue) {
        //    Warning(opStart,
        //        "precision lost in literal conversion to fp16");
        // }
      }
      break;
    case Type::F:
    case Type::DF:
      if (val.isS64()) {
        if (valToken.lexeme == INTLIT10 && val.u64 != 0) {
          // base10
          // examples: 2:f or (1+2):f
          FailAtT(opStart, "immediate integer floating point literals must be "
                           "in hex or binary (e.g. 0x7F800000:f)");
        }
        // base2 or base16 float bits
        // examples:
        //   0x3F000000:f (0.5)
        //   0x7F801010:f (qnan(0x01010))
        //   0xFFC00000:f (-qnan(...))
        //   0x7FC00000:f (qnan())
        //   0x7F800000:f (infinity)
        if (sty == Type::F) {
          // preserve the bits
          val.u32 = (uint32_t)val.s64;
          val.kind = ImmVal::Kind::F32;
        } else {
          // leave :df alone, bits are already set
          val.kind = ImmVal::Kind::F64;
        }
      } else { // ==ImmVal::F64
        if (sty == Type::F) {
          // parsed as double e.g. "3.1414:f"
          // need to narrow to fp32
          // any NaN payload must fit in the smaller mantissa
          // The bits we will remove
          //   ~((uint64_t)F32_MANT_MASK) &
          //      (F64_MANT_MASK >> 1)
          // the mantissa bits that will get truncated
          uint64_t DROPPED_PAYLOAD =
              ~((uint64_t)F32_MANT_MASK) & (F64_MANT_MASK >> 1);
          if (IS_NAN(val.f64) && (val.u64 & DROPPED_PAYLOAD)) {
            FailS(opStart, "NaN payload value overflows");
          }
          //
          // Use a raw bitwise assignment; some compilers will clear
          // the NaN bit by making an assignment
          val.u64 = ConvertDoubleToFloatBits(val.f64);
          val.kind = ImmVal::Kind::F32;
          // the below would be wrong
          //   val = ConvertDoubleToFloat(val.f64);
        } // else: sty == Type::DF (nothing needed)
      }
      break;
    default:
      break;
    }

    // check literal bounds of integral values
    // literals may be signed floating in general
    switch (sty) {
    case Type::B:
      // if (val.kind == ImmVal::S64 &&
      //     ((val.u64 & 0xFFFFFFFFFFFFFF00ull) == 0xFFFFFFFFFFFFFF00ull))
      // {
      //     // negative value, but not too negative for 16 bits
      //     val.u64 &= 0xFFull;
      // }
      CheckLiteralBounds(opStart, sty, val, -128, 127);
      val.kind = ImmVal::Kind::S8;
      val.s64 = val.s8; // sign extend to a 64-bit value
      break;
    case Type::UB:
      // CheckLiteralBounds(opStart, sty, val, 0, 0xFF);
      val.kind = ImmVal::Kind::U8;
      val.u64 = val.u8; // could &= by 0xFF
      break;
    case Type::W:
      // if (val.kind == ImmVal::S64 &&
      //     ((val.u64 & 0xFFFFFFFFFFFF0000ull) == 0xFFFFFFFFFFFF0000ull))
      // {
      //     // negative value, but not too negative for 16 bits
      //    val.u64 &= 0xFFFFull;
      // }
      val.s64 = val.s16; // sign extend to a 64-bit value
      CheckLiteralBounds(opStart, sty, val, -32768, 32767);
      val.kind = ImmVal::Kind::S16;
      break;
    case Type::UW:
      // fails ~1:ub
      // CheckLiteralBounds(opStart, sty, val, 0, 0xFFFF);
      val.kind = ImmVal::Kind::U16;
      val.u64 = val.u16; // truncate to 16-bit: // could &= by 0xFFFF
      break;
    case Type::D:
      val.s64 = val.s32; // sign extend to a 64-bit value
      CheckLiteralBounds(opStart, sty, val, -2147483648ll, 2147483647ll);
      val.kind = ImmVal::Kind::S32;
      break;
    case Type::UD:
      // CheckLiteralBounds(opStart, sty, val, 0, 0xFFFFFFFF);
      // only check if input is signed??? Or reject signed input
      // if (val.kind == ImmVal::S64 &&
      //    ((val.u64 & 0xFFFFFFFF00000000ull) == 0xFFFFFFFF00000000ull))
      // {
      //     // negative value, but we can reprsent it as u32
      // }
      // val.u64 &= 0xFFFFFFFF;
      val.u64 = val.u32; // truncate top bits
      val.kind = ImmVal::Kind::U32;
      break;
    case Type::Q:
      // no conversion needed
      val.kind = ImmVal::Kind::S64;
      CheckLiteralBounds(opStart, sty, val, 0x8000000000000000ll,
                         0x7FFFFFFFFFFFFFFFll);
      break;
    case Type::UQ:
      // no conversion needed
      val.kind = ImmVal::Kind::U64;
      break;
    case Type::HF:
      val.kind = ImmVal::Kind::F16;
      break;
    case Type::F:
      val.kind = ImmVal::Kind::F32;
      break;
    case Type::DF:
      val.kind = ImmVal::Kind::F64;
      break;
    case Type::UV:
    case Type::V:
    case Type::VF:
      val.kind = ImmVal::Kind::U32;
      break;
    default:
      break;
    }

    if (m_opSpec->isBranching()) {
      if (m_opSpec->isJipAbsolute()) {
        m_builder.InstSrcOpImmLabelAbsolute(srcOpIx, opStart, val.s64, sty);
      } else {
        m_builder.InstSrcOpImmLabelRelative(srcOpIx, opStart, val.s64, sty);
      }
    } else {
      m_builder.InstSrcOpImmValue(srcOpIx, opStart, val, sty);
    }
  }

  void FinishSrcOpImmLabel(int srcOpIx, const Loc &opStart,
                           const Loc /* lblLoc */, const std::string &lbl) {
    Type type = ParseSrcOpTypeWithDefault(srcOpIx, true, true);
    m_builder.InstSrcOpImmLabel(srcOpIx, opStart, lbl, type);
  }

  // = '-' | '~' | '(abs)' | '-(abs)' | '~(abs)'
  // = or start of "|r3|" like
  SrcModifier ParseSrcModifierOpt(bool &pipeAbs) {
    SrcModifier srcMod = SrcModifier::NONE;
    if (Consume(SUB) || Consume(TILDE)) { // same lexeme as -
      srcMod = SrcModifier::NEG;
    }
    pipeAbs = LookingAt(PIPE);
    if (Consume(ABS) || Consume(PIPE)) {
      srcMod =
          srcMod == SrcModifier::NEG ? SrcModifier::NEG_ABS : SrcModifier::ABS;
    }
    return srcMod;
  }

  // Special handling for >=XE_HP src1 op, we rely on the src1 op format
  // to determine if this is the new XE_HP encoding.  If the format is
  // r2:4 then should set ExBSO to true later on.
  //
  // In XeHP: only ExBSO descriptors take Src1.Length out of the descriptor
  //         and only if the descriptor is a register (e.g. a0.2)
       //
       // The syntax:
       //   REG
       //      bare register: "r13" or "null"
       //      given a reg ExDesc means to not use ExBSO
       //      NOTE: users are encouraged to use the explicit InstOpt {ExBSO}
       //
       //   REG (COLON|HASH) INT
       //      src one length
       //      given a reg ExDesc, this means to use ExBSO
       //      NOTE: users are encouraged to use the explicit InstOpt {ExBSO}
       //
       //   REG COLON TYPE
  //      r13:ud or null:ud
  //
  //   REG COLON TYPE (COLON|HASH) INT
  //   REG (COLON|HASH) INT COLON TYPE
  //      compatibility with old asm
  //      r13:ud:1 or null:ud:0
  void ParseSendSrc1OpWithOptLen(int &src1Len) {
    m_srcLocs[1] = NextLoc();
    src1Len = -1;
    const RegInfo *regInfo;
    int regNum;
    bool parsedType = false;

    if (ConsumeReg(regInfo, regNum)) {
      // send src1 must not have region and subreg
      m_srcKinds[1] = Operand::Kind::DIRECT;
      if (regInfo->regName != RegName::ARF_NULL &&
          regInfo->regName != RegName::GRF_R) {
        FailAtT(m_srcLocs[1], "invalid src1 register");
      }
      //
      // parse the type
      Type sty = Type::INVALID;
      if (LookingAtSeq(COLON, IDENT)) {
        if (platform() >= Platform::XE3) {
          FailS(NextLoc(),
                "types are forbidden on send operands on this platform");
        }
       // this disables expressions for the length sizes
       //   send ... null:(2*0)
        sty = ParseSendOperandTypeWithDefault(1);
        parsedType = true;
      } else {
        sty = SendOperandDefaultType(1);
      }

      const Token &tk = Next();
      if (Consume(COLON) || Consume(HASH)) {
        // e.g. r12:4 or r12#4 (the latter is legacy only)
        // (we added COLON so we could use this with a preproecssor)
        if (platform() < Platform::XE_HP) {
          FailAtT(tk.loc, "Send with Imm ExDesc should not have Src1.Length "
                          "given on src1 (e.g. r0#4); Src1.Length should be "
                          "in ExDesc[10:6]");
        }
        m_sendSrcLenLocs[1] = NextLoc();
        //
        ImmVal v;
        if (!TryParseIntConstExpr(v, "extended descriptor")) {
          FailT("expected extended send descriptor");
        }
        src1Len = (int)v.s64;
        m_sendSrcLens[1] = src1Len;
        if (src1Len < 0 || src1Len > 0x1F) {
          FailAtT(tk.loc, "Src1.Length is out of range");
        }
        if (regNum + src1Len - 1 > 255) {
          FailAtT(tk.loc, "Src1.Length extends past GRF end");
        }
      } else if (regInfo->regName == RegName::GRF_R &&
                 LookingAtSeq(SUB, INTLIT10)) {
        // enabling a range based syntax as well for src1Len
        //   r12-14 => r12#3
        // users that require the C-preprocessor get tripped up using
        // # as a decorator (e.g. since it's a cpp operator)
        Skip();
        int lastReg = 0;
        ConsumeIntLit(lastReg);
        if (lastReg < regNum)
          FailAtT(tk.loc, "Src1Len: ending register must be >= start reg");
        else if (lastReg > 255)
          FailAtT(tk.loc, "Src1Len: ending register must be <= 255");
        src1Len = lastReg - regNum + 1;
      }

       // null#0:ud
      if (!parsedType)
        sty = ParseSendOperandTypeWithDefault(1);

      Region dfltRgn = Region::SRC010;
      if (m_opSpec->hasImplicitSrcRegion(1, m_execSize, false)) {
        dfltRgn = m_opSpec->implicitSrcRegion(1, m_execSize, false);
      }

      // construct the op directly
      m_builder.InstSrcOpRegDirect(1, m_srcLocs[1], SrcModifier::NONE,
                                   regInfo->regName, RegRef(regNum, 0), dfltRgn,
                                   sty);
    } else {
      FailT("syntax error in send src1");
    }
  }

  // e.g. "r13" or "r13:f"
  void ParseSendSrcOp(int srcOpIx, bool enableImplicitOperand) {
    m_srcLocs[srcOpIx] = NextLoc();

    const RegInfo *regInfo;
    int regNum;

    // For XE send now supports src1; sends goes away
    // To support some older syntax floating around
    // underneath IGA will insert a null src1 if needed
    if (enableImplicitOperand) {
      bool isSuccess = PeekReg(regInfo, regNum);
      if (!isSuccess || regInfo->regName == RegName::ARF_A) {
        m_builder.InstSrcOpRegDirect(
            srcOpIx, m_srcLocs[srcOpIx], SrcModifier::NONE, RegName::ARF_NULL,
            REGREF_ZERO_ZERO, Region::SRC010, Type::INVALID);
        return;
      }
    }

    ParseSrcOp(srcOpIx);
  }
  // e.g. "r13" or "r[s0.2]" (no suffix Src0Len)
  // returns if the operand is indirect (gathering)
  bool ParseSendSrc0OpXe3() {
    m_srcLocs[0] = NextLoc();

    OperandInfo opInfo;
    if (ConsumeIdentEq("r")) {
      m_srcKinds[0] = opInfo.kind = Operand::Kind::INDIRECT;
      ConsumeOrFail(LBRACK, "expected [");
      auto sreg = ParseReg();
      if (sreg.first->regName != RegName::ARF_S) {
        FailAtT(m_srcLocs[0], "gathering send expects s0.# index");
      }
      ConsumeOrFail(DOT, "expected subregister");
      int subregNum = 0;
      ConsumeIntLitOrFail(subregNum, "expected subregister");
      opInfo.regOpIndOff = 0;
      opInfo.regOpReg = RegRef(sreg.second, (uint16_t)subregNum);
      opInfo.regOpName = RegName::ARF_S;
      ConsumeOrFail(RBRACK, "expected ]");
    } else {
      m_srcKinds[0] = opInfo.kind = Operand::Kind::DIRECT;
      auto reg = ParseReg();
      opInfo.regOpName = reg.first->regName;
      opInfo.regOpReg.regNum = reg.second;
      opInfo.regOpReg.subRegNum = 0;
    }

    m_builder.InstSrcOp(0, opInfo);
    return opInfo.kind == Operand::Kind::INDIRECT;
  }

  // e.g. "r13:2" or "r[s0.2]:8" (suffix Src0Len)
  // returns if the operand is indirect (gathering)
  bool ParseSendgSrc0OpXe3p() {
    m_srcLocs[0] = NextLoc();

    OperandInfo opInfo;
    if (ConsumeIdentEq("r")) {
      m_srcKinds[0] = opInfo.kind = Operand::Kind::INDIRECT;
      ConsumeOrFail(LBRACK, "expected [");
      auto sreg = ParseReg();
      if (sreg.first->regName != RegName::ARF_S) {
        FailAtT(m_srcLocs[0], "gathering send expects s0.# index");
      }
      ConsumeOrFail(DOT, "expected subregister");
      int subregNum = 0;
      ConsumeIntLitOrFail(subregNum, "expected subregister");
      opInfo.regOpIndOff = 0;
      opInfo.regOpReg = RegRef(sreg.second, (uint16_t)subregNum);
      opInfo.regOpName = RegName::ARF_S;
      ConsumeOrFail(RBRACK, "expected ]");
      if (m_opSpec->is(Op::SENDGX) || m_opSpec->is(Op::SENDGXC)) {
          FailAtT(m_srcLocs[0], "Sendgx/sendgxc forbids gather src0 operand");
      }
    } else {
      m_srcKinds[0] = opInfo.kind = Operand::Kind::DIRECT;
      auto reg = ParseReg();
      // for sendgx, check is src0 is ARF_NULL
      // if ARF_NULL, change to r511
      if ((m_opSpec->is(Op::SENDGX) || m_opSpec->is(Op::SENDGXC)) &&
           reg.first->regName == RegName::ARF_NULL) {
        opInfo.regOpName = RegName::ARF_NULL;
        opInfo.regOpReg.regNum = (uint16_t)0;
        opInfo.regOpReg.subRegNum = 0;
      } else {
        opInfo.regOpName = reg.first->regName;
        opInfo.regOpReg.regNum = reg.second;
        opInfo.regOpReg.subRegNum = 0;
      }
    }
    int src0Len = 0;
    if (LookingAt(COLON) || opInfo.regOpName == RegName::GRF_R)
      src0Len = ParseSrcOpLenSuffix((int)opInfo.regOpReg.regNum);
    m_builder.InstSrcOp(0, opInfo);
    m_builder.InstSendSrc0Length(src0Len);

    return opInfo.kind == Operand::Kind::INDIRECT;
  }
  void ParseSendgSrc1OpXe3() {
    OperandInfo opInfo;
    m_srcLocs[1] = NextLoc();
    m_srcKinds[1] = opInfo.kind = Operand::Kind::DIRECT;
    auto reg = ParseReg();
      // for sendgx, check if src1 is ARF_NULL
      // if ARF_NULL, then change to r511
    if ((m_opSpec->is(Op::SENDGX) || m_opSpec->is(Op::SENDGXC)) &&
        reg.first->regName == RegName::ARF_NULL) {
      opInfo.regOpName = RegName::ARF_NULL;
      opInfo.regOpReg.regNum = (uint16_t)0;
      opInfo.regOpReg.subRegNum = 0;
    }
    else
    {
      opInfo.regOpName = reg.first->regName;
      opInfo.regOpReg.regNum = reg.second;
      opInfo.regOpReg.subRegNum = 0;
    }
    int src1Len = 0;
    if (LookingAt(COLON) || opInfo.regOpName != RegName::ARF_NULL)
      src1Len = ParseSrcOpLenSuffix((int)opInfo.regOpReg.regNum);
    m_builder.InstSrcOp(1, opInfo);
    m_builder.InstSendSrc1Length(src1Len);
  }


  std::pair<const RegInfo *, uint16_t> ParseReg() {
    const RegInfo *regInfo = nullptr;
    int regNum = -1;
    if (!ConsumeReg(regInfo, regNum)) {
      FailT("expected register");
    } else if (!regInfo->isRegNumberValid(regNum)) {
      FailT("invalid register number");
    }
    if (m_opSpec->is(Op::SENDGX) || m_opSpec->is(Op::SENDGXC)) {
      if (regNum == 511) {
        IGA_ASSERT(false, "r511 is null and should not be referenced directly");
      }
    }
    return std::pair<const RegInfo *, uint16_t>(regInfo, (uint16_t)regNum);
  }

  int ParseSrcOpLenSuffix(int regNum) {
    int srcLen = -1;
    ConsumeOrFail(COLON, "expected payload length suffix (e.g. :2)");

    int regLimit = 255;
    if (m_opSpec->is(Op::SENDGX) || m_opSpec->is(Op::SENDGXC)) {
      regLimit = 511;
    }

    // explicit length syntax
    //   e.g. "r13:4"
    //   e.g. "r13:(2*2)"
    // NOTE: permit : so we can use this in the preprocessor
    // e.g. r13:4
    auto at = NextLoc();
    ImmVal v;
    if (!TryParseIntConstExpr(v, "extended descriptor")) {
      FailT("expected extended send descriptor");
    }
    if (v.s64 < 0 || v.s64 > 0x1F) {
      FailAtT(at, "SrcLen out of range");
    } else if (regNum + (int)v.s64 - 1 > regLimit) {
      WarningAtT(at, "SrcLen register range extends past GRF end");
    }
    if (v.s64 > 32) { // could constrain this more
      FailAtT(at, "invalid payload length");
    }
    srcLen = (int)v.s64;

    return srcLen;
  }

  Type ParseDstOpTypeWithDefault() {
    if (m_opSpec->hasImplicitDstType()) {
      if (LookingAt(COLON)) {
        if (m_opts.deprecatedSyntaxWarnings)
          WarningT("implicit type on dst should be omitted");
        // parse the type but ignore it
        ParseOpTypeWithDefault(DST_TYPES, "expected destination type");
      }
      // use the implicit type anyway
      return m_opSpec->implicitDstType();
    }
    return ParseOpTypeWithDefault(DST_TYPES, "expected destination type");
  }

  Type ParseSrcOpTypeWithDefault(int srcOpIx, bool immOrLbl,
                                 bool isLabel = false) {
    if (m_opSpec->hasImplicitSrcType(srcOpIx, immOrLbl)) {
      if (LookingAt(COLON)) {
        if (m_opts.deprecatedSyntaxWarnings)
          WarningT("implicit type on src", srcOpIx, " should be omitted");
        // parse the type but ignore it
        ParseOpTypeWithDefault(SRC_TYPES, "expected source type");
      }
      // use the implicit type anyway
      return m_opSpec->implicitSrcType(srcOpIx, immOrLbl);
    } else if (m_opSpec->op == Op::MOV && immOrLbl && isLabel) {
      // support mov label without giving label's type
      return Type::UD;
    }

    return ParseOpTypeWithDefault(SRC_TYPES, "expected source type");
  }
  Type ParseSrcOpTypeWithoutDefault(int srcOpIx, bool immOrLbl) {
    if (m_opSpec->hasImplicitSrcType(srcOpIx, immOrLbl)) {
      if (LookingAt(COLON)) {
        if (m_opts.deprecatedSyntaxWarnings)
          WarningT("implicit type on src", srcOpIx, " should be omitted");
        // parse the type but ignore it
        TryParseOpType(SRC_TYPES);
      }
      // use the implicit type anyway
      return m_opSpec->implicitSrcType(srcOpIx, immOrLbl);
    }
    Type t = TryParseOpType(SRC_TYPES);
    if (t == Type::INVALID &&
        !(m_opSpec->isBranching() && !m_model.supportsSimplifiedBranches())) {
      FailT("expected source type");
    }
    return t;
  }
  Type ParseOpTypeWithDefault(const IdentMap<Type> types,
                              const char *expectedErr) {
    auto t = TryParseOpType(types);
    if (t == Type::INVALID) {
      if (m_defaultRegisterType != Type::INVALID) {
        t = m_defaultRegisterType;
      } else if (m_opSpec->isAnySendFormat()) {
        t = Type::UD;
      } else if (m_opSpec->isBranching() &&
                 m_model.supportsSimplifiedBranches()) {
        // no more types for branching
        t = Type::UD;
      } else if (m_opSpec->is(Op::SYNC)) {
        // we allow implicit type for sync reg32 (grf or null)
        t = Type::UB;
      } else {
        FailT(expectedErr);
      }
    }
    return t;
  }
  Type TryParseOpType(const IdentMap<Type> types) {
    if (!LookingAt(COLON)) {
      return Type::INVALID;
    }
    Type type = Type::INVALID;
    if (IdentLookupFrom(1, types, type)) {
      Skip(2);
    }
    return type;
  }

  // (INTEXPR|AddrRegRef) (INTEXPR|AddrRegRef)
  //
  // This function is the same as ParseSendDescs, except for it's handling
  // of ExBSO and Src1Len.
  void ParseSendDescsWithOptSrc1Len(int src1Length) {
    const Loc exDescLoc = NextLoc();
    SendDesc exDesc;
    RegName regName = RegName::INVALID;
    if (ParseAddrRegRefOpt(exDesc.reg, regName)) { // ExDesc is register
      IGA_ASSERT(regName == RegName::ARF_A,
                 "Indirect send exDesc must be ARF_A");
      exDesc.type = SendDesc::Kind::REG32A;
      if (src1Length >= 0) {
        m_implicitExBSO = true;
        m_builder.InstOptAdd(InstOpt::EXBSO);
      }
    } else { // ExDesc is imm
      exDesc.type = SendDesc::Kind::IMM;

      // constant integral expression
      ImmVal v;
      if (!TryParseIntConstExpr(v, "extended descriptor")) {
        FailT("expected extended send descriptor");
      }
      exDesc.imm = (uint32_t)v.s64;

      if (src1Length >= 0) {
        // In XeHP, immediate descriptors still treat Src1.Length as
        // ExDesc[10:6]
        if (platform() == Platform::XE_HP) {
          WarningT("Send with immediate ExDesc should not have "
                   "Src1.Length given on src1 (e.g. r10:4) "
                   "on this platform");
          exDesc.imm |= (((uint32_t)src1Length & 0x1F) << 6);
        }
      }
      if (platform() >= Platform::XE_HPG) {
        // XeHPG+: Src1.Length is not part of ExDesc for ExDesc.IsImm,
        // but are explicit bits in the EU ISA.
        int exDescSrc1Len = (int)(exDesc.imm >> 6) & 0x1F;
        if (src1Length < 0) {
          WarningAtT(exDescLoc,
                     "Src1.Length should suffix src1 register (e.g. r10:4)");
          // take it from the descriptor
          src1Length = exDescSrc1Len;
        } else if (exDescSrc1Len != 0) { // src1Length >= 0
          // they set Src1.Length as part of the descriptor
          WarningAtT(exDescLoc,
                     "Send ExDesc[10:6] is not Src1.Length; suffix length "
                     "on src1 reg  (e.g. r10:4)");
          // if Src1.Length was also set on the register, then ensure
          // that it at least matches the descriptor value
          IGA_ASSERT(src1Length >= 0, "Unexpected value");
          // throw a fit if they mismatch
          if (exDescSrc1Len != src1Length)
            FailAtT(exDescLoc, "mismatch of Src1.Length suffix and "
                               "ExDesc[10:6]");
        } // else: Src1Length >= 0 && exDescSrc1Len == 0 (prefer reg op)

        if (exDesc.imm & 0x7FF) {
          WarningAtT(exDescLoc, "ExDesc[10:0] must be zero");
          exDesc.imm &= ~0x7FF; // [10:0] MBZ
        }
      } // else <=XeHPG (everything is in the imm descriptor)

      if (platform() >= Platform::XE && (exDesc.imm & 0xF)) {
        FailAtT(exDescLoc, "ExDesc[3:0] must be 0's; SFID is expressed "
                           "as a function control value (e.g. send.dc0 ...)");
      }
    }
    if (LookingAt(COLON)) {
      FailAtT(NextLoc(), "extended message descriptor is typeless");
    }
    //
    const Loc descLoc = NextLoc();
    SendDesc desc;
    if (ParseAddrRegRefOpt(desc.reg, regName)) {
      IGA_ASSERT(regName == RegName::ARF_A, "Indirect send desc must be ARF_A");
      desc.type = SendDesc::Kind::REG32A;
    } else {
      // constant integral expression
      ImmVal v;
      if (!TryParseConstExpr(v)) {
        FailT("expected extended send descriptor");
      }
      if (!v.isI64()) {
        FailAtT(descLoc, "immediate descriptor expression must be integral");
      }
      desc.imm = (uint32_t)v.s64;
      desc.type = SendDesc::Kind::IMM;
    }
    if (LookingAt(COLON)) {
      FailT("Message Descriptor should not have a type");
    }
    //
    m_builder.InstSendDescs(exDescLoc, exDesc, descLoc, desc);
    //
    m_builder.InstSendSrc1Length(src1Length);
  }

  SendDesc ParseDesc(const char *which) {
    SendDesc sd;
    RegName regName = RegName::INVALID;
    if (ParseAddrRegRefOpt(sd.reg, regName)) { // ExDesc is register
      IGA_ASSERT(regName == RegName::ARF_A, "Indirect send Desc must be ARF_A");
      sd.type = SendDesc::Kind::REG32A;

    } else { // ExDesc is imm
      sd.type = SendDesc::Kind::IMM;

      // constant integral expression
      ImmVal v;
      if (!TryParseIntConstExprPrimary(ExprParseOpts(), v, which)) {
        FailT("expected ", which);
      }
      sd.imm = (uint32_t)v.s64;
    }
    return sd;
  }

  // Xe2 version of descriptors:
  // (IntExpr COLON)?(INTEXPR|AddrRegRef) (INTEXPR|AddrRegRef)
  // Examples:
  //    0x1234      0x56780002
  //    a0.2        a0.0
  //    0x100:a0.2  0x56780001
  void ParseSendDescsXe2(int src1Len) {
    const Loc exDescLoc = NextLoc();
    ImmVal v;
    uint32_t exImmOffDesc = 0;
    SendDesc exDesc, desc;

    bool z = TryParseIntConstExprPrimary(
        v, "extended descriptor or immediate offset");
    if (z) {
      if (Consume(COLON)) {
        exDesc = ParseDesc("extended descriptor");
        exImmOffDesc = (uint32_t)v.s64;
      } else {
        exDesc.type = SendDesc::Kind::IMM;
        exDesc.imm = (uint32_t)v.s64;
      }
    } else {
      exDesc = ParseDesc("extended descriptor");
    }

    // src1Length set implies ExBSO is set when ExBSO is applicable
    // ExBSO is applicable when exDesc is reg and non-UGM
    if (src1Len >= 0 && exDesc.type != SendDesc::Kind::IMM &&
        m_builder.getSubfunction().send != SFID::UGM) {
      m_implicitExBSO = true;
      m_builder.InstOptAdd(InstOpt::EXBSO);
    }

    const Loc descLoc = NextLoc();
    desc = ParseDesc("Descriptor");

    m_builder.InstSendDescs(exDescLoc, exImmOffDesc, exDesc, descLoc, desc);
    m_builder.InstSendSrc1Length(src1Len);
  }

  void ParseSendgDescsXe3() {
    RegRef id0{REGREF_INVALID};
    RegRef id1{REGREF_INVALID};
    auto at = NextLoc();
    (void)at;
    if (ParseScalarRegRefOpt(id0)) {
      ParseScalarRegRefOpt(id1);
    }
    const Loc exDescLoc = NextLoc();
    ImmVal v;
    bool z = TryParseIntConstExprPrimary(v, "descriptor");
    if (!z)
      FailAtT(exDescLoc, "expected send descriptor");
    m_builder.InstSendDescs(v.u64, id0, id1);
  }

  //
  // (INTEXPR|AddrRegRef) (INTEXPR|AddrRegRef)
  void ParseSendDescsLegacy() {
    IGA_ASSERT(platform() <= Platform::XE, "wrong platform for function");

    const Loc exDescLoc = NextLoc();
    SendDesc exDesc;
    RegName regName = RegName::INVALID;
    if (ParseAddrRegRefOpt(exDesc.reg, regName)) {
      IGA_ASSERT(regName == RegName::ARF_A,
                 "Indirect send exDesc must be ARF_A");
      exDesc.type = SendDesc::Kind::REG32A;
      if (platform() < Platform::XE)
        m_builder.InstSubfunction(SFID::A0REG);
      // subfunc is already set as part of opcode (e.g. send.gtwy)
    } else {
      // constant integral expression
      ImmVal v;
      if (!TryParseConstExpr(v)) {
        FailT("expected extended send descriptor");
      }
      if (!v.isI64()) {
        FailAtT(exDescLoc, "immediate descriptor expression must be integral");
      }
      exDesc.imm = (uint32_t)v.s64;
      exDesc.type = SendDesc::Kind::IMM;
      if (platform() >= Platform::XE && (exDesc.imm & 0xF)) {
        FailAtT(exDescLoc, "ExDesc[3:0] must be 0's; SFID is expressed "
                           "as a function control value (e.g. send.dc0 ...)");
      }
      if (platform() < Platform::XE) {
        SFID sfid = sfidFromEncoding(platform(), exDesc.imm & 0xF);
        m_builder.InstSubfunction(sfid);
      }
    }

    if (LookingAt(COLON)) {
      FailT("extended message descriptor is typeless");
    }

    const Loc descLoc = NextLoc();
    SendDesc desc;
    if (ParseAddrRegRefOpt(desc.reg, regName)) {
      IGA_ASSERT(regName == RegName::ARF_A, "Indirect send Desc must be ARF_A");
      desc.type = SendDesc::Kind::REG32A;
    } else {
      // constant integral expression
      ImmVal v;
      if (!TryParseConstExpr(v)) {
        FailT("expected extended send descriptor");
      }
      if (!v.isI64()) {
        FailAtT(descLoc, "immediate descriptor expression must be integral");
      }
      desc.imm = (uint32_t)v.s64;
      desc.type = SendDesc::Kind::IMM;
    }

    m_builder.InstSendDescs(exDescLoc, exDesc, descLoc, desc);

    if (LookingAt(COLON)) {
      FailT("Message Descriptor is typeless");
    }
  }

  // ('{' InstOptOrDepInfo (',' IDENT)* '}')?
  // InstOptOrDepInfo = DepInfo | InstOpt
  //   InstOpt = 'AccWrEn' | 'Atomic' | ...
  //   DepInfo = 'NoDDChk' | 'NoDDClr' | ...
  void ParseInstOpts() {
    InstOptSet instOpts;
    instOpts.clear();

    if (Consume(LBRACE)) {
      if (!LookingAt(RBRACE))
        ParseInstOptOrFail(instOpts); // else: could be empty list "{}"
      while (Consume(COMMA)) {
        ParseInstOptOrFail(instOpts);
      }
      ConsumeOrFail(RBRACE, "expected }");
    }

    bool src1LengthSuffixSet = m_sendSrcLens[1] != -1;
    if (instOpts.contains(InstOpt::EXBSO) && !src1LengthSuffixSet
        && !isParsingGatherSend() // Gather Send must have ExBSO set but it has
                                  // no explicit src1 length
    ) {
      // GOOD:  send ... r10:2  a0.# ... {ExBSO}
      // ERROR: send ... r10    a0.# ... {ExBSO}
      //   Src1.Length comes from EU bits a0.# holds 26b offset
      //   We throw a tantrum if length is absent
      auto loc = m_srcLocs[1].isValid() ? m_srcLocs[1] : m_mnemonicLoc;
      FailAtT(loc, "send with ExBSO option should have "
                   "Src1.Length suffixing parameter (e.g. r10:4)");
      ////////////////////////////
      //  else if:
      //    platform() >= Platform::XE_HPG
      //    m_builder.getExDesc().isImm() &&
      //    src1LengthSuffixSet
      // GOOD:  send ... r10:2  IMM ... {}
      // GOOD:  send ... r10    a0.# ... {}
      // this is already checked in ExDesc parser
      //
    } else if (instOpts.contains(InstOpt::EXBSO) &&
               m_builder.getExDesc().isImm()) {
      // ERROR: send ... r10    IMM ... {ExBSO}
      // ExBSO only makes sense when using a0.#
      auto loc = m_srcLocs[1].isValid() ? m_srcLocs[1] : m_mnemonicLoc;
      FailAtT(loc, "send with immediate exdesc forbids ExBSO");
    }

    m_builder.InstOptsAdd(instOpts);

    if (m_implicitExBSO && !instOpts.contains(InstOpt::EXBSO)
        && !isParsingGatherSend() // gather-send must have ExBSO set but it has
                                  // no explicit src1 length
        ) {
      WarningAtT(m_mnemonicLoc, "send src1 length implicitly added "
                                "(include {ExBSO})");
    }
  }

  void ParseInstOptOrFail(InstOptSet &instOpts) {
    auto loc = NextLoc();
    if (!tryParseInstOptToken(instOpts) && !tryParseInstOptDepInfo(instOpts)) {
      FailAtT(loc, "invalid instruction option");
    }
  }

  bool tryParseInstOptToken(InstOptSet &instOpts) {
    auto loc = NextLoc();
    InstOpt newOpt = InstOpt::ACCWREN;
    if (ConsumeIdentEq("AccWrEn")) {
      newOpt = InstOpt::ACCWREN;
      if (platform() >= Platform::XE_HPC) {
        FailAtT(loc, "AccWrEn not supported on this platform");
      }
    } else if (ConsumeIdentEq("Atomic")) {
      if (platform() < Platform::GEN7) {
        FailAtT(loc, "Atomic mot supported on given platform");
      }
      newOpt = InstOpt::ATOMIC;
      if (instOpts.contains(InstOpt::SWITCH)) {
        FailAtT(loc, "Atomic mutually exclusive with Switch");
      } else if (instOpts.contains(InstOpt::NOPREEMPT)) {
        FailAtT(loc, "Atomic mutually exclusive with NoPreempt");
      }
    } else if (ConsumeIdentEq("Breakpoint")) {
      newOpt = InstOpt::BREAKPOINT;
    } else if (ConsumeIdentEq("Compacted")) {
      newOpt = InstOpt::COMPACTED;
      if (instOpts.contains(InstOpt::NOCOMPACT)) {
        FailAtT(loc, "Compacted mutually exclusive with "
                     "Uncompacted/NoCompact");
      }
    } else if (ConsumeIdentEq("EOT")) {
      newOpt = InstOpt::EOT;
      if (!m_opSpec->isAnySendFormat()) {
        FailAtT(loc, "EOT is only allowed on send instructions");
      }
    } else if (ConsumeIdentEq("NoCompact") || ConsumeIdentEq("Uncompacted")) {
      newOpt = InstOpt::NOCOMPACT;
      if (instOpts.contains(InstOpt::COMPACTED)) {
        FailAtT(loc, "Uncomapcted/NoCompact mutually exclusive "
                     "with Compacted");
      }
    } else if (ConsumeIdentEq("Serialize")) {
      newOpt = InstOpt::SERIALIZE;
    } else if (ConsumeIdentEq("NoMask")) {
      FailAtT(loc, "NoMask goes precedes predication as (W) for WrEn: "
                   "e.g. (W) op (..) ...   or    (W&f0.0) op (..) ..");
    } else if (ConsumeIdentEq("H1")) {
      FailAtT(loc, "H1 is obsolete; use M0 in execution offset: "
                   "e.g. op (16|M0) ...");
    } else if (ConsumeIdentEq("H2")) {
      FailAtT(loc, "H2 is obsolete; use M16 in execution offset: "
                   "e.g. op (16|M16) ...");
    } else if (ConsumeIdentEq("Q1")) {
      FailAtT(loc, "Q1 is obsolete; use M0 in execution offset: "
                   "e.g. op (8|M0) ...");
    } else if (ConsumeIdentEq("Q2")) {
      FailAtT(loc, "Q2 is obsolete; use M8 in execution offset: "
                   "e.g. op (8|M8) ...");
    } else if (ConsumeIdentEq("Q3")) {
      FailAtT(loc, "Q3 is obsolete; use M16 in execution offset: "
                   "e.g. op (8|M16) ...");
    } else if (ConsumeIdentEq("Q4")) {
      FailAtT(loc, "Q4 is obsolete; use M24 in execution offset: "
                   "e.g. op (8|M24) ...");
    } else if (ConsumeIdentEq("N1")) {
      FailAtT(loc, "N1 is obsolete; use M0 in execution offset: "
                   "e.g. op (4|M0) ...");
    } else if (ConsumeIdentEq("N2")) {
      FailAtT(loc, "N2 is obsolete; use M4 in execution offset: "
                   "e.g. op (4|M4) ...");
    } else if (ConsumeIdentEq("N3")) {
      FailAtT(loc, "N3 is obsolete; use M8 in execution offset: "
                   "e.g. op (4|M8) ...");
    } else if (ConsumeIdentEq("N4")) {
      FailAtT(loc, "N4 is obsolete; use M12 in execution offset: "
                   "e.g. op (4|M12) ...");
    } else if (ConsumeIdentEq("N5")) {
      FailAtT(loc, "N5 is obsolete; use M16 in execution offset: "
                   "e.g. op (4|M16) ...");
    } else if (ConsumeIdentEq("N6")) {
      FailAtT(loc, "N6 is obsolete; use M20 in execution offset: "
                   "e.g. op (4|M20) ...");
    } else if (ConsumeIdentEq("N7")) {
      FailAtT(loc, "N7 is obsolete; use M24 in execution offset: "
                   "e.g. op (4|M24) ...");
    } else if (ConsumeIdentEq("N8")) {
      FailAtT(loc, "N8 is obsolete; use M28 in execution offset: "
                   "e.g. op (4|M28) ...");
    } else {
      return false;
    }

    if (!instOpts.add(newOpt)) {
      // adding the option doesn't change the set... (it's duplicate)
      FailAtT(loc, "duplicate instruction options");
    }
    return true;
  }

  bool tryParseInstOptDepInfo(InstOptSet &instOpts) {
    auto loc = NextLoc();
    if (LookingAt(IDENT)) {
      InstOpt newOpt = InstOpt::ACCWREN;
      bool isSwsbOpt = false;
      // classic instruction option that affects instruction dependency
      // scheduling etc...
      if (ConsumeIdentEq("NoDDChk")) {
        newOpt = InstOpt::NODDCHK;
        if (!m_model.supportsHwDeps()) {
          FailAtT(loc, "NoDDChk not supported on given platform");
        }
      } else if (ConsumeIdentEq("NoDDClr")) {
        newOpt = InstOpt::NODDCLR;
        if (!m_model.supportsHwDeps()) {
          FailAtT(loc, "NoDDClr not supported on given platform");
        }
      } else if (ConsumeIdentEq("NoPreempt")) {
        if (m_model.supportsNoPreempt()) {
          newOpt = InstOpt::NOPREEMPT;
        } else {
          FailAtT(loc, "NoPreempt not supported on given platform");
        }
      } else if (ConsumeIdentEq("NoSrcDepSet")) {
        if (m_model.supportNoSrcDepSet()) {
          newOpt = InstOpt::NOSRCDEPSET;
        } else {
          FailAtT(loc, "NoSrcDep not supported on given platform");
        }
      } else if (ConsumeIdentEq("Switch")) {
        newOpt = InstOpt::SWITCH;
        if (!m_model.supportsHwDeps()) {
          WarningAtT(loc, "ignoring unsupported instruction option {Switch}");
        }
      } else if (ConsumeIdentEq("ExBSO")) {
        if (platform() < Platform::XE_HP) {
          FailAtT(loc, "ExBSO not supported on this platform");
        } else if (!m_opSpec->isAnySendFormat()) {
          FailAtT(loc, "ExBSO is not allowed for non-send instructions");
        }
        newOpt = InstOpt::EXBSO;
      } else if (ConsumeIdentEq("CPS")) {
        if (platform() < Platform::XE_HP || platform() >= Platform::XE2) {
          FailAtT(loc, "CPS not supported on this platform");
        } else if (!m_opSpec->isAnySendFormat()) {
          FailAtT(loc, "CPS is not allowed for non-send instructions");
        }
        newOpt = InstOpt::CPS;
      } else if (ConsumeIdentEq("Fwd")) {
        if (!m_opSpec->isDpasFormat())
          FailAtT(loc, "FWD is not allowed for non-dpas instructions");
        if (platform() <= Platform::XE3)
          FailAtT(loc, "FWD not supported on this platform");
        newOpt = InstOpt::FWD;
      } else if (ConsumeIdentEq("NoAccSBSet")) {
        // try swsb special token: NoAccSBSet
        m_builder.InstDepInfoSpecialToken(loc, SWSB::SpecialToken::NOACCSBSET);
        isSwsbOpt = true;
      } else if (m_opts.swsbEncodeMode >= SWSB_ENCODE_MODE::ThreeDistPipe) {
        // swsb XE encoding: the distance could be A@, F@, L@, I@, M@
        // same pipe dist (e.g. @4) will parse further down
        auto tryParsePipe = [&](const char *distPipeSymbol,
                                SWSB::DistType distPipe) {
          if (LookingAtSeq(IDENT, AT) && ConsumeIdentEq(distPipeSymbol) &&
              Consume(AT)) {
            int32_t regDist = 0;
            Loc distLoc = NextLoc();
            ConsumeIntLitOrFail(regDist, "expected register distance value");
            m_builder.InstDepInfoDist(distLoc, distPipe, regDist);
            return true;
          } else {
            return false;
          }
        };
        //
        bool parsedNamedPipe =
            tryParsePipe("S", SWSB::DistType::REG_DIST_SCALAR) ||
            tryParsePipe("F", SWSB::DistType::REG_DIST_FLOAT) ||
            tryParsePipe("I", SWSB::DistType::REG_DIST_INT) ||
            tryParsePipe("L", SWSB::DistType::REG_DIST_LONG) ||
            tryParsePipe("A", SWSB::DistType::REG_DIST_ALL) ||
            tryParsePipe("M", SWSB::DistType::REG_DIST_MATH);
        if (parsedNamedPipe && m_model.supportsHwDeps()) {
          FailAtT(loc, "software dependencies not supported on this platform");
        }

        isSwsbOpt = true;
        if (!parsedNamedPipe)
          return false; // unrecognized swsb setting
      } else {
        return false; // unrecognized option
      }
      if (!isSwsbOpt && !instOpts.add(newOpt)) {
        // adding the option doesn't change the set... (it's a duplicate)
        FailAtT(loc, "duplicate instruction options");
      }
    } else if (Consume(DOLLAR)) {
      // sbid directive
      if (m_model.supportsHwDeps()) {
        FailAtT(loc, "software dependencies not supported on this platform");
      }
      int32_t sbid;
      ConsumeIntLitOrFail(sbid, "expected SBID token");
      // $4
      if (Consume(DOT)) {
        if (ConsumeIdentEq("dst")) {
          // $4.dst
          m_builder.InstDepInfoSBidDst(loc, sbid);
        } else if (ConsumeIdentEq("src")) {
          // $4.src
          m_builder.InstDepInfoSBidSrc(loc, sbid);
        } else if (ConsumeIdentEq("inc")) {
          // $4.inc
          m_builder.InstDepInfoSBidCntr(loc, sbid);
        } else {
          FailT("invalid SBID directive expecting 'dst' or 'src'");
        }
      } else if (!LookingAtAnyOf(COMMA, RBRACE)) {
        FailT("syntax error in SBID directive"
              " (expected '.' ',' or '}')");
      } else {
        // $4 (allocate/.set)
        m_builder.InstDepInfoSBidAlloc(loc, sbid);
      }
    } else if (Consume(AT)) {
      // min dependency distance @3
      if (m_model.supportsHwDeps()) {
        FailAtT(loc, "software dependencies not supported on this platform");
      }
      int32_t dist;
      auto loc = NextLoc();
      ConsumeIntLitOrFail(dist, "expected register min distance value");
      m_builder.InstDepInfoDist(loc, SWSB::DistType::REG_DIST, dist);
    } else {
      return false;
    }
    return true;
  }

  // FlagRegRef = ('f0'|'f1'|'f2'|'f3') ('.' ('0'|'1'))?
  void ParseFlagRegRef(RegRef &freg) {
    if (!LookingAt(IDENT)) {
      FailT("expected flag register");
    }

    if (ConsumeIdentEq("f0")) {
      freg.regNum = 0;
    } else if (ConsumeIdentEq("f1")) {
      freg.regNum = 1;
    } else if (ConsumeIdentEq("f2")) {
      freg.regNum = 2;
    } else if (ConsumeIdentEq("f3")) {
      freg.regNum = 3;
    } else {
      FailT("unexpected flag register number");
    }

    if (LookingAtSeq(DOT, INTLIT10)) {
      // e.g. f0.1
      //        ^
      // protects predication's use in short predication code
      // (f0.any2h)
      Skip();
      auto srLoc = NextLoc();
      ConsumeIntLitOrFail(freg.subRegNum, "expected flag subregister");
      if (freg.subRegNum > 1) {
        ErrorAtT(srLoc, "flag sub-register out of bounds");
      }
    } else {
      // e.g. f0.any2h (treat as f0.0.any2h)
      freg.subRegNum = 0;
    }
  }

  bool ConsumeDstType(Type &dty) { return ConsumeIdentOneOf(DST_TYPES, dty); }

  bool ConsumeSrcType(Type &sty) { return ConsumeIdentOneOf(SRC_TYPES, sty); }

  // See LookingAtLabelDef for definition of a label
  bool ConsumeLabelDef(std::string &label) {
    if (LookingAtLabelDef()) {
      label = GetTokenAsString();
      (void)Skip(2);
      return true;
    }
    return false;
  }

  // Label = IDENT ':' (not followed by identifier)
  //   mov:ud (16) -- not a label
  //
  //   mov:       -- label
  //      ud (16) ...
  //   mov:       -- label
  //     (f0) ud (16) ...
  //
  //   lbl1:      -- label
  //   lbl2:
  //
  // For now if we get IDENT COLON IDENT, we resolve by:
  //  * if on same line, then it's an instruction
  //  * otherwise it's a label
  bool LookingAtLabelDef() {
    if (LookingAtSeq(IDENT, COLON)) {
      // ensure not a uniform type
      //   mov:ud (..)
      const Token &t2 = Next(2);
      if (t2.lexeme != IDENT || Next(0).loc.line != t2.loc.line) {
        return true;
      }
    }
    return false;
  }

  //
  // DPAS-specific parsing functions (XE+)
  //
  // common function for non-standard operands such as
  //   r13:d (type) or r13:u1 (OperandPrecision)
  // Both dst & src uses this function. 'isDst' tells if
  // it is a dst or src.
  void ParseDpasOp(int opIx, bool isDst) {
    const Loc regStart = NextLoc(0);
    if (!isDst) {
      m_srcLocs[opIx] = regStart;
    }

    if (isDst) {
      // ParseSatOpt increments m_offset.
      if (ParseSatOpt()) {
        m_builder.InstDstOpSaturate();
      }
    }

    // make sure indirect reg addressing is not allowed.
    if (LookingAtIdentEq("r")) {
      FailT("Indirect register addressing not allowed");
    }

    // Match grf such as r10
    const RegInfo *ri = nullptr;
    int regNum = 0;
    if (!ConsumeReg(ri, regNum)) {
      if (isDst)
        FailT("invalid dst");
      else
        FailT("invalid src", opIx);
    }
    if (ri->regName != RegName::GRF_R) {
      // under bdpas, src0, src3 and src4 can be null (meaning +0)
      if (m_opSpec->is(Op::BDPAS)) {
        if ((opIx != 0 && opIx != 3 && opIx != 4) || ri->regName != RegName::ARF_NULL)
          FailT("src", opIx, ": invalid register",
              (opIx == 0 || opIx == 3 || opIx == 4) ?
              " (must be GRF or null)" : " (must be GRF)");
      } else
      // dpas must be GRF except src0, which can be null (meaning +0)
      if (opIx != 0 || ri->regName != RegName::ARF_NULL)
        FailT("src", opIx, ": invalid register",
              opIx == 0 ? " (must be GRF or null)" : " (must be GRF)");
    } else if (!ri->isRegNumberValid(regNum)) {
      FailT("src", opIx, "register number too large", " (", ri->syntax,
            " only has ", ri->numRegs, " on this platform)");
    }

    Loc subRegLoc = NextLoc();
    int subregNum = 0;
    // parse subreg for src2
    //
    // all operands except src2 have an implicit subreg 0.  No explicit
    // subreg is allowed.  For src2, its subreg can be either implicit
    // or explicit.  If it is explicit, the subreg must be half-grf
    // aligned.
    if (LookingAt(DOT)) {
      Skip();
      ConsumeIntLitOrFail(subregNum, "expected subregister");
    }

    // check for reg region, which is not forbidden.
    if (LookingAt(LANGLE)) {
      FailT("instruction does not support regioning");
    }

    // match :<Type> or :<SubBytePrecision>
    Type ty = TryParseOpType(SRC_TYPES);
    if (ty == Type::INVALID) {
      FailT("invalid type");
    }
    if (opIx == 1 && subregNum != 0) {
      // dpas  src1 subreg must be 0
      WarningAtT(subRegLoc, "src1 subregister must be GRF aligned for this op");
    }


    // TODO: we could ensure src2 is half-grf aligned
    // int byteOff = SubRegToBytesOffset(subregNum, RegName::GRF_R, ty);

    RegRef reg(regNum, subregNum);
    if (isDst) {
      m_builder.InstDpasDstOp(regStart, ri->regName, reg, ty);
    } else {
      m_builder.InstDpasSrcOp(opIx, regStart, ri->regName, reg, ty);
    }
  }

  void ParseSdpasMeta() {
    const Loc regStart = NextLoc(0);

    // Match grf such as r10
    const RegInfo *ri = nullptr;
    int regNum = 0;
    if (!ConsumeReg(ri, regNum)) {
      FailT("invalid meta operand");
    }
    if (ri && !ri->isRegNumberValid(regNum)) {
      FailT("invalid register number: ", regNum, " (", ri->syntax, " only has ",
            ri->numRegs, " registers on this platform)");
    }

    int subregNum = 0;
    // match subreg such as .4 (for src2 only).
    //
    // all operands except src2 has an implicit subreg 0. No explicit
    // subreg is allowed.  For src2, its subreg can be either implicit
    // or explicit. If it is explicit, the subreg must be either 0 or 4
    // as src2 is half-grf aligned.
    if (LookingAt(DOT)) {
      Skip();
      ConsumeIntLitOrFail(subregNum, "expected subregister");
    }
    RegRef regRef(regNum, subregNum);
    IGA_ASSERT(ri != nullptr, "nullptr RegInfo");
    m_builder.InstSdpasSrcMeta(regStart, ri->regName, regRef);
  }
};     // class KernelParser

///////////////////////////////////////////////////////////////////////////////
struct ParsedPayloadInfo {
  Loc regLoc = Loc::INVALID;
  int regNum = -1;
  RegName regName = RegName::INVALID;
  Loc regLenLoc = Loc::INVALID;
  int regLen = 0;
  bool regLenWasImplicit = false;
};
struct ParsedAddrOperand {
  Loc surfArgLoc = Loc::INVALID;
  SendDesc surfArg; // won't be set for flat
                    //
  Loc scaleLoc = Loc::INVALID;
  int scale = 1;
  //
  ParsedPayloadInfo payload;
  //
  Loc immOffsetLoc = Loc::INVALID;
  int immOffset = 0;
};

//
// parses a subset of LSC ops
//  EXAMPLES:
//    load.slm                (32|M0)   r10:2:d32    [r20:2]:a32
//    load_strided.ugm.ca.ca  (32|M0)   r10:4:d32x2  [r20:4]:a64
//
//    store.slm       (32|M0)  [r20:2]:a32   r10:2:d32
//    store_quad.slm  (32|M0)  [r20:4]:a64   r10:4:d32.xw
//
//    atomic_fadd.ugm.uc.uc  (32|M0)   r10:2:d32  [r20:4]:a64  null
//
void KernelParser::ParseLdStOpControls(Loc mneLoc,
                                       const SendOpDefinition &opInfo,
                                       VectorMessageArgs &vma) {
  auto tryParseAddrSize = [&](std::string ctrl) {
    if (ctrl == "a16") {
      vma.addrSize = 16;
    } else if (ctrl == "a32") {
      vma.addrSize = 32;
    } else if (ctrl == "a64") {
      vma.addrSize = 64;
    } else {
      return false;
    }
    return true;
  };
  //
  auto tryParseDataType = [&](Loc dtLoc, const std::string& dt) {
    if (dt.size() < 2 || dt[0] != 'd' || !isdigit(dt[1])) {
      return false;
    }
    //
    auto errorAtOffset = [&](size_t off, std::string msg) {
      dtLoc.col += (uint32_t)off;
      dtLoc.offset += (PC)off;
      FailS(dtLoc, msg);
    };
    size_t ix = 1;
    auto skip = [&](size_t i) { ix = std::max<size_t>(i + ix, dt.size()); };
    auto lookingAt = [&](const char *str) { return dt.find(str, ix) == ix; };
    auto consumeIf = [&](const char *str) {
      if (lookingAt(str)) {
        skip(iga::stringLength(str));
        return true;
      }
      return false;
    };
    //////////////////////////////////////////////////////////
    // data size
    //    e.g. d32... or d8u32...
    int dataSize = 0;
    for (; ix < dt.size() && isdigit(dt[ix]); ix++) {
      dataSize = 10 * dataSize + dt[ix] - '0';
    }
    switch (dataSize) {
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
      break;
    default:
      errorAtOffset(0, "invalid data type size");
    }
    // could be d8u32
    vma.dataSizeExpandHigh = false;
    vma.dataSizeReg = vma.dataSizeMem = dataSize;
    if (consumeIf("u32h")) {
      vma.dataSizeReg = 32;
      vma.dataSizeExpandHigh = true;
    } else if (consumeIf("u32")) {
      vma.dataSizeReg = 32;
    }

    //////////////////////////////////////////////////////////
    // vector size
    //  ...x4 or ...x64t or ....xyw (for quad)
    if (opInfo.hasChMask()) {
      if (ix != dt.size())
        errorAtOffset(ix, "unexpected data type");
      // e.g. d32.xz
      //          ^
      if (LookingAtSeq(DOT, IDENT)) {
        Skip();
        auto cmask = ConsumeIdentOrFail("expected op control symbols");
        for (size_t i = 0; i < cmask.size(); i++) {
          auto setElem = [&](int off) {
            if (vma.dataComponentMask & (1 << off)) {
              errorAtOffset(ix + i, "duplicate component mask element");
            }
            vma.dataComponentMask |= (1 << off);
          };
          if (cmask[i] == 'x') {
            setElem(0);
          } else if (cmask[i] == 'y') {
            setElem(1);
          } else if (cmask[i] == 'z') {
            setElem(2);
          } else if (cmask[i] == 'w') {
            setElem(3);
          } else {
            errorAtOffset(ix + i, "invalid component mask symbol");
          }
        }
      } else {
        FailS(NextLoc(), "expected component mask");
      }
    } else {
      // regular vector type
      // e.g. d8, d8u32 or d32x16t
      auto vix = ix;
      int vecSize = 1;
      if (ix < dt.size() - 1 && dt[ix] == 'x' && isdigit(dt[ix + 1])) {
        ix++;
        vecSize = 0;
        for (; ix < dt.size() && isdigit(dt[ix]); ix++)
          vecSize = 10 * vecSize + dt[ix] - '0';
        switch (vecSize) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 8:
        case 16:
        case 32:
        case 64:
          break;
        default:
          errorAtOffset(vix, "invalid vector size");
          break;
        }
      }
      vma.dataVectorSize = (short)vecSize;
      vma.dataTranspose = false;
      //
      // maybe a 't' following
      for (; ix < dt.size(); ix++) {
        if (dt[ix] == 't') {
          if (vma.dataTranspose)
            errorAtOffset(ix, "transpose already set");
          vma.dataTranspose = true;
        } else if (dt[ix] == 'v') {
          if (vma.dataVnni) {
            errorAtOffset(ix, "vnni already set");
          } else if (vma.op != SendOp::LOAD_BLOCK2D) {
            errorAtOffset(ix, "vnni only permitted on load_block2d");
          } else {
            vma.dataVnni = true;
          }
        } else {
          errorAtOffset(ix, "malformed data type suffix");
        }
      } // data type suffix
    }
    //
    return true;
  };
  //
  auto tryCachingSymbol = [](const std::string& cc, CacheOpt &co) {
    if (cc == "df") {
      co = CacheOpt::DEFAULT;
    } else if (cc == "ri") {
      co = CacheOpt::READINVALIDATE;
    } else if (cc == "ca") {
      co = CacheOpt::CACHED;
    } else if (cc == "uc") {
      co = CacheOpt::UNCACHED;
    } else if (cc == "st") {
      co = CacheOpt::STREAMING;
    } else if (cc == "wb") {
      co = CacheOpt::WRITEBACK;
    } else if (cc == "wt") {
      co = CacheOpt::WRITETHROUGH;
    } else if (cc == "cc") {
      co = CacheOpt::CONSTCACHED;
    } else {
      return false;
    }
    return true;
  };
  //
  Loc cacheControlLoc = mneLoc;
  vma.cachingL1 = CacheOpt::DEFAULT;
  vma.cachingL3 = CacheOpt::DEFAULT;
  //
  bool cachingSet = false;
  bool isAddrSizeSet = false;
  bool isDataSizeSet = false;
  //
  while (Consume(DOT)) {
    Loc ctrlLoc = NextLoc();
    auto ctrl = ConsumeIdentOrFail("expected op control symbol");
    if (tryParseAddrSize(ctrl)) {
      if (isAddrSizeSet) {
        FailAtT(ctrlLoc, "duplciate address size");
      }
      isAddrSizeSet = true;
    } else if (tryCachingSymbol(ctrl, vma.cachingL1)) {
      if (cachingSet) {
        FailAtT(ctrlLoc, "duplicate caching options");
      }
      cachingSet = true;
      cacheControlLoc = ctrlLoc;
      if (!LookingAtSeq(DOT, IDENT)) {
        FailT("expected .");
      }
      Skip();
      auto l3sym = GetTokenAsString();
      if (!tryCachingSymbol(l3sym, vma.cachingL3)) {
        FailT("expected caching option");
      }
      Skip();
    } else if (tryParseDataType(ctrlLoc, ctrl)) {
      if (isDataSizeSet)
        FailS(ctrlLoc, "data size already set");
      isDataSizeSet = true;
    } else {
      FailT("invalid ld/st option");
    }
  }
  if (!isAddrSizeSet) {
    FailAtT(mneLoc, "address size not set");
  }
  if (!isDataSizeSet) {
    FailAtT(mneLoc, "data size not set");
  }
  bool isDft =
      vma.cachingL1 != CacheOpt::DEFAULT && vma.cachingL3 != CacheOpt::DEFAULT;
  if (isDft && vma.sfid == SFID::SLM) {
    FailAtT(cacheControlLoc, "SLM forbids cache control options");
  }
}

bool KernelParser::ParseLdStInst() {
  if (!LookingAtSeq(IDENT, DOT, IDENT)) {
    return false;
  }

  VectorMessageArgs vma; // vma {}; crashes VS2019 without dft constructor!
  //
  const auto mneLoc = NextLoc();
  auto mne = GetTokenAsString();
  const SendOpDefinition &opInfo = lookupSendOp(mne.c_str());
  if (!opInfo.isValid())
    return false; // doesn't match a known op
  vma.op = opInfo.op;
  //
  const auto sfidSym = GetTokenAsString(Next(2));
  vma.sfid = FromSyntax<SFID>(sfidSym);
  if (vma.sfid == SFID::INVALID) {
    // better error if we they forget the sfid: e.g. load.d32... instead of load.ugm
    // otherwise sendOpSupportsSyntax() will say the op "isn't supported"
    // the user thinks "BS that "load" isn't supported
    FailS(NextLoc(2), "invalid SFID");
  }
  if (!sendOpSupportsSyntax(platform(), vma.op, vma.sfid)) {
    FailS(NextLoc(), "op not yet supported for ld/st parse");
  }
  if (platform() >= Platform::XE3)
    FailAtT(mneLoc, "load/store syntax not enabled yet for this platform");

  // IGA op
  m_opSpec =
      &m_model.lookupOpSpec(platform() < Platform::XE ? Op::SENDS : Op::SEND);
  if (!m_opSpec->isValid()) {
    FailS(NextLoc(), "INTERNAL ERROR: unable to lookup iga::Op");
  }
  m_builder.InstOp(m_opSpec);
  m_builder.InstSubfunction(vma.sfid);
  //
  Skip(3);
  //
  ParseLdStOpControls(mneLoc, opInfo, vma);
  //
  ChannelOffset chOff = ChannelOffset::M0; // sets m_execSize
  const auto execSizeLoc = NextLoc();
  ParseExecInfo(m_defaultExecutionSize, m_execSize, chOff);
  vma.execSize = static_cast<int>(m_execSize);
  //
  ///////////////////////////////////////////////////////////////////////////
  vma.addrType = AddrType::FLAT;
  //
  // r13 or null
  auto parseReg = [&](ParsedPayloadInfo &ppi) {
    ppi.regLoc = NextLoc();
    const RegInfo *regInfo;
    int regNum;
    if (!ConsumeReg(regInfo, regNum)) {
      FailT("expected register");
      return;
    } else if (regInfo->regName != RegName::GRF_R &&
               regInfo->regName != RegName::ARF_NULL) {
      FailAtT(ppi.regLoc, "register must be GRF or null");
      return;
    }
    ppi.regName = regInfo->regName;
    ppi.regNum = (uint16_t)regNum;
  };

  // r13:0, r13 (meaning r13:1)
  // or null:0, or null meaning (null:0)
  auto parsePayload = [&](ParsedPayloadInfo &ppi) {
    auto regLoc = NextLoc();
    parseReg(ppi);
    if (Consume(COLON)) {
      auto tk = Next();
      ppi.regLenLoc = tk.loc;
      ImmVal val;
      if (!TryParseIntConstExprPrimary(val, "payload suffix expression")) {
        FailAtT(tk.loc, "expected integral payload suffix expression");
      }
      ensureIntegral(tk, val);
      if (val.s64 < 0 || val.s64 > 0x1F) {
        FailAtT(tk.loc, "payload size overflow");
      }
      ppi.regLen = (int)val.s64;
    } else {
      // implicitly 0 or 1 depending on register
      ppi.regLen = ppi.regName == RegName::ARF_NULL ? 0 : 1;
      ppi.regLenLoc = regLoc.endOfToken();
      ppi.regLenWasImplicit = true;
    }
  };

  //   the formatter deduces the length just to be nice to the reader
  auto parsePayloadDst = [&](ParsedPayloadInfo &ppi) {
    parsePayload(ppi);
    auto rgn = m_opSpec->hasImplicitDstRegion(m_builder.isMacroOp())
                   ? m_opSpec->implicitDstRegion(m_builder.isMacroOp()).getHz()
                   : Region::Horz::HZ_1;
    auto ty = SendOperandDefaultType(-1);
    m_builder.InstDstOpRegDirect(ppi.regLoc, ppi.regName, ppi.regNum, rgn, ty);
  };

  auto parsePayloadSrc1 = [&](ParsedPayloadInfo &ppi) {
    parsePayload(ppi);
    m_sendSrcLens[1] = ppi.regLen;
    m_sendSrcLenLocs[1] = ppi.regLenLoc;
    //
    const auto rgn = defaultSendOperandRegion(ppi.regName, 1);
    const auto ty = SendOperandDefaultType(1);
    m_builder.InstSrcOpRegDirect(1, ppi.regLoc, ppi.regName, (int)ppi.regNum,
                                 rgn, ty);
  };

  auto parseA0RegOrImm = [&]() {
    SendDesc surf;
    ConsumeOrFail(LBRACK, "expected [");
    RegName regName = RegName::INVALID;
    if (ParseAddrRegRefOpt(surf.reg, regName)) {
      IGA_ASSERT(regName == RegName::ARF_A, "Indirect send Desc must be ARF_A");
      // surface is a register
      surf.type = SendDesc::Kind::REG32A;
      if (surf.reg.subRegNum & 1) {
        FailT("a0 subregister must be even (values are word aligned)");
      }
    } else {
      // surface is a constant integral expression
      auto loc = NextLoc();
      surf.type = SendDesc::Kind::IMM;
      ImmVal v;
      if (!TryParseIntConstExpr(v, "extended descriptor")) {
        FailT("expected surface state offset");
      }
      if (v.s64 >= 0x03FFFFFF) { // 26b
        FailAtT(loc, "immediate surface state offset is out of bounds");
      }
      surf.imm = (uint32_t)v.s64;
    }
    ConsumeOrFail(RBRACK, "expected ]");
    return surf;
  };

  Loc exDescLoc = mneLoc;
  auto parseAddrOperand = [&](ParsedAddrOperand &addr) {
    addr.surfArgLoc = exDescLoc = NextLoc();
    if (ConsumeIdentEq("bti")) {
      vma.addrType = AddrType::BTI;
      vma.addrSurface = parseA0RegOrImm();
    } else if (ConsumeIdentEq("bss")) {
      vma.addrType = AddrType::BSS;
      vma.addrSurface = parseA0RegOrImm();
    } else if (ConsumeIdentEq("ss")) {
      vma.addrType = AddrType::SS;
      vma.addrSurface = parseA0RegOrImm();
    } else {
      // the "flat" token is optional
      // (if addrtype is absent, we assume flat)
      if (!ConsumeIdentEq("flat") && !LookingAt(LBRACK)) {
        FailT("expected address type or [");
      }
      vma.addrType = AddrType::FLAT;
      vma.addrSurface = 0x0;
    }
    //
    Consume(LBRACK);
    //
    ImmVal v;
    auto scaleTk = Next();
    if (!LookingAt(IDENT) && TryParseIntConstExprPrimary(v)) {
      // LookingAt(..) to avoid failure as r13 parses as IDENT
      // FIXME: parser needs better context to work around this
      // deficiency
      ensureIntegral(scaleTk, v);
      int scale = (int)v.s64;
      ConsumeOrFail(MUL, "expected *");
      //
      // sanity check the size here so we can give good error
      // location info
      int vlen = vma.elementsPerAddress();
      int bytesPerElem = vma.dataSizeMem * vlen / 8;
      if (bytesPerElem > 32) {
        FailS(scaleTk.loc, "scaling factor is too large");
      } else if (scale == bytesPerElem) {
        vma.addrScale = bytesPerElem;
      } else if (scale == 2 * bytesPerElem) {
        vma.addrScale = 2 * bytesPerElem;
      } else if (scale == 4 * bytesPerElem) {
        vma.addrScale = 4 * bytesPerElem;
      } else {
        FailAtT(scaleTk.loc, "invalid scaling factor (must be ",
                1 * bytesPerElem, ", ", 2 * bytesPerElem, ", or ",
                4 * bytesPerElem, ")");
      }
    }
       //
    parsePayload(addr.payload);
    m_sendSrcLens[0] = addr.payload.regLen;
    m_sendSrcLenLocs[0] = addr.payload.regLenLoc;
    //
    vma.addrOffset = 0x0;
    auto addrOffLoc = NextLoc();
    if (LookingAt(ADD) || LookingAt(SUB)) {
      auto immOffLoc = NextLoc();
      auto rangeCheck = [&](int bits, const char *which) {
        if (v.s64 > (1 << (bits - 1)) - 1 || v.s64 < -(1 << (bits - 1))) {
          FailAtT(immOffLoc, "immediate offset exceeds ", bits, "b for ",
                  which);
        }
      };

      exDescLoc = addrOffLoc;
      if (vma.op == SendOp::LOAD_BLOCK2D || vma.op == SendOp::STORE_BLOCK2D) {
        // special handling for block 2d
        bool isNeg = LookingAt(SUB);
        Skip(1);
        ConsumeOrFail(LPAREN, "expected (");
        if (!TryParseIntConstExpr(v, "block2d x-offset")) {
          FailT("syntax error in block2d x-offset");
        }
        if (isNeg) {
          v.s64 = -v.s64;
        }
        rangeCheck(10, "block2d x-offset");
        vma.addrOffsetX = (int)v.s64;
        //
        ConsumeOrFail(COMMA, "expected ,");
        if (!TryParseIntConstExpr(v, "block2d y-offset")) {
          FailT("syntax error in block2d y-offset");
        }
        if (isNeg) {
          v.s64 = -v.s64;
        }
        rangeCheck(10, "block2d y-offset");
        vma.addrOffsetY = (int)v.s64;
        ConsumeOrFail(RPAREN, "expected )");
      } else if (TryParseIntConstExprAddChain(v, "address immediate offset")) {
        if (vma.addrType == AddrType::FLAT) {
          rangeCheck(20, "flat addresses");
        } else if (vma.addrType == AddrType::BSS ||
                   vma.addrType == AddrType::SS) {
          if (vma.sfid == SFID::UGM) {
            rangeCheck(17, "ugm bss/ss addresses");
          } else {
            rangeCheck(16, "non-ugm bss/ss addresses");
          }
        }
        if (v.s64 % 4 != 0)
          FailAtT(immOffLoc, "immediate offset must be 32b aligned");
        vma.addrOffset = (int)v.s64;
      } else {
        FailAtT(immOffLoc, "expected immediate offset expression");
      }
    }
    //
    Consume(RBRACK);
    //
    const auto rgn = defaultSendOperandRegion(addr.payload.regName, 0);
    const auto ty = SendOperandDefaultType(0);
    m_builder.InstSrcOpRegDirect(0, addr.surfArgLoc, addr.payload.regName,
                                 (int)addr.payload.regNum, rgn, ty);
  };

  auto setOpToNull = [&](int opIx, ParsedPayloadInfo &ppi, Loc loc) {
    ppi.regLen = 0;
    ppi.regLoc = ppi.regLenLoc = loc;
    ppi.regName = RegName::ARF_NULL;
    ppi.regNum = 0;
    //
    const auto rgn = defaultSendOperandRegion(ppi.regName, opIx);
    const auto ty = SendOperandDefaultType(opIx);
    if (opIx < 0) {
      m_builder.InstDstOpRegDirect(ppi.regLoc, ppi.regName, ppi.regNum,
                                   rgn.getHz(), ty);
    } else {
      m_builder.InstSrcOpRegDirect(opIx, ppi.regLoc, ppi.regName, ppi.regNum,
                                   rgn, ty);
      m_sendSrcLens[opIx] = 0;
      m_sendSrcLenLocs[opIx] = loc;
    }
  };

  ///////////////////////////////////////////////////////////////////////////
  // actual parsing
  ParsedPayloadInfo dstData;
  ParsedAddrOperand src0Addr;
  ParsedPayloadInfo src1Data;
  if (vma.isLoad()) {
    parsePayloadDst(dstData);
    parseAddrOperand(src0Addr);
    setOpToNull(1, src1Data, mneLoc);
  } else if (vma.isStore()) {
    setOpToNull(-1, dstData, mneLoc); // build a null operand
    parseAddrOperand(src0Addr);
    parsePayloadSrc1(src1Data);
  } else if (vma.isAtomic()) {
    parsePayloadDst(dstData);
    parseAddrOperand(src0Addr);
    parsePayloadSrc1(src1Data);
  } else {
    FailAtT(mneLoc, "unsupported operation for load/store syntax");
  }

  const int GRF_BYTES = platform() >= Platform::XE_HPC ? 64 : 32;
  const int DEFAULT_SIMD = GRF_BYTES / 2;

  int expectedDataRegs = -1;
  int expectedAddrRegs = -1;
  if (vma.op == SendOp::LOAD_BLOCK2D || vma.op == SendOp::STORE_BLOCK2D) {
    expectedAddrRegs = 1;
    // expectedDataRegs = unknown (function of the payload)
  } else if (vma.dataTranspose) {
    // a block operation with a single register payload
    // technically execSize on transpose will be 1, but the
    // logical extension for higher SIMD's exists too;
    // for now we reject it
    if (vma.dataTranspose && vma.execSize != 1) {
      FailAtT(execSizeLoc, "transpose messages must be SIMD1");
    }
    int totalBitsPerSimd = vma.dataSizeReg * vma.elementsPerAddress();
    expectedDataRegs =
        vma.execSize * std::max<int>(totalBitsPerSimd / 8 / GRF_BYTES, 1);
    expectedAddrRegs = 1;
  } else {
    // e.g. SIMD4 rounds up to SIMD8 if messages are SIMD16 by default
    int execElems = std::max<int>(DEFAULT_SIMD / 2, vma.execSize);
    int grfsPerComponent =
        std::max<int>(vma.dataSizeReg * execElems / 8 / GRF_BYTES, 1);
    expectedDataRegs = grfsPerComponent * vma.elementsPerAddress();
    if (opInfo.isAddressScalar()) {
      expectedAddrRegs = 1;
    } else {
      expectedAddrRegs =
          std::max<int>(execElems * vma.addrSize / 8 / GRF_BYTES, 1);
    }
  }
  ///////////////////////////////////////////////////////////////////////////
  // for almost all messages we can deduce the number of address registers
  // needed by the message; one exception is in typed, which uses the address
  // register count to determine which of U, V, R, and LOD are included
  bool checkAddrPayloadSize = true;
  bool hasUvrl =
      vma.sfid == SFID::TGM && (vma.op == SendOp::LOAD_QUAD ||
                                vma.op == SendOp::STORE_QUAD ||
                                vma.op == SendOp::LOAD_QUAD_MSRT ||
                                vma.op == SendOp::STORE_QUAD_MSRT ||
                                vma.isAtomic());
  hasUvrl |= vma.sfid == SFID::TGM && vma.op == SendOp::STORE_UNCOMPRESSED_QUAD;
  bool addrLenIsCorrect = src0Addr.payload.regLen == expectedAddrRegs;
  addrLenIsCorrect |=
      hasUvrl && (src0Addr.payload.regLen != 2 * expectedAddrRegs ||
                  src0Addr.payload.regLen != 3 * expectedAddrRegs ||
                  src0Addr.payload.regLen != 4 * expectedAddrRegs);
  if (checkAddrPayloadSize && !addrLenIsCorrect) {
    WarningAtT(src0Addr.payload.regLoc, "Src0.Length: should be ",
               expectedAddrRegs);
    if (src0Addr.payload.regLenWasImplicit) {
      src0Addr.payload.regLen = expectedAddrRegs;
    }
  }
  //
  if (vma.isLoad() || vma.isAtomic()) {
    // if prefetch or atomic with no return, then
    int expectDstRegs =
        dstData.regName == RegName::ARF_NULL ? 0 : expectedDataRegs;
    if (!dstData.regLenWasImplicit && expectedDataRegs >= 0 &&
        dstData.regLen != expectDstRegs) {
      // if given an explicit number of dst regs, ensure it is correct
      WarningAtT(dstData.regLenLoc, "Dst.Length: should be ", expectDstRegs);
    } else if (dstData.regLenWasImplicit) {
      if (dstData.regName != RegName::ARF_NULL) {
        // assign it, but issue a warning
        dstData.regLen = expectDstRegs;
        WarningAtT(dstData.regLenLoc, "Dst.Length: :", expectDstRegs,
                   " should suffix operand");
      } else {
        dstData.regLen = 0;
      }
    }
  }
  if (vma.isStore() || vma.isAtomic()) {
    int expectSrc1Regs =
        src1Data.regName == RegName::ARF_NULL ? 0 : expectedDataRegs;
    if (vma.isAtomic())
      expectSrc1Regs *= opInfo.numAtomicArgs();
    if (!src1Data.regLenWasImplicit && expectSrc1Regs >= 0 &&
        src1Data.regLen != expectSrc1Regs) {
      WarningAtT(src1Data.regLenLoc, "Src1.Length: should be ", expectSrc1Regs);
    } else if (src1Data.regLenWasImplicit &&
               src1Data.regName != RegName::ARF_NULL) {
      bool hasSrc1LenSfx = platform() <= Platform::XE2;
      if (hasSrc1LenSfx && vma.addrSurface.isReg()) {
        // with a0 exdesc must omit Src1.Length
        // if ExBSO is set
        expectSrc1Regs = -1;
      } else {
        WarningAtT(src1Data.regLenLoc, "Src1.Length: :", expectSrc1Regs,
                   " should suffix operand");
      }
      src1Data.regLen = expectSrc1Regs;
    }
  }

  SendDesc exDesc(0x0), desc(0x0);
  uint32_t exImmOffDesc = 0;

  std::string err;
  if (!encodeDescriptors(platform(), vma, exImmOffDesc, exDesc, desc, err)) {
    if (err.empty())
      err = "unknown error translating load/store op";
    FailS(mneLoc, err);
  }

  desc.imm |= ((uint32_t)src0Addr.payload.regLen & 0xF) << 25;
  desc.imm |= ((uint32_t)dstData.regLen & 0x1F) << 20;
  if (platform() < Platform::XE_HP && exDesc.isImm()) {
    // ExDesc[10:6] on <= XE_HP
    exDesc.imm |= (((uint32_t)src1Data.regLen & 0x1F) << 6);
  }

  m_builder.InstSendSrc0Length(src0Addr.payload.regLen);
  m_builder.InstSendSrc1Length(src1Data.regLen);

  m_builder.InstSendDescs(mneLoc, exImmOffDesc, exDesc, mneLoc, desc);

  return true;
} // KernelParser::ParseLscOp

Kernel *iga::ParseGenKernel(const Model &m, const char *inp,
                            iga::ErrorHandler &e, const ParseOpts &popts) {
  Kernel *k = new Kernel(m);

  InstBuilder h(k, e);
  if (popts.swsbEncodeMode != SWSB_ENCODE_MODE::SWSBInvalidMode)
    h.setSWSBEncodingMode(popts.swsbEncodeMode);

  KernelParser p(m, h, inp, e, popts);
  try {
    p.ParseListing();
  } catch (SyntaxError &) {
    // no need to handle it (error handler has already recorded the errors)
    delete k;
    return nullptr;
  }

  auto &insts = h.getInsts();
  auto blockStarts = Block::inferBlocks(e, k->getMemManager(), insts);
  int id = 0;
  for (const auto &bitr : blockStarts) {
    bitr.second->setID(id++);
    k->appendBlock(bitr.second);
  }

#if 0
    std::stringstream ss;
    ss << "PARSED BLOCKS\n\n";
    for (auto b : k->getBlockList()) {
        ss << "BLOCK[" << b->getID() << "] at pc " <<
            b->getPC() << ":\n";
        ss << "  targeted by {";
        int totalTargets = 0;
        for (auto b2 : k->getBlockList()) {
            for (auto i2 : b2->getInstList()) {
                for (unsigned srcIx = 0; srcIx < i2->getSourceCount(); srcIx++) {
                    const iga::Operand &src = i2->getSource(srcIx);
                    if (src.getTargetBlock() == b) {
                        if (totalTargets++ > 0)
                          ss << ", ";
                        ss << "." << i2->getPC();
                        break;
                    }
                }
            }
        }
        ss << "}\n";
        for (auto i : b->getInstList()) {
            ss << "  ." << i->getPC() << " is " <<
                i->getOpSpec().fullMnemonic << "\n";
        }
        ss << "\n";
    }
    std::cout << ss.str();
#endif

  return k;
}

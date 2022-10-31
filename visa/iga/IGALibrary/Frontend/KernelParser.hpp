/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_FRONTEND_KERNELPARSER
#define IGA_FRONTEND_KERNELPARSER

#include "../ErrorHandler.hpp"
#include "../IR/InstBuilder.hpp"
#include "../IR/Kernel.hpp"
#include "../IR/Loc.hpp"
#include "../IR/Types.hpp"
#include "Parser.hpp"

// #include <functional>
#include <map>
#include <string>

namespace iga {
struct ParseOpts {
  // Enables certain IsaAsm-era directives
  //   .default_execution_size(...)
  //   .default_register_type(...)
  bool supportLegacyDirectives = false;
  // emits warnings about deprecated syntax
  bool deprecatedSyntaxWarnings = true;
  SWSB_ENCODE_MODE swsbEncodeMode = SWSB_ENCODE_MODE::SWSBInvalidMode;

  // sets the maximum number of fatal syntax errors allowable
  // before we give up on the parse
  size_t maxSyntaxErrors = 3;

  ParseOpts(const Model &model) { swsbEncodeMode = model.getSWSBEncodeMode(); }
};

// The primary API for parsing GEN kernels
Kernel *ParseGenKernel(const Model &model, const char *inp, ErrorHandler &e,
                       const ParseOpts &popts);

// using SymbolTableFunc =
//     std::function<bool(const std::string &, ImmVal &)>;

struct ExprParseOpts {
  bool allowFloat = true; // (vs int)
  std::map<std::string, ImmVal> symbols;

  ExprParseOpts(bool allowFlts = true) : allowFloat(allowFlts) {}
};

// Generic GEN parser that can:
//   - parse constant expressions and knows it's model
//   - etc...
//
struct GenParser : public Parser {
  const Model &m_model;
  InstBuilder &m_builder;
  const ParseOpts m_opts;

  GenParser(const Model &model, InstBuilder &handler, const std::string &inp,
            ErrorHandler &eh, const ParseOpts &pots);

  Platform platform() const { return m_model.platform; }

  bool LookupReg(const std::string &str, const RegInfo *&regInfo, int &regNum);
  bool PeekReg(const RegInfo *&regInfo, int &regNum);
  bool ConsumeReg(const RegInfo *&regInfo, int &regNum);

  //////////////////////////////////////////////////////////////////////
  // expression parsing
  // &,|
  // <<,>>
  // +,-
  // *,/,%
  // -(unary neg)
  // literals
  //
  // attempts to parse;
  //   * returns false if parsing failed with no progress was made
  bool TryParseConstExpr(ImmVal &v);
  bool TryParseConstExpr(const ExprParseOpts &pos, ImmVal &v);
  //
  // parses something, but requires it evaluate to an int
  bool TryParseIntConstExpr(ImmVal &v, const char *forWhat = nullptr);
  bool TryParseIntConstExpr(const ExprParseOpts &pos, ImmVal &v,
                            const char *forWhat = nullptr);
  //
  // same as above, but starts at additive level
  bool TryParseIntConstExprAdd(ImmVal &v, const char *forWhat = nullptr);
  bool TryParseIntConstExprAdd(const ExprParseOpts &pos, ImmVal &v,
                               const char *forWhat = nullptr);
  // assumes you are here
  //  X + E1 - E2
  //    ^ (starts here with 0)
  // don't want to negate the whole term
  //  X - 1 - 1
  //    ^^^^^^^ == -2
  // if nothing showing then we return 0 and success (true)
  bool TryParseIntConstExprAddChain(const ExprParseOpts &pos, ImmVal &v,
                                    const char *forWhat = nullptr);
  bool TryParseIntConstExprAddChain(ImmVal &v, const char *forWhat = nullptr);
  //
  // same as above, but starts at the primary level
  // so operators require grouping parentheses to start e.g. (1 + 2)
  bool TryParseIntConstExprPrimary(ImmVal &v, const char *forWhat = nullptr);
  bool TryParseIntConstExprPrimary(const ExprParseOpts &pos, ImmVal &v,
                                   const char *forWhat = nullptr);

protected:
  void ensureIntegral(const Token &t, const ImmVal &v);
  void checkNumTypes(const ImmVal &v1, const Token &t, const ImmVal &v2);
  void checkIntTypes(const ImmVal &v1, const Token &t, const ImmVal &v2);

  ImmVal evalBinExpr(const ImmVal &v1, const Token &t, const ImmVal &v2);
  bool parseBitwiseExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseBitwiseANDExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseBitwiseXORExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseBitwiseORExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseShiftExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseAddExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseMulExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parseUnExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);
  bool parsePrimaryExpr(const ExprParseOpts &po, bool consumed, ImmVal &v);

private:
  void initSymbolMaps();
  std::map<std::string, const RegInfo *> m_regmap;
}; // class GenParser
} // namespace iga
#endif

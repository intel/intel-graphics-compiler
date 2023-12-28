/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_FRONTEND_PARSER_HPP
#define IGA_FRONTEND_PARSER_HPP

#include "../ErrorHandler.hpp"
#include "../IR/Loc.hpp"
#include "../Models/Models.hpp"
#include "BufferedLexer.hpp"

#include <cstdarg>
#include <initializer_list>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace iga {
template <typename T>
using IdentMap = std::initializer_list<std::pair<std::string, T>>;
template <typename T>
static inline T Lookup(std::string sym, const IdentMap<T> &M, T orElse) {
  for (const auto &e : M) {
    if (e.first == sym)
      return e.second;
  }
  return orElse;
}

// this type is used to bail out of the parsing algorithm upon syntax error
struct SyntaxError : std::runtime_error {
  const Loc loc;
  std::string message;

  SyntaxError(const struct Loc &l, const std::string &m) throw()
      : std::runtime_error(m), loc(l), message(m) {}
  ~SyntaxError() {}
};

///////////////////////////////////////////////////////////////////////////
// Recursive descent parser.
// The nomaclaure for method names is roughly:
//   Looking****      peeks at the token, doesn't consume
//   Looking**From    peeks relative to the lexer's current offset
//   Consume****      consume next token if some criteria is true
//   Parse******      generally corresponds to a non-terminal or some
//                    complicated lexemes
//
//
class Parser {
protected:
  BufferedLexer m_lexer;
  ErrorHandler &m_errorHandler;

public:
  Parser(const std::string &inp, ErrorHandler &errHandler)
      : m_lexer(inp), m_errorHandler(errHandler) {}

  //////////////////////////////////////////////////////////////////////
  // DEBUGGING
  // void DumpLookaheads(int n = 1) const {m_lexer.DumpLookaheads(n); }
  void ShowCurrentLexicalContext(std::ostream &os) const {
    ShowCurrentLexicalContext(NextLoc(), os);
  }
  void ShowCurrentLexicalContext(const Loc &loc, std::ostream &os) const;

  //////////////////////////////////////////////////////////////////////
  // WARNINGS and ERRORS
  template <typename... Ts> void WarningT(Ts... ts) {
    WarningS(NextLoc(), iga::format(std::move(ts)...));
  }
  template <typename... Ts> void WarningAtT(const Loc &loc, Ts... ts) {
    WarningS(loc, iga::format(std::move(ts)...));
  }
  void WarningS(const Loc &loc, const std::string &msg);

  template <typename... Ts> void ErrorT(Ts... ts) {
    ErrorAtS(NextLoc(), iga::format(std::move(ts)...));
  }
  template <typename... Ts> void ErrorAtT(const Loc &loc, Ts... ts) {
    ErrorAtS(loc, iga::format(std::move(ts)...));
  }
  void ErrorAtS(const Loc &loc, const std::string &smsg);

  template <typename... Ts> void FailT(Ts... ts) {
    FailS(NextLoc(), iga::format(std::move(ts)...));
  }
  template <typename... Ts> void FailAtT(const Loc &loc, Ts... ts) {
    FailS(loc, iga::format(std::move(ts)...));
  }
  void FailS(const Loc &loc, const std::string &msg);
  void FailAfterPrev(const char *msg);

  //////////////////////////////////////////////////////////////////////
  // BASIC and GENERAL FUNCTIONS
  const Token &Next(int i = 0) const { return m_lexer.Next(i); }

  Loc NextLoc(int i = 0) const { return Next(i).loc; }

  uint32_t ExtentToPrevEnd(const Loc &start) const;

  uint32_t ExtentTo(const Loc &start, const Loc &end) const;

  bool EndOfFile() const { return m_lexer.EndOfFile(); }

  bool Skip(int k = 1) { return m_lexer.Skip(k); }

  std::string GetTokenAsString(const Token &token) const;
  std::string GetTokenAsString() const { return GetTokenAsString(Next()); }

  //////////////////////////////////////////////////////////////////////
  // QUERYING (non-destructive lookahead)
  bool LookingAt(Lexeme lxm) const { return LookingAtFrom(0, lxm); }
  bool LookingAtFrom(int k, Lexeme lxm) const;

  bool LookingAtSeq(Lexeme lxm0, Lexeme lxm1) const {
    return LookingAtSeq({lxm0, lxm1});
  }
  bool LookingAtSeq(Lexeme lxm0, Lexeme lxm1, Lexeme lxm2) const {
    return LookingAtSeq({lxm0, lxm1, lxm2});
  }
  bool LookingAtSeq(std::initializer_list<Lexeme> lxms) const;

  bool LookingAtAnyOf(Lexeme lxm0, Lexeme lxm1) const {
    return LookingAtAnyOf({lxm0, lxm1});
  }
  bool LookingAtAnyOf(Lexeme lxm0, Lexeme lxm1, Lexeme lxm2) const {
    return LookingAtAnyOf({lxm0, lxm1, lxm2});
  }
  bool LookingAtAnyOf(std::initializer_list<Lexeme> lxms) const;
  bool LookingAtAnyOfFrom(int i, std::initializer_list<Lexeme> lxms) const;

  bool LookingAtPrefix(const char *pfx) const;

  //////////////////////////////////////////////////////////////////////
  // CONSUMPTION (destructive lookahead)
  bool Consume(Lexeme lxm) { return m_lexer.Consume(lxm); }
  void ConsumeOrFail(Lexeme lxm, const char *msg);
  // same as above, but the error location chosen is the end of the
  // previous token; i.e. the suffix is screwed up
  void ConsumeOrFailAfterPrev(Lexeme lxm, const char *msg);
  bool Consume(Lexeme lxm0, Lexeme lxm1) {
    // first block doesn't require a label
    if (LookingAtSeq(lxm0, lxm1)) {
      return Skip(2);
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  // IDENTIFIER and RAW STRING MANIPULATION
  bool PrefixAtEq(size_t off, const char *pfx) const;

  bool LookingAtIdentEq(const char *eq) const;
  bool LookingAtIdentEq(int k, const char *eq) const;
  bool LookingAtIdentEq(const Token &tk, const char *eq) const;
  bool ConsumeIdentEq(const char *eq);
  std::string ConsumeIdentOrFail(
      const char *what =
          nullptr); // can tell what type of ident optionally; e.g. "op name"

  bool TokenEq(const Token &tk, const char *eq) const;

  template <typename T>
  bool IdentLookupFrom(int k, const IdentMap<T> &map, T &value) const {
    if (!LookingAtFrom(k, IDENT)) {
      return false;
    }
    for (const auto &p : map) {
      if (TokenEq(Next(k), p.first.c_str())) {
        value = p.second;
        return true;
      }
    }
    return false;
  }

  template <typename T>
  void ConsumeIdentOneOfOrFail(const IdentMap<T> &map, T &value,
                               const char *errExpecting,
                               const char *errInvalid) {
    if (!LookingAt(IDENT)) {
      FailT(errExpecting);
    }
    if (!IdentLookupFrom(0, map, value)) {
      FailT(errInvalid);
    }
    Skip();
  }

  template <typename T>
  bool ConsumeIdentOneOf(const IdentMap<T> &map, T &value) {
    if (LookingAt(IDENT) && IdentLookupFrom(0, map, value)) {
      Skip();
      return true;
    }
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////
  // NUMBERS
  //
  template <typename T> bool ConsumeIntLit(T &value) {
    if (LookingAtAnyOf({INTLIT02, INTLIT10, INTLIT16})) {
      ParseIntFrom(NextLoc(), value);
      Skip();
      return true;
    }
    return false;
  }

  template <typename T> void ConsumeIntLitOrFail(T &value, const char *err) {
    if (!ConsumeIntLit(value)) {
      FailT(err);
    }
  }

  // Examples:
  //   3.141
  //    .451
  //   3.1e7
  //   3e9
  //   3e9.5
  void ParseFltFrom(const Loc loc, double &value);

  template <typename T> void ParseIntFrom(const Loc &loc, T &value) {
    ParseIntFrom(loc.offset, loc.extent, value);
  }

  template <typename T> void ParseIntFrom(size_t off, size_t len, T &value) {
    const std::string &src = m_lexer.GetSource();
    value = 0;
    if (len > 2 && src[off] == '0' &&
        (src[off + 1] == 'b' || src[off + 1] == 'B')) {
      for (size_t i = 2; i < len; i++) {
        char chr = src[off + i];
        T next_value = 2 * value + chr - '0';
        if (next_value < value) {
          FailS(-1, "integer literal too large");
        }
        value = next_value;
      }
    } else if (len > 2 && src[off] == '0' &&
               (src[off + 1] == 'x' || src[off + 1] == 'X')) {
      for (size_t i = 2; i < len; i++) {
        char chr = src[off + i];
        char dig = 0;
        if (chr >= '0' && chr <= '9')
          dig = chr - '0';
        else if (chr >= 'A' && chr <= 'F')
          dig = chr - 'A' + 10;
        else if (chr >= 'a' && chr <= 'f')
          dig = chr - 'a' + 10;
        T next_value = 16 * value + dig;
        if (next_value < value) {
          FailS(-1, "integer literal too large");
        }
        value = next_value;
      }
    } else {
      for (size_t i = 0; i < len; i++) {
        char chr = src[off + i];
        T next_value = 10 * value + chr - '0';
        if (next_value < value) {
          FailS(-1, "integer literal too large");
        }
        value = next_value;
      }
    }
  }
}; // Parser
} // namespace iga

#endif // IGA_FRONTEND_PARSER_HPP

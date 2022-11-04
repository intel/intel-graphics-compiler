/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <limits>
#include <list>

#include "../asserts.hpp"
#include "../strings.hpp"
#include "Floats.hpp"
#include "Parser.hpp"

namespace iga {
//////////////////////////////////////////////////////////////////////
// DEBUGGING
//
void Parser::ShowCurrentLexicalContext(const Loc &loc, std::ostream &os) const {
  WriteTokenContext(m_lexer.GetSource(), loc, os);
}

//////////////////////////////////////////////////////////////////////
// WARNINGS and ERRORS
//
void Parser::WarningS(const Loc &loc, const std::string &msg) {
  m_errorHandler.reportWarning(loc, msg);
}
void Parser::ErrorAtS(const Loc &loc, const std::string &msg) {
  m_errorHandler.reportError(loc, msg);
}

void Parser::FailS(const Loc &loc, const std::string &msg) {
  // DumpLookahead();
  throw SyntaxError(loc, msg);
}

void Parser::FailAfterPrev(const char *msg) {
  Token pv = Next(-1);
  if (pv.loc.extent == 0) {
    // previous location is not valid => use the other path
    FailS(NextLoc(), msg);
  } else {
    // step over the token contents manually
    Loc loc = pv.loc.endOfToken();
    FailS(loc, msg);
  }
}

//////////////////////////////////////////////////////////////////////
// BASIC and GENERAL FUNCTIONS
uint32_t Parser::ExtentToPrevEnd(const Loc &start) const {
  return ExtentTo(start, NextLoc(-1));
}

uint32_t Parser::ExtentTo(const Loc &start, const Loc &end) const {
  return end.offset - start.offset + end.extent;
}

std::string Parser::GetTokenAsString(const Token &token) const {
  return m_lexer.GetSource().substr(token.loc.offset, token.loc.extent);
}

//////////////////////////////////////////////////////////////////////
// QUERYING (non-destructive lookahead)
bool Parser::LookingAtFrom(int k, Lexeme lxm) const {
  return m_lexer.LookingAtFrom(k, lxm);
}

bool Parser::LookingAtSeq(std::initializer_list<Lexeme> lxms) const {
  int i = 0;
  for (Lexeme lxm : lxms) {
    if (!LookingAtFrom(i++, lxm)) {
      return false;
    }
  }
  return true;
}

bool Parser::LookingAtAnyOf(std::initializer_list<Lexeme> lxms) const {
  return LookingAtAnyOfFrom(0, lxms);
}

bool Parser::LookingAtAnyOfFrom(int i,
                                std::initializer_list<Lexeme> lxms) const {
  int off = 0;
  for (Lexeme lxm : lxms) {
    if (LookingAtFrom(off + i, lxm)) {
      return true;
    }
  }
  return false;
}
bool Parser::LookingAtPrefix(const char *pfx) const {
  return PrefixAtEq(NextLoc().offset, pfx);
}

//////////////////////////////////////////////////////////////////////
// CONSUMPTION (destructive lookahead)
void Parser::ConsumeOrFail(Lexeme lxm, const char *msg) {
  if (!Consume(lxm)) {
    FailS(NextLoc(), msg);
  }
}

void Parser::ConsumeOrFailAfterPrev(Lexeme lxm, const char *msg) {
  if (!Consume(lxm)) {
    FailAfterPrev(msg);
  }
}

//////////////////////////////////////////////////////////////////////
// IDENTIFIER and RAW STRING MANIPULATION
bool Parser::PrefixAtEq(size_t off, const char *pfx) const {
  size_t slen = stringLength(pfx);
  if (off + slen > m_lexer.GetSource().size())
    return false;
  return strncmp(pfx, &m_lexer.GetSource()[off], slen) == 0;
}
bool Parser::LookingAtIdentEq(const char *eq) const {
  return LookingAtIdentEq(0, eq);
}
bool Parser::LookingAtIdentEq(int k, const char *eq) const {
  const Token &t = Next(k);
  if (t.lexeme != Lexeme::IDENT)
    return false;
  return TokenEq(Next(k), eq);
}
bool Parser::LookingAtIdentEq(const Token &tk, const char *eq) const {
  if (tk.lexeme != Lexeme::IDENT)
    return false;
  return TokenEq(tk, eq);
}

bool Parser::ConsumeIdentEq(const char *eq) {
  if (LookingAtIdentEq(eq)) {
    Skip();
    return true;
  }
  return false;
}
std::string Parser::ConsumeIdentOrFail(const char *what) {
  const Token &t = Next(0);
  if (t.lexeme != Lexeme::IDENT) {
    if (what == nullptr) {
      FailT("expected identifier");
    } else {
      FailT("expected ", what);
    }
  }
  std::string id = GetTokenAsString(t);
  Skip();
  return id;
}
bool Parser::TokenEq(const Token &tk, const char *eq) const {
  if (!eq)
    return false;
  size_t slen = strlen(eq);
  if (slen != tk.loc.extent ||
      tk.loc.offset + slen > m_lexer.GetSource().size())
    return false;
  const char *str = &m_lexer.GetSource()[tk.loc.offset];
  return strncmp(eq, str, slen) == 0;
  //    return strncmp(eq,&m_input[t.loc.offset],slen) == 0;
}

///////////////////////////////////////////////////////////////////////////
// NUMBERS

// Examples:
//   3.141
//    .451
//   3.1e7
//   3e9
//   3e9.5
void Parser::ParseFltFrom(const Loc loc, double &value) {
  auto str = GetTokenAsString();
  double x = 0.0;
  if (!ParseFLTLIT(str, x)) {
    // NOTE: this is an internal error since it indicates an
    // inconsistency between the lexical specification and the parser
    FailS(loc, "INTERNAL ERROR: parsing float literal (busted lexer?)");
  }
  value = x;
}
} // namespace iga

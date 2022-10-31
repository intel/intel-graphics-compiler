/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_LEXEMES_HPP_
#define _IGA_LEXEMES_HPP_

namespace iga {

enum Lexeme {
  LEXICAL_ERROR = 0, // Windows GDI #define's "ERROR"
  NEWLINE,

  // delimiters
  LANGLE, // <
  RANGLE, // >
  LBRACK, // [
  RBRACK, // ]
  LBRACE, // {
  RBRACE, // }
  LPAREN, // (
  RPAREN, // )

  // separators
  DOT,   // .
  COMMA, // ,
  SEMI,  // ;
  COLON, // :

  // source modifiers and saturation
  // NEG (see SUB), // -       negation (src modifier)
  TILDE, // ~       bitwise complement
  // TODO: this is unsafe (abs) + 4 could be an imm. expression if abs bound
  ABS, // (abs)   absolute value
  SAT, // (sat)   saturatuation

  // reserved symbols
  BANG,   // !
  AT,     // @
  DOLLAR, // $
  HASH,   // #
  EQ,     // =

  // operators (reserved for future arithmetic)
  MUL,  // *
  DIV,  // /
  MOD,  // %
  ADD,  // +
  SUB,  // -
  LSH,  // <<
  RSH,  // >>
  AMP,  // &
  CIRC, // ^  (circumflex)
  PIPE, // |

  // variable lexemes
  IDENT,    // e.g. myVar
  INTLIT02, // 0b1101
  INTLIT10, // 13
  INTLIT16, // 0x123
  FLTLIT,   // 1.3, 0.1, 1e9, 1e-4

  // unused currently
  CHRLIT, // character literals
  STRLIT, // string literals

  END_OF_FILE, // special lexeme that indicates end of file
  NUM_LEXEMES
};

static inline const char *LexemeString(const Lexeme &l) {
#define IGA_LEXEME_TOKEN(T)                                                    \
  case Lexeme::T:                                                              \
    return #T
  switch (l) {
    IGA_LEXEME_TOKEN(LEXICAL_ERROR);
    IGA_LEXEME_TOKEN(NEWLINE);

    IGA_LEXEME_TOKEN(LANGLE); // <
    IGA_LEXEME_TOKEN(RANGLE); // >
    IGA_LEXEME_TOKEN(LBRACK); // [
    IGA_LEXEME_TOKEN(RBRACK); // ]
    IGA_LEXEME_TOKEN(LBRACE); // {
    IGA_LEXEME_TOKEN(RBRACE); // }
    IGA_LEXEME_TOKEN(LPAREN); // (
    IGA_LEXEME_TOKEN(RPAREN); // )

    IGA_LEXEME_TOKEN(AMP);   // &
    IGA_LEXEME_TOKEN(PIPE);  // |
    IGA_LEXEME_TOKEN(DOT);   // .
    IGA_LEXEME_TOKEN(COMMA); // ,
    IGA_LEXEME_TOKEN(SEMI);  // ;
    IGA_LEXEME_TOKEN(COLON); // :

    IGA_LEXEME_TOKEN(TILDE); // ~
    IGA_LEXEME_TOKEN(ABS);   // (abs)
    IGA_LEXEME_TOKEN(SAT);   // (sat)

    IGA_LEXEME_TOKEN(BANG); // !
    IGA_LEXEME_TOKEN(AT);   // @
    IGA_LEXEME_TOKEN(HASH); // #
    IGA_LEXEME_TOKEN(EQ);   // =

    IGA_LEXEME_TOKEN(MUL); // *
    IGA_LEXEME_TOKEN(DIV); // /
    IGA_LEXEME_TOKEN(MOD); // %
    IGA_LEXEME_TOKEN(ADD); // +
    IGA_LEXEME_TOKEN(SUB); // -
    IGA_LEXEME_TOKEN(LSH); // <<
    IGA_LEXEME_TOKEN(RSH); // >>

    IGA_LEXEME_TOKEN(IDENT);    // [_a-zA-Z][_a-zA-Z0-9]*
    IGA_LEXEME_TOKEN(INTLIT02); // an integral pattern (base 2)
    IGA_LEXEME_TOKEN(INTLIT10); // an integral pattern (base 10)
    IGA_LEXEME_TOKEN(INTLIT16); // an integral pattern (base 16)
    IGA_LEXEME_TOKEN(FLTLIT);   // a floating point pattern

    // currently unused
    IGA_LEXEME_TOKEN(CHRLIT); // a character literal
    IGA_LEXEME_TOKEN(STRLIT); // a string literal

    IGA_LEXEME_TOKEN(END_OF_FILE);

  default:
    return "?";
    //  default: {
    //    static char buf[16];
    //    sprintf(buf,"%d?",(int)l);
    //    return buf;
    //    }
  }
#undef IGA_LEXEME_TOKEN
}

} // namespace iga
#endif // _LEXEMES_HPP_

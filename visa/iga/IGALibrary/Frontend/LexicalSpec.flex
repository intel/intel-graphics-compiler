/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

%{
/*
 * Contains the lexical specification for Intel Gen Assembly.
 *
 * Build with:
 *   % flex ThisFile.flex
 * First constructed with flex 2.5.39 (you can use Cygwin if you want).
 *         update 3/2018: flex 2.6.4
 * *** It should build without warnings. ***
 *   => It's nice to strip end of line whitespace on the generated files.
 */
#if defined(_MSC_VER)
#pragma warning(default : 4505)
#pragma warning(disable:4701)
#else
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

#include "Lexemes.hpp"

#define YY_DECL iga::Lexeme yylex (yyscan_t yyscanner, unsigned int &inp_off)

/*
 * It seems many versions of flex don't support column info in re-entrant
 * scanners.  This works around the issue.
 */
#define YY_USER_ACTION \
    yyset_column(yyget_column(yyscanner) + (int)yyget_leng(yyscanner), yyscanner);

%}

%option outfile="lex.yy.cpp" header-file="lex.yy.hpp"
%option nounistd
%option reentrant
%option noyywrap
%option yylineno
/* omits isatty */
%option never-interactive

%x SLASH_STAR
%x STRING_DBL
%x STRING_SNG

DEC_DIGIT    [0-9]
DEC_DIGITS   {DEC_DIGIT}+
/* DEC_FRAC     ({DEC_DIGITS}\.{DEC_DIGITS}?)|({DEC_DIGITS}?\.{DEC_DIGITS}) */
HEX_DIGIT    [0-9A-Fa-f]
HEX_DIGITS   {HEX_DIGIT}+
HEX_FRAC     ({HEX_DIGITS}\.{HEX_DIGITS}?)|({HEX_DIGITS}?\.{HEX_DIGITS})

EXP_SUFFIX   [-+]?{DEC_DIGITS}

%%

<SLASH_STAR>"*/"      { inp_off += 2; BEGIN(INITIAL); }
<SLASH_STAR>[^*\n]+   { inp_off += (unsigned int)yyget_leng(yyscanner); } // eat comment in line chunks
<SLASH_STAR>"*"       { inp_off++; } // eat the lone star
<SLASH_STAR>\n        { inp_off++; }

<STRING_DBL>\"        { inp_off++;
                        BEGIN(INITIAL);
                        return iga::Lexeme::STRLIT; }
<STRING_DBL>\\.       { inp_off += 2; }
<STRING_DBL>.         { inp_off++; }

<STRING_SNG>\'        { inp_off++;
                        BEGIN(INITIAL);
                        return iga::Lexeme::CHRLIT; }
<STRING_SNG>\\.       { inp_off += 2; }
<STRING_SNG>.         { inp_off++; }

"/*"                  {inp_off += 2; BEGIN(SLASH_STAR);}
\<                    return iga::Lexeme::LANGLE;
\>                    return iga::Lexeme::RANGLE;
\[                    return iga::Lexeme::LBRACK;
\]                    return iga::Lexeme::RBRACK;
\{                    return iga::Lexeme::LBRACE;
\}                    return iga::Lexeme::RBRACE;
\(                    return iga::Lexeme::LPAREN;
\)                    return iga::Lexeme::RPAREN;

\$                    return iga::Lexeme::DOLLAR;
\.                    return iga::Lexeme::DOT;
\,                    return iga::Lexeme::COMMA;
\;                    return iga::Lexeme::SEMI;
\:                    return iga::Lexeme::COLON;

\~                    return iga::Lexeme::TILDE;
\(abs\)               return iga::Lexeme::ABS;
\(sat\)               return iga::Lexeme::SAT;

\!                    return iga::Lexeme::BANG;
\@                    return iga::Lexeme::AT;
\#                    return iga::Lexeme::HASH;
\=                    return iga::Lexeme::EQ;

\%                    return iga::Lexeme::MOD;
\*                    return iga::Lexeme::MUL;
\/                    return iga::Lexeme::DIV;
\+                    return iga::Lexeme::ADD;
\-                    return iga::Lexeme::SUB;
\<<                   return iga::Lexeme::LSH;
\>>                   return iga::Lexeme::RSH;
\&                    return iga::Lexeme::AMP;
\^                    return iga::Lexeme::CIRC;
\|                    return iga::Lexeme::PIPE;

{DEC_DIGITS}          return iga::Lexeme::INTLIT10; /* 13 */
0[xX]{HEX_DIGITS}     return iga::Lexeme::INTLIT16; /* 0x13 */
0[bB][01]+            return iga::Lexeme::INTLIT02; /* 0b1101 */

{DEC_DIGITS}\.{DEC_DIGITS}                      return iga::Lexeme::FLTLIT; /* 3.14 (cannot have .3 because that screws up (f0.0)) */
{DEC_DIGITS}(\.{DEC_DIGITS})?[eE]{EXP_SUFFIX}   return iga::Lexeme::FLTLIT; /* 3e-9/3.14e9*/
0[xX]({HEX_FRAC}|{HEX_DIGITS})[pP]{EXP_SUFFIX}  return iga::Lexeme::FLTLIT; /* 0x1.2p3/0x.2p3/0x1.p3/ */
[_a-zA-Z][_a-zA-Z0-9]*                          return iga::Lexeme::IDENT;

%{
/*
 * enables identifier such as "128x16"; this pattern requires a non-zero
 * initial character so that 0x13 will be scanned as a hex int
 */
%}
[1-9][0-9]*x[0-9]+     return iga::Lexeme::IDENT;


\n                     return iga::Lexeme::NEWLINE; /* newlines are explicitly represented */
[ \t\r]+               {inp_off += (unsigned int)yyget_leng(yyscanner);} /* whitespace */;
"//"[^\n]*             {inp_off += (unsigned int)yyget_leng(yyscanner);} /* EOL comment ?*/

.                    return iga::Lexeme::LEXICAL_ERROR;
<<EOF>>              return iga::Lexeme::END_OF_FILE;

%%

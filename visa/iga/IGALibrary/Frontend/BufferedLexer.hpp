/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BUFFEREDLEXER_H
#define _BUFFEREDLEXER_H

#include "../asserts.hpp"
#include "../IR/Loc.hpp"
#include "Lexemes.hpp"

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

// #define DUMP_LEXEMES

#define YY_DECL iga::Lexeme yylex (yyscan_t yyscanner, unsigned int &inp_off)
#ifndef YY_NO_UNISTD_H
#define YY_NO_UNISTD_H
#endif
#include "lex.yy.hpp"

YY_DECL;

namespace iga {

struct Token {
    Lexeme lexeme;
    Loc    loc;

    Token() : lexeme(Lexeme::LEXICAL_ERROR) { }
    Token(
        const Lexeme &lxm,
        uint32_t ln,
        uint32_t cl,
        uint32_t off,
        uint32_t len) : lexeme(lxm), loc(ln,cl,off,len)
    {
    }
};

static void WriteTokenContext(
    const std::string &inp,
    const struct Loc &loc,
    std::ostream &os)
{
    if (loc.offset >= (PC)inp.size()) {
        os << "<<EOF>>" << std::endl;
    } else if (loc.line > 0) {
        size_t k = static_cast<size_t>(loc.offset) - loc.col + 1;
        while (k < inp.size() && inp[k] != '\n' && inp[k] != '\r')
            os << inp[k++];
        os << std::endl;
        if (loc.col > 0) {
            for (size_t i = 1; i < loc.col; i++)
                os << ' ';
            if (loc.line == 0) {
                os << '^';
            } else {
                for (size_t i = 0; i < loc.extent; i++) {
                    os << '~';
                }
            }
        }
        os << std::endl;
    }
}

static std::string GetTokenString(
    const Token &token,
    const std::string &inp)
{
    std::stringstream ss;
    ss << token.loc.line << "." << token.loc.col << ": (" <<
        token.loc.offset << "/" << token.loc.extent << "): " <<
        LexemeString(token.lexeme) << std::endl;
    WriteTokenContext(inp, token.loc, ss);
    return ss.str();
}

class BufferedLexer {
    std::vector<Token>  m_tokens;
    size_t              m_offset, m_mark; // token index of the scanner

    const std::string   m_input;

    Token               m_eof;

public:
    BufferedLexer(const std::string &inp)
        : m_offset(0), m_mark(0)
        , m_input(inp)
        , m_eof(Lexeme::END_OF_FILE, 0, 0, 0, 0)
    {
        yyscan_t yy;

        yylex_init(&yy);
        yy_scan_string(inp.c_str(), yy);
        yyset_lineno(1, yy);
        yyset_column(1, yy);

        unsigned int inpOff = 0, bolOff = 0;

        Lexeme lxm;
        while (true) {
            lxm = yylex(yy, inpOff);

            uint32_t lno = (uint32_t)yyget_lineno(yy);
            uint32_t len = (uint32_t)yyget_leng(yy);
            uint32_t col = (uint32_t)yyget_column(yy) - len;
            uint32_t off = (uint32_t)inpOff;
            if (lxm == Lexeme::NEWLINE) {
                // flex increments yylineno and clear's column before this
                // we fix this by backing up the newline for that case
                // and inferring the final column from the beginning of
                // the last line
                lno--;
                col = inpOff - bolOff + 1;
                bolOff = inpOff;
            }
            // const char *str = yyget_text(yy);
            // printf("AT %u.%u(%u:%u:\"%s\"): %s\n",
            //  lno,col,off,len,str,LexemeString(lxm));
            // struct Loc loc(lno,col,off,len);
            // ShowToken(inp,loc,std::cout);

            if (lxm == Lexeme::END_OF_FILE) {
                m_eof = Token(lxm, lno, col, off, len); // update EOF w/ loc
                m_tokens.push_back(m_eof);
                break;
            }
            m_tokens.emplace_back(lxm, lno, col, off, len);
            inpOff += len;
        }

        yylex_destroy(yy);
    }
    const std::string &GetSource() const {return m_input;}

    size_t GetTokenOffset() const {
        return m_offset;
    }
    void SetTokenOffset(size_t off) {
        m_offset = off;
    }
    void Mark() {
        m_mark = m_offset;
    }
    void Reset() {
        SetTokenOffset(m_mark);
    }

    void DumpTokens(std::ostream &out, const std::string &inp) const {
        for (const auto &t : m_tokens) {
            out << "AT" << t.loc.line << "." << t.loc.col <<
            "(" << t.loc.offset << ":" << t.loc.extent  << ": " <<
            LexemeString(t.lexeme) << std::endl;
            WriteTokenContext(inp,t.loc,out);
        }
    }

    void DumpLookaheads(std::ostream &os, int n = 1) const {
        os << "LEXER: Next " << n << " lookaheads are:\n";
        for (int i = 0; i < n; i++) {
            const Token &tk = Next(i);
            os << "  " << GetTokenString(tk, m_input).c_str() << "\n";
            if (tk.lexeme == Lexeme::END_OF_FILE) {
                break;
            }
        }
    }

    bool EndOfFile() const {
        return m_tokens[m_offset].lexeme == Lexeme::END_OF_FILE;
    }

    bool Skip(int i) {
        int k = (int)m_offset + i;
        if (k < 0 || k >= (int)m_tokens.size()) {
            return false;
        }
        m_offset = k;
#ifdef DUMP_LEXEMES
        DumpLookahead(1);
#endif
        return true;
    }
    bool Consume(Lexeme lxm) {
        if (LookingAt(lxm)) {
            (void)Skip(1);
            return true;
        }
        return false;
    }

    bool LookingAt(Lexeme lxm) const {
        return LookingAtFrom(0,lxm);
    }
    bool LookingAtFrom(int i, Lexeme lx) const {
        return Next(i).lexeme == lx;
    }

    const Token &Next(int i) const {
        int k = (int)m_offset + i;
        if (k < 0 || k >= (int)m_tokens.size()) {
            return m_eof;
        } else {
            return m_tokens[k];
        }
    }
}; // class BufferedLexer

} // namespace iga

#endif // _BUFFEREDLEXER_H

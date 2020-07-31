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
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstdarg>
#include <exception>
#include <functional>
#include <limits>
#include <list>
#include "Parser.hpp"

#include "../asserts.hpp"
#include "../strings.hpp"

namespace iga
{
    //////////////////////////////////////////////////////////////////////
    // DEBUGGING
    //
    void Parser::ShowCurrentLexicalContext(
        std::ostream &os, const Loc &loc) const
    {
        WriteTokenContext(m_lexer.GetSource(), loc, os);
    }

    //////////////////////////////////////////////////////////////////////
    // ERRORS and WARNINGS
    //
    void Parser::Fail(const Loc &loc, const std::string &msg) {
        Fail(loc, msg.c_str());
    }
    void Parser::Fail(const Loc &loc, const char *msg) {
        // DumpLookahead();
        throw SyntaxError(loc, msg);
    }
    void Parser::FailF(const char *pat, ...) {
        va_list ap;
        va_start(ap, pat);
        std::string str = formatv(pat, ap);
        va_end(ap);
        Fail(0, str.c_str());
    }
    void Parser::FailF(const Loc &loc, const char *pat, ...) {
        va_list ap;
        va_start(ap, pat);
        std::string str = formatv(pat, ap);
        va_end(ap);
        Fail(loc, str.c_str());
    }
    void Parser::FailAfterPrev(const char *msg) {
        Token pv = Next(-1);
        if (pv.loc.extent == 0) {
            // previous location is not valid => use the other path
            Fail(msg);
        } else {
            // step over the token contents manually
            Loc loc = pv.loc.endOfToken();
            Fail(loc, msg);
        }
    }
    void Parser::Warning(const Loc &loc, const char *msg) {
        m_errorHandler.reportWarning(loc, msg);
    }
    void Parser::Warning(const char *msg) {
        m_errorHandler.reportWarning(NextLoc(), msg);
    }
    void Parser::WarningF(const char *pat, ...) {
        va_list ap;
        va_start(ap, pat);
        std::string str = formatv(pat, ap);
        va_end(ap);
        m_errorHandler.reportWarning(NextLoc(), str);
    }
    void Parser::WarningF(const Loc &loc, const char *pat, ...) {
        va_list ap;
        va_start(ap, pat);
        std::string str = formatv(pat, ap);
        va_end(ap);
        m_errorHandler.reportWarning(loc, str);
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

    bool Parser::LookingAtAnyOfFrom(int i, std::initializer_list<Lexeme> lxms)
        const
    {
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
            Fail(msg);
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
        size_t slen = strlen(pfx);
        if (off + slen > m_lexer.GetSource().size())
            return false;
        return strncmp(pfx,&m_lexer.GetSource()[off],slen) == 0;
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
        return TokenEq(tk,eq);
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
                Fail("expected identifier");
            } else {
                std::stringstream ss;
                ss << "expected " << what;
                Fail(ss.str().c_str());
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
        return strncmp(eq,str,slen) == 0;
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
        // swap this out with something more platform implementation
        // independent (strtod may be slightly different on MS and non-MS
        const char *val_start = m_lexer.GetSource().c_str() + loc.offset;
        char *val_end;
        value = strtod(val_start,&val_end);
        if ((uint32_t)(val_end - val_start) != loc.extent) {
            // TODO: this is an internal error since it indicates an
            // inconsistency between the lexical specification and the parser
            Fail(loc,"INTERNAL ERROR: parsing float literal");
        }
    }

} // namespace iga


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

#ifndef IGA_ERROR_HANDLER_HPP
#define IGA_ERROR_HANDLER_HPP

#include "IR/Loc.hpp"

#include <string>
#include <vector>
#include <stdexcept>

#ifdef _WIN32
#ifndef NORETURN_DECLSPEC
#define NORETURN_DECLSPEC __declspec(noreturn)
#endif
#ifndef NORETURN_ATTRIBUTE
#define NORETURN_ATTRIBUTE
#endif
#else
#ifndef NORETURN_DECLSPEC
#define NORETURN_DECLSPEC
#endif
#ifndef NORETURN_ATTRIBUTE
#define NORETURN_ATTRIBUTE __attribute__((noreturn))
#endif
#endif

namespace iga {

struct Diagnostic {
    struct Loc            at;
    std::string           message;

    Diagnostic(const struct Loc &a, const std::string &m)
        : at(a), message(m)
    {
    }

    Diagnostic(const int pc, const std::string &m)
        : at(0,0,pc,0), message(m)
    {
    }
}; // end class Diagnostic


class FatalError : public std::runtime_error {
public:
    FatalError() : std::runtime_error("") { }
    FatalError(const char *msg) : std::runtime_error(msg) { }
    FatalError(const std::string& msg) : std::runtime_error(msg) { }
};

// Class that specializes error handling features.
class ErrorHandler {
    std::vector<Diagnostic> m_errors;
    std::vector<Diagnostic> m_warnings;
    bool m_fatalError;
public:
    ErrorHandler(){ m_fatalError = false; }
    bool hasDiagnostics() const { return hasErrors() || hasWarnings(); }
    bool hasErrors() const { return !m_errors.empty(); }
    bool hasWarnings() const { return !m_warnings.empty(); }
    bool hasFatalError() const { return m_fatalError; }
    const std::vector<Diagnostic> &getWarnings() const { return m_warnings; }
    const std::vector<Diagnostic> &getErrors() const { return m_errors; }

    // given a program pc (e.g. decoder/encoder)
    void reportWarning(int pc, const std::string &message) {
        m_warnings.emplace_back(pc, message);
    }
    void reportError(int pc, const std::string &message) {
        m_errors.emplace_back(pc, message);
    }

    // given a source location (parser)
    void reportWarning(const Loc &loc, const std::string &message) {
        m_warnings.emplace_back(loc, message);
    }
    void reportError(const Loc &loc, const std::string &message) {
        m_errors.emplace_back(loc, message);
    }

    // hard stops with an exception a calling frame somewhere up the stack
    // must catch the iga::FatalError.   The ErrorHandler will contain this
    // error message.
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
        NORETURN_DECLSPEC
        void throwFatal(int pc, const char *message)
        NORETURN_ATTRIBUTE
#else
        void throwFatal(int pc, const char *message)
#endif
    {
        reportError(pc, message);
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
        throw FatalError(message);
#else
        m_fatalError = true;
#endif
    }


#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
    NORETURN_DECLSPEC
    void throwFatal(const Loc &loc, const char *message)
    NORETURN_ATTRIBUTE
#else
    void throwFatal(const Loc &loc, const char *message)
#endif
    {
        reportError(loc, message);
        m_fatalError = true;
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
        throw FatalError(message);
#endif
    }

#ifdef IGA_DISABLE_ENCODER_EXCEPTIONS
    void throwFatal(const Loc &loc, const std::string &message)
#else
    NORETURN_DECLSPEC
    void throwFatal(const Loc &loc, const std::string &message)
    NORETURN_ATTRIBUTE
#endif
    {
        reportError(loc, message);
        m_fatalError = true;
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
        throw FatalError(message);
#endif
    }
}; // end class ErrorHandler

} // namespace iga::*

#endif // IGA_ERROR_HANDLER_HPP

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
/*
 * Experimental IGA C++ wrapper interface
 */
#ifndef _IGAX_HPP
#define _IGAX_HPP

#include "common/secure_mem.h"
#include "iga.h"
#include "iga_bxml_ops.hpp"

#include <exception>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>

namespace igax {

enum Platform {
      GEN_INVALID   = IGA_GEN_INVALID
    , GEN7          = IGA_GEN7
    , GEN7P5        = IGA_GEN7p5
    , GEN8          = IGA_GEN8
    , GEN8LP        = IGA_GEN8lp
    , GEN9          = IGA_GEN9
    , GEN9LP        = IGA_GEN9lp
    , GEN9P5        = IGA_GEN9p5
    , GEN10         = IGA_GEN10
    , GEN11         = IGA_GEN11
    , GEN12P1       = IGA_GEN12p1
};

struct Diagnostic {
    std::string message; // diagnostic message
    int line, col;       // line and column (text only)
    int off;             // byte offset (text or binary diagnostics)
    int ext;             // length of region (text only?)

    Diagnostic(const char *msg, int ln, int cl, int o, int e)
        : message(msg), line(ln), col(cl), off(o), ext(e) {}

    // emits the error diagnostic to an output stream
    // includes emitLoc, emitContext, and some other info
    void emit(
        std::ostream &os,
        const std::string &source = "",
        const void *bits = nullptr,
        size_t bitsLen = 0) const;
    // emits just the location
    void emitLoc(std::ostream &os) const;
    // emits information about the location (e.g. marked source line)
    void emitContext(std::ostream &os,
        const std::string &source = "",
        const void *bits = nullptr,
        size_t bitsLen = 0) const;

    // converts the error diagnostic to a string
    // (emit() using std::stringstream)
    // No context info is passed to emit
    std::string str() const { return str(""); }
    // Passes source the context
    std::string str(const std::string &source) const;
    // Passes the binary source to the context
    std::string str(const void *bits, size_t bitsLen) const;
};

// Product type grouping an output result output with all the warnings
// returned in its creation.
template <typename T>
struct Result {
    T                           value;
    std::vector<Diagnostic>     warnings;
};
typedef std::vector<unsigned char> Bits;
struct AsmResult : Result<Bits> { };
struct DisResult : Result<std::string> { };

// a context manages memory and state across the module boundary
class Context {
    iga_context_t context;
    Platform platform;

  public:
    Context(const iga_context_options_t &copts);
    Context(igax::Platform p) : Context(static_cast<iga_gen_t>(p)) {}
    Context(iga_gen_t p) : Context(IGA_CONTEXT_OPTIONS_INIT(p)) {}
    ~Context();

    Platform getPlatform() const { return platform; }

    // assembles a string into bits, returns warning if applicable
    // failure will throw an igax::AssembleError of some sort
    // (see subclasses below)
    AsmResult assembleFromString(
        const std::string &text,
        const iga_assemble_options_t &opts = IGA_ASSEMBLE_OPTIONS_INIT());
    // Disassembles a sequence of bits to a string
    DisResult disassembleToString(
        const void *bits,
        const size_t bitsLen,
        const iga_disassemble_options_t &opts = IGA_DISASSEMBLE_OPTIONS_INIT());
};

// parent class for all IGA API errors
// subclasses contain more specific information
// some obscure errors (e.g. out of memory) lack a subclass and
// will be instances of this class, typically you shouldn't handle these
struct Error : std::exception {
    iga_status_t status; // the error code returned
    const char *api; // the C-API that was called

    Error(iga_status_t _status, const char *_api)
        : status(_status), api(_api) {}

    // emits the error to an output stream
    // sub-errors override this to offer more specific information
    virtual void emit(std::ostream &os) const;

    // converts the error to a string message (via emit and std::stringstream)
    std::string str() const;
};

// indicates a failure to assemble (typically a parse error or encoding error)
struct AssembleError : Error {
    std::vector<Diagnostic> errors;
    std::string source;

    AssembleError(
        iga_status_t st,
        const char *api,
        const std::vector<Diagnostic> &errs,
        const std::string &src)
        : Error(st, api), errors(errs), source(src) {}

    // overrides default implementation to emit the diagnostics instead
    virtual void emit(std::ostream &os) const;
};
// thrown when a syntax error(s) are encountered during assembly
struct SyntaxError : AssembleError {
    SyntaxError(
        const char *api,
        const std::vector<Diagnostic> &errs,
        const std::string &src)
            : AssembleError(IGA_PARSE_ERROR, api, errs, src) { }
};
// thrown when assembly encounters an encoding error
// E.g. we cannot compact an instruction
struct EncodeError : AssembleError {
    EncodeError(
        const char *api,
        std::vector<Diagnostic> errs,
        const std::string src)
            : AssembleError(IGA_ENCODE_ERROR, api, errs, src) { }
};


// indicates a failure to disassemble (malformed bits?)
struct DisassembleError : Error {
    std::vector<Diagnostic> errors;
    const void *bits;
    size_t bitsLen;

    DisassembleError(
        iga_status_t st,
        const char *api,
        const std::vector<Diagnostic> &errs,
        const void *_bits,
        size_t _bitsLen)
            : Error(st, api), errors(errs), bits(_bits), bitsLen(_bitsLen) {}

    virtual void emit(std::ostream &os) const;
};
// most specific case that one wants to react to in DisassembleError
// something is wrong with the bits being decoded
struct DecodeError : DisassembleError {
    DecodeError(
        const char *api,
        const std::vector<Diagnostic> &errs,
        const void *_bits,
        size_t _bitsLen)
            : DisassembleError(IGA_DECODE_ERROR, api, errs, _bits, _bitsLen) { }
};


///////////////////////////////////////////////
// IGA OPSPEC API
//
// Lists information about ops for a given platform
class OpSpec {
    Platform         m_platform;
    iga_opspec_t     m_op;
public:
    OpSpec(Platform p, iga_opspec_t op) : m_platform(p), m_op(op) { }
    iga::Op op() const;
    std::string menmonic() const;
    std::string name() const;
    std::string description() const;
    OpSpec parent() const;

    // enumerates all the operations for a given platform
    static std::vector<OpSpec> enumerate(Platform p);
};


///////////////////////////////////////////////////////////////////////////////
/////////////////////////////// IMPLEMENTATION ////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The API user shouldn't have to directly interact with this code.  It might
// prove helpful in debugging, but our hope is that you can treat this as
// an abstraction.

#define IGA_CHECKED_CALL(API, ...)           \
    do {                                     \
        iga_status_t _st = API(__VA_ARGS__); \
        if (_st != IGA_SUCCESS)              \
            throw Error(_st, #API);          \
    } while (0)

static std::vector<Diagnostic> getDiagnostics(const iga_context_t &context, bool errs)
{
    std::vector<Diagnostic> out;
    const iga_diagnostic_t *ds = nullptr;
    uint32_t dsLen             = 0;
    if (errs) {
        IGA_CHECKED_CALL(iga_get_errors, context, &ds, &dsLen);
    } else {
        IGA_CHECKED_CALL(iga_get_warnings, context, &ds, &dsLen);
    }
    for (uint32_t i = 0; i < dsLen; i++) {
        const char *msg;
        IGA_CHECKED_CALL(iga_diagnostic_get_message, ds + i, &msg);
        uint32_t off;
        IGA_CHECKED_CALL(iga_diagnostic_get_offset, ds + i, &off);

        iga_diagnostic_type_t dt;
        IGA_CHECKED_CALL(iga_diagnostic_get_type, ds + i, &dt);
        if (dt == IGA_DIAGNOSTIC_TEXT) {
            uint32_t line, col, ext;
            IGA_CHECKED_CALL(iga_diagnostic_get_text_line, ds + i, &line);
            IGA_CHECKED_CALL(iga_diagnostic_get_text_column, ds + i, &col);
            IGA_CHECKED_CALL(iga_diagnostic_get_text_extent, ds + i, &ext);
            out.emplace_back(msg, line, col, off, ext);
        } else { // binary
            out.emplace_back(msg, 0, 0, off, 0);
        }
    }
    return out;
}
static std::vector<Diagnostic> getWarnings(const iga_context_t &ctx)
{
    return igax::getDiagnostics(ctx, false);
}
static std::vector<Diagnostic> getErrors(const iga_context_t &ctx)
{
    return igax::getDiagnostics(ctx, true);
}

inline Context::Context(const iga_context_options_t &copts)
    : context(nullptr), platform(static_cast<igax::Platform>(copts.gen))
{
    IGA_CHECKED_CALL(iga_create_context, &copts, &context);
}

inline Context::~Context() {
    // ignore the return value since we are in a destructor
    (void)iga_release_context(context);
    context = nullptr;
}

inline AsmResult Context::assembleFromString(
    const std::string &text,
    const iga_assemble_options_t &opts)
{
    AsmResult result;
    void *bits;
    uint32_t bitsLen;

    iga_status_t st =
        iga_assemble(context, &opts, text.c_str(), &bits, &bitsLen);
    if (st != IGA_SUCCESS) {
        if (st == IGA_UNSUPPORTED_PLATFORM) {
            std::vector<Diagnostic> errs;
            throw AssembleError(st, "iga_assemble", errs, text);
        }
        std::vector<Diagnostic> errs = igax::getErrors(context);
        if (st == IGA_PARSE_ERROR) {
            throw SyntaxError("iga_assemble", errs, text);
        } else if (st == IGA_ENCODE_ERROR) {
            throw EncodeError("iga_assemble", errs, text);
        } else {
            throw AssembleError(st, "iga_assemble", errs, text);
        }
    }

    // copy out the warnings
    result.warnings = igax::getWarnings(context);
    // copy out the bits
    if (bitsLen > 0) {
        result.value.resize(bitsLen);
        memcpy_s(result.value.data(), bitsLen, bits, bitsLen);
    }

    return result;
}

inline DisResult Context::disassembleToString(
    const void *bits,
    const size_t bitsLen,
    const iga_disassemble_options_t &opts)
{
    char *text;
    iga_status_t st = iga_disassemble(
        context,
        &opts,
        bits,
        (uint32_t)bitsLen,
        nullptr,
        nullptr,
        &text);
    if (st != IGA_SUCCESS) {
        if (st == IGA_UNSUPPORTED_PLATFORM) {
            std::vector<Diagnostic> errs;
            throw DisassembleError(st, "iga_disassemble", errs, bits, bitsLen);
        }
        std::vector<Diagnostic> errs = igax::getErrors(context);
        if (st == IGA_DECODE_ERROR) {
            throw DecodeError("iga_disassemble", errs, bits, bitsLen);
        } else {
            throw DisassembleError(st, "iga_disassemble", errs, bits, bitsLen);
        }
    }

    DisResult result;
    result.value = text;
    result.warnings = getWarnings(context);
    return result;
}

inline void Error::emit(std::ostream &os) const {
    os << api << ": " << iga_status_to_string(status);
}
inline std::string Error::str() const {
    std::stringstream ss;
    emit(ss);
    return ss.str();
}

inline void AssembleError::emit(std::ostream &os) const {
    if (status != IGA_PARSE_ERROR && status != IGA_ENCODE_ERROR) {
        os << api << ": " << iga_status_to_string(status) << "\n";
    }
    for (auto &e : errors) {
        e.emit(os, source);
    }
}

inline void DisassembleError::emit(std::ostream &os) const {
    if (status != IGA_DECODE_ERROR) {
        os << api << ": " << iga_status_to_string(status) << "\n";
    }
    for (auto &e : errors) {
        e.emit(os, "", bits, bitsLen);
    }
}

inline void Diagnostic::emitLoc(
    std::ostream &os) const
{
    if (line > 0) { // text error
        os << std::dec << "line " << line << "." << col << ":";
    } else { // binary error
        os << std::hex << "byte offset 0x" << off << ":";
    }
}

inline void Diagnostic::emitContext(
    std::ostream &os,
    const std::string &src,
    const void *bytes,
    size_t bytesLen) const
{
    if (line > 0) {
        // text error
        if (off <= (int)src.length()) {
            // we have context too, print that
            int ix = off;
            while (ix > 0 && src[ix - 1] != '\r' && src[ix - 1] != '\n') {
                ix--;
            }
            do {
                os << src[ix++];
            } while (ix < (int)src.size() && src[ix] != '\r' &&
                     src[ix] != '\n');
            os << "\n";
            for (int i = 1; i < col; i++) {
                os << ' ';
            }
            os << "^\n";
        }
    } else {
        // binary offset
        const unsigned char *bits = (const unsigned char *)bytes;
        if (bits && off + 3 < (int)bytesLen) {
            size_t iLen = (bits[off + 3] & 0x20) ? 8 : 16;
            if ((size_t)off + iLen < (int)bytesLen) {
                for (size_t i = 0; i < iLen && i + off < bytesLen; i++) {
                    if (i > 0)
                        os << ' ';
                    os << std::hex << std::setw(2) << std::setfill('0')
                              << (unsigned)bits[off + i];
                    if (i % 4 == 3)
                        os << ' ';
                }
                os << "\n";
            }
        }
    }
}
inline void Diagnostic::emit(
    std::ostream &os,
    const std::string &src,
    const void *bytes,
    size_t bytesLen) const
{
    emitLoc(os);
    os << " " << message << "\n";
    emitContext(os, src, bytes, bytesLen);
}

inline std::string Diagnostic::str(const std::string &src) const {
    std::stringstream ss;
    emit(ss, src);
    return ss.str();
}
inline std::string Diagnostic::str(const void *bytes, size_t bytesLen) const {
    std::stringstream ss;
    emit(ss, "", bytes, bytesLen);
    return ss.str();
}

inline std::vector<OpSpec> OpSpec::enumerate(Platform p)
{
    iga_opspec_t arrAuto[128];
    iga_opspec_t *arrPtr = &arrAuto[0];
    size_t arrLen = sizeof(arrAuto)/sizeof(arrAuto[0]);
    IGA_CHECKED_CALL(iga_opspec_enumerate, (iga_gen_t)p, arrPtr, &arrLen);
    if (arrLen > sizeof(arrAuto)/sizeof(arrAuto[0])) {
        arrPtr = (iga_opspec_t *)alloca(arrLen * sizeof(iga_opspec_t));
        IGA_CHECKED_CALL(iga_opspec_enumerate, (iga_gen_t)p, arrPtr, &arrLen);
    }
    std::vector<OpSpec> vec;
    vec.reserve(arrLen);
    for (size_t i = 0; i < arrLen; i++) {
        vec.emplace_back(p, arrPtr[i]);
    }
    return vec;
}

inline iga::Op OpSpec::op() const
{
    uint32_t iga_op;
    IGA_CHECKED_CALL(iga_opspec_op, m_op, &iga_op);
    return static_cast<iga::Op>(iga_op);
}


#define IGA_OPSPEC_STRING_GETTER(API, INITSIZE) { \
        char _staticBuf[INITSIZE]; \
        char *strPtr = &_staticBuf[0]; \
        size_t strCap = sizeof(_staticBuf); \
        IGA_CHECKED_CALL(API, m_op, strPtr, &strCap); \
        if (strCap > sizeof(_staticBuf)) { \
            strPtr = (char *)alloca(strCap); \
            IGA_CHECKED_CALL(API, m_op, strPtr, &strCap); \
        } \
        return std::string(strPtr); \
    }
inline std::string OpSpec::menmonic() const
IGA_OPSPEC_STRING_GETTER(iga_opspec_mnemonic, 16);
inline std::string OpSpec::name() const
IGA_OPSPEC_STRING_GETTER(iga_opspec_name, 32);
inline std::string OpSpec::description() const
IGA_OPSPEC_STRING_GETTER(iga_opspec_description, 128);

inline OpSpec OpSpec::parent() const
{
    uint32_t op_enum;
    iga_opspec_t par_op;
    IGA_CHECKED_CALL(iga_opspec_parent_op, m_op, &op_enum);
    IGA_CHECKED_CALL(iga_opspec_from_op,
        static_cast<iga_gen_t>(m_platform),
        op_enum,
        &par_op);
    return OpSpec(m_platform, par_op);
}

#undef IGA_OPSPEC_STRING_GETTER

} // igax::

#endif /* _IGAX_HPP */

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
#ifndef _IGA_MAIN_HPP_
#define _IGA_MAIN_HPP_

#include "fatal.hpp"
#include "io.hpp"

#include "api/iga.h"
#include "api/igax.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <vector>
#ifdef WIN32
// for _setmode(_fileno())...
#include <fcntl.h>
#include <io.h>
#endif

struct Opts {
    // ASM = assembly
    // DIS = disassembly
    // XLST = -Xlist-ops (list ops for a given platform)
    // XIFS = -Xifs (decode fields)
    // XDCMP = -Xdcmp (debug compaction)
    // AUTO = operate based on input (see inferPlatformAndMode below)
    enum Mode {ASM, DIS, XLST, XIFS, XDCMP, XDSD, AUTO};

    std::vector<std::string> inputFiles;             // .empty() means stdin
    std::string outputFile;                          // "" means stdout
    int verbosity            = 0;                    // -q, -v
    Mode mode                = AUTO;                 // -d, -a
    bool numericLabels       = false;                // -n
    iga_gen_t platform       = IGA_GEN_INVALID;      // -p=...
    uint32_t enabledWarnings = IGA_WARNINGS_DEFAULT; // -W*
    bool autoCompact         = false;                // -X[no]-autocompact
    bool legacyDirectives    = false;                // -Xlegacy-directives
    bool errorOnCompactFail  = true;                 // -Xwarn-on-compact-fail
    bool autosetDepInfo      = false;                // -Xauto-deps
    bool syntaxExts          = false;                // -Xsyntax-exts
    bool useNativeEncoder    = false;                // -Xnative

    bool printBits           = false;                // -Xprint-bits
    bool printDeps           = false;                // -Xprint-deps
    bool printHexFloats      = false;                // -Xprint-hex-floats
    bool printLdSt           = false;                // -Xprint-ldst
    bool printInstructionPc  = false;                // -Xprint-pc
};


bool disassemble(
    const Opts &opts,
    igax::Context &ctx,
    const std::string &inpFile); // -d: disassemble.cpp
bool assemble(
    const Opts &opts,
    igax::Context &ctx,
    const std::string &inpFile); // -a: assemble.cpp
bool assemble(
    const Opts &opts,
    igax::Context &ctx,
    const std::string &inpFile,
    const std::string &inpText,
    igax::Bits &bits); // assemble.cpp
bool decodeInstructionFields(
    const Opts &baseOpts); // -Xifs in decode_fields.cpp
bool debugCompaction(
    Opts opts); // -Xdcmp in decode_fields.cpp
bool listOps(
    const Opts &opts,
    const std::string &opmn); // -Xlist-ops: list_ops.cpp
bool decodeSendDescriptor(
    const Opts &opts); // -Xsds in decode_message.cpp

static void setOptBit(uint32_t &opts, uint32_t bit, bool isSet) {
    if (isSet) {
        opts |= bit;
    } else {
        opts &= ~bit;
    }
}

static void writeText(const Opts &opts, const std::string &outp) {
    if (opts.outputFile == "") {
#ifdef WIN32
        // http://stackoverflow.com/questions/22633665/extremely-slow-stdcout-using-ms-compiler
        // MSVC's stdout is unbuffered for most cases ...
        // (internally it can buffer sometimes, but not with std::cout)
        // We can force buffering here (stdout sits below cout apparently).
        // This is the recommended fix.
        setvbuf(stdout, 0, _IOLBF, 4096);
#endif
        writeTextStream("<<stdout>>", std::cout, outp.c_str(), outp.size());
        // fiddled with a different approach here
        //  writeTextStreamF("<<stdout>>", stdout, outp.c_str(), outp.size());
    } else {
        writeTextFile(opts.outputFile.c_str(), outp.c_str(), outp.size());
    }
}

static void writeBinary(const Opts &opts, const void *bits, size_t bitsLen) {
    if (opts.outputFile == "") {
        // have to use C/stdio here since C++ will not let us output binary
#ifdef WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
        if (fwrite(bits, 1, bitsLen, stdout) < bitsLen) {
            fatalExitWithMessage("iga: failed to write entire output");
        }
    } else {
        writeBinaryFile(opts.outputFile.c_str(), bits, bitsLen);
    }
}

#define IGA_CALL(F, ...)                                       \
    do {                                                       \
        iga_status_t _st = F(__VA_ARGS__);                     \
        if (_st != IGA_SUCCESS) {                              \
            fatalExitWithMessage(                              \
                "iga: " #F ": %s", iga_status_to_string(_st)); \
        }                                                      \
    } while (0)


static void emitWarningToStderr(
    const igax::Diagnostic &w,
    const std::string &inp)
{
    w.emitLoc(std::cerr);
    std::cerr << " warning: ";
    emitYellowText(std::cerr, w.message);
    std::cerr << "\n";

    w.emitContext(std::cerr, inp);
}
static void emitWarningToStderr(
    const igax::Diagnostic &w,
    const std::vector<unsigned char> &inp)
{

    w.emitLoc(std::cerr);
    std::cerr << " warning: ";
    emitYellowText(std::cerr, w.message);
    std::cerr << "\n";

    w.emitContext(std::cerr, "", inp.data(), inp.size());
}
static void emitErrorToStderr(
    const igax::Diagnostic &e,
    const std::string &inp)
{
    e.emitLoc(std::cerr);
    std::cerr << " error: ";
    emitRedText(std::cerr, e.message);
    std::cerr << "\n";

    e.emitContext(std::cerr, inp);
}
static void emitErrorToStderr(
    const igax::Diagnostic &e,
    const std::vector<unsigned char> &inp)
{
    e.emitLoc(std::cerr);
    std::cerr << " error: ";
    emitRedText(std::cerr, e.message);
    std::cerr << "\n";

    e.emitContext(std::cerr, "", inp.data(), inp.size());
}

static void inferPlatformAndMode(
    const std::string &file, Opts &os)
{
    std::string ext = "";
    size_t ix = file.rfind('.');
    if (ix != std::string::npos) {
        ext = file.substr(ix + 1);
    }

    // try and infer assemble/disassemble (-d or -a) if needed
    std::string extPfx = ext.substr(0, 3); // "krn" of "krn9" or "krn9p5"
    if (os.mode == Opts::Mode::AUTO) {
        if (extPfx == "dat" || extPfx == "krn") {
            os.mode = Opts::Mode::DIS;
        } else if (extPfx == "isa" || extPfx == "asm") {
            os.mode = Opts::Mode::ASM;
        }
    }

    // try and infer the project (-p) if needed
    if (os.platform == IGA_GEN_INVALID && ext.size() >= 3) {
        std::string prj = ext.substr(3);
        std::transform(prj.begin(), prj.end(), prj.begin(), ::toupper);
        if (prj == "7P5") {
            os.platform = IGA_GEN7p5;
        } else if (prj == "8") {
            os.platform = IGA_GEN8;
        } else if (prj == "9") {
            os.platform = IGA_GEN9;
        }
    }
}

static void ensurePlatformIsSet(const Opts &opts)
{
    if (opts.platform == IGA_GEN_INVALID) {
        const char *tool =
          opts.mode == Opts::Mode::XIFS ? "ifs" :
          opts.mode == Opts::Mode::XDCMP ? "dcmp" :
          opts.mode == Opts::Mode::XDSD ? "dsd" :
            "???";

        fatalExitWithMessage("-X%s: unable to infer platform (use -p)", tool);
    }
}


#endif // _IGA_MAIN_HPP_

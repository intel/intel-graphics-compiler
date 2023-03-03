/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_MAIN_HPP_
#define _IGA_MAIN_HPP_

#include "fatal.hpp"
#include "io.hpp"

#include "api/iga.h"
#include "api/igax.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
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
  enum class Mode { ASM, DIS, XLST, XIFS, XDCMP, XDSD, AUTO };
  enum class Color { NEVER, AUTO, ALWAYS };

  std::vector<std::string> inputFiles;             // .empty() means stdin
  std::string outputFile;                          // "" means stdout
  int verbosity = 0;                               // -q, -v
  Mode mode = Mode::AUTO;                          // -d, -a
  Color color = Color::AUTO;                       // --color=...
  bool numericLabels = false;                      // -n
  iga_gen_t platform = IGA_GEN_INVALID;            // -p=...
  bool outputOnFail = false;                       // --output-on-fail
  uint32_t enabledWarnings = IGA_WARNINGS_DEFAULT; // -W*
  bool autoCompact = false;                        // -X[no]-autocompact
  bool legacyDirectives = false;                   // -Xlegacy-directives
  bool errorOnCompactFail = true;                  // -Xwarn-on-compact-fail
  bool autosetDepInfo = false;                     // -Xauto-deps
  uint32_t sbidCount = 16;                         // -Xsbid-count
  bool syntaxExts = false;                         // -Xsyntax-exts
  bool useNativeEncoder = false;                   // -Xnative
  bool forceNoCompact = false;                     // -Xforce-no-compact
  uint32_t pcOffset = 0; // pcOffset provided with -Xset-pc-base

  bool printBits = false;          // -Xprint-bits
  bool printDefs = false;          // -Xprint-defs
  bool printDeps = false;          // -Xprint-deps
  bool printHexFloats = false;     // -Xprint-hex-floats
  bool printJson = false;          // -Xprint-json
  bool printJsonV1 = false;        // -Xprint-jsonV1
  bool printBfnExprs = true;       // -Xprint-bfnexprs
  bool printLdSt = false;          // -Xprint-ldst
  bool printInstructionPc = false; // -Xprint-pc
};

bool disassemble(const Opts &opts, igax::Context &ctx,
                 const std::string &inpFile); // -d: disassemble.cpp
bool assemble(const Opts &opts, igax::Context &ctx,
              const std::string &inpFile); // -a: assemble.cpp
bool assemble(const Opts &opts, igax::Context &ctx, const std::string &inpFile,
              const std::string &inpText,
              igax::Bits &bits); // assemble.cpp
bool decodeInstructionFields(
    const Opts &baseOpts);       // -Xifs in decode_fields.cpp
bool debugCompaction(Opts opts); // -Xdcmp in decode_fields.cpp
bool listOps(const Opts &opts,
             const std::string &opmn);       // -Xlist-ops: list_ops.cpp
bool decodeSendDescriptor(const Opts &opts); // -Xsds in decode_message.cpp

static inline void setOptBit(uint32_t &opts, uint32_t bit, bool isSet) {
  if (isSet) {
    opts |= bit;
  } else {
    opts &= ~bit;
  }
}

static inline void writeText(const Opts &opts, const std::string &outp) {
  if (opts.outputFile == "") {
#ifdef WIN32
    // http://stackoverflow.com/questions/22633665/extremely-slow-stdcout-using-ms-compiler
    // MSVC's stdout is unbuffered for most cases ...
    // (internally it can buffer sometimes, but not with std::cout)
    // We can force buffering here (apparently stdout sits below cout).
    // This a recommended fix.
    setvbuf(stdout, 0, _IOLBF, 4096);
#endif
    writeTextStream("<<stdout>>", std::cout, outp.c_str());
    // fiddled with a different approach here
    //  writeTextStreamF("<<stdout>>", stdout, outp.c_str(), outp.size());
  } else {
    writeTextFile(opts.outputFile.c_str(), outp.c_str());
  }
}

static inline void writeBinary(const Opts &opts, const void *bits,
                               size_t bitsLen) {
  if (opts.outputFile == "") {
    // have to use C/stdio here since C++ will not let us output binary
#ifdef WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    if (fwrite(bits, 1, bitsLen, stdout) < bitsLen) {
      fatalExitWithMessage("failed to write entire output");
    }
  } else {
    writeBinaryFile(opts.outputFile.c_str(), bits, bitsLen);
  }
}

#define IGA_CALL(F, ...)                                                       \
  do {                                                                         \
    iga_status_t _st = F(__VA_ARGS__);                                         \
    if (_st != IGA_SUCCESS) {                                                  \
      fatalExitWithMessage("iga: " #F ": ", iga_status_to_string(_st));        \
    }                                                                          \
  } while (0)

static inline void emitWarningToStderr(const igax::Diagnostic &w,
                                       const std::string &inp) {
  w.emitLoc(std::cerr);
  std::cerr << " warning: ";
  emitYellowText(std::cerr, w.message);
  std::cerr << "\n";

  w.emitContext(std::cerr, inp);
}

static inline void emitWarningToStderr(const igax::Diagnostic &w,
                                       const std::vector<unsigned char> &inp) {
  w.emitLoc(std::cerr);
  std::cerr << " warning: ";
  emitYellowText(std::cerr, w.message);
  std::cerr << "\n";

  w.emitContext(std::cerr, "", inp.data(), inp.size());
}

static inline void emitErrorToStderr(const igax::Diagnostic &e,
                                     const std::string &inp) {
  e.emitLoc(std::cerr);
  std::cerr << " error: ";
  emitRedText(std::cerr, e.message);
  std::cerr << "\n";

  e.emitContext(std::cerr, inp);
}
static inline void emitErrorToStderr(const igax::Diagnostic &e,
                                     const std::vector<unsigned char> &inp) {
  e.emitLoc(std::cerr);
  std::cerr << " error: ";
  emitRedText(std::cerr, e.message);
  std::cerr << "\n";

  e.emitContext(std::cerr, "", inp.data(), inp.size());
}

static inline std::string normalizePlatformName(std::string inp) {
  std::string norm;
  for (size_t i = 0; i < inp.size(); i++) {
    if (inp[i] == '.')
      norm += 'p'; // 12.1 ==> 12p1
    else
      norm += (char)std::tolower(inp[i]);
  }
  return norm;
}

static inline void inferPlatform(const std::string &file, Opts &os) {
  // try and infer the project (-p) if needed
  std::string ext = "";
  size_t ix = file.rfind('.');
  if (ix != std::string::npos) {
    ext = file.substr(ix + 1);
  }

  if (os.platform == IGA_GEN_INVALID && ext.size() >= 3) {
    // we have a file extension like "asm12p1" or "krn11"
    std::string prj = ext.substr(3); // skip "krn" or "asm" part of ext
    prj = normalizePlatformName(prj);
    try {
      const auto pis = igax::QueryPlatforms();
      for (const auto &pi : pis) {
        if (prj == pi.suffix) {
          os.platform = pi.toGen();
          break;
        }
      }
    } catch (...) {
      // drop exception
    }
  }
}

static inline void inferPlatformAndMode(const std::string &file, Opts &os) {
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
  inferPlatform(file, os);
}

static inline void ensurePlatformIsSet(const Opts &opts) {
  if (opts.platform == IGA_GEN_INVALID) {
    const char *tool = opts.mode == Opts::Mode::XIFS    ? "ifs"
                       : opts.mode == Opts::Mode::XDCMP ? "dcmp"
                       : opts.mode == Opts::Mode::XDSD  ? "dsd"
                                                        : "???";

    fatalExitWithMessage("-X", tool, ": unable to infer platform (use -p=...)");
  }
}

static inline uint32_t makeFormattingOpts(const Opts &opts) {
  uint32_t fmtOpts = 0;

  setOptBit(fmtOpts, IGA_FORMATTING_OPT_NUMERIC_LABELS, opts.numericLabels);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_SYNTAX_EXTS, opts.syntaxExts);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_HEX_FLOATS, opts.printHexFloats);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_PC, opts.printInstructionPc);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_BITS, opts.printBits);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_DEFS, opts.printDefs);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_DEPS, opts.printDeps);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_LDST, opts.printLdSt);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_BFNEXPRS, opts.printBfnExprs);
  bool useColor = opts.color == Opts::Color::ALWAYS ||
                  (opts.color == Opts::Color::AUTO && opts.outputFile.empty() &&
                   iga::IsTty(std::cout));
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_ANSI, useColor);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_JSON, opts.printJson);
  setOptBit(fmtOpts, IGA_FORMATTING_OPT_PRINT_JSON_V1, opts.printJsonV1);
  return fmtOpts;
}

#endif // _IGA_MAIN_HPP_

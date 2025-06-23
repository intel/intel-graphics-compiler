/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_FORMATTER
#define _IGA_FORMATTER

#include "../ErrorHandler.hpp"
#include "../IR/DUAnalysis.hpp"
#include "../IR/Kernel.hpp"
#include "../strings.hpp"
#include "Floats.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace iga {
//////////////////////////////////////////////////////////////////////////
// EXTERNAL INTERFACES
//////////////////////////////////////////////////////////////////////////

typedef const char *(*LabelerFunction)(int32_t, void *);

// Formatter options
struct FormatOpts {
  const Model &model;
  LabelerFunction labeler = nullptr;
  void *labelerContext = nullptr;
  bool numericLabels = false;
  bool hexFloats = true;
  bool printInstPc = false;
  bool syntaxExtensions = false;
  bool syntaxBfnSymbolicFunctions = true;
  // swsb encoding mode, if not specified, the encoding mode will be
  // derived from platform by SWSB::getEncodeMode
  SWSB_ENCODE_MODE swsbEncodingMode = SWSB_ENCODE_MODE::SWSBInvalidMode;
  bool printInstDeps = false;
  bool printInstDefs = false;
  bool printInstBits = true;
  bool printLdSt = false;
  bool printAnsi = false;
  bool printJson = false;
  int printJsonVersion = 1;
  DepAnalysis *liveAnalysis = nullptr;
  uint32_t basePCOffset = 0;

  // format with default labels
  FormatOpts(const Model &m) : model(m) {}

  // format with all parameters settable
  // requires at least a context
  FormatOpts(const Model &m, LabelerFunction _labeler,
             void *_labelerCtx = nullptr, bool _numericLabels = false,
             bool _printInstPc = false, bool _hexFloats = true,
             bool _syntaxExtensions = false)
      : model(m), labeler(_labeler), labelerContext(_labelerCtx),
        numericLabels(_numericLabels), hexFloats(_hexFloats),
        printInstPc(_printInstPc), syntaxExtensions(_syntaxExtensions) {}

  void setSWSBEncodingMode(SWSB_ENCODE_MODE mode) { swsbEncodingMode = mode; }

  // in iga.cpp
  // pcOffset is added to pc string in comments during disassembly
  void addApiOpts(uint32_t fmtOpts, uint32_t pcOffset = 0);
};

void FormatKernel(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                  const Kernel &k, const void *bits = nullptr);

void FormatInstruction(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                       const Instruction &i, const void *bits = nullptr);

#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
// this uses the decoder, which uses exceptions
// but only the IGA tester needs this; so we can ifdef it out
void FormatInstruction(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                       size_t startPc, const void *bits,
                       bool useNativeDecoder = false);
void FormatInstruction(ErrorHandler &e, std::ostream &o, const FormatOpts &opts,
                       const void *bits);
#endif // IGA_DISABLE_ENCODER_EXCEPTIONS

void GetDefaultLabelName(std::ostream &o, int32_t pc);

/////////////////////////////////////////////////////////
// Stuff mainly for testing and debugging.

// for debugging, shows the opcode and bytes
std::string FormatOpBits(const iga::Model &m, const void *bits);
// decodes the op for this platform
std::string FormatOpName(const iga::Model &m, const void *bits);

//////////////////////////////////////////////////////////////////////////
// Used by other modules internal to this directory
//////////////////////////////////////////////////////////////////////////

// This wraps a constant ansi escape sequence in a unique type so
// template functions can handle them different from (const char *)'s.
// We define emitT(ansi_esc) to not count the characters as part of the
// current column (for column alignment).
struct ansi_esc {
  const char *esc;
  constexpr ansi_esc() : ansi_esc(nullptr) {}
  constexpr ansi_esc(const char *_seq) : esc(_seq) {}
};

// This wraps a value of some sort to be emitted with an ANSI escape
// sequence.  We emit this and an ANSI_RESET afterwards.
// E.g. emit(ansi_span<int>("\033[1;32m",0x12))
template <typename T> struct ansi_span {
  ansi_esc esc;
  const T &val;
  constexpr ansi_span(const T &_val) : ansi_span(nullptr, _val) {}
  constexpr ansi_span(const char *_seq, const T &_val) : esc(_seq), val(_val) {}
};

// The abstract implementation of a formatter that has a notion of
// column alignment and some other basic, language-agnostic constructs.
class BasicFormatter {
  size_t currColCapacity; // preferred size of current column
  size_t currColSize;     // current col's size
  size_t currLineDebt;    // sum total of the column overflow
                          // (later cols can pay for overflow
                          // to realign future columns)
  bool printAnsi;

protected:
  ansi_esc ANSI_RESET;
  // subclasses to this class may define other ansi sequences

  std::ostream &o;

  BasicFormatter(bool _printAnsi, std::ostream &out)
      : currColCapacity((size_t)-1), currColSize(0),
        currLineDebt(0), printAnsi(_printAnsi), o(out) {
    // TODO: could make these mappable via environment variable
    // export IGA_FormatAnsiRegisterArf="\033[38;2;138;43;211m"
    // export IGA_FormatAnsiMnemonic=...
    // export IGA_FormatAnsiComment=...
    if (printAnsi) {
      ANSI_RESET = "\033[0m";
    }
  }

public:
  // start or finish a padded column
  void startColumn(int len) {
    IGA_ASSERT(len >= 0, "negative column size");
    IGA_ASSERT(currColCapacity == (size_t)-1,
               "startColumn() called from with-in column "
               "(missing finishColumn())");
    currColCapacity = (size_t)len;
    currColSize = 0;
  }
  void finishColumn() {
    IGA_ASSERT(currColCapacity != (size_t)-1,
               "finishColumn() called with no active column "
               "(missing startColumn(...))");
    //
    if (currColSize <= currColCapacity) {
      // we underflowed; we have space left;
      // if there is debt, we can cover some of it
      size_t padding = currColCapacity - currColSize;
      if (currLineDebt > 0) {
        size_t correction = std::min<size_t>(currLineDebt, padding);
        padding -= correction;
        currLineDebt -= correction;
      }
      emitSpaces(padding);
    } else if (currColSize > currColCapacity) {
      // we overflowed accummulate debt
      currLineDebt += currColSize - currColCapacity;
    }
    //
    currColCapacity = (size_t)-1;
    currColSize = 0;
  }

  void newline() {
    IGA_ASSERT(currColCapacity == (size_t)-1, "column open at newline");
    emit('\n');
    currLineDebt = 0;
  }

  template <typename T> void emitT(ansi_span<T> t) {
    emitT(t.esc);
    emitT(t.value);
    emitT(ANSI_RESET);
  }
  void emitT(ansi_esc e) {
    // specialize this instance to not count the ANSI escapes as
    // output characters (size of column)
    if (e.esc)
      o << e.esc;
  }
  template <typename T> void emitT(const T &t) {
    size_t n = (size_t)o.tellp();
    o << t;
    currColSize += (size_t)o.tellp() - n;
  }

  // enables you emit a list of things
  //
  // emit(foo, bar, baz, ...);
  template <typename T, typename... Ts> void emit(const T& t) { emitT(t); }
  template <typename T, typename... Ts> void emit(const T& t, const Ts&... ts) {
    emitT(t);
    emit(ts...);
  }

  // emits an sequence conditionally surrounded by an ANSI color
  template <typename T, typename... Ts>
  void emitAnsi(bool z, ansi_esc esc, T t0, Ts... ts) {
    if (z)
      emitT(esc);

    emit(std::move(t0), std::move(ts)...);

    if (z)
      emitT(ANSI_RESET);
  }
  template <typename T, typename... Ts>
  void emitAnsi(ansi_esc esc, T t, Ts... ts) {
    emitAnsi(true, esc, std::move(t), std::move(ts)...);
  }

  void emitSpaces(size_t n) {
    for (size_t i = 0; i < n; i++)
      o << ' ';
    currColSize += n;
  }

  template <typename T> void emitDecimal(const T &t) { o << std::dec << t; }

  template <typename T> void emitHex(const T &t, int cw = 0) {
    fmtHex(o, (uint64_t)t, cw);
    o << std::dec;
  }

  template <typename T> void emitHexDigits(const T &t, int cw = 0) {
    fmtHexDigits(o, (uint64_t)t, cw);
    o << std::dec;
  }

  template <typename T> void emitBinary(const T &t) {
    emit("0b");
    std::stringstream ss;
    T bits = t;
    while (bits != 0) {
      emit((bits & 0x1) ? '1' : '0');
      bits >>= 1;
    }
    std::string str = ss.str();
    for (int i = (int)str.length() - 1; i >= 0; i--) {
      emit(str[i]);
    }
  }

  template <typename T> void emitFloat(const T &f) { FormatFloat(o, f); }
};

struct ColumnPreferences {
  int predication;
  int opCodeExecInfo;
  int flagMod;
  int dstOp;
  int srcOp;
  int sendDstOp; // we give send ops less space than normal
  int sendSrcOp; // we give send ops less space than norma
  int sendDesc;
  ColumnPreferences()
      // enough space for (~f0.0) or (W), (W&~f0.0) overflows
      : predication(7)
        // sends (16|M0)
        // madm (16|M16)
        ,
        opCodeExecInfo(13)
        // (le)f0.0
        ,
        flagMod(9)
        // r120.11<1>:hf
        ,
        dstOp(12), srcOp(16), sendDstOp(7) // null:ud
                                           // r120.4<4;4,1>:ub
        ,
        sendSrcOp(6) // r120#1
        ,
        sendDesc(10) {}
};
} // namespace iga

#endif

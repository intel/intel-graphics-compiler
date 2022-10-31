/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BACKEND_BIT_PROCESSOR_HPP
#define IGA_BACKEND_BIT_PROCESSOR_HPP

#include "../ErrorHandler.hpp"
#include "../IR/Instruction.hpp"
#include "../strings.hpp"

#include <cstdarg>
#include <cstdint>

namespace iga {
// This class is a super class of all traversals over a set of encoded
// bits.  It is an adapter to ErrorHandler, but extends that functionality
// with the notion of a current PC.  The PC may be reset via
// restart(), advanced via advancePc(...), and changed via setPc.
// Subclasses should be sensitive to fatal conditions that will propagate
// up via exception.
//
// This traversal supports three levels of errors.
//   - warnings: potentially non-fatal messages; tools may just report
//     these to stderr and exit 0 still
//   - errors: indicate a condition where tools will want to exit non-zero
//     and potentially stop processing early.  The underlying BitProcessor
//     will continue
//   - fatal: a panic state that logs an error and throws an exception
//     to exit processing
class BitProcessor {
private:
  ErrorHandler &m_errorHandler;
  int32_t m_currentPc;
  const Instruction *m_currInst;

protected:
  ErrorHandler &errorHandler() { return m_errorHandler; }

public:
  BitProcessor(ErrorHandler &eh)
      : m_errorHandler(eh), m_currentPc(0), m_currInst(nullptr) {}

  int32_t currentPc() const { return m_currentPc; }
  const Instruction *currentInst() const { return m_currInst; }

  void restart() {
    m_currentPc = 0;
    m_currInst = nullptr;
  }
  void advance(int32_t by, const Instruction *inst) {
    setPc(m_currentPc + by);
    setCurrInst(inst);
  }
  void advancePc(int32_t by) { setPc(m_currentPc + by); }
  void setCurrInst(const Instruction *inst) { m_currInst = inst; }
  void setPc(int32_t pc) { m_currentPc = pc; }
  Loc defaultLoc() const {
    return m_currInst ? m_currInst->getLoc() : m_currentPc;
  }

  ///////////////////////////////////////////////////////////////////////
  // warnings
  template <typename... Ts> void warningT(Ts... ts) {
    warningAtT(defaultLoc(), iga::format(ts...));
  }
  template <typename... Ts> void warningAtT(const Loc &loc, Ts... ts) {
    warningAtS(loc, iga::format(ts...));
  }
  void warningAtS(const Loc &loc, std::string msg);

  ///////////////////////////////////////////////////////////////////////
  // errors
  template <typename... Ts> void errorT(Ts... ts) {
    errorAtT(defaultLoc(), iga::format(ts...));
  }
  template <typename... Ts> void errorAtT(const Loc &loc, Ts... ts) {
    errorAtS(loc, iga::format(ts...));
  }
  void errorAtS(const Loc &loc, std::string msg);

  ///////////////////////////////////////////////////////////////////////
  // fatals stop processing (unless exceptions are disabled)
  template <typename... Ts> void fatalT(Ts... ts) {
    fatalAtS(defaultLoc(), iga::format(ts...));
  }
  template <typename... Ts> void fatalAtT(const Loc &loc, Ts... ts) {
    fatalAtS(loc, iga::format(ts...));
  }
  void fatalAtS(const Loc &loc, std::string msg);

  ///////////////////////////////////////////////////////////////////////
  bool hasFatalError() const { return m_errorHandler.hasFatalError(); }
  size_t getErrorCount() const { return m_errorHandler.getErrors().size(); }
};

// can be intialized in .h, but this is a work around for
// gcc 4.8.3 bug on linux 64bit debug
const static int32_t COMPACTION_CONTROL = 29;
const static int32_t COMPACTED_SIZE = 8;
const static int32_t UNCOMPACTED_SIZE = 16;
} // namespace iga
#endif // IGA_BACKEND_BIT_PROCESSOR_HPP

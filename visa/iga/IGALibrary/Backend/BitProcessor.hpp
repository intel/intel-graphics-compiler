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
#ifndef IGA_BACKEND_BIT_PROCESSOR_HPP
#define IGA_BACKEND_BIT_PROCESSOR_HPP

#include "../ErrorHandler.hpp"
#include "../IR/Instruction.hpp"

#include <cstdint>
#include <cstdarg>

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
    class BitProcessor
    {
    private:
        ErrorHandler       &m_errorHandler;
        int32_t             m_currentPc;
        const Instruction  *m_currInst;
    protected:
        ErrorHandler &errorHandler() { return m_errorHandler; }
    public:
        BitProcessor(ErrorHandler &eh)
            : m_errorHandler(eh)
            , m_currentPc(0)
            , m_currInst(nullptr)
        {
        }

        int32_t currentPc() const {
            return m_currentPc;
        }
        const Instruction *currentInst() const {
            return m_currInst;
        }

        void restart() {
            m_currentPc = 0;
            m_currInst = nullptr;
        }
        void advance(int32_t by, const Instruction *inst) {
            setPc(m_currentPc + by);
            setCurrInst(inst);
        }
        void advancePc(int32_t by) {
            setPc(m_currentPc + by);
        }
        void setCurrInst(const Instruction *inst) {
            m_currInst = inst;
        }
        void setPc(int32_t pc) {
            m_currentPc = pc;
        }

        void warning(const char *patt, ...);
        void warningAt(const Loc& loc, const char *patt, ...);
        void warningAt(const Loc& loc, const char *patt, va_list& va);

        void error(const char *patt, ...);
        void errorAt(const Loc& loc, const char *patt, ...);
        void errorAt(const Loc& loc, const char *patt, va_list& va);

        void fatal(const char *patt, ...);
        void fatalAt(const Loc& loc, const char *patt, ...);
        void fatalAt(const Loc& loc, const char *patt, va_list& va);
        bool hasFatalError() { return m_errorHandler.hasFatalError(); }
    };

    //can be intialized in .h, but this is a work around for
    //gcc 4.8.3 bug on linux 64bit debug
    const static int32_t COMPACTION_CONTROL = 29;
    const static int32_t COMPACTED_SIZE     = 8;
    const static int32_t UNCOMPACTED_SIZE   = 16;
} // namespace iga
#endif // IGA_BACKEND_BIT_PROCESSOR_HPP

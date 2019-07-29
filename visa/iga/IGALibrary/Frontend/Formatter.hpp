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
#ifndef _IGA_FORMATTER
#define _IGA_FORMATTER

#include "Floats.hpp"
#include "../ErrorHandler.hpp"
#include "../IR/DUAnalysis.hpp"
#include "../IR/Kernel.hpp"
#include "../strings.hpp"

#include <iomanip>
#include <ostream>
#include <string>
#include <sstream>

namespace iga {
    //////////////////////////////////////////////////////////////////////////
    // EXTERNAL INTERFACES
    //////////////////////////////////////////////////////////////////////////

    typedef const char  *(*LabelerFunction)(int32_t, void *);

    // Formatter options
    struct FormatOpts {
        Platform          platform;
        LabelerFunction   labeler = nullptr;
        void             *labelerContext = nullptr;
        bool              numericLabels = false;
        bool              hexFloats = true;
        bool              printInstPc = false;
        bool              syntaxExtensions = false;
        bool              printInstDeps = true;
        bool              printInstBits = true;
        bool              printLdSt = false;
        DepAnalysis      *liveAnalysis = nullptr;

        // format with default labels
        FormatOpts(Platform _platform)
            : platform(_platform)
        {
        }

        // format with all parameters settable
        // requires at least a context
        FormatOpts(
            Platform _platform,
            LabelerFunction _labeler,
            void *_labelerCtx = nullptr,
            bool _numericLabels = false,
            bool _printInstPc = false,
            bool _hexFloats = true,
            bool _syntaxExtensions = false)
            : platform(_platform)
            , labeler(_labeler)
            , labelerContext(_labelerCtx)
            , numericLabels(_numericLabels)
            , hexFloats(_hexFloats)
            , printInstPc(_printInstPc)
            , syntaxExtensions(_syntaxExtensions)
        {
        }

    };


    void FormatKernel(
        ErrorHandler &e,
        std::ostream &o,
        const FormatOpts &opts,
        const Kernel &k,
        const void *bits = nullptr);

    void FormatInstruction(
        ErrorHandler &e,
        std::ostream &o,
        const FormatOpts &opts,
        const Instruction &i,
        const void *bits = nullptr);


#ifndef DISABLE_ENCODER_EXCEPTIONS
    // this uses the decoder, which uses exceptions
    void FormatInstruction(
        ErrorHandler &e,
        std::ostream &o,
        const FormatOpts &opts,
        size_t startPc,
        const void *bits,
        bool useNativeDecoder = false);
    void FormatInstruction(
        ErrorHandler &e,
        std::ostream &o,
        const FormatOpts &opts,
        const void *bits);
#endif

    void GetDefaultLabelName(
        std::ostream &o,
        int32_t pc);

    /////////////////////////////////////////////////////////
    // Stuff mainly for testing and debugging.

    // for debugging, shows the opcode and bytes
    std::string FormatOpBits(const iga::Model &m, const void *bits);
    // decodes the op for this platform
    std::string FormatOpName(const iga::Model &m, const void *bits);






    //////////////////////////////////////////////////////////////////////////
    // Used by other modules internal to this directory
    //////////////////////////////////////////////////////////////////////////




    // The abstract implementation of a formatter that has a notion of
    // column alignment and some other basic, language-agnostic constructs.
    class BasicFormatter {
        size_t           currColLen; // length of the current column
        size_t           currColStart; // stream offset of start of last col
        size_t           currColDebt; // how far the current column has overflowed
    protected:
        std::ostream&    o;
        BasicFormatter(std::ostream &out) :
            currColLen(0),
            currColStart(0),
            currColDebt(0),
            o(out)
        {
        }
    public:

        // start or finish a padded column
        void startColumn(int len) {
            currColLen = len;
            currColStart = (size_t)o.tellp();
        }
        void finishColumn() {
            size_t end = (size_t)o.tellp();
            size_t actualWidth = end - currColStart;
            if (actualWidth <= currColLen) {
                // we underflowed; we have space left;
                // if there is debt, we can cover some of it
                size_t padding = currColLen - actualWidth;
                if (currColDebt > 0) {
                    size_t correction = std::min<size_t>(currColDebt, padding);
                    padding -= correction;
                    currColDebt -= correction;
                }
                emitSpaces(padding);
            } else if (actualWidth > currColLen) {
                // we overflowed accummulate debt
                currColDebt += actualWidth - currColLen;
            }
        }


        void newline() {
            emit('\n');
            currColDebt = 0;
        }

        template <typename T>
        void emit(const T &t) {
            o << t;
#ifdef TRACE_EMIT
            std::cerr << t;
            std::cerr.flush();
#endif
        }
        template <typename T, typename U>
        void emit(const T &t, const U &u) {
            emit(t); emit(u);
        }
        template <typename T, typename U, typename V>
        void emit(const T &t, const U &u, const V &v) {
            emit(t); emit(u); emit(v);
        }
        template <typename T, typename U, typename V, typename W>
        void emit(const T &t, const U &u, const V &v, const W &w) {
            emit(t); emit(u); emit(v); emit(w);
        }


        void emitSpaces(size_t n) {
            for (size_t i = 0; i < n; i++) {
                emit(' ');
            }
        }


        template <typename T>
        void emitDecimal(const T &t) {
            o << std::dec << t;
#ifdef TRACE_EMIT
            std::cerr << std::dec << t;
            std::cerr.flush();
#endif
        }


        template <typename T>
        void emitHex(const T &t, int w = 0) {
            fmtHex(o, (uint64_t)t);
#ifdef TRACE_EMIT
            std::cerr << "0x" << std::hex << t << std::dec;
            std::cerr.flush();
#endif
        }


        template <typename T>
        void emitBinary(const T &t) {
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


        template <typename T>
        void emitFloat(const T &f) {
            FormatFloat(o, f);
        }
    };


    struct ColumnPreferences {
        int predication;
        int opCodeExecInfo;
        int flagMod;
        int dstOp;
        int srcOp;
        int sendDstOp; // we give send ops less space than normal
        int sendSrcOp; //we give send ops less space than norma
        int sendDesc;
        ColumnPreferences()
            : predication(8)
            // sends (16|M0)
            // madm (16|M16)
            , opCodeExecInfo(13)
            // (le)f0.0
            , flagMod(9)
            // r120.11<1>:hf
            , dstOp(12)
            , srcOp(16)
            , sendDstOp(7) // null:ud
                           // r120.4<4;4,1>:ub
            , sendSrcOp(6) // r120#1
            , sendDesc(10)
        {
        }
    };
} //end: namespace iga*

#endif

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
#ifndef IGA_FRONTEND_KERNELPARSER
#define IGA_FRONTEND_KERNELPARSER

#include "Parser.hpp"
#include "../IR/InstBuilder.hpp"
#include "../IR/Kernel.hpp"
#include "../IR/Loc.hpp"
#include "../IR/Types.hpp"
#include "../ErrorHandler.hpp"

// #include <functional>
#include <map>
#include <string>

namespace iga {
    struct ParseOpts {
        // Enables certain IsaAsm-era directives
        //   .default_execution_size(...)
        //   .default_register_type(...)
        bool supportLegacyDirectives = false;
        // emits warnings about deprecated syntax
        bool deprecatedSyntaxWarnings = true;
        SWSB_ENCODE_MODE swsbEncodeMode = SWSB_ENCODE_MODE::SWSBInvalidMode;

        // sets the maximum number of fatal syntax errors allowable
        // before we give up on the parse
        size_t maxSyntaxErrors = 3;

        ParseOpts(const Model &model) {
            swsbEncodeMode = model.getSWSBEncodeMode();
        }
    };

    // The primary API for parsing GEN kernels
    Kernel *ParseGenKernel(
        const Model &model,
        const char *inp,
        ErrorHandler &e,
        const ParseOpts &popts);

    // using SymbolTableFunc =
    //     std::function<bool(const std::string &, ImmVal &)>;

    // Generic GEN parser that can:
    //   - parse constant expressions and knows it's model
    //   - etc...
    //
    struct GenParser : public Parser
    {
        const Model&                   m_model;
        InstBuilder&                   m_builder;
        const ParseOpts                m_opts;

        // TODO: sink to KernelParser
        const OpSpec                  *m_opSpec;
        Operand::Kind                  m_srcKinds[3];
        Loc                            m_execSizeLoc;

        GenParser(
            const Model &model,
            InstBuilder &handler,
            const std::string &inp,
            ErrorHandler &eh,
            const ParseOpts &pots);

        Platform platform() const {return m_model.platform;}

        void ParseExecInfo(
            ExecSize dftExecSize,
            ExecSize &execSize,
            ChannelOffset &chOff);
        Type SendOperandDefaultType(int srcIx) const;
        Type ParseSendOperandTypeWithDefault(int srcIx);

        bool LookupReg(
            const std::string &str,
            const RegInfo *&regInfo,
            int &regNum);
        bool PeekReg(const RegInfo*& regInfo, int& regNum);
        bool ConsumeReg(const RegInfo*& regInfo, int &regNum);

        // expression parsing
        // &,|
        // <<,>>
        // +,-
        // *,/,%
        // -(unary neg)
        bool TryParseConstExpr(ImmVal &v,int srcOpIx = -1);
        bool TryParseIntConstExpr(ImmVal &v, const char *forWhat);

    protected:
        void ensureIntegral(const Token &t, const ImmVal &v);
        void checkNumTypes(const ImmVal &v1, const Token &t, const ImmVal &v2);
        void checkIntTypes(const ImmVal &v1, const Token &t, const ImmVal &v2);

        ImmVal evalBinExpr(const ImmVal &v1, const Token &t, const ImmVal &v2);
        bool parseBitwiseExpr(bool consumed, ImmVal &v);
        bool parseShiftExpr(bool consumed, ImmVal &v);
        bool parseAddExpr(bool consumed, ImmVal &v);
        bool parseMulExpr(bool consumed, ImmVal &v);
        bool parseUnExpr(bool consumed, ImmVal &v);
        bool parsePrimary(bool consumed, ImmVal &v);
    private:
        void initSymbolMaps();
        std::map<std::string,const RegInfo*>  m_regmap;
    }; // class GenParser
} // iga::
#endif

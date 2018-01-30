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
#ifndef IGA_FRONTEND_SENDS_INTERFACE
#define IGA_FRONTEND_SENDS_INTERFACE

#include "Types.hpp"
#include "../Formatter.hpp"
#include "../../IR/Loc.hpp"
#include "../../IR/Instruction.hpp"
#include "../../Models/Models.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <ostream>
#include <vector>

//
// This module (directory) desugars and re-sugars send messages into ld/st
// syntax where possible.
//
//  1. The IGA IR for instructions (Instruction.hpp) has no support for
//     load store information currently.  Hence we translate send to ld/st
//     during parse and during formatting.  Decoding and encoding will
//     always be dealing with good-old send syntax (in IR form).
//
//  2. When formatting this interface can always punt (fail) and the
//     caller must fall back to the canonical send syntax.  Moreover,
//     certain send messages will lack a syntax and must fall back.
//
//  3. Parsing works by having the parser parse a very generic format.
//     It forwards
//
//
//
namespace iga
{
    ////////////////////////////////////////////////////////////////////
    // PARSING:
    //
    // Parser pulls:
    //   LdInst = ID ('.' ID)*   LdStExecInfo   LdDst  LdSrc LdInstOpts
    //   StInst = ID ('.' ID)*   LdStExecInfo   StDst  StSrc StInstOpts
    //            ^^^^^^^^^^^ tokens   ^^^ execSize
    //
    // LdStExecInfo = (4/8|M0)
    //
    struct LdStInstSyntax {
        std::vector<std::pair<Loc,std::string>>  tokens;
        ExecSize                                 execSize, messageSimd;
        int                                      dstReg, dstLen; // e.g. r12-13 (is 12 and 1)
        int                                      src0Reg, src0Len;
        int                                      src1Reg, src1Len; // for split sends (-1 means null)
        int32_t                                  ldStAddrOff; // for scratch and SSBO

        // only consists of instruction options not accepted by parent parser
        std::vector<std::pair<Loc,std::string>>  instOpts;
    };

    // parses a sequence
    const OpSpec &ParseMessageOpSpec(
        // TODO: replace by passing in parser
        std::vector<std::pair<Loc,std::string>> &opTokens,
        int execSize,
        std::vector<std::pair<Loc,std::string>> &instOptTokens,
        Message &m,
        std::string &error);
    // most will ignore
    void ParseMessageExecSize(
        // TODO: replace by passing in parser
        Message &m,
        int execSize,
        std::string &error);


    struct LdStInstOutput {
        Op                     op;
        int32_t                desc;
        int32_t                exDesc;

        Loc                    errLoc; // only set upon error
        std::string            errMsg; // only set upon error
    };


    // parsing interface
    LdStInstOutput TranslateSyntaxToMessage(
        const Model &model,
        const LdStInstSyntax &syn);


    ////////////////////////////////////////////////////////////////////
    // FORMATTING:
    //
    // returns false if we cannot format the syntax
    bool TryFormatSendSyntax(
        const Model &model,
        BasicFormatter &fmtr,
        const Instruction &inst,
        std::vector<const char *> &instOpts,
        std::string *whyFailed);




    ////////////////////////////////////////////////////////////////////
    // DECODING send to LD/ST IR
    //

    // If the translation fails, we return a Message with Message::type == INVALID
    Message TranslateSendToMessage(
        const Model &model,
        const Instruction &inst,
        std::string &error);
} // namespace

#endif

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

#pragma once
///
/// ISA IR Disassembler
///

#include "Common_ISA.h"

#include <list>
#include <string>

/// Looks up a GEN variable name
const char* getGenVarName(int id, const print_format_provider_t& header);

/// - Print out kernel scope header
std::string printKernelHeader(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    bool isKernel,
    int funcionId,
    const Options *opt);

///
/// - Takes an isa header, a kernel/function header, and isa instruction and
///   returns a string contraining the instruction's isaasm.
///
std::string printInstruction(
    const print_format_provider_t* header,
    const CISA_INST* instruction,
    const Options *opt);

///
/// - Takes a isa header, a kernel header, and a instruction lists and
///   returns a string contraining the kernel's entire isaasm.
///
std::string printKernel(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const std::list<CISA_INST*>& instructions,
    const Options *opt);

///
/// - Similar to printKernel() but for CISA 3.0+ functions. It requires the
///   function ID to be passed in as an argument as well.
///
std::string printFunction(
    const common_isa_header& isaHeader,
    const function_format_t* header,
    const std::list<CISA_INST*>& instructions,
    int funcionId,
    const Options *opt);

/// Exposing these declare print functions for use by verifier diagnostics code
/// or for disassembly output.
std::string printPredicateDecl(
    const print_format_provider_t* header,
    unsigned declID);
std::string printVariableDecl (
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    unsigned declID,
    bool isKernel,
    unsigned int funcId,
    const Options *options);
std::string printAddressDecl(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    unsigned declID);
std::string printSamplerDecl(
    const print_format_provider_t* header,
    unsigned declID);
std::string printSurfaceDecl(
    const print_format_provider_t* header,
    unsigned declID,
    unsigned numPredefinedSurfaces);
std::string printFuncInput(
    const print_format_provider_t* header,
    unsigned declID,
    bool isKernel,
    const Options* options);
const char* printAsmName(
    const print_format_provider_t* header);
std::string printAttribute(
    const attribute_info_t* attr,
    const print_format_provider_t* kernel,
    bool isKernelAttr = false);

/// Exposing these for inline asm code generation
std::string printVectorOperand(
    const print_format_provider_t* header,
    const vector_opnd& opnd,
    const Options *opt,
    bool showRegion);


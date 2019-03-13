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

#include <list>
#include <string>
#include "Common_ISA.h"

extern std::string printKernelHeader(const common_isa_header& isaHeader, const kernel_format_t* header, bool isKernel, int funcionId, Options *opt);
///
/// - Takes an isa header, a kernel/function header, and isa instruction and returns a string contraining the instruction's isaasm.
///
extern std::string printInstruction(const kernel_format_t* header, const CISA_INST* instruction, Options *opt);

///
/// - Takes a isa header, a kernel header, and a instruction lists and returns a string contraining the kernel's entire isaasm.
///
extern std::string printKernel(const common_isa_header& isaHeader, const kernel_format_t* header, std::list<CISA_INST*>& instructions, Options *opt);

///
/// - Similar to printKernel() but for CISA 3.0+ functions. It requires the function ID to be passed in as an argument as well.
///
extern std::string printFunction(const common_isa_header& isaHeader, const function_format_t* header, std::list<CISA_INST*>& instructions, int funcionId, Options *opt);

/// Exposing these declare print functions for use by verifier diagnostics code or for disassembly output.
extern std::string printPredicateDecl(const kernel_format_t* header, unsigned declID);
extern std::string printVariableDecl (const common_isa_header& isaHeader, const kernel_format_t* header, unsigned declID, bool isKernel, unsigned int funcId, Options *options);
extern std::string printAddressDecl  (const common_isa_header& isaHeader, const kernel_format_t* header, unsigned declID);
extern const char* printAsmName(const kernel_format_t* header);


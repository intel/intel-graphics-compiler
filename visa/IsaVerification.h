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

#include <list>
#include <string>
#include "Common_ISA.h"

#define ERROR_LIST_TYPE std::list<std::string>

#define KERROR_LIST std::list<std::string>& kerror_list
#define ERROR_LIST std::list<std::string>& error_list

void verifyKernelHeader(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    ERROR_LIST,
    Options *options);

void verifyRoutine(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    std::list<CISA_INST*>& instructions,
    ERROR_LIST,
    KERROR_LIST,
    Options *options);

void verifyInstruction(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const CISA_INST* inst,
    ERROR_LIST,
    Options *options);

void writeReport(const char* filename, ERROR_LIST, KERROR_LIST);


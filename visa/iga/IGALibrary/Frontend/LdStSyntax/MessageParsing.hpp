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
#ifndef IGA_FRONTEND_LDSTSYNTAX_MESSAGEPARSING_HPP
#define IGA_FRONTEND_LDSTSYNTAX_MESSAGEPARSING_HPP
#include "../KernelParser.hpp"

#include <string>

namespace iga
{
    // returns false only if no input processed; this is a clue that the
    // calling parser should probably favor a more generic "invalid mnemonic"
    // message
    //
    // will raise a ParseException if the message is not mapped or some other
    // error
    //
    // returns true on succces
    bool ParseLdStInst(ExecSize dftExecSize, GenParser &kp);
}

#endif // IGA_FRONTEND_LDSTSYNTAX_MESSAGPARSING_HPP

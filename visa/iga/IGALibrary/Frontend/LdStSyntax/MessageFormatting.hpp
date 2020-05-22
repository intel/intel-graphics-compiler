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
#ifndef IGA_FRONTEND_LDSTSYNTAX_MESSAGEFORMATTING_HPP
#define IGA_FRONTEND_LDSTSYNTAX_MESSAGEFORMATTING_HPP

#include "../../IR/Instruction.hpp"
#include "../../Models/Models.hpp"

#include <string>
#include <vector>

namespace iga
{
    struct FormatResult {
        std::string opcode; // e.g. "ld.sc8.x4"
        std::string dst;    // e.g. ""r12-14:f" or "[r12-14]:f" (for ld and st)
        std::string src;    // e.g. as above, but flipped
        std::vector<const char *> instOpts; // any field that manifests as an instruction option

        std::string errMessage; // why formatting failed

        FormatResult &error(const char *msg) {errMessage = msg; return *this;}
        bool success() const {return errMessage.empty();}
    };

    // returns false if we cannot format as ld/st and errMessage has more info
    FormatResult FormatLdStInstruction(
        const Model &m,
        const Instruction &i);
}


#endif // IGA_FRONTEND_LDSTSYNTAX_MESSAGEFORMATTING_HPP

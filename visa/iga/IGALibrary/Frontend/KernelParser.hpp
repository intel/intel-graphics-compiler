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

#include "../IR/Types.hpp"
#include "../IR/Kernel.hpp"
#include "../IR/Loc.hpp"
#include "../ErrorHandler.hpp"

namespace iga {
    struct ParseOpts {
        // Enables certain IsaAsm-era directives
        //   .default_execution_size(...)
        //   .default_register_type(...)
        bool supportLegacyDirectives = false;
        // emits warnings about deprecated syntax
        bool deprecatedSyntaxWarnings = true;
        // sets the maximum number of fatal syntax errors allowable
        // before we give up on the parse
        size_t maxSyntaxErrors = 3;

        ParseOpts() { }
    };

    // The primary API for parsing GEN kernels
    Kernel *ParseGenKernel(
        const Model &model,
        const char *inp,
        ErrorHandler &e,
        const ParseOpts &popts = ParseOpts());
}


#endif
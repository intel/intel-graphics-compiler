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

#ifndef IGA_INSTDIFF_HPP
#define IGA_INSTDIFF_HPP

#include "IR/Types.hpp"
#include "api/iga.h"

#include <cstdint>
#include <ostream>

namespace iga
{
    // returns true if a warning or error was emitted
    iga_status_t DecodeFields(
        Platform p,
        bool useNativeDecoder,
        std::ostream &os,
        const uint8_t *bits,
        size_t bitsLen);

    // returns true if a warning or error was emitted
    iga_status_t DiffFields(
        Platform p,
        bool useNativeDecoder,
        std::ostream &os,
        const char *source1,
        const uint8_t *bits1,
        size_t bitsLen1,
        const char *source2,
        const uint8_t *bits2,
        size_t bitsLen2);
    iga_status_t DiffFieldsFromPCs(
        Platform p,
        bool useNativeDecoder,
        std::ostream &os,
        const char *source1,
        size_t pc1,
        const uint8_t *bits1,
        size_t bitsLen1,
        const char *source2,
        size_t pc2,
        const uint8_t *bits2,
        size_t bitsLen2);


    // returns true if a warning or error was emitted
    iga_status_t DebugCompaction(
        Platform p,
        bool useNativeDecoder,
        std::ostream &os,
        const uint8_t *bits,
        size_t bitsLen);
}

#endif // IGA_INSTDIFF_HPP

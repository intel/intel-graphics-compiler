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

#ifndef _IGA_LOC
#define _IGA_LOC
// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.

#include <cstdint>

namespace iga {

// a source location
struct Loc {
    uint32_t line;
    uint32_t col;
    uint32_t offset; // file offset
    uint32_t extent; // length (in characters/bytes)

    Loc() {}
    Loc(uint32_t pc)
        : line(0)
        , col(0)
        , offset(pc)
        , extent(0)
    {
    }
    Loc(uint32_t ln,
        uint32_t cl,
        uint32_t off,
        uint32_t len)
        : line(ln)
        , col(cl)
        , offset(off)
        , extent(len)
    {
    }


    bool isValid() const {
        return line != INVALID.line && col != INVALID.col;
    }
    // tells if the offset value is a PC or text offset
    bool isBinary() const {
        return line == 0 && col == 0;
    }
    bool isText() const {
        return !isBinary();
    }

    // bool operator==(const Loc &rhs) const {
    //    return this->line == rhs.line &&
    //        this->col == rhs.col &&
    //        this->offset == rhs.offset &&
    //        this->extent == rhs.extent;
    // }

    const static Loc INVALID;
};

} // iga::

#endif

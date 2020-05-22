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
#include "iga_bxml_enums.hpp"

#include <cassert>

using namespace iga;


unsigned iga::GetSourceCount(MathFC mfc)
{
    switch (mfc) {
    case MathFC::INV:  return 1;
    case MathFC::LOG:  return 1;
    case MathFC::EXP:  return 1;
    case MathFC::SQT:  return 1;
    case MathFC::RSQT: return 1;
    case MathFC::SIN:  return 1;
    case MathFC::COS:  return 1;
    case MathFC::FDIV: return 2;
    case MathFC::POW:  return 2;
    case MathFC::IDIV: return 2;
    case MathFC::IQOT: return 2;
    case MathFC::IREM: return 2;
    case MathFC::INVM: return 2;
    case MathFC::RSQTM: return 1;
    default:
        assert(0 && "iga_bxml_enums.cpp: iga::GetSourceCount: "
            "needs to define count of operands for this math subfunction");
        return (unsigned)-1;
    }
}





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

#ifndef _IGA_COLORED_IO
#define _IGA_COLORED_IO

#include <ostream>

namespace iga
{
    enum class Color {
        BLACK,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE
    };
    enum class Intensity {
        DULL,
        NORMAL,
        BRIGHT
    };
    enum class Reset {
        RESET
    };
} // namespace iga

std::ostream &operator <<(std::ostream &os, iga::Color);
std::ostream &operator <<(std::ostream &os, iga::Intensity);
std::ostream &operator <<(std::ostream &os, iga::Reset);

namespace iga {
    template <typename T>
    void emitColoredText(iga::Color c, std::ostream &os, const T &t)
    {
        os << c;
        os << Intensity::BRIGHT;
        os << t;
        os << Reset::RESET;
    }
    template <typename T>
    void emitRedText(std::ostream &os, const T &t)
    {
        emitColoredText(Color::RED, os, t);
    }
    template <typename T>
    void emitGreenText(std::ostream &os, const T &t)
    {
        emitColoredText(Color::GREEN, os, t);
    }
    template <typename T>
    void emitYellowText(std::ostream &os, const T &t)
    {
        emitColoredText(Color::YELLOW, os, t);
    }
}

#endif

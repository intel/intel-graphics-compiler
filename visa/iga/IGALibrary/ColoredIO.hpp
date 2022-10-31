/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_COLORED_IO
#define _IGA_COLORED_IO

#include <ostream>

namespace iga {
enum class Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };
enum class Intensity { DULL, NORMAL, BRIGHT };
enum class Reset { RESET };
} // namespace iga

std::ostream &operator<<(std::ostream &os, iga::Color);
std::ostream &operator<<(std::ostream &os, iga::Intensity);
std::ostream &operator<<(std::ostream &os, iga::Reset);

namespace iga {
template <typename T>
void emitColoredText(iga::Color c, std::ostream &os, const T &t) {
  os << c;
  os << Intensity::BRIGHT;
  os << t;
  os << Reset::RESET;
}
template <typename T> void emitRedText(std::ostream &os, const T &t) {
  emitColoredText(Color::RED, os, t);
}
template <typename T> void emitGreenText(std::ostream &os, const T &t) {
  emitColoredText(Color::GREEN, os, t);
}
template <typename T> void emitYellowText(std::ostream &os, const T &t) {
  emitColoredText(Color::YELLOW, os, t);
}
} // namespace iga

#endif

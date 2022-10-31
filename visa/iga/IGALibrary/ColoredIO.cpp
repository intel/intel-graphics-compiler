/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ColoredIO.hpp"
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // so std::min/std::max still work
#endif
#include <Windows.h>
#include <io.h>
#define IS_STDERR_TTY (_isatty(_fileno(stderr)) != 0)
#define IS_STDOUT_TTY (_isatty(_fileno(stdout)) != 0)
#else
#include <unistd.h>
#define IS_STDERR_TTY (isatty(STDERR_FILENO) != 0)
#define IS_STDOUT_TTY (isatty(STDOUT_FILENO) != 0)
#endif

using namespace iga;

static bool isTty(std::ostream &os) {
  return &os == &std::cerr   ? IS_STDERR_TTY
         : &os == &std::cout ? IS_STDOUT_TTY
                             : false;
}

static void reset(std::ostream &os) {
  if (!isTty(os)) {
    return;
  }
  os.flush();
#ifdef _WIN32
  HANDLE h =
      GetStdHandle(&os == &std::cerr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(h, &csbi)) {
    return;
  }
  WORD attrs = (csbi.wAttributes & 0xFFF0) | FOREGROUND_RED | FOREGROUND_GREEN |
               FOREGROUND_BLUE;
  (void)SetConsoleTextAttribute(h, attrs);
#else
  os << "\033[0m";
#endif
}

static void setAttrs(std::ostream &os, const Color *pC, const Intensity *pI) {
  if (!isTty(os)) {
    return;
  }
  os.flush();
#ifdef _WIN32
  HANDLE h =
      GetStdHandle(&os == &std::cerr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(h, &csbi)) {
    return;
  }

  WORD attrs =
      csbi.wAttributes & 0xFFF0; // retain most attributes including background
  if (pI) {
    switch (*pI) {
    case Intensity::BRIGHT:
      attrs |= FOREGROUND_INTENSITY;
      break;
    default:
      break; // dull and normal don't brighten
    }
  } else {
    // retain previous intensity
    attrs |= FOREGROUND_INTENSITY & csbi.wAttributes;
  }
  if (pC) {
    switch (*pC) {
    case Color::RED:
      attrs |= FOREGROUND_RED;
      break;
    case Color::GREEN:
      attrs |= FOREGROUND_GREEN;
      break;
    case Color::BLUE:
      attrs |= FOREGROUND_BLUE;
      break;
    case Color::YELLOW:
      attrs |= FOREGROUND_RED | FOREGROUND_GREEN;
      break;
    case Color::MAGENTA:
      attrs |= FOREGROUND_RED | FOREGROUND_BLUE;
      break;
    case Color::CYAN:
      attrs |= FOREGROUND_GREEN | FOREGROUND_BLUE;
      break;
    default:
      attrs |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
      break;
    }
  } else {
    attrs |= 0xF & csbi.wAttributes; // retain old foreground
  }
  // csbiAttrs = csbi.wAttributes;
  (void)SetConsoleTextAttribute(h, attrs);
#else
  if (pI) {
    switch (*pI) {
    case Intensity::BRIGHT:
      os << "\033[1m";
      break;
    default:
      os << "\033[2m";
      break;
    }
  } // else: retain previous intensity
  if (pC) {
    switch (*pC) {
    case Color::RED:
      os << "\033[31m";
      break;
    case Color::GREEN:
      os << "\033[32m";
      break;
    case Color::YELLOW:
      os << "\033[33m";
      break;
    case Color::BLUE:
      os << "\033[34m";
      break;
    case Color::MAGENTA:
      os << "\033[35m";
      break;
    case Color::CYAN:
      os << "\033[36m";
      break;
    default:
      os << "\033[37m";
      break;
    }
  }
#endif
}

std::ostream &operator<<(std::ostream &os, Color c) {
  setAttrs(os, &c, nullptr);
  return os;
}
std::ostream &operator<<(std::ostream &os, Intensity i) {
  setAttrs(os, nullptr, &i);
  return os;
}
std::ostream &operator<<(std::ostream &os, Reset) {
  reset(os);
  return os;
}

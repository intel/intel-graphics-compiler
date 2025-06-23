/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "system.hpp"
#include "asserts.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // omit min()/max() macros (favor std::min/std::max)
#endif
#include <Windows.h>
#include <io.h>
#define IS_STDERR_TTY (_isatty(_fileno(stderr)) != 0)
#define IS_STDOUT_TTY (_isatty(_fileno(stdout)) != 0)
#include <fcntl.h>
#else
#define IS_STDERR_TTY (isatty(STDERR_FILENO) != 0)
#define IS_STDOUT_TTY (isatty(STDOUT_FILENO) != 0)
#include <errno.h>
#include <string.h> // strerror_r
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace iga;

#ifdef _WIN32
static void EnableColoredIO() {
  static bool enabled = false;
  if (enabled)
    return;
// TODO: should only need to do this on Windows 10 Threshold 2 (TH2),
// "November Update": version 1511 and has the build number 10586
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
  auto enableOnHandle = [](DWORD H_CODE) {
    DWORD mode;
    HANDLE h = GetStdHandle(H_CODE);
    GetConsoleMode(h, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(h, mode);
  };
  enableOnHandle(STD_ERROR_HANDLE);
  enableOnHandle(STD_OUTPUT_HANDLE);
  enabled = true;
}

// create a static constructor
struct dummy {
  dummy() { EnableColoredIO(); };
};
static dummy _dummy;

#else
// unix
//
// nothing needed here
#endif

bool iga::IsStdoutTty() { return IS_STDOUT_TTY; }

bool iga::IsStderrTty() { return IS_STDERR_TTY; }

bool iga::IsTty(const std::ostream &os) {
  if (&os == &std::cout)
    return IsStdoutTty();
  else if (&os == &std::cerr)
    return IsStderrTty();
  return false;
}

void iga::SetStdinBinary() {
#ifdef _WIN32
  (void)_setmode(_fileno(stdin), _O_BINARY);
  // #else Linux doesn't make a distinction
#endif
}

bool iga::DoesFileExist(const std::string &path) {
#ifdef _WIN32
  DWORD dwAttrib = GetFileAttributesA(path.c_str());
  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
          !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat sb = {0};
  if (stat(path.c_str(), &sb) != 0) {
    return false;
  }
  return S_ISREG(sb.st_mode);
#endif
}

// Use the color API's below.
//   emitRedText(std::ostream&,const T&)
//   emit###Text(std::ostream&,const T&)
//
// An RAII type used by the above APIs.
// Use this within a scope where you want colored text.
// The destructor restores or resets the state (even upon exception exit
// out of the scope.
//
// Note, the non-Windows version of this stomps (ANSI reset)'s the terminal.
// If stderr is not a TTY, this has no effect.
//
struct StreamColorSetter {
  enum Color { RED, GREEN, YELLOW };
  std::ostream &stream;

  bool isStderr() const { return &stream == &std::cerr; }
  bool isStdout() const { return &stream == &std::cout; }
  bool isTty() const { return iga::IsTty(stream); }

  void setColor(StreamColorSetter::Color c) {
    if (colorNeedsRestore || !isTty() || (!isStderr() && !isStdout())) {
      return;
    }
    stream.flush();
#ifdef _WIN32
    HANDLE h = GetStdHandle(isStderr() ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    WORD w = FOREGROUND_INTENSITY;
    if (c == StreamColorSetter::Color::RED) {
      w |= FOREGROUND_RED;
    } else if (c == StreamColorSetter::Color::GREEN) {
      w |= FOREGROUND_GREEN;
    } else {
      w |= FOREGROUND_RED | FOREGROUND_GREEN;
    }
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(h, &csbi)) {
      return;
    }
    csbiAttrs = csbi.wAttributes;
    if (!SetConsoleTextAttribute(h, w)) {
      // failed to set
      return;
    }
#else
    if (c == StreamColorSetter::Color::RED) {
      stream << "\033[31;1m"; // ESC[ 31 means red, 1 means intense
    } else if (c == StreamColorSetter::Color::GREEN) {
      stream << "\033[32;1m"; // green
    } else {
      stream << "\033[33;1m"; // yellow
    }
#endif
    colorNeedsRestore = true;
  }
  void restoreColor() {
    if (colorNeedsRestore) {
#ifdef _WIN32
      // restore color attributes
      (void)SetConsoleTextAttribute(
          GetStdHandle(isStderr() ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
          csbiAttrs);
#else
      // ANSI reset; techinally not consistent with Windows impl. which
      // restores the old attributes; this clobbers them all
      stream << "\033[0m";
      stream.flush();
      colorNeedsRestore = false;
#endif
    }
  }

  // requires explicit setting
  StreamColorSetter(std::ostream &os) : stream(os), colorNeedsRestore(false) {}
  // autosets the color in the constructor
  StreamColorSetter(std::ostream &os, StreamColorSetter::Color c)
      : stream(os), colorNeedsRestore(false) {
    setColor(c);
  }
  ~StreamColorSetter() {
    stream.flush();
    restoreColor();
  }
  StreamColorSetter(const StreamColorSetter&) = delete;
  StreamColorSetter& operator=(const StreamColorSetter&) = delete;

  bool colorNeedsRestore;
#ifdef _WIN32
  WORD csbiAttrs;
#endif
};

void iga::EmitRedText(std::ostream &os, const std::string &s) {
  StreamColorSetter scs(os, StreamColorSetter::RED);
  os << s;
}
void iga::EmitGreenText(std::ostream &os, const std::string &s) {
  StreamColorSetter scs(os, StreamColorSetter::GREEN);
  os << s;
}
void iga::EmitYellowText(std::ostream &os, const std::string &s) {
  StreamColorSetter scs(os, StreamColorSetter::YELLOW);
  os << s;
}

unsigned iga::LastError() {
#ifdef _WIN32
  return (unsigned)GetLastError();
#else
  return (unsigned)errno;
#endif // _WIN32
}

static void add_strerror_r_to_error(char *errMsg,
                                    char *strerror_r_return_value) {
  errMsg = strerror_r_return_value;
}

std::string iga::FormatLastError(unsigned errCode) {
  std::string msg;
  char buf[256]{0};
  char *errMsg = &buf[0];
#ifdef _WIN32
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, errCode, 0, (LPTSTR)&errMsg, sizeof(buf), NULL);
  if (errMsg)
    msg = errMsg;
#else
  // Response to issue
  // https://github.com/intel/intel-graphics-compiler/issues/213
  auto strerror_r_return_value = strerror_r(errCode, buf, sizeof(buf));
  add_strerror_r_to_error(errMsg, strerror_r_return_value);
#endif // _WIN32
  if (errMsg == nullptr || errMsg[0] == 0)
    return "???";
  return std::string(errMsg);
}

std::string iga::FixupPath(const std::string &path) {
#ifdef _WIN32
  // Windows paths can be a mess due to the 260 MAX_PATH limit.
  // This fails in the guts on the Windows path expansion (of .) and
  // normalization of path component separates (/ to \) etc.
  // (You'd experience std::ofstream to just fail to open a file.)
  // We work around this here by normalizing and expanding the path to an
  // absolute path.
  //
  // The approach here is to expand the path to a normalized absolute path.
  // This is a bit pesky because GetFullPathNameA is limited to 260 chars.
  // So we must use GetFullPathNameW. We will.
  //  0. manually replace '/' with '\'
  //  1. convert the path to absolute using GetFullPathNameW
  //     (GetFullPathNameA is still limited to 260)
  //  2. convert back to UTF-8
  //  3. prefix a \\?\ so that CreateFile (under std::ofstream) won't
  //     attempt to expand any relative components and choke due to length.
  //
  // If someone can come up with something better please let me know.
  if (path.substr(0, 2) == "\\\\") {
    // Network or UNC path (e.g. \\?\...) can be ignored...
    return path;
  }

  std::string noSlashes = path;
  std::replace(noSlashes.begin(), noSlashes.end(), '/', '\\');
  const char *pathCstr = noSlashes.c_str();
  auto retMbtwc = MultiByteToWideChar(CP_ACP, 0, pathCstr, -1, nullptr, 0);
  if (retMbtwc <= 0) {
    auto err = GetLastError();
    std::stringstream ess;
    ess << "iga::FixupPath:MultiByteToWideChar: " << err;
    IGA_FATAL(ess.str().c_str());
  }
  std::vector<wchar_t> wPath;
  wPath.resize(retMbtwc);
  MultiByteToWideChar(CP_ACP, 0, pathCstr, -1, wPath.data(), (int)wPath.size());

  std::wstring absPathW;
  wchar_t wbuf[256];
  auto retGfpn = GetFullPathNameW(wPath.data(),
                                  (unsigned)sizeof(wbuf) / sizeof(wbuf[0]) - 1,
                                  wbuf, nullptr);
  if (retGfpn == 0) {
    auto err = GetLastError();
    std::stringstream ess;
    ess << "iga::FixupPath:GetFullPathNameW: " << err;
    IGA_FATAL(ess.str().c_str());
    return "";
  } else if (retGfpn >= sizeof(wbuf) / sizeof(wbuf[0]) - 1) {
    std::vector<wchar_t> vec;
    vec.resize(retGfpn + 1);
    retGfpn = GetFullPathNameW(wPath.data(), (unsigned)vec.size() - 1,
                               vec.data(), nullptr);
    absPathW = vec.data();
  } else {
    absPathW = wbuf;
  }
  int sizeNeeded = WideCharToMultiByte(
      CP_ACP, 0, &absPathW[0], (int)absPathW.size(), NULL, 0, NULL, NULL);
  std::string absPathA(sizeNeeded, 0);
  WideCharToMultiByte(CP_ACP, 0, &absPathW[0], (int)absPathW.size(),
                      &absPathA[0], sizeNeeded, NULL, NULL);
  absPathA = "\\\\?\\" + absPathA;
  return absPathA;
#else
  return path;
#endif // _WIN32
}

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
#ifndef _IO_HPP_
#define _IO_HPP_

#ifdef _WIN32
// for doesFileExist()
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include <iostream>
#include <locale>
#include <vector>

#include "fatal.hpp"

#ifdef _WIN32
#include <io.h>
#define IS_STDERR_TTY (_isatty(_fileno(stderr)) != 0)
#define IS_STDOUT_TTY (_isatty(_fileno(stdout)) != 0)
#else
#include <unistd.h>
#define IS_STDERR_TTY (isatty(STDERR_FILENO) != 0)
#define IS_STDOUT_TTY (isatty(STDOUT_FILENO) != 0)
#endif

static void readBinaryStream(
    const char *streamName,
    std::istream &is,
    std::vector<unsigned char> &bin)
{
    bin.clear();
    is.clear();
    while (is.good()) {
        int chr;
        if ((chr = is.get()) == EOF) {
            return;
        }
        bin.push_back((char)chr);
    }
    fatalExitWithMessage("iga: error reading %s", streamName);
}

static void readBinaryFile(
    const char *fileName,
    std::vector<unsigned char> &bin)
{
    std::ifstream is(fileName, std::ios::binary);
    if (!is.is_open()) {
        fatalExitWithMessage("iga: %s: failed to open file", fileName);
    }
    readBinaryStream(fileName, is, bin);
}

static std::string readTextStream(
    const char *streamName,
    std::istream &is)
{
    std::string s;
    is.clear();
    s.append(std::istreambuf_iterator<char>(is),
             std::istreambuf_iterator<char>());
    if (!is.good()) {
        fatalExitWithMessage("iga: error reading %s", streamName);
    }
    return s;
}

static std::string readTextFile(
    const char *fileName)
{
    std::ifstream file(fileName);
    if (!file.good()) {
        fatalExitWithMessage("iga: %s: failed to open file", fileName);
    }
    return readTextStream(fileName,file);
}

static void writeTextStream(
    const char *streamName,
    std::ostream &os,
    const char *output,
    size_t outputLength)
{
    os.clear();
    os << output;
    if (!os.good()) {
        fatalExitWithMessage("iga: error writing %s", streamName);
    }
}

#if 0
static void writeTextStreamF(
    const char *streamName,
    FILE *stream,
    const char *output,
    size_t outputLength)
{
    if (fwrite(output, 1, outputLength, stream) != outputLength) {
        fatalExitWithMessage("iga: error writing %s", streamName);
    }
// This also works...
//    clearerr(stream);
//    fputs(output, stream);
//    if (ferror(stream)) {
//        fatalExitWithMessage("iga: error writing %s", streamName);
//    }
}
#endif

static void writeTextFile(
    const char *fileName,
    const char *output,
    size_t outputLength)
{
    std::ofstream file(fileName);
    if (!file.good()) {
        fatalExitWithMessage("iga: %s: failed to open file", fileName);
    }
    writeTextStream(fileName, file, output, outputLength);
}

static void writeBinaryStream(
    const char *streamName,
    std::ostream &os,
    const void *bits,
    size_t bitsLen)
{
    os.clear();
    os.write((const char *)bits,bitsLen);
    if (!os.good()) {
        fatalExitWithMessage("iga: error writing %s", streamName);
    }
}

static void writeBinaryFile(
  const char *fileName,
  const void *bits,
  size_t bitsLen)
{
    std::ofstream file(fileName,std::ios::binary);
    if (!file.good()) {
        fatalExitWithMessage("iga: %s: failed to open file", fileName);
    }
    writeBinaryStream(fileName,file,bits,bitsLen);
}

static bool doesFileExist(const char *fileName) {
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesA(fileName);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat sb = {0};
    if (stat(fileName,&sb) != 0) {
        fatalExitWithMessage("iga: %s: failed to stat file", fileName);
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

    bool isStderr() const {
        return &stream == &std::cerr;
    }
    bool isStdout() const {
        return &stream == &std::cout;
    }
    bool isTty() const {
        return isStderr() ? IS_STDERR_TTY :
                isStdout() ? IS_STDOUT_TTY :
                false;
    }

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
    StreamColorSetter(std::ostream &os)
        : stream(os)
        , colorNeedsRestore(false)
    {
    }
    // autosets the color in the constructor
    StreamColorSetter(std::ostream &os, StreamColorSetter::Color c)
        : stream(os)
        , colorNeedsRestore(false)
    {
        setColor(c);
    }
    ~StreamColorSetter() {
        stream.flush();
        restoreColor();
    }

    bool colorNeedsRestore;
#ifdef _WIN32
    WORD csbiAttrs;
#endif
};


template <typename T>
void emitRedText(std::ostream &os, const T &t)
{
    StreamColorSetter scs(os, StreamColorSetter::RED);
    os << t;
}
template <typename T>
void emitGreenText(std::ostream &os, const T &t)
{
    StreamColorSetter scs(os, StreamColorSetter::GREEN);
    os << t;
}
template <typename T>
void emitYellowText(std::ostream &os, const T &t)
{
    StreamColorSetter scs(os, StreamColorSetter::YELLOW);
    os << t;
}
#endif // _IO_HPP_

/*========================== begin_copyright_notice ============================

Copyright (c) 2017-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef _IO_HPP_
#define _IO_HPP_

#include <iostream>
#include <fstream>
#include <iostream>
#include <locale>
#include <vector>

#include "fatal.hpp"
#include "system.hpp"


static inline void readBinaryStream(
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
    fatalExitWithMessage(streamName, ": error reading ");
}

#define IGA_STDIN_FILENAME std::string("std::cin")

static inline std::vector<unsigned char> readBinaryStreamStdin()
{
    iga::SetStdinBinary();
    std::vector<unsigned char> bits;
    readBinaryStream("stdin", std::cin, bits);
    return bits;
}

static inline void readBinaryFile(
    const char *fileName, std::vector<unsigned char> &bin)
{
    std::ifstream is(fileName, std::ios::binary);
    if (!is.is_open()) {
        fatalExitWithMessage(fileName, ": failed to open file");
    }
    readBinaryStream(fileName, is, bin);
}

static inline std::string readTextStream(
    const char *streamName,
    std::istream &is)
{
    std::string s;
    is.clear();
    s.append(std::istreambuf_iterator<char>(is),
             std::istreambuf_iterator<char>());
    if (!is.good()) {
        fatalExitWithMessage(streamName, ": error reading");
    }
    return s;
}

static inline std::string readTextFile(
    const char *fileName)
{
    std::ifstream file(fileName);
    if (!file.good()) {
        fatalExitWithMessage(fileName, ": failed to open file");
    }
    return readTextStream(fileName,file);
}

static inline void writeTextStream(
    const char *streamName, std::ostream &os, const char *output)
{
    os.clear();
    os << output;
    if (!os.good()) {
        fatalExitWithMessage(streamName, ": error writing");
    }
}

static inline void writeTextFile(const char *fileName, const char *output)
{
    std::ofstream file(fileName);
    if (!file.good()) {
        fatalExitWithMessage(fileName, ": failed to open file");
    }
    writeTextStream(fileName, file, output);
}

static inline void writeBinaryStream(
    const char *streamName,
    std::ostream &os,
    const void *bits,
    size_t bitsLen)
{
    os.clear();
    os.write((const char *)bits, bitsLen);
    if (!os.good()) {
        fatalExitWithMessage(streamName, ": error writing stream");
    }
}

static inline void writeBinaryFile(
  const char *fileName,
  const void *bits,
  size_t bitsLen)
{
    std::ofstream file(fileName,std::ios::binary);
    if (!file.good()) {
        fatalExitWithMessage(fileName, ": failed to open file");
    }
    writeBinaryStream(fileName,file,bits,bitsLen);
}

static inline bool doesFileExist(const char *fileName) {
    return iga::DoesFileExist(fileName);
}



template <typename T>
void emitRedText(std::ostream &os, const T &t)
{
    std::stringstream ss;
    ss << t;
    iga::EmitRedText(os, ss.str());
}
template <typename T>
void emitGreenText(std::ostream &os, const T &t)
{
    std::stringstream ss;
    ss << t;
    iga::EmitGreenText(os, ss.str());
}
template <typename T>
void emitYellowText(std::ostream &os, const T &t)
{
    std::stringstream ss;
    ss << t;
    iga::EmitYellowText(os, ss.str());
}
#endif // _IO_HPP_

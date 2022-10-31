/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IO_HPP_
#define _IO_HPP_

#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <vector>

#include "fatal.hpp"
#include "system.hpp"

static inline void readBinaryStream(const std::string &streamName,
                                    std::istream &is,
                                    std::vector<unsigned char> &bin) {
  bin.clear();
  is.clear();
  while (is.good()) {
    int chr;
    if ((chr = is.get()) == EOF) {
      return;
    }
    bin.push_back((char)chr);
  }
  fatalExitWithMessage(streamName, ": error reading  (", iga::LastErrorString(),
                       ")");
}

#define IGA_STDIN_FILENAME std::string("std::cin")

static inline std::vector<unsigned char> readBinaryStreamStdin() {
  iga::SetStdinBinary();
  std::vector<unsigned char> bits;
  readBinaryStream("stdin", std::cin, bits);
  return bits;
}

static inline void readBinaryFile(const std::string &fileName,
                                  std::vector<unsigned char> &bin) {
  std::ifstream is(fileName, std::ios::binary);
  if (!is.good()) {
    fatalExitWithMessage(fileName, ": failed to open file (",
                         iga::LastErrorString(), ")");
  }
  readBinaryStream(fileName, is, bin);
}

static inline std::string readTextStream(const std::string &streamName,
                                         std::istream &is) {
  std::string s;
  is.clear();
  s.append(std::istreambuf_iterator<char>(is),
           std::istreambuf_iterator<char>());
  if (!is.good()) {
    fatalExitWithMessage(streamName, ": error reading (",
                         iga::LastErrorString(), ")");
  }
  return s;
}

static inline std::string readTextFile(const std::string &fileName) {
  std::ifstream file(fileName);
  if (!file.good()) {
    fatalExitWithMessage(fileName, ": failed to open file (",
                         iga::LastErrorString(), ")");
  }
  return readTextStream(fileName, file);
}

static inline void writeTextStream(const std::string &streamName,
                                   std::ostream &os, const char *output) {
  os.clear();
  os << output;
  if (!os.good()) {
    fatalExitWithMessage(streamName, ": error writing (",
                         iga::LastErrorString(), ")");
  }
}

static inline void writeTextFile(const std::string &fileName,
                                 const char *output) {
  std::ofstream file(fileName);
  if (!file.good()) {
    fatalExitWithMessage(fileName, ": failed to open file (",
                         iga::LastErrorString(), ")");
  }
  writeTextStream(fileName, file, output);
}

static inline void writeBinaryStream(const std::string &streamName,
                                     std::ostream &os, const void *bits,
                                     size_t bitsLen) {
  os.clear();
  os.write((const char *)bits, bitsLen);
  if (!os.good()) {
    fatalExitWithMessage(streamName, ": error writing stream (",
                         iga::LastErrorString(), ")");
  }
}

static inline void writeBinaryFile(const std::string &fileName,
                                   const void *bits, size_t bitsLen) {
  std::ofstream file(fileName, std::ios::binary);
  if (!file.good()) {
    fatalExitWithMessage(fileName, ": failed to open file (",
                         iga::LastErrorString(), ")");
  }
  writeBinaryStream(fileName, file, bits, bitsLen);
}

static inline bool doesFileExist(const std::string &fileName) {
  return iga::DoesFileExist(fileName);
}

template <typename T> void emitRedText(std::ostream &os, const T &t) {
  std::stringstream ss;
  ss << t;
  iga::EmitRedText(os, ss.str());
}
template <typename T> void emitGreenText(std::ostream &os, const T &t) {
  std::stringstream ss;
  ss << t;
  iga::EmitGreenText(os, ss.str());
}
template <typename T> void emitYellowText(std::ostream &os, const T &t) {
  std::stringstream ss;
  ss << t;
  iga::EmitYellowText(os, ss.str());
}
#endif // _IO_HPP_

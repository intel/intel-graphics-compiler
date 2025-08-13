/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <sstream>

namespace Util {

class BinaryStream {
public:
  BinaryStream();
  ~BinaryStream();

  BinaryStream(const BinaryStream &) = delete;
  BinaryStream &operator=(const BinaryStream &) = delete;

  bool Write(const char *s, std::streamsize n);

  bool Write(const BinaryStream &in);

  template <class T> bool Write(const T &in);

  bool WriteAt(const char *s, std::streamsize n, std::streamsize loc);

  template <class T> bool WriteAt(const T &in, std::streamsize loc) {
    return WriteAt((const char *)&in, sizeof(T), loc);
  }

  bool Align(std::streamsize alignment);
  bool AddPadding(std::streamsize padding);

  const char *GetLinearPointer();

  std::streamsize Size() const;
  std::streamsize Size();

private:
  std::stringstream m_membuf;

  std::string m_LinearPointer;
};

template <class T> bool BinaryStream::Write(const T &in) { return Write((const char *)&in, sizeof(T)); }

} // namespace Util

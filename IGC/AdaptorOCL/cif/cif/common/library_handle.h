/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <memory>
#include <string>

namespace CIF {

class LibraryHandle {
public:
  virtual ~LibraryHandle() = default;

  LibraryHandle(const LibraryHandle &) = delete;
  LibraryHandle(LibraryHandle &&) = delete;
  LibraryHandle &operator=(const LibraryHandle &) = delete;
  LibraryHandle &operator=(LibraryHandle &&) = delete;

  virtual void *GetFuncPointer(const std::string &funcName) const = 0;

protected:
  LibraryHandle() = default;
};

// generic open
// for OS-specific versions, please refer to OS_NAME/LibaryHandle.h headers
std::unique_ptr<LibraryHandle> OpenLibrary(const std::string &path, bool addOsSpecificExtensionToPath);
std::unique_ptr<LibraryHandle> OpenLibrary(const std::wstring &path, bool addOsSpecificExtensionToPath);
}

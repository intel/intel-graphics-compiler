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

#pragma once
#include <Windows.h>

#include "cif/common/library_handle.h"

namespace CIF {

void RAIIReleaseHelper(HMODULE mod) {
  if (NULL == mod) {
    return;
  }
  FreeLibrary(mod);
}

using UniquePtrHMODULE_t = std::unique_ptr<std::remove_pointer<HMODULE>::type,
                                           decltype(&RAIIReleaseHelper)>;
inline UniquePtrHMODULE_t UniquePtrHMODULE(HMODULE mod) {
  return UniquePtrHMODULE_t(mod, RAIIReleaseHelper);
}

class WinLibraryHandle : public LibraryHandle {
public:
  WinLibraryHandle(HMODULE module, bool ownsHandle)
      : module(module), ownsHandle(ownsHandle) {}
  ~WinLibraryHandle() override {
    if (ownsHandle && (NULL != module)) {
      FreeLibrary(module);
    }
  }
  void *GetFuncPointer(const std::string &funcName) const override {
    if (module == NULL) {
      return nullptr;
    }

    return GetProcAddress(module, funcName.c_str());
  }

protected:
  HMODULE module;
  bool ownsHandle;
};

// open based on existing handle - takes ownership over handle
inline std::unique_ptr<LibraryHandle> OpenLibrary(UniquePtrHMODULE_t module) {
  std::unique_ptr<LibraryHandle> ret;
  ret.reset(new WinLibraryHandle(module.release(), true));
  return ret;
}

// open based on existing handle - does not take ownership over handle
inline std::unique_ptr<LibraryHandle> OpenLibrary(HMODULE module) {
  std::unique_ptr<LibraryHandle> ret;
  ret.reset(new WinLibraryHandle(module, false));
  return ret;
}
}

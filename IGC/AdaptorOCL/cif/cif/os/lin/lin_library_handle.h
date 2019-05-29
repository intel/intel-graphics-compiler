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

#pragma once
#include <dlfcn.h>
#include <memory>

#include "cif/common/library_handle.h"

namespace CIF {

void RAIIReleaseHelper(void * mod) noexcept {
  if (nullptr == mod) {
    return;
  }
  dlclose(mod);
}

using UniquePtrLibrary_t = std::unique_ptr<void,
                                           decltype(&RAIIReleaseHelper)>;
inline UniquePtrLibrary_t UniquePtrLibrary(void* mod) {
  return UniquePtrLibrary_t(mod, &CIF::RAIIReleaseHelper);
}

class LinLibraryHandle : public LibraryHandle {
public:
  LinLibraryHandle(void* module, bool ownsHandle)
      : module(module), ownsHandle(ownsHandle) {}
  ~LinLibraryHandle() override {
    if (ownsHandle && (nullptr != module)) {
      dlclose(module);
    }
  }
  void *GetFuncPointer(const std::string &funcName) const override {
    if (module == nullptr) {
      return nullptr;
    }

    return dlsym(module, funcName.c_str());
  }

protected:
  void* module;
  bool ownsHandle;
};

// open based on existing handle - takes ownership over handle
inline std::unique_ptr<LibraryHandle> OpenLibrary(UniquePtrLibrary_t module) {
  std::unique_ptr<LibraryHandle> ret;
  ret.reset(new LinLibraryHandle(module.release(), true));
  return ret;
}

// open based on existing handle - does not take ownership over handle
inline std::unique_ptr<LibraryHandle> OpenLibrary(void* module) {
  std::unique_ptr<LibraryHandle> ret;
  ret.reset(new LinLibraryHandle(module, false));
  return ret;
}
}

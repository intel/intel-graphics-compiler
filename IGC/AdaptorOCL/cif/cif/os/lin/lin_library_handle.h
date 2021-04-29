/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

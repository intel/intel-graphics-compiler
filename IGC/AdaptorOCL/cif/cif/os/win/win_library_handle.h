/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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

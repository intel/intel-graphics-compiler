/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cif/os/lin/lin_library_handle.h"

namespace CIF {

std::unique_ptr<LibraryHandle> OpenLibrary(const std::string &path, bool addOsSpecificExtensionToPath) {
  void *mod = nullptr;
  if(addOsSpecificExtensionToPath){
      mod = dlopen((path + ".so").c_str(), RTLD_NOW);
  }else{
      mod = dlopen(path.c_str(), RTLD_NOW);
  }
  return OpenLibrary(UniquePtrLibrary(mod));
}

std::unique_ptr<LibraryHandle> OpenLibrary(const std::wstring &path, bool addOsSpecificExtensionToPath) {
  std::string pathAsString(path.begin(), path.end());
  return OpenLibrary(pathAsString, addOsSpecificExtensionToPath);
}
}

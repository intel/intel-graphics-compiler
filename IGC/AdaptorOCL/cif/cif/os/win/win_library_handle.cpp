/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cif/os/win/win_library_handle.h"

namespace CIF {

std::unique_ptr<LibraryHandle> OpenLibrary(const std::string &path, bool addOsSpecificExtensionToPath) {
  HMODULE mod = NULL;
  if(addOsSpecificExtensionToPath){
      mod = LoadLibraryExA((path + ".dll").c_str(), NULL, 0);
  }else{
      mod = LoadLibraryExA(path.c_str(), NULL, 0);
  }
  return OpenLibrary(UniquePtrHMODULE(mod));
}

std::unique_ptr<LibraryHandle> OpenLibrary(const std::wstring &path, bool addOsSpecificExtensionToPath) {
  HMODULE mod = NULL;
  if(addOsSpecificExtensionToPath){
      mod = LoadLibraryExW((path + L".dll").c_str(), NULL, 0);
  }else{
      mod = LoadLibraryExW(path.c_str(), NULL, 0);
  }
  return OpenLibrary(UniquePtrHMODULE(mod));
}
}

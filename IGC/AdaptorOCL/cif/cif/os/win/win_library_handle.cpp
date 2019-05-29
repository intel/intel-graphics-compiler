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

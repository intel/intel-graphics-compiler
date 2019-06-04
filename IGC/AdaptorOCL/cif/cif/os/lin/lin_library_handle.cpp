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

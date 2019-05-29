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

#include "cif/common/cif.h"
#include "cif/import/library_api.h"
#include "cif/import/cif_main.h"

namespace CIF {

CIF::RAII::UPtr_t<CIFMain> OpenLibraryInterface(LibraryHandle &lib) {
  CIFMain *ret = nullptr;
  void *createMainFuncPtr = lib.GetFuncPointer(CIF::CreateCIFMainFuncName);
  if (createMainFuncPtr == nullptr) {
    return CIF::RAII::UPtr(ret = nullptr);
  }
  auto CreateCIFFunc =
      reinterpret_cast<CIF::CreateCIFMainFunc_t>(createMainFuncPtr);
  auto main = (*CreateCIFFunc)();
  return CIF::RAII::UPtr(main);
}

std::unique_ptr<CIFPackage>
OpenLibraryInterface(std::unique_ptr<CIF::LibraryHandle> &&lib) {
  if (lib.get() == nullptr) {
    return std::unique_ptr<CIFPackage>(nullptr);
  }
  auto entryPoint = OpenLibraryInterface(*lib.get());
  CIFPackage *pckg = new CIFPackage(std::move(entryPoint), std::move(lib));
  return std::unique_ptr<CIFPackage>(pckg);
}
}

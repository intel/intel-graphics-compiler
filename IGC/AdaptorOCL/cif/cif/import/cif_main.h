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

#include <memory>

#include "cif/common/library_handle.h"
#include "cif/common/cif.h"
#include "cif/common/cif_main.h"

namespace CIF {
struct CIFPackage {
  CIFPackage(CIF::RAII::UPtr_t<CIFMain> &&main, std::unique_ptr<LibraryHandle> lib)
      : lib(std::move(lib)), main(std::move(main)) {}
  ~CIFPackage() = default;
  CIFPackage(const CIFPackage &) = delete;
  CIFPackage &operator=(const CIFPackage &) = delete;
  CIFPackage(CIFPackage &&) = delete;
  CIFPackage *operator=(CIFPackage &&) = delete;

  CIFMain *operator->() { return main.get(); }

  CIFMain *operator->() const { return main.get(); }

  CIFMain *GetCIFMain() const { return main.get(); }

  bool IsValid() const { return (main.get() != nullptr); }

protected:
  // note : the order of members is important
  std::unique_ptr<CIF::LibraryHandle> lib;
  CIF::RAII::UPtr_t<CIFMain> main;
};

// helpers for opening CIF Main inferface over given library handle
CIF::RAII::UPtr_t<CIFMain> OpenLibraryInterface(CIF::LibraryHandle &lib);
std::unique_ptr<CIFPackage> OpenLibraryInterface(std::unique_ptr<CIF::LibraryHandle> &&lib);
}

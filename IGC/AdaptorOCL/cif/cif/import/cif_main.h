/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

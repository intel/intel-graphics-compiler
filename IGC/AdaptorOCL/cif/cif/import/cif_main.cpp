/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

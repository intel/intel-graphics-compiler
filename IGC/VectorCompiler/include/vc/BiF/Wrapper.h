/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_BIF_WRAPPER_H
#define VC_BIF_WRAPPER_H

#include "vc/BiF/RawData.h"

#include <llvm/ADT/StringRef.h>

namespace vc {
namespace bif {

enum class RawKind {
  PrintfCM32,
  PrintfCM64,
  PrintfOCL32,
  PrintfOCL64,
  PrintfZE32,
  PrintfZE64,
  Emulation,
  SPIRVBuiltins
};

inline llvm::StringRef getPrintfCM32RawData() {
  // FIXME: write a module for cmrt printf.
  return "";
}

inline llvm::StringRef getPrintfCM64RawData() {
  // FIXME: write a module for cmrt printf.
  return "";
}

inline llvm::StringRef getPrintfOCL32RawData() {
  return {reinterpret_cast<char *>(VCBiFPrintfOCL32RawData),
          VCBiFPrintfOCL32RawData_size};
}

inline llvm::StringRef getPrintfOCL64RawData() {
  return {reinterpret_cast<char *>(VCBiFPrintfOCL64RawData),
          VCBiFPrintfOCL64RawData_size};
}

inline llvm::StringRef getPrintfZE32RawData() {
  return {reinterpret_cast<char *>(VCBiFPrintfZE32RawData),
          VCBiFPrintfZE32RawData_size};
}

inline llvm::StringRef getPrintfZE64RawData() {
  return {reinterpret_cast<char *>(VCBiFPrintfZE64RawData),
          VCBiFPrintfZE64RawData_size};
}

inline llvm::StringRef getVCEmulationRawData(llvm::StringRef CPUStr) {
  return getVCEmulation64RawDataImpl(CPUStr);
}

inline llvm::StringRef getSPIRVBuiltinsRawData() {
  return {reinterpret_cast<char *>(VCSPIRVBuiltins64RawData),
          VCSPIRVBuiltins64RawData_size};
}

template <enum RawKind> llvm::StringRef getRawData();
template <enum RawKind>
llvm::StringRef getRawDataForArch(llvm::StringRef CPUStr);

template <>
llvm::StringRef getRawDataForArch<RawKind::Emulation>(llvm::StringRef CPUStr) {
  return getVCEmulationRawData(CPUStr);
}

template<>
llvm::StringRef getRawData<RawKind::PrintfOCL32>() {
  return getPrintfOCL32RawData();
}

template<>
llvm::StringRef getRawData<RawKind::PrintfOCL64>() {
  return getPrintfOCL64RawData();
}

template <> llvm::StringRef getRawData<RawKind::PrintfZE32>() {
  return getPrintfZE32RawData();
}

template <> llvm::StringRef getRawData<RawKind::PrintfZE64>() {
  return getPrintfZE64RawData();
}

template <> llvm::StringRef getRawData<RawKind::PrintfCM32>() {
  return getPrintfCM32RawData();
}

template <> llvm::StringRef getRawData<RawKind::PrintfCM64>() {
  return getPrintfCM64RawData();
}

template <> llvm::StringRef getRawData<RawKind::SPIRVBuiltins>() {
  return getSPIRVBuiltinsRawData();
}

} // namespace bif
} // namespace vc

#endif

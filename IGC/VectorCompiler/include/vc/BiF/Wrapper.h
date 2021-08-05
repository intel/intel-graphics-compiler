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
#ifdef IGC_VC_DISABLE_BIF
  return "";
#else  // IGC_VC_DISABLE_BIF
  return {reinterpret_cast<char *>(VCBiFPrintfOCL32RawData),
          VCBiFPrintfOCL32RawData_size};
#endif // IGC_VC_DISABLE_BIF
}

inline llvm::StringRef getPrintfOCL64RawData() {
#ifdef IGC_VC_DISABLE_BIF
  return "";
#else  // IGC_VC_DISABLE_BIF
  return {reinterpret_cast<char *>(VCBiFPrintfOCL64RawData),
          VCBiFPrintfOCL64RawData_size};
#endif // IGC_VC_DISABLE_BIF
}

inline llvm::StringRef getPrintfZE32RawData() {
#ifdef IGC_VC_DISABLE_BIF
  return "";
#else  // IGC_VC_DISABLE_BIF
  return {reinterpret_cast<char *>(VCBiFPrintfZE32RawData),
          VCBiFPrintfZE32RawData_size};
#endif // IGC_VC_DISABLE_BIF
}

inline llvm::StringRef getPrintfZE64RawData() {
#ifdef IGC_VC_DISABLE_BIF
  return "";
#else  // IGC_VC_DISABLE_BIF
  return {reinterpret_cast<char *>(VCBiFPrintfZE64RawData),
          VCBiFPrintfZE64RawData_size};
#endif // IGC_VC_DISABLE_BIF
}

inline llvm::StringRef getVCEmulationRawData() {
#ifdef IGC_VC_DISABLE_BIF
  return "";
#else  // IGC_VC_DISABLE_BIF
  return {reinterpret_cast<char *>(VCEmulation64RawData),
          VCEmulation64RawData_size};
#endif // IGC_VC_DISABLE_BIF
}

inline llvm::StringRef getSPIRVBuiltinsRawData() {
#ifdef IGC_VC_DISABLE_BIF
  return "";
#else  // IGC_VC_DISABLE_BIF
  return {reinterpret_cast<char *>(VCSPIRVBuiltins64RawData),
          VCSPIRVBuiltins64RawData_size};
#endif // IGC_VC_DISABLE_BIF
}

template <enum RawKind> llvm::StringRef getRawData();

template <> llvm::StringRef getRawData<RawKind::Emulation>() {
  return getVCEmulationRawData();
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

/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_SUPPORT_STATUSTRAITS_H
#define VC_SUPPORT_STATUSTRAITS_H

#include "vc/Support/StatusCode.h"

#include "llvm/ADT/StringRef.h"

namespace vc {

// There should be specialization for every error code listed in errc.
// Specialization should define:
//  * llvm::StringRef getMessage() // return description for error
template <errc Code> struct ErrorTraits;

template <> struct ErrorTraits<errc::dynamic_load_fail> {
  static llvm::StringRef getMessage() {
    return "failed to load dynamic library";
  }
};

template <> struct ErrorTraits<errc::symbol_not_found> {
  static llvm::StringRef getMessage() { return "symbol lookup error"; }
};

template <> struct ErrorTraits<errc::bad_spirv> {
  static llvm::StringRef getMessage() { return "bad spirv bitcode"; }
};

template <> struct ErrorTraits<errc::bad_bitcode> {
  static llvm::StringRef getMessage() { return "bad llvm bitcode"; }
};

template <> struct ErrorTraits<errc::invalid_module> {
  static llvm::StringRef getMessage() { return "module verification failed"; }
};

template <> struct ErrorTraits<errc::target_machine_not_created> {
  static llvm::StringRef getMessage() {
    return "target machine creation failed";
  }
};

template <> struct ErrorTraits<errc::not_vc_codegen> {
  static llvm::StringRef getMessage() {
    return "vc codegen path option was not specified";
  }
};

template <> struct ErrorTraits<errc::invalid_api_option> {
  static llvm::StringRef getMessage() { return "invalid api option"; }
};

template <> struct ErrorTraits<errc::invalid_internal_option> {
  static llvm::StringRef getMessage() { return "invalid internal option"; }
};

template <> struct ErrorTraits<errc::bif_load_fail> {
  static llvm::StringRef getMessage() {
    return "failed to load OCL BiF module";
  }
};

template <> struct ErrorTraits<errc::output_not_created> {
  static llvm::StringRef getMessage() { return "could not create output file"; }
};

} // namespace vc

#endif

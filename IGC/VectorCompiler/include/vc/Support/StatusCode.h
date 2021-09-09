/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_SUPPORT_STATUSCODE_H
#define VC_SUPPORT_STATUSCODE_H

#include <system_error>

namespace vc {

enum class errc {
  // DynamicLibrary::getPermanentLibrary failure.
  dynamic_load_fail = 1,

  // DynamicLibrary::getAddressOfSymbol failure.
  symbol_not_found,

  // Spirv read failure.
  bad_spirv,

  // Parse bitcode failure.
  bad_bitcode,

  // Module verification failure.
  invalid_module,

  // Target machine allocation failure.
  target_machine_not_created,

  // VC codegen not specified in options.
  not_vc_codegen,

  // Bad option in api options.
  invalid_api_option,

  // Bad option in internal options.
  invalid_internal_option,

  // Loading of OCL BiF module failed.
  bif_load_fail,

  // Output creation failure.
  output_not_created
};

const std::error_category &err_category() noexcept;

inline std::error_code make_error_code(vc::errc e) noexcept {
  return std::error_code(static_cast<int>(e), vc::err_category());
}

} // namespace vc

namespace std {
template <> struct is_error_code_enum<vc::errc> : std::true_type {};
} // namespace std

#endif

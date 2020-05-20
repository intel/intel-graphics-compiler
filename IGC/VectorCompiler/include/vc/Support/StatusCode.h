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

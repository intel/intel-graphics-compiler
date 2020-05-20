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

} // namespace vc

#endif

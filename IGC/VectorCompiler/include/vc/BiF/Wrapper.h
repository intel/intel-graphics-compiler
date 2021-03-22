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

#ifndef VC_BIF_WRAPPER_H
#define VC_BIF_WRAPPER_H

#include "vc/BiF/RawData.h"

#include <llvm/ADT/StringRef.h>

namespace vc {
namespace bif {

enum class RawKind {
  PrintfOCL32,
  PrintfOCL64,
};

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

template <enum RawKind> llvm::StringRef getRawData();

template<>
llvm::StringRef getRawData<RawKind::PrintfOCL32>() {
  return getPrintfOCL32RawData();
}

template<>
llvm::StringRef getRawData<RawKind::PrintfOCL64>() {
  return getPrintfOCL64RawData();
}

} // namespace bif
} // namespace vc

#endif

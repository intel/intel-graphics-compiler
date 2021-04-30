/*========================== begin_copyright_notice ============================

Copyright (c) 2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef VC_BIF_PRINTF_IFACE_H
#define VC_BIF_PRINTF_IFACE_H

namespace vc {
namespace bif {
namespace printf {

// Internal data will be transfered between implementation functions through
// <TransferDataSize x i32> vector.
inline constexpr int TransferDataSize = 4;

// Vector to pass some information about args to the init function.
namespace ArgsInfoVector {
enum Enum {
  NumTotal, // the number of args pssed to printf (except format string)
  Num64Bit, // the number of 64 bit and not pointer printf args (except format
            // string)
  NumPtr,   // the number of pointer args (those that are for "%p" specifier)
  NumStr,   // the number of string args (those that are for "%s" specifier)
  FormatStrSize, // the size of format string
  Size
};
} // namespace ArgsInfoVector

namespace ArgKind {
enum Enum { Char, Short, Int, Long, Float, Double, Pointer, String };
} // namespace ArgKind

// The arg is passed by 32-bit chunks.
namespace ArgData {
enum Enum { Low, High, Size };
} // namespace ArgData

} // namespace printf
} // namespace bif
} // namespace vc

#endif

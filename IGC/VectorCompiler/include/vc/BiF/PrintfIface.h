/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

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

/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// implicit_args structure/buffer description and utils.
///
//===----------------------------------------------------------------------===//

#ifndef VC_UTILS_GENX_IMPLICITARGSBUFFER_H
#define VC_UTILS_GENX_IMPLICITARGSBUFFER_H

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>

namespace llvm {
class StructType;
class PointerType;
class Value;
class Module;
} // namespace llvm

namespace vc {

// Indirect data heap pointer is provided in r0.0[31:PtrOffsetInR00]
constexpr unsigned PtrOffsetInR00 = 6;

namespace ImplicitArgs {

// An attribute that indicates that for the marked kernel implicit args buffer
// should be generated, e.g. kernel calls external function that uses some
// implicit argument.
inline const char KernelAttr[] = "RequiresImplArgsBuffer";

namespace Buffer {

namespace Indices {
enum Enum {
  StructSize,
  StructVersion,
  NumWorkDim,
  SIMDWidth,
  LocalSizeX,
  LocalSizeY,
  LocalSizeZ,
  GlobalSizeX,
  GlobalSizeY,
  GlobalSizeZ,
  PrintfBufferPtr,
  GlobalOffsetX,
  GlobalOffsetY,
  GlobalOffsetZ,
  LocalIDTablePtr,
  GroupCountX,
  GroupCountY,
  GroupCountZ,
  Size
};
} // namespace Indices

// The name of implicit arguments buffer structure.
inline const char TypeName[] = "vc.implicit.args.buf.type";

// Returns implicit arguments buffer type.
// The returned structure has name \p TypeName.
// The function returns existing type or creates a new one if it hasn't been
// created.
llvm::StructType &getType(llvm::Module &M);

// Returns the type of a pointer to implicit arguments buffer.
llvm::PointerType &getPtrType(llvm::Module &M);

// Inserts instructions to access a pointer to implicit arguments buffer.
// Returns the pointer value. Instructions are inserted via the provided IR
// builder.
llvm::Value &getPointer(llvm::IRBuilder<> &IRB);

// Inserts instructions that access requested buffer field.
// Arguments:
//    \p BufferPtr - a pointer to the whole buffer. Should be obtained via
//                   vc::ImplicitArgs::Buffer::getPointer interface.
//    \p FieldIdx - the index of the requested field.
//    \p IRB - IR builder to insert the instructions.
//    \p Name - a name of the returned value and a prefix for other constructed
//              value names.
llvm::Value &loadField(llvm::Value &BufferPtr, Indices::Enum FieldIdx,
                       llvm::IRBuilder<> &IRB, llvm::StringRef Name = "");

} // namespace Buffer

} // namespace ImplicitArgs

} // end namespace vc

#endif // VC_UTILS_GENX_IMPLICITARGSBUFFER_H

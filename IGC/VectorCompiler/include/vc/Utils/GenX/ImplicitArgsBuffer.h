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

#include <llvm/ADT/Twine.h>
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
                       llvm::IRBuilder<> &IRB, const llvm::Twine &Name = "");

} // namespace Buffer

// Local ID structure type description.
// LocalIDTablePtr field of the implicit args buffer points to the array of
// local ID structures or to the first element of this table (which is the
// same).
namespace LocalID {

// Indices of the structure fields.
namespace Indices {
enum Enum { X, Y, Z, Size };
} // namespace Indices

// The name of local ID structure type.
inline const char TypeName[] = "vc.ia.local.id.type";

// Returns local ID structure type.
// The returned structure has name \p TypeName.
// The function returns existing type or creates a new one if it hasn't been
// created.
llvm::StructType &getType(llvm::Module &M);

// Returns the type of a pointer to local ID structure.
llvm::PointerType &getPtrType(llvm::Module &M);

// Inserts instructions that acceess the pointer to local ID table (to the first
// element of this table).
// Arguments:
//    \p BufferPtr - a pointer to the implicit args buffer. Should be obtained
//                   via vc::ImplicitArgs::Buffer::getPointer interface.
//    \p IRB - IR builder to insert the instructions.
//    \p Name - a name of the returned value and a prefix for other constructed
//              value names.
llvm::Value &getBasePtr(llvm::Value &BufferPtr, llvm::IRBuilder<> &IRB,
                        const llvm::Twine &Name = "ia.local.id.base.ptr");

// Inserts instructions that access the pointer to the local ID structure that
// describes currently executing thread.
// Arguments:
//    \p BufferPtr - a pointer to the implicit args buffer. Should be obtained
//                   via vc::ImplicitArgs::Buffer::getPointer interface.
//    \p IRB - IR builder to insert the instructions.
//    \p Name - a name of the returned value and a prefix for other constructed
//              value names.
llvm::Value &getPointer(llvm::Value &BufferPtr, llvm::IRBuilder<> &IRB,
                        const llvm::Twine &Name = "ia.local.id.ptr");

// Inserts instructions that load a field from local ID structure.
// Arguments:
//    \p LIDStructPtr - a pointer to a local ID structure. Should be obtained
//                      via vc::ImplicitArgs::LocalID::getPointer interface.
//    \p FieldIdx - the index of the requested field.
//    \p IRB - IR builder to insert the instructions.
//    \p Name - a name of the returned value and a prefix for other constructed
//              value names.
llvm::Value &loadField(llvm::Value &LIDStructPtr, Indices::Enum FieldIdx,
                       llvm::IRBuilder<> &IRB,
                       const llvm::Twine &Name = "ia.local.id.field");

} // namespace LocalID

} // namespace ImplicitArgs

} // end namespace vc

#endif // VC_UTILS_GENX_IMPLICITARGSBUFFER_H

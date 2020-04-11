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

#ifndef GENINTRINSICS_H
#define GENINTRINSICS_H

#pragma once

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/Module.h"

#include "llvmWrapper/IR/Attributes.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm {

namespace GenISAIntrinsic {
  enum ID : unsigned {
    no_intrinsic = Intrinsic::num_intrinsics,
#define GET_INTRINSIC_ENUM_VALUES
#include "IntrinsicGenISA.gen"
#undef GET_INTRINSIC_ENUM_VALUES
    num_genisa_intrinsics
  };


  /// Intrinsic::getName(ID) - Return the LLVM name for an intrinsic, such as
  /// "llvm.ppc.altivec.lvx".
  std::string getName(ID id, ArrayRef<Type*> Tys = None);


  /// Intrinsic::getDeclaration(M, ID) - Create or insert an LLVM Function
  /// declaration for an intrinsic, and return it.
  ///
  /// The OverloadedTys parameter is for intrinsics with overloaded types
  /// (i.e., those using iAny, fAny, vAny, or iPTRAny).  For a declaration of
  /// an overloaded intrinsic, Tys must provide exactly one type for each
  /// overloaded type in the intrinsic in order of dst then srcs.
  ///
  /// For instance, consider the following overloaded function.
  ///    uint2 foo(size_t offset, int bar, const __global uint2 *p);
  ///    uint4 foo(size_t offset, int bar, const __global uint4 *p);
  /// Such a function has two overloaded type parameters: dst and src2.
  /// Thus the type array should two elements:
  ///    Type Ts[2]{int2, int2}: to resolve to the first instance.
  ///    Type Ts[2]{int4, int4}: to resolve to the second.
#if defined(ANDROID) || defined(__linux__)
  __attribute__ ((visibility ("default"))) Function *getDeclaration(Module *M, ID id, ArrayRef<Type*> OverloadedTys = None);
#else
  Function *getDeclaration(Module *M, ID id, ArrayRef<Type*> OverloadedTys = None);
#endif
  IGCLLVM::AttributeSet getGenIntrinsicAttributes(LLVMContext& C, GenISAIntrinsic::ID id);

  //Override of isIntrinsic method defined in Function.h
  inline const char * getGenIntrinsicPrefix() { return "llvm.genx."; }
  inline bool isIntrinsic(const Function *CF)
  {
      return (CF->getName().startswith(getGenIntrinsicPrefix()));
  }
  ID getIntrinsicID(const Function *F);

} // namespace GenISAIntrinsic

} // namespace llvm

#endif //GENINTRINSICS_H

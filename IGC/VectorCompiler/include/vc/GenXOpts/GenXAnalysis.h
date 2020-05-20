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
//
// This file declares some hooks that are injected into llvm analysis library
// files to make them work with genx related stuff.
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_GENX_ANALYSIS_H
#define LLVM_GENX_ANALYSIS_H

namespace llvm {

template <typename T> class ArrayRef;
class CallInst;
class Constant;
class DataLayout;
class Instruction;
class ImmutableCallSite;
class Type;
class Use;
class Value;

/// canConstantFoldGenXIntrinsic - Return true if it is even possible to fold
/// a call to the specified GenX intrinsic.
bool canConstantFoldGenXIntrinsic(unsigned IID);

/// ConstantFoldGenXIntrinsic - Attempt to constant fold a call to the
/// specified GenX intrinsic with the specified arguments, returning null if
/// unsuccessful.
Constant *ConstantFoldGenXIntrinsic(unsigned IID, Type *RetTy,
                                    ArrayRef<Constant *> Operands,
                                    ImmutableCallSite CS, const DataLayout *DL);

/// ConstantFoldGenX - Attempt to constant fold genx-related instruction (intrinsic).
/// This function tries to fold operands and then tries to fold instruction
/// itself. Returns nullptr if folding was unsuccessful.
Constant *ConstantFoldGenX(Instruction *I, const DataLayout &DL);

/// Given a GenX intrinsic and a set of arguments, see if we can fold the
/// result.
///
/// If this call could not be simplified returns null.
Value *SimplifyGenXIntrinsic(unsigned IID, Type *RetTy, Use *ArgBegin,
                             Use *ArgEnd);

/// Given a GenX related intruction, see if we can fold the
/// result. This function tries simplification and then constant folding.
///
/// If this instruction could not be simplified returns null.
Value *SimplifyGenX(CallInst *I);

} // end namespace llvm

#endif

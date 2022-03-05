/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_GLOBALVARIABLE_H
#define VC_UTILS_GENX_GLOBALVARIABLE_H

namespace llvm {
class GlobalVariable;
class Value;
class LoadInst;
} // namespace llvm

namespace vc {

// Not every global variable is a real global variable and should be eventually
// encoded as a global variable.
// GenX volatile, predefined vISA variables and printf indexed strings are
// exclusion for now.
// Printf strings should be already legalized to make it possible to use this
// function. So this function can only be used after GenXPrintfLegalization.
bool isRealGlobalVariable(const llvm::GlobalVariable &GV);

// Return the underlying global variable. Return nullptr if it does not exist.
llvm::GlobalVariable *getUnderlyingGlobalVariable(llvm::Value *V);
const llvm::GlobalVariable *getUnderlyingGlobalVariable(const llvm::Value *V);
llvm::GlobalVariable *getUnderlyingGlobalVariable(llvm::LoadInst *LI);
const llvm::GlobalVariable *
getUnderlyingGlobalVariable(const llvm::LoadInst *LI);

} // namespace vc

#endif // VC_UTILS_GENX_GLOBALVARIABLE_H

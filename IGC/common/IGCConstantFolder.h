/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_CONSTANT_FOLDER_H
#define IGC_CONSTANT_FOLDER_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/ConstantFolder.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/ConstantFolder.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

class IGCConstantFolder : public IGCLLVM::ConstantFolderBase
{
public:
    // TODO: Once the switch to LLVM 15 is complete, align the method names
    // with those in the base ConstantFolder class, i.e. migrate from
    // llvm::Constant* Create* to llvm::Value* Fold* infrastructure.
    llvm::Constant* CreateGradientXFine(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientYFine(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientX(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientY(llvm::Constant* C0) const;
    llvm::Constant* CreateRsq(llvm::Constant* C0) const;
    llvm::Constant* CreateRoundNE(llvm::Constant* C0) const;
    llvm::Constant* CreateFSat(llvm::Constant* C0) const;
    llvm::Constant* CreateFAdd(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateFMul(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateFPTrunc(llvm::Constant* C0, llvm::Type* dstType, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateHFToBF8Trunc(llvm::Constant* C0, llvm::Type* dstType, uint32_t roundingMode, bool saturate) const;
    llvm::Constant* CreateHFToHF8Trunc(llvm::Constant* C0, llvm::Type* dstType, uint32_t roundingMode, bool saturate) const;
    llvm::Constant* CreateBF8ToHF(llvm::Constant* C0) const;
    llvm::Constant* CreateHF8ToHF(llvm::Constant* C0) const;
    llvm::Constant* CreateUbfe(llvm::Constant* C0, llvm::Constant* C1, llvm::Constant* C2) const;
    llvm::Constant* CreateIbfe(llvm::Constant* C0, llvm::Constant* C1, llvm::Constant* C2) const;
    llvm::Constant* CreateCanonicalize(llvm::Constant* C0, bool flushDenorms = true) const;
    llvm::Constant* CreateFirstBitHi(llvm::Constant* C0) const;
    llvm::Constant* CreateFirstBitShi(llvm::Constant* C0) const;
    llvm::Constant* CreateFirstBitLo(llvm::Constant* C0) const;
    llvm::Constant* CreateBfi(llvm::Constant* C0, llvm::Constant* C1, llvm::Constant* C2, llvm::Constant* C3) const;
    llvm::Constant* CreateBfrev(llvm::Constant* C0) const;

private:

    llvm::Constant* CreateGradient(llvm::Constant* C0) const;

};

}

#endif // IGC_CONSTANT_FOLDER_H

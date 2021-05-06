/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
    class ModulePass;
}

llvm::ModulePass* createProcessFuncAttributesPass();

llvm::ModulePass* createProcessBuiltinMetaDataPass();

llvm::ModulePass* createInsertDummyKernelForSymbolTablePass();

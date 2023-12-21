/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <Compiler/CodeGenPublic.h>

namespace IGC {
// Class for splitting llvm::Module per kernel
// handles splitting, owns split module, handles proper cleanup
class KernelModuleSplitter
{
public:
    KernelModuleSplitter(IGC::OpenCLProgramContext& oclContext, llvm::Module& module);
    ~KernelModuleSplitter();
    KernelModuleSplitter(const KernelModuleSplitter&) = delete;
    KernelModuleSplitter& operator=(const KernelModuleSplitter&) = delete;
    void restoreOclContextModule();
    void setSplittedModuleInOCLContext();
    void retry();
    void splitModuleForKernel(const llvm::Function *kernelF);

private:
    IGC::OpenCLProgramContext& _oclContext;
    llvm::Module& _originalModule;
    std::unique_ptr<llvm::Module> _splittedModule;
};
} // namespace IGC
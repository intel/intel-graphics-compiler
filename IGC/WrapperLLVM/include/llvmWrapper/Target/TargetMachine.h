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

#ifndef IGCLLVM_TARGET_TARGETMACHINE_H
#define IGCLLVM_TARGET_TARGETMACHINE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 10
    using TargetMachine = llvm::TargetMachine;
#else
    class TargetMachine : public llvm::TargetMachine
    {
    public:
        using CodeGenFileType = llvm::CodeGenFileType;

    protected:
        TargetMachine(const llvm::Target &T, llvm::StringRef DataLayoutString,
            const llvm::Triple &TargetTriple, llvm::StringRef CPU,
            llvm::StringRef FS, const llvm::TargetOptions &Options)
            : llvm::TargetMachine(T, DataLayoutString, TargetTriple, CPU, FS,
                  Options) {}

    private:
        bool addPassesToEmitFile(llvm::PassManagerBase &PM,
            llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
            llvm::CodeGenFileType FileType, bool DisableVerify,
            llvm::MachineModuleInfoWrapperPass *MMIWP) override
        {
            llvm::MachineModuleInfo *MMI = nullptr;
            if (MMIWP)
                MMI = &MMIWP->getMMI();
            return addPassesToEmitFile(PM, o, pi, FileType, DisableVerify, MMI);
        }

    public:
        virtual bool addPassesToEmitFile(llvm::PassManagerBase &PM,
            llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
            CodeGenFileType FileType, bool DisableVerify,
            llvm::MachineModuleInfo *MMI)
        {
            return true;
        }
    };
#endif

#if LLVM_VERSION_MAJOR < 10
    using LLVMTargetMachine = llvm::LLVMTargetMachine;
#else
    class LLVMTargetMachine : public llvm::LLVMTargetMachine
    {
    public:
        using CodeGenFileType = llvm::CodeGenFileType;

    protected:
        LLVMTargetMachine(const llvm::Target &T, llvm::StringRef DataLayoutString,
            const llvm::Triple &TargetTriple, llvm::StringRef CPU,
            llvm::StringRef FS, const llvm::TargetOptions &Options,
            llvm::Reloc::Model RM, llvm::CodeModel::Model CM,
            llvm::CodeGenOpt::Level OL)
            : llvm::LLVMTargetMachine(T, DataLayoutString, TargetTriple, CPU, FS,
                  Options, RM, CM, OL) {}

    private:
        bool addPassesToEmitFile(llvm::PassManagerBase &PM,
            llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
            llvm::CodeGenFileType FileType, bool DisableVerify,
            llvm::MachineModuleInfoWrapperPass *MMIWP) override
        {
            llvm::MachineModuleInfo *MMI = nullptr;
            if (MMIWP)
                MMI = &MMIWP->getMMI();
            return addPassesToEmitFile(PM, o, pi, FileType, DisableVerify, MMI);
        }

    public:
        virtual bool addPassesToEmitFile(llvm::PassManagerBase &PM,
            llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
            CodeGenFileType FileType, bool DisableVerify,
            llvm::MachineModuleInfo *MMI)
        {
            return true;
        }
    };
#endif
} // namespace IGCLLVM

#endif

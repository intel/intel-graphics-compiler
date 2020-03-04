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
#ifdef _MSC_VER
#pragma warning(disable: 4005)
#endif
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>
#include <stdint.h>

#include "usc.h"

#include "../AdaptorOCL/OCL/LoadBuffer.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "common/debug/Debug.hpp"
#include "Probe.h"

namespace SIP
{
    enum SIP_ID {
        GEN9_SIP_DEBUG = 0,
        GEN9_SIP_CSR,
        GEN9_SIP_CSR_DEBUG,

        GEN10_SIP_DEBUG,
        GEN10_SIP_CSR,
        GEN10_SIP_CSR_DEBUG,

        GEN9_SIP_DEBUG_BINDLESS,
        GEN10_SIP_DEBUG_BINDLESS,

        GEN9_BXT_SIP_CSR,
        GEN9_SIP_CSR_DEBUG_LOCAL,
        GEN9_GLV_SIP_CSR,
        GEN11_SIP_CSR,
        GEN11_SIP_CSR_DEBUG,
        GEN11_HP_SIP_CSR,
        GEN11_LKF_SIP_CSR,
        GEN12_LP_CSR,
        GEN12_LP_CSR_DEBUG,
        GEN_SIP_MAX_INDEX
    };

    class CSystemThread
    {
    public:
        static bool   CreateSystemThreadKernel(
            const IGC::CPlatform &platform,
            const USC::SYSTEM_THREAD_MODE mode,
            USC::SSystemThreadKernelOutput* &pSystemThread,
            bool bindlessmode = false);

        static void   DeleteSystemThreadKernel(
            USC::SSystemThreadKernelOutput* &pSystemThread );

    private:
        CSystemThread( void );
    };


    class CGenSystemInstructionKernelProgram
    {
    public:
        CGenSystemInstructionKernelProgram* Create(
            const IGC::CPlatform &platform,
            const USC::SYSTEM_THREAD_MODE mode,
            const bool bindlessMode);

            void Delete(CGenSystemInstructionKernelProgram* &pKernelProgram );

            CGenSystemInstructionKernelProgram(const USC::SYSTEM_THREAD_MODE mode);
            unsigned int  GetProgramSize(){ IGC_ASSERT(m_ProgramSize) ; return m_ProgramSize;}
            void * GetLinearAddress(){ IGC_ASSERT(m_LinearAddress); return m_LinearAddress;}

    protected:

         unsigned int m_ProgramSize;
         void* m_LinearAddress;

    };

} // namespace IGC

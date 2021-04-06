/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/IntrinsicInst.h"
#include "common/LLVMWarningsPop.hpp"

#include "VISAIDebugEmitter.hpp"

#include <set>
#include <map>

namespace llvm
{
    class Module;
}

namespace IGC
{
    class StreamEmitter;
    class VISAModule;
    class DwarfDebug;
    class CodeGenContext;

    class DebugEmitter : public IDebugEmitter
    {
    public:
        DebugEmitter();
        ~DebugEmitter();

        // IDebugEmitter interface methods
        void Initialize(std::unique_ptr<VISAModule> VM, const DebugEmitterOpts& Opts) override;
        std::vector<char> Finalize(bool finalize, DbgDecoder* decodedDbg,
                                   const std::vector<llvm::DISubprogram*>&) override;
        void BeginInstruction(llvm::Instruction* pInst) override;
        void EndInstruction(llvm::Instruction* pInst) override;
        void BeginEncodingMark() override;
        void EndEncodingMark() override;

        IGC::VISAModule* getCurrentVISA() const override { return m_pVISAModule; }
        void setCurrentVISA(IGC::VISAModule* VM) override;
        void registerVISA(IGC::VISAModule*) override;

        void resetModule(std::unique_ptr<IGC::VISAModule> VM) override;

    private:
        /// @brief Reset Debug Emitter instance.
        void Reset();
        void processCurrentFunction(bool finalize, DbgDecoder* decodedDbg);

    private:
        bool m_initialized = false;
        bool m_debugEnabled = false;
        bool doneOnce = false;

        llvm::SmallVector<char, 1000> m_str;
        llvm::raw_svector_ostream m_outStream;

        VISAModule* m_pVISAModule = nullptr;
        /// m_pDwarfDebug - dwarf debug info processor.
        std::unique_ptr<IGC::DwarfDebug> m_pDwarfDebug;
        std::unique_ptr<IGC::StreamEmitter> m_pStreamEmitter;
        std::vector<std::unique_ptr<VISAModule>> toFree;

        unsigned int lastGenOff = 0;

        void writeProgramHeaderTable(bool is64Bit, void* pBuffer, unsigned int size);
        void setElfType(bool is64Bit, void* pBuffer);
    };

} // namespace IGC

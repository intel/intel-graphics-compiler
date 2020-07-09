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
#pragma once

#include "llvm/Config/llvm-config.h"

#include "Compiler/DebugInfo/VISAIDebugEmitter.hpp"

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

    void insertOCLMissingDebugConstMetadata(CodeGenContext* ctx);

    // Helper functions to analyze Debug info metadata present in LLVM IR
    class DebugMetadataInfo
    {
    public:
        static bool hasDashgOption(CodeGenContext* ctx);
        static bool hasAnyDebugInfo(CodeGenContext* ctx, bool& fullDebugInfo, bool& lineNumbersOnly);
        static std::string getUniqueFuncName(llvm::Function& F);
        static std::string getOrigFuncName(std::string cloneName);
    };

    class DebugEmitter : public IDebugEmitter
    {
    public:
        DebugEmitter() : IDebugEmitter(),
            m_initialized(false),
            m_debugEnabled(false),
            m_outStream(m_str),
            m_pVISAModule(nullptr),
            m_pStreamEmitter(nullptr),
            m_pDwarfDebug(nullptr) {}

        ~DebugEmitter();

        // IDebugEmitter interface methods
        void Initialize(CShader* pShader, bool debugEnabled);
        void Finalize(void*& pBuffer, unsigned int& size, bool finalize);
        void BeginInstruction(llvm::Instruction* pInst);
        void EndInstruction(llvm::Instruction* pInst);
        void BeginEncodingMark();
        void EndEncodingMark();
        void Free(void* pBuffer);
        void setFunction(llvm::Function* F, bool isCloned);
        void ResetVISAModule();
        VISAModule* GetVISAModule() { return m_pVISAModule; }
        void SetVISAModule(VISAModule* other) { m_pVISAModule = other; }
        void AddVISAModFunc(IGC::VISAModule* v, llvm::Function* f);

    private:
        /// @brief Reset Debug Emitter instance.
        void Reset();

    private:
        bool m_initialized;
        bool m_debugEnabled;
        bool doneOnce = false;

        llvm::SmallVector<char, 1000> m_str;
        llvm::raw_svector_ostream m_outStream;

        VISAModule* m_pVISAModule;
        StreamEmitter* m_pStreamEmitter;

        /// m_pDwarfDebug - dwarf debug info processor.
        DwarfDebug* m_pDwarfDebug;

        std::vector<VISAModule*> toFree;

        unsigned int lastGenOff = 0;

        void writeProgramHeaderTable(bool is64Bit, void* pBuffer, unsigned int size);
        void setElfType(bool is64Bit, void* pBuffer);
    };

} // namespace IGC
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

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "common/Types.hpp"
#include <string>

namespace llvm
{
class Instruction;
class Function;
}

namespace IGC
{
// Forward declaration
class CShader;
class VISAModule;

/// @brief IDebugEmitter is an interface for debug info emitter class.
///        It can be used by IGC VISA emitter pass to emit debug info.
class IDebugEmitter
{
public:
    /// @brief Creates a new concrete instance of debug emitter.
    /// @return A new instance of debug emitter.
    static IDebugEmitter* Create();

    /// @brief Releases given instance of debug emitter.
    /// @param pDebugEmitter instance of debug emitter to release.
    static void Release(IDebugEmitter* pDebugEmitter);

    IDebugEmitter() {}

    virtual ~IDebugEmitter() {}

    /// @brief Initialize debug emitter for processing the given shader.
    /// @param pShader shader to process, and emit debug info for.
    /// @param debugEnabled indicator for emitting debug info or not.
    virtual void Initialize(CShader* pShader, bool debugEnabled) = 0;

    /// @brief Emit debug info to given buffer and reset debug emitter.
    /// @param pBuffer [OUT] object buffer conatins the emitted debug info.
    /// @param size [OUT] size of debug info buffer.
    // @param finalize [IN] indicates whether this is last function in group.
    virtual void Finalize(void *&pBuffer, unsigned int &size, bool finalize) = 0;

    /// @brief Process instruction before emitting its VISA code.
    /// @param pInst instruction to process.
    virtual void BeginInstruction(llvm::Instruction *pInst) = 0;

    /// @brief Process instruction after emitting its VISA code.
    /// @param pInst instruction to process.
    virtual void EndInstruction(llvm::Instruction *pInst) = 0;

    /// @brief Mark begin of VISA code emitting section.
    virtual void BeginEncodingMark() = 0;

    /// @brief Mark end of VISA code emitting section.
    virtual void EndEncodingMark() = 0;

    /// @brief Free given buffer memory.
    /// @param pBuffer buffer allocated by debug emiiter component.
    virtual void Free(void *pBuffer) = 0;

    virtual void setFunction(llvm::Function* F, bool c) = 0;

    virtual void ResetVISAModule() = 0;

    virtual IGC::VISAModule* GetVISAModule() = 0;

    virtual void SetVISAModule(IGC::VISAModule*) = 0;
};

// Decode Gen debug info data structure
class DbgDecoder
{
public:
    class Mapping
    {
    public:
        class Register
        {
        public:
            uint16_t regNum;
            uint16_t subRegNum; // for GRF, in byte offset
        };
        class Memory
        {
        public:
            uint32_t isBaseOffBEFP : 1; // MSB of 32-bit field denotes whether base if off BE_FP (0) or absolute (1)
            int32_t memoryOffset : 31; // memory offset
        };
        union {
            Register r;
            Memory m;
        };
    };
    class VarAlloc
    {
    public:
        enum VirtualVarType
        {
            VirTypeAddress = 0,
            VirTypeFlag = 1,
            VirTypeGRF = 2
        };
        enum PhysicalVarType
        {
            PhyTypeAddress = 0,
            PhyTypeFlag = 1,
            PhyTypeGRF = 2,
            PhyTypeMemory = 3
        };
        VirtualVarType virtualType;
        PhysicalVarType physicalType;
        Mapping mapping;
    };
    class LiveIntervalsVISA
    {
    public:
        uint16_t start = 0;
        uint16_t end = 0;
        VarAlloc var;
    };
    class VarInfo
    {
    public:
        std::string name;
        std::vector<LiveIntervalsVISA> lrs;

        // Assume JIT always assigned same GRF/scratch slot for entire
        // lifetime for variable
        bool isGRF()
        {
            if (lrs.size() == 0)
                return false;

            if (lrs.front().var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF)
                return true;

            return false;
        }

        bool isSpill()
        {
            if (lrs.size() == 0)
                return false;

            if (lrs.front().var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory)
                return true;

            return false;
        }

        Mapping::Register getGRF()
        {
            return lrs.front().var.mapping.r;
        }

        Mapping::Memory getSpillOffset()
        {
            return lrs.front().var.mapping.m;
        }
    };
    class SubroutineInfo
    {
    public:
        std::string name;
        uint32_t startVISAIndex;
        uint32_t endVISAIndex;
        std::vector<LiveIntervalsVISA> retval;
    };
    class DbgInfoFormat
    {
    public:
        std::string kernelName;
        uint32_t relocOffset = 0;
        std::vector<std::pair<unsigned int, unsigned int>> CISAOffsetMap;
        std::vector<std::pair<unsigned int, unsigned int>> CISAIndexMap;
        std::vector<VarInfo> Vars;

        uint16_t numSubRoutines = 0;
        std::vector<SubroutineInfo> subs;
    };

    std::vector<DbgInfoFormat> compiledObjs;

private:
    void* dbg;
    uint16_t numCompiledObj = 0;
    uint32_t magic = 0;

    template<typename T> T read()
    {
        T* dbgT = (T*)dbg;
        T data = *dbgT;
        dbg = ++dbgT;
        return data;
    }

    VarAlloc readVarAlloc()
    {
        VarAlloc data;
        const unsigned int phyRefGRF = 2;
        const unsigned int phyRefMem = 3;

        data.virtualType = (VarAlloc::VirtualVarType)read<uint8_t>();
        data.physicalType = (VarAlloc::PhysicalVarType)read<uint8_t>();

        if (data.physicalType == phyRefGRF)
        {
            data.mapping.r.regNum = read<uint16_t>();
            data.mapping.r.subRegNum = read<uint16_t>();
        }
        else if (data.physicalType == phyRefMem)
        {
            uint32_t temp = read<uint32_t>();
            data.mapping.m.memoryOffset = (temp & 0x7fffffff);
            data.mapping.m.isBaseOffBEFP = (temp & 0x80000000);
        }
        return data;
    }

    void decode()
    {
        magic = read<uint32_t>();
        numCompiledObj = read<uint16_t>();

        for (unsigned int i = 0; i != numCompiledObj; i++)
        {
            DbgInfoFormat f;
            uint8_t nameLen = read<uint8_t>();
            for (unsigned int j = 0; j != nameLen; j++)
                f.kernelName += read<char>();
            f.relocOffset = read<uint32_t>();

            // cisa offsets map
            uint32_t count = read<uint32_t>();
            for (unsigned int j = 0; j != count; j++)
            {
                uint32_t cisaOffset = read<uint32_t>();
                uint32_t genOffset = read<uint32_t>();
                f.CISAOffsetMap.push_back(std::make_pair(cisaOffset, genOffset));
            }

            // cisa index map
            count = read<uint32_t>();
            for (unsigned int j = 0; j != count; j++)
            {
                uint32_t cisaIndex = read<uint32_t>();
                uint32_t genOffset = read<uint32_t>();
                f.CISAIndexMap.push_back(std::make_pair(cisaIndex, genOffset));
            }

            // var info
            count = read<uint32_t>();
            for (unsigned int j = 0; j != count; j++)
            {
                VarInfo v;

                nameLen = read<uint8_t>();
                for (unsigned int k = 0; k != nameLen; k++)
                    v.name += read<char>();

                auto countLRs = read<uint16_t>();
                for (unsigned int k = 0; k != countLRs; k++)
                {
                    LiveIntervalsVISA lv;
                    lv.start = read<uint16_t>();
                    lv.end = read<uint16_t>();
                    lv.var = readVarAlloc();

                    v.lrs.push_back(lv);
                }

                f.Vars.push_back(v);
            }

            count = read<uint16_t>();
            for (unsigned int j = 0; j != count; j++)
            {
                SubroutineInfo sub;
                nameLen = read<uint8_t>();
                for (unsigned int k = 0; k != nameLen; k++)
                    sub.name += read<char>();

                sub.startVISAIndex = read<uint32_t>();
                sub.endVISAIndex = read<uint32_t>();
                auto countLRs = read<uint16_t>();
                for (unsigned int k = 0; k != countLRs; k++)
                {
                    LiveIntervalsVISA lv;
                    lv.start = read<uint16_t>();
                    lv.end = read<uint16_t>();
                    lv.var = readVarAlloc();

                    sub.retval.push_back(lv);
                }
                f.subs.push_back(sub);
            }

            compiledObjs.push_back(f);
        }
    }

public:
    DbgDecoder(void* buf) : dbg(buf)
    {
        if (buf)
            decode();
    }

    bool getVarInfo(std::string& kernelName, std::string& name, VarInfo& var)
    {
        for (auto& k : compiledObjs)
        {
            if (compiledObjs.size() > 1 &&
                k.kernelName.compare(kernelName) != 0)
                continue;

            for (auto& v : k.Vars)
            {
                if (v.name.compare(name) == 0)
                {
                    var = v;
                    return true;
                }
            }
        }

        return false;
    }
};

} // namespace IGC
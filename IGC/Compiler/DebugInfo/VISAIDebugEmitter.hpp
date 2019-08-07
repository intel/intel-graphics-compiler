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
        virtual void Finalize(void*& pBuffer, unsigned int& size, bool finalize) = 0;

        /// @brief Process instruction before emitting its VISA code.
        /// @param pInst instruction to process.
        virtual void BeginInstruction(llvm::Instruction* pInst) = 0;

        /// @brief Process instruction after emitting its VISA code.
        /// @param pInst instruction to process.
        virtual void EndInstruction(llvm::Instruction* pInst) = 0;

        /// @brief Mark begin of VISA code emitting section.
        virtual void BeginEncodingMark() = 0;

        /// @brief Mark end of VISA code emitting section.
        virtual void EndEncodingMark() = 0;

        /// @brief Free given buffer memory.
        /// @param pBuffer buffer allocated by debug emiiter component.
        virtual void Free(void* pBuffer) = 0;

        virtual void setFunction(llvm::Function* F, bool c) = 0;

        virtual void ResetVISAModule() = 0;

        virtual IGC::VISAModule* GetVISAModule() = 0;

        virtual void SetVISAModule(IGC::VISAModule*) = 0;
    };

    template<typename T> T read(void*& dbg)
    {
        T* dbgT = (T*)dbg;
        T data = *dbgT;
        dbg = ++dbgT;
        return data;
    }

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
        class LiveIntervalGenISA
        {
        public:
            uint32_t start = 0;
            uint32_t end = 0;
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
        class RegInfoMapping
        {
        public:
            uint16_t srcRegOff = 0;
            uint16_t numBytes = 0;
            bool dstInReg = false;
            Mapping dst;
        };
        class PhyRegSaveInfoPerIP
        {
        public:
            uint32_t genIPOffset = 0;
            uint16_t numEntries = 0;
            std::vector<RegInfoMapping> data;
        };
        class CallFrameInfo
        {
        public:
            uint16_t frameSize = 0;
            bool befpValid = false;
            std::vector<LiveIntervalGenISA> befp;
            bool callerbefpValid = false;
            std::vector<LiveIntervalGenISA> callerbefp;
            bool retAddrValid = false;
            std::vector<LiveIntervalGenISA> retAddr;
            uint16_t numCalleeSaveEntries = 0;
            std::vector<PhyRegSaveInfoPerIP> calleeSaveEntry;
            uint16_t numCallerSaveEntries = 0;
            std::vector<PhyRegSaveInfoPerIP> callerSaveEntry;
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
            CallFrameInfo cfi;
        };

        std::vector<DbgInfoFormat> compiledObjs;

    private:
        void readMappingReg(DbgDecoder::Mapping& mapping)
        {
            mapping.r.regNum = read<uint16_t>(dbg);
            mapping.r.subRegNum = read<uint16_t>(dbg);
        }

        void readMappingMem(DbgDecoder::Mapping& mapping)
        {
            uint32_t temp = read<uint32_t>(dbg);
            mapping.m.memoryOffset = (temp & 0x7fffffff);
            mapping.m.isBaseOffBEFP = (temp & 0x80000000);
        }

        LiveIntervalsVISA readLiveIntervalsVISA()
        {
            DbgDecoder::LiveIntervalsVISA lv;
            lv.start = read<uint16_t>(dbg);
            lv.end = read<uint16_t>(dbg);
            lv.var = readVarAlloc();
            return lv;
        }

        LiveIntervalGenISA readLiveIntervalGenISA()
        {
            DbgDecoder::LiveIntervalGenISA lr;
            lr.start = read<uint32_t>(dbg);
            lr.end = read<uint32_t>(dbg);
            lr.var = readVarAlloc();
            return lr;
        }

        RegInfoMapping readRegInfoMapping()
        {
            DbgDecoder::RegInfoMapping info;
            info.srcRegOff = read<uint16_t>(dbg);
            info.numBytes = read<uint16_t>(dbg);
            info.dstInReg = (bool)read<uint8_t>(dbg);
            if (info.dstInReg)
            {
                readMappingReg(info.dst);
            }
            else
            {
                readMappingMem(info.dst);
            }
            return info;
        }

        VarAlloc readVarAlloc()
        {
            DbgDecoder::VarAlloc data;
            const unsigned int phyRefGRF = 2;
            const unsigned int phyRefMem = 3;

            data.virtualType = (DbgDecoder::VarAlloc::VirtualVarType)read<uint8_t>(dbg);
            data.physicalType = (DbgDecoder::VarAlloc::PhysicalVarType)read<uint8_t>(dbg);

            if (data.physicalType == phyRefGRF)
            {
                readMappingReg(data.mapping);
            }
            else if (data.physicalType == phyRefMem)
            {
                readMappingMem(data.mapping);
            }
            return data;
        }

        void* dbg;
        uint16_t numCompiledObj = 0;
        uint32_t magic = 0;

        void decode()
        {
            magic = read<uint32_t>(dbg);
            numCompiledObj = read<uint16_t>(dbg);

            for (unsigned int i = 0; i != numCompiledObj; i++)
            {
                DbgInfoFormat f;
                uint8_t nameLen = read<uint8_t>(dbg);
                for (unsigned int j = 0; j != nameLen; j++)
                    f.kernelName += read<char>(dbg);
                f.relocOffset = read<uint32_t>(dbg);

                // cisa offsets map
                uint32_t count = read<uint32_t>(dbg);
                for (unsigned int j = 0; j != count; j++)
                {
                    uint32_t cisaOffset = read<uint32_t>(dbg);
                    uint32_t genOffset = read<uint32_t>(dbg);
                    f.CISAOffsetMap.push_back(std::make_pair(cisaOffset, f.relocOffset + genOffset));
                }

                // cisa index map
                count = read<uint32_t>(dbg);
                for (unsigned int j = 0; j != count; j++)
                {
                    uint32_t cisaIndex = read<uint32_t>(dbg);
                    uint32_t genOffset = read<uint32_t>(dbg);
                    f.CISAIndexMap.push_back(std::make_pair(cisaIndex, f.relocOffset + genOffset));
                }

                // var info
                count = read<uint32_t>(dbg);
                for (unsigned int j = 0; j != count; j++)
                {
                    VarInfo v;

                    nameLen = read<uint8_t>(dbg);
                    for (unsigned int k = 0; k != nameLen; k++)
                        v.name += read<char>(dbg);

                    auto countLRs = read<uint16_t>(dbg);
                    for (unsigned int k = 0; k != countLRs; k++)
                    {
                        LiveIntervalsVISA lv = readLiveIntervalsVISA();
                        v.lrs.push_back(lv);
                    }

                    f.Vars.push_back(v);
                }

                // subroutines
                count = read<uint16_t>(dbg);
                for (unsigned int j = 0; j != count; j++)
                {
                    SubroutineInfo sub;
                    nameLen = read<uint8_t>(dbg);
                    for (unsigned int k = 0; k != nameLen; k++)
                        sub.name += read<char>(dbg);

                    sub.startVISAIndex = read<uint32_t>(dbg);
                    sub.endVISAIndex = read<uint32_t>(dbg);
                    auto countLRs = read<uint16_t>(dbg);
                    for (unsigned int k = 0; k != countLRs; k++)
                    {
                        LiveIntervalsVISA lv = readLiveIntervalsVISA();
                        sub.retval.push_back(lv);
                    }
                    f.subs.push_back(sub);
                }

                // call frame information
                f.cfi.frameSize = read<uint16_t>(dbg);
                f.cfi.befpValid = (bool)read<uint8_t>(dbg);
                if (f.cfi.befpValid)
                {
                    count = read<uint16_t>(dbg);
                    for (unsigned int j = 0; j != count; j++)
                    {
                        LiveIntervalGenISA lv = readLiveIntervalGenISA();;
                        f.cfi.befp.push_back(lv);
                    }
                }
                f.cfi.callerbefpValid = (bool)read<uint8_t>(dbg);
                if (f.cfi.callerbefpValid)
                {
                    count = read<uint16_t>(dbg);
                    for (unsigned int j = 0; j != count; j++)
                    {
                        LiveIntervalGenISA lv = readLiveIntervalGenISA();
                        f.cfi.callerbefp.push_back(lv);
                    }
                }
                f.cfi.retAddrValid = (bool)read<uint8_t>(dbg);
                if (f.cfi.retAddrValid)
                {
                    count = read<uint16_t>(dbg);
                    for (unsigned int j = 0; j != count; j++)
                    {
                        LiveIntervalGenISA lv = readLiveIntervalGenISA();
                        f.cfi.retAddr.push_back(lv);
                    }
                }
                f.cfi.numCalleeSaveEntries = read<uint16_t>(dbg);
                for (unsigned int j = 0; j != f.cfi.numCalleeSaveEntries; j++)
                {
                    PhyRegSaveInfoPerIP phyRegSave;
                    phyRegSave.genIPOffset = read<uint32_t>(dbg);
                    phyRegSave.numEntries = read<uint16_t>(dbg);
                    for (unsigned int k = 0; k != phyRegSave.numEntries; k++)
                        phyRegSave.data.push_back(readRegInfoMapping());
                    f.cfi.calleeSaveEntry.push_back(phyRegSave);
                }

                f.cfi.numCallerSaveEntries = read<uint16_t>(dbg);
                for (unsigned int j = 0; j != f.cfi.numCallerSaveEntries; j++)
                {
                    PhyRegSaveInfoPerIP phyRegSave;
                    phyRegSave.genIPOffset = read<uint32_t>(dbg);
                    phyRegSave.numEntries = read<uint16_t>(dbg);
                    for (unsigned int k = 0; k != phyRegSave.numEntries; k++)
                        phyRegSave.data.push_back(readRegInfoMapping());
                    f.cfi.callerSaveEntry.push_back(phyRegSave);
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
/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Path.h>
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <Metrics/IGCMetricImpl.h>
#include <Probe/Assertion.h>
#include <iomanip>
#include <common/igc_regkeys.hpp>
#include <visa/G4_IR.hpp>
#include <visa/SpillManagerGMRF.h>
#include <visa/LinearScanRA.h>
#include <visa/LocalRA.h>
#include <visa/LocalScheduler/Dependencies_G4IR.h>

namespace IGCMetrics
{
    IGCMetricImpl::IGCMetricImpl()
    {
        this->isEnabled = false;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        this->countInstInFunc = 0;
#endif
    }
    IGCMetricImpl::~IGCMetricImpl()
    {
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        this->map_EmuCalls.clear();
        this->map_Func.clear();
        for (auto func : map_InstrLoc2Func)
        {
            delete func.second;
        }
        this->map_InstrLoc2Func.clear();
        this->map_Loops.clear();
#endif
    }
    bool IGCMetricImpl::Enable()
    {
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        return isEnabled;
#else
        return false;
#endif
    }

    void IGCMetricImpl::Init(ShaderHash* Hash, bool isEnabled)
    {
        this->isEnabled = isEnabled;
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        std::stringstream ss;

        ss << std::hex
           << std::setfill('0')
           << std::setw(sizeof(Hash->asmHash) * CHAR_BIT / 4)
           << Hash->asmHash;

        oclProgram.set_hash(ss.str());
#endif
    }

    void IGCMetricImpl::OutputMetrics()
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED

        if (IGC_IS_FLAG_ENABLED(MetricsDumpEnable))
        {
            // Out file with ext OPTRPT - OPTimization RePoT
            std::string fileName = "OCL_" + oclProgram.hash() + ".optrpt";

            std::ofstream metric_data;
            metric_data.open(fileName);

            if (metric_data.is_open())
            {
                if (true)
                {
                    // Binary format of protobuf
                    oclProgram.SerializePartialToOstream(&metric_data);
                }
                else
                {
                    // Text readable in JSON format
                    google::protobuf::util::JsonPrintOptions jsonConfig;

                    jsonConfig.add_whitespace = true;
                    jsonConfig.preserve_proto_field_names = true;
                    jsonConfig.always_print_primitive_fields = true;

                    std::string json;
                    google::protobuf::util::MessageToJsonString(oclProgram, &json, jsonConfig);
                    metric_data << json;
                }

                metric_data.close();
            }
        }
#endif
    }

    void IGCMetricImpl::StatBeginEmuFunc(llvm::Instruction* instruction)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        countInstInFunc = CountInstInFunc(instruction->getParent()->getParent());
#endif
    }

    bool isDPType(llvm::Instruction* instruction)
    {
        llvm::Type* type = instruction->getType()->getScalarType();
        if (type->isDoubleTy())
        {
            return true;
        }

        for (unsigned int i = 0; i < instruction->getNumOperands(); ++i)
        {
            type = instruction->getOperand(i)->getType()->getScalarType();
            if (type->isDoubleTy())
            {
                return true;
            }
        }

        return false;
    }

    void IGCMetricImpl::StatEndEmuFunc(llvm::Instruction* emulatedInstruction)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        llvm::DILocation* debLoc = (llvm::DILocation*)emulatedInstruction->getDebugLoc();

        auto func_m_list = GetFuncMetric(emulatedInstruction);
        if (func_m_list == nullptr)
        {
            return;
        }
        for (auto func_m : *func_m_list)
        {
            IGC_METRICS::InstrStats* stats = func_m->mutable_instruction_stats();
            IGC_METRICS::FuncEmuCalls* emuCall_m = nullptr;

            // Count how many instructions we added
            int extraInstrAdded = CountInstInFunc(emulatedInstruction->getParent()->getParent()) -
                countInstInFunc;
            // reset counter
            countInstInFunc = 0;

            if (map_EmuCalls.find(debLoc) != map_EmuCalls.end())
            {
                // For case when receive extra instruction to already recoreded emu-function
                emuCall_m = map_EmuCalls[debLoc];
            }
            else
            {
                // For case if we discover new emulated function
                emuCall_m = func_m->add_emufunctioncalls();
                map_EmuCalls.insert({ debLoc, emuCall_m });

                auto emuCall_m_loc = emuCall_m->add_funccallloc();
                FillCodeRef(emuCall_m_loc, debLoc);
                stats->set_countemulatedinst(stats->countemulatedinst() + 1);

                if (IGC_IS_FLAG_ENABLED(ForceDPEmulation) && isDPType(emulatedInstruction))
                {
                    emuCall_m->set_type(IGC_METRICS::FuncEmuCalls_Reason4FuncEmu_FP_MODEL_MODE);
                }
                else
                {
                    emuCall_m->set_type(IGC_METRICS::FuncEmuCalls_Reason4FuncEmu_NO_HW_SUPPORT);
                }
            }
            // Count amount of instructions created to emulate not supported instruction
            emuCall_m->set_count(emuCall_m->count() + extraInstrAdded);
        }
#endif
    }

    void IGCMetricImpl::StatIncCoalesced(llvm::Instruction* coalescedAccess)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        auto func_m_list = GetFuncMetric(coalescedAccess);
        if (func_m_list == nullptr)
        {
            return;
        }
        for (auto func_m : *func_m_list)
        {
            IGC_METRICS::InstrStats* stats = func_m->mutable_instruction_stats();
            stats->set_countcoalescedaccess(stats->countcoalescedaccess() + 1);
        }
#endif
    }

    void IGCMetricImpl::CollectRegStats(KERNEL_INFO* kernelInfo)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        if (kernelInfo == nullptr)
        {
            return;
        }

        for (auto record = kernelInfo->variables.begin();
            record != kernelInfo->variables.end(); ++record)
        {
            auto varInfo = record->second;
            // Find to which function this var info belongs
            auto keyMap = GetHash(varInfo->srcFilename, varInfo->lineNb);
            auto func_m_list = GetFuncMetric(keyMap);

            if (func_m_list == nullptr)
            {
                continue;
            }

            for (auto func_m : *func_m_list)
            {
                bool alreadyRecord = false;
                for (int i = 0; i < func_m->variables_size(); ++i)
                {
                    if (func_m->variables(i).name() == record->first)
                    {
                        alreadyRecord = true;
                        break;
                    }
                }
                if (alreadyRecord)
                {
                    continue;
                }


                auto varInfo_m = func_m->add_variables();
                varInfo_m->set_name(record->first);
                varInfo_m->set_type((IGC_METRICS::VarInfo_VarType)varInfo->type);
                varInfo_m->set_memoryaccess((IGC_METRICS::VarInfo_MemAccess)varInfo->memoryAccess);
                varInfo_m->set_addrmodel((IGC_METRICS::VarInfo_AddressModel)varInfo->addrModel);
                varInfo_m->set_size(varInfo->size);
                varInfo_m->set_promoted2grf(varInfo->promoted2GRF);
                varInfo_m->set_isspill(varInfo->isSpill);
                varInfo_m->set_isuniform(varInfo->isUniform);
                varInfo_m->set_isconst(varInfo->isConst);

                auto bcInfo = varInfo_m->mutable_bc_stats();
                bcInfo->set_count(varInfo->bc_count);
                bcInfo->set_samebank(varInfo->bc_sameBank);
                bcInfo->set_twosrc(varInfo->bc_twoSrc);

                if (varInfo->srcFilename == nullptr)
                {
                    continue;
                }

                auto codeRef = varInfo_m->mutable_varloc();

                FillCodeRef(codeRef, varInfo->srcFilename, varInfo->lineNb);
            }
        }
#endif
    }

    void IGCMetricImpl::CollectFunctions(llvm::Module* pModule)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        for (auto func_i = pModule->begin(); func_i != pModule->end(); ++func_i)
        {
            llvm::Function& func = *func_i;
            llvm::DISubprogram* func_dbinfo = func.getSubprogram();

            if (func_dbinfo != nullptr)
            {
                IGC_METRICS::Function* func_m = oclProgram.add_functions();

                func_m->set_name(func.getName().str());

                switch (func.getCallingConv())
                {
                case llvm::CallingConv::SPIR_FUNC:
                    func_m->set_type(IGC_METRICS::FunctionType::FUNCTION);
                    break;
                case llvm::CallingConv::SPIR_KERNEL:
                    func_m->set_type(IGC_METRICS::FunctionType::KERNEL);
                    break;
                case llvm::CallingConv::C:
                    func_m->set_type(IGC_METRICS::FunctionType::FUNCTION);
                    break;
                default:
                    IGC_ASSERT_MESSAGE(false, "Unknow Function type");
                    break;
                }
                map_Func.insert({ func_dbinfo , func_m });
                IGC_METRICS::CodeRef* func_m_loc = func_m->mutable_funcloc();
                FillCodeRef(func_m_loc, func_dbinfo);

                GetFunctionCalls(func_m, func);
                CollectInstructions(pModule);
            }
        }
#endif
    }

    void IGCMetricImpl::CollectLoops(llvm::Loop* loop)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        for (auto subLoop : loop->getSubLoops())
        {
            CollectLoop(subLoop);
        }
#endif
    }

    void IGCMetricImpl::CollectLoops(llvm::LoopInfo* loopInfo)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        for (auto loop_i = loopInfo->begin(); loop_i != loopInfo->end(); ++loop_i)
        {
            llvm::Loop* loop = *loop_i;
            CollectLoop(loop);
            CollectLoops(loop);
        }
#endif
    }

    void IGCMetricImpl::FinalizeStats()
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        UpdateLoopsInfo();
#endif
    }

#ifdef IGC_METRICS__PROTOBUF_ATTACHED
    void IGCMetricImpl::UpdateCollectInstructions(llvm::Function* func)
    {
        for (auto bb = func->begin(); bb != func->end(); ++bb)
        {
            for (auto inst_i = bb->begin(); inst_i != bb->end(); ++inst_i)
            {
                llvm::DILocation* dbLoc = inst_i->getDebugLoc();

                if (dbLoc)
                {
                    MFuncList* func_m_list = nullptr;
                    HashKey key = GetHash(dbLoc);
                    llvm::DISubprogram* subProg = func->getSubprogram();

                    if (map_InstrLoc2Func.find(key) == map_InstrLoc2Func.end())
                    {
                        func_m_list = new MFuncList();
                        func_m_list->push_back(map_Func[subProg]);
                        map_InstrLoc2Func.insert({ key, func_m_list });
                    }
                    else
                    {
                        func_m_list = map_InstrLoc2Func[key];
                        if (std::find(func_m_list->begin(),
                            func_m_list->end(), map_Func[subProg]) == func_m_list->end())
                        {
                            func_m_list->push_back(map_Func[subProg]);
                        }
                    }
                }
            }
        }
    }

    void IGCMetricImpl::CollectLoop(llvm::Loop* loop)
    {
        if (loop->getStartLoc() && loop->getStartLoc()->getScope())
        {
            if (map_Loops.find(loop->getStartLoc()->getScope()) == map_Loops.end())
            {
                auto func_m_list = GetFuncMetric(loop);
                if (func_m_list == nullptr)
                {
                    return;
                }
                for (auto func_m : *func_m_list)
                {
                    auto cfg_stats = func_m->mutable_cfg_stats();
                    auto loop_m = cfg_stats->add_loops_stats();
                    auto loopLoc = loop_m->mutable_looploc();

                    FillCodeRef(loopLoc, loop->getStartLoc());
                    loop_m->set_nestinglevel(loop->getLoopDepth());

                    map_Loops.insert({ loop->getStartLoc()->getScope(), loop_m });
                }
            }
        }
    }

    void IGCMetricImpl::UpdateLoopsInfo()
    {
        //TODO
    }

    void IGCMetricImpl::GetFunctionCalls(IGC_METRICS::Function* func_m, llvm::Function& func)
    {
        for (auto bbIt = func.begin(); bbIt != func.end(); bbIt++)
        {
            for (auto instIt = bbIt->begin(); instIt != bbIt->end(); instIt++)
            {
                auto instr_call = llvm::dyn_cast<llvm::CallInst>(instIt);

                if (instr_call != nullptr)
                {
                    auto calledFunc = instr_call->getCalledFunction();

                    if (calledFunc == nullptr)
                    {
                        // TODO: Handle function pointers
                        continue;
                    }

                    auto calledFuncName = calledFunc->getName();
                    auto funcCallType = IGC_METRICS::FuncCalls_FuncCallsType::FuncCalls_FuncCallsType_INLINE;

                    // Check if this is function call which we wanted to track
                    if (calledFuncName.startswith("llvm.dbg") ||
                        calledFuncName.startswith("llvm.genx.GenISA.CatchAllDebugLine"))
                    {
                        // Ignore debugInfo calls
                        continue;
                    }
                    else if (calledFuncName.startswith("__builtin_IB"))
                    {
                        funcCallType = IGC_METRICS::FuncCalls_FuncCallsType::FuncCalls_FuncCallsType_LIBRARY;
                    }
                    else if (calledFuncName.startswith("llvm."))
                    {
                        funcCallType = IGC_METRICS::FuncCalls_FuncCallsType::FuncCalls_FuncCallsType_LIBRARY;
                    }
                    else if (calledFuncName.startswith("__builtin_spirv"))
                    {
                        funcCallType = IGC_METRICS::FuncCalls_FuncCallsType::FuncCalls_FuncCallsType_LIBRARY;
                    }

                    // Get data about this function call
                    IGC_METRICS::FuncCalls* callFunc_m = nullptr;

                    for (int i = 0; i < func_m->functioncalls_size(); ++i)
                    {
                        if (calledFuncName.equals(
                            func_m->functioncalls(i).name()))
                        {
                            // For case if we have already record created
                            callFunc_m = (IGC_METRICS::FuncCalls*)&func_m->functioncalls(i);
                            callFunc_m->set_count(callFunc_m->count() + 1);
                            break;
                        }
                    }
                    if (callFunc_m == nullptr)
                    {
                        // For new case
                        callFunc_m = func_m->add_functioncalls();
                        callFunc_m->set_name(calledFuncName.str());
                        callFunc_m->set_count(1);
                        callFunc_m->set_type(funcCallType);
                    }

                    auto instr_call_dbinfo = instr_call->getDebugLoc();
                    auto callFunc_m_loc = callFunc_m->add_funccallloc();
                    FillCodeRef(callFunc_m_loc, instr_call_dbinfo);
                }
            }
        }
    }

    void IGCMetricImpl::CollectInstructions(llvm::Module* pModule)
    {
        for (auto func = pModule->begin(); func != pModule->end(); ++func)
        {
            llvm::DISubprogram* subProg = func->getSubprogram();

            if (map_Func.find(subProg) == map_Func.end())
            {
                continue;
            }

            UpdateCollectInstructions(&*func);
        }
    }

    int IGCMetricImpl::CountInstInFunc(llvm::Function* pFunc)
    {
        unsigned int instCount = 0;
        for (auto bb = pFunc->begin(); bb != pFunc->end(); ++bb)
        {
            instCount += (unsigned int)std::distance(bb->begin(), bb->end());
        }

        return instCount;
    }

    MFuncList* IGCMetricImpl::GetFuncMetric(HashKey Key)
    {
        if (Key == HashKey_NULL)
        {
            return nullptr;
        }
        if (map_InstrLoc2Func.find(Key) == map_InstrLoc2Func.end())
        {
            return nullptr;
        }
        return map_InstrLoc2Func[Key];
    }

    MFuncList* IGCMetricImpl::GetFuncMetric(llvm::Instruction* pInstr)
    {
        return GetFuncMetric(GetHash(pInstr->getDebugLoc()));
    }

    MFuncList* IGCMetricImpl::GetFuncMetric(llvm::Loop* pLoop)
    {
        return GetFuncMetric(GetHash(pLoop->getStartLoc()));
    }

    HashKey IGCMetricImpl::GetHash(llvm::DILocation* Loc)
    {
        if (Loc == nullptr || Loc->getDirectory().empty() || Loc->getFilename().empty())
        {
            return HashKey_NULL;
        }
        return GetHash(
            GetFullPath(Loc->getDirectory().str(), Loc->getFilename().str()),
            Loc->getLine());
    }

    HashKey IGCMetricImpl::GetHash(const std::string& dir, const std::string& fileName, int line)
    {
        return GetHash(GetFullPath(dir, fileName), line);
    }

    HashKey IGCMetricImpl::GetHash(const char* dir, const char* fileName, int line)
    {
        if (dir == nullptr || fileName == nullptr)
        {
            return HashKey_NULL;
        }
        return GetHash(std::string(dir), std::string(fileName), line);
    }

    HashKey IGCMetricImpl::GetHash(const char* filePathName, int line)
    {
        if (filePathName == nullptr)
        {
            return HashKey_NULL;
        }
        return GetHash(std::string(filePathName), line);
    }

    HashKey IGCMetricImpl::GetHash(const std::string& filePathName, int line)
    {
        if (filePathName.length() == 0 && line == 0)
        {
            return HashKey_NULL;
        }
        std::hash<std::string> hasher;
        return hasher(filePathName + std::to_string(line));
    }

    void IGCMetricImpl::FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DISubprogram* Loc)
    {
        if (Loc == nullptr || Loc->getDirectory().empty() || Loc->getFilename().empty())
        {
            return;
        }
        FillCodeRef(codeRef, GetFullPath(Loc->getDirectory().str(), Loc->getFilename().str()),
            Loc->getLine());
    }

    void IGCMetricImpl::FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DILocation* Loc)
    {
        if (Loc == nullptr || Loc->getDirectory().empty() || Loc->getFilename().empty())
        {
            return;
        }
        FillCodeRef(codeRef, GetFullPath(Loc->getDirectory().str(), Loc->getFilename().str()),
            Loc->getLine());
    }

    void IGCMetricImpl::FillCodeRef(IGC_METRICS::CodeRef* codeRef, const std::string& filePathName, int line)
    {
        if (filePathName.empty())
        {
            return;
        }
        codeRef->set_line(line);
        codeRef->set_pathtofile(filePathName);
    }

    const std::string IGCMetricImpl::GetFullPath(const char* dir, const char* fileName)
    {
        return GetFullPath(std::string(dir), std::string(fileName));
    }

    const std::string IGCMetricImpl::GetFullPath(const std::string& dir, const std::string& fileName)
    {
        llvm::SmallVector<char, 1024> fileNamebuf;
        llvm::sys::path::append(fileNamebuf, dir);
        llvm::sys::path::append(fileNamebuf, fileName);
        std::string fileNameStr(fileNamebuf.begin(), fileNamebuf.end());
        return fileNameStr;
    }

#endif
}
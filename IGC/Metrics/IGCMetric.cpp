/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include <iostream>
#include <fstream>
#include <Metrics/IGCMetric.h>
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
    IGCMetric::IGCMetric() { };
    IGCMetric::~IGCMetric() { };
    bool IGCMetric::Enable()
    {
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        return isEnabled;
#else
        return false;
#endif
    }

    void IGCMetric::Init(ShaderHash* Hash, bool isEnabled)
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

    void IGCMetric::OutputMetrics()
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
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
#endif
    }

    void IGCMetric::StatBeginEmuFunc(llvm::Instruction* instruction)
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

    void IGCMetric::StatEndEmuFunc(llvm::Instruction* emulatedInstruction)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        llvm::DILocation* debLoc = (llvm::DILocation*)emulatedInstruction->getDebugLoc();

        IGC_METRICS::Function* func_m = GetFuncMetric(emulatedInstruction);
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
            emuCall_m_loc->set_line(debLoc->getLine());
            emuCall_m_loc->set_pathtofile(debLoc->getFilename().str());
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
#endif
    }

    void IGCMetric::StatIncCoalesced(llvm::Instruction* coalescedAccess)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::InstrStats* stats = GetFuncMetric(coalescedAccess)
            ->mutable_instruction_stats();
        stats->set_countcoalescedaccess(stats->countcoalescedaccess() + 1);
#endif
    }

    void IGCMetric::CollectRegStats(KERNEL_INFO* kernelInfo)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        for (auto record = kernelInfo->variables.begin();
            record != kernelInfo->variables.end(); ++record)
        {
            auto varInfo = record->second;
            // Find to which function this var info belongs
            auto func_scope = map_Instr[varInfo->lineNb]->getScope();
            // Get function metrics
            auto func_m = map_Func[func_scope];

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

            auto codeRef = varInfo_m->mutable_varloc();
            codeRef->set_line(varInfo->lineNb);
        }
#endif
    }

    void IGCMetric::CollectFunctions(llvm::Module* pModule)
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
                func_m_loc->set_line(func_dbinfo->getLine());
                func_m_loc->set_pathtofile(func_dbinfo->getFilename().str());

                GetFunctionCalls(func_m, func);
                CollectInstructions(pModule);
            }
        }
#endif
    }

    void IGCMetric::CollectLoops(llvm::Loop* loop)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        for (auto subLoop : loop->getSubLoops())
        {
            CollectLoop(subLoop);
        }
#endif
    }

    void IGCMetric::CollectLoops(llvm::LoopInfo* loopInfo)
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

    void IGCMetric::FinalizeStats()
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        UpdateLoopsInfo();
#endif
    }

#ifdef IGC_METRICS__PROTOBUF_ATTACHED
    void IGCMetric::CollectLoop(llvm::Loop* loop)
    {
        if (loop->getStartLoc() && loop->getStartLoc()->getScope())
        {
            if (map_Loops.find(loop->getStartLoc()->getScope()) == map_Loops.end())
            {
                auto func_m = GetFuncMetric(loop);
                auto cfg_stats = func_m->mutable_cfg_stats();
                auto loop_m = cfg_stats->add_loops_stats();
                auto loopLoc = loop_m->mutable_looploc();

                loopLoc->set_line(loop->getStartLoc()->getLine());
                loopLoc->set_pathtofile(loop->getStartLoc()->getFilename().str());
                loop_m->set_nestinglevel(loop->getLoopDepth());

                map_Loops.insert({ loop->getStartLoc()->getScope(), loop_m });
            }
        }
    }

    void IGCMetric::UpdateLoopsInfo()
    {
        //TODO
    }

    void IGCMetric::GetFunctionCalls(IGC_METRICS::Function* func_m, llvm::Function& func)
    {
        for (auto bbIt = func.begin(); bbIt != func.end(); bbIt++)
        {
            for (auto instIt = bbIt->begin(); instIt != bbIt->end(); instIt++)
            {
                auto instr_call = llvm::dyn_cast<llvm::CallInst>(instIt);

                if (instr_call != nullptr)
                {
                    auto calledFunc = instr_call->getCalledFunction();
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
                    callFunc_m_loc->set_line(instr_call_dbinfo.getLine());
                }
            }
        }
    }

    void IGCMetric::CollectInstructions(llvm::Module* pModule)
    {
        for (auto func = pModule->begin(); func != pModule->end(); ++func)
        {
            for (auto bb = func->begin(); bb != func->end(); ++bb)
            {
                for (auto inst_i = bb->begin(); inst_i != bb->end(); ++inst_i)
                {
                    llvm::DILocation* dbLoc = inst_i->getDebugLoc();
                    if (dbLoc &&
                        map_Instr.find(dbLoc->getLine()) == map_Instr.end())
                    {
                        map_Instr.insert({ dbLoc->getLine(), dbLoc});
                    }
                }
            }
        }
    }

    int IGCMetric::CountInstInFunc(llvm::Function* pFunc)
    {
        unsigned int instCount = 0;
        for (auto bb = pFunc->begin(); bb != pFunc->end(); ++bb)
        {
            instCount += (unsigned int)std::distance(bb->begin(), bb->end());
        }

        return instCount;
    }

    IGC_METRICS::Function* IGCMetric::GetFuncMetric(llvm::Instruction* pInstr)
    {
        auto getFunctionParrent = pInstr->getDebugLoc()->getScope();
        return map_Func[getFunctionParrent];
    }

    IGC_METRICS::Function* IGCMetric::GetFuncMetric(llvm::Loop* pLoop)
    {
        auto getFunctionParrent = pLoop->getStartLoc()->getScope()->getSubprogram();
        return map_Func[getFunctionParrent];
    }
#endif
}
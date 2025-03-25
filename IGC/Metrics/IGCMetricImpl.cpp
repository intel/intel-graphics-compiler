/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <algorithm>
#include <string>
#include <unordered_map>
#include <fstream>

#include <iomanip>
#include <common/igc_regkeys.hpp>
#include <Metrics/IGCMetricImpl.h>
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
#include <Metrics/IGCMetricsVer.h>
#endif

#include <Probe/Assertion.h>
#include <Compiler/CISACodeGen/ShaderCodeGen.hpp>
#include <DebugInfo/VISAModule.hpp>
#include <Compiler/DebugInfo/ScalarVISAModule.h>
#include <visaBuilder_interface.h>
#include <visa/Common_ISA.h>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Path.h>
#include <llvm/IR/DebugInfo.h>
#include <llvmWrapper/ADT/Optional.h>
#include <optional>
#include <llvmWrapper/Support/TypeSize.h>
#include "common/LLVMWarningsPop.hpp"

//#define DEBUG_METRIC

namespace IGCMetrics
{
    IGCMetricImpl::IGCMetricImpl()
    {
        this->isEnabled = false;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        this->countInstInFunc = 0;
        this->pMetricData = nullptr;
#endif
    }
    IGCMetricImpl::~IGCMetricImpl()
    {
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        this->map_EmuCalls.clear();
        this->map_Func.clear();
        this->map_Loops.clear();
        if (this->pMetricData != nullptr)
        {
            free(this->pMetricData);
            this->pMetricData = nullptr;
        }
#endif
    }
    bool IGCMetricImpl::Enable()
    {
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        return isEnabled && IGC_GET_FLAG_VALUE(MetricsDumpEnable) > 0;
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

    size_t IGCMetricImpl::getMetricDataSize()
    {
        if (!Enable()) return 0;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        // pMetricData = [VerInfo|CollectedMetricsData....]
        return sizeof(int) + oclProgram.ByteSizeLong();
#else
        return 0;
#endif
    }

    const void* const IGCMetricImpl::getMetricData()
    {
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        return pMetricData;
#else
        return nullptr;
#endif
    }

    void IGCMetricImpl::OutputMetrics()
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED

        if (pMetricData == nullptr)
        {
            // pMetricData = [VerInfo|CollectedMetricsData....]
            pMetricData = malloc(getMetricDataSize());

            // Encode the IGCMetrics version.
            // Currently it's count of commits made for
            // IGC/Metrics/proto_schema folder.
            // Define IGCMetricsVer generated during cmake call.
            // If there was a problem to generate the IGCMetricsVer value,
            // then it will be set to default "0"
            ((int*)pMetricData)[0] = IGCMetricsVer;

            if (oclProgram.ByteSizeLong() > 0)
            {
                // Copy IGC metrics data into stream
                void* pMetricDataInput = (void*)((char*)pMetricData + sizeof(int));
                oclProgram.SerializeToArray(
                    pMetricDataInput, oclProgram.ByteSizeLong());
            }
        }

        if (IGC_GET_FLAG_VALUE(MetricsDumpEnable) > 0)
        {
            // Out file with ext OPTRPT - OPTimization RePoT
            std::string fileName = oclProgram.hash() + ".optrpt";

            std::ofstream metric_data;
            metric_data.open(fileName);

            if (metric_data.is_open())
            {
                if (IGC_GET_FLAG_VALUE(MetricsDumpEnable) == 1)
                {
                    // Binary format of protobuf
                    metric_data.write(
                        (const char*)pMetricData, getMetricDataSize());
                }
                else if (IGC_GET_FLAG_VALUE(MetricsDumpEnable) == 2)
                {
                    // Text readable in JSON format
                    google::protobuf::util::JsonPrintOptions jsonConfig;

                    jsonConfig.add_whitespace = true;
                    jsonConfig.preserve_proto_field_names = true;
                    jsonConfig.always_print_primitive_fields = true;

                    std::string json;
                    google::protobuf::util::MessageToJsonString(oclProgram, &json, jsonConfig);
                    metric_data << "IGCMetricsVer : " << IGCMetricsVer << "\n";
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

        auto func_m = GetFuncMetric(emulatedInstruction);
        if (func_m == nullptr)
        {
            return;
        }

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
#endif
    }

    void IGCMetricImpl::StatIncCoalesced(llvm::Instruction* coalescedAccess)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        auto func_m = GetFuncMetric(coalescedAccess);
        if (func_m == nullptr)
        {
            return;
        }

        IGC_METRICS::InstrStats* stats = func_m->mutable_instruction_stats();
        stats->set_countcoalescedaccess(stats->countcoalescedaccess() + 1);
#endif
    }

    void IGCMetricImpl::CollectRegStats(KERNEL_INFO* kernelInfo, llvm::Function* pFunc)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        if (kernelInfo != nullptr)
        {
            auto func_m = GetFuncMetric(pFunc);
            if (func_m != nullptr)
            {
                auto func_reg_stats_m = func_m->mutable_local_reg_stats();
                func_reg_stats_m->set_percentgrfusage(
                    kernelInfo->precentGRFUsage);
                func_reg_stats_m->set_countregspillsallocated(
                    kernelInfo->numSpillReg);
                func_reg_stats_m->set_countregfillsallocated(
                    kernelInfo->numFillReg);
                func_reg_stats_m->set_countregallocated(
                    kernelInfo->numReg);
                func_reg_stats_m->set_countregtmpallocated(
                    kernelInfo->numTmpReg);
                func_reg_stats_m->set_countbytesregtmpallocated(
                    kernelInfo->bytesOfTmpReg);

                auto func_instr_stats_m = func_m->mutable_instruction_stats();

                auto func_instr_simd_stats_m = func_instr_stats_m->mutable_simd_used();
                func_instr_simd_stats_m->set_simd1(
                    kernelInfo->countSIMD1);
                func_instr_simd_stats_m->set_simd2(
                    kernelInfo->countSIMD2);
                func_instr_simd_stats_m->set_simd4(
                    kernelInfo->countSIMD4);
                func_instr_simd_stats_m->set_simd8(
                    kernelInfo->countSIMD8);
                func_instr_simd_stats_m->set_simd16(
                    kernelInfo->countSIMD16);
                func_instr_simd_stats_m->set_simd32(
                    kernelInfo->countSIMD32);

                if (kernelInfo->lscSends.hasAnyLSCSend)
                {
                    auto lsc_sends_stats_m = func_instr_stats_m->mutable_statslscsends();

                    // Load
                    lsc_sends_stats_m->set_countlsc_load(
                        kernelInfo->lscSends.countLSC_LOAD);
                    lsc_sends_stats_m->set_countlsc_load_strided(
                        kernelInfo->lscSends.countLSC_LOAD_STRIDED);
                    lsc_sends_stats_m->set_countlsc_load_quad(
                        kernelInfo->lscSends.countLSC_LOAD_QUAD);
                    lsc_sends_stats_m->set_countlsc_load_block2d(
                        kernelInfo->lscSends.countLSC_LOAD_BLOCK2D);
                    // Store
                    lsc_sends_stats_m->set_countlsc_store(
                        kernelInfo->lscSends.countLSC_STORE);
                    lsc_sends_stats_m->set_countlsc_store_strided(
                        kernelInfo->lscSends.countLSC_STORE_STRIDED);
                    lsc_sends_stats_m->set_countlsc_store_quad(
                        kernelInfo->lscSends.countLSC_STORE_QUAD);
                    lsc_sends_stats_m->set_countlsc_store_block2d(
                        kernelInfo->lscSends.countLSC_STORE_BLOCK2D);
                    lsc_sends_stats_m->set_countlsc_store_uncompressed(
                        kernelInfo->lscSends.countLSC_STORE_UNCOMPRESSED);
                }

                if (kernelInfo->hdcSends.hasAnyHDCSend)
                {
                    auto hdc_sends_stats_m = func_instr_stats_m->mutable_statshdcsends();
                    // Data Cache0 Messages
                    // Load
                    hdc_sends_stats_m->set_countdc_oword_block_read(
                        kernelInfo->hdcSends.countDC_OWORD_BLOCK_READ);
                    hdc_sends_stats_m->set_countdc_aligned_oword_block_read(
                        kernelInfo->hdcSends.countDC_ALIGNED_OWORD_BLOCK_READ);
                    hdc_sends_stats_m->set_countdc_dword_scattered_read(
                        kernelInfo->hdcSends.countDC_DWORD_SCATTERED_READ);
                    hdc_sends_stats_m->set_countdc_byte_scattered_read(
                        kernelInfo->hdcSends.countDC_BYTE_SCATTERED_READ);
                    hdc_sends_stats_m->set_countdc_qword_scattered_read(
                        kernelInfo->hdcSends.countDC_QWORD_SCATTERED_READ);
                    // Store
                    hdc_sends_stats_m->set_countdc_oword_block_write(
                        kernelInfo->hdcSends.countDC_OWORD_BLOCK_WRITE);
                    hdc_sends_stats_m->set_countdc_dword_scattered_write(
                        kernelInfo->hdcSends.countDC_DWORD_SCATTERED_WRITE);
                    hdc_sends_stats_m->set_countdc_byte_scattered_write(
                        kernelInfo->hdcSends.countDC_BYTE_SCATTERED_WRITE);
                    hdc_sends_stats_m->set_countdc_qword_scattered_write(
                        kernelInfo->hdcSends.countDC_QWORD_SCATTERED_WRITE);

                    // Data Cache1 Messages
                    // Load
                    hdc_sends_stats_m->set_countdc1_untyped_surface_read(
                        kernelInfo->hdcSends.countDC1_UNTYPED_SURFACE_READ);
                    hdc_sends_stats_m->set_countdc1_media_block_read(
                        kernelInfo->hdcSends.countDC1_MEDIA_BLOCK_READ);
                    hdc_sends_stats_m->set_countdc1_typed_surface_read(
                        kernelInfo->hdcSends.countDC1_TYPED_SURFACE_READ);
                    hdc_sends_stats_m->set_countdc1_a64_scattered_read(
                        kernelInfo->hdcSends.countDC1_A64_SCATTERED_READ);
                    hdc_sends_stats_m->set_countdc1_a64_untyped_surface_read(
                        kernelInfo->hdcSends.countDC1_A64_UNTYPED_SURFACE_READ);
                    hdc_sends_stats_m->set_countdc1_a64_block_read(
                        kernelInfo->hdcSends.countDC1_A64_BLOCK_READ);
                    // Store
                    hdc_sends_stats_m->set_countdc1_untyped_surface_write(
                        kernelInfo->hdcSends.countDC1_UNTYPED_SURFACE_WRITE);
                    hdc_sends_stats_m->set_countdc1_media_block_write(
                        kernelInfo->hdcSends.countDC1_MEDIA_BLOCK_WRITE);
                    hdc_sends_stats_m->set_countdc1_typed_surface_write(
                        kernelInfo->hdcSends.countDC1_TYPED_SURFACE_WRITE);
                    hdc_sends_stats_m->set_countdc1_a64_block_write(
                        kernelInfo->hdcSends.countDC1_A64_BLOCK_WRITE);
                    hdc_sends_stats_m->set_countdc1_a64_untyped_surface_write(
                        kernelInfo->hdcSends.countDC1_A64_UNTYPED_SURFACE_WRITE);
                    hdc_sends_stats_m->set_countdc1_a64_scattered_write(
                        kernelInfo->hdcSends.countDC1_A64_SCATTERED_WRITE);

                    // Data Cache2 Messages
                    // Load
                    hdc_sends_stats_m->set_countdc2_untyped_surface_read(
                        kernelInfo->hdcSends.countDC2_UNTYPED_SURFACE_READ);
                    hdc_sends_stats_m->set_countdc2_a64_scattered_read(
                        kernelInfo->hdcSends.countDC2_A64_SCATTERED_READ);
                    hdc_sends_stats_m->set_countdc2_a64_untyped_surface_read(
                        kernelInfo->hdcSends.countDC2_A64_UNTYPED_SURFACE_READ);
                    hdc_sends_stats_m->set_countdc2_byte_scattered_read(
                        kernelInfo->hdcSends.countDC2_BYTE_SCATTERED_READ);
                    // Store
                    hdc_sends_stats_m->set_countdc2_untyped_surface_write(
                        kernelInfo->hdcSends.countDC2_UNTYPED_SURFACE_WRITE);
                    hdc_sends_stats_m->set_countdc2_a64_untyped_surface_write(
                        kernelInfo->hdcSends.countDC2_A64_UNTYPED_SURFACE_WRITE);
                    hdc_sends_stats_m->set_countdc2_a64_scattered_write(
                        kernelInfo->hdcSends.countDC2_A64_SCATTERED_WRITE);
                    hdc_sends_stats_m->set_countdc2_byte_scattered_write(
                        kernelInfo->hdcSends.countDC2_BYTE_SCATTERED_WRITE);

                    // URB Messages
                    // Load
                    hdc_sends_stats_m->set_counturb_read_hword(
                        kernelInfo->hdcSends.countURB_READ_HWORD);
                    hdc_sends_stats_m->set_counturb_read_oword(
                        kernelInfo->hdcSends.countURB_READ_OWORD);
                    hdc_sends_stats_m->set_counturb_simd8_read(
                        kernelInfo->hdcSends.countURB_SIMD8_READ);
                    // Store
                    hdc_sends_stats_m->set_counturb_write_hword(
                        kernelInfo->hdcSends.countURB_WRITE_HWORD);
                    hdc_sends_stats_m->set_counturb_write_oword(
                        kernelInfo->hdcSends.countURB_WRITE_OWORD);
                    hdc_sends_stats_m->set_counturb_simd8_write(
                        kernelInfo->hdcSends.countURB_SIMD8_WRITE);
                }

                if (kernelInfo->spillFills.countBytesSpilled > 0)
                {
                    auto spillFill_m = func_m->mutable_spillfill_stats();

                    spillFill_m->set_countbytesspilled(
                        kernelInfo->spillFills.countBytesSpilled);

                    for (auto spillOrderInstr : kernelInfo->spillFills.spillInstrOrder)
                    {
                        spillFill_m->add_spillinstrvisaid(spillOrderInstr);
                    }
                    for (auto fillOrderInstr : kernelInfo->spillFills.fillInstrOrder)
                    {
                        spillFill_m->add_fillinstrvisaid(fillOrderInstr);
                    }
                    for (auto virtualVar : kernelInfo->spillFills.virtualVars)
                    {
                        spillFill_m->add_virtualvars(virtualVar);
                    }
                }
            }
        }
#endif
    }

    void IGCMetricImpl::CollectFunctions(llvm::Module* pModule)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED

        this->pModule = (IGCLLVM::Module*)pModule;
        fillInstrKindID = pModule->getMDKindID("FillInstr");
        spillInstrKindID = pModule->getMDKindID("SpillInstr");
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
                    IGC_ASSERT_MESSAGE(false, "Unknown Function type");
                    break;
                }
                map_Func.insert({ func_dbinfo , func_m });
                IGC_METRICS::CodeRef* func_m_loc = func_m->mutable_funcloc();
                FillCodeRef(func_m_loc, func_dbinfo);

                GetFunctionData(func_m, func);
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

    void IGCMetricImpl::CollectLoopCyclomaticComplexity(
        llvm::Function* pFunc,
        int LoopCyclomaticComplexity,
        int LoopCyclomaticComplexity_Max)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd16cost = costmodel->mutable_simd16();
            simd16cost->set_loopcyclomaticcomplexity(LoopCyclomaticComplexity);
            simd16cost->set_loopcyclomaticcomplexity_max(LoopCyclomaticComplexity_Max);
            simd16cost->set_loopcyclomaticcomplexity_status(
                LoopCyclomaticComplexity < LoopCyclomaticComplexity_Max ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::CollectNestedLoopsWithMultipleExits(
        llvm::Function* pFunc,
        float NestedLoopsWithMultipleExitsRatio,
        float NestedLoopsWithMultipleExitsRatio_Max)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd16cost = costmodel->mutable_simd16();
            simd16cost->set_nestedloopswithmultipleexitsratio(NestedLoopsWithMultipleExitsRatio);
            simd16cost->set_nestedloopswithmultipleexitsratio_max(NestedLoopsWithMultipleExitsRatio_Max);
            simd16cost->set_nestedloopswithmultipleexitsratio_status(
                NestedLoopsWithMultipleExitsRatio < NestedLoopsWithMultipleExitsRatio_Max ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::CollectLongStridedLdStInLoop(
        llvm::Function* pFunc,
        llvm::Loop* pProblematicLoop,
        int LongStridedLdStInLoop_LdCnt,
        int LongStridedLdStInLoop_StCnt,
        int LongStridedLdStInLoop_MaxCntLdOrSt)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd16cost = costmodel->mutable_simd16();

            if (pProblematicLoop == nullptr)
            {
                simd16cost->set_longstridedldstinloop_status(
                    IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK);
            }
            else
            {
                simd16cost->set_longstridedldstinloop_status(
                    IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
                simd16cost->set_longstridedldstinloop_ldcnt(LongStridedLdStInLoop_LdCnt);
                simd16cost->set_longstridedldstinloop_stcnt(LongStridedLdStInLoop_StCnt);
                simd16cost->set_longstridedldstinloop_maxcntldorst(LongStridedLdStInLoop_MaxCntLdOrSt);

                FillCodeRef(simd16cost->mutable_longstridedldstinloop_problematicloop(),
                    pProblematicLoop->getStartLoc());
            }
        }
#endif
    }

    void IGCMetricImpl::CollectIsGeminiLakeWithDoubles(
        llvm::Function* pFunc,
        bool IsGeminiLakeWithDoubles)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd16cost = costmodel->mutable_simd16();

            simd16cost->set_isgeminilakewithdoubles_status(IsGeminiLakeWithDoubles ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::FinalizeStats()
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        UpdateLoopsInfo();
        UpdateModelCost();
        UpdateFunctionArgumentsList();
        UpdateInstructionStats();
#endif
    }

    void IGCMetricImpl::CollectDataFromDebugInfo(llvm::Function* pFunc, IGC::DebugInfoData *pDebugInfo, const IGC::VISADebugInfo *pVisaDbgInfo)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED

        auto pDbgFunc = pFunc->getSubprogram();
        if (!pDbgFunc || map_Func.find(pDbgFunc) == map_Func.end())
        {
            // For case if we have debugInfo not for user kernel
            return;
        }

        oclProgram.set_device((IGC_METRICS::DeviceType)
            pDebugInfo->m_pShader->m_Platform->getPlatformInfo().eProductFamily);

        llvm::DenseMap<llvm::Function*, IGC::VISAModule*>* pListFuncData =
            &pDebugInfo->m_VISAModules;
        llvm::DenseMap< llvm::DIVariable*, IGC_METRICS::VarInfo*> var_db2metric;

        for (auto pListFuncData_it = pListFuncData->begin();
            pListFuncData_it != pListFuncData->end(); ++pListFuncData_it)
        {
            llvm::Function* pFunc = pListFuncData_it->first;
            IGC::VISAModule* vISAData = pListFuncData_it->second;
            const auto &VDI = vISAData->getVisaObjectDI(*pVisaDbgInfo);
#ifdef DEBUG_METRIC
            std::printf("\nList of symbols:\n");

            for (auto it_dbgInfo = pDebugInfo->m_FunctionSymbols[pFunc].begin();
                it_dbgInfo != pDebugInfo->m_FunctionSymbols[pFunc].end(); ++it_dbgInfo)
            {
                std::printf("pointer{%p} key{%s} val{%s}\n",
                    it_dbgInfo->first,
                    it_dbgInfo->first->getName().str().c_str(), it_dbgInfo->second->getName().getCString());
                it_dbgInfo->first->dump();
            }
#endif
            UpdateMem2RegStats(vISAData);

            const llvm::Value* pVal = nullptr;

            // Iterate over all instruction ported to vISA
            for (auto instr = vISAData->begin(); instr != vISAData->end(); ++instr)
            {
                if (const llvm::DbgDeclareInst* pDbgAddrInst =
                    llvm::dyn_cast<llvm::DbgDeclareInst>(*instr))
                {
                    // Get : call void @llvm.dbg.value
                    pVal = pDbgAddrInst->getAddress();
                }
                else if (const llvm::DbgValueInst* pDbgValInst =
                    llvm::dyn_cast<llvm::DbgValueInst>(*instr))
                {
                    // Get : call void @llvm.dbg.value

                    // Avoid undef values in metadata
                    {
                        llvm::MetadataAsValue* mdAv = llvm::dyn_cast<llvm::MetadataAsValue>(pDbgValInst->getArgOperand(0));
                        if (mdAv != nullptr)
                        {
                            llvm::ValueAsMetadata* vAsMD = llvm::dyn_cast<llvm::ValueAsMetadata>(mdAv->getMetadata());
                            if (vAsMD != nullptr &&
                                llvm::isa<llvm::UndefValue>(vAsMD->getValue()))
                            {
                                continue;
                            }
                        }
                    }

                    pVal = pDbgValInst->getValue();
                }
                else
                {
                    continue;
                }

                auto varLoc = vISAData->GetVariableLocation(*instr);

                IGC_METRICS::VarInfo* varInfo_m = GetVarMetric((llvm::Value*)pVal);

                if (varInfo_m == nullptr)
                {
                    continue;
                }

                // Get CVariable data for this user variable
                auto cvar = pDebugInfo->getMapping(*pFunc, pVal);

#ifdef DEBUG_METRIC
                int users_count = (int)std::distance(pVal->user_begin(), pVal->user_end());
                pVal->dump();
                std::printf("\ninstr (varname:%s, pointer:%p, usage count:%d) :\n", varInfo_m->name().c_str(), pVal, users_count);
                (*instr)->dump();
#endif

                if (!varLoc.IsRegister() &&
                    !varLoc.IsImmediate() &&
                    !varLoc.IsSLM())
                {
                    continue;
                }
                // As for now support only registers, immediates and slm memory to report

                if (cvar == nullptr &&
                    pDebugInfo->m_pShader->GetSymbolMapping().find((llvm::Value*)pVal)
                    != pDebugInfo->m_pShader->GetSymbolMapping().end())
                {
                    cvar = pDebugInfo->m_pShader->GetSymbolMapping()[(llvm::Value*)pVal];
                }

                if (cvar == nullptr &&
                    pDebugInfo->m_pShader->GetGlobalMapping().find((llvm::Value*)pVal)
                    != pDebugInfo->m_pShader->GetGlobalMapping().end())
                {
                    cvar = pDebugInfo->m_pShader->GetGlobalMapping()[(llvm::Value*)pVal];
                }

                if (cvar == nullptr)
                {
                    auto cvar_const = llvm::dyn_cast<llvm::Constant>(pVal);
                    if (cvar_const != nullptr &&
                        pDebugInfo->m_pShader->GetConstantMapping().find((llvm::Constant*)cvar_const)
                        != pDebugInfo->m_pShader->GetConstantMapping().end())
                    {
                        cvar = pDebugInfo->m_pShader->GetConstantMapping()[(llvm::Constant*)cvar_const];
                    }
                }

                if (cvar == nullptr)
                {
                    // If not found, ignore
                    continue;
                }

                varInfo_m->set_size(cvar->GetSize());
                varInfo_m->set_type((IGC_METRICS::VarInfo_VarType)cvar->GetType());

                auto fillRegister = [&](unsigned int reg)
                {
                    const auto* varInfo = vISAData->getVarInfo(VDI, reg);
                    auto varInfo_reg_m = varInfo_m->add_reg();

                    varInfo_reg_m->set_addrmodel(varLoc.IsInGlobalAddrSpace() ?
                        IGC_METRICS::VarInfo_AddressModel::VarInfo_AddressModel_GLOBAL :
                        IGC_METRICS::VarInfo_AddressModel::VarInfo_AddressModel_LOCAL);

                    //varInfo_m->set_memoryaccess((IGC_METRICS::VarInfo_MemAccess)varInfo->memoryAccess);

                    if (varInfo != nullptr)
                    {
                        // check if any?
                        varInfo_reg_m->set_isspill(varInfo->lrs[0].isSpill());
                        varInfo_reg_m->set_liverangestart(varInfo->lrs[0].start);
                        varInfo_reg_m->set_liverangeend(varInfo->lrs[0].end);
                    }
                    varInfo_reg_m->set_isuniform(cvar->IsUniform());
                    varInfo_reg_m->set_isconst(cvar->IsImmediate());
                };

                fillRegister(varLoc.GetRegister());
                // Special case when we have simd32 splitted into two simd16
                if (varLoc.HasLocationSecondReg())
                {
                    fillRegister(varLoc.GetSecondReg());
                }
            }
        }
#endif
    }


    void IGCMetricImpl::CollectInstructionCnt(
        llvm::Function* pFunc,
        int InstCnt,
        int InstCntMax)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd32cost = costmodel->mutable_simd32();

            simd32cost->set_instructioncount(InstCnt);
            simd32cost->set_instructioncount_max(InstCntMax);
            simd32cost->set_instructioncount_status(InstCnt < InstCntMax ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::CollectThreadGroupSize(
        llvm::Function* pFunc,
        int ThreadGroupSize,
        int ThreadGroupSizeMax)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd32cost = costmodel->mutable_simd32();

            simd32cost->set_threadgroupsize(ThreadGroupSize);
            simd32cost->set_threadgroupsize_max(ThreadGroupSizeMax);
            simd32cost->set_threadgroupsize_status(ThreadGroupSize < ThreadGroupSizeMax ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::CollectThreadGroupSizeHint(
        llvm::Function* pFunc,
        int ThreadGroupSizeHint,
        int ThreadGroupSizeHintMax)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd32cost = costmodel->mutable_simd32();

            simd32cost->set_threadgroupsizehint(ThreadGroupSizeHint);
            simd32cost->set_threadgroupsizehint_max(ThreadGroupSizeHintMax);
            simd32cost->set_threadgroupsizehint_status(ThreadGroupSizeHint < ThreadGroupSizeHintMax ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::CollectIsSubGroupFuncIn(
        llvm::Function* pFunc,
        bool flag)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd32cost = costmodel->mutable_simd32();

            simd32cost->set_subgroupfunctionarepresent_status(!flag ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    void IGCMetricImpl::CollectGen9Gen10WithIEEESqrtDivFunc(
        llvm::Function* pFunc,
        bool flag)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd32cost = costmodel->mutable_simd32();

            simd32cost->set_gen9orgen10withieeesqrtordivfunc_status(!flag ?
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK :
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
        }
#endif
    }

    enum {
        LOOPCOUNT_LIKELY_SMALL,
        LOOPCOUNT_LIKELY_LARGE,
        LOOPCOUNT_UNKNOWN
    };

    void IGCMetricImpl::CollectNonUniformLoop(
        llvm::Function* pFunc,
        short LoopCount,
        llvm::Loop* problematicLoop)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pFunc);

        if (func_metric)
        {
            auto costmodel = func_metric->mutable_costmodel_stats();
            auto simd32cost = costmodel->mutable_simd32();

            if (problematicLoop == nullptr)
            {
                simd32cost->set_nonuniformloop_status(
                    IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_OK);
                simd32cost->set_nonuniformloop_count(
                    IGC_METRICS::CostModelStats_CostSIMD32_LoopCount::CostModelStats_CostSIMD32_LoopCount_LIKELY_SMALL);
            }
            else
            {
                simd32cost->set_nonuniformloop_status(
                    IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD);
                simd32cost->set_nonuniformloop_count(
                    (IGC_METRICS::CostModelStats_CostSIMD32_LoopCount)LoopCount);
                auto codeRefloop = simd32cost->mutable_nonuniformloop_problematicloop();
                FillCodeRef(codeRefloop, problematicLoop->getStartLoc());
            }
        }
#endif
    }

    void IGCMetricImpl::UpdateVariable(llvm::Value* Org, llvm::Value* New)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        auto varData = GetVarData(Org);

        if (varData)
        {
            auto MDDILocalVariable = varData->varDILocalVariable;
            llvm::MetadataAsValue* MDValue = makeMDasVal(New);

            llvm::Instruction* insertAfter = nullptr;
            if (auto arg = llvm::dyn_cast<llvm::Argument>(New))
            {
                insertAfter = &*arg->getParent()->getEntryBlock().begin();
            }
            else if (auto instruction = llvm::dyn_cast<llvm::Instruction>(New))
            {
                insertAfter = instruction;
            }
            else
            {
                IGC_ASSERT_MESSAGE(false, "Unknown llvm type");
            }

            makeTrackCall(
                funcTrackValue, { MDValue, MDDILocalVariable }, insertAfter);
        }
#endif
    }

    void IGCMetricImpl::CollectMem2Reg(llvm::AllocaInst* pAllocaInst, IGC::StatusPrivArr2Reg status)
    {
        if (!Enable()) return;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Function* func_metric = GetFuncMetric(pAllocaInst);
        if (func_metric)
        {
            auto var_d = GetVarData(pAllocaInst);

            auto reg_stats = func_metric->mutable_local_reg_stats();

            if (status == IGC::StatusPrivArr2Reg::OK)
            {
                reg_stats->set_countprvarray2grfpromoted(
                    reg_stats->countprvarray2grfpromoted() + 1);
            }
            else
            {
                reg_stats->set_countprvarray2grfnotpromoted(
                    reg_stats->countprvarray2grfnotpromoted() + 1);

                if (var_d)
                {
                    // As for now assuming that all store/load associated with
                    // alloca will be treated as spill/fill
                    std::vector<llvm::LoadInst*> fills;
                    std::vector<llvm::StoreInst*> spills;

                    std::function<void(llvm::User*,
                        std::vector<llvm::LoadInst*>&,
                        std::vector<llvm::StoreInst*>&)>
                        lookForSpillFills
                        = [&](llvm::User* enterUser,
                            std::vector<llvm::LoadInst*>& fills,
                            std::vector<llvm::StoreInst*>& spills)
                    {
                        for (auto user : enterUser->users())
                        {
                            if (auto load = llvm::dyn_cast<llvm::LoadInst>(user))
                            {
                                fills.push_back(load);
                            }
                            else if (auto load = llvm::dyn_cast<llvm::StoreInst>(user))
                            {
                                spills.push_back(load);
                            }
                            else if (llvm::isa<llvm::GetElementPtrInst>(user))
                            {
                                lookForSpillFills(user, fills, spills);
                            }
                        }
                    };

                    // Map in code any refrence to this spill/fills (for metrics)
                    // by adding callinstr llvm.igc.metric.trackSpill/Fill in module for tracking
                    lookForSpillFills(pAllocaInst, fills, spills);

                    for (auto fill : fills)
                    {
                        MDNode* N = llvm::cast<DILocalVariable>(var_d->varDILocalVariable->getMetadata());
                        fill->setMetadata(fillInstrKindID, N);
                    }

                    for (auto spill : spills)
                    {
                        MDNode* N = llvm::cast<DILocalVariable>(var_d->varDILocalVariable->getMetadata());
                        spill->setMetadata(spillInstrKindID, N);
                    }

                    auto allocated = IGCLLVM::makeOptional(
                        pAllocaInst->getAllocationSizeInBits(pModule->getDataLayout()));

                    auto func_m = GetFuncMetric(pAllocaInst);
                    auto spillFill_m = func_m->mutable_spillfill_stats();
                    // Add amount of bytes spilled
                    spillFill_m->set_countbytesspilled(
                        spillFill_m->countbytesspilled() +
                        (int32_t)(allocated.value_or(IGCLLVM::getTypeSize(0)) / 8));
                }
            }

            if (var_d)
            {
                var_d->var_m->set_status_privarr2reg(
                    (IGC_METRICS::VarInfo_PrivArr2Reg)status);
            }
        }
#endif
    }

#ifdef IGC_METRICS__PROTOBUF_ATTACHED

    class CollectSpillFills : public llvm::InstVisitor<CollectSpillFills>
    {
        IGCMetricImpl* metric;
        IGC::VISAModule* CurrentVISA;

    public:

        CollectSpillFills(IGCMetricImpl* metric, IGC::VISAModule* CurrentVISA)
        {
            this->metric = metric;
            this->CurrentVISA = CurrentVISA;
        }

        void visitLoadInst(llvm::LoadInst& instr)
        {
            auto spillMD = instr.getMetadata(metric->fillInstrKindID);
            if (spillMD != nullptr)
            {
                // Some instructions dosen't have visa offset
                // that means the instruction is a dead code
                if (CurrentVISA->HasVisaOffset(&instr))
                {
                    auto func_m = metric->GetFuncMetric(&instr);
                    auto spillFill_m = func_m->mutable_spillfill_stats();

                    spillFill_m->set_countfillinstr(spillFill_m->countfillinstr() + 1);

                    // Add spill instruction to metrics
                    spillFill_m->add_fillinstrvisaid(CurrentVISA->GetVisaOffset(&instr));
                }
            }
        }

        void visitStoreInst(llvm::StoreInst& instr)
        {
            auto fillMD = instr.getMetadata(metric->spillInstrKindID);
            if (fillMD != nullptr)
            {
                // Some instructions dosen't have visa offset
                // that means the instruction is a dead code
                if (CurrentVISA->HasVisaOffset(&instr))
                {
                    auto func_m = metric->GetFuncMetric(&instr);
                    auto spillFill_m = func_m->mutable_spillfill_stats();

                    spillFill_m->set_countspillinstr(spillFill_m->countspillinstr() + 1);

                    // Add spill instruction to metrics
                    spillFill_m->add_spillinstrvisaid(CurrentVISA->GetVisaOffset(&instr));
                }
            }
        }
    };

    void IGCMetricImpl::UpdateMem2RegStats(IGC::VISAModule* CurrentVISA)
    {
        // Now if we have vISA kernel code ready, then we can map
        // spill/fills from IGC to vISA-ID
        CollectSpillFills metricPass(this, CurrentVISA);

        metricPass.visit((llvm::Function*)(CurrentVISA->GetEntryFunction()));
    }

    void IGCMetricImpl::UpdateFunctionArgumentsList()
    {
        for (auto func_i = pModule->begin();
            func_i != pModule->end(); ++func_i)
        {
            llvm::Function* func = &*func_i;

            auto func_m = GetFuncMetric(func);

            if (func_m)
            {
                for (auto arg_i = func->arg_begin();
                    arg_i != func->arg_end(); ++arg_i)
                {
                    llvm::Argument* arg = &*arg_i;
                    bool foundInMetric = false;

                    // Check if we are looking on the explicit argument
                    // which is already added in the metrics for the function
                    if (arg->hasName())
                    {
                        for (int i = 0; i < func_m->arguments_size(); ++i)
                        {
                            if (func_m->arguments(i).name() == arg->getName().str())
                            {
                                foundInMetric = true;
                                break;
                            }
                        }
                    }

                    // Not found - add it as implict argument
                    if (!foundInMetric)
                    {
                        auto func_arg_m = func_m->add_arguments();
                        if (arg->hasName())
                        {
                            func_arg_m->set_name(arg->getName().str());
                        }
                        func_arg_m->set_compilesize((int32_t)arg->getType()->getPrimitiveSizeInBits());
                        func_arg_m->set_type(IGC_METRICS::KernelArg_ArgumentType::KernelArg_ArgumentType_IMPLICIT);
                    }
                }
            }
        }
    }

    void IGCMetricImpl::UpdateModelCost()
    {
        // Function which checks the overall model cost of kernel status for SIMD16 and SIMD32

        auto isOkStatus = [](IGC_METRICS::CostModelStats_CostStatus Status)
        {
            return Status !=
                IGC_METRICS::CostModelStats_CostStatus::CostModelStats_CostStatus_BAD;
        };

        for (auto func_m_i = map_Func.begin(); func_m_i != map_Func.end(); ++func_m_i)
        {
            auto func_m = func_m_i->second;

            if (func_m->has_costmodel_stats())
            {
                auto costmodel = func_m->mutable_costmodel_stats();

                if (costmodel->has_simd16())
                {
                    auto simd16 = costmodel->mutable_simd16();

                    simd16->set_overallstatus(
                        isOkStatus(simd16->loopcyclomaticcomplexity_status()) &&
                        isOkStatus(simd16->nestedloopswithmultipleexitsratio_status()) &&
                        isOkStatus(simd16->longstridedldstinloop_status()) &&
                        isOkStatus(simd16->isgeminilakewithdoubles_status()));
                }

                if (costmodel->has_simd32())
                {
                    auto simd32 = costmodel->mutable_simd32();

                    simd32->set_overallstatus(
                        isOkStatus(simd32->instructioncount_status()) &&
                        isOkStatus(simd32->threadgroupsize_status()) &&
                        isOkStatus(simd32->threadgroupsizehint_status()) &&
                        isOkStatus(simd32->subgroupfunctionarepresent_status()) &&
                        isOkStatus(simd32->gen9orgen10withieeesqrtordivfunc_status()) &&
                        isOkStatus(simd32->nonuniformloop_status()));
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
                auto func_m = GetFuncMetric(loop);
                if (func_m == nullptr)
                {
                    return;
                }

                auto cfg_stats = func_m->mutable_cfg_stats();
                auto loop_m = cfg_stats->add_loops_stats();
                auto loopLoc = loop_m->mutable_looploc();

                FillCodeRef(loopLoc, loop->getStartLoc());
                loop_m->set_nestinglevel(loop->getLoopDepth());

                map_Loops.insert({ loop->getStartLoc()->getScope(), loop_m });
            }
        }
    }

    void IGCMetricImpl::UpdateLoopsInfo()
    {
    }

    class CollectInstrData : public llvm::InstVisitor<CollectInstrData>
    {
        IGCMetricImpl* metric;

    public:

        CollectInstrData(IGCMetricImpl* metric)
        {
            this->metric = metric;
        }

        void visitUnaryOperator(llvm::UnaryOperator& unaryOpInst)
        {
            AddArithmeticinstCount(unaryOpInst);
        }

        void visitBinaryOperator(llvm::BinaryOperator& binaryOpInst)
        {
            AddArithmeticinstCount(binaryOpInst);
        }

        void visitCallInst(llvm::CallInst& callInst)
        {
            /*
            if (GenIntrinsicInst* CI = llvm::dyn_cast<GenIntrinsicInst>(&callInst))
            {
                switch (CI->getIntrinsicID())
                {
                default:
                    break;
                }
            }*/
            if (IntrinsicInst* CI = llvm::dyn_cast<IntrinsicInst>(&callInst))
            {
                switch (CI->getIntrinsicID())
                {
                case Intrinsic::log:
                case Intrinsic::log2:
                case Intrinsic::log10:
                case Intrinsic::cos:
                case Intrinsic::sin:
                case Intrinsic::exp:
                case Intrinsic::exp2:
                    AddTranscendentalFuncCount(callInst);
                    break;
                case Intrinsic::sqrt:
                case Intrinsic::pow:
                case Intrinsic::floor:
                case Intrinsic::ceil:
                case Intrinsic::trunc:
                case Intrinsic::maxnum:
                case Intrinsic::minnum:
                    AddArithmeticinstCount(callInst);
                    break;
                default:
                    break;
                }
            }
        }

        void AddTranscendentalFuncCount(llvm::Instruction& instr)
        {
            // Transcendental functions includes:
            // 1.exponential function
            // 2.logarithm
            // 3.trigonometric functions
            auto func_m = metric->GetFuncMetric(&instr);

            if (func_m)
            {
                auto instr_stats_m = func_m->mutable_instruction_stats();
                instr_stats_m->set_counttranscendentalfunc(
                    instr_stats_m->counttranscendentalfunc() + 1
                );
            }
        }

        void AddArithmeticinstCount(llvm::Instruction& instr)
        {
            auto func_m = metric->GetFuncMetric(&instr);

            if (func_m)
            {
                auto instr_stats_m = func_m->mutable_instruction_stats();
                instr_stats_m->set_countarithmeticinst(
                    instr_stats_m->countarithmeticinst() + 1
                );
            }
        }
    };

    void IGCMetricImpl::UpdateInstructionStats()
    {
        CollectInstrData metricPass(this);

        metricPass.visit(pModule);
    }

    class CollectFuncData : public llvm::InstVisitor<CollectFuncData>
    {
        IGCMetricImpl* metric;

    public:

        CollectFuncData(IGCMetricImpl* metric)
        {
            this->metric = metric;
        }

        void visitDbgVariableIntrinsic(llvm::DbgVariableIntrinsic& dbValInst)
        {
            metric->AddVarMetric(&dbValInst);
        }

        void visitCallInst(llvm::CallInst& callInst)
        {
            auto calledFuncName = callInst.getCalledFunction()->getName();
            if (calledFuncName.startswith("llvm.dbg") ||
                calledFuncName.startswith("llvm.genx.GenISA.CatchAllDebugLine"))
            {
                // Ignore debugInfo calls
                return;
            }


            auto func_m = metric->GetFuncMetric(&callInst);
            auto funcCallType = IGC_METRICS::FuncCalls_FuncCallsType::FuncCalls_FuncCallsType_INLINE;

            if (calledFuncName.startswith("__builtin_IB"))
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
                    callFunc_m = (IGC_METRICS::FuncCalls*) & func_m->functioncalls(i);
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

            auto instr_call_dbinfo = callInst.getDebugLoc();
            auto callFunc_m_loc = callFunc_m->add_funccallloc();
            metric->FillCodeRef(callFunc_m_loc, instr_call_dbinfo);
        }
    };

    void IGCMetricImpl::GetFunctionData(IGC_METRICS::Function* func_m, llvm::Function& func)
    {
        CollectFuncData metricPass(this);

        metricPass.visit(func);

        for (auto bb_i = func.begin(); bb_i != func.end(); ++bb_i)
        {
            llvm::BasicBlock* bb = &*bb_i;

            if (bb->hasName() && (bb->getName().startswith("if.then") ||
                bb->getName().startswith("if.else")))
            {
                auto func_cfg_stats = func_m->mutable_cfg_stats();
                auto ifelse_m = func_cfg_stats->add_ifelsebr_stats();

                ifelse_m->set_countbrtaken((int)std::distance(
                    bb->users().begin(), bb->users().end()));

                llvm::DebugLoc* dbLoc = nullptr;
                auto instr_i = bb->end();

                // Start checking from the terminator instruction
                // then go to the previous one
                do
                {
                    instr_i--;
                    // We need to get debug info
                    dbLoc = (llvm::DebugLoc*)&instr_i->getDebugLoc();
                } while (*dbLoc && instr_i != bb->begin());

                if (*dbLoc)
                {
                    auto ifelse_block_db = llvm::dyn_cast<DILexicalBlock>(
                        dbLoc->getScope());
                    FillCodeRef(ifelse_m->mutable_brloc(), ifelse_block_db);
                }
            }
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

    llvm::CallInst* IGCMetricImpl::makeTrackCall(
        const char* const trackCall,
        ArrayRef<Value*> Args,
        llvm::Instruction* insertAfter)
    {
        auto& ctx = pModule->getContext();
        auto atrr = llvm::AttributeList::get(ctx, {
            {0, llvm::Attribute::get(ctx, llvm::Attribute::AttrKind::OptimizeNone)},
            {1, llvm::Attribute::get(ctx, llvm::Attribute::AttrKind::NoInline)},
            {2, llvm::Attribute::get(ctx, llvm::Attribute::AttrKind::ReadNone)},
            {3, llvm::Attribute::get(ctx, llvm::Attribute::AttrKind::NoAlias)} });

        auto funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(ctx),
            { llvm::Type::getMetadataTy(ctx), llvm::Type::getMetadataTy(ctx) }, false);

        auto funcVal = pModule->getOrInsertFunction(trackCall, funcType, atrr);

        llvm::Function* func = llvm::cast<llvm::Function>(funcVal);

        return llvm::CallInst::Create(func, Args, "", insertAfter->getNextNode());
    }

    IGC_METRICS::VarInfo* IGCMetricImpl::AddVarMetric(llvm::DbgVariableIntrinsic* pInstr)
    {
        int DILocalVariableIndex = 1;
        if (llvm::isa<llvm::DbgValueInst>(pInstr) &&
            pInstr->arg_size() == 4)
        {
            DILocalVariableIndex = 2;
        }

        llvm::MDNode* pNode = nullptr;
        llvm::Value* value = nullptr;
        llvm::MetadataAsValue* MDValue = llvm::dyn_cast<llvm::MetadataAsValue>(pInstr->getArgOperand(0));
        llvm::MetadataAsValue* MDDILocalVariable = llvm::dyn_cast<llvm::MetadataAsValue>(pInstr->getArgOperand(DILocalVariableIndex));
        IGC_METRICS::VarInfo* var_m = nullptr;

        if (MDValue != nullptr)
        {
            llvm::ValueAsMetadata* vAsMD = llvm::dyn_cast<llvm::ValueAsMetadata>(MDValue->getMetadata());
            pNode = llvm::cast<DILocalVariable>(MDDILocalVariable->getMetadata());
            if (vAsMD != nullptr &&
                vAsMD->getValue() != nullptr)
            {
                value = vAsMD->getValue();
            }
        }

        if (pNode && value)
        {
            // Map only once user variable in metrics
            if (map_Var.find(MDDILocalVariable) == map_Var.end())
            {
                // Extract debuginfo variable data to metrics
                llvm::DIVariable* diVar = llvm::cast<llvm::DIVariable>(pNode);

                std::string varName = diVar->getName().str();

                auto func_m = GetFuncMetric(pInstr);

                var_m = func_m->add_variables();
                var_m->set_name(varName);
                FillCodeRef(var_m->mutable_varloc(), diVar);

                // If variable is an argument of function/kernel
                // make a record of this information in metric too
                if (llvm::isa<llvm::Argument>(value))
                {
                    auto func_arg_m = func_m->add_arguments();
                    func_arg_m->set_name(varName);
                    func_arg_m->set_compilesize((int32_t)value->getType()->getPrimitiveSizeInBits());
                    func_arg_m->set_type(IGC_METRICS::KernelArg_ArgumentType::KernelArg_ArgumentType_EXPLICIT);
                }

                // The user variables are identified by the MDAsVal,
                // because they are unique in whole module and aren't
                // recreated/changed during compilation of shader (it doesn't change pointer)
                map_Var[MDDILocalVariable].var_m = var_m;

                // Map in code any refrence to this variable (for metrics)
                // by adding callinstr llvm.igc.metric.trackValue in module for tracking
                makeTrackCall(
                    funcTrackValue, {MDValue, MDDILocalVariable}, pInstr);

                map_Var[MDDILocalVariable].varDILocalVariable = MDDILocalVariable;
            }
            else
            {
                var_m = map_Var[MDDILocalVariable].var_m;
            }

            return var_m;
        }
        // Cannot find associated user-variable with this instruction
        return nullptr;
    }

    IGC_METRICS::VarInfo* IGCMetricImpl::GetVarMetric(llvm::Value* pValue)
    {
        auto data = GetVarData(pValue);
        return data ? data->var_m : nullptr;
    }

    struct VarData* IGCMetricImpl::GetVarData(llvm::Value* pValue)
    {
        // iterate over all user variables which we found
        for (auto trackerVal_i = map_Var.begin();
            trackerVal_i != map_Var.end();
            ++trackerVal_i)
        {
            // The user variables are identified by the MDAsVal,
            // because they are unique in whole module and aren't
            // recreated/changed during compilation of shader (it doesn't change pointer)
            llvm::MetadataAsValue* tracker = (*trackerVal_i).first;

            for (auto user : tracker->users())
            {
                // Check all usage of this MDAsVal and look for the metrics call functions:
                // call void @llvm.igc.metric.trackValue(...)
                if (llvm::CallInst* callInst = dyn_cast<llvm::CallInst>(user))
                {
                    if (callInst->getCalledFunction()->getName().startswith(funcTrackValue))
                    {
                        llvm::Value* trackedValue = callInst->getArgOperand(0);

                        llvm::MetadataAsValue* MDValue = llvm::dyn_cast<llvm::MetadataAsValue>(trackedValue);
                        llvm::ValueAsMetadata* vAsMD = llvm::dyn_cast<llvm::ValueAsMetadata>(MDValue->getMetadata());

                        // Found tracker which looks at defined user variable
                        if (vAsMD && vAsMD->getValue() == pValue)
                        {
                            return &map_Var[tracker];
                        }
                    }
                }
            }
        }
        // Cannot find associated user-variable with this instruction/value
        return nullptr;
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(const llvm::Instruction* const pInstr)
    {
        return GetFuncMetric((llvm::Instruction*)pInstr);
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(llvm::Instruction* pInstr)
    {
        auto func_m = GetFuncMetric(&pInstr->getDebugLoc());
        if (func_m != nullptr)
        {
            return func_m;
        }
        return GetFuncMetric(pInstr->getParent()->getParent());
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(llvm::Loop* pLoop)
    {
        auto func_m = GetFuncMetric(pLoop->getStartLoc());
        if (func_m != nullptr)
        {
            return func_m;
        }
        return GetFuncMetric(pLoop->getBlocks()[0]->getParent());
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(llvm::Function* pFunc)
    {
        return GetFuncMetric(pFunc->getSubprogram());
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(const llvm::DebugLoc* pLoc)
    {
        if (pLoc == nullptr || !pLoc->get())
        {
            return nullptr;
        }
        const MDNode* Scope = pLoc->getInlinedAtScope();
        if (auto* SP = llvm::getDISubprogram(Scope))
        {
            return GetFuncMetric(SP);
        }
        return nullptr;
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(const llvm::DebugLoc& pLoc)
    {
        if (!pLoc.get())
        {
            return nullptr;
        }
        const MDNode* Scope = pLoc.getInlinedAtScope();
        if (auto* SP = llvm::getDISubprogram(Scope))
        {
            return GetFuncMetric(SP);
        }
        return nullptr;
    }

    IGC_METRICS::Function* IGCMetricImpl::GetFuncMetric(llvm::DISubprogram* pFunc)
    {
        if (map_Func.find(pFunc) == map_Func.end())
        {
            return nullptr;
        }
        else
        {
            return map_Func[pFunc];
        }
    }

    void IGCMetricImpl::FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DILexicalBlock* Loc)
    {
        if (Loc == nullptr || Loc->getDirectory().empty() || Loc->getFilename().empty())
        {
            return;
        }
        FillCodeRef(codeRef, GetFullPath(Loc->getDirectory().str(), Loc->getFilename().str()),
            Loc->getLine());
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

    void IGCMetricImpl::FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DIVariable* Var)
    {
        if (Var == nullptr || Var->getDirectory().empty() || Var->getFilename().empty())
        {
            return;
        }
        FillCodeRef(codeRef, GetFullPath(Var->getDirectory().str(), Var->getFilename().str()),
            Var->getLine());
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

    llvm::MetadataAsValue* IGCMetricImpl::makeMDasVal(llvm::Value* Value)
    {
        llvm::MetadataAsValue* MDValue = llvm::MetadataAsValue::get(
            Value->getContext(),
            llvm::ValueAsMetadata::get(Value)
        );
        return MDValue;
    }

#endif

}

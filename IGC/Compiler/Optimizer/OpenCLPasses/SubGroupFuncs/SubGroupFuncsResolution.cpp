/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/SubGroupFuncs/SubGroupFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-sub-group-func-resolution"
#define PASS_DESCRIPTION "Resolves sub group functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SubGroupFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SubGroupFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SubGroupFuncsResolution::ID = 0;

const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BARRIER = "__builtin_IB_sub_group_barrier";
const llvm::StringRef SubGroupFuncsResolution::GET_MAX_SUB_GROUP_SIZE = "__builtin_IB_get_simd_size";
const llvm::StringRef SubGroupFuncsResolution::GET_SUB_GROUP_LOCAL_ID = "__builtin_IB_get_simd_id";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE = "__builtin_IB_simd_shuffle";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_B = "__builtin_IB_simd_shuffle_b";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_C = "__builtin_IB_simd_shuffle_c";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_US = "__builtin_IB_simd_shuffle_us";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_F = "__builtin_IB_simd_shuffle_f";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_H = "__builtin_IB_simd_shuffle_h";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DF = "__builtin_IB_simd_shuffle_df";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DOWN = "__builtin_IB_simd_shuffle_down";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DOWN_US = "__builtin_IB_simd_shuffle_down_us";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DOWN_UC = "__builtin_IB_simd_shuffle_down_uc";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST = "__builtin_IB_simd_broadcast";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST_B = "__builtin_IB_simd_broadcast_b";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST_C = "__builtin_IB_simd_broadcast_c";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST_US = "__builtin_IB_simd_broadcast_us";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST_F = "__builtin_IB_simd_broadcast_f";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST_H = "__builtin_IB_simd_broadcast_h";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_BROADCAST_DF = "__builtin_IB_simd_broadcast_df";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST = "__builtin_IB_simd_clustered_broadcast";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_B =
    "__builtin_IB_simd_clustered_broadcast_b";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_C =
    "__builtin_IB_simd_clustered_broadcast_c";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_US =
    "__builtin_IB_simd_clustered_broadcast_us";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_F =
    "__builtin_IB_simd_clustered_broadcast_f";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_H =
    "__builtin_IB_simd_clustered_broadcast_h";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_DF =
    "__builtin_IB_simd_clustered_broadcast_df";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL = "__builtin_IB_simd_block_read_1_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL = "__builtin_IB_simd_block_read_2_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL = "__builtin_IB_simd_block_read_4_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL = "__builtin_IB_simd_block_read_8_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL_H = "__builtin_IB_simd_block_read_1_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL_H = "__builtin_IB_simd_block_read_2_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL_H = "__builtin_IB_simd_block_read_4_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL_H = "__builtin_IB_simd_block_read_8_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_16_GBL_H = "__builtin_IB_simd_block_read_16_global_h";

const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL_B = "__builtin_IB_simd_block_read_1_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL_B = "__builtin_IB_simd_block_read_2_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL_B = "__builtin_IB_simd_block_read_4_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL_B = "__builtin_IB_simd_block_read_8_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_16_GBL_B = "__builtin_IB_simd_block_read_16_global_b";

const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL_L = "__builtin_IB_simd_block_read_1_global_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL_L = "__builtin_IB_simd_block_read_2_global_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL_L = "__builtin_IB_simd_block_read_4_global_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL_L = "__builtin_IB_simd_block_read_8_global_l";

const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL = "__builtin_IB_simd_block_write_1_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL = "__builtin_IB_simd_block_write_2_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL = "__builtin_IB_simd_block_write_4_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL = "__builtin_IB_simd_block_write_8_global";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL_H = "__builtin_IB_simd_block_write_1_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL_H = "__builtin_IB_simd_block_write_2_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL_H = "__builtin_IB_simd_block_write_4_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL_H = "__builtin_IB_simd_block_write_8_global_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_GBL_H = "__builtin_IB_simd_block_write_16_global_h";

const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL_B = "__builtin_IB_simd_block_write_1_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL_B = "__builtin_IB_simd_block_write_2_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL_B = "__builtin_IB_simd_block_write_4_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL_B = "__builtin_IB_simd_block_write_8_global_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_GBL_B = "__builtin_IB_simd_block_write_16_global_b";

const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL_L = "__builtin_IB_simd_block_write_1_global_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL_L = "__builtin_IB_simd_block_write_2_global_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL_L = "__builtin_IB_simd_block_write_4_global_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL_L = "__builtin_IB_simd_block_write_8_global_l";

const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL = "__builtin_IB_simd_block_read_1_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL = "__builtin_IB_simd_block_read_2_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL = "__builtin_IB_simd_block_read_4_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL = "__builtin_IB_simd_block_read_8_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL_H = "__builtin_IB_simd_block_read_1_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL_H = "__builtin_IB_simd_block_read_2_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL_H = "__builtin_IB_simd_block_read_4_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL_H = "__builtin_IB_simd_block_read_8_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_16_LCL_H = "__builtin_IB_simd_block_read_16_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL_B = "__builtin_IB_simd_block_read_1_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL_B = "__builtin_IB_simd_block_read_2_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL_B = "__builtin_IB_simd_block_read_4_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL_B = "__builtin_IB_simd_block_read_8_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_16_LCL_B = "__builtin_IB_simd_block_read_16_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL_L = "__builtin_IB_simd_block_read_1_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL_L = "__builtin_IB_simd_block_read_2_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL_L = "__builtin_IB_simd_block_read_4_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL_L = "__builtin_IB_simd_block_read_8_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL = "__builtin_IB_simd_block_write_1_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL = "__builtin_IB_simd_block_write_2_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL = "__builtin_IB_simd_block_write_4_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL = "__builtin_IB_simd_block_write_8_local";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL_H = "__builtin_IB_simd_block_write_1_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL_H = "__builtin_IB_simd_block_write_2_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL_H = "__builtin_IB_simd_block_write_4_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL_H = "__builtin_IB_simd_block_write_8_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_LCL_H = "__builtin_IB_simd_block_write_16_local_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL_B = "__builtin_IB_simd_block_write_1_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL_B = "__builtin_IB_simd_block_write_2_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL_B = "__builtin_IB_simd_block_write_4_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL_B = "__builtin_IB_simd_block_write_8_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_LCL_B = "__builtin_IB_simd_block_write_16_local_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL_L = "__builtin_IB_simd_block_write_1_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL_L = "__builtin_IB_simd_block_write_2_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL_L = "__builtin_IB_simd_block_write_4_local_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL_L = "__builtin_IB_simd_block_write_8_local_l";

const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1 = "__builtin_IB_simd_media_block_read_1";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2 = "__builtin_IB_simd_media_block_read_2";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4 = "__builtin_IB_simd_media_block_read_4";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8 = "__builtin_IB_simd_media_block_read_8";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1_H = "__builtin_IB_simd_media_block_read_1_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2_H = "__builtin_IB_simd_media_block_read_2_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4_H = "__builtin_IB_simd_media_block_read_4_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8_H = "__builtin_IB_simd_media_block_read_8_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_16_H = "__builtin_IB_simd_media_block_read_16_h";

const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1_B = "__builtin_IB_simd_media_block_read_1_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2_B = "__builtin_IB_simd_media_block_read_2_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4_B = "__builtin_IB_simd_media_block_read_4_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8_B = "__builtin_IB_simd_media_block_read_8_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_16_B = "__builtin_IB_simd_media_block_read_16_b";

const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1_L = "__builtin_IB_simd_media_block_read_1_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2_L = "__builtin_IB_simd_media_block_read_2_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4_L = "__builtin_IB_simd_media_block_read_4_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8_L = "__builtin_IB_simd_media_block_read_8_l";

const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1 = "__builtin_IB_simd_media_block_write_1";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2 = "__builtin_IB_simd_media_block_write_2";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4 = "__builtin_IB_simd_media_block_write_4";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8 = "__builtin_IB_simd_media_block_write_8";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1_H = "__builtin_IB_simd_media_block_write_1_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2_H = "__builtin_IB_simd_media_block_write_2_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4_H = "__builtin_IB_simd_media_block_write_4_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8_H = "__builtin_IB_simd_media_block_write_8_h";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_16_H = "__builtin_IB_simd_media_block_write_16_h";

const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1_L = "__builtin_IB_simd_media_block_write_1_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2_L = "__builtin_IB_simd_media_block_write_2_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4_L = "__builtin_IB_simd_media_block_write_4_l";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8_L = "__builtin_IB_simd_media_block_write_8_l";

const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1_B = "__builtin_IB_simd_media_block_write_1_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2_B = "__builtin_IB_simd_media_block_write_2_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4_B = "__builtin_IB_simd_media_block_write_4_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8_B = "__builtin_IB_simd_media_block_write_8_b";
const llvm::StringRef SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_16_B = "__builtin_IB_simd_media_block_write_16_b";

const llvm::StringRef SubGroupFuncsResolution::MEDIA_BLOCK_READ = "__builtin_IB_media_block_read";
const llvm::StringRef SubGroupFuncsResolution::MEDIA_BLOCK_WRITE = "__builtin_IB_media_block_write";

const llvm::StringRef SubGroupFuncsResolution::MEDIA_BLOCK_RECTANGLE_READ = "__builtin_IB_media_block_rectangle_read";
const llvm::StringRef SubGroupFuncsResolution::GET_IMAGE_BTI = "__builtin_IB_get_image_bti";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_REDUCE = "__builtin_IB_sub_group_reduce";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_SCAN = "__builtin_IB_sub_group_scan";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_REDUCE = "__builtin_IB_sub_group_clustered_reduce";
const llvm::StringRef SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_SCAN = "__builtin_IB_sub_group_clustered_scan";

const std::array<std::pair<std::string, WaveOps>, 13> SubGroupFuncsResolution::m_spvOpToWaveOpMap = {
    {{"IAdd", WaveOps::SUM},
     {"FAdd", WaveOps::FSUM},
     {"SMax", WaveOps::IMAX},
     {"UMax", WaveOps::UMAX},
     {"FMax", WaveOps::FMAX},
     {"SMin", WaveOps::IMIN},
     {"UMin", WaveOps::UMIN},
     {"FMin", WaveOps::FMIN},
     {"IMul", WaveOps::PROD},
     {"FMul", WaveOps::FPROD},
     {"And", WaveOps::AND},
     {"Or", WaveOps::OR},
     {"Xor", WaveOps::XOR}}};

SubGroupFuncsResolution::SubGroupFuncsResolution(void) : FunctionPass(ID) {
  initializeSubGroupFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool SubGroupFuncsResolution::runOnFunction(Function &F) {
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  m_argIndexMap.clear();
  m_instsToDelete.clear();
  m_changed = false;

  visit(F);

  for (Instruction *inst : m_instsToDelete) {
    inst->eraseFromParent();
  }

  return m_changed;
}

// Helps to obtain temporary index corresponding to the kernel argument.
// This index will be used during codegen to resolve BTIs for Images (SRVs and UAVs).
void SubGroupFuncsResolution::BTIHelper(llvm::CallInst &CI) {
  Function *F = CI.getParent()->getParent();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  for (Function::arg_iterator arg = F->arg_begin(), e = F->arg_end(); arg != e; ++arg) {
    int argNo = (*arg).getArgNo();
    FunctionMetaData *funcMD = &modMD->FuncMD[F];
    ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
    IGC_ASSERT_MESSAGE((size_t)argNo < resAllocMD->argAllocMDList.size(), "ArgAllocMD List Out of Bounds");
    ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[argNo];
    m_argIndexMap[&(*arg)] = CImagesBI::ParamInfo(argAlloc->indexType, (ResourceTypeEnum)argAlloc->type,
                                                  (ResourceExtensionTypeEnum)argAlloc->extensionType);
  }
}

int32_t SubGroupFuncsResolution::GetSIMDSize(Function *F) {
  auto *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto funcInfoMD = pMdUtils->getFunctionsInfoItem(F);
  int32_t simdSize = funcInfoMD->getSubGroupSize()->getSIMDSize();

  return simdSize;
}

void SubGroupFuncsResolution::CheckSIMDSize(Instruction &I, StringRef msg) {
  int32_t simdSize = GetSIMDSize(I.getParent()->getParent());

  if ((simdSize == 32 || m_pCtx->getModuleMetaData()->csInfo.forcedSIMDSize == 32) &&
      m_pCtx->platform.getGRFSize() != 64) {
    m_pCtx->EmitError(std::string(msg).c_str(), &I);
  }
}

WaveOps SubGroupFuncsResolution::GetWaveOp(StringRef funcName) {
  for (const auto &op : SubGroupFuncsResolution::m_spvOpToWaveOpMap) {
    if (funcName.contains(op.first)) {
      return op.second;
    }
  }
  IGC_ASSERT_MESSAGE(0, "Function name does not contain spir-v operation type");
  return WaveOps::UNDEF;
}

void SubGroupFuncsResolution::mediaBlockRead(llvm::CallInst &CI) {
  // Creates intrinsics that will be lowered in the CodeGen and will handle the simd_media_block_read
  SmallVector<Value *, 5> args;
  pushMediaBlockArgs(args, CI);

  // Check if the only use of CI is conversion to float. If so, use float version of intrinsic and remove the cast
  // instruction.

  Value *use = NULL;
  if (CI.hasOneUse()) {
    use = *(CI.user_begin());
  }

  if (use && isa<BitCastInst>(use) &&
      (use->getType()->getScalarType()->isFloatTy() || use->getType()->getScalarType()->isHalfTy())) {
    Type *types[] = {use->getType(), CI.getArgOperand(0)->getType()};
    BitCastInst *bitCast = cast<BitCastInst>(use);
    Function *simdMediaBlockReadFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_simdMediaBlockRead, types);
    Instruction *simdMediaBlockRead = CallInst::Create(simdMediaBlockReadFunc, args, "", &CI);
    use->replaceAllUsesWith(simdMediaBlockRead);
    updateDebugLoc(&CI, simdMediaBlockRead);
    m_instsToDelete.push_back(bitCast);
    m_instsToDelete.push_back(&CI);
  } else {
    Type *types[] = {CI.getType(), CI.getArgOperand(0)->getType()};
    Function *simdMediaBlockReadFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_simdMediaBlockRead, types);
    Instruction *simdMediaBlockRead = CallInst::Create(simdMediaBlockReadFunc, args, "", &CI);
    updateDebugLoc(&CI, simdMediaBlockRead);
    CI.replaceAllUsesWith(simdMediaBlockRead);
    CI.eraseFromParent();
  }
}

void SubGroupFuncsResolution::mediaBlockWrite(llvm::CallInst &CI) {
  SmallVector<Value *, 5> args;
  pushMediaBlockArgs(args, CI);
  args.push_back(CI.getArgOperand(2)); // push data

  Type *types[] = {CI.getArgOperand(0)->getType(), CI.getArgOperand(2)->getType()};
  Function *simdMediaBlockWriteFunc = GenISAIntrinsic::getDeclaration(
      CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_simdMediaBlockWrite, types);
  Instruction *simdMediaBlockWrite = CallInst::Create(simdMediaBlockWriteFunc, args, "", &CI);
  updateDebugLoc(&CI, simdMediaBlockWrite);

  CI.replaceAllUsesWith(simdMediaBlockWrite);
  CI.eraseFromParent();
}

// If CI parameter is %"class.sycl::_V1::ext::oneapi::bfloat16" type, which is { i16 },
// then we need to cast it to i16 type before calling simdBlockRead intrinsic.
static inline Value *castSYCLBFloat16toi16(PointerType *PtrTy, Value *Ptr, CallInst &CI, LLVMContext &C) {
  if (IGCLLVM::isPointerTy(PtrTy))
    return Ptr;

  if (StructType *ST = dyn_cast<StructType>(IGCLLVM::getNonOpaquePtrEltTy(PtrTy))) // Legacy code: getNonOpaquePtrEltTy
  {
    // check if ST has only field and this field is i16 type
    if (ST->getNumElements() == 1 && ST->getElementType(0)->isIntegerTy(16)) {
      return CastInst::CreatePointerCast(Ptr, PointerType::get(Type::getInt16Ty(C), PtrTy->getAddressSpace()), "", &CI);
    }
  }

  return Ptr;
}

void SubGroupFuncsResolution::simdBlockRead(llvm::CallInst &CI, bool hasCacheControls) {
  // Creates intrinsics that will be lowered in the CodeGen and will handle the simd_block_read
  LLVMContext &C = CI.getCalledFunction()->getContext();
  Value *Ptr = CI.getArgOperand(0);
  PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType());
  IGC_ASSERT_MESSAGE(PtrTy, "simdBlockRead has non-pointer type!");
  SmallVector<Value *, 1> args;
  args.push_back(castSYCLBFloat16toi16(PtrTy, Ptr, CI, C));
  SmallVector<Type *, 3> types;
  types.push_back(nullptr);
  types.push_back(nullptr);
  GenISAIntrinsic::ID genIntrinID = GenISAIntrinsic::GenISA_simdBlockRead;
  ADDRESS_SPACE AS = (ADDRESS_SPACE)PtrTy->getAddressSpace();
  bool supportLocal = false;
  supportLocal = m_pCtx->platform.supportSLMBlockMessage();
  if (AS == ADDRESS_SPACE_LOCAL && !supportLocal) {
    m_pCtx->EmitError("BlockReadLocal not supported!", &CI);
    return;
  }

  unsigned int scalarSizeInBits = CI.getType()->getScalarType()->getScalarSizeInBits();

  switch (scalarSizeInBits) {
  case 8:
    types[1] = Type::getInt8PtrTy(C, AS);
    break;
  case 16:
    types[1] = Type::getInt16PtrTy(C, AS);
    break;
  case 64:
    types[1] = (Type::getInt64PtrTy(C, AS));
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "unrecognized bit width!");
    // assertion failed but continue code failsafe using default 32
  case 32:
    types[1] = (Type::getInt32PtrTy(C, AS));
    break;
  }

  // Check if the only use of CI is conversion to float. If so, use float version of intrinsic and remove the cast
  // instruction.

  Value *use = NULL;
  if (CI.hasOneUse()) {
    use = *(CI.user_begin());
  }

  MDNode *CacheCtrlNode = nullptr;
  if (hasCacheControls) {
    int cacheControlValue = (cast<ConstantInt>(CI.getArgOperand(1)))->getZExtValue();
    CacheCtrlNode = MDNode::get(C, ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), cacheControlValue)));
  }

  Instruction *simdBlockRead = nullptr;
  if (use && isa<BitCastInst>(use) &&
      ((use->getType()->getScalarType()->isFloatTy() && scalarSizeInBits == 32) ||
       (use->getType()->getScalarType()->isDoubleTy() && scalarSizeInBits == 64))) {
    BitCastInst *bitCast = cast<BitCastInst>(use);
    types[0] = bitCast->getType();
    Function *simdBlockReadFunc =
        GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(), genIntrinID, types);
    simdBlockRead = CallInst::Create(simdBlockReadFunc, args, "", &CI);
    updateDebugLoc(&CI, simdBlockRead);
    use->replaceAllUsesWith(simdBlockRead);
    m_instsToDelete.push_back(bitCast);
    m_instsToDelete.push_back(&CI);
  } else {
    types[0] = CI.getType();
    Function *simdBlockReadFunc =
        GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(), genIntrinID, types);
    simdBlockRead = CallInst::Create(simdBlockReadFunc, args, "", &CI);
    updateDebugLoc(&CI, simdBlockRead);
    CI.replaceAllUsesWith(simdBlockRead);
    CI.eraseFromParent();
  }

  IGC_ASSERT(simdBlockRead);

  if (CacheCtrlNode) {
    simdBlockRead->setMetadata("lsc.cache.ctrl", CacheCtrlNode);
  }
}

void SubGroupFuncsResolution::simdBlockWrite(llvm::CallInst &CI, bool hasCacheControls) {
  LLVMContext &C = CI.getCalledFunction()->getContext();
  Value *Ptr = CI.getArgOperand(0);
  PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType());
  IGC_ASSERT_MESSAGE(PtrTy, "simdBlockWrite has non-pointer type!");
  ADDRESS_SPACE AS = (ADDRESS_SPACE)PtrTy->getAddressSpace();
  bool supportLocal = false;
  supportLocal = m_pCtx->platform.supportSLMBlockMessage();
  if (AS == ADDRESS_SPACE_LOCAL && !supportLocal) {
    m_pCtx->EmitError("BlockWriteLocal not supported!", &CI);
    return;
  }

  SmallVector<Value *, 2> args;
  SmallVector<Type *, 2> types;
  Value *dataArg = CI.getArgOperand(1);

  args.push_back(castSYCLBFloat16toi16(PtrTy, Ptr, CI, C));
  args.push_back(dataArg);

  switch (dataArg->getType()->getScalarType()->getScalarSizeInBits()) {
  case 8:
    types.push_back(Type::getInt8PtrTy(C, AS));
    break;
  case 16:
    types.push_back(Type::getInt16PtrTy(C, AS));
    break;
  case 64:
    types.push_back(Type::getInt64PtrTy(C, AS));
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "unrecognized bit width!");
    // assertion failed but continue code failsafe using default 32
  case 32:
    types.push_back(Type::getInt32PtrTy(C, AS));
    break;
  }

  types.push_back(dataArg->getType());
  Function *simdBlockWriteFunc = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                                 GenISAIntrinsic::GenISA_simdBlockWrite, types);
  Instruction *simdBlockWrite = CallInst::Create(simdBlockWriteFunc, args, "", &CI);
  updateDebugLoc(&CI, simdBlockWrite);

  if (hasCacheControls) {
    int cacheControlValue = (cast<ConstantInt>(CI.getArgOperand(2)))->getZExtValue();
    MDNode *CacheCtrlNode =
        MDNode::get(C, ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), cacheControlValue)));
    simdBlockWrite->setMetadata("lsc.cache.ctrl", CacheCtrlNode);
  }

  CI.replaceAllUsesWith(simdBlockWrite);
  CI.eraseFromParent();
}

void SubGroupFuncsResolution::pushMediaBlockArgs(llvm::SmallVector<llvm::Value *, 5> &args, llvm::CallInst &CI) {
  LLVMContext &C = CI.getCalledFunction()->getContext();

  if (m_argIndexMap.empty()) {
    BTIHelper(CI);
  }

  Value *image;
  if (m_pCtx->getModuleMetaData()->UseBindlessImage) {
    image = CI.getArgOperand(0);
  } else {
    Value *pImg = nullptr;
    image = IGC::CImagesBI::CImagesUtils::getImageIndex(&m_argIndexMap, &CI, 0, pImg);
  }

  ConstantInt *constIndex = ConstantInt::get((Type::getInt32Ty(C)), 0);
  Instruction *xOffset = ExtractElementInst::Create(CI.getArgOperand(1), constIndex, "xOffset", &CI);

  ConstantInt *constIndex2 = ConstantInt::get((Type::getInt32Ty(C)), 1);
  Instruction *yOffset = ExtractElementInst::Create(CI.getArgOperand(1), constIndex2, "yOffset", &CI);

  BufferType imageType = IGC::CImagesBI::CImagesUtils::getImageType(&m_argIndexMap, &CI, 0);
  uint32_t isUAV = imageType == UAV ? 1 : 0;
  ConstantInt *isImageTypeUAV = ConstantInt::get((Type::getInt32Ty(C)), isUAV);

  updateDebugLoc(&CI, xOffset);
  updateDebugLoc(&CI, yOffset);

  args.push_back(image);
  args.push_back(xOffset);
  args.push_back(yOffset);
  args.push_back(isImageTypeUAV);
}

void SubGroupFuncsResolution::subGroupArithmetic(CallInst &CI, WaveOps op, GroupOpType groupType) {
  IRBuilder<> IRB(&CI);
  IRB.SetCurrentDebugLocation(CI.getDebugLoc());

  Value *arg = CI.getArgOperand(0);
  // GenISA_Wave* instrinsics do not support i1 type. Handle this with i8 version of instrinsic.
  bool isBoolean = (arg->getType() == IRB.getInt1Ty());
  if (isBoolean) {
    arg = IRB.CreateZExt(arg, IRB.getInt8Ty());
  }
  Value *opVal = IRB.getInt8((uint8_t)op);
  Value *waveCall = nullptr;
  if (groupType == GroupOperationReduce) {
    Value *args[] = {arg, opVal, IRB.getInt1(true), IRB.getInt32(0)};
    Function *waveAll = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                        GenISAIntrinsic::GenISA_WaveAll, arg->getType());
    waveCall = IRB.CreateCall(waveAll, args);
  } else if (groupType == GroupOperationScan) {
    Value *args[5] = {arg, opVal, IRB.getInt1(false), IRB.getInt1(true), IRB.getInt32(0)};
    Function *waveScan = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                         GenISAIntrinsic::GenISA_WavePrefix, arg->getType());
    waveCall = IRB.CreateCall(waveScan, args);
  } else if (groupType == GroupOperationClusteredScan) {
    Value *args[4] = {arg, opVal, CI.getArgOperand(1), IRB.getInt32(0)};
    Function *waveClusteredScan = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_WaveClusteredPrefix, arg->getType());
    waveCall = IRB.CreateCall(waveClusteredScan, args);
  } else if (groupType == GroupOperationClusteredReduce) {
    Value *clusterSize = CI.getOperand(1);
    Value *args[4] = {arg, opVal, clusterSize, IRB.getInt32(0)};
    Function *waveClustered = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                              GenISAIntrinsic::GenISA_WaveClustered, arg->getType());
    waveCall = IRB.CreateCall(waveClustered, args);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unsupported group operation type!");
  }

  if (isBoolean) {
    waveCall = IRB.CreateTrunc(waveCall, IRB.getInt1Ty());
  }
  CI.replaceAllUsesWith(waveCall);
  CI.eraseFromParent();
}

void SubGroupFuncsResolution::visitCallInst(CallInst &CI) {
  Function *func = CI.getCalledFunction();
  if (!func)
    return;
  StringRef funcName = func->getName();
  LLVMContext &Ctx = CI.getCalledFunction()->getContext();

  if (funcName.equals(SubGroupFuncsResolution::GET_MAX_SUB_GROUP_SIZE)) {
    int32_t simdSize = GetSIMDSize(CI.getParent()->getParent());
    if (simdSize == 8 || simdSize == 16 || simdSize == 32) {
      auto *C = ConstantInt::get(Type::getInt32Ty(Ctx), simdSize);
      CI.replaceAllUsesWith(C);
    } else {
      // Creates intrinsics that will be lowered in the CodeGen and will handle the sub_group size
      Function *simdSizeFunc =
          GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_simdSize);
      Instruction *simdSize = CallInst::Create(simdSizeFunc, "simdSize", &CI);
      updateDebugLoc(&CI, simdSize);
      CI.replaceAllUsesWith(simdSize);
    }
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::GET_SUB_GROUP_LOCAL_ID)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the sub_group_local_id
    IntegerType *typeInt32 = Type::getInt32Ty(Ctx);

    Function *simdLaneIdFunc =
        GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_simdLaneId);
    Instruction *simdLaneId16 = CallInst::Create(simdLaneIdFunc, "simdLaneId16", &CI);
    Instruction *simdLaneId = ZExtInst::CreateIntegerCast(simdLaneId16, typeInt32, false, "simdLaneId", &CI);
    updateDebugLoc(&CI, simdLaneId16);
    updateDebugLoc(&CI, simdLaneId);
    CI.replaceAllUsesWith(simdLaneId);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_US) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_F) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_H) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_C) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_B) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DF)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the sub_group_shuffle function
    IRBuilder<> IRB(&CI);
    Value *args[3];
    args[0] = CI.getArgOperand(0);
    args[1] = CI.getArgOperand(1);
    args[2] = IRB.getInt32(0);

    Function *simdShuffleFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_WaveShuffleIndex, args[0]->getType());
    Instruction *simdShuffle = CallInst::Create(simdShuffleFunc, args, "simdShuffle", &CI);
    updateDebugLoc(&CI, simdShuffle);
    CI.replaceAllUsesWith(simdShuffle);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST_US) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST_F) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST_H) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST_C) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST_B) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_BROADCAST_DF)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the sub_group_broadcast function
    IRBuilder<> IRB(&CI);
    Value *args[3];
    args[0] = CI.getArgOperand(0);
    args[1] = CI.getArgOperand(1);
    args[2] = IRB.getInt32(0);

    Function *simdBroadcastFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_WaveBroadcast, args[0]->getType());
    Instruction *simdBroadcast = CallInst::Create(simdBroadcastFunc, args, "simdBroadcast", &CI);
    updateDebugLoc(&CI, simdBroadcast);
    CI.replaceAllUsesWith(simdBroadcast);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_US) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_F) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_H) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_C) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_B) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_BROADCAST_DF)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the sub_group_clustered_broadcast function
    IRBuilder<> IRB(&CI);
    Value *args[4];
    args[0] = CI.getArgOperand(0);
    args[1] = CI.getArgOperand(1);
    args[2] = CI.getArgOperand(2);
    args[3] = IRB.getInt32(0);

    if (!isa<ConstantInt>(args[1])) {
      m_pCtx->EmitError("cluster_size argument in clustered_broadcast must be constant.", &CI);
      return;
    }
    if (!isa<ConstantInt>(args[2])) {
      m_pCtx->EmitError("in_cluster_lane argument in clustered_broadcast must be constant.", &CI);
      return;
    }

    Function *simdClusteredBroadcastFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_WaveClusteredBroadcast, args[0]->getType());
    Instruction *simdClusteredBroadcast =
        CallInst::Create(simdClusteredBroadcastFunc, args, "simdClusteredBroadcast", &CI);
    updateDebugLoc(&CI, simdClusteredBroadcast);
    CI.replaceAllUsesWith(simdClusteredBroadcast);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DOWN) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DOWN_US) ||
             funcName.equals(SubGroupFuncsResolution::SUB_GROUP_SHUFFLE_DOWN_UC)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the sub_group_shuffle_down function
    Value *args[3];
    args[0] = CI.getArgOperand(0);
    args[1] = CI.getArgOperand(1);
    args[2] = CI.getArgOperand(2);

    Function *simdShuffleDownFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_simdShuffleDown, args[0]->getType());
    Instruction *simdShuffleDown = CallInst::Create(simdShuffleDownFunc, args, "simdShuffleDown", &CI);
    updateDebugLoc(&CI, simdShuffleDown);
    CI.replaceAllUsesWith(simdShuffleDown);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_16_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_16_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_GBL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_GBL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_GBL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_GBL_L)) {
    CheckSIMDSize(CI, "Block reads not supported in SIMD32");
    simdBlockRead(CI);
  } else if (funcName.startswith("__builtin_IB_cache_controls_simd_block_read")) {
    CheckSIMDSize(CI, "Block reads not supported in SIMD32");
    simdBlockRead(CI, true);
  } else if (funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_GBL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_GBL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_GBL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_GBL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_GBL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_GBL_L)) {
    CheckSIMDSize(CI, "Block writes not supported in SIMD32");
    simdBlockWrite(CI);
  } else if (funcName.startswith("__builtin_IB_cache_controls_simd_block_write")) {
    CheckSIMDSize(CI, "Block writes not supported in SIMD32");
    simdBlockWrite(CI, true);
  } else if (funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_16_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_16_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_1_LCL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_2_LCL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_4_LCL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_READ_8_LCL_L)) {
    CheckSIMDSize(CI, "Block reads not supported in SIMD32");
    simdBlockRead(CI);
  } else if (funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_LCL_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_16_LCL_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_1_LCL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_2_LCL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_4_LCL_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_BLOCK_WRITE_8_LCL_L)) {
    CheckSIMDSize(CI, "Block writes not supported in SIMD32");
    simdBlockWrite(CI);
  } else if (funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_16_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_16_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_1_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_2_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_4_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_READ_8_L)) {
    CheckSIMDSize(CI, "SIMD Media Block Read not supported in SIMD32");
    mediaBlockRead(CI);
  } else if (funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_16_B) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_16_H) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_1_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_2_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_4_L) ||
             funcName.equals(SubGroupFuncsResolution::SIMD_MEDIA_BLOCK_WRITE_8_L)) {
    CheckSIMDSize(CI, "SIMD Media Block Write not supported in SIMD32");
    mediaBlockWrite(CI);
  } else if (funcName.startswith(SubGroupFuncsResolution::MEDIA_BLOCK_READ)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the media_block_read

    SmallVector<Value *, 5> args;
    pushMediaBlockArgs(args, CI);

    // The spec requires that the width and height are compile-time constants.
    Value *blockWidth = CI.getOperand(2);
    if (!isa<ConstantInt>(blockWidth)) {
      blockWidth = ValueTracker::track(&CI, 2);
      if (!blockWidth) {
        m_pCtx->EmitError("width argument supplied to intel_media_block_read*() must be constant.", &CI);
        return;
      }
    }

    Value *blockHeight = CI.getOperand(3);
    if (!isa<ConstantInt>(blockHeight)) {
      blockHeight = ValueTracker::track(&CI, 3);
      if (!blockHeight) {
        m_pCtx->EmitError("height argument supplied to intel_media_block_read*() must be constant.", &CI);
        return;
      }
    }

    args.push_back(blockWidth);
    args.push_back(blockHeight);

    Type *types[] = {CI.getCalledFunction()->getReturnType(), CI.getArgOperand(0)->getType()};
    Function *MediaBlockReadFunc = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                                   GenISAIntrinsic::GenISA_MediaBlockRead, types);

    auto *MediaBlockRead = cast<GenIntrinsicInst>(CallInst::Create(MediaBlockReadFunc, args, "", &CI));
    MediaBlockRead->setDebugLoc(CI.getDebugLoc());

    CheckMediaBlockInstError(MediaBlockRead, true);
    // Return if any error
    if (m_pCtx->HasError()) {
      return;
    }

    CI.replaceAllUsesWith(MediaBlockRead);
    CI.eraseFromParent();
  } else if (funcName.startswith(SubGroupFuncsResolution::MEDIA_BLOCK_WRITE)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the media_block_write

    SmallVector<Value *, 5> args;
    pushMediaBlockArgs(args, CI);

    // The spec requires that the width and height are compile-time constants.
    Value *blockWidth = CI.getOperand(2);
    if (!isa<ConstantInt>(blockWidth)) {
      blockWidth = ValueTracker::track(&CI, 2);
      if (!blockWidth) {
        m_pCtx->EmitError("width argument supplied to intel_media_block_write*() must be constant.", &CI);
        return;
      }
    }

    Value *blockHeight = CI.getOperand(3);
    if (!isa<ConstantInt>(blockHeight)) {
      blockHeight = ValueTracker::track(&CI, 3);
      if (!blockHeight) {
        m_pCtx->EmitError("height argument supplied to intel_media_block_write*() must be constant.", &CI);
        return;
      }
    }

    args.push_back(blockWidth);
    args.push_back(blockHeight);
    args.push_back(CI.getArgOperand(4)); // pixels

    Type *types[] = {CI.getArgOperand(0)->getType(), CI.getArgOperand(4)->getType()};
    Function *MediaBlockWriteFunc = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                                    GenISAIntrinsic::GenISA_MediaBlockWrite, types);

    auto *MediaBlockWrite = cast<GenIntrinsicInst>(CallInst::Create(MediaBlockWriteFunc, args, "", &CI));
    MediaBlockWrite->setDebugLoc(CI.getDebugLoc());

    CheckMediaBlockInstError(MediaBlockWrite, false);
    // Return if any error
    if (m_pCtx->HasError()) {
      return;
    }

    CI.replaceAllUsesWith(MediaBlockWrite);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::MEDIA_BLOCK_RECTANGLE_READ)) {
    // Creates intrinsics that will be lowered in the CodeGen and will handle the simd_media_block_read_8
    SmallVector<Value *, 5> args;
    pushMediaBlockArgs(args, CI);

    args.push_back(CI.getArgOperand(2)); // blockWidth
    args.push_back(CI.getArgOperand(3)); // blockHeight
    args.push_back(CI.getArgOperand(4)); // destination

    Function *MediaBlockRectangleReadFunc = GenISAIntrinsic::getDeclaration(
        CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_MediaBlockRectangleRead);
    Instruction *MediaBlockRectangleRead = CallInst::Create(MediaBlockRectangleReadFunc, args, "", &CI);
    updateDebugLoc(&CI, MediaBlockRectangleRead);
    CI.replaceAllUsesWith(MediaBlockRectangleRead);
    CI.eraseFromParent();
  } else if (funcName.equals(SubGroupFuncsResolution::GET_IMAGE_BTI)) {
    if (m_argIndexMap.empty()) {
      BTIHelper(CI);
    }

    Value *pImg = nullptr;
    ConstantInt *imageIndex = IGC::CImagesBI::CImagesUtils::getImageIndex(&m_argIndexMap, &CI, 0, pImg);

    CI.replaceAllUsesWith(imageIndex);
    CI.eraseFromParent();
  } else if (funcName.startswith(SubGroupFuncsResolution::SUB_GROUP_REDUCE)) {
    return subGroupArithmetic(CI, GetWaveOp(funcName), GroupOperationReduce);
  } else if (funcName.startswith(SubGroupFuncsResolution::SUB_GROUP_SCAN)) {
    return subGroupArithmetic(CI, GetWaveOp(funcName), GroupOperationScan);
  } else if (funcName.startswith(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_REDUCE)) {
    return subGroupArithmetic(CI, GetWaveOp(funcName), GroupOperationClusteredReduce);
  } else if (funcName.startswith(SubGroupFuncsResolution::SUB_GROUP_CLUSTERED_SCAN)) {
    return subGroupArithmetic(CI, GetWaveOp(funcName), GroupOperationClusteredScan);
  } else if (funcName.startswith(SubGroupFuncsResolution::SUB_GROUP_BARRIER)) {
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    // Subgroup barrier is a no-op in HW.
    // For -O0 a dummy instruction is generated in order to preserve debug info.
    if (modMD->compOpt.OptDisable) {
      Function *dummyInst =
          GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_dummyInst);
      auto *dummyIntrinsic = cast<GenIntrinsicInst>(CallInst::Create(dummyInst, "", &CI));
      dummyIntrinsic->setDebugLoc(CI.getDebugLoc());
      CI.eraseFromParent();
    } else {
      Function *waveBarrier =
          GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_wavebarrier);
      auto waveBarrierInst = CallInst::Create(waveBarrier, "", &CI);
      updateDebugLoc(&CI, waveBarrierInst);
      CI.eraseFromParent();
    }
  } else {
    // Non Sub Group function, do nothing
    return;
  }
  m_changed = true;
}

void SubGroupFuncsResolution::CheckMediaBlockInstError(llvm::GenIntrinsicInst *inst, bool isRead) {
  Function *F = inst->getParent()->getParent();

  // Width and height must be supplied as compile time constants.
  uint blockWidth = (uint)cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
  uint blockHeight = (uint)cast<ConstantInt>(inst->getOperand(5))->getZExtValue();

  // Code to extract subgroup size
  IGC::IGCMD::MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(F);
  unsigned int subGrpSize = funcInfoMD->getSubGroupSize()->getSIMDSize();

  auto *pFunc = inst->getCalledFunction();
  auto *pDataType = isRead ? pFunc->getReturnType() : inst->getOperand(6)->getType();

  const llvm::DataLayout *DL = &F->getParent()->getDataLayout();

  uint typeSize = isa<VectorType>(pDataType)
                      ? (uint)DL->getTypeSizeInBits(cast<VectorType>(pDataType)->getElementType()) / 8
                      : (uint)DL->getTypeSizeInBits(pDataType) / 8;

  uint widthInBytes = blockWidth * typeSize;
  uint IOSize = widthInBytes * blockHeight;

  // Determine max rows that can be read by hardware for the given width.
  uint maxRows = 0;
  if (widthInBytes <= 4) {
    maxRows = 64;
  } else if (widthInBytes <= 8) {
    maxRows = 32;
  } else if (widthInBytes <= 16) {
    maxRows = 16;
  } else {
    maxRows = 8;
  }

  {
    std::string builtinPrefix = isRead ? "intel_media_block_read" : "intel_media_block_write";

    if (widthInBytes > 32) // hardware restriction on block read width
    {
      std::string output;
      raw_string_ostream S(output);
      S << "width for " << builtinPrefix << "*() must be <= " << 32 / typeSize;
      S.flush();
      m_pCtx->EmitError(output.c_str(), inst);
      return;
    }

    if (blockHeight > maxRows) // hardware restriction on block read height
    {
      std::string output;
      raw_string_ostream S(output);
      S << "height for " << widthInBytes << " bytes wide " << builtinPrefix << "*() must be <= " << maxRows;
      S.flush();
      m_pCtx->EmitError(output.c_str(), inst);
      return;
    }

    if (subGrpSize != 0) {
      uint maxIOSize = subGrpSize * ((uint)DL->getTypeSizeInBits(pDataType) / 8);

      if (IOSize > maxIOSize) {
        std::string output;
        raw_string_ostream S(output);
        S << builtinPrefix << "*() attempt of " << IOSize << " bytes.  Must be <= " << maxIOSize << " bytes.";
        S.flush();
        m_pCtx->EmitError(output.c_str(), inst);
        return;
      }
    }

    if (widthInBytes % 4 != 0) {
      std::string output;
      raw_string_ostream S(output);
      if (typeSize == 1) {
        S << builtinPrefix << "_uc*() widths must be quad pixel aligned.";
      } else {
        S << builtinPrefix << "_us*() widths must be dual pixel aligned.";
      }
      S.flush();
      m_pCtx->EmitError(output.c_str(), inst);
      return;
    }
  }
}

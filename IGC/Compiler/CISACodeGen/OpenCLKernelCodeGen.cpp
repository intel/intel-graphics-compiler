/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/ADT/StringExtras.h"
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "Compiler/CISACodeGen/CSWalkOrder.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "AdaptorOCL/OCL/KernelAnnotations.hpp"
#include "common/igc_regkeys.hpp"
#include "common/Stats.hpp"
#include "common/SystemThread.h"
#include "common/secure_mem.h"
#include "common/MDFrameWork.h"
#include <iStdLib/utility.h>
#include "Probe/Assertion.h"
#include <fstream>
#include "ZEBinWriter/zebin/source/ZEELFObjectBuilder.hpp"

/***********************************************************************************
This file contains the code specific to opencl kernels
************************************************************************************/

namespace IGC {
using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

bool OpenCLProgramContext::isSPIRV() const { return isSpirV; }

void OpenCLProgramContext::setAsSPIRV() { isSpirV = true; }

bool OpenCLProgramContext::needsDivergentBarrierHandling() const {
  return IGC_IS_FLAG_ENABLED(EnableDivergentBarrierWA) || m_InternalOptions.EnableDivergentBarrierHandling;
}

float OpenCLProgramContext::getProfilingTimerResolution() { return m_ProfilingTimerResolution; }

int32_t OpenCLProgramContext::getNumThreadsPerEU() const {
  if (m_Options.IntelRequiredEUThreadCount) {
    return m_Options.requiredEUThreadCount;
  }
  if (m_InternalOptions.IntelNumThreadPerEU) {
    return m_InternalOptions.numThreadsPerEU;
  }

  return -1;
}

uint32_t OpenCLProgramContext::getExpGRFSize() const {
  if (IGC_GET_FLAG_VALUE(TotalGRFNum)) {
    return IGC_GET_FLAG_VALUE(TotalGRFNum);
  }
  if (m_InternalOptions.IntelExpGRFSize) {
    return m_InternalOptions.expGRFSize;
  }
  if (m_Options.IntelExpGRFSize) {
    return m_Options.expGRFSize;
  }

  return 0;
}

uint32_t OpenCLProgramContext::getNumGRFPerThread(bool returnDefault) {
  if (platform.supportsStaticRegSharing()) {
    if (m_InternalOptions.Intel128GRFPerThread || m_Options.Intel128GRFPerThread) {
      return 128;
    } else if (m_InternalOptions.Intel256GRFPerThread || m_Options.Intel256GRFPerThread ||
               m_Options.IntelLargeRegisterFile) {
      return 256;
    } else if (m_InternalOptions.Intel512GRFPerThread || m_Options.Intel512GRFPerThread) {
      return 512;
    }
  }
  return CodeGenContext::getNumGRFPerThread(returnDefault);
}

bool OpenCLProgramContext::isAutoGRFSelectionEnabled() const {
  if (getNumThreadsPerEU() == 0)
    return true;

  if ((platform.supportsAutoGRFSelection() &&
           (m_DriverInfo.supportsAutoGRFSelection() || m_InternalOptions.IntelEnableAutoLargeGRF ||
            m_Options.IntelEnableAutoLargeGRF) ||
       supportsVRT()) &&
      !m_InternalOptions.Intel128GRFPerThread && !m_Options.Intel128GRFPerThread &&
      !m_InternalOptions.Intel256GRFPerThread && !m_Options.Intel256GRFPerThread &&
      !m_InternalOptions.Intel512GRFPerThread && !m_Options.Intel512GRFPerThread) {
    return true;
  }

  return false;
}

bool OpenCLProgramContext::forceGlobalMemoryAllocation() const {
  return m_InternalOptions.ForceGlobalMemoryAllocation || m_hasGlobalInPrivateAddressSpace;
}

bool OpenCLProgramContext::allocatePrivateAsGlobalBuffer() const {
  return forceGlobalMemoryAllocation() ||
         (m_instrTypes.hasDynamicGenericLoadStore && platform.canForcePrivateToGlobal());
}

bool OpenCLProgramContext::noLocalToGenericOptionEnabled() const { return m_Options.NoLocalToGeneric; }

bool OpenCLProgramContext::mustDistinguishBetweenPrivateAndGlobalPtr() const {
  return m_mustDistinguishBetweenPrivateAndGlobalPtr;
}

void OpenCLProgramContext::setDistinguishBetweenPrivateAndGlobalPtr(bool distinguish) {
  m_mustDistinguishBetweenPrivateAndGlobalPtr = distinguish;
}

bool OpenCLProgramContext::enableTakeGlobalAddress() const {
  return m_Options.EnableTakeGlobalAddress || getModuleMetaData()->capabilities.globalVariableDecorationsINTEL;
}

int16_t OpenCLProgramContext::getVectorCoalescingControl() const {
  // cmdline option > registry key
  int val = m_InternalOptions.VectorCoalescingControl;
  if (val < 0) {
    // no cmdline option
    val = IGC_GET_FLAG_VALUE(VectorAlias);
  }
  return val;
}

uint32_t OpenCLProgramContext::getPrivateMemoryMinimalSizePerThread() const {
  return m_InternalOptions.IntelPrivateMemoryMinimalSizePerThread;
}

bool OpenCLProgramContext::isBufferBoundsChecking() const { return m_InternalOptions.EnableBufferBoundsChecking; }

uint32_t OpenCLProgramContext::getIntelScratchSpacePrivateMemoryMinimalSizePerThread() const {
  return m_InternalOptions.IntelScratchSpacePrivateMemoryMinimalSizePerThread;
}

void OpenCLProgramContext::failOnSpills() {
  if (!m_InternalOptions.FailOnSpill) {
    return;
  }
  // If there is fail-on-spill option provided
  // and __attribute__((annotate("igc-do-not-spill"))) is present for a kernel,
  // we fail compilation
  auto &programList = m_programOutput.m_ShaderProgramList;
  for (auto &kernel : programList) {
    for (auto mode : {SIMDMode::SIMD8, SIMDMode::SIMD16, SIMDMode::SIMD32}) {
      COpenCLKernel *shader = static_cast<COpenCLKernel *>(kernel->GetShader(mode));

      if (!COpenCLKernel::IsValidShader(shader)) {
        continue;
      }

      auto &funcMD = modMD->FuncMD[shader->entry];
      auto &annotatnions = funcMD.UserAnnotations;
      auto output = shader->ProgramOutput();

      if (hasSpills(output->m_scratchSpaceUsedBySpills, output->m_numGRFTotal) &&
          std::find(annotatnions.begin(), annotatnions.end(), "igc-do-not-spill") != annotatnions.end()) {
        std::string msg = "Spills detected in kernel: " + shader->m_kernelInfo.m_kernelName;
        EmitError(msg.c_str(), nullptr);
      }
    }
  }
}

float OpenCLProgramContext::GetSpillThreshold(SIMDMode dispatchSize) {
  float threshold = 0.0f;
  if (this->platform.getGRFSize() >= 64) {
    if (dispatchSize == SIMDMode::SIMD32)
      threshold = float(m_DriverInfo.getSIMD32_SpillThreshold()) / 100.0f;
    else if (dispatchSize == SIMDMode::SIMD16)
      threshold = float(m_DriverInfo.getSIMD16_SpillThreshold() * 2) / 100.0f;
  } else {
    if (dispatchSize == SIMDMode::SIMD16)
      threshold = float(m_DriverInfo.getSIMD16_SpillThreshold()) / 100.0f;
    else if (dispatchSize == SIMDMode::SIMD8)
      threshold = float(m_DriverInfo.getSIMD8_SpillThreshold()) / 100.0f;
  }
  return threshold;
}

unsigned OpenCLProgramContext::GetSlmSizePerSubslice() { return platform.getSlmSizePerSsOrDss(); }

uint64_t OpenCLProgramContext::getMinimumValidAddress() const { return m_InternalOptions.MinimumValidAddress; }

COpenCLKernel::COpenCLKernel(OpenCLProgramContext *ctx, Function *pFunc, CShaderProgram *pProgram)
    : m_State(*pFunc, *pProgram->GetContext()), CComputeShaderBase(pFunc, pProgram, m_State) {
  m_HasTID = false;
  m_HasGlobalSize = false;
  m_disableMidThreadPreemption = false;
  m_perWIStatelessPrivateMemSize = 0;
  m_Context = ctx;
  m_localOffsetsMap.clear();
  m_pBtiLayout = &(ctx->btiLayout);
  m_Platform = &(ctx->platform);
  m_DriverInfo = &(ctx->m_DriverInfo);

  m_regularGRFRequested = false;
  m_largeGRFRequested = false;
  m_annotatedNumThreads = -1;
  if (m_Platform->supportsStaticRegSharing()) {
    // Obtain number of threads from user annotations if it is set
    auto &FuncInfo = m_Context->getModuleMetaData()->FuncMD[pFunc];
    int numThreads = extractAnnotatedNumThreads(FuncInfo);
    if (numThreads >= 0 && m_Platform->isValidNumThreads(numThreads)) {
      m_annotatedNumThreads = numThreads;
    }

    // check if option is set to use certain GRF size
    auto FuncName = pFunc->getName().str();
    for (const auto &SubNameR : ctx->m_Options.RegularGRFKernels) {
      if (FuncName.find(SubNameR) != std::string::npos) {
        m_regularGRFRequested = true;
        break;
      }
    }
    for (const auto &SubNameL : ctx->m_Options.LargeGRFKernels) {
      if (FuncName.find(SubNameL) != std::string::npos) {
        m_largeGRFRequested = true;
        break;
      }
    }
  }
}

COpenCLKernel::~COpenCLKernel() { m_simdProgram.Destroy(); }

void COpenCLKernel::PreCompile() {
  CreateImplicitArgs();
  // We explicitly want this to be GRF-sized, without relation to simd width

  RecomputeBTLayout();

  // need to clear m_walkOrderStruct for each shader compile
  m_Context->m_walkOrderStruct = {};

  ModuleMetaData *modMD = m_Context->getModuleMetaData();
  auto funcIter = modMD->FuncMD.find(entry);

  // Initialize the table of offsets for GlobalVariables representing locals
  if (funcIter != modMD->FuncMD.end()) {
    auto loIter = funcIter->second.localOffsets.begin();
    auto loEnd = funcIter->second.localOffsets.end();
    for (; loIter != loEnd; ++loIter) {
      LocalOffsetMD loHandle = *loIter;
      m_localOffsetsMap[loHandle.m_Var] = loHandle.m_Offset;
    }
  }
  if (m_Platform->supportHWGenerateTID() && m_DriverInfo->SupportHWGenerateTID())
    tryHWGenerateLocalIDs();
}
void COpenCLKernel::tryHWGenerateLocalIDs() {
  auto Dims = IGCMetaDataHelper::getThreadGroupDims(*m_pMdUtils, entry);

  if (!Dims)
    return;

  auto WO = getWorkGroupWalkOrder();
  bool ForcedWalkOrder = false;
  if (WO.dim0 != 0 || WO.dim1 != 0 || WO.dim2 != 0) {
    if (auto Order = checkLegalWalkOrder(*Dims, WO)) {
      ForcedWalkOrder = true;
      // Don't do TileY if forced in this way.
      m_Context->m_walkOrderStruct.m_threadIDLayout = ThreadIDLayout::X;
      m_Context->m_walkOrderStruct.m_walkOrder = *Order;
    } else {
      auto WalkOrder = getWalkOrderInPass(WO.dim0, WO.dim1);
      if (WalkOrder != CS_WALK_ORDER::WO_XYZ) {
        IGC_ASSERT_MESSAGE(0, "unhandled walk order!");
      }
      return;
    }
  }

  // OpenCL currently emits all local IDs even if only one dimension
  // is requested. Let's mirror that for now.
  ImplicitArgs implicitArgs(*entry, m_pMdUtils);
  if (implicitArgs.isImplicitArgExist(ImplicitArg::LOCAL_ID_X) ||
      implicitArgs.isImplicitArgExist(ImplicitArg::LOCAL_ID_Y) ||
      implicitArgs.isImplicitArgExist(ImplicitArg::LOCAL_ID_Z)) {
    if (ForcedWalkOrder)
      m_Context->m_walkOrderStruct.m_enableHWGenerateLID = true;
    setEmitLocalMaskInPass(THREAD_ID_IN_GROUP_Z, m_Context->m_walkOrderStruct.m_emitMask);
  }

  if (!ForcedWalkOrder) {
    selectWalkOrderInPass(false, 0, 0, 0, /* dummy 1D accesses */
                          0,              /* dummy 2D accesses */
                          0,              /* dummy SLM accessed */
                          (*Dims)[0], (*Dims)[1], (*Dims)[2], m_Context, m_Context->m_walkOrderStruct);
  }
  encoder.GetVISABuilder()->SetOption(vISA_autoLoadLocalID, m_Context->m_walkOrderStruct.m_enableHWGenerateLID);
}

WorkGroupWalkOrderMD COpenCLKernel::getWorkGroupWalkOrder() {
  const CodeGenContext *pCtx = GetContext();
  const ModuleMetaData *MMD = pCtx->getModuleMetaData();
  if (auto I = MMD->FuncMD.find(entry); I != MMD->FuncMD.end()) {
    auto &FMD = I->second;
    auto &Order = FMD.workGroupWalkOrder;
    return Order;
  }

  return {};
}

SOpenCLKernelInfo::SResourceInfo COpenCLKernel::getResourceInfo(int argNo) {
  CodeGenContext *pCtx = GetContext();
  ModuleMetaData *modMD = pCtx->getModuleMetaData();
  FunctionMetaData *funcMD = &modMD->FuncMD[entry];
  ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
  IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMD List Out of Bounds");
  ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[argNo];

  SOpenCLKernelInfo::SResourceInfo resInfo;
  ResourceTypeEnum type = (ResourceTypeEnum)argAlloc->type;

  if (type == ResourceTypeEnum::UAVResourceType || type == ResourceTypeEnum::BindlessUAVResourceType) {
    resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_UAV;
  } else if (type == ResourceTypeEnum::SRVResourceType) {
    resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_SRV;
  } else {
    resInfo.Type = SOpenCLKernelInfo::SResourceInfo::RES_OTHER;
  }
  resInfo.Index = argAlloc->indexType;
  return resInfo;
}

ResourceExtensionTypeEnum COpenCLKernel::getExtensionInfo(int argNo) {
  CodeGenContext *pCtx = GetContext();
  ModuleMetaData *modMD = pCtx->getModuleMetaData();
  FunctionMetaData *funcMD = &modMD->FuncMD[entry];
  ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
  IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMD List Out of Bounds");
  ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[argNo];
  return (ResourceExtensionTypeEnum)argAlloc->extensionType;
}

void COpenCLKernel::CreateZEInlineSamplerAnnotations() {
  auto getZESamplerAddrMode = [](int addrMode) {
    switch (addrMode) {
    case LEGACY_CLK_ADDRESS_NONE:
      return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::none;
    case LEGACY_CLK_ADDRESS_CLAMP:
      return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::clamp_border;
    case LEGACY_CLK_ADDRESS_CLAMP_TO_EDGE:
      return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::clamp_edge;
    case LEGACY_CLK_ADDRESS_REPEAT:
      return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::repeat;
    case LEGACY_CLK_ADDRESS_MIRRORED_REPEAT:
      return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::mirror;
    default:
      IGC_ASSERT_MESSAGE(false, "Unsupported sampler addressing mode");
      return zebin::PreDefinedAttrGetter::ArgSamplerAddrMode::none;
    }
  };

  auto getZESamplerFilterMode = [](int filterMode) {
    switch (filterMode) {
    case iOpenCL::SAMPLER_MAPFILTER_POINT:
      return zebin::PreDefinedAttrGetter::ArgSamplerFilterMode::nearest;
    case iOpenCL::SAMPLER_MAPFILTER_LINEAR:
      return zebin::PreDefinedAttrGetter::ArgSamplerFilterMode::linear;
    default:
      IGC_ASSERT_MESSAGE(false, "Unsupported sampler filter mode");
      return zebin::PreDefinedAttrGetter::ArgSamplerFilterMode::nearest;
    }
  };

  auto funcMDIter = m_Context->getModuleMetaData()->FuncMD.find(entry);
  if (funcMDIter != m_Context->getModuleMetaData()->FuncMD.end()) {
    const ResourceAllocMD &resAllocMD = funcMDIter->second.resAllocMD;

    for (const auto &inlineSamplerMD : resAllocMD.inlineSamplersMD) {
      auto addrMode = getZESamplerAddrMode(inlineSamplerMD.addressMode);
      auto filterMode = getZESamplerFilterMode(inlineSamplerMD.MagFilterType);
      bool normalized = inlineSamplerMD.NormalizedCoords != 0 ? true : false;
      zebin::ZEInfoBuilder::addInlineSampler(m_kernelInfo.m_zeInlineSamplers, inlineSamplerMD.index, addrMode,
                                             filterMode, normalized);
    }
  }
}

std::string COpenCLKernel::getKernelArgTypeName(const FunctionMetaData &funcMD, uint argIndex) const {
  // The type name is expected to also have the type size, appended after a ";"
  std::string result = funcMD.m_OpenCLArgTypes[argIndex] + ";";

  // Unfortunately, unlike SPIR, legacy OCL uses an ABI that has byval pointers.
  // So, if the parameter is a byval pointer, look at the contained type
  Function::arg_iterator argumentIter = entry->arg_begin();
  std::advance(argumentIter, argIndex);

  Type *argType = entry->getFunctionType()->getParamType(argIndex);
  if (argumentIter->hasByValAttr()) {
    argType = argumentIter->getParamByValType();
  }

  result += utostr(m_DL->getTypeAllocSize(argType));
  return result;
}

std::string COpenCLKernel::getKernelArgTypeQualifier(const FunctionMetaData &funcMD, uint argIndex) const {
  // If there are no type qualifiers, "NONE" is expected
  std::string result = funcMD.m_OpenCLArgTypeQualifiers[argIndex];
  if (result.empty()) {
    result = "NONE";
  }
  return result;
}

std::string COpenCLKernel::getKernelArgAddressQualifier(const FunctionMetaData &funcMD, uint argIndex) const {
  // The address space is expected to have a __ prefix
  switch (funcMD.m_OpenCLArgAddressSpaces[argIndex]) {
  case ADDRESS_SPACE_CONSTANT:
    return "__constant";
  case ADDRESS_SPACE_GLOBAL:
    return "__global";
  case ADDRESS_SPACE_LOCAL:
    return "__local";
  case ADDRESS_SPACE_PRIVATE:
    return "__private";
  default:
    m_Context->EmitError("Generic pointers are not allowed as kernel argument storage class!", nullptr);
    IGC_ASSERT_MESSAGE(0, "Unexpected address space");
    break;
  }
  return "";
}

std::string COpenCLKernel::getKernelArgAccessQualifier(const FunctionMetaData &funcMD, uint argIndex) const {
  // The access qualifier is expected to have a "__" prefix, or an upper-case "NONE" if there is no qualifier
  std::string result = funcMD.m_OpenCLArgAccessQualifiers[argIndex];
  if (result == "none" || result == "") {
    result = "NONE";
  } else if (result[0] != '_') {
    result = "__" + result;
  }
  return result;
}

uint32_t COpenCLKernel::getReqdSubGroupSize(llvm::Function &F, MetaDataUtils *MDUtils) const {
  FunctionInfoMetaDataHandle funcInfoMD = MDUtils->getFunctionsInfoItem(&F);
  int simd_size = funcInfoMD->getSubGroupSize()->getSIMDSize();

  // Finds the kernel and get the group simd size from the kernel
  if (m_FGA) {
    llvm::Function *Kernel = &F;
    auto FG = m_FGA->getGroup(&F);
    Kernel = FG->getHead();
    funcInfoMD = MDUtils->getFunctionsInfoItem(Kernel);
    simd_size = funcInfoMD->getSubGroupSize()->getSIMDSize();
  }
  return simd_size;
}

uint32_t COpenCLKernel::getMaxPressure(llvm::Function &F, MetaDataUtils *MDUtils) const {
  FunctionInfoMetaDataHandle funcInfoMD = MDUtils->getFunctionsInfoItem(&F);
  unsigned int maxPressure = funcInfoMD->getMaxRegPressure()->getMaxPressure();

  if (m_FGA) {
    llvm::Function *Kernel = &F;
    auto FG = m_FGA->getGroup(&F);
    Kernel = FG->getHead();
    funcInfoMD = MDUtils->getFunctionsInfoItem(Kernel);
    maxPressure = funcInfoMD->getMaxRegPressure()->getMaxPressure();
  }
  return maxPressure;
}

std::string COpenCLKernel::getVecTypeHintTypeString(const VectorTypeHintMetaDataHandle &vecTypeHintInfo) const {
  std::string vecTypeString;

  // Get the information about the type
  Type *baseType = vecTypeHintInfo->getVecType()->getType();
  unsigned int numElements = 1;
  if (baseType->isVectorTy()) {
    numElements = (unsigned)cast<IGCLLVM::FixedVectorType>(baseType)->getNumElements();
    baseType = cast<VectorType>(baseType)->getElementType();
  }

  // ExecutionModel doesn't differentiate base type in term of signed/unsigned.
  if (baseType->isIntegerTy()) {
    vecTypeString += "u";
  }

  switch (baseType->getTypeID()) {
  case Type::IntegerTyID:
    switch (baseType->getIntegerBitWidth()) {
    case 8:
      vecTypeString += "char";
      break;
    case 16:
      vecTypeString += "short";
      break;
    case 32:
      vecTypeString += "int";
      break;
    case 64:
      vecTypeString += "long";
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Unexpected data type in vec_type_hint");
      break;
    }
    break;
  case Type::DoubleTyID:
    vecTypeString += "double";
    break;
  case Type::FloatTyID:
    vecTypeString += "float";
    break;
  case Type::HalfTyID:
    vecTypeString += "half";
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "Unexpected data type in vec_type_hint");
    break;
  }

  if (numElements != 1) {
    vecTypeString += utostr(numElements);
  }

  return vecTypeString;
}

bool COpenCLKernel::CreateZEPayloadArguments(IGC::KernelArg *kernelArg, uint payloadPosition,
                                             PtrArgsAttrMapType &ptrArgsAttrMap) {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
  switch (kernelArg->getArgType()) {

  case KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER: {
    // PayloadHeader contains global work offset x,y,z and local size x,y,z
    // global work offset, size is int32x3
    uint cur_pos = payloadPosition;
    uint32_t size = iOpenCL::DATA_PARAMETER_DATA_SIZE * 3;
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::global_id_offset, cur_pos, size);
    cur_pos += size;
    // local size, size is int32x3, the same as above
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::local_size, cur_pos, size);
    break;
  }
  case KernelArg::ArgType::IMPLICIT_GLOBAL_OFFSET: {
    uint32_t size = iOpenCL::DATA_PARAMETER_DATA_SIZE * 3;
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::global_id_offset, payloadPosition, size);
    break;
  }
  case KernelArg::ArgType::IMPLICIT_PRIVATE_BASE:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::private_base_stateless,
                                                     payloadPosition, kernelArg->getSize());
    break;
  case KernelArg::ArgType::IMPLICIT_INDIRECT_DATA_POINTER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::indirect_data_pointer,
                                                     payloadPosition, kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::scratch_pointer,
                                                     payloadPosition, kernelArg->getSize());
    break;
  case KernelArg::ArgType::IMPLICIT_REGION_GROUP_SIZE:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::region_group_size,
                                                     payloadPosition, kernelArg->getSize());
    break;
  case KernelArg::ArgType::IMPLICIT_REGION_GROUP_WG_COUNT:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::region_group_wg_count,
                                                     payloadPosition, kernelArg->getSize());
    break;
  case KernelArg::ArgType::IMPLICIT_REGION_GROUP_BARRIER_BUFFER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::region_group_barrier_buffer,
                                                     payloadPosition, kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_NUM_GROUPS:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::group_count, payloadPosition,
                                                     iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
    break;

  case KernelArg::ArgType::IMPLICIT_LOCAL_SIZE:
    // FIXME: duplicated information as KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER?
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::local_size, payloadPosition,
                                                     iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
    break;

  case KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::enqueued_local_size,
                                                     payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
    break;

  case KernelArg::ArgType::IMPLICIT_GLOBAL_SIZE:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::global_size, payloadPosition,
                                                     iOpenCL::DATA_PARAMETER_DATA_SIZE * 3);
    break;

  case KernelArg::ArgType::IMPLICIT_WORK_DIM:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::work_dimensions,
                                                     payloadPosition, iOpenCL::DATA_PARAMETER_DATA_SIZE);
    break;

  // pointer args
  case KernelArg::ArgType::PTR_GLOBAL:
  case KernelArg::ArgType::PTR_CONSTANT: {
    uint32_t arg_idx = kernelArg->getAssociatedArgNo();

    FunctionMetaData &funcMD = GetContext()->getModuleMetaData()->FuncMD[entry];
    auto addr_space = kernelArg->getArgType() == KernelArg::ArgType::PTR_GLOBAL
                          ? zebin::PreDefinedAttrGetter::ArgAddrSpace::global
                          : zebin::PreDefinedAttrGetter::ArgAddrSpace::constant;
    auto access_type = zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    if (kernelArg->getArgType() == KernelArg::ArgType::PTR_CONSTANT ||
        arg_idx < funcMD.m_OpenCLArgTypeQualifiers.size() && funcMD.m_OpenCLArgTypeQualifiers[arg_idx] == "const")
      access_type = zebin::PreDefinedAttrGetter::ArgAccessType::readonly;

    // FIXME: do not set bti if the number is 0xffffffff (?)
    SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(arg_idx);
    uint32_t bti_idx = getBTI(resInfo);
    // There are 3 stateful modes, we need to create different payload_arguments for
    // each mode: BTI mode, Bindless legacy mode and Bindless advance
    bool is_stateful_mode = bti_idx != 0xffffffff;

    const bool use_bindless_mode = GetContext()->getModuleMetaData()->compOpt.UseBindlessMode;
    const bool use_bindless_legacy_mode = GetContext()->getModuleMetaData()->compOpt.UseLegacyBindlessMode;

    if (is_stateful_mode && (!use_bindless_mode)) {
      // Add BTI argument if being promoted to stateful BTI mode
      // promoted arg has 0 offset and 0 size
      zebin::ZEInfoBuilder::addPayloadArgumentByPointer(m_kernelInfo.m_zePayloadArgs, 0, 0, arg_idx,
                                                        zebin::PreDefinedAttrGetter::ArgAddrMode::stateful, addr_space,
                                                        access_type);
      // add the corresponding BTI table index
      zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs, bti_idx, arg_idx);
    }

    // check if all reference are promoted, if it is, we can skip creating stateless payload arg
    bool is_stateful_only = is_stateful_mode && IGC_IS_FLAG_ENABLED(EnableStatelessToStateful) &&
                            !m_Context->platform.hasEfficient64bEnabled() && IGC_IS_FLAG_ENABLED(EnableStatefulToken) &&
                            m_DriverInfo->SupportStatefulToken() &&
                            !m_Context->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired &&
                            kernelArg->getArg() &&
                            ((kernelArg->getArgType() == KernelArg::ArgType::PTR_GLOBAL &&
                              (kernelArg->getArg()->use_empty() || !GetHasGlobalStatelessAccess())) ||
                             (kernelArg->getArgType() == KernelArg::ArgType::PTR_CONSTANT &&
                              (kernelArg->getArg()->use_empty() || !GetHasConstantStatelessAccess())));

    bool is_bindless_legacy_mode = use_bindless_mode && use_bindless_legacy_mode;
    bool is_bindless_advance_mode = use_bindless_mode && !use_bindless_legacy_mode;

    // When on bindless advance mode, there's an IMPLICIT_BINDLESS_OFFSET argument generated
    // associated to this argument. Keep track of addrspace and access_type for setting the
    // IMPLICIT_BINDLESS_OFFSET's attributes
    if (is_bindless_advance_mode) {
      auto zeArgTy = zebin::PreDefinedAttrGetter::ArgType::arg_bypointer;
      ptrArgsAttrMap[arg_idx] = std::make_tuple(addr_space, access_type, zeArgTy);
    }

    // For BindlessLegacyMode, this argument represents the bindless offset of the argument. Skip buffer_address
    // creation and fall through to below bindless payload_argument creation
    // For BindlessAdvanceMode and BTI modes, this argument represents either the original stateless address
    // or the "buffer_address" when all accesses are promoted to stateful
    if (is_stateful_only && !is_bindless_legacy_mode) {
      // create buffer_address for statefull only arg in case of address check is needed
      // for example: something like "if(buffer != nullptr)" in the kernel.
      // this address will be accessed as a value and cannot be de-referenced
      zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
          m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::buffer_address, payloadPosition,
          kernelArg->getSize());
      arg.arg_index = arg_idx;
      break;
    }

    ResourceAllocMD &resAllocMD = GetContext()->getModuleMetaData()->FuncMD[entry].resAllocMD;
    IGC_ASSERT_MESSAGE(resAllocMD.argAllocMDList.size() > 0, "ArgAllocMDList is empty.");

    ArgAllocMD &argAlloc = resAllocMD.argAllocMDList[arg_idx];

    zebin::PreDefinedAttrGetter::ArgAddrMode addr_mode = zebin::PreDefinedAttrGetter::ArgAddrMode::stateless;
    if (argAlloc.type == ResourceTypeEnum::BindlessUAVResourceType)
      addr_mode = zebin::PreDefinedAttrGetter::ArgAddrMode::bindless;

    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
        m_kernelInfo.m_zePayloadArgs, payloadPosition, kernelArg->getSize(), arg_idx, addr_mode, addr_space,
        access_type);
    arg.is_pipe =
        arg_idx < funcMD.m_OpenCLArgTypeQualifiers.size() && funcMD.m_OpenCLArgTypeQualifiers[arg_idx] == "pipe";
    break;
  }
  case KernelArg::ArgType::PTR_LOCAL:
    zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
        m_kernelInfo.m_zePayloadArgs, payloadPosition, kernelArg->getSize(), kernelArg->getAssociatedArgNo(),
        zebin::PreDefinedAttrGetter::ArgAddrMode::slm, zebin::PreDefinedAttrGetter::ArgAddrSpace::local,
        zebin::PreDefinedAttrGetter::ArgAccessType::readwrite, kernelArg->getAlignment());
    break;
  // by value arguments
  case KernelArg::ArgType::CONSTANT_REG:
    zebin::ZEInfoBuilder::addPayloadArgumentByValue(m_kernelInfo.m_zePayloadArgs, payloadPosition, kernelArg->getSize(),
                                                    kernelArg->getAssociatedArgNo(), kernelArg->getStructArgOffset(),
                                                    kernelArg->isScalarAsPointer());
    break;

  // Local ids are supported in per-thread payload arguments
  case KernelArg::ArgType::IMPLICIT_LOCAL_ID_X:
  case KernelArg::ArgType::IMPLICIT_LOCAL_ID_Y:
  case KernelArg::ArgType::IMPLICIT_LOCAL_ID_Z:
    break;

  // Bindless offset for pointer argument. This ArgType presents when bindless-advanced-mode
  // is enabled
  case KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET: {
    auto argidx = kernelArg->getAssociatedArgNo();
    IGC_ASSERT_MESSAGE(ptrArgsAttrMap.find(argidx) != ptrArgsAttrMap.end(),
                       "Cannot find ptrArgsAttr for IMPLICIT_BINDLESS_OFFSET");
    PtrArgAttrType &attrs = ptrArgsAttrMap[argidx];

    auto argTy = std::get<2>(attrs);
    if (argTy == zebin::PreDefinedAttrGetter::ArgType::const_base ||
        argTy == zebin::PreDefinedAttrGetter::ArgType::global_base) {
      // If the associated arg for bindless_offset refers to another implicit argument,
      // instead of mapping it via arg_index (since "arg_index" in payload_arguments
      // refers to the user (explicit) arguments' index in the original kernel definition),
      // create another version of the implicit arg with the bindless addrmode set.
      //
      // Example:
      // - arg_type : const_base
      //    offset : 40
      //    size : 8
      // - arg_type : const_base
      //    offset : 80
      //    size : 4
      //    addrmode : bindless
      //
      // Ultimately in zeinfo we end up with two definitions of the implicit arg, where
      // one of them is the stateless pointer, and the other is the bindless offset.
      zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
          m_kernelInfo.m_zePayloadArgs, argTy, payloadPosition, kernelArg->getSize());
      arg.addrmode = zebin::PreDefinedAttrGetter::get(zebin::PreDefinedAttrGetter::ArgAddrMode::bindless);
    } else {
      zebin::ZEInfoBuilder::addPayloadArgumentByPointer(
          m_kernelInfo.m_zePayloadArgs, payloadPosition, kernelArg->getSize(), argidx,
          zebin::PreDefinedAttrGetter::ArgAddrMode::bindless, std::get<0>(attrs), std::get<1>(attrs));
    }
    break;
  }
  // Images
  case KernelArg::ArgType::IMAGE_1D:
  case KernelArg::ArgType::BINDLESS_IMAGE_1D:
  case KernelArg::ArgType::IMAGE_1D_BUFFER:
  case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
  case KernelArg::ArgType::IMAGE_2D:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D:
  case KernelArg::ArgType::IMAGE_3D:
  case KernelArg::ArgType::BINDLESS_IMAGE_3D:
  case KernelArg::ArgType::IMAGE_CUBE:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
  case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
  case KernelArg::ArgType::IMAGE_1D_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
  case KernelArg::ArgType::IMAGE_2D_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
  case KernelArg::ArgType::IMAGE_2D_DEPTH:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
  case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
  case KernelArg::ArgType::IMAGE_2D_MSAA:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
  case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
  case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
  case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
  case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
  case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY: {
    // the image arg is either bindless or stateful.
    // For stateful image argument, the arg has 0 offset and 0 size
    zebin::PreDefinedAttrGetter::ArgAddrMode arg_addrmode = m_ModuleMetadata->UseBindlessImage
                                                                ? zebin::PreDefinedAttrGetter::ArgAddrMode::bindless
                                                                : zebin::PreDefinedAttrGetter::ArgAddrMode::stateful;
    uint arg_off = 0;
    uint arg_size = 0;

    int arg_idx = kernelArg->getAssociatedArgNo();
    if (kernelArg->needsAllocation()) {
      // bindless
      arg_off = payloadPosition;
      arg_size = kernelArg->getSize();
    } else if (arg_addrmode == zebin::PreDefinedAttrGetter::ArgAddrMode::stateful) {
      // add bti index for this arg if it's stateful
      SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(arg_idx);
      zebin::ZEInfoBuilder::addBindingTableIndex(m_kernelInfo.m_zeBTIArgs, getBTI(resInfo), arg_idx);
    }

    auto access_type = [](KernelArg::AccessQual qual) {
      if (qual == KernelArg::AccessQual::READ_ONLY)
        return zebin::PreDefinedAttrGetter::ArgAccessType::readonly;
      if (qual == KernelArg::AccessQual::WRITE_ONLY)
        return zebin::PreDefinedAttrGetter::ArgAccessType::writeonly;
      return zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    }(kernelArg->getAccessQual());

    auto image_type = getZEImageType(getImageTypeFromKernelArg(*kernelArg));

    // add the payload argument
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImage(
        m_kernelInfo.m_zePayloadArgs, arg_off, arg_size, arg_idx, arg_addrmode, access_type, image_type);
    // TODO: When ZEBIN path supports inline sampler, follow Patch Token
    // path to check if the samplers allow 3D images to be represented
    // as arrays of 2D images.
    // (deprecated InlineSamplersAllow3DImageTransformation())
    arg.image_transformable = kernelArg->getArgType() == KernelArg::ArgType::IMAGE_3D &&
                              kernelArg->getImgAccessedIntCoords() && !kernelArg->getImgAccessedFloatCoords();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_HEIGHT: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_height, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_WIDTH: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_width, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_DEPTH: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_depth, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_MIP_LEVELS: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_num_mip_levels, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_DATA_TYPE: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_channel_data_type, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_CHANNEL_ORDER: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_channel_order, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_SRGB_CHANNEL_ORDER: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_srgb_channel_order, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_ARRAY_SIZE: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_array_size, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_IMAGE_NUM_SAMPLES: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::image_num_samples, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_BASEOFFSET: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::flat_image_baseoffset, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_HEIGHT: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::flat_image_height, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_WIDTH: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::flat_image_width, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_FLAT_IMAGE_PITCH: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::flat_image_pitch, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  // sampler
  case KernelArg::ArgType::SAMPLER:
  case KernelArg::ArgType::BINDLESS_SAMPLER: {
    // the sampler arg is either bindless or stateful.
    // For stateful image argument, the arg has 0 offset and 0 size
    // NOTE: we only have stateful sampler now
    zebin::PreDefinedAttrGetter::ArgAddrMode arg_addrmode = m_ModuleMetadata->UseBindlessImage
                                                                ? zebin::PreDefinedAttrGetter::ArgAddrMode::bindless
                                                                : zebin::PreDefinedAttrGetter::ArgAddrMode::stateful;
    uint arg_off = 0;
    uint arg_size = 0;
    if (kernelArg->needsAllocation()) {
      // bindless
      arg_off = payloadPosition;
      arg_size = kernelArg->getSize();
    }

    int arg_idx = kernelArg->getAssociatedArgNo();
    SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(arg_idx);
    auto sampler_type = getZESamplerType(getSamplerTypeFromKernelArg(*kernelArg));
    // add the payload argument
    zebin::ZEInfoBuilder::addPayloadArgumentSampler(
        m_kernelInfo.m_zePayloadArgs, arg_off, arg_size, arg_idx, resInfo.Index, arg_addrmode,
        zebin::PreDefinedAttrGetter::ArgAccessType::readwrite, sampler_type);
    break;
  }
  case KernelArg::ArgType::IMPLICIT_SAMPLER_ADDRESS: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::sampler_address, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_SAMPLER_NORMALIZED: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::sampler_normalized, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_SAMPLER_SNAP_WA: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::sampler_snap_wa, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }

  case KernelArg::ArgType::IMPLICIT_INLINE_SAMPLER: {
    uint32_t arg_idx = kernelArg->getAssociatedArgNo();
    ResourceAllocMD &resAllocMD = GetContext()->getModuleMetaData()->FuncMD[entry].resAllocMD;
    auto it = llvm::find_if(resAllocMD.inlineSamplersMD,
                            [&](auto &inlineSamplerMD) { return inlineSamplerMD.m_Value == arg_idx; });
    IGC_ASSERT_MESSAGE(it != resAllocMD.inlineSamplersMD.end(), "Inline sampler isn't found in metadata.");
    zebin::ZEInfoBuilder::addPayloadArgumentImplicitInlineSampler(m_kernelInfo.m_zePayloadArgs,
                                                                  zebin::PreDefinedAttrGetter::ArgType::inline_sampler,
                                                                  payloadPosition, kernelArg->getSize(), it->index);
    break;
  }

  case KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::buffer_offset, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }
  case KernelArg::ArgType::IMPLICIT_PRINTF_BUFFER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::printf_buffer,
                                                     payloadPosition, kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_ARG_BUFFER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::implicit_arg_buffer,
                                                     payloadPosition, kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_SYNC_BUFFER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::sync_buffer, payloadPosition,
                                                     kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::rt_global_buffer,
                                                     payloadPosition, kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_ASSERT_BUFFER:
    zebin::ZEInfoBuilder::addPayloadArgumentImplicit(m_kernelInfo.m_zePayloadArgs,
                                                     zebin::PreDefinedAttrGetter::ArgType::assert_buffer,
                                                     payloadPosition, kernelArg->getSize());
    break;

  case KernelArg::ArgType::IMPLICIT_CONSTANT_BASE:
  case KernelArg::ArgType::IMPLICIT_GLOBAL_BASE: {
    uint32_t arg_idx = kernelArg->getAssociatedArgNo();
    auto zeArgType = kernelArg->getArgType() == KernelArg::ArgType::IMPLICIT_CONSTANT_BASE
                         ? zebin::PreDefinedAttrGetter::ArgType::const_base
                         : zebin::PreDefinedAttrGetter::ArgType::global_base;
    auto addr_space = kernelArg->getArgType() == KernelArg::ArgType::IMPLICIT_GLOBAL_BASE
                          ? zebin::PreDefinedAttrGetter::ArgAddrSpace::global
                          : zebin::PreDefinedAttrGetter::ArgAddrSpace::constant;
    auto access_type = zebin::PreDefinedAttrGetter::ArgAccessType::readwrite;
    if (kernelArg->getArgType() == KernelArg::ArgType::IMPLICIT_CONSTANT_BASE)
      access_type = zebin::PreDefinedAttrGetter::ArgAccessType::readonly;

    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zeArgType, payloadPosition, kernelArg->getSize(), kernelArg->isScalarAsPointer());
    SOpenCLKernelInfo::SResourceInfo resInfo = getResourceInfo(kernelArg->getAssociatedArgNo());
    unsigned btiValue = getBTI(resInfo);

    if (m_Context->m_InternalOptions.UseBindlessMode) {
      if (m_Context->m_InternalOptions.UseBindlessLegacyMode) {
        zebin::PreDefinedAttrGetter::ArgAddrMode addr_mode = zebin::PreDefinedAttrGetter::ArgAddrMode::bindless;
        arg.addrmode = zebin::PreDefinedAttrGetter::get(addr_mode);
        arg.addrspace = zebin::PreDefinedAttrGetter::get(addr_space);
        arg.access_type = zebin::PreDefinedAttrGetter::get(access_type);
      } else // bindless-advanced-mode
      {
        // For bindless access of const_base and global_base, create an associated arg entry
        // mapping this implicit arg to a bindless_offset arg
        ptrArgsAttrMap[arg_idx] = std::make_tuple(addr_space, access_type, zeArgType);
      }

    } else if (btiValue != 0xffffffff) {
      arg.bti_value = btiValue;
    }
    break;
  }
  case KernelArg::ArgType::IMPLICIT_BUFFER_SIZE: {
    zebin::zeInfoPayloadArgument &arg = zebin::ZEInfoBuilder::addPayloadArgumentImplicit(
        m_kernelInfo.m_zePayloadArgs, zebin::PreDefinedAttrGetter::ArgType::buffer_size, payloadPosition,
        kernelArg->getSize());
    arg.arg_index = kernelArg->getAssociatedArgNo();
    break;
  }

  // We don't need these in ZEBinary, can safely skip them
  case KernelArg::ArgType::IMPLICIT_R0:
  case KernelArg::ArgType::R1:
  case KernelArg::ArgType::STRUCT:
    break;

  case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_ORIGIN:
  case KernelArg::ArgType::IMPLICIT_STAGE_IN_GRID_SIZE:
  default:
    return false;
  } // end switch (kernelArg->getArgType())
#endif // ifndef VK_ONLY_IGC
#endif // ifndef DX_ONLY_IGC
  return true;
}

iOpenCL::IMAGE_MEMORY_OBJECT_TYPE COpenCLKernel::getImageTypeFromKernelArg(const KernelArg &kernelArg) {
  switch (kernelArg.getArgType()) {
  case KernelArg::ArgType::IMAGE_1D:
  case KernelArg::ArgType::BINDLESS_IMAGE_1D:
    return iOpenCL::IMAGE_MEMORY_OBJECT_1D;

  case KernelArg::ArgType::IMAGE_1D_BUFFER:
  case KernelArg::ArgType::BINDLESS_IMAGE_1D_BUFFER:
    return iOpenCL::IMAGE_MEMORY_OBJECT_BUFFER;

  case KernelArg::ArgType::IMAGE_2D:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D:
    if (getExtensionInfo(kernelArg.getAssociatedArgNo()) == ResourceExtensionTypeEnum::MediaResourceType)
      return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA;
    else if (getExtensionInfo(kernelArg.getAssociatedArgNo()) == ResourceExtensionTypeEnum::MediaResourceBlockType)
      return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK;
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D;

  case KernelArg::ArgType::IMAGE_3D:
  case KernelArg::ArgType::BINDLESS_IMAGE_3D:
    return iOpenCL::IMAGE_MEMORY_OBJECT_3D;

  case KernelArg::ArgType::IMAGE_CUBE:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE:
    return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE;

  case KernelArg::ArgType::IMAGE_CUBE_DEPTH:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH:
    // Use regular cube texture for depth:
    return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE;

  case KernelArg::ArgType::IMAGE_1D_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_1D_ARRAY:
    return iOpenCL::IMAGE_MEMORY_OBJECT_1D_ARRAY;

  case KernelArg::ArgType::IMAGE_2D_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_ARRAY:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY;

  case KernelArg::ArgType::IMAGE_2D_DEPTH:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_DEPTH;

  case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_DEPTH_ARRAY:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_DEPTH;

  case KernelArg::ArgType::IMAGE_2D_MSAA:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA;

  case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_ARRAY:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA;

  case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA_DEPTH;

  case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_2D_MSAA_DEPTH_ARRAY:
    return iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA_DEPTH;

  case KernelArg::ArgType::IMAGE_CUBE_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_ARRAY:
    return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY;

  case KernelArg::ArgType::IMAGE_CUBE_DEPTH_ARRAY:
  case KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY:
    // Use regular cube texture array for depth
    return iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY;

  default:
    break;
  }
  return iOpenCL::IMAGE_MEMORY_OBJECT_INVALID;
}

iOpenCL::SAMPLER_OBJECT_TYPE COpenCLKernel::getSamplerTypeFromKernelArg(const KernelArg &kernelArg) {
  IGC_ASSERT(kernelArg.getArgType() == KernelArg::ArgType::SAMPLER ||
             kernelArg.getArgType() == KernelArg::ArgType::BINDLESS_SAMPLER);
  switch (getExtensionInfo(kernelArg.getAssociatedArgNo())) {
  case ResourceExtensionTypeEnum::MediaSamplerType:
    return iOpenCL::SAMPLER_OBJECT_VME;

  case ResourceExtensionTypeEnum::MediaSamplerTypeConvolve:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_2DCONVOLVE;

  case ResourceExtensionTypeEnum::MediaSamplerTypeErode:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_ERODE;

  case ResourceExtensionTypeEnum::MediaSamplerTypeDilate:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_DILATE;

  case ResourceExtensionTypeEnum::MediaSamplerTypeMinMaxFilter:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAXFILTER;

  case ResourceExtensionTypeEnum::MediaSamplerTypeMinMax:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAX;

  case ResourceExtensionTypeEnum::MediaSamplerTypeCentroid:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_CENTROID;

  case ResourceExtensionTypeEnum::MediaSamplerTypeBoolCentroid:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_CENTROID;

  case ResourceExtensionTypeEnum::MediaSamplerTypeBoolSum:
    return iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_SUM;

  default:
    return iOpenCL::SAMPLER_OBJECT_TEXTURE;
  }
}

void COpenCLKernel::ParseShaderSpecificOpcode(llvm::Instruction *inst) {
  auto setStatelessAccess = [&](unsigned AS) {
    if (AS == ADDRESS_SPACE_GLOBAL || AS == ADDRESS_SPACE_GENERIC || AS == ADDRESS_SPACE_GLOBAL_OR_PRIVATE) {
      SetHasGlobalStatelessAccess();
    }

    if (AS == ADDRESS_SPACE_CONSTANT) {
      SetHasConstantStatelessAccess();
    }
  };

  // Currently we see data corruption when we have IEEE macros and midthread preemption enabled.
  // Adding a temporary work around to disable mid thread preemption when we see IEEE Macros.
  switch (inst->getOpcode()) {
  case Instruction::FDiv:
    if (inst->getType()->isDoubleTy()) {
      SetDisableMidthreadPreemption();
    }
    break;
  case Instruction::Call:
    if (inst->getType()->isDoubleTy()) {
      if (GetOpCode(inst) == llvm_sqrt) {
        SetDisableMidthreadPreemption();
      }
    }
    break;
  case Instruction::Load: {
    unsigned AS = cast<LoadInst>(inst)->getPointerAddressSpace();
    setStatelessAccess(AS);
    break;
  }
  case Instruction::Store: {
    unsigned AS = cast<StoreInst>(inst)->getPointerAddressSpace();
    setStatelessAccess(AS);
    break;
  }
  default:
    break;
  }

  if (CallInst *CallI = dyn_cast<CallInst>(inst)) {
    bool mayHasMemoryAccess = true; // for checking stateless access
    if (GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(CallI)) {
      GenISAIntrinsic::ID id = GII->getIntrinsicID();
      switch (id) {
      default:
        break;
      case GenISAIntrinsic::GenISA_dpas:
      case GenISAIntrinsic::GenISA_sub_group_dpas:
      case GenISAIntrinsic::GenISA_sub_group_bdpas:
        m_State.SetHasDPAS();
        break;
      case GenISAIntrinsic::GenISA_ptr_to_pair:
      case GenISAIntrinsic::GenISA_pair_to_ptr:
        mayHasMemoryAccess = false;
        break;
      } // End of switch
    }

    if (InlineAsm *IA = dyn_cast<InlineAsm>(IGCLLVM::getCalledValue(CallI))) {
      if (IA->getAsmString().find("dpas") != std::string::npos) {
        m_State.SetHasDPAS();
      }
    }
    if (mayHasMemoryAccess) {
      // Checking stateless access info
      if (!isa<IntrinsicInst>(CallI) && !isa<GenIntrinsicInst>(CallI)) {
        // function/subroutine call. Give up
        SetHasConstantStatelessAccess();
        SetHasGlobalStatelessAccess();
      } else {
        for (int i = 0, e = (int)IGCLLVM::getNumArgOperands(CallI); i < e; ++i) {
          Value *arg = CallI->getArgOperand(i);
          PointerType *PTy = dyn_cast<PointerType>(arg->getType());
          if (!PTy)
            continue;
          unsigned AS = PTy->getAddressSpace();
          setStatelessAccess(AS);
        }
      }
    }
  }
}

void COpenCLKernel::AllocatePayload() {
  IGC_ASSERT(m_Context);

  bool loadThreadPayload = false;

  loadThreadPayload = m_Platform->supportLoadThreadPayloadForCompute();

  // SKL defaults to indirect thread payload storage.
  // BDW needs CURBE payload. Spec says:
  // "CURBE should be used for the payload when using indirect dispatch rather than indirect payload".
  m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
  if (IGC_IS_FLAG_ENABLED(DisableGPGPUIndirectPayload) ||
      m_Context->platform.getWATable().WaDisableIndirectDataForIndirectDispatch) {
    m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = false;
  }
  if (loadThreadPayload) {
    m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage = true;
  }
  m_kernelInfo.m_threadPayload.HasFlattenedLocalID = false;
  m_kernelInfo.m_threadPayload.HasLocalIDx = false;
  m_kernelInfo.m_threadPayload.HasLocalIDy = false;
  m_kernelInfo.m_threadPayload.HasLocalIDz = false;
  m_kernelInfo.m_threadPayload.HasGlobalIDOffset = false;
  m_kernelInfo.m_threadPayload.HasGroupID = false;
  m_kernelInfo.m_threadPayload.HasLocalID = false;
  m_kernelInfo.m_threadPayload.UnusedPerThreadConstantPresent = false;
  m_kernelInfo.m_threadPayload.HasStageInGridOrigin = false;
  m_kernelInfo.m_threadPayload.HasStageInGridSize = false;
  m_kernelInfo.m_threadPayload.HasRTStackID = false;

  if (m_Context->m_walkOrderStruct.m_enableHWGenerateLID) {
    m_kernelInfo.m_threadPayload.generateLocalID = true;
    m_kernelInfo.m_threadPayload.emitLocalMask = m_Context->m_walkOrderStruct.m_emitMask;
    m_kernelInfo.m_threadPayload.walkOrder = static_cast<unsigned int>(m_Context->m_walkOrderStruct.m_walkOrder);
    m_kernelInfo.m_threadPayload.tileY = (m_Context->m_walkOrderStruct.m_threadIDLayout == ThreadIDLayout::TileY);
  }

  // Set the amount of the private memory used by the kernel
  // Set only if the private memory metadata actually exists and we don't use
  // scratch space for private memory.
  bool noScratchSpacePrivMem = !m_Context->getModuleMetaData()->compOpt.UseScratchSpacePrivateMemory;
  if (noScratchSpacePrivMem) {
    auto StackMemIter = m_Context->getModuleMetaData()->PrivateMemoryPerFG.find(entry);
    if (StackMemIter != m_Context->getModuleMetaData()->PrivateMemoryPerFG.end()) {
      m_perWIStatelessPrivateMemSize = StackMemIter->second;
    }
  }

  m_State.m_ConstantBufferLength = 0;
  m_State.m_NOSBufferSize = 0;

  uint offset = 0;

  uint constantBufferStart = 0;
  bool constantBufferStartSet = false;

  uint prevOffset = 0;
  bool nosBufferAllocated = false;

  KernelArgsOrder::InputType layout = m_kernelInfo.m_threadPayload.CompiledForIndirectPayloadStorage
                                          ? KernelArgsOrder::InputType::INDIRECT
                                          : KernelArgsOrder::InputType::CURBE;

  KernelArgs kernelArgs(*entry, m_DL, m_pMdUtils, m_ModuleMetadata, getGRFSize(), layout);

  // Before main loop, check which local IDs are used.
  setOCLThreadPayloadLocalIDs(kernelArgs);

  if (layout == KernelArgsOrder::InputType::INDIRECT && !loadThreadPayload) {
    kernelArgs.checkForZeroPerThreadData();
  }

  const bool useInlineData = passNOSInlineData();
  const uint inlineDataSize = m_Platform->getInlineDataSize();
  bool inlineDataProcessed = false;
  uint offsetCorrection = 0;

  bool skipIndirectScratchPointer = canSkipScratchPointer(kernelArgs);

  // keep track of the pointer arguments' addrspace and access_type for setting the correct
  // attributes to their corresponding bindless offset arguments
  PtrArgsAttrMapType ptrArgsAttrMap;
  for (KernelArgs::const_iterator i = kernelArgs.begin(), e = kernelArgs.end(); i != e; ++i) {
    KernelArg arg = *i;
    prevOffset = offset;

    if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER && skipIndirectScratchPointer) {
      m_TryNoScratchPointer = true;
      continue;
    }

    // skip unused arguments
    bool IsUnusedArg = isUnusedArg(arg);

    // Runtime Values should not be processed any further. No annotations shall be created for them.
    // Only added to KernelArgs to enforce correct allocation order.
    bool isRuntimeValue = (arg.getArgType() == KernelArg::ArgType::RUNTIME_VALUE);

    if (!constantBufferStartSet && arg.isConstantBuf()) {
      constantBufferStart = offset;
      constantBufferStartSet = true;
    }

    if (!nosBufferAllocated && isRuntimeValue) {
      IGC_ASSERT_MESSAGE(arg.isConstantBuf(), "RuntimeValues must be marked as isConstantBuf");
      AllocateNOSConstants(offset);
      nosBufferAllocated = true;
    }

    // Local IDs are non-uniform and may have two instances in SIMD32 mode
    int numAllocInstances = arg.isImplicitLocalId() ? m_numberInstance : 1;

    if (arg.getArgType() == KernelArg::ArgType::RT_STACK_ID) {
      numAllocInstances = m_numberInstance;
    }

    auto allocSize = arg.getAllocateSize();

    if (!IsUnusedArg && !isRuntimeValue) {
      if (arg.needsAllocation()) {
        // Align on the desired alignment for this argument
        auto alignment = arg.getAlignment();

        if (arg.isArgPtrType())
          alignment = m_Context->getModule()->getDataLayout().getPointerTypeSize(arg.getArg()->getType());

        // FIXME: move alignment checks to implicit arg creation
        if ((arg.isImplicitLocalId() || arg.getArgType() == KernelArg::ArgType::RT_STACK_ID) &&
            m_Platform->getGRFSize() == 64) {
          alignment = 64;
          // generate a single SIMD32 variable in this case
          if (m_State.m_dispatchSize == SIMDMode::SIMD16 && m_Platform->getGRFSize() == 64) {
            allocSize = 64;
          } else {
            allocSize = PVCLSCEnabled() ? 64 : 32;
          }
        }

        offset = iSTD::Align(offset, alignment);

        // Arguments larger than a GRF must be at least GRF-aligned.
        // Arguments smaller than a GRF may not cross GRF boundaries.
        // This means that arguments that cross a GRF boundary
        // must be GRF aligned.
        // Note that this is done AFTER we align on the base alignment,
        // because of edge cases where aligning on the base alignment
        // is what causes the "overflow".
        unsigned int startGRF = offset / getGRFSize();
        unsigned int endGRF = (offset + allocSize - 1) / getGRFSize();
        if (startGRF != endGRF) {
          offset = iSTD::Align(offset, getGRFSize());
        }

        // offsetCorrection should be set only when we are loading payload in kenrel prolog
        if (loadThreadPayload) {
          bool isFirstCrossThreadArgument = constantBufferStartSet && prevOffset == constantBufferStart;
          // if we don't use inline data and first argument does not start in first avaliable register
          // because of its alignment (which can be greater than GRF size), we correct the offset in payload,
          // so that it can be loaded properly in prolog, we want it to be on 0 offset in payload
          //
          // payload_position = offset - constant_buffer_start - correction
          //
          // examples:
          //  alignment   offset   constant_buffer_start  correction  payload_position
          //   128         128      32                     96          0
          //   8           32       32                     0           0
          if (!useInlineData && isFirstCrossThreadArgument) {
            offsetCorrection = offset - constantBufferStart;
          }

          if (useInlineData && !inlineDataProcessed && !arg.isImplicitLocalId() &&
              arg.getArgType() != KernelArg::ArgType::RT_STACK_ID &&
              arg.getArgType() != KernelArg::ArgType::IMPLICIT_R0) {
            // Calc if we can fit this arg in inlinedata:
            // We check if arg exceeds inline data boundaries,
            // if it does, we align it to next GRF.
            if (offset + allocSize - constantBufferStart > inlineDataSize) {
              inlineDataProcessed = true;
              if (getGRFSize() > inlineDataSize) {
                // If inline data is used and a plaftorm has 64B GRFs,
                // we must correct the offset of cross-thread arguments
                // which are not loaded in inline data
                // the reason behind this is that inline data has only 32B,
                // so the position of next arg needs to be aligned to next GRF,
                // because the input arguments are loaded with alignment of GRF
                offset = iSTD::Align(offset, getGRFSize());
              }

              // numAllocInstances can be greater than 1, only when:
              // artype == IMPLICIT_LOCAL_IDS
              // or argtype == RT_STACK_ID,
              // so there is no need to handle it here

              // current arg is first to be loaded (it does not come in inlinedata)
              // so we want it to be at 32B offset in payload annotations
              // (first 32B are for inline data)
              offsetCorrection = offset - inlineDataSize - constantBufferStart;
            }
          }
        }

        // And now actually tell vISA we need this space.
        // (Except for r0, which is a predefined variable, and should never be allocated as input!)
        const llvm::Argument *A = arg.getArg();
        if (A != nullptr && arg.getArgType() != KernelArg::ArgType::IMPLICIT_R0) {
          CVariable *var = GetSymbol(const_cast<Argument *>(A));
          for (int i = 0; i < numAllocInstances; ++i) {
            uint totalOffset = offset + (allocSize * i);
            if ((totalOffset / getGRFSize()) >= m_Context->getNumGRFPerThread()) {
              m_Context->EmitError("Kernel inputs exceed total register size!", A);
              return;
            }
            AllocateInput(var, totalOffset, i);
          }
        }
        // or else we would just need to increase an offset
      }

      const uint offsetInPayload = offset - constantBufferStart - offsetCorrection;

      bool Res = CreateZEPayloadArguments(&arg, offsetInPayload, ptrArgsAttrMap);
      IGC_ASSERT_MESSAGE(Res, "ZEBin: unsupported KernelArg Type");
      (void)Res;

      if (arg.needsAllocation()) {
        for (int i = 0; i < numAllocInstances; ++i) {
          offset += allocSize;
        }
        // FIXME: Should we allocate R0 to be 64 byte for PVC?
        if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_R0 && m_Platform->getGRFSize() == 64) {
          offset += 32;
        }
      }
    }

    if (arg.isConstantBuf()) {
      m_State.m_ConstantBufferLength += offset - prevOffset;
    }
  }

  // Disable EU Fusion.
  if (IGC_IS_FLAG_ENABLED(DisableEuFusion) || m_Context->m_InternalOptions.DisableEUFusion ||
      m_Context->getModuleMetaData()->compOpt.DisableEUFusion) {
    m_kernelInfo.m_executionEnvironment.RequireDisableEUFusion = true;
  }

  // ToDo: we should avoid passing all three dimensions of local id
  if (m_kernelInfo.m_threadPayload.HasLocalIDx || m_kernelInfo.m_threadPayload.HasLocalIDy ||
      m_kernelInfo.m_threadPayload.HasLocalIDz) {
    if (loadThreadPayload) {
      uint dimensions =
          m_kernelInfo.m_threadPayload.HasLocalIDz ? 3 : (m_kernelInfo.m_threadPayload.HasLocalIDy ? 2 : 1);
      uint perThreadInputSize = SIZE_WORD * dimensions * (m_State.m_dispatchSize == SIMDMode::SIMD32 ? 32 : 16);
      if (m_State.m_dispatchSize == SIMDMode::SIMD16 && getGRFSize() == 64) {
        perThreadInputSize *= 2;
      }
      encoder.GetVISAKernel()->AddKernelAttribute("PerThreadInputSize", sizeof(perThreadInputSize),
                                                  &perThreadInputSize);
    }
  }

  m_kernelInfo.m_threadPayload.OffsetToSkipPerThreadDataLoad = 0;
  m_kernelInfo.m_threadPayload.OffsetToSkipSetFFIDGP = 0;

  m_State.m_ConstantBufferLength = iSTD::Align(m_State.m_ConstantBufferLength, getGRFSize());

  CreateZEInlineSamplerAnnotations();
}

bool COpenCLKernel::isUnusedArg(KernelArg &arg) const {

  // For local ID, read from payload.
  if (arg.isImplicitLocalId()) {
    switch (arg.getArgType()) {
    case KernelArg::ArgType::IMPLICIT_LOCAL_ID_X:
      return !m_kernelInfo.m_threadPayload.HasLocalIDx;
    case KernelArg::ArgType::IMPLICIT_LOCAL_ID_Y:
      return !m_kernelInfo.m_threadPayload.HasLocalIDy;
    case KernelArg::ArgType::IMPLICIT_LOCAL_ID_Z:
      return !m_kernelInfo.m_threadPayload.HasLocalIDz;
    default:
      return false;
    }
  }

  if (!arg.getArg() || !arg.getArg()->use_empty())
    return false;

  // Implicit arguments related to buffers can be always removed if unused.
  if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET ||
      arg.getArgType() == KernelArg::ArgType::IMPLICIT_BINDLESS_OFFSET ||
      arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_SIZE)
    return true;

  // When removing unused implicit arguments, assume subroutine calls use implicit arguments.
  if (!AllowRemovingUnusedImplicitArguments(m_Context) || HasSubroutines())
    return false;

  return arg.getArgType() == KernelArg::ArgType::IMPLICIT_PAYLOAD_HEADER || // contains global_id_offset
         arg.getArgType() == KernelArg::ArgType::IMPLICIT_GLOBAL_OFFSET ||
         arg.getArgType() == KernelArg::ArgType::IMPLICIT_ENQUEUED_LOCAL_WORK_SIZE;
}

// On Xe3P, scratch pointer is provided to kernel as implicit argument.
// This impacts how much kernel will spend time in cross thread prolog.
//
// Scratch can be used by both IGC and vISA; IGC doesn't know if scratch
// is used until vISA is done and IGC can check for spills.
//
// This method tries to guess if scratch pointer can be removed from kernel
// arguments. If the guess is wrong and scratch is used, we rely on the
// retry manager to recompile kernel, this time with scratch pointer.
// This is based on assumption that the only case this will happen is when
// kernel spills; in which case we want to recompile anyway. The only
// downside is that retry manager is forced to select recompiled variant,
// as output without scratch pointer is unusable.
//
// This optimization has a narrow use case:
//   1) Scratch pointer can't be used for anything other than spills.
//   2) Retry manager must be enabled.
//   3) It makes sense to try remove scratch pointer if its' presence
//      is a deciding factor between fitting all kernel arguments in
//      inline data or not. If everything fits with scratch pointer,
//      or will not fit without scratch pointer, there is no point in
//      trying to remove it.
bool COpenCLKernel::canSkipScratchPointer(KernelArgs &args) const {
  if (!m_ctx->platform.isCoreChildOf(IGFX_XE3P_CORE))
    return false;

  if (m_ctx->type == ShaderType::OPENCL_SHADER) {
    auto *OCLCtx = static_cast<const OpenCLProgramContext *>(m_ctx);
    if (OCLCtx->m_DriverInfo.UseScratchSpaceForATSPlus())
      return false;
  }

  if (IGC_IS_FLAG_DISABLED(RemoveImplicitScratchPointer))
    return false;

  if (m_ctx->getModuleMetaData()->compOpt.OptDisable || m_ctx->m_retryManager->IsLastTry() ||
      m_ctx->m_retryManager->kernelSkip.count(entry->getName().str()))
    return false;

  if (m_HasStackCall)
    return false;

  // Check if scratch pointer has other uses.
  uint scratch = m_simdProgram.getScratchSpaceUsageInSlot0();
  if (SeparateSpillAndScratch(m_ctx))
    scratch += m_simdProgram.getScratchSpaceUsageInSlot1();
  if (scratch > 0)
    return false;

  if (entry->getInstructionCount() > IGC_GET_FLAG_VALUE(RemoveImplicitScratchPointerInstThreshold))
    return false;

  // Count size of constant buffer with and without scratch pointer.
  // Removing scratch pointer impacts alignment of following arguments;
  // calculate sizes as separate variables.
  uint constantBufferWithScratchPointer = 0;
  uint constantBufferWithoutScratchPointer = 0;

  for (KernelArgs::const_iterator i = args.begin(), e = args.end(); i != e; ++i) {
    KernelArg arg = *i;
    if (arg.isConstantBuf() && arg.needsAllocation()) {
      if (isUnusedArg(arg))
        continue;

      auto alignment = arg.getAlignment();

      if (arg.isArgPtrType())
        alignment = m_Context->getModule()->getDataLayout().getPointerTypeSize(arg.getArg()->getType());

      constantBufferWithScratchPointer =
          iSTD::Align(constantBufferWithScratchPointer, alignment) + arg.getAllocateSize();

      if (arg.getArgType() != KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER)
        constantBufferWithoutScratchPointer =
            iSTD::Align(constantBufferWithoutScratchPointer, alignment) + arg.getAllocateSize();
    }
  }

  // Scratch pointer not present on the list.
  if (constantBufferWithScratchPointer == constantBufferWithoutScratchPointer)
    return false;

  uint limit = m_ctx->platform.getInlineDataSize();
  return constantBufferWithScratchPointer > limit && constantBufferWithoutScratchPointer <= limit;
}

// Set in thread payload what local IDs are required by the kernel.
// Request all local IDs up to highest used dimension.
// Example: If Y is used, X must also be requested.
void COpenCLKernel::setOCLThreadPayloadLocalIDs(KernelArgs &args) {

  m_kernelInfo.m_threadPayload.HasLocalIDx = false;
  m_kernelInfo.m_threadPayload.HasLocalIDy = false;
  m_kernelInfo.m_threadPayload.HasLocalIDz = false;

  const bool canRemove = AllowRemovingUnusedImplicitLocalIDs(m_Context) && !HasSubroutines() && !HasStackCalls();

  for (KernelArgs::const_iterator i = args.begin(), e = args.end(); i != e; ++i) {
    KernelArg arg = *i;

    if (!arg.isImplicitLocalId())
      continue;

    // If kernel has local ID and removing is disabled, all local IDs are marked as used.
    if (!canRemove) {
      m_kernelInfo.m_threadPayload.HasLocalIDx = true;
      m_kernelInfo.m_threadPayload.HasLocalIDy = true;
      m_kernelInfo.m_threadPayload.HasLocalIDz = true;
      return;
    }

    // If local ID is used, all lower dimesions are also used.
    // Let pass through switch cases to set all required flags.
    if (arg.getArg() && !arg.getArg()->use_empty()) {
      switch (arg.getArgType()) {
      case KernelArg::ArgType::IMPLICIT_LOCAL_ID_Z:
        m_kernelInfo.m_threadPayload.HasLocalIDz = true;
      case KernelArg::ArgType::IMPLICIT_LOCAL_ID_Y:
        m_kernelInfo.m_threadPayload.HasLocalIDy = true;
      case KernelArg::ArgType::IMPLICIT_LOCAL_ID_X:
        m_kernelInfo.m_threadPayload.HasLocalIDx = true;
      default:
        break;
      }
    }
  }
}

bool COpenCLKernel::passNOSInlineData() {
  if (IGC_GET_FLAG_VALUE(EnablePassInlineData) == -1) {
    return false;
  }
  const bool forceEnablePassInlineData = (IGC_GET_FLAG_VALUE(EnablePassInlineData) == 1);
  bool passInlineData = false;
  const bool loadThreadPayload = m_Platform->supportLoadThreadPayloadForCompute();
  const bool inlineDataSupportEnabled =
      (m_Platform->supportInlineDataOCL() && (m_DriverInfo->UseInlineData() || forceEnablePassInlineData));
  if (loadThreadPayload && inlineDataSupportEnabled) {
    passInlineData = true;
    // FIXME: vISA assumes inline data size is 1 GRF, but it's 8 dword in HW.
    // The generated cross-thread-load payload would be incorrect when inline data is enabled on
    // platforms those GRF size are not 8 dword.
    // Passed the value assumed by vISA for error detection at runtime side.
    // vISA should be updated to use 8 dword.
    m_kernelInfo.m_threadPayload.PassInlineDataSize = m_Platform->getInlineDataSize();
  }
  return passInlineData;
}

bool COpenCLKernel::loadThreadPayload() { return true; }

unsigned int COpenCLKernel::GetSLMMappingValue(llvm::Value *c) {
  unsigned int val = 0;
  auto localIter = m_localOffsetsMap.find(c);
  if (localIter != m_localOffsetsMap.end()) {
    val = localIter->second;
  } else {
    IGC_ASSERT_MESSAGE(0, "Trying to access a GlobalVariable not in locals map");
  }
  return val;
}

CVariable *COpenCLKernel::GetSLMMapping(llvm::Value *c) {
  VISA_Type type = GetType(c->getType());
  unsigned int val = GetSLMMappingValue(c);
  return ImmToVariable(val, type);
}

unsigned int COpenCLKernel::getSumFixedTGSMSizes(Function *F) {
  // Find whether we have size information for this kernel.
  // If not, then the total TGSM is 0, otherwise pull it from the MD
  ModuleMetaData *modMD = m_Context->getModuleMetaData();
  auto funcMD = modMD->FuncMD.find(F);
  if (funcMD == modMD->FuncMD.end()) {
    return 0;
  }
  return funcMD->second.localSize;
}

void COpenCLKernel::FillZEKernelArgInfo() {
  auto funcMDIt = m_Context->getModuleMetaData()->FuncMD.find(entry);
  if (funcMDIt == m_Context->getModuleMetaData()->FuncMD.end())
    return;

  FunctionMetaData &funcMD = (*funcMDIt).second;
  uint count = funcMD.m_OpenCLArgAccessQualifiers.size();
  for (uint i = 0; i < count; ++i) {
    zebin::zeInfoArgInfo &argInfo = m_kernelInfo.m_zeKernelArgsInfo.emplace_back();
    argInfo.index = i;
    // argument name is not guaranteed to be present if -cl-kernel-arg-info is not passed in.
    if (funcMD.m_OpenCLArgNames.size() > i)
      argInfo.name = funcMD.m_OpenCLArgNames[i];
    argInfo.address_qualifier = getKernelArgAddressQualifier(funcMD, i);
    argInfo.access_qualifier = getKernelArgAccessQualifier(funcMD, i);
    argInfo.type_name = getKernelArgTypeName(funcMD, i);
    argInfo.type_qualifiers = getKernelArgTypeQualifier(funcMD, i);
  }
}

void COpenCLKernel::FillZEUserAttributes(IGC::IGCMD::FunctionInfoMetaDataHandle &funcInfoMD) {
  // intel_reqd_sub_group_size
  SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
  if (subGroupSize->hasValue()) {
    m_kernelInfo.m_zeUserAttributes.intel_reqd_sub_group_size = subGroupSize->getSIMDSize();
  }

  // intel_reqd_workgroup_walk_order
  auto it = m_Context->getModuleMetaData()->FuncMD.find(entry);
  if (it != m_Context->getModuleMetaData()->FuncMD.end()) {
    WorkGroupWalkOrderMD workgroupWalkOrder = it->second.workGroupWalkOrder;
    if (workgroupWalkOrder.dim0 || workgroupWalkOrder.dim1 || workgroupWalkOrder.dim2) {
      m_kernelInfo.m_zeUserAttributes.intel_reqd_workgroup_walk_order.push_back(workgroupWalkOrder.dim0);
      m_kernelInfo.m_zeUserAttributes.intel_reqd_workgroup_walk_order.push_back(workgroupWalkOrder.dim1);
      m_kernelInfo.m_zeUserAttributes.intel_reqd_workgroup_walk_order.push_back(workgroupWalkOrder.dim2);
    }
  }

  // reqd_work_group_size
  ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
  if (threadGroupSize->hasValue()) {
    m_kernelInfo.m_zeUserAttributes.reqd_work_group_size.push_back(threadGroupSize->getXDim());
    m_kernelInfo.m_zeUserAttributes.reqd_work_group_size.push_back(threadGroupSize->getYDim());
    m_kernelInfo.m_zeUserAttributes.reqd_work_group_size.push_back(threadGroupSize->getZDim());
  }

  // vec_type_hint
  VectorTypeHintMetaDataHandle vecTypeHintInfo = funcInfoMD->getOpenCLVectorTypeHint();
  if (vecTypeHintInfo->hasValue()) {
    m_kernelInfo.m_zeUserAttributes.vec_type_hint = getVecTypeHintTypeString(vecTypeHintInfo);
  }

  // work_group_size_hint
  ThreadGroupSizeMetaDataHandle threadGroupSizeHint = funcInfoMD->getThreadGroupSizeHint();
  if (threadGroupSizeHint->hasValue()) {
    m_kernelInfo.m_zeUserAttributes.work_group_size_hint.push_back(threadGroupSizeHint->getXDim());
    m_kernelInfo.m_zeUserAttributes.work_group_size_hint.push_back(threadGroupSizeHint->getYDim());
    m_kernelInfo.m_zeUserAttributes.work_group_size_hint.push_back(threadGroupSizeHint->getZDim());
  }

  // function attribute added at PoisonFP64KernelsPass for describing the invalid kernel reason "uses-fp64-math"
  const std::string invalidAttributeName = "invalid_kernel(\"uses-fp64-math\")";
  std::string llvmFnAttrStr = entry->getAttributes().getAsString(AttributeList::FunctionIndex);
  if (llvmFnAttrStr.find(invalidAttributeName) != std::string::npos) {
    m_kernelInfo.m_zeUserAttributes.invalid_kernel = "uses-fp64-math";
  }
}

void COpenCLKernel::FillKernel(SIMDMode simdMode) {
  auto pOutput = ProgramOutput();
  if (simdMode == SIMDMode::SIMD32)
    m_kernelInfo.m_kernelProgram.simd32 = *pOutput;
  else if (simdMode == SIMDMode::SIMD16)
    m_kernelInfo.m_kernelProgram.simd16 = *pOutput;
  else if (simdMode == SIMDMode::SIMD8)
    m_kernelInfo.m_kernelProgram.simd8 = *pOutput;

  m_Context->SetSIMDInfo(SIMD_SELECTED, simdMode, ShaderDispatchMode::NOT_APPLICABLE);

  m_kernelInfo.m_executionEnvironment.CompiledSIMDSize = numLanes(simdMode);

  m_kernelInfo.m_executionEnvironment.PerThreadPrivateMemoryUsage =
      pOutput->m_UseScratchSpacePrivateMemory ? pOutput->m_scratchSpaceUsedByShader
                                              : m_perWIStatelessPrivateMemSize * numLanes(simdMode);
  m_kernelInfo.m_executionEnvironment.PerThreadSpillMemoryUsage = pOutput->m_scratchSpaceUsedBySpills;

  m_kernelInfo.m_executionEnvironment.PerThreadScratchSpace = pOutput->getScratchSpaceUsageInSlot0();
  m_kernelInfo.m_executionEnvironment.PerThreadScratchSpaceSlot1 = pOutput->getScratchSpaceUsageInSlot1();
  m_kernelInfo.m_executionEnvironment.PerThreadPrivateOnStatelessSize = m_perWIStatelessPrivateMemSize;
  m_kernelInfo.m_kernelProgram.NOSBufferSize =
      m_State.m_NOSBufferSize / getMinPushConstantBufferAlignmentInBytes(); // in 256 bits
  m_kernelInfo.m_kernelProgram.ConstantBufferLength =
      m_State.m_ConstantBufferLength / getMinPushConstantBufferAlignmentInBytes(); // in 256 bits
  m_kernelInfo.m_kernelProgram.MaxNumberOfThreads = m_Platform->getMaxGPGPUShaderThreads() / GetShaderThreadUsageRate();

  m_kernelInfo.m_executionEnvironment.SumFixedTGSMSizes = getSumFixedTGSMSizes(entry);

  // TODO: need to change misleading HasBarriers to NumberofBarriers
  m_kernelInfo.m_executionEnvironment.HasBarriers = m_State.GetBarrierNumber();
  m_kernelInfo.m_executionEnvironment.HasSample = m_State.GetHasSampleGather4();
  m_kernelInfo.m_executionEnvironment.DisableMidThreadPreemption = GetDisableMidThreadPreemption();
  m_kernelInfo.m_executionEnvironment.SubgroupIndependentForwardProgressRequired =
      m_Context->getModuleMetaData()->compOpt.SubgroupIndependentForwardProgressRequired;
  m_kernelInfo.m_executionEnvironment.CompiledForGreaterThan4GBBuffers =
      m_Context->getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired;
  IGC_ASSERT(m_State.gatherMap.size() == 0);
  m_kernelInfo.m_kernelProgram.gatherMapSize = 0;
  m_kernelInfo.m_kernelProgram.bindingTableEntryCount = 0;

  m_kernelInfo.m_executionEnvironment.HasDeviceEnqueue = false;
  m_kernelInfo.m_executionEnvironment.IsSingleProgramFlow = false;
  // m_kernelInfo.m_executionEnvironment.PerSIMDLanePrivateMemorySize = m_perWIStatelessPrivateMemSize;
  m_kernelInfo.m_executionEnvironment.HasFixedWorkGroupSize = false;
  m_kernelInfo.m_kernelName = entry->getName().str();
  m_kernelInfo.m_ShaderHashCode = m_Context->hash.getAsmHash();

  FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(entry);

  ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
  if (threadGroupSize->hasValue()) {
    m_kernelInfo.m_executionEnvironment.HasFixedWorkGroupSize = true;
    m_kernelInfo.m_executionEnvironment.FixedWorkgroupSize[0] = threadGroupSize->getXDim();
    m_kernelInfo.m_executionEnvironment.FixedWorkgroupSize[1] = threadGroupSize->getYDim();
    m_kernelInfo.m_executionEnvironment.FixedWorkgroupSize[2] = threadGroupSize->getZDim();
  }

  SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
  if (subGroupSize->hasValue()) {
    m_kernelInfo.m_executionEnvironment.CompiledSIMDSize = subGroupSize->getSIMDSize();
  }

  auto &FuncMap = m_Context->getModuleMetaData()->FuncMD;
  auto FuncIter = FuncMap.find(entry);
  if (FuncIter != FuncMap.end()) {
    IGC::FunctionMetaData funcMD = FuncIter->second;
    WorkGroupWalkOrderMD workGroupWalkOrder = funcMD.workGroupWalkOrder;

    if (workGroupWalkOrder.dim0 || workGroupWalkOrder.dim1 || workGroupWalkOrder.dim2) {
      m_kernelInfo.m_executionEnvironment.WorkgroupWalkOrder[0] = workGroupWalkOrder.dim0;
      m_kernelInfo.m_executionEnvironment.WorkgroupWalkOrder[1] = workGroupWalkOrder.dim1;
      m_kernelInfo.m_executionEnvironment.WorkgroupWalkOrder[2] = workGroupWalkOrder.dim2;
    }

    m_kernelInfo.m_executionEnvironment.IsInitializer = funcMD.IsInitializer;
    m_kernelInfo.m_executionEnvironment.IsFinalizer = funcMD.IsFinalizer;

    m_kernelInfo.m_executionEnvironment.CompiledSubGroupsNumber = funcMD.CompiledSubGroupsNumber;

    m_kernelInfo.m_executionEnvironment.HasRTCalls = funcMD.hasSyncRTCalls;
    m_kernelInfo.m_executionEnvironment.HasPrintfCalls = funcMD.hasPrintfCalls;
    m_kernelInfo.m_executionEnvironment.RequireAssertBuffer = funcMD.requireAssertBuffer;
    m_kernelInfo.m_executionEnvironment.RequireSyncBuffer = funcMD.requireSyncBuffer;
    m_kernelInfo.m_executionEnvironment.HasIndirectCalls = funcMD.hasIndirectCalls;
  }

  m_kernelInfo.m_executionEnvironment.HasGlobalAtomics = GetHasGlobalAtomics();
  m_kernelInfo.m_threadPayload.OffsetToSkipPerThreadDataLoad = ProgramOutput()->m_offsetToSkipPerThreadDataLoad;
  m_kernelInfo.m_threadPayload.OffsetToSkipSetFFIDGP = ProgramOutput()->m_offsetToSkipSetFFIDGP;

  m_kernelInfo.m_executionEnvironment.NumGRFRequired = ProgramOutput()->m_numGRFTotal;

  m_kernelInfo.m_executionEnvironment.HasDPAS = m_State.GetHasDPAS();
  m_kernelInfo.m_executionEnvironment.StatelessWritesCount = GetStatelessWritesCount();
  m_kernelInfo.m_executionEnvironment.IndirectStatelessCount = GetIndirectStatelessCount();
  m_kernelInfo.m_executionEnvironment.numThreads = ProgramOutput()->m_numThreads;

  m_kernelInfo.m_executionEnvironment.UseBindlessMode = m_Context->m_InternalOptions.UseBindlessMode;
  m_kernelInfo.m_executionEnvironment.HasStackCalls = HasStackCalls();

  if (m_Context->m_DriverInfo.getLscStoresWithNonDefaultL1CacheControls()) {
    m_kernelInfo.m_executionEnvironment.HasLscStoresWithNonDefaultL1CacheControls =
        m_State.GetHasLscStoresWithNonDefaultL1CacheControls();
  }

  FillZEKernelArgInfo();
  FillZEUserAttributes(funcInfoMD);
}

void COpenCLKernel::RecomputeBTLayout() {
  CodeGenContext *pCtx = GetContext();
  ModuleMetaData *modMD = pCtx->getModuleMetaData();
  FunctionMetaData *funcMD = &modMD->FuncMD[entry];
  ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
  // Get the number of UAVs and Resources from MD.
  int numUAVs = resAllocMD->uavsNumType;
  int numResources = resAllocMD->srvsNumType;

  // Now, update the layout information
  USC::SShaderStageBTLayout *layout = ((COCLBTILayout *)m_pBtiLayout)->getModifiableLayout();

  // The BT layout contains the minimum and the maximum number BTI for each kind
  // of resource. E.g. UAVs may be mapped to BTIs 0..3, SRVs to 4..5, and the scratch
  // surface to 6.
  // Note that the names are somewhat misleading. They are used for the sake of consistency
  // with the ICBE sources.

  // Some fields are always 0 for OCL.
  layout->resourceNullBoundOffset = 0;
  layout->immediateConstantBufferOffset = 0;
  layout->interfaceConstantBufferOffset = 0;
  layout->constantBufferNullBoundOffset = 0;
  layout->JournalIdx = 0;
  layout->JournalCounterIdx = 0;

  // And TGSM (aka SLM) is always 254.
  layout->TGSMIdx = 254;

  int index = 0;

  // First, allocate BTI for debug surface
  if (m_Context->m_InternalOptions.KernelDebugEnable) {
    layout->systemThreadIdx = index++;
  }

  // Now, allocate BTIs for all the SRVs.
  layout->minResourceIdx = index;
  if (numResources) {
    index += numResources - 1;
    layout->maxResourceIdx = index++;
  } else {
    layout->maxResourceIdx = index;
  }

  // Now, ConstantBuffers - used as a placeholder for the inline constants, if present.
  layout->minConstantBufferIdx = index;
  layout->maxConstantBufferIdx = index;

  // Now, the UAVs
  layout->minUAVIdx = index;
  if (numUAVs) {
    index += numUAVs - 1;
    layout->maxUAVIdx = index++;
  } else {
    layout->maxUAVIdx = index;
  }

  // And finally, the scratch surface
  layout->surfaceScratchIdx = index++;

  // Overall number of used BT entries, not including TGSM.
  layout->maxBTsize = index;
}

bool COpenCLKernel::HasFullDispatchMask() {
  unsigned int groupSize = IGCMetaDataHelper::getThreadGroupSize(*m_pMdUtils, entry);
  if (groupSize != 0) {
    if (groupSize % numLanes(m_State.m_dispatchSize) == 0) {
      return true;
    }
  }
  return false;
}

unsigned int COpenCLKernel::getBTI(SOpenCLKernelInfo::SResourceInfo &resInfo) {
  switch (resInfo.Type) {
  case SOpenCLKernelInfo::SResourceInfo::RES_UAV:
    return m_pBtiLayout->GetUavIndex(resInfo.Index);
  case SOpenCLKernelInfo::SResourceInfo::RES_SRV:
    return m_pBtiLayout->GetTextureIndex(resInfo.Index);
  default:
    return 0xffffffff;
  }
}

void CollectProgramInfo(OpenCLProgramContext *ctx) {
  MetaDataUtils mdUtils(ctx->getModule());
  ModuleMetaData *modMD = ctx->getModuleMetaData();

  if (modMD->inlineBuffers[InlineProgramScopeBufferType::Constants].allocSize ||
      modMD->inlineBuffers[InlineProgramScopeBufferType::ConstantStrings].allocSize) {
    // For ZeBin, constants are mantained in two separate buffers
    // the first is for general constants, and the second for string literals

    // General constants
    auto &ipsbMDHandle = modMD->inlineBuffers[InlineProgramScopeBufferType::Constants];
    std::unique_ptr<iOpenCL::InitConstantAnnotation> initConstant(new iOpenCL::InitConstantAnnotation());
    initConstant->Alignment = ipsbMDHandle.alignment;
    initConstant->AllocSize = ipsbMDHandle.allocSize;

    size_t bufferSize = (ipsbMDHandle.Buffer).size();
    initConstant->InlineData.resize(bufferSize);
    memcpy_s(initConstant->InlineData.data(), bufferSize, ipsbMDHandle.Buffer.data(), bufferSize);

    ctx->m_programInfo.m_initConstantAnnotation = std::move(initConstant);

    // String literals
    auto &ipsbStringMDHandle = modMD->inlineBuffers[InlineProgramScopeBufferType::ConstantStrings];
    std::unique_ptr<iOpenCL::InitConstantAnnotation> initStringConstant(new iOpenCL::InitConstantAnnotation());
    initStringConstant->Alignment = ipsbStringMDHandle.alignment;
    initStringConstant->AllocSize = ipsbStringMDHandle.allocSize;

    bufferSize = (ipsbStringMDHandle.Buffer).size();
    initStringConstant->InlineData.resize(bufferSize);
    memcpy_s(initStringConstant->InlineData.data(), bufferSize, ipsbStringMDHandle.Buffer.data(), bufferSize);

    ctx->m_programInfo.m_initConstantStringAnnotation = std::move(initStringConstant);
  }

  if (modMD->inlineBuffers[InlineProgramScopeBufferType::Globals].allocSize) {
    auto &ipsbMDHandle = modMD->inlineBuffers[InlineProgramScopeBufferType::Globals];

    std::unique_ptr<iOpenCL::InitGlobalAnnotation> initGlobal(new iOpenCL::InitGlobalAnnotation());
    initGlobal->Alignment = ipsbMDHandle.alignment;
    initGlobal->AllocSize = ipsbMDHandle.allocSize;

    size_t bufferSize = (ipsbMDHandle.Buffer).size();
    initGlobal->InlineData.resize(bufferSize);
    memcpy_s(initGlobal->InlineData.data(), bufferSize, ipsbMDHandle.Buffer.data(), bufferSize);

    ctx->m_programInfo.m_initGlobalAnnotation = std::move(initGlobal);
  }

  // Pointer address relocation table data for GLOBAL buffer
  for (const auto &globalRelocEntry : modMD->GlobalBufferAddressRelocInfo) {
    ctx->m_programInfo.m_GlobalPointerAddressRelocAnnotation.globalReloc.emplace_back(
        (globalRelocEntry.PointerSize == 8) ? vISA::GenRelocType::R_SYM_ADDR : vISA::GenRelocType::R_SYM_ADDR_32,
        (uint32_t)globalRelocEntry.BufferOffset, globalRelocEntry.Symbol);
  }
  // Pointer address relocation table data for CONST buffer
  for (const auto &constRelocEntry : modMD->ConstantBufferAddressRelocInfo) {
    ctx->m_programInfo.m_GlobalPointerAddressRelocAnnotation.globalConstReloc.emplace_back(
        (constRelocEntry.PointerSize == 8) ? vISA::GenRelocType::R_SYM_ADDR : vISA::GenRelocType::R_SYM_ADDR_32,
        (uint32_t)constRelocEntry.BufferOffset, constRelocEntry.Symbol);
  }
}

bool COpenCLKernel::IsValidShader(COpenCLKernel *pShader) {
  return pShader && (pShader->ProgramOutput()->m_programSize > 0);
}

bool COpenCLKernel::IsVisaCompiledSuccessfullyForShader(COpenCLKernel *pShader) {
  return pShader && pShader->GetEncoder().IsVisaCompiledSuccessfully();
}

bool COpenCLKernel::IsVisaCompileStatusFailureForShader(COpenCLKernel *pShader) {
  return pShader && pShader->GetEncoder().IsVisaCompileStatusFailure();
}

enum class RetryType {
  NO_Retry,
  NO_Retry_Pick_Prv,
  NO_Retry_ExceedScratch,
  NO_Retry_WorseStatelessPrivateMemSize,
  YES_Retry,
  YES_ForceRecompilation
};

static unsigned long getScratchUse(CShader *shader, OpenCLProgramContext *ctx) {
  unsigned int totalScratchUse = shader->ProgramOutput()->m_scratchSpaceUsedBySpills;
  if (shader->ProgramOutput()->m_UseScratchSpacePrivateMemory)
    totalScratchUse += shader->ProgramOutput()->m_scratchSpaceUsedByShader;
  return totalScratchUse;
}

static bool exceedMaxScratchUse(CShader *shader, OpenCLProgramContext *ctx) {
  return shader && getScratchUse(shader, ctx) > shader->ProgramOutput()->m_scratchSpaceSizeLimit;
}

static bool isWorsePrivateMemSize(CShader *shader, IGC::CShaderProgram *pPrevKernel) {
  for (auto mode : {SIMDMode::SIMD8, SIMDMode::SIMD16, SIMDMode::SIMD32}) {
    auto pPrevShader = pPrevKernel->GetShader(mode);
    if (pPrevShader) {
      // if after retry the current function generate 10x more (and base version had more than 5Kb)
      // private memory in global memory - consider using previous kernel
      if (pPrevShader->PrivateMemoryPerWI() > 5000 &&
          pPrevShader->PrivateMemoryPerWI() * 10 < shader->PrivateMemoryPerWI()) {
        return true;
      }
      break;
    }
  }

  return false;
}

RetryType NeedsRetry(OpenCLProgramContext *ctx, COpenCLKernel *pShader, CShaderProgram::UPtr &pKernel, Function *pFunc,
                     MetaDataUtils *pMdUtils, SIMDMode simdMode) {
  IGC_ASSERT(pShader && pKernel);
  pShader->FillKernel(simdMode);
  SProgramOutput *pOutput = pShader->ProgramOutput();

  CShader *program = pKernel.get()->GetShader(simdMode);

  if (pShader->TryNoScratchPointer() && getScratchUse(pShader, ctx) > 0) {
    // If IGC removed scratch pointer, yet vISA requires scratch, force retry.
    IGC_ASSERT(!ctx->m_retryManager->IsLastTry());
    return RetryType::YES_Retry;
  }

  bool isWorstThanPrv = false;
  // Look for previous generated shaders
  // ignoring case for multi-simd compilation, or if kernel has stackcalls
  RetryManager *retryMgr = ctx->m_retryManager.get();
  if (!((ctx->m_enableSimdVariantCompilation) && (ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)) &&
      !(program->HasStackCalls() || program->IsIntelSymbolTableVoidProgram())) {
    isWorstThanPrv = !retryMgr->IsBetterThanPrevious(pKernel.get(), 2.0f);
  }

  auto pPreviousKernel = retryMgr->GetPrevious(pKernel.get());

  if (pPreviousKernel && exceedMaxScratchUse(program, ctx)) {
    // For case when we have recompilation but exceed
    // the scratch space in recompiled kernel
    return RetryType::NO_Retry_ExceedScratch;
  } else if (IGC_IS_FLAG_ENABLED(ForceRecompilation) && !ctx->m_retryManager->IsLastTry()) {
    return RetryType::YES_ForceRecompilation;
  } else if (pPreviousKernel && isWorsePrivateMemSize(program, pPreviousKernel)) {
    // For case when we have recompilation but generate 20x more
    // private memory in global memory in recompiled kernel
    return RetryType::NO_Retry_WorseStatelessPrivateMemSize;
  } else if (pShader->IsRecompilationRequestForced() && !ctx->m_retryManager->IsLastTry()) {
    return RetryType::YES_Retry;
  } else if (isWorstThanPrv) {
    return RetryType::NO_Retry_Pick_Prv;
  } else if (!ctx->hasSpills(pOutput->m_scratchSpaceUsedBySpills, pOutput->m_numGRFTotal) ||
             ctx->getModuleMetaData()->compOpt.OptDisable || ctx->m_retryManager->IsLastTry() ||
             (!ctx->m_retryManager->kernelSkip.empty() &&
              ctx->m_retryManager->kernelSkip.count(pFunc->getName().str()))) {
    return RetryType::NO_Retry;
  } else {
    return RetryType::YES_Retry;
  }
}

void GatherDataForDriver(OpenCLProgramContext *ctx, COpenCLKernel *pShader, CShaderProgram::UPtr pKernel,
                         Function *pFunc, MetaDataUtils *pMdUtils, SIMDMode simdMode) {
  IGC_ASSERT_EXIT(ctx && pShader && pKernel && pFunc && pMdUtils);

  CShaderProgram::UPtr pSelectedKernel;
  switch (auto retryType = NeedsRetry(ctx, pShader, pKernel, pFunc, pMdUtils, simdMode)) {
  case RetryType::NO_Retry_WorseStatelessPrivateMemSize:
  case RetryType::NO_Retry_ExceedScratch:
  case RetryType::NO_Retry_Pick_Prv: {
    // In case retry compilation give worst generated kernel
    // consider using the previous one do not retry on this
    // kernel again
    std::ostringstream reason("[RetryManager] Used previous version of the kernel, reason : ", std::ostringstream::ate);

    switch (retryType) {
    case RetryType::NO_Retry_WorseStatelessPrivateMemSize:
      reason << "NO_Retry_WorseStatelessPrivateMemSize";
      break;
    case RetryType::NO_Retry_ExceedScratch:
      reason << "NO_Retry_ExceedScratch";
      break;
    case RetryType::NO_Retry_Pick_Prv:
      reason << "NO_Retry_Pick_Prv";
      break;
    default:
      reason << "Unknown";
      break;
    }
    ctx->EmitWarning(reason.str().c_str(), pFunc);

    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
      // Set mark on the version which we pick in shader dumps
      IGC::Debug::DumpName dumpName =
          IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName()).Type(ctx->type).Hash(ctx->hash).StagedInfo(ctx);

      std::string shaderName(pShader->entry->getName().str());
      pShader->getShaderFileName(shaderName);

      dumpName = dumpName.PostFix(shaderName);

      if (dumpName.allow()) {
        std::ostringstream FullPath(dumpName.str(), std::ostringstream::ate);
        FullPath << "_previous_kernel_pick.txt";

        std::ofstream OutF(FullPath.str(), std::ofstream::out);

        if (OutF)
          OutF.write(reason.str().c_str(), reason.str().length());
      }
    }
    RetryManager *retryMgr = ctx->m_retryManager.get();
    pSelectedKernel = CShaderProgram::UPtr(retryMgr->GetPrevious(pKernel.get(), true));
  }
  case RetryType::NO_Retry: {
    // Save the shader program to the state processor to be handled later
    if (!pSelectedKernel) {
      pSelectedKernel = std::move(pKernel);
    }
  }
    // Common part for NO_Retry:
    {
      if (pSelectedKernel) {
        COMPILER_SHADER_STATS_PRINT(pSelectedKernel->m_shaderStats, ShaderType::OPENCL_SHADER, ctx->hash,
                                    pFunc->getName().str());
        COMPILER_SHADER_STATS_SUM(ctx->m_sumShaderStats, pSelectedKernel->m_shaderStats, ShaderType::OPENCL_SHADER);
        COMPILER_SHADER_STATS_DEL(pSelectedKernel->m_shaderStats);
        ctx->m_programOutput.m_ShaderProgramList.push_back(std::move(pSelectedKernel));
      }
      break;
    }
  case RetryType::YES_Retry:
  case RetryType::YES_ForceRecompilation: {
    ctx->EmitWarning("[RetryManager] Start recompilation of the kernel", pFunc);
    // Collect the current compilation for the next compare
    ctx->m_retryManager->Collect(std::move(pKernel));
    ctx->m_retryManager->kernelSet.insert(pShader->m_kernelInfo.m_kernelName);

    break;
  }
  }
}

static bool verifyHasOOBScratch(OpenCLProgramContext *ctx, COpenCLKernel *simd8Shader, COpenCLKernel *simd16Shader,
                                COpenCLKernel *simd32Shader) {
  auto verify = [ctx](CShader *shader) {
    if (exceedMaxScratchUse(shader, ctx)) {
      return true;
    }
    return false;
  };

  // Need to check if simd* shader is not nullptr and its vISA compile status,
  // since it may be created without going through full vISA compilation and
  // the spill size record may be invalid
  bool result = false;
  if (simd8Shader && !COpenCLKernel::IsVisaCompileStatusFailureForShader(simd8Shader))
    result |= verify(simd8Shader);
  else if (simd16Shader && !COpenCLKernel::IsVisaCompileStatusFailureForShader(simd16Shader))
    result |= verify(simd16Shader);
  else if (simd32Shader && !COpenCLKernel::IsVisaCompileStatusFailureForShader(simd32Shader))
    result |= verify(simd32Shader);

  return result;
}

static void CodeGen(OpenCLProgramContext *ctx, CShaderProgram::KernelShaderMap &shaders) {
  COMPILER_TIME_START(ctx, TIME_CodeGen);
  COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);

  IGCPassManager Passes(ctx, "CG");

  AddLegalizationPasses(*ctx, Passes);

  AddAnalysisPasses(*ctx, Passes);

  if (ctx->m_enableFunctionPointer && (ctx->m_enableSimdVariantCompilation) &&
      ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0) {
    // In order to support compiling multiple SIMD modes for function pointer calls,
    // we require a separate pass manager per SIMD mode, due to interdependencies across
    // function compilations.
    SIMDMode pass1Mode;
    SIMDMode pass2Mode;

    if (ctx->platform.getMinDispatchMode() == SIMDMode::SIMD16) {
      pass1Mode = SIMDMode::SIMD32;
      pass2Mode = SIMDMode::SIMD16;
    } else {
      pass1Mode = SIMDMode::SIMD16;
      pass2Mode = SIMDMode::SIMD8;
    }

    // Run first pass
    AddCodeGenPasses(*ctx, shaders, Passes, pass1Mode, false);
    Passes.run(*(ctx->getModule()));

    // Create and run second pass
    IGCPassManager Passes2(ctx, "CG2");
    // Add required immutable passes
    Passes2.add(new MetaDataUtilsWrapper(ctx->getMetaDataUtils(), ctx->getModuleMetaData()));
    Passes2.add(new CodeGenContextWrapper(ctx));
    Passes2.add(createGenXFunctionGroupAnalysisPass());
    AddCodeGenPasses(*ctx, shaders, Passes2, pass2Mode, false);
    COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);
    Passes2.run(*(ctx->getModule()));

    COMPILER_TIME_END(ctx, TIME_CodeGen);
    DumpLLVMIR(ctx, "codegen");
    return;
  }

  if (ctx->platform.getMinDispatchMode() == SIMDMode::SIMD16) {
    bool abortOnSpills = IGC_GET_FLAG_VALUE(AllowSIMD16DropForXE2Plus) &&
                         (ctx->platform.isCoreXE2() || ctx->platform.isCoreXE3()) &&
                         (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32);
    AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, abortOnSpills);
    AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD16, false);
    ctx->SetSIMDInfo(SIMD_SKIP_HW, SIMDMode::SIMD8, ShaderDispatchMode::NOT_APPLICABLE);
  } else {
    // The order in which we call AddCodeGenPasses matters, please to not change order
    AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));
    AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD16, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 16));
    AddCodeGenPasses(*ctx, shaders, Passes, SIMDMode::SIMD8, false);
  }
  Passes.add(new DebugInfoPass(shaders));
  COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

  Passes.run(*(ctx->getModule()));
  COMPILER_TIME_END(ctx, TIME_CodeGen);
  DumpLLVMIR(ctx, "codegen");
}

void CodeGen(OpenCLProgramContext *ctx) {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
  // Do program-wide code generation.
  // Currently, this just creates the program-scope patch stream.
  if (ctx->m_retryManager->IsFirstTry()) {
    CollectProgramInfo(ctx);
  }

  MetaDataUtils *pMdUtils = ctx->getMetaDataUtils();

  // Clear spill parameters of retry manager in the very begining of code gen
  ctx->m_retryManager->ClearSpillParams();
  // early retry kernel set should always be empty before compilation
  ctx->m_retryManager->earlyRetryKernelSet.clear();

  CShaderProgram::KernelShaderMap shaders;
  CodeGen(ctx, shaders);

  if (ctx->m_programOutput.m_pSystemThreadKernelOutput == nullptr) {
    const auto options = ctx->m_InternalOptions;
    if (options.IncludeSIPCSR || options.IncludeSIPKernelDebug || options.IncludeSIPKernelDebugWithLocalMemory ||
        options.KernelDebugEnable) {
      DWORD systemThreadMode = 0;

      if (options.IncludeSIPCSR) {
        systemThreadMode |= USC::SYSTEM_THREAD_MODE_CSR;
      }

      if (options.KernelDebugEnable || options.IncludeSIPKernelDebug) {
        systemThreadMode |= USC::SYSTEM_THREAD_MODE_DEBUG;
      }

      if (options.IncludeSIPKernelDebugWithLocalMemory) {
        systemThreadMode |= USC::SYSTEM_THREAD_MODE_DEBUG_LOCAL;
      }

      bool success = SIP::CSystemThread::CreateSystemThreadKernel(
          ctx->platform, (USC::SYSTEM_THREAD_MODE)systemThreadMode, ctx->m_programOutput.m_pSystemThreadKernelOutput);

      if (!success) {
        ctx->EmitError("System thread kernel could not be created!", nullptr);
      }
    }
  }

  // Clear the retry set and collect kernels for retry in the loop below.
  ctx->m_retryManager->kernelSet.clear();

  // If kernel needs retry, its shaderProgram should be deleted
  SmallVector<CShaderProgram *, 8> toBeDeleted;
  // gather data to send back to the driver
  for (const auto &k : shaders) {
    Function *pFunc = k.first;
    CShaderProgram::UPtr pKernel = CShaderProgram::UPtr(static_cast<CShaderProgram *>(k.second));
    COpenCLKernel *simd8Shader = static_cast<COpenCLKernel *>(pKernel->GetShader(SIMDMode::SIMD8));
    COpenCLKernel *simd16Shader = static_cast<COpenCLKernel *>(pKernel->GetShader(SIMDMode::SIMD16));
    COpenCLKernel *simd32Shader = static_cast<COpenCLKernel *>(pKernel->GetShader(SIMDMode::SIMD32));

    if ((ctx->m_enableSimdVariantCompilation) && (ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)) {
      // Gather the kernel binary for each compiled kernel
      if (COpenCLKernel::IsValidShader(simd32Shader))
        GatherDataForDriver(ctx, simd32Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD32);
      if (COpenCLKernel::IsValidShader(simd16Shader))
        GatherDataForDriver(ctx, simd16Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD16);
      if (COpenCLKernel::IsValidShader(simd8Shader))
        GatherDataForDriver(ctx, simd8Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD8);
      // TODO: check if we need to invoke verifyOOBScratch(...) here
    } else if (ctx->m_InternalOptions.EmitVisaOnly) {
      if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd32Shader))
        GatherDataForDriver(ctx, simd32Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD32);
      else if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd16Shader))
        GatherDataForDriver(ctx, simd16Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD16);
      else if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(simd8Shader))
        GatherDataForDriver(ctx, simd8Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD8);
    } else if (ctx->m_retryManager->earlyRetryKernelSet.count(pFunc->getName().str())) {
      ctx->EmitWarning("[RetryManager] Start recompilation of the kernel", pFunc);
      ctx->m_retryManager->kernelSet.insert(pFunc->getName().str());
    } else {
      // Gather the kernel binary only for 1 SIMD mode of the kernel
      if (COpenCLKernel::IsValidShader(simd32Shader))
        GatherDataForDriver(ctx, simd32Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD32);
      else if (COpenCLKernel::IsValidShader(simd16Shader))
        GatherDataForDriver(ctx, simd16Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD16);
      else if (COpenCLKernel::IsValidShader(simd8Shader))
        GatherDataForDriver(ctx, simd8Shader, std::move(pKernel), pFunc, pMdUtils, SIMDMode::SIMD8);
      else if (verifyHasOOBScratch(ctx, simd8Shader, simd16Shader, simd32Shader)) {
        // Get the simd* shader with the OOB access.
        COpenCLKernel *shader = exceedMaxScratchUse(simd32Shader, ctx)   ? simd32Shader
                                : exceedMaxScratchUse(simd16Shader, ctx) ? simd16Shader
                                : exceedMaxScratchUse(simd8Shader, ctx)  ? simd8Shader
                                                                         : nullptr;

        IGC_ASSERT(shader);

        if (!ctx->m_retryManager->IsLastTry() && !ctx->getModuleMetaData()->compOpt.OptDisable) {
          // If this is not the last try, force retry on this kernel to potentially avoid
          // OOB access on the next try by reducing spill size and thus SS usage.
          ctx->m_retryManager->kernelSet.insert(shader->entry->getName().str());
        } else {
          auto funcInfoMD = ctx->getMetaDataUtils()->getFunctionsInfoItem(pFunc);
          int reqdSubGroupSize = funcInfoMD->getSubGroupSize()->getSIMDSize();
          if (ctx->m_ForceSIMDRPELimit != 0 && !reqdSubGroupSize) {
            ctx->m_ForceSIMDRPELimit = 0;
            ctx->m_retryManager->kernelSet.insert(shader->entry->getName().str());
            ctx->EmitWarning("we couldn't compile without exceeding max permitted PTSS, drop SIMD \n", nullptr);
            ctx->m_retryManager->DecreaseState();
          } else {
            std::string errorMsg = "total scratch space exceeds HW "
                                   "supported limit for kernel " +
                                   shader->entry->getName().str() + ": " + std::to_string(getScratchUse(shader, ctx)) +
                                   " bytes (max permitted PTSS " +
                                   std::to_string(shader->ProgramOutput()->m_scratchSpaceSizeLimit) + " bytes)";

            ctx->EmitError(errorMsg.c_str(), nullptr);
          }
        }
      }
    }
  }

  // The skip set to avoid retry is not needed. Clear it and collect a new set
  // during retry compilation.
  ctx->m_retryManager->kernelSkip.clear();
#endif // ifndef VK_ONLY_IGC
#endif // ifndef DX_ONLY_IGC
}

bool COpenCLKernel::hasReadWriteImage(llvm::Function &F) {
  if (!isEntryFunc(m_pMdUtils, &F)) {
    // Ignore read/write flags for subroutines for now.
    // TODO: get access types for subroutines without using kernel args
    return false;
  }

  KernelArgs kernelArgs(F, m_DL, m_pMdUtils, m_ModuleMetadata, getGRFSize(), KernelArgsOrder::InputType::INDEPENDENT);
  for (const auto &KA : kernelArgs) {
    // RenderScript annotation sets "read_write" qualifier
    // for any applicable kernel argument, not only for kernel arguments
    // that are images, so we should check if kernel argument is an image.
    if (KA.getAccessQual() == KernelArg::AccessQual::READ_WRITE && KA.getArgType() >= KernelArg::ArgType::IMAGE_1D &&
        KA.getArgType() <= KernelArg::ArgType::BINDLESS_IMAGE_CUBE_DEPTH_ARRAY) {
      return true;
    }
  }
  return false;
}

bool COpenCLKernel::CompileSIMDSize(SIMDMode simdMode, EmitPass &EP, llvm::Function &F) {
  if (!CompileSIMDSizeInCommon(simdMode))
    return false;

  // Skip compiling the simd size if -emit-visa-only and there's already
  // one visa compiled for any other simd size.
  if (m_Context->m_InternalOptions.EmitVisaOnly) {
    for (auto mode : {SIMDMode::SIMD8, SIMDMode::SIMD16, SIMDMode::SIMD32}) {
      COpenCLKernel *shader = static_cast<COpenCLKernel *>(m_parent->GetShader(mode));
      if (COpenCLKernel::IsVisaCompiledSuccessfullyForShader(shader))
        return false;
    }
  }

  if (!m_Context->m_retryManager->IsFirstTry()) {
    m_Context->ClearSIMDInfo(simdMode, ShaderDispatchMode::NOT_APPLICABLE);
    m_Context->SetSIMDInfo(SIMD_RETRY, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
  }

  // Currently the FunctionMetaData is being looked up solely in order to get the hasSyncRTCalls
  // If we would need to get some non-raytracing related field out of the FunctionMetaData,
  // then we can move the lookup out of the #if and just leave the bool hasSyncRTCalls inside.
  auto &FuncMap = m_Context->getModuleMetaData()->FuncMD;
  // we want to check the setting for the associated kernel
  auto FuncIter = FuncMap.find(entry);
  if (FuncIter == FuncMap.end()) { // wasn't able to find the meta data for the passed in llvm::Function!
    // All of the kernels should have an entry in the map.
    IGC_ASSERT(0);
    return false;
  }
  const FunctionMetaData &funcMD = FuncIter->second;
  bool hasSyncRTCalls = funcMD.hasSyncRTCalls; // if the function/kernel has sync raytracing calls

  // If forced SIMD Mode (by driver or regkey), then:
  //  1. Compile only that SIMD mode and nothing else
  //  2. Compile that SIMD mode even if it is not profitable, i.e. even if compileThisSIMD() returns false for it.
  //     So, don't bother checking profitability for it

  unsigned char forcedSIMDSize = m_Context->getModuleMetaData()->csInfo.forcedSIMDSize;

  if (forcedSIMDSize != 0) {
    // Check if forced SIMD width is smaller than required by used platform. Emit error when true.
    if (m_Context->platform.getMinDispatchMode() == SIMDMode::SIMD16 && forcedSIMDSize < 16) {
      m_Context->EmitError((std::string("SIMD size of ") + std::to_string(forcedSIMDSize) +
                            std::string(" has been forced when SIMD size of at least 16") +
                            std::string(" is required on this platform"))
                               .c_str(),
                           &F);
      return false;
    }
    // Entered here means driver has requested a specific SIMD mode, which was forced in the regkey ForceOCLSIMDWidth.
    // We return the condition can we compile the given forcedSIMDSize with this simdMode?
    return (
        // These statements are basically equivalent to (simdMode == forcedSIMDSize)
        (simdMode == SIMDMode::SIMD8 && m_Context->getModuleMetaData()->csInfo.forcedSIMDSize == 8) ||
        (simdMode == SIMDMode::SIMD16 && m_Context->getModuleMetaData()->csInfo.forcedSIMDSize == 16) ||
        // if we want to compile SIMD32, we need to be lacking any raytracing calls; raytracing doesn't support SIMD16
        (simdMode == SIMDMode::SIMD32 && m_Context->getModuleMetaData()->csInfo.forcedSIMDSize == 32 &&
         !hasSyncRTCalls));
  }

  SIMDStatus simdStatus = SIMDStatus::SIMD_FUNC_FAIL;

  if (m_Context->platform.getMinDispatchMode() == SIMDMode::SIMD16) {
    simdStatus = checkSIMDCompileCondsForMin16(simdMode, EP, F, hasSyncRTCalls);
  } else {
    simdStatus = checkSIMDCompileConds(simdMode, EP, F, hasSyncRTCalls);
  }

  // Func and Perf checks pass, compile this SIMD
  if (simdStatus == SIMDStatus::SIMD_PASS) {
    return true;
  }
  // Report an error if intel_reqd_sub_group_size cannot be satisfied
  else {
    MetaDataUtils *pMdUtils = EP.getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CodeGenContext *ctx = GetContext();
    auto reqdSubGroupSize = getReqdSubGroupSize(F, pMdUtils);
    if (reqdSubGroupSize == numLanes(simdMode)) {
      ctx->EmitError((std::string("Cannot compile a kernel in the SIMD mode specified by intel_reqd_sub_group_size(") +
                      std::to_string(reqdSubGroupSize) + std::string(")"))
                         .c_str(),
                     &F);
      return false;
    }
  }

  // Functional failure, skip compiling this SIMD
  if (simdStatus == SIMDStatus::SIMD_FUNC_FAIL)
    return false;

  IGC_ASSERT(simdStatus == SIMDStatus::SIMD_PERF_FAIL);

  return simdStatus == SIMDStatus::SIMD_PASS;
}

static bool shouldDropToSIMD16(uint32_t maxPressure, SIMDMode simdMode, CodeGenContext *pCtx, MetaDataUtils *pMdUtils,
                               llvm::Function *F) {
  if (simdMode != SIMDMode::SIMD32 || !isEntryFunc(pMdUtils, F)) {
    return false;
  }
  if (!pCtx->isAutoGRFSelectionEnabled() || pCtx->getNumGRFPerThread(false) != 0) {
    return false;
  }
  auto threshold = IGC_GET_FLAG_VALUE(EarlySIMD16DropForXE3Threshold);
  bool shouldDrop = maxPressure > threshold;
  return shouldDrop;
}

SIMDStatus COpenCLKernel::checkSIMDCompileCondsForMin16(SIMDMode simdMode, EmitPass &EP, llvm::Function &F,
                                                        bool hasSyncRTCalls) {
  if (simdMode == SIMDMode::SIMD8) {
    return SIMDStatus::SIMD_FUNC_FAIL;
  }

  // Next we check if there is a required sub group size specified
  CodeGenContext *pCtx = GetContext();

  CShader *simd16Program = m_parent->GetShader(SIMDMode::SIMD16);
  CShader *simd32Program = m_parent->GetShader(SIMDMode::SIMD32);

  bool compileFunctionVariants =
      pCtx->m_enableSimdVariantCompilation && (m_FGA && IGC::isIntelSymbolTableVoidProgram(m_FGA->getGroupHead(&F)));

  if ((simd16Program && simd16Program->ProgramOutput()->m_programSize > 0) ||
      (simd32Program && simd32Program->ProgramOutput()->m_programSize > 0)) {
    bool canCompileMultipleSIMD = compileFunctionVariants;
    if (!(canCompileMultipleSIMD && (pCtx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)))
      return SIMDStatus::SIMD_FUNC_FAIL;
  }

  MetaDataUtils *pMdUtils = EP.getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);
  uint32_t requiredSimdSize = getReqdSubGroupSize(F, pMdUtils);

  // there is a requirement for specific compilation size, we can't abort on simd32
  if (requiredSimdSize != 0 && !(requiredSimdSize < 32 && SIMDMode::SIMD32 == simdMode)) {
    EP.m_canAbortOnSpill = false;
  }
  bool hasSubGroupForce = hasSubGroupIntrinsicPVC(F);
  uint32_t maxPressure = getMaxPressure(F, pMdUtils);

  auto FG = m_FGA ? m_FGA->getGroup(&F) : nullptr;
  bool hasStackCall = FG && FG->hasStackCall();
  bool isIndirectGroup = FG && m_FGA->isIndirectCallGroup(FG);
  bool hasSubroutine = FG && !FG->isSingleIgnoringStackOverflowDetection() && !hasStackCall && !isIndirectGroup;
  bool forceLowestSIMDForStackCalls =
      IGC_IS_FLAG_ENABLED(ForceLowestSIMDForStackCalls) && (hasStackCall || isIndirectGroup);

  if (requiredSimdSize == 0) {
    if (maxPressure >= pCtx->m_ForceSIMDRPELimit && simdMode != SIMDMode::SIMD16) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      funcInfoMD->getSubGroupSize()->setSIMDSize(16);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
    if (hasSubroutine && simdMode != SIMDMode::SIMD16) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
    if (forceLowestSIMDForStackCalls && simdMode != SIMDMode::SIMD16) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
  }

  bool optDisable = this->GetContext()->getModuleMetaData()->compOpt.OptDisable;

  if (optDisable && requiredSimdSize == 0) // if simd size not requested in MD
  {
    if (simdMode == SIMDMode::SIMD32) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }

    // simd16 forced when all optimizations disabled due to compile time optimization
    pCtx->getModuleMetaData()->csInfo.forcedSIMDSize = (unsigned char)numLanes(SIMDMode::SIMD16);
  }

  if (simdMode == SIMDMode::SIMD32 && hasSyncRTCalls) {
      return SIMDStatus::SIMD_FUNC_FAIL;
  } else if (simdMode == SIMDMode::SIMD16 && hasSyncRTCalls) {
      return SIMDStatus::SIMD_PASS;
  }

  if (requiredSimdSize) {
    if (requiredSimdSize != numLanes(simdMode)) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
  } else {
    // Checking registry/flag here. Note that if ForceOCLSIMDWidth is set to
    // 8/16/32, only corresponding EnableOCLSIMD<N> is set to true. Therefore,
    // if any of EnableOCLSIMD<N> is disabled, ForceOCLSIMDWidth must set to
    // a value other than <N> if set. See igc_regkeys.cpp for detail.
    if ((simdMode == SIMDMode::SIMD32 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD32)) ||
        (simdMode == SIMDMode::SIMD16 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD16))) {
      return SIMDStatus::SIMD_FUNC_FAIL;
    }

    // Check if we force code generation for the current SIMD size.
    // Note that for SIMD8, we always force it!
    // ATTN: This check is redundant!
    if (numLanes(simdMode) == pCtx->getModuleMetaData()->csInfo.forcedSIMDSize) {
      return SIMDStatus::SIMD_PASS;
    }

    if (simdMode == SIMDMode::SIMD16 && (!pCtx->platform.isCoreXE2() && !pCtx->platform.isCoreXE3()) &&
        !hasSubGroupForce && !forceLowestSIMDForStackCalls && !hasSubroutine) {
      pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }

    if (simdMode == SIMDMode::SIMD32 && hasSubGroupForce) {
      pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
  }

  if (EP.m_canAbortOnSpill && pCtx->platform.isCoreXE3() && IGC_IS_FLAG_ENABLED(AllowEarlySIMD16DropForXE3)) {
    bool shouldDrop = shouldDropToSIMD16(maxPressure, simdMode, pCtx, pMdUtils, &F);
    if (shouldDrop) {
      pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
  }

  return SIMDStatus::SIMD_PASS;
}

int COpenCLKernel::getAnnotatedNumThreads() { return m_annotatedNumThreads; }

bool COpenCLKernel::IsRegularGRFRequested() { return m_regularGRFRequested; }

bool COpenCLKernel::IsLargeGRFRequested() { return m_largeGRFRequested; }

SIMDStatus COpenCLKernel::checkSIMDCompileConds(SIMDMode simdMode, EmitPass &EP, llvm::Function &F,
                                                bool hasSyncRTCalls) {
  CShader *simd8Program = m_parent->GetShader(SIMDMode::SIMD8);
  CShader *simd16Program = m_parent->GetShader(SIMDMode::SIMD16);
  CShader *simd32Program = m_parent->GetShader(SIMDMode::SIMD32);

  CodeGenContext *pCtx = GetContext();

  bool compileFunctionVariants =
      pCtx->m_enableSimdVariantCompilation && (m_FGA && IGC::isIntelSymbolTableVoidProgram(m_FGA->getGroupHead(&F)));

  // Here we see if we have compiled a size for this shader already
  if ((simd8Program && simd8Program->ProgramOutput()->m_programSize > 0) ||
      (simd16Program && simd16Program->ProgramOutput()->m_programSize > 0) ||
      (simd32Program && simd32Program->ProgramOutput()->m_programSize > 0)) {
    bool canCompileMultipleSIMD = compileFunctionVariants;
    if (!(canCompileMultipleSIMD && (pCtx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)))
      return SIMDStatus::SIMD_FUNC_FAIL;
  }

  // Next we check if there is a required sub group size specified
  MetaDataUtils *pMdUtils = EP.getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = pCtx->getModuleMetaData();
  FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(&F);
  uint32_t simd_size = getReqdSubGroupSize(F, pMdUtils);
  uint32_t maxPressure = getMaxPressure(F, pMdUtils);

  // For simd variant functions, detect which SIMD sizes are needed
  if (compileFunctionVariants && F.hasFnAttribute("variant-function-def")) {
    bool canCompile = true;
    if (simdMode == SIMDMode::SIMD16)
      canCompile = F.hasFnAttribute("CompileSIMD16");
    else if (simdMode == SIMDMode::SIMD8)
      canCompile = F.hasFnAttribute("CompileSIMD8");

    if (!canCompile) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
  }

  auto FG = m_FGA ? m_FGA->getGroup(&F) : nullptr;
  bool hasStackCall = FG && FG->hasStackCall();
  bool isIndirectGroup = FG && m_FGA->isIndirectCallGroup(FG);
  bool hasSubroutine = FG && !FG->isSingle() && !hasStackCall && !isIndirectGroup;

  // Library compilation size is forced to a specific SIMD size, don't change it
  bool forceLibSize = pCtx->getModuleMetaData()->compOpt.IsLibraryCompilation &&
                      pCtx->getModuleMetaData()->compOpt.LibraryCompileSIMDSize != 0;

  // If stack calls are present, disable simd32 in order to do CallWA in visa
  if (!forceLibSize && pCtx->platform.requireCallWA() && simdMode == SIMDMode::SIMD32) {
    bool hasNestedCall = FG && FG->hasNestedCall();
    bool hasIndirectCall = FG && FG->hasIndirectCall();
    if (hasNestedCall || hasIndirectCall || isIndirectGroup) {
      // Disable EU fusion if SIMD32 is requested
      if (getReqdSubGroupSize(F, pMdUtils) == numLanes(SIMDMode::SIMD32)) {
        pCtx->getModuleMetaData()->compOpt.DisableEUFusion = true;
        if (FG->getHead() == &F) {
          pCtx->EmitWarning(std::string("EU fusion is disabled, it does not work on the current platform if SIMD32 "
                                        "mode specified by intel_reqd_sub_group_size(32)")
                                .c_str(),
                            &F);
        }
      } else {
        pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_FUNC_FAIL;
      }
    }
  }

  if (simd_size == 0) {
    // Default to lowest SIMD mode for stack calls/indirect calls
    if (IGC_IS_FLAG_ENABLED(ForceLowestSIMDForStackCalls) && (hasStackCall || isIndirectGroup) &&
        simdMode != SIMDMode::SIMD8) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }

    if (maxPressure >= pCtx->m_ForceSIMDRPELimit && simdMode != SIMDMode::SIMD8) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      funcInfoMD->getSubGroupSize()->setSIMDSize(8);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }

    // Just subroutines and subgroup size is not set, default to SIMD8
    if (hasSubroutine && simdMode != SIMDMode::SIMD8) {
      pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
      return SIMDStatus::SIMD_FUNC_FAIL;
    }
  }

  uint32_t groupSize = 0;
  if (modMD->csInfo.maxWorkGroupSize) {
    groupSize = modMD->csInfo.maxWorkGroupSize;
  } else {
    groupSize = IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, &F);
  }

  if (groupSize == 0) {
    groupSize = IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, &F);
  }

  if (simd_size) {
    switch (simd_size) {
      // Apparently the only possible simdModes here are SIMD8, SIMD16, SIMD32
    case 8:
      if (simdMode != SIMDMode::SIMD8) {
        pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_FUNC_FAIL;
      }
      break;
    case 16:
      if (simdMode != SIMDMode::SIMD16) {
        pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_FUNC_FAIL;
      }
      EP.m_canAbortOnSpill = false;
      break;
    case 32:
      if (simdMode != SIMDMode::SIMD32) {
        pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_FUNC_FAIL;
      } else {
        if (hasSyncRTCalls) {
          return SIMDStatus::SIMD_FUNC_FAIL; // SIMD32 unsupported with raytracing calls
        } else {                             // simdMode == SIMDMode::SIMD32 && !hasSyncRTCalls
          EP.m_canAbortOnSpill = false;
        }
      }
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Unsupported required sub group size");
      break;
    }
  } else {
    // Checking registry/flag here. Note that if ForceOCLSIMDWidth is set to
    // 8/16/32, only corresponding EnableOCLSIMD<N> is set to true. Therefore,
    // if any of EnableOCLSIMD<N> is disabled, ForceOCLSIMDWidth must set to
    // a value other than <N> if set. See igc_regkeys.cpp for detail.
    if ((simdMode == SIMDMode::SIMD32 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD32)) ||
        (simdMode == SIMDMode::SIMD16 && IGC_IS_FLAG_DISABLED(EnableOCLSIMD16))) {
      return SIMDStatus::SIMD_FUNC_FAIL;
    }

    // Check if we force code generation for the current SIMD size.
    // Note that for SIMD8, we always force it!
    // ATTN: This check is redundant!
    if (numLanes(simdMode) == pCtx->getModuleMetaData()->csInfo.forcedSIMDSize || simdMode == SIMDMode::SIMD8) {
      return SIMDStatus::SIMD_PASS;
    }

    if (hasSyncRTCalls) {
      // If we get all the way to here, then set it to the preferred SIMD size for Ray Tracing.
      SIMDMode mode = SIMDMode::UNKNOWN;
      mode = m_Context->platform.getPreferredRayTracingSIMDSize();
      return (mode == simdMode) ? SIMDStatus::SIMD_PASS : SIMDStatus::SIMD_FUNC_FAIL;
    }

    if (groupSize != 0 && groupSize <= 16) {
      if (simdMode == SIMDMode::SIMD32 || (groupSize <= 8 && simdMode != SIMDMode::SIMD8)) {
        pCtx->SetSIMDInfo(SIMD_SKIP_THGRPSIZE, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_FUNC_FAIL;
      }
    }

    // Here we check profitablility, etc.
    if (simdMode == SIMDMode::SIMD16) {
      bool optDisable = this->GetContext()->getModuleMetaData()->compOpt.OptDisable;

      if (optDisable) {
        return SIMDStatus::SIMD_FUNC_FAIL;
      }

      // bail out of SIMD16 if it's not profitable.
      Simd32ProfitabilityAnalysis &PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
      if (!PA.isSimd16Profitable()) {
        pCtx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_PERF_FAIL;
      }
    }
    if (simdMode == SIMDMode::SIMD32) {
      bool optDisable = this->GetContext()->getModuleMetaData()->compOpt.OptDisable;

      if (optDisable) {
        return SIMDStatus::SIMD_FUNC_FAIL;
      }

      // bail out of SIMD32 if it's not profitable.
      Simd32ProfitabilityAnalysis &PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
      if (!PA.isSimd32Profitable()) {
        pCtx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, ShaderDispatchMode::NOT_APPLICABLE);
        return SIMDStatus::SIMD_PERF_FAIL;
      }
    }
  }

  return SIMDStatus::SIMD_PASS;
}

} // namespace IGC

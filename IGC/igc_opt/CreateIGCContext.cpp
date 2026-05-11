/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CreateIGCContext.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/API/usc.h"

#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "AdaptorOCL/DriverInfoOCL.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/CommandLine.h"
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

using namespace llvm;

static cl::opt<uint16_t> DeviceIdOption("device-id",
                                        cl::desc("Use to select specific configuration of a destination platform"),
                                        cl::values(clEnumValN(0x0BD4, "0x0BD4", "PVC-VG")), cl::init(0), cl::Hidden);

static cl::opt<uint16_t> RevIdOption("rev-id", cl::desc("Use to select specific platform revision id"),
                                     cl::values(clEnumValN(REVID::REVISION_A0, "A", "Revision A"),
                                                clEnumValN(REVID::REVISION_B, "B", "Revision B"),
                                                clEnumValN(REVID::REVISION_C, "C", "Revision C"),
                                                clEnumValN(REVID::REVISION_D, "D", "Revision D")),
                                     cl::init(0), cl::Hidden);

static cl::opt<bool> HasEmu64BitInsts("has-emulated-64-bit-insts",
                                      cl::desc("Set info about containing emulated 64 bit insts."), cl::init(false),
                                      cl::Hidden);

static cl::opt<ShaderType>
    ShaderTypeOption(cl::desc("Specify the type of shader being compiled:"),
                     cl::values(clEnumValN(ShaderType::VERTEX_SHADER, "inputvs", "Vertex Shader"),
                                clEnumValN(ShaderType::HULL_SHADER, "inpuths", "Hull Shader"),
                                clEnumValN(ShaderType::DOMAIN_SHADER, "inputds", "Domain Shader"),
                                clEnumValN(ShaderType::GEOMETRY_SHADER, "inputgs", "Geometry Shader"),
                                clEnumValN(ShaderType::TASK_SHADER, "inputtask", "Task Shader"),
                                clEnumValN(ShaderType::MESH_SHADER, "inputmesh", "Mesh Shader"),
                                clEnumValN(ShaderType::PIXEL_SHADER, "inputps", "Pixel Shader"),
                                clEnumValN(ShaderType::COMPUTE_SHADER, "inputcs", "Compute Shader"),
                                clEnumValN(ShaderType::OPENCL_SHADER, "inputocl", "OpenCL Shader"),
                                clEnumValN(ShaderType::RAYTRACING_SHADER, "inputrt", "RayTracing Shader")),
                     cl::init(ShaderType::OPENCL_SHADER));

static cl::opt<PRODUCT_FAMILY>
    PlatformOption(cl::desc("Specify destination platform: "),
                   cl::values(clEnumValN(PRODUCT_FAMILY::IGFX_TIGERLAKE_LP, "platformtgllp", "TGLLP"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_ALDERLAKE_S, "platformadls", "ADLS"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_ALDERLAKE_P, "platformadlp", "ADLP"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_ALDERLAKE_N, "platformadln", "ADLN"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_XE_HP_SDV, "platformxehpsdv", "XEHPSDV"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_DG1, "platformdg1", "DG1"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_DG2, "platformdg2", "DG2"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_PVC, "platformpvc", "PVC"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_METEORLAKE, "platformmtl", "MTL"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_ARROWLAKE, "platformarl", "ARL"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_LUNARLAKE, "platformlnl", "LNL"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_BMG, "platformbmg", "BMG"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_PTL, "platformPtl", "PTL"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_NVL_XE3G, "platformNvl_Xe3g", "NVL_XE3G"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_NVL, "platformNvl", "NVL"),
                              clEnumValN(PRODUCT_FAMILY::IGFX_CRI, "platformCri", "CRI")
                                  ),
                   cl::init(PRODUCT_FAMILY::IGFX_TIGERLAKE_LP));

static cl::opt<ClientApi>
    ClientApiOption(cl::desc("Specify Client API: "),
                    cl::values(
                        clEnumValN(ClientApi::OCL, "ocl", "OclCommon"), clEnumValN(ClientApi::NEO, "neo", "Neo-ocl")

                            ),
                    cl::init(ClientApi::Unspecified));

IGC::CodeGenContext *CreateCodeGenContext() {
  const PRODUCT_FAMILY productFamily = PlatformOption;
  const ClientApi api = ClientApiOption;
  const ShaderType shaderType = ShaderTypeOption;
  const uint16_t deviceId = DeviceIdOption;

  static PLATFORM platform = {};
  static std::once_flag initFlag;
  std::call_once(initFlag, [&]() {
    platform.usDeviceID = deviceId;
    platform.eProductFamily = productFamily;
    if (RevIdOption) {
      platform.usRevId = RevIdOption;
    }
    switch (platform.eProductFamily) {
    case PRODUCT_FAMILY::IGFX_TIGERLAKE_LP:
    case PRODUCT_FAMILY::IGFX_ALDERLAKE_S:
    case PRODUCT_FAMILY::IGFX_ALDERLAKE_P:
    case PRODUCT_FAMILY::IGFX_ALDERLAKE_N:
    case PRODUCT_FAMILY::IGFX_DG1:
      platform.eRenderCoreFamily = IGFX_GEN12_CORE;
      break;
    case PRODUCT_FAMILY::IGFX_XE_HP_SDV:
      platform.eRenderCoreFamily = IGFX_XE_HP_CORE;
      break;
    case PRODUCT_FAMILY::IGFX_DG2:
      platform.eRenderCoreFamily = IGFX_XE_HPG_CORE;
      // enable LSC functionality
      platform.usRevId = static_cast<unsigned short>(ACM_G10_GT_REV_ID_B0);
      break;
    case PRODUCT_FAMILY::IGFX_METEORLAKE:
    case PRODUCT_FAMILY::IGFX_ARROWLAKE:
      platform.eRenderCoreFamily = IGFX_XE_HPG_CORE;
      break;
    case PRODUCT_FAMILY::IGFX_BMG:
    case PRODUCT_FAMILY::IGFX_LUNARLAKE:
      platform.eRenderCoreFamily = IGFX_XE2_HPG_CORE;
      break;
    case PRODUCT_FAMILY::IGFX_PVC:
      platform.eRenderCoreFamily = IGFX_XE_HPC_CORE;
      break;
    case PRODUCT_FAMILY::IGFX_PTL:
    case PRODUCT_FAMILY::IGFX_NVL_XE3G:
      platform.eRenderCoreFamily = IGFX_XE3_CORE;
      break;
    case PRODUCT_FAMILY::IGFX_NVL:
    case PRODUCT_FAMILY::IGFX_CRI:
      platform.eRenderCoreFamily = IGFX_XE3P_CORE;
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Unknown product family.");
      break;
    }
  });
  IGC_ASSERT(platform.eProductFamily == productFamily);
  static const IGC::CBTILayout btiLayout(USC::g_cZeroBindingTableLayout.m_Layout);
  static IGC::CPlatform igcPlatform = IGC::CPlatform(platform);

  GT_SYSTEM_INFO gtInfo = {};
  gtInfo.MaxSubSlicesSupported = 1;
  gtInfo.MaxSlicesSupported = 1;
  gtInfo.MaxEuPerSubSlice = 1;
  gtInfo.MaxDualSubSlicesSupported = 1;
  gtInfo.DualSubSliceCount = 1;
  gtInfo.EUCount = 16;
  gtInfo.ThreadCount = 128;
  igcPlatform.SetGTSystemInfo(gtInfo);

  static IGC::CDriverInfo driverInfo;
  const IGC::CDriverInfo *pDriverInfo = &driverInfo;

  switch (api) {
  case ClientApi::OCL: {
    static const TC::CDriverInfoOCLCommon oclDriverInfo;
    pDriverInfo = &oclDriverInfo;
    break;
  }
  case ClientApi::NEO: {
    static const TC::CDriverInfoOCLNEO oclNEODriverInfo;
    pDriverInfo = &oclNEODriverInfo;
    break;
  }
  default:
    break;
  }

  IGC::CodeGenContext *pCtx = nullptr;
  switch (shaderType) {
  case ShaderType::OPENCL_SHADER: {
    static USC::SShaderStageBTLayout zeroLayout = USC::g_cZeroShaderStageBTLayout;
    static const IGC::COCLBTILayout oclLayout(&zeroLayout);
    pCtx = new IGC::OpenCLProgramContext(oclLayout, igcPlatform,
                                         nullptr, // TC::STB_TranslateInputArgs* pInputArgs
                                         *pDriverInfo);
    break;
  }
  default:
    IGC_ASSERT(false && "Unsupported shader stage");
  }

  if (HasEmu64BitInsts)
    pCtx->m_hasEmu64BitInsts = true;

  return pCtx;
}

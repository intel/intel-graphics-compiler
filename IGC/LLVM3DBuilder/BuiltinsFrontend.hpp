/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BUILTINSFRONTEND_H_
#define _BUILTINSFRONTEND_H_

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallVector.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/AsmParser/Parser.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "inc/common/Compiler/API/SurfaceFormats.h"
#include "inc/common/igfxfmid.h"
#include "common/IGCIRBuilder.h"
#include "common/EmUtils.h"
#include "skuwa/iacm_g10_rev_id.h"
#include "skuwa/iacm_g11_rev_id.h"
#include "skuwa/iacm_g12_rev_id.h"
#include "IGC/common/ResourceAddrSpace.h"

namespace llvm {
class GenIntrinsicInst;
}

struct genplatform {
private:
  const PLATFORM *m_platformInfo;

public:
  genplatform(const PLATFORM *platform) { m_platformInfo = platform; }
  genplatform() {}
  GFXCORE_FAMILY GetPlatformFamily() const { return m_platformInfo->eRenderCoreFamily; }
  bool hasHDCSupportForTypedReads() const { return m_platformInfo->eRenderCoreFamily >= IGFX_GEN10_CORE; }
  bool hasHDCSupportForTypedReadsUnormSnormToFloatConversion() const;
  bool hasSupportForAllOCLImageFormats() const;
};

inline bool genplatform::hasHDCSupportForTypedReadsUnormSnormToFloatConversion() const {
  return m_platformInfo->eRenderCoreFamily >= IGFX_GEN10_CORE;
}

inline bool genplatform::hasSupportForAllOCLImageFormats() const {
  bool isDG2B0Plus = SI_WA_FROM(m_platformInfo->usRevId, ACM_G10_GT_REV_ID_B0);
  bool isDG2C0Plus = SI_WA_FROM(m_platformInfo->usRevId, ACM_G10_GT_REV_ID_C0);
  bool isDG2G11EUConfig = GFX_IS_DG2_G11_CONFIG(m_platformInfo->usDeviceID);
  [[maybe_unused]] bool isDG2G12EUConfig = GFX_IS_DG2_G12_CONFIG(m_platformInfo->usDeviceID);
  if ((m_platformInfo->eProductFamily == IGFX_DG2 && isDG2C0Plus) ||
      (m_platformInfo->eProductFamily == IGFX_DG2 && isDG2G11EUConfig && isDG2B0Plus) ||
      (m_platformInfo->eProductFamily == IGFX_METEORLAKE))
  {
    return true;
  }
  return m_platformInfo->eRenderCoreFamily >= IGFX_XE_HPC_CORE;
}

class SampleParamsFromCube {
public:
  llvm::Value *float_xcube;
  llvm::Value *float_ycube;
  llvm::Value *float_aicube;
  llvm::Value *float_address_3;
  llvm::Value *int32_textureIdx;
  llvm::Value *int32_sampler;
  llvm::Value *offsetU;
  llvm::Value *offsetV;
  llvm::Value *offsetR;

  llvm::Value *get_float_xcube() { return float_xcube; }
  llvm::Value *get_float_ycube() { return float_ycube; }
  llvm::Value *get_float_aicube() { return float_aicube; }
  llvm::Value *get_float_address_3() { return float_address_3; }
  llvm::Value *get_int32_textureIdx() { return int32_textureIdx; }
  llvm::Value *get_int32_sampler() { return int32_sampler; }
  llvm::Value *get_offsetU() { return offsetU; }
  llvm::Value *get_offsetV() { return offsetV; }
  llvm::Value *get_offsetR() { return offsetR; }

  void operator=(const SampleParamsFromCube &CUBE_params) {
    float_xcube = CUBE_params.float_xcube;
    float_ycube = CUBE_params.float_ycube;
    float_aicube = CUBE_params.float_aicube;
    float_address_3 = CUBE_params.float_address_3;
    int32_textureIdx = CUBE_params.int32_textureIdx;
    int32_sampler = CUBE_params.int32_sampler;
    offsetU = CUBE_params.offsetU;
    offsetV = CUBE_params.offsetV;
    offsetR = CUBE_params.offsetR;
  }
};

class SampleD_DC_FromCubeParams {
public:
  llvm::Value *float_src_u;
  llvm::Value *dxu;
  llvm::Value *dyu;
  llvm::Value *float_src_v;
  llvm::Value *dxv;
  llvm::Value *dyv;
  llvm::Value *float_src_r;
  llvm::Value *dxr;
  llvm::Value *dyr;
  llvm::Value *float_src_ai;
  llvm::Value *int32_pairedTextureIdx;
  llvm::Value *int32_textureIdx;
  llvm::Value *int32_sampler;
  llvm::Value *int32_offsetU;
  llvm::Value *int32_offsetV;
  llvm::Value *int32_offsetW;

  llvm::Value *get_float_src_u() { return float_src_u; }
  llvm::Value *get_dxu() { return dxu; }
  llvm::Value *get_dyu() { return dyu; }
  llvm::Value *get_float_src_v() { return float_src_v; }
  llvm::Value *get_dxv() { return dxv; }
  llvm::Value *get_dyv() { return dyv; }
  llvm::Value *get_float_src_r() { return float_src_r; }
  llvm::Value *get_dxr() { return dxr; }
  llvm::Value *get_dyr() { return dyr; }
  llvm::Value *get_float_src_ai() { return float_src_ai; }
  llvm::Value *get_int32_pairedTextureIdx() { return int32_pairedTextureIdx; }
  llvm::Value *get_int32_textureIdx() { return int32_textureIdx; }
  llvm::Value *get_int32_sampler() { return int32_sampler; }
  llvm::Value *get_int32_offsetU() { return int32_offsetU; }
  llvm::Value *get_int32_offsetV() { return int32_offsetV; }
  llvm::Value *get_int32_offsetW() { return int32_offsetW; }

  void operator=(const SampleD_DC_FromCubeParams &D_DC_CUBE_params) {
    float_src_u = D_DC_CUBE_params.float_src_u;
    dxu = D_DC_CUBE_params.dxu;
    dyu = D_DC_CUBE_params.dyu;
    float_src_v = D_DC_CUBE_params.float_src_v;
    dxv = D_DC_CUBE_params.dxv;
    dyv = D_DC_CUBE_params.dyv;
    float_src_r = D_DC_CUBE_params.float_src_r;
    dxr = D_DC_CUBE_params.dxr;
    dyr = D_DC_CUBE_params.dyr;
    float_src_ai = D_DC_CUBE_params.float_src_ai;
    int32_pairedTextureIdx = D_DC_CUBE_params.int32_pairedTextureIdx;
    int32_textureIdx = D_DC_CUBE_params.int32_textureIdx;
    int32_sampler = D_DC_CUBE_params.int32_sampler;
    int32_offsetU = D_DC_CUBE_params.int32_offsetU;
    int32_offsetV = D_DC_CUBE_params.int32_offsetV;
    int32_offsetW = D_DC_CUBE_params.int32_offsetW;
  }
};

template <typename T = llvm::ConstantFolder, typename InserterTy = llvm::IRBuilderDefaultInserter>
class LLVM3DBuilder : public llvm::IGCIRBuilder<T, InserterTy> {
public:
  LLVM3DBuilder(llvm::LLVMContext &pCtx, const PLATFORM &pPlatform)
      : llvm::IGCIRBuilder<T, InserterTy>(pCtx), m_Platform(new genplatform(&pPlatform)) {
    Init();
  }

  ~LLVM3DBuilder() {
    if (m_Platform) {
      delete m_Platform;
      m_Platform = NULL;
    }
  }

  LLVM3DBuilder(const LLVM3DBuilder &) = delete;
  LLVM3DBuilder &operator=(const LLVM3DBuilder &) = delete;

  static unsigned EncodeASForGFXResource(const llvm::Value &bufIdx, IGC::BufferType bufType, unsigned uniqueIndAS,
                                         IGC::ResourceDimType resourceDimTypeId = std::nullopt);

  llvm::Value *CreateFAbs(llvm::Value *V);
  llvm::Value *CreateFSat(llvm::Value *V);
  llvm::Value *CreateSqrt(llvm::Value *V);
  llvm::Value *CreateDiscard(llvm::Value *V);
  llvm::Value *CreateFLog(llvm::Value *V);
  llvm::Value *CreateFExp(llvm::Value *V);
  llvm::Value *CreateFloor(llvm::Value *V);
  llvm::Value *CreateCeil(llvm::Value *V);
  llvm::Value *CreateRoundZ(llvm::Value *V);
  llvm::Value *CreateRoundNE(llvm::Value *V);
  llvm::Value *CreateFPow(llvm::Value *LHS, llvm::Value *RHS);
  llvm::Value *CreateFMax(llvm::Value *LHS, llvm::Value *RHS);
  llvm::Value *CreateFMin(llvm::Value *LHS, llvm::Value *RHS);
  llvm::Value *CreateIMulH(llvm::Value *LHS, llvm::Value *RHS);
  llvm::Value *CreateUMulH(llvm::Value *LHS, llvm::Value *RHS);
  llvm::Value *CreateDeriveRTX(llvm::Value *V);
  llvm::Value *CreateDeriveRTX_Fine(llvm::Value *V);
  llvm::Value *CreateDeriveRTY(llvm::Value *V);
  llvm::Value *CreateDeriveRTY_Fine(llvm::Value *V);
  llvm::Value *CreateSin(llvm::Value *V);
  llvm::Value *CreateCos(llvm::Value *V);
  llvm::Value *CreateIsNan(llvm::Value *V);
  llvm::Value *CreateCtpop(llvm::Value *V);
  llvm::Value *CreateFrc(llvm::Value *V);

  llvm::Value *CreateCPSRqstCoarseSize(llvm::Value *pSrcVal);
  llvm::Value *CreateCPSActualCoarseSize(llvm::Value *pSrcVal);

  llvm::Value *getHalf(float f);
  llvm::Value *getFloat(float f);
  llvm::Value *getDouble(double d);
  llvm::Value *CreatePow(llvm::Value *src0, llvm::Value *src1);

  llvm::Value *Create_MAD_Scalar(llvm::Value *float_src0, llvm::Value *float_src1, llvm::Value *float_src2);

  llvm::Value *Create_resinfo(llvm::Value *int32_src_s_mip, llvm::Value *int32_textureIdx);

  llvm::Value *Create_resinfoptr_msaa(llvm::Value *srcBuffer, llvm::Value *float_src_s_mip);

  llvm::Value *CreateReadSurfaceTypeAndFormat(llvm::Value *resource);

  llvm::Value *Create_typedwrite(llvm::Value *dstBuffer, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                 llvm::Value *srcAddressW, llvm::Value *lod, llvm::Value *float_X, llvm::Value *float_Y,
                                 llvm::Value *float_Z, llvm::Value *float_W);

  llvm::Value *Create_typedread(llvm::Value *dstBuffer, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                llvm::Value *srcAddressW, llvm::Value *lod);

  llvm::Value *Create_typedwriteMS(llvm::Value *dstBuffer, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                   llvm::Value *srcAddressW, llvm::Value *sampleIdx, llvm::Value *float_X,
                                   llvm::Value *float_Y, llvm::Value *float_Z, llvm::Value *float_W);

  llvm::Value *Create_typedreadMS(llvm::Value *dstBuffer, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                  llvm::Value *srcAddressW, llvm::Value *sampleIdx);

  llvm::Value *Create_typedread_msaa2D(llvm::Value *srcBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU,
                                       llvm::Value *srcAddressV, llvm::Value *lod);

  llvm::Value *Create_typedread_msaa2DArray(llvm::Value *srcBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU,
                                            llvm::Value *srcAddressV, llvm::Value *srcAddressR, llvm::Value *lod);

  llvm::Value *Create_typedwrite_msaa2D(llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU,
                                        llvm::Value *srcAddressV, llvm::Value *float_X, llvm::Value *float_Y,
                                        llvm::Value *float_Z, llvm::Value *float_W);

  llvm::Value *Create_typedwrite_msaa2DArray(llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU,
                                             llvm::Value *srcAddressV, llvm::Value *srcAddressR, llvm::Value *float_X,
                                             llvm::Value *float_Y, llvm::Value *float_Z, llvm::Value *float_W);

  llvm::Value *Create_dwordatomictypedMsaa2D(llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU,
                                             llvm::Value *srcAddressV, llvm::Value *src, llvm::Value *instType);

  llvm::Value *Create_dwordatomictypedMsaa2DArray(llvm::Value *dstBuffer, llvm::Value *sampleIdx,
                                                  llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                                  llvm::Value *srcAddressR, llvm::Value *src, llvm::Value *instType);

  llvm::Value *Create_cmpxchgatomictypedMsaa2D(llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU,
                                               llvm::Value *srcAddressV, llvm::Value *src0, llvm::Value *src1);

  llvm::Value *Create_cmpxchgatomictypedMsaa2DArray(llvm::Value *dstBuffer, llvm::Value *sampleIdx,
                                                    llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                                    llvm::Value *srcAddressR, llvm::Value *src0, llvm::Value *src1);

  llvm::Value *Create_StatelessAtomic(llvm::Value *ptr, llvm::Value *data, IGC::AtomicOp opcode);

  llvm::Value *Create_InidrectAtomic(llvm::Value *resource, llvm::Value *offset, llvm::Value *data,
                                     IGC::AtomicOp opcode);

  llvm::Value *Create_StatelessAtomicCmpXChg(llvm::Value *ptr, llvm::Value *data0, llvm::Value *data1);

  llvm::Value *Create_InidrectAtomicCmpXChg(llvm::Value *resource, llvm::Value *offset, llvm::Value *data0,
                                            llvm::Value *data1);

  llvm::Value *Create_TypedAtomic(llvm::Value *resource, llvm::Value *addressU, llvm::Value *addressV,
                                  llvm::Value *addressR, llvm::Value *data, IGC::AtomicOp opcode);

  llvm::Value *Create_TypedAtomicCmpXChg(llvm::Value *resource, llvm::Value *addressU, llvm::Value *addressV,
                                         llvm::Value *addressR, llvm::Value *data0, llvm::Value *data1);

  llvm::CallInst *Create_SampleInfo(llvm::Value *int32_resourceIdx);

  llvm::Value *Create_SamplePos(llvm::Value *int32_resourceIdx, llvm::Value *int32_samplerIdx);

  llvm::Value *Create_SyncThreadGroup();
  llvm::Value *Create_FlushSampler();
  llvm::Value *Create_LscFence(llvm::Value *SFID, llvm::Value *FenceOp, llvm::Value *Scope);
  llvm::Value *Create_MemoryFence(bool commit, bool flushRWDataCache, bool flushConstantCache, bool flushTextureCache,
                                  bool flushInstructionCache, bool globalFence);
  llvm::Value *Create_GlobalSync();

  llvm::CallInst *Create_SAMPLE(llvm::Value *coordinate_u, llvm::Value *coordinate_v, llvm::Value *coordinate_r,
                                llvm::Value *coordinate_ai, llvm::Value *ptr_textureIdx, llvm::Value *ptr_sampler,
                                llvm::Value *offsetU, llvm::Value *offsetV, llvm::Value *offsetW, llvm::Value *minlod,
                                bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLE(llvm::Value *coordinate_u, llvm::Value *coordinate_v, llvm::Value *coordinate_r,
                                llvm::Value *coordinate_ai, llvm::Value *ptr_pairedTextureIdx,
                                llvm::Value *ptr_textureIdx, llvm::Value *ptr_sampler, llvm::Value *offsetU,
                                llvm::Value *offsetV, llvm::Value *offsetW, llvm::Value *minlod,
                                bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  SampleParamsFromCube
  Prepare_SAMPLE_Cube_ParamsFromUnormalizedCoords(llvm::Value *int32_lod, llvm::Value *int32_textureIdx,
                                                  llvm::Value *int32_u, llvm::Value *int32_v, llvm::Value *int32_faceid,
                                                  llvm::Value *int32_cube_array_index, llvm::Value *float_array_6_3,
                                                  llvm::Value *int32_sampler);

  SampleParamsFromCube Prepare_SAMPLE_Cube_Params(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                                  llvm::Value *float_address_2, llvm::Value *float_address_3,
                                                  llvm::Value *int32_textureIdx, llvm::Value *int32_sampler);

  llvm::CallInst *Create_SAMPLED(SampleD_DC_FromCubeParams &sampleParams, llvm::Value *minlod = nullptr,
                                 bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *
  Create_SAMPLED(llvm::Value *float_src1_s_chan0, llvm::Value *float_src1_s_chan1, llvm::Value *float_src1_s_chan2,
                 llvm::Value *float_src2_s_chan0, llvm::Value *float_src2_s_chan1, llvm::Value *float_src2_s_chan2,
                 llvm::Value *float_src3_s_chan0, llvm::Value *float_src3_s_chan1, llvm::Value *float_src3_s_chan2,
                 llvm::Value *float_src1_s_chan3, llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
                 llvm::Value *int32_offsetU_358, llvm::Value *int32_offsetV_359, llvm::Value *int32_offsetW_359,
                 llvm::Value *minlod, bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLED(llvm::Value *float_src1_s_chan0, llvm::Value *float_src1_s_chan1,
                                 llvm::Value *float_src1_s_chan2, llvm::Value *float_src2_s_chan0,
                                 llvm::Value *float_src2_s_chan1, llvm::Value *float_src2_s_chan2,
                                 llvm::Value *float_src3_s_chan0, llvm::Value *float_src3_s_chan1,
                                 llvm::Value *float_src3_s_chan2, llvm::Value *float_src1_s_chan3,
                                 llvm::Value *int32_pairedTextureIdx_356, llvm::Value *int32_textureIdx_356,
                                 llvm::Value *int32_sampler_357, llvm::Value *int32_offsetU_358,
                                 llvm::Value *int32_offsetV_359, llvm::Value *int32_offsetW_359, llvm::Value *minlod,
                                 bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEDC(llvm::Value *float_ref, llvm::Value *float_src_u, llvm::Value *dxu, llvm::Value *dyu,
                                  llvm::Value *float_src_v, llvm::Value *dxv, llvm::Value *dyv,
                                  llvm::Value *float_src_r, llvm::Value *dxr, llvm::Value *dyr,
                                  llvm::Value *float_src_ai, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
                                  llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
                                  bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEDC(llvm::Value *float_ref, llvm::Value *float_src_u, llvm::Value *dxu, llvm::Value *dyu,
                                  llvm::Value *float_src_v, llvm::Value *dxv, llvm::Value *dyv,
                                  llvm::Value *float_src_r, llvm::Value *dxr, llvm::Value *dyr,
                                  llvm::Value *float_src_ai, llvm::Value *int32_pairedTextureIdx,
                                  llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                  llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                  llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEDCMlod(llvm::Value *float_ref, llvm::Value *float_src_u, llvm::Value *dxu,
                                      llvm::Value *dyu, llvm::Value *float_src_v, llvm::Value *dxv, llvm::Value *dyv,
                                      llvm::Value *float_src_r, llvm::Value *dxr, llvm::Value *dyr,
                                      llvm::Value *float_src_ai, llvm::Value *minlod,
                                      llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
                                      llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                      llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
                                      bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  SampleD_DC_FromCubeParams Prepare_SAMPLE_D_DC_Cube_Params(SampleD_DC_FromCubeParams &params);

  SampleD_DC_FromCubeParams Prepare_SAMPLE_D_DC_Cube_Params(
      llvm::Value *float_src_x, llvm::Value *float_src_y, llvm::Value *float_src_z, llvm::Value *float_src_ai,
      llvm::Value *dxu, llvm::Value *dyu, llvm::Value *dzu, llvm::Value *dxv, llvm::Value *dyv, llvm::Value *dzv,
      llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357, llvm::Value *int32_offsetU_358,
      llvm::Value *int32_offsetV_359, llvm::Value *int32_offsetW_359);

  SampleD_DC_FromCubeParams Prepare_SAMPLE_D_DC_Cube_Params(
      llvm::Value *float_src_x, llvm::Value *float_src_y, llvm::Value *float_src_z, llvm::Value *float_src_ai,
      llvm::Value *dxu, llvm::Value *dyu, llvm::Value *dzu, llvm::Value *dxv, llvm::Value *dyv, llvm::Value *dzv,
      llvm::Value *int32_pairedTextureIdx_356, llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
      llvm::Value *int32_offsetU_358, llvm::Value *int32_offsetV_359, llvm::Value *int32_offsetW_359);

  llvm::CallInst *Create_SAMPLEB(llvm::Value *float_bias_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
                                 llvm::Value *float_address_2, llvm::Value *float_address_3,
                                 llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                 llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, llvm::Value *minlod,
                                 bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEB(llvm::Value *float_bias_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
                                 llvm::Value *float_address_2, llvm::Value *float_address_3,
                                 llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
                                 llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                 llvm::Value *int32_offsetW, llvm::Value *minlod, bool feedback_enabled = false,
                                 llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEL(llvm::Value *float_lod_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
                                 llvm::Value *float_address_2, llvm::Value *float_address_3,
                                 llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                 llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                 llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEL(llvm::Value *float_lod_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
                                 llvm::Value *float_address_2, llvm::Value *float_address_3,
                                 llvm::Value *int32_textureIdx, llvm::Value *int32_pairedTextureIdx,
                                 llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                 llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                 llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEC(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                 llvm::Value *float_address_1, llvm::Value *float_address_2,
                                 llvm::Value *float_address_3, llvm::Value *int32_textureIdx,
                                 llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                 llvm::Value *int32_offsetR, llvm::Value *minlod, bool feedback_enabled = false,
                                 llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEC(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                 llvm::Value *float_address_1, llvm::Value *float_address_2,
                                 llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx,
                                 llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                 llvm::Value *int32_offsetV, llvm::Value *int32_offsetR, llvm::Value *minlod,
                                 bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLELC(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                  llvm::Value *float_address_1, llvm::Value *float_address_2,
                                  llvm::Value *float_address_3, llvm::Value *float_lod, llvm::Value *int32_textureIdx,
                                  llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                  llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                  llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLELC(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                  llvm::Value *float_address_1, llvm::Value *float_address_2,
                                  llvm::Value *float_address_3, llvm::Value *float_lod,
                                  llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
                                  llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                  llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                  llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEC_LZ(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                    llvm::Value *float_address_1, llvm::Value *float_address_2,
                                    llvm::Value *float_address_3, llvm::Value *int32_textureIdx,
                                    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                    llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                    llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEC_LZ(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                    llvm::Value *float_address_1, llvm::Value *float_address_2,
                                    llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx,
                                    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
                                    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
                                    bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_lod(llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
                             llvm::Value *float_address_3, llvm::Value *int32_textureIdx_356,
                             llvm::Value *int32_sampler_357, bool feedback_enabled = false,
                             llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                 llvm::Value *float_address_2, llvm::Value *float_address_3,
                                 llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
                                 llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
                                 llvm::Value *int32_srcChannel, bool feedback_enabled = false,
                                 llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                 llvm::Value *float_address_2, llvm::Value *float_address_3,
                                 llvm::Value *int32_pairedTextureIdx_356, llvm::Value *int32_textureIdx_356,
                                 llvm::Value *int32_sampler_357, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                 llvm::Value *int32_offsetW, llvm::Value *int32_srcChannel,
                                 bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_load(llvm::Value *int32_sampleIdxU, llvm::Value *int32_sampleIdxV,
                              llvm::Value *int32_sampleIdxR, llvm::Value *int32_lod, llvm::Value *ptr_textureIdx,
                              llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetR,
                              bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_load(llvm::Value *int32_sampleIdxU, llvm::Value *int32_sampleIdxV,
                              llvm::Value *int32_sampleIdxR, llvm::Value *int32_lod, llvm::Value *ptr_pairedTextureIdx,
                              llvm::Value *ptr_textureIdx, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                              llvm::Value *int32_offsetR, bool feedback_enabled = false,
                              llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_ldms(llvm::Value *int32_srcIdxU, llvm::Value *int32_srcIdxV, llvm::Value *int32_srcIdxR,
                              llvm::Value *int32_sampleIdx, llvm::Value *int32_textureIdx, llvm::Value *int32_offsetU,
                              llvm::Value *int32_offsetV, llvm::Value *int32_offsetR, bool feedback_enabled = false,
                              llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4C(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                  llvm::Value *float_address_1, llvm::Value *float_address_2,
                                  llvm::Value *float_address_3, llvm::Value *int32_textureIdx,
                                  llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                  llvm::Value *int32_srcChannel, bool feedback_enabled = false,
                                  llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4C(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                  llvm::Value *float_address_1, llvm::Value *float_address_2,
                                  llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx,
                                  llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                  llvm::Value *int32_offsetV, llvm::Value *int32_srcChannel,
                                  bool feedback_enabled = false, llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4PO(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                   llvm::Value *float_address_2, llvm::Value *float_src_offset_0,
                                   llvm::Value *float_src_offset_1, llvm::Value *int32_textureIdx,
                                   llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                   llvm::Value *int32_srcChannel, bool feedback_enabled = false,
                                   llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4PO(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                   llvm::Value *float_address_2, llvm::Value *float_src_offset_0,
                                   llvm::Value *float_src_offset_1, llvm::Value *int32_pairedTextureIdx,
                                   llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
                                   llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                   llvm::Value *int32_srcChannel, bool feedback_enabled = false,
                                   llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4POC(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                    llvm::Value *float_address_2, llvm::Value *int_src_offset_0,
                                    llvm::Value *int_src_offset_1, llvm::Value *float_src_reference_0,
                                    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
                                    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                    llvm::Value *int32_srcChannel, bool feedback_enabled = false,
                                    llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_gather4POC(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                    llvm::Value *float_address_2, llvm::Value *int_src_offset_0,
                                    llvm::Value *int_src_offset_1, llvm::Value *float_src_reference_0,
                                    llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
                                    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                    llvm::Value *int32_srcChannel, bool feedback_enabled = false,
                                    llvm::Type *returnType = nullptr);

  llvm::Value *Create_gather4PositionOffsets(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                             llvm::Value *float_address_2,
                                             llvm::ArrayRef<llvm::Value *> int_src_offsets,
                                             llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
                                             llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                             llvm::Value *int32_srcChannel, bool feedback_enabled = false);

  llvm::Value *Create_gather4PositionOffsetsC(llvm::Value *float_reference_0, llvm::Value *float_address_0,
                                              llvm::Value *float_address_1, llvm::Value *float_address_2,
                                              llvm::ArrayRef<llvm::Value *> int_src_offsets,
                                              llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
                                              llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                              llvm::Value *int32_srcChannel, bool feedback_enabled = false);

  llvm::CallInst *Create_SAMPLEBC(llvm::Value *float_ref_value, llvm::Value *bias_value, llvm::Value *address_u,
                                  llvm::Value *address_v, llvm::Value *address_r, llvm::Value *address_ai,
                                  llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
                                  llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                  llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEBC(llvm::Value *float_ref_value, llvm::Value *bias_value, llvm::Value *address_u,
                                  llvm::Value *address_v, llvm::Value *address_r, llvm::Value *address_ai,
                                  llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
                                  llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                  llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                  llvm::Type *returnType = nullptr);

  llvm::CallInst *Create_SAMPLEBCMlod(llvm::Value *float_ref_value, llvm::Value *bias_value, llvm::Value *address_u,
                                      llvm::Value *address_v, llvm::Value *address_r, llvm::Value *address_ai,
                                      llvm::Value *minlod, llvm::Value *int32_pairedTextureIdx,
                                      llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
                                      llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                      llvm::Value *int32_offsetW, bool feedback_enabled = false,
                                      llvm::Type *returnType = nullptr);

  llvm::Value *create_indirectLoad(llvm::Value *srcBuffer, llvm::Value *offset, llvm::Value *alignment,
                                   llvm::Type *returnType, bool isVolatile = false);

  llvm::Value *create_indirectStore(llvm::Value *srcBuffer, llvm::Value *offset, llvm::Value *data,
                                    bool isVolatile = false);

  llvm::Value *create_atomicCounterIncrement(llvm::Value *srcBuffer);
  llvm::Value *create_atomicCounterDecrement(llvm::Value *srcBuffer);

  llvm::Value *createThreadLocalId(unsigned int dim);
  llvm::Value *createGroupId(unsigned int dim);

  llvm::Value *CreateF16TOF32(llvm::Value *f16_src);

  IGC::SURFACE_FORMAT GetSubstituteImageFormat(const IGC::SURFACE_FORMAT format) const;

  llvm::Value *CreateImageDataConversion(IGC::SURFACE_FORMAT format, llvm::Value *data);

  llvm::Value *Create_UBFE(llvm::Value *int32_width, llvm::Value *int32_offset, llvm::Value *int32_source);

  llvm::Value *Create_IBFE(llvm::Value *int32_width, llvm::Value *int32_offset, llvm::Value *int32_source);

  llvm::Value *Create_BFI(llvm::Value *int32_width, llvm::Value *int32_offset, llvm::Value *int32_source,
                          llvm::Value *int32_replace);

  llvm::Value *Create_BFREV(llvm::Value *int32_source);

  llvm::Value *Create_FirstBitHi(llvm::Value *int32_source);

  llvm::Value *Create_FirstBitLo(llvm::Value *int32_source);

  llvm::Value *Create_FirstBitShi(llvm::Value *int32_source);

  llvm::Value *CreateEvalSampleIndex(llvm::Value *inputIndex, llvm::Value *sampleIndex, llvm::Value *perspective);

  llvm::Value *CreateEvalSnapped(llvm::Value *inputIndex, llvm::Value *xOffset, llvm::Value *yOffset,
                                 llvm::Value *perspective);

  llvm::Value *CreateSetStream(llvm::Value *StreamId, llvm::Value *emitCount);
  llvm::Value *CreateEndPrimitive(llvm::Value *emitCount);

  llvm::Value *CreateControlPointId();
  llvm::Value *CreatePrimitiveID();
  llvm::Value *CreateInstanceID();
  llvm::Value *CreateCoverage();
  llvm::Value *CreateSampleIndex();
  llvm::Value *CreateStartVertexLocation();
  llvm::Value *CreateStartInstanceLocation();
  llvm::Value *CreateDomainPointInput(unsigned int dimension);
  llvm::Value *create_inputVecF32(llvm::Value *inputIndex, llvm::Value *interpolationMode);
  llvm::Value *create_uavSerializeAll();
  llvm::Value *create_discard(llvm::Value *condition);
  llvm::Value *create_runtime(llvm::Value *offset);
  llvm::CallInst *create_countbits(llvm::Value *src);
  llvm::Value *create_waveBallot(llvm::Value *src, llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveInverseBallot(llvm::Value *src, llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveshuffleIndex(llvm::Value *src, llvm::Value *index, llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveAll(llvm::Value *src, llvm::Value *type, llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_wavePrefix(llvm::Value *src, llvm::Value *type, bool inclusive, llvm::Value *Mask = nullptr,
                                 llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_wavePrefixBitCount(llvm::Value *src, llvm::Value *Mask = nullptr,
                                         llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveMatch(llvm::Instruction *inst, llvm::Value *src, llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveMultiPrefix(llvm::Instruction *I, llvm::Value *Val, llvm::Value *Mask, IGC::WaveOps OpKind,
                                      llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveMultiPrefixBitCount(llvm::Instruction *I, llvm::Value *Val, llvm::Value *Mask,
                                              llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveClusteredAll(llvm::Value *src, llvm::Value *reductionType, llvm::Value *clusterSize,
                                       llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_waveClusteredBroadcast(llvm::Value *src, llvm::Value *clusterLane, llvm::Value *clusterSize,
                                             llvm::Value *helperLaneMode = nullptr);
  llvm::Value *create_quadPrefix(llvm::Value *src, llvm::Value *type, bool inclusive = false);
  llvm::Value *get16BitLaneID();
  llvm::Value *get32BitLaneID();
  llvm::Value *getSimdSize();
  llvm::Value *getFirstLaneID(llvm::Value *helperLaneMode = nullptr);
  llvm::Value *readFirstLane(llvm::Value *src, llvm::Value *helperLaneMode = nullptr);

  void VectorToScalars(llvm::Value *vector, llvm::Value **outScalars, unsigned maxSize,
                       llvm::Value *initializer = nullptr);

  llvm::Value *ScalarsToVector(llvm::Value *(&scalars)[4], unsigned vectorElementCnt);

  llvm::GenIntrinsicInst *CreateLaunder(llvm::Value *V);

private:
  /// @brief helper structure for keeping image format properties used by image read emulation
  struct ImageFormatInfo {
    IGC::SURFACE_FORMAT m_Format; ///< original image format
    unsigned
        m_NumComponents; ///< number of components in the original format, only valid when m_RequiresConversion is true
    unsigned m_BPE;      ///< number of bits per component in the original format, valid only if all components have the
                         ///< same, only valid when m_RequiresConversion is true
    bool m_RequiresConversion;              ///< true if conversion is required
    IGC::SURFACE_FORMAT m_SubstituteFormat; ///< image format programmed in the additional surface state, only valid
                                            ///< when m_RequiresConversion is true
    bool m_Is128b;                          ///< true for 128 bit formats, only valid when m_RequiresConversion is true
  };

  llvm::Constant *GetSnormFactor(unsigned bits);
  llvm::Constant *GetUnormFactor(unsigned bits);

  bool NeedConversionFor128FormatRead(IGC::SURFACE_FORMAT format) const;

  // these functions are taken from OCL builtins
  llvm::Value *CreateDFloor(llvm::Value *val);
  llvm::Value *CreateDTrunc(llvm::Value *val);
  llvm::Value *CreateDCeil(llvm::Value *val);
  llvm::Value *CreateDRoundNE(llvm::Value *val);

  genplatform *m_Platform;

  llvm::Function *llvm_GenISA_ubfe() const;

  llvm::Function *llvm_GenISA_ibfe() const;

  llvm::Function *llvm_GenISA_bfi() const;

  llvm::Function *llvm_GenISA_bfrev() const;

  llvm::Function *llvm_GenISA_firstbitHi() const;

  llvm::Function *llvm_GenISA_firstbitLo() const;

  llvm::Function *llvm_GenISA_firstbitShi() const;

  llvm::Function *llvm_GenISA_ld() const;

  llvm::Function *llvm_GenISA_ldms() const;

  llvm::Function *llvm_GenISA_ldmcs() const;

  llvm::Function *llvm_GenISA_sampleBC_v4f32_f32() const;

  llvm::ConstantInt *m_int0; // 0
  llvm::ConstantInt *m_int1; // 1
  llvm::ConstantInt *m_int2; // 2
  llvm::ConstantInt *m_int3; // 3

  llvm::ConstantFP *m_float0; // 0.0f
  llvm::ConstantFP *m_float1; // 1.0f

  void Init();
};

#include "BuiltinsFrontendDefinitions.hpp"

#endif

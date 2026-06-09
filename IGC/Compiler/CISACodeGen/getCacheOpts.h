/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/**
 * @file getCacheOpts.h
 *
 * @section DESCRIPTION
 *
 * This file contains declarations of utility functions which return the cache options
 * for a given load or store instruction.
 */
#pragma once

#include "visa_igc_common_header.h" // for LSC_L1_L3_CC

#include "common/LLVMWarningsPush.hpp" // for suppressing LLVM warnings
#include "llvm/IR/Instructions.h"      // for llvm::StoreInst, llvm::LoadInst
#include "common/LLVMWarningsPop.hpp"  // for suppressing LLVM warnings
#include <optional>                    // for Optional

#include "Compiler/CodeGenPublic.h"

namespace IGC {
/**
 * @param Ptr        Pointer to memory
 * @param Ctx        Context
 *
 * @return  Optional containing the cache options for that particular store instruction
 *          if the RTRegion is RTAsynctack, SWStack, SWHotZone, RTSynctack
 *          or empty Optional if the RTRegion cannot be known, or if it is a RTGlobals or LocalArgs
 */
std::optional<LSC_L1_L3_CC> getCacheOptsStorePolicy(const llvm::Value *Ptr, const CodeGenContext &Ctx);

/**
 * @param storeInst  LLVM IR store instruction
 * @param Ctx        Context
 *
 * @return  Optional containing the cache options for that particular store instruction
 *          if the RTRegion is RTAsynctack, SWStack, SWHotZone, RTSynctack
 *          or empty Optional if the RTRegion cannot be known, or if it is a RTGlobals or LocalArgs
 */
std::optional<LSC_L1_L3_CC> getCacheOptsStorePolicy(const llvm::StoreInst &storeInst, const CodeGenContext &Ctx);

/**
 * @param Ptr       Pointer to memory
 * @param Ctx        Context
 *
 * @return  Optional containing the cache options for that particular load instruction
 *          if the RTRegion is RTAsynctack, SWStack, SWHotZone, RTSynctack
 *          or empty Optional if the RTRegion cannot be known, or if it is a RTGlobals or LocalArgs
 */
std::optional<LSC_L1_L3_CC> getCacheOptsLoadPolicy(const llvm::Value *Ptr, const CodeGenContext &Ctx);

/**
 * @param loadInst  LLVM IR load instruction
 * @param Ctx        Context
 *
 * @return  Optional containing the cache options for that particular load instruction
 *          if the RTRegion is RTAsynctack, SWStack, SWHotZone, RTSynctack
 *          or empty Optional if the RTRegion cannot be known, or if it is a RTGlobals or LocalArgs
 */
std::optional<LSC_L1_L3_CC> getCacheOptsLoadPolicy(const llvm::LoadInst &loadInst, const CodeGenContext &Ctx);

/**
 * @return the store cache policy for the RTStack
 */
LSC_L1_L3_CC RTStackStorePolicy();

/**
 * @return the store cache policy for the SWHotZone
 */
LSC_L1_L3_CC SWHotZoneStorePolicy();

/**
 * @return the store cache policy for the SWStack
 */
LSC_L1_L3_CC SWStackStorePolicy(const CodeGenContext &Ctx);

/**
 * @return the load cache policy for the RTStack
 */
LSC_L1_L3_CC RTStackLoadPolicy();

/**
 * @return the load cache policy for the SWHotZone
 */
LSC_L1_L3_CC SWHotZoneLoadPolicy();

/**
 * @return the load cache policy for the SWStack
 */
LSC_L1_L3_CC SWStackLoadPolicy(const CodeGenContext &Ctx);

/**
 * Translate a compiler-facing LSC cache-control enum into the emit-time
 * LSC_CACHE_OPTS {l1, l2, l3}. The input may be a legacy two-level
 * LSC_L1_L3_CC value or a native three-level LSC_LDCC / LSC_STCC value packed
 * into the same integer space.
 *
 * This is the single source of truth for the legacy L1/L3 -> L1/L2/L3
 * promotion; it is platform-aware (the result depends on
 * hasNewLSCCacheEncoding() and hasEfficient64bEnabled()). Unsupported enums emit
 * a warning and fall back to LSC_L1DEF_L3DEF.
 *
 * @param l1l3cc              cache-control enum to translate
 * @param isLoad              true for loads/prefetches, false for stores/atomics
 * @param warningContextValue value used to anchor the "unsupported cache
 *                            controls" warning (may be null)
 * @param Ctx                 codegen context (platform predicates + warning sink)
 *
 * @return the resolved cache options for the current platform
 */
LSC_CACHE_OPTS translateLSCCacheControlsEnum(LSC_L1_L3_CC l1l3cc, bool isLoad, const llvm::Value *warningContextValue,
                                             CodeGenContext &Ctx);

/**
 * Compute the constant-buffer load cache-control *enum* decision.
 *
 * Mirrors the former EmitVISA constant-buffer decision: for a load whose buffer
 * operand is a constant buffer (constant addrspace / CONSTANT_BUFFER /
 * BINDLESS_CONSTANT_BUFFER / SSH_BINDLESS_CONSTANT_BUFFER) it returns the IGC1
 * cache-control enum that should override the baseline:
 *   - on Xe2+ (unless DisableSystemMemoryCachingInGPUForConstantBuffers): LSC_L1C_L3CC
 *   - for RAYTRACING_SHADER with ForceRTConstantBufferCacheCtrl: RTConstantBufferCacheCtrl
 * Returns nullopt when no constant-buffer override applies. The returned enum is
 * exactly what translateLSCCacheControlsEnum was previously fed by Emit, so the
 * resolved opts are byte-identical.
 *
 * @param inst load instruction whose buffer operand is inspected
 * @param Ctx  codegen context (platform predicates + shader type)
 */
std::optional<LSC_L1_L3_CC> getConstantBufferLoadCacheControlEnum(llvm::Instruction *inst, CodeGenContext &Ctx);

/**
 * Compute the default ray-tracing cache-control *enum* for loads/stores.
 *
 * Mirrors the former EmitVISA ray-tracing default policy minus the final
 * translateLSCCacheControlsEnum promotion. Defaults:
 *   load  = LSC_L1C_WT_L3C_WB, store = LSC_L1IAR_WB_L3C_WB
 * steered by ForceGenMem{Load,Store}CacheCtrl + the D3DOnly RayDispatch opts.
 * Returns nullopt when ForceGenMemDefaultCacheCtrl forces the platform default
 * (i.e. {DEFAULT,DEFAULT}); the caller must treat that as "leave at default".
 *
 * @param isLoad true for loads/prefetches, false for stores
 * @param Ctx    codegen context (platform predicates + shader type)
 */
std::optional<LSC_L1_L3_CC> getDefaultRaytracingCacheControlEnum(bool isLoad, CodeGenContext &Ctx);
} // namespace IGC

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
} // namespace IGC

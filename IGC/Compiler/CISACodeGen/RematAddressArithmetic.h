/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_REMATADDRESSARITHMETIC_H_
#define _CISA_REMATADDRESSARITHMETIC_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
llvm::FunctionPass *createRematAddressArithmeticPass();
void initializeRematAddressArithmeticPass(llvm::PassRegistry &);
enum REMAT_OPTIONS : uint8_t;
// ChainLimit: number of instructions a remat chain is allowed to copy, chains at or above it are skipped.
// FlowThresholdPercent: percentage of the total flow of all remat targets used as a cutoff for a chain.
// RPELimitPercent: percentage of the GRF budget, kernels with max register pressure below it are not rematted.
// GetSIMDSize: picks the SIMD width used for register pressure estimation per function.
// FirstCompile: the pass runs before SIMD selection. Rematerialization is rolled back when it leaves the pressure
// over the GRF budget of the SIMD16 drop on XE2 or over the largest GRF mode on platforms with VRT.
using RematSIMDSizeFn = SIMDMode (*)(const CodeGenContext *, const llvm::Function *, GenXFunctionGroupAnalysis *);
llvm::FunctionPass *createCloneAddressArithmeticPass(unsigned ChainLimit, unsigned FlowThresholdPercent,
                                                     unsigned RPELimitPercent,
                                                     RematSIMDSizeFn GetSIMDSize = bestGuessSIMDSize,
                                                     bool FirstCompile = false);
llvm::FunctionPass *createCloneAddressArithmeticPassWithFlags(IGC::REMAT_OPTIONS, unsigned ChainLimit,
                                                              unsigned FlowThresholdPercent, unsigned RPELimitPercent,
                                                              RematSIMDSizeFn GetSIMDSize = bestGuessSIMDSize,
                                                              bool FirstCompile = false);
void initializeCloneAddressArithmeticPass(llvm::PassRegistry &);
} // End namespace IGC

#endif // _CISA_REMATADDRESSARITHMETIC_H_

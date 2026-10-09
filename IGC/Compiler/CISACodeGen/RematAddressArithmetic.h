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
// SIMDSize: lane count used for register pressure estimation, 0 asks the pass to use bestGuessSIMDSize().
llvm::FunctionPass *createCloneAddressArithmeticPass(unsigned ChainLimit, unsigned FlowThresholdPercent,
                                                     unsigned RPELimitPercent, unsigned SIMDSize = 0);
llvm::FunctionPass *createCloneAddressArithmeticPassWithFlags(IGC::REMAT_OPTIONS, unsigned ChainLimit,
                                                              unsigned FlowThresholdPercent, unsigned RPELimitPercent,
                                                              unsigned SIMDSize = 0);
void initializeCloneAddressArithmeticPass(llvm::PassRegistry &);
} // End namespace IGC

#endif // _CISA_REMATADDRESSARITHMETIC_H_

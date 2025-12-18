/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Header for defining passes in the new PM
///
//===----------------------------------------------------------------------===//

#ifndef VC_GENXOPTS_GENXOPTSNEWPM_H
#define VC_GENXOPTS_GENXOPTSNEWPM_H

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

class FunctionPass;
class ModulePass;
class Pass;
class PassRegistry;
} // namespace llvm

#define DISABLE_TM

#include "CMABI.h"
#include "CMABIAnalysis.h"
#include "CMImpParam.h"
#include "CMKernelArgOffset.h"
#include "CMLowerVLoadVStore.h"

#include "GenXBIFFlagCtrlResolution.h"
#include "GenXBTIAssignment.h"
#include "GenXCloneIndirectFunctions.h"
#include "GenXImportOCLBiF.h"
#include "GenXLegalizeGVLoadUses.h"
#include "GenXLinkageCorruptor.h"
#include "GenXPacketize.h"
#include "GenXPrintfLegalization.h"
#include "GenXPrintfPhiClonning.h"
#include "GenXPrintfResolution.h"
#include "GenXSimplify.h"
#include "GenXStatePointerFence.h"
#include "GenXTrampolineInsertion.h"
#include "GenXTranslateIntrinsics.h"
#include "GenXTranslateSPIRVBuiltins.h"
#include "GenXTypeLegalization.h"

#endif // VC_GENXOPTS_GENXOPTSNEWPM_H

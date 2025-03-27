/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// NOTE: NO INCLUDE GUARD DESIRED!

#ifndef MODULE_PASS
#define MODULE_PASS(NAME, CREATE_PASS)
#endif

MODULE_PASS("CMABI", CMABIPass())
MODULE_PASS("CMImpParam", CMImpParamPass())
MODULE_PASS("CMKernelArgOffset",
            CMKernelArgOffsetPass(GTM->getGenXSubtarget().getGRFByteSize(),
                                  BC->useBindlessImages()))
MODULE_PASS("GenXPacketize", GenXPacketizePass())
MODULE_PASS("GenXBIFFlagCtrlResolution", GenXBIFFlagCtrlResolutionPass())
MODULE_PASS("GenXBTIAssignment", GenXBTIAssignmentPass(BC->getResult()))

MODULE_PASS("GenXImportOCLBiF", GenXImportOCLBiFPass())
MODULE_PASS("GenXLegalizeGVLoadUses", GenXLegalizeGVLoadUsesPass())
MODULE_PASS("GenXLinkageCorruptor", GenXLinkageCorruptorPass(BC->getResult()))
MODULE_PASS("GenXPrintfLegalization", GenXPrintfLegalizationPass())
MODULE_PASS("GenXPrintfPhiClonning", GenXPrintfPhiClonningPass())
MODULE_PASS("GenXPrintfResolution", GenXPrintfResolutionPass(TM))
MODULE_PASS("GenXTrampolineInsertion",
            GenXTrampolineInsertionPass(BC->getResult()))
MODULE_PASS("GenXTranslateSPIRVBuiltins",
            GenXTranslateSPIRVBuiltinsPass(BC->getResult()))
MODULE_PASS("GenXCloneIndirectFunctions",
            GenXCloneIndirectFunctionsPass(BC->getResult()))
MODULE_PASS("GenXVerify", GenXVerifyPass())

#undef MODULE_PASS

#ifndef FUNCTION_PASS
#define FUNCTION_PASS(NAME, CREATE_PASS)
#endif

FUNCTION_PASS("GenXSimplify", GenXSimplifyPass())
FUNCTION_PASS("CMLowerVLoadVStore", CMLowerVLoadVStorePass())
FUNCTION_PASS("GenXTypeLegalization", GenXTypeLegalizationPass())
FUNCTION_PASS("GenXTranslateIntrinsics", GenXTranslateIntrinsicsPass())
FUNCTION_PASS("GenXLowerAggrCopies", GenXLowerAggrCopiesPass())
FUNCTION_PASS("GenXRegionCollapsing", GenXRegionCollapsingPass(TM))

#undef FUNCTION_PASS

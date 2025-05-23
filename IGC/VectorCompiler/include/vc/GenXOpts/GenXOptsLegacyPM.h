/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This header file defines prototypes for accessor functions that expose passes
// in the GenX transformations library.
//
//===----------------------------------------------------------------------===//

#ifndef VC_GENXOPTS_GENXOPTSLEGACYPM_H
#define VC_GENXOPTS_GENXOPTSLEGACYPM_H

namespace llvm {

class FunctionPass;
class ModulePass;
class Pass;
class PassRegistry;

//===----------------------------------------------------------------------===//
//
// CMImpParam - Transforms to enable implicit parameters
//
Pass *createCMImpParamPass(bool);

//===----------------------------------------------------------------------===//
//
// CMKernelArgOffset - Determine offset of each CM kernel argument
//
Pass *createCMKernelArgOffsetPass(unsigned GrfByteSize, bool UseBindlessImages);

//===----------------------------------------------------------------------===//
//
// CMABI - Fix ABI issues for the genx backend.
//
Pass *createCMABILegacyPass();

//===----------------------------------------------------------------------===//
//
// CMLowerVLoadVStore - Lower CM reference loads and stores.
//
Pass *createCMLowerVLoadVStorePass();

FunctionPass *createGenXReduceIntSizePass();
FunctionPass *createGenXRegionCollapsingPass();
FunctionPass *createGenXSimplifyPass();
FunctionPass *createGenXLowerAggrCopiesPass();

ModulePass *createGenXPacketizePass();
void initializeGenXPacketizePass(PassRegistry &);

void initializeGenXSimplifyPass(PassRegistry &);
void initializeCMABILegacyPass(PassRegistry &);
void initializeCMImpParamPass(PassRegistry &);
void initializeCMKernelArgOffsetPass(PassRegistry &);

ModulePass *createGenXPrintfResolutionPass();
void initializeGenXPrintfResolutionPass(PassRegistry &);

ModulePass *createGenXPrintfPhiClonningPass();
void initializeGenXPrintfPhiClonningPass(PassRegistry &);

ModulePass *createGenXPrintfLegalizationPass();
void initializeGenXPrintfLegalizationPass(PassRegistry &);

ModulePass *createGenXImportOCLBiFPass();
void initializeGenXImportOCLBiFPass(PassRegistry &);

ModulePass *createGenXBIFFlagCtrlResolutionPass();
void initializeGenXBIFFlagCtrlResolutionPass(PassRegistry &);

ModulePass *createGenXBTIAssignmentPass();
void initializeGenXBTIAssignmentPass(PassRegistry &);

ModulePass *createGenXTranslateSPIRVBuiltinsPass();
void initializeGenXTranslateSPIRVBuiltinsPass(PassRegistry &);

ModulePass *createGenXCloneIndirectFunctionsPass();
void initializeGenXCloneIndirectFunctionsPass(PassRegistry &);

ModulePass *createGenXTrampolineInsertionPass();
void initializeGenXTrampolineInsertionPass(PassRegistry &);

ModulePass *createGenXLinkageCorruptorPass();
void initializeGenXLinkageCorruptorPass(PassRegistry &);

FunctionPass *createGenXTranslateIntrinsicsPass();
void initializeGenXTranslateIntrinsicsPass(PassRegistry &);

FunctionPass *createGenXTypeLegalizationPass();
void initializeGenXTypeLegalizationPass(PassRegistry &);

void initializeCMLowerVLoadVStorePass(PassRegistry &);

} // namespace llvm

#endif // VC_GENXOPTS_GENXOPTSLEGACYPM_H

/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This header file defines prototypes for accessor functions that expose passes
// in the GenX transformations library.
//
//===----------------------------------------------------------------------===//

#ifndef VC_GENXOPTS_GENXOPTS_H
#define VC_GENXOPTS_GENXOPTS_H

namespace llvm {

class FunctionPass;
class ModulePass;
class Pass;
class PassRegistry;

//===----------------------------------------------------------------------===//
//
// CMImpParam - Transforms to enable implicit parameters
//
Pass *createCMImpParamPass(bool, bool);

//===----------------------------------------------------------------------===//
//
// CMKernelArgOffset - Determine offset of each CM kernel argument
//
Pass *createCMKernelArgOffsetPass(unsigned GrfByteSize, bool OCLCodeGen);

//===----------------------------------------------------------------------===//
//
// CMABI - Fix ABI issues for the genx backend.
//
Pass *createCMABIPass();

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

void initializeGenXSimplifyPass(PassRegistry &);
void initializeCMABIPass(PassRegistry &);
void initializeCMImpParamPass(PassRegistry &);
void initializeCMKernelArgOffsetPass(PassRegistry &);

ModulePass *createGenXPrintfResolutionPass();
void initializeGenXPrintfResolutionPass(PassRegistry &);

ModulePass *createGenXPrintfLegalizationPass();
void initializeGenXPrintfLegalizationPass(PassRegistry &);

ModulePass *createGenXImportOCLBiFPass();
void initializeGenXImportOCLBiFPass(PassRegistry &);

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

} // End llvm namespace

#endif // VC_GENXOPTS_GENXOPTS_H

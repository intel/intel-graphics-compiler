/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef TARGET_GENX_H
#define TARGET_GENX_H

#include "visa_igc_common_header.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"

#include "vc/Utils/GenX/TypeSize.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

#include <string>

namespace llvm {

class BasicBlock;
class CallInst;
class Constant;
class DebugLoc;
class DominatorTree;
class formatted_raw_ostream;
class Function;
class FunctionGroup;
class FunctionPass;
class GenXSubtarget;
class Instruction;
class MDNode;
class ModulePass;
class ShuffleVectorInst;
class TargetOptions;
class Twine;
class Value;
class raw_ostream;
class raw_pwrite_stream;

enum BalingKind {
  BK_Legalization, // build baling info for legalization
  BK_CodeGen,      // build baling info for the final vISA emission
  BK_Analysis,     // build baling info for analysis (register pressure)
};

enum class PatternMatchKind {
  PreLegalization,   // pattern match before legalization
  PostLegalization,  // pattern match after legalization
};

FunctionPass *createGenXPrinterPass(raw_ostream &O, const std::string &Banner);
ModulePass *createGenXGroupPrinterPass(raw_ostream &O,
                                       const std::string &Banner);
FunctionPass *createGenXAnalysisDumperPass(FunctionPass *Analysis,
                                           StringRef DumpNamePrefix,
                                           StringRef DumpNameSuffix);
ModulePass *createGenXModuleAnalysisDumperPass(ModulePass *Analysis,
                                               StringRef DumpNamePrefix,
                                               StringRef DumpNameSuffix);

FunctionPass *createGenXCFSimplificationPass();
ModulePass *createGenXEarlySimdCFConformancePass();
FunctionPass *createGenXPredToSimdCFPass();
FunctionPass *createGenXReduceIntSizePass();
FunctionPass *createGenXInlineAsmLoweringPass();
FunctionPass *createGenXLoweringPass();
FunctionPass *createGenXVectorCombinerPass();
FunctionPass *createGenXLowerAggrCopiesPass();
FunctionPass *createGenXLowerJmpTableSwitchPass();
FunctionPass *createGenXGEPLoweringPass();
FunctionPass *createGenXRegionCollapsingPass();
FunctionPass *createGenXExtractVectorizerPass();
FunctionPass *createGenXRawSendRipperPass();
FunctionPass *createGenXFuncBalingPass(BalingKind Kind, GenXSubtarget *ST);
FunctionPass *createGenXFuncLiveElementsPass();
FunctionPass *createGenXPrologEpilogInsertionPass();
FunctionPass *createGenXLegalizationPass();
ModulePass *createGenXEmulatePass();
ModulePass *createGenXEmulationImportPass();
ModulePass *createGenXEmulationModulePreparePass();
FunctionPass *createGenXDeadVectorRemovalPass();
FunctionPass *createGenXPatternMatchPass(PatternMatchKind Kind);
FunctionPass *createGenXPostLegalizationPass();
FunctionPass *createGenXPromoteArrayPass();
ModulePass *createGenXThreadPrivateMemoryPass();
FunctionPass *createGenXPromotePredicatePass();
FunctionPass *createGenXIMadPostLegalizationPass();
FunctionPass *createGenXAggregatePseudoLoweringPass();
ModulePass *createGenXModulePass();
ModulePass *createGenXLateSimdCFConformanceWrapperPass();
ModulePass *createGenXLivenessWrapperPass();
FunctionPass *createGenXLoadStoreLoweringPass();
ModulePass *createGenXCategoryWrapperPass();
ModulePass *createGenXGroupBalingWrapperPass(BalingKind Kind,
                                             GenXSubtarget *ST);
ModulePass *createGenXGroupLiveElementsWrapperPass();
ModulePass *createGenXUnbalingWrapperPass();
ModulePass *createGenXDepressurizerWrapperPass();
ModulePass *createGenXLateLegalizationWrapperPass();
ModulePass *createGenXNumberingWrapperPass();
ModulePass *createGenXLiveRangesWrapperPass();
ModulePass *createGenXRematerializationWrapperPass();
ModulePass *createGenXCoalescingWrapperPass();
ModulePass *createGenXGVClobberCheckerWrapperPass();
ModulePass *createGenXAddressCommoningWrapperPass();
ModulePass *createGenXArgIndirectionWrapperPass();
FunctionPass *createGenXTidyControlFlowPass();
ModulePass *createGenXVisaRegAllocWrapperPass();
ModulePass *createGenXCisaBuilderWrapperPass();
ModulePass *createGenXFinalizerPass();
ModulePass *createGenXDebugInfoPass();
ModulePass *createGenXGlobalValueLoweringPass();
ModulePass *createGenXPromoteStatefulToBindlessPass();
ModulePass *createGenXStackUsagePass();
ModulePass *createGenXStructSplitterPass();
FunctionPass *createGenXPredRegionLoweringPass();
FunctionPass *createGenXDebugLegalizationPass();
FunctionPass *createGenXFixInvalidFuncNamePass();
ModulePass *createGenXGASCastWrapperPass();
FunctionPass *createGenXGASDynamicResolutionPass();
ModulePass *createGenXInitBiFConstantsPass();
FunctionPass *createGenXGlobalUniformAnalysisPass();

namespace genx {

// A local encoding (not part of vISA or GenX) of whether an operand should be signed.
enum Signedness {
  DONTCARESIGNED = 3, SIGNED = 1, UNSIGNED = 2
};
constexpr unsigned BoolBits = vc::BoolBits;
constexpr unsigned ByteBits = vc::ByteBits;
constexpr unsigned WordBits = vc::WordBits;
constexpr unsigned DWordBits = vc::DWordBits;
constexpr unsigned QWordBits = vc::QWordBits;
constexpr unsigned OWordBits = vc::OWordBits;

constexpr unsigned ByteBytes = ByteBits / ByteBits;
constexpr unsigned WordBytes = WordBits / ByteBits;
constexpr unsigned DWordBytes = DWordBits / ByteBits;
constexpr unsigned QWordBytes = QWordBits / ByteBits;
constexpr unsigned OWordBytes = OWordBits / ByteBits;

constexpr unsigned SurfaceElementBytes = 4;
constexpr unsigned SamplerElementBytes = 4;

// Currently EM determines behavior of 32 lanes.
// Probably that should be moved to subtarget if
// different targets will support different EM sizes.
constexpr unsigned TotalEMSize = 32;

// vISA allows [-512,511] for operation to be baled as offset
// for rdregion, copied from visa
constexpr int G4_MAX_ADDR_IMM = 511;
constexpr int G4_MIN_ADDR_IMM = -512;

// Default GRF Width if subtarget is not available
constexpr unsigned defaultGRFByteSize = 32;

// describe integer vector immediate (V, UV)
enum ImmIntVec : int8_t {
  Width = 8, // num elem in vector
  ElemSize = 4, // in bits
  MaxUInt = (1 << ElemSize) - 1,
  MinUInt = 0,
  MaxSInt = (1 << (ElemSize - 1)) - 1,
  MinSInt = -(1 << (ElemSize - 1))
};

} // End genx namespace
} // End llvm namespace

#endif

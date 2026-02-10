/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
/// Listed below are ALL llvm/* modules included when building Windows IGC project
/// Not used ones are commented out with a reason for exclusion
//===----------------------------------------------------------------------===//

// LLVM
#include "../common/LLVMWarningsPush.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/PriorityQueue.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/PtrUseVisitor.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/SyntheticCountsUtils.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TargetTransformInfoImpl.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/Bitcode/LLVMBitCodes.h"
#include "llvm/Bitstream/BitstreamReader.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/StackProtector.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/AutoUpgrade.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/GVMaterializer.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassManagers.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PassTimingInfo.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAsmInfoELF.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/StringTableBuilder.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/Option.h"
#include "llvm/Pass.h"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Atomic.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/DebugCounter.h"
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/GenericDomTree.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Mutex.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ScaledNumber.h"
#include "llvm/Support/ScopedPrinter.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Threading.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionImport.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/IPO/Internalize.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/InstSimplifyPass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/FunctionComparator.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/SSAUpdaterBulk.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "../common/LLVMWarningsPop.hpp"
#include "llvm-c/Core.h"
#include "llvmWrapper/TargetParser/Triple.h"
#include "llvmWrapper/ADT/APInt.h"
#include "llvmWrapper/ADT/Optional.h"
#include "llvmWrapper/ADT/STLExtras.h"
#include "llvmWrapper/ADT/StringExtras.h"
#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/Analysis/AliasAnalysis.h"
#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/Analysis/InstructionSimplify.h"
#include "llvmWrapper/Analysis/MemoryLocation.h"
#include "llvmWrapper/Analysis/TargetLibraryInfo.h"
#include "llvmWrapper/IR/BasicBlock.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/CFG.h"
#include "llvmWrapper/IR/ConstantFold.h"
#include "llvmWrapper/IR/ConstantFolder.h"
#include "llvmWrapper/IR/Constants.h"
#include "llvmWrapper/IR/DataLayout.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/DIBuilder.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/IntrinsicInst.h"
#include "llvmWrapper/IR/Intrinsics.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/IR/Operator.h"
#include "llvmWrapper/IR/PatternMatch.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/User.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/MC/MCContext.h"
#include "llvmWrapper/MC/MCObjectFileInfo.h"
#include "llvmWrapper/MC/MCStreamer.h"
#include "llvmWrapper/Option/OptTable.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Support/FileSystem.h"
#include "llvmWrapper/Support/MathExtras.h"
#include "llvmWrapper/Support/Regex.h"
#include "llvmWrapper/Support/TargetRegistry.h"
#include "llvmWrapper/Support/TypeSize.h"
#include "llvmWrapper/Support/YAMLParser.h"
#include "llvmWrapper/Target/TargetMachine.h"
#include "llvmWrapper/Transforms/InstCombine/InstCombineWorklist.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "llvmWrapper/Transforms/Utils/Local.h"
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "llvmWrapper/Transforms/Utils/ValueMapper.h"

// Excluded LLVM headers with reasons

// #include "llvm/ADT/APInt.h" - uses llvmWrapper
// #include "llvm/ADT/Optional.h" - uses llvmWrapper
// #include "llvm/ADT/STLExtras.h" - uses llvmWrapper
// #include "llvm/ADT/StringExtras.h" - uses llvmWrapper
// #include "llvm/ADT/StringRef.h" - uses llvmWrapper
// #include "llvm/Analysis/AliasAnalysis.h" - uses llvmWrapper
// #include "llvm/Analysis/BasicAliasAnalysis.h" - uses llvmWrapper
// #include "llvm/Analysis/CallGraph.h" - uses llvmWrapper
// #include "llvm/Analysis/CFG.h" - uses llvmWrapper
// #include "llvm/Analysis/InstructionSimplify.h" - uses llvmWrapper
// #include "llvm/Analysis/MemoryLocation.h" - uses llvmWrapper
// #include "llvm/Analysis/ScalarEvolutionExpander.h" - breaks llvm 14
// #include "llvm/Analysis/TargetLibraryInfo.h" - uses llvmWrapper
// #include "llvm/CodeGen/CommandFlags.h" - uses llvmWrapper
// #include "llvm/CodeGen/MachineFunction.h" - uses llvmWrapper
// #include "llvm/Config/llvm-config.h" - defines LLVM_VERSION_MAJOR macro
// #include "llvm/IR/Attributes.h" - uses llvmWrapper
// #include "llvm/IR/BasicBlock.h" - uses llvmWrapper
// #include "llvm/IR/CallSite.h" - uses llvmWrapper
// #include "llvm/IR/CFG.h" - uses llvmWrapper
// #include "llvm/IR/ConstantFolder.h" - uses llvmWrapper
// #include "llvm/IR/Constants.h" - - uses llvmWrapper
// #include "llvm/IR/DataLayout.h" - uses llvmWrapper
// #include "llvm/IR/DerivedTypes.h" - uses llvmWrapper
// #include "llvm/IR/DIBuilder.h" - uses llvmWrapper
// #include "llvm/IR/Function.h" - uses llvmWrapper
// #include "llvm/IR/GlobalValue.h" - uses llvmWrapper
// #include "llvm/IR/InstrTypes.h" - uses llvmWrapper
// #include "llvm/IR/Instructions.h" - uses llvmWrapper
// #include "llvm/IR/IntrinsicInst.h" - uses llvmWrapper
// #include "llvm/IR/Intrinsics.h" - uses llvmWrapper
// #include "llvm/IR/IRBuilder.h" - uses llvmWrapper
// #include "llvm/IR/Module.h" - uses llvmWrapper
// #include "llvm/IR/Operator.h" - uses llvmWrapper
// #include "llvm/IR/PatternMatch.h" - uses llvmWrapper
// #include "llvm/IR/Type.h" - uses llvmWrapper
// #include "llvm/IR/Value.h" - uses llvmWrapper
// #include "llvm/MC/MCContext.h" - uses llvmWrapper
// #include "llvm/MC/MCObjectFileInfo.h" - uses llvmWrapper
// #include "llvm/MC/MCStreamer.h" - uses llvmWrapper
// #include "llvm/MC/MCValue.h" - uses llvmWrapper
// #include "llvm/Option/OptTable.h" - uses llvmWrapper
// #include "llvm/Support/FileSystem.h" - uses llvmWrapper
// #include "llvm/Support/PluginLoader.h" - custom defines
// #include "llvm/Support/Regex.h" - uses llvmWrapper
// #include "llvm/Support/SystemUtils.h" - uses llvmWrapper
// #include "llvm/Target/TargetMachine.h" - uses llvmWrapper
// #include "llvm/Transforms/InstCombine/InstCombineWorklist.h" - uses llvmWrapper
// #include "llvm/Transforms/Utils/Cloning.h" - uses llvmWrapper
// #include "llvm/Transforms/Utils/Local.h" - uses llvmWrapper
// #include "llvm/Transforms/Utils/LoopUtils.h" - uses llvmWrapper
// #include "llvm/Transforms/Utils/LowerMemIntrinsics.h" - uses llvmWrapper
// #include "llvm/Transforms/Utils/ValueMapper.h" - uses llvmWrapper
// #include "llvmWrapper/CodeGen/CommandFlags.h" causes multiple symbol definitions
// #include "llvmWrapper/Support/SystemUtils.h" - causes multiple symbol definitions

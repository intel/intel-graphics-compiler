/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2014 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal with the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.
Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the documentation
and/or other materials provided with the distribution.
Neither the names of Advanced Micro Devices, Inc., nor the names of its
contributors may be used to endorse or promote products derived from this
Software without specific prior written permission.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file declares classes and functions shared by SPIR-V reader/writer.

#ifndef SPIRVINTERNAL_HPP_
#define SPIRVINTERNAL_HPP_

#define DEBUG_TYPE "spirv"

#include "libSPIRV/SPIRVUtil.h"
#include "libSPIRV/SPIRVEnum.h"
#include "libSPIRV/SPIRVError.h"
#include "libSPIRV/SPIRVType.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvm/IR/Attributes.h"

#include "llvm/ADT/StringSwitch.h"
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ToolOutputFile.h"

#include "common/LLVMWarningsPop.hpp"

#include "common/UFO/portable_compiler.h"

#include <utility>
#include <functional>


namespace igc_spv {

using namespace llvm;

namespace kOCLTypeQualifierName {
  __attr_unused const static char *Const      = "const";
  __attr_unused const static char *Volatile   = "volatile";
  __attr_unused const static char *Restrict   = "restrict";
  __attr_unused const static char *Pipe       = "pipe";
}

template<> inline void
SPIRVMap<unsigned, Op>::init() {
#define _SPIRV_OP(x,y) add(Instruction::x, Op##y);
  /* Casts */
    _SPIRV_OP(ZExt, UConvert)
    _SPIRV_OP(SExt, SConvert)
    _SPIRV_OP(Trunc, UConvert)
    _SPIRV_OP(FPToUI, ConvertFToU)
    _SPIRV_OP(FPToSI, ConvertFToS)
    _SPIRV_OP(UIToFP, ConvertUToF)
    _SPIRV_OP(SIToFP, ConvertSToF)
    _SPIRV_OP(FPTrunc, FConvert)
    _SPIRV_OP(FPExt, FConvert)
    _SPIRV_OP(PtrToInt, ConvertPtrToU)
    _SPIRV_OP(IntToPtr, ConvertUToPtr)
    _SPIRV_OP(BitCast, Bitcast)
    _SPIRV_OP(GetElementPtr, AccessChain)
  /*Binary*/
    _SPIRV_OP(And, BitwiseAnd)
    _SPIRV_OP(Or, BitwiseOr)
    _SPIRV_OP(Xor, BitwiseXor)
    _SPIRV_OP(Add, IAdd)
    _SPIRV_OP(FAdd, FAdd)
    _SPIRV_OP(Sub, ISub)
    _SPIRV_OP(FSub, FSub)
    _SPIRV_OP(Mul, IMul)
    _SPIRV_OP(FMul, FMul)
    _SPIRV_OP(UDiv, UDiv)
    _SPIRV_OP(SDiv, SDiv)
    _SPIRV_OP(FDiv, FDiv)
    _SPIRV_OP(SRem, SRem)
    _SPIRV_OP(URem, UMod)
    _SPIRV_OP(Shl, ShiftLeftLogical)
    _SPIRV_OP(LShr, ShiftRightLogical)
    _SPIRV_OP(AShr, ShiftRightArithmetic)
#undef _SPIRV_OP
}
typedef SPIRVMap<unsigned, Op> OpCodeMap;

template<> inline void
SPIRVMap<CmpInst::Predicate, Op>::init() {
#define _SPIRV_OP(x,y) add(CmpInst::x, Op##y);
    _SPIRV_OP(FCMP_OEQ, FOrdEqual)
    _SPIRV_OP(FCMP_OGT, FOrdGreaterThan)
    _SPIRV_OP(FCMP_OGE, FOrdGreaterThanEqual)
    _SPIRV_OP(FCMP_OLT, FOrdLessThan)
    _SPIRV_OP(FCMP_OLE, FOrdLessThanEqual)
    _SPIRV_OP(FCMP_ONE, FOrdNotEqual)
    _SPIRV_OP(FCMP_ORD, Ordered)
    _SPIRV_OP(FCMP_UNO, Unordered)
    _SPIRV_OP(FCMP_UEQ, FUnordEqual)
    _SPIRV_OP(FCMP_UGT, FUnordGreaterThan)
    _SPIRV_OP(FCMP_UGE, FUnordGreaterThanEqual)
    _SPIRV_OP(FCMP_ULT, FUnordLessThan)
    _SPIRV_OP(FCMP_ULE, FUnordLessThanEqual)
    _SPIRV_OP(FCMP_UNE, FUnordNotEqual)
    _SPIRV_OP(ICMP_EQ, IEqual)
    _SPIRV_OP(ICMP_NE, INotEqual)
    _SPIRV_OP(ICMP_EQ, LogicalEqual)
    _SPIRV_OP(ICMP_NE, LogicalNotEqual)
    _SPIRV_OP(ICMP_UGT, UGreaterThan)
    _SPIRV_OP(ICMP_UGE, UGreaterThanEqual)
    _SPIRV_OP(ICMP_ULT, ULessThan)
    _SPIRV_OP(ICMP_ULE, ULessThanEqual)
    _SPIRV_OP(ICMP_SGT, SGreaterThan)
    _SPIRV_OP(ICMP_SGE, SGreaterThanEqual)
    _SPIRV_OP(ICMP_SLT, SLessThan)
    _SPIRV_OP(ICMP_SLE, SLessThanEqual)
#undef _SPIRV_OP
}
typedef SPIRVMap<CmpInst::Predicate, Op> CmpMap;

class IntBoolOpMapId;
template<> inline void
SPIRVMap<Op, Op, IntBoolOpMapId>::init() {
  add(OpIEqual,      OpLogicalEqual);
  add(OpINotEqual,   OpLogicalNotEqual);
  add(OpNot,         OpLogicalNot);
  add(OpBitwiseAnd,  OpLogicalAnd);
  add(OpBitwiseOr,   OpLogicalOr);
  add(OpBitwiseXor,  OpLogicalNotEqual);
}
typedef SPIRVMap<Op, Op, IntBoolOpMapId> IntBoolOpMap;

#define SPIR_TARGETTRIPLE32 "spir-unknown-unknown"
#define SPIR_TARGETTRIPLE64 "spir64-unknown-unknown"
#define SPIR_DATALAYOUT32 "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32"\
                          "-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32"\
                          "-v32:32:32-v48:64:64-v64:64:64-v96:128:128"\
                          "-v128:128:128-v192:256:256-v256:256:256"\
                          "-v512:512:512-v1024:1024:1024"
#define SPIR_DATALAYOUT64 "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32"\
                          "-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32"\
                          "-v32:32:32-v48:64:64-v64:64:64-v96:128:128"\
                          "-v128:128:128-v192:256:256-v256:256:256"\
                          "-v512:512:512-v1024:1024:1024"

enum SPIRAddressSpace {
  SPIRAS_Private,
  SPIRAS_Global,
  SPIRAS_Constant,
  SPIRAS_Local,
  SPIRAS_Generic,
  SPIRAS_Count,
  SPIRAS_Input,
};

enum OCLMemFenceKind {
  OCLMF_Local = 1,
  OCLMF_Global = 2,
  OCLMF_Image = 4,
};

enum OCLMemScopeKind {
  OCLMS_work_item,
  OCLMS_work_group,
  OCLMS_device,
  OCLMS_all_svm_devices,
  OCLMS_sub_group,
};

enum OCLMemOrderKind {
  OCLMO_relaxed,
  OCLMO_acquire,
  OCLMO_release,
  OCLMO_acq_rel,
  OCLMO_seq_cst
};

template<> inline void
SPIRVMap<SPIRAddressSpace, SPIRVStorageClassKind>::init() {
  add(SPIRAS_Private, StorageClassFunction);
  add(SPIRAS_Global, StorageClassWorkgroupGlobal);
  add(SPIRAS_Constant, StorageClassUniformConstant);
  add(SPIRAS_Local, StorageClassWorkgroupLocal);
  add(SPIRAS_Generic, StorageClassGeneric);
  add(SPIRAS_Input, StorageClassInput);
}
typedef SPIRVMap<SPIRAddressSpace, SPIRVStorageClassKind> SPIRSPIRVAddrSpaceMap;

// Maps OCL builtin function to SPIRV builtin variable.
template<> inline void
SPIRVMap<std::string, SPIRVAccessQualifierKind>::init() {
  add("read_only", AccessQualifierReadOnly);
  add("write_only", AccessQualifierWriteOnly);
  add("read_write", AccessQualifierReadWrite);
}
typedef SPIRVMap<std::string, SPIRVAccessQualifierKind> SPIRSPIRVAccessQualifierMap;

template<> inline void
SPIRVMap<std::string, Op>::init() {
#define _SPIRV_OP(x,y) add(#x, OpType##y);
    _SPIRV_OP(spirv.Event, Event)
    _SPIRV_OP(opencl.pipe_t, Pipe)
    _SPIRV_OP(spirv.ReserveId, ReserveId)
    _SPIRV_OP(spirv.DeviceEvent, DeviceEvent)
    _SPIRV_OP(spirv.Queue, Queue)
    _SPIRV_OP(spirv.Sampler, Sampler)
#undef _SPIRV_OP
}
typedef SPIRVMap<std::string, Op> BuiltinOpaqueGenericTypeOpCodeMap;

class mapSPIRVBuiltinVariableKind;
template<> inline void
SPIRVMap<SPIRVBuiltinVariableKind, std::string, mapSPIRVBuiltinVariableKind>::init() {
#define _SPIRV_OP(x, ...) add(igc_spv::BuiltIn::x, #x);
#include "libSPIRV/SPIRVBuiltinEnum.h"
#undef _SPIRV_OP
}
typedef SPIRVMap<SPIRVBuiltinVariableKind, std::string, mapSPIRVBuiltinVariableKind> SPIRSPIRVBuiltinVariableMap;

// Maps uniqued builtin function name to SPIRV instruction.
template<> inline void
SPIRVMap<Op, std::string, SPIRVInstruction>::init() {
#define _SPIRV_OP(x) add(x, #x);
// Composite
_SPIRV_OP(OpCompositeConstruct)
_SPIRV_OP(OpVectorExtractDynamic)
_SPIRV_OP(OpVectorInsertDynamic)
// Memory
_SPIRV_OP(OpGenericPtrMemSemantics)
// Atomics
_SPIRV_OP(OpAtomicAnd)
_SPIRV_OP(OpAtomicCompareExchange)
_SPIRV_OP(OpAtomicCompareExchangeWeak)
_SPIRV_OP(OpAtomicExchange)
_SPIRV_OP(OpAtomicFlagClear)
_SPIRV_OP(OpAtomicFlagTestAndSet)
_SPIRV_OP(OpAtomicIAdd)
_SPIRV_OP(OpAtomicIDecrement)
_SPIRV_OP(OpAtomicIIncrement)
_SPIRV_OP(OpAtomicISub)
_SPIRV_OP(OpAtomicLoad)
_SPIRV_OP(OpAtomicOr)
_SPIRV_OP(OpAtomicSMax)
_SPIRV_OP(OpAtomicSMin)
_SPIRV_OP(OpAtomicStore)
_SPIRV_OP(OpAtomicUMax)
_SPIRV_OP(OpAtomicUMin)
_SPIRV_OP(OpAtomicXor)
_SPIRV_OP(OpAtomicFMinEXT)
_SPIRV_OP(OpAtomicFMaxEXT)
_SPIRV_OP(OpAtomicFAddEXT)
_SPIRV_OP(OpDot)
_SPIRV_OP(OpGroupAsyncCopy)
_SPIRV_OP(OpGroupWaitEvents)
_SPIRV_OP(OpFOrdEqual)
_SPIRV_OP(OpFUnordNotEqual)
_SPIRV_OP(OpFOrdGreaterThan)
_SPIRV_OP(OpFOrdGreaterThanEqual)
_SPIRV_OP(OpFOrdLessThan)
_SPIRV_OP(OpFOrdLessThanEqual)
_SPIRV_OP(OpLessOrGreater)
_SPIRV_OP(OpOrdered)
_SPIRV_OP(OpUnordered)
_SPIRV_OP(OpIsFinite)
_SPIRV_OP(OpIsInf)
_SPIRV_OP(OpIsNan)
_SPIRV_OP(OpIsNormal)
_SPIRV_OP(OpSignBitSet)
_SPIRV_OP(OpAny)
_SPIRV_OP(OpAll)
// CL 2.0 kernel enqueue builtins
_SPIRV_OP(OpEnqueueMarker)
_SPIRV_OP(OpEnqueueKernel)
_SPIRV_OP(OpGetKernelNDrangeSubGroupCount)
_SPIRV_OP(OpGetKernelNDrangeMaxSubGroupSize)
_SPIRV_OP(OpGetKernelWorkGroupSize)
_SPIRV_OP(OpGetKernelPreferredWorkGroupSizeMultiple)
_SPIRV_OP(OpRetainEvent)
_SPIRV_OP(OpReleaseEvent)
_SPIRV_OP(OpCreateUserEvent)
_SPIRV_OP(OpIsValidEvent)
_SPIRV_OP(OpSetUserEventStatus)
_SPIRV_OP(OpCaptureEventProfilingInfo)
_SPIRV_OP(OpGetDefaultQueue)
_SPIRV_OP(OpBuildNDRange)
_SPIRV_OP(OpGetKernelLocalSizeForSubgroupCount)
_SPIRV_OP(OpGetKernelMaxNumSubgroups)
// Barriers
_SPIRV_OP(OpMemoryBarrier)
_SPIRV_OP(OpControlBarrier)
// SPV_INTEL_split_barrier
_SPIRV_OP(OpControlBarrierArriveINTEL)
_SPIRV_OP(OpControlBarrierWaitINTEL)
_SPIRV_OP(OpNamedBarrierInitialize)
_SPIRV_OP(OpMemoryNamedBarrier)
// CL 2.0 pipe builtins
_SPIRV_OP(OpReadPipe)
_SPIRV_OP(OpWritePipe)
_SPIRV_OP(OpReservedReadPipe)
_SPIRV_OP(OpReservedWritePipe)
_SPIRV_OP(OpReserveReadPipePackets)
_SPIRV_OP(OpReserveWritePipePackets)
_SPIRV_OP(OpCommitReadPipe)
_SPIRV_OP(OpCommitWritePipe)
_SPIRV_OP(OpIsValidReserveId)
_SPIRV_OP(OpGroupReserveReadPipePackets)
_SPIRV_OP(OpGroupReserveWritePipePackets)
_SPIRV_OP(OpGroupCommitReadPipe)
_SPIRV_OP(OpGroupCommitWritePipe)
_SPIRV_OP(OpGetNumPipePackets)
_SPIRV_OP(OpGetMaxPipePackets)
// Arithmetic Instructions
_SPIRV_OP(OpUMulExtended)
_SPIRV_OP(OpSMulExtended)
// Bit Instructions
_SPIRV_OP(OpBitFieldInsert)
_SPIRV_OP(OpBitFieldSExtract)
_SPIRV_OP(OpBitFieldUExtract)
_SPIRV_OP(OpBitReverse)
_SPIRV_OP(OpBitCount)
// CL 2.0 workgroup/subgroup builtins
_SPIRV_OP(OpGroupAll)
_SPIRV_OP(OpGroupAny)
_SPIRV_OP(OpGroupBroadcast)
_SPIRV_OP(OpGroupFAdd)
_SPIRV_OP(OpGroupFMax)
_SPIRV_OP(OpGroupFMin)
_SPIRV_OP(OpGroupIAdd)
_SPIRV_OP(OpGroupSMax)
_SPIRV_OP(OpGroupSMin)
_SPIRV_OP(OpGroupUMax)
_SPIRV_OP(OpGroupUMin)
// SPIR-V 1.3 non uniform subgroup builtins
_SPIRV_OP(OpGroupNonUniformElect)
_SPIRV_OP(OpGroupNonUniformAll)
_SPIRV_OP(OpGroupNonUniformAny)
_SPIRV_OP(OpGroupNonUniformAllEqual)
_SPIRV_OP(OpGroupNonUniformBroadcast)
_SPIRV_OP(OpGroupNonUniformBroadcastFirst)
_SPIRV_OP(OpGroupNonUniformBallot)
_SPIRV_OP(OpGroupNonUniformInverseBallot)
_SPIRV_OP(OpGroupNonUniformBallotBitExtract)
_SPIRV_OP(OpGroupNonUniformBallotBitCount)
_SPIRV_OP(OpGroupNonUniformBallotFindLSB)
_SPIRV_OP(OpGroupNonUniformBallotFindMSB)
_SPIRV_OP(OpGroupNonUniformShuffle)
_SPIRV_OP(OpGroupNonUniformShuffleXor)
_SPIRV_OP(OpGroupNonUniformShuffleUp)
_SPIRV_OP(OpGroupNonUniformShuffleDown)
_SPIRV_OP(OpGroupNonUniformIAdd)
_SPIRV_OP(OpGroupNonUniformFAdd)
_SPIRV_OP(OpGroupNonUniformIMul)
_SPIRV_OP(OpGroupNonUniformFMul)
_SPIRV_OP(OpGroupNonUniformSMin)
_SPIRV_OP(OpGroupNonUniformUMin)
_SPIRV_OP(OpGroupNonUniformFMin)
_SPIRV_OP(OpGroupNonUniformSMax)
_SPIRV_OP(OpGroupNonUniformUMax)
_SPIRV_OP(OpGroupNonUniformFMax)
_SPIRV_OP(OpGroupNonUniformBitwiseAnd)
_SPIRV_OP(OpGroupNonUniformBitwiseOr)
_SPIRV_OP(OpGroupNonUniformBitwiseXor)
_SPIRV_OP(OpGroupNonUniformLogicalAnd)
_SPIRV_OP(OpGroupNonUniformLogicalOr)
_SPIRV_OP(OpGroupNonUniformLogicalXor)
// SPV_KHR_uniform_group_instructions
_SPIRV_OP(OpGroupIMulKHR)
_SPIRV_OP(OpGroupFMulKHR)
_SPIRV_OP(OpGroupBitwiseAndKHR)
_SPIRV_OP(OpGroupBitwiseOrKHR)
_SPIRV_OP(OpGroupBitwiseXorKHR)
_SPIRV_OP(OpGroupLogicalAndKHR)
_SPIRV_OP(OpGroupLogicalOrKHR)
_SPIRV_OP(OpGroupLogicalXorKHR)
// CL image builtins
_SPIRV_OP(OpSampledImage)
_SPIRV_OP(OpImageSampleImplicitLod)
_SPIRV_OP(OpImageSampleExplicitLod)
_SPIRV_OP(OpImageRead)
_SPIRV_OP(OpImageWrite)
_SPIRV_OP(OpImageQuerySize)
_SPIRV_OP(OpImageQuerySizeLod)
_SPIRV_OP(OpImageQueryFormat)
_SPIRV_OP(OpImageQueryOrder)
_SPIRV_OP(OpImageQueryLevels)
_SPIRV_OP(OpImageQuerySamples)
// Conversion builtins
_SPIRV_OP(OpUConvert)
_SPIRV_OP(OpSConvert)
_SPIRV_OP(OpConvertFToU)
_SPIRV_OP(OpConvertFToS)
_SPIRV_OP(OpConvertUToF)
_SPIRV_OP(OpConvertSToF)
_SPIRV_OP(OpFConvert)
_SPIRV_OP(OpSatConvertUToS)
_SPIRV_OP(OpSatConvertSToU)
_SPIRV_OP(OpConvertFToBF16INTEL)
_SPIRV_OP(OpConvertBF16ToFINTEL)
// SPV_INTEL_arithmetic_fence
_SPIRV_OP(OpArithmeticFenceINTEL)
// Arithmetic Instructions
_SPIRV_OP(OpFRem)
_SPIRV_OP(OpFMod)
// Cast
_SPIRV_OP(OpGenericCastToPtrExplicit)
// Ballot extension
_SPIRV_OP(OpSubgroupBallotKHR)
_SPIRV_OP(OpSubgroupFirstInvocationKHR)
// Shader clock extension
_SPIRV_OP(OpReadClockKHR)
// Intel Subgroups builtins
_SPIRV_OP(OpSubgroupShuffleINTEL)
_SPIRV_OP(OpSubgroupShuffleDownINTEL)
_SPIRV_OP(OpSubgroupShuffleUpINTEL)
_SPIRV_OP(OpSubgroupShuffleXorINTEL)
_SPIRV_OP(OpSubgroupBlockReadINTEL)
_SPIRV_OP(OpSubgroupBlockWriteINTEL)
_SPIRV_OP(OpSubgroupImageBlockReadINTEL)
_SPIRV_OP(OpSubgroupImageBlockWriteINTEL)
// integer dot product
_SPIRV_OP(OpUDotKHR)
_SPIRV_OP(OpSDotKHR)
_SPIRV_OP(OpSUDotKHR)
_SPIRV_OP(OpUDotAccSatKHR)
_SPIRV_OP(OpSDotAccSatKHR)
_SPIRV_OP(OpSUDotAccSatKHR)
_SPIRV_OP(OpJointMatrixLoadINTEL)
_SPIRV_OP(OpJointMatrixStoreINTEL)
_SPIRV_OP(OpJointMatrixMadINTEL)
_SPIRV_OP(OpJointMatrixSUMadINTEL)
_SPIRV_OP(OpJointMatrixUSMadINTEL)
_SPIRV_OP(OpJointMatrixUUMadINTEL)
_SPIRV_OP(OpJointMatrixWorkItemLengthINTEL)
_SPIRV_OP(OpJointMatrixGetElementCoordINTEL)
// Intel media_block_io builtins
_SPIRV_OP(OpSubgroupImageMediaBlockReadINTEL)
_SPIRV_OP(OpSubgroupImageMediaBlockWriteINTEL)
// VME
_SPIRV_OP(OpVmeImageINTEL)
// Initialization phase functions
_SPIRV_OP(OpSubgroupAvcImeInitializeINTEL)
_SPIRV_OP(OpSubgroupAvcFmeInitializeINTEL)
_SPIRV_OP(OpSubgroupAvcBmeInitializeINTEL)
_SPIRV_OP(OpSubgroupAvcSicInitializeINTEL)
// Result and payload types conversion functions
_SPIRV_OP(OpSubgroupAvcMceConvertToImePayloadINTEL)
_SPIRV_OP(OpSubgroupAvcMceConvertToImeResultINTEL)
_SPIRV_OP(OpSubgroupAvcMceConvertToRefPayloadINTEL)
_SPIRV_OP(OpSubgroupAvcMceConvertToRefResultINTEL)
_SPIRV_OP(OpSubgroupAvcMceConvertToSicPayloadINTEL)
_SPIRV_OP(OpSubgroupAvcMceConvertToSicResultINTEL)
_SPIRV_OP(OpSubgroupAvcImeConvertToMcePayloadINTEL)
_SPIRV_OP(OpSubgroupAvcImeConvertToMceResultINTEL)
_SPIRV_OP(OpSubgroupAvcRefConvertToMcePayloadINTEL)
_SPIRV_OP(OpSubgroupAvcRefConvertToMceResultINTEL)
_SPIRV_OP(OpSubgroupAvcSicConvertToMcePayloadINTEL)
_SPIRV_OP(OpSubgroupAvcSicConvertToMceResultINTEL)
// MCE instructions
_SPIRV_OP(OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetInterShapePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetInterDirectionPenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetAcOnlyHaarINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL)
_SPIRV_OP(OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetMotionVectorsINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterDistortionsINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetBestInterDistortionsINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterMajorShapeINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterMinorShapeINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterDirectionsINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterMotionVectorCountINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterReferenceIdsINTEL)
_SPIRV_OP(OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL)
// IME instructions
_SPIRV_OP(OpSubgroupAvcImeSetSingleReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcImeSetDualReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcImeRefWindowSizeINTEL)
_SPIRV_OP(OpSubgroupAvcImeAdjustRefOffsetINTEL)
_SPIRV_OP(OpSubgroupAvcImeSetMaxMotionVectorCountINTEL)
_SPIRV_OP(OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL)
_SPIRV_OP(OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL)
_SPIRV_OP(OpSubgroupAvcImeSetWeightedSadINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithDualReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL)
_SPIRV_OP(OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetSingleReferenceStreaminINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetDualReferenceStreaminINTEL)
_SPIRV_OP(OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL)
_SPIRV_OP(OpSubgroupAvcImeStripDualReferenceStreamoutINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetBorderReachedINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL)
_SPIRV_OP(OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL)
// REF instructions
_SPIRV_OP(OpSubgroupAvcRefSetBidirectionalMixDisableINTEL)
_SPIRV_OP(OpSubgroupAvcRefSetBilinearFilterEnableINTEL)
_SPIRV_OP(OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcRefEvaluateWithDualReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL)
// SIC instructions
_SPIRV_OP(OpSubgroupAvcSicConfigureSkcINTEL)
_SPIRV_OP(OpSubgroupAvcSicConfigureIpeLumaINTEL)
_SPIRV_OP(OpSubgroupAvcSicConfigureIpeLumaChromaINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetMotionVectorMaskINTEL)
_SPIRV_OP(OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL)
_SPIRV_OP(OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL)
_SPIRV_OP(OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL)
_SPIRV_OP(OpSubgroupAvcSicSetBilinearFilterEnableINTEL)
_SPIRV_OP(OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL)
_SPIRV_OP(OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL)
_SPIRV_OP(OpSubgroupAvcSicEvaluateIpeINTEL)
_SPIRV_OP(OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcSicEvaluateWithDualReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL)
_SPIRV_OP(OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetIpeLumaShapeINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetPackedIpeLumaModesINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetIpeChromaModeINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL)
_SPIRV_OP(OpSubgroupAvcSicGetInterRawSadsINTEL)
#undef _SPIRV_OP
}
typedef SPIRVMap<Op, std::string, SPIRVInstruction>
  OCLSPIRVBuiltinMap;

template<> inline void
SPIRVMap<Attribute::AttrKind, SPIRVFuncParamAttrKind>::init() {
  add(Attribute::ZExt, FunctionParameterAttributeZext);
  add(Attribute::SExt, FunctionParameterAttributeSext);
  add(Attribute::ByVal, FunctionParameterAttributeByVal);
  add(Attribute::StructRet, FunctionParameterAttributeSret);
  add(Attribute::NoAlias, FunctionParameterAttributeNoAlias);
  add(Attribute::NoCapture, FunctionParameterAttributeNoCapture);
  add(Attribute::ReadOnly, FunctionParameterAttributeNoWrite);
  add(Attribute::ReadNone, FunctionParameterAttributeNoReadWrite);
}
typedef SPIRVMap<Attribute::AttrKind, SPIRVFuncParamAttrKind>
  SPIRSPIRVFuncParamAttrMap;

template<> inline void
SPIRVMap<Attribute::AttrKind, SPIRVFunctionControlMaskKind>::init() {
  add(Attribute::ReadNone, FunctionControlPureMask);
  add(Attribute::ReadOnly, FunctionControlConstMask);
  add(Attribute::AlwaysInline, FunctionControlInlineMask);
  add(Attribute::NoInline, FunctionControlDontInlineMask);
  add(Attribute::OptimizeNone, FunctionControlOptNoneINTELMask);
}
typedef SPIRVMap<Attribute::AttrKind, SPIRVFunctionControlMaskKind>
  SPIRSPIRVFuncCtlMaskMap;

template<> inline void
SPIRVMap<OCLMemFenceKind, MemorySemanticsMask>::init() {
  add(OCLMF_Local, MemorySemanticsWorkgroupLocalMemoryMask);
  add(OCLMF_Global, MemorySemanticsWorkgroupGlobalMemoryMask);
  add(OCLMF_Image, MemorySemanticsImageMemoryMask);
}
typedef SPIRVMap<OCLMemFenceKind, MemorySemanticsMask>
  OCLMemFenceMap;

template<> inline void
SPIRVMap<OCLMemOrderKind, unsigned, MemorySemanticsMask>::init() {
  add(OCLMO_relaxed, MemorySemanticsMaskNone); //old MemorySemanticsRelaxedMask
  add(OCLMO_acquire, MemorySemanticsAcquireMask);
  add(OCLMO_release, MemorySemanticsReleaseMask);
  add(OCLMO_acq_rel, MemorySemanticsAcquireMask|MemorySemanticsReleaseMask);
  add(OCLMO_seq_cst, MemorySemanticsSequentiallyConsistentMask);
}
typedef SPIRVMap<OCLMemOrderKind, unsigned, MemorySemanticsMask>
  OCLMemOrderMap;

inline unsigned mapOCLMemSemanticToSPIRV(unsigned MemFenceFlag,
    OCLMemOrderKind Order) {
  return OCLMemOrderMap::map(Order) |
      mapBitMask<OCLMemFenceMap>(MemFenceFlag);
}

inline std::pair<unsigned, OCLMemOrderKind>
mapSPIRVMemSemanticToOCL(unsigned Sema) {
  return std::make_pair(rmapBitMask<OCLMemFenceMap>(Sema),
    OCLMemOrderMap::rmap(Sema & 0xF));
}

inline OCLMemOrderKind
mapSPIRVMemOrderToOCL(unsigned Sema) {
  return OCLMemOrderMap::rmap(Sema & 0xF);
}

template<> inline void
SPIRVMap<OCLMemScopeKind, Scope>::init() {
  add(OCLMS_work_item, ScopeInvocation);
  add(OCLMS_work_group, ScopeWorkgroup);
  add(OCLMS_device, ScopeDevice);
  add(OCLMS_all_svm_devices, ScopeCrossDevice);
  add(OCLMS_sub_group, ScopeSubgroup);
}
typedef SPIRVMap<OCLMemScopeKind, Scope>
  OCLMemScopeMap;

template<> inline void
SPIRVMap<std::string, SPIRVGroupOperationKind>::init() {
#define _SPIRV_OP(x,y) add(#x, igc_spv::GroupOperation::y);
_SPIRV_OP(reduce, GroupOperationReduce)
_SPIRV_OP(scan_inclusive, GroupOperationInclusiveScan)
_SPIRV_OP(scan_exclusive, GroupOperationExclusiveScan)
_SPIRV_OP(clustered_reduce, GroupOperationClusteredReduce)
#undef _SPIRV_OP
}
typedef SPIRVMap<std::string, SPIRVGroupOperationKind>
  SPIRSPIRVGroupOperationMap;


template<> inline void
SPIRVMap<std::string, SPIRVFPRoundingModeKind>::init() {
#define _SPIRV_OP(x,y) add("rt"#x, SPIRVFPRoundingModeKind::y);
_SPIRV_OP(e, FPRoundingModeRTE)
_SPIRV_OP(z, FPRoundingModeRTZ)
_SPIRV_OP(p, FPRoundingModeRTP)
_SPIRV_OP(n, FPRoundingModeRTN)
#undef _SPIRV_OP
}
typedef SPIRVMap<std::string, SPIRVFPRoundingModeKind>
  SPIRSPIRVFPRoundingModeMap;

template<> inline void
SPIRVMap<SPIRVTypeImageDescriptor, std::string>::init() {
    add({ Dim1D, 0, 0, 0, 0, 0 }, "img1d");
    add({ DimBuffer, 0, 0, 0, 0, 0 }, "img1d_buffer");
    add({ Dim1D,     0, 1, 0, 0, 0 }, "img1d_array");
    add({ Dim2D,     0, 0, 0, 0, 0 }, "img2d");
    add({ Dim2D,     0, 1, 0, 0, 0 }, "img2d_array");
    add({ Dim2D,     1, 0, 0, 0, 0 }, "img2d_depth");
    add({ Dim2D,     1, 1, 0, 0, 0 }, "img2d_array_depth");
    add({ Dim2D,     0, 0, 1, 0, 0 }, "img2d_msaa");
    add({ Dim2D,     0, 1, 1, 0, 0 }, "img2d_array_msaa");
    add({ Dim2D,     1, 0, 1, 0, 0 }, "img2d_msaa_depth");
    add({ Dim2D,     1, 1, 1, 0, 0 }, "img2d_array_msaa_depth");
    add({ Dim3D,     0, 0, 0, 0, 0 }, "img3d");
}
typedef SPIRVMap<SPIRVTypeImageDescriptor, std::string>
SPIRVImageManglingMap;

#define SPIR_MD_KERNELS                     "opencl.kernels"
#define SPIR_MD_KERNEL_ARG_ADDR_SPACE       "kernel_arg_addr_space"
#define SPIR_MD_KERNEL_ARG_ACCESS_QUAL      "kernel_arg_access_qual"
#define SPIR_MD_KERNEL_ARG_TYPE             "kernel_arg_type"
#define SPIR_MD_KERNEL_ARG_BASE_TYPE        "kernel_arg_base_type"
#define SPIR_MD_KERNEL_ARG_TYPE_QUAL        "kernel_arg_type_qual"
#define SPIR_MD_KERNEL_ARG_NAME             "kernel_arg_name"

namespace kLLVMName {
  const static char builtinPrefix[] = "__builtin_spirv_";
  const static char builtinExtInstPrefixOpenCL[] = "__builtin_spirv_OpenCL_";
}

namespace kSPIRVImageSampledTypeName {
  const static char Float[] = "float";
  const static char Half[] = "half";
  const static char Int[] = "int";
  const static char UInt[] = "uint";
  const static char Void[] = "void";
}

namespace kSPIRVTypeName {
  const static char Delimiter   = '.';
  const static char SampledImage[] = "SampledImage";
  const static char Sampler[] = "Sampler";
  const static char Image[] = "Image";
  const static char Pipe[] = "Pipe";
  const static char ReserveId[] = "ReserveId";
  const static char VmeImageINTEL[] = "VmeImageINTEL";
  const static char PrefixAndDelim[] = "spirv.";
  const static char PostfixDelim = '_';
}

namespace kSPR2TypeName {
  const static char Delimiter   = '.';
  const static char OCLPrefix[]   = "opencl.";
  const static char ImagePrefix[] = "opencl.image";
  const static char Sampler[]     = "opencl.sampler_t";
  const static char Event[]       = "opencl.event_t";
}

namespace kAccessQualName {
  const static char ReadOnly[]    = "read_only";
  const static char WriteOnly[]   = "write_only";
  const static char ReadWrite[]   = "read_write";
}

namespace kSPIRVMD {
  const static char Capability[]        = "spirv.Capability";
  const static char Extension[]         = "spirv.Extension";
  const static char Source[]            = "spirv.Source";
  const static char SourceExtension[]   = "spirv.SourceExtension";
  const static char MemoryModel[]       = "spirv.MemoryModel";
  const static char EntryPoint[]        = "spirv.EntryPoint";
  const static char ExecutionMode[]     = "spirv.ExecutionMode";
}

namespace kSPIR2MD {
  const static char Extensions[]        = "opencl.used.extensions";
  const static char FPContract[]        = "opencl.enable.FP_CONTRACT";
  const static char OCLVer[]            = "opencl.ocl.version";
  const static char OptFeatures[]       = "opencl.used.optional.core.features";
  const static char SPIRVer[]           = "opencl.spir.version";
  const static char VecTyHint[]         = "vec_type_hint";
  const static char WGSize[]            = "reqd_work_group_size";
  const static char ReqdSubgroupSize[]  = "intel_reqd_sub_group_size";
  const static char WGSizeHint[]        = "work_group_size_hint";
}

enum Spir2SamplerKind {
  CLK_ADDRESS_NONE            = 0x0000,
  CLK_ADDRESS_CLAMP           = 0x0004,
  CLK_ADDRESS_CLAMP_TO_EDGE   = 0x0002,
  CLK_ADDRESS_REPEAT          = 0x0006,
  CLK_ADDRESS_MIRRORED_REPEAT = 0x0008,
  CLK_NORMALIZED_COORDS_FALSE = 0x0000,
  CLK_NORMALIZED_COORDS_TRUE  = 0x0001,
  CLK_FILTER_NEAREST          = 0x0010,
  CLK_FILTER_LINEAR           = 0x0020,
};

namespace OclExt {
enum Kind {
#define _SPIRV_OP(x) x,
  _SPIRV_OP(cl_images)
  _SPIRV_OP(cl_doubles)
  _SPIRV_OP(cl_khr_int64_base_atomics)
  _SPIRV_OP(cl_khr_int64_extended_atomics)
  _SPIRV_OP(cl_khr_fp16)
  _SPIRV_OP(cl_khr_gl_sharing)
  _SPIRV_OP(cl_khr_gl_event)
  _SPIRV_OP(cl_khr_d3d10_sharing)
  _SPIRV_OP(cl_khr_media_sharing)
  _SPIRV_OP(cl_khr_d3d11_sharing)
  _SPIRV_OP(cl_khr_global_int32_base_atomics)
  _SPIRV_OP(cl_khr_global_int32_extended_atomics)
  _SPIRV_OP(cl_khr_local_int32_base_atomics)
  _SPIRV_OP(cl_khr_local_int32_extended_atomics)
  _SPIRV_OP(cl_khr_byte_addressable_store)
  _SPIRV_OP(cl_khr_3d_image_writes)
  _SPIRV_OP(cl_khr_gl_msaa_sharing)
  _SPIRV_OP(cl_khr_depth_images)
  _SPIRV_OP(cl_khr_gl_depth_images)
  _SPIRV_OP(cl_khr_subgroups)
  _SPIRV_OP(cl_khr_mipmap_image)
  _SPIRV_OP(cl_khr_mipmap_image_writes)
  _SPIRV_OP(cl_khr_egl_event)
  _SPIRV_OP(cl_khr_srgb_image_writes)
#undef _SPIRV_OP
};
}

template<> inline void
SPIRVMap<OclExt::Kind, std::string>::init() {
#define _SPIRV_OP(x) add(OclExt::x, #x);
  _SPIRV_OP(cl_images)
  _SPIRV_OP(cl_doubles)
  _SPIRV_OP(cl_khr_int64_base_atomics)
  _SPIRV_OP(cl_khr_int64_extended_atomics)
  _SPIRV_OP(cl_khr_fp16)
  _SPIRV_OP(cl_khr_gl_sharing)
  _SPIRV_OP(cl_khr_gl_event)
  _SPIRV_OP(cl_khr_d3d10_sharing)
  _SPIRV_OP(cl_khr_media_sharing)
  _SPIRV_OP(cl_khr_d3d11_sharing)
  _SPIRV_OP(cl_khr_global_int32_base_atomics)
  _SPIRV_OP(cl_khr_global_int32_extended_atomics)
  _SPIRV_OP(cl_khr_local_int32_base_atomics)
  _SPIRV_OP(cl_khr_local_int32_extended_atomics)
  _SPIRV_OP(cl_khr_byte_addressable_store)
  _SPIRV_OP(cl_khr_3d_image_writes)
  _SPIRV_OP(cl_khr_gl_msaa_sharing)
  _SPIRV_OP(cl_khr_depth_images)
  _SPIRV_OP(cl_khr_gl_depth_images)
  _SPIRV_OP(cl_khr_subgroups)
  _SPIRV_OP(cl_khr_mipmap_image)
  _SPIRV_OP(cl_khr_mipmap_image_writes)
  _SPIRV_OP(cl_khr_egl_event)
  _SPIRV_OP(cl_khr_srgb_image_writes)
#undef _SPIRV_OP
};

template<> inline void
SPIRVMap<OclExt::Kind, SPIRVCapabilityKind>::init() {
#define _SPIRV_OP(x,y) add(OclExt::x, SPIRVCapabilityKind::y);
  _SPIRV_OP(cl_images, CapabilityImageBasic)
  _SPIRV_OP(cl_doubles, CapabilityFloat64)
  _SPIRV_OP(cl_khr_int64_base_atomics, CapabilityInt64Atomics)
  _SPIRV_OP(cl_khr_int64_extended_atomics, CapabilityInt64Atomics)
  _SPIRV_OP(cl_khr_fp16, CapabilityFloat16)
  _SPIRV_OP(cl_khr_gl_sharing, CapabilityNone)
  _SPIRV_OP(cl_khr_gl_event, CapabilityNone)
  _SPIRV_OP(cl_khr_d3d10_sharing, CapabilityNone)
  _SPIRV_OP(cl_khr_media_sharing, CapabilityNone)
  _SPIRV_OP(cl_khr_d3d11_sharing, CapabilityNone)
  _SPIRV_OP(cl_khr_global_int32_base_atomics, CapabilityNone)
  _SPIRV_OP(cl_khr_global_int32_extended_atomics, CapabilityNone)
  _SPIRV_OP(cl_khr_local_int32_base_atomics, CapabilityNone)
  _SPIRV_OP(cl_khr_local_int32_extended_atomics, CapabilityNone)
  _SPIRV_OP(cl_khr_byte_addressable_store, CapabilityNone)
  _SPIRV_OP(cl_khr_3d_image_writes, CapabilityImageReadWrite)
  _SPIRV_OP(cl_khr_gl_msaa_sharing, CapabilityNone)
  _SPIRV_OP(cl_khr_depth_images, CapabilityNone)
  _SPIRV_OP(cl_khr_gl_depth_images, CapabilityNone)
  _SPIRV_OP(cl_khr_subgroups, CapabilityGroups)
  _SPIRV_OP(cl_khr_mipmap_image, CapabilityImageMipmap)
  _SPIRV_OP(cl_khr_mipmap_image_writes, CapabilityImageMipmap)
  _SPIRV_OP(cl_khr_egl_event, CapabilityNone)
  _SPIRV_OP(cl_khr_srgb_image_writes, CapabilityImageSRGBWrite)
#undef _SPIRV_OP
};

/// \returns a vector of types for a collection of values.
template<class T>
std::vector<Type *>
getTypes(T V) {
  std::vector<Type *> Tys;
  for (auto &I:V)
    Tys.push_back(I->getType());
  return Tys;
}

void saveLLVMModule(Module *M, const std::string &OutputFile);

PointerType *getOrCreateOpaquePtrType(Module *M, const std::string &Name,
    unsigned AddrSpace = SPIRAS_Global);
void getFunctionTypeParameterTypes(llvm::FunctionType* FT,
    std::vector<Type*>& ArgTys);
Function *getOrCreateFunction(Module *M, Type *RetTy,
    ArrayRef<Type *> ArgTypes, StringRef Name, bool Mangle = false,
    AttributeList *Attrs = nullptr, bool takeName = true);
std::vector<Value *> getArguments(CallInst* CI);

void decorateSPIRVBuiltin(std::string &S);
void decorateSPIRVBuiltin(std::string &S, std::vector<Type*> ArgTypes);
void decorateSPIRVExtInst(std::string &S, std::vector<Type*> ArgTypes);

/// Check if a builtin function
bool isFunctionBuiltin(llvm::Function* F);

/// Get a canonical function name for a SPIR-V op code.
std::string getSPIRVBuiltinName(Op OC, SPIRVInstruction *BI, std::vector<Type*> ArgTypes, std::string suffix);

/// Mutates function call instruction by changing the arguments.
/// \param ArgMutate mutates the function arguments.
/// \return mutated call instruction.
CallInst *mutateCallInst(Module *M, CallInst *CI,
    std::function<std::string (CallInst *, std::vector<Value *> &)>ArgMutate,
    bool Mangle = false, AttributeList *Attrs = nullptr, bool takeName = true);

/// Mutate function by change the arguments.
/// \param ArgMutate mutates the function arguments.
void mutateFunction(Function *F,
    std::function<std::string (CallInst *, std::vector<Value *> &)>ArgMutate,
    bool Builtin , AttributeList *Attrs = nullptr, bool takeName = true);

/// Add a call instruction at \p Pos.
CallInst *addCallInst(Module *M, StringRef FuncName, Type *RetTy,
    ArrayRef<Value *> Args, AttributeList *Attrs, Instruction *Pos,
    bool Builtin, StringRef InstName, bool TakeFuncName = true);

/// Get a 64 bit integer constant.
ConstantInt *getInt64(Module *M, int64_t value);

/// Get a 32 bit integer constant.
ConstantInt *getInt32(Module *M, int value);

/// Decode OpenCL version which is encoded as Major*10^5+Minor*10^3+Rev
std::tuple<unsigned short, unsigned char, unsigned char>
decodeOCLVer(unsigned Ver);

/// Get SPIR-V type name as spirv.BaseTyName.Postfixes.
std::string getSPIRVTypeName(StringRef BaseTyName, StringRef Postfixes = "");

/// Get the postfixes of SPIR-V image type name as in spirv.Image.postfixes.
std::string getSPIRVImageTypePostfixes(StringRef SampledType,
    SPIRVTypeImageDescriptor Desc,
    SPIRVAccessQualifierKind Acc);

/// Get the sampled type name used in postfix of image type in SPIR-V
/// friendly LLVM IR.
std::string getSPIRVImageSampledTypeName(SPIRVType* Ty);

/// Check if a type is SPIRV sampler type.
bool isSPIRVSamplerType(llvm::Type* Ty);

}

#endif

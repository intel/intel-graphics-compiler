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

// This file defines SPIR-V enums.

#ifndef SPIRVENUM_HPP_
#define SPIRVENUM_HPP_

#include "spirv.hpp"
#include "SPIRVOpCode.h"
#include <cstdint>


namespace igc_spv{

typedef uint32_t SPIRVWord;
typedef uint32_t SPIRVId;
#define SPIRVID_MAX       ~0U
#define SPIRVID_INVALID   ~0U
#define SPIRVWORD_MAX     ~0U

inline bool
isValid(SPIRVId Id) { return Id != SPIRVID_INVALID;}

inline SPIRVWord
mkWord(unsigned WordCount, Op OpCode) {
  return (WordCount << 16) | OpCode;
}

const SPIRVWord SPIRVMagicNumber = igc_spv::MagicNumber;

enum SPIRVVersionSupported {
   fullyCompliant = igc_spv::Version
};


enum SPIRVGeneratorKind {
  SPIRVGEN_AMDOpenSourceLLVMSPIRVTranslator = 1,
};

enum SPIRVInstructionSchemaKind {
  SPIRVISCH_Default,
};

typedef igc_spv::Capability SPIRVCapabilityKind;
typedef igc_spv::ExecutionModel SPIRVExecutionModelKind;
typedef igc_spv::ExecutionMode SPIRVExecutionModeKind;
typedef igc_spv::AccessQualifier SPIRVAccessQualifierKind;
typedef igc_spv::AddressingModel SPIRVAddressingModelKind;
typedef igc_spv::LinkageType SPIRVLinkageTypeKind;
typedef igc_spv::MemoryModel SPIRVMemoryModelKind;
typedef igc_spv::StorageClass SPIRVStorageClassKind;
typedef igc_spv::FunctionControlMask SPIRVFunctionControlMaskKind;
typedef igc_spv::FPRoundingMode SPIRVFPRoundingModeKind;
typedef igc_spv::FunctionParameterAttribute SPIRVFuncParamAttrKind;
typedef igc_spv::BuiltIn SPIRVBuiltinVariableKind;
typedef igc_spv::MemoryAccessMask SPIRVMemoryAccessKind;
typedef igc_spv::GroupOperation SPIRVGroupOperationKind;
typedef igc_spv::Dim SPIRVImageDimKind;

template<typename K>
SPIRVCapabilityKind
getCapability(K Key) {
  return SPIRVMap<K, SPIRVCapabilityKind>::map(Key);
}

template<> inline void
SPIRVMap<SPIRVExecutionModelKind, SPIRVCapabilityKind>::init() {
  add(ExecutionModelVertex, CapabilityShader);
  add(ExecutionModelTessellationControl, CapabilityTessellation);
  add(ExecutionModelTessellationEvaluation, CapabilityShader);
  add(ExecutionModelGeometry, CapabilityGeometry);
  add(ExecutionModelFragment, CapabilityShader);
  add(ExecutionModelGLCompute, CapabilityShader);
  add(ExecutionModelKernel, CapabilityKernel);
}

inline bool
isValid(SPIRVExecutionModelKind E) {
  return (unsigned)E < (unsigned)ExecutionModelCount;
}

template<> inline void
SPIRVMap<SPIRVExecutionModeKind, SPIRVCapabilityKind>::init() {
  add(ExecutionModeInvocations, CapabilityGeometry);
  add(ExecutionModeSpacingEqual, CapabilityTessellation);
  add(ExecutionModeSpacingFractionalEven, CapabilityTessellation);
  add(ExecutionModeSpacingFractionalOdd, CapabilityTessellation);
  add(ExecutionModeVertexOrderCw, CapabilityTessellation);
  add(ExecutionModeVertexOrderCcw, CapabilityTessellation);
  add(ExecutionModePixelCenterInteger, CapabilityShader);
  add(ExecutionModeOriginUpperLeft, CapabilityShader);
  add(ExecutionModeOriginLowerLeft, CapabilityShader);
  add(ExecutionModeEarlyFragmentTests, CapabilityShader);
  add(ExecutionModePointMode, CapabilityTessellation);
  add(ExecutionModeXfb, CapabilityShader);
  add(ExecutionModeDepthReplacing, CapabilityShader);
  add(ExecutionModeDepthGreater, CapabilityShader);
  add(ExecutionModeDepthLess, CapabilityShader);
  add(ExecutionModeDepthUnchanged, CapabilityShader);
  add(ExecutionModeLocalSize, CapabilityNone);
  add(ExecutionModeLocalSizeHint, CapabilityKernel);
  add(ExecutionModeInputPoints, CapabilityGeometry);
  add(ExecutionModeInputLines, CapabilityGeometry);
  add(ExecutionModeInputLinesAdjacency, CapabilityGeometry);
  add(ExecutionModeInputTriangles, CapabilityTessellation);
  add(ExecutionModeInputTrianglesAdjacency, CapabilityGeometry);
  add(ExecutionModeInputQuads, CapabilityGeometry);
  add(ExecutionModeInputIsolines, CapabilityGeometry);
  add(ExecutionModeOutputVertices, CapabilityTessellation);
  add(ExecutionModeOutputPoints, CapabilityGeometry);
  add(ExecutionModeOutputLineStrip, CapabilityGeometry);
  add(ExecutionModeOutputTriangleStrip, CapabilityGeometry);
  add(ExecutionModeVecTypeHint, CapabilityKernel);
  add(ExecutionModeContractionOff, CapabilityKernel);
  add(ExecutionModeInitializer, CapabilityKernel);
  add(ExecutionModeFinalizer, CapabilityKernel);
  add(ExecutionModeSubgroupSize, CapabilitySubgroupDispatch);
  add(ExecutionModeSubgroupsPerWorkgroup, CapabilitySubgroupDispatch);
}

inline bool
isValid(SPIRVExecutionModeKind E) {
  return (unsigned)E < (unsigned)ExecutionModeCount;
}

inline bool
isValid(SPIRVLinkageTypeKind L) {
  return (unsigned)L < (unsigned)LinkageTypeCount;
}

template<> inline void
SPIRVMap<SPIRVStorageClassKind, SPIRVCapabilityKind>::init() {
  add(StorageClassUniformConstant, CapabilityNone);
  add(StorageClassInput, CapabilityShader);
  add(StorageClassUniform, CapabilityShader);
  add(StorageClassOutput, CapabilityShader);
  add(StorageClassWorkgroupLocal, CapabilityNone);
  add(StorageClassWorkgroupGlobal, CapabilityNone);
  add(StorageClassPrivateGlobal, CapabilityShader);
  add(StorageClassFunction, CapabilityShader);
  add(StorageClassGeneric, CapabilityKernel);
  add(StorageClassAtomicCounter, CapabilityShader);
}

inline bool
isValid(SPIRVStorageClassKind StorageClass) {
  return (unsigned)StorageClass < (unsigned)StorageClassCount;
}

inline bool
isValid(SPIRVFuncParamAttrKind FPA) {
  return (unsigned)FPA < (unsigned)FunctionParameterAttributeCount;
}

enum SPIRVExtInstSetKind {
  SPIRVEIS_OpenCL,
  SPIRVEIS_DebugInfo,
  SPIRVEIS_OpenCL_DebugInfo_100,
  SPIRVEIS_Count,
};

inline bool
isValid(SPIRVExtInstSetKind BIS) {
  return (unsigned)BIS < (unsigned)SPIRVEIS_Count;
}

template<> inline void
SPIRVMap<SPIRVExtInstSetKind, std::string>::init() {
  add(SPIRVEIS_OpenCL, "OpenCL.std");
  add(SPIRVEIS_DebugInfo, "SPIRV.debug");
  add(SPIRVEIS_OpenCL_DebugInfo_100, "OpenCL.DebugInfo.100");
}
typedef SPIRVMap<SPIRVExtInstSetKind, std::string> SPIRVBuiltinSetNameMap;

inline bool
isValid(SPIRVBuiltinVariableKind Kind) {
  return (unsigned)Kind < (unsigned)BuiltInCount;
}

inline bool isValid(Scope Kind) {
  return (unsigned)Kind <= (unsigned)ScopeInvocation;
}

inline bool isValidSPIRVMemSemanticsMask(SPIRVWord MemMask) {
  return MemMask < 1 << ((unsigned)MemorySemanticsImageMemoryShift + 1);
}

//enum SPIRVSamplerAddressingModeKind
typedef igc_spv::SamplerAddressingMode SPIRVSamplerAddressingModeKind;

//enum SPIRVSamplerFilterModeKind
typedef igc_spv::SamplerFilterMode SPIRVSamplerFilterModeKind;

inline bool isValid(SPIRVGroupOperationKind G) {
  return (unsigned)G < (unsigned)GroupOperationCount;
}

inline unsigned getImageDimension(SPIRVImageDimKind K) {
  switch(K){
  case Dim1D:      return 1;
  case Dim2D:      return 2;
  case Dim3D:      return 3;
  case DimCube:    return 2;
  case DimRect:    return 2;
  case DimBuffer:  return 1;
  default:              return 0;
  }
}

}


#endif /* SPIRVENUM_HPP_ */

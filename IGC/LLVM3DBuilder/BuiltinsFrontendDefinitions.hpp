/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef BUILTINS_FRONTEND_DEFINITIONS_HPP
#define BUILTINS_FRONTEND_DEFINITIONS_HPP

#include "common/debug/DebugMacros.hpp" // VALUE_NAME() definition.
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/AsmParser/Parser.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "visa/include/visa_igc_common_header.h"
#include "IGC/common/ResourceAddrSpace.h"

enum class ADDRESS_SPACE_TYPE : unsigned int {
  ADDRESS_SPACE_PRIVATE = 0,
  ADDRESS_SPACE_GLOBAL = 1,
  ADDRESS_SPACE_CONSTANT = 2,
  ADDRESS_SPACE_LOCAL = 3,
  ADDRESS_SPACE_GENERIC = 4,
  ADDRESS_SPACE_LOCAL_32 = 13,
};

template <typename T, typename Inserter>
unsigned LLVM3DBuilder<T, Inserter>::EncodeASForGFXResource(const llvm::Value &bufIdx, IGC::BufferType bufType,
                                                            unsigned uniqueIndAS,
                                                            IGC::ResourceDimType resourceDimTypeId) {
  return IGC::EncodeAS4GFXResource(
      bufIdx, bufType, uniqueIndAS, false,
      resourceDimTypeId.value_or(IGC::RESOURCE_DIMENSION_TYPE::NUM_RESOURCE_DIMENSION_TYPES));
}

template <typename T, typename Inserter> inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_ubfe() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_ubfe =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ubfe);
  return func_llvm_GenISA_ubfe;
}

template <typename T, typename Inserter> inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_ibfe() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_ibfe =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ibfe);
  return func_llvm_GenISA_ibfe;
}

template <typename T, typename Inserter> inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_bfi() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_bfi =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_bfi);
  return func_llvm_GenISA_bfi;
}

template <typename T, typename Inserter> inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_bfrev() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_bfrev =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_bfrev);
  return func_llvm_GenISA_bfrev;
}

template <typename T, typename Inserter>
inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_firstbitHi() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_firstbitHi =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_firstbitHi);
  return func_llvm_GenISA_firstbitHi;
}

template <typename T, typename Inserter>
inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_firstbitLo() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_firstbitLo =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_firstbitLo);
  return func_llvm_GenISA_firstbitLo;
}

template <typename T, typename Inserter>
inline llvm::Function *LLVM3DBuilder<T, Inserter>::llvm_GenISA_firstbitShi() const {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_firstbitShi =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_firstbitShi);
  return func_llvm_GenISA_firstbitShi;
}

template <typename T, typename Inserter> void LLVM3DBuilder<T, Inserter>::Init() {
  // Cached constants
  m_int0 = this->getInt32(0);
  m_int1 = this->getInt32(1);
  m_int2 = this->getInt32(2);
  m_int3 = this->getInt32(3);
  m_float0 = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(this->getFloatTy(), 0.0));
  m_float1 = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(this->getFloatTy(), 1.0));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_resinfo(llvm::Value *int32_src_s_mip,
                                                               llvm::Value *int32_textureIdx) {
  llvm::Value *packed_params[] = {
      int32_textureIdx,
      int32_src_s_mip,
  };

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_llvm_GenISA_resinfoptr = llvm::GenISAIntrinsic::getDeclaration(
      module, llvm::GenISAIntrinsic::GenISA_resinfoptr, int32_textureIdx->getType());

  llvm::CallInst *packed_resinfo_call = this->CreateCall(func_llvm_GenISA_resinfoptr, packed_params);
  return packed_resinfo_call;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_resinfoptr_msaa(llvm::Value *srcBuffer,
                                                                       llvm::Value *float_src_s_mip) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *func_resinfoptr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_resinfoptr, srcBuffer->getType());

  //%mip_s = bitcast float %float_src_s_mip to i32
  llvm::Value *int32_mip = this->CreateBitCast(float_src_s_mip, this->getInt32Ty(), VALUE_NAME("mip_s"));

  llvm::Value *packed_params[] = {srcBuffer, int32_mip};

  llvm::CallInst *packed_resinfo_call = llvm::cast<llvm::CallInst>(this->CreateCall(func_resinfoptr, packed_params));

  // %tex_s.chan0 = extractelement <4 x i32> %packed_resinfo_call, i32 2
  llvm::Value *int32_info_s_ch2 = this->CreateExtractElement(packed_resinfo_call, this->m_int2);

  llvm::Function *func_sampleinfoptr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleinfoptr, srcBuffer->getType());

  llvm::Value *packed_sampleinfo_params[] = {srcBuffer};

  // Call sampleinfoptr intrinsic to get the number of samples.
  llvm::CallInst *packed_sampleinfo_call =
      llvm::cast<llvm::CallInst>(this->CreateCall(func_sampleinfoptr, packed_sampleinfo_params));

  // We can not use channel 0 of sampleinfo which should contain the correct
  // number of samples retrieved from surface state because this value in surface
  // state must be set to 1 in case of MSAA UAV emulation due to fact that
  // IGC does not support native MSAA UAV messages at the moment.
  // Instead of channel 0 we can use channel 3 of sampleinfo which contains
  // sample position palette index field retrieved from surface state.
  // The sample position palette index field is set to log2(number of samples).

  // Get sample position palette index from sampleinfo. Note that this value
  // is incremented by one from its value in the surface state.
  llvm::Value *int32_sampleinfo_s_chan3 = this->CreateExtractElement(packed_sampleinfo_call, this->m_int3);
  llvm::Value *int32_paletteIndex = this->CreateSub(int32_sampleinfo_s_chan3, this->m_int1);

  // Number of samples = 2 ^ "sample position palette index".
  llvm::Value *int32_numberOfSamples = this->CreateShl(this->m_int1, int32_paletteIndex);

  // Divide depth by number of samples.
  // %depth_s = udiv i32 %src_s.chan2, %src1_s_ch0
  llvm::Value *int32_depth = this->CreateUDiv(int32_info_s_ch2, int32_numberOfSamples, VALUE_NAME("depth_s"));

  llvm::Value *resinfo =
      llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(llvm::Type::getInt32Ty(module->getContext()), 4));

  resinfo = this->CreateInsertElement(resinfo, this->CreateExtractElement(packed_resinfo_call, this->m_int0),
                                      this->getInt32(0), "call_inst");

  resinfo = this->CreateInsertElement(resinfo, this->CreateExtractElement(packed_resinfo_call, this->m_int1),
                                      this->getInt32(1), "call_inst");

  resinfo = this->CreateInsertElement(resinfo, this->CreateExtractElement(packed_resinfo_call, this->m_int3),
                                      this->getInt32(3), "call_inst");

  resinfo = this->CreateInsertElement(resinfo, int32_depth, this->getInt32(2), "call_inst");

  return resinfo;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedwrite(llvm::Value *dstBuffer, llvm::Value *srcAddressU,
                                                                  llvm::Value *srcAddressV, llvm::Value *srcAddressW,
                                                                  llvm::Value *lod, llvm::Value *float_X,
                                                                  llvm::Value *float_Y, llvm::Value *float_Z,
                                                                  llvm::Value *float_W) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pFuncTypedWrite =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedwrite, dstBuffer->getType());

  // R = SampleIndex
  llvm::Value *args[] = {
      dstBuffer, srcAddressU, srcAddressV, srcAddressW, lod, float_X, float_Y, float_Z, float_W,
  };

  llvm::Value *typedwrite = this->CreateCall(pFuncTypedWrite, args);
  return typedwrite;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedread(llvm::Value *srcBuffer, llvm::Value *srcAddressU,
                                                                 llvm::Value *srcAddressV, llvm::Value *srcAddressW,
                                                                 llvm::Value *lod) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pFuncTypedRead =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedread, srcBuffer->getType());

  llvm::Value *args[] = {srcBuffer, srcAddressU, srcAddressV, srcAddressW, lod};

  llvm::Value *typedread = this->CreateCall(pFuncTypedRead, args);
  return typedread;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedwriteMS(llvm::Value *dstBuffer, llvm::Value *srcAddressU,
                                                                    llvm::Value *srcAddressV, llvm::Value *srcAddressW,
                                                                    llvm::Value *sampleIdx, llvm::Value *float_X,
                                                                    llvm::Value *float_Y, llvm::Value *float_Z,
                                                                    llvm::Value *float_W) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pFuncTypedWriteMS =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedwriteMS, dstBuffer->getType());

  llvm::Value *args[] = {
      dstBuffer, srcAddressU, srcAddressV, srcAddressW, sampleIdx, float_X, float_Y, float_Z, float_W,
  };

  llvm::Value *typedwrite = this->CreateCall(pFuncTypedWriteMS, args);
  return typedwrite;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedreadMS(llvm::Value *srcBuffer, llvm::Value *srcAddressU,
                                                                   llvm::Value *srcAddressV, llvm::Value *srcAddressW,
                                                                   llvm::Value *sampleIdx) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pFuncTypedReadMS =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedreadMS, srcBuffer->getType());

  llvm::Value *args[] = {srcBuffer, srcAddressU, srcAddressV, srcAddressW, sampleIdx};

  llvm::Value *typedread = this->CreateCall(pFuncTypedReadMS, args);
  return typedread;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedread_msaa2D(llvm::Value *srcBuffer, llvm::Value *sampleIdx,
                                                                        llvm::Value *srcAddressU,
                                                                        llvm::Value *srcAddressV, llvm::Value *lod) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pFuncTypedRead =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedread, srcBuffer->getType());

  // R = SampleIndex
  llvm::Value *args[] = {srcBuffer, srcAddressU, srcAddressV, sampleIdx, lod};

  llvm::Value *typedread = this->CreateCall(pFuncTypedRead, args);
  return typedread;
}

template <typename T, typename Inserter>
inline llvm::Value *
LLVM3DBuilder<T, Inserter>::Create_typedread_msaa2DArray(llvm::Value *srcBuffer, llvm::Value *sampleIdx,
                                                         llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                                         llvm::Value *srcAddressR, llvm::Value *lod) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  // Call sampleinfoptr intrinsic to get the number of samples.
  // %tex = call <4 x i32> @llvm.GenISA.sampleinfoptr(4x(float)addrspace())
  llvm::Function *pfuncsampleinfoptr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleinfoptr, srcBuffer->getType());
  llvm::Value *packed_sampleinfo_call = this->CreateCall(pfuncsampleinfoptr, srcBuffer);

  // We can not use channel 0 of sampleinfo which should contain the correct
  // number of samples retrieved from surface state because this value in surface
  // state must be set to 1 in case of MSAA UAV emulation due to fact that
  // IGC does not support native MSAA UAV messages at the moment.
  // Instead of channel 0 we can use channel 3 of sampleinfo which contains
  // sample position palette index field retrieved from surface state.
  // The sample position palette index field is set to log2(number of samples).

  // Get sample position palette index from surface state. Note that this value
  // is incremented by one from its value in the surface state.
  llvm::Value *int32_sampleinfo_s_chan3 = this->CreateExtractElement(packed_sampleinfo_call, this->m_int3);
  llvm::Value *int32_paletteIndex = this->CreateSub(int32_sampleinfo_s_chan3, this->m_int1);

  // Number of samples = 2 ^ "sample position palette index".
  llvm::Value *int32_numberOfSamples = this->CreateShl(this->m_int1, int32_paletteIndex);

  // R = R' * num of Samples + SampleIndex
  llvm::Value *int32_mulwithSamples = this->CreateMul(srcAddressR, int32_numberOfSamples, VALUE_NAME("mul_s"));
  llvm::Value *int32_SrcAddrR = this->CreateAdd(int32_mulwithSamples, sampleIdx, VALUE_NAME("source_R"));

  llvm::Function *pFuncTypedRead =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedread, srcBuffer->getType());

  llvm::Value *args[] = {srcBuffer, srcAddressU, srcAddressV, int32_SrcAddrR, lod};

  llvm::Value *typedread = this->CreateCall(pFuncTypedRead, args);
  return typedread;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedwrite_msaa2D(llvm::Value *dstBuffer, llvm::Value *sampleIdx,
                                                                         llvm::Value *srcAddressU,
                                                                         llvm::Value *srcAddressV, llvm::Value *float_X,
                                                                         llvm::Value *float_Y, llvm::Value *float_Z,
                                                                         llvm::Value *float_W) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pFuncTypedWrite =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedwrite, dstBuffer->getType());

  // R = SampleIndex
  llvm::Value *args[] = {
      dstBuffer, srcAddressU, srcAddressV, sampleIdx, m_int0, float_X, float_Y, float_Z, float_W,
  };

  llvm::Value *typedwrite = this->CreateCall(pFuncTypedWrite, args);
  return typedwrite;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_typedwrite_msaa2DArray(
    llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
    llvm::Value *srcAddressR, llvm::Value *float_X, llvm::Value *float_Y, llvm::Value *float_Z, llvm::Value *float_W) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  // Call sampleinfoptr intrinsic to get the number of samples.
  // %tex = call <4 x i32> @llvm.GenISA.sampleinfoptr(4x(float)addrspace())
  llvm::Function *pfuncsampleinfoptr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleinfoptr, dstBuffer->getType());
  llvm::Value *packed_sampleinfo_call = this->CreateCall(pfuncsampleinfoptr, dstBuffer);

  // We can not use channel 0 of sampleinfo which should contain the correct
  // number of samples retrieved from surface state because this value in surface
  // state must be set to 1 in case of MSAA UAV emulation due to fact that
  // IGC does not support native MSAA UAV messages at the moment.
  // Instead of channel 0 we can use channel 3 of sampleinfo which contains
  // sample position palette index field retrieved from surface state.
  // The sample position palette index field is set to log2(number of samples).

  // Get sample position palette index from surface state. Note that this value
  // is incremented by one from its value in the surface state.
  llvm::Value *int32_sampleinfo_s_chan3 = this->CreateExtractElement(packed_sampleinfo_call, this->m_int3);
  llvm::Value *int32_paletteIndex = this->CreateSub(int32_sampleinfo_s_chan3, this->m_int1);

  // Number of samples = 2 ^ "sample position palette index".
  llvm::Value *int32_numberOfSamples = this->CreateShl(this->m_int1, int32_paletteIndex);

  // R = R' * num of Samples + SampleIndex
  llvm::Value *int32_mulwithSamples = this->CreateMul(srcAddressR, int32_numberOfSamples, VALUE_NAME("mul_s"));
  llvm::Value *int32_SrcAddrR = this->CreateAdd(int32_mulwithSamples, sampleIdx, VALUE_NAME("source_R"));

  llvm::Function *pFuncTypedWrite =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_typedwrite, dstBuffer->getType());

  // R = SampleIndex
  llvm::Value *args[] = {
      dstBuffer, srcAddressU, srcAddressV, int32_SrcAddrR, m_int0, float_X, float_Y, float_Z, float_W,
  };

  llvm::Value *typedwrite = this->CreateCall(pFuncTypedWrite, args);
  return typedwrite;
}

template <typename T, typename Inserter>
inline llvm::Value *
LLVM3DBuilder<T, Inserter>::Create_dwordatomictypedMsaa2D(llvm::Value *dstBuffer, llvm::Value *sampleIdx,
                                                          llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                                          llvm::Value *src, llvm::Value *instType) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {src->getType(), dstBuffer->getType()};

  llvm::Function *pFuncDwordAtomicTyped =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_intatomictyped, types);

  // R = SampleIndex
  llvm::Value *args[] = {dstBuffer, srcAddressU, srcAddressV, sampleIdx, src, instType};

  llvm::Value *dwordAtomicTyped = this->CreateCall(pFuncDwordAtomicTyped, args);
  return dwordAtomicTyped;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_dwordatomictypedMsaa2DArray(
    llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
    llvm::Value *srcAddressR, llvm::Value *src, llvm::Value *instType) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  // Call sampleinfoptr intrinsic to get the number of samples.
  // %tex = call <4 x i32> @llvm.GenISA.sampleinfoptr(4x(float)addrspace())
  llvm::Function *pfuncsampleinfoptr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleinfoptr, dstBuffer->getType());
  llvm::Value *packed_sampleinfo_call = this->CreateCall(pfuncsampleinfoptr, dstBuffer);

  // We can not use channel 0 of sampleinfo which should contain the correct
  // number of samples retrieved from surface state because this value in surface
  // state must be set to 1 in case of MSAA UAV emulation due to fact that
  // IGC does not support native MSAA UAV messages at the moment.
  // Instead of channel 0 we can use channel 3 of sampleinfo which contains
  // sample position palette index field retrieved from surface state.
  // The sample position palette index field is set to log2(number of samples).

  // Get sample position palette index from surface state. Note that this value
  // is incremented by one from its value in the surface state.
  llvm::Value *int32_sampleinfo_s_chan3 = this->CreateExtractElement(packed_sampleinfo_call, this->m_int3);
  llvm::Value *int32_paletteIndex = this->CreateSub(int32_sampleinfo_s_chan3, this->m_int1);

  // Number of samples = 2 ^ "sample position palette index".
  llvm::Value *int32_numberOfSamples = this->CreateShl(this->m_int1, int32_paletteIndex);

  // R = R' * num of Samples + SampleIndex
  llvm::Value *int32_mulwithSamples = this->CreateMul(srcAddressR, int32_numberOfSamples, VALUE_NAME("mul_s"));
  llvm::Value *int32_SrcAddrR = this->CreateAdd(int32_mulwithSamples, sampleIdx, VALUE_NAME("source_R"));

  llvm::Type *types[] = {src->getType(), dstBuffer->getType()};

  llvm::Function *pFuncDwordAtomicTyped =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_intatomictyped, types);

  llvm::Value *args[] = {dstBuffer, srcAddressU, srcAddressV, int32_SrcAddrR, src, instType};

  llvm::Value *dwordAtomicTyped = this->CreateCall(pFuncDwordAtomicTyped, args);
  return dwordAtomicTyped;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_StatelessAtomic(llvm::Value *ptr, llvm::Value *data,
                                                                       IGC::AtomicOp opcode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Type *types[] = {data->getType(), ptr->getType(), ptr->getType()};
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_intatomicrawA64, types);

  llvm::Value *args[] = {ptr, ptr, data, this->getInt32(opcode)};
  return this->CreateCall(pFunc, args);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_InidrectAtomic(llvm::Value *resource, llvm::Value *offset,
                                                                      llvm::Value *data, IGC::AtomicOp opcode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {data->getType(), resource->getType()};

  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_intatomicraw, types);

  llvm::Value *args[] = {resource, offset, data, this->getInt32(opcode)};
  return this->CreateCall(pFunc, args);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_StatelessAtomicCmpXChg(llvm::Value *ptr, llvm::Value *data0,
                                                                              llvm::Value *data1) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Type *types[] = {data0->getType(), ptr->getType(), ptr->getType()};
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_icmpxchgatomicrawA64, types);

  llvm::Value *args[] = {
      ptr,
      ptr,
      data0,
      data1,
  };
  return this->CreateCall(pFunc, args);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_InidrectAtomicCmpXChg(llvm::Value *resource, llvm::Value *offset,
                                                                             llvm::Value *data0, llvm::Value *data1) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {data0->getType(), resource->getType()};

  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_icmpxchgatomicraw, types);

  llvm::Value *args[] = {
      resource,
      offset,
      data0,
      data1,
  };
  return this->CreateCall(pFunc, args);
}

template <typename T, typename Inserter>
inline llvm::Value *
LLVM3DBuilder<T, Inserter>::Create_cmpxchgatomictypedMsaa2D(llvm::Value *dstBuffer, llvm::Value *sampleIdx,
                                                            llvm::Value *srcAddressU, llvm::Value *srcAddressV,
                                                            llvm::Value *src0, llvm::Value *src1) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {src0->getType(), dstBuffer->getType()};

  llvm::Function *pFuncCmpxchgatomictyped =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_icmpxchgatomictyped, types);

  // R = SampleIndex
  llvm::Value *args[] = {dstBuffer, srcAddressU, srcAddressV, sampleIdx, src0, src1};

  llvm::Value *dwordCmpxchgatomictyped = this->CreateCall(pFuncCmpxchgatomictyped, args);
  return dwordCmpxchgatomictyped;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_cmpxchgatomictypedMsaa2DArray(
    llvm::Value *dstBuffer, llvm::Value *sampleIdx, llvm::Value *srcAddressU, llvm::Value *srcAddressV,
    llvm::Value *srcAddressR, llvm::Value *src0, llvm::Value *src1) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  // Call sampleinfoptr intrinsic to get the number of samples.
  // %tex = call <4 x i32> @llvm.GenISA.sampleinfoptr(4x(float)addrspace())
  llvm::Function *pfuncsampleinfoptr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleinfoptr, dstBuffer->getType());
  llvm::Value *packed_sampleinfo_call = this->CreateCall(pfuncsampleinfoptr, dstBuffer);

  // We can not use channel 0 of sampleinfo which should contain the correct
  // number of samples retrieved from surface state because this value in surface
  // state must be set to 1 in case of MSAA UAV emulation due to fact that
  // IGC does not support native MSAA UAV messages at the moment.
  // Instead of channel 0 we can use channel 3 of sampleinfo which contains
  // sample position palette index field retrieved from surface state.
  // The sample position palette index field is set to log2(number of samples).

  // Get sample position palette index from surface state. Note that this value
  // is incremented by one from its value in the surface state.
  llvm::Value *int32_sampleinfo_s_chan3 = this->CreateExtractElement(packed_sampleinfo_call, this->m_int3);
  llvm::Value *int32_paletteIndex = this->CreateSub(int32_sampleinfo_s_chan3, this->m_int1);

  // Number of samples = 2 ^ "sample position palette index".
  llvm::Value *int32_numberOfSamples = this->CreateShl(this->m_int1, int32_paletteIndex);

  // R = R' * num of Samples + SampleIndex
  llvm::Value *int32_mulwithSamples = this->CreateMul(srcAddressR, int32_numberOfSamples, VALUE_NAME("mul_s"));
  llvm::Value *int32_SrcAddrR = this->CreateAdd(int32_mulwithSamples, sampleIdx, VALUE_NAME("source_R"));

  llvm::Type *types[] = {src0->getType(), dstBuffer->getType()};

  llvm::Function *pFuncCmpxchgatomictyped =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_icmpxchgatomictyped, types);

  llvm::Value *args[] = {dstBuffer, srcAddressU, srcAddressV, int32_SrcAddrR, src0, src1};

  llvm::Value *dwordCmpxchgatomictyped = this->CreateCall(pFuncCmpxchgatomictyped, args);
  return dwordCmpxchgatomictyped;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_TypedAtomic(llvm::Value *resource, llvm::Value *addressU,
                                                                   llvm::Value *addressV, llvm::Value *addressR,
                                                                   llvm::Value *data, IGC::AtomicOp opcode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {data->getType(), resource->getType()};

  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_intatomictyped, types);

  llvm::Value *args[] = {resource, addressU, addressV, addressR, data, this->getInt32(opcode)};
  return this->CreateCall(pFunc, args);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_TypedAtomicCmpXChg(llvm::Value *resource, llvm::Value *addressU,
                                                                          llvm::Value *addressV, llvm::Value *addressR,
                                                                          llvm::Value *data0, llvm::Value *data1) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {data0->getType(), resource->getType()};

  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_icmpxchgatomictyped, types);

  llvm::Value *args[] = {
      resource, addressU, addressV, addressR, data0, data1,
  };
  return this->CreateCall(pFunc, args);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SampleInfo(llvm::Value *resourcePtr) {
  llvm::Value *packed_tex_params[] = {
      resourcePtr,
  };

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::CallInst *packed_tex_call = llvm::cast<llvm::CallInst>(
      this->CreateCall(llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleinfoptr,
                                                             resourcePtr->getType()),
                       packed_tex_params));

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateReadSurfaceTypeAndFormat(llvm::Value *resourcePtr) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *fn = llvm::GenISAIntrinsic::getDeclaration(
      module, llvm::GenISAIntrinsic::GenISA_readsurfacetypeandformat, resourcePtr->getType());
  llvm::Value *packed_tex_call = this->CreateCall(fn, resourcePtr);
  return packed_tex_call;
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::Create_SyncThreadGroup() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  return this->CreateCall(
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_threadgroupbarrier));
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::Create_FlushSampler() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  return this->CreateCall(llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_flushsampler));
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::Create_LscFence(llvm::Value *SFID, llvm::Value *FenceOp, llvm::Value *Scope) {
  llvm::Value *parameters[] = {
      SFID,
      Scope,
      FenceOp,
  };
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  return this->CreateCall(llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_LSCFence),
                          parameters);
}
template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::Create_MemoryFence(bool commit, bool flushRWDataCache, bool flushConstantCache,
                                                            bool flushTextureCache, bool flushInstructionCache,
                                                            bool globalFence) {
  llvm::Value *parameters[] = {this->getInt1(commit),
                               this->getInt1(flushRWDataCache),
                               this->getInt1(flushConstantCache),
                               this->getInt1(flushTextureCache),
                               this->getInt1(flushInstructionCache),
                               this->getInt1(globalFence),
                               this->getInt1(false),
                               this->getInt1(false),
                               this->getInt32(globalFence ? LSC_SCOPE_GPU : LSC_SCOPE_GROUP)};
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  return this->CreateCall(llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_memoryfence),
                          parameters);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::Create_GlobalSync() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  return this->CreateCall(llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_globalSync));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_SamplePos(llvm::Value *int32_resourceIdx,
                                                                 llvm::Value *int32_samplerIdx) {
  llvm::Value *sampleInfo = this->Create_SampleInfo(int32_resourceIdx);

  llvm::Value *int32_texX = this->CreateExtractElement(sampleInfo, m_int0);
  llvm::Value *int32_texW = this->CreateExtractElement(sampleInfo, m_int3);

  llvm::Value *int32_tempIndex = this->CreateAdd(int32_texX, int32_samplerIdx);
  llvm::Value *int1_ole = this->CreateICmp(llvm::ICmpInst::ICMP_UGT, int32_texX, int32_samplerIdx);
  llvm::Value *int32_sel = this->CreateSelect(int1_ole, int32_tempIndex, m_int0);
  llvm::Value *int1_one = this->CreateICmp(llvm::ICmpInst::ICMP_EQ, int32_texW, m_int1);
  llvm::Value *int32_selIndex = this->CreateSelect(int1_one, m_int0, int32_sel);

  /*
      %tempY = extractelement <32 x f32> <f32 0.0, f32 0.0, f32 4.0 / 16.0, f32 -4.0 / 16.0, f32 -6.0 / 16.0,
                                          f32 -2.0 / 16.0, f32 2.0 / 16.0, f32 6.0 / 16.0, f32 -3.0 / 16.0,
                                          f32 3.0 / 16.0, f32 1.0 / 16.0, f32 -5.0 / 16.0, f32 5.0 / 16.0,
                                          f32 -1.0 / 16.0, f32 7.0 / 16.0, f32 -7.0 / 16.0, f32 1.0 / 16.0,
                                          f32 -3.0 / 16.0, f32 2.0 / 16.0, f32 -1.0 / 16.0, f32 -2.0 / 16.0,
                                          f32 5.0 / 16.0, f32 3.0 / 16.0, f32 -5.0 / 16.0, f32 6.0 / 16.0,
                                          f32 -7.0 / 16.0, f32 -6.0 / 16.0, f32 4.0 / 16.0, f32 0.0,
                                          f32 -4.0 / 16.0, f32 7.0 / 16.0, f32 -8.0 / 16.0>, i32 %selIndex
  */
  llvm::Value *float_y = nullptr;
  {
    llvm::Value *temp = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 32));
    temp = this->CreateInsertElement(temp, this->getFloat(0.0f), this->getInt32(0));
    temp = this->CreateInsertElement(temp, this->getFloat(0.0f), this->getInt32(1));
    temp = this->CreateInsertElement(temp, this->getFloat(4.0f / 16.0f), this->getInt32(2));
    temp = this->CreateInsertElement(temp, this->getFloat(-4.0f / 16.0f), this->getInt32(3));
    temp = this->CreateInsertElement(temp, this->getFloat(-6.0f / 16.0f), this->getInt32(4));
    temp = this->CreateInsertElement(temp, this->getFloat(-2.0f / 16.0f), this->getInt32(5));
    temp = this->CreateInsertElement(temp, this->getFloat(2.0f / 16.0f), this->getInt32(6));
    temp = this->CreateInsertElement(temp, this->getFloat(6.0f / 16.0f), this->getInt32(7));
    temp = this->CreateInsertElement(temp, this->getFloat(-3.0f / 16.0f), this->getInt32(8));
    temp = this->CreateInsertElement(temp, this->getFloat(3.0f / 16.0f), this->getInt32(9));
    temp = this->CreateInsertElement(temp, this->getFloat(1.0f / 16.0f), this->getInt32(10));
    temp = this->CreateInsertElement(temp, this->getFloat(-5.0f / 16.0f), this->getInt32(11));
    temp = this->CreateInsertElement(temp, this->getFloat(5.0f / 16.0f), this->getInt32(12));
    temp = this->CreateInsertElement(temp, this->getFloat(-1.0f / 16.0f), this->getInt32(13));
    temp = this->CreateInsertElement(temp, this->getFloat(7.0f / 16.0f), this->getInt32(14));
    temp = this->CreateInsertElement(temp, this->getFloat(-7.0f / 16.0f), this->getInt32(15));
    temp = this->CreateInsertElement(temp, this->getFloat(1.0f / 16.0f), this->getInt32(16));
    temp = this->CreateInsertElement(temp, this->getFloat(-3.0f / 16.0f), this->getInt32(17));
    temp = this->CreateInsertElement(temp, this->getFloat(2.0f / 16.0f), this->getInt32(18));
    temp = this->CreateInsertElement(temp, this->getFloat(-1.0f / 16.0f), this->getInt32(19));
    temp = this->CreateInsertElement(temp, this->getFloat(-2.0f / 16.0f), this->getInt32(20));
    temp = this->CreateInsertElement(temp, this->getFloat(5.0f / 16.0f), this->getInt32(21));
    temp = this->CreateInsertElement(temp, this->getFloat(3.0f / 16.0f), this->getInt32(22));
    temp = this->CreateInsertElement(temp, this->getFloat(-5.0f / 16.0f), this->getInt32(23));
    temp = this->CreateInsertElement(temp, this->getFloat(6.0f / 16.0f), this->getInt32(24));
    temp = this->CreateInsertElement(temp, this->getFloat(-7.0f / 16.0f), this->getInt32(25));
    temp = this->CreateInsertElement(temp, this->getFloat(-6.0f / 16.0f), this->getInt32(26));
    temp = this->CreateInsertElement(temp, this->getFloat(4.0f / 16.0f), this->getInt32(27));
    temp = this->CreateInsertElement(temp, this->getFloat(0.0f), this->getInt32(28));
    temp = this->CreateInsertElement(temp, this->getFloat(-4.0f / 16.0f), this->getInt32(29));
    temp = this->CreateInsertElement(temp, this->getFloat(7.0f / 16.0f), this->getInt32(30));
    temp = this->CreateInsertElement(temp, this->getFloat(-8.0f / 16.0f), this->getInt32(31));
    float_y = this->CreateExtractElement(temp, int32_selIndex);
  }

  /*
      %tempX = extractelement <32 x f32> <f32 0.0, f32 0.0, f32 4.0 / 16.0, f32 -4.0 / 16.0, f32 -2.0 / 16.0,
                                          f32 6.0 / 16.0, f32 -6.0 / 16.0, f32 2.0 / 16.0, f32 1.0 / 16.0,
                                          f32 -1.0 / 16.0, f32 5.0 / 16.0, f32 -3.0 / 16.0, f32 -5.0 / 16.0,
                                          f32 -7.0 / 16.0, f32 3.0 / 16.0, f32 7.0 / 16.0, f32 1.0 / 16.0,
                                          f32 -1.0 / 16.0, f32 -3.0 / 16.0, f32 4.0 / 16.0, f32 -5.0 / 16.0,
                                          f32 2.0 / 16.0, f32 5.0 / 16.0, f32 3.0 / 16.0, f32 -2.0 / 16.0,
                                          f32 0.0 / 16.0, f32 -4.0 / 16.0, f32 -6.0 / 16.0, f32 -8.0 / 16.0,
                                          f32 7.0 / 16.0, f32 6.0 / 16.0, f32 -7.0 / 16.0>, i32 %selIndex
  */
  llvm::Value *float_x = nullptr;
  {
    llvm::Value *temp = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 32));
    temp = this->CreateInsertElement(temp, this->getFloat(0.0f), this->getInt32(0));
    temp = this->CreateInsertElement(temp, this->getFloat(0.0f), this->getInt32(1));
    temp = this->CreateInsertElement(temp, this->getFloat(4.0f / 16.0f), this->getInt32(2));
    temp = this->CreateInsertElement(temp, this->getFloat(-4.0f / 16.0f), this->getInt32(3));
    temp = this->CreateInsertElement(temp, this->getFloat(-2.0f / 16.0f), this->getInt32(4));
    temp = this->CreateInsertElement(temp, this->getFloat(6.0f / 16.0f), this->getInt32(5));
    temp = this->CreateInsertElement(temp, this->getFloat(-6.0f / 16.0f), this->getInt32(6));
    temp = this->CreateInsertElement(temp, this->getFloat(2.0f / 16.0f), this->getInt32(7));
    temp = this->CreateInsertElement(temp, this->getFloat(1.0f / 16.0f), this->getInt32(8));
    temp = this->CreateInsertElement(temp, this->getFloat(-1.0f / 16.0f), this->getInt32(9));
    temp = this->CreateInsertElement(temp, this->getFloat(5.0f / 16.0f), this->getInt32(10));
    temp = this->CreateInsertElement(temp, this->getFloat(-3.0f / 16.0f), this->getInt32(11));
    temp = this->CreateInsertElement(temp, this->getFloat(-5.0f / 16.0f), this->getInt32(12));
    temp = this->CreateInsertElement(temp, this->getFloat(-7.0f / 16.0f), this->getInt32(13));
    temp = this->CreateInsertElement(temp, this->getFloat(3.0f / 16.0f), this->getInt32(14));
    temp = this->CreateInsertElement(temp, this->getFloat(7.0f / 16.0f), this->getInt32(15));
    temp = this->CreateInsertElement(temp, this->getFloat(1.0f / 16.0f), this->getInt32(16));
    temp = this->CreateInsertElement(temp, this->getFloat(-1.0f / 16.0f), this->getInt32(17));
    temp = this->CreateInsertElement(temp, this->getFloat(-3.0f / 16.0f), this->getInt32(18));
    temp = this->CreateInsertElement(temp, this->getFloat(4.0f / 16.0f), this->getInt32(19));
    temp = this->CreateInsertElement(temp, this->getFloat(-5.0f / 16.0f), this->getInt32(20));
    temp = this->CreateInsertElement(temp, this->getFloat(2.0f / 16.0f), this->getInt32(21));
    temp = this->CreateInsertElement(temp, this->getFloat(5.0f / 16.0f), this->getInt32(22));
    temp = this->CreateInsertElement(temp, this->getFloat(3.0f / 16.0f), this->getInt32(23));
    temp = this->CreateInsertElement(temp, this->getFloat(-2.0f / 16.0f), this->getInt32(24));
    temp = this->CreateInsertElement(temp, this->getFloat(0.0f), this->getInt32(25));
    temp = this->CreateInsertElement(temp, this->getFloat(-4.0f / 16.0f), this->getInt32(26));
    temp = this->CreateInsertElement(temp, this->getFloat(-6.0f / 16.0f), this->getInt32(27));
    temp = this->CreateInsertElement(temp, this->getFloat(-8.0f / 16.0f), this->getInt32(28));
    temp = this->CreateInsertElement(temp, this->getFloat(7.0f / 16.0f), this->getInt32(29));
    temp = this->CreateInsertElement(temp, this->getFloat(6.0f / 16.0f), this->getInt32(30));
    temp = this->CreateInsertElement(temp, this->getFloat(-7.0f / 16.0f), this->getInt32(31));
    float_x = this->CreateExtractElement(temp, int32_selIndex);
  }

  llvm::Value *packed_ret_value = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));
  packed_ret_value = this->CreateInsertElement(packed_ret_value, float_x, this->getInt32(0));
  packed_ret_value = this->CreateInsertElement(packed_ret_value, float_y, this->getInt32(1));
  packed_ret_value = this->CreateInsertElement(packed_ret_value, this->getFloat(0.0f), this->getInt32(2));
  packed_ret_value = this->CreateInsertElement(packed_ret_value, this->getFloat(0.0f), this->getInt32(3));

  return packed_ret_value;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLE(llvm::Value *coordinate_u, llvm::Value *coordinate_v,
                                                                 llvm::Value *coordinate_r, llvm::Value *coordinate_ai,
                                                                 llvm::Value *ptr_textureIdx, llvm::Value *ptr_sampler,
                                                                 llvm::Value *offsetU, llvm::Value *offsetV,
                                                                 llvm::Value *offsetW, llvm::Value *minlod,
                                                                 bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLE(coordinate_u, coordinate_v, coordinate_r, coordinate_ai,
                       llvm::UndefValue::get(ptr_textureIdx->getType()), ptr_textureIdx, ptr_sampler, offsetU, offsetV,
                       offsetW, minlod, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLE(
    llvm::Value *coordinate_u, llvm::Value *coordinate_v, llvm::Value *coordinate_r, llvm::Value *coordinate_ai,
    llvm::Value *ptr_pairedTextureIdx, llvm::Value *ptr_textureIdx, llvm::Value *ptr_sampler, llvm::Value *offsetU,
    llvm::Value *offsetV, llvm::Value *offsetW, llvm::Value *minlod, bool feedback_enabled, llvm::Type *returnType) {
  if (minlod == nullptr) {
    minlod = llvm::ConstantFP::get(coordinate_u->getType(), 0.0);
  }

  llvm::Value *packed_tex_params[] = {
      coordinate_u,   coordinate_v, coordinate_r, coordinate_ai, minlod, ptr_pairedTextureIdx,
      ptr_textureIdx, ptr_sampler,  offsetU,      offsetV,       offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), coordinate_u->getType(),
                         ptr_pairedTextureIdx->getType(), ptr_textureIdx->getType(), ptr_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEC(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetR,
    llvm::Value *minlod, bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLEC(float_reference_0, float_address_0, float_address_1, float_address_2, float_address_3,
                        llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                        int32_offsetU, int32_offsetV, int32_offsetR, minlod, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEC(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_offsetR, llvm::Value *minlod, bool feedback_enabled, llvm::Type *returnType) {
  if (minlod == nullptr) {
    minlod = llvm::ConstantFP::get(float_address_0->getType(), 0.0);
  }

  llvm::Value *packed_tex_params[] = {float_reference_0, float_address_0, float_address_1,        float_address_2,
                                      float_address_3,   minlod,          int32_pairedTextureIdx, int32_textureIdx,
                                      int32_sampler,     int32_offsetU,   int32_offsetV,          int32_offsetR};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_reference_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleCptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleCptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleCptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLELC(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *float_lod, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLELC(float_reference_0, float_address_0, float_address_1, float_address_2, float_address_3,
                         float_lod, llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                         int32_offsetU, int32_offsetV, int32_offsetW, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLELC(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *float_lod,
    llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled,
    llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {float_reference_0, float_lod,       float_address_0,        float_address_1,
                                      float_address_2,   float_address_3, int32_pairedTextureIdx, int32_textureIdx,
                                      int32_sampler,     int32_offsetU,   int32_offsetV,          int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_reference_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleLCptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleLCptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleLCptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEC_LZ(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLEC_LZ(float_reference_0, float_address_0, float_address_1, float_address_2, float_address_3,
                           llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                           int32_offsetU, int32_offsetV, int32_offsetW, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEC_LZ(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_offsetW, bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {float_reference_0,      llvm::ConstantFP::get(float_address_0->getType(), 0.0),
                                      float_address_0,        float_address_1,
                                      float_address_2,        float_address_3,
                                      int32_pairedTextureIdx, int32_textureIdx,
                                      int32_sampler,          int32_offsetU,
                                      int32_offsetV,          int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_reference_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleLCptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleLCptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleLCptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4C(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_srcChannel,
    bool feedback_enabled, llvm::Type *returnType) {
  return Create_gather4C(float_reference_0, float_address_0, float_address_1, float_address_2, float_address_3,
                         llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                         int32_offsetU, int32_offsetV, int32_srcChannel, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4C(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_srcChannel, bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {float_reference_0, float_address_0,        float_address_1,  float_address_2,
                                      float_address_3,   int32_pairedTextureIdx, int32_textureIdx, int32_sampler,
                                      int32_offsetU,     int32_offsetV,          m_int0,           int32_srcChannel};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_reference_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_gather4Cptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_gather4Cptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_gather4Cptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4POC(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *int_src_offset_0, llvm::Value *int_src_offset_1, llvm::Value *float_src_reference_0,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_srcChannel, bool feedback_enabled, llvm::Type *returnType) {
  return Create_gather4POC(float_address_0, float_address_1, float_address_2, int_src_offset_0, int_src_offset_1,
                           float_src_reference_0, llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx,
                           int32_sampler, int32_offsetU, int32_offsetV, int32_srcChannel, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4POC(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *int_src_offset_0, llvm::Value *int_src_offset_1, llvm::Value *float_src_reference_0,
    llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_srcChannel, bool feedback_enabled,
    llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {float_src_reference_0, float_address_0, float_address_1,        int_src_offset_0,
                                      int_src_offset_1,      float_address_2, int32_pairedTextureIdx, int32_textureIdx,
                                      int32_sampler,         int32_offsetU,   int32_offsetV,          m_int0,
                                      int32_srcChannel};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_src_reference_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_gather4POCptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_gather4POCptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_gather4POCptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4PO(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *int_src_offset_0, llvm::Value *int_src_offset_1, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_srcChannel,
    bool feedback_enabled, llvm::Type *returnType) {
  return Create_gather4PO(float_address_0, float_address_1, float_address_2, int_src_offset_0, int_src_offset_1,
                          llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                          int32_offsetU, int32_offsetV, int32_srcChannel, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4PO(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *int_src_offset_0, llvm::Value *int_src_offset_1, llvm::Value *int32_pairedTextureIdx,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_srcChannel, bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {float_address_0, float_address_1,        int_src_offset_0, int_src_offset_1,
                                      float_address_2, int32_pairedTextureIdx, int32_textureIdx, int32_sampler,
                                      int32_offsetU,   int32_offsetV,          m_int0,           int32_srcChannel};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_address_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_gather4POptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_gather4POptr, types);
  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_gather4POptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_gather4PositionOffsets(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::ArrayRef<llvm::Value *> int_src_offsets, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_srcChannel, bool feedback_enabled) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  unsigned int gatherReturnSize = feedback_enabled ? 5 : 4;
  llvm::Value *gatherReturn = llvm::UndefValue::get(
      IGCLLVM::FixedVectorType::get(llvm::Type::getFloatTy(module->getContext()), gatherReturnSize));
  llvm::Value *feedbackValue = this->getInt32(0xffffffff);

  for (int i = 0, j = 0; i < 7; i = i + 2, j++) {
    llvm::Value *packed_tex_call =
        Create_gather4PO(float_address_0, float_address_1, float_address_2, int_src_offsets[i], int_src_offsets[i + 1],
                         int32_textureIdx, int32_sampler, int32_offsetU, int32_offsetV, int32_srcChannel,
                         feedback_enabled, llvm::Type::getFloatTy(module->getContext()));

    gatherReturn = this->CreateInsertElement(
        gatherReturn, this->CreateExtractElement(packed_tex_call, this->getInt32(3)), this->getInt32(j), "call_inst");

    if (feedback_enabled) {
      llvm::Value *callFeedbackValue = this->CreateExtractElement(packed_tex_call, this->getInt32(4));
      callFeedbackValue = this->CreateBitCast(callFeedbackValue, this->getInt32Ty());
      feedbackValue = this->CreateAnd(feedbackValue, callFeedbackValue);
    }
  }

  if (feedback_enabled) {
    IGC_ASSERT(feedbackValue);
    feedbackValue = this->CreateBitCast(feedbackValue, this->getFloatTy());
    gatherReturn = this->CreateInsertElement(gatherReturn, feedbackValue, 4);
  }

  return gatherReturn;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_gather4PositionOffsetsC(
    llvm::Value *float_reference_0, llvm::Value *float_address_0, llvm::Value *float_address_1,
    llvm::Value *float_address_2, llvm::ArrayRef<llvm::Value *> int_src_offsets, llvm::Value *int32_textureIdx_356,
    llvm::Value *int32_sampler_357, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_srcChannel, bool feedback_enabled) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  unsigned int gatherReturnSize = feedback_enabled ? 5 : 4;
  llvm::Value *gatherReturn = llvm::UndefValue::get(
      IGCLLVM::FixedVectorType::get(llvm::Type::getFloatTy(module->getContext()), gatherReturnSize));
  llvm::Value *feedbackValue = this->getInt32(0xffffffff);

  for (int i = 0, j = 0; i < 7; i = i + 2, j++) {
    llvm::Value *packed_tex_1527_call =
        Create_gather4POC(float_address_0, float_address_1, float_address_2, int_src_offsets[i], int_src_offsets[i + 1],
                          float_reference_0, int32_textureIdx_356, int32_sampler_357, int32_offsetU, int32_offsetV,
                          int32_srcChannel, feedback_enabled, llvm::Type::getFloatTy(module->getContext()));

    gatherReturn =
        this->CreateInsertElement(gatherReturn, this->CreateExtractElement(packed_tex_1527_call, this->getInt32(3)),
                                  this->getInt32(j), "call_inst");

    if (feedback_enabled) {
      llvm::Value *callFeedbackValue = this->CreateExtractElement(packed_tex_1527_call, this->getInt32(4));
      callFeedbackValue = this->CreateBitCast(callFeedbackValue, this->getInt32Ty());
      feedbackValue = this->CreateAnd(feedbackValue, callFeedbackValue);
    }
  }

  if (feedback_enabled) {
    IGC_ASSERT(feedbackValue);
    feedbackValue = this->CreateBitCast(feedbackValue, this->getFloatTy());
    gatherReturn = this->CreateInsertElement(gatherReturn, feedbackValue, 4);
  }

  return gatherReturn;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEB(
    llvm::Value *float_bias_0, llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *float_address_3, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
    llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, llvm::Value *minlod, bool feedback_enabled,
    llvm::Type *returnType) {
  return Create_SAMPLEB(float_bias_0, float_address_0, float_address_1, float_address_2, float_address_3,
                        llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                        int32_offsetU, int32_offsetV, int32_offsetW, minlod, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEB(
    llvm::Value *float_bias_0, llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    llvm::Value *minlod, bool feedback_enabled, llvm::Type *returnType) {
  if (minlod == nullptr) {
    minlod = llvm::ConstantFP::get(float_address_0->getType(), 0.0);
  }

  //   %tex = call <4 x float> @llvm.GenISA.sample.v4f32.f32(float %src_s.chan0, float %src_s.chan1, float %src_s.chan2,
  //   float 0.000000e+00, i32 %textureIdx, i32 %sampler, i32 %offsetU, i32 %offsetV, i32 %offsetW)
  llvm::Value *packed_tex_params[] = {float_bias_0,    float_address_0, float_address_1,        float_address_2,
                                      float_address_3, minlod,          int32_pairedTextureIdx, int32_textureIdx,
                                      int32_sampler,   int32_offsetU,   int32_offsetV,          int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_bias_0->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleB_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleBptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleB_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEL(
    llvm::Value *float_lod_0, llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *float_address_3, llvm::Value *ptr_textureIdx, llvm::Value *ptr_sampler, llvm::Value *int32_offsetU,
    llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLEL(float_lod_0, float_address_0, float_address_1, float_address_2, float_address_3,
                        llvm::UndefValue::get(ptr_textureIdx->getType()), ptr_textureIdx, ptr_sampler, int32_offsetU,
                        int32_offsetV, int32_offsetW, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEL(
    llvm::Value *float_lod_0, llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *float_address_3, llvm::Value *ptr_pairedTextureIdx, llvm::Value *ptr_textureIdx,
    llvm::Value *ptr_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {float_lod_0,     float_address_0,      float_address_1, float_address_2,
                                      float_address_3, ptr_pairedTextureIdx, ptr_textureIdx,  ptr_sampler,
                                      int32_offsetU,   int32_offsetV,        int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_lod_0->getType(),
                         ptr_pairedTextureIdx->getType(), ptr_textureIdx->getType(), ptr_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleL_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleLptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleL_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLED(SampleD_DC_FromCubeParams &sampleParams,
                                                                  llvm::Value *minlod, bool feedback_enabled,
                                                                  llvm::Type *returnType) {
  return Create_SAMPLED(sampleParams.get_float_src_u(), sampleParams.get_float_src_v(), sampleParams.get_float_src_r(),
                        sampleParams.get_dxu(), sampleParams.get_dxv(), sampleParams.get_dxr(), sampleParams.get_dyu(),
                        sampleParams.get_dyv(), sampleParams.get_dyr(), sampleParams.get_float_src_ai(),
                        sampleParams.get_int32_pairedTextureIdx(), sampleParams.get_int32_textureIdx(),
                        sampleParams.get_int32_sampler(), sampleParams.get_int32_offsetU(),
                        sampleParams.get_int32_offsetV(), sampleParams.get_int32_offsetW(), minlod, feedback_enabled,
                        returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLED(
    llvm::Value *float_src1_s_chan0, llvm::Value *float_src1_s_chan1, llvm::Value *float_src1_s_chan2,
    llvm::Value *float_src2_s_chan0, llvm::Value *float_src2_s_chan1, llvm::Value *float_src2_s_chan2,
    llvm::Value *float_src3_s_chan0, llvm::Value *float_src3_s_chan1, llvm::Value *float_src3_s_chan2,
    llvm::Value *float_src1_s_chan3, llvm::Value *ptr_textureIdx, llvm::Value *ptr_sampler,
    llvm::Value *int32_offsetU_358, llvm::Value *int32_offsetV_359, llvm::Value *int32_offsetW_359, llvm::Value *minlod,
    bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLED(float_src1_s_chan0, float_src1_s_chan1, float_src1_s_chan2, float_src2_s_chan0,
                        float_src2_s_chan1, float_src2_s_chan2, float_src3_s_chan0, float_src3_s_chan1,
                        float_src3_s_chan2, float_src1_s_chan3, llvm::UndefValue::get(ptr_textureIdx->getType()),
                        ptr_textureIdx, ptr_sampler, int32_offsetU_358, int32_offsetV_359, int32_offsetW_359, minlod,
                        feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLED(
    llvm::Value *float_src1_s_chan0, llvm::Value *float_src1_s_chan1, llvm::Value *float_src1_s_chan2,
    llvm::Value *float_src2_s_chan0, llvm::Value *float_src2_s_chan1, llvm::Value *float_src2_s_chan2,
    llvm::Value *float_src3_s_chan0, llvm::Value *float_src3_s_chan1, llvm::Value *float_src3_s_chan2,
    llvm::Value *float_src1_s_chan3, llvm::Value *ptr_pairedTextureIdx, llvm::Value *ptr_textureIdx,
    llvm::Value *ptr_sampler, llvm::Value *int32_offsetU_358, llvm::Value *int32_offsetV_359,
    llvm::Value *int32_offsetW_359, llvm::Value *minlod, bool feedback_enabled, llvm::Type *returnType) {
  if (minlod == nullptr) {
    minlod = llvm::ConstantFP::get(float_src1_s_chan0->getType(), 0.0);
  }

  //   %tex = call <4 x float> @llvm.GenISA.sample.v4f32.f32D(float %src_s.chan0, float %src2_s.chan0, float
  //   %src3_s.chan0, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float
  //   0.000000e+00, float 0.000000e+00, float 0.000000e+00, i32 %textureIdx, i32 %sampler, i32 %offsetU, i32 0, i32 0)
  llvm::Value *packed_tex_params[] = {float_src1_s_chan0,
                                      float_src2_s_chan0,
                                      float_src3_s_chan0,
                                      float_src1_s_chan1,
                                      float_src2_s_chan1,
                                      float_src3_s_chan1,
                                      float_src1_s_chan2,
                                      float_src2_s_chan2,
                                      float_src3_s_chan2,
                                      float_src1_s_chan3,
                                      minlod,
                                      ptr_pairedTextureIdx,
                                      ptr_textureIdx,
                                      ptr_sampler,
                                      int32_offsetU_358,
                                      int32_offsetV_359,
                                      int32_offsetW_359};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_src1_s_chan0->getType(),
                         ptr_pairedTextureIdx->getType(), ptr_textureIdx->getType(), ptr_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleDptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleDptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleDptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEDCMlod(
    llvm::Value *float_ref, llvm::Value *float_src_u, llvm::Value *dxu, llvm::Value *dyu, llvm::Value *float_src_v,
    llvm::Value *dxv, llvm::Value *dyv, llvm::Value *float_src_r, llvm::Value *dxr, llvm::Value *dyr,
    llvm::Value *float_src_ai, llvm::Value *minlod, llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    bool feedback_enabled, llvm::Type *returnType) {
  //   %tex = call <4 x float> @llvm.GenISA.sample.v4f32.f32D(float %float_ref, float %float_src_u, float %dxu, float
  //   %dxu, float %dyu, float float_src_v,
  //                           float %dxv, float %dyv, float %float_src_r, float %dxr, float %dyr, flaot %minlod, float
  //                           0.000000e+00, i32 %textureIdx, i32 %sampler, i32 %offsetU, i32 %offsetV, i32 %offsetW)
  llvm::Value *packed_tex_params[] = {float_ref,
                                      float_src_u,
                                      dxu,
                                      dyu,
                                      float_src_v,
                                      dxv,
                                      dyv,
                                      float_src_r,
                                      dxr,
                                      dyr,
                                      float_src_ai,
                                      minlod,
                                      int32_pairedTextureIdx,
                                      int32_textureIdx,
                                      int32_sampler,
                                      int32_offsetU,
                                      int32_offsetV,
                                      int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_ref->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleDCMlodptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleDCMlodptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleDCMlodptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEDC(
    llvm::Value *float_ref, llvm::Value *float_src_u, llvm::Value *dxu, llvm::Value *dyu, llvm::Value *float_src_v,
    llvm::Value *dxv, llvm::Value *dyv, llvm::Value *float_src_r, llvm::Value *dxr, llvm::Value *dyr,
    llvm::Value *float_src_ai, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU,
    llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled, llvm::Type *returnType) {
  return Create_SAMPLEDC(float_ref, float_src_u, dxu, dyu, float_src_v, dxv, dyv, float_src_r, dxr, dyr, float_src_ai,
                         llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                         int32_offsetU, int32_offsetV, int32_offsetW, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEDC(
    llvm::Value *float_ref, llvm::Value *float_src_u, llvm::Value *dxu, llvm::Value *dyu, llvm::Value *float_src_v,
    llvm::Value *dxv, llvm::Value *dyv, llvm::Value *float_src_r, llvm::Value *dxr, llvm::Value *dyr,
    llvm::Value *float_src_ai, llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    bool feedback_enabled, llvm::Type *returnType) {
  //   %tex = call <4 x float> @llvm.GenISA.sample.v4f32.f32D(float %float_ref, float %float_src_u, float %dxu, float
  //   %dxu, float %dyu, float float_src_v,
  //                           float %dxv, float %dyv, float %float_src_r, float %dxr, float %dyr, float 0.000000e+00,
  //                           i32 %textureIdx, i32 %sampler, i32 %offsetU, i32 %offsetV, i32 %offsetW)
  llvm::Value *packed_tex_params[] = {float_ref,
                                      float_src_u,
                                      dxu,
                                      dyu,
                                      float_src_v,
                                      dxv,
                                      dyv,
                                      float_src_r,
                                      dxr,
                                      dyr,
                                      float_src_ai,
                                      int32_pairedTextureIdx,
                                      int32_textureIdx,
                                      int32_sampler,
                                      int32_offsetU,
                                      int32_offsetV,
                                      int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_ref->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleDCptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleDCptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleDCptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *
LLVM3DBuilder<T, Inserter>::Create_lod(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                       llvm::Value *float_address_2, llvm::Value *float_address_3,
                                       llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
                                       bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {
      float_address_0, float_address_1, float_address_2, float_address_3, int32_textureIdx_356, int32_sampler_357,
  };

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_address_0->getType(),
                         int32_textureIdx_356->getType(), int32_sampler_357->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_lodptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_lodptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_lodptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *float_address_3, llvm::Value *int32_textureIdx_356, llvm::Value *int32_sampler_357,
    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, llvm::Value *int32_srcChannel,
    bool feedback_enabled, llvm::Type *returnType) {
  return Create_gather4(float_address_0, float_address_1, float_address_2, float_address_3,
                        llvm::UndefValue::get(int32_textureIdx_356->getType()), int32_textureIdx_356, int32_sampler_357,
                        int32_offsetU, int32_offsetV, int32_offsetW, int32_srcChannel, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_gather4(
    llvm::Value *float_address_0, llvm::Value *float_address_1, llvm::Value *float_address_2,
    llvm::Value *float_address_3, llvm::Value *int32_pairedTextureIdx_356, llvm::Value *int32_textureIdx_356,
    llvm::Value *int32_sampler_357, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    llvm::Value *int32_srcChannel, bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {
      float_address_0,      float_address_1,   float_address_2, float_address_3, int32_pairedTextureIdx_356,
      int32_textureIdx_356, int32_sampler_357, int32_offsetU,   int32_offsetV,   int32_offsetW,
      int32_srcChannel};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_address_0->getType(),
                         int32_pairedTextureIdx_356->getType(), int32_textureIdx_356->getType(),
                         int32_sampler_357->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_gather4ptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_gather4ptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_gather4ptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_load(llvm::Value *int32_sampleIdxU,
                                                               llvm::Value *int32_sampleIdxV,
                                                               llvm::Value *int32_sampleIdxR, llvm::Value *int32_lod,
                                                               llvm::Value *ptr_textureIdx, llvm::Value *int32_offsetU,
                                                               llvm::Value *int32_offsetV, llvm::Value *int32_offsetR,
                                                               bool feedback_enabled, llvm::Type *returnType) {
  return Create_load(int32_sampleIdxU, int32_sampleIdxV, int32_sampleIdxR, int32_lod,
                     llvm::UndefValue::get(ptr_textureIdx->getType()), ptr_textureIdx, int32_offsetU, int32_offsetV,
                     int32_offsetR, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_load(
    llvm::Value *int32_sampleIdxU, llvm::Value *int32_sampleIdxV, llvm::Value *int32_sampleIdxR, llvm::Value *int32_lod,
    llvm::Value *ptr_pairedTextureIdx, llvm::Value *ptr_textureIdx, llvm::Value *int32_offsetU,
    llvm::Value *int32_offsetV, llvm::Value *int32_offsetR, bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {int32_sampleIdxU, int32_sampleIdxV,     int32_lod,
                                      int32_sampleIdxR, ptr_pairedTextureIdx, ptr_textureIdx,
                                      int32_offsetU,    int32_offsetV,        int32_offsetR};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, feedback_enabled ? 5 : 4),
                         ptr_pairedTextureIdx->getType(), ptr_textureIdx->getType()};

  llvm::Function *func_llvm_GenISA_ldptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ldptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_ldptr_v4f32_f32, packed_tex_params);

  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_ldms(llvm::Value *int32_srcIdxU, llvm::Value *int32_srcIdxV,
                                                               llvm::Value *int32_srcIdxR, llvm::Value *int32_sampleIdx,
                                                               llvm::Value *int32_textureIdx,
                                                               llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
                                                               llvm::Value *int32_offsetR, bool feedback_enabled,
                                                               llvm::Type *returnType) {
  llvm::Value *packed_mcs_params[] = {int32_srcIdxU,    int32_srcIdxV, int32_srcIdxR, m_int0,
                                      int32_textureIdx, int32_offsetU, int32_offsetV, int32_offsetR};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(this->getInt32Ty(), 2), this->getInt32Ty(),
                         int32_textureIdx->getType()};
  llvm::Function *func_llvm_GenISA_ldmcsptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ldmcsptr, types);
  llvm::CallInst *packed_mcs_call = this->CreateCall(func_llvm_GenISA_ldmcsptr_v4f32_f32, packed_mcs_params);

  llvm::Value *mcs_ch0 = this->CreateExtractElement(packed_mcs_call, m_int0);
  llvm::Value *mcs_ch1 = this->CreateExtractElement(packed_mcs_call, m_int1);

  llvm::Value *packed_tex_params[] = {int32_sampleIdx, mcs_ch0,       mcs_ch1,      int32_srcIdxU,
                                      int32_srcIdxV,   int32_srcIdxR, m_int0,       int32_textureIdx,
                                      int32_offsetU,   int32_offsetV, int32_offsetR};

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types_ldms[] = {IGCLLVM::FixedVectorType::get(dstType, 4), int32_textureIdx->getType()};
  if (feedback_enabled) {
    types_ldms[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }

  llvm::Function *func_llvm_GenISA_ldmsptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ldmsptr, types_ldms);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_ldmsptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline SampleParamsFromCube LLVM3DBuilder<T, Inserter>::Prepare_SAMPLE_Cube_ParamsFromUnormalizedCoords(
    llvm::Value *int32_lod, llvm::Value *int32_textureIdx, llvm::Value *int32_u, llvm::Value *int32_v,
    llvm::Value *int32_faceid, llvm::Value *int32_cube_array_index, llvm::Value *float_array_6_3,
    llvm::Value *int32_sampler) {
  // Samplers point of reference is always center of the face, which is (0,0)
  // That means the four vertices of the normalized cube are depiced as below
  //(-1,-1)        (1,-1)
  //   -------|---------
  //   |      |        |
  //   |      |        |
  //   |---------------|
  //   |      |(0,0)   |
  //   |      |        |
  //   -------|---------
  //(-1,1)          (1,1)
  // Thus each un-normalized coordiate (x,y) needs to be normalized between <-1,1>
  // Below is the Math to normalize between <-1,1>
  // u = (u * 2 + 1)/width - 1
  // v = (v * 2 + 1)/height - 1

  // Using resinfo extract width and height of the buffer
  // Using resinfo extract width and height of the buffer
  llvm::Value *resinfo = this->Create_resinfo(int32_lod, int32_textureIdx);
  llvm::Value *width = this->CreateExtractElement(resinfo, m_int0);
  llvm::Value *height = this->CreateExtractElement(resinfo, m_int1);

  // convert u, v, width and height to float
  llvm::Value *float_u = this->CreateUIToFP(int32_u, this->getFloatTy());
  llvm::Value *float_v = this->CreateUIToFP(int32_v, this->getFloatTy());
  width = this->CreateUIToFP(width, this->getFloatTy());
  height = this->CreateUIToFP(height, this->getFloatTy());
  // define some constants
  llvm::Value *float_minus1 = this->getFloat(-1.0);
  llvm::Value *float_2 = this->getFloat(2.0);

  // u and v represent the coordinates of a texel for a given face
  // Now normalize u in the range [-1,1] using following equation
  // u = (2*u + 1)/width -1
  float_u = this->CreateFAdd(this->CreateFMul(float_u, float_2), m_float1);
  float_u = this->CreateFSub(this->CreateFDiv(float_u, width), m_float1);
  // Now normalize v in the range [-1,1] using following equation
  // v = (v * 2 + 1)/height - 1
  float_v = this->CreateFAdd(this->CreateFMul(float_v, float_2), m_float1);
  float_v = this->CreateFSub(this->CreateFDiv(float_v, height), m_float1);

  llvm::Value *minus_floatu = this->CreateFMul(float_u, float_minus1); //-u
  llvm::Value *minus_floatv = this->CreateFMul(float_v, float_minus1); //-v
  llvm::Value *float_arrayIndex = this->CreateUIToFP(int32_cube_array_index, this->getFloatTy());
  // This array represents how the u and v value needs to be picked, for a face
  unsigned num_cube_faces = 6;
  unsigned num_dimensions = 3;

  // The mapping of face-id to texture surface is as follows
  //+x->face 0, -x->face 1, +y -> face 2, -y -> face 3, +z -> face 4, -z -> face 5
  // Now for each face we need to transform the normalized coordinates as follows
  // face 0(+X) = (-v, -u), face 1(-X) = (-v, u), face 2(+Y) = (u, v)
  // face 3(-Y) = (u, -v) , face 4(+Z) = (u, -v), face 5(+Z) = (-u, -v)
  // Refer to https://en.wikipedia.org/wiki/Cube_mapping for details
  llvm::Value *cubeCoordMap[6][3] = {
      // clang-format off
        { m_float1     ,    minus_floatv,   minus_floatu    }, //+x = face0
        { float_minus1 ,    minus_floatv,   float_u         }, //-x = face1
        { float_u ,         m_float1    ,   float_v         }, //+y = face2
        { float_u ,         float_minus1,   minus_floatv    }, //-y = face3
        { float_u ,         minus_floatv,   m_float1        }, //+z = face4
        { minus_floatu ,    minus_floatv,   float_minus1    }  //-z = face5
      // clang-format on
  };
  // Now populate the 6x3 array with values of cubeCoordMap
  llvm::Value *indexList[2];
  llvm::Value *row, *elt;
  indexList[0] = m_int0;
  for (unsigned faceid = 0; faceid < num_cube_faces; faceid++) {
    indexList[1] = this->getInt32(faceid);
    row = this->CreateGEP(float_array_6_3, llvm::ArrayRef<llvm::Value *>(indexList, 2));
    for (unsigned j = 0; j < num_dimensions; j++) {
      indexList[1] = this->getInt32(j);
      elt = this->CreateGEP(row, llvm::ArrayRef<llvm::Value *>(indexList, 2));
      this->CreateStore(cubeCoordMap[faceid][j], elt);
    }
  }

  // Now pick the one the row indexed by int32_faceid
  llvm::Value *finalCoords[3];
  indexList[1] = int32_faceid;
  row = this->CreateGEP(float_array_6_3, llvm::ArrayRef<llvm::Value *>(indexList, 2));
  for (unsigned i = 0; i < 3; i++) {
    indexList[1] = this->getInt32(i);
    elt = this->CreateGEP(row, llvm::ArrayRef<llvm::Value *>(indexList, 2));
    finalCoords[i] = this->CreateLoad(this->getFloatTy(), elt);
  }

  SampleParamsFromCube CubeRetParams;
  CubeRetParams.float_xcube = finalCoords[0];
  CubeRetParams.float_ycube = finalCoords[1];
  CubeRetParams.float_address_3 = finalCoords[2];
  CubeRetParams.float_aicube = float_arrayIndex;
  CubeRetParams.int32_textureIdx = int32_textureIdx;
  CubeRetParams.int32_sampler = int32_sampler;
  CubeRetParams.offsetU = int32_u;
  CubeRetParams.offsetV = int32_v;
  CubeRetParams.offsetR = m_int0; // Not used
  return CubeRetParams;
}

template <typename T, typename Inserter>
inline SampleParamsFromCube
LLVM3DBuilder<T, Inserter>::Prepare_SAMPLE_Cube_Params(llvm::Value *float_address_0, llvm::Value *float_address_1,
                                                       llvm::Value *float_address_2, llvm::Value *float_address_3,
                                                       llvm::Value *int32_textureIdx, llvm::Value *int32_sampler) {
  IGC_ASSERT(nullptr != float_address_0);
  llvm::Type *const coordType = float_address_0->getType();
  IGC_ASSERT(nullptr != coordType);
  IGC_ASSERT(coordType->isFloatTy() || coordType->isHalfTy());

  llvm::Value *zero = llvm::ConstantFP::get(coordType, 0.0);

  //   %xneg_s = fsub float 0.000000e+00, %src_s.chan0
  llvm::Value *float_xneg_s_1389 = this->CreateFSub(zero, float_address_0, VALUE_NAME("xneg_s"));

  //   %cmpx_s = fcmp oge float %src_s.chan0, 0.000000e+00
  llvm::Value *int1_cmpx_s_1390 =
      this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_address_0, zero, VALUE_NAME("cmpx_s"));

  //   %xabs_s = select i1 %cmpx_s, float %src_s.chan0, float %xneg_s
  llvm::Value *float_xabs_s_1391 =
      this->CreateSelect(int1_cmpx_s_1390, float_address_0, float_xneg_s_1389, VALUE_NAME("xabs_s"));

  //   %yneg_s = fsub float 0.000000e+00, %src_s.chan1
  llvm::Value *float_yneg_s_1392 = this->CreateFSub(zero, float_address_1, VALUE_NAME("yneg_s"));

  //   %cmpy_s = fcmp oge float %src_s.chan1, 0.000000e+00
  llvm::Value *int1_cmpy_s_1393 =
      this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_address_1, zero, VALUE_NAME("cmpy_s"));

  //   %yabs_s = select i1 %cmpy_s, float %src_s.chan1, float %yneg_s
  llvm::Value *float_yabs_s_1394 =
      this->CreateSelect(int1_cmpy_s_1393, float_address_1, float_yneg_s_1392, VALUE_NAME("yabs_s"));

  //   %aineg_s = fsub float 0.000000e+00, %src_s.chan2
  llvm::Value *float_aineg_s_1395 = this->CreateFSub(zero, float_address_2, VALUE_NAME("aineg_s"));

  //   %cmpai_s = fcmp oge float %src_s.chan2, 0.000000e+00
  llvm::Value *int1_cmpai_s_1396 =
      this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_address_2, zero, VALUE_NAME("cmpai_s"));

  //   %aiabs_s = select i1 %cmpai_s, float %src_s.chan2, float %aineg_s
  llvm::Value *float_aiabs_s_1397 =
      this->CreateSelect(int1_cmpai_s_1396, float_address_2, float_aineg_s_1395, VALUE_NAME("aiabs_s"));

  //   %oge0_s = fcmp oge float %xabs_s, %yabs_s
  llvm::Value *int1_oge0_s_1398 =
      this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_xabs_s_1391, float_yabs_s_1394, VALUE_NAME("oge0_s"));

  //   %max1_s = select i1 %oge0_s, float %xabs_s, float %yabs_s
  llvm::Value *float_max1_s_1399 =
      this->CreateSelect(int1_oge0_s_1398, float_xabs_s_1391, float_yabs_s_1394, VALUE_NAME("max1_s"));

  //   %oge1_s = fcmp oge float %max1_s, %aiabs_s
  llvm::Value *int1_oge1_s_1400 =
      this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_max1_s_1399, float_aiabs_s_1397, VALUE_NAME("oge1_s"));

  //   %max2_s = select i1 %oge1_s, float %max1_s, float %aiabs_s
  llvm::Value *float_max2_s_1401 =
      this->CreateSelect(int1_oge1_s_1400, float_max1_s_1399, float_aiabs_s_1397, VALUE_NAME("max2_s"));

  //   %xcube_s = fdiv float %src_s.chan0, %max2_s
  llvm::Value *float_xcube_s_1402 = this->CreateFDiv(float_address_0, float_max2_s_1401, VALUE_NAME("xcube_s"));

  //   %ycube_s = fdiv float %src_s.chan1, %max2_s
  llvm::Value *float_ycube_s_1403 = this->CreateFDiv(float_address_1, float_max2_s_1401, VALUE_NAME("ycube_s"));

  //   %aicube_s = fdiv float %src_s.chan2, %max2_s
  llvm::Value *float_aicube_s_1404 = this->CreateFDiv(float_address_2, float_max2_s_1401, VALUE_NAME("aicube_s"));

  SampleParamsFromCube CubeRetParams;

  CubeRetParams.float_xcube = float_xcube_s_1402;
  CubeRetParams.float_ycube = float_ycube_s_1403;
  CubeRetParams.float_aicube = float_aicube_s_1404;
  CubeRetParams.float_address_3 = float_address_3;
  CubeRetParams.int32_textureIdx = int32_textureIdx;
  CubeRetParams.int32_sampler = int32_sampler;
  CubeRetParams.offsetU = m_int0;
  CubeRetParams.offsetV = m_int0;
  CubeRetParams.offsetR = m_int0;

  return CubeRetParams;
}

template <typename T, typename Inserter>
inline SampleD_DC_FromCubeParams
LLVM3DBuilder<T, Inserter>::Prepare_SAMPLE_D_DC_Cube_Params(SampleD_DC_FromCubeParams &params) {
  return Prepare_SAMPLE_D_DC_Cube_Params(
      params.float_src_u, params.float_src_v, params.float_src_r, params.float_src_ai, params.dxu, params.dxv,
      params.dxr, params.dyu, params.dyv, params.dyr, params.int32_pairedTextureIdx, params.int32_textureIdx,
      params.int32_sampler, params.int32_offsetU, params.int32_offsetV, params.int32_offsetW);
}

template <typename T, typename Inserter>
inline SampleD_DC_FromCubeParams LLVM3DBuilder<T, Inserter>::Prepare_SAMPLE_D_DC_Cube_Params(
    llvm::Value *float_src_r, llvm::Value *float_src_s, llvm::Value *float_src_t, llvm::Value *float_src_ai,
    llvm::Value *float_drdx, llvm::Value *float_dsdx, llvm::Value *float_dtdx, llvm::Value *float_drdy,
    llvm::Value *float_dsdy, llvm::Value *float_dtdy, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW) {
  return Prepare_SAMPLE_D_DC_Cube_Params(float_src_r, float_src_s, float_src_t, float_src_ai, float_drdx, float_dsdx,
                                         float_dtdx, float_drdy, float_dsdy, float_dtdy,
                                         llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx,
                                         int32_sampler, int32_offsetU, int32_offsetV, int32_offsetW);
}

template <typename T, typename Inserter>
inline SampleD_DC_FromCubeParams LLVM3DBuilder<T, Inserter>::Prepare_SAMPLE_D_DC_Cube_Params(
    llvm::Value *float_src_r, llvm::Value *float_src_s, llvm::Value *float_src_t, llvm::Value *float_src_ai,
    llvm::Value *float_drdx, llvm::Value *float_dsdx, llvm::Value *float_dtdx, llvm::Value *float_drdy,
    llvm::Value *float_dsdy, llvm::Value *float_dtdy, llvm::Value *int32_pairedTextureIdx_356,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_offsetW) {
  //  For cube texture sampling, sampling instruction must receive proper cube face ID
  //  together with coordinates projected onto that face. Gradients also have to be transformed
  //  into the same (cube face) address space.
  //  To achieve this we first have to find a major coordinate, then normalize coordinates
  //  and select remaining ones as u/v coordinates for the face. Because of the cube texture layout
  //  in memory (as 6 2D faces) this sometimes involves changing the coordinate direction (sign).
  //  Gradients are transformed using quotient rule for derivatives:
  //        (fA/fB)' = (fA'*fB - fB'*fA)/fB^2
  //  where fA and fB are base functions, i.e. base cube coordinates in this case.
  //  Note that we first normalize coordinates and all derivatives, so calculations
  //  here use the form:
  //        (fA/fB)' = [fA'/fB] - [fB'/fB]*[fA/fB]
  IGC_UNUSED(int32_offsetU);
  IGC_UNUSED(int32_offsetV);
  IGC_UNUSED(int32_offsetW);
  IGC_ASSERT(nullptr != this->GetInsertBlock());
  llvm::Function *const parentFunc = this->GetInsertBlock()->getParent();
  IGC_ASSERT(nullptr != float_src_r);
  llvm::Type *const coordType = float_src_r->getType();
  IGC_ASSERT(nullptr != coordType);
  IGC_ASSERT(coordType->isFloatTy() || coordType->isHalfTy());

  llvm::Value *zero = llvm::ConstantFP::get(coordType, 0.0);

  // Create coordinate absolute values to look for major.
  llvm::Value *float_abs_r = this->CreateFAbs(float_src_r);
  llvm::Value *float_abs_s = this->CreateFAbs(float_src_s);
  llvm::Value *float_abs_t = this->CreateFAbs(float_src_t);

  {
    llvm::BasicBlock *currentBlock = this->GetInsertBlock();
    bool shouldSplitBB = this->GetInsertPoint() != currentBlock->end();

    // Create basic blocks.
    llvm::BasicBlock *block_final = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cubefinal_block"));

    llvm::BasicBlock *block_major_t = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cubemajor_t_block"));
    llvm::BasicBlock *block_not_t = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cubenott_block"));
    llvm::BasicBlock *block_zp = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cube_face_zp_block"));
    llvm::BasicBlock *block_zm = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cube_face_zm_block"));

    llvm::BasicBlock *block_major_s = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cubemajor_s_block"));
    llvm::BasicBlock *block_yp = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cube_face_yp_block"));
    llvm::BasicBlock *block_ym = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cube_face_ym_block"));

    llvm::BasicBlock *block_major_r = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cubemajor_r_block"));
    llvm::BasicBlock *block_xp = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cube_face_xp_block"));
    llvm::BasicBlock *block_xm = llvm::BasicBlock::Create(this->getContext(), VALUE_NAME("cube_face_xm_block"));

    // Find the major coordinate (and thus cube face), precedence is Z,Y,X.
    llvm::Value *int1_cmp_tges =
        this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_abs_t, float_abs_s, VALUE_NAME("cmp_tges"));

    llvm::Value *int1_cmp_tger =
        this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_abs_t, float_abs_r, VALUE_NAME("cmp_tger"));

    llvm::Value *int1_tgesr = this->CreateAnd(int1_cmp_tger, int1_cmp_tges);

    // Major coordinate is T, faces could be +Z or -Z
    llvm::BasicBlock *splitBlock = nullptr;
    if (shouldSplitBB) {
      IGC_ASSERT(nullptr != currentBlock);
      IGC_ASSERT(currentBlock->getTerminator());
      splitBlock = currentBlock->splitBasicBlock(this->GetInsertPoint()->getNextNode());
      currentBlock->getTerminator()->eraseFromParent();
      this->SetInsertPoint(currentBlock);
    }
    this->CreateCondBr(int1_tgesr, block_major_t, block_not_t);
    this->SetInsertPoint(block_major_t);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_major_t);

    // Normalize coordinates and gradients.
    llvm::Value *float_tnorm_r = this->CreateFDiv(float_src_r, float_abs_t, VALUE_NAME("tnorm_r"));
    llvm::Value *float_tnorm_s = this->CreateFDiv(float_src_s, float_abs_t, VALUE_NAME("tnorm_s"));
    llvm::Value *float_tnorm_drdx = this->CreateFDiv(float_drdx, float_abs_t, VALUE_NAME("tnorm_drdx"));
    llvm::Value *float_tnorm_drdy = this->CreateFDiv(float_drdy, float_abs_t, VALUE_NAME("tnorm_drdy"));
    llvm::Value *float_tnorm_dsdx = this->CreateFDiv(float_dsdx, float_abs_t, VALUE_NAME("tnorm_dsdx"));
    llvm::Value *float_tnorm_dsdy = this->CreateFDiv(float_dsdy, float_abs_t, VALUE_NAME("tnorm_dsdy"));
    llvm::Value *float_tnorm_dtdx = this->CreateFDiv(float_dtdx, float_abs_t, VALUE_NAME("tnorm_dtdx"));
    llvm::Value *float_tnorm_dtdy = this->CreateFDiv(float_dtdy, float_abs_t, VALUE_NAME("tnorm_dtdy"));

    // Select positive or negative face.
    llvm::Value *int1_cmpx_t = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_src_t, zero, VALUE_NAME("cmpx_t"));
    this->CreateCondBr(int1_cmpx_t, block_zp, block_zm);
    this->SetInsertPoint(block_zp);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_zp);

    // Face +Z,
    // major = neg T
    // u     = R
    // v     = neg S

    llvm::Value *float_face_zp_id = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(coordType, 4.0));

    // Select u from s/r/t
    llvm::Value *float_face_zp_u = float_tnorm_r;

    // Select v from s/r/t
    llvm::Value *float_face_zp_v = this->CreateFNeg(float_tnorm_s, VALUE_NAME("face_zp_v"));

    // du/dx = dm * u + d{s/r/t}/dx
    llvm::Value *float_neg_dmx4 = this->CreateFNeg(float_tnorm_dtdx, VALUE_NAME("neg_dmx"));
    llvm::Value *float_dmxu4 = this->CreateFMul(float_neg_dmx4, float_tnorm_r, VALUE_NAME("dmxu"));
    llvm::Value *float_face_zp_dudx = this->CreateFAdd(float_dmxu4, float_tnorm_drdx, VALUE_NAME("face_zp_dudx"));

    // du/dy = dm * u + d{s/r/t}/dy
    llvm::Value *float_neg_dmy4 = this->CreateFNeg(float_tnorm_dtdy, VALUE_NAME("neg_dmy"));
    llvm::Value *float_dmyu4 = this->CreateFMul(float_neg_dmy4, float_tnorm_r, VALUE_NAME("dmyu"));
    llvm::Value *float_face_zp_dudy = this->CreateFAdd(float_dmyu4, float_tnorm_drdy, VALUE_NAME("face_zp_dvdx"));

    // dv/dx = dm * v + d{s/r/t}/dx
    llvm::Value *float_dmxv4 = this->CreateFMul(float_tnorm_dtdx, float_tnorm_s, VALUE_NAME("dmxv"));
    llvm::Value *float_face_zp_dvdx = this->CreateFSub(float_dmxv4, float_tnorm_dsdx, VALUE_NAME("face_zp_dvdx"));

    // dv/dy = dm * v + d{s/r/t}/dy
    llvm::Value *float_dmyv4 = this->CreateFMul(float_tnorm_dtdy, float_tnorm_s, VALUE_NAME("dmyv"));
    llvm::Value *float_face_zp_dvdy = this->CreateFSub(float_dmyv4, float_tnorm_dsdy, VALUE_NAME("face_zp_dvdy"));

    this->CreateBr(block_final);
    this->SetInsertPoint(block_zm);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_zm);

    // Face -Z,
    // major = T
    // u     = neg R
    // v     = neg S

    llvm::Value *float_face_zm_id = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(coordType, 5.0));

    // Select u from s/r/t
    llvm::Value *float_face_zm_u = this->CreateFNeg(float_tnorm_r, VALUE_NAME("face_zm_u"));

    // Select v from s/r/t
    llvm::Value *float_face_zm_v = this->CreateFNeg(float_tnorm_s, VALUE_NAME("face_zm_v"));

    // du/dx = dm * u + d{s/r/t}/dx
    llvm::Value *float_dmxu5 = this->CreateFMul(float_tnorm_dtdx, float_face_zm_u, VALUE_NAME("dmxu"));
    llvm::Value *float_face_zm_dudx = this->CreateFSub(float_dmxu5, float_tnorm_drdx, VALUE_NAME("face_zm_dudx"));

    // du/dy = dm * u + d{s/r/t}/dy
    llvm::Value *float_dmyu5 = this->CreateFMul(float_tnorm_dtdy, float_face_zm_u, VALUE_NAME("dmyu"));
    llvm::Value *float_face_zm_dudy = this->CreateFSub(float_dmyu5, float_tnorm_drdy, VALUE_NAME("face_zm_dvdx"));

    // dv/dx = dm * v + d{s/r/t}/dx
    llvm::Value *float_dmxv5 = this->CreateFMul(float_tnorm_dtdx, float_face_zm_v, VALUE_NAME("dmxv"));
    llvm::Value *float_face_zm_dvdx = this->CreateFSub(float_dmxv5, float_tnorm_dsdx, VALUE_NAME("face_zm_dvdx"));

    // dv/dy = dm * v + d{s/r/t}/dy
    llvm::Value *float_dmyv5 = this->CreateFMul(float_tnorm_dtdy, float_face_zm_v, VALUE_NAME("dmyv"));
    llvm::Value *float_face_zm_dvdy = this->CreateFSub(float_dmyv5, float_tnorm_dsdy, VALUE_NAME("face_zm_dvdy"));

    this->CreateBr(block_final);
    this->SetInsertPoint(block_not_t);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_not_t);

    // Choose major S or R.
    llvm::Value *int1_cmp_sger =
        this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_abs_s, float_abs_r, VALUE_NAME("cmp_sger"));

    // Major coordinate is S, faces could be +Y or -Y
    this->CreateCondBr(int1_cmp_sger, block_major_s, block_major_r);
    this->SetInsertPoint(block_major_s);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_major_s);

    // Normalize coordinates and gradients.
    llvm::Value *float_snorm_r = this->CreateFDiv(float_src_r, float_abs_s, VALUE_NAME("snorm_r"));
    llvm::Value *float_snorm_t = this->CreateFDiv(float_src_t, float_abs_s, VALUE_NAME("snorm_t"));
    llvm::Value *float_snorm_drdx = this->CreateFDiv(float_drdx, float_abs_s, VALUE_NAME("snorm_drdx"));
    llvm::Value *float_snorm_drdy = this->CreateFDiv(float_drdy, float_abs_s, VALUE_NAME("snorm_drdy"));
    llvm::Value *float_snorm_dsdx = this->CreateFDiv(float_dsdx, float_abs_s, VALUE_NAME("snorm_dsdx"));
    llvm::Value *float_snorm_dsdy = this->CreateFDiv(float_dsdy, float_abs_s, VALUE_NAME("snorm_dsdy"));
    llvm::Value *float_snorm_dtdx = this->CreateFDiv(float_dtdx, float_abs_s, VALUE_NAME("snorm_dtdx"));
    llvm::Value *float_snorm_dtdy = this->CreateFDiv(float_dtdy, float_abs_s, VALUE_NAME("snorm_dtdy"));

    // Select positive or negative face.
    llvm::Value *int1_cmpx_s = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_src_s, zero, VALUE_NAME("cmpx_s"));
    this->CreateCondBr(int1_cmpx_s, block_yp, block_ym);
    this->SetInsertPoint(block_yp);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_yp);

    // Face +Y,
    // major = neg S
    // u     = R
    // v     = T

    llvm::Value *float_face_yp_id = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(coordType, 2.0));

    // Select u from s/r/t
    llvm::Value *float_face_yp_u = float_snorm_r;

    // Select v from s/r/t
    llvm::Value *float_face_yp_v = float_snorm_t;

    // du/dx = dm * u + d{s/r/t}/dx
    llvm::Value *float_neg_dmx2 = this->CreateFNeg(float_snorm_dsdx, VALUE_NAME("neg_dmx"));
    llvm::Value *float_dmxu2 = this->CreateFMul(float_neg_dmx2, float_snorm_r, VALUE_NAME("dmxu"));
    llvm::Value *float_face_yp_dudx = this->CreateFAdd(float_dmxu2, float_snorm_drdx, VALUE_NAME("face_yp_dudx"));

    // du/dy = dm * u + d{s/r/t}/dy
    llvm::Value *float_neg_dmy2 = this->CreateFNeg(float_snorm_dsdy, VALUE_NAME("neg_dmy"));
    llvm::Value *float_dmyu2 = this->CreateFMul(float_neg_dmy2, float_snorm_r, VALUE_NAME("dmyu"));
    llvm::Value *float_face_yp_dudy = this->CreateFAdd(float_dmyu2, float_snorm_drdy, VALUE_NAME("face_yp_dvdx"));

    // dv/dx = dm * v + d{s/r/t}/dx
    llvm::Value *float_dmxv2 = this->CreateFMul(float_neg_dmx2, float_snorm_t, VALUE_NAME("dmxv"));
    llvm::Value *float_face_yp_dvdx = this->CreateFAdd(float_dmxv2, float_snorm_dtdx, VALUE_NAME("face_yp_dvdx"));

    // dv/dy = dm * v + d{s/r/t}/dy
    llvm::Value *float_dmyv2 = this->CreateFMul(float_neg_dmy2, float_snorm_t, VALUE_NAME("dmyv"));
    llvm::Value *float_face_yp_dvdy = this->CreateFAdd(float_dmyv2, float_snorm_dtdy, VALUE_NAME("face_yp_dvdy"));

    this->CreateBr(block_final);
    this->SetInsertPoint(block_ym);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_ym);

    // Face -Y,
    // major = S
    // u     = R
    // v     = neg T

    llvm::Value *float_face_ym_id = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(coordType, 3.0));

    // Select u from s/r/t
    llvm::Value *float_face_ym_u = float_snorm_r;

    // Select v from s/r/t
    llvm::Value *float_face_ym_v = this->CreateFNeg(float_snorm_t, VALUE_NAME("face_ym_v"));

    // du/dx = dm * u + d{s/r/t}/dx
    llvm::Value *float_dmxu3 = this->CreateFMul(float_snorm_dsdx, float_snorm_r, VALUE_NAME("dmxu"));
    llvm::Value *float_face_ym_dudx = this->CreateFAdd(float_dmxu3, float_snorm_drdx, VALUE_NAME("face_ym_dudx"));

    // du/dy = dm * u + d{s/r/t}/dy
    llvm::Value *float_dmyu3 = this->CreateFMul(float_snorm_dsdy, float_snorm_r, VALUE_NAME("dmyu"));
    llvm::Value *float_face_ym_dudy = this->CreateFAdd(float_dmyu3, float_snorm_drdy, VALUE_NAME("face_ym_dvdx"));

    // dv/dx = dm * v + d{s/r/t}/dx
    llvm::Value *float_dmxv3 = this->CreateFMul(float_snorm_dsdx, float_face_ym_v, VALUE_NAME("dmxv"));
    llvm::Value *float_face_ym_dvdx = this->CreateFSub(float_dmxv3, float_snorm_dtdx, VALUE_NAME("face_ym_dvdx"));

    // dv/dy = dm * v + d{s/r/t}/dy
    llvm::Value *float_dmyv3 = this->CreateFMul(float_snorm_dsdx, float_face_ym_v, VALUE_NAME("dmyv"));
    llvm::Value *float_face_ym_dvdy = this->CreateFSub(float_dmyv3, float_snorm_dtdy, VALUE_NAME("face_ym_dvdy"));

    this->CreateBr(block_final);
    this->SetInsertPoint(block_major_r);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_major_r);

    // Major coordinate is R, faces could be +X or -X

    // Normalize coordinates and gradients.
    llvm::Value *float_rnorm_s = this->CreateFDiv(float_src_s, float_abs_r, VALUE_NAME("rnorm_r"));
    llvm::Value *float_rnorm_t = this->CreateFDiv(float_src_t, float_abs_r, VALUE_NAME("rnorm_t"));
    llvm::Value *float_rnorm_drdx = this->CreateFDiv(float_drdx, float_abs_r, VALUE_NAME("rnorm_drdx"));
    llvm::Value *float_rnorm_drdy = this->CreateFDiv(float_drdy, float_abs_r, VALUE_NAME("rnorm_drdy"));
    llvm::Value *float_rnorm_dsdx = this->CreateFDiv(float_dsdx, float_abs_r, VALUE_NAME("rnorm_dsdx"));
    llvm::Value *float_rnorm_dsdy = this->CreateFDiv(float_dsdy, float_abs_r, VALUE_NAME("rnorm_dsdy"));
    llvm::Value *float_rnorm_dtdx = this->CreateFDiv(float_dtdx, float_abs_r, VALUE_NAME("rnorm_dtdx"));
    llvm::Value *float_rnorm_dtdy = this->CreateFDiv(float_dtdy, float_abs_r, VALUE_NAME("rnorm_dtdy"));

    // Select positive or negative face.
    llvm::Value *int1_cmpx_r = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, float_src_r, zero, VALUE_NAME("cmpx_r"));
    this->CreateCondBr(int1_cmpx_r, block_xp, block_xm);
    this->SetInsertPoint(block_xp);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_xp);

    // Face +X,
    // major = neg R
    // u     = neg T
    // v     = neg S

    llvm::Value *float_face_xp_id = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(coordType, 0.0));

    // Select u from s/r/t
    llvm::Value *float_face_xp_u = this->CreateFNeg(float_rnorm_t, VALUE_NAME("face_xp_u"));

    // Select v from s/r/t
    llvm::Value *float_face_xp_v = this->CreateFNeg(float_rnorm_s, VALUE_NAME("face_xp_v"));

    // du/dx = dm * u + d{s/r/t}/dx
    llvm::Value *float_dmxu0 = this->CreateFMul(float_rnorm_drdx, float_rnorm_t, VALUE_NAME("dmxu"));
    llvm::Value *float_face_xp_dudx = this->CreateFSub(float_dmxu0, float_rnorm_dtdx, VALUE_NAME("face_xp_dudx"));

    // du/dy = dm * u + d{s/r/t}/dy
    llvm::Value *float_dmyu0 = this->CreateFMul(float_rnorm_drdy, float_rnorm_t, VALUE_NAME("dmyu"));
    llvm::Value *float_face_xp_dudy = this->CreateFSub(float_dmyu0, float_rnorm_dtdy, VALUE_NAME("face_xp_dvdx"));

    // dv/dx = dm * v + d{s/r/t}/dx
    llvm::Value *float_dmxv0 = this->CreateFMul(float_rnorm_drdx, float_rnorm_s, VALUE_NAME("dmxv"));
    llvm::Value *float_face_xp_dvdx = this->CreateFSub(float_dmxv0, float_rnorm_dsdx, VALUE_NAME("face_xp_dvdx"));

    // dv/dy = dm * v + d{s/r/t}/dy
    llvm::Value *float_dmyv0 = this->CreateFMul(float_rnorm_drdy, float_rnorm_s, VALUE_NAME("dmyv"));
    llvm::Value *float_face_xp_dvdy = this->CreateFSub(float_dmyv0, float_rnorm_dsdy, VALUE_NAME("face_xp_dvdy"));

    this->CreateBr(block_final);
    this->SetInsertPoint(block_xm);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_xm);

    // Face -X,
    // major = R
    // u     = T
    // v     = neg S

    llvm::Value *float_face_xm_id = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(coordType, 1.0));

    // Select u from s/r/t
    llvm::Value *float_face_xm_u = float_rnorm_t;

    // Select v from s/r/t
    llvm::Value *float_face_xm_v = this->CreateFNeg(float_rnorm_s, VALUE_NAME("face_xm_v"));

    // du/dx = dm * u + d{s/r/t}/dx
    llvm::Value *float_dmxu1 = this->CreateFMul(float_rnorm_drdx, float_rnorm_t, VALUE_NAME("dmxu"));
    llvm::Value *float_face_xm_dudx = this->CreateFAdd(float_dmxu1, float_rnorm_dtdx, VALUE_NAME("face_xm_dudx"));

    // du/dy = dm * u + d{s/r/t}/dy
    llvm::Value *float_dmyu1 = this->CreateFMul(float_rnorm_drdy, float_rnorm_t, VALUE_NAME("dmyu"));
    llvm::Value *float_face_xm_dudy = this->CreateFAdd(float_dmyu1, float_rnorm_dtdx, VALUE_NAME("face_xm_dvdx"));

    // dv/dx = dm * v + d{s/r/t}/dx
    llvm::Value *float_dmxv1 = this->CreateFMul(float_rnorm_drdx, float_face_xm_v, VALUE_NAME("dmxv"));
    llvm::Value *float_face_xm_dvdx = this->CreateFSub(float_dmxv1, float_rnorm_dsdx, VALUE_NAME("face_xm_dvdx"));

    // dv/dy = dm * v + d{s/r/t}/dy
    llvm::Value *float_dmyv1 = this->CreateFMul(float_rnorm_drdy, float_face_xm_v, VALUE_NAME("dmyv"));
    llvm::Value *float_face_xm_dvdy = this->CreateFSub(float_dmyv1, float_rnorm_dsdy, VALUE_NAME("face_xm_dvdy"));

    this->CreateBr(block_final);
    this->SetInsertPoint(block_final);
    IGCLLVM::pushBackBasicBlock(parentFunc, block_final);

    llvm::PHINode *phi_u = this->CreatePHI(coordType, 6, VALUE_NAME("phi_u"));
    phi_u->addIncoming(float_face_xp_u, block_xp);
    phi_u->addIncoming(float_face_xm_u, block_xm);
    phi_u->addIncoming(float_face_yp_u, block_yp);
    phi_u->addIncoming(float_face_ym_u, block_ym);
    phi_u->addIncoming(float_face_zp_u, block_zp);
    phi_u->addIncoming(float_face_zm_u, block_zm);

    llvm::PHINode *phi_v = this->CreatePHI(coordType, 6, VALUE_NAME("phi_v"));
    phi_v->addIncoming(float_face_xp_v, block_xp);
    phi_v->addIncoming(float_face_xm_v, block_xm);
    phi_v->addIncoming(float_face_yp_v, block_yp);
    phi_v->addIncoming(float_face_ym_v, block_ym);
    phi_v->addIncoming(float_face_zp_v, block_zp);
    phi_v->addIncoming(float_face_zm_v, block_zm);

    llvm::PHINode *phi_dudx = this->CreatePHI(coordType, 6, VALUE_NAME("phi_dudx"));
    phi_dudx->addIncoming(float_face_xp_dudx, block_xp);
    phi_dudx->addIncoming(float_face_xm_dudx, block_xm);
    phi_dudx->addIncoming(float_face_yp_dudx, block_yp);
    phi_dudx->addIncoming(float_face_ym_dudx, block_ym);
    phi_dudx->addIncoming(float_face_zp_dudx, block_zp);
    phi_dudx->addIncoming(float_face_zm_dudx, block_zm);

    llvm::PHINode *phi_dudy = this->CreatePHI(coordType, 6, VALUE_NAME("phi_dudy"));
    phi_dudy->addIncoming(float_face_xp_dudy, block_xp);
    phi_dudy->addIncoming(float_face_xm_dudy, block_xm);
    phi_dudy->addIncoming(float_face_yp_dudy, block_yp);
    phi_dudy->addIncoming(float_face_ym_dudy, block_ym);
    phi_dudy->addIncoming(float_face_zp_dudy, block_zp);
    phi_dudy->addIncoming(float_face_zm_dudy, block_zm);

    llvm::PHINode *phi_dvdx = this->CreatePHI(coordType, 6, VALUE_NAME("phi_dvdx"));
    phi_dvdx->addIncoming(float_face_xp_dvdx, block_xp);
    phi_dvdx->addIncoming(float_face_xm_dvdx, block_xm);
    phi_dvdx->addIncoming(float_face_yp_dvdx, block_yp);
    phi_dvdx->addIncoming(float_face_ym_dvdx, block_ym);
    phi_dvdx->addIncoming(float_face_zp_dvdx, block_zp);
    phi_dvdx->addIncoming(float_face_zm_dvdx, block_zm);

    llvm::PHINode *phi_dvdy = this->CreatePHI(coordType, 6, VALUE_NAME("phi_dvdy"));
    phi_dvdy->addIncoming(float_face_xp_dvdy, block_xp);
    phi_dvdy->addIncoming(float_face_xm_dvdy, block_xm);
    phi_dvdy->addIncoming(float_face_yp_dvdy, block_yp);
    phi_dvdy->addIncoming(float_face_ym_dvdy, block_ym);
    phi_dvdy->addIncoming(float_face_zp_dvdy, block_zp);
    phi_dvdy->addIncoming(float_face_zm_dvdy, block_zm);

    llvm::PHINode *phi_face_id = this->CreatePHI(coordType, 6, VALUE_NAME("phi_face_id"));
    phi_face_id->addIncoming(float_face_xp_id, block_xp);
    phi_face_id->addIncoming(float_face_xm_id, block_xm);
    phi_face_id->addIncoming(float_face_yp_id, block_yp);
    phi_face_id->addIncoming(float_face_ym_id, block_ym);
    phi_face_id->addIncoming(float_face_zp_id, block_zp);
    phi_face_id->addIncoming(float_face_zm_id, block_zm);

    if (shouldSplitBB) {
      llvm::BranchInst *brInst = this->CreateBr(splitBlock);
      this->SetInsertPoint(brInst);
    }

    SampleD_DC_FromCubeParams D_DC_CUBE_params;

    D_DC_CUBE_params.float_src_u = phi_u;
    D_DC_CUBE_params.dxu = phi_dudx;
    D_DC_CUBE_params.dyu = phi_dudy;
    D_DC_CUBE_params.float_src_v = phi_v;
    D_DC_CUBE_params.dxv = phi_dvdx;
    D_DC_CUBE_params.dyv = phi_dvdy;
    D_DC_CUBE_params.float_src_r = phi_face_id;
    D_DC_CUBE_params.dxr = zero;
    D_DC_CUBE_params.dyr = zero;
    D_DC_CUBE_params.float_src_ai = float_src_ai;
    D_DC_CUBE_params.int32_pairedTextureIdx = int32_pairedTextureIdx_356;
    D_DC_CUBE_params.int32_textureIdx = int32_textureIdx;
    D_DC_CUBE_params.int32_sampler = int32_sampler;
    D_DC_CUBE_params.int32_offsetU = m_int0;
    D_DC_CUBE_params.int32_offsetV = m_int0;
    D_DC_CUBE_params.int32_offsetW = m_int0;

    return D_DC_CUBE_params;
  }
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFAbs(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *fabs = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::fabs, V->getType());
  return this->CreateCall(fabs, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFSat(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *fsat =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_fsat, V->getType());
  return this->CreateCall(fsat, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateF16TOF32(llvm::Value *f16_src) {
  llvm::Value *f32_dst = this->CreateFPExt(f16_src, this->getFloatTy(), VALUE_NAME("src0_s"));
  return f32_dst;
}

/*****************************************************************************\
Description:
Returns true if additional conversion is required if given format is
128bit.

Input:
SURFACE_FORMAT format           - conversion format

Output:
bool - return value.

\*****************************************************************************/
template <typename T, typename Inserter>
bool LLVM3DBuilder<T, Inserter>::NeedConversionFor128FormatRead(IGC::SURFACE_FORMAT format) const {
  bool needsConversion = true;

  if ((format == IGC::SURFACE_FORMAT::SURFACE_FORMAT_R32G32B32A32_FLOAT) ||
      (format == IGC::SURFACE_FORMAT::SURFACE_FORMAT_R32G32B32A32_UINT) ||
      (format == IGC::SURFACE_FORMAT::SURFACE_FORMAT_R32G32B32A32_SINT)) {
    needsConversion = false;
  }

  return needsConversion;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_UBFE(llvm::Value *int32_width, llvm::Value *int32_offset,
                                                            llvm::Value *int32_source) {
  //   %res = call i32 @llvm.GenISA.ubfe(i32 %src0_s, i32 %src1_s, i32 %src2_s)
  llvm::Value *packed_params[] = {int32_width, int32_offset, int32_source};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_ubfe(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_IBFE(llvm::Value *int32_width, llvm::Value *int32_offset,
                                                            llvm::Value *int32_source) {
  //   %res = call i32 @llvm.GenISA.ibfe(i32 %int32_width, i32 %int32_offset, i32 %int32_source)
  llvm::Value *packed_params[] = {int32_width, int32_offset, int32_source};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_ibfe(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_BFI(llvm::Value *int32_width, llvm::Value *int32_offset,
                                                           llvm::Value *int32_source, llvm::Value *int32_replace) {
  llvm::Value *packed_params[] = {int32_width, int32_offset, int32_source, int32_replace};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_bfi(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_BFREV(llvm::Value *int32_source) {
  llvm::Value *packed_params[] = {int32_source};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_bfrev(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_FirstBitHi(llvm::Value *int32_source) {
  llvm::Value *packed_params[] = {int32_source};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_firstbitHi(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_FirstBitLo(llvm::Value *int32_source) {
  llvm::Value *packed_params[] = {int32_source};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_firstbitLo(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::Create_FirstBitShi(llvm::Value *int32_source) {
  llvm::Value *packed_params[] = {int32_source};
  llvm::CallInst *int32_res = llvm::cast<llvm::CallInst>(this->CreateCall(llvm_GenISA_firstbitShi(), packed_params));
  return int32_res;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_indirectLoad(llvm::Value *srcBuffer, llvm::Value *offset,
                                                                    llvm::Value *alignment, llvm::Type *returnType,
                                                                    bool isVolatile /* false */) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Type *types[] = {returnType, srcBuffer->getType()};
  llvm::Function *pfuncLdPtr =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ldrawvector_indexed, types);
  return this->CreateCall4(pfuncLdPtr, srcBuffer, offset, alignment, this->getInt1(isVolatile));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_indirectStore(llvm::Value *srcBuffer, llvm::Value *offset,
                                                                     llvm::Value *data, bool isVolatile /* false */) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Type *types[] = {
      srcBuffer->getType(),
      data->getType(),
  };
  llvm::GenISAIntrinsic::ID id = data->getType()->isVectorTy() ? llvm::GenISAIntrinsic::GenISA_storerawvector_indexed
                                                               : llvm::GenISAIntrinsic::GenISA_storeraw_indexed;
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, id, types);
  llvm::Value *alignment = this->getInt32(data->getType()->getScalarSizeInBits() / 8);
  return this->CreateCall5(pFunc, srcBuffer, offset, data, alignment, this->getInt1(isVolatile));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_atomicCounterIncrement(llvm::Value *srcBuffer) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_atomiccounterinc,
                                                                srcBuffer->getType());
  return this->CreateCall(pFunc, srcBuffer);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_atomicCounterDecrement(llvm::Value *srcBuffer) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(
      module, llvm::GenISAIntrinsic::GenISA_atomiccounterpredec, srcBuffer->getType());
  return this->CreateCall(pFunc, srcBuffer);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::createThreadLocalId(unsigned int dim) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getInt32Ty());
  return this->CreateCall(pFunc, this->getInt32(IGC::THREAD_ID_IN_GROUP_X + dim),
                          (dim == 0)   ? "LocalID_X"
                          : (dim == 1) ? "LocalID_Y"
                                       : "LocalID_Z");
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::createGroupId(unsigned int dim) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::THREAD_GROUP_ID_X + dim)), this->getInt32Ty(),
                             (dim == 0)   ? "GroupID_X"
                             : (dim == 1) ? "GroupID_Y"
                                          : "GroupID_Z");
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFrc(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *frc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_frc);
  return this->CreateCall(frc, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateSin(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *sin = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::sin, V->getType());
  return this->CreateCall(sin, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateCos(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *cos = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::cos, V->getType());
  return this->CreateCall(cos, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateSqrt(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *sqrt = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::sqrt, V->getType());
  return this->CreateCall(sqrt, V);
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFPow(llvm::Value *LHS, llvm::Value *RHS) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *fpow = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::pow, LHS->getType());
  return this->CreateCall2(fpow, LHS, RHS);
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFMax(llvm::Value *LHS, llvm::Value *RHS) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *fmax = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::maxnum, LHS->getType());
  return this->CreateCall2(fmax, LHS, RHS);
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFMin(llvm::Value *LHS, llvm::Value *RHS) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *fmin = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::minnum, LHS->getType());
  return this->CreateCall2(fmin, LHS, RHS);
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::CreateIMulH(llvm::Value *LHS, llvm::Value *RHS) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *imulh =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_imulH, LHS->getType());
  return this->CreateCall2(imulh, LHS, RHS);
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::CreateUMulH(llvm::Value *LHS, llvm::Value *RHS) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *umulh =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_umulH, LHS->getType());
  return this->CreateCall2(umulh, LHS, RHS);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDiscard(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *discard = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_discard);
  return this->CreateCall(discard, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFLog(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *flog = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::log2, V->getType());
  return this->CreateCall(flog, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFExp(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *fexp = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::exp2, V->getType());
  return this->CreateCall(fexp, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDFloor(llvm::Value *src) {
  llvm::Module *const mod = this->GetInsertBlock()->getParent()->getParent();
  IGC_ASSERT(nullptr != mod);
  llvm::Function *func = mod->getFunction("__builtin_floor_f64");
  if (func != nullptr) {
    return this->CreateCall(func, src);
  }

  // from OCL builtin: double @__builtin_spirv_floor_f64(double %x)
  static const char *const code =
      "define linkonce double @__builtin_floor_f64(double %x)                              \n"
      "    alwaysinline nounwind readnone {                                       \n"
      "  %1 = bitcast double %x to i64                                            \n"
      "  %2 = lshr i64 %1, 32                                                     \n"
      "  %3 = trunc i64 %2 to i32                                                 \n"
      "  %4 = lshr i64 %1, 52                                                     \n"
      "  %5 = trunc i64 %4 to i32                                                 \n"
      "  %6 = and i32 %5, 2047                                                    \n"
      "  %7 = sub nsw i32 1023, %6                                                \n"
      "  %8 = add nsw i32 %7, 52                                                  \n"
      "  %9 = add nsw i32 %7, 20                                                  \n"
      "  %10 = icmp sgt i32 %8, 32                                                \n"
      "  %11 = select i1 %10, i32 32, i32 %8                                      \n"
      "  %12 = icmp sgt i32 %9, 20                                                \n"
      "  %13 = select i1 %12, i32 20, i32 %9                                      \n"
      "  %14 = icmp sgt i32 %11, 0                                                \n"
      "  %15 = select i1 %14, i32 %11, i32 0                                      \n"
      "  %16 = icmp sgt i32 %13, 0                                                \n"
      "  %17 = select i1 %16, i32 %13, i32 0                                      \n"
      "  %18 = and i32 %15, 31                                                    \n"
      "  %19 = shl i32 -1, %18                                                    \n"
      "  %20 = and i32 %17, 31                                                    \n"
      "  %21 = shl i32 -1, %20                                                    \n"
      "  %22 = icmp ne i32 %15, 32                                                \n"
      "  %23 = select i1 %22, i32 %19, i32 0                                      \n"
      "  %24 = icmp eq i32 %17, 32                                                \n"
      "  %25 = icmp ult i32 %6, 1023                                              \n"
      "  %or.cond.i = or i1 %25, %24                                              \n"
      "  %maskValHigh32bit.0.i = select i1 %or.cond.i, i32 -2147483648, i32 %21   \n"
      "  %maskValLow32bit.0.i = select i1 %or.cond.i, i32 0, i32 %23              \n"
      "  %26 = trunc i64 %1 to i32                                                \n"
      "  %27 = and i32 %maskValLow32bit.0.i, %26                                  \n"
      "  %28 = and i32 %maskValHigh32bit.0.i, %3                                  \n"
      "  %29 = zext i32 %28 to i64                                                \n"
      "  %30 = shl nuw i64 %29, 32                                                \n"
      "  %31 = zext i32 %27 to i64                                                \n"
      "  %32 = or i64 %30, %31                                                    \n"
      "  %33 = bitcast i64 %32 to double                                          \n"
      "  %34 = sub i64 %1, %32                                                    \n"
      "  %35 = lshr i64 %34, 32                                                   \n"
      "  %36 = or i64 %35, %34                                                    \n"
      "  %37 = trunc i64 %36 to i32                                               \n"
      "  %38 = icmp eq i32 %37, 0                                                 \n"
      "  %39 = ashr i64 %1, 31                                                    \n"
      "  %.op = and i64 %39, -4616189618054758400                                 \n"
      "  %40 = bitcast i64 %.op to double                                         \n"
      "  %41 = select i1 %38, double -0.000000e+00, double %40                    \n"
      "  %42 = fadd double %33, %41                                               \n"
      "  ret double %42                                                           \n"
      "}";

  llvm::MemoryBufferRef codeBuf(code, "<string>");
  llvm::SMDiagnostic diagnostic;
  const bool failed = llvm::parseAssemblyInto(codeBuf, mod, nullptr, diagnostic);
  (void)failed;
  IGC_ASSERT_MESSAGE(false == failed, "Error parse llvm assembly");

  func = mod->getFunction("__builtin_floor_f64");
  return this->CreateCall(func, src);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateFloor(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  if (V->getType() == this->getDoubleTy()) {
    return CreateDFloor(V);
  } else {
    llvm::Function *floor = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::floor, V->getType());
    return this->CreateCall(floor, V);
  }
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDCeil(llvm::Value *src) {
  llvm::Module *const mod = this->GetInsertBlock()->getParent()->getParent();
  IGC_ASSERT(nullptr != mod);
  llvm::Function *func = mod->getFunction("__builtin_ceil_f64");
  if (func != nullptr) {
    return this->CreateCall(func, src);
  }

  // from OCL builtin: double @__builtin_spirv_ceil_f64(double %x)
  static const char *const code =
      "define linkonce double @__builtin_ceil_f64(double %x)                               \n"
      "    alwaysinline nounwind readnone {                                       \n"
      "  %1 = bitcast double %x to i64                                            \n"
      "  %2 = lshr i64 %1, 32                                                     \n"
      "  %3 = trunc i64 %2 to i32                                                 \n"
      "  %4 = lshr i64 %1, 52                                                     \n"
      "  %5 = trunc i64 %4 to i32                                                 \n"
      "  %6 = and i32 %5, 2047                                                    \n"
      "  %7 = sub nsw i32 1023, %6                                                \n"
      "  %8 = add nsw i32 %7, 52                                                  \n"
      "  %9 = add nsw i32 %7, 20                                                  \n"
      "  %10 = icmp sgt i32 %8, 32                                                \n"
      "  %11 = select i1 %10, i32 32, i32 %8                                      \n"
      "  %12 = icmp sgt i32 %9, 20                                                \n"
      "  %13 = select i1 %12, i32 20, i32 %9                                      \n"
      "  %14 = icmp sgt i32 %11, 0                                                \n"
      "  %15 = select i1 %14, i32 %11, i32 0                                      \n"
      "  %16 = icmp sgt i32 %13, 0                                                \n"
      "  %17 = select i1 %16, i32 %13, i32 0                                      \n"
      "  %18 = and i32 %15, 31                                                    \n"
      "  %19 = shl i32 -1, %18                                                    \n"
      "  %20 = and i32 %17, 31                                                    \n"
      "  %21 = shl i32 -1, %20                                                    \n"
      "  %22 = icmp ne i32 %15, 32                                                \n"
      "  %23 = select i1 %22, i32 %19, i32 0                                      \n"
      "  %24 = icmp eq i32 %17, 32                                                \n"
      "  %25 = icmp ult i32 %6, 1023                                              \n"
      "  %or.cond.i = or i1 %25, %24                                              \n"
      "  %maskValHigh32bit.0.i = select i1 %or.cond.i, i32 -2147483648, i32 %21   \n"
      "  %maskValLow32bit.0.i = select i1 %or.cond.i, i32 0, i32 %23              \n"
      "  %26 = trunc i64 %1 to i32                                                \n"
      "  %27 = and i32 %maskValLow32bit.0.i, %26                                  \n"
      "  %28 = and i32 %maskValHigh32bit.0.i, %3                                  \n"
      "  %29 = zext i32 %28 to i64                                                \n"
      "  %30 = shl nuw i64 %29, 32                                                \n"
      "  %31 = zext i32 %27 to i64                                                \n"
      "  %32 = or i64 %30, %31                                                    \n"
      "  %33 = bitcast i64 %32 to double                                          \n"
      "  %34 = sub i64 %1, %32                                                    \n"
      "  %35 = lshr i64 %34, 32                                                   \n"
      "  %36 = or i64 %35, %34                                                    \n"
      "  %37 = trunc i64 %36 to i32                                               \n"
      "  %38 = icmp eq i32 %37, 0                                                 \n"
      "  %39 = ashr i64 %1, 31                                                    \n"
      "  %40 = and i64 %39, -4607182418800017408                                  \n"
      "  %.op = add nsw i64 %40, 4607182418800017408                              \n"
      "  %41 = bitcast i64 %.op to double                                         \n"
      "  %42 = select i1 %38, double -0.000000e+00, double %41                    \n"
      "  %43 = fadd double %33, %42                                               \n"
      "  ret double %43                                                           \n"
      "}";

  llvm::MemoryBufferRef codeBuf(code, "<string>");
  llvm::SMDiagnostic diagnostic;
  const bool failed = llvm::parseAssemblyInto(codeBuf, mod, nullptr, diagnostic);
  (void)failed;
  IGC_ASSERT_MESSAGE(false == failed, "Error parse llvm assembly");

  func = mod->getFunction("__builtin_ceil_f64");

  return this->CreateCall(func, src);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateCeil(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  if (V->getType() == this->getDoubleTy()) {
    return CreateDCeil(V);
  } else {
    llvm::Function *ceil = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::ceil, V->getType());
    return this->CreateCall(ceil, V);
  }
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDTrunc(llvm::Value *src) {
  llvm::Module *const mod = this->GetInsertBlock()->getParent()->getParent();
  IGC_ASSERT(nullptr != mod);
  llvm::Function *func = mod->getFunction("__builtin_trunc_f64");
  if (func != nullptr) {
    return this->CreateCall(func, src);
  }

  // from OCL builtin: double @__builtin_spirv_trunc_f64(double %x)
  static const char *const code = "define linkonce double @__builtin_trunc_f64(double %x)                        \n"
                                  "    alwaysinline nounwind readnone {                                       \n"
                                  "  %1 = bitcast double %x to i64                                            \n"
                                  "  %2 = lshr i64 %1, 32                                                     \n"
                                  "  %3 = trunc i64 %2 to i32                                                 \n"
                                  "  %4 = lshr i64 %1, 52                                                     \n"
                                  "  %5 = trunc i64 %4 to i32                                                 \n"
                                  "  %6 = and i32 %5, 2047                                                    \n"
                                  "  %7 = sub nsw i32 1023, %6                                                \n"
                                  "  %8 = add nsw i32 %7, 52                                                  \n"
                                  "  %9 = add nsw i32 %7, 20                                                  \n"
                                  "  %10 = icmp sgt i32 %8, 32                                                \n"
                                  "  %11 = select i1 %10, i32 32, i32 %8                                      \n"
                                  "  %12 = icmp sgt i32 %9, 20                                                \n"
                                  "  %13 = select i1 %12, i32 20, i32 %9                                      \n"
                                  "  %14 = icmp sgt i32 %11, 0                                                \n"
                                  "  %15 = select i1 %14, i32 %11, i32 0                                      \n"
                                  "  %16 = icmp sgt i32 %13, 0                                                \n"
                                  "  %17 = select i1 %16, i32 %13, i32 0                                      \n"
                                  "  %18 = and i32 %15, 31                                                    \n"
                                  "  %19 = shl i32 -1, %18                                                    \n"
                                  "  %20 = and i32 %17, 31                                                    \n"
                                  "  %21 = shl i32 -1, %20                                                    \n"
                                  "  %22 = icmp ne i32 %15, 32                                                \n"
                                  "  %23 = select i1 %22, i32 %19, i32 0                                      \n"
                                  "  %24 = icmp eq i32 %17, 32                                                \n"
                                  "  %25 = icmp ult i32 %6, 1023                                              \n"
                                  "  %or.cond = or i1 %25, %24                                                \n"
                                  "  %maskValHigh32bit.0 = select i1 %or.cond, i32 -2147483648, i32 %21       \n"
                                  "  %maskValLow32bit.0 = select i1 %or.cond, i32 0, i32 %23                  \n"
                                  "  %26 = trunc i64 %1 to i32                                                \n"
                                  "  %27 = and i32 %maskValLow32bit.0, %26                                    \n"
                                  "  %28 = and i32 %maskValHigh32bit.0, %3                                    \n"
                                  "  %29 = zext i32 %28 to i64                                                \n"
                                  "  %30 = shl nuw i64 %29, 32                                                \n"
                                  "  %31 = zext i32 %27 to i64                                                \n"
                                  "  %32 = or i64 %30, %31                                                    \n"
                                  "  %33 = bitcast i64 %32 to double                                          \n"
                                  "  ret double %33                                                           \n"
                                  "}";

  llvm::MemoryBufferRef codeBuf(code, "<string>");
  llvm::SMDiagnostic diagnostic;
  const bool failed = llvm::parseAssemblyInto(codeBuf, mod, nullptr, diagnostic);
  (void)failed;
  IGC_ASSERT_MESSAGE(false == failed, "Error parse llvm assembly");

  func = mod->getFunction("__builtin_trunc_f64");

  return this->CreateCall(func, src);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateRoundZ(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  if (V->getType() == this->getDoubleTy()) {
    return CreateDTrunc(V);
  } else {
    llvm::Function *trunc = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::trunc, V->getType());
    return this->CreateCall(trunc, V);
  }
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDRoundNE(llvm::Value *src) {
  llvm::Module *const mod = this->GetInsertBlock()->getParent()->getParent();
  IGC_ASSERT(nullptr != mod);
  llvm::Function *func = mod->getFunction("__builtin_roundne_f64");
  if (func != nullptr) {
    return this->CreateCall(func, src);
  }

  // From OCL builtin: double @__builtin_spirv_rint_f64(double %x)
  static const char *const code =
      "define linkonce double @__builtin_roundne_f64(double %x)                            \n"
      "    alwaysinline nounwind readnone {                                       \n"
      "  %1 = bitcast double %x to i64                                            \n"
      "  %2 = and i64 %1, 9223372036854775807                                     \n"
      "  %3 = bitcast i64 %2 to double                                            \n"
      "  %4 = lshr i64 %2, 52                                                     \n"
      "  %5 = trunc i64 %4 to i32                                                 \n"
      "  %6 = icmp ult i32 %5, 1075                                               \n"
      "  %7 = zext i1 %6 to i32                                                   \n"
      "  %8 = sitofp i32 %7 to double                                             \n"
      "  %9 = fmul double %8, 5.000000e-01                                        \n"
      "  %10 = fadd double %3, %9                                                 \n"
      "  %11 = bitcast double %10 to i64                                          \n"
      "  %12 = lshr i64 %11, 32                                                   \n"
      "  %13 = trunc i64 %12 to i32                                               \n"
      "  %14 = lshr i64 %11, 52                                                   \n"
      "  %15 = trunc i64 %14 to i32                                               \n"
      "  %16 = and i32 %15, 2047                                                  \n"
      "  %17 = sub nsw i32 1023, %16                                              \n"
      "  %18 = add nsw i32 %17, 52                                                \n"
      "  %19 = add nsw i32 %17, 20                                                \n"
      "  %20 = icmp sgt i32 %18, 32                                               \n"
      "  %21 = select i1 %20, i32 32, i32 %18                                     \n"
      "  %22 = icmp sgt i32 %19, 20                                               \n"
      "  %23 = select i1 %22, i32 20, i32 %19                                     \n"
      "  %24 = icmp sgt i32 %21, 0                                                \n"
      "  %25 = select i1 %24, i32 %21, i32 0                                      \n"
      "  %26 = icmp sgt i32 %23, 0                                                \n"
      "  %27 = select i1 %26, i32 %23, i32 0                                      \n"
      "  %28 = and i32 %25, 31                                                    \n"
      "  %29 = shl i32 -1, %28                                                    \n"
      "  %30 = and i32 %27, 31                                                    \n"
      "  %31 = shl i32 -1, %30                                                    \n"
      "  %32 = icmp ne i32 %25, 32                                                \n"
      "  %33 = select i1 %32, i32 %29, i32 0                                      \n"
      "  %34 = icmp eq i32 %27, 32                                                \n"
      "  %35 = icmp ult i32 %16, 1023                                             \n"
      "  %or.cond.i = or i1 %35, %34                                              \n"
      "  %maskValHigh32bit.0.i = select i1 %or.cond.i, i32 -2147483648, i32 %31   \n"
      "  %maskValLow32bit.0.i = select i1 %or.cond.i, i32 0, i32 %33              \n"
      "  %36 = trunc i64 %11 to i32                                               \n"
      "  %37 = and i32 %maskValLow32bit.0.i, %36                                  \n"
      "  %38 = and i32 %maskValHigh32bit.0.i, %13                                 \n"
      "  %39 = zext i32 %38 to i64                                                \n"
      "  %40 = shl nuw i64 %39, 32                                                \n"
      "  %41 = zext i32 %37 to i64                                                \n"
      "  %42 = or i64 %40, %41                                                    \n"
      "  %43 = bitcast i64 %42 to double                                          \n"
      "  %44 = fptoui double %43 to i64                                           \n"
      "  %.tr = trunc i64 %44 to i32                                              \n"
      "  %45 = fsub double %43, %3                                                \n"
      "  %46 = fcmp oeq double %45, 5.000000e-01                                  \n"
      "  %47 = zext i1 %46 to i32                                                 \n"
      "  %48 = and i32 %.tr, %47                                                  \n"
      "  %49 = uitofp i32 %48 to double                                           \n"
      "  %50 = fsub double %43, %49                                               \n"
      "  %51 = and i64 %1, -9223372036854775808                                   \n"
      "  %52 = bitcast double %50 to i64                                          \n"
      "  %53 = or i64 %52, %51                                                    \n"
      "  %54 = bitcast i64 %53 to double                                          \n"
      "  ret double %54                                                           \n"
      "}";

  llvm::MemoryBufferRef codeBuf(code, "<string>");
  llvm::SMDiagnostic diagnostic;
  const bool failed = llvm::parseAssemblyInto(codeBuf, mod, nullptr, diagnostic);
  (void)failed;
  IGC_ASSERT_MESSAGE(false == failed, "Error parse llvm assembly");

  func = mod->getFunction("__builtin_roundne_f64");

  return this->CreateCall(func, src);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateRoundNE(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  if (V->getType() == this->getDoubleTy()) {
    return CreateDRoundNE(V);
  } else if (V->getType() == this->getHalfTy()) {
    V = this->CreateFPExt(V, this->getFloatTy());
    llvm::Function *roundne = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ROUNDNE);
    V = this->CreateCall(roundne, V);
    return this->CreateFPTrunc(V, this->getHalfTy());
  } else {
    llvm::Function *roundne = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_ROUNDNE);
    return this->CreateCall(roundne, V);
  }
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateIsNan(llvm::Value *V) {
  // fcmp_uno yields true if either operand is a QNAN. Since we compare the same numer with itself.
  // If V is not NAN it will return false
  return this->CreateFCmp(llvm::FCmpInst::FCMP_UNO, V, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateCtpop(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *ctpop = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::ctpop, V->getType());
  return this->CreateCall(ctpop, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::getHalf(float f) {
  return llvm::ConstantFP::get(this->getHalfTy(), f);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::getFloat(float f) {
  return llvm::ConstantFP::get(this->getFloatTy(), f);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::getDouble(double d) {
  return llvm::ConstantFP::get(this->getDoubleTy(), d);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDeriveRTX(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *floor =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_GradientX, V->getType());
  return this->CreateCall(floor, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDeriveRTX_Fine(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *floor =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_GradientXfine, V->getType());
  return this->CreateCall(floor, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDeriveRTY(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *floor =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_GradientY, V->getType());
  return this->CreateCall(floor, V);
}

template <typename T, typename Inserter> llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDeriveRTY_Fine(llvm::Value *V) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *floor =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_GradientYfine, V->getType());
  return this->CreateCall(floor, V);
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::Create_MAD_Scalar(llvm::Value *float_src0, llvm::Value *float_src1,
                                                           llvm::Value *float_src2) {
  llvm::Module *const module = this->GetInsertBlock()->getParent()->getParent();
  IGC_ASSERT(nullptr != module);
  IGC_ASSERT(nullptr != float_src0);

  // Builtin Signature: float (float, float, float)
  IGC_ASSERT_MESSAGE((float_src0->getType() == llvm::Type::getHalfTy(module->getContext()) ||
                      float_src0->getType() == this->getFloatTy() || float_src0->getType() == this->getDoubleTy()),
                     "Type check @MAD.scalar arg: 0");
  IGC_ASSERT_MESSAGE((float_src1->getType() == llvm::Type::getHalfTy(module->getContext()) ||
                      float_src1->getType() == this->getFloatTy() || float_src1->getType() == this->getDoubleTy()),
                     "Type check @MAD.scalar arg: 1");
  IGC_ASSERT_MESSAGE((float_src2->getType() == llvm::Type::getHalfTy(module->getContext()) ||
                      float_src2->getType() == this->getFloatTy() || float_src2->getType() == this->getDoubleTy()),
                     "Type check @MAD.scalar arg: 2");

  llvm::Function *madFunc = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::fma, float_src0->getType());
  llvm::Value *args[] = {float_src0, float_src1, float_src2};
  llvm::Value *float_madres_s = this->CreateCall(madFunc, args);

  return float_madres_s;
}

template <typename T, typename Inserter>
llvm::Value *LLVM3DBuilder<T, Inserter>::CreatePow(llvm::Value *src0, llvm::Value *src1) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *powFunc = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::pow, src0->getType());
  llvm::Value *args[] = {src0, src1};
  llvm::Value *powres_s = this->CreateCall(powFunc, args);

  return powres_s;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEBCMlod(
    llvm::Value *float_ref_value, llvm::Value *bias_value, llvm::Value *address_u, llvm::Value *address_v,
    llvm::Value *address_r, llvm::Value *address_ai, llvm::Value *minlod, llvm::Value *int32_pairedTextureIdx,
    llvm::Value *int32_textureIdx, llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV,
    llvm::Value *int32_offsetW, bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {
      float_ref_value,        bias_value,       address_u,     address_v,     address_r,     address_ai,   minlod,
      int32_pairedTextureIdx, int32_textureIdx, int32_sampler, int32_offsetU, int32_offsetV, int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {
      IGCLLVM::FixedVectorType::get(dstType, 4), float_ref_value->getType(),  minlod->getType(),
      int32_pairedTextureIdx->getType(),         int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleBCMlodptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleBCMlodptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleBCMlodptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEBC(
    llvm::Value *float_ref_value, llvm::Value *bias_value, llvm::Value *address_u, llvm::Value *address_v,
    llvm::Value *address_r, llvm::Value *address_ai, llvm::Value *int32_textureIdx, llvm::Value *int32_sampler,
    llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW, bool feedback_enabled,
    llvm::Type *returnType) {
  return Create_SAMPLEBC(float_ref_value, bias_value, address_u, address_v, address_r, address_ai,
                         llvm::UndefValue::get(int32_textureIdx->getType()), int32_textureIdx, int32_sampler,
                         int32_offsetU, int32_offsetV, int32_offsetW, feedback_enabled, returnType);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::Create_SAMPLEBC(
    llvm::Value *float_ref_value, llvm::Value *bias_value, llvm::Value *address_u, llvm::Value *address_v,
    llvm::Value *address_r, llvm::Value *address_ai, llvm::Value *int32_pairedTextureIdx, llvm::Value *int32_textureIdx,
    llvm::Value *int32_sampler, llvm::Value *int32_offsetU, llvm::Value *int32_offsetV, llvm::Value *int32_offsetW,
    bool feedback_enabled, llvm::Type *returnType) {
  llvm::Value *packed_tex_params[] = {
      float_ref_value,        bias_value,       address_u,     address_v,     address_r,     address_ai,
      int32_pairedTextureIdx, int32_textureIdx, int32_sampler, int32_offsetU, int32_offsetV, int32_offsetW};

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Type *dstType = (returnType != nullptr) ? returnType : this->getFloatTy();
  llvm::Type *types[] = {IGCLLVM::FixedVectorType::get(dstType, 4), float_ref_value->getType(),
                         int32_pairedTextureIdx->getType(), int32_textureIdx->getType(), int32_sampler->getType()};
  if (feedback_enabled) {
    types[0] = IGCLLVM::FixedVectorType::get(dstType, 5);
  }
  llvm::Function *func_llvm_GenISA_sampleBCptr_v4f32_f32 =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_sampleBCptr, types);

  llvm::CallInst *packed_tex_call = this->CreateCall(func_llvm_GenISA_sampleBCptr_v4f32_f32, packed_tex_params);
  return packed_tex_call;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateEvalSampleIndex(llvm::Value *inputIndex, llvm::Value *sampleIndex,
                                                                      llvm::Value *perspective) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pullBarys =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_PullSampleIndexBarys);
  llvm::Value *bary = this->CreateCall2(pullBarys, sampleIndex, perspective);
  llvm::Function *interpolate =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_Interpolate);
  return this->CreateCall2(interpolate, inputIndex, bary);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateEvalSnapped(llvm::Value *inputIndex, llvm::Value *xOffset,
                                                                  llvm::Value *yOffset, llvm::Value *perspective) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();

  llvm::Function *pullBarys =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_PullSnappedBarys);
  llvm::Value *bary = this->CreateCall3(pullBarys, xOffset, yOffset, perspective);
  llvm::Function *interpolate =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_Interpolate);
  return this->CreateCall2(interpolate, inputIndex, bary);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateSetStream(llvm::Value *StreamId, llvm::Value *emitCount) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *fn = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_SetStream);
  return this->CreateCall2(fn, StreamId, emitCount);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateEndPrimitive(llvm::Value *emitCount) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *fn = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_EndPrimitive);
  return this->CreateCall(fn, emitCount);
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateControlPointId() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *fn =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_HSControlPointID);
  return this->CreateCall(fn);
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreatePrimitiveID() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::PRIMITIVEID)), this->getInt32Ty());
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateInstanceID() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::GS_INSTANCEID)), this->getInt32Ty());
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateSampleIndex() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::SAMPLEINDEX)), this->getInt32Ty());
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateCoverage() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::INPUT_COVERAGE_MASK)), this->getInt32Ty());
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateStartVertexLocation() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::XP0)), this->getInt32Ty());
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateStartInstanceLocation() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateBitCast(this->CreateCall(pFunc, this->getInt32(IGC::XP1)), this->getInt32Ty());
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateDomainPointInput(unsigned int dim) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  return this->CreateCall(pFunc, this->getInt32(IGC::DOMAIN_POINT_ID_X + dim));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_inputVecF32(llvm::Value *inputIndex,
                                                                   llvm::Value *interpolationMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_inputVec, this->getFloatTy());
  return this->CreateCall2(pFunc, inputIndex, interpolationMode);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_discard(llvm::Value *condition) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_discard);
  return this->CreateCall(pFunc, condition);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_runtime(llvm::Value *offset) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_RuntimeValue);
  return this->CreateCall(pFunc, offset);
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_uavSerializeAll() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_uavSerializeAll);
  return this->CreateCall(pFunc);
}

template <typename T, typename Inserter>
inline llvm::CallInst *LLVM3DBuilder<T, Inserter>::create_countbits(llvm::Value *src) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::ctpop, this->getInt32Ty());
  return this->CreateCall(pFunc, src);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveInverseBallot(llvm::Value *src,
                                                                         llvm::Value *helperLaneMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_WaveInverseBallot);
  return this->CreateCall2(pFunc, src, helperLaneMode ? helperLaneMode : this->getInt32(0));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveBallot(llvm::Value *src, llvm::Value *helperLaneMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_WaveBallot);
  return this->CreateCall2(pFunc, src, helperLaneMode ? helperLaneMode : this->getInt32(0));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveshuffleIndex(llvm::Value *src, llvm::Value *index,
                                                                        llvm::Value *helperLaneMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Type *srcType = src->getType();
  if (srcType == this->getInt1Ty()) {
    src = this->CreateZExt(src, this->getInt32Ty());
  }
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_WaveShuffleIndex, src->getType());
  llvm::Value *retVal = this->CreateCall3(pFunc, src, index, (helperLaneMode ? helperLaneMode : this->getInt32(0)));
  if (srcType == this->getInt1Ty()) {
    retVal = this->CreateTrunc(retVal, srcType);
  }
  return retVal;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveAll(llvm::Value *src, llvm::Value *type,
                                                               llvm::Value *helperLaneMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_WaveAll, src->getType());
  return this->CreateCall4(pFunc, src, type, this->getInt1(true), helperLaneMode ? helperLaneMode : this->getInt32(0));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_wavePrefix(llvm::Value *src, llvm::Value *type, bool inclusive,
                                                                  llvm::Value *Mask, llvm::Value *helperLaneMode) {
  // If a nullptr is passed in for 'Mask' (as is the default), just include
  // all lanes.
  Mask = Mask ? Mask : this->getInt1(true);
  helperLaneMode = helperLaneMode ? helperLaneMode : this->getInt32(0);

  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_WavePrefix, src->getType());
  return this->CreateCall5(pFunc, src, type, this->getInt1(inclusive), Mask, helperLaneMode);
}

// We currently use the combination of 'convergent' and
// 'inaccessiblememonly' to prevent code motion of
// wave intrinsics.  Removing 'readnone' from a callsite
// is not sufficient to stop LICM from looking back up to the
// function definition for the attribute.  We can short circuit that
// by creating an operand bundle.  The name "nohoist" is not
// significant; anything will do.
inline llvm::CallInst *setUnsafeToHoistAttr(llvm::CallInst *CI) {
  CI->setConvergent();
  IGCLLVM::setOnlyAccessesInaccessibleMemory(*CI);
  llvm::OperandBundleDef OpDef("nohoist", llvm::ArrayRef<llvm::Value *>());

  // An operand bundle cannot be appended onto a call after creation.
  // clone the instruction but add our operandbundle on as well.
  llvm::SmallVector<llvm::OperandBundleDef, 1> OpBundles;
  CI->getOperandBundlesAsDefs(OpBundles);
  OpBundles.push_back(OpDef);
  llvm::CallInst *NewCall = llvm::CallInst::Create(CI, OpBundles, CI);
  CI->replaceAllUsesWith(NewCall);
  return NewCall;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_wavePrefixBitCount(llvm::Value *src, llvm::Value *Mask,
                                                                          llvm::Value *helperLaneMode) {
  // bits = ballot(bBit);
  // laneMaskLT = (1 << WaveGetLaneIndex()) - 1;
  // prefixBitCount = countbits(bits & laneMaskLT);
  llvm::Value *ballot = this->create_waveBallot(src, helperLaneMode);
  if (Mask)
    ballot = this->CreateAnd(ballot, Mask);
  llvm::Value *shlLaneId = this->CreateShl(this->getInt32(1), this->get32BitLaneID());
  llvm::Value *laneMask = this->CreateSub(shlLaneId, this->getInt32(1));
  llvm::Value *mask = this->CreateAnd(ballot, laneMask);

  // update llvm.ctpop so it won't be hoisted/sunk out of the loop.
  auto *PopCnt = this->create_countbits(mask);
  auto *NoHoistPopCnt = setUnsafeToHoistAttr(PopCnt);
  PopCnt->eraseFromParent();
  return NoHoistPopCnt;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveMatch(llvm::Instruction *inst, llvm::Value *src,
                                                                 llvm::Value *helperLaneMode) {

  // Note that we will stay in the loop above as long as there is at least
  // one active lane remaining.

  // We will split the basic blocks twice.  The first will create a
  // pre-header for the loop code.  The second will separate the WaveMatch
  // from code after it so it can be broken down into a sequence of
  // instructions and then branch to the remaining code when done.

  auto *PreHeader = inst->getParent();
  auto *BodyBlock = PreHeader->splitBasicBlock(inst, VALUE_NAME(inst->getName() + "wavematch-body"));
  auto *EndBlock = BodyBlock->splitBasicBlock(inst, VALUE_NAME(inst->getName() + "wavematch-end"));

  // Make sure that we set the insert point again as we've just invalidated
  // it with the splitBasicBlock() calls above.
  this->SetInsertPoint(BodyBlock->getTerminator());

  // Now generate the code for a single iteration of the code
  auto *FirstValue = this->readFirstLane(src);
  llvm::Value *CmpRes = nullptr;
  if (src->getType()->isFloatingPointTy())
    CmpRes = this->CreateFCmpOEQ(FirstValue, src);
  else
    CmpRes = this->CreateICmpEQ(FirstValue, src);

  auto *Mask = this->create_waveBallot(CmpRes, helperLaneMode);

  // Replace the current terminator to either exit the loop
  // or branch back for another iteration.
  auto *Br = BodyBlock->getTerminator();
  this->SetInsertPoint(Br);
  this->CreateCondBr(CmpRes, EndBlock, BodyBlock);
  Br->eraseFromParent();

  // Now, gather up the output struct outside of the loop
  this->SetInsertPoint(&*EndBlock->getFirstInsertionPt());

  return Mask;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveMultiPrefix(llvm::Instruction *I, llvm::Value *Val,
                                                                       llvm::Value *Mask, IGC::WaveOps OpKind,
                                                                       llvm::Value *helperLaneMode) {
  // This implementation is similar create_waveMatch() in that we loop
  // until all subsets of lanes are processed.
  auto *PreHeader = I->getParent();
  auto *BodyBlock = PreHeader->splitBasicBlock(I, "multiprefix-body");
  auto *EndBlock = BodyBlock->splitBasicBlock(I, "multiprefix-end");

  // Make sure that we set the insert point again as we've just invalidated
  // it with the splitBasicBlock() calls above.
  this->SetInsertPoint(BodyBlock->getTerminator());

  // Now generate the code for a single iteration of the code
  auto *FirstValue = this->readFirstLane(Mask);
  auto *ParticipatingLanes = this->create_waveInverseBallot(FirstValue, helperLaneMode);

  auto *WavePrefix =
      this->create_wavePrefix(Val, this->getInt8((uint8_t)OpKind), false, ParticipatingLanes, helperLaneMode);

  // Replace the current terminator to either exit the loop
  // or branch back for another iteration.
  auto *Br = BodyBlock->getTerminator();
  this->SetInsertPoint(Br);
  this->CreateCondBr(ParticipatingLanes, EndBlock, BodyBlock);
  Br->eraseFromParent();

  this->SetInsertPoint(&*EndBlock->getFirstInsertionPt());

  return WavePrefix;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveMultiPrefixBitCount(llvm::Instruction *I, llvm::Value *Val,
                                                                               llvm::Value *Mask,
                                                                               llvm::Value *helperLaneMode) {
  // Similar structure to waveMatch and waveMultiPrefix
  auto *PreHeader = I->getParent();
  auto *BodyBlock = PreHeader->splitBasicBlock(I, "multiprefixbitcount-body");
  auto *EndBlock = BodyBlock->splitBasicBlock(I, "multiprefixbitcount-end");

  // Make sure that we set the insert point again as we've just invalidated
  // it with the splitBasicBlock() calls above.
  this->SetInsertPoint(BodyBlock->getTerminator());

  // Now generate the code for a single iteration of the code
  auto *FirstValue = this->readFirstLane(Mask);

  auto *Count = this->create_wavePrefixBitCount(Val, FirstValue, helperLaneMode);

  // Replace the current terminator to either exit the loop
  // or branch back for another iteration.
  auto *Br = BodyBlock->getTerminator();
  this->SetInsertPoint(Br);
  auto *ParticipatingLanes = this->create_waveInverseBallot(FirstValue, helperLaneMode);
  this->CreateCondBr(ParticipatingLanes, EndBlock, BodyBlock);
  Br->eraseFromParent();

  this->SetInsertPoint(&*EndBlock->getFirstInsertionPt());

  return Count;
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_waveClusteredAll(llvm::Value *src, llvm::Value *reductionType,
                                                                        llvm::Value *clusterSize,
                                                                        llvm::Value *helperLaneMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_WaveClustered, src->getType());
  return this->CreateCall4(pFunc, src, reductionType, clusterSize, helperLaneMode ? helperLaneMode : this->getInt32(0));
}

template <typename T, typename Inserter>
inline llvm::Value *
LLVM3DBuilder<T, Inserter>::create_waveClusteredBroadcast(llvm::Value *src, llvm::Value *clusterLane,
                                                          llvm::Value *clusterSize, llvm::Value *helperLaneMode) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(
      module, llvm::GenISAIntrinsic::GenISA_WaveClusteredBroadcast, src->getType());
  // If helperLaneMode is nullptr, use 0
  llvm::Value *hlm = helperLaneMode ? helperLaneMode : this->getInt32(0);
  return this->CreateCall4(pFunc, src, clusterSize, clusterLane, hlm);
}
template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::create_quadPrefix(llvm::Value *src, llvm::Value *type, bool inclusive) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_QuadPrefix, src->getType());
  return this->CreateCall3(pFunc, src, type, this->getInt1(inclusive));
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::get16BitLaneID() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_simdLaneId);
  llvm::Value *int16LaneId = this->CreateCall(pFunc);
  return int16LaneId;
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::get32BitLaneID() {
  return this->CreateZExt(get16BitLaneID(), this->getInt32Ty());
}

template <typename T, typename Inserter> inline llvm::Value *LLVM3DBuilder<T, Inserter>::getSimdSize() {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_simdSize);
  return this->CreateCall(pFunc);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::getFirstLaneID(llvm::Value *helperLaneMode) {
  // fbl(WaveBallot(true))
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Value *ballot = this->create_waveBallot(this->getInt1(1), helperLaneMode);
  llvm::Function *pFunc = llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_firstbitLo);
  return this->CreateCall(pFunc, ballot);
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::readFirstLane(llvm::Value *src, llvm::Value *helperLaneMode) {
  llvm::Value *firstLaneID = this->getFirstLaneID(helperLaneMode);
  return this->create_waveshuffleIndex(src, firstLaneID, helperLaneMode);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Creates data conversion for typed image reads.
///     Gen HW has supports only limited number of surface formats through data
/// port data cache typed read messages. Complete lists of formats supported
/// for read is available in Programmer's Reference Manual.
/// Some of the unsupported formats are  mandatory in Vulkan and OGL.
/// In order to support these formats the driver and the compiler implement the
/// following emulation:
/// Since Gen9 HW typed read messages return raw data when reading from an
/// unsupported format. It's enough to call the conversion method
/// CreateImageDataConversion() using data returned from typed read messages.
///
/// @param format Surface format of the typed image (original i.e. from shader)
/// @param data Data returned by typed read message
/// @returns llvm::Value* Vector of data converted to the input surface format.
///
template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateImageDataConversion(IGC::SURFACE_FORMAT format,
                                                                          llvm::Value *data) {
  IGC_ASSERT(nullptr != m_Platform);

  switch (format) {
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16B16A16_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16B16A16_SNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8B8A8_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8B8A8_SNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16_SNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8_SNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16_SNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8_SNORM:
    if (m_Platform->hasHDCSupportForTypedReadsUnormSnormToFloatConversion()) {
      return data;
    }
    break;
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R10G10B10A2_UNORM:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R11G11B10_FLOAT:
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R10G10B10A2_UINT:
    if (m_Platform->hasSupportForAllOCLImageFormats()) {
      return data;
    }
    break;
  default:
    break;
  }

  llvm::Value *pFormatConvertedLLVMLdUAVTypedResult = data;
  switch (format) {
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16B16A16_UNORM: {
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));
    llvm::Value *pConstFloat =
        llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(this->getFloatTy(), (1.0f / 65535.0f)));
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempInt16 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pMaskLow = this->getInt32(0x0000FFFF);
    llvm::Value *pShift16 = this->getInt32(0x00000010);

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));

    // Retrieve unsigned short value (component 0).
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());
    pTempInt16 = this->CreateAnd(pTempInt32, pMaskLow);

    // Convert unsigned short to float (component 0).
    pTempFloat = this->CreateUIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    // Retrieve unsigned short value (component 1).
    pTempInt16 = this->CreateLShr(pTempInt32, pShift16);

    // Convert unsigned short to float (component 1).
    pTempFloat = this->CreateUIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 1 in output vector (pTempVec4[1]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // pTempFloat = pLdUAVTypedResult[1];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(1));

    // Retrieve unsigned short value (component 2).
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());
    pTempInt16 = this->CreateAnd(pTempInt32, pMaskLow);

    // Convert unsigned short to float (component 2).
    pTempFloat = this->CreateUIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 2 in output vector (pTempVec4[2]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(2));

    // Retrieve unsigned short value (component 3).
    pTempInt16 = this->CreateLShr(pTempInt32, pShift16);

    // Convert unsigned short to float (component 3).
    pTempFloat = this->CreateUIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 3 in output vector (pTempVec4[3]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(3));

    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16B16A16_SNORM: {
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));
    llvm::Value *pScalingFactor = this->getFloat(1.0f / 32767.0f);
    llvm::Value *pTempInt32;
    llvm::Value *pTempInt16;
    llvm::Value *pTempFloat;
    llvm::Value *pNegativeOne = this->getFloat(-1.0f);
    llvm::Value *pCmp_result;
    llvm::Value *fieldWidth = this->getInt32(16);

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));

    // Retrieve unsigned short value (component 0).
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());
    pTempInt16 = this->Create_IBFE(fieldWidth, this->getInt32(0), pTempInt32);

    // Convert signed short to float (component 0).
    pTempFloat = this->CreateSIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Compare with -1.0f
    pCmp_result = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, pNegativeOne);
    pTempFloat = this->CreateSelect(pCmp_result, pTempFloat, pNegativeOne);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    // Retrieve unsigned short value (component 1).
    pTempInt16 = this->CreateAShr(pTempInt32, 16);

    // Convert signed short to float (component 1).
    pTempFloat = this->CreateSIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Compare with -1.0f
    pCmp_result = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, pNegativeOne);
    pTempFloat = this->CreateSelect(pCmp_result, pTempFloat, pNegativeOne);

    // Store component 1 in output vector (pTempVec4[1]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // pTempFloat = pLdUAVTypedResult[1];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(1));

    // Retrieve unsigned short value (component 2).
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());
    pTempInt16 = this->Create_IBFE(fieldWidth, this->getInt32(0), pTempInt32);

    // Convert unsigned short to float (component 2).
    pTempFloat = this->CreateSIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Compare with -1.0f
    pCmp_result = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, pNegativeOne);
    pTempFloat = this->CreateSelect(pCmp_result, pTempFloat, pNegativeOne);

    // Store component 2 in output vector (pTempVec4[2]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(2));

    // Retrieve unsigned short value (component 3).
    pTempInt16 = this->CreateAShr(pTempInt32, 16);

    // Convert unsigned short to float (component 3).
    pTempFloat = this->CreateSIToFP(pTempInt16, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Compare with -1.0f
    pCmp_result = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, pNegativeOne);
    pTempFloat = this->CreateSelect(pCmp_result, pTempFloat, pNegativeOne);

    // Store component 3 in output vector (pTempVec4[3]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(3));

    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R10G10B10A2_UNORM: {
    llvm::Value *pImmediateXYZ =
        llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(this->getFloatTy(), (1.0f / 1023.0f)));
    llvm::Value *pImmediateW = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(this->getFloatTy(), (1.0f / 3.0f)));
    llvm::Value *pMaskXYZ = this->getInt32(0x000003ff);
    llvm::Value *pMaskW = this->getInt32(0x00000003);
    llvm::Value *pShiftData = this->getInt32(10);

    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempIntWithMask = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pTempShiftRightData = llvm::UndefValue::get(this->getInt32Ty());

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));

    // Retrieve unsigned short value (component 0).
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());
    pTempIntWithMask = this->CreateAnd(pTempInt32, pMaskXYZ);

    // Convert unsigned short to float (component 0).
    pTempFloat = this->CreateUIToFP(pTempIntWithMask, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pImmediateXYZ);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    // Retrieve unsigned short value (component 0).
    pTempShiftRightData = this->CreateLShr(pTempInt32, pShiftData);

    pTempIntWithMask = this->CreateAnd(pTempShiftRightData, pMaskXYZ);

    // Convert unsigned short to float.
    pTempFloat = this->CreateUIToFP(pTempIntWithMask, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pImmediateXYZ);

    // Store component 1 in output vector (pTempVec4[1]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // Retrieve unsigned short value.
    pTempShiftRightData = this->CreateLShr(pTempShiftRightData, pShiftData);

    pTempIntWithMask = this->CreateAnd(pTempShiftRightData, pMaskXYZ);

    // Convert unsigned short to float.
    pTempFloat = this->CreateUIToFP(pTempIntWithMask, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pImmediateXYZ);

    // Store component 2 in output vector (pTempVec4[1]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(2));

    // Retrieve unsigned short value.
    pTempShiftRightData = this->CreateLShr(pTempShiftRightData, pShiftData);

    pTempIntWithMask = this->CreateAnd(pTempShiftRightData, pMaskW);

    // Convert unsigned short to float.
    pTempFloat = this->CreateUIToFP(pTempIntWithMask, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pImmediateW);

    // Store component 3 in output vector (pTempVec4[1]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R11G11B10_FLOAT: {
    // This surface format packs 3 half-float values into 32-bit string.
    // Half-floats are always non-negative, so to save space sign bit
    // is not stored and assumed to be zero.
    // Only 11 or 10 most significant bits (not counting sign bit)
    // of the 16 bits of IEEE 754 float16 are stored.
    // The least significant bits of the mantissa are assumed to be zero.
    // First value is stored in bits 0--10.     (r)
    // Second value is stored in bits 11 - 22   (g)
    // Third value is stored in bits 22 - 31    (b)
    // Fourth value is set to 1.0f.

    llvm::Value *pMaskX = this->getInt32(0x000007ff);
    llvm::Value *pMaskY = this->getInt32(0x00007ff0);
    llvm::Value *pMaskZ = this->getInt32(0x00007fe0);
    llvm::Value *pShiftDataX = this->getInt32(4);
    llvm::Value *pShiftDataY = this->getInt32(7);
    llvm::Value *pShiftDataZ = this->getInt32(10);
    llvm::Value *pTempFloat;
    llvm::Value *pTempFloat0;
    llvm::Value *pTempInt;
    llvm::Value *pTempInt0;

    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat0 = pLdUAVTypedResult[0];
    pTempFloat0 = this->CreateExtractElement(data, this->getInt32(0));
    pTempInt0 = this->CreateBitCast(pTempFloat0, this->getInt32Ty());

    pTempInt = this->CreateAnd(pTempInt0, pMaskX);
    pTempInt = this->CreateShl(pTempInt, pShiftDataX);
    pTempInt = this->CreateTrunc(pTempInt, this->getInt16Ty());
    pTempFloat = this->CreateBitCast(pTempInt, llvm::Type::getHalfTy(this->getContext()));
    pTempFloat = this->CreateF16TOF32(pTempFloat);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    pTempInt0 = this->CreateLShr(pTempInt0, pShiftDataY);
    pTempInt = this->CreateAnd(pTempInt0, pMaskY);
    pTempInt = this->CreateTrunc(pTempInt, this->getInt16Ty());
    pTempFloat = this->CreateBitCast(pTempInt, llvm::Type::getHalfTy(this->getContext()));
    pTempFloat = this->CreateF16TOF32(pTempFloat);

    // Store component 1 in output vector (pTempVec4[1]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    pTempInt0 = this->CreateLShr(pTempInt0, pShiftDataZ);
    pTempInt = this->CreateAnd(pTempInt0, pMaskZ);
    pTempInt = this->CreateTrunc(pTempInt, this->getInt16Ty());
    pTempFloat = this->CreateBitCast(pTempInt, llvm::Type::getHalfTy(this->getContext()));
    pTempFloat = this->CreateF16TOF32(pTempFloat);

    // Store component 2 in output vector (pTempVec4[2]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(2));

    // store 1.0 into component 3
    pTempVec4 = this->CreateInsertElement(pTempVec4, getFloat(1.0f), this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R10G10B10A2_UINT: {
    // AND          ro.x, ri.x, { 0x000003ff };
    // SHR          ri.x, ri.x, { 10 };
    // AND          ro.y, ri.x, { 0x000003ff };
    // SHR          ri.x, ri.x, { 10 };
    // AND          ro.z, ri.x, { 0x000003ff };
    // SHR          ri.x, ri.x, { 10 };
    // AND          ro.w, ri.x, { 0x00000003 };
    // copy results
    llvm::Value *pMaskXYZ = this->getInt32(0x000003ff);
    llvm::Value *pMaskW = this->getInt32(0x00000003);
    llvm::Value *pShiftDataXYZ = this->getInt32(10);

    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempIntRes = llvm::UndefValue::get(this->getInt32Ty());

    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // AND          ro.x, ri.x, { 0x000003ff };
    pTempIntRes = this->CreateAnd(pTempInt32, pMaskXYZ);
    pTempFloat = this->CreateBitCast(pTempIntRes, this->getFloatTy());

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    // SHR          ri.x, ri.x, { 10 };
    // AND          ro.y, ri.x, { 0x000003ff };
    pTempInt32 = this->CreateLShr(pTempInt32, pShiftDataXYZ);
    pTempIntRes = this->CreateAnd(pTempInt32, pMaskXYZ);
    pTempFloat = this->CreateBitCast(pTempIntRes, this->getFloatTy());

    // Store component 1 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // SHR          ri.x, ri.x, { 10 };
    // AND          ro.z, ri.x, { 0x000003ff };
    pTempInt32 = this->CreateLShr(pTempInt32, pShiftDataXYZ);
    pTempIntRes = this->CreateAnd(pTempInt32, pMaskXYZ);
    pTempFloat = this->CreateBitCast(pTempIntRes, this->getFloatTy());

    // Store component 2 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(2));

    // SHR          ri.x, ri.x, { 10 };
    // AND          ro.w, ri.x, { 3 };
    pTempInt32 = this->CreateLShr(pTempInt32, pShiftDataXYZ);
    pTempIntRes = this->CreateAnd(pTempInt32, pMaskW);
    pTempFloat = this->CreateBitCast(pTempIntRes, this->getFloatTy());

    // Store component 3 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8B8A8_UNORM: {
    // immX = 0x8, immY = 0x10, immZ = 0x18
    // immMaskLow = 0x000000FF
    // AND rTemp.x, ri.x, immMaskLow
    // ubfe rTemp.y, immX, immX, ri.x
    // ubfe rTemp.z, immX, immY, ri.x
    // ubfe rTemp.w, immX, immZ, ri.x
    // ubtof rTemp, rTemp
    // Fmul  rOutput, rTemp, 1.0f/255.0f
    llvm::Value *pMaskLow8 = this->getInt32(0x000000FF);
    llvm::Value *pImmX = this->getInt32(0x8);
    llvm::Value *pImmY = this->getInt32(0x10);
    llvm::Value *pImmZ = this->getInt32(0x18);
    llvm::Value *pConstFloat = this->getFloat(1.0f / 255.0f);
    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempInt32Res = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // AND rTemp.x, ri.x, immMaskLow
    pTempInt32Res = this->CreateAnd(pTempInt32, pMaskLow8);

    // ubtof rTemp.x, rTemp.x
    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());

    // Fmul  rOutput.x, rTemp.x, 1.0f/255.0f
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    // ubfe rTemp.y, immX, immX,  ri.x
    pTempInt32Res = this->Create_UBFE(pImmX, pImmX, pTempInt32);

    // ubtof rTemp.y, rTemp.y
    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());

    // Fmul  rOutput.y, rTemp.y, 1.0f/255.0f
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 1 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // ubfe rTemp.z, immX, immY,  ri.x
    pTempInt32Res = this->Create_UBFE(pImmX, pImmY, pTempInt32);

    // ubtof rTemp.z, rTemp.z
    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());

    // Fmul  rOutput.z, rTemp.z, 1.0f/255.0f
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 2 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(2));

    // ubfe rTemp.w, immX, immZ,  ri.x
    pTempInt32Res = this->Create_UBFE(pImmX, pImmZ, pTempInt32);

    // ubtof rTemp.w, rTemp.w
    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());

    // Fmul  rOutput.w, rTemp.w, 1.0f/255.0f
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 3 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(3));

    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8B8A8_SNORM: {
    llvm::Value *pScalingFactor = this->getFloat(1.0f / 127.0f);
    llvm::Value *fieldWidth = this->getInt32(8);
    llvm::Value *fpNegOne = this->getFloat(-1.0f);

    // pTempFloat = pLdUAVTypedResult[0];
    llvm::Value *pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    // cast to int32 since result is seen as float
    llvm::Value *pInputAsInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // create 4-component output vector
    llvm::Value *pOutputVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // for each of the four channels
    for (unsigned int ch = 0; ch < 4; ++ch) {
      // extract 8 bits with sign extend from position 8*ch..8*ch+7
      // for bits 24..31 we can use arithmetic shift right instead of bit extract
      llvm::Value *pTempInt32Res = (ch < 3) ? this->Create_IBFE(fieldWidth, this->getInt32(8 * ch), pInputAsInt32)
                                            : this->CreateAShr(pInputAsInt32, 8 * ch);

      // convert to float
      pTempFloat = this->CreateSIToFP(pTempInt32Res, this->getFloatTy());

      // multiply bthis->y the scaling factor 1.0f/127.0f
      pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

      // Fcmp_ge rFlag, rTemp.x, -1.0f
      // Sel.rFlag rOutput.x, rTemp.x, -1.0f
      llvm::Value *pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, fpNegOne);
      pTempFloat = this->CreateSelect(pFlag, pTempFloat, fpNegOne);

      // Store component ch in output vector (pTempVec4[0]).
      pOutputVec4 = this->CreateInsertElement(pOutputVec4, pTempFloat, this->getInt32(ch));
    }

    pFormatConvertedLLVMLdUAVTypedResult = pOutputVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16_UNORM: {
    // immMaskHigh = 0x0000FFFF
    // rImm.zw = {0.0f, 1.0f}
    // AND rTemp.x, ri.x, immMaskHigh
    // SHR rTemp.y, ri.x, 0x10,
    // USTOF rTemp.xy, rTemp.xy
    // FMUL rOutput.xy, rTemp.xy, 1.0f/65535.0f
    // MOV rOutput.zw, rImm.zw
    llvm::Value *pMaskHigh = this->getInt32(0x0000FFFF);
    llvm::Value *pShiftVal = this->getInt32(0x10);
    llvm::Value *pImmZ = this->getFloat(0.0f);
    llvm::Value *pImmW = this->getFloat(1.0f);
    llvm::Value *pConstFloat = this->getFloat(1.0f / 65535.0f);
    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempInt32Res = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // AND rTemp.x, ri.x, immMaskHigh
    pTempInt32Res = this->CreateAnd(pTempInt32, pMaskHigh);

    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    pTempInt32Res = this->CreateLShr(pTempInt32, pShiftVal);
    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 1 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // Store component 2 to value 0.0f in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pImmZ, this->getInt32(2));

    // Store component 3 to Value 1.0f in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pImmW, this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16G16_SNORM: {
    // immMaskLow16 = 0x0000FFFF
    // rImm.zw = {0.0f, 1.0f}
    // AND rTemp.x, ri.x, immMaskLow16
    // SHR rTemp.y, ri.x, 0x10,
    // STOF rTemp.xy, rTemp.xy
    // FMUL rTemp.xy, rTemp.xy, 1.0f / 32767.0f
    // FCMP_GE rFlag.xy, rTemp.xy, -1.0f
    // SEL_rFlag.xy rOutput.xy, rTemp.xy, -1.0f
    // MOV rOutput.zw, rImm.zw
    llvm::Value *pScalingFactor = getFloat(1.0f / 32767.0f);
    llvm::Value *pOutVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    llvm::Value *pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    llvm::Value *pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // extract bits 0..15 and sign extend the result
    llvm::Value *pTempInt32Res = Create_IBFE(this->getInt32(16), this->getInt32(0), pTempInt32);

    // convert to float and apply scaling factor
    pTempFloat = this->CreateSIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // clamp to range [-1.0f, 1.0f] since the value can be little less than -1.0f
    // Fcmp_ge rFlag, rTemp.x, -1.0f
    // Sel.rFlag rOutput.x, rTemp.x, -1.0f
    llvm::Value *pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, this->getFloat(-1.0f));
    pTempFloat = this->CreateSelect(pFlag, pTempFloat, this->getFloat(-1.0f));

    // Store component 0 in output vector (pTempVec4[0]).
    pOutVec4 = this->CreateInsertElement(pOutVec4, pTempFloat, this->getInt32(0));

    // extract bits 16..31 with sign extension
    pTempInt32Res = this->CreateAShr(pTempInt32, 16);
    pTempFloat = this->CreateSIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Fcmp_ge rFlag, rTemp.y, -1.0f
    // Sel.rFlag rOutput.y, rTemp.y, -1.0f
    pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, this->getFloat(-1.0f));

    pTempFloat = this->CreateSelect(pFlag, pTempFloat, this->getFloat(-1.0f));

    // Store component 1 in output vector (pTempVec4[0]).
    pOutVec4 = this->CreateInsertElement(pOutVec4, pTempFloat, this->getInt32(1));

    // Store 0.0f, 1.0f in the remaining components of the output vector
    pOutVec4 = this->CreateInsertElement(pOutVec4, getFloat(0.0f), this->getInt32(2));
    pOutVec4 = this->CreateInsertElement(pOutVec4, getFloat(1.0f), this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pOutVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8_UNORM: {
    // immMaskLow8 = 0x000000FF
    // rImm.zw = {0.0f, 1.0f}
    // AND rTemp.x, ri.x, immMaskLow8
    // SHR rTemp.y, ri.x, 0x8,
    // USTOF rTemp.xy, rTemp.xy
    // FMUL rOutput.xy, rTemp.xy, 1.0f / 255.0f
    // MOV rOutput.zw, rImm.zw
    llvm::Value *pMaskLow8 = this->getInt32(0x000000FF);
    llvm::Value *pShiftVal = this->getInt32(0x8);
    llvm::Value *pImmZ = this->getFloat(0.0f);
    llvm::Value *pImmW = this->getFloat(1.0f);
    llvm::Value *pConstFloat = this->getFloat(1.0f / 255.0f);
    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempInt32Res = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // AND rTemp.x, ri.x, immMaskHigh
    pTempInt32Res = this->CreateAnd(pTempInt32, pMaskLow8);

    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    pTempInt32Res = this->CreateLShr(pTempInt32, pShiftVal);
    pTempFloat = this->CreateUIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pConstFloat);

    // Store component 1 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(1));

    // Store component 2 to value 0.0f in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pImmZ, this->getInt32(2));

    // Store component 3 to Value 1.0f in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pImmW, this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8G8_SNORM: {
    // immMaskLow8 = 0x000000FF
    // rImm.zw = {0.0f, 1.0f}
    // AND rTemp.x, ri.x, immMaskLow8
    // SHR rTemp.y, ri.x, 0x8,
    // STOF rTemp.xy, rTemp.xy
    // FMUL rTemp.xy, rTemp.xy, 1.0f / 127.0f
    // FCMP_GE rFlag.xy, rTemp.xy, -1.0f
    // SEL_rFlag.xy rOutput.xy, rTemp.xy, -1.0f
    // MOV rOutput.zw, rImm.zw
    llvm::Value *pScalingFactor = getFloat(1.0f / 127.0f);

    llvm::Value *pOutVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    llvm::Value *fieldWidth = this->getInt32(8);

    // pTempFloat = pLdUAVTypedResult[0];
    llvm::Value *pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    llvm::Value *pInputInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    llvm::Value *pTempInt32Res = Create_IBFE(fieldWidth, this->getInt32(0), pInputInt32);
    pTempFloat = this->CreateSIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Fcmp_ge rFlag, rTemp.x, -1.0f
    // Sel.rFlag rOutput.x, rTemp.x, -1.0f
    llvm::Value *pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, getFloat(-1.0f));

    pTempFloat = this->CreateSelect(pFlag, pTempFloat, getFloat(-1.0f));
    // Store component 0 in output vector (pTempVec4[0]).
    pOutVec4 = this->CreateInsertElement(pOutVec4, pTempFloat, this->getInt32(0));

    // extract bits 8..15 and sign extend the result
    pTempInt32Res = this->Create_IBFE(fieldWidth, this->getInt32(8), pInputInt32);

    pTempFloat = this->CreateSIToFP(pTempInt32Res, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Fcmp_ge rFlag, rTemp.y, -1.0f
    // Sel.rFlag rOutput.y, rTemp.y, -1.0f
    pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, getFloat(-1.0f));
    pTempFloat = this->CreateSelect(pFlag, pTempFloat, getFloat(-1.0f));

    // store the value in component 1 of the output vector
    pOutVec4 = this->CreateInsertElement(pOutVec4, pTempFloat, this->getInt32(1));

    // store 0.0f, 1.0f in the remaining components of the output vector
    pOutVec4 = this->CreateInsertElement(pOutVec4, getFloat(0.0f), this->getInt32(2));
    pOutVec4 = this->CreateInsertElement(pOutVec4, getFloat(1.0f), this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pOutVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16_UNORM: {
    // rImm.yzw = {0.0f, 0.0f, 1.0f}
    // USTOF rTemp.x, ri.x
    // FMUL rOutput.x, rTemp.x, 1.0f / 65535.0f
    // MOV rOutput.yzw, rImm.yzw
    llvm::Value *pScalingFactor = getFloat(1.0f / 65535.0f);
    llvm::Value *pTempFloat = llvm::UndefValue::get(this->getFloatTy());
    llvm::Value *pTempInt32 = llvm::UndefValue::get(this->getInt32Ty());
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    pTempFloat = this->CreateUIToFP(pTempInt32, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));

    // Store 0.0f, 0.0f, 1.0f, in remaining components of the output
    llvm::Value *pFPZero = getFloat(0.0f);
    pTempVec4 = this->CreateInsertElement(pTempVec4, pFPZero, this->getInt32(1));
    pTempVec4 = this->CreateInsertElement(pTempVec4, pFPZero, this->getInt32(2));
    pTempVec4 = this->CreateInsertElement(pTempVec4, getFloat(1.0f), this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R16_SNORM: {
    // rImm.yzw = {0.0f, 0.0f, 1.0f}
    // STOF rTemp.x, ri.x
    // FMUL rTemp.x, rTemp.x, 1.0f / 32767.0f
    // FCMP_GE rFlag.x, rTemp.x, -1.0f
    // SEL_rFlag.x rOutput.x, rTemp.x, -1.0f
    // MOV rOutput.yzw, rImm.yzw
    llvm::Value *pFPZero = getFloat(0.0f);
    llvm::Value *pScalingFactor = getFloat(1.0f / 32767.0f);
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    llvm::Value *pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    llvm::Value *pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    pTempInt32 = this->Create_IBFE(this->getInt32(16), this->getInt32(0), pTempInt32);

    pTempFloat = this->CreateSIToFP(pTempInt32, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // compare with -1.0f and clamp to -1.0 if less than -1.0
    // Fcmp_ge rFlag, rTemp.x, -1.0f
    // Sel.rFlag rOutput.x, rTemp.x, -1.0f
    llvm::Value *pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, getFloat(-1.0f));
    pTempFloat = this->CreateSelect(pFlag, pTempFloat, getFloat(-1.0f));

    // Store the result in component 0 of the output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));
    // Store 0.0f, 0.0f, 1.0f in remaining components
    pTempVec4 = this->CreateInsertElement(pTempVec4, pFPZero, this->getInt32(1));
    pTempVec4 = this->CreateInsertElement(pTempVec4, pFPZero, this->getInt32(2));
    pTempVec4 = this->CreateInsertElement(pTempVec4, getFloat(1.0f), this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8_UNORM: {
    // rImm.yzw = {0.0f, 0.0f, 1.0f}
    // USTOF rTemp.x, ri.x
    // FMUL rOutput.x, rTemp.x, 1.0f / 255.0f
    // MOV rOutput.yzw, rImm.yzw
    // UBTOF        ro.x, ri.x;
    llvm::Value *fpZero = this->getFloat(0.0f);
    llvm::Value *pScalingFactor = getFloat(1.0f / 255.0f);
    llvm::Value *pTempVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    llvm::Value *pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    llvm::Value *pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    pTempFloat = this->CreateUIToFP(pTempInt32, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Store component 0 in output vector (pTempVec4[0]).
    pTempVec4 = this->CreateInsertElement(pTempVec4, pTempFloat, this->getInt32(0));
    // fill the rest with 0.0f, 0.0f, 1.0f
    pTempVec4 = this->CreateInsertElement(pTempVec4, fpZero, this->getInt32(1));
    pTempVec4 = this->CreateInsertElement(pTempVec4, fpZero, this->getInt32(2));
    pTempVec4 = this->CreateInsertElement(pTempVec4, getFloat(1.0f), this->getInt32(3));
    pFormatConvertedLLVMLdUAVTypedResult = pTempVec4;
    break;
  }
  case IGC::SURFACE_FORMAT::SURFACE_FORMAT_R8_SNORM: {
    // rImm.yzw = {0.0f, 0.0f, 1.0f}
    // STOF rTemp.x, ri.x
    // FMUL rTemp.x, rTemp.x, 1.0f / 127.0f
    // FCMP_GE rFlag.x, rTemp.x, -1.0f
    // SEL_rFlag.x rOutput.x, rTemp.x, -1.0f
    // MOV rOutput.yzw, rImm.yzw
    llvm::Value *pFpZero = getFloat(0.0f);
    llvm::Value *pFpNegOne = getFloat(-1.0f);
    llvm::Value *pScalingFactor = getFloat(1.0f / 127.0f);
    llvm::Value *pOutVec4 = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 4));

    // pTempFloat = pLdUAVTypedResult[0];
    llvm::Value *pTempFloat = this->CreateExtractElement(data, this->getInt32(0));
    llvm::Value *pTempInt32 = this->CreateBitCast(pTempFloat, this->getInt32Ty());

    // extract bits 0..7 and sign extend the result
    pTempInt32 = this->Create_IBFE(this->getInt32(8), this->getInt32(0), pTempInt32);

    // convert to float and apply scaling factor
    pTempFloat = this->CreateSIToFP(pTempInt32, this->getFloatTy());
    pTempFloat = this->CreateFMul(pTempFloat, pScalingFactor);

    // Fcmp_ge rFlag, rTemp.x, -1.0f
    // Sel.rFlag rOutput.x, rTemp.x, -1.0f
    llvm::Value *pFlag = this->CreateFCmp(llvm::FCmpInst::FCMP_OGE, pTempFloat, pFpNegOne);
    pTempFloat = this->CreateSelect(pFlag, pTempFloat, pFpNegOne);

    // Store component 0 in output vector (pTempVec4[0]).
    pOutVec4 = this->CreateInsertElement(pOutVec4, pTempFloat, this->getInt32(0));

    // Store 0.0f, 0.0f, 1.0f in the remaining components of the output vector
    pOutVec4 = this->CreateInsertElement(pOutVec4, pFpZero, this->getInt32(1));
    pOutVec4 = this->CreateInsertElement(pOutVec4, pFpZero, this->getInt32(2));
    pOutVec4 = this->CreateInsertElement(pOutVec4, getFloat(1.0f), this->getInt32(3));

    pFormatConvertedLLVMLdUAVTypedResult = pOutVec4;
    break;
  }
  default:
    break;
  }

  return pFormatConvertedLLVMLdUAVTypedResult;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Extract all scalars from a vector
/// @param  vector Llvm value of a vector
/// @param  outScalars pointer to the output array of scalars
/// @param  maxSize Size of the output array
/// @param  initializer optional parameter to set to unused elements
///
template <typename T, typename Inserter>
inline void LLVM3DBuilder<T, Inserter>::VectorToScalars(llvm::Value *vector, llvm::Value **outScalars, unsigned maxSize,
                                                        llvm::Value *initializer) {
  IGC_ASSERT(nullptr != vector);
  IGC_ASSERT(nullptr != vector->getType());
  IGC_ASSERT(vector->getType()->isVectorTy());

  const unsigned count = (unsigned)llvm::cast<IGCLLVM::FixedVectorType>(vector->getType())->getNumElements();
  IGC_ASSERT(1 < count);
  IGC_ASSERT(count <= 4);
  IGC_ASSERT(count <= maxSize);

  for (unsigned vecElem = 0; vecElem < maxSize; vecElem++) {
    if (vecElem >= count) {
      outScalars[vecElem] = initializer;
      continue;
    }
    outScalars[vecElem] = this->CreateExtractElement(vector, this->getInt32(vecElem));
  }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Aggregates scalar values to a vector
/// @param  scalars Array of scalars
/// @param  vectorElementCnt The number of elements in the vector to create.
/// @return Vector of type resultType
///
template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::ScalarsToVector(llvm::Value *(&scalars)[4], unsigned vectorElementCnt) {
  llvm::Type *const resultType = IGCLLVM::FixedVectorType::get(scalars[0]->getType(), vectorElementCnt);
  IGC_ASSERT(nullptr != resultType);
  llvm::Value *result = llvm::UndefValue::get(resultType);

  for (unsigned i = 0; i < llvm::cast<IGCLLVM::FixedVectorType>(resultType)->getNumElements(); i++) {
    IGC_ASSERT(nullptr != scalars[i]);
    IGC_ASSERT(llvm::cast<llvm::VectorType>(resultType)->getElementType() == scalars[i]->getType());

    result = this->CreateInsertElement(result, scalars[i], this->getInt32(i));
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Returns the normalization factor for UNORM formats
/// @param  bits Number of bits in the UNORM value
/// @return llvm::Constant* unorm factor
template <typename T, typename Inserter>
inline llvm::Constant *LLVM3DBuilder<T, Inserter>::GetUnormFactor(unsigned bits) {
  float maxUint = (float)((1 << bits) - 1);
  return llvm::ConstantFP::get(this->getFloatTy(), (1.0f / maxUint));
};

///////////////////////////////////////////////////////////////////////////////
/// @brief Returns the normalization factor for SNORM formats
/// @param  bits Number of bits in the SNORM value
/// @return llvm::Constant* snorm factor
template <typename T, typename Inserter>
inline llvm::Constant *LLVM3DBuilder<T, Inserter>::GetSnormFactor(unsigned bits) {
  float maxSint = (float)(((1 << bits) - 1) / 2);
  return llvm::ConstantFP::get(this->getFloatTy(), (1.0f / maxSint));
};

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateCPSRqstCoarseSize(llvm::Value *pSrcVal) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());

  llvm::Value *sizeX = this->CreateCall(pFunc, this->getInt32(IGC::REQUESTED_COARSE_SIZE_X));
  llvm::Value *sizeY = this->CreateCall(pFunc, this->getInt32(IGC::REQUESTED_COARSE_SIZE_Y));
  llvm::Value *vec = this->CreateInsertElement(llvm::UndefValue::get(pSrcVal->getType()), sizeX, this->getInt32(0));
  return this->CreateInsertElement(vec, sizeY, this->getInt32(1));
}

template <typename T, typename Inserter>
inline llvm::Value *LLVM3DBuilder<T, Inserter>::CreateCPSActualCoarseSize(llvm::Value *pSrcVal) {
  llvm::Module *module = this->GetInsertBlock()->getParent()->getParent();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_DCL_SystemValue, this->getFloatTy());
  llvm::Value *sizeX = this->CreateCall(pFunc, this->getInt32(IGC::ACTUAL_COARSE_SIZE_X));
  llvm::Value *sizeY = this->CreateCall(pFunc, this->getInt32(IGC::ACTUAL_COARSE_SIZE_Y));
  llvm::Value *vec = this->CreateInsertElement(llvm::UndefValue::get(pSrcVal->getType()), sizeX, this->getInt32(0));
  return this->CreateInsertElement(vec, sizeY, this->getInt32(1));
}


template <typename T, typename Inserter>
inline llvm::GenIntrinsicInst *LLVM3DBuilder<T, Inserter>::CreateLaunder(llvm::Value *pSrcVal) {
  llvm::Module *module = this->GetInsertBlock()->getModule();
  llvm::Function *pFunc =
      llvm::GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_launder, pSrcVal->getType());
  auto *CI = this->CreateCall(pFunc, pSrcVal, VALUE_NAME(pSrcVal->getName() + llvm::Twine(".launder")));
  return llvm::cast<llvm::GenIntrinsicInst>(CI);
}

#endif // BUILTINS_FRONTEND_DEFINITIONS_HPP

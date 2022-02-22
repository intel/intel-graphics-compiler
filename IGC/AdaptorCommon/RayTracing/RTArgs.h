/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "RTStackFormat.h"
#include "common/MDFrameWork.h"
#include "Compiler/CodeGenPublic.h"
#include "RTBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/ADT/Optional.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

// Simple class to query the position of each arg in a raytracing shader.
class ArgQuery
{
public:
    ArgQuery(const llvm::Function& F, const CodeGenContext& Ctx);
    ArgQuery(const FunctionMetaData& FMD);
    ArgQuery(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD);

    llvm::Argument* getPayloadArg(const llvm::Function *F) const;
    llvm::Argument* getHitAttribArg(const llvm::Function *F) const;
private:
    // Unlike DXR, Vulkan raytracing allows the optional specification of these
    // values.
    llvm::Optional<uint32_t> TraceRayPayloadIdx;
    llvm::Optional<uint32_t> HitAttributeIdx;
    llvm::Optional<uint32_t> CallableShaderPayloadIdx;

    CallableShaderTypeMD ShaderTy = NumberOfCallableShaderTypes;
private:
    llvm::Optional<uint32_t> getPayloadArgNo() const;
    llvm::Optional<uint32_t> getHitAttribArgNo() const;
    const llvm::Argument* getArg(
        const llvm::Function *F,
        llvm::Optional<uint32_t> ArgNo) const;

    void init(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD);
};

// When invoking a TraceRay() or CallShader(), these are the arguments that you
// must supply to the callee
class TraceRayRTArgs
{
public:
    using ReturnIP    = uint64_t;
    using PointerSize = uint64_t;

    llvm::Value* getReturnIPPtr(
        llvm::IRBuilder<>& IRB,
        llvm::Type* PayloadTy,
        llvm::RTBuilder::SWStackPtrVal* FrameAddr,
        const llvm::Twine &FrameName = "");

    llvm::Value* getPayloadPtr(
        llvm::IRBuilder<>& IRB,
        llvm::Type* PayloadTy,
        llvm::RTBuilder::SWStackPtrVal* FrameAddr,
        const llvm::Twine &FrameName = "");

    llvm::Value* getPayloadPaddingPtr(
        llvm::IRBuilder<>& IRB,
        llvm::Type* PayloadTy,
        llvm::RTBuilder::SWStackPtrVal* FrameAddr,
        const llvm::Twine &FrameName = "");

    bool needPayloadPadding() const;

    RayDispatchShaderContext &Ctx;
public:
    static constexpr uint32_t ReturnIPSlot        = 0;
    static constexpr uint32_t PayloadSlot         = 1;
    static constexpr uint32_t PayloadPaddingSlot  = 2;
protected:
    TraceRayRTArgs(
        RayDispatchShaderContext &Ctx,
        RayTracingSWTypes& RTSWTypes,
        const llvm::DataLayout &DL);

    static uint32_t getReturnIPOffset();
    static uint32_t getPayloadOffset();
    RayTracingSWTypes& RTSWTypes;
    uint32_t SWStackAddrSpace = 0;

    const llvm::DataLayout& DL;
private:
    using TypeCacheTy = llvm::DenseMap<llvm::PointerType*, llvm::StructType*>;
    llvm::PointerType* getType(llvm::PointerType* PayloadTy);
    TypeCacheTy ExistingStructs;
    TypeCacheTy& getCache();
};

// Handle arguments in a raytracing shader.
class RTArgs : public TraceRayRTArgs
{
public:
    RTArgs(
        const llvm::Function *RootFunc,
        CallableShaderTypeMD FuncType,
        llvm::Optional<RTStackFormat::HIT_GROUP_TYPE> HitGroupTy,
        RayDispatchShaderContext *Ctx,
        const FunctionMetaData& FMD,
        RayTracingSWTypes &RTSWTypes,
        bool LogStackFrameEntries = false);
public:
    llvm::Argument* getPayloadArg(const llvm::Function *F) const;
    llvm::Argument* getHitAttribArg(const llvm::Function *F) const;
    llvm::Value* getCustomHitAttribPtr(
        llvm::IRBuilder<> &IRB,
        llvm::RTBuilder::SWStackPtrVal* FrameAddr,
        llvm::Type* CustomHitAttrTy);
    llvm::Value* getHitKindPtr(
        llvm::IRBuilder<> &IRB,
        llvm::RTBuilder::SWStackPtrVal* FrameAddr);
    bool isProcedural() const;
public:
    CallableShaderTypeMD FuncType;
    const llvm::Function *RootFunction = nullptr;
protected:
    bool LogStackFrameEntries = false;
    const FunctionMetaData& FMD;
protected:
    uint32_t getCustomHitAttrOffset() const;
    uint32_t getHitKindOffset() const;
    uint32_t ArgumentSize = 0;
    void addArguments();
    llvm::SmallVector<llvm::Type*, 4> ArgTys;
    std::vector<StackFrameEntry> ArgumentEntries;
    void recordArgEntry(
        const std::string &Name,
        const std::string &TypeRepr,
        uint32_t Size,
        uint32_t Offset);
    llvm::StructType* getArgumentType(llvm::Type* CustomHitAttrTy = nullptr);
private:
    using TypeCacheTy =
        llvm::DenseMap<
            std::pair<llvm::PointerType*, llvm::Type*>,
            llvm::StructType*>;
    TypeCacheTy ExistingStructs;
    TypeCacheTy& getCache();
    llvm::Optional<RTStackFormat::HIT_GROUP_TYPE> HitGroupTy;
    llvm::Optional<uint32_t> HitKindSlot;
    llvm::Optional<uint32_t> CustomHitAttrSlot;

    ArgQuery Args;
};

} // namespace IGC


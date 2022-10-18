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

namespace IGC
{
    // Simple class to query the position of each arg in a raytracing shader.
    class ArgQuery
    {
    public:
        ArgQuery(const llvm::Function& F, const CodeGenContext& Ctx);
        ArgQuery(const FunctionMetaData& FMD);
        ArgQuery(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD);

        llvm::Argument* getPayloadArg(const llvm::Function* F) const;
        llvm::Argument* getHitAttribArg(const llvm::Function* F) const;
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
            const llvm::Function* F,
            llvm::Optional<uint32_t> ArgNo) const;

        void init(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD);
    };
} // namespace IGC

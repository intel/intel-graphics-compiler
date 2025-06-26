/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This contains a collection of methods to inspect the positions/existence
/// of arguments in a shader as well as methods to compute the types of the
/// argument portion of a stack frame.
///
//===----------------------------------------------------------------------===//

#include "RTStackFormat.h"
#include "RTArgs.h"
#include "MDFrameWork.h"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include <optional>
#include "Compiler/CISACodeGen/getCacheOpts.h"

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

void ArgQuery::init(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD)
{
    ShaderTy = FuncType;
    uint32_t Idx = 0;
    switch (FuncType)
    {
    case RayGen:
    case CallStackHandler:
        break;
    case Miss:
        if (FMD.rtInfo.hasTraceRayPayload)
            TraceRayPayloadIdx = Idx++;
        break;
    case Callable:
        if (FMD.rtInfo.hasCallableData)
            CallableShaderPayloadIdx = Idx++;
        break;
    case ClosestHit:
    // For any-hit shaders, there are two cases:
    // 1. procedural hit-group: call will be inlined and will use no
    // stack (args just passed as normal function args).
    // 2. triangle hit-group: ray payload passed, hit attributes will
    // come from potentialHit portion of stack.
    // TODO: in the procedural case we should have already deleted
    // the shader by this point.
    case AnyHit:
        if (FMD.rtInfo.hasTraceRayPayload)
            TraceRayPayloadIdx = Idx++;
        if (FMD.rtInfo.hasHitAttributes)
            HitAttributeIdx    = Idx++;
        break;
    case Intersection:
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "unknown func type!");
        break;
    }
}

ArgQuery::ArgQuery(const Function& F, const CodeGenContext& Ctx)
{
    auto* MMD = Ctx.getModuleMetaData();
    auto& FuncMD = MMD->FuncMD;
    auto I = FuncMD.find(const_cast<Function*>(&F));
    IGC_ASSERT_MESSAGE(I != FuncMD.end(), "Missing metadata?");
    auto& FMD = I->second;

    init(FMD.rtInfo.callableShaderType, FMD);
}

ArgQuery::ArgQuery(const FunctionMetaData& FMD)
{
    init(FMD.rtInfo.callableShaderType, FMD);
}

ArgQuery::ArgQuery(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD)
{
    init(FuncType, FMD);
}

Argument* ArgQuery::getPayloadArg(const Function* F) const
{
    return const_cast<Argument*>(getArg(F, getPayloadArgNo()));
}

Argument* ArgQuery::getHitAttribArg(const Function* F) const
{
    return const_cast<Argument*>(getArg(F, getHitAttribArgNo()));
}

std::optional<uint32_t> ArgQuery::getPayloadArgNo() const
{
    if (ShaderTy == Callable)
        return CallableShaderPayloadIdx;
    else
        return TraceRayPayloadIdx;

    return std::nullopt;
}

const Argument* ArgQuery::getArg(
    const Function* F,
    std::optional<uint32_t> ArgNo) const
{
    // Not specified
    if (!ArgNo)
        return nullptr;

    auto* Arg = F->arg_begin();
    if (F->arg_size() <= *ArgNo)
        return nullptr;

    std::advance(Arg, *ArgNo);

    return Arg;
}

std::optional<uint32_t> ArgQuery::getHitAttribArgNo() const
{
    return HitAttributeIdx;
}


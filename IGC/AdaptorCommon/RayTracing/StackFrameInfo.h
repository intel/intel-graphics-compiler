/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/MDFrameWork.h"
#include "Compiler/CodeGenPublic.h"
#include "RTStackFormat.h"
#include "RTArgs.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/Optional.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class StackFrameInfo : public RTArgs
{
public:
    StackFrameInfo(
        const llvm::Function* RootFunc,
        CallableShaderTypeMD ShaderType,
        llvm::Optional<RTStackFormat::HIT_GROUP_TYPE> HitGroupTy,
        RayDispatchShaderContext* RayCtx,
        const FunctionMetaData& FMD,
        RayTracingSWTypes &RTSWTypes,
        bool LogStackEntries = false);
public:
    void addFunction(const llvm::Function *F);
    llvm::Value* getSpillPtr(
        llvm::IRBuilder<> &IRB,
        llvm::RTBuilder::SWStackPtrVal *FrameAddr,
        const llvm::SpillValueIntrinsic* SI,
        const llvm::Twine& Name = "") const;
    llvm::Value* getFillPtr(
        llvm::IRBuilder<> &IRB,
        llvm::RTBuilder::SWStackPtrVal *FrameAddr,
        const llvm::FillValueIntrinsic* FI,
        const llvm::Twine& Name = "") const;
    llvm::Value* getAllocaPtr(
        llvm::IRBuilder<> &IRB,
        llvm::RTBuilder::SWStackPtrVal *FrameAddr,
        const llvm::AllocaInst* AI,
        const llvm::Twine& Name = "") const;
    uint32_t getFrameSize() const;
    void finalize();
    bool isContinuation(const llvm::Function* F) const;
    bool isRoot(const llvm::Function* F) const;
    bool isRayGen() const;
    bool isRayGenRoot(const llvm::Function* F) const;
    uint32_t getSpillOffset() const;
private:
    uint32_t getAllocasOffset() const;
    uint32_t TotalAllocaSize = 0;
    uint32_t TotalSpillSize  = 0;
private:
    void addAllocas(const llvm::Function *F);
    void addFills(const llvm::Function *F);
    void addSpills(const llvm::Function *F);
    void addSpillsCompacted(const llvm::Function *F);
    void addSpillsUncompacted(const llvm::Function *F);
    // Just used for shader dumping
    void recordAllocaEntry(const llvm::AllocaInst* AI, uint32_t Size);
    void recordSpillUnionEntry(
        const llvm::SpillValueIntrinsic* SV,
        std::vector<StackFrameEntry> &Entries,
        uint32_t Size);
    std::vector<StackFrameEntry> AllocaEntries;
    std::vector<StackFrameSpillUnion> SpillEntries;
private:
    llvm::Optional<uint32_t> ArgumentSlot;
    llvm::Optional<uint32_t> AllocaSlot;
    llvm::Optional<uint32_t> SpillSlot;

    llvm::StructType* AllocaStructTy = nullptr;
    llvm::DenseMap<const llvm::AllocaInst*, uint32_t> AllocaIdxMap;
    // maps a basic block with spills to the associated struct type representing
    // all those spills.
    llvm::MapVector<const llvm::BasicBlock*, llvm::StructType*> SpillTyMap;
    // maps a spill instruction to its position in a basic block (e.g., the
    // 5th spill from top to bottom in some block would have index = 4 while
    // the 5th spill in another block would also have index = 4).
    llvm::DenseMap<const llvm::SpillValueIntrinsic*, uint32_t> SpillIdxMap;
    // Maps fills in a function to the corresponding type.
    llvm::MapVector<const llvm::Function*, llvm::StructType*> FillTyMap;
    // Maps fills to their corresponding index in the type.
    llvm::DenseMap<const llvm::FillValueIntrinsic*, uint32_t> FillIdxMap;

    bool skipRecording() const;
};

} // namespace IGC

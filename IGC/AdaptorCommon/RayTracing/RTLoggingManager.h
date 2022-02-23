/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <array>

#include "Compiler/CISACodeGen/DriverInfo.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Optional.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/IR/Constant.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class RTLoggingManager
{
public:
    enum FormatIndex : uint32_t
    {
        DISPATCH_RAY_INDEX,
        TRACE_RAY,
        FORWARD_RAY,
        NumIndices
    };
public:
    RTLoggingManager(const CDriverInfo& DriverInfo);
    bool isEnabled() const;

    llvm::Optional<uint32_t> getIndex(llvm::StringRef FormatString) const;

    template <typename Fn>
    llvm::Constant* getFormatString(FormatIndex Index, Fn F) {
        using namespace llvm;
        if (Value* V = FormatCache[Index])
            return cast<Constant>(V);

        StringRef FormatString = IndexToFormat[Index];
        Constant* Val = F(FormatString);
        FormatCache[Index] = Val;

        return Val;
    }
private:
    bool Enabled = false;
    void addToTable(llvm::StringRef FormatString, FormatIndex Index);
private:
    llvm::DenseMap<llvm::StringRef, uint32_t> FormatToIndex;
    std::array<llvm::StringRef, NumIndices> IndexToFormat{};
    std::array<llvm::WeakVH, NumIndices> FormatCache{};
};

} // namespace IGC

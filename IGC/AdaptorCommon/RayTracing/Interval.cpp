/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// A simple set of utilities for working with intervals.
///
//===----------------------------------------------------------------------===//

#include "Interval.h"
#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/MathExtras.h>
#include "common/LLVMWarningsPop.hpp"

static uint64_t offsetToAlign(uint64_t Value, uint64_t Align)
{
    return llvm::alignTo(Value, Align) - Value;
}

namespace Intervals {

// Returns true iff two well-formed intervals overlap by any amount.
bool overlap(const Interval& A, const Interval& B)
{
    return (A.first <= B.second && A.second >= B.first);
}

// Returns true iff within two well-formed intervals, A fully overlaps B.
bool fully_overlap(const Interval& A, const Interval& B)
{
    return (A.first <= B.first && A.second >= B.second);
}

// Returns true iff A and B touch at their endpoints.
bool contiguous(const Interval& A, const Interval& B)
{
    return (A.second + 1 == B.first) || (B.second + 1 == A.first);
}

Interval expandRange(
    uint32_t BlockSize,
    uint32_t BlockAlign,
    uint64_t StartOffset,
    uint64_t EndOffset)
{
    // Find the first aligned address that we should start reading from.
    StartOffset = llvm::alignDown(StartOffset, BlockAlign);

    // Get last address rounded up such that `BlockSize` will evenly divide
    // the range.
    uint64_t NumBytes = EndOffset - StartOffset + 1;
    uint64_t Diff = offsetToAlign(NumBytes, BlockSize);
    EndOffset += Diff;

    IGC_ASSERT((EndOffset - StartOffset + 1) % BlockSize == 0);

    return std::make_pair(StartOffset, EndOffset);
}

void mergeIntervals(
    SortedIntervals& Intervals, std::function<bool(uint64_t)> canMerge)
{
    auto merge = [&](SortedIntervals& I)
    {
        const uint64_t Sentinel = std::numeric_limits<uint64_t>::max();
        bool Changed = false;
        for (int32_t i = 0; i < (int32_t)(I.size()) - 1; /* empty */)
        {
            auto& First  = I[i];
            auto& Second = I[i + 1];

            if (contiguous(First, Second))
            {
                uint64_t NewSize = Second.second - First.first + 1;
                if (canMerge(NewSize))
                {
                    // extend the first interval to include the second.
                    First.second = Second.second;
                    // Mark the second interval for deletion
                    Second.first = Sentinel;
                    i += 2;
                    Changed = true;
                    continue;
                }
            }

            i++;
        }

        I.erase(std::remove_if(
            I.begin(), I.end(),
            [&](auto &A) { return A.first == Sentinel; }), I.end());

        return Changed;
    };

    while (merge(Intervals));
}

} // namespace Intervals


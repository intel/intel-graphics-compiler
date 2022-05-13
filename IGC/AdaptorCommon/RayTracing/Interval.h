/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <utility>
#include <functional>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace Intervals {

    // Represents a well-formed interval (i.e., an interval [A, B] s.t. A <= B).
    using Interval        = std::pair<uint64_t, uint64_t>;

    // Sorted and disjoint intervals that need to be loaded.
    using SortedIntervals = llvm::SmallVector<Interval, 4>;

    // Returns true iff two well-formed intervals overlap by any amount.
    bool overlap(const Interval& A, const Interval& B);

    // Returns true iff within two well-formed intervals, A fully overlaps B.
    bool fully_overlap(const Interval& A, const Interval& B);

    // Returns true iff A and B touch at their endpoints.
    bool contiguous(const Interval& A, const Interval& B);

    // Returns a new interval that expands the left and right of
    // [StartOffset, EndOffset] so it is aligned according to BlockAlign and
    // BlockSize.
    Interval expandRange(
        uint32_t BlockSize,
        uint32_t BlockAlign,
        uint64_t StartOffset,
        uint64_t EndOffset);

    // Given a collection of sorted and disjoint intervals, merge the contiguous
    // ones that satisfy the `canMerge(NewSize)` predicate.
    void mergeIntervals(
        SortedIntervals& Intervals, std::function<bool(uint64_t)> canMerge);

} // namespace Intervals


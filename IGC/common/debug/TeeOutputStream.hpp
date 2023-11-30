/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
namespace Debug
{
    class TeeOutputStream
        : public llvm::raw_ostream
    {
    public:
        TeeOutputStream(
            llvm::raw_ostream& s0,
            llvm::raw_ostream& s1);

        TeeOutputStream(
            llvm::raw_ostream* pLHS,
            bool shouldDeleteLHS,
            llvm::raw_ostream* pRHS,
            bool shouldDeleteRHS);

        ~TeeOutputStream();
        TeeOutputStream(const TeeOutputStream&) = delete;
        TeeOutputStream& operator=(const TeeOutputStream&) = delete;

        size_t preferred_buffer_size() const override;

        llvm::raw_ostream &changeColor(enum llvm::raw_ostream::Colors colors, bool bold, bool bg) override;

        llvm::raw_ostream &resetColor() override;

        llvm::raw_ostream &reverseColor() override;

    private:
        void write_impl(const char *Ptr, size_t Size) override;

        uint64_t current_pos() const override;

    private:
        llvm::raw_ostream*     const m_LHS;
        llvm::raw_ostream*     const m_RHS;
        bool                   const m_deleteLHS;
        bool                   const m_deleteRHS;
    };
} // namespace Debug
} // namespace IGC

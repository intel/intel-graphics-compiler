/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/debug/TeeOutputStream.hpp"
#include "common/debug/Debug.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
namespace Debug
{

TeeOutputStream::TeeOutputStream(llvm::raw_ostream& lhs, llvm::raw_ostream& rhs)
    : raw_ostream( true /* unbuffered */ )
    , m_LHS(&lhs)
    , m_RHS(&rhs)
    , m_deleteLHS(false)
    , m_deleteRHS(false)
{
}

TeeOutputStream::TeeOutputStream(
        llvm::raw_ostream* pLHS,
        bool shouldDeleteLHS,
        llvm::raw_ostream* pRHS,
        bool shouldDeleteRHS)
    : raw_ostream( true /* unbuffered */ )
    , m_LHS(pLHS)
    , m_RHS(pRHS)
    , m_deleteLHS(shouldDeleteLHS)
    , m_deleteRHS(shouldDeleteRHS)
{
    IGC_ASSERT_MESSAGE((nullptr != pLHS), "LHS must not be null");
    IGC_ASSERT_MESSAGE((nullptr != pRHS), "RHS must not be null");
}

TeeOutputStream::~TeeOutputStream()
{
    if (m_deleteLHS)
    {
        delete m_LHS;
    }
    if (m_deleteRHS)
    {
        delete m_RHS;
    }
}

size_t TeeOutputStream::preferred_buffer_size() const
{
    return m_LHS->GetBufferSize();
}

llvm::raw_ostream& TeeOutputStream::changeColor(
    enum llvm::raw_ostream::Colors colors,
    bool bold,
    bool bg)
{
    if (m_LHS->has_colors()) m_LHS->changeColor(colors, bold, bg);
    if (m_RHS->has_colors()) m_RHS->changeColor(colors, bold, bg);
    return *this;
}

llvm::raw_ostream& TeeOutputStream::resetColor()
{
    if (m_LHS->has_colors()) m_LHS->resetColor();
    if (m_RHS->has_colors()) m_RHS->resetColor();
    return *this;
}

llvm::raw_ostream& TeeOutputStream::reverseColor()
{
    if (m_LHS->has_colors()) m_LHS->reverseColor();
    if (m_RHS->has_colors()) m_RHS->reverseColor();
    return *this;
}

void TeeOutputStream::write_impl(const char *Ptr, size_t Size)
{
    m_LHS->write(Ptr, Size);
    m_RHS->write(Ptr, Size);
}

uint64_t TeeOutputStream::current_pos() const
{
    return m_LHS->tell() - m_LHS->GetNumBytesInBuffer();
}

} // namespace Debug
} // namespace IGC

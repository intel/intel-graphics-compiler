/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Atomic.h>
#include "common/LLVMWarningsPop.hpp"
#include "MetaDataTraits.h"
#include "Probe/Assertion.h"

namespace IGC
{

    template<class MDorNamedMD>
    inline llvm::Metadata* getMDOpnd(MDorNamedMD* MD, unsigned idx)
    {
        IGC_ASSERT(0);
        return nullptr;
    }

    template<>
    inline llvm::Metadata* getMDOpnd<llvm::MDNode>(
        llvm::MDNode* MD, unsigned idx)
    {
        return MD->getOperand(idx).get();
    }

    template<>
    inline llvm::Metadata* getMDOpnd<llvm::NamedMDNode>(
        llvm::NamedMDNode* MD, unsigned idx)
    {
        return MD->getOperand(idx);
    }

    template<>
    inline llvm::Metadata* getMDOpnd<const llvm::MDNode>(
        const llvm::MDNode* MD, unsigned idx)
    {
        return MD->getOperand(idx).get();
    }

    template<>
    inline llvm::Metadata* getMDOpnd<const llvm::NamedMDNode>(
        const llvm::NamedMDNode* MD, unsigned idx)
    {
        return MD->getOperand(idx);
    }


    ///
    // Iterator over the meta data nodes list. It is assumed that
    // all the nodes are of the same type ( as specified by the T template parameter)
    // Template parameters:
    // T - type of the entry node
    // N - type of the root(parent) node (supported types are MDNode and NamedMDNode )
    // C - traits type (see the MDValueTraits )
    //
    template<class T, class N = llvm::MDNode, class C = MDValueTraits<T> >
    class MetaDataIterator
    {
    public:
        typedef typename C::value_type value_type;
        typedef MetaDataIterator<T, N, C> _Myt;

        ///
        // Ctor. Creates the sentinel iterator. Usually used as an end iterator
        //
        explicit MetaDataIterator(const N* pNode) :
            m_pNode(pNode),
            m_index(pNode->getNumOperands())
        {
        }

        ///
        // Ctor. Create the iterator on given index
        explicit MetaDataIterator(const N* pNode, unsigned int index) :
            m_pNode(pNode),
            m_index(index)
        {
            IGC_ASSERT(index <= pNode->getNumOperands());
        }


        llvm::Metadata* operator *()
        {
            IGC_ASSERT_MESSAGE(false == isNil(), "m_Index has to be 0");

            return getMDOpnd(m_pNode, m_index);
        }
        ///
        // returns the current item in the list
        value_type get()
        {
            IGC_ASSERT_MESSAGE(false == isNil(), "m_Index has to be 0");

            return C::load(getMDOpnd(m_pNode, m_index));
        }

        _Myt& operator++()
        {
            if (!isNil())
                ++m_index;
            return (*this);
        }

        _Myt operator++(int)
        {
            _Myt tmp = *this;
            ++* this;
            return tmp;
        }

        bool operator == (const _Myt& rhs) const
        {
            return m_pNode == rhs.m_pNode &&
                m_index == rhs.m_index;
        }

        bool operator != (const _Myt& rhs) const
        {
            return !this->operator==(rhs);
        }

    private:
        bool isNil()
        {
            return m_index == m_pNode->getNumOperands();
        }

    private:
        const N* m_pNode; // pointer to the parent node
        unsigned int m_index;
    };

} //namespace

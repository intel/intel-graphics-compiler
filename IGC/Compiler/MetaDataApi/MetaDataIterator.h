/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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

namespace IGC
{

    template<class MDorNamedMD>
    inline llvm::Metadata* getMDOpnd(MDorNamedMD* MD, unsigned idx)
    {
        assert(false);
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
            assert(index <= pNode->getNumOperands());
        }


        llvm::Metadata* operator *()
        {
            if (isNil())
            {
                assert(0 && "m_Index has to be 0");
            }
            return getMDOpnd(m_pNode, m_index);
        }
        ///
        // returns the current item in the list
        value_type get()
        {
            if (isNil())
            {
                assert(0 && "m_Index has to be 0");
            }
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

        bool operator == (const _Myt& rhs)
        {
            return m_pNode == rhs.m_pNode &&
                m_index == rhs.m_index;
        }

        bool operator != (const _Myt& rhs)
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

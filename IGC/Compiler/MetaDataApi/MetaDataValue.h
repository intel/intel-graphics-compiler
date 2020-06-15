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

#include "MetaDataTraits.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Value.h>
#include <llvm/IR/Metadata.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    // Represents the meta data value stored using the positional schema
    // The root node is actuall storing the value
    template<class T, class Traits = MDValueTraits<T> >
    class MetaDataValue
    {
    public:
        typedef typename Traits::value_type value_type;

        MetaDataValue(llvm::Metadata* pNode) :
            m_pNode(pNode),
            m_value(Traits::load(pNode)),
            m_isDirty(false),
            m_hasGeneratedValued(false)
        {
        }

        MetaDataValue() :
            m_pNode(NULL),
            m_value(),
            m_isDirty(false),
            m_hasGeneratedValued(false)
        {
        }

        MetaDataValue(const value_type& val) :
            m_pNode(NULL),
            m_value(val),
            m_isDirty(true),
            m_hasGeneratedValued(false)
        {
        }

        operator value_type()
        {
            return m_value;
        }

        value_type get() const
        {
            return m_value;
        }

        llvm::MDNode* getMDNode()
        {
            return m_pNode;
        }

        void set(const value_type& val)
        {
            m_value = val;
            m_isDirty = true;
        }

        bool dirty() const
        {
            return m_isDirty;
        }

        bool hasValue() const
        {
            return m_pNode != NULL || dirty() || m_hasGeneratedValued;
        }

        void discardChanges()
        {
            m_isDirty = false;
        }

        void save(llvm::LLVMContext& context, llvm::Metadata* pNode) const
        {
            if (m_pNode == pNode && !dirty())
            {
                // we are saving to our own node but nothing was changed
                return;
            }

            if (!hasValue())
            {
                return;
            }

#if 0
            pNode->replaceAllUsesWith(generateNode(context));
#else
            IGC_ASSERT(0);
#endif
        }

        llvm::Metadata* generateNode(llvm::LLVMContext& context) const
        {
            if (!hasValue())
            {
                return NULL;
            }

            m_hasGeneratedValued = true;

            return Traits::generateValue(context, m_value);
        }

    private:
        llvm::Metadata* m_pNode;
        value_type m_value;
        bool m_isDirty;
        mutable bool m_hasGeneratedValued;
    };


    ///
    // Represents the meta data value stored using the 'named' schema
    // The root node should have two operands nodes:
    //  - the first one is MDString storing the name
    //  - the second one is actual value
    template<class T, class Traits = MDValueTraits<T> >
    class NamedMetaDataValue
    {
    public:
        typedef typename Traits::value_type value_type;

        NamedMetaDataValue() :
            m_pNode(NULL),
            m_id("")
        {
        }

        NamedMetaDataValue(llvm::Metadata* pNode) :
            m_pNode(pNode),
            m_id(getIdNode(pNode)),
            m_value(getValueNode(pNode))
        {
        }

        NamedMetaDataValue(const char* name) :
            m_pNode(NULL),
            m_id(name)
        {
        }

        NamedMetaDataValue(std::string name) :
            m_pNode(NULL),
            m_id(name.c_str())
        {
        }

        NamedMetaDataValue(const char* name, const value_type& val) :
            m_pNode(NULL),
            m_id(name),
            m_value(val)
        {
        }

        operator value_type()
        {
            return (value_type)m_value;
        }

        value_type get() const
        {
            return m_value.get();
        }

        void set(const value_type& val)
        {
            m_value.set(val);
        }

        bool dirty() const
        {
            return m_value.dirty();
        }

        bool hasValue() const
        {
            return m_value.hasValue();
        }

        void discardChanges()
        {
            m_value.discardChanges();
        }

        void save(llvm::LLVMContext& context, llvm::Metadata* pNode) const
        {
            if (m_pNode == pNode && !dirty())
            {
                return;
            }

            if (!hasValue())
            {
                return;
            }

            llvm::MDNode* pMDNode = llvm::dyn_cast<llvm::MDNode>(pNode);
            IGC_ASSERT_MESSAGE(pMDNode, "Named value parent node is not of MDNode type");

            if (pMDNode->getNumOperands() != 2)
            {
#if 0
                pMDNode->replaceAllUsesWith(generateNode(context));
#else
                IGC_ASSERT(0);
#endif
                return;
            }

            m_id.save(context, pMDNode->getOperand(0));
            m_value.save(context, pMDNode->getOperand(1));
        }

        llvm::Metadata* generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector< llvm::Metadata*, 2> args;

            args.push_back(m_id.generateNode(context));
            args.push_back(m_value.generateNode(context));

            return llvm::MDNode::get(context, args);
        }

        NamedMetaDataValue& operator=(llvm::Value* pNode)
        {
            m_pNode = pNode;
            m_id = getIdNode(pNode);
            m_value = getValueNode(pNode);

            return *this;
        }

        NamedMetaDataValue& operator=(const char* name)
        {
            m_pNode = NULL;
            m_id = name;

            return *this;
        }

        NamedMetaDataValue& operator=(std::string name)
        {
            m_pNode = NULL;
            m_id = name.c_str();

            return *this;
        }

    private:
        llvm::Metadata* getIdNode(const llvm::Metadata* pNode)
        {
            if (NULL == pNode)
            {
                return NULL; //this is allowed for optional nodes
            }

            const llvm::MDNode* const pMDNode = llvm::dyn_cast<const llvm::MDNode>(pNode);

            IGC_ASSERT_MESSAGE(nullptr != pMDNode, "Named value parent node is not of MDNode type");

            IGC_ASSERT_MESSAGE(1 <= pMDNode->getNumOperands(), "Named value doesn't have a name node");

            llvm::MDString* const pIdNode = llvm::dyn_cast<llvm::MDString>(pMDNode->getOperand(0));

            IGC_ASSERT_MESSAGE(nullptr != pIdNode, "Named list id node is not a string");

            return pIdNode;
        }

        llvm::Metadata* getValueNode(const llvm::Metadata* pNode)
        {
            if (NULL == pNode)
            {
                return NULL; //this is allowed for optional nodes
            }

            const llvm::MDNode* const pMDNode = llvm::dyn_cast<const llvm::MDNode>(pNode);

            IGC_ASSERT_MESSAGE(nullptr != pMDNode, "Named value parent node is not of MDNode type");

            IGC_ASSERT_MESSAGE(2 <= pMDNode->getNumOperands(), "Named value doesn't have a value node");

            return pMDNode->getOperand(1).get();
        }

    private:
        llvm::Metadata* m_pNode; // root node initialized during the load
        MetaDataValue<std::string>  m_id;
        MetaDataValue<T, Traits> m_value;
    };


} //namespace

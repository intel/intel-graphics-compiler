/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "MetaDataTraits.h"
#include "MetaDataValue.h"
#include "MetaDataIterator.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Atomic.h>
#include "common/LLVMWarningsPop.hpp"
#include <atomic>
#include "Probe/Assertion.h"

namespace IGC
{
    // Base interface for the metadata struct object
    // Meta data object support the following behaviors:
    //  * Ref counting
    //  * Dirty bit
    //  * Optional support for 'named object'
    // 'named object' is a metadata object with the first operand containing the 'id' string
    struct IMetaDataObject
    {
        IMetaDataObject(const llvm::MDNode* pNode, bool hasId) :
            m_refCount(0),
            m_id(hasId ? getIdNode(pNode) : NULL)
        {
        }

        IMetaDataObject(const char* name) :
            m_refCount(0),
            m_id(name)
        {
        }

        IMetaDataObject() :
            m_refCount(0),
            m_id(NULL)
        {}

        virtual ~IMetaDataObject()
        {}

        void addRef()
        {
            m_refCount++;
        }

        void releaseRef()
        {
            if (--m_refCount == 0)
                delete this;
        }

        llvm::StringRef getId()
        {
            return m_id.get();
        }

        void setId(const std::string& id)
        {
            m_id.set(id);
        }

        virtual bool dirty() const = 0;

        virtual void discardChanges() = 0;

    protected:
        // Returns the Id node given the parent MDNode
        // Id node is always a first operand of the parent node and
        // should be stored as MDString
        llvm::Metadata* getIdNode(const llvm::MDNode* pNode) const
        {
            if (NULL == pNode)
            {
                // optional node
                return NULL;
            }

            IGC_ASSERT_MESSAGE(0 < pNode->getNumOperands(), "Named list doesn't have a name node");

            llvm::MDString* pIdNode = llvm::dyn_cast<llvm::MDString>(pNode->getOperand(0));

            // This could be a nullptr too. Intentional.
            return pIdNode;
        }

        // Returns the start index
        unsigned int getStartIndex() const
        {
            return m_id.hasValue() ? 1 : 0;
        }

        // Returns a node given the parent MDNode and the node index
        llvm::Metadata* getNumberedNode(const llvm::MDNode* pParentNode, unsigned int Index) const
        {
            if (!pParentNode)
            {
                return nullptr;
            }
            return pParentNode->getOperand(getStartIndex() + Index).get();
        }

        // Returns a node given the parent MDNode and the node name
        llvm::MDNode* getNamedNode(const llvm::MDNode* pParentNode, const char* pName) const
        {
            auto isNamedNode = [](const llvm::Metadata* pNode, const char* pName)
            {
                const llvm::MDNode* pMDNode = llvm::dyn_cast<llvm::MDNode>(pNode);
                if (!pMDNode || pMDNode->getNumOperands() == 0)
                {
                    return false;
                }

                const llvm::MDString* pIdNode = llvm::dyn_cast<const llvm::MDString>(pMDNode->getOperand(0));
                if (!pIdNode)
                {
                    return false;
                }

                return pIdNode->getString().compare(pName) == 0;
            };

            if (!pParentNode)
            {
                return nullptr;
            }

            using NodeIterator = MetaDataIterator<llvm::Metadata>;
            for (NodeIterator i = NodeIterator(pParentNode, getStartIndex()), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (i.get() && isNamedNode(i.get(), pName))
                {
                    return llvm::cast<llvm::MDNode>(i.get());
                }
            }
            return nullptr;
        }

        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            if (m_id.hasValue())
                m_id.save(context, getIdNode(pNode));
        }

        llvm::Metadata* generateNode(llvm::LLVMContext& context) const
        {
            return m_id.generateNode(context);
        }

    private:
        mutable std::atomic<int> m_refCount;
        MetaDataValue<std::string>  m_id;
    };

    // Smart pointer for handling the IMetaDataObject interfaces
    template<class T>
    class MetaObjectHandle
    {
    public:
        typedef T ObjectType;

        explicit MetaObjectHandle(T* rhs = 0)
            : m_ptr(rhs)
        {
            addRef();
        }

        MetaObjectHandle(const MetaObjectHandle<T>& rhs)
            : m_ptr(rhs.get())
        {
            addRef();
        }

        template<class _Other>
        MetaObjectHandle(const MetaObjectHandle<_Other>& rhs)
            : m_ptr(rhs.get())
        {
            addRef();
        }

        template<class _Other>
        operator MetaObjectHandle<_Other>() const
        {
            return (MetaObjectHandle<_Other>(*this));
        }

        template<class _Other>
        MetaObjectHandle<T>& operator=(const MetaObjectHandle<_Other>& rhs)
        {
            reset(rhs.get());
            return (*this);
        }

        MetaObjectHandle<T>& operator=(const MetaObjectHandle<T>& rhs)
        {
            reset(rhs.get());
            return (*this);
        }

        MetaObjectHandle<T>& operator=(const MetaObjectHandle<T>* rhs)
        {
            reset(rhs->get());
            return (*this);
        }

        MetaObjectHandle<T>& operator=(T* rhs)
        {
            reset(rhs);
            return (*this);
        }

        ~MetaObjectHandle()
        {
            releaseRef();
        }

        T& operator*() const
        {
            IGC_ASSERT(m_ptr != 0);
            return (*get());
        }

        T* operator->() const
        {
            IGC_ASSERT(m_ptr != 0);
            return (get());
        }

        T* get() const
        {
            return (m_ptr);
        }

        T** getOutPtr()
        {
            return (&m_ptr);
        }

        void addRef()
        {
            if (m_ptr)
                m_ptr->addRef();
        }

        void releaseRef()
        {
            if (m_ptr)
                m_ptr->releaseRef();
            m_ptr = 0;
        }

        T* release()
        {   // !!! This method do not decrement the reference count and thus should be used with care !!!
            T* _Tmp = m_ptr;
            m_ptr = nullptr;
            return (_Tmp);
        }

        void reset(T* rhs = nullptr)
        {
            releaseRef();
            m_ptr = rhs;
            addRef();
        }

        bool dirty() const
        {
            if (m_ptr)
                return m_ptr->dirty();
            return false;
        }

        void discardChanges()
        {
            if (m_ptr)
                m_ptr->discardChanges();
        }

        llvm::Metadata* generateNode(llvm::LLVMContext& context) const
        {
            if (m_ptr)
                return m_ptr->generateNode(context);
            return NULL;
        }

        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            if (m_ptr)
                m_ptr->save(context, pNode);
        }

    private:
        T* m_ptr;    // the wrapped object pointer
    };

    template< class T>
    struct MDValueTraits<MetaObjectHandle<T>>
    {
        typedef MetaObjectHandle<T> value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            if (NULL == pNode)
            {
                return MetaObjectHandle<T>(new T());
            }
            else
            {
                llvm::MDNode* const pMDNode = llvm::dyn_cast<llvm::MDNode>(pNode);
                IGC_ASSERT_MESSAGE(nullptr != pMDNode, "pNode is not an MDNode value");
                return MetaObjectHandle<T>(new T(pMDNode, false));
            }
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            return val->generateNode(context);
        }

        static bool dirty(const value_type& val)
        {
            return val->dirty();
        }

        static void discardChanges(value_type& val)
        {
            val->discardChanges();
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* target, const value_type& val)
        {
            val->save(context, llvm::cast<llvm::MDNode>(target));
        }
    };
}

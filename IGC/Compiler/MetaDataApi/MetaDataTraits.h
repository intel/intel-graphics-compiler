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
#include <llvm/IR/Type.h>
#include <llvm/Support/Atomic.h>
#include <llvm/Support/DataTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "MetaDataApiUtils.h"
#include "Probe/Assertion.h"
#include "CommonMacros.h"

namespace IGC
{
    // Generic template for the traits types.
    // Assumes the the T type is inherited from the llvm::Value
    template< class T, typename C = void >
    struct MDValueTraits
    {
        // Value type that will be used in the metadata containers
        typedef T* value_type;

        ///
        // Loads the given value_type from the given node
        static value_type load(llvm::Metadata* pMD)
        {
            if (NULL == pMD) // || !pNode->hasValueHandle())
            {
                // it is ok to pass NULL nodes - part of support for optional values
                return NULL;
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pMD);
            value_type pT = llvm::dyn_cast<T>(val->getValue());
            IGC_ASSERT_MESSAGE(pT, "can't load value, wrong node type");
            return pT;
        }

        ///
        // Creates the new metadata node from the given value_type
        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            //return const_cast<value_type>(val);
            return llvm::ConstantAsMetadata::get(val);
        }

        ///
        // Indicates that the value_type was changed
        static bool dirty(const value_type& val)
        {
            return false;
        }

        ///
        // Discard value changes
        static void discardChanges(value_type& val)
        {
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            generateValue(context, val);
        }
    };

    ///
    // Metadata traits specialization for the std::string type
    // We are storing std::string by value
    template<>
    struct MDValueTraits<std::string, void>
    {
        typedef std::string value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                return std::string();
            }

            llvm::MDString* mdStr = llvm::dyn_cast<llvm::MDString>(pNode);
            IGC_ASSERT_MESSAGE(mdStr, "can't load string, wrong node type");
            return value_type(mdStr->getString());
        }

        static llvm::MDString* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            return llvm::MDString::get(context, val);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        static void discardChanges(value_type& val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }
    };

    template<>
    struct MDValueTraits<bool, void>
    {
        typedef bool value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            //we allow for NULL value loads
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                return value_type();
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pNode);
            llvm::ConstantInt* pval = llvm::dyn_cast<llvm::ConstantInt>(val->getValue());
            IGC_ASSERT_MESSAGE(pval, "can't load bool value, wrong node type");
            return pval->isOne();
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            llvm::Constant* v = val
                ? llvm::ConstantInt::getTrue(context)
                : llvm::ConstantInt::getFalse(context);
            return llvm::ConstantAsMetadata::get(v);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        // Take val by value, not by reference as in other specializations,
        // as we use vector<bool> in MetaData templates, which has a specialization in STL,
        // that prevents us from taking bool& here.
        static void discardChanges(value_type val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }

    };

    template<>
    struct MDValueTraits<int64_t, void>
    {
        typedef int64_t value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            //we allow for NULL value loads
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                return value_type();
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pNode);
            llvm::ConstantInt* pval = llvm::dyn_cast<llvm::ConstantInt>(val->getValue());
            if (!pval)
            {
                return 0;
            }
            return pval->getValue().getSExtValue();
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            llvm::Constant* v =
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), val);
            return llvm::ConstantAsMetadata::get(v);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        static void discardChanges(value_type& val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }
    };

    template<>
    struct MDValueTraits<int32_t, void>
    {
        typedef int32_t value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            //we allow for NULL value loads
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                return value_type();
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pNode);
            llvm::ConstantInt* pval = llvm::dyn_cast<llvm::ConstantInt>(val->getValue());
            if (!pval)
            {
                return 0;
            }
            return (int32_t)pval->getValue().getSExtValue();
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            llvm::Constant* v =
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), val);
            return llvm::ConstantAsMetadata::get(v);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        static void discardChanges(value_type& val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }
    };

    template<>
    struct MDValueTraits<int8_t, void>
    {
        typedef int8_t value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            //we allow for NULL value loads
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                return value_type();
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pNode);
            llvm::ConstantInt* pval = llvm::dyn_cast<llvm::ConstantInt>(val->getValue());
            IGC_ASSERT_MESSAGE(pval, "can't load int value, wrong node type");
            return (int8_t)pval->getValue().getSExtValue();
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            llvm::Constant* v =
                llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), val);
            return llvm::ConstantAsMetadata::get(v);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        static void discardChanges(value_type& val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }
    };

    template<>
    struct MDValueTraits<float, void>
    {
        typedef float value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            //we allow for NULL value loads
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                return value_type();
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pNode);
            llvm::ConstantFP* pval = llvm::dyn_cast<llvm::ConstantFP>(val->getValue());
            if (!pval)
            {
                return 0.0f;
            }
            return pval->getValueAPF().convertToFloat();
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            llvm::Constant* v =
                llvm::ConstantFP::get(llvm::Type::getFloatTy(context), val);
            return llvm::ConstantAsMetadata::get(v);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        static void discardChanges(value_type& val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }
    };


    template<>
    struct MDValueTraits<llvm::Function, void>
    {
        typedef llvm::Function* value_type;

        static value_type load(llvm::Metadata* pNode)
        {
            if (NULL == pNode) // || !pNode->hasValueHandle())
            {
                // it is ok to pass NULL nodes - part of support for optional values
                return NULL;
            }

            llvm::ValueAsMetadata* val = llvm::dyn_cast<llvm::ValueAsMetadata>(pNode);
            value_type pT = llvm::dyn_cast<llvm::Function>(
                val->getValue()->stripPointerCasts());
            IGC_ASSERT_MESSAGE(pT, "can't load value, wrong node type");
            return pT;
        }

        static llvm::Metadata* generateValue(llvm::LLVMContext& context, const value_type& val)
        {
            IGC_UNUSED(context);
            //return static_cast<llvm::Value*>(const_cast<value_type>(val));
            return llvm::ConstantAsMetadata::get(val);
        }

        static bool dirty(const value_type& val)
        {
            IGC_UNUSED(val);
            return false;
        }

        static void discardChanges(value_type& val)
        {
            IGC_UNUSED(val);
        }

        static void save(llvm::LLVMContext& context, llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(context, val);
        }
    };

    template<>
    struct MDValueTraits<llvm::MDNode, void>
    {
        typedef llvm::MDNode* value_type;

        static value_type load(llvm::Metadata* pMD)
        {
            if (nullptr == pMD)
            {
                return nullptr;
            }

            value_type pNode = llvm::dyn_cast<llvm::MDNode>(pMD);
            IGC_ASSERT_MESSAGE(pNode, "can't load value, wrong node type");
            return pNode;
        }

        static llvm::Metadata* generateValue(
            llvm::LLVMContext& ctx, const value_type& val)
        {
            IGC_UNUSED(ctx);
            return const_cast<value_type>(val);
        }

        static bool dirty(const value_type& val) { IGC_UNUSED(val); return false; }
        static void discardChanges(value_type& val) { IGC_UNUSED(val); }

        static void save(llvm::LLVMContext& ctx,
            llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(ctx, val);
        }
    };

    template<>
    struct MDValueTraits<llvm::Metadata, void>
    {
        typedef llvm::Metadata* value_type;

        static value_type load(llvm::Metadata* pMD)
        {
            if (nullptr == pMD)
            {
                return nullptr;
            }

            return pMD;
        }

        static llvm::Metadata* generateValue(
            llvm::LLVMContext& ctx, const value_type& val)
        {
            IGC_UNUSED(ctx);
            return const_cast<value_type>(val);
        }

        static bool dirty(const value_type& val) { IGC_UNUSED(val); return false; }
        static void discardChanges(value_type& val) { IGC_UNUSED(val); }

        static void save(llvm::LLVMContext& ctx,
            llvm::Metadata* trgt, const value_type& val)
        {
            IGC_UNUSED(trgt);
            generateValue(ctx, val);
        }
    };


} //namespace

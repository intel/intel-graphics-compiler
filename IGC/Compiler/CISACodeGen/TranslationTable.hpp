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
#include <llvm/Pass.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

class CodeGenContextWrapper;
class WIAnalysis;
class FastValueMapBase;

class TranslationTable : public llvm::FunctionPass
{

public:
    static char ID;

    TranslationTable();

    void RegisterListener(FastValueMapBase* fvmb)
    {
        m_ValueMaps.push_back(fvmb);
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AU.setPreservesAll();
    }

    bool runOnFunction(llvm::Function& F) override;

    virtual  llvm::StringRef getPassName() const override {
        return "TranslationTable";
    }

    unsigned int GetNumIDs() const { return m_NumIDS; }

    void RegisterNewValueAndAssignID(llvm::Value* val);

private:

    unsigned int m_NumIDS;
    llvm::SmallVector<FastValueMapBase*, 8> m_ValueMaps;

}; //class TranslationTable


//Since we are mapping Value pointers to attribute, to avoid confusion
//we will not call this as 'key to value' mapping but 'value to attribute' mapping

//Each mapped attribute type should provide an implementation of this template
//specialization.
template<typename T>
struct FastValueMapAttributeInfo {
    //no default implementation
};

// Provide FastValueMapAttributeInfo for unsigned ints.
template<> struct FastValueMapAttributeInfo<unsigned> {
    static inline unsigned getEmptyAttribute() { return ~0U; }
};

//===----------------------------------------------------------------------===//
class FastValueMapBase
{
public:
    FastValueMapBase() : m_pTT(nullptr)
    {
    }

    FastValueMapBase(TranslationTable* table) : m_pTT(table)
    {
    }

    //This have to be virtual, since only a concrete class knows the exact
    //type of the attribute which is to be initialized.
    virtual void Update() = 0;

public:
    TranslationTable* m_pTT;

};

//===----------------------------------------------------------------------===//
template<typename A, typename T, typename AttributeInfoT = FastValueMapAttributeInfo<T> >
class FastValueMapImpl : public FastValueMapBase
{
    //No default implementation.
};

#if 0
//===----------------------------------------------------------------------===//
//Let us use std::vector as a type selctor for this specialization.
template<typename T, typename AttributeInfoT>
class FastValueMapImpl<std::vector<T>, T, AttributeInfoT> : public FastValueMapBase
{
public:
    FastValueMapImpl(TranslationTable* table) : FastValueMapBase(table)
    {
        Initialize(table);
    }

    FastValueMapImpl()
    {
    }

    virtual void Update()
    {
        m_containerVector.push_back(AttributeInfoT::getEmptyAttribute());
    }

    void Initialize(TranslationTable* table)
    {
        m_pTT = table;
        const unsigned int numIDS = table->GetNumIDs();
        m_containerVector.reserve(numIDS + (unsigned int)(numIDS * 0.3));
        m_containerVector.resize(numIDS, AttributeInfoT::getEmptyAttribute());
        assert(m_containerVector.size() == numIDS);
    }

    //Semantics:
    //Even if we assume that all values have been registered the attribute
    //for a given value may not have been initialized yet. It is perfectly
    //fine to query a map for such an attribute. Thus, we must detect this case
    //and return a non-initialized value.
    T GetAttributeWithoutCreating(const llvm::Value* val) const
    {

        unsigned int ID = val->GetID();
        if (ID < m_containerVector.size())
        {
            return m_containerVector[ID];
        }
        else
        {
            return AttributeInfoT::getEmptyAttribute();
        }
    }

    //We should always assume that we are setting an attribute for a value
    //that has already been 'registered', thus, having a place in the vector.
    void SetAttribute(const llvm::Value* val, T attr)
    {
        assert(val->GetID() < m_containerVector.size());
        m_containerVector[val->GetID()] = attr;
    }

    T end() const
    {
        return AttributeInfoT::getEmptyAttribute();
    }

    void clear()
    {
        //TODO: so far, do nothing
        //need to think more about this scenario
        m_containerVector.clear();
    }

private:
    std::vector<T> m_containerVector;
};
#endif

//===----------------------------------------------------------------------===//
//Let us use llvm::DenseMap as a type selctor for this specialization.
template<typename T, typename AttributeInfoT>
class FastValueMapImpl<llvm::DenseMap<const llvm::Value*, T>, T, AttributeInfoT> : public FastValueMapBase
{
public:

    FastValueMapImpl(TranslationTable* table) : FastValueMapBase(table)
    {
        Initialize(table);
    }

    FastValueMapImpl()
    {
    }

    void Initialize(TranslationTable* table)
    {
        //DenseMap should be kept at less than 75% of its capacity.
        unsigned int preferredSize = (unsigned int) (table->GetNumIDs() * 1.4); //be little bit conservative

        //Add some more space for growing.
        preferredSize = (unsigned int)(preferredSize * 1.1);

        m_attributeMap.reserve(preferredSize);
    }

    T GetAttributeWithoutCreating(const llvm::Value* val) const
    {
        typename llvm::DenseMap<const llvm::Value*, T>::const_iterator it;
        it = m_attributeMap.find(val);
        if (it != m_attributeMap.end())
        {
            return (*it).second;
        }

        return AttributeInfoT::getEmptyAttribute();
    }

    void SetAttribute(const llvm::Value* val, T attr)
    {
        m_attributeMap[val] = attr;
    }


    virtual void Update()
    {
        //TODO: what do we do? Should we re-size?
        //m_containerVector.push_back(AttributeInfoT::getEmptyAttribute());
    }

    T end() const
    {
        return AttributeInfoT::getEmptyAttribute();
    }

    void clear()
    {
        //TODO: need to think more about this scenario
        m_attributeMap.clear();
    }
private:

    llvm::DenseMap<const llvm::Value*, T> m_attributeMap;


};


#define MapBased
//#define IDBased

#ifdef IDBased
template<typename T, typename AttributeInfoT>
class FastValueMap : public FastValueMapImpl<std::vector<T>, T, AttributeInfoT>
{

};
#endif

#ifdef MapBased
template<typename T, typename AttributeInfoT>
class FastValueMap : public FastValueMapImpl<llvm::DenseMap<const llvm::Value*, T>, T, AttributeInfoT>
{

};
#endif


} //namespace IGC
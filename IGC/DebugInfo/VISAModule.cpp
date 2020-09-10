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
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvmWrapper/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "common/LLVMWarningsPop.hpp"

#include "VISAModule.hpp"
#include "DebugInfoUtils.hpp"
#include "LexicalScopes.hpp"

#include <vector>

#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

VISAModule::VISAModule(llvm::Function * Entry)
{
    m_pEntryFunc = Entry;
    m_pModule = m_pEntryFunc->getParent();
    isCloned = false;
}

VISAModule::~VISAModule()
{
}

VISAModule::const_iterator VISAModule::begin() const
{
    return m_instList.begin();
}

VISAModule::const_iterator VISAModule::end() const
{
    return m_instList.end();
}

void VISAModule::BeginInstruction(Instruction* pInst)
{
    IGC_ASSERT_MESSAGE(!m_instInfoMap.count(pInst), "Instruction emitted twice!");
    // Assume VISA Id was updated by this point, validate that.
    ValidateVisaId();
    unsigned int nextVISAInstId = m_currentVisaId + 1;
    m_instInfoMap[pInst] = InstructionInfo(INVALID_SIZE, nextVISAInstId);
    m_instList.push_back(pInst);
}

void VISAModule::EndInstruction(Instruction* pInst)
{
    IGC_ASSERT_MESSAGE(m_instList.size() > 0, "Trying to end Instruction other than the last one called with begin!");
    IGC_ASSERT_MESSAGE(m_instList.back() == pInst, "Trying to end Instruction other than the last one called with begin!");
    IGC_ASSERT_MESSAGE(m_instInfoMap.count(pInst), "Trying to end instruction more than once!");
    IGC_ASSERT_MESSAGE(m_instInfoMap[pInst].m_size == INVALID_SIZE, "Trying to end instruction more than once!");

    // Assume VISA Id was updated by this point, validate that.
    ValidateVisaId();

    unsigned currInstOffset = m_instInfoMap[pInst].m_offset;
    unsigned nextInstOffset = m_currentVisaId + 1;
    m_instInfoMap[m_instList.back()].m_size = nextInstOffset - currInstOffset;
}

void VISAModule::BeginEncodingMark()
{
    ValidateVisaId();
}

void VISAModule::EndEncodingMark()
{
    UpdateVisaId();
}

unsigned int VISAModule::GetVisaOffset(const llvm::Instruction* pInst) const
{
    InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
    IGC_ASSERT_MESSAGE(itr != m_instInfoMap.end(), "Invalid Instruction");
    return itr->second.m_offset;
}

unsigned int VISAModule::GetVisaSize(const llvm::Instruction* pInst) const
{
    InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
    IGC_ASSERT_MESSAGE(itr != m_instInfoMap.end(), "Invalid Instruction");
    IGC_ASSERT_MESSAGE(itr->second.m_size != INVALID_SIZE, "Invalid Size");
    return itr->second.m_size;
}

unsigned VISAModule::GetFunctionNumber(const char* name)
{
    for (auto it : FuncIDMap)
    {
        if (it.first->getName().compare(name) == 0)
        {
            return it.second;
        }
    }

    auto md = GetModule()->getNamedMetadata("igc.device.enqueue");
    if (md)
    {
        for (unsigned int i = 0; i < md->getNumOperands(); i++)
        {
            auto mdOpnd = md->getOperand(i);
            auto first = dyn_cast_or_null<MDString>(mdOpnd->getOperand(0));
            if (first &&
                first->getString().equals(name))
            {
                auto second = dyn_cast_or_null<MDString>(mdOpnd->getOperand(1));
                if (second)
                {
                    return GetFunctionNumber(second->getString().data());
                }
            }
        }
    }

    // If name lookup fails then check mapping passed. This is useful
    // for where llvm::Function has a different name than
    // its DISubprogram metdata node.
    for (auto it = DISPToFunc->begin(), itEnd = DISPToFunc->end(); it != itEnd; it++)
    {
        auto item = (*it);
        auto DISP = item.first;
        if (DISP->getName().compare(name) == 0)
        {
            auto func = item.second;
            auto lookup = FuncIDMap.find(func);
            if (lookup != FuncIDMap.end())
                return lookup->second;
            else
            {
                IGC_ASSERT_MESSAGE(0, "Unexpected function number");
                return 0;
            }
        }
    }

    IGC_ASSERT_MESSAGE(0, "Unexpected function number");
    return 0;
}

unsigned VISAModule::GetFunctionNumber(const llvm::Function* F)
{
    if (FuncIDMap.size() == 0)
    {
        unsigned int id = 0;
        for (auto funcIt = F->getParent()->begin();
            funcIt != F->getParent()->end();
            funcIt++)
        {
            auto func = &(*funcIt);
            FuncIDMap.insert(std::make_pair(func, id++));
        }
    }

    if (isCloned)
    {
        // If function is cloned and has "$dup at the end
        // then replace name without $dup$0.
        auto funcName = F->getName();
        auto match = funcName.rfind("$dup");

        if (match != llvm::StringRef::npos)
        {
            std::string origFuncName = funcName.substr(0, match).str();

            return GetFunctionNumber(origFuncName.data());
        }
    }

    if (FuncIDMap.find(F) != FuncIDMap.end())
    {
        return FuncIDMap.find(F)->second;
    }

    IGC_ASSERT_MESSAGE(0, "Unexpected function number");

    return 0;
}

bool VISAModule::IsDebugValue(const Instruction* pInst) const
{
    //return isa<DbgDeclareInst>(pInst) || isa<DbgValueInst>(pInst);
    return isa<DbgInfoIntrinsic>(pInst);
}

const MDNode* VISAModule::GetDebugVariable(const Instruction* pInst) const
{
    if (const DbgDeclareInst * pDclInst = dyn_cast<DbgDeclareInst>(pInst))
    {
        return pDclInst->getVariable();
    }
    if (const DbgValueInst * pValInst = dyn_cast<DbgValueInst>(pInst))
    {
        return pValInst->getVariable();
    }
    IGC_ASSERT_MESSAGE(0, "Expected debug info instruction");
    return nullptr;
}

void VISAModule::GetConstantData(const Constant* pConstVal, DataVector& rawData) const
{
    if (dyn_cast<ConstantPointerNull>(pConstVal))
    {
        DataLayout DL(GetDataLayout());
        rawData.insert(rawData.end(), DL.getPointerSize(), 0);
    }
    else if (const ConstantDataSequential * cds = dyn_cast<ConstantDataSequential>(pConstVal))
    {
        for (unsigned i = 0; i < cds->getNumElements(); i++) {
            GetConstantData(cds->getElementAsConstant(i), rawData);
        }
    }
    else if (const ConstantAggregateZero * cag = dyn_cast<ConstantAggregateZero>(pConstVal))
    {
        // Zero aggregates are filled with, well, zeroes.
        DataLayout DL(GetDataLayout());
        const unsigned int zeroSize = (unsigned int)(DL.getTypeAllocSize(cag->getType()));
        rawData.insert(rawData.end(), zeroSize, 0);
    }
    // If this is an sequential type which is not a CDS or zero, have to collect the values
    // element by element. Note that this is not exclusive with the two cases above, so the
    // order of ifs is meaningful.
    else if (
        pConstVal->getType()->isVectorTy() ||
        pConstVal->getType()->isArrayTy() ||
        pConstVal->getType()->isStructTy())
    {
        const int numElts = pConstVal->getNumOperands();
        for (int i = 0; i < numElts; ++i)
        {
            Constant* C = pConstVal->getAggregateElement(i);
            IGC_ASSERT_MESSAGE(C, "getAggregateElement returned null, unsupported constant");
            // Since the type may not be primitive, extra alignment is required.
            GetConstantData(C, rawData);
        }
    }
    // And, finally, we have to handle base types - ints and floats.
    else
    {
        APInt intVal(32, 0, false);
        if (const ConstantInt * ci = dyn_cast<ConstantInt>(pConstVal))
        {
            intVal = ci->getValue();
        }
        else if (const ConstantFP * cfp = dyn_cast<ConstantFP>(pConstVal))
        {
            intVal = cfp->getValueAPF().bitcastToAPInt();
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported constant type");
        }

        const int bitWidth = intVal.getBitWidth();
        IGC_ASSERT_MESSAGE((0 < bitWidth), "Unsupported bitwidth");
        IGC_ASSERT_MESSAGE((bitWidth % 8 == 0), "Unsupported bitwidth");
        IGC_ASSERT_MESSAGE((bitWidth <= 64), "Unsupported bitwidth");

        const uint64_t* val = intVal.getRawData();
        rawData.insert(rawData.end(), (char*)val, ((char*)val) + (bitWidth / 8));
    }
}

const Module* VISAModule::GetModule() const
{
    return m_pModule;
}

const Function* VISAModule::GetEntryFunction() const
{
    return m_pEntryFunc;
}

const LLVMContext& VISAModule::GetContext() const
{
    return m_pModule->getContext();
}

const std::string VISAModule::GetDataLayout() const
{
    return m_pModule->getDataLayout().getStringRepresentation();
}

const std::string& VISAModule::GetTargetTriple() const
{
    return m_triple;
}

void VISAModule::Reset()
{
    m_instList.clear();
    isCloned = false;
    UpdateVisaId();
}

void VISAModule::buildDirectElfMaps()
{
    auto co = getCompileUnit();
    VISAIndexToInst.clear();
    VISAIndexToSize.clear();
    for (VISAModule::const_iterator II = begin(), IE = end(); II != IE; ++II)
    {
        const Instruction* pInst = *II;

        InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
        if (itr == m_instInfoMap.end())
            continue;

        unsigned int currOffset = itr->second.m_offset;
        VISAIndexToInst.insert(std::make_pair(currOffset, pInst));
        unsigned int currSize = itr->second.m_size;
        for (auto index = currOffset; index != (currOffset + currSize); index++)
            VISAIndexToSize.insert(std::make_pair(index,
                std::make_pair(currOffset, currSize)));
    }

    GenISAToVISAIndex.clear();
    for (auto i = 0;
        i != co->CISAIndexMap.size();
        i++)
    {
        auto& item = co->CISAIndexMap[i];
        GenISAToVISAIndex.push_back(std::make_pair(item.second, item.first));
    }

    // Compute all Gen ISA offsets corresponding to each VISA index
    VISAIndexToAllGenISAOff.clear();
    for (auto& item : co->CISAIndexMap)
    {
        auto VISAIndex = item.first;
        auto GenISAOffset = item.second;
        auto it = VISAIndexToAllGenISAOff.find(VISAIndex);
        if (it != VISAIndexToAllGenISAOff.end())
            it->second.push_back(GenISAOffset);
        else
        {
            std::vector<unsigned int> vec;
            vec.push_back(GenISAOffset);
            VISAIndexToAllGenISAOff[VISAIndex] = vec;
        }
    }

    GenISAInstSizeBytes.clear();
    for (auto i = 0; i != co->CISAIndexMap.size() - 1; i++)
    {
        unsigned int size = GenISAToVISAIndex[i + 1].first - GenISAToVISAIndex[i].first;
        GenISAInstSizeBytes.insert(std::make_pair(GenISAToVISAIndex[i].first, size));
    }
    GenISAInstSizeBytes.insert(std::make_pair(GenISAToVISAIndex[GenISAToVISAIndex.size() - 1].first, 16));
}

std::vector<std::pair<unsigned int, unsigned int>> VISAModule::getGenISARange(const InsnRange& Range)
{
    // Given a range, return vector of start-end range for corresponding Gen ISA instructions
    auto start = Range.first;
    auto end = Range.second;

    // Range consists of a sequence of LLVM IR instructions. This function needs to return
    // a range of corresponding Gen ISA instructions. Instruction scheduling in Gen ISA
    // means several independent sub-ranges will be present.
    std::vector<std::pair<unsigned int, unsigned int>> GenISARange;
    bool endNextInst = false;

    auto getNextInst = [](const llvm::Instruction* start)
    {
        // Return consecutive instruction in llvm IR.
        // Iterate to next BB if required.
        if (start->getNextNode())
            return start->getNextNode();
        else if (start->getParent()->getNextNode())
            return &(start->getParent()->getNextNode()->front());
        return (const llvm::Instruction*)nullptr;
    };

    while (1)
    {
        if (!start || !end || endNextInst)
            break;

        if (start == end)
            endNextInst = true;

        // Get VISA index/size for "start" LLVM IR inst
        InstInfoMap::const_iterator itr = m_instInfoMap.find(start);
        if (itr == m_instInfoMap.end())
        {
            start = getNextInst(start);
            continue;
        }
        auto startVISAOffset = itr->second.m_offset;
        // VISASize indicated # of VISA insts emitted for this
        // LLVM IR inst
        auto VISASize = GetVisaSize(start);

        for (unsigned int i = 0; i != VISASize; i++)
        {
            auto VISAIndex = startVISAOffset + i;
            auto it = VISAIndexToAllGenISAOff.find(VISAIndex);
            if (it != VISAIndexToAllGenISAOff.end())
            {
                int lastEnd = -1;
                for (auto& genInst : it->second)
                {
                    unsigned int sizeGenInst = GenISAInstSizeBytes[genInst];

                    if (GenISARange.size() > 0)
                        lastEnd = GenISARange.back().second;

                    if (lastEnd == genInst)
                    {
                        GenISARange.back().second += sizeGenInst;
                    }
                    else
                    {
                        GenISARange.push_back(std::make_pair(genInst, genInst + sizeGenInst));
                    }
                    lastEnd = GenISARange.back().second;
                }
            }
        }

        start = getNextInst(start);
    }

    class Comp
    {
    public:
        bool operator()(const std::pair<unsigned int, unsigned int>& a,
            const std::pair<unsigned int, unsigned int>& b)
        {
            return a.first < b.first;
        }
    } Comp;

    if (GenISARange.size() == 0)
        return GenISARange;

    std::sort(GenISARange.begin(), GenISARange.end(), Comp);

    for (unsigned int i = 0; i != GenISARange.size() - 1; i++)
    {
        if (GenISARange[i].first == (unsigned int)-1 && GenISARange[i].second == (unsigned int)-1)
            continue;

        for (unsigned int j = i + 1; j != GenISARange.size(); j++)
        {
            if (GenISARange[j].first == (unsigned int)-1 && GenISARange[j].second == (unsigned int)-1)
                continue;

            if (GenISARange[j].first == GenISARange[i].second)
            {
                GenISARange[i].second = GenISARange[j].second;
                GenISARange[j].first = (unsigned int)-1;
                GenISARange[j].second = (unsigned int)-1;
            }
        }
    }


    for (auto it = GenISARange.begin(); it != GenISARange.end();)
    {
        if ((*it).first == (unsigned int)-1 && (*it).second == (unsigned int)-1)
        {
            it = GenISARange.erase(it);
            continue;
        }
        it++;
    }

    return GenISARange;
}

bool VISAVariableLocation::IsSampler() const
{
    if (!HasSurface())
        return false;

    auto surface = GetSurface();
    if (surface >= VISAModule::SAMPLER_REGISTER_BEGIN &&
        surface < VISAModule::SAMPLER_REGISTER_BEGIN + VISAModule::SAMPLER_REGISTER_NUM)
        return true;
    return false;
}

bool VISAVariableLocation::IsTexture() const
{
    if (!HasSurface())
        return false;

    auto surface = GetSurface();
    if (surface >= VISAModule::TEXTURE_REGISTER_BEGIN &&
        surface < VISAModule::TEXTURE_REGISTER_BEGIN + VISAModule::TEXTURE_REGISTER_NUM)
        return true;
    return false;
}

bool VISAVariableLocation::IsSLM() const
{
    if (!HasSurface())
        return false;

    auto surface = GetSurface();
    if (surface == VISAModule::LOCAL_SURFACE_BTI + VISAModule::TEXTURE_REGISTER_BEGIN)
        return true;
    return false;
}

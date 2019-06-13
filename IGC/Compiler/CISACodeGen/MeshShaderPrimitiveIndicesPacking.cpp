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


#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
#include "common/Types.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/IGCIRBuilder.h"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/MeshShaderPrimitiveIndicesPacking.hpp"


namespace IGC
{
class MeshShaderPrimitiveIndicesPacking : public llvm::FunctionPass
{
public:
    static char ID; ///< ID used by the llvm PassManager (the value is not important)

    MeshShaderPrimitiveIndicesPacking() :
        llvm::FunctionPass(ID)
    { }

    ~MeshShaderPrimitiveIndicesPacking()
    { }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
    }

    virtual bool runOnFunction(llvm::Function& function) override;

    virtual llvm::StringRef getPassName() const override
    {
        return "MeshShaderPrimitiveIndicesPacking";
    }

private:
    void GetCalls(
        llvm::Function& F,
        std::vector<llvm::GenIntrinsicInst*>& sivOutputs) const;

    bool PatternMatch(
        llvm::Value* primitiveIndex,
        const unsigned int numIndicesPerPrimitive,
        llvm::Value*& base,
        unsigned int& elem) const;

    bool Pack(
        const unsigned int numIndicesPerPrimitive,
        const std::vector<llvm::GenIntrinsicInst*>& sivOutputs) const;
};

char MeshShaderPrimitiveIndicesPacking::ID = 0;

void MeshShaderPrimitiveIndicesPacking::GetCalls(
    llvm::Function& F,
    std::vector<llvm::GenIntrinsicInst*>& sivOutputs) const
{
    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            const llvm::GenISAIntrinsic::ID sivGenIntrinsicID = llvm::GenISAIntrinsic::GenISA_OUTPUT;
            const llvm::GenISAIntrinsic::ID primitveDataGenIntrinsicID = llvm::GenISAIntrinsic::GenISA_OutputMeshPrimitiveData; // llvm::GenISAIntrinsic::GenISA_OutputTessControlPoint
            const llvm::GenISAIntrinsic::ID vertexDataGenIntrinsicID = llvm::GenISAIntrinsic::GenISA_OutputMeshVertexData;

            if (llvm::GenIntrinsicInst* inst = llvm::dyn_cast<llvm::GenIntrinsicInst>(II))
            {
                if (inst->getIntrinsicID() == sivGenIntrinsicID)
                {
                    const ShaderOutputType usage = static_cast<ShaderOutputType>(
                        llvm::cast<llvm::ConstantInt>(inst->getOperand(4))->getZExtValue());
                    if (usage == SHADER_OUTPUT_TYPE_PRIMITIVE_INDICES)
                    {
                        sivOutputs.push_back(inst);
                    }
                }
            }
        }
    }
}

bool MeshShaderPrimitiveIndicesPacking::PatternMatch(
    llvm::Value* primitiveIndex,
    const unsigned int numIndicesPerPrimitive,
    llvm::Value*& base,
    unsigned int& elem) const
{
    bool result = false;
    if (llvm::isa<llvm::ConstantInt>(primitiveIndex))
    {
        unsigned int index = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(primitiveIndex)->getZExtValue());
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(primitiveIndex->getContext());
        base = llvm::ConstantInt::get(i32Ty, index / numIndicesPerPrimitive);
        elem = index % numIndicesPerPrimitive;
        result = true;
    }
    else if (llvm::isa<llvm::Instruction>(primitiveIndex))
    {
        llvm::Instruction* inst = llvm::cast<llvm::Instruction>(primitiveIndex);
        if (inst->getOpcode() == llvm::Instruction::Add &&
            llvm::isa<llvm::Instruction>(inst->getOperand(0)) &&
            llvm::isa<llvm::ConstantInt>(inst->getOperand(1)))
        {
            llvm::Instruction* src0 = llvm::cast<llvm::Instruction>(inst->getOperand(0));
            elem = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
            if (elem < numIndicesPerPrimitive &&
                src0->getOpcode() == llvm::Instruction::Mul &&
                llvm::isa<llvm::ConstantInt>(src0->getOperand(1)) &&
                numIndicesPerPrimitive == llvm::cast<llvm::ConstantInt>(src0->getOperand(1))->getZExtValue())
            {
                base = src0->getOperand(0);
                result = true;
            }
        }
        else if (inst->getOpcode() == llvm::Instruction::Mul &&
            llvm::isa<llvm::ConstantInt>(inst->getOperand(1)) &&
            numIndicesPerPrimitive == llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue())
        {
            base = inst->getOperand(0);
            elem = 0;
            result = true;
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// @brief Verifies if indices can be packed and does the packing.
// Currently only a limited number of cases is supported. Indices can be packed
// if:
//  - all output calls are in the same basic block
//  - output indices are either constant or can be pattern matched with the
//    following:
//        outputIndex = base * numVeritcesInPrimitve + elem
//  - a primitive index can only we written once
bool MeshShaderPrimitiveIndicesPacking::Pack(
    const unsigned int numIndicesPerPrimitive,
    const std::vector<llvm::GenIntrinsicInst*>& sivOutputs) const
{
    bool canPack = true;
    std::map<std::pair<llvm::Value*, unsigned int>, llvm::GenIntrinsicInst*> sortedCalls;
    for (auto ci : sivOutputs)
    {
        if (ci->getParent() != sivOutputs.front()->getParent())
        {
            // Primitive indices are written from different basic blocks.
            canPack = false;
            break;
        }
        llvm::Value* base;
        unsigned int elem;
        canPack = PatternMatch(ci->getOperand(5), numIndicesPerPrimitive, base, elem);
        if (!canPack)
        {
            // output index wasn't matched
            break;
        }
        auto val = std::make_pair(std::make_pair(base, elem), ci);
        auto it = sortedCalls.insert(val);
        if (it.second == false)
        {
            // primitive index was written more than once
            canPack = false;
            break;
        }
    }

    // Check if for each base (primitive), all associated elements (vertex indices) are set
    if (canPack)
    {
        for (const auto& entry : sortedCalls)
        {
            llvm::Value* const base = entry.first.first;
            for (unsigned int elem = 0; elem < numIndicesPerPrimitive; elem++)
            {
                auto key = std::make_pair(base, elem);
                if (sortedCalls.find(key) == sortedCalls.end())
                {
                    canPack = false;
                    break;
                }
            }
            if (!canPack)
            {
                break;
            }
        }
    }

    if (canPack)
    {
        llvm::IGCIRBuilder<> builder(sivOutputs.front()->getParent()->getTerminator());
        llvm::Function*  outputFunction = llvm::GenISAIntrinsic::getDeclaration(
            builder.GetInsertBlock()->getParent()->getParent(),
            llvm::GenISAIntrinsic::GenISA_OUTPUT,
            builder.getFloatTy());

        llvm::Value* undef = llvm::UndefValue::get(builder.getFloatTy());

        while (!sortedCalls.empty())
        {
            auto entry = sortedCalls.begin();
            llvm::Value* base = entry->first.first;

            llvm::Value* primitiveIndices = builder.getInt32(0);
            for (unsigned int elem = 0; elem < numIndicesPerPrimitive; elem++)
            {
                auto key = std::make_pair(base, elem);
                auto it = sortedCalls.find(key);
                assert(it != sortedCalls.end());
                llvm::GenIntrinsicInst* ci = it->second;
                llvm::Value* vertexIndex = builder.CreateBitCast(ci->getOperand(0), builder.getInt32Ty());
                primitiveIndices = builder.CreateOr(
                    primitiveIndices,
                    builder.CreateShl(vertexIndex, builder.getInt32(elem * 8)));
                sortedCalls.erase(it);
            }
            primitiveIndices = builder.CreateBitCast(primitiveIndices, builder.getFloatTy());
            llvm::Value* args[] = {
                primitiveIndices, undef, undef, undef,
                builder.getInt32(SHADER_OUTPUT_TYPE_PRIMITIVE_INDICES),
                base };
            builder.CreateCall(outputFunction, args);
        }
    }

    return canPack;
}


///////////////////////////////////////////////////////////////////////////////
bool MeshShaderPrimitiveIndicesPacking::runOnFunction(llvm::Function& F)
{
    bool modified = false;
    auto& md = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->msInfo;

    const IGC::GFX3DMESH_OUTPUT_TOPOLOGY primitveTopology =
        static_cast<IGC::GFX3DMESH_OUTPUT_TOPOLOGY>(md.PrimitiveTopology);
    if (primitveTopology == IGC::GFX3DMESH_OUTPUT_TOPOLOGY::LINE ||
        primitveTopology == IGC::GFX3DMESH_OUTPUT_TOPOLOGY::TRI)
    {
        std::vector<llvm::GenIntrinsicInst*> sivOutputs;
        GetCalls(F, sivOutputs);

        if (!sivOutputs.empty())
        {
            assert(md.IndexFormat == static_cast<unsigned int>(IGC::GFX3DMESH_INDEX_FORMAT::NUM_MAX) &&
                "More than one function contains these calls.");

            const unsigned int numIndicesPerPrimitive = primitveTopology == IGC::GFX3DMESH_OUTPUT_TOPOLOGY::TRI ? 3 : 2;
            bool packed = Pack(numIndicesPerPrimitive, sivOutputs);
            if (packed)
            {
                modified = true;
                md.IndexFormat = static_cast<unsigned int>(IGC::GFX3DMESH_INDEX_FORMAT::U888X);
                for (llvm::GenIntrinsicInst* ci : sivOutputs)
                {
                    ci->eraseFromParent();
                }
            }
            else
            {
                md.IndexFormat = static_cast<unsigned int>(IGC::GFX3DMESH_INDEX_FORMAT::U32);
            }
        }
    }
    else
    {
        assert(primitveTopology == IGC::GFX3DMESH_OUTPUT_TOPOLOGY::POINT);
        md.IndexFormat = static_cast<unsigned int>(IGC::GFX3DMESH_INDEX_FORMAT::U32);
    }

    return modified;
}

//////////////////////////////////////////////////////////////////////////
llvm::Pass* createMeshShaderPrimitiveIndicesPacking()
{
    return new MeshShaderPrimitiveIndicesPacking();
}
}

namespace llvm
{
#define PASS_FLAG "igc-mesh-shader-primitive-indices-packing"
#define PASS_DESCRIPTION "Mesh shader primitive indices packing"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
using IGC::MeshShaderPrimitiveIndicesPacking;
IGC_INITIALIZE_PASS_BEGIN(MeshShaderPrimitiveIndicesPacking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MeshShaderPrimitiveIndicesPacking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
}

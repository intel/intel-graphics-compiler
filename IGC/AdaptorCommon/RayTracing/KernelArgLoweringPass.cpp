/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// After running ConvertDXIL (at least through the DX path), accesses to
/// root signature entries (i.e., descriptor tables, root constants and root
/// descriptors) will be represented by calls to GenISA_RuntimeValue and
/// GenISA_LocalRootSignatureValue.  We don't want to complicate ConvertDXIL
/// with extra raytracing specific processing so we then lower those intrinsics
/// to global and local pointer offsets.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include <vector>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/Optional.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace llvm;

class BindlessKernelArgLoweringPass : public ModulePass
{
public:
    BindlessKernelArgLoweringPass(): ModulePass(ID)
    {
        initializeBindlessKernelArgLoweringPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "BindlessKernelArgLoweringPass";
    }

    static char ID;
};

char BindlessKernelArgLoweringPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG1 "Bindless-Kernel-argument-lowering-pass"
#define PASS_DESCRIPTION1 "lowering kernel arguments"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(BindlessKernelArgLoweringPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(BindlessKernelArgLoweringPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

struct SlotInfo
{
    uint32_t GEPIdx = 0;
    Type* Ty = nullptr;
    SlotInfo(Type* Ty) : Ty(Ty) {}
};

using Slots = std::map<uint64_t, SlotInfo>;

// A given function may access at most one global root signature and/or one
// local root signature (you could, for example, have a module in which every
// shader has a unique global or local root signature).
//
// This computes, for that function, the struct type to access a root signature
// based on the 'RuntimeValue' and 'LocalRootSignatureValue' uses that we
// observe in the shader.  Note that the accesses don't have to be contiguous
// so there may be padding inserted into the struct.
static StructType* processSlot(Function& F, Slots& Slot, uint32_t Align, bool isGlobalRootSig, const Twine &Name = "")
{
    auto& DL = F.getParent()->getDataLayout();
    auto& C = F.getContext();

    SmallVector<Type*, 4> Tys;

    uint64_t CurrIdx = 0;
    uint64_t CurrConsumedBytes = 0;
    for (auto& P : Slot)
    {
        // The offset in the global root signature is in dwords, so convert it to bytes.
        // The offset in the local root signature is already in bytes.
        uint64_t ByteOffset = isGlobalRootSig ? sizeof(DWORD) * P.first : P.first;
        if (CurrConsumedBytes < ByteOffset)
        {
            // Inject explicit padding
            uint64_t Padding = ByteOffset - CurrConsumedBytes;
            auto* ArrayTy = ArrayType::get(Type::getInt8Ty(C), Padding);
            Tys.push_back(ArrayTy);
            CurrConsumedBytes = ByteOffset;
            CurrIdx++;
        }
        SlotInfo& SI = P.second;
        SI.GEPIdx = (uint32_t)CurrIdx;

        Tys.push_back(SI.Ty);
        CurrConsumedBytes += DL.getTypeAllocSize(SI.Ty);
        CurrIdx++;
    }

    RTBuilder::injectPadding(*F.getParent(), Tys, Align, true);

    auto* RootSigTy = !Name.isTriviallyEmpty() ?
        StructType::create(C, Tys, Name.str(), true) :
        StructType::get(C, Tys, true);
    return RootSigTy;
}

static std::pair<Type*, Type*> getSlots(
    Function &F, Slots& GlobalSlots, Slots& LocalSlots)
{
    for (auto& I : instructions(F))
    {
        if (isa<GenIntrinsicInst>(&I, GenISAIntrinsic::GenISA_RuntimeValue))
        {
            // If an array of AccelerationStructures is accessed indirectly
            // a slot for entire array should be created.
            if (!isa<ConstantInt>(I.getOperand(0)))
            {
                // Accesses to an array of AccelerationStructures are marked metadata,
                // which stores the size of an array and its offset in the payload.
                llvm::MDNode* accArrayOffsetNode = I.getMetadata("accelerationStructureArrayOffset");
                llvm::MDNode* accArraySizeNode = I.getMetadata("accelerationStructureArraySize");

                if ((accArrayOffsetNode != nullptr) && (accArraySizeNode != nullptr))
                {
                    llvm::Value* operandValue = llvm::cast<llvm::ValueAsMetadata>(accArrayOffsetNode->getOperand(0))->getValue();
                    uint32_t accArrayOffset = (uint32_t)llvm::cast<llvm::ConstantInt>(operandValue)->getZExtValue();

                    operandValue = llvm::cast<llvm::ValueAsMetadata>(accArraySizeNode->getOperand(0))->getValue();
                    uint32_t accArraySize = (uint32_t)llvm::cast<llvm::ConstantInt>(operandValue)->getZExtValue();

                    llvm::Type* arrayType = llvm::ArrayType::get(Type::getInt64Ty(F.getContext()), accArraySize);

                    GlobalSlots.insert(std::make_pair(accArrayOffset, SlotInfo(arrayType)));
                }
            }
            else
            {
                uint64_t DwordOffset =
                    cast<ConstantInt>(I.getOperand(0))->getZExtValue();
                GlobalSlots.insert(std::make_pair(DwordOffset, SlotInfo(I.getType())));
            }
        }
        else if (isa<GenIntrinsicInst>(&I, GenISAIntrinsic::GenISA_LocalRootSignatureValue))
        {
            // If an array is accessed indirectly, a slot for entire array should be created.
            if (!isa<ConstantInt>(I.getOperand(0)))
            {
                // Array access related data is passed through metadata.
                // Metadata stores the size of an array and its offset in shader record buffer.
                llvm::MDNode* arrayOffsetNode = I.getMetadata("shaderRecordArrayOffset");
                llvm::MDNode* arraySizeNode = I.getMetadata("shaderRecordArraySize");

                if ((arrayOffsetNode != nullptr) && (arraySizeNode != nullptr))
                {
                    llvm::Value* operandValue = llvm::cast<llvm::ValueAsMetadata>(arrayOffsetNode->getOperand(0))->getValue();
                    uint32_t accArrayOffset = (uint32_t)llvm::cast<llvm::ConstantInt>(operandValue)->getZExtValue();

                    operandValue = llvm::cast<llvm::ValueAsMetadata>(arraySizeNode->getOperand(0))->getValue();
                    uint32_t accArraySize = (uint32_t)llvm::cast<llvm::ConstantInt>(operandValue)->getZExtValue();

                    llvm::Type* arrayType = llvm::ArrayType::get(I.getType(), accArraySize);

                    LocalSlots.insert(std::make_pair(accArrayOffset, SlotInfo(arrayType)));
                }
            }
            else
            {
                uint64_t byteOffset =
                    cast<ConstantInt>(I.getOperand(0))->getZExtValue();
               LocalSlots.insert(std::make_pair(byteOffset, SlotInfo(I.getType())));
            }
        }
    }

    Type *GlobalTy = [&]()
    {
        std::string Name = std::string("IGC::RTGlobalsAndRootSig::") + F.getName().str();
        auto* GlobalRootSigTy = processSlot(
            F, GlobalSlots, alignof(IGC::TypeHoleGlobalRootSig), true);

        auto* CombinedTy =
            RTBuilder::getRTGlobalsAndRootSig(
                *F.getParent(),
                GlobalRootSigTy,
                Name);

        return CombinedTy;
    }();

    Type *LocalTy = [&]()
    {
        std::string Name = std::string("IGC::LocalRootSig::") + F.getName().str();
        auto* LocalRootSigTy = processSlot(
            F, LocalSlots, alignof(RTStackFormat::TypeHoleLocalRootSig), false, Name);

        return LocalRootSigTy;
    }();

    return std::make_pair(GlobalTy, LocalTy);
}

bool BindlessKernelArgLoweringPass::runOnModule(Module &M)
{
    auto* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    std::vector<Instruction*> toDelete;
    RTBuilder builder(M.getContext(), *pCtx);
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        Slots GlobalSlots;
        Slots LocalSlots;
        auto [GlobalStructTy, LocalStructTy] = getSlots(
            F, GlobalSlots, LocalSlots);
        for (auto& I : instructions(F))
        {
            bool found = false;
            Value* basePtr = nullptr;
            Instruction* intrin = &I;
            builder.SetInsertPoint(intrin);

            if (isa<GenIntrinsicInst>(intrin, GenISAIntrinsic::GenISA_RuntimeValue))
            {
                basePtr = builder.getGlobalBufferPtr();
                uint32_t AddrSpace = basePtr->getType()->getPointerAddressSpace();
                basePtr = builder.CreateBitCast(
                    basePtr,
                    GlobalStructTy->getPointerTo(AddrSpace),
                    VALUE_NAME("&RTGlobalsAndRootSig"));

                SmallVector<Value*, 4> Indices;

                // If an array of AccelerationStructures is accessed indirectly,
                // create a GEP with a proper number of indices.
                if (!isa<ConstantInt>(intrin->getOperand(0)))
                {
                    // Accesses to an array of AccelerationStructures are marked metadata,
                    // which stores AccelerationStructures array offset in the payload.
                    llvm::MDNode* accArrayOffsetNode = intrin->getMetadata("accelerationStructureArrayOffset");

                    if (accArrayOffsetNode != nullptr)
                    {
                        // Get the offset from the payload from metadata.
                        llvm::Value* operandValue = llvm::cast<llvm::ValueAsMetadata>(accArrayOffsetNode->getOperand(0))->getValue();
                        uint32_t accArrayOffset = (uint32_t)llvm::cast<llvm::ConstantInt>(operandValue)->getZExtValue();

                        auto I = GlobalSlots.find(accArrayOffset);

                        // Subtract the offset from index taken from RuntimeValue intrinsic.
                        // The offset will be applied in the GEP indices as the value from
                        // GlobalSlots.
                        llvm::Value* gepIndex = builder.CreateSub(intrin->getOperand(0), builder.getInt32(accArrayOffset));

                        Indices =
                        {
                            builder.getInt32(0),
                            builder.getInt32(1), // access the global root sig
                            builder.getInt32(I->second.GEPIdx),
                            gepIndex
                        };
                    }
                    else
                    {
                        IGC_ASSERT_MESSAGE(0, "Missing AccelerationStructure array info metadata.");
                    }
                }
                else
                {
                    uint64_t DwordOffset =
                        cast<ConstantInt>(intrin->getOperand(0))->getZExtValue();
                    auto I = GlobalSlots.find(DwordOffset);
                    IGC_ASSERT_MESSAGE((I != GlobalSlots.end()), "missing?");

                    Indices =
                    {
                        builder.getInt32(0),
                        builder.getInt32(1), // access the global root sig
                        builder.getInt32(I->second.GEPIdx)
                    };
                }

                static_assert(offsetof(IGC::RTGlobalsAndRootSig, GlobalRootSig) ==
                    sizeof(RayDispatchGlobalData), "changed?");

                basePtr = builder.CreateInBoundsGEP(basePtr, Indices,
                    VALUE_NAME("&GlobalRootSigElt[]"));

                found = true;
            }
            else if (auto* gen_intrinsic = dyn_cast<GenIntrinsicInst>(intrin, GenISAIntrinsic::GenISA_LocalRootSignatureValue))
            {
                Optional<unsigned> RootSigSize;
                if (uint64_t Size = gen_intrinsic->getImm64Operand(1))
                    RootSigSize = static_cast<uint32_t>(Size);

                basePtr = builder.getLocalBufferPtr(RootSigSize);

                basePtr = builder.CreateBitCast(
                    basePtr,
                    LocalStructTy->getPointerTo(basePtr->getType()->getPointerAddressSpace()),
                    VALUE_NAME("localPtr"));

                SmallVector<Value*, 4> Indices;

                // If an array is accessed indirectly, create a GEP with a proper number of indices.
                if (!isa<ConstantInt>(intrin->getOperand(0)))
                {
                    // Metadata stores the size of an array and its offset in shader record buffer.
                    llvm::MDNode* arrayOffsetNode = intrin->getMetadata("shaderRecordArrayOffset");

                    if (arrayOffsetNode != nullptr)
                    {
                        // Get the offset from metadata.
                        llvm::Value* operandValue = llvm::cast<llvm::ValueAsMetadata>(arrayOffsetNode->getOperand(0))->getValue();
                        uint32_t arrayOffset = (uint32_t)llvm::cast<llvm::ConstantInt>(operandValue)->getZExtValue();

                        auto I = LocalSlots.find(arrayOffset);
                        IGC_ASSERT_MESSAGE((I != LocalSlots.end()), "missing?");

                        // Calculate the offset in bytes from the beginning of array
                        llvm::Value* gepIndex = builder.CreateSub(intrin->getOperand(0), builder.getInt32(arrayOffset));
                        // Divide by the size of array element to get element index
                        uint32_t typeSizeInBytes = (uint32_t)(F.getParent()->getDataLayout().getTypeAllocSize(I->second.Ty->getArrayElementType()));
                        gepIndex = builder.CreateUDiv(gepIndex, builder.getInt32(typeSizeInBytes));

                        Indices =
                        {
                            builder.getInt32(0),
                            builder.getInt32(I->second.GEPIdx),
                            gepIndex
                        };
                    }
                    else
                    {
                        IGC_ASSERT_MESSAGE(0, "Missing shader record buffer array info metadata.");
                    }
                }
                else
                {
                    uint64_t byteOffset =
                        cast<ConstantInt>(intrin->getOperand(0))->getZExtValue();
                    auto I = LocalSlots.find(byteOffset);
                    IGC_ASSERT_MESSAGE((I != LocalSlots.end()), "missing?");
                    Indices = {
                        builder.getInt32(0),
                        builder.getInt32(I->second.GEPIdx)
                    };
                }

                basePtr = builder.CreateInBoundsGEP(basePtr, Indices,
                    VALUE_NAME("&LocalRootSigElt[]"));

                found = true;
            }

            if (found)
            {
                LoadInst* LI = builder.CreateLoad(basePtr);
                RTBuilder::setInvariantLoad(LI);
                intrin->replaceAllUsesWith(LI);
                toDelete.push_back(intrin);
            }
        }
    }

    for (auto it : toDelete)
    {
        it->eraseFromParent();
    }

    return true;
}

namespace IGC
{

Pass* CreateBindlessKernelArgLoweringPass(void)
{
    return new BindlessKernelArgLoweringPass();
}

} // namespace IGC

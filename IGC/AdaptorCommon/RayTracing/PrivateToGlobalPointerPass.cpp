/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// DXC generates DXIL code that contains pointers with address space 0
// (aka the private address space).  For example:
//
// define void @hit(i32* noalias %payload,
//                  %struct.BuiltInTriangleIntersectionAttributes* %attr)
//
// for DX12 compilations, we currently always have address space 0 set to
// be 32-bit.  There are two main reasons for this:
//
// 1) We lower 'alloca' instructions into scratch surface accesses.  We want
//    all of the pointer arithmetic offseting into the surface to be 32-bit.
// 2) address space 0 is also the 'default' address space.  When querying the
//    datalayout for pointer size, pointers with address spaces that aren't in
//    the datalayout will default to whatever address space 0 is. We
//    encode bindless and stateful accesses in the address space which turn
//    out to have large values.  They will default to 32-bit, which is what
//    we want.
//
// Arguments like the above 'hit' function are actually stored in the RTStack.
// These will be retrieved via A64 stateless accesses though they come in as
// address space 0 pointers.  The goal of this pass is to convert all private
// pointers to global pointers so that the incoming IR actual represents how
// we want to codegen.  Downstream passes may then, for example, promote those
// global allocas into the usual private allocas so that scratch space would
// be reserved for them rather than the RTStack.

#include "RTBuilder.h"
#include "RayTracingInterface.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/ADT/StringMap.h>
#include <llvmWrapper/Transforms/Utils/ValueMapper.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace llvm;

class PrivatePtrRemapper : public ValueMapTypeRemapper
{
public:
    PrivatePtrRemapper(const ModuleMetaData &MMD)
    {
        SWStackAddrSpace = RTBuilder::getSWStackAddrSpace(MMD);
    }

    // The sole goal of this function is to recursively walk any type and,
    // if there is a private pointer nested within, it will be converted to
    // a global pointer.
    Type* remapType(Type* Ty) override
    {
        if (Ty->isIntegerTy())
            return Ty;

        if (Ty->isFloatingPointTy())
            return Ty;

        if (Ty->isVoidTy())
            return Ty;

        if (Ty->isStructTy() && cast<StructType>(Ty)->isOpaque())
            return Ty;

        if (auto * PTy = dyn_cast<PointerType>(Ty))
        {
            if (PTy->getAddressSpace() == ADDRESS_SPACE_PRIVATE)
            {
                // This is the only line in this function that actually changes
                // a type.  Everything else is structure walking.
                return PointerType::get(
                    remapType(PTy->getPointerElementType()), SWStackAddrSpace);
            }
            else
            {
                return PointerType::get(
                    remapType(PTy->getPointerElementType()),
                    PTy->getPointerAddressSpace());
            }
        }
        else if (auto * ArrayTy = dyn_cast<ArrayType>(Ty))
        {
            return ArrayType::get(remapType(ArrayTy->getArrayElementType()),
                ArrayTy->getNumElements());
        }
        else if (auto * VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
        {
            return IGCLLVM::FixedVectorType::get(remapType(VecTy->getElementType()),
                (uint32_t)VecTy->getNumElements());
        }
        else if (auto * StructTy = dyn_cast<StructType>(Ty))
        {
            bool Packed = StructTy->isPacked();
            auto& C = Ty->getContext();

            SmallVector<Type*, 4> Tys;
            for (auto* FieldTy : StructTy->elements())
                Tys.push_back(remapType(FieldTy));

            // they're actually the same: if this was a named struct then, if we
            // just create a new struct, it will have a different name and the
            // types won't match even though they're structurally the same.
            if (makeArrayRef(Tys) == StructTy->elements())
                return StructTy;

            StructType* NewTy = nullptr;
            if (StructTy->hasName())
            {
                StringRef Name = StructTy->getName();
                auto I = NamedStructsMap.find(Name);
                if (I != NamedStructsMap.end())
                {
                    NewTy = I->second;
                }
                else
                {
                    NewTy = StructType::create(
                        C, Tys, StructTy->getStructName(), Packed);
                    NamedStructsMap.insert(std::make_pair(Name, NewTy));
                }
            }
            else
            {
                NewTy = StructType::get(C, Tys, Packed);
            }

            return NewTy;
        }
        else if (auto * FTy = dyn_cast<FunctionType>(Ty))
        {
            return remapType(FTy);
        }

        IGC_ASSERT_MESSAGE(0, "unknown type!");
        return nullptr;
    }

    FunctionType* remapType(FunctionType* FTy)
    {
        auto* RetTy = remapType(FTy->getReturnType());
        bool isVarArg = FTy->isVarArg();
        SmallVector<Type*, 4> Tys;
        for (auto* ArgTy : FTy->params())
            Tys.push_back(remapType(ArgTy));
        return FunctionType::get(RetTy, Tys, isVarArg);
    }

    void clear()
    {
        NamedStructsMap.clear();
    }
private:
    StringMap<StructType*> NamedStructsMap;
    uint32_t SWStackAddrSpace = 0;
};

//////////////////////////////////////////////////////////////////////////
class PrivateToGlobalPointerPass : public ModulePass
{
public:
    PrivateToGlobalPointerPass() : ModulePass(ID) {}
    bool runOnModule(Module &M) override;

    StringRef getPassName() const override
    {
        return "PrivateToGlobalPointerPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }
    static char ID;
private:
    Function* updateMap(
        Function& F,
        PrivatePtrRemapper &Remapper,
        ValueToValueMapTy &VM);
    CodeGenContext* m_Ctx;
};

char PrivateToGlobalPointerPass::ID = 0;

Function* PrivateToGlobalPointerPass::updateMap(
    Function& F,
    PrivatePtrRemapper& Remapper,
    ValueToValueMapTy& VM)
{
    auto* NewFuncTy = Remapper.remapType(F.getFunctionType());

    // Intrinsic types are completely determined by their declaration because
    // they have no function body.
    if (F.isIntrinsic())
    {
        // This is the shortcut case where most intrinsics won't have a private
        // pointer so we just leave it untouched.
        if (F.getFunctionType() == NewFuncTy)
        {
            VM[&F] = &F;
            return nullptr;
        }
        // Skip the gen intrinsics: they will be handled below.
        else if (F.getIntrinsicID() != Intrinsic::not_intrinsic)
        {
            auto* NewFunc = RTBuilder::updateIntrinsicMangle(NewFuncTy, F);
            VM[&F] = NewFunc;
            return NewFunc;
        }
    }

    auto* NewFunc = Function::Create(
        NewFuncTy, F.getLinkage(), F.getName(), F.getParent());

    VM[&F] = NewFunc;

    NewFunc->stealArgumentListFrom(F);
    // We don't take the name for an intrinsic so that we don't shadow an
    // overload string that actually matches the function type.  For example:
    //
    // declare void @llvm.genx.func.p0i8(i8*)
    //
    // When we transform this function, the name would stay the same though it
    // would now be:
    //
    // declare void @llvm.genx.func.p0i8(i8 addrspace(1)*)
    //
    // If a later pass wanted to actually create a this overloaded intrinsic
    // with a private pointer, that name would already exist for the global.
    // Instead, we get a unique symbol now so it would look like:
    //
    // declare void @llvm.genx.func.p0i8.0(i8 addrspace(1)*) ; note trailing '0'
    //
    // Ideally we would remangle the name to the appropriate type but it's not
    // clear that there is a generic way to determine which arguments are
    // overloads so this suffices.
    // Note: this is only for gen intrinsics as regular builtin intrinsics
    // are handled generically above.
    if (!F.isIntrinsic())
        NewFunc->takeName(&F);

    NewFunc->copyMetadata(&F, 0);
    NewFunc->copyAttributesFrom(&F);
    NewFunc->setSubprogram(F.getSubprogram());
    F.setSubprogram(nullptr);

    if (!F.isDeclaration())
    {
        NewFunc->getBasicBlockList().splice(NewFunc->end(), F.getBasicBlockList());

        IGCMD::IGCMetaDataHelper::moveFunction(
            *m_Ctx->getMetaDataUtils(), *m_Ctx->getModuleMetaData(), &F, NewFunc);
    }

    return NewFunc;
}

bool PrivateToGlobalPointerPass::runOnModule(Module &M)
{
    ValueToValueMapTy VM;
    m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    PrivatePtrRemapper Remapper(*m_Ctx->getModuleMetaData());

    SmallVector<Function*, 4> Funcs;
    for (auto& F : M)
        Funcs.push_back(&F);

    SmallVector<Function*, 4> NewFuncs;
    for (auto* F : Funcs)
    {
        auto* NewFunc = updateMap(*F, Remapper, VM);
        if (NewFunc)
            NewFuncs.push_back(NewFunc);
    }

    for (auto* F : NewFuncs)
        RemapFunction(
            *F, VM, RF_IgnoreMissingLocals | RF_ReuseAndMutateDistinctMDs, &Remapper);

    for (auto* F : Funcs)
    {
        if (F->use_empty())
            F->eraseFromParent();
    }

    m_Ctx->getMetaDataUtils()->save(M.getContext());
    DumpLLVMIR(m_Ctx, "PrivateToGlobalPointer");
    return true;
}

namespace IGC
{
    Pass* createPrivateToGlobalPointerPass()
    {
        return new PrivateToGlobalPointerPass();
    }
}

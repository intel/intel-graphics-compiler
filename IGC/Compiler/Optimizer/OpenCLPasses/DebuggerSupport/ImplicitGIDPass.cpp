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
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/Attributes.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DebuggerSupport/ImplicitGIDPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "DebugInfo/DebugInfoUtils.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-add-implicit-gid"
#define PASS_DESCRIPTION "Implicit Global Id Pass - Add parameters for debugging"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ImplicitGlobalId, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ImplicitGlobalId, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ImplicitGlobalId::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(CleanImplicitIds, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CleanImplicitIds, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CleanImplicitIds::ID = 0;

ImplicitGlobalId::ImplicitGlobalId() : ModulePass(ID)
{
    initializeImplicitGlobalIdPass(*llvm::PassRegistry::getPassRegistry());
}

bool ImplicitGlobalId::runOnModule(Module& M)
{
    if (!DebugInfoUtils::HasDebugInfo(M))
    {
        // If there is no debug info, then it must be GenISA debugger.
        // Thus, no need to add OpenCL global id variable. Just return.
        return false;
    }
    m_pModule = &M;
    m_pDIB = std::unique_ptr<DIBuilder>(new DIBuilder(M));

    m_pContext = &M.getContext();

    // Prime a DebugInfoFinder that can be queried about various bits of
    // debug information in the module.
    //m_DbgInfoFinder = DebugInfoFinder();
    //m_DbgInfoFinder.processModule(M);

    const llvm::DataLayout* pDL = &M.getDataLayout();

    m_uiSizeT = pDL->getPointerSizeInBits();
    IGC_ASSERT(m_uiSizeT == 32 || m_uiSizeT == 64);

    bool changed = false;
    for (Module::iterator fi = M.begin(), fe = M.end(); fi != fe; ++fi)
    {
        if (!(*fi).isDeclaration())
        {
            changed |= runOnFunction(*fi);
        }
    }

    return changed;
}

bool ImplicitGlobalId::runOnFunction(Function& F)
{
    IGC_ASSERT_MESSAGE(!F.isDeclaration(), "Expect kernel functions, which must be defined");

    insertComputeIds(&F);
    return true;
}

std::vector<llvm::Value*> ImplicitGlobalId::runOnBasicBlock(llvm::Instruction* insertBefore, GlobalOrLocal wi, DebugLoc loc)
{
    IRBuilder<> B(insertBefore);
    B.SetCurrentDebugLocation(loc);

    Value* id_at_dim = CreateGetId(B, wi);

    std::string name = "__ocl_dbg_gid";
    if (wi == GlobalOrLocal::Local)
    {
        name = "__ocl_dbg_lid";
    }
    else if (wi == GlobalOrLocal::WorkItem)
    {
        name = "__ocl_dbg_grid";
    }

    std::vector<llvm::Value*> vec;
    for (unsigned int i = 0; i != 3; i++)
    {
        auto extractElem = B.CreateExtractElement(id_at_dim, i, Twine(name) + Twine(i));
        if (isa<Instruction>(extractElem))
        {
            cast<Instruction>(extractElem)->setDebugLoc(B.getCurrentDebugLocation());
        }
        vec.push_back(extractElem);
    }

    return vec;
}

void ImplicitGlobalId::insertComputeIds(Function* pFunc)
{
    // Find the first and second instructions in the function
    //
    BasicBlock& entry_block = pFunc->getEntryBlock();
    Instruction* insert_before = &(entry_block.front());

    IGC_ASSERT_MESSAGE(insert_before, "There is no instruction in the current basic block!");

    // Prepare to create debug metadata for implicit gid variables
    //
    llvm::DIScope* scope = nullptr;
    DebugLoc loc;
    auto gotScope = getBBScope(entry_block, scope, loc);
    if (!gotScope)
        return;

    llvm::DIType* gid_di_type = getOrCreateIntDIType();
    SmallVector<uint64_t, 1> NewDIExpr;

    auto gidVals = runOnBasicBlock(insert_before, GlobalOrLocal::Global, loc);
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the gids
        //
        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, gidVals[i]->getName(), nullptr,
            1, gid_di_type);

        m_pDIB->insertDbgValueIntrinsic(gidVals[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);
    }

    // Similar code for local id
    llvm::DIType* lid_di_type = getOrCreateIntDIType();
    auto lidVals = runOnBasicBlock(insert_before, GlobalOrLocal::Local, loc);
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the gids
        //
        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, lidVals[i]->getName(), nullptr,
            1, lid_di_type);

        m_pDIB->insertDbgValueIntrinsic(lidVals[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);
    }

    // Similar code for work item id
    llvm::DIType* grid_di_type = getOrCreateIntDIType();
    auto gridVals = runOnBasicBlock(insert_before, GlobalOrLocal::WorkItem, loc);
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the work item ids
        //
        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, gridVals[i]->getName(), nullptr, 1, grid_di_type);

        m_pDIB->insertDbgValueIntrinsic(gridVals[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);
    }
}

bool ImplicitGlobalId::getBBScope(const BasicBlock& BB, llvm::DIScope*& scope_out, DebugLoc& loc_out)
{
    auto F = BB.getParent();
    if (!F->getSubprogram())
        return false;

    scope_out = F->getSubprogram();
    loc_out = DILocation::get(F->getContext(), F->getSubprogram()->getLine(), 0, scope_out);

    return true;
}

llvm::DIType* ImplicitGlobalId::getOrCreateIntDIType()
{
    std::string typeName = m_uiSizeT == 32 ? "int" : "long long";
    auto it = m_DbgInfoFinder.types();
    for (auto t_i = it.begin();
        t_i != it.end(); ++t_i)
    {
        auto type = (*t_i);
        if (type &&
            type->getName() == typeName)
        {
            return type;
        }
    }
    // If the type wasn't found, create it now
    return m_pDIB->createBasicType(typeName, m_uiSizeT, dwarf::DW_ATE_signed);
}

Value* ImplicitGlobalId::CreateGetId(IRBuilder<>& B, GlobalOrLocal wi)
{
    const std::string nameCall = std::string("globalId");
    std::string nameFunc;
    if (wi == GlobalOrLocal::Global)
    {
        nameFunc = "__builtin_spirv_BuiltInGlobalInvocationId";
    }
    else if (wi == GlobalOrLocal::Local)
    {
        nameFunc = "__builtin_spirv_BuiltInLocalInvocationId";
    }
    else if (wi == GlobalOrLocal::WorkItem)
    {
        nameFunc = "__builtin_spirv_BuiltInWorkgroupId";
    }

    Function* getFunc = m_pModule->getFunction(nameFunc);

    if (!getFunc)
    {
        //Create one
        // Create parameters and return values
        Type* pResult = IGCLLVM::FixedVectorType::get(IntegerType::get(m_pModule->getContext(), m_uiSizeT), 3);
        std::vector<Type*> funcTyArgs;

        // Create function declaration
        FunctionType* pFuncTy = FunctionType::get(pResult, funcTyArgs, false);
        IGC_ASSERT_MESSAGE(pFuncTy, "Failed to create new function type");
        Function* pNewFunc = Function::Create(pFuncTy, GlobalValue::ExternalLinkage, nameFunc, m_pModule);
        IGC_ASSERT_MESSAGE(pNewFunc, "Failed to create new function declaration");

        // Set function attributes
        IGCLLVM::AttributeSet funcAttrs;
        AttrBuilder attBuilder;
        attBuilder.addAttribute(Attribute::NoUnwind).addAttribute(Attribute::ReadNone);
        funcAttrs = IGCLLVM::AttributeSet::get(pNewFunc->getContext(), IGCLLVM::AttributeSet::FunctionIndex, attBuilder);
        pNewFunc->setAttributes(funcAttrs);

        getFunc = pNewFunc;
    }
    // Create call instruction
    std::vector<Value*> args;
    auto callInst = B.CreateCall(getFunc, args, nameCall);

    callInst->setDebugLoc(B.getCurrentDebugLocation());

    return callInst;
}

CleanImplicitIds::CleanImplicitIds() : ModulePass(ID)
{
}

bool CleanImplicitIds::runOnModule(Module& M)
{
    // For every function, check whether redundant pre-defined variables
    // have been emitted. If yes, erase them from program. Each inlined
    // function should refer to kernel's register holding pre-defined
    // variable.

    bool change = false;
    for (auto& F : M)
    {
        if(!F.isDeclaration())
            change |= processFunc(F);
    }

    return change;
}

bool CleanImplicitIds::processFunc(Function& F)
{
    // Each function gets its own copy of pre-defined vairables - __ocl_*.
    // This is done before inlining. These pre-defined variables are
    // immutable. Post inlining, it means there would be as many instances
    // of these variables (and corresponding computation) as are number of
    // inlined callees. This is redundant. This function cleans up all
    // redundant copies of pre-defined variables and their associated
    // computation.
    SmallVector<std::pair<std::string, llvm::DbgValueInst*>, 9> PredefinedInsts;
    SmallVector<DbgValueInst*, 10> ToErase;
    auto DIB = std::unique_ptr<DIBuilder>(new DIBuilder(*F.getParent()));

    bool Changed = false;

    auto FindPredefinedInst = [](auto& PredefinedInsts, std::string& name)
    {
        for (auto& Item : PredefinedInsts)
        {
            if (Item.first == name)
                return Item.second;
        }
        return (llvm::DbgValueInst*)nullptr;
    };

    for (auto& BB : F)
    {
        for (auto& I : BB)
        {
            if (auto DbgVal = dyn_cast_or_null<DbgValueInst>(&I))
            {
                auto Name = DbgVal->getVariable()->getName().str();
                if (DebugInfoUtils::IsSpecialDebugVariable(Name))
                {
                    auto PredefVar = FindPredefinedInst(PredefinedInsts, Name);
                    if (PredefVar == nullptr)
                    {
                        PredefinedInsts.push_back(std::make_pair(Name, DbgVal));
                    }
                    else
                    {
                        // dbg.value from inlined function
                        DIB->insertDbgValueIntrinsic(PredefVar->getValue(), DbgVal->getVariable(),
                            DbgVal->getExpression(), DbgVal->getDebugLoc(), &I);
                        ToErase.push_back(DbgVal);
                        Changed = true;
                    }
                }
            }
        }
    }

    for (auto DbgVal : ToErase)
    {
        auto OldVar = DbgVal->getValue();

        SmallVector<Instruction*, 5> WorkList;
        WorkList.push_back(cast<Instruction>(OldVar));

        DbgVal->eraseFromParent();
        while (WorkList.size() > 0)
        {
            auto Top = WorkList.back();
            WorkList.pop_back();
            if (Top->user_empty())
            {
                for(unsigned int I = 0; I != Top->getNumOperands(); ++I)
                {
                    auto Opnd = Top->getOperand(I);
                    if (isa<Instruction>(Opnd))
                        WorkList.push_back(cast<Instruction>(Opnd));

                }
                Top->eraseFromParent();
            }
            else if (Top->getNumUses() == 1)
            {
                // Catch specific case:
                // %localIdX49 = zext i16 %localIdX to i32
                // % cmp20.i.i.i.i = icmp ult i32 % localIdX49, 65536
                // call void @llvm.assume(i1 % cmp20.i.i.i.i) #5
                // Top is localIdX49 definition
                if (auto CmpI = dyn_cast_or_null<ICmpInst>(*Top->user_begin()))
                {
                    if (CmpI->getOperand(0) == Top &&
                        isa<ConstantInt>(CmpI->getOperand(1)))
                    {
                        if (CmpI->getNumUses() == 1)
                        {
                            if (auto IntrinsicI = dyn_cast_or_null<IntrinsicInst>(*CmpI->user_begin()))
                            {
                                auto IntrinsicId = IntrinsicI->getIntrinsicID();
                                if (IntrinsicId == Intrinsic::assume)
                                {
                                    IntrinsicI->eraseFromParent();
                                    CmpI->eraseFromParent();
                                    // retry
                                    WorkList.push_back(Top);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return Changed;
}

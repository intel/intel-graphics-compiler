/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Attributes.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DebuggerSupport/ImplicitGIDPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/DebugInfo/Utils.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"
#include "Compiler/CISACodeGen/helper.h"

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

#define PASS_FLAG2 "igc-clear-implicit-ids"
#define PASS_DESCRIPTION2 "Clear implicit Id Pass - Clear redundant parameters"
IGC_INITIALIZE_PASS_BEGIN(CleanImplicitIds, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CleanImplicitIds, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY, PASS_ANALYSIS)

char CleanImplicitIds::ID = 0;

ImplicitGlobalId::ImplicitGlobalId() : ModulePass(ID)
{
    initializeImplicitGlobalIdPass(*llvm::PassRegistry::getPassRegistry());
}

bool ImplicitGlobalId::runOnModule(Module& M)
{
    if (!Utils::HasDebugInfo(M))
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

    // When stack calls are enabled, default behavior is to skip these in all functions
    if (F.getCallingConv() != llvm::CallingConv::SPIR_KERNEL &&
        !IGC::ForceAlwaysInline() &&
        IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg))
    {
        // Insert in functions only when reg key is set
        if (IGC_IS_FLAG_DISABLED(EmitPreDefinedForAllFunctions))
        {
            return false;
        }
    }

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
            auto extractElemInst = dyn_cast_or_null<Instruction>(extractElem);

            if (IGC_IS_FLAG_ENABLED(UseOffsetInLocation))
            {
                IGC_ASSERT_MESSAGE(extractElemInst, "__ocl_dbg_ will not be marked as Output");
                if (extractElemInst)
                {
                    // Note: for debugging purposes __ocl_dbg_gid0/1/2, __ocl_dbg_lid0/1/2, __ocl_dbg_grid0/1/2
                    // will be marked as Output to keep its liveness all time
                    auto extractElemInstMD = MDNode::get(m_pModule->getContext(), nullptr);

                    extractElemInst->setMetadata("implicitGlobalID", extractElemInstMD);
                }
            }

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

    auto gidVals = runOnBasicBlock(insert_before, GlobalOrLocal::Global, DebugLoc());

    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the gids
        //
        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, gidVals[i]->getName(), nullptr,
            1, gid_di_type);

        m_pDIB->insertDbgValueIntrinsic(gidVals[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);

        Value* Args[1] = { gidVals[i] };
        Instruction* intrinsicDummy = GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_dummyInstID, Args[0]->getType()),
            Args,
            "",
            insert_before);
        intrinsicDummy->setDebugLoc(loc.get());
        auto intrinsicDummyMD = MDNode::get(m_pModule->getContext(), nullptr);
        intrinsicDummy->setMetadata("implicitGlobalID", intrinsicDummyMD);
        intrinsicDummy->setMetadata(gidVals[i]->getName(), intrinsicDummyMD);
    }

    // Similar code for local id
    llvm::DIType* lid_di_type = getOrCreateIntDIType();
    auto lidVals = runOnBasicBlock(insert_before, GlobalOrLocal::Local, DebugLoc());
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the gids
        //
        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, lidVals[i]->getName(), nullptr,
            1, lid_di_type);

        m_pDIB->insertDbgValueIntrinsic(lidVals[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);

        Value* Args[1] = { lidVals[i] };
        Instruction* intrinsicDummy = GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_dummyInstID, Args[0]->getType()),
            Args,
            "",
            insert_before);
        intrinsicDummy->setDebugLoc(loc.get());
        auto intrinsicDummyMD = MDNode::get(m_pModule->getContext(), nullptr);
        intrinsicDummy->setMetadata("implicitGlobalID", intrinsicDummyMD);
        intrinsicDummy->setMetadata(lidVals[i]->getName(), intrinsicDummyMD);
    }

    // Similar code for work item id
    llvm::DIType* grid_di_type = getOrCreateIntDIType();
    auto gridVals = runOnBasicBlock(insert_before, GlobalOrLocal::WorkItem, DebugLoc());
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the work item ids
        //
        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, gridVals[i]->getName(), nullptr, 1, grid_di_type);

        m_pDIB->insertDbgValueIntrinsic(gridVals[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);

        Value* Args[1] = { gridVals[i] };
        Instruction* intrinsicDummy = GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_dummyInstID, Args[0]->getType()),
            Args,
            "",
            insert_before);
        intrinsicDummy->setDebugLoc(loc.get());
        auto intrinsicDummyMD = MDNode::get(m_pModule->getContext(), nullptr);
        intrinsicDummy->setMetadata("implicitGlobalID", intrinsicDummyMD);
        intrinsicDummy->setMetadata(gridVals[i]->getName(), intrinsicDummyMD);
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
        nameFunc = "_Z26__intel_GlobalInvocationIdv";
    }
    else if (wi == GlobalOrLocal::Local)
    {
        nameFunc = "_Z25__intel_LocalInvocationIdv";
    }
    else if (wi == GlobalOrLocal::WorkItem)
    {
        nameFunc = "_Z19__intel_WorkgroupIdv";
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
        AttributeList funcAttrs;
        AttrBuilder attBuilder;
        attBuilder.addAttribute(Attribute::NoUnwind).addAttribute(Attribute::ReadNone);
        funcAttrs = AttributeList::get(pNewFunc->getContext(), AttributeList::FunctionIndex, attBuilder);
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
    DbgValueInst* PredefinedInsts[__OCL_DBG_VARIABLES] = {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    CallInst* PredefinedDummyInsts[__OCL_DBG_VARIABLES] = {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    SmallVector<DbgValueInst*, 10> ToErase;
    SmallVector<CallInst*, 10> ToEraseDummy;
    auto DIB = std::unique_ptr<DIBuilder>(new DIBuilder(*F.getParent()));

    bool Changed = false;

    auto FindPredefinedInst = [](auto& PredefinedInsts, std::string& name)
    {
        unsigned int idx = Utils::GetSpecialDebugVariableHash(name);
        IGC_ASSERT_MESSAGE(idx < 9, "Name __ocl_dbg_* not found ");
        if (idx < __OCL_DBG_VARIABLES)
            return PredefinedInsts[idx];

        return (llvm::DbgValueInst*)nullptr;
    };

    auto FindPredefinedDummyInst = [](auto& PredefinedDummyInsts, std::string& name)
    {
        unsigned int idx = Utils::GetSpecialDebugVariableHash(name);
        IGC_ASSERT_MESSAGE(idx < 9, "Name __ocl_dbg_* not found ");
        if (idx < __OCL_DBG_VARIABLES)
            return PredefinedDummyInsts[idx];

        return (llvm::CallInst*)nullptr;
    };

    for (auto& BB : F)
    {
        for (auto& I : BB)
        {
            if (auto DbgVal = dyn_cast_or_null<DbgValueInst>(&I))
            {
                auto Name = DbgVal->getVariable()->getName().str();
                if (Utils::IsSpecialDebugVariable(Name))
                {
                    auto PredefVar = FindPredefinedInst(PredefinedInsts, Name);
                    if (PredefVar == nullptr)
                    {
                        unsigned int idx = Utils::GetSpecialDebugVariableHash(Name);
                        IGC_ASSERT_MESSAGE(idx < __OCL_DBG_VARIABLES, "Name __ocl_dbg_* not found ");

                        if (idx < __OCL_DBG_VARIABLES)
                            PredefinedInsts[idx] = DbgVal;
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
            else if (dyn_cast_or_null<CallInst>(&I) &&
                isa<GenIntrinsicInst>(&I) &&
                (static_cast<GenIntrinsicInst*>(&I)->getIntrinsicID() == GenISAIntrinsic::GenISA_dummyInstID) &&
                I.getMetadata("implicitGlobalID"))
            {
                auto CI = dyn_cast_or_null<CallInst>(&I);
                auto Name = Utils::GetSpecialVariableMetaName(&I);
                IGC_ASSERT_MESSAGE(!Name.empty(), "Unexpected dummy instruction");
                if (!Name.empty())
                {
                    auto PredefVar = FindPredefinedDummyInst(PredefinedDummyInsts, Name);

                    if (PredefVar == nullptr)
                    {
                        unsigned int idx = Utils::GetSpecialDebugVariableHash(Name);
                        IGC_ASSERT_MESSAGE(idx < __OCL_DBG_VARIABLES, "Name __ocl_dbg_* not found ");

                        if (idx < __OCL_DBG_VARIABLES)
                            PredefinedDummyInsts[idx] = CI;
                    }
                    else
                    {
                        // Preserving CallInst from inlined function

                        Value* Args[1] = { PredefVar->getArgOperand(0) };
                        Instruction* intrinsicDummy = GenIntrinsicInst::Create(
                            GenISAIntrinsic::getDeclaration(CI->getModule(), GenISAIntrinsic::GenISA_dummyInstID, Args[0]->getType()),
                            Args,
                            "",
                            CI);
                        intrinsicDummy->setDebugLoc(I.getDebugLoc());
                        intrinsicDummy->copyMetadata(I);
                        intrinsicDummy->copyIRFlags(&I);

                        ToEraseDummy.push_back(CI);
                    }
                }
            }
        }
    }

    // Erase all old dummy instructions, which have been replaced by new ones.
    for (auto CI : ToEraseDummy)
    {
        CI->eraseFromParent();
    }

    for (auto DbgVal : ToErase)
    {
        auto OldVar = DbgVal->getValue();

        SmallVector<Instruction*, 5> WorkList;
        if (isa<Instruction>(OldVar)) {
            WorkList.push_back(cast<Instruction>(OldVar));
        }

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
            else if (Top->hasOneUse())
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
                        if (CmpI->hasOneUse())
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

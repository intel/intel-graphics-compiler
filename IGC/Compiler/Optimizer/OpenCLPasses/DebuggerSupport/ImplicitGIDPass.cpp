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
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/Optimizer/OpenCLPasses/DebuggerSupport/ImplicitGIDPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/DebugInfo/DebugInfoUtils.hpp"
#include "Compiler/IGCPassSupport.h"

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
    assert(m_uiSizeT == 32 || m_uiSizeT == 64);

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
    assert(!F.isDeclaration() && "Expect kernel functions, which must be defined");

    insertComputeIds(&F);
    return true;
}

void ImplicitGlobalId::runOnBasicBlock(unsigned i, Instruction *pAlloca, Instruction *insertBefore, GlobalOrLocal wi)
{
    IRBuilder<> B(insertBefore);
    // **********************************************************************
    // DO NOT ADD DEBUG INFO TO THE CODE WHICH GENERATES GET_GLOBAL_ID AND THE
    // STORE, OR THE DEBUGGER WILL NOT BREAK AT A GIVEN GLOBAL_ID
    B.SetCurrentDebugLocation(DebugLoc());
    // **********************************************************************

    Value *id_at_dim = CreateGetId(i, B, wi);

    // get_global/local_id returns size_t, but we want to always pass a 64-bit
    // number. So we may need to extend the result to 64 bits.
    //
    const IntegerType *id_type = dyn_cast<IntegerType>(id_at_dim->getType());
    if (id_type && id_type->getBitWidth() != 64)
    {
        Type *int64Type = IntegerType::getInt64Ty(*m_pContext);
        std::string name = "gid";
        if (wi == GlobalOrLocal::Local)
        {
            name = "lid";
        }
        else if (wi == GlobalOrLocal::WorkItem)
        {
            name = "grid";
        }
        Value *zext_gid = B.CreateZExt(id_at_dim, int64Type, Twine(name) + Twine(i) + Twine("_i64"));

        id_at_dim = zext_gid;
    }
    B.CreateStore(id_at_dim, pAlloca);
}

void ImplicitGlobalId::insertComputeIds(Function* pFunc)
{
    // Find the first and second instructions in the function
    //
    BasicBlock& entry_block = pFunc->getEntryBlock();
    Instruction* insert_before = &(entry_block.front());

    assert( insert_before && "There is no instruction in the current basic block!" );

    // Prepare to create debug metadata for implicit gid variables
    //
    llvm::DIScope* scope = nullptr;
    DebugLoc loc;
    auto gotScope = getBBScope(entry_block, scope, loc);
    if (!gotScope)
        return;

    llvm::DIType* gid_di_type = getOrCreateUlongDIType();
    AllocaInst *gid_alloca[3];
    SmallVector<uint64_t, 1> NewDIExpr;
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the gids
        //
        gid_alloca[i] = new AllocaInst(
            IntegerType::getInt64Ty(*m_pContext), 0, Twine("__ocl_dbg_gid") + Twine(i), insert_before);

        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, gid_alloca[i]->getName(), nullptr,
            1, gid_di_type);

        m_pDIB->insertDeclare(gid_alloca[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);
        runOnBasicBlock(i, gid_alloca[i], insert_before, GlobalOrLocal::Global);
    }

    // Similar code for local id
    llvm::DIType* lid_di_type = getOrCreateUlongDIType();
    AllocaInst *lid_alloca[3];
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the gids
        //
        lid_alloca[i] = new AllocaInst(
            IntegerType::getInt64Ty(*m_pContext), 0, Twine("__ocl_dbg_lid") + Twine(i), insert_before);

        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, lid_alloca[i]->getName(), nullptr,
            1, lid_di_type);

        m_pDIB->insertDeclare(lid_alloca[i], var, m_pDIB->createExpression(NewDIExpr), loc.get() , insert_before);
        runOnBasicBlock(i, lid_alloca[i], insert_before, GlobalOrLocal::Local);
    }

    // Similar code for work item id
    llvm::DIType* grid_di_type = getOrCreateUlongDIType();
    AllocaInst *grid_alloca[3];
    for (unsigned i = 0; i < 3; ++i)
    {
        // Create implicit local variables to hold the work item ids
        //
        grid_alloca[i] = new AllocaInst(
            IntegerType::getInt64Ty(*m_pContext), 0, Twine("__ocl_dbg_grid") + Twine(i), insert_before);

        llvm::DILocalVariable* var = m_pDIB->createAutoVariable(scope, grid_alloca[i]->getName(), nullptr, 1, grid_di_type);

        m_pDIB->insertDeclare(grid_alloca[i], var, m_pDIB->createExpression(NewDIExpr), loc.get(), insert_before);
        runOnBasicBlock(i, grid_alloca[i], insert_before, GlobalOrLocal::WorkItem);
    }
}

bool ImplicitGlobalId::getBBScope(const BasicBlock& BB, llvm::DIScope*& scope_out, DebugLoc& loc_out)
{
    for (BasicBlock::const_iterator BI = BB.begin(), BE = BB.end(); BI != BE; ++BI)
    {
        DebugLoc loc = BI->getDebugLoc();
        if (!loc)
            continue;
        auto scope = loc.getScope();
        if (llvm::isa<llvm::DILexicalBlock>(scope) || 
            llvm::isa<DISubprogram>(scope))
        {
            scope_out = llvm::cast<llvm::DIScope>(scope);
            loc_out = loc;
            return true;
        }
    }
    return false;
}

llvm::DIType* ImplicitGlobalId::getOrCreateUlongDIType()
{
    auto it = m_DbgInfoFinder.types();
    for (auto t_i = it.begin();
        t_i != it.end(); ++t_i)
    {
        auto type = (*t_i);
        if (type &&
            type->getName() == "long unsigned int")
        {
            return type;
        }
    }
    // If the type wasn't found, create it now
    return m_pDIB->createBasicType("long unsigned int", 64, dwarf::DW_ATE_unsigned);
}

Value* ImplicitGlobalId::CreateGetId(unsigned dim, IRBuilder<> &B, GlobalOrLocal wi)
{
    const char dimChr = '0' + dim;
    const std::string nameCall = std::string("globalId") + dimChr;
    std::string nameFunc;
    if (wi == GlobalOrLocal::Global)
    {
        nameFunc = "_Z13get_global_idj";
    }
    else if (wi == GlobalOrLocal::Local)
    {
        nameFunc = "_Z12get_local_idj";
    }
    else if (wi == GlobalOrLocal::WorkItem)
    {
        nameFunc = "_Z12get_group_idj";
    }
    Type* uint32_type = IntegerType::get(m_pModule->getContext(), 32);

    Function* getFunc = m_pModule->getFunction(nameFunc);

    if (!getFunc)
    {
        //Create one
        // Create parameters and return values
        Type *pResult = IntegerType::get(m_pModule->getContext(), m_uiSizeT);
        std::vector<Type*> funcTyArgs;
        funcTyArgs.push_back(uint32_type);

        // Create function declaration
        FunctionType *pFuncTy = FunctionType::get(pResult, funcTyArgs, false);
        assert(pFuncTy && "Failed to create new function type");
        Function *pNewFunc = Function::Create(pFuncTy, GlobalValue::ExternalLinkage, nameFunc, m_pModule);
        assert(pNewFunc && "Failed to create new function declaration");

        // Set function attributes
		IGCLLVM::AttributeSet funcAttrs;
        AttrBuilder attBuilder;
        attBuilder.addAttribute(Attribute::NoUnwind).addAttribute(Attribute::ReadNone);
        funcAttrs = IGCLLVM::AttributeSet::get(pNewFunc->getContext(), IGCLLVM::AttributeSet::FunctionIndex, attBuilder);
        pNewFunc->setAttributes(funcAttrs);

        getFunc = pNewFunc;
    }
    // Create arguments and call instruction
    Value* const_dim = ConstantInt::get(uint32_type, dim, false);
    return B.CreateCall(getFunc, const_dim, nameCall);
}
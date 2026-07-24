/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AddImplicitArgs.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "LLVM3DBuilder/MetadataBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SCCIterator.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DerivedTypes.h>
#include "llvm/IR/DIBuilder.h"
#include <llvm/ADT/DepthFirstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/DIBuilder.h"
#include "llvmWrapper/IR/DebugInfo.h"
#include <llvmWrapper/ADT/STLExtras.h>
#include <llvmWrapper/IR/Instructions.h>

#include "common/debug/Debug.hpp"
#include "DebugInfo/VISADebugEmitter.hpp"
#include "DebugInfo/DbgVariableTypes.hpp"
#include <map>
#include <utility>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-add-implicit-args"
#define PASS_DESCRIPTION "Add implicit args to all functions in the module and adjusts call to these functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AddImplicitArgsLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(AddImplicitArgsLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

struct ImplicitStructArgument {
  union {
    struct {
      unsigned int argId : 11;
      unsigned int argOffset : 11;
      unsigned int argExplicitNum : 10;
    } All;
    unsigned int Value;
  } DW0;
};

char AddImplicitArgsLPM::ID = 0;

AddImplicitArgsLPM::AddImplicitArgsLPM() : ModulePass(ID) {
  initializeAddImplicitArgsLPMPass(*PassRegistry::getPassRegistry());
}

bool AddImplicitArgs::run(Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx) {
  MapList<Function *, Function *> funcsMapping;
  MapList<Function *, Function *> funcsMappingForReplacement;
  m_pMdUtils = pMdUtils;
  m_ctx = pCtx;
  CodeGenContext *ctx = m_ctx;

  // Update function signatures
  // Create new functions with implicit args
  for (Function &func : M) {
    // Only handle functions defined in this module
    if (func.isDeclaration())
      continue;
    // skip non-entry functions
    if (m_pMdUtils->findFunctionsInfoItem(&func) == m_pMdUtils->end_FunctionsInfo())
      continue;
    // Skip functions called from functions marked with stackcall attribute
    // Also skip the dummy kernel if one was created
    if (hasStackCallInCG(&func) || IGC::isIntelSymbolTableVoidProgram(&func)) {
      ctx->getModuleMetaData()->FuncMD[&func].implicitArgInfoList.clear();
      continue;
    }

    // see the detail in StatelessToStateful.cpp.
    // If SToSProducesPositivePointer is true, do not generate implicit arguments.
    if (IGC_IS_FLAG_DISABLED(SToSProducesPositivePointer) &&
        (ctx->getModuleMetaData()->compOpt.HasBufferOffsetArg || IGC_IS_FLAG_ENABLED(EnableSupportBufferOffset))) {
      ImplicitArgs::addBufferOffsetArgs(func, m_pMdUtils, ctx->getModuleMetaData());
    }

    if (ctx->getModuleMetaData()->compOpt.UseBindlessMode && !ctx->getModuleMetaData()->compOpt.UseLegacyBindlessMode) {
      ImplicitArgs::addBindlessOffsetArgs(func, m_pMdUtils, ctx->getModuleMetaData());
    }

    ImplicitArgs implicitArgs(func, m_pMdUtils, ctx->getModuleMetaData());

    // Create the new function body and insert it into the module
    FunctionType *pNewFTy = getNewFuncType(&func, implicitArgs);
    Function *pNewFunc = Function::Create(pNewFTy, func.getLinkage());
    pNewFunc->copyAttributesFrom(&func);
    pNewFunc->setSubprogram(func.getSubprogram());
    M.getFunctionList().insert(func.getIterator(), pNewFunc);
    pNewFunc->takeName(&func);

    // Since we have now created the new function, splice the body of the old
    // function right into the new function, leaving the old body of the function empty.
    IGCLLVM::splice(pNewFunc, pNewFunc->begin(), &func);

    // Loop over the argument list, transferring uses of the old arguments over to
    // the new arguments
    updateNewFuncArgs(&func, pNewFunc, implicitArgs);

    // Map old func to new func
    funcsMapping[&func] = pNewFunc;

    if (!func.use_empty()) {
      funcsMappingForReplacement[&func] = pNewFunc;
    }
  }

  if (!IGC::ForceAlwaysInline(ctx)) {
    for (const auto &I : funcsMappingForReplacement) {
      replaceAllUsesWithNewOCLBuiltinFunction(I.first, I.second);
    }
  }

  // Update IGC Metadata
  // Function declarations are changing, this needs to be reflected in the metadata.
  for (const auto &i : funcsMapping) {
    IGCMD::IGCMetaDataHelper::moveFunction(*m_pMdUtils, *ctx->getModuleMetaData(), i.first, i.second);

    MetadataBuilder mbuilder(&M);
    mbuilder.UpdateShadingRate(i.first, i.second);
  }
  // Update LLVM metadata based on IGC MetadataUtils
  m_pMdUtils->save(M.getContext());

  // Return if any error
  if (m_ctx->HasError()) {
    return false;
  }
  // Go over all changed functions
  for (const auto &I : funcsMapping) {
    Function *pFunc = I.first;

    IGC_ASSERT(nullptr != pFunc);
    IGC_ASSERT_MESSAGE(pFunc->use_empty(), "Assume all user function are inlined at this point");

    // Now, after changing function signature
    // and validating there are no calls to the old function, we can erase it.
    pFunc->eraseFromParent();
  }

  return true;
}

bool AddImplicitArgs::hasStackCallInCG(const Function *F) {
  if (F->hasFnAttribute("visaStackCall") || F->hasFnAttribute("referenced-indirectly"))
    return true;

  for (auto u = F->user_begin(), e = F->user_end(); u != e; u++) {
    if (const CallInst *call = dyn_cast<CallInst>(*u)) {
      const Function *parent = call->getParent()->getParent();
      if (parent != F && hasStackCallInCG(parent))
        return true;
    }
  }
  return false;
}

FunctionType *AddImplicitArgs::getNewFuncType(const Function *pFunc, const ImplicitArgs &implicitArgs) {
  // Add all explicit parameters
  const FunctionType *pFuncType = pFunc->getFunctionType();
  std::vector<Type *> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());

  // Add implicit arguments parameter types
  for (unsigned int i = 0; i < implicitArgs.size(); ++i) {
    newParamTypes.push_back(implicitArgs[i].getLLVMType(pFunc->getContext()));
  }

  // Create new function type with explicit and implicit parameter types
  return FunctionType::get(pFunc->getReturnType(), newParamTypes, pFunc->isVarArg());
}

void AddImplicitArgs::updateNewFuncArgs(llvm::Function *pFunc, llvm::Function *pNewFunc,
                                        const ImplicitArgs &implicitArgs) {
  // Loop over the argument list, transferring uses of the old arguments over to
  // the new arguments, also transferring over the names as well.
  std::vector<std::pair<DbgVarInstEntry *, unsigned int>> newAddr;
  bool fullDebugInfo = false;
  bool lineNumbersOnly = false;
  CodeGenContext *ctx = m_ctx;
  DebugMetadataInfo::hasAnyDebugInfo(ctx, fullDebugInfo, lineNumbersOnly);

  if (fullDebugInfo) {
    // For each argument of the old function, pFunc, find the dbg.declare
    // entries (intrinsics on LLVM <22, debug records on >=22) that reference it
    // as their address operand and remember the argument's position so we can
    // fix them later. Discovery goes through findDbgDeclareUses because debug
    // records are not part of the instruction stream.
    unsigned int i = 0;
    for (auto arg = pFunc->arg_begin(); arg != pFunc->arg_end(); ++arg, ++i) {
      for (auto *DDI : IGCLLVM::findDbgDeclareUses(&(*arg)))
        newAddr.push_back(std::make_pair(DDI, i));
    }
  }

  Function::arg_iterator currArg = pNewFunc->arg_begin();
  for (Function::arg_iterator I = pFunc->arg_begin(), E = pFunc->arg_end(); I != E; ++I, ++currArg) {
    llvm::Value *newArg = coerce(&(*currArg), I->getType(), &pNewFunc->getEntryBlock().front());
    // Move the name and users over to the new version.
    I->replaceAllUsesWith(newArg);
    currArg->takeName(&(*I));
  }

  // In the following loop, fix dbg.declare nodes that reference function arguments.
  // This occurs for example when a struct type is passed as a kernel parameter
  // byval. Bug#GD-429 had this exact issue. If we don't do this then we lose
  // mapping of argument to dbg.declare and elf file comes up with empty
  // storage location for the variable.
  for (const auto &toReplace : newAddr) {
    auto *d = toReplace.first;

    IGCLLVM::DIBuilder Builder(*pNewFunc->getParent());
    auto DIVar = d->getVariable();
    auto DIExpr = d->getExpression();

    IGC_ASSERT(toReplace.second < pNewFunc->arg_size());
    Value *v = pNewFunc->arg_begin() + toReplace.second;
    Builder.insertDeclare(v, DIVar, DIExpr, d->getDebugLoc().get(), IGC::dbgVarAnchorInstMutable(d));
    d->eraseFromParent();
  }

  // Set implicit argument names
  InfoToImpArgMap &infoToArg = m_FuncInfoToImpArgMap[pNewFunc];
  ImpArgToExpNum &argImpToExpNum = m_FuncImpToExpNumMap[pNewFunc];
  for (unsigned int i = 0; i < implicitArgs.size(); ++i, ++currArg) {
    currArg->setName(implicitArgs[i].getName());

    ImplicitArg::ArgType argId = implicitArgs[i].getArgType();
    ImplicitStructArgument info;
    info.DW0.All.argId = argId;
    info.DW0.All.argExplicitNum = 0;
    info.DW0.All.argOffset = 0;
    if ((argId >= ImplicitArg::ArgType::STRUCT_START && argId <= ImplicitArg::IMAGES_END) ||
        argId == ImplicitArg::ArgType::GET_OBJECT_ID || argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE) {
      // struct, image
      const auto &argInfo = m_ctx->getModuleMetaData()->FuncMD[pFunc].implicitArgInfoList[i];
      IGC_ASSERT_MESSAGE(argInfo.explicitArgNum != -1, "wrong data in MetaData");

      argImpToExpNum[&(*currArg)] = info.DW0.All.argExplicitNum = argInfo.explicitArgNum;
      if (argId <= ImplicitArg::ArgType::STRUCT_END) {
        IGC_ASSERT_MESSAGE(argInfo.structArgOffset != -1, "wrong data in MetaData");
        info.DW0.All.argOffset = argInfo.structArgOffset;
      }
    }
    infoToArg[info.DW0.Value] = &(*currArg);
  }
}

void AddImplicitArgs::replaceAllUsesWithNewOCLBuiltinFunction(llvm::Function *old_func, llvm::Function *new_func) {
  IGC_ASSERT(!old_func->use_empty());

  auto &subFuncImplicitArgs = m_ctx->getModuleMetaData()->FuncMD[old_func].implicitArgInfoList;

  std::vector<Instruction *> list_delete;
  old_func->removeDeadConstantUsers();
  std::vector<Value *> functionUserList(old_func->user_begin(), old_func->user_end());
  for (auto U : functionUserList) {
    CallInst *cInst = dyn_cast<CallInst>(U);
    auto BC = dyn_cast<BitCastInst>(U);
    if (BC && BC->hasOneUse())
      cInst = dyn_cast<CallInst>(BC->user_back());

    if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer)) {
      if (!cInst || IGCLLVM::getCalledValue(cInst) != old_func) {
        // Support indirect function pointer usages
        if (Instruction *userInst = dyn_cast<Instruction>(U)) {
          IRBuilder<> builder(userInst);
          Value *fncast = builder.CreateBitCast(new_func, old_func->getType());
          userInst->replaceUsesOfWith(old_func, fncast);
          continue;
        } else if (ConstantExpr *OldExpr = dyn_cast<ConstantExpr>(U)) {
          Constant *fncast = ConstantExpr::getBitCast(new_func, old_func->getType());
          SmallVector<Constant *, 8> NewOps;
          for (auto Op : OldExpr->operand_values()) {
            NewOps.push_back(Op == old_func ? fncast : cast<Constant>(Op));
          }
          auto NewExpr = OldExpr->getWithOperands(NewOps);
          OldExpr->replaceAllUsesWith(NewExpr);
          OldExpr->destroyConstant();
          continue;
        }
      }
    }

    if (!cInst) {
      IGC_ASSERT_MESSAGE(0, "Unknown function usage");
      m_ctx->EmitError(" undefined reference to `jmp()' ", U);
      return;
    }
    // Return if any error
    if (m_ctx->HasError()) {
      return;
    }

    std::vector<Value *> new_args;
    Function *parent_func = cInst->getParent()->getParent();
    size_t numArgOperands = IGCLLVM::getNumArgOperands(cInst);

    // let's prepare argument list on new call function
    llvm::Function::arg_iterator new_arg_iter = new_func->arg_begin();
    llvm::Function::arg_iterator new_arg_end = new_func->arg_end();

    IGC_ASSERT(new_func->arg_size() >= numArgOperands);

    // basic arguments
    for (unsigned int i = 0; i < numArgOperands; ++i, ++new_arg_iter) {
      llvm::Value *arg = coerce(cInst->getOperand(i), new_arg_iter->getType(), cInst);
      new_args.push_back(arg);
    }

    // implicit arguments
    int cImpCount = 0;
    InfoToImpArgMap &infoToArg = m_FuncInfoToImpArgMap[parent_func];
    ImpArgToExpNum &argImpToExpNum = m_FuncImpToExpNumMap[new_func];
    while (new_arg_iter != new_arg_end) {
      ImplicitArg::ArgType argId = static_cast<ImplicitArg::ArgType>(subFuncImplicitArgs[cImpCount].argId);

      ImplicitStructArgument info;
      info.DW0.All.argId = argId;
      info.DW0.All.argExplicitNum = 0;
      info.DW0.All.argOffset = 0;

      if (argId < ImplicitArg::ArgType::STRUCT_START) {
        IGC_ASSERT_MESSAGE(infoToArg.find(info.DW0.Value) != infoToArg.end(),
                           "Can't find the implicit argument on parent function");
        new_args.push_back(infoToArg[info.DW0.Value]);
      } else if (argId <= ImplicitArg::ArgType::STRUCT_END) {
        IGC_ASSERT_MESSAGE(0, "wrong argument type in user function");
      } else if (argId <= ImplicitArg::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID ||
                 argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE) {
        // special handling for image info types, such as ImageWidth, ImageHeight, ...
        // and struct type

        IGC_ASSERT_MESSAGE(argImpToExpNum.find(&(*new_arg_iter)) != argImpToExpNum.end(),
                           "Can't find explicit argument number");

        // tracing it on parent function argument list
        Value *callArg = ValueTracker::track(cInst, argImpToExpNum[&(*new_arg_iter)]);
        Argument *arg = cast<Argument>(callArg);

        // build info

        info.DW0.All.argExplicitNum = arg->getArgNo();
        IGC_ASSERT_MESSAGE(infoToArg.find(info.DW0.Value) != infoToArg.end(),
                           "Can't find the implicit argument on parent function");

        new_args.push_back(infoToArg[info.DW0.Value]);
      } else {
        IGC_ASSERT_MESSAGE(infoToArg.find(info.DW0.Value) != infoToArg.end(),
                           "Can't find the implicit argument on parent function");
        new_args.push_back(infoToArg[info.DW0.Value]);
      }
      ++new_arg_iter;
      ++cImpCount;
    }

    // insert new call instruction before old one
    llvm::CallInst *inst;
    if (new_func->getReturnType()->isVoidTy()) {
      inst = CallInst::Create(new_func, new_args, "", IGCLLVM::insertPosition(cInst));
    } else {
      inst = CallInst::Create(new_func, new_args, new_func->getName(), IGCLLVM::insertPosition(cInst));
    }
    inst->setCallingConv(new_func->getCallingConv());
    inst->setDebugLoc(cInst->getDebugLoc());
    cInst->replaceAllUsesWith(inst);
    list_delete.push_back(cInst);
  }

  for (auto i : list_delete) {
    i->eraseFromParent();
  }
}

llvm::Value *AddImplicitArgs::coerce(llvm::Value *arg, llvm::Type *type, llvm::Instruction *insertBefore) {
  if (arg->getType() == type)
    return arg;

  // fix opaque type mismatch on %opencl...
  std::string str0;
  llvm::raw_string_ostream s(str0);
  arg->getType()->print(s);
  return new llvm::BitCastInst(arg, type, "", IGCLLVM::insertPosition(insertBefore));
}

// Builtin CallGraph Analysis
#define PASS_FLAG2 "igc-callgraphscc-analysis"
#define PASS_DESCRIPTION2 "Analyzes CallGraphSCC"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(BuiltinCallGraphAnalysisLPM, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(BuiltinCallGraphAnalysisLPM, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char BuiltinCallGraphAnalysisLPM::ID = 0;

BuiltinCallGraphAnalysisLPM::BuiltinCallGraphAnalysisLPM() : ModulePass(ID) {
  initializeBuiltinCallGraphAnalysisLPMPass(*PassRegistry::getPassRegistry());
}

bool BuiltinCallGraphAnalysis::run(Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx,
                                   CallGraph &CG) {
  m_pMdUtils = pMdUtils;
  m_ctx = pCtx;

  if (IGC::ForceAlwaysInline(pCtx)) {
    return false;
  }

  // Traverse each SCC
  for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I) {
    traverseCallGraphSCC(*I);
  }
  m_pMdUtils->save(M.getContext());

  // Detect stack calls that use implicit args, and force inline them, since they are not supported
  if (IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg)) {
    pruneCallGraphForStackCalls(CG);
  }

  return false;
}

// Do a DFS in the call graph, and for any stack-call func that has implicit args,
// force inline on it and all callers along the DFS path.
bool BuiltinCallGraphAnalysis::pruneCallGraphForStackCalls(CallGraph &CG) {
  bool changed = false;
  llvm::SmallSet<Function *, 16> PrunedFuncs;
  for (auto IT = df_begin(&CG), EI = df_end(&CG); IT != EI; IT++) {
    Function *F = IT->getFunction();
    if (F && !F->isDeclaration() && F->hasFnAttribute("visaStackCall")) {
      if (!m_ctx->getModuleMetaData()->FuncMD[F].implicitArgInfoList.empty()) {
        for (unsigned i = 0; i < IT.getPathLength(); i++) {
          Function *pFuncOnPath = IT.getPath(i)->getFunction();
          if (pFuncOnPath && !isEntryFunc(m_ctx->getModuleMetaData(), pFuncOnPath)) {
            if (pFuncOnPath->hasFnAttribute("hasRecursion")) {
              IGC_ASSERT_MESSAGE(0, "Cannot inline for recursion!");
              return false;
            }
            PrunedFuncs.insert(pFuncOnPath);
          }
        }
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(PrintStackCallDebugInfo) && !PrunedFuncs.empty())
    dbgs() << "\nForce Inlined Functions via ForceInlineStackCallWithImplArg:\n";

  for (auto pF : PrunedFuncs) {
    // We can only remove the "visaStackCall" attribute if the function isn't called indirectly,
    // since these attributes are always coupled together.
    if (pF->hasFnAttribute("referenced-indirectly")) {
      CodeGenContext *pCtx = m_ctx;
      bool emitError = false;
      if (pCtx->type == ShaderType::OPENCL_SHADER) {
        // If this option is passed, emit error when extern functions use implicit arg buffer
        auto ClContext = static_cast<OpenCLProgramContext *>(pCtx);
        emitError = ClContext->m_Options.EmitErrorsForLibCompilation;
      }
      if (IGC_IS_FLAG_DISABLED(EnableGlobalStateBuffer) && emitError) {
        IGC_ASSERT_MESSAGE(
            0, "Cannot force inline indirect calls! Requires IA Buffer support, i.e. EnableGlobalStateBuffer = 1");
        m_ctx->EmitError("Exported functions does not support implicit arguments", pF);
      }
      continue;
    }

    pF->removeFnAttr("visaStackCall");
    pF->removeFnAttr(llvm::Attribute::NoInline);
    pF->addFnAttr(llvm::Attribute::AlwaysInline);

    if (IGC_IS_FLAG_ENABLED(PrintStackCallDebugInfo))
      dbgs() << pF->getName().str() << "\n";
    changed = true;
  }
  return changed;
}

void BuiltinCallGraphAnalysis::traverseCallGraphSCC(const std::vector<CallGraphNode *> &SCCNodes) {
  // all functions in one scc should end up with the same result
  ImplicitArgumentDetail *argData = nullptr;
  for (auto CGN : SCCNodes) {
    Function *f = CGN->getFunction();
    if (!f || f->isDeclaration())
      continue;
    // Fail on variadic functions.
    if (f->isVarArg()) {
      std::string Msg = "Invalid user defined function being processed: ";
      Msg += f->getName();
      Msg += "()\n";
      m_ctx->EmitError(Msg.c_str(), f);
      return;
    }
    if (argData == nullptr) {
      argDetails.push_back(IGCLLVM::make_unique<ImplicitArgumentDetail>());
      argData = argDetails[argDetails.size() - 1].get();
    }

    // calculate args from sub-routine.
    // This function have not been processed yet, therefore no map-entry for it yet
    IGC_ASSERT(argMap.count(f) == 0);
    for (const auto &N : (*CGN)) {
      Function *sub = N.second->getFunction();
      // if callee has not been visited
      auto argMapIter = argMap.find(sub);
      // if we have processed the arguments for callee
      if (argMapIter != argMap.end()) {
        IGC_ASSERT(nullptr != argMapIter->second);
        combineTwoArgDetail(*argData, *(argMapIter->second), *N.first);
      }
    }
  }

  for (auto CGN : SCCNodes) {
    Function *f = CGN->getFunction();
    if (!f || f->isDeclaration())
      continue;
    auto &implList = m_ctx->getModuleMetaData()->FuncMD[f].implicitArgInfoList;
    // calculate implicit args from function metadata

    // build everything
    for (const auto &argInfo : implList) {
      ImplicitArg::ArgType argId = static_cast<ImplicitArg::ArgType>(argInfo.argId);

      if (argId < ImplicitArg::ArgType::STRUCT_START) {
        // unique implicit argument
        // if doesn't exist, the following line will add one.
        ImplicitArg::ArgValSet &setx = argData->ArgsMaps[argId];
        setx.insert(0);
      } else if (argId <= ImplicitArg::ArgType::STRUCT_END) {
        // aggregate implicity argument

        ImplicitStructArgument info;
        info.DW0.All.argExplicitNum = argInfo.explicitArgNum;
        info.DW0.All.argOffset = argInfo.structArgOffset;
        info.DW0.All.argId = argId;
        argData->StructArgSet.insert(info.DW0.Value);
      } else if (argId <= ImplicitArg::ArgType::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID ||
                 argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE) {
        // image index, project id

        int argNum = argInfo.explicitArgNum;
        ImplicitArg::ArgValSet &setx = argData->ArgsMaps[argId];
        setx.insert(argNum);
      } else {
        // unique implicit argument
        // if doesn't exist, the following line will add one.
        ImplicitArg::ArgValSet &setx = argData->ArgsMaps[argId];
        setx.insert(0);
      }
    }
  }
  for (auto CGN : SCCNodes) {
    Function *f = CGN->getFunction();
    if (!f || f->isDeclaration())
      continue;
    // write everything back into metaData
    writeBackAllIntoMetaData(*argData, f);
    argMap.insert(std::make_pair(f, argData));
  }
}

void BuiltinCallGraphAnalysis::combineTwoArgDetail(ImplicitArgumentDetail &retD, const ImplicitArgumentDetail &argD,
                                                   llvm::Value *v) const {
  for (const auto &argPair : argD.ArgsMaps) {
    ImplicitArg::ArgType argId = argPair.first;
    if (argId < ImplicitArg::ArgType::STRUCT_START) {
      // unique implicit argument
      // if doesn't exist, the following line will add one.
      ImplicitArg::ArgValSet &setx = retD.ArgsMaps[argId];
      setx.insert(0);
    } else if (argId <= ImplicitArg::ArgType::STRUCT_END) {
      // aggregate implicit argument
      IGC_ASSERT_MESSAGE(0, "wrong location for this kind of argument type");

    } else if (argId <= ImplicitArg::ArgType::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID ||
               argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE) {
      // image index

      CallInst *cInst = dyn_cast<CallInst>(v);
      if (!cInst) {
        IGC_ASSERT_MESSAGE(0, " Not supported");
        m_ctx->EmitError(" undefined reference to `jmp()' ", v);
        return;
      }

      ImplicitArg::ArgValSet &setx = retD.ArgsMaps[argId];

      // loop all image arguments on the sub-funtion.
      for (const auto &argI : argPair.second) {
        // find it from calling instruction, and trace it back to parent function argument
        Value *callArg = ValueTracker::track(cInst, argI);
        const Argument *arg = dyn_cast<Argument>(callArg);
        IGC_ASSERT_MESSAGE(nullptr != arg, "Not supported");
        setx.insert(arg->getArgNo());
      }
    } else {
      // unique implicit argument
      // if doesn't exist, the following line will add one.
      ImplicitArg::ArgValSet &setx = retD.ArgsMaps[argId];
      setx.insert(0);
    }
  }

  IGC_ASSERT_MESSAGE(0 == argD.StructArgSet.size(), "wrong argument type in user function");
}

void BuiltinCallGraphAnalysis::writeBackAllIntoMetaData(const ImplicitArgumentDetail &data, Function *f) {
  CodeGenContext *pCtx = m_ctx;
  auto &argList = pCtx->getModuleMetaData()->FuncMD[f].implicitArgInfoList;
  argList.clear();

  bool isEntry = isEntryFunc(pCtx->getModuleMetaData(), f);

  // Check if DP emulation is used and the function uses DP operations. Emulation needs r0 and private_base
  // implicit args, so these args have to exist. r0 and private_base args are adding by analysis passes.
  bool needImplicitArgs = pCtx->type == ShaderType::OPENCL_SHADER && pCtx->m_hasDPEmu;

  for (const auto &A : data.ArgsMaps) {
    // For the implicit args that have GenISAIntrinsic support used in subroutines,
    // they do not require to be added as explicit arguments other than in the caller kernel.
    // Always add metadata for stackcalls to provide info for inlining. They won't be added
    // to function argument list.
    ImplicitArg::ArgType argId = A.first;
    if (!isEntry && ImplicitArgs::hasIntrinsicSupport(argId) && !needImplicitArgs) {
      bool isStackCall = f->hasFnAttribute("visaStackCall");
      if (!isStackCall && IGC_IS_FLAG_ENABLED(EnableImplicitArgAsIntrinsic)) {
        continue;
      }
    }
    if (argId < ImplicitArg::ArgType::STRUCT_START) {
      // unique implicit argument, add it on metadata
      ArgInfoMD argMD;
      argMD.argId = argId;
      argList.push_back(argMD);
    } else if (argId <= ImplicitArg::ArgType::STRUCT_END) {
      // aggregate implicity argument

      IGC_ASSERT_MESSAGE(0, "wrong location for this kind of argument type");

    } else if (argId <= ImplicitArg::ArgType::IMAGES_END || argId == ImplicitArg::ArgType::GET_OBJECT_ID ||
               argId == ImplicitArg::ArgType::GET_BLOCK_SIMD_SIZE) {
      // image index

      for (const auto &vOnSet : A.second) {
        ArgInfoMD argMD;
        argMD.argId = argId;
        argMD.explicitArgNum = vOnSet;
        argList.push_back(argMD);
      }
    } else {
      // unique implicit argument

      ArgInfoMD argMD;
      argMD.argId = argId;
      argList.push_back(argMD);
    }
  }

  for (const auto N : data.StructArgSet) // argument number
  {
    ArgInfoMD argMD;
    ImplicitStructArgument info;
    info.DW0.Value = N;

    argMD.explicitArgNum = info.DW0.All.argExplicitNum;
    argMD.structArgOffset = info.DW0.All.argOffset; // offset
    argMD.argId = info.DW0.All.argId;               // type
    argList.push_back(argMD);
  }
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses AddImplicitArgsNPM::run(Module &M, ModuleAnalysisManager &AM) {
  AddImplicitArgs impl;
  bool changed =
      impl.run(M, AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils, AM.getResult<CodeGenContextAnalysis>(M).Ctx);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PreservedAnalyses BuiltinCallGraphAnalysisNPM::run(Module &M, ModuleAnalysisManager &AM) {
  BuiltinCallGraphAnalysis impl;
  bool changed = impl.run(M, AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils,
                          AM.getResult<CodeGenContextAnalysis>(M).Ctx, AM.getResult<CallGraphAnalysis>(M));
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

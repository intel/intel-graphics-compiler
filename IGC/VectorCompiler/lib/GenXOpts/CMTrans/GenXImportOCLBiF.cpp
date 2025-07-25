/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXImportOCLBiF
/// -----------
///
/// This pass import Builtin Function library compiled into bitcode
///
/// - analysis functions called by the main module
///
/// - import used function, and remove unused functions
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cmimportbif"

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/General/BiF.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "Probe/Assertion.h"
#include "llvm/GenXIntrinsics/GenXIntrinsicInst.h"
#include <BiFManager/BiFManagerHandler.hpp>

using namespace llvm;

class BIConvert {
  // builtins that maps to one intrinsic
  std::map<StringRef, unsigned> OneMap;
  // builtins that maps to two intrinsics
  std::map<StringRef, std::pair<unsigned, unsigned>> TwoMap;

public:
  BIConvert();
  void runOnModule(Module &M);
};

BIConvert::BIConvert() {
  // float-to-float
  OneMap["__builtin_IB_frnd_ne"] = GenXIntrinsic::genx_rnde;
  OneMap["__builtin_IB_ftoh_rtn"] = GenXIntrinsic::genx_rndd;
  OneMap["__builtin_IB_ftoh_rtp"] = GenXIntrinsic::genx_rndu;
  OneMap["__builtin_IB_ftoh_rtz"] = GenXIntrinsic::genx_rndz;
  OneMap["__builtin_IB_dtoh_rtn"] = GenXIntrinsic::genx_rnde;
  OneMap["__builtin_IB_dtoh_rtp"] = GenXIntrinsic::genx_rndu;
  OneMap["__builtin_IB_dtoh_rtz"] = GenXIntrinsic::genx_rndz;
  OneMap["__builtin_IB_dtof_rtn"] = GenXIntrinsic::genx_rnde;
  OneMap["__builtin_IB_dtof_rtp"] = GenXIntrinsic::genx_rndu;
  OneMap["__builtin_IB_dtof_rtz"] = GenXIntrinsic::genx_rndz;
  // math
  OneMap["__builtin_IB_frnd_pi"] = GenXIntrinsic::genx_rndu;
  OneMap["__builtin_IB_frnd_ni"] = GenXIntrinsic::genx_rndd;
  OneMap["__builtin_IB_frnd_zi"] = GenXIntrinsic::genx_rndz;
  OneMap["__builtin_IB_native_cosf"] = Intrinsic::cos;
  OneMap["__builtin_IB_native_cosh"] = Intrinsic::cos;
  OneMap["__builtin_IB_native_sinf"] = Intrinsic::sin;
  OneMap["__builtin_IB_native_sinh"] = Intrinsic::sin;
  OneMap["__builtin_IB_native_exp2f"] = Intrinsic::exp2;
  OneMap["__builtin_IB_native_exp2h"] = Intrinsic::exp2;
  OneMap["__builtin_IB_native_log2f"] = Intrinsic::log2;
  OneMap["__builtin_IB_native_log2h"] = Intrinsic::log2;
  OneMap["__builtin_IB_native_sqrtf"] = Intrinsic::sqrt;
  OneMap["__builtin_IB_native_sqrth"] = Intrinsic::sqrt;
  OneMap["__builtin_IB_native_sqrtd"] = Intrinsic::sqrt;
  OneMap["__builtin_IB_popcount_1u32"] = GenXIntrinsic::genx_cbit;
  OneMap["__builtin_IB_popcount_1u16"] = GenXIntrinsic::genx_cbit;
  OneMap["__builtin_IB_popcount_1u8"] = GenXIntrinsic::genx_cbit;
  OneMap["__builtin_IB_native_powrf"] = Intrinsic::pow;
  OneMap["__builtin_IB_fma"] = Intrinsic::fma;
  OneMap["__builtin_IB_fmah"] = Intrinsic::fma;
  OneMap["__builtin_IB_bfrev"] = GenXIntrinsic::genx_bfrev;
  OneMap["__builtin_IB_fmax"] = Intrinsic::maxnum;
  OneMap["__builtin_IB_fmin"] = Intrinsic::minnum;
  OneMap["__builtin_IB_HMAX"] = Intrinsic::maxnum;
  OneMap["__builtin_IB_HMIN"] = Intrinsic::minnum;
  OneMap["__builtin_IB_dmax"] = Intrinsic::maxnum;
  OneMap["__builtin_IB_dmin"] = Intrinsic::minnum;
  // ieee
  OneMap["__builtin_IB_ieee_sqrt"] = GenXIntrinsic::genx_ieee_sqrt;
  OneMap["__builtin_IB_ieee_divide"] = GenXIntrinsic::genx_ieee_div;
  OneMap["__builtin_IB_ieee_divide_f64"] = GenXIntrinsic::genx_ieee_div;

  TwoMap["__builtin_IB_dtoi8_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi8_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi8_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi16_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi16_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi16_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi32_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi32_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi32_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi64_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi64_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptosi_sat);
  TwoMap["__builtin_IB_dtoi64_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptosi_sat);

  TwoMap["__builtin_IB_dtoui8_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui8_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui8_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui16_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui16_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui16_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui32_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui32_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui32_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui64_rtn"] =
      std::make_pair(GenXIntrinsic::genx_rndd, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui64_rtp"] =
      std::make_pair(GenXIntrinsic::genx_rndu, GenXIntrinsic::genx_fptoui_sat);
  TwoMap["__builtin_IB_dtoui64_rte"] =
      std::make_pair(GenXIntrinsic::genx_rnde, GenXIntrinsic::genx_fptoui_sat);

  TwoMap["__builtin_IB_fma_rtz_f64"] =
      std::make_pair(Intrinsic::fma, GenXIntrinsic::genx_rndz);
  TwoMap["__builtin_IB_fma_rtz_f32"] =
      std::make_pair(Intrinsic::fma, GenXIntrinsic::genx_rndz);
  TwoMap["__builtin_IB_fma_rtp_f64"] =
      std::make_pair(Intrinsic::fma, GenXIntrinsic::genx_rndu);
  TwoMap["__builtin_IB_fma_rtn_f64"] =
      std::make_pair(Intrinsic::fma, GenXIntrinsic::genx_rndd);
}

// Get intrinsic declaration for given base call instruction and intrinsic ID.
// Specialized for OneMap IID usage -- in case IID is not genx intrinsic id,
// assume that this is intrinsic with only one overloaded type (fma).
static Function *getOneMapIntrinsicDeclaration(CallInst &CI, const unsigned IID,
                                               Module &M) {
  if (GenXIntrinsic::isGenXIntrinsic(IID))
    return vc::getGenXDeclarationForIdFromArgs(
        CI.getType(), CI.args(), static_cast<GenXIntrinsic::ID>(IID), M);

  return Intrinsic::getDeclaration(&M, static_cast<Intrinsic::ID>(IID),
                                   {CI.getType()});
}

void BIConvert::runOnModule(Module &M) {
  std::vector<Instruction *> ListDelete;
  for (Function &func : M) {
    for (auto &BB : func) {
      for (auto I = BB.begin(), E = BB.end(); I != E; I++) {
        CallInst *InstCall = dyn_cast<CallInst>(I);
        if (!InstCall)
          continue;
        Function *callee = InstCall->getCalledFunction();
        if (!callee)
          continue;
        // get rid of lifetime marker, avoid dealing with it in packetizer
        Intrinsic::ID id = (Intrinsic::ID)callee->getIntrinsicID();
        if (id == Intrinsic::lifetime_start || id == Intrinsic::lifetime_end) {
          ListDelete.push_back(InstCall);
          continue;
        }

        StringRef CalleeName = callee->getName();
        // Check if it exists in the one-intrinsic map.
        if (OneMap.count(CalleeName)) {
          const unsigned IID = OneMap[CalleeName];
          Function *const IntrinFunc =
              getOneMapIntrinsicDeclaration(*InstCall, IID, M);
          const SmallVector<llvm::Value *, 3> Args{InstCall->args()};
          Instruction *const IntrinCall =
              CallInst::Create(IntrinFunc, Args, InstCall->getName(), InstCall);
          if (!GenXIntrinsic::isGenXIntrinsic(IID)) {
            switch (IID) {
            default:
              IntrinCall->setHasApproxFunc(true);
              break;
            case Intrinsic::fma:
            case Intrinsic::maxnum:
            case Intrinsic::minnum:
              break;
            }
          }
          IntrinCall->setDebugLoc(InstCall->getDebugLoc());
          InstCall->replaceAllUsesWith(IntrinCall);
          ListDelete.push_back(InstCall);
        }
        // check if the builtin maps to two intrinsics
        else if (TwoMap.count(CalleeName)) {
          auto pair = TwoMap[CalleeName];
          // create the 1st intrinsic
          Type *tys0[1];
          SmallVector<llvm::Value *, 3> args0;
          // build type-list for the 1st intrinsic
          tys0[0] = InstCall->getArgOperand(0)->getType();
          // build argument list for the 1st intrinsic
          args0.append(InstCall->op_begin(),
                       InstCall->op_begin() +
                           IGCLLVM::getNumArgOperands(InstCall));
          Function *IntrinFunc0 =
              GenXIntrinsic::getAnyDeclaration(&M, pair.first, tys0);
          Instruction *IntrinCall0 = CallInst::Create(
              IntrinFunc0, args0, InstCall->getName(), InstCall);
          IntrinCall0->setDebugLoc(InstCall->getDebugLoc());
          // create the 2nd intrinsic
          Type *tys1[2];
          SmallVector<llvm::Value *, 3> args1;
          // build type-list for the 1st intrinsic
          tys1[0] = callee->getReturnType();
          tys1[1] = IntrinCall0->getType();
          // build argument list for the 1st intrinsic
          args1.push_back(IntrinCall0);
          Function *IntrinFunc1 =
              GenXIntrinsic::getAnyDeclaration(&M, pair.second, tys1);
          Instruction *IntrinCall1 = CallInst::Create(
              IntrinFunc1, args1, InstCall->getName(), InstCall);
          IntrinCall1->setDebugLoc(InstCall->getDebugLoc());
          InstCall->replaceAllUsesWith(IntrinCall1);
          ListDelete.push_back(InstCall);
        }
        // other cases
        else if (CalleeName.startswith("__builtin_IB_itof")) {
          Instruction *Replace = SIToFPInst::Create(
              Instruction::SIToFP, InstCall->getArgOperand(0),
              callee->getReturnType(), InstCall->getName(), InstCall);
          Replace->setDebugLoc(InstCall->getDebugLoc());
          InstCall->replaceAllUsesWith(Replace);
          ListDelete.push_back(InstCall);
        } else if (CalleeName.startswith("__builtin_IB_uitof")) {
          Instruction *Replace = UIToFPInst::Create(
              Instruction::UIToFP, InstCall->getArgOperand(0),
              callee->getReturnType(), InstCall->getName(), InstCall);
          Replace->setDebugLoc(InstCall->getDebugLoc());
          InstCall->replaceAllUsesWith(Replace);
          ListDelete.push_back(InstCall);
        } else if (CalleeName.startswith("__builtin_IB_mul_rtz")) {
          Instruction *Mul = BinaryOperator::Create(
              Instruction::FMul, InstCall->getArgOperand(0),
              InstCall->getArgOperand(1), InstCall->getName(), InstCall);
          Mul->setDebugLoc(InstCall->getDebugLoc());
          Type *tys[1];
          SmallVector<llvm::Value *, 3> args;
          // build type-list for the 1st intrinsic
          tys[0] = InstCall->getArgOperand(0)->getType();
          // build argument list for the 1st intrinsic
          args.push_back(Mul);
          Function *IntrinFunc = GenXIntrinsic::getAnyDeclaration(
              &M, GenXIntrinsic::genx_rndz, tys);
          Instruction *IntrinCall =
              CallInst::Create(IntrinFunc, args, InstCall->getName(), InstCall);
          IntrinCall->setDebugLoc(InstCall->getDebugLoc());
          InstCall->replaceAllUsesWith(IntrinCall);
          ListDelete.push_back(InstCall);
        } else if (CalleeName.startswith("__builtin_IB_add_rtz")) {
          Instruction *Add = BinaryOperator::Create(
              Instruction::FAdd, InstCall->getArgOperand(0),
              InstCall->getArgOperand(1), InstCall->getName(), InstCall);
          Add->setDebugLoc(InstCall->getDebugLoc());
          Type *tys[1];
          SmallVector<llvm::Value *, 3> args;
          // build type-list for the 1st intrinsic
          tys[0] = InstCall->getArgOperand(0)->getType();
          // build argument list for the 1st intrinsic
          args.push_back(Add);
          Function *IntrinFunc = GenXIntrinsic::getAnyDeclaration(
              &M, GenXIntrinsic::genx_rndz, tys);
          Instruction *IntrinCall =
              CallInst::Create(IntrinFunc, args, InstCall->getName(), InstCall);
          IntrinCall->setDebugLoc(InstCall->getDebugLoc());
          InstCall->replaceAllUsesWith(IntrinCall);
          ListDelete.push_back(InstCall);
        }
      }
    }
  }
  // clean up the dead calls
  for (auto I : ListDelete) {
    I->eraseFromParent();
  }
}

static void removeFunctionBitcasts(Module &M) {
  std::vector<Instruction *> list_delete;
  DenseMap<Function *, std::vector<Function *>> bitcastFunctionMap;

  for (Function &func : M) {
    for (auto &BB : func) {
      for (auto I = BB.begin(), E = BB.end(); I != E; I++) {
        CallInst *pInstCall = dyn_cast<CallInst>(I);
        if (!pInstCall || pInstCall->getCalledFunction())
          continue;
        auto *funcToBeChanged = dyn_cast<Function>(
            IGCLLVM::getCalledValue(pInstCall)->stripPointerCasts());
        if (!funcToBeChanged || funcToBeChanged->isDeclaration())
          continue;
        // Map between values (functions) in source of bitcast
        // to their counterpart values in destination
        llvm::ValueToValueMapTy operandMap;
        Function *pDstFunc = nullptr;
        auto BCFMI = bitcastFunctionMap.find(funcToBeChanged);
        bool notExists = BCFMI == bitcastFunctionMap.end();
        if (!notExists) {
          auto funcVec = bitcastFunctionMap[funcToBeChanged];
          notExists = true;
          for (Function *F : funcVec) {
            if (pInstCall->getFunctionType() == F->getFunctionType()) {
              notExists = false;
              pDstFunc = F;
              break;
            }
          }
        }

        if (notExists) {
          pDstFunc = Function::Create(pInstCall->getFunctionType(),
                                      funcToBeChanged->getLinkage(),
                                      funcToBeChanged->getName(), &M);
          if (pDstFunc->arg_size() != funcToBeChanged->arg_size())
            continue;
          // Need to copy the attributes over too.
          auto FuncAttrs = funcToBeChanged->getAttributes();
          pDstFunc->setAttributes(FuncAttrs);

          // Go through and convert function arguments over, remembering the
          // mapping.
          Function::arg_iterator itSrcFunc = funcToBeChanged->arg_begin();
          Function::arg_iterator eSrcFunc = funcToBeChanged->arg_end();
          llvm::Function::arg_iterator itDest = pDstFunc->arg_begin();

          for (; itSrcFunc != eSrcFunc; ++itSrcFunc, ++itDest) {
            itDest->setName(itSrcFunc->getName());
            operandMap[&(*itSrcFunc)] = &(*itDest);
          }

          // Clone the body of the function into the dest function.
          SmallVector<ReturnInst *, 8> Returns;
          IGCLLVM::CloneFunctionInto(
              pDstFunc, funcToBeChanged, operandMap,
              IGCLLVM::CloneFunctionChangeType::LocalChangesOnly, Returns, "");

          // Update returns to match a new function ret type.
          Instruction::CastOps castOp = Instruction::BitCast;
          Type *oldRetTy = funcToBeChanged->getFunctionType()->getReturnType();
          Type *newRetTy = pInstCall->getFunctionType()->getReturnType();
          if (oldRetTy == newRetTy) {
            // Do nothing.
          } else if (oldRetTy->isIntOrIntVectorTy() &&
                     newRetTy->isIntOrIntVectorTy()) {
            unsigned oldRetTypeSize = oldRetTy->getScalarSizeInBits();
            unsigned newRetTypeSize = newRetTy->getScalarSizeInBits();
            if (oldRetTypeSize > newRetTypeSize)
              castOp = Instruction::Trunc;
            else {
              AttributeSet retAttrs =
                  funcToBeChanged->getAttributes().getRetAttrs();
              if (retAttrs.hasAttribute(Attribute::ZExt))
                castOp = Instruction::ZExt;
              else if (retAttrs.hasAttribute(Attribute::SExt))
                castOp = Instruction::SExt;
              else
                llvm_unreachable("Expected ext attribute on return type.");
            }
          } else
            vc::diagnose(pInstCall->getContext(), "GenXImportOCLBiF",
                         "Unhandled function pointer cast.", pInstCall);

          if (oldRetTy != newRetTy)
            llvm::for_each(Returns, [castOp, newRetTy](ReturnInst *RI) {
              auto *CI = CastInst::Create(castOp, RI->getReturnValue(),
                                          newRetTy, ".rvc", RI);
              RI->setOperand(0, CI);
            });

          pDstFunc->setCallingConv(funcToBeChanged->getCallingConv());
          bitcastFunctionMap[funcToBeChanged].push_back(pDstFunc);
        }

        std::vector<Value *> Args;
        for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(pInstCall); I != E;
             ++I) {
          Args.push_back(pInstCall->getArgOperand(I));
        }
        auto newCI = CallInst::Create(pDstFunc, Args, "", pInstCall);
        newCI->takeName(pInstCall);
        newCI->setCallingConv(pInstCall->getCallingConv());
        pInstCall->replaceAllUsesWith(newCI);
        pInstCall->dropAllReferences();
        if (funcToBeChanged->use_empty())
          funcToBeChanged->eraseFromParent();

        list_delete.push_back(pInstCall);
      }
    }
  }

  for (auto i : list_delete) {
    i->eraseFromParent();
  }
}

static bool isOCLBuiltinDecl(const Function &F) {
  if (!F.isDeclaration())
    return false;
  if (F.isIntrinsic() || GenXIntrinsic::isGenXIntrinsic(&F))
    return false;
  // presuming that the only declarations left are from OCL header
  return true;
}

class GenXImportOCLBiF final : public ModulePass {

public:
  static char ID;
  GenXImportOCLBiF() : ModulePass(ID) {}
  StringRef getPassName() const override { return "GenX import OCL BiF"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};

char GenXImportOCLBiF::ID = 0;

INITIALIZE_PASS_BEGIN(GenXImportOCLBiF, "GenXImportOCLBiF", "GenXImportOCLBiF",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXImportOCLBiF, "GenXImportOCLBiF", "GenXImportOCLBiF",
                    false, false)

namespace llvm {
ModulePass *createGenXImportOCLBiFPass() {
  initializeGenXImportOCLBiFPass(*PassRegistry::getPassRegistry());
  return new GenXImportOCLBiF;
}
} // namespace llvm

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses
GenXImportOCLBiFPass::run(llvm::Module &M,
                          llvm::AnalysisManager<llvm::Module> &) {
  GenXImportOCLBiF GenXImportOCL;
  if (GenXImportOCL.runOnModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif

void GenXImportOCLBiF::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
}

// Whether module has uresolved calls to OpenCL builtins.
static bool OCLBuiltinsRequired(const Module &M) {
  return std::any_of(M.begin(), M.end(),
                     [](const Function &F) { return isOCLBuiltinDecl(F); });
}

static void forceInlining(Module &M, const StringSet<> &GVS) {
  for (auto &Entry : GVS) {
    StringRef Name = Entry.getKey();
    Value *GV = M.getValueSymbolTable().lookup(Name);
    if (!isa<Function>(GV))
      continue;
    cast<Function>(GV)->addFnAttr(Attribute::AlwaysInline);
  }
}

bool GenXImportOCLBiF::runOnModule(Module &M) {
  if (llvm::none_of(M, [](const Function &F) { return isOCLBuiltinDecl(F); }))
    return false;

#ifdef BIF_LINK_BC
  // Link all needed Bif instr
  {
    IGC::BiFManager::BiFManagerHandler bifLinker(M.getContext());

    bifLinker.SetTargetTriple(M.getTargetTriple());
    bifLinker.SetDataLayout(M.getDataLayout());
    bifLinker.SetCallbackLinker([](Module &M, const StringSet<> &GVS) {
      internalizeModule(M, [&GVS](const GlobalValue &GV) {
        return !GV.hasName() || (GVS.count(GV.getName()) == 0);
      });
      // FIXME: workaround to solve several issues in the backend, remove it
      forceInlining(M, GVS);
    });

    bifLinker.LinkBiF(M);
  }
#endif // BIF_LINK_BC

  removeFunctionBitcasts(M);
  BIConvert{}.runOnModule(M);
  return true;
}

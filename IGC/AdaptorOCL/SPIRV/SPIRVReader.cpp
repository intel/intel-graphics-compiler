/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2014 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal with the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.
Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the documentation
and/or other materials provided with the distribution.
Neither the names of Advanced Micro Devices, Inc., nor the names of its
contributors may be used to endorse or promote products derived from this
Software without specific prior written permission.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file implements conversion of SPIR-V binary to LLVM IR.

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/DIBuilder.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Support/TypeSize.h"

#include <llvm/Support/ScaledNumber.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Analysis/CFG.h>
#include "libSPIRV/SPIRVDebugInfoExt.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "common/LLVMWarningsPop.hpp"
#include "libSPIRV/SPIRVAsm.h"
#include "llvm/IR/InlineAsm.h"

#include "libSPIRV/SPIRVFunction.h"
#include "libSPIRV/SPIRVInstruction.h"
#include "libSPIRV/SPIRVModule.h"
#include "libSPIRV/SPIRVMemAliasingINTEL.h"
#include "SPIRVInternal.h"
#include "common/MDFrameWork.h"
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/MDBuilder.h>

#include <iostream>
#include <fstream>

#include "Probe/Assertion.h"

using namespace llvm;


namespace igc_spv{
// Prefix for placeholder global variable name.
const char* kPlaceholderPrefix = "placeholder.";

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
// Save the translated LLVM before validation for debugging purpose.
static bool DbgSaveTmpLLVM = false;
static const char *DbgTmpLLVMFileName = "_tmp_llvmspirv_module.ll";
#endif

static bool
isOpenCLKernel(SPIRVFunction *BF) {
   return BF->getModule()->isEntryPoint(ExecutionModelKernel, BF->getId());
}

__attr_unused static void
dumpLLVM(Module *M, const std::string &FName) {
  std::error_code EC;
  raw_fd_ostream FS(FName, EC, sys::fs::OF_None);
  if (!FS.has_error()) {
    FS << *M;
  }
  FS.close();
}

static MDNode*
getMDNodeStringIntVec(LLVMContext *Context, llvm::StringRef Str,
    const std::vector<SPIRVWord>& IntVals) {
  std::vector<Metadata*> ValueVec;
  ValueVec.push_back(MDString::get(*Context, Str));
  for (auto &I:IntVals)
    ValueVec.push_back(ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(*Context), I)));
  return MDNode::get(*Context, ValueVec);
}

static MDNode*
getMDTwoInt(LLVMContext *Context, unsigned Int1, unsigned Int2) {
  std::vector<Metadata*> ValueVec;
  ValueVec.push_back(ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(*Context), Int1)));
  ValueVec.push_back(ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(*Context), Int2)));
  return MDNode::get(*Context, ValueVec);
}

static MDNode*
getMDString(LLVMContext *Context, llvm::StringRef Str) {
  std::vector<Metadata*> ValueVec;
  if (!Str.empty())
    ValueVec.push_back(MDString::get(*Context, Str));
  return MDNode::get(*Context, ValueVec);
}

static void
addOCLVersionMetadata(LLVMContext *Context, Module *M,
    llvm::StringRef MDName, unsigned Major, unsigned Minor) {
  NamedMDNode *NamedMD = M->getOrInsertNamedMetadata(MDName);
  NamedMD->addOperand(getMDTwoInt(Context, Major, Minor));
}

static void
addNamedMetadataString(LLVMContext *Context, Module *M,
    llvm::StringRef MDName, llvm::StringRef Str) {
  NamedMDNode *NamedMD = M->getOrInsertNamedMetadata(MDName);
  NamedMD->addOperand(getMDString(Context, Str));
}

static void
addOCLKernelArgumentMetadata(LLVMContext *Context,
  std::vector<llvm::Metadata*> &KernelMD, llvm::StringRef MDName,
    SPIRVFunction *BF, std::function<Metadata *(SPIRVFunctionParameter *)>Func){
  std::vector<Metadata*> ValueVec;
    ValueVec.push_back(MDString::get(*Context, MDName));
  BF->foreachArgument([&](SPIRVFunctionParameter *Arg) {
    ValueVec.push_back(Func(Arg));
  });
  KernelMD.push_back(MDNode::get(*Context, ValueVec));
}

class SPIRVToLLVM;

class SPIRVToLLVMDbgTran {
public:
  SPIRVToLLVMDbgTran(SPIRVModule *TBM, Module *TM, SPIRVToLLVM* s)
  :BM(TBM), M(TM), SpDbg(BM), Builder(*M), SPIRVTranslator(s) {
    Enable = BM->hasDebugInfo();
  }

  void addDbgInfoVersion() {
    if (!Enable)
      return;
    M->addModuleFlag(Module::Warning, "Dwarf Version",
        dwarf::DWARF_VERSION);
    M->addModuleFlag(Module::Warning, "Debug Info Version",
        DEBUG_METADATA_VERSION);
  }

  DIFile *getDIFile(const std::string &FileName){
    return getOrInsert(FileMap, FileName, [=](){
      std::string BaseName;
      std::string Path;
      splitFileName(FileName, BaseName, Path);
      if (!BaseName.empty())
        return Builder.createFile(BaseName, Path);
      else
        return Builder.createFile("", "");
    });
  }

  DIFile* getDIFile(SPIRVString* inst)
  {
      if (!inst)
          return nullptr;

      return getDIFile(inst->getStr());
  }

  DIFile* getDIFile(SPIRVExtInst* inst)
  {
      OpDebugSource src(inst);

      return getDIFile(src.getFileStr());
  }

  DICompileUnit* createCompileUnit() {
      if (!Enable || cu)
          return cu;

      OpCompilationUnit cunit(BM->getCompilationUnit());

      auto lang = convertSPIRVSourceLangToDWARF(cunit.getLang());
      auto file = getDIFile(BM->get<SPIRVExtInst>(cunit.getSource()));
      auto producer = "spirv";
      auto flags = "";
      auto rv = 0;

      cu = Builder.createCompileUnit(lang, file, producer, false, flags, rv);

      addDbgInfoVersion();

      return addMDNode(BM->getCompilationUnit(), cu);
  }

  DIExpression* createExpression(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIExpression*>(inst))
          return n;

      OpDebugExpression dbgExpr(inst);

      auto numOperations = dbgExpr.getNumOperations();
      llvm::SmallVector<uint64_t, 5> Exprs;
      for (unsigned int i = 0; i != numOperations; ++i)
      {
          OpDebugOperation operation(BM->get<SPIRVExtInst>(dbgExpr.getOperation(i)));
          SPIRVDebug::ExpressionOpCode op = (SPIRVDebug::ExpressionOpCode)operation.getOperation();
          switch (op)
          {
#define CASE(dwarfOp, SPIRVOp)             case SPIRVOp: Exprs.push_back(dwarfOp); break;

              CASE(dwarf::DW_OP_deref, SPIRVDebug::ExpressionOpCode::Deref);
              CASE(dwarf::DW_OP_plus, SPIRVDebug::ExpressionOpCode::Plus);
              CASE(dwarf::DW_OP_minus, SPIRVDebug::ExpressionOpCode::Minus);
              CASE(dwarf::DW_OP_plus_uconst, SPIRVDebug::ExpressionOpCode::PlusUconst);
              CASE(dwarf::DW_OP_bit_piece, SPIRVDebug::ExpressionOpCode::BitPiece);
              CASE(dwarf::DW_OP_swap, SPIRVDebug::ExpressionOpCode::Swap);
              CASE(dwarf::DW_OP_xderef, SPIRVDebug::ExpressionOpCode::Xderef);
              CASE(dwarf::DW_OP_stack_value, SPIRVDebug::ExpressionOpCode::StackValue);
              CASE(dwarf::DW_OP_constu, SPIRVDebug::ExpressionOpCode::Constu);
              CASE(dwarf::DW_OP_LLVM_fragment, SPIRVDebug::ExpressionOpCode::Fragment);
#if LLVM_VERSION_MAJOR >= 9
              CASE(dwarf::DW_OP_LLVM_convert, SPIRVDebug::ExpressionOpCode::Convert);
#endif
              CASE(dwarf::DW_OP_addr, SPIRVDebug::ExpressionOpCode::Addr);
              CASE(dwarf::DW_OP_const1u, SPIRVDebug::ExpressionOpCode::Const1u);
              CASE(dwarf::DW_OP_const1s, SPIRVDebug::ExpressionOpCode::Const1s);
              CASE(dwarf::DW_OP_const2u, SPIRVDebug::ExpressionOpCode::Const2u);
              CASE(dwarf::DW_OP_const2s, SPIRVDebug::ExpressionOpCode::Const2s);
              CASE(dwarf::DW_OP_const4u, SPIRVDebug::ExpressionOpCode::Const4u);
              CASE(dwarf::DW_OP_const4s, SPIRVDebug::ExpressionOpCode::Const4s);
              CASE(dwarf::DW_OP_const8u, SPIRVDebug::ExpressionOpCode::Const8u);
              CASE(dwarf::DW_OP_const8s, SPIRVDebug::ExpressionOpCode::Const8s);
              CASE(dwarf::DW_OP_consts, SPIRVDebug::ExpressionOpCode::Consts);
              CASE(dwarf::DW_OP_dup, SPIRVDebug::ExpressionOpCode::Dup);
              CASE(dwarf::DW_OP_drop, SPIRVDebug::ExpressionOpCode::Drop);
              CASE(dwarf::DW_OP_over, SPIRVDebug::ExpressionOpCode::Over);
              CASE(dwarf::DW_OP_pick, SPIRVDebug::ExpressionOpCode::Pick);
              CASE(dwarf::DW_OP_rot, SPIRVDebug::ExpressionOpCode::Rot);
              CASE(dwarf::DW_OP_abs, SPIRVDebug::ExpressionOpCode::Abs);
              CASE(dwarf::DW_OP_and, SPIRVDebug::ExpressionOpCode::And);
              CASE(dwarf::DW_OP_div, SPIRVDebug::ExpressionOpCode::Div);
              CASE(dwarf::DW_OP_mod, SPIRVDebug::ExpressionOpCode::Mod);
              CASE(dwarf::DW_OP_mul, SPIRVDebug::ExpressionOpCode::Mul);
              CASE(dwarf::DW_OP_neg, SPIRVDebug::ExpressionOpCode::Neg);
              CASE(dwarf::DW_OP_not, SPIRVDebug::ExpressionOpCode::Not);
              CASE(dwarf::DW_OP_or, SPIRVDebug::ExpressionOpCode::Or);
              CASE(dwarf::DW_OP_shl, SPIRVDebug::ExpressionOpCode::Shl);
              CASE(dwarf::DW_OP_shr, SPIRVDebug::ExpressionOpCode::Shr);
              CASE(dwarf::DW_OP_shra, SPIRVDebug::ExpressionOpCode::Shra);
              CASE(dwarf::DW_OP_xor, SPIRVDebug::ExpressionOpCode::Xor);
              CASE(dwarf::DW_OP_bra, SPIRVDebug::ExpressionOpCode::Bra);
              CASE(dwarf::DW_OP_eq, SPIRVDebug::ExpressionOpCode::Eq);
              CASE(dwarf::DW_OP_ge, SPIRVDebug::ExpressionOpCode::Ge);
              CASE(dwarf::DW_OP_gt, SPIRVDebug::ExpressionOpCode::Gt);
              CASE(dwarf::DW_OP_le, SPIRVDebug::ExpressionOpCode::Le);
              CASE(dwarf::DW_OP_lt, SPIRVDebug::ExpressionOpCode::Lt);
              CASE(dwarf::DW_OP_ne, SPIRVDebug::ExpressionOpCode::Ne);
              CASE(dwarf::DW_OP_skip, SPIRVDebug::ExpressionOpCode::Skip);
              CASE(dwarf::DW_OP_lit0, SPIRVDebug::ExpressionOpCode::Lit0);
              CASE(dwarf::DW_OP_lit1, SPIRVDebug::ExpressionOpCode::Lit1);
              CASE(dwarf::DW_OP_lit2, SPIRVDebug::ExpressionOpCode::Lit2);
              CASE(dwarf::DW_OP_lit3, SPIRVDebug::ExpressionOpCode::Lit3);
              CASE(dwarf::DW_OP_lit4, SPIRVDebug::ExpressionOpCode::Lit4);
              CASE(dwarf::DW_OP_lit5, SPIRVDebug::ExpressionOpCode::Lit5);
              CASE(dwarf::DW_OP_lit6, SPIRVDebug::ExpressionOpCode::Lit6);
              CASE(dwarf::DW_OP_lit7, SPIRVDebug::ExpressionOpCode::Lit7);
              CASE(dwarf::DW_OP_lit8, SPIRVDebug::ExpressionOpCode::Lit8);
              CASE(dwarf::DW_OP_lit9, SPIRVDebug::ExpressionOpCode::Lit9);
              CASE(dwarf::DW_OP_lit10, SPIRVDebug::ExpressionOpCode::Lit10);
              CASE(dwarf::DW_OP_lit11, SPIRVDebug::ExpressionOpCode::Lit11);
              CASE(dwarf::DW_OP_lit12, SPIRVDebug::ExpressionOpCode::Lit12);
              CASE(dwarf::DW_OP_lit13, SPIRVDebug::ExpressionOpCode::Lit13);
              CASE(dwarf::DW_OP_lit14, SPIRVDebug::ExpressionOpCode::Lit14);
              CASE(dwarf::DW_OP_lit15, SPIRVDebug::ExpressionOpCode::Lit15);
              CASE(dwarf::DW_OP_lit16, SPIRVDebug::ExpressionOpCode::Lit16);
              CASE(dwarf::DW_OP_lit17, SPIRVDebug::ExpressionOpCode::Lit17);
              CASE(dwarf::DW_OP_lit18, SPIRVDebug::ExpressionOpCode::Lit18);
              CASE(dwarf::DW_OP_lit19, SPIRVDebug::ExpressionOpCode::Lit19);
              CASE(dwarf::DW_OP_lit20, SPIRVDebug::ExpressionOpCode::Lit20);
              CASE(dwarf::DW_OP_lit21, SPIRVDebug::ExpressionOpCode::Lit21);
              CASE(dwarf::DW_OP_lit22, SPIRVDebug::ExpressionOpCode::Lit22);
              CASE(dwarf::DW_OP_lit23, SPIRVDebug::ExpressionOpCode::Lit23);
              CASE(dwarf::DW_OP_lit24, SPIRVDebug::ExpressionOpCode::Lit24);
              CASE(dwarf::DW_OP_lit25, SPIRVDebug::ExpressionOpCode::Lit25);
              CASE(dwarf::DW_OP_lit26, SPIRVDebug::ExpressionOpCode::Lit26);
              CASE(dwarf::DW_OP_lit27, SPIRVDebug::ExpressionOpCode::Lit27);
              CASE(dwarf::DW_OP_lit28, SPIRVDebug::ExpressionOpCode::Lit28);
              CASE(dwarf::DW_OP_lit29, SPIRVDebug::ExpressionOpCode::Lit29);
              CASE(dwarf::DW_OP_lit30, SPIRVDebug::ExpressionOpCode::Lit30);
              CASE(dwarf::DW_OP_lit31, SPIRVDebug::ExpressionOpCode::Lit31);
              CASE(dwarf::DW_OP_reg0, SPIRVDebug::ExpressionOpCode::Reg0);
              CASE(dwarf::DW_OP_reg1, SPIRVDebug::ExpressionOpCode::Reg1);
              CASE(dwarf::DW_OP_reg2, SPIRVDebug::ExpressionOpCode::Reg2);
              CASE(dwarf::DW_OP_reg3, SPIRVDebug::ExpressionOpCode::Reg3);
              CASE(dwarf::DW_OP_reg4, SPIRVDebug::ExpressionOpCode::Reg4);
              CASE(dwarf::DW_OP_reg5, SPIRVDebug::ExpressionOpCode::Reg5);
              CASE(dwarf::DW_OP_reg6, SPIRVDebug::ExpressionOpCode::Reg6);
              CASE(dwarf::DW_OP_reg7, SPIRVDebug::ExpressionOpCode::Reg7);
              CASE(dwarf::DW_OP_reg8, SPIRVDebug::ExpressionOpCode::Reg8);
              CASE(dwarf::DW_OP_reg9, SPIRVDebug::ExpressionOpCode::Reg9);
              CASE(dwarf::DW_OP_reg10, SPIRVDebug::ExpressionOpCode::Reg10);
              CASE(dwarf::DW_OP_reg11, SPIRVDebug::ExpressionOpCode::Reg11);
              CASE(dwarf::DW_OP_reg12, SPIRVDebug::ExpressionOpCode::Reg12);
              CASE(dwarf::DW_OP_reg13, SPIRVDebug::ExpressionOpCode::Reg13);
              CASE(dwarf::DW_OP_reg14, SPIRVDebug::ExpressionOpCode::Reg14);
              CASE(dwarf::DW_OP_reg15, SPIRVDebug::ExpressionOpCode::Reg15);
              CASE(dwarf::DW_OP_reg16, SPIRVDebug::ExpressionOpCode::Reg16);
              CASE(dwarf::DW_OP_reg17, SPIRVDebug::ExpressionOpCode::Reg17);
              CASE(dwarf::DW_OP_reg18, SPIRVDebug::ExpressionOpCode::Reg18);
              CASE(dwarf::DW_OP_reg19, SPIRVDebug::ExpressionOpCode::Reg19);
              CASE(dwarf::DW_OP_reg20, SPIRVDebug::ExpressionOpCode::Reg20);
              CASE(dwarf::DW_OP_reg21, SPIRVDebug::ExpressionOpCode::Reg21);
              CASE(dwarf::DW_OP_reg22, SPIRVDebug::ExpressionOpCode::Reg22);
              CASE(dwarf::DW_OP_reg23, SPIRVDebug::ExpressionOpCode::Reg23);
              CASE(dwarf::DW_OP_reg24, SPIRVDebug::ExpressionOpCode::Reg24);
              CASE(dwarf::DW_OP_reg25, SPIRVDebug::ExpressionOpCode::Reg25);
              CASE(dwarf::DW_OP_reg26, SPIRVDebug::ExpressionOpCode::Reg26);
              CASE(dwarf::DW_OP_reg27, SPIRVDebug::ExpressionOpCode::Reg27);
              CASE(dwarf::DW_OP_reg28, SPIRVDebug::ExpressionOpCode::Reg28);
              CASE(dwarf::DW_OP_reg29, SPIRVDebug::ExpressionOpCode::Reg29);
              CASE(dwarf::DW_OP_reg30, SPIRVDebug::ExpressionOpCode::Reg30);
              CASE(dwarf::DW_OP_reg31, SPIRVDebug::ExpressionOpCode::Reg31);
              CASE(dwarf::DW_OP_breg0, SPIRVDebug::ExpressionOpCode::Breg0);
              CASE(dwarf::DW_OP_breg1, SPIRVDebug::ExpressionOpCode::Breg1);
              CASE(dwarf::DW_OP_breg2, SPIRVDebug::ExpressionOpCode::Breg2);
              CASE(dwarf::DW_OP_breg3, SPIRVDebug::ExpressionOpCode::Breg3);
              CASE(dwarf::DW_OP_breg4, SPIRVDebug::ExpressionOpCode::Breg4);
              CASE(dwarf::DW_OP_breg5, SPIRVDebug::ExpressionOpCode::Breg5);
              CASE(dwarf::DW_OP_breg6, SPIRVDebug::ExpressionOpCode::Breg6);
              CASE(dwarf::DW_OP_breg7, SPIRVDebug::ExpressionOpCode::Breg7);
              CASE(dwarf::DW_OP_breg8, SPIRVDebug::ExpressionOpCode::Breg8);
              CASE(dwarf::DW_OP_breg9, SPIRVDebug::ExpressionOpCode::Breg9);
              CASE(dwarf::DW_OP_breg10, SPIRVDebug::ExpressionOpCode::Breg10);
              CASE(dwarf::DW_OP_breg11, SPIRVDebug::ExpressionOpCode::Breg11);
              CASE(dwarf::DW_OP_breg12, SPIRVDebug::ExpressionOpCode::Breg12);
              CASE(dwarf::DW_OP_breg13, SPIRVDebug::ExpressionOpCode::Breg13);
              CASE(dwarf::DW_OP_breg14, SPIRVDebug::ExpressionOpCode::Breg14);
              CASE(dwarf::DW_OP_breg15, SPIRVDebug::ExpressionOpCode::Breg15);
              CASE(dwarf::DW_OP_breg16, SPIRVDebug::ExpressionOpCode::Breg16);
              CASE(dwarf::DW_OP_breg17, SPIRVDebug::ExpressionOpCode::Breg17);
              CASE(dwarf::DW_OP_breg18, SPIRVDebug::ExpressionOpCode::Breg18);
              CASE(dwarf::DW_OP_breg19, SPIRVDebug::ExpressionOpCode::Breg19);
              CASE(dwarf::DW_OP_breg20, SPIRVDebug::ExpressionOpCode::Breg20);
              CASE(dwarf::DW_OP_breg21, SPIRVDebug::ExpressionOpCode::Breg21);
              CASE(dwarf::DW_OP_breg22, SPIRVDebug::ExpressionOpCode::Breg22);
              CASE(dwarf::DW_OP_breg23, SPIRVDebug::ExpressionOpCode::Breg23);
              CASE(dwarf::DW_OP_breg24, SPIRVDebug::ExpressionOpCode::Breg24);
              CASE(dwarf::DW_OP_breg25, SPIRVDebug::ExpressionOpCode::Breg25);
              CASE(dwarf::DW_OP_breg26, SPIRVDebug::ExpressionOpCode::Breg26);
              CASE(dwarf::DW_OP_breg27, SPIRVDebug::ExpressionOpCode::Breg27);
              CASE(dwarf::DW_OP_breg28, SPIRVDebug::ExpressionOpCode::Breg28);
              CASE(dwarf::DW_OP_breg29, SPIRVDebug::ExpressionOpCode::Breg29);
              CASE(dwarf::DW_OP_breg30, SPIRVDebug::ExpressionOpCode::Breg30);
              CASE(dwarf::DW_OP_breg31, SPIRVDebug::ExpressionOpCode::Breg31);
              CASE(dwarf::DW_OP_regx, SPIRVDebug::ExpressionOpCode::Regx);
              CASE(dwarf::DW_OP_fbreg, SPIRVDebug::ExpressionOpCode::Fbreg);
              CASE(dwarf::DW_OP_bregx, SPIRVDebug::ExpressionOpCode::Bregx);
              CASE(dwarf::DW_OP_piece, SPIRVDebug::ExpressionOpCode::Piece);
              CASE(dwarf::DW_OP_deref_size, SPIRVDebug::ExpressionOpCode::DerefSize);
              CASE(dwarf::DW_OP_xderef_size, SPIRVDebug::ExpressionOpCode::XderefSize);
              CASE(dwarf::DW_OP_nop, SPIRVDebug::ExpressionOpCode::Nop);
              CASE(dwarf::DW_OP_push_object_address, SPIRVDebug::ExpressionOpCode::PushObjectAddress);
              CASE(dwarf::DW_OP_call2, SPIRVDebug::ExpressionOpCode::Call2);
              CASE(dwarf::DW_OP_call4, SPIRVDebug::ExpressionOpCode::Call4);
              CASE(dwarf::DW_OP_call_ref, SPIRVDebug::ExpressionOpCode::CallRef);
              CASE(dwarf::DW_OP_form_tls_address, SPIRVDebug::ExpressionOpCode::FormTlsAddress);
              CASE(dwarf::DW_OP_call_frame_cfa, SPIRVDebug::ExpressionOpCode::CallFrameCfa);
              CASE(dwarf::DW_OP_implicit_value, SPIRVDebug::ExpressionOpCode::ImplicitValue);
              CASE(dwarf::DW_OP_implicit_pointer, SPIRVDebug::ExpressionOpCode::ImplicitPointer);
              CASE(dwarf::DW_OP_addrx, SPIRVDebug::ExpressionOpCode::Addrx);
              CASE(dwarf::DW_OP_constx, SPIRVDebug::ExpressionOpCode::Constx);
              CASE(dwarf::DW_OP_entry_value, SPIRVDebug::ExpressionOpCode::EntryValue);
              CASE(dwarf::DW_OP_const_type, SPIRVDebug::ExpressionOpCode::ConstTypeOp);
              CASE(dwarf::DW_OP_regval_type, SPIRVDebug::ExpressionOpCode::RegvalType);
              CASE(dwarf::DW_OP_deref_type, SPIRVDebug::ExpressionOpCode::DerefType);
              CASE(dwarf::DW_OP_xderef_type, SPIRVDebug::ExpressionOpCode::XderefType);
              CASE(dwarf::DW_OP_reinterpret, SPIRVDebug::ExpressionOpCode::Reinterpret);
          default:
              break;
          }

          unsigned int numOperands = 0;
          if (SPIRVDebug::OpCountMap.find(op) != SPIRVDebug::OpCountMap.end())
              numOperands = SPIRVDebug::OpCountMap[op];

          if (numOperands > 0)
          {
              for (unsigned int j = 1; j != numOperands; ++j)
              {
                  Exprs.push_back(operation.getLiteral(j));
              }
          }
      }

      return addMDNode(inst, Builder.createExpression(Exprs));
  }

  DINode::DIFlags static decodeFlag(SPIRVWord spirvFlags)
  {
      DINode::DIFlags flags = DINode::FlagZero;

      if (spirvFlags & SPIRVDebug::FlagIsArtificial)
          flags |= llvm::DINode::FlagArtificial;
      if (spirvFlags & SPIRVDebug::FlagIsExplicit)
          flags |= llvm::DINode::FlagExplicit;
      if (spirvFlags & SPIRVDebug::FlagIsPrototyped)
          flags |= llvm::DINode::FlagPrototyped;
      if (spirvFlags & SPIRVDebug::FlagIsLValueReference)
          flags |= llvm::DINode::FlagLValueReference;
      if (spirvFlags & SPIRVDebug::FlagIsRValueReference)
          flags |= llvm::DINode::FlagRValueReference;
      if ((spirvFlags & SPIRVDebug::FlagAccess) == SPIRVDebug::FlagIsPublic)
          flags |= llvm::DINode::FlagPublic;
      if ((spirvFlags & SPIRVDebug::FlagAccess) == SPIRVDebug::FlagIsProtected)
          flags |= llvm::DINode::FlagProtected;
      if ((spirvFlags & SPIRVDebug::FlagAccess) == SPIRVDebug::FlagIsPrivate)
          flags |= llvm::DINode::FlagPrivate;
      if (spirvFlags & SPIRVDebug::FlagIsObjectPointer)
          flags |= llvm::DINode::FlagObjectPointer;
      if (spirvFlags & SPIRVDebug::FlagIsStaticMember)
          flags |= llvm::DINode::FlagStaticMember;

      return flags;
  }

  DIType* createTypeBasic(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeBasic type(inst);
      uint64_t sizeInBits = type.getSize();
      unsigned int encoding;

      auto enc = type.getEncoding();
      auto& name = type.getName()->getStr();

      switch (enc)
      {
      case SPIRVDebug::EncodingTag::Boolean:
          encoding = dwarf::DW_ATE_boolean;
          break;
      case SPIRVDebug::EncodingTag::Address:
          encoding = dwarf::DW_ATE_address;
          break;
      case SPIRVDebug::EncodingTag::Float:
          encoding = dwarf::DW_ATE_float;
          break;
      case SPIRVDebug::EncodingTag::Signed:
          encoding = dwarf::DW_ATE_signed;
          break;
      case SPIRVDebug::EncodingTag::Unsigned:
          encoding = dwarf::DW_ATE_unsigned;
          break;
      case SPIRVDebug::EncodingTag::SignedChar:
          encoding = dwarf::DW_ATE_signed_char;
          break;
      case SPIRVDebug::EncodingTag::UnsignedChar:
          encoding = dwarf::DW_ATE_unsigned_char;
          break;
      default:
          return addMDNode(inst, Builder.createUnspecifiedType(name));
      }

      return addMDNode(inst, Builder.createBasicType(name, sizeInBits, encoding));
  }

  DIType* createPtrType(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugPtrType ptrType(inst);

      auto pointeeType = createType(BM->get<SPIRVExtInst>(ptrType.getBaseType()));
      auto flags = ptrType.getFlags();

      if (flags & SPIRVDebug::Flag::FlagIsLValueReference)
          return addMDNode(inst, Builder.createReferenceType(dwarf::DW_TAG_reference_type, pointeeType, M->getDataLayout().getPointerSizeInBits()));
      else if (flags & SPIRVDebug::Flag::FlagIsRValueReference)
          return addMDNode(inst, Builder.createReferenceType(dwarf::DW_TAG_rvalue_reference_type, pointeeType, M->getDataLayout().getPointerSizeInBits()));
      else if (flags & SPIRVDebug::Flag::FlagIsObjectPointer)
      {
          auto objType = Builder.createPointerType(pointeeType, M->getDataLayout().getPointerSizeInBits());
          return addMDNode(inst, Builder.createObjectPointerType(objType));
      }
      else
          return addMDNode(inst, Builder.createPointerType(pointeeType, M->getDataLayout().getPointerSizeInBits()));
  }

  DIType* createTypeQualifier(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeQualifier qualType(inst);

      auto baseType = createType(BM->get<SPIRVExtInst>(qualType.getBaseType()));
      auto qual = qualType.getQualifier();

      unsigned int qualifier = 0;
      if (qual == OpDebugTypeQualifier::TypeQualifier::qual_const)
          qualifier = dwarf::DW_TAG_const_type;
      else if (qual == OpDebugTypeQualifier::TypeQualifier::qual_restrict)
          qualifier = dwarf::DW_TAG_restrict_type;
      else if (qual == OpDebugTypeQualifier::TypeQualifier::qual_volatile)
          qualifier = dwarf::DW_TAG_volatile_type;

      return addMDNode(inst, Builder.createQualifiedType(qualifier, baseType));
  }

  uint64_t static getSizeInBits(DIType* type)
  {
      if (!type)
          return 0;

      auto Tag = type->getTag();
      if (Tag != dwarf::DW_TAG_member && Tag != dwarf::DW_TAG_typedef &&
          Tag != dwarf::DW_TAG_const_type && Tag != dwarf::DW_TAG_volatile_type &&
          Tag != dwarf::DW_TAG_restrict_type)
          return type->getSizeInBits();

      if (isa<llvm::DIBasicType>(type))
          return type->getSizeInBits();
      else if (auto DITy = dyn_cast_or_null<llvm::DIDerivedType>(type))
      {
#if LLVM_VERSION_MAJOR <= 8
          auto baseType = DITy->getBaseType().resolve();
#else
          auto baseType = DITy->getBaseType();
#endif
          return getSizeInBits(baseType);
      }
      else if (auto CTy = dyn_cast_or_null<llvm::DICompositeType>(type))
      {
#if LLVM_VERSION_MAJOR <= 8
          auto baseType = CTy->getBaseType().resolve();
#else
          auto baseType = DITy->getBaseType();
#endif
          return getSizeInBits(baseType);
      }

      return 0;
  }

  DIType* createMember(SPIRVExtInst* inst);

  Metadata* createArrayBound(SPIRVId id, int64_t& intResult)
  {
      SPIRVEntry* e = BM->get<SPIRVEntry>(id);
      switch (e->getOpCode())
      {
      case OpConstant:
          intResult = static_cast<SPIRVConstant*>(e)->getZExtIntValue();
          return nullptr;
      case OpExtInst:
          {
              SPIRVExtInst* ext = static_cast<SPIRVExtInst*>(e);
              switch (ext->getExtOp())
              {
              case OCLExtOpDbgKind::DebugInfoNone:
                  return nullptr;
              case OCLExtOpDbgKind::LocalVariable:
                  return createLocalVar(ext);
              case OCLExtOpDbgKind::DbgExpr:
                  return createExpression(ext);
              default:
                  llvm_unreachable("Unknown array bound");
                  return nullptr;
              }
          }
      default:
          llvm_unreachable("Unknown array bound");
          return nullptr;
      }
  }

  DIType* createTypeArray(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeArray arrayType(inst);

      auto baseType = createType(BM->get<SPIRVExtInst>(arrayType.getBaseType()));
      auto numDims = arrayType.getNumDims();
      SmallVector<llvm::Metadata *, 8> subscripts;
      uint64_t totalBits = getSizeInBits(baseType);

      for (unsigned int i = 0; i < numDims; i++)
      {
          int64_t loConst = 0;
          Metadata* loExpr = arrayType.hasLowerBounds() ? createArrayBound(arrayType.getDimLowerBound(i), loConst) : nullptr;

          int64_t countConst = 0;
          Metadata* countExpr = createArrayBound(arrayType.getDimCount(i), countConst);

          DISubrange* subrange;
          (void) loExpr;
#if LLVM_VERSION_MAJOR >= 11
          if (loExpr)
          {
              if (!countExpr)
              {
                  totalBits *= static_cast<uint64_t>(countConst);
                  countExpr = ConstantAsMetadata::get(ConstantInt::get(Type::getInt64Ty(M->getContext()), countConst));
              }
              subrange = Builder.getOrCreateSubrange(nullptr, loExpr, countExpr, nullptr);
          }
          else
#endif
          if (countExpr)
              subrange = Builder.getOrCreateSubrange(loConst, countExpr);
          else
          {
              totalBits *= static_cast<uint64_t>(countConst);
              subrange = Builder.getOrCreateSubrange(loConst, countConst);
          }
          subscripts.push_back(subrange);
      }

      DINodeArray subscriptArray = Builder.getOrCreateArray(subscripts);
      return addMDNode(inst, Builder.createArrayType(totalBits, 0, baseType, subscriptArray));
  }

  DIType* createTypeVector(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeVector vectorType(inst);

      auto size = vectorType.getNumComponents();
      auto type = createType(BM->get<SPIRVExtInst>(vectorType.getBaseType()));

      SmallVector<llvm::Metadata *, 8> subscripts;
      subscripts.push_back(Builder.getOrCreateSubrange(0, size));
      DINodeArray subscriptArray = Builder.getOrCreateArray(subscripts);

      return addMDNode(inst, Builder.createVectorType(size * getSizeInBits(type), 0, type, subscriptArray));
  }

  DIType* createTypeDef(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeDef typeDef(inst);

      auto type = createType(BM->get<SPIRVExtInst>(typeDef.getBaseType()));
      auto& name = typeDef.getName()->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(typeDef.getSource()));
      auto line = typeDef.getLine();
      auto scopeContext = createScope(BM->get<SPIRVExtInst>(typeDef.getParent()));

      return addMDNode(inst, Builder.createTypedef(type, name, file, line, scopeContext));
  }

  DIType* createTypeEnum(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeEnum typeEnum(inst);
      auto scope = createScope(BM->get<SPIRVExtInst>(typeEnum.getParent()));
      auto& name = typeEnum.getName()->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(typeEnum.getSource()));
      auto line = typeEnum.getLine();
      auto size = typeEnum.getSize();
      auto type = createType(BM->get<SPIRVExtInst>(typeEnum.getType()));

      SmallVector<Metadata*,6> elements;

      for (unsigned int i = 0; i != typeEnum.getNumItems(); i++)
      {
          auto item = typeEnum.getItem(i);
          auto enumerator = Builder.createEnumerator(item.first->getStr(), item.second);
          elements.push_back(enumerator);
      }

      auto nodeArray = Builder.getOrCreateArray(llvm::makeArrayRef(elements));

      return addMDNode(inst, Builder.createEnumerationType(scope, name, file, line, size, 0, nodeArray, type));
  }

  DIType* createCompositeType(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeComposite compositeType(inst);

      auto tag = compositeType.getTag();
      auto& name = compositeType.getName()->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(compositeType.getSource()));
      auto line = compositeType.getLine();
      auto size = compositeType.getSize();
      auto spirvFlags = compositeType.getFlags();
      auto scope = createScope(BM->get<SPIRVExtInst>(compositeType.getParent()));

      DINode::DIFlags flags = decodeFlag(spirvFlags);

      if (!scope)
          scope = cu;

#if 0
      // SPIRV spec has single parent field whereas LLVM DIBuilder API requires Scope as well as DerivedFrom.
      // What is expected behavior?

      // parent may be OpDebugCompilationUnit/OpDebugFunction/OpDebugLexicalBlock/OpDebugTypeComposite
      DIType* from = nullptr;
      auto parentInst = BM->get<SPIRVExtInst>(parent);
      if (parentInst->getExtOp() == OCLExtOpDbgKind::CompileUnit)
          from = nullptr;
      else if (parentInst->getExtOp() == OCLExtOpDbgKind::Function)
          from = nullptr;//createFunction(parentInst);
      else if (parentInst->getExtOp() == OCLExtOpDbgKind::LexicalBlock)
          from = nullptr; //createLexicalBlock(parentInst);
      else if (parentInst->getExtOp() == OCLExtOpDbgKind::TypeComposite)
          from = createCompositeType(parentInst);
#endif

      DIType* derivedFrom = nullptr;
      DICompositeType* newNode = nullptr;
      if (tag == SPIRVDebug::CompositeTypeTag::Structure)
      {
          newNode = addMDNode(inst, Builder.createStructType(scope, name,
              file, line, size, 0, flags, derivedFrom, DINodeArray()));
      }
      else if (tag == SPIRVDebug::CompositeTypeTag::Union)
      {
          newNode = addMDNode(inst, Builder.createUnionType(scope, name,
              file, line, size, 0, flags, DINodeArray()));
      }
      else if (tag == SPIRVDebug::CompositeTypeTag::Class)
      {
          // TODO: should be replaced with createClassType, when bug with creating
          // ClassType with llvm::dwarf::DW_TAG_struct_type tag will be fixed
          auto CT = Builder.createReplaceableCompositeType(
              llvm::dwarf::DW_TAG_class_type, name, scope, file, line, 0,
              size, 0, flags);
          CT = llvm::MDNode::replaceWithDistinct(llvm::TempDICompositeType(CT));

          newNode = addMDNode(inst, CT);
      }

      SmallVector<Metadata*, 6> elements;
      for (unsigned int i = 0; i != compositeType.getNumItems(); i++)
      {
          auto member = static_cast<SPIRVExtInst*>(BM->getEntry(compositeType.getItem(i)));
          elements.push_back(createType(member));
      }

      DINodeArray Elements = Builder.getOrCreateArray(elements);
      Builder.replaceArrays(newNode, Elements);

      return newNode;
  }

  DIType* createTypeInherit(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugTypeInheritance typeInherit(inst);

      auto type = createType(BM->get<SPIRVExtInst>(typeInherit.getChild()));
      auto base = createType(BM->get<SPIRVExtInst>(typeInherit.getParent()));
      auto offset = typeInherit.getOffset();
      auto spirvFlags = typeInherit.getFlags();

      auto flags = decodeFlag(spirvFlags);

      return addMDNode(inst, Builder.createInheritance(type, base, offset, flags));
  }

  DIType* createPtrToMember(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIType*>(inst))
          return n;

      OpDebugPtrToMember ptrToMember(inst);

      auto pointee = createType(BM->get<SPIRVExtInst>(ptrToMember.getType()));
      auto Class = createType(BM->get<SPIRVExtInst>(ptrToMember.getParent()));
      auto size = M->getDataLayout().getPointerSizeInBits();

      return addMDNode(inst, Builder.createMemberPointerType(pointee, Class, size));
  }

  DITemplateParameter* createTemplateParm(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DITemplateParameter*>(inst))
          return n;

      auto templateOp = inst->getExtOp();

      if (templateOp == OCLExtOpDbgKind::TypeTemplateParameter)
      {
          OpDebugTypeTemplateParm parm(inst);
          auto actualType = createType(BM->get<SPIRVExtInst>(parm.getActualType()));
          if (!parm.hasValue())
          {
              return addMDNode(inst, Builder.createTemplateTypeParameter(cu, parm.getName()->getStr(),
                  actualType));
          }
          else
          {
              auto val = parm.getValue();
              auto constant = ConstantInt::get(Type::getInt64Ty(M->getContext()), val);
              return addMDNode(inst, Builder.createTemplateValueParameter(cu, parm.getName()->getStr(),
                  actualType, constant));
          }
      }
      else if (templateOp == OCLExtOpDbgKind::TypeTemplateTemplateParameter)
      {
          OpDebugTypeTemplateParmPack parmPack(inst);
          auto& name = parmPack.getName()->getStr();
          SmallVector<llvm::Metadata *, 8> Elts;
          for (unsigned int i = 0; i != parmPack.getNumParms(); i++) {
              Elts.push_back(createTemplateParm(BM->get<SPIRVExtInst>(parmPack.getParm(i))));
          }
          DINodeArray pack = Builder.getOrCreateArray(Elts);

          return addMDNode(inst, Builder.createTemplateParameterPack(cu, name, nullptr, pack));
      }
      else if (templateOp == OCLExtOpDbgKind::TypeTemplateParameterPack)
      {
          OpDebugTypeTemplateTemplateParm tempTempParm(inst);
          auto& name = tempTempParm.getName()->getStr();
          auto& templName = tempTempParm.getTemplateName()->getStr();
          return addMDNode(inst, Builder.createTemplateTemplateParameter(cu, name, nullptr, templName));
      }
      return nullptr;
  }

  MDNode* createTypeTemplate(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DITemplateParameter*>(inst))
          return n;

      OpDebugTypeTemplate typeTemplate(inst);

      auto target = createType(BM->get<SPIRVExtInst>(typeTemplate.getTarget()));
      SmallVector<llvm::Metadata *, 8> Elts;
      for (unsigned int i = 0; i != typeTemplate.getNumParms(); i++)
      {
          auto parm = BM->get<SPIRVExtInst>(typeTemplate.getParm(i));

          Elts.push_back(createTemplateParm(parm));
      }
      DINodeArray TParams = Builder.getOrCreateArray(Elts);

      if (DICompositeType *Comp = dyn_cast<DICompositeType>(target)) {
          Builder.replaceArrays(Comp, Comp->getElements(), TParams);
          return Comp;
      }
      if (isa<DISubprogram>(target)) {
          // This constant matches with one used in
          // DISubprogram::getRawTemplateParams()
#if LLVM_VERSION_MAJOR == 4
          const unsigned TemplateParamsIndex = 8;
#elif LLVM_VERSION_MAJOR >= 7
          const unsigned TemplateParamsIndex = 9;
#endif
          target->replaceOperandWith(TemplateParamsIndex, TParams.get());
          IGC_ASSERT_MESSAGE(cast<DISubprogram>(target)->getRawTemplateParams() == TParams.get(), "Invalid template parameters");
          return target;
      }

      return nullptr;
  }

  DIType* createType(SPIRVExtInst* type)
  {
      if (!type)
      {
          // return void type
          return Builder.createNullPtrType();
      }

      if (auto n = getExistingNode<DIType*>(type))
          return n;

      switch (type->getExtOp())
      {
      case OCLExtOpDbgKind::TypeBasic:
          return createTypeBasic(type);
      case OCLExtOpDbgKind::TypePtr:
          return createPtrType(type);
      case OCLExtOpDbgKind::TypeComposite:
          return createCompositeType(type);
      case OCLExtOpDbgKind::TypeQualifier:
          return createTypeQualifier(type);
      case OCLExtOpDbgKind::TypeArray:
          return createTypeArray(type);
      case OCLExtOpDbgKind::TypeVector:
          return createTypeVector(type);
      case OCLExtOpDbgKind::TypeDef:
          return createTypeDef(type);
      case OCLExtOpDbgKind::TypeEnum:
          return createTypeEnum(type);
      case OCLExtOpDbgKind::TypeInheritance:
          return createTypeInherit(type);
      case OCLExtOpDbgKind::TypePtrToMember:
          return createPtrToMember(type);
      case OCLExtOpDbgKind::TypeFunction:
          return createSubroutineType(type);
      case OCLExtOpDbgKind::TypeTemplate:
          return (DIType*)createTypeTemplate(type);
      case OCLExtOpDbgKind::Function:
          return (DIType*)createFunction(type);
      case OCLExtOpDbgKind::TypeMember:
          return createMember(type);
      case OCLExtOpDbgKind::FuncDecl:
          return (DIType*)createFunctionDecl(type);
      case OCLExtOpDbgKind::DebugInfoNone:
          return Builder.createUnspecifiedType("unspecified_type");
      default:
          break;
      }

      return addMDNode(type, Builder.createBasicType("int", 4, 0));
  }

  DIGlobalVariable* createGlobalVariable(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIGlobalVariable*>(inst))
          return n;

      OpDebugGlobalVar globalVar(inst);

      auto& name = globalVar.getName()->getStr();
      auto& linkageName = globalVar.getLinkageName()->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(globalVar.getSource()));
      auto line = globalVar.getLine();
      auto type = createType(BM->get<SPIRVExtInst>(globalVar.getType()));

      return addMDNode(inst, Builder.createTempGlobalVariableFwdDecl(getCompileUnit(), name, linkageName, file,
          (unsigned int)line, type, true));
  }

  DISubprogram* createFunctionDecl(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DISubprogram*>(inst))
          return n;

      OpDebugFuncDecl funcDcl(inst);

      auto scope = createScope(BM->get<SPIRVExtInst>(funcDcl.getParent()));
      auto& name = funcDcl.getName()->getStr();
      auto& linkageName = funcDcl.getLinkageName()->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(funcDcl.getSource()));
      auto line = funcDcl.getLine();
      auto type = createSubroutineType(BM->get<SPIRVExtInst>(funcDcl.getType()));

      auto spirvFlags = funcDcl.getFlags();

      DINode::DIFlags flags = decodeFlag(spirvFlags);

      bool isDefinition = spirvFlags & SPIRVDebug::FlagIsDefinition;
      bool isOptimized = spirvFlags & SPIRVDebug::FlagIsOptimized;
      bool isLocal = spirvFlags & SPIRVDebug::FlagIsLocal;

      SmallVector<llvm::Metadata*, 8> Elts;
      DINodeArray TParams = Builder.getOrCreateArray(Elts);
      llvm::DITemplateParameterArray TParamsArray = TParams.get();

      if (isa<DICompositeType>(scope) || isa<DINamespace>(scope) || isa<DIModule>(scope))
          return addMDNode(inst, Builder.createMethod(scope, name, linkageName, file, line, type,
              isLocal, isDefinition, 0, 0, 0, nullptr, flags, isOptimized, TParamsArray));
      else
        return addMDNode(inst, Builder.createTempFunctionFwdDecl(scope, name, linkageName, file, (unsigned int)line, type,
          isLocal, isDefinition, (unsigned int)line, flags, isOptimized, TParamsArray));
  }

  bool isTemplateType(SPIRVExtInst* inst)
  {
      return (inst->getExtOp() == OCLExtOpDbgKind::TypeTemplate);
  }

  bool isTypeVoid(SPIRVId id)
  {
      auto entry = BM->get<SPIRVEntry>(id);
      return entry && entry->getOpCode() == igc_spv::Op::OpTypeVoid;
  }

  DISubroutineType* createSubroutineType(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DISubroutineType*>(inst))
          return n;

      OpDebugSubroutineType spType(inst);
      std::vector<Metadata*> Args;
      Args.push_back((isTypeVoid(spType.getReturnType()) || isDebugInfoNone(spType.getReturnType())) ? nullptr :
          createType(BM->get<SPIRVExtInst>(spType.getReturnType())));
      auto flags = decodeFlag(spType.getFlags());

      for (unsigned int i = 0; i != spType.getNumParms(); i++)
      {
          auto parmType = spType.getParmType(i);
          Args.push_back(createType(static_cast<SPIRVExtInst*>(BM->getValue(parmType))));
      }

      return addMDNode(inst, Builder.createSubroutineType(Builder.getOrCreateTypeArray(Args), flags));
  }

  bool isDebugInfoNone(SPIRVId id)
  {
      auto e = BM->get<SPIRVEntry>(id);
      if (e->getOpCode() == igc_spv::Op::OpExtInst)
      {
          auto entry = BM->get<SPIRVExtInst>(id);
          return entry && entry->getExtOp() == OCLExtOpDbgKind::DebugInfoNone;
      }
      return false;
  }

  DISubprogram* createFunction(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DISubprogram*>(inst))
          return n;

      OpDebugSubprogram sp(inst);

      auto scope = createScope(BM->get<SPIRVExtInst>(sp.getParent()));
      auto& name = sp.getName()->getStr();
      auto& linkageName = sp.getLinkage()->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(sp.getSource()));
      auto spType = createSubroutineType(BM->get<SPIRVExtInst>(sp.getType()));
      auto spirvFlags = sp.getFlags();

      DINode::DIFlags flags = decodeFlag(spirvFlags);

      bool isDefinition = spirvFlags & SPIRVDebug::FlagIsDefinition;
      bool isOptimized = spirvFlags & SPIRVDebug::FlagIsOptimized;
      bool isLocal = spirvFlags & SPIRVDebug::FlagIsLocal;
      auto funcSPIRVId = sp.getSPIRVFunction();

      DISubprogram* decl = nullptr;
      if (sp.hasDeclaration() &&
          !isDebugInfoNone(sp.getDecl()))
          decl = (DISubprogram*)createType(BM->get<SPIRVExtInst>(sp.getDecl()));

      SmallVector<llvm::Metadata *, 8> Elts;
      DINodeArray TParams = Builder.getOrCreateArray(Elts);
      llvm::DITemplateParameterArray TParamsArray = TParams.get();
      DISubprogram* diSP = nullptr;
      if ((isa<DICompositeType>(scope) || isa<DINamespace>(scope) || isa<DIModule>(scope)) && !isDefinition)
      {
          diSP = Builder.createMethod(scope, name, linkageName, file, sp.getLine(), spType, isLocal, isDefinition,
              0, 0, 0, nullptr, flags, isOptimized, TParamsArray);
      }
      else
      {
          diSP = Builder.createFunction(scope, name, linkageName, file, sp.getLine(), spType, isLocal, isDefinition,
              sp.getScopeLine(), flags, isOptimized, TParamsArray, decl);
      }
      FuncIDToDISP[funcSPIRVId] = diSP;
      return addMDNode(inst, diSP);
  }

  DIScope* createLexicalBlock(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIScope*>(inst))
          return n;

      OpDebugLexicalBlock lb(inst);

      auto scope = createScope(BM->get<SPIRVExtInst>(lb.getParent()));
      auto file = getDIFile(BM->get<SPIRVExtInst>(lb.getSource()));

      if (!scope || isa<DIFile>(scope))
          return nullptr;

      if(lb.hasNameSpace() || isa<DICompileUnit>(scope))
          return addMDNode(inst, Builder.createNameSpace(scope, lb.getNameSpace()->getStr(), file, lb.getLine(), false));

      return addMDNode(inst, Builder.createLexicalBlock(scope, file, lb.getLine(), lb.getColumn()));
  }

  DILexicalBlockFile* createLexicalBlockDiscriminator(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DILexicalBlockFile*>(inst))
          return n;

      OpDebugLexicalBlkDiscriminator lbDisc(inst);

      auto scope = createScope(BM->get<SPIRVExtInst>(lbDisc.getParent()));
      auto file = getDIFile(BM->get<SPIRVExtInst>(lbDisc.getSource()));
      auto disc = lbDisc.getDiscriminator();

      return addMDNode(inst, Builder.createLexicalBlockFile(scope, file, disc));
  }

  DILocation* createInlinedAt(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DILocation*>(inst))
          return n;



      OpDebugInlinedAt inlinedAt(inst);
      DILocation* iat = nullptr;

      auto line = inlinedAt.getLine();
      auto scope = createScope(BM->get<SPIRVExtInst>(inlinedAt.getScope()));
      if(inlinedAt.inlinedAtPresent())
        iat = createInlinedAt(BM->get<SPIRVExtInst>(inlinedAt.getInlinedAt()));

      return addMDNode(inst, createLocation(line, 0, scope, iat));
  }

  // Encode a line number, a file name and a directory to a string, where a line no., file name
  // and directory are separated by '?' character: lineNumber?fileName?directory
  // There is a workaround for DIModule creation in earlier LLVM versions, where a line and a file
  // parameters are not supported in DIBuilder.
  void encodeLineAndFileForISysRoot(unsigned int line, llvm::DIFile* file, std::string& lineAndFile)
  {
#if LLVM_VERSION_MAJOR < 11
      lineAndFile.append(std::to_string(line));
      lineAndFile.append("?");
      if (file)
      {
          lineAndFile.append(file->getDirectory().str());
          lineAndFile.append("?");
          lineAndFile.append(file->getFilename().str());
      }
#endif  // LLVM_VERSION_MAJOR < 11.
  }

  DIModule* createModuleINTEL(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DIModule*>(inst))
          return n;

      OpDebugModuleINTEL moduleINTEL(inst);

      auto scope = createScope(BM->get<SPIRVExtInst>(moduleINTEL.getParent()));
      auto& name = BM->get<SPIRVString>(moduleINTEL.getName())->getStr();
      auto cfgMacros = BM->get<SPIRVString>(moduleINTEL.getConfigurationMacros())->getStr();
      auto inclPath = BM->get<SPIRVString>(moduleINTEL.getIncludePath())->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(moduleINTEL.getSource()));
      auto line = moduleINTEL.getLine();
      auto apiNotesFile = BM->get<SPIRVString>(moduleINTEL.getAPINotesFile())->getStr();
      bool isDecl = moduleINTEL.getIsDecl() ? true : false;

      std::string encodedLineAndFile = "";
      encodeLineAndFileForISysRoot(line, file, encodedLineAndFile);
      llvm::StringRef iSysRoot = StringRef(encodedLineAndFile);

      DIModule* diModule = addMDNode(inst, Builder.createModule(scope, name, cfgMacros, inclPath, apiNotesFile, file, line, isDecl, iSysRoot));

      return diModule;
  }

  DINode* createImportedEntity(SPIRVExtInst* inst, SmallVector<TrackingMDNodeRef, 4>& allImportedModules)
  {
      if (auto n = getExistingNode<DINode*>(inst))
          return n;

      OpDebugImportedEntity importedEntity(inst);

      auto scope = createScope(BM->get<SPIRVExtInst>(importedEntity.getParent()));
      auto file = getDIFile(BM->get<SPIRVExtInst>(importedEntity.getSource()));
      auto line = importedEntity.getLine();
      auto entity = importedEntity.getEntity();

      DINode* diNode = nullptr;

      if (BM->get<SPIRVExtInst>(entity)->getExtOp() == OCLExtOpDbgKind::ModuleINTEL)
      {
          auto* diModule = createModuleINTEL(BM->get<SPIRVExtInst>(entity));
          IGC_ASSERT(diModule);
          DIImportedEntity* IE = Builder.createImportedModule(scope, diModule, file, line);
          diNode = addMDNode(inst, IE);

          IGC_ASSERT_MESSAGE(scope, "Invalid Scope encoding!");
          if (isa<DILocalScope>(scope))
          {
              allImportedModules.emplace_back(IE);
          }
          else
          {
              // TODO? ReplaceImportedEntities = true;
          }
      }

      return diNode;
  }

  DIScope* createScope(SPIRVExtInst* inst)
  {
      if (!inst)
          return nullptr;

      if (inst->isString())
      {
          // Treat inst as a SPIRVInstruction instead of SPIRVExtInst
          // This is a WA since SPIRV emitter emits OpString as
          // scope of DebugFunction ext opcode.
          return getDIFile(((SPIRVString*)inst)->getStr());
      }

      if (inst->getExtOp() == OCLExtOpDbgKind::Scope)
      {
          OpDebugScope scope(inst);
          return createScope(BM->get<SPIRVExtInst>(scope.getScope()));
      }

      if (inst->getExtOp() == OCLExtOpDbgKind::Function)
      {
          return createFunction(inst);
      }
      else if (inst->getExtOp() == OCLExtOpDbgKind::LexicalBlock)
      {
          return createLexicalBlock(inst);
      }
      else if (inst->getExtOp() == OCLExtOpDbgKind::LexicalBlockDiscriminator)
      {
          return createLexicalBlockDiscriminator(inst);
      }
      else if (inst->getExtOp() == OCLExtOpDbgKind::CompileUnit)
      {
          return createCompileUnit();
      }
      else if (inst->getExtOp() == OCLExtOpDbgKind::TypeComposite ||
          inst->getExtOp() == OCLExtOpDbgKind::TypeTemplate)
      {
          return createType(inst);
      }
      else if (inst->getExtOp() == OCLExtOpDbgKind::ModuleINTEL)
      {
          return createModuleINTEL(inst);
      }
      else
      {
          return getDIFile("unexpected path in getScope()");
      }
      return nullptr;
  }

  DILocation* getInlinedAtFromScope(SPIRVExtInst* inst)
  {
      if (inst->getExtOp() == OCLExtOpDbgKind::Scope)
      {
          OpDebugScope scope(inst);

          if (!scope.hasInlinedAt())
              return nullptr;

          return createInlinedAt(BM->get<SPIRVExtInst>(scope.getInlinedAt()));
      }

      return nullptr;
  }

  DILocalVariable* createInlinedLocalVar(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DILocalVariable*>(inst))
          return n;

      OpDebugInlinedLocalVar var(inst);

      auto origVar = createLocalVar(BM->get<SPIRVExtInst>(var.getVar()));
      //auto inlinedAt = createInlinedAt(BM->get<SPIRVExtInst>(var.getInlinedAt()));

      return addMDNode(inst, origVar);
  }

  DILocalVariable* createLocalVar(SPIRVExtInst* inst)
  {
      if (auto n = getExistingNode<DILocalVariable*>(inst))
          return n;

      OpDebugLocalVar var(inst);
      auto scope = createScope(BM->get<SPIRVExtInst>(var.getParent()));
      auto& name = BM->get<SPIRVString>(var.getName())->getStr();
      auto file = getDIFile(BM->get<SPIRVExtInst>(var.getSource()));
      auto type = createType(BM->get<SPIRVExtInst>(var.getType()));
      auto spirvFlags = var.getFlags();
      auto line = var.getLine();

      auto flags = decodeFlag(spirvFlags);

      if (var.isParamVar())
      {
          return addMDNode(inst, Builder.createParameterVariable(scope, name, var.getArgNo(), file, line, type, false, flags));
      }
      else
      {
          return addMDNode(inst, Builder.createAutoVariable(scope, name, file, line, type, false, flags));
      }
  }

  DIGlobalVariableExpression* createGlobalVar(SPIRVExtInst* inst);

  DILocation* createLocation(SPIRVWord line, SPIRVWord column, DIScope* scope, DILocation* inlinedAt = nullptr)
  {
      if (scope && isa<DIFile>(scope))
          return nullptr;

      return DILocation::get(M->getContext(), (unsigned int)line, (unsigned int)column, scope, inlinedAt);
  }

  Instruction* createDbgDeclare(SPIRVExtInst* inst, Value* localVar, BasicBlock* insertAtEnd)
  {
      // Format
      // 8  12   <id>  Result Type  Result <id>  <id> Set  28  <id> Local Variable <id> Variable  <id> Expression
      OpDebugDeclare dbgDcl(inst);

      auto scope = createScope(inst->getDIScope());
      auto iat = getInlinedAtFromScope(inst->getDIScope());
      if (!scope)
          return nullptr;
      auto dbgDclInst = Builder.insertDeclare(localVar,
          createLocalVar(BM->get<SPIRVExtInst>(dbgDcl.getVar())),
          createExpression(BM->get<SPIRVExtInst>(dbgDcl.getExpression())),
          createLocation(inst->getLine()->getLine(), inst->getLine()->getColumn(), scope, iat),
          insertAtEnd);
      return dbgDclInst;
  }

  Instruction* createDbgValue(SPIRVExtInst* inst, Value* localVar, BasicBlock* insertAtEnd)
  {
      OpDebugValue dbgValue(inst);

      auto scope = createScope(inst->getDIScope());
      auto iat = getInlinedAtFromScope(inst->getDIScope());
      if (!scope)
          return nullptr;
      auto dbgValueInst = Builder.insertDbgValueIntrinsic(localVar, 0,
          createLocalVar(BM->get<SPIRVExtInst>(dbgValue.getVar())),
          createExpression(BM->get<SPIRVExtInst>(dbgValue.getExpression())),
          createLocation(inst->getLine()->getLine(), inst->getLine()->getColumn(), scope, iat),
          insertAtEnd);

      return dbgValueInst;
  }

  void transGlobals()
  {
      if (!Enable)
          return;

      auto globalVars = BM->getGlobalVars();

      for (auto& gvar : globalVars)
      {
          (void)createGlobalVar(gvar);
      }
  }

  void transImportedEntities()
  {
      if (!Enable)
          return;

      DICompileUnit* CU = getCompileUnit();

      auto importedEntities = BM->getImportedEntities();

      SmallVector<TrackingMDNodeRef, 4> AllImportedModules;
      for (auto& importedEntity : importedEntities)
          (void)createImportedEntity(importedEntity, AllImportedModules);


      if (!AllImportedModules.empty())
          CU->replaceImportedEntities(
              MDTuple::get(CU->getContext(), SmallVector<Metadata*, 16>(AllImportedModules.begin(), AllImportedModules.end())));
      else
          CU->replaceImportedEntities(nullptr);  // If there were no local scope imported entities, we can map the whole list to nullptr.
  }

  void transDbgInfo(SPIRVValue *SV, Value *V);

  void finalize() {
    if (!Enable)
      return;
    Builder.finalize();
  }

  template<typename T>
  T getExistingNode(SPIRVInstruction* inst)
  {
      auto it = MDMap.find(inst);
      if (it != MDMap.end())
          return (T)it->second;
      return nullptr;
  }

  template<typename T>
  T addMDNode(SPIRVInstruction* inst, T node)
  {
      MDMap[inst] = node;
      return node;
  }

  DISubprogram* getDISP(SPIRVId id)
  {
      auto it = FuncIDToDISP.find(id);
      if (it != FuncIDToDISP.end())
          return (*it).second;
      return nullptr;
  }

private:
  SPIRVModule *BM;
  Module *M;
  SPIRVDbgInfo SpDbg;
  IGCLLVM::DIBuilder Builder;
  bool Enable;
  DICompileUnit* cu = nullptr;
  SPIRVToLLVM* SPIRVTranslator = nullptr;
  std::unordered_map<std::string, DIFile*> FileMap;
  std::unordered_map<Function *, DISubprogram*> FuncMap;
  std::unordered_map<SPIRVInstruction*, MDNode*> MDMap;
  std::unordered_map<SPIRVId, DISubprogram*> FuncIDToDISP;

  DICompileUnit* getCompileUnit() { return cu; }

  void splitFileName(const std::string &FileName,
      std::string &BaseName,
      std::string &Path) {
    auto Loc = FileName.find_last_of("/\\");
    if (Loc != std::string::npos) {
      BaseName = FileName.substr(Loc + 1);
      Path = FileName.substr(0, Loc);
    } else {
      BaseName = FileName;
      Path = ".";
    }
  }
};

class SPIRVToLLVM {
public:
  SPIRVToLLVM(Module *LLVMModule, SPIRVModule *TheSPIRVModule)
    :M((IGCLLVM::Module*)LLVMModule), BM(TheSPIRVModule), DbgTran(BM, M, this){
      if (M)
          Context = &M->getContext();
      else
          Context = NULL;
  }

  Type *transType(SPIRVType *BT);
  GlobalValue::LinkageTypes transLinkageType(const SPIRVValue* V);
  /// Decode SPIR-V encoding of vector type hint execution mode.
  Type *decodeVecTypeHint(LLVMContext &C, unsigned code);
  std::string transTypeToOCLTypeName(SPIRVType *BT, bool IsSigned = true);
  std::vector<Type *> transTypeVector(const std::vector<SPIRVType *>&);
  bool translate();
  bool transAddressingModel();

  enum class BoolAction
  {
      Promote,
      Truncate,
      Noop
  };

  Value *transValue(SPIRVValue *, Function *F, BasicBlock *,
      bool CreatePlaceHolder = true, BoolAction Action = BoolAction::Promote);
  Value *transValueWithoutDecoration(SPIRVValue *, Function *F, BasicBlock *,
      bool CreatePlaceHolder);
  bool transDecoration(SPIRVValue *, Value *);
  bool transAlign(SPIRVValue *, Value *);
  Instruction *transOCLBuiltinFromExtInst(SPIRVExtInst *BC, BasicBlock *BB);
  std::vector<Value *> transValue(const std::vector<SPIRVValue *>&, Function *F,
      BasicBlock *, BoolAction Action = BoolAction::Promote);
  Function *transFunction(SPIRVFunction *F);
  bool transFPContractMetadata();
  bool transKernelMetadata();
  bool transNonTemporalMetadata(Instruction* I);
  void transCapsIntoMetadata(IGC::ModuleMetaData& MD);
  template <typename SPIRVInstType>
  void transAliasingMemAccess(SPIRVInstType* BI, Instruction* I);
  void addMemAliasMetadata(Instruction* I, SPIRVId AliasListId,
      uint32_t AliasMDKind);
  void transSourceLanguage();
  bool transSourceExtension();
  /*InlineAsm*/ Value *transAsmINTEL(SPIRVAsmINTEL *BA, Function *F,
                                     BasicBlock *BB);
  CallInst *transAsmCallINTEL(SPIRVAsmCallINTEL *BI, Function *F,
                              BasicBlock *BB);

  Type* m_NamedBarrierType = nullptr;
  Type* getNamedBarrierType();
  Value *transConvertInst(SPIRVValue* BV, Function* F, BasicBlock* BB);
  Instruction *transSPIRVBuiltinFromInst(SPIRVInstruction *BI, BasicBlock *BB);
  void transOCLVectorLoadStore(std::string& UnmangledName,
      std::vector<SPIRVWord> &BArgs);

  /// Post-process translated LLVM module for OpenCL.
  bool postProcessOCL();

  Instruction* transDebugInfo(SPIRVExtInst*, llvm::BasicBlock*);
  SPIRVToLLVMDbgTran& getDbgTran() { return DbgTran; }

  /// \brief Post-process OpenCL builtin functions returning struct type.
  ///
  /// Some OpenCL builtin functions are translated to SPIR-V instructions with
  /// struct type result, e.g. NDRange creation functions. Such functions
  /// need to be post-processed to return the struct through sret argument.
  bool postProcessFunctionsReturnStruct(Function *F);

  /// \brief Post-process OpenCL builtin functions having aggregate argument.
  ///
  /// These functions are translated to functions with aggregate type argument
  /// first, then post-processed to have pointer arguments.
  bool postProcessFunctionsWithAggregateArguments(Function *F);

  typedef DenseMap<SPIRVType *, Type *> SPIRVToLLVMTypeMap;
  typedef DenseMap<SPIRVValue *, Value *> SPIRVToLLVMValueMap;
  typedef DenseMap<SPIRVFunction *, Function *> SPIRVToLLVMFunctionMap;
  typedef DenseMap<GlobalVariable *, SPIRVBuiltinVariableKind> BuiltinVarMap;
  typedef std::unordered_map<SPIRVId, MDNode*> SPIRVToLLVMMDAliasInstMap;

  // A SPIRV value may be translated to a load instruction of a placeholder
  // global variable. This map records load instruction of these placeholders
  // which are supposed to be replaced by the real values later.
  typedef std::map<SPIRVValue *, LoadInst*> SPIRVToLLVMPlaceholderMap;

  typedef std::map<const BasicBlock*, const SPIRVValue*>
      SPIRVToLLVMLoopMetadataMap;

  Value *getTranslatedValue(SPIRVValue *BV);

private:
  IGCLLVM::Module *M;
  BuiltinVarMap BuiltinGVMap;
  LLVMContext *Context;
  SPIRVModule *BM;
  SPIRVToLLVMTypeMap TypeMap;
  SPIRVToLLVMValueMap ValueMap;
  SPIRVToLLVMFunctionMap FuncMap;
  SPIRVToLLVMPlaceholderMap PlaceholderMap;
  SPIRVToLLVMDbgTran DbgTran;
  GlobalVariable *m_NamedBarrierVar;
  GlobalVariable *m_named_barrier_id;
  DICompileUnit* compileUnit = nullptr;

  // These storages are used to prevent duplication of alias.scope/noalias
  // metadata
  SPIRVToLLVMMDAliasInstMap MDAliasDomainMap;
  SPIRVToLLVMMDAliasInstMap MDAliasScopeMap;
  SPIRVToLLVMMDAliasInstMap MDAliasListMap;

  // Loops metadata is translated in the end of a function translation.
  // This storage contains pairs of translated loop header basic block and loop
  // metadata SPIR-V instruction in SPIR-V representation of this basic block.
  SPIRVToLLVMLoopMetadataMap FuncLoopMetadataMap;

  Type *mapType(SPIRVType *BT, Type *T) {
    TypeMap[BT] = T;
    return T;
  }

  // If a value is mapped twice, the existing mapped value is a placeholder,
  // which must be a load instruction of a global variable whose name starts
  // with kPlaceholderPrefix.
  Value *mapValue(SPIRVValue *BV, Value *V) {
    auto Loc = ValueMap.find(BV);
    if (Loc != ValueMap.end()) {
      if (Loc->second == V)
        return V;
      auto LD = dyn_cast<LoadInst>(Loc->second);
      IGC_ASSERT_EXIT(nullptr != LD);
      auto Placeholder = dyn_cast<GlobalVariable>(LD->getPointerOperand());
      IGC_ASSERT_EXIT(nullptr != Placeholder);
      IGC_ASSERT_EXIT_MESSAGE(Placeholder->getName().startswith(kPlaceholderPrefix), "A value is translated twice");
      // Replaces placeholders for PHI nodes
      LD->replaceAllUsesWith(V);
      LD->eraseFromParent();
      Placeholder->eraseFromParent();
    }
    ValueMap[BV] = V;
    return V;
  }

  bool isSPIRVBuiltinVariable(GlobalVariable *GV,
      SPIRVBuiltinVariableKind *Kind = nullptr) {
    auto Loc = BuiltinGVMap.find(GV);
    if (Loc == BuiltinGVMap.end())
      return false;
    if (Kind)
      *Kind = Loc->second;
    return true;
  }
  // OpenCL function always has NoUnwound attribute.
  // Change this if it is no longer true.
  bool isFuncNoUnwind() const { return true;}
  bool isSPIRVCmpInstTransToLLVMInst(SPIRVInstruction *BI) const;
  bool transOCLBuiltinsFromVariables();
  bool transOCLBuiltinFromVariable(GlobalVariable *GV,
      SPIRVBuiltinVariableKind Kind);
  MDString *transOCLKernelArgTypeName(SPIRVFunctionParameter *);

  Value *mapFunction(SPIRVFunction *BF, Function *F) {
    FuncMap[BF] = F;
    return F;
  }

  Type *getTranslatedType(SPIRVType *BT);

  SPIRVErrorLog &getErrorLog() {
    return BM->getErrorLog();
  }

  void setCallingConv(CallInst *Call) {
    Function *F = Call->getCalledFunction();
    Call->setCallingConv(F->getCallingConv());
  }

  void setAttrByCalledFunc(CallInst *Call);
  Type *transFPType(SPIRVType* T);
  BinaryOperator *transShiftLogicalBitwiseInst(SPIRVValue* BV, BasicBlock* BB,
      Function* F);
  Instruction *transCmpInst(SPIRVValue* BV, BasicBlock* BB, Function* F);
  Instruction *transLifetimeInst(SPIRVInstTemplateBase* BV, BasicBlock* BB, Function* F);
  uint64_t calcImageType(const SPIRVValue *ImageVal);
  std::string transOCLImageTypeName(igc_spv::SPIRVTypeImage* ST);
  std::string transOCLSampledImageTypeName(igc_spv::SPIRVTypeSampledImage* ST);
  std::string transOCLImageTypeAccessQualifier(igc_spv::SPIRVTypeImage* ST);
  std::string transOCLPipeTypeAccessQualifier(igc_spv::SPIRVTypePipe* ST);
  std::string transOCLPipeTypeName(igc_spv::SPIRVTypePipe* PT, SPIRVAccessQualifierKind PipeAccess);
  std::string transVMEImageTypeName(igc_spv::SPIRVTypeVmeImageINTEL* VT);

  Value *oclTransConstantSampler(igc_spv::SPIRVConstantSampler* BCS, BasicBlock *BB);

  template<class Source, class Func>
  bool foreachFuncCtlMask(Source, Func);

  template <typename LoopInstType>
  void setLLVMLoopMetadata(const LoopInstType* LM, Instruction* BI);
  void transLLVMLoopMetadata(const Function* F);
  inline llvm::Metadata *getMetadataFromName(std::string Name);
  inline std::vector<llvm::Metadata *>
    getMetadataFromNameAndParameter(std::string Name, SPIRVWord Parameter);

  Value *promoteBool(Value *pVal, BasicBlock *BB);
  Value *truncBool(Value *pVal, BasicBlock *BB);
  Type  *truncBoolType(SPIRVType *SPVType, Type *LLType);

  void transMemAliasingINTELDecorations(SPIRVValue* BV, Value* V);
  void transHostAccessINTELDecorations(SPIRVValue* BV, Value* V);
};

DIGlobalVariableExpression* SPIRVToLLVMDbgTran::createGlobalVar(SPIRVExtInst* inst)
{
    if (auto n = getExistingNode<DIGlobalVariableExpression*>(inst))
        return n;

    OpDebugGlobalVar var(inst);
    auto ctxt = createScope(BM->get<SPIRVExtInst>(var.getParent()));
    auto& name = var.getName()->getStr();
    auto& linkageName = var.getLinkageName()->getStr();
    auto file = getDIFile(BM->get<SPIRVExtInst>(var.getSource()));
    auto type = createType(BM->get<SPIRVExtInst>(var.getType()));
    auto varValue = static_cast<SPIRVValue*>(BM->getEntry(var.getVariable()));
    auto flags = var.getFlags();
    bool isLocal = (flags & SPIRVDebug::Flag::FlagIsLocal) ? true : false;

    auto globalVarMD = addMDNode(inst, Builder.createGlobalVariableExpression(ctxt, name, linkageName, file, var.getLine(), type, isLocal));

    llvm::Value* llvmValue = nullptr;
    if (varValue)
    {
        llvmValue = SPIRVTranslator->getTranslatedValue(varValue);

        if (llvmValue && dyn_cast_or_null<GlobalVariable>(llvmValue))
        {
            dyn_cast<GlobalVariable>(llvmValue)->addDebugInfo(globalVarMD);
        }
    }

    return globalVarMD;
}

void SPIRVToLLVMDbgTran::transDbgInfo(SPIRVValue *SV, Value *V) {
    if (!SV)
        return;

    if (!Enable || !SV->hasLine() || !SV->getDIScope())
        return;
    if (auto I = dyn_cast<Instruction>(V)) {
        IGC_ASSERT_MESSAGE(SV->isInst(), "Invalid instruction");
        auto SI = static_cast<SPIRVInstruction *>(SV);
        IGC_ASSERT(nullptr != SI);
        IGC_ASSERT_MESSAGE(nullptr != SI->getParent(), "Invalid instruction");
        IGC_ASSERT_MESSAGE(nullptr != SI->getParent()->getParent(), "Invalid instruction");
        auto Line = SV->getLine();
        DILocation* iat = nullptr;
        DIScope* scope = nullptr;
        if (SV->getDIScope())
        {
            scope = SPIRVTranslator->getDbgTran().createScope(SV->getDIScope());
            iat = SPIRVTranslator->getDbgTran().getInlinedAtFromScope(SV->getDIScope());
        }

        SPIRVTranslator->getDbgTran().createLocation(Line->getLine(),
            Line->getColumn(), scope, iat);

        if(scope && !isa<DIFile>(scope))
            I->setDebugLoc(DILocation::get(scope->getContext(), Line->getLine(), Line->getColumn(),
                scope, iat));
    }
}

DIType* SPIRVToLLVMDbgTran::createMember(SPIRVExtInst* inst)
{
    if (auto n = getExistingNode<DIType*>(inst))
        return n;

    OpDebugTypeMember typeMember(inst);

    auto scope = createType(BM->get<SPIRVExtInst>(typeMember.getParent()));
    auto& name = typeMember.getName()->getStr();
    auto file = getDIFile(BM->get<SPIRVExtInst>(typeMember.getSource()));
    auto line = typeMember.getLine();
    auto size = typeMember.getSize();
    auto offset = typeMember.getOffset();
    auto flagRaw = typeMember.getFlags();
    auto type = createType(BM->get<SPIRVExtInst>(typeMember.getType()));

    auto flags = decodeFlag(flagRaw);

    if (flags & DINode::FlagStaticMember && typeMember.hasInitConst())
    {
        SPIRVValue* constVal = (SPIRVValue*)BM->getEntry(typeMember.getInitConstId());
        auto val = SPIRVTranslator->transValue(constVal, nullptr, nullptr);
        return addMDNode(inst, Builder.createStaticMemberType(scope, name, file, line, type, flags,
            llvm::cast<llvm::Constant>(val)));
    }

    return addMDNode(inst, Builder.createMemberType(scope, name, file, line, size, 0, offset, flags, type));
}

Type *
SPIRVToLLVM::getTranslatedType(SPIRVType *BV){
  auto Loc = TypeMap.find(BV);
  if (Loc != TypeMap.end())
    return Loc->second;
  return nullptr;
}

Value *
SPIRVToLLVM::getTranslatedValue(SPIRVValue *BV){
  auto Loc = ValueMap.find(BV);
  if (Loc != ValueMap.end())
    return Loc->second;
  return nullptr;
}

void
SPIRVToLLVM::setAttrByCalledFunc(CallInst *Call) {
  Function *F = Call->getCalledFunction();
  if (F->isIntrinsic()) {
    return;
  }
  Call->setCallingConv(F->getCallingConv());
  Call->setAttributes(F->getAttributes());
}

// Translate aliasing decorations applied to instructions. These decorations
// are mapped on alias.scope and noalias metadata in LLVM. Translation of
// optional string operand isn't yet supported in the translator.
void SPIRVToLLVM::transMemAliasingINTELDecorations(SPIRVValue* BV, Value* V) {
  if (!BV->isInst())
    return;
  Instruction* Inst = dyn_cast<Instruction>(V);
  if (!Inst)
    return;
  if (BV->hasDecorateId(DecorationAliasScopeINTEL)) {
    std::vector<SPIRVId> AliasListIds;
    AliasListIds =
      BV->getDecorationIdLiterals(DecorationAliasScopeINTEL);
    IGC_ASSERT_MESSAGE(AliasListIds.size() == 1,
      "Memory aliasing decorations must have one argument");
    addMemAliasMetadata(Inst, AliasListIds[0], LLVMContext::MD_alias_scope);
  }
  if (BV->hasDecorateId(DecorationNoAliasINTEL)) {
    std::vector<SPIRVId> AliasListIds;
    AliasListIds =
      BV->getDecorationIdLiterals(DecorationNoAliasINTEL);
    IGC_ASSERT_MESSAGE(AliasListIds.size() == 1,
      "Memory aliasing decorations must have one argument");
    addMemAliasMetadata(Inst, AliasListIds[0], LLVMContext::MD_noalias);
  }
}

void SPIRVToLLVM::transHostAccessINTELDecorations(SPIRVValue* BV, Value* V) {
  if (auto GV = dyn_cast<GlobalVariable>(V)) {
    if (BV->hasDecorate(DecorationHostAccessINTEL)) {
      std::vector<SPIRVDecorate const*> Decorates = BV->getDecorations(DecorationHostAccessINTEL);
      IGC_ASSERT(Decorates.size() == 1);
      const auto* const HostAccDeco =
        static_cast<const SPIRVDecorateHostAccessINTEL*>(Decorates[0]);
      GV->addAttribute("host_var_name", HostAccDeco->getVarName());
    }
  }
}

SPIRAddressSpace
getOCLOpaqueTypeAddrSpace(Op OpCode)
{
    switch (OpCode)
    {
    case OpTypePipe:
        // these types are handled in special way at SPIRVToLLVM::transType
        return SPIRAS_Global;
    case OpConstantSampler:
    case OpTypeSampler:
        return SPIRAS_Constant;
    default:
        //OpTypeQueue:
        //OpTypeEvent:
        //OpTypeDeviceEvent:
        //OpTypeReserveId
        return SPIRAS_Private;
    }
}

bool
SPIRVToLLVM::transOCLBuiltinsFromVariables(){
  std::vector<GlobalVariable *> WorkList;
  for (auto I = M->global_begin(), E = M->global_end(); I != E; ++I) {
    SPIRVBuiltinVariableKind Kind = BuiltInCount;
    if (!isSPIRVBuiltinVariable(&(*I), &Kind))
      continue;
    if (!transOCLBuiltinFromVariable(&(*I), Kind))
      return false;
    WorkList.push_back(&(*I));
  }
  for (auto &I:WorkList) {
    I->eraseFromParent();
  }
  return true;
}

// For integer types shorter than 32 bit, unsigned/signedness can be inferred
// from zext/sext attribute.
MDString *
SPIRVToLLVM::transOCLKernelArgTypeName(SPIRVFunctionParameter *Arg) {
  auto Ty = Arg->isByVal() ? Arg->getType()->getPointerElementType() :
    Arg->getType();
  return MDString::get(*Context, transTypeToOCLTypeName(Ty, !Arg->isZext()));
}

//
// The function performs a DFS on an instruction graph (dir: instruction -> users).
// In each node, it checks if the instruction type is the same as the corresponding
// type passed as a template parameter and then runs the callback. The deep of search
// is determined by the number of template parameters (not counting Callback type).
// The function returns true if and only if all types match. Pattern types should be
// passed in the same order as the instructions in IR.
//
template <typename PatternTypeFirst, typename Callback>
bool checkAndProcessInstructionPattern(Instruction* instruction, Callback callback) {
  if (auto inst = dyn_cast<PatternTypeFirst>(instruction)) {
    callback(inst);
    return true;
  }
  return false;
}

template <typename PatternTypeFirst, typename PatternTypeSecond, typename... PatternTypeRest, typename Callback>
bool checkAndProcessInstructionPattern(Instruction* instruction, Callback callback) {
  if (auto inst = dyn_cast<PatternTypeFirst>(instruction)) {
    for (auto user : inst->users()) {
      if (!checkAndProcessInstructionPattern<PatternTypeSecond, PatternTypeRest...>(cast<Instruction>(&*user), callback)) {
        return false;
      }
    }
    callback(inst);
    return true;
  }
  return false;
}

// Variable like GlobalInvocationId[x] -> get_global_id(x).
// Variable like WorkDim -> get_work_dim().
//
// Supported patterns:
// 1. Load(GlobalVariable)
// 2. Load(AddrSpaceCastInst(GlobalVariable))
// 3. Load(GetElementPtr(AddrSpaceCastInst(GlobalVariable)))
bool SPIRVToLLVM::transOCLBuiltinFromVariable(GlobalVariable *GV,
                                              SPIRVBuiltinVariableKind Kind) {
  std::string FuncName;
  if (!SPIRSPIRVBuiltinVariableMap::find(Kind, &FuncName))
    return false;

  decorateSPIRVBuiltin(FuncName);
  Function *Func = M->getFunction(FuncName);
  if (!Func) {
    Type *ReturnTy = GV->getType()->getPointerElementType();
    FunctionType *FT = FunctionType::get(ReturnTy, false);
    Func = Function::Create(FT, GlobalValue::ExternalLinkage, FuncName, M);
    Func->setCallingConv(CallingConv::SPIR_FUNC);
    Func->addFnAttr(Attribute::NoUnwind);
    Func->addFnAttr(Attribute::ReadNone);
  }

  SmallVector<Instruction *, 4> Deletes;
  SmallVector<Instruction *, 4> Users;
  auto addToDeletesOrUsers = [&Deletes, &Users](Instruction* inst) {
    if (auto load = dyn_cast<LoadInst>(&*inst)) {
      Users.push_back(load);
    }
    Deletes.push_back(inst);
  };

  for (auto user : GV->users()) {
    auto inst = cast<Instruction>(&*user);

    if (checkAndProcessInstructionPattern<LoadInst>(inst, addToDeletesOrUsers))
      continue;
    if (checkAndProcessInstructionPattern<AddrSpaceCastInst, LoadInst>(inst, addToDeletesOrUsers))
      continue;
    if (checkAndProcessInstructionPattern<AddrSpaceCastInst, GetElementPtrInst, LoadInst>(inst, addToDeletesOrUsers))
      continue;

    IGC_ASSERT_MESSAGE(false, "Unknown pattern");
  }

  for (auto& user : Users) {
    auto load = cast<LoadInst>(user);

    auto call = CallInst::Create(Func, "", load);
    call->takeName(load);
    call->setDebugLoc(load->getDebugLoc());
    setAttrByCalledFunc(call);

    Value* substitution = call;
    // Check if an ExtractElement instruction is needed.
    if (Func->getFunctionType()->getReturnType()->isVectorTy() &&
        !load->getPointerOperandType()->getPointerElementType()->isVectorTy()) {
      auto gep = cast<GetElementPtrInst>(load->getOperand(0));
      IGC_ASSERT_MESSAGE(gep, "Expected GetElementrPtr instruction");
      IGC_ASSERT_MESSAGE(gep->getNumIndices() == 2, "Expected GetElementrPtr with exactly two indices");
      auto offset = cast<Value>(gep->idx_begin()[1]);
      substitution = ExtractElementInst::Create(substitution, offset, "", load);
    }
    load->replaceAllUsesWith(substitution);
  }
  for (auto &I : Deletes) {
    if (I->use_empty())
      I->eraseFromParent();
    else
      IGC_ASSERT(0);
  }

  return true;
}

Type *
SPIRVToLLVM::transFPType(SPIRVType* T) {
  switch(T->getFloatBitWidth()) {
  case 16: return Type::getHalfTy(*Context);
  case 32: return Type::getFloatTy(*Context);
  case 64: return Type::getDoubleTy(*Context);
  default:
    llvm_unreachable("Invalid type");
    return nullptr;
  }
}

std::string
SPIRVToLLVM::transOCLImageTypeName(igc_spv::SPIRVTypeImage* ST) {
  return getSPIRVTypeName(
    kSPIRVTypeName::Image,
    getSPIRVImageTypePostfixes(
      getSPIRVImageSampledTypeName(ST->getSampledType()),
      ST->getDescriptor(),
      ST->hasAccessQualifier()
      ? ST->getAccessQualifier()
      : AccessQualifierReadOnly));
}

std::string
SPIRVToLLVM::transVMEImageTypeName(igc_spv::SPIRVTypeVmeImageINTEL* VT) {
  return getSPIRVTypeName(
    kSPIRVTypeName::VmeImageINTEL,
    getSPIRVImageTypePostfixes(
      getSPIRVImageSampledTypeName(VT->getImageType()->getSampledType()),
      VT->getImageType()->getDescriptor(),
      VT->getImageType()->hasAccessQualifier()
      ? VT->getImageType()->getAccessQualifier()
      : AccessQualifierReadOnly));
}

std::string
SPIRVToLLVM::transOCLSampledImageTypeName(igc_spv::SPIRVTypeSampledImage* ST) {
  return getSPIRVTypeName(
    kSPIRVTypeName::SampledImage,
    getSPIRVImageTypePostfixes(
      getSPIRVImageSampledTypeName(ST->getImageType()->getSampledType()),
      ST->getImageType()->getDescriptor(),
      ST->getImageType()->hasAccessQualifier()
      ? ST->getImageType()->getAccessQualifier()
      : AccessQualifierReadOnly));
}

inline llvm::Metadata *SPIRVToLLVM::getMetadataFromName(std::string Name) {
  return llvm::MDNode::get(*Context, llvm::MDString::get(*Context, Name));
}

inline std::vector<llvm::Metadata *>
SPIRVToLLVM::getMetadataFromNameAndParameter(std::string Name,
                                             SPIRVWord Parameter) {
  return {MDString::get(*Context, Name),
          ConstantAsMetadata::get(
              ConstantInt::get(Type::getInt32Ty(*Context), Parameter))};
}


template <typename LoopInstType>
void SPIRVToLLVM::setLLVMLoopMetadata(const LoopInstType* LM, Instruction* BI) {
  if (!LM)
    return;

  IGC_ASSERT(BI && isa<BranchInst>(BI));

  auto Temp = MDNode::getTemporary(*Context, None);
  auto Self = MDNode::get(*Context, Temp.get());
  Self->replaceOperandWith(0, Self);

  SPIRVWord LC = LM->getLoopControl();
  if (LC == LoopControlMaskNone) {
    BI->setMetadata("llvm.loop", Self);
    return;
  }

  unsigned NumParam = 0;
  std::vector<llvm::Metadata *> Metadata;
  std::vector<SPIRVWord> LoopControlParameters = LM->getLoopControlParameters();
  Metadata.push_back(llvm::MDNode::get(*Context, Self));

  // To correctly decode loop control parameters, order of checks for loop
  // control masks must match with the order given in the spec (see 3.23),
  // i.e. check smaller-numbered bits first.
  // Unroll and UnrollCount loop controls can't be applied simultaneously with
  // DontUnroll loop control.
  if (LC & LoopControlUnrollMask)
    Metadata.push_back(getMetadataFromName("llvm.loop.unroll.enable"));
  else if (LC & LoopControlDontUnrollMask)
    Metadata.push_back(getMetadataFromName("llvm.loop.unroll.disable"));
  if (LC & LoopControlDependencyInfiniteMask)
    Metadata.push_back(getMetadataFromName("llvm.loop.ivdep.enable"));
  if (LC & LoopControlDependencyLengthMask) {
    if (!LoopControlParameters.empty()) {
      Metadata.push_back(llvm::MDNode::get(
          *Context,
          getMetadataFromNameAndParameter("llvm.loop.ivdep.safelen",
                                          LoopControlParameters[NumParam])));
      ++NumParam;
      IGC_ASSERT_MESSAGE(NumParam <= LoopControlParameters.size(), "Missing loop control parameter!");
    }
  }
  // Placeholder for LoopControls added in SPIR-V 1.4 spec (see 3.23)
  if (LC & LoopControlMinIterationsMask) {
    ++NumParam;
    IGC_ASSERT_MESSAGE(NumParam <= LoopControlParameters.size(), "Missing loop control parameter!");
  }
  if (LC & LoopControlMaxIterationsMask) {
    ++NumParam;
    IGC_ASSERT_MESSAGE(NumParam <= LoopControlParameters.size(), "Missing loop control parameter!");
  }
  if (LC & LoopControlIterationMultipleMask) {
    ++NumParam;
    IGC_ASSERT_MESSAGE(NumParam <= LoopControlParameters.size(), "Missing loop control parameter!");
  }
  if (LC & LoopControlPeelCountMask) {
    ++NumParam;
    IGC_ASSERT_MESSAGE(NumParam <= LoopControlParameters.size(), "Missing loop control parameter!");
  }
  if (LC & LoopControlPartialCountMask && !(LC & LoopControlDontUnrollMask)) {
    // If unroll factor is set as '1' - disable loop unrolling
    if (1 == LoopControlParameters[NumParam])
      Metadata.push_back(getMetadataFromName("llvm.loop.unroll.disable"));
    else
      Metadata.push_back(llvm::MDNode::get(
          *Context,
          getMetadataFromNameAndParameter("llvm.loop.unroll.count",
                                          LoopControlParameters[NumParam])));
    ++NumParam;
    IGC_ASSERT_MESSAGE(NumParam <= LoopControlParameters.size(), "Missing loop control parameter!");
  }
  llvm::MDNode *Node = llvm::MDNode::get(*Context, Metadata);

  // Set the first operand to refer itself
  Node->replaceOperandWith(0, Node);
  BI->setMetadata("llvm.loop", Node);
}

void SPIRVToLLVM::transLLVMLoopMetadata(const Function *F) {
  assert(F);

  if (!FuncLoopMetadataMap.empty()) {
    // In SPIRV loop metadata is linked to a header basic block of a loop
    // whilst in LLVM IR it is linked to a latch basic block (the one
    // whose back edge goes to a header basic block) of the loop.

    using Edge = std::pair<const BasicBlock *, const BasicBlock *>;
    SmallVector<Edge, 32> Edges;
    FindFunctionBackedges(*F, Edges);

    for (const auto &BkEdge : Edges) {
      // Check that loop header BB contains loop metadata.
      const auto LMDItr = FuncLoopMetadataMap.find(BkEdge.second);
      if (LMDItr == FuncLoopMetadataMap.end())
        continue;

      auto *BI = const_cast<Instruction *>(BkEdge.first->getTerminator());
      const auto *LMD = LMDItr->second;
      if (LMD->getOpCode() == OpLoopMerge) {
        const auto *LM = static_cast<const SPIRVLoopMerge *>(LMD);
        setLLVMLoopMetadata<SPIRVLoopMerge>(LM, BI);
      } else if (LMD->getOpCode() == OpLoopControlINTEL) {
        const auto *LCI = static_cast<const SPIRVLoopControlINTEL *>(LMD);
        setLLVMLoopMetadata<SPIRVLoopControlINTEL>(LCI, BI);
      }
    }

    // Loop metadata map should be re-filled during each function translation.
    FuncLoopMetadataMap.clear();
  }
}

GlobalValue::LinkageTypes
SPIRVToLLVM::transLinkageType(const SPIRVValue* V) {
  std::string ValueName = V->getName();
  if (ValueName == "llvm.used" || ValueName == "llvm.compiler.used")
    return GlobalValue::AppendingLinkage;
  if (V->getLinkageType() == LinkageTypeInternal) {
    return GlobalValue::InternalLinkage;
  }
  else if (V->getLinkageType() == LinkageTypeImport) {
    // Function declaration
    if (V->getOpCode() == OpFunction) {
      if (static_cast<const SPIRVFunction*>(V)->getNumBasicBlock() == 0)
        return GlobalValue::ExternalLinkage;
    }
    // Variable declaration
    if (V->getOpCode() == OpVariable) {
      if (static_cast<const SPIRVVariable*>(V)->getInitializer() == 0)
        return GlobalValue::ExternalLinkage;
    }
    // Definition
    return GlobalValue::AvailableExternallyLinkage;
  }
  else if (V->getLinkageType() == LinkageTypeLinkOnceODR) {
    return GlobalValue::LinkOnceODRLinkage;
  }
  else {// LinkageTypeExport
    if (V->getOpCode() == OpVariable) {
      if (static_cast<const SPIRVVariable*>(V)->getInitializer() == 0 )
        // Tentative definition
        return GlobalValue::CommonLinkage;
    }
    return GlobalValue::ExternalLinkage;
  }
}

Type *
SPIRVToLLVM::transType(SPIRVType *T) {
  auto Loc = TypeMap.find(T);
  if (Loc != TypeMap.end())
    return Loc->second;

  T->validate();
  switch(T->getOpCode()) {
  case OpTypeVoid:
    return mapType(T, Type::getVoidTy(*Context));
  case OpTypeBool:
    return mapType(T, Type::getInt8Ty(*Context));
  case OpTypeInt:
    return mapType(T, Type::getIntNTy(*Context, T->getIntegerBitWidth()));
  case OpTypeFloat:
    return mapType(T, transFPType(T));
  case OpTypeArray:
    return mapType(T, ArrayType::get(transType(T->getArrayElementType()),
        T->getArrayLength()));
  case OpTypeTokenINTEL:
    return mapType(T, Type::getTokenTy(*Context));
  case OpTypePointer:
    return mapType(T, PointerType::get(transType(T->getPointerElementType()),
      SPIRSPIRVAddrSpaceMap::rmap(T->getPointerStorageClass())));
  case OpTypeVector:
    return mapType(T, IGCLLVM::FixedVectorType::get(transType(T->getVectorComponentType()),
        T->getVectorComponentCount()));
  case OpTypeOpaque:
    return mapType(T, StructType::create(*Context, T->getName()));
  case OpTypeFunction: {
    auto FT = static_cast<SPIRVTypeFunction *>(T);
    auto RT = transType(FT->getReturnType());
    std::vector<Type *> PT;
    for (size_t I = 0, E = FT->getNumParameters(); I != E; ++I)
      PT.push_back(transType(FT->getParameterType(I)));
    return mapType(T, FunctionType::get(RT, PT, false));
    }
  case OpTypeImage:{
   return mapType(T, getOrCreateOpaquePtrType(M,
          transOCLImageTypeName(static_cast<SPIRVTypeImage *>(T))));
  }
  case OpTypeSampledImage: {
    auto ST = static_cast<SPIRVTypeSampledImage*>(T);
    return mapType(
      T, getOrCreateOpaquePtrType(M, transOCLSampledImageTypeName(ST)));
  }
  case OpTypeVmeImageINTEL: {
    auto* VT = static_cast<SPIRVTypeVmeImageINTEL*>(T);
    return mapType(T, getOrCreateOpaquePtrType(M, transVMEImageTypeName(VT)));
  }
  case OpTypeStruct: {
    auto ST = static_cast<SPIRVTypeStruct *>(T);
    auto Name = ST->getName();
    if (Name.empty())
        Name = "structtype";
    auto *pStructTy = StructType::create(*Context, Name);
    mapType(ST, pStructTy);
    SmallVector<Type *, 4> MT;
    for (size_t I = 0, E = ST->getMemberCount(); I != E; ++I)
      MT.push_back(transType(ST->getMemberType(I)));
    for (auto& CI : ST->getContinuedInstructions())
        for (size_t I = 0, E = CI->getNumElements(); I != E; ++I)
            MT.push_back(transType(CI->getMemberType(I)));
    pStructTy->setBody(MT, ST->isPacked());
    return pStructTy;
    }
  case OpTypePipe: {
    auto PT = static_cast<SPIRVTypePipe*>(T);
    return mapType(T, getOrCreateOpaquePtrType(
      M,
      transOCLPipeTypeName(PT, PT->getAccessQualifier()),
      getOCLOpaqueTypeAddrSpace(T->getOpCode())));
  }
  case OpTypePipeStorage:
  {
    return mapType(T, Type::getInt8PtrTy(*Context, SPIRAS_Global));
  }
  case OpTypeNamedBarrier:
  {
    return mapType(T, getNamedBarrierType());
  }
  case OpTypeBufferSurfaceINTEL: {
    return mapType(T,
                   getOrCreateOpaquePtrType(M, "intel.buffer_rw_t",
                                            SPIRAddressSpace::SPIRAS_Global));
  }
  case OpTypeJointMatrixINTEL:
  {
    SPIRVTypeJointMatrixINTEL *MT = static_cast<SPIRVTypeJointMatrixINTEL *>(T);
    std::string typeName = "intel.joint_matrix_" + MT->getMangledName() + "_t";
    return mapType(T, getOrCreateOpaquePtrType(M, typeName, SPIRAddressSpace::SPIRAS_Global));
  }
  default: {
    auto OC = T->getOpCode();
    if (isOpaqueGenericTypeOpCode(OC) ||
        isSubgroupAvcINTELTypeOpCode(OC))
    {
        auto name = isSubgroupAvcINTELTypeOpCode(OC) ?
            SPIRVSubgroupINTELTypeOpCodeMap::rmap(OC) :
            BuiltinOpaqueGenericTypeOpCodeMap::rmap(OC);
        auto *pST = IGCLLVM::getTypeByName(M, name);
        pST = pST ? pST : StructType::create(*Context, name);

        return mapType(T, PointerType::get(pST, getOCLOpaqueTypeAddrSpace(OC)));
    }
    llvm_unreachable("Not implemented");
    }
  }
  return 0;
}

std::string
SPIRVToLLVM::transOCLPipeTypeName(igc_spv::SPIRVTypePipe* PT,
  SPIRVAccessQualifierKind PipeAccess) {
  IGC_ASSERT_MESSAGE((PipeAccess == AccessQualifierReadOnly || PipeAccess == AccessQualifierWriteOnly), "Invalid access qualifier");
  return std::string(kSPIRVTypeName::PrefixAndDelim) + kSPIRVTypeName::Pipe +
    kSPIRVTypeName::Delimiter + kSPIRVTypeName::PostfixDelim +
    PipeAccess;
}

std::string
SPIRVToLLVM::transTypeToOCLTypeName(SPIRVType *T, bool IsSigned) {
  switch(T->getOpCode()) {
  case OpTypeVoid:
    return "void";
  case OpTypeBool:
    return "bool";
  case OpTypeInt: {
    std::string Prefix = IsSigned ? "" : "u";
    switch(T->getIntegerBitWidth()) {
    case 8:
      return Prefix + "char";
    case 16:
      return Prefix + "short";
    case 32:
      return Prefix + "int";
    case 64:
      return Prefix + "long";
    default:
      llvm_unreachable("invalid integer size");
      return Prefix + std::string("int") + T->getIntegerBitWidth() + "_t";
    }
  }
  break;
  case OpTypeFloat:
    switch(T->getFloatBitWidth()){
    case 16:
      return "half";
    case 32:
      return "float";
    case 64:
      return "double";
    default:
      llvm_unreachable("invalid floating pointer bitwidth");
      return std::string("float") + T->getFloatBitWidth() + "_t";
    }
    break;
  case OpTypeArray:
    return "array";
  case OpTypePointer: {
    SPIRVType* ET = T->getPointerElementType();
    if (isa<OpTypeFunction>(ET)) {
      SPIRVTypeFunction* TF = static_cast<SPIRVTypeFunction*>(ET);
      std::string name = transTypeToOCLTypeName(TF->getReturnType());
      name += " (*)(";
      for (unsigned I = 0, E = TF->getNumParameters(); I < E; ++I)
        name += transTypeToOCLTypeName(TF->getParameterType(I)) + ',';
      name.back() = ')'; // replace the last comma with a closing brace.
      return name;
    }
    return transTypeToOCLTypeName(ET) + "*";
  }
  case OpTypeVector:
    return transTypeToOCLTypeName(T->getVectorComponentType()) +
        T->getVectorComponentCount();
  case OpTypeOpaque:
      return T->getName();
  case OpTypeFunction:
    return "function";
  case OpTypeStruct: {
    auto Name = T->getName();
    if (Name.find("struct.") == 0)
      Name[6] = ' ';
    else if (Name.find("union.") == 0)
      Name[5] = ' ';
    return Name;
  }
  case OpTypePipe:
    return "pipe";
  case OpTypeSampler:
    return "sampler_t";
  case OpTypeImage:
    return rmap<std::string>(static_cast<SPIRVTypeImage *>(T)->getDescriptor());
  default:
      if (isOpaqueGenericTypeOpCode(T->getOpCode())) {
        auto Name = BuiltinOpaqueGenericTypeOpCodeMap::rmap(T->getOpCode());
        if (Name.find("opencl.") == 0) {
            return Name.substr(7);
        } else {
            return Name;
        }
      }
      llvm_unreachable("Not implemented");
      return "unknown";
  }
}

std::vector<Type *>
SPIRVToLLVM::transTypeVector(const std::vector<SPIRVType *> &BT) {
  std::vector<Type *> T;
  for (auto I: BT)
    T.push_back(transType(I));
  return T;
}

std::vector<Value *>
SPIRVToLLVM::transValue(const std::vector<SPIRVValue *> &BV, Function *F,
    BasicBlock *BB, BoolAction Action) {
  std::vector<Value *> V;
  for (auto I: BV)
    V.push_back(transValue(I, F, BB, true, Action));
  return V;
}

bool
SPIRVToLLVM::isSPIRVCmpInstTransToLLVMInst(SPIRVInstruction* BI) const {
  auto OC = BI->getOpCode();
  return isCmpOpCode(OC) &&
      !(OC >= OpLessOrGreater && OC <= OpUnordered);
}

Value *
SPIRVToLLVM::transValue(SPIRVValue *BV, Function *F, BasicBlock *BB,
    bool CreatePlaceHolder, BoolAction Action)
{
  auto procBool = [&](Value *v)
  {
      if (Action == BoolAction::Noop)
          return v;

      if (!BV->hasType())
          return v;

      if (!BV->getType()->isTypeVectorOrScalarBool())
          return v;

      return Action == BoolAction::Promote ?
          promoteBool(v, BB) :
          truncBool(v, BB);
  };

  SPIRVToLLVMValueMap::iterator Loc = ValueMap.find(BV);
  if (Loc != ValueMap.end() && (!PlaceholderMap.count(BV) || CreatePlaceHolder))
  {
      return procBool(Loc->second);
  }

  BV->validate();

  auto V = transValueWithoutDecoration(BV, F, BB, CreatePlaceHolder);
  if (!V) {
    return nullptr;
  }
  V->setName(BV->getName());
  if (!transDecoration(BV, V)) {
    IGC_ASSERT_EXIT_MESSAGE(0, "trans decoration fail");
    return nullptr;
  }

  return procBool(V);
}

Value *
SPIRVToLLVM::transConvertInst(SPIRVValue* BV, Function* F, BasicBlock* BB) {
  SPIRVUnary* BC = static_cast<SPIRVUnary*>(BV);
  auto Src = transValue(BC->getOperand(0), F, BB, BB ? true : false);
  auto Dst = transType(BC->getType());
  CastInst::CastOps CO = Instruction::BitCast;
  bool IsExt = Dst->getScalarSizeInBits()
      > Src->getType()->getScalarSizeInBits();
  switch (BC->getOpCode()) {
  case OpPtrCastToGeneric:
  case OpGenericCastToPtr:
    CO = Instruction::AddrSpaceCast;
    break;
  case OpSConvert:
    CO = IsExt ? Instruction::SExt : Instruction::Trunc;
    break;
  case OpUConvert:
    CO = IsExt ? Instruction::ZExt : Instruction::Trunc;
    break;
  case OpFConvert:
    CO = IsExt ? Instruction::FPExt : Instruction::FPTrunc;
    break;
  default:
    CO = static_cast<CastInst::CastOps>(OpCodeMap::rmap(BC->getOpCode()));
  }
  IGC_ASSERT_MESSAGE(CastInst::isCast(CO), "Invalid cast op code");
  if (BB)
    return CastInst::Create(CO, Src, Dst, BV->getName(), BB);
  return ConstantExpr::getCast(CO, dyn_cast<Constant>(Src), Dst);
}

static void applyFPFastMathModeDecorations(const SPIRVValue* BV,
    Instruction* Inst) {
    SPIRVWord V;
    FastMathFlags FMF;
    if (BV->hasDecorate(DecorationFPFastMathMode, 0, &V)) {
        if (V & FPFastMathModeNotNaNMask)
            FMF.setNoNaNs();
        if (V & FPFastMathModeNotInfMask)
            FMF.setNoInfs();
        if (V & FPFastMathModeNSZMask)
            FMF.setNoSignedZeros();
        if (V & FPFastMathModeAllowRecipMask)
            FMF.setAllowReciprocal();
        if (V & FPFastMathModeAllowContractINTELMask)
            FMF.setAllowContract();
        if (V & FPFastMathModeAllowReassocINTELMask)
            FMF.setAllowReassoc();
        if (V & FPFastMathModeFastMask)
            FMF.setFast();
        Inst->setFastMathFlags(FMF);
    }
}

BinaryOperator *SPIRVToLLVM::transShiftLogicalBitwiseInst(SPIRVValue* BV,
    BasicBlock* BB,Function* F) {
  SPIRVBinary* BBN = static_cast<SPIRVBinary*>(BV);
  IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  Instruction::BinaryOps BO;
  auto OP = BBN->getOpCode();
  if (isLogicalOpCode(OP))
    OP = IntBoolOpMap::rmap(OP);
  BO = static_cast<Instruction::BinaryOps>(OpCodeMap::rmap(OP));
  auto Inst = BinaryOperator::Create(BO,
      transValue(BBN->getOperand(0), F, BB),
      transValue(BBN->getOperand(1), F, BB), BV->getName(), BB);

  if (BV->hasDecorate(DecorationNoSignedWrap)) {
    Inst->setHasNoSignedWrap(true);
  }

  if (BV->hasDecorate(DecorationNoUnsignedWrap)) {
    Inst->setHasNoUnsignedWrap(true);
  }

  applyFPFastMathModeDecorations(BV, Inst);
  return Inst;
}

Instruction *
SPIRVToLLVM::transLifetimeInst(SPIRVInstTemplateBase* BI, BasicBlock* BB, Function* F)
{
    auto ID = (BI->getOpCode() == OpLifetimeStart) ?
        Intrinsic::lifetime_start :
        Intrinsic::lifetime_end;
#if LLVM_VERSION_MAJOR >= 7
    auto *pFunc = Intrinsic::getDeclaration(M, ID, llvm::ArrayRef<llvm::Type*>(PointerType::getInt8PtrTy(*Context)));
#else
    auto *pFunc = Intrinsic::getDeclaration(M, ID);
#endif
    auto *pPtr = transValue(BI->getOperand(0), F, BB);

    Value *pArgs[] =
    {
        ConstantInt::get(Type::getInt64Ty(*Context), BI->getOpWord(1)),
        CastInst::CreatePointerCast(pPtr, PointerType::getInt8PtrTy(*Context), "", BB)
    };

    auto *pCI = CallInst::Create(pFunc, pArgs, "", BB);
    return pCI;
}

Instruction *
SPIRVToLLVM::transCmpInst(SPIRVValue* BV, BasicBlock* BB, Function* F) {
  SPIRVCompare* BC = static_cast<SPIRVCompare*>(BV);
  IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  SPIRVType* BT = BC->getOperand(0)->getType();
  Instruction* Inst = nullptr;
  if (BT->isTypeVectorOrScalarInt()
   || BT->isTypePointer()
   || BT->isTypeQueue()
   || BT->isTypeBool())
    Inst = new ICmpInst(*BB, CmpMap::rmap(BC->getOpCode()),
        transValue(BC->getOperand(0), F, BB),
        transValue(BC->getOperand(1), F, BB));
  else if (BT->isTypeVectorOrScalarFloat())
    Inst = new FCmpInst(*BB, CmpMap::rmap(BC->getOpCode()),
        transValue(BC->getOperand(0), F, BB),
        transValue(BC->getOperand(1), F, BB));
  IGC_ASSERT_MESSAGE(Inst, "not implemented");

  if (PlaceholderMap.count(BV) && Inst){
    if (BV->getType()->isTypeBool())
      Inst = cast<Instruction>(promoteBool(Inst, BB));
    else
      IGC_ASSERT(Inst->getType() == transType(BC->getType()));

    IGC_ASSERT_MESSAGE(Inst, "Out of memory");
  }
  return Inst;
}

bool
SPIRVToLLVM::postProcessOCL() {
  // I think we dont need it
  std::vector <Function*> structFuncs;
  for (auto& F : M->functions())
  {
      if (F.getReturnType()->isStructTy())
      {
          structFuncs.push_back(&F);
      }
  }
  for (auto structFunc : structFuncs)
    postProcessFunctionsReturnStruct(structFunc);

  std::vector<Function*> aggrFuncs;
  for (auto& F : M->functions())
  {
      if (std::any_of(F.arg_begin(), F.arg_end(), [](Argument& arg) { return arg.getType()->isAggregateType(); }) )
          {
              aggrFuncs.push_back(&F);
          }
  }
  for (auto aggrFunc : aggrFuncs)
      postProcessFunctionsWithAggregateArguments(aggrFunc);

  return true;
}

bool
SPIRVToLLVM::postProcessFunctionsReturnStruct(Function *F) {

  if (!F->getReturnType()->isStructTy())
    return false;

  std::string Name = F->getName().str();
  F->setName(Name + ".old");

  std::vector<Type *> ArgTys;
  getFunctionTypeParameterTypes(F->getFunctionType(), ArgTys);
  ArgTys.insert(ArgTys.begin(), PointerType::get(F->getReturnType(),
      SPIRAS_Private));
  auto newFType = FunctionType::get(Type::getVoidTy(*Context), ArgTys, false);
  auto NewF = cast<Function>(M->getOrInsertFunction(Name,  newFType));
  NewF->setCallingConv(F->getCallingConv());

  if (!F->isDeclaration()) {
      ValueToValueMapTy VMap;
      llvm::SmallVector<llvm::ReturnInst*, 8> Returns;
      auto OldArgIt = F->arg_begin();
      auto NewArgIt = NewF->arg_begin();
      ++NewArgIt; // Skip first argument that we added.
      for (; OldArgIt != F->arg_end(); ++OldArgIt, ++NewArgIt) {
          NewArgIt->setName(OldArgIt->getName());
          VMap[&*OldArgIt] = &*NewArgIt;
      }
      IGCLLVM::CloneFunctionInto(NewF, F, VMap, true, Returns);
      auto DL = M->getDataLayout();
      const auto ptrSize = DL.getPointerSize();

      for (auto RetInst : Returns) {
          IGCLLVM::IRBuilder<> builder(RetInst);
          Type* retTy = RetInst->getReturnValue()->getType();
          Value* returnedValPtr = builder.CreateAlloca(retTy);
          builder.CreateStore(RetInst->getReturnValue(), returnedValPtr);
          auto size = DL.getTypeAllocSize(retTy);
          builder.CreateMemCpy(&*NewF->arg_begin(), returnedValPtr, size, ptrSize);
          builder.CreateRetVoid();
          RetInst->eraseFromParent();
      }
  }

  for (auto I = F->user_begin(), E = F->user_end(); I != E;) {
    if (auto CI = dyn_cast<CallInst>(*I++)) {
      auto Args = getArguments(CI);
      IGCLLVM::IRBuilder<> builder(CI);
      //auto Alloca = new AllocaInst(CI->getType(), "", CI);
      auto Alloca = builder.CreateAlloca(CI->getType());
      Args.insert(Args.begin(), Alloca);
      auto NewCI = CallInst::Create(NewF, Args, "", CI);
      NewCI->setCallingConv(CI->getCallingConv());
      auto Load = new LoadInst(
#if LLVM_VERSION_MAJOR > 7
      Alloca->getType()->getPointerElementType(),
#endif
      Alloca,"",CI);
      CI->replaceAllUsesWith(Load);
      CI->eraseFromParent();
    }
  }
  F->dropAllReferences();
  F->removeFromParent();
  return true;
}

bool
SPIRVToLLVM::postProcessFunctionsWithAggregateArguments(Function* F) {
  auto Name = F->getName();
  auto Attrs = F->getAttributes();
  auto DL = M->getDataLayout();
  auto ptrSize = DL.getPointerSize();

  mutateFunction (F,
      [=](CallInst *CI, std::vector<Value *> &Args) {
    auto FBegin = CI->getParent()->getParent()->begin()->getFirstInsertionPt();
    IGCLLVM::IRBuilder<> builder(&(*FBegin));

    for (auto &I:Args) {
      auto T = I->getType();
      if (!T->isAggregateType())
        continue;

      if (auto constVal = dyn_cast<Constant>(I)) {
        I = new GlobalVariable(*M, T, true, GlobalValue::InternalLinkage, constVal);
      } else {
        builder.SetInsertPoint(CI);
        Value* allocaInst = builder.CreateAlloca(T);
        builder.CreateStore(I, allocaInst);
        I = allocaInst;
      }

      builder.SetInsertPoint(&*FBegin);
      auto Alloca = builder.CreateAlloca(T);
      Alloca->setAlignment(IGCLLVM::getCorrectAlign(ptrSize));

      auto size = DL.getTypeAllocSize(T);
      builder.SetInsertPoint(CI);
      builder.CreateMemCpy(Alloca, I, size, ptrSize);
      if (T->isArrayTy()) {
        I = ptrSize > 4
          ? builder.CreateConstInBoundsGEP2_64(Alloca, 0, 0)
          : builder.CreateConstInBoundsGEP2_32(nullptr, Alloca, 0, 0);
      } else if (T->isStructTy()) {
        I = Alloca;
      } else {
        llvm_unreachable("Unknown aggregate type!");
      }
    }
    return Name.str();
  }, false, &Attrs);
  return true;
}

std::string
SPIRVToLLVM::transOCLPipeTypeAccessQualifier(igc_spv::SPIRVTypePipe* ST) {
  return SPIRSPIRVAccessQualifierMap::rmap(ST->getAccessQualifier());
}

Value *
SPIRVToLLVM::oclTransConstantSampler(igc_spv::SPIRVConstantSampler* BCS, BasicBlock* BB) {
  auto* SamplerT = getOrCreateOpaquePtrType(
    M,
    std::string(kSPIRVTypeName::PrefixAndDelim) + std::string(kSPIRVTypeName::Sampler),
    SPIRAS_Constant);
  auto* I32Ty = IntegerType::getInt32Ty(*Context);
  auto* FTy = FunctionType::get(SamplerT, { I32Ty }, false);

  Function* Func = cast<Function>(M->getOrInsertFunction("__translate_sampler_initializer", FTy));

  auto Lit = (BCS->getAddrMode() << 1) | BCS->getNormalized() |
             ((BCS->getFilterMode() + 1) << 4);

  return CallInst::Create(Func, { ConstantInt::get(I32Ty, Lit) }, "", BB);
}

static void insertCastAfter(Instruction* I, Instruction* Cast)
{
    if (I->getOpcode() == Instruction::PHI) // put cast after last phi in BB
    {
        BasicBlock* BB = I->getParent();
        IGC_ASSERT_MESSAGE(BB, "Invalid parent");
        BasicBlock::iterator BBI = BB->end();
        do {
            --BBI;
        } while (!isa<PHINode>(BBI));
        Instruction* lastPhi = &(*BBI);
        IGC_ASSERT_MESSAGE(lastPhi, "BasicBlock most contain at least one PHI");
        Cast->insertAfter(lastPhi);
    }
    else
    {
        Cast->insertAfter(I);
    }
}

Value *SPIRVToLLVM::promoteBool(Value *pVal, BasicBlock *BB)
{
    if (!pVal->getType()->getScalarType()->isIntegerTy(1))
        return pVal;

    auto *PromoType = isa<VectorType>(pVal->getType()) ?
        cast<Type>(IGCLLVM::FixedVectorType::get(Type::getInt8Ty(pVal->getContext()),
        (unsigned)cast<IGCLLVM::FixedVectorType>(pVal->getType())->getNumElements())) :
        Type::getInt8Ty(pVal->getContext());

    if (auto *C = dyn_cast<Constant>(pVal))
        return ConstantExpr::getZExtOrBitCast(C, PromoType);

    if (BB == nullptr)
        return pVal;

    if (auto *Arg = dyn_cast<Argument>(pVal))
    {
        auto &entry = BB->getParent()->getEntryBlock();
        Instruction *Cast = nullptr;
        if (entry.empty())
        {
            Cast = CastInst::CreateZExtOrBitCast(Arg, PromoType, "i1promo", BB);
        }
        else
        {
            auto IP = entry.begin();
            while (isa<AllocaInst>(IP)) ++IP;
            if (IP == BB->end())
                Cast = CastInst::CreateZExtOrBitCast(Arg, PromoType, "i1promo", BB);
            else
                Cast = CastInst::CreateZExtOrBitCast(Arg, PromoType, "i1promo", &(*IP));
        }
        return Cast;
    }

    auto *pInst = cast<Instruction>(pVal);
    auto *Cast = CastInst::CreateZExtOrBitCast(pInst, PromoType, "i1promo");
    insertCastAfter(pInst, Cast);
    return Cast;
}

Value *SPIRVToLLVM::truncBool(Value *pVal, BasicBlock *BB)
{
    if (!pVal->getType()->getScalarType()->isIntegerTy(8))
        return pVal;

    auto *TruncType = isa<VectorType>(pVal->getType()) ?
        cast<Type>(IGCLLVM::FixedVectorType::get(Type::getInt1Ty(pVal->getContext()),
            (unsigned)cast<IGCLLVM::FixedVectorType>(pVal->getType())->getNumElements())) :
        Type::getInt1Ty(pVal->getContext());

    if (auto *C = dyn_cast<Constant>(pVal))
        return ConstantExpr::getTruncOrBitCast(C, TruncType);

    if (BB == nullptr)
        return pVal;

    if (auto *Arg = dyn_cast<Argument>(pVal))
    {
        auto &entry = BB->getParent()->getEntryBlock();
        Instruction *Cast = nullptr;
        if (entry.empty())
        {
            Cast = CastInst::CreateTruncOrBitCast(Arg, TruncType, "i1trunc", BB);
        }
        else
        {
            auto IP = entry.begin();
            while (isa<AllocaInst>(IP))
            {
                if (++IP == BB->end())
                    break;
            }
            if (IP == BB->end())
                Cast = CastInst::CreateTruncOrBitCast(Arg, TruncType, "i1trunc", BB);
            else
                Cast = CastInst::CreateTruncOrBitCast(Arg, TruncType, "i1trunc", &(*IP));
        }
        return Cast;
    }

    auto *pInst = cast<Instruction>(pVal);
    auto *Cast = CastInst::CreateTruncOrBitCast(pInst, TruncType, "i1trunc");
    // This is a WA for the case where placeholder instruction is being assigned to a wrong basic block.
    // The WA will be removed during switchover to Khronos SPIRV - LLVM Translator.
    auto* nextInst = pInst->getNextNonDebugInstruction();
    if (nullptr != nextInst && nextInst->getOpcode() == Instruction::Trunc && (nextInst->getOperand(0) == Cast->getOperand(0)))
    {
      BB->getInstList().push_back(Cast);
      return Cast;
    }

    insertCastAfter(pInst, Cast);
    return Cast;
}

Type *SPIRVToLLVM::truncBoolType(SPIRVType *SPVType, Type *LLType)
{
    if (!SPVType->isTypeVectorOrScalarBool())
        return LLType;

    return isa<VectorType>(LLType) ?
        cast<Type>(IGCLLVM::FixedVectorType::get(Type::getInt1Ty(LLType->getContext()),
                                   (unsigned)cast<IGCLLVM::FixedVectorType>(LLType)->getNumElements())) :
        Type::getInt1Ty(LLType->getContext());
}

// Translate aliasing memory access masks for SPIRVLoad and SPIRVStore
// instructions. These masks are mapped on alias.scope and noalias
// metadata in LLVM. Translation of optional string operand isn't yet supported
// in the translator.
template <typename SPIRVInstType>
void SPIRVToLLVM::transAliasingMemAccess(SPIRVInstType* BI, Instruction* I) {
  static_assert(std::is_same<SPIRVInstType, SPIRVStore>::value ||
                std::is_same<SPIRVInstType, SPIRVLoad>::value,
                "Only stores and loads can be aliased by memory access mask");
  if (BI->SPIRVMemoryAccess::isNoAlias())
    addMemAliasMetadata(I, BI->SPIRVMemoryAccess::getNoAliasInstID(),
                        LLVMContext::MD_noalias);
  if (BI->SPIRVMemoryAccess::isAliasScope())
    addMemAliasMetadata(I, BI->SPIRVMemoryAccess::getAliasScopeInstID(),
                        LLVMContext::MD_alias_scope);
}

// Create and apply alias.scope/noalias metadata
void SPIRVToLLVM::addMemAliasMetadata(Instruction* I, SPIRVId AliasListId,
  uint32_t AliasMDKind) {
  SPIRVAliasScopeListDeclINTEL* AliasList =
    BM->get<SPIRVAliasScopeListDeclINTEL>(AliasListId);
  std::vector<SPIRVId> AliasScopeIds = AliasList->getArguments();
  MDBuilder MDB(*Context);
  SmallVector<Metadata*, 4> MDScopes;
  for (const auto ScopeId : AliasScopeIds) {
    SPIRVAliasScopeDeclINTEL* AliasScope =
      BM->get<SPIRVAliasScopeDeclINTEL>(ScopeId);
    std::vector<SPIRVId> AliasDomainIds = AliasScope->getArguments();
    // Currently we expect exactly one argument for aliasing scope
    // instruction.
    // TODO: add translation of string scope and domain operand.
    IGC_ASSERT_MESSAGE(AliasDomainIds.size() == 1,
      "AliasScopeDeclINTEL must have exactly one argument");
    SPIRVId AliasDomainId = AliasDomainIds[0];
    // Create and store unique domain and scope metadata
    MDAliasDomainMap.emplace(AliasDomainId,
                             MDB.createAnonymousAliasScopeDomain());
    MDAliasScopeMap.emplace(ScopeId, MDB.createAnonymousAliasScope(
                                       MDAliasDomainMap[AliasDomainId]));
    MDScopes.emplace_back(MDAliasScopeMap[ScopeId]);
  }
  // Create and store unique alias.scope/noalias metadata
  MDAliasListMap.emplace(AliasListId,
                         MDNode::concatenate(I->getMetadata(AliasMDKind),
                                             MDNode::get(*Context, MDScopes)));
  I->setMetadata(AliasMDKind, MDAliasListMap[AliasListId]);
}

void transFunctionPointerCallArgumentAttributes(SPIRVValue *BV, CallInst *CI) {
  std::vector<SPIRVDecorate const *> ArgumentAttributes =
      BV->getDecorations(DecorationArgumentAttributeINTEL);

  for (const auto *Dec : ArgumentAttributes) {
    std::vector<SPIRVWord> Literals = Dec->getVecLiteral();
    SPIRVWord ArgNo = Literals[0];
    SPIRVWord SpirvAttr = Literals[1];
    Attribute::AttrKind LlvmAttrKind = SPIRSPIRVFuncParamAttrMap::rmap(
        static_cast<SPIRVFuncParamAttrKind>(SpirvAttr));
#if LLVM_VERSION_MAJOR < 13
    bool IsTypeAttrKind;
    switch (LlvmAttrKind) {
    case Attribute::ByVal:
#if LLVM_VERSION_MAJOR >= 11
    case Attribute::Preallocated:
#endif
#if LLVM_VERSION_MAJOR >= 12
    case Attribute::ByRef:
#endif
    case Attribute::StructRet:
    case Attribute::InAlloca:
        IsTypeAttrKind = true;
        break;
    default:
        IsTypeAttrKind = false;
    }
#else
    bool IsTypeAttrKind = Attribute::isTypeAttrKind(LlvmAttrKind);
#endif

    auto LlvmAttr = IsTypeAttrKind
            ? Attribute::get(CI->getContext(), LlvmAttrKind,
                             cast<PointerType>(CI->getOperand(ArgNo)->getType())
                                 ->getElementType())
            : Attribute::get(CI->getContext(), LlvmAttrKind);
    CI->addParamAttr(ArgNo, LlvmAttr);
  }
}

/// For instructions, this function assumes they are created in order
/// and appended to the given basic block. An instruction may use a
/// instruction from another BB which has not been translated. Such
/// instructions should be translated to place holders at the point
/// of first use, then replaced by real instructions when they are
/// created.
///
/// When CreatePlaceHolder is true, create a load instruction of a
/// global variable as placeholder for SPIRV instruction. Otherwise,
/// create instruction and replace placeholder if there is one.
Value *
SPIRVToLLVM::transValueWithoutDecoration(SPIRVValue *BV, Function *F,
    BasicBlock *BB, bool CreatePlaceHolder){
  auto OC = BV->getOpCode();
  IntBoolOpMap::rfind(OC, &OC);

  // Translation of non-instruction values
  switch(OC) {
  case OpSpecConstant:
  case OpConstant: {
    SPIRVConstant *BConst = static_cast<SPIRVConstant *>(BV);
    SPIRVType *BT = BV->getType();
    Type *LT = transType(BT);
    uint64_t V = BConst->getZExtIntValue();
    if(BV->hasDecorate(DecorationSpecId)) {
      IGC_ASSERT_EXIT_MESSAGE(OC == OpSpecConstant, "Only SpecConstants can be specialized!");
      SPIRVWord specid = *BV->getDecorate(DecorationSpecId).begin();
      if(BM->isSpecConstantSpecialized(specid))
        V = BM->getSpecConstant(specid);
    }
    switch(BT->getOpCode()) {
    case OpTypeBool:
    case OpTypeInt:
      return mapValue(BV, ConstantInt::get(LT, V,
        static_cast<SPIRVTypeInt*>(BT)->isSigned()));
    case OpTypeFloat: {
      const llvm::fltSemantics *FS = nullptr;
      switch (BT->getFloatBitWidth()) {
      case 16:
        FS = &APFloat::IEEEhalf();
        break;
      case 32:
        FS = &APFloat::IEEEsingle();
        break;
      case 64:
        FS = &APFloat::IEEEdouble();
        break;
      default:
        IGC_ASSERT_EXIT_MESSAGE(0, "invalid float type");
        break;
      }
      return mapValue(BV, ConstantFP::get(*Context, APFloat(*FS,
          APInt(BT->getFloatBitWidth(), V))));
    }
    default:
      llvm_unreachable("Not implemented");
      return NULL;
    }
  }
  break;

  case OpSpecConstantTrue:
    if (BV->hasDecorate(DecorationSpecId)) {
      SPIRVWord specid = *BV->getDecorate(DecorationSpecId).begin();
      if (BM->isSpecConstantSpecialized(specid)) {
        if (BM->getSpecConstant(specid))
          return mapValue(BV, ConstantInt::getTrue(*Context));
        else
          return mapValue(BV, ConstantInt::getFalse(*Context));
      }
    }
  // intentional fall-through: if decoration was not specified, treat this
  // as a OpConstantTrue (default spec constant value)
  case OpConstantTrue:
    return mapValue(BV, ConstantInt::getTrue(*Context));

  case OpSpecConstantFalse:
    if (BV->hasDecorate(DecorationSpecId)) {
      SPIRVWord specid = *BV->getDecorate(DecorationSpecId).begin();
      if (BM->isSpecConstantSpecialized(specid)) {
        if (BM->getSpecConstant(specid))
          return mapValue(BV, ConstantInt::getTrue(*Context));
        else
          return mapValue(BV, ConstantInt::getFalse(*Context));
      }
    }
  // intentional fall-through: if decoration was not specified, treat this
  // as a OpConstantFalse (default spec constant value)
  case OpConstantFalse:
    return mapValue(BV, ConstantInt::getFalse(*Context));

  case OpConstantNull: {
    auto LT = transType(BV->getType());
    return mapValue(BV, Constant::getNullValue(LT));
  }

  case OpSpecConstantComposite:
  case OpConstantComposite: {
    auto BCC = static_cast<SPIRVConstantComposite*>(BV);
    std::vector<Constant *> CV;
    for (auto &I:BCC->getElements())
      CV.push_back(dyn_cast<Constant>(transValue(I, F, BB)));
    for (auto& CI : BCC->getContinuedInstructions()) {
        for (auto& I : CI->getElements())
            CV.push_back(dyn_cast<Constant>(transValue(I, F, BB)));
    }
    switch(BV->getType()->getOpCode()) {
    case OpTypeVector:
      return mapValue(BV, ConstantVector::get(CV));
    case OpTypeArray:
      return mapValue(BV, ConstantArray::get(
          dyn_cast<ArrayType>(transType(BCC->getType())), CV));
    case OpTypeStruct:
      return mapValue(BV, ConstantStruct::get(
          dyn_cast<StructType>(transType(BCC->getType())), CV));
    default:
      llvm_unreachable("not implemented");
      return nullptr;
    }
  }
  break;

  case OpCompositeConstruct: {
    auto BCC = static_cast<SPIRVCompositeConstruct*>(BV);
    std::vector<Value *> CV;
    for(auto &I : BCC->getElements())
    {
      CV.push_back( transValue( I,F,BB ) );
    }
    switch(BV->getType()->getOpCode()) {
    case OpTypeVector:
    {
      Type  *T = transType( BCC->getType() );

      Value *undef = llvm::UndefValue::get( T );
      Value *elm1  = undef;
      uint32_t pos = 0;

      auto  CreateCompositeConstruct = [&]( Value* Vec,Value* ValueToBeInserted,uint32_t Pos )
      {
        Value *elm = InsertElementInst::Create(
          Vec,
          ValueToBeInserted,
          ConstantInt::get( *Context,APInt( 32,Pos ) ),
          BCC->getName(),BB );
        return elm;
      };

      for(unsigned i = 0; i < CV.size(); i++)
      {
        if(CV[i]->getType()->isVectorTy())
        {
          for(uint32_t j = 0; j < cast<IGCLLVM::FixedVectorType>(CV[i]->getType())->getNumElements(); j++)
          {
            Value *v = ExtractElementInst::Create( CV[i],ConstantInt::get( *Context,APInt( 32,j ) ),BCC->getName(),BB );
            elm1 = CreateCompositeConstruct( elm1,v,pos++ );
          }
        }
        else
        {
          elm1 = CreateCompositeConstruct( elm1,CV[i],pos++ );
        }
      }
      return mapValue( BV,elm1 );
    }
    break;
    case OpTypeArray:
    case OpTypeStruct:
    {
      Type *T = transType( BV->getType() );
      Value *undef = llvm::UndefValue::get( T );
      Value *elm1  = undef;

      for(unsigned i = 0; i < CV.size(); i++)
      {
        elm1 = InsertValueInst::Create(
          elm1,
          CV[i],
          i,
          BCC->getName(),BB );
      }
      return mapValue( BV,elm1 );
    }
    break;
    case OpTypeJointMatrixINTEL:
    {
      IGC_ASSERT(CV.size() == 1);

      std::vector<Type *> ArgTys = { CV[0]->getType() };
      std::vector<Value *> Args = { CV[0] };

      Type *T = transType( BV->getType() );
      FunctionType *builtinTy = FunctionType::get( T, ArgTys, false );

      auto BI = static_cast<SPIRVInstruction *>( BV );
      std::string builtinName( getSPIRVBuiltinName( BV->getOpCode(), BI, ArgTys, "JointMatrixINTEL" ) );
      Function *Func = cast<Function>( M->getOrInsertFunction( builtinName, builtinTy ) );

      CallInst *CI = CallInst::Create( Func, Args, "matrix", BB );
      return mapValue( BV, CI );
    }
    break;
    default:
      llvm_unreachable( "not implemented" );
      return nullptr;
    }
  }
  break;
  case OpConstantSampler: {
    auto BCS = static_cast<SPIRVConstantSampler*>(BV);
    return oclTransConstantSampler(BCS, BB);
  }

  case OpSpecConstantOp: {
    auto BI = createInstFromSpecConstantOp(
        static_cast<SPIRVSpecConstantOp*>(BV));
    return mapValue(BV, transValue(BI, nullptr, nullptr, false));
  }

  case OpConstFunctionPointerINTEL: {
    SPIRVConstFunctionPointerINTEL* BC =
      static_cast<SPIRVConstFunctionPointerINTEL*>(BV);
    SPIRVFunction* F = BC->getFunction();
    BV->setName(F->getName());
    return mapValue(BV, transFunction(F));
  }

  case OpConstantPipeStorage:
  {
      auto CPS = static_cast<SPIRVConstantPipeStorage*>(BV);
      const uint32_t packetSize    = CPS->GetPacketSize();
      const uint32_t packetAlign   = CPS->GetPacketAlignment();
      const uint32_t maxNumPackets = CPS->GetCapacity();
      // This value matches the definition from the runtime and that from pipe.cl.
      const uint32_t INTEL_PIPE_HEADER_RESERVED_SPACE = 128;

      const uint32_t numPacketsAlloc = maxNumPackets + 1;
      const uint32_t bufSize = packetSize * numPacketsAlloc + INTEL_PIPE_HEADER_RESERVED_SPACE;

      SmallVector<uint8_t, 256> buf(bufSize, 0);
      // Initialize the pipe_max_packets field in the control structure.
      for (unsigned i = 0; i < 4; i++)
          buf[i] = (uint8_t)((numPacketsAlloc >> (8 * i)) & 0xff);

      auto *pInit = ConstantDataArray::get(*Context, buf);

      GlobalVariable *pGV = new GlobalVariable(
          *M,
          pInit->getType(),
          false,
          GlobalVariable::InternalLinkage,
          pInit,
          Twine("pipebuf"),
          nullptr,
          GlobalVariable::ThreadLocalMode::NotThreadLocal,
          SPIRAS_Global);

      pGV->setAlignment(IGCLLVM::getCorrectAlign(std::max(4U, packetAlign)));

      auto *pStorageTy = transType(CPS->getType());
      return mapValue(CPS, ConstantExpr::getBitCast(pGV, pStorageTy));
  }

  case OpUndef:
    return mapValue(BV, UndefValue::get(transType(BV->getType())));

  case OpVariable: {
    auto BVar = static_cast<SPIRVVariable *>(BV);
    auto Ty = transType(BVar->getType()->getPointerElementType());
    bool IsConst = BVar->isConstant();
    llvm::GlobalValue::LinkageTypes LinkageTy = transLinkageType(BVar);
    Constant *Initializer = nullptr;
    SPIRVStorageClassKind BS = BVar->getStorageClass();
    SPIRVValue *Init = BVar->getInitializer();

    if (isSPIRVSamplerType(Ty) && BS == StorageClassUniformConstant) {
      // Skip generating llvm code during translation of a variable definition,
      // generate code only for its uses
      if (!BB)
        return nullptr;

      IGC_ASSERT_MESSAGE(Init, "UniformConstant OpVariable with sampler type must have an initializer!");
      return transValue(Init, F, BB);
    }

    if (Init)
        Initializer = dyn_cast<Constant>(transValue(Init, F, BB, false));
    else if (LinkageTy == GlobalValue::CommonLinkage)
        // In LLVM variables with common linkage type must be initilized by 0
        Initializer = Constant::getNullValue(Ty);
    else if (BS == StorageClassWorkgroupLocal)
        Initializer = UndefValue::get(Ty);

    if (BS == StorageClassFunction && !Init) {
        IGC_ASSERT_MESSAGE(BB, "Invalid BB");
        IGCLLVM::IRBuilder<> builder(BB);
        //return mapValue(BV, new AllocaInst(Ty, BV->getName(), BB));
        return mapValue(BV, builder.CreateAlloca(Ty, nullptr, BV->getName()));
    }
    auto AddrSpace = SPIRSPIRVAddrSpaceMap::rmap(BS);
    auto LVar = new GlobalVariable(*M, Ty, IsConst, LinkageTy, Initializer,
        BV->getName(), 0, GlobalVariable::NotThreadLocal, AddrSpace);
    GlobalVariable::UnnamedAddr addrType = (IsConst && Ty->isArrayTy() &&
        Ty->getArrayElementType()->isIntegerTy(8)) ? GlobalVariable::UnnamedAddr::Global :
        GlobalVariable::UnnamedAddr::None;
    LVar->setUnnamedAddr(addrType);
    SPIRVBuiltinVariableKind BVKind = BuiltInCount;
    if (BVar->isBuiltin(&BVKind))
      BuiltinGVMap[LVar] = BVKind;
    return mapValue(BV, LVar);
  }
  break;

  case OpFunctionParameter: {
    auto BA = static_cast<SPIRVFunctionParameter*>(BV);
    IGC_ASSERT_MESSAGE(F, "Invalid function");
    unsigned ArgNo = 0;
    for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
        ++I, ++ArgNo) {
      if (ArgNo == BA->getArgNo())
        return mapValue(BV, &(*I));
    }
    IGC_ASSERT_EXIT_MESSAGE(0, "Invalid argument");
    return NULL;
  }
  break;

  case OpFunction:
    return mapValue(BV, transFunction(static_cast<SPIRVFunction *>(BV)));

  case OpAsmINTEL:
    return mapValue(BV, transAsmINTEL(static_cast<SPIRVAsmINTEL *>(BV), F, BB));

  case OpLabel:
    return mapValue(BV, BasicBlock::Create(*Context, BV->getName(), F));
    break;
  default:
    // do nothing
    break;
  }

  // During translation of OpSpecConstantOp we create an instruction
  // corresponding to the Opcode operand and then translate this instruction.
  // For such instruction BB and F should be nullptr, because it is a constant
  // expression declared out of scope of any basic block or function.
  // All other values require valid BB pointer.
  IGC_ASSERT_MESSAGE(((isSpecConstantOpAllowedOp(OC) && !F && !BB) || BB), "Invalid BB");

  // Creation of place holder
  if (CreatePlaceHolder) {
    auto GV = new GlobalVariable(*M,
        transType(BV->getType()),
        false,
        GlobalValue::PrivateLinkage,
        nullptr,
        std::string(kPlaceholderPrefix) + BV->getName(),
        0, GlobalVariable::NotThreadLocal, 0);

    auto LD = new LoadInst(
#if LLVM_VERSION_MAJOR > 7
      GV->getType()->getPointerElementType(),
#endif
      GV, BV->getName(), BB);
    PlaceholderMap[BV] = LD;
    return mapValue(BV, LD);
  }

  // Translation of instructions
  switch (BV->getOpCode()) {
  case OpVariableLengthArrayINTEL: {
    auto *VLA = static_cast<SPIRVVariableLengthArrayINTEL *>(BV);
    llvm::Type *Ty = transType(BV->getType()->getPointerElementType());
    llvm::Value *ArrSize = transValue(VLA->getOperand(0), F, BB);
    return mapValue(
        BV, new AllocaInst(Ty, SPIRAS_Private, ArrSize, BV->getName(), BB));
  }

  case OpRestoreMemoryINTEL: {
    auto *Restore = static_cast<SPIRVRestoreMemoryINTEL *>(BV);
    llvm::Value *Ptr = transValue(Restore->getOperand(0), F, BB);
    Function *StackRestore =
        Intrinsic::getDeclaration(M, Intrinsic::stackrestore);
    return mapValue(BV, CallInst::Create(StackRestore, {Ptr}, "", BB));
  }

  case OpSaveMemoryINTEL: {
    Function *StackSave = Intrinsic::getDeclaration(M, Intrinsic::stacksave);
    return mapValue(BV, CallInst::Create(StackSave, "", BB));
  }

  case OpBranch: {
    auto *BR = static_cast<SPIRVBranch *>(BV);
    auto *BI = BranchInst::Create(
        cast<BasicBlock>(transValue(BR->getTargetLabel(), F, BB)), BB);
    // Loop metadata will be translated in the end of function translation.
    return mapValue(BV, BI);
    }
    break;

  case OpBranchConditional: {
    auto *BR = static_cast<SPIRVBranchConditional *>(BV);
    auto *BC = BranchInst::Create(
        cast<BasicBlock>(transValue(BR->getTrueLabel(), F, BB)),
        cast<BasicBlock>(transValue(BR->getFalseLabel(), F, BB)),
        // cond must be an i1, truncate bool to i1 if it was an i8.
        transValue(BR->getCondition(), F, BB, true, BoolAction::Truncate), BB);
    // Loop metadata will be translated in the end of function translation.
    return mapValue(BV, BC);
    }
    break;

  case OpPhi: {
    auto Phi = static_cast<SPIRVPhi *>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    auto LPhi = dyn_cast<PHINode>(mapValue(BV, PHINode::Create(
      transType(Phi->getType()),
      Phi->getPairs().size() / 2,
      Phi->getName(),
      BB)));
    Phi->foreachPair([&](SPIRVValue *IncomingV, SPIRVBasicBlock *IncomingBB,
      size_t Index){
      auto Translated = transValue(IncomingV, F, BB);
      LPhi->addIncoming(Translated,
        dyn_cast<BasicBlock>(transValue(IncomingBB, F, BB)));
    });
    return LPhi;
    }
    break;

  case OpReturn:
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    return mapValue(BV, ReturnInst::Create(*Context, BB));
    break;

  case OpReturnValue: {
    auto RV = static_cast<SPIRVReturnValue *>(BV);
    return mapValue(BV, ReturnInst::Create(*Context,
      transValue(RV->getReturnValue(), F, BB), BB));
    }
    break;

  case OpStore: {
    SPIRVStore *BS = static_cast<SPIRVStore*>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    auto *pValue    = transValue(BS->getSrc(), F, BB);
    auto *pPointer  = transValue(BS->getDst(), F, BB);
    bool isVolatile =
        BS->hasDecorate(DecorationVolatile) || BS->SPIRVMemoryAccess::isVolatile() != 0;
    unsigned alignment = BS->SPIRVMemoryAccess::getAlignment();

    if (auto *CS = dyn_cast<ConstantStruct>(pValue))
    {
        // Break up a store with a literal struct as the value as we don't have any
        // legalization infrastructure to do it:
        // Ex.
        // store %0 { <2 x i32> <i32 -2100480000, i32 2100480000>, %1 { i32 -2100483600, i8 -128 } }, %0 addrspace(1)* %a
        // =>
        // %CS.tmpstore = alloca %0
        // %0 = getelementptr inbounds %0* %CS.tmpstore, i32 0, i32 0
        // store <2 x i32> <i32 -2100480000, i32 2100480000>, <2 x i32>* %0
        // %1 = getelementptr inbounds %0* %CS.tmpstore, i32 0, i32 1
        // %2 = getelementptr inbounds %1* %1, i32 0, i32 0
        // store i32 -2100483600, i32* %2
        // %3 = getelementptr inbounds %1* %1, i32 0, i32 1
        // store i8 -128, i8* %3
        // %4 = bitcast %0 addrspace(1)* %a to i8 addrspace(1)*
        // %5 = bitcast %0* %CS.tmpstore to i8*
        // call void @llvm.memcpy.p1i8.p0i8.i64(i8 addrspace(1)* %4, i8* %5, i64 16, i32 0, i1 false)
        // So we emit this store in a similar fashion as clang would.
        IGCLLVM::IRBuilder<> IRB(&F->getEntryBlock(), F->getEntryBlock().begin());
        auto DL = M->getDataLayout();
        std::function<void(ConstantStruct*, Value*)>
        LowerConstantStructStore = [&](ConstantStruct *CS, Value *pointer)
        {
            for (unsigned I = 0, E = CS->getNumOperands(); I != E; I++)
            {
                auto *op = CS->getOperand(I);
                auto *pGEP = IRB.CreateConstInBoundsGEP2_32(nullptr, pointer, 0, I);
                if (auto *InnerCS = dyn_cast<ConstantStruct>(op))
                    LowerConstantStructStore(InnerCS, pGEP);
                else
                    IRB.CreateStore(op, pGEP);
            }
        };
        auto *pAlloca = IRB.CreateAlloca(pValue->getType(), nullptr, "CS.tmpstore");
        IRB.SetInsertPoint(BB);
        LowerConstantStructStore(CS, pAlloca);
        auto *pDst = IRB.CreateBitCast(pPointer,
            Type::getInt8PtrTy(*Context, pPointer->getType()->getPointerAddressSpace()));
        auto *pSrc = IRB.CreateBitCast(pAlloca, Type::getInt8PtrTy(*Context));
        auto *pMemCpy = IRB.CreateMemCpy(pDst, pSrc,
            DL.getTypeAllocSize(pAlloca->getAllocatedType()),
            alignment, isVolatile);
        return mapValue(BV, pMemCpy);
    }

    StoreInst *SI = new StoreInst(
      pValue,
      pPointer,
      isVolatile,
      IGCLLVM::getCorrectAlign(alignment),
      BB);
    if(BS->SPIRVMemoryAccess::isNonTemporal())
      transNonTemporalMetadata(SI);
    transAliasingMemAccess<SPIRVStore>(BS, SI);
    return mapValue(BV, SI);
  }
  break;

  case OpLoad: {
    SPIRVLoad *BL = static_cast<SPIRVLoad*>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    auto val = transValue(BL->getSrc(), F, BB);
    LoadInst* LI = new LoadInst(
#if LLVM_VERSION_MAJOR > 7
      val->getType()->getPointerElementType(),
#endif
      val,
      BV->getName(),
      BL->hasDecorate(DecorationVolatile) || BL->SPIRVMemoryAccess::isVolatile() != 0,
      IGCLLVM::getCorrectAlign(BL->SPIRVMemoryAccess::getAlignment()),
      BB);
    if(BL->SPIRVMemoryAccess::isNonTemporal())
      transNonTemporalMetadata(LI);
    transAliasingMemAccess<SPIRVLoad>(BL, LI);
    return mapValue(BV, LI);
    }
    break;

  case OpCopyMemorySized: {
    SPIRVCopyMemorySized *BC = static_cast<SPIRVCopyMemorySized *>(BV);
    CallInst *CI = nullptr;
    llvm::Value *Dst = transValue(BC->getTarget(), F, BB);
    unsigned Align = BC->getAlignment();
    llvm::Value *Size = transValue(BC->getSize(), F, BB);
    bool IsVolatile = BC->SPIRVMemoryAccess::isVolatile();
    IGCLLVM::IRBuilder<> Builder(BB);

    // If we copy from zero-initialized array, we can optimize it to llvm.memset
    if (BC->getSource()->getOpCode() == OpBitcast) {
      SPIRVValue *Source =
        static_cast<SPIRVBitcast *>(BC->getSource())->getOperand(0);
      if (Source->isVariable()) {
        auto *Init = static_cast<SPIRVVariable *>(Source)->getInitializer();

        if (Init && Init->getOpCode() == OpConstantNull) {
          SPIRVType *Ty = static_cast<SPIRVConstantNull *>(Init)->getType();
          if (Ty->isTypeArray()) {
              Type* Int8Ty = Type::getInt8Ty(Dst->getContext());
              llvm::Value* Src = ConstantInt::get(Int8Ty, 0);
              llvm::Value* newDst = Dst;
              if (!Dst->getType()->getPointerElementType()->isIntegerTy(8)) {
                  Type* Int8PointerTy = Type::getInt8PtrTy(Dst->getContext(),
                      Dst->getType()->getPointerAddressSpace());
                  newDst = llvm::BitCastInst::CreatePointerCast(Dst,
                      Int8PointerTy, "", BB);
              }
              CI = Builder.CreateMemSet(newDst, Src, Size, IGCLLVM::getCorrectAlign(Align), IsVolatile);
          }
        }
      }
    }
    if (!CI) {
      llvm::Value *Src = transValue(BC->getSource(), F, BB);
      CI = Builder.CreateMemCpy(Dst, Src, Size, Align, IsVolatile);
    }
    if (isFuncNoUnwind())
      CI->getFunction()->addFnAttr(Attribute::NoUnwind);
    return mapValue(BV, CI);
  }
  break;
  case OpCopyObject: {
    auto BI = static_cast<SPIRVInstTemplateBase*>(BV);
    auto source = transValue( BI->getOperand( 0 ),F,BB );
    return mapValue( BV,source );
  }
  case OpSelect: {
    SPIRVSelect *BS = static_cast<SPIRVSelect*>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    return mapValue(BV, SelectInst::Create(
      // cond must be an i1, truncate bool to i1 if it was an i8.
      transValue(BS->getCondition(), F, BB, true, BoolAction::Truncate),
      transValue(BS->getTrueValue(), F, BB),
      transValue(BS->getFalseValue(), F, BB),
      BV->getName(), BB));
    }
    break;
  case OpLoopMerge:          // Will be translated after all other function's
  case OpLoopControlINTEL:   // instructions are translated.
    {
    FuncLoopMetadataMap[BB] = BV;
    return nullptr;
    }
    break;

  case OpSwitch: {
    auto BS = static_cast<SPIRVSwitch *>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    auto Select = transValue(BS->getSelect(), F, BB);
    auto LS = SwitchInst::Create(Select,
      dyn_cast<BasicBlock>(transValue(BS->getDefault(), F, BB)),
      BS->getNumPairs(), BB);
    BS->foreachPair(
        [&](SPIRVSwitch::LiteralTy Literals, SPIRVBasicBlock *Label) {
        IGC_ASSERT_MESSAGE(!Literals.empty(), "Literals should not be empty");
        IGC_ASSERT_MESSAGE(Literals.size() <= 2, "Number of literals should not be more then two");
        uint64_t Literal = uint64_t(Literals.at(0));
        if (Literals.size() == 2) {
          Literal += uint64_t(Literals.at(1)) << 32;
        }
          LS->addCase(ConstantInt::get(dyn_cast<IntegerType>(Select->getType()), Literal),
                      dyn_cast<BasicBlock>(transValue(Label, F, BB)));
        });
    return mapValue(BV, LS);
    }
    break;

  case OpAccessChain:
  case OpInBoundsAccessChain:
  case OpPtrAccessChain:
  case OpInBoundsPtrAccessChain: {
    auto AC = static_cast<SPIRVAccessChainBase *>(BV);
    auto Base = transValue(AC->getBase(), F, BB);
    Type *BaseTy = cast<PointerType>(Base->getType())->getPointerElementType();
    auto Index = transValue(AC->getIndices(), F, BB);
    if (!AC->hasPtrIndex())
      Index.insert(Index.begin(), getInt32(M, 0));
    auto IsInbound = AC->isInBounds();
    Value *V = nullptr;
    if (BB) {
      auto GEP = GetElementPtrInst::Create(BaseTy, Base, Index, BV->getName(), BB);
      GEP->setIsInBounds(IsInbound);
      V = GEP;
    } else {
      V = ConstantExpr::getGetElementPtr(BaseTy, dyn_cast<Constant>(Base), Index, IsInbound);
    }
    return mapValue(BV, V);
    }
    break;

  case OpCompositeExtract: {
    SPIRVCompositeExtract *CE = static_cast<SPIRVCompositeExtract *>(BV);
    auto Type = CE->getComposite()->getType();
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    if (Type->isTypeVector())
    {
        IGC_ASSERT_MESSAGE(CE->getIndices().size() == 1, "Invalid index");
        return mapValue(BV, ExtractElementInst::Create(
            transValue(CE->getComposite(), F, BB),
            ConstantInt::get(*Context, APInt(32, CE->getIndices()[0])),
            BV->getName(), BB));
    }
    else
    {
        return mapValue(BV, ExtractValueInst::Create(
            transValue(CE->getComposite(), F, BB),
            CE->getIndices(),
            BV->getName(), BB));
    }
    }
    break;

  case OpVectorExtractDynamic: {
    auto CE = static_cast<SPIRVVectorExtractDynamic *>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    if (CE->getVector()->getType()->getOpCode() == OpTypeJointMatrixINTEL)
    {
        Value *matrix = transValue(CE->getVector(), F, BB);
        Value *index = transValue(CE->getIndex(), F, BB);

        Type *returnType = transType(CE->getType());

        std::vector<Type *> ArgTys = { matrix->getType(), index->getType() };
        std::vector<Value *> Args = { matrix, index };

        FunctionType *builtinTy = FunctionType::get(returnType, ArgTys, false);

        auto BI = static_cast<SPIRVInstruction *>(BV);
        std::string builtinName(getSPIRVBuiltinName(BV->getOpCode(), BI, ArgTys, "JointMatrixINTEL"));
        Function *Func = cast<Function>(M->getOrInsertFunction( builtinName, builtinTy ));

        CallInst *CI = CallInst::Create(Func, Args, "slice_element", BB);
        return mapValue(BV, CI);
    }
    else
    {
        return mapValue(BV, ExtractElementInst::Create(
            transValue(CE->getVector(), F, BB),
            transValue(CE->getIndex(), F, BB),
            BV->getName(), BB));
    }
    }
    break;

  case OpCompositeInsert: {
    auto CI = static_cast<SPIRVCompositeInsert *>(BV);
    auto Type = CI->getComposite()->getType();
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    if (Type->isTypeVector())
    {
        IGC_ASSERT_MESSAGE(CI->getIndices().size() == 1, "Invalid index");
        return mapValue(BV, InsertElementInst::Create(
            transValue(CI->getComposite(), F, BB),
            transValue(CI->getObject(), F, BB),
            ConstantInt::get(*Context, APInt(32, CI->getIndices()[0])),
            BV->getName(), BB));
    }
    else
    {
        return mapValue(BV, InsertValueInst::Create(
            transValue(CI->getComposite(), F, BB),
            transValue(CI->getObject(), F, BB),
            CI->getIndices(),
            BV->getName(), BB));
    }
    }
    break;

  case OpVectorInsertDynamic: {
    auto CI = static_cast<SPIRVVectorInsertDynamic *>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    if (CI->getVector()->getType()->getOpCode() == OpTypeJointMatrixINTEL)
    {
        Value *matrix = transValue(CI->getVector(), F, BB);
        Value *component = transValue(CI->getComponent(), F, BB);
        Value *index = transValue(CI->getIndex(), F, BB);

        Type *returnType = transType(CI->getType());

        std::vector<Type *> ArgTys = {
            matrix->getType(), component->getType(), index->getType()
        };
        std::vector<Value *> Args = { matrix, component, index };

        FunctionType *builtinTy = FunctionType::get(returnType, ArgTys, false);

        auto BI = static_cast<SPIRVInstruction *>(BV);
        std::string builtinName(getSPIRVBuiltinName(BV->getOpCode(), BI, ArgTys, "JointMatrixINTEL"));
        Function *Func = cast<Function>(M->getOrInsertFunction( builtinName, builtinTy ));

        CallInst *CI = CallInst::Create(Func, Args, "matrix", BB);
        return mapValue(BV, CI);
    }
    else
    {
        return mapValue(BV, InsertElementInst::Create(
          transValue(CI->getVector(), F, BB),
          transValue(CI->getComponent(), F, BB),
          transValue(CI->getIndex(), F, BB),
          BV->getName(), BB));
    }
    }
    break;

  case OpVectorShuffle: {
    auto VS = static_cast<SPIRVVectorShuffle *>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    std::vector<Constant *> Components;
    IntegerType *Int32Ty = IntegerType::get(*Context, 32);
    for (auto I : VS->getComponents()) {
      if (I == static_cast<SPIRVWord>(-1))
        Components.push_back(UndefValue::get(Int32Ty));
      else
        Components.push_back(ConstantInt::get(Int32Ty, I));
    }
    return mapValue(BV, new ShuffleVectorInst(
      transValue(VS->getVector1(), F, BB),
      transValue(VS->getVector2(), F, BB),
      ConstantVector::get(Components),
      BV->getName(), BB));
    }
    break;

  case OpAssumeTrueKHR: {
    IRBuilder<> Builder(BB);
    SPIRVAssumeTrueKHR* BC = static_cast<SPIRVAssumeTrueKHR*>(BV);
    Value* Condition = transValue(BC->getCondition(), F, BB);
    Condition = Builder.CreateTrunc(Condition, Type::getInt1Ty(*Context));
    return mapValue(BV, Builder.CreateAssumption(Condition));
  }

  case OpExpectKHR: {
    IRBuilder<> Builder(BB);
    SPIRVExpectKHRInstBase* BC = static_cast<SPIRVExpectKHRInstBase*>(BV);
    Value* Val = transValue(BC->getOperand(0), F, BB);
    Value* ExpVal = transValue(BC->getOperand(1), F, BB);
    Function* ExpectFn = Intrinsic::getDeclaration(M, Intrinsic::expect, Val->getType());
    return mapValue(
        BV, Builder.CreateCall(ExpectFn, {Val, ExpVal}));
  }

  case OpBitReverse: {
    SPIRVUnary* BR = static_cast<SPIRVUnary*>(BV);
    IGC_ASSERT_MESSAGE(BR->getType()->isTypeInt(), "Unsupported type");
    IntegerType* IntTy = static_cast<IntegerType*>(transType(BR->getType()));
    Function* intr = Intrinsic::getDeclaration(M, Intrinsic::bitreverse, IntTy);
    auto* Call = CallInst::Create(intr, transValue(BR->getOperand(0), F, BB), BR->getName(), BB);
    return mapValue(BV, Call);
  }

  case OpFunctionCall: {
    SPIRVFunctionCall *BC = static_cast<SPIRVFunctionCall *>(BV);
    IGC_ASSERT_MESSAGE(BB, "Invalid BB");
    auto Call = CallInst::Create(
      transFunction(BC->getFunction()),
      transValue(BC->getArgumentValues(), F, BB),
      BC->getName(),
      BB);
    setCallingConv(Call);
    setAttrByCalledFunc(Call);
    return mapValue(BV, Call);
    }
    break;

  case OpAsmCallINTEL:
    return mapValue(
        BV, transAsmCallINTEL(static_cast<SPIRVAsmCallINTEL *>(BV), F, BB));

  case OpFunctionPointerCallINTEL: {
    SPIRVFunctionPointerCallINTEL *BC =
      static_cast<SPIRVFunctionPointerCallINTEL*>(BV);
    auto func = transValue(BC->getCalledValue(), F, BB);
    auto Call = CallInst::Create(
#if LLVM_VERSION_MAJOR > 7
        llvm::cast<llvm::FunctionType>(
            llvm::cast<llvm::PointerType>(func->getType())->getElementType()),
#endif
        func,
        transValue(BC->getArgumentValues(), F, BB),
        BC->getName(),
        BB);
    transFunctionPointerCallArgumentAttributes(BV, Call);
    // Assuming we are calling a regular device function
    Call->setCallingConv(CallingConv::SPIR_FUNC);
    // Don't set attributes, because at translation time we don't know which
    // function exactly we are calling.
    return mapValue(BV, Call);
    }

  case OpExtInst:
    return mapValue(BV, transOCLBuiltinFromExtInst(
      static_cast<SPIRVExtInst *>(BV), BB));
    break;

  case OpSNegate: {
    SPIRVUnary *BC = static_cast<SPIRVUnary*>(BV);
    return mapValue(BV, BinaryOperator::CreateNSWNeg(
      transValue(BC->getOperand(0), F, BB),
      BV->getName(), BB));
    }

  case OpFNegate: {
    SPIRVUnary *BC = static_cast<SPIRVUnary*>(BV);
    auto Neg =
#if LLVM_VERSION_MAJOR <= 10
        BinaryOperator
#else
        UnaryOperator
#endif
        ::CreateFNeg(
      transValue(BC->getOperand(0), F, BB),
      BV->getName(), BB);
    applyFPFastMathModeDecorations(BV, Neg);
    return mapValue(BV, Neg);
    }
    break;

  case OpNot: {
    SPIRVUnary *BC = static_cast<SPIRVUnary*>(BV);
    return mapValue(BV, BinaryOperator::CreateNot(
      transValue(BC->getOperand(0), F, BB),
      BV->getName(), BB));
    }
    break;

  case OpSizeOf:
  {
      auto BI = static_cast<SPIRVSizeOf*>(BV);
      IGC_ASSERT_MESSAGE(BI->getOpWords().size() == 1, "OpSizeOf takes one argument!");
      // getOperands() returns SPIRVValue(s) but this argument is a SPIRVType so
      // we have to just grab it by its entry id.
      auto pArg = BI->get<SPIRVTypePointer>(BI->getOpWord(0));
      auto pointee = pArg->getPointerElementType();
      auto DL = M->getDataLayout();
      uint64_t size = DL.getTypeAllocSize(transType(pointee));
      return mapValue(BV, ConstantInt::get(Type::getInt32Ty(*Context), size));
  }
  case OpCreatePipeFromPipeStorage:
  {
      auto BI = static_cast<SPIRVCreatePipeFromPipeStorage*>(BV);
      IGC_ASSERT_MESSAGE(BI->getOpWords().size() == 1, "OpCreatePipeFromPipeStorage takes one argument!");
      return mapValue(BI, CastInst::CreateTruncOrBitCast(
          transValue(BI->getOperand(0), F, BB),
          transType(BI->getType()),
          "", BB));
  }
  case OpUnreachable:
  {
      return mapValue(BV, new UnreachableInst(*Context, BB));
  }
  case OpLifetimeStart:
  case OpLifetimeStop:
  {
      return mapValue(BV,
          transLifetimeInst(static_cast<SPIRVInstTemplateBase*>(BV), BB, F));
  }
  case OpVectorTimesScalar:
  {
      auto BI = static_cast<SPIRVInstTemplateBase*>(BV);

      auto Vector = transValue(BI->getOperand(0), F, BB);
      auto Scalar = transValue(BI->getOperand(1), F, BB);

      auto VecType = cast<IGCLLVM::FixedVectorType>(Vector->getType());
      auto Undef   = UndefValue::get(VecType);

      auto ScalarVec = InsertElementInst::Create(Undef, Scalar,
          ConstantInt::getNullValue(Type::getInt32Ty(*Context)), "", BB);

      for (unsigned i = 1; i < VecType->getNumElements(); i++)
      {
          ScalarVec = InsertElementInst::Create(ScalarVec, Scalar,
              ConstantInt::get(Type::getInt32Ty(*Context), i), "", BB);
      }

      return mapValue(BV, BinaryOperator::CreateFMul(Vector, ScalarVec, "", BB));
  }
  case OpSMod:
  {
      auto BI = static_cast<SPIRVSRem*>(BV);
      auto *a = transValue(BI->getOperand(0), F, BB);
      auto *b = transValue(BI->getOperand(1), F, BB);
      auto *zero = ConstantInt::getNullValue(a->getType());
      auto *ShiftAmt = ConstantInt::get(
          a->getType()->getScalarType(),
          a->getType()->getScalarSizeInBits() - 1);
      auto *ShiftOp = isa<VectorType>(a->getType()) ?
          ConstantVector::getSplat(
              IGCLLVM::getElementCount((unsigned)cast<IGCLLVM::FixedVectorType>(a->getType())->getNumElements()), ShiftAmt) :
          ShiftAmt;

      // OCL C:
      //
      // int mod(int a, int b)
      // {
      //     int out = a % b;
      //     if (((a >> 31) != (b >> 31)) & out != 0)
      //     {
      //         // only add b to out if sign(a) != sign(b) and out != 0.
      //         out += b;
      //     }
      //
      //     return out;
      // }

      // %out = srem %a, %b
      auto *out = BinaryOperator::CreateSRem(a, b, "", BB);
      // %sha = ashr %a, 31
      auto *sha = BinaryOperator::CreateAShr(a, ShiftOp, "", BB);
      // %shb = ashr %b, 31
      auto *shb = BinaryOperator::CreateAShr(b, ShiftOp, "", BB);
      // %cmp1 = icmp ne %sha, %shb
      auto *cmp1 = CmpInst::Create(Instruction::ICmp, llvm::CmpInst::ICMP_NE,
          sha, shb, "", BB);
      // %cmp2 = icmp ne %out, 0
      auto *cmp2 = CmpInst::Create(Instruction::ICmp, llvm::CmpInst::ICMP_NE,
          out, zero, "", BB);
      // %and  = and %cmp1, %cmp2
      auto *and1 = BinaryOperator::CreateAnd(cmp1, cmp2, "", BB);
      // %add  = add %out, %b
      auto *add = BinaryOperator::CreateAdd(out, b, "", BB);
      // %sel  = select %and, %add, %out
      auto *sel = SelectInst::Create(and1, add, out, "", BB);

      return mapValue(BV, sel);
  }
  case OpArithmeticFenceINTEL:
  {
    // Translate arithmetic fence to it's operand to avoid crash on unknown opcode.
    // It is planned to be translated into @llvm.arithmetic.fence.f64 in the future.
    auto* BC = static_cast<SPIRVUnary*>(BV);
    return mapValue(BV, transValue(BC->getOperand(0), F, BB));
  }
  case OpJointMatrixLoadINTEL: {
    SPIRVJointMatrixLoadINTEL *ML = static_cast<SPIRVJointMatrixLoadINTEL *>(BV);
    std::vector<SPIRVValue *> BArgs = ML->getOperands();
    enum SPVIdx { Pointer, Stride, Layout, Scope, MemOp };

    SPIRVTypeJointMatrixINTEL *MatTy = static_cast<SPIRVTypeJointMatrixINTEL *>(ML->getType());
    const unsigned loadLayout = (unsigned)BM->get<SPIRVConstant>(BArgs[Layout]->getId())->getZExtIntValue();

    IGC_ASSERT_MESSAGE(BB, "Invalid BB");

    /* Get arugment values for the intrinsic call */
    Value *PtrVal = transValue(BArgs[Pointer], F, BB);
    Value *StrideVal = transValue(BArgs[Stride], F, BB);

    unsigned AS = static_cast<PointerType *>(PtrVal->getType())->getAddressSpace();
    /* Prepare types for the call: */
    Type *RetTy      = transType(MatTy);
    Type *PtrTy      = PointerType::get(Type::getInt8Ty(*Context), AS);
    Type *StrideTy   = Type::getInt32Ty(*Context);
    Type *LayoutTy   = Type::getInt32Ty(*Context);

    std::vector<Type *> ArgTys = {
      PtrTy, StrideTy, LayoutTy
    };
    FunctionType *builtinTy = FunctionType::get(RetTy, ArgTys, false);

    /* Cast if necessary and prepare rest of the arguments: */
    CastInst *Ptr = CastInst::CreatePointerCast(PtrVal, PtrTy, "", BB);
    if (StrideVal->getType() != StrideTy) {
      IGC_ASSERT_MESSAGE(StrideVal->getType()->isIntegerTy(),
          "Unspupported matrix stide type in load instruction.");
      StrideVal = CastInst::CreateIntegerCast(StrideVal, StrideTy, false, "stride", Ptr);
    }

    Value *LoadLayoutVal = ConstantInt::get(LayoutTy, loadLayout);

    /* Get function to call */
    std::string suffix = MatTy->getMangledName();
    auto BI = static_cast<SPIRVInstruction *>(BV);
    std::string builtinName(getSPIRVBuiltinName(BV->getOpCode(), BI, ArgTys, suffix));
    Function *Func = cast<Function>(M->getOrInsertFunction(builtinName, builtinTy));

    std::vector<Value *> Args = { Ptr, StrideVal, LoadLayoutVal };
    CallInst *CI = CallInst::Create(Func, Args, "matrix", BB);
    return mapValue(BV, CI);
  }
  case OpJointMatrixStoreINTEL: {
    SPIRVJointMatrixStoreINTEL *MS = static_cast<SPIRVJointMatrixStoreINTEL *>(BV);
    std::vector<SPIRVValue *> BArgs = MS->getOperands();
    enum SPVIdx { Pointer, Object, Stride, Layout, Scope, MemOp };

    SPIRVTypeJointMatrixINTEL *MatTy = static_cast<SPIRVTypeJointMatrixINTEL *>(BArgs[Object]->getType());
    const unsigned storeLayout = (unsigned)BM->get<SPIRVConstant>(BArgs[Layout]->getId())->getZExtIntValue();

    IGC_ASSERT_MESSAGE(BB, "Invalid BB");

    /* Get arugment values for the intrinsic call */
    Value *MatrixVal = transValue(BArgs[Object], F, BB);
    Value *PtrVal    = transValue(BArgs[Pointer], F, BB);
    Value *StrideVal = transValue(BArgs[Stride], F, BB);

    unsigned AS = static_cast<PointerType *>(PtrVal->getType())->getAddressSpace();
    /* Prepare types for the call: */
    Type *MatrixTy   = transType(MatTy);
    Type *PtrTy      = PointerType::get(Type::getInt8Ty(*Context), AS);
    Type *StrideTy   = Type::getInt32Ty(*Context);
    Type *LayoutTy   = Type::getInt32Ty(*Context);

    std::vector<Type *> ArgTys = {
      PtrTy, MatrixTy, StrideTy, LayoutTy
    };
    FunctionType *builtinTy = FunctionType::get(Type::getVoidTy(*Context), ArgTys, false);

    /* Cast if necessary and prepare rest of the arguments: */
    CastInst *Ptr = CastInst::CreatePointerCast(PtrVal, PtrTy, "", BB);
    if (StrideVal->getType() != StrideTy) {
      IGC_ASSERT_MESSAGE(StrideVal->getType()->isIntegerTy(),
          "Unspupported matrix stide type in store instruction.");
      StrideVal = CastInst::CreateIntegerCast(StrideVal, StrideTy, false, "stride", Ptr);
    }

    Value *StoreLayoutVal = ConstantInt::get(LayoutTy, storeLayout);

    /* Get function to call */
    std::string suffix = MatTy->getMangledName();
    auto BI = static_cast<SPIRVInstruction *>(BV);
    std::string builtinName(getSPIRVBuiltinName(BV->getOpCode(), BI, ArgTys, suffix));
    Function *Func = cast<Function>(M->getOrInsertFunction(builtinName, builtinTy));

    std::vector<Value *> Args = { Ptr, MatrixVal, StrideVal, StoreLayoutVal };
    CallInst *CI = CallInst::Create(Func, Args, "", BB);
    return mapValue(BV, CI);
  }
  case OpJointMatrixMadINTEL:
  case OpJointMatrixSUMadINTEL:
  case OpJointMatrixUSMadINTEL:
  case OpJointMatrixUUMadINTEL: {
    SPIRVInstruction *MI = static_cast<SPIRVInstruction *>(BV);
    std::vector<SPIRVValue *> BArgs = MI->getOperands();

    enum SPVIdx { A, B, C, Scope };
    auto *MatATy = static_cast<SPIRVTypeJointMatrixINTEL *>(BArgs[A]->getType());
    auto *MatBTy = static_cast<SPIRVTypeJointMatrixINTEL *>(BArgs[B]->getType());
    auto *MatCTy = static_cast<SPIRVTypeJointMatrixINTEL *>(BArgs[C]->getType());

    auto *ResMatTy = static_cast<SPIRVTypeJointMatrixINTEL *>(BV->getType());

    const unsigned sizeM = MatATy->getRows();
    const unsigned sizeK = MatATy->getColumns();
    const unsigned sizeN = MatBTy->getColumns();

    IGC_ASSERT(sizeM == MatCTy->getRows());
    IGC_ASSERT(sizeN == MatCTy->getColumns());
    IGC_ASSERT(sizeK == MatBTy->getRows());

    IGC_ASSERT(ResMatTy->getRows()    == MatCTy->getRows());
    IGC_ASSERT(ResMatTy->getColumns() == MatCTy->getColumns());

    Type *RetTy  = transType(ResMatTy);
    Type *ATy    = transType(MatATy);
    Type *BTy    = transType(MatBTy);
    Type *CTy    = transType(MatCTy);

    std::vector<Type *> ArgTys = { ATy, BTy, CTy };
    FunctionType *builtinTy = FunctionType::get(RetTy, ArgTys, false);

    auto BI = static_cast<SPIRVInstruction *>(BV);
    std::string builtinName(getSPIRVBuiltinName(BV->getOpCode(), BI, ArgTys, ""));
    Function *Func = cast<Function>(M->getOrInsertFunction(builtinName, builtinTy));

    std::vector<Value *> Args = {
      transValue(BArgs[A], F, BB),
      transValue(BArgs[B], F, BB),
      transValue(BArgs[C], F, BB),
    };
    CallInst *CI = CallInst::Create(Func, Args, "matrix", BB);
    return mapValue(BV, CI);
  }
  default: {
    auto OC = BV->getOpCode();
    if (isSPIRVCmpInstTransToLLVMInst(static_cast<SPIRVInstruction*>(BV))) {
      return mapValue(BV, transCmpInst(BV, BB, F));
    } else if (isCvtOpCode(OC)) {
        auto BI = static_cast<SPIRVInstruction *>(BV);
        Value *Inst = nullptr;
        if (BI->hasFPRoundingMode() || BI->isSaturatedConversion() || OC == OpGenericCastToPtrExplicit)
           Inst = transSPIRVBuiltinFromInst(BI, BB);
        else
          Inst = transConvertInst(BV, F, BB);
        return mapValue(BV, Inst);
    } else if (OCLSPIRVBuiltinMap::find(OC) ||
               isIntelSubgroupOpCode(OC)) {
       return mapValue(BV, transSPIRVBuiltinFromInst(
          static_cast<SPIRVInstruction *>(BV), BB));
    } else if (isBinaryShiftLogicalBitwiseOpCode(OC) ||
                isLogicalOpCode(OC)) {
          return mapValue(BV, transShiftLogicalBitwiseInst(BV, BB, F));
    }
    return mapValue(BV, transSPIRVBuiltinFromInst(
      static_cast<SPIRVInstruction *>(BV), BB));
  }

  IGC_ASSERT_EXIT_MESSAGE(0, "Translation of SPIRV instruction not implemented");
  return NULL;
  }
}

template<class SourceTy, class FuncTy>
bool
SPIRVToLLVM::foreachFuncCtlMask(SourceTy Source, FuncTy Func) {
  SPIRVWord FCM = Source->getFuncCtlMask();
  SPIRSPIRVFuncCtlMaskMap::foreach([&](Attribute::AttrKind Attr,
      SPIRVFunctionControlMaskKind Mask){
    if (FCM & Mask)
      Func(Attr);
  });
  return true;
}

Function *
SPIRVToLLVM::transFunction(SPIRVFunction *BF) {
  auto Loc = FuncMap.find(BF);
  if (Loc != FuncMap.end())
    return Loc->second;

  auto IsKernel = BM->isEntryPoint(ExecutionModelKernel, BF->getId());

  if (IsKernel) {
      // search for a previous function with the same name
      // upgrade it to a kernel and drop this if it's found
      for (auto& I : FuncMap) {
          auto BFName = I.getFirst()->getName();
          if (BF->getName() == BFName) {
              auto* F = I.getSecond();
              F->setCallingConv(CallingConv::SPIR_KERNEL);
              F->setLinkage(GlobalValue::ExternalLinkage);
              F->setDSOLocal(false);
              F = cast<Function>(mapValue(BF, F));
              mapFunction(BF, F);
              return F;
          }
      }
  }

  auto Linkage = IsKernel ? GlobalValue::ExternalLinkage :
      transLinkageType(BF);
  FunctionType *FT = dyn_cast<FunctionType>(transType(BF->getFunctionType()));
  Function *F = dyn_cast<Function>(mapValue(BF, Function::Create(FT, Linkage,
      BF->getName(), M)));
  mapFunction(BF, F);
  if (BF->hasDecorate(DecorationReferencedIndirectlyINTEL))
    F->addFnAttr("referenced-indirectly");
  if (!F->isIntrinsic()) {
    F->setCallingConv(IsKernel ? CallingConv::SPIR_KERNEL :
        CallingConv::SPIR_FUNC);
    if (isFuncNoUnwind())
      F->addFnAttr(Attribute::NoUnwind);
    foreachFuncCtlMask(BF, [&](Attribute::AttrKind Attr){
      F->addFnAttr(Attr);
    });
  }

  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
      ++I) {
    auto BA = BF->getArgument(I->getArgNo());
    mapValue(BA, &(*I));
    const std::string &ArgName = BA->getName();
    if (!ArgName.empty())
      I->setName(ArgName);
    BA->foreachAttr([&](SPIRVFuncParamAttrKind Kind){
     if (Kind == FunctionParameterAttributeCount)
        return;
#if LLVM_VERSION_MAJOR >= 12
     Attribute::AttrKind LLVMKind = SPIRSPIRVFuncParamAttrMap::rmap(Kind);
     Type *AttrTy = nullptr;
     if (LLVMKind == Attribute::AttrKind::ByVal)
       AttrTy = cast<PointerType>(I->getType())->getElementType();
     else if (LLVMKind == Attribute::AttrKind::StructRet)
       AttrTy = I->getType();
     // Make sure to use a correct constructor for a typed/typeless attribute
     auto A = AttrTy ? Attribute::get(*Context, LLVMKind, AttrTy)
                     : Attribute::get(*Context, LLVMKind);
     I->addAttr(A);
#else
     F->addAttribute(I->getArgNo() + 1, SPIRSPIRVFuncParamAttrMap::rmap(Kind));
#endif
    });
  }
  BF->foreachReturnValueAttr([&](SPIRVFuncParamAttrKind Kind){
    if (Kind == FunctionParameterAttributeCount)
      return;
    IGCLLVM::addRetAttr(F, SPIRSPIRVFuncParamAttrMap::rmap(Kind));
  });

  // Creating all basic blocks before creating instructions.
  for (size_t I = 0, E = BF->getNumBasicBlock(); I != E; ++I) {
    transValue(BF->getBasicBlock(I), F, nullptr, true, BoolAction::Noop);
  }

  for (size_t I = 0, E = BF->getNumBasicBlock(); I != E; ++I) {
    SPIRVBasicBlock *BBB = BF->getBasicBlock(I);
    BasicBlock *BB = dyn_cast<BasicBlock>(transValue(BBB, F, nullptr, true, BoolAction::Noop));
    for (size_t BI = 0, BE = BBB->getNumInst(); BI != BE; ++BI) {
      SPIRVInstruction *BInst = BBB->getInst(BI);
      transValue(BInst, F, BB, false, BoolAction::Noop);
    }
  }

  transLLVMLoopMetadata(F);

  return F;
}

Value *SPIRVToLLVM::transAsmINTEL(SPIRVAsmINTEL *BA, Function *F, BasicBlock *BB) {
  bool HasSideEffect = BA->hasDecorate(DecorationSideEffectsINTEL);
  return InlineAsm::get(
      dyn_cast<FunctionType>(transType(BA->getFunctionType())),
      BA->getInstructions(), BA->getConstraints(), HasSideEffect,
      /* IsAlignStack */ false, InlineAsm::AsmDialect::AD_ATT);
}

CallInst *SPIRVToLLVM::transAsmCallINTEL(SPIRVAsmCallINTEL *BI, Function *F,
                                         BasicBlock *BB) {
  auto *IA = cast<InlineAsm>(transValue(BI->getAsm(), F, BB));
  auto Args = transValue(BM->getValues(BI->getArguments()), F, BB);
  return CallInst::Create(IA, Args, BI->getName(), BB);
}

uint64_t SPIRVToLLVM::calcImageType(const SPIRVValue *ImageVal)
{
  const SPIRVTypeImage* TI = nullptr;
  if (ImageVal->getType()->isTypeSampledImage()) {
    TI = static_cast<SPIRVTypeSampledImage*>(ImageVal->getType())->getImageType();
  }
  else if (ImageVal->getType()->isTypeVmeImageINTEL()) {
    TI = static_cast<SPIRVTypeVmeImageINTEL*>(ImageVal->getType())->getImageType();
  }
  else {
    TI = static_cast<SPIRVTypeImage*>(ImageVal->getType());
  }

  const auto &Desc = TI->getDescriptor();
  uint64_t ImageType = 0;

  ImageType |= ((uint64_t)Desc.Dim                 & 0x7) << 59;
  ImageType |= ((uint64_t)Desc.Depth               & 0x1) << 58;
  ImageType |= ((uint64_t)Desc.Arrayed             & 0x1) << 57;
  ImageType |= ((uint64_t)Desc.MS                  & 0x1) << 56;
  ImageType |= ((uint64_t)Desc.Sampled             & 0x3) << 62;
  ImageType |= ((uint64_t)TI->getAccessQualifier() & 0x3) << 54;

  return ImageType;
}

Instruction *
SPIRVToLLVM::transSPIRVBuiltinFromInst(SPIRVInstruction *BI, BasicBlock *BB) {
  IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  const auto OC = BI->getOpCode();
  auto Ops = BI->getOperands();
  // builtins use bool for scalar and ucharx for vector bools.  Truncate
  // or promote as necessary.
  std::vector<Value *> operands;
  for (auto I : Ops)
  {
      BoolAction Action = I->getType()->isTypeBool() ?
          BoolAction::Truncate :
          BoolAction::Promote;
      operands.push_back(transValue(I, BB->getParent(), BB, true, Action));
  }

  bool hasReturnTypeInTypeList = false;

  std::string suffix;
  if (isCvtOpCode(OC))
  {
      hasReturnTypeInTypeList = true;

      if (BI->isSaturatedConversion() &&
          !(BI->getOpCode() == OpSatConvertSToU || BI->getOpCode() == OpSatConvertUToS))
      {
          suffix += "_Sat";
      }

      SPIRVFPRoundingModeKind kind;
      std::string rounding_string;
      if (BI->hasFPRoundingMode(&kind))
      {
          switch (kind)
          {
              case FPRoundingModeRTE:
                  rounding_string = "_RTE";
                  break;
              case FPRoundingModeRTZ:
                  rounding_string = "_RTZ";
                  break;
              case FPRoundingModeRTP:
                  rounding_string = "_RTP";
                  break;
              case FPRoundingModeRTN:
                  rounding_string = "_RTN";
                  break;
              default:
                  break;
          }
      }
      suffix += rounding_string;
  }
  std::vector<Type*> ArgTys;
  for (auto &v : operands)
  {
      // replace function by function pointer
      auto *ArgTy = v->getType()->isFunctionTy() ?
          v->getType()->getPointerTo() :
          v->getType();

      ArgTys.push_back(ArgTy);
  }

  // OpImageSampleExplicitLod: SImage | Coordinate | ImageOperands | LOD
  // OpImageWrite:             Image  | Image Type | Coordinate | Texel
  // OpImageRead:              Image  | Image Type | Coordinate

  // Look for opaque image pointer operands and convert it with an i64 type
  bool convertImageToI64 =
      (OC != OpSubgroupImageBlockReadINTEL &&
       OC != OpSubgroupImageBlockWriteINTEL &&
       OC != OpImageRead &&
       OC != OpImageWrite &&
       OC != OpImageQueryFormat &&
       OC != OpImageQueryOrder &&
       OC != OpImageQuerySizeLod &&
       OC != OpImageQuerySize &&
       OC != OpImageQueryLevels &&
       OC != OpImageQuerySamples &&
       OC != OpVmeImageINTEL &&
       OC != OpSampledImage &&
       OC != OpImageSampleExplicitLod &&
       OC != OpSubgroupImageMediaBlockReadINTEL &&
       OC != OpSubgroupImageMediaBlockWriteINTEL);

  if (convertImageToI64)
  {
      for (auto i = 0U; i < BI->getOperands().size(); i++) {

          SPIRVValue* imagePtr = BI->getOperands()[i];

          if (imagePtr->getType()->isTypeImage())
          {
              IGC_ASSERT(isa<PointerType>(transType(imagePtr->getType())));

              Value* ImageArgVal = llvm::PtrToIntInst::Create(
                  Instruction::PtrToInt,
                  transValue(imagePtr, BB->getParent(), BB),
                  Type::getInt64Ty(*Context),
                  "ImageArgVal",
                  BB);

              // replace opaque pointer type with i64 type
              IGC_ASSERT(ArgTys[i] == transType(imagePtr->getType()));
              ArgTys[i] = ImageArgVal->getType();
              operands[i] = ImageArgVal;
          }
      }
  }

  if (isImageOpCode(OC) && OC != OpImageRead && OC != OpImageWrite && OC != OpImageSampleExplicitLod)
  {
      hasReturnTypeInTypeList = true;

      // need to widen coordinate type
      SPIRVValue* coordinate = BI->getOperands()[1];
      Type* coordType = transType(coordinate->getType());

      Value *imageCoordinateWiden = nullptr;
      if (!isa<VectorType>(coordType))
      {
          Value *undef = UndefValue::get(IGCLLVM::FixedVectorType::get(coordType, 4));

          imageCoordinateWiden = InsertElementInst::Create(
              undef,
              transValue(coordinate, BB->getParent(), BB),
              ConstantInt::get(Type::getInt32Ty(*Context), 0),
              "",
              BB);
      }
      else if (cast<IGCLLVM::FixedVectorType>(coordType)->getNumElements() != 4)
      {
          Value *undef = UndefValue::get(coordType);

          SmallVector<Constant*, 4> shuffleIdx;
          for (unsigned i = 0; i < cast<IGCLLVM::FixedVectorType>(coordType)->getNumElements(); i++)
              shuffleIdx.push_back(ConstantInt::get(Type::getInt32Ty(*Context), i));

          for (uint64_t i = (unsigned)cast<IGCLLVM::FixedVectorType>(coordType)->getNumElements(); i < 4; i++)
              shuffleIdx.push_back(ConstantInt::get(Type::getInt32Ty(*Context), 0));

          imageCoordinateWiden = new ShuffleVectorInst(
              transValue(coordinate, BB->getParent(), BB),
              undef,
              ConstantVector::get(shuffleIdx),
              "",
              BB);
      }

      if (imageCoordinateWiden != nullptr)
      {
          const  uint32_t  CoordArgIdx = (OC == OpImageSampleExplicitLod) ? 1 : 2;
          ArgTys[CoordArgIdx] = imageCoordinateWiden->getType();
          operands[CoordArgIdx] = imageCoordinateWiden;
      }
  }

  if (OC == OpImageQuerySizeLod ||
      OC == OpImageQuerySize ||
      OC == OpSubgroupBlockReadINTEL ||
      OC == OpSubgroupImageBlockReadINTEL ||
      OC == OpImageRead ||
      OC == OpImageSampleExplicitLod ||
      OC == OpSubgroupImageMediaBlockReadINTEL)
  {
      hasReturnTypeInTypeList = true;
  }

  Type *RetTy = Type::getVoidTy(*Context);
  if (BI->hasType())
  {
      auto *pTrans = transType(BI->getType());
      RetTy = BI->getType()->isTypeBool() ?
          truncBoolType(BI->getType(), pTrans) :
          pTrans;

  }

  if (hasReturnTypeInTypeList)
  {
      ArgTys.insert(ArgTys.begin(), RetTy);
  }

  std::string builtinName(getSPIRVBuiltinName(OC, BI, ArgTys, suffix));

  if (hasReturnTypeInTypeList)
  {
      ArgTys.erase(ArgTys.begin());
  }

  Function* Func = M->getFunction(builtinName);
  FunctionType* FT = FunctionType::get(RetTy, ArgTys, false);
  if (!Func || Func->getFunctionType() != FT)
  {
     Func = Function::Create(FT, GlobalValue::ExternalLinkage, builtinName, M);
     Func->setCallingConv(CallingConv::SPIR_FUNC);
     if (isFuncNoUnwind())
        Func->addFnAttr(Attribute::NoUnwind);
  }

  auto Call = CallInst::Create(Func, operands, "", BB);

  Call->setName(BI->getName());
  setAttrByCalledFunc(Call);
  return Call;
}

Type* SPIRVToLLVM::getNamedBarrierType()
{
    if (!m_NamedBarrierType)
    {
        llvm::SmallVector<Type*, 3> NamedBarrierSturctType(3, Type::getInt32Ty(*Context));
        m_NamedBarrierType = StructType::create(*Context, NamedBarrierSturctType, "struct.__namedBarrier")->getPointerTo(SPIRAS_Local);
    }
    return m_NamedBarrierType;
}

bool
SPIRVToLLVM::translate() {
  if (!transAddressingModel())
    return false;

  compileUnit = DbgTran.createCompileUnit();

  for (unsigned I = 0, E = BM->getNumVariables(); I != E; ++I) {
    auto BV = BM->getVariable(I);
    if (BV->getStorageClass() != StorageClassFunction)
      transValue(BV, nullptr, nullptr, true, BoolAction::Noop);
  }

  for (unsigned I = 0, E = BM->getNumFunctions(); I != E; ++I) {
    transFunction(BM->getFunction(I));
  }
  for(auto& funcs : FuncMap)
  {
      auto diSP = getDbgTran().getDISP(funcs.first->getId());
      if (diSP)
          funcs.second->setSubprogram(diSP);
  }
  if (!transKernelMetadata())
    return false;
  if (!transFPContractMetadata())
    return false;
  transSourceLanguage();
  if (!transSourceExtension())
    return false;
  if (!transOCLBuiltinsFromVariables())
    return false;
  if (!postProcessOCL())
    return false;

  DbgTran.transGlobals();

  DbgTran.transImportedEntities();

  DbgTran.finalize();
  return true;
}

bool
SPIRVToLLVM::transAddressingModel() {
  switch (BM->getAddressingModel()) {
  case AddressingModelPhysical64:
    M->setTargetTriple(SPIR_TARGETTRIPLE64);
    M->setDataLayout(SPIR_DATALAYOUT64);
    break;
  case AddressingModelPhysical32:
    M->setTargetTriple(SPIR_TARGETTRIPLE32);
    M->setDataLayout(SPIR_DATALAYOUT32);
    break;
  case AddressingModelLogical:
    // Do not set target triple and data layout
    break;
  default:
    SPIRVCKRT(0, InvalidAddressingModel, "Actual addressing mode is " +
        (unsigned)BM->getAddressingModel());
  }
  return true;
}

bool
SPIRVToLLVM::transDecoration(SPIRVValue *BV, Value *V) {
  if (!transAlign(BV, V))
    return false;
  transMemAliasingINTELDecorations(BV, V);
  transHostAccessINTELDecorations(BV, V);
  DbgTran.transDbgInfo(BV, V);
  return true;
}

bool
SPIRVToLLVM::transFPContractMetadata() {
  bool ContractOff = false;
  for (unsigned I = 0, E = BM->getNumFunctions(); I != E; ++I) {
    SPIRVFunction *BF = BM->getFunction(I);
    if (!isOpenCLKernel(BF))
      continue;
    if (BF->getExecutionMode(ExecutionModeContractionOff)) {
      ContractOff = true;
      break;
    }
  }
  if (!ContractOff)
    M->getOrInsertNamedMetadata(igc_spv::kSPIR2MD::FPContract);
  return true;
}

std::string SPIRVToLLVM::transOCLImageTypeAccessQualifier(
    igc_spv::SPIRVTypeImage* ST) {
  return SPIRSPIRVAccessQualifierMap::rmap(ST->getAccessQualifier());
}

Type *
SPIRVToLLVM::decodeVecTypeHint(LLVMContext &C, unsigned code) {
    unsigned VecWidth = code >> 16;
    unsigned Scalar = code & 0xFFFF;
    Type *ST = nullptr;
    switch (Scalar) {
    case 0:
    case 1:
    case 2:
    case 3:
        ST = IntegerType::get(C, 1 << (3 + Scalar));
        break;
    case 4:
        ST = Type::getHalfTy(C);
        break;
    case 5:
        ST = Type::getFloatTy(C);
        break;
    case 6:
        ST = Type::getDoubleTy(C);
        break;
    default:
        llvm_unreachable("Invalid vec type hint");
    }
    if (VecWidth < 1)
        return ST;
    return IGCLLVM::FixedVectorType::get(ST, VecWidth);
}

// Information of types of kernel arguments may be additionally stored in
// 'OpString "kernel_arg_type.%kernel_name%.type1,type2,type3,..' instruction.
// Try to find such instruction and generate metadata based on it.
static bool transKernelArgTypeMedataFromString(
    LLVMContext *Ctx, std::vector<llvm::Metadata*> &KernelMD, SPIRVModule *BM,
    Function *Kernel, std::string MDName)
    {
    std::string ArgTypePrefix =
        std::string(MDName) + "." + Kernel->getName().str() + ".";
    auto ArgTypeStrIt = std::find_if(
        BM->getStringVec().begin(), BM->getStringVec().end(),
        [=](SPIRVString *S) { return S->getStr().find(ArgTypePrefix) == 0; });

    if (ArgTypeStrIt == BM->getStringVec().end())
        return false;

    std::string ArgTypeStr =
        (*ArgTypeStrIt)->getStr().substr(ArgTypePrefix.size());
    std::vector<Metadata *> TypeMDs;
    TypeMDs.push_back(MDString::get(*Ctx, MDName));

    int countBraces = 0;
    std::string::size_type start = 0;
    for (std::string::size_type i = 0; i < ArgTypeStr.length(); i++) {
        switch (ArgTypeStr[i]) {
        case '<':
            countBraces++;
            break;
        case '>':
            countBraces--;
            break;
        case ',':
            if (countBraces == 0) {
                TypeMDs.push_back(MDString::get(*Ctx, ArgTypeStr.substr(start, i - start)));
                start = i + 1;
            }
        }
    }

    KernelMD.push_back(MDNode::get(*Ctx, TypeMDs));
    return true;
}

// Some of the metadata may disappear when linking LLVM modules; attributes are much more permament.
static void convertAnnotaionsToAttributes(llvm::Function *F, const std::vector<std::string> &annotations)
{
    for (const std::string &annotation : annotations)
    {
        if (annotation == "igc-force-stackcall")
        {
            F->addFnAttr("igc-force-stackcall");
        }
        else if (annotation == "sycl-unmasked")
        {
            F->addFnAttr("sycl-unmasked");
        }
    }
}

bool
SPIRVToLLVM::transNonTemporalMetadata(Instruction* I) {
    Constant* One = ConstantInt::get(Type::getInt32Ty(*Context), 1);
    MDNode* Node = MDNode::get(*Context, ConstantAsMetadata::get(One));
    I->setMetadata(M->getMDKindID("nontemporal"), Node);
    return true;
}

void
SPIRVToLLVM::transCapsIntoMetadata(IGC::ModuleMetaData& MD) {
  for (auto& Cap : BM->getCapability())
  {
    if (Cap == CapabilityGlobalVariableDecorationsINTEL)
      MD.capabilities.globalVariableDecorationsINTEL = true;
  }
}

bool
SPIRVToLLVM::transKernelMetadata()
{
    IGC::ModuleMetaData MD;

    // TODO: Once IGC is switched to use Khronos SPIRV-LLVM Translator,
    // special API to query capabilities should be used to produce metadata.
    // (https://github.com/KhronosGroup/SPIRV-LLVM-Translator/pull/1178)
    transCapsIntoMetadata(MD);

    NamedMDNode *KernelMDs = M->getOrInsertNamedMetadata(SPIR_MD_KERNELS);
    for (unsigned I = 0, E = BM->getNumFunctions(); I != E; ++I)
    {
        SPIRVFunction *BF = BM->getFunction(I);
        Function *F = static_cast<Function *>(getTranslatedValue(BF));
        IGC_ASSERT_MESSAGE(F, "Invalid translated function");

        // __attribute__((annotate("some_user_annotation"))) are passed via
        // UserSemantic decoration on functions.
        if (BF->hasDecorate(DecorationUserSemantic)) {
          auto &funcInfo = MD.FuncMD[F];
          funcInfo.UserAnnotations = BF->getDecorationStringLiteral(DecorationUserSemantic);
          convertAnnotaionsToAttributes(F, funcInfo.UserAnnotations);
        }

        if (F->getCallingConv() != CallingConv::SPIR_KERNEL || F->isDeclaration())
            continue;
        std::vector<llvm::Metadata*> KernelMD;
        KernelMD.push_back(ValueAsMetadata::get(F));

        // Generate metadata for kernel_arg_address_spaces
        addOCLKernelArgumentMetadata(Context, KernelMD,
            SPIR_MD_KERNEL_ARG_ADDR_SPACE, BF,
            [=](SPIRVFunctionParameter *Arg){
            SPIRVType *ArgTy = Arg->getType();
            SPIRAddressSpace AS = SPIRAS_Private;
            if (ArgTy->isTypePointer())
                AS = SPIRSPIRVAddrSpaceMap::rmap(ArgTy->getPointerStorageClass());
            else if (ArgTy->isTypeOCLImage() || ArgTy->isTypePipe())
                AS = SPIRAS_Global;
            return ConstantAsMetadata::get(
                ConstantInt::get(Type::getInt32Ty(*Context), AS));
        });
        // Generate metadata for kernel_arg_access_qual
        addOCLKernelArgumentMetadata(Context, KernelMD,
            SPIR_MD_KERNEL_ARG_ACCESS_QUAL, BF,
            [=](SPIRVFunctionParameter *Arg){
            std::string Qual;
            auto T = Arg->getType();
            if (T->isTypeOCLImage()) {
                auto ST = static_cast<SPIRVTypeImage *>(T);
                Qual = transOCLImageTypeAccessQualifier(ST);
            }
            else if (T->isTypePipe()){
                auto PT = static_cast<SPIRVTypePipe *>(T);
                Qual = transOCLPipeTypeAccessQualifier(PT);
            }
            else
                Qual = "none";
            return MDString::get(*Context, Qual);
        });
        // Generate metadata for kernel_arg_type
        if (!transKernelArgTypeMedataFromString(Context, KernelMD, BM, F,
                                                SPIR_MD_KERNEL_ARG_TYPE)) {
            addOCLKernelArgumentMetadata(Context, KernelMD,
                SPIR_MD_KERNEL_ARG_TYPE, BF,
                [=](SPIRVFunctionParameter *Arg) {
                return transOCLKernelArgTypeName(Arg);
            });
        }
        if (!transKernelArgTypeMedataFromString(Context, KernelMD, BM, F,
                                                SPIR_MD_KERNEL_ARG_TYPE_QUAL))
        // Generate metadata for kernel_arg_type_qual
        addOCLKernelArgumentMetadata(Context, KernelMD,
            SPIR_MD_KERNEL_ARG_TYPE_QUAL, BF,
            [=](SPIRVFunctionParameter *Arg){
            std::string Qual;
            if (Arg->hasDecorate(DecorationVolatile))
                Qual = kOCLTypeQualifierName::Volatile;
            Arg->foreachAttr([&](SPIRVFuncParamAttrKind Kind){
                Qual += Qual.empty() ? "" : " ";
                switch (Kind){
                case FunctionParameterAttributeNoAlias:
                    Qual += kOCLTypeQualifierName::Restrict;
                    break;
                case FunctionParameterAttributeNoWrite:
                    Qual += kOCLTypeQualifierName::Const;
                    break;
                default:
                    // do nothing.
                    break;
                }
            });
            if (Arg->getType()->isTypePipe()) {
                Qual += Qual.empty() ? "" : " ";
                Qual += kOCLTypeQualifierName::Pipe;
            }
            return MDString::get(*Context, Qual);
        });
        // Generate metadata for kernel_arg_base_type
        addOCLKernelArgumentMetadata(Context, KernelMD,
            SPIR_MD_KERNEL_ARG_BASE_TYPE, BF,
            [=](SPIRVFunctionParameter *Arg){
            return transOCLKernelArgTypeName(Arg);
        });
        // Generate metadata for kernel_arg_name
        bool ArgHasName = true;
        BF->foreachArgument([&](SPIRVFunctionParameter *Arg) {
            ArgHasName &= !Arg->getName().empty();
        });
        if (ArgHasName)
            addOCLKernelArgumentMetadata(Context, KernelMD,
                SPIR_MD_KERNEL_ARG_NAME, BF,
                [=](SPIRVFunctionParameter *Arg) {
            return MDString::get(*Context, Arg->getName());
        });
        // Generate metadata for reqd_work_group_size
        if (auto EM = BF->getExecutionMode(ExecutionModeLocalSize)) {
            KernelMD.push_back(getMDNodeStringIntVec(Context,
                igc_spv::kSPIR2MD::WGSize, EM->getLiterals()));
        }
        // Generate metadata for work_group_size_hint
        if (auto EM = BF->getExecutionMode(ExecutionModeLocalSizeHint)) {
            KernelMD.push_back(getMDNodeStringIntVec(Context,
                igc_spv::kSPIR2MD::WGSizeHint, EM->getLiterals()));
        }
        // Generate metadata for vec_type_hint
        if (auto EM = BF->getExecutionMode(ExecutionModeVecTypeHint)) {
            std::vector<Metadata*> MetadataVec;
            MetadataVec.push_back(MDString::get(*Context, igc_spv::kSPIR2MD::VecTyHint));
            Type *VecHintTy = decodeVecTypeHint(*Context, EM->getLiterals()[0]);
            MetadataVec.push_back(ValueAsMetadata::get(UndefValue::get(VecHintTy)));
            MetadataVec.push_back(
                ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(*Context),
                0)));
            KernelMD.push_back(MDNode::get(*Context, MetadataVec));
        }

        auto &funcInfo = MD.FuncMD[F];

        // Generate metadata for initializer
        if (BF->getExecutionMode(ExecutionModeInitializer)) {
            funcInfo.IsInitializer = true;
        }

        // Generate metadata for finalizer
        if (BF->getExecutionMode(ExecutionModeFinalizer)) {
            funcInfo.IsFinalizer = true;
        }

        // Generate metadata for SubgroupSize
        if (auto EM = BF->getExecutionMode(ExecutionModeSubgroupSize))
        {
            unsigned subgroupSize = EM->getLiterals()[0];
            std::vector<Metadata*> MetadataVec;
            MetadataVec.push_back(MDString::get(*Context, igc_spv::kSPIR2MD::ReqdSubgroupSize));
            MetadataVec.push_back(
                ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(*Context),
                subgroupSize)));
            KernelMD.push_back(MDNode::get(*Context, MetadataVec));
        }

        // Generate metadata for SubgroupsPerWorkgroup
        if (auto EM = BF->getExecutionMode(ExecutionModeSubgroupsPerWorkgroup))
        {
            funcInfo.CompiledSubGroupsNumber = EM->getLiterals()[0];
        }

        // Generate metadata for MaxByteOffset decorations
        {
            bool ArgHasMaxByteOffset = false;
            BF->foreachArgument([&](SPIRVFunctionParameter *Arg)
            {
                SPIRVWord offset;
                ArgHasMaxByteOffset |= Arg->hasMaxByteOffset(offset);
            });

            if (ArgHasMaxByteOffset)
            {
                BF->foreachArgument([&](SPIRVFunctionParameter *Arg)
                {
                    SPIRVWord offset;
                    bool ok = Arg->hasMaxByteOffset(offset);
                    // If the decoration is not present on an argument of the function,
                    // encode that as a zero in the metadata.  That currently seems
                    // like a degenerate case wouldn't be worth optimizing.
                    unsigned val = ok ? offset : 0;
                    funcInfo.maxByteOffsets.push_back(val);
                });
            }
        }

        llvm::MDNode *Node = MDNode::get(*Context, KernelMD);
        KernelMDs->addOperand(Node);
    }

    IGC::serialize(MD, M);

    return true;
}

bool
SPIRVToLLVM::transAlign(SPIRVValue *BV, Value *V) {
  if (auto AL = dyn_cast<AllocaInst>(V)) {
    SPIRVWord Align = 0;
    if (BV->hasAlignment(&Align))
      AL->setAlignment(IGCLLVM::getCorrectAlign(Align));
    return true;
  }
  if (auto GV = dyn_cast<GlobalVariable>(V)) {
    SPIRVWord Align = 0;
    if (BV->hasAlignment(&Align))
      GV->setAlignment(IGCLLVM::getCorrectAlign(Align));
    return true;
  }
  return true;
}

void
SPIRVToLLVM::transOCLVectorLoadStore(std::string& UnmangledName,
    std::vector<SPIRVWord> &BArgs) {
  if (UnmangledName.find("vload") == 0 &&
      UnmangledName.find("n") != std::string::npos) {
    if (BArgs.back() > 1) {
      UnmangledName.replace(UnmangledName.find("n"), 1, std::to_string(BArgs.back()));
    } else {
      UnmangledName.erase(UnmangledName.find("n"), 1);
    }
  } else if (UnmangledName.find("vstore") == 0) {
    if (UnmangledName.find("n") != std::string::npos) {
      auto T = BM->getValueType(BArgs[0]);
      if (T->isTypeVector()) {
        auto W = T->getVectorComponentCount();
        UnmangledName.replace(UnmangledName.find("n"), 1, std::to_string(W));
      } else {
        UnmangledName.erase(UnmangledName.find("n"), 1);
      }
    }
   }
}

Instruction* SPIRVToLLVM::transDebugInfo(SPIRVExtInst* BC, BasicBlock* BB)
{
    if (!BC)
        return nullptr;

    auto extOp = (OCLExtOpDbgKind)BC->getExtOp();

    switch (extOp)
    {
    case OCLExtOpDbgKind::DbgDcl:
    {
        OpDebugDeclare dbgDcl(BC);
        auto lvar = dbgDcl.getLocalVar();
        SPIRVValue* spirvVal = static_cast<SPIRVValue*>(BM->getEntry(lvar));
        SPIRVToLLVMValueMap::iterator Loc = ValueMap.find(spirvVal);
        if (Loc != ValueMap.end())
        {
            return DbgTran.createDbgDeclare(BC, Loc->second, BB);
        }
        break;
    }

    case OCLExtOpDbgKind::DbgVal:
    {
        OpDebugValue dbgValue(BC);
        llvm::Value* Value = nullptr;
        auto lvar = dbgValue.getValueVar();
        SPIRVValue* spirvVal = static_cast<SPIRVValue*>(BM->getEntry(lvar));
        if (spirvVal->getOpCode() == Op::OpConstant)
        {
            Value = transValueWithoutDecoration(spirvVal, BB->getParent(), BB, false);
        }
        else {
            SPIRVToLLVMValueMap::iterator Loc = ValueMap.find(spirvVal);
            if (Loc != ValueMap.end())
            {
                SPIRVToLLVMValueMap::iterator Loc = ValueMap.find(spirvVal);
                Value = Loc->second;
            }
        }
        if(Value)
        {
            return DbgTran.createDbgValue(BC, Value, BB);
        }
        break;
    }

    default:
        break;
    }

    return nullptr;
}

Instruction *
SPIRVToLLVM::transOCLBuiltinFromExtInst(SPIRVExtInst *BC, BasicBlock *BB) {
  IGC_ASSERT_MESSAGE(BB, "Invalid BB");
  SPIRVWord EntryPoint = BC->getExtOp();
  SPIRVExtInstSetKind Set = BM->getBuiltinSet(BC->getExtSetId());
  bool IsPrintf = false;
  std::string FuncName;

  if (Set == SPIRVEIS_DebugInfo ||
      Set == SPIRVEIS_OpenCL_DebugInfo_100)
  {
      return transDebugInfo(BC, BB);
  }

  IGC_ASSERT_MESSAGE(Set == SPIRVEIS_OpenCL, "Not OpenCL extended instruction");
  if (EntryPoint == igc_OpenCLLIB::printf)
    IsPrintf = true;
  else {
      FuncName = OCLExtOpMap::map(static_cast<OCLExtOpKind>(
        EntryPoint));
  }

  auto BArgs = BC->getArguments();
  transOCLVectorLoadStore(FuncName, BArgs);

  // keep builtin functions written with bool as i1, truncate down if necessary.
  auto Args = transValue(BC->getValues(BArgs), BB->getParent(), BB, BoolAction::Truncate);
  std::vector<Type*> ArgTypes;
  for (auto &v : Args)
  {
      ArgTypes.push_back(v->getType());
  }

  bool IsVarArg = false;
  if (IsPrintf)
  {
      FuncName = "printf";
      IsVarArg = true;
      ArgTypes.resize(1);
  }
  else
  {
      decorateSPIRVExtInst(FuncName, ArgTypes);
  }

  FunctionType *FT = FunctionType::get(
      truncBoolType(BC->getType(), transType(BC->getType())),
      ArgTypes,
      IsVarArg);
  Function *F = M->getFunction(FuncName);
  if (!F) {
    F = Function::Create(FT,
      GlobalValue::ExternalLinkage,
      FuncName,
      M);
    F->setCallingConv(CallingConv::SPIR_FUNC);
    if (isFuncNoUnwind())
      F->addFnAttr(Attribute::NoUnwind);
  }
  CallInst *Call = CallInst::Create(F,
      Args,
      BC->getName(),
      BB);
  setCallingConv(Call);
  IGCLLVM::addFnAttr(Call, Attribute::NoUnwind);
  return Call;
}

// SPIR-V only contains language version. Use OpenCL language version as
// SPIR version.
void
SPIRVToLLVM::transSourceLanguage() {
  SPIRVWord Ver = 0;
  SpvSourceLanguage Lang = BM->getSourceLanguage(&Ver);
  if (Lang == SpvSourceLanguageOpenCL_C || Lang == SpvSourceLanguageOpenCL_CPP) {
    unsigned short Major = 0;
    unsigned char Minor = 0;
    unsigned char Rev = 0;
    std::tie(Major, Minor, Rev) = decodeOCLVer(Ver);
    addOCLVersionMetadata(Context, M, kSPIR2MD::SPIRVer, Major, Minor);
    addOCLVersionMetadata(Context, M, kSPIR2MD::OCLVer, Major, Minor);
  }
}

bool
SPIRVToLLVM::transSourceExtension() {
  auto ExtSet = rmap<OclExt::Kind>(BM->getExtension());
  auto CapSet = rmap<OclExt::Kind>(BM->getCapability());
  for (auto &I:CapSet)
    ExtSet.insert(I);
  auto OCLExtensions = getStr(map<std::string>(ExtSet));
  std::string OCLOptionalCoreFeatures;
  static const char * const OCLOptCoreFeatureNames[] = {
      "cl_images",
      "cl_doubles",
  };
  for (auto &I:OCLOptCoreFeatureNames) {
    size_t Loc = OCLExtensions.find(I);
    if (Loc != std::string::npos) {
      OCLExtensions.erase(Loc, std::strlen(I));
      if (!OCLOptionalCoreFeatures.empty())
        OCLOptionalCoreFeatures += ' ';
      OCLOptionalCoreFeatures += I;
    }
  }
  addNamedMetadataString(Context, M, kSPIR2MD::Extensions, OCLExtensions);
  addNamedMetadataString(Context, M, kSPIR2MD::OptFeatures,
      OCLOptionalCoreFeatures);
  return true;
}

__attr_unused static void dumpSPIRVBC(const char* fname, const char* data, unsigned int size)
{
    FILE* fp;
    fp = fopen(fname, "wb");
    if(fp != NULL) {
      fwrite(data, 1, size, fp);
      fclose(fp);
    }
}

bool ReadSPIRV(LLVMContext &C, std::istream &IS, Module *&M,
    std::string &ErrMsg,
    std::unordered_map<uint32_t, uint64_t> *specConstants) {
  std::unique_ptr<SPIRVModule> BM( SPIRVModule::createSPIRVModule() );
  BM->setSpecConstantMap(specConstants);
  IS >> *BM;
  bool Succeed = BM->getError(ErrMsg) == SPIRVEC_Success;
  if (Succeed) {
    BM->resolveUnknownStructFields();
    M = new Module("", C);
    SPIRVToLLVM BTL(M, BM.get());

    if (!BTL.translate()) {
      BM->getError(ErrMsg);
      Succeed = false;
    }

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
    if (DbgSaveTmpLLVM)
      dumpLLVM(M, DbgTmpLLVMFileName);
#endif
  }
  if (!Succeed) {
    delete M;
    M = nullptr;
  }
  return Succeed;
}

}

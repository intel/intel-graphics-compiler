/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Bitcode/BitcodeWriter.h"
#include <llvm/Bitcode/BitcodeReader.h>
#include "llvm/IR/DebugInfo.h"
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/debug/Dump.hpp"
#include "common/secure_mem.h"
#include "Compiler/GenUpdateCB.h"
#include "Compiler/CISACodeGen/helper.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include <iStdLib/MemCopy.h>
#include "common/LLVMUtils.h"
#include "Probe/Assertion.h"

char IGC::GenUpdateCB::ID = 0;

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(GenUpdateCB, "GenUpdateCB", "GenUpdateCB", false, false)
IGC_INITIALIZE_PASS_END(GenUpdateCB, "GenUpdateCB", "GenUpdateCB", false, false)

static bool isResInfo(GenIntrinsicInst *inst, unsigned &texId, unsigned &lod, bool &isUAV) {
  if (inst && inst->getIntrinsicID() == GenISAIntrinsic::GenISA_resinfoptr) {
    ConstantInt *vlod = dyn_cast<ConstantInt>(inst->getOperand(1));
    if (!vlod)
      return false;

    Value *texOp = inst->getOperand(0);
    BufferType bufType;
    unsigned as = texOp->getType()->getPointerAddressSpace();
    unsigned bufIdx = 0;         // default
    bool directIndexing = false; // default

    bufType = DecodeAS4GFXResource(as, directIndexing, bufIdx);
    if (!directIndexing || (bufType != RESOURCE && bufType != UAV))
      return false;

    texId = bufIdx;
    lod = (unsigned)vlod->getZExtValue();
    isUAV = (bufType == UAV);
    return true;
  }
  return false;
}

bool GenUpdateCB::isConstantBufferLoad(LoadInst *inst, unsigned &bufId) {
  if (!inst)
    return false;

  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  ModuleMetaData *modMD = m_ctx->getModuleMetaData();
  unsigned as = inst->getPointerAddressSpace();
  bool directBuf;
  BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
  if (bufType == CONSTANT_BUFFER && directBuf && bufId < 15 && bufId != modMD->pushInfo.inlineConstantBufferSlot) {
    if (IntToPtrInst *itop = dyn_cast<IntToPtrInst>(inst->getOperand(0))) {
      if (isa<Constant>(itop->getOperand(0))) {
        return true;
      }
    } else if (ConstantExpr *cExpr = dyn_cast<ConstantExpr>(inst->getOperand(0))) {
      if (cExpr->getOpcode() == Instruction::IntToPtr && isa<Constant>(cExpr->getOperand(0))) {
        return true;
      }
    } else if ([[maybe_unused]] ConstantPointerNull *constNullptr =
                   dyn_cast<ConstantPointerNull>(inst->getOperand(0))) {
      return true;
    }
  }
  return false;
}

bool GenUpdateCB::allSrcConstantOrImm(Instruction *inst) {
  uint i = 0;
  for (i = 0; i < inst->getNumOperands(); i++) {
    if (dyn_cast<Constant>(inst->getOperand(i))) {
      continue;
    }

    LoadInst *loadInst = llvm::dyn_cast<llvm::LoadInst>(inst->getOperand(i));
    unsigned bufId = 0;

    if (loadInst && isConstantBufferLoad(loadInst, bufId)) {
      continue;
    }

    break;
  }
  return (i == inst->getNumOperands());
}

bool GenUpdateCB::updateCbAllowedInst(Instruction *inst) {
  if (!inst)
    return false;

  switch (inst->getOpcode()) {
  case Instruction::Add:
  case Instruction::FAdd:
  case Instruction::Sub:
  case Instruction::FSub:
  case Instruction::Mul:
  case Instruction::FMul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    return true;
  default:;
  }

  if (CallInst *callI = dyn_cast<CallInst>(inst)) {
    ConstantFP *C0 = dyn_cast<ConstantFP>(inst->getOperand(0));
    switch (GetOpCode(callI)) {
    case llvm_log:
    case llvm_sqrt:
      if (C0 && C0->getValueAPF().convertToFloat() > 0) {
        return true;
      }
      break;
    case llvm_pow:
    case llvm_cos:
    case llvm_sin:
    case llvm_exp:
    case llvm_floor:
    case llvm_ceil:
    case llvm_fabs:
    case llvm_max:
    case llvm_min:
    case llvm_rsq:
    case llvm_fsat:
      return true;
    default:
      return false;
    }
  }
  return false;
}

void GenUpdateCB::InsertInstTree(Instruction *inst, Instruction *pos) {
  if (!inst || dyn_cast<Constant>(inst) || vmap[inst]) {
    return;
  }

  unsigned bufId = 0, texId = 0, lod = 0;
  bool isUAV = false;
  if (isConstantBufferLoad(dyn_cast<LoadInst>(inst), bufId)) {
    Instruction *Clone = inst->clone();
    vmap[inst] = Clone;
    Clone->insertBefore(pos);
    m_ConstantBufferUsageMask |= (1 << bufId);
    return;
  } else if (isResInfo(dyn_cast<GenIntrinsicInst>(inst), texId, lod, isUAV)) {
    CallInst *cloneInst = cast<CallInst>(inst->clone());
    vmap[inst] = cloneInst;
    cloneInst->insertBefore(pos);

    llvm::Function *pfunc = nullptr;
    pfunc = GenISAIntrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns, GenISAIntrinsic::GenISA_resinfoptr,
                                            inst->getOperand(0)->getType());
    cloneInst->setCalledFunction(pfunc);
    return;
  }

  for (uint i = 0; i < inst->getNumOperands(); i++) {
    InsertInstTree(dyn_cast<Instruction>(inst->getOperand(i)), pos);
  }

  Instruction *Clone = inst->clone();
  vmap[inst] = Clone;

  for (uint i = 0; i < Clone->getNumOperands(); i++) {
    if (vmap[inst->getOperand(i)]) {
      Clone->setOperand(i, vmap[inst->getOperand(i)]);
    }
  }
  Clone->insertBefore(pos);

  // update declare
  llvm::Function *pfunc = nullptr;
  if (CallInst *callI = dyn_cast<CallInst>(Clone)) {
    switch (GetOpCode(callI)) {
    case llvm_log:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::log2,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_sqrt:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::sqrt,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_pow:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::pow,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_cos:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::cos,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_sin:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::sin,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_exp:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::exp2,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;

    case llvm_floor:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::floor,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_ceil:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::ceil,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_fabs:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::fabs,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_max:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::maxnum,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_min:
      pfunc = llvm::Intrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, Intrinsic::minnum,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_rsq:
      pfunc = llvm::GenISAIntrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, GenISAIntrinsic::GenISA_rsq,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    case llvm_fsat:
      pfunc = llvm::GenISAIntrinsic::getDeclaration(
          m_ConstantBufferReplaceShaderPatterns, GenISAIntrinsic::GenISA_fsat,
          llvm::ArrayRef<llvm::Type *>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Intrinsic not supported");
      break;
    }
    callI->setCalledFunction(pfunc);
  }
}

Instruction *GenUpdateCB::CreateModule(Module *M) {
  llvm::IRBuilder<> cb_builder(M->getContext());
  m_ConstantBufferReplaceShaderPatterns = new Module("CB", M->getContext());
  m_ConstantBufferReplaceShaderPatterns->setDataLayout(M->getDataLayout());
  Function *entry = Function::Create(FunctionType::get(cb_builder.getVoidTy(), false), GlobalValue::ExternalLinkage,
                                     "CBEntry", m_ConstantBufferReplaceShaderPatterns);
  BasicBlock *pEntryBlock = BasicBlock::Create(M->getContext(), "entry", entry);
  cb_builder.SetInsertPoint(pEntryBlock);

  Instruction *ret = cb_builder.CreateRetVoid();

  return ret;
}

bool GenUpdateCB::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  if (!m_ctx->m_DriverInfo.AllowGenUpdateCB(m_ctx->type) || IGC_IS_FLAG_DISABLED(EnableGenUpdateCB)) {
    return false;
  }

  m_CbUpdateMap.clear();
  bool foundCases = false;
  bool changed = false;

  // travel through instructions and mark the ones that are calculated with CB or imm
  DominatorTree &dom_tree = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  bool hasChange = true;
  while (hasChange) {
    hasChange = false;
    for (df_iterator<DomTreeNode *> DI = df_begin(dom_tree.getRootNode()), dom_end = df_end(dom_tree.getRootNode());
         DI != dom_end; ++DI) {
      BasicBlock *BB = DI->getBlock();
      for (auto BI = BB->begin(), BE = BB->end(); BI != BE;) {
        Instruction *inst = llvm::dyn_cast<Instruction>(&(*BI++));
        unsigned bufId = 0;
        unsigned texId = 0, lod = 0;
        bool isUAV = false;

        if (m_CbUpdateMap.count(inst) != 0) {
          continue;
        } else if (isConstantBufferLoad(dyn_cast<LoadInst>(inst), bufId)) {
          m_CbUpdateMap[inst] = FLAG_LOAD;
        } else if (IGC_IS_FLAG_ENABLED(EnableGenUpdateCBResInfo) &&
                   isResInfo(dyn_cast<GenIntrinsicInst>(inst, GenISAIntrinsic::GenISA_resinfoptr), texId, lod, isUAV)) {
          unsigned nelems = (unsigned)cast<IGCLLVM::FixedVectorType>(inst->getType())->getNumElements();
          SmallVector<SmallVector<ExtractElementInst *, 1>, 4> extracts(nelems);
          if (VectorUsedByConstExtractOnly(inst, extracts)) {
            m_CbUpdateMap[inst] = FLAG_RESINFO;
            for (unsigned i = 0; i < nelems; i++) {
              for (auto II : extracts[i]) {
                m_CbUpdateMap[II] = FLAG_RESINFO;
              }
            }
          }
        } else if (updateCbAllowedInst(inst)) {
          bool foundNewInst = 0;
          unsigned flag = 0;
          for (uint i = 0; i < inst->getNumOperands(); i++) {
            if (dyn_cast<Constant>(inst->getOperand(i))) {
              ;
            } else if (m_CbUpdateMap.count(inst->getOperand(i)) == 0) {
              foundNewInst = 0;
              break;
            } else {
              flag |= m_CbUpdateMap[inst->getOperand(i)];
              foundNewInst = 1;
            }
          }

          if (foundNewInst && m_CbUpdateMap.count(inst) == 0) {
            hasChange = true;
            foundCases = true;
            m_CbUpdateMap[inst] = flag;
          }
        }
      }
    }
  }

  // check whether given instruction are used outside of m_CbUpdateMap
  auto instUsed = [&](Instruction *inst) {
    for (auto nextInst = inst->user_begin(); nextInst != inst->user_end(); nextInst++) {
      if (m_CbUpdateMap.count(*nextInst) == 0) {
        return true;
      }
    }
    return false;
  };

  // look for cases to create mini-shader

  // Retreive next avilable GRF offset for constant payload
  // Convert byte offset to DWORD offset
  uint counter = m_ctx->m_constantPayloadNextAvailableGRFOffset >> 2;
  if (foundCases) {
    Instruction *ret = nullptr;
    llvm::IRBuilder<> orig_builder(F.getContext());

    for (auto iter = m_CbUpdateMap.begin(); iter != m_CbUpdateMap.end(); iter++) {
      if (counter >= m_maxCBcases) {
        break;
      }

      Instruction *inst = llvm::dyn_cast<Instruction>(iter->first);
      unsigned flag = iter->second;
      unsigned bufId = 0;
      unsigned texId = 0, lod = 0;
      bool isUAV = false;
      bool lastInstUsed = false;

      if (isConstantBufferLoad(dyn_cast<LoadInst>(inst), bufId) ||
          isResInfo(dyn_cast<GenIntrinsicInst>(inst), texId, lod, isUAV)) {
        // skip root values
        continue;
      }

      if (flag == FLAG_LOAD && !allSrcConstantOrImm(inst) && inst->getType()->getScalarSizeInBits() == 32 &&
          inst->getType()->isFloatTy()) // last check needs to be removed to enable integer cases
      {
        // for const buffer load, !allSrcConstantOrImm() check skips
        // the simple cases so it doesn't get triggered too many times.
        lastInstUsed = instUsed(inst);
      } else if ((flag & FLAG_RESINFO) != 0) {
        // always pickup resinfo results
        lastInstUsed = instUsed(inst);
      }

      if (lastInstUsed) {
        if (!m_ConstantBufferReplaceShaderPatterns) {
          ret = CreateModule(F.getParent());
        }
        llvm::IRBuilder<> cb_builder(m_ConstantBufferReplaceShaderPatterns->getContext());

        cb_builder.SetInsertPoint(ret);
        // add inst and its sources to the CB mini shader
        AllocaInst *storeAlloca = cb_builder.CreateAlloca(inst->getType());
        StoreInst *store = cb_builder.CreateStore(inst, storeAlloca);
        InsertInstTree(inst, store);
        StripDebugInfo(*m_ConstantBufferReplaceShaderPatterns);
        store->setOperand(0, vmap[inst]);

        // replace original shader with read from runtime
        llvm::Function *runtimeFunc =
            llvm::GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_RuntimeValue);
        Instruction *pValue = orig_builder.CreateCall(runtimeFunc, orig_builder.getInt32(counter));
        pValue->insertBefore(inst);

        if (inst->getType()->isIntegerTy()) {
          pValue = llvm::cast<llvm::Instruction>(orig_builder.CreateBitCast(pValue, orig_builder.getInt32Ty()));
          pValue->insertBefore(inst);
        }

        inst->replaceAllUsesWith(pValue);
        counter++;
        changed = true;
      }
    }

    if (m_ConstantBufferReplaceShaderPatterns) {
      // write the minishader Module to memory
      llvm::SmallVector<char, 4> bitcodeSV;
      llvm::raw_svector_ostream bitcodeSS(bitcodeSV);
      llvm::WriteBitcodeToFile(*m_ConstantBufferReplaceShaderPatterns, bitcodeSS);

      if (IGC_IS_FLAG_ENABLED(DumpLLVMIR)) {
        IGC::Debug::DumpName name = IGC::Debug::GetLLDumpName(m_ctx, "gencb");
        IGC::Debug::DumpLLVMIRText(m_ConstantBufferReplaceShaderPatterns, name);
      }

      size_t bufferSize = bitcodeSS.str().size();
      void *CBPatterns = aligned_malloc(bufferSize, 16);

      iSTD::MemCopy(CBPatterns, const_cast<char *>(bitcodeSS.str().data()), bufferSize);

      // return
      m_ctx->m_ConstantBufferReplaceShaderPatterns = CBPatterns;
      m_ctx->m_ConstantBufferReplaceShaderPatternsSize = bufferSize;
      m_ctx->m_ConstantBufferUsageMask = m_ConstantBufferUsageMask;
      m_ctx->m_ConstantBufferReplaceSize = iSTD::Align(counter, 8) / 8;

      // Update derived constants offset
      m_ctx->m_constantPayloadOffsets.DerivedConstantsOffset = m_ctx->m_constantPayloadNextAvailableGRFOffset;
      // Update next available GRF offset
      // conevrt DWORD offset to byte offset
      m_ctx->m_constantPayloadNextAvailableGRFOffset = counter << 2;
    }
  }
  return changed;
}

namespace IGC {
union {
  float f;
  int i;
  uint u;
} ftod0, ftod1, ftodTemp;

uint lookupValue(Value *op, DenseMap<Value *, uint> &CalculatedValue) {
  if (ConstantFP *c = dyn_cast<ConstantFP>(op)) {
    float floatValue = c->getValueAPF().convertToFloat();
    ftodTemp.f = floatValue;
    return ftodTemp.u;
  } else if (ConstantInt *c = dyn_cast<ConstantInt>(op)) {
    ftodTemp.i = (int)(c->getSExtValue());
    return ftodTemp.u;
  } else {
    IGC_ASSERT_MESSAGE(CalculatedValue.find(op) != CalculatedValue.end(), "can't find matching cb value");

    return CalculatedValue[op];
  }
  return 0;
}

static inline float denormToZeroF(float f) {
  if (std::fpclassify(f) == FP_SUBNORMAL) {
    return f < 0.0f ? -0.0f : 0.0f;
  }
  return f;
}

static inline float utof(uint32_t bits) {
  union {
    float f;
    uint32_t u;
  } un;
  un.u = bits;
  return un.f;
}

static inline uint32_t ftou_ftz(float f) {
  union {
    float f;
    uint32_t u;
  } un;
  un.f = denormToZeroF(f);
  return un.u;
}

void FoldDerivedConstant(char *bitcode, uint bitcodeSize, void *CBptr[15],
                         std::function<void(uint[4], uint, uint, bool)> getResInfoCB, uint *pNewCB) {
  // load module from memory
  llvm::Module *M = NULL;

  llvm::StringRef bitRef(bitcode, bitcodeSize);
  std::unique_ptr<llvm::MemoryBuffer> bitcodeMem =
      llvm::MemoryBuffer::getMemBuffer(bitRef, "", /* Null Term  = */ false);

  bool isBitCode = llvm::isBitcode((const unsigned char *)bitcodeMem->getBufferStart(),
                                   (const unsigned char *)bitcodeMem->getBufferEnd());

  LLVMContextWrapper context;
  if (isBitCode) {
    llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
        llvm::parseBitcodeFile(bitcodeMem->getMemBufferRef(), context);

    if (llvm::Error EC = ModuleOrErr.takeError()) {
      IGC_ASSERT_MESSAGE(0, "parsing bitcode failed");
    } else {
      M = ModuleOrErr.get().release();
    }
  } else {
    IGC_ASSERT_MESSAGE(0, "parsing bitcode failed");
  }

  // start constant folding
  struct ResInfoResult {
    unsigned info[4];
    ResInfoResult() { info[0] = info[1] = info[2] = info[3] = 0; }
    ResInfoResult(unsigned res[4]) { memcpy_s(info, sizeof(info), res, sizeof(info)); }
  };
  DenseMap<Value *, uint> CalculatedValue;
  DenseMap<Value *, ResInfoResult> resInfo;

  int newCBIndex = 0;
  BasicBlock *BB = &M->getFunctionList().begin()->getEntryBlock();
  for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
    Instruction *inst = &(*II);
    if (dyn_cast<AllocaInst>(inst) || dyn_cast<ReturnInst>(inst)) {
      continue;
    } else if (LoadInst *ld = dyn_cast<LoadInst>(inst)) {
      bool directBuf;
      unsigned bufId;
      IGC::DecodeAS4GFXResource(ld->getPointerAddressSpace(), directBuf, bufId);
      int offset = IGC::getConstantBufferLoadOffset(ld);
      uint32_t bits = *(uint32_t *)((char *)CBptr[bufId] + offset);

      CalculatedValue[ld] = ld->getType()->isFloatTy() ? ftou_ftz(utof(bits)) : bits;
    } else if (GenIntrinsicInst *intrin = dyn_cast<GenIntrinsicInst>(inst, GenISAIntrinsic::GenISA_resinfoptr)) {
      unsigned texId, lod;
      bool isUAV;
      unsigned res[4];
      isResInfo(intrin, texId, lod, isUAV);
      getResInfoCB(res, texId, lod, isUAV);
      ResInfoResult r(res);
      resInfo[intrin] = r;
    } else if (ExtractElementInst *extract = dyn_cast<ExtractElementInst>(inst)) {
      unsigned idx = (unsigned)cast<ConstantInt>(extract->getIndexOperand())->getZExtValue();
      unsigned val = resInfo[extract->getVectorOperand()].info[idx];
      CalculatedValue[extract] = val;
    } else if (dyn_cast<StoreInst>(inst)) {
      pNewCB[newCBIndex] = (uint)(CalculatedValue[inst->getOperand(0)]);
      newCBIndex++;
    } else {
      ftod0.u = lookupValue(inst->getOperand(0), CalculatedValue);

      if (CallInst *callI = dyn_cast<CallInst>(inst)) {
        switch (GetOpCode(callI)) {
        case llvm_cos:
          ftodTemp.f = cosf(ftod0.f);
          break;
        case llvm_sin:
          ftodTemp.f = sinf(ftod0.f);
          break;
        case llvm_log:
          ftodTemp.f = log10f(ftod0.f) / log10f(2.0f);
          break;
        case llvm_exp:
          ftodTemp.f = powf(2.0f, ftod0.f);
          break;
        case llvm_sqrt:
          ftodTemp.f = sqrtf(ftod0.f);
          break;
        case llvm_floor:
          ftodTemp.f = floorf(ftod0.f);
          break;
        case llvm_ceil:
          ftodTemp.f = ceilf(ftod0.f);
          break;
        case llvm_fabs:
          ftodTemp.f = fabs(ftod0.f);
          break;
        case llvm_pow:
          ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);
          ftodTemp.f = powf(ftod0.f, ftod1.f);
          break;
        case llvm_max:
          ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);
          // cannot use std::max since:
          //   std::max(Nan, x) = Nan
          //   std::max(x, NaN) = x
          //   fmax(Nan, x) = x
          //   fmax(x, Nan) = x
          ftodTemp.f = fmaxf(ftod0.f, ftod1.f);
          break;
        case llvm_min:
          ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);
          ftodTemp.f = fminf(ftod0.f, ftod1.f);
          break;
        case llvm_rsq:
          ftodTemp.f = 1.0f / sqrt(ftod0.f);
          break;
        case llvm_fsat:
          ftodTemp.f = fminf(1.0f, fmaxf(0.0f, ftod0.f));
          break;
        default:
          IGC_ASSERT(0);
          break;
        }
      } else {
        ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);

        switch (inst->getOpcode()) {
        case Instruction::Add:
          ftodTemp.i = ftod0.i + ftod1.i;
          break;
        case Instruction::FAdd:
          ftodTemp.f = ftod0.f + ftod1.f;
          break;
        case Instruction::Sub:
          ftodTemp.i = ftod0.i + ftod1.i;
          break;
        case Instruction::FSub:
          if (ftod0.f == 0.0f && isa<ConstantFP>(inst->getOperand(0))) {
            ftodTemp.f = -ftod1.f;
          } else {
            ftodTemp.f = ftod0.f - ftod1.f;
          }
          break;
        case Instruction::Mul:
          ftodTemp.i = ftod0.i * ftod1.i;
          break;
        case Instruction::FMul:
          ftodTemp.f = ftod0.f * ftod1.f;
          break;
        case Instruction::UDiv:
          ftodTemp.u = ftod0.u / ftod1.u;
          break;
        case Instruction::SDiv:
          ftodTemp.i = ftod0.i / ftod1.i;
          break;
        case Instruction::FDiv:
          ftodTemp.f = ftod0.f / ftod1.f;
          break;
        case Instruction::URem:
          ftodTemp.u = ftod0.u % ftod1.u;
          break;
        case Instruction::SRem:
          ftodTemp.i = ftod0.i % ftod1.i;
          break;
        case Instruction::Shl:
          ftodTemp.i = ftod0.i << ftod1.i;
          break;
        case Instruction::LShr:
          ftodTemp.u = ftod0.u >> ftod1.u;
          break;
        case Instruction::AShr:
          ftodTemp.i = (ftod0.i < 0) ? (-1 * (abs(ftod0.i) >> ftod1.i)) : ((ftod0.i) >> ftod1.i);
          break;
        case Instruction::And:
          ftodTemp.u = ftod0.u & ftod1.u;
          break;
        case Instruction::Or:
          ftodTemp.u = ftod0.u | ftod1.u;
          break;
        case Instruction::Xor:
          ftodTemp.u = ftod0.u ^ ftod1.u;
          break;
        default:
          IGC_ASSERT(0);
          break;
        }
      }

      if (inst->getType()->isFloatTy()) {
        CalculatedValue[inst] = ftou_ftz(ftodTemp.f);
      }
    }
  }
}
} // namespace IGC

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Attributes.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/Constants.h"
#include "ShaderTypesEnum.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-opencl-printf-resolution"
#define PASS_DESCRIPTION "Resolves OpenCL printf calls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(OpenCLPrintfResolutionLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(OpenCLPrintfResolutionLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char OpenCLPrintfResolutionLPM::ID = 0;

//
// FORMAT OF PRINTF OUTPUT BUFFER:
// ================================
/*
  ======================================================================
  | DWORD  bufferSize             Size of the buffer in bytes          |  <-- This value is incremented by atomic_add
  |====================================================================|
  | DWORD  stringIndex_ch_0       Index of format string for channel 0 |  \
  |--------------------------------------------------------------------|  |
  | DWORD  data0Type              Type identifier                      |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data0                  Data for channel 0                   |  |
  |--------------------------------------------------------------------|  | Channel 0 data
  | . . .  . . .                                                       |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data1Type              Type identifier                      |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data1                  Data for channel 0                   |  /
  |====================================================================|
  | DWORD  stringIndex_ch_1       Index of format string for channel 1 |  \
  |--------------------------------------------------------------------|  |
  | DWORD  data0Type              Type identifier                      |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data0                  Data for channel 1                   |  |
  |--------------------------------------------------------------------|  |  Channel 1 data
  | . . .  . . .                                                       |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data0Type              Type identifier                      |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data1                  Data for channel 1                   |  /
  |====================================================================|
  | . . .  . . .                                                       |
  | . . .  . . .                                                       |
  | . . .  . . .                                                       |
  |====================================================================|
  | DWORD  stringIndex_ch_N       Index of format string for channel N |  \
  |--------------------------------------------------------------------|  |
  | DWORD  data0Type              Type identifier                      |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data0                  Data for channel N                   |  |
  |--------------------------------------------------------------------|  |  Channel N data
  | . . .  . . .                                                       |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data0Type              Type identifier                      |  |
  |--------------------------------------------------------------------|  |
  | DWORD  data1                  Data for channel N                   |  /
  |--------------------------------------------------------------------|
*/

// For vector arguments, 2 type identifiers are used: 1st is IGC::SHADER_PRINTF_VECTOR_*  and 2nd is the vector length.
// These 2 type identifiers are followed by the elements of the vector.
// Example: float4
//
// |------------------------------|
// |  IGC::SHADER_PRINTF_VECTOR_FLOAT  |
// |------------------------------|
// |             0x4              |
// |------------------------------|
// |      < vec_element_0 >       |
// |------------------------------|
// |      < vec_element_1 >       |
// |------------------------------|
// |      < vec_element_2 >       |
// |------------------------------|
// |      < vec_element_3 >       |
// |------------------------------|

OpenCLPrintfResolutionLPM::OpenCLPrintfResolutionLPM() : FunctionPass(ID) {
  initializeOpenCLPrintfResolutionLPMPass(*PassRegistry::getPassRegistry());
}

bool IGC::OpenCLPrintfResolution::doInitialization(Module &M) {
  m_module = (IGCLLVM::Module *)&M;
  m_context = &M.getContext();
  m_CGContext = nullptr;
  m_stringIndex = 0;
  m_ptrSizeIntType = M.getDataLayout().getIntPtrType(*m_context, ADDRESS_SPACE_GLOBAL);
  m_int32Type = Type::getInt32Ty(*m_context);

  // The legacy llvm::FunctionPass::doInitialization() base is a no-op returning false.
  return false;
}

bool OpenCLPrintfResolution::runOnFunction(Function &F, IGC::CodeGenContext *pCtx,
                                           IGC::IGCMD::MetaDataUtils *pMdUtils) {
  m_CGContext = pCtx;
  m_fp64Supported = !m_CGContext->platform.hasNoFP64Inst();
  m_pMdUtils = pMdUtils;

  // Gather all found printf calls into the m_printfCalls vector.
  visit(F);

  bool changed = !m_printfCalls.empty();

  // Put strings found in the printf calls into metadata.
  // Replace the printf calls with sequences of instructions that
  // writes data into printf output buffer.
  for (CallInst *printfCall : m_printfCalls) {
    m_DL = printfCall->getDebugLoc();
    expandPrintfCall(*printfCall, F);
  }

  m_printfCalls.clear();

  return changed;
}

void OpenCLPrintfResolution::visitCallInst(CallInst &callInst) {
  if (!callInst.getCalledFunction()) {
    return;
  }

  StringRef funcName = callInst.getCalledFunction()->getName();
  if (funcName == OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME ||
      funcName == OpenCLPrintfAnalysis::BUILTIN_PRINTF_FUNCTION_NAME) {
    m_printfCalls.push_back(&callInst);
  }
}

std::string OpenCLPrintfResolution::getEscapedString(const ConstantDataSequential *pCDS) {
  std::string Name;
  // This is to avoid unnecessary characters that exceed the char range
  for (unsigned i = 0, len = pCDS->getNumElements() - 1; i != len; i++) {
    if (isa<ConstantInt>(pCDS->getElementAsConstant(i))) {
      if ((cast<ConstantInt>(pCDS->getElementAsConstant(i))->getZExtValue()) > 127) {
        Name = "";
        return Name;
      }
      unsigned char C = (char)cast<ConstantInt>(pCDS->getElementAsConstant(i))->getZExtValue();

      if (isprint(C) && (C != '\\') && (C != '"')) {
        Name.push_back(C);
      } else {
        Name.push_back('\\');
        switch (C) {
        case '\a':
          Name.push_back('a');
          break;
        case '\b':
          Name.push_back('b');
          break;
        case '\f':
          Name.push_back('f');
          break;
        case '\n':
          Name.push_back('n');
          break;
        case '\r':
          Name.push_back('r');
          break;
        case '\t':
          Name.push_back('t');
          break;
        case '\v':
          Name.push_back('v');
          break;
        default:
          Name.push_back(C);
          break;
        }
      }
    } else {
      Name = "";
      return Name;
    }
  }
  return Name;
}

Value *OpenCLPrintfResolution::processPrintfString(Value *arg, Function &F) {
  GlobalVariable *formatString = nullptr;

  if (isa<GlobalVariable>(arg)) {
    formatString = dyn_cast_or_null<GlobalVariable>(arg);
    if ((nullptr == formatString) || !formatString->hasInitializer()) {
      IGC_ASSERT_MESSAGE(0, "Unexpected printf argument (expected string literal)");
      return ConstantInt::get(m_int32Type, -1);
    }

    if (m_CGContext->type == ShaderType::OPENCL_SHADER) {
      return arg;
    }

    ConstantDataArray *formatStringConst = dyn_cast<ConstantDataArray>(formatString->getInitializer());
    std::string escaped_string = formatStringConst ? getEscapedString(formatStringConst) : "";

    // preventing MD enries duplication
    if (m_MapStringStringIndex.find(escaped_string) != m_MapStringStringIndex.end()) {
      return ConstantInt::get(m_int32Type, m_MapStringStringIndex[escaped_string]);
    }


    // Add new metadata node and put the printf string into it.
    // The first element of metadata node is the string index,
    // the second element is the string itself.
    NamedMDNode *namedMDNode = m_module->getOrInsertNamedMetadata(getPrintfStringsMDNodeName(F));
    SmallVector<Metadata *, 2> args;
    Metadata *stringIndexVal = ConstantAsMetadata::get(ConstantInt::get(m_int32Type, m_stringIndex));

    MDString *final_string = MDString::get(*m_context, escaped_string);

    args.push_back(stringIndexVal);
    args.push_back(final_string);

    MDNode *itemMDNode = MDNode::get(*m_context, args);
    namedMDNode->addOperand(itemMDNode);

    m_MapStringStringIndex[escaped_string] = m_stringIndex;

    return ConstantInt::get(m_int32Type, m_stringIndex++);
  } else if (CastInst *castInst = dyn_cast<CastInst>(arg)) {
    return processPrintfString(castInst->getOperand(0), F);
  } else if (GetElementPtrInst *getElemPtrInst = dyn_cast<GetElementPtrInst>(arg)) {
    IGC_ASSERT_MESSAGE(getElemPtrInst->hasAllZeroIndices(), "Only All Zero indices GEP supported");
    return processPrintfString(getElemPtrInst->getPointerOperand(), F);
  } else if (SelectInst *selectInst = dyn_cast<SelectInst>(arg)) {
    SelectInst *selectInst2 =
        SelectInst::Create(selectInst->getOperand(0), processPrintfString(selectInst->getOperand(1), F),
                           processPrintfString(selectInst->getOperand(2), F), "", IGCLLVM::insertPosition(selectInst));
    // TODO: Clean up the original select within the current pass
    return selectInst2;
  } else if (PHINode *phiNode = dyn_cast<PHINode>(arg)) {
    unsigned inNum = phiNode->getNumIncomingValues();
    PHINode *newPhi = nullptr;
    for (unsigned i = 0; i < inNum; ++i) {
      Value *newIV = processPrintfString(phiNode->getIncomingValue(i), F);
      if (!newPhi)
        newPhi = PHINode::Create(newIV->getType(), inNum, "", IGCLLVM::insertPosition(phiNode));
      newPhi->addIncoming(newIV, phiNode->getIncomingBlock(i));
    }
    // TODO: Clean up the original PHI node within the current pass
    return newPhi;
  } else {
    IGC_ASSERT_MESSAGE(0, "Unsupported Instruction!");
  }

  return ConstantInt::get(m_int32Type, -1);
}

// Checks pathes to global variables and returns true if all paths lead to constant strings.
// Only these instructions acepted in pathes:
// * a CastInst
// * a GEP with all-zero indices
// * a SelectInst
// * a PHINode
// It is expected that the paths are not looped.
bool OpenCLPrintfResolution::argIsString(Value *arg) {
  if (isa<GlobalVariable>(arg)) {
    GlobalVariable *formatString = dyn_cast_or_null<GlobalVariable>(arg);
    if (nullptr == formatString || !formatString->hasInitializer()) {
      return false;
    }
    Constant *initializer = formatString->getInitializer();
    if (IGCLLVM::Constant::isNullValue(initializer) && initializer->getType()->isArrayTy()) {
      // Is zeroinitializer; can't be casted to ConstantDataArray.
      // This caused zeroinitializers to be treated as non-strings,
      // which caused assertion errors.
      return true;
    }
    ConstantDataArray *formatStringConst = dyn_cast<ConstantDataArray>(initializer);
    if (!formatStringConst || !formatStringConst->isCString()) {
      return false;
    }
    return true;
  } else if (CastInst *castInst = dyn_cast<CastInst>(arg)) {
    return argIsString(castInst->getOperand(0));
  }
  if (GetElementPtrInst *getElemPtrInst = dyn_cast<GetElementPtrInst>(arg)) {
    return getElemPtrInst->hasAllZeroIndices() && argIsString(getElemPtrInst->getPointerOperand());
  } else if (SelectInst *selectInst = dyn_cast<SelectInst>(arg)) {
    return argIsString(selectInst->getOperand(1)) && argIsString(selectInst->getOperand(2));
  } else if (PHINode *phiNode = dyn_cast<PHINode>(arg)) {
    for (unsigned i = 0; i < phiNode->getNumIncomingValues(); i++) {
      if (!argIsString(phiNode->getIncomingValue(i)))
        return false;
    }
    return true;
  }
  return false;
}

std::string OpenCLPrintfResolution::getPrintfStringsMDNodeName(Function &F) { return "printf.strings"; }

static StoreInst *genStoreInternal(Value *Val, Value *Ptr, BasicBlock *InsertAtEnd, const DebugLoc &DL,
                                   bool isNontemporal) {
  bool isVolatile = false;
  unsigned Align = 4;
  auto SI = new llvm::StoreInst(Val, Ptr, isVolatile, IGCLLVM::getCorrectAlign(Align), InsertAtEnd);
  SI->setDebugLoc(DL);
  if (isNontemporal) {
    Constant *One = ConstantInt::get(Type::getInt32Ty(SI->getContext()), 1);
    MDNode *Node = MDNode::get(SI->getContext(), ConstantAsMetadata::get(One));
    SI->setMetadata(LLVMContext::MD_nontemporal, Node);
  }
  return SI;
}

Value *OpenCLPrintfResolution::emitInlineFormatString(Value *writeOffset, SPrintfArgDescriptor *argDesc,
                                                      Value *inlineFmtLenWithNull, Value *inlineFmtAlignedLen,
                                                      BasicBlock *bblockTrue, bool isPrintfBuiltin) {
  Type *i64Ty = Type::getInt64Ty(*m_context);
  Type *i8Ty = Type::getInt8Ty(*m_context);
  Value *constVal8 = ConstantInt::get(m_ptrSizeIntType, 8);

  // *write_offset = INLINE_STRING_FLAG | length; write_offset += 8
  Value *lenI64 = CastInst::Create(Instruction::CastOps::ZExt, inlineFmtLenWithNull, i64Ty, "", bblockTrue);
  Value *slotVal = BinaryOperator::CreateOr(lenI64, ConstantInt::get(i64Ty, SHADER_PRINTF_INLINE_STRING_FLAG),
                                            "fmt_inline_slot", bblockTrue);
  Instruction *writeOffsetPtr =
      CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset,
                       IGCLLVM::PointerType::get(i64Ty, ADDRESS_SPACE_GLOBAL), "write_offset_ptr", bblockTrue);
  writeOffsetPtr->setDebugLoc(m_DL);
  genStoreInternal(slotVal, writeOffsetPtr, bblockTrue, m_DL, isPrintfBuiltin);
  writeOffset = BinaryOperator::CreateAdd(writeOffset, constVal8, "write_offset", bblockTrue);
  cast<Instruction>(writeOffset)->setDebugLoc(m_DL);

  // memcpy(write_offset, formatString, byteLength); write_offset += alignedLen
  unsigned srcAS = cast<PointerType>(argDesc->value->getType())->getAddressSpace();
  Instruction *dstPtr =
      CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset,
                       IGCLLVM::PointerType::get(i8Ty, ADDRESS_SPACE_GLOBAL), "write_offset_ptr", bblockTrue);
  dstPtr->setDebugLoc(m_DL);
  Value *srcPtr = argDesc->value;
  Type *srcI8PtrTy = IGCLLVM::PointerType::get(i8Ty, srcAS);
  if (srcPtr->getType() != srcI8PtrTy)
    srcPtr = CastInst::CreatePointerCast(srcPtr, srcI8PtrTy, "", bblockTrue);
  IRBuilder<> memcpyBuilder(bblockTrue);
  memcpyBuilder.SetCurrentDebugLocation(m_DL);
  memcpyBuilder.CreateMemCpy(dstPtr, IGCLLVM::getCorrectAlign(1), srcPtr, IGCLLVM::getCorrectAlign(1),
                             inlineFmtLenWithNull);

  Value *alignedLenOffset = inlineFmtAlignedLen;
  if (m_ptrSizeIntType != m_int32Type)
    alignedLenOffset =
        CastInst::Create(Instruction::CastOps::ZExt, inlineFmtAlignedLen, m_ptrSizeIntType, "", bblockTrue);
  writeOffset = BinaryOperator::CreateAdd(writeOffset, alignedLenOffset, "write_offset", bblockTrue);
  cast<Instruction>(writeOffset)->setDebugLoc(m_DL);

  return writeOffset;
}

void OpenCLPrintfResolution::removeExcessArgs() {
  SPrintfArgDescriptor *formatStringArgDesc = &m_argDescriptors[0];

  Value *formatString = formatStringArgDesc->value;
  [[maybe_unused]] IGC::SHADER_PRINTF_TYPE dataType = formatStringArgDesc->argType;
  IGC_ASSERT(dataType == SHADER_PRINTF_STRING_LITERAL);

  // Inline format strings have unknown content here (including globals without a
  // constant-string initializer, e.g. SLM), so we cannot count specifiers.
  if (formatStringArgDesc->inlineFmtString)
    return;

  if (auto GV = dyn_cast<GlobalVariable>(formatString)) {
    IGC_ASSERT(GV->hasInitializer());

    unsigned int numFormatSpecifiers = 0;

    Constant *initializer = GV->getInitializer();
    if (IGCLLVM::Constant::isNullValue(initializer) && initializer->getType()->isArrayTy())
      // The string literal is empty, can't be casted to ConstantDataArray
      // and there is no way it contains any format specifiers.
      numFormatSpecifiers = 0;
    else if (auto *formatStringConst = dyn_cast<ConstantDataArray>(initializer))
      numFormatSpecifiers = getNumFormatSpecifiers(formatStringConst);
    else
      // Not a constant string we can parse; leave the arg list untouched.
      return;

    if (m_argDescriptors.size() > numFormatSpecifiers + 1)
      m_argDescriptors.erase(m_argDescriptors.begin() + numFormatSpecifiers + 1, m_argDescriptors.end());
  }
}

void OpenCLPrintfResolution::expandPrintfCall(CallInst &printfCall, Function &F) {
  /* Replace a printf call with IR instructions that fill the printf
     output buffer created by the Runtime:
     --------------------------------------------------------------------------
           bufferPtr      - pointer to the printf output buffer. This pointer
                            is an implicit kernel argument. It is loaded into
                            GRF as part of thread payload.
           bufferSize     - size of the printf output buffer. By agreement with
                            Runtime, it is 4 Mb.
           dataSize       - size of printf data for current thread.

     Note: we use STATELESS mode for printf buffer access.
    ---------------------------------------------------------------------------
        writeOffset = atomic_add(bufferPtr, dataSize);
        writePtr = bufferPtr + writeOffset;
        endOffset = writeOffset + dataSize;
        if (endOffset < bufferSize) {                \
            // Write the format string index         |
            *writePtr = stringIndex;                 |
            writePtr += 4;                           |
                                                     |
            // Write the argument type               |
            *writePtr = argument[1].dataType;        |
            writePtr += 4;                           |
            // Write the argument value              |
            *writePtr = argument[1].value;           |
            writePtr += 4;                           | bblockTrue
            . . .                                    |
            . . .                                    |
            // Write the argument type               |
            *writePtr = argument[N].dataType;        |
            writePtr += 4;                           |
            // Write the argument value              |
            *writePtr = argument[N].value;           |
            writePtr += 4;                           |
                                                     |
            // printf returns 0 if successful        |
            return_val = 0;                          /
        }
        else {                                                           \
            // Check if the remaining output                             |
            // buffer space is enough for writing                        |
            //invalid string index.                                      |
            endOffset = writeOffset + 4;                                 |
            if (endOffset < bufferSize) {           \                    | bblockFalse
                // Write the invalid string index.  | bblockErrorString  |
                *writePtr = -1;                     |                    |
            }                                       /                    |
            // printf returns -1 if failed                               |
            return_val = -1;                                             /
       }

       We also support printf to any provided buffer.
       This is done with special builtin with following signature:
       int __builtin_IB_printf_to_buffer(global char* buf, global char* currentOffset, int bufSize, ...);
         buf - pointer to the begging of the buffer.
         currentOffset - pointer to the location with the current offset that will be atomically incremented.
           In the case of regular printf this offset is on the first DWORD of printfBuffer.
           E.g. in assert buffer it is on the third DWORD.
          bufSize - total size of the buffer.
       Note: in the case of builtin printf, all the stores will be nontemporal.


     ----------------------------------------------------------------------
  */
  MetaDataUtils *MdUtils = m_pMdUtils;
  ImplicitArgs implicitArgs(F, MdUtils, m_CGContext->getModuleMetaData());
  bool isPrintfBuiltin = OpenCLPrintfAnalysis::isBuiltinPrintf(printfCall.getCalledFunction());

  // Put all printf argument into m_argDescriptors vector.
  // Scalarize vector arguments and substitute string arguments by their indices.
  preprocessPrintfArgs(printfCall);

  removeExcessArgs();

  // An inline format string needs a runtime length to size the record and the
  // copy. emitStrlenWithNull() splits the block, so capture 'currentBBlock' after.
  SPrintfArgDescriptor *fmtDesc = &m_argDescriptors[0];
  bool inlineFmtString = fmtDesc->inlineFmtString;
  Value *inlineFmtLenWithNull = nullptr;
  Value *inlineFmtAlignedLen = nullptr;

  Value *dataSizeVal = ConstantInt::get(m_int32Type, getTotalDataSize());
  if (inlineFmtString) {
    inlineFmtLenWithNull = emitStrlenWithNull(fmtDesc->value, &printfCall);
    // alignedLen = (lenWithNull + 3) & ~3
    Value *lenPlus3 = BinaryOperator::CreateAdd(inlineFmtLenWithNull, ConstantInt::get(m_int32Type, 3), "",
                                                IGCLLVM::insertPosition(&printfCall));
    inlineFmtAlignedLen = BinaryOperator::CreateAnd(lenPlus3, ConstantInt::get(m_int32Type, ~3u), "fmt_aligned_len",
                                                    IGCLLVM::insertPosition(&printfCall));
    // The flag|length slot is the 8 bytes already counted by getTotalDataSize();
    // only the padded string bytes are extra.
    dataSizeVal =
        BinaryOperator::CreateAdd(dataSizeVal, inlineFmtAlignedLen, "data_size", IGCLLVM::insertPosition(&printfCall));
  }

  BasicBlock *currentBBlock = printfCall.getParent();

  // writeOffset = atomic_add(bufferPtr, dataSize)
  Value *basebufferPtr = isPrintfBuiltin ? printfCall.getArgOperand(0)
                                         : implicitArgs.getImplicitArgValue(F, ImplicitArg::PRINTF_BUFFER, MdUtils);

  Value *currentOffsetPtr = isPrintfBuiltin ? printfCall.getArgOperand(1) : basebufferPtr;
  Instruction *writeOffsetStart = genAtomicAdd(currentOffsetPtr, dataSizeVal, printfCall, "write_offset");
  writeOffsetStart->setDebugLoc(m_DL);

  Instruction *writeOffset = writeOffsetStart;
  Instruction *writeOffsetPtr = nullptr;

  // end_offset = write_offset + data_size
  Instruction *endOffset =
      BinaryOperator::CreateAdd(writeOffset, dataSizeVal, "end_offset", IGCLLVM::insertPosition(&printfCall));
  endOffset->setDebugLoc(m_DL);

  Value *bufferMaxSize = isPrintfBuiltin
                             ? printfCall.getArgOperand(2)
                             : ConstantInt::get(m_int32Type, m_CGContext->m_DriverInfo.getPrintfBufferSize());

  // write_ptr = buffer_ptr + write_offset;
  if (m_ptrSizeIntType != writeOffset->getType()) {
    writeOffset = CastInst::Create(Instruction::CastOps::ZExt, writeOffset, m_ptrSizeIntType, "write_offset",
                                   IGCLLVM::insertPosition(&printfCall));
    writeOffset->setDebugLoc(m_DL);
  }
  Instruction *bufferPtr = CastInst::Create(Instruction::CastOps::PtrToInt, basebufferPtr, m_ptrSizeIntType,
                                            "buffer_ptr", IGCLLVM::insertPosition(&printfCall));
  bufferPtr->setDebugLoc(m_DL);
  Instruction *writeOffsetAdd =
      BinaryOperator::CreateAdd(bufferPtr, writeOffset, "write_offset", IGCLLVM::insertPosition(&printfCall));
  writeOffsetAdd->setDebugLoc(m_DL);
  writeOffset = writeOffsetAdd;

  // if (end_offset < output_buffer_size))
  Instruction *cmp1 = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_ULE, endOffset, bufferMaxSize, "",
                                      IGCLLVM::insertPosition(&printfCall));
  cmp1->setDebugLoc(m_DL);

  // Since we need to insert a branch here, the current basic block should be
  // splitted into two parts.
  BasicBlock *bblockJoin = currentBBlock->splitBasicBlock(BasicBlock::iterator(printfCall), "bblockJoin");

  // Create "true" and "false" branches.
  BasicBlock *bblockTrue = BasicBlock::Create(*m_context, "write_offset_true", &F, bblockJoin);
  BasicBlock *bblockFalse = BasicBlock::Create(*m_context, "write_offset_false", &F, bblockJoin);

  currentBBlock->getTerminator()->eraseFromParent();
  BranchInst *brInst = BranchInst::Create(bblockTrue, bblockFalse, cmp1, currentBBlock);
  brInst->setDebugLoc(m_DL);

  //  ----------- Fill "true" block ----------------

  // write_offset += 4;
  Value *constVal4 = ConstantInt::get(m_ptrSizeIntType, 4);

  for (size_t i = 0, size = m_argDescriptors.size(); i < size; ++i) {
    SPrintfArgDescriptor *argDesc = &m_argDescriptors[i];
    Value *printfArg = argDesc->value;
    IGC::SHADER_PRINTF_TYPE dataType = argDesc->argType;

    // Inline format string: [marker:u64][byteLength:u32][bytes padded to 4].
    if (i == 0 && argDesc->inlineFmtString) {
      writeOffset = cast<Instruction>(emitInlineFormatString(writeOffset, argDesc, inlineFmtLenWithNull,
                                                             inlineFmtAlignedLen, bblockTrue, isPrintfBuiltin));
      continue;
    }

    // We don't store the dataType for format string (which is the first entry in m_argDescriptors).
    if (i != 0) {
      // *write_offset = argument[i].dataType
      Value *argTypeVal = ConstantInt::get(m_int32Type, (unsigned int)dataType);
      writeOffsetPtr = CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset,
                                        IGCLLVM::PointerType::get(m_int32Type, ADDRESS_SPACE_GLOBAL),
                                        "write_offset_ptr", bblockTrue);
      writeOffsetPtr->setDebugLoc(m_DL);
      genStoreInternal(argTypeVal, writeOffsetPtr, bblockTrue, m_DL, isPrintfBuiltin);

      // write_offset += 4
      writeOffset = BinaryOperator::CreateAdd(writeOffset, constVal4, "write_offset", bblockTrue);
      writeOffset->setDebugLoc(m_DL);

      // For vector arguments, add vector size after type ID.
      if (argDesc->vecSize > 0) {
        Value *vecSizeVal = ConstantInt::get(m_int32Type, argDesc->vecSize);
        writeOffsetPtr = CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset,
                                          IGCLLVM::PointerType::get(m_int32Type, ADDRESS_SPACE_GLOBAL),
                                          "write_offset_ptr", bblockTrue);
        writeOffsetPtr->setDebugLoc(m_DL);
        genStoreInternal(vecSizeVal, writeOffsetPtr, bblockTrue, m_DL, isPrintfBuiltin);

        // write_offset += 4
        writeOffset = BinaryOperator::CreateAdd(writeOffset, constVal4, "write_offset", bblockTrue);
        writeOffset->setDebugLoc(m_DL);
      }
    }

    writeOffsetPtr = generateCastToPtr(argDesc, writeOffset, bblockTrue);
    writeOffsetPtr->setDebugLoc(m_DL);

    if (dataType == SHADER_PRINTF_STRING_LITERAL && m_CGContext->type == ShaderType::OPENCL_SHADER) {
      printfArg = CastInst::Create(Instruction::CastOps::PtrToInt, argDesc->value, Type::getInt64Ty(*m_context), "",
                                   bblockTrue);
    }

    // *write_offset = argument[i].value
    genStoreInternal(printfArg, writeOffsetPtr, bblockTrue, m_DL, isPrintfBuiltin);

    // write_offset += argument[i].size
    Value *offsetInc = ConstantInt::get(m_ptrSizeIntType, getArgTypeSize(dataType, argDesc->vecSize));
    writeOffset = BinaryOperator::CreateAdd(writeOffset, offsetInc, "write_offset", bblockTrue);
    writeOffset->setDebugLoc(m_DL);
  } // for (SPrintfArgDescriptor *argDesc : m_argDescriptors)

  brInst = BranchInst::Create(bblockJoin, bblockTrue);
  brInst->setDebugLoc(m_DL);

  //  ----------- Fill "false" block ----------------
  // end_offset = write_offset + 4
  Value *constVal4_32 = ConstantInt::get(m_int32Type, 4);
  endOffset = BinaryOperator::CreateAdd(writeOffsetStart, constVal4_32, "end_offset", bblockFalse);
  endOffset->setDebugLoc(m_DL);
  // if (end_offset < output_buffer_size)
  Instruction *cmp2 = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_ULE, endOffset, bufferMaxSize, "", bblockFalse);
  cmp2->setDebugLoc(m_DL);
  // Here, we generate code that checks if the error string index can be
  // written into the output buffer.
  BasicBlock *bblockErrorString = BasicBlock::Create(*m_context, "write_error_string", &F, bblockJoin);
  BasicBlock *bblockFalseJoin = BasicBlock::Create(*m_context, "bblockFalseJoin", &F, bblockJoin);

  brInst = BranchInst::Create(bblockErrorString, bblockFalseJoin, cmp2, bblockFalse);
  brInst->setDebugLoc(m_DL);

  // *writeOffset = -1;
  Value *constValErrStringIdx = ConstantInt::get(m_int32Type, -1);
  writeOffsetPtr = CastInst::Create(Instruction::CastOps::IntToPtr, writeOffsetAdd,
                                    IGCLLVM::PointerType::get(m_int32Type, ADDRESS_SPACE_GLOBAL), "write_offset_ptr",
                                    bblockErrorString);
  writeOffsetPtr->setDebugLoc(m_DL);
  genStoreInternal(constValErrStringIdx, writeOffsetPtr, bblockErrorString, m_DL, isPrintfBuiltin);
  brInst = BranchInst::Create(bblockFalseJoin, bblockErrorString);
  brInst->setDebugLoc(m_DL);

  // bblockFalseJoin is an empty basic block,
  // it is needed to assure bblockJoin have only 2 predecessors.
  brInst = BranchInst::Create(bblockJoin, bblockFalseJoin);
  brInst->setDebugLoc(m_DL);

  // return_val = select cmp1, 0, -1
  Value *constVal0 = ConstantInt::get(m_int32Type, 0);
  Value *constValm1 = ConstantInt::get(m_int32Type, -1);
  Instruction *returnVal =
      SelectInst::Create(cmp1, constVal0, constValm1, "printf_ret_val", IGCLLVM::insertPosition(&printfCall));
  returnVal->setDebugLoc(m_DL);

  printfCall.replaceAllUsesWith(returnVal);
  printfCall.eraseFromParent();
  m_argDescriptors.clear();
}

Value *OpenCLPrintfResolution::fixupPrintfArg(CallInst &printfCall, Value *arg, IGC::SHADER_PRINTF_TYPE &argDataType) {
  // For string argument, add the string to the metadata and put the string index
  // into the vector of arguments.
  switch (argDataType) {
  case IGC::SHADER_PRINTF_STRING_LITERAL: {
    Function *F = printfCall.getParent()->getParent();
    return processPrintfString(arg, *F);
  } break;
  case IGC::SHADER_PRINTF_POINTER: {
    Instruction *tmp = CastInst::Create(Instruction::CastOps::PtrToInt, arg, m_ptrSizeIntType, "",
                                        IGCLLVM::insertPosition(&printfCall));
    tmp->setDebugLoc(m_DL);
    return tmp;
  } break;
  case IGC::SHADER_PRINTF_FLOAT:
  case IGC::SHADER_PRINTF_VECTOR_FLOAT:
  case IGC::SHADER_PRINTF_DOUBLE:
  case IGC::SHADER_PRINTF_VECTOR_DOUBLE:
    // Cast halfs back to float. Cast doubles to floats if the platform does not support double fp type.
    if (arg->getType()->getScalarType()->isHalfTy() ||
        (!m_fp64Supported && arg->getType()->getScalarType()->isDoubleTy())) {
      if (argDataType == IGC::SHADER_PRINTF_DOUBLE)
        argDataType = IGC::SHADER_PRINTF_FLOAT;
      if (argDataType == IGC::SHADER_PRINTF_VECTOR_DOUBLE)
        argDataType = IGC::SHADER_PRINTF_VECTOR_FLOAT;

      if (ConstantFP *constVal = dyn_cast<ConstantFP>(arg)) {
        // If this is a constant, just replace it.
        bool ignored;
        APFloat FV = constVal->getValueAPF();
        FV.convert(APFloat::IEEEsingle(), APFloat::rmNearestTiesToEven, &ignored);
        return ConstantFP::get(arg->getContext(), FV);
      } else if (CastInst *fpCastVal = dyn_cast<CastInst>(arg)) {
        // If this is a fpcast, use the origin value.
        Type *srcType = fpCastVal->getSrcTy();
        if (srcType->isFloatTy() ||
            (srcType->isVectorTy() && cast<VectorType>(srcType)->getElementType()->isFloatTy())) {
          return fpCastVal->getOperand(0);
        }
      }

      Type *newType = Type::getFloatTy(arg->getContext());
      if (auto argVT = dyn_cast<IGCLLVM::FixedVectorType>(arg->getType())) {
        newType = IGCLLVM::FixedVectorType::get(newType, (unsigned)argVT->getNumElements());
      }

      Instruction *tmp = CastInst::CreateFPCast(arg, newType, "to_float", IGCLLVM::insertPosition(&printfCall));
      tmp->setDebugLoc(m_DL);
      return tmp;
    }
    break;

  default:
    break;
  }

  return arg;
}

unsigned OpenCLPrintfResolution::getNumFormatSpecifiers(const ConstantDataArray *dataArray) {
  unsigned int count = 0;
  StringRef formatString = dataArray->getRawDataValues();

  const size_t length = formatString.size();
  for (size_t i = 0; i < length; i++) {
    if (formatString[i] == '%') {
      if (i + 1 < length && formatString[i + 1] == '%') {
        i++;
        continue;
      }

      count++;
    }
  }

  return count;
}

void OpenCLPrintfResolution::preprocessPrintfArgs(CallInst &printfCall) {
  int i = 0;
  if (OpenCLPrintfAnalysis::isBuiltinPrintf(printfCall.getCalledFunction())) {
    // printf builtin function has buffer pointer, current offset pointer and buffer size as first three arguments.
    // Skip them here, as we want to collect the arguments starting from format string.
    i = 3;
  }
  for (int numArgs = IGCLLVM::getNumArgOperands(&printfCall); i < numArgs; ++i) {
    Value *arg = printfCall.getOperand(i);
    Type *argType = arg->getType();
    // The first collected descriptor is always the format string.
    bool isFormatString = m_argDescriptors.empty();
    IGC::SHADER_PRINTF_TYPE argDataType = getPrintfArgDataType(arg);
    bool inlineFmtString = false;

    // SPV_EXT_relaxed_printf_string_address_space: a format string not resolving
    // to a compile-time global is emitted inline into the buffer. Keep the raw
    // pointer and skip fixupPrintfArg, which would assert on a non-global. OCL only.
    if (isFormatString && m_CGContext->type == ShaderType::OPENCL_SHADER &&
        argDataType != IGC::SHADER_PRINTF_STRING_LITERAL && argType->isPointerTy()) {
      argDataType = IGC::SHADER_PRINTF_STRING_LITERAL;
      inlineFmtString = true;
    } else {
      arg = fixupPrintfArg(printfCall, arg, argDataType);
    }
    uint vecSize = 0;
    if (auto argVType = dyn_cast<IGCLLVM::FixedVectorType>(argType)) {
      vecSize = (uint)argVType->getNumElements();
    }
    m_argDescriptors.push_back(SPrintfArgDescriptor(argDataType, arg, vecSize, inlineFmtString));
  }
}

CallInst *OpenCLPrintfResolution::genAtomicAdd(Value *outputBufferPtr, Value *dataSize, CallInst &printfCall,
                                               StringRef name) {
  // outputBufferPtr->getType() could be "i8 addrspace(1)*", and the atomic prototype
  // requires "i32 addrspace(1)*":
  //
  //   %writeOffset = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* <outputBufferPtr>,
  //                                                               i32 <dataSize>)
  //
  Type *bufPtrType = IGCLLVM::getInt32PtrTy(*m_context, ADDRESS_SPACE_GLOBAL);
  if (outputBufferPtr->getType() != bufPtrType) {
    outputBufferPtr = CastInst::Create(Instruction::CastOps::BitCast, outputBufferPtr, bufPtrType, "ptrBC",
                                       IGCLLVM::insertPosition(&printfCall));
  }

  if (m_atomicAddFunc == nullptr) {
    Type *argTypes[] = {outputBufferPtr->getType(), dataSize->getType()};
    FunctionType *atomicFuncType = FunctionType::get(dataSize->getType(), argTypes, false);
    m_atomicAddFunc = cast<Function>(
        m_module->getOrInsertFunction("__builtin_IB_atomic_add_global_i32", atomicFuncType, AttributeList()));
  }
  std::vector<Value *> args;
  args.push_back(outputBufferPtr);
  args.push_back(dataSize);

  return CallInst::Create(m_atomicAddFunc, args, name, IGCLLVM::insertPosition(&printfCall));
}

unsigned int OpenCLPrintfResolution::getArgTypeSize(IGC::SHADER_PRINTF_TYPE argType, uint vecSize) {
  switch (argType) {
  case IGC::SHADER_PRINTF_LONG:
  case IGC::SHADER_PRINTF_DOUBLE:
  case IGC::SHADER_PRINTF_POINTER: // Runtime expects 64 bit value for pointer regardless of its actual size.
    return 8;
  case IGC::SHADER_PRINTF_VECTOR_LONG:
  case IGC::SHADER_PRINTF_VECTOR_DOUBLE:
    return vecSize * 8;

  case IGC::SHADER_PRINTF_STRING_LITERAL: {
    // The size of the format string address
    return (m_CGContext->type == ShaderType::OPENCL_SHADER) ? 8 : 4;
  }

  default:
    if (vecSize > 0) {
      return vecSize * 4;
    } else {
      return 4;
    }
  }
}

unsigned int OpenCLPrintfResolution::getTotalDataSize() {
  IGC_ASSERT_MESSAGE(m_argDescriptors.size() > 0, "Empty printf arguments list.");
  unsigned int dataSize = 0;
  SPrintfArgDescriptor *argDesc = &m_argDescriptors[0];
  // Add the size that represents a format string.
  dataSize += getArgTypeSize(argDesc->argType, argDesc->vecSize);

  // Skip 0-th operand (format string) and count total size of
  // the remaining arguments.
  for (size_t i = 1, size = m_argDescriptors.size(); i < size; ++i) {
    argDesc = &m_argDescriptors[i];
    // Add size of the data type identifier.
    dataSize += 4;
    // Vector arguments require additional type identifier - number of elements.
    if (argDesc->vecSize > 0) {
      dataSize += 4;
    }
    // Add size of the data itself.
    dataSize += getArgTypeSize(argDesc->argType, argDesc->vecSize);
  }
  return dataSize;
}

IGC::SHADER_PRINTF_TYPE OpenCLPrintfResolution::getPrintfArgDataType(Value *printfArg) {
  Type *argType = printfArg->getType();

  if (auto argVType = dyn_cast<VectorType>(argType)) {
    Type *elemType = argVType->getElementType();
    if (elemType->isFloatingPointTy()) {
      if (elemType->isDoubleTy())
        return IGC::SHADER_PRINTF_VECTOR_DOUBLE;
      else
        return IGC::SHADER_PRINTF_VECTOR_FLOAT;
    } else if (elemType->isIntegerTy()) {
      unsigned int typeSize = elemType->getScalarSizeInBits();
      switch (typeSize) {
      case 8:
        return IGC::SHADER_PRINTF_VECTOR_BYTE;
      case 16:
        return IGC::SHADER_PRINTF_VECTOR_SHORT;
      case 32:
        return IGC::SHADER_PRINTF_VECTOR_INT;
      case 64:
        return IGC::SHADER_PRINTF_VECTOR_LONG;
      }
    }
  } else if (argType->isFloatingPointTy()) {
    if (argType->isDoubleTy())
      return IGC::SHADER_PRINTF_DOUBLE;
    else
      return IGC::SHADER_PRINTF_FLOAT;
  } else if (argType->isIntegerTy()) {
    unsigned int typeSize = argType->getScalarSizeInBits();
    switch (typeSize) {
    case 8:
      return IGC::SHADER_PRINTF_BYTE;
    case 16:
      return IGC::SHADER_PRINTF_SHORT;
    case 32:
      return IGC::SHADER_PRINTF_INT;
    case 64:
      return IGC::SHADER_PRINTF_LONG;
    }
  } else if (argIsString(printfArg)) {
    return IGC::SHADER_PRINTF_STRING_LITERAL;
  } else if (argType->isPointerTy()) {
    return IGC::SHADER_PRINTF_POINTER;
  }
  return IGC::SHADER_PRINTF_INVALID;
}

Instruction *OpenCLPrintfResolution::generateCastToPtr(SPrintfArgDescriptor *argDesc, Value *writeOffset,
                                                       BasicBlock *bblock) {
  Type *castedType = nullptr;

  switch (argDesc->argType) {
  case IGC::SHADER_PRINTF_BYTE:
  case IGC::SHADER_PRINTF_SHORT:
  case IGC::SHADER_PRINTF_INT:
  case IGC::SHADER_PRINTF_LONG:
  case IGC::SHADER_PRINTF_FLOAT:
  case IGC::SHADER_PRINTF_DOUBLE:
  case IGC::SHADER_PRINTF_VECTOR_BYTE:
  case IGC::SHADER_PRINTF_VECTOR_SHORT:
  case IGC::SHADER_PRINTF_VECTOR_INT:
  case IGC::SHADER_PRINTF_VECTOR_LONG:
  case IGC::SHADER_PRINTF_VECTOR_FLOAT:
  case IGC::SHADER_PRINTF_VECTOR_DOUBLE: {
    Type *origType = argDesc->value->getType();
    castedType = IGCLLVM::PointerType::get(origType, ADDRESS_SPACE_GLOBAL);
    break;
  }

  case IGC::SHADER_PRINTF_STRING_LITERAL: {
    if (m_CGContext->type == ShaderType::OPENCL_SHADER)
      castedType = IGCLLVM::getInt64PtrTy(*m_context, ADDRESS_SPACE_GLOBAL);
    else
      castedType = IGCLLVM::getInt32PtrTy(*m_context, ADDRESS_SPACE_GLOBAL);
    break;
  }
  case IGC::SHADER_PRINTF_POINTER:
    castedType = IGCLLVM::PointerType::get(m_ptrSizeIntType, ADDRESS_SPACE_GLOBAL);
    break;

  default:
    IGC_ASSERT_MESSAGE(0, "Unexpected printf argument type");
    break;
  }

  return CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset, castedType, "write_offset_ptr", bblock);
}

Value *OpenCLPrintfResolution::emitStrlenWithNull(Value *strPtr, Instruction *insertBefore) {
  // Scan bytes from strPtr until the null terminator and return the index past
  // it (length including null). doneBB is reached only from the loop, so the
  // returned value dominates it.
  Type *i8Ty = Type::getInt8Ty(*m_context);
  BasicBlock *entryBB = insertBefore->getParent();
  Function *F = entryBB->getParent();
  BasicBlock *doneBB = entryBB->splitBasicBlock(insertBefore->getIterator(), "strlen.done");
  BasicBlock *loopBB = BasicBlock::Create(*m_context, "strlen.loop", F, doneBB);

  // Normalize the format pointer to i8* in its own address space.
  unsigned strAS = cast<PointerType>(strPtr->getType())->getAddressSpace();
  Type *i8PtrTy = IGCLLVM::PointerType::get(i8Ty, strAS);
  if (strPtr->getType() != i8PtrTy)
    strPtr =
        CastInst::CreatePointerCast(strPtr, i8PtrTy, "fmt_i8ptr", IGCLLVM::insertPosition(entryBB->getTerminator()));

  // Redirect entry's fallthrough (created by splitBasicBlock) to the loop.
  entryBB->getTerminator()->eraseFromParent();
  BranchInst *toLoop = BranchInst::Create(loopBB, entryBB);
  toLoop->setDebugLoc(m_DL);

  IRBuilder<> B(loopBB);
  B.SetCurrentDebugLocation(m_DL);
  PHINode *idx = B.CreatePHI(m_int32Type, 2, "strlen_idx");
  idx->addIncoming(ConstantInt::get(m_int32Type, 0), entryBB);
  Value *gepIndices[] = {idx};
  Value *elemPtr = B.CreateGEP(i8Ty, strPtr, gepIndices, "strlen_ptr");
  Value *ch = B.CreateLoad(i8Ty, elemPtr, "strlen_ch");
  Value *idxNext = B.CreateAdd(idx, ConstantInt::get(m_int32Type, 1), "strlen_idx_next");
  idx->addIncoming(idxNext, loopBB);
  Value *isNull = B.CreateICmpEQ(ch, ConstantInt::get(i8Ty, 0), "strlen_is_null");
  B.CreateCondBr(isNull, doneBB, loopBB);

  return idxNext;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses OpenCLPrintfResolutionNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto &MDU = AM.getResult<MetaDataUtilsAnalysis>(M);
  CodeGenContext *pCtx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  OpenCLPrintfResolution impl;
  impl.doInitialization(M);
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, pCtx, MDU.MdUtils);
  }
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

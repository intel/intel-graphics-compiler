/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/RayTracing/RTLoggingManager.h"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Attributes.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Intrinsics.h"
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
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
IGC_INITIALIZE_PASS_BEGIN(OpenCLPrintfResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(OpenCLPrintfResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char OpenCLPrintfResolution::ID = 0;

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

// Looks for a GlobalVariable related with given value.
// Returns nullptr if on the way to the global variable
// found anything that is not :
// * a CastInst
// * a GEP with non-zero indices
// * a SelectInst
// * a PHINode
// In case of select or phi instruction two operands are added to the vector.
// In another case only one is added.
inline SmallVector<Value*, 2> getGlobalVariable(Value* const v)
{
    SmallVector<Value *, 2> curr;
    curr.push_back(v);

    while (nullptr != curr.front() || nullptr != curr.back())
    {
        if (curr.size() == 1 && isa<GlobalVariable>(curr.front()))
        {
            break;
        }
        else if (curr.size() == 2 && (isa<GlobalVariable>(curr.front()) && isa<GlobalVariable>(curr.back())))
        {
            break;
        }

        if (CastInst* castInst = dyn_cast<CastInst>(curr.front()))
        {
            curr.pop_back();
            curr.push_back(castInst->getOperand(0));
        }
        else if (GetElementPtrInst* getElemPtrInst = dyn_cast<GetElementPtrInst>(curr.front()))
        {
            if (curr.size() == 2)
            {
                if (GetElementPtrInst* getElemPtrInst2 = dyn_cast<GetElementPtrInst>(curr.back()))
                {
                    curr.pop_back();
                    curr.pop_back();
                    curr.push_back(getElemPtrInst->hasAllZeroIndices() ? getElemPtrInst->getPointerOperand() : nullptr);
                    curr.push_back(getElemPtrInst2->hasAllZeroIndices() ? getElemPtrInst2->getPointerOperand() : nullptr);
                }
            }
            else
            {
                curr.pop_back();
                curr.push_back(getElemPtrInst->hasAllZeroIndices() ? getElemPtrInst->getPointerOperand() : nullptr);
            }
        }
        else if (SelectInst* selectInst = dyn_cast<SelectInst>(curr.front()))
        {
            if (curr.size() == 2)
            {
                //  Nested select instruction is not supported
                curr.front() = nullptr;
                curr.back() = nullptr;
            }
            else
            {
                curr.pop_back();
                curr.push_back(selectInst->getOperand(1));
                curr.push_back(selectInst->getOperand(2));
            }
        }
        else if (PHINode* phiNode = dyn_cast<PHINode>(curr.front()))
        {
            if (curr.size() == 2)
            {
                //  Nested phi instruction is not supported
                curr.front() = nullptr;
                curr.back() = nullptr;
            }
            else
            {
                curr.pop_back();
                curr.push_back(phiNode->getOperand(0));
                curr.push_back(phiNode->getOperand(1));
            }
        }
        else
        {
            // Unhandled value type
            curr.front() = nullptr;
            if (curr.size() == 2)
            {
                curr.back() = nullptr;
            }
        }
    }
    return curr;
}

OpenCLPrintfResolution::OpenCLPrintfResolution() : FunctionPass(ID), m_atomicAddFunc(nullptr)
{
    initializeOpenCLPrintfResolutionPass(*PassRegistry::getPassRegistry());
}

bool IGC::OpenCLPrintfResolution::doInitialization(Module& M)
{
    m_module = (IGCLLVM::Module*) & M;
    m_context = &M.getContext();
    m_CGContext = nullptr;
    m_stringIndex = 0;
    m_ptrSizeIntType = M.getDataLayout().getIntPtrType(*m_context, ADDRESS_SPACE_GLOBAL);
    m_int32Type = Type::getInt32Ty(*m_context);

    return FunctionPass::doInitialization(M);
}

bool OpenCLPrintfResolution::runOnFunction(Function& F)
{
    if (m_CGContext == nullptr) {
        m_CGContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        m_fp64Supported = !m_CGContext->platform.hasNoFP64Inst();
    }

    // Gather all found printf calls into the m_printfCalls vector.
    visit(F);

    bool changed = !m_printfCalls.empty();

    // Put strings found in the printf calls into metadata.
    // Replace the printf calls with sequences of instructions that
    // writes data into printf output buffer.
    for (CallInst* printfCall : m_printfCalls)
    {
        m_DL = printfCall->getDebugLoc();
        expandPrintfCall(*printfCall, F);
    }

    m_printfCalls.clear();

    return changed;
}

void OpenCLPrintfResolution::visitCallInst(CallInst& callInst)
{
    if (!callInst.getCalledFunction())
    {
        return;
    }

    StringRef  funcName = callInst.getCalledFunction()->getName();
    if (funcName == OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME)
    {
        m_printfCalls.push_back(&callInst);
    }
}

std::string OpenCLPrintfResolution::getEscapedString(const ConstantDataSequential* pCDS)
{
    std::string Name;
    // This is to avoid unnecessary characters that exceed the char range
    for (unsigned i = 0, len = pCDS->getNumElements() - 1; i != len; i++)
    {
        if (isa<ConstantInt>(pCDS->getElementAsConstant(i)))
        {
            if ((cast<ConstantInt>(pCDS->getElementAsConstant(i))->getZExtValue()) > 127)
            {
                Name = "";
                return Name;
            }
            unsigned char C = (char)cast<ConstantInt>(pCDS->getElementAsConstant(i))->getZExtValue();

            if (isprint(C) &&
                (C != '\\') &&
                (C != '"'))
            {
                Name.push_back(C);
            }
            else
            {
                Name.push_back('\\');
                switch (C)
                {
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
        }
        else
        {
            Name = "";
            return Name;
        }
    }
    return Name;
}

Value* OpenCLPrintfResolution::processPrintfString(Value* printfArg, Function& F)
{
    GlobalVariable* formatString = nullptr;
    SmallVector<Value*, 2> curr = getGlobalVariable(printfArg);
    SmallVector<unsigned int, 2> sv;
    for (auto curr_i : curr)
    {
        auto& curr_e = *curr_i;

        formatString = dyn_cast_or_null<GlobalVariable>(&curr_e);
        ConstantDataArray* formatStringConst = ((nullptr != formatString) && (formatString->hasInitializer())) ?
            dyn_cast<ConstantDataArray>(formatString->getInitializer()) :
            nullptr;

        if (nullptr == formatStringConst)
        {
            IGC_ASSERT_MESSAGE(0, "Unexpected printf argument (expected string literal)");
            return 0;
        }

        if (m_CGContext->type == ShaderType::RAYTRACING_SHADER)
        {
            auto* Ctx = static_cast<RayDispatchShaderContext*>(m_CGContext);
            m_stringIndex = *Ctx->LogMgr.getIndex(formatStringConst->getAsCString());
        }

        // Add new metadata node and put the printf string into it.
        // The first element of metadata node is the string index,
        // the second element is the string itself.
        NamedMDNode* namedMDNode = m_module->getOrInsertNamedMetadata(getPrintfStringsMDNodeName(F));
        SmallVector<Metadata*, 2>  args;
        Metadata* stringIndexVal = ConstantAsMetadata::get(
            ConstantInt::get(m_int32Type, m_stringIndex));

        sv.push_back(m_stringIndex++);

        std::string escaped_string = getEscapedString(formatStringConst);
        MDString* final_string = MDString::get(*m_context, escaped_string);

        args.push_back(stringIndexVal);
        args.push_back(final_string);

        MDNode* itemMDNode = MDNode::get(*m_context, args);
        namedMDNode->addOperand(itemMDNode);
    }

    // Checks if the vector have two elements.
    // If it has it adds a new phi/select instruction that is responsible
    // for the correct execution of the basic instruction.
    // This information is forwarded to the store instruction.
    if (curr.size() == 2)
    {
        if (GetElementPtrInst* getElemPtrInst = dyn_cast<GetElementPtrInst>(printfArg))
        {
            if (PHINode* phiNode = dyn_cast<PHINode>(getElemPtrInst->getPointerOperand()))
            {
                PHINode* phiNode2 = PHINode::Create(m_int32Type, 2, "", phiNode);
                phiNode2->addIncoming(ConstantInt::get(m_int32Type, sv.front()), phiNode->getIncomingBlock(0));
                phiNode2->addIncoming(ConstantInt::get(m_int32Type, sv.back()), phiNode->getIncomingBlock(1));
                return phiNode2;
            }
        }
        else if (SelectInst* selectInst = dyn_cast<SelectInst>(printfArg))
        {
            SelectInst* selectInst2 = SelectInst::Create(selectInst->getOperand(0), ConstantInt::get(m_int32Type, sv.front()),
                ConstantInt::get(m_int32Type, sv.back()), "", selectInst);
            return selectInst2;
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Instructions in the vector are not supported!");
        }
    }
    ModuleMetaData* modMd = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    if (IGC_IS_FLAG_ENABLED(EnableZEBinary) || modMd->compOpt.EnableZEBinary)
    {
        return printfArg;
    }
    else
    {
        return ConstantInt::get(m_int32Type, m_stringIndex - 1);
    }

}


bool OpenCLPrintfResolution::argIsString(Value* printfArg)
{
    GlobalVariable* formatString = nullptr;
    SmallVector<Value*, 2> curr = getGlobalVariable(printfArg);

    for (auto curr_i : curr)
    {
        auto& curr_e = *curr_i;
        formatString = dyn_cast_or_null<GlobalVariable>(&curr_e);
        if (nullptr == formatString || !formatString->hasInitializer())
        {
            return false;
        }
        ConstantDataArray* formatStringConst = dyn_cast<ConstantDataArray>(formatString->getInitializer());
        if (!formatStringConst || !formatStringConst->isCString())
        {
            return false;
        }
    }
    return true;
}

std::string OpenCLPrintfResolution::getPrintfStringsMDNodeName(Function& F)
{
    return "printf.strings";
}

static StoreInst* genStoreInternal(Value* Val, Value* Ptr, BasicBlock* InsertAtEnd, DebugLoc DL)
{
    bool isVolatile = false;
    unsigned Align = 4;
    auto SI = new llvm::StoreInst(Val, Ptr, isVolatile, IGCLLVM::getCorrectAlign(Align), InsertAtEnd);
    SI->setDebugLoc(DL);
    return SI;
}

void OpenCLPrintfResolution::expandPrintfCall(CallInst& printfCall, Function& F)
{
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
       ----------------------------------------------------------------------
    */
    MetaDataUtils* MdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ImplicitArgs implicitArgs(F, MdUtils);

    BasicBlock* currentBBlock = printfCall.getParent();

    // Put all printf argument into m_argDescriptors vector.
    // Scalarize vector arguments and substitute string arguments by their indices.
    preprocessPrintfArgs(printfCall);

    // writeOffset = atomic_add(bufferPtr, dataSize)
    Value* basebufferPtr = implicitArgs.getImplicitArgValue(F, ImplicitArg::PRINTF_BUFFER, MdUtils);

    Value* dataSizeVal = ConstantInt::get(m_int32Type, getTotalDataSize());
    Instruction* writeOffsetStart = genAtomicAdd(basebufferPtr, dataSizeVal, printfCall, "write_offset");
    writeOffsetStart->setDebugLoc(m_DL);

    Instruction* writeOffset = writeOffsetStart;
    Instruction* writeOffsetPtr = nullptr;

    // end_offset = write_offset + data_size
    Instruction* endOffset = BinaryOperator::CreateAdd(writeOffset, dataSizeVal, "end_offset", &printfCall);
    endOffset->setDebugLoc(m_DL);

    Value* bufferMaxSize = ConstantInt::get(m_int32Type, m_CGContext->m_DriverInfo.getPrintfBufferSize());

    // write_ptr = buffer_ptr + write_offset;
    if (m_ptrSizeIntType != writeOffset->getType())
    {
        writeOffset = CastInst::Create(Instruction::CastOps::ZExt,
            writeOffset,
            m_ptrSizeIntType,
            "write_offset",
            &printfCall);
        writeOffset->setDebugLoc(m_DL);
    }
    Instruction* bufferPtr = CastInst::Create(Instruction::CastOps::PtrToInt,
        basebufferPtr,
        m_ptrSizeIntType,
        "buffer_ptr",
        &printfCall);
    bufferPtr->setDebugLoc(m_DL);
    Instruction* writeOffsetAdd = BinaryOperator::CreateAdd(bufferPtr,
        writeOffset,
        "write_offset",
        &printfCall);
    writeOffsetAdd->setDebugLoc(m_DL);
    writeOffset = writeOffsetAdd;

    // if (end_offset < output_buffer_size))
    Instruction* cmp1 = CmpInst::Create(Instruction::ICmp,
        CmpInst::ICMP_ULE,
        endOffset,
        bufferMaxSize,
        "",
        &printfCall);
    cmp1->setDebugLoc(m_DL);

    // Since we need to insert a branch here, the current basic block should be
    // splitted into two parts.
    BasicBlock* bblockJoin = currentBBlock->splitBasicBlock(BasicBlock::iterator(printfCall), "bblockJoin");

    // Create "true" and "false" branches.
    BasicBlock* bblockTrue = BasicBlock::Create(*m_context, "write_offset_true", &F, bblockJoin);
    BasicBlock* bblockFalse = BasicBlock::Create(*m_context, "write_offset_false", &F, bblockJoin);

    currentBBlock->getTerminator()->eraseFromParent();
    BranchInst* brInst = BranchInst::Create(bblockTrue, bblockFalse, cmp1, currentBBlock);
    brInst->setDebugLoc(m_DL);

    //  ----------- Fill "true" block ----------------

    // write_offset += 4;
    Value* constVal4 = ConstantInt::get(m_ptrSizeIntType, 4);

    for (size_t i = 0, size = m_argDescriptors.size(); i < size; ++i)
    {
        SPrintfArgDescriptor* argDesc = &m_argDescriptors[i];
        Value* printfArg = argDesc->value;
        IGC::SHADER_PRINTF_TYPE dataType = argDesc->argType;

        // We don't store the dataType for format string (which is the first entry in m_argDescriptors).
        if (i != 0)
        {
            // *write_offset = argument[i].dataType
            Value* argTypeVal = ConstantInt::get(m_int32Type, (unsigned int)dataType);
            writeOffsetPtr = CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset,
                m_int32Type->getPointerTo(ADDRESS_SPACE_GLOBAL), "write_offset_ptr", bblockTrue);
            writeOffsetPtr->setDebugLoc(m_DL);
            genStoreInternal(argTypeVal, writeOffsetPtr, bblockTrue, m_DL);

            // write_offset += 4
            writeOffset = BinaryOperator::CreateAdd(writeOffset, constVal4, "write_offset", bblockTrue);
            writeOffset->setDebugLoc(m_DL);

            // For vector arguments, add vector size after type ID.
            if (argDesc->vecSize > 0) {
                Value* vecSizeVal = ConstantInt::get(m_int32Type, argDesc->vecSize);
                writeOffsetPtr = CastInst::Create(Instruction::CastOps::IntToPtr, writeOffset,
                    m_int32Type->getPointerTo(ADDRESS_SPACE_GLOBAL), "write_offset_ptr", bblockTrue);
                writeOffsetPtr->setDebugLoc(m_DL);
                genStoreInternal(vecSizeVal, writeOffsetPtr, bblockTrue, m_DL);

                // write_offset += 4
                writeOffset = BinaryOperator::CreateAdd(writeOffset, constVal4, "write_offset", bblockTrue);
                writeOffset->setDebugLoc(m_DL);
            }
        }

        writeOffsetPtr = generateCastToPtr(argDesc, writeOffset, bblockTrue);
        writeOffsetPtr->setDebugLoc(m_DL);

        ModuleMetaData* modMd = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        if (dataType == SHADER_PRINTF_STRING_LITERAL && (
            IGC_IS_FLAG_ENABLED(EnableZEBinary) || modMd->compOpt.EnableZEBinary))
        {
            printfArg = CastInst::Create(Instruction::CastOps::PtrToInt,
                argDesc->value,
                m_ptrSizeIntType,
                "",
                bblockTrue);
        }

        // *write_offset = argument[i].value
        genStoreInternal(printfArg, writeOffsetPtr, bblockTrue, m_DL);

        // write_offset += argument[i].size
        Value* offsetInc = ConstantInt::get(m_ptrSizeIntType, getArgTypeSize(dataType, argDesc->vecSize));
        writeOffset = BinaryOperator::CreateAdd(writeOffset, offsetInc, "write_offset", bblockTrue);
        writeOffset->setDebugLoc(m_DL);
    } // for (SPrintfArgDescriptor *argDesc : m_argDescriptors)

    brInst = BranchInst::Create(bblockJoin, bblockTrue);
    brInst->setDebugLoc(m_DL);

    //  ----------- Fill "false" block ----------------
    // end_offset = write_offset + 4
    Value* constVal4_32 = ConstantInt::get(m_int32Type, 4);
    endOffset = BinaryOperator::CreateAdd(writeOffsetStart, constVal4_32, "end_offset", bblockFalse);
    endOffset->setDebugLoc(m_DL);
    // if (end_offset < output_buffer_size)
    Instruction* cmp2 = CmpInst::Create(Instruction::ICmp,
        CmpInst::ICMP_ULE,
        endOffset,
        bufferMaxSize,
        "",
        bblockFalse);
    cmp2->setDebugLoc(m_DL);
    // Here, we generate code that checks if the error string index can be
    // written into the output buffer.
    BasicBlock* bblockErrorString = BasicBlock::Create(*m_context, "write_error_string", &F, bblockJoin);
    BasicBlock* bblockFalseJoin = BasicBlock::Create(*m_context, "bblockFalseJoin", &F, bblockJoin);

    brInst = BranchInst::Create(bblockErrorString, bblockFalseJoin, cmp2, bblockFalse);
    brInst->setDebugLoc(m_DL);

    // *writeOffset = -1;
    Value* constValErrStringIdx = ConstantInt::get(m_int32Type, -1);
    writeOffsetPtr = CastInst::Create(Instruction::CastOps::IntToPtr,
        writeOffsetAdd,
        m_int32Type->getPointerTo(ADDRESS_SPACE_GLOBAL),
        "write_offset_ptr",
        bblockErrorString);
    writeOffsetPtr->setDebugLoc(m_DL);
    genStoreInternal(constValErrStringIdx, writeOffsetPtr, bblockErrorString, m_DL);
    brInst = BranchInst::Create(bblockFalseJoin, bblockErrorString);
    brInst->setDebugLoc(m_DL);

    // bblockFalseJoin is an empty basic block,
    // it is needed to assure bblockJoin have only 2 predecessors.
    brInst = BranchInst::Create(bblockJoin, bblockFalseJoin);
    brInst->setDebugLoc(m_DL);

    // return_val = select cmp1, 0, -1
    Value* constVal0 = ConstantInt::get(m_int32Type, 0);
    Value* constValm1 = ConstantInt::get(m_int32Type, -1);
    Instruction* returnVal = SelectInst::Create(cmp1, constVal0, constValm1, "printf_ret_val", &printfCall);
    returnVal->setDebugLoc(m_DL);

    printfCall.replaceAllUsesWith(returnVal);
    printfCall.eraseFromParent();
    m_argDescriptors.clear();
}

Value* OpenCLPrintfResolution::fixupPrintfArg(CallInst& printfCall, Value* arg, IGC::SHADER_PRINTF_TYPE& argDataType)
{
    // For string argument, add the string to the metadata and put the string index
    // into the vector of arguments.
    switch (argDataType)
    {
    case IGC::SHADER_PRINTF_STRING_LITERAL:
    {
        Function* F = printfCall.getParent()->getParent();
        return processPrintfString(arg, *F);
    }
    break;
    case IGC::SHADER_PRINTF_POINTER:
    {
        Instruction* tmp = CastInst::Create(Instruction::CastOps::PtrToInt,
            arg,
            m_ptrSizeIntType,
            "",
            &printfCall);
        tmp->setDebugLoc(m_DL);
        return tmp;
    }
    break;
    case IGC::SHADER_PRINTF_FLOAT:
    case IGC::SHADER_PRINTF_VECTOR_FLOAT:
    case IGC::SHADER_PRINTF_DOUBLE:
    case IGC::SHADER_PRINTF_VECTOR_DOUBLE:
        // Cast halfs back to float. Cast doubles to floats if the platform does not support double fp type.
        if (arg->getType()->getScalarType()->isHalfTy() || (!m_fp64Supported && arg->getType()->getScalarType()->isDoubleTy()))
        {
            if (argDataType == IGC::SHADER_PRINTF_DOUBLE)
                argDataType = IGC::SHADER_PRINTF_FLOAT;
            if (argDataType == IGC::SHADER_PRINTF_VECTOR_DOUBLE)
                argDataType = IGC::SHADER_PRINTF_VECTOR_FLOAT;

            if (ConstantFP * constVal = dyn_cast<ConstantFP>(arg))
            {
                // If this is a constant, just replace it.
                bool ignored;
                APFloat FV = constVal->getValueAPF();
                FV.convert(APFloat::IEEEsingle(), APFloat::rmNearestTiesToEven, &ignored);
                return ConstantFP::get(arg->getContext(), FV);
            }
            else if (CastInst * fpCastVal = dyn_cast<CastInst>(arg))
            {
                // If this is a fpcast, use the origin value.
                Type* srcType = fpCastVal->getSrcTy();
                if (srcType->isFloatTy() ||
                    (srcType->isVectorTy() && cast<VectorType>(srcType)->getElementType()->isFloatTy()))
                {
                    return fpCastVal->getOperand(0);
                }
            }

            Type* newType = Type::getFloatTy(arg->getContext());
            if (auto argVT = dyn_cast<IGCLLVM::FixedVectorType>(arg->getType()))
            {
                newType = IGCLLVM::FixedVectorType::get(newType, (unsigned)argVT->getNumElements());
            }

            Instruction* tmp = CastInst::CreateFPCast(arg,
                newType,
                "to_float",
                &printfCall);
            tmp->setDebugLoc(m_DL);
            return tmp;
        }
        break;

    default:
        break;
    }

    return arg;
}

void OpenCLPrintfResolution::preprocessPrintfArgs(CallInst& printfCall)
{
    for (int i = 0, numArgs = IGCLLVM::getNumArgOperands(&printfCall); i < numArgs; ++i)
    {
        Value* arg = printfCall.getOperand(i);
        Type* argType = arg->getType();
        IGC::SHADER_PRINTF_TYPE argDataType = getPrintfArgDataType(arg);
        arg = fixupPrintfArg(printfCall, arg, argDataType);
        uint vecSize = 0;
        if (auto argVType = dyn_cast<IGCLLVM::FixedVectorType>(argType)) {
            vecSize = (uint)argVType->getNumElements();
        }
        m_argDescriptors.push_back(SPrintfArgDescriptor(argDataType, arg, vecSize));
    }
}

CallInst* OpenCLPrintfResolution::genAtomicAdd(Value* outputBufferPtr,
    Value* dataSize,
    CallInst& printfCall,
    StringRef name)
{
    // outputBufferPtr->getType() could be "i8 addrspace(1)*", and the atomic prototype
    // requires "i32 addrspace(1)*":
    //
    //   %writeOffset = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* <outputBufferPtr>,
    //                                                               i32 <dataSize>)
    //
    Type* bufPtrType = Type::getInt32PtrTy(*m_context, ADDRESS_SPACE_GLOBAL);
    if (outputBufferPtr->getType() != bufPtrType) {
        outputBufferPtr = CastInst::Create(Instruction::CastOps::BitCast,
            outputBufferPtr,
            bufPtrType,
            "ptrBC",
            &printfCall);
    }

    if (m_atomicAddFunc == nullptr) {
        Type* argTypes[] = { outputBufferPtr->getType(), dataSize->getType() };
        FunctionType* atomicFuncType = FunctionType::get(dataSize->getType(), argTypes, false);
        m_atomicAddFunc = cast<Function>(m_module->getOrInsertFunction("__builtin_IB_atomic_add_global_i32",
            atomicFuncType, AttributeList()));
    }
    std::vector<Value*> args;
    args.push_back(outputBufferPtr);
    args.push_back(dataSize);

    return CallInst::Create(m_atomicAddFunc, args, name, &printfCall);
}

unsigned int OpenCLPrintfResolution::getArgTypeSize(IGC::SHADER_PRINTF_TYPE argType, uint vecSize)
{
    switch (argType) {
    case IGC::SHADER_PRINTF_BYTE:
        return 1;
    case IGC::SHADER_PRINTF_SHORT:
        return 2;
    case IGC::SHADER_PRINTF_LONG:
    case IGC::SHADER_PRINTF_DOUBLE:
    case IGC::SHADER_PRINTF_POINTER:    // Runtime expects 64 bit value for pointer regardless of its actual size.
        return 8;
    case IGC::SHADER_PRINTF_VECTOR_LONG:
    case IGC::SHADER_PRINTF_VECTOR_DOUBLE:
        return vecSize * 8;

    case IGC::SHADER_PRINTF_STRING_LITERAL: {
        ModuleMetaData* modMd = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        if (IGC_IS_FLAG_ENABLED(EnableZEBinary) || modMd->compOpt.EnableZEBinary) {
            // The size of the format string address
            return 8;
        } else {
            // The size of the format string index
            return 4;
        }
    }

    default:
        if (vecSize > 0) {
            return vecSize * 4;
        }
        else {
            return  4;
        }
    }
}

unsigned int OpenCLPrintfResolution::getTotalDataSize()
{
    IGC_ASSERT_MESSAGE(m_argDescriptors.size() > 0, "Empty printf arguments list.");
    unsigned int dataSize = 0;
    SPrintfArgDescriptor* argDesc = &m_argDescriptors[0];
    // Add the size that represents a format string.
    dataSize += getArgTypeSize(argDesc->argType, argDesc->vecSize);

    // Skip 0-th operand (format string) and count total size of
    // the remaining arguments.
    for (size_t i = 1, size = m_argDescriptors.size(); i < size; ++i)
    {
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

IGC::SHADER_PRINTF_TYPE OpenCLPrintfResolution::getPrintfArgDataType(Value* printfArg)
{
    Type* argType = printfArg->getType();

    if (auto argVType = dyn_cast<VectorType>(argType))
    {
        Type* elemType = argVType->getElementType();
        if (elemType->isFloatingPointTy())
        {
            if (elemType->isDoubleTy())
                return IGC::SHADER_PRINTF_VECTOR_DOUBLE;
            else
                return IGC::SHADER_PRINTF_VECTOR_FLOAT;
        }
        else if (elemType->isIntegerTy())
        {
            unsigned int typeSize = elemType->getScalarSizeInBits();
            switch (typeSize)
            {
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
    }
    else if (argType->isFloatingPointTy())
    {
        if (argType->isDoubleTy())
            return IGC::SHADER_PRINTF_DOUBLE;
        else
            return IGC::SHADER_PRINTF_FLOAT;
    }
    else if (argType->isIntegerTy())
    {
        unsigned int typeSize = argType->getScalarSizeInBits();
        switch (typeSize)
        {
        case 8:
            return IGC::SHADER_PRINTF_BYTE;
        case 16:
            return IGC::SHADER_PRINTF_SHORT;
        case 32:
            return IGC::SHADER_PRINTF_INT;
        case 64:
            return IGC::SHADER_PRINTF_LONG;
        }
    }
    else if (argIsString(printfArg))
    {
        return IGC::SHADER_PRINTF_STRING_LITERAL;
    }
    else if (argType->isPointerTy())
    {
        return IGC::SHADER_PRINTF_POINTER;
    }
    return IGC::SHADER_PRINTF_INVALID;
}

Instruction* OpenCLPrintfResolution::generateCastToPtr(SPrintfArgDescriptor* argDesc,
    Value* writeOffset, BasicBlock* bblock)
{
    Type* castedType = nullptr;

    switch (argDesc->argType)
    {
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
        Type* origType = argDesc->value->getType();
        castedType = origType->getPointerTo(ADDRESS_SPACE_GLOBAL);
        break;
    }

    case IGC::SHADER_PRINTF_STRING_LITERAL: {
        ModuleMetaData* modMd = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        if (IGC_IS_FLAG_ENABLED(EnableZEBinary) || modMd->compOpt.EnableZEBinary)
            castedType = m_ptrSizeIntType->getPointerTo(ADDRESS_SPACE_GLOBAL);
        else
            castedType = Type::getInt32PtrTy(*m_context, ADDRESS_SPACE_GLOBAL);
        break;
    }
    case IGC::SHADER_PRINTF_POINTER:
        castedType = m_ptrSizeIntType->getPointerTo(ADDRESS_SPACE_GLOBAL);
        break;

    default:
        IGC_ASSERT_MESSAGE(0, "Unexpected printf argument type");
        break;
    }

    return CastInst::Create(Instruction::CastOps::IntToPtr,
        writeOffset,
        castedType,
        "write_offset_ptr",
        bblock);
}

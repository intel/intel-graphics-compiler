/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/ValueTracking.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-programscope-constant-analysis"
#define PASS_DESCRIPTION "Creates annotations for OpenCL program-scope structures"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ProgramScopeConstantAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ProgramScopeConstantAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ProgramScopeConstantAnalysis::ID = 0;

ProgramScopeConstantAnalysis::ProgramScopeConstantAnalysis() : ModulePass(ID)
{
    initializeProgramScopeConstantAnalysisPass(*PassRegistry::getPassRegistry());
}

bool ProgramScopeConstantAnalysis::runOnModule(Module& M)
{
    bool hasInlineConstantBuffer = false;
    bool hasInlineGlobalBuffer = false;

    BufferOffsetMap inlineProgramScopeOffsets;

    // maintains pointer information so we can patch in
    // actual pointer addresses in runtime.
    PointerOffsetInfoList pointerOffsetInfoList;

    LLVMContext& C = M.getContext();
    m_DL = &M.getDataLayout();

    auto Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    MetaDataUtils* mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_pModuleMd = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    SmallVector<GlobalVariable*, 32> zeroInitializedGlobals;

    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
    {
        GlobalVariable* globalVar = &(*I);

        PointerType* const ptrType = cast<PointerType>(globalVar->getType());
        IGC_ASSERT_MESSAGE(nullptr != ptrType, "The type of a global variable must be a pointer type");

        // Pointer's address space should be either constant or global
        // The ?: is a workaround for clang bug, clang creates string constants with private address sapce!
        // When clang bug is fixed it should become:
        // const unsigned AS = ptrType->getAddressSpace();
        const unsigned AS = ptrType->getAddressSpace() != ADDRESS_SPACE_PRIVATE ? ptrType->getAddressSpace() : ADDRESS_SPACE_CONSTANT;

        if (ptrType->getAddressSpace() == ADDRESS_SPACE_PRIVATE)
        {
            Ctx->m_hasGlobalInPrivateAddressSpace = true;
        }

        // local address space variables are also generated as GlobalVariables.
        // Ignore them here.
        if (AS == ADDRESS_SPACE_LOCAL)
        {
            continue;
        }

        if (AS != ADDRESS_SPACE_CONSTANT &&
            AS != ADDRESS_SPACE_GLOBAL)
        {
            IGC_ASSERT_MESSAGE(0, "program scope variable with unexpected address space");
            continue;
        }

        if (!globalVar->hasInitializer())
        {
            Value* inst = nullptr;
            for (auto u : globalVar->users())
            {
                if (dyn_cast_or_null<Instruction>(u))
                {
                    inst = u;
                }
            }
            std::string ErrorMsg = "Global constant without initializer!";
            Ctx->EmitError(ErrorMsg.c_str(), inst);
            continue;
        }

        // The only way to get a null initializer is via an external variable.
        // Linking has already occurred; everything should be resolved.
        Constant* initializer = globalVar->getInitializer();
        IGC_ASSERT_MESSAGE(initializer, "Constant must be initialized");
        if (!initializer)
        {
            continue;
        }

        // If this variable isn't used, don't add it to the buffer.
        if (globalVar->use_empty())
        {
            // If compiler requests global symbol for external/common linkage, add it reguardless if it is used
            bool requireGlobalSymbol = Ctx->enableTakeGlobalAddress() &&
                (globalVar->hasCommonLinkage() || globalVar->hasExternalLinkage());

            if (!requireGlobalSymbol)
                continue;
        }

        DataVector* inlineProgramScopeBuffer = nullptr;
        if (AS == ADDRESS_SPACE_GLOBAL)
        {
            if (!hasInlineGlobalBuffer)
            {
                InlineProgramScopeBuffer ilpsb;
                ilpsb.alignment = 0;
                ilpsb.allocSize = 0;
                m_pModuleMd->inlineGlobalBuffers.push_back(ilpsb);
                hasInlineGlobalBuffer = true;
            }
            inlineProgramScopeBuffer = &m_pModuleMd->inlineGlobalBuffers.back().Buffer;
        }
        else
        {
            if (!hasInlineConstantBuffer)
            {
                // General constants
                InlineProgramScopeBuffer ilpsb;
                ilpsb.alignment = 0;
                ilpsb.allocSize = 0;
                m_pModuleMd->inlineConstantBuffers.push_back(ilpsb);

                // String literals
                InlineProgramScopeBuffer ilpsbString;
                ilpsbString.alignment = 0;
                ilpsbString.allocSize = 0;
                m_pModuleMd->inlineConstantBuffers.push_back(ilpsbString);
                hasInlineConstantBuffer = true;
            }

            // When ZeBin is enabled, constant variables that are string literals
            // will be stored in the second const buffer
            ConstantDataSequential* cds = dyn_cast<ConstantDataSequential>(initializer);
            bool isStringConst = cds && (cds->isCString() || cds->isString());
            if ((IGC_IS_FLAG_ENABLED(EnableZEBinary) || m_pModuleMd->compOpt.EnableZEBinary) &&
                isStringConst)
            {
                inlineProgramScopeBuffer = &m_pModuleMd->inlineConstantBuffers[1].Buffer;
            }
            else
            {
                inlineProgramScopeBuffer = &m_pModuleMd->inlineConstantBuffers[0].Buffer;
            }
        }

        if (initializer->isZeroValue())
        {
            // For zero initialized values, we dont need to copy the data, just tell driver how much to allocate
            // However, if it's used as a pointer value, we need to do patching and therefore cannot defer the offset calculation
            bool hasPointerUser = false;
            for (auto UI : globalVar->users())
            {
                if (isa<Constant>(UI) && UI->getType()->isPointerTy())
                {
                    hasPointerUser = true;
                    break;
                }
            }
            if (!hasPointerUser)
            {
                zeroInitializedGlobals.push_back(globalVar);
                continue;
            }
        }

        // Align the buffer.
        if (inlineProgramScopeBuffer->size() != 0)
        {
#if LLVM_VERSION_MAJOR < 11
            alignBuffer(*inlineProgramScopeBuffer, m_DL->getPreferredAlignment(globalVar));
#else
            alignBuffer(*inlineProgramScopeBuffer, (unsigned int)m_DL->getPreferredAlign(globalVar).value());
#endif
        }

        // Ok, buffer is aligned, remember where this inline variable starts.
        inlineProgramScopeOffsets[globalVar] = inlineProgramScopeBuffer->size();

        // Add the data to the buffer
        addData(initializer, *inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, AS);
    }

    // Set the needed allocation size to the actual buffer size
    if (hasInlineGlobalBuffer)
        m_pModuleMd->inlineGlobalBuffers.back().allocSize = m_pModuleMd->inlineGlobalBuffers.back().Buffer.size();
    if (hasInlineConstantBuffer)
    {
        m_pModuleMd->inlineConstantBuffers[0].allocSize = m_pModuleMd->inlineConstantBuffers[0].Buffer.size();
        m_pModuleMd->inlineConstantBuffers[1].allocSize = m_pModuleMd->inlineConstantBuffers[1].Buffer.size();
    }
    // Calculate the correct offsets for zero-initialized globals/constants
    // Total allocation size in runtime needs to include zero-init values, but data copied to compiler output can ignore them
    for (auto globalVar : zeroInitializedGlobals)
    {
        unsigned AS = cast<PointerType>(globalVar->getType())->getAddressSpace();
        unsigned &offset = (AS == ADDRESS_SPACE_GLOBAL) ? m_pModuleMd->inlineGlobalBuffers.back().allocSize : m_pModuleMd->inlineConstantBuffers[0].allocSize;
#if LLVM_VERSION_MAJOR < 11
        offset = iSTD::Align(offset, m_DL->getPreferredAlignment(globalVar));
#else
        offset = iSTD::Align(offset, (unsigned)m_DL->getPreferredAlign(globalVar).value());
#endif
        inlineProgramScopeOffsets[globalVar] = offset;
        offset += (unsigned)(m_DL->getTypeAllocSize(globalVar->getType()->getPointerElementType()));
    }

    if (inlineProgramScopeOffsets.size())
    {
        // Add globals tracked in metadata to the "llvm.used" list so they won't be deleted by optimizations
        llvm::SmallVector<GlobalValue*, 4> gvec;
        for (auto Node : inlineProgramScopeOffsets)
        {
            gvec.push_back(Node.first);
        }
        ArrayRef<GlobalValue*> globalArray(gvec);
        IGC::appendToUsed(M, globalArray);
    }

    // Check if zebin is enabled
    bool zebinEnable = IGC_IS_FLAG_ENABLED(EnableZEBinary) || m_pModuleMd->compOpt.EnableZEBinary;

    // patch-token-path:
    //     Just add the implicit argument to each function if a constant
    //     buffer has been created.  This will technically burn a patch
    //     token on kernels that don't actually use the buffer but it saves
    //     us having to walk the def-use chain (we can't just check if a
    //     constant is used in the kernel; for example, a global buffer
    //     may contain pointers that in turn point into the constant
    //     address space).
    // zebinary path:
    //     Don't add the implicit arguments and rely solely on relocations
    //     for global variable reference since the implicit arguments were
    //     removed from zebinary.
    if (!zebinEnable && hasInlineConstantBuffer)
    {
        for (auto& pFunc : M)
        {
            if (pFunc.isDeclaration()) continue;
            // Skip functions called from function marked with stackcall attribute
            if (AddImplicitArgs::hasStackCallInCG(&pFunc, *Ctx)) continue;

            // Always add for kernels and subroutines
            SmallVector<ImplicitArg::ArgType, 1> implicitArgs;
            implicitArgs.push_back(ImplicitArg::CONSTANT_BASE);
            ImplicitArgs::addImplicitArgs(pFunc, implicitArgs, mdUtils);
        }
    }

    if (!zebinEnable && hasInlineGlobalBuffer)
    {
        for (auto& pFunc : M)
        {
            if (pFunc.isDeclaration()) continue;
            // Skip functions called from function marked with stackcall attribute
            if (AddImplicitArgs::hasStackCallInCG(&pFunc, *Ctx)) continue;

            // Always add for kernels and subroutines
            SmallVector<ImplicitArg::ArgType, 1> implicitArgs;
            implicitArgs.push_back(ImplicitArg::GLOBAL_BASE);
            ImplicitArgs::addImplicitArgs(pFunc, implicitArgs, mdUtils);
        }
    }

    // Setup the metadata for pointer patch info to be utilized during
    // OCL codegen.

    if (pointerOffsetInfoList.size() > 0)
    {
        for (auto& info : pointerOffsetInfoList)
        {
            // We currently just use a single buffer at index 0; hardcode
            // the patch to reference it.

            if (info.AddressSpaceWherePointerResides == ADDRESS_SPACE_GLOBAL)
            {
                PointerProgramBinaryInfo ppbi;
                ppbi.PointerBufferIndex = 0;
                ppbi.PointerOffset = int_cast<int32_t>(info.PointerOffsetFromBufferBase);
                ppbi.PointeeBufferIndex = 0;
                ppbi.PointeeAddressSpace = info.AddressSpacePointedTo;
                m_pModuleMd->GlobalPointerProgramBinaryInfos.push_back(ppbi);
            }
            else if (info.AddressSpaceWherePointerResides == ADDRESS_SPACE_CONSTANT)
            {
                PointerProgramBinaryInfo ppbi;
                ppbi.PointerBufferIndex = 0;
                ppbi.PointerOffset = int_cast<int32_t>(info.PointerOffsetFromBufferBase);
                ppbi.PointeeBufferIndex = 0;
                ppbi.PointeeAddressSpace = info.AddressSpacePointedTo;
                m_pModuleMd->ConstantPointerProgramBinaryInfos.push_back(ppbi);
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "trying to patch unsupported address space");
            }
        }
    }

    const bool changed = !inlineProgramScopeOffsets.empty();
    for (auto offset : inlineProgramScopeOffsets)
    {
        m_pModuleMd->inlineProgramScopeOffsets[offset.first] = offset.second;
    }

    // Update LLVM metadata based on IGC MetadataUtils
    if (changed)
    {
        mdUtils->save(C);
    }

    return changed;
}

void ProgramScopeConstantAnalysis::alignBuffer(DataVector& buffer, unsigned int alignment)
{
    int bufferLen = buffer.size();
    int alignedLen = iSTD::Align(bufferLen, alignment);
    if (alignedLen > bufferLen)
    {
        buffer.insert(buffer.end(), alignedLen - bufferLen, 0);
    }
}

/////////////////////////////////////////////////////////////////
//
// WalkCastsToFindNamedAddrSpace()
//
// If a generic address space pointer is discovered, we attmept
// to walk back to find the named address space if we can.
//
static unsigned WalkCastsToFindNamedAddrSpace(const Value* val)
{
    IGC_ASSERT(isa<PointerType>(val->getType()));

    const unsigned currAddrSpace = cast<PointerType>(val->getType())->getAddressSpace();

    if (currAddrSpace != ADDRESS_SPACE_GENERIC)
    {
        return currAddrSpace;
    }

    if (const Operator * op = dyn_cast<Operator>(val))
    {
        // look through the bitcast (to be addrspacecast in 3.4).
        if (op->getOpcode() == Instruction::BitCast ||
            op->getOpcode() == Instruction::AddrSpaceCast)
        {
            return WalkCastsToFindNamedAddrSpace(op->getOperand(0));
        }
        // look through the (inttoptr (ptrtoint @a)) combo.
        else if (op->getOpcode() == Instruction::IntToPtr)
        {
            if (const Operator * opop = dyn_cast<Operator>(op->getOperand(0)))
            {
                if (opop->getOpcode() == Instruction::PtrToInt)
                {
                    return WalkCastsToFindNamedAddrSpace(opop->getOperand(0));
                }
            }
        }
        // Just look through the gep if it does no offset arithmetic.
        else if (const GEPOperator * GEP = dyn_cast<GEPOperator>(op))
        {
            if (GEP->hasAllZeroIndices())
            {
                return WalkCastsToFindNamedAddrSpace(GEP->getPointerOperand());
            }
        }
    }

    return currAddrSpace;
}

void ProgramScopeConstantAnalysis::addData(Constant* initializer,
    DataVector& inlineProgramScopeBuffer,
    PointerOffsetInfoList& pointerOffsetInfoList,
    BufferOffsetMap& inlineProgramScopeOffsets,
    unsigned addressSpace)
{
    // Initial alignment padding before insert the current constant into the buffer.
    alignBuffer(inlineProgramScopeBuffer, (unsigned)m_DL->getABITypeAlignment(initializer->getType()));

    // We need to do extra work with pointers here: we don't know their actual addresses
    // at compile time so we find the offset from the base of the buffer they point to
    // so we can patch in the absolute address later.
    if (PointerType * ptrType = dyn_cast<PointerType>(initializer->getType()))
    {
        int64_t offset = 0;
        const unsigned int pointerSize = int_cast<unsigned int>(m_DL->getTypeAllocSize(ptrType));
        // This case is the most common: here, we look for a pointer that can be decomposed into
        // a base + offset with the base itself being another global variable previously defined.
        if (GlobalVariable * ptrBase = dyn_cast<GlobalVariable>(GetPointerBaseWithConstantOffset(initializer, offset, *m_DL)))
        {
            const unsigned pointedToAddrSpace = WalkCastsToFindNamedAddrSpace(initializer);

            IGC_ASSERT(addressSpace == ADDRESS_SPACE_GLOBAL || addressSpace == ADDRESS_SPACE_CONSTANT);

            // We can only patch global and constant pointers.
            if (pointedToAddrSpace == ADDRESS_SPACE_GLOBAL || pointedToAddrSpace == ADDRESS_SPACE_CONSTANT)
            {
                if (IGC_IS_FLAG_ENABLED(EnableZEBinary) || m_pModuleMd->compOpt.EnableZEBinary)
                {
                    // For zebin, instead of relying on the old patching logic, we can let RT directly patch the
                    // physical address of the previously defined global into the current buffer that uses it.
                    // TODO: Remove old patch logic when zebin is enabled
                    auto relocInfo = (addressSpace == ADDRESS_SPACE_GLOBAL) ?
                        &m_pModuleMd->GlobalBufferAddressRelocInfo :
                        &m_pModuleMd->ConstantBufferAddressRelocInfo;

                    PointerAddressRelocInfo ginfo;
                    ginfo.BufferOffset = inlineProgramScopeBuffer.size();
                    ginfo.PointerSize = pointerSize;
                    ginfo.Symbol = ptrBase->getName().str();
                    relocInfo->push_back(ginfo);

                    // Here, we write the offset relative to the start of the base global var.
                    // Runtime will add the base global's absolute address to the offset.
                    inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), (char*)&offset, ((char*)&offset) + pointerSize);
                }
                else
                {
                    auto iter = inlineProgramScopeOffsets.find(ptrBase);
                    IGC_ASSERT(iter != inlineProgramScopeOffsets.end());

                    const uint64_t pointeeOffset = iter->second + offset;

                    pointerOffsetInfoList.push_back(
                        PointerOffsetInfo(
                            addressSpace,
                            inlineProgramScopeBuffer.size(),
                            pointedToAddrSpace));

                    // For old patching logic, write the offset relative to the entire global/constant buffer where the base global resides.
                    // The base address of the buffer will be added to it at runtime.
                    inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), (char*)&pointeeOffset, ((char*)&pointeeOffset) + pointerSize);
                }
            }
            else
            {
                // Just insert zero here.  This may be some pointer to private that will be set sometime later
                // inside a kernel.  We can't patch it in so we just set it to zero here.
                inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), pointerSize, 0);
            }
        }
        else if (dyn_cast<ConstantPointerNull>(initializer))
        {
            inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), pointerSize, 0);
        }
        else if (isa<FunctionType>(ptrType->getPointerElementType()))
        {
            // Save patch info for function pointer to be patched later by runtime
            // The initializer value must be a function pointer and has the "referenced-indirectly" attribute
            Function* F = dyn_cast<Function>(initializer);
            if (F && F->hasFnAttribute("referenced-indirectly"))
            {
                IGC_ASSERT(addressSpace == ADDRESS_SPACE_GLOBAL || addressSpace == ADDRESS_SPACE_CONSTANT);
                IGC_ASSERT(pointerSize == 8 || pointerSize == 4);
                auto relocInfo = (addressSpace == ADDRESS_SPACE_GLOBAL) ?
                    &m_pModuleMd->GlobalBufferAddressRelocInfo :
                    &m_pModuleMd->ConstantBufferAddressRelocInfo;

                PointerAddressRelocInfo finfo;
                finfo.BufferOffset = inlineProgramScopeBuffer.size();
                finfo.PointerSize = pointerSize;
                finfo.Symbol = F->getName().str();
                relocInfo->push_back(finfo);
            }
            inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), pointerSize, 0);
        }
        else if (ConstantExpr * ce = dyn_cast<ConstantExpr>(initializer))
        {
            if (ce->getOpcode() == Instruction::IntToPtr)
            {
                // intoptr can technically convert vectors of ints into vectors of pointers
                // in an LLVM sense but OpenCL has no vector of pointers type.
                if (isa<ConstantInt>(ce->getOperand(0))) {
                    uint64_t val = *cast<ConstantInt>(ce->getOperand(0))->getValue().getRawData();
                    inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), (char*)& val, ((char*)& val) + pointerSize);
                }
                else {
                    addData(ce->getOperand(0), inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
                }
            }
            else if (GEPOperator * GEP = dyn_cast<GEPOperator>(ce))
            {
                for (auto& Op : GEP->operands())
                    if (Constant * C = dyn_cast<Constant>(&Op))
                        addData(C, inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
            }
            else if (ce->getOpcode() == Instruction::AddrSpaceCast ||
                ce->getOpcode() == Instruction::BitCast)
            {
                if (Constant * C = dyn_cast<Constant>(ce->getOperand(0)))
                    addData(C, inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "unknown constant expression");
            }
        }
        else
        {
            // What other shapes can pointers take at the program scope?
            IGC_ASSERT_MESSAGE(0, "unknown pointer shape encountered");
        }
    }
    else if (const UndefValue * UV = dyn_cast<UndefValue>(initializer))
    {
        // It's undef, just throw in zeros.
        const unsigned int zeroSize = int_cast<unsigned int>(m_DL->getTypeAllocSize(UV->getType()));
        inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), zeroSize, 0);
    }
    // Must check for constant expressions before we start doing type-based checks
    else if (ConstantExpr * ce = dyn_cast<ConstantExpr>(initializer))
    {
        // Constant expressions are evil. We only handle a subset that we expect.
        // Right now, this means a bitcast, or a ptrtoint/inttoptr pair.
        // Handle it by adding the source of the cast.
        if (ce->getOpcode() == Instruction::BitCast ||
            ce->getOpcode() == Instruction::AddrSpaceCast)
        {
            addData(ce->getOperand(0), inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
        }
        else if (ce->getOpcode() == Instruction::IntToPtr)
        {
            ConstantExpr* const opExpr = dyn_cast<ConstantExpr>(ce->getOperand(0));
            IGC_ASSERT_MESSAGE(nullptr != opExpr, "Unexpected operand of IntToPtr");
            IGC_ASSERT_MESSAGE(opExpr->getOpcode() == Instruction::PtrToInt, "Unexpected operand of IntToPtr");
            addData(opExpr->getOperand(0), inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
        }
        else if (ce->getOpcode() == Instruction::PtrToInt)
        {
            addData(ce->getOperand(0), inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unexpected constant expression type");
        }
    }
    else if (ConstantDataSequential * cds = dyn_cast<ConstantDataSequential>(initializer))
    {
        for (unsigned i = 0; i < cds->getNumElements(); i++) {
            addData(cds->getElementAsConstant(i), inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
        }
    }
    else if (ConstantAggregateZero * cag = dyn_cast<ConstantAggregateZero>(initializer))
    {
        // Zero aggregates are filled with, well, zeroes.
        const unsigned int zeroSize = int_cast<unsigned int>(m_DL->getTypeAllocSize(cag->getType()));
        inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), zeroSize, 0);
    }
    // If this is an sequential type which is not a CDS or zero, have to collect the values
    // element by element. Note that this is not exclusive with the two cases above, so the
    // order of ifs is meaningful.
    else if (
        initializer->getType()->isArrayTy() ||
        initializer->getType()->isStructTy() ||
        initializer->getType()->isVectorTy()
    )
    {
        const int numElts = initializer->getNumOperands();
        for (int i = 0; i < numElts; ++i)
        {
            Constant* C = initializer->getAggregateElement(i);
            IGC_ASSERT_MESSAGE(C, "getAggregateElement returned null, unsupported constant");
            // Since the type may not be primitive, extra alignment is required.
            addData(C, inlineProgramScopeBuffer, pointerOffsetInfoList, inlineProgramScopeOffsets, addressSpace);
        }
    }
    // And, finally, we have to handle base types - ints and floats.
    else
    {
        APInt intVal(32, 0, false);
        if (ConstantInt * ci = dyn_cast<ConstantInt>(initializer))
        {
            intVal = ci->getValue();
        }
        else if (ConstantFP * cfp = dyn_cast<ConstantFP>(initializer))
        {
            intVal = cfp->getValueAPF().bitcastToAPInt();
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported constant type");
        }

        const int bitWidth = intVal.getBitWidth();
        IGC_ASSERT_MESSAGE((bitWidth % 8 == 0), "Unsupported bitwidth");
        IGC_ASSERT_MESSAGE((bitWidth <= 64), "Unsupported bitwidth");

        const uint64_t* const val = intVal.getRawData();
        inlineProgramScopeBuffer.insert(inlineProgramScopeBuffer.end(), (char*)val, ((char*)val) + (bitWidth / 8));
    }


    // final padding.  This gets used by the vec3 types that will insert zero padding at the
    // end after inserting the actual vector contents (this is due to sizeof(vec3) == 4 * sizeof(scalarType)).
    alignBuffer(inlineProgramScopeBuffer, (unsigned)m_DL->getABITypeAlignment(initializer->getType()));
}

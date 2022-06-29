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

// This file defines utility classes and functions shared by SPIR-V
// reader/writer.

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Support/ToolOutputFile.h"

#include <llvm/Support/ScaledNumber.h>
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Regex.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "common/LLVMWarningsPop.hpp"

#include "libSPIRV/SPIRVInstruction.h"
#include "SPIRVInternal.h"
#include "Mangler/ParameterType.h"
#include "Probe/Assertion.h"

namespace igc_spv{

void
saveLLVMModule(Module *M, const std::string &OutputFile) {
  std::error_code EC;
  llvm::ToolOutputFile Out(OutputFile.c_str(), EC, sys::fs::OF_None);
  IGC_ASSERT_EXIT_MESSAGE((!EC), "Failed to open file");
  IGCLLVM::WriteBitcodeToFile(M, Out.os());
  Out.keep();
}

PointerType*
getOrCreateOpaquePtrType(Module *M, const std::string &Name,
    unsigned AddrSpace) {
  auto OpaqueType = IGCLLVM::getTypeByName(M, Name);
  if (!OpaqueType)
    OpaqueType = StructType::create(M->getContext(), Name);
  return PointerType::get(OpaqueType, AddrSpace);
}

void
getFunctionTypeParameterTypes(llvm::FunctionType* FT,
    std::vector<Type*>& ArgTys) {
  for (auto I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
    ArgTys.push_back(*I);
  }
}

Function *
getOrCreateFunction(Module *M, Type *RetTy, ArrayRef<Type *> ArgTypes,
    StringRef Name, bool builtin, AttributeList *Attrs, bool takeName) {
  std::string FuncName(Name);
  if (builtin)
     decorateSPIRVBuiltin(FuncName, ArgTypes);

  FunctionType *FT = FunctionType::get(
      RetTy,
      ArgTypes,
      false);
  Function *F = M->getFunction(FuncName);
  if (!F || F->getFunctionType() != FT) {
    auto NewF = Function::Create(FT,
      GlobalValue::ExternalLinkage,
      FuncName,
      M);
    if (F && takeName)
      NewF->takeName(F);
    F = NewF;
    F->setCallingConv(CallingConv::SPIR_FUNC);
    if (Attrs)
      F->setAttributes(*Attrs);
  }
  return F;
}

std::vector<Value *>
getArguments(CallInst* CI) {
  std::vector<Value*> Args;
  for (unsigned I = 0, E = IGCLLVM::getNumArgOperands(CI); I != E; ++I) {
    Args.push_back(CI->getArgOperand(I));
  }
  return Args;
}

std::string recursive_mangle(const Type* pType)
{
    Type::TypeID ID = pType->getTypeID();

    switch (ID)
    {
        case Type::FloatTyID:
            return "f32";
        case Type::DoubleTyID:
            return "f64";
        case Type::HalfTyID:
            return "f16";
        case Type::IntegerTyID:
            return "i" + utostr(pType->getIntegerBitWidth());
        case IGCLLVM::VectorTyID:
        {
            unsigned vecLen = (unsigned)cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
            Type* pEltType = cast<VectorType>(pType)->getElementType();
            return "v" + utostr(vecLen) + recursive_mangle(pEltType);
        }
        case Type::PointerTyID:
        {
            unsigned int AS = pType->getPointerAddressSpace();
            Type* pPointeeType = pType->getPointerElementType();

            StructType* ST = dyn_cast<StructType>(pPointeeType);
            if (ST && ST->isOpaque())
            {
                StringRef structName = ST->getName();
                bool isImage = structName.startswith(std::string(kSPIRVTypeName::PrefixAndDelim) + std::string(kSPIRVTypeName::Image)) ||
                    structName.startswith(std::string(kSPIRVTypeName::PrefixAndDelim) + std::string(kSPIRVTypeName::SampledImage));
                if (isImage)
                {
                    SmallVector<StringRef, 8> matches;
                    Regex regex("([0-6])_([0-2])_([0-1])_([0-1])_([0-2])_([0-9]+)_([0-2])");
                    SPIRVTypeImageDescriptor Desc;
                    if (regex.match(structName, &matches))
                    {
                        uint8_t dimension = 0;
                        matches[1].getAsInteger(0, dimension);
                        Desc.Dim = static_cast<SPIRVImageDimKind>(dimension);
                        matches[2].getAsInteger(0, Desc.Depth);
                        matches[3].getAsInteger(0, Desc.Arrayed);
                        matches[4].getAsInteger(0, Desc.MS);
                        matches[5].getAsInteger(0, Desc.Sampled);
                        matches[6].getAsInteger(0, Desc.Format);

                        uint8_t spirvAccess = 0;
                        matches[7].getAsInteger(0, spirvAccess);
                        SPIRVAccessQualifierKind Acc = static_cast<SPIRVAccessQualifierKind>(spirvAccess);

                        std::string typeMangling = SPIRVImageManglingMap::map(Desc);
                        switch (Acc)
                        {
                        case AccessQualifierReadOnly:
                            typeMangling += "_ro";
                            break;
                        case AccessQualifierWriteOnly:
                            typeMangling += "_wo";
                            break;
                        case AccessQualifierReadWrite:
                            typeMangling += "_rw";
                            break;
                        default:
                            IGC_ASSERT_MESSAGE(0, "Unsupported access qualifier!");
                        }
                        return typeMangling;
                    }
                    IGC_ASSERT_MESSAGE(0, "Inconsistent SPIRV image!");
                }
                bool isPipe_ro = structName.startswith(std::string(kSPIRVTypeName::PrefixAndDelim) + std::string(kSPIRVTypeName::Pipe) + "._0");
                if (isPipe_ro) return "Pipe_ro";
                bool isPipe_wo = structName.startswith(std::string(kSPIRVTypeName::PrefixAndDelim) + std::string(kSPIRVTypeName::Pipe) + "._1");
                if (isPipe_wo) return "Pipe_wo";
                bool isReserveId = structName.startswith(std::string(kSPIRVTypeName::PrefixAndDelim) + std::string(kSPIRVTypeName::ReserveId));
                if (isReserveId) return "ReserveId";
                return "i64";
            }

            return "p" + utostr(AS) + recursive_mangle(pPointeeType);
        }
        case Type::StructTyID:
        {
            auto structName = cast<StructType>(pType)->getName().str();
            auto pointPos = structName.rfind('.');
            return pointPos != structName.npos ? structName.substr(pointPos + 1) : structName;
        }
        case Type::FunctionTyID:
        {
            return "func";
        }
        case Type::ArrayTyID:
        {
            auto elemType = pType->getArrayElementType();
            auto numElems = pType->getArrayNumElements();
            return "a" + utostr(numElems) + recursive_mangle(elemType);
        }
        default:
            IGC_ASSERT_EXIT_MESSAGE(0, "unhandled type to mangle!");
            return "";
    }
}


std::string Mangler(const std::string &FuncName, const std::vector<Type*> &ArgTypes)
{
    std::string str_type;
    for (auto U : ArgTypes)
    {
        str_type += "_" + recursive_mangle(U);
    }
    return FuncName + str_type;
}

void
decorateSPIRVBuiltin(std::string &S)
{
    S = std::string(kLLVMName::builtinPrefix) + S;
}

void
decorateSPIRVBuiltin(std::string &S, std::vector<Type*> ArgTypes) {
   S.assign(std::string(kLLVMName::builtinPrefix) + Mangler(S,ArgTypes));
}

void
decorateSPIRVExtInst(std::string &S, std::vector<Type*> ArgTypes) {
   S.assign(std::string(kLLVMName::builtinExtInstPrefixOpenCL) + Mangler(S,ArgTypes));
}

bool
isFunctionBuiltin(llvm::Function* F) {
  return F && F->isDeclaration() && F->getName().startswith(kLLVMName::builtinPrefix);
}

std::string
getSPIRVBuiltinName(Op OC, SPIRVInstruction *BI, std::vector<Type*> ArgTypes, std::string suffix) {
  std::string name = OCLSPIRVBuiltinMap::map(OC);

  if (!name.empty()) {
    name = name + suffix;
    decorateSPIRVBuiltin(name, ArgTypes);
  }
  else
  {
    IGC_ASSERT_EXIT_MESSAGE(0, "Couldn't find opcode in map!");
  }
  return name;
}

CallInst *
mutateCallInst(Module *M, CallInst *CI,
    std::function<std::string (CallInst *, std::vector<Value *> &)>ArgMutate,
    bool Mangle, AttributeList *Attrs, bool TakeFuncName) {

  auto Args = getArguments(CI);
  auto NewName = ArgMutate(CI, Args);
  std::string InstName;
  if (!CI->getType()->isVoidTy() && CI->hasName()) {
    InstName = CI->getName().str();
    CI->setName(InstName + ".old");
  }

  auto NewCI = addCallInst(M, NewName, CI->getType(), Args, Attrs, CI, Mangle,
    InstName, TakeFuncName);

  Function* OldF = CI->getCalledFunction();
  Function* NewF = NewCI->getCalledFunction();
  if (!OldF->isDeclaration() &&
    NewF->isDeclaration()) {
    // This means that we need to clone the old function into new one.
    // It is needed because when Clang is compiling to llvm-bc it does the same thing.
    // If we want to link with such modules, we need to make the behaviour similar.
    IGC_ASSERT(OldF->getNumOperands() == NewF->getNumOperands());
    ValueToValueMapTy VMap;
    llvm::SmallVector<llvm::ReturnInst*, 8> Returns;
    BasicBlock* EntryBB = BasicBlock::Create(M->getContext(), "", NewF);
    IGCLLVM::IRBuilder<> builder(EntryBB);

    for (auto OldArgIt = OldF->arg_begin(), NewArgIt = NewF->arg_begin(); OldArgIt != OldF->arg_end(); ++OldArgIt, ++NewArgIt) {
      NewArgIt->setName(OldArgIt->getName().str());
      if (OldArgIt->getType() == NewArgIt->getType()) {
        VMap[&*OldArgIt] = &*NewArgIt;
      }
      else {
        IGC_ASSERT(NewArgIt->getType()->isPointerTy());
        LoadInst* Load = builder.CreateLoad(&*NewArgIt);
        VMap[&*OldArgIt] = Load;
      }
    }

    IGCLLVM::CloneFunctionInto(NewF, OldF, VMap, true, Returns);

    // Merge the basic block with Load instruction with the original entry basic block.
    BasicBlock* ClonedEntryBB = cast<BasicBlock>(VMap[&*OldF->begin()]);
    builder.CreateBr(ClonedEntryBB);
  }

  CI->replaceAllUsesWith(NewCI);
  CI->dropAllReferences();
  CI->removeFromParent();
  return NewCI;
}

void
mutateFunction(Function *F,
    std::function<std::string (CallInst *, std::vector<Value *> &)>ArgMutate,
    bool Mangle, AttributeList *Attrs, bool TakeFuncName) {
  auto M = F->getParent();
  for (auto I = F->user_begin(), E = F->user_end(); I != E;) {
    if (auto CI = dyn_cast<CallInst>(*I++))
      mutateCallInst(M, CI, ArgMutate, Mangle, Attrs, TakeFuncName);
  }
  if (F->use_empty()) {
    F->dropAllReferences();
    F->removeFromParent();
  }
}

CallInst *
addCallInst(Module *M, StringRef FuncName, Type *RetTy, ArrayRef<Value *> Args,
    AttributeList *Attrs, Instruction *Pos, bool Mangle, StringRef InstName,
    bool TakeFuncName) {
  auto F = getOrCreateFunction(M, RetTy, getTypes(Args),
      FuncName, Mangle, Attrs, TakeFuncName);
  auto CI = CallInst::Create(F, Args, InstName, Pos);
  CI->setCallingConv(F->getCallingConv());
  return CI;
}

ConstantInt *
getInt64(Module *M, int64_t value) {
  return ConstantInt::get(Type::getInt64Ty(M->getContext()), value, true);
}

ConstantInt *
getInt32(Module *M, int value) {
  return ConstantInt::get(Type::getInt32Ty(M->getContext()), value, true);
}

std::tuple<unsigned short, unsigned char, unsigned char>
decodeOCLVer(unsigned Ver) {
  unsigned short Major = Ver / 100000;
  unsigned char Minor = (Ver % 100000) / 1000;
  unsigned char Rev = Ver % 1000;
  return std::make_tuple(Major, Minor, Rev);
}

std::string getSPIRVTypeName(StringRef BaseName, StringRef Postfixes) {
    assert(!BaseName.empty() && "Invalid SPIR-V type Name");
    auto TN = std::string(kSPIRVTypeName::PrefixAndDelim) + BaseName.str();
    if (Postfixes.empty())
        return TN;
    return TN + kSPIRVTypeName::Delimiter + Postfixes.str();
}

std::string getSPIRVImageTypePostfixes(StringRef SampledType,
    SPIRVTypeImageDescriptor Desc,
    SPIRVAccessQualifierKind Acc) {
    std::string S;
    raw_string_ostream OS(S);
    OS << kSPIRVTypeName::PostfixDelim << SampledType
        << kSPIRVTypeName::PostfixDelim << Desc.Dim << kSPIRVTypeName::PostfixDelim
        << Desc.Depth << kSPIRVTypeName::PostfixDelim << Desc.Arrayed
        << kSPIRVTypeName::PostfixDelim << Desc.MS << kSPIRVTypeName::PostfixDelim
        << Desc.Sampled << kSPIRVTypeName::PostfixDelim << Desc.Format
        << kSPIRVTypeName::PostfixDelim << Acc;
    return OS.str();
}

std::string getSPIRVImageSampledTypeName(SPIRVType* Ty) {
    switch (Ty->getOpCode()) {
    case OpTypeVoid:
        return kSPIRVImageSampledTypeName::Void;
    case OpTypeInt:
        if (Ty->getIntegerBitWidth() == 32) {
            if (static_cast<SPIRVTypeInt*>(Ty)->isSigned())
                return kSPIRVImageSampledTypeName::Int;
            else
                return kSPIRVImageSampledTypeName::UInt;
        }
        break;
    case OpTypeFloat:
        switch (Ty->getFloatBitWidth()) {
        case 16:
            return kSPIRVImageSampledTypeName::Half;
        case 32:
            return kSPIRVImageSampledTypeName::Float;
        default:
            break;
        }
        break;
    default:
        break;
    }
    llvm_unreachable("Invalid sampled type for image");
    return std::string();
}

bool isSPIRVSamplerType(llvm::Type* Ty) {
  if (auto PT = dyn_cast<PointerType>(Ty))
    if (auto ST = dyn_cast<StructType>(PT->getPointerElementType()))
      if (ST->isOpaque()) {
        auto Name = ST->getName();
        if (Name.startswith(std::string(kSPIRVTypeName::PrefixAndDelim) + kSPIRVTypeName::Sampler)) {
          return true;
        }
      }
  return false;
}

}


/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <Compiler/CodeGenPublic.h>
#include "GenIntrinsics.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/CodeGen/ValueTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "../../inc/common/UFO/portable_compiler.h"
#include <cstring>
#include "Probe/Assertion.h"

using namespace llvm;

namespace llvm {

namespace GenISAIntrinsic {
/// Intrinsic::getType(ID) - Return the function type for an intrinsic.
///
static FunctionType *getType(LLVMContext &Context, ID id,
    ArrayRef<Type*> Tys = None);

/// Map a GCC builtin name to an intrinsic ID.
ID getIntrinsicForGCCBuiltin(const char *Prefix, const char *BuiltinName);


/// Intrinsic::isOverloaded(ID) - Returns true if the intrinsic can be
/// overloaded.
__attr_unused static bool isOverloaded(ID id);

/// IITDescriptor - This is a type descriptor which explains the type
/// requirements of an intrinsic.  This is returned by
/// getIntrinsicInfoTableEntries.
struct IITDescriptor {
    enum IITDescriptorKind {
        Void, VarArg, MMX, Token, Metadata, Half, Float, Double,
        Integer, Vector, Pointer, Struct,
        Argument, ExtendArgument, TruncArgument, HalfVecArgument,
        SameVecWidthArgument, PtrToArgument, VecOfPtrsToElt
    } Kind;

    union {
        unsigned Integer_Width;
        unsigned Float_Width;
        unsigned Vector_Width;
        unsigned Pointer_AddressSpace;
        unsigned Struct_NumElements;
        unsigned Argument_Info;
    };

    enum ArgKind {
        AK_Any,
        AK_AnyInteger,
        AK_AnyFloat,
        AK_AnyVector,
        AK_AnyPointer
    };
    unsigned getArgumentNumber() const {
        IGC_ASSERT(Kind == Argument || Kind == ExtendArgument || Kind == TruncArgument || Kind == HalfVecArgument || Kind == SameVecWidthArgument || Kind == PtrToArgument || Kind == VecOfPtrsToElt);
        return Argument_Info >> 3;
    }
    ArgKind getArgumentKind() const {
        IGC_ASSERT(Kind == Argument || Kind == ExtendArgument || Kind == TruncArgument || Kind == HalfVecArgument || Kind == SameVecWidthArgument || Kind == PtrToArgument || Kind == VecOfPtrsToElt);
        return (ArgKind)(Argument_Info & 7);
    }

    static IITDescriptor get(IITDescriptorKind K, unsigned Field) {
        IITDescriptor Result = { K,{ Field } };
        return Result;
    }
};

/// getIntrinsicInfoTableEntries - Return the IIT table descriptor for the
/// specified intrinsic into an array of IITDescriptors.
///
void getIntrinsicInfoTableEntries(ID id, SmallVectorImpl<IITDescriptor> &T, ArrayRef<Type*> Tys);
ID lookupGenIntrinsicID(const char *Name, unsigned int Len);
}
}
enum IIT_Info {
  // Common values should be encoded with 0-15.
  IIT_Done = 0,
  IIT_I1   = 1,
  IIT_I8   = 2,
  IIT_I16  = 3,
  IIT_I32  = 4,
  IIT_I64  = 5,
  IIT_F16  = 6,
  IIT_F32  = 7,
  IIT_F64  = 8,
  IIT_V2   = 9,
  IIT_V4   = 10,
  IIT_V8   = 11,
  IIT_V16  = 12,
  IIT_V32  = 13,
  IIT_PTR  = 14,
  IIT_ARG  = 15,

  // Values from 16+ are only encodable with the inefficient encoding.
  IIT_V64  = 16,
  IIT_MMX  = 17,
  IIT_TOKEN = 18,
  IIT_METADATA = 19,
  IIT_EMPTYSTRUCT = 20,
  IIT_STRUCT2 = 21,
  IIT_STRUCT3 = 22,
  IIT_STRUCT4 = 23,
  IIT_STRUCT5 = 24,
  IIT_EXTEND_ARG = 25,
  IIT_TRUNC_ARG = 26,
  IIT_ANYPTR = 27,
  IIT_V1   = 28,
  IIT_VARARG = 29,
  IIT_HALF_VEC_ARG = 30,
  IIT_SAME_VEC_WIDTH_ARG = 31,
  IIT_PTR_TO_ARG = 32,
  IIT_VEC_OF_PTRS_TO_ELT = 33,
  IIT_I128 = 34,
  IIT_V512 = 35,
  IIT_V1024 = 36
};

static Type *DecodeFixedType(ArrayRef<GenISAIntrinsic::IITDescriptor> &Infos,
                             ArrayRef<Type*> Tys, LLVMContext &Context) {
  using namespace GenISAIntrinsic;
  IITDescriptor D = Infos.front();
  Infos = Infos.slice(1);

  switch (D.Kind) {
  case IITDescriptor::Void: return Type::getVoidTy(Context);
  case IITDescriptor::VarArg: return Type::getVoidTy(Context);
  case IITDescriptor::MMX: return Type::getX86_MMXTy(Context);
  case IITDescriptor::Token: return Type::getTokenTy(Context);
  case IITDescriptor::Metadata: return Type::getMetadataTy(Context);
  case IITDescriptor::Half: return Type::getHalfTy(Context);
  case IITDescriptor::Float: return Type::getFloatTy(Context);
  case IITDescriptor::Double: return Type::getDoubleTy(Context);

  case IITDescriptor::Integer:
    return IntegerType::get(Context, D.Integer_Width);
  case IITDescriptor::Vector:
    return IGCLLVM::FixedVectorType::get(DecodeFixedType(Infos, Tys, Context),D.Vector_Width);
  case IITDescriptor::Pointer:
    return PointerType::get(DecodeFixedType(Infos, Tys, Context),
                            D.Pointer_AddressSpace);
  case IITDescriptor::Struct: {
    Type *Elts[5];
    IGC_ASSERT_MESSAGE(D.Struct_NumElements <= 5, "Can't handle this yet");
    for (unsigned i = 0, e = D.Struct_NumElements; i != e; ++i)
      Elts[i] = DecodeFixedType(Infos, Tys, Context);
    return StructType::get(Context, makeArrayRef(Elts,D.Struct_NumElements));
  }

  case IITDescriptor::Argument:
    return Tys[D.getArgumentNumber()];
  case IITDescriptor::ExtendArgument: {
    Type *Ty = Tys[D.getArgumentNumber()];
    if (VectorType *VTy = dyn_cast<VectorType>(Ty))
      return VectorType::getExtendedElementVectorType(VTy);

    return IntegerType::get(Context, 2 * cast<IntegerType>(Ty)->getBitWidth());
  }
  case IITDescriptor::TruncArgument: {
    Type *Ty = Tys[D.getArgumentNumber()];
    if (VectorType *VTy = dyn_cast<VectorType>(Ty))
      return VectorType::getTruncatedElementVectorType(VTy);

    IntegerType *ITy = cast<IntegerType>(Ty);
    IGC_ASSERT(nullptr != ITy);
    IGC_ASSERT(ITy->getBitWidth() % 2 == 0);
    return IntegerType::get(Context, ITy->getBitWidth() / 2);
  }
  case IITDescriptor::HalfVecArgument:
    return VectorType::getHalfElementsVectorType(cast<VectorType>(
                                                  Tys[D.getArgumentNumber()]));
  case IITDescriptor::SameVecWidthArgument: {
    Type *EltTy = DecodeFixedType(Infos, Tys, Context);
    Type *Ty = Tys[D.getArgumentNumber()];
    if (IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty)) {
      return IGCLLVM::FixedVectorType::get(EltTy, int_cast<unsigned int>(VTy->getNumElements()));
    }
    IGC_ASSERT_EXIT_MESSAGE(0, "unhandled");
  }
  case IITDescriptor::PtrToArgument: {
    Type *Ty = Tys[D.getArgumentNumber()];
    return PointerType::getUnqual(Ty);
  }
  case IITDescriptor::VecOfPtrsToElt: {
      Type *Ty = Tys[D.getArgumentNumber()];
      IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
      if (!VTy)
          IGC_ASSERT_EXIT_MESSAGE(0, "Expected an argument of Vector Type");
      Type *EltTy = cast<VectorType>(VTy)->getElementType();
      return IGCLLVM::FixedVectorType::get(PointerType::getUnqual(EltTy),
          int_cast<unsigned int>(VTy->getNumElements()));
  }
 }
  IGC_ASSERT_EXIT_MESSAGE(0, "unhandled");
  return nullptr;
}


static void DecodeIITType(unsigned &NextElt, ArrayRef<unsigned char> Infos,
                      SmallVectorImpl<GenISAIntrinsic::IITDescriptor> &OutputTable, ArrayRef<Type*> Tys = None) {
  IIT_Info Info = IIT_Info(Infos[NextElt++]);
  unsigned StructElts = 2;
  using namespace GenISAIntrinsic;

  switch (Info) {
  case IIT_Done:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Void, 0));
    return;
  case IIT_VARARG:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::VarArg, 0));
    return;
  case IIT_MMX:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::MMX, 0));
    return;
  case IIT_TOKEN:
      OutputTable.push_back(IITDescriptor::get(IITDescriptor::Token, 0));
    return;
  case IIT_METADATA:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Metadata, 0));
    return;
  case IIT_F16:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Half, 0));
    return;
  case IIT_F32:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Float, 0));
    return;
  case IIT_F64:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Double, 0));
    return;
  case IIT_I1:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Integer, 1));
    return;
  case IIT_I8:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Integer, 8));
    return;
  case IIT_I16:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Integer,16));
    return;
  case IIT_I32:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Integer, 32));
    return;
  case IIT_I64:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Integer, 64));
    return;
  case IIT_I128:
      OutputTable.push_back(IITDescriptor::get(IITDescriptor::Integer, 128));
      return;
  case IIT_V1:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 1));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V2:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 2));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V4:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 4));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V8:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 8));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V16:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 16));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V32:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 32));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V64:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 64));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_V512:
      OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 512));
      DecodeIITType(NextElt, Infos, OutputTable);
      return;
  case IIT_V1024:
      OutputTable.push_back(IITDescriptor::get(IITDescriptor::Vector, 1024));
      return;
  case IIT_PTR:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Pointer, 0));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  case IIT_ANYPTR: {  // [ANYPTR addrspace, subtype]
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Pointer,
                                             Infos[NextElt++]));
    DecodeIITType(NextElt, Infos, OutputTable);
    return;
  }
  case IIT_ARG: {
    unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
      IITDescriptor argDesc = IITDescriptor::get(IITDescriptor::Argument, ArgInfo);
      if (argDesc.getArgumentKind() == IITDescriptor::AK_Any) {
          if (argDesc.getArgumentNumber() < Tys.size()) {
              OutputTable.push_back(IITDescriptor::get(IITDescriptor::Argument, ArgInfo));
              NextElt++;
          }
          else {
              DecodeIITType(NextElt, Infos, OutputTable);
          }
      }
      else {
          OutputTable.push_back(IITDescriptor::get(IITDescriptor::Argument, ArgInfo));
      }
    return;
  }
  case IIT_EXTEND_ARG: {
    unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::ExtendArgument,
                                             ArgInfo));
    return;
  }
  case IIT_TRUNC_ARG: {
    unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::TruncArgument,
                                             ArgInfo));
    return;
  }
  case IIT_HALF_VEC_ARG: {
    unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::HalfVecArgument,
                                             ArgInfo));
    return;
  }
  case IIT_SAME_VEC_WIDTH_ARG: {
    unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::SameVecWidthArgument,
                                             ArgInfo));
    return;
  }
  case IIT_PTR_TO_ARG: {
    unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::PtrToArgument,
                                             ArgInfo));
    return;
  }
  case IIT_VEC_OF_PTRS_TO_ELT: {
      unsigned ArgInfo = (NextElt == Infos.size() ? 0 : Infos[NextElt++]);
      OutputTable.push_back(IITDescriptor::get(IITDescriptor::VecOfPtrsToElt,

                                             ArgInfo));
    return;
  }
  case IIT_EMPTYSTRUCT:
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Struct, 0));
    return;
  case IIT_STRUCT5: ++StructElts; // FALL THROUGH.
  case IIT_STRUCT4: ++StructElts; // FALL THROUGH.
  case IIT_STRUCT3: ++StructElts; // FALL THROUGH.
  case IIT_STRUCT2: {
    OutputTable.push_back(IITDescriptor::get(IITDescriptor::Struct,StructElts));

    for (unsigned i = 0; i != StructElts; ++i)
      DecodeIITType(NextElt, Infos, OutputTable);
    return;
  }
  }
  IGC_ASSERT_EXIT_MESSAGE(0, "unhandled");
}

// This defines the "Intrinsic::getIntrinsicForGCCBuiltin()" method.
#define GET_LLVM_INTRINSIC_FOR_GCC_BUILTIN
#include "IntrinsicGenISA.gen"
#undef GET_LLVM_INTRINSIC_FOR_GCC_BUILTIN

/// Returns a stable mangling for the type specified for use in the name
/// mangling scheme used by 'any' types in intrinsic signatures.  The mangling
/// of named types is simply their name.  Manglings for unnamed types consist
/// of a prefix ('p' for pointers, 'a' for arrays, 'f_' for functions)
/// combined with the mangling of their component types.  A vararg function
/// type will have a suffix of 'vararg'.  Since function types can contain
/// other function types, we close a function type mangling with suffix 'f'
/// which can't be confused with it's prefix.  This ensures we don't have
/// collisions between two unrelated function types. Otherwise, you might
/// parse ffXX as f(fXX) or f(fX)X.  (X is a placeholder for any other type.)
static std::string getMangledTypeStr(Type* Ty) {
  std::string Result;
  if (PointerType* PTyp = dyn_cast<PointerType>(Ty)) {
    Result += "p" + llvm::utostr(PTyp->getAddressSpace()) +
      getMangledTypeStr(PTyp->getElementType());
  } else if (ArrayType* ATyp = dyn_cast<ArrayType>(Ty)) {
    Result += "a" + llvm::utostr(ATyp->getNumElements()) +
      getMangledTypeStr(ATyp->getElementType());
  } else if (StructType* STyp = dyn_cast<StructType>(Ty)) {
    if(!STyp->isLiteral())
        Result += STyp->getName();
    else {
        Result += "s" + llvm::utostr(STyp->getNumElements());
        for(unsigned int i = 0; i < STyp->getNumElements(); i++)
            Result += getMangledTypeStr(STyp->getElementType(i));
    }
  } else if (FunctionType* FT = dyn_cast<FunctionType>(Ty)) {
    Result += "f_" + getMangledTypeStr(FT->getReturnType());
    for (size_t i = 0; i < FT->getNumParams(); i++)
      Result += getMangledTypeStr(FT->getParamType(i));
    if (FT->isVarArg())
      Result += "vararg";
    // Ensure nested function types are distinguishable.
    Result += "f";
  }
  else if (isa<VectorType>(Ty))
    Result += "v" + utostr(cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements()) +
      getMangledTypeStr(cast<VectorType>(Ty)->getElementType());
  else if (Ty)
    Result += EVT::getEVT(Ty).getEVTString();
  return Result;
}


std::string GenISAIntrinsic::getName(GenISAIntrinsic::ID id, ArrayRef<Type*> Tys) {
  //IGC_ASSERT_MESSAGE(id < num_genisa_intrinsics, "Invalid intrinsic ID!");
  if (id > (GenISAIntrinsic::ID)Intrinsic::num_intrinsics)
    id = (GenISAIntrinsic::ID)(id-Intrinsic::num_intrinsics);
  static const char * const Table[] = {
    "not_intrinsic",
#define GET_INTRINSIC_NAME_TABLE
#include "IntrinsicGenISA.gen"
#undef GET_INTRINSIC_NAME_TABLE
  };
  if (Tys.empty())
    return Table[id];
  std::string Result(Table[id]);
  for (unsigned i = 0; i < Tys.size(); ++i) {
    Result += "." + getMangledTypeStr(Tys[i]);
  }
  return Result;
}

GenISAIntrinsic::IntrinsicComments GenISAIntrinsic::getIntrinsicComments(
    GenISAIntrinsic::ID id) {
#define GET_INTRINSIC_COMMENTS_TABLE
#include "IntrinsicGenISA.gen"
#undef GET_INTRINSIC_COMMENTS_TABLE
}

FunctionType *GenISAIntrinsic::getType(LLVMContext &Context,
                                 ID id, ArrayRef<Type*> Tys) {
  SmallVector<IITDescriptor, 8> Table;
  getIntrinsicInfoTableEntries(id, Table, Tys);

  ArrayRef<IITDescriptor> TableRef = Table;
  Type *ResultTy = DecodeFixedType(TableRef, Tys, Context);

  SmallVector<Type*, 8> ArgTys;
  while (!TableRef.empty())
    ArgTys.push_back(DecodeFixedType(TableRef, Tys, Context));

  // DecodeFixedType returns Void for IITDescriptor::Void and IITDescriptor::VarArg
  // If we see void type as the type of the last argument, it is vararg intrinsic
  if (!ArgTys.empty() && ArgTys.back()->isVoidTy()) {
    ArgTys.pop_back();
    return FunctionType::get(ResultTy, ArgTys, true);
  }
  return FunctionType::get(ResultTy, ArgTys, false);
}

bool GenISAIntrinsic::isOverloaded(GenISAIntrinsic::ID id) {
#define GET_INTRINSIC_OVERLOAD_TABLE
#include "IntrinsicGenISA.gen"
#undef GET_INTRINSIC_OVERLOAD_TABLE
}

/// This defines the "getAttributes(ID id)" method.
#define GET_INTRINSIC_ATTRIBUTES
#include "IntrinsicGenISA.gen"
#undef GET_INTRINSIC_ATTRIBUTES

#define GET_INTRINSIC_GENERATOR_GLOBAL
#include "IntrinsicGenISA.gen"
#undef GET_INTRINSIC_GENERATOR_GLOBAL

AttributeList GenISAIntrinsic::getGenIntrinsicAttributes(
    LLVMContext& C, GenISAIntrinsic::ID id)
{
    return getAttributes(C, (GenISAIntrinsic::ID)(id - 1));
}

Function *GenISAIntrinsic::getDeclaration(
    Module *Module, GenISAIntrinsic::ID id, ArrayRef<Type*> Tys)
{
    // There can never be multiple globals with the same name of different types,
    // because intrinsics must be a specific type.
    IGCLLVM::Module* M = (IGCLLVM::Module*)Module;

    Function *F =
        dyn_cast<Function>(
            M->getOrInsertFunction(
                getName((GenISAIntrinsic::ID)(id-Intrinsic::num_intrinsics), Tys),
                getType(
                    M->getContext(),
                    (GenISAIntrinsic::ID)(id-Intrinsic::num_intrinsics),
                    Tys),
                getAttributes(M->getContext(),(GenISAIntrinsic::ID)(id - 1))));

    IGC_ASSERT_MESSAGE(F, "getOrInsertFunction probably returned constant expression!");
    // Since Function::isIntrinsic() will return true due to llvm.* prefix,
    // Module::getOrInsertFunction fails to add the attributes.
    // explicitly adding the attribute to handle this problem.
    // This since is setup on the function declaration, attribute assignment
    // is global and hence this approach suffices.
    F->setAttributes(getAttributes(M->getContext(), (GenISAIntrinsic::ID)(id - 1)));
    return F;
}

void GenISAIntrinsic::getIntrinsicInfoTableEntries(
    GenISAIntrinsic::ID id,
    SmallVectorImpl<IITDescriptor> &T,
    ArrayRef<Type*> Tys)
{
  // Check to see if the intrinsic's type was expressible by the table.
  unsigned TableVal = IIT_Table[id-1];

  // Decode the TableVal into an array of IITValues.
  SmallVector<unsigned char, 8> IITValues;
  ArrayRef<unsigned char> IITEntries;
  unsigned NextElt = 0;
  if ((TableVal >> 31) != 0) {
    // This is an offset into the IIT_LongEncodingTable.
    IITEntries = IIT_LongEncodingTable;

    // Strip sentinel bit.
    NextElt = (TableVal << 1) >> 1;
  } else {
    // Decode the TableVal into an array of IITValues.  If the entry was encoded
    // into a single word in the table itself, decode it now.
    do {
      IITValues.push_back(TableVal & 0xF);
      TableVal >>= 4;
    } while (TableVal);

    IITEntries = IITValues;
    NextElt = 0;
  }

  // Okay, decode the table into the output vector of IITDescriptors.
  DecodeIITType(NextElt, IITEntries, T, Tys);
  while (NextElt < IITEntries.size() && IITEntries[NextElt] != 0)
    DecodeIITType(NextElt, IITEntries, T, Tys);
}

GenISAIntrinsic::ID GenISAIntrinsic::getIntrinsicID(const Function *F) {
    IGC_ASSERT(nullptr != F);
    IGC::LLVMContextWrapper::SafeIntrinsicIDCacheTy* safeIntrinsicIDCache =
        &(static_cast<IGC::LLVMContextWrapper*>(&F->getContext())->m_SafeIntrinsicIDCache);
    IGC_ASSERT(nullptr != safeIntrinsicIDCache);

    //If you do not find the function ptr as key corresponding to the GenISAIntrinsic::ID add the new key
    auto it = (*safeIntrinsicIDCache).find(F);
    if (it == (*safeIntrinsicIDCache).end()) {
        GenISAIntrinsic::ID Id = no_intrinsic;
        const ValueName* const valueName = F->getValueName();
        if (nullptr != valueName)
        {
            llvm::StringRef prefix = getGenIntrinsicPrefix();
            llvm::StringRef Name = valueName->getKey();
            if (Name.size() > prefix.size() && Name.startswith(prefix))
            {
                Id = lookupGenIntrinsicID(Name.data(), Name.size());
                (*safeIntrinsicIDCache)[F] = Id;
            }
        }
        return Id;
    }
    else
    {
        // If you have an entry for the function ptr corresponding to the GenISAIntrinsic::ID return it back,
        //instead of going through a lengthy look-up.
        return (static_cast<GenISAIntrinsic::ID>(it->second));
    }
}

GenISAIntrinsic::ID GenISAIntrinsic::lookupGenIntrinsicID(const char *Name, unsigned int Len)
{
#define GET_FUNCTION_RECOGNIZER
#include "IntrinsicGenISA.gen"
#undef GET_FUNCTION_RECOGNIZER
    return no_intrinsic;
}

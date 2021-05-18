/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/ADT/StringRef.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"

#include "LLVMSPIRVLib.h"
#include "llvm/Target/TargetMachine.h"

#include "gtest/gtest.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include <strstream>
#include <memory>

using namespace llvm;

namespace {

static GenXIntrinsic::ID BeginGenXID = llvm::GenXIntrinsic::genx_3d_load;
static GenXIntrinsic::ID EndGenXID = llvm::GenXIntrinsic::genx_zzzzend;

// Currently returns some fixed types.
Type *generateAnyType(Intrinsic::IITDescriptor::ArgKind AK, LLVMContext &Ctx) {
  using namespace Intrinsic;

  switch (AK) {
  case IITDescriptor::AK_Any:
  case IITDescriptor::AK_AnyInteger:
    return Type::getInt32Ty(Ctx);
  case IITDescriptor::AK_AnyFloat:
    return Type::getDoubleTy(Ctx);
  case IITDescriptor::AK_AnyPointer:
    return Type::getInt32PtrTy(Ctx);
  case IITDescriptor::AK_AnyVector:
    return IGCLLVM::FixedVectorType::get(Type::getInt32Ty(Ctx), 8);
  }
  llvm_unreachable("All types should be handled");
}

void generateOverloadedTypes(GenXIntrinsic::ID Id, LLVMContext &Ctx,
                             SmallVectorImpl<Type *> &Tys) {
  using namespace Intrinsic;

  SmallVector<IITDescriptor, 8> Table;
  GenXIntrinsic::getIntrinsicInfoTableEntries(Id, Table);

  for (unsigned i = 0, e = Table.size(); i != e; ++i) {
    auto Desc = Table[i];
    if (Desc.Kind != IITDescriptor::Argument)
      continue;

    size_t ArgNum = Desc.getArgumentNumber();
    Tys.resize(std::max(ArgNum + 1, Tys.size()));

    Tys[ArgNum] = generateAnyType(Desc.getArgumentKind(), Ctx);
  }
}

static std::string ty2s(Type* ty) {
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  ty->print(rso, true);
  return rso.str();
}
static std::string k2s(std::map<std::string, Attribute::AttrKind>& s,
                       Attribute::AttrKind kkk) {
  for (const auto& i: s) {
    if (i.second == kkk)
      return i.first;
  }
  return "n/a";
}
class SpirvConvertionsTest : public testing::Test {
protected:
  void SetUp() override {
    M_.reset(new Module("Test_Module", Ctx_));
    M_->setTargetTriple("spir64-unknown-unknown");
  }

  void TearDown() override {
    M_.reset();
  }

  Module* Retranslate(LLVMContext& ctx, std::string& err) {
    err.clear();
    std::stringstream ss;
    writeSpirv(M_.get(), ss, err);

    if (!err.empty())
      return nullptr;

    std::string s_sv_ir = ss.str();
    std::istrstream ir_stream(s_sv_ir.data(), s_sv_ir.size());

    Module* result = nullptr;
    readSpirv(ctx, ir_stream, result, err);

    if (!err.empty())
      return nullptr;

    return result;
  }

  LLVMContext Ctx_;
  std::unique_ptr<Module> M_;
  std::set<std::string> FN_;
};

TEST_F(SpirvConvertionsTest, IntrinsicAttrs) {
  Type *FArgTy[] = {Type::getInt32PtrTy(Ctx_)};
  FunctionType *FT = FunctionType::get(Type::getVoidTy(Ctx_), FArgTy, false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "", M_.get());
  BasicBlock *BB = BasicBlock::Create(Ctx_, "", F);

  IRBuilder<> Builder(BB);

  for (unsigned id = BeginGenXID; id < EndGenXID; ++id) {
    GenXIntrinsic::ID XID = static_cast<GenXIntrinsic::ID>(id);

    SmallVector<Type *, 8> Tyss;
    generateOverloadedTypes(XID, Ctx_, Tyss);

    Function* f = GenXIntrinsic::getGenXDeclaration(M_.get(), XID, Tyss);
    SmallVector<Value *, 8> Args;
    for (Type* ty: f->getFunctionType()->params()) {
      Value* arg = llvm::Constant::getNullValue(ty);
      Args.push_back(arg);

      FN_.insert(f->getName().str());
      /*
      std::cout << "name: " << f->getName().str() << "\n";
      Type* aty = arg->getType();
      std::cout << "    param_type: " << ty2s(ty) << ' ' << (void*)ty <<  "\n";
      std::cout << "    arg_type: " << ty2s(aty) << ' ' << (void*)aty <<  "\n";
      */
    }
    Builder.CreateCall(f, Args);
  }
  llvm::Error merr = M_->materializeAll();
  if (merr)
    FAIL() << "materialization a module resulted in failure: " << merr << "\n";

  std::string err;
  LLVMContext C;
  Module* M = Retranslate(C, err);
  if (!M) {
    FAIL() << "failure during retranslation: " << err << "\n";
    return;
  }

  // M_->dump();
  // M->dump();

  for (const std::string& fname :FN_) {
    // std::cout << "processing <" << fname << ">" << "\n";
    Function* fl = M->getFunction(fname);
    Function* fr = M_->getFunction(fname);

    if (!fl)
      FAIL() << "could not find <" << fname << "> in the converted Module\n";
    if (!fr)
      FAIL() << "could not find <" << fname << "> in the original Module\n";

    // fl->getAttributes().dump();
    // fr->getAttributes().dump();

    for (unsigned i = Attribute::None; i < Attribute::EndAttrKinds; ++i) {
      Attribute::AttrKind att = (Attribute::AttrKind)i;
      EXPECT_TRUE(fl->hasFnAttribute(att) == fr->hasFnAttribute(att));
    }
  }
}

TEST_F(SpirvConvertionsTest, FunctionAttrs) {

  // TODO: think about how one can test all attributes. Right now the problem
  // is that I don't know how to diffirentiate between attributes which require
  // a value from those that don't.
  std::map<std::string, Attribute::AttrKind> kinds = {
    { "Convergent", Attribute::Convergent },
    { "NoReturn", Attribute::NoReturn },
    { "NoInline", Attribute::NoInline },
    { "NoUnwind", Attribute::NoUnwind },
    { "ReadNone", Attribute::ReadNone },
    { "SafeStack", Attribute::SafeStack },
    { "WriteOnly", Attribute::WriteOnly },
  };
  for (const auto& k : kinds) {
    Type *FArgTy[] = {Type::getInt32PtrTy(Ctx_)};
    FunctionType *FT = FunctionType::get(Type::getVoidTy(Ctx_), FArgTy, false);
    Function* test_f =
      Function::Create(FT, Function::ExternalLinkage, k.first, M_.get());
    for (unsigned i = Attribute::None; i < Attribute::EndAttrKinds; ++i) {
      if (test_f->hasFnAttribute((Attribute::AttrKind)i)) {
        test_f->removeFnAttr((Attribute::AttrKind)i);
      }
    }
    test_f->addFnAttr(k.second);
    BasicBlock *aux_BB = BasicBlock::Create(Ctx_, "", test_f);
    IRBuilder<> aux_Builder(aux_BB);
  }

  std::string err;
  LLVMContext C;
  Module* M = Retranslate(C, err);
  if (!M) {
    FAIL() << "failure during retranslation: " << err << "\n";
    return;
  }
  for (const auto& k : kinds) {
    Function* fl = M->getFunction(k.first);
    Function* fr = M_->getFunction(k.first);
    for (unsigned i = Attribute::None; i < Attribute::EndAttrKinds; ++i) {
      Attribute::AttrKind att = (Attribute::AttrKind)i;
      if ((fl->hasFnAttribute(att) != fr->hasFnAttribute(att))) {
        FAIL() << "Attriubute mismatch for <" << k.first << "> at attr:" <<
            i << " (" << k2s(kinds, att) << ")\n";
      }
    }
  }
  // M_->dump();
  // M->dump();
}


} // namespace

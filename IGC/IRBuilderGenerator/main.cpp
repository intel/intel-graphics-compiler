/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This tool is used to autogenerate C++ IR Builder code from llvm bitcode.
///
/// A simple file (reflection.cpp) includes RTStackFormat.h and implements a
/// collection of small accessor functions that serve as hooks for this tool
/// to autogenerate code.  The following annotations are currently supported:
///
/// [[clang::annotate("type-of")]]
/// void foo<T>(T x)
///   - This will generate a function that builds the type of T and has the same
///     name. If T is a pointer, it builds the pointee type.
///
/// [[clang::annotate("create")]]
///   - Generates the supported sequence of instructions via the IRBuilder.
///
/// Other function calls
///   - Calls will be processed according to their namespace. All calls should
///     be declared under the special namespace "hook".
///   - hook::bi::*
///       - Will generate a call to a function in the IRBuilder that must be
///         implemented manually.
///   - hook::fn::*
///       - Will add an additional templated function argument to the function
///         that will be passed in from the caller.
///
/// See below command line help for info on clang command line to generate the
/// input bitcode.
///
//===----------------------------------------------------------------------===//

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Support/YAMLTraits.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/YAMLParser.h"

#include <map>
#include <optional>

using namespace llvm;

// Address space descriptor from YAML
struct AddressSpaceDesc {
  std::string name;
  std::string description;
  bool constant = false;
  unsigned AS = 0; // Will be auto-assigned based on order
};

constexpr unsigned FIRST_ADDRSPACE = 100;

// YAML parsing traits
namespace llvm {
namespace yaml {
template <> struct MappingTraits<AddressSpaceDesc> {
  static void mapping(IO &io, AddressSpaceDesc &desc) {
    io.mapRequired("name", desc.name);
    io.mapRequired("description", desc.description);
    io.mapRequired("constant", desc.constant);
  }
};
} // namespace yaml
} // namespace llvm

LLVM_YAML_IS_SEQUENCE_VECTOR(AddressSpaceDesc)

struct AddressSpaceConfig {
  std::vector<AddressSpaceDesc> address_spaces;
};

namespace llvm {
namespace yaml {
template <> struct MappingTraits<AddressSpaceConfig> {
  static void mapping(IO &io, AddressSpaceConfig &config) { io.mapRequired("address_spaces", config.address_spaces); }
};
} // namespace yaml
} // namespace llvm

// Global storage for address space info loaded from YAML
static std::vector<AddressSpaceDesc> LoadedAddrspaces;

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input bitcode file>"), cl::value_desc("filename"),
                                          cl::init(""));

static cl::opt<std::string> OutputFilename(cl::Positional, cl::desc("<output header file>"), cl::value_desc("header"),
                                           cl::init(""));

static cl::opt<std::string> YamlPath("yaml-path", cl::desc("Path to address space descriptor YAML file"),
                                     cl::value_desc("yaml"), cl::init(""));

static cl::opt<std::string> GenDescPath("gen-desc",
                                        cl::desc("Generate address space descriptor header at specified path"),
                                        cl::value_desc("path"), cl::init(""));

static cl::opt<bool> DoMangle("mangle-names", cl::desc("Mangles all strings"), cl::init(false));

enum class FuncScope { PRIVATE, PUBLIC };

static cl::opt<FuncScope> AutogenScope("scope", cl::desc("Which functions to process"),
                                       cl::values(clEnumValN(FuncScope::PRIVATE, "private", "private header"),
                                                  clEnumValN(FuncScope::PUBLIC, "public", "public header")),
                                       cl::init(FuncScope::PRIVATE));

using HoleMap = MapVector<const Type *, std::string>;
using AlignMap = DenseMap<const Type *, uint32_t>;

// TODO: check for cycles if we want to process cyclic structures.
void findHoleTys(const Type *Ty, HoleMap &Tys) {
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    if (!StructTy->isLiteral()) {
      assert(StructTy->hasName() && "no name?");
      StringRef Name = StructTy->getName();
      if (Name.contains("TypeHole")) {
        // Strip off namespaces
        size_t Idx = Name.find_last_of(':');
        StringRef SanitizedName = (Idx == StringRef::npos) ? Name : Name.substr(Idx + 1);
        Tys.insert(std::make_pair(Ty, SanitizedName.str()));
      }
    }

    for (auto *EltTy : StructTy->elements())
      findHoleTys(EltTy, Tys);
  } else if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    findHoleTys(ArrayTy->getElementType(), Tys);
  } else if (auto *VecTy = dyn_cast<VectorType>(Ty)) {
    findHoleTys(VecTy->getElementType(), Tys);
  } else if (auto *PTy = dyn_cast<PointerType>(Ty)) {
    if (!IGCLLVM::isPointerTy(PTy))
      findHoleTys(IGCLLVM::getNonOpaquePtrEltTy(PTy), Tys);
  }
}

std::string sanitize(const std::string &Name) {
  if (Name.empty())
    return "";

  // Soft-mangle the name to avoid collisions with keywords sneaky macros
  std::string result = "_igc_";

  std::string valid = "abcdefghijklmnopqrstuvwxyz"
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                      "_0123456789";
  std::transform(Name.begin(), Name.end(), std::back_inserter(result),
                 [&](const char c) { return (std::string::npos == valid.find(c)) ? '_' : c; });
  return result;
}

template <typename Range, typename Fn> std::string makeList(Range &&R, Fn F, StringRef Sep) {
  std::vector<std::string> Items;
  llvm::transform(R, std::back_inserter(Items), F);
  return llvm::join(Items, Sep);
}

void emitType(const Type *Ty, const HoleMap &HoleTys, const AlignMap &Aligns, raw_ostream &OS, uint32_t Level,
              bool Root) {
  if (auto *StructTy = dyn_cast<StructType>(Ty)) {
    assert(!StructTy->isOpaque() && "encountered opaque struct?");

    if (!Root) {
      OS.indent(Level * 2) << sanitize(StructTy->getName().str()) << "(M";
      HoleMap HoleTys;
      findHoleTys(StructTy, HoleTys);
      if (!HoleTys.empty())
        OS << ", ";
      OS << makeList(HoleTys, [](const auto &P) { return P.second; }, ", ");
      OS << ")";
      return;
    }

    uint32_t In = Level + 1;
    {
      auto HI = HoleTys.find(Ty);
      if (HI != HoleTys.end()) {
        auto AI = Aligns.find(StructTy);
        if (AI != Aligns.end()) {
          // Assertion tests if given type obeys the alignment requirement.
          OS.indent(Level * 2) << "[&] {\n";
          OS.indent(In * 2) << "(void)M;\n";
          OS.indent(In * 2) << "IGC_ASSERT_MESSAGE(Derived::checkAlign(M, cast<StructType>(" << HI->second << "), "
                            << AI->second << "), \"type not aligned!\");\n";
          OS.indent(In * 2) << "return " << HI->second << ";\n";
          OS.indent(Level * 2) << "}()";
        } else {
          OS.indent(Level * 2) << HI->second;
        }
        return;
      }
    }

    auto mangleName = [](const std::string &S) {
      std::string Quoted = "\"" + S + "\"";
      return Quoted;
    };

    OS.indent(Level * 2) << "[&] {\n";
    OS.indent(In * 2) << "StringRef StructName = " << mangleName(StructTy->getName().str()) << ";\n";
    OS.indent(In * 2) << "if (auto *Ty = IGCLLVM::getTypeByName(&M, StructName))\n";
    OS.indent((In + 1) * 2) << "return Ty;\n";
    OS.indent(In * 2) << "Type* Tys[] = {\n";
    for (auto *EltTy : StructTy->elements()) {
      emitType(EltTy, HoleTys, Aligns, OS, In + 1, false);
      OS << ",\n";
    }
    OS.indent(In * 2) << "};\n";
    {
      // Inject explicit padding as needed
      auto I = Aligns.find(StructTy);
      if (I != Aligns.end()) {
        OS.indent(In * 2) << "injectPadding(M, Tys, " << I->second << ", " << (StructTy->isPacked() ? "true" : "false")
                          << ");\n";
      }
    }
    if (StructTy->isLiteral()) {
      OS.indent(In * 2) << "return StructType::get(M.getContext(), Tys, " << (StructTy->isPacked() ? "true" : "false")
                        << ");\n";
    } else {
      OS.indent(In * 2) << "return StructType::create(M.getContext(), Tys, StructName, "
                        << (StructTy->isPacked() ? "true" : "false") << ");\n";
    }
    OS.indent(Level * 2) << "}()";
  } else if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
    OS.indent(Level * 2) << "[&] {\n";
    uint32_t In = Level + 1;
    OS.indent(In * 2) << "auto *EltTy =\n";
    emitType(ArrayTy->getElementType(), HoleTys, Aligns, OS, In + 1, false);
    OS << ";\n";
    OS.indent(In * 2) << "return ArrayType::get(EltTy, " << ArrayTy->getNumElements() << ");\n";
    OS.indent(Level * 2) << "}()";
  } else if (auto *VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty)) {
    OS.indent(Level * 2) << "[&] {\n";
    uint32_t In = Level + 1;
    OS.indent(In * 2) << "auto *EltTy =\n";
    emitType(VecTy->getElementType(), HoleTys, Aligns, OS, In + 1, false);
    OS << ";\n";
    OS.indent(In * 2) << "return IGCLLVM::FixedVectorType::get(EltTy, " << VecTy->getNumElements() << ");\n";
    OS.indent(Level * 2) << "}()";
  } else if (auto *PTy = dyn_cast<PointerType>(Ty)) {
    if (!IGCLLVM::isPointerTy(PTy)) {
      OS.indent(Level * 2) << "[&] {\n";
      uint32_t In = Level + 1;
      OS.indent(In * 2) << "auto *EltTy =\n";
      emitType(IGCLLVM::getNonOpaquePtrEltTy(PTy), HoleTys, Aligns, OS, In + 1, false);
      OS << ";\n";
      OS.indent(In * 2) << "return PointerType::get(EltTy, " << PTy->getAddressSpace() << ");\n";
      OS.indent(Level * 2) << "}()";
    } else {
      OS.indent(Level * 2) << "PointerType::get(M.getContext(), " << PTy->getAddressSpace() << ")";
    }
  } else if (auto *IntTy = dyn_cast<IntegerType>(Ty)) {
    OS.indent(Level * 2) << "IntegerType::get(M.getContext(), " << IntTy->getBitWidth() << ")";
  } else if (Ty->isHalfTy()) {
    OS.indent(Level * 2) << "Type::getHalfTy(M.getContext())";
  } else if (Ty->isFloatTy()) {
    OS.indent(Level * 2) << "Type::getFloatTy(M.getContext())";
  } else if (Ty->isDoubleTy()) {
    OS.indent(Level * 2) << "Type::getDoubleTy(M.getContext())";
  } else {
    assert(0 && "unhandled type!");
    errs() << "Couldn't handle type: ";
    Ty->print(errs());
    OS << "\n";
  }
}

bool processAlignOf(const Function &F, AlignMap &Aligns) {
  if (F.arg_size() != 1) {
    errs() << F.getName() << ": Only one argument should be specified!\n";
    return false;
  }

  const Argument *A = F.arg_begin();
  auto *StructTy = cast<StructType>(A->getParamStructRetType());

  Align Align = *A->getParamAlign();
  Aligns[StructTy] = static_cast<uint32_t>(Align.value());

  return true;
}

bool emitType(const StructType *Ty, StringRef FName, raw_ostream &OS, const AlignMap &Aligns, bool Root) {
  HoleMap HoleTys;
  findHoleTys(Ty, HoleTys);

  OS << "static Type* " << FName << "(Module &M";
  for (auto &P : HoleTys)
    OS << ", Type* " << P.second;

  OS << ")\n";
  OS << "{\n";
  OS << "  return\n";
  emitType(Ty, HoleTys, Aligns, OS, 1, Root);
  OS << ";\n}\n";

  return true;
}

bool processGetType(const Function &F, raw_ostream &OS, const AlignMap &Aligns) {
  if (F.arg_size() != 1) {
    errs() << F.getName() << ": Should have only one argument!\n";
    return false;
  }

  const Argument *A = F.arg_begin();
  auto *STy = cast<StructType>(A->getParamStructRetType());

  return emitType(STy, F.getName(), OS, Aligns, false);
}

struct HookInfo {
  enum class FnType {
    BUILTIN,
    FUNCTION,
  } FnTy;

  std::string FnName;

private:
  HookInfo(FnType Ty, const std::string &FnName) : FnTy(Ty), FnName(FnName) {}

public:
  static std::optional<HookInfo> parse(StringRef Name) {
    std::string DName = demangle(Name.str());
    StringRef S{DName};
    if (!S.contains("hook::"))
      return std::nullopt;

    size_t Loc = S.rfind("hook::");
    if (Loc == StringRef::npos)
      return std::nullopt;

    S = S.substr(Loc);

    Loc = S.find_first_of('(');
    if (Loc == StringRef::npos)
      return std::nullopt;

    S = S.substr(0, Loc);

    auto [_, RHS] = S.split("::");
    auto [TheType, TheName] = RHS.split("::");

    FnType T;
    if (TheType == "bi")
      T = FnType::BUILTIN;
    else if (TheType == "fn")
      T = FnType::FUNCTION;
    else
      return std::nullopt;

    if (TheName.empty())
      return std::nullopt;

    return HookInfo{T, TheName.str()};
  }
};

class SymbolTracker {
  DenseMap<const Value *, std::string> Table;
  uint64_t Count = 0;

  std::string setName(const Value *V, StringRef Name, bool IsPrefix) {
    std::string S;
    raw_string_ostream OS(S);
    OS << Name;
    if (IsPrefix)
      OS << Count++;
    OS.flush();
    assert(Table.count(V) == 0 && "already existing?");
    Table[V] = S;
    return S;
  }

  std::string setName(const Value *V, StringRef Prefix) {
    return V->hasName() ? setName(V, sanitize(V->getName().str()), false) : setName(V, Prefix, true);
  }

public:
  SymbolTracker() = default;
  std::string getName(const Value *V) const {
    auto I = Table.find(V);
    assert(I != Table.end() && "missing?");
    return I->second;
  }

  std::string addInst(const Instruction *I) { return setName(I, "V_"); }

  std::string addArg(const Argument *Arg) { return setName(Arg, "arg_"); }

  std::string addBB(const BasicBlock *BB) { return setName(BB, "BB_"); }
};

using AddrspaceToValueMap = DenseMap<uint32_t, const Value *>;

class TypeRepr {
  const AddrspaceToValueMap &AddrMap;
  const SymbolTracker &SymTracker;
  const SmallDenseSet<uint32_t> &ReservedAddrspaces;

public:
  TypeRepr(const AddrspaceToValueMap &AddrMap, const SymbolTracker &SymTracker,
           const SmallDenseSet<uint32_t> &ReservedAddrspaces)
      : AddrMap(AddrMap), SymTracker(SymTracker), ReservedAddrspaces(ReservedAddrspaces) {}

  std::string getTypeRepr(const Type *Ty) const {
    std::string S;
    raw_string_ostream OS(S);
    if (auto *ArrayTy = dyn_cast<ArrayType>(Ty)) {
      OS << "ArrayType::get(" << getTypeRepr(ArrayTy->getElementType());
      OS << ", " << ArrayTy->getNumElements() << ")";
      return OS.str();
    } else if (auto *VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty)) {
      OS << "IGCLLVM::FixedVectorType::get(" << getTypeRepr(VecTy->getElementType());
      OS << ", " << VecTy->getNumElements() << ")";
      return OS.str();
    } else if (auto *PTy = dyn_cast<PointerType>(Ty)) {
      OS << "PointerType::get(";
      if (!IGCLLVM::isPointerTy(PTy))
        OS << getTypeRepr(IGCLLVM::getNonOpaquePtrEltTy(PTy));
      else
        OS << "*derived().getCtx().getLLVMContext()";
      OS << ", ";
      uint32_t Addrspace = PTy->getPointerAddressSpace();
      if (ReservedAddrspaces.count(Addrspace) != 0) {
        // If this is reserved, we better have a replacement for it.
        // Otherwise, we would get a weird miscompile.
        assert((AddrMap.count(Addrspace) != 0) && "missing addrspace?");
      }
      if (auto I = AddrMap.find(Addrspace); I != AddrMap.end()) {
        OS << SymTracker.getName(I->second) << "->getType()->getPointerAddressSpace()";
      } else {
        OS << Addrspace;
      }
      OS << ")";
      return OS.str();
    } else if (auto *StructTy = dyn_cast<StructType>(Ty)) {
      assert(!StructTy->isOpaque() && "encountered opaque struct?");
      assert(StructTy->hasName() && "anonymous or literal?");
      OS << sanitize(StructTy->getName().str()) << "(*derived().getCtx().getModule())";
      return OS.str();
    } else if (auto *IntTy = dyn_cast<IntegerType>(Ty)) {
      OS << "derived().getInt" << IntTy->getBitWidth() << "Ty()";
      return OS.str();
    } else if (Ty->isHalfTy()) {
      return "derived().getHalfTy()";
    } else if (Ty->isFloatTy()) {
      return "derived().getFloatTy()";
    } else if (Ty->isDoubleTy()) {
      return "derived().getDoubleTy()";
    } else {
      errs() << "Couldn't handle type: ";
      Ty->print(errs());
      errs() << "\n";
      assert(0 && "unhandled type!");
    }
    return "<badty>";
  }
};

class ValueRepr {
  const TypeRepr &TyRepr;
  SymbolTracker &SymTracker;

public:
  ValueRepr(const TypeRepr &TyRepr, SymbolTracker &SymTracker) : TyRepr(TyRepr), SymTracker(SymTracker) {}

  std::string getValueRepr(const Value *V) {
    if (isa<Argument>(V) || isa<Instruction>(V) || isa<BasicBlock>(V)) {
      return SymTracker.getName(V);
    } else if (isa<Constant>(V)) {
      std::string S;
      raw_string_ostream OS(S);
      if (auto *CI = dyn_cast<ConstantInt>(V)) {
        uint64_t Val = CI->getZExtValue();
        uint32_t BitWidth = cast<IntegerType>(CI->getType())->getBitWidth();
        // large integers will get warnings such as:
        // "integer literal is too large to be represented in a signed integer type"
        const char *Suffix = (Val > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) ? "u" : "";
        if (BitWidth == 1)
          OS << (Val ? "derived().getTrue()" : "derived().getFalse()");
        else
          OS << "derived().getInt" << BitWidth << "(" << Val << Suffix << ")";
      } else if (auto *FP = dyn_cast<ConstantFP>(V)) {
        uint64_t Val = FP->getValueAPF().bitcastToAPInt().getZExtValue();

        auto emit = [&](StringRef Kind, uint32_t NumBits) {
          OS << "llvm::ConstantFP::get(*derived().getCtx().getLLVMContext(), llvm::APFloat(llvm::APFloat::IEEE" << Kind
             << "(), llvm::APInt(" << NumBits << ", 0x";
          OS.write_hex(Val) << ")))";
        };

        if (FP->getType()->isHalfTy()) {
          emit("half", 16);
        } else if (FP->getType()->isFloatTy()) {
          emit("single", 32);
        } else if (FP->getType()->isDoubleTy()) {
          emit("double", 64);
        } else {
          assert(0 && "unhandled type!");
        }
      } else if (auto *CV = dyn_cast<ConstantVector>(V)) {
        OS << "llvm::ConstantVector::get({ ";
        OS << makeList(CV->operands(), [&](const Use &U) { return getValueRepr(U.get()); }, ", ");
        OS << " })";
      } else if (auto *CDV = dyn_cast<ConstantDataVector>(V)) {
        SmallVector<Constant *, 4> Constants;
        for (uint32_t i = 0; i < CDV->getNumElements(); i++)
          Constants.push_back(CDV->getElementAsConstant(i));
        OS << "llvm::ConstantVector::get({ ";
        OS << makeList(Constants, [&](const Constant *C) { return getValueRepr(C); }, ", ");
        OS << " })";
      } else if (auto *U = dyn_cast<UndefValue>(V)) {
        OS << "llvm::UndefValue::get(";
        OS << TyRepr.getTypeRepr(U->getType());
        OS << ")";
      } else if (auto *NP = dyn_cast<ConstantPointerNull>(V)) {
        OS << "llvm::Constant::getNullValue(";
        OS << TyRepr.getTypeRepr(NP->getType());
        OS << ")";
      } else {
        assert(0 && "unhandled!");
      }
      return OS.str();
    } else {
      assert(0 && "unhandled!");
    }
    return "<badref>";
  }
};

struct FuncAnnotation {
  enum class Action {
    NONE,
    CREATE,
    ALIGNOF,
    TYPEOF,
  } Act = Action::NONE;

  FuncScope Scope = FuncScope::PRIVATE;
};

using AnnotationMap = DenseMap<Function *, FuncAnnotation>;

AnnotationMap parseAnnotations(const Module &M) {
  AnnotationMap Map;

  SmallDenseSet<StringRef> AnnotFilter{"create", "private", "public", "align-of", "type-of"};
  for (auto &GV : M.globals()) {
    if (GV.getName() == "llvm.global.annotations") {
      auto *CA = cast<ConstantArray>(GV.getInitializer());
      for (Value *Op : CA->operands()) {
        auto *CS = cast<ConstantStruct>(Op);
        // The first field of the struct contains a pointer to annotated variable
        auto *Func = cast<Function>(CS->getOperand(0)->stripPointerCasts());

        GlobalVariable *CurGV = cast<GlobalVariable>(CS->getOperand(1)->stripPointerCasts());
        StringRef AnnotationString = cast<ConstantDataArray>(CurGV->getInitializer())->getAsCString();

        if (AnnotFilter.count(AnnotationString) == 0) {
          errs() << "** WARNING **: unknown annotation '" << AnnotationString << "'\n";
          continue;
        }

        auto &Annot = Map[Func];

        if (AnnotationString == "create")
          Annot.Act = FuncAnnotation::Action::CREATE;
        else if (AnnotationString == "align-of")
          Annot.Act = FuncAnnotation::Action::ALIGNOF;
        else if (AnnotationString == "type-of")
          Annot.Act = FuncAnnotation::Action::TYPEOF;

        if (AnnotationString == "public")
          Annot.Scope = FuncScope::PUBLIC;
        else if (AnnotationString == "private")
          Annot.Scope = FuncScope::PRIVATE;
      }
    }
  }

  return Map;
}

bool processCreate(const Function &F, raw_ostream &OS, const AnnotationMap &Annotations) {
  SymbolTracker SymTracker;
  AddrspaceToValueMap AddrMap;
  SmallDenseSet<uint32_t> ReservedAddrspaces;
  for (auto &ASInfo : LoadedAddrspaces)
    ReservedAddrspaces.insert(ASInfo.AS);
  for (auto &Arg : F.args()) {
    SymTracker.addArg(&Arg);
    if (auto *PTy = dyn_cast<PointerType>(Arg.getType())) {
      uint32_t Addrspace = PTy->getPointerAddressSpace();
      if (ReservedAddrspaces.count(Addrspace) != 0)
        AddrMap.insert({Addrspace, &Arg});
    }
  }
  DominatorTree DT{const_cast<Function &>(F)};
  for (auto *DomNode : depth_first(&DT)) {
    auto &BB = *DomNode->getBlock();
    for (auto &I : BB) {
      auto *CI = dyn_cast<CallInst>(&I);
      if (!CI)
        continue;
      auto *PTy = dyn_cast<PointerType>(I.getType());
      if (!PTy)
        continue;
      uint32_t Addrspace = PTy->getPointerAddressSpace();
      if (ReservedAddrspaces.count(Addrspace) == 0)
        continue;
      if (auto *F = CI->getCalledFunction()) {
        if (HookInfo::parse(F->getName()))
          AddrMap.insert({Addrspace, CI});
      }
    }
  }

  TypeRepr TyRepr{AddrMap, SymTracker, ReservedAddrspaces};
  ValueRepr ValRepr{TyRepr, SymTracker};

  const ReturnInst *UniqueReturn = nullptr;
  const Instruction *ReturnVal = nullptr;
  for (auto &BB : F) {
    if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator())) {
      assert(!UniqueReturn && "should only have one!");
      UniqueReturn = RI;
      if (auto *I = dyn_cast_or_null<Instruction>(RI->getReturnValue())) {
        // Calls may not take another argument
        if (!isa<CallInst>(I))
          ReturnVal = I;
      }
    }
  }

  auto repr = [&](const Value *V) { return ValRepr.getValueRepr(V); };

  auto reprTy = [&](const Type *Ty) { return TyRepr.getTypeRepr(Ty); };

  auto addVar = [&](const Instruction *V) {
    if (!V->use_empty())
      OS << "auto* " << SymTracker.addInst(V) << " = ";

    return V->hasName() ? V->getName().str() : "";
  };

  auto unknownInst = [](const Instruction &I) {
    errs() << "unhandled instruction: ";
    I.print(errs());
    errs() << "\n";
  };

  auto valueName = [](const std::string &S, bool comma = true) -> std::string {
    if (S.empty())
      return "";

    std::string Tmp;
    raw_string_ostream OS(Tmp);
    if (comma)
      OS << ", ";
    if (S == "_ReturnName")
      OS << "_ReturnName";
    else
      OS << "VALUE_NAME(\"" << S << "\")";
    return OS.str();
  };

  constexpr uint32_t Level = 1;

  {
    std::map<std::string, std::string> Funcs;
    uint32_t Cnt = 0;
    for (auto &I : instructions(F)) {
      if (isa<IntrinsicInst>(&I))
        continue;

      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (auto *CalledFn = CI->getCalledFunction()) {
          auto HI = HookInfo::parse(CalledFn->getName());
          if (!HI) {
            errs() << "Couldn't handle function: " << CalledFn->getName() << '\n';
            assert(0 && "failed to parse?");
            return false;
          }

          if (Funcs.count(HI->FnName) != 0)
            continue;

          if (HI->FnTy == HookInfo::FnType::FUNCTION)
            Funcs[HI->FnName] = "FnType" + std::to_string(Cnt++);
        }
      }
    }

    if (!Funcs.empty()) {
      OS << "template <";
      OS << makeList(Funcs, [&](const auto &P) { return "typename " + P.second; }, ", ");
      OS << ">\n";
    }
    OS << (F.getReturnType()->isVoidTy() ? "void" : "auto*");
    OS << " " << F.getName() << "(";
    OS << makeList(F.args(), [&](const Argument &A) { return "Value* " + repr(&A); }, ", ");
    if (!Funcs.empty()) {
      OS << ", ";
      OS << makeList(Funcs, [&](const auto &P) { return P.second + " " + P.first; }, ", ");
    }
  }

  if (ReturnVal) {
    if (!F.arg_empty())
      OS << ", ";
    OS << "const Twine& _ReturnName = \"\"";
  }
  OS << ")\n";
  OS << "{\n";

  for (auto &Arg : F.args()) {
    if (Arg.use_empty())
      OS.indent(Level * 2) << "(void)" << repr(&Arg) << ";\n";
  }

  // If there is control flow, we need to generate the extra blocks.
  const bool HasMultiBB = (F.size() > 1);
  if (HasMultiBB) {
    auto addBB = [&](const BasicBlock &BB) {
      OS << "auto* " << SymTracker.addBB(&BB) << " = ";
      return BB.hasName() ? BB.getName().str() : "";
    };

    OS.indent(Level * 2);
    addBB(F.getEntryBlock());
    OS << "derived().GetInsertBlock();\n";

    OS.indent(Level * 2) << "auto* _CurIP = &*derived().GetInsertPoint();\n";

    OS.indent(Level * 2) << "auto *_JoinBB = " << repr(&F.getEntryBlock()) << "->splitBasicBlock(_CurIP";
    OS << valueName(F.getName().str() + ".join") << ");\n";

    OS.indent(Level * 2) << repr(&F.getEntryBlock()) << "->getTerminator()->eraseFromParent();\n";

    for (auto &BB : drop_begin(F, 1)) {
      OS.indent(Level * 2);
      std::string BBName = addBB(BB);
      std::string ValName = valueName(F.getName().str() + "." + BBName, false);
      OS << "BasicBlock::Create(*derived().getCtx().getLLVMContext(), " << ValName << ", "
         << "_JoinBB->getParent(), _JoinBB);\n";
    }
  }

  for (auto *DomNode : depth_first(&DT)) {
    auto &BB = *DomNode->getBlock();
    if (HasMultiBB) {
      OS.indent(Level * 2) << "derived().SetInsertPoint(" << repr(&BB) << ");\n";
    }
    for (auto &I : BB) {
      OS.indent(Level * 2);
      std::string VarName = addVar(&I);
      if (&I == ReturnVal)
        VarName = "_ReturnName";
      auto emitGenericInst = [&](StringRef Name, const Instruction *I, const Type *Ty = nullptr,
                                 const std::string &ExtraArg = {}) {
        OS << "derived()." << Name << "(";
        if (Ty)
          OS << reprTy(Ty) << ", ";
        OS << makeList(I->operands(), [&](const Use &U) { return repr(U.get()); }, ", ");
        OS << ExtraArg;
        OS << valueName(VarName, I->getNumOperands() != 0);
        OS << ")";
      };

      if (auto *GEPI = dyn_cast<GetElementPtrInst>(&I)) {
        StringRef FnName = GEPI->isInBounds() ? "CreateInBoundsGEP" : "CreateGEP";
        OS << "derived()." << FnName << "(";
        OS << reprTy(GEPI->getSourceElementType()) << ", ";
        OS << repr(GEPI->getPointerOperand()) << ", ";
        SmallVector<Value *, 4> Indices(GEPI->idx_begin(), GEPI->idx_end());
        if (Indices.size() > 1)
          OS << "{ ";
        OS << makeList(Indices, [&](const Value *V) { return repr(V); }, ", ");
        if (Indices.size() > 1)
          OS << " }";
        OS << valueName(VarName) << ")";
      } else if (auto *BCI = dyn_cast<BitCastInst>(&I)) {
        OS << "derived().CreateBitCast(" << repr(BCI->getOperand(0)) << ", ";
        OS << reprTy(BCI->getDestTy()) << valueName(VarName) << ")";
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
        emitGenericInst("CreateStore", SI);
      } else if (auto *LI = dyn_cast<LoadInst>(&I)) {
        emitGenericInst("CreateLoad", LI, LI->getType());
        if (LI->getMetadata(LLVMContext::MD_invariant_load)) {
          OS << ";\n";
          OS.indent(Level * 2) << "derived().setInvariantLoad(" << repr(LI) << ")";
        }
        if (LI->isVolatile()) {
          OS << ";\n";
          OS.indent(Level * 2) << repr(LI) << "->setVolatile(true)";
        }
      } else if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
        StringRef FName;
        switch (BO->getOpcode()) {
#define HANDLE_BINARY_INST(N, OPC, CLASS)                                                                              \
  case Instruction::OPC:                                                                                               \
    FName = "Create" #OPC;                                                                                             \
    break;
#include "llvm/IR/Instruction.def"
#undef HANDLE_BINARY_INST
        default:
          unknownInst(I);
          return false;
        }

        emitGenericInst(FName, BO);
      } else if (auto *UO = dyn_cast<UnaryOperator>(&I)) {
        StringRef FName;
        switch (UO->getOpcode()) {
#define HANDLE_UNARY_INST(N, OPC, CLASS)                                                                               \
  case Instruction::OPC:                                                                                               \
    FName = "Create" #OPC;                                                                                             \
    break;
#include "llvm/IR/Instruction.def"
#undef HANDLE_UNARY_INST
        default:
          unknownInst(I);
          return false;
        }

        emitGenericInst(FName, UO);
      } else if (auto *CastI = dyn_cast<CastInst>(&I)) {
        // We really shouldn't be seeing addrspacecast. We can support
        // it if needed, but best to explicitly block it for now.
        if (CastI->getOpcode() == Instruction::AddrSpaceCast) {
          unknownInst(I);
          return false;
        }

        StringRef FName;
        switch (CastI->getOpcode()) {
#define HANDLE_CAST_INST(N, OPC, CLASS)                                                                                \
  case Instruction::OPC:                                                                                               \
    FName = "Create" #OPC;                                                                                             \
    break;
#include "llvm/IR/Instruction.def"
#undef HANDLE_CAST_INST
        default:
          unknownInst(I);
          return false;
        }

        OS << "derived()." << FName << "(" << repr(CastI->getOperand(0)) << ", ";
        OS << reprTy(CastI->getDestTy()) << valueName(VarName) << ")";
      } else if (auto *SelInst = dyn_cast<SelectInst>(&I)) {
        emitGenericInst("CreateSelect", SelInst);
      } else if (auto *EEI = dyn_cast<ExtractElementInst>(&I)) {
        emitGenericInst("CreateExtractElement", EEI);
      } else if (auto *IEI = dyn_cast<InsertElementInst>(&I)) {
        emitGenericInst("CreateInsertElement", IEI);
      } else if (auto *SVI = dyn_cast<ShuffleVectorInst>(&I)) {
        std::string maskArg;
        // In recent llvm versions, the mask in this instruction is not
        // the operand, but it is needed to create the instruction.
        maskArg = ", " + repr(IGCLLVM::getShuffleMaskForBitcode(SVI));
        emitGenericInst("CreateShuffleVector", SVI, nullptr, maskArg);
      } else if (auto *CompareInst = dyn_cast<CmpInst>(&I)) {
        std::string FName;
        if (CompareInst->isIntPredicate()) {
          FName = "CreateICmp";
        } else if (CompareInst->isFPPredicate()) {
          FName = "CreateFCmp";
        } else {
          assert(0 && "unknown cmp?");
        }
        FName += CompareInst->getPredicateName(CompareInst->getPredicate()).upper();
        emitGenericInst(FName, CompareInst);
      } else if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
        FunctionType *FTy = II->getCalledFunction()->getFunctionType();
        // Accumulate an array of overloaded types for the given intrinsic
        SmallVector<Type *, 4> ArgTys;
        {
          SmallVector<Intrinsic::IITDescriptor, 8> Table;
          getIntrinsicInfoTableEntries(II->getIntrinsicID(), Table);
          ArrayRef<Intrinsic::IITDescriptor> TableRef = Table;

          if (Intrinsic::matchIntrinsicSignature(FTy, TableRef, ArgTys)) {
            assert(0 && "unhandled?");
          }

          StringRef Name = Intrinsic::getBaseName(II->getIntrinsicID());
          auto [__, RHS] = StringRef{Name}.split('.');

          SmallVector<StringRef, 2> parts;
          RHS.split(parts, '.');

          OS << "derived().CreateIntrinsic(Intrinsic::";
          auto part = parts.begin();
          while (true) {
            OS << *part;
            part++;
            if (part == parts.end())
              break;
            OS << '_';
          }

          OS << ", { ";
          OS << makeList(ArgTys, [&](const Type *Ty) { return reprTy(Ty); }, ", ");
          OS << " }, { ";
          OS << makeList(II->args(), [&](const Use &U) { return repr(U.get()); }, ", ");
          OS << " }, nullptr" << valueName(VarName) << ")";
        }
      } else if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (auto *CalledFn = CI->getCalledFunction()) {
          auto HI = HookInfo::parse(CalledFn->getName());
          if (!HI) {
            errs() << "Couldn't handle function: " << CalledFn->getName() << '\n';
            assert(0 && "failed to parse?");
            return false;
          }
          // BUILTIN hooks are methods on the derived builder class
          // FUNCTION hooks are template parameters passed in as arguments
          if (HI->FnTy == HookInfo::FnType::BUILTIN)
            OS << "derived()." << HI->FnName << "(";
          else
            OS << HI->FnName << "(";
          OS << makeList(CI->args(), [&](const Use &U) { return repr(U.get()); }, ", ");
          OS << ")";
        } else {
          unknownInst(I);
          return false;
        }
      } else if (auto *PN = dyn_cast<PHINode>(&I)) {
        OS << "derived().CreatePHI(" << reprTy(PN->getType()) << ", " << PN->getNumIncomingValues()
           << valueName(VarName) << ")";
      } else if (auto *RI = dyn_cast<ReturnInst>(&I)) {
        if (HasMultiBB) {
          OS << "derived().CreateBr(_JoinBB)";
        } else {
          if (auto *Val = RI->getReturnValue())
            OS << "return " << repr(Val);
          else
            OS << "return";
        }
      } else if (auto *BI = dyn_cast<BranchInst>(&I)) {
        // branch argument ordering is unusual, do this manually.
        if (BI->isConditional()) {
          OS << "derived().CreateCondBr(" << repr(BI->getCondition()) << ", " << repr(BI->getSuccessor(0)) << ", "
             << repr(BI->getSuccessor(1)) << ")";
        } else {
          OS << "derived().CreateBr(" << repr(BI->getSuccessor(0)) << ")";
        }
      } else {
        unknownInst(I);
        return false;
      }
      OS << ";\n";
    }
  }

  // Patch up phi nodes
  for (auto &BB : F) {
    for (auto &PN : BB.phis()) {
      for (uint32_t i = 0; i < PN.getNumIncomingValues(); i++) {
        auto *InBB = PN.getIncomingBlock(i);
        auto *InVal = PN.getIncomingValue(i);
        OS.indent(Level * 2) << repr(&PN) << "->addIncoming(" << repr(InVal) << ", " << repr(InBB) << ");\n";
      }
    }
  }

  if (HasMultiBB) {
    OS.indent(Level * 2) << "derived().SetInsertPoint(_CurIP);\n";
    if (auto *RetVal = UniqueReturn->getReturnValue())
      OS.indent(Level * 2) << "return " << repr(RetVal) << ";\n";
  }

  OS << "}\n";

  return true;
}

void addPreamble(raw_ostream &OS) {
  const char *Notice =
      R""""(
///////////////////////////////////////
// ****This file is autogenerated****
///////////////////////////////////////

)"""";

  OS << Notice;
}

bool process(const Module &M, raw_ostream &OS) {
  addPreamble(OS);

  AlignMap Aligns;

  auto Annotations = parseAnnotations(M);

  bool Success = true;
  for (auto &F : M) {
    auto I = Annotations.find(&F);
    if (I == Annotations.end())
      continue;
    if (I->second.Act == FuncAnnotation::Action::ALIGNOF)
      Success &= processAlignOf(F, Aligns);
  }

  if (AutogenScope == FuncScope::PRIVATE) {
    TypeFinder Finder;
    Finder.run(M, true);

    for (auto *StructTy : Finder) {
      Success &= emitType(StructTy, sanitize(StructTy->getName().str()), OS, Aligns, true);
    }
  }

  for (auto &F : M) {
    auto I = Annotations.find(&F);
    if (I == Annotations.end())
      continue;

    if (AutogenScope != I->second.Scope)
      continue;

    switch (I->second.Act) {
    case FuncAnnotation::Action::TYPEOF:
      Success &= processGetType(F, OS, Aligns);
      break;
    case FuncAnnotation::Action::CREATE:
      Success &= processCreate(F, OS, Annotations);
      break;
    default:
      continue;
    }
    OS << "\n";
  }
  return Success;
}

void markInvariant(Module &M) {
  SmallDenseSet<uint32_t> ConstantAS;
  for (auto &ASInfo : LoadedAddrspaces) {
    if (ASInfo.constant)
      ConstantAS.insert(ASInfo.AS);
  }

  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    for (auto &I : instructions(F)) {
      auto *LI = dyn_cast<LoadInst>(&I);
      if (!LI || ConstantAS.count(LI->getPointerAddressSpace()) == 0)
        continue;

      auto *EmptyNode = MDNode::get(LI->getContext(), nullptr);
      LI->setMetadata(LLVMContext::MD_invariant_load, EmptyNode);
    }
  }
}

void sanitizeInsts(Module &M) {
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    for (auto &I : make_early_inc_range(instructions(F))) {
      if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
        auto ID = II->getIntrinsicID();
        if (ID == Intrinsic::experimental_noalias_scope_decl)
          II->eraseFromParent();
      }
    }
  }
}

void rewriteAnonTypes(Module &M) {
  // We need to rewrite types that have "anon" in the name to types that
  // have a name that won't collide with types already present in the module.
  //
  // Here, we iterate over all the struct types in the module and rewrite
  // those with "anon" to have an "igc.auto." prefix.
  for (auto &STy : M.getIdentifiedStructTypes()) {
    if (STy->getName().contains("anon")) {
      std::string NewName = "igc.auto." + STy->getName().str();
      STy->setName(NewName);
    }
  }
}

void preprocess(Module &M) {
  llvm::legacy::PassManager mpm;
  mpm.add(createUnifyFunctionExitNodesPass());
  mpm.add(createLowerSwitchPass());
  mpm.run(M);

  markInvariant(M);
  sanitizeInsts(M);
  rewriteAnonTypes(M);
}

bool loadAddressSpacesFromYAML(const std::string &YamlPath) {
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr = MemoryBuffer::getFile(YamlPath);
  if (std::error_code EC = FileOrErr.getError()) {
    errs() << "Error opening YAML file '" << YamlPath << "': " << EC.message() << "\n";
    return false;
  }

  yaml::Input YamlInput(FileOrErr.get()->getBuffer());
  AddressSpaceConfig Config;
  YamlInput >> Config;

  if (YamlInput.error()) {
    errs() << "Error parsing YAML file '" << YamlPath << "'\n";
    return false;
  }

  LoadedAddrspaces.clear();
  for (size_t i = 0; i < Config.address_spaces.size(); i++) {
    auto &desc = Config.address_spaces[i];
    desc.AS = FIRST_ADDRSPACE + static_cast<uint32_t>(i);
    LoadedAddrspaces.push_back(desc);
  }

  return true;
}

bool generateDescriptorHeader(const std::string &YamlPath, const std::string &OutputPath) {
  // Load the YAML configuration
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr = MemoryBuffer::getFile(YamlPath);
  if (std::error_code EC = FileOrErr.getError()) {
    errs() << "Error opening YAML file '" << YamlPath << "': " << EC.message() << "\n";
    return false;
  }

  yaml::Input YamlInput(FileOrErr.get()->getBuffer());
  AddressSpaceConfig Config;
  YamlInput >> Config;

  if (YamlInput.error()) {
    errs() << "Error parsing YAML file '" << YamlPath << "'\n";
    return false;
  }

  // Open output file
  std::error_code EC;
  sys::fs::OpenFlags fsFlags = sys::fs::OF_TextWithCRLF;
  raw_fd_ostream outfile(OutputPath, EC, fsFlags);

  if (EC) {
    errs() << "Couldn't open output file '" << OutputPath << "' for writing: " << EC.message() << "\n";
    return false;
  }

  // Write the header
  outfile << "// AUTOGENERATED FILE - DO NOT EDIT\n";
  outfile << "// Generated from: " << YamlPath << "\n\n";
  outfile << "#pragma once\n\n";
  outfile << "struct AddrspaceInfo {\n";
  outfile << "  unsigned AS;\n";
  outfile << "  bool Constant;\n";
  outfile << "  constexpr AddrspaceInfo(unsigned AS, bool Constant) : AS(AS), Constant(Constant) {}\n";
  outfile << "};\n\n";
  outfile << "constexpr AddrspaceInfo ReservedAS[] = {\n";
  outfile << "    // clang-format off\n";

  for (size_t i = 0; i < Config.address_spaces.size(); i++) {
    auto &desc = Config.address_spaces[i];
    unsigned AS = FIRST_ADDRSPACE + static_cast<unsigned>(i);
    const char *constant = desc.constant ? "true " : "false";
    outfile << "    AddrspaceInfo{ " << AS << ", " << constant << " }, // " << desc.name << ": " << desc.description
            << "\n";
  }

  outfile << "    // clang-format on\n";
  outfile << "};\n\n";

  // Generate address space defines
  outfile << "#if defined(__clang__)\n";
  for (size_t i = 0; i < Config.address_spaces.size(); i++) {
    auto &desc = Config.address_spaces[i];
    outfile << "// " << desc.description << "\n";
    outfile << "#define " << desc.name << " __attribute__((address_space(ReservedAS[" << i << "].AS)))\n";
  }
  outfile << "#else\n";
  for (size_t i = 0; i < Config.address_spaces.size(); i++) {
    auto &desc = Config.address_spaces[i];
    outfile << "#define " << desc.name << "\n";
  }
  outfile << "#endif // __clang__\n";

  return true;
}

int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);

  llvm_shutdown_obj Y;

  const char *Overview =
      R""""(
Generate code for the specified bitcode.

)"""";

  if (!cl::ParseCommandLineOptions(argc, argv, Overview))
    return 1;

  // Handle --gen-desc mode: generate address space descriptor header and exit
  if (!GenDescPath.empty()) {
    if (YamlPath.empty()) {
      errs() << "Error: --gen-desc requires --yaml-path to be specified\n";
      return 1;
    }
    return generateDescriptorHeader(YamlPath, GenDescPath) ? 0 : 1;
  }

  // Validate required arguments for normal mode
  if (InputFilename.empty() || OutputFilename.empty()) {
    errs() << "Error: input bitcode file and output header file are required\n";
    return 1;
  }

  // Load address space configuration from YAML if provided
  if (!YamlPath.empty()) {
    if (!loadAddressSpacesFromYAML(YamlPath)) {
      errs() << "Failed to load address space configuration from: " << YamlPath << "\n";
      return 1;
    }
  }

  LLVMContext Context;
  SMDiagnostic Err;

  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);

  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  std::error_code EC;

  sys::fs::OpenFlags fsFlags = sys::fs::OF_TextWithCRLF;
  raw_fd_ostream outfile(OutputFilename, EC, fsFlags);

  if (EC) {
    errs() << "Couldn't open file for writing!\n";
    return 1;
  }

  preprocess(*M);

  return process(*M, outfile) ? 0 : 1;
}

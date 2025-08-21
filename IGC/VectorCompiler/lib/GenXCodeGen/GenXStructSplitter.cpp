/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXStructSplitter
/// ------------------
/// It is a module pass whose purpose is to split all complicate structs into
/// plain substructs for further optimizations.
/// eg. {vec3f, vec3f, f, vec5i} will become {vec3f, vec3f, f} {vec5i}.
///
/// It does in 2 main steps:
/// 1. Resolves which structs can be split and splits it.
///   a. Collects all structs.
///   b. Creates DependencyGraph (DAG) of struct usage.
///       Contains information about nested structures.
///   c. Splits structures starting from the nodes that have no output.
/// 2. Replaces all structures if it is possible.
///   a. Replaces allocas.
///     I. Generates new allocas.
///     II. Updates debug information.
///   b. Replaces all uses of allocas (GEP and PTI).
///     I. Replace all uses of GEP and PTI.
///
/// Ex. (C-like):
///   struct A = {int, float};
///   A a;
///   int i = a.int;
/// Will become:
///   struct Ai = {int};
///   struct Af = {float};
///   Ai ai;
///   Af af;
///   int i = ai.int;
/// With unwrapping optimization (The unique-type element is
/// extracted from structure):
///   Ai is a int;
///   Af is a float;
///   Ai ai;
///   Af af;
///   int i = ai;
///
/// Limitations:
///   1. Structure contains array of complex structs.
///   2. Structure is allocated as an array.
///   3. Structure contains prohibitted structure.
///   4. Structure using instruction is not GEP, PTI, alloca.
///       Except pattern: alloca->bitcast->lifetime.start/end
///       This unstructions are ignored.
///   5. Users of the PTI not add, insertelement, shufflevector, read/write.
///   6. Pointer of the structure goes in function (except read/write).
///   7. Pointer offset from the beginning of the structure covers different
///      types.
///   8. Pointer offset from the beginning of the structure covers unsequential
///      split structs.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include <optional>

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/General/DebugInfo.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvmWrapper/ADT/Optional.h>
#include <llvmWrapper/Support/Alignment.h>

#include "Probe/Assertion.h"

#include <unordered_map>
#include <unordered_set>

using namespace llvm;

#define DEBUG_TYPE "GENX_STRUCT_SPLITTER"

static cl::opt<bool> PerformStructSplitting(
    "vc-struct-splitting", cl::init(true), cl::Hidden,
    cl::desc(
        "Performs splitting complicate-constucted structs to plain structs."));

namespace {

class GenXStructSplitter : public ModulePass {
public:
  static char ID;

  explicit GenXStructSplitter() : ModulePass(ID) {}
  ~GenXStructSplitter() = default;

  StringRef getPassName() const override { return "GenX struct splitter"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};

} // namespace

namespace llvm {
void initializeGenXStructSplitterPass(PassRegistry &);
}

char GenXStructSplitter::ID = 0;
INITIALIZE_PASS_BEGIN(GenXStructSplitter, "GenXStructSplitter",
                      "GenXStructSplitter", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXStructSplitter, "GenXStructSplitter",
                    "GenXStructSplitter", false, false)

ModulePass *llvm::createGenXStructSplitterPass() {
  initializeGenXStructSplitterPass(*PassRegistry::getPassRegistry());
  return new GenXStructSplitter;
}

void GenXStructSplitter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
  AU.setPreservesAll();
}

static Type *getArrayFreeTy(Type *Ty);
static Type *getBaseTy(Type *Ty);
static const char *getTypePrefix(Type &Ty);

// Class to do first analysis and ban all structures, which cannot be split
// at advance. It bans structures containing array of complex structs. It bans
// structures containing banned structs.
class StructFilter {
  std::unordered_set<StructType *> BannedStructs;

  bool checkForArrayOfComplicatedStructs(StructType &STy) const;
  bool checkForElementOfBannedStruct(StructType &STy) const;

public:
  StructFilter(Module &M);
  bool isStructBanned(StructType &STy) const;

  void print(raw_ostream &os = llvm::errs()) const;
};

// Class to handle all information about Structs which are used in Module.
// It contains initial structs info, performs struct splitting,
// contains transformation of structs and final structs info in Module.
// Example of work of this class is showed with dependencies:
// C : { [5 x A], i, f, [5 x B], D }
// A : { f, [5 x f] }
// B : { i, [5 x i] }
// D : { A, B }
// Where A,B,C,D are structs; f,i - base unsplit types.
class DependencyGraph {
public:
  //***************************************
  // Part responsible for types definition.
  //***************************************

  // Helper structure contains split struct or unwrapped type and position of
  // data. SElement is unwrapped in case of splitting structures with only one
  // element inside. So element is not a split structure, but the type
  // itself, e.g. {int} -> int.
  class SElement {
    Type *Ty{nullptr};
    unsigned Index{0};
    bool IsUnwrapped{false};

  public:
    SElement() = default;
    SElement(StructType *const &InTy, unsigned InIndex);
    SElement(Type *const &InTy);

    StructType *getStructTyIfPossible() const;
    StructType *getStructTy() const;
    Type *getTy() const;
    Type *retrieveElemTy() const;
    unsigned getIndex() const;
    bool isUnwrapped() const;
    void print(raw_ostream &os = llvm::errs()) const;
  };

  // Helper class contains array of Types and Indices where Type is placed.
  // Position of the element in the array defines element's position in the
  // split structure.
  class SElementsOfType {
    std::vector<Type *> Types;
    // vector of Indices corresponding to vector of Types.
    std::vector<unsigned> IndicesOfTypes;
    // vector of values for non-const indices
    std::vector<Value *> Values;

  public:
    SElementsOfType(unsigned Size);
    SElementsOfType(const std::vector<Type *> &InTypes);

    unsigned size() const;

    Type *getTyAt(unsigned Index) const;
    unsigned getIdxAt(unsigned Index) const;
    Value *getValAt(unsigned Index) const;
    std::pair<Type *&, unsigned &> at(unsigned Index);
    std::pair<Type *const &, const unsigned &> at(unsigned Index) const;

    const std::vector<Type *> &getTypesArray() const { return Types; }

    using ty_iterator = std::vector<Type *>::iterator;
    using idx_iterator = std::vector<unsigned>::iterator;
    using val_iterator = std::vector<Value *>::iterator;
    using const_ty_iterator = std::vector<Type *>::const_iterator;
    using const_idx_iterator = std::vector<unsigned>::const_iterator;
    using const_val_iterator = std::vector<Value *>::const_iterator;

    // Iterator for SElementsOfType to be able to iterate over a range of
    // Types and IndicesOfTypes simultaneously.
    using value_type = std::tuple<const_ty_iterator::value_type,
                                  const_idx_iterator::value_type,
                                  const_val_iterator::value_type>;
    struct const_iterator {
      using iterator_category = std::forward_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = SElementsOfType::value_type;
      using pointer = value_type *;
      using reference =
          value_type; // Reference is a value_type as right now I guarantee,
                      // that value pointed by iterator wont be changed, so
                      // copy of pair is not crusual.
      const_iterator() = delete;
      const_iterator(const_ty_iterator TyItIn, const_idx_iterator IdxItIn,
                     const_val_iterator ValItIn);
      reference operator*() const;
      const_iterator &operator++();
      const_iterator operator++(int);
      const_iterator operator+(difference_type RHS) const;

      friend bool operator==(const const_iterator &LHS,
                             const const_iterator &RHS);
      friend bool operator!=(const const_iterator &LHS,
                             const const_iterator &RHS);

    private:
      const_ty_iterator TyIt;
      const_idx_iterator IdxIt;
      const_val_iterator ValIt;
    };

    ty_iterator ty_begin() { return Types.begin(); }
    const_ty_iterator ty_begin() const { return Types.begin(); }
    ty_iterator ty_end() { return Types.end(); }
    const_ty_iterator ty_end() const { return Types.end(); }
    idx_iterator idx_begin() { return IndicesOfTypes.begin(); }
    const_idx_iterator idx_begin() const { return IndicesOfTypes.begin(); }
    idx_iterator idx_end() { return IndicesOfTypes.end(); }
    const_idx_iterator idx_end() const { return IndicesOfTypes.end(); }
    val_iterator val_begin() { return Values.begin(); }
    const_val_iterator val_begin() const { return Values.begin(); }
    val_iterator val_end() { return Values.end(); }
    const_val_iterator val_end() const { return Values.end(); }
    const_iterator begin() const {
      return const_iterator{Types.begin(), IndicesOfTypes.begin(),
                            Values.begin()};
    }
    const_iterator end() const {
      return const_iterator{Types.end(), IndicesOfTypes.end(), Values.end()};
    }
    auto indices() const { return make_range(idx_begin(), idx_end()); }
    auto types() const { return make_range(ty_begin(), ty_end()); }
    auto values() const { return make_range(val_begin(), val_end()); }

    void emplace_back(Type &Ty, unsigned Index, Value *V = nullptr);
    void push_back(const value_type &Elem);
    void emplace_back(const SElement &Elem, Value *V = nullptr);

    void print(raw_ostream &Os = llvm::errs()) const;
  };

  // The SMap is a full collection of Structs in Module within the
  // complete information about types and elements which are used in structure.
  // The STypes is a full information about elements used in Struct.
  // The Idea is to separate all elements by their baseType.
  // SMap looks like:
  // StructType : (BaseTy: <ElementType, IndexOfElement> <,> ...)
  // C : (i: <i, 1> <[5xB], 3>)
  //     (f: <[5xA], 0> <f, 2>)
  //     (D: <D, 4>)
  // A : (f: <f, 0> <[5xf], 1>)
  // B : (i: <i, 0> <[5xi], 1>)
  // D : (f: <A, 0>)
  //     (i: <B, 0>)
  using STypes = std::unordered_map<Type *, SElementsOfType>;
  using SMap = std::unordered_map<StructType *, STypes>;

  // The typedefs bellow are full information about struct's transformation.
  // The idea is to generate map between old struct's elements and new split
  // struct's elements. Index of the element in InitSTy is index of
  // ElemMapping and Index of the element of SplitSTy will be
  // put according the InitSTy's element index. VecOfStructInfo looks like:
  // InitSTy : (SplitSTy, Index) (,) <- at position=0
  //           (,)                   <- at position=1
  // eg. splitting of structs D and C:
  // D : (Df, 0)
  //     (Di, 0)
  // C : (Cf, 0)
  //     (Ci, 0)
  //     (Cf, 1)
  //     (Ci, 1)
  //     (Cf, 2) (Ci, 2)
  // And SMap in that case also contains:
  // Ci : (i: <i, 0> <[5xB], 1> <Di, 2)
  // Cf : (f: <[5xA], 0> <f, 1> <Df, 2)
  // Df : (f: <A, 0>)
  // Di : (i: <B, 0>)
  // List of new elements which are on place of old unsplit struct.
  // Several elements can apply one place if one unsplit element is split to
  // several.
  using ListOfSplitElements = std::list<SElement>;
  // Vector of new structs elements. The position of the element is in
  // accordance with the index of this element in unsplit structure.
  using ElemMapping = std::vector<ListOfSplitElements>;
  // All collection of new elements.
  using InfoAboutSplitStruct = std::pair<StructType *, ElemMapping>;
  // Info about all structs to be split.
  // Vector has been chosen to save the transformation chronology.
  using VecOfStructInfo = std::vector<InfoAboutSplitStruct>;

private:
  LLVMContext &Ctx;

  SMap AllStructs;
  VecOfStructInfo SplitStructs;

  // A helping map for fast access to necessary structure transformation.
  // Uses reverse_iterator, because VecOfStructInfo is processed backwards.
  std::unordered_map<StructType *, VecOfStructInfo::const_reverse_iterator>
      InfoToMerge;

  // Node represents an aggregative StructType with Nodes(another Structs) on
  // which it depends.
  class Node {
    StructType *STy{nullptr};

    // During the Graph transformation intermediate unsplit stucts will be
    // generated.
    // eg: structure C_BS will be generated
    //  C_BS : { [5 x A], i, f, [5 x B], Df, Di }.
    // But C_BS is the same C structure in terms of dependencies,
    // so PreviousNames set contains all previous Node representations.
    std::unordered_set<StructType *> PreviousNames;
    // eg. C has children A, B, D
    // C contains structures A, B, D.
    std::unordered_set<Node *> ChildSTys;
    // eg. A has parents D, C.
    // A is contained by structures D, C.
    std::unordered_set<Node *> ParentSTys;

  public:
    Node(StructType &InSTy) : STy{&InSTy} {}

    bool hasParent() const { return !ParentSTys.empty(); }
    bool hasChild() const { return !ChildSTys.empty(); }
    void insertParent(Node &ParentNode);
    void insertChild(Node &ChildNode);
    void eraseChild(Node &ChildNode);
    bool containsStruct(StructType &InSTy) const;
    void substitute(StructType &InSTy);

    StructType *getType() const { return STy; }

    using iterator = std::unordered_set<Node *>::iterator;
    using const_iterator = std::unordered_set<Node *>::const_iterator;
    iterator parent_begin() { return ParentSTys.begin(); }
    const_iterator parent_begin() const { return ParentSTys.begin(); }
    iterator parent_end() { return ParentSTys.end(); }
    const_iterator parent_end() const { return ParentSTys.end(); }
    auto parents() const { return make_range(parent_begin(), parent_end()); }
    auto parents() { return make_range(parent_begin(), parent_end()); }

    iterator child_begin() { return ChildSTys.begin(); }
    const_iterator child_begin() const { return ChildSTys.begin(); }
    iterator child_end() { return ChildSTys.end(); }
    const_iterator child_end() const { return ChildSTys.end(); }
    auto children() const { return make_range(child_begin(), child_end()); }
    auto children() { return make_range(child_begin(), child_end()); }

    void dump(int tab, raw_ostream &os = llvm::errs()) const;
  };

  // Class responsible for allocating and releasing memory occupied by Nodes.
  class NodeMemoryManager {
    std::vector<std::unique_ptr<Node>> Nodes;

  public:
    NodeMemoryManager(Module &M);
    Node *create(StructType &STy);
  };
  NodeMemoryManager NodeMM;

  //***************************************
  // Part responsible for graph handling.
  //***************************************

  // Heads contains all Nodes that have no parents.
  std::vector<Node *> Heads;
  void generateGraph();

  // Helper type that is used to track Nodes placement in Graph.
  using NodeTracker = std::unordered_map<StructType *, Node *>;
  Node *createNode(StructType &STy, NodeTracker &Inserted);
  void processNode(Node &SNode);
  void remakeParent(Node &SNode, Node &SNodeToChange,
                    ArrayRef<Type *> NewReplaceStructs);
  void recreateGraph();

  //************************************************************
  // Part responsible for recording information about structures
  // and tracking transformation.
  //************************************************************

  void setInfoAboutStructure(StructType &STy);
  void mergeStructGenerationInfo();
  StructType *checkAbilityToMerge(const ElemMapping &NewSTypes) const;

public:
  DependencyGraph(Module &M, const StructFilter &Filter);
  void run();

  //***************************************
  // Part responsible for info accessing.
  //***************************************
  bool isPlain(StructType &STy) const;
  bool isStructProcessed(StructType &STy) const;
  const STypes &getStructComponens(StructType &STy) const;
  Type *getPlainSubTy(StructType &STy) const;
  const ElemMapping &getElemMappingFor(StructType &STy) const;
  const ListOfSplitElements &getElementsListOfSTyAtIdx(StructType &STy,
                                                       unsigned Idx) const;
  std::vector<Type *> getUniqueSplitTypes(StructType &STy) const;
  //***************************************
  // Part responsible for dumping.
  //***************************************
  void printData(raw_ostream &os = llvm::errs()) const;
  void print(raw_ostream &os = llvm::errs()) const;
  void printGraph(raw_ostream &os = llvm::errs()) const;
  void printGeneration(raw_ostream &os) const;
};

// This class handles all instructions that use split structs.
struct Substituter : public InstVisitor<Substituter> {
  // Aliases for types.
  // Contains instructions to be substituted with.
  using InstsToSubstitute =
      std::vector<std::pair<Instruction *, Instruction *>>;
  // Contains map of instructions and base type this instruction operates.
  using TypeToInstrMap = std::unordered_map<Type *, Instruction *>;

  using ElemMapping = DependencyGraph::ElemMapping;
  using ListOfSplitElements = DependencyGraph::ListOfSplitElements;
  using SElementsOfType = DependencyGraph::SElementsOfType;
  using SElement = DependencyGraph::SElement;

private:
  LLVMContext &Ctx;
  const DataLayout &DL;
  StructFilter Filter;
  DependencyGraph Graph;
  vc::DIBuilder DIB;

  std::vector<AllocaInst *> Allocas;

  bool processAlloca(AllocaInst &Alloca);

  void updateDbgInfo(ArrayRef<Type *> TypesToGenerateDI, AllocaInst &AI,
                     AllocaInst &NewAI, DbgDeclareInst &DbgDeclare);
  TypeToInstrMap generateNewAllocas(AllocaInst &OldInst);
  AllocaInst *generateAlloca(AllocaInst &AI,
                             const DependencyGraph::SElement &TyElem);
  void createLifetime(Instruction *OldI, AllocaInst *NewAI);
  Instruction *generateNewGEPs(GetElementPtrInst &GEPI, Type &DestSTy,
                               const DependencyGraph::SElementsOfType &IdxPath,
                               const TypeToInstrMap &NewInstr,
                               unsigned PlainTyIdx) const;

  static std::tuple<DependencyGraph::SElementsOfType, std::vector<Type *>>
  getIndicesPath(GetElementPtrInst &GEPI);
  static std::optional<
      std::tuple<std::vector<GetElementPtrInst *>, std::vector<PtrToIntInst *>>>
  getInstUses(Instruction &I);
  static std::optional<uint64_t> processAddOrInst(Instruction &I,
                                                  BinaryOperator &BO);

  bool processGEP(GetElementPtrInst &GEPI, const TypeToInstrMap &NewInstr,
                  InstsToSubstitute /*OUT*/ &InstToInst);
  bool processPTI(PtrToIntInst &PTI, Type *Ty, const TypeToInstrMap &NewInstr,
                  InstsToSubstitute /*OUT*/ &InstToInst);
  static bool processPTIsUses(Instruction &I, uint64_t /*OUT*/ &MaxPtrOffset);

public:
  Substituter(Module &M);

  void visitAllocaInst(AllocaInst &AI);
  bool processAllocas();

  void printAllAllocas(raw_ostream &Os = llvm::errs());
};

bool GenXStructSplitter::runOnModule(Module &M) {
  const auto &BC = getAnalysis<GenXBackendConfig>();
  if (PerformStructSplitting && BC.doStructSplitting())
    return Substituter{M}.processAllocas();

  return false;
}

//__________________________________________________________________
//          Block of StructFilter definition.
//__________________________________________________________________

//
// Performs module checking for banned structs.
//
StructFilter::StructFilter(Module &M) {
  std::list<StructType *> NotBannedYet;
  // Looks for an element as an array.
  for (auto &&STy : M.getIdentifiedStructTypes())
    if (checkForArrayOfComplicatedStructs(*STy))
      NotBannedYet.push_front(STy);
    else
      BannedStructs.emplace(STy);

  // Looks for an element as banned struct.
  for (auto It = NotBannedYet.begin(); It != NotBannedYet.end(); /*none*/)
    if (!checkForElementOfBannedStruct(**It)) {
      BannedStructs.emplace(*It);
      NotBannedYet.erase(It);
      It = NotBannedYet.begin();
    } else
      ++It;
}

//
//  Returns true if STy is banned, otherwise - false.
//
bool StructFilter::isStructBanned(StructType &STy) const {
  return BannedStructs.find(&STy) != BannedStructs.end();
}

//
//  Checks if structure contains array or vector of complex type.
//  Returns true if it does not.
//
bool StructFilter::checkForArrayOfComplicatedStructs(StructType &STy) const {
  auto IsSequential = [](Type &Ty) {
    return Ty.isVectorTy() || Ty.isArrayTy();
  };

  return !llvm::any_of(STy.elements(), [IsSequential](Type *Elem) {
    Type *BaseTy = getArrayFreeTy(Elem);
    if (StructType *SBTy = dyn_cast<StructType>(BaseTy))
      return IsSequential(*Elem) && BaseTy == getBaseTy(SBTy);
    return false;
  });
}

//
//  Checks if structure contains an element of a banned struct.
//  Returns true if it does not.
//
bool StructFilter::checkForElementOfBannedStruct(StructType &STy) const {
  return !llvm::any_of(STy.elements(), [this](Type *Elem) {
    Type *BaseTy = getArrayFreeTy(Elem);
    if (StructType *SBTy = dyn_cast<StructType>(BaseTy))
      return isStructBanned(*SBTy);
    return false;
  });
}

//__________________________________________________________________
//          Block of DependencyGraph definition.
//__________________________________________________________________

//
//  Tries to get a base type of structure if structure is plain.
//  If STy is not plain then tries to use getBaseTy().
//
Type *DependencyGraph::getPlainSubTy(StructType &STy) const {
  if (isPlain(STy)) {
    auto It = AllStructs.find(&STy);
    IGC_ASSERT_EXIT(It != AllStructs.end());
    return It->second.begin()->first;
  } else
    return getBaseTy(&STy);
}

//
//  * Determines if structure STy is plain:
//     is contained in AllStruct
//     and there is only one baseTy.
//  * Works not intuitive for structs like: C1: { C2 }
//     returns true even though C2 can be complicated.
//
bool DependencyGraph::isPlain(StructType &STy) const {
  auto FindIt = AllStructs.find(&STy);
  return FindIt != AllStructs.end() && FindIt->second.size() < 2;
}

//
//  Checks if Struct has been processed, so info about it exists in InfoToMerge.
//  Returns true if record about struct exists, otherwise - false.
//
bool DependencyGraph::isStructProcessed(StructType &STy) const {
  return InfoToMerge.find(&STy) != InfoToMerge.end();
}

//
//  Gets the element's information of the struct.
//  Requires structure to be processed before.
//
const DependencyGraph::STypes &
DependencyGraph::getStructComponens(StructType &STy) const {
  auto FindIt = AllStructs.find(&STy);
  IGC_ASSERT_EXIT_MESSAGE(
      FindIt != AllStructs.end(),
      "Info about struct has to be collected before getting components.\n");
  return FindIt->second;
}

//
//  Gets vector of elements substitution of old struct with new substructs'
//  elements.
//
const DependencyGraph::ElemMapping &
DependencyGraph::getElemMappingFor(StructType &STy) const {
  auto FindIt = InfoToMerge.find(&STy);
  IGC_ASSERT_EXIT_MESSAGE(
      FindIt != InfoToMerge.end(),
      "Struct has to be processed before getting indices mapping.\n");
  return FindIt->second->second;
}

//
//  Gets element's list which substitutes split struct's(STy) element at
//  index(Idx).
//
const DependencyGraph::ListOfSplitElements &
DependencyGraph::getElementsListOfSTyAtIdx(StructType &STy,
                                           unsigned Idx) const {
  const ElemMapping &VecOfSTy = getElemMappingFor(STy);
  IGC_ASSERT_EXIT_MESSAGE(Idx < VecOfSTy.size(),
                          "Attempt to get element out of borders.");
  return VecOfSTy.at(Idx);
}

//
// Gets unique types into which the structure STy is split.
//
std::vector<Type *>
DependencyGraph::getUniqueSplitTypes(StructType &STy) const {
  std::unordered_set<Type *> UniqueSplitTypes;
  // Vector is for determination of structs order.
  std::vector<Type *> UniqueSplitTypesInOrder;
  // Gets unique substructs.
  for (auto &&C : getElemMappingFor(STy)) {
    for (auto &&BaseTy : C) {
      auto [_, IsInserted] = UniqueSplitTypes.emplace(BaseTy.getTy());
      if (IsInserted)
        UniqueSplitTypesInOrder.push_back(BaseTy.getTy());
    }
  }
  return UniqueSplitTypesInOrder;
}

//
//  * By AllStructs info generates dependency graph of structs.
//  * eg. generates smth like this:
//    C -----> A
//     \     /
//      \-> D
//       \   \
//        \-> B
//
void DependencyGraph::generateGraph() {
  LLVM_DEBUG(dbgs() << "Graph generating begin.\n");
  NodeTracker Inserted;
  Heads.reserve(AllStructs.size());
  for (auto &&[STy, _] : AllStructs) {
    if (Inserted.find(STy) != Inserted.end())
      // If already in graph -> skip
      continue;
    Heads.push_back(createNode(*STy, Inserted));
  }

  // During Graph creation a similar case can occur: (C and D are heads)
  // C -> D ..
  // D -> A
  // Removes D as it has parent=C.
  // Cleans up Heads. Erases all entities with parent.
  llvm::erase_if(Heads, [](Node *HeadNode) { return HeadNode->hasParent(); });
}

//
//  Creates the Node and places dependencies according to the Struct.
//
DependencyGraph::Node *DependencyGraph::createNode(StructType &STy,
                                                   NodeTracker &Inserted) {
  LLVM_DEBUG(dbgs() << "Creating node for struct: " << STy << "\n");
  auto FindIt = Inserted.find(&STy);
  if (FindIt != Inserted.end()) {
    // This can occur when Struct has a processed child element.
    // Parent will be automatically set right after this function.
    // Later clean-up heads. This node will be erased as it has parents.
    Node *node = FindIt->second;
    return node;
  }

  Node *ThisNode = NodeMM.create(STy);
  auto [It, IsInserted] = Inserted.emplace(&STy, ThisNode);
  IGC_ASSERT_MESSAGE(IsInserted,
                     "Processing Node which already has been processed.");

  for (auto &&[BaseTy, Children] : getStructComponens(STy))
    for (auto &&Child : Children.types())
      if (StructType *ChildSTy = dyn_cast<StructType>(getArrayFreeTy(Child))) {
        Node *ChildNode = createNode(*ChildSTy, Inserted);
        ChildNode->insertParent(*ThisNode);
        ThisNode->insertChild(*ChildNode);
      }

  return ThisNode;
}

//
//  * Processes the bottom node. Assumes that this node has no children
//    so all elements in this struct are plain.
//  * Splits this struct into plain substructs and recreates parent nodes.
//  * Eventually deletes this node from graph as
//    after processing struct will be split to plain substructs
//    and parent nodes will no longer need to track it.
//  * While processing nodes graph will self destruct.
//  * Info about all structs in Module (AllStructs) will be updated.
//  * Info about structs transformation (SplitStructs) will be updated.
//
void DependencyGraph::processNode(Node &SNode) {
  // Go to the bottom of the graph.
  while (SNode.hasChild())
    processNode(**SNode.child_begin());

  LLVM_DEBUG(dbgs() << "Processing node for struct: " << *SNode.getType()
                    << "\n");
  // Splitting always gets a plain type, so graph will be changed anyway.
  if (StructType *OldSTy = SNode.getType()) {
    // Splitting.
    const STypes &Types = getStructComponens(*OldSTy);
    // Indices of unsplit struct will be matched with indices of elements of
    // new split structs.
    ElemMapping IndicesMap(OldSTy->getNumElements());

    // First initialization with zeros as pos in vector will be Index of
    // Element.
    std::vector<Type *> GeneratedTypesInOrder(OldSTy->getNumElements());

    StringRef OldSTyName = OldSTy->getName();

    for (auto &&[BaseTy, Elements] : Types) {
      // Start point of Simplification.
      Type *NewPlainType{nullptr};
      if (Elements.size() == 1) {
        // It means that structure contains only one element that can be used
        // directly without structure.
        NewPlainType = Elements.getTyAt(0);
        IndicesMap[Elements.getIdxAt(0)].emplace_back(NewPlainType);
      } else if (!isPlain(*OldSTy)) {
        StructType *NewPlainStruct = StructType::create(
            Ctx, Elements.getTypesArray(),
            Twine(OldSTyName + "." + getTypePrefix(*BaseTy) + ".split").str());
        NewPlainType = NewPlainStruct;
        // Update AllStructs info.
        setInfoAboutStructure(*NewPlainStruct);
        // Match old elements with new elements.
        for (auto &&ElemIndex : enumerate(Elements.indices()))
          IndicesMap[ElemIndex.value()].emplace_back(NewPlainStruct,
                                                     ElemIndex.index());
      } else
        // Plain structs with more than 1 elements -> skip as there is nothing
        // to do.
        continue;
      // Way to implement ordering in Types placing.
      // Affects on ordering in List and ordering of elements in structures.
      // Prevents from cases like this:
      //
      // A { int, float, int, float };
      // C_BS { Af, Ai, float } and at another time: C_BS { Ai, Af, float };
      //
      // Order defined by elements in original structure.
      GeneratedTypesInOrder[Elements.getIdxAt(0)] = NewPlainType;
    }

    // Cleans an array from nullptr elements.
    llvm::erase_if(GeneratedTypesInOrder, [](Type *Ty) { return !Ty; });

    // Remakes parents if there is splitting or simplification.
    if (GeneratedTypesInOrder.size()) {
      // Updates SplitStructs.
      SplitStructs.emplace_back(OldSTy, std::move(IndicesMap));

      // Remakes parent Node.
      // As D will be split to Di,Df so C(parent) has to be split to
      // Ci,Cf. It will be done in 3 steps:
      // 1st: Creates intermediate struct before splitting:
      //   C_BS : {Ci, Cf}
      // 2nd: Substitutes struct C to C_BS in Node responsible for C.
      // 3rd: When D processing is done, C(C_BS) will be
      //      automatically split to Ci,Cf as Node responsible
      //      for C(C_BS) will no longer have children.
      // In this case there will be a record in transformation info:
      //  D     -> Di, Df
      //  C     -> C_BS
      //  C_BS  -> Ci, Cf
      // Therefore, transformation C->C_BS->Ci,Cf can be merged to C->Ci,Cf
      //
      // In case of simplification: type will be substituted directly, without
      // structure.
      // F { float } C { F, float } C_BS -> { float , float }
      llvm::for_each(SNode.parents(), [&](Node *ParentNode) {
        remakeParent(*ParentNode, SNode, GeneratedTypesInOrder);
      });
    }
  }

  // Removes dependencies.
  llvm::for_each(SNode.parents(),
                 [&SNode](Node *ParentNode) { ParentNode->eraseChild(SNode); });
}

//
//  * Creates unsplit struct with new element's types generated from child
//    Node.
//  * As D is split to Di,Df, structure C has to change element D to Di,Df and
//    splits later. remakeParent substitutes structure C in SNode with structure
//    C_BS that contains Di,Df.
//  * SNode - current parent node to be changed.
//  * SNodeToChange - child node that already has been changed.
//
void DependencyGraph::remakeParent(Node &SNode, Node &SNodeToChange,
                                   ArrayRef<Type *> NewReplaceTypes) {
  LLVM_DEBUG(dbgs() << "Recreating parent node: " << *SNode.getType()
                    << "\n\tChild node: " << *SNodeToChange.getType() << "\n");
  StructType *CurrentS = SNode.getType();
  StringRef CurrentSName = CurrentS->getName();
  const unsigned NumElements = CurrentS->getNumElements();
  const unsigned NewMaxSize = NumElements + NewReplaceTypes.size() - 1;
  std::vector<Type *> NewElements;
  NewElements.reserve(NewMaxSize);
  // First create an empty structure.
  // Later setBody with elements. It is for completing VecOfStructInfo.
  StructType *BeforeSplitingS = StructType::create(
      CurrentS->getContext(), Twine(CurrentSName + "_BS").str());
  ElemMapping NewIndices(NumElements);
  unsigned ExpandIndicies{0};
  for (auto &&ElemEnum : enumerate(CurrentS->elements())) {
    Type *Elem = ElemEnum.value();
    const unsigned Index = ElemEnum.index();
    if (StructType *SElem = dyn_cast<StructType>(Elem);
        SElem && SNodeToChange.containsStruct(*SElem)) {
      // If element of structure is split element, then we need to replace
      // this element with new.
      for (auto &&NewSTy : NewReplaceTypes) {
        IGC_ASSERT_EXIT((uint64_t)Index + (uint64_t)ExpandIndicies + 1ull <
                        std::numeric_limits<uint32_t>::max());
        NewElements.emplace_back(NewSTy);
        NewIndices[Index].emplace_back(BeforeSplitingS,
                                       Index + ExpandIndicies++);
      }
      // The Index will be inc, so there is no need of extra offset.
      --ExpandIndicies;
    } else {
      // If element of structure is not changed, then just copies info about it
      // and places right indices.
      NewElements.emplace_back(Elem);
      NewIndices[Index].emplace_back(BeforeSplitingS, Index + ExpandIndicies);
    }
  }

  BeforeSplitingS->setBody(NewElements);

  // Updates AllStructs and SplitStructs info.
  setInfoAboutStructure(*BeforeSplitingS);
  SplitStructs.emplace_back(CurrentS, std::move(NewIndices));

  // Substitutes structure in Node.
  SNode.substitute(*BeforeSplitingS);
}

//
//  For each Node in head launches Graph processing.
//  After processing as node is deleted we remove it from Heads.
//
void DependencyGraph::recreateGraph() {
  LLVM_DEBUG(dbgs() << "Graph recreating begin.\n");
  for (auto *Node : Heads)
    processNode(*Node);
}

//
//  Records information about structure into AllStructs.
//
void DependencyGraph::setInfoAboutStructure(StructType &STy) {
  LLVM_DEBUG(dbgs() << "Collecting infornation about struct: " << STy << "\n");
  STypes BaseTypes;
  const unsigned NumberOfElems = STy.getNumElements();
  // Puts each element and its index according to the base type.
  for (auto &&ElemEnum : enumerate(STy.elements())) {
    Type *Elem = ElemEnum.value();
    const unsigned Index = ElemEnum.index();
    Type *BaseTy = getBaseTy(Elem);
    // BaseTy can be a structure in AllStructs, so we get the info from
    // AllStructs.
    if (StructType *SBTy = dyn_cast<StructType>(BaseTy))
      BaseTy = getPlainSubTy(*SBTy);

    auto FindIt = BaseTypes.find(BaseTy);
    if (FindIt == BaseTypes.end()) {
      // If there is no entity with baseTy, creates it with preallocated array.
      bool IsInserted = false;
      std::tie(FindIt, IsInserted) =
          BaseTypes.emplace(BaseTy, SElementsOfType{NumberOfElems});
      IGC_ASSERT_EXIT_MESSAGE(IsInserted,
                              "Record about BaseTy already exists.");
      IGC_ASSERT_EXIT_MESSAGE(FindIt != BaseTypes.end(),
                              "Record about BaseTy already exists.");
    }
    // Emplace element to (created or existed) info(SElementsOfType) about
    // BaseTy.
    FindIt->second.emplace_back(*Elem, Index);
  }

  auto [_, IsInserted] = AllStructs.emplace(&STy, std::move(BaseTypes));
  IGC_ASSERT_MESSAGE(IsInserted,
                     "Processing Struct which already has been processed.");
}

//
//  * As BeforeSplitting struct is temporary it can be removed from
//    transformation info.
//  * Also only here the InfoToMerge is filling.
//
void DependencyGraph::mergeStructGenerationInfo() {
  LLVM_DEBUG(dbgs() << "Merging structs.\n");
  for (auto It = SplitStructs.rbegin(), End = SplitStructs.rend(); It != End;
       ++It) {
    StructType &SToMerge = *It->first;
    ElemMapping &InfoAboutS = It->second;

    if (StructType *SToMergeWith = checkAbilityToMerge(InfoAboutS)) {
      LLVM_DEBUG(dbgs() << "Able to merge: " << SToMerge << "\n\tWith "
                        << *SToMergeWith << "\n");

      const ElemMapping &InfoAboutTemporaryS = getElemMappingFor(*SToMergeWith);
      // Every element of the structure SToMerge will be substituted with
      // element from the structure SToMergeWith and/or new elements from
      // SToMergeWith will be placed in SToMerge.
      for (ListOfSplitElements &ElementsList : InfoAboutS) {
        for (SElement &Element : ElementsList) {
          IGC_ASSERT_MESSAGE(!Element.isUnwrapped(),
                             "Attempt to merge unwrapped type.");
          IGC_ASSERT_MESSAGE(Element.getIndex() < InfoAboutTemporaryS.size(),
                             "Attempt to get element out of borders.");
          const ListOfSplitElements &NewElement =
              InfoAboutTemporaryS.at(Element.getIndex());

          auto EIt = NewElement.rbegin();
          // Changes current element and, if on this 'Element.Index' lots of new
          // elements are to be placed, extends list with new elements.
          // Pushes front new element not to invalidate iterations.
          // Iterates from end to begin (rbegin to rend) to keep order of
          // elements.
          // eg. merges information
          // G    : (G_BS, 0) (...)
          //        (...)
          // G_BS : (SomeS, 5) (Gf, 0) (Gi, 0)
          //
          // Will become
          // G    : (SomeS, 5) (Gf, 0) (Gi, 0) (...)

          // Element (G_BS, 0) will become (Gi, 0).
          Element = *EIt;
          // Elements (SomeS, 5) (Gf, 0) will be placed before (G_BS, 0) in the
          // same order as in G_BS.
          while (++EIt != NewElement.rend())
            ElementsList.push_front(*EIt);
        }
      }
    }

    InfoToMerge.emplace(It->first, It);
  }
}

//
//  * We are able to merge two struct's records only if new elements of struct
//    are the same.
//  * C : (C_BS, 0)
//        (C_BS, 1)
//        (C_BS, 2)
//        (C_BS, 3)
//        (C_BS, 4) (C_BS, 5)
//  * C_BS :  (Cf, 0)
//            (Ci, 0)
//            (Cf, 1)
//            (Ci, 1)
//            (Cf, 2)
//            (Ci, 2)
//
StructType *
DependencyGraph::checkAbilityToMerge(const ElemMapping &NewSTypes) const {
  IGC_ASSERT_MESSAGE(NewSTypes.size(), "Merging empty structs.");
  IGC_ASSERT_MESSAGE(NewSTypes.begin()->size(), "Merging empty structs.");
  auto FirstElem = NewSTypes.begin()->begin();
  if (FirstElem->isUnwrapped())
    return nullptr;
  StructType *STyToCheck = FirstElem->getStructTy();

  // Checks that all split structs are same. It is the main criteria for
  // iterations of splitting to be merged.
  bool AreSameStructs = llvm::all_of(NewSTypes, [STyToCheck](const auto &S) {
    return llvm::all_of(S, [STyToCheck](const auto &Elt) {
      return Elt.getStructTyIfPossible() == STyToCheck;
    });
  });

  if (AreSameStructs && isStructProcessed(*STyToCheck))
    return STyToCheck;
  return nullptr;
}

//
//  Constructor gets all initial information about structures in Module.
//
DependencyGraph::DependencyGraph(Module &M, const StructFilter &Filter)
    : Ctx{M.getContext()}, NodeMM{M} {
  for (auto &&STy : M.getIdentifiedStructTypes())
    if (!Filter.isStructBanned(*STy))
      setInfoAboutStructure(*STy);
}

//
//  Launches structure dependencies processing.
//
void DependencyGraph::run() {
  generateGraph();
  recreateGraph();
  mergeStructGenerationInfo();
}

//__________________________________________________________________
//          Block of Substituter definition.
//__________________________________________________________________

//
//  Collects all information of structs, allocas and launches struct splittting,
//  based on this information.
//
Substituter::Substituter(Module &M)
    : Ctx{M.getContext()}, DL{M.getDataLayout()}, Filter{M}, Graph{M, Filter},
      DIB{M} {
  Graph.run();
  LLVM_DEBUG(Graph.print(dbgs()));

  // Visit should be after graph processing.
  visit(M);
}

//
//  Collects all allocas that allocate memory for structure to split.
//
void Substituter::visitAllocaInst(AllocaInst &AI) {
  if (StructType *STy = dyn_cast<StructType>(AI.getAllocatedType()))
    if (Graph.isStructProcessed(*STy)) {
      LLVM_DEBUG(dbgs() << "Collecting alloca to replace: " << AI << "\n");
      Allocas.emplace_back(&AI);
    }
}

//
//  Generates new allocas.
//  Returns Instruction set within base types for easy access
//  and Instruction vector in order of generation.
//
Substituter::TypeToInstrMap
Substituter::generateNewAllocas(AllocaInst &OldInst) {
  LLVM_DEBUG(dbgs() << "Generating allocas to replace: " << OldInst << "\n");
  StructType &STy = *cast<StructType>(OldInst.getAllocatedType());

  std::unordered_set<Type *> UniqueSplitTypes;
  TypeToInstrMap NewInstructions;

  for (auto &&Base : Graph.getElemMappingFor(STy)) {
    for (auto &&BaseTy : Base) {
      Type *NewTy = BaseTy.getTy();
      auto [_, IsTypeInserted] = UniqueSplitTypes.emplace(NewTy);
      if (IsTypeInserted) {
        // Generating one alloca to each unique type.
        AllocaInst *NewAlloca = generateAlloca(OldInst, BaseTy);
        auto [_, IsAllocaInserted] =
            NewInstructions.emplace(getBaseTy(NewTy), NewAlloca);
        IGC_ASSERT_MESSAGE(IsAllocaInserted,
                           "Alloca instruction responsible for structure(type) "
                           "has already been created.\n");
      }
    }
  }
  return NewInstructions;
}

// Help function to print info about unsupported debug intrinsics.
static void reportUnsupportedDbgIntrinsics(
    raw_ostream &Os, StringRef Reason,
    const SmallVectorImpl<DbgVariableIntrinsic *> &DbgIntrinsics) {
  Os << Reason << ":\n";
  for (auto *DbgIntrinsic : DbgIntrinsics)
    Os << '\t' << *DbgIntrinsic << '\n';
}

// Finds DbgDeclareInst in mix of debug intrinsics
// that Val points to. Can return only one dbg.declare.
// Returns nullptr it there is no any dbg.declare or more than one.
static DbgDeclareInst *getDbgDeclare(Value &Val) {
  // Gets the mix of dbg.declare and dbg.addr.
  SmallVector<DbgVariableIntrinsic *, 4> DbgIntrinsics;
  findDbgUsers(DbgIntrinsics, &Val);

  // If there is no DI at all, returns nullptr without warning.
  if (DbgIntrinsics.empty())
    return nullptr;

  SmallVector<DbgVariableIntrinsic *, 4> DbgDeclares;
  llvm::copy_if(
      DbgIntrinsics, std::back_inserter(DbgDeclares),
      [](DbgVariableIntrinsic *Intr) { return isa<DbgDeclareInst>(Intr); });

  // Returns nullptr if there is no dbg.declare at all.
  if (DbgDeclares.empty()) {
    LLVM_DEBUG(reportUnsupportedDbgIntrinsics(
        dbgs(), "No dbg.declare for value", DbgIntrinsics));
    return nullptr;
  }

  // Returns nullptr if there are more than one dbg.declares.
  if (DbgDeclares.size() > 1) {
    LLVM_DEBUG(reportUnsupportedDbgIntrinsics(
        dbgs(), "Too many dbg.declare for value", DbgDeclares));
    return nullptr;
  }

  return cast<DbgDeclareInst>(DbgDeclares.front());
}

//
// Generates new alloca for TyElem to replace AI - the old one.
// For TyElem generates DI.
//
AllocaInst *
Substituter::generateAlloca(AllocaInst &AI,
                            const DependencyGraph::SElement &TyElem) {
  IRBuilder<> IRB{&AI};

  Type *NewTy = TyElem.getTy();
  AllocaInst *NewAI = IRB.CreateAlloca(
      NewTy, 0, AI.getName() + "." + getTypePrefix(*getBaseTy(NewTy)));
  NewAI->setAlignment(IGCLLVM::getAlign(AI));

  DbgDeclareInst *DbgDeclare = getDbgDeclare(AI);
  if (!DbgDeclare)
    return NewAI;

  // *NewTy can be:
  // - base type, non-split struct (also base type).
  // - split struct (for each element to generate DI).
  std::vector<Type *> TypesToGenerateDI;
  if (TyElem.isUnwrapped()) {
    TypesToGenerateDI.push_back(NewTy);
  } else {
    StructType &STy = *TyElem.getStructTy();
    llvm::copy(STy.elements(), std::back_inserter(TypesToGenerateDI));
  }
  updateDbgInfo(TypesToGenerateDI, AI, *NewAI, *DbgDeclare);

  return NewAI;
}

//
// Deduces lifetime.start and lifetime.end instructions for old alloca and
// generates them for new one
//
void Substituter::createLifetime(Instruction *OldI, AllocaInst *NewAI) {
  for (auto *User : OldI->users()) {
    if (auto *CastI = dyn_cast<BitCastInst>(User)) {
      createLifetime(CastI, NewAI);
    } else if (auto *II = dyn_cast<IntrinsicInst>(User)) {
      auto MaybeSize =
          IGCLLVM::makeOptional(NewAI->getAllocationSizeInBits(DL));
      IGC_ASSERT_EXIT(MaybeSize.has_value());

      IRBuilder<> Builder(II);
      auto *SizeC = Builder.getInt64(MaybeSize.value() / genx::ByteBits);

      switch (II->getIntrinsicID()) {
      case Intrinsic::lifetime_start:
        Builder.CreateLifetimeStart(NewAI, SizeC);
        break;
      case Intrinsic::lifetime_end:
        Builder.CreateLifetimeEnd(NewAI, SizeC);
        break;
      default:
        break;
      }
    }
  }
}

//
// Help-function to go other the list and check if Type with possition Idx is
// found. FirstMatch is for search regardless of index. Finds the firts match of
// type.
//
static bool isElementInList(const DependencyGraph::ListOfSplitElements &List,
                            Type *Ty, unsigned Idx, bool FirstMatch) {
  auto ListIt = llvm::find_if(
      List, [Idx, Ty, FirstMatch](const DependencyGraph::SElement &Elem) {
        // FirstMatch is needed only for searching in
        // substructures, where the first occurrence of type is to
        // be found.
        if (FirstMatch)
          return Elem.getTy() == Ty;
        // If element is unwrapped -> only checks on types match.
        // If element is not unwrapped -> additionaly checks Index.
        return Elem.getTy() == Ty &&
               (Elem.isUnwrapped() ||
                !Elem.isUnwrapped() && Elem.getIndex() == Idx);
      });
  return ListIt != List.end();
}

//
// Finds record in IdxMap of Ty placed on Idx position.
// FirstMatch is for search regardless of index.
//
static auto findRecord(const DependencyGraph::ElemMapping &IdxMap, Type *Ty,
                       unsigned Idx, bool FirstMatch) {
  auto FindIt = llvm::find_if(IdxMap, [Ty, Idx, FirstMatch](auto &&List) {
    return isElementInList(List, Ty, Idx, FirstMatch);
  });
  IGC_ASSERT_EXIT_MESSAGE(FindIt != IdxMap.end(), "Record has to be found!");
  return FindIt;
};

//
// For NewAI for each TypesToGenerateDI (elements of the new structure/type)
// generates DI based on DI about AI. Calculates proper fragments of the
// elements for new types.
// TODO: Cannot proper handle cases when order of elements is in mess.
// ex:
//  A = {i, f, f, i}; B = {i, A, f};
//  A will split to: Ai = {i, i};
//                   Af = {f, f};
//  B will split to: Bi = {i, Ai};
//                   Bf = {Af, f};
//  DI fragments about B will be:
//  - about Bi:
//    dbg.declare(..., ..., fragment(0, 32)) <- the first element of B.
//    dbg.declare(..., ..., fragment(32, 64)) <- problem:
//      should be like fragment(32, 32) <- from the first element of A
//                and  fragment(128, 32) <- from the last element of A.
//    But as Bi consists of { i, Ai }: elements of Bi are i and
//    Ai, that already contains { i, i } so fragment is (32, 64).
//  B actually is {i, {i, f, f, i}, f}, but after splitting
//                    ^- from A
//  B is {i, {i, i}, {f, f}, f}
//           ^- Ai   ^- Af
//
void Substituter::updateDbgInfo(ArrayRef<Type *> TypesToGenerateDI,
                                AllocaInst &AI, AllocaInst &NewAI,
                                DbgDeclareInst &DbgDeclare) {
  LLVM_DEBUG(dbgs() << "Rewritting dbg info about: " << AI << "\n");
  DILocalVariable &Var = *DbgDeclare.getVariable();
  DIExpression &Expr = *DbgDeclare.getExpression();
  DILocation &DbgLoc = *DbgDeclare.getDebugLoc();

  Type *NewTy = NewAI.getAllocatedType();
  StructType &STy = *cast<StructType>(AI.getAllocatedType());
  const ElemMapping &IdxMap = Graph.getElemMappingFor(STy);

  // For each element of the new structure generates DI.
  for (auto &&TypeEnum : enumerate(TypesToGenerateDI)) {
    unsigned Offset{0};

    // Type and position of the element.
    Type *TyToGenDI = TypeEnum.value();
    unsigned ElemIdx = TypeEnum.index();

    auto getOffsetInBits = [this](unsigned Idx, StructType *STy) {
      return DL.getStructLayout(STy)->getElementOffsetInBits(Idx);
    };

    // Finds original type from which element came.
    auto VecIt = findRecord(IdxMap, NewTy, ElemIdx, false);
    unsigned IdxOfOrigStruct = VecIt - IdxMap.begin();
    Type *OrigTy = STy.getElementType(IdxOfOrigStruct);

    // If it is processed Structure, offset of the element in this substructure
    // has to be calculated.
    if (StructType *OrigSTy = dyn_cast<StructType>(OrigTy);
        OrigSTy && Graph.isStructProcessed(*OrigSTy)) {
      const ElemMapping &SubSIdxMap = Graph.getElemMappingFor(*OrigSTy);
      // Offset from OrigStruct + Offset from SubStruct.
      auto SubSVecIt =
          findRecord(SubSIdxMap, TyToGenDI, /*does not matter*/ 0, true);
      unsigned IdxOfSubStruct = SubSVecIt - SubSIdxMap.begin();
      Offset = getOffsetInBits(IdxOfOrigStruct, &STy) +
               getOffsetInBits(IdxOfSubStruct, OrigSTy);
    } else
      // Offset from OrigStruct.
      Offset = getOffsetInBits(IdxOfOrigStruct, &STy);

    auto FragExpr =
        IGCLLVM::makeOptional(DIExpression::createFragmentExpression(
            &Expr, Offset, DL.getTypeAllocSizeInBits(TyToGenDI)));

    IGC_ASSERT_MESSAGE(FragExpr.has_value(), "Failed to create new expression");

    Instruction &NewDbgDeclare =
        *DIB.createDbgDeclare(NewAI, Var, *FragExpr.value(), DbgLoc, AI);
    LLVM_DEBUG(dbgs() << "New dbg.declare is created: " << NewDbgDeclare
                      << '\n';);
  }
}

//
//  Returns Instruction responsible for processing Type.
//
static Instruction *findProperInstruction(
    Type *Ty, const std::unordered_map<Type *, Instruction *> &NewInstr) {
  auto FindInstrIt = NewInstr.find(Ty);
  IGC_ASSERT_EXIT_MESSAGE(
      FindInstrIt != NewInstr.end(),
      "Cannot find instruction according to split structure type.");
  return FindInstrIt->second;
}

//
//  Creating new GEPI instruction.
//  GEPI - instruction to replace.
//  PlainType - the result type of new gep work.
//  IdxPath - the sequence of indices to recive needed type.
//  NewInstr - instruction map to set proper uses.
//  PlainTyIdx - index of the first plain type.
//
Instruction *
Substituter::generateNewGEPs(GetElementPtrInst &GEPI, Type &PlainType,
                             const DependencyGraph::SElementsOfType &IdxPath,
                             const TypeToInstrMap &NewInstr,
                             unsigned PlainTyIdx) const {
  LLVM_DEBUG(dbgs() << "Generating GEP to replace: " << GEPI << "\n");
  IGC_ASSERT_MESSAGE(PlainTyIdx <= IdxPath.size(),
                     "Index of the plain type is out of boundaries.");

  SElementsOfType LocalIdxPath{IdxPath.size()};

  // Generates new indices path till PlainTyIdx.
  std::for_each(IdxPath.begin(), IdxPath.begin() + PlainTyIdx,
                [&PlainType, &LocalIdxPath, this](auto &&Elem) {
                  auto [Ty, Idx, V] = std::move(Elem);
                  StructType *STy = cast<StructType>(Ty);
                  const ListOfSplitElements &ListOfPossibleTypes =
                      Graph.getElementsListOfSTyAtIdx(*STy, Idx);

                  auto FindIt = llvm::find_if(
                      ListOfPossibleTypes, [&PlainType](auto &&PosElem) {
                        Type *PossibleTy = PosElem.getTy();
                        // For now getBaseTy is similar to getting type from
                        // structure info, but in further it may be different
                        // while processing arrays and vectors of structures.
                        return getBaseTy(PossibleTy) == &PlainType;
                      });
                  IGC_ASSERT_EXIT_MESSAGE(FindIt != ListOfPossibleTypes.end(),
                                          "No substitution type.");
                  // Skip indices if it gives unwrapped type.
                  if (!FindIt->isUnwrapped())
                    LocalIdxPath.emplace_back(*FindIt, V);
                });
  std::copy(IdxPath.begin() + PlainTyIdx, IdxPath.end(),
            std::back_inserter(LocalIdxPath));

  // If Size == 0 then we do not need to create a GEP. Just find proper previous
  // instruction.
  Instruction *ToInsert = findProperInstruction(&PlainType, NewInstr);
  const unsigned Size = LocalIdxPath.size();
  if (!Size) {
    LLVM_DEBUG(dbgs() << "Instruction has been reused: " << *ToInsert << "\n");
    return ToInsert;
  }

  // Generates new IdxList for instruction.
  std::vector<Value *> IdxList;
  IdxList.reserve(Size + 1);
  IdxList.emplace_back(*GEPI.idx_begin());
  for (unsigned Idx = 0; Idx < Size; Idx++)
    if (auto *V = LocalIdxPath.getValAt(Idx))
      IdxList.emplace_back(V);
    else
      IdxList.emplace_back(
          ConstantInt::get(Ctx, APInt(32, LocalIdxPath.getIdxAt(Idx))));

  Type *Inserted = LocalIdxPath.getTyAt(0);

  IRBuilder<> IRB{&GEPI};
  Instruction *NewGEP = cast<Instruction>(
      IRB.CreateGEP(Inserted, ToInsert, IdxList, GEPI.getName() + ".split"));

  LLVM_DEBUG(dbgs() << "Instruction has been created: " << *NewGEP << "\n");

  return NewGEP;
}
//
//  An entry point of replacement instructions.
//  First replaces allocas, then replaces GEP and so one.
//
bool Substituter::processAllocas() {
  bool Changed{false};
  for (AllocaInst *Alloca : Allocas)
    Changed |= processAlloca(*Alloca);

  return Changed;
}
//
//  Processes Alloca and all users of it. If processing current alloca fails,
//  process the next one.
//
bool Substituter::processAlloca(AllocaInst &Alloca) {
  LLVM_DEBUG(dbgs() << "Processing alloca: " << Alloca << "\n");
  auto InstUses = getInstUses(Alloca);
  if (!InstUses)
    return false;
  auto [UsesGEP, UsesPTI] = std::move(InstUses.value());

  TypeToInstrMap NewInstrs = generateNewAllocas(Alloca);

  InstsToSubstitute InstToInst;

  for (GetElementPtrInst *GEP : UsesGEP)
    if (!processGEP(*GEP, NewInstrs, InstToInst))
      return false;
  for (PtrToIntInst *PTI : UsesPTI)
    if (!processPTI(*PTI, Alloca.getAllocatedType(), NewInstrs, InstToInst))
      return false;

  for (auto &[InstToReplace, ToInst] : InstToInst)
    InstToReplace->replaceAllUsesWith(ToInst);

  for (auto &[Ty, Inst] : NewInstrs) {
    auto *NewAlloca = cast<AllocaInst>(Inst);
    createLifetime(&Alloca, NewAlloca);
  }

  return true;
}

//
// Retrieves information of Type gotten within each index access.
// eg.
//  %a = gep C, 0, 4, 0
//  (C, 4) -> D
//  (D, 0) -> A
//
std::tuple<DependencyGraph::SElementsOfType, std::vector<Type *>>
Substituter::getIndicesPath(GetElementPtrInst &GEPI) {
  const unsigned Size = GEPI.getNumIndices() - 1;
  DependencyGraph::SElementsOfType IdxPath{Size};
  std::vector<Type *> GottenTypeArr;
  GottenTypeArr.reserve(Size);

  // Skips first operator as it always 0 to deref poiterTy and get to structTy.
  Type *CurrentType = GEPI.getSourceElementType();
  for (auto It = GEPI.idx_begin() + 1, End = GEPI.idx_end(); It != End; ++It) {
    Value *VIdx = *It;
    if (CurrentType->isVectorTy() || CurrentType->isArrayTy()) {
      IdxPath.emplace_back(*CurrentType, 0, VIdx);
      CurrentType = CurrentType->getContainedType(0);
    } else {
      // Struct indices are required to be i32 constant
      auto *CIdx = cast<Constant>(VIdx);
      IGC_ASSERT(CIdx->getType()->getScalarType()->isIntegerTy(32));
      const APInt &Int = CIdx->getUniqueInteger();
      uint64_t Idx = Int.getZExtValue();
      IdxPath.emplace_back(*CurrentType, Idx);
      CurrentType = CurrentType->getContainedType(Idx);
    }
    GottenTypeArr.emplace_back(CurrentType);
  }
  return std::make_tuple(std::move(IdxPath), std::move(GottenTypeArr));
}

//
// Gets GEP and PTI users of instruction I.
//
std::optional<
    std::tuple<std::vector<GetElementPtrInst *>, std::vector<PtrToIntInst *>>>
Substituter::getInstUses(Instruction &I) {
  // Checks That users of Instruction are appropriate.
  std::vector<GetElementPtrInst *> UsesGEP;
  std::vector<PtrToIntInst *> UsesPTI;
  UsesGEP.reserve(I.getNumUses());
  UsesPTI.reserve(I.getNumUses());
  for (const auto &U : I.uses())
    if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(U.getUser()))
      UsesGEP.push_back(GEP);
    else if (PtrToIntInst *PTI = dyn_cast<PtrToIntInst>(U.getUser()))
      UsesPTI.push_back(PTI);
    else if (BitCastInst *BC = dyn_cast<BitCastInst>(U.getUser())) {
      auto UnsupportedBCUser = llvm::find_if_not(BC->users(), [](User *BCU) {
        auto IID = vc::getAnyIntrinsicID(BCU);
        return IID == llvm::Intrinsic::lifetime_start ||
               IID == llvm::Intrinsic::lifetime_end;
      });

      if (UnsupportedBCUser != BC->users().end()) {
        LLVM_DEBUG(
            dbgs()
            << "WARN:: Bitcast is used where it cannot be used!\n\tBitcast: "
            << *BC << "\n\tUser:    " << **UnsupportedBCUser << "\n");
        return std::nullopt;
      }
    } else {
      LLVM_DEBUG(
          dbgs()
          << "WARN:: Struct is used where it cannot be used!\n\tInstruction: "
          << *U.getUser() << "\n");
      return std::nullopt;
    }
  return std::make_tuple(std::move(UsesGEP), std::move(UsesPTI));
}

//
//  * Generates new instructions that use split struct.
//  * If result of old instruction is a struct to be split:
//    generates new instructions and results of them are split structs.
//  * If result of old instruction is unsplit struct or data:
//    just generates new instruction with proper access indices.
//  * FE1: %d = gep C, 0, 4
//     -> %di = gep Ci, 0, 2
//     -> %df = gep Cf, 0, 1
//  * FE2: %a = gep C, 0, 4, 0
//     -> %a = gep Cf, 0, 2, 0
//
bool Substituter::processGEP(GetElementPtrInst &GEPI,
                             const TypeToInstrMap &NewInstr,
                             InstsToSubstitute /*OUT*/ &InstToInst) {
  LLVM_DEBUG(dbgs() << "Processing uses of instruction: " << GEPI << "\n");
  auto [IdxPath, GottenTypeArr] = std::move(getIndicesPath(GEPI));
  const unsigned Size = GottenTypeArr.size();
  IGC_ASSERT_MESSAGE(
      IdxPath.size() == Size,
      "IdxPath and GottenTypeArr must be consistent with each other.");

  // Finds the first index of plain type.
  // All indices after PlaintTyIdx can be copied.
  auto FindIt = llvm::find_if(GottenTypeArr, [this](Type *Ty) {
    StructType *STy = dyn_cast<StructType>(Ty);
    return !STy || !Graph.isStructProcessed(*STy);
  });
  unsigned PlainTyIdx = FindIt - GottenTypeArr.begin();

  if (PlainTyIdx == Size) {
    Type *PrevType = IdxPath.getTyAt(Size - 1);
    // In case of opaque pointers, GEP's leading zero indices can be omitted and
    // in this case previous type is not a structure type
    if (!isa<StructType>(PrevType)) {
      Type *PlainType = getBaseTy(GottenTypeArr[Size - 1]);
      Instruction *ToInsert =
          generateNewGEPs(GEPI, *PlainType, IdxPath, NewInstr, Size - 1);
      InstToInst.emplace_back(cast<Instruction>(&GEPI), ToInsert);
      return true;
    }
    // Case of FE1
    auto InstUses = getInstUses(GEPI);
    if (!InstUses)
      return false;
    auto [UsesGEP, UsesPTI] = std::move(InstUses.value());

    // That means that we are getting split struct so we need to create GEPs.
    // STyToBeSplit is the result of the instruction.
    unsigned Idx = IdxPath.getIdxAt(Size - 1);
    StructType *STyToBeSplit = cast<StructType>(PrevType);
    const ListOfSplitElements &ListOfPossibleTypes =
        Graph.getElementsListOfSTyAtIdx(*STyToBeSplit, Idx);

    TypeToInstrMap NewInstructions;
    NewInstructions.reserve(ListOfPossibleTypes.size());

    // For each substruct we have to generate it's own IdxPath and GEP.
    for (auto &&DestETy : ListOfPossibleTypes) {
      Type *PlainType = DestETy.isUnwrapped()
                            ? getBaseTy(DestETy.getTy())
                            : Graph.getPlainSubTy(*DestETy.getStructTy());

      Instruction *ToInsert =
          generateNewGEPs(GEPI, *PlainType, IdxPath, NewInstr, PlainTyIdx);
      NewInstructions.emplace(PlainType, ToInsert);
    }

    // Runs user processing on GEP and PTI users.
    // All uses has to be changed.
    for (GetElementPtrInst *GEP : UsesGEP)
      if (!processGEP(*GEP, NewInstructions, InstToInst))
        return false;
    for (PtrToIntInst *PTI : UsesPTI)
      if (!processPTI(*PTI, GEPI.getResultElementType(), NewInstructions,
                      InstToInst))
        return false;
  } else {
    Type *PrevType = IdxPath.getTyAt(PlainTyIdx);
    Type *PlainType = getBaseTy(GottenTypeArr[PlainTyIdx]);
    // In case of opaque pointers, GEP's leading zero indices can be omitted and
    // in this case previous type is not a structure type
    if (auto *STyToBeSplit = dyn_cast<StructType>(PrevType)) {
      unsigned Idx = IdxPath.getIdxAt(PlainTyIdx);
      IGC_ASSERT_MESSAGE(
          Graph.getElementsListOfSTyAtIdx(*STyToBeSplit, Idx).size() == 1,
          "Access to element of Struct does not get unsplit type.");
      PlainTyIdx += 1;
    }
    Instruction *ToInsert =
        generateNewGEPs(GEPI, *PlainType, IdxPath, NewInstr, PlainTyIdx);
    InstToInst.emplace_back(cast<Instruction>(&GEPI), ToInsert);
  }
  return true;
}

//
// Verifies that element does not violate restrictions for PTI to be
// substituted. Checks that:
//  1. Unwrapped element can be only the first.
//  2. Element type matches the FirstElementType.
//  3. Indices are sequential.
// Returns false if one of the point is violated.
//
// STy - Processing structure just for debbuging information.
// NewElem - Current element to be verified.
// TheFirstElemTy - The first element type of the structure.
// IdxOfSplitStructElem - Index of the current element to be verified in
//  the original structure.
//
static bool verifyElement(StructType &STy,
                          const DependencyGraph::SElement &NewElem,
                          Type *TheFirstElemTy, unsigned IdxOfSplitStructElem) {
  if (NewElem.isUnwrapped() && IdxOfSplitStructElem) {
    // Ai { Bi, i32 };
    // ptr = &Ai;
    // ptr += sizeof(bi);
    // Prohibeted as poiter covers unsequential types.
    // Unwrapped type can be only at the first position.
    LLVM_DEBUG(dbgs() << "WARN:: Struct (" << STy
                      << ") cannot be split as pointer offset covers "
                         "unsequential types and base type("
                      << *NewElem.getTy()
                      << ") can be only in the begining.\n");
    return false;
  }
  if (NewElem.getTy() != TheFirstElemTy) {
    // A {i32, i32, float}; Offset = 8byte
    // Prohibited as offset covers i32 and float.
    LLVM_DEBUG(dbgs() << "WARN:: Struct (" << STy
                      << ") cannot be split as pointer offset covers "
                         "different split types.\n");
    return false;
  }
  if (!NewElem.isUnwrapped() && NewElem.getIndex() != IdxOfSplitStructElem) {
    LLVM_DEBUG(dbgs() << "WARN:: Struct (" << STy
                      << ") cannot be split as pointer offset covers "
                         "unsequential types.\n");
    return false;
  }
  return true;
}

//
//  Checks if accessing by ptr covers one unsplit block and substitutes
//  struct. Tracks max offset of ptr until ptr goes to function. If function is
//  read/write, then checks if max offset lies within unsplit block. If it
//  does, then substitutes the struct. Otherwise we cannot split the struct.
//
bool Substituter::processPTI(PtrToIntInst &PTI, Type *Ty,
                             const TypeToInstrMap &NewInstr,
                             InstsToSubstitute /*OUT*/ &InstToInst) {
  auto &STy = *cast<StructType>(Ty);
  uint64_t MaxPtrOffset{0};
  if (!processPTIsUses(PTI, MaxPtrOffset))
    return false;

  const ElemMapping &IdxMapping = Graph.getElemMappingFor(STy);
  const SElement &TheFirstNewElem = *IdxMapping.begin()->begin();
  // Previously, this rule was a limitation on the PTI instruction, now when
  // strict element order in list has been introduced, it is essential logical
  // part of pass.
  IGC_ASSERT_MESSAGE(TheFirstNewElem.isUnwrapped() ||
                         !TheFirstNewElem.getIndex(),
                     "The first element of the original structure has to be "
                     "mathced with the first element of the split structure.");
  Type *TheFirstElemTy = TheFirstNewElem.getTy();

  // If MaxPtrOffset covers elements, which will be laid sequitially within one
  // new struct, then we can substitute PTI.
  int Index = 0;
  for (auto &&C : IdxMapping) {
    bool EarlyExit = false;
    for (auto &&Elem : C) {
      if (!verifyElement(STy, Elem, TheFirstElemTy, Index++))
        return false;

      if (!MaxPtrOffset) {
        EarlyExit = true;
        break;
      }
      const uint64_t SizeOfElem =
          vc::getTypeSize(Elem.retrieveElemTy(), &DL).inBytes();
      MaxPtrOffset = SizeOfElem > MaxPtrOffset ? 0 : MaxPtrOffset - SizeOfElem;
    }
    if (EarlyExit)
      break;
  }

  Instruction *ToInsert =
      findProperInstruction(getBaseTy(TheFirstElemTy), NewInstr);

  IRBuilder<> IRB{&PTI};
  Value *NewPTI =
      IRB.CreatePtrToInt(ToInsert, PTI.getType(), PTI.getName() + ".split");

  LLVM_DEBUG(dbgs() << "New Instruction has been created: " << *NewPTI << "\n");
  InstToInst.emplace_back(cast<Instruction>(&PTI), cast<Instruction>(NewPTI));
  return true;
}

//
// Callculates offset after add instruction.
//
std::optional<uint64_t> Substituter::processAddOrInst(Instruction &User,
                                                      BinaryOperator &BO) {
  IGC_ASSERT_EXIT(BO.getOpcode() == Instruction::Add ||
                  BO.getOpcode() == Instruction::Or);
  // Do Ptr Offset calculation.
  uint64_t LocalPtrOffset{0};
  Value *V0 = BO.getOperand(0);
  // If the one of operands is the Instruction then the other is ptr offset.
  // It can be vector or scalar.
  // "add V 5", "add 5 V", "or V 5" or "or 5 V"
  Value *ToCalculateOffset =
      dyn_cast<Instruction>(V0) != &User ? V0 : BO.getOperand(1);
  Constant *ConstantOffsets = dyn_cast<Constant>(ToCalculateOffset);
  if (!ConstantOffsets) {
    LLVM_DEBUG(dbgs() << "WARN:: Calculation of the pointer offset has to "
                         "be staticly known\n. Bad instruction: "
                      << BO << "\n");
    return std::nullopt;
  }
  Type *OffsetTy = ToCalculateOffset->getType();
  if (OffsetTy->isVectorTy()) {
    const unsigned Width =
        cast<IGCLLVM::FixedVectorType>(OffsetTy)->getNumElements();
    for (unsigned i = 0; i != Width; ++i) {
      Value *OffsetValue = ConstantOffsets->getAggregateElement(i);
      Constant *COffsetValue = cast<Constant>(OffsetValue);
      uint64_t Offset = COffsetValue->getUniqueInteger().getZExtValue();
      LocalPtrOffset = std::max(LocalPtrOffset, Offset);
    }
  } else if (OffsetTy->isIntegerTy()) {
    uint64_t Offset = ConstantOffsets->getUniqueInteger().getZExtValue();
    LocalPtrOffset = std::max(LocalPtrOffset, Offset);
  } else {
    LLVM_DEBUG(
        dbgs()
        << "Offset is unsupported type. Has to be Integer or Vector, but: "
        << *OffsetTy << "\n");
    return std::nullopt;
  }
  return LocalPtrOffset;
}

//
//  Checks for appropreate operations on ptr and calculates max offset of ptr.
//  Calculation has to be done staticly.
//  ptr may only go to read/write funcitons.
//
bool Substituter::processPTIsUses(Instruction &I,
                                  uint64_t /*OUT*/ &MaxPtrOffset) {
  uint64_t LocalPtrOffset{0};
  for (const auto &U : I.uses()) {
    Instruction *User = cast<Instruction>(U.getUser());
    auto Opcode = User->getOpcode();
    if (Opcode == Instruction::Add || Opcode == Instruction::Or) {
      IGC_ASSERT_EXIT(dyn_cast<BinaryOperator>(User));
      BinaryOperator *BO = cast<BinaryOperator>(User);
      auto Offset = processAddOrInst(I, *BO);
      if (!Offset)
        return false;
      LocalPtrOffset = std::max(LocalPtrOffset, Offset.value());
    } else if (auto *LdI = dyn_cast<LoadInst>(User)) {
      // simple loads are allowed
      if (!LdI->isSimple())
        return false;
      continue;
    } else if (auto *StI = dyn_cast<StoreInst>(User)) {
      // simple stores are allowed
      if (!StI->isSimple() || StI->getValueOperand() == &I)
        return false;
      continue;
    } else if (User->mayReadOrWriteMemory()) {
      // memory read or write intrinsics like gather/scatter are allowed
      auto IID = vc::getAnyIntrinsicID(User);
      if (!vc::isAnyNonTrivialIntrinsic(IID))
        return false;
      continue;
    } else if (Opcode != Instruction::ShuffleVector &&
               Opcode != Instruction::InsertElement &&
               Opcode != Instruction::IntToPtr) {
      // These extensions are to fit the pattern of using ptrtoint:
      // %pti = ptrtoint %StructTy* %ray to i64
      // %base = insertelement <16 x i64> undef, i64 %pti, i32 0
      // %shuffle = shufflevector <16 x i64> %base, <16 x i64> undef, <16 x i32>
      // zeroinitializer %offset = add nuw nsw <16 x i64> %shuffle, <i64 0, i64
      // 4, ...>

      // Anything else is prohibited.
      return false;
    }

    // Does next processing.
    if (!processPTIsUses(*User, LocalPtrOffset))
      return false;
  }
  MaxPtrOffset += LocalPtrOffset;
  return true;
}

//__________________________________________________________________
//          Block of SElement definition.
//__________________________________________________________________
DependencyGraph::SElement::SElement(StructType *const &InTy, unsigned InIndex)
    : Ty{InTy}, Index{InIndex}, IsUnwrapped{false} {}

DependencyGraph::SElement::SElement(Type *const &InTy)
    : Ty{InTy}, IsUnwrapped{true} {}

// Returns element as structure type. Returns nullptr, if element is unwrapped
// type.
StructType *DependencyGraph::SElement::getStructTyIfPossible() const {
  return IsUnwrapped ? nullptr : getStructTy();
}

// Returns element as structure type. Generates assertion, if element is
// unwrapped type.
StructType *DependencyGraph::SElement::getStructTy() const {
  IGC_ASSERT_MESSAGE(!IsUnwrapped, "Getting unwrapped type.");
  return cast<StructType>(Ty);
}

Type *DependencyGraph::SElement::getTy() const { return Ty; }

// Returns real element type.
// If SElement is unwrapped, returns Ty itself.
// Otherwise, extracts type from the structure by Index.
Type *DependencyGraph::SElement::retrieveElemTy() const {
  return IsUnwrapped ? Ty : getStructTy()->getTypeAtIndex(Index);
}

unsigned DependencyGraph::SElement::getIndex() const {
  IGC_ASSERT_MESSAGE(!IsUnwrapped, "Getting Index of unwrapped type.");
  return Index;
}

bool DependencyGraph::SElement::isUnwrapped() const { return IsUnwrapped; }

//__________________________________________________________________
//          Block of SElementsOfType definition.
//__________________________________________________________________
DependencyGraph::SElementsOfType::SElementsOfType(unsigned Size) {
  Types.reserve(Size);
  IndicesOfTypes.reserve(Size);
};

// Automaticaly matches Types with sequential Indices.
DependencyGraph::SElementsOfType::SElementsOfType(
    const std::vector<Type *> &InTypes)
    : Types{InTypes}, IndicesOfTypes(Types.size()) {
  std::iota(IndicesOfTypes.begin(), IndicesOfTypes.end(), 0);
}

void DependencyGraph::SElementsOfType::emplace_back(Type &Ty, unsigned Index,
                                                    Value *V) {
  Types.emplace_back(&Ty);
  IndicesOfTypes.emplace_back(Index);
  Values.emplace_back(V);
}

void DependencyGraph::SElementsOfType::push_back(const value_type &Elem) {
  emplace_back(*std::get<0>(Elem), std::get<1>(Elem), std::get<2>(Elem));
}

void DependencyGraph::SElementsOfType::emplace_back(const SElement &Elem,
                                                    Value *V) {
  IGC_ASSERT_MESSAGE(
      !Elem.isUnwrapped(),
      "Element is unwrapped and cannot be placed in indices chain.");
  emplace_back(*Elem.getTy(), Elem.getIndex(), V);
}

unsigned DependencyGraph::SElementsOfType::size() const {
  const unsigned Size = Types.size();
  IGC_ASSERT_MESSAGE(Size == IndicesOfTypes.size(),
                     "Size of Types and Indices has to be the same.");
  return Size;
}

Type *DependencyGraph::SElementsOfType::getTyAt(unsigned Index) const {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return Types.at(Index);
}

unsigned DependencyGraph::SElementsOfType::getIdxAt(unsigned Index) const {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return IndicesOfTypes.at(Index);
}

Value *DependencyGraph::SElementsOfType::getValAt(unsigned Index) const {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return Values.at(Index);
}

std::pair<Type *&, unsigned &>
DependencyGraph::SElementsOfType::at(unsigned Index) {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return std::make_pair(std::ref(Types.at(Index)),
                        std::ref(IndicesOfTypes.at(Index)));
}

std::pair<Type *const &, const unsigned &>
DependencyGraph::SElementsOfType::at(unsigned Index) const {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return std::make_pair(std::ref(Types.at(Index)),
                        std::ref(IndicesOfTypes.at(Index)));
}
//__________________________________________________________________
//          Block of const_iterator definition for SElementsOfType
//__________________________________________________________________
DependencyGraph::SElementsOfType::const_iterator::const_iterator(
    const_ty_iterator TyItIn, const_idx_iterator IdxItIn,
    const_val_iterator ValItIn)
    : TyIt{TyItIn}, IdxIt{IdxItIn}, ValIt{ValItIn} {}

DependencyGraph::SElementsOfType::const_iterator::reference
DependencyGraph::SElementsOfType::const_iterator::operator*() const {
  return std::make_tuple(*TyIt, *IdxIt, *ValIt);
}

DependencyGraph::SElementsOfType::const_iterator &
DependencyGraph::SElementsOfType::const_iterator::operator++() {
  ++TyIt;
  ++IdxIt;
  ++ValIt;
  return *this;
}

DependencyGraph::SElementsOfType::const_iterator
DependencyGraph::SElementsOfType::const_iterator::operator++(int) {
  const_iterator Tmp = *this;
  ++(*this);
  return Tmp;
}

DependencyGraph::SElementsOfType::const_iterator
DependencyGraph::SElementsOfType::const_iterator::operator+(
    difference_type RHS) const {
  return const_iterator{TyIt + RHS, IdxIt + RHS, ValIt + RHS};
}

bool operator==(const DependencyGraph::SElementsOfType::const_iterator &LHS,
                const DependencyGraph::SElementsOfType::const_iterator &RHS) {
  IGC_ASSERT_MESSAGE((LHS.TyIt == RHS.TyIt) == (LHS.IdxIt == RHS.IdxIt),
                     "Iterators are not in accordance with each other.");
  return LHS.TyIt == RHS.TyIt;
};

bool operator!=(const DependencyGraph::SElementsOfType::const_iterator &LHS,
                const DependencyGraph::SElementsOfType::const_iterator &RHS) {
  return !(LHS == RHS);
};

//__________________________________________________________________
//          Block of Node definition.
//__________________________________________________________________
void DependencyGraph::Node::insertParent(Node &ParentNode) {
  auto &&[It, IsInserted] = ParentSTys.emplace(&ParentNode);
  // Insertion may not occur in similar case like insertChild.
}

void DependencyGraph::Node::insertChild(Node &ChildNode) {
  auto &&[It, IsInserted] = ChildSTys.emplace(&ChildNode);
  // Insertion may not occur if there is a dependency like : G {C, C};
}

// Checks if STy is one of the Node definitions.
bool DependencyGraph::Node::containsStruct(StructType &InSTy) const {
  return (STy == &InSTy) ? true
                         : PreviousNames.find(&InSTy) != PreviousNames.end();
}

// Sets STy as new definition of the Node.
void DependencyGraph::Node::substitute(StructType &InSTy) {
  PreviousNames.emplace(STy);
  STy = &InSTy;
}

void DependencyGraph::Node::eraseChild(Node &ChildNode) {
  size_t ElCount = ChildSTys.erase(&ChildNode);
  IGC_ASSERT(ElCount);
}

//__________________________________________________________________
//          Block of NodeMemoryManager definition.
//__________________________________________________________________
DependencyGraph::NodeMemoryManager::NodeMemoryManager(Module &M) {
  Nodes.reserve(M.getIdentifiedStructTypes().size());
}

// Allocates memory and holds pointer.
DependencyGraph::Node *
DependencyGraph::NodeMemoryManager::create(StructType &STy) {
  Nodes.emplace_back(std::make_unique<Node>(STy));
  return Nodes.back().get();
}

//
//  Retrieves base type. It tries to unwrap structures and arrays.
//
Type *getBaseTy(Type *Ty) {
  IGC_ASSERT(Ty);

  Type *BaseTy{getArrayFreeTy(Ty)};
  while (StructType *STy = dyn_cast<StructType>(BaseTy)) {
    // Empty structure.
    if (!STy->getNumElements())
      return STy;

    BaseTy = getBaseTy(*STy->element_begin());
    // Check that all elements in struct are the same type/subtype.
    if (llvm::any_of(STy->elements(), [BaseTy](Type *Elem) {
          return BaseTy != getBaseTy(Elem);
        }))
      return STy;
  }
  return BaseTy;
}

//
//  Retrieves base type of array or vector.
//
Type *getArrayFreeTy(Type *Ty) {
  IGC_ASSERT(Ty);
  while (isa<ArrayType>(Ty) || isa<VectorType>(Ty))
    Ty = Ty->getContainedType(0);
  return Ty;
}

//
//  Help function to get type-specific prefix for naming
//
const char *getTypePrefix(Type &Ty) {
  Type::TypeID ID = Ty.getTypeID();
  switch (ID) {
  case Type::VoidTyID:
    return "void";
  case Type::HalfTyID:
    return "h";
  case Type::FloatTyID:
    return "f";
  case Type::DoubleTyID:
    return "d";
  case Type::X86_FP80TyID:
    return "x86fp";
  case Type::FP128TyID:
    return "fp";
  case Type::PPC_FP128TyID:
    return "ppcfp";
  case Type::LabelTyID:
    return "l";
  case Type::MetadataTyID:
    return "m";
  case Type::X86_MMXTyID:
    return "mmx";
  case Type::TokenTyID:
    return "t";
  case Type::IntegerTyID:
    return "i";
  case Type::FunctionTyID:
    return "foo";
  case Type::StructTyID:
    return "s";
  case Type::ArrayTyID:
    return "a";
  case Type::PointerTyID:
    return "p";
  default:
    return "unnamed";
  }
}

//__________________________________________________________________
//          Block of data printing.
//__________________________________________________________________
void StructFilter::print(raw_ostream &Os) const {
  Os << "Banned structs:\n";
  for (auto *STy : BannedStructs)
    Os << "\t" << *STy << "\n";
  Os << "\n";
}

void DependencyGraph::SElement::print(raw_ostream &Os) const {
  if (Ty) {
    Os << "Ty: " << *Ty;
    if (!IsUnwrapped)
      Os << "  Index: " << Index;
  }
}

void DependencyGraph::SElementsOfType::print(raw_ostream &Os) const {
  for (auto &&[Type, Idx, Val] : *this)
    Os << "\t\tTy: " << *Type << " at pos: " << Idx << "\n";
}

void DependencyGraph::Node::dump(int Tab, raw_ostream &Os) const {
  if (!STy)
    return;
  for (int i = 0; i != Tab; ++i)
    Os << "    ";
  Tab++;
  Os << "Node: " << *STy << "\n";
  if (!ChildSTys.empty()) {
    for (int i = 0; i != Tab; ++i)
      Os << "    ";
    Os << "With children\n";
  }
  for (auto Child : ChildSTys)
    Child->dump(Tab, Os);
}

void DependencyGraph::printData(raw_ostream &Os) const {
  for (auto &&[Struct, SubTypes] : AllStructs) {
    Os << "Struct " << *Struct << " consists of:\n";
    for (auto &&[SubType, Tys] : SubTypes) {
      Os << "\t"
         << "BaseTy: " << *SubType << "\n";
      Tys.print(Os);
    }
  }
}

void DependencyGraph::print(raw_ostream &Os) const {
  Os << "\n _________________________________";
  Os << "\n/                                 \\\n";
  Os << "Data:\n";
  printData(Os);
  Os << "\nGraph:\n";
  printGraph(Os);
  Os << "\nGenerations:\n";
  printGeneration(Os);
  Os << "\\_________________________________/\n";
}

void DependencyGraph::printGraph(raw_ostream &Os) const {
  for (auto Head : Heads) {
    Os << "Head:\n";
    Head->dump(1, Os);
  }
}

void DependencyGraph::printGeneration(raw_ostream &Os) const {
  for (auto &&SplitStruct : SplitStructs) {
    Os << "Split struct: " << *SplitStruct.first << " to: \n";
    for (auto &&ChangedTo : SplitStruct.second) {
      for (auto &&Elem : ChangedTo) {
        Os << "  ";
        Elem.print(Os);
        Os << ",  ";
      }
      Os << "\n";
    }
  }
}

void Substituter::printAllAllocas(raw_ostream &Os) {
  Os << "Allocas\n";
  for (auto &&Alloca : Allocas)
    Os << *Alloca << "\n";
  Os << "\n";
}

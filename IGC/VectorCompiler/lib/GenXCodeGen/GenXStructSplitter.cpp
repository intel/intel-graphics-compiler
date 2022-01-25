/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

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
/// 1. Resolves which structs can be splitted and splits it.
///   a. Collects all structs.
///   b. Creates DependencyGraph of struct usage.
///       Which structs contain which structs.
///   c. Splits structs.
/// 2. Replaces all structures if it is possible.
///   a. Replaces allocas.
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
///
/// Limitations:
///   1. Structure contains array of complex structs.
///   2. Structure is allocated as an array.
///   3. Structure contains prohibitted structure.
///   4. Structure using instruction is not GEP, PTI, alloca.
///   5. Users of the PTI not add, insertelement, shufflevector, read/write.
///   6. Pointer of the structure goes in function (except read/write).
///   7. Pointer offset from the begging of the structure covers different
///      types.
///   8. Pointer offset from the begging of the structure covers unsequential
///      splitted structs.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/GenXDiagnostic.h"
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
static char const *getTypePrefix(Type &Ty);

// Class to do first analysis and ban all structures, which cannot be splitted
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
// Where A,B,C,D are structs; f,i - base unsplitted types.
class DependencyGraph {
public:
  //***************************************
  // Part responsible for types definition
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
    unsigned getIndex() const;
    bool isUnwrapped() const;
    void print(raw_ostream &os = llvm::errs()) const;
  };

  // Helped class contains array of Types and Indices on which Type is placed.
  // It is used for keeping elements of structure within the same subtype.
  class SElementsOfType {
    std::vector<Type *> Types;
    // vector of Indices correspondence to vector of Types
    std::vector<unsigned> IndicesOfTypes;

  public:
    SElementsOfType(unsigned Size);
    SElementsOfType(std::vector<Type *> const &InTypes);

    unsigned size() const;

    Type *getTyAt(unsigned Index) const;
    unsigned getIdxAt(unsigned Index) const;
    std::pair<Type *&, unsigned &> at(unsigned Index);
    std::pair<Type *const &, unsigned const &> at(unsigned Index) const;

    std::vector<Type *> const &getTypesArray() const { return Types; }

    using TypeIt = std::vector<Type *>::iterator;
    using IdxIt = std::vector<unsigned>::iterator;
    using TypeItConst = std::vector<Type *>::const_iterator;
    using IdxItConst = std::vector<unsigned>::const_iterator;

    // Iterator for SElementsOfType to be able to iterate over a range of
    // Types and IndicesOfTypes simultaneously.
    using value_type =
        std::pair<TypeItConst::value_type, IdxItConst::value_type>;
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
      const_iterator(TypeItConst TyItIn, IdxItConst IdxItIn);
      reference operator*() const;
      const_iterator &operator++();
      const_iterator operator++(int);
      const_iterator operator+(difference_type RHS) const;

      friend bool operator==(const const_iterator &LHS,
                             const const_iterator &RHS);
      friend bool operator!=(const const_iterator &LHS,
                             const const_iterator &RHS);

    private:
      TypeItConst TyIt;
      IdxItConst IdxIt;
    };

    TypeIt ty_begin() { return Types.begin(); }
    TypeItConst ty_begin() const { return Types.begin(); }
    TypeIt ty_end() { return Types.end(); }
    TypeItConst ty_end() const { return Types.end(); }
    IdxIt idx_begin() { return IndicesOfTypes.begin(); }
    IdxItConst idx_begin() const { return IndicesOfTypes.begin(); }
    IdxIt idx_end() { return IndicesOfTypes.end(); }
    IdxItConst idx_end() const { return IndicesOfTypes.end(); }
    const_iterator begin() const {
      return const_iterator{Types.begin(), IndicesOfTypes.begin()};
    }
    const_iterator end() const {
      return const_iterator{Types.end(), IndicesOfTypes.end()};
    }
    auto indices() const { return make_range(idx_begin(), idx_end()); }
    auto types() const { return make_range(ty_begin(), ty_end()); }

    void emplace_back(Type &Ty, unsigned Index);
    void push_back(value_type const &Elm);
    void emplace_back(SElement const &Elm);

    void print(raw_ostream &Os = llvm::errs()) const;
  };

  // The SMap is a full collection of Structs in Module within the
  // complete information about types and elements which are used in structure.
  // The STypes is a full information about elements used in Struct
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

  // The typedefs bellow are a full information about struct's transformation.
  // The idea is to generate map between Old struct's elements and new splitted
  // struct's elements Index of the element in InitSTy is index of
  // VecOfNewIndiciesDefinition and Index of the element of SplittedSTy will be
  // put according the InitSTy's element index. VecOfStructInfo looks like:
  // InitSTy : (SplittedSTy, Index) (,) <- at position=0
  //           (,)                      <- at position=1
  // FE. splitting of structs D and C:
  // D : (Df, 0)
  //     (Di, 0)
  // C : (Cf, 0)
  //     (Ci, 0)
  //     (Cf, 1)
  //     (Ci, 1)
  //     (Cf, 2) (Ci, 2)
  // And SMap in that case also contains
  // Ci : (i: <i, 0> <[5xB], 1> <Di, 2)
  // Cf : (f: <[5xA], 0> <f, 1> <Df, 2)
  // Df : (f: <A, 0>)
  // Di : (i: <B, 0>)
  // List of new structs which are on place of old unsplitted struct
  using ListOfSplittedElements = std::list<SElement>;
  // Vector of new structs elements. Position of element is corresponsible with
  // the index of this element in unsplitted structure
  using VecOfNewIndiciesDefinition = std::vector<ListOfSplittedElements>;
  // All collection of new elements
  using InfoAboutSplittedStruct =
      std::pair<StructType *, VecOfNewIndiciesDefinition>;
  // Info about all structs to be splitted.
  // Vector has been chosen to save the chronology of transformation.
  using VecOfStructInfo = std::vector<InfoAboutSplittedStruct>;

private:
  LLVMContext &Ctx;

  SMap AllStructs;
  VecOfStructInfo SplittedStructs;

  // A helped map for fast access to necessary structure transformation.
  std::unordered_map<StructType *, VecOfStructInfo::const_reverse_iterator>
      InfoToMerge;

  // Node represents an aggregative StructType with Nodes(another Structs) on
  // which it depends.
  class Node {
    StructType *STy{nullptr};

    // During the Graph transformation unsplitted stucts will be generated.
    // FE struct C_BS will be generated:
    // C_BS : { [5 x A], i, f, [5 x B], Df, Di }
    // But C_BS is the same C struct in terms of dependencies,
    // so PreviousNames set contains all previouse Node representaions.
    std::unordered_set<StructType *> PreviousNames;
    // FE C has childrens A, B, D
    std::unordered_set<Node *> ChildSTys;
    // FE A has parents D, C
    std::unordered_set<Node *> ParentSTys;

  public:
    Node(StructType &InSTy) : STy{&InSTy} {}

    bool hasParent() const { return !ParentSTys.empty(); }
    bool hasChild() const { return !ChildSTys.empty(); }
    void insertParent(Node &ParentNode);
    void insertChild(Node &ChildNode);
    void eraseChild(Node &ChildNode);
    bool isContainsStruct(StructType &InSTy) const;
    void substitute(StructType &InSTy);

    StructType *getType() const { return STy; }

    using NodeIt = std::unordered_set<Node *>::iterator;
    using NodeIt_const = std::unordered_set<Node *>::const_iterator;
    NodeIt parent_begin() { return ParentSTys.begin(); }
    NodeIt_const parent_begin() const { return ParentSTys.begin(); }
    NodeIt parent_end() { return ParentSTys.end(); }
    NodeIt_const parent_end() const { return ParentSTys.end(); }

    NodeIt child_begin() { return ChildSTys.begin(); }
    NodeIt_const child_begin() const { return ChildSTys.begin(); }
    NodeIt child_end() { return ChildSTys.end(); }
    NodeIt_const child_end() const { return ChildSTys.end(); }

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
  // Part responsible for graph handling
  //***************************************

  // Heads contains all Nodes that have no parents.
  std::vector<Node *> Heads;
  void generateGraph();

  // Helped type is used to track if Node already placed in Graph
  using NodeTracker = std::unordered_map<StructType *, Node *>;
  Node *createNode(StructType &STy, NodeTracker &Inserted);
  void processNode(Node &SNode);
  void remakeParent(Node &SNode, Node &SNodeToChange,
                    ArrayRef<Type *> NewReplaceStructs);
  void recreateGraph();

  //***************************************
  // Part responsible for records collection and handling
  //***************************************

  void setInfoAboutStructure(StructType &STy);
  void mergeStructGenerationInfo();
  StructType *
  checkAbilityToMerge(VecOfNewIndiciesDefinition const &NewSTypes) const;

public:
  DependencyGraph(Module &M, StructFilter const &Filter);
  void run();

  //***************************************
  // Part responsible for info accessing
  //***************************************
  bool isPlain(StructType &STy) const;
  bool isStructProcessed(StructType &STy) const;
  STypes const &getStructComponens(StructType &STy) const;
  Type *getPlainSubTy(StructType &STy) const;
  VecOfNewIndiciesDefinition const &
  getVecOfStructIdxMapping(StructType &STy) const;
  ListOfSplittedElements const &getElementsListOfSTyAtIdx(StructType &STy,
                                                          unsigned Idx) const;
  std::vector<Type *> getUniqueSplittedTypes(StructType &STy) const;
  //***************************************
  // Part responsible for dumping
  //***************************************
  void printData(raw_ostream &os = llvm::errs()) const;
  void print(raw_ostream &os = llvm::errs()) const;
  void printDump(raw_ostream &os = llvm::errs()) const;
  void printGeneration(raw_ostream &os) const;
};

// Class to handle all instructions that are use splitted structs.
class Substituter : public InstVisitor<Substituter> {
  LLVMContext &Ctx;
  DataLayout const &DL;
  StructFilter Filter;
  DependencyGraph Graph;
  vc::DIBuilder DIB;

  std::vector<AllocaInst *> Allocas;
  // Contains all instruction to substitute.
  using VecOfInstructionSubstitution =
      std::vector<std::pair<Instruction *, Instruction *>>;
  // Contains map of instructions and base type this instruction operates.
  using TypeToInstrMap = std::unordered_map<Type *, Instruction *>;

  bool processAlloca(AllocaInst &Alloca);

  void updateDbgInfo(ArrayRef<Type *> TypesToGenerateDI, AllocaInst &AI,
                     AllocaInst &NewAI, DbgDeclareInst &DbgDeclare);
  TypeToInstrMap generateNewAllocas(AllocaInst &OldInst);
  AllocaInst *generateAlloca(AllocaInst &AI,
                             const DependencyGraph::SElement &TyElm);
  Instruction *generateNewGEPs(GetElementPtrInst &GEPI, Type &DestSTy,
                               DependencyGraph::SElementsOfType const &IdxPath,
                               TypeToInstrMap const &NewInstr,
                               unsigned PlainTyIdx) const;

  static Optional<
      std::tuple<DependencyGraph::SElementsOfType, std::vector<Type *>>>
  getIndicesPath(GetElementPtrInst &GEPI);
  static Optional<
      std::tuple<std::vector<GetElementPtrInst *>, std::vector<PtrToIntInst *>>>
  getInstUses(Instruction &I);
  static Optional<uint64_t> processAddInst(Instruction &I, BinaryOperator &BO);

  bool processGEP(GetElementPtrInst &GEPI, TypeToInstrMap const &NewInstr,
                  VecOfInstructionSubstitution /*OUT*/ &InstToInst);
  bool processPTI(PtrToIntInst &PTI, TypeToInstrMap const &NewInstr,
                  VecOfInstructionSubstitution /*OUT*/ &InstToInst);
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
//          Block of StructFilter definition
//__________________________________________________________________

//
//  Performs checking of module for banned structs.
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
//  Returns true if STy is banned and false - if not.
//
bool StructFilter::isStructBanned(StructType &STy) const {
  return BannedStructs.find(&STy) != BannedStructs.end();
}

//
//  Checks if structure has array of complex type.
//  Returns true if has not got.
//
bool StructFilter::checkForArrayOfComplicatedStructs(StructType &STy) const {
  auto IsSequential = [](Type &Ty) {
    return Ty.isVectorTy() || Ty.isArrayTy();
  };

  return !std::any_of(
      STy.elements().begin(), STy.elements().end(), [IsSequential](Type *Elm) {
        Type *BaseTy = getArrayFreeTy(Elm);
        if (StructType *SBTy = dyn_cast<StructType>(BaseTy))
          return IsSequential(*Elm) && BaseTy == getBaseTy(SBTy);
        return false;
      });
}

//
//  Checks if structure has element of banned struct.
//  Returns true if has not got.
//
bool StructFilter::checkForElementOfBannedStruct(StructType &STy) const {
  return !std::any_of(STy.elements().begin(), STy.elements().end(),
                      [this](Type *Elm) {
                        Type *BaseTy = getArrayFreeTy(Elm);
                        if (StructType *SBTy = dyn_cast<StructType>(BaseTy))
                          return isStructBanned(*SBTy);
                        return false;
                      });
}

//__________________________________________________________________
//          Block of DependencyGraph definition
//__________________________________________________________________

//
//  Tries to get a base type of structure if structure is plain.
//  If STy is not plain then tries to use getBaseTy().
//
Type *DependencyGraph::getPlainSubTy(StructType &STy) const {
  return isPlain(STy) ? AllStructs.find(&STy)->second.begin()->first
                      : getBaseTy(&STy);
}

//
//  * Determines if structure STy is plain:
//     contains in AllStruct
//     and there is only one baseTy
//  * Works unobvious for structs like: C1: { C2 }
//
bool DependencyGraph::isPlain(StructType &STy) const {
  auto FindIt = AllStructs.find(&STy);
  return FindIt != AllStructs.end() && FindIt->second.size() < 2;
}

//
//  Checks if Struct has been processed, so info about it exists in InfoToMerge.
//  Returns true if record about struct exists, false - conversely.
//
bool DependencyGraph::isStructProcessed(StructType &STy) const {
  return InfoToMerge.find(&STy) != InfoToMerge.end();
}

//
//  Gets the element's information of the struct.
//  Assertion if struct's info has not been collected.
//
DependencyGraph::STypes const &
DependencyGraph::getStructComponens(StructType &STy) const {
  auto FindIt = AllStructs.find(&STy);
  IGC_ASSERT_MESSAGE(
      FindIt != AllStructs.end(),
      "Info about struct has to be collected before getting components.\n");
  return FindIt->second;
}

//
//  Gets vector of elements substitution of old struct with new substructs'
//  elements.
//
DependencyGraph::VecOfNewIndiciesDefinition const &
DependencyGraph::getVecOfStructIdxMapping(StructType &STy) const {
  auto FindIt = InfoToMerge.find(&STy);
  IGC_ASSERT_MESSAGE(
      FindIt != InfoToMerge.end(),
      "Struct has to be processed before getting indices mapping.\n");
  return FindIt->second->second;
}

//
//  Gets element's list which substitutes splitted struct's(STy) element at
//  index(Idx).
//
DependencyGraph::ListOfSplittedElements const &
DependencyGraph::getElementsListOfSTyAtIdx(StructType &STy,
                                           unsigned Idx) const {
  VecOfNewIndiciesDefinition const &VecOfSTy = getVecOfStructIdxMapping(STy);
  IGC_ASSERT_MESSAGE(Idx < VecOfSTy.size(),
                     "Attempt to get element out of borders.");
  return VecOfSTy.at(Idx);
}

//
// Gets unique types into which the structure STy is split.
//
std::vector<Type *>
DependencyGraph::getUniqueSplittedTypes(StructType &STy) const {
  std::unordered_set<Type *> UniqueSplittedTypes;
  // Vector is for determination of structs order.
  std::vector<Type *> UniqueSplittedTypesInOrder;
  // Gets unique substructs.
  for (auto &&ListOfBaseTys : getVecOfStructIdxMapping(STy))
    for (auto &&BaseTy : ListOfBaseTys) {
      auto [It, IsInserted] = UniqueSplittedTypes.emplace(BaseTy.getTy());
      if (IsInserted)
        UniqueSplittedTypesInOrder.push_back(BaseTy.getTy());
    }
  return UniqueSplittedTypesInOrder;
}

//
//  * By AllStructs info generates dependency graph of structs.
//  * FE generates smth like this:
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
  for (auto &&[STy, ChildrensBaseTys] : AllStructs) {
    if (Inserted.find(STy) != Inserted.end())
      // If already in graph -> skip
      continue;
    Heads.push_back(createNode(*STy, Inserted));
  }

  // During Graph creation where can be similar case: (C and D are heads)
  // C -> D ..
  // D -> A
  // So we want to remove D as it has parent=C
  // Cleanup Heads. Erase all entities with parent
  Heads.erase(
      std::remove_if(Heads.begin(), Heads.end(),
                     [](Node *HeadNode) { return HeadNode->hasParent(); }),
      Heads.end());
}

//
//  Creates the Node and places dependencies according to the Struct.
//
DependencyGraph::Node *DependencyGraph::createNode(StructType &STy,
                                                   NodeTracker &Inserted) {
  LLVM_DEBUG(dbgs() << "Creating node for struct: " << STy << "\n");
  auto FindIt = Inserted.find(&STy);
  if (FindIt != Inserted.end()) {
    // This can occur when Struct has an processed child element.
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
//  * Processes the bottom node. Assume that this node has no children
//    so all elements in this struct are plain.
//  * Splits this struct into plain substructs and recreates parents node.
//  * Eventually deletes this node from graph and releases memory as
//    after processing struct will be splitted to plain substructs
//    and parents node will no longer needed to track it.
//  * While processing nodes graph will self destruct.
//  * Info about all structs in Module (AllStructs) will be updated.
//  * Info about structs transformation (SplittedStructs) will be updated.
//
void DependencyGraph::processNode(Node &SNode) {
  // Go to the bottom of the graph.
  while (SNode.hasChild())
    processNode(**SNode.child_begin());

  LLVM_DEBUG(dbgs() << "Processing node for struct: " << *SNode.getType()
                    << "\n");
  // Splitting always gets a plain type, so graph will be changed any way
  if (StructType *OldSTy = SNode.getType()) {
    // Splitting
    STypes const &Types = getStructComponens(*OldSTy);
    // Indices of unsplitted struct will be matched with indices of elemetnts of
    // new splitted structs.
    VecOfNewIndiciesDefinition IndicesMap(OldSTy->getNumElements());

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
            Twine(OldSTyName + "." + getTypePrefix(*BaseTy) + ".splitted")
                .str());
        NewPlainType = NewPlainStruct;
        // Update AllStructs info.
        setInfoAboutStructure(*NewPlainStruct);
        // Match old elements with new elements.
        for (auto ElmIndex : enumerate(Elements.indices()))
          IndicesMap[ElmIndex.value()].emplace_back(NewPlainStruct,
                                                    ElmIndex.index());
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
    GeneratedTypesInOrder.erase(
        llvm::remove_if(GeneratedTypesInOrder, [](Type *Ty) { return !Ty; }),
        GeneratedTypesInOrder.end());

    // Do if there is splitting or simplification.
    if (GeneratedTypesInOrder.size()) {
      // Update SplittedStructs
      SplittedStructs.emplace_back(OldSTy, std::move(IndicesMap));

      // Remake parent Node.
      // As D will be splitted to Di,Df so C(parent) has to be splitted to
      // Ci,Cf. It will be done in 3 steps: 1st: Create new struct before
      // splitting: C_BS : {Ci, Cf} 2nd: Substitute struct C in Node represented
      // this struct to C_BS 3rd: When D processing will be changed, C(C_BS)
      // will be automatically splitted to Ci,Cf as Node responsible for C(C_BS)
      // will no longer have childrens.
      // In this case there will be a record in transformation info:
      //  D     -> Di, Df
      //  C     -> C_BS
      //  C_BS  -> Ci, Cf
      // so transformation C->C_BS->Ci,Cf can be merged to C->Ci,Cf
      //
      // In case of simplification: type will be substituted directly, without
      // structure.
      // F { float } C { F, float } C_BS -> { float , float }
      std::for_each(SNode.parent_begin(), SNode.parent_end(),
                    [&](Node *ParentNode) {
                      remakeParent(*ParentNode, SNode, GeneratedTypesInOrder);
                    });
    }
  }

  // Remove dependencies.
  std::for_each(SNode.parent_begin(), SNode.parent_end(),
                [&SNode](Node *ParentNode) { ParentNode->eraseChild(SNode); });
}

//
//  * Creates unsplitted struct with new element's types generated from child
//    Node.
//  * As D splitted to Di,Df, structure C has to change element D to Di,Df and
//    splits later. So after this function in Node responsible for C, will
//    be placed new structure C_BS and later C_BS will be processed.
//  * SNode - current parent node to be changed
//  * SNodeToChange - child node that already has been changed
//
void DependencyGraph::remakeParent(Node &SNode, Node &SNodeToChange,
                                   ArrayRef<Type *> NewReplaceTypes) {
  LLVM_DEBUG(dbgs() << "Recreating parent node: " << *SNode.getType()
                    << "\n\tChild node: " << *SNodeToChange.getType() << "\n");
  StructType *CurrentS = SNode.getType();
  StringRef CurrentSName = CurrentS->getName();
  unsigned const NumElements = CurrentS->getNumElements();
  unsigned const NewMaxSize = NumElements + NewReplaceTypes.size() - 1;
  std::vector<Type *> NewElements;
  NewElements.reserve(NewMaxSize);
  // First create an empty struct
  // Later setBody with elements. It is for completing VecOfStructInfo
  StructType *BeforeSplitingS = StructType::create(
      CurrentS->getContext(), Twine(CurrentSName + "_BS").str());
  VecOfNewIndiciesDefinition NewIndices(NumElements);
  unsigned Index{0};
  unsigned ExpandIndicies{0};
  for (auto &&Elm : CurrentS->elements()) {
    if (StructType *SElm = dyn_cast<StructType>(Elm);
        SElm && SNodeToChange.isContainsStruct(*SElm)) {
      // If element of structure is splitted element, then we need to replace
      // this element with new.
      for (auto &&NewSTy : NewReplaceTypes) {
        NewElements.emplace_back(NewSTy);
        NewIndices[Index].emplace_back(BeforeSplitingS,
                                       Index + ExpandIndicies++);
      }
      // The Index will be inc, so there is no need of extra offset
      --ExpandIndicies;
    } else {
      // If element of structure is not changed, then just copies info about it
      // and places right indices.
      NewElements.emplace_back(Elm);
      NewIndices[Index].emplace_back(BeforeSplitingS, Index + ExpandIndicies);
    }
    ++Index;
  }

  BeforeSplitingS->setBody(NewElements);

  // Updates AllStructs and SplittedStructs info.
  setInfoAboutStructure(*BeforeSplitingS);
  SplittedStructs.emplace_back(CurrentS, std::move(NewIndices));

  // Substitutes structure in Node
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
  unsigned Index{0};
  unsigned const NumElements = STy.getNumElements();
  // SElementsOfType reservs memory to avoid reallocations and easy access
  // Will be more memory overhead
  for (auto &&Elm : STy.elements()) {
    Type *BaseTy = getBaseTy(Elm);
    // BaseTy can be structure in AllStructs, so we get info from AllStructs
    if (StructType *SBTy = dyn_cast<StructType>(BaseTy))
      BaseTy = getPlainSubTy(*SBTy);

    auto FindIt = BaseTypes.find(BaseTy);
    if (FindIt == BaseTypes.end()) {
      // If there is no entity with baseTy, creates it with preallocated array
      auto [It, IsInserted] =
          BaseTypes.emplace(BaseTy, SElementsOfType{NumElements});
      It->second.emplace_back(*Elm, Index++);
    } else
      // If there is an entity with baseTy, inserts it into array
      FindIt->second.emplace_back(*Elm, Index++);
  }

  auto [It, IsInserted] = AllStructs.emplace(&STy, std::move(BaseTypes));
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
  for (auto It = SplittedStructs.rbegin(), End = SplittedStructs.rend();
       It != End; ++It) {
    StructType &SToMerge = *It->first;
    VecOfNewIndiciesDefinition &InfoAboutS = It->second;

    if (StructType *SToMergeWith = checkAbilityToMerge(InfoAboutS)) {
      LLVM_DEBUG(dbgs() << "Able to merge: " << SToMerge << "\n\tWith "
                        << *SToMergeWith << "\n");

      VecOfNewIndiciesDefinition const &InfoAboutTemporaryS =
          getVecOfStructIdxMapping(*SToMergeWith);
      // Every element of the structure SToMerge will be substituted with
      // element from the structure SToMergeWith and/or new elements from
      // SToMergeWith will be placed in SToMerge.
      for (ListOfSplittedElements &ElementsList : InfoAboutS) {
        for (SElement &Element : ElementsList) {
          IGC_ASSERT_MESSAGE(!Element.isUnwrapped(),
                             "Attempt to merge unwrapped type.");
          IGC_ASSERT_MESSAGE(Element.getIndex() < InfoAboutTemporaryS.size(),
                             "Attempt to get element out of borders.");
          ListOfSplittedElements const &NewElement =
              InfoAboutTemporaryS.at(Element.getIndex());

          auto EIt = NewElement.rbegin();
          // Changes current element and, if on this 'Element.Index' lots of new
          // elements are to be placed, extends list with new elements.
          // Pushes front new element not to invalidate iterations.
          // Iterates from end to begin (rbegin to rend) to keep order of
          // elements.
          // FE, merges information
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
StructType *DependencyGraph::checkAbilityToMerge(
    VecOfNewIndiciesDefinition const &NewSTypes) const {
  IGC_ASSERT_MESSAGE(NewSTypes.size(), "Merging empty structs.");
  IGC_ASSERT_MESSAGE(NewSTypes.begin()->size(), "Merging empty structs.");
  auto FirstElm = NewSTypes.begin()->begin();
  if (FirstElm->isUnwrapped())
    return nullptr;
  StructType *STyToCheck = FirstElm->getStructTy();

  // Checks that all split structs are same. It is the main criteria for
  // iterations of splitting to be merged.
  bool AreSameStructs =
      std::all_of(NewSTypes.begin(), NewSTypes.end(),
                  [STyToCheck](auto &&SplittedElements) {
                    return std::all_of(
                        SplittedElements.begin(), SplittedElements.end(),
                        [STyToCheck](auto &&Element) {
                          return Element.getStructTyIfPossible() == STyToCheck;
                        });
                  });

  if (AreSameStructs && isStructProcessed(*STyToCheck))
    return STyToCheck;
  return nullptr;
}

//
//  Constructor gets all initial information about structures in Module.
//
DependencyGraph::DependencyGraph(Module &M, StructFilter const &Filter)
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
//          Block of Substituter definition
//__________________________________________________________________

//
//  Collects all information of structs, allocas and launches struct splittting,
//    based on this information.
//
Substituter::Substituter(Module &M)
    : Ctx{M.getContext()}, DL{M.getDataLayout()}, Filter{M}, Graph{M, Filter},
      DIB{M} {
  Graph.run();
  LLVM_DEBUG(Graph.print(dbgs()));

  // visit should be after graph processing
  visit(M);
}

//
//  Collects all allocas that allocate memory for structure to split.
//
void Substituter::visitAllocaInst(AllocaInst &AI) {
  if (StructType *STy = dyn_cast<StructType>(AI.getAllocatedType()))
    if (Graph.isStructProcessed(*STy)) {
      LLVM_DEBUG(dbgs() << "Collecting alloca to replace: " << AI << "\n");
      // In case if struct S marked to be splitted, but there is
      // alloca [5 x %struct.C] then skip.
      // Gets only allocas which will be splitted
      // InfoToMerge contains this info
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

  for (auto &&ListOfBaseTys : Graph.getVecOfStructIdxMapping(STy))
    for (auto &&BaseTy : ListOfBaseTys) {
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
// Generates new alloca for TyElm to replace AI - the old one.
// For TyElm generates DI.
//
AllocaInst *
Substituter::generateAlloca(AllocaInst &AI,
                            const DependencyGraph::SElement &TyElm) {
  IRBuilder<> IRB{&AI};

  Type *NewTy = TyElm.getTy();
  AllocaInst &NewAI = *IRB.CreateAlloca(
      NewTy, 0, AI.getName() + "." + getTypePrefix(*getBaseTy(NewTy)));
  NewAI.setAlignment(IGCLLVM::getAlign(AI));

  DbgDeclareInst *DbgDeclare = getDbgDeclare(AI);
  if (!DbgDeclare)
    return &NewAI;

  // *NewTy can be:
  // - base type, non-split struct (also base type).
  // - split struct (for each element to generate DI).
  std::vector<Type *> TypesToGenerateDI;
  if (TyElm.isUnwrapped()) {
    TypesToGenerateDI.push_back(NewTy);
  } else {
    StructType &STy = *TyElm.getStructTy();
    llvm::copy(STy.elements(), std::back_inserter(TypesToGenerateDI));
  }
  updateDbgInfo(TypesToGenerateDI, AI, NewAI, *DbgDeclare);

  return &NewAI;
}

//
// Help-function to go other the list and check if Type with possition Idx is
// found. FirstMatch is for search regardless of index. Finds the firts match of
// type.
//
static bool isElementInList(const DependencyGraph::ListOfSplittedElements &List,
                            Type *Ty, unsigned Idx, bool FirstMatch) {
  auto ListIt =
      std::find_if(List.begin(), List.end(),
                   [Idx, Ty, FirstMatch](const DependencyGraph::SElement &Elm) {
                     // FirstMatch is needed only for searching in
                     // substructures, where the first occurrence of type is to
                     // be found.
                     if (FirstMatch)
                       return Elm.getTy() == Ty;
                     // If element is unwrapped -> only checks on types match.
                     // If element is not unwrapped -> additionaly checks Index.
                     return Elm.getTy() == Ty &&
                            (Elm.isUnwrapped() ||
                             !Elm.isUnwrapped() && Elm.getIndex() == Idx);
                   });
  return ListIt != List.end();
}

//
// Finds record in IdxMap of Ty placed on Idx position.
// FirstMatch is for search regardless of index.
//
static auto
findRecord(const DependencyGraph::VecOfNewIndiciesDefinition &IdxMap, Type *Ty,
           unsigned Idx, bool FirstMatch) {
  auto FindIt = std::find_if(
      IdxMap.begin(), IdxMap.end(), [&Ty, &Idx, FirstMatch](auto &&List) {
        return isElementInList(List, Ty, Idx, FirstMatch);
      });
  IGC_ASSERT_MESSAGE(FindIt != IdxMap.end(), "Record has to be found!");
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

  using VecOfNewIndiciesDefinition =
      DependencyGraph::VecOfNewIndiciesDefinition;
  using SElement = DependencyGraph::SElement;
  StructType &STy = *cast<StructType>(AI.getAllocatedType());
  const VecOfNewIndiciesDefinition &IdxMap =
      Graph.getVecOfStructIdxMapping(STy);

  // For each element of the new structure generates DI.
  for (auto &&TypeEnum : enumerate(TypesToGenerateDI)) {
    unsigned Offset{0};

    // Type and position of the element.
    Type *TyToGenDI = TypeEnum.value();
    unsigned ElmIdx = TypeEnum.index();

    auto getOffsetInBits = [this](unsigned Idx, StructType *STy) {
      return DL.getStructLayout(STy)->getElementOffsetInBits(Idx);
    };

    // Finds original type from which element came.
    auto VecIt = findRecord(IdxMap, NewTy, ElmIdx, false);
    unsigned IdxOfOrigStruct = VecIt - IdxMap.begin();
    Type *OrigTy = STy.getElementType(IdxOfOrigStruct);

    // If it is processed Structure, offset of the element in this substructure
    // has to be calculated.
    if (StructType *OrigSTy = dyn_cast<StructType>(OrigTy);
        OrigSTy && Graph.isStructProcessed(*OrigSTy)) {
      const VecOfNewIndiciesDefinition &SubSIdxMap =
          Graph.getVecOfStructIdxMapping(*OrigSTy);
      // Offset from OrigStruct + Offset from SubStruct.
      auto SubSVecIt =
          findRecord(SubSIdxMap, TyToGenDI, /*does not matter*/ 0, true);
      unsigned IdxOfSubStruct = SubSVecIt - SubSIdxMap.begin();
      Offset = getOffsetInBits(IdxOfOrigStruct, &STy) +
               getOffsetInBits(IdxOfSubStruct, OrigSTy);
    } else
      // Offset from OrigStruct
      Offset = getOffsetInBits(IdxOfOrigStruct, &STy);

    auto FragExpr = DIExpression::createFragmentExpression(
        &Expr, Offset, DL.getTypeAllocSizeInBits(TyToGenDI));
    IGC_ASSERT_MESSAGE(FragExpr.hasValue(), "Failed to create new expression");

    Instruction &NewDbgDeclare =
        *DIB.createDbgDeclare(NewAI, Var, *FragExpr.getValue(), DbgLoc, AI);
    LLVM_DEBUG(dbgs() << "New dbg.declare is created: " << NewDbgDeclare
                      << '\n';);
  }
}

//
//  Returns Instruction responsible for processing Type.
//
static Instruction *findProperInstruction(
    Type *Ty, std::unordered_map<Type *, Instruction *> const &NewInstr) {
  auto FindInstrIt = NewInstr.find(Ty);
  IGC_ASSERT_MESSAGE(
      FindInstrIt != NewInstr.end(),
      "Cannot find instruction according to splitted structure type.");
  return FindInstrIt->second;
}

//
//  Creating new GEPI instruction.
//  GEPI - instruction to replace.
//  PlainType - the result type of new gep work.
//  IdxPath - the sequence of indices to recive needed type.
//  NewInstr - instruction map to set proper uses.
//  PlainTyIdx - index of the first plain type..
//
Instruction *
Substituter::generateNewGEPs(GetElementPtrInst &GEPI, Type &PlainType,
                             DependencyGraph::SElementsOfType const &IdxPath,
                             TypeToInstrMap const &NewInstr,
                             unsigned PlainTyIdx) const {
  using SElement = DependencyGraph::SElement;
  using ListOfSplittedElements = DependencyGraph::ListOfSplittedElements;
  using SElementsOfType = DependencyGraph::SElementsOfType;
  LLVM_DEBUG(dbgs() << "Generating GEP to replace: " << GEPI << "\n");
  IGC_ASSERT_MESSAGE(PlainTyIdx <= IdxPath.size(),
                     "Index of the plain type is out of boundaries.");

  SElementsOfType LocalIdxPath{IdxPath.size()};

  // Generates new indices path till PlainTyIdx.
  std::for_each(IdxPath.begin(), IdxPath.begin() + PlainTyIdx,
                [&PlainType, &LocalIdxPath, this](auto &&Elm) {
                  auto [Ty, Idx] = Elm;
                  StructType *STy = cast<StructType>(Ty);
                  ListOfSplittedElements const &ListOfPossibleTypes =
                      Graph.getElementsListOfSTyAtIdx(*STy, Idx);

                  auto FindIt = std::find_if(
                      ListOfPossibleTypes.begin(), ListOfPossibleTypes.end(),
                      [&PlainType](auto &&PosElm) {
                        Type *PossibleTy = PosElm.getTy();
                        // For now getBaseTy is similar to getting type from
                        // structure info, but in further it may be different
                        // while processing arrays and vectors of structures.
                        return getBaseTy(PossibleTy) == &PlainType;
                      });
                  IGC_ASSERT_MESSAGE(FindIt != ListOfPossibleTypes.end(),
                                     "No substitution type.");
                  // Skip indices if it gives unwrapped type.
                  if (!FindIt->isUnwrapped())
                    LocalIdxPath.emplace_back(*FindIt);
                });
  std::copy(IdxPath.begin() + PlainTyIdx, IdxPath.end(),
            std::back_inserter(LocalIdxPath));

  // If Size == 0 then we do not need to create a GEP. Just find proper previous
  // instruction.
  Instruction *ToInsert = findProperInstruction(&PlainType, NewInstr);
  unsigned const Size = LocalIdxPath.size();
  if (!Size) {
    LLVM_DEBUG(dbgs() << "Instruction has been reused: " << *ToInsert << "\n");
    return ToInsert;
  }

  // Generates new IdxList for instruction.
  std::vector<Value *> IdxList;
  IdxList.reserve(Size + 1);
  IdxList.emplace_back(*GEPI.idx_begin());
  for (unsigned i = 0; i != Size; ++i)
    // TODO how to chose i32 or i64 for indices value?
    IdxList.emplace_back(
        ConstantInt::get(Ctx, APInt(32, LocalIdxPath.getIdxAt(i))));

  Type *Inserted = LocalIdxPath.getTyAt(0);

  IRBuilder<> IRB{&GEPI};
  Instruction *NewGEP = cast<Instruction>(
      IRB.CreateGEP(Inserted, ToInsert, IdxList, GEPI.getName() + ".splitted"));

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
  auto [UsesGEP, UsesPTI] = std::move(InstUses.getValue());

  TypeToInstrMap NewInstrs = generateNewAllocas(Alloca);

  VecOfInstructionSubstitution InstToInst;

  for (GetElementPtrInst *GEP : UsesGEP)
    if (!processGEP(*GEP, NewInstrs, InstToInst))
      return false;
  for (PtrToIntInst *PTI : UsesPTI)
    if (!processPTI(*PTI, NewInstrs, InstToInst))
      return false;

  for (auto [InstToReplace, ToInst] : InstToInst)
    InstToReplace->replaceAllUsesWith(ToInst);

  return true;
}

//
// Retrieves information of Type gotten within each index access.
// FE:
//  %a = gep C, 0, 4, 0
//  (C, 4) -> D
//  (D, 0) -> A
//
Optional<std::tuple<DependencyGraph::SElementsOfType, std::vector<Type *>>>
Substituter::getIndicesPath(GetElementPtrInst &GEPI) {
  unsigned const Size = GEPI.getNumIndices() - 1;
  DependencyGraph::SElementsOfType IdxPath{Size};
  std::vector<Type *> GottenTypeArr;
  GottenTypeArr.reserve(Size);

  // Skip first operator as it always 0 to rename poiterTy and get to structTy
  Type *CurrentType = GEPI.getSourceElementType();
  for (auto It = GEPI.idx_begin() + 1, End = GEPI.idx_end(); It != End; ++It) {
    Value *VIdx = *It;
    if (Constant *CIdx = dyn_cast<Constant>(VIdx)) {
      APInt const &Int = CIdx->getUniqueInteger();
      // Naive assumption that all indices are unsigned greater then zero and
      // scalar
      uint64_t Idx = Int.getZExtValue();

      // This approach can fail in case of dynamic indices.
      // To use table in that case.
      Type *GottenType{nullptr};
      if (CurrentType->isVectorTy() || CurrentType->isArrayTy())
        GottenType = CurrentType->getContainedType(0);
      else
        GottenType = CurrentType->getContainedType(Idx);

      IdxPath.emplace_back(*CurrentType, Idx);
      GottenTypeArr.emplace_back(GottenType);
      CurrentType = GottenType;
    } else {
      LLVM_DEBUG(dbgs() << "WARN:: Non constant indices do not supported!\n");
      return None;
    }
  }
  return std::make_tuple(std::move(IdxPath), std::move(GottenTypeArr));
}

//
// Gets GEP and PTI users of instruction I.
//
Optional<
    std::tuple<std::vector<GetElementPtrInst *>, std::vector<PtrToIntInst *>>>
Substituter::getInstUses(Instruction &I) {
  // Checks That users of Instruction are appropriate.
  std::vector<GetElementPtrInst *> UsesGEP;
  std::vector<PtrToIntInst *> UsesPTI;
  UsesGEP.reserve(I.getNumUses());
  UsesPTI.reserve(I.getNumUses());
  for (auto const &U : I.uses())
    if (GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(U.getUser()))
      UsesGEP.push_back(I);
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
        return None;
      }
    } else {
      LLVM_DEBUG(
          dbgs()
          << "WARN:: Struct is used where it cannot be used!\n\tInstruction: "
          << *U.getUser() << "\n");
      return None;
    }
  return std::make_tuple(std::move(UsesGEP), std::move(UsesPTI));
}

//
//  * Generates new instructions that use splitted struct.
//  * In case if result of old instruction is a struct to be splitted, then
//    generates new instructions and results of them are splitted structs.
//  * In case if result of old instruction is unsplitted struct or data,
//    when just generate new instruction with proper access indecies.
//  * FE1: %d = gep C, 0, 4
//     -> %di = gep Ci, 0, 2
//     -> %df = gep Cf, 0, 1
//  * FE2: %a = gep C, 0, 4, 0
//     -> %a = gep Cf, 0, 2, 0
//
bool Substituter::processGEP(GetElementPtrInst &GEPI,
                             TypeToInstrMap const &NewInstr,
                             VecOfInstructionSubstitution /*OUT*/ &InstToInst) {
  LLVM_DEBUG(dbgs() << "Processing uses of instruction: " << GEPI << "\n");
  using SElement = DependencyGraph::SElement;
  using ListOfSplittedElements = DependencyGraph::ListOfSplittedElements;
  auto IndicesPath = getIndicesPath(GEPI);
  if (!IndicesPath)
    return false;
  auto [IdxPath, GottenTypeArr] = std::move(IndicesPath.getValue());
  unsigned const Size = GottenTypeArr.size();
  IGC_ASSERT_MESSAGE(
      IdxPath.size() == Size,
      "IdxPath and GottenTypeArr must be consistent with each other.");

  // Find the first index of plain type.
  // All indices after PlaintTyIdx can be just copied.
  auto FindIt = std::find_if(GottenTypeArr.begin(), GottenTypeArr.end(),
                             [this](Type *Ty) {
                               StructType *STy = dyn_cast<StructType>(Ty);
                               return !STy || !Graph.isStructProcessed(*STy);
                             });
  unsigned PlainTyIdx = FindIt - GottenTypeArr.begin();

  if (PlainTyIdx == Size) {
    // Case of FE1
    auto InstUses = getInstUses(GEPI);
    if (!InstUses)
      return false;
    auto [UsesGEP, UsesPTI] = std::move(InstUses.getValue());

    // That means that we getting splitted struct so we need to create GEPs.
    // STyToBeSplitted is the result of instruction.
    Type *PrevType = IdxPath.getTyAt(Size - 1);
    unsigned Idx = IdxPath.getIdxAt(Size - 1);
    StructType *STyToBeSplitted = cast<StructType>(PrevType);
    ListOfSplittedElements const &ListOfPossibleTypes =
        Graph.getElementsListOfSTyAtIdx(*STyToBeSplitted, Idx);

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
      if (!processPTI(*PTI, NewInstructions, InstToInst))
        return false;
  } else {
    Type *PrevType = IdxPath.getTyAt(PlainTyIdx);
    unsigned Idx = IdxPath.getIdxAt(PlainTyIdx);
    StructType *STyToBeSplitted = cast<StructType>(PrevType);
    IGC_ASSERT_MESSAGE(
        Graph.getElementsListOfSTyAtIdx(*STyToBeSplitted, Idx).size() == 1,
        "Access to element of Struct does not get unsplitted type.");
    Type *PlainType = getBaseTy(GottenTypeArr[PlainTyIdx]);
    Instruction *ToInsert =
        generateNewGEPs(GEPI, *PlainType, IdxPath, NewInstr, PlainTyIdx + 1);
    InstToInst.emplace_back(cast<Instruction>(&GEPI), ToInsert);
  }
  return true;
}

//
//  Checks if accessing by ptr covers one unsplitted block and substitutes
//  struct. Tracks max offset of ptr until ptr goes to function. If function is
//  read/write, then check if max offset lies within unsplited block. If it
//  does, then substitutes struct. Overwise we cannot split struct.
//
bool Substituter::processPTI(PtrToIntInst &PTI, TypeToInstrMap const &NewInstr,
                             VecOfInstructionSubstitution /*OUT*/ &InstToInst) {

  StructType *STy = dyn_cast<StructType>(
      PTI.getPointerOperand()->getType()->getPointerElementType());
  IGC_ASSERT_MESSAGE(STy, "Operand of PTI has to be StructType.");

  uint64_t MaxPtrOffset{0};
  if (!processPTIsUses(PTI, MaxPtrOffset))
    return false;

  // If MaxPtrOffset covers elements, which will be laid sequitially within one
  // new struct, then we can substiture PTI with new PTI;
  unsigned IdxOfOldElm{0};
  Type *SplitTy{nullptr};
  unsigned IdxOfSplittedStructElm{0};
  DependencyGraph::VecOfNewIndiciesDefinition const &IdxMapping =
      Graph.getVecOfStructIdxMapping(*STy);
  for (auto &&Elm : STy->elements()) {
    IGC_ASSERT_MESSAGE(IdxOfOldElm < IdxMapping.size(),
                       "Attempt to get element out of borders.");
    DependencyGraph::ListOfSplittedElements const &ListOfElements =
        IdxMapping.at(IdxOfOldElm++);
    for (auto &&NewElm : ListOfElements) {
      if (!SplitTy) {
        // The head of sequential check.
        SplitTy = NewElm.getTy();
        IdxOfSplittedStructElm = 0;
        if (!NewElm.isUnwrapped() && NewElm.getIndex()) {
          // A {i32, float}
          // Af {float} <- prohibited
          // Ai {i32}   <- allowed
          LLVM_DEBUG(dbgs() << "WARN:: Struct (" << *STy
                            << ") cannot be splitted as the first element of "
                               "the splitted struct has to be the first "
                               "element of the original struct!\n");
          return false;
        }
      } else {
        if (NewElm.isUnwrapped()) {
          // Ai { Bi, i32 };
          // ptr = &Ai;
          // ptr += sizeof(bi);
          // Prohibeted as poiter covers unsequential types.
          // Unwrapped type can be only an the first position.
          LLVM_DEBUG(dbgs()
                     << "WARN:: Struct (" << *STy
                     << ") cannot be splitted as pointer offset covers "
                        "unsequential types and base type("
                     << *NewElm.getTy() << ") can be only in the begining.\n");
          return false;
        }
        if (NewElm.getTy() != SplitTy) {
          // A {i32, i32, float}; Offset = 8byte
          // Prohibited as offset covers i32 and float
          LLVM_DEBUG(dbgs() << "WARN:: Struct (" << *STy
                            << ") cannot be splitted as pointer offset covers "
                               "different splitted types.\n");
          return false;
        }
        if (NewElm.getIndex() != ++IdxOfSplittedStructElm) {
          LLVM_DEBUG(dbgs() << "WARN:: Struct (" << *STy
                            << ") cannot be splitted as pointer offset covers "
                               "unsequential types.\n");
          return false;
        }
      }
    }

    if (!MaxPtrOffset)
      break;
    uint64_t const SizeOfElm = DL.getTypeAllocSizeInBits(Elm) / genx::ByteBits;
    MaxPtrOffset = SizeOfElm > MaxPtrOffset ? 0 : MaxPtrOffset - SizeOfElm;
  }

  Instruction *ToInsert = findProperInstruction(getBaseTy(SplitTy), NewInstr);

  IRBuilder<> IRB{&PTI};
  Value *NewPTI =
      IRB.CreatePtrToInt(ToInsert, PTI.getType(), PTI.getName() + ".splitted");

  LLVM_DEBUG(dbgs() << "New Instruction has been created: " << *NewPTI << "\n");
  InstToInst.emplace_back(cast<Instruction>(&PTI), cast<Instruction>(NewPTI));
  return true;
}

//
// Callculates offset after add instruction.
//
Optional<uint64_t> Substituter::processAddInst(Instruction &User,
                                               BinaryOperator &BO) {
  // Do Ptr Offset calculation.
  uint64_t LocalPtrOffset{0};
  Value *V0 = BO.getOperand(0);
  // If the one of operands is the Instruction then the other is ptr offset.
  // It can be vector or scalar.
  // "add V 5" or "add 5 V"
  Value *ToCalculateOffset =
      dyn_cast<Instruction>(V0) != &User ? V0 : BO.getOperand(1);
  Constant *ConstantOffsets = dyn_cast<Constant>(ToCalculateOffset);
  if (!ConstantOffsets) {
    LLVM_DEBUG(dbgs() << "WARN:: Calculation of the pointer offset has to "
                         "be staticly known\n. Bad instruction: "
                      << BO << "\n");
    return None;
  }
  Type *OffsetTy = ToCalculateOffset->getType();
  if (OffsetTy->isVectorTy()) {
    unsigned const Width =
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
    return None;
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
  for (auto const &U : I.uses()) {
    Instruction *User = dyn_cast<Instruction>(U.getUser());
    if (User->getOpcode() == Instruction::FAdd ||
        User->getOpcode() == Instruction::Add) {
      BinaryOperator *BO = dyn_cast<BinaryOperator>(User);
      auto Offset = processAddInst(I, *BO);
      if (!Offset)
        return false;
      LocalPtrOffset = std::max(LocalPtrOffset, Offset.getValue());
    } else if (GenXIntrinsic::isGenXIntrinsic(User) &&
               User->mayReadOrWriteMemory()) {
      // We can read/write from/to unsplitted block.
      continue;
    } else if (User->getOpcode() != Instruction::ShuffleVector &&
               User->getOpcode() != Instruction::InsertElement) {
      // There extensions are to fit the pattern of using ptrtoint:
      // %pti = ptrtoint %StructTy* %ray to i64
      // %base = insertelement <16 x i64> undef, i64 %pti, i32 0
      // %shuffle = shufflevector <16 x i64> %base, <16 x i64> undef, <16 x i32>
      // zeroinitializer %offset = add nuw nsw <16 x i64> %shuffle, <i64 0, i64
      // 4, ...>

      // Anything else is prohibited.
      return false;
    }

    // Do next processings
    if (!processPTIsUses(*User, LocalPtrOffset))
      return false;
  }
  MaxPtrOffset += LocalPtrOffset;
  return true;
}

//__________________________________________________________________
//          Block of SElement definition
//__________________________________________________________________
DependencyGraph::SElement::SElement(StructType *const &InTy,
                                    unsigned InIndex)
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

unsigned DependencyGraph::SElement::getIndex() const {
  IGC_ASSERT_MESSAGE(!IsUnwrapped, "Getting Index of unwrapped type.");
  return Index;
}

bool DependencyGraph::SElement::isUnwrapped() const {
  return IsUnwrapped;
}

//__________________________________________________________________
//          Block of SElementsOfType definition
//__________________________________________________________________
DependencyGraph::SElementsOfType::SElementsOfType(unsigned Size) {
  Types.reserve(Size);
  IndicesOfTypes.reserve(Size);
};

// Automaticaly matches Types with sequential Indices
DependencyGraph::SElementsOfType::SElementsOfType(
    std::vector<Type *> const &InTypes)
    : Types{InTypes}, IndicesOfTypes(Types.size()) {
  std::iota(IndicesOfTypes.begin(), IndicesOfTypes.end(), 0);
}

void DependencyGraph::SElementsOfType::emplace_back(Type &Ty, unsigned Index) {
  Types.emplace_back(&Ty);
  IndicesOfTypes.emplace_back(Index);
}

void DependencyGraph::SElementsOfType::push_back(value_type const &Elm) {
  emplace_back(*Elm.first, Elm.second);
}

void DependencyGraph::SElementsOfType::emplace_back(SElement const &Elm) {
  IGC_ASSERT_MESSAGE(
      !Elm.isUnwrapped(),
      "Element is unwrapped and cannot be placed in indices chain.");
  emplace_back(*Elm.getTy(), Elm.getIndex());
}

unsigned DependencyGraph::SElementsOfType::size() const {
  unsigned const Size = Types.size();
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

std::pair<Type *&, unsigned &>
DependencyGraph::SElementsOfType::at(unsigned Index) {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return std::make_pair(std::ref(Types.at(Index)),
                        std::ref(IndicesOfTypes.at(Index)));
}

std::pair<Type *const &, unsigned const &>
DependencyGraph::SElementsOfType::at(unsigned Index) const {
  IGC_ASSERT_MESSAGE(Index < size(), "Attempt to get element out of borders.");
  return std::make_pair(std::ref(Types.at(Index)),
                        std::ref(IndicesOfTypes.at(Index)));
}
//__________________________________________________________________
//          Block of const_iterator definition for SElementsOfType
//__________________________________________________________________
DependencyGraph::SElementsOfType::const_iterator::const_iterator(
    TypeItConst TyItIn, IdxItConst IdxItIn)
    : TyIt{TyItIn}, IdxIt{IdxItIn} {}

DependencyGraph::SElementsOfType::const_iterator::reference
    DependencyGraph::SElementsOfType::const_iterator::operator*() const {
  return std::make_pair(*TyIt, *IdxIt);
}

DependencyGraph::SElementsOfType::const_iterator &
DependencyGraph::SElementsOfType::const_iterator::operator++() {
  ++TyIt;
  ++IdxIt;
  return *this;
}

DependencyGraph::SElementsOfType::const_iterator
DependencyGraph::SElementsOfType::const_iterator::operator++(int) {
  const_iterator Tmp = *this;
  ++(*this);
  return Tmp;
}

DependencyGraph::SElementsOfType::const_iterator
DependencyGraph::SElementsOfType::const_iterator::
operator+(difference_type RHS) const {
  return const_iterator{TyIt + RHS, IdxIt + RHS};
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
//          Block of Node definition
//__________________________________________________________________
void DependencyGraph::Node::insertParent(Node &ParentNode) {
  auto &&[It, IsInserted] = ParentSTys.emplace(&ParentNode);
  // Insertion may not occur in simillar case like insertChild
}

void DependencyGraph::Node::insertChild(Node &ChildNode) {
  auto &&[It, IsInserted] = ChildSTys.emplace(&ChildNode);
  // Insertion may not occur if there is a dependency like : G {C, C};
}

// Checks of STy is previouse definition of the Node.
bool DependencyGraph::Node::isContainsStruct(StructType &InSTy) const {
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
//          Block of NodeMemoryManager definition
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
    // If empty struct
    if (!STy->getNumElements())
      return STy;

    BaseTy = getBaseTy(*STy->element_begin());
    // Check that all elements in struct are the same type/subtype
    for (auto &&Elm : STy->elements())
      if (BaseTy != getBaseTy(Elm))
        return STy;
  }
  return BaseTy;
}

//
//  Retrieves base type of the array
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
char const *getTypePrefix(Type &Ty) {
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
//          Block of data printing
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
  for (unsigned i = 0, end = Types.size(); i != end; ++i)
    Os << "\t\tTy: " << *(Types[i]) << " at pos: " << IndicesOfTypes[i] << "\n";
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
    Os << "With childs\n";
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
  printDump(Os);
  Os << "\nGenerations:\n";
  printGeneration(Os);
  Os << "\\_________________________________/\n";
}

void DependencyGraph::printDump(raw_ostream &Os) const {
  for (auto Head : Heads) {
    Os << "Head:\n";
    Head->dump(1, Os);
  }
}

void DependencyGraph::printGeneration(raw_ostream &Os) const {
  for (auto &&SplittedStruct : SplittedStructs) {
    Os << "Splitted struct: " << *SplittedStruct.first << " to: \n";
    for (auto &&ChangedTo : SplittedStruct.second) {
      for (auto &&Elm : ChangedTo) {
        Os << "  ";
        Elm.print(Os);
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

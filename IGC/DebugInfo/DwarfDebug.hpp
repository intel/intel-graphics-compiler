/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfDebug.h
///////////////////////////////////////////////////////////////////////////////
#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Allocator.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

// TODO: remove wrapper since we don't need LLVM 7 support
#include "llvmWrapper/IR/IntrinsicInst.h"

#include "DIE.hpp"
#include "LexicalScopes.hpp"
#include "VISAModule.hpp"

#include "EmitterOpts.hpp"

#include "Probe/Assertion.h"
#include <set>

namespace llvm {
class MCSection;
}

bool isUnsignedDIType(IGC::DwarfDebug *DD, llvm::DIType *Ty);
const llvm::Instruction *getNextInst(const llvm::Instruction *start);

// As per SPIRV spec, storage class value 4 corresponds to WG.
// We lower this value verbatim in SPIRV translator.
// So if dwarf address space = 4, mark it as SLM.
// In Khronos translator dwarf address space = 3 is SLM.
// https://github.com/KhronosGroup/SPIRV-LLVM-Translator/blob/main/docs/SPIRVRepresentationInLLVM.rst#address-spaces
constexpr unsigned DwarfLocalAddressSpaceTag = 3u;

static inline bool isSLMAddressSpaceTag(unsigned addrSpace) {
  return addrSpace == DwarfLocalAddressSpaceTag;
}

namespace IGC {
class StreamEmitter;
class DbgVariable;

class CompileUnit;
class DIEAbbrev;
class DIE;
class DIEBlock;
class DIEEntry;
class DwarfDebug;
class VISADebugInfo;

/// \brief This struct describes location entries emitted in the .debug_loc
/// section.
class DotDebugLocEntry {
  // start/end %ip
  uint32_t offset = 0;

  // The location in the machine frame.
  const llvm::DbgVariableIntrinsic *m_pDbgInst = nullptr;

  // The variable to which this location entry corresponds.
  const llvm::MDNode *Variable = nullptr;

  llvm::MCSymbol *Symbol = nullptr;

public:
  uint64_t start = 0;
  uint64_t end = 0;

  DotDebugLocEntry() : m_pDbgInst(nullptr), Variable(nullptr) {}
  DotDebugLocEntry(const llvm::MCSymbol *B, const llvm::MCSymbol *E,
                   const llvm::DbgVariableIntrinsic *pDbgInst,
                   const llvm::MDNode *V)
      : m_pDbgInst(pDbgInst), Variable(V) {}
  DotDebugLocEntry(const uint64_t s, const uint64_t e,
                   const llvm::DbgVariableIntrinsic *pDbgInst,
                   const llvm::MDNode *V)
      : start(s), end(e), m_pDbgInst(pDbgInst), Variable(V) {}

  /// \brief Empty entries are also used as a trigger to emit temp label. Such
  /// labels are referenced is used to find debug_loc offset for a given DIE.
  bool isEmpty() const { return start == 0 && end == 0; }
  const llvm::MDNode *getVariable() const { return Variable; }
  const llvm::DbgVariableIntrinsic *getDbgInst() const { return m_pDbgInst; }
  uint64_t getStart() const { return start; }
  uint64_t getEnd() const { return end; }

  std::vector<unsigned char> loc;

  uint32_t getOffset() const { return offset; }
  void setOffset(uint32_t o) { offset = o; }

  llvm::MCSymbol *getSymbol() const { return Symbol; }
  void setSymbol(llvm::MCSymbol *S) { Symbol = S; }
};

//===----------------------------------------------------------------------===//
/// \brief This enum is used to describe whether a register represents one of
/// the SIMD32 register halves.
enum class DbgRegisterType : uint8_t {
  Regular = 0,   // Represents all SIMD channels for a source variable, no slice
  FirstHalf = 1, // SIMD32 sliced - lower channels
  SecondHalf = 2 // SIMD32 sliced - upper channels
};

//===----------------------------------------------------------------------===//
/// \brief This class is used to track local variable information.
class DbgVariable {
public:
  constexpr static unsigned int InvalidDotDebugLocOffset = ~0u;

private:
  // Variable Descriptor
  const llvm::DILocalVariable *Var = nullptr;
  // Inlined at location
  const llvm::DILocation *IA = nullptr;
  // Variable DIE
  DIE *TheDIE = nullptr;
  // Offset in DotDebugLocEntries.
  unsigned int DotDebugLocOffset = InvalidDotDebugLocOffset;
  // Corresponding Abstract variable, if any
  DbgVariable *AbsVar = nullptr;
  // DBG_VALUE instruction of the variable
  const llvm::DbgVariableIntrinsic *m_pDbgInst = nullptr;

  // isLocationInlined is true when we expect location to be inlined in
  // DW_AT_location.
  bool isLocationInlined = false;

  DbgRegisterType RegType = DbgRegisterType::Regular;

public:
  // AbsVar may be NULL.
  DbgVariable(const llvm::DILocalVariable *V,
              const llvm::DILocation *IA = nullptr, DbgVariable *AV = nullptr)
      : Var(V), IA(IA), AbsVar(AV) {}

  // Accessors.
  const llvm::DILocation *getLocation() const { return IA; }
  const llvm::DILocalVariable *getVariable() const { return Var; }

  void setDIE(DIE *D) { TheDIE = D; }
  DIE *getDIE() const { return TheDIE; }

  void setDotDebugLocOffset(unsigned int O) { DotDebugLocOffset = O; }
  unsigned int getDotDebugLocOffset() const { return DotDebugLocOffset; }

  llvm::StringRef getName() const { return Var->getName(); }
  DbgVariable *getAbstractVariable() const { return AbsVar; }

  const llvm::DbgVariableIntrinsic *getDbgInst() const { return m_pDbgInst; }
  void setDbgInst(const llvm::DbgVariableIntrinsic *pInst) {
    IGC_ASSERT(pInst && IsSupportedDebugInst(pInst));
    m_pDbgInst = pInst;
  }

  // TODO: re-design required since current approach does not support
  // DIArgList
  /// Returns size of the location associated with the current instance
  /// of the source variable, should be usde to calculate required
  /// storage required
  unsigned getRegisterValueSizeInBits(const DwarfDebug *DD) const;

  // Location is implicit when it's DIExpression ends with
  // !DIExpression(DW_OP_stack_value)
  bool currentLocationIsImplicit() const;

  // Location is memory address when it is described with llvm.dbg.declare
  bool currentLocationIsMemoryAddress() const;

  // "Simple Indirect Value" is a dbg.value of the following form:
  //  llvm.dbg.value(pointer to ..., ..., !DIExpression(DW_OP_deref,
  //  [DW_OP_LLVM_fragment]))
  //
  // Other types of indirect values are currently not supported.
  // For example, if we encounter an expression like this:
  // call void @llvm.dbg.value(metadata ..., ..., metadata
  // !DIExpression(DW_OP_deref), DW_OP_plus_uconst, 3) We won't be able to emit
  // a proper expression, due to limitations of the current *implementation*. A
  // proper expression should have DW_OP_stack_value to form a proper location
  // descriptor needed in this context.
  bool currentLocationIsSimpleIndirectValue() const;
  bool currentLocationIsVector() const;

  bool currentLocationIsInlined() const { return isLocationInlined; }
  void setLocationInlined(bool isInlined = true) {
    isLocationInlined = isInlined;
  }

  DbgRegisterType getLocationRegisterType() const { return RegType; }
  void setLocationRegisterType(DbgRegisterType RegType) {
    this->RegType = RegType;
  }

  void emitExpression(CompileUnit *CU, IGC::DIEBlock *Block) const;

  // Translate tag to proper Dwarf tag.
  llvm::dwarf::Tag getTag() const {
    // FIXME: Why don't we just infer this tag and store it all along?
    if (Var->isParameter())
      return llvm::dwarf::DW_TAG_formal_parameter;

    return llvm::dwarf::DW_TAG_variable;
  }
  /// \brief Return true if DbgVariable is artificial.
  bool isArtificial() const {
    if (Var->isArtificial())
      return true;
    if (getType()->isArtificial())
      return true;
    return false;
  }

  bool isObjectPointer() const {
    if (Var->isObjectPointer())
      return true;
    if (getType()->isObjectPointer())
      return true;
    return false;
  }

  bool isBlockByrefVariable() const;

  llvm::DIType *getType() const;

  static bool IsSupportedDebugInst(const llvm::Instruction *Inst);
  void print(llvm::raw_ostream &O, bool NestedAbstract = false) const;
  static void printDbgInst(llvm::raw_ostream &O, const llvm::Instruction *Inst,
                           const char *NodePrefixes = "     ");
#ifndef NDEBUG
  void dump() const;
  static void dumpDbgInst(const llvm::Instruction *Inst);
#endif // NDEBUG
};

/// \brief Helper used to pair up a symbol and its DWARF compile unit.
struct SymbolCU {
  SymbolCU(CompileUnit *CU, const llvm::MCSymbol *Sym) : Sym(Sym), CU(CU) {}
  const llvm::MCSymbol *Sym;
  CompileUnit *CU;
};

// DwarfDISubprogramCache intended to speedup extraction of DISubprogram
// nodes
// The usage of this class solves 2 problems:
// I. While searching for DISubprogram nodes of a complex shader
//    with lots of kernels and functions we may spend significant
//    amount of time traversing the same function several times.
//    Caching results of a traversal avoids these reduntant
//    calculations.
// II. Input to IGC may have hundreds of kernels.  When emitting to
//    dwarf, we can emit subprogram DIEs defined in current kernel (+
//    it's recursive callees) as well as declarations of other kernels
//    and functions in input. These declarations quickly add up and
//    cause bloat of elf size without adding much benefit. This class
//    provides an interface to return only those DISubprogram nodes for
//    which we want DIE emitted to elf.  This only includes DIEs for
//    subprograms ever referenced in this kernel (+ it's recursive
//    callees). We skip emitting declaration DIEs for which no code is
//    emitted in current kernel.
class DwarfDISubprogramCache {
  using DISubprogramNodes = std::vector<llvm::DISubprogram *>;
  std::unordered_map<const llvm::Function *, DISubprogramNodes> DISubprograms;

  void updateDISPCache(const llvm::Function *F);

public:
  DISubprogramNodes findNodes(const std::vector<llvm::Function *> &Functions);
};

/// \brief Collects and handles llvm::dwarf debug information.
class DwarfDebug {
  // Target of Dwarf emission.
  IGC::StreamEmitter *Asm;
  const IGC::DebugEmitterOpts &EmitSettings;

  ::IGC::VISAModule *m_pModule;

  DwarfDISubprogramCache *DISPCache;

  // All DIEValues are allocated through this allocator.
  llvm::BumpPtrAllocator DIEValueAllocator;

  // Handle to the a compile unit used for the inline extension handling.
  CompileUnit *FirstCU;

  // Maps MDNode with its corresponding CompileUnit.
  llvm::DenseMap<const llvm::MDNode *, CompileUnit *> CUMap;

  // Maps subprogram MDNode with its corresponding CompileUnit.
  llvm::DenseMap<const llvm::MDNode *, CompileUnit *> SPMap;

  // Maps a CU DIE with its corresponding CompileUnit.
  llvm::DenseMap<const DIE *, CompileUnit *> CUDieMap;

  /// Maps MDNodes for type sysstem with the corresponding DIEs. These DIEs can
  /// be shared across CUs, that is why we keep the map here instead
  /// of in CompileUnit.
  llvm::DenseMap<const llvm::MDNode *, DIE *> MDTypeNodeToDieMap;

  // Used to uniquely define abbreviations.
  llvm::FoldingSet<DIEAbbrev> AbbreviationsSet;

  // A list of all the unique abbreviations in use.
  std::vector<DIEAbbrev *> Abbreviations;

  // Stores the current file ID for a given compile unit.
  llvm::DenseMap<unsigned, unsigned> FileIDCUMap;
  // Source id map, i.e. CUID, source filename and directory,
  // separated by a zero byte, mapped to a unique id.
  llvm::StringMap<unsigned, llvm::BumpPtrAllocator &> SourceIdMap;

  // List of all labels used in aranges generation.
  std::vector<SymbolCU> ArangeLabels;

  // Provides a unique id per text section.
  typedef llvm::DenseMap<const llvm::MCSection *,
                         llvm::SmallVector<SymbolCU, 8>>
      SectionMapType;
  SectionMapType SectionMap;

  ::IGC::LexicalScopes LScopes;

  // Collection of abstract subprogram DIEs.
  llvm::DenseMap<const llvm::MDNode *, DIE *> AbstractSPDies;

  // Collection of dbg variables of a scope.
  using DbgVariablesVect = llvm::SmallVector<DbgVariable *, 8>;
  using ScopeVariablesMap =
      llvm::DenseMap<::IGC::LexicalScope *, DbgVariablesVect>;
  ScopeVariablesMap ScopeVariables;

  // List of arguments for current function.
  DbgVariablesVect CurrentFnArguments;

  // Collection of abstract variables.
  llvm::DenseMap<const llvm::MDNode *, DbgVariable *> AbstractVariables;

  // storage for all created DbgVariables
  std::vector<std::unique_ptr<DbgVariable>> DbgVariablesStorage;

  // Collection of DotDebugLocEntry.
  using DotDebugLocEntryVect = llvm::SmallVector<DotDebugLocEntry, 4>;
  DotDebugLocEntryVect DotDebugLocEntries;
  DotDebugLocEntryVect TempDotDebugLocEntries;

  // Collection of subprogram DIEs that are marked (at the end of the module)
  // as DW_AT_inline.
  llvm::SmallPtrSet<DIE *, 4> InlinedSubprogramDIEs;

  // This is a collection of subprogram MDNodes that are processed to
  // create DIEs.
  llvm::SmallPtrSet<const llvm::MDNode *, 16> ProcessedSPNodes;

  // Maps instruction with label emitted before instruction.
  llvm::DenseMap<const llvm::Instruction *, llvm::MCSymbol *> LabelsBeforeInsn;

  // Maps instruction with label emitted after instruction.
  llvm::DenseMap<const llvm::Instruction *, llvm::MCSymbol *> LabelsAfterInsn;

  // Every user variable mentioned by a DBG_VALUE instruction in order of
  // appearance.
  llvm::SmallVector<const llvm::MDNode *, 8> UserVariables;

  // For each user variable, keep a list of DBG_VALUE instructions in order.
  // The list can also contain normal instructions that clobber the previous
  // DBG_VALUE.
  using InstructionsList =
      llvm::SmallVector<const llvm::DbgVariableIntrinsic *, 4>;
  typedef llvm::DenseMap<const llvm::MDNode *, InstructionsList>
      DbgValueHistoryMap;
  DbgValueHistoryMap DbgValues;

  llvm::SmallVector<const llvm::MCSymbol *, 8> DebugRangeSymbols;

  // Store vector of MCSymbol->Raw .debug_ranges data.
  // MCSymbol* is nullptr when not using relocatable elf.
  std::vector<std::pair<llvm::MCSymbol *, llvm::SmallVector<unsigned int, 8>>>
      GenISADebugRangeSymbols;

  // Previous instruction's location information. This is used to determine
  // label location to indicate scope boundries in llvm::dwarf debug info.
  llvm::DebugLoc PrevInstLoc;
  llvm::MCSymbol *PrevLabel;

  // This location indicates end of function prologue and beginning of function
  // body.
  llvm::DebugLoc PrologEndLoc;

  // Section Symbols: these are assembler temporary labels that are emitted at
  // the beginning of each supported llvm::dwarf section.  These are used to
  // form section offsets and are created by EmitSectionLabels.
  llvm::MCSymbol *DwarfInfoSectionSym, *DwarfAbbrevSectionSym;
  llvm::MCSymbol *DwarfStrSectionSym, *TextSectionSym,
      *DwarfDebugRangeSectionSym;
  llvm::MCSymbol *DwarfDebugLocSectionSym, *DwarfLineSectionSym;
  llvm::MCSymbol *FunctionBeginSym, *FunctionEndSym;
  llvm::MCSymbol *ModuleBeginSym, *ModuleEndSym;
  llvm::MCSymbol *DwarfFrameSectionSym;

  // As an optimization, there is no need to emit an entry in the directory
  // table for the same directory as DW_AT_comp_dir.
  llvm::StringRef CompilationDir;

  // Counter for assigning globally unique IDs for CUs.
  unsigned GlobalCUIndexCount;

  // Holders for the various debug information flags that we might need to
  // have exposed. See accessor functions below for description.

  // Holder for types that are going to be extracted out into a type unit.
  std::vector<DIE *> TypeUnits;

  // Version of llvm::dwarf we're emitting.
  unsigned DwarfVersion;

  // A pointer to all units in the section.
  llvm::SmallVector<CompileUnit *, 1> CUs;

  // Collection of strings for this unit and assorted symbols.
  // A String->Symbol mapping of strings used by indirect
  // references.
  typedef llvm::StringMap<std::pair<llvm::MCSymbol *, unsigned>,
                          llvm::BumpPtrAllocator &>
      StrPool;
  StrPool StringPool;
  unsigned NextStringPoolNumber;
  std::string StringPref;

  std::vector<llvm::Function *> RegisteredFunctions;
  llvm::DenseMap<VISAModule *, llvm::Function *> VISAModToFunc;

  llvm::DenseMap<LexicalScope *, DIE *> AbsLexicalScopeDIEMap;

private:
  // AbsVar may be NULL.
  DbgVariable *createDbgVariable(const llvm::DILocalVariable *V,
                                 const llvm::DILocation *IA = nullptr,
                                 DbgVariable *AV = nullptr);

  void addScopeVariable(::IGC::LexicalScope *LS, DbgVariable *Var);

  /// \brief Find abstract variable associated with Var.
  DbgVariable *findAbstractVariable(llvm::DIVariable *Var, llvm::DebugLoc Loc);

  /// \brief Find DIE for the given subprogram and attach appropriate
  /// DW_AT_low_pc, DW_AT_high_pc and DW_AT_INTEL_simd_width.
  /// If there are globalvariables in this scope then create and insert
  /// DIEs for these variables.
  DIE *updateSubprogramScopeDIE(CompileUnit *SPCU, llvm::DISubprogram *SP);

  /// \brief Construct new DW_TAG_lexical_block for this scope and
  /// attach DW_AT_low_pc/DW_AT_high_pc and DW_AT_INTEL_simd_width labels.
  DIE *constructLexicalScopeDIE(CompileUnit *TheCU, ::IGC::LexicalScope *Scope);
  /// A helper function to check whether the DIE for a given Scope is going
  /// to be null.
  bool isLexicalScopeDIENull(::IGC::LexicalScope *Scope);

  /// \brief This scope represents inlined body of a function. Construct
  /// DIE to represent this concrete inlined copy of the function.
  DIE *constructInlinedScopeDIE(CompileUnit *TheCU, ::IGC::LexicalScope *Scope);

  /// \brief Construct a DIE for this scope.
  DIE *constructScopeDIE(CompileUnit *TheCU, ::IGC::LexicalScope *Scope);
  /// A helper function to create children of a Scope DIE.
  DIE *createScopeChildrenDIE(CompileUnit *TheCU, ::IGC::LexicalScope *Scope,
                              llvm::SmallVectorImpl<DIE *> &Children);

  /// \brief Emit initial Dwarf sections with a label at the start of each one.
  void emitSectionLabels();

  /// \brief Compute the size and offset of a DIE given an incoming Offset.
  unsigned computeSizeAndOffset(DIE *Die, unsigned Offset);

  /// \brief Compute the size and offset of all the DIEs.
  void computeSizeAndOffsets();

  /// \brief Attach DW_AT_inline attribute with inlined subprogram DIEs.
  void computeInlinedDIEs();

  /// \brief Collect info for variables that were optimized out.
  void collectDeadVariables();

  /// \brief Finish off debug information after all functions have been
  /// processed.
  void finalizeModuleInfo();

  /// \brief Emit labels to close any remaining sections that have been left
  /// open.
  void endSections();

  /// \brief Emit the debug info section.
  void emitDebugInfo();

  /// \brief Emit the abbreviation section.
  void emitAbbreviations();

  /// \brief Emit visible names into a debug str section.
  void emitDebugStr();

  /// \brief Emit visible names into a debug loc section.
  void emitDebugLoc();

  /// \brief Emit visible names into a debug ranges section.
  void emitDebugRanges();

  /// \brief Emit visible names into a debug macinfo section.
  void emitDebugMacInfo();

  /// \brief Recursively Emits a debug information entry.
  void emitDIE(DIE *Die);

  /// \brief Create new CompileUnit for the given metadata node with tag
  /// DW_TAG_compile_unit.
  CompileUnit *constructCompileUnit(llvm::DICompileUnit *DIUnit);

  /// \brief Construct subprogram DIE.
  void constructSubprogramDIE(CompileUnit *TheCU, const llvm::MDNode *N);

  /// \brief Register a source line with debug info. Returns the unique
  /// label that was emitted and which provides correspondence to the
  /// source line list.
  void recordSourceLine(unsigned Line, unsigned Col, const llvm::MDNode *Scope,
                        unsigned Flags);

  /// \brief Indentify instructions that are marking the beginning of or
  /// ending of a scope.
  void identifyScopeMarkers();

  /// \brief If Var is an current function argument that add it in
  /// CurrentFnArguments list.
  bool addCurrentFnArgument(const llvm::Function *MF, DbgVariable *Var,
                            ::IGC::LexicalScope *Scope);

  /// \brief Populate LexicalScope entries with variables' info.
  void collectVariableInfo(
      const llvm::Function *MF,
      llvm::SmallPtrSet<const llvm::MDNode *, 16> &ProcessedVars);

  /// \brief Ensure that a label will be emitted before MI.
  void requestLabelBeforeInsn(const llvm::Instruction *MI) {
    LabelsBeforeInsn.insert(std::make_pair(MI, (llvm::MCSymbol *)0));
  }

  /// \brief Return Label preceding the instruction.
  llvm::MCSymbol *getLabelBeforeInsn(const llvm::Instruction *MI) {
    llvm::MCSymbol *Label = LabelsBeforeInsn.lookup(MI);
    IGC_ASSERT_MESSAGE(Label, "Didn't insert label before instruction");
    return Label;
  }

  /// \brief Ensure that a label will be emitted after MI.
  void requestLabelAfterInsn(const llvm::Instruction *MI) {
    LabelsAfterInsn.insert(std::make_pair(MI, (llvm::MCSymbol *)0));
  }

  /// \brief Return Label immediately following the instruction.
  llvm::MCSymbol *getLabelAfterInsn(const llvm::Instruction *MI) {
    return LabelsAfterInsn.lookup(MI);
  }

  /// isSubprogramContext - Return true if Context is either a subprogram
  /// or another context nested inside a subprogram.
  bool isSubprogramContext(const llvm::MDNode *Context);

  /// \brief Define a unique number for the abbreviation.
  void assignAbbrevNumber(DIEAbbrev &Abbrev);

  void discoverDISPNodes(DwarfDISubprogramCache &Cache);
  void discoverDISPNodes();

public:
  //===--------------------------------------------------------------------===//
  // Main entry points.
  //
  DwarfDebug(IGC::StreamEmitter *A, ::IGC::VISAModule *M);

  IGC::StreamEmitter &getStreamEmitter() const { return *Asm; }

  const IGC::DebugEmitterOpts &getEmitterSettings() const {
    return EmitSettings;
  }
  void setDISPCache(DwarfDISubprogramCache *Cache) { DISPCache = Cache; }

  void insertDIE(const llvm::MDNode *TypeMD, DIE *Die) {
    MDTypeNodeToDieMap.insert(std::make_pair(TypeMD, Die));
  }
  DIE *getDIE(const llvm::MDNode *TypeMD) {
    return MDTypeNodeToDieMap.lookup(TypeMD);
  }

  /// \brief Emit all Dwarf sections that should come prior to the
  /// content.
  void beginModule();

  /// \brief Emit all Dwarf sections that should come after the content.
  void endModule();

  /// \brief Gather pre-function debug information.
  void beginFunction(const llvm::Function *MF, IGC::VISAModule *);

  /// \brief Gather and emit post-function debug information.
  void endFunction(const llvm::Function *MF);

  /// \brief Process beginning of an instruction.
  void beginInstruction(const llvm::Instruction *MI, bool recordSrcLine);

  /// \brief Process end of an instruction.
  void endInstruction(const llvm::Instruction *MI);

  /// \brief Add a DIE to the set of types that we're going to pull into
  /// type units.
  void addTypeUnitType(DIE *Die) { TypeUnits.push_back(Die); }

  /// \brief Add a label so that arange data can be generated for it.
  void addArangeLabel(SymbolCU SCU) { ArangeLabels.push_back(SCU); }

  /// \brief Look up the source id with the given directory and source file
  /// names. If none currently exists, create a new id and insert it in the
  /// SourceIds map.
  unsigned getOrCreateSourceID(llvm::StringRef DirName,
                               llvm::StringRef FullName, unsigned CUID);

  /// Returns the Dwarf Version.
  unsigned getDwarfVersion() const { return DwarfVersion; }

  /// Find the MDNode for the given reference.
  template <typename T> inline T *resolve(T *Ref) const { return Ref; }
  /// \brief Returns the entry into the start of the pool.
  llvm::MCSymbol *getStringPoolSym();

  /// \brief Returns an entry into the string pool with the given
  /// string text.
  llvm::MCSymbol *getStringPoolEntry(llvm::StringRef Str);

  void registerVISA(IGC::VISAModule *M);

  const llvm::Function *GetPrimaryEntry() const;
  llvm::Function *GetFunction(const VISAModule *M) const;
  VISAModule *GetVISAModule(const llvm::Function *F) const;

  using DataVector = std::vector<unsigned char>;
  void ExtractConstantData(const llvm::Constant *ConstVal, DataVector &R) const;

  /// Construct imported_module or imported_declaration DIE.
  void constructThenAddImportedEntityDIE(CompileUnit *TheCU,
                                         llvm::DIImportedEntity *IE);

private:
  // DISubprograms used by the currently processed shader
  std::vector<llvm::DISubprogram *> DISubprogramNodes;

  // line#, vector<inlinedAt>
  llvm::DenseMap<unsigned int, std::vector<llvm::DILocation *>> isStmtSet;

  const IGC::VISAObjectDebugInfo *VisaDbgInfo = nullptr;

  // store all instructions corresponding to same InlinedAt MDNode
  llvm::DenseMap<llvm::MDNode *, std::vector<const llvm::Instruction *>>
      SameIATInsts;

  // Store label for each %ip
  llvm::DenseMap<unsigned int, llvm::MCSymbol *> LabelsBeforeIp;

  // function, inlinedAt
  llvm::DenseMap<llvm::DISubprogram *, llvm::SmallPtrSet<llvm::DILocation *, 5>>
      prologueEnd;

public:
  bool prologueEndExists(llvm::DISubprogram *sp, llvm::DILocation *dl,
                         bool add) {
    auto it = prologueEnd.find(sp);
    if (it == prologueEnd.end()) {
      if (add)
        prologueEnd[sp].insert(dl);
      return false;
    }

    if (it->second.find(dl) != it->second.end())
      return true;

    if (add)
      it->second.insert(dl);

    return false;
  }

  bool isStmtExists(unsigned int line, llvm::DILocation *inlinedAt, bool add) {
    auto it = isStmtSet.find(line);
    if (it == isStmtSet.end()) {
      if (add) {
        std::vector<llvm::DILocation *> v = {inlinedAt};
        isStmtSet.insert(std::make_pair(line, v));
      }
      return false;
    }

    for (auto &iat : (*it).second) {
      if (iat == inlinedAt)
        return true;
    }

    if (add)
      (*it).second.push_back(inlinedAt);

    return false;
  }

  unsigned int lowPc = 0, highPc = 0;

  // SIMD width
  unsigned short simdWidth = 0;

  const IGC::VISAObjectDebugInfo &getVisaDebugInfo() {
    IGC_ASSERT(VisaDbgInfo);
    return *VisaDbgInfo;
  }

  void setVisaDbgInfo(const IGC::VISAObjectDebugInfo &VDI) {
    VisaDbgInfo = &VDI;
  }

  llvm::MCSymbol *CopyDebugLoc(unsigned int offset);
  unsigned int CopyDebugLocNoReloc(unsigned int o);

  const VISAModule *GetVISAModule() const { return m_pModule; }

  llvm::MCSymbol *GetLabelBeforeIp(unsigned int ip);

  // If this type is derived from a base type then return base type size.
  static uint64_t getBaseTypeSize(const llvm::DIType *Ty);

  uint32_t getBEFPSubReg() {
    auto ver = GetABIVersion();
    if (ver < 3)
      return BEFPSubReg_1_2;
    return BEFPSubReg_3;
  }

  uint32_t getBESPSubReg() {
    auto ver = GetABIVersion();
    if (ver < 3)
      return BESPSubReg_1_2;
    return BESPSubReg_3;
  }

  uint32_t getRetIPSubReg() {
    auto ver = GetABIVersion();
    if (ver < 3)
      return RetIpSubReg_1_2;
    return RetIpSubReg_3;
  }

  uint32_t RetIpByteSize() {
    auto ver = GetABIVersion();
    if (ver < 3)
      return 4;
    return 8;
  }

private:
  void encodeRange(CompileUnit *TheCU, DIE *ScopeDIE,
                   const llvm::SmallVectorImpl<InsnRange> *Ranges);
  void encodeScratchAddrSpace(std::vector<uint8_t> &data);
  uint32_t writeSubroutineCIE();
  uint32_t writeStackcallCIE();
  void writeFDESubroutine(VISAModule *m);
  void writeFDEStackCall(VISAModule *m);

  // Store offset of 2 CIEs, one for stack call and other for subroutines.
  uint32_t offsetCIEStackCall = 0;
  uint32_t offsetCIESubroutine = 0;

  // Offsets (DW) to special registers are stored as constants here.
  // These are specified in VISA ABI.

  // r[MAX-GRF - 3]
  static const unsigned int SpecialGRFOff_VISAABI_1 = 3;
  static const unsigned int SpecialGRFOff_VISAABI_2_3 = 1;
  static const unsigned int RetIpSubReg_1_2 = 0; // :ud
  static const unsigned int RetIpSubReg_3 = 4;   // :ud
  static const unsigned int RetEMSubReg_1_2 = 1; // :ud
  static const unsigned int RetEMSubReg_3 = 6;   // :ud
  static const unsigned int BESPSubReg_1_2 = 2;  // :ud
  static const unsigned int BESPSubReg_3 = 2;    // :ud
  static const unsigned int BEFPSubReg_1_2 = 3;  // :ud
  static const unsigned int BEFPSubReg_3 = 0;    // :ud

  uint32_t GetSpecialGRF() {
    if (!EmitSettings.ZeBinCompatible)
      return GetVISAModule()->getNumGRFs() - SpecialGRFOff_VISAABI_1;
    return GetVISAModule()->getNumGRFs() - SpecialGRFOff_VISAABI_2_3;
  }

  uint32_t GetABIVersion() { return getEmitterSettings().VISAABIVersion; }
};
} // namespace IGC

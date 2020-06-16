/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//


///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfDebug.h
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "llvm/Config/llvm-config.h"
#include "Compiler/DebugInfo/DIE.hpp"
#include "Compiler/DebugInfo/LexicalScopes.hpp"
#include "Compiler/DebugInfo/Version.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/Allocator.h"
#include "llvmWrapper/IR/DebugInfoMetadata.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/DebugInfo/VISAModule.hpp"
#include "Compiler/DebugInfo/LexicalScopes.hpp"
#include <set>
#include "Probe/Assertion.h"

namespace llvm
{
    class MCSection;
}

bool isUnsignedDIType(IGC::DwarfDebug* DD, llvm::DIType* Ty);

namespace IGC
{
    class VISAMachineModuleInfo;
    class StreamEmitter;
    class DbgVariable;

    class CompileUnit;
    class DIEAbbrev;
    class DIE;
    class DIEBlock;
    class DIEEntry;
    class DwarfDebug;

    class DbgDecoder;

    /// \brief This struct describes location entries emitted in the .debug_loc
    /// section.
    class DotDebugLocEntry
    {
        // Begin and end symbols for the address range that this location is valid.
        const llvm::MCSymbol* Begin = nullptr;
        const llvm::MCSymbol* End = nullptr;

        // start/end %ip
        const uint64_t start = 0;
        const uint64_t end = 0;
        uint32_t offset = 0;

        // The location in the machine frame.
        const llvm::Instruction* m_pDbgInst;

        // The variable to which this location entry corresponds.
        const llvm::MDNode* Variable;

        // Whether this location has been merged.
        bool Merged;

    public:
        DotDebugLocEntry() : Begin(0), End(0), m_pDbgInst(nullptr), Variable(nullptr), Merged(false) { }
        DotDebugLocEntry(const llvm::MCSymbol* B, const llvm::MCSymbol* E, const llvm::Instruction* pDbgInst, const llvm::MDNode* V)
            : Begin(B), End(E), m_pDbgInst(pDbgInst), Variable(V), Merged(false) { }
        DotDebugLocEntry(const uint64_t s, const uint64_t e, const llvm::Instruction* pDbgInst, const llvm::MDNode* V)
            : start(s), end(e), m_pDbgInst(pDbgInst), Variable(V) {}

        /// \brief Empty entries are also used as a trigger to emit temp label. Such
        /// labels are referenced is used to find debug_loc offset for a given DIE.
        bool isEmpty() { return start == 0 && end == 0; }
        bool isMerged() { return Merged; }
        void Merge(DotDebugLocEntry* Next)
        {
            if (Begin && m_pDbgInst == Next->m_pDbgInst && End == Next->Begin)
            {
                Next->Begin = Begin;
                Merged = true;
            }
        }

        const llvm::MDNode* getVariable() const { return Variable; }
        const llvm::MCSymbol* getBeginSym() const { return Begin; }
        const llvm::MCSymbol* getEndSym() const { return End; }
        const llvm::Instruction* getDbgInst() const { return m_pDbgInst; }
        uint64_t getStart() const { return start; }
        uint64_t getEnd() const { return end; }

        std::vector<unsigned char> loc;

        uint32_t getOffset() { return offset; }
        void setOffset(uint32_t o) { offset = o; }

    };

    //===----------------------------------------------------------------------===//
    /// \brief This class is used to track local variable information.
    class DbgVariable
    {
        const llvm::DILocalVariable* Var;   // Variable Descriptor.
        const llvm::DILocation* IA;              // Inlined at location.
        DIE* TheDIE;                       // Variable DIE.
        unsigned DotDebugLocOffset;        // Offset in DotDebugLocEntries.
        DbgVariable* AbsVar = nullptr;     // Corresponding Abstract variable, if any.
        const llvm::Instruction* m_pDbgInst; // DBG_VALUE instruction of the variable.
        std::string decorations;

    public:
        // AbsVar may be NULL.
        DbgVariable(const llvm::DILocalVariable* V, const llvm::DILocation* IA_, DbgVariable* AV)
            : Var(V), IA(IA_), TheDIE(0), DotDebugLocOffset(~0U), AbsVar(AV), m_pDbgInst(0) { }

        // Accessors.
        const llvm::DILocation* getLocation() const { return IA; }
        const llvm::DILocalVariable* getVariable()      const { return Var; }
        void setDIE(DIE* D) { TheDIE = D; }
        DIE* getDIE()                       const { return TheDIE; }
        void setDotDebugLocOffset(unsigned O) { DotDebugLocOffset = O; }
        unsigned getDotDebugLocOffset()     const { return DotDebugLocOffset; }
        llvm::StringRef getName()           const { return Var->getName(); }
        DbgVariable* getAbstractVariable()  const { return AbsVar; }
        const llvm::Instruction* getDbgInst() const { return m_pDbgInst; }
        void setDbgInst(const llvm::Instruction* pInst) { m_pDbgInst = pInst; }

        /// If this type is derived from a base type then return base type size
        /// even if it derived directly or indirectly from Composite Type
        uint64_t getBasicTypeSize(llvm::DIDerivedType* Ty);

        /// Return base type size even if it derived directly or indirectly from Composite Type
        uint64_t getBasicSize(DwarfDebug* DD);

        // Translate tag to proper Dwarf tag.

        llvm::dwarf::Tag getTag()                  const
        {
            // FIXME: Why don't we just infer this tag and store it all along?
            if (Var->isParameter())
                return llvm::dwarf::DW_TAG_formal_parameter;

            return llvm::dwarf::DW_TAG_variable;
        }
        /// \brief Return true if DbgVariable is artificial.
        bool isArtificial()                const
        {
            if (Var->isArtificial())
                return true;
            if (getType()->isArtificial())
                return true;
            return false;
        }

        bool isObjectPointer()             const
        {
            if (Var->isObjectPointer())
                return true;
            if (getType()->isObjectPointer())
                return true;
            return false;
        }

#if LLVM_3_5
        bool variableHasComplexAddress()   const
        {
            IGC_ASSERT_MESSAGE(Var.isVariable(), "Invalid complex DbgVariable!");
            return Var.hasComplexAddress();
        }
#endif
        bool isBlockByrefVariable()        const;
#if LLVM_3_5
        unsigned getNumAddrElements()      const
        {
            IGC_ASSERT_MESSAGE(Var.isVariable(), "Invalid complex DbgVariable!");
            return Var.getNumAddrElements();
        }
        uint64_t getAddrElement(unsigned i) const
        {
            return Var.getAddrElement(i);
        }
#endif
        llvm::DIType* getType() const;

        std::string& getDecorations()
        {
            return decorations;
        }

    private:
#if LLVM_VERSION_MAJOR <= 8
        /// resolve - Look in the DwarfDebug map for the MDNode that
        /// corresponds to the reference.
        /// Find the MDNode for the given reference.
        template <typename T> T * resolve(llvm::TypedDINodeRef<T> Ref) const {
            return Ref.resolve();
        }
#else
        template <typename T> inline T* resolve(T* Ref) const {
            return Ref;
        }
#endif
    };

    /// \brief Helper used to pair up a symbol and its DWARF compile unit.
    struct SymbolCU
    {
        SymbolCU(CompileUnit* CU, const llvm::MCSymbol* Sym) : Sym(Sym), CU(CU) {}
        const llvm::MCSymbol* Sym;
        CompileUnit* CU;
    };

    /// \brief Collects and handles llvm::dwarf debug information.
    class DwarfDebug
    {
        // Target of Dwarf emission.
        IGC::StreamEmitter* Asm;

        ::IGC::VISAModule* m_pModule;

        // All DIEValues are allocated through this allocator.
        llvm::BumpPtrAllocator DIEValueAllocator;

        // Handle to the a compile unit used for the inline extension handling.
        CompileUnit* FirstCU;

        // Maps MDNode with its corresponding CompileUnit.
        llvm::DenseMap <const llvm::MDNode*, CompileUnit*> CUMap;

        // Maps subprogram MDNode with its corresponding CompileUnit.
        llvm::DenseMap <const llvm::MDNode*, CompileUnit*> SPMap;

        // Maps a CU DIE with its corresponding CompileUnit.
        llvm::DenseMap <const DIE*, CompileUnit*> CUDieMap;

        /// Maps MDNodes for type sysstem with the corresponding DIEs. These DIEs can
        /// be shared across CUs, that is why we keep the map here instead
        /// of in CompileUnit.
        llvm::DenseMap<const llvm::MDNode*, DIE*> MDTypeNodeToDieMap;

        // Used to uniquely define abbreviations.
        llvm::FoldingSet<DIEAbbrev> AbbreviationsSet;

        // A list of all the unique abbreviations in use.
        std::vector<DIEAbbrev*> Abbreviations;

        // Stores the current file ID for a given compile unit.
        llvm::DenseMap <unsigned, unsigned> FileIDCUMap;
        // Source id map, i.e. CUID, source filename and directory,
        // separated by a zero byte, mapped to a unique id.
        llvm::StringMap<unsigned, llvm::BumpPtrAllocator&> SourceIdMap;

        // List of all labels used in aranges generation.
        std::vector<SymbolCU> ArangeLabels;

        // Provides a unique id per text section.
        typedef llvm::DenseMap<const llvm::MCSection*, llvm::SmallVector<SymbolCU, 8> > SectionMapType;
        SectionMapType SectionMap;

        // List of arguments for current function.
        llvm::SmallVector<DbgVariable*, 8> CurrentFnArguments;

        ::IGC::LexicalScopes LScopes;

        // Collection of abstract subprogram DIEs.
        llvm::DenseMap<const llvm::MDNode*, DIE*> AbstractSPDies;

        // Collection of dbg variables of a scope.
        typedef llvm::DenseMap<::IGC::LexicalScope*, llvm::SmallVector<DbgVariable*, 8> > ScopeVariablesMap;
        ScopeVariablesMap ScopeVariables;

        // Collection of abstract variables.
        llvm::DenseMap<const llvm::MDNode*, DbgVariable*> AbstractVariables;

        // Collection of DotDebugLocEntry.
        llvm::SmallVector<DotDebugLocEntry, 4> DotDebugLocEntries;
        llvm::SmallVector<DotDebugLocEntry, 4> TempDotDebugLocEntries;

        // Collection of subprogram DIEs that are marked (at the end of the module)
        // as DW_AT_inline.
        llvm::SmallPtrSet<DIE*, 4> InlinedSubprogramDIEs;

        // This is a collection of subprogram MDNodes that are processed to
        // create DIEs.
        llvm::SmallPtrSet<const llvm::MDNode*, 16> ProcessedSPNodes;

        // Maps instruction with label emitted before instruction.
        llvm::DenseMap<const llvm::Instruction*, llvm::MCSymbol*> LabelsBeforeInsn;

        // Maps instruction with label emitted after instruction.
        llvm::DenseMap<const llvm::Instruction*, llvm::MCSymbol*> LabelsAfterInsn;

        // Every user variable mentioned by a DBG_VALUE instruction in order of
        // appearance.
        llvm::SmallVector<const llvm::MDNode*, 8> UserVariables;

        // For each user variable, keep a list of DBG_VALUE instructions in order.
        // The list can also contain normal instructions that clobber the previous
        // DBG_VALUE.
        typedef llvm::DenseMap<const llvm::MDNode*, llvm::SmallVector<const llvm::Instruction*, 4> > DbgValueHistoryMap;
        DbgValueHistoryMap DbgValues;

        llvm::SmallVector<const llvm::MCSymbol*, 8> DebugRangeSymbols;

        // Used when emitting Gen ISA offsets directly
        llvm::SmallVector<unsigned int, 8> GenISADebugRangeSymbols;

        // Previous instruction's location information. This is used to determine
        // label location to indicate scope boundries in llvm::dwarf debug info.
        llvm::DebugLoc PrevInstLoc;
        llvm::MCSymbol* PrevLabel;

        // This location indicates end of function prologue and beginning of function
        // body.
        llvm::DebugLoc PrologEndLoc;

        // Section Symbols: these are assembler temporary labels that are emitted at
        // the beginning of each supported llvm::dwarf section.  These are used to form
        // section offsets and are created by EmitSectionLabels.
        llvm::MCSymbol* DwarfInfoSectionSym, * DwarfAbbrevSectionSym;
        llvm::MCSymbol* DwarfStrSectionSym, * TextSectionSym, * DwarfDebugRangeSectionSym;
        llvm::MCSymbol* DwarfDebugLocSectionSym, * DwarfLineSectionSym;
        llvm::MCSymbol* FunctionBeginSym, * FunctionEndSym;
        llvm::MCSymbol* ModuleBeginSym, * ModuleEndSym;
        llvm::MCSymbol* DwarfFrameSectionSym;

        // As an optimization, there is no need to emit an entry in the directory
        // table for the same directory as DW_AT_comp_dir.
        llvm::StringRef CompilationDir;

        // Counter for assigning globally unique IDs for CUs.
        unsigned GlobalCUIndexCount;

        // Holders for the various debug information flags that we might need to
        // have exposed. See accessor functions below for description.

        // Holder for types that are going to be extracted out into a type unit.
        std::vector<DIE*> TypeUnits;

        // Version of llvm::dwarf we're emitting.
        unsigned DwarfVersion;

        // A pointer to all units in the section.
        llvm::SmallVector<CompileUnit*, 1> CUs;

        // Collection of strings for this unit and assorted symbols.
        // A String->Symbol mapping of strings used by indirect
        // references.
        typedef llvm::StringMap<std::pair<llvm::MCSymbol*, unsigned>, llvm::BumpPtrAllocator&> StrPool;
        StrPool StringPool;
        unsigned NextStringPoolNumber;
        std::string StringPref;

    private:

        void addScopeVariable(::IGC::LexicalScope* LS, DbgVariable* Var);

        /// \brief Find abstract variable associated with Var.
        DbgVariable* findAbstractVariable(llvm::DIVariable* Var, llvm::DebugLoc Loc);

        /// \brief Find DIE for the given subprogram and attach appropriate
        /// DW_AT_low_pc, DW_AT_high_pc and DW_AT_INTEL_simd_width.
        /// If there are globalvariables in this scope then create and insert
        /// DIEs for these variables.
        DIE* updateSubprogramScopeDIE(CompileUnit* SPCU, llvm::DISubprogram* SP);

        /// \brief Construct new DW_TAG_lexical_block for this scope and
        /// attach DW_AT_low_pc/DW_AT_high_pc and DW_AT_INTEL_simd_width labels.
        DIE* constructLexicalScopeDIE(CompileUnit* TheCU, ::IGC::LexicalScope* Scope);
        /// A helper function to check whether the DIE for a given Scope is going
        /// to be null.
        bool isLexicalScopeDIENull(::IGC::LexicalScope* Scope);

        /// \brief This scope represents inlined body of a function. Construct
        /// DIE to represent this concrete inlined copy of the function.
        DIE* constructInlinedScopeDIE(CompileUnit* TheCU, ::IGC::LexicalScope* Scope);

        /// \brief Construct a DIE for this scope.
        DIE* constructScopeDIE(CompileUnit* TheCU, ::IGC::LexicalScope* Scope);
        /// A helper function to create children of a Scope DIE.
        DIE* createScopeChildrenDIE(CompileUnit* TheCU, ::IGC::LexicalScope* Scope,
            llvm::SmallVectorImpl<DIE*>& Children);

        /// \brief Emit initial Dwarf sections with a label at the start of each one.
        void emitSectionLabels();

        /// \brief Compute the size and offset of a DIE given an incoming Offset.
        unsigned computeSizeAndOffset(DIE* Die, unsigned Offset);

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

        /// \brief Emit debug frame section to allow stack traversal.
        void emitDebugFrame();

        /// \brief Recursively Emits a debug information entry.
        void emitDIE(DIE* Die);

        /// \brief Create new CompileUnit for the given metadata node with tag
        /// DW_TAG_compile_unit.
        CompileUnit* constructCompileUnit(llvm::DICompileUnit* DIUnit);

        /// \brief Construct subprogram DIE.
        void constructSubprogramDIE(CompileUnit* TheCU, const llvm::MDNode* N);

        /// \brief Register a source line with debug info. Returns the unique
        /// label that was emitted and which provides correspondence to the
        /// source line list.
        void recordSourceLine(unsigned Line, unsigned Col, const llvm::MDNode* Scope,
            unsigned Flags);

        /// \brief Indentify instructions that are marking the beginning of or
        /// ending of a scope.
        void identifyScopeMarkers();

        /// \brief If Var is an current function argument that add it in
        /// CurrentFnArguments list.
        bool addCurrentFnArgument(const llvm::Function* MF,
            DbgVariable* Var, ::IGC::LexicalScope* Scope);

        /// \brief Populate LexicalScope entries with variables' info.
        void collectVariableInfo(const llvm::Function* MF,
            llvm::SmallPtrSet<const llvm::MDNode*, 16> & ProcessedVars);

        /// \brief Ensure that a label will be emitted before MI.
        void requestLabelBeforeInsn(const llvm::Instruction* MI)
        {
            LabelsBeforeInsn.insert(std::make_pair(MI, (llvm::MCSymbol*)0));
        }

        /// \brief Return Label preceding the instruction.
        llvm::MCSymbol* getLabelBeforeInsn(const llvm::Instruction* MI)
        {
            llvm::MCSymbol* Label = LabelsBeforeInsn.lookup(MI);
            IGC_ASSERT_MESSAGE(Label, "Didn't insert label before instruction");
            return Label;
        }

        /// \brief Ensure that a label will be emitted after MI.
        void requestLabelAfterInsn(const llvm::Instruction* MI)
        {
            LabelsAfterInsn.insert(std::make_pair(MI, (llvm::MCSymbol*)0));
        }

        /// \brief Return Label immediately following the instruction.
        llvm::MCSymbol* getLabelAfterInsn(const llvm::Instruction* MI)
        {
            return LabelsAfterInsn.lookup(MI);
        }

        /// isSubprogramContext - Return true if Context is either a subprogram
        /// or another context nested inside a subprogram.
        bool isSubprogramContext(const llvm::MDNode* Context);

        /// \brief Define a unique number for the abbreviation.
        void assignAbbrevNumber(DIEAbbrev& Abbrev);

    public:
        //===--------------------------------------------------------------------===//
        // Main entry points.
        //
        DwarfDebug(IGC::StreamEmitter* A, ::IGC::VISAModule* M);

        ~DwarfDebug()
        {
            if (decodedDbg)
            {
                delete decodedDbg;
                decodedDbg = nullptr;
            }
        }

        void insertDIE(const llvm::MDNode* TypeMD, DIE* Die)
        {
            MDTypeNodeToDieMap.insert(std::make_pair(TypeMD, Die));
        }
        DIE* getDIE(const llvm::MDNode* TypeMD)
        {
            return MDTypeNodeToDieMap.lookup(TypeMD);
        }

        /// \brief Emit all Dwarf sections that should come prior to the
        /// content.
        void beginModule();

        /// \brief Emit all Dwarf sections that should come after the content.
        void endModule();

        /// \brief Gather pre-function debug information.
        void beginFunction(const llvm::Function* MF, IGC::VISAModule*);

        /// \brief Gather and emit post-function debug information.
        void endFunction(const llvm::Function* MF);

        /// \brief Process beginning of an instruction.
        void beginInstruction(const llvm::Instruction* MI, bool recordSrcLine);

        /// \brief Process end of an instruction.
        void endInstruction(const llvm::Instruction* MI);

        /// \brief Add a DIE to the set of types that we're going to pull into
        /// type units.
        void addTypeUnitType(DIE* Die) { TypeUnits.push_back(Die); }

        /// \brief Add a label so that arange data can be generated for it.
        void addArangeLabel(SymbolCU SCU) { ArangeLabels.push_back(SCU); }

        /// \brief Look up the source id with the given directory and source file
        /// names. If none currently exists, create a new id and insert it in the
        /// SourceIds map.
        unsigned getOrCreateSourceID(llvm::StringRef DirName, llvm::StringRef FullName, unsigned CUID);

        /// Returns the Dwarf Version.
        unsigned getDwarfVersion() const { return DwarfVersion; }

        /// Find the MDNode for the given reference.
#if LLVM_VERSION_MAJOR <= 8
        template <typename T> T * resolve(llvm::TypedDINodeRef<T> Ref) const
        {
            return Ref.resolve();
        }
#else
        template <typename T> inline T* resolve(T* Ref) const
        {
            return Ref;
        }
#endif
        /// \brief Returns the entry into the start of the pool.
        llvm::MCSymbol* getStringPoolSym();

        /// \brief Returns an entry into the string pool with the given
        /// string text.
        llvm::MCSymbol* getStringPoolEntry(llvm::StringRef Str);

        //Following added during LLVM 4.0 upgrade

        // Add DISubprogram node only if it isnt already present.
        // Not using std::set as we need to preserve order of
        // insertion and iteration.
        // Return true if insertion successful, false if entry
        // already exists.
        bool addUniqueDISP(llvm::DISubprogram* sp)
        {
            auto it = std::find(DISubprogramNodes.begin(), DISubprogramNodes.end(), sp);
            if (it == DISubprogramNodes.end())
            {
                DISubprogramNodes.push_back(sp);
                return true;
            }
            return false;
        }
    private:
        // Store all DISubprogram nodes from LLVM IR as they are no longer available
        // in DICompileUnit
        std::vector<llvm::DISubprogram*> DISubprogramNodes;
        std::map<llvm::DISubprogram*, const llvm::Function*> DISPToFunction;

        void gatherDISubprogramNodes();

        // line#, vector<inlinedAt>
        std::map<unsigned int, std::vector<llvm::DILocation*>> isStmtSet;

        DbgDecoder* decodedDbg = nullptr;

        std::map<llvm::MDNode*, std::vector<const llvm::Instruction*>> SameIATInsts;
    public:
        std::map<llvm::DISubprogram*, const llvm::Function*>* getDISPToFunction()
        {
            return &DISPToFunction;
        }

        bool isStmtExists(unsigned int line, llvm::DILocation* inlinedAt, bool add)
        {
            auto it = isStmtSet.find(line);
            if (it == isStmtSet.end())
            {
                if (add)
                {
                    std::vector<llvm::DILocation*> v = { inlinedAt };
                    isStmtSet.insert(std::make_pair(line, v));
                }
                return false;
            }

            for (auto& iat : (*it).second)
            {
                if (iat == inlinedAt)
                    return true;
            }

            if (add)
                (*it).second.push_back(inlinedAt);

            return false;
        }

        unsigned int lowPc = 0, highPc = 0;

		// SIMD width
        unsigned short simdWidth = 0;   // Not set until IGC_IS_FLAG_ENABLED(EnableSIMDLaneDebugging)

        DbgDecoder* getDecodedDbg() { return decodedDbg; }
        void setDecodedDbg(DbgDecoder* d)
        {
            if (decodedDbg)
            {
                delete decodedDbg;
            }
            decodedDbg = d;
        }
        unsigned int CopyDebugLoc(unsigned int offset);


    private:
        void encodeRange(CompileUnit* TheCU, DIE* ScopeDIE, const llvm::SmallVectorImpl<InsnRange>* Ranges);
        void writeCIE();
        void writeFDE(DbgDecoder::SubroutineInfo& sub);
        const uint8_t returnReg = 128;
    };
} // namespace IGC


/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfDebug.cpp
///////////////////////////////////////////////////////////////////////////////

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Debug.h"
#include "llvm/IR/Function.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/LEB128.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/MC/MCDwarf.h"
#include "common/LLVMWarningsPop.hpp"

#include "DwarfDebug.hpp"
#include "DIE.hpp"
#include "DwarfCompileUnit.hpp"
#include "StreamEmitter.hpp"
#include "VISAModule.hpp"

#include <unordered_set>
#include <list>

#include "Probe/Assertion.h"

#define DEBUG_TYPE "dwarfdebug"

using namespace llvm;
using namespace IGC;

//===----------------------------------------------------------------------===//

// Configuration values for initial hash set sizes (log2).
//
static const unsigned InitAbbreviationsSetSize = 9; // log2(512)

const char* beginSymbol = ".begin";
const char* endSymbol = ".end";

bool DbgVariable::isBlockByrefVariable() const {
#if LLVM_VERSION_MAJOR < 10
    IGC_ASSERT_MESSAGE(Var, "Invalid complex DbgVariable!");
    return Var->getType()->isBlockByrefStruct();
#else
    // isBlockByrefStruct is no more support by LLVM10 IR - more info in this commit below:
    // https://github.com/llvm/llvm-project/commit/0779dffbd4a927d7bf9523482481248c51796907
    return false;
#endif
}

/// If this type is derived from a base type then return base type size
/// even if it derived directly or indirectly from Derived Type
uint64_t DbgVariable::getBasicTypeSize(DICompositeType* Ty)
{
    unsigned Tag = Ty->getTag();

    if (Tag != dwarf::DW_TAG_member && Tag != dwarf::DW_TAG_typedef &&
        Tag != dwarf::DW_TAG_const_type && Tag != dwarf::DW_TAG_volatile_type &&
        Tag != dwarf::DW_TAG_restrict_type)
    {
        return Ty->getSizeInBits();
    }

    DIType* BaseType = Ty->getBaseType();

    // If this is a derived type, go ahead and get the base type, unless it's a
    // reference then it's just the size of the field. Pointer types have no need
    // of this since they're a different type of qualification on the type.
    if (BaseType->getTag() == dwarf::DW_TAG_reference_type ||
        BaseType->getTag() == dwarf::DW_TAG_rvalue_reference_type)
    {
        return Ty->getSizeInBits();
    }

    if (isa<DIDerivedType>(BaseType))
    {
        return getBasicTypeSize(cast<DIDerivedType>(BaseType));
    }
    else if (isa<DICompositeType>(BaseType))
    {
        return getBasicTypeSize(cast<DICompositeType>(BaseType));
    }
    else if (isa<DIBasicType>(BaseType))
    {
        return BaseType->getSizeInBits();
    }
    else
    {
        // Be prepared for unexpected.
        IGC_ASSERT_MESSAGE(0, "Missing support for this type");
    }

    return BaseType->getSizeInBits();
}

/// If this type is derived from a base type then return base type size
/// even if it derived directly or indirectly from Composite Type
uint64_t DbgVariable::getBasicTypeSize(DIDerivedType* Ty)
{
    unsigned Tag = Ty->getTag();

    if (Tag != dwarf::DW_TAG_member && Tag != dwarf::DW_TAG_typedef &&
        Tag != dwarf::DW_TAG_const_type && Tag != dwarf::DW_TAG_volatile_type &&
        Tag != dwarf::DW_TAG_restrict_type)
    {
        return Ty->getSizeInBits();
    }

    DIType* BaseType = Ty->getBaseType();

    // If this is a derived type, go ahead and get the base type, unless it's a
    // reference then it's just the size of the field. Pointer types have no need
    // of this since they're a different type of qualification on the type.
    if (BaseType->getTag() == dwarf::DW_TAG_reference_type ||
        BaseType->getTag() == dwarf::DW_TAG_rvalue_reference_type)
    {
        return Ty->getSizeInBits();
    }

    if (isa<DIDerivedType>(BaseType))
    {
        return getBasicTypeSize(cast<DIDerivedType>(BaseType));
    }
    else if (isa<DICompositeType>(BaseType))
    {
        return getBasicTypeSize(cast<DICompositeType>(BaseType));
    }
    else if (isa<DIBasicType>(BaseType))
    {
        return BaseType->getSizeInBits();
    }
    else
    {
        // Be prepared for unexpected.
        IGC_ASSERT_MESSAGE(0, "Missing support for this type");
    }

    return BaseType->getSizeInBits();
}

/// Return base type size even if it derived directly or indirectly from Composite Type
uint64_t DbgVariable::getBasicSize(DwarfDebug* DD)
{
    uint64_t varSizeInBits = getType()->getSizeInBits();

    if (isa<DIDerivedType>(getType()))
    {
        // If type is derived then size of a basic type is needed
        DIType* Ty = getType();
        DIDerivedType* DDTy = cast<DIDerivedType>(Ty);
        varSizeInBits = getBasicTypeSize(DDTy);
        IGC_ASSERT_MESSAGE(varSizeInBits > 0, "\nVariable's basic type size 0\n");
        IGC_ASSERT_MESSAGE(!(varSizeInBits == 0 && getType()->getSizeInBits() == 0),
            "\nVariable's basic type size 0 and getType()->getSizeInBits() 0\n");
    }
    else
    {
        IGC_ASSERT_MESSAGE(varSizeInBits > 0, "Not derived type variable's size 0");
    }

    return varSizeInBits;
}

DIType* DbgVariable::getType() const
{
    return getVariable()->getType();
}

/// Return Dwarf Version by checking module flags.
static unsigned getDwarfVersionFromModule(const Module* M)
{
    auto* Val = cast_or_null<ConstantAsMetadata>(M->getModuleFlag("Dwarf Version"));
    if (!Val)
        return dwarf::DWARF_VERSION;
    return (unsigned)(cast<ConstantInt>(Val->getValue())->getZExtValue());
}

void DwarfDISubprogramCache::updateDISPCache(const llvm::Function *F)
{
    llvm::DenseSet<const DISubprogram*> DISPToFunction;
    llvm::DenseSet<const MDNode*> Processed;

    if (auto *DISP = F->getSubprogram())
        DISubprograms[F].push_back(DISP);

    for (auto I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I)
    {
        auto debugLoc = I->getDebugLoc().get();
        while (debugLoc)
        {
            auto scope = debugLoc->getScope();
            if (scope &&
                dyn_cast_or_null<llvm::DILocalScope>(scope) &&
                Processed.find(scope) == Processed.end())
            {
                auto DISP = cast<llvm::DILocalScope>(scope)->getSubprogram();
                if (DISPToFunction.find(DISP) == DISPToFunction.end())
                {
                    DISubprograms[F].push_back(DISP);
                    DISPToFunction.insert(DISP);
                    Processed.insert(scope);
                }
            }

            if (debugLoc->getInlinedAt())
                debugLoc = debugLoc->getInlinedAt();
            else
                debugLoc = nullptr;
        }
    }
}

DwarfDISubprogramCache::DISubprogramNodes
DwarfDISubprogramCache::findNodes (const std::vector<Function*>& Functions)
{
    DISubprogramNodes Result;
    // to ensure that Result does not contain duplicates
    std::unordered_set<const llvm::DISubprogram*> UniqueDISP;

    for (const auto* F: Functions)
    {
        // If we don't have a list of DISP nodes for the processed function -
        // create one and store it in cache
        if (DISubprograms.find(F) == DISubprograms.end())
            updateDISPCache(F);

        const auto& DISPNodes = DISubprograms[F];
        for (auto *DISP : DISPNodes)
        {
            // we should report only unique DISP nodes
            if (UniqueDISP.find(DISP) != UniqueDISP.end())
                continue;

            Result.push_back(DISP);
            UniqueDISP.insert(DISP);
        }
    }
    return Result;
}
DwarfDebug::DwarfDebug(StreamEmitter* A, VISAModule* M) :
    Asm(A), EmitSettings(Asm->GetEmitterSettings()), m_pModule(M),
    DISPCache(nullptr),
    FirstCU(0),
    //AbbreviationsSet(InitAbbreviationsSetSize),
    SourceIdMap(DIEValueAllocator),
    PrevLabel(nullptr),
    GlobalCUIndexCount(0),
    StringPool(DIEValueAllocator),
    NextStringPoolNumber(0),
    StringPref("info_string")
{

    DwarfInfoSectionSym = nullptr;
    DwarfAbbrevSectionSym = nullptr;
    DwarfLineSectionSym = nullptr;
    DwarfStrSectionSym = nullptr;
    DwarfDebugRangeSectionSym = nullptr;
    DwarfDebugLocSectionSym = nullptr;
    TextSectionSym = nullptr;
    DwarfFrameSectionSym = nullptr;

    FunctionBeginSym = FunctionEndSym = nullptr;;
    ModuleBeginSym = ModuleEndSym = nullptr;;

    DwarfVersion = getDwarfVersionFromModule(M->GetModule());
}

DwarfDebug::~DwarfDebug()
{
    decodedDbg = nullptr;
}

MCSymbol* DwarfDebug::getStringPoolSym()
{
    return Asm->GetTempSymbol(StringPref);
}

MCSymbol* DwarfDebug::getStringPoolEntry(StringRef Str)
{
    std::pair<MCSymbol*, unsigned>& Entry = StringPool[Str];
    if (!Entry.first) {
        Entry.second = StringPool.size() - 1;
        Entry.first = Asm->GetTempSymbol(StringPref, Entry.second);
    }
    return Entry.first;
}
void DwarfDebug::registerVISA(IGC::VISAModule* M)
{
    IGC_ASSERT(M);
    auto *F = M->getFunction();
    // Sanity check that we have only single module associated
    // with a Function
    auto *EM = GetVISAModule(F);
    if (M == EM)
        return;
    // TODO: we need to change this one to assert
    if (EM != nullptr) {
        VISAModToFunc.erase(EM);
    }
    RegisteredFunctions.push_back(F);
    VISAModToFunc[M] = RegisteredFunctions.back();
}

// Define a unique number for the abbreviation.
//
void DwarfDebug::assignAbbrevNumber(IGC::DIEAbbrev& Abbrev)
{
    // Check the set for priors.
    DIEAbbrev* InSet = AbbreviationsSet.GetOrInsertNode(&Abbrev);

    // If it's newly added.
    if (InSet == &Abbrev)
    {
        // Add to abbreviation list.
        Abbreviations.push_back(&Abbrev);

        // Assign the vector position + 1 as its number.
        Abbrev.setNumber(Abbreviations.size());
    }
    else
    {
        // Assign existing abbreviation number.
        Abbrev.setNumber(InSet->getNumber());
    }
}

/// isSubprogramContext - Return true if Context is either a subprogram
/// or another context nested inside a subprogram.
bool DwarfDebug::isSubprogramContext(const MDNode* D)
{
    if (!D)
        return false;
    if (isa<DISubprogram>(D))
        return true;
    if (isa<DIType>(D))
        return isSubprogramContext(resolve(cast<DIType>(D)->getScope()));
    return false;
}

// Find DIE for the given subprogram and attach appropriate DW_AT_low_pc
// and DW_AT_high_pc attributes. If there are global variables in this
// scope then create and insert DIEs for these variables.
DIE* DwarfDebug::updateSubprogramScopeDIE(CompileUnit* SPCU, DISubprogram* SP)
{
    DIE* SPDie = SPCU->getDIE(SP);

    IGC_ASSERT_MESSAGE(SPDie, "Unable to find subprogram DIE!");

    // If we're updating an abstract DIE, then we will be adding the children and
    // object pointer later on. But what we don't want to do is process the
    // concrete DIE twice.
    if (DIE * AbsSPDIE = AbstractSPDies.lookup(SP))
    {
        // Pick up abstract subprogram DIE.
        SPDie = SPCU->createAndAddDIE(dwarf::DW_TAG_subprogram, *SPCU->getCUDie());
        SPCU->addDIEEntry(SPDie, dwarf::DW_AT_abstract_origin, AbsSPDIE);
    }
    else
    {
        DISubprogram* SPDecl = SP->getDeclaration();
        if (!SPDecl)
        {
            // There is not any need to generate specification DIE for a function
            // defined at compile unit level. If a function is defined inside another
            // function then gdb prefers the definition at top level and but does not
            // expect specification DIE in parent function. So avoid creating
            // specification DIE for a function defined inside a function.
            DIScope* SPContext = resolve(SP->getScope());
            if (SP->isDefinition() && SPContext && !isa<DICompileUnit>(SPContext) &&
                !isa<DIFile>(SPContext) && !isSubprogramContext(SPContext))
            {
                SPCU->addFlag(SPDie, dwarf::DW_AT_declaration);

                // Add arguments.
                DISubroutineType* SPTy = SP->getType();
                if (SPTy)
                {
                    DITypeRefArray Args = SPTy->getTypeArray();
                    uint16_t SPTag = (uint16_t)SPTy->getTag();
                    if (SPTag == dwarf::DW_TAG_subroutine_type)
                    {
                        for (unsigned i = 1, N = Args.size(); i < N; ++i)
                        {
                            DIE* Arg = SPCU->createAndAddDIE(dwarf::DW_TAG_formal_parameter, *SPDie);
                            DIType* ATy = cast<DIType>(Args[i]);
                            SPCU->addType(Arg, ATy);
                            if (ATy->isArtificial())
                                SPCU->addFlag(Arg, dwarf::DW_AT_artificial);
                            if (ATy->isObjectPointer())
                                SPCU->addDIEEntry(SPDie, dwarf::DW_AT_object_pointer, Arg);
                        }
                    }
                    DIE* SPDeclDie = SPDie;
                    SPDie = SPCU->createAndAddDIE(dwarf::DW_TAG_subprogram, *SPCU->getCUDie());
                    SPCU->addDIEEntry(SPDie, dwarf::DW_AT_specification, SPDeclDie);
                }
            }
        }
    }

    if (EmitSettings.EnableRelocation)
    {
        auto Id = m_pModule->GetFuncId();
        SPCU->addLabelAddress(SPDie, dwarf::DW_AT_low_pc,
                              Asm->GetTempSymbol("func_begin", Id));
        SPCU->addLabelAddress(SPDie, dwarf::DW_AT_high_pc,
                              Asm->GetTempSymbol("func_end", Id));
    }
    else
    {
        SPCU->addUInt(SPDie, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, lowPc);
        SPCU->addUInt(SPDie, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, highPc);
    }

    return SPDie;
}

/// Check whether we should create a DIE for the given Scope, return true
/// if we don't create a DIE (the corresponding DIE is null).
bool DwarfDebug::isLexicalScopeDIENull(LexicalScope* Scope)
{
    if (Scope->isAbstractScope())
        return false;

    // We don't create a DIE if there is no Range.
    const SmallVectorImpl<InsnRange>& Ranges = Scope->getRanges();
    if (Ranges.empty())
        return true;

    if (Ranges.size() > 1)
        return false;

    return false;
}

// Construct new DW_TAG_lexical_block for this scope and attach
// DW_AT_low_pc/DW_AT_high_pc labels as well as DW_AT_INTEL_simd_width.
// Also add DW_AT_abstract_origin when lexical scope is from inlined code.
DIE* DwarfDebug::constructLexicalScopeDIE(CompileUnit* TheCU, LexicalScope* Scope)
{
    if (isLexicalScopeDIENull(Scope))
        return 0;

    DIE* ScopeDIE = new DIE(dwarf::DW_TAG_lexical_block);
    if (Scope->isAbstractScope())
    {
        AbsLexicalScopeDIEMap.insert(std::make_pair(Scope, ScopeDIE));
        return ScopeDIE;
    }

    const SmallVectorImpl<InsnRange>& Ranges = Scope->getRanges();
    IGC_ASSERT_MESSAGE(Ranges.empty() == false, "LexicalScope does not have instruction markers!");

    if (Scope->getInlinedAt())
    {
        auto abstractScope = LScopes.findAbstractScope(dyn_cast_or_null<DILocalScope>(Scope->getScopeNode()));
        if (abstractScope)
        {
            auto AbsDIE = AbsLexicalScopeDIEMap.lookup(abstractScope);
            if (AbsDIE)
            {
                // Point to corresponding abstract instance of DW_TAG_lexical_block
                TheCU->addDIEEntry(ScopeDIE, dwarf::DW_AT_abstract_origin, AbsDIE);
            }
        }
    }

    if (EmitSettings.EmitDebugRanges)
    {
        encodeRange(TheCU, ScopeDIE, &Ranges);
    }
    else
    {
        // This makes sense only for full debug info.
        // Resolve VISA index to Gen IP here.
        auto start = Ranges.front().first;
        auto end = Ranges.back().second;
        InsnRange RI(start, end);
        auto GenISARanges = m_pModule->getGenISARange(RI);

        if (GenISARanges.size() > 0)
        {
            // Emit loc/high_pc
            if (EmitSettings.EnableRelocation)
            {
                auto StartLabel = GetLabelBeforeIp(GenISARanges.front().first);
                auto EndLabel = GetLabelBeforeIp(GenISARanges.back().second);
                TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_low_pc, StartLabel);
                TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_high_pc, EndLabel);
            }
            else
            {
                TheCU->addUInt(ScopeDIE, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, GenISARanges.front().first);
                TheCU->addUInt(ScopeDIE, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, GenISARanges.back().second);
            }
        }
    }
    return ScopeDIE;
}

void DwarfDebug::encodeRange(CompileUnit* TheCU, DIE* ScopeDIE, const llvm::SmallVectorImpl<InsnRange>* Ranges)
{
    // When functions are inlined, their allocas get hoisted to top
    // of kernel, including their dbg.declares. Since dbg.declare
    // nodes have DebugLoc, it means the function would've 2
    // live-intervals, first one being hoisted dbg.declare/alloca
    // and second being actual function. When emitting debug_loc
    // we only want to use the second interval since it includes
    // actual function user wants to debug. Following loop prunes
    // Ranges vector to include only actual function. It does so
    // by checking whether any sub-range has DebugLoc attached to
    // non-DbgInfoIntrinsic instruction.
    auto IsValidRange = [](const InsnRange& R)
    {
        auto start = R.first;
        auto end = R.second;
        while (end != start && start)
        {
            if (!llvm::isa<DbgInfoIntrinsic>(start))
                if (start->getDebugLoc())
                    return true;

            start = getNextInst(start);
        }
        return false;
    };

    llvm::SmallVector<InsnRange, 5> PrunedRanges;
    for (auto& R : *Ranges)
    {
        if (IsValidRange(R))
            PrunedRanges.push_back(R);
    }

    // This makes sense only for full debug info.
    if (PrunedRanges.size() == 0)
        return;

    // Resolve VISA index to Gen IP here.
    std::vector<std::pair<unsigned int, unsigned int>> AllGenISARanges;
    for (SmallVectorImpl<InsnRange>::const_iterator RI = PrunedRanges.begin(),
        RE = PrunedRanges.end(); RI != RE; ++RI)
    {
        auto GenISARanges = m_pModule->getGenISARange(*RI);
        for (auto& R : GenISARanges)
        {
            AllGenISARanges.push_back(R);
        }
    }

    m_pModule->coalesceRanges(AllGenISARanges);

    if (AllGenISARanges.size() == 1)
    {
        // Emit low_pc/high_pc inlined in DIE
        if (EmitSettings.EnableRelocation)
        {
            auto StartLabel = GetLabelBeforeIp(AllGenISARanges.front().first);
            auto EndLabel = GetLabelBeforeIp(AllGenISARanges.front().second);
            TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_low_pc, StartLabel);
            TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_high_pc, EndLabel);
        }
        else
        {
            TheCU->addUInt(ScopeDIE, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, AllGenISARanges.front().first);
            TheCU->addUInt(ScopeDIE, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, AllGenISARanges.front().second);
        }
    }
    else if (AllGenISARanges.size() > 1)
    {
        // Emit to debug_ranges
        llvm::MCSymbol* NewLabel = nullptr;
        if (EmitSettings.EnableRelocation)
        {
            NewLabel = Asm->CreateTempSymbol();
            TheCU->addLabelLoc(ScopeDIE, dwarf::DW_AT_ranges, NewLabel);
        }
        else
        {
            auto GetDebugRangeSize = [&]()
            {
                size_t TotalSize = 0;
                for (auto& Entry : GenISADebugRangeSymbols)
                {
                    TotalSize += Entry.second.size();
                }
                return TotalSize;
            };

            TheCU->addUInt(ScopeDIE, dwarf::DW_AT_ranges, dwarf::DW_FORM_sec_offset,
                GetDebugRangeSize() * Asm->GetPointerSize());
        }

        llvm::SmallVector<unsigned int, 8> Data;

        for (auto& item : AllGenISARanges)
        {
            Data.push_back(item.first);
            Data.push_back(item.second);
        }

        // Terminate the range list
        Data.push_back(0);
        Data.push_back(0);

        GenISADebugRangeSymbols.emplace_back(std::make_pair(NewLabel, Data));
    }
}

// This scope represents inlined body of a function. Construct DIE to
// represent this concrete inlined copy of the function.
DIE* DwarfDebug::constructInlinedScopeDIE(CompileUnit* TheCU, LexicalScope* Scope)
{
    if (!Scope->getScopeNode())
        return NULL;

    const SmallVectorImpl<InsnRange>& Ranges = Scope->getRanges();
    IGC_ASSERT_MESSAGE(Ranges.empty() == false, "LexicalScope does not have instruction markers!");

    const MDNode* DS = Scope->getScopeNode();
    DISubprogram* InlinedSP = getDISubprogram(DS);
    DIE* OriginDIE = TheCU->getDIE(InlinedSP);
    if (!OriginDIE)
    {
        LLVM_DEBUG(dbgs() << "Unable to find original DIE for an inlined subprogram.");
        return NULL;
    }

    DIE* ScopeDIE = new DIE(dwarf::DW_TAG_inlined_subroutine);

    InlinedSubprogramDIEs.insert(OriginDIE);
    TheCU->addDIEEntry(ScopeDIE, dwarf::DW_AT_abstract_origin, OriginDIE);

    // Add the call site information to the DIE.
    DILocation* DL = cast<DILocation>(const_cast<MDNode*>(Scope->getInlinedAt()));
    unsigned int fileId = getOrCreateSourceID(DL->getFilename(), DL->getDirectory(), TheCU->getUniqueID());
    TheCU->addUInt(ScopeDIE, dwarf::DW_AT_call_file, None, fileId);
    TheCU->addUInt(ScopeDIE, dwarf::DW_AT_call_line, None, DL->getLine());

    // .debug_range section has not been laid out yet. Emit offset in
    // .debug_range as a uint, size 4, for now. emitDIE will handle
    // DW_AT_ranges appropriately.
    encodeRange(TheCU, ScopeDIE, &Ranges);

    return ScopeDIE;
}

DIE* DwarfDebug::createScopeChildrenDIE(CompileUnit* TheCU, LexicalScope* Scope, SmallVectorImpl<DIE*>& Children)
{
    DIE* ObjectPointer = NULL;

    SmallVector<DbgVariable*, 8> dbgVariables;

    // Collect arguments for current function.
    if (LScopes.isCurrentFunctionScope(Scope))
    {
        dbgVariables.insert(dbgVariables.end(), CurrentFnArguments.begin(), CurrentFnArguments.end());
    }

    // Collect lexical scope variables.
    const SmallVectorImpl<DbgVariable*>& Variables = ScopeVariables.lookup(Scope);
    dbgVariables.insert(dbgVariables.end(), Variables.begin(), Variables.end());

    // Collect all argument/variable children
    for (unsigned i = 0, N = dbgVariables.size(); i < N; ++i)
    {
        DbgVariable* ArgDV = dbgVariables[i];
        if (!ArgDV) continue;
        if (DIE * Arg = TheCU->constructVariableDIE(*ArgDV, Scope->isAbstractScope()))
        {
            Children.push_back(Arg);
            if (ArgDV->isObjectPointer()) ObjectPointer = Arg;
        }
    }

    const SmallVectorImpl<LexicalScope*>& Scopes = Scope->getChildren();
    for (unsigned j = 0, M = Scopes.size(); j < M; ++j)
    {
        if (DIE * Nested = constructScopeDIE(TheCU, Scopes[j]))
        {
            Children.push_back(Nested);
        }
    }
    return ObjectPointer;
}

// Construct a DIE for this scope.
DIE* DwarfDebug::constructScopeDIE(CompileUnit* TheCU, LexicalScope* Scope)
{
    if (!Scope || !Scope->getScopeNode())
        return NULL;

    const MDNode* DS = Scope->getScopeNode();

    SmallVector<DIE*, 8> Children;
    DIE* ObjectPointer = NULL;
    bool ChildrenCreated = false;

    // We try to create the scope DIE first, then the children DIEs. This will
    // avoid creating un-used children then removing them later when we find out
    // the scope DIE is null.
    DIE* ScopeDIE = NULL;
    if (isa<DISubprogram>(DS) && Scope->getInlinedAt())
    {
        ScopeDIE = constructInlinedScopeDIE(TheCU, Scope);
    }
    else if (isa<DISubprogram>(DS))
    {
        ProcessedSPNodes.insert(DS);
        if (Scope->isAbstractScope())
        {
            ScopeDIE = TheCU->getDIE(cast<DINode>(const_cast<MDNode*>(DS)));
            // Note down abstract DIE.
            if (ScopeDIE)
            {
                AbstractSPDies.insert(std::make_pair(DS, ScopeDIE));
            }
        }
        else
        {
            ScopeDIE = updateSubprogramScopeDIE(TheCU, cast<DISubprogram>(const_cast<MDNode*>(DS)));
        }
    }
    else
    {
        // Early exit when we know the scope DIE is going to be null.
        if (isLexicalScopeDIENull(Scope))
            return NULL;

        // We create children here when we know the scope DIE is not going to be
        // null and the children will be added to the scope DIE.
        ObjectPointer = createScopeChildrenDIE(TheCU, Scope, Children);
        ChildrenCreated = true;

        if (Children.empty())
            return NULL;
        ScopeDIE = constructLexicalScopeDIE(TheCU, Scope);
        IGC_ASSERT_MESSAGE(ScopeDIE, "Scope DIE should not be null.");
    }

    if (!ScopeDIE)
    {
        IGC_ASSERT_MESSAGE(Children.empty(), "We create children only when the scope DIE is not null.");
        return NULL;
    }
    if (!ChildrenCreated)
    {
        // We create children when the scope DIE is not null.
        ObjectPointer = createScopeChildrenDIE(TheCU, Scope, Children);
    }

    // Add children
    for (SmallVectorImpl<DIE*>::iterator I = Children.begin(),
        E = Children.end(); I != E; ++I)
    {
        ScopeDIE->addChild(*I);
    }

    if (isa<DISubprogram>(DS) && ObjectPointer != NULL)
    {
        TheCU->addDIEEntry(ScopeDIE, dwarf::DW_AT_object_pointer, ObjectPointer);
    }

    return ScopeDIE;
}

// Look up the source id with the given directory and source file names.
// If none currently exists, create a new id and insert it in the
// SourceIds map. This can update DirectoryNames and SourceFileNames maps
// as well.
unsigned DwarfDebug::getOrCreateSourceID(StringRef FileName, StringRef DirName, unsigned CUID)
{
    // If we use .loc in assembly, we can't separate .file entries according to
    // compile units. Thus all files will belong to the default compile unit.

    // If FE did not provide a file name, then assume stdin.
    if (FileName.empty())
    {
        return getOrCreateSourceID("<stdin>", StringRef(), CUID);
    }

    // TODO: this might not belong here. See if we can factor this better.
    if (DirName == CompilationDir)
    {
        DirName = "";
    }

    // FileIDCUMap stores the current ID for the given compile unit.
    unsigned SrcId = FileIDCUMap[CUID] + 1;

    // We look up the CUID/file/dir by concatenating them with a zero byte.
    SmallString<128> NamePair;
    NamePair += utostr(CUID);
    NamePair += '\0';
    NamePair += DirName;
    NamePair += '\0'; // Zero bytes are not allowed in paths.
    NamePair += FileName;

    auto item = SourceIdMap.insert(std::make_pair(NamePair, std::move(SrcId)));
    if (!item.second)
    {
        return item.first->second;
    }

    FileIDCUMap[CUID] = SrcId;
    // Print out a .file directive to specify files for .loc directives.
    Asm->EmitDwarfFileDirective(SrcId, DirName, FileName, CUID);

    return SrcId;
}

// Create new CompileUnit for the given metadata node with tag
// DW_TAG_compile_unit.
CompileUnit* DwarfDebug::constructCompileUnit(DICompileUnit* DIUnit)
{
    StringRef FN = DIUnit->getFilename();
    CompilationDir = DIUnit->getDirectory();

    DIE* Die = new DIE(dwarf::DW_TAG_compile_unit);
    CompileUnit* NewCU = new CompileUnit(GlobalCUIndexCount++, Die, DIUnit, Asm, this);

    FileIDCUMap[NewCU->getUniqueID()] = 0;
    // Call this to emit a .file directive if it wasn't emitted for the source
    // file this CU comes from yet.
    getOrCreateSourceID(FN, CompilationDir, NewCU->getUniqueID());

    auto producer = DIUnit->getProducer();
    auto strProducer = producer.str();
    if (producer.startswith("clang version"))
    {
        auto pos = strProducer.find("(");
        strProducer = strProducer.substr(0, pos);
        producer = strProducer.data();
    }
    NewCU->addString(Die, dwarf::DW_AT_producer, producer);
    NewCU->addUInt(Die, dwarf::DW_AT_language, dwarf::DW_FORM_data2, DIUnit->getSourceLanguage());
    NewCU->addString(Die, dwarf::DW_AT_name, FN);

    ModuleBeginSym = Asm->GetTempSymbol("module_begin", NewCU->getUniqueID());
    ModuleEndSym = Asm->GetTempSymbol("module_end", NewCU->getUniqueID());

    // Assumes in correct section after the entry point.
    Asm->EmitLabel(ModuleBeginSym);
    // 2.17.1 requires that we use DW_AT_low_pc for a single entry point
    // into an entity. We're using 0 (or a NULL label) for this.

    if (EmitSettings.EnableRelocation)
    {
        NewCU->addLabelAddress(Die, dwarf::DW_AT_low_pc, ModuleBeginSym);
        NewCU->addLabelAddress(Die, dwarf::DW_AT_high_pc, ModuleEndSym);

        NewCU->addLabelLoc(Die, dwarf::DW_AT_stmt_list, DwarfLineSectionSym);
    }
    else
    {
        auto highPC = m_pModule->getUnpaddedProgramSize();
        NewCU->addUInt(Die, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, 0);
        NewCU->addUInt(Die, dwarf::DW_AT_high_pc, Optional<dwarf::Form>(), highPC);

        // DW_AT_stmt_list is a offset of line number information for this
        // compile unit in debug_line section. For split dwarf this is
        // left in the skeleton CU and so not included.
        // The line table entries are not always emitted in assembly, so it
        // is not okay to use line_table_start here.
        NewCU->addUInt(Die, dwarf::DW_AT_stmt_list, dwarf::DW_FORM_sec_offset, 0);
    }

    simdWidth = m_pModule->GetSIMDSize();
    NewCU->addSimdWidth(Die, simdWidth);

    // If we're using split dwarf the compilation dir is going to be in the
    // skeleton CU and so we don't need to duplicate it here.
    if (!CompilationDir.empty())
    {
        NewCU->addString(Die, dwarf::DW_AT_comp_dir, CompilationDir);
    }

    // GD-215:
    // Add API and version
    auto lang = m_pModule->GetModule()->getNamedMetadata("igc.input.ir");
    if (lang && lang->getNumOperands() > 0)
    {
        auto mdNode = lang->getOperand(0);
        if (mdNode &&
            mdNode->getNumOperands() > 2)
        {
            auto op0 = dyn_cast_or_null<MDString>(mdNode->getOperand(0));
            auto op1 = dyn_cast_or_null<ConstantAsMetadata>(mdNode->getOperand(1));
            auto op2 = dyn_cast_or_null<ConstantAsMetadata>(mdNode->getOperand(2));

            if (op0 && op1 && op2)
            {
                if (op0->getString() == "ocl")
                {
                    std::string str;
                    str = "Intel OpenCL ";
#if LLVM_VERSION_MAJOR >= 13
                    str += toString(op1->getValue()->getUniqueInteger(), 10, false);
#else
                    str += op1->getValue()->getUniqueInteger().toString(10, false);
#endif
                    str += ".";
#if LLVM_VERSION_MAJOR >= 13
                    str += toString(op2->getValue()->getUniqueInteger(), 10, false);
#else
                    str += op2->getValue()->getUniqueInteger().toString(10, false);
#endif

                    NewCU->addString(Die, dwarf::DW_AT_description, llvm::StringRef(str));
                }
            }
        }
    }

    if (!FirstCU)
    {
        FirstCU = NewCU;
    }

    CUs.push_back(NewCU);

    CUMap.insert(std::make_pair(DIUnit, NewCU));
    CUDieMap.insert(std::make_pair(Die, NewCU));
    return NewCU;
}

// Construct subprogram DIE.
void DwarfDebug::constructSubprogramDIE(CompileUnit* TheCU, const MDNode* N)
{
    // FIXME: We should only call this routine once, however, during LTO if a
    // program is defined in multiple CUs we could end up calling it out of
    // beginModule as we walk the CUs.

    CompileUnit*& CURef = SPMap[N];
    if (CURef)
        return;
    CURef = TheCU;

    DISubprogram* SP = cast<DISubprogram>(const_cast<MDNode*>(N));
    if (!SP->isDefinition())
    {
        // This is a method declaration which will be handled while constructing
        // class type.
        return;
    }

    TheCU->getOrCreateSubprogramDIE(SP);
}

void DwarfDebug::discoverDISPNodes(DwarfDISubprogramCache &Cache)
{
    IGC_ASSERT(DISubprogramNodes.empty());
    DISubprogramNodes = Cache.findNodes(RegisteredFunctions);
}

void DwarfDebug::discoverDISPNodes()
{
    if (DISPCache)
    {
        discoverDISPNodes(*DISPCache);
    }
    else
    {
        DwarfDISubprogramCache TemporaryCache;
        discoverDISPNodes(TemporaryCache);
    }
}

// Emit all Dwarf sections that should come prior to the content. Create
// global DIEs and emit initial debug info sections.
void DwarfDebug::beginModule()
{
    const Module* M = m_pModule->GetModule();
    // If module has named metadata anchors then use them, otherwise scan the
    // module using debug info finder to collect debug info.
    NamedMDNode* CU_Nodes = M->getNamedMetadata("llvm.dbg.cu");
    if (!CU_Nodes) return;
    // discover DISubprogramNodes for all the registered visaModules
    discoverDISPNodes();
    // Emit initial sections so we can reference labels later.
    emitSectionLabels();

    for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i)
    {
        DICompileUnit* CUNode = cast<DICompileUnit>(CU_Nodes->getOperand(i));
        CompileUnit* CU = constructCompileUnit(CUNode);

        for (auto* DISP : DISubprogramNodes)
        {
            constructSubprogramDIE(CU, DISP);
        }

        auto EnumTypes = CUNode->getEnumTypes();
        for (unsigned i = 0, e = EnumTypes.size(); i != e; ++i)
        {
            CU->getOrCreateTypeDIE(EnumTypes[i]);
        }

        auto RetainedTypes = CUNode->getRetainedTypes();
        for (unsigned i = 0, e = RetainedTypes.size(); i != e; ++i)
        {
            CU->getOrCreateTypeDIE(RetainedTypes[i]);
        }

        // Assume there is a single CU
        break;
    }

    // Prime section data.
    SectionMap[Asm->GetTextSection()];

    if(DwarfFrameSectionNeeded())
    {
        Asm->SwitchSection(Asm->GetDwarfFrameSection());
        if (m_pModule->hasOrIsStackCall(*decodedDbg))
        {
            // First stack call CIE is written out,
            // next subroutine CIE if required.
            offsetCIEStackCall = 0;
            offsetCIESubroutine = writeStackcallCIE();
        }

        if (m_pModule->getSubroutines(*decodedDbg)->size() > 0)
        {
            //writeSubroutineCIE();
        }
    }
}

// Attach DW_AT_inline attribute with inlined subprogram DIEs.
void DwarfDebug::computeInlinedDIEs()
{
    // Attach DW_AT_inline attribute with inlined subprogram DIEs.
    for (SmallPtrSet<DIE*, 4>::iterator AI = InlinedSubprogramDIEs.begin(),
        AE = InlinedSubprogramDIEs.end(); AI != AE; ++AI)
    {
        DIE* ISP = *AI;
        FirstCU->addUInt(ISP, dwarf::DW_AT_inline, None, dwarf::DW_INL_inlined);
    }
    for (DenseMap<const MDNode*, DIE*>::iterator AI = AbstractSPDies.begin(),
        AE = AbstractSPDies.end(); AI != AE; ++AI)
    {
        DIE* ISP = AI->second;
        if (InlinedSubprogramDIEs.count(ISP))
            continue;
        FirstCU->addUInt(ISP, dwarf::DW_AT_inline, None, dwarf::DW_INL_inlined);
    }
}

// Collect info for variables that were optimized out.
void DwarfDebug::collectDeadVariables()
{
    const Module* M = m_pModule->GetModule();
    NamedMDNode* CU_Nodes = M->getNamedMetadata("llvm.dbg.cu");
    if (!CU_Nodes) return;

    for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i)
    {
        DICompileUnit* TheCU = cast<DICompileUnit>(CU_Nodes->getOperand(i));

        for (auto* SP : DISubprogramNodes)
        {
            if (!SP)
                continue;

            if (ProcessedSPNodes.count(SP) != 0 ||
                !isa<DISubprogram>(SP) || !SP->isDefinition())
            {
                continue;
            }
            auto Variables = SP->getRetainedNodes();
            if (Variables.size() == 0)
                continue;

            // Construct subprogram DIE and add variables DIEs.
            CompileUnit* SPCU = CUMap.lookup(TheCU);
            IGC_ASSERT_MESSAGE(SPCU, "Unable to find Compile Unit!");
            // FIXME: See the comment in constructSubprogramDIE about duplicate
            // subprogram DIEs.
            constructSubprogramDIE(SPCU, SP);
            DIE* SPDIE = SPCU->getDIE(SP);
            for (unsigned vi = 0, ve = Variables.size(); vi != ve; ++vi)
            {
                DIVariable* DV = cast<DIVariable>(Variables[i]);
                if (!isa<DILocalVariable>(DV))
                    continue;
                DbgVariable NewVar(cast<DILocalVariable>(DV), NULL, nullptr);
                if (DIE * VariableDIE = SPCU->constructVariableDIE(NewVar, false))
                {
                    SPDIE->addChild(VariableDIE);
                }
            }
        }

        // Assume there is a single CU
        break;
    }
}

void DwarfDebug::finalizeModuleInfo()
{
    // Collect info for variables that were optimized out.
    collectDeadVariables();

    // Attach DW_AT_inline attribute with inlined subprogram DIEs.
    computeInlinedDIEs();

    // Handle anything that needs to be done on a per-cu basis.
    for (DenseMap<const MDNode*, CompileUnit*>::iterator CUI = CUMap.begin(),
        CUE = CUMap.end(); CUI != CUE; ++CUI)
    {
        CompileUnit* TheCU = CUI->second;
        // Emit DW_AT_containing_type attribute to connect types with their
        // vtable holding type.
        TheCU->constructContainingTypeDIEs();
    }

    // Compute DIE offsets and sizes.
    computeSizeAndOffsets();
}

#if 0
// Disabled because getLabelBeginName() isnt present in LLVM 3.8

// Helper for sorting sections into a stable output order.
static bool SectionSort(const MCSection * A, const MCSection * B)
{
    std::string LA = (A ? A->getLabelBeginName() : "");
    std::string LB = (B ? B->getLabelBeginName() : "");
    return LA < LB;
}
#endif

void DwarfDebug::endSections()
{
    // Filter labels by section.
    for (size_t n = 0; n < ArangeLabels.size(); n++)
    {
        const SymbolCU& SCU = ArangeLabels[n];
        if (SCU.Sym->isInSection())
        {
            // Make a note of this symbol and it's section.
            const MCSection* Section = &SCU.Sym->getSection();
            if (!Section->getKind().isMetadata())
                SectionMap[Section].push_back(SCU);
        }
        else
        {
            // Some symbols (e.g. common/bss on mach-o) can have no section but still
            // appear in the output. This sucks as we rely on sections to build
            // arange spans. We can do it without, but it's icky.
            SectionMap[NULL].push_back(SCU);
        }
    }

    // Build a list of sections used.
    std::vector<const MCSection*> Sections;
    for (SectionMapType::iterator it = SectionMap.begin(); it != SectionMap.end(); it++)
    {
        const MCSection* Section = it->first;
        Sections.push_back(Section);
    }

    // Sort the sections into order.
    // This is only done to ensure consistent output order across different runs.
    //std::sort(Sections.begin(), Sections.end(), SectionSort);

    // Add terminating symbols for each section.
    for (unsigned ID = 0; ID < Sections.size(); ID++)
    {
        const MCSection* Section = Sections[ID];
        MCSymbol* Sym = NULL;

        if (Section)
        {
            // We can't call MCSection::getLabelEndName, as it's only safe to do so
            // if we know the section name up-front. For user-created sections, the resulting
            // label may not be valid to use as a label. (section names can use a greater
            // set of characters on some systems)
            Sym = Asm->GetTempSymbol("debug_end", ID);
            Asm->SwitchSection(Section);
            Asm->EmitLabel(Sym);
        }

        // Insert a final terminator.
        SectionMap[Section].push_back(SymbolCU(NULL, Sym));
    }
}

// Emit all Dwarf sections that should come after the content.
void DwarfDebug::endModule()
{
    if (!FirstCU) return;

    // Assumes in correct section after the entry point.
    Asm->SwitchSection(Asm->GetTextSection());
    Asm->EmitLabel(ModuleEndSym);

    // End any existing sections.
    // TODO: Does this need to happen?
    endSections();

    // Finalize the debug info for the module.
    finalizeModuleInfo();

    // Emit visible names into a debug str section.
    emitDebugStr();

    // Emit all the DIEs into a debug info section.
    emitDebugInfo();

    // Corresponding abbreviations into a abbrev section.
    emitAbbreviations();

    // Emit info into a debug loc section.
    emitDebugLoc();

    // Emit info into a debug ranges section.
    emitDebugRanges();

    // Emit info into a debug macinfo section.
    emitDebugMacInfo();

    // clean up.
    SPMap.clear();
    for (DenseMap<const MDNode*, CompileUnit*>::iterator I = CUMap.begin(), E = CUMap.end(); I != E; ++I)
    {
        auto CU = I->second;
        auto CUDie = CU->getCUDie();
        delete CUDie;
        delete CU;
    }
    CUMap.clear();

    // Reset these for the next Module if we have one.
    FirstCU = NULL;
}

// Find abstract variable, if any, associated with Var.
DbgVariable* DwarfDebug::findAbstractVariable(DIVariable* DV, DebugLoc ScopeLoc)
{
    // More then one inlined variable corresponds to one abstract variable.
    //DIVariable Var = cleanseInlinedVariable(DV, Ctx);
    DbgVariable* AbsDbgVariable = AbstractVariables.lookup(DV);
    if (AbsDbgVariable)
        return AbsDbgVariable;

    LexicalScope* Scope = LScopes.findAbstractScope(cast<DILocalScope>(ScopeLoc.getScope()));
    if (!Scope)
        return NULL;

    AbsDbgVariable = new DbgVariable(cast<DILocalVariable>(DV), NULL, nullptr);
    addScopeVariable(Scope, AbsDbgVariable);
    AbstractVariables[DV] = AbsDbgVariable;
    return AbsDbgVariable;
}

// If Var is a current function argument then add it to CurrentFnArguments list.
bool DwarfDebug::addCurrentFnArgument(const Function* MF, DbgVariable* Var, LexicalScope* Scope)
{
    const DILocalVariable* DV = Var->getVariable();
    unsigned ArgNo = DV->getArg();

    if (!LScopes.isCurrentFunctionScope(Scope) ||
        !DV->isParameter() ||
        ArgNo == 0)
    {
        return false;
    }

    size_t Size = CurrentFnArguments.size();
    if (Size == 0)
    {
        CurrentFnArguments.resize(MF->arg_size());
    }
    // llvm::Function argument size is not good indicator of how many
    // arguments does the function have at source level.
    if (ArgNo > Size)
    {
        CurrentFnArguments.resize(ArgNo * 2);
    }
    CurrentFnArguments[ArgNo - 1] = Var;
    return true;
}

template<typename T>
void write(std::vector<unsigned char>& vec, T data)
{
    unsigned char* base = (unsigned char*)& data;
    for (unsigned int i = 0; i != sizeof(T); i++)
        vec.push_back(*(base + i));
}

void write(std::vector<unsigned char>& vec, const unsigned char* data, uint8_t N)
{
    for (unsigned int i = 0; i != N; i++)
        vec.push_back(*(data + i));
}

void writeULEB128(std::vector<unsigned char>& vec, uint64_t data)
{
    auto uleblen = getULEB128Size(data);
    uint8_t* buf = (uint8_t*)malloc(uleblen * sizeof(uint8_t));
    encodeULEB128(data, buf);
    write(vec, buf, uleblen);
    free(buf);
}

// Find variables for each lexical scope.
void DwarfDebug::collectVariableInfo(const Function* MF, SmallPtrSet<const MDNode*, 16> & Processed)
{
    // Store pairs of <MDNode*, DILocation*> as we encounter them.
    // This allows us to emit 1 entry per function.
    std::vector<std::tuple<MDNode*, DILocation*, DbgVariable*>> addedEntries;
    std::map<llvm::DIScope*, std::vector<llvm::Instruction*>> instsInScope;

    TempDotDebugLocEntries.clear();

    auto isAdded = [&addedEntries](MDNode* md, DILocation* iat)
    {
        for (auto item : addedEntries)
        {
            if (std::get<0>(item) == md && std::get<1>(item) == iat)
                return std::get<2>(item);
        }
        return (DbgVariable*)nullptr;
    };

    auto findClosestStartEnd = [this](uint16_t start, uint16_t end)
    {
        std::pair<uint64_t, uint64_t> startEnd = std::make_pair(0, 0);
        if (start >= end)
            return startEnd;

        while (start < end)
        {
            auto startIt = m_pModule->VISAIndexToAllGenISAOff.find(start);
            if (startIt == m_pModule->VISAIndexToAllGenISAOff.end())
            {
                start++;
            }
            else
            {
                startEnd.first = startIt->second.front();
                break;
            }
        }

        if (start >= end)
            return startEnd;

        while (end > start && end > 0)
        {
            auto endIt = m_pModule->VISAIndexToAllGenISAOff.find(end);
            if (endIt == m_pModule->VISAIndexToAllGenISAOff.end())
            {
                end--;
            }
            else
            {
                startEnd.second = endIt->second.back();
                break;
            }
        }

        if (start >= end)
            return startEnd;

        return startEnd;
    };

    auto encodeImm = [&](IGC::DotDebugLocEntry& dotLoc, uint32_t offset,
        llvm::SmallVector<DotDebugLocEntry, 4>& TempDotDebugLocEntries,
        uint64_t rangeStart, uint64_t rangeEnd,
        uint32_t pointerSize, DbgVariable* RegVar, const ConstantInt* pConstInt)
    {
        auto oldSize = dotLoc.loc.size();

        auto op = llvm::dwarf::DW_OP_implicit_value;
        const unsigned int lebSize = 8;
        write(dotLoc.loc, (unsigned char*)&rangeStart, pointerSize);
        write(dotLoc.loc, (unsigned char*)&rangeEnd, pointerSize);
        write(dotLoc.loc, (uint16_t)(sizeof(uint8_t) + sizeof(const unsigned char) + lebSize));
        write(dotLoc.loc, (uint8_t)op);
        write(dotLoc.loc, (const unsigned char*)&lebSize, 1);
        if (isUnsignedDIType(this, RegVar->getType()))
        {
            uint64_t constValue = pConstInt->getZExtValue();
            write(dotLoc.loc, (unsigned char*)&constValue, lebSize);
        }
        else
        {
            int64_t constValue = pConstInt->getSExtValue();
            write(dotLoc.loc, (unsigned char*)&constValue, lebSize);
        }
        offset += dotLoc.loc.size() - oldSize;

        TempDotDebugLocEntries.push_back(dotLoc);
    };

    auto encodeReg = [&](IGC::DotDebugLocEntry& dotLoc, uint32_t offset,
        llvm::SmallVector<DotDebugLocEntry, 4>& TempDotDebugLocEntries,
        uint64_t startRange, uint64_t endRange,
        uint32_t pointerSize, DbgVariable* RegVar, std::vector<VISAVariableLocation>& Locs,
        DbgDecoder::LiveIntervalsVISA& genIsaRange)
    {
        auto allCallerSave = m_pModule->getAllCallerSave(*decodedDbg,
                                                         startRange, endRange, genIsaRange);
        std::vector<DbgDecoder::LiveIntervalsVISA> vars = { genIsaRange };

        auto oldSize = dotLoc.loc.size();
        dotLoc.start = startRange;
        TempDotDebugLocEntries.push_back(dotLoc);
        write(TempDotDebugLocEntries.back().loc, (unsigned char*)&startRange, pointerSize);

        for (auto it : allCallerSave)
        {
            TempDotDebugLocEntries.back().end = std::get<0>(it);
            write(TempDotDebugLocEntries.back().loc, (unsigned char*)&std::get<0>(it), pointerSize);
            auto block = FirstCU->buildGeneral(*RegVar, &Locs, &vars);
            std::vector<unsigned char> buffer;
            if (block)
                block->EmitToRawBuffer(buffer);
            write(TempDotDebugLocEntries.back().loc, (uint16_t)buffer.size());
            write(TempDotDebugLocEntries.back().loc, buffer.data(), buffer.size());

            offset += TempDotDebugLocEntries.back().loc.size() - oldSize;

            DotDebugLocEntry another(dotLoc.getStart(), dotLoc.getEnd(), dotLoc.getDbgInst(), dotLoc.getVariable());
            another.start = std::get<0>(it);
            another.end = std::get<1>(it);
            TempDotDebugLocEntries.push_back(another);
            oldSize = TempDotDebugLocEntries.back().loc.size();
            // write actual caller save location
            write(TempDotDebugLocEntries.back().loc, (unsigned char*)&std::get<0>(it), pointerSize);
            write(TempDotDebugLocEntries.back().loc, (unsigned char*)&std::get<1>(it), pointerSize);
            auto callerSaveVars = vars;
            callerSaveVars.front().var.physicalType = DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory;
            callerSaveVars.front().var.mapping.m.isBaseOffBEFP = 0;
            callerSaveVars.front().var.mapping.m.memoryOffset = std::get<2>(it);
            block = FirstCU->buildGeneral(*RegVar, &Locs, &callerSaveVars);
            buffer.clear();
            if (block)
                block->EmitToRawBuffer(buffer);
            write(TempDotDebugLocEntries.back().loc, (uint16_t)buffer.size());
            write(TempDotDebugLocEntries.back().loc, buffer.data(), buffer.size());

            offset += TempDotDebugLocEntries.back().loc.size() - oldSize;

            if (std::get<1>(it) >= endRange)
                return;

            // start new interval with original location
            DotDebugLocEntry yetAnother(dotLoc.getStart(), dotLoc.getEnd(), dotLoc.getDbgInst(), dotLoc.getVariable());
            yetAnother.start = std::get<1>(it);
            TempDotDebugLocEntries.push_back(yetAnother);
            oldSize = TempDotDebugLocEntries.back().loc.size();
            write(TempDotDebugLocEntries.back().loc, (unsigned char*)&std::get<1>(it), pointerSize);
        }

        TempDotDebugLocEntries.back().end = endRange;
        write(TempDotDebugLocEntries.back().loc, (unsigned char*)&endRange, pointerSize);

        auto block = FirstCU->buildGeneral(*RegVar, &Locs, &vars);
        std::vector<unsigned char> buffer;
        if (block)
            block->EmitToRawBuffer(buffer);
        write(TempDotDebugLocEntries.back().loc, (uint16_t)buffer.size());
        write(TempDotDebugLocEntries.back().loc, buffer.data(), buffer.size());

        offset += TempDotDebugLocEntries.back().loc.size() - oldSize;
    };

    unsigned int offset = 0;
    unsigned int pointerSize = m_pModule->getPointerSize();
    for (SmallVectorImpl<const MDNode*>::const_iterator UVI = UserVariables.begin(),
        UVE = UserVariables.end(); UVI != UVE; ++UVI)
    {
        const MDNode* Var = *UVI;
        if (Processed.count(Var))
            continue;

        // History contains relevant DBG_VALUE instructions for Var and instructions
        // clobbering it.
        SmallVectorImpl<const Instruction*>& History = DbgValues[Var];
        if (History.empty())
            continue;

        auto origLocSize = TempDotDebugLocEntries.size();

        // This loop was added during upgrade to clang 3.8.1. Upto clang 3.6, each inlined function copy
        // got unique version of auto variable metadata nodes. This is because each auto variable had
        // "inlinedAt" field as part of its metadata. With clang 3.8.1, inlinedAt field is no
        // longer part of variable metadata node. This causes auto variable nodes of all inlined functions
        // to collapse in to a single metadata node. Following loop iterates over all dbg.declare instances
        // for inlined functions and creates new DbgVariable instances for each.

        // DbgVariable is created once per variable to be emitted to dwarf.
        // If a function is inlined x times, there would be x number of DbgVariable instances.
        std::unordered_map<DbgVariable*, std::list<std::tuple<unsigned int, unsigned int, DbgVariable*, const llvm::Instruction*>>> DbgValuesWithGenIP;
        for (auto HI = History.begin(), HE = History.end(); HI != HE; HI++)
        {
            auto H = (*HI);
            DIVariable* DV = cast<DIVariable>(const_cast<MDNode*>(Var));

            LexicalScope* Scope = NULL;
            if (DV->getTag() == dwarf::DW_TAG_formal_parameter &&
                DV->getScope() &&
                DV->getScope()->getName() == MF->getName())
            {
                Scope = LScopes.getCurrentFunctionScope();
            }
            else if (auto IA = H->getDebugLoc().getInlinedAt())
            {
                Scope = LScopes.findInlinedScope(cast<DILocalScope>(DV->getScope()),
                    IA);
            }
            else
            {
                Scope = LScopes.findLexicalScope(cast<DILocalScope>(DV->getScope()));
            }

            // If variable scope is not found then skip this variable.
            if (!Scope)
                continue;

            Processed.insert(DV);
            const Instruction* pInst = H; // History.front();

            IGC_ASSERT_MESSAGE(m_pModule->IsDebugValue(pInst), "History must begin with debug value");
            DbgVariable* AbsVar = findAbstractVariable(DV, pInst->getDebugLoc());
            DbgVariable* RegVar = nullptr;

            auto prevRegVar = isAdded(DV, pInst->getDebugLoc().getInlinedAt());

            if (!prevRegVar)
            {
                RegVar = new DbgVariable(cast<DILocalVariable>(DV),
                    AbsVar != nullptr ? AbsVar->getLocation() : nullptr, AbsVar);

                if (!addCurrentFnArgument(MF, RegVar, Scope))
                {
                    addScopeVariable(Scope, RegVar);
                }
                addedEntries.push_back(std::make_tuple(DV, pInst->getDebugLoc().getInlinedAt(), RegVar));

                RegVar->setDbgInst(pInst);
            }
            else
                RegVar = prevRegVar;

            if (AbsVar)
            {
                AbsVar->setDbgInst(pInst);
            }

            // Conditions below decide whether we want to emit location to debug_loc or inline it
            // in the DIE. To inline in DIE, we simply dont emit anything here and continue the loop.
            bool needsCallerSave = m_pModule->getCompileUnit(*decodedDbg)->cfi.numCallerSaveEntries > 0;
            if (!EmitSettings.EmitDebugLoc && !needsCallerSave)
                continue;

            if (EmitSettings.UseOffsetInLocation && isa<DbgDeclareInst>(pInst))
            {
                // When using OffsetInLocation, we emit offset of variable from privateBase.
                // This works only for -O0 when variables are stored in memory.
                // When optimizations are enabled, ie when pInst is not dbgDeclare, we
                // may choose to emit locations to debug_loc as variables may be mapped to
                // registers.
                continue;
            }

            // assume that VISA preserves location thoughout its lifetime
            auto Locs = m_pModule->GetVariableLocation(pInst);
            auto& Loc = Locs.front();

            if (Loc.IsSampler() || Loc.IsSLM() ||
                Loc.HasSurface())
            {
                // Assume location of these types doesnt change
                // throughout program. Revisit this if required.
                continue;
            }

            auto start = (*HI);
            auto end = start;

            if (HI + 1 != HE)
                end = HI[1];
            else
            {
                // Find loc of last instruction in current function (same IAT)
                auto lastIATit = SameIATInsts.find(start->getDebugLoc().getInlinedAt());
                if (lastIATit != SameIATInsts.end())
                    end = (*lastIATit).second.back();
            }

            IGC::InsnRange InsnRange(start, end);
            auto GenISARange = m_pModule->getGenISARange(InsnRange);

            // Emit location within the DIE for dbg.declare
            if (History.size() == 1 && isa<DbgDeclareInst>(pInst) && !needsCallerSave)
                continue;

            for (auto range : GenISARange)
            {
                DbgValuesWithGenIP[RegVar].push_back(std::make_tuple(range.first, range.second, RegVar, pInst));
            }
        }

        DIVariable* DV = cast<DIVariable>(const_cast<MDNode*>(Var));
        for (auto& d : DbgValuesWithGenIP)
        {
            d.second.sort([](std::tuple<unsigned int, unsigned int, DbgVariable*, const llvm::Instruction*>& first,
                std::tuple<unsigned int, unsigned int, DbgVariable*, const llvm::Instruction*>& second)
                {
                    return std::get<0>(first) < std::get<0>(second);
                });

            struct PrevLoc
            {
                enum class Type
                {
                    Empty = 0,
                    Imm = 1,
                    Reg = 2
                };
                Type t = Type::Empty;
                uint64_t start = 0;
                uint64_t end = 0;
                DbgVariable* dbgVar = nullptr;
                const llvm::Instruction* pInst = nullptr;
                const ConstantInt* imm = nullptr;

                std::vector<VISAVariableLocation> Locs;
                DbgDecoder::LiveIntervalsVISA genIsaRange;
            };

            PrevLoc p;
            auto encodePrevLoc = [&](DotDebugLocEntry& dotLoc, llvm::SmallVector<DotDebugLocEntry, 4>& TempDotDebugLocEntries, unsigned int& offset)
            {
                if (p.dbgVar->getDotDebugLocOffset() == ~0U)
                {
                    p.dbgVar->setDotDebugLocOffset(offset);
                }
                if (p.t == PrevLoc::Type::Imm)
                {
                    encodeImm(dotLoc, offset, TempDotDebugLocEntries, p.start, p.end, pointerSize, p.dbgVar, p.imm);
                }
                else
                {
                    encodeReg(dotLoc, offset, TempDotDebugLocEntries, p.start, p.end, pointerSize, p.dbgVar, p.Locs, p.genIsaRange);
                }
                p.t = PrevLoc::Type::Empty;
            };
            for (auto& range : d.second)
            {
                auto startIp = std::get<0>(range);
                auto endIp = std::get<1>(range);
                auto RegVar = std::get<2>(range);
                auto pInst = std::get<3>(range);

                auto Locs = m_pModule->GetVariableLocation(pInst);
                auto& Loc = Locs.front();

                // Variable has a constant value so inline it in DIE
                if (d.second.size() == 1 && Loc.IsImmediate())
                    continue;

                DotDebugLocEntry dotLoc(startIp, endIp, pInst, DV);
                dotLoc.setOffset(offset);

                if (Loc.IsImmediate())
                {
                    const Constant* pConstVal = Loc.GetImmediate();
                    if (const ConstantInt* pConstInt = dyn_cast<ConstantInt>(pConstVal))
                    {
                        // Always emit an 8-byte value
                        uint64_t rangeStart = startIp;
                        uint64_t rangeEnd = endIp;

                        if (p.t == PrevLoc::Type::Imm &&
                            p.end < rangeEnd &&
                            p.imm == pConstInt)
                        {
                            // extend
                            p.end = rangeEnd;
                            continue;
                        }

                        if (p.end >= rangeEnd)
                            continue;

                        if (rangeStart == rangeEnd)
                            continue;

                        if (p.t != PrevLoc::Type::Empty)
                        {
                            // Emit previous location to debug_loc
                            encodePrevLoc(dotLoc, TempDotDebugLocEntries, offset);
                        }

                        p.t = PrevLoc::Type::Imm;
                        p.start = rangeStart;
                        p.end = rangeEnd;
                        p.imm = pConstInt;
                        p.dbgVar = RegVar;
                        p.pInst = pInst;
                    }
                }
                else if (Loc.IsRegister())
                {
                    DbgDecoder::VarInfo varInfo;
                    auto regNum = Loc.GetRegister();
                    m_pModule->getVarInfo(*decodedDbg, "V", regNum, varInfo);
                    for (auto& genIsaRange : varInfo.lrs)
                    {
                        auto startEnd = findClosestStartEnd(genIsaRange.start, genIsaRange.end);

                        uint64_t startRange = startEnd.first;
                        uint64_t endRange = startEnd.second;

                        if (startRange == endRange)
                            continue;

                        if (endRange < startIp)
                            continue;
                        if (startRange > endIp)
                            continue;

                        startRange = std::max(startRange, (uint64_t)startIp);
                        endRange = std::min(endRange, (uint64_t)endIp);

                        if (p.t == PrevLoc::Type::Reg &&
                            p.end < endRange)
                        {
                            if ((p.genIsaRange.isGRF() && genIsaRange.isGRF() &&
                                p.genIsaRange.getGRF() == genIsaRange.getGRF()) ||
                                (p.genIsaRange.isSpill() && genIsaRange.isSpill() &&
                                    p.genIsaRange.getSpillOffset() == genIsaRange.getSpillOffset()))
                            {
                                // extend
                                p.end = endRange;
                                continue;
                            }
                        }

                        if (p.end >= endRange)
                            continue;

                        if (startRange == endRange)
                            continue;

                        if (p.t != PrevLoc::Type::Empty)
                        {
                            encodePrevLoc(dotLoc, TempDotDebugLocEntries, offset);
                        }

                        p.t = PrevLoc::Type::Reg;
                        p.start = startRange;
                        p.end = endRange;
                        p.dbgVar = RegVar;
                        p.Locs = Locs;
                        p.genIsaRange = genIsaRange;
                        p.pInst = pInst;
                    }
                }
            }

            if (p.t != PrevLoc::Type::Empty)
            {
                DotDebugLocEntry dotLoc(p.start, p.end, p.pInst, DV);
                dotLoc.setOffset(offset);
                encodePrevLoc(dotLoc, TempDotDebugLocEntries, offset);
            }

            if (TempDotDebugLocEntries.size() > origLocSize)
            {
                TempDotDebugLocEntries.push_back(DotDebugLocEntry());
                offset += pointerSize * 2;
            }
        }
    }

    // Collect info for variables that were optimized out.
    LexicalScope* FnScope = LScopes.getCurrentFunctionScope();
    auto Variables = cast<DISubprogram>(FnScope->getScopeNode())->getRetainedNodes();

    for (unsigned i = 0, e = Variables.size(); i != e; ++i)
    {
        DILocalVariable* DV = cast_or_null<DILocalVariable>(Variables[i]);
        if (!DV || !Processed.insert(DV).second)
            continue;
        if (LexicalScope * Scope = LScopes.findLexicalScope(DV->getScope()))
        {
            addScopeVariable(Scope, new DbgVariable(DV, NULL, nullptr));
        }
    }
}

llvm::MCSymbol* DwarfDebug::CopyDebugLoc(unsigned int o)
{
    // TempDotLocEntries has all entries discovered in collectVariableInfo.
    // But some of those entries may not get emitted. This function
    // is invoked when writing out DIE. At this time, it can be decided
    // whether debug_range for a variable will be emitted to debug_ranges.
    // If yes, it is copied over to DotDebugLocEntries and new offset is
    // returned.
    unsigned int offset = 0, index = 0;
    bool found = false, done = false;
    unsigned int pointerSize = m_pModule->getPointerSize();

    // Compute offset in DotDebugLocEntries
    for (auto& item : DotDebugLocEntries)
    {
        if (item.isEmpty())
            offset += pointerSize * 2;
        else
            offset += item.loc.size();
    }

    auto Label = Asm->GetTempSymbol("debug_loc", offset);
    auto RetLabel = Label;

    while (!done)
    {
        if (!found &&
            TempDotDebugLocEntries[index].getOffset() == o)
        {
            found = true;
        }
        else if (!found)
        {
            index++;
            continue;
        }

        if (found)
        {
            // Append data to DotLocEntries
            auto& Entry = TempDotDebugLocEntries[index];
            Entry.setSymbol(Label);
            Label = nullptr;
            DotDebugLocEntries.push_back(Entry);
            if (TempDotDebugLocEntries[index].isEmpty())
            {
                done = true;
            }
        }
        index++;
    }

    return RetLabel;
}

unsigned int DwarfDebug::CopyDebugLocNoReloc(unsigned int o)
{
    // TempDotLocEntries has all entries discovered in collectVariableInfo.
    // But some of those entries may not get emitted. This function
    // is invoked when writing out DIE. At this time, it can be decided
    // whether debug_range for a variable will be emitted to debug_ranges.
    // If yes, it is copied over to DotDebugLocEntries and new offset is
    // returned.
    unsigned int offset = 0, index = 0;
    bool found = false, done = false;
    unsigned int pointerSize = m_pModule->getPointerSize();

    // Compute offset in DotDebugLocEntries
    for (auto& item : DotDebugLocEntries)
    {
        if (item.isEmpty())
            offset += pointerSize * 2;
        else
            offset += item.loc.size();
    }

    while (!done)
    {
        if (!found &&
            TempDotDebugLocEntries[index].getOffset() == o)
        {
            found = true;
        }
        else if (!found)
        {
            index++;
            continue;
        }

        if (found)
        {
            // Append data to DotLocEntries
            DotDebugLocEntries.push_back(TempDotDebugLocEntries[index]);
            if (TempDotDebugLocEntries[index].isEmpty())
            {
                done = true;
            }
        }
        index++;
    }

    return offset;
}

// Process beginning of an instruction.
void DwarfDebug::beginInstruction(const Instruction* MI, bool recordSrcLine)
{
    // Check if source location changes, but ignore DBG_VALUE locations.
    if (!m_pModule->IsDebugValue(MI) &&
        recordSrcLine)
    {
        DebugLoc DL = MI->getDebugLoc();
        if (DL && DL != PrevInstLoc)
        {
            unsigned Flags = 0;
            PrevInstLoc = DL;
            if (DL == PrologEndLoc)
            {
                Flags |= DWARF2_FLAG_PROLOGUE_END;
                PrologEndLoc = DebugLoc();
            }
            if (!PrologEndLoc)
            {
                bool setIsStmt = true;
                auto line = DL.getLine();
                auto inlinedAt = DL.getInlinedAt();
                auto it = isStmtSet.find(line);

                if (it != isStmtSet.end())
                {
                    // is_stmt is set only if line#,
                    // inlinedAt combination is
                    // never seen before.
                    auto& iat = (*it).second;
                    for (auto& item : iat)
                    {
                        if (item == inlinedAt)
                        {
                            setIsStmt = false;
                            break;
                        }
                    }
                }

                if (setIsStmt)
                {
                    Flags |= DWARF2_FLAG_IS_STMT;

                    isStmtSet[line].push_back(inlinedAt);
                }
            }

            const MDNode* Scope = DL.getScope();
            recordSourceLine(DL.getLine(), DL.getCol(), Scope, Flags);
        }
    }

    // Insert labels where requested.
    DenseMap<const Instruction*, MCSymbol*>::iterator I = LabelsBeforeInsn.find(MI);

    // No label needed or Label already assigned.
    if (I == LabelsBeforeInsn.end() || I->second)
        return;

    if (!PrevLabel)
    {
        PrevLabel = Asm->CreateTempSymbol();
        Asm->EmitLabel(PrevLabel);
    }
    I->second = PrevLabel;
}

// Process end of an instruction.
void DwarfDebug::endInstruction(const Instruction* MI)
{
    // Don't create a new label after DBG_VALUE instructions.
    // They don't generate code.
    if (!m_pModule->IsDebugValue(MI))
        PrevLabel = 0;

    DenseMap<const Instruction*, MCSymbol*>::iterator I = LabelsAfterInsn.find(MI);

    // No label needed or Label already assigned.
    if (I == LabelsAfterInsn.end() || I->second)
        return;

    // We need a label after this instruction.
    if (!PrevLabel)
    {
        PrevLabel = Asm->CreateTempSymbol();
        Asm->EmitLabel(PrevLabel);
    }
    I->second = PrevLabel;
}

// Each LexicalScope has first instruction and last instruction to mark
// beginning and end of a scope respectively. Create an inverse map that list
// scopes starts (and ends) with an instruction. One instruction may start (or
// end) multiple scopes. Ignore scopes that are not reachable.
void DwarfDebug::identifyScopeMarkers()
{
    SmallVector<LexicalScope*, 4> WorkList;
    WorkList.push_back(LScopes.getCurrentFunctionScope());
    while (!WorkList.empty())
    {
        LexicalScope* S = WorkList.pop_back_val();

        const SmallVectorImpl<LexicalScope*>& Children = S->getChildren();
        if (!Children.empty())
        {
            for (SmallVectorImpl<LexicalScope*>::const_iterator SI = Children.begin(),
                SE = Children.end(); SI != SE; ++SI)
            {
                WorkList.push_back(*SI);
            }
        }

        if (S->isAbstractScope())
            continue;

        const SmallVectorImpl<InsnRange>& Ranges = S->getRanges();
        if (Ranges.empty())
            continue;
        for (SmallVectorImpl<InsnRange>::const_iterator RI = Ranges.begin(),
            RE = Ranges.end(); RI != RE; ++RI)
        {
            IGC_ASSERT_MESSAGE(RI->first, "InsnRange does not have first instruction!");
            IGC_ASSERT_MESSAGE(RI->second, "InsnRange does not have second instruction!");
            requestLabelBeforeInsn(RI->first);
            requestLabelAfterInsn(RI->second);
        }
    }
}

// Walk up the scope chain of given debug loc and find line number info
// for the function.
static DebugLoc getFnDebugLoc(DebugLoc DL, const LLVMContext& Ctx)
{
    // Get MDNode for DebugLoc's scope.
    while (DILocation * InlinedAt = DL.getInlinedAt())
    {
        DL = DebugLoc(InlinedAt);
    }
    const MDNode* Scope = DL.getScope();

    DISubprogram* SP = getDISubprogram(Scope);
    if (SP)
    {
        // Check for number of operands since the compatibility is cheap here.
        if (SP->getNumOperands() > 19)
        {
            return DILocation::get(SP->getContext(), SP->getScopeLine(), 0, SP);
        }
        return DILocation::get(SP->getContext(), SP->getLine(), 0, SP);
    }

    return DebugLoc();
}

// Gather pre-function debug information.  Assumes being called immediately
// after the function entry point has been emitted.
void DwarfDebug::beginFunction(const Function* MF, IGC::VISAModule* v)
{
    // Reset PrologEndLoc so that when processing next function with same DwarfDebug
    // instance doesnt use stale value.
    PrologEndLoc = DebugLoc();
    // Clear stale isStmt from previous function compilation.
    isStmtSet.clear();
    m_pModule = v;

    // Grab the lexical scopes for the function, if we don't have any of those
    // then we're not going to be able to do anything.
    LScopes.initialize(m_pModule);
    if (LScopes.empty())
        return;

    IGC_ASSERT_MESSAGE(UserVariables.empty(), "Maps weren't cleaned");
    IGC_ASSERT_MESSAGE(DbgValues.empty(), "Maps weren't cleaned");

    // Make sure that each lexical scope will have a begin/end label.
    identifyScopeMarkers();

    // Set DwarfCompileUnitID in MCContext to the Compile Unit this function
    // belongs to so that we add to the correct per-cu line table in the
    // non-asm case.
    LexicalScope* FnScope = LScopes.getCurrentFunctionScope();
    CompileUnit* TheCU = SPMap.lookup(FnScope->getScopeNode());
    IGC_ASSERT_MESSAGE(TheCU, "Unable to find compile unit!");
    Asm->SetDwarfCompileUnitID(TheCU->getUniqueID());

    // Emit a label for the function so that we have a beginning address.
    FunctionBeginSym = Asm->GetTempSymbol("func_begin", m_pModule->GetFuncId());
    // Assumes in correct section after the entry point.
    Asm->EmitLabel(FunctionBeginSym);

    llvm::MDNode* prevIAT = nullptr;

    for (auto II = m_pModule->begin(), IE = m_pModule->end(); II != IE; ++II)
    {
        const Instruction* MI = *II;
        auto Loc = MI->getDebugLoc();

        if (Loc &&
            Loc.getScope() != prevIAT)
        {
            SameIATInsts[Loc.getInlinedAt()].push_back(MI);
            prevIAT = Loc.getInlinedAt();
        }

        if (m_pModule->IsDebugValue(MI))
        {
            IGC_ASSERT_MESSAGE(MI->getNumOperands() > 1, "Invalid machine instruction!");

            // Keep track of user variables.
            const MDNode* Var = m_pModule->GetDebugVariable(MI);

            // Check the history of this variable.
            SmallVectorImpl<const Instruction*>& History = DbgValues[Var];
            if (History.empty())
            {
                UserVariables.push_back(Var);
                // The first mention of a function argument gets the FunctionBeginSym
                // label, so arguments are visible when breaking at function entry.
                const DIVariable* DV = cast_or_null<DIVariable>(Var);
                if (DV && DV->getTag() == dwarf::DW_TAG_formal_parameter &&
                    getDISubprogram(DV->getScope())->describes(MF))
                {
                    LabelsBeforeInsn[MI] = FunctionBeginSym;
                }
            }
            else
            {
                // We have seen this variable before. Try to coalesce DBG_VALUEs.
                const Instruction* Prev = History.back();
                if (m_pModule->IsDebugValue(Prev))
                {
                    // Coalesce identical entries at the end of History.
                    if (History.size() >= 2 &&
                        Prev->isIdenticalTo(History[History.size() - 2]))
                    {
                        LLVM_DEBUG(dbgs() << "Coalescing identical DBG_VALUE entries:\n"
                            << "\t" << *Prev << "\t"
                            << *History[History.size() - 2] << "\n");
                        History.pop_back();
                    }
                }
            }
            History.push_back(MI);
        }
        else if(m_pModule->IsExecutableInst(*MI))
        {
            // Not a DBG_VALUE instruction.

            // First known non-DBG_VALUE and non-frame setup location marks
            // the beginning of the function body.
            if (!PrologEndLoc && Loc)
            {
                PrologEndLoc = Loc;
            }
        }
    }

    for (DbgValueHistoryMap::iterator I = DbgValues.begin(), E = DbgValues.end(); I != E; ++I)
    {
        SmallVectorImpl<const Instruction*>& History = I->second;
        if (History.empty())
            continue;

        // Request labels for the full history.
        for (unsigned i = 0, e = History.size(); i != e; ++i)
        {
            const Instruction* MI = History[i];
            if (m_pModule->IsDebugValue(MI))
                requestLabelBeforeInsn(MI);
            else
                requestLabelAfterInsn(MI);
        }
    }

    PrevInstLoc = DebugLoc();
    PrevLabel = FunctionBeginSym;

    // Record beginning of function.
    if (PrologEndLoc)
    {
        DebugLoc FnStartDL = getFnDebugLoc(PrologEndLoc, MF->getContext());
        const MDNode* Scope = FnStartDL.getScope();
        // We'd like to list the prologue as "not statements" but GDB behaves
        // poorly if we do that. Revisit this with caution/GDB (7.5+) testing.
        recordSourceLine(FnStartDL.getLine(), FnStartDL.getCol(), Scope, DWARF2_FLAG_IS_STMT);
    }
}

void DwarfDebug::addScopeVariable(LexicalScope* LS, DbgVariable* Var)
{
    SmallVectorImpl<DbgVariable*>& Vars = ScopeVariables[LS];
    const DILocalVariable* DV = Var->getVariable();
    // Variables with positive arg numbers are parameters.
    if (unsigned ArgNum = DV->getArg())
    {
        // Keep all parameters in order at the start of the variable list to ensure
        // function types are correct (no out-of-order parameters)
        //
        // This could be improved by only doing it for optimized builds (unoptimized
        // builds have the right order to begin with), searching from the back (this
        // would catch the unoptimized case quickly), or doing a binary search
        // rather than linear search.
        SmallVectorImpl<DbgVariable*>::iterator I = Vars.begin();
        while (I != Vars.end())
        {
            unsigned CurNum = (*I)->getVariable()->getArg();
            // A local (non-parameter) variable has been found, insert immediately
            // before it.
            if (CurNum == 0)
                break;
            // A later indexed parameter has been found, insert immediately before it.
            if (CurNum > ArgNum)
                break;
            ++I;
        }
        Vars.insert(I, Var);
        return;
    }

    Vars.push_back(Var);
}

// Gather and emit post-function debug information.
void DwarfDebug::endFunction(const Function* MF)
{
    if (LScopes.empty()) return;

    // Define end label for subprogram.
    FunctionEndSym = Asm->GetTempSymbol("func_end", m_pModule->GetFuncId());
    // Assumes in correct section after the entry point.
    Asm->EmitLabel(FunctionEndSym);

    Asm->EmitELFDiffSize(FunctionBeginSym, FunctionEndSym, FunctionBeginSym);

    // Set DwarfCompileUnitID in MCContext to default value.
    Asm->SetDwarfCompileUnitID(0);

    SmallPtrSet<const MDNode*, 16> ProcessedVars;
    collectVariableInfo(MF, ProcessedVars);

    LexicalScope* FnScope = LScopes.getCurrentFunctionScope();
    CompileUnit* TheCU = SPMap.lookup(FnScope->getScopeNode());
    IGC_ASSERT_MESSAGE(TheCU, "Unable to find compile unit!");

    // Construct abstract scopes.
    ArrayRef<LexicalScope*> AList = LScopes.getAbstractScopesList();
    for (unsigned i = 0, e = AList.size(); i != e; ++i)
    {
        LexicalScope* AScope = AList[i];
        const DISubprogram* SP = cast_or_null<DISubprogram>(AScope->getScopeNode());
        if (SP)
        {
            // Collect info for variables that were optimized out.
            auto Variables = SP->getRetainedNodes();
            for (unsigned i = 0, e = Variables.size(); i != e; ++i)
            {
                DILocalVariable* DV = cast_or_null<DILocalVariable>(Variables[i]);
                if (!DV || !ProcessedVars.insert(DV).second) continue;
                // Check that DbgVariable for DV wasn't created earlier, when
                // findAbstractVariable() was called for inlined instance of DV.
                //LLVMContext &Ctx = DV->getContext();
                //DIVariable CleanDV = cleanseInlinedVariable(DV, Ctx);
                //if (AbstractVariables.lookup(CleanDV)) continue;
                if (LexicalScope * Scope = LScopes.findAbstractScope(DV->getScope()))
                {
                    addScopeVariable(Scope, new DbgVariable(DV, NULL, nullptr));
                }
            }
        }
        if (ProcessedSPNodes.count(AScope->getScopeNode()) == 0)
        {
            constructScopeDIE(TheCU, AScope);
        }
    }

    constructScopeDIE(TheCU, FnScope);

    if (DwarfFrameSectionNeeded())
    {
        Asm->SwitchSection(Asm->GetDwarfFrameSection());
        if (m_pModule->hasOrIsStackCall(*decodedDbg))
        {
            writeFDEStackCall(m_pModule);
        }
        else
        {
            //writeFDESubroutine(m_pModule);
        }
    }

    // Clear debug info
    for (ScopeVariablesMap::iterator
        I = ScopeVariables.begin(), E = ScopeVariables.end(); I != E; ++I)
    {
        for (auto V : I->second)
            delete V;
        I->second.clear();
    }
    ScopeVariables.clear();
    for (auto V : CurrentFnArguments)
        delete V;
    CurrentFnArguments.clear();
    UserVariables.clear();
    DbgValues.clear();
    AbstractVariables.clear();
    LabelsBeforeInsn.clear();
    LabelsAfterInsn.clear();
    PrevLabel = NULL;
}

// Register a source line with debug info. Returns the  unique label that was
// emitted and which provides correspondence to the source line list.
void DwarfDebug::recordSourceLine(
    unsigned Line, unsigned Col, const MDNode* S, unsigned Flags)
{
    StringRef Fn;
    StringRef Dir;
    unsigned Src = 1;
    if (S)
    {
        if (isa<DICompileUnit>(S))
        {
            const DICompileUnit* CU = cast<DICompileUnit>(S);
            Fn = CU->getFilename();
            Dir = CU->getDirectory();
        }
        else if (isa<DIFile>(S))
        {
            const DIFile* F = cast<DIFile>(S);
            Fn = F->getFilename();
            Dir = F->getDirectory();
        }
        else if (isa<DISubprogram>(S))
        {
            const DISubprogram* SP = cast<DISubprogram>(S);
            Fn = SP->getFilename();
            Dir = SP->getDirectory();
        }
        else if (isa<DILexicalBlockFile>(S))
        {
            const DILexicalBlockFile* DBF = cast<DILexicalBlockFile>(S);
            Fn = DBF->getFilename();
            Dir = DBF->getDirectory();
        }
        else if (isa<DILexicalBlock>(S))
        {
            const DILexicalBlock* DB = cast<DILexicalBlock>(S);
            Fn = DB->getFilename();
            Dir = DB->getDirectory();
        }
        else
        {
            IGC_ASSERT_EXIT_MESSAGE(0, "Unexpected scope info");
        }

        Src = getOrCreateSourceID(Fn, Dir, Asm->GetDwarfCompileUnitID());
    }
    Asm->EmitDwarfLocDirective(Src, Line, Col, Flags, 0, 0, Fn);
}

//===----------------------------------------------------------------------===//
// Emit Methods
//===----------------------------------------------------------------------===//

// Compute the size and offset of a DIE. The offset is relative to start of the
// CU. It returns the offset after laying out the DIE.
unsigned DwarfDebug::computeSizeAndOffset(DIE* Die, unsigned Offset)
{
    // Get the children.
    const std::vector<DIE*>& Children = Die->getChildren();

    // Record the abbreviation.
    assignAbbrevNumber(Die->getAbbrev());

    // Get the abbreviation for this DIE.
    unsigned AbbrevNumber = Die->getAbbrevNumber();
    const DIEAbbrev* Abbrev = Abbreviations[AbbrevNumber - 1];

    // Set DIE offset
    Die->setOffset(Offset);

    // Start the size with the size of abbreviation code.
    Offset += getULEB128Size(AbbrevNumber);

    const SmallVectorImpl<DIEValue*>& Values = Die->getValues();
    const SmallVectorImpl<DIEAbbrevData>& AbbrevData = Abbrev->getData();

    // Size the DIE attribute values.
    for (unsigned i = 0, N = Values.size(); i < N; ++i)
    {
        // Size attribute value.
        Offset += Values[i]->SizeOf(Asm, AbbrevData[i].getForm());
    }

    // Size the DIE children if any.
    if (!Children.empty())
    {
        IGC_ASSERT_MESSAGE(Abbrev->getChildrenFlag() == dwarf::DW_CHILDREN_yes, "Children flag not set");

        for (unsigned j = 0, M = Children.size(); j < M; ++j)
        {
            Offset = computeSizeAndOffset(Children[j], Offset);
        }

        // End of children marker.
        Offset += sizeof(int8_t);
    }

    Die->setSize(Offset - Die->getOffset());
    return Offset;
}

// Compute the size and offset for each DIE.
void DwarfDebug::computeSizeAndOffsets()
{
    // Offset from the first CU in the debug info section is 0 initially.
    unsigned SecOffset = 0;

    // Iterate over each compile unit and set the size and offsets for each
    // DIE within each compile unit. All offsets are CU relative.
    for (SmallVectorImpl<CompileUnit*>::iterator I = CUs.begin(), E = CUs.end(); I != E; ++I)
    {
        (*I)->setDebugInfoOffset(SecOffset);

        // CU-relative offset is reset to 0 here.
        unsigned Offset = sizeof(int32_t) + // Length of Unit Info
            (*I)->getHeaderSize(); // Unit-specific headers

        // EndOffset here is CU-relative, after laying out
        // all of the CU DIE.
        unsigned EndOffset = computeSizeAndOffset((*I)->getCUDie(), Offset);
        SecOffset += EndOffset;
    }
}

// Switch to the specified MCSection and emit an assembler
// temporary label to it if SymbolStem is specified.
static MCSymbol* emitSectionSym(
    StreamEmitter* Asm, const MCSection* Section, const char* SymbolStem = 0)
{
    Asm->SwitchSection(Section);
    if (!SymbolStem) return 0;

    MCSymbol* TmpSym = Asm->GetTempSymbol(SymbolStem);
    Asm->EmitLabel(TmpSym);
    return TmpSym;
}

// Emit initial Dwarf sections with a label at the start of each one.
void DwarfDebug::emitSectionLabels()
{
    // Dwarf sections base addresses.
    DwarfInfoSectionSym = emitSectionSym(Asm, Asm->GetDwarfInfoSection(), "section_info");
    DwarfAbbrevSectionSym = emitSectionSym(Asm, Asm->GetDwarfAbbrevSection(), "section_abbrev");

    DwarfFrameSectionSym = emitSectionSym(Asm, Asm->GetDwarfFrameSection(), "dwarf_frame");

    if (const MCSection * MacroInfo = Asm->GetDwarfMacroInfoSection())
    {
        emitSectionSym(Asm, MacroInfo);
    }

    DwarfLineSectionSym = emitSectionSym(Asm, Asm->GetDwarfLineSection(), "section_line");
    emitSectionSym(Asm, Asm->GetDwarfLocSection());

    DwarfStrSectionSym = emitSectionSym(Asm, Asm->GetDwarfStrSection(), "info_string");

    DwarfDebugRangeSectionSym = emitSectionSym(Asm, Asm->GetDwarfRangesSection(), "debug_range");

    DwarfDebugLocSectionSym = emitSectionSym(Asm, Asm->GetDwarfLocSection(), "section_debug_loc");

    emitSectionSym(Asm, Asm->GetDataSection());

    TextSectionSym = emitSectionSym(Asm, Asm->GetTextSection(), "text_begin");
}

// Emit visible names into a debug str section.
void DwarfDebug::emitDebugStr()
{
    const MCSection* StrSection = Asm->GetDwarfStrSection();
    if (StringPool.empty()) return;

    // Start the dwarf str section.
    Asm->SwitchSection(StrSection);

    // Get all of the string pool entries and put them in an array by their ID so
    // we can sort them.
    SmallVector<std::pair<unsigned,
        StringMapEntry<std::pair<MCSymbol*, unsigned> >*>, 64> Entries;

    for (StringMap<std::pair<MCSymbol*, unsigned> >::iterator
        I = StringPool.begin(), E = StringPool.end();
        I != E; ++I)
    {
        Entries.push_back(std::make_pair(I->second.second, &*I));
    }

    array_pod_sort(Entries.begin(), Entries.end());

    for (unsigned i = 0, e = Entries.size(); i != e; ++i)
    {
        // Emit a label for reference from debug information entries.
        Asm->EmitLabel(Entries[i].second->getValue().first);

        // Emit the string itself with a terminating null byte.
        Asm->EmitBytes(StringRef(Entries[i].second->getKeyData(), Entries[i].second->getKeyLength() + 1));
    }
}

// Recursively emits a debug information entry.
void DwarfDebug::emitDIE(DIE* Die)
{
    // Get the abbreviation for this DIE.
    unsigned AbbrevNumber = Die->getAbbrevNumber();
    const DIEAbbrev* Abbrev = Abbreviations[AbbrevNumber - 1];

    // Emit the code (index) for the abbreviation.
    Asm->EmitULEB128(AbbrevNumber);

    const SmallVectorImpl<DIEValue*>& Values = Die->getValues();
    const SmallVectorImpl<DIEAbbrevData>& AbbrevData = Abbrev->getData();

    // Emit the DIE attribute values.
    for (unsigned i = 0, N = Values.size(); i < N; ++i)
    {
        dwarf::Attribute Attr = AbbrevData[i].getAttribute();
        dwarf::Form Form = AbbrevData[i].getForm();
        IGC_ASSERT_MESSAGE(Form, "Too many attributes for DIE (check abbreviation)");

        switch (Attr)
        {
        case dwarf::DW_AT_abstract_origin:
        case dwarf::DW_AT_type:
        case dwarf::DW_AT_friend:
        case dwarf::DW_AT_specification:
        case dwarf::DW_AT_import:
        case dwarf::DW_AT_containing_type:
        {
            DIE* Origin = cast<DIEEntry>(Values[i])->getEntry();
            unsigned Addr = Origin->getOffset();
            if (Form == dwarf::DW_FORM_ref_addr)
            {
                // For DW_FORM_ref_addr, output the offset from beginning of debug info
                // section. Origin->getOffset() returns the offset from start of the
                // compile unit.
                CompileUnit* CU = CUDieMap.lookup(Origin->getCompileUnit());
                IGC_ASSERT_MESSAGE(CU, "CUDie should belong to a CU.");
                Addr += CU->getDebugInfoOffset();
                Asm->EmitLabelPlusOffset(DwarfInfoSectionSym, Addr,
                    DIEEntry::getRefAddrSize(Asm, getDwarfVersion()));
            }
            else
            {
                // Make sure Origin belong to the same CU.
                IGC_ASSERT_MESSAGE(Die->getCompileUnit() == Origin->getCompileUnit(), "The referenced DIE should belong to the same CU in ref4");
                Asm->EmitInt32(Addr);
            }
            break;
        }
        case dwarf::DW_AT_ranges:
            // DW_AT_range encodes offset in debug_range section.
        case dwarf::DW_AT_location:
            // DW_AT_location encodes offset in debug_loc section
        case dwarf::DW_AT_accessibility:
            Values[i]->EmitValue(Asm, Form);
            break;

        default:
            // Emit an attribute using the defined form.
            Values[i]->EmitValue(Asm, Form);
            break;
        }
    }

    // Emit the DIE children if any.
    if (Abbrev->getChildrenFlag() == dwarf::DW_CHILDREN_yes)
    {
        const std::vector<DIE*>& Children = Die->getChildren();

        for (unsigned j = 0, M = Children.size(); j < M; ++j)
        {
            emitDIE(Children[j]);
        }

        Asm->EmitInt8(0);
    }
}

// Emit the debug info section.
void DwarfDebug::emitDebugInfo()
{
    const MCSection* USection = Asm->GetDwarfInfoSection();
    const MCSection* ASection = Asm->GetDwarfAbbrevSection();
    const MCSymbol* ASectionSym = DwarfAbbrevSectionSym;

    Asm->SwitchSection(USection);
    for (SmallVectorImpl<CompileUnit*>::iterator I = CUs.begin(),
        E = CUs.end(); I != E; ++I)
    {
        CompileUnit* TheCU = *I;
        DIE* Die = TheCU->getCUDie();

        // Emit the compile units header.
        Asm->EmitLabel(Asm->GetTempSymbol(/*USection->getLabelBeginName()*/".debug_info_begin", TheCU->getUniqueID()));

        // Emit size of content not including length itself
        // Emit ("Length of Unit");
        Asm->EmitInt32(TheCU->getHeaderSize() + Die->getSize());

        TheCU->emitHeader(ASection, ASectionSym);

        emitDIE(Die);
        Asm->EmitLabel(Asm->GetTempSymbol(/*USection->getLabelEndName()*/".debug_info_end", TheCU->getUniqueID()));
    }
}

// Emit the abbreviation section.
void DwarfDebug::emitAbbreviations()
{
    const MCSection* Section = Asm->GetDwarfAbbrevSection();

    // Check to see if it is worth the effort.
    if (!Abbreviations.empty())
    {
        // Start the debug abbrev section.
        Asm->SwitchSection(Section);

        MCSymbol* Begin = Asm->GetTempSymbol(/*Section->getLabelBeginName()*/".debug_abbrev_begin");
        Asm->EmitLabel(Begin);

        // For each abbrevation.
        for (unsigned i = 0, N = Abbreviations.size(); i < N; ++i)
        {
            // Get abbreviation data
            const DIEAbbrev* Abbrev = Abbreviations.at(i);

            // Emit the abbrevations code (base 1 index.)
            Asm->EmitULEB128(Abbrev->getNumber(), "Abbreviation Code");

            // Emit the abbreviations data.
            Abbrev->Emit(Asm);
        }

        // Mark end of abbreviations.
        Asm->EmitULEB128(0, "EOM(3)");

        MCSymbol* End = Asm->GetTempSymbol(/*Section->getLabelEndName()*/".debug_abbrev_end");
        Asm->EmitLabel(End);
    }
}

// Emit locations into the debug loc section.
void DwarfDebug::emitDebugLoc()
{
    if (DotDebugLocEntries.empty())
        return;

#if 1
    Asm->SwitchSection(Asm->GetDwarfLocSection());
    unsigned int size = Asm->GetPointerSize();

    for (SmallVectorImpl<DotDebugLocEntry>::iterator
        I = DotDebugLocEntries.begin(), E = DotDebugLocEntries.end();
        I != E; ++I)
    {
        DotDebugLocEntry& Entry = *I;
        if (Entry.isEmpty())
        {
            Asm->EmitIntValue(0, size);
            Asm->EmitIntValue(0, size);
        }
        else
        {
            auto Symbol = Entry.getSymbol();
            if(Symbol)
                Asm->EmitLabel(Symbol);
            for (unsigned int byte = 0; byte != Entry.loc.size(); byte++)
            {
                Asm->EmitIntValue(Entry.loc[byte], 1);
            }
        }
    }

    DotDebugLocEntries.clear();

#else
    for (SmallVectorImpl<DotDebugLocEntry>::iterator
        I = DotDebugLocEntries.begin(), E = DotDebugLocEntries.end();
        I != E; ++I)
    {
        DotDebugLocEntry& Entry = *I;
        if (I + 1 != DotDebugLocEntries.end())
            Entry.Merge(I + 1);
    }

    // Start the dwarf loc section.
    Asm->SwitchSection(Asm->GetDwarfLocSection());
    unsigned int size = Asm->GetPointerSize();
    Asm->EmitLabel(Asm->GetTempSymbol("debug_loc", 0));
    unsigned index = 1;
    for (SmallVectorImpl<DotDebugLocEntry>::iterator
        I = DotDebugLocEntries.begin(), E = DotDebugLocEntries.end();
        I != E; ++I, ++index)
    {
        DotDebugLocEntry& Entry = *I;
        if (Entry.isMerged()) continue;
        if (Entry.isEmpty())
        {
            Asm->EmitIntValue(0, size);
            Asm->EmitIntValue(0, size);
            Asm->EmitLabel(Asm->GetTempSymbol("debug_loc", index));
        }
        else
        {
            Asm->EmitSymbolValue(Entry.getBeginSym(), size);
            Asm->EmitSymbolValue(Entry.getEndSym(), size);
            //const DIVariable* DV = cast<DIVariable>(Entry.getVariable());
            // Emit ("Loc expr size");
            MCSymbol* begin = Asm->CreateTempSymbol();
            MCSymbol* end = Asm->CreateTempSymbol();
            Asm->EmitLabelDifference(end, begin, 2);
            Asm->EmitLabel(begin);

            const Instruction* pDbgInst = Entry.getDbgInst();
            VISAVariableLocation Loc = m_pModule->GetVariableLocation(pDbgInst);

            // Variable can be immdeiate or in a location (but not both)
            if (Loc.IsImmediate())
            {
                const Constant* pConstVal = Loc.GetImmediate();
                VISAModule::DataVector rawData;
                m_pModule->GetConstantData(pConstVal, rawData);
                const unsigned char* pData8 = rawData.data();
                int NumBytes = rawData.size();
                Asm->EmitInt8(dwarf::DW_OP_implicit_value);
                Asm->EmitULEB128(rawData.size());
                bool LittleEndian = Asm->IsLittleEndian();

                // Output the constant to DWARF one byte at a time.
                for (int i = 0; i < NumBytes; i++)
                {
                    uint8_t c = (LittleEndian) ? pData8[i] : pData8[(NumBytes - 1 - i)];

                    Asm->EmitInt8(c);
                }
            }
            else
            {
                // Variable which is not immediate can have location or nothing.
                IGC_ASSERT_MESSAGE(!Loc.HasSurface(), "Variable with surface should not change location");

                if (Loc.HasLocation())
                {
                    IGC_ASSERT_MESSAGE(Loc.IsRegister(), "Changable location can be an offset! Handle this case");
                    // InstCombine optimization may produce case where In Memory variable changes location
                    // Thus, In Memory variable indecator is passed as indirect location flag.
                    Asm->EmitDwarfRegOp(Loc.GetRegister(), Loc.GetOffset(), Loc.IsInMemory());
                }
            }
            Asm->EmitLabel(end);
        }
    }
#endif
}

// Emit visible names into a debug ranges section.
void DwarfDebug::emitDebugRanges()
{
    // Start the dwarf ranges section.
    Asm->SwitchSection(Asm->GetDwarfRangesSection());
    unsigned char size = (unsigned char)Asm->GetPointerSize();

    for (auto& Entry : GenISADebugRangeSymbols)
    {
        auto Label = Entry.first;
        if (Label)
            Asm->EmitLabel(Label);
        for (auto Data : Entry.second)
        {
            Asm->EmitIntValue(Data, size);
        }
    }
}

// Emit visible names into a debug macinfo section.
void DwarfDebug::emitDebugMacInfo()
{
    if (const MCSection * pLineInfo = Asm->GetDwarfMacroInfoSection())
    {
        // Start the dwarf macinfo section.
        Asm->SwitchSection(pLineInfo);
    }
}

uint32_t DwarfDebug::writeSubroutineCIE()
{
    std::vector<uint8_t> data;
    auto numGRFs = GetVISAModule()->getNumGRFs();

    // Emit CIE
    auto ptrSize = Asm->GetPointerSize();
    // The size of the length field plus the value of length must be an integral multiple of the address size.
    uint8_t lenSize = ptrSize;
    if (ptrSize == 8)
        lenSize = 12;

    // Write CIE_id
    write(data, ptrSize == 4 ? (uint32_t)0xfffffffe : (uint64_t)0xfffffffffffffffe);

    // version - ubyte
    write(data, (uint8_t)4);

    // augmentation - UTF8 string
    write(data, (uint8_t)0);

    // address size - ubyte
    write(data, (uint8_t)ptrSize);

    // segment size - ubyte
    write(data, (uint8_t)0);

    // code alignment factor - uleb128
    write(data, (uint8_t)1);

    // data alignment factor - sleb128
    write(data, (uint8_t)1);

    // return address register - uleb128
    // set machine return register to one which is physically
    // absent. later CFA instructions map this to a valid GRF.
    writeULEB128(data, numGRFs);


    // initial instructions (array of ubyte)
    // DW_CFA_def_cfa -> fpreg+0
    write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa);
    writeULEB128(data, 0);
    writeULEB128(data, 0);

    while ((lenSize + data.size()) % ptrSize != 0)
        // Insert DW_CFA_nop
        write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

    // Emit length with marker 0xffffffff for 8-byte ptr
    // DWARF4 spec:
    //  in the 64-bit DWARF format, an initial length field is 96 bits in size, and has two parts:
    //  * The first 32-bits have the value 0xffffffff.
    //  * The following 64-bits contain the actual length represented as an unsigned 64-bit integer.
    //
    // In the 32-bit DWARF format, an initial length field (see Section 7.2.2) is an unsigned 32-bit integer
    //  (which must be less than 0xfffffff0)

    uint32_t bytesWritten = 0;
    if (ptrSize == 8)
    {
        Asm->EmitInt32(0xffffffff);
        bytesWritten = 4;
    }
    Asm->EmitIntValue(data.size(), ptrSize);
    bytesWritten += ptrSize;

    for (auto& byte : data)
        Asm->EmitInt8(byte);
    bytesWritten += data.size();

    return bytesWritten;
}

uint32_t DwarfDebug::writeStackcallCIE()
{
    std::vector<uint8_t> data, data1;
    auto numGRFs = GetVISAModule()->getNumGRFs();
    auto specialGRF = GetSpecialGRF();

    auto copyVec = [&data](std::vector<uint8_t>& other)
    {
        for (auto t : other)
            data.push_back(t);
    };

    auto writeUndefined = [](std::vector<uint8_t>& data, uint32_t srcReg)
    {
        write(data, (uint8_t)llvm::dwarf::DW_CFA_undefined);
        writeULEB128(data, srcReg);
    };

    auto writeSameValue = [](std::vector<uint8_t>& data, uint32_t srcReg)
    {
        write(data, (uint8_t)llvm::dwarf::DW_CFA_same_value);
        writeULEB128(data, srcReg);
    };

    // Emit CIE
    auto ptrSize = Asm->GetPointerSize();
    // The size of the length field plus the value of length must be an integral multiple of the address size.
    uint8_t lenSize = ptrSize;
    if (ptrSize == 8)
        lenSize = 12;

    // Write CIE_id
    write(data, ptrSize == 4 ? (uint32_t)0xffffffff : (uint64_t)0xffffffffffffffff);

    // version - ubyte
    write(data, (uint8_t)4);

    // augmentation - UTF8 string
    write(data, (uint8_t)0);

    // address size - ubyte
    write(data, (uint8_t)ptrSize);

    // segment size - ubyte
    write(data, (uint8_t)0);

    // code alignment factor - uleb128
    write(data, (uint8_t)1);

    // data alignment factor - sleb128
    write(data, (uint8_t)1);

    // return address register - uleb128
    // set machine return register to one which is physically
    // absent. later CFA instructions map this to a valid GRF.
    writeULEB128(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(numGRFs));

    // initial instructions (array of ubyte)
    // DW_OP_regx r125
    // DW_OP_bit_piece 32 96
    write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa_expression);

    data1.clear();
    write(data1, (uint8_t)llvm::dwarf::DW_OP_regx);
    auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF);
    writeULEB128(data1, DWRegEncoded);
    write(data1, (uint8_t)llvm::dwarf::DW_OP_bit_piece);
    writeULEB128(data1, 32);
    writeULEB128(data1, BEFPSubReg * 4 * 8);

    writeULEB128(data, data1.size());
    copyVec(data1);

    // emit same value for all callee save entries in frame
    unsigned int calleeSaveStart = (numGRFs - 8) / 2;

    // caller save - undefined rule
    for (unsigned int grf = 0; grf != calleeSaveStart; ++grf)
    {
        writeUndefined(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(grf));
    }

    // callee save - same value rule
    for (unsigned int grf = calleeSaveStart; grf != numGRFs; ++grf)
    {
        writeSameValue(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(grf));
    }

    // move return address register to actual location
    // DW_CFA_register     numGRFs      specialGRF
    write(data, (uint8_t)llvm::dwarf::DW_CFA_register);
    writeULEB128(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(numGRFs));
    writeULEB128(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));

    while ((lenSize + data.size()) % ptrSize != 0)
        // Insert DW_CFA_nop
        write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

    // Emit length with marker 0xffffffff for 8-byte ptr
    // DWARF4 spec:
    //  in the 64-bit DWARF format, an initial length field is 96 bits in size, and has two parts:
    //  * The first 32-bits have the value 0xffffffff.
    //  * The following 64-bits contain the actual length represented as an unsigned 64-bit integer.
    //
    // In the 32-bit DWARF format, an initial length field (see Section 7.2.2) is an unsigned 32-bit integer
    //  (which must be less than 0xfffffff0)

    uint32_t bytesWritten = 0;
    if (ptrSize == 8)
    {
        Asm->EmitInt32(0xffffffff);
        bytesWritten = 4;
    }
    Asm->EmitIntValue(data.size(), ptrSize);
    bytesWritten += ptrSize;

    for (auto& byte : data)
        Asm->EmitInt8(byte);
    bytesWritten += data.size();

    return bytesWritten;
}

void DwarfDebug::writeFDESubroutine(VISAModule* m)
{
    std::vector<uint8_t> data;

    auto firstInst = (m->GetInstInfoMap()->begin())->first;
    auto funcName = firstInst->getParent()->getParent()->getName();

    const IGC::DbgDecoder::SubroutineInfo* sub = nullptr;
    const auto* co = m->getCompileUnit(*decodedDbg);
    for (const auto& s : co->subs)
    {
        if (s.name.compare(funcName.str()) == 0)
        {
            sub = &s;
            break;
        }
    }

    if (!sub)
        return;

    auto numGRFs = GetVISAModule()->getNumGRFs();

    // Emit CIE
    auto ptrSize = Asm->GetPointerSize();
    uint8_t lenSize = 4;
    if (ptrSize == 8)
        lenSize = 12;

    // CIE_ptr (4/8 bytes)
    write(data, ptrSize == 4 ? (uint32_t)offsetCIESubroutine : (uint64_t)offsetCIESubroutine);

    // initial location
    auto getGenISAOffset = [co](unsigned int VISAIndex)
    {
        uint64_t genOffset = 0;

        for (auto& item : co->CISAIndexMap)
        {
            if (item.first >= VISAIndex)
            {
                genOffset = item.second;
                break;
            }
        }

        return genOffset;
    };
    uint64_t genOffStart = this->lowPc;
    uint64_t genOffEnd = this->highPc;
    auto& retvarLR = sub->retval;
    IGC_ASSERT_MESSAGE(retvarLR.size() > 0, "expecting GRF for return");
    IGC_ASSERT_MESSAGE(retvarLR[0].var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF, "expecting GRF for return");

    // assume ret var is live throughout sub-routine and it is contained
    // in same GRF.
    uint32_t linearAddr = (retvarLR.front().var.mapping.r.regNum * m_pModule->getGRFSize()) +
        retvarLR.front().var.mapping.r.subRegNum;

    // initial location
    write(data, ptrSize == 4 ? (uint32_t)genOffStart : genOffStart);

    // address range
    write(data, ptrSize == 4 ? (uint32_t)(genOffEnd - genOffStart) :
        (genOffEnd - genOffStart));

    // instruction - ubyte
    write(data, (uint8_t)llvm::dwarf::DW_CFA_register);

    // return reg operand
    writeULEB128(data, numGRFs);

    // actual reg holding retval
    writeULEB128(data, linearAddr);

    // initial instructions (array of ubyte)
    while ((lenSize + data.size()) % ptrSize != 0)
        // Insert DW_CFA_nop
        write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

    // Emit length with marker 0xffffffff for 8-byte ptr
    if (ptrSize == 8)
        Asm->EmitInt32(0xffffffff);
    Asm->EmitIntValue(data.size(), ptrSize);

    for (auto& byte : data)
        Asm->EmitInt8(byte);
}

void DwarfDebug::writeFDEStackCall(VISAModule* m)
{
    std::vector<uint8_t> data;
    uint64_t loc = 0;
    uint64_t LabelOffset = std::numeric_limits<uint64_t>::max();
    // <ip, <instructions to write> >
    auto sortAsc = [](uint64_t a, uint64_t b) { return a < b; };
    std::map <uint64_t, std::vector<uint8_t>, decltype(sortAsc)> cfaOps(sortAsc);
    const auto& dbgInfo = *m->getCompileUnit(*decodedDbg);
    auto numGRFs = GetVISAModule()->getNumGRFs();
    auto specialGRF = GetSpecialGRF();

    auto advanceLoc = [&loc](std::vector<uint8_t>& data, uint64_t newLoc)
    {
        uint64_t diff = newLoc - loc;
        if (diff == 0)
            return;

        if (diff < (1<<(8*sizeof(uint8_t)-1)))
        {
            write(data, (uint8_t)llvm::dwarf::DW_CFA_advance_loc1);
            write(data, (uint8_t)diff);
        }
        else if (diff < (1 << (8*sizeof(uint16_t)-1)))
        {
            write(data, (uint8_t)llvm::dwarf::DW_CFA_advance_loc2);
            write(data, (uint16_t)diff);
        }
        else
        {
            write(data, (uint8_t)llvm::dwarf::DW_CFA_advance_loc4);
            write(data, (uint32_t)diff);
        }
        loc = newLoc;
    };

    // offset to read off be_fp
    // deref - decide whether or not to emit DW_OP_deref
    // normalizeResult - true when reading a value from scratch space that is a scratch space address
    auto writeOffBEFP = [specialGRF, this](std::vector<uint8_t>& data, uint32_t offset, bool deref, bool normalizeResult)
    {
        // DW_OP_const1u 12
        // DW_OP_regx 125
        // DW_OP_const2u 96
        // DW_OP_const1u 32
        // DW_OP_INTEL_push_bit_piece_stack
        // DW_OP_const1u 16
        // DW_OP_mul
        // DW_OP_constu <memory offset>
        // DW_OP_plus
        // DW_OP_constu 0x900000000000000
        // DW_OP_or
        // DW_OP_deref
        std::vector<uint8_t> data1;

        auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF);
        write(data1, (uint8_t)llvm::dwarf::DW_OP_regx);
        writeULEB128(data1, DWRegEncoded);
        write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
        write(data1, (uint16_t)(BEFPSubReg * 4 * 8));
        write(data1, (uint8_t)llvm::dwarf::DW_OP_const1u);
        write(data1, (uint8_t)32);
        write(data1, (uint8_t)DW_OP_INTEL_push_bit_piece_stack);

        if (EmitSettings.ScratchOffsetInOW)
        {
            // when scratch offset is in OW, be_fp has to be multiplied by 16
            // to normalize and generate byte offset for complete address
            // computation.
            write(data1, (uint8_t)llvm::dwarf::DW_OP_const1u);
            write(data1, (uint8_t)16);
            write(data1, (uint8_t)llvm::dwarf::DW_OP_mul);
        }

        write(data1, (uint8_t)llvm::dwarf::DW_OP_constu);
        writeULEB128(data1, offset);
        write(data1, (uint8_t)llvm::dwarf::DW_OP_plus);

        // indicate that the resulting address is on BE stack
        if (!EmitSettings.EnableGTLocationDebugging)
        {
            Address addr;
            addr.Set(Address::Space::eScratch, 0, 0);

            write(data1, (uint8_t)llvm::dwarf::DW_OP_const8u);
            write(data1, (uint64_t)addr.GetAddress());

            write(data1, (uint8_t)llvm::dwarf::DW_OP_or);
        }
        else
        {
            uint32_t scratchBaseAddrEncoded =
                GetEncodedRegNum<RegisterNumbering::ScratchBase>(dwarf::DW_OP_breg0);

            write(data1, (uint8_t)scratchBaseAddrEncoded);
            writeULEB128(data1, 0);
        }

        if (deref)
            write(data1, (uint8_t)llvm::dwarf::DW_OP_deref);

        if (EmitSettings.ScratchOffsetInOW &&
            normalizeResult)
        {
            // since data stored in scratch space is also in oword units, normalize it
            write(data1, (uint8_t)llvm::dwarf::DW_OP_const1u);
            write(data1, (uint8_t)16);
            write(data1, (uint8_t)llvm::dwarf::DW_OP_mul);
        }

        writeULEB128(data, data1.size());
        for (auto item : data1)
            write(data, (uint8_t)item);
    };

    auto writeLR = [this, writeOffBEFP](std::vector<uint8_t>& data, const DbgDecoder::LiveIntervalGenISA& lr, bool deref,
        bool normalizeResult)
    {
        if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory)
        {
            writeOffBEFP(data, (uint32_t)lr.var.mapping.m.memoryOffset, deref, normalizeResult);

            IGC_ASSERT_MESSAGE(!lr.var.mapping.m.isBaseOffBEFP, "Expecting location offset from BE_FP");
        }
        else if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF)
        {
            IGC_ASSERT_MESSAGE(false, "Not expecting CFA to be in non-GRF location");
        }
    };

    auto writeSameValue = [](std::vector<uint8_t>& data, uint32_t srcReg)
    {
        write(data, (uint8_t)llvm::dwarf::DW_CFA_same_value);
        writeULEB128(data, srcReg);
    };

    auto ptrSize = Asm->GetPointerSize();
    auto& cfi = dbgInfo.cfi;
    // Emit CIE
    uint8_t lenSize = 4;
    if (ptrSize == 8)
        lenSize = 12;

    // CIE_ptr (4/8 bytes)
    write(data, ptrSize == 4 ? (uint32_t)offsetCIEStackCall : (uint64_t)offsetCIEStackCall);

    // initial location
    auto genOffStart = dbgInfo.relocOffset;
    auto genOffEnd = highPc;

    // LabelOffset holds offset where start %ip is written to buffer.
    // Code later uses this to insert label for relocation.
    LabelOffset = data.size();
    if (EmitSettings.EnableRelocation)
    {
        write(data, ptrSize == 4 ? (uint32_t)0xfefefefe : (uint64_t)0xfefefefefefefefe);
    }
    else
    {
        write(data, ptrSize == 4 ? (uint32_t)genOffStart : (uint64_t)genOffStart);
    }

    // address range
    write(data, ptrSize == 4 ? (uint32_t)(genOffEnd - genOffStart) :
        (uint64_t)(genOffEnd - genOffStart));

    const unsigned int MovGenInstSizeInBytes = 16;

    // write CFA
    if (cfi.callerbefpValid)
    {
        const auto& callerFP = cfi.callerbefp;
        for (const auto& item : callerFP)
        {
            // map out CFA to an offset on be stack
            write(cfaOps[item.start], (uint8_t)llvm::dwarf::DW_CFA_def_cfa_expression);
            writeLR(cfaOps[item.start], item, true, true);
        }

        // describe r125 is at [r125.3]:ud
        auto ip = callerFP.front().start;
        write(cfaOps[ip], (uint8_t)llvm::dwarf::DW_CFA_expression);
        writeULEB128(cfaOps[ip], GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));
        writeOffBEFP(cfaOps[ip], 0, false, false);
        writeSameValue(cfaOps[callerFP.back().end + MovGenInstSizeInBytes],
                       GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));
    }

    // write return addr on stack
    if (cfi.retAddrValid)
    {
        auto& retAddr = cfi.retAddr;
        for (auto& item : retAddr)
        {
            // start live-range
            write(cfaOps[item.start], (uint8_t)llvm::dwarf::DW_CFA_expression);
            writeULEB128(cfaOps[item.start], GetEncodedRegNum<RegisterNumbering::GRFBase>(numGRFs));
            writeLR(cfaOps[item.start], item, false, false);

            // end live-range
            // VISA emits following:
            // 624: ...
            // 640: (W) mov (4|M0) r125.0<1>:ud  r59.4<4;4,1>:ud <-- restore ret %ip, and caller
            // 656: ret (8|M0) r125.0:ud

            // VISA dbg:
            // Return addr saved at:
            // Live intervals :
            // (64, 640) @ Spilled(offset = 0 bytes) (off be_fp)

            // As per VISA debug info, return %ip restore instruction is at offset 640 above.
            // But when we stop at 640, we still want ret %ip to be read from frame descriptor
            // in memory as current frame is still value. Only from offset 656 should we read
            // ret %ip from r125 directly. This is achieved by taking offset 640 reported by
            // VISA debug info and adding 16 to it which is size of the mov instruction.
            write(cfaOps[item.end + MovGenInstSizeInBytes], (uint8_t)llvm::dwarf::DW_CFA_register);
            writeULEB128(cfaOps[item.end + MovGenInstSizeInBytes],
                         GetEncodedRegNum<RegisterNumbering::GRFBase>(numGRFs));
            writeULEB128(cfaOps[item.end + MovGenInstSizeInBytes],
                         GetEncodedRegNum<RegisterNumbering::GRFBase>(GetSpecialGRF()));
        }
    }
    else
    {
        if (m->GetType() == VISAModule::ObjectType::KERNEL)
        {
            // set return location to be undefined in top frame
            write(cfaOps[0], (uint8_t)llvm::dwarf::DW_CFA_undefined);
            writeULEB128(cfaOps[0], GetEncodedRegNum<RegisterNumbering::GRFBase>(numGRFs));
        }
    }

    // write callee save
    if (cfi.calleeSaveEntry.size() > 0)
    {
        // set holds any callee save GRF that has been saved already to stack.
        // this is required because of some differences between dbginfo structure
        // reporting callee save and dwarf's debug_frame section requirements.
        std::unordered_set<uint32_t> calleeSaveRegsSaved;
        for (auto& item : cfi.calleeSaveEntry)
        {
            for (unsigned int idx = 0; idx != item.data.size(); ++idx)
            {
                auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->getGRFSize());
                if (calleeSaveRegsSaved.find(regNum) == calleeSaveRegsSaved.end())
                {
                    write(cfaOps[item.genIPOffset], (uint8_t)llvm::dwarf::DW_CFA_expression);
                    writeULEB128(cfaOps[item.genIPOffset], GetEncodedRegNum<RegisterNumbering::GRFBase>(regNum));
                    writeOffBEFP(cfaOps[item.genIPOffset], item.data[idx].dst.m.memoryOffset, false, false);
                    calleeSaveRegsSaved.insert(regNum);
                }
                else
                {
                    // already saved, so no need to emit same save again
                }
            }

            // check whether an entry is present in calleeSaveRegsSaved but not in cfi.calleeSaveEntry
            // missing entries are available in original locations
            for (auto it = calleeSaveRegsSaved.begin(); it != calleeSaveRegsSaved.end();)
            {
                bool found = false;
                for (unsigned int idx = 0; idx != item.data.size(); ++idx)
                {
                    auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->getGRFSize());
                    if ((*it) == regNum)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    writeSameValue(cfaOps[item.genIPOffset],
                                   GetEncodedRegNum<RegisterNumbering::GRFBase>((*it)));
                    it = calleeSaveRegsSaved.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }

    // write to actual buffer
    advanceLoc(data, loc);
    for (auto& item : cfaOps)
    {
        advanceLoc(data, item.first);
        for (auto& c : item.second)
        {
            data.push_back(c);
        }
    }

    // initial instructions (array of ubyte)
    while ((lenSize + data.size()) % ptrSize != 0)
        // Insert DW_CFA_nop
        write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

    // Emit length with marker 0xffffffff for 8-byte ptr
    if (ptrSize == 8)
        Asm->EmitInt32(0xffffffff);
    Asm->EmitIntValue(data.size(), ptrSize);

    if (EmitSettings.EnableRelocation)
    {
        uint32_t ByteOffset = 0;

        for (auto it = data.begin(); it != data.end(); ++it)
        {
            auto byte = *it;
            if (ByteOffset++ == (uint32_t)LabelOffset)
            {
                auto Label = GetLabelBeforeIp(genOffStart);
                Asm->EmitLabelReference(Label, ptrSize, false);
                // Now skip ptrSize number of bytes from data
                std::advance(it, ptrSize);
                byte = *it;
            }
            Asm->EmitInt8(byte);
        }
    }
    else
    {
        for (auto& byte : data)
            Asm->EmitInt8(byte);
    }
}
bool DwarfDebug::DwarfFrameSectionNeeded() const
{
    return (m_pModule->hasOrIsStackCall(*decodedDbg) || (m_pModule->getSubroutines(*decodedDbg)->size() > 0));
}

llvm::MCSymbol* DwarfDebug::GetLabelBeforeIp(unsigned int ip)
{
    auto it = LabelsBeforeIp.find(ip);
    if (it != LabelsBeforeIp.end())
        return (*it).second;
    auto NewLabel = Asm->CreateTempSymbol();
    LabelsBeforeIp[ip] = NewLabel;
    return NewLabel;
}

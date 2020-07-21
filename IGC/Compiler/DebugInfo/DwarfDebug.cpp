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
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfDebug.cpp
///////////////////////////////////////////////////////////////////////////////

#include "llvm/Config/llvm-config.h"
#define DEBUG_TYPE "dwarfdebug"
#include "Compiler/DebugInfo/DwarfDebug.hpp"
#include "Compiler/DebugInfo/DIE.hpp"
#include "Compiler/DebugInfo/DwarfCompileUnit.hpp"
#include "Compiler/DebugInfo/StreamEmitter.hpp"
#include "Compiler/DebugInfo/VISAModule.hpp"
#include "Compiler/DebugInfo/Version.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Support/Debug.h"
#include <llvmWrapper/IR/Function.h>
#include "llvmWrapper/BinaryFormat/Dwarf.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
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
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/messageEncoding.hpp"

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
    return Var->getType()
#if LLVM_VERSION_MAJOR <= 8
        .resolve()
#endif
        ->isBlockByrefStruct();
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

    DIType* BaseType = resolve(Ty->getBaseType());

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

    DIType* BaseType = resolve(Ty->getBaseType());

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
    //DIType* Ty = Var->getType()
//#if LLVM_VERSION_MAJOR <= 8
    //    .resolve()
//#endif
    //    ;
// #if LLVM_VERSION_MAJOR < 10
    // isBlockByrefStruct is no more support by LLVM10 IR - more info in this commit below:
    // https://github.com/llvm/llvm-project/commit/0779dffbd4a927d7bf9523482481248c51796907
    // FIXME: isBlockByrefVariable should be reformulated in terms of complex
    // addresses instead.
    //if (Ty->isBlockByrefStruct()) {
        /* Byref variables, in Blocks, are declared by the programmer as
        "SomeType VarName;", but the compiler creates a
        __Block_byref_x_VarName struct, and gives the variable VarName
        either the struct, or a pointer to the struct, as its type.  This
        is necessary for various behind-the-scenes things the compiler
        needs to do with by-reference variables in blocks.

        However, as far as the original *programmer* is concerned, the
        variable should still have type 'SomeType', as originally declared.

        The following function dives into the __Block_byref_x_VarName
        struct to find the original type of the variable.  This will be
        passed back to the code generating the type for the Debug
        Information Entry for the variable 'VarName'.  'VarName' will then
        have the original type 'SomeType' in its debug information.

        The original type 'SomeType' will be the type of the field named
        'VarName' inside the __Block_byref_x_VarName struct.

        NOTE: In order for this to not completely fail on the debugger
        side, the Debug Information Entry for the variable VarName needs to
        have a DW_AT_location that tells the debugger how to unwind through
        the pointers and __Block_byref_x_VarName struct to find the actual
        value of the variable.  The function addBlockByrefType does this.  */
        //DIType* subType = Ty;
        //uint16_t tag = (uint16_t)Ty->getTag();

        //if (tag == dwarf::DW_TAG_pointer_type)
        //    subType = resolve(cast<DIDerivedType>(Ty)->getBaseType());

        //auto Elements = cast<DICompositeType>(subType)->getElements();
        //for (unsigned i = 0, N = Elements.size(); i < N; ++i) {
        //    auto* DT = cast<DIDerivedType>(Elements[i]);
        //    if (getName() == DT->getName())
        //        return resolve(DT->getBaseType());
        //}
    //}
//#endif
    //return Ty;
    return resolve(getVariable()->getType());
}

/// Return Dwarf Version by checking module flags.
static unsigned getDwarfVersionFromModule(const Module* M)
{
    auto* Val = cast_or_null<ConstantAsMetadata>(M->getModuleFlag("Dwarf Version"));
    if (!Val)
        return dwarf::DWARF_VERSION;
    return (unsigned)(cast<ConstantInt>(Val->getValue())->getZExtValue());
}

DwarfDebug::DwarfDebug(StreamEmitter* A, VISAModule* M) :
    Asm(A), m_pModule(M), FirstCU(0),
    //AbbreviationsSet(InitAbbreviationsSetSize),
    SourceIdMap(DIEValueAllocator),
    PrevLabel(NULL), GlobalCUIndexCount(0),
    StringPool(DIEValueAllocator),
    NextStringPoolNumber(0), StringPref("info_string")
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

    gatherDISubprogramNodes();
    M->SetDwarfDebug(this);
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

#if 1
    if (m_pModule->isDirectElfInput)
    {
        SPCU->addUInt(SPDie, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, lowPc);
        SPCU->addUInt(SPDie, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, highPc);


    }
    else
    {
        SPCU->addLabelAddress(SPDie, dwarf::DW_AT_low_pc,
            Asm->GetTempSymbol("func_begin", m_pModule->GetFunctionNumber(SP->getName().data())));
        SPCU->addLabelAddress(SPDie, dwarf::DW_AT_high_pc,
            Asm->GetTempSymbol("func_end", m_pModule->GetFunctionNumber(SP->getName().data())));

        if (IGC_IS_FLAG_ENABLED(EnableSIMDLaneDebugging))
        {
            // Emit SIMD width
            SPCU->addUInt(SPDie, (dwarf::Attribute)DW_AT_INTEL_simd_width, dwarf::DW_FORM_data2,
                m_pModule->GetSIMDSize());
        }
    }
#else
    // Following existed before supporting stack calls. GetFunctionNumber() was hard
    // coded to return 0 always so for single function this worked. With stackcall
    // functions though we need this to change. Primarily because SP.getFunction()
    // is nullptr and there is no way to get Function* from SP directly.
    SPCU->addLabelAddress(SPDie, dwarf::DW_AT_low_pc,
        Asm->GetTempSymbol("func_begin", m_pModule->GetFunctionNumber(SP.getFunction())));
    SPCU->addLabelAddress(SPDie, dwarf::DW_AT_high_pc,
        Asm->GetTempSymbol("func_end", m_pModule->GetFunctionNumber(SP.getFunction())));
#endif

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

    if (m_pModule->isDirectElfInput)
        return false;

    // We don't create a DIE if we have a single Range and the end label
    // is null.
    SmallVectorImpl<InsnRange>::const_iterator RI = Ranges.begin();
    MCSymbol* End = getLabelAfterInsn(RI->second);
    return !End;
}

// Construct new DW_TAG_lexical_block for this scope and attach
// DW_AT_low_pc/DW_AT_high_pc labels as well as DW_AT_INTEL_simd_width
DIE* DwarfDebug::constructLexicalScopeDIE(CompileUnit* TheCU, LexicalScope* Scope)
{
    if (isLexicalScopeDIENull(Scope))
        return 0;

    DIE* ScopeDIE = new DIE(dwarf::DW_TAG_lexical_block);
    if (Scope->isAbstractScope())
        return ScopeDIE;

    const SmallVectorImpl<InsnRange>& Ranges = Scope->getRanges();
    IGC_ASSERT_MESSAGE(Ranges.empty() == false, "LexicalScope does not have instruction markers!");

    if (m_pModule->isDirectElfInput)
    {
        if (IGC_IS_FLAG_ENABLED(EmitDebugRanges))
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
                TheCU->addUInt(ScopeDIE, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, GenISARanges.front().first);
                TheCU->addUInt(ScopeDIE, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, GenISARanges.back().second);
            }
        }
        return ScopeDIE;
    }

    // CQ#: 21731
    // Emit lowpc/highpc only
    // If several ranges were discovered from lexical info, then emit lowpc from
    // first subrange and highpc from last one
    auto start = Ranges.front().first;
    auto end = Ranges.back().second;
    DebugRangeSymbols.push_back(getLabelBeforeInsn(start));
    DebugRangeSymbols.push_back(getLabelAfterInsn(end));

    MCSymbol* StartLabel = getLabelBeforeInsn(start);
    MCSymbol* EndLabel = getLabelAfterInsn(end);
    TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_low_pc, StartLabel);
    TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_high_pc, EndLabel);

    DebugRangeSymbols.push_back(NULL);
    DebugRangeSymbols.push_back(NULL);
    return ScopeDIE;
}

void DwarfDebug::encodeRange(CompileUnit* TheCU, DIE* ScopeDIE, const llvm::SmallVectorImpl<InsnRange>* Ranges)
{
    // This makes sense only for full debug info.
    // Resolve VISA index to Gen IP here.
    if (Ranges->size() == 0)
        return;

    bool needDebugRange = true;
    if (Ranges->size() == 1)
    {
        auto start = Ranges->front().first;
        auto end = Ranges->back().second;
        InsnRange RI(start, end);
        auto GenISARanges = m_pModule->getGenISARange(RI);

        if (GenISARanges.size() == 1)
        {
            // Emit loc/high_pc
            TheCU->addUInt(ScopeDIE, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, GenISARanges.front().first);
            TheCU->addUInt(ScopeDIE, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, GenISARanges.back().second);

            needDebugRange = false;
        }
    }

    if (needDebugRange || Ranges->size() > 1)
    {
        bool rangeAdded = false;
        for (SmallVectorImpl<InsnRange>::const_iterator RI = Ranges->begin(),
            RE = Ranges->end(); RI != RE; ++RI)
        {
            auto&& GenISARanges = m_pModule->getGenISARange(*RI);
            for (auto& item : GenISARanges)
            {
                if (!rangeAdded)
                {
                    TheCU->addUInt(ScopeDIE, dwarf::DW_AT_ranges, dwarf::DW_FORM_sec_offset,
                        GenISADebugRangeSymbols.size() * Asm->GetPointerSize());
                    rangeAdded = true;
                }
                GenISADebugRangeSymbols.push_back(item.first);
                GenISADebugRangeSymbols.push_back(item.second);
            }
        }
        if (rangeAdded)
        {
            // Terminate the range list.
            GenISADebugRangeSymbols.push_back(0);
            GenISADebugRangeSymbols.push_back(0);
        }
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

    if (!m_pModule->isDirectElfInput)
    {
        // Emit lowpc/highpc only
        // If several ranges were discovered from lexical info, then emit lowpc from
        // first subrange and highpc from last one
        auto start = Ranges.front().first;
        auto end = Ranges.back().second;
        DebugRangeSymbols.push_back(getLabelBeforeInsn(start));
        DebugRangeSymbols.push_back(getLabelAfterInsn(end));

        MCSymbol* StartLabel = getLabelBeforeInsn(start);
        MCSymbol* EndLabel = getLabelAfterInsn(end);
        TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_low_pc, StartLabel);
        TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_high_pc, EndLabel);

        DebugRangeSymbols.push_back(NULL);
        DebugRangeSymbols.push_back(NULL);
        return ScopeDIE;
    }
    else
    {
        // .debug_range section has not been laid out yet. Emit offset in
        // .debug_range as a uint, size 4, for now. emitDIE will handle
        // DW_AT_ranges appropriately.
        if (IGC_IS_FLAG_ENABLED(EmitDebugRanges))
        {
            encodeRange(TheCU, ScopeDIE, &Ranges);
        }
        else
        {
            TheCU->addUInt(ScopeDIE, dwarf::DW_AT_ranges, dwarf::DW_FORM_sec_offset,
                GenISADebugRangeSymbols.size() * Asm->GetPointerSize());
            for (SmallVectorImpl<InsnRange>::const_iterator RI = Ranges.begin(),
                RE = Ranges.end(); RI != RE; ++RI)
            {
                auto&& GenISARanges = m_pModule->getGenISARange(*RI);
                for (auto& item : GenISARanges)
                {
                    GenISADebugRangeSymbols.push_back(item.first);
                    GenISADebugRangeSymbols.push_back(item.second);
                }
            }
            // Terminate the range list.
            GenISADebugRangeSymbols.push_back(0);
            GenISADebugRangeSymbols.push_back(0);
        }

        return ScopeDIE;
    }
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

    if (m_pModule->isDirectElfInput)
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
    else
    {
        NewCU->addLabelAddress(Die, dwarf::DW_AT_low_pc, ModuleBeginSym);
        NewCU->addLabelAddress(Die, dwarf::DW_AT_high_pc, ModuleEndSym);

        // Define start line table label for each Compile Unit.
        MCSymbol* LineTableStartSym = Asm->GetTempSymbol("line_table_start", NewCU->getUniqueID());
        Asm->SetMCLineTableSymbol(LineTableStartSym, NewCU->getUniqueID());

        // Use a single line table if we are using .loc and generating assembly.
        bool UseTheFirstCU = (NewCU->getUniqueID() == 0);
        NewCU->addLabel(Die, dwarf::DW_AT_stmt_list, dwarf::DW_FORM_sec_offset,
            UseTheFirstCU ? Asm->GetTempSymbol("section_line") : LineTableStartSym);
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
                    str += op1->getValue()->getUniqueInteger().toString(10, false);
                    str += ".";
                    str += op2->getValue()->getUniqueInteger().toString(10, false);

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

// Emit all Dwarf sections that should come prior to the content. Create
// global DIEs and emit initial debug info sections.
void DwarfDebug::beginModule()
{
    const Module* M = m_pModule->GetModule();
    // If module has named metadata anchors then use them, otherwise scan the
    // module using debug info finder to collect debug info.
    NamedMDNode* CU_Nodes = M->getNamedMetadata("llvm.dbg.cu");
    if (!CU_Nodes) return;
    // Emit initial sections so we can reference labels later.
    emitSectionLabels();

    for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i)
    {
        DICompileUnit* CUNode = cast<DICompileUnit>(CU_Nodes->getOperand(i));
        CompileUnit* CU = constructCompileUnit(CUNode);

        //DISubprogramArray SPs = CUNode->getSubprograms();
        //for (unsigned i = 0, e = SPs.size(); i != e; ++i)
        //{
        //    constructSubprogramDIE(CU, SPs[i]);
        //}

        // Added during upgrade to LLVM 4.0
        // Assume all functions belong to same Compile Unit
        // With LLVM 4.0 DISubprogram nodes are no longer
        // present in DICompileUnit node.
        for (auto& DISP : DISubprogramNodes)
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
        writeCIE();
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

        for (auto& SP : DISubprogramNodes)
        {
            if (!SP)
                continue;

            if (ProcessedSPNodes.count(SP) != 0 ||
                !isa<DISubprogram>(SP) || !SP->isDefinition())
            {
                continue;
            }
#if LLVM_VERSION_MAJOR == 4
            DILocalVariableArray Variables = SP->getVariables();
#elif LLVM_VERSION_MAJOR >= 7
            auto Variables = SP->getRetainedNodes();
#endif
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
    Asm->EmitLabel(ModuleEndSym);

    // End any existing sections.
    // TODO: Does this need to happen?
    endSections();

    // Finalize the debug info for the module.
    finalizeModuleInfo();

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
        delete I->second;
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
        CurrentFnArguments.resize(IGCLLVM::GetFuncArgSize(MF));
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

    auto isAdded = [&addedEntries](MDNode* md, DILocation* iat)
    {
        for (auto item : addedEntries)
        {
            if (std::get<0>(item) == md && std::get<1>(item) == iat)
                return std::get<2>(item);
        }
        return (DbgVariable*)nullptr;
    };

    unsigned int offset = 0;
    unsigned int pointerSize = getPointerSize((llvm::Module&) * MF->getParent());
    unsigned char bufLEB128[64];
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

            if (!m_pModule->isDirectElfInput || IGC_IS_FLAG_DISABLED(EmitDebugLoc))
                continue;

            // assume that VISA preserves location thoughout its lifetime
            auto Loc = m_pModule->GetVariableLocation(pInst);

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

            if ((History.size() == 1 && GenISARange.size() == 1) ||
                GenISARange.size() == 0)
            {
                // Emit location within the DIE
                continue;
            }

            for (auto range : GenISARange)
            {
                DotDebugLocEntry dotLoc(range.first, range.second, pInst, DV);
                dotLoc.setOffset(offset);

                if (Loc.IsImmediate())
                {
                    const Constant* pConstVal = Loc.GetImmediate();
                    if (const ConstantInt * pConstInt = dyn_cast<ConstantInt>(pConstVal))
                    {
                        auto op = llvm::dwarf::DW_OP_implicit_value;
                        // Always emit an 8-byte value
                        const unsigned int lebSize = 8;
                        uint64_t rangeStart = range.first;
                        uint64_t rangeEnd = range.second;

                        // Emit location to debug_loc
                        if (RegVar->getDotDebugLocOffset() == ~0U)
                        {
                            RegVar->setDotDebugLocOffset(offset);
                        }

                        write(dotLoc.loc, (unsigned char*)& rangeStart, pointerSize);
                        write(dotLoc.loc, (unsigned char*)& rangeEnd, pointerSize);
                        write(dotLoc.loc, (uint16_t)(sizeof(uint8_t) + sizeof(const unsigned char) + lebSize));
                        write(dotLoc.loc, (uint8_t)op);
                        write(dotLoc.loc, (const unsigned char*)& lebSize, 1);
                        if (isUnsignedDIType(this, RegVar->getType()))
                        {
                            uint64_t constValue = pConstInt->getZExtValue();
                            write(dotLoc.loc, (unsigned char*)& constValue, lebSize);
                        }
                        else
                        {
                            int64_t constValue = pConstInt->getSExtValue();
                            write(dotLoc.loc, (unsigned char*)& constValue, lebSize);
                        }
                    }
                    RegVar->getDecorations().append("u ");
                }
                else if (Loc.IsRegister())
                {
                    DbgDecoder::VarInfo varInfo;
                    auto regNum = Loc.GetRegister();
                    m_pModule->getVarInfo("V", regNum, varInfo);

                    for (auto& genIsaRange : varInfo.lrs)
                    {
                        auto startIt = m_pModule->VISAIndexToAllGenISAOff.find(genIsaRange.start);
                        if (startIt == m_pModule->VISAIndexToAllGenISAOff.end())
                            continue;
                        uint64_t startRange = (*startIt).second.front();
                        auto endIt = m_pModule->VISAIndexToAllGenISAOff.find(genIsaRange.end);
                        if (endIt == m_pModule->VISAIndexToAllGenISAOff.end())
                            continue;
                        uint64_t endRange = (*endIt).second.back();

                        if (endRange < range.first)
                            continue;
                        if (startRange > range.second)
                            continue;

                        startRange = std::max(startRange, (uint64_t)range.first);
                        endRange = std::min(endRange, (uint64_t)range.second);

                        // Emit location to debug_loc
                        if (RegVar->getDotDebugLocOffset() == ~0U)
                        {
                            RegVar->setDotDebugLocOffset(offset);
                        }

                        write(dotLoc.loc, (unsigned char*)& startRange, pointerSize);
                        write(dotLoc.loc, (unsigned char*)& endRange, pointerSize);

                        if (genIsaRange.isGRF())
                        {
                            bool hasSubReg = genIsaRange.getGRF().subRegNum != 0;
                            unsigned int subRegSize = 0, offsetLEB128Size = 0, sizeLEB128Size = 0;
                            if (hasSubReg)
                            {
                                unsigned int subReg = genIsaRange.getGRF().subRegNum;
                                auto offsetInBits = subReg * 8;
                                auto sizeInBits = (m_pModule->m_pShader->getGRFSize() * 8) - offsetInBits;
                                offsetLEB128Size = encodeULEB128(offsetInBits, bufLEB128);
                                sizeLEB128Size = encodeULEB128(sizeInBits, bufLEB128);

                                subRegSize = sizeof(uint8_t) + offsetLEB128Size + sizeLEB128Size;
                            }

                            regNum = genIsaRange.getGRF().regNum;
                            unsigned int op = llvm::dwarf::DW_OP_breg0;
                            if (Loc.IsInMemory())
                            {
                                // use bregx
                            }
                            else
                            {
                                // use regx
                                op = llvm::dwarf::DW_OP_reg0;
                            }

                            if (!IGC_IS_FLAG_ENABLED(EnableSIMDLaneDebugging))
                            {
                                if (regNum <= 31)
                                {
                                    auto lebsize1 = op == llvm::dwarf::DW_OP_breg0 ? 1 : 0; // immediate 0 for breg0-31
                                    op += regNum;
                                    write(dotLoc.loc, (uint16_t)(sizeof(uint8_t) + lebsize1 + subRegSize));
                                    write(dotLoc.loc, (uint8_t)op);
                                    if (lebsize1 > 0)
                                    {
                                        write(dotLoc.loc, (uint8_t)0);
                                    }
                                }
                                else
                                {
                                    op = op == llvm::dwarf::DW_OP_breg0 ? llvm::dwarf::DW_OP_bregx : llvm::dwarf::DW_OP_regx;
                                    if (op == llvm::dwarf::DW_OP_bregx)
                                    {
                                        auto lebsize = encodeULEB128(regNum, bufLEB128);
                                        auto lebsize1 = 1; // immediate 0
                                        write(dotLoc.loc, (uint16_t)(sizeof(uint8_t) + lebsize + lebsize1 + subRegSize));
                                        write(dotLoc.loc, (uint8_t)op);
                                        write(dotLoc.loc, bufLEB128, lebsize);
                                        write(dotLoc.loc, (uint8_t)0);
                                    }
                                    else
                                    {
                                        auto lebsize = encodeULEB128(regNum, bufLEB128);
                                        write(dotLoc.loc, (uint16_t)(sizeof(uint8_t) + lebsize + subRegSize));
                                        write(dotLoc.loc, (uint8_t)op);
                                        write(dotLoc.loc, bufLEB128, lebsize);
                                    }
                                }

                                if (hasSubReg)
                                {
                                    unsigned int subReg = genIsaRange.getGRF().subRegNum;
                                    auto offsetInBits = subReg * 8;
                                    auto sizeInBits = (m_pModule->m_pShader->getGRFSize() * 8) - offsetInBits;

                                    write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_bit_piece);
                                    sizeLEB128Size = encodeULEB128(sizeInBits, bufLEB128);
                                    write(dotLoc.loc, (unsigned char*)bufLEB128, sizeLEB128Size);
                                    offsetLEB128Size = encodeULEB128(offsetInBits, bufLEB128);
                                    write(dotLoc.loc, (unsigned char*)bufLEB128, offsetLEB128Size);
                                }
                            }
                            else
                            {
                                IGC_ASSERT(IGC_IS_FLAG_ENABLED(EnableSIMDLaneDebugging));

                                uint32_t grfSize = m_pModule->m_pShader->getGRFSize();
                                unsigned int vectorNumElements = 1;
                                uint64_t varSizeInBits = Loc.IsInMemory() ? (uint64_t)Asm->GetPointerSize() * 8 : RegVar->getBasicSize(this);
                                uint32_t varSizeInReg = (Loc.IsInMemory() && varSizeInBits < 32) ? 32 : (uint32_t)varSizeInBits;
                                uint32_t numOfRegs = ((varSizeInReg * (uint32_t)simdWidth) > (grfSize * 8)) ?
                                    ((varSizeInReg * (uint32_t)simdWidth) / (grfSize * 8)) : 1;
                                uint8_t varSizeInBytes = 8;
                                if (varSizeInBits <= 0xFF)
                                {
                                    varSizeInBytes = 1;
                                }
                                else if (varSizeInBits <= 0xFFFF)
                                {
                                    varSizeInBytes = 2;
                                }
                                else if (varSizeInBits <= 0xFFFFFFFF)
                                {
                                    varSizeInBytes = 4;
                                }
                                IGC_ASSERT_MESSAGE(varSizeInBytes <= 4, "Unexpected variable's size");

                                auto simdLaneSize =
                                    sizeof(uint8_t) +                  // subReg or DW_OP_INTEL_push_simd_lane
                                    sizeof(uint8_t) +                  // opLit
                                    sizeof(uint8_t) +                  // DW_OP_shl
                                    sizeof(uint8_t) + varSizeInBytes + // DW_OP_const1u+val
                                    sizeof(uint8_t);                   // DW_OP_INTEL_bit_piece_stack

                                if (Loc.IsVectorized() == true)
                                {
                                    vectorNumElements = Loc.GetVectorNumElements();
                                }

                                // Calculate and write size of encodings for all vectors (if vectorized).
                                uint16_t allVectorsSize = 0;
                                auto regNumCurr = regNum;
                                for (unsigned int vectorElem = 0; vectorElem < vectorNumElements; ++vectorElem)
                                {
                                    if (regNumCurr <= 31)
                                    {
                                        // immediate 0 for breg0-31
                                        uint16_t lebsize1 = op >= llvm::dwarf::DW_OP_breg0 && op <= llvm::dwarf::DW_OP_breg31? 1 : 0;
                                        allVectorsSize += (uint16_t)(sizeof(uint8_t) + lebsize1 + simdLaneSize);
                                    }
                                    else
                                    {
                                        if (op == llvm::dwarf::DW_OP_breg0)
                                        {
                                            // DW_OP_bregx
                                            auto lebsize = encodeULEB128(regNumCurr, bufLEB128);
                                            uint16_t lebsize1 = 1; // immediate 0
                                            allVectorsSize += (uint16_t)(sizeof(uint8_t) + lebsize + lebsize1 + simdLaneSize);
                                        }
                                        else
                                        {
                                            // DW_OP_regx
                                            auto lebsize = encodeULEB128(regNum, bufLEB128);
                                            allVectorsSize += (uint16_t)(sizeof(uint8_t) + lebsize + simdLaneSize);
                                        }
                                    }
                                    regNumCurr = regNumCurr + numOfRegs;
                                }
                                write(dotLoc.loc, allVectorsSize);

                                auto opCurr = op;
                                for (unsigned int vectorElem = 0; vectorElem < vectorNumElements; ++vectorElem)
                                {
                                    if (regNum <= 31)
                                    {
                                        auto lebsize1 = op == llvm::dwarf::DW_OP_breg0 ? 1 : 0; // immediate 0 for breg0-31
                                        opCurr = op + regNum;
                                        write(dotLoc.loc, (uint8_t)opCurr);
                                        if (lebsize1 > 0)
                                        {
                                            write(dotLoc.loc, (uint8_t)0);
                                        }
                                    }
                                    else
                                    {
                                        opCurr = op == llvm::dwarf::DW_OP_breg0 ? llvm::dwarf::DW_OP_bregx : llvm::dwarf::DW_OP_regx;
                                        if (opCurr == llvm::dwarf::DW_OP_bregx)
                                        {
                                            write(dotLoc.loc, (uint8_t)opCurr);
                                            auto lebsize = encodeULEB128(regNum, bufLEB128);
                                            write(dotLoc.loc, bufLEB128, lebsize);
                                            write(dotLoc.loc, (uint8_t)0);
                                        }
                                        else
                                        {
                                            write(dotLoc.loc, (uint8_t)opCurr);
                                            auto lebsize = encodeULEB128(regNum, bufLEB128);
                                            write(dotLoc.loc, bufLEB128, lebsize);
                                        }
                                    }

                                    IGC_ASSERT_MESSAGE(varSizeInBits % 8 == 0, "Unexpected variable's size");
                                    IGC_ASSERT_MESSAGE(varSizeInBits <= 0x80000000, "Too huge variable's size");
                                    unsigned int opLit = llvm::dwarf::DW_OP_lit5;  // Assume unpacked <= 32 variable size

                                    // Verify if variable's size if a power of 2 and greater than 32 bits.
                                    bool isSizePowerOf2 = false;
                                    uint64_t powerOf2 = 1 << 5;
                                    for (int bitPos = 6; bitPos < 32; bitPos++)
                                    {
                                        powerOf2 = powerOf2 << 1;
                                        if (varSizeInBits == powerOf2)
                                        {
                                            isSizePowerOf2 = true;
                                            opLit = llvm::dwarf::DW_OP_lit0 + bitPos;
                                        }
                                    }

                                    IGC_ASSERT_MESSAGE(isSizePowerOf2 == true, "Missing support for variable size other than power of 2");

                                    if (Loc.IsVectorized() == false)
                                    {
                                        unsigned int subRegInBits = genIsaRange.getGRF().subRegNum * 8;

                                        // Scalar in a subregister
                                        write(dotLoc.loc, (uint8_t)subRegInBits);
                                    }
                                    else
                                    {
                                        // SIMD lane
                                        write(dotLoc.loc, (uint8_t)DW_OP_INTEL_push_simd_lane);
                                    }

                                    // If not directly in a register then fp16/int16/fp8/int8 unpacked in 32-bit subregister
                                    // as well as 32-bit float/int, while 64-bit variable takes two 32-bit subregisters.
                                    write(dotLoc.loc, (uint8_t)opLit);

                                    write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_shl);

                                    if (varSizeInBits <= 0xFF)
                                    {
                                        write(dotLoc.loc, (uint8_t)dwarf::DW_OP_const1u);
                                        write(dotLoc.loc, (uint8_t)varSizeInBits);
                                    }
                                    else if (varSizeInBits <= 0xFFFF)
                                    {
                                        write(dotLoc.loc, (uint8_t)dwarf::DW_OP_const2u);
                                        write(dotLoc.loc, (uint16_t)varSizeInBits);
                                    }
                                    else if (varSizeInBits <= 0xFFFFFFFF)
                                    {
                                        write(dotLoc.loc, (uint8_t)dwarf::DW_OP_const4u);
                                        write(dotLoc.loc, (uint32_t)varSizeInBits);
                                    }
                                    else
                                    {
                                        write(dotLoc.loc, (uint8_t)dwarf::DW_OP_const8u);
                                        write(dotLoc.loc, (uint64_t)varSizeInBits);
                                    }

                                    write(dotLoc.loc, (uint8_t)DW_OP_INTEL_bit_piece_stack);

                                    regNum = regNum + numOfRegs;
                                    IGC_ASSERT_MESSAGE(((simdWidth < 32) && (grfSize == 32)), "SIMD32 debugging not supported");

                                }
                            }
                        }
                        else if (genIsaRange.isSpill())
                        {
                            if (!IGC_IS_FLAG_ENABLED(EnableSIMDLaneDebugging))
                            {
                                Address addr;
                                addr.Set(Address::Space::eScratch, 0, genIsaRange.getSpillOffset().memoryOffset);

                                unsigned int op = llvm::dwarf::DW_OP_const8u;
                                write(dotLoc.loc, (uint16_t)(sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint8_t)));
                                write(dotLoc.loc, (uint8_t)op);
                                write(dotLoc.loc, addr.GetAddress());
                                op = llvm::dwarf::DW_OP_deref;
                                write(dotLoc.loc, (uint8_t)op);
                            }
                            else
                            {
                                // Scratch space
                                // 1 DW_OP_bregx <scrbase>, <offset>
                                uint64_t scratchBaseAddr = 0; // TBD MT
                                unsigned int vectorNumElements = 1;
                                uint32_t grfSize = m_pModule->m_pShader->getGRFSize();
                                uint64_t varSizeInBits = Loc.IsInMemory() ? (uint64_t)Asm->GetPointerSize() * 8 : RegVar->getBasicSize(this);
                                uint32_t varSizeInReg = (Loc.IsInMemory() && varSizeInBits < 32) ? 32 : (uint32_t)varSizeInBits;
                                uint32_t numOfRegs = ((varSizeInReg * (uint32_t)simdWidth) > (grfSize * 8)) ?
                                    ((varSizeInReg * (uint32_t)simdWidth) / (grfSize * 8)) : 1;
                                auto lebsizeScratchBaseAddr = encodeULEB128(scratchBaseAddr, bufLEB128); // Address for bregx
                                auto lebsizeScratchOffset = encodeULEB128(genIsaRange.getSpillOffset().memoryOffset, bufLEB128); // Offset for bregx
                                uint8_t varSizeInBytes = 8;
                                if (varSizeInBits <= 0xFF)
                                {
                                    varSizeInBytes = 1;
                                }
                                else if (varSizeInBits <= 0xFFFF)
                                {
                                    varSizeInBytes = 2;
                                }
                                else if (varSizeInBits <= 0xFFFFFFFF)
                                {
                                    varSizeInBytes = 4;
                                }
                                IGC_ASSERT_MESSAGE(varSizeInBytes <= 4, "Unexpected variable's size");

                                auto simdLaneSize =
                                    sizeof(uint8_t) +                  // subReg or DW_OP_INTEL_push_simd_lane
                                    sizeof(uint8_t) +                  // opLit
                                    sizeof(uint8_t) +                  // DW_OP_shl
                                    sizeof(uint8_t) +                  // DW_OP_plus
                                    sizeof(uint8_t);                   // DW_OP_deref

                                if (Loc.IsVectorized() == true)
                                {
                                    vectorNumElements = Loc.GetVectorNumElements();
                                }

                                // Calculate and write size of encodings for all vectors (if vectorized).
                                uint16_t allVectorsSize = 0;
                                auto currMemoryOffset = genIsaRange.getSpillOffset().memoryOffset;
                                for (unsigned int vectorElem = 0; vectorElem < vectorNumElements; ++vectorElem)
                                {
                                    if (IGC_IS_FLAG_ENABLED(EnableGTLocationDebugging))
                                    {
                                        // DW_OP_bregx
                                        lebsizeScratchOffset = encodeULEB128(currMemoryOffset, bufLEB128);
                                        allVectorsSize += (uint16_t)(sizeof(uint8_t) + lebsizeScratchBaseAddr + lebsizeScratchOffset + simdLaneSize);

                                        currMemoryOffset += numOfRegs * m_pModule->m_pShader->getGRFSize();
                                    }
                                    else
                                    {
                                        allVectorsSize += (uint16_t)(sizeof(uint8_t) + sizeof(uint64_t) + simdLaneSize);
                                    }
                                }

                                IGC_ASSERT_MESSAGE(varSizeInBits % 8 == 0, "Unexpected variable's size");

                                write(dotLoc.loc, allVectorsSize);

                                currMemoryOffset = genIsaRange.getSpillOffset().memoryOffset;
                                for (unsigned int vectorElem = 0; vectorElem < vectorNumElements; ++vectorElem)
                                {
                                    if (IGC_IS_FLAG_ENABLED(EnableGTLocationDebugging))
                                    {
                                        write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_bregx);
                                        lebsizeScratchBaseAddr = encodeULEB128(scratchBaseAddr, bufLEB128); // Address for bregx
                                        write(dotLoc.loc, bufLEB128, lebsizeScratchBaseAddr);
                                        lebsizeScratchOffset = encodeULEB128(currMemoryOffset, bufLEB128); // Offset for bregx
                                        write(dotLoc.loc, bufLEB128, lebsizeScratchOffset);

                                        currMemoryOffset += numOfRegs * m_pModule->m_pShader->getGRFSize();
                                    }
                                    else
                                    {
                                        Address addr;
                                        uint64_t memoryOffset = (uint64_t)genIsaRange.getSpillOffset().memoryOffset +
                                            (uint64_t)(vectorElem * numOfRegs * m_pModule->m_pShader->getGRFSize());
                                        addr.Set(Address::Space::eScratch, 0, memoryOffset);

                                        write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_const8u);
                                        write(dotLoc.loc, addr.GetAddress());
                                    }

                                    // Emit SIMD lane for spill (unpacked)
                                    IGC_ASSERT_MESSAGE(varSizeInBits % 8 == 0, "Unexpected variable's size");
                                    IGC_ASSERT_MESSAGE(varSizeInBits <= 0x80000000, "Too huge variable's size");

                                    unsigned int opLit = llvm::dwarf::DW_OP_lit6;  // Always 64-bit ptrs in scratch space
#if 0
                                    unsigned int opLit = llvm::dwarf::DW_OP_lit5;  // Assume unpacked <= 32 variable size

                                    // Verify if variable's size if a power of 2 and greater than 32 bits.
                                    bool isSizePowerOf2 = false;
                                    uint64_t powerOf2 = 1 << 5;
                                    for (int bitPos = 6; bitPos < 32; bitPos++)
                                    {
                                        powerOf2 = powerOf2 << 1;
                                        if (varSizeInBits == powerOf2)
                                        {
                                            isSizePowerOf2 = true;
                                            opLit = llvm::dwarf::DW_OP_lit0 + bitPos;
                                        }
                                    }
#endif // 0
                                    // SIMD lane
                                    write(dotLoc.loc, (uint8_t)DW_OP_INTEL_push_simd_lane);

                                    // If not directly in a register then fp16/int16/fp8/int8 unpacked in 32-bit subregister
                                    // as well as 32-bit float/int, while 64-bit variable takes two 32-bit subregisters.
                                    write(dotLoc.loc, (uint8_t)opLit);
                                    write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_shl);
                                    write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_plus);
                                    write(dotLoc.loc, (uint8_t)llvm::dwarf::DW_OP_deref);

                                    regNum = regNum + numOfRegs;
                                    IGC_ASSERT_MESSAGE(((simdWidth < 32) && (grfSize == 32)), "SIMD32 debugging not supported");
                                }
                            }
                        }
                        if (Loc.IsVectorized())
                            RegVar->getDecorations().append("v ");
                        else
                            RegVar->getDecorations().append("u ");
                    }
                }
                if (RegVar->getDotDebugLocOffset() != ~0U)
                {
                    offset += dotLoc.loc.size();
                    TempDotDebugLocEntries.push_back(dotLoc);
                }
            }
        }
        if (TempDotDebugLocEntries.size() > origLocSize)
        {
            TempDotDebugLocEntries.push_back(DotDebugLocEntry());
            offset += pointerSize * 2;
        }

#if 0
        // Simplify ranges that are fully coalesced.
        if (History.size() <= 1 || (History.size() == 2 && pInst->isIdenticalTo(History.back())))
        {
            RegVar->setDbgInst(pInst);
            continue;
        }

        // Handle multiple DBG_VALUE instructions describing one variable.
        RegVar->setDotDebugLocOffset(DotDebugLocEntries.size());

        for (SmallVectorImpl<const Instruction*>::const_iterator
            HI = History.begin(), HE = History.end(); HI != HE; ++HI)
        {
            const Instruction* Begin = *HI;
            IGC_ASSERT_MESSAGE(m_pModule->IsDebugValue(Begin), "Invalid History entry");

            // Compute the range for a register location.
            const MCSymbol* FLabel = getLabelBeforeInsn(Begin);
            const MCSymbol* SLabel = 0;

            if (HI + 1 == HE)
            {
                // If Begin is the last instruction in History then its value is valid
                // until the end of the function.
                SLabel = FunctionEndSym;
            }
            else
            {
                const Instruction* End = HI[1];
                DEBUG(dbgs() << "DotDebugLoc Pair:\n" << "\t" << *Begin << "\t" << *End << "\n");
                if (m_pModule->IsDebugValue(End))
                {
                    SLabel = getLabelBeforeInsn(End);
                }
                else
                {
                    // End is a normal instruction clobbering the range.
                    SLabel = getLabelAfterInsn(End);
                    IGC_ASSERT_MESSAGE(SLabel, "Forgot label after clobber instruction");
                    ++HI;
                }
            }

            // The value is valid until the next DBG_VALUE or clobber.
            const MDNode* Var = m_pModule->GetDebugVariable(Begin);
            DotDebugLocEntries.push_back(DotDebugLocEntry(FLabel, SLabel, Begin, Var));
        }
        DotDebugLocEntries.push_back(DotDebugLocEntry());
#endif
    }

    // Collect info for variables that were optimized out.
    LexicalScope* FnScope = LScopes.getCurrentFunctionScope();
#if LLVM_VERSION_MAJOR == 4
    DILocalVariableArray Variables = cast<DISubprogram>(FnScope->getScopeNode())->getVariables();
#elif LLVM_VERSION_MAJOR >= 7
    auto Variables = cast<DISubprogram>(FnScope->getScopeNode())->getRetainedNodes();
#endif

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

unsigned int DwarfDebug::CopyDebugLoc(unsigned int o)
{
    // TempDotLocEntries has all entries discovered in collectVariableInfo.
    // But some of those entries may not get emitted. This function
    // is invoked when writing out DIE. At this time, it can be decided
    // whether debug_range for a variable will be emitted to debug_ranges.
    // If yes, it is copied over to DotDebugLocEntries and new offset is
    // returned.
    unsigned int offset = 0, index = 0;
    bool found = false, done = false;
    unsigned int pointerSize = getPointerSize(*const_cast<llvm::Module*>(m_pModule->GetModule()));

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
            return DebugLoc::get(SP->getScopeLine(), 0, SP);
        }
        return DebugLoc::get(SP->getLine(), 0, SP);
    }

    return DebugLoc();
}

// Gather pre-function debug information.  Assumes being called immediately
// after the function entry point has been emitted.
void DwarfDebug::beginFunction(const Function* MF, IGC::VISAModule* v)
{
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
    FunctionBeginSym = Asm->GetTempSymbol("func_begin", m_pModule->GetFunctionNumber(MF));
    // Assumes in correct section after the entry point.
    Asm->EmitLabel(FunctionBeginSym);

    llvm::MDNode* prevIAT = nullptr;

    for (auto II = m_pModule->begin(), IE = m_pModule->end(); II != IE; ++II)
    {
        const Instruction* MI = *II;

        if (MI->getDebugLoc() &&
            MI->getDebugLoc().getScope() != prevIAT)
        {
            SameIATInsts[MI->getDebugLoc().getInlinedAt()].push_back(MI);
            prevIAT = MI->getDebugLoc().getInlinedAt();
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
        else
        {
            // Not a DBG_VALUE instruction.

            // First known non-DBG_VALUE and non-frame setup location marks
            // the beginning of the function body.
            if (!PrologEndLoc && MI->getDebugLoc())
            {
                PrologEndLoc = MI->getDebugLoc();
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
    FunctionEndSym = Asm->GetTempSymbol("func_end", m_pModule->GetFunctionNumber(MF));
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
#if LLVM_VERSION_MAJOR == 4
            DILocalVariableArray Variables = SP->getVariables();
#elif LLVM_VERSION_MAJOR >= 7
            auto Variables = SP->getRetainedNodes();
#endif

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
        if (m_pModule->hasOrIsStackCall())
        {
            writeFDEStackCall(m_pModule);
        }
        else
        {
            writeFDESubroutine(m_pModule);
        }
    }

    // Clear debug info
    for (ScopeVariablesMap::iterator
        I = ScopeVariables.begin(), E = ScopeVariables.end(); I != E; ++I)
    {
        DeleteContainerPointers(I->second);
    }
    ScopeVariables.clear();
    DeleteContainerPointers(CurrentFnArguments);
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

    TextSectionSym = emitSectionSym(Asm, Asm->GetTextSection(), "text_begin");

    emitSectionSym(Asm, Asm->GetDataSection());
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
            // DW_AT_range Value encodes offset in debug_range section.
            Values[i]->EmitValue(Asm, Form);
            break;
        case dwarf::DW_AT_location:
            if (DIELabel * L = dyn_cast<DIELabel>(Values[i]))
            {
                Asm->EmitSectionOffset(L->getValue(), DwarfDebugLocSectionSym);
            }
            else
            {
                Values[i]->EmitValue(Asm, Form);
            }
            break;
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

    if (m_pModule->isDirectElfInput)
    {
        // Range is already in Gen ISA units so emit it as integer
        for (auto& data : GenISADebugRangeSymbols)
        {
            Asm->EmitIntValue(data, size);
        }
    }
    else
    {
        for (SmallVectorImpl<const MCSymbol*>::iterator
            I = DebugRangeSymbols.begin(), E = DebugRangeSymbols.end();
            I != E; ++I)
        {
            if (*I)
            {
#if 0
                Asm->EmitIntValue((*I)->getOffset(), size);
#else
                Asm->EmitSymbolValue(const_cast<MCSymbol*>(*I), size);
#endif
            }
            else
                Asm->EmitIntValue(0, size);
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

void DwarfDebug::writeCIE()
{
    std::vector<uint8_t> data;

    // Emit CIE
    auto ptrSize = Asm->GetPointerSize();
    // The size of the length field plus the value of length must be an integral multiple of the address size.
    uint8_t lenSize = ptrSize;
    if (ptrSize == 8)
        lenSize = 12;

    // Write CIE_id
    // Only 1 CIE is emitted
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
    writeULEB128(data, returnReg);


    // initial instructions (array of ubyte)
    // DW_CFA_def_cfa -> fpreg+0
    write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa);
    writeULEB128(data, fpReg);
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

    if (ptrSize == 8)
        Asm->EmitInt32(0xffffffff);
    Asm->EmitIntValue(data.size(), ptrSize);

    for (auto& byte : data)
        Asm->EmitInt8(byte);
}

void DwarfDebug::writeFDESubroutine(VISAModule* m)
{
    std::vector<uint8_t> data;

    auto firstInst = (m->GetInstInfoMap()->begin())->first;
    auto funcName = firstInst->getParent()->getParent()->getName();

    IGC::DbgDecoder::SubroutineInfo* sub = nullptr;
    auto co = m->getCompileUnit();
    for (auto& s : co->subs)
    {
        if (s.name.compare(funcName) == 0)
        {
            sub = &s;
            break;
        }
    }

    if (!sub)
        return;

    // Emit CIE
    auto ptrSize = Asm->GetPointerSize();
    uint8_t lenSize = 4;
    if (ptrSize == 8)
        lenSize = 12;

    // CIE_ptr (4/8 bytes)
    write(data, ptrSize == 4 ? (uint32_t)0 : (uint64_t)0);

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
    uint64_t genOffStart = m->GetDwarfDebug()->lowPc;
    uint64_t genOffEnd = m->GetDwarfDebug()->highPc;
    auto& retvarLR = sub->retval;
    IGC_ASSERT_MESSAGE(retvarLR.size() > 0, "expecting GRF for return");
    IGC_ASSERT_MESSAGE(retvarLR[0].var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF, "expecting GRF for return");

    // assume ret var is live throughout sub-routine and it is contained
    // in same GRF.
    uint32_t linearAddr = (retvarLR.front().var.mapping.r.regNum * m_pModule->m_pShader->getGRFSize()) +
        retvarLR.front().var.mapping.r.subRegNum;

    // initial location
    write(data, ptrSize == 4 ? (uint32_t)genOffStart : genOffStart);

    // address range
    write(data, ptrSize == 4 ? (uint32_t)(genOffEnd - genOffStart) :
        (genOffEnd - genOffStart));

    // instruction - ubyte
    write(data, (uint8_t)llvm::dwarf::DW_CFA_register);

    // return reg operand
    writeULEB128(data, returnReg);

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
    // <ip, <instructiont to write>
    auto sortAsc = [](uint64_t a, uint64_t b) { return a < b; };
    std::map <uint64_t, std::vector<uint8_t>, decltype(sortAsc)> cfaOps(sortAsc);
    auto& dbgInfo = *m->getCompileUnit();

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

    auto writeSameValue = [](std::vector<uint8_t>& data, uint32_t srcReg)
    {
        write(data, (uint8_t)llvm::dwarf::DW_CFA_same_value);
        writeULEB128(data, srcReg);
    };

    auto writeRegToMem = [](std::vector<uint8_t>& data, uint32_t srcReg, DbgDecoder::Mapping& mapping)
    {
        // srcReg -> (fp) + mapping.m.offset
        write(data, (uint8_t)llvm::dwarf::DW_CFA_offset_extended);
        writeULEB128(data, srcReg);
        writeULEB128(data, mapping.m.memoryOffset);
        IGC_ASSERT_MESSAGE(!mapping.m.isBaseOffBEFP, "Expecting location offset from BE_FP");
    };

    auto writeNonCFALoc = [](std::vector<uint8_t>& data, uint32_t srcReg, DbgDecoder::LiveIntervalGenISA& lr)
    {
        if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory)
        {
            write(data, (uint8_t)llvm::dwarf::DW_CFA_offset_extended);
            writeULEB128(data, srcReg);
            writeULEB128(data, lr.var.mapping.m.memoryOffset);
            IGC_ASSERT_MESSAGE(!lr.var.mapping.m.isBaseOffBEFP, "Expecting location offset from BE_FP");
        }
        else if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF)
        {
            write(data, (uint8_t)llvm::dwarf::DW_CFA_register);
            writeULEB128(data, srcReg);
            writeULEB128(data, lr.var.mapping.r.regNum);
        }
    };

    auto writeCFALoc = [](std::vector<uint8_t>& data, uint32_t srcReg, DbgDecoder::LiveIntervalGenISA& lr)
    {
        if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory)
        {
            // map out CFA to an offset on be stack
            // TODO: use  DW_CFA_def_cfa_expression
            write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa);
            // The DW_CFA_def_cfa instruction takes two unsigned LEB128 operands representing a register number and a (non-factored) offset.
            writeULEB128(data, srcReg);
            writeULEB128(data, lr.var.mapping.m.memoryOffset);
            IGC_ASSERT_MESSAGE(!lr.var.mapping.m.isBaseOffBEFP, "Expecting location offset from BE_FP");
        }
        else if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF)
        {
            // map CFA to physical GRF
            write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa_register);
            writeULEB128(data, lr.var.mapping.r.regNum);
            write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa_offset);
            writeULEB128(data, 0);
        }
    };

    auto ptrSize = Asm->GetPointerSize();
    auto& cfi = dbgInfo.cfi;
    // Emit CIE
    uint8_t lenSize = 4;
    if (ptrSize == 8)
        lenSize = 12;

    // CIE_ptr (4/8 bytes)
    write(data, ptrSize == 4 ? (uint32_t)0 : (uint64_t)0);

    // initial location
    auto genOffStart = dbgInfo.relocOffset;
    auto genOffEnd = highPc;

    write(data, ptrSize == 4 ? (uint32_t)genOffStart : (uint64_t)genOffStart);

    // address range
    write(data, ptrSize == 4 ? (uint32_t)(genOffEnd - genOffStart) :
        (uint64_t)(genOffEnd - genOffStart));

    // emit same value for all callee save entries in frame
    std::unordered_set<uint32_t> uniqueCalleeSave;
    for (auto& item : cfi.calleeSaveEntry)
    {
        for (unsigned int idx = 0; idx != item.data.size(); ++idx)
        {
            auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->m_pShader->getGRFSize());
            uniqueCalleeSave.insert(regNum);
        }
    }

    for (auto r : uniqueCalleeSave)
    {
        writeSameValue(cfaOps[0], r);
    }

    // first write instructions to retrieve CFA (ie, be_fp of caller frame)
    if (cfi.callerbefpValid)
    {
        auto& callerFP = cfi.callerbefp;
        unsigned int regNum = 0;
        for (auto& item : callerFP)
        {
            if (item.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory)
            {
                // Caller CFA in memory offset by current CFA
                writeNonCFALoc(cfaOps[item.start], regNum, item);
            }
            else
            {
                regNum = item.var.mapping.r.regNum;
                writeCFALoc(cfaOps[item.start], fpReg, item);
            }
        }
    }

    // write return addr on stack
    if (cfi.retAddrValid)
    {
        auto& retAddr = cfi.retAddr;
        for (auto& item : retAddr)
        {
            writeNonCFALoc(cfaOps[item.start], returnReg, item);
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
                auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->m_pShader->getGRFSize());
                if (calleeSaveRegsSaved.find(regNum) == calleeSaveRegsSaved.end())
                {
                    writeRegToMem(cfaOps[item.genIPOffset], regNum, item.data[idx].dst);
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
                    auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->m_pShader->getGRFSize());
                    if ((*it) == regNum)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    writeSameValue(cfaOps[item.genIPOffset], (*it));
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

    for (auto& byte : data)
        Asm->EmitInt8(byte);
}

void DwarfDebug::gatherDISubprogramNodes()
{
    // Discover all DISubprogram nodes in program and store them
    // in an std::set. With LLVM 4.0 DISubprogram nodes are no
    // longer stored in DICompileUnit. Instead DISubprogram nodes
    // point to their corresponding DICompileUnit. This function
    // iterates over all instructions to find unique DISubprogram
    // nodes and stores them in an std::set for other functions
    // to iterate over.

    DISubprogramNodes.clear();
    DISPToFunction.clear();

    for (auto& F : *m_pModule->GetModule())
    {
        if (auto diSubprogram = F.getSubprogram())
        {
            addUniqueDISP(diSubprogram);
        }

        for (auto& bb : F)
        {
            for (auto& inst : bb)
            {
                auto debugLoc = inst.getDebugLoc();
                while (debugLoc)
                {
                    auto scope = debugLoc.getScope();
                    if (scope &&
                        dyn_cast_or_null<llvm::DILocalScope>(scope))
                    {
                        auto DISP = cast<llvm::DILocalScope>(scope)->getSubprogram();
                        addUniqueDISP(DISP);
                        DISPToFunction.insert(std::make_pair(DISP, &F));
                    }

                    if (debugLoc.getInlinedAt())
                        debugLoc = debugLoc.getInlinedAt();
                    else
                        debugLoc = nullptr;
                }
            }
        }
    }

    m_pModule->setDISPToFuncMap(&DISPToFunction);
}

bool VISAModule::getVarInfo(std::string prefix, unsigned int vreg, DbgDecoder::VarInfo& var)
{
    std::string name = prefix + std::to_string(vreg);
    if (VirToPhyMap.size() == 0)
    {
        // populate map one time
        auto co = getCompileUnit();
        if (co)
        {
            for (auto& v : co->Vars)
            {
                VirToPhyMap.insert(std::make_pair(v.name, v));
            }
        }
    }

    auto it = VirToPhyMap.find(name);
    if (it == VirToPhyMap.end())
        return false;

    var = (*it).second;
    return true;
}

bool VISAModule::hasOrIsStackCall() const
{
    auto co = getCompileUnit();
    if (!co)
        return false;

    auto& cfi = co->cfi;
    if (cfi.befpValid || cfi.frameSize > 0 || cfi.retAddr.size() > 0)
        return true;

    return false;
}

std::vector<DbgDecoder::SubroutineInfo>* VISAModule::getSubroutines() const
{
    std::vector<DbgDecoder::SubroutineInfo> subs;
    auto co = getCompileUnit();
    if (co)
        return &co->subs;
    return nullptr;
}

DbgDecoder::DbgInfoFormat* VISAModule::getCompileUnit() const
{
    for (auto& co : dd->getDecodedDbg()->compiledObjs)
    {
        // TODO: Fix this for stack call use
        if (dd->getDecodedDbg()->compiledObjs.size() == 1 ||
            co.kernelName.compare(m_pEntryFunc->getName().str()) == 0)
        {
            return &co;
        }
    }

    return nullptr;
}

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
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfCompilerUnit.cpp
///////////////////////////////////////////////////////////////////////////////

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/GlobalValue.h"
#include "llvmWrapper/IR/IntrinsicInst.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#if LLVM_VERSION_MAJOR >= 11
#include "llvm/CodeGen/DIE.h"
#endif
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IntrinsicInst.h"
#include "common/LLVMWarningsPop.hpp"

#include "DwarfCompileUnit.hpp"
#include "DIE.hpp"
#include "DwarfDebug.hpp"
#include "StreamEmitter.hpp"
#include "VISAModule.hpp"
#include "Version.hpp"

#include "Compiler/CISACodeGen/messageEncoding.hpp"

#include "Probe/Assertion.h"

#define DEBUG_TYPE "dwarfdebug"

using namespace llvm;
using namespace ::IGC;

/// CompileUnit - Compile unit constructor.
CompileUnit::CompileUnit(unsigned UID, DIE* D, DICompileUnit* Node,
    StreamEmitter* A, IGC::DwarfDebug* DW)
    : UniqueID(UID), Node(Node), CUDie(D), Asm(A),
    EmitSettings(A->GetEmitterSettings()),
    DD(DW), IndexTyDie(0), DebugInfoOffset(0)
{
    DIEIntegerOne = new (DIEValueAllocator)DIEInteger(1);
    insertDIE(Node, D);
}

/// ~CompileUnit - Destructor for compile unit.
CompileUnit::~CompileUnit()
{
    for (unsigned j = 0, M = DIEBlocks.size(); j < M; ++j)
        DIEBlocks[j]->~DIEBlock();

    for (unsigned j = 0, M = DIEInlinedStrings.size(); j < M; ++j)
        DIEInlinedStrings[j]->~DIEInlinedString();
}

/// createDIEEntry - Creates a new DIEEntry to be a proxy for a debug
/// information entry.
IGC::DIEEntry* CompileUnit::createDIEEntry(DIE* Entry)
{
    DIEEntry* Value = new (DIEValueAllocator)DIEEntry(Entry, DD->getDwarfVersion());
    return Value;
}

/// getDefaultLowerBound - Return the default lower bound for an array. If the
/// DWARF version doesn't handle the language, return -1.
int64_t CompileUnit::getDefaultLowerBound() const
{
    switch (getLanguage())
    {
    default:
        break;

    case dwarf::DW_LANG_C89:
    case dwarf::DW_LANG_C99:
    case dwarf::DW_LANG_C:
    case dwarf::DW_LANG_C_plus_plus:
    case dwarf::DW_LANG_ObjC:
    case dwarf::DW_LANG_ObjC_plus_plus:
        return 0;

    case dwarf::DW_LANG_Fortran77:
    case dwarf::DW_LANG_Fortran90:
    case dwarf::DW_LANG_Fortran95:
        return 1;

        // The languages below have valid values only if the DWARF version >= 4.
    case dwarf::DW_LANG_Java:
    case dwarf::DW_LANG_Python:
    case dwarf::DW_LANG_UPC:
    case dwarf::DW_LANG_D:
        if (dwarf::DWARF_VERSION >= 4)
            return 0;
        break;

    case dwarf::DW_LANG_Ada83:
    case dwarf::DW_LANG_Ada95:
    case dwarf::DW_LANG_Cobol74:
    case dwarf::DW_LANG_Cobol85:
    case dwarf::DW_LANG_Modula2:
    case dwarf::DW_LANG_Pascal83:
    case dwarf::DW_LANG_PLI:
        if (dwarf::DWARF_VERSION >= 4)
            return 1;
        break;
    }

    return -1;
}

/// Check whether the DIE for this MDNode can be shared across CUs.
static bool isShareableAcrossCUs(llvm::MDNode* D)
{
    // When the MDNode can be part of the type system, the DIE can be
    // shared across CUs.
    return (isa<DIType>(D) || (isa<DISubprogram>(D) && !(cast<DISubprogram>(D)->isDefinition())));
}

/// getDIE - Returns the debug information entry map slot for the
/// specified debug variable. We delegate the request to DwarfDebug
/// when the DIE for this MDNode can be shared across CUs. The mappings
/// will be kept in DwarfDebug for shareable DIEs.
::IGC::DIE* CompileUnit::getDIE(llvm::DINode* D) const {
    if (isShareableAcrossCUs(D))
        return DD->getDIE(D);
    return MDNodeToDieMap.lookup(D);
}

/// insertDIE - Insert DIE into the map. We delegate the request to DwarfDebug
/// when the DIE for this MDNode can be shared across CUs. The mappings
/// will be kept in DwarfDebug for shareable DIEs.
void CompileUnit::insertDIE(llvm::MDNode* Desc, DIE* D)
{
    if (isShareableAcrossCUs(Desc))
    {
        DD->insertDIE(Desc, D);
        return;
    }
    MDNodeToDieMap.insert(std::make_pair(Desc, D));
}

/// addFlag - Add a flag that is true.
void CompileUnit::addFlag(DIE* Die, dwarf::Attribute Attribute)
{
    if (DD->getDwarfVersion() >= 4)
        Die->addValue(Attribute, dwarf::DW_FORM_flag_present, DIEIntegerOne);
    else
        Die->addValue(Attribute, dwarf::DW_FORM_flag, DIEIntegerOne);
}

/// addUInt - Add an unsigned integer attribute data and value.
///
void CompileUnit::addUInt(DIE* Die, dwarf::Attribute Attribute,
    Optional<dwarf::Form> Form, uint64_t Integer)
{
    if (!Form)
    {
        Form = DIEInteger::BestForm(false, Integer);
    }
    DIEValue* Value = Integer == 1 ? DIEIntegerOne : new (DIEValueAllocator)DIEInteger(Integer);
    Die->addValue(Attribute, *Form, Value);
}

void CompileUnit::addUInt(IGC::DIEBlock* Block, dwarf::Form Form, uint64_t Integer)
{
    IGC_ASSERT_MESSAGE((Form == dwarf::Form::DW_FORM_data1 && Integer <= std::numeric_limits<unsigned char>::max()) ||
        (Form == dwarf::Form::DW_FORM_data2 && Integer <= std::numeric_limits<unsigned short>::max()) ||
        (Form == dwarf::Form::DW_FORM_data4 && Integer <= std::numeric_limits<unsigned int>::max() ||
        (Form != dwarf::Form::DW_FORM_data1 && Form != dwarf::Form::DW_FORM_data2 && Form != dwarf::Form::DW_FORM_data4)),
        "Insufficient bits in form for encoding");
    addUInt(Block, (dwarf::Attribute)0, Form, Integer);
}

/// addSInt - Add an signed integer attribute data and value.
///
void CompileUnit::addSInt(DIE* Die, dwarf::Attribute Attribute,
    Optional<dwarf::Form> Form, int64_t Integer)
{
    if (!Form)
    {
        Form = DIEInteger::BestForm(true, Integer);
    }
    DIEValue* Value = new (DIEValueAllocator)DIEInteger(Integer);
    Die->addValue(Attribute, *Form, Value);
}

void CompileUnit::addSInt(IGC::DIEBlock* Die, Optional<dwarf::Form> Form, int64_t Integer)
{
    IGC_ASSERT_MESSAGE((Form == dwarf::Form::DW_FORM_data1 && Integer >= std::numeric_limits<signed char>::min() && Integer <= std::numeric_limits<signed char>::max()) ||
        (Form == dwarf::Form::DW_FORM_data2 && Integer >= std::numeric_limits<short>::min() && Integer <= std::numeric_limits<short>::max()) ||
        (Form == dwarf::Form::DW_FORM_data4 && Integer >= std::numeric_limits<int>::min() && Integer <= std::numeric_limits<int>::max()) ||
        (Form != dwarf::Form::DW_FORM_data1 && Form != dwarf::Form::DW_FORM_data2 && Form != dwarf::Form::DW_FORM_data4),
        "Insufficient bits in form for encoding");
    addSInt(Die, (dwarf::Attribute)0, Form, Integer);
}

/// addString - Add a string attribute data and value. We always emit a
/// reference to the string pool instead of immediate strings so that DIEs have
/// more predictable sizes. In the case of split dwarf we emit an index
/// into another table which gets us the static offset into the string
/// table.
void CompileUnit::addString(DIE* Die, dwarf::Attribute Attribute, StringRef String)
{
    if (DD->IsDirectElfInput())
    {
        // Emit string inlined
        auto Str = new (DIEValueAllocator) DIEInlinedString(String);
        // Collect all inlined string DIEs to later call dtor
        DIEInlinedStrings.push_back(Str);
        Die->addValue(Attribute, dwarf::DW_FORM_string, Str);
    }
    else
    {
        MCSymbol* Symb = DD->getStringPoolEntry(String);
        DIEValue* Value = new (DIEValueAllocator)DIELabel(Symb);
        DIEValue* Str = new (DIEValueAllocator)DIEString(Value, String);
        Die->addValue(Attribute, dwarf::DW_FORM_strp, Str);
    }
}

/// addExpr - Add a Dwarf expression attribute data and value.
///
void CompileUnit::addExpr(IGC::DIEBlock* Die, dwarf::Form Form, const MCExpr* Expr)
{
    DIEValue* Value = new (DIEValueAllocator)DIEExpr(Expr);
    Die->addValue((dwarf::Attribute)0, Form, Value);
}

/// addLabel - Add a Dwarf label attribute data and value.
///
void CompileUnit::addLabel(DIE* Die, dwarf::Attribute Attribute, dwarf::Form Form, const MCSymbol* Label)
{
    DIEValue* Value = new (DIEValueAllocator)DIELabel(Label);
    Die->addValue(Attribute, Form, Value);
}

void CompileUnit::addLabel(IGC::DIEBlock* Die, dwarf::Form Form, const MCSymbol* Label)
{
    addLabel(Die, (dwarf::Attribute)0, Form, Label);
}

/// addLabelAddress - Add a dwarf label attribute data and value using
/// DW_FORM_addr or DW_FORM_GNU_addr_index.
///
void CompileUnit::addLabelAddress(DIE* Die, dwarf::Attribute Attribute, MCSymbol* Label)
{
    if (Label)
    {
        DD->addArangeLabel(SymbolCU(this, Label));
    }

    if (Label != NULL)
    {
        DIEValue* Value = new (DIEValueAllocator)DIELabel(Label);
        Die->addValue(Attribute, dwarf::DW_FORM_addr, Value);
    }
    else
    {
        DIEValue* Value = new (DIEValueAllocator)DIEInteger(0);
        Die->addValue(Attribute, dwarf::DW_FORM_addr, Value);
    }
}

/// addOpAddress - Add a dwarf op address data and value using the
/// form given and an op of either DW_FORM_addr or DW_FORM_GNU_addr_index.
///
void CompileUnit::addOpAddress(IGC::DIEBlock* Die, const MCSymbol* Sym)
{
    DD->addArangeLabel(SymbolCU(this, Sym));
    addUInt(Die, dwarf::DW_FORM_data1, dwarf::DW_OP_addr);
    addLabel(Die, dwarf::DW_FORM_udata, Sym);
}

/// addDelta - Add a label delta attribute data and value.
///
void CompileUnit::addDelta(DIE* Die, dwarf::Attribute Attribute,
    dwarf::Form Form, const MCSymbol* Hi, const MCSymbol* Lo)
{
    DIEValue* Value = new (DIEValueAllocator)DIEDelta(Hi, Lo);
    Die->addValue(Attribute, Form, Value);
}

/// addDIEEntry - Add a DIE attribute data and value.
///
void CompileUnit::addDIEEntry(DIE* Die, dwarf::Attribute Attribute, DIE* Entry)
{
    addDIEEntry(Die, Attribute, createDIEEntry(Entry));
}

void CompileUnit::addDIEEntry(DIE* Die, dwarf::Attribute Attribute, DIEEntry* Entry)
{
    const DIE* DieCU = Die->getCompileUnitOrNull();
    const DIE* EntryCU = Entry->getEntry()->getCompileUnitOrNull();
    if (!DieCU)
    {
        // We assume that Die belongs to this CU, if it is not linked to any CU yet.
        DieCU = getCUDie();
    }
    if (!EntryCU)
    {
        EntryCU = getCUDie();
    }
    dwarf::Form Form = EntryCU == DieCU ? dwarf::DW_FORM_ref4 : dwarf::DW_FORM_ref_addr;
    Die->addValue(Attribute, Form, Entry);
}

/// Create a DIE with the given Tag, add the DIE to its parent, and
/// call insertDIE if MD is not null.
IGC::DIE* CompileUnit::createAndAddDIE(unsigned Tag, DIE& Parent, llvm::DINode* N)
{
    DIE* Die = new DIE(Tag);
    Parent.addChild(Die);
    if (N)
    {
        insertDIE(N, Die);
    }

    return Die;
}

/// addBlock - Add block data.
///
void CompileUnit::addBlock(DIE* Die, dwarf::Attribute Attribute, IGC::DIEBlock* Block)
{
    Block->ComputeSize(Asm);
    DIEBlocks.push_back(Block); // Memoize so we can call the destructor later on.
    Die->addValue(Attribute, Block->BestForm(), Block);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE* Die, DIScope* S, unsigned Line)
{
    // If the line number is 0, don't add it.
    if (Line == 0) return;

    unsigned FileID = DD->getOrCreateSourceID(S->getFilename(), S->getDirectory(), getUniqueID());
    IGC_ASSERT_MESSAGE(FileID, "Invalid file id");
    addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
    addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE* Die, DIVariable* V)
{
    // Verify variable.
    if (!isa<DIVariable>(V)) return;

    addSourceLine(Die, V->getScope(), V->getLine());
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE* Die, DISubprogram* SP)
{
    // Verify subprogram.
    if (!isa<DISubprogram>(SP)) return;

    addSourceLine(Die, SP, SP->getLine());
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE* Die, DIType* Ty)
{
    // Verify type.
    if (!isa<DIType>(Ty)) return;

    addSourceLine(Die, Ty, Ty->getLine());
}

#if LLVM_VERSION_MAJOR == 4
/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE * Die, DINamespace * NS)
{
    // Verify namespace.
    if (!isa<DINamespace>(NS)) return;

    addSourceLine(Die, NS, NS->getLine());
}
#endif

void CompileUnit::addRegisterLoc(IGC::DIEBlock* TheDie, unsigned DWReg, int64_t Offset, const llvm::Instruction* dbgInst)
{
    if (isa<llvm::DbgDeclareInst>(dbgInst))
        addRegisterOffset(TheDie, DWReg, Offset);
    else
        addRegisterOp(TheDie, DWReg);
}

/// addRegisterOp - Add register operand.
void CompileUnit::addRegisterOp(IGC::DIEBlock* TheDie, unsigned DWReg)
{
    auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(
        DWReg, EmitSettings.UseNewRegisterEncoding);
    if (DWRegEncoded < 32)
    {
        addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_reg0 + DWRegEncoded);
    }
    else
    {
        addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_regx);
        addUInt(TheDie, dwarf::DW_FORM_udata, DWRegEncoded);
    }
}

/// addRegisterOffset - Add register offset.
void CompileUnit::addRegisterOffset(IGC::DIEBlock* TheDie, unsigned DWReg, int64_t Offset)
{
    auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(
        DWReg, EmitSettings.UseNewRegisterEncoding);
    if (DWRegEncoded < 32)
    {
        addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_breg0 + DWRegEncoded);
    }
    else
    {
        addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_bregx);
        addUInt(TheDie, dwarf::DW_FORM_udata, DWRegEncoded);
    }
    addSInt(TheDie, dwarf::DW_FORM_sdata, Offset);
}

/// isTypeSigned - Return true if the type is signed.
static bool isTypeSigned(DwarfDebug* DD, DIType* Ty, int* SizeInBits)
{
    if (isa<DIDerivedType>(Ty))
    {
        return isTypeSigned(DD, DD->resolve(cast<DIDerivedType>(Ty)->getBaseType()), SizeInBits);
    }
    if (isa<DIBasicType>(Ty))
    {
        if (cast<DIBasicType>(Ty)->getEncoding() == dwarf::DW_ATE_signed ||
            cast<DIBasicType>(Ty)->getEncoding() == dwarf::DW_ATE_signed_char)
        {
            *SizeInBits = (int)cast<DIBasicType>(Ty)->getSizeInBits();
            return true;
        }
    }
    return false;
}

/// Return true if type encoding is unsigned.
bool isUnsignedDIType(DwarfDebug* DD, DIType* Ty)
{
    DIDerivedType* DTy = dyn_cast_or_null<DIDerivedType>(Ty);
    if (DTy)
    {
        return isUnsignedDIType(DD, DD->resolve(DTy->getBaseType()));
    }

    DIBasicType* BTy = dyn_cast_or_null<DIBasicType>(Ty);
    if (BTy)
    {
        unsigned Encoding = BTy->getEncoding();
        if (Encoding == dwarf::DW_ATE_unsigned ||
            Encoding == dwarf::DW_ATE_unsigned_char ||
            Encoding == dwarf::DW_ATE_boolean)
        {
            return true;
        }
    }
    return false;
}

/// If this type is derived from a base type then return base type size.
static uint64_t getBaseTypeSize(DwarfDebug* DD, DIDerivedType* Ty)
{
    unsigned Tag = Ty->getTag();

    if (Tag != dwarf::DW_TAG_member && Tag != dwarf::DW_TAG_typedef &&
        Tag != dwarf::DW_TAG_const_type && Tag != dwarf::DW_TAG_volatile_type &&
        Tag != dwarf::DW_TAG_restrict_type)
    {
        return Ty->getSizeInBits();
    }

    DIType* BaseType = DD->resolve(Ty->getBaseType());

    // If this type is not derived from any type then take conservative approach.
    if (isa<DIBasicType>(BaseType))
    {
        return Ty->getSizeInBits();
    }

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
        return getBaseTypeSize(DD, cast<DIDerivedType>(BaseType));
    }

    return BaseType->getSizeInBits();
}

/// addConstantFPValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantFPValue(DIE* Die, const ConstantFP* CFP)
{
    // Pass this down to addConstantValue as an unsigned bag of bits.
    addConstantValue(Die, CFP->getValueAPF().bitcastToAPInt(), true);
}

/// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE* Die, const ConstantInt* CI, bool Unsigned)
{
    addConstantValue(Die, CI->getValue(), Unsigned);
}

// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE* Die, const APInt& Val, bool Unsigned)
{
    unsigned CIBitWidth = Val.getBitWidth();
    if (CIBitWidth <= 64)
    {
        // If we're a signed constant definitely use sdata.
        if (!Unsigned)
        {
            addSInt(Die, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, Val.getSExtValue());
            return;
        }

        // Else use data for now unless it's larger than we can deal with.
        dwarf::Form Form;
        switch (CIBitWidth)
        {
        case 8:
            Form = dwarf::DW_FORM_data1;
            break;
        case 16:
            Form = dwarf::DW_FORM_data2;
            break;
        case 32:
            Form = dwarf::DW_FORM_data4;
            break;
        case 64:
            Form = dwarf::DW_FORM_data8;
            break;
        default:
            Form = dwarf::DW_FORM_udata;
            break;
        }
        addUInt(Die, dwarf::DW_AT_const_value, Form, Val.getZExtValue());
        return;
    }

    IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();

    // Get the raw data form of the large APInt.
    const uint64_t* Ptr64 = Val.getRawData();

    int NumBytes = Val.getBitWidth() / 8; // 8 bits per byte.
    bool LittleEndian = Asm->IsLittleEndian();

    // Output the constant to DWARF one byte at a time.
    for (int i = 0; i < NumBytes; i++)
    {
        uint8_t c;
        if (LittleEndian)
        {
            c = (uint8_t)(Ptr64[i / 8] >> (8 * (i & 7)));
        }
        else
        {
            c = (uint8_t)(Ptr64[(NumBytes - 1 - i) / 8] >> (8 * ((NumBytes - 1 - i) & 7)));
        }
        addUInt(Block, dwarf::DW_FORM_data1, c);
    }

    addBlock(Die, dwarf::DW_AT_const_value, Block);
}

/// addConstantData - Add constant data entry in variable DIE.
void CompileUnit::addConstantData(DIE* Die, const unsigned char* Ptr8, int NumBytes)
{
    IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();

    bool LittleEndian = Asm->IsLittleEndian();

    // Output the constant to DWARF one byte at a time.
    for (int i = 0; i < NumBytes; i++)
    {
        uint8_t c = (LittleEndian) ? Ptr8[i] : Ptr8[(NumBytes - 1 - i)];

        addUInt(Block, dwarf::DW_FORM_data1, c);
    }

    addBlock(Die, dwarf::DW_AT_const_value, Block);
}

/// addTemplateParams - Add template parameters into buffer.
void CompileUnit::addTemplateParams(DIE& Buffer, llvm::DINodeArray TParams)
{
    // Add template parameters.
    for (const auto* Element : TParams) {
        if (auto * TTP = dyn_cast<DITemplateTypeParameter>(Element))
            constructTemplateTypeParameterDIE(Buffer, const_cast<DITemplateTypeParameter*>(TTP));
        else if (auto * TVP = dyn_cast<DITemplateValueParameter>(Element))
            constructTemplateValueParameterDIE(Buffer, const_cast<DITemplateValueParameter*>(TVP));
    }
}

/// getOrCreateContextDIE - Get context owner's DIE.
IGC::DIE* CompileUnit::getOrCreateContextDIE(DIScope* Context)
{
    if (!Context || isa<DIFile>(Context))
        return getCUDie();
    if (auto * T = dyn_cast<DIType>(Context))
        return getOrCreateTypeDIE(T);
    if (auto * NS = dyn_cast<DINamespace>(Context))
        return getOrCreateNameSpace(NS);
    if (auto * SP = dyn_cast<DISubprogram>(Context))
        return getOrCreateSubprogramDIE(SP);

    IGC_ASSERT_MESSAGE(nullptr == dyn_cast<DIModule>(Context), "Missing implementation for DIModule!");

    return getDIE(Context);

}

/// getOrCreateTypeDIE - Find existing DIE or create new DIE for the
/// given DIType.
IGC::DIE* CompileUnit::getOrCreateTypeDIE(const MDNode* TyNode)
{
    if (!TyNode)
        return NULL;

    DIType* const Ty = cast_or_null<DIType>(const_cast<MDNode*>(TyNode));
    IGC_ASSERT(nullptr != Ty);

    // Construct the context before querying for the existence of the DIE in case
    // such construction creates the DIE.
    DIE* const ContextDIE = getOrCreateContextDIE(resolve(Ty->getScope()));
    IGC_ASSERT(nullptr != ContextDIE);

    DIE* TyDIE = getDIE(Ty);
    if (nullptr != TyDIE)
        return TyDIE;

    // Create new type.
    TyDIE = createAndAddDIE(Ty->getTag(), *ContextDIE, Ty);

    if (isa<DIBasicType>(Ty))
        constructTypeDIE(*TyDIE, cast<DIBasicType>(Ty));
    else if (isa<DICompositeType>(Ty))
        constructTypeDIE(*TyDIE, cast<DICompositeType>(Ty));
    else if (isa<DISubroutineType>(Ty))
        constructTypeDIE(*TyDIE, cast<DISubroutineType>(Ty));
    else
    {
        IGC_ASSERT_MESSAGE(isa<DIDerivedType>(Ty), "Unknown kind of DIType");
        constructTypeDIE(*TyDIE, cast<DIDerivedType>(Ty));
    }

    return TyDIE;
}

/// addType - Add a new type attribute to the specified entity.
void CompileUnit::addType(DIE* Entity, DIType* Ty, dwarf::Attribute Attribute)
{
    IGC_ASSERT_MESSAGE(nullptr != Ty, "Trying to add a type that doesn't exist?");

    // Check for pre-existence.
    DIEEntry* Entry = getDIEEntry(Ty);
    // If it exists then use the existing value.
    if (Entry)
    {
        addDIEEntry(Entity, Attribute, Entry);
        return;
    }

    // Construct type.
    DIE* Buffer = getOrCreateTypeDIE(Ty);

    // Set up proxy.
    Entry = createDIEEntry(Buffer);
    insertDIEEntry(Ty, Entry);
    addDIEEntry(Entity, Attribute, Entry);
}

// addSimdWidth - add SIMD width
void CompileUnit::addSimdWidth(DIE* Die, uint16_t SimdWidth)
{
    if (EmitSettings.EnableSIMDLaneDebugging)
    {
        // Emit SIMD width
        addUInt(Die, (dwarf::Attribute)DW_AT_INTEL_simd_width, dwarf::DW_FORM_data2,
            SimdWidth);
    }
}

// addGTRelativeLocation - add a sequence of attributes to calculate either:
// - BTI-relative location of variable in surface state, or
// - stateless surface location, or
// - bindless surface location or
// - bindless sampler location
void CompileUnit::addGTRelativeLocation(IGC::DIEBlock* Block, VISAVariableLocation* Loc)
{
    if (EmitSettings.EnableGTLocationDebugging && Loc->HasSurface())
    {
        uint32_t bti = Loc->GetSurface() - VISAModule::TEXTURE_REGISTER_BEGIN;

        IGC_ASSERT_MESSAGE(bti >= 0 && bti <= 255, "Surface BTI out of scope");

        if ((bti == STATELESS_BTI) || (bti == STATELESS_NONCOHERENT_BTI))  // 255, 253
        {
            // Stateless (coherent and non-coherent)
            addStatelessLocation(Block, Loc);
        }
        else if (bti == SLM_BTI) // 254
        {
            // SLM
            addSLMLocation(Block, Loc);
        }
        else if (bti == BINDLESS_BTI) // 252
        {
            // Bindless
            if (Loc->IsSampler())
            {
                // Bindless sampler
                addBindlessSamplerLocation(Block, Loc);
            }
            else
            {
                // Bindless surface
                addBindlessSurfaceLocation(Block, Loc);
            }
        }
        else if ((bti >= 0) && (bti <= 240))
        {
            // BTI surface

            // When BTI not in a register:
            // 1 DW_OP_breg5 (<bti> * 4)    , Note: breg5 stands for Binding Table Base Address
            // 2 DW_OP_deref_size 4
            // 3 DW_OP_const4u 0xffffffc0
            // 4 DW_OP_and
            // 5 DW_OP_breg8 32             , Note: breg8 stands for Surface State Base Address
            // 6 DW_OP_plus
            // 7 DW_OP_deref
            // 8 DW_OP_plus_uconst <offset>

            // When BTI in a register r0:
            // 1 DW_OP_reg16
            // 2 DW_OP_const1u <offset>
            // 3 DW_OP_const1u 32
            // 4 DW_OP_push_bit_piece_stack
            // 5 DW_OP_breg5 0              , Note: breg5 stands for Binding Table Base Address
            // 6 DW_OP_deref_size 4
            // 7 DW_OP_const4u 0xffffffc0
            // 8 DW_OP_and
            // 9 DW_OP_breg8 32             , Note: breg8 stands for Surface State Base Address
            // 10 DW_OP_plus
            // 11 DW_OP_deref
            // 12 DW_OP_plus_uconst <offset>

            IGC_ASSERT(EmitSettings.UseNewRegisterEncoding);
            IGC_ASSERT_MESSAGE(false == Loc->IsInGlobalAddrSpace(), "Missing surface for variable location");

            uint64_t btiBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::BTBase>(
                dwarf::DW_OP_breg0, EmitSettings.UseNewRegisterEncoding);
            uint64_t surfaceStateBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::SurfStateBase>(
                dwarf::DW_OP_breg0, EmitSettings.UseNewRegisterEncoding);
            uint32_t surfaceOffset = Loc->GetOffset();  // Surface offset

            addUInt(Block, dwarf::DW_FORM_data1, btiBaseAddrEncoded);  // Binding Table Base Address
            addSInt(Block, dwarf::DW_FORM_sdata, bti << 2);            // bti*4, offset for BT Base Address

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref_size);
            addUInt(Block, dwarf::DW_FORM_data1, 4);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const4u);
            addUInt(Block, dwarf::DW_FORM_data4, 0xffffffc0);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_and);

            addUInt(Block, dwarf::DW_FORM_data1, surfaceStateBaseAddrEncoded);  // Surface State Base Address
            addSInt(Block, dwarf::DW_FORM_sdata, 32);                           // Offset for Surface State Base Address

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
            addUInt(Block, dwarf::DW_FORM_udata, surfaceOffset);  // Offset
        }
        else
        {
            IGC_ASSERT_MESSAGE(false, "Unsupported BTI");
        }
    }
}

// addBindlessOrStatelessLocation - add a sequence of attributes to calculate stateless or
// bindless location of variable. baseAddr is one of the following base addreses:
// - General State Base Address when variable located in stateless surface
// - Bindless Surface State Base Address when variable located in bindless surface
// - Bindless Sampler State Base Addres when variable located in bindless sampler
// Note: Scratch space location is not handled here.
void CompileUnit::addBindlessOrStatelessLocation(IGC::DIEBlock* Block, VISAVariableLocation* Loc, uint32_t baseAddrEncoded)
{
    if (EmitSettings.EnableGTLocationDebugging)
    {
        IGC_ASSERT_MESSAGE(Loc->IsInGlobalAddrSpace(), "Neither bindless nor stateless");

        if (Loc->HasLocation())  // Is offset available as literal?
        {
            // Stateless (BTI 255 or 253) addressing using surface offset available as literal
            // 1 DW_OP_breg7 <offset>    , breg7 stands for General State Base Address
            // or
            // Bindless Surface addressing using surface offset available as literal
            // 1 DW_OP_breg9 <offset>   , breg9 stands for Bindless Surface State Base Address
            // or
            // Bindless Sampler addressing using surface offset available as literal
            // 1 DW_OP_breg10 <offset>   , breg10 stands for Bindless Sampler State Base Address
            uint32_t offset = Loc->GetOffset();

            addUInt(Block, dwarf::DW_FORM_data1, baseAddrEncoded);  // Base address of surface or sampler
            addSInt(Block, dwarf::DW_FORM_sdata, offset);           // Offset to base address
        }
        else if (Loc->IsRegister())
        {
            // Stateless surface or bindless surface or bindless sampler with offset not available as literal.
            // For example, if offset were available in register r0 and we assume the
            // DWARF number of r0 to be 16, the following expression can be generated
            // for stateless surface:
            // 1 DW_OP_breg7 0            , breg7 stands for General State Base Address
            // 2 DW_OP_breg16 0
            // 3 DW_OP_plus
            // or for bindless surface:
            // 1 DW_OP_breg9 0            , breg9 stands for Bindless Surface State Base Address
            // 2 DW_OP_breg16 0
            // 3 DW_OP_plus
            // or for bindless sampler:
            // 1 DW_OP_breg10 0           , breg10 stands for Bindless Sampler State Base Address
            // 2 DW_OP_breg16 0
            // 3 DW_OP_plus
            uint32_t regNum = Loc->GetRegister();

            addUInt(Block, dwarf::DW_FORM_data1, baseAddrEncoded);  // Base address of surface or sampler
            addSInt(Block, dwarf::DW_FORM_sdata, 0);                // No literal offset to base address

            addRegisterOffset(Block, regNum, 0);                    // Offset available in register

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unexpected bindless or stateless variable - offset neither literal nor in register");
        }
    }
}

// addStatelessLocation - add a sequence of attributes to calculate stateless surface location of variable
void CompileUnit::addStatelessLocation(IGC::DIEBlock* Block, VISAVariableLocation* Loc)
{
    if (EmitSettings.EnableGTLocationDebugging)
    {
        // Use virtual debug register with Stateless Surface State Base Address
        IGC_ASSERT(EmitSettings.UseNewRegisterEncoding);
        uint32_t statelessBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::GenStateBase>(
            dwarf::DW_OP_breg0, EmitSettings.UseNewRegisterEncoding);
        IGC_ASSERT_MESSAGE(Loc->HasSurface(), "Missing surface for variable location");

        addBindlessOrStatelessLocation(Block, Loc, statelessBaseAddrEncoded);
    }
}

// addBindlessSurfaceLocation - add a sequence of attributes to calculate bindless surface location of variable
void CompileUnit::addBindlessSurfaceLocation(IGC::DIEBlock* Block, VISAVariableLocation* Loc)
{
    if (EmitSettings.EnableGTLocationDebugging)
    {
        IGC_ASSERT(EmitSettings.UseNewRegisterEncoding);
        // Use virtual debug register with Bindless Surface State Base Address
        uint32_t bindlessSurfBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::BindlessSurfStateBase>(
            dwarf::DW_OP_breg0, EmitSettings.UseNewRegisterEncoding);

        IGC_ASSERT_MESSAGE(Loc->HasSurface(), "Missing surface for variable location");

        // Bindless Surface addressing using bindless offset stored in a register (for example) r0,
        // while offset is literal:
        // 1 DW_OP_reg16
        // 2 DW_OP_const1u <bit-offset to reg0>
        // 3 DW_OP_const1u 32
        // 4 DW_OP_push_bit_piece_stack
        // 5 DW_OP_breg9 32
        // 6 DW_OP_plus
        // 7 DW_OP_deref
        // 8 DW_OP_plus_uconst <offset>
        // or
        // Bindless Surface addressing using bindless offset and surface offset both stored in a register
        // (for example) r0 contains bindless offset while r1 contains surface offset.
        // 1 DW_OP_reg16
        // 2 DW_OP_const1u <bit-offset to reg0>
        // 3 DW_OP_const1u 32
        // 4 DW_OP_push_bit_piece_stack
        // 5 DW_OP_breg9 32
        // 6 DW_OP_plus
        // 7 DW_OP_deref
        // 8 DW_OP_reg17
        // 9 DW_OP_const1u <bit-offset to reg1>
        // 10 DW_OP_const1u 32
        // 11 DW_OP_push_bit_piece_stack

        uint16_t regNumWithBindlessOffset = 0;  // TBD Bindless offset in GRF
        uint16_t bitOffsetToBindlessReg = 0;    // TBD bit-offset to GRF with bindless offset

        addRegisterOp(Block, regNumWithBindlessOffset);  // Bindless offset to base address
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
        addUInt(Block, dwarf::DW_FORM_data1, bitOffsetToBindlessReg);
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
        addUInt(Block, dwarf::DW_FORM_data1, 32);
        addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_bit_piece_stack);
        addUInt(Block, dwarf::DW_FORM_data1, bindlessSurfBaseAddrEncoded);  // Bindless Surface Base Address
        addUInt(Block, dwarf::DW_FORM_udata, 32);
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);

        if (Loc->HasLocation())  // Is surface offset available as literal?
        {
            uint32_t offset = Loc->GetOffset();     // Surface offset
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
            addUInt(Block, dwarf::DW_FORM_udata, offset);
        }
        else if (Loc->IsRegister())  // Is surface offset available as register?
        {
            DbgDecoder::VarInfo varInfo;
            auto regNum = Loc->GetRegister();
            auto VISAMod = const_cast<VISAModule*>(Loc->GetVISAModule());
            VISAMod->getVarInfo("V", regNum, varInfo);
            uint16_t regNumWithSurfOffset = varInfo.lrs.front().getGRF().regNum;
            unsigned int subReg = varInfo.lrs.front().getGRF().subRegNum;
            auto bitOffsetToSurfReg = subReg * 8;  // Bit-offset to GRF with surface offset
            // auto sizeInBits = (VISAMod->m_pShader->getGRFSize() * 8) - offsetInBits;

            addRegisterOp(Block, regNumWithSurfOffset);  // Surface offset (in GRF) to base address
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
            addUInt(Block, dwarf::DW_FORM_data1, bitOffsetToSurfReg);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const1u);
            addUInt(Block, dwarf::DW_FORM_data1, 32);
            addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_bit_piece_stack);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unexpected bindless variable - offset neither literal nor in register");
        }
    }
}


// addBindlessSamplerLocation - add a sequence of attributes to calculate bindless sampler location of variable
void CompileUnit::addBindlessSamplerLocation(IGC::DIEBlock* Block, VISAVariableLocation* Loc)
{
    if (EmitSettings.EnableGTLocationDebugging)
    {
        IGC_ASSERT(EmitSettings.UseNewRegisterEncoding);
        // Use virtual debug register with Bindless Sampler State Base Address
        uint32_t bindlessSamplerBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::BindlessSamplerStateBase>(
            dwarf::DW_OP_breg0, EmitSettings.UseNewRegisterEncoding);

        IGC_ASSERT_MESSAGE(Loc->IsSampler(), "Missing sampler for variable location");

        addBindlessOrStatelessLocation(Block, Loc, bindlessSamplerBaseAddrEncoded);
    }
}

// addScratchLocation - add a sequence of attributes to emit scratch space location
// of variable
void CompileUnit::addScratchLocation(IGC::DIEBlock* Block, DbgDecoder::LiveIntervalsVISA* lr, int32_t vectorOffset)
{
    uint32_t offset = lr->getSpillOffset().memoryOffset + vectorOffset;

    IGC_ASSERT_MESSAGE(EmitSettings.EnableSIMDLaneDebugging, "SIMD lane expressions support only");

    if (EmitSettings.EnableGTLocationDebugging)
    {
        IGC_ASSERT(EmitSettings.UseNewRegisterEncoding);
        // For spills to the scratch area at offset available as literal
        // 1 DW_OP_breg6 <offset>    , breg6 stands for Scratch Space Base Address
        uint32_t scratchBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::ScratchBase>(
            dwarf::DW_OP_breg0, EmitSettings.UseNewRegisterEncoding);

        addUInt(Block, dwarf::DW_FORM_data1, scratchBaseAddrEncoded);  // Scratch Base Address
        addSInt(Block, dwarf::DW_FORM_sdata, offset);                  // Offset to base address
        // DW_OP_deref moved to the end of SIMD lane snippet
    }
    else
    {
        Address addr;
        addr.Set(Address::Space::eScratch, 0, offset);

        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
        addUInt(Block, dwarf::DW_FORM_data8, addr.GetAddress());
        // DW_OP_deref moved to the end of SIMD lane snippet
    }
}

// addSLMLocation - add a sequence of attributes to emit SLM location of variable
void CompileUnit::addSLMLocation(IGC::DIEBlock* Block, VISAVariableLocation* Loc)
{
    if (EmitSettings.EnableGTLocationDebugging)
    {
        IGC_ASSERT(EmitSettings.UseNewRegisterEncoding);
        IGC_ASSERT_MESSAGE(Loc->IsSLM(), "SLM expected as variable location");

        // For SLM addressing using address <slm - va> available as literal
        // 1 DW_OP_addr <slm - va>
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_addr);
        addUInt(Block, dwarf::DW_FORM_data4, Loc->GetOffset());
    }
}

// addSimdLane - add a sequence of attributes to calculate location of vectorized variable
// among SIMD lanes, e.g. a GRF subregister.
//
// CASE 1: Example of expression generated for 64-bit (or 32-bit) pointer to a variable,
// which is located in scratch: (note: DW_OP_const8u address is generated earlier)
// DW_OP_INTEL_push_simd_lane
// DW_OP_lit3 (or lit2 for 32-bit ptr)
// DW_OP_shl
// DW_OP_plus
// DW_OP_deref
//
// CASE 2: Example of expressions generated for 64-bit ptr addresses in SIMD8 or SIMD16:
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit2
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit3
// 8 DW_OP_and
// 9 DW_OP_const1u 64
// 10 DW_OP_mul
// 11 DW_OP_const1u 64
// 12 DW_OP_INTEL_push_bit_piece_stack
//
// CASE 3: Example of expressions generated for 32-bit ptr in SIMD8 or SIMD16
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit3
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit7
// 8 DW_OP_and
// 9 DW_OP_const1u 32
// 10 DW_OP_mul
// 11 DW_OP_const1u 32  (or 16 or 8)
// 12 DW_OP_INTEL_bit_piece_stack
//
// CASE 4: Example of expression generated for 64-bit or 32-bit or
// 16-bit packed or 8-bit packed variable in SIMD8 or SIMD16:
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit2 or lit3 or lit4 or lit5 respectively for 64/32/16/8 bit variable
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit3 or lit7 or lit15 or lit31 respectively for 64/32/16/8 bit variable
// 8 DW_OP_and
// 9 DW_OP_const1u 64 or 32 or 16 or 8
// 10 DW_OP_mul
// 11 DW_OP_const1u 64 or 32 or 16 or 8
// 12 DW_OP_INTEL_bit_piece_stack
//
// CASE 5: Example of expression generated for 16-bit or 8-bit variable unpacked
// in SIMD8 or SIMD16:
// 1 DW_OP_INTEL_push_simd_lane
//   DW_OP_lit16 <--
//   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
// 2 DW_OP_lit3
// 3 DW_OP_shr
// 4 DW_OP_plus_uconst(<n> +16)
// 5 DW_OP_INTEL_regs
// 6 DW_OP_INTEL_push_simd_lane
// 7 DW_OP_lit7
// 8 DW_OP_and
// 9 DW_OP_const1u 32
// 10 DW_OP_mul
// 11 DW_OP_const1u 16 or 8
// 12 DW_OP_INTEL_bit_piece_stack
//
void CompileUnit::addSimdLane(IGC::DIEBlock* Block, DbgVariable& DV, VISAVariableLocation *Loc,
    DbgDecoder::LiveIntervalsVISA * lr, uint16_t simdWidthOffset, bool isPacked, bool isSecondHalf)
{
    auto EmitPushSimdLane = [this](IGC::DIEBlock* Block, bool isSecondHalf)
    {
        addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_simd_lane);
        if (isSecondHalf)
        {
            // Fix offset to use for second half of SIMD32
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit16);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_minus);
        }
    };
    if (EmitSettings.EnableSIMDLaneDebugging && Loc->IsVectorized())
    {
        // SIMD lane
        auto *VISAMod = Loc->GetVISAModule();

        uint64_t varSizeInBits = Loc->IsInMemory() ? (uint64_t)Asm->GetPointerSize() * 8 : DV.getBasicSize(DD);
        IGC_ASSERT_MESSAGE(varSizeInBits % 8 == 0, "Variable's size not aligned to byte");

        if (lr->isSpill())
        {
            // CASE 1: Example of expression generated for 64-bit or 32-bit ptr to a variable,
            // which is located in scratch:
            // (note: DW_OP_const8u address is generated earlier)
            // DW_OP_INTEL_push_simd_lane
            // DW_OP_lit3 (or DW_OP_lit2 for 32-bit ptr)
            // DW_OP_shl
            // DW_OP_plus
            // DW_OP_deref

            EmitPushSimdLane(Block, isSecondHalf);
            // *8 if 64-bit ptr or *4 if 32-bit ptr.
            dwarf::LocationAtom litOP = (varSizeInBits == 64) ? dwarf::DW_OP_lit3 : dwarf::DW_OP_lit2;
            IGC_ASSERT_MESSAGE((varSizeInBits == 32) || (varSizeInBits == 64), "Unexpected ptr size");

            addUInt(Block, dwarf::DW_FORM_data1, litOP);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_shl);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
        }
        else
        {
            // This case handles the case where a source variable is held in
            // GRF or a ptr to it is held in GRF.

            // CASE 2 and CASE 3: Expressions generated for 64-bit (or 32-bit) bit ptr addresses in SIMD8 or SIMD16:
            // 1 DW_OP_INTEL_push_simd_lane
            //   DW_OP_lit16 <--
            //   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
            // 2 DW_OP_lit2 (CASE 3: lit3)
            // 3 DW_OP_shr
            // 4 DW_OP_plus_uconst(<n> +16)
            // 5 DW_OP_INTEL_regs
            // 6 DW_OP_INTEL_push_simd_lane
            // 7 DW_OP_lit3 (CASE 3: lit7)
            // 8 DW_OP_and
            // 9 DW_OP_const1u 64 (CASE 3: 32)
            // 10 DW_OP_mul
            // 11 DW_OP_const1u 64 (CASE 3: 32)
            // 12 DW_OP_INTEL_push_bit_piece_stack
            //
            // CASE 4: Example of expression generated for 64-bit or 32-bit or
            // 16-bit packed or 8-bit packed variable in SIMD8 or SIMD16:
            // 1 DW_OP_INTEL_push_simd_lane
            //   DW_OP_lit16 <--
            //   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
            // 2 DW_OP_lit2 or lit3 or lit4 or lit5 respectively for 64/32/16/8 bit variable
            // 3 DW_OP_shr
            // 4 DW_OP_plus_uconst(<n> +16)
            // 5 DW_OP_INTEL_regs
            // 6 DW_OP_INTEL_push_simd_lane
            // 7 DW_OP_lit3 or lit7 or lit15 or lit31 respectively for 64/32/16/8 bit variable
            // 8 DW_OP_and
            // 9 DW_OP_const1u 64 or 32 or 16 or 8
            // 10 DW_OP_mul
            // 11 DW_OP_const1u 64 or 32 or 16 or 8
            // 12 DW_OP_INTEL_bit_piece_stack
            //
            // CASE 5: Example of expression generated for 16-bit or 8-bit variable unpacked
            // in SIMD8 or SIMD16:
            // 1 DW_OP_INTEL_push_simd_lane
            //   DW_OP_lit16 <--
            //   DW_OP_minus <-- Emitted only for second half of SIMD32 kernels
            // 2 DW_OP_lit3
            // 3 DW_OP_shr
            // 4 DW_OP_plus_uconst(<n> +16)
            // 5 DW_OP_INTEL_regs
            // 6 DW_OP_INTEL_push_simd_lane
            // 7 DW_OP_lit7
            // 8 DW_OP_and
            // 9 DW_OP_const1u 32
            // 10 DW_OP_mul
            // 11 DW_OP_const1u 16 or 8
            // 12 DW_OP_INTEL_piece_stack

            // If unpacked then small variable takes up 32 bits else when packed fits its exact size
            uint32_t bitsUsedByVar = (isPacked || varSizeInBits > 32) ? (uint32_t)varSizeInBits : 32;
            uint32_t variablesInSingleGRF = (VISAMod->getGRFSize() * 8) / bitsUsedByVar;
            uint32_t valForSubRegLit = (uint32_t)log2(variablesInSingleGRF);

            // TODO: missing case lr->getGRF().subRegNum > 0
            // unsigned int subReg = lr->getGRF().subRegNum;
            // auto offsetInBits = subReg * 8;
            uint32_t regNumOffset = (variablesInSingleGRF == 0 || simdWidthOffset < variablesInSingleGRF) ?
                0 : (simdWidthOffset / variablesInSingleGRF);
            uint32_t regNum = lr->getGRF().regNum + regNumOffset;
            auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(
                regNum, EmitSettings.UseNewRegisterEncoding);

            // 1 var fits the whole GRF then set litForSubReg=dwarf::DW_OP_lit0 and litForSIMDlane=dwarf::DW_OP_lit0,
            // 2 vars   - dwarf::DW_OP_lit1 and dwarf::DW_OP_lit1,
            // 4 vars   - dwarf::DW_OP_lit2 and dwarf::DW_OP_lit3,
            // 8 vars   - dwarf::DW_OP_lit3 and dwarf::DW_OP_lit7,
            // 16 vars  - dwarf::DW_OP_lit4 and dwarf::DW_OP_lit15,
            // 32 vars  - dwarf::DW_OP_lit5 and dwarf::DW_OP_lit31,
            // 64 vars  - dwarf::DW_OP_lit6 and dwarf::DW_OP_const1u 63,
            // 128 vars - dwarf::DW_OP_lit7 and dwarf::DW_OP_const1u 127,
            // 256 vars - dwarf::DW_OP_lit8 and dwarf::DW_OP_const1u 255,
            // 512 vars - dwarf::DW_OP_lit9 and dwarf::DW_OP_const2u 511, etc.
            dwarf::LocationAtom litForSubReg = (dwarf::LocationAtom)(dwarf::DW_OP_lit0 + valForSubRegLit);
            dwarf::LocationAtom litForSIMDlane = dwarf::DW_OP_const2u;
            if (variablesInSingleGRF <= 32)
            {
                litForSIMDlane = (dwarf::LocationAtom)(dwarf::DW_OP_lit0 + variablesInSingleGRF - 1);
            }
            else if (variablesInSingleGRF <= 256)
            {
                litForSIMDlane = dwarf::DW_OP_const1u;
            }

            EmitPushSimdLane(Block, isSecondHalf);

            addUInt(Block, dwarf::DW_FORM_data1, litForSubReg);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_shr);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
            addUInt(Block, dwarf::DW_FORM_udata, DWRegEncoded);  // Register ID is shifted by offset
            addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_regs);
            EmitPushSimdLane(Block, false);
            addUInt(Block, dwarf::DW_FORM_data1, litForSIMDlane);
            if (variablesInSingleGRF > 256)
            {
                addUInt(Block, dwarf::DW_FORM_data2, variablesInSingleGRF - 1);
            }
            else if (variablesInSingleGRF > 32)
            {
                addUInt(Block, dwarf::DW_FORM_data1, variablesInSingleGRF - 1);
            }

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_and);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
            addUInt(Block, dwarf::DW_FORM_data2, bitsUsedByVar);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_mul);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
            addUInt(Block, dwarf::DW_FORM_data2, varSizeInBits);

            if (isa<llvm::DbgDeclareInst>(DV.getDbgInst()))
            {
                // Pointer
                addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_bit_piece_stack);
            }
            else
            {
                // Variable
                addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_bit_piece_stack);
            }
        }
    }
}

// addSimdLaneScalar - add a sequence of attributes to calculate location of scalar variable
// e.g. a GRF subregister.
void CompileUnit::addSimdLaneScalar(IGC::DIEBlock* Block, DbgVariable& DV, VISAVariableLocation* Loc, DbgDecoder::LiveIntervalsVISA* lr, uint16_t subRegInBytes)
{
    if (EmitSettings.EnableSIMDLaneDebugging)
    {
        IGC_ASSERT_MESSAGE(lr->isSpill() == false, "Scalar spilled in scratch space");

        // addRegisterOffset(Block, varInfo.lrs.front().getGRF().regNum, 0);
        uint64_t varSizeInBits = Loc->IsInMemory() ? (uint64_t)Asm->GetPointerSize() * 8 : DV.getBasicSize(DD);

        auto offsetInBits = subRegInBytes * 8;

        if (isa<llvm::DbgDeclareInst>(DV.getDbgInst()))
        {
            // Pointer
            addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_bit_piece_stack);
        }
        else
        {
            // Variable
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
        }
        addUInt(Block, dwarf::DW_FORM_udata, varSizeInBits);
        addUInt(Block, dwarf::DW_FORM_udata, offsetInBits);
    }
}

/// getParentContextString - Walks the metadata parent chain in a language
/// specific manner (using the compile unit language) and returns
/// it as a string. This is done at the metadata level because DIEs may
/// not currently have been added to the parent context and walking the
/// DIEs looking for names is more expensive than walking the metadata.
std::string CompileUnit::getParentContextString(DIScope* Context) const
{
    if (!Context)
        return "";

    // FIXME: Decide whether to implement this for non-C++ languages.
    if (getLanguage() != dwarf::DW_LANG_C_plus_plus)
        return "";

    std::string CS;

    SmallVector<DIScope*, 1> Parents;
    while (!isa<DICompileUnit>(Context))
    {
        Parents.push_back(Context);
        if (Context->getScope())
            Context = resolve(Context->getScope());
        else
            // Structure, etc types will have a NULL context if they're at the top
            // level.
            break;
    }

    // Reverse iterate over our list to go from the outermost construct to the
    // innermost.
    for (SmallVectorImpl<DIScope*>::reverse_iterator I = Parents.rbegin(),
        E = Parents.rend();
        I != E; ++I)
    {
        DIScope* Ctx = *I;
        StringRef Name = Ctx->getName();
        if (!Name.empty())
        {
            CS += Name;
            CS += "::";
        }
    }

    return CS;
}

/// constructTypeDIE - Construct basic type die from DIBasicType.
void CompileUnit::constructTypeDIE(DIE& Buffer, DIBasicType* BTy)
{
    // Get core information.
    StringRef Name = BTy->getName();
    // Add name if not anonymous or intermediate type.
    if (!Name.empty())
    {
        addString(&Buffer, dwarf::DW_AT_name, Name);
    }

    // An unspecified type only has a name attribute.
    if (BTy->getTag() == dwarf::DW_TAG_unspecified_type)
        return;

    addUInt(&Buffer, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, BTy->getEncoding());

    uint64_t Size = BTy->getSizeInBits() >> 3;
    addUInt(&Buffer, dwarf::DW_AT_byte_size, None, Size);
}

/// constructTypeDIE - Construct derived type die from DIDerivedType.
void CompileUnit::constructTypeDIE(DIE& Buffer, DIDerivedType* DTy)
{
    // Get core information.
    StringRef Name = DTy->getName();
    uint64_t Size = DTy->getSizeInBits() >> 3;
    uint16_t Tag = Buffer.getTag();

    // Map to main type, void will not have a type.
    DIType* FromTy = resolve(DTy->getBaseType());
    if (FromTy)
        addType(&Buffer, FromTy);

    // Add name if not anonymous or intermediate type.
    if (!Name.empty())
        addString(&Buffer, dwarf::DW_AT_name, Name);

    // Add size if non-zero (derived types might be zero-sized.)
    if (Size && Tag != dwarf::DW_TAG_pointer_type)
        addUInt(&Buffer, dwarf::DW_AT_byte_size, None, Size);

    if (Tag == dwarf::DW_TAG_ptr_to_member_type)
        addDIEEntry(&Buffer, dwarf::DW_AT_containing_type, getOrCreateTypeDIE(resolve(DTy->getClassType())));
    // Add source line info if available and TyDesc is not a forward declaration.
    if (!DTy->isForwardDecl())
        addSourceLine(&Buffer, DTy);
}

/// Return true if the type is appropriately scoped to be contained inside
/// its own type unit.
static bool isTypeUnitScoped(DIType* Ty, const DwarfDebug* DD)
{
    DIScope* Parent = DD->resolve(Ty->getScope());
    while (Parent)
    {
        // Don't generate a hash for anything scoped inside a function.
        if (isa<DISubprogram>(Parent))
            return false;

        Parent = DD->resolve(Parent->getScope());
    }
    return true;
}

/// Return true if the type should be split out into a type unit.
static bool shouldCreateTypeUnit(DICompositeType* CTy, const DwarfDebug* DD)
{
    uint16_t Tag = (uint16_t)CTy->getTag();

    switch (Tag)
    {
    case dwarf::DW_TAG_structure_type:
    case dwarf::DW_TAG_union_type:
    case dwarf::DW_TAG_enumeration_type:
    case dwarf::DW_TAG_class_type:
        // If this is a class, structure, union, or enumeration type
        // that is a definition (not a declaration), and not scoped
        // inside a function then separate this out as a type unit.
        return !CTy->isForwardDecl() && isTypeUnitScoped(CTy, DD);
    default:
        return false;
    }
}

void CompileUnit::constructTypeDIE(DIE& Buffer, DISubroutineType* STy)
{
    DITypeRefArray Elements = cast<DISubroutineType>(STy)->getTypeArray();
    DIType* RTy = resolve(Elements[0]);
    if (RTy)
        addType(&Buffer, RTy);

    bool isPrototyped = true;

    if (Elements.size() == 2 &&
        !Elements[1])
        isPrototyped = false;

    // Add arguments.
    for (unsigned i = 1, N = Elements.size(); i < N; ++i)
    {
        DIType* Ty = resolve(Elements[i]);
        if (!Ty)
        {
            createAndAddDIE(dwarf::DW_TAG_unspecified_parameters, Buffer);
            isPrototyped = false;
        }
        else
        {
            DIE* Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, Buffer);
            addType(Arg, Ty);
            if (Ty->isArtificial())
                addFlag(Arg, dwarf::DW_AT_artificial);
        }
    }
    // Add prototype flag if we're dealing with a C language and the
    // function has been prototyped.
    uint16_t Language = getLanguage();
    if (isPrototyped &&
        (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 ||
            Language == dwarf::DW_LANG_ObjC))
        addFlag(&Buffer, dwarf::DW_AT_prototyped);
}

/// constructTypeDIE - Construct type DIE from DICompositeType.
void CompileUnit::constructTypeDIE(DIE& Buffer, DICompositeType* CTy)
{
    // Get core information.
    StringRef Name = CTy->getName();

    uint64_t Size = CTy->getSizeInBits() >> 3;
    uint16_t Tag = Buffer.getTag();

    switch (Tag)
    {
    case dwarf::DW_TAG_array_type:
        constructArrayTypeDIE(Buffer, CTy);
        break;
    case dwarf::DW_TAG_enumeration_type:
        constructEnumTypeDIE(Buffer, CTy);
        break;
    case dwarf::DW_TAG_subroutine_type:
    {
        // Add return type. A void return won't have a type.
        DITypeRefArray Elements = cast<DISubroutineType>(CTy)->getTypeArray();
        DIType* RTy = resolve(Elements[0]);
        if (RTy)
            addType(&Buffer, RTy);

        bool isPrototyped = true;

        if (Elements.size() == 2 &&
            !Elements[1])
            isPrototyped = false;

        // Add arguments.
        for (unsigned i = 1, N = Elements.size(); i < N; ++i)
        {
            DIType* Ty = resolve(Elements[i]);
            if (!Ty)
            {
                createAndAddDIE(dwarf::DW_TAG_unspecified_parameters, Buffer);
                isPrototyped = false;
            }
            else
            {
                DIE* Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, Buffer);
                addType(Arg, Ty);
                if (Ty->isArtificial())
                    addFlag(Arg, dwarf::DW_AT_artificial);
            }
        }
        // Add prototype flag if we're dealing with a C language and the
        // function has been prototyped.
        uint16_t Language = getLanguage();
        if (isPrototyped &&
            (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 ||
                Language == dwarf::DW_LANG_ObjC))
            addFlag(&Buffer, dwarf::DW_AT_prototyped);
    } break;
    case dwarf::DW_TAG_structure_type:
    case dwarf::DW_TAG_union_type:
    case dwarf::DW_TAG_class_type:
    {
        // Add elements to structure type.
        DINodeArray Elements = CTy->getElements();
        for (unsigned i = 0, N = Elements.size(); i < N; ++i)
        {
            DINode* Element = Elements[i];
            DIE* ElemDie = NULL;
            if (isa<DISubprogram>(Element))
            {
                DISubprogram* SP = cast<DISubprogram>(Element);
                ElemDie = getOrCreateSubprogramDIE(SP);

                dwarf::AccessAttribute dw_access = (SP->isProtected()) ? dwarf::DW_ACCESS_protected :
                    (SP->isPrivate()) ? dwarf::DW_ACCESS_private : dwarf::DW_ACCESS_public;
                addUInt(ElemDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1, dw_access);

                if (SP->isExplicit())
                {
                    addFlag(ElemDie, dwarf::DW_AT_explicit);
                }
            }
            else if (isa<DIDerivedType>(Element))
            {
                DIDerivedType* DDTy = cast<DIDerivedType>(Element);
                if (DDTy->getTag() == dwarf::DW_TAG_friend)
                {
                    ElemDie = createAndAddDIE(dwarf::DW_TAG_friend, Buffer);
                    addType(ElemDie, resolve(DDTy->getBaseType()),
                        dwarf::DW_AT_friend);
                }
                else if (DDTy->isStaticMember())
                {
                    getOrCreateStaticMemberDIE(DDTy);
                }
                else
                {
                    constructMemberDIE(Buffer, DDTy);
                }
            }
            else
                continue;
        }

        DIType* ContainingType = resolve(CTy->getBaseType());
        if (ContainingType &&
            isa<DICompositeType>(ContainingType))
            addDIEEntry(&Buffer, dwarf::DW_AT_containing_type,
                getOrCreateTypeDIE(ContainingType));

        // Add template parameters to a class, structure or union types.
        // FIXME: The support isn't in the metadata for this yet.
        if (Tag == dwarf::DW_TAG_class_type ||
            Tag == dwarf::DW_TAG_structure_type ||
            Tag == dwarf::DW_TAG_union_type)
        {
            addTemplateParams(Buffer, CTy->getTemplateParams());
        }

        break;
    }
    default:
        break;
    }

    // Add name if not anonymous or intermediate type.
    if (!Name.empty())
        addString(&Buffer, dwarf::DW_AT_name, Name);

    if (Tag == dwarf::DW_TAG_enumeration_type ||
        Tag == dwarf::DW_TAG_class_type ||
        Tag == dwarf::DW_TAG_structure_type ||
        Tag == dwarf::DW_TAG_union_type)
    {
        // Add size if non-zero (derived types might be zero-sized.)
        // TODO: Do we care about size for enum forward declarations?
        if (Size)
        {
            addUInt(&Buffer, dwarf::DW_AT_byte_size, None, Size);
        }
        else if (!CTy->isForwardDecl())
        {
            // Add zero size if it is not a forward declaration.
            addUInt(&Buffer, dwarf::DW_AT_byte_size, None, 0);
        }

        // If we're a forward decl, say so.
        if (CTy->isForwardDecl())
        {
            addFlag(&Buffer, dwarf::DW_AT_declaration);
        }

        // Add source line info if available.
        if (!CTy->isForwardDecl())
        {
            addSourceLine(&Buffer, CTy);
        }
    }
    // If this is a type applicable to a type unit it then add it to the
    // list of types we'll compute a hash for later.
    if (shouldCreateTypeUnit(CTy, DD))
    {
        DD->addTypeUnitType(&Buffer);
    }
}

/// constructTemplateTypeParameterDIE - Construct new DIE for the given
/// DITemplateTypeParameter.
void
CompileUnit::constructTemplateTypeParameterDIE(DIE& Buffer, DITemplateTypeParameter* TP)
{
    DIE* ParamDIE = createAndAddDIE(dwarf::DW_TAG_template_type_parameter, Buffer);
    // Add the type if it exists, it could be void and therefore no type.
    if (TP->getType())
    {
        addType(ParamDIE, resolve(TP->getType()));
    }
    if (!TP->getName().empty())
    {
        addString(ParamDIE, dwarf::DW_AT_name, TP->getName());
    }
}

/// constructTemplateValueParameterDIE - Construct new DIE for the given
/// DITemplateValueParameter.
void CompileUnit::constructTemplateValueParameterDIE(
    DIE& Buffer, DITemplateValueParameter* VP)
{
    DIE* ParamDIE = createAndAddDIE(VP->getTag(), Buffer);

    // Add the type if there is one, template template and template parameter
    // packs will not have a type.
    if (VP->getTag() == dwarf::DW_TAG_template_value_parameter)
    {
        addType(ParamDIE, resolve(VP->getType()));
    }
    if (!VP->getName().empty())
    {
        addString(ParamDIE, dwarf::DW_AT_name, VP->getName());
    }

    if (Metadata * Val = VP->getValue())
    {
        if (ConstantInt * CI = mdconst::dyn_extract<ConstantInt>(Val))
        {
            addConstantValue(ParamDIE, CI, isUnsignedDIType(DD, resolve(VP->getType())));
        }
        else if (GlobalValue * GV = mdconst::dyn_extract<GlobalValue>(Val))
        {
            // For declaration non-type template parameters (such as global values and
            // functions)
            IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();
            addOpAddress(Block, Asm->GetSymbol(GV));
            // Emit DW_OP_stack_value to use the address as the immediate value of the
            // parameter, rather than a pointer to it.
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);
            addBlock(ParamDIE, dwarf::DW_AT_location, Block);
        }
        else if (VP->getTag() == dwarf::DW_TAG_GNU_template_template_param)
        {
            IGC_ASSERT(isa<MDString>(Val));
            addString(ParamDIE, dwarf::DW_AT_GNU_template_name, cast<MDString>(Val)->getString());
        }
        else if (VP->getTag() == dwarf::DW_TAG_GNU_template_parameter_pack)
        {
            IGC_ASSERT(isa<MDNode>(Val));
            //DIArray A(cast<MDNode>(Val));
            //addTemplateParams(*ParamDIE, A);
        }
    }
}


/// getOrCreateNameSpace - Create a DIE for DINameSpace.
IGC::DIE* CompileUnit::getOrCreateNameSpace(DINamespace* NS)
{
    // Construct the context before querying for the existence of the DIE in case
    // such construction creates the DIE.
    DIE* ContextDIE = getOrCreateContextDIE(NS->getScope());

    DIE* NDie = getDIE(NS);
    if (NDie)
        return NDie;

    NDie = createAndAddDIE(dwarf::DW_TAG_namespace, *ContextDIE, NS);

    if (!NS->getName().empty())
    {
        addString(NDie, dwarf::DW_AT_name, NS->getName());
    }
#if LLVM_VERSION_MAJOR == 4
    addSourceLine(NDie, NS);
#endif
    return NDie;
}

/// getOrCreateSubprogramDIE - Create new DIE using SP.
IGC::DIE* CompileUnit::getOrCreateSubprogramDIE(DISubprogram* SP)
{
    // Construct the context before querying for the existence of the DIE in case
    // such construction creates the DIE (as is the case for member function
    // declarations).
    DIE* ContextDIE = getOrCreateContextDIE(resolve(SP->getScope()));

    DIE* SPDie = getDIE(SP);
    if (SPDie)
        return SPDie;

    DISubprogram* SPDecl = SP->getDeclaration();
    if (SPDecl &&
        isa<DISubprogram>(SPDecl))
    {
        // Add subprogram definitions to the CU die directly.
        ContextDIE = CUDie;
    }

    // DW_TAG_inlined_subroutine may refer to this DIE.
    SPDie = createAndAddDIE(dwarf::DW_TAG_subprogram, *ContextDIE, SP);

    DIE* DeclDie = NULL;
    if (SPDecl &&
        isa<DISubprogram>(SPDecl))
    {
        DeclDie = getOrCreateSubprogramDIE(SPDecl);
    }

    // Add function template parameters.
    addTemplateParams(*SPDie, SP->getTemplateParams());

    // If this DIE is going to refer declaration info using AT_specification
    // then there is no need to add other attributes.
    if (DeclDie)
    {
        // Refer function declaration directly.
        addDIEEntry(SPDie, dwarf::DW_AT_specification, DeclDie);

        return SPDie;
    }

    // Add the linkage name if we have one.
    StringRef LinkageName = SP->getLinkageName();
    if (!LinkageName.empty())
    {
        addString(SPDie, dwarf::DW_AT_MIPS_linkage_name, IGCLLVM::GlobalValue::getRealLinkageName(LinkageName));
    }

    // Constructors and operators for anonymous aggregates do not have names.
    if (!SP->getName().empty())
    {
        addString(SPDie, dwarf::DW_AT_name, SP->getName());
    }

    addSourceLine(SPDie, SP);

    addSimdWidth(SPDie, DD->simdWidth);

    // Add the prototype if we have a prototype and we have a C like
    // language.
    uint16_t Language = getLanguage();
    if (SP->isPrototyped() &&
        (Language == dwarf::DW_LANG_C89 ||
            Language == dwarf::DW_LANG_C99 ||
            Language == dwarf::DW_LANG_ObjC))
    {
        addFlag(SPDie, dwarf::DW_AT_prototyped);
    }

    // Following sourced from DwarfUnit.cpp from llvm3.6.2 src
    DISubroutineType* SPTy = SP->getType();

    if (!SPTy)
    {
        // KW fix, this branch should never be taken
        return SPDie;
    }

    IGC_ASSERT_MESSAGE(SPTy->getTag() == dwarf::DW_TAG_subroutine_type, "the type of a subprogram should be a subroutine");

    DITypeRefArray Args = SPTy->getTypeArray();
    // Add a return type. If this is a type like a C/C++ void type we don't add a
    // return type.
    if (Args.size() > 0 && resolve(Args[0]))
        addType(SPDie, resolve(Args[0]));

    unsigned VK = SP->getVirtuality();
    if (VK)
    {
        addUInt(SPDie, dwarf::DW_AT_virtuality, dwarf::DW_FORM_data1, VK);
        IGC::DIEBlock* Block = getDIEBlock();
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
        addUInt(Block, dwarf::DW_FORM_udata, SP->getVirtualIndex());
        addBlock(SPDie, dwarf::DW_AT_vtable_elem_location, Block);
        ContainingTypeMap.insert(std::make_pair(SPDie, resolve(SP->getContainingType())));
    }

    if (!SP->isDefinition())
    {
        addFlag(SPDie, dwarf::DW_AT_declaration);

        // Add arguments. Do not add arguments for subprogram definition. They will
        // be handled while processing variables.
        for (unsigned i = 1, N = Args.size(); i < N; ++i)
        {
            DIE* Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, *SPDie);
            DIType* ATy = resolve(Args[i]);
            addType(Arg, ATy);
            if (ATy->isArtificial())
            {
                addFlag(Arg, dwarf::DW_AT_artificial);
            }
        }
    }

    if (SP->isArtificial())
    {
        addFlag(SPDie, dwarf::DW_AT_artificial);
    }

    if (!SP->isLocalToUnit())
    {
        addFlag(SPDie, dwarf::DW_AT_external);
    }

    return SPDie;
}

/// constructSubrangeDIE - Construct subrange DIE from DISubrange.
void CompileUnit::constructSubrangeDIE(DIE& Buffer, DISubrange* SR, DIE* IndexTy)
{
    DIE* DW_Subrange = createAndAddDIE(dwarf::DW_TAG_subrange_type, Buffer);
    addDIEEntry(DW_Subrange, dwarf::DW_AT_type, IndexTy);

    // The LowerBound value defines the lower bounds which is typically zero for
    // C/C++. The Count value is the number of elements.  Values are 64 bit. If
    // Count == -1 then the array is unbounded and we do not emit
    // DW_AT_lower_bound and DW_AT_upper_bound attributes. If LowerBound == 0 and
    // Count == 0, then the array has zero elements in which case we do not emit
    // an upper bound.
#if LLVM_VERSION_MAJOR <= 10
    int64_t LowerBound = SR->getLowerBound();
#endif
    int64_t DefaultLowerBound = getDefaultLowerBound();
    int64_t Count = SR->getCount()
#if LLVM_VERSION_MAJOR >= 7
        .dyn_cast<ConstantInt*>()->getSExtValue()
#endif
        ;

#if LLVM_VERSION_MAJOR >= 11
    auto addBoundTypeEntry = [&](dwarf::Attribute Attr,
        DISubrange::BoundType Bound) -> void {
        if (auto * BV = Bound.dyn_cast<DIVariable*>()) {
            if (auto * VarDIE = getDIE(BV))
                addDIEEntry(DW_Subrange, Attr, VarDIE);
        }
/*
LLVM11-UPGRADE : TODO
        else if (auto * BE = Bound.dyn_cast<DIExpression*>()) {
            DIELoc* Loc = new (DIEValueAllocator) DIELoc;
            DIEDwarfExpression DwarfExpr(*Asm, getCU(), *Loc);
            DwarfExpr.setMemoryLocationKind();
            DwarfExpr.addExpression(BE);
            addBlock(DW_Subrange, Attr, DwarfExpr.finalize());
        }
*/
        else if (auto * BI = Bound.dyn_cast<ConstantInt*>()) {
            if (Attr != dwarf::DW_AT_lower_bound || DefaultLowerBound == -1 ||
                BI->getSExtValue() != DefaultLowerBound)
                addSInt(DW_Subrange, Attr, dwarf::DW_FORM_sdata, BI->getSExtValue());
        }
    };

    addBoundTypeEntry(dwarf::DW_AT_lower_bound, SR->getLowerBound());

    if (auto * CV = SR->getCount().dyn_cast<DIVariable*>()) {
        if (auto * CountVarDIE = getDIE(CV))
            addDIEEntry(DW_Subrange, dwarf::DW_AT_count, CountVarDIE);
    }
    else if (Count != -1)
        addUInt(DW_Subrange, dwarf::DW_AT_count, None, Count);

    addBoundTypeEntry(dwarf::DW_AT_upper_bound, SR->getUpperBound());

    addBoundTypeEntry(dwarf::DW_AT_byte_stride, SR->getStride());
#else
    if (DefaultLowerBound == -1 || LowerBound != DefaultLowerBound)
    {
        addUInt(DW_Subrange, dwarf::DW_AT_lower_bound, None, LowerBound);
    }

    if (Count != -1 && Count != 0)
    {
        // FIXME: An unbounded array should reference the expression that defines
        // the array.
        addUInt(DW_Subrange, dwarf::DW_AT_upper_bound, None, LowerBound + Count - 1);
    }
#endif
}

/// constructArrayTypeDIE - Construct array type DIE from DICompositeType.
void CompileUnit::constructArrayTypeDIE(DIE& Buffer, DICompositeType* CTy)
{
    if (CTy->isVector())
    {
        addFlag(&Buffer, dwarf::DW_AT_GNU_vector);
    }

    // Emit the element type.
    addType(&Buffer, resolve(CTy->getBaseType()));

    // Get an anonymous type for index type.
    // FIXME: This type should be passed down from the front end
    // as different languages may have different sizes for indexes.
    DIE* IdxTy = getIndexTyDie();
    if (!IdxTy)
    {
        // Construct an anonymous type for index type.
        IdxTy = createAndAddDIE(dwarf::DW_TAG_base_type, *CUDie);
        addString(IdxTy, dwarf::DW_AT_name, "int");
        addUInt(IdxTy, dwarf::DW_AT_byte_size, None, sizeof(int32_t));
        addUInt(IdxTy, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed);
        setIndexTyDie(IdxTy);
    }

    // Add subranges to array type.
    DINodeArray Elements = CTy->getElements();
    for (unsigned i = 0, N = Elements.size(); i < N; ++i)
    {
        auto Element = Elements[i];
        if (Element->getTag() == dwarf::DW_TAG_subrange_type)
        {
            constructSubrangeDIE(Buffer, cast<DISubrange>(Element), IdxTy);
        }
    }
}

/// constructEnumTypeDIE - Construct an enum type DIE from DICompositeType.
void CompileUnit::constructEnumTypeDIE(DIE& Buffer, DICompositeType* CTy)
{
    DINodeArray Elements = CTy->getElements();

    // Add enumerators to enumeration type.
    for (unsigned i = 0, N = Elements.size(); i < N; ++i)
    {
        DIEnumerator* Enum = cast_or_null<DIEnumerator>(Elements[i]);
        if (Enum)
        {
            DIE* Enumerator = createAndAddDIE(dwarf::DW_TAG_enumerator, Buffer);
            StringRef Name = Enum->getName();
            addString(Enumerator, dwarf::DW_AT_name, Name);
            int64_t Value = Enum->getValue()
#if LLVM_VERSION_MAJOR >= 11
                .getZExtValue()
#endif
                ;
            addSInt(Enumerator, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, Value);
        }
    }
    DIType* DTy = resolve(CTy->getBaseType());
    if (DTy)
    {
        addType(&Buffer, DTy);
        addFlag(&Buffer, dwarf::DW_AT_enum_class);
    }
}

/// constructContainingTypeDIEs - Construct DIEs for types that contain
/// vtables.
void CompileUnit::constructContainingTypeDIEs()
{
    for (DenseMap<DIE*, const MDNode*>::iterator CI = ContainingTypeMap.begin(),
        CE = ContainingTypeMap.end(); CI != CE; ++CI)
    {
        DIE* SPDie = CI->first;
        DINode* D = cast<DINode>(const_cast<MDNode*>(CI->second));
        if (!D)
            continue;
        DIE* NDie = getDIE(D);
        if (!NDie)
            continue;
        addDIEEntry(SPDie, dwarf::DW_AT_containing_type, NDie);
    }
}

/// constructVariableDIE - Construct a DIE for the given DbgVariable.
IGC::DIE* CompileUnit::constructVariableDIE(DbgVariable& DV, bool isScopeAbstract)
{
    StringRef Name = DV.getName();

    // Define variable debug information entry.
    DIE* VariableDie = new DIE(DV.getTag());
    DbgVariable* AbsVar = DV.getAbstractVariable();
    DIE* AbsDIE = AbsVar ? AbsVar->getDIE() : NULL;
    if (AbsDIE)
    {
        addDIEEntry(VariableDie, dwarf::DW_AT_abstract_origin, AbsDIE);
    }
    else
    {
        if (!Name.empty())
        {
            addString(VariableDie, dwarf::DW_AT_name, Name);
        }
        addSourceLine(VariableDie, const_cast<DILocalVariable*>(DV.getVariable()));
        addType(VariableDie, DV.getType());
    }

    if (DV.isArtificial())
    {
        addFlag(VariableDie, dwarf::DW_AT_artificial);
    }

    if (isScopeAbstract)
    {
        DV.setDIE(VariableDie);
        return VariableDie;
    }

    // Add variable address.

    unsigned Offset = DV.getDotDebugLocOffset();
    if (Offset != ~0U)
    {
        if (DD->IsDirectElfInput())
        {
            // Copy over references ranges to DotLocDebugEntries
            Offset = DD->CopyDebugLoc(Offset);
            addUInt(VariableDie, dwarf::DW_AT_location, dwarf::DW_FORM_sec_offset, Offset);
            if (DV.getDecorations().size() > 0)
            {
                addString(VariableDie, dwarf::DW_AT_description, DV.getDecorations());
            }
        }
        else
        {
            addLabel(VariableDie, dwarf::DW_AT_location,
                DD->getDwarfVersion() >= 4 ? dwarf::DW_FORM_sec_offset : dwarf::DW_FORM_data4,
                Asm->GetTempSymbol("debug_loc", Offset));
        }
        DV.setDIE(VariableDie);
        return VariableDie;
    }

    // Check if variable is described by a DBG_VALUE instruction.
    const Instruction* pDbgInst = DV.getDbgInst();
    if (!pDbgInst)
    {
        DV.setDIE(VariableDie);
        return VariableDie;
    }

    buildLocation(pDbgInst, DV, VariableDie);

    return VariableDie;
}

void CompileUnit::buildLocation(const llvm::Instruction* pDbgInst, DbgVariable& DV, IGC::DIE* VariableDie)
{
    auto F = pDbgInst->getParent()->getParent();
    auto VISAModule = DD->GetVISAModule(F);
    auto Locs = VISAModule->GetVariableLocation(pDbgInst);
    auto& FirstLoc = Locs.front();

    // Variable can be immdeiate or in a location (but not both)
    if (FirstLoc.IsImmediate())
    {
        const Constant* pConstVal = FirstLoc.GetImmediate();
        if (const ConstantInt * pConstInt = dyn_cast<ConstantInt>(pConstVal))
        {
            addConstantValue(VariableDie, pConstInt, isUnsignedDIType(DD, DV.getType()));
        }
        else if (const ConstantFP * pConstFP = dyn_cast<ConstantFP>(pConstVal))
        {
            addConstantFPValue(VariableDie, pConstFP);
        }
        else
        {
            VISAModule::DataVector rawData;
            VISAModule->GetConstantData(pConstVal, rawData);
            addConstantData(VariableDie, rawData.data(), rawData.size());
        }
        DV.setDIE(VariableDie);
        return;
    }

    bool addDecoration = false;
    if (VISAModule->isDirectElfInput)
    {
        if (FirstLoc.HasSurface())
        {
            IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();
            addRegisterOp(Block, FirstLoc.GetSurface());
            // Now attach the surface information to the DIE.
            addBlock(VariableDie, dwarf::DW_AT_segment, Block);
        }

        IGC::DIEBlock* locationAT = nullptr;
        if (FirstLoc.IsSLM())
            locationAT = buildSLM(DV, &FirstLoc);
        else if (FirstLoc.IsSampler())
            locationAT = buildSampler(DV, &FirstLoc);
        else if (FirstLoc.HasSurface() &&
            (DV.getType() && DV.getType()->getTag() == dwarf::DW_TAG_pointer_type))
            locationAT = buildPointer(DV, &FirstLoc);
        else
            locationAT = buildGeneral(DV, &Locs, nullptr);

        if (locationAT)
            addBlock(VariableDie, dwarf::DW_AT_location, locationAT);
        addDecoration = true;
    }
    else
    {
        // Variable which is not immediate can have surface, location or both.
        if (FirstLoc.HasSurface())
        {
            IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();
            addRegisterOp(Block, FirstLoc.GetSurface());
            // Now attach the surface information to the DIE.
            addBlock(VariableDie, dwarf::DW_AT_segment, Block);
            if (!FirstLoc.HasLocation())
            {
                // Make sure there is always a location attribute when there is a surface attribute.
                // In this case, attach an zero address opcode location information to the DIE.
                IGC::DIEBlock* nBlock = new (DIEValueAllocator)IGC::DIEBlock();
                addUInt(nBlock, dwarf::DW_FORM_data1, dwarf::DW_OP_addr);
                dwarf::Form form = (Asm->GetPointerSize() == 8) ? dwarf::DW_FORM_data8 : dwarf::DW_FORM_data4;
                addUInt(nBlock, form, 0);

                addBlock(VariableDie, dwarf::DW_AT_location, nBlock);
            }
        }

        if (FirstLoc.HasLocation())
        {
#if 0
            /* This has been disabled because of following kind of metadata node generated:
            !98 = metadata !{i32 786688, metadata !11, metadata !"adder", metadata !6, i32 4, metadata !14, i32 0, metadata !56, i64
            2, i64 1, i64 32}

            numOperands here indicates complex addressing is used. But for complex addressing,
            9th operand should be a metadata node whereas here integer nodes are added.
            */
            IGC_ASSERT_MESSAGE(!(DV.variableHasComplexAddress() || DV.isBlockByrefVariable()), "Should handle complex address");
#endif
            IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();

            if (!FirstLoc.IsInMemory())
            {
                IGC_ASSERT_MESSAGE(FirstLoc.IsRegister(), "Direct location must be register");
                addRegisterOp(Block, FirstLoc.GetRegister());
            }
            else
            {
                if (FirstLoc.IsRegister())
                {
                    addRegisterOffset(Block, FirstLoc.GetRegister(), FirstLoc.GetOffset());
                }
                else
                {
                    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
                    addSInt(Block, dwarf::DW_FORM_udata, FirstLoc.GetOffset());
                }
            }

            // Now attach the location information to the DIE.
            addBlock(VariableDie, dwarf::DW_AT_location, Block);

            addDecoration = true;
        }

        DV.setDIE(VariableDie);
    }

    if (addDecoration)
    {
        if (FirstLoc.IsVectorized())
        {
            // Add description stating whether variable was vectorized in VISA
            addString(VariableDie, dwarf::DW_AT_description, "vectorized");
            uint16_t simdSize = VISAModule->GetSIMDSize();
            addString(VariableDie, dwarf::DW_AT_description,
                simdSize == 8 ? "simd8" : simdSize == 16 ? "simd16" : simdSize == 32 ? "simd32" : "???");
        }

        if (FirstLoc.IsInGlobalAddrSpace())
        {
            addString(VariableDie, dwarf::DW_AT_description, "global");
        }
    }
}

IGC::DIEBlock* CompileUnit::buildPointer(DbgVariable& var, VISAVariableLocation* loc)
{
    auto bti = loc->GetSurface() - VISAModule::TEXTURE_REGISTER_BEGIN;

    IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const4u);
    addUInt(Block, dwarf::DW_FORM_data4, 0);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
    addUInt(Block, dwarf::DW_FORM_data1, 32);
    addUInt(Block, dwarf::DW_FORM_data1, 0);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit0);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
    addUInt(Block, dwarf::DW_FORM_data1, 16);
    addUInt(Block, dwarf::DW_FORM_data1, 0);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
    addUInt(Block, dwarf::DW_FORM_data2, bti & 0xffff);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
    addUInt(Block, dwarf::DW_FORM_data1, 8);
    addUInt(Block, dwarf::DW_FORM_data1, 0);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit2);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);

    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
    addUInt(Block, dwarf::DW_FORM_data1, 8);
    addUInt(Block, dwarf::DW_FORM_data1, 0);

    return Block;
}

IGC::DIEBlock* CompileUnit::buildSampler(DbgVariable& var, VISAVariableLocation* loc)
{
    IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();

    if (EmitSettings.EnableGTLocationDebugging && loc->IsInGlobalAddrSpace())
    {
        addBindlessSamplerLocation(Block, loc); // Emit SLM location expression
    }
    else
    {
        Address addr;
        addr.Set(Address::Space::eSampler, loc->GetSurface() - VISAModule::SAMPLER_REGISTER_BEGIN, loc->GetOffset());

        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
        addUInt(Block, dwarf::DW_FORM_data8, addr.GetAddress());
    }

    return Block;
}

IGC::DIEBlock* CompileUnit::buildSLM(DbgVariable& var, VISAVariableLocation* loc)
{
    auto VISAMod = const_cast<VISAModule*>(loc->GetVISAModule());
    DbgDecoder::VarInfo varInfo;
    auto regNum = loc->GetRegister();
    VISAMod->getVarInfo("V", regNum, varInfo);

    if (varInfo.lrs.size() == 0)
        return nullptr;

    IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();

    if (loc->IsRegister())
    {
        if (!EmitSettings.EnableGTLocationDebugging)
        {
            Address addr;
            addr.Set(Address::Space::eLocal, 0, 0);

            addRegisterLoc(Block, varInfo.lrs.front().getGRF().regNum, 0, var.getDbgInst());

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref_size);
            addUInt(Block, dwarf::DW_FORM_data1, 4);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const2u);
            addUInt(Block, dwarf::DW_FORM_data2, 0xffff);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_and);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
            addUInt(Block, dwarf::DW_FORM_data8, addr.GetAddress());

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);

            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);
        }
        else
        {
            addSLMLocation(Block, loc); // Emit SLM location expression
        }

        addSimdLane(Block, var, loc, &varInfo.lrs.front(), 0, false, false); // Emit SIMD lane for SLM (unpacked)
        if (EmitSettings.EnableSIMDLaneDebugging)
        {
            IGC_ASSERT(loc->GetVectorNumElements() <= 1);
        }
    }
    else
    {
        if (!EmitSettings.EnableGTLocationDebugging)
        {
            // Immediate offset in to SLM
            auto offset = loc->GetOffset();

            Address addr;
            addr.Set(Address::Space::eLocal, 0, offset);
            addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
            addUInt(Block, dwarf::DW_FORM_data8, addr.GetAddress());
        }
        else
        {
            addSLMLocation(Block, loc); // Emit SLM location expression
        }

        addSimdLane(Block, var, loc, &varInfo.lrs.front(), 0, false, false); // Emit SIMD lane for SLM (unpacked)
        if (EmitSettings.EnableSIMDLaneDebugging)
        {
            IGC_ASSERT(loc->GetVectorNumElements() <= 1);
        }
    }
    return Block;
}

IGC::DIEBlock* CompileUnit::buildGeneral(DbgVariable& var, std::vector<VISAVariableLocation>* locs, std::vector<DbgDecoder::LiveIntervalsVISA>* vars)
{
    IGC::DIEBlock* Block = new (DIEValueAllocator)IGC::DIEBlock();
    bool emitLocation = false;
    bool isSliced = (locs->size() > 1);
    bool firstHalf = true;
    DIEValue* secondHalfOff = nullptr, *skipOff = nullptr;
    unsigned int offsetTaken = 0, offsetNotTaken = 0, offsetEnd = 0;
    // locs contains 1 item for SIMD8/SIMD16 kernels describing locations of all channels.
    // locs contains 2 items for SIMD32 kernels. First item has storage mapping for lower 16
    // channels, second item has storage mapping for upper 16 channels.
    for (auto& locV : *locs)
    {
        auto loc = &locV;
        int64_t offset = 0;
        auto storageMD = var.getDbgInst()->getMetadata("StorageOffset");
        auto VISAMod = const_cast<VISAModule*>(loc->GetVISAModule());
        VISAVariableLocation V(VISAMod);
        if (storageMD && EmitSettings.EmitOffsetInDbgLoc)
        {
            // Storage offset is known so emit location as
            // DW_OP_bregx (privateBase) + StorageOffset*SIMD_SIZE
            // This is executed only when llvm.dbg.declare still exists.
            // With mem2reg run, data is stored in GRFs and this wont be
            // executed.
            auto privateBaseRegNum = loc->GetVISAModule()->getPrivateBaseReg();
            if (privateBaseRegNum)
            {
                auto simdSize = VISAMod->GetSIMDSize();
                offset = simdSize * dyn_cast<ConstantAsMetadata>(storageMD->getOperand(0))->getValue()->getUniqueInteger().getSExtValue();
                V = VISAVariableLocation(VISAModule::GENERAL_REGISTER_BEGIN + privateBaseRegNum, loc->IsRegister(),
                    loc->IsInMemory(), loc->GetVectorNumElements(), loc->IsVectorized(), loc->IsInGlobalAddrSpace(), VISAMod);
                loc = &V;
            }
        }

        if (EmitSettings.EnableSIMDLaneDebugging && isSliced)
        {
            // DW_OP_push_simd_lane
            // DW_OP_lit16
            // DW_OP_ge
            // DW_OP_bra secondHalf
            // -- emit first half
            // DW_OP_skip end
            // secondHalf:
            // -- emit second half
            // end:
            if (firstHalf)
            {
                addUInt(Block, dwarf::DW_FORM_data1, DW_OP_INTEL_push_simd_lane);
                addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_lit16);
                addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_ge);
                addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bra);
                addUInt(Block, dwarf::DW_FORM_data2, 0xf00d);
                secondHalfOff = Block->getValues().back();
                IGC_ASSERT_MESSAGE(isa<DIEInteger>(secondHalfOff), "Expecting DIEInteger");
                offsetNotTaken = Block->ComputeSizeOnTheFly(Asm);
            }
            else
            {
                addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_skip);
                addUInt(Block, dwarf::DW_FORM_data2, 0xbeef);
                skipOff = Block->getValues().back();
                IGC_ASSERT_MESSAGE(isa<DIEInteger>(skipOff), "Expecting DIEInteger");
                offsetTaken = Block->ComputeSizeOnTheFly(Asm);
                cast<DIEInteger>(secondHalfOff)->setValue(offsetTaken - offsetNotTaken);
            }
        }

        DbgDecoder::VarInfo varInfo;
        if (!vars)
        {
            // When vars is valid, use it to encode location directly, otherwise
            // rely on getVarInfo result here.
            auto regNum = loc->GetRegister();
            VISAMod->getVarInfo("V", regNum, varInfo);
        }

        if (varInfo.lrs.size() > 0 || (vars && vars->size() >= (firstHalf ? 1u : 2u)))
        {
            auto& lrToUse = vars ? vars->at(firstHalf ? 0 : 1) : varInfo.lrs.front();
            emitLocation = true;
            if (lrToUse.isGRF())
            {
                uint16_t regNum = lrToUse.getGRF().regNum;

                if (!EmitSettings.EnableSIMDLaneDebugging)
                {
                    addRegisterLoc(Block, lrToUse.getGRF().regNum, offset, var.getDbgInst());

                    if (lrToUse.getGRF().subRegNum != 0)
                    {
                        unsigned int subReg = lrToUse.getGRF().subRegNum;
                        auto offsetInBits = subReg * 8;
                        auto sizeInBits = (VISAMod->getGRFSize() * 8) - offsetInBits;

                        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_bit_piece);
                        addUInt(Block, dwarf::DW_FORM_udata, sizeInBits);
                        addUInt(Block, dwarf::DW_FORM_udata, offsetInBits);
                    }
                }
                else
                {
                    IGC_ASSERT(EmitSettings.EnableSIMDLaneDebugging);

                    if (loc->IsVectorized() == false)
                    {
                        unsigned int subReg = lrToUse.getGRF().subRegNum;
                        addRegisterLoc(Block, regNum, 0, var.getDbgInst());

                        addGTRelativeLocation(Block, loc); // Emit GT-relative location expression

                        addSimdLaneScalar(Block, var, loc, &lrToUse, subReg); // Emit subregister for GRF (unpacked)

                        break;
                    }
                    else
                    {
                        for (unsigned int vectorElem = 0; vectorElem < loc->GetVectorNumElements(); ++vectorElem)
                        {
                            addGTRelativeLocation(Block, loc); // Emit GT-relative location expression

                            // Emit SIMD lane for GRF (unpacked)
                            addSimdLane(Block, var, loc, &lrToUse, (uint16_t)(DD->simdWidth * vectorElem), false, !firstHalf);
                        }
                    }
                }
            }
            else if (lrToUse.isSpill())
            {
                // handle spill
                if (!EmitSettings.EnableSIMDLaneDebugging)
                {
                    Address addr;
                    addr.Set(Address::Space::eScratch, 0, lrToUse.getSpillOffset().memoryOffset);

                    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_const8u);
                    addUInt(Block, dwarf::DW_FORM_data8, addr.GetAddress());
                    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
                }
                else
                {
                    if (loc->IsVectorized() == false)
                    {
                        addScratchLocation(Block, &lrToUse, 0);
                    }
                    else
                    {
                        uint16_t grfSize = (uint16_t)VISAMod->getGRFSize();
                        uint16_t varSizeInBits = loc->IsInMemory() ? (uint16_t)Asm->GetPointerSize() * 8 : (uint16_t)var.getBasicSize(DD);
                        uint16_t varSizeInReg = (uint16_t)(loc->IsInMemory() && varSizeInBits < 32) ? 32 : varSizeInBits;
                        uint16_t numOfRegs = ((varSizeInReg * (uint16_t)DD->simdWidth) > (grfSize * 8)) ?
                            ((varSizeInReg * (uint16_t)DD->simdWidth) / (grfSize * 8)) : 1;

                        for (unsigned int vectorElem = 0; vectorElem < loc->GetVectorNumElements(); ++vectorElem)
                        {
                            addScratchLocation(Block, &lrToUse, vectorElem * numOfRegs * grfSize);
                            addSimdLane(Block, var, loc, &lrToUse, 0, false, !firstHalf); // Emit SIMD lane for spill (unpacked)
                        }
                    }
                }
            }
            else
            {
                //IGC_ASSERT_MESSAGE(false, "\nVariable neither in GRF nor spilled\n");
            }

            // Emit DIExpression if it exists
            if (auto dbgInst = dyn_cast_or_null<IGCLLVM::DbgVariableIntrinsic>(var.getDbgInst()))
            {
                if (auto expr = dbgInst->getExpression())
                {
                    for (auto elem : expr->getElements())
                    {
                        auto BF = DIEInteger::BestForm(false, elem);
                        addUInt(Block, BF, elem);
                    }
                }
            }
        }
        firstHalf = false;
    }

    if (skipOff)
    {
        offsetEnd = Block->ComputeSizeOnTheFly(Asm);
        cast<DIEInteger>(skipOff)->setValue(offsetEnd - offsetTaken);
    }

    if (!emitLocation)
        return nullptr;

    return Block;
}

/// constructMemberDIE - Construct member DIE from DIDerivedType.
void CompileUnit::constructMemberDIE(DIE& Buffer, DIDerivedType* DT)
{
    DIE* MemberDie = createAndAddDIE(DT->getTag(), Buffer);
    StringRef Name = DT->getName();
    if (!Name.empty())
    {
        addString(MemberDie, dwarf::DW_AT_name, Name);
    }

    addType(MemberDie, resolve(DT->getBaseType()));

    addSourceLine(MemberDie, DT);

    IGC::DIEBlock* MemLocationDie = new (DIEValueAllocator)IGC::DIEBlock();
    addUInt(MemLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);

    if (DT->getTag() == dwarf::DW_TAG_inheritance && DT->isVirtual())
    {
        // For C++, virtual base classes are not at fixed offset. Use following
        // expression to extract appropriate offset from vtable.
        // BaseAddr = ObAddr + *((*ObAddr) - Offset)

        IGC::DIEBlock* VBaseLocationDie = new (DIEValueAllocator)IGC::DIEBlock();
        addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_dup);
        addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
        addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
        addUInt(VBaseLocationDie, dwarf::DW_FORM_udata, DT->getOffsetInBits());
        addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_minus);
        addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
        addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);

        addBlock(MemberDie, dwarf::DW_AT_data_member_location, VBaseLocationDie);
    }
    else
    {
        uint64_t Size = DT->getSizeInBits();
        uint64_t FieldSize = getBaseTypeSize(DD, DT);
        uint64_t OffsetInBytes;

        if (Size != FieldSize)
        {
            // Handle bitfield.
            addUInt(MemberDie, dwarf::DW_AT_byte_size, None,
                getBaseTypeSize(DD, DT) >> 3);
            addUInt(MemberDie, dwarf::DW_AT_bit_size, None, DT->getSizeInBits());

            uint64_t Offset = DT->getOffsetInBits();
            uint64_t AlignMask = ~(DT->getAlignInBits() - 1);
            uint64_t HiMark = (Offset + FieldSize) & AlignMask;
            uint64_t FieldOffset = (HiMark - FieldSize);
            Offset -= FieldOffset;

            // Maybe we need to work from the other end.
            if (Asm->IsLittleEndian())
                Offset = FieldSize - (Offset + Size);
            addUInt(MemberDie, dwarf::DW_AT_bit_offset, None, Offset);

            // Here WD_AT_data_member_location points to the anonymous
            // field that includes this bit field.
            OffsetInBytes = FieldOffset >> 3;
        }
        else
        {
            // This is not a bitfield.
            OffsetInBytes = DT->getOffsetInBits() >> 3;
        }
        addUInt(MemberDie, dwarf::DW_AT_data_member_location, None, OffsetInBytes);
    }
    dwarf::AccessAttribute dw_access = (DT->isProtected()) ? dwarf::DW_ACCESS_protected :
        (DT->isPrivate()) ? dwarf::DW_ACCESS_private : dwarf::DW_ACCESS_public;
    addUInt(MemberDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1, dw_access);

    if (DT->isVirtual())
    {
        addUInt(MemberDie, dwarf::DW_AT_virtuality, dwarf::DW_FORM_data1, dwarf::DW_VIRTUALITY_virtual);
    }

    if (DT->isArtificial())
    {
        addFlag(MemberDie, dwarf::DW_AT_artificial);
    }
}

/// getOrCreateStaticMemberDIE - Create new DIE for C++ static member.
IGC::DIE* CompileUnit::getOrCreateStaticMemberDIE(DIDerivedType* DT)
{
    // Construct the context before querying for the existence of the DIE in case
    // such construction creates the DIE.
    DIE* ContextDIE = getOrCreateContextDIE(resolve(DT->getScope()));
    IGC_ASSERT_MESSAGE(dwarf::isType(ContextDIE->getTag()), "Static member should belong to a type.");

    DIE* StaticMemberDIE = getDIE(DT);
    if (StaticMemberDIE)
        return StaticMemberDIE;

    StaticMemberDIE = createAndAddDIE(DT->getTag(), *ContextDIE, DT);

    DIType* Ty = resolve(DT->getBaseType());

    addString(StaticMemberDIE, dwarf::DW_AT_name, DT->getName());
    addType(StaticMemberDIE, Ty);
    addSourceLine(StaticMemberDIE, DT);
    addFlag(StaticMemberDIE, dwarf::DW_AT_external);
    addFlag(StaticMemberDIE, dwarf::DW_AT_declaration);

    // FIXME: We could omit private if the parent is a class_type, and
    // public if the parent is something else.
    dwarf::AccessAttribute dw_access = (DT->isProtected()) ? dwarf::DW_ACCESS_protected :
        (DT->isPrivate()) ? dwarf::DW_ACCESS_private : dwarf::DW_ACCESS_public;
    addUInt(StaticMemberDIE, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1, dw_access);

    if (const ConstantInt * CI = dyn_cast_or_null<ConstantInt>(DT->getConstant()))
    {
        addConstantValue(StaticMemberDIE, CI, isUnsignedDIType(DD, Ty));
    }

    if (const ConstantFP * CFP = dyn_cast_or_null<ConstantFP>(DT->getConstant()))
    {
        addConstantFPValue(StaticMemberDIE, CFP);
    }

    return StaticMemberDIE;
}

void CompileUnit::emitHeader(const MCSection* ASection, const MCSymbol* ASectionSym)
{
    // Emit ("DWARF version number");
    Asm->EmitInt16(DD->getDwarfVersion());
    // Emit ("Offset Into Abbrev. Section");
    Asm->EmitSectionOffset(Asm->GetTempSymbol(/*ASection->getLabelBeginName()*/".debug_abbrev_begin"), ASectionSym);
    // Emit ("Address Size (in bytes)");
    Asm->EmitInt8(Asm->GetPointerSize());
}


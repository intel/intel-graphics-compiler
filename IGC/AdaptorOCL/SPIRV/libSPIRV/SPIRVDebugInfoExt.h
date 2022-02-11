/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines Debug Info ext instruction class for SPIR-V.
///
//===----------------------------------------------------------------------===//

#ifndef SPIRVDEBUGINFOEXT_HPP_
#define SPIRVDEBUGINFOEXT_HPP_

#include <llvm/BinaryFormat/Dwarf.h>

#include "SPIRVEntry.h"
#include "SPIRVInstruction.h"
#include "SPIRVExtInst.h"
#include "spirv.hpp"

namespace igc_spv {

    // SPIRVDebug is shared with common clang's SPIRV emitter
    namespace SPIRVDebug {

        const unsigned int DebugInfoVersion = 10001;

        enum Instruction {
            DebugInfoNone = 0,
            CompilationUnit = 1,
            TypeBasic = 2,
            TypePointer = 3,
            TypeQualifier = 4,
            TypeArray = 5,
            TypeVector = 6,
            Typedef = 7,
            TypeFunction = 8,
            TypeEnum = 9,
            TypeComposite = 10,
            TypeMember = 11,
            Inheritance = 12,
            TypePtrToMember = 13,
            TypeTemplate = 14,
            TypeTemplateParameter = 15,
            TypeTemplateTemplateParameter = 16,
            TypeTemplateParameterPack = 17,
            GlobalVariable = 18,
            FunctionDecl = 19,
            Function = 20,
            LexicalBlock = 21,
            LexicalBlockDiscriminator = 22,
            Scope = 23,
            NoScope = 24,
            InlinedAt = 25,
            LocalVariable = 26,
            InlinedVariable = 27,
            Declare = 28,
            Value = 29,
            Operation = 30,
            Expression = 31,
            MacroDef = 32,
            MacroUndef = 33,
            ImportedEntity = 34,
            Source = 35,
            ModuleINTEL = 36,
            InstCount = 37
        };

        enum Flag {
            FlagIsProtected = 1 << 0,
            FlagIsPrivate = 1 << 1,
            FlagIsPublic = FlagIsPrivate | FlagIsProtected,
            FlagAccess = FlagIsPublic,
            FlagIsLocal = 1 << 2,
            FlagIsDefinition = 1 << 3,
            FlagIsFwdDecl = 1 << 4,
            FlagIsArtificial = 1 << 5,
            FlagIsExplicit = 1 << 6,
            FlagIsPrototyped = 1 << 7,
            FlagIsObjectPointer = 1 << 8,
            FlagIsStaticMember = 1 << 9,
            FlagIsIndirectVariable = 1 << 10,
            FlagIsLValueReference = 1 << 11,
            FlagIsRValueReference = 1 << 12,
            FlagIsOptimized = 1 << 13,
            FlagIsEnumClass = 1 << 14,
            FlagTypePassByValue = 1 << 15,
            FlagTypePassByReference = 1 << 16,
        };

        enum EncodingTag {
            Unspecified = 0,
            Address = 1,
            Boolean = 2,
            Float = 3,
            Signed = 4,
            SignedChar = 5,
            Unsigned = 6,
            UnsignedChar = 7
        };

        enum CompositeTypeTag {
            Class = 0,
            Structure = 1,
            Union = 2
        };

        enum TypeQualifierTag {
            ConstType = 0,
            VolatileType = 1,
            RestrictType = 2,
            AtomicType = 3
        };

        enum ExpressionOpCode {
            Deref = 0,
            Plus = 1,
            Minus = 2,
            PlusUconst = 3,
            BitPiece = 4,
            Swap = 5,
            Xderef = 6,
            StackValue = 7,
            Constu = 8,
            Fragment = 9,
            Convert = 10,
            Addr = 11,
            Const1u = 12,
            Const1s = 13,
            Const2u = 14,
            Const2s = 15,
            Const4u = 16,
            Const4s = 17,
            Const8u = 18,
            Const8s = 19,
            Consts = 20,
            Dup = 21,
            Drop = 22,
            Over = 23,
            Pick = 24,
            Rot = 25,
            Abs = 26,
            And = 27,
            Div = 28,
            Mod = 29,
            Mul = 30,
            Neg = 31,
            Not = 32,
            Or = 33,
            Shl = 34,
            Shr = 35,
            Shra = 36,
            Xor = 37,
            Bra = 38,
            Eq = 39,
            Ge = 40,
            Gt = 41,
            Le = 42,
            Lt = 43,
            Ne = 44,
            Skip = 45,
            Lit0 = 46,
            Lit1 = 47,
            Lit2 = 48,
            Lit3 = 49,
            Lit4 = 50,
            Lit5 = 51,
            Lit6 = 52,
            Lit7 = 53,
            Lit8 = 54,
            Lit9 = 55,
            Lit10 = 56,
            Lit11 = 57,
            Lit12 = 58,
            Lit13 = 59,
            Lit14 = 60,
            Lit15 = 61,
            Lit16 = 62,
            Lit17 = 63,
            Lit18 = 64,
            Lit19 = 65,
            Lit20 = 66,
            Lit21 = 67,
            Lit22 = 68,
            Lit23 = 69,
            Lit24 = 70,
            Lit25 = 71,
            Lit26 = 72,
            Lit27 = 73,
            Lit28 = 74,
            Lit29 = 75,
            Lit30 = 76,
            Lit31 = 77,
            Reg0 = 78,
            Reg1 = 79,
            Reg2 = 80,
            Reg3 = 81,
            Reg4 = 82,
            Reg5 = 83,
            Reg6 = 84,
            Reg7 = 85,
            Reg8 = 86,
            Reg9 = 87,
            Reg10 = 88,
            Reg11 = 89,
            Reg12 = 90,
            Reg13 = 91,
            Reg14 = 92,
            Reg15 = 93,
            Reg16 = 94,
            Reg17 = 95,
            Reg18 = 96,
            Reg19 = 97,
            Reg20 = 98,
            Reg21 = 99,
            Reg22 = 100,
            Reg23 = 101,
            Reg24 = 102,
            Reg25 = 103,
            Reg26 = 104,
            Reg27 = 105,
            Reg28 = 106,
            Reg29 = 107,
            Reg30 = 108,
            Reg31 = 109,
            Breg0 = 110,
            Breg1 = 111,
            Breg2 = 112,
            Breg3 = 113,
            Breg4 = 114,
            Breg5 = 115,
            Breg6 = 116,
            Breg7 = 117,
            Breg8 = 118,
            Breg9 = 119,
            Breg10 = 120,
            Breg11 = 121,
            Breg12 = 122,
            Breg13 = 123,
            Breg14 = 124,
            Breg15 = 125,
            Breg16 = 126,
            Breg17 = 127,
            Breg18 = 128,
            Breg19 = 129,
            Breg20 = 130,
            Breg21 = 131,
            Breg22 = 132,
            Breg23 = 133,
            Breg24 = 134,
            Breg25 = 135,
            Breg26 = 136,
            Breg27 = 137,
            Breg28 = 138,
            Breg29 = 139,
            Breg30 = 140,
            Breg31 = 141,
            Regx = 142,
            Fbreg = 143,
            Bregx = 144,
            Piece = 145,
            DerefSize = 146,
            XderefSize = 147,
            Nop = 148,
            PushObjectAddress = 149,
            Call2 = 150,
            Call4 = 151,
            CallRef = 152,
            FormTlsAddress = 153,
            CallFrameCfa = 154,
            ImplicitValue = 155,
            ImplicitPointer = 156,
            Addrx = 157,
            Constx = 158,
            EntryValue = 159,
            ConstTypeOp = 160,
            RegvalType = 161,
            DerefType = 162,
            XderefType = 163,
            Reinterpret = 164
        };

        static std::unordered_map<ExpressionOpCode, unsigned> OpCountMap{
        { Deref,              1 },
        { Plus,               1 },
        { Minus,              1 },
        { PlusUconst,         2 },
        { BitPiece,           3 },
        { Swap,               1 },
        { Xderef,             1 },
        { StackValue,         1 },
        { Constu,             2 },
        { Fragment,           3 },
        { Convert,            3 },
        { Addr,               2 },
        { Const1u,            2 },
        { Const1s,            2 },
        { Const2u,            2 },
        { Const2s,            2 },
        { Const4u,            2 },
        { Const4s,            2 },
        { Const8u,            2 },
        { Const8s,            2 },
        { Consts,             2 },
        { Dup,                1 },
        { Drop,               1 },
        { Over,               1 },
        { Pick,               1 },
        { Rot,                1 },
        { Abs,                1 },
        { And,                1 },
        { Div,                1 },
        { Mod,                1 },
        { Mul,                1 },
        { Neg,                1 },
        { Not,                1 },
        { Or,                 1 },
        { Shl,                1 },
        { Shr,                1 },
        { Shra,               1 },
        { Xor,                1 },
        { Bra,                2 },
        { Eq,                 1 },
        { Ge,                 1 },
        { Gt,                 1 },
        { Le,                 1 },
        { Lt,                 1 },
        { Ne,                 1 },
        { Skip,               2 },
        { Lit0,               1 },
        { Lit1,               1 },
        { Lit2,               1 },
        { Lit3,               1 },
        { Lit4,               1 },
        { Lit5,               1 },
        { Lit6,               1 },
        { Lit7,               1 },
        { Lit8,               1 },
        { Lit9,               1 },
        { Lit10,              1 },
        { Lit11,              1 },
        { Lit12,              1 },
        { Lit13,              1 },
        { Lit14,              1 },
        { Lit15,              1 },
        { Lit16,              1 },
        { Lit17,              1 },
        { Lit18,              1 },
        { Lit19,              1 },
        { Lit20,              1 },
        { Lit21,              1 },
        { Lit22,              1 },
        { Lit23,              1 },
        { Lit24,              1 },
        { Lit25,              1 },
        { Lit26,              1 },
        { Lit27,              1 },
        { Lit28,              1 },
        { Lit29,              1 },
        { Lit30,              1 },
        { Lit31,              1 },
        { Reg0,               1 },
        { Reg1,               1 },
        { Reg2,               1 },
        { Reg3,               1 },
        { Reg4,               1 },
        { Reg5,               1 },
        { Reg6,               1 },
        { Reg7,               1 },
        { Reg8,               1 },
        { Reg9,               1 },
        { Reg10,              1 },
        { Reg11,              1 },
        { Reg12,              1 },
        { Reg13,              1 },
        { Reg14,              1 },
        { Reg15,              1 },
        { Reg16,              1 },
        { Reg17,              1 },
        { Reg18,              1 },
        { Reg19,              1 },
        { Reg20,              1 },
        { Reg21,              1 },
        { Reg22,              1 },
        { Reg23,              1 },
        { Reg24,              1 },
        { Reg25,              1 },
        { Reg26,              1 },
        { Reg27,              1 },
        { Reg28,              1 },
        { Reg29,              1 },
        { Reg30,              1 },
        { Reg31,              1 },
        { Breg0,              2 },
        { Breg1,              2 },
        { Breg2,              2 },
        { Breg3,              2 },
        { Breg4,              2 },
        { Breg5,              2 },
        { Breg6,              2 },
        { Breg7,              2 },
        { Breg8,              2 },
        { Breg9,              2 },
        { Breg10,             2 },
        { Breg11,             2 },
        { Breg12,             2 },
        { Breg13,             2 },
        { Breg14,             2 },
        { Breg15,             2 },
        { Breg16,             2 },
        { Breg17,             2 },
        { Breg18,             2 },
        { Breg19,             2 },
        { Breg20,             2 },
        { Breg21,             2 },
        { Breg22,             2 },
        { Breg23,             2 },
        { Breg24,             2 },
        { Breg25,             2 },
        { Breg26,             2 },
        { Breg27,             2 },
        { Breg28,             2 },
        { Breg29,             2 },
        { Breg30,             2 },
        { Breg31,             2 },
        { Regx,               2 },
        { Fbreg,              1 },
        { Bregx,              3 },
        { Piece,              2 },
        { DerefSize,          2 },
        { XderefSize,         2 },
        { Nop,                1 },
        { PushObjectAddress,  1 },
        { Call2,              2 },
        { Call4,              2 },
        { CallRef,            2 },
        { FormTlsAddress,     1 },
        { CallFrameCfa,       1 },
        { ImplicitValue,      3 },
        { ImplicitPointer,    3 },
        { Addrx,              2 },
        { Constx,             2 },
        { EntryValue,         3 },
        { ConstTypeOp,        4 },
        { RegvalType,         3 },
        { DerefType,          3 },
        { XderefType,         3 },
        { Reinterpret,        2 },
        };

        enum ImportedEntityTag {
            ImportedModule = 0,
            ImportedDeclaration = 1,
        };

        namespace Operand {

            namespace CompilationUnit {
                enum {
                    SPIRVDebugInfoVersionIdx = 0,
                    DWARFVersionIdx = 1,
                    SourceIdx = 2,
                    LanguageIdx = 3,
                    OperandCount = 4
                };
            }

            namespace Source {
                enum {
                    FileIdx = 0,
                    TextIdx = 1,
                    OperandCount = 2
                };
            }

            namespace TypeBasic {
                enum {
                    NameIdx = 0,
                    SizeIdx = 1,
                    EncodingIdx = 2,
                    OperandCount = 3
                };
            }

            namespace TypePointer {
                enum {
                    BaseTypeIdx = 0,
                    StorageClassIdx = 1,
                    FlagsIdx = 2,
                    OperandCount = 3
                };
            }

            namespace TypeQualifier {
                enum {
                    BaseTypeIdx = 0,
                    QualifierIdx = 1,
                    OperandCount = 2
                };
            }

            namespace TypeArray {
                enum {
                    BaseTypeIdx = 0,
                    ComponentCountIdx = 1,
                    MinOperandCount = 2
                };
            }

            namespace TypeVector = TypeArray;

            namespace Typedef {
                enum {
                    NameIdx = 0,
                    BaseTypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    OperandCount = 6
                };
            }

            namespace TypeFunction {
                enum {
                    FlagsIdx = 0,
                    ReturnTypeIdx = 1,
                    FirstParameterIdx = 2,
                    MinOperandCount = 2
                };
            }

            namespace TypeEnum {
                enum {
                    NameIdx = 0,
                    UnderlyingTypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    SizeIdx = 6,
                    FlagsIdx = 7,
                    FirstEnumeratorIdx = 8,
                    MinOperandCount = 8
                };
            }

            namespace TypeComposite {
                enum {
                    NameIdx = 0,
                    TagIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    LinkageNameIdx = 6,
                    SizeIdx = 7,
                    FlagsIdx = 8,
                    FirstMemberIdx = 9,
                    MinOperandCount = 9
                };
            }

            namespace TypeMember {
                enum {
                    NameIdx = 0,
                    TypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    OffsetIdx = 6,
                    SizeIdx = 7,
                    FlagsIdx = 8,
                    ValueIdx = 9,
                    MinOperandCount = 9
                };
            }

            namespace TypeInheritance {
                enum {
                    ChildIdx = 0,
                    ParentIdx = 1,
                    OffsetIdx = 2,
                    SizeIdx = 3,
                    FlagsIdx = 4,
                    OperandCount = 5
                };
            }

            namespace PtrToMember {
                enum {
                    MemberTypeIdx = 0,
                    ParentIdx = 1,
                    OperandCount = 2
                };
            }

            namespace Template {
                enum {
                    TargetIdx = 0,
                    FirstParameterIdx = 1,
                    MinOperandCount = 1
                };
            }

            namespace TemplateParameter {
                enum {
                    NameIdx = 0,
                    TypeIdx = 1,
                    ValueIdx = 2,
                    SourceIdx = 3,
                    LineIdx = 4,
                    ColumnIdx = 5,
                    OperandCount = 6
                };
            }

            namespace TemplateTemplateParameter {
                enum {
                    NameIdx = 0,
                    TemplateNameIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    OperandCount = 4
                };
            }

            namespace TemplateParameterPack {
                enum {
                    NameIdx = 0,
                    SourceIdx = 1,
                    LineIdx = 2,
                    ColumnIdx = 3,
                    FirstParameterIdx = 4,
                    MinOperandCount = 4
                };
            }

            namespace GlobalVariable {
                enum {
                    NameIdx = 0,
                    TypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    LinkageNameIdx = 6,
                    VariableIdx = 7,
                    FlagsIdx = 8,
                    StaticMemberDeclarationIdx = 9,
                    MinOperandCount = 9
                };
            }

            namespace FunctionDeclaration {
                enum {
                    NameIdx = 0,
                    TypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    LinkageNameIdx = 6,
                    FlagsIdx = 7,
                    OperandCount = 8
                };
            }

            namespace Function {
                enum {
                    NameIdx = 0,
                    TypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    LinkageNameIdx = 6,
                    FlagsIdx = 7,
                    ScopeLineIdx = 8,
                    FunctionIdIdx = 9,
                    DeclarationIdx = 10,
                    MinOperandCount = 10
                };
            }

            namespace LexicalBlock {
                enum {
                    SourceIdx = 0,
                    LineIdx = 1,
                    ColumnIdx = 2,
                    ParentIdx = 3,
                    NameIdx = 4,
                    MinOperandCount = 4
                };
            }

            namespace LexicalBlockDiscriminator {
                enum {
                    SourceIdx = 0,
                    DiscriminatorIdx = 1,
                    ParentIdx = 2,
                    OperandCount = 3
                };
            }

            namespace Scope {
                enum {
                    ScopeIdx = 0,
                    InlinedAtIdx = 1,
                    MinOperandCount = 1
                };
            }

            namespace NoScope {
                // No operands
            }

            namespace InlinedAt {
                enum {
                    LineIdx = 0,
                    ScopeIdx = 1,
                    InlinedIdx = 2,
                    MinOperandCount = 2
                };
            }

            namespace LocalVariable {
                enum {
                    NameIdx = 0,
                    TypeIdx = 1,
                    SourceIdx = 2,
                    LineIdx = 3,
                    ColumnIdx = 4,
                    ParentIdx = 5,
                    FlagsIdx = 6,
                    ArgNumberIdx = 7,
                    MinOperandCount = 7
                };
            }

            namespace InlinedVariable {
                enum {
                    VariableIdx = 0,
                    InlinedIdx = 1,
                    OperandCount = 2
                };
            }

            namespace DebugDeclare {
                enum {
                    DebugLocalVarIdx = 0,
                    VariableIdx = 1,
                    ExpressionIdx = 2,
                    OperandCount = 3
                };
            }

            namespace DebugValue {
                enum {
                    DebugLocalVarIdx = 0,
                    ValueIdx = 1,
                    ExpressionIdx = 2,
                    FirstIndexOperandIdx = 3,
                    MinOperandCount = 3
                };
            }

            namespace Operation {
                enum {
                    OpCodeIdx = 0
                };
                static std::map<ExpressionOpCode, unsigned> OpCountMap{
                    { Deref,      1 },
                    { Plus,       2 },
                    { Minus,      2 },
                    { PlusUconst, 2 },
                    { BitPiece,   3 },
                    { Swap,       1 },
                    { Xderef,     1 },
                    { StackValue, 1 },
                    { Constu,     2 },
                    { Fragment,   3 }
                };
            }

            namespace ImportedEntity {
                enum {
                    NameIdx = 0,
                    TagIdx = 1,
                    SourceIdx = 3,
                    EntityIdx = 4,
                    LineIdx = 5,
                    ColumnIdx = 6,
                    ParentIdx = 7,
                    OperandCount = 8
                };
            }

            namespace ModuleINTEL {
                enum {
                    NameIdx = 0,
                    SourceIdx = 1,
                    LineIdx = 2,
                    ParentIdx = 3,
                    ConfigurationMacrosIdx = 4,
                    IncludePathIdx = 5,
                    APINotesFileIdx = 6,
                    IsDeclIdx = 7,
                    OperandCount = 8
                };
            }
        } // namespace Operand
    } // namespace SPIRVDebug

    // SPIRV Debug info class containers. These classes are light-weight as they
    // only help interpret a given SPIRVExtInst during SPIRV->LLVM IR translation.
    class OpDebugInfoBase
    {
    public:
        OpDebugInfoBase(SPIRVExtInst* i) : extInst(i)
        {
        }

    protected:
        SPIRVExtInst* const extInst;
        template <typename T>
        T arg(unsigned int id) const
        {
            return static_cast<T>(extInst->getArguments()[id]);
        }
        SPIRVString* str(SPIRVId id) const
        {
            auto item = extInst->getModule()->getEntry(arg<SPIRVId>(id));
            if (item->isString())
                return static_cast<SPIRVString*>(item);
            else
                return nullptr;
        }
        uint64_t const_val(SPIRVId id) const
        {
            auto item = extInst->getModule()->getEntry(arg<SPIRVId>(id));
            if (item->isConstant())
                return static_cast<SPIRVConstant*>(item)->getZExtIntValue();
            else
                return (uint64_t)-1;
        }
        unsigned int getNumArgs() const { return extInst->getArguments().size(); }
    };

    class OpCompilationUnit : OpDebugInfoBase
    {
    public:
        OpCompilationUnit(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVWord getSPIRVDIVersion() { return arg<SPIRVWord>(SPIRVDebug::Operand::CompilationUnit::SPIRVDebugInfoVersionIdx); }
        SPIRVWord getDWARFVersion() { return arg<SPIRVWord>(SPIRVDebug::Operand::CompilationUnit::DWARFVersionIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::CompilationUnit::SourceIdx); }
        SPIRVWord getLang() { return arg<SPIRVWord>(SPIRVDebug::Operand::CompilationUnit::LanguageIdx); }
    };

    class OpDebugLexicalBlock : OpDebugInfoBase
    {
    public:
        OpDebugLexicalBlock(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getResultId() { return extInst->getId(); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::LexicalBlock::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::LexicalBlock::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::LexicalBlock::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::LexicalBlock::ParentIdx); }
        const bool hasNameSpace() { return getNumArgs() > SPIRVDebug::Operand::LexicalBlock::MinOperandCount; }
        SPIRVString* getNameSpace() { return str(SPIRVDebug::Operand::LexicalBlock::NameIdx); }
    };

    class OpDebugSubprogram : OpDebugInfoBase
    {
    public:
        OpDebugSubprogram(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::Function::NameIdx); }
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::Function::TypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::Function::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::Function::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::Function::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::Function::ParentIdx); }
        SPIRVString* getLinkage() { return str(SPIRVDebug::Operand::Function::LinkageNameIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::Function::FlagsIdx); }
        SPIRVWord getScopeLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::Function::ScopeLineIdx); }
        SPIRVId getSPIRVFunction() { return arg<SPIRVId>(SPIRVDebug::Operand::Function::FunctionIdIdx); }
        bool hasDeclaration() { return getNumArgs() > SPIRVDebug::Operand::Function::DeclarationIdx; }
        SPIRVId getDecl() { return arg<SPIRVId>(SPIRVDebug::Operand::Function::DeclarationIdx); }
    };

    class OpDebugSubroutineType : OpDebugInfoBase
    {
    public:
        OpDebugSubroutineType(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeFunction::FlagsIdx); }
        SPIRVId getReturnType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeFunction::ReturnTypeIdx); }
        unsigned int getNumParms() { return getNumArgs() - SPIRVDebug::Operand::TypeFunction::FirstParameterIdx; }
        SPIRVId getParmType(unsigned int i) { return arg<SPIRVId>(i + SPIRVDebug::Operand::TypeFunction::FirstParameterIdx); }
    };

    class OpDebugDeclare : OpDebugInfoBase
    {
    public:
        OpDebugDeclare(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getResultId() { return extInst->getId(); }
        SPIRVId getVar() { return arg<SPIRVId>(SPIRVDebug::Operand::DebugDeclare::DebugLocalVarIdx); }
        SPIRVId getLocalVar() { return arg<SPIRVId>(SPIRVDebug::Operand::DebugDeclare::VariableIdx); }
        SPIRVId getExpression() { return arg<SPIRVId>(SPIRVDebug::Operand::DebugDeclare::ExpressionIdx); }
    };

    class OpDebugValue : OpDebugInfoBase
    {
    public:
        OpDebugValue(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getResultId() { return extInst->getId(); }
        SPIRVId getVar() { return arg<SPIRVId>(SPIRVDebug::Operand::DebugValue::DebugLocalVarIdx); }
        SPIRVId getValueVar() { return arg<SPIRVId>(SPIRVDebug::Operand::DebugValue::ValueIdx); }
        SPIRVId getExpression() { return arg<SPIRVId>(SPIRVDebug::Operand::DebugValue::ExpressionIdx); }
        SPIRVId getIndex(unsigned int i) { return arg<SPIRVId>(SPIRVDebug::Operand::DebugValue::FirstIndexOperandIdx + i); }
    };

    class OpDebugLocalVar : OpDebugInfoBase
    {
    public:
        bool isParamVar() { return getNumArgs() > SPIRVDebug::Operand::LocalVariable::ArgNumberIdx; }
        OpDebugLocalVar(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getResultId() { return extInst->getId(); }
        SPIRVId getName() { return arg<SPIRVId>(SPIRVDebug::Operand::LocalVariable::NameIdx); }
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::LocalVariable::TypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::LocalVariable::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::LocalVariable::LineIdx); }
        SPIRVWord getCol() { return arg<SPIRVWord>(SPIRVDebug::Operand::LocalVariable::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::LocalVariable::ParentIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::LocalVariable::FlagsIdx); }
        SPIRVWord getArgNo() { return arg<SPIRVWord>(SPIRVDebug::Operand::LocalVariable::ArgNumberIdx); }
    };

    class OpDebugInlinedLocalVar : OpDebugInfoBase
    {
    public:
        OpDebugInlinedLocalVar(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getVar() { return arg<SPIRVId>(SPIRVDebug::Operand::InlinedVariable::VariableIdx); }
        SPIRVId getInlinedAt() { return arg<SPIRVId>(SPIRVDebug::Operand::InlinedVariable::InlinedIdx); }
    };

    class OpDebugTypeBasic : OpDebugInfoBase
    {
    public:
        OpDebugTypeBasic(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TypeBasic::NameIdx); }
        uint64_t getSize() { return const_val(SPIRVDebug::Operand::TypeBasic::SizeIdx); }
        SPIRVDebug::EncodingTag getEncoding() { return arg<SPIRVDebug::EncodingTag>(SPIRVDebug::Operand::TypeBasic::EncodingIdx); }
    };

    class OpDebugPtrType : OpDebugInfoBase
    {
    public:
        OpDebugPtrType(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getBaseType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypePointer::BaseTypeIdx); }
        SPIRVWord getStorageClass() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypePointer::StorageClassIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypePointer::FlagsIdx); }
    };

    class OpDebugTypeQualifier : OpDebugInfoBase
    {
    public:
        OpDebugTypeQualifier(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getBaseType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeQualifier::BaseTypeIdx); }
        enum TypeQualifier
        {
            qual_const = 0,
            qual_volatile = 1,
            qual_restrict = 2
        };
        TypeQualifier getQualifier() { return arg<TypeQualifier>(SPIRVDebug::Operand::TypeQualifier::QualifierIdx); }
    };

    class OpDebugTypeArray : OpDebugInfoBase
    {
    public:
        OpDebugTypeArray(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getBaseType() const { return arg<SPIRVId>(SPIRVDebug::Operand::TypeArray::BaseTypeIdx); }
        bool hasLowerBounds() const
        {
            return (getNumArgs() - SPIRVDebug::Operand::TypeArray::ComponentCountIdx) % 2 == 0;
        }
        SPIRVWord getNumDims() const
        {
            unsigned int n = getNumArgs() - SPIRVDebug::Operand::TypeArray::ComponentCountIdx;
            return hasLowerBounds() ? n / 2 : n;
        }
        SPIRVId getDimCount(unsigned int i) const { return arg<SPIRVId>(SPIRVDebug::Operand::TypeArray::ComponentCountIdx + i); }
        SPIRVId getDimLowerBound(unsigned int i) const
        {
            if (hasLowerBounds()) {
                return arg<SPIRVId>(SPIRVDebug::Operand::TypeArray::ComponentCountIdx + getNumDims() + i);
            }
            return SPIRVID_INVALID;
        }
    };

    class OpDebugTypeVector : OpDebugInfoBase
    {
    public:
        OpDebugTypeVector(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getBaseType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeVector::BaseTypeIdx); }
        SPIRVWord getNumComponents() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeVector::ComponentCountIdx); }
    };

    class OpDebugTypeDef : OpDebugInfoBase
    {
    public:
        OpDebugTypeDef(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::Typedef::NameIdx); }
        SPIRVId getBaseType() { return arg<SPIRVId>(SPIRVDebug::Operand::Typedef::BaseTypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::Typedef::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::Typedef::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::Typedef::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::Typedef::ParentIdx); }
    };

    class OpDebugTypeEnum : OpDebugInfoBase
    {
    public:
        OpDebugTypeEnum(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TypeEnum::NameIdx); }
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeEnum::UnderlyingTypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeEnum::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeEnum::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeEnum::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeEnum::ParentIdx); }
        uint64_t getSize() { return const_val(SPIRVDebug::Operand::TypeEnum::SizeIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeEnum::FlagsIdx); }
        std::pair<SPIRVString*, uint64_t> getItem(unsigned int idx)
        {
            return std::make_pair(str((idx * 2) + SPIRVDebug::Operand::TypeEnum::FirstEnumeratorIdx+1),
                const_val((idx * 2) + SPIRVDebug::Operand::TypeEnum::FirstEnumeratorIdx));
        }
        unsigned int getNumItems() { return (getNumArgs() - SPIRVDebug::Operand::TypeEnum::FirstEnumeratorIdx)/2; }
    };

    class OpDebugTypeComposite : OpDebugInfoBase
    {
    public:
        OpDebugTypeComposite(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TypeComposite::NameIdx); }
        SPIRVWord getTag() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeComposite::TagIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeComposite::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeComposite::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeComposite::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeComposite::ParentIdx); }
        SPIRVString* getLinkageName() { return str(SPIRVDebug::Operand::TypeComposite::LinkageNameIdx); }
        uint64_t getSize() { return const_val(SPIRVDebug::Operand::TypeComposite::SizeIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeComposite::FlagsIdx); }
        unsigned int getNumItems() { return (getNumArgs() - SPIRVDebug::Operand::TypeComposite::FirstMemberIdx); }
        SPIRVId getItem(unsigned int i) { return arg<SPIRVId>(i + SPIRVDebug::Operand::TypeComposite::FirstMemberIdx); }
    };

    class OpDebugTypeMember : OpDebugInfoBase
    {
    public:
        OpDebugTypeMember(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TypeMember::NameIdx); }
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeMember::TypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeMember::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeMember::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeMember::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeMember::ParentIdx); }
        uint64_t getOffset() { return const_val(SPIRVDebug::Operand::TypeMember::OffsetIdx); }
        uint64_t getSize() { return const_val(SPIRVDebug::Operand::TypeMember::SizeIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeMember::FlagsIdx); }
        uint64_t getInitConst() { return const_val(SPIRVDebug::Operand::TypeMember::ValueIdx); }
        SPIRVId getInitConstId() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeMember::ValueIdx); }
        bool hasInitConst() { return getNumArgs() > SPIRVDebug::Operand::TypeMember::MinOperandCount; }
    };

    class OpDebugTypeInheritance : OpDebugInfoBase
    {
    public:
        OpDebugTypeInheritance(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getChild() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeInheritance::ChildIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeInheritance::ParentIdx); }
        uint64_t getOffset() { return const_val(SPIRVDebug::Operand::TypeInheritance::OffsetIdx); }
        uint64_t getSize() { return const_val(SPIRVDebug::Operand::TypeInheritance::SizeIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::TypeInheritance::FlagsIdx); }
    };

    class OpDebugPtrToMember : OpDebugInfoBase
    {
    public:
        OpDebugPtrToMember(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::PtrToMember::MemberTypeIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::PtrToMember::ParentIdx); }
    };

    class OpDebugGlobalVar : OpDebugInfoBase
    {
    public:
        OpDebugGlobalVar(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::GlobalVariable::NameIdx); }
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::GlobalVariable::TypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::GlobalVariable::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::GlobalVariable::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::GlobalVariable::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::GlobalVariable::ParentIdx); }
        SPIRVString* getLinkageName() { return str(SPIRVDebug::Operand::GlobalVariable::LinkageNameIdx); }
        SPIRVId getVariable() { return arg<SPIRVId>(SPIRVDebug::Operand::GlobalVariable::VariableIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::GlobalVariable::FlagsIdx); }
        bool hasStaticDecl() { return getNumArgs() > SPIRVDebug::Operand::GlobalVariable::MinOperandCount; }
        SPIRVId getStaticMemberDecl() { return arg<SPIRVId>(SPIRVDebug::Operand::GlobalVariable::StaticMemberDeclarationIdx); }
    };

    class OpDebugFuncDecl : OpDebugInfoBase
    {
    public:
        OpDebugFuncDecl(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::FunctionDeclaration::NameIdx); }
        SPIRVId getType() { return arg<SPIRVId>(SPIRVDebug::Operand::FunctionDeclaration::TypeIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::FunctionDeclaration::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::FunctionDeclaration::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::FunctionDeclaration::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::FunctionDeclaration::ParentIdx); }
        SPIRVString* getLinkageName() { return str(SPIRVDebug::Operand::FunctionDeclaration::LinkageNameIdx); }
        SPIRVWord getFlags() { return arg<SPIRVWord>(SPIRVDebug::Operand::FunctionDeclaration::FlagsIdx); }
    };

    class OpDebugLexicalBlkDiscriminator : OpDebugInfoBase
    {
    public:
        OpDebugLexicalBlkDiscriminator(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::LexicalBlockDiscriminator::SourceIdx); }
        SPIRVWord getDiscriminator() { return arg<SPIRVWord>(SPIRVDebug::Operand::LexicalBlockDiscriminator::DiscriminatorIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::LexicalBlockDiscriminator::ParentIdx); }
    };

    class OpDebugScope : OpDebugInfoBase
    {
    public:
        OpDebugScope(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getScope() { return arg<SPIRVId>(SPIRVDebug::Operand::Scope::ScopeIdx); }
        bool hasInlinedAt() { return getNumArgs() > SPIRVDebug::Operand::Scope::MinOperandCount; }
        SPIRVId getInlinedAt() { return arg<SPIRVId>(SPIRVDebug::Operand::Scope::InlinedAtIdx); }
    };

    class OpDebugSource : OpDebugInfoBase
    {
    public:
        OpDebugSource(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getFileStr() { return str(SPIRVDebug::Operand::Source::FileIdx); }
    };

    class OpDebugInlinedAt : OpDebugInfoBase
    {
    public:
        OpDebugInlinedAt(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::InlinedAt::LineIdx); }
        SPIRVId getScope() { return arg<SPIRVId>(SPIRVDebug::Operand::InlinedAt::ScopeIdx); }
        bool inlinedAtPresent() { return getNumArgs() > SPIRVDebug::Operand::InlinedAt::MinOperandCount; }
        SPIRVId getInlinedAt() { return arg<SPIRVId>(SPIRVDebug::Operand::InlinedAt::InlinedIdx); }
    };

    class OpDebugTypeTemplate : OpDebugInfoBase
    {
    public:
        OpDebugTypeTemplate(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getTarget() { return arg<SPIRVId>(SPIRVDebug::Operand::Template::TargetIdx); }
        SPIRVId getParm(unsigned int i) { return arg<SPIRVId>(i+SPIRVDebug::Operand::Template::FirstParameterIdx); }
        unsigned int getNumParms() { return (getNumArgs() - SPIRVDebug::Operand::Template::FirstParameterIdx); }
    };

    class OpDebugTypeTemplateParm : OpDebugInfoBase
    {
    public:
        OpDebugTypeTemplateParm(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TemplateParameter::NameIdx); }
        SPIRVId getActualType() { return arg<SPIRVId>(SPIRVDebug::Operand::TemplateParameter::TypeIdx); }
        bool hasValue() { return const_val(SPIRVDebug::Operand::TemplateParameter::ValueIdx) != (uint64_t)-1; }
        uint64_t getValue() { return const_val(SPIRVDebug::Operand::TemplateParameter::ValueIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::TemplateParameter::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::TemplateParameter::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::TemplateParameter::ColumnIdx); }
    };

    class OpDebugTypeTemplateParmPack : OpDebugInfoBase
    {
    public:
        OpDebugTypeTemplateParmPack(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TemplateParameterPack::NameIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::TemplateParameterPack::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::TemplateParameterPack::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::TemplateParameterPack::ColumnIdx); }
        SPIRVId getParm(unsigned int i) { return arg<SPIRVId>(i + SPIRVDebug::Operand::TemplateParameterPack::FirstParameterIdx); }
        unsigned int getNumParms() { return (getNumArgs() - SPIRVDebug::Operand::TemplateParameterPack::FirstParameterIdx); }

    };

    class OpDebugTypeTemplateTemplateParm : OpDebugInfoBase
    {
    public:
        OpDebugTypeTemplateTemplateParm(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVString* getName() { return str(SPIRVDebug::Operand::TemplateTemplateParameter::NameIdx); }
        SPIRVString* getTemplateName() { return str(SPIRVDebug::Operand::TemplateTemplateParameter::TemplateNameIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::TemplateTemplateParameter::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::TemplateTemplateParameter::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVWord>(SPIRVDebug::Operand::TemplateTemplateParameter::ColumnIdx); }
    };

    class OpDebugExpression : OpDebugInfoBase
    {
    public:
        OpDebugExpression(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getOperation(unsigned int idx) { return arg<SPIRVId>(idx); }
        unsigned int getNumOperations() { return getNumArgs(); }
    };

    class OpDebugOperation : OpDebugInfoBase
    {
    public:
        OpDebugOperation(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        unsigned int getNumLiterals() { return getNumArgs() - SPIRVDebug::Operand::Operation::OpCodeIdx - 1; }
        unsigned int getLiteral(unsigned int idx) { return arg<SPIRVWord>(idx + SPIRVDebug::Operand::Operation::OpCodeIdx); }
        SPIRVWord getOperation() { return arg<SPIRVWord>(SPIRVDebug::Operand::Operation::OpCodeIdx); }
    };

    class OpDebugModuleINTEL : OpDebugInfoBase
    {
    public:
        OpDebugModuleINTEL(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getName() { return arg<SPIRVId>(SPIRVDebug::Operand::ModuleINTEL::NameIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::ModuleINTEL::SourceIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::ModuleINTEL::LineIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::ModuleINTEL::ParentIdx); }
        SPIRVId getConfigurationMacros() { return arg<SPIRVId>(SPIRVDebug::Operand::ModuleINTEL::ConfigurationMacrosIdx); }
        SPIRVId getIncludePath() { return arg<SPIRVId>(SPIRVDebug::Operand::ModuleINTEL::IncludePathIdx); }
        SPIRVId getAPINotesFile() { return arg<SPIRVId>(SPIRVDebug::Operand::ModuleINTEL::APINotesFileIdx); }
        SPIRVWord getIsDecl() { return arg<SPIRVWord>(SPIRVDebug::Operand::ModuleINTEL::IsDeclIdx); }
    };

    class OpDebugImportedEntity : OpDebugInfoBase
    {
    public:
        OpDebugImportedEntity(SPIRVExtInst* extInst) : OpDebugInfoBase(extInst) {}
        SPIRVId getName() { return arg<SPIRVId>(SPIRVDebug::Operand::ImportedEntity::NameIdx); }
        SPIRVWord getTag() { return arg<SPIRVId>(SPIRVDebug::Operand::ImportedEntity::TagIdx); }
        SPIRVId getSource() { return arg<SPIRVId>(SPIRVDebug::Operand::ImportedEntity::SourceIdx); }
        SPIRVWord getEntity() { return arg<SPIRVWord>(SPIRVDebug::Operand::ImportedEntity::EntityIdx); }
        SPIRVWord getLine() { return arg<SPIRVWord>(SPIRVDebug::Operand::ImportedEntity::LineIdx); }
        SPIRVWord getColumn() { return arg<SPIRVId>(SPIRVDebug::Operand::ImportedEntity::ColumnIdx); }
        SPIRVId getParent() { return arg<SPIRVId>(SPIRVDebug::Operand::ImportedEntity::ParentIdx); }
    };
}

using namespace llvm;

inline igc_spv::SpvSourceLanguage convertDWARFSourceLangToSPIRV(dwarf::SourceLanguage DwarfLang) {
    switch (DwarfLang) {
        // When updating this function, make sure to also
        // update convertSPIRVSourceLangToDWARF()

        // LLVM does not yet define DW_LANG_C_plus_plus_17
        // case dwarf::SourceLanguage::DW_LANG_C_plus_plus_17:
    case dwarf::SourceLanguage::DW_LANG_C_plus_plus_14:
    case dwarf::SourceLanguage::DW_LANG_C_plus_plus:
        return igc_spv::SpvSourceLanguage::SpvSourceLanguageCPP_for_OpenCL;
    case dwarf::SourceLanguage::DW_LANG_C99:
    case dwarf::SourceLanguage::DW_LANG_OpenCL:
        return igc_spv::SpvSourceLanguage::SpvSourceLanguageOpenCL_C;
    default:
        return igc_spv::SpvSourceLanguage::SpvSourceLanguageUnknown;
    }
}

inline dwarf::SourceLanguage convertSPIRVSourceLangToDWARF(unsigned SourceLang) {
    switch (SourceLang) {
        // When updating this function, make sure to also
        // update convertDWARFSourceLangToSPIRV()
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageOpenCL_CPP:
        return dwarf::SourceLanguage::DW_LANG_C_plus_plus_14;
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageCPP_for_OpenCL:
        // LLVM does not yet define DW_LANG_C_plus_plus_17
        // SourceLang = dwarf::SourceLanguage::DW_LANG_C_plus_plus_17;
        return dwarf::SourceLanguage::DW_LANG_C_plus_plus_14;
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageOpenCL_C:
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageESSL:
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageGLSL:
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageHLSL:
    case igc_spv::SpvSourceLanguage::SpvSourceLanguageUnknown:
        return dwarf::DW_LANG_OpenCL;
    default:
        // Workaround on frontends generating SPIR-V out of SpvSourceLanguage scope.
        return (dwarf::SourceLanguage)SourceLang;
    }
}

#endif

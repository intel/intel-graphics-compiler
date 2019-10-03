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


======================= end_copyright_notice ==================================*///
//===----------------------------------------------------------------------===//
/// \file
///
/// This file defines Debug Info ext instruction class for SPIR-V.
///
//===----------------------------------------------------------------------===//

#ifndef SPIRVDEBUGINFOEXT_HPP_
#define SPIRVDEBUGINFOEXT_HPP_

#include "SPIRVEntry.h"
#include "SPIRVInstruction.h"
#include "SPIRVExtInst.h"

namespace spv {

    // SPIRVDebug is shared with common clang's SPIRV emitter
    namespace SPIRVDebug {

        const unsigned int DebugInfoVersion = 10000;

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
            TypeTemplateParameterPack = 16,
            TypeTemplateTemplateParameter = 17,
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
            InstCount = 36
        };

        enum Flag {
            FlagIsPrivate = 1 << 0,
            FlagIsProtected = 1 << 1,
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
            Fragment = 9
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

        } // namespace Operand
    } // namespace SPIRVDebug

    // SPIRV Debug info class containers. These classes are light-weight as they
    // only help interpret a given SPIRVExtInst during SPIRV->LLVM IR translation.
    class OpDebugInfoBase
    {
    public:
        OpDebugInfoBase(SPIRVExtInst* i)
        {
            extInst = i;
        }
        bool isOpDebugInfo() { return true; }

    protected:
        SPIRVExtInst* extInst = nullptr;
        template <typename T>
        T arg(unsigned int id)
        {
            return static_cast<T>(extInst->getArguments()[id]);
        }
        SPIRVString* str(SPIRVId id)
        {
            auto item = extInst->getModule()->getEntry(arg<SPIRVId>(id));
            if (item->isString())
                return static_cast<SPIRVString*>(item);
            else
                return nullptr;
        }
        uint64_t const_val(SPIRVId id)
        {
            auto item = extInst->getModule()->getEntry(arg<SPIRVId>(id));
            if (item->isConstant())
                return static_cast<SPIRVConstant*>(item)->getZExtIntValue();
            else
                return (uint64_t)-1;
        }
        unsigned int getNumArgs() { return extInst->getArguments().size(); }
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
        SPIRVId getBaseType() { return arg<SPIRVId>(SPIRVDebug::Operand::TypeArray::BaseTypeIdx); }
        SPIRVWord getNumDims() { return (getNumArgs() - SPIRVDebug::Operand::TypeArray::ComponentCountIdx); }
        SPIRVId getComponentCount(unsigned int i) { return arg<SPIRVId>(i + SPIRVDebug::Operand::TypeArray::ComponentCountIdx); }
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
}
#endif

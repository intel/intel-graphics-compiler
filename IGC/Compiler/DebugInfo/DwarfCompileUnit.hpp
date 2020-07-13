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
// This file is based on llvm-3.4\lib\CodeGen\AsmPrinter\DwarfCompilerUnit.h
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "llvm/Config/llvm-config.h"

#include "Compiler/DebugInfo/DIE.hpp"
#include "Compiler/DebugInfo/DwarfDebug.hpp"
#include "Compiler/DebugInfo/Version.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DebugInfo.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class MachineLocation;
    class ConstantInt;
    class ConstantFP;
    class MCExpr;
}

namespace IGC
{
    class DbgVariable;

    //===----------------------------------------------------------------------===//
    /// CompileUnit - This dwarf writer support class manages information associated
    /// with a source file.
    class CompileUnit {
        /// UniqueID - a numeric ID unique among all CUs in the module
        ///
        unsigned UniqueID;

        /// Node - llvm::MDNode for the compile unit.
        llvm::DICompileUnit* Node;

        /// CUDie - Compile unit debug information entry.
        ///
        DIE* CUDie;

        /// Asm - Target of Dwarf emission.
        StreamEmitter* Asm;

        // Holders for some common dwarf information.
        IGC::DwarfDebug* DD;

        /// IndexTyDie - An anonymous type for index type.  Owned by CUDie.
        DIE* IndexTyDie;

        /// MDNodeToDieMap - Tracks the mapping of unit level debug information
        /// variables to debug information entries.
        llvm::DenseMap<const llvm::MDNode*, DIE*> MDNodeToDieMap;

        /// MDNodeToDIEEntryMap - Tracks the mapping of unit level debug information
        /// descriptors to debug information entries using a DIEEntry proxy.
        llvm::DenseMap<const llvm::MDNode*, DIEEntry*> MDNodeToDIEEntryMap;

        /// DIEBlocks - A list of all the DIEBlocks in use.
        std::vector<DIEBlock*> DIEBlocks;

        /// DIEInlinedStrings - A list of all the DIEInlinedStrings in use.
        std::vector<DIEInlinedString*> DIEInlinedStrings;

        /// ContainingTypeMap - This map is used to keep track of subprogram DIEs that
        /// need DW_AT_containing_type attribute. This attribute points to a DIE that
        /// corresponds to the llvm::MDNode mapped with the subprogram DIE.
        llvm::DenseMap<DIE*, const llvm::MDNode*> ContainingTypeMap;

        // DIEValueAllocator - All DIEValues are allocated through this allocator.
        llvm::BumpPtrAllocator DIEValueAllocator;

        // DIEIntegerOne - A preallocated DIEValue because 1 is used frequently.
        DIEInteger* DIEIntegerOne;

    public:
        CompileUnit(unsigned UID, DIE* D, llvm::DICompileUnit* CU,
            StreamEmitter* A, IGC::DwarfDebug* DW);
        ~CompileUnit();

        // Accessors.
        unsigned getUniqueID() const { return UniqueID; }
        uint16_t getLanguage() const { return (uint16_t)Node->getSourceLanguage(); }
        llvm::DICompileUnit* getNode() const { return Node; }
        DIE* getCUDie() const { return CUDie; }

        unsigned getDebugInfoOffset() const { return DebugInfoOffset; }
        void setDebugInfoOffset(unsigned DbgInfoOff) { DebugInfoOffset = DbgInfoOff; }

        /// hasContent - Return true if this compile unit has something to write out.
        ///
        bool hasContent() const { return !CUDie->getChildren().empty(); }

        /// getParentContextString - Get a string containing the language specific
        /// context for a global name.
        std::string getParentContextString(llvm::DIScope* Context) const;

        /// getDIE - Returns the debug information entry map slot for the
        /// specified debug variable. We delegate the request to DwarfDebug
        /// when the llvm::MDNode can be part of the type system, since DIEs for
        /// the type system can be shared across CUs and the mappings are
        /// kept in DwarfDebug.
        DIE* getDIE(llvm::DINode* D) const;

        DIEBlock* getDIEBlock() { return new (DIEValueAllocator)DIEBlock(); }

        /// insertDIE - Insert DIE into the map. We delegate the request to DwarfDebug
        /// when the llvm::MDNode can be part of the type system, since DIEs for
        /// the type system can be shared across CUs and the mappings are
        /// kept in DwarfDebug.
        void insertDIE(llvm::MDNode* Desc, DIE* D);

        /// addDie - Adds or interns the DIE to the compile unit.
        ///
        void addDie(DIE* Buffer) { CUDie->addChild(Buffer); }

        /// addFlag - Add a flag that is true to the DIE.
        void addFlag(DIE* Die, llvm::dwarf::Attribute Attribute);

        /// addUInt - Add an unsigned integer attribute data and value.
        ///
        void addUInt(DIE* Die, llvm::dwarf::Attribute Attribute, llvm::Optional<llvm::dwarf::Form> Form,
            uint64_t Integer);

        void addUInt(DIEBlock* Block, llvm::dwarf::Form Form, uint64_t Integer);

        /// addSInt - Add an signed integer attribute data and value.
        ///
        void addSInt(DIE* Die, llvm::dwarf::Attribute Attribute, llvm::Optional<llvm::dwarf::Form> Form,
            int64_t Integer);

        void addSInt(DIEBlock* Die, llvm::Optional<llvm::dwarf::Form> Form, int64_t Integer);

        /// addString - Add a string attribute data and value.
        ///
        void addString(DIE* Die, llvm::dwarf::Attribute Attribute, const llvm::StringRef Str);

        /// addExpr - Add a Dwarf expression attribute data and value.
        ///
        void addExpr(DIEBlock* Die, llvm::dwarf::Form Form, const llvm::MCExpr* Expr);

        /// addLabel - Add a Dwarf label attribute data and value.
        ///
        void addLabel(DIE* Die, llvm::dwarf::Attribute Attribute, llvm::dwarf::Form Form,
            const llvm::MCSymbol* Label);

        void addLabel(DIEBlock* Die, llvm::dwarf::Form Form, const llvm::MCSymbol* Label);

        /// addLabelAddress - Add a dwarf label attribute data and value using
        /// either DW_FORM_addr or DW_FORM_GNU_addr_index.
        ///
        void addLabelAddress(DIE* Die, llvm::dwarf::Attribute Attribute, llvm::MCSymbol* Label);

        /// addOpAddress - Add a dwarf op address data and value using the
        /// form given and an op of either DW_FORM_addr or DW_FORM_GNU_addr_index.
        ///
        void addOpAddress(DIEBlock* Die, const llvm::MCSymbol* Label);

        /// addDelta - Add a label delta attribute data and value.
        ///
        void addDelta(DIE* Die, llvm::dwarf::Attribute Attribute, llvm::dwarf::Form Form, const llvm::MCSymbol* Hi,
            const llvm::MCSymbol* Lo);

        /// addDIEEntry - Add a DIE attribute data and value.
        ///
        void addDIEEntry(DIE* Die, llvm::dwarf::Attribute Attribute, DIE* Entry);

        /// addDIEEntry - Add a DIE attribute data and value.
        ///
        void addDIEEntry(DIE* Die, llvm::dwarf::Attribute Attribute, DIEEntry* Entry);

        /// addBlock - Add block data.
        ///
        void addBlock(DIE* Die, llvm::dwarf::Attribute Attribute, DIEBlock* Block);

        /// addSourceLine - Add location information to specified debug information
        /// entry.
        void addSourceLine(DIE* Die, llvm::DIScope* S, unsigned Line);
        void addSourceLine(DIE* Die, llvm::DIVariable* V);
        void addSourceLine(DIE* Die, llvm::DISubprogram* SP);
        void addSourceLine(DIE* Die, llvm::DIType* Ty);
#if LLVM_VERSION_MAJOR == 4
        void addSourceLine(DIE * Die, llvm::DINamespace * NS);
#endif

        /// addConstantValue - Add constant value entry in variable DIE.
        void addConstantValue(DIE* Die, const llvm::ConstantInt* CI, bool Unsigned);
        void addConstantValue(DIE* Die, const llvm::APInt& Val, bool Unsigned);

        /// addConstantFPValue - Add constant value entry in variable DIE.
        void addConstantFPValue(DIE* Die, const llvm::ConstantFP* CFP);

        /// addConstantData - Add constant data entry in variable DIE.
        void addConstantData(DIE* Die, const unsigned char* Ptr8, int NumBytes);

        /// addTemplateParams - Add template parameters in buffer.
        void addTemplateParams(DIE& Buffer, llvm::DINodeArray TParams);

        ///  addRegisterLoc - Decide whether to emit regx or bregx
        void addRegisterLoc(DIEBlock* TheDie, unsigned DWReg, int64_t Offset, const llvm::Instruction* dbgInst);

        /// addRegisterOp - Add register operand.
        void addRegisterOp(DIEBlock* TheDie, unsigned Reg);

        /// addRegisterOffset - Add register offset.
        void addRegisterOffset(DIEBlock* TheDie, unsigned Reg, int64_t Offset);

        /// addType - Add a new type attribute to the specified entity. This takes
        /// and attribute parameter because DW_AT_friend attributes are also
        /// type references.
        void addType(DIE* Entity, llvm::DIType* Ty, llvm::dwarf::Attribute Attribute = llvm::dwarf::DW_AT_type);

        // addSimdWidth - add SIMD width
        void addSimdWidth(DIE* Die, uint16_t SimdWidth);

        // addGTRelativeLocation - add a sequence of attributes to calculate either:
        // - BTI-relative location of variable in surface state, or
        // - stateless surface location, or
        // - bindless surface location or
        // - bindless sampler location
        void addGTRelativeLocation(DIEBlock* Block, VISAVariableLocation* Loc);

        // addBindlessOrStatelessLocation - add a sequence of attributes to calculate stateless or
        // bindless location of variable. baseAddr is one of the following base addreses:
        // - General State Base Address when variable located in stateless surface
        // - Bindless Surface State Base Address when variable located in bindless surface
        // - Bindless Sampler State Base Addres when variable located in bindless sampler
        void addBindlessOrStatelessLocation(DIEBlock* Block, VISAVariableLocation* Loc, uint32_t baseAddr);

        // addStatelessLocation - add a sequence of attributes to calculate stateless surface location of variable
        void addStatelessLocation(DIEBlock* Block, VISAVariableLocation* Loc);

        // addBindlessSurfaceLocation - add a sequence of attributes to calculate bindless surface location of variable
        void addBindlessSurfaceLocation(DIEBlock* Block, VISAVariableLocation* Loc);

        // addBindlessSamplerLocation - add a sequence of attributes to calculate bindless sampler location of variable
        void addBindlessSamplerLocation(DIEBlock* Block, VISAVariableLocation* Loc);

        // addScratchLocation - add a sequence of attributes to emit scratch space location
        // of variable
        void addScratchLocation(DIEBlock* Block, DbgDecoder::VarInfo* varInfo, int32_t vectorOffset);

        // addSLMLocation - add a sequence of attributes to emit SLM location of variable
        void addSLMLocation(DIEBlock* Block, VISAVariableLocation* Loc);

        // addSimdLane - add a sequence of attributes to calculate location of variable
        // among SIMD lanes, e.g. a GRF subregister.
        void addSimdLane(DIEBlock* Block, DbgVariable& DV, VISAVariableLocation *Loc, bool isPacked);

        // addSimdLaneScalar - add a sequence of attributes to calculate location of scalar variable
        // e.g. a GRF subregister.
        void addSimdLaneScalar(DIEBlock* Block, DbgVariable& DV, VISAVariableLocation* Loc, uint16_t subRegInBytes);

        /// getOrCreateNameSpace - Create a DIE for DINameSpace.
        DIE* getOrCreateNameSpace(llvm::DINamespace* NS);

        /// getOrCreateSubprogramDIE - Create new DIE using SP.
        DIE* getOrCreateSubprogramDIE(llvm::DISubprogram* SP);

        /// getOrCreateTypeDIE - Find existing DIE or create new DIE for the
        /// given llvm::DIType.
        DIE* getOrCreateTypeDIE(const llvm::MDNode* N);

        /// getOrCreateContextDIE - Get context owner's DIE.
        DIE* getOrCreateContextDIE(llvm::DIScope* Context);

        /// constructContainingTypeDIEs - Construct DIEs for types that contain
        /// vtables.
        void constructContainingTypeDIEs();

        /// constructVariableDIE - Construct a DIE for the given DbgVariable.
        DIE* constructVariableDIE(DbgVariable& DV, bool isScopeAbstract);

        /// Create a DIE with the given Tag, add the DIE to its parent, and
        /// call insertDIE if MD is not null.
        DIE* createAndAddDIE(unsigned Tag, DIE& Parent, llvm::DINode* N = nullptr);

        /// Compute the size of a header for this unit, not including the initial
        /// length field.
        unsigned getHeaderSize() const
        {
            return sizeof(int16_t) + // DWARF version number
                sizeof(int32_t) + // Offset Into Abbrev. Section
                sizeof(int8_t);   // Pointer Size (in bytes)
        }

        /// Emit the header for this unit, not including the initial length field.
        void emitHeader(const llvm::MCSection* ASection, const llvm::MCSymbol* ASectionSym);

    private:
        /// constructTypeDIE - Construct basic type die from llvm::DIBasicType.
        void constructTypeDIE(DIE& Buffer, llvm::DIBasicType* BTy);

        /// constructTypeDIE - Construct derived type die from llvm::DIDerivedType.
        void constructTypeDIE(DIE& Buffer, llvm::DIDerivedType* DTy);

        /// constructTypeDIE - Construct type DIE from DICompositeType.
        void constructTypeDIE(DIE& Buffer, llvm::DICompositeType* CTy);

        /// constructTypeDIE - Construct type DIE from DISubroutineType
        /// This was added for LLVM 3.8 since DISubroutineType is no longer
        /// derived from DICompositeType.
        void constructTypeDIE(DIE& Buffer, llvm::DISubroutineType* STy);

        /// constructSubrangeDIE - Construct subrange DIE from DISubrange.
        void constructSubrangeDIE(DIE& Buffer, llvm::DISubrange* SR, DIE* IndexTy);

        /// constructArrayTypeDIE - Construct array type DIE from DICompositeType.
        void constructArrayTypeDIE(DIE& Buffer, llvm::DICompositeType* CTy);

        /// constructEnumTypeDIE - Construct enum type DIE from DIEnumerator.
        void constructEnumTypeDIE(DIE& Buffer, llvm::DICompositeType* CTy);

        /// constructMemberDIE - Construct member DIE from llvm::DIDerivedType.
        void constructMemberDIE(DIE& Buffer, llvm::DIDerivedType* DT);

        /// constructTemplateTypeParameterDIE - Construct new DIE for the given
        /// llvm::DITemplateTypeParameter.
        void constructTemplateTypeParameterDIE(DIE& Buffer,
            llvm::DITemplateTypeParameter* TP);

        /// constructTemplateValueParameterDIE - Construct new DIE for the given
        /// llvm::DITemplateValueParameter.
        void constructTemplateValueParameterDIE(DIE& Buffer,
            llvm::DITemplateValueParameter* TVP);

        /// getOrCreateStaticMemberDIE - Create new static data member DIE.
        DIE* getOrCreateStaticMemberDIE(llvm::DIDerivedType* DT);

        /// Offset of the CUDie from beginning of debug info section.
        unsigned DebugInfoOffset;

        /// getLowerBoundDefault - Return the default lower bound for an array. If the
        /// DWARF version doesn't handle the language, return -1.
        int64_t getDefaultLowerBound() const;

        /// getDIEEntry - Returns the debug information entry for the specified
        /// debug variable.
        DIEEntry* getDIEEntry(const llvm::MDNode* N) const
        {
            return MDNodeToDIEEntryMap.lookup(N);
        }

        /// insertDIEEntry - Insert debug information entry into the map.
        void insertDIEEntry(const llvm::MDNode* N, DIEEntry* E)
        {
            MDNodeToDIEEntryMap.insert(std::make_pair(N, E));
        }

        // getIndexTyDie - Get an anonymous type for index type.
        DIE* getIndexTyDie() { return IndexTyDie; }

        // setIndexTyDie - Set D as anonymous type for index which can be reused
        // later.
        void setIndexTyDie(DIE* D) { IndexTyDie = D; }

        /// createDIEEntry - Creates a new DIEEntry to be a proxy for a debug
        /// information entry.
        DIEEntry* createDIEEntry(DIE* Entry);

        /// resolve - Look in the DwarfDebug map for the llvm::MDNode that
        /// corresponds to the reference.
#if LLVM_VERSION_MAJOR <= 8
        template <typename T> T * resolve(llvm::TypedDINodeRef<T> Ref) const
        {
            return DD->resolve(Ref);
        }
#else
        template <typename T> inline T* resolve(T* Ref) const
        {
            return DD->resolve(Ref);
        }
#endif

        // Added for 1-step elf
        void buildLocation(const llvm::Instruction*, IGC::DbgVariable&, IGC::DIE*);
        void buildPointer(DbgVariable&, DIE*, VISAVariableLocation*);
        void buildSampler(DbgVariable&, DIE*, VISAVariableLocation*);
        void buildSLM(DbgVariable&, DIE*, VISAVariableLocation*);
        void buildGeneral(DbgVariable&, DIE*, VISAVariableLocation*);
    };

} // namespace IGC

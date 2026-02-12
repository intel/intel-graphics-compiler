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

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/STLExtras.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/MD5.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/ADT/StringExtras.h"
// clang-format on

#include <llvmWrapper/ADT/Optional.h>
#include "DwarfDebug.hpp"
#include "DIE.hpp"
#include "DwarfCompileUnit.hpp"
#include "StreamEmitter.hpp"
#include "Utils.hpp"
#include "VISADebugInfo.hpp"
#include "VISAModule.hpp"

#include <list>
#include <optional>
#include <unordered_set>

#include "Probe/Assertion.h"

#define DEBUG_TYPE "dwarfdebug"

using namespace llvm;
using namespace IGC;

//===----------------------------------------------------------------------===//

// Configuration values for initial hash set sizes (log2).
//
static const unsigned InitAbbreviationsSetSize = 9; // log2(512)

const char *beginSymbol = ".begin";
const char *endSymbol = ".end";

bool DbgVariable::isBlockByrefVariable() const {
  // isBlockByrefStruct is no more support by LLVM10 IR - more info in this
  // commit below:
  // https://github.com/llvm/llvm-project/commit/0779dffbd4a927d7bf9523482481248c51796907
  return false;
}

static bool IsDebugInst(const llvm::Instruction *Inst) {
  if (!isa<DbgInfoIntrinsic>(Inst))
    return false;
#ifndef NDEBUG
  if (!DbgVariable::IsSupportedDebugInst(Inst)) {
    LLVM_DEBUG(dbgs() << "WARNING! Unsupported DbgInfo Instruction detected:\n"; DbgVariable::dumpDbgInst(Inst));
  }
#endif // NDEBUG
  return true;
}

static const MDNode *GetDebugVariable(const Instruction *Inst) {
  IGC_ASSERT(DbgVariable::IsSupportedDebugInst(Inst));

  if (const auto *DclInst = dyn_cast<DbgDeclareInst>(Inst))
    return DclInst->getVariable();

  if (const DbgValueInst *ValInst = dyn_cast<DbgValueInst>(Inst))
    return ValInst->getVariable();

  return nullptr;
}

bool DbgVariable::IsSupportedDebugInst(const llvm::Instruction *Inst) {
  IGC_ASSERT(Inst);
  return dyn_cast<DbgValueInst>(Inst) || dyn_cast<DbgDeclareInst>(Inst);
}

bool DbgVariable::currentLocationIsImplicit() const {
  const auto *DbgInst = getDbgInst();
  if (!DbgInst)
    return false;
  return DbgInst->getExpression()->isImplicit();
}

bool DbgVariable::currentLocationIsMemoryAddress() const {
  const auto *DbgInst = getDbgInst();
  if (!DbgInst)
    return false;
  return isa<llvm::DbgDeclareInst>(DbgInst);
}

bool DbgVariable::currentLocationIsSimpleIndirectValue() const {
  if (currentLocationIsImplicit())
    return false;

  const auto *DbgInst = getDbgInst();
  if (!isa<llvm::DbgValueInst>(DbgInst))
    return false;
  auto *Expr = DbgInst->getExpression();

  // IMPORTANT: changes here should be in sync with DbgVariable::emitExpression
  Value *IRLocation = IGCLLVM::getVariableLocation(DbgInst);
  if (!IRLocation->getType()->isPointerTy())
    return false;

  if (!Expr->startsWithDeref())
    return false;

  if (!std::all_of(Expr->expr_op_begin(), Expr->expr_op_end(), [](const auto &DIOp) {
        return DIOp.getOp() == dwarf::DW_OP_deref || DIOp.getOp() == dwarf::DW_OP_LLVM_fragment;
      })) {
    // backout if the expression contains something other than deref/fragment
    return false;
  }

  return true;
}

bool DbgVariable::currentLocationIsVector() const {
  const auto *DbgInst = getDbgInst();
  if (!isa<llvm::DbgValueInst>(DbgInst))
    return false;

  Value *IRLocation = IGCLLVM::getVariableLocation(DbgInst);
  if (!IRLocation->getType()->isVectorTy())
    return false;
  return true;
}

void DbgVariable::emitExpression(CompileUnit *CU, IGC::DIEBlock *Block) const {
  IGC_ASSERT(CU);
  IGC_ASSERT(Block);

  const auto *DbgInst = getDbgInst();
  if (!DbgInst)
    return;

  const DIExpression *DIExpr = DbgInst->getExpression();
  llvm::SmallVector<uint64_t, 5> Elements;
  int BitPieceIndex = -1;
  for (auto I = DIExpr->expr_op_begin(), E = DIExpr->expr_op_end(); I != E; ++I) {
    switch (I->getOp()) {
    case dwarf::DW_OP_LLVM_fragment: {
      BitPieceIndex = Elements.size();
      uint64_t offset = I->getArg(0);
      uint64_t size = I->getArg(1);
      Elements.push_back(dwarf::DW_OP_bit_piece);
      Elements.push_back(size);
      Elements.push_back(offset);
      continue;
    }

    case dwarf::DW_OP_LLVM_convert:
      if (I->getArg(1) == dwarf::DW_ATE_unsigned) {
        uint64_t bits = I->getArg(0);
        if (bits < 64) {
          if (bits <= 8)
            Elements.push_back(dwarf::DW_OP_const1u);
          else if (bits <= 16)
            Elements.push_back(dwarf::DW_OP_const2u);
          else if (bits <= 32)
            Elements.push_back(dwarf::DW_OP_const4u);
          else
            Elements.push_back(dwarf::DW_OP_const8u);
          Elements.push_back(((uint64_t)1 << bits) - 1);
          Elements.push_back(dwarf::DW_OP_and);
        }
        continue;
      }
      break;

    default:
      break;
    }
    I->appendToVector(Elements);
  }
  const bool isSimpleIndirect = currentLocationIsSimpleIndirectValue();
  if (isSimpleIndirect)
    // drop OP_deref
    Elements.erase(Elements.begin());
  bool shouldResetStackValue = currentLocationIsImplicit();
  if (shouldResetStackValue && !Elements.empty() && *Elements.rbegin() == dwarf::DW_OP_stack_value) {
    Elements.pop_back();
  }
  const bool isFirstHalf = this->RegType == DbgRegisterType::FirstHalf;
  bool isStackValueNeeded =
      !isSimpleIndirect && !currentLocationIsMemoryAddress() && !currentLocationIsVector() && !isFirstHalf;

  if (isStackValueNeeded) {
    auto InsertPos = Elements.end();

    // For expression with DW_OP_bit_piece, DW_OP_stack_value must be before it.
    if (BitPieceIndex != -1) {
      InsertPos = Elements.begin() + BitPieceIndex;
    }
    Elements.insert(InsertPos, dwarf::DW_OP_stack_value);
    CU->stackValueOffset = 1;
  }

  for (auto elem : Elements) {
    auto BF = DIEInteger::BestForm(false, elem);
    CU->addUInt(Block, BF, elem);
  }
}

unsigned DbgVariable::getRegisterValueSizeInBits(const DwarfDebug *DD) const {
  IGC_ASSERT(getDbgInst() != nullptr);
  // There was a re-design of DbgVariableIntrinsic to suppport DIArgList
  // See: e5d958c45629ccd2f5b5f7432756be1d0fcf052c (~llvm-14)
  // So most likely we'll have to revise the relevant codebase.
  Value *IRLoc = IGCLLVM::getVariableLocation(getDbgInst());
  auto *Ty = IRLoc->getType();
  IGC_ASSERT(Ty->isSingleValueType());

  auto LocationSizeInBits = DD->GetVISAModule()->getTypeSizeInBits(Ty);

  const auto *VisaModule = DD->GetVISAModule();
  const auto GRFSizeInBits = VisaModule->getGRFSizeInBits();
  const auto NumGRF = VisaModule->getNumGRFs();
  const auto MaxGRFSpaceInBits = GRFSizeInBits * NumGRF;

  IGC_ASSERT(MaxGRFSpaceInBits / GRFSizeInBits == NumGRF);

  auto Result = LocationSizeInBits;
  if (LocationSizeInBits > MaxGRFSpaceInBits) {
    LLVM_DEBUG(dbgs() << "Error: location size is " << LocationSizeInBits << " , while only " << MaxGRFSpaceInBits
                      << " bits available! location size truncated.");
    IGC_ASSERT_MESSAGE(false, "reported register location is large than available GRF space!");
    Result = 0;
  }

  if (DD->getEmitterSettings().EnableDebugInfoValidation)
    DD->getStreamEmitter().verifyRegisterLocationSize(*this, *DD, MaxGRFSpaceInBits, LocationSizeInBits);

  IGC_ASSERT(Result <= std::numeric_limits<unsigned>::max());
  return static_cast<unsigned>(Result);
}

DIType *DbgVariable::getType() const { return getVariable()->getType(); }

/// Return Dwarf Version by checking module flags.
static unsigned getDwarfVersionFromModule(const Module *M) {
  auto *Val = cast_or_null<ConstantAsMetadata>(M->getModuleFlag("Dwarf Version"));
  if (!Val)
    return dwarf::DWARF_VERSION;

  unsigned Version = cast<ConstantInt>(Val->getValue())->getZExtValue();
  if (Version == 0)
    return dwarf::DWARF_VERSION;

  return Version;
}

void DwarfDISubprogramCache::updateDISPCache(const llvm::Function *F) {
  llvm::DenseSet<const DISubprogram *> DISPToFunction;
  llvm::DenseSet<const MDNode *> Processed;

  if (auto *DISP = F->getSubprogram())
    DISubprograms[F].push_back(DISP);

  for (auto I = llvm::inst_begin(F), E = llvm::inst_end(F); I != E; ++I) {
    auto debugLoc = I->getDebugLoc().get();
    while (debugLoc) {
      auto scope = debugLoc->getScope();
      if (scope && dyn_cast_or_null<llvm::DILocalScope>(scope) && Processed.find(scope) == Processed.end()) {
        auto DISP = cast<llvm::DILocalScope>(scope)->getSubprogram();
        if (DISPToFunction.find(DISP) == DISPToFunction.end()) {
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

DwarfDISubprogramCache::DISubprogramNodes DwarfDISubprogramCache::findNodes(const std::vector<Function *> &Functions) {
  DISubprogramNodes Result;
  // to ensure that Result does not contain duplicates
  std::unordered_set<const llvm::DISubprogram *> UniqueDISP;

  for (const auto *F : Functions) {
    // If we don't have a list of DISP nodes for the processed function -
    // create one and store it in cache
    if (DISubprograms.find(F) == DISubprograms.end())
      updateDISPCache(F);

    const auto &DISPNodes = DISubprograms[F];
    for (auto *DISP : DISPNodes) {
      // we should report only unique DISP nodes
      if (UniqueDISP.find(DISP) != UniqueDISP.end())
        continue;

      Result.push_back(DISP);
      UniqueDISP.insert(DISP);
    }
  }
  return Result;
}
DwarfDebug::DwarfDebug(StreamEmitter *A, VISAModule *M)
    : Asm(A), EmitSettings(Asm->GetEmitterSettings()), m_pModule(M), DISPCache(nullptr), FirstCU(0),
      // AbbreviationsSet(InitAbbreviationsSetSize),
      SourceIdMap(DIEValueAllocator), PrevLabel(nullptr), GlobalCUIndexCount(0), StringPool(DIEValueAllocator),
      NextStringPoolNumber(0), StringPref("info_string") {

  DwarfVersion = getDwarfVersionFromModule(M->GetModule());
  PointerSize = Asm->GetPointerSize();
  // Currently the maximum version of dwarf that LLVM should emit is 4
  if (DwarfVersion > 4)
    Asm->SetDwarfVersion(DwarfVersion);
}

MCSymbol *DwarfDebug::getStringPoolSym() { return Asm->GetTempSymbol(StringPref); }

MCSymbol *DwarfDebug::getStringPoolEntry(StringRef Str) {
  std::pair<MCSymbol *, unsigned> &Entry = StringPool[Str];
  if (!Entry.first) {
    Entry.second = StringPool.size() - 1;
    Entry.first = Asm->GetTempSymbol(StringPref, Entry.second);
  }
  return Entry.first;
}
void DwarfDebug::registerVISA(IGC::VISAModule *M) {
  IGC_ASSERT(M);
  auto *F = M->getFunction();
  // Sanity check that we have only single module associated
  // with a Function
  auto *EM = GetVISAModule(F);
  if (M == EM)
    return;
  // TODO: we need to change this one to assertion statement
  if (EM != nullptr) {
    VISAModToFunc.erase(EM);
  }
  RegisteredFunctions.push_back(F);
  VISAModToFunc[M] = RegisteredFunctions.back();
}

const llvm::Function *DwarfDebug::GetPrimaryEntry() const {
  auto FoundIt = std::find_if(VISAModToFunc.begin(), VISAModToFunc.end(),
                              [](const auto &Item) { return Item.first->isPrimaryFunc(); });
  IGC_ASSERT(FoundIt != VISAModToFunc.end());
  return FoundIt->second;
}

llvm::Function *DwarfDebug::GetFunction(const VISAModule *M) const {
  auto it = VISAModToFunc.find(M);
  if (it != VISAModToFunc.end())
    return (*it).second;
  return nullptr;
}

VISAModule *DwarfDebug::GetVISAModule(const llvm::Function *F) const {
  for (auto &p : VISAModToFunc) {
    if (p.second == F)
      return p.first;
  }
  return nullptr;
}

// Define a unique number for the abbreviation.
//
void DwarfDebug::assignAbbrevNumber(IGC::DIEAbbrev &Abbrev) {
  // Check the set for priors.
  DIEAbbrev *InSet = AbbreviationsSet.GetOrInsertNode(&Abbrev);

  // If it's newly added.
  if (InSet == &Abbrev) {
    // Add to abbreviation list.
    Abbreviations.push_back(&Abbrev);

    // Assign the vector position + 1 as its number.
    Abbrev.setNumber(Abbreviations.size());
  } else {
    // Assign existing abbreviation number.
    Abbrev.setNumber(InSet->getNumber());
  }
}

/// isSubprogramContext - Return true if Context is either a subprogram
/// or another context nested inside a subprogram.
bool DwarfDebug::isSubprogramContext(const MDNode *D) {
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
DIE *DwarfDebug::updateSubprogramScopeDIE(CompileUnit *SPCU, DISubprogram *SP) {
  DIE *SPDie = SPCU->getDIE(SP);

  IGC_ASSERT_MESSAGE(SPDie, "Unable to find subprogram DIE!");

  // If we're updating an abstract DIE, then we will be adding the children and
  // object pointer later on. But what we don't want to do is process the
  // concrete DIE twice.
  if (DIE *AbsSPDIE = AbstractSPDies.lookup(SP)) {
    // Pick up abstract subprogram DIE.
    SPDie = SPCU->createAndAddDIE(dwarf::DW_TAG_subprogram, *SPCU->getCUDie());
    SPCU->addDIEEntry(SPDie, dwarf::DW_AT_abstract_origin, AbsSPDIE);
  } else {
    DISubprogram *SPDecl = SP->getDeclaration();
    if (!SPDecl) {
      // There is not any need to generate specification DIE for a function
      // defined at compile unit level. If a function is defined inside another
      // function then gdb prefers the definition at top level and but does not
      // expect specification DIE in parent function. So avoid creating
      // specification DIE for a function defined inside a function.
      DIScope *SPContext = resolve(SP->getScope());
      if (SP->isDefinition() && SPContext && !isa<DICompileUnit>(SPContext) && !isa<DIFile>(SPContext) &&
          !isSubprogramContext(SPContext)) {
        SPCU->addFlag(SPDie, dwarf::DW_AT_declaration);

        // Add arguments.
        DISubroutineType *SPTy = SP->getType();
        if (SPTy) {
          DITypeRefArray Args = SPTy->getTypeArray();
          uint16_t SPTag = (uint16_t)SPTy->getTag();
          if (SPTag == dwarf::DW_TAG_subroutine_type) {
            for (unsigned i = 1, N = Args.size(); i < N; ++i) {
              DIE *Arg = SPCU->createAndAddDIE(dwarf::DW_TAG_formal_parameter, *SPDie);
              DIType *ATy = cast<DIType>(Args[i]);
              SPCU->addType(Arg, ATy);
              if (ATy->isArtificial())
                SPCU->addFlag(Arg, dwarf::DW_AT_artificial);
              if (ATy->isObjectPointer())
                SPCU->addDIEEntry(SPDie, dwarf::DW_AT_object_pointer, Arg);
            }
          }
          DIE *SPDeclDie = SPDie;
          SPDie = SPCU->createAndAddDIE(dwarf::DW_TAG_subprogram, *SPCU->getCUDie());
          SPCU->addDIEEntry(SPDie, dwarf::DW_AT_specification, SPDeclDie);
        }
      }
    }
  }

  if (m_pModule->getFunction() && m_pModule->getFunction()->getSubprogram() == SP &&
      m_pModule->GetType() == VISAModule::ObjectType::SUBROUTINE) {
    SPCU->addUInt(SPDie, dwarf::DW_AT_calling_convention, dwarf::DW_FORM_data1, dwarf::DW_CC_nocall);
  }

  if (EmitSettings.EnableRelocation) {
    auto Id = m_pModule->GetFuncId();
    SPCU->addLabelAddress(SPDie, dwarf::DW_AT_low_pc, Asm->GetTempSymbol("func_begin", Id));
    SPCU->addLabelAddress(SPDie, dwarf::DW_AT_high_pc, Asm->GetTempSymbol("func_end", Id));
  } else {
    SPCU->addUInt(SPDie, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, lowPc);
    SPCU->addUInt(SPDie, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, highPc);
  }

  return SPDie;
}

/// Check whether we should create a DIE for the given Scope, return true
/// if we don't create a DIE (the corresponding DIE is null).
bool DwarfDebug::isLexicalScopeDIENull(LexicalScope *Scope) {
  if (Scope->isAbstractScope())
    return false;

  // We don't create a DIE if there is no Range.
  const SmallVectorImpl<InsnRange> &Ranges = Scope->getRanges();
  if (Ranges.empty())
    return true;

  if (Ranges.size() > 1)
    return false;

  return false;
}

// Construct new DW_TAG_lexical_block for this scope and attach
// DW_AT_low_pc/DW_AT_high_pc labels as well as DW_AT_INTEL_simd_width.
// Also add DW_AT_abstract_origin when lexical scope is from inlined code.
DIE *DwarfDebug::constructLexicalScopeDIE(CompileUnit *TheCU, LexicalScope *Scope) {
  if (isLexicalScopeDIENull(Scope))
    return 0;

  DIE *ScopeDIE = new DIE(dwarf::DW_TAG_lexical_block);
  if (Scope->isAbstractScope()) {
    AbsLexicalScopeDIEMap.insert(std::make_pair(Scope, ScopeDIE));
    return ScopeDIE;
  }

  const SmallVectorImpl<InsnRange> &Ranges = Scope->getRanges();
  IGC_ASSERT_MESSAGE(Ranges.empty() == false, "LexicalScope does not have instruction markers!");

  if (Scope->getInlinedAt()) {
    auto abstractScope = LScopes.findAbstractScope(dyn_cast_or_null<DILocalScope>(Scope->getScopeNode()));
    if (abstractScope) {
      auto AbsDIE = AbsLexicalScopeDIEMap.lookup(abstractScope);
      if (AbsDIE) {
        // Point to corresponding abstract instance of DW_TAG_lexical_block
        TheCU->addDIEEntry(ScopeDIE, dwarf::DW_AT_abstract_origin, AbsDIE);
      }
    }
  }

  encodeRange(TheCU, ScopeDIE, &Ranges);

  return ScopeDIE;
}

void DwarfDebug::encodeRange(CompileUnit *TheCU, DIE *ScopeDIE, const llvm::SmallVectorImpl<InsnRange> *Ranges) {
  // Attaches gen isa ranges to the provided DIE
  // gen isa ranges are calculated based on the input vISA intervals once
  // vISA intevals are resolved, the respected gen isa ranges are coalesced.
  // Gen isa ranges can be attached either as DW_AT_low_pc/DW_AT_high_pc or
  // as DW_AT_ranges if we have more than one range associated with it.
  // In the latter case, the respected ranges are stored in
  // GenISADebugRangeSymbols (as a pair of <Label, RangesList>)

  auto IsValidRange = [](const InsnRange &R) {
    auto start = R.first;
    auto end = R.second;
    while (end != start && start) {
      if (!llvm::isa<DbgInfoIntrinsic>(start))
        if (start->getDebugLoc())
          return true;

      start = getNextInst(start);
    }
    return false;
  };

  llvm::SmallVector<InsnRange, 5> PrunedRanges;
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
  for (auto &R : *Ranges) {
    if (IsValidRange(R))
      PrunedRanges.push_back(R);
  }

  // This makes sense only for full debug info.
  if (PrunedRanges.size() == 0)
    return;

  // Resolve VISA index to Gen IP here.
  VISAModule::GenISARange AllGenISARanges;
  for (SmallVectorImpl<InsnRange>::const_iterator RI = PrunedRanges.begin(), RE = PrunedRanges.end(); RI != RE; ++RI) {
    auto GenISARanges = m_pModule->getGenISARange(*VisaDbgInfo, *RI);
    for (auto &R : GenISARanges) {
      AllGenISARanges.push_back(R);
    }
  }

  m_pModule->coalesceRanges(AllGenISARanges);

  if (AllGenISARanges.size() == 1) {
    // Emit low_pc/high_pc inlined in DIE
    if (EmitSettings.EnableRelocation) {
      auto StartLabel = GetLabelBeforeIp(AllGenISARanges.front().first);
      auto EndLabel = GetLabelBeforeIp(AllGenISARanges.front().second);
      TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_low_pc, StartLabel);
      TheCU->addLabelAddress(ScopeDIE, dwarf::DW_AT_high_pc, EndLabel);
    } else {
      TheCU->addUInt(ScopeDIE, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, AllGenISARanges.front().first);
      TheCU->addUInt(ScopeDIE, dwarf::DW_AT_high_pc, dwarf::DW_FORM_addr, AllGenISARanges.front().second);
    }
  } else if (AllGenISARanges.size() > 1) {
    // Emit to debug_ranges
    llvm::MCSymbol *NewLabel = nullptr;
    if (EmitSettings.EnableRelocation) {
      NewLabel = Asm->CreateTempSymbol();
      TheCU->addLabelLoc(ScopeDIE, dwarf::DW_AT_ranges, NewLabel);
    } else {
      auto GetDebugRangeSize = [&]() {
        size_t TotalSize = 0;
        for (auto &Entry : GenISADebugRangeSymbols) {
          TotalSize += Entry.second.size();
        }
        return TotalSize;
      };

      TheCU->addUInt(ScopeDIE, dwarf::DW_AT_ranges, dwarf::DW_FORM_sec_offset, GetDebugRangeSize() * PointerSize);
    }

    llvm::SmallVector<unsigned int, 8> Data;

    for (auto &item : AllGenISARanges) {
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
DIE *DwarfDebug::constructInlinedScopeDIE(CompileUnit *TheCU, LexicalScope *Scope) {
  if (!Scope->getScopeNode())
    return NULL;

  const SmallVectorImpl<InsnRange> &Ranges = Scope->getRanges();
  IGC_ASSERT_MESSAGE(Ranges.empty() == false, "LexicalScope does not have instruction markers!");

  const MDNode *DS = Scope->getScopeNode();
  DISubprogram *InlinedSP = getDISubprogram(DS);
  DIE *OriginDIE = TheCU->getDIE(InlinedSP);
  if (!OriginDIE) {
    LLVM_DEBUG(dbgs() << "Unable to find original DIE for an inlined subprogram.");
    return NULL;
  }

  DIE *ScopeDIE = new DIE(dwarf::DW_TAG_inlined_subroutine);

  InlinedSubprogramDIEs.insert(OriginDIE);
  TheCU->addDIEEntry(ScopeDIE, dwarf::DW_AT_abstract_origin, OriginDIE);

  // Add the call site information to the DIE.
  DILocation *DL = cast<DILocation>(const_cast<MDNode *>(Scope->getInlinedAt()));
  unsigned int fileId = getOrCreateSourceID(DL->getFilename(), DL->getDirectory(), getMD5AsBytes(DL->getFile()),
                                            DL->getSource(), TheCU->getUniqueID(), false);
  TheCU->addUInt(ScopeDIE, dwarf::DW_AT_call_file, std::nullopt, fileId);
  TheCU->addUInt(ScopeDIE, dwarf::DW_AT_call_line, std::nullopt, DL->getLine());

  // .debug_range section has not been laid out yet. Emit offset in
  // .debug_range as a uint, size 4, for now. emitDIE will handle
  // DW_AT_ranges appropriately.
  encodeRange(TheCU, ScopeDIE, &Ranges);

  return ScopeDIE;
}

DIE *DwarfDebug::createScopeChildrenDIE(CompileUnit *TheCU, LexicalScope *Scope, SmallVectorImpl<DIE *> &Children) {
  DIE *ObjectPointer = NULL;

  SmallVector<DbgVariable *, 8> dbgVariables;
  // Collect arguments for current function.
  if (LScopes.isCurrentFunctionScope(Scope)) {
    std::copy(CurrentFnArguments.begin(), CurrentFnArguments.end(), std::back_inserter(dbgVariables));
  }

  {
    // Collect lexical scope variables.
    const DbgVariablesVect &Variables = ScopeVariables.lookup(Scope);
    std::copy(Variables.begin(), Variables.end(), std::back_inserter(dbgVariables));
  }

  // Create and collect all argument/variable children
  for (DbgVariable *ArgDV : dbgVariables) {
    if (!ArgDV)
      continue;
    DIE *Arg = TheCU->constructVariableDIE(*ArgDV, Scope->isAbstractScope());
    Children.push_back(Arg);
    if (ArgDV->isObjectPointer())
      ObjectPointer = Arg;
  }

  // Apply attributes to each created DIE. It needs to be done after creating all
  // subprogram DIEs, beacuse we might need reference to them in addType() function.
  for (DbgVariable *ArgDV : dbgVariables) {
    if (!ArgDV)
      continue;
    if (DIE *VarDIE = TheCU->getDIE(const_cast<DILocalVariable *>(ArgDV->getVariable())))
      TheCU->applyVariableAttributes(*ArgDV, VarDIE, Scope->isAbstractScope());
  }

  // There is no need to emit empty lexical block DIE.
  for (auto *constIE : TheCU->ImportedEntities[Scope->getScopeNode()]) {
    llvm::MDNode *MD = const_cast<llvm::MDNode *>(constIE);
    llvm::DIImportedEntity *IE = cast<llvm::DIImportedEntity>(MD);
    DIE *IEDie = TheCU->constructImportedEntityDIE(IE);
    if (IEDie)
      Children.push_back(IEDie);
  }

  const SmallVectorImpl<LexicalScope *> &Scopes = Scope->getChildren();
  for (unsigned j = 0, M = Scopes.size(); j < M; ++j) {
    if (DIE *Nested = constructScopeDIE(TheCU, Scopes[j])) {
      Children.push_back(Nested);
    }
  }

  return ObjectPointer;
}

// Construct a DIE for this scope.
DIE *DwarfDebug::constructScopeDIE(CompileUnit *TheCU, LexicalScope *Scope) {
  if (!Scope || !Scope->getScopeNode())
    return NULL;

  const MDNode *DS = Scope->getScopeNode();

  SmallVector<DIE *, 8> Children;
  DIE *ObjectPointer = NULL;
  bool ChildrenCreated = false;

  // We try to create the scope DIE first, then the children DIEs. This will
  // avoid creating un-used children then removing them later when we find out
  // the scope DIE is null.
  DIE *ScopeDIE = NULL;
  if (isa<DISubprogram>(DS) && Scope->getInlinedAt()) {
    ScopeDIE = constructInlinedScopeDIE(TheCU, Scope);
  } else if (isa<DISubprogram>(DS)) {
    ProcessedSPNodes.insert(DS);
    if (Scope->isAbstractScope()) {
      ScopeDIE = TheCU->getDIE(cast<DINode>(const_cast<MDNode *>(DS)));
      // Note down abstract DIE.
      if (ScopeDIE) {
        AbstractSPDies.insert(std::make_pair(DS, ScopeDIE));
      }
    } else {
      ScopeDIE = updateSubprogramScopeDIE(TheCU, cast<DISubprogram>(const_cast<MDNode *>(DS)));
    }
  } else {
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

  if (!ScopeDIE) {
    IGC_ASSERT_MESSAGE(Children.empty(), "We create children only when the scope DIE is not null.");
    return NULL;
  }
  if (!ChildrenCreated) {
    // We create children when the scope DIE is not null.
    ObjectPointer = createScopeChildrenDIE(TheCU, Scope, Children);
  }

  // Add children
  for (SmallVectorImpl<DIE *>::iterator I = Children.begin(), E = Children.end(); I != E; ++I) {
    ScopeDIE->addChild(*I);
  }

  if (isa<DISubprogram>(DS) && ObjectPointer != NULL) {
    TheCU->addDIEEntry(ScopeDIE, dwarf::DW_AT_object_pointer, ObjectPointer);
  }

  return ScopeDIE;
}

// Look up the source id with the given directory and source file names.
// If none currently exists, create a new id and insert it in the
// SourceIds map. This can update DirectoryNames and SourceFileNames maps
// as well.
unsigned DwarfDebug::getOrCreateSourceID(llvm::StringRef FileName, llvm::StringRef DirName,
                                         IGCLLVM::optional<llvm::MD5::MD5Result> Checksum,
                                         IGCLLVM::optional<llvm::StringRef> Source, unsigned CUID, bool SetRootFile) {
  // If we use .loc in assembly, we can't separate .file entries according to
  // compile units. Thus all files will belong to the default compile unit.

  // If FE did not provide a file name, then assume stdin.
  if (FileName.empty()) {
    return getOrCreateSourceID("<stdin>", StringRef(), Checksum, Source, CUID, false);
  }

  if (DwarfVersion <= 4 && !CompilationDir.empty() && DirName == CompilationDir)
    DirName = "";

  // FileIDCUMap stores the current ID for the given compile unit.
  // DWARF v5: the root file is represented as file entry #0
  unsigned SrcId = SetRootFile ? 0 : FileIDCUMap[CUID] + 1;

  // We look up the CUID/file/dir by concatenating them with a zero byte.
  SmallString<128> NamePair;
  NamePair += utostr(CUID);
  NamePair += '\0';
  NamePair += DirName;
  NamePair += '\0'; // Zero bytes are not allowed in paths.
  NamePair += FileName;

  auto item = SourceIdMap.insert(std::make_pair(NamePair, SrcId));
  if (!item.second) {
    return item.first->second;
  }

  FileIDCUMap[CUID] = SrcId;

  // DWARF v5: the primary source file of the CU is the root file, represented as file entry #0. There is no such
  // distinction for earlier DWARF versions.
  // Therefore, in DWARF v5, we emit .file 0 directive for the primary source file and .file directive for the other
  // files. In older versions, we emit .file directive for all files.
  // The emission itself is needed to specify files for .loc directives.
  if (SetRootFile)
    Asm->EmitDwarfFile0Directive(SrcId, DirName, FileName, Checksum, Source, CUID);
  else
    Asm->EmitDwarfFileDirective(SrcId, DirName, FileName, Checksum, Source, CUID);

  return SrcId;
}

// Create new CompileUnit for the given metadata node with tag
// DW_TAG_compile_unit.
CompileUnit *DwarfDebug::constructCompileUnit(DICompileUnit *DIUnit) {
  StringRef FN = DIUnit->getFilename();
  CompilationDir = DIUnit->getDirectory();
  bool SetRootFile = DwarfVersion >= 5;

  DIE *Die = new DIE(dwarf::DW_TAG_compile_unit);
  CompileUnit *NewCU = new CompileUnit(GlobalCUIndexCount++, Die, DIUnit, Asm, this);

  FileIDCUMap[NewCU->getUniqueID()] = 0;
  // Call this to emit a .file / .file 0 directive if it wasn't emitted for the source
  // file this CU comes from yet.
  getOrCreateSourceID(FN, CompilationDir, getMD5AsBytes(DIUnit->getFile()), DIUnit->getSource(), NewCU->getUniqueID(),
                      SetRootFile);

  auto producer = DIUnit->getProducer();
  auto strProducer = producer.str();
  if (producer.startswith("clang version")) {
    auto pos = strProducer.find("(");
    strProducer = strProducer.substr(0, pos);
    producer = strProducer.data();
  }
  NewCU->addString(Die, dwarf::DW_AT_producer, producer);

  NewCU->addUInt(Die, dwarf::DW_AT_language, dwarf::DW_FORM_data2,
                 getSourceLanguage(DIUnit, GetVISAModule()->GetModule()));
  NewCU->addString(Die, dwarf::DW_AT_name, FN);

  ModuleBeginSym = Asm->GetTempSymbol("module_begin", NewCU->getUniqueID());
  ModuleEndSym = Asm->GetTempSymbol("module_end", NewCU->getUniqueID());

  // Assumes in correct section after the entry point.
  Asm->EmitLabel(ModuleBeginSym);
  // 2.17.1 requires that we use DW_AT_low_pc for a single entry point
  // into an entity. We're using 0 (or a NULL label) for this.

  if (EmitSettings.EnableRelocation) {
    NewCU->addLabelAddress(Die, dwarf::DW_AT_low_pc, ModuleBeginSym);
    NewCU->addLabelAddress(Die, dwarf::DW_AT_high_pc, ModuleEndSym);

    NewCU->addLabelLoc(Die, dwarf::DW_AT_stmt_list, DwarfLineSectionSym);
  } else {
    auto highPC = m_pModule->getUnpaddedProgramSize();
    NewCU->addUInt(Die, dwarf::DW_AT_low_pc, dwarf::DW_FORM_addr, 0);
    NewCU->addUInt(Die, dwarf::DW_AT_high_pc, std::optional<dwarf::Form>(), highPC);

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
  if (!CompilationDir.empty()) {
    NewCU->addString(Die, dwarf::DW_AT_comp_dir, CompilationDir);
  }

  // GD-215:
  // Add API and version
  auto lang = m_pModule->GetModule()->getNamedMetadata("igc.input.ir");
  if (lang && lang->getNumOperands() > 0) {
    auto mdNode = lang->getOperand(0);
    if (mdNode && mdNode->getNumOperands() > 2) {
      auto op0 = dyn_cast_or_null<MDString>(mdNode->getOperand(0));
      auto op1 = dyn_cast_or_null<ConstantAsMetadata>(mdNode->getOperand(1));
      auto op2 = dyn_cast_or_null<ConstantAsMetadata>(mdNode->getOperand(2));

      if (op0 && op1 && op2) {
        if (op0->getString() == "ocl") {
          std::string str;
          str = "Intel OpenCL ";
          str += IGCLLVM::toString(op1->getValue()->getUniqueInteger(), 10, false);
          str += ".";
          str += IGCLLVM::toString(op2->getValue()->getUniqueInteger(), 10, false);

          NewCU->addString(Die, dwarf::DW_AT_description, llvm::StringRef(str));
        }
      }
    }
  }

  if (!FirstCU) {
    FirstCU = NewCU;
  }

  for (auto *IE : DIUnit->getImportedEntities())
    NewCU->addImportedEntity(IE);

  CUs.push_back(NewCU);

  CUMap.insert(std::make_pair(DIUnit, NewCU));
  CUDieMap.insert(std::make_pair(Die, NewCU));
  return NewCU;
}

// Construct subprogram DIE.
void DwarfDebug::constructSubprogramDIE(CompileUnit *TheCU, const MDNode *N) {
  // FIXME: We should only call this routine once, however, during LTO if a
  // program is defined in multiple CUs we could end up calling it out of
  // beginModule as we walk the CUs.

  CompileUnit *&CURef = SPMap[N];
  if (CURef)
    return;
  CURef = TheCU;

  DISubprogram *SP = cast<DISubprogram>(const_cast<MDNode *>(N));
  if (!SP->isDefinition()) {
    // This is a method declaration which will be handled while constructing
    // class type.
    return;
  }

  TheCU->getOrCreateSubprogramDIE(SP);
}

void DwarfDebug::ExtractConstantData(const llvm::Constant *ConstVal, DwarfDebug::DataVector &Result) const {
  IGC_ASSERT(ConstVal);

  if (dyn_cast<ConstantPointerNull>(ConstVal)) {
    DataLayout DL(GetVISAModule()->GetDataLayout());
    Result.insert(Result.end(), DL.getPointerSize(), 0);
  } else if (const ConstantDataSequential *cds = dyn_cast<ConstantDataSequential>(ConstVal)) {
    for (unsigned i = 0; i < cds->getNumElements(); i++) {
      ExtractConstantData(cds->getElementAsConstant(i), Result);
    }
  } else if (const ConstantAggregateZero *cag = dyn_cast<ConstantAggregateZero>(ConstVal)) {
    // Zero aggregates are filled with, well, zeroes.
    DataLayout DL(GetVISAModule()->GetDataLayout());
    const unsigned int zeroSize = (unsigned int)(DL.getTypeAllocSize(cag->getType()));
    Result.insert(Result.end(), zeroSize, 0);
  }
  // If this is an sequential type which is not a CDS or zero, have to collect
  // the values element by element. Note that this is not exclusive with the two
  // cases above, so the order of ifs is meaningful.
  else if (ConstVal->getType()->isVectorTy() || ConstVal->getType()->isArrayTy() || ConstVal->getType()->isStructTy()) {
    const int numElts = ConstVal->getNumOperands();
    for (int i = 0; i < numElts; ++i) {
      Constant *C = ConstVal->getAggregateElement(i);
      IGC_ASSERT_MESSAGE(C, "getAggregateElement returned null, unsupported constant");
      // Since the type may not be primitive, extra alignment is required.
      ExtractConstantData(C, Result);
    }
  }
  // And, finally, we have to handle base types - ints and floats.
  else {
    APInt intVal(32, 0, false);
    if (const ConstantInt *ci = dyn_cast<ConstantInt>(ConstVal)) {
      intVal = ci->getValue();
    } else if (const ConstantFP *cfp = dyn_cast<ConstantFP>(ConstVal)) {
      intVal = cfp->getValueAPF().bitcastToAPInt();
    } else if (isa<UndefValue>(ConstVal)) {
      intVal = llvm::APInt(32, 0, false);
    } else if (const ConstantExpr *cExpr = dyn_cast<ConstantExpr>(ConstVal)) {
      // under some weird and obscure conditions we can and up with
      // constant expressions. Usually this is an indication of
      // a problem in the frontend our poorly-written user code.
      // Handle some cases observed in practice and report a usability issue
      if (cExpr->isCast() && cExpr->getType()->isPointerTy() && cExpr->getOperand(0)->getType()->isIntegerTy()) {
        intVal = cast<ConstantInt>(cExpr->getOperand(0))->getValue();
      } else {
        IGC_ASSERT_MESSAGE(0, "unsupported constant expression type");
      }
      getStreamEmitter().reportUsabilityIssue("unexpected constant expression", cExpr);
    } else {
      IGC_ASSERT_MESSAGE(0, "Unsupported constant type");
    }

    auto bitWidth = intVal.getBitWidth();
    IGC_ASSERT_MESSAGE((0 < bitWidth), "Unsupported bitwidth");
    IGC_ASSERT_MESSAGE((bitWidth % 8 == 0), "Unsupported bitwidth");
    IGC_ASSERT_MESSAGE((bitWidth <= 64), "Unsupported bitwidth");
    auto ByteWidth = bitWidth / 8;
    const char *DataPtr = reinterpret_cast<const char *>(intVal.getRawData());

    Result.insert(Result.end(), DataPtr, DataPtr + ByteWidth);
  }
}

void DwarfDebug::constructThenAddImportedEntityDIE(CompileUnit *TheCU, DIImportedEntity *IE) {
  if (isa_and_nonnull<DILocalScope>(IE->getScope()))
    return;

  if (DIE *D = TheCU->getOrCreateContextDIE(IE->getScope())) {
    DIE *IEDie = TheCU->constructImportedEntityDIE(IE);
    if (IEDie)
      D->addChild(IEDie);
  }
}

void DwarfDebug::discoverDISPNodes(DwarfDISubprogramCache &Cache) {
  IGC_ASSERT(DISubprogramNodes.empty());
  DISubprogramNodes = Cache.findNodes(RegisteredFunctions);
}

void DwarfDebug::discoverDISPNodes() {
  if (DISPCache) {
    discoverDISPNodes(*DISPCache);
  } else {
    DwarfDISubprogramCache TemporaryCache;
    discoverDISPNodes(TemporaryCache);
  }
}

IGCLLVM::optional<llvm::MD5::MD5Result> DwarfDebug::getMD5AsBytes(const llvm::DIFile *File) const {
  IGC_ASSERT(File);

  IGCLLVM::optional<llvm::DIFile::ChecksumInfo<StringRef>> Checksum = File->getChecksum();

  if (!Checksum || Checksum->Kind != llvm::DIFile::CSK_MD5)
    return IGCLLVM::optional<llvm::MD5::MD5Result>();

  // Convert the string checksum to an MD5Result for the streamer.
  // The verifier validates the checksum so we assume it's okay.
  // An MD5 checksum is 16 bytes.
  std::string ChecksumString = fromHex(Checksum->Value);
  llvm::MD5::MD5Result CKMem;
  const size_t MD5NumBytes = 16;
  std::memcpy(&CKMem, ChecksumString.data(), MD5NumBytes);
  return CKMem;
}

/// Sort and unique GVEs by comparing their fragment offset.
static SmallVectorImpl<CompileUnit::GlobalExpr> &sortGlobalExprs(SmallVectorImpl<CompileUnit::GlobalExpr> &GVEs) {
  llvm::sort(GVEs, [](CompileUnit::GlobalExpr A, CompileUnit::GlobalExpr B) {
    // Sort order: first null exprs, then exprs without fragment
    // info, then sort by fragment offset in bits.
    // FIXME: Come up with a more comprehensive comparator so
    // the sorting isn't non-deterministic, and so the following
    // std::unique call works correctly.
    if (!A.Expr || !B.Expr)
      return !!B.Expr;
    auto FragmentA = A.Expr->getFragmentInfo();
    auto FragmentB = B.Expr->getFragmentInfo();
    if (!FragmentA || !FragmentB)
      return !!FragmentB;
    return FragmentA->OffsetInBits < FragmentB->OffsetInBits;
  });
  GVEs.erase(std::unique(GVEs.begin(), GVEs.end(),
                         [](CompileUnit::GlobalExpr A, CompileUnit::GlobalExpr B) { return A.Expr == B.Expr; }),
             GVEs.end());
  return GVEs;
}

// Emit all Dwarf sections that should come prior to the content. Create
// global DIEs and emit initial debug info sections.
void DwarfDebug::beginModule() {
  const Module *M = m_pModule->GetModule();
  IGC_ASSERT(M);
  // TODO: use debug_compile_units.empty() once LLVM 9 support is dropped
  if (M->debug_compile_units().begin() == M->debug_compile_units().end())
    return;

  DenseMap<DIGlobalVariable *, SmallVector<CompileUnit::GlobalExpr, 1>> GVMap;
  for (const GlobalVariable &Global : M->globals()) {
    SmallVector<DIGlobalVariableExpression *, 1> GVs;
    Global.getDebugInfo(GVs);
    for (auto *GVE : GVs)
      GVMap[GVE->getVariable()].push_back({&Global, GVE->getExpression()});
  }

  // discover DISubprogramNodes for all the registered visaModules
  discoverDISPNodes();
  // Emit initial sections so we can reference labels later.
  emitSectionLabels();

  DICompileUnit *CUNode = *M->debug_compile_units_begin();
  CompileUnit *CU = constructCompileUnit(CUNode);

  // Global Variables.
  for (auto *GVE : CUNode->getGlobalVariables()) {
    // Don't bother adding DIGlobalVariableExpressions listed in the CU if we
    // already know about the variable and it isn't adding a constant
    // expression.
    auto &GVMapEntry = GVMap[GVE->getVariable()];
    auto *Expr = GVE->getExpression();
    if (!GVMapEntry.size() || (Expr && Expr->isConstant()))
      GVMapEntry.push_back({nullptr, Expr});
  }

  DenseSet<DIGlobalVariable *> Processed;
  for (auto *GVE : CUNode->getGlobalVariables()) {
    DIGlobalVariable *GV = GVE->getVariable();

    // Skip variables in different addrspaces than global,
    // SLM globals and constants will be handled later. This
    // will avoid duplication.
    if (GVMap.find(GV) == GVMap.end())
      continue;

    bool AllInGlobalAS = llvm::all_of(GVMap.find(GV)->second, [](const CompileUnit::GlobalExpr &GE) {
      return GE.Var && GE.Var->getAddressSpace() == ADDRESS_SPACE_GLOBAL;
    });

    if (Processed.insert(GV).second && AllInGlobalAS)
      CU->getOrCreateGlobalVariableDIE(GV, sortGlobalExprs(GVMap[GV]));
  }

  for (auto *DISP : DISubprogramNodes)
    constructSubprogramDIE(CU, DISP);

  for (auto *Ty : CUNode->getEnumTypes())
    CU->getOrCreateTypeDIE(Ty);

  for (auto *Ty : CUNode->getRetainedTypes())
    CU->getOrCreateTypeDIE(Ty);

  // Emit imported_modules last so that the relevant context is already
  // available.
  for (auto *IE : CUNode->getImportedEntities())
    constructThenAddImportedEntityDIE(CU, IE);

  [[maybe_unused]] auto NumDebugCUs = std::distance(M->debug_compile_units_begin(), M->debug_compile_units_end());
  if (NumDebugCUs != 1) {
    LLVM_DEBUG(dbgs() << "Warning: Module contains " << NumDebugCUs
                      << " debug compile units. Only modules with one CU are supported currently.\n");
  }
  // Prime section data.
  SectionMap[Asm->GetTextSection()];

  Asm->SwitchSection(Asm->GetDwarfFrameSection());
  offsetCIEStackCall = 0;
  offsetCIESubroutine = writeStackcallCIE();
  // First stack call CIE is written out,
  // next subroutine CIE.
  writeSubroutineCIE();
}

// Attach DW_AT_inline attribute with inlined subprogram DIEs.
void DwarfDebug::computeInlinedDIEs() {
  // Attach DW_AT_inline attribute with inlined subprogram DIEs.
  for (SmallPtrSet<DIE *, 4>::iterator AI = InlinedSubprogramDIEs.begin(), AE = InlinedSubprogramDIEs.end(); AI != AE;
       ++AI) {
    DIE *ISP = *AI;
    FirstCU->addUInt(ISP, dwarf::DW_AT_inline, std::nullopt, dwarf::DW_INL_inlined);
  }
  // TODO: fixup non-deterministic traversal
  for (DenseMap<const MDNode *, DIE *>::iterator AI = AbstractSPDies.begin(), AE = AbstractSPDies.end(); AI != AE;
       ++AI) {
    DIE *ISP = AI->second;
    if (InlinedSubprogramDIEs.count(ISP))
      continue;
    FirstCU->addUInt(ISP, dwarf::DW_AT_inline, std::nullopt, dwarf::DW_INL_inlined);
  }
}

// Collect info for variables that were optimized out.
void DwarfDebug::collectDeadVariables() {
  const Module *M = m_pModule->GetModule();
  NamedMDNode *CU_Nodes = M->getNamedMetadata("llvm.dbg.cu");
  if (!CU_Nodes)
    return;

  for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
    DICompileUnit *TheCU = cast<DICompileUnit>(CU_Nodes->getOperand(i));

    for (auto *SP : DISubprogramNodes) {
      if (!SP)
        continue;

      if (ProcessedSPNodes.count(SP) != 0 || !isa<DISubprogram>(SP) || !SP->isDefinition()) {
        continue;
      }
      auto Variables = SP->getRetainedNodes();
      if (Variables.size() == 0)
        continue;

      // Construct subprogram DIE and add variables DIEs.
      CompileUnit *SPCU = CUMap.lookup(TheCU);
      IGC_ASSERT_MESSAGE(SPCU, "Unable to find Compile Unit!");
      // FIXME: See the comment in constructSubprogramDIE about duplicate
      // subprogram DIEs.
      constructSubprogramDIE(SPCU, SP);
      DIE *SPDIE = SPCU->getDIE(SP);
      for (unsigned vi = 0, ve = Variables.size(); vi != ve; ++vi) {
        DIVariable *DV = cast<DIVariable>(Variables[i]);
        if (!isa<DILocalVariable>(DV))
          continue;
        DbgVariable NewVar(cast<DILocalVariable>(DV));
        DIE *VariableDIE = SPCU->constructVariableDIE(NewVar, false);
        if (SPCU->getDIE(const_cast<DILocalVariable *>(NewVar.getVariable()))) {
          SPCU->applyVariableAttributes(NewVar, VariableDIE, false);
        }
        SPDIE->addChild(VariableDIE);
      }
    }

    // Assume there is a single CU
    break;
  }
}

void DwarfDebug::finalizeModuleInfo() {
  // Collect info for variables that were optimized out.
  LLVM_DEBUG(dbgs() << "[DwarfDebug] collecting dead variables ---\n");
  collectDeadVariables();
  LLVM_DEBUG(dbgs() << "[DwarfDebug] dead variables collected ***\n");

  // Attach DW_AT_inline attribute with inlined subprogram DIEs.
  computeInlinedDIEs();

  // Handle anything that needs to be done on a per-cu basis.
  for (DenseMap<const MDNode *, CompileUnit *>::iterator CUI = CUMap.begin(), CUE = CUMap.end(); CUI != CUE; ++CUI) {
    CompileUnit *TheCU = CUI->second;
    // Emit DW_AT_containing_type attribute to connect types with their
    // vtable holding type.
    TheCU->constructContainingTypeDIEs();
  }

  // Compute DIE offsets and sizes.
  computeSizeAndOffsets();
}

void DwarfDebug::endSections() {
  // Filter labels by section.
  for (size_t n = 0; n < ArangeLabels.size(); n++) {
    const SymbolCU &SCU = ArangeLabels[n];
    if (SCU.Sym->isInSection()) {
      // Make a note of this symbol and it's section.
      const MCSection *Section = &SCU.Sym->getSection();
      if (!Section->getKind().isMetadata())
        SectionMap[Section].push_back(SCU);
    } else {
      // Some symbols (e.g. common/bss on mach-o) can have no section but still
      // appear in the output. This sucks as we rely on sections to build
      // arange spans. We can do it without, but it's icky.
      SectionMap[NULL].push_back(SCU);
    }
  }

  // Build a list of sections used.
  std::vector<const MCSection *> Sections;
  for (SectionMapType::iterator it = SectionMap.begin(); it != SectionMap.end(); it++) {
    const MCSection *Section = it->first;
    Sections.push_back(Section);
  }

  // Sort the sections into order.
  // This is only done to ensure consistent output order across different runs.
  // std::sort(Sections.begin(), Sections.end(), SectionSort);

  // Add terminating symbols for each section.
  for (unsigned ID = 0; ID < Sections.size(); ID++) {
    const MCSection *Section = Sections[ID];
    MCSymbol *Sym = NULL;

    if (Section) {
      // We can't call MCSection::getLabelEndName, as it's only safe to do so
      // if we know the section name up-front. For user-created sections, the
      // resulting label may not be valid to use as a label. (section names can
      // use a greater set of characters on some systems)
      Sym = Asm->GetTempSymbol("debug_end", ID);
      Asm->SwitchSection(Section);
      Asm->EmitLabel(Sym);
    }

    // Insert a final terminator.
    SectionMap[Section].push_back(SymbolCU(NULL, Sym));
  }
}

// Emit all Dwarf sections that should come after the content.
void DwarfDebug::endModule() {
  if (!FirstCU)
    return;

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
  for (DenseMap<const MDNode *, CompileUnit *>::iterator I = CUMap.begin(), E = CUMap.end(); I != E; ++I) {
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
DbgVariable *DwarfDebug::findAbstractVariable(DIVariable *DV, DebugLoc ScopeLoc) {
  // More then one inlined variable corresponds to one abstract variable.
  // DIVariable Var = cleanseInlinedVariable(DV, Ctx);
  DbgVariable *AbsDbgVariable = AbstractVariables.lookup(DV);
  if (AbsDbgVariable)
    return AbsDbgVariable;

  LexicalScope *Scope = LScopes.findAbstractScope(cast<DILocalScope>(ScopeLoc.getScope()));
  if (!Scope)
    return NULL;

  AbsDbgVariable = createDbgVariable(cast<DILocalVariable>(DV));
  LLVM_DEBUG(dbgs() << "  abstract variable: "; AbsDbgVariable->dump());
  addScopeVariable(Scope, AbsDbgVariable);
  AbstractVariables[DV] = AbsDbgVariable;
  return AbsDbgVariable;
}

// If Var is a current function argument then add it to CurrentFnArguments list.
bool DwarfDebug::addCurrentFnArgument(const Function *MF, DbgVariable *Var, LexicalScope *Scope) {
  const DILocalVariable *DV = Var->getVariable();
  unsigned ArgNo = DV->getArg();

  if (!LScopes.isCurrentFunctionScope(Scope) || !DV->isParameter() || ArgNo == 0) {
    return false;
  }

  size_t Size = CurrentFnArguments.size();
  if (Size == 0) {
    CurrentFnArguments.resize(MF->arg_size());
  }
  // llvm::Function argument size is not good indicator of how many
  // arguments does the function have at source level.
  if (ArgNo > Size) {
    CurrentFnArguments.resize(ArgNo * 2);
  }
  CurrentFnArguments[ArgNo - 1] = Var;
  return true;
}

template <typename T> void write(std::vector<unsigned char> &vec, T data) {
  unsigned char *base = (unsigned char *)&data;
  for (unsigned int i = 0; i != sizeof(T); i++)
    vec.push_back(*(base + i));
}

void write(std::vector<unsigned char> &vec, const unsigned char *data, unsigned int N) {
  for (unsigned int i = 0; i != N; i++)
    vec.push_back(*(data + i));
}

void writeULEB128(std::vector<unsigned char> &vec, uint64_t data) {
  auto uleblen = getULEB128Size(data);
  uint8_t *buf = (uint8_t *)malloc(uleblen * sizeof(uint8_t));
  encodeULEB128(data, buf);
  write(vec, buf, uleblen);
  free(buf);
}

// Find variables for each lexical scope.
void DwarfDebug::collectVariableInfo(const Function *MF, SmallPtrSet<const MDNode *, 16> &Processed) {
  // Store pairs of <MDNode*, DILocation*> as we encounter them.
  // This allows us to emit 1 entry per function.
  std::vector<std::tuple<MDNode *, DILocation *, DbgVariable *>> addedEntries;
  std::map<llvm::DIScope *, std::vector<llvm::Instruction *>> instsInScope;

  TempDotDebugLocEntries.clear();

  auto isAdded = [&addedEntries](MDNode *md, DILocation *iat) {
    for (const auto &item : addedEntries) {
      if (std::get<0>(item) == md && std::get<1>(item) == iat)
        return std::get<2>(item);
    }
    return (DbgVariable *)nullptr;
  };

  using IntervalTy = decltype(DbgDecoder::LiveIntervalGenISA::start);
  auto findSemiOpenInterval = [this](IntervalTy start, IntervalTy end) -> std::pair<IntervalTy, IntervalTy> {
    if (start >= end)
      return {0, 0};
    const auto &Map = VisaDbgInfo->getVisaToGenLUT();
    auto LB = Map.lower_bound(start);
    auto UB = Map.upper_bound(end);
    if (LB == Map.end() || UB == Map.end())
      return {0, 0};

    start = LB->second.front();
    end = UB->second.front();
    if (start >= end)
      return {0, 0};
    return {start, end};
  };

  auto encodeImm = [&](IGC::DotDebugLocEntry &dotLoc, uint32_t &offset, DotDebugLocEntryVect &TempDotDebugLocEntries,
                       uint64_t rangeStart, uint64_t rangeEnd, uint32_t pointerSize, DbgVariable *RegVar,
                       const ConstantInt *pConstInt) {
    auto op = llvm::dwarf::DW_OP_implicit_value;
    const unsigned int lebSize = 8;

    dotLoc.start = rangeStart;
    dotLoc.end = rangeEnd;

    // Store location expression bytes in loc
    write(dotLoc.loc, (uint8_t)op);
    write(dotLoc.loc, (const unsigned char *)&lebSize, 1);
    if (isUnsignedDIType(this, RegVar->getType())) {
      uint64_t constValue = pConstInt->getZExtValue();
      write(dotLoc.loc, (unsigned char *)&constValue, lebSize);
    } else {
      int64_t constValue = pConstInt->getSExtValue();
      write(dotLoc.loc, (unsigned char *)&constValue, lebSize);
    }

    TempDotDebugLocEntries.push_back(dotLoc);

    // For DWARF v4 offsets, account for start / end + u16 length + expr bytes
    offset += pointerSize * 2 + 2 + dotLoc.loc.size();
  };

  auto encodeReg = [&](IGC::DotDebugLocEntry &dotLoc, uint32_t &offset, DotDebugLocEntryVect &TempDotDebugLocEntries,
                       uint64_t startRange, uint64_t endRange, uint32_t pointerSize, DbgVariable *RegVar,
                       VISAVariableLocation &Loc, DbgDecoder::LiveIntervalsVISA &visaRange,
                       DbgDecoder::LiveIntervalsVISA &visaRange2nd) {
    auto allCallerSave = m_pModule->getAllCallerSave(*VisaDbgInfo, startRange, endRange, visaRange);
    std::vector<DbgDecoder::LiveIntervalsVISA> vars = {visaRange};

    if (Loc.HasLocationSecondReg())
      vars.push_back(visaRange2nd); // SIMD32 2nd register

    dotLoc.start = startRange;
    TempDotDebugLocEntries.push_back(dotLoc);

    for (const auto &it : allCallerSave) {
      TempDotDebugLocEntries.back().end = std::get<0>(it);
      auto block = FirstCU->buildGeneral(*RegVar, Loc, &vars,
                                         nullptr); // No variable DIE
      std::vector<unsigned char> buffer;
      if (block)
        block->EmitToRawBuffer(buffer);
      write(TempDotDebugLocEntries.back().loc, buffer.data(), buffer.size());
      offset += pointerSize * 2 + 2 + buffer.size();

      DotDebugLocEntry another(dotLoc.getStart(), dotLoc.getEnd(), dotLoc.getDbgInst(), dotLoc.getVariable());
      another.start = std::get<0>(it);
      another.end = std::get<1>(it);
      TempDotDebugLocEntries.push_back(another);
      // write actual caller save location expression
      auto callerSaveVars = vars;
      callerSaveVars.front().var.physicalType = DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory;
      callerSaveVars.front().var.mapping.m.isBaseOffBEFP = 0;
      callerSaveVars.front().var.mapping.m.memoryOffset = std::get<2>(it);
      block = FirstCU->buildGeneral(*RegVar, Loc, &callerSaveVars,
                                    nullptr); // No variable DIE
      buffer.clear();
      if (block)
        block->EmitToRawBuffer(buffer);
      write(TempDotDebugLocEntries.back().loc, buffer.data(), buffer.size());
      offset += pointerSize * 2 + 2 + buffer.size();

      if (std::get<1>(it) >= endRange)
        return;

      // start new interval with original location
      DotDebugLocEntry yetAnother(dotLoc.getStart(), dotLoc.getEnd(), dotLoc.getDbgInst(), dotLoc.getVariable());
      yetAnother.start = std::get<1>(it);
      TempDotDebugLocEntries.push_back(yetAnother);
    }

    TempDotDebugLocEntries.back().end = endRange;
    auto block = FirstCU->buildGeneral(*RegVar, Loc, &vars, nullptr); // No variable DIE
    std::vector<unsigned char> buffer;
    if (block)
      block->EmitToRawBuffer(buffer);
    write(TempDotDebugLocEntries.back().loc, buffer.data(), buffer.size());
    offset += pointerSize * 2 + 2 + buffer.size();
  };

  uint32_t offset = 0;
  unsigned int pointerSize = m_pModule->getPointerSize();
  for (const MDNode *Var : UserVariables) {
    if (Processed.count(Var))
      continue;

    LLVM_DEBUG(dbgs() << "$$$ processing user variable: [" << cast<DIVariable>(Var)->getName() << "], type("
                      << *cast<DIVariable>(Var)->getType() << ")\n");

    // History contains relevant DBG_VALUE instructions for Var and instructions
    // clobbering it.
    InstructionsList &History = DbgValues[Var];
    if (History.empty()) {
      LLVM_DEBUG(dbgs() << "   user variable has no history, skipped\n");
      continue;
    }
    LLVM_DEBUG(dbgs() << "    variable history size: " << History.size() << "\n");

    auto origLocSize = TempDotDebugLocEntries.size();

    // Following loop iterates over all dbg.declare instances
    // for inlined functions and creates new DbgVariable instances for each.

    // DbgVariable is created once per variable to be emitted to dwarf.
    // If a function is inlined x times, there would be x number of DbgVariable
    // instances.
    using DbgVarIPInfo = std::tuple<unsigned int, unsigned int, DbgVariable *, const llvm::DbgVariableIntrinsic *>;
    // TODO: consider replacing std::list to std::vector
    std::unordered_map<DbgVariable *, std::list<DbgVarIPInfo>> DbgValuesWithGenIP;
    for (auto HI = History.begin(), HE = History.end(); HI != HE; HI++) {
      const auto *H = (*HI);
      DIVariable *DV = cast<DIVariable>(const_cast<MDNode *>(Var));

      LexicalScope *Scope = NULL;
      if (DV->getTag() == dwarf::DW_TAG_formal_parameter && DV->getScope() &&
          DV->getScope()->getName() == MF->getName()) {
        Scope = LScopes.getCurrentFunctionScope();
      } else if (auto IA = H->getDebugLoc().getInlinedAt()) {
        Scope = LScopes.findInlinedScope(cast<DILocalScope>(DV->getScope()), IA);
      } else {
        Scope = LScopes.findLexicalScope(cast<DILocalScope>(DV->getScope()));
      }

      // If variable scope is not found then skip this variable.
      if (!Scope)
        continue;

      Processed.insert(DV);
      const llvm::DbgVariableIntrinsic *pInst = H; // History.front();

      IGC_ASSERT_MESSAGE(IsDebugInst(pInst), "History must begin with debug instruction");
      DbgVariable *AbsVar = findAbstractVariable(DV, pInst->getDebugLoc());
      DbgVariable *RegVar = nullptr;

      auto prevRegVar = isAdded(DV, pInst->getDebugLoc().getInlinedAt());

      if (!prevRegVar) {
        RegVar = createDbgVariable(cast<DILocalVariable>(DV), AbsVar ? AbsVar->getLocation() : nullptr, AbsVar);
        LLVM_DEBUG(dbgs() << "  regular variable: "; RegVar->dump());

        if (!addCurrentFnArgument(MF, RegVar, Scope))
          addScopeVariable(Scope, RegVar);

        addedEntries.push_back(std::make_tuple(DV, pInst->getDebugLoc().getInlinedAt(), RegVar));
      } else
        RegVar = prevRegVar;

      // assume that VISA preserves location thoughout its lifetime
      auto Loc = m_pModule->GetVariableLocation(pInst);

      LLVM_DEBUG(Loc.print(dbgs()));

      // Conditions below decide whether we want to emit location to debug_loc
      // or inline it in the DIE. To inline in DIE, we simply set dbg
      // instruction and break. Location list won't be emitted.

      // We emit inlined location for the variable in the following cases:
      // 1. There is a single llvm.dbg.declare instruction describing memory
      // location of the variable.
      // 2. There is a single llvm.dbg.value instruction with immediate value -
      // we assume that the value of the variable is constant.
      // We want to support more cases in the future.

      if (History.size() == 1) {
        if (Loc.IsImmediate() ||
            (isa<DbgDeclareInst>(pInst) && (pInst->getMetadata("StorageOffset") || Loc.HasSurface() || Loc.IsSLM()))) {
          RegVar->setDbgInst(pInst);
          RegVar->setLocationInlined(true);
          break;
        }
      }

      if (History.size() > 1 && isa<DbgDeclareInst>(pInst)) {
        LLVM_DEBUG(dbgs() << "Warning: We don't expect many llvm.dbg.declare calls for a single variable.\n");
      }

      const Instruction *start = (*HI);
      const Instruction *end = start;

      if (HI + 1 != HE)
        end = HI[1];
      else if (auto lastIATit = SameIATInsts.find(start->getDebugLoc().getInlinedAt());
               lastIATit != SameIATInsts.end()) {
        // Find loc of last instruction in current function (same IAT)
        end = (*lastIATit).second.back();
      }

      if (start == end) {
        continue;
      }

      auto GenISARange = m_pModule->getGenISARange(*VisaDbgInfo, {start, end});
      for (const auto &range : GenISARange) {
        DbgValuesWithGenIP[RegVar].emplace_back(range.first, range.second, RegVar, pInst);
      }
    }

    DIVariable *DV = cast<DIVariable>(const_cast<MDNode *>(Var));

    if (!DbgValuesWithGenIP.empty())
      LLVM_DEBUG(dbgs() << "  number of IP intervals for the usage of "
                        << "the source variable: " << DbgValuesWithGenIP.size() << "\n");

    // TODO: fixup non-determenistic traversal
    for (auto &d : DbgValuesWithGenIP) {
      d.second.sort([](const DbgVarIPInfo &first, const DbgVarIPInfo &second) {
        return std::get<0>(first) < std::get<0>(second);
      });

      struct PrevLoc {
        enum class Type { Empty = 0, Imm = 1, Reg = 2 };
        Type t = Type::Empty;
        uint64_t start = 0;
        uint64_t end = 0;
        DbgVariable *dbgVar = nullptr;
        const llvm::DbgVariableIntrinsic *pInst = nullptr;
        const ConstantInt *imm = nullptr;

        VISAVariableLocation Loc;
        DbgDecoder::LiveIntervalsVISA visaRange;
        DbgDecoder::LiveIntervalsVISA visaRange2nd; // In a case of SIMD32
      };

      PrevLoc p;
      auto encodePrevLoc = [&](DotDebugLocEntry &dotLoc, DotDebugLocEntryVect &TempDotDebugLocEntries,
                               uint32_t &offset) {
        if (p.dbgVar->getDotDebugLocOffset() == DbgVariable::InvalidDotDebugLocOffset) {
          p.dbgVar->setDotDebugLocOffset(offset);
        }
        // This instruction bind shouldn't be done like that.
        // DbgVariable class is representing the whole variable,
        // not single dbg intrinsic only. This should be changed when
        // location building will be possible without DbgVariable
        // instruction.
        p.dbgVar->setDbgInst(p.pInst);
        if (p.t == PrevLoc::Type::Imm) {
          encodeImm(dotLoc, offset, TempDotDebugLocEntries, p.start, p.end, pointerSize, p.dbgVar, p.imm);
        } else {
          encodeReg(dotLoc, offset, TempDotDebugLocEntries, p.start, p.end, pointerSize, p.dbgVar, p.Loc, p.visaRange,
                    p.visaRange2nd);
        }
        p.t = PrevLoc::Type::Empty;
      };
      for (auto &range : d.second) {
        // TODO do we really need this variables in tuple
        auto [startIp, endIp, RegVar, pInst] = range;

        auto CurLoc = m_pModule->GetVariableLocation(pInst);

        LLVM_DEBUG(dbgs() << "  Processing Location at IP Range: [0x"; dbgs().write_hex(startIp) << "; " << "0x";
                   dbgs().write_hex(endIp) << "]\n"; CurLoc.print(dbgs()););

        DotDebugLocEntry dotLoc(startIp, endIp, pInst, DV);
        dotLoc.setOffset(offset);

        if (CurLoc.IsImmediate()) {
          const Constant *pConstVal = CurLoc.GetImmediate();
          if (const ConstantInt *pConstInt = dyn_cast<ConstantInt>(pConstVal)) {
            // Always emit an 8-byte value
            uint64_t rangeStart = startIp;
            uint64_t rangeEnd = endIp;

            if (p.t == PrevLoc::Type::Imm && p.end < rangeEnd && p.imm == pConstInt) {
              // extend
              p.end = rangeEnd;
              continue;
            }

            if (p.end >= rangeEnd)
              continue;

            if (rangeStart == rangeEnd)
              continue;

            if (p.t != PrevLoc::Type::Empty) {
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
        } else if (CurLoc.IsRegister()) {
          auto regNum = CurLoc.GetRegister();
          const auto *VarInfo = m_pModule->getVarInfo(*VisaDbgInfo, regNum);
          if (!VarInfo)
            continue;
          for (const auto &visaRange : VarInfo->lrs) {
            auto startEnd = findSemiOpenInterval(visaRange.start, visaRange.end);

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

            if (p.t == PrevLoc::Type::Reg && p.end < endRange) {
              if ((p.visaRange.isGRF() && visaRange.isGRF() && p.visaRange.getGRF() == visaRange.getGRF()) ||
                  (p.visaRange.isSpill() && visaRange.isSpill() &&
                   p.visaRange.getSpillOffset() == visaRange.getSpillOffset())) {
                // extend
                p.end = endRange;
                continue;
              }
            }

            if (p.end >= endRange)
              continue;

            if (startRange == endRange)
              continue;

            if (p.t != PrevLoc::Type::Empty) {
              encodePrevLoc(dotLoc, TempDotDebugLocEntries, offset);
            }

            p.t = PrevLoc::Type::Reg;
            p.start = startRange;
            p.end = endRange;
            p.dbgVar = RegVar;
            p.Loc = CurLoc;
            p.visaRange = visaRange;
            if (CurLoc.HasLocationSecondReg()) {
              auto regNum2nd = CurLoc.GetSecondReg();
              const auto *VarInfo2nd = m_pModule->getVarInfo(*VisaDbgInfo, regNum2nd);
              if (VarInfo2nd)
                p.visaRange2nd = (*VarInfo2nd->lrs.rbegin());
            }
            p.pInst = pInst;

            LLVM_DEBUG(dbgs() << "  Fix IP Range to: [0x"; dbgs().write_hex(p.start) << "; " << "0x";
                       dbgs().write_hex(p.end) << ")\n";);
          }
        }
      }

      if (p.t != PrevLoc::Type::Empty) {
        DotDebugLocEntry dotLoc(p.start, p.end, p.pInst, DV);
        dotLoc.setOffset(offset);
        encodePrevLoc(dotLoc, TempDotDebugLocEntries, offset);
      }

      if (TempDotDebugLocEntries.size() > origLocSize) {
        TempDotDebugLocEntries.push_back(DotDebugLocEntry());
        offset += pointerSize * 2;
      }
    }
  }

  // Collect info for variables that were optimized out.
  LexicalScope *FnScope = LScopes.getCurrentFunctionScope();
  auto Variables = cast<DISubprogram>(FnScope->getScopeNode())->getRetainedNodes();

  for (unsigned i = 0, e = Variables.size(); i != e; ++i) {
    DILocalVariable *DV = cast_or_null<DILocalVariable>(Variables[i]);
    if (!DV || !Processed.insert(DV).second)
      continue;
    if (LexicalScope *Scope = LScopes.findLexicalScope(DV->getScope())) {
      auto *Var = createDbgVariable(DV);
      LLVM_DEBUG(dbgs() << "  optimized-out variable: "; Var->dump());
      addScopeVariable(Scope, Var);
    }
  }
}

// Returns either the offset (unsigned int) in the .debug_loc section or a relocation label (llvm::MCSymbol*)
// depending on whether relocation is enabled. Copies debug location entries from temporary storage to the final list.
std::variant<unsigned int, llvm::MCSymbol *> DwarfDebug::CopyDebugLoc(unsigned int o, bool relocationEnabled) {
  // TempDotLocEntries has all entries discovered in collectVariableInfo.
  // But some of those entries may not get emitted. This function
  // is invoked when writing out DIE. At this time, it can be decided
  // whether debug_range for a variable will be emitted to debug_ranges.
  // If yes, it is copied over to DotDebugLocEntries and new offset or
  // label is returned.
  unsigned int offset = 0, index = 0;
  bool found = false, done = false;
  unsigned int pointerSize = m_pModule->getPointerSize();
  llvm::MCSymbol *Label = nullptr;
  std::variant<unsigned int, llvm::MCSymbol *> offsetOrLabel;

  // Compute offset in DotDebugLocEntries
  for (auto &item : DotDebugLocEntries) {
    if (item.isEmpty())
      offset += pointerSize * 2;
    else
      offset += pointerSize * 2 + 2 + item.loc.size();
  }

  if (relocationEnabled) {
    Label = Asm->GetTempSymbol("debug_loc", offset);
    offsetOrLabel = Label;
  } else {
    offsetOrLabel = offset;
  }

  while (!done && index < TempDotDebugLocEntries.size()) {
    if (!found && TempDotDebugLocEntries[index].getOffset() == o) {
      found = true;
    } else if (!found) {
      index++;
      continue;
    }

    if (found) {
      // Append data to DotLocEntries
      auto &Entry = TempDotDebugLocEntries[index];

      if (relocationEnabled) {
        Entry.setSymbol(Label);
        Label = nullptr;
      }

      DotDebugLocEntries.push_back(Entry);
      if (TempDotDebugLocEntries[index].isEmpty()) {
        done = true;
      }
    }
    index++;
  }

  return offsetOrLabel;
}

llvm::MCSymbol *DwarfDebug::CopyDebugLoc(unsigned int offset) {
  return std::get<llvm::MCSymbol *>(CopyDebugLoc(offset, true));
}

unsigned int DwarfDebug::CopyDebugLocNoReloc(unsigned int offset) {
  return std::get<unsigned int>(CopyDebugLoc(offset, false));
}

// Process beginning of an instruction.
void DwarfDebug::beginInstruction(const Instruction *MI, bool recordSrcLine) {
  // Check if source location changes, but ignore DBG_VALUE locations.
  if (!IsDebugInst(MI) && recordSrcLine) {
    DebugLoc DL = MI->getDebugLoc();
    if (DL && DL != PrevInstLoc) {
      unsigned Flags = 0;
      PrevInstLoc = DL;
      if (DL == PrologEndLoc) {
        Flags |= DWARF2_FLAG_PROLOGUE_END;
        PrologEndLoc = DebugLoc();
      }
      if (!PrologEndLoc) {
        bool setIsStmt = true;
        auto line = DL.getLine();
        auto inlinedAt = DL.getInlinedAt();
        auto it = isStmtSet.find(line);

        if (it != isStmtSet.end()) {
          // is_stmt is set only if line#,
          // inlinedAt combination is
          // never seen before.
          auto &iat = (*it).second;
          for (auto &item : iat) {
            if (item == inlinedAt) {
              setIsStmt = false;
              break;
            }
          }
        }

        if (setIsStmt) {
          Flags |= DWARF2_FLAG_IS_STMT;

          isStmtSet[line].push_back(inlinedAt);
        }
      }

      const MDNode *Scope = DL.getScope();
      recordSourceLine(DL.getLine(), DL.getCol(), Scope, Flags);
    }
  }

  // Insert labels where requested.
  DenseMap<const Instruction *, MCSymbol *>::iterator I = LabelsBeforeInsn.find(MI);

  // No label needed or Label already assigned.
  if (I == LabelsBeforeInsn.end() || I->second)
    return;

  if (!PrevLabel) {
    PrevLabel = Asm->CreateTempSymbol();
    Asm->EmitLabel(PrevLabel);
  }
  I->second = PrevLabel;
}

// Process end of an instruction.
void DwarfDebug::endInstruction(const Instruction *MI) {
  // Don't create a new label after DBG_VALUE instructions.
  // They don't generate code.
  if (!IsDebugInst(MI))
    PrevLabel = 0;

  DenseMap<const Instruction *, MCSymbol *>::iterator I = LabelsAfterInsn.find(MI);

  // No label needed or Label already assigned.
  if (I == LabelsAfterInsn.end() || I->second)
    return;

  // We need a label after this instruction.
  if (!PrevLabel) {
    PrevLabel = Asm->CreateTempSymbol();
    Asm->EmitLabel(PrevLabel);
  }
  I->second = PrevLabel;
}

// Each LexicalScope has first instruction and last instruction to mark
// beginning and end of a scope respectively. Create an inverse map that list
// scopes starts (and ends) with an instruction. One instruction may start (or
// end) multiple scopes. Ignore scopes that are not reachable.
void DwarfDebug::identifyScopeMarkers() {
  SmallVector<LexicalScope *, 4> WorkList;
  WorkList.push_back(LScopes.getCurrentFunctionScope());
  while (!WorkList.empty()) {
    LexicalScope *S = WorkList.pop_back_val();

    const SmallVectorImpl<LexicalScope *> &Children = S->getChildren();
    if (!Children.empty()) {
      for (SmallVectorImpl<LexicalScope *>::const_iterator SI = Children.begin(), SE = Children.end(); SI != SE; ++SI) {
        WorkList.push_back(*SI);
      }
    }

    if (S->isAbstractScope())
      continue;

    const SmallVectorImpl<InsnRange> &Ranges = S->getRanges();
    if (Ranges.empty())
      continue;
    for (SmallVectorImpl<InsnRange>::const_iterator RI = Ranges.begin(), RE = Ranges.end(); RI != RE; ++RI) {
      IGC_ASSERT_MESSAGE(RI->first, "InsnRange does not have first instruction!");
      IGC_ASSERT_MESSAGE(RI->second, "InsnRange does not have second instruction!");
      requestLabelBeforeInsn(RI->first);
      requestLabelAfterInsn(RI->second);
    }
  }
}

// Walk up the scope chain of given debug loc and find line number info
// for the function.
static DebugLoc getFnDebugLoc(DebugLoc DL, const LLVMContext &Ctx) {
  // Get MDNode for DebugLoc's scope.
  while (DILocation *InlinedAt = DL.getInlinedAt()) {
    DL = DebugLoc(InlinedAt);
  }
  const MDNode *Scope = DL.getScope();

  DISubprogram *SP = getDISubprogram(Scope);
  if (SP) {
    // Check for number of operands since the compatibility is cheap here.
    if (SP->getNumOperands() > 19) {
      return DILocation::get(SP->getContext(), SP->getScopeLine(), 0, SP);
    }
    return DILocation::get(SP->getContext(), SP->getLine(), 0, SP);
  }

  return DebugLoc();
}

// Gather pre-function debug information.  Assumes being called immediately
// after the function entry point has been emitted.
void DwarfDebug::beginFunction(const Function *MF, IGC::VISAModule *v) {
  // Reset PrologEndLoc so that when processing next function with same
  // DwarfDebug instance doesnt use stale value.
  PrologEndLoc = DebugLoc();
  // Clear stale isStmt from previous function compilation.
  isStmtSet.clear();
  m_pModule = v;

  // Emit a label for the function so that we have a beginning address.
  FunctionBeginSym = Asm->GetTempSymbol("func_begin", m_pModule->GetFuncId());
  // Assumes in correct section after the entry point.
  Asm->EmitLabel(FunctionBeginSym);

  // Grab the lexical scopes for the function, if we don't have any of those
  // then we're not going to be able to do anything.
  LScopes.initialize(m_pModule);
  if (LScopes.empty()) {
    LLVM_DEBUG(dbgs() << "[DwarfDebug]: no scope detected\n");
    return;
  }

  IGC_ASSERT_MESSAGE(UserVariables.empty(), "Maps weren't cleaned");
  IGC_ASSERT_MESSAGE(DbgValues.empty(), "Maps weren't cleaned");

  // Make sure that each lexical scope will have a begin/end label.
  identifyScopeMarkers();

  // Set DwarfCompileUnitID in MCContext to the Compile Unit this function
  // belongs to so that we add to the correct per-cu line table in the
  // non-asm case.
  LexicalScope *FnScope = LScopes.getCurrentFunctionScope();
  CompileUnit *TheCU = SPMap.lookup(FnScope->getScopeNode());
  IGC_ASSERT_MESSAGE(TheCU, "Unable to find compile unit!");
  Asm->SetDwarfCompileUnitID(TheCU->getUniqueID());

  llvm::MDNode *prevIAT = nullptr;

  for (auto II = m_pModule->begin(), IE = m_pModule->end(); II != IE; ++II) {
    const Instruction *MI = *II;
    const auto &Loc = MI->getDebugLoc();

    if (Loc && Loc.getScope() != prevIAT) {
      SameIATInsts[Loc.getInlinedAt()].push_back(MI);
      prevIAT = Loc.getInlinedAt();
    }

    if (IsDebugInst(MI)) {
      IGC_ASSERT_MESSAGE(MI->getNumOperands() > 1, "Invalid machine instruction!");

      // Keep track of user variables.
      const MDNode *Var = GetDebugVariable(MI);

      // Check the history of this variable.
      InstructionsList &History = DbgValues[Var];
      if (History.empty()) {
        UserVariables.push_back(Var);
        // The first mention of a function argument gets the FunctionBeginSym
        // label, so arguments are visible when breaking at function entry.
        const DIVariable *DV = cast_or_null<DIVariable>(Var);
        if (DV && DV->getTag() == dwarf::DW_TAG_formal_parameter && getDISubprogram(DV->getScope())->describes(MF)) {
          LabelsBeforeInsn[MI] = FunctionBeginSym;
        }
      } else {
        // We have seen this variable before. Try to coalesce DBG_VALUEs.
        const Instruction *Prev = History.back();
        // Coalesce identical entries at the end of History.
        if (History.size() >= 2 && Prev->isIdenticalTo(History[History.size() - 2])) {
          LLVM_DEBUG(dbgs() << "Coalescing identical DBG_VALUE entries:\n"; DbgVariable::dumpDbgInst(Prev));
          History.pop_back();
        }
      }
      History.push_back(cast<llvm::DbgVariableIntrinsic>(MI));
    } else if (m_pModule->IsExecutableInst(*MI)) {
      // Not a DBG_VALUE instruction.

      // First known non-DBG_VALUE and non-frame setup location marks
      // the beginning of the function body.
      if (!PrologEndLoc && Loc) {
        PrologEndLoc = Loc;
      }
    }
  }

  // TODO: fixup non-deterministic traversal
  for (const auto &HistoryInfo : DbgValues) {
    const InstructionsList &History = HistoryInfo.second;
    if (History.empty())
      continue;

    // Request labels for the full history.
    for (const Instruction *MI : History) {
      if (IsDebugInst(MI))
        requestLabelBeforeInsn(MI);
      else
        requestLabelAfterInsn(MI);
    }
  }

  PrevInstLoc = DebugLoc();
  PrevLabel = FunctionBeginSym;

  // Record beginning of function.
  if (PrologEndLoc) {
    DebugLoc FnStartDL = getFnDebugLoc(PrologEndLoc, MF->getContext());
    const MDNode *Scope = FnStartDL.getScope();
    // We'd like to list the prologue as "not statements" but GDB behaves
    // poorly if we do that. Revisit this with caution/GDB (7.5+) testing.
    recordSourceLine(FnStartDL.getLine(), FnStartDL.getCol(), Scope, DWARF2_FLAG_IS_STMT);
  }
}

DbgVariable *DwarfDebug::createDbgVariable(const llvm::DILocalVariable *V, const llvm::DILocation *IA,
                                           DbgVariable *AV) {
  DbgVariablesStorage.push_back(std::make_unique<DbgVariable>(V, IA, AV));
  LLVM_DEBUG(dbgs() << "[DwarfDebug] created DbgVariable instance...\n");
  return DbgVariablesStorage.back().get();
}

void DwarfDebug::addScopeVariable(LexicalScope *LS, DbgVariable *Var) {
  DbgVariablesVect &Vars = ScopeVariables[LS];
  const DILocalVariable *DV = Var->getVariable();
  // Variables with positive arg numbers are parameters.
  if (unsigned ArgNum = DV->getArg()) {
    // Keep all parameters in order at the start of the variable list to ensure
    // function types are correct (no out-of-order parameters)
    //
    // This could be improved by only doing it for optimized builds (unoptimized
    // builds have the right order to begin with), searching from the back (this
    // would catch the unoptimized case quickly), or doing a binary search
    // rather than linear search.
    auto I = Vars.begin();
    while (I != Vars.end()) {
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
void DwarfDebug::endFunction(const Function *MF) {
  // Define end label for subprogram.
  FunctionEndSym = Asm->GetTempSymbol("func_end", m_pModule->GetFuncId());
  // Assumes in correct section after the entry point.
  Asm->EmitLabel(FunctionEndSym);

  Asm->EmitELFDiffSize(FunctionBeginSym, FunctionEndSym, FunctionBeginSym);
  if (!LScopes.empty()) {

    // Set DwarfCompileUnitID in MCContext to default value.
    Asm->SetDwarfCompileUnitID(0);

    SmallPtrSet<const MDNode *, 16> ProcessedVars;
    LLVM_DEBUG(dbgs() << "[DwarfDebug] collecting variables ---\n");
    collectVariableInfo(MF, ProcessedVars);
    LLVM_DEBUG(dbgs() << "[DwarfDebug] variables collected ***\n");

    LexicalScope *FnScope = LScopes.getCurrentFunctionScope();
    CompileUnit *TheCU = SPMap.lookup(FnScope->getScopeNode());
    IGC_ASSERT_MESSAGE(TheCU, "Unable to find compile unit!");

    // Construct abstract scopes.
    LLVM_DEBUG(dbgs() << "[DwarfDebug] constructing abstract scopes ---\n");
    ArrayRef<LexicalScope *> AList = LScopes.getAbstractScopesList();
    for (unsigned i = 0, e = AList.size(); i != e; ++i) {
      LexicalScope *AScope = AList[i];
      const DISubprogram *SP = cast_or_null<DISubprogram>(AScope->getScopeNode());
      if (SP) {
        // Collect info for variables that were optimized out.
        auto Variables = SP->getRetainedNodes();
        for (unsigned i = 0, e = Variables.size(); i != e; ++i) {
          DILocalVariable *DV = cast_or_null<DILocalVariable>(Variables[i]);
          if (!DV || !ProcessedVars.insert(DV).second)
            continue;
          // Check that DbgVariable for DV wasn't created earlier, when
          // findAbstractVariable() was called for inlined instance of DV.
          // LLVMContext &Ctx = DV->getContext();
          // DIVariable CleanDV = cleanseInlinedVariable(DV, Ctx);
          // if (AbstractVariables.lookup(CleanDV)) continue;
          if (LexicalScope *Scope = LScopes.findAbstractScope(DV->getScope())) {
            auto *Var = createDbgVariable(DV);
            LLVM_DEBUG(dbgs() << "  optimized-out variable: "; Var->dump());
            addScopeVariable(Scope, Var);
          }
        }
      }
      if (ProcessedSPNodes.count(AScope->getScopeNode()) == 0) {
        constructScopeDIE(TheCU, AScope);
      }
    }
    LLVM_DEBUG(dbgs() << "[DwarfDebug] abstract scopes constructed ***\n");

    LLVM_DEBUG(dbgs() << "[DwarfDebug] constructing FnScope ---\n");
    constructScopeDIE(TheCU, FnScope);
    LLVM_DEBUG(dbgs() << "[DwarfDebug] FnScope constructed ***\n");
  }

  Asm->SwitchSection(Asm->GetDwarfFrameSection());

  if (m_pModule->GetType() == VISAModule::ObjectType::SUBROUTINE) {
    LLVM_DEBUG(dbgs() << "[DwarfDebug] writing FDESubproutine ***\n");
    writeFDESubroutine(m_pModule);
  } else {
    LLVM_DEBUG(dbgs() << "[DwarfDebug] writing FDEStackCall ***\n");
    writeFDEStackCall(m_pModule);
  }

  ScopeVariables.clear();
  CurrentFnArguments.clear();
  DbgVariablesStorage.clear();
  UserVariables.clear();
  DbgValues.clear();
  AbstractVariables.clear();
  LabelsBeforeInsn.clear();
  LabelsAfterInsn.clear();
  PrevLabel = NULL;
}

// Register a source line with debug info. Returns the  unique label that was
// emitted and which provides correspondence to the source line list.
void DwarfDebug::recordSourceLine(unsigned Line, unsigned Col, const MDNode *S, unsigned Flags) {
  unsigned FNo = 1;
  StringRef Fn;
  StringRef Dir;
  IGCLLVM::optional<llvm::MD5::MD5Result> Chs;
  IGCLLVM::optional<llvm::StringRef> Src;
  if (S) {
    if (isa<DICompileUnit>(S)) {
      const DICompileUnit *CU = cast<DICompileUnit>(S);
      Fn = CU->getFilename();
      Dir = CU->getDirectory();
      Chs = getMD5AsBytes(CU->getFile());
      Src = CU->getSource();
    } else if (isa<DIFile>(S)) {
      const DIFile *F = cast<DIFile>(S);
      Fn = F->getFilename();
      Dir = F->getDirectory();
      Chs = getMD5AsBytes(F);
      Src = F->getSource();
    } else if (isa<DISubprogram>(S)) {
      const DISubprogram *SP = cast<DISubprogram>(S);
      Fn = SP->getFilename();
      Dir = SP->getDirectory();
      Chs = getMD5AsBytes(SP->getFile());
      Src = SP->getSource();
    } else if (isa<DILexicalBlockFile>(S)) {
      const DILexicalBlockFile *DBF = cast<DILexicalBlockFile>(S);
      Fn = DBF->getFilename();
      Dir = DBF->getDirectory();
      Chs = getMD5AsBytes(DBF->getFile());
      Src = DBF->getSource();
    } else if (isa<DILexicalBlock>(S)) {
      const DILexicalBlock *DB = cast<DILexicalBlock>(S);
      Fn = DB->getFilename();
      Dir = DB->getDirectory();
      Chs = getMD5AsBytes(DB->getFile());
      Src = DB->getSource();
    } else {
      IGC_ASSERT_EXIT_MESSAGE(0, "Unexpected scope info");
    }

    FNo = getOrCreateSourceID(Fn, Dir, Chs, Src, Asm->GetDwarfCompileUnitID(), false);
  }
  Asm->EmitDwarfLocDirective(FNo, Line, Col, Flags, 0, 0, Fn);
}

//===----------------------------------------------------------------------===//
// Emit Methods
//===----------------------------------------------------------------------===//

// Compute the size and offset of a DIE. The offset is relative to start of the
// CU. It returns the offset after laying out the DIE.
unsigned DwarfDebug::computeSizeAndOffset(DIE *Die, unsigned Offset) {
  // Get the children.
  const std::vector<DIE *> &Children = Die->getChildren();

  // Record the abbreviation.
  assignAbbrevNumber(Die->getAbbrev());

  // Get the abbreviation for this DIE.
  unsigned AbbrevNumber = Die->getAbbrevNumber();
  const DIEAbbrev *Abbrev = Abbreviations[AbbrevNumber - 1];

  // Set DIE offset
  Die->setOffset(Offset);

  // Start the size with the size of abbreviation code.
  Offset += getULEB128Size(AbbrevNumber);

  const SmallVectorImpl<DIEValue *> &Values = Die->getValues();
  const SmallVectorImpl<DIEAbbrevData> &AbbrevData = Abbrev->getData();

  // Size the DIE attribute values.
  for (unsigned i = 0, N = Values.size(); i < N; ++i) {
    // Size attribute value.
    Offset += Values[i]->SizeOf(Asm, AbbrevData[i].getForm());
  }

  // Size the DIE children if any.
  if (!Children.empty()) {
    IGC_ASSERT_MESSAGE(Abbrev->getChildrenFlag() == dwarf::DW_CHILDREN_yes, "Children flag not set");

    for (unsigned j = 0, M = Children.size(); j < M; ++j) {
      Offset = computeSizeAndOffset(Children[j], Offset);
    }

    // End of children marker.
    Offset += sizeof(int8_t);
  }

  Die->setSize(Offset - Die->getOffset());
  return Offset;
}

// Compute the size and offset for each DIE.
void DwarfDebug::computeSizeAndOffsets() {
  // Offset from the first CU in the debug info section is 0 initially.
  unsigned SecOffset = 0;

  // Iterate over each compile unit and set the size and offsets for each
  // DIE within each compile unit. All offsets are CU relative.
  for (SmallVectorImpl<CompileUnit *>::iterator I = CUs.begin(), E = CUs.end(); I != E; ++I) {
    (*I)->setDebugInfoOffset(SecOffset);

    // CU-relative offset is reset to 0 here.
    unsigned Offset = sizeof(int32_t) +      // Length of Unit Info
                      (*I)->getHeaderSize(); // Unit-specific headers

    // EndOffset here is CU-relative, after laying out
    // all of the CU DIE.
    unsigned EndOffset = computeSizeAndOffset((*I)->getCUDie(), Offset);
    SecOffset += EndOffset;
  }
}

// Switch to the specified MCSection and emit an assembler
// temporary label to it if SymbolStem is specified.
static MCSymbol *emitSectionSym(StreamEmitter *Asm, const MCSection *Section, const char *SymbolStem = 0) {
  Asm->SwitchSection(Section);
  if (!SymbolStem)
    return 0;

  MCSymbol *TmpSym = Asm->GetTempSymbol(SymbolStem);
  Asm->EmitLabel(TmpSym);
  return TmpSym;
}

// Emit initial Dwarf sections with a label at the start of each one.
void DwarfDebug::emitSectionLabels() {
  // Dwarf sections base addresses.
  DwarfInfoSectionSym = emitSectionSym(Asm, Asm->GetDwarfInfoSection(), "section_info");
  DwarfAbbrevSectionSym = emitSectionSym(Asm, Asm->GetDwarfAbbrevSection(), "section_abbrev");

  DwarfFrameSectionSym = emitSectionSym(Asm, Asm->GetDwarfFrameSection(), "dwarf_frame");

  if (const MCSection *MacroInfo = Asm->GetDwarfMacroInfoSection()) {
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
void DwarfDebug::emitDebugStr() {
  const MCSection *StrSection = Asm->GetDwarfStrSection();
  if (StringPool.empty())
    return;

  // Start the dwarf str section.
  Asm->SwitchSection(StrSection);

  // Get all of the string pool entries and put them in an array by their ID so
  // we can sort them.
  SmallVector<std::pair<unsigned, StringMapEntry<std::pair<MCSymbol *, unsigned>> *>, 64> Entries;

  for (StringMap<std::pair<MCSymbol *, unsigned>>::iterator I = StringPool.begin(), E = StringPool.end(); I != E; ++I) {
    Entries.push_back(std::make_pair(I->second.second, &*I));
  }

  array_pod_sort(Entries.begin(), Entries.end());

  for (unsigned i = 0, e = Entries.size(); i != e; ++i) {
    // Emit a label for reference from debug information entries.
    Asm->EmitLabel(Entries[i].second->getValue().first);

    // Emit the string itself with a terminating null byte.
    Asm->EmitBytes(StringRef(Entries[i].second->getKeyData(), Entries[i].second->getKeyLength() + 1));
  }
}

// Recursively emits a debug information entry.
void DwarfDebug::emitDIE(DIE *Die) {
  // Get the abbreviation for this DIE.
  unsigned AbbrevNumber = Die->getAbbrevNumber();
  const DIEAbbrev *Abbrev = Abbreviations[AbbrevNumber - 1];

  // Emit the code (index) for the abbreviation.
  Asm->EmitULEB128(AbbrevNumber);

  const SmallVectorImpl<DIEValue *> &Values = Die->getValues();
  const SmallVectorImpl<DIEAbbrevData> &AbbrevData = Abbrev->getData();

  // Emit the DIE attribute values.
  for (unsigned i = 0, N = Values.size(); i < N; ++i) {
    dwarf::Attribute Attr = AbbrevData[i].getAttribute();
    dwarf::Form Form = AbbrevData[i].getForm();
    IGC_ASSERT_MESSAGE(Form, "Too many attributes for DIE (check abbreviation)");

    switch (Attr) {
    case dwarf::DW_AT_abstract_origin:
    case dwarf::DW_AT_type:
    case dwarf::DW_AT_friend:
    case dwarf::DW_AT_specification:
    case dwarf::DW_AT_import:
    case dwarf::DW_AT_containing_type: {
      DIE *Origin = cast<DIEEntry>(Values[i])->getEntry();
      unsigned Addr = Origin->getOffset();
      if (Form == dwarf::DW_FORM_ref_addr) {
        // For DW_FORM_ref_addr, output the offset from beginning of debug info
        // section. Origin->getOffset() returns the offset from start of the
        // compile unit.
        CompileUnit *CU = CUDieMap.lookup(Origin->getCompileUnit());
        IGC_ASSERT_MESSAGE(CU, "CUDie should belong to a CU.");
        Addr += CU->getDebugInfoOffset();
        Asm->EmitLabelPlusOffset(DwarfInfoSectionSym, Addr, DIEEntry::getRefAddrSize(Asm, getDwarfVersion()));
      } else {
        // Make sure Origin belong to the same CU.
        IGC_ASSERT_MESSAGE(Die->getCompileUnit() == Origin->getCompileUnit(),
                           "The referenced DIE should belong to the same CU in ref4");
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
  if (Abbrev->getChildrenFlag() == dwarf::DW_CHILDREN_yes) {
    const std::vector<DIE *> &Children = Die->getChildren();

    for (unsigned j = 0, M = Children.size(); j < M; ++j) {
      emitDIE(Children[j]);
    }

    Asm->EmitInt8(0);
  }
}

// Emit the debug info section.
void DwarfDebug::emitDebugInfo() {
  const MCSection *USection = Asm->GetDwarfInfoSection();
  const MCSection *ASection = Asm->GetDwarfAbbrevSection();
  const MCSymbol *ASectionSym = DwarfAbbrevSectionSym;

  Asm->SwitchSection(USection);
  for (SmallVectorImpl<CompileUnit *>::iterator I = CUs.begin(), E = CUs.end(); I != E; ++I) {
    CompileUnit *TheCU = *I;
    DIE *Die = TheCU->getCUDie();

    // Emit the compile units header.
    Asm->EmitLabel(Asm->GetTempSymbol(
        /*USection->getLabelBeginName()*/ ".debug_info_begin", TheCU->getUniqueID()));

    // Emit size of content not including length itself
    // Emit ("Length of Unit");
    Asm->EmitInt32(TheCU->getHeaderSize() + Die->getSize());

    TheCU->emitHeader(ASection, ASectionSym);

    emitDIE(Die);
    Asm->EmitLabel(Asm->GetTempSymbol(/*USection->getLabelEndName()*/ ".debug_info_end", TheCU->getUniqueID()));
  }
}

// Emit the abbreviation section.
void DwarfDebug::emitAbbreviations() {
  const MCSection *Section = Asm->GetDwarfAbbrevSection();

  // Check to see if it is worth the effort.
  if (!Abbreviations.empty()) {
    // Start the debug abbrev section.
    Asm->SwitchSection(Section);

    MCSymbol *Begin = Asm->GetTempSymbol(
        /*Section->getLabelBeginName()*/ ".debug_abbrev_begin");
    Asm->EmitLabel(Begin);

    // For each abbrevation.
    for (unsigned i = 0, N = Abbreviations.size(); i < N; ++i) {
      // Get abbreviation data
      const DIEAbbrev *Abbrev = Abbreviations.at(i);

      // Emit the abbrevations code (base 1 index.)
      Asm->EmitULEB128(Abbrev->getNumber(), "Abbreviation Code");

      // Emit the abbreviations data.
      Abbrev->Emit(Asm);
    }

    // Mark end of abbreviations.
    Asm->EmitULEB128(0, "EOM(3)");

    MCSymbol *End = Asm->GetTempSymbol(/*Section->getLabelEndName()*/ ".debug_abbrev_end");
    Asm->EmitLabel(End);
  }
}

// Emit locations into the debug loc section.
void DwarfDebug::emitDebugLoc() {
  if (DotDebugLocEntries.empty())
    return;

  Asm->SwitchSection(Asm->GetDwarfLocSection());

  for (const DotDebugLocEntry &Entry : DotDebugLocEntries) {
    if (Entry.isEmpty()) {
      Asm->EmitIntValue(0, PointerSize);
      Asm->EmitIntValue(0, PointerSize);
    } else {
      auto *Symbol = Entry.getSymbol();
      if (Symbol)
        Asm->EmitLabel(Symbol);

      // Emit start and end addresses
      Asm->EmitIntValue(Entry.start, PointerSize);
      Asm->EmitIntValue(Entry.end, PointerSize);

      // Emit expression bytes
      const size_t exprSize = Entry.loc.size();
      IGC_ASSERT_MESSAGE(exprSize <= std::numeric_limits<uint16_t>::max(),
                         "DWARF v4 uses 16-bit length, expression size is too large");
      Asm->EmitInt16((uint16_t)exprSize);
      for (size_t i = 0; i < exprSize; i++)
        Asm->EmitInt8(Entry.loc[i]);
    }
  }

  DotDebugLocEntries.clear();
}

// Emit visible names into a debug ranges section.
void DwarfDebug::emitDebugRanges() {
  // Start the dwarf ranges section.
  Asm->SwitchSection(Asm->GetDwarfRangesSection());
  unsigned char size = (unsigned char)Asm->GetPointerSize();

  for (auto &Entry : GenISADebugRangeSymbols) {
    auto Label = Entry.first;
    if (Label)
      Asm->EmitLabel(Label);
    for (auto Data : Entry.second) {
      Asm->EmitIntValue(Data, size);
    }
  }
}

// Emit visible names into a debug macinfo section.
void DwarfDebug::emitDebugMacInfo() {
  if (const MCSection *pLineInfo = Asm->GetDwarfMacroInfoSection()) {
    // Start the dwarf macinfo section.
    Asm->SwitchSection(pLineInfo);
  }
}

void DwarfDebug::encodeScratchAddrSpace(std::vector<uint8_t> &data) {
  uint32_t scratchBaseAddrEncoded = 0;
  if (m_pModule->usesSlot1ScratchSpill()) {
    scratchBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::ScratchBaseSlot1>(dwarf::DW_OP_breg0);
  } else {
    scratchBaseAddrEncoded = GetEncodedRegNum<RegisterNumbering::ScratchBase>(dwarf::DW_OP_breg0);
  }

  write(data, (uint8_t)scratchBaseAddrEncoded);
  writeULEB128(data, 0);
  write(data, (uint8_t)llvm::dwarf::DW_OP_plus);
}

uint32_t DwarfDebug::writeSubroutineCIE() {
  std::vector<uint8_t> data;
  auto numGRFs = GetVISAModule()->getNumGRFs();

  auto writeSameValue = [](std::vector<uint8_t> &data, uint32_t srcReg) {
    write(data, (uint8_t)llvm::dwarf::DW_CFA_same_value);
    writeULEB128(data, srcReg);
  };

  // Emit CIE
  // The size of the length field plus the value of length must be an integral
  // multiple of the address size.
  uint8_t lenSize = PointerSize;
  if (PointerSize == 8)
    lenSize = 12;

  // Write CIE_id
  write(data, PointerSize == 4 ? (uint32_t)0xffffffff : (uint64_t)0xffffffffffffffff);

  // version - ubyte
  write(data, (uint8_t)4);

  // augmentation - UTF8 string
  write(data, (uint8_t)0);

  // address size - ubyte
  write(data, (uint8_t)PointerSize);

  // segment size - ubyte
  write(data, (uint8_t)0);

  // code alignment factor - uleb128
  write(data, (uint8_t)1);

  // data alignment factor - sleb128
  write(data, (uint8_t)1);

  // return address register - uleb128
  // set machine return register to one which is physically
  // absent. later CFA instructions map this to a valid GRF.
  writeULEB128(data, RegisterNumbering::IP);

  // initial instructions (array of ubyte)
  // same value rule for all GRFs
  for (unsigned int grf = 0; grf != numGRFs; ++grf) {
    writeSameValue(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(grf));
  }

  while ((lenSize + data.size()) % PointerSize != 0)
    // Insert DW_CFA_nop
    write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

  // Emit length with marker 0xffffffff for 8-byte ptr
  // DWARF4 spec:
  //  in the 64-bit DWARF format, an initial length field is 96 bits in size,
  //  and has two parts:
  //  * The first 32-bits have the value 0xffffffff.
  //  * The following 64-bits contain the actual length represented as an
  //  unsigned 64-bit integer.
  //
  // In the 32-bit DWARF format, an initial length field (see Section 7.2.2) is
  // an unsigned 32-bit integer
  //  (which must be less than 0xfffffff0)

  uint32_t bytesWritten = 0;
  if (PointerSize == 8) {
    Asm->EmitInt32(0xffffffff);
    bytesWritten = 4;
  }
  Asm->EmitIntValue(data.size(), PointerSize);
  bytesWritten += PointerSize;

  for (auto &byte : data)
    Asm->EmitInt8(byte);
  bytesWritten += data.size();

  return bytesWritten;
}

uint32_t DwarfDebug::writeStackcallCIE() {
  std::vector<uint8_t> data, data1;
  auto numGRFs = GetVISAModule()->getNumGRFs();
  auto specialGRF = GetSpecialGRF();

  auto writeUndefined = [](std::vector<uint8_t> &data, uint32_t srcReg) {
    write(data, (uint8_t)llvm::dwarf::DW_CFA_undefined);
    writeULEB128(data, srcReg);
  };

  auto writeSameValue = [](std::vector<uint8_t> &data, uint32_t srcReg) {
    write(data, (uint8_t)llvm::dwarf::DW_CFA_same_value);
    writeULEB128(data, srcReg);
  };

  // Emit CIE
  // The size of the length field plus the value of length must be an integral
  // multiple of the address size.
  uint8_t lenSize = PointerSize;
  if (PointerSize == 8)
    lenSize = 12;

  // Write CIE_id
  write(data, PointerSize == 4 ? (uint32_t)0xffffffff : (uint64_t)0xffffffffffffffff);

  // version - ubyte
  write(data, (uint8_t)4);

  // augmentation - UTF8 string
  write(data, (uint8_t)0);

  // address size - ubyte
  write(data, (uint8_t)PointerSize);

  // segment size - ubyte
  write(data, (uint8_t)0);

  // code alignment factor - uleb128
  write(data, (uint8_t)1);

  // data alignment factor - sleb128
  write(data, (uint8_t)1);

  // return address register - uleb128
  // set machine return register to one which is physically
  // absent. later CFA instructions map this to a valid GRF.
  writeULEB128(data, RegisterNumbering::IP);

  // initial instructions (array of ubyte)
  // DW_OP_regx r125
  // DW_OP_bit_piece 32 96
  write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa_expression);

  // The DW_CFA_def_cfa_expression instruction takes a single operand
  // encoded as a DW_FORM_exprloc.
  // We define DW_CFA_def_cfa_expression to point to caller's BE_SP.
  auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF);
  write(data1, (uint8_t)llvm::dwarf::DW_OP_const4u);
  write(data1, (uint32_t)(DWRegEncoded));
  write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
  write(data1, (uint16_t)(getBESPSubReg() * 4 * 8));
  write(data1, (uint8_t)DW_OP_INTEL_regval_bits);
  write(data1, (uint8_t)32);

  if (EmitSettings.ScratchOffsetInOW) {
    // when scratch offset is in OW, be_fp has to be multiplied by 16
    // to normalize and generate byte offset for complete address
    // computation.
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const1u);
    write(data1, (uint8_t)16);
    write(data1, (uint8_t)llvm::dwarf::DW_OP_mul);
  }

  // indicate that the resulting address is on BE stack
  encodeScratchAddrSpace(data1);

  writeULEB128(data, data1.size());
  for (auto item : data1)
    write(data, (uint8_t)item);

  // emit same value for all callee save entries in frame
  unsigned int calleeSaveStart = (numGRFs - 8) / 2;

  // caller save - undefined rule
  for (unsigned int grf = 0; grf != calleeSaveStart; ++grf) {
    writeUndefined(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(grf));
  }

  // callee save - same value rule
  for (unsigned int grf = calleeSaveStart; grf != numGRFs; ++grf) {
    writeSameValue(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(grf));
  }

  // move return address register to actual location
  if (GetABIVersion() < 3) {
    // DW_CFA_register     IP      specialGRF
    write(data, (uint8_t)llvm::dwarf::DW_CFA_register);
    writeULEB128(data, RegisterNumbering::IP);
    writeULEB128(data, GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));
  } else {
    // Set the IP register to point to location of return IP in r127. DW_OP_drop is used because DW_CFA_val_expression
    // implicitly pushes CFA first, but we don't use its value in this expression. DW_CFA_val_expression is used
    // because IP is directly stored in the register.
    write(data, (uint8_t)llvm::dwarf::DW_CFA_val_expression);
    writeULEB128(data, RegisterNumbering::IP);

    data1.clear();
    write(data1, (uint8_t)llvm::dwarf::DW_OP_drop);
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const4u);
    write(data1, (uint32_t)(DWRegEncoded));
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
    write(data1, (uint16_t)(getRetIPSubReg() * 4 * 8));
    write(data1, (uint8_t)DW_OP_INTEL_regval_bits);
    write(data1, (uint8_t)64);

    writeULEB128(data, data1.size());
    for (auto item : data1)
      write(data, (uint8_t)item);
  }

  while ((lenSize + data.size()) % PointerSize != 0)
    // Insert DW_CFA_nop
    write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

  // Emit length with marker 0xffffffff for 8-byte ptr
  // DWARF4 spec:
  //  in the 64-bit DWARF format, an initial length field is 96 bits in size,
  //  and has two parts:
  //  * The first 32-bits have the value 0xffffffff.
  //  * The following 64-bits contain the actual length represented as an
  //  unsigned 64-bit integer.
  //
  // In the 32-bit DWARF format, an initial length field (see Section 7.2.2) is
  // an unsigned 32-bit integer
  //  (which must be less than 0xfffffff0)

  uint32_t bytesWritten = 0;
  if (PointerSize == 8) {
    Asm->EmitInt32(0xffffffff);
    bytesWritten = 4;
  }
  Asm->EmitIntValue(data.size(), PointerSize);
  bytesWritten += PointerSize;

  for (auto &byte : data)
    Asm->EmitInt8(byte);
  bytesWritten += data.size();

  return bytesWritten;
}

void DwarfDebug::writeFDESubroutine(VISAModule *m) {
  std::vector<uint8_t> data, data1;
  uint64_t LabelOffset = std::numeric_limits<uint64_t>::max();

  auto firstInst = (m->GetInstInfoMap()->begin())->first;
  // TODO: fixup to a proper name getter
  auto funcName = firstInst->getParent()->getParent()->getName();

  const IGC::DbgDecoder::SubroutineInfo *sub = nullptr;
  for (const auto &s : VisaDbgInfo->getSubroutines()) {
    if (s.name.compare(funcName.str()) == 0) {
      sub = &s;
      break;
    }
  }

  if (!sub)
    return;

  // Emit CIE
  uint8_t lenSize = 4;
  if (PointerSize == 8)
    lenSize = 12;

  // CIE_ptr (4/8 bytes)
  write(data, PointerSize == 4 ? (uint32_t)offsetCIESubroutine : (uint64_t)offsetCIESubroutine);

  uint64_t genOffStart = this->lowPc;
  uint64_t genOffEnd = this->highPc;
  auto &retvarLR = sub->retval;
  IGC_ASSERT_MESSAGE(retvarLR.size() > 0, "expecting GRF for return");
  IGC_ASSERT_MESSAGE(retvarLR[0].var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF,
                     "expecting GRF for return");

  // assume ret var is live throughout sub-routine and it is contained
  // in same GRF.
  uint16_t retRegNum = retvarLR.front().var.mapping.r.regNum;
  uint16_t retSubRegNum = retvarLR.front().var.mapping.r.subRegNum;

  // initial location
  // LabelOffset holds offset where start %ip is written to buffer.
  // Code later uses this to insert label for relocation.
  LabelOffset = data.size();
  if (EmitSettings.EnableRelocation) {
    write(data, PointerSize == 4 ? (uint32_t)0xfefefefe : (uint64_t)0xfefefefefefefefe);
  } else {
    write(data, PointerSize == 4 ? (uint32_t)genOffStart : (uint64_t)genOffStart);
  }

  // address range
  write(data, PointerSize == 4 ? (uint32_t)(genOffEnd - genOffStart) : (uint64_t)(genOffEnd - genOffStart));

  // instruction - ubyte
  write(data, (uint8_t)llvm::dwarf::DW_CFA_val_expression);

  // return reg operand
  writeULEB128(data, RegisterNumbering ::IP);

  // actual reg holding retval
  data1.clear();
  write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
  write(data1, (uint16_t)GetEncodedRegNum<RegisterNumbering::GRFBase>(retRegNum));
  write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
  write(data1, (uint16_t)(retSubRegNum * 8));
  write(data1, (uint8_t)DW_OP_INTEL_regval_bits);
  write(data1, (uint8_t)32);

  writeULEB128(data, data1.size());
  for (auto item : data1)
    write(data, (uint8_t)item);

  // initial instructions (array of ubyte)
  // DW_CFA_def_cfa_expression = DW_OP_lit0
  write(data, (uint8_t)llvm::dwarf::DW_CFA_def_cfa_expression);
  data1.clear();
  if (m->StackCallContext()) {
    // CFA is BE_SP location
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
    write(data1, (uint16_t)GetEncodedRegNum<RegisterNumbering::GRFBase>(GetSpecialGRF()));
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const1u);
    if (GetABIVersion() == 3)
      write(data1, (uint8_t)(BESPSubReg_3 * 32));
    else
      write(data1, (uint8_t)(BESPSubReg_1_2 * 32));
    write(data1, (uint8_t)DW_OP_INTEL_regval_bits);
    write(data1, (uint8_t)32);
    encodeScratchAddrSpace(data1);
  } else {
    write(data1, (uint8_t)llvm::dwarf::DW_OP_lit0);
  }
  writeULEB128(data, data1.size());
  for (auto item : data1)
    write(data, (uint8_t)item);

  while ((lenSize + data.size()) % PointerSize != 0)
    // Insert DW_CFA_nop
    write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

  // Emit length with marker 0xffffffff for 8-byte ptr
  if (PointerSize == 8)
    Asm->EmitInt32(0xffffffff);
  Asm->EmitIntValue(data.size(), PointerSize);

  if (EmitSettings.EnableRelocation) {
    uint32_t ByteOffset = 0;

    for (auto it = data.begin(); it != data.end(); ++it) {
      auto byte = *it;
      if (ByteOffset++ == (uint32_t)LabelOffset) {
        auto Label = GetLabelBeforeIp(genOffStart);
        Asm->EmitLabelReference(Label, PointerSize, false);
        // Now skip PointerSize number of bytes from data
        std::advance(it, PointerSize);
        byte = *it;
      }
      Asm->EmitInt8(byte);
    }
  } else {
    for (auto &byte : data)
      Asm->EmitInt8(byte);
  }
}

void DwarfDebug::writeFDEStackCall(VISAModule *m) {
  std::vector<uint8_t> data;
  uint64_t loc = 0;
  uint64_t LabelOffset = std::numeric_limits<uint64_t>::max();
  // <ip, <instructions to write> >
  auto sortAsc = [](uint64_t a, uint64_t b) { return a < b; };
  std::map<uint64_t, std::vector<uint8_t>, decltype(sortAsc)> cfaOps(sortAsc);
  const auto &DbgInfo = *VisaDbgInfo;
  auto specialGRF = GetSpecialGRF();

  auto advanceLoc = [&loc](std::vector<uint8_t> &data, uint64_t newLoc) {
    uint64_t diff = newLoc - loc;
    if (diff == 0)
      return;

    if (diff < (1 << (8 * sizeof(uint8_t) - 1))) {
      write(data, (uint8_t)llvm::dwarf::DW_CFA_advance_loc1);
      write(data, (uint8_t)diff);
    } else if (diff < (1 << (8 * sizeof(uint16_t) - 1))) {
      write(data, (uint8_t)llvm::dwarf::DW_CFA_advance_loc2);
      write(data, (uint16_t)diff);
    } else {
      write(data, (uint8_t)llvm::dwarf::DW_CFA_advance_loc4);
      write(data, (uint32_t)diff);
    }
    loc = newLoc;
  };

  // When useBEFP is true, expression refers to BEFP. Otherwise,
  // expression refers to BESP.
  auto writeOffBEStack = [specialGRF, this](std::vector<uint8_t> &data, bool useBEFP) {
    // DW_OP_const4u 127
    // DW_OP_const2u BE_FP_SubReg
    // DW_OP_INTEL_regval_bits 32
    // DW_OP_breg6
    // DW_OP_plus

    std::vector<uint8_t> data1;

    auto DWRegEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF);
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const4u);
    write(data1, (uint32_t)(DWRegEncoded));
    write(data1, (uint8_t)llvm::dwarf::DW_OP_const2u);
    write(data1, (uint16_t)((useBEFP ? getBEFPSubReg() : getBESPSubReg()) * 4 * 8));
    write(data1, (uint8_t)DW_OP_INTEL_regval_bits);
    write(data1, (uint8_t)32);

    if (EmitSettings.ScratchOffsetInOW) {
      // when scratch offset is in OW, be_fp has to be multiplied by 16
      // to normalize and generate byte offset for complete address
      // computation.
      write(data1, (uint8_t)llvm::dwarf::DW_OP_const1u);
      write(data1, (uint8_t)16);
      write(data1, (uint8_t)llvm::dwarf::DW_OP_mul);
    }

    // indicate that the resulting address is on BE stack
    encodeScratchAddrSpace(data1);

    writeULEB128(data, data1.size());
    for (auto item : data1)
      write(data, (uint8_t)item);
  };

  auto writeLR = [this, writeOffBEStack](std::vector<uint8_t> &data, const DbgDecoder::LiveIntervalGenISA &lr,
                                         bool useBEFP) {
    if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeMemory) {
      writeOffBEStack(data, useBEFP);

      IGC_ASSERT_MESSAGE(!lr.var.mapping.m.isBaseOffBEFP, "Expecting location offset from BE_FP");
    } else if (lr.var.physicalType == DbgDecoder::VarAlloc::PhysicalVarType::PhyTypeGRF) {
      IGC_ASSERT_MESSAGE(false, "Not expecting CFA to be in non-GRF location");
    }
  };

  auto writeUndefined = [](std::vector<uint8_t> &data, uint32_t srcReg) {
    write(data, (uint8_t)llvm::dwarf::DW_CFA_undefined);
    writeULEB128(data, srcReg);
  };

  auto writeSameValue = [](std::vector<uint8_t> &data, uint32_t srcReg) {
    write(data, (uint8_t)llvm::dwarf::DW_CFA_same_value);
    writeULEB128(data, srcReg);
  };

  // Caller has written DW_CFA_expression and a register. Here, we
  // emit offset off CFA where the register is stored. For eg,
  // DW_CFA_expression: r143 (DW_OP_const4u: 0; DW_OP_plus)
  //
  // means that r127 is stored at [CFA + 0]
  auto writeCFAExpr = [](std::vector<uint8_t> &data, uint32_t offset) {
    std::vector<uint8_t> data1;

    write(data1, (uint8_t)llvm::dwarf::DW_OP_const4u);
    write(data1, (uint32_t)offset);
    write(data1, (uint8_t)llvm::dwarf::DW_OP_plus);

    writeULEB128(data, data1.size());
    for (auto item : data1)
      write(data, (uint8_t)item);
  };

  const auto &CFI = DbgInfo.getCFI();
  // Emit CIE
  uint8_t lenSize = 4;
  if (PointerSize == 8)
    lenSize = 12;

  // CIE_ptr (4/8 bytes)
  write(data, PointerSize == 4 ? (uint32_t)offsetCIEStackCall : (uint64_t)offsetCIEStackCall);

  // initial location
  auto genOffStart = DbgInfo.getRelocOffset();
  auto genOffEnd = highPc;

  // LabelOffset holds offset where start %ip is written to buffer.
  // Code later uses this to insert label for relocation.
  LabelOffset = data.size();
  if (EmitSettings.EnableRelocation) {
    write(data, PointerSize == 4 ? (uint32_t)0xfefefefe : (uint64_t)0xfefefefefefefefe);
  } else {
    write(data, PointerSize == 4 ? (uint32_t)genOffStart : (uint64_t)genOffStart);
  }

  // address range
  write(data, PointerSize == 4 ? (uint32_t)(genOffEnd - genOffStart) : (uint64_t)(genOffEnd - genOffStart));

  const unsigned int MovGenInstSizeInBytes = 16;

  // write CFA
  if (CFI.callerbefpValid) {
    const auto &callerFP = CFI.callerbefp;
    for (const auto &item : callerFP) {
      // map out CFA to an offset on be stack
      write(cfaOps[item.start], (uint8_t)llvm::dwarf::DW_CFA_def_cfa_expression);
      writeLR(cfaOps[item.start], item, true);
    }

    // describe r125 is at [r125.3]:ud
    auto ip = callerFP.front().start;
    write(cfaOps[ip], (uint8_t)llvm::dwarf::DW_CFA_expression);
    writeULEB128(cfaOps[ip], GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));
    writeCFAExpr(cfaOps[ip], 0);
    writeSameValue(cfaOps[callerFP.back().end + MovGenInstSizeInBytes],
                   GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));
  }

  // write return addr on stack
  if (CFI.retAddrValid) {
    auto &retAddr = CFI.retAddr;
    for (auto &item : retAddr) {
      // start live-range
      write(cfaOps[item.start], (uint8_t)llvm::dwarf::DW_CFA_expression);
      writeULEB128(cfaOps[item.start], RegisterNumbering::IP);
      writeCFAExpr(cfaOps[item.start], getRetIPSubReg() * 4);

      // end live-range
      // VISA emits following:
      // 624: ...
      // 640: (W) mov (4|M0) r125.0<1>:ud  r59.4<4;4,1>:ud <-- restore ret %ip,
      // and caller 656: ret (8|M0) r125.0:ud

      // VISA dbg:
      // Return addr saved at:
      // Live intervals :
      // (64, 640) @ Spilled(offset = 0 bytes) (off be_fp)

      // As per VISA debug info, return %ip restore instruction is at offset 640
      // above. But when we stop at 640, we still want ret %ip to be read from
      // frame descriptor in memory as current frame is still value. Only from
      // offset 656 should we read ret %ip from r125 directly. This is achieved
      // by taking offset 640 reported by VISA debug info and adding 16 to it
      // which is size of the mov instruction.
      // move return address register to actual location
      if (GetABIVersion() < 3) {
        // DW_CFA_register     IP      specialGRF
        write(cfaOps[item.end + MovGenInstSizeInBytes], (uint8_t)llvm::dwarf::DW_CFA_register);
        writeULEB128(cfaOps[item.end + MovGenInstSizeInBytes], RegisterNumbering::IP);
        writeULEB128(cfaOps[item.end + MovGenInstSizeInBytes],
                     GetEncodedRegNum<RegisterNumbering::GRFBase>(specialGRF));
      } else {
        // DW_CFA_restore restores value of IP register as defined in CIE. This is valid in epilog once we restore r127.
        write(cfaOps[item.end + MovGenInstSizeInBytes], (uint8_t)(llvm::dwarf::DW_CFA_restore | RegisterNumbering::IP));
      }
    }
  } else {
    if (m->GetType() == VISAModule::ObjectType::KERNEL) {
      // set return location to be undefined in top frame
      writeUndefined(cfaOps[0], RegisterNumbering::IP);
    }
  }

  // write channel enable (currently only in -O0)
  if (CFI.CEOffsetFromFPOff != 0xffff) {
    write(cfaOps[CFI.CEStoreIP], (uint8_t)llvm::dwarf::DW_CFA_expression);
    writeULEB128(cfaOps[CFI.CEStoreIP], RegisterNumbering::EMask);
    writeCFAExpr(cfaOps[CFI.CEStoreIP], CFI.CEOffsetFromFPOff);

    // set EMask to undefined when epilog starts to prevent
    // dereferencing invalid CFA
    writeUndefined(cfaOps[CFI.callerbefp.back().end + MovGenInstSizeInBytes], RegisterNumbering::EMask);
  }

  // write callee save
  if (CFI.calleeSaveEntry.size() > 0) {
    // set holds any callee save GRF that has been saved already to stack.
    // this is required because of some differences between dbginfo structure
    // reporting callee save and dwarf's debug_frame section requirements.
    std::set<uint32_t> calleeSaveRegsSaved;
    for (auto &item : CFI.calleeSaveEntry) {
      for (unsigned int idx = 0; idx != item.data.size(); ++idx) {
        auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->getGRFSizeInBytes());
        if (calleeSaveRegsSaved.find(regNum) == calleeSaveRegsSaved.end()) {
          write(cfaOps[item.genIPOffset], (uint8_t)llvm::dwarf::DW_CFA_expression);
          writeULEB128(cfaOps[item.genIPOffset], GetEncodedRegNum<RegisterNumbering::GRFBase>(regNum));
          writeCFAExpr(cfaOps[item.genIPOffset], (uint32_t)item.data[idx].dst.m.memoryOffset);
          calleeSaveRegsSaved.insert(regNum);
        } else {
          // already saved, so no need to emit same save again
        }
      }

      // check whether an entry is present in calleeSaveRegsSaved but not in
      // CFI.calleeSaveEntry missing entries are available in original locations
      for (auto it = calleeSaveRegsSaved.begin(); it != calleeSaveRegsSaved.end();) {
        bool found = false;
        for (unsigned int idx = 0; idx != item.data.size(); ++idx) {
          auto regNum = (uint32_t)item.data[idx].srcRegOff / (m_pModule->getGRFSizeInBytes());
          if ((*it) == regNum) {
            found = true;
            break;
          }
        }
        if (!found) {
          writeSameValue(cfaOps[item.genIPOffset], GetEncodedRegNum<RegisterNumbering::GRFBase>((*it)));
          it = calleeSaveRegsSaved.erase(it);
          continue;
        }
        ++it;
      }
    }
  }

  // write to actual buffer
  advanceLoc(data, loc);
  for (auto &item : cfaOps) {
    advanceLoc(data, item.first);
    for (auto &c : item.second) {
      data.push_back(c);
    }
  }

  // initial instructions (array of ubyte)
  while ((lenSize + data.size()) % PointerSize != 0)
    // Insert DW_CFA_nop
    write(data, (uint8_t)llvm::dwarf::DW_CFA_nop);

  // Emit length with marker 0xffffffff for 8-byte ptr
  if (PointerSize == 8)
    Asm->EmitInt32(0xffffffff);
  Asm->EmitIntValue(data.size(), PointerSize);

  if (EmitSettings.EnableRelocation) {
    uint32_t ByteOffset = 0;

    for (auto it = data.begin(); it != data.end(); ++it) {
      auto byte = *it;
      if (ByteOffset++ == (uint32_t)LabelOffset) {
        auto Label = GetLabelBeforeIp(genOffStart);
        Asm->EmitLabelReference(Label, PointerSize, false);
        // Now skip PointerSize number of bytes from data
        std::advance(it, PointerSize);
        byte = *it;
      }
      Asm->EmitInt8(byte);
    }
  } else {
    for (auto &byte : data)
      Asm->EmitInt8(byte);
  }
}

llvm::MCSymbol *DwarfDebug::GetLabelBeforeIp(unsigned int ip) {
  auto it = LabelsBeforeIp.find(ip);
  if (it != LabelsBeforeIp.end())
    return (*it).second;
  auto NewLabel = Asm->CreateTempSymbol();
  LabelsBeforeIp[ip] = NewLabel;
  return NewLabel;
}

// If this type is derived from a base type then return base type size.
uint64_t DwarfDebug::getBaseTypeSize(const llvm::DIType *Ty) {
  IGC_ASSERT(Ty);
  const DIDerivedType *DDTy = dyn_cast<DIDerivedType>(Ty);
  if (!DDTy)
    return Ty->getSizeInBits();

  unsigned Tag = DDTy->getTag();

  if (Tag != dwarf::DW_TAG_member && Tag != dwarf::DW_TAG_typedef && Tag != dwarf::DW_TAG_const_type &&
      Tag != dwarf::DW_TAG_volatile_type && Tag != dwarf::DW_TAG_restrict_type && Tag != dwarf::DW_TAG_atomic_type &&
      Tag != dwarf::DW_TAG_immutable_type)
    return DDTy->getSizeInBits();

  DIType *BaseType = DDTy->getBaseType();

  if (!BaseType) {
    IGC_ASSERT_MESSAGE(false, "Empty base type!");
    return 0;
  }

  // If this is a derived type, go ahead and get the base type, unless it's a
  // reference then it's just the size of the field. Pointer types have no need
  // of this since they're a different type of qualification on the type.
  if (BaseType->getTag() == dwarf::DW_TAG_reference_type || BaseType->getTag() == dwarf::DW_TAG_rvalue_reference_type)
    return Ty->getSizeInBits();

  return getBaseTypeSize(BaseType);
}

void DbgVariable::print(raw_ostream &O, bool NestedAbstract) const {
  auto makePrefix = [](unsigned SpaceNum, const char *Pfx) { return std::string(SpaceNum, ' ').append(Pfx); };

  auto Prefix = makePrefix(NestedAbstract ? 8 : 4, "| ");

  O << "DbgVariable: {\n";
  if (Var) {
    O << Prefix << "Name: " << Var->getName() << ";\n";
    O << Prefix << "Type: " << *getType() << ";\n";
    O << Prefix << "Props: { IsParameter: " << Var->isParameter() << ", ";
    O << "IsArtificial: " << Var->isArtificial() << ", ";
    O << "IsObjPtr: " << isObjectPointer() << " } ;\n";
  } else {
    O << Prefix << "Name/Type: <null>;\n";
  }

  if (IA)
    O << Prefix << "InlinedAt: " << *IA << ";\n";
  else
    O << Prefix << "InlinedAt: none;\n";

  if (AbsVar) {
    O << Prefix << "AbsVar: ";
    if (NestedAbstract)
      O << "<hidden>;\n";
    else
      AbsVar->print(O, true);
  } else {
    O << Prefix << "AbsVar: none;\n";
  }

  if (m_pDbgInst) {
    if (isa<DbgInfoIntrinsic>(m_pDbgInst)) {
      O << Prefix << "ValInst: ";
      DbgVariable::printDbgInst(O, m_pDbgInst, makePrefix(NestedAbstract ? 14 : 8, "").c_str());
    } else
      O << Prefix << "ValInst: " << *m_pDbgInst << "\n";
  } else
    O << Prefix << "ValInst: " << "none;\n";

  O << makePrefix(NestedAbstract ? 4 : 0, "") << "}\n";
}

void DbgVariable::printDbgInst(llvm::raw_ostream &O, const llvm::Instruction *Inst, const char *NodePrefixes) {
  IGC_ASSERT(Inst);
  const auto *DbgInfoInst = cast<DbgInfoIntrinsic>(Inst);
  auto OperandNum = DbgInfoInst->getNumOperands();
  IGC_ASSERT(OperandNum > 0);
  // last operand is usually function attributes, so we skip them
  auto OperandsToPrint = OperandNum - 1;
  O << *DbgInfoInst << "\n";
  for (unsigned OperandIdx = 0; OperandIdx < OperandsToPrint; ++OperandIdx) {
    O << NodePrefixes << "node#" << OperandIdx << " " << *DbgInfoInst->getOperand(OperandIdx) << "\n";
  }
}

#ifndef NDEBUG
void DbgVariable::dump() const { print(dbgs(), false); }

void DbgVariable::dumpDbgInst(const llvm::Instruction *Inst) {
  IGC_ASSERT(Inst);
  printDbgInst(dbgs(), Inst);
}
#endif // NDEBUG

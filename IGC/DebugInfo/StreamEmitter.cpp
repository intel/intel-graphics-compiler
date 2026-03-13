/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfoELF.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MD5.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/ADT/STLExtras.h"
#include "llvmWrapper/MC/MCStreamer.h"
#include "llvmWrapper/ADT/Optional.h"
#include "llvmWrapper/Support/TargetRegistry.h"
// clang-format on

#include "StreamEmitter.hpp"
#include "DIE.hpp"
#include "DwarfDebug.hpp"

#include "Probe/Assertion.h"

#define DEBUG_TYPE "dwarfdebug"

using namespace llvm;
using namespace IGC;

namespace IGC {
///////////////////////////////////////////////////////////////////////////////
// Following classes extend abstract MC classes.
// These classes are needed to create concrete instance of MCStreamer.
///////////////////////////////////////////////////////////////////////////////

class VISAMCAsmInfo : public MCAsmInfoELF {
public:
  VISAMCAsmInfo(unsigned int pointerSize) : MCAsmInfoELF() {
    DwarfUsesRelocationsAcrossSections = true;
    CodePointerSize = pointerSize;
  }
};

class VISAELFObjectWriter : public MCELFObjectTargetWriter {
public:
  VISAELFObjectWriter(uint8_t osABI, uint16_t eMachine)
      : MCELFObjectTargetWriter(true /* is64Bit */, osABI, eMachine, true /* hasRelocationAddend */) {}

  unsigned getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel) const {
    IGC_ASSERT_MESSAGE(IsPCRel == false, "expecting non-PC relative reloc type");
    unsigned type = ELF::R_X86_64_NONE;

    switch ((unsigned)Fixup.getKind()) {
    default:
      IGC_ASSERT_EXIT_MESSAGE(0, "invalid fixup kind!");
    case FK_Data_8:
      IGC_ASSERT_MESSAGE(Target.isAbsolute() || Target.getSymA()->getKind() == MCSymbolRefExpr::VK_None,
                         "expecting absolute target reloc");
      type = ELF::R_X86_64_64;
      break;
    case FK_Data_4:
      type = ELF::R_X86_64_32;
      break;
    case FK_Data_2:
      type = ELF::R_X86_64_16;
      break;
    case FK_Data_1:
      type = ELF::R_X86_64_8;
      break;
    }

    return type;
  }
};

class VISAAsmBackend : public MCAsmBackend {
  StringRef m_targetTriple;

public:
  VISAAsmBackend(StringRef targetTriple) : MCAsmBackend(support::endianness::little), m_targetTriple(targetTriple) {}

  unsigned getNumFixupKinds() const override { return 0; }

  static unsigned getFixupKindLog2Size(unsigned Kind) {
    switch (Kind) {
    default:
      IGC_ASSERT_EXIT_MESSAGE(0, "invalid fixup kind!");
    case FK_Data_1:
      return 0;
    case FK_Data_2:
      return 1;
    case FK_Data_4:
      return 2;
    case FK_Data_8:
      return 3;
    }
  }

  void applyFixup(const MCAssembler &Asm, const MCFixup &fixup, const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t value, bool IsResolved, const MCSubtargetInfo *STI) const override {
    unsigned size = 1 << getFixupKindLog2Size(fixup.getKind());

    IGC_ASSERT_MESSAGE(fixup.getOffset() + size <= Data.size(), "Invalid fixup offset!");

    // Check that uppper bits are either all zeros or all ones.
    // Specifically ignore overflow/underflow as long as the leakage is
    // limited to the lower bits. This is to remain compatible with
    // other assemblers.
    IGC_ASSERT_MESSAGE(isIntN(size * 8 + 1, value), "value does not fit in the fixup field");

    for (unsigned i = 0; i != size; ++i) {
      Data[fixup.getOffset() + i] = uint8_t(value >> (i * 8));
    }
  }

  bool mayNeedRelaxation(const MCInst &inst, const MCSubtargetInfo &STI) const override {
    // TODO: implement this
    IGC_ASSERT_EXIT_MESSAGE(0, "Unimplemented");
    return false;
  }

  bool fixupNeedsRelaxation(const MCFixup &fixup, uint64_t value, const MCRelaxableFragment *pDF,
                            const MCAsmLayout &layout) const override {
    // TODO: implement this
    IGC_ASSERT_EXIT_MESSAGE(0, "Unimplemented");
    return false;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count, const MCSubtargetInfo *STI) const override {
    const char nop = (char)0x90;
    for (uint64_t i = 0; i < Count; ++i) {
      OS.write(&nop, 1);
    }
    return true;
  }

  std::unique_ptr<MCObjectTargetWriter> createObjectTargetWriter() const override {
    // TODO: implement this
    IGC_ASSERT_UNREACHABLE(); // Unimplemented
  }
}; // namespace IGC

class VISAMCCodeEmitter : public MCCodeEmitter {
  /// EncodeInstruction - Encode the given \p inst to bytes on the output
  /// stream \p OS.
  virtual void encodeInstruction(const MCInst &inst, raw_ostream &os, SmallVectorImpl<MCFixup> &fixups,
                                 const MCSubtargetInfo &m) const {
    // TODO: implement this
    IGC_ASSERT_EXIT_MESSAGE(0, "Unimplemented");
  }

public:
  VISAMCCodeEmitter() = default;
  ~VISAMCCodeEmitter() = default;
  VISAMCCodeEmitter(const VISAMCCodeEmitter &) = delete;
  VISAMCCodeEmitter &operator=(const VISAMCCodeEmitter &) = delete;
};

} // namespace IGC

StreamEmitter::StreamEmitter(raw_pwrite_stream &outStream, const std::string &dataLayout,
                             const std::string &targetTriple, const StreamEmitter::Settings &Options)
    : m_targetTriple(targetTriple), m_setCounter(0), StreamOptions(Options) {
  m_pDataLayout = new DataLayout(dataLayout);
  m_pSrcMgr = new SourceMgr();
  m_pAsmInfo = new VISAMCAsmInfo(GetPointerSize());
  m_pObjFileInfo = new IGCLLVM::MCObjectFileInfo();

  MCRegisterInfo *regInfo = nullptr;
  Triple triple = Triple(GetTargetTriple());

  // Create new MC context
  m_pContext =
      IGCLLVM::CreateMCContext(triple, (const llvm::MCAsmInfo *)m_pAsmInfo, regInfo, m_pObjFileInfo, m_pSrcMgr);

  m_pObjFileInfo->InitMCObjectFileInfo(triple, false, *m_pContext);

  uint8_t osABI = MCELFObjectTargetWriter::getOSABI(triple.getOS());
  // Earlier eMachine was set to ELF::EM_X86_64 or ELF::EM_386
  // This creates a problem for gdb so it is now set to 182
  // which is an encoding reserved for Intel. It is not part of
  // the enum so its value in inlined.
#define EM_INTEL_GEN 182
  uint16_t eMachine = EM_INTEL_GEN;
  if (StreamOptions.EnforceAMD64Machine)
    eMachine = ELF::EM_X86_64;
  std::unique_ptr<MCAsmBackend> pAsmBackend = IGCLLVM::make_unique<VISAAsmBackend>(GetTargetTriple());
  std::unique_ptr<MCELFObjectTargetWriter> pTargetObjectWriter =
      IGCLLVM::make_unique<VISAELFObjectWriter>(osABI, eMachine);
  std::unique_ptr<MCObjectWriter> pObjectWriter =
      createELFObjectWriter(std::move(pTargetObjectWriter), outStream, true);
  std::unique_ptr<MCCodeEmitter> pCodeEmitter = IGCLLVM::make_unique<VISAMCCodeEmitter>();

  bool isRelaxAll = false;
  bool isNoExecStack = false;
  m_pMCStreamer = createELFStreamer(*m_pContext, std::move(pAsmBackend), std::move(pObjectWriter),
                                    std::move(pCodeEmitter), isRelaxAll);

  IGCLLVM::initSections(m_pMCStreamer, isNoExecStack, m_pContext);
}

StreamEmitter::~StreamEmitter() {
  delete m_pMCStreamer;
  delete m_pContext;
  delete m_pSrcMgr;
  delete m_pAsmInfo;
  delete m_pObjFileInfo;
  delete m_pDataLayout;
}

unsigned int StreamEmitter::GetPointerSize() const { return DwarfDebug::PointerSize; }

bool StreamEmitter::IsLittleEndian() const { return m_pDataLayout->isLittleEndian(); }

const MCSection *StreamEmitter::GetTextSection() const { return GetObjFileLowering().getTextSection(); }

const MCSection *StreamEmitter::GetDataSection() const { return GetObjFileLowering().getDataSection(); }

const MCSection *StreamEmitter::GetDwarfAbbrevSection() const { return GetObjFileLowering().getDwarfAbbrevSection(); }

const MCSection *StreamEmitter::GetDwarfInfoSection() const { return GetObjFileLowering().getDwarfInfoSection(); }

const MCSection *StreamEmitter::GetDwarfLineSection() const { return GetObjFileLowering().getDwarfLineSection(); }

const MCSection *StreamEmitter::GetDwarfLocSection() const { return GetObjFileLowering().getDwarfLocSection(); }

const MCSection *StreamEmitter::GetDwarfMacroInfoSection() const {
  // return GetObjFileLowering().getDwarfMacroInfoSection();
  return nullptr;
}

const MCSection *StreamEmitter::GetDwarfRangesSection() const { return GetObjFileLowering().getDwarfRangesSection(); }

const MCSection *StreamEmitter::GetDwarfStrSection() const { return GetObjFileLowering().getDwarfStrSection(); }

const MCSection *StreamEmitter::GetDwarfFrameSection() const { return GetObjFileLowering().getDwarfFrameSection(); }

const MCSection *StreamEmitter::GetDwarfAddrSection() const { return GetObjFileLowering().getDwarfAddrSection(); }

const MCSection *StreamEmitter::GetDwarfLoclistsSection() const {
  return GetObjFileLowering().getDwarfLoclistsSection();
}

const MCSection *StreamEmitter::GetDwarfRnglistsSection() const {
  return GetObjFileLowering().getDwarfRnglistsSection();
}

void StreamEmitter::SwitchSection(const MCSection *pSection, const MCExpr *pSubsection) const {
  IGCLLVM::switchSection(m_pMCStreamer, const_cast<MCSection *>(pSection), pSubsection);
}

MCSymbol *StreamEmitter::GetSymbol(const GlobalValue *pGV) const {
  /*
  //Original code (as reference)
  SmallString<60> NameStr;
  M.getNameWithPrefix(NameStr, pGV, false);
  return m_pContext->GetOrCreateSymbol(NameStr.str());
  */
  IGC_ASSERT_MESSAGE(pGV->hasName(), "TODO: fix this case");
  return m_pContext->getOrCreateSymbol(Twine(m_pAsmInfo->getPrivateGlobalPrefix()) + pGV->getName());
}

MCSymbol *StreamEmitter::GetTempSymbol(StringRef name, uint64_t id) const {
  return m_pContext->getOrCreateSymbol(Twine(m_pAsmInfo->getPrivateGlobalPrefix()) + name + Twine(id));
}

MCSymbol *StreamEmitter::GetTempSymbol(StringRef name) const {
  return m_pContext->getOrCreateSymbol(Twine(m_pAsmInfo->getPrivateGlobalPrefix()) + name);
}

MCSymbol *StreamEmitter::CreateTempSymbol() const { return m_pContext->createTempSymbol(); }

MCSymbol *StreamEmitter::CreateTempSymbol(const Twine &Name) const { return m_pContext->createTempSymbol(Name, true); }

unsigned StreamEmitter::GetDwarfCompileUnitID() const { return m_pContext->getDwarfCompileUnitID(); }

void StreamEmitter::SetDwarfCompileUnitID(unsigned cuIndex) const { m_pContext->setDwarfCompileUnitID(cuIndex); }

void StreamEmitter::SetDwarfVersion(unsigned DwarfVersion) const { m_pContext->setDwarfVersion(DwarfVersion); }

void StreamEmitter::EmitBytes(StringRef data, unsigned addrSpace) const { m_pMCStreamer->emitBytes(data); }

void StreamEmitter::EmitValue(const MCExpr *value, unsigned size, unsigned addrSpace) const {

  m_pMCStreamer->emitValue(value, size);
}

void StreamEmitter::EmitIntValue(uint64_t value, unsigned size, unsigned addrSpace) const {
  m_pMCStreamer->emitIntValue(value, size);
}

void StreamEmitter::EmitInt8(int value) const { EmitIntValue(value, 1); }

void StreamEmitter::EmitInt16(int value) const { EmitIntValue(value, 2); }

void StreamEmitter::EmitInt32(int value) const { EmitIntValue(value, 4); }

void StreamEmitter::EmitSLEB128(int64_t value, const char * /*desc*/) const {
  m_pMCStreamer->emitSLEB128IntValue(value);
}

void StreamEmitter::EmitULEB128(uint64_t value, llvm::StringRef /*desc*/, unsigned padTo) const {
  m_pMCStreamer->emitULEB128IntValue(value);
}

void StreamEmitter::EmitLabel(MCSymbol *pLabel) const { m_pMCStreamer->emitLabel(pLabel); }

void StreamEmitter::EmitLabelDifference(const MCSymbol *pHi, const MCSymbol *pLo, unsigned size) const {
  const MCExpr *hiExpr = MCSymbolRefExpr::create(pHi, *m_pContext);
  const MCExpr *loExpr = MCSymbolRefExpr::create(pLo, *m_pContext);

  // Get the pHi-pLo expression.
  const MCExpr *pDiff = MCBinaryExpr::createSub(hiExpr, loExpr, *m_pContext);

  if (!m_pAsmInfo->doesSetDirectiveSuppressReloc()) {
    m_pMCStreamer->emitValue(pDiff, size);
    return;
  }

  // Otherwise, emit with .set (aka assignment).
  MCSymbol *pSetLabel = GetTempSymbol("set", m_setCounter++);

  m_pMCStreamer->emitAssignment(pSetLabel, pDiff);

  m_pMCStreamer->emitSymbolValue(pSetLabel, size);
}

void StreamEmitter::EmitLabelOffsetDifference(const MCSymbol *pHi, uint64_t Offset, const MCSymbol *pLo,
                                              unsigned size) const {
  const MCExpr *pHiExpr = MCSymbolRefExpr::create(pHi, *m_pContext);
  const MCExpr *pLoExpr = MCSymbolRefExpr::create(pLo, *m_pContext);
  const MCExpr *pOffsetExpr = MCConstantExpr::create(Offset, *m_pContext);

  // Emit pHi+Offset - pLo
  // Get the pHi+Offset expression.
  const MCExpr *pPlus = MCBinaryExpr::createAdd(pHiExpr, pOffsetExpr, *m_pContext);

  // Get the pHi+Offset-pLo expression.
  const MCExpr *pDiff = MCBinaryExpr::createSub(pPlus, pLoExpr, *m_pContext);

  if (!m_pAsmInfo->doesSetDirectiveSuppressReloc()) {
    m_pMCStreamer->emitValue(pDiff, size);
    return;
  }
  // Otherwise, emit with .set (aka assignment).
  MCSymbol *pSetLabel = GetTempSymbol("set", m_setCounter++);

  m_pMCStreamer->emitAssignment(pSetLabel, pDiff);

  m_pMCStreamer->emitSymbolValue(pSetLabel, size);
}

void StreamEmitter::EmitLabelPlusOffset(const MCSymbol *pLabel, uint64_t Offset, unsigned size,
                                        bool /*isSectionRelative*/) const {
  // Emit pLabel+Offset (or just pLabel if Offset is zero)
  const MCExpr *pLabelExpr = MCSymbolRefExpr::create(pLabel, *m_pContext);
  const MCExpr *pOffsetExpr = MCConstantExpr::create(Offset, *m_pContext);

  const MCExpr *pExpr = (Offset) ? MCBinaryExpr::createAdd(pLabelExpr, pOffsetExpr, *m_pContext) : pLabelExpr;

  m_pMCStreamer->emitValue(pExpr, size);
}

void StreamEmitter::EmitLabelReference(const MCSymbol *pLabel, unsigned size, bool isSectionRelative) const {
  EmitLabelPlusOffset(pLabel, 0, size, isSectionRelative);
}

void StreamEmitter::EmitELFDiffSize(MCSymbol *pLabel, const MCSymbol *pHi, const MCSymbol *pLo) const {
  const MCExpr *hiExpr = MCSymbolRefExpr::create(pHi, *m_pContext);
  const MCExpr *loExpr = MCSymbolRefExpr::create(pLo, *m_pContext);

  // Get the pHi-pLo expression.
  const MCExpr *pDiff = MCBinaryExpr::createSub(hiExpr, loExpr, *m_pContext);

  m_pMCStreamer->emitELFSize(pLabel, pDiff);
}

void StreamEmitter::EmitSymbolValue(const MCSymbol *pSym, unsigned size, unsigned addrSpace) const {
  m_pMCStreamer->emitSymbolValue(pSym, size);
}

void StreamEmitter::EmitSectionOffset(const MCSymbol *pLabel, const MCSymbol *pSectionLabel) const {
  // Get the section that we're referring to, based on pSectionLabel.
  [[maybe_unused]] const MCSection &section = pSectionLabel->getSection();

  // If pLabel has already been emitted, verify that it is in the same section
  // as section label for sanity.
  IGC_ASSERT_MESSAGE((!pLabel->isInSection() || &pLabel->getSection() == &section),
                     "section offset using wrong section base for label");

  // If the section in question will end up with an address of 0 anyway, we can
  // just emit an absolute reference to save a relocation.
#if 0
    if (section.isBaseAddressKnownZero())
    {
        m_pMCStreamer->EmitSymbolValue(pLabel, 4);
        return;
    }
#endif

  // Otherwise, emit it as a label difference from the start of the section.
  EmitLabelDifference(pLabel, pSectionLabel, 4);
}

MCSymbol *StreamEmitter::EmitDwarfUnitLength(const Twine &Prefix, const Twine &Comment) const {
  return m_pMCStreamer->emitDwarfUnitLength(Prefix, Comment);
}

void StreamEmitter::EmitDwarfRegOp(unsigned reg, unsigned offset, bool indirect) const {
  auto regEncoded = GetEncodedRegNum<RegisterNumbering::GRFBase>(reg);
  if (indirect) {
    if (regEncoded < 32) {
      EmitInt8(dwarf::DW_OP_breg0 + regEncoded);
    } else {
      // Emit ("DW_OP_bregx");
      EmitInt8(dwarf::DW_OP_bregx);
      EmitULEB128(regEncoded);
    }
    EmitSLEB128(offset);
  } else {
    if (regEncoded < 32) {
      EmitInt8(dwarf::DW_OP_reg0 + regEncoded);
    } else {
      // Emit ("DW_OP_regx");
      EmitInt8(dwarf::DW_OP_regx);
      EmitULEB128(regEncoded);
    }
  }
}

bool StreamEmitter::EmitDwarfFileDirective(unsigned fileNo, llvm::StringRef directory, llvm::StringRef filename,
                                           IGCLLVM::optional<llvm::MD5::MD5Result> checksum,
                                           IGCLLVM::optional<llvm::StringRef> source, unsigned cuID) const {
  auto result = m_pMCStreamer->emitDwarfFileDirective(fileNo, directory, filename, checksum, source, cuID) != 0;
  return result;
}

void StreamEmitter::EmitDwarfFile0Directive(unsigned fileNo, StringRef directory, StringRef filename,
                                            IGCLLVM::optional<llvm::MD5::MD5Result> checksum,
                                            IGCLLVM::optional<llvm::StringRef> source, unsigned cuID) const {
  m_pMCStreamer->emitDwarfFile0Directive(directory, filename, checksum, source, cuID);
}

void StreamEmitter::EmitDwarfLocDirective(unsigned fileNo, unsigned line, unsigned column, unsigned flags, unsigned isa,
                                          unsigned discriminator, StringRef fileName) const {
  m_pMCStreamer->emitDwarfLocDirective(fileNo, line, column, flags, isa, discriminator, fileName);
}

void StreamEmitter::SetMCLineTableSymbol(MCSymbol *pSym, unsigned id) const {
  //    m_pContext->setMCLineTableSymbol(pSym, id);
}

void StreamEmitter::Finalize() const {
  IGCLLVM::finish(m_pMCStreamer);
  m_pMCStreamer->reset();
}

const MCObjectFileInfo &StreamEmitter::GetObjFileLowering() const {
  IGC_ASSERT_MESSAGE(m_pObjFileInfo, "Object File Lowering was not initialized");
  return *m_pObjFileInfo;
}

void StreamEmitter::verifyRegisterLocationSize(const IGC::DbgVariable &VarVal, const IGC::DwarfDebug &DD,
                                               unsigned MaxGRFSpaceInBits, uint64_t ExpectedSize) {
  if (!GetEmitterSettings().EnableDebugInfoValidation)
    return;

  auto *DbgInst = VarVal.getDbgInst();
  IGC_ASSERT(DbgInst);
  auto *Ty = DbgInst->getType();
  IGC_ASSERT(Ty->isSingleValueType());

  if (Ty->isPointerTy())
    return; // no validation for pointers (for now)

  auto *Expr = DbgInst->getExpression();
  if (Expr->isFragment() || Expr->isImplicit()) {
    // TODO: implement some sanity checks
    return;
  }
  DiagnosticBuff Diag;
  auto DwarfTypeSize = DwarfDebug::getBaseTypeSize(VarVal.getType());
  if (DwarfTypeSize != ExpectedSize) {
    Diag.out() << "ValidationFailure [regLocSize] -- DWARF Type Size: " << DwarfTypeSize
               << ", expected: " << ExpectedSize << "\n";
  }
  if (ExpectedSize > MaxGRFSpaceInBits) {
    Diag.out() << "ValidationFailure [GRFSpace] -- Available GRF space: " << MaxGRFSpaceInBits
               << ", while expected value size: " << ExpectedSize << "\n";
  }

  // Dump DbgVariable if errors were reported
  verificationReport(VarVal, Diag);
}

void StreamEmitter::verifyRegisterLocationExpr(const DbgVariable &DV, const DwarfDebug &DD) {
  if (!GetEmitterSettings().EnableDebugInfoValidation)
    return;

  // TODO: add checks for locations other than llvm.dbg.value
  if (DV.currentLocationIsMemoryAddress())
    return;

  auto *DbgInst = DV.getDbgInst();
  if (!isa<llvm::DbgValueInst>(DbgInst))
    return;

  DiagnosticBuff Diag;
  if (!DV.currentLocationIsImplicit() && !DV.currentLocationIsSimpleIndirectValue()) {
    if (DbgInst->getExpression()->isComplex()) {
      Diag.out() << "ValidationFailure [UnexpectedComlexExpression]" << " for a simple register location\n";
    }
  }
  verificationReport(DV, Diag);
}

void StreamEmitter::reportUsabilityIssue(llvm::StringRef Msg, const llvm::Value *Ctx) {
  if (!GetEmitterSettings().EnableDebugInfoValidation)
    return;

  DiagnosticBuff Diag;
  Diag.out() << "ValidationFailure [UsabilityIssue] " << Msg << "\n";

  if (Ctx) {
    Ctx->print(Diag.out());
    Diag.out() << "\n";
  }

  verificationReport(Diag);
}

void StreamEmitter::verificationReport(const DbgVariable &VarVal, DiagnosticBuff &Diag) {
  if (Diag.out().tell() == 0)
    return;

  VarVal.print(Diag.out());
  Diag.out() << "==============\n";

  verificationReport(Diag);
}

void StreamEmitter::verificationReport(DiagnosticBuff &Diag) {
  if (Diag.out().tell() == 0)
    return;

  const auto &ErrMsg = Diag.out().str();

  ErrorLog.append(ErrMsg);
  LLVM_DEBUG(dbgs() << ErrMsg);
}

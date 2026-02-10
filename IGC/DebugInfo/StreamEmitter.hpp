/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/MC/MCSection.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MD5.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/MC/MCContext.h"
#include "llvmWrapper/MC/MCObjectFileInfo.h"
#include "llvmWrapper/ADT/Optional.h"
// clang-format on

#include <string>

#include "EmitterOpts.hpp"

namespace llvm {
class MCStreamer;
class SourceMgr;
class MCAsmInfo;
} // namespace llvm

namespace IGC {
class DbgVariable;
class DwarfDebug;
class VISAVariableLocation;

/// @brief StreamEmitter provides API methods for emitting elf object.
///        It will be used to emit the debug info sections in dwarf format.
class StreamEmitter {
public:
  typedef DebugEmitterOpts Settings;
  /// @brief Constructor.
  /// @param raw_ostream instance of raw_ostream to emit all bitcode into.
  /// @param dataLayout data layout string.
  /// @param targetTriple target triple string.
  StreamEmitter(llvm::raw_pwrite_stream &, const std::string &dataLayout, const std::string &targetTriple,
                const Settings &EmitterOptions);

  /// @brief Destructor.
  ~StreamEmitter();
  StreamEmitter(const StreamEmitter &) = delete;
  StreamEmitter &operator=(const StreamEmitter &) = delete;

  const Settings &GetEmitterSettings() const { return StreamOptions; };

  /// @brief Get pointer size.
  /// @return pointer size in bits.
  unsigned int GetPointerSize() const;

  /// @brief Get endianness data stored in memory.
  /// @return true for Little-endian, and false for Big-endian.
  bool IsLittleEndian() const;

  /// @brief Section getter methods
  const llvm::MCSection *GetTextSection() const;
  const llvm::MCSection *GetDataSection() const;
  const llvm::MCSection *GetDwarfAbbrevSection() const;
  const llvm::MCSection *GetDwarfInfoSection() const;
  const llvm::MCSection *GetDwarfLineSection() const;
  const llvm::MCSection *GetDwarfLocSection() const;
  const llvm::MCSection *GetDwarfMacroInfoSection() const;
  const llvm::MCSection *GetDwarfRangesSection() const;
  const llvm::MCSection *GetDwarfStrSection() const;
  const llvm::MCSection *GetDwarfFrameSection() const;

  /// @brief Set the current section where code is being emitted to.
  /// @param pSection section to switch to
  /// @param pSubsection subsiction to switch to (optional)
  void SwitchSection(const llvm::MCSection *pSection, const llvm::MCExpr *pSubsection = 0) const;

  /// @brief sympol getters
  /// @param pGV Global Variable
  /// @return Machine Code symbol
  llvm::MCSymbol *GetSymbol(const llvm::GlobalValue *pGV) const;

  /// @brief Return the MCSymbol corresponding to the assembler
  ///        temporary label with the specified stem and unique ID.
  /// @param name symbol name
  /// @param id symbol id
  /// @return Machine Code symbol
  llvm::MCSymbol *GetTempSymbol(llvm::StringRef name, uint64_t id) const;

  /// @brief Return an assembler temporary label with the specified stem.
  /// @param name symbol name
  /// @return Machine Code symbol
  llvm::MCSymbol *GetTempSymbol(llvm::StringRef name) const;

  /// @brief Return an assembler temporary label with the specified stem.
  /// @return Machine Code symbol
  llvm::MCSymbol *CreateTempSymbol() const;

  /// @brief Dwarf CU getter & setter
  unsigned GetDwarfCompileUnitID() const;
  void SetDwarfCompileUnitID(unsigned cuIndex) const;

  /// @brief Update the maximum version of dwarf that LLVM should emit
  void SetDwarfVersion(unsigned DwarfVersion) const;

  //===------------------------------------------------------------------===//
  // Dwarf Emission Helper Routines
  //===------------------------------------------------------------------===//

  /// @brief Emit the bytes in \p Data into the output.
  void EmitBytes(llvm::StringRef data, unsigned addrSpace = 0) const;

  /// @brief Emit the expression @p value into the output as a native
  ///        integer of the given @p size bytes.
  void EmitValue(const llvm::MCExpr *pValue, unsigned size, unsigned addrSpace = 0) const;

  /// @brief Special case of EmitValue that avoids the client having
  ///        to pass in a MCExpr for constant integers.
  void EmitIntValue(uint64_t value, unsigned size, unsigned addrSpace = 0) const;

  /// @brief Emit a byte directive and value.
  void EmitInt8(int value) const;

  /// @brief Emit a short directive and value.
  void EmitInt16(int value) const;

  /// @brief Emit a long directive and value.
  void EmitInt32(int value) const;

  /// @brief emit the specified signed leb128 value.
  void EmitSLEB128(int64_t value, const char *pDesc = 0) const;

  /// @brief emit the specified unsigned leb128 value.
  void EmitULEB128(uint64_t value, llvm::StringRef desc = "", unsigned padTo = 0) const;

  /// @brief Emit a label
  void EmitLabel(llvm::MCSymbol *pLabel) const;

  /// @brief Emit something like ".long pHi-pLo" where the size in bytes
  ///        of the directive is specified by size and pHi/pLo specify the
  ///        labels.  This implicitly uses .set if it is available.
  void EmitLabelDifference(const llvm::MCSymbol *pHi, const llvm::MCSymbol *pLo, unsigned size) const;

  /// @brief Emit something like ".long pHi+offset-pLo" where the size in bytes
  ///        of the directive is specified by size and pHi/pLo specify the
  ///        labels.  This implicitly uses .set if it is available.
  void EmitLabelOffsetDifference(const llvm::MCSymbol *pHi, uint64_t offset, const llvm::MCSymbol *pLo,
                                 unsigned size) const;

  /// @brief Emit something like ".long pLabel+offset" where the size in bytes
  ///        of the directive is specified by size and pLabel specifies the
  ///        label.  This implicitly uses .set if it is available.
  void EmitLabelPlusOffset(const llvm::MCSymbol *pLabel, uint64_t offset, unsigned size,
                           bool isSectionRelative = false) const;

  /// @brief Emit something like ".long pLabel" where the size in bytes of the
  ///        directive is specified by size and pLabel specifies the label.
  void EmitLabelReference(const llvm::MCSymbol *pLabel, unsigned size, bool isSectionRelative = false) const;

  void EmitELFDiffSize(llvm::MCSymbol *pLabel, const llvm::MCSymbol *pHi, const llvm::MCSymbol *pLo) const;

  /// @brief Special case of EmitValue that avoids the client
  ///        having to pass in a MCExpr for MCSymbols.
  void EmitSymbolValue(const llvm::MCSymbol *pSym, unsigned size, unsigned addrSpace = 0) const;

  /// @brief Emit the 4-byte offset of pLabel from the start of its section.
  ///        This can be done with a special directive if the target supports
  ///        it (e.g. cygwin) or by emitting it as an offset from a label at
  ///        the start of the section.
  ///
  ///        SectionLabel is a temporary label emitted at the start of the
  ///        section that pLabel lives in.
  void EmitSectionOffset(const llvm::MCSymbol *pLabel, const llvm::MCSymbol *pSectionLabel) const;

  /// @brief Emit dwarf register operation.
  void EmitDwarfRegOp(unsigned reg, unsigned offset = 0, bool indirect = 0) const;

  /// @brief Associate a filename with a specified logical file number.
  ///        This implements the DWARF2 '.file 4 "foo.c"' assembler directive.
  bool EmitDwarfFileDirective(unsigned fileNo, llvm::StringRef directory, llvm::StringRef filename,
                              IGCLLVM::optional<llvm::MD5::MD5Result> Checksum,
                              IGCLLVM::optional<llvm::StringRef> Source, unsigned cuID) const;

  /// @brief Specify the "root" file of the compilation, using the ".file 0" extension.
  void EmitDwarfFile0Directive(unsigned fileNo, llvm::StringRef directory, llvm::StringRef filename,
                               IGCLLVM::optional<llvm::MD5::MD5Result> Checksum,
                               IGCLLVM::optional<llvm::StringRef> Source, unsigned cuID = 0) const;

  /// @brief This implements the DWARF2 '.loc fileno lineno ...' assembler
  /// directive.
  void EmitDwarfLocDirective(unsigned fileNo, unsigned line, unsigned column, unsigned flags, unsigned isa,
                             unsigned discriminator, llvm::StringRef fileName) const;

  /// @brief Maps given line table symbol to given ID
  void SetMCLineTableSymbol(llvm::MCSymbol *pSym, unsigned id) const;

  /// @brief Finalize the streamer, flush all written bytes.
  void Finalize() const;

  const std::string &getErrors() const { return ErrorLog; }

  void reportUsabilityIssue(llvm::StringRef Msg, const llvm::Value *Ctx = nullptr);
  void verifyRegisterLocationSize(const DbgVariable &VarVal, const DwarfDebug &DD, unsigned MaxGRFSpaceInBits,
                                  uint64_t ExpectedSize);
  void verifyRegisterLocationExpr(const DbgVariable &VarVal, const DwarfDebug &DD);

private:
  class DiagnosticBuff {
    std::string Buff;
    llvm::raw_string_ostream OS;

  public:
    DiagnosticBuff() : OS(Buff) {}
    llvm::raw_string_ostream &out() { return OS; }
  };

  void verificationReport(const DbgVariable &VarVal, DiagnosticBuff &DiagBuff);
  void verificationReport(DiagnosticBuff &DiagBuff);

  /// @brief Return information about object file lowering.
  const llvm::MCObjectFileInfo &GetObjFileLowering() const;

  /// @brief Return the target triple string.
  const std::string &GetTargetTriple() const { return m_targetTriple; }

private:
  llvm::MCStreamer *m_pMCStreamer;
  llvm::MCContext *m_pContext;
  llvm::SourceMgr *m_pSrcMgr;
  llvm::MCAsmInfo *m_pAsmInfo;
  IGCLLVM::MCObjectFileInfo *m_pObjFileInfo;
  const llvm::DataLayout *m_pDataLayout;
  const std::string &m_targetTriple;
  Settings StreamOptions;
  std::string ErrorLog;

  mutable unsigned int m_setCounter;
};

} // namespace IGC

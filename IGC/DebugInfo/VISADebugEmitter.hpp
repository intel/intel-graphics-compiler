/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "VISAIDebugEmitter.hpp"

#include <map>
#include <set>

namespace llvm {
class Module;
}

namespace IGC {

class StreamEmitter;
class VISAModule;
class DwarfDebug;
class DwarfDISubprogramCache;
class CodeGenContext;
class VISADebugInfo;

class DebugEmitter : public IDebugEmitter {
public:
  DebugEmitter();
  ~DebugEmitter();

  // IDebugEmitter interface methods
  void Initialize(std::unique_ptr<VISAModule> VM,
                  const DebugEmitterOpts &Opts) override;

  void SetDISPCache(DwarfDISubprogramCache *DISPCache) override;

  std::vector<char> Finalize(bool Finalize,
                             const IGC::VISADebugInfo &VisaDbgInfo) override;

  void BeginInstruction(llvm::Instruction *pInst) override;
  void EndInstruction(llvm::Instruction *pInst) override;
  void BeginEncodingMark() override;
  void EndEncodingMark() override;

  IGC::VISAModule *getCurrentVISA() const override { return m_pVISAModule; }
  void setCurrentVISA(IGC::VISAModule *VM) override;
  void registerVISA(IGC::VISAModule *) override;

  void resetModule(std::unique_ptr<IGC::VISAModule> VM) override;

  const std::string &getErrors() const override;

private:
  /// @brief Reset Debug Emitter instance.
  void Reset();
  void processCurrentFunction(bool Finalize,
                              const IGC::VISAObjectDebugInfo &VisaDbgInfo);

private:
  bool m_initialized = false;
  bool m_debugEnabled = false;
  bool doneOnce = false;

  llvm::SmallVector<char, 1000> m_str;
  llvm::raw_svector_ostream m_outStream;
  std::string m_errs;

  VISAModule *m_pVISAModule = nullptr;
  /// m_pDwarfDebug - dwarf debug info processor.
  std::unique_ptr<IGC::DwarfDebug> m_pDwarfDebug;
  std::unique_ptr<IGC::StreamEmitter> m_pStreamEmitter;
  std::vector<std::unique_ptr<VISAModule>> toFree;

  unsigned int lastGenOff = 0;

  void writeProgramHeaderTable(bool is64Bit, void *pBuffer, unsigned int size);
  void prepareElfForZeBinary(bool is64Bit, char *pElfBuffer,
                             size_t elfBufferSize, size_t kernelNameWithDotSize,
                             size_t *pEndOfDotTextNameInStrtab);
  void setElfType(bool is64Bit, void *pBuffer);
};

} // namespace IGC

/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "VISADebugDecoder.hpp"

#include "Probe/Assertion.h"

#include <string>
#include <vector>

namespace llvm {
class Instruction;
class Function;
} // namespace llvm

namespace IGC {

struct DebugEmitterOpts;

class CShader;
class VISAModule;
class DwarfDISubprogramCache;
class VISADebugInfo;

/// @brief IDebugEmitter is an interface for debug info emitter class.
///        It can be used by IGC VISA emitter pass to emit debug info.
class IDebugEmitter {
public:
  /// @brief Creates a new concrete instance of debug emitter.
  /// @return A new instance of debug emitter.
  static IDebugEmitter *Create();

  /// @brief Releases given instance of debug emitter.
  /// @param pDebugEmitter instance of debug emitter to release.
  static void Release(IDebugEmitter *pDebugEmitter);

  IDebugEmitter() {}

  virtual ~IDebugEmitter() {}

  /// @brief Initialize debug emitter for processing the given shader.
  /// @param MainVisa module to process, and emit debug info for.
  /// @param debugEnabled indicator for emitting debug info or not.
  virtual void Initialize(std::unique_ptr<IGC::VISAModule> VM,
                          const DebugEmitterOpts &Opts) = 0;

  /// @brief DISPCache is used to optimize discovery of DISubprogram
  //  nodes. Calling this method is optional (this is an optimization).
  /// @param DISPCache [IN] pointer to an external DwarfDISubprogramCache
  virtual void SetDISPCache(DwarfDISubprogramCache *DISPCache) = 0;
  /// @brief Emit debug info to given buffer and reset debug emitter.
  /// @param Finalize [IN] indicates whether this is last function in group.
  /// @param VisaDbgIngo [IN] holds decoded VISA debug information.
  /// @return memory buffer which contains the emitted debug info.
  virtual std::vector<char> Finalize(bool Finalize,
                                     const IGC::VISADebugInfo &VisaDbgInfo) = 0;

  /// @brief Process instruction before emitting its VISA code.
  /// @param pInst instruction to process.
  virtual void BeginInstruction(llvm::Instruction *pInst) = 0;

  /// @brief Process instruction after emitting its VISA code.
  /// @param pInst instruction to process.
  virtual void EndInstruction(llvm::Instruction *pInst) = 0;

  /// @brief Mark begin of VISA code emitting section.
  virtual void BeginEncodingMark() = 0;

  /// @brief Mark end of VISA code emitting section.
  virtual void EndEncodingMark() = 0;

  virtual void resetModule(std::unique_ptr<IGC::VISAModule> VM) = 0;

  /// @brief returns currently proccessed object
  virtual IGC::VISAModule *getCurrentVISA() const = 0;
  /// @brief sets new object to process
  virtual void setCurrentVISA(IGC::VISAModule *VF) = 0;

  /// @brief registers VISA object for bookkeeping purposes
  virtual void registerVISA(IGC::VISAModule *VF) = 0;

  virtual const std::string &getErrors() const = 0;
};

} // namespace IGC

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


//===- llvm/Bitcode/BitcodeReader.h - Bitcode reader ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header defines interfaces to read LLVM bitcode files/streams.
//
//===----------------------------------------------------------------------===//

#ifndef __DRIVERINTERFACE_UPGRADER_BITCODE_READER_H__
#define __DRIVERINTERFACE_UPGRADER_BITCODE_READER_H__

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/Bitcode/BitstreamReader.h>
#include <llvm/Bitcode/LLVMBitCodes.h>
#include "llvm/Bitcode/BitcodeReader.h"
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Comdat.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/GVMaterializer.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/OperandTraits.h>
#include <llvm/IR/TrackingMDRef.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Support/MemoryBuffer.h>
#include <deque>
#include <system_error>
#include <vector>
#include "common/LLVMWarningsPop.hpp"

namespace upgrader {

using namespace llvm;

  struct BitcodeFileContents;

  /// Basic information extracted from a bitcode module to be used for LTO.
  struct BitcodeLTOInfo {
    bool IsThinLTO;
    bool HasSummary;
    bool EnableSplitLTOUnit;
  };

  /// Represents a module in a bitcode file.
  class BitcodeModule {
    // This covers the identification (if present) and module blocks.
    ArrayRef<uint8_t> Buffer;
    StringRef ModuleIdentifier;

    // The string table used to interpret this module.
    StringRef Strtab;

    // The bitstream location of the IDENTIFICATION_BLOCK.
    uint64_t IdentificationBit;

    // The bitstream location of this module's MODULE_BLOCK.
    uint64_t ModuleBit;

    BitcodeModule(ArrayRef<uint8_t> Buffer, StringRef ModuleIdentifier,
                  uint64_t IdentificationBit, uint64_t ModuleBit)
        : Buffer(Buffer), ModuleIdentifier(ModuleIdentifier),
          IdentificationBit(IdentificationBit), ModuleBit(ModuleBit) {}

    // Calls the ctor.
    friend Expected<BitcodeFileContents>
   getBitcodeFileContents(MemoryBufferRef Buffer);

    Expected<std::unique_ptr<Module>> getModuleImpl(LLVMContext &Context,
                                                    bool MaterializeAll,
                                                    bool ShouldLazyLoadMetadata,
                                                    bool IsImporting);

  public:
    StringRef getBuffer() const {
      return StringRef((const char *)Buffer.begin(), Buffer.size());
    }

    StringRef getStrtab() const { return Strtab; }

    StringRef getModuleIdentifier() const { return ModuleIdentifier; }

    /// Read the bitcode module and prepare for lazy deserialization of function
    /// bodies. If ShouldLazyLoadMetadata is true, lazily load metadata as well.
    /// If IsImporting is true, this module is being parsed for ThinLTO
    /// importing into another module.
    Expected<std::unique_ptr<Module>> getLazyModule(LLVMContext &Context,
                                                    bool ShouldLazyLoadMetadata,
                                                    bool IsImporting);

    /// Read the entire bitcode module and return it.
    Expected<std::unique_ptr<Module>> parseModule(LLVMContext &Context);

    /// Returns information about the module to be used for LTO: whether to
    /// compile with ThinLTO, and whether it has a summary.
    Expected<BitcodeLTOInfo> getLTOInfo();

    /// Parse the specified bitcode buffer, returning the module summary index.
    Expected<std::unique_ptr<ModuleSummaryIndex>> getSummary();

    /// Parse the specified bitcode buffer and merge its module summary index
    /// into CombinedIndex.
    Error readSummary(ModuleSummaryIndex &CombinedIndex, StringRef ModulePath,
                      uint64_t ModuleId);
  };

  struct BitcodeFileContents {
    std::vector<BitcodeModule> Mods;
    StringRef Symtab, StrtabForSymtab;
  };

  /// Returns the contents of a bitcode file. This includes the raw contents of
  /// the symbol table embedded in the bitcode file. Clients which require a
  /// symbol table should prefer to use irsymtab::read instead of this function
  /// because it creates a reader for the irsymtab and handles upgrading bitcode
  /// files without a symbol table or with an old symbol table.
  Expected<BitcodeFileContents> getBitcodeFileContents(MemoryBufferRef Buffer);

  /// Returns a list of modules in the specified bitcode buffer.
  Expected<std::vector<BitcodeModule>>
  getBitcodeModuleList(MemoryBufferRef Buffer);

  /// Read the header of the specified bitcode buffer and prepare for lazy
  /// deserialization of function bodies. If ShouldLazyLoadMetadata is true,
  /// lazily load metadata as well. If IsImporting is true, this module is
  /// being parsed for ThinLTO importing into another module.
  Expected<std::unique_ptr<Module>>
  getLazyBitcodeModule(MemoryBufferRef Buffer, LLVMContext &Context,
                       bool ShouldLazyLoadMetadata = false,
                       bool IsImporting = false);

  /// Read the specified bitcode file, returning the module.
  Expected<std::unique_ptr<Module>> parseBitcodeFile(MemoryBufferRef Buffer,
                                                     LLVMContext &Context);

  /// Returns LTO information for the specified bitcode file.
  Expected<BitcodeLTOInfo> getBitcodeLTOInfo(MemoryBufferRef Buffer);

  /// Parse the specified bitcode buffer, returning the module summary index.
  Expected<std::unique_ptr<ModuleSummaryIndex>>
  getModuleSummaryIndex(MemoryBufferRef Buffer);

  /// Parse the specified bitcode buffer and merge the index into CombinedIndex.
  Error readModuleSummaryIndex(MemoryBufferRef Buffer,
                               ModuleSummaryIndex &CombinedIndex,
                               uint64_t ModuleId);

  /// Parse the module summary index out of an IR file and return the module
  /// summary index object if found, or an empty summary if not. If Path refers
  /// to an empty file and IgnoreEmptyThinLTOIndexFile is true, then
  /// this function will return nullptr.
  Expected<std::unique_ptr<ModuleSummaryIndex>>
  getModuleSummaryIndexForFile(StringRef Path,
                               bool IgnoreEmptyThinLTOIndexFile = false);
} // end namespace llvm

#endif

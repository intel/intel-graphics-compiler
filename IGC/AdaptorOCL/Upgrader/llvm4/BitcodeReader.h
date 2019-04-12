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

//===- BitcodeReader.h - Internal BitcodeReader impl ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header defines the BitcodeReader class.
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

    /// Represents a module in a bitcode file.
    class BitcodeModule {
      // This covers the identification (if present) and module blocks.
      ArrayRef<uint8_t> Buffer;
      StringRef ModuleIdentifier;

      // The bitstream location of the IDENTIFICATION_BLOCK.
      uint64_t IdentificationBit;

      // The bitstream location of this module's MODULE_BLOCK.
      uint64_t ModuleBit;

      BitcodeModule(ArrayRef<uint8_t> Buffer, StringRef ModuleIdentifier,
                    uint64_t IdentificationBit, uint64_t ModuleBit)
          : Buffer(Buffer), ModuleIdentifier(ModuleIdentifier),
            IdentificationBit(IdentificationBit), ModuleBit(ModuleBit) {}

      // Calls the ctor.
      friend  Expected<std::vector<BitcodeModule>>
        getBitcodeModuleList(MemoryBufferRef Buffer);

      Expected<std::unique_ptr<Module>> getModuleImpl(LLVMContext &Context,
                                                      bool MaterializeAll,
                                                      bool ShouldLazyLoadMetadata,
                                                      bool IsImporting);

    public:
      StringRef getBuffer() const {
        return StringRef((const char *)Buffer.begin(), Buffer.size());
      }

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

      /// Check if the given bitcode buffer contains a summary block.
      Expected<bool> hasSummary();

      /// Parse the specified bitcode buffer, returning the module summary index.
      Expected<std::unique_ptr<ModuleSummaryIndex>> getSummary();
    };

    /// Returns a list of modules in the specified bitcode buffer.
    //Expected<std::vector<BitcodeModule>>
    //getBitcodeModuleList(MemoryBufferRef Buffer);

    /// Read the header of the specified bitcode buffer and prepare for lazy
    /// deserialization of function bodies. If ShouldLazyLoadMetadata is true,
    /// lazily load metadata as well. If IsImporting is true, this module is
    /// being parsed for ThinLTO importing into another module.
    Expected<std::unique_ptr<Module>>
    getLazyBitcodeModule(MemoryBufferRef Buffer, LLVMContext &Context,
                         bool ShouldLazyLoadMetadata = false,
                         bool IsImporting = false);
    Expected<std::unique_ptr<Module>> parseBitcodeFile(MemoryBufferRef Buffer,
                                                       LLVMContext &Context);

#if 0
/// Read the header of the specified bitcode buffer and prepare for lazy
/// deserialization of function bodies.  If successful, this moves Buffer. On
/// error, this *does not* move Buffer.
ErrorOr<Module *>
getLazyBitcodeModule(std::unique_ptr<MemoryBuffer> &&Buffer,
           LLVMContext &Context,
           DiagnosticHandlerFunction DiagnosticHandler = nullptr);

/// Read the header of the specified stream and prepare for lazy
/// deserialization and streaming of function bodies.
ErrorOr<std::unique_ptr<Module>> getStreamedBitcodeModule(
StringRef Name, DataStreamer *Streamer, LLVMContext &Context,
DiagnosticHandlerFunction DiagnosticHandler = nullptr);

/// Read the header of the specified bitcode buffer and extract just the
/// triple information. If successful, this returns a string. On error, this
/// returns "".
std::string
getBitcodeTargetTriple(MemoryBufferRef Buffer, LLVMContext &Context,
         DiagnosticHandlerFunction DiagnosticHandler = nullptr);

/// Read the specified bitcode file, returning the module.
ErrorOr<Module *>
parseBitcodeFile(MemoryBufferRef Buffer, LLVMContext &Context,
       DiagnosticHandlerFunction DiagnosticHandler = nullptr);
#endif
} // End upgrader namespace

#endif

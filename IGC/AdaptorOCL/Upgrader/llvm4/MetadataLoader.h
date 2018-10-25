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

//===-- Bitcode/Reader/MetadataLoader.h - Load Metadatas -------*- C++ -*-====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class handles loading Metadatas.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_BITCODE_READER_METADATALOADER_H
#define LLVM_LIB_BITCODE_READER_METADATALOADER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <functional>
#include <memory>

namespace llvm {
class BitcodeReaderValueList;
class BitstreamCursor;
class DISubprogram;
class Error;
class Function;
class Instruction;
class Metadata;
class MDNode;
class Module;
class Type;

/// Helper class that handles loading Metadatas and keeping them available.
class MetadataLoader {
  class MetadataLoaderImpl;
  std::unique_ptr<MetadataLoaderImpl> Pimpl;
  Error parseMetadata(bool ModuleLevel);

public:
  ~MetadataLoader();
  MetadataLoader(BitstreamCursor &Stream, Module &TheModule,
                 BitcodeReaderValueList &ValueList, bool IsImporting,
                 std::function<Type *(unsigned)> getTypeByID);
  MetadataLoader &operator=(MetadataLoader &&);
  MetadataLoader(MetadataLoader &&);

  // Parse a module metadata block
  Error parseModuleMetadata() { return parseMetadata(true); }

  // Parse a function metadata block
  Error parseFunctionMetadata() { return parseMetadata(false); }

  /// Set the mode to strip TBAA metadata on load.
  void setStripTBAA(bool StripTBAA = true);

  /// Return true if the Loader is stripping TBAA metadata.
  bool isStrippingTBAA();

  // Return true there are remaining unresolved forward references.
  bool hasFwdRefs() const;

  /// Return the given metadata, creating a replaceable forward reference if
  /// necessary.
  Metadata *getMetadataFwdRefOrLoad(unsigned Idx);

  MDNode *getMDNodeFwdRefOrNull(unsigned Idx);

  /// Return the DISubprogra metadata for a Function if any, null otherwise.
  DISubprogram *lookupSubprogramForFunction(Function *F);

  /// Parse a `METADATA_ATTACHMENT` block for a function.
  Error parseMetadataAttachment(
      Function &F, const SmallVectorImpl<Instruction *> &InstructionList);

  /// Parse a `METADATA_KIND` block for the current module.
  Error parseMetadataKinds();

  unsigned size() const;
  void shrinkTo(unsigned N);
};
}

#endif // LLVM_LIB_BITCODE_READER_METADATALOADER_H

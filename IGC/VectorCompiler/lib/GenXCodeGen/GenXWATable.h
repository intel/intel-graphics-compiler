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

#ifndef VCOPT_LIB_GENXCODEGEN_GENXWATABLE_H
#define VCOPT_LIB_GENXCODEGEN_GENXWATABLE_H

#include <llvm/Pass.h>

#include <inc/common/sku_wa.h>

namespace llvm {

void initializeGenXWATablePass(PassRegistry &PR);

// Transparent wrapper around driver WA_TABLE.
class GenXWATable : public ImmutablePass {
  WA_TABLE *WaTable = nullptr;

public:
  static char ID;

  GenXWATable() : ImmutablePass(ID) {}

  GenXWATable(WA_TABLE *Table) : ImmutablePass(ID), WaTable{Table} {
    initializeGenXWATablePass(*PassRegistry::getPassRegistry());
  }

  // This can return nullptr which means that we don't know
  // workarounds for current platform.
  WA_TABLE *getWATable() const { return WaTable; }
};
} // namespace llvm

#endif

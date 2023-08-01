/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef RELOCATION_ENTRY_HPP
#define RELOCATION_ENTRY_HPP

#include "G4_IR.hpp"
#include "include/RelocationInfo.h"

namespace vISA {
class RelocationEntry {
  G4_INST *inst; // instruction to be relocated
  int opndPos;   // operand to be relocated. This should be a RelocImm
  using RelocationType = GenRelocType;
  RelocationType relocType;
  std::string symName; // the symbol name that it's address to be resolved

  RelocationEntry(G4_INST *i, int pos, RelocationType type,
                  const std::string &symbolName)
      : inst(i), opndPos(pos), relocType(type), symName(symbolName) {}

public:
  static RelocationEntry &createRelocation(G4_Kernel &kernel, G4_INST &inst,
                                           int opndPos,
                                           const std::string &symbolName,
                                           RelocationType type);

  G4_INST *getInst() const { return inst; }

  RelocationType getType() const { return relocType; }
  const char *getTypeString() const {return getTypeString(getType());}
  static const char *getTypeString(RelocationType rt);

  uint32_t getOpndPos() const { return opndPos; }

  const std::string &getSymbolName() const { return symName; }

  void doRelocation(const G4_Kernel &k, void *binary, uint32_t binarySize);

  uint32_t getTargetOffset(const IR_Builder &builder) const;

  void dump(std::ostream &os = std::cerr) const;
}; // RelocationEntry

} // namespace vISA

#endif // RELOCATION_ENTRY_HPP

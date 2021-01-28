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
#ifndef RELOCATION_ENTRY_HPP
#define RELOCATION_ENTRY_HPP

#include "Gen4_IR.hpp"
#include "include/RelocationInfo.h"

namespace vISA
{
class RelocationEntry
{
    G4_INST* inst;             // instruction to be relocated
    int opndPos;               // operand to be relocated. This should be a RelocImm
    using RelocationType = GenRelocType;
    RelocationType relocType;
    std::string symName;       // the symbol name that it's address to be resolved

    RelocationEntry(
        G4_INST* i, int pos, RelocationType type, const std::string& symbolName)
        : inst(i), opndPos(pos), relocType(type), symName(symbolName) { }

public:
    static RelocationEntry& createRelocation(G4_Kernel& kernel, G4_INST& inst,
        int opndPos, const std::string& symbolName, RelocationType type);

    G4_INST* getInst() const {return inst;}

    RelocationType getType() const {return relocType;}
    const char* getTypeString() const;

    uint32_t getOpndPos() const {return opndPos;}

    const std::string& getSymbolName() const {return symName;}

    void doRelocation(const G4_Kernel& k, void* binary, uint32_t binarySize);

    uint32_t getTargetOffset(const IR_Builder& builder) const;

    void dump(std::ostream &os = std::cerr) const;
}; // RelocationEntry

} // vISA::

#endif // RELOCATION_ENTRY_HPP

/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// file SPIRVMemAliasingINTEL.h from the LLVM/SPIR-V Translator
// This file defines the memory aliasing entries defined in SPIRV spec with op
// codes.

#ifndef SPIRV_LIBSPIRV_SPIRVMEMALIASINGINTEL_H
#define SPIRV_LIBSPIRV_SPIRVMEMALIASINGINTEL_H

#include "SPIRVEntry.h"

namespace igc_spv {

    template <Op TheOpCode, SPIRVWord TheFixedWordCount>
    class SPIRVMemAliasingINTELGeneric : public SPIRVEntry {
    public:
        SPIRVMemAliasingINTELGeneric(SPIRVModule* TheModule, SPIRVId TheId,
            const std::vector<SPIRVId>& TheArgs)
            : SPIRVEntry(TheModule, TheArgs.size() + TheFixedWordCount, TheOpCode,
                TheId), Args(TheArgs) {
            SPIRVMemAliasingINTELGeneric::validate();
            IGC_ASSERT_MESSAGE(TheModule, "Invalid module");
        }

        SPIRVMemAliasingINTELGeneric() : SPIRVEntry(TheOpCode) {}

        const std::vector<SPIRVId>& getArguments() const { return Args; }

        void setWordCount(SPIRVWord TheWordCount) override {
            SPIRVEntry::setWordCount(TheWordCount);
            Args.resize(TheWordCount - TheFixedWordCount);
        }

        void validate() const override { SPIRVEntry::validate(); }

        CapVec getRequiredCapability() const override {
            return getVec(CapabilityMemoryAccessAliasingINTEL);
        }

    protected:
        static const SPIRVWord FixedWC = TheFixedWordCount;
        static const Op OC = TheOpCode;
        std::vector<SPIRVId> Args;
        _SPIRV_DEF_DEC2(Id, Args)
    };
#define _SPIRV_OP_(x, ...)                                                      \
  typedef SPIRVMemAliasingINTELGeneric<Op##x, __VA_ARGS__> SPIRV##x;
    // Intel Memory Alasing Instructions
    _SPIRV_OP_(AliasDomainDeclINTEL, 2)
        _SPIRV_OP_(AliasScopeDeclINTEL, 2)
        _SPIRV_OP_(AliasScopeListDeclINTEL, 2)
#undef _SPIRV_OP_
} // SPIRV
#endif // SPIRV_LIBSPIRV_SPIRVMEMALIASINGINTEL_H

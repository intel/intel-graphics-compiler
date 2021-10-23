/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BACKEND_GED_GEDBITPROCESSOR_HPP_
#define _IGA_BACKEND_GED_GEDBITPROCESSOR_HPP_

#include "../BitProcessor.hpp"
#include "../../IR/Instruction.hpp"
#include "../../IR/Kernel.hpp"
#include "../../Models/Models.hpp"

namespace iga
{
    // Stuff that's common to both the encoder and decoder for GED
    //
    // Methods in this class have acecss to:
    //   - model information (m_model)
    //   - the parent bit processor (thus we can raise errors and warnings)
    //
    class GEDBitProcessor : public BitProcessor
    {
    protected:
        const Model &m_model;

        Platform platform() const {return m_model.platform;}

    public:
        GEDBitProcessor(const Model& model,ErrorHandler& errHandler);

        // GEN8-9 CSR (context save and restore) usually via mov of
        // "acc3-9" (mme1-7) need Align16 and
        // specific muxes to distinguish the acc.
        // NOTE: acc2 is mme0, but it encodes Align1 and doesn't need the
        // workaround code for CSR
        //
        // These both check the platform too, so no need to also call
        //   isAlign16MathMacroRegisterCsrPlatform
        bool isAlign16MathMacroRegisterCsrOperand(
            Operand::Kind opKind,
            RegName reg,
            uint16_t regNum) const;
        bool isAlign16MathMacroRegisterCsrOperand(const Operand &op) const {
            return isAlign16MathMacroRegisterCsrOperand(
                op.getKind(),
                op.getDirRegName(),
                op.getDirRegRef().regNum);
        }
        bool isAlign16MathMacroRegisterCsrPlatform() const;

        // TODO: some functionality in Model that is shared by
        // ged::Decoder and ged::Encoder might make more sense in this class
        // now that it fits as a common place for shared functionality.
        //
        // A good litmus test is whether that functionality is used by some
        // other third party (keep it in Model in that case).
    }; // GEDBitProcessor
} // iga::

#endif

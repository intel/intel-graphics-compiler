/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// IGA headers
#include "../Backend/GED/Encoder.hpp"
#include "igaEncoderWrapper.hpp"

using namespace iga;

iga_status_t KernelEncoder::encode()
{

    ErrorHandler errHandler;
    EncoderOpts enc_opt(m_autoCompact, true);
    enc_opt.autoDepSet = m_enableAutoDeps;
    enc_opt.swsbEncodeMode = m_swsbEncodeMode;

    Encoder enc(m_kernel->getModel(), errHandler, enc_opt);
    enc.encodeKernel(
        *m_kernel,
        m_kernel->getMemManager(),
        m_buf,
        m_binarySize);
#ifdef _DEBUG
    if (errHandler.hasErrors()) {
        // failed encode
        for (auto &e : errHandler.getErrors()) {
            std::cerr <<
                "line " << e.at.line <<
                ", col " << e.at.col << ": " <<
                e.message << "\n";
        }
        return IGA_ERROR;
    }
    if (errHandler.hasWarnings()) {
        for (auto &w : errHandler.getWarnings()) {
            std::cerr <<
                "line " << w.at.line <<
                ", col " << w.at.col << ": " <<
                w.message << "\n";
        }
        // fallthrough to IGA_SUCCESS
    }
#endif // _DEBUG
    return IGA_SUCCESS;
}

bool KernelEncoder::patchImmValue(const Model& model, unsigned char* binary, Type type, const ImmVal &val) {
    // check if the first instruction is compacted and get the instruction length
    // FIXME: compact bit extract code copy from Decoder::getBitField(COMPACTION_CONTROL, 1)
    const uint32_t* inst_start = (const uint32_t*)binary;
    uint32_t mask = (1 << (uint32_t)1) - 1;
    int32_t inst_len = ((inst_start[COMPACTION_CONTROL / 32] >> (COMPACTION_CONTROL % 32)) & mask) != 0 ?
        COMPACTED_SIZE : UNCOMPACTED_SIZE;

    // decode the instruction to ged_inst_t
    ged_ins_t ged_inst;
    memset(&ged_inst, 0, sizeof(ged_inst));
    const GED_MODEL& ged_model = lowerPlatform(model.platform);
    GED_RETURN_VALUE status = GED_DecodeIns(ged_model, binary, (uint32_t)inst_len, &ged_inst);
    assert(status == GED_RETURN_VALUE_SUCCESS);
    if (status != GED_RETURN_VALUE_SUCCESS)
        return false;

    // set the imm value
    status = GED_SetImm(&ged_inst, Encoder::typeConvesionHelper(val, type));
    assert(status == GED_RETURN_VALUE_SUCCESS);
    if (status != GED_RETURN_VALUE_SUCCESS)
        return false;

    // encode the instruction and write back to binary,
    // if the instruction is compacted, then it must still be compacted now
    if (inst_len == COMPACTED_SIZE)
        status = GED_EncodeIns(&ged_inst, GED_INS_TYPE_COMPACT, binary);
    else
        status = GED_EncodeIns(&ged_inst, GED_INS_TYPE_NATIVE, binary);
    assert(status == GED_RETURN_VALUE_SUCCESS);
    if (status != GED_RETURN_VALUE_SUCCESS)
        return false;

    return true;
}
/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "iga_main.hpp"


bool disassemble(
    const Opts &opts, igax::Context &ctx, const std::string &inpFile)
{
    std::vector<unsigned char> inp;
    if (inpFile == IGA_STDIN_FILENAME) {
        inp = readBinaryStreamStdin();
    } else {
        readBinaryFile(inpFile.c_str(), inp);
    }

    iga_disassemble_options_t dopts = IGA_DISASSEMBLE_OPTIONS_INIT();
    dopts.formatting_opts = makeFormattingOpts(opts);
    dopts.base_pc_offset             = opts.pcOffset;
    setOptBit(dopts.decoder_opts,
        IGA_DECODING_OPT_NATIVE,
        opts.useNativeEncoder);
    try {
        auto r = ctx.disassembleToString(inp.data(), inp.size(), dopts);
        for (auto &w : r.warnings) {
            emitWarningToStderr(w, inp);
        }
        writeText(opts, r.value);
        return true;
    } catch (const igax::DisassembleError &err) {
        // some error where we can report several potentially
        for (auto &e : err.errors) {
            emitErrorToStderr(e, inp);
        }
        if (err.errors.empty()) {
            // e.g. some failures don't have diagnostics
            //      invalid project for instance
            err.emit(std::cerr);
        }
        if (opts.outputOnFail)
            writeText(opts, err.outputText);
    } catch (const igax::Error &err) {
        // some other error
        err.emit(std::cerr);
    }
    return false;
}

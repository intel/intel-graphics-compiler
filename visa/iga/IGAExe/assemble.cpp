/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "iga_main.hpp"


bool assemble(
    const Opts &opts,
    igax::Context &ctx,
    const std::string &inpFile)
{
    std::string inpText;
    if (inpFile == IGA_STDIN_FILENAME) {
        igax::Bits stdinBits = readBinaryStreamStdin();
        stdinBits.push_back(0); // NUL
        inpText = (const char *)stdinBits.data();
    } else {
        inpText = readTextFile(inpFile.c_str());
    }

    igax::Bits bits;
    bool success = assemble(opts, ctx, inpFile, inpText, bits);
    if (success) {
        writeBinary(opts, bits.data(), bits.size());
    }
    return success;
}

bool assemble(
    const Opts &opts,
    igax::Context &ctx,
    const std::string &,
    const std::string &inpText,
    igax::Bits &bits)
{
    iga_assemble_options_t aopts = IGA_ASSEMBLE_OPTIONS_INIT();
    aopts.enabled_warnings = opts.enabledWarnings;

    setOptBit(aopts.encoder_opts,
        IGA_ENCODER_OPT_AUTO_COMPACT,
        opts.autoCompact);
    setOptBit(aopts.encoder_opts,
        IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL,
        opts.errorOnCompactFail);
    setOptBit(aopts.encoder_opts,
        IGA_ENCODER_OPT_AUTO_DEPENDENCIES,
        opts.autosetDepInfo);
    setOptBit(aopts.encoder_opts,
        IGA_ENCODER_OPT_USE_NATIVE,
        opts.useNativeEncoder);
    setOptBit(aopts.encoder_opts,
        IGA_ENCODER_OPT_FORCE_NO_COMPACT,
        opts.forceNoCompact);
    setOptBit(aopts.syntax_opts,
        IGA_SYNTAX_OPT_LEGACY_SYNTAX,
        opts.legacyDirectives);
    setOptBit(aopts.syntax_opts,
        IGA_SYNTAX_OPT_EXTENSIONS,
        opts.syntaxExts);
    aopts.sbid_count = opts.sbidCount;

    try {
        auto r = ctx.assembleFromString(inpText, aopts);
        for (auto &w : r.warnings) {
            emitWarningToStderr(w, inpText);
        }
        bits = r.value;
        return true;
    } catch (const igax::AssembleError &err) {
        for (auto &e : err.errors) {
            emitErrorToStderr(e, inpText);
        }
        if (err.errors.empty()) {
            // e.g. some failures don't have diagnostics
            //      invalid project for instance
            err.emit(std::cerr);
        }
        bits.clear();
    } catch (const igax::Error &err) {
        // some other error
        err.emit(std::cerr);
        bits.clear();
    }
    return false;
}

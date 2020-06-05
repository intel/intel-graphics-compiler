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

#include "iga_main.hpp"

bool assemble(
    const Opts &opts,
    igax::Context &ctx,
    const std::string &inpFile)
{
    std::string inpText = readTextFile(inpFile.c_str());
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
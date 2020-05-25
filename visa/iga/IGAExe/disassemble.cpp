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

bool disassemble(
    const Opts &opts, igax::Context &ctx, const std::string &inpFile)
{
    std::vector<unsigned char> inp;
    readBinaryFile(inpFile.c_str(), inp);

    iga_disassemble_options_t dopts = IGA_DISASSEMBLE_OPTIONS_INIT();
    dopts.formatting_opts = makeFormattingOpts(opts);
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
    } catch (const igax::Error &err) {
        // some other error
        err.emit(std::cerr);
    }
    return false;
}

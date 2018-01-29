#include "iga_main.hpp"

bool disassemble(const Opts &opts, igax::Context &ctx, std::string &inpFile)
{
    std::vector<unsigned char> inp;
    readBinaryFile(inpFile.c_str(), inp);

    iga_disassemble_options_t dopts = IGA_DISASSEMBLE_OPTIONS_INIT();
    setOptBit(dopts.formatting_opts,
        IGA_FORMATTING_OPT_NUMERIC_LABELS,
        opts.numericLabels);
    setOptBit(dopts.formatting_opts,
        IGA_FORMATTING_OPT_SYNTAX_EXTS,
        opts.syntaxExts);
    setOptBit(dopts.formatting_opts,
        IGA_FORMATTING_OPT_PRINT_HEX_FLOATS,
        opts.printHexFloats);
    setOptBit(dopts.formatting_opts,
        IGA_FORMATTING_OPT_PRINT_PC,
        opts.printInstructionPc);
    setOptBit(dopts.formatting_opts,
        IGA_FORMATTING_OPT_PRINT_BITS,
        opts.printBits);
    setOptBit(dopts.formatting_opts,
        IGA_FORMATTING_OPT_PRINT_DEPS,
        opts.printDeps);
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
        return false;
    } catch (const igax::Error &err) {
        // some other error
        err.emit(std::cerr);
        return false;
    }
}

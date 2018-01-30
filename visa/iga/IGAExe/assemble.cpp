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
    const std::string &inpFile,
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
        bits.clear();
    } catch (const igax::Error &err) {
        // some other error
        err.emit(std::cerr);
        bits.clear();
    }
    return false;
}
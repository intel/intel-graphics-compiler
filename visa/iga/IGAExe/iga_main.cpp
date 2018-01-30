#include "iga_main.hpp"
#include "opts.hpp"


extern "C" int iga_main(int argc, const char **argv)
{
    struct Opts baseOpts;

    std::string title = "Intel Graphics Assembler ";
    title += iga_version_string();

    opts::CmdlineSpec<Opts> cmdline(
        title.c_str(),
        opts::exeName(argv[0]),
        "assembles 'file.asm' to file.krn (SKL and HSW respectively):\n"
        "  % " IGA_EXE "  file.asm  -p=9   -a   > file.krn\n"
        "  % " IGA_EXE "  file.asm  -p=7p5 -a  -o file.krn\n"
        "disassembles 'file.bin' to 'file.asm' (SKL)\n"
        "  % " IGA_EXE "  file.bin  -p=9  -d   > file.asm\n"
        "  % " IGA_EXE "  file.bin  -p=9  -d  -o file.asm\n"
        "same as above, but appends PC to each instruction\n"
        "  % " IGA_EXE "  file.bin  -p=9  -Xprint-pc  -d   > file.asm\n"
        "  % " IGA_EXE "  file.bin  -p=9  -Xprint-pc  -d  -o file.asm\n"
        "disassembles for SKL (-d and -p=9 inferred from extension) to stdout\n"
        "  % " IGA_EXE "  file.krn9\n"
        "assembles for SKL (-a and -p=9 inferred from extension) to "
        "file.krn\n"
        "  % " IGA_EXE "  file.asm9 >file.krn\n");
    cmdline.defineFlag(
        "d",
        "disassemble",
        "disassembles the input file",
        "This makes iga disassemble the file (ignoring extension).  "
        "Without this iga attempts to infer the mode based on the extension.  "
        "Files ending in '.krn' or '.dat' are treated as binary.",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::DIS;
        });
    cmdline.defineFlag(
        "a",
        "assemble",
        "assembles the input file",
        "This makes iga assemble the file (ignoring extension).  "
        "Without this iga attempts to infer the mode based on the extension.  "
        "Files ending in '.asm' are treated as binary.",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::ASM;
        });
    cmdline.defineFlag(
        "n",
        "numeric-labels",
        "use numeric labels",
        "labels will be in bytes relative to the IP pre-increment "
        "(even for jmpi and on HSW)",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.numericLabels = true;
        });
    ///////////////////////////////////////////// about the 80 col limit
    cmdline.defineOpt(
        "p",
        "platform",
        "DEVICE",
        "specifies the platform (e.g. \"GEN9\")",
        "Examples of valid platforms are: GEN7.5, GEN8, GEN8LP, GEN9, ...\n"
        "We also accept code names: HSW, BDW, CHV, ...",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler &err, Opts &baseOpts) {
            std::string inp = cinp;
            std::transform(inp.begin(), inp.end(), inp.begin(), ::toupper);

            iga_gen_t p = IGA_GEN_INVALID;
            if (inp == "IVB" || inp == "GEN7" || inp == "7") {
                p = IGA_GEN7;
            } else if (
                inp == "HSW" || inp == "GEN7.5" || inp == "GEN7_5" ||
                inp == "GEN7P5" || inp == "7.5" || inp == "7P5") {
                p = IGA_GEN7p5;
            } else if (inp == "BDW" || inp == "GEN8" || inp == "8") {
                p = IGA_GEN8;
            } else if (
                inp == "CHV" || inp == "GEN8LP" || inp == "8LP" ||
                inp == "GEN8.1" || inp == "GEN8P1" || inp == "8.1") {
                p = IGA_GEN8lp;
            } else if (inp == "SKL" || inp == "GEN9" || inp == "9") {
                p = IGA_GEN9;
            } else if (
                inp == "BXT" || inp == "GEN9LP" || inp == "9LP" ||
                inp == "GEN9.1" || inp == "GEN9P1" || inp == "9.1") {
                p = IGA_GEN9lp;
            } else if (
                inp == "KBL" || inp == "GEN9.5" || inp == "GEN9_5" ||
                inp == "GEN9P5" || inp == "9.5" || inp == "9P5") {
                p = IGA_GEN9p5;
            }  else if (inp == "CNL" || inp == "GEN10" || inp == "10") {
                p = IGA_GEN10;
            } else {
                err("invalid platform option");
            }
            baseOpts.platform = p;
        });

    cmdline.defineOpt(
        "o",
        "output",
        "FILE",
        "specifies the output file",
        "This option sets the output file.  If unset iga defaults to stdout.",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *inp, const opts::ErrorHandler &err, Opts &opts) {
            opts.outputFile = inp;
        });

    // TODO: maybe treat this as a fused argument -W....
    // then we allow stuff like -Wregions,types,no-scheduling
    opts::Group<Opts> &wGrp = cmdline.defineGroup("W", "Warnings");
    wGrp.defineFlag(
        "none",
        nullptr,
        "disables all warnings",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings = IGA_WARNINGS_NONE;
        });
    wGrp.defineFlag(
        "all",
        nullptr,
        "enables all warnings",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings = IGA_WARNINGS_ALL;
        });
    wGrp.defineFlag(
        "default",
        nullptr,
        "uses default warnings",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings = IGA_WARNINGS_DEFAULT;
        });
    wGrp.defineFlag(
        "normal-form",
        nullptr,
        "checks some don't-care fields for being in normal forms",
        "examples of this are types and regions on send operands",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings |= IGA_WARNINGS_NORMFORM;
        });
    wGrp.defineFlag(
        "regions",
        nullptr,
        "enables warnings on invalid regions",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings |= IGA_WARNINGS_REGIONS;
        });
    wGrp.defineFlag(
        "types",
        nullptr,
        "enables warnings on invalid operand type combinations",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings |= IGA_WARNINGS_TYPES;
        });
    wGrp.defineFlag(
        "scheduling",
        nullptr,
        "enables warnings related to scheduling (e.g. use of Switch)",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings |= IGA_WARNINGS_SCHED;
        });


    ///////////////////////////////////////////////////////////////////////////
    // -X...
    // EXPERIMENTAL OPTIONS
    opts::Group<Opts> &xGrp = cmdline.defineGroup("X", "Experimental Options");
    xGrp.defineFlag(
        "no-ir-checking",
        nullptr,
        "disables the IR checking on assembly",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.enabledWarnings = IGA_WARNINGS_NONE;
            fprintf(stderr, "%s\n",
                "-Xdisable-ir-checking is deprecated; use -W* options");
        });
    xGrp.defineFlag(
        "autocompact",
        nullptr,
        "auto compacts unmarked instructions",
        "This automatically attempts to compact instructions lacking a "
        "'Compacted' or 'NoCompact' instruction option.  If the instruction"
        "has a Compacted or NoCompact annotation, IGA respects that unless"
        "the compacted form does not exist.",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.autoCompact = true;
        });
    xGrp.defineFlag(
        "no-autocompact",
        nullptr,
        "the inverse (default) of -Xautocompact",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.autoCompact = false;
        });
    xGrp.defineFlag(
        "list-ops",
        nullptr,
        "displays all ops for the given platform",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::XLST;
        });
    xGrp.defineFlag(
        "legacy-directives",
        nullptr,
        "enable some IsaAsm era directives",
        "enables .default_execution_width and .default_type directives",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.legacyDirectives = true;
        });
    xGrp.defineFlag(
        "warn-on-compact-fail",
        nullptr,
        "makes compaction failure a warning instead of an error",
        "By default we fail if we are unable to compact an instruction with "
        "the {Compacted} option set; this allows one to make it a warning",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.errorOnCompactFail = false;
        });
    xGrp.defineFlag(
        "auto-deps",
        nullptr,
        "IGA automatically sets instruction dependency information",
        "",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.autosetDepInfo = true;
        });
    xGrp.defineFlag(
        "native-enc",
        nullptr,
        "Use IGA's native encoder",
        "",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.useNativeEncoder = true;
        });
    xGrp.defineFlag(
        "ifs",
        nullptr,
        "dump instruction fields",
        "This mode decodes instructions.\n"
        "EXAMPLES:\n"
        "  % iga -Xifs foo.krn9\n"
        "    decodes kernel in foo.krn9 (-p=9 inferred)\n"
        "  % iga -Xifs \"64 00 43 00  a0 0a 05 00   24 01 00 80  00 00 00 00\"\n"
        "    decodes the hex bits above\n"
        "  % iga -p=9 -Xifs foo.krn9 \"64 00 43 00  a0 0a 05 00   24 01 00 80  00 00 00 00\"\n"
        "    decodes and compares the fields\n",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::XIFS;
        });
    xGrp.defineFlag(
        "dcmp",
        nullptr,
        "debug compaction",
        "This mode debugs an instruction's compaction.  The input format is the same as -Xifs\n"
        "See that option for more information\n",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::XDCMP;
        });
    xGrp.defineFlag(
        "syntax-exts",
        nullptr,
        "enables certain syntax extensions",
        "",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.syntaxExts = true;
        });
    xGrp.defineFlag(
        "print-hex-floats",
        nullptr,
        "format floats in hexadecimal",
        "The intent is that IGA should always format floats in a way such that it "
          "will parse back them bit-precise; however, if we muck that up or if you just"
          "like the consistency, this option enables that behavior.",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler err, Opts &baseOpts) {
            baseOpts.printHexFloats = true;
        });
    xGrp.defineFlag(
        "print-pc",
        nullptr,
        "print PC with each instruction",
        "An instruction PC will be added as a comment",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.printInstructionPc = true;
        });
    xGrp.defineFlag(
        "print-bits",
        nullptr,
        "prints bits decoded with each instruction",
        "The instruction bits are emitted with each instruction",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.printBits = true;
        });
    xGrp.defineFlag(
        "print-deps",
        nullptr,
        "prints dependency info with each instruction",
        "IGA does not promise this to be exact at the moment. "
          "E.g. our analysis may be a local one only.",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.printDeps = true;
        });

    // ARGS
    cmdline.defineArg(
        "FILE",
        "the input files",
        "The input files to assemble.  This defaults to stdin.",
        opts::OptAttrs::ALLOW_UNSET | opts::OptAttrs::ALLOW_MULTI,
        [] (const char *inp, const opts::ErrorHandler &err, Opts &opts) {
            opts.inputFiles.push_back(inp);
        });

    cmdline.parse(argc, argv, baseOpts);

    // override various options not set
    auto optsForFile = [&] (const std::string &inpFile) {
        // get the file extension (e.g. foo.krn9)
        Opts os         = baseOpts;
        inferPlatformAndMode(inpFile, os);
        if (os.mode == Opts::Mode::AUTO) {
            fatalExitWithMessage(
                "%s: cannot infer mode based on file extension"
                " (use -d or -a to set mode)",
                inpFile.c_str());
        }
        if (os.platform == IGA_GEN_INVALID) {
            fatalExitWithMessage(
                "%s: cannot infer project based on file extension"
                " (use -p=...)",
                inpFile.c_str());
        }
        return os;
    };

    // one of the files has an error
    bool hasError = false;

    if (baseOpts.mode == Opts::XLST) {
        if (baseOpts.platform == IGA_GEN_INVALID) {
            fatalExitWithMessage("op listing requires platform (-p)");
        }
        // list all operations if they used -Xlist-ops
        if (baseOpts.inputFiles.empty()) {
            hasError |= listOps(baseOpts, "");
        } else {
            for (auto &inpFile : baseOpts.inputFiles) {
                // try and list a menonic matching this op
                hasError |= listOps(baseOpts, inpFile);
            }
        }
    } else if (baseOpts.mode == Opts::XIFS) {
        hasError |= decodeInstructionFields(baseOpts);
    } else if (baseOpts.mode == Opts::XDCMP) {
        hasError |= debugCompaction(baseOpts);
    } else {
        if (baseOpts.inputFiles.empty()) {
            fatalExitWithMessage("at least one file required");
        }

        // iterate each file and process it
        for (auto &inpFile : baseOpts.inputFiles) {
            if (!doesFileExist(inpFile.c_str())) {
                fatalExitWithMessage("%s: file not found", inpFile.c_str());
            }

            struct Opts opts = optsForFile(inpFile);
            try {
                igax::Context ctx(opts.platform);
                if (opts.mode == Opts::DIS) {
                    hasError |= !disassemble(opts, ctx, inpFile);
                } else if (opts.mode == Opts::ASM) {
                    hasError |= !assemble(opts, ctx, inpFile);
                } else {
                    fatalExitWithMessage(
                        "%s: mode (-a or -d) must be specified for this file",
                        inpFile.c_str());
                }
            } catch (const igax::Error &err) {
                err.emit(std::cerr);
            }
        }
    }

    return hasError ? EXIT_FAILURE : EXIT_SUCCESS;
}

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
#include "opts.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <tuple>


extern "C" int iga_main(int argc, const char **argv)
{
    struct Opts baseOpts;

    std::string title = "Intel Graphics Assembler ";
    title += iga_version_string();

    opts::CmdlineSpec<Opts> cmdline(
        title.c_str(),
        opts::exeName(argv[0]),
        "  % " IGA_EXE "  file.asm  -p=9  -a   > file.krn\n"
        "  % " IGA_EXE "  file.asm  -p=11 -a  -o file.krn\n"
        "assembles 'file.asm' to 'file.krn' for GEN9 and GEN11, respectively\n"
        "  % " IGA_EXE "  file.asm9 >file.krn9\n"
        "similar to above, but the mode and platform are inferred by the extension\n"
        "(.asm* => -a) and (.*9 => -p=9)\n"
        "  % " IGA_EXE "  file.krn  -p=9  -d   > file.asm\n"
        "  % " IGA_EXE "  file.krn  -p=9  -d  -o file.asm\n"
        "disassembles 'file.krn' to 'file.asm' for GEN9\n"
        "  % " IGA_EXE "  file.krn9\n"
        "similar to above, but the mode and platform are inferred by the extension\n"
        "(.krn* => -d) and (.*9 => -p=9)\n"
        "  % " IGA_EXE "  file.bin  -p=9  -d  -Xprint-pc\n"
        "similar to the previous, but appends PC to each instruction;"
        " outputs to stdout\n");
    cmdline.defineFlag(
        "d",
        "disassemble",
        "disassembles the input file",
        "This makes iga disassemble the file (ignoring extension).  "
        "Without this iga attempts to infer the mode based on the extension.  "
        "Files ending in '.krn' are assumed binary without this option.",
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
        "Files ending in '.asm' are assumed syntax input without this option.",
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
    std::string platformExtendedDescription;
    std::vector<std::tuple<iga_gen_t,std::string>> platforms;
    {
        std::stringstream ss;
        const char *defaultPlatformInfo =
            "Examples of valid platforms are: GEN7.5, GEN8, GEN8LP, GEN9, ...\n"
            "We also accept code names: HSW, BDW, CHV, ...";
        size_t nps;
        if (iga_platforms_list(0,nullptr,&nps) != IGA_SUCCESS) {
            platformExtendedDescription = defaultPlatformInfo;
        } else {
            iga_gen_t *gens = (iga_gen_t *)alloca(nps);
            if (iga_platforms_list(nps,gens,&nps) != IGA_SUCCESS) {
               platformExtendedDescription = defaultPlatformInfo;
            } else {
                ss << "Valid platforms are:\n";
                for (size_t pi = 0; pi < nps/sizeof(iga_gen_t); pi++) {
                    const char *suffix = nullptr;
                    if (iga_platform_symbol_suffix(gens[pi],&suffix) == IGA_SUCCESS) {
                        ss << "  - " << std::setw(5) << std::left << suffix <<
                          " (for GEN" << suffix << ")\n";
                        platforms.emplace_back(gens[pi],suffix);
                    }
                } // for
                platformExtendedDescription = ss.str();
            }
        }
    } // platform extended description
    cmdline.defineOpt(
        "p",
        "platform",
        "PLATFORM",
        "specifies the platform (e.g. \"GEN9\")",
        platformExtendedDescription.c_str(),
        opts::OptAttrs::ALLOW_UNSET,
        [&] (const char *cinp, const opts::ErrorHandler &err, Opts &baseOpts) {
            std::string inp;
            while (*cinp) {
                inp += std::toupper(*cinp++);
            }
            if (inp.substr(0,3) == "GEN")
              inp = inp.substr(3); // GEN9 -> 9

            // try IGA-preferred names first
            iga_gen_t p = IGA_GEN_INVALID;
            for (const auto &pt : platforms) {
                std::string pnm = std::get<1>(pt);
                std::transform(pnm.begin(), pnm.end(), pnm.begin(), ::toupper);
                if (pnm == inp) {
                    p = std::get<0>(pt);
                    break;
                }
            }

            if (p != IGA_GEN_INVALID) {
              baseOpts.platform = p;
              return;
            }

            // fall back on code names and other names
            if (inp == "IVB" || inp == "7") {
                p = IGA_GEN7;
            } else if (inp == "HSW" || inp == "7P5" || inp == "7.5") {
                p = IGA_GEN7p5;
            } else if (inp == "BDW" || inp == "8") {
                p = IGA_GEN8;
            } else if (inp == "CHV" ||
                inp == "8LP" || inp == "8P1" || inp == "8.1")
            {
                p = IGA_GEN8lp;
            } else if (inp == "SKL" || inp == "9") {
                p = IGA_GEN9;
            } else if (inp == "BXT" ||
                inp == "9LP" || inp == "9P1" || inp == "9.1")
            {
                p = IGA_GEN9lp;
            } else if (inp == "KBL" || inp == "9P5" || inp == "9.5") {
                p = IGA_GEN9p5;
            } else if (inp == "CNL" || inp == "10") {
                p = IGA_GEN10;
            } else if (inp == "ICL" || inp == "CNLH" || inp == "11") {
                p = IGA_GEN11;
            } else {
                err("invalid platform option "
                    "(use option -h=p to list platforms)");
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
        baseOpts.outputFile);

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
        baseOpts.autoCompact);
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
        "dsd",
        nullptr,
        "decode send descriptor",
        "This mode attempts to decode send descriptors to messages.\n"
        " This is a best effort and not all messages or platforms are "
            "supported.\n"
        "\n"
        "SFIDS are: DC0, DC1, DC2, DCRO, GTWY, RC, SMPL, TS, URB"
        "\n"
        "\n"
        "EXAMPLES:\n"
        "  % iga -p=11 -Xdsd DC1 0x0000010C  0x04025C01\n"
        "    decodes message info for a GEN11 descriptor on SFID (DC1)\n",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::XDSD;
        });
    xGrp.defineFlag(
        "ifs",
        nullptr,
        "dump instruction fields",
        "This mode decodes instructions.\n"
        "\n"
        "EXAMPLES:\n"
        "  % iga -Xifs foo.krn9\n"
        "    decodes kernel from foo.krn9 (-p=9 inferred)\n"
        "  % iga -p=9 -Xifs \"64 00 43 00  a0 0a 05 00   24 01 00 80  00 00 00 00\"\n"
        "    decodes the hex bits above into ISA\n"
        "  % iga -p=9 -Xifs foo.krn9 \"64 00 43 00  a0 0a 05 00   24 01 00 80  00 00 00 00\"\n"
        "    decodes and compares the fields\n",
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::XIFS;
        });
    xGrp.defineFlag(
        "ldst-syntax",
        nullptr,
        "displays all ops for the given platform",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler &, Opts &baseOpts) {
            baseOpts.printLdSt = true;
        });
    xGrp.defineFlag(
        "legacy-directives",
        nullptr,
        "enable some IsaAsm era directives",
        "enables .default_execution_width and .default_type directives",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.legacyDirectives);
    xGrp.defineFlag(
        "list-ops",
        nullptr,
        "displays all ops for the given platform",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler, Opts &baseOpts) {
            baseOpts.mode = Opts::Mode::XLST;
        });
    xGrp.defineFlag(
        "native",
        nullptr,
        "Use IGA's native encoder/decoder",
        "",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.useNativeEncoder);
    xGrp.defineFlag(
        "no-autocompact",
        nullptr,
        "the inverse (default) of -Xautocompact",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *, const opts::ErrorHandler&, Opts &baseOpts) {
            baseOpts.autoCompact = false;
        });
    xGrp.defineFlag(
        "no-print-ldst",
        nullptr,
        "the inverse (default) of -Xprint-ldst",
        nullptr,
        opts::OptAttrs::ALLOW_UNSET,
        [] (const char *cinp, const opts::ErrorHandler &, Opts &baseOpts) {
            baseOpts.printLdSt = false;
        });
    xGrp.defineFlag(
        "syntax-exts",
        nullptr,
        "enables certain syntax extensions",
        "",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.syntaxExts);
    xGrp.defineFlag(
        "print-hex-floats",
        nullptr,
        "format floats in hexadecimal",
        "The intent is that IGA should always format floats in a way such that it "
          "will parse back them bit-precise; however, if we muck that up or if you just"
          "like the consistency, this option enables that behavior.",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.printHexFloats);
    xGrp.defineFlag(
        "print-pc",
        nullptr,
        "print PC with each instruction",
        "An instruction PC will be added as a comment",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.printInstructionPc);
    xGrp.defineFlag(
        "print-bits",
        nullptr,
        "prints bits decoded with each instruction",
        "The instruction bits are emitted with each instruction",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.printBits);
    xGrp.defineFlag(
        "print-deps",
        nullptr,
        "prints dependency info with each instruction",
        "IGA does not promise this to be exact at the moment. "
          "E.g. our analysis may be a local one only.",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.printDeps);
    xGrp.defineFlag(
        "print-ldst",
        nullptr,
        "enables load/store pseudo intructions where possible",
        "Send instructions are emitted as load/store instructions",
        opts::OptAttrs::ALLOW_UNSET,
        baseOpts.printLdSt);
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

    // ARGS
    cmdline.defineArg(
        "FILE",
        "the input files",
        "The input files to assemble.  This defaults to stdin.",
        opts::OptAttrs::ALLOW_UNSET | opts::OptAttrs::ALLOW_MULTI,
        [] (const char *inp, const opts::ErrorHandler &err, Opts &baseOpts) {
            baseOpts.inputFiles.push_back(inp);
        });

    cmdline.parse(argc, argv, baseOpts);

    // override various options not set
    auto optsForFile =
        [&] (const std::string &inpFile) {
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
    } else if (baseOpts.mode == Opts::XDSD) {
        hasError |= decodeSendDescriptor(baseOpts);
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

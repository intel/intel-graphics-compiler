/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "iga_main.hpp"

#include "ColoredIO.hpp"
#include "InstDiff.hpp"

static void errorInFile(const Opts &opts, const std::string &inpFile,
                        const char *msg) {
  std::stringstream ss;
  const char *tool = opts.mode == Opts::Mode::XDCMP ? "dcmp" : "ifs";
  ss << "-X" << tool << ": " << inpFile << ": " << msg;
  fatalExitWithMessage(ss.str());
};

static void ifXdcmpCheckForNonCompacted(Opts::Mode m, const std::string &str) {
  if (m == Opts::Mode::XDCMP && str.find("Compacted") != std::string::npos) {
    std::cerr << iga::Color::YELLOW
              << "warning: -Xdcmp should take a non-compacted instruction "
                 "for analysis\n"
              << iga::Reset::RESET;
  }
}

static void parseBitsFromFile(const std::string &inpFile,
                              Opts opts0, // copy
                              igax::Bits &bits) {
  Opts opts = opts0;
  opts.mode = Opts::Mode::AUTO; // allow file inference
  inferPlatformAndMode(inpFile, opts);

  if (opts.mode == Opts::Mode::DIS) {
    readBinaryFile(inpFile.c_str(), bits);
  } else if (opts.mode == Opts::Mode::ASM) {
    if (opts.platform == IGA_GEN_INVALID) {
      errorInFile(opts, inpFile, "platform required (-p)");
    }
    igax::Context ctx(opts.platform);
    std::string inpText = readTextFile(inpFile.c_str());
    ifXdcmpCheckForNonCompacted(opts0.mode, inpText);
    if (!assemble(opts, ctx, inpFile, inpText, bits)) {
      errorInFile(opts, inpFile, "failed to assemble file");
    }
  } else {
    errorInFile(opts, inpFile, "cannot infer mode from file extension");
  }
}

static void parseBitsAsSyntax(const std::string &inp, Opts &opts,
                              igax::Bits &bits) {
  // try as a string syntax:
  //   mov (8|M0)    (ov)f0.1   r0.0<1>:f     r1.1<0;1,0>:f
  igax::Context ctx(opts.platform);
  Opts opts1 = opts;
  ifXdcmpCheckForNonCompacted(opts.mode, inp);
  if (!assemble(opts, ctx, "<arg>", inp, bits)) {
    errorInFile(opts, "<arg>", "failed to assemble argument string");
  }
}

static void parseBitsAsHex(const std::string &inp, Opts &opts, bool hasSeps,
                           igax::Bits &bits) {
  size_t off = 0;
  auto atEof = [&]() { return off == inp.size(); };
  auto skipWs = [&]() {
    while (!atEof() && isspace(inp[off]))
      off++;
  };
  auto error = [&](std::string msg) {
    std::stringstream ss;
    const char *tool = opts.mode == Opts::Mode::XDCMP ? "dcmp" : "ifs";
    ss << "-X" << tool << ": malformed input:" << msg << "\n";
    ss << inp << "\n";
    for (size_t i = 0; i < off; i++) {
      ss << ' ';
    }
    ss << "^\n";
    fatalExitWithMessage(ss.str().c_str());
  };
  auto consumeIf = [&](char c) {
    if (!atEof() && inp[off] == c) {
      off++;
      return true;
    } else {
      return false;
    }
  };
  auto consume = [&](char c) {
    if (!consumeIf(c)) {
      std::stringstream ss;
      ss << "expected '" << c << "'";
      error(ss.str().c_str());
    }
  };
  // e.g. 61 00 43 00  a0 0a 05 00   24 01 00 80  00 00 00 00
  // 30 ...
  // ^ just the hex digit
  auto parseHexNybble = [&]() {
    if (atEof() || !isxdigit(inp[off])) {
      return -1;
    }
    int digit = 0;
    char chr = (char)::toupper(inp[off]);
    if (chr >= '0' && chr <= '9') {
      digit = chr - '0';
    } else if (chr >= 'A' && chr <= 'F') {
      digit = chr - 'A' + 10;
    } else {
      digit = -1;
    }
    off++;
    return digit;
  };
  // 30 45 46 76 ...
  auto parseByte = [&]() {
    int n1 = parseHexNybble();
    if (n1 < 0) {
      return -1;
    }
    int n2 = parseHexNybble();
    if (n2 < 0) {
      error("expected hex digit");
      return -1;
    }
    skipWs();
    return ((n1 << 4) | n2);
  };

  // skip any prefix
  skipWs();
  if (hasSeps) {
    // E.g. packed words in little endian format
    //    00000000`70000124`00050aa0`00430061
    //          W3`      W2`      W1`      W0
    //  or
    //    00000000_70000124_00050aa0_00430061 (C++17 literal separators)
    /// we order these in reverse order
    //
    // high bytes first, big-endian 128b word
    uint8_t ws[4][4];

    auto parseWord = [&](int wIx) {
      for (int i = 3; i >= 0; i--) {
        int b = parseByte();
        if (b < 0) {
          std::stringstream ss;
          ss << "invalid file, syntax, or raw byte sequence "
                "(w"
             << wIx << ")";
          error(ss.str().c_str());
        }
        ws[wIx][i] = (uint8_t)b;
      }
    };
    auto addWord = [&](uint8_t *w) {
      for (int i = 0; i < 4; i++)
        bits.push_back(w[i]);
    };

    parseWord(3);
    char sep = '_';
    if (consumeIf('`')) {
      sep = '`';
    } else {
      consume('_');
    }
    parseWord(2);
    if (consumeIf(sep)) {
      parseWord(1);
      consume(sep);
      parseWord(0);

      addWord(ws[0]);
      addWord(ws[1]);
      addWord(ws[2]);
      addWord(ws[3]);
    } else {
      // compacted
      addWord(ws[2]);
      addWord(ws[3]);
    }
    if (!atEof()) {
      error("invalid file, syntax, or raw byte sequence (stuff at EOF)");
    }
  } else if (inp.size() - off > 3 && isxdigit(inp[off]) &&
             isxdigit(inp[off + 1]) && isspace(inp[off + 2])) {
    // unpacked spaced bytes (e.g. from hex dump)
    // 31 00 13 14 54 ...
    int b;
    while ((b = parseByte()) != -1) {
      bits.push_back((uint8_t)b);
    }
  } else {
    // something else
    error("invalid file, syntax, or raw byte sequence");
  }
}

static igax::Bits parseBits(const std::string &inp, Opts &opts) {
  igax::Bits bits;
  if (doesFileExist(inp.c_str())) {
    if (opts.verbosity > 1) {
      std::cerr << "iga: parsing argument as file (since it exists)\n";
    }
    inferPlatform(inp, opts);
    parseBitsFromFile(inp, opts, bits);
  } else {
    bool allHexDigits = true, hasSeps = false;
    // strip leading 0x if it's present
    size_t i = 0;
    if (inp.size() >= 2 && inp[0] == '0' && (inp[1] == 'x' || inp[1] == 'X'))
      i += 2;
    for (; i < inp.size(); i++) {
      hasSeps |= inp[i] == '`' || inp[i] == '_';
      if (!isxdigit(inp[i]) && !isspace(inp[i]) && inp[i] != '`' &&
          inp[i] != '_') {
        allHexDigits = false;
        break;
      }
    }
    if (allHexDigits) {
      if (opts.verbosity > 1) {
        std::cerr << "iga: parsing argument as immediate hex string\n";
      }
      parseBitsAsHex(inp, opts, hasSeps, bits);
    } else {
      if (opts.verbosity > 1) {
        std::cerr << "iga: parsing argument as syntax string\n";
      }
      parseBitsAsSyntax(inp, opts, bits);
    }
  }
  return bits;
}

static bool decodeFieldsSingle(Opts opts) {
  inferPlatformAndMode(opts.inputFiles[0], opts);
  ensurePlatformIsSet(opts);

  igax::Bits bits = parseBits(opts.inputFiles[0], opts);
  std::ofstream *outfile = nullptr;
  if (!opts.outputFile.empty()) {
    outfile = new std::ofstream(opts.outputFile, std::ios::out);
  }
  std::ostream &os = outfile ? *outfile : std::cout;
  uint32_t fmtOpts = makeFormattingOpts(opts);
  iga_status_t st = iga::DecodeFields(iga::ToPlatform(opts.platform),
                                      opts.verbosity, opts.useNativeEncoder,
                                      fmtOpts, os, bits.data(), bits.size());
  if (st != IGA_SUCCESS) {
    std::cerr << iga_status_to_string(st) << "\n";
  }
  os.flush();
  if (outfile) {
    delete outfile;
  }

  return st != IGA_SUCCESS;
}

static bool decodeFieldsDiff(Opts opts) {
  Opts opts0 = opts;
  const char *source0 = nullptr, *source1 = nullptr;
  if (opts.inputFiles[0] == "@") { // comparing native and IGA encoders
    opts0.useNativeEncoder = !opts.useNativeEncoder;
    opts0.inputFiles[0] = opts.inputFiles[1];
    source0 = opts0.useNativeEncoder ? "IGA" : "GED";
    source1 = opts0.useNativeEncoder ? "GED" : "IGA";
  }
  igax::Bits bits0 = parseBits(opts0.inputFiles[0], opts0);
  Opts opts1 = opts;
  if (opts.inputFiles[1] == "@") {
    opts1.useNativeEncoder = !opts.useNativeEncoder;
    opts1.inputFiles[1] = opts.inputFiles[0];
    source0 = opts1.useNativeEncoder ? "IGA" : "GED";
    source1 = opts1.useNativeEncoder ? "GED" : "IGA";
  }
  igax::Bits bits1 = parseBits(opts1.inputFiles[1], opts1);

  if (opts.platform == IGA_GEN_INVALID) {
    if (opts0.platform != IGA_GEN_INVALID &&
        opts1.platform != IGA_GEN_INVALID && opts0.platform != opts1.platform) {
      fatalExitWithMessage(
          "-Xifs: conflicting platforms inferred (override with -p)");
    }
    if (opts0.platform != IGA_GEN_INVALID) {
      opts.platform = opts0.platform;
    } else if (opts1.platform != IGA_GEN_INVALID) {
      opts.platform = opts1.platform;
    } else {
      fatalExitWithMessage("-Xifs: unable to infer platform (use -p)");
    }
  } // else: we have (-p=...)

  uint32_t fmtOpts = makeFormattingOpts(opts);
  std::ofstream *outfile = nullptr;
  if (!opts.outputFile.empty()) {
    outfile = new std::ofstream(opts.outputFile, std::ios::out);
  }
  std::ostream &os = outfile ? *outfile : std::cout;
  //
  iga_status_t st =
      iga::DiffFields(static_cast<iga::Platform>(opts.platform), opts.verbosity,
                      opts.useNativeEncoder, fmtOpts, os, source0, bits0.data(),
                      bits0.size(), source1, bits1.data(), bits1.size());
  if (st == IGA_DIFF_FAILURE) {
    std::cout << "differences encountered\n";
  } else if (st != IGA_SUCCESS) {
    std::cerr << iga_status_to_string(st) << "\n";
  }

  os.flush();
  if (outfile) {
    delete outfile;
  }

  return st != IGA_SUCCESS;
}

bool decodeInstructionFields(const Opts &baseOpts) {
  if (baseOpts.inputFiles.size() == 1) {
    return decodeFieldsSingle(baseOpts);
  } else if (baseOpts.inputFiles.size() == 2) {
    return decodeFieldsDiff(baseOpts);
  } else {
    fatalExitWithMessage("-Xifs requires one or two arguments");
    return true;
  }
}

bool debugCompaction(Opts opts) {
  // turn off autocompaction
  // FIXME: this means -Xautocompact and {Compacted} will lead to wonky
  // results we should determine a sane thing to do pre-compacted data
  // (should rethink this, maybe just -Xifs compacted values)
  opts.autoCompact = false;

  if (opts.inputFiles.empty()) {
    fatalExitWithMessage("-Xdcmp requires an argument");
    return true;
  }
  inferPlatformAndMode(opts.inputFiles[0], opts);
  ensurePlatformIsSet(opts);

  igax::Bits bits = parseBits(opts.inputFiles[0], opts);

  std::ofstream *outfile = nullptr;
  if (!opts.outputFile.empty()) {
    outfile = new std::ofstream(opts.outputFile, std::ios::out);
  }
  std::ostream &os = outfile ? *outfile : std::cout;
  uint32_t fmtOpts = makeFormattingOpts(opts);

  std::string warnings;
  iga_status_t st = iga::DebugCompaction(iga::ToPlatform(opts.platform),
                                         opts.verbosity, opts.useNativeEncoder,
                                         fmtOpts, os, bits.data(), bits.size());
  if (st != IGA_SUCCESS) {
    std::cerr << "decode error: " << iga_status_to_string(st) << "\n";
  }
  emitYellowText(std::cerr, warnings);

  if (!opts.outputFile.empty()) {
    delete outfile;
  }

  return st != IGA_SUCCESS;
}

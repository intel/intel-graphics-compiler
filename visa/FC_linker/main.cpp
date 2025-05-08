/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The offline linker of CM FC.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#if defined(_MSC_VER)
static int opterr = 1,       // if error message should be printed.
    optind = 1,              // index into parent argv vector.
    optopt,                  // character checked for validity.
    optreset;                // reset getopt.
static const char *optarg;   // argument associated with option.

static const char BADCH = '?';
static const char BADARG = ':';
static const char *EMSG = "";

///
/// @brief getopt Parse argc/argv argument vector.
///
int getopt(int nargc, char *const nargv[], const char *ostr) {
  static const char *place = EMSG; // option letter processing
  const char *oli;           // option letter list index

  if (optreset || !*place) { // update scanning pointer
    optreset = 0;
    if (optind >= nargc || *(place = nargv[optind]) != '-') {
      place = EMSG;
      return (-1);
    }
    if (place[1] && *++place == '-') { // found "--"
      ++optind;
      place = EMSG;
      return (-1);
    }
  }
  // option letter okay?
  if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(ostr, optopt))) {
    // if the user didn't specify '-' as an option, assume it means -1.
    if (optopt == (int)'-')
      return (-1);
    if (!*place)
      ++optind;
    if (opterr && *ostr != ':')
      (void)printf("illegal option -- %c\n", optopt);
    return (BADCH);
  }
  if (*++oli != ':') { // don't need argument.
    optarg = NULL;
    if (!*place)
      ++optind;
  } else {      // need an argument.
    if (*place) // no white space
      optarg = place;
    else if (nargc <= ++optind) { // no arg
      place = EMSG;
      if (*ostr == ':')
        return (BADARG);
      if (opterr)
        (void)printf("option requires an argument -- %c\n", optopt);
      return (BADCH);
    } else // white space
      optarg = nargv[optind];
    place = EMSG;
    ++optind;
  }
  return (optopt); // dump back option letter.
}
#else
#include <getopt.h>
#endif

#include "PatchInfoDumper.h"
#include "PatchInfoLinker.h"
#include "PatchInfoReader.h"
#include "PatchInfoRecord.h"
#include "cm_fc_ld.h"

static bool Brief = false;

static void usage(FILE *fp, int argc, char *argv[]) {
  std::fprintf(fp, "usage: %s [-c|-d|-q] [-e <string>] [-o <file>] file...\n\n",
               argv[0]);
  std::fprintf(fp, "CM fast composite offline linker.\n\n");
  std::fprintf(fp, "%-28s%s\n", "  -c", "Combine kernels specified.");
  std::fprintf(fp, "%-28s%s\n", "  -d", "Dump the patch info.");
  std::fprintf(fp, "%-28s%s\n", "  -q", "Query the callee info.");
  std::fprintf(fp, "%-28s%s\n", "  -r", "Read the patch info.");
  std::fprintf(fp, "%-28s%s\n", "  -o <file>", "Place the output into <file>.");
  std::fprintf(fp, "%-28s%s\n", "  -e <string>", "Extra link options.");
  std::fprintf(fp, "\n");
}

static std::string loadFile(const char *FPath) {
  std::ifstream IFS(FPath, std::ios::in | std::ios::binary);
  if (!IFS.good()) {
    std::cerr << "Cannot open '" << FPath << "' for read!\n";
    std::exit(EXIT_FAILURE);
  }
  std::ostringstream SS;
  SS << IFS.rdbuf();
  return std::move(SS.str());
}

static void dump(const char *FPath, const char *BinPath, FILE *fp) {
  std::string Buf = loadFile(FPath);
  std::string Bin;
  if (BinPath)
    Bin = loadFile(BinPath);
  dumpPatchInfo((void *)Buf.data(), Buf.size(), (void *)Bin.data(), Bin.size(),
                fp);
  return;
}

static void dump_args(int argc, char *argv[], const char *Out) {
  FILE *fp = stdout;
  if (Out) {
#if defined(_MSC_VER)
    fp = nullptr;
    fopen_s(&fp, Out, "w");
#else
    fp = std::fopen(Out, "w");
#endif
    if (!fp) {
      std::cerr << "Cannot open '" << Out << "' for write!\n";
      std::exit(EXIT_FAILURE);
    }
  }
  if (argc > 0) {
    char *Path = argv[0];
    char *BinPath = (argc > 1) ? argv[1] : nullptr;
    dump(Path, BinPath, fp);
  }
  if (Out)
    fclose(fp);
}

static void query(const char *FPath, FILE *fp) {
  std::string Buf = loadFile(FPath);
  // std::size_t Size = 256;
  const char *Name = nullptr;
  cm_fc_link_type LinkType = CM_FC_LINK_TYPE_NONE;
  cm::patch::Collection C;
  int Ret;
  Ret = cm_fc_get_callee_info(Buf.data(), Buf.size(), (void *)&C, &LinkType);
  if (Ret == CM_FC_OK) {
    cm::patch::Binary &Bin = *C.bin_begin();
    cm::patch::Relocation &Rel = *Bin.rel_begin();
    Name = Rel.getSymbol()->getName();
  }

  static const char *LinkTypeString[] = {"NONE", "CALLER", "CALLEE"};

  std::cout << "Link Type: " << LinkTypeString[LinkType] << '\n';
  if (Name == nullptr)
    std::cout << "No callee.\n";
  else
    std::cout << "Callee Name: '" << Name << "'\n";
}

static void query_args(int argc, char *argv[], const char *Out) {
  FILE *fp = stdout;
  if (Out) {
#if defined(_MSC_VER)
    fp = nullptr;
    fopen_s(&fp, Out, "w");
#else
    fp = std::fopen(Out, "w");
#endif
    if (!fp) {
      std::cerr << "Cannot open '" << Out << "' for write!\n";
      std::exit(EXIT_FAILURE);
    }
  }
  for (int i = 0; i != argc; ++i)
    query(argv[i], fp);
  if (Out)
    fclose(fp);
}

static void combine(const std::vector<std::string> &Bufs, char *argv[],
                    FILE *fp, const char *ExtraOpts) {
  std::vector<cm_fc_kernel_t> Kernels;
  cm::patch::Collection C;

  for (unsigned i = 0, e = unsigned(Bufs.size()); i != e; i += 2) {
    cm_fc_kernel_t K;
    K.patch_buf = Bufs[i].data();
    K.patch_size = Bufs[i].size();
    K.binary_buf = Bufs[i + 1].data();
    K.binary_size = Bufs[i + 1].size();
    Kernels.push_back(K);
  }

  if (linkPatchInfo(C, Kernels.size(), Kernels.data(), ExtraOpts)) {
    std::cerr << "Failed to combine kernels.\n";
    return;
  }

  const std::string &B = C.getLinkedBinary();

  std::fwrite(B.data(), 1, B.size(), fp);
}

static void combine_args(int argc, char *argv[], const char *Out,
                         const char *ExtraOpts) {
  if (argc <= 0 || (argc & 1) != 0) {
    std::cerr << "Kernel combining needs pairs of patch-info and bin files.\n";
    std::exit(EXIT_FAILURE);
  }

  FILE *fp = stdout;
  if (Out) {
#if defined(_MSC_VER)
    fp = nullptr;
    fopen_s(&fp, Out, "w");
#else
    fp = std::fopen(Out, "w");
#endif
    if (!fp) {
      std::cerr << "Cannot open '" << Out << "' for write!\n";
      std::exit(EXIT_FAILURE);
    }
  }

  std::vector<std::string> Bufs;
  for (int i = 0; i != argc; i += 2) {
    char *InfoPath = argv[i + 0];
    char *BinPath = argv[i + 1];
    Bufs.push_back(std::move(loadFile(InfoPath)));
    Bufs.push_back(std::move(loadFile(BinPath)));
  }
  combine(Bufs, argv, fp, ExtraOpts);
  if (Out)
    fclose(fp);
}

static void read(const char *FPath, const char *BinPath, FILE *fp) {
  std::string Buf = loadFile(FPath);
  std::string Bin;
  if (BinPath)
    Bin = loadFile(BinPath);
  cm::patch::Collection C;
  if (readPatchInfo(Buf.data(), Buf.size(), C))
    return;
  if (C.bin_begin() == C.bin_end())
    return;
  if (Bin.empty())
    return;

  auto &Binary = *C.bin_begin();

  unsigned n = 0;
  for (auto RI = Binary.initreg_begin(), RE = Binary.initreg_end(); RI != RE;
       ++RI, ++n) {
    std::fprintf(fp, "  [%3u]:", n);
    std::fprintf(fp, "  %08x", RI->getOffset());
    std::fprintf(fp, "  r%-3u", RI->getRegNo());
    std::fprintf(fp, "  %04x", RI->getDUT());
    std::fprintf(fp, "\n");
  }

  n = 0;
  for (auto RI = Binary.finireg_begin(), RE = Binary.finireg_end(); RI != RE;
       ++RI, ++n) {
    std::fprintf(fp, "  [%3u]:", n);
    std::fprintf(fp, "  %08x", RI->getOffset());
    std::fprintf(fp, "  r%-3u", RI->getRegNo());
    std::fprintf(fp, "  %04x", RI->getDUT());
    std::fprintf(fp, "\n");
  }

  return;
}

static void read_args(int argc, char *argv[], const char *Out) {
  FILE *fp = stdout;
  if (Out) {
#if defined(_MSC_VER)
    fp = nullptr;
    fopen_s(&fp, Out, "w");
#else
    fp = std::fopen(Out, "w");
#endif
    if (!fp) {
      std::cerr << "Cannot open '" << Out << "' for write!\n";
      std::exit(EXIT_FAILURE);
    }
  }
  if (argc > 0) {
    char *Path = argv[0];
    char *BinPath = (argc > 1) ? argv[1] : nullptr;
    read(Path, BinPath, fp);
  }
  if (Out)
    fclose(fp);
}

int main(int argc, char *argv[]) {
  const char *ExtraOptions = nullptr;
  const char *Out = nullptr;
  char Action = '\0';

  int Opt;
  while ((Opt = getopt(argc, argv, "cdqrBDe:o:")) != -1) {
    switch (Opt) {
    case 'c':
    case 'd':
    case 'q':
    case 'r':
      if (Action != '\0') {
        std::cerr << "Only one mode could be specified. "
                     "Query mode is already specified\n\n";
        usage(stderr, argc, argv);
      }
      Action = (char)Opt;
      break;
    case 'o':
      Out = optarg;
      break;
    case 'e':
      ExtraOptions = optarg;
      break;
    default:
      usage(stdout, argc, argv);
      std::exit(EXIT_FAILURE);
      break;
    }
  }

  if (optind >= argc) {
    std::cerr << "Expect argument(s) after options.\n\n";
    usage(stderr, argc, argv);
    std::exit(EXIT_FAILURE);
  }

  switch (Action) {
  case 'c':
    combine_args(argc - optind, &argv[optind], Out, ExtraOptions);
    break;
  case 'd':
    dump_args(argc - optind, &argv[optind], Out);
    break;
  case 'q':
    query_args(argc - optind, &argv[optind], Out);
    break;
  case 'r':
    read_args(argc - optind, &argv[optind], Out);
    break;
  }

  return 0;
}

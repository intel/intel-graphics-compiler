/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <fstream>
#include <iostream>
#include <string>

#include "Assertions.h"
#include "BinaryCISAEmission.h"
#include "Common_ISA.h"
#include "Common_ISA_framework.h"
#include "DebugInfo.h"
#include "Option.h"
#include "PlatformInfo.h"
#include "Timer.h"
#include "VISAKernel.h"
#include "visa_igc_common_header.h"


#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Path.h>

///
/// Reads byte code and calls the builder API as it does so.
///
extern bool readIsaBinaryNG(const char *buf, CISA_IR_Builder *builder,
                            std::vector<VISAKernel *> &kernels,
                            const char *kernelName, unsigned int majorVersion,
                            unsigned int minorVersion);

#ifndef DLL_MODE
int parseText(llvm::StringRef fileName, int argc, const char *argv[],
              Options &opt);
#endif

#ifndef DLL_MODE
int parseBinary(std::string fileName, int argc, const char *argv[],
                Options &opt) {
  vISA::Mem_Manager mem(4096);

  /// Try opening the file.
  FILE *isafile = fopen(fileName.c_str(), "rb");
  if (!isafile) {
    std::cerr << fileName << ": cannot open file\n";
    return EXIT_FAILURE;
  }

  /// Calculate file size.
  fseek(isafile, 0, SEEK_END);
  long isafilesize = ftell(isafile);
  rewind(isafile);

  /// Reading file into buffer.
  char *isafilebuf = (char *)mem.alloc(isafilesize);
  if (isafilesize != fread(isafilebuf, 1, isafilesize, isafile)) {
    std::cerr << fileName << ": Unable to read entire file into buffer.\n";
    return EXIT_FAILURE;
  }
  fclose(isafile);

  TARGET_PLATFORM platform =
      static_cast<TARGET_PLATFORM>(opt.getuInt32Option(vISA_PlatformSet));
  VISA_BUILDER_OPTION builderOption =
      (platform == GENX_NONE) ? VISA_BUILDER_VISA : VISA_BUILDER_BOTH;
  CISA_IR_Builder *cisa_builder = NULL;

  CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_DEFAULT, builderOption,
                                 platform, argc, argv);
  vISA_ASSERT(cisa_builder, "cisa_builder is NULL.");

  std::vector<VISAKernel *> kernels;
  bool success = readIsaBinaryNG(isafilebuf, cisa_builder, kernels, NULL,
                                 COMMON_ISA_MAJOR_VER, COMMON_ISA_MINOR_VER);
  if (!success)
    return EXIT_FAILURE;
  std::string binFileName;

  if (auto cisaBinaryName =
          cisa_builder->m_options.getOptionCstr(vISA_OutputvISABinaryName)) {
    binFileName = cisaBinaryName;
  }

  auto result = cisa_builder->Compile((char *)binFileName.c_str());
  if (result != VISA_SUCCESS) {
    return result;
  }
  result = CISA_IR_Builder::DestroyBuilder(cisa_builder);
  return result;
}
#endif

#ifndef DLL_MODE

int main(int argc, const char *argv[]) {
  std::cout << argv[0];
  for (int i = 1; i < argc; i++)
    std::cout << " " << argv[i];
  std::cout << "\n";

#if 0
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_crtBreakAlloc = 4763;
#endif

  // here we let the tool quit instead of crashing
  if (argc < 2) {
    Options::showUsage(COUT_ERROR);
    return 1;
  }

  int startPos = 1;
  bool parserMode = false;
  llvm::StringRef input = argv[1];
  llvm::StringRef ext = llvm::sys::path::extension(input);
  if (ext == ".visaasm" || ext == ".isaasm") {
    startPos++;
    parserMode = true;
  } else if (ext == ".isa") {
    startPos++;
  }

  // Note that we process options twice in offline mode (once here and once in
  // createBuilder), since we have to get some essential options such as
  // platform and stepping
  Options opt;
  if (!opt.parseOptions(argc - startPos, &argv[startPos])) {
    return 1;
  }

  TARGET_PLATFORM platform =
      static_cast<TARGET_PLATFORM>(opt.getuInt32Option(vISA_PlatformSet));
  bool dumpCommonIsa = false;
  bool generateBinary = false;
  opt.getOption(vISA_GenerateISAASM, dumpCommonIsa);

  if (opt.getOption(vISA_isParseMode))
    parserMode = true;

  opt.getOption(vISA_GenerateBinary, generateBinary);
  if (platform == GENX_NONE &&
      ((!dumpCommonIsa && !parserMode) || generateBinary)) {
    std::cerr << "USAGE: must specify platform\n";
    Options::showUsage(COUT_ERROR);
    return EXIT_FAILURE;
  }

  if (opt.getOptionCstr(vISA_DecodeDbg)) {
    const char *dbgName;
    opt.getOption(vISA_DecodeDbg, dbgName);
    decodeAndDumpDebugInfo((char *)dbgName, platform);
    return EXIT_SUCCESS;
  }

  // TODO: Will need to adjust the platform from option for parseBinary() and
  // parseText() if not specifying platform is allowed at this point.

  //
  // for debug print lex results to stdout (default)
  // for release open "lex.out" and redirect lex results
  //

  // holds storage for the stem of input file.
  std::string stem;

  // If the file name is not set by the user, then use the same
  // file name (drop path)
  // we do not include an extension as other logic suffixes that later
  // depending on the desired
  //   e.g. foo/bar.visaasm ==> ./bar
  if (!opt.isOptionSetByUser(VISA_AsmFileName)) {
    stem = llvm::sys::path::stem(input).str();
    opt.setOptionInternally(VISA_AsmFileName, stem.c_str());
  }

  int err = VISA_SUCCESS;
  if (parserMode) {
    err = parseText(input, argc - startPos, &argv[startPos], opt);
  } else {
    err = parseBinary(input.str(), argc - startPos, &argv[startPos], opt);
  }

#ifdef COLLECT_ALLOCATION_STATS
#if 0
    cout << "# allocation: " << numAllocations << endl;
    cout << "total allocation size: " << (totalAllocSize / 1024) << " KB" << endl;
    cout << "# mallocs: " << numMallocCalls << endl;
    cout << "total malloc size: " << (totalMallocSize / 1024) << " KB" << endl;
    cout << "# memory managers: " << numMemManagers << endl;
    cout << "Max Arena list length: " << maxArenaLength << endl;
#else
  cout << numAllocations << "\t" << (totalAllocSize / 1024) << "\t"
       << numMallocCalls << "\t" << (totalMallocSize / 1024) << "\t"
       << numMemManagers << "\t" << maxArenaLength << endl;
#endif
#endif
  return err;
}
#endif

#ifndef DLL_MODE

extern int CISAparse(CISA_IR_Builder *builder);

int parseText(llvm::StringRef fileName, int argc, const char *argv[],
              Options &opt) {
  TARGET_PLATFORM platform =
      static_cast<TARGET_PLATFORM>(opt.getuInt32Option(vISA_PlatformSet));
  VISA_BUILDER_OPTION builderOption =
      (platform == GENX_NONE) ? VISA_BUILDER_VISA : VISA_BUILDER_BOTH;

  CISA_IR_Builder *cisa_builder = nullptr;

  auto err = CISA_IR_Builder::CreateBuilder(
      cisa_builder, vISA_ASM_READER, builderOption, platform, argc, argv);
  if (err)
    return EXIT_FAILURE;

  CISAin = fopen(fileName.data(), "r");
  if (!CISAin) {
    std::cerr << fileName.data() << ": cannot open vISA assembly file\n";
    return EXIT_FAILURE;
  }

  CISAdebug = 0;
  int fail = CISAparse(cisa_builder);
  fclose(CISAin);
  if (fail) {
    if (cisa_builder->HasParseError()) {
      std::cerr << cisa_builder->GetParseError() << "\n";
    } else {
      std::cerr << "error during parsing: CISAparse() returned " << fail
                << "\n";
    }
    return EXIT_FAILURE;
  }

  // If the input text lacks "OutputAsmPath" (and it should), then we can
  // override it with the implied name here.
  auto k = cisa_builder->get_kernel();
  if (k->getOutputAsmPath().empty()) {
    const char *outputPrefix = opt.getOptionCstr(VISA_AsmFileName);
    k->setOutputAsmPath(outputPrefix);
    cisa_builder->getOptions()->setOptionInternally(VISA_AsmFileName,
                                                    outputPrefix);
  }

  std::string binFileName;
  if (auto cisaBinaryName = opt.getOptionCstr(vISA_OutputvISABinaryName)) {
    binFileName = cisaBinaryName;
  } else {
    // Use VISA_AsmFileName as the stem of the final binary.
    binFileName = opt.getOptionCstr(VISA_AsmFileName);
    binFileName += ".isa";
  }

  auto compErr = cisa_builder->Compile(binFileName.c_str());
  if (compErr) {
    std::cerr << cisa_builder->GetCriticalMsg() << "\n";
    return EXIT_FAILURE;
  }
  auto dstbErr = CISA_IR_Builder::DestroyBuilder(cisa_builder);
  return dstbErr ? EXIT_FAILURE : EXIT_SUCCESS;
}
#endif

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
#include "VISADefines.h"
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

#define JIT_SUCCESS 0
#define JIT_INVALID_INPUT 1
#define JIT_CISA_ERROR 3
#define JIT_INVALID_PLATFORM 5

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

#define COMMON_ISA_MAX_KERNEL_NAME_LEN 255

int JITCompileAllOptions(const char *kernelName, const void *kernelIsa,
                         unsigned int kernelIsaSize, void *&genBinary,
                         unsigned int &genBinarySize, const char *platformStr,
                         int majorVersion, int minorVersion, int numArgs,
                         const char *args[], char *errorMsg,
                         vISA::FINALIZER_INFO *jitInfo, void *gtpin_init) {
  // This function becomes the new entry point even for JITCompile clients.
  if (kernelName == NULL || kernelIsa == NULL ||
      std::string_view(kernelName).size() > COMMON_ISA_MAX_KERNEL_NAME_LEN) {
    return JIT_INVALID_INPUT;
  }
  // This must be done before processing the options,
  // as some options depend on the platform
  TARGET_PLATFORM platform =
      vISA::PlatformInfo::getVisaPlatformFromStr(platformStr);
  if (platform == GENX_NONE) {
    return JIT_INVALID_PLATFORM;
  }

  genBinary = NULL;
  genBinarySize = 0;

  char *isafilebuf = (char *)kernelIsa;
  CISA_IR_Builder *cisa_builder = NULL;

  // HW mode: default: GEN path; if dump/verify/debug: Both path
  VISA_BUILDER_OPTION builderOption = VISA_BUILDER_GEN;
#if defined(_DEBUG)
  builderOption = VISA_BUILDER_BOTH;
#endif

  CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_DEFAULT, builderOption,
                                 platform, numArgs, args);
  cisa_builder->setGtpinInit(gtpin_init);

  if (!cisa_builder) {
    return JIT_CISA_ERROR;
  }

  std::vector<VISAKernel *> kernels;
  bool passed = readIsaBinaryNG(isafilebuf, cisa_builder, kernels, kernelName,
                                majorVersion, minorVersion);

  if (!passed) {
    return JIT_CISA_ERROR;
  }

  cisa_builder->Compile("");

  VISAKernel *kernel = kernels[0];
  vISA::FINALIZER_INFO *tempJitInfo = NULL;
  void *genxBinary = NULL;
  int size = 0;
  kernel->GetJitInfo(tempJitInfo);
  kernel->GetGenxDebugInfo(tempJitInfo->genDebugInfo,
                           tempJitInfo->genDebugInfoSize);

  if (gtpin_init) {
    // Return free GRF info
    kernel->GetGTPinBuffer(tempJitInfo->freeGRFInfo,
                           tempJitInfo->freeGRFInfoSize);
  }

  if (jitInfo != NULL && tempJitInfo != NULL)
    memcpy_s(jitInfo, sizeof(vISA::FINALIZER_INFO), tempJitInfo,
             sizeof(vISA::FINALIZER_INFO));

  if (!(0 == kernel->GetGenxBinary(genxBinary, size) && genxBinary != NULL)) {
    return JIT_INVALID_INPUT;
  }
  genBinary = genxBinary;
  genBinarySize = size;

  CISA_IR_Builder::DestroyBuilder(cisa_builder);
  return JIT_SUCCESS;
}

/**
 * This is the main entry point for CM.
 */
DLL_EXPORT int JITCompile(const char *kernelName, const void *kernelIsa,
                          unsigned int kernelIsaSize, void *&genBinary,
                          unsigned int &genBinarySize, const char *platform,
                          int majorVersion, int minorVersion, int numArgs,
                          const char *args[], char *errorMsg,
                          vISA::FINALIZER_INFO *jitInfo) {
  // JITCompile will invoke the other JITCompile API that supports relocation.
  // Via this path, relocs will be NULL. This way we can share a single
  // implementation of JITCompile.

  return JITCompileAllOptions(
      kernelName, kernelIsa, kernelIsaSize, genBinary, genBinarySize, platform,
      majorVersion, minorVersion, numArgs, args, errorMsg, jitInfo, nullptr);
}

DLL_EXPORT int JITCompile_v2(const char *kernelName, const void *kernelIsa,
                             unsigned int kernelIsaSize, void *&genBinary,
                             unsigned int &genBinarySize, const char *platform,
                             int majorVersion, int minorVersion, int numArgs,
                             const char *args[], char *errorMsg,
                             vISA::FINALIZER_INFO *jitInfo, void *gtpin_init) {
  // JITCompile will invoke the other JITCompile API that supports relocation.
  // Via this path, relocs will be NULL. This way we can share a single
  // implementation of JITCompile.
  return JITCompileAllOptions(
      kernelName, kernelIsa, kernelIsaSize, genBinary, genBinarySize, platform,
      majorVersion, minorVersion, numArgs, args, errorMsg, jitInfo, gtpin_init);
}

DLL_EXPORT void getJITVersion(unsigned int &majorV, unsigned int &minorV) {
  majorV = COMMON_ISA_MAJOR_VER;
  minorV = COMMON_ISA_MINOR_VER;
}

#ifndef DLL_MODE

int main(int argc, const char *argv[]) {
  // Dump to cerr to avoid polluting cout as cout might be used for lit test.
  std::cerr << argv[0];
  for (int i = 1; i < argc; i++)
    std::cerr << " " << argv[i];
  std::cerr << "\n";

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

DLL_EXPORT void freeBlock(void *ptr) {
  if (ptr != NULL) {
    free(ptr);
  }
}


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
  for (auto it = cisa_builder->kernel_begin(), ie = cisa_builder->kernel_end();
       it != ie; ++it) {
    auto k = *it;
    if (k->getIsKernel() && k->getOutputAsmPath().empty()) {
      const char *outputPrefix = opt.getOptionCstr(VISA_AsmFileName);
      k->setOutputAsmPath(outputPrefix);
      cisa_builder->getOptions()->setOptionInternally(VISA_AsmFileName,
                                                      outputPrefix);
    }
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

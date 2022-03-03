/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <iostream>
#include <fstream>
#include <string>


#include "visa_igc_common_header.h"
#include "Common_ISA_framework.h"
#include "VISAKernel.h"
#include "Option.h"
#include "Common_ISA.h"
#include "BinaryCISAEmission.h"
#include "Timer.h"
#include "DebugInfo.h"
#include "PlatformInfo.h"

#ifndef DLL_MODE
#include "EnumFiles.hpp"
#endif


///
/// Reads byte code and calls the builder API as it does so.
///
extern bool readIsaBinaryNG(const char *buf, CISA_IR_Builder *builder,
                            std::vector<VISAKernel *> &kernels,
                            const char *kernelName, unsigned int majorVersion,
                            unsigned int minorVersion);

#ifndef DLL_MODE
int parseText(
    std::string fileName,
    int argc, const char *argv[], Options &opt);
#endif

// default size of the physical reg pool mem manager in bytes
#define PHY_REG_MEM_SIZE   (16*1024)

// default size of the kernel mem manager in bytes
#define KERNEL_MEM_SIZE    (4*1024*1024)

#define JIT_SUCCESS                     0
#define JIT_INVALID_INPUT               1
#define JIT_CISA_ERROR                  3
#define JIT_INVALID_PLATFORM            5

#ifndef DLL_MODE
int parseBinary(
    std::string fileName,
    int argc, const char *argv[], Options &opt)
{
    // read in common isa binary file
    int c;
    char *buf;
    FILE *commonISAInput = NULL;
    long file_size;
    unsigned byte_pos = 0;
    common_isa_header commonISAHeader;
    vISA::Mem_Manager globalMem(KERNEL_MEM_SIZE);

    // open common isa file
    if ((commonISAInput = fopen(fileName.c_str(), "rb")) == NULL)
    {
        std::cerr << fileName << ": cannot open file\n";
        return EXIT_FAILURE;
    }

    fseek(commonISAInput, 0, SEEK_END);
    file_size = ftell(commonISAInput);
    fseek(commonISAInput, 0, SEEK_SET);

    buf = (char*)globalMem.alloc(file_size + 1);
    char *buf_ptr = buf;
    while ((c = fgetc(commonISAInput)) != EOF) {
        *buf_ptr = c;
        buf_ptr++;
    }

    processCommonISAHeader(commonISAHeader, byte_pos, buf, &globalMem);
    fclose(commonISAInput);
    vISA::Mem_Manager mem(4096);

    /// Try opening the file.
    FILE* isafile = fopen(fileName.c_str(), "rb");
    if (!isafile) {
        std::cerr << fileName << ": cannot open file\n";
        return EXIT_FAILURE;
    }

    /// Calculate file size.
    fseek(isafile, 0, SEEK_END);
    long isafilesize = ftell(isafile);
    rewind(isafile);

    /// Reading file into buffer.
    char* isafilebuf = (char*)mem.alloc(isafilesize);
    if (isafilesize != fread(isafilebuf, 1, isafilesize, isafile))
    {
        std::cerr << fileName << ": Unable to read entire file into buffer.\n";
        return EXIT_FAILURE;
    }
    fclose(isafile);

    TARGET_PLATFORM platform = static_cast<TARGET_PLATFORM>(opt.getuInt32Option(vISA_PlatformSet));
    VISA_BUILDER_OPTION builderOption =
        (platform == GENX_NONE) ? VISA_BUILDER_VISA : VISA_BUILDER_BOTH;
    CISA_IR_Builder* cisa_builder = NULL;

    CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_DEFAULT, builderOption, platform, argc, argv);
    MUST_BE_TRUE(cisa_builder, "cisa_builder is NULL.");

    std::vector<VISAKernel*> kernels;
    bool success = readIsaBinaryNG(
        isafilebuf, cisa_builder, kernels, NULL, COMMON_ISA_MAJOR_VER, COMMON_ISA_MINOR_VER);
    if (!success)
        return EXIT_FAILURE;
    std::string binFileName;

    if (cisa_builder->m_options.getOption(vISA_OutputvISABinaryName))
    {
        const char* cisaBinaryName = NULL;
        cisa_builder->m_options.getOption(vISA_GetvISABinaryName, cisaBinaryName);
        binFileName = cisaBinaryName;
    }

    auto result = cisa_builder->Compile((char*)binFileName.c_str());
    if (result != VISA_SUCCESS)
    {
        return result;
    }
    result = CISA_IR_Builder::DestroyBuilder(cisa_builder);
    return result;
}
#endif

int JITCompileAllOptions(const char* kernelName,
    const void* kernelIsa,
    unsigned int kernelIsaSize,
    void* &genBinary,
    unsigned int& genBinarySize,
    const char* platformStr,
    int majorVersion,
    int minorVersion,
    int numArgs,
    const char* args[],
    char* errorMsg,
    FINALIZER_INFO* jitInfo,
    void* gtpin_init)
{
    // This function becomes the new entry point even for JITCompile clients.
    if (kernelName == NULL || kernelIsa == NULL || strlen(kernelName) > COMMON_ISA_MAX_KERNEL_NAME_LEN)
    {
        return JIT_INVALID_INPUT;
    }
    // This must be done before processing the options,
    // as some options depend on the platform
    TARGET_PLATFORM platform = vISA::PlatformInfo::getVisaPlatformFromStr(platformStr);
    if (platform == GENX_NONE)
    {
        return JIT_INVALID_PLATFORM;
    }

    genBinary = NULL;
    genBinarySize = 0;

    char* isafilebuf = (char*)kernelIsa;
    CISA_IR_Builder* cisa_builder = NULL;
    // HW mode: default: GEN path; if dump/verify: Both path
    VISA_BUILDER_OPTION builderOption = VISA_BUILDER_GEN;

    CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_DEFAULT, builderOption, platform, numArgs, args);
    cisa_builder->setGtpinInit(gtpin_init);

    if (!cisa_builder)
    {
        return JIT_CISA_ERROR;
    }

    std::vector<VISAKernel*> kernels;
    bool passed = readIsaBinaryNG(isafilebuf, cisa_builder, kernels, kernelName, majorVersion, minorVersion);

    if (!passed)
    {
        return JIT_CISA_ERROR;
    }

    cisa_builder->Compile("");

    VISAKernel* kernel = kernels[0];
    FINALIZER_INFO* tempJitInfo = NULL;
    void* genxBinary = NULL;
    int size = 0;
    kernel->GetJitInfo(tempJitInfo);
    kernel->GetGenxDebugInfo(tempJitInfo->genDebugInfo, tempJitInfo->genDebugInfoSize);

    if (gtpin_init)
    {
        // Return free GRF info
        kernel->GetGTPinBuffer(tempJitInfo->freeGRFInfo, tempJitInfo->freeGRFInfoSize);
    }

    if (jitInfo != NULL && tempJitInfo != NULL)
        memcpy_s(jitInfo, sizeof(FINALIZER_INFO), tempJitInfo, sizeof(FINALIZER_INFO));

    if (!(0 == kernel->GetGenxBinary(genxBinary, size) && genxBinary != NULL))
    {
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
DLL_EXPORT int JITCompile(const char* kernelName,
                          const void* kernelIsa,
                          unsigned int kernelIsaSize,
                          void* &genBinary,
                          unsigned int& genBinarySize,
                          const char* platform,
                          int majorVersion,
                          int minorVersion,
                          int numArgs,
                          const char* args[],
                          char* errorMsg,
                          FINALIZER_INFO* jitInfo)
{
    // JITCompile will invoke the other JITCompile API that supports relocation.
    // Via this path, relocs will be NULL. This way we can share a single
    // implementation of JITCompile.

    return JITCompileAllOptions(kernelName, kernelIsa, kernelIsaSize, genBinary, genBinarySize,
        platform, majorVersion, minorVersion, numArgs, args, errorMsg, jitInfo, nullptr);
}

DLL_EXPORT int JITCompile_v2(const char* kernelName,
    const void* kernelIsa,
    unsigned int kernelIsaSize,
    void* &genBinary,
    unsigned int& genBinarySize,
    const char* platform,
    int majorVersion,
    int minorVersion,
    int numArgs,
    const char* args[],
    char* errorMsg,
    FINALIZER_INFO* jitInfo,
    void* gtpin_init)
{
    // JITCompile will invoke the other JITCompile API that supports relocation.
    // Via this path, relocs will be NULL. This way we can share a single
    // implementation of JITCompile.
    return JITCompileAllOptions(kernelName, kernelIsa, kernelIsaSize, genBinary, genBinarySize,
        platform, majorVersion, minorVersion, numArgs, args, errorMsg, jitInfo, gtpin_init);
}

DLL_EXPORT void getJITVersion(unsigned int& majorV, unsigned int& minorV)
{
    majorV = COMMON_ISA_MAJOR_VER;
    minorV = COMMON_ISA_MINOR_VER;
}

#ifndef  DLL_MODE

/// Returns true if a string ends with an expected suffix.
static bool endsWith(const std::string &str, const std::string &suf)
{
    if (str.length() < suf.length())
        return false;
    return 0 == str.compare(str.length() - suf.length(), suf.length(), suf);
}

int main(int argc, const char *argv[])
{
    char fileName[256];
    std::cout << argv[0];
    for (int i = 1; i < argc; i++)
        std::cout << " " << argv[i];
    std::cout << std::endl;

#if 0
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_crtBreakAlloc = 4763;
#endif


    // here we let the tool quit instead of crashing
    if (argc < 2)
    {
        Options::showUsage(COUT_ERROR);
        return 1;
    }

    int startPos = 1;
    bool parserMode = false;
    if (endsWith(argv[startPos], ".visaasm") || endsWith(argv[startPos], ".isaasm"))
    {
        startPos++;
        parserMode = true;
    }
    else if (endsWith(argv[startPos], ".isa"))
    {
        startPos++;
    }

    // Note that we process options twice in offline mode (once here and once in createBuilder),
    // since we have to get some essential options such as platform and stepping
    Options opt;
    if (!opt.parseOptions(argc - startPos, &argv[startPos]))
    {
        return 1;
    }

    TARGET_PLATFORM platform = static_cast<TARGET_PLATFORM>(opt.getuInt32Option(vISA_PlatformSet));
    bool dumpCommonIsa = false;
    bool generateBinary = false;
    opt.getOption(vISA_GenerateISAASM, dumpCommonIsa);

    if (opt.getOption(vISA_isParseMode))
        parserMode = true;

    opt.getOption(vISA_GenerateBinary, generateBinary);
    if (platform == GENX_NONE && ((!dumpCommonIsa && !parserMode) || generateBinary))
    {
        std::cerr << "USAGE: must specify platform\n";
        Options::showUsage(COUT_ERROR);
        return EXIT_FAILURE;
    }

    if (opt.getOptionCstr(vISA_DecodeDbg))
    {
        const char* dbgName;
        opt.getOption(vISA_DecodeDbg, dbgName);
        decodeAndDumpDebugInfo((char*)dbgName, platform);
        return EXIT_SUCCESS;
    }

    // TODO: Will need to adjust the platform from option for parseBinary() and
    // parseText() if not specifying platform is allowed at this point.

    //
    // for debug print lex results to stdout (default)
    // for release open "lex.out" and redirect lex results
    //
    std::list<std::string> filesList;
    if (parserMode && opt.getOption(vISA_IsaasmNamesFileUsed))
    {
        const char* isaasmNamesFile;
        opt.getOption(vISA_ISAASMNamesFile, isaasmNamesFile);
        strcpy_s(fileName, 256, isaasmNamesFile);
        filesList.push_back(fileName);
    }
    else
    {
        //
        // allow input filename with any suffix or no suffix
        //
        std::string cmdLine = argv[1];
        if (!PrepareInput(cmdLine, filesList)) {
            std::cerr << "ERROR: Unable to open input file(s)." << std::endl;
            return 1;
        }
        std::string::size_type testNameEnd = cmdLine.find_last_of(".");
        std::string testName;
        if (testNameEnd != std::string::npos)
            testName = cmdLine.substr(0, testNameEnd);
        else
            testName = cmdLine;

        std::string::size_type numChars = cmdLine.copy(fileName, std::string::npos);
        fileName[numChars] = '\0';
    }

    // holds storage for file names that we automatically deduce in the loop
    // below; memory can be deallocated later
    std::vector<std::string> asmFileRoots;

    int err = VISA_SUCCESS;
    for (auto fName : filesList)
    {
        // If the file name is not set by the user, then use the same
        // file name (drop path)
        // we do not include an extension as other logic suffixes that later
        // depending on the desired
        //   e.g. foo/bar.visaasm ==> ./bar
        if (!opt.isOptionSetByUser(VISA_AsmFileName)) {
            auto extOff = fName.rfind('.');
            if (extOff == std::string::npos) {
                std::cerr << fName << ": cannot find file extension";
                return 1;
            }
            size_t fileStartOff = 0;
            for (int i = (int)extOff; i >= 0; i--) {
                if (fName[i] == '\\' || fName[i] == '/') {
                    fileStartOff = (size_t)i + 1;
                    break;
                }
            }
            auto baseRoot = fName.substr(fileStartOff, extOff - fileStartOff);
            asmFileRoots.emplace_back(baseRoot);
            opt.setOptionInternally(VISA_AsmFileName,
                asmFileRoots.back().c_str());
        }

        if (parserMode)
        {
            err = parseText(fName, argc - startPos, &argv[startPos], opt);
        }
        else
        {
            err = parseBinary(fName, argc - startPos, &argv[startPos], opt);
        }
        if (err)
            break;
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
    cout << numAllocations << "\t" << (totalAllocSize / 1024) << "\t" <<
        numMallocCalls << "\t" << (totalMallocSize / 1024) << "\t" << numMemManagers <<
        "\t" << maxArenaLength << endl;
#endif
#endif
    return err;
}
#endif

DLL_EXPORT void freeBlock(void* ptr)
{
    if (ptr != NULL)
    {
        free(ptr);
    }
}

void* allocCodeBlock(size_t sz)
{
    // allocate buffer for the Gen kernel binary.
    // for now just use malloc, but eventually runtime should provide this function
    return (char*) malloc(sz * sizeof(char));
}

#ifndef DLL_MODE

extern int CISAparse(CISA_IR_Builder* builder);

int parseText(std::string fileName, int argc, const char *argv[], Options &opt)
{
    int num_kernels = 0;
    std::string testName;

    std::list<std::string> file_names;
    bool isaasmNamesFileUsed =  false;

    opt.getOption(vISA_IsaasmNamesFileUsed, isaasmNamesFileUsed);
    if (isaasmNamesFileUsed) {
        std::ifstream os;
        os.open(fileName, std::ios::in);
        if (!os.is_open()) {
            std::cerr << fileName << ": could not open an isaasm names input file.\n";
            return EXIT_FAILURE;
        }

        std::string line;
        while (!os.eof()) {
            std::getline(os, line);
            if (line == "")
                continue;
            file_names.push_back(line);
            num_kernels++;
        }
        os.close();
    } else {
        num_kernels = 1;
        file_names.push_back(fileName);
    }

    //used to ignore duplicate file names
    std::map<std::string, bool> files_parsed;

    TARGET_PLATFORM platform = static_cast<TARGET_PLATFORM>(opt.getuInt32Option(vISA_PlatformSet));
    VISA_BUILDER_OPTION builderOption =
        (platform == GENX_NONE) ? VISA_BUILDER_VISA : VISA_BUILDER_BOTH;

    CISA_IR_Builder *cisa_builder = nullptr;

    auto err = CISA_IR_Builder::CreateBuilder(
        cisa_builder, vISA_ASM_READER, builderOption, platform, argc, argv);
    if (err)
        return EXIT_FAILURE;

    for (int i = 0; i < num_kernels; i++)
    {
        if (files_parsed.find(file_names.front()) != files_parsed.end())
        {
            file_names.pop_front();
            continue;
        }
        else
        {
            files_parsed[file_names.front()] = true;
        }

        auto vISAFileName = file_names.front();
        CISAin = fopen(vISAFileName.c_str(), "r");
        if (!CISAin)
        {
            std::cerr <<  vISAFileName << ": cannot open vISA assembly file\n";
            return EXIT_FAILURE;
        }

        std::string::size_type testNameEnd = vISAFileName.find_last_of('.');
        std::string::size_type testNameStart = vISAFileName.find_last_of('\\');

        if (testNameStart != std::string::npos)
            testNameStart++;
        else
            testNameStart = 0;

        if (testNameEnd != std::string::npos)
            testName = vISAFileName.substr(testNameStart, testNameEnd);
        else
            testName = vISAFileName;

        CISAdebug = 0;
        int fail = CISAparse(cisa_builder);
        fclose(CISAin);
        if (fail)
        {
            if (cisa_builder->HasParseError()) {
                std::cerr << cisa_builder->GetParseError() << "\n";
            } else {
                std::cerr << "error during parsing: CISAparse() returned " << fail << "\n";
            }
            return EXIT_FAILURE;
        }

        // If the input text lacks "OutputAsmPath" (and it should),
        // then we can override it with the implied name here.
        auto k = cisa_builder->get_kernel();
        if (k->getOutputAsmPath().empty()) {
            const char *outputPrefix = opt.getOptionCstr(VISA_AsmFileName);
            k->setOutputAsmPath(outputPrefix);
            cisa_builder->getOptions()->setOptionInternally(VISA_AsmFileName, outputPrefix);
        }

        file_names.pop_front();
    }

    std::string binFileName = testName + ".isa";

    bool outputCISABinaryName = false;
    opt.getOption(vISA_OutputvISABinaryName, outputCISABinaryName);
    if (outputCISABinaryName)
    {
        char const* cisaBinaryName;
        opt.getOption(vISA_GetvISABinaryName, cisaBinaryName);
        binFileName = cisaBinaryName;
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

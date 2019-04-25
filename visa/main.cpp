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

#include <iostream>
#include <fstream>

#include "BuildIR.h"
#include "visa_igc_common_header.h"
#include "Common_ISA_framework.h"
#include "VISAKernel.h"
#include "BuildCISAIR.h"
#include "FlowGraph.h"
#include "Optimizer.h"
#include "Option.h"
#include "GTGPU_RT_ASM_Interface.h"
#include "Common_ISA.h"
#include "BinaryCISAEmission.h"
#include "Timer.h"
#include "BinaryEncoding.h"
#include "JitterDataStruct.h"
#ifndef DLL_MODE
#include "EnumFiles.hpp"
#endif

using namespace std;

///
/// Reads byte code and calls the builder API as it does so.
///
extern bool readIsaBinaryNG(const char *buf, CISA_IR_Builder *builder,
                            vector<VISAKernel *> &kernels,
                            const char *kernelName, unsigned int majorVersion,
                            unsigned int minorVersion);

#ifndef DLL_MODE
void parseWrapper(const char *fileName, int argc, const char *argv[], Options &opt);
#endif

// default size of the physical reg pool mem manager in bytes
#define PHY_REG_MEM_SIZE   (16*1024)

// default size of the kernel mem manager in bytes
#define KERNEL_MEM_SIZE    (4*1024*1024)

#define JIT_SUCCESS                     0
#define JIT_INVALID_INPUT               1
#define JIT_CISA_ERROR                  3
#define JIT_INVALID_PLATFORM            5

// create a parser interface for lex and yacc.
// Note that for thread safety this should only be used in CISA.y
_THREAD CISA_IR_Builder * pCisaBuilder = NULL;

#ifndef DLL_MODE
void parse(const char *fileName, std::string testName, int argc, const char *argv[], Options &opt)
{
    vISA::Mem_Manager phyRegMem(PHY_REG_MEM_SIZE);
    vISA::PhyRegPool phyRegPool(phyRegMem, opt.getuInt32Option(vISA_TotalGRFNum));

    // read in common isa binary file
    int c;
    char *buf;
    FILE *commonISAInput = NULL;
    long file_size;
    unsigned byte_pos = 0;
    common_isa_header commonISAHeader;
    vISA::Mem_Manager globalMem(KERNEL_MEM_SIZE);

    // open common isa file
    if ((commonISAInput = fopen(fileName, "rb")) == NULL)
    {
        fprintf(stderr, "Cannot open file %s\n", fileName);
        exit(1);
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
    FILE* isafile = fopen(fileName, "rb");
    if (!isafile) {
        cerr << "Failure, unable to be opened." << endl;
        exit(EXIT_FAILURE);
    }

    /// Calculate file size.
    fseek(isafile, 0, SEEK_END);
    long isafilesize = ftell(isafile);
    rewind(isafile);

    /// Reading file into buffer.
    char* isafilebuf = (char*)mem.alloc(isafilesize);
    if (isafilesize != fread(isafilebuf, 1, isafilesize, isafile))
    {
        cerr << "Unable to read entire file into buffer." << endl;
        exit(EXIT_FAILURE);
    }
    fclose(isafile);

    TARGET_PLATFORM platform = getGenxPlatform();
    CM_VISA_BUILDER_OPTION builderOption =
        (platform == GENX_NONE) ? CM_CISA_BUILDER_CISA : CM_CISA_BUILDER_BOTH;
    CISA_IR_Builder* cisa_builder = NULL;
    VISA_WA_TABLE visaWaTable;

    CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_MEDIA, builderOption, platform, argc, argv, &visaWaTable, true);
    MUST_BE_TRUE(cisa_builder, "cisa_builder is NULL.");

    vector<VISAKernel*> kernels;
    readIsaBinaryNG(isafilebuf, cisa_builder, kernels, NULL, COMMON_ISA_MAJOR_VER, COMMON_ISA_MINOR_VER);
    std::string binFileName;

    if (cisa_builder->m_options.getOption(vISA_OutputvISABinaryName))
    {
        const char* cisaBinaryName = NULL;
        cisa_builder->m_options.getOption(vISA_GetvISABinaryName, cisaBinaryName);
        binFileName = cisaBinaryName;
    }

    // Pass testname to CISA IR Builder instance so that
    // it can use it to create filename for FC patch
    cisa_builder->setTestName(testName);

    int result = cisa_builder->Compile((char*)binFileName.c_str());
    CISA_IR_Builder::DestroyBuilder(cisa_builder);
    if (result != CM_SUCCESS)
    {
        exit(1);
    }
}
#endif

int JITCompileAllOptions(const char* kernelName,
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
    // This function becomes the new entry point even for JITCompile clients.
    if (kernelName == NULL || kernelIsa == NULL || strlen(kernelName) > COMMON_ISA_MAX_KERNEL_NAME_LEN)
    {
        return JIT_INVALID_INPUT;
    }
    // This must be done before processing the options,
    // as some options depend on the platform
    if (SetPlatform(platform) != 0)
    {
        return JIT_INVALID_PLATFORM;
    }

    genBinary = NULL;
    genBinarySize = 0;

    char* isafilebuf = (char*)kernelIsa;
    CISA_IR_Builder* cisa_builder = NULL;
    // HW mode: default: GEN path; if dump/verify: Both path
    CM_VISA_BUILDER_OPTION builderOption = CM_CISA_BUILDER_GEN;

    VISA_WA_TABLE visaWaTable;

    CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_MEDIA, builderOption, getGenxPlatform(), numArgs, args, &visaWaTable, true);
    cisa_builder->setGtpinInit(gtpin_init);

    if (!cisa_builder)
    {
        return JIT_CISA_ERROR;
    }

    vector<VISAKernel*> kernels;
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
    void* buf;
    unsigned int bufSize;
    kernel->GetGenxDebugInfo(tempJitInfo->genDebugInfo, tempJitInfo->genDebugInfoSize, buf, bufSize);

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

DLL_EXPORT void getJITVersion(unsigned int& majorV, unsigned int& minorV )
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

int main( int argc, const char *argv[] )
{
    char fileName[256];
    std::string testName = "F5";
    cout << argv[0];
    for (int i = 1; i < argc; i++) cout << " " << argv[i];
    cout << endl;

#if 0
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
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
    if (opt.getOptionCstr(vISA_DecodeDbg))
    {
        const char* dbgName;
        opt.getOption(vISA_DecodeDbg, dbgName);
        decodeAndDumpDebugInfo((char*)dbgName);
        exit(0);
    }

    bool platformIsSet = false;
    bool dumpCommonIsa = false;
    bool generateBinary = false;
    opt.getOption(vISA_PlatformIsSet, platformIsSet);
    opt.getOption(vISA_GenerateISAASM, dumpCommonIsa);

    if (opt.getOption(vISA_isParseMode))
        parserMode = true;

    opt.getOption(vISA_GenerateBinary, generateBinary);
    if (!platformIsSet && ((!dumpCommonIsa && !parserMode) || generateBinary))
    {
        std::cout << "USAGE: must specify platform" << std::endl;
        Options::showUsage(COUT_ERROR);
        return 1;
    }

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
            std::cout << "ERROR: Unable to open input file(s)." << std::endl;
            return 1;
        }
        std::string::size_type testNameEnd = cmdLine.find_last_of(".");
        if (testNameEnd != std::string::npos)
            testName = cmdLine.substr(0, testNameEnd);
        else
            testName = cmdLine;

        std::string::size_type numChars = cmdLine.copy(fileName, std::string::npos);
        fileName[numChars] = '\0';
    }

    for (auto fName : filesList)
    {
        if (parserMode)
        {
            parseWrapper(fName.c_str(), argc - startPos, &argv[startPos], opt);
        }
        else
        {
            parse(fName.c_str(), testName, argc - startPos, &argv[startPos], opt);
        }
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
    return 0;
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

extern int CISAparse();

void parseWrapper(const char *fileName, int argc, const char *argv[], Options &opt)
{
    vISA::Mem_Manager cisaBinaryMem(4194304);
    vISA::Mem_Manager phyRegMem(PHY_REG_MEM_SIZE);
    vISA::PhyRegPool phyRegPool(phyRegMem, opt.getuInt32Option(vISA_TotalGRFNum));
    int num_kernels = 0;
    std::string testName;

    std::list<std::string> file_names;
    bool isaasmNamesFileUsed =  false;

    opt.getOption(vISA_IsaasmNamesFileUsed, isaasmNamesFileUsed);
    if (isaasmNamesFileUsed) {
        std::ifstream os;
        os.open(fileName, std::ios::in);
        if (!os.is_open()) {
            printf("Could not open an isaasm names input file.\n");
            exit(1);
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
    }
    else {
        num_kernels = 0;
        file_names.push_back(fileName);
        num_kernels++;
    }

    //used to ignore duplicate file names
    std::map<std::string, bool> files_parsed;

    // isaasm path is vISA path only; input: .isaasm, output: .isa
    TARGET_PLATFORM platform = getGenxPlatform();
    CM_VISA_BUILDER_OPTION builderOption =
        (platform == GENX_NONE) ? CM_CISA_BUILDER_CISA : CM_CISA_BUILDER_BOTH;
    CISA_IR_Builder *cisa_builder = NULL;
    VISA_WA_TABLE visaWaTable;

    CISA_IR_Builder::CreateBuilder(cisa_builder, vISA_PARSER, builderOption, getGenxPlatform(), argc, argv, &visaWaTable, true);

    for(int i = 0; i < num_kernels; i++)
    {

        if(files_parsed.find(file_names.front()) != files_parsed.end())
        {
            file_names.pop_front();
            continue;
        }
        else
        {
            files_parsed[file_names.front()] = true;
        }
        //
        // parser takes pBuilder to create G4_INST inst list
        //
        if (!cisa_builder->openCISAParsingFile( file_names.front().c_str(),"r"))
        {
            printf("ERROR: Can not open file %s!\n", file_names.front().c_str());
            exit(1);                        // make the tool quit in error case
        }

        std::string::size_type testNameEnd = file_names.front().find_last_of(".");
        std::string::size_type testNameStart = file_names.front().find_last_of("\\");

        if (testNameStart != std::string::npos)
            testNameStart++;
        else
            testNameStart = 0;

        if (testNameEnd != std::string::npos)
            testName = file_names.front().substr(testNameStart, testNameEnd);
        else
            testName = file_names.front();

        CISAdebug = 0;
        int fail;
        // remove new programming rule option

        fail = CISAparse();
        cisa_builder->closeCISAParsingFile();
        if (fail)
        {
            printf("Error during parsing: CISAparse() exited with exit code %d\n", fail);
            exit(1);
        }

        file_names.pop_front();
    }

    std::string binFileName = testName;
    binFileName.append(".isa");

    bool outputCISABinaryName = false;
    opt.getOption(vISA_OutputvISABinaryName, outputCISABinaryName);
    if(outputCISABinaryName)
    {
        char const *cisaBinaryName;
        opt.getOption(vISA_GetvISABinaryName, cisaBinaryName);
        binFileName = cisaBinaryName;
    }

    cisa_builder->Compile((char *)binFileName.c_str());
    CISA_IR_Builder::DestroyBuilder(cisa_builder);
}
#endif

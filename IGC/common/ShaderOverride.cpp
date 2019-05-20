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
#include "../../../visa/iga/IGALibrary/api/igad.h"
#include <iostream>
#include <fstream>
#include <iostream>
#include "Compiler/CodeGenPublic.h"
#include "visaBuilder_interface.h"
#include "common/secure_mem.h"
#include "common/secure_string.h"
#if defined(WIN32)
#include "WinDef.h"
#include "Windows.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
__inline HINSTANCE GetModuleHINSTANCE() { return (HINSTANCE)&__ImageBase; }

__inline HMODULE
LoadDependency(const TCHAR *dependencyFileName)
{
    TCHAR dllPath[MAX_PATH];

    GetModuleFileName(GetModuleHINSTANCE(), dllPath, MAX_PATH);

    std::basic_string<TCHAR> dllStringPath = std::basic_string<TCHAR>(dllPath);
    auto lastPostion = dllStringPath.find_last_of('\\');
    dllStringPath = dllStringPath.substr(0, lastPostion + 1);
    dllStringPath.append(std::basic_string<TCHAR>(dependencyFileName));

    return LoadLibraryEx(dllStringPath.c_str(), NULL, 0);
}
#else
#include <dlfcn.h>     // dlopen, dlsym, dlclose
#define HMODULE void *
static void * GetProcAddress(
    void * hModule,
    const char * pProcName)
{
    return dlsym(hModule, pProcName);
}
#endif

#if defined(WIN32)
#define CDECLATTRIBUTE __cdecl
#elif __GNUC__
#ifdef __x86_64__
#define CDECLATTRIBUTE
#else
#define CDECLATTRIBUTE                 __attribute__((__cdecl__))
#endif
#endif



static void* loadBinFile(
    const std::string& fileName,
    int& binSize)
{
    FILE* fp;
    fp = fopen(fileName.c_str(), "rb");
    if(fp == nullptr)
    {
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    binSize = int_cast<int>(ftell(fp));
    rewind(fp);

    void* buf = malloc(sizeof(char)* binSize);
    if(buf == nullptr)
    {
        fclose(fp);
        return nullptr;
    }

    size_t nr = fread(buf, 1, binSize, fp);
    if(nr != binSize)
    {
        fclose(fp);
        free(buf);
        return nullptr;
    }

    fclose(fp);
    return buf;
}

iga_gen_t GetIGAPlatform(const IGC::CPlatform* platform)
{
    switch(platform->GetPlatformFamily())
    {
    case IGFX_GEN8_CORE:
        if(platform->getPlatformInfo().eProductFamily == IGFX_CHERRYVIEW)
        {
            return IGA_GEN8lp;
        }
        else
        {
            return IGA_GEN8;
        }
    case IGFX_GEN9_CORE:
    case IGFX_GENNEXT_CORE:
        if(platform->getPlatformInfo().eProductFamily == IGFX_BROXTON ||
            platform->getPlatformInfo().eProductFamily == IGFX_GEMINILAKE)
        {
            return IGA_GEN9lp;
        }
        else
        {
            return IGA_GEN9;
        }
    default:
        assert(0 && "unsupported platform");
        break;
    }
    return IGA_GEN9;
}

void appendToShaderOverrideLogFile(std::string &binFileName, const char * message)
{
    auto overridePath = std::string(IGC::Debug::GetShaderOverridePath());
    overridePath.append("OverrideLog.txt");
    std::cout << std::endl << message << binFileName.c_str() << std::endl;
    std::ofstream os(overridePath.c_str(), std::ios::app);
    if(os.is_open())
    {
        os << message << binFileName.c_str() << std::endl;
    }
    os.close();
}

void overrideShaderIGA(const IGC::CodeGenContext* context, void *& genxbin, int & binSize, std::string &binFileName, bool &binOverride)
{
    std::ifstream is(binFileName.c_str(), std::ios::in);
    std::string asmContext;
    void * overrideBinary = nullptr;
    uint32_t overrideBinarySize = 0;

    if(is.is_open())
    {
        asmContext.append(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
        is.close();
    }
    else
    {
        binOverride = false;
        return;
    }

    HMODULE hModule = NULL;
    pIGACreateContext           fCreateContext;
    pIGAGetErrors               fIGAGetErrors;
    pIGAGetWarnings             fIGAGetWarnings;
    pIGADiagnosticGetMessage    fIGADiagnosticGetMessage;
    pIGAAssemble                fIGAAssemble;
    pIGAReleaseContext          fIGAReleaseContext;

#if defined(WIN32)
    TCHAR *igaName = nullptr;
#ifdef _WIN64
    char igaName64[] = "iga64.dll";
    igaName = (TCHAR*)igaName64;
#else
    char igaName32[] = "iga32.dll";
    igaName = (TCHAR*)igaName32;
#endif    //_WIN64

    hModule = LoadDependency(igaName);

#else
    // linux
#ifdef __x86_64__
    char igaName[] = "libiga64.so";
#else
    char igaName[] = "libiga32.so";
#endif

#ifdef SANITIZER_BUILD
    hModule = dlopen(igaName, RTLD_LAZY);
#else
    hModule = dlopen(igaName, RTLD_LAZY | RTLD_DEEPBIND);
#endif

#endif

    if (hModule == nullptr)
    {
        binOverride = false;
        appendToShaderOverrideLogFile(binFileName, "OVERRIDE FAILED DUE TO MISSING IGA LIBRARY: ");
        return;
    }

    fCreateContext = (pIGACreateContext)GetProcAddress(hModule, IGA_CREATE_CONTEXT_STR);
    fIGAGetErrors = (pIGAGetErrors)GetProcAddress(hModule, IGA_GET_ERRORS_STR);
    fIGAGetWarnings = (pIGAGetWarnings)GetProcAddress(hModule, IGA_GET_WARNINGS_STR);
    fIGADiagnosticGetMessage = (pIGADiagnosticGetMessage)GetProcAddress(hModule, IGA_DIAGNOSTIC_GET_MESSAGE_STR);
    fIGAAssemble = (pIGAAssemble)GetProcAddress(hModule, IGA_ASSEMBLE_STR);
    fIGAReleaseContext = (pIGAReleaseContext)GetProcAddress(hModule, IGA_RELEASE_CONTEXT_STR);

    if (fCreateContext == nullptr ||
        fCreateContext == nullptr ||
        fIGAGetErrors == nullptr ||
        fIGAGetWarnings == nullptr ||
        fIGADiagnosticGetMessage == nullptr ||
        fIGAAssemble == nullptr ||
        fIGAReleaseContext == nullptr)
    {
        binOverride = false;
        appendToShaderOverrideLogFile(binFileName, "OVERRIDE FAILED DUE TO IGA LOAD ERRORS: ");
        return;
    }

    iga_context_t ctx;

    iga_context_options_t ctxOpts = IGA_CONTEXT_OPTIONS_INIT(GetIGAPlatform(&(context->platform)));
    if(fCreateContext(&ctxOpts, &ctx) != IGA_SUCCESS) {
        binOverride = false;
        appendToShaderOverrideLogFile(binFileName, "OVERRIDE FAILED DUE TO IGA CONTEXT CREATION FAILURE: ");
        return;
    }


    iga_assemble_options_t asmOpts = IGA_ASSEMBLE_OPTIONS_INIT();

    iga_status_t assembleRes = fIGAAssemble(ctx, &asmOpts, asmContext.c_str(), &overrideBinary, &overrideBinarySize);
    if (assembleRes != IGA_SUCCESS) {
        binOverride = false;

        std::stringstream msgss;
        msgss <<  "OVERRIDE FAILED DUE TO SHADER ASSEMBLY FAILURE (" << assembleRes << ") : ";
        appendToShaderOverrideLogFile(binFileName, msgss.str().c_str());

        const iga_diagnostic_t * errors;
        uint32_t count;
        if (fIGAGetErrors(ctx, &errors, &count) == IGA_SUCCESS)
        {
            for (uint32_t i = 0; i < count; i++)
            {
                std::stringstream errorss;
                errorss <<  "error (" << i+1 << "/"<< count << "): Line " << errors[i].line <<": Col: " << errors[i].column  << " " << errors[i].message << " ";
                appendToShaderOverrideLogFile(binFileName, errorss.str().c_str());
            }
        }
        if (fIGAGetWarnings(ctx, &errors, &count) == IGA_SUCCESS)
        {
            for (uint32_t i = 0; i < count; i++)
            {
                std::stringstream errorss;
                errorss << "warning (" << i + 1 << "/" << count << "): Line " << errors[i].line << ": Col: " << errors[i].column << " " << errors[i].message << " ";
                appendToShaderOverrideLogFile(binFileName, errorss.str().c_str());
            }
        }

        return;
    }


    freeBlock(genxbin);
    //when we releaseContext memory returned is freed
    genxbin = malloc(overrideBinarySize);
    if(genxbin)
    {
        memcpy_s(genxbin, overrideBinarySize, overrideBinary, overrideBinarySize);
        binSize = overrideBinarySize;
        binOverride = true;

        appendToShaderOverrideLogFile(binFileName, "OVERRIDEN: ");
    }

    fIGAReleaseContext(ctx);

}

void overrideShaderBinary(void *& genxbin, int & binSize, std::string &binFileName, bool &binOverride)
{
    void* loadBin = nullptr;
    int loadBinSize = 0;

    loadBin = loadBinFile(binFileName, loadBinSize);
    if(loadBin != nullptr)
    {
        freeBlock(genxbin);
        genxbin = loadBin;
        binSize = loadBinSize;
        binOverride = true;
        appendToShaderOverrideLogFile(binFileName, "OVERRIDEN: ");
    }
}

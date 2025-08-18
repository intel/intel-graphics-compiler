/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "visa/iga/IGALibrary/api/igad.h"
#include <iostream>
#include <fstream>
#include <iostream>
#include <chrono>
#include "Compiler/CodeGenPublic.h"
#include "visaBuilder_interface.h"
#include "common/secure_mem.h"
#include "common/secure_string.h"
#include "Probe/Assertion.h"
#if defined(WIN32)
#include "WinDef.h"
#include "Windows.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
__inline HINSTANCE GetModuleHINSTANCE() { return (HINSTANCE)&__ImageBase; }

__inline HMODULE LoadDependency(const TCHAR *dependencyFileName) {
  TCHAR dllPath[MAX_PATH];

  GetModuleFileName(GetModuleHINSTANCE(), dllPath, MAX_PATH);

  std::basic_string<TCHAR> dllStringPath = std::basic_string<TCHAR>(dllPath);
  auto lastPostion = dllStringPath.find_last_of('\\');
  dllStringPath = dllStringPath.substr(0, lastPostion + 1);
  dllStringPath.append(std::basic_string<TCHAR>(dependencyFileName));

  return LoadLibraryEx(dllStringPath.c_str(), NULL, 0);
}
#else
#include <dlfcn.h> // dlopen, dlsym, dlclose
#define HMODULE void *
static void *GetProcAddress(void *hModule, const char *pProcName) { return dlsym(hModule, pProcName); }
#endif

#if defined(WIN32)
#define CDECLATTRIBUTE __cdecl
#elif __GNUC__
#if defined(__x86_64__) || defined(__ARM_ARCH)
#define CDECLATTRIBUTE
#else
#define CDECLATTRIBUTE __attribute__((__cdecl__))
#endif
#endif

static void *loadBinFile(const std::string &fileName, int &binSize) {
  FILE *fp;
  fp = fopen(fileName.c_str(), "rb");
  if (fp == nullptr) {
    return nullptr;
  }

  fseek(fp, 0, SEEK_END);
  binSize = int_cast<int>(ftell(fp));
  if (binSize <= 0) {
    fclose(fp);
    return nullptr;
  }
  rewind(fp);

  void *buf = malloc(sizeof(char) * binSize);
  if (buf == nullptr) {
    fclose(fp);
    return nullptr;
  }

  size_t nr = fread(buf, 1, binSize, fp);
  if (nr != binSize) {
    fclose(fp);
    free(buf);
    return nullptr;
  }

  fclose(fp);
  return buf;
}

iga_gen_t GetIGAPlatform(PLATFORM const &platform) {
  auto &ProductFamily = platform.eProductFamily;
  switch (platform.eRenderCoreFamily) {
  case IGFX_GEN8_CORE:
    if (ProductFamily == IGFX_CHERRYVIEW) {
      return IGA_GEN8lp;
    } else {
      return IGA_GEN8;
    }
  case IGFX_GEN9_CORE:
  case IGFX_GENNEXT_CORE:
    if (ProductFamily == IGFX_BROXTON || ProductFamily == IGFX_GEMINILAKE) {
      return IGA_GEN9lp;
    } else {
      return IGA_GEN9;
    }
  case IGFX_GEN10_CORE:
    return IGA_GEN10;
  case IGFX_GEN11_CORE:
    return IGA_GEN11;
  case IGFX_GEN12_CORE:
  case IGFX_GEN12LP_CORE:
  case IGFX_XE_HP_CORE:
  case IGFX_XE_HPG_CORE:
  case IGFX_XE_HPC_CORE:
    if (ProductFamily == IGFX_TIGERLAKE_LP || ProductFamily == IGFX_DG1 || ProductFamily == IGFX_ROCKETLAKE ||
        ProductFamily == IGFX_ALDERLAKE_S || ProductFamily == IGFX_ALDERLAKE_P || ProductFamily == IGFX_ALDERLAKE_N
    ) {
      return IGA_GEN12p1;
    } else if (ProductFamily == IGFX_XE_HP_SDV) {
      return IGA_XE_HP;
    } else if (ProductFamily == IGFX_DG2 || ProductFamily == IGFX_METEORLAKE) {
      return IGA_XE_HPG;
    } else if (ProductFamily == IGFX_ARROWLAKE) {
      return IGA_XE_HPG;
    } else if (ProductFamily == IGFX_PVC) {
      return IGA_XE_HPC;
    }
    IGC_ASSERT_MESSAGE(0, "unsupported platform");
    break;
  case IGFX_XE2_HPG_CORE:
    if (ProductFamily == IGFX_BMG || ProductFamily == IGFX_LUNARLAKE) {
      return IGA_XE2;
    }
    IGC_ASSERT_MESSAGE(0, "unsupported platform");
    break;
  case IGFX_XE3_CORE:
    return IGA_XE3;
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported platform");
    break;
  }
  return IGA_GEN9;
}

void appendToShaderOverrideLogFile(std::string const &binFileName, const char *message) {
  auto overridePath = std::string(IGC::Debug::GetShaderOverridePath());
  overridePath.append("OverrideLog.txt");
  printf("\n%s %s\n", message, binFileName.c_str());
  std::ofstream os(overridePath.c_str(), std::ios::app);
  if (os.is_open()) {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto c_time = ctime(&now_time);
    c_time[strlen(c_time) - 1] = '\t';
    os << c_time << message << binFileName.c_str() << std::endl;
  }
  os.close();
}

void overrideShaderIGA(PLATFORM const &platform, void *&genxbin, int &binSize, std::string const &binFileName,
                       bool &binOverride) {
  std::ifstream is(binFileName.c_str(), std::ios::in);
  std::string asmContext;
  void *overrideBinary = nullptr;
  uint32_t overrideBinarySize = 0;

  if (is.is_open()) {
    asmContext.append(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
    is.close();
  } else {
    binOverride = false;
    return;
  }

  HMODULE hModule = NULL;
  pIGACreateContext fCreateContext;
  pIGAGetErrors fIGAGetErrors;
  pIGAGetWarnings fIGAGetWarnings;
  pIGADiagnosticGetMessage fIGADiagnosticGetMessage;
  pIGAAssemble fIGAAssemble;
  pIGAReleaseContext fIGAReleaseContext;

#if defined(WIN32)
  TCHAR *igaName = nullptr;
#ifdef _WIN64
  char igaName64[] = "iga64.dll";
  igaName = (TCHAR *)igaName64;
#else
  char igaName32[] = "iga32.dll";
  igaName = (TCHAR *)igaName32;
#endif //_WIN64

  hModule = LoadDependency(igaName);

#else
  // linux
#ifdef __x86_64__
  char igaName[] = "libiga64.so";
#else
  char igaName[] = "libiga32.so";
#endif

  hModule = dlopen(igaName, RTLD_LAZY);
#endif

  if (hModule == nullptr) {
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

  if (fCreateContext == nullptr || fIGAGetErrors == nullptr || fIGAGetWarnings == nullptr ||
      fIGADiagnosticGetMessage == nullptr || fIGAAssemble == nullptr || fIGAReleaseContext == nullptr) {
    binOverride = false;
    appendToShaderOverrideLogFile(binFileName, "OVERRIDE FAILED DUE TO IGA LOAD ERRORS: ");
    return;
  }

  iga_context_t ctx;

  iga_context_options_t ctxOpts = IGA_CONTEXT_OPTIONS_INIT(GetIGAPlatform(platform));
  if (fCreateContext(&ctxOpts, &ctx) != IGA_SUCCESS) {
    binOverride = false;
    appendToShaderOverrideLogFile(binFileName, "OVERRIDE FAILED DUE TO IGA CONTEXT CREATION FAILURE: ");
    return;
  }

  iga_assemble_options_t asmOpts = IGA_ASSEMBLE_OPTIONS_INIT();

  iga_status_t assembleRes = fIGAAssemble(ctx, &asmOpts, asmContext.c_str(), &overrideBinary, &overrideBinarySize);
  if (assembleRes != IGA_SUCCESS) {
    binOverride = false;

    std::stringstream msgss;
    msgss << "OVERRIDE FAILED DUE TO SHADER ASSEMBLY FAILURE (" << assembleRes << ") : ";
    appendToShaderOverrideLogFile(binFileName, msgss.str().c_str());

    const iga_diagnostic_t *errors;
    uint32_t count;
    if (fIGAGetErrors(ctx, &errors, &count) == IGA_SUCCESS) {
      for (uint32_t i = 0; i < count; i++) {
        std::stringstream errorss;
        errorss << "error (" << i + 1 << "/" << count << "): Line " << errors[i].line << ": Col: " << errors[i].column
                << " " << errors[i].message << " ";
        appendToShaderOverrideLogFile(binFileName, errorss.str().c_str());
      }
    }
    if (fIGAGetWarnings(ctx, &errors, &count) == IGA_SUCCESS) {
      for (uint32_t i = 0; i < count; i++) {
        std::stringstream errorss;
        errorss << "warning (" << i + 1 << "/" << count << "): Line " << errors[i].line << ": Col: " << errors[i].column
                << " " << errors[i].message << " ";
        appendToShaderOverrideLogFile(binFileName, errorss.str().c_str());
      }
    }

    return;
  }

  freeBlock(genxbin);
  // when we releaseContext memory returned is freed
  genxbin = malloc(overrideBinarySize);
  if (genxbin) {
    memcpy_s(genxbin, overrideBinarySize, overrideBinary, overrideBinarySize);
    binSize = overrideBinarySize;
    binOverride = true;

    appendToShaderOverrideLogFile(binFileName, "OVERRIDEN: ");
  }

  fIGAReleaseContext(ctx);
}

void overrideShaderBinary(void *&genxbin, int &binSize, std::string const &binFileName, bool &binOverride) {
  void *loadBin = nullptr;
  int loadBinSize = 0;

  loadBin = loadBinFile(binFileName, loadBinSize);
  if (loadBin != nullptr) {
    freeBlock(genxbin);
    genxbin = loadBin;
    binSize = loadBinSize;
    binOverride = true;
    appendToShaderOverrideLogFile(binFileName, "OVERRIDEN: ");
  }
}

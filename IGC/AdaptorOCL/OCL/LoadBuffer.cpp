/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorOCL/OCL/LoadBuffer.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/MemoryBuffer.h"
#include "common/LLVMWarningsPop.hpp"

#include <cstdlib>

using namespace llvm;
#ifdef LLVM_ON_UNIX

#include "Probe/Assertion.h"
#include <dlfcn.h>
#include <stdio.h>

void *getIgcHandle() {
  Dl_info dlInfo;
  // Find path to IGC dynamic lib.
  if (dladdr((void *)getIgcHandle, &dlInfo) != 0) {
    void *h = dlopen(dlInfo.dli_fname, RTLD_LAZY);
    return h;
  }
  return NULL;
}

MemoryBuffer *llvm::LoadBufferFromResource(const char *pResName, const char *pResType) {
  // Symbol Name is <type>_<number>
  char name[73];      // 64 + 9 for prefix
  char size_name[78]; // 64 + 9 for prefix + 5 for suffix
  void *module = RTLD_DEFAULT;
  void *sizeSymbol;
  void *symbol = NULL;
  MemoryBuffer *buffer = NULL;

  snprintf(name, sizeof(name), "_igc_bif_%s_%s", pResType, &pResName[1]);
  snprintf(size_name, sizeof(size_name), "_igc_bif_%s_%s_size", pResType, &pResName[1]);

  // Test if RTLD_DEFAULT works for the current module. If not, try to load the IGC module explicitly.
  sizeSymbol = dlsym(module, size_name);
  if (!sizeSymbol) {
    module = getIgcHandle();
    if (module == NULL) {
      IGC_ASSERT_EXIT_MESSAGE(0, "Failed  to open IGC\n");
      return NULL;
    }
    sizeSymbol = dlsym(module, size_name);
  }

  if (sizeSymbol) {
    symbol = dlsym(module, name);
  }

  if (symbol) {
    // Create a copy of the buffer for the caller. This copy is managed
    buffer = MemoryBuffer::getMemBufferCopy(StringRef((char *)symbol, *(uint32_t *)sizeSymbol)).release();
  }

  // Drop the reference taken by getIgcHandle. The module stays mapped, as it is the one
  // currently executing, and the buffer above is already a copy.
  if (module != RTLD_DEFAULT) {
    dlclose(module);
  }

  if (!buffer) {
    IGC_ASSERT_EXIT_MESSAGE(0, "LoadBufferFromResource: symbol not found [%s]\n", sizeSymbol ? name : size_name);
    return NULL;
  }
  return buffer;
}

#endif

#ifdef WIN32
#include <Windows.h>
// Windows.h defines MemoryFence as _mm_mfence, but this conflicts with llvm::sys::MemoryFence
#undef MemoryFence

HRSRC FindResourceCompat(HMODULE hMod, const char *pResName, const char *pResType) {
  std::wstring resNameW;
  std::wstring resTypeW;
  if (pResName != nullptr) {
    resNameW.assign(pResName, pResName + strlen(pResName));
  }
  if (pResType != nullptr) {
    resTypeW.assign(pResType, pResType + strlen(pResType));
  }

  return FindResourceExW(hMod, pResType ? resTypeW.c_str() : nullptr, pResName ? resNameW.c_str() : nullptr,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
}

MemoryBuffer *llvm::LoadBufferFromResource(const char *pResName, const char *pResType) {
  HMODULE hMod = NULL;

  // Get the handle to the current module
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     (LPCSTR)llvm::LoadBufferFromResource, &hMod);
  if (hMod == NULL) {
    return NULL;
  }

  // Locate the resource
  HRSRC hRes = FindResourceCompat(hMod, pResName, pResType);

  if (hRes == NULL) {
    return NULL;
  }

  // Load the resource
  HGLOBAL hBytes = LoadResource(hMod, hRes);
  if (hBytes == NULL) {
    return NULL;
  }

  // Get the base address to the resource. This call doesn't really lock it
  const char *pData = (char *)LockResource(hBytes);
  if (pData == NULL) {
    return NULL;
  }

  // Get the buffer size
  size_t dResSize = SizeofResource(hMod, hRes);
  if (dResSize == 0) {
    return NULL;
  }

  // this memory never needs to be freeded since it is not dynamically allocated
  return MemoryBuffer::getMemBuffer(StringRef(pData, dResSize), "", false).release();
}
#endif // LLVM_ON_WIN32

MemoryBuffer *llvm::LoadBufferFromFile(const std::string &FileName) {
  std::string FullFileName(FileName);

  // If buffer is not on the current working directory try to load it from
  // a predetermined path
  char *pEnv = getenv("INTEL_OPENCL_GPU_DIR");
  FullFileName.insert(0, "Inc\\");
  if (pEnv) {
    FullFileName.insert(0, "\\");
    FullFileName.insert(0, pEnv);
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr = MemoryBuffer::getFileOrSTDIN(FullFileName);

  return FileOrErr.get().release();
}

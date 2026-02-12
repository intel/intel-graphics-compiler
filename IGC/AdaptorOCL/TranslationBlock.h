/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "CommonMacros.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include <stdint.h>
#include <stddef.h>
#include <string>

#if defined(_WIN32)
// INSIDE_PLUGIN must be defined in the pre-processor definitions of the
// CTranslationBlock DLL project
#define TRANSLATION_BLOCK_CALLING_CONV __cdecl
#ifndef TRANSLATION_BLOCK_API
#ifdef INSIDE_PLUGIN
#define TRANSLATION_BLOCK_API __declspec(dllexport)
#else
#define TRANSLATION_BLOCK_API __declspec(dllimport)
#endif
#endif
#else
#define TRANSLATION_BLOCK_CALLING_CONV
#ifndef TRANSLATION_BLOCK_API
#define TRANSLATION_BLOCK_API __attribute__((visibility("default")))
#endif
#endif

namespace TC {
static const uint32_t STB_VERSION = 1006UL;
static const uint32_t STB_MAX_ERROR_STRING_SIZE = 1024UL;

// Forward prototyping
struct STB_RegisterArgs;
struct STB_CreateArgs;
class CTranslationBlock;

extern "C" TRANSLATION_BLOCK_API void TRANSLATION_BLOCK_CALLING_CONV Register(STB_RegisterArgs *pRegisterArgs);
extern "C" TRANSLATION_BLOCK_API CTranslationBlock *TRANSLATION_BLOCK_CALLING_CONV Create(STB_CreateArgs *pCreateArgs);
extern "C" TRANSLATION_BLOCK_API void TRANSLATION_BLOCK_CALLING_CONV Delete(CTranslationBlock *pBlock);

typedef void(TRANSLATION_BLOCK_CALLING_CONV *PFNREGISTER)(STB_RegisterArgs *pRegisterArgs);
typedef CTranslationBlock *(TRANSLATION_BLOCK_CALLING_CONV *PFNCREATE)(STB_CreateArgs *pCreateArgs);
typedef void(TRANSLATION_BLOCK_CALLING_CONV *PFNDELETE)(CTranslationBlock *pBlock);

#undef TRANSLATION_BLOCK_CALLING_CONV

/******************************************************************************\

Enumeration:
    TB_DATA_FORMAT

Description:
    Possible i/o formats for the translation classes

\******************************************************************************/
enum TB_DATA_FORMAT {
  TB_DATA_FORMAT_UNKNOWN,
  TB_DATA_FORMAT_OCL_TEXT,
  TB_DATA_FORMAT_OCL_BINARY,
  TB_DATA_FORMAT_LLVM_TEXT,
  TB_DATA_FORMAT_LLVM_BINARY,
  TB_DATA_FORMAT_GHAL_TEXT,
  TB_DATA_FORMAT_GHAL_BINARY,
  TB_DATA_FORMAT_DEVICE_TEXT,
  TB_DATA_FORMAT_DEVICE_BINARY,
  TB_DATA_FORMAT_LLVM_ARCHIVE,
  TB_DATA_FORMAT_ELF,
  TB_DATA_FORMAT_RS_LLVM_BINARY,
  TB_DATA_FORMAT_RS_INFO,
  TB_DATA_FORMAT_SPIR_V,
  TB_DATA_FORMAT_COHERENT_DEVICE_BINARY,
  TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY,
  NUM_TB_DATA_FORMATS
};

/******************************************************************************\

Structure:
    STB_TranslationCode

Description:
    Structure used to describe the requested translation type

\******************************************************************************/
union STB_TranslationCode {
  struct {
    TB_DATA_FORMAT Input : 16;
    TB_DATA_FORMAT Output : 16;
  } Type;

  uint32_t Code;
};

/******************************************************************************\

Structure:
    STB_CreateArgs

Description:
    Structure used to store arguments used to pass data to the Create function

\******************************************************************************/
struct STB_CreateArgs {
  // INFO keep two first fields in this order ! for version ICBE 1003 compatibility
  STB_TranslationCode TranslationCode;
  void *pCreateData;

  STB_CreateArgs() {
    TranslationCode.Code = 0;
    pCreateData = NULL;
  }
};

/******************************************************************************\

Structure:
    STB_RegisterArgs

Description:
    Structure containing a pointer to an array of supported translation codes
    and a variable informing us of the size of the translation code array.

    The calling function is responsible for deleting the memory allocated
    for the translation code array.

Note:
    Version is contained in this header

\******************************************************************************/
struct STB_RegisterArgs {
  uint32_t Version;
  uint32_t NumTranslationCodes;
  STB_TranslationCode *pTranslationCodes;

  STB_RegisterArgs() {
    Version = STB_VERSION;
    NumTranslationCodes = 0;
    pTranslationCodes = NULL;
  }
};

/******************************************************************************\

Structure:
    STB_TranslateInputArgs

Description:
    Structure used to pass input variables to the translation block

\******************************************************************************/
struct STB_TranslateInputArgs {
  char *pInput;                 // data to be translated
  uint32_t InputSize;           // size of data to be translated
  const char *pOptions;         // list of build/compile options
  uint32_t OptionsSize;         // size of options list
  const char *pInternalOptions; // list of build/compile options
  uint32_t InternalOptionsSize; // size of options list
  void *pTracingOptions;        // instrumentation options
  uint32_t TracingOptionsCount; // number of instrumentation options
  void *GTPinInput;             // input structure for GTPin requests
  bool CompileTimeStatisticsEnable;
  const uint32_t *pSpecConstantsIds;    // user-defined spec constants ids
  const uint64_t *pSpecConstantsValues; // spec constants values to be translated
  uint32_t SpecConstantsSize;           // number of specialization constants
  const char **pVISAAsmToLinkArray;     // array of additional visa assembly in text format
                                        // that should be linked with compiled module.
                                        // Used e.g. for "sginvoke" functionality.
  uint32_t NumVISAAsmsToLink;
  const char **pDirectCallFunctions;
  uint32_t NumDirectCallFunctions;

  STB_TranslateInputArgs() {
    pInput = NULL;
    InputSize = 0;
    pOptions = NULL;
    OptionsSize = 0;
    pInternalOptions = NULL;
    InternalOptionsSize = 0;
    pTracingOptions = NULL;
    TracingOptionsCount = 0;
    GTPinInput = NULL;
    CompileTimeStatisticsEnable = false;
    pSpecConstantsIds = NULL;
    pSpecConstantsValues = NULL;
    SpecConstantsSize = 0;
    pVISAAsmToLinkArray = NULL;
    NumVISAAsmsToLink = 0;
    pDirectCallFunctions = NULL;
    NumDirectCallFunctions = 0;
  }
};

/******************************************************************************\

Structure:
    STB_TranslateOutputArgs

Description:
    Structure used to hold data returned from the translation block

\******************************************************************************/
typedef llvm::SmallVector<char, 0> OutBufferType;
struct STB_TranslateOutputArgs {
  OutBufferType Output;    // translated data buffer
  std::string ErrorString; // string to print if translate fails
  OutBufferType DebugData; // translated debug data buffer
};

struct TranslationBlockVersion {
  static const uint32_t VersioningIsUnsupported = (uint32_t)-1;
  uint32_t TBVersion; // STB_VERSION version of this translation block
  uint32_t BuildId;   // build ID (for CI builds) of this translation block
};

/******************************************************************************\
Class:
    CTranslationBlock

Description:
    Interface used to expose required functions to translation plug-ins
\******************************************************************************/
class CTranslationBlock {
public:
  virtual bool Translate(const STB_TranslateInputArgs *pInput, STB_TranslateOutputArgs *pOutput) = 0;

  virtual bool GetOpcodes(void *pOpcodes, uint32_t pOpcodesSize) {
    IGC_UNUSED(pOpcodes);
    IGC_UNUSED(pOpcodesSize);
    return false;
  }
  virtual bool GetOpcodesCount(uint32_t *pOpcodesCount, uint32_t *pOpcodeSize) {
    IGC_UNUSED(pOpcodesCount);
    IGC_UNUSED(pOpcodeSize);
    return false;
  }
  virtual TranslationBlockVersion GetVersion() const {
    TranslationBlockVersion tbv;
    tbv.TBVersion = TC::STB_VERSION;
    uint32_t buildId = TranslationBlockVersion::VersioningIsUnsupported;
#ifdef TB_BUILD_ID
    buildId = TB_BUILD_ID;
#endif
    tbv.BuildId = buildId;

    return tbv;
  }
  virtual ~CTranslationBlock() {}
};
} // namespace TC

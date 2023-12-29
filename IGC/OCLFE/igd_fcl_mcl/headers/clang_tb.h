/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "GlobalData.h"
#include "igfxfmid.h"
#include "TranslationBlock.h"
#include <string>
#include <memory>
#include <vector>

#include "LoadBuffer.h"
#include "AdaptorOCL/CLElfLib/ElfReader.h"
// TODO: really need this to point to the header file from the common clang
// zip instead of our own copy.
#include "common_clang.h"

namespace llvm { class MemoryBuffer; }

// forward decls
namespace CLElfLib{ class CElfReader; }

namespace Intel {
  namespace OpenCL {
    namespace ClangFE {
      struct IOCLFEBinaryResult;
    }
  }
}

namespace TC
{
  struct TranslateClangArgs;

  static const STB_TranslationCode g_cClangTranslationCodes[] =
  {
    { TB_DATA_FORMAT_OCL_TEXT,  TB_DATA_FORMAT_LLVM_TEXT },
    { TB_DATA_FORMAT_OCL_TEXT,  TB_DATA_FORMAT_LLVM_BINARY },
    { TB_DATA_FORMAT_OCL_TEXT,  TB_DATA_FORMAT_SPIR_V },
    { TB_DATA_FORMAT_ELF,       TB_DATA_FORMAT_LLVM_BINARY },
    { TB_DATA_FORMAT_ELF,       TB_DATA_FORMAT_SPIR_V },
    { TB_DATA_FORMAT_ELF,       TB_DATA_FORMAT_LLVM_ARCHIVE }
  };

#ifdef _WIN32
#ifdef COMMON_CLANG_LIB_FULL_NAME
  struct CCModuleStruct {
    typedef decltype(Compile) *PFcnCCCompile;

    void* pModule = nullptr;
    PFcnCCCompile pCompile = nullptr;

    const char *pModuleName = COMMON_CLANG_LIB_FULL_NAME;
  };
#else
#   error "Common clang name not defined"
#endif
#endif

  /***************************************************************************\

  Class:
      CClangTranslationBlock

  Description:

  \***************************************************************************/
  class CClangTranslationBlock
      : public CTranslationBlock
  {
      TB_DATA_FORMAT m_InputFormat{};
      TB_DATA_FORMAT m_OutputFormat{};
#ifdef _WIN32
    CCModuleStruct m_CCModule;
#endif


  public:
      static bool     Create(
                          const STB_CreateArgs* pCreateArgs,
                          STB_TranslateOutputArgs* pOutputArgs,
                          CClangTranslationBlock* &pTranslationBlock );

      static void     Delete(
                          CClangTranslationBlock* &pTranslationBlock );

      virtual bool    Translate(
                          const STB_TranslateInputArgs* pInputArgs,
                          STB_TranslateOutputArgs* pOutputArgs );

      virtual bool    FreeAllocations(
                          STB_TranslateOutputArgs* pOutputArgs );

      void SetOclApiVersion(unsigned int oclVersion){
          m_OCL_Ver = std::to_string(oclVersion);
      }

      static void SetErrorString(const char* pErrorString, STB_TranslateOutputArgs* pOutputArgs);

  private:
    // Is assigned the value of the OpenCL API version that corresponds to the
    // current OCL runtime driver.  The values are defined in the OpenCL spec
    // (e.g., 1.1 -> 110, 1.2 -> 120, etc.).  This is equivalent to
    // the __OPENCL_VERSION__ macro (spec 6.10).
    std::string m_OCL_Ver;

    // vector of supported OpenCL extensions for current device
    std::vector<std::string> m_Extensions;

    PRODUCT_FAMILY m_HWPlatform{};   // The HW platform that the device is running on
    GFXCORE_FAMILY m_CoreFamily{};
    SGlobalData m_GlobalData{};

    // Using auto pointer to get CTH MemoryBuffer from LoadBuffer
    char *m_cthBuffer = nullptr;

  protected:
      CClangTranslationBlock( void );

      virtual ~CClangTranslationBlock( void );


      // Read OCL version from internal options
      std::string GetOclApiVersion( const char* pInternalOptions ) const;

      bool TranslateClang( const TranslateClangArgs* pInputArgs,
                           STB_TranslateOutputArgs* pOutputArgs,
                           std::string& exceptString,
                           const char* pInternalOptions);

      bool TranslateElf( const STB_TranslateInputArgs* pInputArgs,
                         STB_TranslateOutputArgs* pOutputArgs,
                         std::string& exceptString);

      void GetTranslateClangArgs( char* pInput,
                                  uint32_t    uiInputSize,
                                  const char* pOptions,
                                  const char* pInternalOptions,
                                  TranslateClangArgs* pClangArgs,
                                  std::string& exceptString);

      void GetTranslateClangArgs( CLElfLib::CElfReader* pElfReader,
                                  const char* pOptions,
                                  const char* pInternalOptions,
                                  TranslateClangArgs* pClangArgs,
                                  std::string& exceptString);

      void EnsureProperPCH( TranslateClangArgs* pArgs, const char* pInternalOptions, std::string& exceptString);

      bool ReturnSuppliedIR( const STB_TranslateInputArgs* pInputArgs,
                           STB_TranslateOutputArgs* pOutputArgs );

      bool Initialize( const STB_CreateArgs* pCreateArgs );
  };

} // namespace TC

namespace FCL {

  // type for name of output folder
  using OutputFolderName = const char*;

  // function returns full path where to dump shaders for FCL
  OutputFolderName  GetShaderOutputFolder();

  // different register-dependent functions
  bool GetFCLShaderDumpEnable();
  bool GetFCLShaderDumpPidDisable();
  bool GetFCLDumpToCurrentDir();
  bool GetFCLDumpToCustomDir();
  bool GetFCLEnableKernelNamesBasedHash();
} // namespace FCL

// convenient macro to check FCL flags
#define FCL_IGC_IS_FLAG_ENABLED(name) FCL::GetFCL##name()

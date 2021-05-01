/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../headers/clang_debug.h"

#if defined _WIN32

#if defined(_DEBUG) || defined(_INTERNAL)

namespace clang
{

  /*****************************************************************************\
  Declaring all registry values for CLANG component:
  \*****************************************************************************/

  const SDebugVariableEnumValue g_cl_intel_simd_operations_placeholder[] =
  {
    { "cl_intel_simd_operations_placeholder functionality is disabled (default)", 0x0 },
    { "cl_intel_simd_operations_placeholder functionality is enabled", 0x1 },
  };

  const SDebugVariableEnumValue g_cl_khr_fp16[] =
  {
    { "cl_khr_fp16 forced disabled on unsupported products (default)", 0x0 },
    { "cl_khr_fp16 is enabled", 0x1 },
  };

  const SDebugVariableEnumValue g_cl_khr_fp64[] =
  {
    { "cl_khr_fp64 forced disabled on unsupported products (default)", 0x0 },
    { "cl_khr_fp64 is enabled", 0x1 },
  };

  const SDebugVariableEnumValue g_cl_khr_subgroups[] =
  {
    { "cl_khr_subgroups stays disabled on unsupported products (default)", 0x0 },
    { "cl_khr_subgroups is enabled", 0x1 },
  };

  struct DEBUG_VARIABLES_REGKEY
  {
    ///////////////////////////////////////////////////////////////////////////////
    // CLANG_Debug
    ///////////////////////////////////////////////////////////////////////////////
    OCL_CLANG_DeclareEnum(
      cl_intel_simd_operations_placeholder,
      "CLANG_Debug",
      "Enables the prototype SIMD built-in functions.  You probably don't want to set this if you're writing new code - use the cl_intel_subgroups functionality instead.  See: SIMD Shuffles for more information.",
      g_cl_intel_simd_operations_placeholder);

    OCL_CLANG_DeclareEnum(
      cl_khr_fp16_, // cl_khr_fp16 actual ask ???
      "CLANG_Debug",
      "Force enables the cl_khr_fp16 (half precision) extension through the compiler, even on products that do not support the extension by default.  Currently, this only applies to Broadwell (Gen8).  This includes: defining cl_khr_fp16, allowing the pragma for cl_khr_fp16, and enabling fp16 code generation in the IGIL backend.",
      g_cl_khr_fp16);

    OCL_CLANG_DeclareEnum(
      cl_khr_fp64_,
      "CLANG_Debug",
      "Enables the cl_khr_fp64 (double precision) extension through the compiler.  This includes: defining cl_khr_fp64, allowing the pragma for cl_khr_fp64, and enabling fp64 code generation in the IGIL backend.  This is mostly intended to aid performance evaluation of fp64 features and is not a conformant fp64 implementation.  Be very careful drawing performance conclusions using this registry key if your workload uses a non-conformant fp64 built-in function",
      g_cl_khr_fp64);

    OCL_CLANG_DeclareEnum(
      cl_khr_subgroups,
      "CLANG_Debug",
      "Force enables the cl_khr_subgroups extension through the compiler.  This includes: defining cl_khr_subgroups and allowing the pragma for cl_khr_subgroups.  This was primarily added to get additional test coverage for the cl_intel_subgroups functionality via the Khronos cl_khr_subroups conformance tests, though this hasn't been tried to see how useful it is in practice.",
      g_cl_khr_subgroups);

    OCL_CLANG_DeclareString(
      LLVMOptionsString,
      "CLANG_Debug",
      "Pass compile options to LLVM (for debug builds). Any valid LLVM options can be listed here. A good place to start may be something like: -debug -debug-only=isel -print-machineinstrs -print-after-all");

    OCL_CLANG_DeclareString(
      CLANGOptionsString,
      "CLANG_Debug",
      "Pass compile options to CLANG (for debug builds)."); // Get description info later
  };

  DEBUG_VARIABLES_REGKEY g_DebugVariables;

  const DWORD NUM_DEBUG_VARIABLES = sizeof(DEBUG_VARIABLES_REGKEY) / sizeof(SDebugVariableMetaData);

  /*****************************************************************************\
  Function:LoadDebugVariables

  Description:Loads debug variables from the registry

  Input: None

  Output:None
  \*****************************************************************************/

  void __cdecl LoadDebugVariables(void)
  {
#if defined(WIN32) && defined(ENABLE_REGISTRY_READ)

    SDebugVariableMetaData* pDebugVariable =
      (SDebugVariableMetaData*)&g_DebugVariables;

    for (DWORD i = 0; i < NUM_DEBUG_VARIABLES; i++)
    {
      DWORD value = 0;

      const char* name = pDebugVariable[i].GetName();

      value = llvm::ReadRegistry(name, value);

      pDebugVariable[i].m_IsSet = TRUE;

      pDebugVariable[i].m_Value = value;
    }
#endif
  }

  PFNLOADDEBUGVARIABLES pfnLoadDebugVariables = LoadDebugVariables;

  void __cdecl DumpRegistryKeyDefinitions(void)
  {
    // Create the directory path
    CreateDirectoryA("C:\\Intel", NULL);
    CreateDirectoryA("C:\\Intel\\IGfx", NULL);
    CreateDirectoryA("C:\\Intel\\IGfx\\GfxRegistryManager", NULL);
    CreateDirectoryA("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys", NULL);

    // Create the XML file to hold the debug variable definitions
    FILE* fp = NULL;
    errno_t result = fopen_s(&fp, "C:\\Intel\\IGfx\\GfxRegistryManager\\Keys\\OCL.CLANG.xml", "w");
    if (fp == NULL ||
      result != 0)
    {
      printf_s("Unsuccessful OCL registry dump");
      return;
    }

    SDebugVariableMetaData* pDebugVariable =
      (SDebugVariableMetaData*)&g_DebugVariables;

    const char* currentGroup = "Generic";

    // Generate the XML
    fprintf(fp, "<RegistryKeys>\n");

    // Dump the DEBUG_VARIABLES entries
    for (DWORD i = 0; i < NUM_DEBUG_VARIABLES; i++)
    {
      if (strcmp(currentGroup, pDebugVariable[i].GetGroup()) != 0)
      {
        // Create new group tag
        currentGroup = pDebugVariable[i].GetGroup();
        fprintf(fp, "  <Group name=\"%s\">\n", currentGroup);
      }
      pDebugVariable[i].DumpDefinition(fp);
    }

    fprintf(fp, "  </Group>\n");
    fprintf(fp, "</RegistryKeys>\n");

    fclose(fp);
    fp = NULL;
  }

} // namespace clang

#endif

#endif

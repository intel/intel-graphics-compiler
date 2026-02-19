#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================


# This file sets up the following libraries and executables:
# Target name                |     CMake project name             |   Supported Platforms
#----------------------------+------------------------------------+---------------------------
# GenX_IR.exe (GenX_IR)      |     GenX_IR_Exe                    |   Windows, Linux
# vISA libs will be linked directly into igc.dll

include(Functions.cmake)

# TODO: Move this to a common file?
# Get the latest commit hash
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions(-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\")

#none of the values about 32/64 are being propogated down
#and BuildSetup.cmake is not part of IGC enviroment
#so setting it manually

# Distinguish between 32 and 64 bits
# The string that is set is used to modify the target names of some of the libraries generated
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(TARGET_MODIFIER "32")
    set(PB_PATH_MODIFIER "x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TARGET_MODIFIER "64")
    set(PB_PATH_MODIFIER "x64")
else()
    message(FATAL_ERROR "unexpected platform")
endif()

set(LINK_DLL_IGA 1)


# Set up build flags to use dynamic multi-threaded runtime (/MD) or
# to use static multi-threaded runtime (/MT)
option(LINK_AS_STATIC_LIB "link with /MT or /MD" ON)


################################################################################
# FC_link Related

if (WIN32)
message("-- Configuring Fast Composite Component")
add_subdirectory(FC_linker)
endif (WIN32)
################################################################################
# IGA Related
#
#   Controls whether internal or external GED is used and whether IP sensitive
#   code is used or not (with appropriate changes in IGA).
#

  set(GED_BRANCH GED_external)

message("-- Configuring Intel Gen Assembler (IGA) Component")
message("--  - GED_BRANCH:           ${GED_BRANCH}")
message("--  - CMAKE_CXX_COMPILER:   ${CMAKE_CXX_COMPILER}")
add_subdirectory(iga/GEDLibrary/${GED_BRANCH})
add_subdirectory(iga/IGALibrary)
if (WIN32 OR UNIX)
  add_subdirectory(iga/IGAExe)
endif (WIN32 OR UNIX)

cmake_minimum_required(VERSION 3.10)

# In the case where this is the IGC build we need to add a dummy custom target check_headers
add_custom_target(check_headers)

set_target_properties(check_headers PROPERTIES FOLDER "Misc")

if(MSVC)
  bs_set_wdk(check_headers)
endif(MSVC)

if(BS_USE_OSDM_BUILD_SYSTEM)
  include(${BUILD_SYS_INC}/utils.cmake)
  # Locate bison and flex using common bs macro
  bs_find_flex()
  bs_find_bison()
  if (WIN32)
    set(WIN_FLEX_FLAG "--wincompat")
  endif(WIN32)
else()
  # Locate bison and flex if we're on windows (they're included in the repository)
  # The following commands will prime the CMake cache with the local paths to the relevant executables and
  # prevent the find_package commands from looking elsewhere
  if (WIN32)
    # Find the GNUTools path
    # In CM_RT depo, GNUTools is in the same level with CM_jitter
    # In gfx_Development depo, GNUTools is in media/cmrtlib/proprietary
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../GNUTools)
      set(GNUTOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../GNUTools)
    elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../media/cmrtlib/GNUTools)
      set(GNUTOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../media/cmrtlib/GNUTools)
    elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../cmc/support/GNUTools)
      set(GNUTOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cmc/support/GNUTools)
    else ()
      message( FATAL_ERROR "No GNUTools folder found.")
    endif ()
    find_program(BISON_EXECUTABLE bison.bat PATHS ${GNUTOOLS_DIR}/bin DOC "path to the bison executable")
    find_program(FLEX_EXECUTABLE flex PATHS ${GNUTOOLS_DIR}/bin DOC "path to the flex executable")
    #set (BISON_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../GNUTools/bin/bison)
    #set (FLEX_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../GNUTools/bin/flex)
    set (ENV{M4} "${GNUTOOLS_DIR}/bin/m4.exe")
  endif (WIN32)

  # Here the default package support for bison and flex is invoked. This does something for Windows
  # and Linux but is effectively by-passed for Android as we've already explicitly set the bison and
  # flex executable variables for that platform
  find_package(BISON)
  find_package(FLEX)
endif()
# Set up the bison and flex targets. These commands will set up commands to generate the appropriate
# source files from the input grammars. It will also set up the dependencies correctly for any
# library or executable that uses the generated source

set(bison_output_file ${CMAKE_CURRENT_BINARY_DIR}/CISA.tab.cpp)
set(flex_output_file ${CMAKE_CURRENT_BINARY_DIR}/lex.CISA.cpp)

BISON_TARGET(CISAParser CISA.y ${bison_output_file} COMPILE_FLAGS "-vt -p CISA")
FLEX_TARGET(CISAScanner CISA.l ${flex_output_file} COMPILE_FLAGS "-PCISA ${WIN_FLEX_FLAG}")
ADD_FLEX_BISON_DEPENDENCY(CISAScanner CISAParser)
set(CISAScanner_dependencies)

list(APPEND CISAScanner_dependencies
    ${FLEX_CISAScanner_OUTPUTS}
    ${BISON_CISAParser_OUTPUTS})

add_custom_target(CISAScanner_target DEPENDS ${CISAScanner_dependencies})

set_target_properties(CISAScanner_target PROPERTIES FOLDER "Misc")

if (MSVC)
  #set up standard defines from the common WDK path.
  bs_set_wdk(CISAScanner_target)
endif()

# Set up include paths used by all the libraries in this file.
set(Jitter_inc_dirs ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (LINK_AS_STATIC_LIB)
  win_static_runtime()
else (LINK_AS_STATIC_LIB)
  win_dynamic_runtime()
endif (LINK_AS_STATIC_LIB)

include_directories(${Jitter_inc_dirs})

# Tell cmake to generate code to compile the flex and bison generated source as c++ rather than c
# (due to the fact that they are .c files rather than .cpp)
set_source_files_properties( CISA.tab.cpp lex.CISA.cpp PROPERTIES LANGUAGE CXX )

  set(LocalScheduler_SOURCES
    LocalScheduler/Dependencies_G4IR.cpp
    LocalScheduler/G4_Sched.cpp
    LocalScheduler/LatencyTable.cpp
    LocalScheduler/LocalScheduler_G4IR.cpp
    LocalScheduler/SWSB_G4IR.cpp
    )

  set(LocalScheduler_HEADERS
    LocalScheduler/Dependencies_G4IR.h
    LocalScheduler/LatencyTable.h
    LocalScheduler/LocalScheduler_G4IR.h
    LocalScheduler/SWSB_G4IR.h
    )

# Set up some common source files used in all the projects so they only need to be defined once
set(GenX_Common_Sources_VisaToG4
  VisaToG4/TranslateALU.cpp
  VisaToG4/TranslateBranches.cpp
  VisaToG4/TranslateMath.cpp
  VisaToG4/TranslateMisc.cpp
  VisaToG4/TranslateSend3D.cpp
  VisaToG4/TranslateSendLdStLegacy.cpp
  VisaToG4/TranslateSendLdStLsc.cpp
  VisaToG4/TranslateSendMedia.cpp
  VisaToG4/TranslateSendRaw.cpp
  VisaToG4/TranslateSendSync.cpp
  )

set(GenX_Common_Sources_G4_IRBuilder
  BuildIRImpl.cpp)

set(GenX_Common_Sources_G4_Passes
  Passes/AccSubstitution.cpp
  Passes/AccSubstitution.hpp
  Passes/SRSubstitution.cpp
  Passes/SRSubstitution.hpp
  Passes/InsertS0Movs.cpp
  Passes/InsertS0Movs.hpp
  Passes/InsertThryld.cpp
  Passes/InsertThryld.hpp
  Passes/InstCombine.cpp
  Passes/InstCombine.hpp
  Passes/LVN.cpp
  Passes/LVN.hpp
  Passes/MergeScalars.cpp
  Passes/MergeScalars.hpp
  Passes/SendFusion.cpp
  Passes/SendFusion.hpp
  Passes/StaticProfiling.cpp
  Passes/StaticProfiling.hpp
  )


set(GenX_Common_Sources_External_Other
  Assertions.cpp
  Attributes.cpp
  BinaryEncoding.cpp
  BinaryEncodingCNL.cpp
  BinaryEncodingIGA.cpp
  BuildCISAIRImpl.cpp
  ByteCodeReaderNG.cpp
  CFGStructurizer.cpp
  Common_BinaryEncoding.cpp
  DebugInfo.cpp
  RADebug.cpp
  FlagSpillCleanup.cpp
  FlowGraph.cpp
  FrequencyInfo.cpp
  G4_BB.cpp
  G4_IR.cpp
  G4_Kernel.cpp
  G4_SendDescs.cpp
  G4_Verifier.cpp
  GraphColor.cpp
  HWConformity.cpp
  HWConformityPro.cpp
  IncrementalRA.cpp
  EmuInt64Add.cpp
  InstSplit.cpp
  JitterDataStruct.cpp
  KernelCost.cpp
  LinearScanRA.cpp
  LocalDataflow.cpp
  LocalRA.cpp
  LoopAnalysis.cpp
  Lowered_IR.cpp
  MetadataDumpRA.cpp
  Optimizer.cpp
  PhyRegUsage.cpp
  PointsToAnalysis.cpp
  PreDefinedVars.cpp
  PrologEpilog.cpp
  RPE.cpp
  ReduceExecSize.cpp
  RegAlloc.cpp
  RelocationEntry.cpp
  Rematerialization.cpp
  SCCAnalysis.cpp
  SplitAlignedScalars.cpp
  SpillCleanup.cpp
  SpillCode.cpp
  SpillManagerGMRF.cpp
  SWWA.cpp
  TranslationInterface.cpp
  VISAKernelImpl.cpp
  VarSplit.cpp
  ifcvt.cpp
  main.cpp
  ${LocalScheduler_SOURCES}
)

set(GenX_Common_Sources_External
  ${GenX_Common_Sources_External_Other}
  ${GenX_Common_Sources_G4_IRBuilder}
  ${GenX_Common_Sources_G4_Passes}
  ${GenX_Common_Sources_VisaToG4}
)

set(GenX_Common_Sources
    ${GenX_Common_Sources_External}
)

set(GenX_CISA_dis_Common_Sources
  Common_ISA_framework.cpp
  Common_ISA_util.cpp
  IsaDescription.cpp
  IsaDisassembly.cpp
  IsaVerification.cpp
  DebugInfo.cpp
)

set(Jitter_Common_Sources
  Common_ISA.cpp
)

set(Jitter_Utility_Files
  Arena.cpp
  Arena.h
  common.cpp
  Mem_Manager.cpp
  Mem_Manager.h
  Option.cpp
  )

set(GenX_Utility_Files
  BitSet.cpp
  BitSet.h
  FastSparseBitVector.h
  Timer.cpp
  Timer.h
  )

set(GenX_Common_Headers
  Attributes.hpp
  Assertions.h
  BinaryEncoding.h
  BinaryEncodingCNL.h
  BinaryEncodingIGA.h
  BuildCISAIR.h
  BuildIR.h
  CFGStructurizer.h
  Common_BinaryEncoding.h
  Common_GEN.h
  DebugInfo.h
  RADebug.h
  FlagSpillCleanup.h
  FlowGraph.h
  FrequencyInfo.h
  G4_BB.hpp
  G4_Declare.h
  G4_IR.hpp
  G4_Instruction.h
  G4_Kernel.hpp
  G4_MsgOpDefs.hpp
  G4_Opcode.h
  G4_Operand.h
  G4_Register.h
  G4_SendDescs.hpp
  G4_Verifier.hpp
  GraphColor.h
  HWConformity.h
  EmuInt64Add.h
  IGfxHwEuIsaCNL.h
  InstSplit.h
  KernelCost.hpp
  LinearScanRA.h
  LocalDataflow.h
  LocalRA.h
  Metadata.h
  MetadataDumpRA.h
  Optimizer.h
  PhyRegUsage.h
  PointsToAnalysis.h
  PreDefinedVars.h
  RPE.h
  RegAlloc.h
  RelocationEntry.hpp
  Rematerialization.h
  SCCAnalysis.h
  SpillCleanup.h
  SpillCode.h
  SpillManagerGMRF.h
  SplitAlignedScalars.h
  VISAKernel.h
  VarSplit.h
  HWCaps.inc
  include/JitterDataStruct.h
  include/KernelInfo.h
  include/KernelCostInfo.h
  include/RT_Jitter_Interface.h
  include/VISABuilderAPIDefinition.h
  include/VISAOptions.h
  include/VISADefines.h
  include/VISAOptionsDefs.h
  include/gtpin_IGC_interface.h
  include/visa_igc_common_header.h
  ${LocalScheduler_HEADERS}
)
set(GenX_CISA_dis_Common_Headers
  common.h
  Common_ISA_framework.h
  Common_ISA_util.h
  IsaDescription.h
  IsaDisassembly.h
  IsaVerification.h
  Option.h
  PlatformInfo.h
)

set(Jitter_Common_Headers
  Common_ISA.h
)

add_compile_definitions(
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# ###############################################################
# GenX_IR_Exe
# ###############################################################
if (UNIX OR WIN32)
  set(GenX_IR_EXE_SOURCES
    ${GenX_Common_Sources}
    ${GenX_CISA_dis_Common_Sources}
    ${Jitter_Common_Sources}
    )

  set(GenX_IR_EXE_UTILITY
    ${Jitter_Utility_Files}
    ${GenX_Utility_Files}
    )

  set(GenX_IR_EXE_HEADERS
    ${GenX_Common_Headers}
    ${GenX_CISA_dis_Common_Headers}
    ${Jitter_Common_Headers}
    ${GenX_Common_Headers}
    )

  set(GenX_IR_EXE_lex_yacc
    CISA.l
    CISA.y
    )

  set(GenX_IR_EXE_All_Sources
    ${GenX_IR_EXE_SOURCES}
    ${GenX_IR_EXE_UTILITY}
    ${GenX_IR_EXE_HEADERS}
    ${BISON_CISAParser_OUTPUTS}
    ${FLEX_CISAScanner_OUTPUTS}
    ${GenX_IR_EXE_lex_yacc}
    )

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IGC_BUILD__TOOLS_OUTPUT_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IGC_BUILD__TOOLS_OUTPUT_DIR})
  setup_executable(GenX_IR_Exe "${GenX_IR_EXE_All_Sources}" FALSE "GenX_IR")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "")
  add_dependencies(GenX_IR_Exe check_headers)
  add_dependencies(GenX_IR_Exe CISAScanner_target)

  source_group("Utility Files" FILES ${GenX_IR_EXE_UTILITY} )
  source_group("Header Files" FILES ${GenX_IR_EXE_HEADERS} )
  source_group("G4 IR Builder" FILES ${GenX_Common_Sources_G4_IRBuilder} )
  source_group("G4 Passes" FILES ${GenX_Common_Sources_G4_Passes} )
  source_group("Visa to G4 IR Translation" FILES ${GenX_Common_Sources_VisaToG4} )
  source_group("Lex Yacc Files" FILES ${GenX_IR_EXE_lex_yacc} )

  igc_get_llvm_targets(LLVM_LIBS Support)
  target_link_libraries(GenX_IR_Exe IGA_SLIB IGA_ENC_LIB ${LLVM_LIBS})

  if (UNIX)
    target_link_libraries(GenX_IR_Exe dl)
    if(NOT ANDROID)
      target_link_libraries(GenX_IR_Exe rt)
    endif()
  endif(UNIX)

     set(GenX_IR_Exe_DEFINITIONS STANDALONE_MODE)

  set_target_properties(GenX_IR_Exe PROPERTIES
          COMPILE_DEFINITIONS "${GenX_IR_Exe_DEFINITIONS}"
          FOLDER "Tools/CM_JITTER_EXE")

  if (MSVC)
  #set up standard defines from the common WDK path.
  bs_set_wdk(GenX_IR_Exe)
  endif()


endif(UNIX OR WIN32)

# ###############################################################
# GenX_IR (dll)
# ###############################################################

set(GenX_IR_DLL_SOURCES
  ${GenX_Common_Sources}
  ${GenX_CISA_dis_Common_Sources}
  ${Jitter_Common_Sources}
  visaBuilder_export.cpp
)

set(GenX_IR_DLL_UTILITY
  ${Jitter_Utility_Files}
  ${GenX_Utility_Files}
)

set(GenX_IR_DLL_HEADERS
  ${GenX_Common_Headers}
  ${GenX_CISA_dis_Common_Headers}
  ${Jitter_Common_Headers}
  ${GenX_Common_Headers}
)

if (WIN32)
  set(JIT_CUSTOM_LINK_FLAGS "/OPT:ref /MANIFEST:NO")
else(WIN32)
  set(JIT_CUSTOM_LINK_FLAGS "")
endif(WIN32)

#disable exceptions, windows only for now
if(WIN32)
  string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  add_definitions(-D_HAS_EXCEPTIONS=0)
endif()

  add_library(GenX_IR STATIC
       ${GenX_IR_DLL_SOURCES}
       ${GenX_IR_DLL_UTILITY}
       ${GenX_IR_DLL_HEADERS}
       ${BISON_CISAParser_OUTPUTS}
       ${FLEX_CISAScanner_OUTPUTS}
       ${GenX_IR_EXE_lex_yacc}
    )
  set_target_properties(GenX_IR PROPERTIES FOLDER "Libraries")
  add_dependencies(GenX_IR CISAScanner_target)
  set_target_properties( GenX_IR PROPERTIES OUTPUT_NAME "igfxcmjit${TARGET_MODIFIER}")
  if(WIN32)
    target_link_libraries(GenX_IR ${GCC_SECURE_LINK_FLAGS} IGA_ENC_LIB IGA_SLIB)
    add_dependencies(GenX_IR IGA_DLL)
  else()
    target_link_libraries(GenX_IR ${GCC_SECURE_LINK_FLAGS} IGA_ENC_LIB IGA_SLIB)
    add_dependencies(GenX_IR IGA_DLL)
  endif(WIN32)

source_group("Header Files" FILES ${GenX_IR_DLL_HEADERS} )
source_group("Utility Files" FILES ${GenX_IR_DLL_UTILITY} )

    set(GenX_IR_definitions DLL_MODE)
if( WIN32 AND ("${BUILD_WINDOWS_MOBILE}" STREQUAL "TRUE") )
    set(GenX_IR_definitions ${GenX_IR_definitions} _ATL_NO_WIN_SUPPORT)
    win_mobile_set_ignore_specific_libraries(GenX_IR)
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(GenX_IR_definitions ${GenX_IR_definitions} WIN64)
endif()

 if (MSVC)
   #set up standard defines from the common WDK path.
   bs_set_wdk(GenX_IR)
 endif()

if(NOT WIN32)
  set_target_properties( GenX_IR PROPERTIES PREFIX "")
endif()

# Copy any required headers
set(headers_to_copy
  include/visaBuilder_interface.h
  include/VISABuilderAPIDefinition.h
  include/visa_igc_common_header.h
  include/JitterDataStruct.h
  include/KernelInfo.h
  include/KernelCostInfo.h
)

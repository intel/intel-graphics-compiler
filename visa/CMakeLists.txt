
# This file sets up the following libraries and executables:
# Target name                |     CMake project name             |   Supported Platforms
#----------------------------+------------------------------------+---------------------------
# GenX_IR.exe (GenX_IR)      |     GenX_IR_Exe                    |   Windows, Linux
# CISA_ld.exe (CISA_ld)      |     CISA_ld_Exe                    |   Windows, Linux
# If IGC_BUILD is set, vISA libs will be linked directly into igc.dll

include(Functions.cmake)


#none of the values about 32/64 are being propogated down
#and BuildSetup.cmake is not part of IGC enviroment
#so setting it manually
if(IGC_BUILD)
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
endif(IGC_BUILD)

if (NOT IGC_BUILD)
set(MEDIA_IGA 1)
endif(NOT IGC_BUILD)
if(IGC_BUILD)
set(LINK_DLL_IGA 1)
endif(IGC_BUILD)


# Set up build flags to use dynamic multi-threaded runtime (/MD) or
# to use static multi-threaded runtime (/MT)
option(LINK_AS_STATIC_LIB "link with /MT or /MD" ON)


################################################################################
# IGA Related
#
#   Controls whether internal or external GED is used and whether IP sensitive
#   code is used or not (with appropriate changes in IGA).
#

if(NOT IGC_BUILD)
endif(NOT IGC_BUILD)

  set(GED_BRANCH GED_external)

message("-- Configuring Intel Gen Assembler (IGA) Component")
message("--  - GED_BRANCH:           ${GED_BRANCH}")
message("--  - CMAKE_CXX_COMPILER:   ${CMAKE_CXX_COMPILER}")
add_subdirectory(iga/GEDLibrary/${GED_BRANCH})
add_subdirectory(iga/IGALibrary)
if (WIN32 OR UNIX)
  add_subdirectory(iga/IGAExe)
endif (WIN32 OR UNIX)

if(WIN32)
  cmake_minimum_required(VERSION 3.1)
  cmake_policy(SET CMP0043 OLD)
else()
  cmake_minimum_required(VERSION 2.8)
endif(WIN32)

# In the case where this is the IGC build we need to add a dummy custom target check_headers
if (IGC_BUILD)
  add_custom_target(check_headers)

  if(MSVC)
    bs_set_wdk(check_headers)
  endif(MSVC)

endif (IGC_BUILD)

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

  # The Android definition has some issues locating an appropriate bison and flex so here we
  # explicitly set the executable variables directly to prevent find_package from failing
  if (ANDROID)
    set (BISON_EXECUTABLE /usr/bin/bison)
    set (FLEX_EXECUTABLE /usr/bin/flex)
  endif(ANDROID)

  # Here the default package support for bison and flex is invoked. This does something for Windows
  # and Linux but is effectively by-passed for Android as we've already explicitly set the bison and
  # flex executable variables for that platform
  find_package(BISON)
  find_package(FLEX)
endif()
# Set up the bison and flex targets. These commands will set up commands to generate the appropriate
# source files from the input grammars. It will also set up the dependencies correctly for any
# library or executable that uses the generated source
BISON_TARGET(CISAParser CISA.y ${CMAKE_CURRENT_BINARY_DIR}/CISA.tab.c COMPILE_FLAGS "-vt -p CISA")
FLEX_TARGET(CISAScanner CISA.l ${CMAKE_CURRENT_BINARY_DIR}/lex.CISA.c COMPILE_FLAGS "-PCISA ${WIN_FLEX_FLAG}")
ADD_FLEX_BISON_DEPENDENCY(CISAScanner CISAParser)

# Set up include paths used by all the libraries in this file
# There is a windows specific include path for GNUTools support that is inside the repository. The
# equivalent support is already included for Linux and Android so is not required for those platforms
set(Jitter_inc_dirs ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ../../compiler/include)
if (WIN32)
  set(Jitter_inc_dirs ${Jitter_inc_dirs} ${GNUTOOLS_DIR}/include)
endif (WIN32)

if (LINK_AS_STATIC_LIB)
  win_static_runtime()
else (LINK_AS_STATIC_LIB)
  win_dynamic_runtime()
endif (LINK_AS_STATIC_LIB)

# Some Android specific paths primarily to enable support for STL
if(ANDROID AND MEDIA_IGA)
  set(NDK_Libstdcxx $ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libcxx)
  set(Extra_Android_Inc ${NDK_Libstdcxx}/include ${ANDROID_NDK}/sources/android/support/include ${NDK_Libstdcxx}/include/backward)
  link_directories($ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/$ENV{CMRT_ARCH})
  include_directories(${Extra_Android_Inc})
elseif(ANDROID)
	link_directories($ENV{ANDROID_PRODUCT_OUT}/system/lib)
endif(ANDROID AND MEDIA_IGA)

include_directories(${Jitter_inc_dirs})

# Tell cmake to generate code to compile the flex and bison generated source as c++ rather than c
# (due to the fact that they are .c files rather than .cpp)
set_source_files_properties( CISA.tab.c lex.CISA.c PROPERTIES LANGUAGE CXX )

  set(LocalScheduler_SOURCES
    LocalScheduler/Dependencies_G4IR.cpp
    LocalScheduler/G4_Sched.cpp
    LocalScheduler/LatencyTable.cpp
    LocalScheduler/LocalScheduler_G4IR.cpp
    )

  set(LocalScheduler_HEADERS
    LocalScheduler/Dependencies_G4IR.h
    LocalScheduler/LatencyTable.h
    LocalScheduler/LocalScheduler_G4IR.h
    )

# Set up some common source files used in all the projects so they only need to be defined once
set(GenX_Common_Sources_External
  BinaryCISAEmission.cpp
  BinaryEncoding.cpp
  Common_BinaryEncoding.cpp
  BinaryEncodingCNL.cpp
  BinaryEncodingIGA.cpp
  BuildCISAIRImpl.cpp
  BuildIRImpl.cpp
  ByteCodeReaderNG.cpp
  DebugInfo.cpp
  FlowGraph.cpp
  CFGStructurizer.cpp
  Gen4_IR.cpp
  GraphColor.cpp
  HWConformity.cpp
  SendFusion.cpp
  LocalDataflow.cpp
  LocalRA.cpp
  Lowered_IR.cpp
  main.cpp
  Optimizer.cpp
  PhyRegCompute.cpp
  PhyRegUsage.cpp
  ReduceExecSize.cpp
  RegAlloc.cpp
  SpillCode.cpp
  SpillManagerGMRF.cpp
  TranslationInterface.cpp
  VISAKernelImpl.cpp
  G4Verifier.cpp
  LVN.cpp
  ifcvt.cpp
  PreDefinedVars.cpp
  SpillCleanup.cpp
  Rematerialization.cpp
  RPE.cpp
  ${LocalScheduler_SOURCES}
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
set(CISA_ld_Common_Sources
  CISA_ld/CISA_ld.cpp
  CISA_ld/CISALinker.cpp
  DebugInfo.h
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
  include/VISAOptions.h
  BitSet.cpp
  BitSet.h
  Timer.cpp
  Timer.h
  )

set(GenX_Common_Headers
  BinaryCISAEmission.h
  BinaryEncoding.h
  Common_BinaryEncoding.h
  BinaryEncodingCNL.h
  BinaryEncodingIGA.h
  IGfxHwEuIsaCNL.h
  BuildCISAIR.h
  BuildIR.h
  cm_portability.h
  Common_GEN.h
  DebugInfo.h
  FlowGraph.h
  CFGStructurizer.h
  G4_Opcode.h
  Gen4_IR.hpp
  GraphColor.h
  GTGPU_RT_ASM_Interface.h
  HWConformity.h
  include/JitterDataStruct.h
  SendFusion.h
  LocalRA.h
  Optimizer.h
  PhyRegUsage.h
  RegAlloc.h
  RT_Jitter_Interface.h
  SpillCode.h
  SpillManagerGMRF.h
  include/visa_igc_common_header.h
  include/VISABuilderAPIDefinition.h
  VISAKernel.h
  G4Verifier.h
  LVN.h
  PreDefinedVars.h
  SpillCleanup.h
  Rematerialization.h
  Metadata.h
  RPE.h
  include/gtpin_IGC_interface.h
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
)
set(CISA_ld_Common_Headers
  CISA_ld/CISA_ld.h
  CISA_ld/CISALinker.h
)
set(Jitter_Common_Headers
  Common_ISA.h
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
    EnumFiles.hpp
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

  source_group("Utility Files" FILES ${GenX_IR_EXE_UTILITY} )
  source_group("Header Files" FILES ${GenX_IR_EXE_HEADERS} )
  source_group("Lex Yacc Files" FILES ${GenX_IR_EXE_lex_yacc} )
  target_link_libraries(GenX_IR_Exe IGA_SLIB IGA_ENC_LIB)
  if (ANDROID AND MEDIA_IGA)
     target_link_libraries(GenX_IR_Exe c++_static)
  endif(ANDROID AND MEDIA_IGA)

  if (UNIX AND NOT ANDROID)
    target_link_libraries(GenX_IR_Exe rt dl)
  endif(UNIX AND NOT ANDROID)

     set(GenX_IR_Exe_DEFINITIONS STANDALONE_MODE)

  set_target_properties(GenX_IR_Exe PROPERTIES
          COMPILE_DEFINITIONS "${GenX_IR_Exe_DEFINITIONS}"
          FOLDER CM_JITTER_EXE)
      
  if(IGC_BUILD AND MSVC)
  #set up standard defines from the common WDK path.
  bs_set_wdk(GenX_IR_Exe)
  endif()
  
  if(UNIX)
    install(TARGETS GenX_IR_Exe DESTINATION ${IGC_INSTALL_TIME_ROOT_DIR}/bin COMPONENT igc-media)
  endif(UNIX)

  # Copy executable
  if (NOT IGC_BUILD)
      copy_exe(GenX_IR_Exe)
  endif (NOT IGC_BUILD)

endif(UNIX OR WIN32)
# ###############################################################
# CISA_ld_Exe
# ###############################################################
if (NOT IGC_BUILD)
  if (UNIX OR WIN32)
    set(CISA_ld_EXE_SOURCES
      ${CISA_ld_Common_Sources}
      ${Jitter_Common_Sources}
      )
    set(CISA_ld_EXE_UTILITY
      ${Jitter_Utility_Files}
      )
    set(CISA_ld_EXE_HEADERS
      ${CISA_ld_Common_Headers}
      ${Jitter_Common_Headers}
      ${GenX_Common_Headers}
      )
    setup_executable(CISA_ld_Exe "${CISA_ld_EXE_SOURCES};${CISA_ld_EXE_UTILITY};${CISA_ld_EXE_HEADERS}" FALSE "CISA_ld")
    if (ANDROID AND MEDIA_IGA)
       target_link_libraries(CISA_ld_Exe c++_static)
    endif(ANDROID AND MEDIA_IGA)
    add_dependencies(CISA_ld_Exe check_headers)
    source_group("Header Files" FILES ${CISA_ld_EXE_HEADERS} )
    source_group("Utility Files" FILES ${CISA_ld_EXE_UTILITY} )
    set_target_properties(CISA_ld_Exe PROPERTIES COMPILE_DEFINITIONS STANDALONE_MODE FOLDER CM_JITTER_EXE
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

    # Post build steps
    copy_exe( CISA_ld_Exe )
  endif(UNIX OR WIN32)
endif (NOT IGC_BUILD)

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

if (WIN32 AND NOT IGC_BUILD)
  add_compile_options(/WX)
endif(WIN32 AND NOT IGC_BUILD)

if (IGC_BUILD)
  add_library(GenX_IR STATIC 
       ${GenX_IR_DLL_SOURCES} 
       ${GenX_IR_DLL_UTILITY} 
       ${GenX_IR_DLL_HEADERS}
    )
  set_target_properties( GenX_IR PROPERTIES OUTPUT_NAME "igfxcmjit${TARGET_MODIFIER}")
  if(WIN32)
    target_link_libraries(GenX_IR ${GCC_SECURE_LINK_FLAGS} IGA_ENC_LIB IGA_SLIB)
    add_dependencies(GenX_IR IGA_DLL)
  else()
    target_link_libraries(GenX_IR ${GCC_SECURE_LINK_FLAGS} IGA_ENC_LIB IGA_SLIB)
    add_dependencies(GenX_IR IGA_DLL)
  endif(WIN32)
endif (IGC_BUILD)

if (ANDROID  AND MEDIA_IGA)
   target_link_libraries(GenX_IR c++_static)
endif(ANDROID AND MEDIA_IGA)

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

if(NOT WIN32)
  set_target_properties( GenX_IR PROPERTIES PREFIX "")
endif()

if(IGC_BUILD AND MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(GenX_IR)
endif()

# Copy any required headers
set(headers_to_copy
  include/visaBuilder_interface.h
  include/VISABuilderAPIDefinition.h
  include/visa_igc_common_header.h
  include/JitterDataStruct.h
)

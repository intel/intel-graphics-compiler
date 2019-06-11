# GEDLibrary/GED
if(WIN32)
cmake_minimum_required(VERSION 3.1)
else()
cmake_minimum_required(VERSION 2.8)
endif(WIN32)

project(GEDLibrary)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(PLATFORM_TARGET ia32)
  set(GED_TARGET TARGET_IA32)
else()
  set(PLATFORM_TARGET intel64)
  set(GED_TARGET TARGET_INTEL64)
endif()


# Add the directories that the particular build is going to consider
add_subdirectory(Source/ged/xcoder)
add_subdirectory(Source/common)
add_subdirectory(build/autogen-${PLATFORM_TARGET})

# Set up windows mobile build flags to use dynamic multi-threaded runtime (/MD)
# Set up windows(other) build flags to use static multi-threaded runtime (/MT)
if( WIN32 AND ("${BUILD_WINDOWS_MOBILE}" STREQUAL "TRUE") OR (IGC_OPTION__WINDOWS_MOBILE AND LINK_DLL_IGA) )
      win_dynamic_runtime()
else(WIN32 AND ("${BUILD_WINDOWS_MOBILE}" STREQUAL "TRUE") OR (IGC_OPTION__WINDOWS_MOBILE AND LINK_DLL_IGA))
win_static_runtime()
endif(WIN32 AND ("${BUILD_WINDOWS_MOBILE}" STREQUAL "TRUE") OR (IGC_OPTION__WINDOWS_MOBILE AND LINK_DLL_IGA))

set(GED_files
  ${GED_xcoder_cpp}
  ${GED_xcoder_h}
  ${GED_common_cpp}
  ${GED_common_h}
  ${GED_autogen${PLATFORM_TARGET}_cpp}
  ${GED_autogen${PLATFORM_TARGET}_h})

include_directories(Source/ged Source/common Source "build/autogen-${PLATFORM_TARGET}")
if (NOT IGC_BUILD)
    set(GED_validation_api GED_VALIDATION_API)
endif()

set(GED_definitions_debug _CONSOLE _LIB ${GED_TARGET} GED_DISASSEMBLY GED_DEBUG ${GED_validation_api})
set(GED_definitions _CONSOLE _LIB ${GED_TARGET} GED_DISASSEMBLY ${GED_validation_api})

#Needs to be before add_library otherwise it doesn't get set.
if(MSVC)
        add_compile_options($<$<CONFIG:Release>:/Ox> /Gy)
endif(MSVC)

if (MSVC)
  set(IGA_LINK_FLAGS "/OPT:REF /OPT:ICF /LTCG /INCREMENTAL:NO")
else(MSVC)
  set(IGA_LINK_FLAGS "")
endif(MSVC)

add_library(GEDLibrary OBJECT ${GED_files})

set_property( TARGET GEDLibrary APPEND PROPERTY
    OUTPUT_NAME                                "ged${TARGET_MODIFIER}"
  )
set_property( TARGET GEDLibrary APPEND PROPERTY
    COMPILE_DEFINITIONS                        ${GED_definitions}
  )
set_property( TARGET GEDLibrary APPEND PROPERTY
    COMPILE_DEFINITIONS                        $<$<CONFIG:Debug>:GED_DEBUG>
  )

# hidden visibility suppresses PLT entries from being created for GED and functions and trims the interface down to a minimal size
#Was manifested when IGA was used in GT-PIN which also uses GED. IGA ended up using GED library linked with GT-PIN not IGA one
#https://gcc.gnu.org/wiki/Visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(NOT WIN32)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden"  )
  set_target_properties( GEDLibrary PROPERTIES PREFIX "")
endif()

source_group("SourceFiles/xcoder" FILES ${GED_xcoder_cpp})
source_group("HeaderFiles/xcoder" FILES ${GED_xcoder_h})
source_group("SourceFiles/common" FILES ${GED_common_cpp})
source_group("HeaderFiles/common" FILES ${GED_common_h})
source_group("SourceFiles/auto${PLATFORM_TARGET}" FILES ${GED_autogen${PLATFORM_TARGET}_cpp})
source_group("HeaderFiles/auto${PLATFORM_TARGET}" FILES ${GED_autogen${PLATFORM_TARGET}_h})

# target_include_directories(GEDLibrary PRIVATE include)
if(ANDROID AND MEDIA_IGA)
  set(NDK_Libstdcxx $ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libcxx)
  set(Extra_Android_Inc ${NDK_Libstdcxx}/include ${ANDROID_NDK}/sources/android/support/include
    ${NDK_Libstdcxx}/include/backward)
  include_directories(${Extra_Android_Inc})
endif(ANDROID AND MEDIA_IGA)

#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# GEDLibrary/GED
cmake_minimum_required(VERSION 3.10)

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
if( WIN32 AND ("${BUILD_WINDOWS_MOBILE}" STREQUAL "TRUE") OR (IGC_OPTION__WINDOWS_MOBILE AND LINK_DLL_IGA))
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

set(GED_definitions_hide_enums
    GED_ACCESS_MODE_HIDDEN
    GED_ACC_WR_CTRL_HIDDEN
    GED_ADDR_MODE_HIDDEN
    GED_ARCH_REG_HIDDEN
    GED_ATOMIC_OPERATION_TYPE_HIDDEN
    GED_BLOCK_SIZE_HIDDEN
    GED_BRANCH_CTRL_HIDDEN
    GED_CHANNEL_MASK_HIDDEN
    GED_CHANNEL_MODE_HIDDEN
    GED_CHANNEL_OFFSET_HIDDEN
    GED_COND_MODIFIER_HIDDEN
    GED_DATA_TYPE_HIDDEN
    GED_DEBUG_CTRL_HIDDEN
    GED_DEP_CTRL_HIDDEN
    GED_DST_CHAN_EN_HIDDEN
    GED_EOT_HIDDEN
    GED_EXECUTION_DATA_TYPE_HIDDEN
    GED_EXEC_MASK_OFFSET_CTRL_HIDDEN
    GED_FUSION_CTRL_HIDDEN
    GED_HEADER_PRESENT_HIDDEN
    GED_IS_IMM_HIDDEN
    GED_MASK_CTRL_HIDDEN
    GED_MATH_FC_HIDDEN
    GED_MATH_MACRO_EXT_HIDDEN
    GED_MESSAGE_TYPE_HIDDEN
    GED_MODELS_ARRAY_FUNCTION_HIDDEN
    GED_MODELS_ARRAY_NAMES_HIDDEN
    GED_NO_SRC_DEP_SET_HIDDEN
    GED_OBSCURE_MODEL_NAMES
    GED_OPCODE_HIDDEN
    GED_PRECISION_HIDDEN
    GED_PRED_CTRL_HIDDEN
    GED_PRED_INV_HIDDEN
    GED_REG_FILE_HIDDEN
    GED_REP_CTRL_HIDDEN
    GED_RETURN_DATA_CONTROL_HIDDEN
    GED_SATURATE_HIDDEN
    GED_SFID_HIDDEN
    GED_SIMDMODE_HIDDEN
    GED_SLOT_GROUP_HIDDEN
    GED_SRC_MOD_HIDDEN
    GED_STORE_HINT_HIDDEN
    GED_SUB_BYTE_PRECISION_HIDDEN
    GED_SUB_FUNC_ID_HIDDEN
    GED_SWIZZLE_HIDDEN
    GED_SYNC_FC_HIDDEN
    GED_THREAD_CTRL_HIDDEN)


set(GED_definitions       _CONSOLE _LIB ${GED_TARGET} GED_DISASSEMBLY ${GED_definitions_hide_enums})

# Needs to be before add_library otherwise it doesn't get set.
if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Ox> /Gy)
endif(MSVC)

add_library(GEDLibrary OBJECT ${GED_files})
set_target_properties(GEDLibrary PROPERTIES
    FOLDER                                     "IGAProjs"
    OUTPUT_NAME                                "ged${TARGET_MODIFIER}"
)
set_property(TARGET GEDLibrary APPEND PROPERTY
    COMPILE_DEFINITIONS                        ${GED_definitions}
  )
set_property(TARGET GEDLibrary APPEND PROPERTY
    COMPILE_DEFINITIONS                        $<$<CONFIG:Debug>:GED_DEBUG>
  )

# Hidden visibility suppresses PLT entries from being created for GED and
# functions and trims the interface down to a minimal size
# This was manifested when IGA was used in GT-PIN which also uses GED.
# IGA ended up using GED library linked with GT-PIN not IGA one
# https://gcc.gnu.org/wiki/Visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set_target_properties(GEDLibrary PROPERTIES PREFIX "")
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

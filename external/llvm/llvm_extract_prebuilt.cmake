#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# This code is responsible for extracting prebuilt llvm from downaloaded assets.

include_guard(DIRECTORY)

message(STATUS "Extracting prebuilt LLVM form assets")

find_file(LLVM_ARCHIVE llvm-prebuilt-windows.7z
    PATHS ${DEFAULT_IGC_LLVM_PREBUILDS_DIRS}
    PATH_SUFFIXES "Windows/${IGC_OPTION__LLVM_PREFERRED_VERSION}")

if("${LLVM_ARCHIVE}" STREQUAL "LLVM_ARCHIVE-NOTFOUND")
    message(FATAL_ERROR "LLVM archived asset not found")
endif()

get_filename_component(LLVM_PREBUILT_ASSET_DIR ${LLVM_ARCHIVE} DIRECTORY) # llvm_version
get_filename_component(LLVM_PREBUILT_ASSET_DIR ${LLVM_PREBUILT_ASSET_DIR} DIRECTORY) # windows
get_filename_component(LLVM_PREBUILT_ASSET_DIR ${LLVM_PREBUILT_ASSET_DIR} DIRECTORY) # llvm_prebuilt_windwos

set(ARCHIVE_ASSETVERSION "${LLVM_PREBUILT_ASSET_DIR}/.assetversion")
if(NOT EXISTS ${ARCHIVE_ASSETVERSION})
    message(FATAL_ERROR "LLVM archived asset version file not found in ${LLVM_PREBUILT_ASSET_DIR}")
endif()


set(LLVM_ROOT "${CMAKE_CURRENT_BINARY_DIR}/llvm_prebuilt_windows/${IGC_OPTION__LLVM_PREFERRED_VERSION}")
set(LLVM_ROOT "${LLVM_ROOT}" CACHE PATH "Path to LLVM prebuild")

if(EXISTS ${LLVM_ROOT})
    set(PREBUILT_ASSETVERSION "${LLVM_ROOT}/.assetversion")

    if(EXISTS ${PREBUILT_ASSETVERSION})
        bs_find_python3()
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import filecmp; print(filecmp.cmp('${PREBUILT_ASSETVERSION}','${ARCHIVE_ASSETVERSION}'), end='')"
            OUTPUT_VARIABLE IS_SAME_VERSION
        )

        if(${IS_SAME_VERSION} STREQUAL "True")
            # Prebuilt with same version is already extracted
            return()
        endif()
    endif()
endif()

bs_find_7z()
# extract prebuilts from asset
execute_process(COMMAND ${7Z} x -Y -o${LLVM_ROOT} ${LLVM_ARCHIVE})
# copy asset version file
file(COPY ${ARCHIVE_ASSETVERSION} DESTINATION ${LLVM_ROOT})

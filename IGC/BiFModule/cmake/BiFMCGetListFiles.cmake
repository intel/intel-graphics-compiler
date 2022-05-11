#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

function(get_bif_src_list opencl-h_SRC bifM_SRC listOut)
    # Get all files inside BiFModule and make sha1 of them
    message("[IGC\\BiFModuleCache] - Looking for files inside ${bifM_SRC}")
    file(GLOB_RECURSE files ${bifM_SRC}*.*)
    set(listSRC "")
    foreach(_file ${files})
        list(APPEND listSRC ${_file})
        #message("[IGC\\BiFModuleCache] - File added ${_file}")
    endforeach()

    # Get all opencl-c headers for clang
    get_filename_component(opencl-headers-dir ${opencl-h_SRC} DIRECTORY)
    list(APPEND listSRC ${opencl-headers-dir}/opencl-c-base.h)
    list(APPEND listSRC ${opencl-h_SRC})

    list(SORT listSRC)
    set(${listOut} ${listSRC} PARENT_SCOPE)
endfunction()
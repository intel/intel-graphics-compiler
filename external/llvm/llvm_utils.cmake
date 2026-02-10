#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)


#
# Macro to set python interpreter for LLVM
#
macro(llvm_utils_python_set proj_name)
  # If cached PYTHON_EXECUTABLE already exists save it to restore.
  get_property(PYTHON_EXECUTABLE_BACKUP CACHE PYTHON_EXECUTABLE PROPERTY VALUE)
  # Set python interpreter for LLVM.
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE} CACHE PATH "" FORCE)
  message(STATUS "[${proj_name}] PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
endmacro()

#
# Macro to restore python interpreter
#
macro(llvm_utils_python_restore)
  if(PYTHON_EXECUTABLE_BACKUP)
    # Restore python interpreter.
    set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE_BACKUP} CACHE PATH "" FORCE)
  else()
    # Clear python interpreter from LLVM.
    unset(PYTHON_EXECUTABLE CACHE)
  endif()
endmacro()

#
# Macro to clear build flags that already set
#
macro(llvm_utils_push_build_flags proj_name)
    message(STATUS "[${proj_name}] Clearing build system compilation flags")

    unset(CMAKE_C_FLAGS)
    unset(CMAKE_CXX_FLAGS)
    unset(CMAKE_SHARED_LINKER_FLAGS)
    unset(CMAKE_EXE_LINKER_FLAGS)
    unset(CMAKE_STATIC_LINKER_FLAGS)
    unset(CMAKE_LOCAL_LINKER_FLAGS)
    unset(CMAKE_MODULE_LINKER_FLAGS)

    foreach(configuration_type ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
        string(TOUPPER ${configuration_type} capitalized_configuration_type)

        unset(CMAKE_C_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_CXX_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type})
    endforeach()

    # Disable warnings-as-errors for LLVM target
    if(NOT WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
        message(STATUS "[${proj_name}] Disabling warnings-as-errors for LLVM")
    endif()
endmacro()

# Helper macro to be called from project preparation cmakes.
# It will set correct flags and other variables for LLVM-based projects.
# Parameters:
#  proj_name -- project name
#  src_dir -- source directory that contains project
#  bin_dir -- binary directory for project
macro(build_llvm_project proj_name src_dir bin_dir)
  # Do not alter LLVM compilation flags -- clear them all.
  llvm_utils_push_build_flags(${proj_name})
  llvm_utils_python_set(${proj_name})
  add_subdirectory(${src_dir} ${bin_dir} EXCLUDE_FROM_ALL)
  llvm_utils_python_restore()
endmacro()

# Convenience macro to set option and record its value in list.
macro(set_llvm_opt opt)
  set(${opt} ${ARGN})
  list(APPEND LLVM_OPTIONS "-D${opt}=${${opt}}")
endmacro()

# Helper macro to register LLVM external project.
# proj -- project to register
# source_dir -- sources of project
# RELATIVE_TO_LLVM -- whether project is located in LLVM-project tree
macro(register_llvm_external_project proj source_dir)
  cmake_parse_arguments(ARG "RELATIVE_TO_LLVM" "" "" ${ARGN})
  if(ARG_RELATIVE_TO_LLVM)
    set_property(GLOBAL APPEND PROPERTY IGC_LLVM_PROJECT_${proj}_RELATIVE YES)
  endif()
  set_property(GLOBAL APPEND PROPERTY IGC_LLVM_PROJECTS ${proj})
  set_property(GLOBAL PROPERTY IGC_LLVM_PROJECT_${proj}_DIR ${source_dir})
endmacro()

# Get registered projects.
macro(get_llvm_external_projects out)
  get_property(${out} GLOBAL PROPERTY IGC_LLVM_PROJECTS)
endmacro()

# Get source dir for LLVM project.
# proj -- project to get directory for
# out -- output variable
# BASE_DIR dir -- if project is RELATIVE_TO_LLVM then use "dir" as a base
macro(get_llvm_external_project_dir proj out)
  cmake_parse_arguments(ARG "" "BASE_DIR" "" ${ARGN})
  get_property(${out} GLOBAL PROPERTY IGC_LLVM_PROJECT_${proj}_DIR)
  get_property(is_rel GLOBAL PROPERTY IGC_LLVM_PROJECT_${proj}_RELATIVE)
  if(is_rel AND ARG_BASE_DIR)
    get_filename_component(${out} ${${out}} ABSOLUTE BASE_DIR ${ARG_BASE_DIR})
  endif()
endmacro()

# Helper function to define and check build mode variable for project.
# proj_name -- name of project to print in messages.
# var -- output variable that should be defined.
macro(llvm_define_mode_variable proj_name var)
  set(SOURCE_MODE_NAME "Source")
  set(PREBUILDS_MODE_NAME "Prebuilds")

  if(${var} AND NOT (${var} MATCHES "^(${SOURCE_MODE_NAME}|${PREBUILDS_MODE_NAME})$"))
    message(FATAL_ERROR "${proj_name} mode can be only ${SOURCE_MODE_NAME}, ${PREBUILDS_MODE_NAME} or empty!")
  endif()
endmacro()

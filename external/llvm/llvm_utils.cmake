#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2018-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)


#
# Macro to set python interpreter for LLVM
#
macro(llvm_utils_python_set)
  # If cached PYTHON_EXECUTABLE already exists save it to restore.
  get_property(PYTHON_EXECUTABLE_BACKUP CACHE PYTHON_EXECUTABLE PROPERTY VALUE)
  # Set python interpreter for LLVM.
  set(PYTHON_EXECUTABLE ${PYTHON} CACHE PATH "" FORCE)
  message(STATUS "[LLVM] PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
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
macro(llvm_utils_push_build_flags)
    message(STATUS "[LLVM] Clearing build system compilation flags")

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
endmacro()

# Convenience macro to set option and record its value in list.
macro(set_llvm_opt opt)
  set(${opt} ${ARGN})
  list(APPEND LLVM_OPTIONS "-D${opt}=${${opt}}")
endmacro()


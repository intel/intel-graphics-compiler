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

# Macro to set python interpreter for LLVM.
macro(llvm_utils_python_set)
  # If cached PYTHON_EXECUTABLE already exists save it to restore.
  get_property(PYTHON_EXECUTABLE_BACKUP CACHE PYTHON_EXECUTABLE PROPERTY VALUE)
  # Set python interpreter for LLVM.
  set(PYTHON_EXECUTABLE ${PYTHON} CACHE PATH "" FORCE)
  message(STATUS "[LLVM] PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
endmacro()


# Macro to restore python interpreter.
macro(llvm_utils_python_restore)
  if(PYTHON_EXECUTABLE_BACKUP)
    # Restore python interpreter.
    set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE_BACKUP} CACHE PATH "" FORCE)
  else()
    # Clear python interpreter from LLVM.
    unset(PYTHON_EXECUTABLE CACHE)
  endif()
endmacro()

# Macro to set build flags for LLVM.
macro(llvm_utils_set_build_flags)

  set(cfgs ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
  list(TRANSFORM cfgs TOUPPER)
  list(TRANSFORM cfgs PREPEND "_")
  list(APPEND cfgs "")

  foreach(cfg IN LISTS cfgs)
    foreach(flag_type "C" "CXX")
      set(CMAKE_${flag_type}_FLAGS${cfg} "${llvm_lang_flags}")
    endforeach()
    foreach(flag_type "SHARED_LINKER" "EXE_LINKER" "STATIC_LINKER" "MODULE_LINKER")
      set(CMAKE_${flag_type}_FLAGS${cfg} "${llvm_link_flags}")
    endforeach()
  endforeach()
endmacro()

# Convenience macro to set option and record its value in list.
macro(set_llvm_opt opt)
  set(${opt} ${ARGN})
  list(APPEND LLVM_OPTIONS "-D${opt}=${${opt}}")
endmacro()


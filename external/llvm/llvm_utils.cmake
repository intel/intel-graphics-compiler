#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice ==================================


#
# Macro to set python interpreter for LLVM
#
macro(llvm_utils_python_set)
    # Unset directory scope PYTHON_EXECUTABLE variable
    unset(PYTHON_EXECUTABLE)
    # Check for cached PYTHON_EXECUTABLE variable
    if(PYTHON_EXECUTABLE)
        # If cached PYTHON_EXECUTABLE already exists save it to restore
        set(PYTHON_EXECUTABLE_BACKUP ${PYTHON_EXECUTABLE})
    endif()
    # Set python interpreter for LLVM
    set(PYTHON_EXECUTABLE ${PYTHON} CACHE PATH "desc" FORCE)
    message(STATUS "[LLVM] PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")	
endmacro()

#
# Macro to restore python interpreter
#
macro(llvm_utils_python_restore)
    if(PYTHON_EXECUTABLE_BACKUP)
        # Restore python interpreter
        set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE_BACKUP} CACHE PATH "desc" FORCE)
    else()
        # Clear python interpreter for LLVM
        unset(PYTHON_EXECUTABLE CACHE)
    endif()
endmacro()

#
# Macro to clear and backup build flags already set
#
macro(llvm_utils_push_build_flags)
    message(STATUS "[LLVM] Clearing build system compilation flags")

    set(CMAKE_C_FLAGS_BACKUP ${CMAKE_C_FLAGS})
    set(CMAKE_CXX_FLAGS_BACKUP ${CMAKE_CXX_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS_BACKUP ${CMAKE_SHARED_LINKER_FLAGS})
    set(CMAKE_EXE_LINKER_FLAGS_BACKUP ${CMAKE_EXE_LINKER_FLAGS})
    set(CMAKE_STATIC_LINKER_FLAGS_BACKUP ${CMAKE_STATIC_LINKER_FLAGS})
    set(CMAKE_LOCAL_LINKER_FLAGS_BACKUP ${CMAKE_LOCAL_LINKER_FLAGS})
    set(CMAKE_MODULE_LINKER_FLAGS_BACKUP ${CMAKE_MODULE_LINKER_FLAGS})

    if(PRINT_DEBUG)
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_C_FLAGS             = ${CMAKE_C_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_CXX_FLAGS           = ${CMAKE_CXX_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_EXE_LINKER_FLAGS    = ${CMAKE_EXE_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_STATIC_LINKER_FLAGS = ${CMAKE_STATIC_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_LOCAL_LINKER_FLAGS  = ${CMAKE_LOCAL_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_MODULE_LINKER_FLAGS = ${CMAKE_MODULE_LINKER_FLAGS}")
    endif()

    unset(CMAKE_C_FLAGS)
    unset(CMAKE_CXX_FLAGS)
    unset(CMAKE_SHARED_LINKER_FLAGS)
    unset(CMAKE_EXE_LINKER_FLAGS)
    unset(CMAKE_STATIC_LINKER_FLAGS)
    unset(CMAKE_LOCAL_LINKER_FLAGS)
    unset(CMAKE_MODULE_LINKER_FLAGS)

    foreach(configuration_type ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
        string(TOUPPER ${configuration_type} capitalized_configuration_type)

        set(CMAKE_C_FLAGS_${capitalized_configuration_type}_BACKUP ${CMAKE_C_FLAGS_${capitalized_configuration_type}})
        set(CMAKE_CXX_FLAGS_${capitalized_configuration_type}_BACKUP ${CMAKE_CXX_FLAGS_${capitalized_configuration_type}})
        set(CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP ${CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type}}) 
        set(CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP ${CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}})
        set(CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP ${CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type}})
        set(CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP ${CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type}})

        if(PRINT_DEBUG)
            message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_C_FLAGS_${capitalized_configuration_type}             = ${CMAKE_C_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_CXX_FLAGS_${capitalized_configuration_type}           = ${CMAKE_CXX_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type} = ${CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}    = ${CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type} = ${CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_push_build_flags() CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type} = ${CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type}}")
        endif()

        unset(CMAKE_C_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_CXX_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type})
        unset(CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type})

    endforeach()

endmacro()

#
# Macro to restore build flags set previously
#
macro(llvm_utils_pop_build_flags)
    message(STATUS "[LLVM] Restoring build system compilation flags")

    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS_BACKUP})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_BACKUP})
    set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_BACKUP}) 
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS_BACKUP})
    set(CMAKE_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS_BACKUP})
    set(CMAKE_LOCAL_LINKER_FLAGS ${CMAKE_LOCAL_LINKER_FLAGS_BACKUP})
    set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_BACKUP})

    if(PRINT_DEBUG)
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_C_FLAGS             = ${CMAKE_C_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_CXX_FLAGS           = ${CMAKE_CXX_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_EXE_LINKER_FLAGS    = ${CMAKE_EXE_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_STATIC_LINKER_FLAGS = ${CMAKE_STATIC_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_LOCAL_LINKER_FLAGS  = ${CMAKE_LOCAL_LINKER_FLAGS}")
        message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_MODULE_LINKER_FLAGS = ${CMAKE_MODULE_LINKER_FLAGS}")
    endif()

    unset(CMAKE_C_FLAGS_BACKUP)
    unset(CMAKE_CXX_FLAGS_BACKUP)
    unset(CMAKE_SHARED_LINKER_FLAGS_BACKUP)
    unset(CMAKE_EXE_LINKER_FLAGS_BACKUP)
    unset(CMAKE_STATIC_LINKER_FLAGS_BACKUP)
    unset(CMAKE_LOCAL_LINKER_FLAGS_BACKUP)
    unset(CMAKE_MODULE_LINKER_FLAGS_BACKUP)

    foreach(configuration_type ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
        string(TOUPPER ${configuration_type} capitalized_configuration_type)

        set(CMAKE_C_FLAGS_${capitalized_configuration_type} ${CMAKE_C_FLAGS_${capitalized_configuration_type}_BACKUP})
        set(CMAKE_CXX_FLAGS_${capitalized_configuration_type} ${CMAKE_CXX_FLAGS_${capitalized_configuration_type}_BACKUP})
        set(CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type} ${CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP}) 
        set(CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type} ${CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP})
        set(CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type} ${CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP})
        set(CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type} ${CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP})

        if(PRINT_DEBUG)
            message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_C_FLAGS_${capitalized_configuration_type}             = ${CMAKE_C_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_CXX_FLAGS_${capitalized_configuration_type}           = ${CMAKE_CXX_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type} = ${CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}    = ${CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type} = ${CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type}}")
            message(STATUS "[LLVM] llvm_utils_pop_build_flags() CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type} = ${CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type}}")
        endif()

        unset(CMAKE_C_FLAGS_${capitalized_configuration_type}_BACKUP)
        unset(CMAKE_CXX_FLAGS_${capitalized_configuration_type}_BACKUP)
        unset(CMAKE_SHARED_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP)
        unset(CMAKE_EXE_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP)
        unset(CMAKE_STATIC_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP)
        unset(CMAKE_MODULE_LINKER_FLAGS_${capitalized_configuration_type}_BACKUP)

    endforeach()

endmacro()


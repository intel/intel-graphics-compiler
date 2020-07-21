
# TODO: now we have (historically) two name definitions:
# CMFE_WRAPPER_NAME is used to get a name of a pre-built library (if any)
# INSTALL_CMFE_NAME denotes the final name, which is used as a destination name
# Looks like we can have only one (CMFE_WRAPPER_NAME to denote the name of an existing library)

set(ADAPTOR_CM_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
if (WIN32 OR CYGWIN)
  set(CMFE_WRAPPER_NAME "clangFEWrapper.dll")
else()
  set(CMFE_WRAPPER_NAME "libclangFEWrapper.so")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_cpuSuffix "64")
else()
  set(_cpuSuffix "32")
endif()

if(WIN32)
  set(INSTALL_CMFE_NAME "clangFEWrapper.dll")
  if("${_cpuSuffix}" STREQUAL "32")
    set(INSTALL_CMFE_NAME "clangFEWrapper32.dll")
  endif()
else(WIN32)
  set(INSTALL_CMFE_NAME "${CMFE_WRAPPER_NAME}")
endif(WIN32)


add_subdirectory(AdaptorCM)


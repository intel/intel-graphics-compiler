
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include(cmfe_deps)

message(STATUS
  "[CMFE] CM Frontend headers are taken from: ${CMFE_INCLUDE_DIRS}")
message(STATUS
  "[CMFE] Using CMFE library name: ${INSTALL_CMFE_NAME}")

include(${IGC_SOURCE_DIR}/cmake/utils.cmake)
add_subdirectory(AdaptorCM)

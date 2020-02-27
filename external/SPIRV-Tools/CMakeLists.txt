cmake_minimum_required(VERSION 2.8.12)

message(STATUS "============================ SPIRV-Tools project ============================")

set(SPIRV-Headers_SOURCE_DIR "${BS_DIR_EXTERNAL_COMPONENTS}/external/SPIRV-Headers") # used in subdirectory
set(SPIRV-Tools_SOURCE_DIR "${BS_DIR_EXTERNAL_COMPONENTS}/external/SPIRV-Tools")
set(SPIRV-Tools_OUTPUT_DIR "${IGC_OPTION__OUTPUT_DIR}/external/SPIRV-Tools/build")
set(IGC_BUILD__SPIRV-Headers_DIR "${SPIRV-Headers_SOURCE_DIR}" PARENT_SCOPE)
set(IGC_BUILD__SPIRV-Tools_DIR "${SPIRV-Tools_SOURCE_DIR}" PARENT_SCOPE)

# options used in subdirectory
option(SKIP_SPIRV_TOOLS_INSTALL "Skip SPIRV-Tools installation" ON)
option(SPIRV_SKIP_TESTS "Skip building SPIRV-Tools tests along with the library" ON)
option(SPIRV_SKIP_EXECUTABLES "Skip building SPIRV-Tools executables and tests along with the library" ON)
option(SPIRV_HEADERS_SKIP_INSTALL "Skip SPIRV-Headers installation" ON)
option(SPIRV_HEADERS_SKIP_EXAMPLES "Skip building SPIRV-Headers examples" ON)

message(STATUS "SPIRV-Tools source directory: ${SPIRV-Tools_SOURCE_DIR}")
message(STATUS "SPIRV-Headers source directory: ${SPIRV-Headers_SOURCE_DIR}")
message(STATUS "SPIRV-Tools output directory: ${SPIRV-Tools_OUTPUT_DIR}")
message(STATUS "Option SKIP_SPIRV_TOOLS_INSTALL: ${SKIP_SPIRV_TOOLS_INSTALL}")
message(STATUS "Option SPIRV_SKIP_TESTS: ${SPIRV_SKIP_TESTS}")
message(STATUS "Option SPIRV_SKIP_EXECUTABLES: ${SPIRV_SKIP_EXECUTABLES}")
message(STATUS "Option SPIRV_HEADERS_SKIP_INSTALL: ${SPIRV_HEADERS_SKIP_INSTALL}")
message(STATUS "Option SPIRV_HEADERS_SKIP_EXAMPLES: ${SPIRV_HEADERS_SKIP_EXAMPLES}")

add_subdirectory("${SPIRV-Tools_SOURCE_DIR}" "${SPIRV-Tools_OUTPUT_DIR}" EXCLUDE_FROM_ALL)
set(IGC_BUILD__PROJ__SPIRV-Tools "SPIRV-Tools" PARENT_SCOPE) # same name as in subdirectory
set_target_properties("SPIRV-Tools" PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_RELEASE TRUE)

message(STATUS "=============================================================================")

set(BACKEND_PLUGIN_SOURCES
  BackendPlugin.cpp
  )

add_library(VCBackendPlugin
  MODULE
  ${BACKEND_PLUGIN_SOURCES}
  )

# Hack to avoid transitive LLVM dependencies that will break
# plugin because of duplicate global variables.
# 'CODEGEN_LIBS' will consist of 'VCCodeGen' and all its direct dependencies.
# This should be enough for now. In case of indirect dependencies on VectorCompiler
# libraries, searching algorithm needs to be improved.
# 'CODEGEN_LIBS_FILES' will be expanded to plain library names so cmake
# will not add any transitive dependencies when target is linked against them.
get_target_property(CODEGEN_LIBS VCCodeGen LINK_LIBRARIES)
set(CODEGEN_LIBS VCCodeGen ${CODEGEN_LIBS})
foreach(target ${CODEGEN_LIBS})
  # Filter out interface libraries -- these will not produce any files.
  get_target_property(TARGET_TYPE ${target} TYPE)
  if(NOT ("${TARGET_TYPE}" STREQUAL "INTERFACE_LIBRARY"))
    set(CODEGEN_LIBS_FILES ${CODEGEN_LIBS_FILES} "$<TARGET_FILE:${target}>")
  endif()
endforeach()

# Cmake also does not add any dependencies for libraries
# that are linked this way.
add_dependencies(VCBackendPlugin
  ${CODEGEN_LIBS}
  )

target_link_libraries(VCBackendPlugin
  PRIVATE
  VCHeaders
  ${CODEGEN_LIBS_FILES}
  # GenX_IR actually should be linked to LLVMGenXCodeGen.
  GenX_IR
  )

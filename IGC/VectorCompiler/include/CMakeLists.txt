# Special common target for headers that propagates
# needed include directories and dependencies.
add_library(VCHeaders INTERFACE)

target_include_directories(VCHeaders
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

add_dependencies(VCHeaders
  intrinsics_gen
  GenXIntrinsicsGen
  VectorCompilerOptions
  )

# Additional things like header generators.
add_subdirectory(vc)

# Special common target for headers that propagates
# needed include directories and dependencies.
add_library(VCHeaders INTERFACE)

target_include_directories(VCHeaders
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

target_link_libraries(VCHeaders
  INTERFACE
  VCIntrinsicsHeaders
  )

add_dependencies(VCHeaders
  intrinsics_gen
  VectorCompilerOptions
  )

# Additional things like header generators.
add_subdirectory(vc)

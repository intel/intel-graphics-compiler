set(SUPPORT_SOURCES
  Status.cpp
  ShaderDump.cpp
  BackendConfig.cpp
  )

add_library(VCSupport ${SUPPORT_SOURCES})
igc_get_llvm_targets(LLVM_LIBS
  Support
  Core
  )
target_link_libraries(VCSupport
  VCHeaders

  ${LLVM_LIBS}
  )

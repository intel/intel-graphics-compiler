set(SUPPORT_SOURCES
  Options.cpp
  )

add_library(VCOptions ${SUPPORT_SOURCES})
igc_get_llvm_targets(LLVM_LIBS
  Option
  )
target_link_libraries(VCOptions
  VCHeaders

  ${LLVM_LIBS}
  )

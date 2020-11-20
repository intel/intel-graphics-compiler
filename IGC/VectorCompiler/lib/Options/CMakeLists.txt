set(SUPPORT_SOURCES
  Options.cpp
  )

add_library(VCOptions ${SUPPORT_SOURCES})
vc_get_llvm_targets(LLVM_LIBS
  LLVMOption
  )
target_link_libraries(VCOptions
  VCHeaders

  ${LLVM_LIBS}
  )

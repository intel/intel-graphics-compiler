set(SUPPORT_SOURCES
  Status.cpp
  Options.cpp
  )

add_library(VCSupport ${SUPPORT_SOURCES})
vc_get_llvm_targets(LLVM_LIBS
  LLVMSupport
  LLVMOption
  )
target_link_libraries(VCSupport
  VCHeaders

  ${LLVM_LIBS}
  )

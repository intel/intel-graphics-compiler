set(SUPPORT_SOURCES
  Status.cpp
  ShaderDump.cpp
  )

add_library(VCSupport ${SUPPORT_SOURCES})
vc_get_llvm_targets(LLVM_LIBS
  LLVMSupport
  )
target_link_libraries(VCSupport
  VCHeaders

  ${LLVM_LIBS}
  )

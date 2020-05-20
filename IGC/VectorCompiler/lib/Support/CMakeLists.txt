set(SUPPORT_SOURCES
  Status.cpp
  Options.cpp
  )

add_library(VCSupport ${SUPPORT_SOURCES})
target_link_libraries(VCSupport
  VCHeaders
  LLVMSupport
  LLVMOption
  )

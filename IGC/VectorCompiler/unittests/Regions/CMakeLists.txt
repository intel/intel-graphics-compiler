set(LLVM_LINK_COMPONENTS
  Core
  Support
  CodeGen
  GenXCodeGen
  GenXOpts
  )

add_genx_unittest(RegionsTests
  OverlapTest.cpp
  )


target_include_directories(RegionsTests PRIVATE  "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/GenXCodeGen")
target_link_libraries(RegionsTests PRIVATE LLVMTestingSupport)



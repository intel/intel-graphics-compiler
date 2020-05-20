set(LLVM_LINK_COMPONENTS
  Core
  Support
  CodeGen
  GenXIntrinsics
  SPIRVLib
  )

add_genx_unittest(SPIRVConversionsTests
  SPIRVConversionsTest.cpp
  )

target_include_directories(SPIRVConversionsTests PRIVATE ${SPIRV_INCLUDE_DIR})
target_link_libraries(SPIRVConversionsTests PRIVATE LLVMTestingSupport)



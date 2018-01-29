include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__CorrectlyRoundedDivSqrt
    "${CMAKE_CURRENT_SOURCE_DIR}/CorrectlyRoundedDivSqrt.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_CorrectlyRoundedDivSqrt ${IGC_BUILD__SRC__CorrectlyRoundedDivSqrt} PARENT_SCOPE)

set(IGC_BUILD__HDR__CorrectlyRoundedDivSqrt
    "${CMAKE_CURRENT_SOURCE_DIR}/CorrectlyRoundedDivSqrt.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_CorrectlyRoundedDivSqrt ${IGC_BUILD__HDR__CorrectlyRoundedDivSqrt} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_CorrectlyRoundedDivSqrt
    "CorrectlyRoundedDivSqrt"
    FILES
      ${IGC_BUILD__SRC__CorrectlyRoundedDivSqrt}
      ${IGC_BUILD__HDR__CorrectlyRoundedDivSqrt}
  )
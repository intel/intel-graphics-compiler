include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__SetFastMathFlags
    "${CMAKE_CURRENT_SOURCE_DIR}/SetFastMathFlags.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SetFastMathFlags ${IGC_BUILD__SRC__SetFastMathFlags} PARENT_SCOPE)

set(IGC_BUILD__HDR__SetFastMathFlags
    "${CMAKE_CURRENT_SOURCE_DIR}/SetFastMathFlags.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SetFastMathFlags ${IGC_BUILD__HDR__SetFastMathFlags} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_SetFastMathFlags
    "SetFastMathFlags"
    FILES
      ${IGC_BUILD__SRC__SetFastMathFlags}
      ${IGC_BUILD__HDR__SetFastMathFlags}
  )
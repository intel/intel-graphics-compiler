include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__StatelessToStatefull
    "${CMAKE_CURRENT_SOURCE_DIR}/StatelessToStatefull.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_StatelessToStatefull ${IGC_BUILD__SRC__StatelessToStatefull} PARENT_SCOPE)

set(IGC_BUILD__HDR__StatelessToStatefull
    "${CMAKE_CURRENT_SOURCE_DIR}/StatelessToStatefull.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_StatelessToStatefull ${IGC_BUILD__HDR__StatelessToStatefull} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_StatelessToStatefull
    "StatelessToStatefull"
    FILES
      ${IGC_BUILD__SRC__StatelessToStatefull}
      ${IGC_BUILD__HDR__StatelessToStatefull}
  )
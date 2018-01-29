include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__OpenGLPasses
    "${CMAKE_CURRENT_SOURCE_DIR}/GlobalToLocal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteBoolAlloca.cpp"
  )
set(IGC_BUILD__SRC__Optimizer_OpenGLPasses ${IGC_BUILD__SRC__OpenGLPasses} PARENT_SCOPE)

set(IGC_BUILD__HDR__OpenGLPasses
    "${CMAKE_CURRENT_SOURCE_DIR}/GlobalToLocal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteBoolAlloca.h"
  )
set(IGC_BUILD__HDR__Optimizer_OpenGLPasses ${IGC_BUILD__HDR__OpenGLPasses} PARENT_SCOPE)


igc_sg_register(
    Compiler__Optimizer_OpenGLPasses
    "OpenGLPasses"
    FILES
      ${IGC_BUILD__SRC__OpenGLPasses}
      ${IGC_BUILD__HDR__OpenGLPasses}
  )
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__DebugInfo "${CMAKE_CURRENT_SOURCE_DIR}/ScalarVISAModule.cpp")
set(IGC_BUILD__SRC__Compiler_DebugInfo ${IGC_BUILD__SRC__DebugInfo} PARENT_SCOPE)

set(IGC_BUILD__HDR__DebugInfo "${CMAKE_CURRENT_SOURCE_DIR}/ScalarVISAModule.h")
set(IGC_BUILD__HDR__Compiler_DebugInfo ${IGC_BUILD__HDR__DebugInfo} PARENT_SCOPE)

igc_sg_register(
    Compiler__DebugInfo
    "DebugInfo"
    FILES
      ${IGC_BUILD__SRC__DebugInfo}
      ${IGC_BUILD__HDR__DebugInfo}
  )


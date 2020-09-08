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

set(DEBUG_INFO_LIBRARY_SOURCES
  StreamEmitter.cpp
  DIE.cpp
  DwarfCompileUnit.cpp
  LexicalScopes.cpp
  DwarfDebug.cpp
  VISADebugEmitter.cpp
  VISAModule.cpp
)

add_library(LibDebugInfo STATIC ${DEBUG_INFO_LIBRARY_SOURCES})
add_dependencies(LibDebugInfo intrinsics_gen ${IGC_BUILD__PROJ__GenISAIntrinsics})

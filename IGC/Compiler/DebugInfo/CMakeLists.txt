include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DebugInfo
    "${CMAKE_CURRENT_SOURCE_DIR}/DIE.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DwarfCompileUnit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DwarfDebug.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LexicalScopes.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/StreamEmitter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/VISADebugEmitter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/VISAModule.cpp"
  )
set(IGC_BUILD__SRC__Compiler_DebugInfo ${IGC_BUILD__SRC__DebugInfo} PARENT_SCOPE)

set(IGC_BUILD__HDR__DebugInfo
    "${CMAKE_CURRENT_SOURCE_DIR}/DebugInfoUtils.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DIE.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DwarfCompileUnit.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DwarfDebug.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LexicalScopes.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/StreamEmitter.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/VISADebugEmitter.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/VISAIDebugEmitter.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/VISAModule.hpp"
  )
set(IGC_BUILD__HDR__Compiler_DebugInfo ${IGC_BUILD__HDR__DebugInfo} PARENT_SCOPE)


igc_sg_register(
    Compiler__DebugInfo
    "DebugInfo"
    FILES
      ${IGC_BUILD__SRC__DebugInfo}
      ${IGC_BUILD__HDR__DebugInfo}
  )
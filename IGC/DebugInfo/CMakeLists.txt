set(DEBUG_INFO_LIBRARY_SOURCES
  StreamEmitter.cpp
  DIE.cpp
  DwarfCompileUnit.cpp
  LexicalScopes.cpp
  DwarfDebug.cpp
  VISADebugEmitter.cpp
  VISADebugDecoder.cpp
  VISAModule.cpp
)

add_library(GenXDebugInfo STATIC ${DEBUG_INFO_LIBRARY_SOURCES})
add_dependencies(GenXDebugInfo intrinsics_gen ${IGC_BUILD__PROJ__GenISAIntrinsics})

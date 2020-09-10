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

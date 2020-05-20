add_subdirectory(GenXOpts)
add_subdirectory(GenXCodeGen)
add_subdirectory(Support)

# Plugin support.
# Only for linux.
if(LLVM_ON_UNIX)
  add_subdirectory(BackendPlugin)
endif()

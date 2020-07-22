# Convenience function to get list of LLVM components for
# target_link_library. If LLVM was configured with llvm dylib, then
# included in dylib llvm targets should be replaced with LLVM
# lib. Otherwise, just return passed libraries.
# ret -- name of variable with returned targets list. All other
# arguments are targets to process.
function(vc_get_llvm_targets RET)
  set(TARGETS ${ARGN})
  if (LLVM_LINK_LLVM_DYLIB)
    if ("${LLVM_DYLIB_COMPONENTS}" STREQUAL "all")
      set(TARGETS "")
    else()
      list(REMOVE_ITEM TARGETS ${LLVM_DYLIB_COMPONENTS})
    endif()
    set(TARGETS ${TARGETS} LLVM)
  endif()
  set(${RET} ${TARGETS} PARENT_SCOPE)
endfunction()

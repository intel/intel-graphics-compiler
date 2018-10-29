include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(4.0)


set(IGC_BUILD__SRC__IGCInstCombiner
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineAddSub.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineAndOrXor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineCalls.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineCasts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineCompares.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineLoadStoreAlloca.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineMulDivRem.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombinePHI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineSelect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineShifts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineSimplifyDemanded.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineVectorOps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstructionCombining.cpp"
  )
set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner 
${IGC_BUILD__SRC__IGCInstCombiner_4_0} 
PARENT_SCOPE)

set(IGC_BUILD__HDR__IGCInstCombiner
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineInternal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCInstructionCombining.hpp"
  )
set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner 
${IGC_BUILD__HDR__IGCInstCombiner_4_0} 
PARENT_SCOPE)


igc_sg_register(
    Compiler__Optimizer_IGCInstCombiner
    "IGCInstCombiner"
    GROUPS
	Optimizer__IGCInstCombiner_4_0
    FILES
#      ${IGC_BUILD__SRC__IGCInstCombiner}
#      ${IGC_BUILD__HDR__IGCInstCombiner}
  )

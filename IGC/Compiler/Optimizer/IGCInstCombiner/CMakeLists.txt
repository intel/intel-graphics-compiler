include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(LLVM_VERSION_MAJOR EQUAL 4)
  add_subdirectory(4.0)
  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner ${IGC_BUILD__SRC__IGCInstCombiner_4_0} PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner ${IGC_BUILD__HDR__IGCInstCombiner_4_0} PARENT_SCOPE)
elseif(LLVM_VERSION_MAJOR EQUAL 7)
  add_subdirectory(7.0)
  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner ${IGC_BUILD__SRC__IGCInstCombiner_7_0} PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner ${IGC_BUILD__HDR__IGCInstCombiner_7_0} PARENT_SCOPE)
elseif(LLVM_VERSION_MAJOR EQUAL 8)
  add_subdirectory(8.0)
  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner ${IGC_BUILD__SRC__IGCInstCombiner_8_0} PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner ${IGC_BUILD__HDR__IGCInstCombiner_8_0} PARENT_SCOPE)
else()
  message(FATAL_ERROR "Error: unsupported version of LLVM: ${LLVM_VERSION_MAJOR}.")
endif()

set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner
    "${IGC_BUILD__HDR__Optimizer_IGCInstCombiner}"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCInstructionCombining.h"
    )

if(LLVM_VERSION_MAJOR EQUAL 4 OR
   LLVM_VERSION_MAJOR EQUAL 7 OR
   LLVM_VERSION_MAJOR EQUAL 8)
igc_sg_register(
    Compiler__Optimizer_IGCInstCombiner
    "IGCInstCombiner"
    GROUPS
      Optimizer__IGCInstCombiner_4_0
      Optimizer__IGCInstCombiner_7_0
      Optimizer__IGCInstCombiner_8_0
    FILES
  )
endif()
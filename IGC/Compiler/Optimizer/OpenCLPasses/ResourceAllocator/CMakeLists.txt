include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ResourceAllocator
    "${CMAKE_CURRENT_SOURCE_DIR}/ResourceAllocator.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ResourceAllocator ${IGC_BUILD__SRC__ResourceAllocator} PARENT_SCOPE)

set(IGC_BUILD__HDR__ResourceAllocator
    "${CMAKE_CURRENT_SOURCE_DIR}/ResourceAllocator.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ResourceAllocator ${IGC_BUILD__HDR__ResourceAllocator} PARENT_SCOPE)
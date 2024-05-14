
set(IGC_BUILD__PROJ__PCH        "${IGC_BUILD__PROJ_NAME_PREFIX}PCH")
set(IGC_BUILD__PROJ__PCH        "${IGC_BUILD__PROJ__PCH}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__PCH  "${IGC_BUILD__PROJ__PCH}")

set(PCH_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/llvm_dummy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llvm_dummy.h
)
if(MSVC)
    add_compile_options(/w14701) #Unintialized Variable Warning
endif()

add_library(${IGC_BUILD__PROJ__PCH} STATIC ${PCH_FILES})

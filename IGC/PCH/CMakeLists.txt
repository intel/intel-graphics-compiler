set(PCH_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/llvm.h
)

add_custom_target(PCH SOURCES ${PCH_FILES})

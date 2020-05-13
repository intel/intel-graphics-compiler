add_subdirectory ("include")
add_subdirectory ("lib")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(zebinlib STATIC ${ZE_INFO_SOURCE_FILE} ${ZE_INFO_INCLUDE_FILE})

if(MSVC)
    set(ZEBIN_COMPILER_OPTIONS
        /w44141
        /w44146
        /w44244
    )
    target_compile_options(zebinlib PRIVATE ${ZEBIN_COMPILER_OPTIONS})
endif()

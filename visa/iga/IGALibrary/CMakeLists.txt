##############################################
##############################################
# IGA dynamic and static library generation

set (CMAKE_CXX_STANDARD 14)

# add GED libraries and includes
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(GED_INCLUDE "../GEDLibrary/${GED_BRANCH}/build/autogen-ia32")
else()
  set(GED_INCLUDE "../GEDLibrary/${GED_BRANCH}/build/autogen-intel64")
endif()
include_directories(${GED_INCLUDE} "../GEDLibrary/${GED_BRANCH}/Source/common")

# Add subdirectories with all the various IGA sources
add_subdirectory(api)
add_subdirectory(Backend)
add_subdirectory(Frontend)
add_subdirectory(IR)
add_subdirectory(MemManager)
add_subdirectory(Models)
add_subdirectory(Timer)

if (LINK_AS_STATIC_LIB)
  win_static_runtime()
else (LINK_AS_STATIC_LIB)
  win_dynamic_runtime()
endif (LINK_AS_STATIC_LIB)

# Mimick IGC/MDF warnings within IGA library on our side to ensure we don't
# break the build.
if(MSVC_GENERATOR)
  add_compile_options(/w14189)
  add_compile_options(/w14062)
  add_compile_options(/WX)
endif()

if(IGC_BUILD)
    get_property(TEST_POP DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)

    foreach(arg ${TEST_POP})
       if(${arg} MATCHES "_HAS_EXCEPTIONS=0")
        set(bar "${bar} _HAS_EXCEPTIONS=1")
       else()
        set(bar "${bar} ${arg}")
       endif()
     endforeach(arg ${foo})

    if(MSVC)
      add_compile_options(/EHsc)
    else(MSVC)
      add_compile_options(-fexceptions)
    endif(MSVC)

    string(REPLACE " " ";" TEST_POP ${bar})

    set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS
        COMPILE_DEFINITIONS                        "${TEST_POP}"
      )
endif(IGC_BUILD)

if (MSVC)
  # /GS- is a subset of sdl-
  # "$<$<CONFIG:Release>:/GL>" - causes IGS windows release build failure. Need to look in to why later.
  add_compile_options(/sdl- $<$<CONFIG:Release>:/Ox> /Gy)
endif (MSVC)

##############################################################################
# IGA (dll and lib) sources
##############################################################################

# files in the root directory (TODO: move to ./misc or ./iga or ./util (need better name))
set(IGA_Misc
  ${CMAKE_CURRENT_SOURCE_DIR}/ColoredIO.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ColoredIO.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EnumBitset.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ErrorHandler.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/InstDiff.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/InstDiff.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/asserts.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/asserts.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bits.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deprecation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/strings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/strings.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp
)

set(IGA_Frontend
  ${IGA_Frontend_Formatter}
  ${IGA_Frontend_Formatter_LdStSyntax}
  ${IGA_Frontend_Parser}
  ${IGA_Frontend_Parser_LdStSyntax}
)

set(IGA_Backend_GED
  ${IGA_Backend_GED_EncoderOnly}
  ${IGA_Backend_GED_Decoder}
  )


# purposly exclude iga.cpp since it must be compiled differently for .lib/.a and .so/.dll
set(IGA_SOURCES
  ${IGA_API_Headers}
  ${IGA_Backend}
  ${IGA_Backend_GED}
  ${IGA_Backend_Native}
  ${IGA_Backend_Native_PstG12}
  ${IGA_Frontend}
  ${IGA_IR}
  ${IGA_MemManager}
  ${IGA_Misc}
  ${IGA_Models}
  ${IGA_Timer}
  ${GED_INCLUDE}/ged.h
)

set(FLAG "")

set(IGA_DEFINITIONS _CONSOLE _LIB ${VSVERSION} ${GED_BRANCH})
if (MSVC)
  if(IGC_OPTION__UNIVERSAL_DRIVER)
    set(IGA_LINK_FLAGS "/OPT:REF /OPT:ICF /INCREMENTAL:NO /NODEFAULTLIB:kernel32.lib")
  else()
    set(IGA_LINK_FLAGS "/OPT:REF /OPT:ICF /INCREMENTAL:NO")
  endif()
    set(IGA_COMPILER_FLAGS "/GL")
else(MSVC)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions ("-Wno-logical-op-parentheses")
  endif()
  set(IGA_LINK_FLAGS "")
  set(IGA_COMPILER_FLAGS "")
endif(MSVC)

if(UNIX)
  set(FPIC_FLAG "-fPIC")
endif(UNIX)

if(ANDROID AND MEDIA_IGA)
  set(NDK_Libstdcxx $ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libcxx)
  set(Extra_Android_Inc ${NDK_Libstdcxx}/include ${ANDROID_NDK}/sources/android/support/include ${NDK_Libstdcxx}/include/backward)
  include_directories(${Extra_Android_Inc})
  link_directories($ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/$ENV{CMRT_ARCH})
endif(ANDROID AND MEDIA_IGA)

##############################################################################
# iga_o##.lib
#
# Partial IGA object library (everything except the iga.cpp interface,
# which needs to be compiled differently for .lib and .dll/.so targets.
##############################################################################
add_library(IGA_OLIB OBJECT ${IGA_SOURCES})
set_target_properties(IGA_OLIB PROPERTIES
    OUTPUT_NAME                                "iga_o${TARGET_MODIFIER}"
    COMPILE_DEFINITIONS                        "${IGA_DEFINITIONS}"
    COMPILER_FLAGS                             "${IGA_COMPILER_FLAGS}"
    LINK_FLAGS                                 "${IGA_LINK_FLAGS}"
    FOLDER                                     "IGAProjs"
  )

if(IGC_BUILD AND MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(IGA_OLIB)
endif()

##############################################################################
# iga_s##.lib
#
# IGA static library.  Combines piga.lib with the api/iga.cpp interface file
##############################################################################
add_library(IGA_SLIB STATIC $<TARGET_OBJECTS:GEDLibrary> $<TARGET_OBJECTS:IGA_OLIB> ${IGA_API})

set(IGA_SLIB_LINK_FLAGS "${IGA_LINK_FLAGS}")
if(MSVC)
  set(IGA_SLIB_LINK_FLAGS "${IGA_SLIB_LINK_FLAGS} /LTCG")
endif()
set(IGA_SLIB_DEFINITIONS ${IGA_DEFINITIONS} IGA_BUILDING_LIB)



# The Windows DLL also produces an import library whose name conflicts with
# this static library.  Hence, we must call this something other than the
set_target_properties(IGA_SLIB PROPERTIES
    OUTPUT_NAME                                "iga_s${TARGET_MODIFIER}"
  )
set_target_properties(IGA_SLIB PROPERTIES
    COMPILE_DEFINITIONS                        "${IGA_SLIB_DEFINITIONS}"
    COMPILER_FLAGS                             "${IGA_COMPILER_FLAGS}"
    LINK_FLAGS                                 "${IGA_SLIB_LINK_FLAGS}"
    FOLDER                                     "IGAProjs"
  )

if(IGC_BUILD AND MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(IGA_SLIB)
endif()

##############################################################################
# iga_enc##.lib
#
# The IGA static library that includes only the encoder (no parser or decoder)
# and is compiled without C++ exception support (for IGC compatibility).
#
# This cannot use the object library and must rebuild everything from source
# since exceptions must be disabled.
##############################################################################
set(IGA_SOURCES_ENCODER_ONLY
  ${IGA_API_EncoderInterface}
  ${IGA_Backend}
  ${IGA_Backend_GED_EncoderOnly}
  ${IGA_Frontend_Formatter}
  ${IGA_Frontend_Formatter_LdStSyntax}
  ${IGA_IR}
  ${IGA_MemManager}
#  ${IGA_Misc}
  ${IGA_Models}
  ${IGA_Timer}
  ${GED_INCLUDE}/ged.h
)

add_library(IGA_ENC_LIB STATIC $<TARGET_OBJECTS:GEDLibrary> ${IGA_SOURCES_ENCODER_ONLY})

set(IGA_ENC_LIB_DEFINITIONS ${IGA_DEFINITIONS} IGA_BUILDING_LIB)

set_target_properties(IGA_ENC_LIB PROPERTIES
    OUTPUT_NAME                                "iga_enc${TARGET_MODIFIER}"
    COMPILE_DEFINITIONS                        "${IGA_ENC_LIB_DEFINITIONS}"
    COMPILER_FLAGS                             "${IGA_COMPILER_FLAGS}"
    LINK_FLAGS                                 "${IGA_LINK_FLAGS}"
    FOLDER                                     "IGAProjs"
  )

if(IGC_BUILD AND MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(IGA_ENC_LIB)
endif()

# DISABLE_ENCODER_EXCEPTIONS: Disabling exceptions for encoder only project.
# So it can be used in vISA BE, which is used in IGC. (IGC doesn't permit C++
# exceptions to be used
# Alex: seems like this is the way to do it so it works in various
# linux, android builds.
target_compile_definitions(IGA_ENC_LIB PUBLIC DISABLE_ENCODER_EXCEPTIONS)

##############################################################################
# iga##.dll
##############################################################################
add_library(IGA_DLL SHARED $<TARGET_OBJECTS:GEDLibrary> $<TARGET_OBJECTS:IGA_OLIB> ${IGA_API})

set(IGA_DLL_DEFINITIONS ${IGA_DEFINITIONS} IGA_BUILDING_DLL)
if (WIN32)
  # MSVC / MinGW / Cygwin / Clang all on windows generate iga64.dll
  set_target_properties(IGA_DLL PROPERTIES PREFIX "")
endif(WIN32)
set(IGA_DLL_LINK_FLAGS ${IGA_LINK_FLAGS})

# TRB: why do we APPEND here instead of setting these?
set_property(TARGET IGA_DLL APPEND PROPERTY
    OUTPUT_NAME                                 "iga${TARGET_MODIFIER}"
    )
set_property(TARGET IGA_DLL APPEND PROPERTY
    COMPILE_DEFINITIONS                         ${IGA_DLL_DEFINITIONS}
  )
set_property(TARGET IGA_DLL APPEND PROPERTY
    LINK_FLAGS                                  "${IGA_DLL_LINK_FLAGS}"
  )
set_property(TARGET IGA_DLL APPEND PROPERTY
    FOLDER                                      "IGAProjs"
  )

if(IGC_BUILD AND MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(IGA_DLL)

add_custom_command( TARGET IGA_DLL
                    POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:IGA_DLL> $<TARGET_FILE_DIR:GenX_IR_Exe>
                    COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_PDB_FILE:IGA_DLL> $<TARGET_FILE_DIR:GenX_IR_Exe>
)
endif()

# Hidden visibility suppresses PLT entries from being created for GED
# and functions and trims the interface down to a minimal size.
# This was needed when IGA was used by GT-PIN, which also uses GED.
# IGA ended up using GED library linked with GT-PIN rather than the IGA copy.
# see: https://gcc.gnu.org/wiki/Visibility
#
# WARNING: do not move the static libraries below this, this is a
# global setting, and will apply to this and later projects.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)


##############################################################################
# Various Shared Project Properties
##############################################################################


source_group("API" FILES ${IGA_API_Headers} ${IGA_API_EncoderInterface} ${IGA_API})
source_group("Backend" FILES ${IGA_Backend})

if(WIN32)
source_group("Backend\\GED" FILES ${IGA_Backend_GED})
source_group("Backend\\Native" FILES ${IGA_Backend_Native})
else()
source_group("Backend/GED" FILES ${IGA_Backend_GED})
source_group("Backend/Native" FILES ${IGA_Backend_Native})
endif()


source_group("Frontend" FILES ${IGA_Frontend_Formatter} ${IGA_Frontend_Parser})
if(WIN32)
source_group("Frontend\\LdStSyntax" FILES ${IGA_Frontend_Formatter_LdStSyntax} ${IGA_Frontend_Parser_LdStSyntax})
else()
source_group("Frontend/LdStSyntax" FILES ${IGA_Frontend_Formatter_LdStSyntax} ${IGA_Frontend_Parser_LdStSyntax})
endif()

source_group("IR" FILES ${IGA_IR})
source_group("MemManager" FILES ${IGA_MemManager})
source_group("Models" FILES ${IGA_Models})
source_group("Misc" FILES ${IGA_Misc} ${IGA_Timer})

if(ANDROID AND MEDIA_IGA)
  target_link_libraries(IGA_DLL c++_static)
  target_link_libraries(IGA_SLIB c++_static)
  target_link_libraries(IGA_ENC_LIB c++_static)
endif(ANDROID AND MEDIA_IGA)
# target_link_libraries(IGA PRIVATE GEDLibrary)

  if(IGC_BUILD)
    set_target_properties(IGA_DLL PROPERTIES
                          VERSION "${IGC_API_MAJOR_VERSION}.${IGC_API_MINOR_VERSION}.${IGC_API_PATCH_VERSION}"
                          SOVERSION "${IGC_API_MAJOR_VERSION}")
  endif()
 
  if(UNIX)
    if (CMAKE_INSTALL_FULL_LIBDIR)
        set(IGA_LIB_PATH ${CMAKE_INSTALL_FULL_LIBDIR})
    else()
        set(IGA_LIB_PATH ${CMAKE_INSTALL_PREFIX}/lib)
    endif()
    INSTALL(TARGETS IGA_DLL
        LIBRARY DESTINATION ${IGA_LIB_PATH} COMPONENT igc-core)
    unset(IGA_LIB_PATH)
  elseif(WIN32)
    install(TARGETS IGA_DLL RUNTIME
            DESTINATION Release/lh64              CONFIGURATIONS Release)
    install(TARGETS IGA_DLL RUNTIME
            DESTINATION Release-Internal/lh64     CONFIGURATIONS ReleaseInternal)
    install(TARGETS IGA_DLL RUNTIME
            DESTINATION Debug/lh64                CONFIGURATIONS Debug)

    install(FILES $<TARGET_PDB_FILE:IGA_DLL>
            DESTINATION Release/lh64/pdb          CONFIGURATIONS Release)
    install(FILES $<TARGET_PDB_FILE:IGA_DLL>
            DESTINATION Release-Internal/lh64/pdb CONFIGURATIONS ReleaseInternal)
    install(FILES $<TARGET_PDB_FILE:IGA_DLL>
            DESTINATION Debug/lh64/pdb            CONFIGURATIONS Debug)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # Only copy 64 build
        install(TARGETS IGA_DLL RUNTIME
                DESTINATION ../Test_Tools/Release/x64/Standalones          CONFIGURATIONS Release)
        install(TARGETS IGA_DLL RUNTIME
                DESTINATION ../Test_Tools/Release-Internal/x64/Standalones CONFIGURATIONS ReleaseInternal)
        install(TARGETS IGA_DLL RUNTIME
                DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)

        install(FILES $<TARGET_PDB_FILE:IGA_DLL>
                DESTINATION ../Test_Tools/Release/x64/Standalones          CONFIGURATIONS Release)
        install(FILES $<TARGET_PDB_FILE:IGA_DLL>
                DESTINATION ../Test_Tools/Release-Internal/x64/Standalones CONFIGURATIONS ReleaseInternal)
        install(FILES $<TARGET_PDB_FILE:IGA_DLL>
                DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)
    endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

  endif(UNIX)

project(IGA_EXE)
# Set up windows mobile build flags to use dynamic multi-threaded runtime (/MD)
# Set up windows(other) build flags to use static multi-threaded runtime (/MT)
#
# This mimicks vISA/MDF configurations
win_static_runtime()


set(IGA_EXE_CPP
  ${CMAKE_CURRENT_SOURCE_DIR}/assemble.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/disassemble.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/decode_fields.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/list_ops.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(IGA_EXE_HPP
  ${CMAKE_CURRENT_SOURCE_DIR}/fatal.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_main.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/opts.hpp
)

source_group("HeaderFiles" FILES ${IGA_EXE_HPP})
source_group("SourceFiles" FILES ${IGA_EXE_CPP})

set(IGA_EXE_DEFINITIONS _CONSOLE _LIB ${GED_BRANCH})

add_executable(IGA_EXE ${IGA_EXE_CPP} ${IGA_EXE_HPP})
if(MSVC_GENERATOR)
  # Due to all files being built in same directory (see upper-level cmake)
  # and name conflict between exe and dll builds fail sometime on the .ilk collision.
  # Specifically, MSVC names the .ilk file iga32.ilk for this executable,
  # but this name collides with the incremental file produced by iga32.dll.
  # (The same problem applies to 64-bit).
  #
  # We had the same problem with the PDB file, but MSVC allows you to
  # specify the PDB file name (see PDB_NAME below).
  #
  # Solutions:
  #   1. Don't generate the files in the same directory.
  #      We need this because the MCM build is hardwired to expect
  #      all the output in the same directory.
  #       => Requires MCM to fix their odd throw-it-all-in-one-blender
  #          (directory) approach.
  #
  #   2. Copy the exe files *after* they are built.  They all build
  #      in their own clean environment as MSVC and other compilers
  #      expect.  Not sure why this doesn't work.  I think the MCM scripts
  #      cannot deal with this approach either.
  #
  #   3. Create separate CMakeLists for MCM and "normal users".
  #
  #   4. Rename iga32.exe to iga_exe32.exe or something.  I am strongly against
  #      this.  In general, VPG already does a horrible job naming things,
  #      this is just blunt-force humiliation to have to rename "thing" to "thing2"
  #      because we're too lazy and incompetent to solve this correctly.
  #
  #   5. Disable incremental linking (prevents the collision).  This makes rebuild
  #      slightly less efficient on Windows, but works around this issue.  In general
  #      building everything in one directory is inherantly unsafe and this just
  #      kicks the can down the road.  But for now, this is what we can agree on.
  set(IGA_EXE_LINK_FLAGS "/INCREMENTAL:NO")
else()
  # This doesn't impact non-MSVC compilers.
  set(IGA_EXE_LINK_FLAGS "")
endif()

# Set release build specific defines
set_target_properties( IGA_EXE PROPERTIES
    OUTPUT_NAME                                "iga${TARGET_MODIFIER}"
    COMPILE_DEFINITIONS                        "${IGA_EXE_DEFINITIONS}"
    PDB_NAME                                   "iga_exe"
    LINK_FLAGS                                 "${IGA_EXE_LINK_FLAGS}"
  )

include_directories("../IGALibrary")

if(NOT WIN32)
  set_target_properties(IGA_EXE PROPERTIES PREFIX "")
  target_link_libraries(IGA_EXE PUBLIC IGA_SLIB "-lrt")
else()
  target_link_libraries(IGA_EXE PUBLIC IGA_SLIB)
endif()

INSTALL(TARGETS IGA_EXE
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
    ARCHIVE DESTINATION .
)

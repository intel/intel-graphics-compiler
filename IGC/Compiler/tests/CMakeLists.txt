if(NOT TARGET igc_opt)
  message("[check-igc] LIT tests disabled. Missing igc_opt target.")
elseif(NOT BS_ENABLE_ULT)
  message("[check-igc] LIT tests disabled. Set `BS_ENABLE_ULT` to enable.")
else()
  # Canonicalization will set undefined variable to false. This step is to make
  # sure that `configure_file` called later on will generate a correct Python
  # script.
  llvm_canonicalize_cmake_booleans(
    LLVM_TOOL_LTO_BUILD
    HAVE_OCAMLOPT
    HAVE_OCAML_OUNIT
    LLVM_INCLUDE_GO_TESTS
    LLVM_USE_INTEL_JITEVENTS
    HAVE_LIBZ
    HAVE_LIBXAR
    LLVM_ENABLE_DIA_SDK
    LLVM_ENABLE_FFI
    ENABLE_SHARED
    ENABLE_ASSERTIONS
    BUILD_SHARED_LIBS
    )

  # Variables set here are used by `configure_file` call and by
  # `add_lit_testsuite` later on.
  set(IGC_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
      CACHE STRING "desc" FORCE)
  set(LLVM_TOOLS_DIR ${CMAKE_CURRENT_BINARY_DIR}/lit-tools
      CACHE STRING "desc" FORCE)
  set(LLVM_LIT_OUTPUT_DIR ${LLVM_TOOLS_DIR}
      CACHE STRING "desc" FORCE)
  set(LLVM_MAIN_SRC_DIR ${LLVM_SOURCE_DIR}
      CACHE STRING "desc" FORCE)
  set(LIT_COMMAND "${PYTHON_EXECUTABLE};${LLVM_MAIN_SRC_DIR}/utils/lit/lit.py"
      CACHE STRING "desc" FORCE)

  # This file is basically used to transfer variables from CMake to LIT.
  set(IGC_LIT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
    ${IGC_LIT_CONFIG_FILE}
    )

  # If any new tool is required by any of the LIT tests add it here:
  set(IGC_LIT_TEST_DEPENDS
    FileCheck
    llvm-config
    count
    not
    igc_opt
    )

  # This will create a target called `check-igc`, which will run all tests from
  # IGC/Compiler/tests directory. The tests will be run on files in the source
  # directory, since they are not modified, there doesn't seem to be any reason
  # for copying them.
  add_lit_testsuite(check-igc "Running the IGC LIT tests"
    ${CMAKE_CURRENT_SOURCE_DIR}
    PARAMS llvm_site_config=${IGC_LIT_CONFIG_FILE}
    DEPENDS ${IGC_LIT_TEST_DEPENDS}
    )

  # LIT will be using binaires from `LLVM_TOOLS_DIR`. The target below will
  # populate this directory.
  if(WIN32)
    set(_command xcopy)
    set(_parameters /Y/I)
    add_custom_command(
      TARGET check-igc
      PRE_BUILD
      COMMAND ${_command} "\"$<TARGET_FILE_DIR:igc_opt>\"" "\"${LLVM_TOOLS_DIR}\"" ${_parameters}
      COMMAND ${_command} "\"$<TARGET_FILE_DIR:FileCheck>\"" "\"${LLVM_TOOLS_DIR}\"" ${_parameters}
    )
  else()
    add_custom_command(
      TARGET check-igc
      PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:igc_opt> ${LLVM_TOOLS_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:FileCheck> ${LLVM_TOOLS_DIR}
    )
  endif()

  # Tests should not be excluded from "Build Solution" in VS.
  set_target_properties(check-igc PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD OFF)

  # Two lines below are just used to group LIT reated targets in single directory
  # in IDE. They are completely optional.
  set_target_properties(check-igc PROPERTIES FOLDER "LIT Tests")
endif()

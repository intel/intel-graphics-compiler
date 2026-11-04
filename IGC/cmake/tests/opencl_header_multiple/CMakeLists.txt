#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Reproduces: a toolchain that provides more than one opencl-c.h breaks configure
# (intel/intel-graphics-compiler#314).
#
# IGC has no CMake test harness - there is no top-level enable_testing() and no
# add_test() anywhere in the tree - so this reproducer is a standalone project
# that has to be configured by hand:
#
#   cmake -S IGC/cmake/tests/opencl_header_multiple -B <some-build-dir>
#
# It succeeds (configure exits 0) when the module under test behaves and fails
# with a FATAL_ERROR naming the scenario otherwise. No compiler, no LLVM and no
# network access are needed; everything the module under test looks at is
# generated below.
#
# Each scenario runs igc_find_opencl_clang.cmake in a child configure (probe/)
# and asserts on what that child reported and on its exit status. Doing it in a
# child is what makes the assertions independent: a hard CMake error inside one
# of the replayed consumers would otherwise abort the whole run and hide every
# later check.

cmake_minimum_required(VERSION 3.13)
project(igc_opencl_header_multiple NONE)

set(_fake "${CMAKE_CURRENT_BINARY_DIR}/fake-prebuilds")

# A prebuilds directory that legitimately ships two opencl-c.h: the one
# opencl-clang installs under include/cclang, and the one clang itself installs
# under lib/clang/<version>/include. This is the layout from the bug report.
set(_cclangHeader "${_fake}/include/cclang/opencl-c.h")
set(_clangHeader  "${_fake}/lib/clang/17.0.6/include/opencl-c.h")
file(WRITE "${_cclangHeader}" "// fake opencl-clang header\n")
file(WRITE "${_clangHeader}" "// fake clang header\n")
file(WRITE "${_fake}/include/cclang/opencl-c-base.h" "// fake base header\n")
file(WRITE "${_fake}/lib/libopencl-clang.so" "")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/clang-stub/clang"
     "#!/bin/sh\necho \"clang version 17.0.5 (fake)\" 1>&2\n")
file(COPY "${CMAKE_CURRENT_BINARY_DIR}/clang-stub/clang" DESTINATION "${_fake}/bin"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

# Guard against the fixture silently losing its point.
file(GLOB_RECURSE _candidates "${_fake}/*opencl-c.h")
list(LENGTH _candidates _candidateCount)
if(_candidateCount LESS 2)
  message(FATAL_ERROR "fixture is broken: the fake prebuilds dir must contain at least two "
                      "opencl-c.h, found ${_candidateCount}: ${_candidates}")
endif()

# Runs the probe project and returns its exit status plus the values it reported.
function(run_probe scenario)
  set(_bin "${CMAKE_CURRENT_BINARY_DIR}/probe-${scenario}")
  file(REMOVE_RECURSE "${_bin}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -S "${CMAKE_CURRENT_LIST_DIR}/probe" -B "${_bin}"
            "-DIGC_TEST_PREBUILDS_DIR=${_fake}" ${ARGN}
    OUTPUT_VARIABLE _out ERROR_VARIABLE _err RESULT_VARIABLE _rc)
  set(_log "${_out}${_err}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/probe-${scenario}.log" "${_log}")
  set(PROBE_RC "${_rc}" PARENT_SCOPE)
  foreach(_key HEADER_COUNT HEADER_VALUE HEADER_DIR BIF_SRC_COUNT)
    set(_value "")
    if(_log MATCHES "IGC_TEST_${_key}=([^\r\n]*)")
      set(_value "${CMAKE_MATCH_1}")
    endif()
    set("PROBE_${_key}" "${_value}" PARENT_SCOPE)
  endforeach()
endfunction()

function(expect scenario what expected actual)
  if(NOT "${expected}" STREQUAL "${actual}")
    message(FATAL_ERROR "[${scenario}] ${what}: expected '${expected}', got '${actual}'\n"
                        "see ${CMAKE_CURRENT_BINARY_DIR}/probe-${scenario}.log")
  endif()
endfunction()

# --------------------------------------------------------------------------
# Scenario 1: two headers, no override. The module must reduce them to exactly
# one path, and deterministically the same one, so that every consumer of
# ${opencl-header} keeps working. Before the fix the glob result stays a list,
# get_filename_component() is handed three arguments and the child configure
# dies with "unknown component".
# --------------------------------------------------------------------------
run_probe(multiple)
expect(multiple "number of opencl-c.h paths" "1" "${PROBE_HEADER_COUNT}")
expect(multiple "selected opencl-c.h" "${_cclangHeader}" "${PROBE_HEADER_VALUE}")
expect(multiple "child configure exit status" "0" "${PROBE_RC}")
if("${PROBE_BIF_SRC_COUNT}" STREQUAL "" OR PROBE_BIF_SRC_COUNT LESS 1)
  message(FATAL_ERROR "[multiple] get_bif_src_list() produced '${PROBE_BIF_SRC_COUNT}' entries: "
                      "its arguments were bound to the wrong values")
endif()

# --------------------------------------------------------------------------
# Scenario 2: the documented IGC_OPTION__OPENCL_HEADER_PATH override. It has to
# be honoured on the prebuilds branch too, which is the branch the bug was
# reported on; before the fix the override was only consulted when opencl-clang
# came from the system, and the glob won here regardless of it.
# --------------------------------------------------------------------------
run_probe(override "-DIGC_OPTION__OPENCL_HEADER_PATH=${_clangHeader}")
expect(override "number of opencl-c.h paths" "1" "${PROBE_HEADER_COUNT}")
expect(override "selected opencl-c.h" "${_clangHeader}" "${PROBE_HEADER_VALUE}")
expect(override "child configure exit status" "0" "${PROBE_RC}")

message(STATUS "PASS: opencl-c.h selection is single-valued, deterministic and overridable")

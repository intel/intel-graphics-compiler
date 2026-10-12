#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#=========================== Host / Target Architecture ========================

include_guard(DIRECTORY)

# Detects host and target architecture.
#
# Currently supports: Windows32, Windows64, WindowsARM, Android32, Android64, AndroidMIPS, AndroidARM,
#                     Linux32, Linux64, LinuxMIPS, LinuxRISCV, LinuxARM
#
# @param targetArchVarName Name of variable placeholder for target architecture.
# @param hostArchVarName   Name of variable placeholder for host architecture.
function(igc_arch_detect targetArchVarName hostArchVarName)
  string(TOLOWER "${CMAKE_GENERATOR}" _cmakeGenerator)
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _cmakeTargetProcessor)

  # Target architecture:
  # Detect target architecture on Windows using suffix from generator.
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    if(_cmakeGenerator MATCHES " (arm|aarch64)$")
      set(_targetArchitecture "WindowsARM")
    elseif(_cmakeGenerator MATCHES " (x64|x86_64|win64|amd64|64[ ]*\\-[ ]*bit)$")
      set(_targetArchitecture "Windows64")
    else()
      set(_targetArchitecture "Windows32")
    endif()
  # Use system processor set by toolchain or CMake.
  elseif(ANDROID OR (CMAKE_SYSTEM_NAME MATCHES "Linux"))
    if(ANDROID)
      set(_targetArchOS "Android")
    else()
      set(_targetArchOS "Linux")
    endif()

    if(_cmakeTargetProcessor MATCHES "(x64|x86_64|amd64|64[ ]*\\-[ ]*bit)")
      set(_targetArchitecture "${_targetArchOS}64")
    elseif(_cmakeTargetProcessor MATCHES "(x32|x86|i[0-9]+86|32[ ]*\\-[ ]*bit)")
      set(_targetArchitecture "${_targetArchOS}32")
    elseif(_cmakeTargetProcessor MATCHES "mips")
      set(_targetArchitecture "${_targetArchOS}MIPS")
    elseif(_cmakeTargetProcessor MATCHES "riscv64")
      set(_targetArchitecture "${_targetArchOS}RISCV")
    else()
      set(_targetArchitecture "${_targetArchOS}ARM")
    endif()
  else()
    set(_targetArchitecture "Unknown-NOTFOUND")
  endif()

  # Host architecture:
  # Detect system architecture using WMI.
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    if (("$ENV{PROCESSOR_ARCHITECTURE}" MATCHES "AMD64") OR
        ("$ENV{PROCESSOR_ARCHITEW6432}" MATCHES "AMD64"))
      set(_hostArchitecture "Windows64")
    else()
      set(_hostArchitecture "Windows32")
    endif()
  # Use 'uname -m' to detect kernel architecture.
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(_hostArchOS "Linux")
    set(_osArchitecture "x86_64")
    execute_process(
        COMMAND uname -m
        TIMEOUT 10
        OUTPUT_VARIABLE _osArchitecture
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string(TOLOWER "${_osArchitecture}" _osArchitecture)

    if(_osArchitecture MATCHES "(x64|x86_64|amd64|64[ ]*\\-[ ]*bit)")
      set(_hostArchitecture "${_hostArchOS}64")
    elseif(_osArchitecture MATCHES "(x32|x86|i[0-9]+86|32[ ]*\\-[ ]*bit)")
      set(_hostArchitecture "${_hostArchOS}32")
    elseif(_osArchitecture MATCHES "mips")
      set(_hostArchitecture "${_hostArchOS}MIPS")
    elseif(_osArchitecture MATCHES "riscv64")
      set(_hostArchitecture "${_hostArchOS}RISCV")
    else()
      set(_hostArchitecture "${_hostArchOS}ARM")
    endif()
  else()
    set(_hostArchitecture "Unknown-NOTFOUND")
  endif()

  set("${targetArchVarName}" "${_targetArchitecture}" PARENT_SCOPE)
  set("${hostArchVarName}"   "${_hostArchitecture}" PARENT_SCOPE)
endfunction()


# Determines whether architecture is valid (accepts only normalized).
#
# @param retValName Name of variable placeholder where result will be returned.
# @param arch       Architecture name to validate. Architecture should be normalized.
function(igc_arch_validate retVarName arch)
  # Allowed architectures (list).
  set(__allowedArchs
      "Windows32" "Windows64"               "WindowsARM"
      "Android32" "Android64" "AndroidMIPS" "AndroidARM"
      "Linux32"   "Linux64"   "LinuxMIPS"   "LinuxRISCV"
      "LinuxARM"
    )

  list(FIND __allowedArchs "${arch}" _allowedArchIdx)
  if(_allowedArchIdx LESS 0)
    set("${retVarName}" NO  PARENT_SCOPE)
  else()
    set("${retVarName}" YES PARENT_SCOPE)
  endif()
endfunction()


# Normalizes architecture name. If architecture is not supported by helper functions
# the "Unknown-NOTFOUND" will be returned.
#
# Currently supports: Windows32, Windows64, WindowsARM, Android32, Android64, AndroidMIPS, AndroidARM,
#                     Linux32, Linux64, LinuxMIPS, LinuxRISCV, LinuxARM
#
# @param retValName Name of variable placeholder where result will be returned.
# @param arch       Architecture name to normalize / filter.
function(igc_arch_normalize retVarName arch)
  string(TOLOWER "${arch}" _arch)
  string(STRIP "${_arch}" _arch)

  if(_arch MATCHES "^win")
    set(_osPart "Windows")
  elseif(_arch MATCHES "^and")
    set(_osPart "Android")
  elseif(_arch MATCHES "^lin")
    set(_osPart "Linux")
  else()
    set(_osPart "Windows")
  endif()

  if(_arch MATCHES "64$")
    set(_cpuPart "64")
  elseif(_arch MATCHES "(32|86)$")
    set(_cpuPart "32")
  elseif(_arch MATCHES "mips$")
    set(_cpuPart "MIPS")
  elseif(_arch MATCHES "riscv$")
    set(_cpuPart "RISCV")
  elseif(_arch MATCHES "arm|aarch64")
    set(_cpuPart "ARM")
  else()
    set("${retVarName}" "Unknown-NOTFOUND" PARENT_SCOPE)
    return()
  endif()

  set(_normalizedArch "${_osPart}${_cpuPart}")
  igc_arch_validate(_archValid "${_normalizedArch}")
  if(_archValid)
    set("${retVarName}" "${_normalizedArch}" PARENT_SCOPE)
  else()
    set("${retVarName}" "Unknown-NOTFOUND" PARENT_SCOPE)
  endif()
endfunction()

# Gets OS platform used in specified architecture. If it cannot be determined
# the "Unknown-NOTFOUND" will be returned.
#
# @param retValName Name of variable placeholder where result will be returned.
# @param arch       Architecture name to get info from.
function(igc_arch_get_os retVarName arch)
  set(_os_detect_list "^(Windows|Android|Linux)(.*)$")

  if(arch MATCHES "${_os_detect_list}")
    set("${retVarName}" "${CMAKE_MATCH_1}" PARENT_SCOPE)
  else()
    set("${retVarName}" "Unknown-NOTFOUND" PARENT_SCOPE)
  endif()
endfunction()

function(igc_arch_get_cpu retVarName)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set("${retVarName}" "64" PARENT_SCOPE)
  else()
    set("${retVarName}" "32" PARENT_SCOPE)
  endif()
endfunction()

# Determines whether cross-compilation is needed.
#
# @param retValName Name of variable placeholder where result will be returned.
# @param targetArch Target architecture (it will be normalized).
# @param hostArch   Host architecture (it will be normalized).
function(igc_arch_crosscompile_needed retVarName targetArch hostArch)
  # Allowed cross-executions (keys list, lists for each key).
  # Key:    host architecture.
  # Values: target architecture that can be run on host (different than host).
  set(__allowedCrossExecution "Windows64")
  set(__allowedCrossExecution_Windows64 "Windows32")

  igc_arch_normalize(_targetArch "${targetArch}")
  igc_arch_normalize(_hostArch   "${hostArch}")

  if(_targetArch STREQUAL _hostArch)
    set("${retVarName}" NO PARENT_SCOPE)
  else()
    list(FIND __allowedCrossExecution "${_hostArch}" _keyIdx)
    if(_keyIdx LESS 0)
      set("${retVarName}" YES PARENT_SCOPE)
    else()
      list(FIND "__allowedCrossExecution_${_hostArch}" "${_targetArch}" _valIdx)
      if(_valIdx LESS 0)
        set("${retVarName}" YES PARENT_SCOPE)
      else()
        set("${retVarName}" NO PARENT_SCOPE)
      endif()
    endif()
  endif()
endfunction()

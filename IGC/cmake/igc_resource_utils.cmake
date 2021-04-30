#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#=============================== Resource files ================================

include_guard(DIRECTORY)

# Registers configuration for resource files for specific idenitifer.
#
# The configuration of resource files can be fetched using igc_rc_get_resource() function.
#
# Current version of CMake does not allow to modify sources list after calling add_*()
# functions, so resources still need to be added by using igc_rc_get_resource() to get
# list of resource files and adding it to add_*().
#
# igc_rc_register_resource(
#     resourceId
#     [RESOURCES                      [importedId [importedId [...]]]]
#     [FILE                           rcFile [rcFile [...]]
#       [[APPEND] INCLUDE_DIRECTORIES [incudeDir [includeDir [...]]]]
#       [[APPEND] DEFINES             [define [define [...]]]]
#       [[APPEND] FLAGS               [flag [flag [...]]]]]
#   )
#
# @param resourceId Identifier of resource file or of group of resource files.
# @param importedId Identifiers of additional resources that should be added with currently registered
#                   resource.
# @param rcFile     Resource files which are be connected to specified resource identifier.
# @param incudeDir  Additional for resource files.
# @param define     Additional defines for resource files.
# @param flag       Additional flags for resource files.
function(igc_rc_register_resource resourceId)
  if(NOT ("${resourceId}" MATCHES "[a-zA-Z0-9_]+"))
    message(FATAL_ERROR "The resource identifier \"${resourceId}\" is invalid.")
  endif()

  set(_rcPropPrefix "IGC_PROPERTY__RC_RESOURCES_R${resourceId}_")
  set(_rcIdxPropName "${_rcPropPrefix}INDEX")

  # Preserve resource index to support multiple registration calls.
  get_property(_rcIdxSet GLOBAL PROPERTY "${_rcIdxPropName}" SET)
  if(_rcIdxSet)
    get_property(_rcIdx GLOBAL PROPERTY "${_rcIdxPropName}")
  else()
    set(_rcIdx 0)
  endif()

  set(_rcFilesPropName           "${_rcPropPrefix}G${_rcIdx}_FILES")
  set(_includeDirsPropName       "${_rcPropPrefix}G${_rcIdx}_INCDIRS")
  set(_definesPropName           "${_rcPropPrefix}G${_rcIdx}_DEFINES")
  set(_flagsPropName             "${_rcPropPrefix}G${_rcIdx}_FLAGS")
  set(_importResIdsPropName      "${_rcPropPrefix}G${_rcIdx}_IMPORTED_RES_IDS")
  set(_appendIncludeDirsPropName "${_includeDirsPropName}_APPEND")
  set(_appendDefinesPropName     "${_definesPropName}_APPEND")
  set(_appendFlagsPropName       "${_flagsPropName}_APPEND")

  set(_importResIds)
  set(_rcFiles)
  set(_includeDirs)
  set(_defines)
  set(_flags)
  set(_appendIncludeDirs NO)
  set(_appendDefines     NO)
  set(_appendFlags       NO)

  set(_parseState 1)
  foreach(_resArg ${ARGN})
    string(REPLACE ";" "\;" _resArg "${_resArg}") # [WA#1] Must escape ; again if occurred in item.

    # States: [0] <param> [1] *1( "RESOURCES" [100] *<param> [100] ) *1( "FILE" [2] <param> [3] *<param> [3]
    #         *1( *1( "APPEND" [4] ) "INCLUDE_DIRECTORIES" [5] *<param> [5] ) *1( *1( "APPEND" [6] ) "DEFINES" [7] *<param> [7] )
    #         *1( *1( "APPEND" [8] ) "FLAGS" [9] *<param> [9] ) )
    # Transitions: 0 -> 1 // by explict parameters
    #              1 (FILE) -> 2 -> 3
    #              1 (RESOURCES) -> 100
    #              3 (APPEND) -> 4 {6,8}
    #              3 (INCLUDE_DIRECTORIES) -> 5
    #              3 (DEFINES) -> 7
    #              3 (FLAGS) -> 9
    #              3 -> 3
    #              4 (INCLUDE_DIRECTORIES) -> 5
    #              4 (DEFINES) -> 7
    #              4 (FLAGS) -> 9
    #              5 (APPEND) -> 6 {8}
    #              5 (DEFINES) -> 7
    #              5 (FLAGS) -> 9
    #              5 -> 5
    #              6 (DEFINES) -> 7
    #              6 (FLAGS) -> 9
    #              7 (APPEND) -> 8
    #              7 (FLAGS) -> 9
    #              7 -> 7
    #              8 (FLAGS) -> 9
    #              9 -> 9
    #              100 (FILE) -> 2 -> 3
    #              100 -> 100
    # Stop States: 3, 5, 7, 9, 100
    if(_parseState EQUAL 1)
      if(_resArg MATCHES "^FILE$")
        set(_parseState 2)
      elseif(_resArg MATCHES "^RESOURCES$")
        set(_parseState 100)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_resArg}\".")
      endif()
    elseif(_parseState EQUAL 2)
      set(_rcFiles "${_resArg}")
      set(_parseState 3)
    elseif(_parseState EQUAL 3)
      if(_resArg MATCHES "^APPEND$")
        set(_parseState 4)
      elseif(_resArg MATCHES "^INCLUDE_DIRECTORIES$")
        set(_parseState 5)
      elseif(_resArg MATCHES "^DEFINES$")
        set(_parseState 7)
      elseif(_resArg MATCHES "^FLAGS$")
        set(_parseState 9)
      else()
        list(APPEND _rcFiles "${_resArg}")
      endif()
    elseif(_parseState EQUAL 4)
      if(_resArg MATCHES "^INCLUDE_DIRECTORIES$")
        set(_appendIncludeDirs YES)
        set(_parseState 5)
      elseif(_resArg MATCHES "^DEFINES$")
        set(_appendDefines YES)
        set(_parseState 7)
      elseif(_resArg MATCHES "^FLAGS$")
        set(_appendFlags YES)
        set(_parseState 9)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_resArg}\".")
      endif()
    elseif(_parseState EQUAL 5)
      if(_resArg MATCHES "^APPEND$")
        set(_parseState 6)
      elseif(_resArg MATCHES "^DEFINES$")
        set(_parseState 7)
      elseif(_resArg MATCHES "^FLAGS$")
        set(_parseState 9)
      else()
        list(APPEND _includeDirs "${_resArg}")
      endif()
    elseif(_parseState EQUAL 6)
      if(_resArg MATCHES "^DEFINES$")
        set(_appendDefines YES)
        set(_parseState 7)
      elseif(_resArg MATCHES "^FLAGS$")
        set(_appendFlags YES)
        set(_parseState 9)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_resArg}\".")
      endif()
    elseif(_parseState EQUAL 7)
      if(_resArg MATCHES "^APPEND$")
        set(_parseState 8)
      elseif(_resArg MATCHES "^FLAGS$")
        set(_parseState 9)
      else()
        list(APPEND _defines "${_resArg}")
      endif()
    elseif(_parseState EQUAL 8)
      if(_resArg MATCHES "^FLAGS$")
        set(_appendFlags YES)
        set(_parseState 9)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_resArg}\".")
      endif()
    elseif(_parseState EQUAL 9)
      list(APPEND _flags "${_resArg}")
    elseif(_parseState EQUAL 100)
      if(_resArg MATCHES "^FILE$")
        set(_parseState 2)
      else()
        list(APPEND _importResIds "${_resArg}")
      endif()
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_resArg}\".")
    endif()
  endforeach()
  if(NOT ((_parseState EQUAL 3) OR (_parseState EQUAL 5) OR (_parseState EQUAL 7) OR (_parseState EQUAL 9) OR (_parseState EQUAL 100)))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()

  if(DEFINED _rcFiles)
    list(REMOVE_DUPLICATES _rcFiles)
  endif()
  if(DEFINED _importResIds)
    list(REMOVE_DUPLICATES _importResIds)
  endif()

  set_property(GLOBAL PROPERTY "${_rcFilesPropName}"           "${_rcFiles}")
  set_property(GLOBAL PROPERTY "${_includeDirsPropName}"       "${_includeDirs}")
  set_property(GLOBAL PROPERTY "${_definesPropName}"           "${_defines}")
  set_property(GLOBAL PROPERTY "${_flagsPropName}"             "${_flags}")
  set_property(GLOBAL PROPERTY "${_importResIdsPropName}"      "${_importResIds}")
  set_property(GLOBAL PROPERTY "${_appendIncludeDirsPropName}" "${_appendIncludeDirs}")
  set_property(GLOBAL PROPERTY "${_appendDefinesPropName}"     "${_appendDefines}")
  set_property(GLOBAL PROPERTY "${_appendFlagsPropName}"       "${_appendFlags}")

  math(EXPR _rcIdx "${_rcIdx} + 1")
  set_property(GLOBAL PROPERTY "${_rcIdxPropName}" "${_rcIdx}")
endfunction()


# Preconfigures and returns names of resource files connected to specified resource identifier.
#
# Preconfiguration sets additional definitions, include directories, etc. for each resource file.
# The returned files can be used only in current directory targets.
#
# @param rcFilesVarName Name of variable placeholder where names of resource files will be returned.
# @param resourceId     Idenfifier of resource file or of group of resource files.
function(igc_rc_get_resource rcFilesVarName resourceId)
  if(NOT ("${resourceId}" MATCHES "[a-zA-Z0-9_]+"))
    message(FATAL_ERROR "The resource identifier \"${resourceId}\" is invalid.")
  endif()

  set(_rcPropPrefix "IGC_PROPERTY__RC_RESOURCES_R${resourceId}_")
  set(_rcIdxPropName "${_rcPropPrefix}INDEX")

  # Preserve resource index to support multiple registration calls.
  get_property(_rcIdxSet GLOBAL PROPERTY "${_rcIdxPropName}" SET)
  if(_rcIdxSet)
    get_property(_rcIdx GLOBAL PROPERTY "${_rcIdxPropName}")
  else()
    message(AUTHOR_WARNING "Resource connected to \"${resourceId}\" not found.")
    set(_rcIdx 0)
  endif()

  set(_targetRcFiles)

  if(_rcIdx GREATER 0)
    math(EXPR _rcIdxM1 "${_rcIdx} - 1")

    foreach(_idx RANGE 0 "${_rcIdxM1}")
      set(_rcFilesPropName           "${_rcPropPrefix}G${_idx}_FILES")
      set(_includeDirsPropName       "${_rcPropPrefix}G${_idx}_INCDIRS")
      set(_definesPropName           "${_rcPropPrefix}G${_idx}_DEFINES")
      set(_flagsPropName             "${_rcPropPrefix}G${_idx}_FLAGS")
      set(_importResIdsPropName      "${_rcPropPrefix}G${_idx}_IMPORTED_RES_IDS")
      set(_appendIncludeDirsPropName "${_includeDirsPropName}_APPEND")
      set(_appendDefinesPropName     "${_definesPropName}_APPEND")
      set(_appendFlagsPropName       "${_flagsPropName}_APPEND")

      get_property(_rcFiles           GLOBAL PROPERTY "${_rcFilesPropName}")
      get_property(_includeDirs       GLOBAL PROPERTY "${_includeDirsPropName}")
      get_property(_defines           GLOBAL PROPERTY "${_definesPropName}")
      get_property(_flags             GLOBAL PROPERTY "${_flagsPropName}")
      get_property(_importResIds      GLOBAL PROPERTY "${_importResIdsPropName}")
      get_property(_appendIncludeDirs GLOBAL PROPERTY "${_appendIncludeDirsPropName}")
      get_property(_appendDefines     GLOBAL PROPERTY "${_appendDefinesPropName}")
      get_property(_appendFlags       GLOBAL PROPERTY "${_appendFlagsPropName}")

      set(_includeDirFlags)
      foreach(_includeDir ${_includeDirs})
        string(REPLACE ";" "\;" _includeDir "${_includeDir}") # [WA#1] Must escape ; again if occurred in item.
        if(DEFINED _includeDirFlags)
          set(_includeDirFlags "${_includeDirFlags} /I\"${_includeDir}\"")
        else()
          set(_includeDirFlags "/I\"${_includeDir}\"")
        endif()
      endforeach()

      set(_flagFlags)
      foreach(_flag ${_flags})
        string(REPLACE ";" "\;" _flag "${_flag}") # [WA#1] Must escape ; again if occurred in item.
        if(DEFINED _flagFlags)
          set(_flagFlags "${_flagFlags} ${_flag}")
        else()
          set(_flagFlags "${_flag}")
        endif()
      endforeach()

      if(DEFINED _flagFlags)
        set(_allFlags "${_flagFlags} ${_includeDirFlags}")
      else()
        set(_allFlags "${_includeDirFlags}")
      endif()

      foreach(_rcFile ${_rcFiles})
        string(REPLACE ";" "\;" _rcFile "${_rcFile}") # [WA#1] Must escape ; again if occurred in item.

        if(_appendDefines)
          set_property(SOURCE "${_rcFile}" APPEND PROPERTY COMPILE_DEFINITIONS "${_defines}")
        else()
          set_property(SOURCE "${_rcFile}" PROPERTY COMPILE_DEFINITIONS "${_defines}")
        endif()

        # NOTE: Currently there is no include dirs (per source), so appending options are limited.
        if(_appendFlags OR _appendIncludeDirs)
          set_property(SOURCE "${_rcFile}" APPEND PROPERTY COMPILE_FLAGS "${_allFlags}")
        else()
          set_property(SOURCE "${_rcFile}" PROPERTY COMPILE_FLAGS "${_allFlags}")
        endif()

        list(APPEND _targetRcFiles "${_rcFile}")
      endforeach()

      foreach(_importResId ${_importResIds})
        string(REPLACE ";" "\;" _importResId "${_importResId}") # [WA#1] Must escape ; again if occurred in item.

        set(_importedResources)
        igc_rc_get_resource(_importedResources "${_importResId}")
        list(APPEND _targetRcFiles "${_importedResources}")
      endforeach()
    endforeach()
  endif()

  if(DEFINED _targetRcFiles)
    list(REMOVE_DUPLICATES _targetRcFiles)
  endif()
  set("${rcFilesVarName}" "${_targetRcFiles}" PARENT_SCOPE)
endfunction()

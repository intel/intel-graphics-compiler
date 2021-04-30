#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#============================== Source groups ==================================

include_guard(DIRECTORY)

# Register source group (advanced version).
#
# igc_sg_register(
#     srcGrpId
#     filterLabel
#     [GROUPS [childSrcGrpId [childSrcGrpId [...]]]]
#     [FILES  [srcFile [srcFile [...]]]]
#     [REGULAR_EXPRESSION [srcRe [srcRe [...]]]]
#     [...]
#     [{GROUPS|FILES|REGULAR_EXPRESSION} ...]
#   )
#
# @param srcGrpId      Source group to register.
# @param filterLabel   Label of filter for source group. Last registered label is used.
# @param childSrcGrpId Child source group. Their filter label will be prepended with filter labels
#                      of ancestors source groups.
# @param srcFile       Source files registered in the group. (Last source group match will be used though.)
# @param srcRe         Regular expression that identify files added to source group.
function(igc_sg_register srcGrpId filterLabel)
  set(_sgPropPrefix "IGC_PROPERTY__SRC_GROUPS_SG${srcGrpId}_")
  set(_sgFilterPropName "${_sgPropPrefix}FILTER")
  set(_sgGroupsPropName "${_sgPropPrefix}GROUPS")
  set(_sgFilesPropName  "${_sgPropPrefix}FILES")
  set(_sgRePropName     "${_sgPropPrefix}REGEX")

  set_property(GLOBAL PROPERTY "${_sgFilterPropName}" "${filterLabel}")
  get_property(_sgGroups GLOBAL PROPERTY "${_sgGroupsPropName}")
  get_property(_sgFiles  GLOBAL PROPERTY "${_sgFilesPropName}")
  get_property(_sgRe     GLOBAL PROPERTY "${_sgRePropName}")

  set(_parseState 2)
  foreach(_sgArg ${ARGN})
    string(REPLACE ";" "\;" _sgArg "${_sgArg}") # [WA#1] Must escape ; again if occurred in item.

    # States: [0] <param> [1] <param> [2] *( ( "GROUPS" | "FILES" | "REGULAR_EXPRESSION" ) [3] *<param> [3] ) )
    # Transitions: 0 -> 1 -> 2 // by explict parameters
    #              2 (GROUPS) -> 3
    #              2 (FILES) -> 3
    #              2 (REGULAR_EXPRESSION) -> 3
    #              3 (GROUPS) -> 3
    #              3 (FILES) -> 3
    #              3 (REGULAR_EXPRESSION) -> 3
    #              3 -> 3
    # Stop States: 2, 3
    if(_parseState EQUAL 2)
      if(_sgArg MATCHES "^GROUPS$")
        set(_sgCollection "_sgGroups")
        set(_parseState 3)
      elseif(_sgArg MATCHES "^FILES$")
        set(_sgCollection "_sgFiles")
        set(_parseState 3)
      elseif(_sgArg MATCHES "^REGULAR_EXPRESSION$")
        set(_sgCollection "_sgRe")
        set(_parseState 3)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_sgArg}\".")
      endif()
    elseif(_parseState EQUAL 3)
      if(_sgArg MATCHES "^GROUPS$")
        set(_sgCollection "_sgGroups")
      elseif(_sgArg MATCHES "^FILES$")
        set(_sgCollection "_sgFiles")
      elseif(_sgArg MATCHES "^REGULAR_EXPRESSION$")
        set(_sgCollection "_sgRe")
      else()
        list(APPEND "${_sgCollection}" "${_sgArg}")
      endif()
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_sgArg}\".")
    endif()
  endforeach()
  if(NOT ((_parseState EQUAL 2) OR (_parseState EQUAL 3)))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()

  if(DEFINED _sgGroups)
    list(REMOVE_DUPLICATES _sgGroups)
  endif()
  if(DEFINED _sgFiles)
    list(REMOVE_DUPLICATES _sgFiles)
  endif()
  if(DEFINED _sgRe)
    list(REMOVE_DUPLICATES _sgRe)
  endif()

  set_property(GLOBAL PROPERTY "${_sgGroupsPropName}" "${_sgGroups}")
  set_property(GLOBAL PROPERTY "${_sgFilesPropName}"  "${_sgFiles}")
  set_property(GLOBAL PROPERTY "${_sgRePropName}"     "${_sgRe}")
endfunction()

# Defines filter in source group for specific source directory.
#
# @param srcGrpId Registered source group used to create filter definitions.
function(igc_sg_define srcGrpId)
  set(_sgPropPrefix "IGC_PROPERTY__SRC_GROUPS_SG${srcGrpId}_")
  set(_sgFilterPropName "${_sgPropPrefix}FILTER")
  set(_sgGroupsPropName "${_sgPropPrefix}GROUPS")
  set(_sgFilesPropName  "${_sgPropPrefix}FILES")
  set(_sgRePropName     "${_sgPropPrefix}REGEX")

  get_property(_sgFilterSet GLOBAL PROPERTY "${_sgFilterPropName}" SET)
  if(NOT _sgFilterSet)
    return()
  endif()

  list(LENGTH ARGN _paramsCount)
  if(_paramsCount GREATER 0)
    list(GET ARGN 0 _sgFilterPrefix)
    string(REPLACE ";" "\;" _sgFilterPrefix "${_sgFilterPrefix}\\") # [WA#1] Must escape ; again if occurred in item.
  else()
    set(_sgFilterPrefix)
  endif()

  get_property(_sgFilter GLOBAL PROPERTY "${_sgFilterPropName}")
  get_property(_sgGroups GLOBAL PROPERTY "${_sgGroupsPropName}")
  get_property(_sgFiles  GLOBAL PROPERTY "${_sgFilesPropName}")
  get_property(_sgRe     GLOBAL PROPERTY "${_sgRePropName}")

  foreach(_sgReItem ${_sgRe})
    string(REPLACE ";" "\;" _sgReItem "${_sgReItem}") # [WA#1] Must escape ; again if occurred in item.
    source_group("${_sgFilterPrefix}${_sgFilter}" REGULAR_EXPRESSION "${_sgReItem}")
  endforeach()
  source_group("${_sgFilterPrefix}${_sgFilter}" FILES ${_sgFiles})

  foreach(_sgGroup ${_sgGroups})
    string(REPLACE ";" "\;" _sgGroup "${_sgGroup}") # [WA#1] Must escape ; again if occurred in item.
    igc_sg_define("${_sgGroup}" "${_sgFilterPrefix}${_sgFilter}")
  endforeach()
endfunction()

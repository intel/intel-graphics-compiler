#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#====================================== Flags ==================================

include_guard(DIRECTORY)

# Escapes text for regular expressions.
#
# @param retValName Name of variable placeholder where result will be returned.
# @param text       Text to escape.
function(igc_regex_escape retVarName text)
  string(REGEX REPLACE "\\^|\\$|\\.|\\-|\\[|\\]|\\(|\\)|\\+|\\*|\\?|\\||\\\\" "\\\\\\0" _escapedText "${text}")
  set("${retVarName}" "${_escapedText}" PARENT_SCOPE)
endfunction(igc_regex_escape)

# Removes flags from variable which match specified regular expression.
#
# @param flagsVarName      Name of variable with flags. The variable will be updated.
# @param [flag [flag ...]] Regular expressions which describe flags to remove.
function(igc_flag_remove_re flagsVarName)
  set(_updatedFlags "${${flagsVarName}}")
  foreach(_flag ${ARGN})
    string(REPLACE ";" "\;" _flag "${_flag}") # [WA#1] Must escape ; again if occurred in flag.
    string(REGEX REPLACE "([ ]+|^)(${_flag})([ ]+|$)" " " _updatedFlags "${_updatedFlags}")
  endforeach()
  string(STRIP "${_updatedFlags}" _updatedFlags)

  if(_updatedFlags MATCHES "^$") # [WA#3] Empty string sometimes unsets variable which can lead to reading of backing cached variable with the same name.
    set("${flagsVarName}" " " PARENT_SCOPE)
  else()
    set("${flagsVarName}" "${_updatedFlags}" PARENT_SCOPE)
  endif()
endfunction(igc_flag_remove_re)

# Adds flags to variable. If flag is in variable it is omitted.
#
# @param flagsVarName      Name of variable with flags. The variable will be updated.
# @param [flag [flag ...]] Flags which will be added to variable (if they are not in it already).
function(igc_flag_add_once flagsVarName)
  set(_updatedFlags "${${flagsVarName}}")
  foreach(_flag ${ARGN})
    string(REPLACE ";" "\;" _flag "${_flag}") # [WA#1] Must escape ; again if occurred in flag.
    igc_regex_escape(_escapedFlag "${_flag}")
    if(NOT (_updatedFlags MATCHES "([ ]+|^)(${_escapedFlag})([ ]+|$)"))
      set(_updatedFlags "${_updatedFlags} ${_flag}")
    endif()
  endforeach()
  string(STRIP "${_updatedFlags}" _updatedFlags)

  set("${flagsVarName}" "${_updatedFlags}" PARENT_SCOPE)
endfunction(igc_flag_add_once)

# Removes flags from variable which match specified flag.
#
# @param flagsVarName      Name of variable with flags. The variable will be updated.
# @param [flag [flag ...]] Strings which are equal to flags to remove.
function(igc_flag_remove flagsVarName)
  set(_updatedFlags "${${flagsVarName}}")
  foreach(_flag ${ARGN})
    string(REPLACE ";" "\;" _flag "${_flag}") # [WA#1] Must escape ; again if occurred in flag.
    igc_regex_escape(_escapedFlag "${_flag}")
    igc_flag_remove_re(_updatedFlags "${_escapedFlag}")
  endforeach()

  set("${flagsVarName}" "${_updatedFlags}" PARENT_SCOPE)
endfunction(igc_flag_remove)

# Adds flags to flag property of target (if they are not added already).
#
# @param targetName        Name of target.
# @param propertyName      String with property name.
# @param [flag [flag ...]] Flags which will be added to property (if they are not in it already).
function(igc_target_flag_property_add_once targetName propertyName)
  get_property(_flagsExist TARGET "${targetName}" PROPERTY "${propertyName}" DEFINED)
  if(NOT _flagsExist)
    # If the property doesn't exist for the target define it
    define_property(TARGET PROPERTY "${propertyName}" BRIEF_DOCS "${propertyName}" FULL_DOCS "${propertyName}")
  endif()

  get_property(_flags TARGET "${targetName}" PROPERTY "${propertyName}")
  igc_flag_add_once(_flags "${ARGN}") # [WA#2] To handle ; correctly some lists must be put in string form.
  set_property(TARGET "${targetName}" PROPERTY "${propertyName}" "${_flags}")
endfunction()

# Adds flags to flag property of target (if they are not added already).
#
# Flags are read from variable (if defined). The base property is updated by base variable and
# all per-configuration properties (with _<CONFIG> suffix) are updated by variables named from
# base name with _<CONFIG> suffix appended.
#
# @param targetName        Name of target.
# @param propertyBaseName  String with property name where add flags to (base name).
# @param varBaseName       Variable name where list of flags is stored (base name).
function(igc_target_flag_property_add_once_config_var targetName propertyBaseName varBaseName)
  if(DEFINED "${varBaseName}")
    igc_target_flag_property_add_once("${targetName}" "${propertyBaseName}" "${${varBaseName}}")  # [WA#2] To handle ; correctly some lists must be put in string form.
  endif()

  foreach(_configName ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(REPLACE ";" "\;" _configName "${_configName}") # [WA#1] Must escape ; again if occurred in item.
    string(TOUPPER "${_configName}" _upperConfigName)
    set(_propertyName  "${propertyBaseName}_${_upperConfigName}")
    set(_varName       "${varBaseName}_${_upperConfigName}")

    if(DEFINED "${_varName}")
      igc_target_flag_property_add_once("${targetName}" "${_propertyName}" "${${_varName}}") # [WA#2] To handle ; correctly some lists must be put in string form.
    endif()

    unset(_upperConfigName)
    unset(_propertyName)
    unset(_varName)
  endforeach()
endfunction()

# Registers setting and relations on flag options which are usually connected to compiler / linker options.
#
# Settings on the same domain can be set on different calls of this function. Groups with the same
# name will be overwritten (by last GROUP entry of last call of function).
# The same overwrite behavior is defined for aliases with the same name.
#
# igc_flag_register_settings(
#     settingsDomainName
#     [GROUP [NO_REGEX] [NAME groupName] groupedFlag [groupedFlag [...]]]
#     [GROUP [NO_REGEX] [NAME groupName] groupedFlag [groupedFlag [...]]]
#     [...]
#     [ALIAS aliasName [aliasedFlag [{ALLOW_MULTIPLE | REMOVE_GROUP }]]]
#     [ALIAS aliasName [aliasedFlag [{ALLOW_MULTIPLE | REMOVE_GROUP }]]]
#     [...]
#   )
#
# GROUP groups mutually exclusive flags. Only one flag from the group can be applied. NO_REGEX indicates
# that grouped flags are described directly (not as regular expression patterns). NAME allows to create
# named group.
#
# ALIAS allows to apply flags using unified name. ALLOW_MULTIPLE indicates that flag can be applied multiple times.
# ALLOW_MULTIPLE only works for non-grouped aliased flags. REMOVE_GROUP treats aliasedFlag as group name which
# flags will be removed when alias will be applied.
#
# @param settingsDomainName Domain name for settings / relations. The domain name allows to differentate and
#                           to create multiple sets of relations. All operations on settings functions use
#                           domain name to identify relations domain.
# @param groupName          Optional name of the group. Named groups can be overwritten.
# @param groupedFlag        Flags which are defined in specified group. Flags in the group are treated as
#                           mutually exclusive.
# @param aliasName          Name of alias for the flag.
# @param aliasedFlag        Raw value of flag. It contains group name when REMOVE_GROUP is specified.
function(igc_flag_register_settings settingsDomainName)
  set(_settingsPropPrefix    "IGC_PROPERTY__FLAG_SETTINGS_D${settingsDomainName}_")
  set(_namedGroupsPropName   "${_settingsPropPrefix}PNAMED_GROUPS")
  set(_groupIdxCountPropName "${_settingsPropPrefix}PGROUP_INDEX")
  set(_aliasesPropName       "${_settingsPropPrefix}PALIASES")

  # Preserve group index to support multiple registration calls.
  get_property(_groupIdxSet GLOBAL PROPERTY "${_groupIdxCountPropName}" SET)
  if(_groupIdxSet)
    get_property(_groupIdx GLOBAL PROPERTY "${_groupIdxCountPropName}")
  else()
    set(_groupIdx 0)
    set_property(GLOBAL PROPERTY "${_groupIdxCountPropName}" "${_groupIdx}")
  endif()
  # Use named groups to verify connections.
  get_property(_namedGroups GLOBAL PROPERTY "${_namedGroupsPropName}")

  set(_parseState 1)
  foreach(_settingsArg ${ARGN})
    string(REPLACE ";" "\;" _settingsArg "${_settingsArg}")  # [WA#1] Must escape ; again if occurred in item.

    # States: [0] <param> [1] *( "GROUP" [2] *1( "NO_REGEX" [3] ) *1( "NAME" [4] <param> [5] ) <param> [6] *<param> [6] )
    #         *( "ALIAS" [7] <param> [8] *1( <param> [9] *<param> [9] *1( { "ALLOW_MULTIPLE" | "REMOVE_GROUP" } [10] ) ) )
    # Transitions: 0 -> 1 // by explict parameter
    #              1 (GROUP) -> 2
    #              1 (ALIAS) -> 7
    #              2 (NO_REGEX) -> 3
    #              2 (NAME) -> 4
    #              2 -> 6
    #              3 (NAME) -> 4
    #              3 -> 6
    #              4 -> 5 -> 6
    #              6 (GROUP) -> 2
    #              6 (ALIAS) -> 7
    #              6 -> 6
    #              7 -> 8
    #              8 (ALIAS) -> 7
    #              8 -> 9
    #              9 (ALIAS) -> 7
    #              9 (ALLOW_MULTIPLE) -> 10 (ALIAS) -> 7
    #              9 (REMOVE_GROUP)   -> 10 (ALIAS) -> 7
    #              9 -> 9
    # Stop States: 1, 6, 8, 9, 10
    if(_parseState EQUAL 1)
      if(_settingsArg MATCHES "^GROUP$")
        set(_parseState 2)
      elseif(_settingsArg MATCHES "^ALIAS$")
        set(_parseState 7)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_settingsArg}\".")
      endif()
    elseif(_parseState EQUAL 2)
      set(_groupUseRe YES)
      set(_namedGroup NO)
      set(_groupName "I${_groupIdx}")
      set(_groupedFlagsPropName "${_settingsPropPrefix}G${_groupName}_FLAGS")

      math(EXPR _groupIdx "${_groupIdx} + 1")

      if(_settingsArg MATCHES "^NO_REGEX$")
        set(_groupUseRe NO)
        set(_parseState 3)
      elseif(_settingsArg MATCHES "^NAME$")
        set(_parseState 4)
      else()
        if(NOT _groupUseRe)
          igc_regex_escape(_settingsArg "${_settingsArg}")
        endif()
        set_property(GLOBAL PROPERTY "${_groupedFlagsPropName}" "${_settingsArg}")
        set(_parseState 6)
      endif()
    elseif(_parseState EQUAL 3)
      if(_settingsArg MATCHES "^NAME$")
        set(_parseState 4)
      else()
        if(NOT _groupUseRe)
          igc_regex_escape(_settingsArg "${_settingsArg}")
        endif()
        set_property(GLOBAL PROPERTY "${_groupedFlagsPropName}" "${_settingsArg}")
        set(_parseState 6)
      endif()
    elseif(_parseState EQUAL 4)
      set(_namedGroup YES)
      set(_groupName "N${_settingsArg}")
      set(_groupedFlagsPropName "${_settingsPropPrefix}G${_groupName}_FLAGS")

      math(EXPR _groupIdx "${_groupIdx} - 1") # Named group does not have index identifier (we can reuse pre-allocated index).

      set(_parseState 5)
    elseif(_parseState EQUAL 5)
      if(NOT _groupUseRe)
        igc_regex_escape(_settingsArg "${_settingsArg}")
      endif()
      set_property(GLOBAL PROPERTY "${_groupedFlagsPropName}" "${_settingsArg}")
      set(_parseState 6)
    elseif(_parseState EQUAL 6)
      # Updating list of named groups or next available index (for unnamed groups).
      # This action should be triggered at transition to state 6 which is Stop state, so the action must be also when there is no more parameters.
      if(_namedGroup)
        list(FIND _namedGroups "${_groupName}" _namedGroupIdx)
        if(_namedGroupIdx LESS 0)
          set_property(GLOBAL APPEND PROPERTY "${_namedGroupsPropName}" "${_groupName}")
          list(APPEND _namedGroups "${_groupName}")
        endif()
      else()
        set_property(GLOBAL PROPERTY "${_groupIdxCountPropName}" "${_groupIdx}")
      endif()

      if(_settingsArg MATCHES "^GROUP$")
        set(_parseState 2)
      elseif(_settingsArg MATCHES "^ALIAS$")
        set(_parseState 7)
      else()
        if(NOT _groupUseRe)
          igc_regex_escape(_settingsArg "${_settingsArg}")
        endif()
        set_property(GLOBAL APPEND PROPERTY "${_groupedFlagsPropName}" "${_settingsArg}")
      endif()
    elseif(_parseState EQUAL 7)
      set(_aliasName "${_settingsArg}")
      set(_aliasRawPropName        "${_settingsPropPrefix}A${_aliasName}_RAW")
      set(_aliasRGroupPropName     "${_settingsPropPrefix}A${_aliasName}_REMOVE_GROUP")
      set(_aliasAllowMultiPropName "${_settingsPropPrefix}A${_aliasName}_ALLOW_MULTIPLE")

      get_property(_aliases GLOBAL PROPERTY "${_aliasesPropName}")
      list(FIND _aliases "${_aliasName}" _aliasIdx)
      if(_aliasIdx LESS 0)
        set_property(GLOBAL APPEND PROPERTY "${_aliasesPropName}" "${_aliasName}")
      endif()

      set_property(GLOBAL PROPERTY "${_aliasRawPropName}")
      set_property(GLOBAL PROPERTY "${_aliasRGroupPropName}")
      set_property(GLOBAL PROPERTY "${_aliasAllowMultiPropName}" NO)

      set(_parseState 8)
    elseif(_parseState EQUAL 8)
      if(_settingsArg MATCHES "^ALIAS$")
        set(_parseState 7)
      else()
        set_property(GLOBAL PROPERTY "${_aliasRawPropName}" "${_settingsArg}")
        set(_parseState 9)
      endif()
    elseif(_parseState EQUAL 9)
      if(_settingsArg MATCHES "^ALIAS$")
        set(_parseState 7)
      elseif(_settingsArg MATCHES "^ALLOW_MULTIPLE$")
        set_property(GLOBAL PROPERTY "${_aliasAllowMultiPropName}" YES)
        set(_parseState 10)
      elseif(_settingsArg MATCHES "^REMOVE_GROUP$")
        get_property(_groupsToRemove GLOBAL PROPERTY "${_aliasRawPropName}")
        set_property(GLOBAL PROPERTY "${_aliasRawPropName}")
        foreach(_groupToRemove ${_groupsToRemove})
          string(REPLACE ";" "\;" _groupToRemove "${_groupToRemove}") # [WA#1] Must escape ; again if occurred in item.
          list(FIND _namedGroups "N${_groupToRemove}" _namedGroupIdx)
          if(_namedGroupIdx LESS 0)
            message(WARNING "Named group \"${_groupToRemove}\" referenced in \"${_aliasName}\" alias does not exist yet.")
          endif()
          set_property(GLOBAL APPEND PROPERTY "${_aliasRGroupPropName}" "N${_groupToRemove}")
        endforeach()
        set(_parseState 10)
      else()
        set_property(GLOBAL APPEND PROPERTY "${_aliasRawPropName}" "${_settingsArg}")
      endif()
    elseif(_parseState EQUAL 10)
      if(_settingsArg MATCHES "^ALIAS$")
        set(_parseState 7)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_settingsArg}\".")
      endif()
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_settingsArg}\".")
    endif()
  endforeach()
  if(_parseState EQUAL 6)
    # Updating list of named groups or next available index (for unnamed groups).
    # This action should be triggered at transition to state 6 which is Stop state, so the action must be also when there is no more parameters.
    if(_namedGroup)
      list(FIND _namedGroups "${_groupName}" _namedGroupIdx)
      if(_namedGroupIdx LESS 0)
        set_property(GLOBAL APPEND PROPERTY "${_namedGroupsPropName}" "${_groupName}")
        list(APPEND _namedGroups "${_groupName}")
      endif()
    else()
      set_property(GLOBAL PROPERTY "${_groupIdxCountPropName}" "${_groupIdx}")
    endif()
  endif()
  if(NOT ((_parseState EQUAL 1) OR (_parseState EQUAL 6) OR (_parseState EQUAL 8) OR (_parseState EQUAL 9) OR (_parseState EQUAL 10)))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()
endfunction()

# Applies settings to flag variables. Settings are applied according to registered configuration
# (by igc_flag_register_settings).
#
# igc_flag_apply_settings(
#     settingsDomainName
#     flagsMainVarName
#     [FLAG         flagsVarName [flagsVarName [...]]]
#     [FLAG         flagsVarName [flagsVarName [...]]]
#     [...]
#     [SET          flag [flag [...]]]
#     [SET_RAW      flag [flag [...]]]
#     [REMOVE_GROUP groupName [groupName [...]]]
#     [{SET|SET_RAW|REMOVE_GROUP} ...]
#     [...]
#   )
#
# Allowed operation to apply:
# SET          - sets flag (takes into consideration aliases).
# SET_RAW      - sets flag (does NOT take aliases into consideration).
# REMOVE_GROUP - removes all flags identified by specified group.
# Operations are applied in definition order.
#
# @param settingsDomainName Domain name for settings / relations. The domain name allows to differentate and
#                           to create multiple sets of relations. All operations on settings functions use
#                           domain name to identify relations domain.
# @param flagsMainVarName   Name of main variable for flags. Any added flags are added to main variable.
# @param flagsVarName       Names of any additional flag variables which will be cleaned up from mutually
#                           exclusive flags (or from selected groups of flags).
# @param flag               Flags to set (SET or SET_RAW).
# @param groupName          Names of groups of flags to remove (REMOVE_GROUP).
function(igc_flag_apply_settings settingsDomainName flagsMainVarName)
  set(_settingsPropPrefix    "IGC_PROPERTY__FLAG_SETTINGS_D${settingsDomainName}_")
  set(_namedGroupsPropName   "${_settingsPropPrefix}PNAMED_GROUPS")
  set(_groupIdxCountPropName "${_settingsPropPrefix}PGROUP_INDEX")
  set(_aliasesPropName       "${_settingsPropPrefix}PALIASES")

  get_property(_domainExists GLOBAL PROPERTY "${_groupIdxCountPropName}" SET)
  if(NOT _domainExists)
    message(FATAL_ERROR "Settings domain \"${settingsDomainName}\" does not exist.")
  endif()

  set(_flagVarNames "${flagsMainVarName}")
  set(_operations)
  set(_flagsOrGroups)

  set(_parseState 2)
  foreach(_settingsArg ${ARGN})
    string(REPLACE ";" "\;" _settingsArg "${_settingsArg}") # [WA#1] Must escape ; again if occurred in item.

    # States: [0] <param> [1] <param> [2] *( "FLAG" [3] <param> [4] *<param> [4] ) *( ( "SET" | "SET_RAW" | "REMOVE_GROUP" ) [5] <param> [6] *<param> [6] )
    # Transitions: 0 -> 1 -> 2 // by explict parameters
    #              2 (FLAG) -> 3
    #              2 (SET|SET_RAW|REMOVE_GROUP) -> 5
    #              3 -> 4
    #              4 (FLAG) -> 3
    #              4 (SET|SET_RAW|REMOVE_GROUP) -> 5
    #              4 -> 4
    #              5 -> 6
    #              6 (SET|SET_RAW|REMOVE_GROUP) -> 5
    #              6 -> 6
    # Stop States: 2, 4, 6
    if(_parseState EQUAL 2)
      if(_settingsArg MATCHES "^FLAG$")
        set(_parseState 3)
      elseif(_settingsArg MATCHES "^SET|SET_RAW|REMOVE_GROUP$")
        set(_opType "${CMAKE_MATCH_0}")
        set(_parseState 5)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_settingsArg}\".")
      endif()
    elseif(_parseState EQUAL 3)
      list(APPEND _flagVarNames "${_settingsArg}")
      set(_parseState 4)
    elseif(_parseState EQUAL 4)
      if(_settingsArg MATCHES "^FLAG$")
        set(_parseState 3)
      elseif(_settingsArg MATCHES "^SET|SET_RAW|REMOVE_GROUP$")
        set(_opType "${CMAKE_MATCH_0}")
        set(_parseState 5)
      else()
        list(APPEND _flagVarNames "${_settingsArg}")
      endif()
    elseif(_parseState EQUAL 5)
      list(APPEND _operations    "${_opType}")
      list(APPEND _flagsOrGroups "${_settingsArg}")
      set(_parseState 6)
    elseif(_parseState EQUAL 6)
      if(_settingsArg MATCHES "^SET|SET_RAW|REMOVE_GROUP$")
        set(_opType "${CMAKE_MATCH_0}")
        set(_parseState 5)
      else()
        list(APPEND _operations    "${_opType}")
        list(APPEND _flagsOrGroups "${_settingsArg}")
      endif()
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_settingsArg}\".")
    endif()
  endforeach()
  if(NOT ((_parseState EQUAL 2) OR (_parseState EQUAL 4) OR (_parseState EQUAL 6)))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()

  set(_updatedFlagsMainVarName "_updatedFlags_${flagsMainVarName}")
  foreach(_flagVarName ${_flagVarNames})
    string(REPLACE ";" "\;" _flagVarName "${_flagVarName}")
    set("_updatedFlags_${_flagVarName}" "${${_flagVarName}}")
  endforeach()

  get_property(_groupIdx    GLOBAL PROPERTY "${_groupIdxCountPropName}") # Next available index for unnamed group.
  get_property(_namedGroups GLOBAL PROPERTY "${_namedGroupsPropName}")
  get_property(_aliases     GLOBAL PROPERTY "${_aliasesPropName}")
  set(_groups "${_namedGroups}")
  if(_groupIdx GREATER 0)
    math(EXPR _groupIdxM1 "${_groupIdx} - 1")
    foreach(_idx RANGE 0 ${_groupIdxM1})
      list(APPEND _groups "I${_idx}")
    endforeach()
  endif()

  set(_operationIdx 0)
  foreach(_operation ${_operations}) # Operation type does not have ; -> no need for [WA#1]
    list(GET _flagsOrGroups ${_operationIdx} _flagOrGroup)
    string(REPLACE ";" "\;" _flagOrGroup "${_flagOrGroup}") # [WA#1] Must escape ; again if occurred in item.

    set(_groupsToRemove)
    set(_flagsToAdd)

    set(_aliasRawPropName        "${_settingsPropPrefix}A${_flagOrGroup}_RAW")
    set(_aliasRGroupPropName     "${_settingsPropPrefix}A${_flagOrGroup}_REMOVE_GROUP")
    set(_aliasAllowMultiPropName "${_settingsPropPrefix}A${_flagOrGroup}_ALLOW_MULTIPLE")

    # Removing aliases and splitting operations into remove group/add flag categories.
    if(_operation MATCHES "^SET$")
      list(FIND _aliases "${_flagOrGroup}" _aliasIdx)
      if(_aliasIdx LESS 0)
        list(APPEND _flagsToAdd "${_flagOrGroup}")
      else()
        get_property(_rawValueSet    GLOBAL PROPERTY "${_aliasRawPropName}" SET)
        get_property(_removeGroupSet GLOBAL PROPERTY "${_aliasRGroupPropName}" SET)
        get_property(_allowMultiple  GLOBAL PROPERTY "${_aliasAllowMultiPropName}")

        if(_removeGroupSet)
          get_property(_removeGroup GLOBAL PROPERTY "${_aliasRGroupPropName}")
          list(APPEND _groupsToRemove "${_removeGroup}")
        elseif(_rawValueSet)
          get_property(_rawValue    GLOBAL PROPERTY "${_aliasRawPropName}")
          list(APPEND _flagsToAdd "${_rawValue}")
        endif()
      endif()
    elseif(_operation MATCHES "^SET_RAW$")
      list(APPEND _flagsToAdd "${_flagOrGroup}")
    elseif(_operation MATCHES "^REMOVE_GROUP$")
      list(APPEND _groupsToRemove "$N{_flagOrGroup}")
    endif()

    # Taking into consideration mutual exclusion groups.
    if((DEFINED _flagsToAdd) AND (NOT _allowMultiple))
      list(REMOVE_DUPLICATES _flagsToAdd)
    endif()

    foreach(_flagToAdd ${_flagsToAdd})
      string(REPLACE ";" "\;" _flagToAdd "${_flagToAdd}") # [WA#1] Must escape ; again if occurred in item.

      foreach(_group ${_groups})
        string(REPLACE ";" "\;" _group "${_group}") # [WA#1] Must escape ; again if occurred in item.

        set(_groupedFlagsPropName "${_settingsPropPrefix}G${_group}_FLAGS")

        get_property(_groupedFlags GLOBAL PROPERTY "${_groupedFlagsPropName}")
        foreach(_groupedFlag ${_groupedFlags})
          string(REPLACE ";" "\;" _groupedFlag "${_groupedFlag}") # [WA#1] Must escape ; again if occurred in item.

          if(_flagToAdd MATCHES "([ ]+|^)(${_groupedFlag})([ ]+|$)")
            list(APPEND _groupsToRemove "${_group}")
            break()
          endif()
        endforeach()
      endforeach()
    endforeach()

    # Removing all groups of mutually exclusive options that collide with added flags or
    # has been selected to remove.
    if(DEFINED _groupsToRemove)
      list(REMOVE_DUPLICATES _groupsToRemove)
    endif()

    #message("GR ---> ${_groupsToRemove}")
    #message("FA ---> ${_flagsToAdd}")

    foreach(_groupToRemove ${_groupsToRemove})
      string(REPLACE ";" "\;" _groupToRemove "${_groupToRemove}") # [WA#1] Must escape ; again if occurred in item.

      set(_groupedFlagsPropName "${_settingsPropPrefix}G${_groupToRemove}_FLAGS")

      list(FIND _groups "${_groupToRemove}" _groupToRemoveIdx)
      if(_groupToRemoveIdx LESS 0)
        string(REGEX REPLACE "^N" "" _groupToRemove "${_groupToRemove}")
        message(WARNING "Group of options to remove \"${_groupToRemove}\" cannot be found and will be omitted.")
      else()
        get_property(_groupedFlags GLOBAL PROPERTY "${_groupedFlagsPropName}")
        foreach(_flagVarName ${_flagVarNames})
          string(REPLACE ";" "\;" _flagVarName "${_flagVarName}") # [WA#1] Must escape ; again if occurred in item.

          igc_flag_remove_re("_updatedFlags_${_flagVarName}" "${_groupedFlags}") # [WA#2] To handle ; correctly some lists must be put in string form.
        endforeach()
      endif()
    endforeach()

    # Adding flags.
    if(NOT _allowMultiple)
      # If multiple flags are not allowed, the flags must be moved to main variable.
      foreach(_flagVarName ${_flagVarNames})
        string(REPLACE ";" "\;" _flagVarName "${_flagVarName}") # [WA#1] Must escape ; again if occurred in item.

        if(NOT (_flagVarName STREQUAL flagsMainVarName))
          igc_flag_remove("_updatedFlags_${_flagVarName}" "${_flagsToAdd}") # [WA#2] To handle ; correctly some lists must be put in string form.
        endif()
      endforeach()
      igc_flag_add_once("${_updatedFlagsMainVarName}" "${_flagsToAdd}") # [WA#2] To handle ; correctly some lists must be put in string form.
    else()
      foreach(_flagToAdd ${_flagsToAdd})
        string(REPLACE ";" "\;" _flagToAdd "${_flagToAdd}") # [WA#1] Must escape ; again if occurred in item.
        set("${_updatedFlagsMainVarName}" "${${_updatedFlagsMainVarName}} ${_flagToAdd}")
      endforeach()
    endif()

    math(EXPR _operationIdx "${_operationIdx} + 1")
  endforeach()

  # Returning flags.
  foreach(_flagVarName ${_flagVarNames})
    string(REPLACE ";" "\;" _flagVarName "${_flagVarName}")
    set("${_flagVarName}" "${_updatedFlags_${_flagVarName}}" PARENT_SCOPE)
  endforeach()
endfunction()

# Applies settings to configuration flag variable (variable which has per-configuration subvariables).
# Settings are applied according to registered configuration (by igc_flag_register_settings).
#
# igc_config_flag_apply_settings(
#     settingsDomainName
#     flagsVarBaseName
#     { PATTERN | NEG_PATTERN | ALL_PATTERN | ALL_PATTERN_NOINHERIT }
#     configPattern
#     [SET          flag [flag [...]]]
#     [SET_RAW      flag [flag [...]]]
#     [REMOVE_GROUP groupName [groupName [...]]]
#     [{SET|SET_RAW|REMOVE_GROUP} ...]
#     [...]
#   )
#
# Allowed operation to apply:
# SET          - sets flag (takes into consideration aliases).
# SET_RAW      - sets flag (does NOT take aliases into consideration).
# REMOVE_GROUP - removes all flags identified by specified group.
# Operations are applied in definition order.
#
# @param settingsDomainName Domain name for settings / relations. The domain name allows to differentate and
#                           to create multiple sets of relations. All operations on settings functions use
#                           domain name to identify relations domain.
# @param flagsVarBaseName   Base name of flags variable. Per-configuration variables are constructed by
#                           attaching _<CONFIG> suffix. Base name variable is treated as variable
#                           which contains flags common to all configurations.
# @param patternType        Pattern type:
#                            - PATTERN     - Normal regular expression pattern (select configuration that match
#                                            pattern). Behaves like ALL_PATTERN when matched all configurations.
#                            - NEG_PATTERN - Negated regular expression pattern (selects configuration that do not
#                                            match pattern). Behaves like ALL_PATTERN when matched all configurations.
#                            - ALL_PATTERN           - configPattern parameter is ignored and all configurations
#                                                      are selected. The settings are applied to common/base configuration
#                                                      variable (and they are inherited).
#                                                      When inherited common flag is removed, the removal affects all configurations
#                                                      (flags removed from common variable due to redundance or mutual exclusion
#                                                      are not moved to specific configs on applied operations).
#                            - ALL_PATTERN_NOINHERIT - configPattern parameter is ignored and all configurations
#                                                      are selected. The settings are applied to all specific configuration
#                                                      variables (and they are NOT inherited). Use this if you want to
#                                                      preserve flag in configurations that are mutually exclusive with flags
#                                                      in other configurations.
# @param configPattern      Regular expression which select configurations to which settings will be applied.
# @param flag               Flags to set (SET or SET_RAW).
# @param groupName          Names of groups of flags to remove (REMOVE_GROUP).
function(igc_config_flag_apply_settings settingsDomainName flagsVarBaseName patternType configPattern)
  set(_updatedFlags "${${flagsVarBaseName}}")

  set(_negativePattern NO)
  set(_matchAllConfigs NO)
  set(_configNoinherit NO)

  if(patternType MATCHES "^PATTERN$")
    # Only for check.
  elseif(patternType MATCHES "^NEG_PATTERN$")
    set(_negativePattern YES)
  elseif(patternType MATCHES "^ALL_PATTERN$")
    set(_matchAllConfigs YES)
  elseif(patternType MATCHES "^ALL_PATTERN_NOINHERIT$")
    set(_matchAllConfigs YES)
    set(_configNoinherit YES)
  else()
    message(FATAL_ERROR "Pattern type \"${patternType}\" is invalid. Supported patter types/keywords:\nPATTERN, NEG_PATTERN, ALL_PATTERN, ALL_PATTERN_NOINHERIT")
  endif()

  set(_matchedAllConfigs YES)
  set(_selectedConfigs)
  set(_selectedFlagsVarNames)
  foreach(_configName ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(REPLACE ";" "\;" _configName "${_configName}") # [WA#1] Must escape ; again if occurred in item.

    if(_matchAllConfigs OR (_negativePattern AND (NOT (_configName MATCHES "${configPattern}"))) OR ((NOT _negativePattern) AND (_configName MATCHES "${configPattern}")))
      string(TOUPPER "${_configName}" _upperConfigName)
      set(_updatedConfigFlagsName "_updatedFlags_${_upperConfigName}")

      set("${_updatedConfigFlagsName}" "${${flagsVarBaseName}_${_upperConfigName}}")

      list(APPEND _selectedConfigs "${_upperConfigName}")
      list(APPEND _selectedFlagsVarNames "${_updatedConfigFlagsName}")
    else()
      set(_matchedAllConfigs NO)
    endif()
  endforeach()

  if(_matchedAllConfigs AND (NOT _configNoinherit))
    igc_flag_apply_settings("${settingsDomainName}" _updatedFlags FLAG "${_selectedFlagsVarNames}" "${ARGN}") # [WA#2] To handle ; correctly some lists must be put in string form.
  else()
    foreach(_selectedFlagsVarName ${_selectedFlagsVarNames})
      string(REPLACE ";" "\;" _selectedFlagsVarName "${_selectedFlagsVarName}") # [WA#1] Must escape ; again if occurred in item.

      igc_flag_apply_settings("${settingsDomainName}" "${_selectedFlagsVarName}" FLAG _updatedFlags "${ARGN}") # [WA#2] To handle ; correctly some lists must be put in string form.
    endforeach()
  endif()


  set("${flagsVarBaseName}" "${_updatedFlags}" PARENT_SCOPE)
  foreach(_upperConfigName ${_selectedConfigs})
    string(REPLACE ";" "\;" _upperConfigName "${_upperConfigName}") # [WA#1] Must escape ; again if occurred in item.
    set(_updatedConfigFlagsName "_updatedFlags_${_upperConfigName}")

    set("${flagsVarBaseName}_${_upperConfigName}" "${${_updatedConfigFlagsName}}" PARENT_SCOPE)
  endforeach()
endfunction()

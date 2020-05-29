#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice ==================================


# CMake settings:
#  - IGC_OPTION__LIBRARY_NAME
#FCL
#  - FCL_OPTION__LIBRARY_NAME
#  - IGC_OPTION__ARCHITECTURE_HOST
#  - IGC_OPTION__ARCHITECTURE_TARGET
#  - IGC_OPTION__BIF_LINK_BC
#  - IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS
#  - IGC_OPTION__OUTPUT_DIR
#  - IGC_OPTION__USCLAUNCHER_TOOL for ILAdapter

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
if (${CMAKE_VERSION} VERSION_GREATER "3.0")
  cmake_policy(SET CMP0043 OLD) # Have CMake to use the old behavior for COMPILE_DEFINITIONS_<CONFIG>
  if (${CMAKE_VERSION} VERSION_GREATER "3.1")
    cmake_policy(SET CMP0054 OLD) # Have CMake to automatically expand variables in strings
  endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default languages: C, C++.
project(IGC)

if(UNIX)
  include(GNUInstallDirs)
endif()

if(POLICY CMP0058)
    cmake_policy(SET CMP0058 NEW)
endif()



# Increment IGC_API_MAJOR_VERSION if there is ABI Change.
if(NOT DEFINED IGC_API_MAJOR_VERSION)
  set(IGC_API_MAJOR_VERSION 1)
endif()
if(NOT DEFINED IGC_API_MINOR_VERSION)
  set(IGC_API_MINOR_VERSION 0)
endif()
# IGC_API_PATCH_VERSION
if(DEFINED IGC_PACKAGE_RELEASE)
      set(IGC_API_PATCH_VERSION ${IGC_PACKAGE_RELEASE})
else()
      set(IGC_API_PATCH_VERSION 1)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ======================================================================================================
# ================================================ UTILS ===============================================
# ======================================================================================================

# ================================================ Flags ===============================================

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


# Macros to set the runtime to /MT under windows (statically link to runtime.
# Suitable for multi-threaded as well)
macro(  win_static_runtime )
  # Windows only
  # NOTE: this is a directory level setting so everything for a given
  # CMakeLists.txt must be either /MT or /MD
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(configurations
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELEASEINTERNAL
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELEASEINTERNAL
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    foreach(configuration ${configurations})
      if(${configuration} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
      endif()
    endforeach()
  endif()
endmacro( win_static_runtime)

# Macros to set the runtime to /MD under windows (dynamically link to runtime)
macro(  win_dynamic_runtime )
  # Windows only
  # NOTE: this is a directory level setting so everything for a given CMakeLists.txt
  # must be either /MT or /MD
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(configurations
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELEASEINTERNAL
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELEASEINTERNAL
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    foreach(configuration ${configurations})
      if(${configuration} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${configuration} "${${configuration}}")
      endif()
    endforeach()
  endif()
endmacro( win_dynamic_runtime)

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

# ===================================== Host / Target Architecture =====================================

# Detects host and target architecture.
#
# Currently supports: Windows32, Windows64, WindowsARM, Android32, Android64, AndroidMIPS, AndroidARM,
#                     Linux32, Linux64, LinuxMIPS, LinuxARM
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
  elseif(ANDROID OR (CMAKE_SYSTEM_NAME MATCHES "Linux")
      )
    if(ANDROID)
      set(_targetArchOS "Android")
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
      set(_targetArchOS "Linux")
    endif()

    if(_cmakeTargetProcessor MATCHES "(x64|x86_64|amd64|64[ ]*\\-[ ]*bit)")
      set(_targetArchitecture "${_targetArchOS}64")
    elseif(_cmakeTargetProcessor MATCHES "(x32|x86|i[0-9]+86|32[ ]*\\-[ ]*bit)")
      set(_targetArchitecture "${_targetArchOS}32")
    elseif(_cmakeTargetProcessor MATCHES "mips")
      set(_targetArchitecture "${_targetArchOS}MIPS")
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
  elseif((CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
      )
    if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
      set(_hostArchOS "Linux")
    endif()

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
      "Linux32"   "Linux64"   "LinuxMIPS"   "LinuxARM"
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
#                     Linux32, Linux64, LinuxMIPS, LinuxARM
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
  elseif(_arch MATCHES "mips^")
    set(_cpuPart "MIPS")
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

  set(_os_detect_list "^(Windows|Android|Linux)(.*)$")

# Gets OS platform used in specified architecture. If it cannot be determined
# the "Unknown-NOTFOUND" will be returned.
#
# @param retValName Name of variable placeholder where result will be returned.
# @param arch       Architecture name to get info from.
function(igc_arch_get_os retVarName arch)

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

# ================================== Tools / custom build step helpers =================================

# Helper function which register tool activity and allow to generate command-line for it.
#
# igc_tool_register_activity(
#                          targetName
#                          activityName
#     [ARCH_OS             archOs
#       [ARCH_CPU          archCpu]]
#     [PLACEHOLDER         placeholder]
#     [ALLOW_EXTRA_PARAMS  allowExtraParams]
#                          [commandElem [commandElem [...]]]
#   )
#
# @param targetName       Target name of imported / created tool which will be executed during activity.
# @param activityName     Name of registered activity. The name can be registered multiple times (for different architectures).
# @param archOs           OS architecture compatible with activity. If not specified, the activity is treated as OS-generic.
# @param archCpu          CPU architecture compatible with activity. If not specified, the activity is treated as CPU-generic.
#                         Can be only specified when OS architecture is specified.
# @param placeholder      Text that will be searched to replace with actual parameters.
# @param allowExtraParams If TRUE the additional commandline parameters specified in igc_tool_get_activity_commandline will
#                         be passed to command-line (templated as last commandElem).
# @param commandElem      Command-line elements (with optional placeholders for parameters).
function(igc_tool_register_activity targetName activityName)
  if(NOT (TARGET "${targetName}"))
    message(FATAL_ERROR "Target \"${targetName}\" does not exist. Activity \"${activityName}\" will not be registered.")
  endif()
  if(activityName MATCHES "^[ ]*$")
    message(FATAL_ERROR "Activity name is empty or blank.")
  endif()


  set(_archOs)
  set(_archCpu)
  set(_placeholder "<<<param>>>")
  set(_allowExtraParams NO)
  set(_commandElems)

  set(_parseState 0)
  foreach(_toolArg ${ARGN})
    string(REPLACE ";" "\;" _toolArg "${_toolArg}") # [WA#1] Must escape ; again if occurred in item.

    # States: [0] *1( "ARCH_OS" [1] <param> [2] *1( "ARCH_CPU" [3] <param> [4] ) ) *1( "PLACEHOLDER" [5] <param> [6] ) *1( "ALLOW_EXTRA_PARAMS" [7] <param> [8] ) *<param> [8]
    # Transitions: 0 (ARCH_OS) -> 1 -> 2
    #              0 (PLACEHOLDER) -> 5 -> 6
    #              0 (ALLOW_EXTRA_PARAMS) -> 7 -> 8
    #              0 -> 8
    #              2 (ARCH_CPU) -> 3 -> 4
    #              2 (PLACEHOLDER) -> 5 -> 6
    #              2 (ALLOW_EXTRA_PARAMS) -> 7 -> 8
    #              2 -> 8
    #              4 (PLACEHOLDER) -> 5 -> 6
    #              4 (ALLOW_EXTRA_PARAMS) -> 7 -> 8
    #              4 -> 8
    #              6 (ALLOW_EXTRA_PARAMS) -> 7 -> 8
    #              6 -> 8
    # Stop States: 8
    if(_parseState EQUAL 0)
      if(_toolArg MATCHES "^ARCH_OS$")
        set(_parseState 1)
      elseif(_toolArg MATCHES "^PLACEHOLDER$")
        set(_parseState 5)
      elseif(_toolArg MATCHES "^ALLOW_EXTRA_PARAMS$")
        set(_parseState 7)
      else()
        list(APPEND _commandElems "${_toolArg}")
        set(_parseState 8)
      endif()
    elseif(_parseState EQUAL 1)
      set(_archOs "${_toolArg}")
      set(_parseState 2)
    elseif(_parseState EQUAL 2)
      if(_toolArg MATCHES "^ARCH_CPU$")
        set(_parseState 3)
      elseif(_toolArg MATCHES "^PLACEHOLDER$")
        set(_parseState 5)
      elseif(_toolArg MATCHES "^ALLOW_EXTRA_PARAMS$")
        set(_parseState 7)
      else()
        list(APPEND _commandElems "${_toolArg}")
        set(_parseState 8)
      endif()
    elseif(_parseState EQUAL 3)
      set(_archCpu "${_toolArg}")
      set(_parseState 4)
    elseif(_parseState EQUAL 4)
      if(_toolArg MATCHES "^PLACEHOLDER$")
        set(_parseState 5)
      elseif(_toolArg MATCHES "^ALLOW_EXTRA_PARAMS$")
        set(_parseState 7)
      else()
        list(APPEND _commandElems "${_toolArg}")
        set(_parseState 8)
      endif()
    elseif(_parseState EQUAL 5)
      set(_placeholder "${_toolArg}")
      set(_parseState 6)
    elseif(_parseState EQUAL 6)
      if(_toolArg MATCHES "^ALLOW_EXTRA_PARAMS$")
        set(_parseState 7)
      else()
        list(APPEND _commandElems "${_toolArg}")
        set(_parseState 8)
      endif()
    elseif(_parseState EQUAL 7)
      set(_allowExtraParams "${_toolArg}")
      set(_parseState 8)
    elseif(_parseState EQUAL 8)
      list(APPEND _commandElems "${_toolArg}")
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_toolArg}\".")
    endif()
  endforeach()
  if(NOT (_parseState EQUAL 8))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()

  igc_regex_escape(_placeholderRe "${_placeholder}")
  string(LENGTH "${_placeholderRe}" _placeholderReLen)
  if(_placeholderReLen EQUAL 0)
    message(FATAL_ERROR "Placeholder cannot be empty string.")
  endif()


  string(REPLACE ";" "\;" _activityName "${activityName}")
  if(DEFINED _archCpu)
    set(_activityPropName "IGC_PROPERTY__TOOL__OS${_archOs}_CPU${_archCpu}_A${_activityName}__ACTIVITY")
  elseif(DEFINED _archOs)
    set(_activityPropName "IGC_PROPERTY__TOOL__OS${_archOs}_A${_activityName}__ACTIVITY")
  else()
    set(_activityPropName "IGC_PROPERTY__TOOL__A${_activityName}__ACTIVITY")
  endif()

  set(_commandElemIdx 0)
  foreach(_commandElem ${_commandElems})
    string(REPLACE ";" "\;" _commandElem "${_commandElem}") # [WA#1] Must escape ; again if occurred in item.

    set(_commandElemParts)
    set(_scannedElem "${_commandElem}")
    while(TRUE)
      if(_scannedElem MATCHES "${_placeholderRe}(.*)$")
        set(_scannedElemRest "${CMAKE_MATCH_1}")
        string(REGEX REPLACE "${_placeholderRe}.*$" "" _scannedElemPrefix "${_scannedElem}")
        list(APPEND _commandElemParts "P${_scannedElemPrefix}")
        set(_scannedElem "${_scannedElemRest}")
      else()
        list(APPEND _commandElemParts "P${_scannedElem}")
        break()
      endif()
    endwhile()

    set_property(TARGET "${targetName}" PROPERTY "${_activityPropName}_I${_commandElemIdx}" "${_commandElemParts}")

    math(EXPR _commandElemIdx "${_commandElemIdx} + 1")
  endforeach()
  set_property(TARGET "${targetName}" PROPERTY "${_activityPropName}"       ${_commandElemIdx})
  set_property(TARGET "${targetName}" PROPERTY "${_activityPropName}_EXTRA" ${_allowExtraParams})
endfunction()

# Gets command-line for selected activity for specified tool target. Takes host architecture
# into consideration - uses IGC_OPTION__ARCHITECTURE_HOST variable do determine it.
#
# Allows to unify usage of tools between multiple platforms. The activity must be
# registered before it can be used. The activity can be registered using
# igc_tool_register_activity function, for example:
#
# add_executable(sampleTool IMPORTED)
# set_property(TARGET sampleTool PROPERTY IMPORTED_LOCATION "<Host OS-specific path to tool>")
# igc_tool_register_activity(sampleTool sampleOperation
#     ARCH_OS Windows
#     /sampleOption /sampleWindowsOption=<<<param>>>
#   )
# igc_tool_register_activity(sampleTool sampleOperation
#     ARCH_OS  Linux
#     ARCH_CPU 64
#     -sampleOption -sampleLinuxOption=<<<param>>>
#   )
#
# will register simpleOperation activity for sampleTool. Returned command-line will depend
# on host architecture:
#
# igc_tool_get_activity_commandline(commandLine sampleTool sampleOperation param1Value)
#
# will write to commandLine:
#   [Windows] "sampleTool" "/sampleOption" "/sampleWindowsOption=param1Value
#   [Linux64] "sampleTool" "-sampleOption" "-sampleLinuxOption=param1Value
#
# @param retVarName    Name of variable where command-line text will be returned.
#                      The list compatible with COMMAND from custom build target/command
#                      is returned. It should not be used outside this element.
# @param targetName    Target name of imported / created tool which will be executed during activity.
#                      The activity must be registered for tool target name.
# @param activityName  Name of activity for which command-line will be returned. The function
#                      throws fatal error when activity cannot be found for current tool target
#                      and current host architecture (read from IGC_OPTION__ARCHITECTURE_HOST).
# @param [param [...]] Additional configurable parameters for command-line.
#                      Values specified here fill the placeholder in registered command-line elements.
function(igc_tool_get_activity_commandline retVarName targetName activityName)
  # Name of variable where current host architecture is stored.
  set(__hostArchVarName "IGC_OPTION__ARCHITECTURE_HOST")

  if(NOT (TARGET "${targetName}"))
    message(FATAL_ERROR "Target \"${targetName}\" does not exist. The tool is not available.")
  endif()
  if(activityName MATCHES "^[ ]*$")
    message(FATAL_ERROR "Activity name is empty or blank.")
  endif()


  # Select most matching activity (with most matching architecture - most specific first, most generic last).
  igc_arch_get_os(_archOs   "${${__hostArchVarName}}")
  igc_arch_get_cpu(_archCpu)

  string(REPLACE ";" "\;" _activityName "${activityName}")
  set(_activityPropNames
      "IGC_PROPERTY__TOOL__OS${_archOs}_CPU${_archCpu}_A${_activityName}__ACTIVITY"
      "IGC_PROPERTY__TOOL__OS${_archOs}_A${_activityName}__ACTIVITY"
      "IGC_PROPERTY__TOOL__A${_activityName}__ACTIVITY"
    )

  set(_hasActivity NO)
  foreach(_activityPropName ${_activityPropNames})
    string(REPLACE ";" "\;" _activityPropName "${_activityPropName}") # [WA#1] Must escape ; again if occurred in item.

    get_property(_hasActivity TARGET "${targetName}" PROPERTY "${_activityPropName}" SET)
    if(_hasActivity)
      set(_selectedActivityPropName "${_activityPropName}")
      break()
    endif()
  endforeach()

  if(NOT _hasActivity)
    message(FATAL_ERROR "The tool idenfified by target \"${targetName}\" does not support activity \"${activityName}\" for current architecture:\n${_archOs}, ${_archCpu}.")
  endif()


  # Template all placeholders.
  list(LENGTH ARGN _paramsCount)

  get_property(_commandElemsCount TARGET "${targetName}" PROPERTY "${_selectedActivityPropName}")
  get_property(_allowExtraParams  TARGET "${targetName}" PROPERTY "${_selectedActivityPropName}_EXTRA")

  set(_currentParamIdx 0)
  set(_commandLine "${targetName}")
  if(_commandElemsCount GREATER 0)
    math(EXPR _commandElemsCountM1 "${_commandElemsCount} - 1")
    foreach(_commandElemIdx RANGE 0 ${_commandElemsCountM1})
      get_property(_commandElemParts TARGET "${targetName}" PROPERTY "${_selectedActivityPropName}_I${_commandElemIdx}")

      # Support for templating extra parameters by using last template sufficient number of times.
      if(_allowExtraParams AND (_commandElemIdx EQUAL _commandElemsCountM1))
        if(_currentParamIdx LESS _paramsCount)
          math(EXPR _maxIterations "${_paramsCount} - ${_currentParamIdx}")
        else()
          set(_maxIterations 0)
        endif()
      else()
        set(_maxIterations 1)
      endif()

      while(_maxIterations GREATER 0)
        set(_firstPart YES)
        foreach(_commandElemPart ${_commandElemParts})
          string(REPLACE ";" "\;" _commandElemPart "${_commandElemPart}") # [WA#1] Must escape ; again if occurred in item.

          string(REGEX REPLACE "^P" "" _commandElemPart "${_commandElemPart}")
          if(_firstPart)
            set(_commandElem "${_commandElemPart}")
            set(_firstPart NO)
          else()
            if(_currentParamIdx LESS _paramsCount)
              list(GET ARGN ${_currentParamIdx} _paramValue)
              string(REPLACE ";" "\;" _paramValue "${_paramValue}") # [WA#1] Must escape ; again if occurred in item.
              math(EXPR _currentParamIdx "${_currentParamIdx} + 1")
            else()
              set(_paramValue "")
            endif()
            set(_commandElem "${_commandElem}${_paramValue}${_commandElemPart}")
          endif()
        endforeach()
        list(APPEND _commandLine "${_commandElem}")

        # Finish templating extra parameters when there is no extra parameters
        # or number of repeats is equal number of extra parameters (when template
        # does not have placeholder).
        if(_currentParamIdx LESS _paramsCount)
          math(EXPR _maxIterations "${_maxIterations} - 1")
        else()
          set(_maxIterations 0)
        endif()
      endwhile()
    endforeach()
  elseif(_allowExtraParams)
    while(_currentParamIdx LESS _paramsCount)
      list(GET ARGN ${_currentParamIdx} _paramValue)
      string(REPLACE ";" "\;" _paramValue "${_paramValue}") # [WA#1] Must escape ; again if occurred in item.
      math(EXPR _currentParamIdx "${_currentParamIdx} + 1")
      list(APPEND _commandLine "${_paramValue}")
    endwhile()
  endif()


  set("${retVarName}" "${_commandLine}" PARENT_SCOPE)
endfunction()

# ======================================== Custom configuration ========================================

# Adds custom build configuration settings.
#
# @param configName     Name of new custom configuration.
# @param copyConfigName Name of configuration which new configuration will settings will be copied from.
# @param [configPath]   Path element that represents custom configuration.
function(igc_custom_build_add configName copyConfigName)
  string(TOUPPER "${configName}" _configName)
  string(TOUPPER "${copyConfigName}" _copyConfigName)

  set("CMAKE_CXX_FLAGS_${_configName}"           "${CMAKE_CXX_FLAGS_${_copyConfigName}}"           PARENT_SCOPE)
  set("CMAKE_C_FLAGS_${_configName}"             "${CMAKE_C_FLAGS_${_copyConfigName}}"             PARENT_SCOPE)
  set("CMAKE_EXE_LINKER_FLAGS_${_configName}"    "${CMAKE_EXE_LINKER_FLAGS_${_copyConfigName}}"    PARENT_SCOPE)
  set("CMAKE_MODULE_LINKER_FLAGS_${_configName}" "${CMAKE_MODULE_LINKER_FLAGS_${_copyConfigName}}" PARENT_SCOPE)
  set("CMAKE_SHARED_LINKER_FLAGS_${_configName}" "${CMAKE_SHARED_LINKER_FLAGS_${_copyConfigName}}" PARENT_SCOPE)
  set("CMAKE_STATIC_LINKER_FLAGS_${_configName}" "${CMAKE_STATIC_LINKER_FLAGS_${_copyConfigName}}" PARENT_SCOPE)
  mark_as_advanced(
      "CMAKE_CXX_FLAGS_${_configName}"
      "CMAKE_C_FLAGS_${_configName}"
      "CMAKE_EXE_LINKER_FLAGS_${_configName}"
      "CMAKE_MODULE_LINKER_FLAGS_${_configName}"
      "CMAKE_SHARED_LINKER_FLAGS_${_configName}"
      "CMAKE_STATIC_LINKER_FLAGS_${_configName}"
    )

  list(LENGTH ARGN _optArgsCount)
  if(_optArgsCount GREATER 0)
    list(GET ARGN 0 _configPath)
    string(REPLACE ";" "\;" _configPath "${_configPath}") # [WA#1] Must escape ; again if occurred in item.
    set("IGC_BUILD__${_configName}_CFG_PATH" "${_configPath}" PARENT_SCOPE)
  endif()
endfunction()

# Gets configuration path element corresponding to selected configuration.
#
# @param retValName Name of variable placeholder where result will be returned.
# @param configName Name of configuration.
function(igc_get_cfg_path retVarName configName)
  string(TOUPPER "${configName}" _configName)
  set(_configPathVarName "IGC_BUILD__${_configName}_CFG_PATH")
  if(DEFINED "${_configPathVarName}")
    set("${retVarName}" "${${_configPathVarName}}" PARENT_SCOPE)
  else()
    set("${retVarName}" "${configName}" PARENT_SCOPE)
  endif()
endfunction()

# =========================================== Resource files ===========================================

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

# ========================================= Symbols management =========================================

# Excludes from symbol export specified targets.
#
# igc_target_exclude_from_symbol_export(
#     targetName
#     [nonExportableTarget [nonExportableTarget [...]]]
#   )
#
# @param targetName          CMake target where specified linked targets will be excluded
#                            from symbol export.
# @param nonExportableTarget Targets that will be excluded from symbol export (for target targetName).
function(igc_target_exclude_from_symbol_export targetName)
  if(NOT (TARGET "${targetName}"))
    message(FATAL_ERROR "The target \"${targetName}\" is not defined.")
  endif()

  if(_igc_compiler_is_gnu_or_clang)
    foreach(_configName ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
      string(REPLACE ";" "\;" _configName "${_configName}") # [WA#1] Must escape ; again if occurred in item.
      string(TOUPPER "${_configName}" _upperConfigName)

      set(_locPropName "LOCATION_${_upperConfigName}")

      set(_excludedExportTargets)
      foreach(_excludedExportTarget ${ARGN})
        string(REPLACE ";" "\;" _excludedExportTarget "${_excludedExportTarget}") # [WA#1] Must escape ; again if occurred in item.
        if(TARGET "${_excludedExportTarget}")
          get_property(_locationSet TARGET "${_excludedExportTarget}" PROPERTY "${_locPropName}" SET)
          if(_locationSet)
            get_property(_location TARGET "${_excludedExportTarget}" PROPERTY "${_locPropName}")
          else()
            get_property(_location TARGET "${_excludedExportTarget}" PROPERTY LOCATION)
          endif()
        else()
          message(FATAL_ERROR "The excluded target \"${_excludedExportTarget}\" is not defined.")
        endif()

        if(NOT (_location MATCHES "(^|-NOTFOUND)$"))
          get_filename_component(_fileNameWoExt "${_location}" NAME_WE)

          if(DEFINED _excludedExportTargets)
            set(_excludedExportTargets "${_excludedExportTargets}:${_fileNameWoExt}")
          else()
            set(_excludedExportTargets "${_fileNameWoExt}")
          endif()
        endif()
      endforeach()

      set_property(TARGET "${targetName}" APPEND PROPERTY "LINK_FLAGS_${_upperConfigName}" "-Wl,--exclude-libs,${_excludedExportTargets}")
    endforeach()
  else()
    message(WARNING "Symbol exclusion works only for GCC-like compilers.")
  endif()
endfunction()

# ============================================ Source groups ===========================================

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

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================


# ======================================================================================================
# ==================================== BUILD CONFIGURATIONS (part 1) ===================================
# ======================================================================================================

# IGC project available configuration types:
# One standard configuration + "ReleaseInternal" custom configurations.
set(IGC_CMAKE_CONFIGURATION_TYPES
  "Debug"
  "Release"
)
set(IGC_CMAKE_CONFIGURATION_TYPES ${IGC_CMAKE_CONFIGURATION_TYPES} CACHE STRING "IGC available build configurations." FORCE)

if(CMAKE_CONFIGURATION_TYPES)
  # Multi-configuration generator: use CMAKE_CONFIGURATION_TYPES variable
  set(CMAKE_CONFIGURATION_TYPES ${IGC_CMAKE_CONFIGURATION_TYPES})

  if(CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE variable is not allowed while using multi-configuration generator!")
  endif()
else()
  # Single-configuration generator: use CMAKE_BUILD_TYPE variable
  if(DEFINED CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _buildType)
    string(STRIP "${_buildType}" _buildType)

    if(_buildType MATCHES "^release$")
      set(_buildType "Release")
    elseif(_buildType MATCHES "^release[ ]*-?[ ]*internal$")
      set(_buildType "ReleaseInternal")
    elseif(_buildType MATCHES "^debug$")
      set(_buildType "Debug")
    else()
      set(_buildType "Release")
      message(WARNING "CMAKE_BUILD_TYPE: Unknown build configuration. The following configurations are available: ${IGC_CMAKE_CONFIGURATION_TYPES}.\nThe \"${_buildType}\" configuration will be used.\nThis value has meaning only for single-configuration generators (like Make). It will be ignored for MSVC/XCode.")
    endif()
  else()
    set(_buildType "Release")
    message(WARNING "CMAKE_BUILD_TYPE: No build configuration specified. The following configurations are available: ${IGC_CMAKE_CONFIGURATION_TYPES}.\nThe \"${_buildType}\" configuration will be used.\nThis value has meaning only for single-configuration generators (like Make). It will be ignored for MSVC/XCode.")
  endif()
  set(CMAKE_BUILD_TYPE "${_buildType}")
  unset(_buildType)
endif()

# Patterns which identify debug and release configurations.
set(IGC_BUILD__DEBUG_PATTERN   "^Debug$")
set(IGC_BUILD__RELEASE_PATTERN "^Release$")
set(IGC_BUILD__RELINT_PATTERN  "^ReleaseInternal$")

# The configuration variables used below are likely to have been most recently
# defined by the project statement's call to the toolchain file
# ${CMAKE_TOOLCHAIN_FILE}.

# Some known cmake variables describing compiler:
# - CMAKE_CXX_COMPILER_ID == "Clang..." -> Clang
# - CMAKE_CXX_COMPILER_ID == "GNU"   -> GCC
# - CMAKE_CXX_COMPILER_ID == "Intel" -> Intel C++
# - CMAKE_CXX_COMPILER_ID == "MSVC"  -> using Visual Studio C++

# Local variables describing compilers:
# _igc_compiler_is_gnu       - C/C++ is gcc/g++ (not clang)
# _igc_compiler_is_clang     - C/C++ is clang/clang++ (not gcc/g++)
# _igc_compiler_is_gnu_or_clang  = C/C++ is either gnu or clang

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(_igc_compiler_is_gnu 1)
else()
  set(_igc_compiler_is_gnu 0)
endif()

if((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(_igc_compiler_is_clang 1)
else()
  set(_igc_compiler_is_clang 0)
endif((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))

if(_igc_compiler_is_gnu OR _igc_compiler_is_clang)
  set(_igc_compiler_is_gnu_or_clang 1)
else()
  set(_igc_compiler_is_gnu_or_clang 0)
endif(_igc_compiler_is_gnu OR _igc_compiler_is_clang)

#
#  First, Set IGC_OPTION__ARCHITECTURE_TARGET, IGC_OPTION__ARCHITECTURE_HOST,
#  and IGC_BUILD__CROSSCOMPILE_NEEDED.
#
igc_arch_detect(IGC_BUILD__DETECTED_ARCHITECTURE_TARGET IGC_BUILD__DETECTED_ARCHITECTURE_HOST)
igc_arch_validate(_detectedTargetArchValid "${IGC_BUILD__DETECTED_ARCHITECTURE_TARGET}")
igc_arch_validate(_detectedHostArchValid   "${IGC_BUILD__DETECTED_ARCHITECTURE_HOST}")

if(DEFINED IGC_OPTION__ARCHITECTURE_TARGET)
  igc_arch_normalize(_targetArchitecture "${IGC_OPTION__ARCHITECTURE_TARGET}")
elseif(_detectedTargetArchValid)
  set(_targetArchitecture "${IGC_BUILD__DETECTED_ARCHITECTURE_TARGET}")
  message(WARNING "IGC_OPTION__ARCHITECTURE_TARGET: Target architecture is not specified. Trying to deduce it from context.")
else()
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_TARGET: Target architecture is not specified and cannot be deduced from context.\nPlease specify one, e.g. Windows32, Linux64, Android32, ...")
endif()

if(DEFINED IGC_OPTION__ARCHITECTURE_HOST)
  igc_arch_normalize(_hostArchitecture "${IGC_OPTION__ARCHITECTURE_HOST}")
elseif(_detectedHostArchValid)
  set(_hostArchitecture "${IGC_BUILD__DETECTED_ARCHITECTURE_HOST}")
  if(CMAKE_CROSSCOMPILING)
    message(WARNING "IGC_OPTION__ARCHITECTURE_HOST: Host architecture is not specified. Trying to deduce it from context.")
  endif()
elseif(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_HOST: Host architecture is not specified and cannot be deduced from context.\nPlease specify one, e.g. Windows32, Linux64, Android32, ...")
else()
  set(_hostArchitecture "${_targetArchitecture}")
endif()

igc_arch_crosscompile_needed(IGC_BUILD__CROSSCOMPILE_NEEDED "${_targetArchitecture}" "${_hostArchitecture}")

set(IGC_OPTION__ARCHITECTURE_TARGET "${_targetArchitecture}" CACHE STRING "Target architecture for compilation." FORCE)
set(IGC_OPTION__ARCHITECTURE_HOST   "${_hostArchitecture}"   CACHE STRING "Architecture of host used during compilation. Only used when cross-compiling." FORCE)
unset(_detectedTargetArchValid)
unset(_detectedHostArchValid)
unset(_targetArchitecture)
unset(_hostArchitecture)


# GFX_DEVELOPMENT_DIR shall be provided by UFO build system
if(NOT GFX_DEVELOPMENT_DIR)
  get_filename_component(GFX_DEVELOPMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../ ABSOLUTE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(PYTHON_EXECUTABLE "${BS_DIR_EXTERNAL_COMPONENTS}/build-tools/python3-win64/python.exe")
else()
  find_program(PYTHON_EXECUTABLE NAMES "python3" "python")
endif()

set(IGC_CODEGEN_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")
if(MSVC_IDE)
    set(IGC_CODEGEN_DIR "${IGC_CODEGEN_BASE_DIR}/${CMAKE_CFG_INTDIR}")
else()
    set(IGC_CODEGEN_DIR "${IGC_CODEGEN_BASE_DIR}")
endif()
add_subdirectory(MDAutogen ${IGC_CODEGEN_BASE_DIR}/MDAutogen)
set_source_files_properties(${IGC_CODEGEN_DIR}/MDNodeFunctions.gen PROPERTIES GENERATED TRUE)


# ============================================================================
# ========== BUILD CONFIGURATIONS (part 2) ===================================
# ============================================================================
# The section must be after project definition and its supported languages because otherwise
# used variables are not available yet.

# Setting flags for custom "ReleaseInternal" configurations.
igc_custom_build_add("ReleaseInternal"       "Release")

# Populating global property with list of debug configurations (based on pattern).
set(_debugConfigs)
foreach(_configName ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
  if(_configName MATCHES "${IGC_BUILD__DEBUG_PATTERN}")
    list(APPEND _debugConfigs "${_configName}")
  endif()
endforeach()
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS ${_debugConfigs})
unset(_debugConfigs)

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================


# ======================================================================================================
# ========================================= BUILD CMAKE OPTIONS ========================================
# ======================================================================================================

if(DEFINED IGC_OPTION__LIBRARY_NAME)
  set(IGC_LIBRARY_NAME "${IGC_OPTION__LIBRARY_NAME}")
else()
  set(IGC_LIBRARY_NAME "igc")
endif()
set(IGC_LIBRARY_NAME ${IGC_LIBRARY_NAME} PARENT_SCOPE)

#FCL

if(DEFINED FCL_OPTION__LIBRARY_NAME)
  set(FCL_LIBRARY_NAME "${FCL_OPTION__LIBRARY_NAME}")
else()
  set(FCL_LIBRARY_NAME "igdfcl")
endif()
set(FCL_LIBRARY_NAME ${FCL_LIBRARY_NAME} PARENT_SCOPE)


set(_allowBifLink ON)


set(IGC_OPTION__BIF_LINK_BC ${_allowBifLink} CACHE BOOL "Built-in Functions: Link .bc files into final shared libraries.")
mark_as_advanced(IGC_OPTION__BIF_LINK_BC)
unset(_allowBifLink)



set(IGC_OPTION__BIF_SRC_OCL_DIR "${IGC_SOURCE_DIR}/BiFModule"
    CACHE PATH "Built-in Functions: Root directory where sources for OpenCL builtins are located.")
mark_as_advanced(IGC_OPTION__BIF_SRC_OCL_DIR)

if(IGC_BUILD__CROSSCOMPILE_NEEDED AND (NOT CMAKE_CROSSCOMPILING))
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_TARGET,\nIGC_OPTION__ARCHITECTURE_HOST: Current target / host architecture combination requires cross-compiling. Please specify correct toolchain.")
endif()

igc_arch_validate(_archValid "${IGC_OPTION__ARCHITECTURE_TARGET}")
if(NOT _archValid)
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_TARGET: Target architecture \"${IGC_OPTION__ARCHITECTURE_TARGET}\" is invalid.\nPlease specify correct one, e.g. Windows32, Linux64, Android32, ...")
endif()
igc_arch_validate(_archValid "${IGC_OPTION__ARCHITECTURE_HOST}")
if(NOT _archValid)
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_HOST: Host architecture \"${IGC_OPTION__ARCHITECTURE_HOST}\" is invalid.\nPlease specify correct one, e.g. Windows32, Linux64, Android32, ...")
endif()


set(IGC_OPTION__IST_IGCC_ONLY ON CACHE BOOL
    "Only building igcc part of IST (IGCStandalone).")

set(IGC_OPTION__BUILD_IGC_OPT ON CACHE BOOL "Build project igc_opt.")

set(IGC_OPTION__USCLAUNCHER_TOOL OFF CACHE BOOL
    "Building USCLauncher tool for ILAdapter")

igc_arch_get_cpu(_cpuSuffix)
if(NOT DEFINED IGC_OPTION__OUTPUT_DIR)
set(IGC_OPTION__OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}" CACHE PATH "Output directory path where the final libraries will be stored.")
endif()
get_filename_component(IGC_OPTION__OUTPUT_DIR ${IGC_OPTION__OUTPUT_DIR} ABSOLUTE)
set(USC_OPTION__OUTPUT_DIR "${IGC_SOURCE_DIR}/../../../output/dump${_cpuSuffix}/usc" CACHE PATH "Output directory path where the final USC libraries will be stored.")
unset(_cpuSuffix)

if(NOT DEFINED IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS)
    set(IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS OFF)
endif()

set(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT ON CACHE BOOL "Compile/link: Allow unsafe size optimization like --gc-sections.")
mark_as_advanced(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT)

# IGC install time absolute path
if(IGC_OPTION__INSTALL_TIME_ROOT_DIR)
    get_filename_component(IGC_INSTALL_TIME_ROOT_DIR ${IGC_OPTION__INSTALL_TIME_ROOT_DIR} ABSOLUTE)
else()
    set(IGC_INSTALL_TIME_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
endif()

# ============================================= Status =================================================

message(STATUS "================================ IGC Project ================================")
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Build types: ${CMAKE_CONFIGURATION_TYPES} (multi-configuration generator)")
else()
    message(STATUS "Build type:  ${CMAKE_BUILD_TYPE} (single-configuration generator)")
endif()
message(STATUS "Build tools: ${IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS}")
message(STATUS "LLVM dir:    ${IGC_OPTION__LLVM_DUMP_DIR}")
message(STATUS "")
message(STATUS "Output directory:")
message(STATUS "   \"${IGC_OPTION__OUTPUT_DIR}\"")
message(STATUS "Install root directory:")
message(STATUS "   \"${IGC_INSTALL_TIME_ROOT_DIR}\"")
message(STATUS "Architecture:")
message(STATUS " - target: ${IGC_OPTION__ARCHITECTURE_TARGET}    (detected: ${IGC_BUILD__DETECTED_ARCHITECTURE_TARGET})")
message(STATUS " - host:   ${IGC_OPTION__ARCHITECTURE_HOST}    (detected: ${IGC_BUILD__DETECTED_ARCHITECTURE_HOST})")
message(STATUS "")
message(STATUS "Cross-compilation needed: ${IGC_BUILD__CROSSCOMPILE_NEEDED}")
message(STATUS "Cross-compilation set:    ${CMAKE_CROSSCOMPILING}")
message(STATUS "")
message(STATUS "Advanced:")
message(STATUS " - Link BiF resources:              ${IGC_OPTION__BIF_LINK_BC}")
message(STATUS " - Building Windows Universal:      ${IGC_OPTION__UNIVERSAL_DRIVER}")
message(STATUS "=============================================================================")

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================


# ======================================================================================================
# ======================================== COMPILE/LINK SETTINGS =======================================
# ======================================================================================================

# =========================================== Helper variables =========================================


# We are building IGC.
set(IGC_BUILD YES)


if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(LLVM_ON_WIN32 ON)
else()
  set(LLVM_ON_WIN32 OFF)
endif()

# NOTE: LLVM_ON_UNIX is also true for Android (CMAKE_SYSTEM_NAME specified by Android toolchain file is also "Linux",
# but we should make sure so there is no errors in our assumptions).
if((CMAKE_SYSTEM_NAME MATCHES "Linux") OR ANDROID)
  set(LLVM_ON_UNIX ON)
else()
  set(LLVM_ON_UNIX OFF)
endif()


if(_igc_compiler_is_gnu_or_clang AND NOT IGC_BUILD_DISABLE_FLAGS_GNU_CLANG)
  set(IGC_BUILD__WHOLE_ARCHIVE    "-Wl,--whole-archive")
  set(IGC_BUILD__NO_WHOLE_ARCHIVE "-Wl,--no-whole-archive")
  set(IGC_BUILD__START_GROUP      "-Wl,--start-group")
  set(IGC_BUILD__END_GROUP        "-Wl,--end-group")
else()
  set(IGC_BUILD__WHOLE_ARCHIVE    "")
  set(IGC_BUILD__NO_WHOLE_ARCHIVE "")
  set(IGC_BUILD__START_GROUP      "")
  set(IGC_BUILD__END_GROUP        "")
endif()

set(IGC_BUILD__PROJ_NAME_PREFIX "")

#SPIRV consumption variable

  set(IGC_BUILD__SPIRV_ENABLED ON)



# ======================================== Path helper variables =======================================


set(IGC_BUILD__IGC_SRC_DIR          "${IGC_SOURCE_DIR}")
set(IGC_BUILD__IGC_COMMON_DIR       "${IGC_BUILD__IGC_SRC_DIR}/common")

set(IGC_BUILD__IGC_BIN_DIR          "${IGC_BINARY_DIR}")

set(IGC_BUILD__GFX_DEV_SRC_DIR      "${IGC_SOURCE_DIR}/..")
set(IGC_BUILD__GFX_DEV_TOOLS_DIR    "${IGC_SOURCE_DIR}/../../Tools")
set(IGC_BUILD__GFX_OCL_DIR          "${IGC_SOURCE_DIR}/../../Source_OCL")

set(IGC_BUILD__COMMON_DIR           "${IGC_BUILD__GFX_DEV_SRC_DIR}/Common")
set(IGC_BUILD__3D_COMMON_DIR        "${IGC_BUILD__GFX_DEV_SRC_DIR}/3d/common")

set(IGC_BUILD__INC_DIR              "${IGC_BUILD__GFX_DEV_SRC_DIR}/inc")
set(IGC_BUILD__COMMON_COMPILER_DIR  "${IGC_BUILD__GFX_DEV_SRC_DIR}/inc/common/Compiler")
set(IGC_BUILD__VISA_DIR             "${IGC_BUILD__GFX_DEV_SRC_DIR}/visa")

set(IGC_BUILD__TOOLS_IGC_DIR        "${IGC_BUILD__GFX_DEV_TOOLS_DIR}/IGC")
set(IGC_BUILD__TOOLS_IST_DIR        "${IGC_BUILD__GFX_DEV_TOOLS_DIR}/IST")
set(IGC_BUILD__TOOLS_USCLAUNCHER_DIR "${GFX_DEVELOPMENT_DIR}/Test/usc/src")


set(IGC_BUILD__TOOLS_7ZIP_WIN_DIR   "${BS_DIR_EXTERNAL_COMPONENTS}/build-tools/7zip-win64")
set(IGC_BUILD__TOOLS_PAX_APP_DIR    "/usr")
set(IGC_BUILD__TOOLS_7Z_LINUX_DIR "${IGC_BUILD__GFX_DEV_TOOLS_DIR}/Linux/tools/7z9.2/bin")

set(IGC_BUILD__BIF_DIR              "${IGC_OPTION__OUTPUT_DIR}/bif/${CMAKE_CFG_INTDIR}")
set(IGC_BUILD__TOOLS_OUTPUT_DIR     "${IGC_OPTION__OUTPUT_DIR}/Tools")
set(IGC_BUILD__UNIT_TEST_OUTPUT_DIR "${IGC_OPTION__OUTPUT_DIR}/UnitTests")
set(IGC_BUILD__CTH_DIR              "${IGC_OPTION__OUTPUT_DIR}/fcl")

# =========================================== Command-line Tools =======================================

add_executable(IGC_TARGET__TOOLS_COPY  IMPORTED)
add_executable(IGC_TARGET__TOOLS_7Z    IMPORTED)
add_executable(IGC_TARGET__TOOLS_TARGZ IMPORTED)
add_executable(IGC_TARGET__TOOLS_PAX   IMPORTED)


# Looking for tools.
igc_arch_get_os(_archOs "${IGC_OPTION__ARCHITECTURE_HOST}")
if(_archOs MATCHES "^Windows$")
  # NOTE: The most similar copy tool to cp is in powershell (cp, Copy-Item).
  # NOTE: 7-zip is used as tar tool on Windows.
  find_program(IGC_FIND__TOOLS_COPY  NAMES powershell powershell.exe                                          DOC "Path to copy tool."   NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_7Z    NAMES 7z   7z.exe               PATHS "${IGC_BUILD__TOOLS_7ZIP_WIN_DIR}" DOC "Path to 7-zip tool."  NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_7Z    NAMES 7z   7z.exe                                                        DOC "Path to 7-zip tool."  NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_TARGZ NAMES 7z   7z.exe               PATHS "${IGC_BUILD__TOOLS_7ZIP_WIN_DIR}" DOC "Path to tar.gz tool." NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_TARGZ NAMES 7z   7z.exe                                                        DOC "Path to tar.gz tool." NO_CMAKE_FIND_ROOT_PATH)
else()
  find_program(IGC_FIND__TOOLS_COPY  cp                  DOC "Path to copy tool."   NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_7Z    NAMES 7z 7za        PATHS "${IGC_BUILD__TOOLS_7Z_LINUX_DIR}" DOC "Path to 7-zip tool."  NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_TARGZ tar                 DOC "Path to tar.gz tool." NO_CMAKE_FIND_ROOT_PATH)
  find_program(IGC_FIND__TOOLS_PAX   pax                 DOC "Path to pax tool."    NO_CMAKE_FIND_ROOT_PATH)
endif()
unset(_archOs)

foreach(_toolName IN ITEMS "COPY" "7Z" "TARGZ" "PAX")
  if((DEFINED "IGC_FIND__TOOLS_PART_OF_SHELL_${_toolName}" AND "IGC_FIND__TOOLS_PART_OF_SHELL_${_toolName}") OR (EXISTS "${IGC_FIND__TOOLS_${_toolName}}"))
    set(_toolLocation "${IGC_FIND__TOOLS_${_toolName}}")
  else()
    set(_toolLocation "IGC_TARGET__TOOLS_${_toolName}-NOTFOUND")
  endif()
  set_property(TARGET "IGC_TARGET__TOOLS_${_toolName}" PROPERTY IMPORTED_LOCATION "${_toolLocation}")
  unset(_toolLocation)
endforeach()

    # Including CIF
    SET(CIF_BASE_DIR "AdaptorOCL/cif")

    add_subdirectory("${CIF_BASE_DIR}/cif" ${CMAKE_CURRENT_BINARY_DIR}/cif)

# Activities.
igc_tool_register_activity(
    IGC_TARGET__TOOLS_COPY
    CopyPreserve
    ARCH_OS Windows
    "-NonInteractive" "-Command" "cp" "-Force" "<<<param>>>" "<<<param>>>"
  )
igc_tool_register_activity(
    IGC_TARGET__TOOLS_COPY
    CopyPreserve
    "-pf" "<<<param>>>" "<<<param>>>"
  )

igc_tool_register_activity(
    IGC_TARGET__TOOLS_7Z
    Extract
    "x" "-t7z" "-y" "<<<param>>>" "-o<<<param>>>"
  )

igc_tool_register_activity(
    IGC_TARGET__TOOLS_PAX
    Extract
    "-rjf " "<<<param>>>"
  )

igc_tool_register_activity(
    IGC_TARGET__TOOLS_TARGZ
    Extract
    ARCH_OS Windows
    "x" "<<<param>>>" "-so" "|" "$<TARGET_FILE:IGC_TARGET__TOOLS_TARGZ>" "x" "-ttar" "-si" "-y" "-o<<<param>>>"
  )
igc_tool_register_activity(
    IGC_TARGET__TOOLS_TARGZ
    Extract
    "-xzf" "<<<param>>>" "-C" "<<<param>>>"
  )

# ================================================ Outputs =============================================

set(EXECUTABLE_OUTPUT_PATH ${IGC_OPTION__OUTPUT_DIR})
set(LIBRARY_OUTPUT_PATH    ${IGC_OPTION__OUTPUT_DIR})

# =============================== Compiler options (generic configuration) =============================

# Compiler options:
#   = - Equal
#   ^ - Implicitly defined by differnt option (like /O2).
#   ! - Should not be changed (should not be defined in first place -> there is no non-Windows support).
#   * - Needs to be modified in CMake.
#   + - Needs to be added in CMake.
#   - - Needs to be removed in CMake.
#
#   MSVC:
#     =/analyze-   =/EHsc   =/errorReport:prompt   =/fp:precise   =/Gd   =/Gm-   ^/GF    /GR-   ^/GS   ^/Gy    /MP    /MT   =/nologo   =/O2   ^/Ob2   ^/Oi    /Os   =/Oy-          =/W3   !/Zc:forScope-   =/Zc:wchar_t    /Zi    /Zp8
#     ?/wd"4146"   ?/wd"4355"   ?/wd"4800"   ?/wd"4482"   ?/wd"4244"   ?/WX
#   CMake:
#     =/analyze-   =/EHsc   =/errorReport:prompt   =/fp:precise   =/Gd   =/Gm-   ^/GF   */GR    ^/GS   ^/Gy   +/MP   */MD   =/nologo   =/O2   ^/Ob2   ^/Oi   +/Os   =/Oy-   -/TP   =/W3    /Zc:forScope    =/Zc:wchar_t   +/Zi   ^/Zp8
#                                                                      ?/WX-
#
#   Notes:
#    - "/Zc:forScope-" is non-standard behavior which is not portable to other OSes. It was defined for MFC and should not be used in modern code.
#    - "/Zp8" is default max. alignment for struct members (this option and its behavior can be hard to port to other OSes).
#    - "/wd" some of the warning are critically useful (especially when warning level is less than 4):
#      4146 -> this can lead to problems with comparison; limits or numeric_limits usually allow to quickly correct the code.
#      4244 -> this can lead to hard to catch problems with loss of data (especially when /W2 or /W3 type occurred and in x86_64 architetures).
#      4355 -> warning: "this" passed to base class should be off by default (what enabled this?).
#      4482 -> C++11 compilers allow enum qualification without warning (should not be problem since MSVC 2013).
#      4800 -> can be disabled but it can be corrected pretty quick.
#      4267 -> var : conversion from 'size_t' to 'type', possible loss of data
#      4273 -> function' : inconsistent DLL linkage
#    - "/WX" should be re-enabled when all warnings are cleaned from code.
#    - Although "/O1" is usual set of settings for optmimization for size, but "/O2" "/Oi" "/Os" is preferred in IGC.
#    - we want to disable warning 4267: conversion from 'size_t' to 'uint32_t', possible loss of data
#    - Adding warning 4062 to be emiited at level 1: enumerator 'identifier' in switch of enum 'enumeration' is not handled


if(MSVC)
  igc_flag_register_settings(
      CompilerOptions

      GROUP NAME CallingConvention  "/G[drzv]"
      GROUP NAME CompileAs          "/T[CP]"
      GROUP NAME DebugInfoFormat    "/Z[7iI]"
      GROUP NAME Exceptions         "/EHa" "/EHsc?"
      GROUP NAME FaviorSizeSpeed    "/O[st]"
      GROUP NAME Optimization       "/O[d12x]"
      GROUP NAME RuntimeLibrary     "/M[TD]d?"
      GROUP NAME Rtti               "/GR-?"
      GROUP NAME TreatWarnAsError   "/WX-?"

      ALIAS AdditionalOptions          "/w14062" "/Wv:18"
      ALIAS CallingConvCdecl           "/Gd"
      ALIAS CallingConvFastcall        "/Gr"
      ALIAS CallingConvStdcall         "/Gz"
      ALIAS CallingConvVectorcall      "/Gv"
      ALIAS CompileAsC                 "/TC"
      ALIAS CompileAsCpp               "/TP"
      ALIAS CompileAsDefault           CompileAs REMOVE_GROUP
      ALIAS DebugInfoDisabled          DebugInfoFormat REMOVE_GROUP
      ALIAS DebugInfoProgramDatabase   "/Zi"
      ALIAS DisableSpecificWarnings    "/wd4267" "/wd4273" "/wd4091" "/wd4996"
      ALIAS ExceptionsDisabled         Exceptions REMOVE_GROUP
      ALIAS ExceptionsEnabled          "/EHsc"
      ALIAS FaviorSize                 "/Os"
      ALIAS FaviorSpeed                "/Ot"
      ALIAS MultiProcessorCompilation  "/MP"
      ALIAS OptimizeDisabled           "/Od"
      ALIAS OptimizeSize               "/O2" "/Oi" "/Os"
      ALIAS OptimizeSpeed              "/O2" "/Oi" "/Ot"
      ALIAS OptimizeFull               "/Ox"
      ALIAS RtMultiThreadedStatic      "/MT"
      ALIAS RtMultiThreadedShared      "/MD"
      ALIAS RtMultiThreadedStaticDebug "/MTd"
      ALIAS RtMultiThreadedSharedDebug "/MDd"
      ALIAS RttiDisabled               "/GR-"
      ALIAS RttiEnabled                "/GR"
      ALIAS TreatWarnAsErrorDisabled   "/WX-"
      ALIAS TreatWarnAsErrorEnabled    "/WX"
      ALIAS UnreferencedVariable       "/w14189"
      ALIAS WholeProgramOptimization   "/GL"
      ALIAS ControlFlowGuard           "/guard:cf"

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )
elseif(_igc_compiler_is_gnu)
  igc_flag_register_settings(
      CompilerOptions

      GROUP NAME DebugInfoFormat    "-g[0-3]?"
      GROUP NAME Exceptions         "-f(no-)?exceptions"
      GROUP NAME Optimization       "-O[0-3sg]?"
      GROUP NAME Rtti               "-f(no-)?rtti"
      GROUP NAME TreatWarnAsError   "-Werror"

      ALIAS AdditionalOptions
      ALIAS CallingConvCdecl
      ALIAS CallingConvFastcall
      ALIAS CallingConvStdcall
      ALIAS CallingConvVectorcall
      ALIAS CompileAsC
      ALIAS CompileAsCpp
      ALIAS CompileAsDefault
      ALIAS DebugInfoDisabled          "-g0"
      ALIAS DebugInfoProgramDatabase   "-g"
      ALIAS DisableSpecificWarnings
      ALIAS ExceptionsDisabled         "-fno-exceptions"
      ALIAS ExceptionsEnabled          "-fexceptions"
      ALIAS FaviorSize                 "-Os"
      ALIAS FaviorSpeed                "-O3"
      ALIAS MultiProcessorCompilation
      ALIAS OptimizeDisabled           "-O0"
      ALIAS OptimizeSize               "-Os"
      ALIAS OptimizeSpeed              "-O2"
      ALIAS OptimizeFull               "-03"
      ALIAS RtMultiThreadedStatic
      ALIAS RtMultiThreadedShared
      ALIAS RtMultiThreadedStaticDebug
      ALIAS RtMultiThreadedSharedDebug
      ALIAS RttiDisabled               "-fno-rtti"
      ALIAS RttiEnabled                "-frtti"
      ALIAS TreatWarnAsErrorDisabled   TreatWarnAsError REMOVE_GROUP
      ALIAS TreatWarnAsErrorEnabled    "-Werror"
      ALIAS UnreferencedVariable
      ALIAS WholeProgramOptimization
      ALIAS ControlFlowGuard

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )

  if(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT)
    igc_flag_register_settings(
        CompilerOptions

        GROUP NAME EmitDataSec "-fdata-sections"
        GROUP NAME EmitFuncSec "-ffunction-sections"

        ALIAS DeadCodeEliminate  "-fdata-sections" "-ffunction-sections"
        ALIAS DeadCodePreserve   EmitDataSec EmitFuncSec REMOVE_GROUP
      )
  endif()
elseif(_igc_compiler_is_clang)
  igc_flag_register_settings(
      CompilerOptions

      GROUP NAME DebugInfoFormat    "-g(dwardf-[2-4]?)?"
      GROUP NAME Exceptions         "-f(no-)?exceptions"
      GROUP NAME Optimization       "-O([0-3]?|fast|s|z)?"
      GROUP NAME Rtti               "-f(no-)?rtti"
      GROUP NAME TreatWarnAsError   "-Werror"

      ALIAS AdditionalOptions
      ALIAS CallingConvCdecl
      ALIAS CallingConvFastcall
      ALIAS CallingConvStdcall
      ALIAS CallingConvVectorcall
      ALIAS CompileAsC
      ALIAS CompileAsCpp
      ALIAS CompileAsDefault
      ALIAS DebugInfoDisabled          DebugInfoFormat REMOVE_GROUP
      ALIAS DebugInfoProgramDatabase   "-g"
      ALIAS DisableSpecificWarnings
      ALIAS ExceptionsDisabled         Exceptions REMOVE_GROUP
      ALIAS ExceptionsEnabled          "-fexceptions"
      ALIAS FaviorSize                 "-Os"
      ALIAS FaviorSpeed                "-O3"
      ALIAS MultiProcessorCompilation
      ALIAS OptimizeDisabled           "-O0"
      ALIAS OptimizeSize               "-Os"
      ALIAS OptimizeSpeed              "-O2"
      ALIAS OptimizeFull               "-03"
      ALIAS RtMultiThreadedStatic
      ALIAS RtMultiThreadedShared
      ALIAS RtMultiThreadedStaticDebug
      ALIAS RtMultiThreadedSharedDebug
      ALIAS RttiDisabled               "-fno-rtti"
      ALIAS RttiEnabled                Rtti REMOVE_GROUP
      ALIAS TreatWarnAsErrorDisabled   TreatWarnAsError REMOVE_GROUP
      ALIAS TreatWarnAsErrorEnabled    "-Werror"
      ALIAS UnreferencedVariable
      ALIAS WholeProgramOptimization
      ALIAS ControlFlowGuard

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )
else()
  igc_flag_register_settings(
      CompilerOptions

      ALIAS AdditionalOptions
      ALIAS CallingConvCdecl
      ALIAS CallingConvFastcall
      ALIAS CallingConvStdcall
      ALIAS CallingConvVectorcall
      ALIAS CompileAsC
      ALIAS CompileAsCpp
      ALIAS CompileAsDefault
      ALIAS DebugInfoDisabled
      ALIAS DebugInfoProgramDatabase
      ALIAS DisableSpecificWarnings
      ALIAS ExceptionsDisabled
      ALIAS ExceptionsEnabled
      ALIAS FaviorSize
      ALIAS FaviorSpeed
      ALIAS MultiProcessorCompilation
      ALIAS OptimizeDisabled
      ALIAS OptimizeSize
      ALIAS OptimizeSpeed
      ALIAS OptimizeFull
      ALIAS RtMultiThreadedStatic
      ALIAS RtMultiThreadedShared
      ALIAS RtMultiThreadedStaticDebug
      ALIAS RtMultiThreadedSharedDebug
      ALIAS RttiDisabled
      ALIAS RttiEnabled
      ALIAS TreatWarnAsErrorDisabled
      ALIAS TreatWarnAsErrorEnabled
      ALIAS UnreferencedVariable
      ALIAS WholeProgramOptimization
      ALIAS ControlFlowGuard

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )
endif()

# ================================ Linker options (generic configuration) ==============================
if(MSVC)
  if(IGC_OPTION__UNIVERSAL_DRIVER)
    igc_flag_register_settings(
        LinkerOptions

        GROUP NAME DebugInfo         "/[dD][eE][bB][uU][gG]"
        GROUP NAME DeadCode          "/[oO][pP][tT]:([nN][oO])?[rR][eE][fF]"
        GROUP NAME IdenticalCodeData "/[oO][pP][tT]:([nN][oO])?[iI][cC][fF](=[0-9]+)?"

        ALIAS AdditionalOptions
        ALIAS DebugInfoDisabled          DebugInfo REMOVE_GROUP
        ALIAS DebugInfoProgramDatabase   "/DEBUG"
        ALIAS DeadCodeEliminate          "/OPT:REF"
        ALIAS DeadCodePreserve           "/OPT:NOREF"
        ALIAS IdenticalCodeDataFold      "/OPT:ICF"
        ALIAS IdenticalCodeDataNoFold    "/OPT:NOICF"
        ALIAS LinkTimeCodeGeneration     "/LTCG"
        ALIAS ControlFlowGuard           "/guard:cf"
        ALIAS NoDefaultLib               "/NODEFAULTLIB:kernel32.lib"
      )
  else()
    igc_flag_register_settings(
        LinkerOptions

        GROUP NAME DebugInfo         "/[dD][eE][bB][uU][gG]"
        GROUP NAME DeadCode          "/[oO][pP][tT]:([nN][oO])?[rR][eE][fF]"
        GROUP NAME IdenticalCodeData "/[oO][pP][tT]:([nN][oO])?[iI][cC][fF](=[0-9]+)?"

        ALIAS DebugInfoDisabled          DebugInfo REMOVE_GROUP
        ALIAS DebugInfoProgramDatabase   "/DEBUG"
        ALIAS DeadCodeEliminate          "/OPT:REF"
        ALIAS DeadCodePreserve           "/OPT:NOREF"
        ALIAS IdenticalCodeDataFold      "/OPT:ICF"
        ALIAS IdenticalCodeDataNoFold    "/OPT:NOICF"
        ALIAS LinkTimeCodeGeneration     "/LTCG"
        ALIAS ControlFlowGuard           "/guard:cf"
        ALIAS NoDefaultLib
      )
  endif()
else()
  igc_flag_register_settings(
      LinkerOptions

      ALIAS DebugInfoDisabled
      ALIAS DebugInfoProgramDatabase
      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
      ALIAS IdenticalCodeDataFold
      ALIAS IdenticalCodeDataNoFold
      ALIAS LinkTimeCodeGeneration
      ALIAS ControlFlowGuard
      ALIAS NoDefaultLib
    )

  if(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT)
    if(_igc_compiler_is_gnu_or_clang)
      igc_flag_register_settings(
          LinkerOptions

          GROUP NAME GcSections "-Wl,--gc-sections"

          ALIAS DeadCodeEliminate  "-Wl,--gc-sections"
          ALIAS DeadCodePreserve   GcSections REMOVE_GROUP
        )
    endif()

  endif()
endif()

# =========================================== Compiler options =========================================

foreach(_compilerFlagName IN ITEMS "CMAKE_CXX_FLAGS" "CMAKE_C_FLAGS")
  # Change some generic settings of compiler.
  # NOTE: Debug info generation is enabled for all build configuration, because it is separate on Windows
  #       and we will use "strip" command on Linux and Android (to separate it).

  if(IGC_OPTION__UNIVERSAL_DRIVER)
    igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET
            CompileAsDefault
            DisableSpecificWarnings
            ExceptionsDisabled
            MultiProcessorCompilation
            DeadCodeEliminate
            CallingConvCdecl
    )
  else()
    igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET
            CompileAsDefault

            DisableSpecificWarnings
            ExceptionsDisabled
            MultiProcessorCompilation
            DeadCodeEliminate
    )
  endif()

  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      ALL_PATTERN_NOINHERIT ""
      SET
        DebugInfoProgramDatabase
        OptimizeSize
        RtMultiThreadedStatic
    )
  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      PATTERN "${IGC_BUILD__DEBUG_PATTERN}"
      SET
        OptimizeDisabled
        RtMultiThreadedStaticDebug
        UnreferencedVariable
    )
  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
      SET
        OptimizeSpeed
        ControlFlowGuard
        # Disable LTCG due to increase of release build time
        #WholeProgramOptimization
    )
  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      PATTERN "${IGC_BUILD__RELINT_PATTERN}"
      SET
        OptimizeSpeed
        ControlFlowGuard
    )


  # Adding needed settings specific to MSVC.
  if(MSVC)
  # Adding needed settings specific to GCC.
  # NOTE: Following options can be needed in the future (although some not recommended: NR):
  # [NR] -fno-short-enums
  # [NR] -fno-tree-pre
  #      -fno-omit-frame-pointer
  #      -Wa,--noexecstack
  #      -fkeep-inline-functions
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
         SET
           AdditionalOptions
           RttiDisabled
           TreatWarnAsErrorEnabled
    )
  elseif( ANDROID )
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          -pipe
          -fmessage-length=0
          -march=corei7
          -mstackrealign
          -fms-extensions
          -fvisibility=hidden
          -Werror
          -Wno-unused-parameter
          -Wno-missing-field-initializers
          -Wwrite-strings
          -Wno-long-long
          -Wswitch
          -Wno-sign-compare
          -Wno-unused-result
          -Wno-enum-compare
          -Wno-type-limits
          -Wno-ignored-qualifiers
          -Wno-format
          -Wno-format-security
          -Wno-extra
          -Wno-write-strings
          -finline
          -fno-strict-aliasing
          -msse
          -msse2
          -msse3
          -mssse3
          -msse4
          -msse4.1
          -msse4.2
          -Wno-unknown-pragmas
          -fPIC
    )
    if(_igc_compiler_is_gnu)
      igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      ALL_PATTERN ""
      SET_RAW
        -funswitch-loops
        -Wno-maybe-uninitialized
    )
    endif()
    if(_igc_compiler_is_gnu OR (_igc_compiler_is_clang AND NOT CMAKE_C_COMPILER_VERSION LESS "4.0"))
      igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      ALL_PATTERN ""
      SET_RAW
            -finline-functions
    )
    endif()

  elseif(_igc_compiler_is_gnu_or_clang)
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          -pipe
          -fmessage-length=0
          -march=corei7
          -mstackrealign
          -fms-extensions
          -Werror
          -Wno-unused-parameter
          -Wno-missing-field-initializers
          -Wwrite-strings
          -Wno-long-long
          -Wswitch
          -Wno-sign-compare
          -Wno-unused-result
          -Wno-enum-compare
          -Wno-type-limits
          -Wno-ignored-qualifiers
          -Wno-shadow
          -Wformat
          -Wformat-security
          -Wno-extra
          -Wno-write-strings
          -finline
          -fno-strict-aliasing
          -msse
          -msse2
          -msse3
          -mssse3
          -msse4
          -msse4.1
          -msse4.2
          -Wno-unknown-pragmas
          -fPIC
      )
      igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
        SET_RAW
          -D_FORTIFY_SOURCE=2
          -fstack-protector
      )
      if(_igc_compiler_is_gnu OR (_igc_compiler_is_clang AND NOT CMAKE_C_COMPILER_VERSION LESS "4.0"))
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          -finline-functions
      )
      endif()
      if(NOT _igc_compiler_is_clang)
        set(flags -funswitch-loops -Wno-maybe-uninitialized)
        if(NOT ANDROID)
          list(APPEND flags -lrt)
        endif()
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          ${flags}
      )
      endif()
  elseif(_igc_compiler_is_clang)
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
        -pipe
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -Wcovered-switch-default
        -Wno-unused-parameter
        -Wno-gnu
        -finline
        -fkeep-inline-functions
        -msse4.2
      )
      if(NOT CMAKE_C_COMPILER_VERSION LESS "4.0")
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          -finline-functions
      )
      endif()
  endif()
endforeach()

if( ANDROID OR _igc_compiler_is_gnu_or_clang)
  igc_config_flag_apply_settings(
      LinkerOptions
      CMAKE_SHARED_LINKER_FLAGS
      ALL_PATTERN ""
      SET_RAW
    -Wl,--no-undefined
    )
endif()

# NOTE: [WA] Workaround for OpenGL-IGC crash problem. Due to different handling of static objects destructors
#            the destructor of PassRegistry was called two times during exiting of OpenGL.
#            This switch is not conformant with C++ standard (order of destruction of static elements is not met) but
#            it is used in Android build system (possibliy due to Android bionic __cxa_atexit limitations).
if(_igc_compiler_is_gnu_or_clang)
  if(ANDROID)
    igc_config_flag_apply_settings(
        CompilerOptions
        CMAKE_CXX_FLAGS
        ALL_PATTERN ""
        SET_RAW
          -fno-use-cxa-atexit
          -fvisibility-inlines-hidden
          -fpermissive
          -fno-rtti
      )
  endif()
endif()

# Unconditionally disable rtti (C++ only)
igc_config_flag_apply_settings(
    CompilerOptions
    CMAKE_CXX_FLAGS
    ALL_PATTERN ""
    SET
       RttiDisabled
  )



if(_igc_compiler_is_gnu_or_clang)
    igc_config_flag_apply_settings(
        CompilerOptions
        CMAKE_CXX_FLAGS
        ALL_PATTERN ""
        SET_RAW
          -fvisibility=hidden
          -fvisibility-inlines-hidden
    )
endif()

# ============================================ Linker options ==========================================

foreach(_linkerFlagName IN ITEMS "CMAKE_EXE_LINKER_FLAGS" "CMAKE_SHARED_LINKER_FLAGS")
  # Change some generic settings of linker.
  # NOTE: Debug info generation is enabled for all build configuration, because it is separate on Windows
  #       and we will use "strip" command on Linux and Android (to separate it).

  igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      ALL_PATTERN ""
      SET
        DebugInfoProgramDatabase
        NoDefaultLib
    )

  igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      ALL_PATTERN_NOINHERIT ""
      SET
        DeadCodeEliminate
        IdenticalCodeDataFold
    )
    igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
      SET
        ControlFlowGuard
        # Disable LTCG due to release build time increase
        #LinkTimeCodeGeneration
    )
    igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      PATTERN "${IGC_BUILD__RELINT_PATTERN}"
      SET
        ControlFlowGuard
    )
    igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      PATTERN "${IGC_BUILD__DEBUG_PATTERN}"
      SET
        DeadCodePreserve
        IdenticalCodeDataNoFold
    )
endforeach()

if(MSVC)
if(LLVM_VERSION_MAJOR EQUAL 8)
message("WA For LLVM8 : Igonre warning 4199 - Need to fix it")
  igc_config_flag_apply_settings(
      LinkerOptions
      CMAKE_SHARED_LINKER_FLAGS
      ALL_PATTERN ""
      SET_RAW
        "/MANIFEST:NO"
        "/ignore:4099"
        "/ignore:4197"
        "/ignore:4199"
      )
else()
  igc_config_flag_apply_settings(
      LinkerOptions
      CMAKE_SHARED_LINKER_FLAGS
      ALL_PATTERN ""
      SET_RAW
        "/MANIFEST:NO"
        "/ignore:4099"
        "/ignore:4197"
      )
endif()
  # Disable LTCG due to release build time increase
  #igc_config_flag_apply_settings(
  #    LinkerOptions
  #    CMAKE_STATIC_LINKER_FLAGS
  #    PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
  #    SET
  #      LinkTimeCodeGeneration
  #)
# Force static linking of common libraries on Android for shared objects.
elseif(_igc_compiler_is_gnu_or_clang)
  if(NOT ANDROID AND NOT IGC_BUILD_DISABLE_FLAGS_GNU_CLANG)
    igc_config_flag_apply_settings(
        LinkerOptions
        CMAKE_SHARED_LINKER_FLAGS
        ALL_PATTERN ""
        SET_RAW
         -Wl,-z,noexecstack
         -Wl,-z,relro
         -Wl,-z,now
      )
  endif()
endif()

if(_igc_compiler_is_clang)
  igc_config_flag_apply_settings(
      LinkerOptions
      CMAKE_SHARED_LINKER_FLAGS
      ALL_PATTERN ""
      SET_RAW
        -shared
        -Wl,-undefined,dynamic_lookup
        -Wl,-headerpad_max_install_names
    )
endif()


# ================================== Compiler preprocessor definitions =================================

# ?  CL_NUMBER                < env:CL_NUMBER if env:CL_NUMBER != ''
# ?* BUILD_NUMBER             < env:BUILD_NUMBER if env:BUILD_NUMBER != ''
# M  IGC_EXPORTS=1            if !IGCStandalone
# M  _COMPILER_DLL_           if Compiler|| GenISAIntrinisics
# *  _CONSOLE                 if visa
# +  _CRT_SECURE_NO_WARNINGS
# +  _DEBUG                   if cfg:Debug
# +  _IGC_
# +  _INTERNAL                if cfg:ReleaseInternal
# *  _LIB                     if build:static library
# +  _SCL_SECURE_NO_WARNINGS
# +* DEBUG                    if cfg:Debug
# +* DLL_CPP                  if cfg:ReleaseInternal && build:shared library
# +  DLL_MODE                 if visa
# M  GHAL3D_SHADERDUMP_ENABLE if IGCStandalone
# +  GHAL3D=USC
# +  ICBE_LHDM                if platform:Windows
# +* INSIDE_PLUGIN
# M  iSTD=USC_iSTD            if IGCStanalone
# +  ISTDLIB_UMD              ?
# +  NDEBUG                   if !cfg:Debug
# +  NOMINMAX
# M* RIIGC                    if IGCStanalone
# +  USC_DXX                  if cfg:Debug
# +  USC_EXPORTS
# +  VER_H                    if !platform:Windows
# M  WIN32                    if platform:Windows && bits:32
# +  WIN64                    if platform:Windows && bits:64
# +  i386=1                   if bits:32
# +  _X86_=1                  if bits:32
# +  _AMD64_                  if bits:64
# + ENABLE_REGISTRY_READ      if cfg:Debug or cfg:ReleaseInternal

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
      _IGC_
      GHAL3D=USC
      _HAS_EXCEPTIONS=0
      IGC_EXPORTS=1
      USC_EXPORTS=1
      ISTDLIB_UMD
      IGC_CMAKE
      __STDC_CONSTANT_MACROS
      __STDC_LIMIT_MACROS
    )

if(DEFINED VME_TYPES_DEFINED)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
      VME_TYPES_DEFINED=${VME_TYPES_DEFINED}
    )
endif()


#SPIRV consumption flag
if(IGC_BUILD__SPIRV_ENABLED)
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
      IGC_SPIRV_ENABLED
    )
endif()

  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
      _SCL_SECURE_NO_WARNINGS
      _CRT_SECURE_NO_WARNINGS
      NOMINMAX
      STD_CALL
      INSIDE_PLUGIN
      _COMPILER_DLL_
    )

# Additional definitions for vISA projects.
set(IGC_BUID__VISA_COMPILE_DEFINITIONS
    DLL_MODE
  )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        _AMD64_
      )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        _X86_=1
        i386=1
      )
  endif()

if(LLVM_ON_WIN32)
  # NOTE: _WIN32 should be defined by compiler.
  # TODO: WIN32 definition is used differently depending on project.
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
      ICBE_LHDM
      _WIN32
      "WIN32"
      WIN32_LEAN_AND_MEAN=1
      WINNT=1
    )

  # NOTE: _WIN64 should be defined by compiler.
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        _WIN64
        WIN64
      )
  endif()
endif()

if(LLVM_ON_UNIX)
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
      ICBE_LINUX
      LINUX

      USE_MMX
      USE_SSE
      USE_SSE2
      USE_SSE3
      USE_SSSE3
    )
endif()


set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG
    _DEBUG
    USC_DXX
    ENABLE_REGISTRY_READ
  )

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASEINTERNAL
    _INTERNAL
    DLL_CPP
    ENABLE_REGISTRY_READ
  )

if(NOT _ASSERTBUILD)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASEINTERNAL
   NDEBUG
  )
endif()

if(NOT ANDROID)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG
    CL_KHR_FP64_EXT
  )
set_property(DIRECTORY APPEND PROPERTY "COMPILE_DEFINITIONS_RELEASE INTERNAL"
    CL_KHR_FP64_EXT
)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
  CL_KHR_FP64_EXT
)
endif()

#NOTE: CMake defines some of compile definitions in compiler flags. We have to make sure that
#      NDEBUG is not one of them when assert build flag is set

if(_ASSERTBUILD)
if(MSVC)
  set(_flagToRemove "/D[ ]*NDEBUG")
elseif(_igc_compiler_is_gnu_or_clang)
  set(_flagToRemove "-DNDEBUG")
endif()
igc_flag_remove_re(CMAKE_C_FLAGS                          "${_flagToRemove}")
igc_flag_remove_re(CMAKE_CXX_FLAGS                        "${_flagToRemove}")
igc_flag_remove_re(CMAKE_C_FLAGS_RELEASEINTERNAL   "${_flagToRemove}")
igc_flag_remove_re(CMAKE_CXX_FLAGS_RELEASEINTERNAL "${_flagToRemove}")
unset(_flagToRemove)
endif()

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE
    NDEBUG
  )

if(DEFINED ENV{TEST_P4_CHANGELIST})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    TB_BUILD_ID=$ENV{TEST_P4_CHANGELIST}
  )
endif()

# ======================================================================================================

if(IGC_BUILD__SHOW_CFLAGS)
  get_cmake_property(_compilerOptionsVarNames VARIABLES)
  foreach(_compilerOptionsVarName ${_compilerOptionsVarNames})
    if(_compilerOptionsVarName MATCHES "^CMAKE_.*_FLAGS(_.*)?$")
      message("[F] ${_compilerOptionsVarName}:    ${${_compilerOptionsVarName}}")
    endif()
  endforeach()
endif()

# ==================================== LLVM package ====================================================

if(TARGET LLVMCore)
    # LLVM targets have been already defined.
    if(LLVM_USE_PREBUILT)
        message(STATUS "[IGC] Using LLVM prebuilt ${LLVM_PACKAGE_VERSION}")
        message(STATUS "[IGC] Using LLVM libraries from: ${LLVM_LIBRARY_DIR}")
    else()
        message(STATUS "[IGC] Using LLVM sources")
    endif()

    message(STATUS "[IGC] Using LLVM includes from: ${LLVM_INCLUDE_DIRS}")
else()

    message(STATUS "[IGC] LLVM targets are not defined. Searching for LLVM.")
    if(NOT IGC_PREFERRED_LLVM_VERSION)
        set(IGC_PREFERRED_LLVM_VERSION "7.0.0")
    endif()

    # Look for LLVM sources.
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../llvm_patches)
      set(IGC_LLVM_HOME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../llvm_patches)
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../external/llvm)
      set(IGC_LLVM_HOME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/llvm)
    endif()

    if(IGC_LLVM_HOME_DIR AND NOT IGC_OPTION__FORCE_SYSTEM_LLVM)
      message(STATUS "[IGC] IGC will build LLVM from sources, because ${IGC_LLVM_HOME_DIR} was found. Use IGC_OPTION__FORCE_SYSTEM_LLVM=1 to use system LLVM.")
      if(IGC_OPTION__LLVM_DUMP_DIR)
         set(LLVM_PREBUILT_PATH ${IGC_OPTION__LLVM_DUMP_DIR})
         include(${IGC_LLVM_HOME_DIR}/llvm_prebuilt.cmake)
      else()
          message(STATUS "[IGC] LLVM compilation from ${IGC_LLVM_HOME_DIR}")
          # Place LLVM build directory inside IGC build directory
          add_subdirectory(${IGC_LLVM_HOME_DIR} ${CMAKE_CURRENT_BINARY_DIR}/llvm/build)
      endif()
    else()
        find_package(LLVM ${IGC_PREFERRED_LLVM_VERSION})
        if(LLVM_FOUND)
          message(STATUS "[IGC] Using system LLVM ${LLVM_PACKAGE_VERSION}")
          set(IGC_BUILD__USING_SYSTEM_LLVM TRUE)
       else()
          message(FATAL_ERROR "[IGC] Unknown location of LLVM component. Couldn't find neither LLVM package in the system nor LLVM source files.")
       endif()
     endif()
endif()

if(NOT DEFINED COMMON_CLANG_LIBRARY_NAME)
    set(COMMON_CLANG_LIBRARY_NAME opencl-clang)
endif()

if(WIN32)
    igc_arch_get_cpu(_cpuSuffix)
    set(COMMON_CLANG_LIBRARY_NAME ${COMMON_CLANG_LIBRARY_NAME}${_cpuSuffix})
    set(COMMON_CLANG_LIB_FULL_NAME "${COMMON_CLANG_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    message(STATUS "OpenCL Clang library name to load: ${COMMON_CLANG_LIB_FULL_NAME}")

    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
          COMMON_CLANG_LIB_FULL_NAME=\"${COMMON_CLANG_LIB_FULL_NAME}\"
        )
else()
    set(COMMON_CLANG_LIB_FULL_NAME "lib${COMMON_CLANG_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

if(LLVM_LINK_LLVM_DYLIB)
    # LLVM was built and configured in a way that tools (in our case IGC) should be linked
    # against single LLVM dynamic library.

    # SET_LLVM_LIB_PATH is a CMake variable that can be passed in to specify the location
    # to look for the LLVM .so. In some cases this is useful if multiple LLVM versions are installed
    if(SET_LLVM_LIB_PATH)
        set(IGC_BUILD__LLVM_LIBS_TO_LINK "${SET_LLVM_LIB_PATH}/libLLVM-${LLVM_VERSION_MAJOR}.so")
        message(STATUS "[IGC] Link against specified LLVM dylib ${IGC_BUILD__LLVM_LIBS_TO_LINK}")
    else()
        find_library(IGC_BUILD__LLVM_LIBS_TO_LINK "libLLVM-${LLVM_VERSION_MAJOR}.so")
        if(IGC_BUILD__LLVM_LIBS_TO_LINK)
          message(STATUS "[IGC] Link against found LLVM dylib ${IGC_BUILD__LLVM_LIBS_TO_LINK}")
        else()
          message(FATAL_ERROR "[IGC] Could not find the LLVM dylib. Aborting.")
        endif()
    endif()

else()
    # LLVM was built into multiple libraries (static or shared).
    message(STATUS "[IGC] Link against LLVM static or shared component libs")

    # Link targets/dependencies (in required link order).
    # NOTE: Since the libraries are grouped in the same link group (in GCC/CLANG),
    #       there is no longer need to order in most dependant first manner.
    set(IGC_BUILD__LLVM_LIBS_TO_LINK
        "LLVMipo"
        "LLVMIRReader"
        "LLVMBitWriter"
        "LLVMBinaryFormat"
        "LLVMAsmParser"
        "LLVMBitReader"
        "LLVMLinker"
        "LLVMCodeGen"
        "LLVMScalarOpts"
        "LLVMTransformUtils"
        "LLVMAnalysis"
        "LLVMTarget"
        "LLVMObjCARCOpts"
        "LLVMVectorize"
        "LLVMInstrumentation"
        "LLVMObject"
        "LLVMMCParser"
        "LLVMProfileData"
        "LLVMMC"
        "LLVMCore"
        "LLVMSupport"
        "LLVMDemangle"
        )

    if(LLVM_VERSION_MAJOR GREATER_EQUAL 8)
        list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
          "LLVMInstCombine"
          )
    endif()

    if(LLVM_VERSION_MAJOR GREATER_EQUAL 9)
        list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
          "LLVMBitstreamReader"
          )
    endif()

endif()

# ==================================== WrapperLLVM package =============================================

add_subdirectory(WrapperLLVM)
igc_sg_define(IGC__WrapperLLVM)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/WrapperLLVM/include")

# ======================================== Include directories =========================================


include_directories(
    "${IGC_CODEGEN_DIR}"

    "${IGC_BUILD__IGC_SRC_DIR}"
    "${IGC_BUILD__IGC_COMMON_DIR}"

    "${IGC_BUILD__GFX_DEV_SRC_DIR}"

    "${IGC_BUILD__COMMON_DIR}"
    "${IGC_BUILD__3D_COMMON_DIR}"

    "${IGC_BUILD__INC_DIR}"
    "${IGC_BUILD__INC_DIR}/common"

    "${IGC_BUILD__COMMON_COMPILER_DIR}"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API"

    "${IGC_BUILD__VISA_DIR}/include"

    "${IGC_OPTION__OUTPUT_DIR}"

    "${IGC_BUILD__IGC_SRC_DIR}/AdaptorOCL/ocl_igc_shared/executable_format"
    "${IGC_BUILD__IGC_SRC_DIR}/AdaptorOCL"
    )


  include_directories(
      "${IGC_BUILD__COMMON_COMPILER_DIR}/common"
  )
  include_directories(
      "${CIF_INCLUDE_DIR}"
  )

include_directories(
    "${LLVM_INCLUDE_DIRS}"
)

if(LLVM_ON_WIN32)
  include_directories(
      "${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d9/api"
      "${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d9"
      "${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d10/API"
    )
endif()

# ==================================== ZE Binary Writer ================================================
add_subdirectory(ZEBinWriter/zebin)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ZEBinWriter/zebin/include")

# ===================================== Projects and source groups =====================================

# Suffixes of main projects.
# __igc_common - contains sources/headers/resources/settings common for all main projects (see list below).
# __igc_dll    - dynamic library for IGC.
# __igc_lib    - static library for IGC.
# Static libraries are only used for some tools.
# NOTE: Due to limitation of Windows resources and exports cannot be passed
#       easily from static library to dynamic. For resources, please use


set(IGC_BUILD__MAIN_IGC_LIB_SUFFIXES
    "__igc_dll"
)

set(IGC_BUILD__PROJ__igc_dll              "${IGC_BUILD__PROJ_NAME_PREFIX}igc_dll")
set(IGC_BUILD__PROJ_LABEL__igc_dll        "${IGC_BUILD__PROJ__igc_dll}")

#FCL
set(IGC_BUILD__PROJ__fcl_dll              "${IGC_BUILD__PROJ_NAME_PREFIX}fcl_dll")
set(IGC_BUILD__PROJ_LABEL__fcl_dll        "${IGC_BUILD__PROJ__fcl_dll}")

set(IGC_BUILD__PROJ__igc_lib              "${IGC_BUILD__PROJ_NAME_PREFIX}igc_lib")
set(IGC_BUILD__PROJ_LABEL__igc_lib        "${IGC_BUILD__PROJ__igc_lib}")

set(IGC_BUILD__PROJ_LABEL__GenX_IR        "${IGC_BUILD__PROJ_NAME_PREFIX}vISA")

add_subdirectory(common)
igc_sg_define(IGC__common)
igc_sg_define(IGC__Common_CLElfLib)


if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(IGC_BUILD__SPIRV_TOOLS_ENABLED ON)
endif()

if(IGC_BUILD__SPIRV_TOOLS_ENABLED)
    add_subdirectory("${IGC_SOURCE_DIR}/../external/SPIRV-Tools" "${IGC_OPTION__OUTPUT_DIR}/external/SPIRV-Tools/config" EXCLUDE_FROM_ALL)
    include_directories("${IGC_BUILD__SPIRV-Headers_DIR}/include")
    include_directories("${IGC_BUILD__SPIRV-Tools_DIR}/include")
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG IGC_SPIRV_TOOLS_ENABLED)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_RELEASEINTERNAL IGC_SPIRV_TOOLS_ENABLED)
    igc_sg_define(IGC__SPIRV-Tools)
endif()

add_subdirectory(GenISAIntrinsics)

if(LLVM_ON_WIN32
        )
  add_subdirectory(AdaptorOCL)
  igc_sg_define(IGC__AdaptorOCL)

endif()


add_subdirectory(Compiler)
add_subdirectory(DriverInterface)
igc_sg_define(IGC__DriverInterface)

# Built-in Functions (and resources).
if(IGC_OPTION__BIF_LINK_BC)
  add_subdirectory(BiFModule)
if(NOT CMAKE_WDDM_LINUX)
  add_subdirectory(ElfPackager)
endif()
endif()

    add_subdirectory(OCLFE)

if(MSVC)
  set(IGC_BUILD__RES__IGC__igc_common
      "${CMAKE_CURRENT_SOURCE_DIR}/igc.rc"
    )
  igc_rc_register_resource(IGC__igc_dll RESOURCES DriverInterface__igc_dll FILE "${IGC_BUILD__RES__IGC__igc_common}") # [WA#2]
  igc_rc_register_resource(IGC__igc_lib RESOURCES DriverInterface__igc_dll FILE "${IGC_BUILD__RES__IGC__igc_common}") # [WA#2]

  igc_rc_get_resource(IGC_BUILD__RES__IGC__igc_dll IGC__igc_dll)
  # Static libraries on Windows cannot pass resources to final libraries.
  set(IGC_BUILD__RES__IGC__igc_lib)
  source_group("Resources" FILES
    ${IGC_BUILD__RES__IGC__igc_dll}
  )
endif()


# Sources/headers for main libraries.
set(IGC_BUILD__SRC__IGC__igc_common
    ${IGC_BUILD__SRC__IGC_AdaptorOCL}
    ${IGC_BUILD__SRC__IGC_common}
    ${IGC_BUILD__SRC__IGC_Common_CLElfLib}
  )


set(IGC_BUILD__SRC__IGC__igc_dll
    ${IGC_BUILD__SRC__IGC__igc_common}
    ${IGC_BUILD__SRC__IGC_DriverInterface__igc_dll}
  )


#FCL
set(IGC_BUILD__SRC__IGC__fcl_dll
    ${IGC_BUILD__SRC__FCL}
  )

set(IGC_BUILD__SRC__IGC__igc_lib
    ${IGC_BUILD__SRC__IGC__igc_dll})

set(IGC_BUILD__HDR__IGC__igc_common
    ${IGC_BUILD__HDR__IGC_AdaptorOCL} # currently empty
    ${IGC_BUILD__HDR__IGC_common}
    ${IGC_BUILD__HDR__IGC_Common_CLElfLib}
    )


set(IGC_BUILD__HDR__IGC__igc_dll
    ${IGC_BUILD__HDR__IGC__igc_common}
    ${IGC_BUILD__HDR__IGC_DriverInterface__igc_dll}
    ${IGC_WrapperLLVM_HDR}
  )

#FCL
set(IGC_BUILD__HDR__IGC__fcl_dll
    ${IGC_BUILD__HDR__FCL}
  )

set(IGC_BUILD__HDR__IGC__igc_lib
    ${IGC_BUILD__HDR__IGC__igc_dll}
  )

#FCL
set(IGC_BUILD__RES__IGC__fcl_dll
    ${IGC_BUILD__RES__FCL}
  )

# Main library targets (library names).
add_library("${IGC_BUILD__PROJ__igc_dll}" SHARED
    ${IGC_BUILD__SRC__IGC__igc_dll}
    ${IGC_BUILD__HDR__IGC__igc_dll}
    ${IGC_BUILD__RES__IGC__igc_dll}
  )

#FCL
  add_library("${IGC_BUILD__PROJ__fcl_dll}" SHARED
    ${IGC_BUILD__SRC__IGC__fcl_dll}
    ${IGC_BUILD__HDR__IGC__fcl_dll}
    ${IGC_BUILD__RES__IGC__fcl_dll}
  )

add_dependencies("${IGC_BUILD__PROJ__fcl_dll}" opencl-clang-lib)


target_include_directories(${IGC_BUILD__PROJ__igc_dll} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared/device_enqueue"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared/executable_format"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/cif"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/"
  "${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/"
)


#IGC_DLL
igc_target_flag_property_add_once_config_var("${IGC_BUILD__PROJ__igc_dll}" LINK_FLAGS IGC_BUILD__EXPORT_SYMBOLS_LINK_FLAGS)
set_property(TARGET "${IGC_BUILD__PROJ__igc_dll}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__igc_dll}")

#FCL
#igc_target_flag_property_add_once_config_var("${IGC_BUILD__PROJ__fcl_dll}" LINK_FLAGS IGC_BUILD__EXPORT_SYMBOLS_LINK_FLAGS)
set_property(TARGET "${IGC_BUILD__PROJ__fcl_dll}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__fcl_dll}")



#Set Universal properties if this is universal build
if(MSVC AND IGC_OPTION__UNIVERSAL_DRIVER)
    bs_set_wdk(${IGC_BUILD__PROJ__igc_dll})
    set_target_properties(${IGC_BUILD__PROJ__igc_dll} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__PROJ__igc_dll} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__PROJ__igc_dll} PROPERTIES VS_TARGET_PLATFORM_VERSION "")
    windows_libs_props_universal(${IGC_BUILD__PROJ__igc_dll})
    bs_set_wdk(${IGC_BUILD__PROJ__fcl_dll})
    set_target_properties(${IGC_BUILD__PROJ__fcl_dll} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__PROJ__fcl_dll} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__PROJ__fcl_dll} PROPERTIES VS_TARGET_PLATFORM_VERSION "")
    windows_libs_props_universal(${IGC_BUILD__PROJ__fcl_dll})
endif()

if(IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS)
  # NOTE: Remember to always pass resources when linking with static library.
  #       Use igc_rc_get_resource() with resource identifier like IGC__igc_lib
  #       Also use IGC_BUILD__LINK_LINE__igc_lib variable to get target_link_libraries()'s line which
  #       links igc_lib correctly (with correct resolve of circular dependencies).
  add_library("${IGC_BUILD__PROJ__igc_lib}" STATIC
      ${IGC_BUILD__SRC__IGC__igc_lib}
      ${IGC_BUILD__HDR__IGC__igc_lib}
    )
  if(MSVC)
    bs_set_wdk(${IGC_BUILD__PROJ__igc_lib})
    windows_libs_props_universal(${IGC_BUILD__PROJ__igc_lib})
  endif()
  set_property(TARGET "${IGC_BUILD__PROJ__igc_lib}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__igc_lib}")
  set_property(TARGET "${IGC_BUILD__PROJ__igc_lib}" PROPERTY EXCLUDE_FROM_ALL YES)
  set(IGC_BUILD__MAIN_IGC_LIB_SUFFIXES
      ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES}
      "__igc_lib"
    )
endif()

if(IGC_OPTION__USCLAUNCHER_TOOL)
  # NOTE: Remember to always pass resources when linking with static library.
  #       Use igc_rc_get_resource() with resource identifier like IGC__igc_libto get proper resource files.
  #       Also use IGC_BUILD__LINK_LINE__igc_lib variable to get target_link_libraries()'s line which
  #       links igc_lib correctly (with correct resolve of circular dependencies).
  add_library("${IGC_BUILD__PROJ__igc_lib}" STATIC
      ${IGC_BUILD__SRC__IGC__igc_lib}
      ${IGC_BUILD__HDR__IGC__igc_lib}
    )
  if(MSVC)
    bs_set_wdk(${IGC_BUILD__PROJ__igc_lib})
    windows_libs_props_universal(${IGC_BUILD__PROJ__igc_lib})
  endif()

  set_property(TARGET "${IGC_BUILD__PROJ__igc_lib}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__igc_lib}")
  set_property(TARGET "${IGC_BUILD__PROJ__igc_lib}" PROPERTY EXCLUDE_FROM_ALL YES)
  set(IGC_BUILD__MAIN_IGC_LIB_SUFFIXES
      ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES}
      "__igc_lib"
    )
endif()

# Output names.
if(LLVM_ON_UNIX)
  set(_cpuSuffix "")
else()

  igc_arch_get_cpu(_cpuSuffix)
endif()
set_target_properties("${IGC_BUILD__PROJ__igc_dll}" PROPERTIES OUTPUT_NAME "${IGC_LIBRARY_NAME}${_cpuSuffix}")
set_target_properties("${IGC_BUILD__PROJ__igc_dll}" PROPERTIES
                      VERSION "${IGC_API_MAJOR_VERSION}.${IGC_API_MINOR_VERSION}.${IGC_API_PATCH_VERSION}"
                      SOVERSION "${IGC_API_MAJOR_VERSION}")
unset(_cpuSuffix)

#FCL
if(LLVM_ON_UNIX)
  set(_cpuSuffix "")
else()
  igc_arch_get_cpu(_cpuSuffix)
endif()
set_target_properties("${IGC_BUILD__PROJ__fcl_dll}" PROPERTIES OUTPUT_NAME "${FCL_LIBRARY_NAME}${_cpuSuffix}")
set_target_properties("${IGC_BUILD__PROJ__fcl_dll}" PROPERTIES
                      VERSION "${IGC_API_MAJOR_VERSION}.${IGC_API_MINOR_VERSION}.${IGC_API_PATCH_VERSION}"
                      SOVERSION "${IGC_API_MAJOR_VERSION}")
unset(_cpuSuffix)

# Injecting external vISA project.
add_subdirectory("${IGC_BUILD__VISA_DIR}" visa)
set_property(TARGET "GenX_IR" APPEND PROPERTY COMPILE_DEFINITIONS
    ${IGC_BUID__VISA_COMPILE_DEFINITIONS}
  )
set_property(TARGET "GenX_IR" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__GenX_IR}")


# =================================== Link targets and dependencies ====================================


foreach(_libBuildSuffix ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES})
  add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"     "${IGC_BUILD__PROJ__GenISAIntrinsics}")
  add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"     MDAutogen)

  if(MSVC AND IGC_OPTION__BIF_LINK_BC)
    add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"   "${IGC_BUILD__PROJ__BiFModule_OCL}")
    add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"   "${IGC_BUILD__PROJ__ElfPackager}")
  endif()
endforeach()

#FCL
add_dependencies("${IGC_BUILD__PROJ__fcl_dll}"     "${IGC_BUILD__PROJ__BiFModule_OCL}")
if (NOT CMAKE_WDDM_LINUX)
add_dependencies("${IGC_BUILD__PROJ__fcl_dll}"     "${IGC_BUILD__PROJ__ElfPackager}")
endif()

if(LLVM_ON_WIN32)
  if(IGC_OPTION__USCLAUNCHER_TOOL)
    add_dependencies("${IGC_BUILD__PROJ__ILAdapter}"           "${IGC_BUILD__PROJ__GenISAIntrinsics}")
  endif()
endif()


foreach(_libBuildSuffix ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES})
  get_property(_targetType TARGET "${IGC_BUILD__PROJ${_libBuildSuffix}}" PROPERTY TYPE)
  if(_targetType MATCHES "^STATIC_LIBRARY$")
    set(_targetIsStatic YES)
  else()
    set(_targetIsStatic NO)
  endif()
  unset(_targetType)

  # list of libraries linked in all configurations
  set(_targetLinkLineCommon)

  # Generated link line for IGC static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()

  if(LLVM_ON_WIN32)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ__AdaptorOCL}"
    )

    if(IGC_OPTION__USCLAUNCHER_TOOL)
      list(APPEND _targetLinkLineCommon
          "${IGC_BUILD__PROJ__ILAdapter}")
    endif()

  endif()

list(APPEND _targetLinkLineCommon zebinlib)

  list(APPEND _targetLinkLineCommon
      "${IGC_BUILD__PROJ__AdaptorOGL}"
  )


  list(APPEND _targetLinkLineCommon
      "${IGC_BUILD__PROJ__Compiler}"
      "${IGC_BUILD__PROJ__GenISAIntrinsics}"
      GenX_IR
    )

  if(LLVM_ON_UNIX AND IGC_OPTION__BIF_LINK_BC)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__WHOLE_ARCHIVE}"
        "${IGC_BUILD__PROJ__BiFLib_OCL}"
        "${IGC_BUILD__NO_WHOLE_ARCHIVE}"
      )
  endif()

  # Generated link line for IGC static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()

  list(APPEND _targetLinkLineCommon
      "${IGC_BUILD__START_GROUP}"
      ${IGC_BUILD__LLVM_LIBS_TO_LINK}
      "${IGC_BUILD__END_GROUP}"
    )

  if(_targetIsStatic)
    # Link line for projects that use IGC as static library (allows to resolve multi-circular dependencies between libraries).
    set("IGC_BUILD__LINK_LINE_RELEASE${_libBuildSuffix}"
        ${_targetLinkLineCommon}
      )
  else()
    if(CMAKE_WDDM_LINUX)
    target_link_libraries("${IGC_BUILD__PROJ${_libBuildSuffix}}" PRIVATE
        "${}"
        "${_targetLinkLineCommon}"
        "${CMAKE_DL_LIBS}"
      )
    target_include_directories("${IGC_BUILD__PROJ${_libBuildSuffix}}" PRIVATE ${LLVM_INCLUDE_DIRS})
    else()
    target_link_libraries("${IGC_BUILD__PROJ${_libBuildSuffix}}" PRIVATE
        "${_targetLinkLineCommon}"
        "${CMAKE_DL_LIBS}"
      )
    endif()
    # Link line for shared / dynamic library requires only library project (all static libs are linked inside).
    set_property(TARGET "${IGC_BUILD__PROJ${_libBuildSuffix}}" PROPERTY LINK_INTERFACE_LIBRARIES "")
    set("IGC_BUILD__LINK_LINE${_libBuildSuffix}"
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()
endforeach()

#FCL
  get_property(_targetType TARGET "${IGC_BUILD__PROJ__fcl_dll}" PROPERTY TYPE)
  if(_targetType MATCHES "^STATIC_LIBRARY$")
    set(_targetIsStatic YES)
  else()
    set(_targetIsStatic NO)
  endif()
  unset(_targetType)


  set(_targetLinkLine)

  # Generated link line for IGC/IGC RS static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLine
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()

  # Generated link line for IGC/IGC RS static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLine
        "${IGC_BUILD__PROJ__fcl_dll}"
      )
  endif()

    if(LLVM_ON_UNIX)
      list(APPEND _targetLinkLine
          "${IGC_BUILD__WHOLE_ARCHIVE}"
          "${IGC_BUILD__PROJ__CTHLib_FCL}"
          "${IGC_BUILD__NO_WHOLE_ARCHIVE}"
        )
    endif()

  list(APPEND _targetLinkLine
      "${IGC_BUILD__START_GROUP}"
      ${IGC_BUILD__LLVM_LIBS_TO_LINK}
      "${IGC_BUILD__END_GROUP}"
    )


list(REMOVE_ITEM _targetLinkLine
    IGC_BUILD__LLVM_LIBS_TO_LINK
    )

if(_targetIsStatic)
    # Link line for projects that use IGC as static library (allows to resolve multi-circular dependencies between libraries).
    set("IGC_BUILD__LINK_LINE__fcl_dll"
        ${_targetLinkLine}
      )
  else()
    if (UNIX)
        set(COMMON_CLANG opencl-clang-lib)
    else()
        set(COMMON_CLANG "")
    endif()

    target_link_libraries("${IGC_BUILD__PROJ__fcl_dll}"
        ${_targetLinkLine}
        ${COMMON_CLANG}
        ${CMAKE_DL_LIBS}
      )
    # Link line for shared / dynamic library requires only library project (all static libs are linked inside).
    set_property(TARGET "${IGC_BUILD__PROJ__fcl_dll}" PROPERTY LINK_INTERFACE_LIBRARIES "")
    set("IGC_BUILD__LINK_LINE__fcl_dll"
        "${IGC_BUILD__PROJ__fcl_dll}"
      )
  endif()

unset(_targetIsStatic)
unset(_targetLinkLine)



win_static_runtime()

if(IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS)
  if (IGC_OPTION__BUILD_IGC_OPT)
    add_subdirectory(igc_opt)
  endif()
  # TODO: If we want IGCStandalone on Linux, someone must clean the code, so it will be compiling.
  if(LLVM_ON_UNIX)
    add_subdirectory("${IGC_BUILD__TOOLS_IGC_DIR}" tools)
  endif()
  if(LLVM_ON_WIN32)
    add_subdirectory("${IGC_BUILD__TOOLS_IGC_DIR}" ist)
  endif()
endif()

if(IGC_OPTION__USCLAUNCHER_TOOL)
  if (IGC_OPTION__BUILD_IGC_OPT)
    add_subdirectory(igc_opt)
  endif()
  # TODO: If we want IGCStandalone on Linux, someone must clean the code, so it will be compiling.
  if(LLVM_ON_WIN32)
    add_subdirectory("${IGC_BUILD__TOOLS_USCLAUNCHER_DIR}" usclauncher)
  endif()
endif()

    if(MSVC)
        add_custom_command( TARGET ${IGC_BUILD__PROJ__igc_dll}
                            POST_BUILD
                            COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${IGC_BUILD__PROJ__igc_dll}>" "$<TARGET_FILE_DIR:${IGC_BUILD__PROJ__igc_opt}>"
                            COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>" "$<TARGET_FILE_DIR:${IGC_BUILD__PROJ__igc_opt}>"
        )
    endif()

# ============================================== LIT TESTS =============================================

add_subdirectory(Compiler/tests)
if(TARGET "check-igc")
  add_dependencies("${IGC_BUILD__PROJ__igc_dll}" "check-igc")
endif()

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================

# Workaround for MSVS10 to avoid the "Dialog Hell"
# NOTE: This could be removed with future version of CMake.
if(MSVC_VERSION EQUAL 1600)
  set(IGC_SLN_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/IGC.sln")
  if (EXISTS "${IGC_SLN_FILENAME}")
    file(APPEND "${IGC_SLN_FILENAME}" "\n# This should be regenerated!\n")
  endif (EXISTS "${IGC_SLN_FILENAME}")
endif()

########################################################################
# cpack to create deb package
########################################################################

set(IGC_PC_PACKAGE_RELEASE "")
if(DEFINED IGC_PACKAGE_RELEASE)
  set(IGC_PC_PACKAGE_RELEASE "-${IGC_PACKAGE_RELEASE}")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/igc.opencl.h.in ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc.opencl.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/igc-opencl.pc.in ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc-opencl.pc @ONLY)

if(UNIX)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-core)
  install(TARGETS ${IGC_BUILD__PROJ__fcl_dll} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_interface DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/cif DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)

  install(
    CODE "file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/postinst \"/sbin/ldconfig\n\" )"
    CODE "file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/postrm \"/sbin/ldconfig\n\" )"
    CODE "file( COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/postinst DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )"
    CODE "file( COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/postrm DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )"
    COMPONENT igc-core
  )

  install(FILES ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc-opencl.pc DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig COMPONENT igc-opencl-devel)
  install(FILES ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc.opencl.h DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
   if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../NOTICES.txt)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../NOTICES.txt DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/igc COMPONENT igc-core)
  endif()
  install(FILES ${IGC_BUILD__VISA_DIR}/include/RelocationInfo.h DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/visa COMPONENT igc-opencl-devel)
  install(DIRECTORY ${IGC_BUILD__VISA_DIR}/iga/IGALibrary/api/ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/iga COMPONENT igc-opencl-devel FILES_MATCHING PATTERN "*.h*")
elseif (WIN32)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll}
          RUNTIME DESTINATION Release/lh64          CONFIGURATIONS Release)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll}
          RUNTIME DESTINATION Release-Internal/lh64 CONFIGURATIONS ReleaseInternal)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll}
          RUNTIME DESTINATION Debug/lh64            CONFIGURATIONS Debug)

  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
          DESTINATION Release/lh64/pdb              CONFIGURATIONS Release)
  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
          DESTINATION Release-Internal/lh64/pdb     CONFIGURATIONS ReleaseInternal)
  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
          DESTINATION Debug/lh64/pdb                CONFIGURATIONS Debug)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # Only copy 64 build
    install(TARGETS ${IGC_BUILD__PROJ__igc_dll} RUNTIME
            DESTINATION ../Test_Tools/Release/x64/Standalones          CONFIGURATIONS Release)
    install(TARGETS ${IGC_BUILD__PROJ__igc_dll} RUNTIME
            DESTINATION ../Test_Tools/Release-Internal/x64/Standalones CONFIGURATIONS ReleaseInternal)
    install(TARGETS ${IGC_BUILD__PROJ__igc_dll} RUNTIME
            DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)

    install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
            DESTINATION ../Test_Tools/Release/x64/Standalones          CONFIGURATIONS Release)
    install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
            DESTINATION ../Test_Tools/Release-Internal/x64/Standalones CONFIGURATIONS ReleaseInternal)
    install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
            DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)
  endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

  install(TARGETS ${IGC_BUILD__PROJ__fcl_dll}
          RUNTIME DESTINATION Release/lh64          CONFIGURATIONS Release)
  install(TARGETS ${IGC_BUILD__PROJ__fcl_dll}
          RUNTIME DESTINATION Release-Internal/lh64 CONFIGURATIONS ReleaseInternal)
  install(TARGETS ${IGC_BUILD__PROJ__fcl_dll}
          RUNTIME DESTINATION Debug/lh64            CONFIGURATIONS Debug)

  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__fcl_dll}>
          DESTINATION Release/lh64/pdb              CONFIGURATIONS Release)
  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__fcl_dll}>
          DESTINATION Release-Internal/lh64/pdb     CONFIGURATIONS ReleaseInternal)
  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__fcl_dll}>
          DESTINATION Debug/lh64/pdb                CONFIGURATIONS Debug)
endif()

if(UNIX)
  if(IGC_OPTION__CPACK_GENERATOR)
    set(CPACK_GENERATOR "${IGC_OPTION__CPACK_GENERATOR}")
  else()
    # If generators list was not defined build native package for current distro
    if(EXISTS "/etc/debian_version")
      set(CPACK_GENERATOR "DEB")
    elseif(EXISTS "/etc/redhat-release")
      set(CPACK_GENERATOR "RPM")
    else()
      set(CPACK_GENERATOR "TXZ")
      set(CPACK_SET_DESTDIR ON)
    endif()
  endif()

  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "intel")
  set(CPACK_PACKAGE_ARCHITECTURE "x86_64")
  set(CPACK_PACKAGE_NAME "intel")
  set(CPACK_PACKAGE_VERSION_MAJOR ${IGC_API_MAJOR_VERSION})
  set(CPACK_PACKAGE_VERSION_MINOR ${IGC_API_MINOR_VERSION})
  set(CPACK_PACKAGE_VERSION_PATCH ${IGC_API_PATCH_VERSION})
  set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${IGC_INSTALL_TIME_ROOT_DIR})
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "postinst;postrm")
  set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Intel(R) Graphics Compiler for OpenCL(TM)")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
  set(CPACK_RPM_PACKAGE_RELOCATABLE TRUE)
  set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")

  set(CPACK_DEBIAN_IGC-CORE_FILE_NAME "intel-igc-core_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-MEDIA_FILE_NAME "intel-igc-media_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-MEDIA_PACKAGE_DEPENDS "intel-igc-core(=${CPACK_PACKAGE_VERSION})")
  set(CPACK_DEBIAN_IGC-OPENCL_FILE_NAME "intel-igc-opencl_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-OPENCL_PACKAGE_DEPENDS "intel-igc-core(=${CPACK_PACKAGE_VERSION})")
  set(CPACK_DEBIAN_IGC-OPENCL-DEVEL_FILE_NAME "intel-igc-opencl-devel_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-OPENCL-DEVEL_PACKAGE_DEPENDS "intel-igc-opencl(=${CPACK_PACKAGE_VERSION})")

  set(CPACK_RPM_IGC-CORE_FILE_NAME "intel-igc-core-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-MEDIA_FILE_NAME "intel-igc-media-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-MEDIA_PACKAGE_REQUIRES "intel-igc-core = ${CPACK_PACKAGE_VERSION}")
  set(CPACK_RPM_IGC-OPENCL_FILE_NAME "intel-igc-opencl-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-OPENCL_PACKAGE_REQUIRES "intel-igc-core = ${CPACK_PACKAGE_VERSION}")
  set(CPACK_RPM_IGC-OPENCL-DEVEL_FILE_NAME "intel-igc-opencl-devel-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-OPENCL-DEVEL_PACKAGE_REQUIRES "intel-igc-opencl = ${CPACK_PACKAGE_VERSION}}")

  set(CPACK_ARCHIVE_IGC-CORE_FILE_NAME "intel-igc-core-${CPACK_PACKAGE_VERSION}.${CPACK_PACKAGE_ARCHITECTURE}")
  set(CPACK_ARCHIVE_IGC-MEDIA_FILE_NAME "intel-igc-media-${CPACK_PACKAGE_VERSION}.${CPACK_PACKAGE_ARCHITECTURE}")
  set(CPACK_ARCHIVE_IGC-OPENCL_FILE_NAME "intel-igc-opencl-${CPACK_PACKAGE_VERSION}.${CPACK_PACKAGE_ARCHITECTURE}")
  set(CPACK_ARCHIVE_IGC-OPENCL-DEVEL_FILE_NAME "intel-igc-opencl-devel-${CPACK_PACKAGE_VERSION}.${CPACK_PACKAGE_ARCHITECTURE}")

  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
  set(CPACK_COMPONENTS_ALL igc-core igc-media igc-opencl igc-opencl-devel)

  include (CPack)
endif(UNIX)

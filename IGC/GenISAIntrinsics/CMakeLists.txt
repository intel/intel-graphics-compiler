#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__GenISAIntrinsics       "${IGC_BUILD__PROJ_NAME_PREFIX}GenISAIntrinsics")
set(IGC_BUILD__PROJ__GenISAIntrinsics       "${IGC_BUILD__PROJ__GenISAIntrinsics}" PARENT_SCOPE)

set(IGC_BUILD__PROJ_LABEL__GenISAIntrinsics "${IGC_BUILD__PROJ__GenISAIntrinsics}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__GenISAIntrinsics_GENERATOR_INPUT_MODULES
    "${CMAKE_CURRENT_SOURCE_DIR}/Intrinsic_definitions.py"
    )

set(IGC_BUILD__GenISAIntrinsics_GENERATOR_SCRIPTS
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/generate_intrinsic_files.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/Intrinsic_definition_objects.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/Intrinsic_definition_translation.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/Intrinsic_generator.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/Intrinsic_utils.py"
    )

set(IGC_BUILD__GenISAIntrinsics_GENERATOR_TEMPLATES
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/templates/GenIntrinsicDescription.h.mako"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/templates/GenIntrinsicDefinition.cpp.mako"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/templates/GenIntrinsicDefinition.h.mako"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/templates/GenIntrinsicEnum.h.mako"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/templates/GenIntrinsicLookupTable.h.mako"
    )

set(IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_HEADERS
    "${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}/GenIntrinsicDescription.h"
    "${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}/GenIntrinsicDefinition.h"
    "${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}/GenIntrinsicEnum.h"
    "${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}/GenIntrinsicLookupTable.h"
    )

set(IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_SOURCES
    "${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}/GenIntrinsicDefinition.cpp"
    )

add_custom_command(
OUTPUT
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_HEADERS}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_SOURCES}
COMMAND
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${BS_DIR_EXTERNAL_COMPONENTS}/build-tools/python-site-packages"
        ${PYTHON_EXECUTABLE} generate_intrinsic_files.py
            "${IGC_BUILD__GenISAIntrinsics_GENERATOR_INPUT_MODULES}" --output ${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}
            $<$<NOT:$<CONFIG:Release>>:"--use_comments">
DEPENDS
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_INPUT_MODULES}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_SCRIPTS}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_TEMPLATES}
WORKING_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/generator
COMMAND_EXPAND_LISTS
    )

add_custom_target(IntrinsicDefintionGenerator
DEPENDS
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_HEADERS}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_SOURCES}
SOURCES
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_INPUT_MODULES}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_SCRIPTS}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_TEMPLATES}
    )

source_group(
TREE
    ${CMAKE_CURRENT_SOURCE_DIR}
FILES
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_INPUT_MODULES}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_SCRIPTS}
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_TEMPLATES}
    )

set_property(TARGET IntrinsicDefintionGenerator PROPERTY FOLDER "CommonGenerators")

set(IGC_BUILD__SRC__GenISAIntrinsics
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsics.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsicFunctions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsicLookup.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LlvmTypesMapping.cpp"
  )

set(IGC_BUILD__HDR__GenISAIntrinsics
    ${IGC_BUILD__GenISAIntrinsics_GENERATOR_OUTPUT_HEADERS}
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsicInst.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsics.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsicFunctions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsicLookup.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LlvmTypesMapping.h"
  )

add_library("${IGC_BUILD__PROJ__GenISAIntrinsics}" STATIC
    ${IGC_BUILD__SRC__GenISAIntrinsics}
    ${IGC_BUILD__HDR__GenISAIntrinsics}
  )
target_include_directories("${IGC_BUILD__PROJ__GenISAIntrinsics}"
PRIVATE
    ${IGC_OPTION__OUTPUT_DIR}/${IGC_CMAKE_CFG_INTDIR}
    )
set_property(TARGET "${IGC_BUILD__PROJ__GenISAIntrinsics}" PROPERTY
    PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__GenISAIntrinsics}")

#set up standard defines from the common WDK path.
if(MSVC)
    bs_set_wdk(${IGC_BUILD__PROJ__GenISAIntrinsics})
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_TARGET_PLATFORM_VERSION "")
    windows_libs_props_universal(${IGC_BUILD__PROJ__GenISAIntrinsics})
endif()

# add dependency for generating llvm intrinsics
if(TARGET intrinsics_gen)
    add_dependencies("${IGC_BUILD__PROJ__GenISAIntrinsics}" intrinsics_gen)
endif()

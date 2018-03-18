# ===========================================================================
# ================================================ UTILS ====================
# ===========================================================================

# ===========================================================================
# ===========================================================================
# ===========================================================================

# ===========================================================================
# ======================================== BUILD CONFIGURATION ==============
# ===========================================================================

set(IGC_BUILD__PROJ__GenISAIntrinsics       "${IGC_BUILD__PROJ_NAME_PREFIX}GenISAIntrinsics")
set(IGC_BUILD__PROJ__GenISAIntrinsics       "${IGC_BUILD__PROJ__GenISAIntrinsics}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__GenISAIntrinsics "${IGC_BUILD__PROJ__GenISAIntrinsics}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__GenISAIntrinsics
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsics.cpp"
  )

set(IGC_BUILD__HDR__GenISAIntrinsics
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsicInst.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenIntrinsics.h"
  )

add_library("${IGC_BUILD__PROJ__GenISAIntrinsics}" STATIC
    ${IGC_BUILD__SRC__GenISAIntrinsics}
    ${IGC_BUILD__HDR__GenISAIntrinsics}
  )
set_property(TARGET "${IGC_BUILD__PROJ__GenISAIntrinsics}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__GenISAIntrinsics}")


# ===========================================================================
# ===========================================================================
# ===========================================================================

# ===========================================================================
# ===================================== BUILD STEP CONFIGURATION ============
# ===========================================================================


set(IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME  "${IGC_BUILD__PROJ__GenISAIntrinsics}Tablegen")
set(IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_LABEL "${IGC_BUILD__PROJ_LABEL__GenISAIntrinsics}-Tablegen")

set(_intrinsicScript "${IGC_SOURCE_DIR}/GenISAIntrinsics/Intrinsics.py")
set(_fileOutput "${IGC_OPTION__OUTPUT_DIR}/IntrinsicGenISA.gen")

    add_custom_command(OUTPUT "${_fileOutput}"
        COMMAND "${IGC_PYTHON}"
        ARGS ${_intrinsicScript} "Intrinsic_definitions.py" ${_fileOutput}
        DEPENDS "${_intrinsicScript}"
        COMMENT "Running Intrinsics.py (updated generated file only if changes were detected)."
      )

add_custom_target("${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME}"
    DEPENDS "${_fileOutput}"
    DEPENDS "${_intrinsicScript}"
    COMMENT "Tablegenning GenISAIntrinsics."
  )

set_property(TARGET "${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME}"
    PROPERTY PROJECT_LABEL "${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_LABEL}")

add_dependencies("${IGC_BUILD__PROJ__GenISAIntrinsics}" "${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME}")

#set up standard defines from the common WDK path.
if(MSVC)
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_PRE_DEFAULT_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/../../WrkSpace/PropertySheets/igfx_env.props")
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__PROJ__GenISAIntrinsics} PROPERTIES VS_TARGET_PLATFORM_VERSION "")

    set_target_properties(${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME} PROPERTIES VS_PRE_DEFAULT_PROPS "${CMAKE_CURRENT_SOURCE_DIR}/../../WrkSpace/PropertySheets/igfx_env.props")
    set_target_properties(${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__GenISAIntrinsics_TABLEGEN_TARGET_NAME} PROPERTIES VS_TARGET_PLATFORM_VERSION "")
endif()

win_static_runtime()


# ============================================================================
# ============================================================================
# ============================================================================

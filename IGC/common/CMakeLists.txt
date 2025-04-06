#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#
#

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__common
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_regkeys.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCConstantFolder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LLVMUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ShaderOverride.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Stats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SysUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCSPIRVParser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BuiltinTypes.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/debug/Debug.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debug/Dump.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debug/TeeOutputStream.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/SystemThread.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MDFrameWork.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ModuleSplitter.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/FunctionUpgrader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntrinsicAnnotator.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/SerializePrintMetaDataPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrintCodeGenContextPass.cpp"
  )

# Add generated source files
list(APPEND IGC_BUILD__SRC__common
    "${IGC_CODEGEN_DIR}/MDNodeFunctions.gen" # Needed by MDFrameWork.cpp
)


list(APPEND IGC_BUILD__SRC__common
  "${CMAKE_CURRENT_SOURCE_DIR}/allocator.cpp"
  ${IGC_BUILD__SRC__Win}
  )

set(IGC_BUILD__SRC__IGC_common
    ${IGC_BUILD__SRC__common}
    ${IGC_BUILD__SRC_Internal__common}
    PARENT_SCOPE
  )

set(IGC_BUILD__HDR__common
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_debug.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_flags.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_flags.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_regkeys.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_regkeys.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_regkeys_enums_defs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_resourceDimTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCConstantFolder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LLVMWarningsPop.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LLVMWarningsPush.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LLVMUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MemStats.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ModuleSplitter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaderOverride.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Stats.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SysUtils.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCSPIRVParser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/BuiltinTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Units.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MDFrameWork.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/debug/Debug.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debug/DebugMacros.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debug/Dump.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debug/TeeOutputStream.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/FunctionUpgrader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntrinsicAnnotator.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/StringMacros.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/SerializePrintMetaDataPass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrintCodeGenContextPass.h"
  )
if(LLVM_ON_WIN32)
  set(IGC_BUILD__HDR_Win__common
      "${CMAKE_CURRENT_SOURCE_DIR}/SystemThread.h"
    )
endif()



list(APPEND IGC_BUILD__HDR__common
  "${CMAKE_CURRENT_SOURCE_DIR}/allocator.h"
  )

set(IGC_BUILD__HDR__IGC_common
    ${IGC_BUILD__HDR__common}
    ${IGC_BUILD__HDR_Win__common}
    ${IGC_BUILD__HDR_Internal__common}
    PARENT_SCOPE
  )


igc_sg_register(
    IGC__common
    "common"
    FILES
      ${IGC_BUILD__SRC__common}
      ${IGC_BUILD__HDR__common}
      ${IGC_BUILD__HDR_Win__common}
  )

# ======================================================================================================

# NOTE: Due to compilation conflict with old build system CLElfLib components were moved here.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/CLElfLib")


set(IGC_BUILD__SRC__Common_CLElfLib
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/CLElfLib/ElfReader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/CLElfLib/ElfWriter.cpp"
  )
set(IGC_BUILD__SRC__IGC_Common_CLElfLib ${IGC_BUILD__SRC__Common_CLElfLib} PARENT_SCOPE)

set(IGC_BUILD__HDR__Common_CLElfLib
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/CLElfLib/CLElfTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/CLElfLib/ElfReader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/CLElfLib/ElfWriter.h"
  )
set(IGC_BUILD__HDR__IGC_Common_CLElfLib ${IGC_BUILD__HDR__Common_CLElfLib} PARENT_SCOPE)


igc_sg_register(
    IGC__Common_CLElfLib
    "ELF"
    FILES
      ${IGC_BUILD__SRC__Common_CLElfLib}
      ${IGC_BUILD__HDR__Common_CLElfLib}
  )

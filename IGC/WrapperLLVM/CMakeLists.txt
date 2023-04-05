#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__WrapperLLVM       "${IGC_BUILD__PROJ_NAME_PREFIX}WrapperLLVM")
set(IGC_BUILD__PROJ__WrapperLLVM       "${IGC_BUILD__PROJ__WrapperLLVM}" PARENT_SCOPE)

set(IGC_WrapperLLVM_HDR
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/ADT/APInt.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/ADT/Optional.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/ADT/STLExtras.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/ADT/StringExtras.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/ADT/StringRef.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Analysis/AliasAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Analysis/CallGraph.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Analysis/InstructionSimplify.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Analysis/MemoryLocation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Analysis/TargetLibraryInfo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/CodeGen/CommandFlags.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/MC/MCContext.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/MC/MCStreamer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/MC/MCObjectFileInfo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Attributes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/IRBuilder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/CallSite.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/CFG.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Comdat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Constant.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/ConstantFold.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/ConstantFolder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Constants.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/DataLayout.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/DerivedTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/DIBuilder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Function.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/InstrTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Instructions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/InstVisitor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/IntrinsicInst.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Intrinsics.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/IRBuilder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Module.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Operator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/PatternMatch.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Type.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/User.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/IR/Value.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Option/OptTable.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/Alignment.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/FileSystem.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/MathExtras.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/MemoryBuffer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/Regex.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/SystemUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/TargetRegistry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/TypeSize.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Support/YAMLParser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Target/TargetMachine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Transforms/InstCombine/InstCombineWorklist.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Transforms/Scalar.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Transforms/Utils/Cloning.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Transforms/Utils/LoopUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Transforms/Utils/Local.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/llvmWrapper/Transforms/Utils/ValueMapper.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/lldWrapper/Common/Driver.h"
  )

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${LLVM_INCLUDE_DIRS}"
    "${IGC_BUILD__IGC_COMMON_DIR}"
)

if(IGC_OPTION__LLVM_MODE STREQUAL PREBUILDS_MODE_NAME AND WIN32)
  include_directories(
    "${SPIRVLLVMTranslator_INCLUDE_DIR}"
  )
endif()

igc_sg_register(
    IGC__WrapperLLVM
    "WrapperLLVM"
    FILES
      ${IGC_WrapperLLVM_HDR}
  )

set(IGC_WrapperLLVM_HDR ${IGC_WrapperLLVM_HDR} PARENT_SCOPE)

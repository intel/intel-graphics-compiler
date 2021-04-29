#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

CMAKE_MINIMUM_REQUIRED( VERSION 3.1 )

PROJECT( CIF )

SET(gPtrSize 32)
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(gPtrSize 64)
ENDIF()

SET(ThirdPartyDir "third_party")

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
IF(NOT MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
ENDIF(NOT MSVC)

INCLUDE_DIRECTORIES( "." )
INCLUDE_DIRECTORIES( "${ThirdPartyDir}/.." )

ADD_SUBDIRECTORY( cif )

MESSAGE( "Gathered CIF headers : ${CIF_HEADERS}" )
MESSAGE( "Gathered CIF headers : ${CIF_SOURCE}" )

MACRO(ADD_RUN_GTEST_TARGET TARGET_NAME CMD_NAME DEPENDENCIES)
    SET(ALL_DEPENDENCIES "${CMD_NAME}" "${DEPENDENCIES}")
    ADD_CUSTOM_TARGET(${TARGET_NAME} ALL DEPENDS ${ALL_DEPENDENCIES})
    ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME}
                       POST_BUILD
                       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                       COMMENT Running ${TARGET_NAME} tests
                       COMMAND ${CMD_NAME} --gtest_repeat=1 --gtest_output=xml:test_out/test_details_18_1_3_8.xml
                      )
ENDMACRO(ADD_RUN_GTEST_TARGET)

ADD_SUBDIRECTORY( cif_tests )


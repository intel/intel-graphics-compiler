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

CMAKE_MINIMUM_REQUIRED( VERSION 3.1 )

PROJECT( CIF )

SET(gPtrSize 32)
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(gPtrSize 64)
ENDIF()
  
SET(ThirdPartyDir "third_party")

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_CXX_STANDARD 14)
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


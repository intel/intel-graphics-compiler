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

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__MetaDataApi
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCMetaDataHelper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataApi.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpirMetaDataApi.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PurgeMetaDataUtils.cpp"
  )
set(IGC_BUILD__SRC__Compiler_MetaDataApi
    ${IGC_BUILD__SRC__MetaDataApi}
    PARENT_SCOPE
  )

set(IGC_BUILD__HDR__MetaDataApi
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCMetaDataHelper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MapList.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataApi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataApiUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataIterator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataTraits.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataValue.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpirMetaDataApi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PurgeMetaDataUtils.hpp"
  )
set(IGC_BUILD__HDR__Compiler_MetaDataApi
    ${IGC_BUILD__HDR__MetaDataApi}
    PARENT_SCOPE
  )

igc_sg_register(
    Compiler__MetaDataApi
    "MetaDataApi"
    FILES
      ${IGC_BUILD__SRC__MetaDataApi}
      ${IGC_BUILD__HDR__MetaDataApi}
  )

##################################################################
# common to both encoders
set(IGA_Backend
  ${CMAKE_CURRENT_SOURCE_DIR}/BitProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/BitProcessor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DecoderOpts.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EncoderOpts.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MessageInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MessageInfo.hpp
  PARENT_SCOPE
)

##################################################################
# GED encoder

set(IGA_Backend_GED_EncoderOnly
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Encoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/EncoderCommon.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDBitProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDBitProcessor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDToIGATranslation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDUtil.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/IGAToGEDTranslation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Interface.hpp
 PARENT_SCOPE
)

##################################################################
# GED decoder

set(IGA_Backend_GED_Decoder
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Decoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Decoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/DecoderCommon.hpp
  PARENT_SCOPE
)

##################################################################
# native encoder
set(IGA_Backend_Native
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/Field.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstDecoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstEncoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstEncoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/Interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/Interface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/MInst.hpp
  PARENT_SCOPE
)

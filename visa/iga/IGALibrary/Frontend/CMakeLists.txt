set(IGA_Frontend_Parser
  ${CMAKE_CURRENT_SOURCE_DIR}/BufferedLexer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Lexemes.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/KernelParser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/KernelParser.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.hpp
  PARENT_SCOPE
)

set(IGA_Frontend_Formatter_Only
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/IRToString.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/Interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/Interface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/LoadBlock.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/LoadCommon.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/LoadScattered.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/Types.hpp
  PARENT_SCOPE
)

set(IGA_Frontend_Sends
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/Interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/Interface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/LoadBlock.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/LoadCommon.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/LoadScattered.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sends/Types.hpp
  PARENT_SCOPE
)
# Not sure why this doesn't work...
# set(IGA_Frontend
#  ${IGA_Frontend_Parser}
#  ${IGA_Frontend_Formatter}
#  PARENT_SCOPE
# )
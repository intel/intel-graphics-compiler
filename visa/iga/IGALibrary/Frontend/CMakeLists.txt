set(IGA_Frontend_Formatter
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/IRToString.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SendDescriptorDecoding.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SendDescriptorDecoding.hpp
  PARENT_SCOPE
)
set(IGA_Frontend_Formatter_LdStSyntax
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/MessageFormatting.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/MessageFormatting.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/MTypes.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/Syntax.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/Syntax.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/Tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/Tables.hpp
  PARENT_SCOPE
)


# The parser component is optional since it requires exceptions and IGC
# has exceptions disabled.  Hence we split these logically
set(IGA_Frontend_Parser
  ${CMAKE_CURRENT_SOURCE_DIR}/BufferedLexer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/KernelParser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/KernelParser.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Lexemes.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.hpp
  PARENT_SCOPE
)

set(IGA_Frontend_Parser_LdStSyntax
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/MessageParsing.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LdStSyntax/MessageParsing.hpp
  PARENT_SCOPE
)


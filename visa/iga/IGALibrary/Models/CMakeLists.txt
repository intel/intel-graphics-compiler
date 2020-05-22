
set(IGA_ModelsOpen
  ${CMAKE_CURRENT_SOURCE_DIR}/Models.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Models.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/OpSpec.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/Model7P5.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/Model8.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/Model9.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/Model10.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/Model11.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/Model12P1.hpp
)
set(IGA_ModelsExteneded )

set(IGA_Models
  ${IGA_ModelsOpen}
  ${IGA_ModelsExteneded}
  PARENT_SCOPE
)

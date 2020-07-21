if(LLVM_ON_WIN32)
  add_compile_options(/experimental:external)
  foreach(INCDIR ${LLVM_INCLUDE_DIRS})
    add_compile_options("SHELL:/external:I ${INCDIR}")
  endforeach()
  add_compile_options(/external:W0)

  # disable 32/64 warnings
  add_compile_options(/wd4244)

  #  disable unary minus to unsigned type warning
  add_compile_options(/wd4146)

  # disable implicitly deleted dtor warning
  add_compile_options(/wd4624)
endif()

add_library(AdaptorCM STATIC
  Frontend.cpp
  InputArgsWrapper.cpp
)

target_link_libraries(AdaptorCM
  LLVMSupport
)

target_compile_definitions(AdaptorCM PRIVATE
                           -DCMFE_WRAPPER_NAME=\"${INSTALL_CMFE_NAME}\")


target_include_directories(AdaptorCM PUBLIC "$<BUILD_INTERFACE:${ADAPTOR_CM_INCLUDE_DIRS}>")

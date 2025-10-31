#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================
# Set precompiled header files for Windows builds
if(MSVC)
message(STATUS "Using precompiled headers")
  set(PCHTargets
    PCH             #0
    Compiler        #1
    AdaptorOCL      #2
    zebinlib        #4
    VISALinkerDriver#5
    Probe           #7
    GenISAIntrinsics#8
    GenX_IR         #9
    GenXDebugInfo   #10
    ${IGC_BUILD__PROJ__igc_lib} #11
    BiFManager #12
  )

  if(IGC_BUILD__VC_ENABLED)
    list(APPEND PCHTargets VCIGCDeps)   #13
  endif()

  if(IGC_BUILD__VULKAN_FRONTEND_ENABLED)
    list(APPEND PCHTargets VulkanFrontend)  #14
  endif()

  #setting reuse indexs
  set(index 0)
  set(reuse_ind 1 2 3 4 5 6 7 11 13)
  #collecting options and definitions pools
  set(opt_list  "")
  set(def_list  "")
  foreach(_target ${PCHTargets})
    # matching reuse index
    set(ind_found FALSE)
    foreach(elem ${reuse_ind})
      if(${elem} EQUAL ${index})
        set(ind_found TRUE)
      endif()
    endforeach()
    # append to list if it is our reuse target
    if(${ind_found})
      get_target_property(OPTIONS ${_target} COMPILE_OPTIONS)
      get_target_property(DEFINITIONS ${_target} COMPILE_DEFINITIONS)
      list(APPEND opt_list ${OPTIONS})
      list(APPEND def_list ${DEFINITIONS})
    endif()
    math(EXPR index "${index} + 1")
  endforeach()

  #remove redundent ones
  list(REMOVE_DUPLICATES opt_list)
  list(REMOVE_DUPLICATES def_list)
  list(REMOVE_ITEM def_list "DEFINITIONS-NOTFOUND")

  #remove redundent ones
  set(index 0) #reinitialize index
  foreach(_target ${PCHTargets})
    add_dependencies(${_target}
      intrinsics_gen # generate *inc files for llvm/IR/Attributes.h
      IntrinsicDefintionGenerator # generate IntrinsicGenISA.gen for GenIntrinsicEnum.h
    )
    # matching reuse index
    set(ind_found FALSE)
    foreach(elem ${reuse_ind})
      if(${elem} EQUAL ${index})
        set(ind_found TRUE)
      endif()
    endforeach()
    if(NOT ${ind_found}) # building PCH
        # setting up for target PCH
        if(index EQUAL 0)
          # matching definitions
          set(diff_defs ${def_list})
          get_target_property(TARGET_COMPILE_DEFINITIONS ${_target} COMPILE_DEFINITIONS) # get current target definition list
          list(REMOVE_ITEM diff_defs ${TARGET_COMPILE_DEFINITIONS}) # remove common ones
          target_compile_definitions(${_target} PUBLIC ${diff_defs})   # apply different ones

          # matching options
          set(diff_opts ${opt_list}) #  copy a opt_list for different list
          get_target_property(TARGET_COMPILE_OPTIONS ${_target} COMPILE_OPTIONS) # get current target option list
          list(REMOVE_ITEM diff_opts ${TARGET_COMPILE_OPTIONS}) # remove common ones
          target_compile_options(${_target} PUBLIC ${diff_opts})   # apply different ones
        endif()
        #building PCH for not reuse targets
        target_precompile_headers(${_target}
          PRIVATE
          "$<$<COMPILE_LANGUAGE:CXX>:${IGC_BUILD__IGC_SRC_DIR}/PCH/llvm.h>"
          "$<$<COMPILE_LANGUAGE:CXX>:${IGC_BUILD__IGC_SRC_DIR}/PCH/common.h>"
        )
    else() # reuse PCH
      # matching definitions for reuse targets
      set(diff_defs ${def_list})
      get_target_property(TARGET_COMPILE_DEFINITIONS ${_target} COMPILE_DEFINITIONS) # get current target definition list
      list(REMOVE_ITEM diff_defs ${TARGET_COMPILE_DEFINITIONS}) # remove common ones
      target_compile_definitions(${_target} PUBLIC ${diff_defs})   # apply different ones

      set(diff_opts ${opt_list}) #  copy a opt_list for different list
      get_target_property(TARGET_COMPILE_OPTIONS ${_target} COMPILE_OPTIONS) # get current target option list
      list(REMOVE_ITEM diff_opts ${TARGET_COMPILE_OPTIONS}) # remove common ones
      target_compile_options(${_target} PUBLIC ${diff_opts})   # apply different ones

      target_precompile_headers(${_target} REUSE_FROM PCH)  # reuse PCH
    endif()
    math(EXPR index "${index} + 1") #update index
  endforeach()
endif()


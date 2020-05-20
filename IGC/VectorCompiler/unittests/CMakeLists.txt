add_custom_target(GenXUnitTests)
set_target_properties(GenXUnitTests PROPERTIES FOLDER "GenXTests")

function(add_genx_unittest test_dirname)
  add_unittest(GenXUnitTests ${test_dirname} ${ARGN})
endfunction()

add_subdirectory(SPIRVConversions)
add_subdirectory(Regions)

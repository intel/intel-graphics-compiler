##############################################################
#
# This file includes the configuration and build rules which
# apply to the xcoder sub-module.
#
##############################################################

# Verify the module makefile watermark.
ifeq ($(GED_GED_MAKEFILE),)
    $(error This file should never be called directly. It should be included from the ged module's makefile)
endif

# Define the sub-module makefile watermark.
ifneq ($(GED_GED_XCODER_MAKEFILE),)
    $(error GED_GED_XCODER_MAKEFILE should only be defined from within this file)
else
    GED_GED_XCODER_MAKEFILE := GED_GED_XCODER_MAKEFILE
endif


##############################################################
#
# Configuration
#
##############################################################

# The following configuration is only relevant when building the xcoder sub-module.
ifeq ($(SUBMODULE),xcoder)

###### Directories ######

ACTIVEDIR := $(XCODERDIR)

###### Compiler flags ######

ACTIVE_IPATH_ROOTS := $(SOURCEDIR) $(GEDDIR)
ACTIVE_CXXFLAGS := $(IGNORE_STRICT_ALIASING) $(IGNORE_IGNORED_QUALIFIERS) $(USESTDLIB) $(USING_CPP11)

endif # ifeq ($(SUBMODULE),xcoder)


##############################################################
#
# Build rules
#
##############################################################

ifneq ($(SUBMODULE),xcoder)

$(OBJDIR)$(TARGET_BUILD)xcoder$(LIB_SUFFIX): $(wildcard $(XCODERDIR)*.cpp $(XCODERDIR)*.h)

endif # ifneq ($(SUBMODULE),xcoder)

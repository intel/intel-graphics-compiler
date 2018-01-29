##############################################################
#
# This file includes the configuration and build rules which
# apply to the common module.
#
##############################################################

# Verify the top-level makefile watermark.
ifeq ($(GED_TOP_LEVEL_MAKEFILE),)
    $(error This file should never be called directly. It should be included from GED's top level makefile)
endif

# Define the module makefile watermark.
ifneq ($(GED_COMMON_MAKEFILE),)
    $(error GED_COMMON_MAKEFILE should only be defined from within this file)
else
    GED_COMMON_MAKEFILE := GED_COMMON_MAKEFILE
endif

# Verify build target compatibility.
ifeq ($(MODULE),common)
    ifeq ($(TARGET_BUILD),)
        $(error TARGET_BUILD must be specified when building the common module)
    endif
    ifneq ($(TARGET_BUILD),exe)
        ifneq ($(TARGET_BUILD),lib)
            ifneq ($(TARGET_BUILD),dll)
                $(error TARGET_BUILD must be one of [exe | lib | dll])
            endif
        endif
    endif
endif # ifeq ($(MODULE),common)


##############################################################
#
# Configuration
#
##############################################################

# The following configuration is only relevant when building the common module.
ifeq ($(MODULE),common)

###### Directories ######

ACTIVEDIR := $(COMMONDIR)

###### Compiler flags ######

ACTIVE_IPATH_ROOTS := $(SOURCEDIR)
ACTIVE_CXXFLAGS := $(IGNORE_MISSING_BRACES)

###### Archiver flags ######

ACTIVE_ARFLAGS += $(IGNORE_UNUSED_OBJECTS)

endif # ifeq ($(MODULE),common)


##############################################################
#
# Build rules
#
##############################################################

ifneq ($(MODULE),common)

$(OBJDIR)execommon$(LIB_SUFFIX): $(wildcard $(COMMONDIR)*.cpp $(COMMONDIR)*.h)
$(OBJDIR)libcommon$(LIB_SUFFIX): $(wildcard $(COMMONDIR)*.cpp $(COMMONDIR)*.h)
$(OBJDIR)dllcommon$(LIB_SUFFIX): $(wildcard $(COMMONDIR)*.cpp $(COMMONDIR)*.h)

$(OBJDIR)$(TARGET_BUILD)common$(LIB_SUFFIX): $(wildcard $(COMMONDIR)*.cpp $(COMMONDIR)*.h)

endif # ifneq ($(MODULE),common)

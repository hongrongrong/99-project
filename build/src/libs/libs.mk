
LIBS_BUILD_DIR = $(BUILD_SRC_DIR)/libs

#include $(LIBS_BUILD_DIR)/*/*.mk
#shared lib
include $(LIBS_BUILD_DIR)/msstd/*.mk
include $(LIBS_BUILD_DIR)/msdb/*.mk

#static lib
#include $(LIBS_BUILD_DIR)/msstd/*.mk



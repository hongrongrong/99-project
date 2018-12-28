
#src build目录
BUILD_SRC_DIR	:= $(BUILD_DIR)/src

#src通用mk文件.导出,为src下的具体makefile文件所使用
BUILD_COMMON_MK	:= $(BUILD_DIR)/src/common.mk
BUILD_RULES_MK	:= $(BUILD_DIR)/src/rules.mk

export BUILD_COMMON_MK
export BUILD_RULES_MK

include $(BUILD_SRC_DIR)/rules.mk

#每个文件路径下的mk文件都要添加至此
include $(BUILD_SRC_DIR)/libs/*.mk
include $(BUILD_SRC_DIR)/profile/*.mk



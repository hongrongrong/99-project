######################################################################
## Filename:      profile.makefile
## Author:        bruce.milesight
## Created at:    2015-11-17
##                
## Description:   profile
##               
## Copyright (C)  milesight
##                
######################################################################
#需要包含的目录
INCLUDE	:= -I$(PUBLIC_INC_DIR) \
		   #-I$(PUBLIC_INC_DIR)/msdb 
#需要的lib库
#LDFLAGS	:= -ldl -L$(PUBLIC_LIB_DIR) -lmsstd -lmsdb -lpthread -L$(STD_USRLIB_DIR) -lsqlite3
LDFLAGS	:= -ldl -L$(PUBLIC_LIB_DIR) -lpthread

#目标类型("exe"-生成可执行文件 "so"-生成动态库 "a"-生成静态库)
TARGET_TYPE:="exe"

#导入通用mk文件
include $(BUILD_COMMON_MK)

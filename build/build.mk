#以下变量在platform中具体指定
#平台类型
PLATFORM        :=
PLATFORM_NAME   :=

#交叉编译器
TARGET_CC       := gcc
TARGET_CPP      :=
TARGET_STRIP    :=
TARGET_AR       :=




#存放编译生成的中间临时(.o .a)文件
TEMP_TARGET_DIR 		:= $(TOP_DIR)/temp/

#存放公共头文件
PUBLIC_INC_DIR			:= $(SRC_DIR)/public/include

#存放DB升级文件
PUBLIC_DB_DIR			:= $(SRC_DIR)/db

#存放编译生成的静态库(.a)文件
PUBLIC_STATIC_LIB_DIR 	:= $(SRC_DIR)/public/lib/

#存放编译后的动态库(.so)文件(for make filesys)
PUBLIC_LIB_DIR  		:= $(TARGETS_DIR)/app/libs

#存放编译后的可执行(app)文件(for make filesys)
PUBLIC_APP_DIR  		:= $(TARGETS_DIR)/app/bin

#以下变量在build/src目录的子mk中被赋值
#所有要编译的 lib
LIBS            :=

#所有要编译的 app
APPS            :=

#所有要编译的 sys
#SYS             :=

#所有要编译的 app
#LIBSCLEAN				:=

#所有要执行的 App clean
CLEANS			:=

#所有要执行的 Sys clean
#SYSCLEANS		:=

#所有要执行的 distclean
DISTCLEANS		:=

#导入src mk,初始化相关变量
include $(BUILD_DIR)/src/src.mk

#编译所有的 lib
libs: $(LIBS)  
libs-clean:$(LIBSCLEAN)#msstd-clean disk-clean cellular-clean ovfcli-clean avilib-clean msdb-clean mssocket-clean libvapi-clean osa-clean encrypt-clean p2pcli-clean msptz-clean


#编译所有的 app
apps: $(LIBS)  $(APPS)

#执行所有的 libs and app clean
appsclean: $(CLEANS) 

#执行所有的  clean
distclean: $(CLEANS) $(DISTCLEANS)


#.PHONY: appsclean sysclean distclean libs apps sysall all
.PHONY: appsclean distclean libs apps

#/*!
# *
# * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
# *
# * Project:  Application Client
# * Author:   Benjamin I. Williams
# * Created:  2004-12-16
# *
# */


# discover base directory
#
X_PWD = $(shell pwd)
ifeq ($(wildcard top.mak), top.mak)
BASE_DIR := $(X_PWD)
else
ifeq ($(wildcard ../top.mak), ../top.mak)
BASE_DIR := $(X_PWD)/..
else
ifeq ($(wildcard ../../top.mak), ../../top.mak)
BASE_DIR := $(X_PWD)/../..
else
$(error cannot find base directory)
endif
endif
endif


# default values (these are overridden by the app_config.mak
# file.  To change these, run the configure utility program
# instead of editing the values below
#
BUILD_MODE=RELEASE
UI_MODE=GUI
OPTIMIZATION_MODE=ON

-include $(BASE_DIR)/app_config.mak



# commands
#
CC = gcc
CPP = g++
STATIC_LINKFLAGS = -static


# optimizations
#
DEBUG_FLAGS = -ggdb -D_DEBUG
OPTIMIZATION_FLAGS = -O2 -fomit-frame-pointer


# large file support
#
LARGEFILE_DEFINES = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES 
THREADING_DEFINES = -D_REENTRANT -D_THREAD_SAFE




# discover platform
# 
ifdef HOMEDRIVE
X_ARCH = WIN32
else
UNAME = $(shell uname)
ifeq ($(UNAME), Darwin)
X_ARCH = MAC
else
X_ARCH = LINUX
endif
endif



# compile flags
#
ifeq ($(BUILD_MODE),DEBUG)
TOP_CFLAGS := $(DEBUG_FLAGS)
endif

TOP_CFLAGS := $(TOP_CFLAGS) $(LARGEFILE_DEFINES)

ifeq ($(OPTIMIZATION_MODE),ON)
TOP_CFLAGS := $(TOP_CFLAGS) $(OPTIMIZATION_FLAGS)
endif

TOP_CFLAGS := $(TOP_CFLAGS) $(THREADING_DEFINES)
TOP_CFLAGS := $(TOP_CFLAGS) $(THREADING_DEFINES) --std=c++11



# directories
#
SUPP_DIR := $(BASE_DIR)/supp

ifeq ($(BUILD_MODE),DEBUG)
WX_DIR := ${SUPP_DIR}/wxWidgets
#WX_DIR := ${SUPP_DIR}/wxWidgetsDebug
else
WX_DIR := ${SUPP_DIR}/wxWidgets
endif


# -- library configuration -------------------------------------------------
#


# check whether we are building a debug or release build
#
ifeq ($(BUILD_MODE),DEBUG)
OUTPUT_DIR := $(BASE_DIR)/debugu
WORK_DIR := debugu
LIB_SUFFIX := ud
WXCONFIG_BUILDMODE=debug
else
OUTPUT_DIR := $(BASE_DIR)/releaseu
WORK_DIR := releaseu
LIB_SUFFIX := u
WXCONFIG_BUILDMODE=release
endif

# check what platform we are building on
#
ifeq ($(X_ARCH),MAC)
WXCONFIG_PLATFORM=mac
else
WXCONFIG_PLATFORM=gtk2
endif

# check which wx version we are using
#

WX_CONFIG := ${WX_DIR}/wx-config


# build path of the wx config utility
#

WX_CFLAGS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --cppflags)
WX_CFLAGSBASE := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --cflags base)

WX_LIBS := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --libs aui,stc,html,adv,net,xml,core,base)
WX_LIBSNONET := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --libs aui,stc,html,adv,xml,core,base)
WX_LIBSBASE := $(shell ${WX_CONFIG} --prefix=${WX_DIR} --libs base)



# glib/gtk configuration (linux only)
#
ifeq ($(X_ARCH), LINUX)
GLIB_CONFIG := pkg-config glib-2.0
GLIB_CFLAGS := $(shell ${GLIB_CONFIG} --cflags)
GLIB_LIBS := $(shell ${GLIB_CONFIG} --libs)
GTK_CONFIG := pkg-config gtk+-2.0
GTK_CFLAGS := $(shell ${GTK_CONFIG} --cflags)
GTK_LIBS := $(shell ${GTK_CONFIG} --libs)
endif


# curl configuration
#
#CURL_CFLAGS := -I${SUPP_DIR}/curl/include
#CURL_LIBS := ${SUPP_DIR}/curl/lib/.libs/libcurl.a ${SUPP_DIR}/libssh2/src/.libs/libssh2.a /usr/lib/libssl.a /usr/lib/libcrypto.a
CURL_CFLAGS := 
CURL_LIBS := -lcurl -lssh2 -lssl -lcrypto


# haru configuration
#
HARU_INCLUDE = -I${SUPP_DIR}/haru/include
HARU_LIBS = ${SUPP_DIR}/haru/src/.libs/libhpdf.a


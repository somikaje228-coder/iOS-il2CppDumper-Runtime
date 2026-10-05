
ARCHS = arm64
DEBUG = 0
FINALPACKAGE = 1
FOR_RELEASE = 1
IGNORE_WARNINGS = 1
TARGET = iphone:clang:latest:11.0

# THEOS_PACKAGE_SCHEME = rootless
  
include $(THEOS)/makefiles/common.mk

ZIP_ARCHIVE_DEFINES = -DHAVE_INTTYPES_H -DHAVE_PKCRYPT -DHAVE_STDINT_H -DHAVE_WZAES -DHAVE_ZLIB

TWEAK_NAME = IOSIl2CppDumper

$(TWEAK_NAME)_FILES = src/Tweak.mm src/AlertUtils.mm \
$(wildcard src/Core/*.cpp) \
$(wildcard includes/SSZipArchive/SSZipArchive.m) $(wildcard includes/SSZipArchive/minizip/*.c)

$(TWEAK_NAME)_CFLAGS = -fobjc-arc $(ZIP_ARCHIVE_DEFINES) -Wno-deprecated-declarations

$(TWEAK_NAME)_CCFLAGS = -std=c++11 -Iincludes -I$(KITTYMEMORY_PATH) -O2 -DkNO_KEYSTONE -DkNO_SUBSTRATE

$(TWEAK_NAME)_LDFLAGS = -lz -liconv -ldl

$(TWEAK_NAME)_FRAMEWORKS = UIKit Foundation Security

include $(THEOS_MAKE_PATH)/tweak.mk
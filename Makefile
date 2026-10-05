ARCHS = arm64 arm64e
TARGET = iphone:clang:latest:15.0
THEOS_PACKAGE_SCHEME = rootless

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = IOSIl2CppDumper

IOSIl2CppDumper_FILES = src/Tweak.mm
IOSIl2CppDumper_FILES += $(wildcard src/Core/*.cpp)
IOSIl2CppDumper_FILES += $(wildcard src/UI/*.mm)
IOSIl2CppDumper_FILES += $(wildcard includes/SSZipArchive/*.c)
IOSIl2CppDumper_FILES += $(wildcard includes/SSZipArchive/*.m)
IOSIl2CppDumper_FILES += $(wildcard includes/minizip/*.c)

IOSIl2CppDumper_FILES += $(wildcard includes/SSZipArchive/minizip/*.c)
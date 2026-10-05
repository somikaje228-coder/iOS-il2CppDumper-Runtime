ARCHS = arm64 arm64e
TARGET = iphone:clang:latest:15.0
THEOS_PACKAGE_SCHEME = rootless

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = IOSIl2CppDumper

IOSIl2CppDumper_FILES = src/Tweak.mm
IOSIl2CppDumper_FILES += src/AlertUtils.mm
IOSIl2CppDumper_FILES += $(wildcard src/Core/*.cpp)
IOSIl2CppDumper_FILES += $(wildcard includes/SSZipArchive/*.c)
IOSIl2CppDumper_FILES += $(wildcard includes/SSZipArchive/*.m)
IOSIl2CppDumper_FILES += $(wildcard includes/SSZipArchive/minizip/*.c)

IOSIl2CppDumper_CFLAGS = -fobjc-arc -Wno-deprecated-literal-operator -Wno-deprecated-declarations -Wno-c99-extensions
IOSIl2CppDumper_CCFLAGS = -std=c++17 -Iincludes
IOSIl2CppDumper_LDFLAGS = -lz -liconv -ldl
IOSIl2CppDumper_FRAMEWORKS = UIKit Foundation

include $(THEOS_MAKE_PATH)/tweak.mk
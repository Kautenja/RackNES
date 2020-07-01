FLAGS += \
	-DTEST \
	-Wno-unused-local-typedefs

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/nes/*.cpp)
SOURCES += $(wildcard src/nes/mappers/*.cpp)
SOURCES += $(wildcard src/nes/apu/*.cpp)
SOURCES += $(wildcard src/nes/ntsc/*.c)

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk

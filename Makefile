FLAGS += \
	-DTEST \
	-Wno-unused-local-typedefs

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/nes/*.cpp)
SOURCES += $(wildcard src/nes/mappers/*.cpp)
SOURCES += $(wildcard src/nes/apu/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk

# Optional production-widget renderer; no audio device or installed plugin needed.
.DEFAULT_GOAL := capture
RACK_DIR ?= ../../../..
RACK_DIR := $(abspath $(RACK_DIR))
include $(RACK_DIR)/arch.mk
BUILD := .build
PYTHON ?= python3
CPPFLAGS += -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include
CXXFLAGS += -std=c++11 -O2 -g
CFLAGS += -O2
ifdef ARCH_X64
CXXFLAGS += -march=nehalem
CFLAGS += -march=nehalem
endif
ifdef ARCH_MAC
GL_LIBS := -framework OpenGL
else ifdef ARCH_WIN
$(error Capture currently supports macOS and Linux desktop sessions)
else
GL_LIBS := -lGL
endif
SOURCES := ../../src/CVGenie.cpp ../../src/base64.cpp $(wildcard ../../src/nes/*.cpp ../../src/nes/apu/*.cpp)
OBJECTS := $(patsubst ../../%.cpp,$(BUILD)/%.o,$(SOURCES)) $(BUILD)/src/nes/ntsc/nes_ntsc.o
.PHONY: capture screenshots
capture: $(BUILD)/capture $(BUILD)/arhythmetic-units.nes
	@mkdir -p $(BUILD)/user
	DYLD_LIBRARY_PATH="$(RACK_DIR)" LD_LIBRARY_PATH="$(RACK_DIR)" $(BUILD)/capture "$(RACK_DIR)" "$(abspath ../..)" "$(abspath $(BUILD))"
screenshots: capture
	$(PYTHON) export_screenshots.py $(BUILD) ../../manual
$(BUILD)/arhythmetic-units.nes: make_demo_rom.py
	@mkdir -p $(BUILD)
	$(PYTHON) $< $@
$(BUILD)/capture: $(BUILD)/capture.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -L$(RACK_DIR) -lRack $(GL_LIBS) -o $@
$(BUILD)/capture.o: capture.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(BUILD)/%.o: ../../%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(BUILD)/%.o: ../../%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
-include $(OBJECTS:.o=.d) $(BUILD)/capture.d

# Standalone assertions against the real Rack SDK, without a new test framework.
.DEFAULT_GOAL := check
RACK_DIR ?= ../../..
RACK_DIR := $(abspath $(RACK_DIR))
BUILD := .build
SANITIZERS ?= -fsanitize=address,undefined -fno-sanitize-recover=all
CPPFLAGS += -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include
CXXFLAGS += -std=c++11 -g -O1 -fno-omit-frame-pointer $(SANITIZERS)
CFLAGS += -g -O1 -fno-omit-frame-pointer $(SANITIZERS)
LDLIBS += -L$(RACK_DIR) -lRack
SOURCES := ../src/base64.cpp $(wildcard ../src/nes/*.cpp ../src/nes/apu/*.cpp)
OBJECTS := $(patsubst ../%.cpp,$(BUILD)/%.o,$(SOURCES)) $(BUILD)/src/nes/ntsc/nes_ntsc.o
.PHONY: check
check: $(BUILD)/cv-genie $(BUILD)/racknes
	DYLD_LIBRARY_PATH="$(RACK_DIR):$$DYLD_LIBRARY_PATH" LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $(BUILD)/cv-genie
	DYLD_LIBRARY_PATH="$(RACK_DIR):$$DYLD_LIBRARY_PATH" LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $(BUILD)/racknes
$(BUILD)/cv-genie: cv_genie.cpp ../src/CVGenie.cpp ../src/GameMaps.hpp ../src/theme.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(BUILD)/cv-genie.d $< $(LDLIBS) -o $@
$(BUILD)/racknes: racknes.cpp ../src/RackNES.cpp ../src/nes/emulator.hpp $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(BUILD)/racknes.d $< $(OBJECTS) $(LDLIBS) -o $@
$(BUILD)/%.o: ../%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(BUILD)/%.o: ../%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
-include $(OBJECTS:.o=.d) $(BUILD)/cv-genie.d $(BUILD)/racknes.d

// A Nintendo Entertainment System (NES) module.
// Copyright 2020 Christian Kauten
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cstring>
#include <filesystem>
#include <string>
#include <jansson.h>
#include "plugin.hpp"
#include "osdialog.h"
#include "components.hpp"
#include "widget/display.hpp"
#include "nes/emulator.hpp"
#include "theme.hpp"

/// a trigger for a button with a CV input.
struct CVButtonTrigger {
    /// the trigger for the button
    dsp::SchmittTrigger buttonTrigger;
    /// the trigger for the CV
    dsp::SchmittTrigger cvTrigger;

    /// Process the input signals.
    ///
    /// @param button the value of the button signal [0, 1]
    /// @param cv the value of the CV signal [-10, 10]
    /// @returns true if either signal crossed a rising edge
    ///
    inline bool process(float button, float cv) {
        bool buttonPress = buttonTrigger.process(button);
        bool cvGate = cvTrigger.process(rescale(cv, 0.1, 2.0f, 0.f, 1.f));
        return buttonPress or cvGate;
    }

    /// Return a boolean determining if either the button or CV gate is high.
    inline bool isHigh() {
        return buttonTrigger.isHigh() or cvTrigger.isHigh();
    }
};

// ---------------------------------------------------------------------------
// MARK: Module
// ---------------------------------------------------------------------------

/// A Nintendo Entertainment System (NES) module.
struct RackNES : Module {
    enum ParamIds {
        PARAM_CLOCK,
        PARAM_CLOCK_ATT,
        ENUMS(PARAM_CH, NES::APU::NUM_CHANNELS),
        PARAM_MIX,
        PARAM_SAVE, PARAM_LOAD, PARAM_HANG, PARAM_RESET,
        PARAM_PLAYER1_A, PARAM_PLAYER1_B, PARAM_PLAYER1_SELECT, PARAM_PLAYER1_START,
        PARAM_PLAYER1_UP, PARAM_PLAYER1_DOWN, PARAM_PLAYER1_LEFT, PARAM_PLAYER1_RIGHT,
        PARAM_PLAYER2_A, PARAM_PLAYER2_B, PARAM_PLAYER2_SELECT, PARAM_PLAYER2_START,
        PARAM_PLAYER2_UP, PARAM_PLAYER2_DOWN, PARAM_PLAYER2_LEFT, PARAM_PLAYER2_RIGHT,
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_PLAYER1_A, INPUT_PLAYER1_B, INPUT_PLAYER1_SELECT, INPUT_PLAYER1_START,
        INPUT_PLAYER1_UP, INPUT_PLAYER1_DOWN, INPUT_PLAYER1_LEFT, INPUT_PLAYER1_RIGHT,
        INPUT_PLAYER2_A, INPUT_PLAYER2_B, INPUT_PLAYER2_SELECT, INPUT_PLAYER2_START,
        INPUT_PLAYER2_UP, INPUT_PLAYER2_DOWN, INPUT_PLAYER2_LEFT, INPUT_PLAYER2_RIGHT,
        INPUT_CLOCK, INPUT_SAVE, INPUT_LOAD, INPUT_HANG, INPUT_RESET,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_CLOCK,
        ENUMS(OUTPUT_CH, NES::APU::NUM_CHANNELS),
        OUTPUT_MIX,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    /// the NES emulator
    NES::Emulator emulator;
    /// the RGBA pixels on the screen in binary representation
    uint8_t screen[NES::Emulator::SCREEN_BYTES];
    /// a pulse generator for generating pulses every frame event
    dsp::PulseGenerator clockGenerator;

    /// a Schmitt Trigger for handling player 1 button inputs
    CVButtonTrigger player1Triggers[8];
    /// a Schmitt Trigger for handling player 2 button inputs
    CVButtonTrigger player2Triggers[8];

    /// triggers for handling button presses and CV inputs for the save input
    CVButtonTrigger saveButton;
    /// triggers for handling button presses and CV inputs for the load input
    CVButtonTrigger loadButton;
    /// triggers for handling button presses and CV inputs for the hang input
    CVButtonTrigger hangButton;
    /// triggers for handling button presses and CV inputs for the reset input
    CVButtonTrigger resetButton;
    /// the NES emulator backup state
    json_t* backup = nullptr;

    /// a data signal from the widget for when the user selects a new ROM
    std::string rom_path_signal = "";
    /// a flag for telling the widget that a ROM file load was attempted for a
    /// ROM with a mapper that has not been implemented yet
    bool mapper_not_found_signal = false;
    /// a flag for telling the widget that a ROM file load failed for an
    /// unknown reason
    bool rom_load_failed_signal = false;
    /// a flag for telling the widget that a ROM file load (from JSON) failed
    bool rom_reload_failed_signal = false;

    /// a clock divider for running CV acquisition slower than audio rate
    dsp::ClockDivider cvDivider;

    /// messages from CV Genie expander
    uint16_t rightMessages[2][8][2] = {};

    /// Initialize a new Nintendo Entertainment System (NES) module.
    RackNES() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PARAM_CLOCK, -4.f, 4.f, 0.f, "Clock Speed", " MHz", 2.f, NES::CLOCK_RATE / 1000000.f);
        configParam(PARAM_CLOCK_ATT, -1.f, 1.f, 0.f, "Clock Speed CV Attenuverter", "%", 0.f, 100.f);
        configParam(PARAM_CH + 0, 0.f, 2.f, 1.f, "Square 1 Volume", "%", 0.f, 100.f);
        configParam(PARAM_CH + 1, 0.f, 2.f, 1.f, "Square 2 Volume", "%", 0.f, 100.f);
        configParam(PARAM_CH + 2, 0.f, 2.f, 1.f, "Triangle Volume", "%", 0.f, 100.f);
        configParam(PARAM_CH + 3, 0.f, 2.f, 1.f, "Noise Volume", "%",    0.f, 100.f);
        configParam(PARAM_CH + 4, 0.f, 2.f, 1.f, "DMC Volume", "%",      0.f, 100.f);
        configParam(PARAM_MIX,    0.f, 2.f, 1.f, "Mix Volume", "%",      0.f, 100.f);
        configButton(PARAM_SAVE,           "Save State");
        configButton(PARAM_LOAD,           "Load State");
        configButton(PARAM_HANG,           "Hang Emulation");
        configButton(PARAM_RESET,          "Reset NES");
        configButton(PARAM_PLAYER1_A,      "Player 1 A");
        configButton(PARAM_PLAYER1_B,      "Player 1 B");
        configButton(PARAM_PLAYER1_SELECT, "Player 1 Select");
        configButton(PARAM_PLAYER1_START,  "Player 1 Start");
        configButton(PARAM_PLAYER1_UP,     "Player 1 Up");
        configButton(PARAM_PLAYER1_DOWN,   "Player 1 Down");
        configButton(PARAM_PLAYER1_LEFT,   "Player 1 Left");
        configButton(PARAM_PLAYER1_RIGHT,  "Player 1 Right");
        configButton(PARAM_PLAYER2_A,      "Player 2 A");
        configButton(PARAM_PLAYER2_B,      "Player 2 B");
        configButton(PARAM_PLAYER2_SELECT, "Player 2 Select");
        configButton(PARAM_PLAYER2_START,  "Player 2 Start");
        configButton(PARAM_PLAYER2_UP,     "Player 2 Up");
        configButton(PARAM_PLAYER2_DOWN,   "Player 2 Down");
        configButton(PARAM_PLAYER2_LEFT,   "Player 2 Left");
        configButton(PARAM_PLAYER2_RIGHT,  "Player 2 Right");
        // Configure metadata for the input and output ports
        configInput(INPUT_PLAYER1_A,       "Player 1 \"A\" gate");
        configInput(INPUT_PLAYER1_B,       "Player 1 \"B\" gate");
        configInput(INPUT_PLAYER1_SELECT,  "Player 1 \"Select\" gate");
        configInput(INPUT_PLAYER1_START,   "Player 1 \"Start\" gate");
        configInput(INPUT_PLAYER1_UP,      "Player 1 \"Up\" gate");
        configInput(INPUT_PLAYER1_DOWN,    "Player 1 \"Down\" gate");
        configInput(INPUT_PLAYER1_LEFT,    "Player 1 \"Left\" gate");
        configInput(INPUT_PLAYER1_RIGHT,   "Player 1 \"Right\" gate");
        configInput(INPUT_PLAYER2_A,       "Player 2 \"A\" gate");
        configInput(INPUT_PLAYER2_B,       "Player 2 \"B\" gate");
        configInput(INPUT_PLAYER2_SELECT,  "Player 2 \"Select\" gate");
        configInput(INPUT_PLAYER2_START,   "Player 2 \"Start\" gate");
        configInput(INPUT_PLAYER2_UP,      "Player 2 \"Up\" gate");
        configInput(INPUT_PLAYER2_DOWN,    "Player 2 \"Down\" gate");
        configInput(INPUT_PLAYER2_LEFT,    "Player 2 \"Left\" gate");
        configInput(INPUT_PLAYER2_RIGHT,   "Player 2 \"Right\" gate");
        configInput(INPUT_CLOCK,           "CPU clock speed");
        configInput(INPUT_SAVE,            "Save state trigger");
        configInput(INPUT_LOAD,            "Load state trigger");
        configInput(INPUT_HANG,            "Hang gate");
        configInput(INPUT_RESET,           "Reset trigger");
        configOutput(OUTPUT_CLOCK,         "CPU clock");
        configOutput(OUTPUT_CH + 0,        "Square voice 1");
        configOutput(OUTPUT_CH + 1,        "Square voice 2");
        configOutput(OUTPUT_CH + 2,        "Triangle voice");
        configOutput(OUTPUT_CH + 3,        "Noise voice");
        configOutput(OUTPUT_CH + 4,        "DMC sample voice");
        configOutput(OUTPUT_MIX,           "Audio mix");
        // set the division for the CV processing
        cvDivider.setDivision(16);
        // draw the initial screen
        initalizeScreen();
        // set the emulator's clock rate to the Rack rate
        emulator.set_clock_rate(768000);
        emulator.set_sample_rate(APP->engine->getSampleRate());
        // initialize expander messages
        rightExpander.producerMessage = rightMessages[0];
        rightExpander.consumerMessage = rightMessages[1];
    }

    /// Handle a new ROM being loaded into the emulator.
    void handleNewROM() {
        // create a new emulator with the specified ROM and reset it
        if (NES::Cartridge::is_valid_rom(rom_path_signal)) {  // ROM file valid
            // if load game returns true, the load succeeded
            if (emulator.load_game(rom_path_signal)) {
                // remove the existing backup if there is one
                if (backup != nullptr) delete backup;
                backup = nullptr;
                // done loading, return to caller
                return;
            }
            // ROM load failed, initialize screen and send error signal
            initalizeScreen();
            // send a mapper not found signal to the widget to display a
            // UI dialog to the user
            mapper_not_found_signal = true;
        } else {  // ROM file not valid, initialize screen and send error signal
            initalizeScreen();
            // send a ROM load failure signal to the widget to display a
            // UI dialog to the user
            rom_load_failed_signal = true;
        }
    }

    /// Initialize the screen with empty pixels.
    inline void initalizeScreen() {
        std::memset(screen, 0, NES::Emulator::SCREEN_BYTES);
    }

    /// Copy the RGBA screen buffer from the NES to the local screen buffer.
    inline void copyScreen() {
        std::memcpy(screen, emulator.get_screen_buffer(), NES::Emulator::SCREEN_BYTES);
    }

    /// Return the clock speed of the NES.
    inline uint64_t getClockSpeed() {
        // get the control voltage scaled in [-2, 2]
        auto cv = inputs[INPUT_CLOCK].getVoltage() / 5.f;
        // apply the attenuverter to the CV signal
        cv *= params[PARAM_CLOCK_ATT].getValue();
        // get the parameter in [-4, 4]
        auto param = params[PARAM_CLOCK].getValue();
        // calculate the exponential frequency
        return NES::CLOCK_RATE * powf(2.f, clamp(param + cv, -4.f, 4.f));
    }

    /// Process the inputs from the panel.
    void processCV() {
        // process the hang input for hanging the emulation
        hangButton.process(
            params[PARAM_HANG].getValue(),
            inputs[INPUT_HANG].getVoltage()
        );

        // NOTE: process the save, reset, restore in given order to ensure
        // that when all go high on the same frame, the emulator stays in its
        // current state
        // handle inputs to the save button and CV
        if (saveButton.process(
            params[PARAM_SAVE].getValue(),
            inputs[INPUT_SAVE].getVoltage()
        )) {
            // delete existing save
            if (backup != nullptr) delete backup;
            // create a new save of the NES state
            backup = emulator.dataToJson();
        }
        // handle inputs to the reset button and CV
        if (resetButton.process(
            params[PARAM_RESET].getValue(),
            inputs[INPUT_RESET].getVoltage()
        )) emulator.reset();
        // handle inputs to the load button and CV
        if (loadButton.process(
            params[PARAM_LOAD].getValue(),
            inputs[INPUT_LOAD].getVoltage()
        ) && backup != nullptr) emulator.dataFromJson(backup);

        // get the controller for both players as a byte where each bit
        // represents the gate signal for whether one of the 8 buttons are
        // held
        NES::NES_Byte player1 = 0;
        NES::NES_Byte player2 = 0;
        // iterate over the buttons
        for (std::size_t button = 0; button < 8; button++) {
            {  // player 1 scope
                // process the voltage with the Schmitt Trigger
                player1Triggers[button].process(
                    params[PARAM_PLAYER1_A + button].getValue(),
                    inputs[INPUT_PLAYER1_A + button].getVoltage()
                );
                // the position for the current button's index
                player1 += player1Triggers[button].isHigh() << button;
            } {  // player 2 scope
                // process the voltage with the Schmitt Trigger
                player2Triggers[button].process(
                    params[PARAM_PLAYER2_A + button].getValue(),
                    inputs[INPUT_PLAYER2_A + button].getVoltage()
                );
                // the position for the current button's index
                player2 += player2Triggers[button].isHigh() << button;
            }
        }
        // set the controller values
        emulator.set_controllers(player1, player2);
    }

    /// Process messages to/from expander modules.
    void processExpanders() {
        if (rightExpander.module) {  // an expander exists to the right
            // check if the expander is an Input Genie
            if (rightExpander.module->model == modelInputGenie) {
                // Get message from the Input Genie
                uint16_t *message = reinterpret_cast<uint16_t*>(rightExpander.consumerMessage);
                // Write requested values from message to requested memory locations
                for (int i = 0; i < 16; i += 2) {
                    if (message[i] != 0) {  // data available for consumption
                        // write the address, data tuple to the emulator
                        emulator.get_memory_buffer()[message[i]] = message[i + 1];
                        // consume the data by setting the address to 0
                        message[i] = 0;
                    }
                }
            }
            /// TODO: Output Genie
            /* // check if the expander is an Output Genie
            else if (rightExpander.module->model == modelOutputGenie) {
                // get producer message from RackNES in order to fill it and flip it into a consumer message for RackNES
                uint16_t *message = (uint16_t*) rightExpander.module->leftExpander.producerMessage;
                // iterate over all of the Output Genie's memory location selectors
                for (int i = 0; i < 8; i++) {
                    // check if the selector has been assigned

                }
            } */
        }
    }

    /// Process a sample.
    void process(const ProcessArgs &args) override {
        // check for a new ROM to load
        if (!rom_path_signal.empty()) {
            handleNewROM();
            // clear the ROM path signal from the widget
            rom_path_signal = "";
            // set the sample rate of the emulator
            // onSampleRateChange();
        }

        // process CV if the CV clock divider is high
        if (cvDivider.process())
            processCV();
        // process expanders at every sample step
        processExpanders();

        // stop processing if the hang button is high
        if (hangButton.isHigh()) return;

        // run the number of cycles through the NES that are required. pass a
        // callback to copy the screen every time a new frame renders
        for (std::size_t i = 0; i < getClockSpeed() / args.sampleRate; i++)
            emulator.cycle([&]() { copyScreen(); });

        // set the clock output based on the NES frame-rate
        outputs[OUTPUT_CLOCK].setVoltage(10.f * emulator.is_clock_high());
        // create a placeholder for the mix output
        float mix = 0.f;
        // iterate over the synthesis channels on the NES
        for (std::size_t i = 0; i < NES::APU::NUM_CHANNELS; i++) {
            // get the level of the channel from the knob's position
            auto level = params[PARAM_CH + i].getValue();
            // get the voltage for this channel
            auto voltage = level * emulator.get_audio_voltage(i);
            // integrate the voltage to the mix if the channel is not connected
            if (!outputs[OUTPUT_CH + i].isConnected()) mix += voltage;
            // set the output voltage for the channel
            outputs[OUTPUT_CH + i].setVoltage(voltage);
        }
        // set the output voltage for the channel mix
        outputs[OUTPUT_MIX].setVoltage(params[PARAM_MIX].getValue() * mix);
    }

    /// @brief Respond to sample rate of the host environment changing.
    void onSampleRateChange() override {
        emulator.set_sample_rate(APP->engine->getSampleRate());
    }

    /// @brief Respond to the module being reset by the host environment.
    void onReset() override {
        emulator.remove_game();
        if (backup != nullptr) { delete backup; backup = nullptr; }
        initalizeScreen();
    }

    /// @brief Convert the module's state to a JSON object.
    ///
    /// @returns a pointer to a new json_t object with the module's state
    ///
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "emulator", emulator.dataToJson());
        // make sure there is a backup JSON before trying to save it
        if (backup != nullptr) {
            json_object_set_new(rootJ, "backup", json_deep_copy(backup));
        }
        return rootJ;
    }

    /// @brief Load the module's state from a JSON object.
    ///
    /// @param rootJ a pointer to a json_t with state data for this module
    ///
    void dataFromJson(json_t* rootJ) override {
        json_t* emulator_data = json_object_get(rootJ, "emulator");
        // load emulator
        if (emulator_data) {
            // set the reload signal based on whether the reload from JSON
            // succeeded. dataFromJson returns true for success, false for fail
            rom_reload_failed_signal = !emulator.dataFromJson(emulator_data);
            // if the reload failed, get out of here
            if (rom_reload_failed_signal) return;
        }
        // load backup
        json_t* backup_data = json_object_get(rootJ, "backup");
        // delete any existing backup before overwriting
        if (backup != nullptr) delete backup;
        backup = nullptr;
        // set the backup data if there is one, otherwise just nullptr it.
        // the initial JSON that is passed in is dynamically allocated by the
        // caller, so deep copy it.
        if (backup_data) backup = json_deep_copy(backup_data);
    }
};

// ---------------------------------------------------------------------------
// MARK: Widget
// ---------------------------------------------------------------------------

/// A menu item for loading ROMs into the emulator.
struct ROMMenuItem : MenuItem {
    /// the module associated with the menu item
    RackNES* module = nullptr;

    /// Respond to an action on the menu item.
    void onAction(const event::Action &e) override {
        // check for a ROM path to use as an existing directory
        auto rom_path = module->emulator.get_rom_path();
        // if the ROM path is empty, fall back on the user's home directory
        auto dir = rom_path.empty() ?
            asset::user("") : rack::system::getDirectory(rom_path);
        // filter to for files with a ".nes" or ".NES" extension
        auto filter = osdialog_filters_parse("NES ROM:nes,NES");
        auto path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, filter);
        osdialog_filters_free(filter);
        if (path) {  // the user selected a path
            module->rom_path_signal = path;
            free(path);
        }
    }
};

/// The basename for the RackNES panel files.
const char BASENAME[] = "res/RackNES";

/// The widget structure that lays out the panel of the module and the UI menus.
struct RackNESWidget : ThemedWidget<BASENAME> {
    /// a pointer to the display to render the NES screen to
    Display* display = nullptr;

    /// Create a new NES widget for the given NES module.
    ///
    /// @param module the module to create a widget for
    ///
    RackNESWidget(RackNES* module) {
        setModule(module);
        // setup the display for the NES screen
        display = new Display(
            Vec(157, 18),                                         // screen position
            static_cast<RackNES*>(module)->screen,                // pixel buffer
            Vec(NES::Emulator::WIDTH, NES::Emulator::HEIGHT),     // buffer size
            Vec(NES::Emulator::WIDTH_NES, NES::Emulator::HEIGHT)  // image size
        );
        addChild(display);
        // panel screws
        addChild(createWidget<ScrewSilver>(Vec(7 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 8 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(7 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        // clock controls
        addOutput(createOutput<PJ301MPort>(Vec(116, 49), module, RackNES::OUTPUT_CLOCK));
        addParam(createParam<Rogan3PSNES>(Vec(107, 91), module, RackNES::PARAM_CLOCK));
        addParam(createParam<Rogan1PRed>(Vec(114, 151), module, RackNES::PARAM_CLOCK_ATT));
        addInput(createInput<PJ301MPort>(Vec(116, 213), module, RackNES::INPUT_CLOCK));
        // emulator controls
        addInput(createInput<PJ301MPort>(Vec(421, 48), module, RackNES::INPUT_SAVE));
        addInput(createInput<PJ301MPort>(Vec(421, 103), module, RackNES::INPUT_LOAD));
        addInput(createInput<PJ301MPort>(Vec(421, 158), module, RackNES::INPUT_HANG));
        addInput(createInput<PJ301MPort>(Vec(421, 213), module, RackNES::INPUT_RESET));
        addParam(createParam<NESSwitchVertical>(Vec(454, 40), module, RackNES::PARAM_SAVE));
        addParam(createParam<NESSwitchVertical>(Vec(454, 95), module, RackNES::PARAM_LOAD));
        addParam(createParam<NESSwitchVertical>(Vec(454, 150), module, RackNES::PARAM_HANG));
        addParam(createParam<NESSwitchVertical>(Vec(454, 205), module, RackNES::PARAM_RESET));
        // global knobs
        addOutput(createOutput<PJ301MPort>(Vec(162, 279), module, RackNES::OUTPUT_CH + 0));
        addOutput(createOutput<PJ301MPort>(Vec(206, 279), module, RackNES::OUTPUT_CH + 1));
        addOutput(createOutput<PJ301MPort>(Vec(250, 279), module, RackNES::OUTPUT_CH + 2));
        addOutput(createOutput<PJ301MPort>(Vec(294, 279), module, RackNES::OUTPUT_CH + 3));
        addOutput(createOutput<PJ301MPort>(Vec(338, 279), module, RackNES::OUTPUT_CH + 4));
        addOutput(createOutput<PJ301MPort>(Vec(382, 279), module, RackNES::OUTPUT_MIX));
        addParam(createParam<Rogan2PRed>(Vec(158, 321), module, RackNES::PARAM_CH + 0));
        addParam(createParam<Rogan2PRed>(Vec(202, 321), module, RackNES::PARAM_CH + 1));
        addParam(createParam<Rogan2PRed>(Vec(246, 321), module, RackNES::PARAM_CH + 2));
        addParam(createParam<Rogan2PRed>(Vec(290, 321), module, RackNES::PARAM_CH + 3));
        addParam(createParam<Rogan2PRed>(Vec(334, 321), module, RackNES::PARAM_CH + 4));
        addParam(createParam<Rogan2PRed>(Vec(378, 321), module, RackNES::PARAM_MIX));
        // player 1 inputs
        addInput(createInput<PJ301MPort>(Vec(62, 22),  module, RackNES::INPUT_PLAYER1_UP));
        addInput(createInput<PJ301MPort>(Vec(62, 68),  module, RackNES::INPUT_PLAYER1_DOWN));
        addInput(createInput<PJ301MPort>(Vec(62, 114), module, RackNES::INPUT_PLAYER1_LEFT));
        addInput(createInput<PJ301MPort>(Vec(62, 160), module, RackNES::INPUT_PLAYER1_RIGHT));
        addInput(createInput<PJ301MPort>(Vec(62, 206), module, RackNES::INPUT_PLAYER1_SELECT));
        addInput(createInput<PJ301MPort>(Vec(62, 252), module, RackNES::INPUT_PLAYER1_START));
        addInput(createInput<PJ301MPort>(Vec(62, 289), module, RackNES::INPUT_PLAYER1_B));
        addInput(createInput<PJ301MPort>(Vec(62, 335), module, RackNES::INPUT_PLAYER1_A));
        // player 1 buttons
        addParam(createParam<CKD6>(Vec(24, 13),  module, RackNES::PARAM_PLAYER1_UP));
        addParam(createParam<CKD6>(Vec(24, 59),  module, RackNES::PARAM_PLAYER1_DOWN));
        addParam(createParam<CKD6>(Vec(24, 105), module, RackNES::PARAM_PLAYER1_LEFT));
        addParam(createParam<CKD6>(Vec(24, 152), module, RackNES::PARAM_PLAYER1_RIGHT));
        addParam(createParam<CKD6>(Vec(24, 199), module, RackNES::PARAM_PLAYER1_SELECT));
        addParam(createParam<CKD6>(Vec(24, 244), module, RackNES::PARAM_PLAYER1_START));
        addParam(createParam<CKD6_NES_Red>(Vec(24, 290), module, RackNES::PARAM_PLAYER1_B));
        addParam(createParam<CKD6_NES_Red>(Vec(24, 336), module, RackNES::PARAM_PLAYER1_A));
        // player 2 inputs
        addInput(createInput<PJ301MPort>(Vec(482, 22),  module, RackNES::INPUT_PLAYER2_UP));
        addInput(createInput<PJ301MPort>(Vec(482, 68),  module, RackNES::INPUT_PLAYER2_DOWN));
        addInput(createInput<PJ301MPort>(Vec(482, 114), module, RackNES::INPUT_PLAYER2_LEFT));
        addInput(createInput<PJ301MPort>(Vec(482, 160), module, RackNES::INPUT_PLAYER2_RIGHT));
        addInput(createInput<PJ301MPort>(Vec(482, 206), module, RackNES::INPUT_PLAYER2_SELECT));
        addInput(createInput<PJ301MPort>(Vec(482, 252), module, RackNES::INPUT_PLAYER2_START));
        addInput(createInput<PJ301MPort>(Vec(482, 289), module, RackNES::INPUT_PLAYER2_B));
        addInput(createInput<PJ301MPort>(Vec(482, 335), module, RackNES::INPUT_PLAYER2_A));
        // player 2 buttons
        addParam(createParam<CKD6>(Vec(515, 13),  module, RackNES::PARAM_PLAYER2_UP));
        addParam(createParam<CKD6>(Vec(515, 59),  module, RackNES::PARAM_PLAYER2_DOWN));
        addParam(createParam<CKD6>(Vec(515, 105), module, RackNES::PARAM_PLAYER2_LEFT));
        addParam(createParam<CKD6>(Vec(515, 152), module, RackNES::PARAM_PLAYER2_RIGHT));
        addParam(createParam<CKD6>(Vec(515, 199), module, RackNES::PARAM_PLAYER2_SELECT));
        addParam(createParam<CKD6>(Vec(515, 244), module, RackNES::PARAM_PLAYER2_START));
        addParam(createParam<CKD6_NES_Red>(Vec(515, 290), module, RackNES::PARAM_PLAYER2_B));
        addParam(createParam<CKD6_NES_Red>(Vec(515, 336), module, RackNES::PARAM_PLAYER2_A));
    }

    /// Draw the widget in the rack window.
    ///
    /// @param args the draw arguments for this render call
    ///
    void draw(const DrawArgs& args) override {
        // call the super call to get all default behaviors of the superclass
        ModuleWidget::draw(args);
        // set the screen state based on the existence of the module
        display->is_on = module != nullptr;
        // make sure the module has been initialized before proceeding. module
        // will be null when viewing the module in the browser
        if (module == nullptr) return;
        // re-scope the module as the RackNES subtype (guaranteed) for signal
        // handling in this UI context
        auto module = static_cast<RackNES*>(this->module);
        // handle signal from module that ROM file has unimplemented mapper
        if (module->mapper_not_found_signal) {
            module->mapper_not_found_signal = false;
            static constexpr auto MSG = "ASIC mapper not implemented for ROM!";
            osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, MSG);
        }
        // handle signal from module that ROM load failed for unknown reason
        if (module->rom_load_failed_signal) {
            module->rom_load_failed_signal = false;
            static constexpr auto MSG = "ROM file failed to load!";
            osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, MSG);
        }
        // handle signal from module that ROM in JSON was not found
        if (module->rom_reload_failed_signal) {
            module->rom_reload_failed_signal = false;
            static constexpr auto MSG = "ROM file was not found!";
            osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, MSG);
        }
    }

    /// Add context items for the module to a menu on the UI.
    ///
    /// @param menu the menu to add the context items to
    ///
    inline void appendContextMenu(ui::Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(construct<ROMMenuItem>(
            &ROMMenuItem::text,
            "Load ROM",
            &ROMMenuItem::module,
            static_cast<RackNES*>(this->module)
        ));
        ThemedWidget<BASENAME>::appendContextMenu(menu);
    }

    /// Respond to a path being dropped onto the module.
    ///
    /// @param event the event data for the path drop event
    ///
    inline void onPathDrop(const event::PathDrop& event) override {
        static_cast<RackNES*>(module)->rom_path_signal = std::string(event.paths[0]);
    }
};

Model *modelRackNES = createModel<RackNES, RackNESWidget>("RackNES");

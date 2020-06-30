// A Nintendo Entertainment System (NES) module.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
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
#include <string>
#include <jansson.h>
#include "plugin.hpp"
#include "osdialog.h"
#include "components.hpp"
#include "nes/emulator.hpp"

// TODO: resolve segmentation fault from loading emulator state where the ROM
//       path has moved

/// a trigger for a button with a CV input.
struct CVButtonTrigger {
    /// the trigger for the button
    rack::dsp::SchmittTrigger buttonTrigger;
    /// the trigger for the CV
    rack::dsp::SchmittTrigger cvTrigger;

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
        CLOCK_PARAM,
        CLOCK_ATT_PARAM,
        VOLUME_PARAM,
        BACKUP_PARAM, RESTORE_PARAM, RESET_PARAM,
        PLAYER1_A_PARAM, PLAYER1_B_PARAM, PLAYER1_SELECT_PARAM, PLAYER1_START_PARAM,
        PLAYER1_UP_PARAM, PLAYER1_DOWN_PARAM, PLAYER1_LEFT_PARAM, PLAYER1_RIGHT_PARAM,
        PLAYER2_A_PARAM, PLAYER2_B_PARAM, PLAYER2_SELECT_PARAM, PLAYER2_START_PARAM,
        PLAYER2_UP_PARAM, PLAYER2_DOWN_PARAM, PLAYER2_LEFT_PARAM, PLAYER2_RIGHT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PLAYER1_A_INPUT, PLAYER1_B_INPUT, PLAYER1_SELECT_INPUT, PLAYER1_START_INPUT,
        PLAYER1_UP_INPUT, PLAYER1_DOWN_INPUT, PLAYER1_LEFT_INPUT, PLAYER1_RIGHT_INPUT,
        PLAYER2_A_INPUT, PLAYER2_B_INPUT, PLAYER2_SELECT_INPUT, PLAYER2_START_INPUT,
        PLAYER2_UP_INPUT, PLAYER2_DOWN_INPUT, PLAYER2_LEFT_INPUT, PLAYER2_RIGHT_INPUT,
        CLOCK_INPUT, BACKUP_INPUT, RESTORE_INPUT, RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CLOCK_OUTPUT,
        SOUND_OUTPUT,
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

    /// triggers for handling button presses and CV inputs for the reset input
    CVButtonTrigger resetButton;
    /// triggers for handling button presses and CV inputs for the backup input
    CVButtonTrigger backupButton;
    /// triggers for handling button presses and CV inputs for the restore input
    CVButtonTrigger restoreButton;
    /// the NES emulator backup state
    json_t* backup = nullptr;

    /// a data signal from the widget for when the user selects a new ROM
    std::string rom_path_signal = "";
    /// a signal flag for detecting sample rate changes
    bool new_sample_rate = false;
    /// a flag for telling the widget that a ROM file load was attempted for a
    /// ROM with a mapper that has not been implemented yet
    bool mapper_not_found_signal = false;
    /// a flag for telling the widget that a ROM file load failed for an
    /// unknown reason
    bool rom_load_failed_signal = false;
    /// a flag for telling the widget that a ROM file load (from JSON) failed
    bool rom_reload_failed_signal = false;

    /// Initialize a new Nintendo Entertainment System (NES) module.
    RackNES() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CLOCK_PARAM, -4.f, 4.f, 0.f, "Clock Speed", " MHz", 2.f, NES::CLOCK_RATE / 1000000.f);
        configParam(CLOCK_ATT_PARAM, -1.f, 1.f, 0.f, "Clock Speed CV Attenuverter", "%", 0.f, 100.f);
        configParam(VOLUME_PARAM, 0.f, 2.f, 1.f, "Volume", "%", 0.f, 100.f);
        // buttons
        configParam(BACKUP_PARAM,         0.f, 1.f, 0.f, "Save State");
        configParam(RESTORE_PARAM,        0.f, 1.f, 0.f, "Load State");
        configParam(RESET_PARAM,          0.f, 1.f, 0.f, "Reset NES");
        configParam(PLAYER1_A_PARAM,      0.f, 1.f, 0.f, "Player 1 A");
        configParam(PLAYER1_B_PARAM,      0.f, 1.f, 0.f, "Player 1 B");
        configParam(PLAYER1_SELECT_PARAM, 0.f, 1.f, 0.f, "Player 1 Select");
        configParam(PLAYER1_START_PARAM,  0.f, 1.f, 0.f, "Player 1 Start");
        configParam(PLAYER1_UP_PARAM,     0.f, 1.f, 0.f, "Player 1 Up");
        configParam(PLAYER1_DOWN_PARAM,   0.f, 1.f, 0.f, "Player 1 Down");
        configParam(PLAYER1_LEFT_PARAM,   0.f, 1.f, 0.f, "Player 1 Left");
        configParam(PLAYER1_RIGHT_PARAM,  0.f, 1.f, 0.f, "Player 1 Right");
        configParam(PLAYER2_A_PARAM,      0.f, 1.f, 0.f, "Player 2 A");
        configParam(PLAYER2_B_PARAM,      0.f, 1.f, 0.f, "Player 2 B");
        configParam(PLAYER2_SELECT_PARAM, 0.f, 1.f, 0.f, "Player 2 Select");
        configParam(PLAYER2_START_PARAM,  0.f, 1.f, 0.f, "Player 2 Start");
        configParam(PLAYER2_UP_PARAM,     0.f, 1.f, 0.f, "Player 2 Up");
        configParam(PLAYER2_DOWN_PARAM,   0.f, 1.f, 0.f, "Player 2 Down");
        configParam(PLAYER2_LEFT_PARAM,   0.f, 1.f, 0.f, "Player 2 Left");
        configParam(PLAYER2_RIGHT_PARAM,  0.f, 1.f, 0.f, "Player 2 Right");
        // draw the initial screen
        initalizeScreen();
        // set the emulator's clock rate to the Rack rate
        emulator.set_clock_rate(768000);
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
        auto cv = inputs[CLOCK_INPUT].getVoltage() / 5.f;
        // apply the attenuverter to the CV signal
        cv *= params[CLOCK_ATT_PARAM].getValue();
        // get the parameter in [-4, 4]
        auto param = params[CLOCK_PARAM].getValue();
        // calculate the exponential frequency
        return NES::CLOCK_RATE * powf(2.f, rack::clamp(param + cv, -4.f, 4.f));
    }

    /// Return the output audio from the NES after applying the volume.
    inline float getAudio() {
        return params[VOLUME_PARAM].getValue() * emulator.get_audio_voltage();
    }

    /// Process a sample.
    void process(const ProcessArgs &args) override {
        // check for a new ROM to load
        if (!rom_path_signal.empty()) {
            handleNewROM();
            // clear the ROM path signal from the widget
            rom_path_signal = "";
            // set the sample rate of the emulator
            new_sample_rate = true;
        }

        // check for sample rate changes from the engine to send to the NES
        if (new_sample_rate) {
            emulator.set_sample_rate(static_cast<uint32_t>(args.sampleRate));
            new_sample_rate = false;
        }

        // NOTE: process the backup, reset, restore in given order to ensure
        // that when all go high on the same frame, the emulator stays in its
        // current state
        // handle inputs to the backup button and CV
        if (backupButton.process(
            params[BACKUP_PARAM].getValue(),
            inputs[BACKUP_INPUT].getVoltage()
        )) {
            // delete existing backup
            if (backup != nullptr) delete backup;
            // create a new backup of the NES state
            backup = emulator.dataToJson();
        }
        // handle inputs to the reset button and CV
        if (resetButton.process(
            params[RESET_PARAM].getValue(),
            inputs[RESET_INPUT].getVoltage()
        )) emulator.reset();
        // handle inputs to the restore button and CV
        if (restoreButton.process(
            params[RESTORE_PARAM].getValue(),
            inputs[RESTORE_INPUT].getVoltage()
        ) && backup != nullptr) emulator.dataFromJson(backup);

        // get the controller for both players as a byte where each bit
        // represents the gate signal for whether one of the 8 buttons are
        // held
        NES::NES_Byte player1 = 0;
        NES::NES_Byte player2 = 0;
        // iterate over the buttons
        for (int button = 0; button < 8; button++) {
            {  // player 1 scope
                // process the voltage with the Schmitt Trigger
                player1Triggers[button].process(
                    params[PLAYER1_A_PARAM + button].getValue(),
                    inputs[PLAYER1_A_INPUT + button].getVoltage()
                );
                // the position for the current button's index
                player1 += player1Triggers[button].isHigh() << button;
            } {  // player 2 scope
                // process the voltage with the Schmitt Trigger
                player2Triggers[button].process(
                    params[PLAYER2_A_PARAM + button].getValue(),
                    inputs[PLAYER2_A_INPUT + button].getVoltage()
                );
                // the position for the current button's index
                player2 += player2Triggers[button].isHigh() << button;
            }
        }
        // set the controller values
        emulator.set_controllers(player1, player2);

        // run the number of cycles through the NES that are required. pass a
        // callback to copy the screen every time a new frame renders
        for (uint32_t i = 0; i < getClockSpeed() / args.sampleRate; i++)
            emulator.cycle([&]() { copyScreen(); });

        // set the clock output based on the NES frame-rate
        outputs[CLOCK_OUTPUT].setVoltage(10.f * emulator.is_clock_high());
        // get the sound output from the emulator
        outputs[SOUND_OUTPUT].setVoltage(getAudio());
    }

    /// Respond to the change of sample rate in the engine.
    void onSampleRateChange() override { new_sample_rate = true; }

    /// Respond to the user resetting the module with the "Initialize" action.
    void onReset() override {
        emulator.remove_game();
        if (backup != nullptr) { delete backup; backup = nullptr; }
        initalizeScreen();
    }

    /// Convert the module's state to a JSON object.
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "emulator", emulator.dataToJson());
        // make sure there is a backup JSON before trying to save it
        if (backup != nullptr) {
            json_object_set_new(rootJ, "backup", json_deep_copy(backup));
        }
        return rootJ;
    }

    /// Load the module's state from a JSON object.
    void dataFromJson(json_t* rootJ) override {
        json_t* emulator_data = json_object_get(rootJ, "emulator");
        // load emulator
        if (emulator_data) {
            // set the reload signal based on whether the reload from JSON
            // succeeded. dataFromJson returns true for success, false for fail
            rom_reload_failed_signal = !emulator.dataFromJson(emulator_data);
            // if the reload failed, get out of here
            if (rom_reload_failed_signal) return;
            new_sample_rate = true;
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

/// The widget structure that lays out the panel of the module and the UI menus.
struct RackNESWidget : ModuleWidget {
    /// a pointer to the image to draw the screen to
    int screen = -1;

    /// Create a new NES widget for the given NES module.
    ///
    /// @param module the module to create a widget for
    ///
    RackNESWidget(RackNES* module) {
        setModule(module);
        static constexpr auto panel = "res/RackNES.svg";
        setPanel(APP->window->loadSvg(asset::plugin(plugin_instance, panel)));
        // panel screws
        addChild(createWidget<ScrewSilver>(Vec(7 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 8 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(7 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        // global inputs
        addInput(createInput<PJ301MPort>(Vec(118, 279), module, RackNES::CLOCK_INPUT));
        addInput(createInput<PJ301MPort>(Vec(202, 279), module, RackNES::BACKUP_INPUT));
        addInput(createInput<PJ301MPort>(Vec(244, 279), module, RackNES::RESTORE_INPUT));
        addInput(createInput<PJ301MPort>(Vec(286, 279), module, RackNES::RESET_INPUT));
        // global outputs
        addOutput(createOutput<PJ301MPort>(Vec(161, 279), module, RackNES::CLOCK_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec(329, 279), module, RackNES::SOUND_OUTPUT));
        // global buttons
        addParam(createParam<CKD6>(Vec(199, 313), module, RackNES::BACKUP_PARAM));
        addParam(createParam<CKD6>(Vec(242, 313), module, RackNES::RESTORE_PARAM));
        addParam(createParam<CKD6>(Vec(283, 313), module, RackNES::RESET_PARAM));
        // global knobs
        addParam(createParam<Rogan3PSNES>(Vec(152, 328), module, RackNES::CLOCK_PARAM));
        addParam(createParam<Rogan1PRed>(Vec(116, 317), module, RackNES::CLOCK_ATT_PARAM));
        addParam(createParam<Rogan2PRed>(Vec(325, 321), module, RackNES::VOLUME_PARAM));
        // player 1 inputs
        addInput(createInput<PJ301MPort>(Vec(62, 22), module, RackNES::PLAYER1_UP_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 68), module, RackNES::PLAYER1_DOWN_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 114), module, RackNES::PLAYER1_LEFT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 160), module, RackNES::PLAYER1_RIGHT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 206), module, RackNES::PLAYER1_SELECT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 252), module, RackNES::PLAYER1_START_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 289), module, RackNES::PLAYER1_B_INPUT));
        addInput(createInput<PJ301MPort>(Vec(62, 335), module, RackNES::PLAYER1_A_INPUT));
        // player 1 buttons
        addParam(createParam<CKD6>(Vec(24, 13), module, RackNES::PLAYER1_UP_PARAM));
        addParam(createParam<CKD6>(Vec(24, 59), module, RackNES::PLAYER1_DOWN_PARAM));
        addParam(createParam<CKD6>(Vec(24, 105), module, RackNES::PLAYER1_LEFT_PARAM));
        addParam(createParam<CKD6>(Vec(24, 152), module, RackNES::PLAYER1_RIGHT_PARAM));
        addParam(createParam<CKD6>(Vec(24, 199), module, RackNES::PLAYER1_SELECT_PARAM));
        addParam(createParam<CKD6>(Vec(24, 244), module, RackNES::PLAYER1_START_PARAM));
        addParam(createParam<CKD6>(Vec(24, 290), module, RackNES::PLAYER1_B_PARAM));
        addParam(createParam<CKD6>(Vec(24, 336), module, RackNES::PLAYER1_A_PARAM));
        // player 2 inputs
        addInput(createInput<PJ301MPort>(Vec(392, 22), module, RackNES::PLAYER2_UP_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 68), module, RackNES::PLAYER2_DOWN_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 114), module, RackNES::PLAYER2_LEFT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 160), module, RackNES::PLAYER2_RIGHT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 206), module, RackNES::PLAYER2_SELECT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 252), module, RackNES::PLAYER2_START_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 289), module, RackNES::PLAYER2_B_INPUT));
        addInput(createInput<PJ301MPort>(Vec(392, 335), module, RackNES::PLAYER2_A_INPUT));
        // player 2 buttons
        addParam(createParam<CKD6>(Vec(425, 13), module, RackNES::PLAYER2_UP_PARAM));
        addParam(createParam<CKD6>(Vec(425, 59), module, RackNES::PLAYER2_DOWN_PARAM));
        addParam(createParam<CKD6>(Vec(425, 105), module, RackNES::PLAYER2_LEFT_PARAM));
        addParam(createParam<CKD6>(Vec(425, 152), module, RackNES::PLAYER2_RIGHT_PARAM));
        addParam(createParam<CKD6>(Vec(425, 199), module, RackNES::PLAYER2_SELECT_PARAM));
        addParam(createParam<CKD6>(Vec(425, 244), module, RackNES::PLAYER2_START_PARAM));
        addParam(createParam<CKD6>(Vec(425, 290), module, RackNES::PLAYER2_B_PARAM));
        addParam(createParam<CKD6>(Vec(425, 336), module, RackNES::PLAYER2_A_PARAM));
    }

    /// Draw the widget in the rack window.
    ///
    /// @param args the draw arguments for this render call
    ///
    void draw(const DrawArgs& args) override {
        // call the super call to get all default behaviors of the superclass
        ModuleWidget::draw(args);
        // the x position of the screen
        static constexpr int x = 112;
        // the y position of the screen
        static constexpr int y = 18;
        // the width of the screen
        static constexpr int w = NES::Emulator::WIDTH;
        // the height of the screen
        static constexpr int h = NES::Emulator::HEIGHT;
        // make sure the module has been initialized before proceeding. module
        // will be null when viewing the module in the browser
        if (module == nullptr)
            return;
        // get the rendered screen from the module
        auto nesModule = static_cast<RackNES*>(module);
        auto pixels = reinterpret_cast<const uint8_t*>(nesModule->screen);
        // draw the screen
        if (screen == -1)  // check if the screen has been initialized yet
            screen = nvgCreateImageRGBA(args.vg, w, h, 0, pixels);
        else  // update the screen with the pixel data
            nvgUpdateImage(args.vg, screen, pixels);
        // get the screen as a fill paint (for a rectangle)
        auto imgPaint = nvgImagePattern(args.vg, x, y, w, h, 0, screen, 1.0f);
        // create a path for the rectangle to show the screen
        nvgBeginPath(args.vg);
        // create a rectangle to draw the screen
        nvgRect(args.vg, x, y, w, h);
        // paint the rectangle's fill from the screen
        nvgFillPaint(args.vg, imgPaint);
        nvgFill(args.vg);

        // handle signal from module that ROM file has unimplemented mapper
        if (nesModule->mapper_not_found_signal) {
            nesModule->mapper_not_found_signal = false;
            static constexpr auto MSG = "ASIC mapper not implemented for ROM!";
            osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, MSG);
        }
        // handle signal from module that ROM load failed for unknown reason
        if (nesModule->rom_load_failed_signal) {
            nesModule->rom_load_failed_signal = false;
            static constexpr auto MSG = "ROM file failed to load!";
            osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, MSG);
        }
        // handle signal from module that ROM in JSON was not found
        if (nesModule->rom_reload_failed_signal) {
            nesModule->rom_reload_failed_signal = false;
            static constexpr auto MSG = "ROM file was not found!";
            osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, MSG);
        }
    }

    /// A menu item for loading ROMs into the emulator.
    struct ROMItem : MenuItem {
        /// the module associated with the menu item
        RackNES* module = nullptr;

        /// Respond to an action on the menu item.
        void onAction(const event::Action &e) override {
            // check for a ROM path to use as an existing directory
            auto rom_path = module->emulator.get_rom_path();
            // if the ROM path is empty, fall back on the user's home directory
            auto dir = rom_path.empty() ?
                asset::user("") : rack::string::directory(rom_path);
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

    /// Add a content menu to the module widget.
    void appendContextMenu(ui::Menu* menu) override {
        static constexpr auto TEXT = "Load ROM";
        auto module = dynamic_cast<RackNES*>(this->module);
        menu->addChild(construct<MenuLabel>());
        menu->addChild(construct<ROMItem>(&ROMItem::text, TEXT, &ROMItem::module, module));
    }
};


Model *modelRackNES = createModel<RackNES, RackNESWidget>("RackNES");

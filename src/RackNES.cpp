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

#include <string>
#include <jansson.h>
#include "plugin.hpp"
#include "osdialog.h"
#include "components.hpp"
#include "clock.hpp"
#include "nes/emulator.hpp"

// the values of buttons on the NES
// enum class NESButtons {
//     NoOp =   0b00000000,
//     A =      0b00000001,
//     B =      0b00000010,
//     Select = 0b00000100,
//     Start =  0b00001000,
//     Up =     0b00010000,
//     Down =   0b00100000,
//     Left =   0b01000000,
//     Right =  0b10000000,
// };

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
    NES::Emulator* emulator = nullptr;
    /// the path to the ROM for the emulator
    std::string rom_path = "";
    /// a signal determining if a game was inserted into the emulator, i.e.,
    /// from the widget on the UI thread
    bool did_insert_game = false;
    /// a signal flag for detecting sample rate changes in the process loop
    bool new_sample_rate = false;
    /// the RGBA pixels on the screen in binary representation
    uint8_t screen[NES::Emulator::SCREEN_BYTES];

    /// the clock for maintaining the frame-rate of the NES (60Hz)
    Clock clock;
    /// a Schmitt Trigger for handling clock rising edges
    rack::dsp::SchmittTrigger clockTrigger;

    /// triggers for handling button presses and CV inputs for the backup input
    CVButtonTrigger backupButton;
    /// triggers for handling button presses and CV inputs for the restore input
    CVButtonTrigger restoreButton;
    /// triggers for handling button presses and CV inputs for the reset input
    CVButtonTrigger resetButton;

    /// a Schmitt Trigger for handling player 1 button inputs
    CVButtonTrigger player1Triggers[8];
    /// a Schmitt Trigger for handling player 2 button inputs
    CVButtonTrigger player2Triggers[8];

    /// Initialize a new Nintendo Entertainment System (NES) module.
    RackNES() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CLOCK_PARAM, -4.f, 4.f, 0.f, "Clock Speed", " Hz", 2.f, 60.f);
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
        // the emulator is NTSC, which runs at 60Hz native
        clock.setFrequency(60.f);
        // draw the initial screen
        initalizeScreen();
    }

    /// Handle a new ROM being loaded into the emulator.
    ///
    /// @param sampleRate the sample rate the engine is running at
    ///
    void handleNewROM(int sampleRate) {
        // nothing to do if the UI hasn't set the did insert game flag
        if (!did_insert_game) return;
        // reset the did insert game event signal
        did_insert_game = false;
        // check if an emulator exists, i.e., a game is already loaded
        if (emulator != nullptr) delete emulator;
        // create a new emulator with the specified ROM and reset it
        if (NES::Cartridge::is_valid_rom(rom_path)) {  // ROM file valid
            try {
                emulator = new NES::Emulator(rom_path);
                emulator->set_sample_rate(sampleRate);
                emulator->reset();
            } catch (std::exception e) {  // ROM failed to load
                emulator = nullptr;
                // (TODO: error screen)
                initalizeScreen();
            }
        } else {  // ROM file not valid
            emulator = nullptr;
            // (TODO: error screen)
            initalizeScreen();
        }
    }

    /// Initialize the screen with empty pixels.
    inline void initalizeScreen() {
        memset(screen, 0, NES::Emulator::SCREEN_BYTES);
    }

    /// Copy the screen buffer from the NES in BGR to the local buffer in RGBA.
    inline void copyScreen() {
        memcpy(screen, emulator->get_screen_buffer(), NES::Emulator::SCREEN_BYTES);
    }

    /// Return the clock speed of the NES.
    inline float getClockSpeed() {
        // the range of the clock's value in Hz
        static const float CLOCK_RATE_MIN = 2.f;
        static const float CLOCK_RATE_MAX = 1000.f;
        // get the control voltage scaled in [-2, 2]
        auto cv = inputs[CLOCK_INPUT].getVoltage() / 5.f;
        // apply the attenuverter to the CV signal
        cv *= params[CLOCK_ATT_PARAM].getValue();
        // get the parameter in [-5, 5]
        auto param = params[CLOCK_PARAM].getValue();
        // calculate the exponential frequency
        auto frequency = 60.f * powf(2.f, param + cv);
        // return the clock speed clamped within the legal values
        return rack::clamp(frequency, CLOCK_RATE_MIN, CLOCK_RATE_MAX);
    }

    /// Return the output audio from the NES after applying the volume.
    inline float getAudio() {
        return params[VOLUME_PARAM].getValue() * emulator->get_audio_voltage();
    }

    /// Process a sample.
    void process(const ProcessArgs &args) override {
        // check for a new ROM to load
        handleNewROM(static_cast<int>(args.sampleRate));

        // check for sample rate changes from the engine to send to the NES
        if (new_sample_rate && emulator != nullptr) {
            emulator->set_sample_rate(static_cast<int>(args.sampleRate));
            new_sample_rate = false;
        }

        // NOTE: process the backup, reset, restore in given order to ensure
        // that when all go high on the same frame, the emulator stays in its
        // current state
        // handle inputs to the backup button and CV
        if (backupButton.process(
            params[BACKUP_PARAM].getValue(),
            inputs[BACKUP_INPUT].getVoltage()
        ) && emulator != nullptr) emulator->backup();
        // handle inputs to the reset button and CV
        if (resetButton.process(
            params[RESET_PARAM].getValue(),
            inputs[RESET_INPUT].getVoltage()
        ) && emulator != nullptr) emulator->reset();
        // handle inputs to the restore button and CV
        if (restoreButton.process(
            params[RESTORE_PARAM].getValue(),
            inputs[RESTORE_INPUT].getVoltage()
        ) && emulator != nullptr) emulator->restore();

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

        // process the native clock of the module
        clock.process(args.sampleTime);
        auto clockVoltage = rescale(clock.getVoltage(), 0.1, 2.0f, 0.f, 1.f);
        // run a frame on the emulator if the clock is high
        if (clockTrigger.process(clockVoltage) and emulator != nullptr) {
            // set the frequency of the clock and back-end sound engine
            auto frequency = getClockSpeed();
            clock.setFrequency(frequency);
            emulator->set_frame_rate(frequency);
            // set the controller values
            *emulator->get_controller(0) = player1;
            *emulator->get_controller(1) = player2;
            // run a complete frame through the emulator
            emulator->step();
            // copy the screen to the internal module buffer. this is necessary
            // because the emulator renders in BGR and we need RGBA. It also
            // prevents the UI thread from needing direct access to the emulator
            copyScreen();
        }
        // set the clock output trigger based on the clock signal
        outputs[CLOCK_OUTPUT].setVoltage(10.f * clockTrigger.isHigh());
        // get the sound output from the emulator
        if (emulator != nullptr)
            outputs[SOUND_OUTPUT].setVoltage(getAudio());
    }

    /// Respond to the change of sample rate in the engine.
    void onSampleRateChange() override { new_sample_rate = true; }

    /// Respond to the user resetting the module with the "Initialize" action
    void onReset() override {
        // remove the path to the current ROM
        rom_path = "";
        // deallocate the emulator from RAM
        if (emulator != nullptr) {
            delete emulator;
            emulator = nullptr;
        }
        // reset the screen to an initial state
        initalizeScreen();
    }

    /// Respond to the module being removed from the rack.
    void onRemove() override {
        // deallocate the emulator from RAM
        if (emulator != nullptr) {
            delete emulator;
            emulator = nullptr;
        }
    }

    /// Convert the module's state to a JSON object.
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "rom_path", json_string(rom_path.c_str()));
        return rootJ;
    }

    /// Load the module's state from a JSON object.
    void dataFromJson(json_t* rootJ) override {
        json_t* rom_path_ = json_object_get(rootJ, "rom_path");
        if (rom_path_) {  // a ROM was loaded into the emulator
            rom_path = json_string_value(rom_path_);
            did_insert_game = !rom_path.empty();
        }
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
    RackNESWidget(RackNES *module) {
        setModule(module);
        static const auto panel = "res/RackNES.svg";
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
        static const int x = 112;
        // the y position of the screen
        static const int y = 18;
        // the width of the screen
        static const int w = NES::Emulator::WIDTH;
        // the height of the screen
        static const int h = NES::Emulator::HEIGHT;
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
    }

    /// A menu item for loading ROMs into the emulator.
    struct RomSelectItem : MenuItem {
        /// the module associated with the menu item
        RackNES* module;

        /// Respond to an action on the menu item
        void onAction(const event::Action &e) override {
            // get the path from the OS dialog window (use the directory from
            // last path if it is available)
            std::string dir = module->rom_path.empty() ?
                asset::user("") : rack::string::directory(module->rom_path);
            // filter to only allow files with a ".nes" or ".NES" extension
            osdialog_filters *filters = osdialog_filters_parse("NES ROM:nes,NES");
            char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, filters);
            osdialog_filters_free(filters);
            if (path) {  // a path was selected
                module->rom_path = path;
                module->did_insert_game = true;
                free(path);
            }
        }
    };

    /// Add a content menu to the module widget.
    void appendContextMenu(ui::Menu *menu) override {
        RackNES* module = dynamic_cast<RackNES*>(this->module);
        assert(module);
        // add a ROM selection menu item
        menu->addChild(construct<MenuLabel>());
        menu->addChild(construct<RomSelectItem>(
            &MenuItem::text,
            "Load ROM",
            &RomSelectItem::module,
            module
        ));
    }

};

/// the global instance of the model
Model *modelRackNES = createModel<RackNES, RackNESWidget>("RackNES");

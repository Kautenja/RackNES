// Memory map data from https://datacrystal.romhacking.net
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

#include "plugin.hpp"
#include "GameMaps.hpp"
#include "theme.hpp"

// ---------------------------------------------------------------------------
// MARK: Module
// ---------------------------------------------------------------------------

/// An Expander Module for reading from and writing to RackNES emulator RAM
/// @tparam INPUTS the number of inputs
/// @tparam OUTPUTS the number of outputs
/// @details
/// INPUTS, OUTPUTS only valid as either <0, 8> or <8, 0>
template <int INPUTS, int OUTPUTS>
struct CVGenie : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        ENUMS(INPUT_MEMVAL, INPUTS),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(OUTPUT_MEMVAL, OUTPUTS),
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    /// Container for maps of game-specific memory locations
    GameMap gameMap = GameMap();
    /// The currently selected memory locations
    int memLoc[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

    /// A Schmitt Trigger used when a memory location is marked as a boolean
    /// toggle (i.e., with only two valid values)
    dsp::SchmittTrigger cvTrigger[8];
    /// The current state of selected & toggleable memory locations
    bool toggleState[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    /// Initialize a new CV Genie module
    CVGenie() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    /// Reset module to initialized state
    void onReset() final {
        gameMap.gameId = -1;
        memset(memLoc, -1, sizeof memLoc);
    }

    /// Randomize the memory-location selectors
    void onRandomize() final {
        for (int i = 0; i < 8; i++)
            memLoc[i] = random::uniform() * gameMap.getNumCheats();
    }

    /// Process a sample.
    void process(const ProcessArgs &args) final {
        // check if a RackNES module is immediately to the left of this module
		if (leftExpander.module && leftExpander.module->model == modelRackNES) {
            // check if this is an Input Genie
            if (INPUTS == 8) {
                // get producer message from RackNES in order to fill it and flip it into a consumer message for RackNES
                uint16_t *message = (uint16_t*) leftExpander.module->rightExpander.producerMessage;
                // each of 8 possible messages contains 2 submessages (address and value)
                // so, iterate over all possible submessages
                for (int i = 0; i < 16; i += 2) {
                    // check if the input associated with this message is connected
                    if (inputs[INPUT_MEMVAL + i / 2].isConnected()) {
                        // check if the associated memory location is a toggle
                        if (gameMap.isToggle(memLoc[i / 2])) {  // NOTE: toggle often takes a few triggers to work...
                            // check if a trigger has been received at the input
                            auto cv = inputs[INPUT_MEMVAL + i / 2].getVoltage();
                            if (cvTrigger[i / 2].process(rescale(cv, 0.1, 2, 0, 1))) {
                                // flip the state of the toggle
                                toggleState[i / 2] ^= true;
                                // write the selected memory address to submessage 1
                                message[i] = gameMap.getAddress(memLoc[i / 2]);
                                // write the state of the toggle to submessage 2
                                message[i + 1] = toggleState[i / 2];
                            }
                        }
                        // otherwise, this memory location is not a toggle
                        else {
                            // write the selected memory address to submessage 1
                            message[i] = gameMap.getAddress(memLoc[i / 2]);
                            // get the minimum and maximum values for the selected memory location
                            uint8_t min = gameMap.getMinValue(memLoc[i / 2]);
                            uint8_t max = gameMap.getMaxValue(memLoc[i / 2]);
                            // check if the minimum is less than maximum
                            if (min < max) {
                                // scale the input voltage from 0v-10v to the appropriate two-byte range
                                message[i + 1] = rescale(inputs[INPUT_MEMVAL + i / 2].getVoltage(), 0.f, 10.f, min, max);
                            }
                            else {
                                // flip input voltage to 10v-0v, then scale to the appropriate range
                                message[i + 1] = rescale(10.f - inputs[INPUT_MEMVAL + i / 2].getVoltage(), 0.f, 10.f, max, min);
                            }
                        }
                    }
                    else
                        // input is not connected, so signal to RackNES that nothing should be written
                        message[i] = 0;
                }
            }
            else {
                /// TODO: Output Genie
                /* // get message from RackNES
                uint16_t *message = (uint16_t*) leftExpander.consumerMessage;
                // iterate over all possible messages
                for (int i = 0; i < 8; i++) {
                    // check if the message is empty
                    if (message[i] != -1) {
                        // check if a cable is connected to the output
                        if (outputs[OUTPUT_MEMVAL + i].isConnected()) {
                            // get the minimum and maximum values for the selected memory location
                            uint8_t min = gameMap.getMinValue(memLoc[i / 2]);
                            uint8_t max = gameMap.getMaxValue(memLoc[i / 2]);
                            // check if the minimum is less than maximum
                            if (min < max) {
                                // scale the two-byte value from its valid range to a voltage from 0V-10V
                                outputs[OUTPUT_MEMVAL + i].setVoltage(rescale(message[i], min, max, 0.f, 10.f));
                            }
                            else {
                                // scale the two-byte value from its valid range to a voltage from 0V-10V
                                outputs[OUTPUT_MEMVAL + i].setVoltage(rescale(message[i], max, min, 0.f, 10.f));
                            }
                        }
                    }
                } */
            }
            // flip messages at the end of the timestep
			leftExpander.module->rightExpander.messageFlipRequested = true;
		}
		else {
			// RackNES is not connected.
		}
	}

    /// Convert the module's state to a JSON object.
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "Game", json_integer(gameMap.gameId));
        json_t* memLocationsJ = json_array();
        for (int i = 0; i < 8; i++) {
            json_t* locationJ = json_object();
            json_object_set_new(locationJ, "Location", json_integer(memLoc[i]));
            json_array_append_new(memLocationsJ, locationJ);
        }
        json_object_set_new(rootJ, "Memory Locations", memLocationsJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        gameMap.setGame(static_cast<GameIds>(json_integer_value(json_object_get(rootJ, "Game"))));
        json_t* memLocationsJ = json_object_get(rootJ, "Memory Locations");
        json_t* locationJ; size_t locationIndex;
        json_array_foreach(memLocationsJ, locationIndex, locationJ) {
            memLoc[locationIndex] = json_integer_value(json_object_get(locationJ, "Location"));
        }
    }
};

// ---------------------------------------------------------------------------
// MARK: Widget
// ---------------------------------------------------------------------------

/// A menu item for selecting a memory location
template <class TModule, int SELECTOR_ID>
struct ElementItem : ui::MenuItem {
    /// the module associated with the menu item
    TModule* module;
    /// the ID of the menu item
    int elementId;

    /// Respond to an action on the menu item
    void onAction(const event::Action& e) override {
		module->memLoc[SELECTOR_ID] = elementId;
	}

    /// Associate a module with the menu item
    void setModule(TModule* module) {
        this->module = module;
    }
};

/// An indicator which displays the currently selected memory location
/// On being clicked, spawns a menu containing selectable memory locations
template <class TModule, int SELECTOR_ID>
struct ElementChoice : LedDisplayChoice {
    /// the module associated with the indicator
    TModule* module;

    /// Set the module of the indicator
    void setModule(TModule* module) {
        this->module = module;
    }

    /// Respond to an action on the indicator (open the menu)
    void onAction(const event::Action& e) override {
        /// create the menu
		ui::Menu* menu = createMenu();
        /// add a label to the top of the menu
		menu->addChild(createMenuLabel("Game Element"));
        /// add all available memory locations for the currently selected game
        for (int i = -1; i < (int)module->gameMap.getNumCheats(); i++) {
            /// create a menu item for a memory location
			ElementItem<TModule, SELECTOR_ID>* item = new ElementItem<TModule, SELECTOR_ID>;
            item->setModule(module);
            item->elementId = i;
            /// set the first menu item to "Unassigned", and the rest to their specified names
			item->text = i > -1 ? module->gameMap.getName(i) : "Unassigned";
            /// add a checkmark if an item is previously selected
			item->rightText = CHECKMARK(item->elementId == module->memLoc[SELECTOR_ID]);
            /// add the item to the menu
			menu->addChild(item);
		}
	}
	void step() override {
        /// Set the indicator's text to the specified name of the currently selected memory location
        /// Set to "Unassigned" if no memory location is selected
		text = (module && module->memLoc[SELECTOR_ID] > -1) ? module->gameMap.getName(module->memLoc[SELECTOR_ID]) : "Unassigned";
	}
};

/// An LED display containing an indicator for the currently selected memory location
/// The display's child widget (the indicator) also spawns a selection menu upon being clicked
template <class TModule, int SELECTOR_ID>
struct GenieMemorySelectorWidget : LedDisplay {
    /// the child indicator widget for this display
    ElementChoice<TModule, SELECTOR_ID>* elementChoice;

    /// create the indicator, set its position & size, and set its associated module
    void setModule(TModule* module) {
        if (!module) return;

        Vec pos = Vec(-4, -2);
        elementChoice = createWidget<ElementChoice<TModule, SELECTOR_ID>>(pos);
        elementChoice->box.size = this->box.size;
        elementChoice->setModule(module);
        addChild(elementChoice);
    }
};

/// A menu item for selecting a game
template <class TModule>
struct GameItem : ui::MenuItem {
    /// the module associated with the menu item
    TModule* module;
    /// the ID of the menu item
    GameIds gameId;

    /// Respond to an action on the menu item
    void onAction(const event::Action& e) override {
		module->gameMap.setGame(gameId);
	}

    /// Associate a module with the menu item
    void setModule(TModule* module) {
        this->module = module;
    }
};

/// An indicator which displays the currently selected game
/// On being clicked, spawns a menu containing selectable games
template <class TModule>
struct GameChoice : LedDisplayChoice {
    /// the module associated with the indicator
    TModule* module;

    /// associate a module with the indicator
    void setModule(TModule* module) {
        this->module = module;
    }

    /// Respond to an action on the indicator (open the menu)
    void onAction(const event::Action& e) override {
        /// create the menu
		ui::Menu* menu = createMenu();
        /// add a label to the top of the menu
		menu->addChild(createMenuLabel("Games"));
        /// add all available games to the menu
        for (int i = 0; i < NUM_GAMES; i++) {
            /// create a menu item for a game
			GameItem<TModule>* item = new GameItem<TModule>;
            item->setModule(module);
            item->gameId = static_cast<GameIds>(i);
			item->text = module->gameMap.getGameName(i);
            /// add a checkmark if a game has been previously selected
			item->rightText = CHECKMARK(item->gameId == module->gameMap.gameId);
            /// add the item to the menu
			menu->addChild(item);
		}
	}
	void step() override {
        /// Set the indicator's text to the name of the currently selected game
        /// Set to "No Game Selected" if no game is selected
        /// Set to "CV Genie" if the module has not been created (we are in the module browser or library.vcvrack.com)
		text = module ? (module->gameMap.gameId > -1 ? module->gameMap.getGameName(module->gameMap.gameId) : "No Game Selected") : "CV Genie";
	}
};

/// An LED display containing an indicator for the currently selected game
/// The display's child widget (the indicator) also spawns a selection menu upon being clicked
template <class TModule>
struct GenieGameSelectorWidget : LedDisplay {
    /// the child indicator widget for this display
    GameChoice<TModule>* gameChoice;

    /// create the indicator, set its position & size, and set its associated module
    void setModule(TModule* module) {
        if (!module) return;

        Vec pos = Vec(-4, -2);
        gameChoice = createWidget<GameChoice<TModule>>(pos);
        gameChoice->box.size = this->box.size;
        gameChoice->setModule(module);
        addChild(gameChoice);
    }
};

/// The basename for the RackNES panel files.
const char BASENAME[] = "res/CVGenie";

/// The widget structure that lays out the panel of an Input Genie and the UI menus.
struct InputGenieWidget : ThemedWidget<BASENAME> {
    typedef CVGenie<8, 0> TInputGenie;

    /// Create a new Input Genie widget for the given Input Genie module.
    ///
    /// @param module the module to create a widget for
    ///
    explicit InputGenieWidget(TInputGenie* module) {
        setModule(module);
        // panel screws
        addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));
        // game selector
        GenieGameSelectorWidget<TInputGenie>* gameSelectorWidget = createWidget<GenieGameSelectorWidget<TInputGenie>>(Vec(9.507, 22));
        gameSelectorWidget->box.size = mm2px(Vec(55.f, 7.809f));
        gameSelectorWidget->setModule(module);
        addChild(gameSelectorWidget);
        // memory location selectors
        GenieMemorySelectorWidget<TInputGenie, 0>* memSelectorWidget0 = createWidget<GenieMemorySelectorWidget<TInputGenie, 0>>(Vec(66.5895, 58.397));
        memSelectorWidget0->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget0->setModule(module);
        addChild(memSelectorWidget0);
        GenieMemorySelectorWidget<TInputGenie, 1>* memSelectorWidget1 = createWidget<GenieMemorySelectorWidget<TInputGenie, 1>>(Vec(66.5895, 96.842));
        memSelectorWidget1->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget1->setModule(module);
        addChild(memSelectorWidget1);
        GenieMemorySelectorWidget<TInputGenie, 2>* memSelectorWidget2 = createWidget<GenieMemorySelectorWidget<TInputGenie, 2>>(Vec(66.5895, 135.283));
        memSelectorWidget2->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget2->setModule(module);
        addChild(memSelectorWidget2);
        GenieMemorySelectorWidget<TInputGenie, 3>* memSelectorWidget3 = createWidget<GenieMemorySelectorWidget<TInputGenie, 3>>(Vec(66.5895, 173.728));
        memSelectorWidget3->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget3->setModule(module);
        addChild(memSelectorWidget3);
        GenieMemorySelectorWidget<TInputGenie, 4>* memSelectorWidget4 = createWidget<GenieMemorySelectorWidget<TInputGenie, 4>>(Vec(66.5895, 212.173));
        memSelectorWidget4->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget4->setModule(module);
        addChild(memSelectorWidget4);
        GenieMemorySelectorWidget<TInputGenie, 5>* memSelectorWidget5 = createWidget<GenieMemorySelectorWidget<TInputGenie, 5>>(Vec(66.5895, 250.614));
        memSelectorWidget5->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget5->setModule(module);
        addChild(memSelectorWidget5);
        GenieMemorySelectorWidget<TInputGenie, 6>* memSelectorWidget6 = createWidget<GenieMemorySelectorWidget<TInputGenie, 6>>(Vec(66.5895, 289.059));
        memSelectorWidget6->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget6->setModule(module);
        addChild(memSelectorWidget6);
        GenieMemorySelectorWidget<TInputGenie, 7>* memSelectorWidget7 = createWidget<GenieMemorySelectorWidget<TInputGenie, 7>>(Vec(66.5895, 327.504));
        memSelectorWidget7->box.size = mm2px(Vec(34.f, 7.809f));
        memSelectorWidget7->setModule(module);
        addChild(memSelectorWidget7);
        // inputs
        addInput(createInput<PJ301MPort>(Vec(14.007, 57.397), module, TInputGenie::INPUT_MEMVAL + 0));
        addInput(createInput<PJ301MPort>(Vec(14.007, 96.842), module, TInputGenie::INPUT_MEMVAL + 1));
        addInput(createInput<PJ301MPort>(Vec(14.007, 135.283), module, TInputGenie::INPUT_MEMVAL + 2));
        addInput(createInput<PJ301MPort>(Vec(14.007, 173.728), module, TInputGenie::INPUT_MEMVAL + 3));
        addInput(createInput<PJ301MPort>(Vec(14.007, 212.173), module, TInputGenie::INPUT_MEMVAL + 4));
        addInput(createInput<PJ301MPort>(Vec(14.007, 250.614), module, TInputGenie::INPUT_MEMVAL + 5));
        addInput(createInput<PJ301MPort>(Vec(14.007, 289.059), module, TInputGenie::INPUT_MEMVAL + 6));
        addInput(createInput<PJ301MPort>(Vec(14.007, 327.504), module, TInputGenie::INPUT_MEMVAL + 7));
    }
};

Model* modelInputGenie = createModel<CVGenie<8, 0>, InputGenieWidget>("InputGenie");

/// The widget structure that lays out the panel of an Output Genie and the UI menus.
struct OutputGenieWidget : ThemedWidget<BASENAME> {
    typedef CVGenie<0, 8> TOutputGenie;

    /// Create a new Output Genie widget for the given Output Genie module.
    ///
    /// @param module the module to create a widget for
    ///
    explicit OutputGenieWidget(TOutputGenie* module) {
        setModule(module);
        // panel screws
        addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));
        // game selector
        GenieGameSelectorWidget<TOutputGenie>* gameSelectorWidget = createWidget<GenieGameSelectorWidget<TOutputGenie>>(Vec(9.507, 13));
        gameSelectorWidget->box.size = mm2px(Vec(55.f, 7.809f));
        gameSelectorWidget->setModule(module);
        addChild(gameSelectorWidget);
        // memory location selectors
        GenieMemorySelectorWidget<TOutputGenie, 0>* memSelectorWidget0 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 0>>(Vec(14.007, 58.397));
        memSelectorWidget0->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget0->setModule(module);
        addChild(memSelectorWidget0);
        GenieMemorySelectorWidget<TOutputGenie, 1>* memSelectorWidget1 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 1>>(Vec(14.007, 96.842));
        memSelectorWidget1->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget1->setModule(module);
        addChild(memSelectorWidget1);
        GenieMemorySelectorWidget<TOutputGenie, 2>* memSelectorWidget2 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 2>>(Vec(14.007, 135.283));
        memSelectorWidget2->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget2->setModule(module);
        addChild(memSelectorWidget2);
        GenieMemorySelectorWidget<TOutputGenie, 3>* memSelectorWidget3 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 3>>(Vec(14.007, 173.728));
        memSelectorWidget3->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget3->setModule(module);
        addChild(memSelectorWidget3);
        GenieMemorySelectorWidget<TOutputGenie, 4>* memSelectorWidget4 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 4>>(Vec(14.007, 212.173));
        memSelectorWidget4->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget4->setModule(module);
        addChild(memSelectorWidget4);
        GenieMemorySelectorWidget<TOutputGenie, 5>* memSelectorWidget5 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 5>>(Vec(14.007, 250.614));
        memSelectorWidget5->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget5->setModule(module);
        addChild(memSelectorWidget5);
        GenieMemorySelectorWidget<TOutputGenie, 6>* memSelectorWidget6 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 6>>(Vec(14.007, 289.059));
        memSelectorWidget6->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget6->setModule(module);
        addChild(memSelectorWidget6);
        GenieMemorySelectorWidget<TOutputGenie, 7>* memSelectorWidget7 = createWidget<GenieMemorySelectorWidget<TOutputGenie, 7>>(Vec(14.007, 327.504));
        memSelectorWidget7->box.size = mm2px(Vec(33.f, 7.809f));
        memSelectorWidget7->setModule(module);
        addChild(memSelectorWidget7);
        // outputs
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 58.397), module, TOutputGenie::OUTPUT_MEMVAL + 0));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 96.842), module, TOutputGenie::OUTPUT_MEMVAL + 1));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 135.283), module, TOutputGenie::OUTPUT_MEMVAL + 2));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 173.728), module, TOutputGenie::OUTPUT_MEMVAL + 3));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 212.173), module, TOutputGenie::OUTPUT_MEMVAL + 4));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 250.614), module, TOutputGenie::OUTPUT_MEMVAL + 5));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 289.059), module, TOutputGenie::OUTPUT_MEMVAL + 6));
        addOutput(createOutput<PJ301MPort>(Vec(129.5895, 327.504), module, TOutputGenie::OUTPUT_MEMVAL + 7));
    }
};

Model* modelOutputGenie = createModel<CVGenie<0, 8>, OutputGenieWidget>("OutputGenie");

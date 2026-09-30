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

#include <atomic>
#include <cmath>
#include <string>

#include "plugin.hpp"
#include "GameMaps.hpp"
#include "theme.hpp"

/// Describe byte endpoints in their mapped order, including descending toggles.
static std::string elementDescription(const GameParameter* parameter) {
    if (!parameter) return "Unassigned: no memory writes";
    return parameter->toggle
        ? string::f("Trigger toggle: %d / %d", parameter->minimum, parameter->maximum)
        : string::f("Continuous 0-10 V: %d to %d", parameter->minimum, parameter->maximum);
}

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
    GameMap gameMap;
    /// Engine-owned selections, published atomically for UI labels and menus.
    std::atomic<int> memLoc[8];

    /// A Schmitt Trigger used when a memory location is marked as a boolean
    /// toggle (i.e., with only two valid values)
    dsp::SchmittTrigger cvTrigger[8];
    /// Engine-owned choice of the first (false) or second (true) endpoint.
    bool toggleState[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    /// A row menu request carries its game so stale menus cannot change a new map.
    struct SelectionRequest {
        int game;
        int element;
    };

    /// UI requests are applied on the engine thread; -2 means no pending change.
    std::atomic<int> requestedGame{-2};
    std::atomic<SelectionRequest> requestedElement[8];

    /// Build hover text on the UI thread from published selections and static maps.
    std::string rowDescription(int row) const {
        const auto* parameter = gameMap.getParameter(memLoc[row].load());
        if (!parameter) return elementDescription(nullptr);
        std::string text = parameter->name + "\n" + elementDescription(parameter);
        if (parameter->toggle)
            text += "\nTrigger at 2 V; rearm at 0.1 V or below.";
        return text;
    }

    /// Rack requests port descriptions on hover; no strings change in process().
    struct RowPortInfo : engine::PortInfo {
        std::string getDescription() override {
            return static_cast<CVGenie*>(module)->rowDescription(portId);
        }
    };

    /// Initialize a new CV Genie module.
    CVGenie() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int row = 0; row < INPUTS; row++)
            configInput<RowPortInfo>(INPUT_MEMVAL + row, string::f("Row %d CV", row + 1));
        onReset();
    }

    /// @brief Clear row assignments and their engine-owned trigger state.
    void clearRows() {
        for (int row = 0; row < 8; row++) {
            memLoc[row].store(-1);
            toggleState[row] = false;
            cvTrigger[row].reset();
        }
    }

    /// @brief Apply a game change on the engine thread, clearing stale rows.
    void selectGame(int id) {
        const int nextGame = GameMap::isValidGame(id) ? id : -1;
        if (gameMap.gameId.load() == nextGame) return;
        clearRows();
        gameMap.setGame(nextGame);
    }

    /// @brief Apply a row selection on the engine thread and reset its toggle.
    void selectElement(int row, int id) {
        if (row < 0 || row >= 8) return;
        const int nextElement = gameMap.getParameter(id) ? id : -1;
        if (memLoc[row].load() == nextElement) return;
        memLoc[row].store(nextElement);
        toggleState[row] = false;
        cvTrigger[row].reset();
    }

    /// @brief Apply bounded UI requests before building any expander messages.
    void processSelections() {
        const int game = requestedGame.exchange(-2);
        if (game != -2) selectGame(game);
        for (int row = 0; row < 8; row++) {
            const auto request = requestedElement[row].exchange({-1, -2});
            if (request.element != -2 && request.game == gameMap.gameId.load())
                selectElement(row, request.element);
        }
    }

    /// Reset module to initialized state.
    void onReset() final {
        gameMap.setGame(-1);
        clearRows();
        requestedGame.store(-2);
        for (auto& request : requestedElement) request.store({-1, -2});
    }

    /// Randomize selections only when a game has available entries.
    void onRandomize() final {
        processSelections();
        const unsigned count = gameMap.getNumCheats();
        if (count == 0) return;
        clearRows();
        for (int row = 0; row < 8; row++)
            selectElement(row, static_cast<int>(random::uniform() * count));
    }

    /// Process a sample, emitting at most one address/value pair per row.
    void process(const ProcessArgs& args) final {
        processSelections();
        if (INPUTS != 8 || !leftExpander.module ||
            leftExpander.module->model != modelRackNES) return;
        auto* message = static_cast<uint16_t*>(leftExpander.module->rightExpander.producerMessage);
        if (!message) return;
        for (int row = 0; row < 8; row++) {
            // Never replay a previous write when this sample has no event.
            message[2 * row] = 0;
            message[2 * row + 1] = 0;
            const auto* parameter = gameMap.getParameter(memLoc[row].load());
            const float voltage = inputs[INPUT_MEMVAL + row].getVoltage();
            if (!parameter || !inputs[INPUT_MEMVAL + row].isConnected() ||
                !std::isfinite(voltage)) {
                cvTrigger[row].reset();
                continue;
            }
            uint8_t value;
            if (parameter->toggle) {
                if (!cvTrigger[row].process(rescale(voltage, 0.1f, 2.f, 0.f, 1.f)))
                    continue;
                toggleState[row] = !toggleState[row];
                value = toggleState[row] ? parameter->maximum : parameter->minimum;
            } else {
                const float normalized = clamp(voltage, 0.f, 10.f) / 10.f;
                value = static_cast<uint8_t>(rescale(normalized, 0.f, 1.f,
                    parameter->minimum, parameter->maximum));
            }
            message[2 * row] = parameter->address;
            message[2 * row + 1] = value;
        }
        leftExpander.module->rightExpander.messageFlipRequested = true;
    }

    /// Convert the module's state to a JSON object.
    json_t* dataToJson() override {
        processSelections();
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "Game", json_integer(gameMap.gameId.load()));
        json_t* memLocationsJ = json_array();
        for (int i = 0; i < 8; i++) {
            json_t* locationJ = json_object();
            json_object_set_new(locationJ, "Location", json_integer(memLoc[i].load()));
            json_object_set_new(locationJ, "Toggle State", json_boolean(toggleState[i]));
            json_array_append_new(memLocationsJ, locationJ);
        }
        json_object_set_new(rootJ, "Memory Locations", memLocationsJ);
        return rootJ;
    }

    /// Restore at most eight valid rows; legacy patches default toggles to false.
    void dataFromJson(json_t* rootJ) override {
        onReset();
        json_t* gameJ = json_object_get(rootJ, "Game");
        if (!json_is_integer(gameJ)) return;
        const json_int_t game = json_integer_value(gameJ);
        if (game < 0 || game >= NUM_GAMES) return;
        selectGame(static_cast<int>(game));
        json_t* locationsJ = json_object_get(rootJ, "Memory Locations");
        for (int row = 0; row < 8; row++) {
            json_t* locationJ = json_array_get(locationsJ, row);
            json_t* indexJ = json_object_get(locationJ, "Location");
            if (!json_is_integer(indexJ)) continue;
            const json_int_t index = json_integer_value(indexJ);
            if (index < 0 || index >= gameMap.getNumCheats()) continue;
            selectElement(row, static_cast<int>(index));
            toggleState[row] = json_is_true(json_object_get(locationJ, "Toggle State"));
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
    /// The map shown when this menu was opened.
    int gameId = -1;

    /// Respond to an action on the menu item
    void onAction(const event::Action& e) override {
		module->requestedElement[SELECTOR_ID].store({gameId, elementId});
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
    TModule* module = nullptr;

    /// Scene-owned hover text, detached when the selector leaves or is removed.
    struct RowTooltip : ui::Tooltip {
        ElementChoice* choice = nullptr;
        ~RowTooltip() override { choice->tooltip = nullptr; }
        void step() override {
            text = choice->module->rowDescription(SELECTOR_ID);
            Tooltip::step();
        }
    };
    RowTooltip* tooltip = nullptr;

    ~ElementChoice() override { destroyTooltip(); }

    /// Remove hover help before opening a menu or deleting this selector.
    void destroyTooltip() {
        if (!tooltip) return;
        if (tooltip->parent) tooltip->parent->removeChild(tooltip);
        delete tooltip;
        tooltip = nullptr;
    }

    void onEnter(const EnterEvent& e) override {
        if (!module || !settings::tooltips || tooltip) return;
        tooltip = new RowTooltip;
        tooltip->choice = this;
        APP->scene->addChild(tooltip);
    }

    void onLeave(const LeaveEvent& e) override { destroyTooltip(); }

    /// Set the module of the indicator
    void setModule(TModule* module) {
        this->module = module;
    }

    /// Respond to an action on the indicator (open the menu)
    void onAction(const event::Action& e) override {
        if (!module) return;
        destroyTooltip();
        /// create the menu
		ui::Menu* menu = createMenu();
        /// add a label to the top of the menu
		menu->addChild(createMenuLabel("Game Element"));
        // Keep labels and requests tied to the map shown when this menu opens.
        const int gameId = module->gameMap.gameId.load();
        GameMap menuMap;
        menuMap.setGame(gameId);
        for (int i = -1; i < static_cast<int>(menuMap.getNumCheats()); i++) {
            /// create a menu item for a memory location
			ElementItem<TModule, SELECTOR_ID>* item = new ElementItem<TModule, SELECTOR_ID>;
            item->setModule(module);
            item->elementId = i;
            item->gameId = gameId;
            /// set the first menu item to "Unassigned", and the rest to their specified names
			item->text = menuMap.getName(i);
            /// add a checkmark if an item is previously selected
            const auto* parameter = menuMap.getParameter(i);
            item->rightText = parameter ? elementDescription(parameter) : "";
            if (item->elementId == module->memLoc[SELECTOR_ID].load())
                item->rightText += " " + std::string(CHECKMARK_STRING);
            /// add the item to the menu
			menu->addChild(item);
		}
	}
    void step() override {
        /// Set the indicator's text to the specified name of the currently selected memory location
        /// Set to "Unassigned" if no memory location is selected
        const auto* parameter = module
            ? module->gameMap.getParameter(module->memLoc[SELECTOR_ID].load()) : nullptr;
        text = parameter ? parameter->name : "Unassigned";
        LedDisplayChoice::step();
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
		module->requestedGame.store(gameId);
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
		text = module ? (module->gameMap.gameId.load() > -1 ? module->gameMap.getGameName(module->gameMap.gameId.load()) : "No Game Selected") : "CV Genie";
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

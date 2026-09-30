// Focused regression checks against the actual CV Genie module and Rack SDK.
#include <cassert>
#include <cstdio>
#include <limits>

#include "../src/CVGenie.cpp"

Plugin* plugin_instance = nullptr;
static Model racknesModel;
Model* modelRackNES = &racknesModel;
using Genie = CVGenie<8, 0>;

/// A real Rack Module neighbor with the same two-buffer expander contract.
struct Fixture {
    Genie genie;
    Module parent;
    uint16_t messages[2][16] = {};
    Module::ProcessArgs args = {};

    Fixture() {
        parent.model = modelRackNES;
        parent.rightExpander.producerMessage = messages[0];
        parent.rightExpander.consumerMessage = messages[1];
        genie.leftExpander.module = &parent;
        assert(genie.requestedElement[0].is_lock_free());
        args.sampleRate = 48000.f;
        args.sampleTime = 1.f / args.sampleRate;
    }

    void connect(int row, float voltage) {
        // Rack's engine, rather than Port::setChannels(), connects a port.
        genie.inputs[row].channels = 1;
        genie.inputs[row].setVoltage(voltage);
    }

    uint16_t* process() {
        genie.process(args);
        return static_cast<uint16_t*>(parent.rightExpander.producerMessage);
    }

    void flip() {
        std::swap(parent.rightExpander.producerMessage,
                  parent.rightExpander.consumerMessage);
    }
};

void check_unassigned_and_invalid_selections() {
    Fixture f;
    f.connect(0, 5.f);
    assert(f.process()[0] == 0);
    assert(f.genie.gameMap.getNumCheats() == 0);
    assert(f.genie.gameMap.getName(-1) == "Unassigned");
    assert(f.genie.gameMap.getGameName(999) == "No Game Selected");
    f.genie.onRandomize();
    for (int row = 0; row < 8; row++) assert(f.genie.memLoc[row] == -1);
    for (int game : {-1, static_cast<int>(NUM_GAMES), 999}) {
        f.genie.gameMap.setGame(game);
        for (int element : {-1, 53, 999}) {
            assert(f.genie.gameMap.getAddress(element) == 0);
            assert(f.genie.gameMap.getMinValue(element) == 0);
            assert(f.genie.gameMap.getMaxValue(element) == 0);
            assert(!f.genie.gameMap.isToggle(element));
        }
    }
    f.genie.selectGame(PLUMBER);
    f.genie.memLoc[0] = 53;
    assert(f.process()[0] == 0);
    f.genie.memLoc[0] = -1;
    assert(f.process()[0] == 0);
}

void check_continuous_voltage() {
    Fixture f;
    f.genie.selectGame(PLUMBER);
    f.genie.selectElement(0, 5);  // 0--255 horizontal level position.
    const float voltages[] = {-20.f, 0.f, 5.f, 10.f, 20.f};
    const uint16_t bytes[] = {0, 0, 127, 255, 255};
    for (int i = 0; i < 5; i++) {
        f.connect(0, voltages[i]);
        const auto* message = f.process();
        assert(message[0] == 0x006D && message[1] == bytes[i]);
    }
    for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                          std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity()}) {
        f.connect(0, invalid);
        assert(f.process()[0] == 0);
    }
    f.genie.selectGame(TUNIC);
    f.genie.selectElement(0, 54);  // Nonzero lower endpoint, 8--255.
    f.connect(0, -10.f);
    assert(f.process()[1] == 8);
    f.connect(0, 5.f);
    assert(f.process()[1] == 131);
    f.connect(0, 20.f);
    assert(f.process()[1] == 255);
    f.genie.inputs[0].channels = 0;
    assert(f.process()[0] == 0);
}

void check_toggles_and_message_lifetime() {
    // Includes descending endpoints and a non-boolean maximum.
    const int games[] = {PLUMBER, TUNIC, TUNIC};
    const int elements[] = {14, 23, 33};
    const uint16_t low[] = {0, 128, 0};
    const uint16_t high[] = {46, 1, 2};
    for (int i = 0; i < 3; i++) {
        Fixture f;
        f.genie.selectGame(games[i]);
        f.genie.selectElement(0, elements[i]);
        f.connect(0, 0.f);
        assert(f.process()[0] == 0);
        f.connect(0, 2.f);
        assert(f.process()[1] == high[i]);
        // Deliberately leave old data in the other buffer: no repeated writes.
        f.flip();
        assert(f.process()[0] == 0);
        f.flip();
        assert(f.process()[0] == 0);
        f.connect(0, 1.f);  // Hysteresis: this does not re-arm the gate.
        assert(f.process()[0] == 0);
        f.connect(0, 5.f);
        assert(f.process()[0] == 0);
        f.connect(0, 0.f);
        assert(f.process()[0] == 0);
        f.connect(0, 5.f);
        assert(f.process()[1] == low[i]);
    }
}

void check_selection_handoff() {
    Fixture f;
    f.genie.selectGame(TUNIC);
    f.genie.selectElement(0, 54);
    f.connect(0, 5.f);
    GameItem<Genie> game;
    game.setModule(&f.genie);
    game.gameId = PLUMBER;
    game.onAction(event::Action());
    assert(f.genie.gameMap.gameId == TUNIC);  // UI only queues a change.
    assert(f.process()[0] == 0);
    assert(f.genie.gameMap.gameId == PLUMBER && f.genie.memLoc[0] == -1);
    ElementItem<Genie, 0> element;
    element.setModule(&f.genie);
    element.gameId = TUNIC;  // Menu was opened before the game changed.
    element.elementId = 5;
    element.onAction(event::Action());
    assert(f.process()[0] == 0 && f.genie.memLoc[0] == -1);
    element.gameId = PLUMBER;
    element.onAction(event::Action());
    assert(f.process()[0] == 0x006D);
    f.genie.onRandomize();
    for (int row = 0; row < 8; row++) {
        assert(f.genie.gameMap.getParameter(f.genie.memLoc[row]));
        assert(!f.genie.toggleState[row]);
    }
    f.genie.onReset();
    assert(f.process()[0] == 0 && f.genie.gameMap.gameId == -1);
    // Selecting another entry resets its toggle without a new module instance.
    f.genie.selectGame(PLUMBER);
    f.genie.selectElement(0, 14);
    f.connect(0, 0.f);
    f.process();
    f.connect(0, 5.f);
    assert(f.process()[1] == 46);
    f.genie.selectElement(0, 10);
    assert(!f.genie.toggleState[0]);
    f.connect(0, 0.f);
    f.process();
    f.connect(0, 5.f);
    assert(f.process()[1] == 1);
}

void check_patch_compatibility() {
    // Load the real legacy fixture's Genie data without loading its ROM/audio UI.
    json_error_t error;
    json_t* patch = json_load_file("../patches/debugCVGenie.vcv", 0, &error);
    assert(patch);
    json_t* modules = json_object_get(patch, "modules");
    bool foundGenie = false;
    size_t moduleIndex;
    json_t* moduleJ;
    json_array_foreach(modules, moduleIndex, moduleJ) {
        const char* model = json_string_value(json_object_get(moduleJ, "model"));
        if (!model || std::string(model) != "InputGenie") continue;
        Fixture legacy;
        legacy.genie.dataFromJson(json_object_get(moduleJ, "data"));
        assert(legacy.genie.gameMap.gameId == PLUMBER);
        assert(legacy.genie.memLoc[0] == 6);
        for (int row = 1; row < 8; row++) assert(legacy.genie.memLoc[row] == -1);
        legacy.connect(0, 5.f);
        assert(legacy.process()[0] == 0x0086 && legacy.process()[1] == 127);
        foundGenie = true;
    }
    assert(foundGenie);
    json_decref(patch);
    Fixture f;
    f.genie.selectGame(PLUMBER);
    f.genie.selectElement(0, 14);
    f.connect(0, 0.f);
    f.process();
    f.connect(0, 5.f);
    f.process();
    json_t* saved = f.genie.dataToJson();
    Fixture restored;
    restored.genie.dataFromJson(saved);
    assert(restored.genie.gameMap.gameId == PLUMBER);
    assert(restored.genie.memLoc[0] == 14 && restored.genie.toggleState[0]);
    restored.connect(0, 0.f);
    restored.process();
    restored.connect(0, 5.f);
    assert(restored.process()[1] == 0);  // Continue the saved toggle sequence.
    json_t* locations = json_object_get(saved, "Memory Locations");
    json_object_del(json_array_get(locations, 0), "Toggle State");
    restored.genie.dataFromJson(saved);
    assert(restored.genie.memLoc[0] == 14 && !restored.genie.toggleState[0]);
    // Oversized arrays, invalid indices/types, missing fields, and huge IDs.
    for (int i = 0; i < 12; i++) json_array_append_new(locations, json_object());
    json_object_set_new(json_array_get(locations, 0), "Location", json_integer(999999999999LL));
    json_object_set_new(json_array_get(locations, 1), "Location", json_string("5"));
    restored.genie.dataFromJson(saved);
    assert(restored.genie.memLoc[0] == -1 && restored.genie.memLoc[1] == -1);
    json_object_set_new(saved, "Game", json_integer(999999999999LL));
    restored.genie.dataFromJson(saved);
    assert(restored.genie.gameMap.gameId == -1);
    json_object_del(saved, "Game");
    restored.genie.dataFromJson(saved);
    assert(restored.genie.gameMap.gameId == -1);
    json_decref(saved);
}

void check_enemy_headings() {
    GameMap map;
    map.setGame(PLUMBER);
    for (int slot = 0; slot < 5; slot++) {
        assert(map.getAddress(27 + slot) == 0x0046 + slot);
        assert(map.getAddress(27 + slot) != map.getAddress(26));
    }
}

/// Exercise every catalog entry through real selection, CV, and JSON seams.
void check_catalog() {
    static_assert(PLUMBER == 0 && TUNIC == 1, "Legacy game IDs must not change");
    const char* expected_names[] = {
        "Super Mario Bros.", "The Legend of Zelda", "Mega Man", "Mega Man 2",
        "Castlevania", "Castlevania II: Simon's Quest", "Contra", "Metroid",
        "Ninja Gaiden", "Tetris (Nintendo)"
    };
    assert(NUM_GAMES == 10);
    assert(PARAMETER_COUNTS[PLUMBER] == 53 && PARAMETER_COUNTS[TUNIC] == 55);
    Fixture menu_fixture;
    menu_fixture.genie.selectGame(TUNIC);
    GameChoice<Genie> choice;
    choice.setModule(&menu_fixture.genie);
    ui::Menu menu;
    choice.appendGameItems(&menu);
    unsigned menu_game = 0;
    for (auto* child : menu.children) {
        auto* item = dynamic_cast<GameItem<Genie>*>(child);
        if (!item) continue;  // Skip the menu label.
        assert(menu_game < NUM_GAMES);
        assert(item->gameId == menu_game);
        assert(item->text == expected_names[menu_game]);
        assert(item->rightText == CHECKMARK(menu_game == TUNIC));
        item->onAction(event::Action());
        menu_fixture.process();
        assert(menu_fixture.genie.gameMap.gameId == static_cast<int>(menu_game));
        menu_game++;
    }
    assert(menu_game == NUM_GAMES);
    for (int game = 0; game < NUM_GAMES; game++) {
        Fixture f;
        GameItem<Genie> item;
        item.setModule(&f.genie);
        item.gameId = static_cast<GameIds>(game);
        item.onAction(event::Action());
        f.process();
        assert(f.genie.gameMap.gameId == game);
        assert(f.genie.gameMap.getGameName(game) == expected_names[game]);
        const unsigned count = f.genie.gameMap.getNumCheats();
        assert(count > 0);
        assert(!f.genie.gameMap.getParameter(count));
        for (unsigned index = 0; index < count; index++) {
            const auto* parameter = f.genie.gameMap.getParameter(index);
            assert(parameter && !parameter->name.empty());
            assert(parameter->address > 0 && parameter->address < 0x0800);
            if (game > TUNIC) {
                assert(parameter->minimum < parameter->maximum);
                for (unsigned other = 0; other < index; other++) {
                    const auto* prior = f.genie.gameMap.getParameter(other);
                    assert(prior->address != parameter->address);
                    assert(prior->name != parameter->name);
                }
            }
            f.genie.selectElement(0, index);
            f.connect(0, 0.f);
            auto* message = f.process();
            if (parameter->toggle) {
                assert(message[0] == 0);
                f.connect(0, 5.f);
                message = f.process();
                assert(message[0] == parameter->address);
                assert(message[1] == parameter->maximum);
                assert(f.process()[0] == 0);
                f.connect(0, 0.f);
                f.process();
                f.connect(0, 5.f);
                message = f.process();
                assert(message[0] == parameter->address);
                assert(message[1] == parameter->minimum);
            } else {
                assert(message[0] == parameter->address);
                assert(message[1] == parameter->minimum);
                f.connect(0, 10.f);
                message = f.process();
                assert(message[0] == parameter->address);
                assert(message[1] == parameter->maximum);
            }
            json_t* saved = f.genie.dataToJson();
            Fixture restored;
            restored.genie.dataFromJson(saved);
            json_decref(saved);
            assert(restored.genie.gameMap.gameId == game);
            assert(restored.genie.memLoc[0] == static_cast<int>(index));
            assert(restored.genie.gameMap.getAddress(restored.genie.memLoc[0]) ==
                   parameter->address);
            restored.connect(0, parameter->toggle ? 0.f : 10.f);
            auto* restored_message = restored.process();
            if (parameter->toggle) {
                assert(restored_message[0] == 0);
                restored.connect(0, 5.f);
                restored_message = restored.process();
            }
            assert(restored_message[0] == parameter->address);
            assert(restored_message[1] == parameter->maximum);
            f.genie.inputs[0].channels = 0;
            assert(f.process()[0] == 0);
            f.flip();
            assert(f.process()[0] == 0);
        }
        f.genie.onRandomize();
        for (int row = 0; row < 8; row++)
            assert(f.genie.gameMap.getParameter(f.genie.memLoc[row]));
    }
    // Independently reviewed anchors catch map/count/pointer mismatches.
    assert(games[MEGA_MAN][0].address == 0x006A);
    assert(games[MEGA_MAN][0].maximum == 28);
    assert(games[MEGA_MAN_2][3].address == 0x00A7);
    assert(games[MEGA_MAN_2][3].maximum == 4);
    assert(games[CASTLEVANIA][2].minimum == 1);
    assert(games[CASTLEVANIA_2][3].address == 0x0420);
    assert(games[CONTRA][8].address == 0x00AB);
    assert(games[CONTRA][8].maximum == 4);
    assert(games[METROID][2].address == 0x010E);
    assert(games[METROID][2].toggle);
    assert(games[NINJA_GAIDEN][2].maximum == 16);
    assert(games[TETRIS][1].address == 0x0041);
    assert(games[TETRIS][1].maximum == 19);
}

int main() {
    // Rack's menu child cleanup uses the context's event state.
    Context context;
    context.event = new widget::EventState;
    contextSet(&context);
    check_unassigned_and_invalid_selections();
    check_continuous_voltage();
    check_toggles_and_message_lifetime();
    check_selection_handoff();
    check_patch_compatibility();
    check_enemy_headings();
    check_catalog();
    std::puts("CV Genie: selection, voltage, toggle, expander, and patch checks passed");
}

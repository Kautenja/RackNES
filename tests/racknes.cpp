// Focused snapshot ownership, failed-load, and expander-boundary regressions.
#include <cassert>
#include <cstdio>
#include <memory>

#include "../src/RackNES.cpp"

Plugin* plugin_instance = nullptr;
static Model inputGenieModel;
Model* modelInputGenie = &inputGenieModel;

int main() {
    Context context;
    context.engine = new engine::Engine;
    contextSet(&context);
    {
        std::unique_ptr<RackNES> module(new RackNES);
        // An empty module must serialize without touching uninitialized hardware.
        json_t* initial = module->dataToJson();
        assert(json_object_size(json_object_get(initial, "emulator")) == 0);
        json_decref(initial);
        // Hold one independent JSON reference to observe proper release.
        json_t* saved = json_object();
        json_object_set_new(saved, "payload", json_string("snapshot"));
        module->backup = json_incref(saved);
        module->onReset();
        assert(saved->refcount == 1 && module->backup == nullptr);
        module->backup = json_incref(saved);
        json_t* empty = json_object();
        module->dataFromJson(empty);
        assert(saved->refcount == 1 && module->backup == nullptr);
        json_decref(empty);
        module->backup = json_incref(saved);
        module->screen[0] = 73;
        module->screen[NES::Emulator::SCREEN_BYTES - 1] = 29;
        module->rom_path_signal = "/nonexistent-racknes-regression-fixture.nes";
        module->handleNewROM();
        assert(module->rom_load_failed_signal);
        assert(module->screen[0] == 73);
        assert(module->screen[NES::Emulator::SCREEN_BYTES - 1] == 29);
        assert(module->backup == saved && saved->refcount == 2);
        // SAVE replaces and correctly releases the old JSON snapshot.
        module->processCV();
        module->params[RackNES::PARAM_SAVE].setValue(1.f);
        module->processCV();
        assert(saved->refcount == 1 && module->backup != saved);
        json_t* latest = json_incref(module->backup);
        Module genie;
        genie.model = modelInputGenie;
        module->rightExpander.module = &genie;
        auto* message = static_cast<uint16_t*>(module->rightExpander.consumerMessage);
        auto* ram = module->emulator.get_memory_buffer();
        message[0] = 0x0046;
        message[1] = 2;
        message[2] = 0x07FF;
        message[3] = 255;
        message[4] = 0x0800;
        message[5] = 7;
        message[6] = 0xFFFF;
        message[7] = 7;
        message[8] = 0x0046;
        message[9] = 1;  // A later row wins when addresses coincide.
        module->processExpanders();
        assert(ram[0x46] == 1 && ram[0x7FF] == 255);
        for (int row = 0; row < 8; row++) assert(message[2 * row] == 0);
        module.reset();
        assert(latest->refcount == 1);
        json_decref(latest);
        json_decref(saved);
    }
    std::puts("RackNES: snapshot ownership, failed ROM display, and RAM bounds passed");
}

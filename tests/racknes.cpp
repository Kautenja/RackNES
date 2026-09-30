// Focused snapshot, cartridge header, controller, and expander regressions.
#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

#include "../src/RackNES.cpp"

Plugin* plugin_instance = nullptr;
static Model inputGenieModel;
Model* modelInputGenie = &inputGenieModel;

/// Preserve held buttons and the unread portion of a controller stream.
static void check_controller_state() {
    NES::Controller source, restored;
    source.write_buttons(0xA5);
    source.strobe(0);
    for (int i = 0; i < 3; ++i) source.read();
    json_t* saved = source.dataToJson();
    restored.dataFromJson(saved);
    for (int i = 3; i < 8; ++i)
        assert(restored.read() == (0x40 | ((0xA5 >> i) & 1)));
    restored.strobe(0);
    for (int i = 0; i < 8; ++i)
        assert(restored.read() == (0x40 | ((0xA5 >> i) & 1)));
    for (json_t* invalid : {json_integer(-1), json_integer(256),
                           json_boolean(true), json_string("3"), json_real(3.0)}) {
        json_t* bad = json_object();
        json_object_set(bad, "joypad_buttons", invalid);
        json_object_set(bad, "joypad_bits", invalid);
        source.dataFromJson(bad);
        json_t* actual = source.dataToJson();
        assert(json_equal(actual, saved));
        json_decref(actual);
        json_decref(bad);
        json_decref(invalid);
    }
    json_decref(saved);
}

/// Decode mapper IDs without treating iNES RAM size as NES 2.0 mapper bits.
static void check_mapper_headers() {
    const char* path = ".build/mapper-header.nes";
    std::vector<char> bytes(16 + 0x8000 + 0x2000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = 2; bytes[5] = 1;
    NES::Emulator active;
    for (int format : {0x00, 0x04, 0x08, 0x0C}) {
        for (int mapper = 0; mapper < 4; ++mapper) {
            bytes[6] = mapper << 4;
            bytes[7] = format;
            bytes[8] = 1;
            std::ofstream file(path, std::ios::binary);
            file.write(bytes.data(), bytes.size());
            file.close();
            assert(file.good());
            const NES::ROM rom(path);
            assert(rom.get_mapper_number() == (format == 0x08 ? 0x100 + mapper : mapper));
            std::unique_ptr<NES::Cartridge> cartridge(NES::Cartridge::create(path, []() {}));
            assert(static_cast<bool>(cartridge) == (format != 0x08));
            if (format == 0 && mapper < 3) {
                json_t* saved = cartridge->get_mapper()->dataToJson();
                json_t* ram = json_object_get(saved, "character_ram");
                assert(json_is_string(ram) && json_string_length(ram) == 0);
                cartridge->get_mapper()->dataFromJson(saved);
                json_t* restored = cartridge->get_mapper()->dataToJson();
                assert(json_equal(saved, restored));
                json_decref(restored);
                json_decref(saved);
            }
            if (!active.has_game()) {
                assert(active.load_game(path));
                active.get_memory_buffer()[0x10] = 0xA5;
            }
            if (format == 0x08) {
                NES::Emulator empty;
                json_t* state = json_pack("{s:{s:s}}", "cartridge", "rom_path", path);
                assert(!empty.dataFromJson(state));
                assert(!empty.has_game());
                assert(!active.dataFromJson(state));
                assert(active.has_game() && active.get_memory_buffer()[0x10] == 0xA5);
                json_decref(state);
            }
        }
    }
    assert(std::remove(path) == 0);
}

/// Check MMC1 CHR bank pairs at startup and across register/mode changes.
static void check_mmc1_chr_banks() {
    const char* path = ".build/mmc1-chr.nes";
    std::vector<char> bytes(16 + 0x8000 + 0x4000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = 2; bytes[5] = 2; bytes[6] = 0x10;
    for (int i = 0; i < 0x4000; ++i)
        bytes[16 + 0x8000 + i] = 0x40 + i / 0x1000;
    std::ofstream file(path, std::ios::binary);
    file.write(bytes.data(), bytes.size());
    file.close();
    assert(file.good());
    std::unique_ptr<NES::Cartridge> cartridge(NES::Cartridge::create(path, []() {}));
    assert(cartridge);
    auto* mapper = cartridge->get_mapper();
    const auto check_banks = [&](int lower, int upper) {
        assert(mapper->readCHR(0x0000) == 0x40 + lower);
        assert(mapper->readCHR(0x0FFF) == 0x40 + lower);
        assert(mapper->readCHR(0x1000) == 0x40 + upper);
        assert(mapper->readCHR(0x1FFF) == 0x40 + upper);
    };
    const auto write_register = [&](NES::NES_Address address, int value) {
        for (int bit = 0; bit < 5; ++bit)
            mapper->writePRG(address, (value >> bit) & 1);
    };
    check_banks(0, 1);
    for (int bank = 0; bank < 4; ++bank) {
        write_register(0xA000, bank);
        check_banks(bank & ~1, (bank & ~1) + 1);
        write_register(0x8000, 0x0C);
        check_banks(bank & ~1, (bank & ~1) + 1);
    }
    write_register(0x8000, 0x1C);
    write_register(0xC000, 1);
    check_banks(3, 1);
    write_register(0xA000, 2);
    check_banks(2, 1);
    write_register(0xA000, 3);
    write_register(0x8000, 0x0C);
    check_banks(2, 3);
    write_register(0x8000, 0x1C);
    check_banks(3, 1);
    assert(std::remove(path) == 0);
}

int main() {
    check_controller_state();
    check_mmc1_chr_banks();
    check_mapper_headers();
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
    std::puts("RackNES: controller state, mapper headers/CHR, snapshots, failed loads, and RAM bounds passed");
}

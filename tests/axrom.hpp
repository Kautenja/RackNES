// AxROM cases adapted from nes-py 301da52f7f75de38 (MIT).
// Copyright (c) 2019 Christian Kauten. See docs/licenses/THIRD-PARTY.txt.
// RackNES-specific image validation, JSON, ownership and DMC regressions.
// Included after the production RackNES module in the assertion runner.

#ifndef RACKNES_TESTS_AXROM_HPP
#define RACKNES_TESTS_AXROM_HPP

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

/// Original synthetic bank markers; no commercial ROM data is required.
static std::vector<unsigned char> axrom_image(int banks) {
    std::vector<unsigned char> bytes(16 + banks * 0x8000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = banks * 2; bytes[6] = 0x70;
    for (int i = 0; i < banks * 0x8000; ++i) bytes[16 + i] = 0x20 + i / 0x4000;
    return bytes;
}

static void write_axrom_image(const std::vector<unsigned char>& bytes) {
    std::ofstream file(".build/axrom.nes", std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close();
    assert(file.good());
}

/// Banking, page callbacks, writable CHR, clone rebinding and JSON validation.
static void check_axrom() {
    const char* path = ".build/axrom.nes";
    for (int banks : {1, 2, 4, 8}) {
        write_axrom_image(axrom_image(banks));
        NES::PictureBus bus;
        int callbacks = 0;
        std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path, [&]() {
            ++callbacks;
            bus.update_mirroring();
        }));
        assert(cart && callbacks == 0);  // No callback before bus attachment.
        auto* mapper = cart->get_mapper();
        bus.set_mapper(mapper);
        assert(mapper->getNameTableMirroring() == NES::ONE_SCREEN_LOWER);
        bus.write(0x2000, 0xA1);
        mapper->writePRG(0x8000, 0x10);
        bus.write(0x2400, 0xB2);
        for (int value = 0; value < 256; ++value) {
            mapper->writePRG(value & 1 ? 0x8000 : 0xFFFF, value);
            const int bank = (value & 7) % banks;
            assert(mapper->readPRG(0x8000) == 0x20 + bank * 2);
            assert(mapper->readPRG(0xBFFF) == 0x20 + bank * 2);
            assert(mapper->readPRG(0xC000) == 0x21 + bank * 2);
            assert(mapper->readPRG(0xFFFF) == 0x21 + bank * 2);
            for (int address : {0x2000, 0x2400, 0x2800, 0x2C00, 0x3000})
                assert(bus.read(address) == (value & 0x10 ? 0xB2 : 0xA1));
        }
        for (int i = 0; i < 0x2000; ++i) bus.write(i, i ^ (i >> 8));
        json_t* saved = mapper->dataToJson();
        mapper->writePRG(0x8000, 0);
        bus.write(0, 0xFF);
        mapper->dataFromJson(saved);
        json_t* restored = mapper->dataToJson();
        assert(json_equal(saved, restored));
        json_decref(restored);
        assert(bus.read(0x2800) == 0xB2);
        // Corruption must leave bank selection and every CHR byte untouched.
        for (int kind = 0; kind < 7; ++kind) {
            json_t* bad = json_deep_copy(saved);
            if (kind < 3)
                json_object_set_new(bad, "bank_select", kind == 0 ? json_integer(-1) :
                    kind == 1 ? json_integer(8) : json_boolean(true));
            else if (kind == 3) json_object_del(bad, "character_ram");
            else if (kind == 4) json_object_set_new(bad, "character_ram", json_string("short"));
            else {
                std::string ram = json_string_value(json_object_get(bad, "character_ram"));
                ram[kind == 5 ? 42 : ram.size() - 1] = '!';
                json_object_set_new(bad, "character_ram", json_string(ram.c_str()));
            }
            assert(!NES::MapperAxROM::is_valid_state(bad));
            mapper->dataFromJson(bad);
            restored = mapper->dataToJson();
            assert(json_equal(saved, restored));
            json_decref(restored);
            json_decref(bad);
        }
        NES::PictureBus clone_bus;
        std::unique_ptr<NES::Cartridge> clone(cart->clone([&]() { clone_bus.update_mirroring(); }));
        clone_bus.set_mapper(clone->get_mapper());
        clone_bus.write(0x2000, 0xC3);
        clone->get_mapper()->writeCHR(0, 0xAA);
        assert(mapper->readCHR(0) == 0);
        const int old_callbacks = callbacks;
        cart.reset();  // Clone must own its ROM and never call the old bus.
        clone->get_mapper()->writePRG(0x8000, 0);
        clone->get_mapper()->writePRG(0x8000, 0x17);
        assert(callbacks == old_callbacks);
        assert(clone_bus.read(0x2400) == 0xC3);
        assert(clone->get_mapper()->readPRG(0xFFFF) == 0x21 + ((7 % banks) * 2));
        assert(clone->get_mapper()->readCHR(0) == 0xAA);
        json_decref(saved);
    }
    // Bus conflicts use the byte visible before changing banks or mirroring.
    for (int submapper : {0, 1, 2}) {
        auto bytes = axrom_image(4);
        bytes[7] = 8; bytes[8] = submapper << 4; bytes[11] = 7;
        bytes[16] = 0x11;
        bytes[16 + 0x8000] = 0x02;
        write_axrom_image(bytes);
        std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path, []() {}));
        assert(cart);
        cart->get_mapper()->writePRG(0x8000, 0x13);
        assert(cart->get_mapper()->readPRG(0x9000) == (submapper == 2 ? 0x22 : 0x26));
        cart->get_mapper()->writePRG(0x8000, 0x13);
        assert(cart->get_mapper()->readPRG(0x9000) == (submapper == 2 ? 0x24 : 0x26));
        assert(cart->get_mapper()->getNameTableMirroring() ==
            (submapper == 2 ? NES::ONE_SCREEN_LOWER : NES::ONE_SCREEN_HIGHER));
    }
    // Reject unsupported/truncated layouts without replacing an active game.
    write_axrom_image(axrom_image(1));
    NES::Emulator active;
    assert(active.load_game(path));
    active.get_memory_buffer()[0x46] = 0xA5;
    for (int kind = 0; kind < 15; ++kind) {
        auto bytes = axrom_image(1);
        switch (kind) {
            case 0: bytes.resize(8); break;
            case 1: bytes.pop_back(); break;
            case 2: bytes[4] = 0; break;
            case 3: bytes[4] = 3; break;
            case 4: bytes[5] = 1; break;
            case 5: bytes[6] |= 2; break;
            case 6: bytes[6] |= 4; break;
            case 7: bytes[6] |= 8; break;
            case 8: bytes[7] = 1; break;
            case 9: bytes[9] = 1; break;
            case 10: bytes[0] = 'X'; break;
            case 11: bytes[7] = 8; bytes[8] = 0x30; bytes[11] = 7; break;
            case 12: bytes[7] = 8; bytes[11] = 8; break;
            case 13: bytes[7] = 8; bytes[11] = 7; bytes[12] = 1; break;
            case 14: bytes.push_back(0); break;
        }
        write_axrom_image(bytes);
        assert(!active.load_game(path));
        assert(active.has_game() && active.get_memory_buffer()[0x46] == 0xA5);
    }
    write_axrom_image(axrom_image(4));
    std::unique_ptr<NES::Cartridge> source(NES::Cartridge::create(path, []() {}));
    source->get_mapper()->writePRG(0x8000, 0x13);
    source->get_mapper()->writeCHR(0x1FFF, 0xCD);
    json_t* state = json_object();
    json_object_set_new(state, "cartridge", source->dataToJson());
    assert(active.dataFromJson(state));
    json_t* full = active.dataToJson();
    auto* picture = json_object_get(full, "picture_bus");
    json_object_set_new(picture, "name_tables[0]", json_integer(0));
    assert(active.dataFromJson(full));
    json_t* roundtrip = active.dataToJson();
    assert(json_equal(json_object_get(full, "cartridge"),
                      json_object_get(roundtrip, "cartridge")));
    assert(json_integer_value(json_object_get(json_object_get(roundtrip,
        "picture_bus"), "name_tables[0]")) == 0x400);
    active.reset();  // CPU reset retains AxROM latch and CHR RAM.
    json_t* reset_state = active.dataToJson();
    assert(json_equal(json_object_get(full, "cartridge"),
                      json_object_get(reset_state, "cartridge")));
    json_decref(reset_state);
    active.get_memory_buffer()[0x46] = 0xA5;
    json_t* cart_state = json_object_get(full, "cartridge");
    json_object_del(json_object_get(cart_state, "mapper"), "character_ram");
    assert(!active.dataFromJson(full));
    assert(active.get_memory_buffer()[0x46] == 0xA5);
    json_decref(full); json_decref(roundtrip); json_decref(state);
    assert(active.load_game(path));  // ROM replacement powers on fresh state.
    full = active.dataToJson();
    assert(json_integer_value(json_object_get(json_object_get(
        json_object_get(full, "cartridge"), "mapper"), "bank_select")) == 0);
    json_decref(full);
    assert(std::remove(path) == 0);
}

/// Exercise the actual APU DMC callback through the same CPU bus as playback.
static void check_axrom_dmc() {
    const char* path = ".build/axrom.nes";
    write_axrom_image(axrom_image(4));
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path, []() {}));
    NES::MainBus bus;
    bus.set_mapper(cart->get_mapper());
    NES::APU apu;
    int reads[4] = {};
    int bank = 0;
    apu.set_dmc_reader([&](void*, cpu_addr_t address) -> int {
        const int value = bus.read(address);
        assert(address >= 0xC000 && value == 0x21 + bank * 2);
        ++reads[bank];
        return value;
    });
    apu.set_irq_callback([](void*) {});
    apu.reset();
    apu.write(0x4010, 0x4F);  // Loop, fastest DMC period, IRQ disabled.
    apu.write(0x4012, 0); apu.write(0x4013, 1); apu.write(0x4015, 0x10);
    for (bank = 0; bank < 4; ++bank) {
        bus.write(0x8000, bank);
        for (int cycle = 0; cycle < 20000; ++cycle) apu.cycle();
        assert(reads[bank] > 0);
    }
    assert(std::remove(path) == 0);
}

/// SAVE/LOAD and patch persistence retain independent live and backup banks.
static void check_axrom_module() {
    const char* path = ".build/axrom.nes";
    write_axrom_image(axrom_image(4));
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path, []() {}));
    cart->get_mapper()->writePRG(0x8000, 0x13);
    cart->get_mapper()->writeCHR(0x1FFF, 0xCD);
    json_t* state = json_object();
    json_object_set_new(state, "cartridge", cart->dataToJson());
    std::unique_ptr<RackNES> source(new RackNES);
    assert(source->emulator.dataFromJson(state));
    source->processCV();
    source->params[RackNES::PARAM_SAVE].setValue(1.f);
    source->processCV();
    assert(source->backup);
    assert(source->emulator.load_game(path));
    json_t* patch = source->dataToJson();
    std::unique_ptr<RackNES> restored(new RackNES);
    restored->dataFromJson(patch);
    assert(!restored->rom_reload_failed_signal && restored->backup);
    source.reset();
    json_decref(patch);
    restored->processCV();
    restored->params[RackNES::PARAM_LOAD].setValue(1.f);
    restored->processCV();
    json_t* loaded = restored->emulator.dataToJson();
    assert(json_equal(json_object_get(loaded, "cartridge"),
                      json_object_get(state, "cartridge")));
    json_decref(loaded); json_decref(state);
    restored->onReset();
    assert(!restored->emulator.has_game() && restored->backup == nullptr);
    assert(std::remove(path) == 0);
}

#endif  // RACKNES_TESTS_AXROM_HPP

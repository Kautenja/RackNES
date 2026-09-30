// MMC2 banking/latch cases adapted from nes-py 301da52f7f75de38 (MIT).
// Copyright (c) 2019 Christian Kauten. See docs/licenses/THIRD-PARTY.txt.
// Included after the production RackNES module in the assertion runner.
#ifndef RACKNES_TESTS_MMC2_HPP
#define RACKNES_TESTS_MMC2_HPP

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

/// Distinct 8 KiB PRG and 4 KiB CHR markers in an original synthetic ROM.
static std::vector<unsigned char> mmc2_image(int prg = 16, int chr = 32) {
    std::vector<unsigned char> bytes(16 + prg * 0x2000 + chr * 0x1000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = prg / 2; bytes[5] = chr / 2; bytes[6] = 0x90;
    for (int i = 0; i < prg * 0x2000; ++i) bytes[16 + i] = 0x20 + i / 0x2000;
    for (int i = 0; i < chr * 0x1000; ++i)
        bytes[16 + prg * 0x2000 + i] = 0x40 + i / 0x1000;
    return bytes;
}

static void write_mmc2_image(const std::vector<unsigned char>& bytes) {
    std::ofstream file(".build/mmc2.nes", std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close();
    assert(file.good());
}

static void configure_mmc2(NES::ROM::Mapper* mapper) {
    mapper->writePRG(0xB000, 1); mapper->writePRG(0xC000, 2);
    mapper->writePRG(0xD000, 3); mapper->writePRG(0xE000, 4);
}

/// CPU windows, register masks, CHR trigger boundaries and old-byte ordering.
static void check_mmc2_mapping() {
    const char* path = ".build/mmc2.nes";
    for (int prg : {4, 8, 16}) {
        for (int chr : {2, 4, 8, 16, 32}) {
            write_mmc2_image(mmc2_image(prg, chr));
            NES::PictureBus picture;
            std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path,
                [&]() { picture.update_mirroring(); }));
            assert(cart);
            auto* mapper = cart->get_mapper();
            picture.set_mapper(mapper);
            NES::MainBus bus;
            bus.set_mapper(mapper);
            bus.write(0x6000, 0xA1); bus.write(0x7FFF, 0xB2);
            assert(bus.read(0x6000) == 0xA1 && bus.read(0x7FFF) == 0xB2);
            assert(bus.get_page_pointer(0x7F)[255] == 0xB2);
            for (int value = 0; value < 256; ++value) {
                bus.write(value & 1 ? 0xA000 : 0xAFFF, value);
                assert(bus.read(0x8000) == 0x20 + (value & 15) % prg);
                assert(bus.read(0x9FFF) == bus.read(0x8000));
                for (int window = 1; window < 4; ++window) {
                    assert(bus.read(0x8000 + window * 0x2000) == 0x20 + prg - 4 + window);
                    assert(bus.read(0x9FFF + window * 0x2000) == 0x20 + prg - 4 + window);
                }
                for (int reg = 0; reg < 4; ++reg)
                    bus.write(0xBFFF + reg * 0x1000, value);
                assert(picture.read(0x0000) == 0x40 + (value & 31) % chr);
                assert(picture.read(0x1000) == 0x40 + (value & 31) % chr);
            }
            configure_mmc2(mapper);
            picture.read(0x0FE8); picture.read(0x1FE8);
            for (int address : {0x0FD7, 0x0FD9, 0x0FDF}) picture.read(address);
            assert(picture.read(0) == 0x40 + 2 % chr);
            assert(picture.read(0x0FD8) == 0x40 + 2 % chr);
            assert(picture.read(0) == 0x40 + 1 % chr);
            for (int address : {0x0FE7, 0x0FE9, 0x0FEF}) picture.read(address);
            assert(picture.read(0) == 0x40 + 1 % chr);
            assert(picture.read(0x0FE8) == 0x40 + 1 % chr);
            for (int offset = 0; offset < 8; ++offset) {
                assert(picture.read(0x1FD8 + offset) == 0x40 + 4 % chr);
                assert(picture.read(0x1000) == 0x40 + 3 % chr);
                for (int address : {0x1FE7, 0x1FF0}) picture.read(address);
                assert(picture.read(0x1000) == 0x40 + 3 % chr);
                assert(picture.read(0x1FE8 + offset) == 0x40 + 3 % chr);
                for (int address : {0x1FD7, 0x1FE0}) picture.read(address);
            }
            picture.write(0x0FD8, 0xFF); picture.write(0x1FD8, 0xFF);
            assert(picture.read(0) == 0x40 + 2 % chr);
            assert(picture.read(0x1000) == 0x40 + 4 % chr);
            mapper->writePRG(0x8000, 0xFF); mapper->writePRG(0x9FFF, 0xFF);
            assert(picture.read(0) == 0x40 + 2 % chr);
            for (int mode : {0, 1, 0xFE, 0xFF}) {
                bus.write(0xFFFF, mode);
                picture.write(0x2000, 0x11);
                picture.write(mode & 1 ? 0x2800 : 0x2400, 0x22);
                assert(picture.read(0x2400) == (mode & 1 ? 0x11 : 0x22));
                assert(picture.read(0x2800) == (mode & 1 ? 0x22 : 0x11));
            }
            // Save immediately after a trigger, before observing the new bank.
            picture.read(0x0FD8);
            json_t* saved = mapper->dataToJson();
            picture.read(0x0FE8);
            mapper->dataFromJson(saved);
            assert(picture.read(0) == 0x40 + 1 % chr);
            for (int kind = 0; kind < 5; ++kind) {
                json_t* bad = json_deep_copy(saved);
                if (kind == 0) json_object_set_new(bad, "register_prg", json_integer(16));
                if (kind == 1) json_array_set_new(json_object_get(bad, "register_chr"), 0, json_integer(-1));
                if (kind == 2) json_array_append_new(json_object_get(bad, "register_chr"), json_integer(0));
                if (kind == 3) json_object_set_new(bad, "left_latch_fe", json_integer(0));
                if (kind == 4) json_object_del(bad, "horizontal");
                assert(!NES::MapperMMC2::is_valid_state(bad));
                mapper->dataFromJson(bad);
                json_t* actual = mapper->dataToJson();
                assert(json_equal(saved, actual));
                json_decref(actual); json_decref(bad);
            }
            NES::PictureBus clone_bus;
            std::unique_ptr<NES::Cartridge> clone(cart->clone([&]() { clone_bus.update_mirroring(); }));
            clone_bus.set_mapper(clone->get_mapper());
            cart.reset();
            assert(clone_bus.read(0) == 0x40 + 1 % chr);
            clone->get_mapper()->writePRG(0xA000, 1);
            assert(clone->get_mapper()->readPRG(0x8000) == 0x21);
            clone->get_mapper()->writePRG(0xF000, 0);
            assert(clone->get_mapper()->getNameTableMirroring() == NES::VERTICAL);
            json_decref(saved);
        }
    }
    assert(std::remove(path) == 0);
}

/// Pattern bytes stay fixed across a triggering tile and snapshot continuation.
static void check_mmc2_ppu() {
    write_mmc2_image(mmc2_image());
    NES::PictureBus bus;
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(".build/mmc2.nes",
        [&]() { bus.update_mirroring(); }));
    auto* mapper = cart->get_mapper();
    bus.set_mapper(mapper);
    configure_mmc2(mapper);
    bus.write(0x2000, 0xFD); bus.write(0x2001, 0);
    NES::PPU ppu;
    ppu.reset(); ppu.set_mask(0x08);  // Background enabled, first 8 pixels clipped.
    json_t* state = json_pack("{s:i,s:i,s:i}", "pipeline_state", 1, "cycles", 1, "scanline", 0);
    ppu.dataFromJson(state); json_decref(state);
    ppu.cycle(bus);
    assert(mapper->readCHR(0) == 0x41);  // Hidden tile still clocks FD.
    state = ppu.dataToJson();
    json_t* fetch = json_object_get(state, "chr_latch_fetches");
    assert(json_integer_value(json_object_get(fetch, "low")) == 0x42);
    assert(json_integer_value(json_object_get(fetch, "high")) == 0x42);
    // Change the latch after the fetch; the remaining pixels cannot reread it.
    mapper->readCHR(0x0FE8);
    NES::PPU restored;
    restored.reset(); restored.dataFromJson(state);
    for (int pixel = 1; pixel < 8; ++pixel) {
        restored.cycle(bus);
        assert(mapper->readCHR(0) == 0x42);
    }
    json_decref(state);
    // Covered sprite 1 must fetch after sprite 0, even when sprites are hidden.
    // Its FD tile changes the latch, but sprite 0 keeps its own fetched bytes.
    for (int mask : {0x08, 0x18, 0x10}) {
        ppu.reset(); ppu.set_mask(mask);
        std::array<NES::NES_Byte, 256> oam;
        oam.fill(0xFF);
        oam[0] = 0; oam[1] = 0xFE; oam[2] = 0; oam[3] = 0;
        oam[4] = 0; oam[5] = 0xFD; oam[6] = 0; oam[7] = 0;
        ppu.do_DMA(oam.data());
        mapper->readCHR(0x0FD8);
        state = json_pack("{s:i,s:i,s:i}", "pipeline_state", 1,
            "cycles", NES::SCANLINE_END_CYCLE, "scanline", 0);
        ppu.dataFromJson(state); json_decref(state);
        ppu.cycle(bus);
        assert(mapper->readCHR(0) == 0x41);
        state = ppu.dataToJson();
        fetch = json_object_get(state, "chr_latch_fetches");
        json_t* sprites = json_object_get(fetch, "sprites");
        assert(json_integer_value(json_array_get(sprites, 0)) == 0x41);
        assert(json_integer_value(json_array_get(sprites, 1)) == 0x41);
        assert(json_integer_value(json_array_get(sprites, 2)) == 0x42);
        assert(json_integer_value(json_array_get(sprites, 3)) == 0x42);
        restored.reset(); restored.dataFromJson(state);
        json_t* actual = restored.dataToJson();
        assert(json_equal(fetch, json_object_get(actual, "chr_latch_fetches")));
        json_decref(actual); json_decref(state);
    }
    // Both sprite heights and vertical flips must address the correct row.
    for (int variant = 0; variant < 4; ++variant) {
        const bool tall = variant >= 2, flipped = variant & 1;
        const int line = tall ? (flipped ? 7 : 8) : (flipped ? 7 : 0);
        ppu.reset(); ppu.control(tall ? 0x20 : 0); ppu.set_mask(0x10);
        std::array<NES::NES_Byte, 256> oam;
        oam.fill(0xFF);
        oam[0] = 0; oam[1] = 0xFD; oam[2] = flipped ? 0x80 : 0; oam[3] = 255;
        ppu.do_DMA(oam.data());
        mapper->readCHR(0x0FE8); mapper->readCHR(0x1FE8);
        state = json_pack("{s:i,s:i,s:i}", "pipeline_state", 1,
            "cycles", NES::SCANLINE_END_CYCLE, "scanline", line);
        ppu.dataFromJson(state); json_decref(state);
        ppu.cycle(bus);
        assert(mapper->readCHR(tall ? 0x1000 : 0) == (tall ? 0x43 : 0x41));
        state = ppu.dataToJson();
        fetch = json_object_get(state, "chr_latch_fetches");
        assert(json_integer_value(json_array_get(json_object_get(fetch, "sprites"), 1)) ==
            (tall ? 0x44 : 0x42));
        json_decref(state);
    }
    // CPU PPUDATA buffering returns the old byte from a triggering read.
    ppu.reset();
    mapper->readCHR(0x0FE8);
    ppu.set_data_address(0x0F); ppu.set_data_address(0xD8);
    assert(ppu.get_data(bus) == 0);
    assert(ppu.get_data(bus) == 0x42);
    assert(ppu.get_data(bus) == 0x41);
    assert(std::remove(".build/mmc2.nes") == 0);
}

/// Supported header variants, rejected layouts, live/backup state and PRG RAM.
static void check_mmc2_state() {
    const char* path = ".build/mmc2.nes";
    for (int format : {0, 8}) for (int battery : {0, 2}) {
        auto bytes = mmc2_image();
        bytes[6] |= battery; bytes[7] = format;
        if (format) bytes[10] = battery ? 0x70 : 7;
        write_mmc2_image(bytes);
        std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path, []() {}));
        assert(cart && cart->get_mapper()->hasExtendedRAM());
    }
    std::unique_ptr<RackNES> source(new RackNES);
    assert(source->emulator.load_game(path));
    source->emulator.get_memory_buffer()[0x46] = 0xA5;
    for (int kind = 0; kind < 14; ++kind) {
        auto bytes = mmc2_image();
        switch (kind) {
            case 0: bytes.resize(8); break;
            case 1: bytes.pop_back(); break;
            case 2: bytes[4] = 0; break;
            case 3: bytes[4] = 16; break;
            case 4: bytes[5] = 0; break;
            case 5: bytes[5] = 32; break;
            case 6: bytes[6] |= 4; break;
            case 7: bytes[6] |= 8; break;
            case 8: bytes[7] = 1; break;
            case 9: bytes[9] = 1; break;
            case 10: bytes[0] = 'X'; break;
            case 11: bytes[7] = 8; bytes[8] = 0x10; bytes[10] = 7; break;
            case 12: bytes[7] = 8; bytes[10] = 0; break;
            case 13: bytes.push_back(0); break;
        }
        write_mmc2_image(bytes);
        assert(!source->emulator.load_game(path));
        assert(source->emulator.get_memory_buffer()[0x46] == 0xA5);
    }
    write_mmc2_image(mmc2_image());
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(path, []() {}));
    auto* mapper = cart->get_mapper();
    configure_mmc2(mapper); mapper->writePRG(0xA000, 3);
    mapper->writePRG(0xF000, 0); mapper->readCHR(0x0FD8); mapper->readCHR(0x1FD8);
    json_t* state = source->emulator.dataToJson();
    json_object_set_new(state, "cartridge", cart->dataToJson());
    // Persist the bus-owned 8 KiB RAM in its existing JSON field.
    std::array<NES::NES_Byte, 0x2000> ram = {};
    ram.front() = 0xA1; ram.back() = 0xB2;
    const std::string encoded = base64_encode(ram.data(), ram.size());
    json_object_set_new(json_object_get(state, "bus"), "extended_ram", json_string(encoded.c_str()));
    assert(source->emulator.dataFromJson(state));
    source->processCV(); source->params[RackNES::PARAM_SAVE].setValue(1.f); source->processCV();
    assert(source->emulator.load_game(path));
    json_t* fresh = source->emulator.dataToJson();
    assert(base64_decode(json_string_value(json_object_get(json_object_get(fresh,
        "bus"), "extended_ram"))) == std::string(0x2000, '\0'));
    json_decref(fresh);
    json_t* patch = source->dataToJson();
    std::unique_ptr<RackNES> restored(new RackNES);
    restored->dataFromJson(patch);
    assert(!restored->rom_reload_failed_signal);
    source.reset(); json_decref(patch);
    restored->processCV(); restored->params[RackNES::PARAM_LOAD].setValue(1.f); restored->processCV();
    json_t* actual = restored->emulator.dataToJson();
    assert(json_equal(json_object_get(state, "cartridge"), json_object_get(actual, "cartridge")));
    assert(json_equal(json_object_get(json_object_get(state, "bus"), "extended_ram"),
                      json_object_get(json_object_get(actual, "bus"), "extended_ram")));
    // Bad new fields are rejected before replacing the loaded state.
    restored->emulator.get_memory_buffer()[0x46] = 0xA5;
    json_object_set_new(json_object_get(json_object_get(state, "cartridge"), "mapper"),
                        "register_prg", json_integer(99));
    assert(!restored->emulator.dataFromJson(state));
    json_object_set_new(json_object_get(json_object_get(state, "cartridge"), "mapper"),
                        "register_prg", json_integer(3));
    json_object_set_new(json_object_get(json_object_get(state, "ppu"), "chr_latch_fetches"),
                        "low", json_integer(256));
    assert(!restored->emulator.dataFromJson(state));
    assert(restored->emulator.get_memory_buffer()[0x46] == 0xA5);
    json_decref(actual); json_decref(state);
    restored->onReset();
    assert(!restored->emulator.has_game() && !restored->backup);
    assert(std::remove(path) == 0);
}

/// Actual CPU writes/PPUDATA reads traverse the complete emulator callbacks.
static void check_mmc2_cpu() {
    auto bytes = mmc2_image();
    const std::size_t start = 16 + 15 * 0x2000;
    std::size_t pc = start;
    bytes[pc++] = 0x78;  // SEI
    const auto write = [&](int address, int value) {
        bytes[pc++] = 0xA9; bytes[pc++] = value;
        bytes[pc++] = 0x8D; bytes[pc++] = address & 255; bytes[pc++] = address >> 8;
    };
    const auto read = [&](int address) {
        bytes[pc++] = 0xAD; bytes[pc++] = address & 255; bytes[pc++] = address >> 8;
    };
    const auto store = [&](int address) { bytes[pc++] = 0x85; bytes[pc++] = address; };
    write(0x2001, 0); write(0xB000, 1); write(0xC000, 2);
    write(0x2006, 0x0F); write(0x2006, 0xD8);
    read(0x2007); read(0x2007); store(0x46);
    read(0x2007); store(0x47);
    write(0xA000, 3); read(0x8000); store(0x48);
    write(0x6000, 0xAA); read(0x6000); store(0x49);
    const int loop = 0xE000 + pc - start;
    bytes[pc++] = 0x4C; bytes[pc++] = loop & 255; bytes[pc++] = loop >> 8;
    bytes[16 + 16 * 0x2000 - 4] = 0;
    bytes[16 + 16 * 0x2000 - 3] = 0xE0;
    write_mmc2_image(bytes);
    std::unique_ptr<NES::Emulator> emulator(new NES::Emulator);
    assert(emulator->load_game(".build/mmc2.nes"));
    for (int cycle = 0; cycle < 5000; ++cycle) emulator->cycle([]() {});
    const auto* ram = emulator->get_memory_buffer();
    assert(ram[0x46] == 0x42 && ram[0x47] == 0x41);
    assert(ram[0x48] == 0x23 && ram[0x49] == 0xAA);
    assert(std::remove(".build/mmc2.nes") == 0);
}

#endif  // RACKNES_TESTS_MMC2_HPP

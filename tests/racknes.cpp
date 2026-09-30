// Focused snapshot, cartridge header, controller, and expander regressions.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// This executable hosts Rack's engine for module tests. Read its internal
// constructor before rack.hpp applies the plugin-only PRIVATE restriction.
#include <engine/Engine.hpp>
#undef PRIVATE
#include "../src/RackNES.cpp"

Plugin* plugin_instance = nullptr;
static Model inputGenieModel;
Model* modelInputGenie = &inputGenieModel;

#include "axrom.hpp"
#include "mmc2.hpp"
#include "mmc3.hpp"
#include "sram.hpp"
#include "ntsc.hpp"

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
    // Keep large emulator instances off the sanitizer-instrumented stack.
    std::unique_ptr<NES::Emulator> active(new NES::Emulator);
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
            if (!active->has_game()) {
                assert(active->load_game(path));
                active->get_memory_buffer()[0x10] = 0xA5;
            }
            if (format == 0x08) {
                std::unique_ptr<NES::Emulator> empty(new NES::Emulator);
                json_t* state = json_pack("{s:{s:s}}", "cartridge", "rom_path", path);
                assert(!empty->dataFromJson(state));
                assert(!empty->has_game());
                assert(!active->dataFromJson(state));
                assert(active->has_game() && active->get_memory_buffer()[0x10] == 0xA5);
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
    // Old snapshots stored derived offsets from the broken startup/alignment.
    // The serialized registers, including partial writes, remain authoritative.
    for (int mode : {0x0C, 0x1C}) {
        write_register(0x8000, mode);
        mapper->writePRG(0xA000, 0);  // First bit of selecting CHR bank 2.
        json_t* legacy = mapper->dataToJson();
        json_object_set_new(legacy, "first_bank_chr", json_integer(0));
        json_object_set_new(legacy, "second_bank_chr", json_integer(0));
        mapper->dataFromJson(legacy);
        check_banks(mode == 0x0C ? 2 : 3, mode == 0x0C ? 3 : 1);
        for (int bit = 1; bit < 5; ++bit)
            mapper->writePRG(0xA000, (2 >> bit) & 1);
        check_banks(2, mode == 0x0C ? 3 : 1);
        write_register(0xA000, 3);
        json_decref(legacy);
    }
    assert(std::remove(path) == 0);
}

/// MMC1 work RAM exists without a battery, as required by Metroid (#26).
static void check_mmc1_work_ram() {
    const char* path = ".build/mmc1-ram.nes";
    std::vector<char> bytes(16 + 8 * 0x4000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = 8;  // 128 KiB PRG, CHR RAM, legacy unspecified PRG RAM size.
    for (int bank = 0; bank < 8; ++bank)
        bytes[16 + bank * 0x4000] = 0x40 + bank;
    for (int flags : {0x10, 0x12}) {
        bytes[6] = flags;
        std::ofstream file(path, std::ios::binary);
        file.write(bytes.data(), bytes.size());
        file.close();
        assert(file.good());
        std::unique_ptr<NES::Cartridge> cartridge(NES::Cartridge::create(path, []() {}));
        assert(cartridge);
        auto* mapper = cartridge->get_mapper();
        NES::MainBus bus;
        bus.set_mapper(mapper);
        // The old battery-only check silently discarded this write.
        bus.write(0x6000, 0xA5);
        assert(bus.read(0x6000) == 0xA5);
        for (int address = 0x6000; address < 0x8000; ++address)
            bus.write(address, (address ^ (address >> 8)) & 0xFF);
        const auto check_ram = [&]() {
            for (int address = 0x6000; address < 0x8000; ++address)
                assert(bus.read(address) == ((address ^ (address >> 8)) & 0xFF));
            for (int page = 0x60; page <= 0x7F; ++page) {
                const auto* data = bus.get_page_pointer(page);
                assert(data);
                for (int i = 0; i < 256; ++i)
                    assert(data[i] == (i ^ page));
            }
        };
        check_ram();
        // PRG switching and serial-register reset must not replace work RAM.
        for (int bank = 0; bank < 8; ++bank) {
            mapper->writePRG(0x8000, 0x80);
            for (int bit = 0; bit < 5; ++bit)
                mapper->writePRG(0xE000, (bank >> bit) & 1);
            assert(bus.read(0x8000) == 0x40 + bank);
            assert(bus.read(0xC000) == 0x47);
            check_ram();
        }
        json_t* saved = bus.dataToJson();
        bus.write(0x6000, 0);
        bus.dataFromJson(saved);
        check_ram();
        // Old non-battery snapshots contain an empty RAM string. Short or
        // mistyped external data must not shrink storage used by the bus.
        for (json_t* value : {json_string(""), json_string("pQ=="),
                              json_integer(1), json_null()}) {
            json_t* legacy = json_pack("{s:O}", "extended_ram", value);
            bus.dataFromJson(legacy);
            // Malformed direct bus restores preserve existing mapper-owned RAM.
            // Full legacy emulator restores below create a fresh zeroed mapper.
            check_ram();
            bus.write(0x7FFF, 0x5A);
            assert(bus.read(0x7FFF) == 0x5A);
            bus.dataFromJson(saved);
            check_ram();
            json_decref(legacy);
            json_decref(value);
        }
        json_decref(saved);
        // Replacing a cartridge starts fresh storage; missing fields preserve it.
        std::unique_ptr<NES::Cartridge> replacement(NES::Cartridge::create(path, []() {}));
        assert(replacement);
        bus.set_mapper(replacement->get_mapper());
        for (int address = 0x6000; address < 0x8000; ++address)
            assert(bus.read(address) == 0);
        bus.write(0x6000, 0xA5);
        json_t* missing = json_object();
        bus.dataFromJson(missing);
        assert(bus.read(0x6000) == 0xA5);
        json_decref(missing);
    }
    // Execute an original CPU program that stages sprite and room data in
    // work RAM, then transfers it to internal RAM and the PPU, like Metroid.
    const unsigned char program[] = {
        0x78,                           // SEI
        0xA9, 0x00, 0x8D, 0x00, 0x20,   // Disable NMI.
        0x8D, 0x01, 0x20,               // Disable rendering during upload.
        0xA9, 0x5A, 0x8D, 0x00, 0x60,   // Room byte at $6000.
        0xA9, 0xA5, 0x8D, 0xA0, 0x6E,   // Intro sprite byte at $6EA0.
        0xAD, 0xA0, 0x6E, 0x8D, 0x00, 0x02,
        0xA9, 0x20, 0x8D, 0x06, 0x20,   // PPUADDR = $2000.
        0xA9, 0x00, 0x8D, 0x06, 0x20,
        0xAD, 0x00, 0x60, 0x8D, 0x07, 0x20,
        0x4C, 0x29, 0xC1                // Loop at $C129.
    };
    bytes[6] = 0x10;
    for (size_t i = 0; i < sizeof(program); ++i)
        bytes[16 + 7 * 0x4000 + 0x100 + i] = program[i];
    bytes[16 + 8 * 0x4000 - 4] = 0;
    bytes[16 + 8 * 0x4000 - 3] = 0xC1;
    std::ofstream file(path, std::ios::binary);
    file.write(bytes.data(), bytes.size());
    file.close();
    assert(file.good());
    std::unique_ptr<NES::Emulator> emulator(new NES::Emulator);
    assert(emulator->load_game(path));
    for (int cycle = 0; cycle < 1000; ++cycle) emulator->cycle([]() {});
    assert(emulator->get_memory_buffer()[0x200] == 0xA5);
    json_t* saved = emulator->dataToJson();
    const auto picture_ram = base64_decode(json_string_value(
        json_object_get(json_object_get(saved, "picture_bus"), "ram")));
    assert(picture_ram[0] == 0x5A);
    // Restore the complete emulator, including its cartridge and work RAM.
    assert(emulator->dataFromJson(saved));
    json_t* restored = emulator->dataToJson();
    assert(json_equal(json_object_get(saved, "bus"), json_object_get(restored, "bus")));
    json_decref(restored);
    // Old snapshots must remain safe when the running CPU next reads work RAM.
    json_object_set_new(json_object_get(saved, "bus"), "extended_ram", json_string(""));
    assert(emulator->dataFromJson(saved));
    restored = emulator->dataToJson();
    const auto work_ram = base64_decode(json_string_value(
        json_object_get(json_object_get(restored, "bus"), "extended_ram")));
    assert(work_ram == std::string(0x2000, '\0'));
    json_decref(restored);
    json_decref(saved);
    assert(std::remove(path) == 0);
}

/// Negative samples agree across bulk, overlapping, stereo, and reader paths.
static void check_blip_sample_reads() {
    Blip_Buffer bulk, chunked, stereo, direct;
    blip_sample_t input[32];
    for (int i = 0; i < 32; ++i) input[i] = (i / 4) % 2 ? 1000 : -1000;
    for (auto* buffer : {&bulk, &chunked, &stereo, &direct}) {
        assert(buffer->sample_rate(48000) == nullptr);
        buffer->clock_rate(48000);
        buffer->mix_samples(input, 32);
        buffer->end_frame(64);
    }
    blip_sample_t expected[64], interleaved[128];
    for (auto& value : interleaved) value = 1234;
    assert(bulk.read_samples(expected, 64) == 64);
    assert(stereo.read_samples(interleaved, 64, true) == 64);
    Blip_Reader reader;
    const int bass_shift = reader.begin(direct);
    bool negative = false, positive = false;
    for (int i = 0; i < 64; ++i) {
        blip_sample_t value;
        assert(chunked.read_samples(&value, 1) == 1);
        assert(value == expected[i]);
        assert(interleaved[2 * i] == expected[i]);
        assert(interleaved[2 * i + 1] == 1234);
        assert(reader.read() == expected[i]);
        reader.next(bass_shift);
        negative |= value < 0;
        positive |= value > 0;
    }
    reader.end(direct);
    direct.remove_samples(64);
    assert(negative && positive);
    assert(chunked.samples_avail() == 0 && direct.samples_avail() == 0);
}

/// Reset clears stale PPU status and the buffered PPUDATA read.
static void check_ppu_reset() {
    NES::PPU ppu;
    NES::PictureBus bus;
    ppu.reset();
    assert(ppu.get_status() == 0);
    bus.write(0x2000, 0x55);
    ppu.set_data_address(0x20);
    ppu.set_data_address(0x00);
    assert(ppu.get_data(bus) == 0);
    assert(ppu.get_data(bus) == 0x55);
    json_t* stale = json_pack("{s:b,s:i}", "is_sprite_zero_hit", 1, "data_buffer", 0x77);
    ppu.dataFromJson(stale);
    json_decref(stale);
    assert(ppu.get_status() == 0x40);
    ppu.reset();
    assert(ppu.get_status() == 0);
    ppu.set_data_address(0x20);
    ppu.set_data_address(0x00);
    assert(ppu.get_data(bus) == 0);
    assert(ppu.get_data(bus) == 0x55);
}

/// Original CPU program driving all five voices, with looping DMC.
static std::vector<unsigned char> make_audio_image(std::size_t* bank_write_high = nullptr) {
    std::vector<unsigned char> bytes(16 + 0x8000 + 0x8000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = 2; bytes[5] = 4;
    std::size_t pc = 16;
    bytes[pc++] = 0x78;  // SEI
    const auto write_register = [&](int address, int value) {
        bytes[pc++] = 0xA9; bytes[pc++] = value;  // LDA immediate
        bytes[pc++] = 0x8D;  // STA absolute
        bytes[pc++] = address & 0xFF; bytes[pc++] = address >> 8;
    };
    write_register(0x4017, 0x40);
    write_register(0x4015, 0x0F);
    write_register(0x4000, 0xBF); write_register(0x4001, 0x08);
    write_register(0x4002, 0x40); write_register(0x4003, 0x08);
    write_register(0x4004, 0x7A); write_register(0x4005, 0x08);
    write_register(0x4006, 0x80); write_register(0x4007, 0x08);
    write_register(0x4008, 0xFF); write_register(0x400A, 0x60);
    write_register(0x400B, 0x08); write_register(0x400C, 0x3F);
    write_register(0x400E, 0x04); write_register(0x400F, 0x08);
    write_register(0x4010, 0x4F); write_register(0x4011, 0x20);
    write_register(0x4012, 0x00); write_register(0x4013, 0x01);
    write_register(0x4015, 0x1F);
    const int loop = 0x8000 + pc - 16;
    if (bank_write_high) *bank_write_high = pc + 4;
    write_register(0x8000, 3);  // CHR bank selection while DMC reads PRG.
    bytes[pc++] = 0x4C; bytes[pc++] = loop & 0xFF; bytes[pc++] = loop >> 8;
    for (int i = 0; i < 17; ++i) bytes[16 + 0x4000 + i] = 0x55;
    bytes[16 + 0x7FFC] = 0; bytes[16 + 0x7FFD] = 0x80;
    return bytes;
}

/// Compare all five voices on NROM, CNROM, AxROM, MMC2, MMC3, and MMC1.
static void check_graphics_audio_preservation(uint64_t blip_clock) {
    const char* path = ".build/mapper-audio.nes";
    std::size_t bank_write_high = 0;
    auto bytes = make_audio_image(&bank_write_high);
    uint64_t fingerprint = 14695981039346656037ULL;
    for (int rate : {44100, 48000, 96000, 192000}) {
        std::unique_ptr<NES::Emulator> emulators[6];
        for (int variant = 0; variant < 6; ++variant) {
            bytes[6] = variant == 0 ? 0x00 : variant == 1 ? 0x33 : variant == 5 ? 0x10 : 0x70;
            // AxROM uses CHR RAM; each variant executes the same program.
            bytes[5] = variant == 0 || variant == 5 ? 1 : variant == 1 ? 4 : 0;
            std::ofstream file(path, std::ios::binary);
            if (variant == 4) {
                auto image = mmc3_image(4, 8);
                std::copy(bytes.begin() + 16, bytes.begin() + 16 + 0x8000, image.begin() + 16);
                std::fill(image.begin() + 16 + 0x8000, image.end(), 0);
                // Same CPU timing; write MMC3's bank data register, selecting
                // CHR while DMC uses the fixed C000 window.
                image[bank_write_high - 1] = 1;
                file.write(reinterpret_cast<const char*>(image.data()), image.size());
            } else if (variant == 3) {
                auto image = mmc2_image(16, 8);
                // MMC2 switches only the first 8 KiB. Repeat code in banks 0
                // and 3; keep the reference's last three windows fixed.
                for (int bank : {0, 3}) {
                    std::copy(bytes.begin() + 16, bytes.begin() + 16 + 0x2000,
                              image.begin() + 16 + bank * 0x2000);
                    image[bank_write_high + bank * 0x2000] = 0xA0;
                }
                std::copy(bytes.begin() + 16 + 0x2000, bytes.begin() + 16 + 0x8000,
                          image.begin() + 16 + 13 * 0x2000);
                std::fill(image.begin() + 16 + 16 * 0x2000, image.end(), 0);
                file.write(reinterpret_cast<const char*>(image.data()), image.size());
            } else if (variant == 2) {
                // Four identical PRG banks let the CPU switch to bank 3 while
                // retaining the NROM oracle for every instruction/DMC sample.
                auto image = axrom_image(4);
                for (int bank = 0; bank < 4; ++bank)
                    std::copy(bytes.begin() + 16, bytes.begin() + 16 + 0x8000,
                              image.begin() + 16 + bank * 0x8000);
                file.write(reinterpret_cast<const char*>(image.data()), image.size());
            } else {
                file.write(reinterpret_cast<const char*>(bytes.data()),
                           16 + 0x8000 + bytes[5] * 0x2000);
            }
            file.close();
            assert(file.good());
            emulators[variant].reset(new NES::Emulator);
            assert(emulators[variant]->load_game(path));
            emulators[variant]->set_sample_rate(rate);
            emulators[variant]->set_clock_rate(blip_clock);
        }
        int nonzero[5] = {};
        int frames[6] = {};
        for (int sample = 0; sample < 2000; ++sample) {
            for (int variant = 0; variant < 6; ++variant)
                for (int cycle = 0; cycle < NES::CLOCK_RATE / double(rate); ++cycle)
                    emulators[variant]->cycle([&]() { ++frames[variant]; });
            for (int channel = 0; channel < 5; ++channel) {
                const int16_t value = emulators[0]->get_audio_sample(channel);
                assert(value == emulators[1]->get_audio_sample(channel));
                assert(value == emulators[2]->get_audio_sample(channel));
                assert(value == emulators[3]->get_audio_sample(channel));
                assert(value == emulators[4]->get_audio_sample(channel));
                assert(value == emulators[5]->get_audio_sample(channel));
                if (value != 0) ++nonzero[channel];
                fingerprint ^= static_cast<uint16_t>(value);
                fingerprint *= 1099511628211ULL;
            }
        }
        json_t* mmc2_state = emulators[3]->dataToJson();
        assert(json_integer_value(json_object_get(json_object_get(json_object_get(
            mmc2_state, "cartridge"), "mapper"), "register_prg")) == 3);
        json_decref(mmc2_state);
        for (int count : nonzero) assert(count > 0);
        assert(frames[0] == frames[1] && frames[0] == frames[2] && frames[0] == frames[3] && frames[0] == frames[4] && frames[0] == frames[5]);
    }
    assert(std::remove(path) == 0);
    std::printf("NROM/CNROM/AxROM/MMC2/MMC3/MMC1 PCM at Blip clock %llu: %llx\n",
                static_cast<unsigned long long>(blip_clock),
                static_cast<unsigned long long>(fingerprint));
    std::fflush(stdout);
}

/// Adapted from nes-py's cartridge and CNROM tests at 301da52f7f75de38.
/// See docs/licenses/THIRD-PARTY.txt for provenance and the MIT notice.
static void check_upstream_graphics_fixes() {
    const char* path = ".build/mapper-graphics.nes";
    std::vector<char> bytes(16 + 0x8000 + 0x2000, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = 2; bytes[5] = 1;
    const auto write_rom = [&]() {
        std::ofstream file(path, std::ios::binary);
        file.write(bytes.data(), bytes.size());
        file.close();
        assert(file.good());
    };
    for (int mapper : {0, 2, 3}) {
        for (int flags : {0, 1, 2, 3, 8, 9, 10, 11}) {
            bytes[6] = (mapper << 4) | flags;
            write_rom();
            std::unique_ptr<NES::Cartridge> cartridge(NES::Cartridge::create(path, []() {}));
            const auto expected = flags & 8 ? NES::FOUR_SCREEN :
                (flags & 1 ? NES::VERTICAL : NES::HORIZONTAL);
            assert(cartridge->get_mapper()->getNameTableMirroring() == expected);
            // Four-screen metadata alone does not implement four-screen VRAM.
            if (flags & 8) continue;
            NES::PictureBus bus;
            bus.set_mapper(cartridge->get_mapper());
            bus.write(0x2000, 0x11);
            bus.write(flags & 1 ? 0x2400 : 0x2800, 0x22);
            assert(bus.read(0x2000) == 0x11);
            assert(bus.read(0x2400) == (flags & 1 ? 0x22 : 0x11));
            assert(bus.read(0x2800) == (flags & 1 ? 0x11 : 0x22));
            assert(bus.read(0x2C00) == 0x22);
        }
    }
    bytes[6] = 0x30;
    for (int banks : {0, 1, 2, 4}) {
        bytes[5] = banks;
        bytes.resize(16 + 0x8000 + banks * 0x2000);
        for (int i = 0; i < 0x8000; ++i) bytes[16 + i] = 0x10 + i / 0x4000;
        for (int i = 0; i < banks * 0x2000; ++i)
            bytes[16 + 0x8000 + i] = 0x40 + i / 0x2000;
        write_rom();
        std::unique_ptr<NES::Cartridge> cartridge(NES::Cartridge::create(path, []() {}));
        auto* mapper = cartridge->get_mapper();
        for (int value = 0; value < 256; ++value) {
            mapper->writePRG(0x8000, value);
            const int expected = banks ? 0x40 + (value & 3) % banks : 0;
            assert(mapper->readCHR(0) == expected);
            assert(mapper->readCHR(0x1FFF) == expected);
            mapper->writeCHR(0, 0xFF);
            assert(mapper->readCHR(0) == expected);
            // CHR selection must not change CPU/DMC-visible PRG windows.
            assert(mapper->readPRG(0x8000) == 0x10);
            assert(mapper->readPRG(0xBFFF) == 0x10);
            assert(mapper->readPRG(0xC000) == 0x11);
            assert(mapper->readPRG(0xFFFF) == 0x11);
            json_t* saved = mapper->dataToJson();
            assert(json_integer_value(json_object_get(saved, "select_chr")) == (value & 3));
            mapper->writePRG(0x8000, 0);
            mapper->dataFromJson(saved);
            assert(mapper->readCHR(0) == expected);
            json_decref(saved);
        }
        json_t* legacy = json_pack("{s:i}", "select_chr", 65535);
        mapper->dataFromJson(legacy);
        assert(mapper->readCHR(0x1FFF) == (banks ? 0x40 + 65535 % banks : 0));
        json_decref(legacy);
    }
    assert(std::remove(path) == 0);
}

#include "mmc1_host.hpp"
#include "replay.hpp"

int main(int argc, char** argv) {
    if (argc == 7 && std::string(argv[1]) == "--replay") return replay_game(argv, false);
    if (argc == 7 && std::string(argv[1]) == "--replay-roundtrip") return replay_game(argv, true);
    // Allow isolated audio characterization; the default Make target runs it too.
    if (argc == 2 && std::string(argv[1]) == "--audio-only") {
        check_graphics_audio_preservation(NES::CLOCK_RATE);
        check_graphics_audio_preservation(768000);
        return 0;
    }
    if (argc == 5 && std::string(argv[1]) == "--sram-interop") {
        check_sram_interop(argv[2], argv[3], argv[4]);
        return 0;
    }
    assert(argc == 1);
    check_controller_state();
    check_blip_sample_reads();
    check_ppu_reset();
    check_ntsc_palette_range();
    check_mmc1_chr_banks();
    check_mmc1_work_ram();
    check_mapper_headers();
    check_upstream_graphics_fixes();
    check_axrom();
    check_axrom_dmc();
    check_mmc3_banks();
    check_mmc3_irq();
    check_irq_sources();
    check_mmc2_mapping();
    check_mmc2_ppu();
    check_mmc2_cpu();
    Context context;
    context.engine = new engine::Engine;
    context.event = new widget::EventState;
    contextSet(&context);
    check_sram_domains();
    check_sram_mailbox();
    check_sram_files_and_module();
    check_sram_menus();
    check_axrom_module();
    check_mmc2_state();
    check_mmc3_emulator();
    check_mmc1_host();
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
    std::puts("RackNES: controller state, mapper headers/CHR/AxROM/MMC2/MMC3, IRQs, MMC1 RAM/host, palette/NTSC, snapshots, failed loads, and RAM bounds passed");
}

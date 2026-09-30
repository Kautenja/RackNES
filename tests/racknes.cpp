// Focused snapshot, cartridge header, controller, and expander regressions.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
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

/// Exercise all five voices through CPU bus writes on NROM and CNROM.
static void check_graphics_audio_preservation(uint64_t blip_clock) {
    const char* path = ".build/mapper-audio.nes";
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
    write_register(0x8000, 3);  // CHR bank selection while DMC reads PRG.
    bytes[pc++] = 0x4C; bytes[pc++] = loop & 0xFF; bytes[pc++] = loop >> 8;
    for (int i = 0; i < 17; ++i) bytes[16 + 0x4000 + i] = 0x55;
    bytes[16 + 0x7FFC] = 0; bytes[16 + 0x7FFD] = 0x80;
    uint64_t fingerprint = 14695981039346656037ULL;
    for (int rate : {44100, 48000, 96000, 192000}) {
        std::unique_ptr<NES::Emulator> emulators[2];
        for (int variant = 0; variant < 2; ++variant) {
            bytes[6] = variant == 0 ? 0x00 : 0x33;
            // NROM has one CHR bank; CNROM has four. PRG is identical.
            bytes[5] = variant == 0 ? 1 : 4;
            std::ofstream file(path, std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data()),
                       16 + 0x8000 + bytes[5] * 0x2000);
            file.close();
            assert(file.good());
            emulators[variant].reset(new NES::Emulator);
            assert(emulators[variant]->load_game(path));
            emulators[variant]->set_sample_rate(rate);
            emulators[variant]->set_clock_rate(blip_clock);
        }
        int nonzero[5] = {};
        int frames[2] = {};
        for (int sample = 0; sample < 2000; ++sample) {
            for (int variant = 0; variant < 2; ++variant)
                for (int cycle = 0; cycle < NES::CLOCK_RATE / double(rate); ++cycle)
                    emulators[variant]->cycle([&]() { ++frames[variant]; });
            for (int channel = 0; channel < 5; ++channel) {
                const int16_t value = emulators[0]->get_audio_sample(channel);
                assert(value == emulators[1]->get_audio_sample(channel));
                if (value != 0) ++nonzero[channel];
                fingerprint ^= static_cast<uint16_t>(value);
                fingerprint *= 1099511628211ULL;
            }
        }
        for (int count : nonzero) assert(count > 0);
        assert(frames[0] == frames[1]);
    }
    assert(std::remove(path) == 0);
    std::printf("NROM/CNROM PCM at Blip clock %llu: %llx\n",
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

int main(int argc, char** argv) {
    // Allow isolated audio characterization; the default Make target runs it too.
    if (argc == 2 && std::string(argv[1]) == "--audio-only") {
        check_graphics_audio_preservation(NES::CLOCK_RATE);
        check_graphics_audio_preservation(768000);
        return 0;
    }
    assert(argc == 1);
    check_controller_state();
    check_blip_sample_reads();
    check_ppu_reset();
    check_mmc1_chr_banks();
    check_mapper_headers();
    check_upstream_graphics_fixes();
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

// Deterministic SRAM domain, mailbox, file, and module lifecycle checks.
#ifndef RACKNES_TEST_SRAM_HPP
#define RACKNES_TEST_SRAM_HPP

#include <thread>
#ifndef _WIN32
#include <csignal>
#include <sys/resource.h>
#endif

static std::vector<unsigned char> sram_image(int mapper = 1, bool nes2 = false) {
    std::vector<unsigned char> image(16 + 0x8000 + (mapper == 2 ? 0 : 0x2000));
    image[0] = 'N'; image[1] = 'E'; image[2] = 'S'; image[3] = 0x1A;
    image[4] = 2; image[5] = mapper == 2 ? 0 : 1;
    image[6] = (mapper << 4) | 2;
    image[7] = nes2 ? 8 : 0;
    image[8] = nes2 ? 0 : 1;
    image[10] = nes2 ? 0x70 : 0;
    image[11] = nes2 && mapper == 2 ? 7 : 0;
    image[16] = 0x4C; image[17] = 0; image[18] = 0x80;  // JMP $8000
    image[16 + 0x7FFC] = 0; image[16 + 0x7FFD] = 0x80;
    return image;
}

static void sram_write_fixture(const std::string& path,
                               const std::vector<unsigned char>& bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close();
    assert(file.good());
}

static void check_sram_domains() {
    const char* path = ".build/sram-domain.nes";
    std::array<NES::NES_Byte, SRAMTransfer::SIZE> pattern, actual;
    for (std::size_t i = 0; i < pattern.size(); ++i) pattern[i] = (i * 29 + i / 256) & 255;
    for (int mapper = 0; mapper <= 3; ++mapper) {
        for (bool nes2 : {false, true}) {
            const auto image = sram_image(mapper, nes2);
            sram_write_fixture(path, image);
            NES::Emulator emulator;
            assert(emulator.load_game(path));
            assert(emulator.persistent_size() == pattern.size());
            emulator.set_controllers(0x53, 0xA7);
            for (int i = 0; i < 100; ++i) emulator.cycle([] {});
            json_t* before = emulator.dataToJson();
            assert(!emulator.import_sram(pattern.data(), pattern.size() - 1));
            assert(!emulator.import_sram(pattern.data(), pattern.size() + 1));
            assert(emulator.import_sram(pattern.data(), pattern.size()));
            assert(emulator.export_sram(actual.data(), actual.size()));
            assert(actual == pattern);
            json_t* after = emulator.dataToJson();
            // CPU, PPU, APU, controller, mapper and all other JSON is unchanged.
            json_object_set(json_object_get(after, "bus"), "extended_ram",
                json_object_get(json_object_get(before, "bus"), "extended_ram"));
            assert(json_equal(before, after));
            json_decref(before); json_decref(after);
            // Existing patches continue to use bus.extended_ram, with no new key.
            json_t* legacy = emulator.dataToJson();
            NES::Emulator restored;
            assert(restored.dataFromJson(legacy));
            assert(restored.export_sram(actual.data(), actual.size()));
            assert(actual == pattern);
            // Invalid RAM payload must never resize the CPU-visible window.
            for (json_t* bad : {json_integer(42), json_string(""), json_string("AA==")}) {
                json_object_set(json_object_get(legacy, "bus"), "extended_ram", bad);
                assert(restored.dataFromJson(legacy));
                assert(restored.export_sram(actual.data(), actual.size()));
                assert(std::all_of(actual.begin(), actual.end(), [](unsigned char b) { return b == 0; }));
                json_decref(bad);
            }
            json_decref(legacy);
            // Ambiguous, mixed, volatile-only, extended, and unsupported domains.
            for (int variant = 0; variant < 9; ++variant) {
                auto bad = image;
                if (variant == 0) bad[6] &= ~2;       // no battery
                if (variant == 1) bad[8] = nes2 ? 0x10 : 0;
                if (variant == 2) bad[8] = nes2 ? 0x10 : 2;
                if (variant == 3) bad[6] |= 4;        // trainer
                if (variant == 4) bad[6] |= 8;        // four-screen
                if (variant == 5) bad[10] = nes2 ? 0x77 : 1;
                if (variant == 6) bad[11] = 0x70;     // CHR NVRAM / dirty header
                if (variant == 7) bad[12] = 1;
                if (variant == 8) bad.push_back(0);   // unknown trailing domain
                sram_write_fixture(path, bad);
                assert(emulator.load_game(path));
                assert(emulator.persistent_size() == 0);
                assert(!emulator.import_sram(pattern.data(), pattern.size()));
                assert(!emulator.export_sram(actual.data(), actual.size()));
            }
        }
    }
    std::remove(path);
}

static void check_sram_mailbox() {
    const char* path = ".build/sram-mailbox.nes";
    sram_write_fixture(path, sram_image());
    NES::Emulator emulator;
    SRAMTransfer transfer;
    assert(!transfer.begin(true));
    assert(emulator.load_game(path));
    transfer.publish(emulator);
    assert(transfer.begin(true));
    assert(!transfer.begin(false));
    transfer.buffer().fill(0xA5);
    transfer.process(emulator);  // Preparing is not visible to the engine.
    assert(!transfer.ready());
    transfer.submit();
    transfer.process(emulator);
    assert(transfer.ready() && transfer.outcome() == SRAMTransfer::Result::Success);
    assert(!transfer.begin(false));  // Done needs an explicit acknowledgement.
    transfer.release();
    for (int event = 0; event < 4; ++event) {
        assert(transfer.begin(true));
        transfer.buffer().fill(0xDE);
        transfer.submit();
        if (event == 0) emulator.reset();
        if (event == 1) assert(emulator.load_game(path));
        if (event == 2) {
            json_t* state = emulator.dataToJson();
            assert(emulator.dataFromJson(state));
            json_decref(state);
        }
        if (event == 3) emulator.remove_game();
        json_t* before = emulator.dataToJson();
        transfer.process(emulator);
        assert(transfer.ready() && transfer.outcome() == SRAMTransfer::Result::Stale);
        json_t* after = emulator.dataToJson();
        assert(json_equal(before, after));
        json_decref(before); json_decref(after);
        transfer.release();
    }
    assert(emulator.load_game(path));
    transfer.publish(emulator);
    std::atomic<bool> finished{false};
    // The actual ownership protocol under concurrent engine/UI scheduling.
    std::thread engine([&] {
        while (!finished.load(std::memory_order_acquire)) transfer.process(emulator);
    });
    for (int i = 0; i < 1000; ++i) {
        assert(transfer.begin(true));
        transfer.buffer().fill(i & 255);
        transfer.submit();
        while (!transfer.ready()) std::this_thread::yield();
        assert(transfer.outcome() == SRAMTransfer::Result::Success);
        transfer.release();
        assert(transfer.begin(false));
        transfer.submit();
        while (!transfer.ready()) std::this_thread::yield();
        assert(transfer.outcome() == SRAMTransfer::Result::Success);
        for (auto byte : transfer.buffer()) assert(byte == (i & 255));
        transfer.release();
    }
    finished.store(true, std::memory_order_release);
    engine.join();
    transfer.close();
    assert(!transfer.begin(true));
    std::remove(path);
}

static void check_sram_files_and_module() {
    const char* rom = ".build/sram-module.nes";
    const char* file = ".build/sram-test.sav";
    sram_write_fixture(rom, sram_image());
    std::unique_ptr<RackNES> module(new RackNES);
    module->rom_path_signal = rom;
    module->handleNewROM();
    module->rom_path_signal.clear();
    auto transfer = module->sram;
    module->params[RackNES::PARAM_HANG].setValue(1);
    module->processCV();
    module->backup = module->emulator.dataToJson();
    json_t* backup = json_deep_copy(module->backup);
    std::vector<unsigned char> pattern(SRAMTransfer::SIZE);
    for (std::size_t i = 0; i < pattern.size(); ++i) pattern[i] = i * 13 + i / 256;
    sram_write_fixture(file, pattern);
    assert(transfer->begin(true));
    assert(SRAMFiles::read(file, *transfer));
    transfer->submit();
    Module::ProcessArgs args{};
    args.sampleRate = 48000; args.sampleTime = 1.f / args.sampleRate;
    module->process(args);
    assert(transfer->ready() && transfer->outcome() == SRAMTransfer::Result::Success);
    assert(json_equal(backup, module->backup));
    transfer->release();
    assert(transfer->begin(false));
    transfer->submit();
    module->process(args);
    assert(transfer->ready() && transfer->outcome() == SRAMTransfer::Result::Success);
    assert(std::equal(pattern.begin(), pattern.end(), transfer->buffer().begin()));
    assert(SRAMFiles::write(file, *transfer));  // atomic overwrite
#ifndef _WIN32
    // Force a partial write, preserving the old regular-file destination.
    struct rlimit previous, limited;
    assert(getrlimit(RLIMIT_FSIZE, &previous) == 0);
    limited = previous;
    limited.rlim_cur = 4096;
    const auto previous_handler = std::signal(SIGXFSZ, SIG_IGN);
    assert(setrlimit(RLIMIT_FSIZE, &limited) == 0);
    transfer->buffer().fill(0xEC);
    assert(!SRAMFiles::write(file, *transfer));
    assert(setrlimit(RLIMIT_FSIZE, &previous) == 0);
    std::signal(SIGXFSZ, previous_handler);
    assert(SRAMFiles::read(file, *transfer));
    assert(std::equal(pattern.begin(), pattern.end(), transfer->buffer().begin()));
#endif
    assert(!SRAMFiles::write(".build/nonexistent-parent/save.sav", *transfer));
    assert(!SRAMFiles::write(".build", *transfer));  // replacement failure
    assert(SRAMFiles::read(file, *transfer));
    assert(std::equal(pattern.begin(), pattern.end(), transfer->buffer().begin()));
    // Generation change after snapshot cancels before replacing destination.
    module->onReset();
    transfer->buffer().fill(0xEE);
    assert(!SRAMFiles::write(file, *transfer));
    assert(SRAMFiles::read(file, *transfer));
    assert(std::equal(pattern.begin(), pattern.end(), transfer->buffer().begin()));
    transfer->release();
    module->rom_path_signal = rom; module->handleNewROM();
    module->rom_path_signal.clear();
    for (std::size_t size : {std::size_t(0), std::size_t(8191), std::size_t(8193)}) {
        assert(transfer->begin(true));
        sram_write_fixture(file, std::vector<unsigned char>(size, 7));
        json_t* before = module->dataToJson();
        assert(!SRAMFiles::read(file, *transfer));
        assert(!SRAMFiles::read(".build/absent-sram-file", *transfer));
        transfer->release();
        module->process(args);
        json_t* after = module->dataToJson();
        assert(json_equal(before, after));
        json_decref(before); json_decref(after);
    }
    // LOAD restores old SRAM without changing the saved snapshot itself.
    module->backup = module->emulator.dataToJson();
    assert(transfer->begin(true));
    transfer->buffer().fill(0xE5);
    transfer->submit(); module->process(args); transfer->release();
    module->params[RackNES::PARAM_LOAD].setValue(1);
    module->processCV();
    std::array<NES::NES_Byte, SRAMTransfer::SIZE> restored;
    assert(module->emulator.export_sram(restored.data(), restored.size()));
    for (auto byte : restored) assert(byte == 0);
    module->params[RackNES::PARAM_LOAD].setValue(0);
    module->processCV();
    // Patch restore also cancels queued work.
    module->dataFromJson(backup);  // Empty module-level JSON still invalidates.
    assert(transfer->begin(true));
    transfer->submit();
    json_t* patch = module->dataToJson();
    module->dataFromJson(patch);
    json_decref(patch);
    module->process(args);
    assert(transfer->ready() && transfer->outcome() == SRAMTransfer::Result::Stale);
    transfer->release();
    assert(transfer->begin(false));
    transfer->submit();
    module.reset();  // UI service survives, without any dangling module pointer.
    assert(!transfer->current() && !transfer->supported());
    assert(!SRAMFiles::write(file, *transfer));
    json_decref(backup);
    for (bool done : {false, true}) {
        module.reset(new RackNES);
        module->rom_path_signal = rom; module->handleNewROM();
        transfer = module->sram;
        assert(transfer->begin(false));
        if (done) { transfer->submit(); transfer->process(module->emulator); }
        module.reset();  // Destroy during a dialog or after acknowledgement.
        assert(!transfer->current());
        assert(!SRAMFiles::write(file, *transfer));
    }
    std::remove(file); std::remove(rom);
}

static void check_sram_menus() {
    for (int variant = 0; variant < 3; ++variant) {
        ui::Menu menu;
        std::shared_ptr<SRAMTransfer> transfer;
        NES::Emulator emulator;
        if (variant) transfer = std::make_shared<SRAMTransfer>();
        if (variant == 2) {
            sram_write_fixture(".build/sram-menu.nes", sram_image());
            assert(emulator.load_game(".build/sram-menu.nes"));
            transfer->publish(emulator);
        }
        appendSRAMMenu(&menu, transfer);
        unsigned count = 0;
        for (auto* child : menu.children) {
            auto* item = dynamic_cast<SRAMMenuItem*>(child);
            if (!item) continue;
            assert(item->disabled == (variant != 2));
            assert(item->text == (count == 0 ? "Import SRAM..." : "Export SRAM..."));
            ++count;
        }
        assert(count == 2);
    }
    std::remove(".build/sram-menu.nes");
}

/// Optional independent-core interchange driver used by sram_interop.py.
static void check_sram_interop(const char* rom, const char* input, const char* output) {
    NES::Emulator emulator;
    assert(emulator.load_game(rom));
    SRAMTransfer transfer;
    transfer.publish(emulator);
    assert(transfer.begin(true));
    assert(SRAMFiles::read(input, transfer));
    const auto imported = transfer.buffer();
    transfer.submit(); transfer.process(emulator);
    assert(transfer.ready() && transfer.outcome() == SRAMTransfer::Result::Success);
    transfer.release();
    for (int i = 0; i < 10000; ++i) emulator.cycle([] {});
    for (int i = 0; i < 16; ++i) assert(emulator.get_memory_buffer()[0x10 + i] == imported[i]);
    assert(transfer.begin(false));
    transfer.submit(); transfer.process(emulator);
    assert(transfer.ready() && transfer.outcome() == SRAMTransfer::Result::Success);
    assert(transfer.buffer() == imported);
    assert(SRAMFiles::write(output, transfer));
    transfer.release();
}

#endif  // RACKNES_TEST_SRAM_HPP

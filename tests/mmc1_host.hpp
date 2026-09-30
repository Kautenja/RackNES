// MMC1 host lifecycle, routing, and clock regressions for spec 005.
#ifndef RACKNES_TESTS_MMC1_HOST_HPP
#define RACKNES_TESTS_MMC1_HOST_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

/// Run the original five-voice fixture through the actual Rack module.
static void check_mmc1_host() {
    const char* path = ".build/mmc1-host.nes";
    auto bytes = make_audio_image();
    bytes[6] = 0x10;
    bytes[5] = 1;
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), 16 + 0x8000 + 0x2000);
    file.close();
    assert(file.good());
    std::unique_ptr<RackNES> module(new RackNES);
    assert(module->emulator.load_game(path));
    Module::ProcessArgs args = {};
    module->params[RackNES::PARAM_MIX].setValue(0.7f);
    int nonzero[5] = {};
    // Keep one running module across actual host sample-rate notifications.
    for (int rate : {44100, 48000}) {
        APP->engine->setSampleRate(rate);
        module->onSampleRateChange();
        args.sampleRate = rate;
        args.sampleTime = 1.f / rate;
        for (int clock : {-4, 0, 4}) {
            module->params[RackNES::PARAM_CLOCK].setValue(clock);
            assert(module->getClockSpeed() ==
                   static_cast<uint64_t>(NES::CLOCK_RATE * std::pow(2.f, clock)));
            for (int sample = 0; sample < 2000; ++sample) {
                // Cover every connection combination, including all and none.
                const int connections = (sample / 32) & 31;
                for (int channel = 0; channel < 5; ++channel)
                    module->outputs[RackNES::OUTPUT_CH + channel].channels =
                        (connections >> channel) & 1;
                module->process(args);
                float expected = 0.f;
                for (int channel = 0; channel < 5; ++channel) {
                    const float value = module->outputs[RackNES::OUTPUT_CH + channel].getVoltage();
                    assert(std::isfinite(value));
                    if (value != 0.f) ++nonzero[channel];
                    if (!(connections & (1 << channel))) expected += value;
                }
                assert(std::fabs(module->outputs[RackNES::OUTPUT_MIX].getVoltage() -
                                 0.7f * expected) < 1e-5f);
                const float frame_clock = module->outputs[RackNES::OUTPUT_CLOCK].getVoltage();
                assert(frame_clock == 0.f || frame_clock == 10.f);
            }
        }
    }
    for (int count : nonzero) assert(count > 0);
    module->params[RackNES::PARAM_CLOCK].setValue(0.f);
    module->params[RackNES::PARAM_CLOCK_ATT].setValue(1.f);
    module->inputs[RackNES::INPUT_CLOCK].channels = 1;
    for (float voltage : {-10.f, 0.f, 10.f}) {
        module->inputs[RackNES::INPUT_CLOCK].setVoltage(voltage);
        assert(module->getClockSpeed() == static_cast<uint64_t>(
            NES::CLOCK_RATE * std::pow(2.f, voltage / 5.f)));
        for (int sample = 0; sample < 128; ++sample) module->process(args);
    }
    module->inputs[RackNES::INPUT_CLOCK].channels = 0;
    module->params[RackNES::PARAM_CLOCK_ATT].setValue(0.f);

    // Hang freezes CPU/PPU state and output voltages while controls still run.
    module->params[RackNES::PARAM_HANG].setValue(1.f);
    module->processCV();
    json_t* held = module->emulator.dataToJson();
    float voltages[RackNES::NUM_OUTPUTS];
    for (int port = 0; port < RackNES::NUM_OUTPUTS; ++port)
        voltages[port] = module->outputs[port].getVoltage();
    for (int sample = 0; sample < 64; ++sample) module->process(args);
    json_t* after = module->emulator.dataToJson();
    for (const char* part : {"cpu", "ppu", "bus"})
        assert(json_equal(json_object_get(held, part), json_object_get(after, part)));
    for (int port = 0; port < RackNES::NUM_OUTPUTS; ++port)
        assert(voltages[port] == module->outputs[port].getVoltage());
    json_decref(held); json_decref(after);
    module->params[RackNES::PARAM_HANG].setValue(0.f);
    module->processCV();

    // Simultaneous SAVE, RESET, LOAD restores the pre-reset machine.
    module->emulator.get_memory_buffer()[0x10] = 0xA5;
    json_t* before = module->emulator.dataToJson();
    for (int param : {RackNES::PARAM_SAVE, RackNES::PARAM_RESET, RackNES::PARAM_LOAD})
        module->params[param].setValue(1.f);
    module->processCV();
    after = module->emulator.dataToJson();
    for (const char* part : {"cartridge", "cpu", "ppu", "bus", "picture_bus"})
        assert(json_equal(json_object_get(before, part), json_object_get(after, part)));
    json_decref(after);
    for (int param : {RackNES::PARAM_SAVE, RackNES::PARAM_RESET, RackNES::PARAM_LOAD})
        module->params[param].setValue(0.f);
    module->processCV();
    module->emulator.get_memory_buffer()[0x10] = 0x5A;
    json_t* patch = module->dataToJson();
    std::unique_ptr<RackNES> restored(new RackNES);
    restored->dataFromJson(patch);
    assert(!restored->rom_reload_failed_signal && restored->backup);
    assert(restored->emulator.get_memory_buffer()[0x10] == 0x5A);
    restored->processCV();
    restored->params[RackNES::PARAM_LOAD].setValue(1.f);
    restored->processCV();
    assert(restored->emulator.get_memory_buffer()[0x10] == 0xA5);
    after = restored->emulator.dataToJson();
    for (const char* part : {"cartridge", "cpu", "ppu", "bus", "picture_bus"})
        assert(json_equal(json_object_get(before, part), json_object_get(after, part)));
    json_decref(after); json_decref(before); json_decref(patch);
    // Snapshot audio is not sample-exact; require continued sound on every voice.
    int resumed[5] = {};
    for (int sample = 0; sample < 2000; ++sample) {
        restored->process(args);
        for (int channel = 0; channel < 5; ++channel) {
            const float value = restored->outputs[RackNES::OUTPUT_CH + channel].getVoltage();
            assert(std::isfinite(value));
            if (value != 0.f) ++resumed[channel];
        }
    }
    for (int count : resumed) assert(count > 0);

    // APU register reconstruction can notify a changed IRQ schedule. It must
    // not service an interrupt while the saved CPU and stack are being restored.
    json_t* irq_state = restored->emulator.dataToJson();
    json_t* cpu_state = json_object_get(irq_state, "cpu");
    json_object_set_new(cpu_state, "flags", json_integer(0));
    json_t* apu_state = json_object_get(json_object_get(irq_state, "apu"), "apu");
    json_object_set_new(apu_state, "w4017", json_integer(0xC0));
    assert(restored->emulator.dataFromJson(irq_state));
    after = restored->emulator.dataToJson();
    assert(json_equal(cpu_state, json_object_get(after, "cpu")));
    assert(json_equal(json_object_get(irq_state, "bus"), json_object_get(after, "bus")));
    json_decref(after); json_decref(irq_state);

    // Missing replacement preserves the active machine, displayed frame, backup.
    before = restored->emulator.dataToJson();
    json_t* backup = restored->backup;
    std::vector<uint8_t> screen(restored->screen,
                                restored->screen + NES::Emulator::SCREEN_BYTES);
    restored->rom_path_signal = ".build/no-such-mmc1-game.nes";
    restored->handleNewROM();
    assert(restored->rom_load_failed_signal && restored->backup == backup);
    after = restored->emulator.dataToJson();
    assert(json_equal(before, after));
    assert(std::equal(screen.begin(), screen.end(), restored->screen));
    json_decref(before); json_decref(after);
    // Successful replacement starts a fresh cartridge and clears the backup.
    restored->rom_path_signal = path;
    restored->handleNewROM();
    assert(restored->emulator.has_game() && restored->backup == nullptr);
    after = restored->emulator.dataToJson();
    const auto work_ram = base64_decode(json_string_value(
        json_object_get(json_object_get(after, "bus"), "extended_ram")));
    assert(work_ram == std::string(0x2000, '\0'));
    json_decref(after);
    restored->onReset();
    assert(!restored->emulator.has_game() && restored->backup == nullptr);
    for (uint8_t pixel : restored->screen) assert(pixel == 0);
    assert(std::remove(path) == 0);
}

#endif  // RACKNES_TESTS_MMC1_HOST_HPP

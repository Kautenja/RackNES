// Optional, headless game evidence capture. No game assets are distributed.
#ifndef RACKNES_TESTS_REPLAY_HPP
#define RACKNES_TESTS_REPLAY_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "../src/nes/emulator.hpp"

/// Replay frame-indexed controller bytes; export paired raw/filtered frames.
/// Arguments: ROM, output prefix, sample rate, frame count, input script.
/// Script lines: completed-frame-count player1-byte player2-byte (decimal).
static int replay_game(char** argv, bool roundtrip) {
    const int rate = std::stoi(argv[4]);
    const int limit = std::stoi(argv[5]);
    assert((rate == 44100 || rate == 48000) && limit > 0 && limit <= 36000);
    struct Input { int frame, p1, p2; };
    std::vector<Input> inputs;
    std::ifstream script(argv[6]);
    assert(script.good());
    Input input;
    while (script >> input.frame >> input.p1 >> input.p2) {
        assert(input.frame >= 0 && input.p1 >= 0 && input.p1 <= 255 &&
               input.p2 >= 0 && input.p2 <= 255);
        assert(inputs.empty() || input.frame > inputs.back().frame);
        inputs.push_back(input);
    }
    assert(script.eof());
    assert(!roundtrip || limit > 1260);
    std::unique_ptr<NES::Emulator> emulator(new NES::Emulator);
    assert(emulator->load_game(argv[2]));
    emulator->set_sample_rate(rate);
    emulator->set_clock_rate(768000);
    const std::string prefix = argv[3];
    std::ofstream audio(prefix + ".s16le", std::ios::binary);
    assert(audio.good());
    int frame = 0;
    bool started = false;
    std::size_t next_input = 0;
    uint64_t samples = 0;
    json_t* saved = nullptr;
    std::unique_ptr<nes_ntsc_t> filter(new nes_ntsc_t);
    nes_ntsc_init(filter.get(), &nes_ntsc_composite);
    std::vector<NES::NES_Pixel> expected(NES::Emulator::PIXELS);
    while (frame < limit) {
        while (next_input < inputs.size() && inputs[next_input].frame == frame) {
            emulator->set_controllers(inputs[next_input].p1, inputs[next_input].p2);
            ++next_input;
        }
        for (int cycle = 0; cycle < NES::CLOCK_RATE / double(rate); ++cycle) {
            emulator->cycle([]() {});
            if (!emulator->is_video_frame_complete()) {
                started = true;
                continue;
            }
            if (!started) continue;  // Reset begins on the pre-render line too.
            ++frame;
            if (frame % 60 == 0 || frame == limit) {
                const auto* indices = emulator->get_palette_buffer();
                for (int i = 0; i < 256 * 240; ++i) assert(indices[i] < 64);
                bool matching_filter = false;
                for (int phase : {0, 1}) {
                    nes_ntsc_blit(filter.get(), indices, 256, phase, 256, 240,
                                  expected.data(), NES::NTSC_PITCH);
                    matching_filter |= std::equal(expected.begin(), expected.end(),
                                                   emulator->get_screen_buffer());
                }
                assert(matching_filter);
                const auto name = prefix + "-" + std::to_string(frame);
                std::ofstream raw(name + ".pgm", std::ios::binary);
                raw << "P5\n256 240\n63\n";
                raw.write(reinterpret_cast<const char*>(emulator->get_palette_buffer()),
                          256 * 240);
                std::ofstream filtered(name + ".ppm", std::ios::binary);
                filtered << "P6\n" << NES::Emulator::WIDTH << " 240\n255\n";
                const auto* pixels = emulator->get_screen_buffer();
                for (int i = 0; i < NES::Emulator::WIDTH * 240; ++i) {
                    const char rgb[] = {static_cast<char>(pixels[i]),
                        static_cast<char>(pixels[i] >> 8), static_cast<char>(pixels[i] >> 16)};
                    filtered.write(rgb, 3);
                }
                assert(raw.good() && filtered.good());
                json_t* state = emulator->dataToJson();
                assert(json_dump_file(state, (name + ".json").c_str(), JSON_INDENT(2)) == 0);
                json_decref(state);
            }
            if (roundtrip && frame == 1200) saved = emulator->dataToJson();
            if (roundtrip && frame == 1260) {
                assert(saved && emulator->dataFromJson(saved));
                json_t* restored = emulator->dataToJson();
                for (const char* part : {"cartridge", "cpu", "ppu", "bus", "picture_bus"}) {
                    if (!json_equal(json_object_get(saved, part), json_object_get(restored, part))) {
                        std::fprintf(stderr, "Roundtrip mismatch: %s\n", part);
                        json_dump_file(saved, (prefix + "-saved.json").c_str(), JSON_INDENT(2));
                        json_dump_file(restored, (prefix + "-restored.json").c_str(), JSON_INDENT(2));
                        assert(false);
                    }
                }
                json_decref(restored);
            }
            // Apply inputs at the video boundary, before the next CPU cycle.
            while (next_input < inputs.size() && inputs[next_input].frame == frame) {
                emulator->set_controllers(inputs[next_input].p1, inputs[next_input].p2);
                ++next_input;
            }
            if (frame == limit) break;
        }
        for (int channel = 0; channel < 5; ++channel) {
            const uint16_t value = emulator->get_audio_sample(channel);
            const char bytes[] = {static_cast<char>(value), static_cast<char>(value >> 8)};
            audio.write(bytes, 2);
        }
        ++samples;
    }
    json_decref(saved);
    assert(audio.good());
    std::printf("Replay: %d video frames, %llu host samples at %d Hz, five-channel s16le\n",
                frame, static_cast<unsigned long long>(samples), rate);
    return 0;
}

#endif  // RACKNES_TESTS_REPLAY_HPP

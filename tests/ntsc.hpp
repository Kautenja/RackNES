// Palette-range and full-frame NTSC regressions for issue #45.
#ifndef RACKNES_TESTS_NTSC_HPP
#define RACKNES_TESTS_NTSC_HPP

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>

#include "../src/base64.h"
#include "../src/nes/ppu.hpp"

/// Render all 64 colors with each combination of unused palette bits.
static void check_ntsc_palette_range() {
    NES::PictureBus palette_bus;
    for (int value = 0; value < 256; ++value) {
        for (int address = 0; address < 32; ++address) {
            palette_bus.write(0x3F00 + address, value);
            // $3F10 writes the backdrop alias in the existing bus mapping.
            const int slot = address == 0x10 ? 0 : address;
            assert(palette_bus.read_palette(slot) == (value & 0x3F));
        }
    }
    const int width = NES::SCANLINE_VISIBLE_DOTS;
    const int height = NES::VISIBLE_SCANLINES;
    const int output_width = NES::SCANLINE_VISIBLE_DOTS_NTSC;
    std::vector<NES::NES_Byte> input(width * height);
    for (int y = 0; y < height; ++y)
        std::fill_n(input.begin() + y * width, width, y & 0x3F);
    std::unique_ptr<nes_ntsc_t> filter(new nes_ntsc_t);
    nes_ntsc_init(filter.get(), &nes_ntsc_composite);
    std::vector<NES::NES_Pixel> expected(output_width * height);
    nes_ntsc_blit(filter.get(), input.data(), width, 1, width, height,
                  expected.data(), NES::NTSC_PITCH);

    for (int high_bits : {0x00, 0x40, 0x80, 0xC0}) {
        for (bool restored : {false, true}) {
            std::unique_ptr<NES::PPU> ppu(new NES::PPU);
            NES::PictureBus bus;
            ppu->reset();
            // Use the backdrop so the fixture needs no cartridge or CHR data.
            ppu->set_mask(0);
            for (int cycle = 0; cycle <= NES::SCANLINE_END_CYCLE; ++cycle)
                ppu->cycle(bus);
            for (int y = 0; y < height; ++y) {
                const NES::NES_Byte color = (y & 0x3F) | high_bits;
                if (restored) {
                    // Old snapshots may contain unmasked palette bytes.
                    std::vector<NES::NES_Byte> palette(32, color);
                    const auto encoded = base64_encode(palette.data(), palette.size());
                    json_t* state = json_pack("{s:s}", "palette", encoded.c_str());
                    bus.dataFromJson(state);
                    json_decref(state);
                } else {
                    ppu->set_data_address(0x3F);
                    ppu->set_data_address(0x00);
                    ppu->set_data(bus, color);
                }
                for (int cycle = 0; cycle < NES::SCANLINE_END_CYCLE; ++cycle)
                    ppu->cycle(bus);
            }
            // Post-render plus 20 vblank lines publishes the filtered frame.
            for (int cycle = 0; cycle < 21 * NES::SCANLINE_END_CYCLE; ++cycle)
                ppu->cycle(bus);
            if (!std::equal(expected.begin(), expected.end(),
                            ppu->get_screen_buffer())) {
                std::fprintf(stderr, "NTSC mismatch: high bits 0x%02X, restored %d\n",
                             high_bits, restored);
                assert(false);
            }
            for (int address = 0; address < 32; ++address)
                assert(bus.read_palette(address) < nes_ntsc_palette_size);
        }
    }
}

#endif  // RACKNES_TESTS_NTSC_HPP

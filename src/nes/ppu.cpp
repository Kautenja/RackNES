//  Program:      nes-py
//  File:         ppu.cpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//
//  Reset and pattern-fetch structure adapted from nes-py
//  301da52f7f75de38 (MIT).
//  See docs/licenses/THIRD-PARTY.txt for the upstream notice.
//

#include <cstring>
#include "ppu.hpp"

namespace NES {

void PPU::reset() {
    irq_sprite_addresses.fill(0);
    latch_background_valid = false;
    latch_background_address = latch_background_page = 0;
    latch_background_low = latch_background_high = 0;
    latch_sprite_patterns.fill(0);
    is_long_sprites = false;
    is_interrupting = false;
    is_vblank = false;
    is_sprite_zero_hit = false;
    is_showing_background = true;
    is_showing_sprites = true;
    is_hiding_edge_background = false;
    is_hiding_edge_sprites = false;
    is_even_frame = true;
    is_first_write = true;
    background_page = LOW;
    sprite_page = LOW;
    data_address = 0;
    cycles = 0;
    scanline = 0;
    sprite_data_address = 0;
    fine_x_scroll = 0;
    temp_address = 0;
    data_buffer = 0;
    data_address_increment = 1;
    pipeline_state = PRE_RENDER;
    scanline_sprites.reserve(8);
    scanline_sprites.resize(0);
    // setup the NTSC video filter
    nes_ntsc_setup_t setup;
    setup = nes_ntsc_composite;
    nes_ntsc_init(&ntsc, &setup);
}

void PPU::fetch_latched_sprite_patterns(PictureBus& bus) {
    latch_sprite_patterns.fill(0);
    if (!is_showing_background && !is_showing_sprites) return;
    const int length = is_long_sprites ? 16 : 8;
    for (std::size_t slot = 0; slot < scanline_sprites.size() && slot < 8; ++slot) {
        const std::size_t index = scanline_sprites[slot] * 4;
        const NES_Byte tile = sprite_memory[index + 1];
        int row = scanline - sprite_memory[index];
        if (sprite_memory[index + 2] & 0x80) row ^= length - 1;
        NES_Address address;
        if (is_long_sprites) {
            row = (row & 7) | ((row & 8) << 1);
            address = (tile >> 1) * 32 + row;
            address |= (tile & 1) << 12;
        } else {
            address = tile * 16 + row + (sprite_page == HIGH ? 0x1000 : 0);
        }
        latch_sprite_patterns[slot * 2] = bus.read(address);
        latch_sprite_patterns[slot * 2 + 1] = bus.read(address + 8);
    }
}

// Emit the external fetch address by dot, independently of cached pixel reads.
// The renderer remains scanline based; this is not a dot-accurate PPU pipeline.
void PPU::clock_mapper_address(PictureBus& bus) {
    const bool rendering = is_showing_background || is_showing_sprites;
    if (!rendering || (pipeline_state != PRE_RENDER && pipeline_state != RENDER)) {
        bus.clock_address(data_address);
        return;
    }
    if (cycles == 257) {
        // Empty sprite slots still fetch tile $FF. In 8x16 mode its table is high.
        irq_sprite_addresses.fill(is_long_sprites || sprite_page == HIGH ? 0x1FF0 : 0x0FF0);
        const int height = is_long_sprites ? 16 : 8;
        unsigned slot = 0;
        for (unsigned i = 0; i < 64 && slot < 8; ++i) {
            int row = (pipeline_state == PRE_RENDER ? -1 : scanline) - sprite_memory[i * 4];
            if (row < 0 || row >= height) continue;
            const NES_Byte tile = sprite_memory[i * 4 + 1];
            if (sprite_memory[i * 4 + 2] & 0x80) row ^= height - 1;
            irq_sprite_addresses[slot++] = is_long_sprites ?
                ((tile & 1) << 12) + (tile & 0xFE) * 16 + (row & 7) + ((row & 8) << 1) :
                (sprite_page << 12) + tile * 16 + row;
        }
    }
    NES_Address address = 0x2000;  // Nametable/attribute/dummy fetch: A12 low.
    if (cycles >= 1 && cycles <= 336 && ((cycles - 1) & 7) >= 4) {
        if (cycles >= 257 && cycles <= 320)
            address = irq_sprite_addresses[(cycles - 257) / 8];
        else
            address = background_page << 12;
    }
    bus.clock_address(address);
}

void PPU::cycle(PictureBus& bus) {
    const bool address_fetches = bus.observesPPUAddresses();
    if (address_fetches) clock_mapper_address(bus);
    const bool latch_fetches = bus.hasCHRReadLatches() || address_fetches;
    const bool rendering = is_showing_background || is_showing_sprites;
    const bool scroll_enabled = latch_fetches ? rendering :
        (is_showing_background && is_showing_sprites);
    switch (pipeline_state) {
        case PRE_RENDER: {
            if (cycles == 1)
                is_vblank = is_sprite_zero_hit = false;
            else if (cycles == SCANLINE_VISIBLE_DOTS + 2 && scroll_enabled) {
                // Set bits related to horizontal position
                data_address &= ~0x41f; //Unset horizontal bits
                data_address |= temp_address & 0x41f; //Copy
            }
            else if (cycles > 280 && cycles <= 304 && scroll_enabled) {
                // Set vertical bits
                data_address &= ~0x7be0; //Unset bits related to horizontal
                data_address |= temp_address & 0x7be0; //Copy
            }
            // if (cycles > 257 && cycles < 320)
            //     sprite_data_address = 0;
            // if rendering is on, every other frame is one cycle shorter
            if (cycles >= SCANLINE_END_CYCLE - (!is_even_frame && scroll_enabled)) {
                latch_background_valid = false;
                latch_sprite_patterns.fill(0);
                pipeline_state = RENDER;
                cycles = scanline = 0;
            }
            break;
        }
        case RENDER: {
            if (cycles > 0 && cycles <= SCANLINE_VISIBLE_DOTS) {
                NES_Byte bgColor = 0, sprColor = 0;
                bool bgOpaque = false, sprOpaque = true;
                bool spriteForeground = false;

                int x = cycles - 1;
                int y = scanline;

                if (is_showing_background || (latch_fetches && rendering)) {
                    auto x_fine = (fine_x_scroll + x) % 8;
                    if (!is_hiding_edge_background || x >= 8 || latch_fetches) {
                        // fetch tile
                        // mask off fine y
                        auto address = 0x2000 | (data_address & 0x0FFF);
                        //auto address = 0x2000 + x / 8 + (y / 8) * (SCANLINE_VISIBLE_DOTS / 8);
                        NES_Byte tile = bus.read(address);

                        //fetch pattern
                        //Each pattern occupies 16 bytes, so multiply by 16
                        //Add fine y
                        address = (tile * 16) + ((data_address >> 12/*y % 8*/) & 0x7);
                        //set whether the pattern is in the high or low page
                        address |= background_page << 12;
                        //Get the corresponding bit determined by (8 - x_fine) from the right
                        //bit 0 of palette entry
                        NES_Byte low, high;
                        if (latch_fetches) {
                            if (!latch_background_valid ||
                                latch_background_address != data_address ||
                                latch_background_page != background_page) {
                                latch_background_low = bus.read(address);
                                latch_background_high = bus.read(address + 8);
                                latch_background_address = data_address;
                                latch_background_page = background_page;
                                latch_background_valid = true;
                            }
                            low = latch_background_low;
                            high = latch_background_high;
                        } else {
                            low = bus.read(address);
                            high = bus.read(address + 8);
                        }
                        bgColor = (low >> (7 ^ x_fine)) & 1;
                        bgColor |= ((high >> (7 ^ x_fine)) & 1) << 1;

                        //flag used to calculate final pixel with the sprite pixel
                        bgOpaque = bgColor;

                        //fetch attribute and calculate higher two bits of palette
                        address = 0x23C0 | (data_address & 0x0C00) | ((data_address >> 4) & 0x38)
                                    | ((data_address >> 2) & 0x07);
                        auto attribute = bus.read(address);
                        int shift = ((data_address >> 4) & 4) | (data_address & 2);
                        //Extract and set the upper two bits for the color
                        bgColor |= ((attribute >> shift) & 0x3) << 2;
                    }
                    //Increment/wrap coarse X
                    if (x_fine == 7) {
                        latch_background_valid = false;
                        // if coarse X == 31
                        if ((data_address & 0x001F) == 31) {
                            // coarse X = 0
                            data_address &= ~0x001F;
                            // switch horizontal nametable
                            data_address ^= 0x0400;
                        }
                        else
                            // increment coarse X
                            data_address += 1;
                    }
                }

                if (latch_fetches && (!is_showing_background ||
                    (is_hiding_edge_background && x < 8))) {
                    bgColor = 0;
                    bgOpaque = false;
                }
                if (is_showing_sprites && (!is_hiding_edge_sprites || x >= 8)) {
                    for (std::size_t slot = 0; slot < scanline_sprites.size(); ++slot) {
                        if (latch_fetches && slot >= 8) break;
                        const auto i = scanline_sprites[slot];
                        NES_Byte spr_x =     sprite_memory[i * 4 + 3];

                        if (0 > x - spr_x || x - spr_x >= 8)
                            continue;

                        NES_Byte spr_y     = sprite_memory[i * 4 + 0] + 1,
                             tile      = sprite_memory[i * 4 + 1],
                             attribute = sprite_memory[i * 4 + 2];

                        int length = (is_long_sprites) ? 16 : 8;

                        int x_shift = (x - spr_x) % 8, y_offset = (y - spr_y) % length;

                        if ((attribute & 0x40) == 0) //If NOT flipping horizontally
                            x_shift ^= 7;
                        if ((attribute & 0x80) != 0) //IF flipping vertically
                            y_offset ^= (length - 1);

                        NES_Address address = 0;

                        if (!is_long_sprites) {
                            address = tile * 16 + y_offset;
                            if (sprite_page == HIGH) address += 0x1000;
                        }
                        // 8 x 16 sprites
                        else {
                            //bit-3 is one if it is the bottom tile of the sprite, multiply by two to get the next pattern
                            y_offset = (y_offset & 7) | ((y_offset & 8) << 1);
                            address = (tile >> 1) * 32 + y_offset;
                            address |= (tile & 1) << 12; //Bank 0x1000 if bit-0 is high
                        }

                        const NES_Byte low = latch_fetches ?
                            latch_sprite_patterns[slot * 2] : bus.read(address);
                        const NES_Byte high = latch_fetches ?
                            latch_sprite_patterns[slot * 2 + 1] : bus.read(address + 8);
                        sprColor |= (low >> x_shift) & 1;
                        sprColor |= ((high >> x_shift) & 1) << 1;

                        if (!(sprOpaque = sprColor)) {
                            sprColor = 0;
                            continue;
                        }

                        sprColor |= 0x10; //Select sprite palette
                        sprColor |= (attribute & 0x3) << 2; //bits 2-3

                        spriteForeground = !(attribute & 0x20);

                        //Sprite-0 hit detection
                        if (!is_sprite_zero_hit && is_showing_background && i == 0 && sprOpaque && bgOpaque)
                            is_sprite_zero_hit = true;

                        break; //Exit the loop now since we've found the highest priority sprite
                    }
                }
                // get the address of the color in the palette
                NES_Byte paletteAddr = bgColor;
                if ( (!bgOpaque && sprOpaque) || (bgOpaque && sprOpaque && spriteForeground) )
                    paletteAddr = sprColor;
                else if (!bgOpaque && !sprOpaque)
                    paletteAddr = 0;
                // lookup the pixel in the palette and write it to the screen
                nes_pixels[y][x] = bus.read_palette(paletteAddr);
            }
            else if (cycles == SCANLINE_VISIBLE_DOTS + 1 &&
                     (is_showing_background || (latch_fetches && rendering))) {
                //Shamelessly copied from nesdev wiki
                if ((data_address & 0x7000) != 0x7000) {  // if fine Y < 7
                    // increment fine Y
                    data_address += 0x1000;
                } else {
                    // fine Y = 0
                    data_address &= ~0x7000;
                    // let y = coarse Y
                    int y = (data_address & 0x03E0) >> 5;
                    if (y == 29) {
                        // coarse Y = 0
                        y = 0;
                        // switch vertical nametable
                        data_address ^= 0x0800;
                    } else if (y == 31) {
                        // coarse Y = 0, nametable not switched
                        y = 0;
                    } else {
                        // increment coarse Y
                        y += 1;
                    }
                    // put coarse Y back into data_address
                    data_address = (data_address & ~0x03E0) | (y << 5);
                }
            }
            else if (cycles == SCANLINE_VISIBLE_DOTS + 2 && scroll_enabled) {
                // Copy bits related to horizontal position
                data_address &= ~0x41f;
                data_address |= temp_address & 0x41f;
            }

            if (cycles >= SCANLINE_END_CYCLE) {
                // Find and index sprites that are on the next Scanline
                // This isn't where/when this indexing, actually copying in
                // 2C02 is done but (I think) it shouldn't hurt any games if
                // this is done here
                scanline_sprites.resize(0);

                int range = 8;
                if (is_long_sprites)
                    range = 16;

                NES_Byte j = 0;
                for (NES_Byte i = sprite_data_address / 4; i < 64; ++i) {
                    auto diff = (scanline - sprite_memory[i * 4]);
                    if (0 <= diff && diff < range) {
                        scanline_sprites.push_back(i);
                        if (++j >= 8)
                            break;
                    }
                }

                if (latch_fetches) fetch_latched_sprite_patterns(bus);
                latch_background_valid = false;
                ++scanline;
                cycles = 0;
            }

            if (scanline >= VISIBLE_SCANLINES)
                pipeline_state = POST_RENDER;

            break;
        }
        case POST_RENDER: {
            if (cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                cycles = 0;
                pipeline_state = VERTICAL_BLANK;
            }
            break;
        }
        case VERTICAL_BLANK: {
            if (cycles == 1 && scanline == VISIBLE_SCANLINES + 1) {
                is_vblank = true;
                if (is_interrupting) vblank_callback();
            }

            if (cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                cycles = 0;
            }

            if (scanline >= FRAME_END_SCANLINE) {  // end of video frame
                // render the frame using the NTSC video filter
                nes_ntsc_blit(
                    &ntsc,                  // configured NTSC object
                    *nes_pixels,            // input buffer of NES pixels
                    SCANLINE_VISIBLE_DOTS,  // width of the NES screen
                    is_even_frame,          // alternating frame flag
                    SCANLINE_VISIBLE_DOTS,  // width of the NES screen
                    VISIBLE_SCANLINES,      // height of the NES screen
                    *ntsc_screen,           // output buffer to write to
                    NTSC_PITCH              // number of bytes in an output row
                );
                // update the PPU state
                pipeline_state = PRE_RENDER;
                scanline = 0;
                is_even_frame = !is_even_frame;
            }

            break;
        }
        default: NES_DEBUG("PPU::cycle reached invalid state");
    }
    ++cycles;
}

void PPU::do_DMA(const NES_Byte* page_ptr) {
    std::memcpy(
        sprite_memory.data() + sprite_data_address,
        page_ptr,
        256 - sprite_data_address
    );
    if (sprite_data_address)
        std::memcpy(
            sprite_memory.data(),
            page_ptr + (256 - sprite_data_address),
            sprite_data_address
        );
}

void PPU::control(NES_Byte ctrl) {
    latch_background_valid = false;
    is_interrupting = ctrl & 0x80;
    is_long_sprites = ctrl & 0x20;
    background_page = static_cast<CharacterPage>(!!(ctrl & 0x10));
    sprite_page = static_cast<CharacterPage>(!!(ctrl & 0x8));
    if (ctrl & 0x4)
        data_address_increment = 0x20;
    else
        data_address_increment = 1;
    // baseNameTable = (ctrl & 0x3) * 0x400 + 0x2000;
    // Set the nametable in the temp address, this will be reflected in the
    // data address during rendering
    // v-- Unset
    temp_address &= ~0xc00;
    // v-- Set according to ctrl bits
    temp_address |= (ctrl & 0x3) << 10;
}

void PPU::set_mask(NES_Byte mask) {
    latch_background_valid = false;
    is_hiding_edge_background = !(mask & 0x2);
    is_hiding_edge_sprites = !(mask & 0x4);
    is_showing_background = mask & 0x8;
    is_showing_sprites = mask & 0x10;
}

NES_Byte PPU::get_status() {
    NES_Byte status = is_sprite_zero_hit << 6 | is_vblank << 7;
    // data_address = 0;
    is_vblank = false;
    is_first_write = true;
    return status;
}

void PPU::set_data_address(NES_Byte address) {
    latch_background_valid = false;
    // data_address = ((data_address << 8) & 0xff00) | address;
    if (is_first_write) {
        // Unset the upper byte
        temp_address &= ~0xff00;
        temp_address |= (address & 0x3f) << 8;
        is_first_write = false;
    } else {
        // Unset the lower byte;
        temp_address &= ~0xff;
        temp_address |= address;
        data_address = temp_address;
        is_first_write = true;
    }
}

NES_Byte PPU::get_data(PictureBus& bus) {
    bus.observe_address(data_address);
    auto data = bus.read(data_address);
    data_address += data_address_increment;
    bus.observe_address(data_address);
    // Reads are delayed by one byte/read when address is in this range
    if (data_address < 0x3f00)
        // Return from the data buffer and store the current value in the buffer
        std::swap(data, data_buffer);
    return data;
}

void PPU::set_data(PictureBus& bus, NES_Byte data) {
    bus.observe_address(data_address);
    bus.write(data_address, data);
    data_address += data_address_increment;
    bus.observe_address(data_address);
}

void PPU::set_scroll(NES_Byte scroll) {
    latch_background_valid = false;
    if (is_first_write) {
        temp_address &= ~0x1f;
        temp_address |= (scroll >> 3) & 0x1f;
        fine_x_scroll = scroll & 0x7;
        is_first_write = false;
    } else {
        temp_address &= ~0x73e0;
        temp_address |= ((scroll & 0x7) << 12) | ((scroll & 0xf8) << 2);
        is_first_write = true;
    }
}

}  // namespace NES

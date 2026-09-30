//  Program:      nes-py
//  File:         ppu.hpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//
//  Pattern-fetch state adapted from nes-py 301da52f7f75de38 (MIT).
//  See docs/licenses/THIRD-PARTY.txt for provenance.
//

#ifndef NES_PPU_HPP
#define NES_PPU_HPP

#include "picture_bus.hpp"
#include "ntsc/nes_ntsc.h"
#include <array>
#include <jansson.h>
#include <functional>
#include <string>

namespace NES {

/// The number of visible scan lines (i.e., the height of the screen)
static constexpr int VISIBLE_SCANLINES = 240;
/// The number of visible dots per scan line (i.e., the width of the screen)
static constexpr int SCANLINE_VISIBLE_DOTS = 256;
/// The number of visible dots per scan line after rendering
static constexpr int SCANLINE_VISIBLE_DOTS_NTSC = NES_NTSC_OUT_WIDTH(SCANLINE_VISIBLE_DOTS);
/// the number of bytes in an NTSC pixel row
static constexpr int NTSC_PITCH = SCANLINE_VISIBLE_DOTS_NTSC * sizeof(NES_Pixel);
/// The number of cycles per scanline
static constexpr int SCANLINE_CYCLE_LENGTH = 341;
/// The last cycle of a scan line (changed from 340 to fix render glitch)
static constexpr int SCANLINE_END_CYCLE = 341;
/// The last scanline per frame
static constexpr int FRAME_END_SCANLINE = 261;

/// The Picture Processing Unit (PPU) for the NES
class PPU {
 private:
    /// The callback to fire when entering vertical blanking mode
    Callback vblank_callback;
    /// The OAM memory (sprites)
    std::vector<NES_Byte> sprite_memory = std::vector<NES_Byte>(64 * 4);
    /// OAM memory (sprites) for the next scanline
    std::vector<NES_Byte> scanline_sprites;

    /// Pattern bytes already fetched for CHR-latch mappers. These bytes must
    /// survive a snapshot: rereading CHR could observe a different bank.
    bool latch_background_valid = false;
    NES_Address latch_background_address = 0;
    NES_Byte latch_background_page = 0;
    NES_Byte latch_background_low = 0;
    NES_Byte latch_background_high = 0;
    std::array<NES_Byte, 16> latch_sprite_patterns = {};

    /// Fetch selected sprite rows in OAM order, including covered/hidden rows.
    void fetch_latched_sprite_patterns(PictureBus& bus);

    /// Sprite pattern addresses captured for the timed MMC3 fetch slots.
    std::array<NES_Address, 8> irq_sprite_addresses = {};
    void clock_mapper_address(PictureBus& bus);

    /// The current pipeline state of the PPU
    enum State {
        PRE_RENDER,
        RENDER,
        POST_RENDER,
        VERTICAL_BLANK
    } pipeline_state;

    /// The number of cycles left in the frame
    int cycles;
    /// the current scanline of the frame
    int scanline;
    /// whether the PPU is on an even frame
    bool is_even_frame;

    // Status

    /// whether the PPU is in vertical blanking mode
    bool is_vblank;
    /// whether sprite 0 has been hit (i.e., collision detection)
    bool is_sprite_zero_hit;

    // Registers

    /// the current data address to (read / write) (from / to)
    NES_Address data_address;
    /// a temporary address register
    NES_Address temp_address;
    /// the fine scrolling position
    NES_Byte fine_x_scroll;
    /// TODO: doc
    bool is_first_write;
    /// The address of the data buffer
    NES_Byte data_buffer;
    /// the read / write address for the OAM memory (sprites)
    NES_Byte sprite_data_address;

    // Mask

    /// whether the PPU is showing sprites
    bool is_showing_sprites;
    /// whether the PPU is showing background pixels
    bool is_showing_background;
    /// whether the PPU is hiding sprites along the edges
    bool is_hiding_edge_sprites;
    /// whether the PPU is hiding the background along the edges
    bool is_hiding_edge_background;

    // Setup flags and variables

    /// TODO: doc
    bool is_long_sprites;
    /// whether the PPU is in the interrupt handler
    bool is_interrupting;

    /// TODO: doc
    enum CharacterPage {
        LOW,
        HIGH,
    } background_page, sprite_page;

    /// The value to increment the data address by
    NES_Address data_address_increment;

    /// The internal screen data structure as a vector representation of a
    /// matrix of height matching the visible scans lines and width matching
    /// the number of visible scan line dots. the data type is 8-bit NES pixel
    /// index corresponding to a value in the NES palette
    NES_Byte nes_pixels[VISIBLE_SCANLINES][SCANLINE_VISIBLE_DOTS];
    /// the NTSC video filter for rendering RGB pixels from NES pixels
    nes_ntsc_t ntsc;
    /// The RGB pixels rendered by the NTSC video filter
    NES_Pixel ntsc_screen[VISIBLE_SCANLINES][SCANLINE_VISIBLE_DOTS_NTSC];

 public:
    /// A completed CPU PPUADDR write drives the external address bus.
    void observe_cpu_address(PictureBus& bus) {
        if (is_first_write) bus.observe_address(data_address);
    }
    static bool is_valid_irq_fetch_state(json_t* state) {
        if (!json_is_array(state) || json_array_size(state) != 8) return false;
        for (int i = 0; i < 8; ++i) {
            json_t* value = json_array_get(state, i);
            if (!json_is_integer(value) || json_integer_value(value) < 0 ||
                json_integer_value(value) > 0x1FFF) return false;
        }
        return true;
    }

    /// Validate the new fetch state before replacing a loaded emulator.
    static bool is_valid_latch_fetch_state(json_t* state) {
        if (!json_is_object(state) ||
            !json_is_boolean(json_object_get(state, "valid"))) return false;
        const char* fields[] = {"address", "page", "low", "high"};
        const int limits[] = {65535, 1, 255, 255};
        for (int i = 0; i < 4; ++i) {
            json_t* value = json_object_get(state, fields[i]);
            if (!json_is_integer(value) || json_integer_value(value) < 0 ||
                json_integer_value(value) > limits[i]) return false;
        }
        json_t* sprites = json_object_get(state, "sprites");
        if (!json_is_array(sprites) || json_array_size(sprites) != 16) return false;
        for (int i = 0; i < 16; ++i) {
            json_t* value = json_array_get(sprites, i);
            if (!json_is_integer(value) || json_integer_value(value) < 0 ||
                json_integer_value(value) > 255) return false;
        }
        return true;
    }

    /// Perform a single cycle on the PPU.
    void cycle(PictureBus& bus);

    /// Reset the PPU.
    void reset();

    /// Set the interrupt callback for the CPU.
    ///
    /// @param callback the callback for handling interrupts from the PPU
    ///
    inline void set_interrupt_callback(Callback cb) { vblank_callback = cb; }

    /// TODO: doc
    void do_DMA(const NES_Byte* page_ptr);

    // MARK: Callbacks mapped to CPU address space

    /// Set the control register to a new value.
    ///
    /// @param ctrl the new control register byte
    ///
    void control(NES_Byte ctrl);

    /// Set the mask register to a new value.
    ///
    /// @param mask the new mask value
    ///
    void set_mask(NES_Byte mask);

    /// Set the scroll register to a new value.
    ///
    /// @param scroll the new scroll register value
    ///
    void set_scroll(NES_Byte scroll);

    /// Return the value in the PPU status register.
    NES_Byte get_status();

    /// TODO: doc
    void set_data_address(NES_Byte address);

    /// Read data off the picture bus.
    ///
    /// @param bus the bus to read data off of
    ///
    NES_Byte get_data(PictureBus& bus);

    /// TODO: doc
    void set_data(PictureBus& bus, NES_Byte data);

    /// Set the sprite data address to a new value.
    ///
    /// @param address the new OAM data address
    ///
    inline void set_OAM_address(NES_Byte address) {
        sprite_data_address = address;
    }

    /// Read a byte from OAM memory at the sprite data address.
    ///
    /// @return the byte at the given address in OAM memory
    ///
    inline NES_Byte get_OAM_data() const {
        return sprite_memory[sprite_data_address];
    }

    /// Write a byte to OAM memory at the sprite data address.
    ///
    /// @param value the byte to write to the given address
    ///
    inline void set_OAM_data(NES_Byte value) {
        sprite_memory[sprite_data_address++] = value;
    }

    /// Return a pointer to the screen buffer.
    inline NES_Pixel* get_screen_buffer() { return *ntsc_screen; }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() const {
        json_t* rootJ = json_object();
        json_t* fetch = json_pack("{s:b,s:i,s:i,s:i,s:i}",
            "valid", latch_background_valid, "address", latch_background_address,
            "page", latch_background_page, "low", latch_background_low,
            "high", latch_background_high);
        json_t* sprites = json_array();
        for (NES_Byte byte : latch_sprite_patterns)
            json_array_append_new(sprites, json_integer(byte));
        json_object_set_new(fetch, "sprites", sprites);
        json_object_set_new(rootJ, "chr_latch_fetches", fetch);
        json_t* irq_fetches = json_array();
        for (NES_Address address : irq_sprite_addresses)
            json_array_append_new(irq_fetches, json_integer(address));
        json_object_set_new(rootJ, "irq_sprite_addresses", irq_fetches);
        // encode sprite_memory
        {
            auto data_string = base64_encode(&sprite_memory[0], sprite_memory.size());
            json_object_set_new(rootJ, "sprite_memory", json_string(data_string.c_str()));
        }
        // encode scanline_sprites
        {
            auto data_string = base64_encode(scanline_sprites.data(), scanline_sprites.size());
            json_object_set_new(rootJ, "scanline_sprites", json_string(data_string.c_str()));
        }
        json_object_set_new(rootJ, "pipeline_state", json_integer(pipeline_state));
        json_object_set_new(rootJ, "cycles", json_integer(cycles));
        json_object_set_new(rootJ, "scanline", json_integer(scanline));
        json_object_set_new(rootJ, "is_even_frame", json_boolean(is_even_frame));
        json_object_set_new(rootJ, "is_vblank", json_boolean(is_vblank));
        json_object_set_new(rootJ, "is_sprite_zero_hit", json_boolean(is_sprite_zero_hit));
        json_object_set_new(rootJ, "data_address", json_integer(data_address));
        json_object_set_new(rootJ, "temp_address", json_integer(temp_address));
        json_object_set_new(rootJ, "fine_x_scroll", json_integer(fine_x_scroll));
        json_object_set_new(rootJ, "is_first_write", json_boolean(is_first_write));
        json_object_set_new(rootJ, "data_buffer", json_integer(data_buffer));
        json_object_set_new(rootJ, "sprite_data_address", json_integer(sprite_data_address));
        json_object_set_new(rootJ, "is_showing_sprites", json_boolean(is_showing_sprites));
        json_object_set_new(rootJ, "is_showing_background", json_boolean(is_showing_background));
        json_object_set_new(rootJ, "is_hiding_edge_sprites", json_boolean(is_hiding_edge_sprites));
        json_object_set_new(rootJ, "is_hiding_edge_background", json_boolean(is_hiding_edge_background));
        json_object_set_new(rootJ, "is_long_sprites", json_boolean(is_long_sprites));
        json_object_set_new(rootJ, "is_interrupting", json_boolean(is_interrupting));
        json_object_set_new(rootJ, "background_page", json_integer(background_page));
        json_object_set_new(rootJ, "sprite_page", json_integer(sprite_page));
        json_object_set_new(rootJ, "data_address_increment", json_integer(data_address_increment));
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        irq_sprite_addresses.fill(0);
        json_t* irq_fetches = json_object_get(rootJ, "irq_sprite_addresses");
        if (is_valid_irq_fetch_state(irq_fetches))
            for (int i = 0; i < 8; ++i)
                irq_sprite_addresses[i] = json_integer_value(json_array_get(irq_fetches, i));
        latch_background_valid = false;
        latch_background_address = latch_background_page = 0;
        latch_background_low = latch_background_high = 0;
        latch_sprite_patterns.fill(0);
        json_t* fetch = json_object_get(rootJ, "chr_latch_fetches");
        if (is_valid_latch_fetch_state(fetch)) {
            latch_background_valid = json_is_true(json_object_get(fetch, "valid"));
            latch_background_address = json_integer_value(json_object_get(fetch, "address"));
            latch_background_page = json_integer_value(json_object_get(fetch, "page"));
            latch_background_low = json_integer_value(json_object_get(fetch, "low"));
            latch_background_high = json_integer_value(json_object_get(fetch, "high"));
            for (int i = 0; i < 16; ++i)
                latch_sprite_patterns[i] = json_integer_value(json_array_get(
                    json_object_get(fetch, "sprites"), i));
        }
        // load sprite_memory
        {
            json_t* json_data = json_object_get(rootJ, "sprite_memory");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                sprite_memory = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
        // load scanline_sprites
        {
            json_t* json_data = json_object_get(rootJ, "scanline_sprites");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                scanline_sprites = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
        // load pipeline_state
        {
            json_t* json_data = json_object_get(rootJ, "pipeline_state");
            if (json_data) pipeline_state = static_cast<State>(json_integer_value(json_data));
        }
        // load cycles
        {
            json_t* json_data = json_object_get(rootJ, "cycles");
            if (json_data) cycles = json_integer_value(json_data);
        }
        // load scanline
        {
            json_t* json_data = json_object_get(rootJ, "scanline");
            if (json_data) scanline = json_integer_value(json_data);
        }
        // load is_even_frame
        {
            json_t* json_data = json_object_get(rootJ, "is_even_frame");
            if (json_data) is_even_frame = json_boolean_value(json_data);
        }
        // load is_vblank
        {
            json_t* json_data = json_object_get(rootJ, "is_vblank");
            if (json_data) is_vblank = json_boolean_value(json_data);
        }
        // load is_sprite_zero_hit
        {
            json_t* json_data = json_object_get(rootJ, "is_sprite_zero_hit");
            if (json_data) is_sprite_zero_hit = json_boolean_value(json_data);
        }
        // load data_address
        {
            json_t* json_data = json_object_get(rootJ, "data_address");
            if (json_data) data_address = json_integer_value(json_data);
        }
        // load temp_address
        {
            json_t* json_data = json_object_get(rootJ, "temp_address");
            if (json_data) temp_address = json_integer_value(json_data);
        }
        // load fine_x_scroll
        {
            json_t* json_data = json_object_get(rootJ, "fine_x_scroll");
            if (json_data) fine_x_scroll = json_integer_value(json_data);
        }
        // load is_first_write
        {
            json_t* json_data = json_object_get(rootJ, "is_first_write");
            if (json_data) is_first_write = json_boolean_value(json_data);
        }
        // load data_buffer
        {
            json_t* json_data = json_object_get(rootJ, "data_buffer");
            if (json_data) data_buffer = json_integer_value(json_data);
        }
        // load sprite_data_address
        {
            json_t* json_data = json_object_get(rootJ, "sprite_data_address");
            if (json_data) sprite_data_address = json_integer_value(json_data);
        }
        // load is_showing_sprites
        {
            json_t* json_data = json_object_get(rootJ, "is_showing_sprites");
            if (json_data) is_showing_sprites = json_boolean_value(json_data);
        }
        // load is_showing_background
        {
            json_t* json_data = json_object_get(rootJ, "is_showing_background");
            if (json_data) is_showing_background = json_boolean_value(json_data);
        }
        // load is_hiding_edge_sprites
        {
            json_t* json_data = json_object_get(rootJ, "is_hiding_edge_sprites");
            if (json_data) is_hiding_edge_sprites = json_boolean_value(json_data);
        }
        // load is_hiding_edge_background
        {
            json_t* json_data = json_object_get(rootJ, "is_hiding_edge_background");
            if (json_data) is_hiding_edge_background = json_boolean_value(json_data);
        }
        // load is_long_sprites
        {
            json_t* json_data = json_object_get(rootJ, "is_long_sprites");
            if (json_data) is_long_sprites = json_boolean_value(json_data);
        }
        // load is_interrupting
        {
            json_t* json_data = json_object_get(rootJ, "is_interrupting");
            if (json_data) is_interrupting = json_boolean_value(json_data);
        }
        // load background_page
        {
            json_t* json_data = json_object_get(rootJ, "background_page");
            if (json_data) background_page = static_cast<CharacterPage>(json_integer_value(json_data));
        }
        // load sprite_page
        {
            json_t* json_data = json_object_get(rootJ, "sprite_page");
            if (json_data) sprite_page = static_cast<CharacterPage>(json_integer_value(json_data));
        }
        // load data_address_increment
        {
            json_t* json_data = json_object_get(rootJ, "data_address_increment");
            if (json_data) data_address_increment = json_integer_value(json_data);
        }
    }
};

}  // namespace NES

#endif  // NES_PPU_HPP

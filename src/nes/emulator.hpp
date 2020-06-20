//  Program:      nes-py
//  File:         emulator.hpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include <string>
#include <jansson.h>
#include "common.hpp"
#include "controller.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "apu.hpp"
#include "main_bus.hpp"
#include "picture_bus.hpp"
#include "cartridge.hpp"

namespace NES {

/// An NES Emulator and OpenAI Gym interface
class Emulator {
 private:
    /// the number of elapsed cycles
    uint32_t cycles = 0;
    /// the virtual cartridge with ROM and mapper data
    Cartridge* cartridge = nullptr;
    /// the 2 controllers on the emulator
    Controller controllers[2];

    /// the main data bus of the emulator
    MainBus bus;
    /// the picture bus from the PPU of the emulator
    PictureBus picture_bus;

    /// The emulator's CPU
    CPU cpu;
    /// the emulators' PPU
    PPU ppu;
    /// The emulator's APU
    APU apu;

 public:
    /// The width of the NES screen in pixels
    static const int WIDTH = SCANLINE_VISIBLE_DOTS;
    /// The height of the NES screen in pixels
    static const int HEIGHT = VISIBLE_SCANLINES;
    /// the number of pixels on the NES
    static const int PIXELS = WIDTH * HEIGHT;
    /// the number of bytes in the screen (RGBx)
    static const int SCREEN_BYTES = PIXELS * 4;

    /// Initialize a new emulator.
    Emulator() {
        // set the read callbacks
        bus.set_read_callback(PPUSTATUS, [&](void) { return ppu.get_status();          });
        bus.set_read_callback(PPUDATA,   [&](void) { return ppu.get_data(picture_bus); });
        bus.set_read_callback(JOY1,      [&](void) { return controllers[0].read();     });
        bus.set_read_callback(JOY2,      [&](void) { return controllers[1].read();     });
        bus.set_read_callback(OAMDATA,   [&](void) { return ppu.get_OAM_data();        });
        bus.set_read_callback(SND_CHN,   [&](void) { return apu.read_status();         });
        // set the write callbacks
        bus.set_write_callback(PPUCTRL,  [&](NES_Byte b) { ppu.control(b);                                             });
        bus.set_write_callback(PPUMASK,  [&](NES_Byte b) { ppu.set_mask(b);                                            });
        bus.set_write_callback(OAMADDR,  [&](NES_Byte b) { ppu.set_OAM_address(b);                                     });
        bus.set_write_callback(PPUADDR,  [&](NES_Byte b) { ppu.set_data_address(b);                                    });
        bus.set_write_callback(PPUSCROL, [&](NES_Byte b) { ppu.set_scroll(b);                                          });
        bus.set_write_callback(PPUDATA,  [&](NES_Byte b) { ppu.set_data(picture_bus, b);                               });
        bus.set_write_callback(OAMDMA,   [&](NES_Byte b) { cpu.skip_DMA_cycles(); ppu.do_DMA(bus.get_page_pointer(b)); });
        bus.set_write_callback(JOY1,     [&](NES_Byte b) { controllers[0].strobe(b); controllers[1].strobe(b);         });
        bus.set_write_callback(OAMDATA,  [&](NES_Byte b) { ppu.set_OAM_data(b);                                        });
        // APU
        bus.set_write_callback(SQ1_VOL,     [&](NES_Byte b) { apu.write(SQ1_VOL, b);     });
        bus.set_write_callback(SQ1_SWEEP,   [&](NES_Byte b) { apu.write(SQ1_SWEEP, b);   });
        bus.set_write_callback(SQ1_LO,      [&](NES_Byte b) { apu.write(SQ1_LO, b);      });
        bus.set_write_callback(SQ1_HI,      [&](NES_Byte b) { apu.write(SQ1_HI, b);      });
        bus.set_write_callback(SQ2_VOL,     [&](NES_Byte b) { apu.write(SQ2_VOL, b);     });
        bus.set_write_callback(SQ2_SWEEP,   [&](NES_Byte b) { apu.write(SQ2_SWEEP, b);   });
        bus.set_write_callback(SQ2_LO,      [&](NES_Byte b) { apu.write(SQ2_LO, b);      });
        bus.set_write_callback(SQ2_HI,      [&](NES_Byte b) { apu.write(SQ2_HI, b);      });
        bus.set_write_callback(TRI_LINEAR,  [&](NES_Byte b) { apu.write(TRI_LINEAR, b);  });
        bus.set_write_callback(APU_UNUSED1, [&](NES_Byte b) { apu.write(APU_UNUSED1, b); });
        bus.set_write_callback(TRI_LO,      [&](NES_Byte b) { apu.write(TRI_LO, b);      });
        bus.set_write_callback(TRI_HI,      [&](NES_Byte b) { apu.write(TRI_HI, b);      });
        bus.set_write_callback(NOISE_VOL,   [&](NES_Byte b) { apu.write(NOISE_VOL, b);   });
        bus.set_write_callback(APU_UNUSED2, [&](NES_Byte b) { apu.write(APU_UNUSED2, b); });
        bus.set_write_callback(NOISE_LO,    [&](NES_Byte b) { apu.write(NOISE_LO, b);    });
        bus.set_write_callback(NOISE_HI,    [&](NES_Byte b) { apu.write(NOISE_HI, b);    });
        bus.set_write_callback(DMC_FREQ,    [&](NES_Byte b) { apu.write(DMC_FREQ, b);    });
        bus.set_write_callback(DMC_RAW,     [&](NES_Byte b) { apu.write(DMC_RAW, b);     });
        bus.set_write_callback(DMC_START,   [&](NES_Byte b) { apu.write(DMC_START, b);   });
        bus.set_write_callback(DMC_LEN,     [&](NES_Byte b) { apu.write(DMC_LEN, b);     });
        bus.set_write_callback(SND_CHN,     [&](NES_Byte b) { apu.write(SND_CHN, b);     });
        bus.set_write_callback(JOY2,        [&](NES_Byte b) { apu.write(JOY2, b);        });
        // set the interrupt callback for the PPU
        ppu.set_interrupt_callback([&]() { cpu.interrupt(bus, CPU::NMI_INTERRUPT); });
        // setup the DMC reader callback (for loading samples from RAM)
        apu.set_dmc_reader([&](void*, cpu_addr_t addr) -> int { return bus.read(addr);  });
        apu.set_irq_callback([&](void*) { cpu.interrupt(bus, CPU::IRQ_INTERRUPT); });
    }

    // Destroy this emulator.
    ~Emulator() { if (cartridge != nullptr) delete cartridge; }

    /// Return true if the clock is high, false otherwise.
    inline bool is_clock_high() {
        static constexpr float CLOCK_PW = 0.5;
        return cycles < CYCLES_PER_FRAME / (1.f / CLOCK_PW);
    }

    /// Return true if the emulator has a game inserted.
    inline bool has_game() const { return cartridge != nullptr; }

    /// Load a new game into the emulator.
    ///
    /// @param path a path to the ROM to load into the emulator
    /// @returns true if the load succeeded, false otherwise
    /// @note when returning false, the emulator remains in its current state
    /// @note the boolean output answers the question: is the ASIC mapper
    ///       implemented for the ROM at given path?
    ///
    bool load_game(const std::string& path) {
        // load the new game, but don't overwrite the cartridge yet
        auto game = Cartridge::create(path, [&](){
            picture_bus.update_mirroring();
        });
        // if the game is nullptr the load failed, return false
        if (game == nullptr) return false;
        // check for an existing game and delete it if it exists
        if (cartridge != nullptr) delete cartridge;
        // assign the game pointer to the cartridge slot
        cartridge = game;
        // setup the buses and reset the machine
        bus.set_mapper(cartridge->get_mapper());
        picture_bus.set_mapper(cartridge->get_mapper());
        reset();
        // load succeeded, return true
        return true;
    }

    /// Remove the inserted game from the emulator.
    inline void remove_game() {
        if (cartridge != nullptr) {
            delete cartridge;
            cartridge = nullptr;
        }
    }

    /// Set the sample rate to a new value.
    ///
    /// @param value the frame rate, i.e., 96000Hz
    ///
    inline void set_sample_rate(uint32_t value = APU::SAMPLE_RATE) {
        apu.set_sample_rate(value);
    }

    /// Set the clock-rate to a new value.
    ///
    /// @param value the frame rate, i.e., 1789773CPS
    ///
    inline void set_clock_rate(uint64_t value = CLOCK_RATE) {
        apu.set_clock_rate(value);
    }

    /// Return the path to the ROM on disk.
    inline std::string get_rom_path() const {
        if (has_game()) return cartridge->get_rom_path();
        return "";
    }

    /// Return a 32-bit pointer to the screen buffer's first address.
    ///
    /// @return a 32-bit pointer to the screen buffer's first address
    ///
    inline NES_Pixel* get_screen_buffer() { return ppu.get_screen_buffer(); }

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    inline NES_Byte* get_memory_buffer() { return bus.get_memory_buffer(); }

    /// Return a pointer to a controller port
    ///
    /// @param port the port of the controller to return the pointer to
    /// @return a pointer to the byte buffer for the controller state
    ///
    inline NES_Byte* get_controller(int port) {
        return controllers[port].get_joypad_buffer();
    }

    /// Write buttons to the virtual controller.
    ///
    /// @param buttons the button bitmap to write to the controller
    ///
    inline void set_controller(int port, NES_Byte buttons) {
        controllers[port].write_buttons(buttons);
    }

    /// Write buttons to the virtual controller.
    ///
    /// @param player1 the button bitmap to write to port 1 controller
    /// @param player2 the button bitmap to write to port 2 controller
    ///
    inline void set_controllers(NES_Byte player1, NES_Byte player2) {
        controllers[0].write_buttons(player1);
        controllers[1].write_buttons(player2);
    }

    /// Return an audio sample from the APU of the emulator.
    inline int16_t get_audio_sample() {
        if (!has_game()) return 0;
        return apu.get_sample();
    }

    /// Return an audio sample from the APU of the emulator [-10.f, 10.f].
    /// If there is no game in the emulator, returns 0V.
    inline float get_audio_voltage() {
        return 10.f * apu.get_sample() / static_cast<float>(1 << 15);
    }

    /// Load the ROM into the NES.
    inline void reset() {
        // ignore the call if there is no game
        if (!has_game()) return;
        // reset the CPU, PPU, and APU
        cpu.reset(bus);
        ppu.reset();
        apu.reset();
    }

    /// Run a single CPU cycle on the emulator.
    ///
    /// @param callback a callback function for when a frame event occurs
    ///
    template<typename EndOfFrameCallback>
    inline void cycle(EndOfFrameCallback callback) {
        // ignore the call if there is no game
        if (!has_game()) return;
        // 3 PPU steps per CPU step
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        cpu.cycle(bus);
        apu.cycle();
        // increment the cycles counter
        ++cycles;
        // check for the end of the frame
        if (cycles >= CYCLES_PER_FRAME) {
            cycles = 0;
            callback();
        }
    }

    /// Copy data from another instance of APU.
    void copy_from(const Emulator &other) {
        if (other.cartridge != nullptr) {  // other has cartridge to clone
            cartridge = other.cartridge->clone();
        } else if (cartridge != nullptr) {  // other has no cartridge, this does
            delete cartridge;
            cartridge = nullptr;
        }
        cycles = other.cycles;
        controllers[0] = other.controllers[0];
        controllers[1] = other.controllers[1];
        bus = other.bus;
        picture_bus = other.picture_bus;
        cpu = other.cpu;
        ppu = other.ppu;
        apu.copy_from(other.apu);
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() const {
        json_t* rootJ = json_object();
        if (cartridge != nullptr)
            json_object_set_new(rootJ, "cartridge", cartridge->dataToJson());
        json_object_set_new(rootJ, "controllers[0]", controllers[0].dataToJson());
        json_object_set_new(rootJ, "controllers[1]", controllers[1].dataToJson());
        json_object_set_new(rootJ, "bus", bus.dataToJson());
        json_object_set_new(rootJ, "picture_bus", picture_bus.dataToJson());
        json_object_set_new(rootJ, "cpu", cpu.dataToJson());
        json_object_set_new(rootJ, "ppu", ppu.dataToJson());
        json_object_set_new(rootJ, "apu", apu.dataToJson());
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load cartridge
        {
            json_t* json_data = json_object_get(rootJ, "cartridge");
            // if there is not cartridge data, there is no emulator state to
            // load, return
            if (!json_data) return;
            json_t* rom_path_data = json_object_get(json_data, "rom_path");
            // if the cartridge does not have a ROM, there is no emulator
            // state to load, return
            if (!rom_path_data) return;
            // load the game into the machine before loading the cartridge
            // data (because cartridge may be nullptr)
            load_game(json_string_value(rom_path_data));
            cartridge->dataFromJson(json_data);
        }
        // load controllers[0]
        {
            json_t* json_data = json_object_get(rootJ, "controllers[0]");
            if (json_data) controllers[0].dataFromJson(json_data);
        }
        // load controllers[1]
        {
            json_t* json_data = json_object_get(rootJ, "controllers[1]");
            if (json_data) controllers[1].dataFromJson(json_data);
        }
        // load bus
        {
            json_t* json_data = json_object_get(rootJ, "bus");
            if (json_data) bus.dataFromJson(json_data);
        }
        // load picture_bus
        {
            json_t* json_data = json_object_get(rootJ, "picture_bus");
            if (json_data) picture_bus.dataFromJson(json_data);
        }
        // load cpu
        {
            json_t* json_data = json_object_get(rootJ, "cpu");
            if (json_data) cpu.dataFromJson(json_data);
        }
        // load ppu
        {
            json_t* json_data = json_object_get(rootJ, "ppu");
            if (json_data) ppu.dataFromJson(json_data);
        }
        // load apu
        {
            json_t* json_data = json_object_get(rootJ, "apu");
            if (json_data) apu.dataFromJson(json_data);
        }
    }
};

}  // namespace NES

#endif  // NES_EMULATOR_HPP

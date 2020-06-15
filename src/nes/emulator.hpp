//  Program:      nes-py
//  File:         emulator.hpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include <string>
#include "common.hpp"
#include "cartridge.hpp"
#include "controller.hpp"
#include "apu.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "main_bus.hpp"
#include "picture_bus.hpp"

namespace NES {

/// An NES Emulator and OpenAI Gym interface
class Emulator {
 private:
    /// the virtual cartridge with ROM and mapper data
    Cartridge cartridge;
    /// the mapper for the cartridge
    Mapper* mapper;
    /// the 2 controllers on the emulator
    Controller controllers[2];

    /// whether the emulator has a backup available
    bool has_backup = false;

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

    /// the main data bus of the emulator
    MainBus backup_bus;
    /// the picture bus from the PPU of the emulator
    PictureBus backup_picture_bus;
    /// The emulator's CPU
    CPU backup_cpu;
    /// the emulators' PPU
    PPU backup_ppu;

 public:
    /// The number of cycles in 1 frame
    static const int CYCLES_PER_FRAME = 29781;
    /// The width of the NES screen in pixels
    static const int WIDTH = SCANLINE_VISIBLE_DOTS;
    /// The height of the NES screen in pixels
    static const int HEIGHT = VISIBLE_SCANLINES;
    /// the number of pixels on the NES
    static const int PIXELS = WIDTH * HEIGHT;
    /// the number of bytes in the screen (RGBx)
    static const int SCREEN_BYTES = PIXELS * 4;

    /// Initialize a new emulator with a path to a ROM file.
    ///
    /// @param rom_path the path to the ROM for the emulator to run
    ///
    explicit Emulator(const std::string& rom_path);

    /// Set the sample rate to a new value.
    ///
    /// @param value the frame rate, i.e., 96000Hz
    ///
    inline void set_sample_rate(int value) { apu.set_sample_rate(value); }

    /// Set the frame-rate to a new value.
    ///
    /// @param value the frame rate, i.e., 60FPS
    ///
    inline void set_frame_rate(float value) { apu.set_frame_rate(value); }

    /// Return the path to the ROM on disk.
    inline std::string get_rom_path() const { return cartridge.get_rom_path(); }

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

    /// Return an audio sample from the APU of the emulator.
    inline int16_t get_audio_sample() { return apu.get_sample(); }

    /// Return an audio sample from the APU of the emulator [-10.f, 10.f].
    inline float get_audio_voltage() {
        return 10.f * apu.get_sample() / static_cast<float>(1 << 15);
    }

    /// Load the ROM into the NES.
    inline void reset() { cpu.reset(bus); ppu.reset(); }

    /// Run a single CPU cycle on the emulator.
    void cycle() {
        // 3 PPU steps per CPU step
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        cpu.cycle(bus);
        // 1 APU cycle per CPU step
        apu.cycle();
    }

    /// Perform a step on the emulator, i.e., a single frame.
    void step() {
        // render a single frame on the emulator
        for (int i = 0; i < CYCLES_PER_FRAME; i++) cycle();
        // finish the frame on the APU
        apu.end_frame();
    }

    /// Create a backup state on the emulator.
    inline void backup() {
        backup_bus = bus;
        backup_picture_bus = picture_bus;
        backup_cpu = cpu;
        backup_ppu = ppu;
        has_backup = true;
    }

    /// Restore the backup state on the emulator.
    inline void restore() {
        // restore if there is a backup available
        if (!has_backup) return;
        bus = backup_bus;
        picture_bus = backup_picture_bus;
        cpu = backup_cpu;
        ppu = backup_ppu;
    }
};

}  // namespace NES

#endif  // NES_EMULATOR_HPP

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
    int cycles = 0;
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

    // ------------------------------------------------------------------------
    // MARK: Backup state variables: TODO: improve organization, i.e., a
    //       EmulatorData struct or something similar
    // ------------------------------------------------------------------------

    /// whether the emulator has a backup available
    bool has_backup = false;

    int backup_cycles = 0;
    /// the virtual cartridge with ROM and mapper data
    Cartridge* backup_cartridge = nullptr;
    /// the 2 controllers on the emulator
    Controller backup_controllers[2];

    /// the main data bus of the emulator
    MainBus backup_bus;
    /// the picture bus from the PPU of the emulator
    PictureBus backup_picture_bus;

    /// The emulator's CPU
    CPU backup_cpu;
    /// the emulators' PPU
    PPU backup_ppu;
    /// The emulator's APU
    APU backup_apu;

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

    /// Initialize a new emulator.
    Emulator() {
        // set the read callbacks
        bus.set_read_callback(PPUSTATUS, [&](void) { return ppu.get_status();          });
        bus.set_read_callback(PPUDATA,   [&](void) { return ppu.get_data(picture_bus); });
        bus.set_read_callback(JOY1,      [&](void) { return controllers[0].read();     });
        bus.set_read_callback(JOY2,      [&](void) { return controllers[1].read();     });
        bus.set_read_callback(OAMDATA,   [&](void) { return ppu.get_OAM_data();        });
        bus.set_read_callback(APUSTATUS, [&](void) { return apu.read_status();         });
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
        bus.set_write_callback(APU0,               [&](NES_Byte b) { apu.write(APU0, b);               });
        bus.set_write_callback(APU1,               [&](NES_Byte b) { apu.write(APU1, b);               });
        bus.set_write_callback(APU2,               [&](NES_Byte b) { apu.write(APU2, b);               });
        bus.set_write_callback(APU3,               [&](NES_Byte b) { apu.write(APU3, b);               });
        bus.set_write_callback(APU4,               [&](NES_Byte b) { apu.write(APU4, b);               });
        bus.set_write_callback(APU5,               [&](NES_Byte b) { apu.write(APU5, b);               });
        bus.set_write_callback(APU6,               [&](NES_Byte b) { apu.write(APU6, b);               });
        bus.set_write_callback(APU7,               [&](NES_Byte b) { apu.write(APU7, b);               });
        bus.set_write_callback(APU8,               [&](NES_Byte b) { apu.write(APU8, b);               });
        bus.set_write_callback(APU9,               [&](NES_Byte b) { apu.write(APU9, b);               });
        bus.set_write_callback(APUA,               [&](NES_Byte b) { apu.write(APUA, b);               });
        bus.set_write_callback(APUB,               [&](NES_Byte b) { apu.write(APUB, b);               });
        bus.set_write_callback(APUC,               [&](NES_Byte b) { apu.write(APUC, b);               });
        bus.set_write_callback(APUD,               [&](NES_Byte b) { apu.write(APUD, b);               });
        bus.set_write_callback(APUE,               [&](NES_Byte b) { apu.write(APUE, b);               });
        bus.set_write_callback(APUF,               [&](NES_Byte b) { apu.write(APUF, b);               });
        bus.set_write_callback(DMC_STATUS,         [&](NES_Byte b) { apu.write(DMC_STATUS, b);         });
        bus.set_write_callback(DMC_LOAD_COUNTER,   [&](NES_Byte b) { apu.write(DMC_LOAD_COUNTER, b);   });
        bus.set_write_callback(DMC_SAMPLE_ADDRESS, [&](NES_Byte b) { apu.write(DMC_SAMPLE_ADDRESS, b); });
        bus.set_write_callback(DMC_SAMPLE_LENGTH,  [&](NES_Byte b) { apu.write(DMC_SAMPLE_LENGTH, b);  });
        bus.set_write_callback(APUSTATUS,          [&](NES_Byte b) { apu.write(APUSTATUS, b);          });
        bus.set_write_callback(JOY2,               [&](NES_Byte b) { apu.write(JOY2, b);               });
        // set the interrupt callback for the PPU
        ppu.set_interrupt_callback([&]() { cpu.interrupt(bus, CPU::NMI_INTERRUPT); });
        // setup the DMC reader callback (for loading samples from RAM)
        apu.set_dmc_reader([&](void*, cpu_addr_t addr) -> int { return bus.read(addr);  });
        apu.set_irq_callback([&](void*) { cpu.interrupt(bus, CPU::IRQ_INTERRUPT); });
    }

    // Destroy this emulator
    ~Emulator() { if (cartridge != nullptr) delete cartridge; }

    /// Return true if the emulator has a game inserted.
    inline bool has_game() { return cartridge != nullptr; }

    /// Load a new game into the emulator.
    ///
    /// @param path a path to the ROM to load into the emulator
    ///
    inline void load_game(const std::string& path) {
        if (cartridge != nullptr) delete cartridge;
        cartridge = new Cartridge(path, [&](){ picture_bus.update_mirroring(); });
        bus.set_mapper(cartridge->get_mapper());
        picture_bus.set_mapper(cartridge->get_mapper());
        reset();
    }

    /// Remove the inserted game from the emulator.
    inline void remove_game() {
        if (cartridge != nullptr) delete cartridge;
        cartridge = nullptr;
    }

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
    inline std::string get_rom_path() const { return cartridge->get_rom_path(); }

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
    inline int16_t get_audio_sample() { return apu.get_sample(); }

    /// Return an audio sample from the APU of the emulator [-10.f, 10.f].
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
    inline void cycle() {
        // ignore the call if there is no game
        if (!has_game()) return;
        // 3 PPU steps per CPU step
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        cpu.cycle(bus);
        // 1 APU cycle per CPU step
        apu.cycle();
        // increment the cycles counter
        ++cycles;
    }

    /// Return true if the number of cycles to complete a frame has occurred.
    inline bool is_frame_complete() { return cycles >= CYCLES_PER_FRAME; }

    /// Finish a frame.
    inline void end_frame() {
        apu.end_frame();
        // reset the cycle counter back to 0
        cycles = 0;
    }

    /// Perform a step on the emulator, i.e., a single frame.
    inline void frame() {
        // ignore the call if there is no game
        if (!has_game()) return;
        // render a single frame on the emulator
        while (!is_frame_complete()) cycle();
        end_frame();
    }

    /// Create a backup state on the emulator.
    inline void backup() {
        // ignore the call if there is no game
        if (!has_game()) return;
        if (cartridge != nullptr) backup_cartridge = cartridge->clone();
        backup_cycles = cycles;
        backup_controllers[0] = controllers[0];
        backup_controllers[1] = controllers[1];
        backup_bus = bus;
        backup_picture_bus = picture_bus;
        backup_cpu = cpu;
        backup_ppu = ppu;
        backup_apu.copy_from(apu);
        has_backup = true;
    }

    /// Restore the backup state on the emulator.
    inline void restore() {
        // ignore the call if there is no game
        if (!has_game()) return;
        // restore if there is a backup available
        if (!has_backup) return;
        if (backup_cartridge != nullptr) cartridge = backup_cartridge->clone();
        cycles = backup_cycles;
        controllers[0] = backup_controllers[0];
        controllers[1] = backup_controllers[1];
        bus = backup_bus;
        picture_bus = backup_picture_bus;
        cpu = backup_cpu;
        ppu = backup_ppu;
        apu.copy_from(backup_apu);
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        if (cartridge != nullptr)
            json_object_set_new(rootJ, "cartridge", cartridge->dataToJson());
        json_object_set_new(rootJ, "controllers[0]", controllers[0].dataToJson());
        json_object_set_new(rootJ, "controllers[1]", controllers[1].dataToJson());
        // TODO: serialize backups
        // json_object_set_new(rootJ, "has_backup", json_boolean(has_backup));
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
            if (json_data) cartridge->dataFromJson(json_data);
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
        // TODO: serialize backups
        // // load has_backup
        // {
        //     json_t* json_data = json_object_get(rootJ, "has_backup");
        //     if (json_data) has_backup = json_boolean_value(json_data);
        // }
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

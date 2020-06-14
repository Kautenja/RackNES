//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "emulator.hpp"
#include "mapper_factory.hpp"
#include "log.hpp"

namespace NES {

Emulator::Emulator(const std::string& rom_path_) : rom_path(rom_path_) {
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
    bus.set_write_callback(DMC_STATUS,         [&](NES_Byte b) { apu.write(DMC_STATUS, b);         });
    bus.set_write_callback(DMC_LOAD_COUNTER,   [&](NES_Byte b) { apu.write(DMC_LOAD_COUNTER, b);   });
    bus.set_write_callback(DMC_SAMPLE_ADDRESS, [&](NES_Byte b) { apu.write(DMC_SAMPLE_ADDRESS, b); });
    bus.set_write_callback(DMC_SAMPLE_LENGTH,  [&](NES_Byte b) { apu.write(DMC_SAMPLE_LENGTH, b);  });
    bus.set_write_callback(APUSTATUS,          [&](NES_Byte b) { apu.write(APUSTATUS, b);          });
    bus.set_write_callback(JOY2,               [&](NES_Byte b) { apu.write(JOY2, b);               });
    // set the interrupt callback for the PPU
    ppu.set_interrupt_callback([&]() { cpu.interrupt(bus, CPU::NMI_INTERRUPT); });
    // load the ROM from disk, expect that the Python code has validated it
    cartridge.loadFromFile(rom_path);
    // create the mapper based on the mapper ID in the iNES header of the ROM
    auto mapper = MapperFactory(&cartridge, [&](){ picture_bus.update_mirroring(); });
    // give the IO buses a pointer to the mapper
    bus.set_mapper(mapper);
    picture_bus.set_mapper(mapper);
    // setup the DMC reader callback (for loading samples from RAM)
    apu.set_dmc_reader([&](void*, cpu_addr_t addr) -> int { return bus.read(addr);  });
}

void Emulator::step() {
    // render a single frame on the emulator
    for (int cycleIndex = 0; cycleIndex < CYCLES_PER_FRAME; cycleIndex++) {
        // 3 PPU steps per CPU step
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        cpu.cycle(bus);
        apu.cycle();
    }
    // finish the frame on the APU
    apu.end_frame();
}

}  // namespace NES

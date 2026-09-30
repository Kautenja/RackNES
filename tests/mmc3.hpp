// MMC3 banking cases adapted from nes-py 301da52f7f75de38 (MIT).
// Copyright (c) 2019 Christian Kauten. See docs/licenses/THIRD-PARTY.txt.
#ifndef RACKNES_TESTS_MMC3_HPP
#define RACKNES_TESTS_MMC3_HPP

#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

static std::vector<unsigned char> mmc3_image(int prg = 64, int chr = 256, bool four = false) {
    std::vector<unsigned char> bytes(16 + prg * 0x2000 + chr * 0x400, 0);
    bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1A;
    bytes[4] = prg / 2; bytes[5] = chr / 8; bytes[6] = four ? 0x48 : 0x40;
    for (int i = 0; i < prg * 0x2000; ++i) bytes[16 + i] = i / 0x2000;
    for (int i = 0; i < chr * 0x400; ++i) bytes[16 + prg * 0x2000 + i] = i / 0x400;
    return bytes;
}
static void write_mmc3_image(const std::vector<unsigned char>& bytes) {
    std::ofstream file(".build/mmc3.nes", std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close(); assert(file.good());
}
static void mmc3_edge(NES::ROM::Mapper* mapper, int low = 10) {
    mapper->observePPUAddress(0);
    for (int dot = 0; dot < low; ++dot) mapper->clockPPU();
    mapper->observePPUAddress(0x1000);
}

static void check_mmc3_banks() {
    for (int count : {4, 8, 16, 32, 64}) {
        write_mmc3_image(mmc3_image(count));
        NES::PictureBus picture;
        std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(".build/mmc3.nes",
            [&]() { picture.update_mirroring(); }));
        assert(cart); picture.set_mapper(cart->get_mapper());
        auto* mapper = cart->get_mapper();
        NES::MainBus bus; bus.set_mapper(mapper);
        for (int value = 0; value < 256; ++value) {
            for (int mode : {0, 0x40}) {
                mapper->writePRG(0x9FFE, mode | 6); mapper->writePRG(0x9FFF, value);
                mapper->writePRG(0x8000, mode | 7); mapper->writePRG(0x8001, value + 1);
                assert(bus.read(mode ? 0xC000 : 0x8000) == (value & 63) % count);
                assert(bus.read(mode ? 0x8000 : 0xC000) == count - 2);
                assert(bus.read(0xBFFF) == ((value + 1) & 63) % count);
                assert(bus.read(0xFFFF) == count - 1);
                assert(bus.get_page_pointer(mode ? 0xC0 : 0x80)[255] == (value & 63) % count);
            }
        }
        bus.write(0x6000, 0xA5); bus.write(0x7FFF, 0x5A);
        bus.write(0xA001, 0xC0); bus.write(0x6000, 0);
        assert(bus.read(0x6000) == 0xA5);
        bus.write(0xBFFF, 0); bus.write(0x7FFF, 0);
        assert(bus.read(0x6000) == 0 && bus.get_page_pointer(0x7F)[255] == 0);
        bus.write(0xA001, 0x80);
        assert(bus.read(0x7FFF) == 0x5A);
        for (int mode : {0, 1}) {
            bus.write(0xBFFE, mode);
            picture.write(0x2000, 11); picture.write(mode ? 0x2800 : 0x2400, 22);
            assert(picture.read(0x2400) == (mode ? 11 : 22));
            assert(picture.read(0x2800) == (mode ? 22 : 11));
        }
    }
    for (int count : {8, 16, 32, 64, 128, 256}) {
        write_mmc3_image(mmc3_image(4, count));
        std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(".build/mmc3.nes", []() {}));
        auto* m = cart->get_mapper();
        for (int value = 0; value < 256; ++value) for (int mode : {0, 0x80}) {
            for (int reg = 0; reg < 8; ++reg) {
                m->writePRG(0x8000, reg | mode); m->writePRG(0x8001, value + reg);
            }
            for (int slot = 0; slot < 8; ++slot) {
                const int bank = slot < 4 ? ((value + slot / 2) & 0xFE) + (slot & 1) :
                    (value + slot - 2) & 255;
                const int address = (slot ^ (mode ? 4 : 0)) * 0x400;
                assert(m->readCHR(address) == bank % count);
                m->writeCHR(address, 99); assert(m->readCHR(address + 1023) == bank % count);
            }
        }
    }
    write_mmc3_image(mmc3_image(4, 0, true));
    NES::PictureBus picture;
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(".build/mmc3.nes",
        [&]() { picture.update_mirroring(); }));
    auto* m = cart->get_mapper(); picture.set_mapper(m);
    for (int i = 0; i < 8; ++i) picture.write(i * 0x400, i + 10);
    m->writePRG(0x8000, 0x82); m->writePRG(0x8001, 7);
    assert(picture.read(0) == 17); picture.write(0, 77);
    m->writePRG(0x8000, 0); assert(picture.read(0x1C00) == 77);
    for (int i = 0; i < 4; ++i) picture.write(0x2000 + i * 0x400, i + 31);
    m->writePRG(0xA000, 1);
    for (int i = 0; i < 4; ++i) assert(picture.read(0x2000 + i * 0x400) == i + 31);
    json_t* saved = m->dataToJson();
    assert(NES::MapperMMC3::is_valid_state(saved));
    std::unique_ptr<NES::Cartridge> clone(cart->clone()); cart.reset();
    assert(clone->get_mapper()->readCHR(0x1C00) == 77);
    clone->get_mapper()->dataFromJson(saved); json_decref(saved);
    assert(std::remove(".build/mmc3.nes") == 0);
}

static void check_mmc3_irq() {
    write_mmc3_image(mmc3_image());
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(".build/mmc3.nes", []() {}));
    auto* m = cart->get_mapper();
    m->writePRG(0xC000, 1); m->writePRG(0xC001, 0); m->writePRG(0xE001, 0);
    for (int i = 0; i < 100; ++i) mmc3_edge(m, 9);
    assert(!m->irqPending());
    mmc3_edge(m); assert(!m->irqPending());  // Reload 1.
    // Pixel/read count cannot advance the low-time filter.
    m->observePPUAddress(0);
    for (int i = 0; i < 100; ++i) { m->readCHR(0); m->observePPUAddress(0); }
    m->observePPUAddress(0x1000); assert(!m->irqPending());
    mmc3_edge(m); assert(m->irqPending());
    m->writePRG(0xE000, 0); assert(!m->irqPending());
    m->writePRG(0xC000, 0); mmc3_edge(m); assert(!m->irqPending());
    m->writePRG(0xE001, 0); assert(!m->irqPending());
    mmc3_edge(m); assert(m->irqPending());  // MMC3B zero-reload event.
    json_t* saved = m->dataToJson();
    m->writePRG(0xE000, 0); m->dataFromJson(saved); assert(m->irqPending());
    json_decref(saved);
    // Save partway through a low period; resume must qualify the same edge.
    m->writePRG(0xE000, 0); m->writePRG(0xE001, 0); m->observePPUAddress(0);
    for (int i = 0; i < 8; ++i) m->clockPPU();
    saved = m->dataToJson();
    m->resetPPUObservation(); m->dataFromJson(saved);
    for (int i = 0; i < 8; ++i) m->clockPPU();
    m->observePPUAddress(0x1000); assert(m->irqPending()); json_decref(saved);
    // Actual PPU dots: hidden sprites/dummy slots still give one edge per line.
    NES::PictureBus bus; bus.set_mapper(m);
    NES::PPU ppu;
    for (int control : {0x08, 0x10}) for (int mask : {0x08, 0x10, 0x18, 0}) {
        ppu.reset(); ppu.control(control); ppu.set_mask(mask);
        std::array<NES::NES_Byte, 256> oam; oam.fill(0xFF); ppu.do_DMA(oam.data());
        m->writePRG(0xE000, 0); m->writePRG(0xC000, 0); m->writePRG(0xC001, 0);
        mmc3_edge(m); m->resetPPUObservation();
        m->writePRG(0xC000, 200); m->writePRG(0xC001, 0);
        for (int dot = 0; dot < 341 * 4; ++dot) ppu.cycle(bus);
        saved = m->dataToJson();
        const int counter = json_integer_value(json_object_get(saved, "irq_counter"));
        assert(counter == (mask ? 197 : 0));
        json_decref(saved);
    }
    // Mixed-table 8x16 sprites can produce multiple filtered edges; OAM order
    // matters, screen x/clipping does not. Snapshot the selected fetch addresses.
    ppu.reset(); ppu.control(0x20); ppu.set_mask(0x08);
    std::array<NES::NES_Byte, 256> oam; oam.fill(0xFF);
    for (int i = 0; i < 8; ++i) {
        oam[i * 4] = 0; oam[i * 4 + 1] = i & 1; oam[i * 4 + 2] = 0x80;
        oam[i * 4 + 3] = i * 30;
    }
    ppu.do_DMA(oam.data());
    saved = json_pack("{s:i,s:i,s:i}", "pipeline_state", 1, "cycles", 257, "scanline", 0);
    ppu.dataFromJson(saved); json_decref(saved);
    m->resetPPUObservation(); m->writePRG(0xC000, 200); m->writePRG(0xC001, 0);
    for (int i = 0; i < 64; ++i) ppu.cycle(bus);
    saved = m->dataToJson(); assert(json_integer_value(json_object_get(saved, "irq_counter")) == 197);
    json_decref(saved);
    saved = ppu.dataToJson();
    const auto* addresses = json_object_get(saved, "irq_sprite_addresses");
    for (int i = 0; i < 8; ++i)
        assert((json_integer_value(json_array_get(addresses, i)) & 0x1000) == ((i & 1) << 12));
    NES::PPU restored; restored.reset(); restored.dataFromJson(saved);
    json_t* actual = restored.dataToJson(); assert(json_equal(saved, actual));
    json_decref(actual); json_decref(saved);
    assert(std::remove(".build/mmc3.nes") == 0);
}

static void check_irq_sources() {
    auto bytes = mmc3_image(4, 8);
    const unsigned base = 16 + 3 * 0x2000;
    std::fill(bytes.begin() + 16, bytes.begin() + 16 + 0x8000, 0xEA);
    bytes[base] = 0x58;  // CLI
    bytes[base + 0x100] = 0x40; bytes[base + 0x200] = 0x40;  // RTI handlers
    bytes[base + 0x1FFA] = 0; bytes[base + 0x1FFB] = 0xE2;
    bytes[base + 0x1FFC] = 0; bytes[base + 0x1FFD] = 0xE0;
    bytes[base + 0x1FFE] = 0; bytes[base + 0x1FFF] = 0xE1;
    write_mmc3_image(bytes);
    std::unique_ptr<NES::Cartridge> cart(NES::Cartridge::create(".build/mmc3.nes", []() {}));
    NES::MainBus bus; bus.set_mapper(cart->get_mapper());
    NES::CPU cpu; cpu.reset(bus);
    const auto pc = [&]() {
        json_t* s = cpu.dataToJson(); int value = json_integer_value(json_object_get(s, "register_PC"));
        json_decref(s); return value;
    };
    cpu.cycle(bus, true); assert(pc() == 0xE001);  // Masked: execute CLI.
    cpu.cycle(bus, true); assert(pc() == 0xE001);  // Finish instruction.
    cpu.cycle(bus, true); assert(pc() == 0xE100);
    assert(bus.read(0x1FB) == 0x20);  // IRQ stack: U set, B/I clear.
    for (int i = 0; i < 7; ++i) cpu.cycle(bus, false);
    assert(pc() == 0xE001);  // RTI used hardware flag positions.
    cpu.reset(bus); cpu.cycle(bus); cpu.cycle(bus);
    cpu.skip_DMA_cycles(); cpu.request_nmi();
    json_t* pending = cpu.dataToJson();
    NES::CPU clone; clone.dataFromJson(pending); json_decref(pending);
    for (int i = 0; i < 513; ++i) cpu.cycle(bus, true);
    assert(pc() == 0xE001);  // Both sources wait through DMA.
    cpu.cycle(bus, true); assert(pc() == 0xE200);  // NMI wins.
    json_t* cloned = clone.dataToJson(); assert(json_is_true(json_object_get(cloned, "nmi_pending")));
    json_decref(cloned);
    // Hardware status stack encoding for PHP/PLP and BRK, independent of JSON.
    cpu.reset(bus);
    json_t* cpu_state = cpu.dataToJson();
    json_object_set_new(cpu_state, "register_PC", json_integer(0));
    cpu.dataFromJson(cpu_state); json_decref(cpu_state);
    bus.write(0, 0x08); bus.write(1, 0x28);  // PHP, PLP
    cpu.cycle(bus); assert(bus.read(0x1FD) == 0x34);
    bus.write(0x1FD, 0xC3);  // N,V,Z,C set; I clear.
    for (int i = 0; i < 3; ++i) cpu.cycle(bus);
    bus.write(2, 0x08);
    for (int i = 0; i < 4; ++i) cpu.cycle(bus);
    assert(bus.read(0x1FD) == 0xF3);
    cpu.reset(bus); cpu_state = cpu.dataToJson();
    json_object_set_new(cpu_state, "register_PC", json_integer(0));
    cpu.dataFromJson(cpu_state); json_decref(cpu_state); bus.write(0, 0);
    cpu.cycle(bus); assert(pc() == 0xE100 && bus.read(0x1FC) == 2);
    assert(bus.read(0x1FB) == 0x34);
    for (int i = 0; i < 6; ++i) cpu.cycle(bus);
    assert(pc() == 0xE100); cpu.cycle(bus); assert(pc() == 2);
    // APU polling must neither report future schedules nor acknowledge flags.
    NES::APU apu; apu.reset(); apu.set_dmc_reader([](void*, cpu_addr_t) { return 0x55; });
    assert(!apu.irq_pending());
    for (int i = 0; i < 31000; ++i) apu.cycle();
    assert(apu.irq_pending()); assert(apu.irq_pending());
    json_t* apu_state = apu.dataToJson();
    NES::APU restored_apu; restored_apu.reset();
    restored_apu.set_dmc_reader([](void*, cpu_addr_t) { return 0x55; });
    restored_apu.dataFromJson(apu_state); json_decref(apu_state);
    assert(restored_apu.irq_pending());
    auto* m = cart->get_mapper(); m->writePRG(0xE001, 0); mmc3_edge(m);
    assert(m->irqPending());
    assert(apu.read_status() & 0x40); assert(!apu.irq_pending()); assert(m->irqPending());
    apu.write(0x4017, 0x40);  // Inhibit frame IRQ while testing DMC.
    apu.write(0x4010, 0x8F); apu.write(0x4012, 0); apu.write(0x4013, 0); apu.write(0x4015, 0x10);
    for (int i = 0; i < 1000; ++i) apu.cycle();
    assert(apu.irq_pending()); assert(apu.read_status() & 0x80); assert(apu.irq_pending());
    m->writePRG(0xE000, 0); assert(!m->irqPending() && apu.irq_pending());
    apu.write(0x4015, 0); assert(!apu.irq_pending());
    // DMC refills observe live PRG banks, including MMC3's inverted C000 slot.
    NES::APU dmc; dmc.reset(); bool saw_one = false, saw_two = false;
    dmc.set_dmc_reader([&](void*, cpu_addr_t address) {
        const auto value = bus.read(address);
        saw_one |= value == 1; saw_two |= value == 2;
        return value;
    });
    // Use distinct bytes in RAM-independent ROM banks from a fresh marker image.
    write_mmc3_image(mmc3_image(4, 8));
    std::unique_ptr<NES::Cartridge> marker(NES::Cartridge::create(".build/mmc3.nes", []() {}));
    bus.set_mapper(marker->get_mapper());
    bus.write(0x8000, 0x46); bus.write(0x8001, 1);
    dmc.write(0x4010, 0x4F); dmc.write(0x4012, 0); dmc.write(0x4013, 0); dmc.write(0x4015, 0x10);
    for (int i = 0; i < 1000; ++i) dmc.cycle();
    bus.write(0x8001, 2);
    for (int i = 0; i < 1000; ++i) dmc.cycle();
    assert(saw_one && saw_two);
    assert(std::remove(".build/mmc3.nes") == 0);
}

/// Whole CPU/PPU/mapper IRQ path, patch snapshots and rejected image/state.
static void check_mmc3_emulator() {
    auto bytes = mmc3_image(4, 8);
    const unsigned start = 16 + 3 * 0x2000;
    unsigned pc = start;
    bytes[pc++] = 0x78;
    const auto write = [&](unsigned address, unsigned value) {
        bytes[pc++] = 0xA9; bytes[pc++] = value;
        bytes[pc++] = 0x8D; bytes[pc++] = address & 255; bytes[pc++] = address >> 8;
    };
    write(0x4017, 0x40); write(0x2000, 8); write(0x2001, 0x18);
    write(0xC000, 2); write(0xC001, 0); write(0xE001, 0);
    bytes[pc++] = 0x58;  // CLI
    const unsigned loop = 0xE000 + pc - start;
    bytes[pc++] = 0x4C; bytes[pc++] = loop & 255; bytes[pc++] = loop >> 8;
    pc = start + 0x100;
    bytes[pc++] = 0xE6; bytes[pc++] = 0x46;  // INC $46
    write(0xE000, 0); write(0xE001, 0); bytes[pc++] = 0x40;
    bytes[start + 0x1FFC] = 0; bytes[start + 0x1FFD] = 0xE0;
    bytes[start + 0x1FFE] = 0; bytes[start + 0x1FFF] = 0xE1;
    write_mmc3_image(bytes);
    std::unique_ptr<RackNES> module(new RackNES);
    assert(module->emulator.load_game(".build/mmc3.nes"));
    for (int i = 0; i < 20000; ++i) module->emulator.cycle([]() {});
    assert(module->emulator.get_memory_buffer()[0x46] > 30);
    assert(module->emulator.get_memory_buffer()[0x46] < 100);
    json_t* state = module->emulator.dataToJson();
    json_t* mapper = json_object_get(json_object_get(state, "cartridge"), "mapper");
    json_object_set_new(mapper, "irq_pending", json_true());
    assert(module->emulator.dataFromJson(state));
    module->processCV(); module->params[RackNES::PARAM_SAVE].setValue(1.f); module->processCV();
    json_t* patch = module->dataToJson();
    std::unique_ptr<RackNES> restored(new RackNES); restored->dataFromJson(patch);
    assert(!restored->rom_reload_failed_signal); module.reset(); json_decref(patch);
    restored->processCV(); restored->params[RackNES::PARAM_LOAD].setValue(1.f); restored->processCV();
    json_t* actual = restored->emulator.dataToJson();
    assert(json_equal(mapper, json_object_get(json_object_get(actual, "cartridge"), "mapper")));
    json_decref(actual);
    for (int i = 0; i < 8; ++i) {
        json_t* bad = json_deep_copy(state);
        json_t* m = json_object_get(json_object_get(bad, "cartridge"), "mapper");
        if (i == 0) json_object_set_new(m, "irq_counter", json_integer(256));
        if (i == 1) json_object_set_new(m, "bank_select", json_integer(8));
        if (i == 2) json_object_set_new(m, "a12_low_dots", json_integer(17));
        if (i == 3) json_object_set_new(m, "irq_pending", json_integer(1));
        if (i == 4) json_object_set_new(m, "chr_ram", json_string("AA=="));
        if (i == 5) json_array_remove(json_object_get(m, "banks"), 0);
        if (i == 6) json_array_set_new(json_object_get(m, "banks"), 1, json_integer(-1));
        if (i == 7) json_object_set_new(json_object_get(bad, "ppu"), "irq_sprite_addresses", json_array());
        assert(!restored->emulator.dataFromJson(bad)); json_decref(bad);
    }
    for (int kind = 0; kind < 12; ++kind) {
        auto bad = bytes;
        switch (kind) {
            case 0: bad.pop_back(); break;
            case 1: bad.push_back(0); break;
            case 2: bad[6] |= 4; break;
            case 3: bad[7] = 1; break;
            case 4: bad[4] = 3; break;
            case 5: bad[5] = 3; break;
            case 6: bad[7] = 8; bad[8] = 0x10; break;
            case 7: bad[7] = 8; bad[10] = 0; break;
            case 8: bad[9] = 1; break;
            case 9: bad[8] = 2; break;
            case 10: bad[7] = 8; bad[12] = 1; break;
            case 11: bad[0] = 'X'; break;
        }
        write_mmc3_image(bad); assert(!restored->emulator.load_game(".build/mmc3.nes"));
    }
    for (int ram : {0, 8}) for (int battery : {0, 2}) {
        auto valid = mmc3_image(4, ram);
        valid[6] |= battery; valid[7] = 8; valid[10] = battery ? 0x70 : 7;
        valid[11] = ram ? 0 : 7; write_mmc3_image(valid);
        assert(restored->emulator.load_game(".build/mmc3.nes"));
    }
    json_decref(state); assert(std::remove(".build/mmc3.nes") == 0);
}

#endif  // RACKNES_TESTS_MMC3_HPP

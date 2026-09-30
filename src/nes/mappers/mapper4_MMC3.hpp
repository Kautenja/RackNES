//  Program:      nes-py
//  File:         mapper_MMC3.hpp
//  Description:  MMC3B/C banking, RAM protection and filtered A12 IRQs
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//
//  Adapted from nes-py 301da52f7f75de38 (MIT), mapper_MMC3.hpp/.cpp.
//  RackNES uses elapsed PPU dots, level IRQs and banked CHR RAM.
//  See docs/licenses/THIRD-PARTY.txt.

#ifndef NES_MAPPERS_MAPPER_MMC3_HPP
#define NES_MAPPERS_MAPPER_MMC3_HPP

#include <algorithm>
#include <array>
#include "../rom.hpp"

namespace NES {

/// Common MMC3B/C TxROM; MMC6 and alternate IRQ silicon are excluded.
class MapperMMC3 : public ROM::Mapper {
 private:
    // Conservative elapsed-dot filter for this scanline renderer. It rejects
    // the nine-dot gap at a reversed-table line boundary, while admitting
    // mixed 8x16 sprite gaps. Hardware M2 phase is not modeled.
    static constexpr unsigned A12_LOW_DOTS = 10;
    NES_Byte bank_select = 0;
    std::array<NES_Byte, 8> banks = {{0, 2, 4, 5, 6, 7, 0, 1}};
    NES_Byte ram_control = 0x80;
    bool horizontal;
    Callback mirroring_callback;
    std::array<NES_Byte, 0x2000> chr_ram = {};
    NES_Byte irq_latch = 0, irq_counter = 0;
    bool irq_reload = false, irq_enabled = false, irq_pending = false;
    bool a12_high = false;
    /// Saturating low duration in PPU dots, independent of renderer reads.
    NES_Byte a12_low_dots = 0;

    std::size_t chr_offset(NES_Address address) const {
        unsigned slot = (address & 0x1FFF) / 0x400;
        if (bank_select & 0x80) slot ^= 4;
        const unsigned bank = slot < 4 ? (banks[slot / 2] & 0xFE) + (slot & 1) :
            banks[slot - 2];
        const std::size_t size = rom.getVROM().empty() ? chr_ram.size() : rom.getVROM().size();
        return (bank % (size / 0x400)) * 0x400 + (address & 0x3FF);
    }

 public:
    /// Exact standard NTSC layouts; NES 2.0 submapper 0 uses MMC3B/C behavior.
    static bool supports(const std::array<NES_Byte, 16>& h, std::streamoff size) {
        if (h[0] != 'N' || h[1] != 'E' || h[2] != 'S' || h[3] != 0x1A ||
            (h[6] & 0xF4) != 0x40 || h[4] < 2 || h[4] > 32 ||
            (h[4] & (h[4] - 1)) || h[5] > 32 || (h[5] & (h[5] - 1)) ||
            size != 16 + static_cast<std::streamoff>(h[4]) * 0x4000 + h[5] * 0x2000)
            return false;
        if (h[7] == 0) {
            if (h[8] > 1) return false;
            for (std::size_t i = 9; i < h.size(); ++i) if (h[i]) return false;
            return true;
        }
        if (h[7] != 8 || h[8] || h[9] ||
            h[10] != (h[6] & 2 ? 0x70 : 7) || h[11] != (h[5] ? 0 : 7))
            return false;
        for (std::size_t i = 12; i < h.size(); ++i) if (h[i]) return false;
        return true;
    }

    MapperMMC3(ROM& cart, Callback callback) : Mapper(cart),
        horizontal(cart.getNameTableMirroring() == HORIZONTAL), mirroring_callback(callback) { }

    MapperMMC3(ROM& cart, const MapperMMC3& other, Callback callback) :
        Mapper(cart), bank_select(other.bank_select), banks(other.banks),
        ram_control(other.ram_control), horizontal(other.horizontal),
        mirroring_callback(callback), chr_ram(other.chr_ram), irq_latch(other.irq_latch),
        irq_counter(other.irq_counter), irq_reload(other.irq_reload),
        irq_enabled(other.irq_enabled), irq_pending(other.irq_pending),
        a12_high(other.a12_high), a12_low_dots(other.a12_low_dots) { }

    MapperMMC3* clone() override { return new MapperMMC3(*this); }
    bool hasExtendedRAM() const override { return true; }
    bool canReadPRGRAM() const override { return ram_control & 0x80; }
    bool canWritePRGRAM() const override { return (ram_control & 0xC0) == 0x80; }
    bool observesPPUAddresses() const override { return true; }
    bool irqPending() const override { return irq_pending; }
    NameTableMirroring getNameTableMirroring() const override {
        if (rom.getNameTableMirroring() == FOUR_SCREEN) return FOUR_SCREEN;
        return horizontal ? HORIZONTAL : VERTICAL;
    }

    NES_Byte readPRG(NES_Address address) override {
        const auto& memory = rom.getROM();
        const std::size_t count = memory.size() / 0x2000;
        unsigned slot = (address & 0x7FFF) / 0x2000;
        if ((bank_select & 0x40) && !(slot & 1)) slot ^= 2;
        const std::size_t bank = slot == 0 ? banks[6] & 0x3F :
            slot == 1 ? banks[7] & 0x3F : count - 4 + slot;
        return memory[(bank % count) * 0x2000 + (address & 0x1FFF)];
    }

    void writePRG(NES_Address address, NES_Byte value) override {
        switch (address & 0xE001) {
            case 0x8000: bank_select = value & 0xC7; break;
            case 0x8001: banks[bank_select & 7] = value; break;
            case 0xA000:
                horizontal = value & 1;
                if (mirroring_callback) mirroring_callback();
                break;
            case 0xA001: ram_control = value & 0xC0; break;
            case 0xC000: irq_latch = value; break;
            case 0xC001: irq_reload = true; break;
            case 0xE000: irq_enabled = irq_pending = false; break;
            case 0xE001: irq_enabled = true; break;
        }
    }

    NES_Byte readCHR(NES_Address address) override {
        const auto offset = chr_offset(address);
        return rom.getVROM().empty() ? chr_ram[offset] : rom.getVROM()[offset];
    }
    void writeCHR(NES_Address address, NES_Byte value) override {
        if (rom.getVROM().empty()) chr_ram[chr_offset(address)] = value;
    }

    /// CPU accesses observe address changes without advancing elapsed time.
    void observePPUAddress(NES_Address address) override {
        const bool high = address & 0x1000;
        if (high && !a12_high && a12_low_dots >= A12_LOW_DOTS) {
            if (!irq_counter || irq_reload) irq_counter = irq_latch;
            else --irq_counter;
            irq_reload = false;
            if (!irq_counter && irq_enabled) irq_pending = true;
        }
        if (high || a12_high) a12_low_dots = 0;
        a12_high = high;
    }
    /// Called once per PPU dot, including CPU/DMA stalls.
    void clockPPU() override {
        if (!a12_high && a12_low_dots < A12_LOW_DOTS) ++a12_low_dots;
    }
    void resetPPUObservation() override { a12_high = false; a12_low_dots = 0; }

    static bool is_valid_state(json_t* root) {
        if (!json_is_object(root)) return false;
        const char* integers[] = {"bank_select", "ram_control", "irq_latch", "irq_counter", "a12_low_dots"};
        for (const char* key : integers) {
            json_t* value = json_object_get(root, key);
            if (!json_is_integer(value) || json_integer_value(value) < 0 ||
                json_integer_value(value) > 255) return false;
        }
        if ((json_integer_value(json_object_get(root, "bank_select")) & ~0xC7) ||
            (json_integer_value(json_object_get(root, "ram_control")) & ~0xC0) ||
            json_integer_value(json_object_get(root, "a12_low_dots")) > A12_LOW_DOTS) return false;
        for (const char* key : {"horizontal", "irq_reload", "irq_enabled", "irq_pending", "a12_high"})
            if (!json_is_boolean(json_object_get(root, key))) return false;
        if ((json_is_true(json_object_get(root, "irq_pending")) &&
             !json_is_true(json_object_get(root, "irq_enabled"))) ||
            (json_is_true(json_object_get(root, "a12_high")) &&
             json_integer_value(json_object_get(root, "a12_low_dots")))) return false;
        json_t* registers = json_object_get(root, "banks");
        if (!json_is_array(registers) || json_array_size(registers) != 8) return false;
        for (int i = 0; i < 8; ++i) {
            json_t* v = json_array_get(registers, i);
            if (!json_is_integer(v) || json_integer_value(v) < 0 || json_integer_value(v) > 255)
                return false;
        }
        json_t* ram = json_object_get(root, "chr_ram");
        return json_is_string(ram) && json_string_length(ram) == 10924 && base64_decode(json_string_value(ram)).size() == 0x2000;
    }

    json_t* dataToJson() override {
        json_t* root = json_pack("{s:i,s:i,s:i,s:i,s:i,s:b,s:b,s:b,s:b,s:b}",
            "bank_select", bank_select, "ram_control", ram_control,
            "irq_latch", irq_latch, "irq_counter", irq_counter, "a12_low_dots", a12_low_dots,
            "horizontal", horizontal, "irq_reload", irq_reload, "irq_enabled", irq_enabled,
            "irq_pending", irq_pending, "a12_high", a12_high);
        json_t* registers = json_array();
        for (NES_Byte b : banks) json_array_append_new(registers, json_integer(b));
        json_object_set_new(root, "banks", registers);
        json_object_set_new(root, "chr_ram", json_string(base64_encode(chr_ram.data(), chr_ram.size()).c_str()));
        return root;
    }

    void dataFromJson(json_t* root) override {
        if (!is_valid_state(root)) return;
        bank_select = json_integer_value(json_object_get(root, "bank_select"));
        ram_control = json_integer_value(json_object_get(root, "ram_control"));
        irq_latch = json_integer_value(json_object_get(root, "irq_latch"));
        irq_counter = json_integer_value(json_object_get(root, "irq_counter"));
        a12_low_dots = json_integer_value(json_object_get(root, "a12_low_dots"));
        horizontal = json_is_true(json_object_get(root, "horizontal"));
        irq_reload = json_is_true(json_object_get(root, "irq_reload"));
        irq_enabled = json_is_true(json_object_get(root, "irq_enabled"));
        irq_pending = json_is_true(json_object_get(root, "irq_pending"));
        a12_high = json_is_true(json_object_get(root, "a12_high"));
        for (int i = 0; i < 8; ++i) banks[i] = json_integer_value(json_array_get(json_object_get(root, "banks"), i));
        const auto data = base64_decode(json_string_value(json_object_get(root, "chr_ram")));
        std::copy(data.begin(), data.end(), chr_ram.begin());
        if (mirroring_callback) mirroring_callback();
    }
};

}  // namespace NES
#endif  // NES_MAPPERS_MAPPER_MMC3_HPP

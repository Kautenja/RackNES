//  Program:      nes-py
//  File:         mapper_MMC2.hpp
//  Description:  MMC2/PxROM banking and post-read CHR latches for RackNES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//
//  Adapted from nes-py 301da52f7f75de38 (MIT), mapper_MMC2.hpp/.cpp
//  and mapper_bank.hpp. See docs/licenses/THIRD-PARTY.txt.

#ifndef NES_MAPPERS_MAPPER_MMC2_HPP
#define NES_MAPPERS_MAPPER_MMC2_HPP

#include <array>
#include "../rom.hpp"

namespace NES {

/// Mapper 9: one switchable PRG bank, three fixed banks and two CHR latches.
class MapperMMC2 : public ROM::Mapper {
 private:
    NES_Byte register_prg = 0;
    /// FD/FE banks for the lower half, then FD/FE banks for the upper half.
    std::array<NES_Byte, 4> register_chr = {};
    bool left_latch_fe = true;
    bool right_latch_fe = true;
    bool horizontal;
    Callback mirroring_callback;

 public:
    /// Accept standard NTSC PxROM with 8 KiB PRG RAM and banked CHR ROM.
    static bool supports(const std::array<NES_Byte, 16>& header,
                         std::streamoff file_size) {
        if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' ||
            header[3] != 0x1A || (header[6] & 0xFC) != 0x90)
            return false;
        const unsigned prg = header[4], chr = header[5];
        if (prg < 2 || prg > 8 || (prg & (prg - 1)) ||
            chr < 1 || chr > 16 || (chr & (chr - 1)) ||
            file_size != 16 + static_cast<std::streamoff>(prg) * 0x4000 + chr * 0x2000)
            return false;
        if (header[7] == 0) {
            if (header[8] > 1) return false;
            for (std::size_t i = 9; i < header.size(); ++i)
                if (header[i]) return false;
            return true;
        }
        // NES 2.0 submapper 0 only: explicit 8 KiB RAM (or battery RAM),
        // no CHR RAM, extended sizes, alternate timing or extra devices.
        if (header[7] != 8 || header[8] || header[9] ||
            header[10] != (header[6] & 2 ? 0x70 : 7) || header[11])
            return false;
        for (std::size_t i = 12; i < header.size(); ++i)
            if (header[i]) return false;
        return true;
    }

    MapperMMC2(ROM& cart, Callback callback) : Mapper(cart),
        horizontal(cart.getNameTableMirroring() == HORIZONTAL),
        mirroring_callback(callback) { }

    /// Rebind a cartridge clone to its own ROM and picture-bus callback.
    MapperMMC2(ROM& cart, const MapperMMC2& other, Callback callback) :
        Mapper(cart), register_prg(other.register_prg), register_chr(other.register_chr),
        left_latch_fe(other.left_latch_fe), right_latch_fe(other.right_latch_fe),
        horizontal(other.horizontal), mirroring_callback(callback) { }

    /// Standalone mapper copies borrow their source ROM, like existing mappers.
    MapperMMC2* clone() override { return new MapperMMC2(*this); }
    bool hasExtendedRAM() const override { return true; }
    bool hasCHRReadLatches() const override { return true; }
    NameTableMirroring getNameTableMirroring() const override {
        return horizontal ? HORIZONTAL : VERTICAL;
    }

    NES_Byte readPRG(NES_Address address) override {
        const auto& memory = rom.getROM();
        const std::size_t count = memory.size() / 0x2000;
        if (count < 4) return 0;
        const std::size_t window = (address & 0x7FFF) / 0x2000;
        const std::size_t bank = window ? count - 4 + window : register_prg % count;
        return memory[bank * 0x2000 + (address & 0x1FFF)];
    }

    void writePRG(NES_Address address, NES_Byte value) override {
        switch (address & 0xF000) {
            case 0xA000: register_prg = value & 0x0F; break;
            case 0xB000: case 0xC000: case 0xD000: case 0xE000:
                register_chr[(address >> 12) - 0xB] = value & 0x1F;
                break;
            case 0xF000:
                if (horizontal != bool(value & 1)) {
                    horizontal = value & 1;
                    if (mirroring_callback) mirroring_callback();
                }
                break;
            default: break;
        }
    }

    /// Read and latch as one operation; the triggering byte uses the old bank.
    NES_Byte readCHR(NES_Address address) override {
        address &= 0x1FFF;
        const auto& memory = rom.getVROM();
        const std::size_t count = memory.size() / 0x1000;
        if (!count) return 0;
        const unsigned reg = address < 0x1000 ? unsigned(left_latch_fe) :
            2 + unsigned(right_latch_fe);
        const std::size_t bank = register_chr[reg] % count;
        const NES_Byte value = memory[bank * 0x1000 + (address & 0x0FFF)];
        if (address == 0x0FD8) left_latch_fe = false;
        else if (address == 0x0FE8) left_latch_fe = true;
        else if (address >= 0x1FD8 && address <= 0x1FDF) right_latch_fe = false;
        else if (address >= 0x1FE8 && address <= 0x1FEF) right_latch_fe = true;
        return value;
    }

    /// Supported images contain read-only CHR; writes do not clock latches.
    void writeCHR(NES_Address, NES_Byte) override { }

    static bool is_valid_state(json_t* root) {
        if (!json_is_object(root)) return false;
        json_t* prg = json_object_get(root, "register_prg");
        if (!json_is_integer(prg) || json_integer_value(prg) < 0 ||
            json_integer_value(prg) > 15) return false;
        json_t* chr = json_object_get(root, "register_chr");
        if (!json_is_array(chr) || json_array_size(chr) != 4) return false;
        for (std::size_t i = 0; i < 4; ++i) {
            json_t* bank = json_array_get(chr, i);
            if (!json_is_integer(bank) || json_integer_value(bank) < 0 ||
                json_integer_value(bank) > 31) return false;
        }
        return json_is_boolean(json_object_get(root, "left_latch_fe")) &&
            json_is_boolean(json_object_get(root, "right_latch_fe")) &&
            json_is_boolean(json_object_get(root, "horizontal"));
    }

    json_t* dataToJson() override {
        json_t* root = json_pack("{s:i,s:b,s:b,s:b}", "register_prg", register_prg,
            "left_latch_fe", left_latch_fe, "right_latch_fe", right_latch_fe,
            "horizontal", horizontal);
        json_t* chr = json_array();
        for (NES_Byte bank : register_chr) json_array_append_new(chr, json_integer(bank));
        json_object_set_new(root, "register_chr", chr);
        return root;
    }

    void dataFromJson(json_t* root) override {
        if (!is_valid_state(root)) return;
        register_prg = json_integer_value(json_object_get(root, "register_prg"));
        for (std::size_t i = 0; i < 4; ++i)
            register_chr[i] = json_integer_value(json_array_get(
                json_object_get(root, "register_chr"), i));
        left_latch_fe = json_is_true(json_object_get(root, "left_latch_fe"));
        right_latch_fe = json_is_true(json_object_get(root, "right_latch_fe"));
        horizontal = json_is_true(json_object_get(root, "horizontal"));
        if (mirroring_callback) mirroring_callback();
    }
};

}  // namespace NES

#endif  // NES_MAPPERS_MAPPER_MMC2_HPP

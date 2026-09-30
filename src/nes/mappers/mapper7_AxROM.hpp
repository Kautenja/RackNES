//  Program:      nes-py
//  File:         mapper_AxROM.hpp
//  Description:  AxROM banking adapted for RackNES callbacks and JSON
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//
//  Adapted from nes-py 301da52f7f75de38 (MIT), mapper_AxROM.hpp/.cpp
//  and mapper_bank.hpp. See docs/licenses/THIRD-PARTY.txt.

#ifndef NES_MAPPERS_MAPPER_AXROM_HPP
#define NES_MAPPERS_MAPPER_AXROM_HPP

#include <algorithm>
#include <array>
#include <string>
#include "../rom.hpp"

namespace NES {

/// Mapper 7: one 32 KiB PRG window, 8 KiB CHR RAM and one-screen mirroring.
class MapperAxROM : public ROM::Mapper {
 private:
    /// PRG bits 0--2 and nametable page bit 4; other bits are not connected.
    NES_Byte bank_select = 0;
    /// Writable pattern memory, allocated with the mapper before playback.
    std::array<NES_Byte, 0x2000> character_ram = {};
    /// NES 2.0 submapper 2 resolves writes against the old PRG bank.
    bool bus_conflicts;
    /// Notify the owning picture bus only after a mirroring change.
    Callback mirroring_callback;

 public:
    /// Accept only bounded standard CHR-RAM layouts before ROM allocation.
    /// iNES and NES 2.0 submappers 0/1 use the upstream no-conflict policy.
    static bool supports(const std::array<NES_Byte, 16>& header,
                         std::streamoff file_size) {
        if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' ||
            header[3] != 0x1A || (header[6] & 0xFE) != 0x70 || header[5])
            return false;
        // Whole power-of-two 32 KiB banks, up to the three-bit latch limit.
        const unsigned banks = header[4];
        if (banks < 2 || banks > 16 || (banks & (banks - 1)) ||
            file_size != 16 + static_cast<std::streamoff>(banks) * 0x4000)
            return false;
        if (header[7] == 0) {
            // Legacy RAM-size zero/one is commonly present even without RAM.
            if (header[8] > 1) return false;
            for (std::size_t i = 9; i < header.size(); ++i)
                if (header[i]) return false;
            return true;
        }
        // NES 2.0: mapper 7, submapper 0/1/2, 8 KiB volatile CHR RAM,
        // no PRG RAM, no extended ROM sizes, NTSC and no extra devices.
        if (header[7] != 0x08 || (header[8] & 0x0F) ||
            (header[8] >> 4) > 2 || header[9] || header[10] || header[11] != 7)
            return false;
        for (std::size_t i = 12; i < header.size(); ++i)
            if (header[i]) return false;
        return true;
    }

    /// Create a freshly powered mapper; the factory attaches the bus later.
    MapperAxROM(ROM& cart, Callback callback, bool conflicts = false) :
        Mapper(cart), bus_conflicts(conflicts), mirroring_callback(callback) { }

    /// Copy state while explicitly rebinding ROM storage and the bus callback.
    MapperAxROM(ROM& cart, const MapperAxROM& other, Callback callback) :
        Mapper(cart), bank_select(other.bank_select),
        character_ram(other.character_ram), bus_conflicts(other.bus_conflicts),
        mirroring_callback(callback) { }

    /// A standalone mapper copy borrows the same ROM; cartridge copies rebind.
    MapperAxROM* clone() override { return new MapperAxROM(*this); }

    NameTableMirroring getNameTableMirroring() const override {
        return bank_select & 0x10 ? ONE_SCREEN_HIGHER : ONE_SCREEN_LOWER;
    }

    NES_Byte readPRG(NES_Address address) override {
        const auto& memory = rom.getROM();
        const std::size_t count = memory.size() / 0x8000;
        if (!count) return 0;
        const std::size_t bank = (bank_select & 7) % count;
        return memory[bank * 0x8000 + (address & 0x7FFF)];
    }

    void writePRG(NES_Address address, NES_Byte value) override {
        if (bus_conflicts) value &= readPRG(address);
        const bool changed_page = (bank_select ^ value) & 0x10;
        bank_select = value & 0x17;
        if (changed_page && mirroring_callback) mirroring_callback();
    }

    NES_Byte readCHR(NES_Address address) override {
        return character_ram[address & 0x1FFF];
    }

    void writeCHR(NES_Address address, NES_Byte value) override {
        character_ram[address & 0x1FFF] = value;
    }

    /// Reject malformed mapper state before changing the live cartridge.
    static bool is_valid_state(json_t* root) {
        if (!json_is_object(root)) return false;
        json_t* bank = json_object_get(root, "bank_select");
        json_t* ram = json_object_get(root, "character_ram");
        const json_int_t value = json_integer_value(bank);
        if (!json_is_integer(bank) || value < 0 || (value & ~0x17) ||
            !json_is_string(ram) || json_string_length(ram) != 10924)
            return false;
        const std::string encoded(json_string_value(ram), json_string_length(ram));
        // Validate the alphabet/padding before the bundled decoder can throw.
        if (encoded.back() != '=') return false;
        for (std::size_t i = 0; i + 1 < encoded.size(); ++i) {
            const char c = encoded[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '+' || c == '/'))
                return false;
        }
        const std::string decoded = base64_decode(encoded);
        return decoded.size() == 0x2000 &&
            base64_encode(reinterpret_cast<const unsigned char*>(decoded.data()),
                          decoded.size()) == encoded;
    }

    json_t* dataToJson() override {
        const std::string ram = base64_encode(character_ram.data(), character_ram.size());
        return json_pack("{s:i,s:s}", "bank_select", bank_select,
                         "character_ram", ram.c_str());
    }

    void dataFromJson(json_t* root) override {
        if (!is_valid_state(root)) return;
        const std::string ram = base64_decode(
            json_string_value(json_object_get(root, "character_ram")));
        std::copy(ram.begin(), ram.end(), character_ram.begin());
        bank_select = static_cast<NES_Byte>(
            json_integer_value(json_object_get(root, "bank_select")));
        if (mirroring_callback) mirroring_callback();
    }
};

}  // namespace NES

#endif  // NES_MAPPERS_MAPPER_AXROM_HPP

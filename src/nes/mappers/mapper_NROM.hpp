//  Program:      nes-py
//  File:         mapper_NROM.hpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_NROM_HPP
#define NES_MAPPERS_MAPPER_NROM_HPP

#include <vector>
#include "../rom.hpp"
#include "../log.hpp"

namespace NES {

/// The NROM mapper (mapper #0).
class MapperNROM : public ROM::Mapper {
 private:
    /// whether there are 1 or 2 banks
    bool is_one_bank;
    /// whether this mapper uses character RAM
    bool has_character_ram;
    /// the character RAM on the mapper
    std::vector<NES_Byte> character_ram;

 public:
    /// Create a new mapper with a rom.
    ///
    /// @param cart a reference to a rom for the mapper to access
    ///
    explicit MapperNROM(ROM& cart) : Mapper(cart),
        is_one_bank(rom.getROM().size() == 0x4000),
        has_character_ram(rom.getVROM().size() == 0) {
        if (has_character_ram) {
            character_ram.resize(0x2000);
            LOG(Info) << "Uses character RAM" << std::endl;
        }
    }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    inline NES_Byte readPRG(NES_Address address) override {
        if (!is_one_bank)
            return rom.getROM()[address - 0x8000];
        else  // mirrored
            return rom.getROM()[(address - 0x8000) & 0x3fff];
    }

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writePRG(NES_Address address, NES_Byte value) override {
        LOG(InfoVerbose) << "ROM memory write attempt at " <<
            +address << " to set " << +value << std::endl;
    }

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) override {
        if (has_character_ram)
            return character_ram[address];
        else
            return rom.getVROM()[address];
    }

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writeCHR(NES_Address address, NES_Byte value) override {
        if (has_character_ram)
            character_ram[address] = value;
        else
            LOG(Info) << "Read-only CHR memory write attempt at " <<
                std::hex << address << std::endl;
    }
};

}  // namespace NES

#endif // NES_MAPPERS_MAPPER_NROM_HPP

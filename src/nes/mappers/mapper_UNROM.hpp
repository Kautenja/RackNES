//  Program:      nes-py
//  File:         mapper_UNROM.hpp
//  Description:  An implementation of the UNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_UNROM_HPP
#define NES_MAPPERS_MAPPER_UNROM_HPP

#include <vector>
#include "../cartridge.hpp"
#include "../log.hpp"

namespace NES {

/// The UxROM mapper (mapper #2).
class MapperUNROM : public Cartridge::Mapper {
 private:
    /// whether the cartridge use character RAM
    bool has_character_ram;
    /// the pointer to the last bank
    std::size_t last_bank_pointer;
    /// TODO: what is this?
    NES_Address select_prg;
    /// The character RAM on the mapper
    std::vector<NES_Byte> character_ram;

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperUNROM(Cartridge& cart): Mapper(cart),
        has_character_ram(cartridge.getVROM().size() == 0),
        last_bank_pointer(cartridge.getROM().size() - 0x4000),
        select_prg(0) {
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
        if (address < 0xc000)
            return cartridge.getROM()[((address - 0x8000) & 0x3fff) | (select_prg << 14)];
        else
            return cartridge.getROM()[last_bank_pointer + (address & 0x3fff)];
    }

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writePRG(NES_Address address, NES_Byte value) override {
        select_prg = value;
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
            return cartridge.getVROM()[address];
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

#endif  // NES_MAPPERS_MAPPER_UNROM_HPP

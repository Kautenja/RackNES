//  Program:      nes-py
//  File:         mapper_CNROM.hpp
//  Description:  An implementation of the CNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_CNROM_HPP
#define NES_MAPPERS_MAPPER_CNROM_HPP

#include "../mapper.hpp"
#include "../log.hpp"

namespace NES {

/// The CNROM mapper (mapper #3).
class MapperCNROM : public Mapper {
 private:
    /// whether there are 1 or 2 banks
    bool is_one_bank;
    /// TODO: what is this value
    NES_Address select_chr;

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperCNROM(Cartridge* cart) :
        Mapper(cart),
        is_one_bank(cart->getROM().size() == 0x4000),
        select_chr(0) { }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    inline NES_Byte readPRG(NES_Address address) override {
        if (!is_one_bank)
            return cartridge->getROM()[address - 0x8000];
        else  // mirrored
            return cartridge->getROM()[(address - 0x8000) & 0x3fff];
    }

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writePRG(NES_Address address, NES_Byte value) override {
        select_chr = value & 0x3;
    }

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) override {
        return cartridge->getVROM()[address | (select_chr << 13)];
    }

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writeCHR(NES_Address address, NES_Byte value) override {
        LOG(Info) << "Read-only CHR memory write attempt at " <<
            std::hex << address << std::endl;
    }
};

}  // namespace NES

#endif // NES_MAPPERS_MAPPER_CNROM_HPP

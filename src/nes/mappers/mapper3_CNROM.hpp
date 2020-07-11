//  Program:      nes-py
//  File:         mapper_CNROM.hpp
//  Description:  An implementation of the CNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_CNROM_HPP
#define NES_MAPPERS_MAPPER_CNROM_HPP

#include "../rom.hpp"
#include "../log.hpp"

namespace NES {

/// The CNROM mapper (mapper #3).
class MapperCNROM : public ROM::Mapper {
 private:
    /// whether there are 1 or 2 banks
    bool is_one_bank;
    /// TODO: what is this value
    NES_Address select_chr;

 public:
    /// Create a new mapper with a rom.
    ///
    /// @param rom_ a reference to a rom for the mapper to access
    ///
    explicit MapperCNROM(ROM& rom_) : Mapper(rom_),
        is_one_bank(rom.getROM().size() == 0x4000),
        select_chr(0) { }

    /// Create a mapper as a copy of another mapper.
    MapperCNROM(const MapperCNROM& other) : ROM::Mapper(*this),
        is_one_bank(other.is_one_bank),
        select_chr(other.select_chr) { }

    /// Destroy this mapper.
    ~MapperCNROM() override { }

    /// Clone the mapper, i.e., the virtual copy constructor
    MapperCNROM* clone() override { return new MapperCNROM(*this); }

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
        select_chr = value & 0x3;
    }

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) override {
        return rom.getVROM()[address | (select_chr << 13)];
    }

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writeCHR(NES_Address address, NES_Byte value) override {
        NES_DEBUG("Read-only CHR memory write attempt at " << std::hex << address);
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "is_one_bank", json_boolean(is_one_bank));
        json_object_set_new(rootJ, "select_chr", json_integer(select_chr));
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) override {
        // load is_one_bank
        {
            json_t* json_data = json_object_get(rootJ, "is_one_bank");
            if (json_data) is_one_bank = json_boolean_value(json_data);
        }
        // load select_chr
        {
            json_t* json_data = json_object_get(rootJ, "select_chr");
            if (json_data) select_chr = json_integer_value(json_data);
        }
    }
};

}  // namespace NES

#endif // NES_MAPPERS_MAPPER_CNROM_HPP

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
            LOG << "Uses character RAM" << std::endl;
        }
    }

    /// Create a mapper as a copy of another mapper.
    MapperNROM(const MapperNROM& other) : ROM::Mapper(*this),
        is_one_bank(other.is_one_bank),
        has_character_ram(other.has_character_ram),
        character_ram(other.character_ram) { }

    /// Destroy this mapper.
    ~MapperNROM() override { }

    /// Clone the mapper, i.e., the virtual copy constructor
    MapperNROM* clone() override { return new MapperNROM(*this); }

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
        LOG << "ROM memory write attempt at " <<
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
            LOG << "Read-only CHR memory write attempt at " <<
                std::hex << address << std::endl;
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "is_one_bank", json_boolean(is_one_bank));
        json_object_set_new(rootJ, "has_character_ram", json_boolean(has_character_ram));
        // encode character_ram
        {
            auto data_string = base64_encode(&character_ram[0], character_ram.size());
            json_object_set_new(rootJ, "character_ram", json_string(data_string.c_str()));
        }
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) override {
        // load is_one_bank
        {
            json_t* json_data = json_object_get(rootJ, "is_one_bank");
            if (json_data)
                is_one_bank = json_boolean_value(json_data);
        }
        // load has_character_ram
        {
            json_t* json_data = json_object_get(rootJ, "has_character_ram");
            if (json_data)
                has_character_ram = json_boolean_value(json_data);
        }
        // load character_ram
        {
            json_t* json_data = json_object_get(rootJ, "character_ram");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                character_ram = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
    }
};

}  // namespace NES

#endif // NES_MAPPERS_MAPPER_NROM_HPP

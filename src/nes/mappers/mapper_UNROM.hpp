//  Program:      nes-py
//  File:         mapper_UNROM.hpp
//  Description:  An implementation of the UNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_UNROM_HPP
#define NES_MAPPERS_MAPPER_UNROM_HPP

#include <string>
#include <vector>
#include "../rom.hpp"
#include "../log.hpp"

namespace NES {

/// The UxROM mapper (mapper #2).
class MapperUNROM : public ROM::Mapper {
 private:
    /// whether the rom use character RAM
    bool has_character_ram;
    /// the pointer to the last bank
    std::size_t last_bank_pointer;
    /// TODO: what is this?
    NES_Address select_prg;
    /// The character RAM on the mapper
    std::vector<NES_Byte> character_ram;

 public:
    /// Create a new mapper with a rom.
    ///
    /// @param cart a reference to a rom for the mapper to access
    ///
    explicit MapperUNROM(ROM& cart): Mapper(cart),
        has_character_ram(rom.getVROM().size() == 0),
        last_bank_pointer(rom.getROM().size() - 0x4000),
        select_prg(0) {
        if (has_character_ram) {
            character_ram.resize(0x2000);
            LOG(Info) << "Uses character RAM" << std::endl;
        }
    }

    /// Create a mapper as a copy of another mapper.
    MapperUNROM(const MapperUNROM& other) : ROM::Mapper(*this),
        has_character_ram(other.has_character_ram),
        last_bank_pointer(other.last_bank_pointer),
        select_prg(other.select_prg),
        character_ram(other.character_ram) { }

    /// Destroy this mapper.
    ~MapperUNROM() override { }

    /// Clone the mapper, i.e., the virtual copy constructor
    MapperUNROM* clone() override { return new MapperUNROM(*this); }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    inline NES_Byte readPRG(NES_Address address) override {
        if (address < 0xc000)
            return rom.getROM()[((address - 0x8000) & 0x3fff) | (select_prg << 14)];
        else
            return rom.getROM()[last_bank_pointer + (address & 0x3fff)];
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

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "has_character_ram", json_boolean(has_character_ram));
        json_object_set_new(rootJ, "last_bank_pointer", json_integer(last_bank_pointer));
        json_object_set_new(rootJ, "select_prg", json_integer(select_prg));
        // encode character_ram
        {
            auto data_string = base64_encode(&character_ram[0], character_ram.size());
            json_object_set_new(rootJ, "character_ram", json_string(data_string.c_str()));
        }
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load has_character_ram
        {
            json_t* json_data = json_object_get(rootJ, "has_character_ram");
            if (json_data)
                has_character_ram = json_boolean_value(json_data);
        }
        // load last_bank_pointer
        {
            json_t* json_data = json_object_get(rootJ, "last_bank_pointer");
            if (json_data)
                last_bank_pointer = json_integer_value(json_data);
        }
        // load select_prg
        {
            json_t* json_data = json_object_get(rootJ, "select_prg");
            if (json_data)
                select_prg = json_integer_value(json_data);
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

#endif  // NES_MAPPERS_MAPPER_UNROM_HPP

//  Program:      nes-py
//  File:         mapper_MMC1.hpp
//  Description:  An implementation of the MMC1 mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_MMC1_HPP
#define NES_MAPPERS_MAPPER_MMC1_HPP

#include <functional>
#include <string>
#include <vector>
#include "../rom.hpp"
#include "../log.hpp"

namespace NES {

/// The MMC1 mapper (mapper #1).
class MapperMMC1 : public ROM::Mapper {
 private:
    /// The mirroring callback on the PPU
    Callback mirroring_callback;
    /// the mirroring mode on the device
    NameTableMirroring mirroring;
    /// whether the rom uses character RAM
    bool has_character_ram;
    /// the mode for CHR ROM
    int mode_chr;
    /// the mode for PRG ROM
    int mode_prg;
    /// a temporary register
    NES_Byte temp_register;
    /// a write counter
    int write_counter;
    /// the PRG register
    NES_Byte register_prg;
    /// The first CHR register
    NES_Byte register_chr0;
    /// The second CHR register
    NES_Byte register_chr1;
    /// The first PRG bank
    std::size_t first_bank_prg;
    /// The second PRG bank
    std::size_t second_bank_prg;
    /// The first CHR bank
    std::size_t first_bank_chr;
    /// The second CHR bank
    std::size_t second_bank_chr;
    /// The character RAM on the rom
    std::vector<NES_Byte> character_ram;

    /// TODO: what does this do
    void calculatePRGPointers() {
        if (mode_prg <= 1) {  // 32KB changeable
            // equivalent to multiplying 0x8000 * (register_prg >> 1)
            first_bank_prg = 0x4000 * (register_prg & ~1);
            // add 16KB
            second_bank_prg = first_bank_prg + 0x4000;
        } else if (mode_prg == 2) {  // fix first switch second
            first_bank_prg = 0;
            second_bank_prg = first_bank_prg + 0x4000 * register_prg;
        } else {  // switch first fix second
            first_bank_prg = 0x4000 * register_prg;
            second_bank_prg = rom.getROM().size() - 0x4000;
        }
    }

 public:
    /// Create a new mapper with a rom.
    ///
    /// @param cart a reference to a rom for the mapper to access
    /// @param mirroring_cb the callback to change mirroring modes on the PPU
    ///
    MapperMMC1(ROM& cart, Callback mirroring_cb) : Mapper(cart),
        mirroring_callback(mirroring_cb),
        mirroring(HORIZONTAL),
        mode_chr(0),
        mode_prg(3),
        temp_register(0),
        write_counter(0),
        register_prg(0),
        register_chr0(0),
        register_chr1(0),
        first_bank_prg(0),
        second_bank_prg(rom.getROM().size() - 0x4000),
        first_bank_chr(0),
        second_bank_chr(0) {
        if (rom.getVROM().size() == 0) {
            has_character_ram = true;
            character_ram.resize(0x2000);
            LOG(Info) << "Uses character RAM" << std::endl;
        } else {
            LOG(Info) << "Using CHR-ROM" << std::endl;
            has_character_ram = false;
            first_bank_chr = 0;
            second_bank_chr = 0x1000 * register_chr1;
        }
    }

    /// Create a mapper as a copy of another mapper.
    MapperMMC1(const MapperMMC1& other) : ROM::Mapper(*this),
        mirroring_callback(other.mirroring_callback),
        mirroring(other.mirroring),
        has_character_ram(other.has_character_ram),
        mode_chr(other.mode_chr),
        mode_prg(other.mode_prg),
        temp_register(other.temp_register),
        write_counter(other.write_counter),
        register_prg(other.register_prg),
        register_chr0(other.register_chr0),
        register_chr1(other.register_chr1),
        first_bank_prg(other.first_bank_prg),
        second_bank_prg(other.second_bank_prg),
        first_bank_chr(other.first_bank_chr),
        second_bank_chr(other.second_bank_chr),
        character_ram(other.character_ram) { }

    /// Destroy this mapper.
    ~MapperMMC1() override { }

    /// Clone the mapper, i.e., the virtual copy constructor
    MapperMMC1* clone() override { return new MapperMMC1(*this); }

    /// Return the name table mirroring mode of this mapper.
    inline NameTableMirroring getNameTableMirroring() const override {
        return mirroring;
    }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    inline NES_Byte readPRG(NES_Address address) override {
        if (address < 0xc000)
            return rom.getROM()[first_bank_prg + (address & 0x3fff)];
        else
            return rom.getROM()[second_bank_prg + (address & 0x3fff)];
    }

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG(NES_Address address, NES_Byte value) override {
        if (!(value & 0x80)) {  // reset bit is NOT set
            temp_register = (temp_register >> 1) | ((value & 1) << 4);
            ++write_counter;

            if (write_counter == 5) {
                if (address <= 0x9fff) {
                    switch (temp_register & 0x3) {
                        case 0: { mirroring = ONE_SCREEN_LOWER;   break; }
                        case 1: { mirroring = ONE_SCREEN_HIGHER;  break; }
                        case 2: { mirroring = VERTICAL;           break; }
                        case 3: { mirroring = HORIZONTAL;         break; }
                    }
                    mirroring_callback();

                    mode_chr = (temp_register & 0x10) >> 4;
                    mode_prg = (temp_register & 0xc) >> 2;
                    calculatePRGPointers();

                    // Recalculate CHR pointers
                    if (mode_chr == 0) {  // one 8KB bank
                        // ignore last bit
                        first_bank_chr = 0x1000 * (register_chr0 | 1);
                        second_bank_chr = first_bank_chr + 0x1000;
                    } else {  // two 4KB banks
                        first_bank_chr = 0x1000 * register_chr0;
                        second_bank_chr = 0x1000 * register_chr1;
                    }
                } else if (address <= 0xbfff) {  // CHR Reg 0
                    register_chr0 = temp_register;
                    // OR 1 if 8KB mode
                    first_bank_chr = 0x1000 * (temp_register | (1 - mode_chr));
                    if (mode_chr == 0)
                        second_bank_chr = first_bank_chr + 0x1000;
                } else if (address <= 0xdfff) {
                    register_chr1 = temp_register;
                    if(mode_chr == 1)
                        second_bank_chr = 0x1000 * temp_register;
                } else {
                    // TODO: PRG-RAM
                    if ((temp_register & 0x10) == 0x10) {
                        LOG(Info) << "PRG-RAM activated" << std::endl;
                    }
                    temp_register &= 0xf;
                    register_prg = temp_register;
                    calculatePRGPointers();
                }

                temp_register = 0;
                write_counter = 0;
            }
        } else {  // reset
            temp_register = 0;
            write_counter = 0;
            mode_prg = 3;
            calculatePRGPointers();
        }
    }

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) override {
        if (has_character_ram)
            return character_ram[address];
        else if (address < 0x1000)
            return rom.getVROM()[first_bank_chr + address];
        else
            return rom.getVROM()[second_bank_chr + (address & 0xfff)];
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
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "mirroring", json_integer(mirroring));
        json_object_set_new(rootJ, "has_character_ram", json_boolean(has_character_ram));
        json_object_set_new(rootJ, "mode_chr", json_integer(mode_chr));
        json_object_set_new(rootJ, "mode_prg", json_integer(mode_prg));
        json_object_set_new(rootJ, "temp_register", json_integer(temp_register));
        json_object_set_new(rootJ, "write_counter", json_integer(write_counter));
        json_object_set_new(rootJ, "register_prg", json_integer(register_prg));
        json_object_set_new(rootJ, "register_chr0", json_integer(register_chr0));
        json_object_set_new(rootJ, "register_chr1", json_integer(register_chr1));
        json_object_set_new(rootJ, "first_bank_prg", json_integer(first_bank_prg));
        json_object_set_new(rootJ, "second_bank_prg", json_integer(second_bank_prg));
        json_object_set_new(rootJ, "first_bank_chr", json_integer(first_bank_chr));
        json_object_set_new(rootJ, "second_bank_chr", json_integer(second_bank_chr));
        {
            auto data_string = base64_encode(&character_ram[0], character_ram.size());
            json_object_set_new(rootJ, "character_ram", json_string(data_string.c_str()));
        }
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) override {
        // load mirroring
        {
            json_t* json_data = json_object_get(rootJ, "mirroring");
            if (json_data) mirroring = static_cast<NameTableMirroring>(json_integer_value(json_data));
        }
        // load has_character_ram
        {
            json_t* json_data = json_object_get(rootJ, "has_character_ram");
            if (json_data) has_character_ram = json_boolean_value(json_data);
        }
        // load mode_chr
        {
            json_t* json_data = json_object_get(rootJ, "mode_chr");
            if (json_data) mode_chr = json_integer_value(json_data);
        }
        // load mode_prg
        {
            json_t* json_data = json_object_get(rootJ, "mode_prg");
            if (json_data) mode_prg = json_integer_value(json_data);
        }
        // load temp_register
        {
            json_t* json_data = json_object_get(rootJ, "temp_register");
            if (json_data) temp_register = json_integer_value(json_data);
        }
        // load write_counter
        {
            json_t* json_data = json_object_get(rootJ, "write_counter");
            if (json_data) write_counter = json_integer_value(json_data);
        }
        // load register_prg
        {
            json_t* json_data = json_object_get(rootJ, "register_prg");
            if (json_data) register_prg = json_integer_value(json_data);
        }
        // load register_chr0
        {
            json_t* json_data = json_object_get(rootJ, "register_chr0");
            if (json_data) register_chr0 = json_integer_value(json_data);
        }
        // load register_chr1
        {
            json_t* json_data = json_object_get(rootJ, "register_chr1");
            if (json_data) register_chr1 = json_integer_value(json_data);
        }
        // load first_bank_prg
        {
            json_t* json_data = json_object_get(rootJ, "first_bank_prg");
            if (json_data) first_bank_prg = json_integer_value(json_data);
        }
        // load second_bank_prg
        {
            json_t* json_data = json_object_get(rootJ, "second_bank_prg");
            if (json_data) second_bank_prg = json_integer_value(json_data);
        }
        // load first_bank_chr
        {
            json_t* json_data = json_object_get(rootJ, "first_bank_chr");
            if (json_data) first_bank_chr = json_integer_value(json_data);
        }
        // load second_bank_chr
        {
            json_t* json_data = json_object_get(rootJ, "second_bank_chr");
            if (json_data) second_bank_chr = json_integer_value(json_data);
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

#endif  // NES_MAPPERS_MAPPER_MMC1_HPP

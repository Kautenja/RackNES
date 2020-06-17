//  Program:      nes-py
//  File:         cartridge.hpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_CARTRIDGE_HPP
#define NES_CARTRIDGE_HPP

#include <jansson.h>
#include <fstream>
#include <string>
#include <vector>
#include <jansson.h>
#include "../base64.h"
#include "common.hpp"

namespace NES {

/// Mirroring modes supported by the NES
enum NameTableMirroring {
    HORIZONTAL  = 0,
    VERTICAL    = 1,
    FOUR_SCREEN  = 8,
    ONE_SCREEN_LOWER,
    ONE_SCREEN_HIGHER,
};

/// A cartridge holding game ROM and a special hardware mapper emulation
class ROM {
 protected:
    /// the path to the ROM file on disk
    std::string rom_path;
    /// the PRG ROM
    std::vector<NES_Byte> prg_rom;
    /// the CHR ROM
    std::vector<NES_Byte> chr_rom;
    /// the name table mirroring mode
    NES_Byte name_table_mirroring;
    /// the mapper ID number
    NES_Byte mapper_number;
    /// whether this cartridge uses extended RAM
    bool has_extended_ram;

 public:
    /// Return true of the path points to a valid iNES file.
    ///
    /// @param path the path to the potential file to check
    /// @returns true if the path points to a valid iNES file, false otherwise
    ///
    static inline bool is_valid_rom(const std::string& path) {
        // the "magic number", i.e., sentinel value, of the header
        static const std::vector<NES_Byte> MAGIC = {0x4E, 0x45, 0x53, 0x1A};
        // // create a stream to load the ROM file
        std::ifstream romFile(path, std::ios_base::binary | std::ios_base::in);
        /// return false if the file could not be opened
        if (!romFile.is_open()) return false;
        // create a byte vector for the iNES header
        std::vector<NES_Byte> header;
        // resize the header vector
        header.resize(MAGIC.size());
        // read the header data from the file
        romFile.read(reinterpret_cast<char*>(&header[0]), MAGIC.size());
        // check if the header is equal to the magic number
        return header == MAGIC;
    }

    /// Initialize a new ROM file.
    explicit ROM(const std::string& path) : rom_path(path) {
        // create a stream to load the ROM file
        std::ifstream romFile(path, std::ios_base::binary | std::ios_base::in);
        // create a byte vector for the iNES header
        std::vector<NES_Byte> header;
        header.resize(0x10);
        romFile.read(reinterpret_cast<char*>(&header[0]), 0x10);
        // read internal data
        name_table_mirroring = header[6] & 0xB;
        mapper_number = ((header[6] >> 4) & 0xf) | (header[7] & 0xf0);
        has_extended_ram = header[6] & 0x2;
        // read PRG-ROM 16KB banks
        NES_Byte banks = header[4];
        prg_rom.resize(0x4000 * banks);
        romFile.read(reinterpret_cast<char*>(&prg_rom[0]), 0x4000 * banks);
        // read CHR-ROM 8KB banks
        NES_Byte vbanks = header[5];
        if (!vbanks) return;
        chr_rom.resize(0x2000 * vbanks);
        romFile.read(reinterpret_cast<char*>(&chr_rom[0]), 0x2000 * vbanks);
    }

    /// Return the path to the ROM on disk.
    inline std::string get_rom_path() const { return rom_path; }

    /// Return the ROM data.
    const inline std::vector<NES_Byte>& getROM() const { return prg_rom; }

    /// Return the VROM data.
    const inline std::vector<NES_Byte>& getVROM() const { return chr_rom; }

    /// Return the name table mirroring mode.
    inline NameTableMirroring getNameTableMirroring() const {
        return static_cast<NameTableMirroring>(name_table_mirroring);
    }

    /// Return the mapper ID number.
    inline NES_Byte get_mapper_number() const { return mapper_number; }

    /// Return a boolean determining whether this cartridge uses extended RAM.
    inline bool hasExtendedRAM() const { return has_extended_ram; }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "rom_path", json_string(rom_path.c_str()));
        // encode prg_rom
        {
            auto data_string = base64_encode(&prg_rom[0], prg_rom.size());
            json_object_set_new(rootJ, "prg_rom", json_string(data_string.c_str()));
        }
        // encode chr_rom
        {
            auto data_string = base64_encode(&chr_rom[0], chr_rom.size());
            json_object_set_new(rootJ, "chr_rom", json_string(data_string.c_str()));
        }
        json_object_set_new(rootJ, "name_table_mirroring", json_integer(name_table_mirroring));
        json_object_set_new(rootJ, "mapper_number", json_integer(mapper_number));
        json_object_set_new(rootJ, "has_extended_ram", json_boolean(has_extended_ram));
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load rom_path
        {
            json_t* json_data = json_object_get(rootJ, "rom_path");
            if (json_data)
                rom_path = json_string_value(json_data);
        }
        // load prg_rom
        {
            json_t* json_data = json_object_get(rootJ, "prg_rom");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                prg_rom = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
        // load chr_rom
        {
            json_t* json_data = json_object_get(rootJ, "chr_rom");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                chr_rom = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
        // load name_table_mirroring
        {
            json_t* json_data = json_object_get(rootJ, "name_table_mirroring");
            if (json_data)
                name_table_mirroring = json_integer_value(json_data);
        }
        // load mapper_number
        {
            json_t* json_data = json_object_get(rootJ, "mapper_number");
            if (json_data)
                mapper_number = json_integer_value(json_data);
        }
        // load has_extended_ram
        {
            json_t* json_data = json_object_get(rootJ, "has_extended_ram");
            if (json_data)
                has_extended_ram = json_boolean_value(json_data);
        }
    }

    /// An ASIC mapper for different NES cartridges.
    class Mapper {
     protected:
        /// The cartridge this mapper associates with
        ROM& rom;

        /// Create a mapper as a copy of another mapper.
        Mapper(const Mapper& other) : rom(other.rom) { }

     public:
        /// Create a new mapper with a rom and given type.
        ///
        /// @param rom_ a reference to a rom for the mapper to access
        ///
        explicit Mapper(ROM& rom_) : rom(rom_) { }

        /// Destroy this mapper.
        virtual ~Mapper() { }

        /// Clone the mapper, i.e., the virtual copy constructor
        virtual Mapper* clone() = 0;

        /// Return true if this mapper has extended RAM, false otherwise.
        inline bool hasExtendedRAM() const { return rom.hasExtendedRAM(); }

        /// Return the name table mirroring mode of this mapper.
        inline virtual NameTableMirroring getNameTableMirroring() {
            return rom.getNameTableMirroring();
        }

        /// Read a byte from the PRG RAM.
        ///
        /// @param address the 16-bit address of the byte to read
        /// @return the byte located at the given address in PRG RAM
        ///
        virtual NES_Byte readPRG(NES_Address address) = 0;

        /// Write a byte to an address in the PRG RAM.
        ///
        /// @param address the 16-bit address to write to
        /// @param value the byte to write to the given address
        ///
        virtual void writePRG(NES_Address address, NES_Byte value) = 0;

        /// Read a byte from the CHR RAM.
        ///
        /// @param address the 16-bit address of the byte to read
        /// @return the byte located at the given address in CHR RAM
        ///
        virtual NES_Byte readCHR(NES_Address address) = 0;

        /// Write a byte to an address in the CHR RAM.
        ///
        /// @param address the 16-bit address to write to
        /// @param value the byte to write to the given address
        ///
        virtual void writeCHR(NES_Address address, NES_Byte value) = 0;

        /// Convert the object's state to a JSON object.
        virtual json_t* dataToJson() = 0;

        /// Load the object's state from a JSON object.
        virtual void dataFromJson(json_t* rootJ) = 0;
    };
};

}  // namespace NES

#endif // NES_CARTRIDGE_HPP

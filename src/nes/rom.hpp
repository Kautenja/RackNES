//  Program:      nes-py
//  File:         cartridge.hpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_CARTRIDGE_HPP
#define NES_CARTRIDGE_HPP

#include <fstream>
#include <string>
#include <vector>
#include <jansson.h>
#include "../base64.h"
#include "common.hpp"
#include "log.hpp"

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

 public:
    /// @brief Return true of the path points to a valid iNES file.
    ///
    /// @param path the path to the potential file to check
    /// @returns true if the path points to a valid iNES file, false otherwise
    ///
    static inline bool is_valid_rom(const std::string& path) {
        // the "magic number", i.e., sentinel value, of the header
        static const std::vector<NES_Byte> MAGIC = {0x4E, 0x45, 0x53, 0x1A};
        // // create a stream to load the ROM file
        std::ifstream romFile(path, std::ios_base::binary | std::ios_base::in);
        /// return false if the file did not open
        if (!romFile.is_open()) return false;
        // create a byte vector for the iNES header
        std::vector<NES_Byte> header(MAGIC.size());
        // read the header data from the file
        romFile.read(reinterpret_cast<char*>(&header[0]), MAGIC.size());
        // check if the header is equal to the magic number
        return header == MAGIC;
    }

    /// the size of the iNES head in bytes
    static constexpr int HEADER_SIZE = 16;

    /// the indexes of semantic bytes in the iNES header.
    enum HEADER_BYTES {
        MAGIC_NIBBLE = 0,  // the magic nibble "NES<EOF>"
        PRG_ROM_SIZE = 4,
        CHR_ROM_SIZE,
        FLAGS6,
        FLAGS7,
        FLAGS8,
        FLAGS9,
        FLAGS10,
    };

    /// a structure for working with the flags 6 byte in the iNES header.
    union Flags6 {
        struct {
            /// the hard-wired name-table mirroring type (true = vertical)
            bool is_vertical_mirroring: 1;
            /// whether "battery" and other non-volatile memory is present
            bool has_persistent_memory: 1;
            /// whether the 512-byte trainer is present
            bool has_trainer: 1;
            /// whether hard-wired four-screen mode is present
            bool is_four_screen_mode: 1;
            /// the low bits of the mapper number
            uint8_t mapper_low: 4;
        } flags;
        /// the name-table mirroring mode
        uint8_t name_table_mirroring: 4;
        /// the byte representation of the flag register
        uint8_t byte;
    } flags6;

    /// the different console types.
    enum ConsoleType {
        NES_FAMICOM = 0,
        VS_SYSTEM,
        PLAYCHOICE10,
        EXTENDED
    };

    /// a structure for working with the flags 7 byte in the iNES header.
    union Flags7 {
        struct {
            /// the console type ID (followed by an unused bit)
            uint8_t console_type: 2, : 1;
            /// whether the ROM file is NES2.0 format (true) or iNES (false)
            bool is_nes_2: 1;
            /// the middle bits of the mapper number
            uint8_t mapper_mid: 4;
        } flags;
        /// the byte representation of the flag register
        uint8_t byte;
    } flags7;

    /// @brief Initialize a new ROM file.
    ///
    /// @param path the path to the iNES or NES2.0 file on disk
    ///
    explicit ROM(const std::string& path) : rom_path(path) {
        // create a stream to load the ROM file
        std::ifstream romFile(path, std::ios_base::binary | std::ios_base::in);
        // create a byte vector for the iNES header
        std::vector<NES_Byte> header(HEADER_SIZE);
        romFile.read(reinterpret_cast<char*>(&header[0]), HEADER_SIZE);
        // read the flag registers
        flags6 = reinterpret_cast<Flags6&>(header[FLAGS6]);
        flags7 = reinterpret_cast<Flags7&>(header[FLAGS7]);
        // read PRG-ROM 16KB banks
        static constexpr uint64_t PRG_BANK_SIZE = 0x4000;
        NES_Byte banks = header[PRG_ROM_SIZE];
        prg_rom.resize(PRG_BANK_SIZE * banks);
        romFile.read(reinterpret_cast<char*>(&prg_rom[0]), PRG_BANK_SIZE * banks);
        // read CHR-ROM 8KB banks
        static constexpr uint64_t CHR_BANK_SIZE = 0x4000;
        NES_Byte vbanks = header[CHR_ROM_SIZE];
        if (!vbanks) return;
        chr_rom.resize(CHR_BANK_SIZE * vbanks);
        romFile.read(reinterpret_cast<char*>(&chr_rom[0]), CHR_BANK_SIZE * vbanks);
    }

    /// @brief Return the path to the ROM on disk.
    ///
    /// @returns the string path to the ROM data on disk
    ///
    inline std::string get_rom_path() const { return rom_path; }

    /// @brief Return the ROM data.
    ///
    /// @return the ROM data of the ROM
    ///
    const inline std::vector<NES_Byte>& getROM() const { return prg_rom; }

    /// @brief Return the VROM data.
    ///
    /// @return the CHR/VROM data of the ROM
    ///
    const inline std::vector<NES_Byte>& getVROM() const { return chr_rom; }

    /// @brief Return the name table mirroring mode.
    ///
    /// @returns the name table mirroring mode used by the ROM
    ///
    inline NameTableMirroring getNameTableMirroring() const {
        return static_cast<NameTableMirroring>(flags6.name_table_mirroring);
    }

    /// @brief Return the mapper ID number.
    ///
    /// @returns the iNES mapper ID for the cartridge mapper
    ///
    inline uint16_t get_mapper_number() const {
        return (flags7.flags.mapper_mid << 4) | flags6.flags.mapper_low;
    }

    /// @brief Return a boolean determining whether this cartridge uses
    /// extended RAM.
    ///
    /// @returns true if the ROM requires extended RAM, false otherwise
    ///
    inline bool hasExtendedRAM() const {
        return flags6.flags.has_persistent_memory;
    }

    /// @brief Convert the object's state to a JSON object.
    ///
    /// @returns a JSON representation of this instance's data
    ///
    json_t* dataToJson() const {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "rom_path", json_string(rom_path.c_str()));
        // // encode prg_rom
        // {
        //     auto data_string = base64_encode(&prg_rom[0], prg_rom.size());
        //     json_object_set_new(rootJ, "prg_rom", json_string(data_string.c_str()));
        // }
        // // encode chr_rom
        // {
        //     auto data_string = base64_encode(&chr_rom[0], chr_rom.size());
        //     json_object_set_new(rootJ, "chr_rom", json_string(data_string.c_str()));
        // }
        json_object_set_new(rootJ, "flags6", json_integer(flags6.byte));
        json_object_set_new(rootJ, "flags7", json_integer(flags7.byte));
        return rootJ;
    }

    /// @brief Load the object's state from a JSON object.
    ///
    /// @param rootJ the serialized JSON data to load into this object
    ///
    void dataFromJson(json_t* rootJ) {
        // load rom_path
        {
            json_t* json_data = json_object_get(rootJ, "rom_path");
            if (json_data)
                rom_path = json_string_value(json_data);
        }
        // // load prg_rom
        // {
        //     json_t* json_data = json_object_get(rootJ, "prg_rom");
        //     if (json_data) {
        //         std::string data_string = json_string_value(json_data);
        //         data_string = base64_decode(data_string);
        //         prg_rom = std::vector<NES_Byte>(data_string.begin(), data_string.end());
        //     }
        // }
        // // load chr_rom
        // {
        //     json_t* json_data = json_object_get(rootJ, "chr_rom");
        //     if (json_data) {
        //         std::string data_string = json_string_value(json_data);
        //         data_string = base64_decode(data_string);
        //         chr_rom = std::vector<NES_Byte>(data_string.begin(), data_string.end());
        //     }
        // }
        // load flags6
        {
            json_t* json_data = json_object_get(rootJ, "flags6");
            if (json_data) flags6.byte = json_integer_value(json_data);
        }
        // load flags7
        {
            json_t* json_data = json_object_get(rootJ, "flags7");
            if (json_data) flags7.byte = json_integer_value(json_data);
        }
    }

    /// An iNES mapper for different NES cartridges.
    class Mapper {
     protected:
        /// The ROM file this mapper interacts with
        ROM& rom;

        /// Create a mapper as a copy of another mapper (disabled).
        Mapper(const Mapper& other) : rom(other.rom) { }

     public:
        /// @brief Create a new mapper with a rom and given type.
        ///
        /// @param rom_ a reference to a rom for the mapper to access
        ///
        explicit Mapper(ROM& rom_) : rom(rom_) { }

        /// @brief  Destroy this mapper.
        virtual ~Mapper() { }

        /// @brief Clone the mapper, i.e., the virtual copy constructor
        virtual Mapper* clone() = 0;

        /// @brief Return a boolean determining whether this cartridge uses
        /// extended RAM.
        ///
        /// @returns true if the ROM requires extended RAM, false otherwise
        ///
        inline bool hasExtendedRAM() const { return rom.hasExtendedRAM(); }

        /// @brief Return the name table mirroring mode.
        ///
        /// @returns the name table mirroring mode used by the ROM
        ///
        inline virtual NameTableMirroring getNameTableMirroring() const {
            return rom.getNameTableMirroring();
        }

        /// Read a byte from the PRG RAM.
        ///
        /// @param address the 16-bit address of the byte to read
        /// @returns the byte located at the given address in PRG RAM
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
        /// @returns the byte located at the given address in CHR RAM
        ///
        virtual NES_Byte readCHR(NES_Address address) = 0;

        /// Write a byte to an address in the CHR RAM.
        ///
        /// @param address the 16-bit address to write to
        /// @param value the byte to write to the given address
        ///
        virtual void writeCHR(NES_Address address, NES_Byte value) = 0;

        /// @brief Convert the object's state to a JSON object.
        ///
        /// @returns a JSON representation of this instance's data
        ///
        virtual json_t* dataToJson() = 0;

        /// @brief Load the object's state from a JSON object.
        ///
        /// @param rootJ the serialized JSON data to load into this object
        ///
        virtual void dataFromJson(json_t* rootJ) = 0;
    };
};

}  // namespace NES

#endif // NES_CARTRIDGE_HPP

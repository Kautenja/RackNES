//  Program:      nes-py
//  File:         cartridge.hpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <fstream>
#include <vector>
#include <string>
#include "common.hpp"

namespace NES {

/// A cartridge holding game ROM and a special hardware mapper emulation
class Cartridge {
 private:
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

    /// Initialize a new cartridge
    Cartridge() :
        name_table_mirroring(0),
        mapper_number(0),
        has_extended_ram(false) { }

    /// Return the ROM data.
    const inline std::vector<NES_Byte>& getROM() { return prg_rom; }

    /// Return the VROM data.
    const inline std::vector<NES_Byte>& getVROM() { return chr_rom; }

    /// Return the mapper ID number.
    inline NES_Byte getMapper() { return mapper_number; }

    /// Return the name table mirroring mode.
    inline NES_Byte getNameTableMirroring() { return name_table_mirroring; }

    /// Return a boolean determining whether this cartridge uses extended RAM.
    inline bool hasExtendedRAM() { return has_extended_ram; }

    /// Load a ROM file into the cartridge and build the corresponding mapper.
    void loadFromFile(std::string path);
};

}  // namespace NES

#endif // CARTRIDGE_HPP

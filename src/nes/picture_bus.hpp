//  Program:      nes-py
//  File:         picture_bus.hpp
//  Description:  This class houses picture bus data from the PPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_PICTURE_BUS_HPP
#define NES_PICTURE_BUS_HPP

#include <vector>
#include <cstdlib>
#include "common.hpp"
#include "mapper.hpp"
#include "log.hpp"

namespace NES {

/// The bus for graphical data to travel along
class PictureBus {
 private:
    /// the VRAM on the picture bus
    std::vector<NES_Byte> ram;
    /// indexes where they start in RAM vector
    std::size_t name_tables[4] = {0, 0, 0, 0};
    /// the palette for decoding RGB tuples
    std::vector<NES_Byte> palette;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;

 public:
    /// Initialize a new picture bus.
    PictureBus() : ram(0x800), palette(0x20), mapper(nullptr) { }

    /// Read a byte from an address on the VRAM.
    ///
    /// @param address the 16-bit address of the byte to read in the VRAM
    ///
    /// @return the byte located at the given address
    ///
    NES_Byte read(NES_Address address) {
        if (address < 0x2000) {
            return mapper->readCHR(address);
        } else if (address < 0x3eff) {  // Name tables up to 0x3000, then mirrored up to 0x3ff
            if (address < 0x2400)  // NT0
                return ram[name_tables[0] + (address & 0x3ff)];
            else if (address < 0x2800)  // NT1
                return ram[name_tables[1] + (address & 0x3ff)];
            else if (address < 0x2c00)  // NT2
                return ram[name_tables[2] + (address & 0x3ff)];
            else  // NT3
                return ram[name_tables[3] + (address & 0x3ff)];
        } else if (address < 0x3fff) {
            return palette[address & 0x1f];
        }
        return 0;
    }

    /// Write a byte to an address in the VRAM.
    ///
    /// @param address the 16-bit address to write the byte to in VRAM
    /// @param value the byte to write to the given address
    ///
    void write(NES_Address address, NES_Byte value) {
        if (address < 0x2000) {
            mapper->writeCHR(address, value);
        } else if (address < 0x3eff) {  // Name tables up to 0x3000, then mirrored up to 0x3ff
            if (address < 0x2400)  // NT0
                ram[name_tables[0] + (address & 0x3ff)] = value;
            else if (address < 0x2800)  // NT1
                ram[name_tables[1] + (address & 0x3ff)] = value;
            else if (address < 0x2c00)  // NT2
                ram[name_tables[2] + (address & 0x3ff)] = value;
            else  // NT3
                ram[name_tables[3] + (address & 0x3ff)] = value;
        } else if (address < 0x3fff) {
            if (address == 0x3f10)
                palette[0] = value;
            else
                palette[address & 0x1f] = value;
        }
    }

    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    inline void set_mapper(Mapper *mapper) {
        this->mapper = mapper; update_mirroring();
    }

    /// Read a color index from the palette.
    ///
    /// @param address the address of the palette color
    ///
    /// @return the index of the RGB tuple in the color array
    ///
    inline NES_Byte read_palette(NES_Byte address) const {
        return palette[address];
    }

    /// Update the mirroring and name table from the mapper.
    void update_mirroring() {
        switch (mapper->getNameTableMirroring()) {
            case HORIZONTAL:
                name_tables[0] = name_tables[1] = 0;
                name_tables[2] = name_tables[3] = 0x400;
                LOG(InfoVerbose) <<
                    "Horizontal Name Table mirroring set. (Vertical Scrolling)" <<
                    std::endl;
                break;
            case VERTICAL:
                name_tables[0] = name_tables[2] = 0;
                name_tables[1] = name_tables[3] = 0x400;
                LOG(InfoVerbose) <<
                    "Vertical Name Table mirroring set. (Horizontal Scrolling)" <<
                    std::endl;
                break;
            case ONE_SCREEN_LOWER:
                name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0;
                LOG(InfoVerbose) <<
                    "Single Screen mirroring set with lower bank." <<
                    std::endl;
                break;
            case ONE_SCREEN_HIGHER:
                name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0x400;
                LOG(InfoVerbose) <<
                    "Single Screen mirroring set with higher bank." <<
                    std::endl;
                break;
            default:
                name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0;
                LOG(Error) <<
                    "Unsupported Name Table mirroring : " <<
                    mapper->getNameTableMirroring() <<
                    std::endl;
        }
    }
};

}  // namespace NES

#endif  // NES_PICTURE_BUS_HPP

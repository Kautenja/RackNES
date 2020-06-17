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
#include "cartridge.hpp"
#include "log.hpp"

namespace NES {

/// The bus for graphical data to travel along
class PictureBus {
 private:
    /// the VRAM on the picture bus
    std::vector<NES_Byte> ram = std::vector<NES_Byte>(0x800);
    /// indexes where they start in RAM vector
    std::vector<std::size_t> name_tables = {0, 0, 0, 0};
    /// the palette for decoding RGB tuples
    std::vector<NES_Byte> palette = std::vector<NES_Byte>(0x20);
    /// a pointer to the mapper on the cartridge
    ROM::Mapper* mapper = nullptr;

 public:
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
    inline void set_mapper(ROM::Mapper *mapper_) {
        mapper = mapper_;
        update_mirroring();
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
                LOG(InfoVerbose) << "Horizontal Name Table mirroring set. (Vertical Scrolling)" << std::endl;
                break;
            case VERTICAL:
                name_tables[0] = name_tables[2] = 0;
                name_tables[1] = name_tables[3] = 0x400;
                LOG(InfoVerbose) << "Vertical Name Table mirroring set. (Horizontal Scrolling)" << std::endl;
                break;
            case ONE_SCREEN_LOWER:
                name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0;
                LOG(InfoVerbose) << "Single Screen mirroring set with lower bank." << std::endl;
                break;
            case ONE_SCREEN_HIGHER:
                name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0x400;
                LOG(InfoVerbose) << "Single Screen mirroring set with higher bank." << std::endl;
                break;
            default:
                name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0;
                LOG(Error) << "Unsupported Name Table mirroring : " << mapper->getNameTableMirroring() << std::endl;
        }
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        // encode ram
        {
            auto data_string = base64_encode(&ram[0], ram.size());
            json_object_set_new(rootJ, "ram", json_string(data_string.c_str()));
        }
        // encode name_tables (TODO: fix for endian-ness cast std::size_t to char is not safe)
        {
            auto data_string = base64_encode((char*) &name_tables[0], sizeof(name_tables));
            json_object_set_new(rootJ, "name_tables", json_string(data_string.c_str()));
        }
        // encode palette
        {
            auto data_string = base64_encode(&palette[0], palette.size());
            json_object_set_new(rootJ, "palette", json_string(data_string.c_str()));
        }
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load ram
        {
            json_t* json_data = json_object_get(rootJ, "ram");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                ram = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
        // load name_tables
        {
            json_t* json_data = json_object_get(rootJ, "name_tables");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                name_tables = std::vector<std::size_t>(data_string.begin(), data_string.end());
            }
        }
        // load palette
        {
            json_t* json_data = json_object_get(rootJ, "palette");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                palette = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
    }
};

}  // namespace NES

#endif  // NES_PICTURE_BUS_HPP

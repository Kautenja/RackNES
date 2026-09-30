//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  An abstract factory for mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPER_FACTORY_HPP
#define NES_MAPPER_FACTORY_HPP

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <jansson.h>
#include "rom.hpp"
#include "mappers/mapper0_NROM.hpp"
#include "mappers/mapper1_MMC1.hpp"
#include "mappers/mapper2_UNROM.hpp"
#include "mappers/mapper3_CNROM.hpp"
#include "mappers/mapper7_AxROM.hpp"
#include "mappers/mapper9_MMC2.hpp"

namespace NES {

/// An NES cartridge including ROM and mapper.
class Cartridge : public ROM {
 protected:
    /// the mapper for the cartridge
    Mapper* mapper = nullptr;

    /// Create a new Cartridge.
    ///
    /// @param path the path to the ROM for the callback
    ///
    explicit Cartridge(const std::string& path) : ROM(path) { }

 public:
    /// an enumeration of supported mapper IDs
    enum class MapperID : uint16_t {
        NROM   = 0,
        MMC1   = 1,
        UNROM  = 2,
        CNROM  = 3,
        AXROM  = 7,
        MMC2   = 9,
    };

    /// Create a new Cartridge.
    ///
    /// @param path the path to the ROM for the callback
    /// @param callback a callback to update name-table mirroring on the PPU
    /// @param cartridge_state optional saved state to validate before loading
    ///
    static inline Cartridge* create(const std::string& path, Callback callback,
                                    json_t* cartridge_state = nullptr) {
        // Inspect new mapper layouts before the legacy loader allocates/reads.
        std::ifstream file(path, std::ios::binary);
        std::array<NES_Byte, 16> header = {};
        if (!file.read(reinterpret_cast<char*>(header.data()), header.size()))
            return nullptr;
        const bool axrom = (header[6] >> 4) == 7 && (header[7] & 0xF0) == 0 &&
            ((header[7] & 0x0C) != 0x08 || (header[8] & 0x0F) == 0);
        if (axrom) {
            file.seekg(0, std::ios::end);
            if (!MapperAxROM::supports(header, file.tellg()) ||
                (cartridge_state && !MapperAxROM::is_valid_state(
                    json_object_get(cartridge_state, "mapper"))))
                return nullptr;
        }
        const bool mmc2 = (header[6] >> 4) == 9 && (header[7] & 0xF0) == 0 &&
            ((header[7] & 0x0C) != 0x08 || (header[8] & 0x0F) == 0);
        if (mmc2) {
            file.seekg(0, std::ios::end);
            if (!MapperMMC2::supports(header, file.tellg()) ||
                (cartridge_state && !MapperMMC2::is_valid_state(
                    json_object_get(cartridge_state, "mapper"))))
                return nullptr;
        }
        // initialize a new cartridge
        auto cartridge = new Cartridge(path);
        // load the mapper
        NES_DEBUG("loading mapper with ID " << static_cast<int>(cartridge->get_mapper_number()));
        switch (static_cast<MapperID>(cartridge->get_mapper_number())) {
            case MapperID::NROM:  cartridge->mapper = new MapperNROM(*cartridge);           break;
            case MapperID::MMC1:  cartridge->mapper = new MapperMMC1(*cartridge, callback); break;
            case MapperID::UNROM: cartridge->mapper = new MapperUNROM(*cartridge);          break;
            case MapperID::CNROM: cartridge->mapper = new MapperCNROM(*cartridge);          break;
            case MapperID::AXROM:
                cartridge->mapper = new MapperAxROM(*cartridge, callback,
                    header[7] == 0x08 && (header[8] >> 4) == 2);
                break;
            case MapperID::MMC2:
                cartridge->mapper = new MapperMMC2(*cartridge, callback);
                break;
            default: delete cartridge; cartridge = nullptr;
        }
        // return the cartridge
        return cartridge;
    }

    /// Copy this cartridge.
    Cartridge(const Cartridge& other, Callback callback = Callback()) : ROM(other) {
        if (other.mapper != nullptr) {
            if (get_mapper_number() == 7)
                mapper = new MapperAxROM(*this,
                    *static_cast<const MapperAxROM*>(other.mapper), callback);
            else if (get_mapper_number() == 9)
                mapper = new MapperMMC2(*this,
                    *static_cast<const MapperMMC2*>(other.mapper), callback);
            else
                mapper = other.mapper->clone();
            std::copy(other.mapper->get_extended_ram(),
                other.mapper->get_extended_ram() + other.mapper->extended_ram_size(),
                mapper->get_extended_ram());
        }
    }

    /// Destroy this cartridge.
    ~Cartridge() { if (mapper != nullptr) delete mapper; }

    /// Clone the cartridge, i.e., the virtual copy constructor.
    Cartridge* clone(Callback callback = Callback()) { return new Cartridge(*this, callback); }

    /// Return a pointer to the mapper for the cartridge.
    inline Mapper* get_mapper() { return mapper; }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() const {
        json_t* rootJ = ROM::dataToJson();
        json_object_set_new(rootJ, "mapper", mapper->dataToJson());
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        ROM::dataFromJson(rootJ);
        json_t* json_data = json_object_get(rootJ, "mapper");
        if (json_data) mapper->dataFromJson(json_data);
    }
};

}  // namespace NES

#endif  // NES_MAPPER_FACTORY_HPP

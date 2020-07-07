//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  An abstract factory for mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPER_FACTORY_HPP
#define NES_MAPPER_FACTORY_HPP

#include <string>
#include <jansson.h>
#include "rom.hpp"
#include "mappers/mapper0_NROM.hpp"
#include "mappers/mapper1_MMC1.hpp"
#include "mappers/mapper2_UNROM.hpp"
#include "mappers/mapper3_CNROM.hpp"

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
    enum class MapperID : NES_Byte {
        NROM   = 0,
        MMC1   = 1,
        UNROM  = 2,
        CNROM  = 3,
    };

    /// Create a new Cartridge.
    ///
    /// @param path the path to the ROM for the callback
    /// @param callback a callback to update name-table mirroring on the PPU
    ///
    static inline Cartridge* create(const std::string& path, Callback callback) {
        // initialize a new cartridge
        auto cartridge = new Cartridge(path);
        // load the mapper
        LOG(INFO) << "loading mapper with ID " << static_cast<int>(cartridge->get_mapper_number()) << std::endl;
        switch (static_cast<MapperID>(cartridge->get_mapper_number())) {
            case MapperID::NROM:  cartridge->mapper = new MapperNROM(*cartridge);           break;
            case MapperID::MMC1:  cartridge->mapper = new MapperMMC1(*cartridge, callback); break;
            case MapperID::UNROM: cartridge->mapper = new MapperUNROM(*cartridge);          break;
            case MapperID::CNROM: cartridge->mapper = new MapperCNROM(*cartridge);          break;
            default: delete cartridge; cartridge = nullptr;
        }
        // return the cartridge
        return cartridge;
    }

    /// Copy this cartridge.
    Cartridge(const Cartridge& other) : ROM(other) {
        if (other.mapper != nullptr) mapper = other.mapper->clone();
    }

    /// Destroy this cartridge.
    ~Cartridge() { if (mapper != nullptr) delete mapper; }

    /// Clone the cartridge, i.e., the virtual copy constructor.
    Cartridge* clone() { return new Cartridge(*this); }

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

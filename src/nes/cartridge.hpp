//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  An abstract factory for mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPER_FACTORY_HPP
#define NES_MAPPER_FACTORY_HPP

#include <exception>
#include <string>
#include <jansson.h>
#include "rom.hpp"
#include "mappers/mapper_NROM.hpp"
#include "mappers/mapper_MMC1.hpp"
#include "mappers/mapper_UNROM.hpp"
#include "mappers/mapper_CNROM.hpp"

namespace NES {

/// An exception that is raised when a mapper cannot be found for a ROM.
class MapperNotFound: public std::exception {
 protected:
    /// Error message.
    std::string msg_;

 public:
    /// Constructor (C strings).
    /// @param message C-style string error message.
    ///                The string contents are copied upon construction.
    ///                Hence, responsibility for deleting the char* lies
    ///                with the caller.
    ///
    explicit MapperNotFound(const char* message) : msg_(message) { }

    /// Constructor (C++ STL strings).
    /// @param message The error message.
    ///
    explicit MapperNotFound(const std::string& message) : msg_(message) { }

    /// Destructor.
    /// Virtual to allow for subclassing.
    ///
    virtual ~MapperNotFound() throw() { }

    /// Returns a pointer to the (constant) error description.
    /// @return A pointer to a const char*. The underlying memory
    ///         is in posession of the Exception object. Callers must
    ///         not attempt to free the memory.
    ///
    virtual const char* what() const throw() { return msg_.c_str(); }
};

/// An NES cartridge including ROM and mapper.
class Cartridge : public ROM {
 protected:
    /// the mapper for the cartridge
    Mapper* mapper = nullptr;

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
    /// @param callback a callback to update nametable mirroring on the PPU
    ///
    Cartridge(const std::string& path, Callback callback) :
        ROM(path) {
        LOG(INFO) << "loading mapper with ID " << static_cast<int>(mapper_number) << std::endl;
        switch (static_cast<MapperID>(mapper_number)) {
            case MapperID::NROM:  mapper = new MapperNROM(*this);           break;
            case MapperID::MMC1:  mapper = new MapperMMC1(*this, callback); break;
            case MapperID::UNROM: mapper = new MapperUNROM(*this);          break;
            case MapperID::CNROM: mapper = new MapperCNROM(*this);          break;
            default: throw MapperNotFound("mapper not implemented");
        }
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
    json_t* dataToJson() {
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

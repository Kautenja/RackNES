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
    Cartridge(
        const std::string& path,
        std::function<void(void)> callback
    ) : ROM(path) {
        switch (static_cast<MapperID>(mapper_number)) {
            case MapperID::NROM:  mapper = new MapperNROM(*this);           break;
            case MapperID::MMC1:  mapper = new MapperMMC1(*this, callback); break;
            case MapperID::UNROM: mapper = new MapperUNROM(*this);          break;
            case MapperID::CNROM: mapper = new MapperCNROM(*this);          break;
            default: throw MapperNotFound("mapper not implemented");
        }
    }

    // TODO: virtual deleter for ROM and ROM::Mapper
    // ~Cartridge() { delete mapper; }

    /// Return a pointer to the mapper for the cartridge.
    Mapper* get_mapper() { return mapper; }
};

}  // namespace NES

#endif  // NES_MAPPER_FACTORY_HPP

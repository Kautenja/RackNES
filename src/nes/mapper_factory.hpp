//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  An abstract factory for mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPER_FACTORY_HPP
#define NES_MAPPER_FACTORY_HPP

#include <string>
#include "cartridge.hpp"
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

/// an enumeration of mapper IDs
enum class MapperID : NES_Byte {
    NROM   = 0,
    MMC1   = 1,
    UNROM  = 2,
    CNROM  = 3,
    // MMC3   = 4,
    // AOROM  = 7
};

class CartridgeMapper : public Cartridge {
 protected:
    /// the mapper for the cartridge
    Mapper* mapper = nullptr;

 public:
    CartridgeMapper(
        const std::string& path,
        std::function<void(void)> callback
    ) : Cartridge(path) {
        switch (static_cast<MapperID>(getMapper())) {
            case MapperID::NROM:  mapper = new MapperNROM(*this);           break;
            case MapperID::MMC1:  mapper = new MapperMMC1(*this, callback); break;
            case MapperID::UNROM: mapper = new MapperUNROM(*this);          break;
            case MapperID::CNROM: mapper = new MapperCNROM(*this);          break;
            // case MapperID::MMC3:  mapper = new MapperMMC3(*this);           break;
            // case MapperID::AOROM: mapper = new MapperAOROM(*this);          break;
            default: throw MapperNotFound("mapper not implemented");
        }
    }

    // TODO: virtual deleter for Mapper
    // ~CartridgeMapper() { delete mapper; }

    Mapper* get_mapper() { return mapper; }
};

}  // namespace NES

#endif  // NES_MAPPER_FACTORY_HPP

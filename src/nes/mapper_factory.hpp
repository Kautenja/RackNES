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
class MapperNotFoundException: public std::exception {
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
    explicit MapperNotFoundException(const char* message) : msg_(message) { }

    /// Constructor (C++ STL strings).
    /// @param message The error message.
    ///
    explicit MapperNotFoundException(const std::string& message) : msg_(message) { }

    /// Destructor.
    /// Virtual to allow for subclassing.
    ///
    virtual ~MapperNotFoundException() throw() { }

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

/// Create a mapper for the given cartridge with optional callback function
///
/// @param game the cartridge to initialize a mapper for
/// @param callback the callback function for the mapper (if necessary)
///
static Cartridge::Mapper* MapperFactory(Cartridge& game, std::function<void(void)> callback) {
    switch (static_cast<MapperID>(game.getMapper())) {
        case MapperID::NROM:  return new MapperNROM(game);
        case MapperID::MMC1:  return new MapperMMC1(game, callback);
        case MapperID::UNROM: return new MapperUNROM(game);
        case MapperID::CNROM: return new MapperCNROM(game);
        // case MapperID::MMC3:  return new MapperMMC3(game);
        // case MapperID::AOROM: return new MapperAOROM(game);
        default: throw MapperNotFoundException("mapper not implemented");
    }
}

}  // namespace NES

#endif  // NES_MAPPER_FACTORY_HPP

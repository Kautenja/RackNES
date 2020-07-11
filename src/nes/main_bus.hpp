//  Program:      nes-py
//  File:         main_bus.hpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAIN_BUS_HPP
#define NES_MAIN_BUS_HPP

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <jansson.h>
#include "common.hpp"
#include "cartridge.hpp"

namespace NES {

/// The IO registers on the main bus
enum IORegisters {
    PPUCTRL =     0x2000,
    PPUMASK,
    PPUSTATUS,
    OAMADDR,
    OAMDATA,
    PPUSCROL,
    PPUADDR,
    PPUDATA,
    SQ1_VOL =     0x4000,
    SQ1_SWEEP =   0x4001,
    SQ1_LO =      0x4002,
    SQ1_HI =      0x4003,
    SQ2_VOL =     0x4004,
    SQ2_SWEEP =   0x4005,
    SQ2_LO =      0x4006,
    SQ2_HI =      0x4007,
    TRI_LINEAR =  0x4008,
    APU_UNUSED1 = 0x4009,  // unused for APU, but may be used for APU stuff
    TRI_LO =      0x400A,
    TRI_HI =      0x400B,
    NOISE_VOL =   0x400C,
    APU_UNUSED2 = 0x400D,  // unused for APU, but may be used for APU stuff
    NOISE_LO =    0x400E,
    NOISE_HI =    0x400F,
    DMC_FREQ =    0x4010,
    DMC_RAW =     0x4011,
    DMC_START =   0x4012,
    DMC_LEN =     0x4013,
    OAMDMA =      0x4014,
    SND_CHN =     0x4015,
    JOY1 =        0x4016,
    JOY2 =        0x4017,
    // $4018-$401F -> APU and I/O functionality that is normally disabled
};

// TODO: test potential performance improvements from alternate map hash algos

/// An enum functor object for calculating the hash of an enum class
/// https://stackoverflow.com/questions/18837857/cant-use-enum-class-as-unordered-map-key
struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
};

/// a type for write callback functions
typedef std::function<void(NES_Byte)> WriteCallback;
/// a map type from IORegsiters to WriteCallbacks
typedef std::unordered_map<IORegisters, WriteCallback, EnumClassHash> IORegisterToWriteCallbackMap;
/// a type for read callback functions
typedef std::function<NES_Byte(void)> ReadCallback;
/// a map type from IORegsiters to ReadCallbacks
typedef std::unordered_map<IORegisters, ReadCallback, EnumClassHash> IORegisterToReadCallbackMap;

/// The main bus for data to travel along the NES hardware
class MainBus {
 private:
    /// The RAM on the main bus
    std::vector<NES_Byte> ram = std::vector<NES_Byte>(0x800, 0);
    /// The extended RAM (if the mapper has extended RAM)
    std::vector<NES_Byte> extended_ram = std::vector<NES_Byte>(0);
    /// a pointer to the mapper on the cartridge
    ROM::Mapper* mapper = nullptr;
    /// a map of IO registers to callback methods for writes
    IORegisterToWriteCallbackMap write_callbacks;
    /// a map of IO registers to callback methods for reads
    IORegisterToReadCallbackMap read_callbacks;

 public:
    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    void set_mapper(ROM::Mapper* mapper_) {
        mapper = mapper_;
        if (mapper->hasExtendedRAM()) extended_ram.resize(0x2000);
    }

    /// Set a callback for when writes occur.
    inline void set_write_callback(IORegisters reg, WriteCallback callback) {
        write_callbacks.insert({reg, callback});
    }

    /// Set a callback for when reads occur.
    inline void set_read_callback(IORegisters reg, ReadCallback callback) {
        read_callbacks.insert({reg, callback});
    }

    /// Return a pointer to the page in memory.
    const NES_Byte* get_page_pointer(NES_Byte page) const {
        NES_Address address = page << 8;
        if (address < 0x2000) {
            return &ram[address & 0x7ff];
        } else if (address < 0x4020) {
            NES_DEBUG("Register address memory pointer access attempt");
        } else if (address < 0x6000) {
            NES_DEBUG("Expansion ROM access attempted, which is unsupported");
        } else if (address < 0x8000 && mapper->hasExtendedRAM()) {
            return &extended_ram[address - 0x6000];
        }
        return nullptr;
    }

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    inline NES_Byte* get_memory_buffer() { return &ram.front(); }

    /// Read a byte from an address on the RAM.
    ///
    /// @param address the 16-bit address of the byte to read in the RAM
    ///
    /// @return the byte located at the given address
    ///
    NES_Byte read(NES_Address address) {
        if (address < 0x2000) {
            return ram[address & 0x7ff];
        } else if (address < 0x4020) {
            if (address < 0x4000) {  // PPU registers, mirrored
                auto reg = static_cast<IORegisters>(address & 0x2007);
                if (read_callbacks.count(reg)) {
                    return read_callbacks.at(reg)();
                } else {
                    NES_DEBUG("No read callback registered for I/O register at: " << std::hex << +address);
                }
            } else if (address < 0x4018 && address >= 0x4000) {  // only *some* IO registers (mostly APU)
                auto reg = static_cast<IORegisters>(address);
                if (read_callbacks.count(reg)) {
                    return read_callbacks.at(reg)();
                } else {
                    NES_DEBUG("No read callback registered for I/O register at: " << std::hex << +address);
                }
            }
            else {
                NES_DEBUG("Read access attempt at: " << std::hex << +address);
            }
        } else if (address < 0x6000) {
            NES_DEBUG("Expansion ROM read attempted. This is currently unsupported");
        } else if (address < 0x8000) {
            if (mapper->hasExtendedRAM()) return extended_ram[address - 0x6000];
        } else {
            return mapper->readPRG(address);
        }
        return 0;
    }

    /// Write a byte to an address in the RAM.
    ///
    /// @param address the 16-bit address to write the byte to in RAM
    /// @param value the byte to write to the given address
    ///
    void write(NES_Address address, NES_Byte value) {
        if (address < 0x2000) {
            ram[address & 0x7ff] = value;
        } else if (address < 0x4020) {
            if (address < 0x4000) {  // PPU registers, mirrored
                auto reg = static_cast<IORegisters>(address & 0x2007);
                if (write_callbacks.count(reg)) {
                    return write_callbacks.at(reg)(value);
                } else {
                    NES_DEBUG("No write callback registered for I/O register at: " << std::hex << +address);
                }
            } else if (address < 0x4018 && address >= 0x4000) {  // only some registers (mostly APU)
                auto reg = static_cast<IORegisters>(address);
                if (write_callbacks.count(reg)) {
                    return write_callbacks.at(reg)(value);
                } else {
                    NES_DEBUG("No write callback registered for I/O register at: " << std::hex << +address);
                }
            } else {
                NES_DEBUG("Write access attmept at: " << std::hex << +address);
            }
        } else if (address < 0x6000) {
            NES_DEBUG("Expansion ROM write access attempted. This is currently unsupported");
        } else if (address < 0x8000) {
            if (mapper->hasExtendedRAM()) extended_ram[address - 0x6000] = value;
        } else {
            mapper->writePRG(address, value);
        }
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() const {
        json_t* rootJ = json_object();
        // encode ram
        {
            auto data_string = base64_encode(&ram[0], ram.size());
            json_object_set_new(rootJ, "ram", json_string(data_string.c_str()));
        }
        // encode extended_ram
        {
            auto data_string = base64_encode(&extended_ram[0], extended_ram.size());
            json_object_set_new(rootJ, "extended_ram", json_string(data_string.c_str()));
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
        // load extended_ram
        {
            json_t* json_data = json_object_get(rootJ, "extended_ram");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                extended_ram = std::vector<NES_Byte>(data_string.begin(), data_string.end());
            }
        }
    }
};

}  // namespace NES

#endif  // NES_MAIN_BUS_HPP

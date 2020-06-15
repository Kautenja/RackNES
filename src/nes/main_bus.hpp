//  Program:      nes-py
//  File:         main_bus.hpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAIN_BUS_HPP
#define NES_MAIN_BUS_HPP

#include <functional>
#include <vector>
#include <unordered_map>
#include "common.hpp"
#include "mapper.hpp"
#include "log.hpp"

namespace NES {

/// The IO registers on the main bus
enum IORegisters {
    PPUCTRL = 0x2000,
    PPUMASK,
    PPUSTATUS,
    OAMADDR,
    OAMDATA,
    PPUSCROL,
    PPUADDR,
    PPUDATA,
    APU0 = 0x4000,
    APU1 = 0x4001,
    APU2 = 0x4002,
    APU3 = 0x4003,
    APU4 = 0x4004,
    APU5 = 0x4005,
    APU6 = 0x4006,
    APU7 = 0x4007,
    APU8 = 0x4008,
    APU9 = 0x4009,
    DMC_STATUS = 0x4010,
    DMC_LOAD_COUNTER = 0x4011,
    DMC_SAMPLE_ADDRESS = 0x4012,
    DMC_SAMPLE_LENGTH = 0x4013,
    OAMDMA = 0x4014,
    APUSTATUS = 0x4015,
    JOY1 = 0x4016,
    JOY2 = 0x4017,
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
    std::vector<NES_Byte> ram;
    /// The extended RAM (if the mapper has extended RAM)
    std::vector<NES_Byte> extended_ram;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;
    /// a map of IO registers to callback methods for writes
    IORegisterToWriteCallbackMap write_callbacks;
    /// a map of IO registers to callback methods for reads
    IORegisterToReadCallbackMap read_callbacks;

 public:
    /// Initialize a new main bus.
    MainBus() : ram(0x800, 0), mapper(nullptr) { }

    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    void set_mapper(Mapper* mapper) {
        this->mapper = mapper;
        if (mapper->hasExtendedRAM())
            extended_ram.resize(0x2000);
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
    const NES_Byte* get_page_pointer(NES_Byte page) {
        NES_Address address = page << 8;
        if (address < 0x2000)
            return &ram[address & 0x7ff];
        else if (address < 0x4020)
            LOG(Error) << "Register address memory pointer access attempt" << std::endl;
        else if (address < 0x6000)
            LOG(Error) << "Expansion ROM access attempted, which is unsupported" << std::endl;
        else if (address < 0x8000)
            if (mapper->hasExtendedRAM())
                return &extended_ram[address - 0x6000];

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
                if (read_callbacks.count(reg))
                    return read_callbacks.at(reg)();
                else
                    LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
            } else if (address < 0x4018 && address >= 0x4000) {  // only *some* IO registers (mostly APU)
                auto reg = static_cast<IORegisters>(address);
                if (read_callbacks.count(reg))
                    return read_callbacks.at(reg)();
                else
                    LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
            }
            else {
                LOG(InfoVerbose) << "Read access attempt at: " << std::hex << +address << std::endl;
            }
        } else if (address < 0x6000) {
            LOG(InfoVerbose) << "Expansion ROM read attempted. This is currently unsupported" << std::endl;
        } else if (address < 0x8000) {
            if (mapper->hasExtendedRAM())
                return extended_ram[address - 0x6000];
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
                if (write_callbacks.count(reg))
                    return write_callbacks.at(reg)(value);
                else
                    LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
            } else if (address < 0x4018 && address >= 0x4000) {  // only some registers (mostly APU)
                auto reg = static_cast<IORegisters>(address);
                if (write_callbacks.count(reg))
                    return write_callbacks.at(reg)(value);
                else
                    LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
            } else {
                LOG(InfoVerbose) << "Write access attmept at: " << std::hex << +address << std::endl;
            }
        } else if (address < 0x6000) {
            LOG(InfoVerbose) << "Expansion ROM access attempted. This is currently unsupported" << std::endl;
        } else if (address < 0x8000) {
            if (mapper->hasExtendedRAM())
                extended_ram[address - 0x6000] = value;
        } else {
            mapper->writePRG(address, value);
        }
    }
};

}  // namespace NES

#endif  // NES_MAIN_BUS_HPP

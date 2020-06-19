//  Program:      nes-py
//  File:         cpu.hpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_CPU_HPP
#define NES_CPU_HPP

#include <jansson.h>
#include "common.hpp"
#include "cpu_opcodes.hpp"
#include "main_bus.hpp"

namespace NES {

/// The MOS6502 CPU for the Nintendo Entertainment System (NES).
class CPU {
 private:
    /// The program counter register
    NES_Address register_PC = 0x34;
    /// The stack pointer register
    NES_Byte register_SP = 0xFD;
    /// The A register
    NES_Byte register_A = 0;
    /// The X register
    NES_Byte register_X = 0;
    /// The Y register
    NES_Byte register_Y = 0;
    /// The flags register
    CPU_Flags flags = {.byte = 0b00110100};
    /// The number of cycles to skip
    int skip_cycles = 0;
    /// The number of cycles the CPU has run
    int cycles = 0;

    /// Set the zero and negative flags based on the given value.
    ///
    /// @param value the value to set the zero and negative flags using
    ///
    inline void set_ZN(NES_Byte value) {
        flags.bits.Z = !value; flags.bits.N = value & 0x80;
    }

    /// Read a 16-bit address from the bus given an address.
    ///
    /// @param bus the bus to read data from
    /// @param address the address in memory to read an address from
    /// @return the 16-bit address located at the given memory address
    ///
    inline NES_Address read_address(MainBus &bus, NES_Address address) {
        return bus.read(address) | bus.read(address + 1) << 8;
    }

    /// Push a value onto the stack.
    ///
    /// @param bus the bus to read data from
    /// @param value the value to push onto the stack
    ///
    inline void push_stack(MainBus &bus, NES_Byte value) {
        bus.write(0x100 | register_SP--, value);
    }

    /// Pop a value off the stack.
    ///
    /// @param bus the bus to read data from
    /// @return the value on the top of the stack
    ///
    inline NES_Byte pop_stack(MainBus &bus) {
        return bus.read(0x100 | ++register_SP);
    }

    /// Increment the skip cycles if two addresses refer to different pages.
    ///
    /// @param a an address
    /// @param b another address
    /// @param inc the number of skip cycles to add
    ///
    inline void set_page_crossed(NES_Address a, NES_Address b, int inc = 1) {
        if ((a & 0xff00) != (b & 0xff00)) skip_cycles += inc;
    }

    /// Execute an implied mode instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool implied(MainBus &bus, NES_Byte opcode);

    /// Execute a branch instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool branch(MainBus &bus, NES_Byte opcode);

    /// Execute a type 0 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type0(MainBus &bus, NES_Byte opcode);

    /// Execute a type 1 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type1(MainBus &bus, NES_Byte opcode);

    /// Execute a type 2 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type2(MainBus &bus, NES_Byte opcode);

    /// Reset the emulator using the given starting address.
    ///
    /// @param start_address the starting address for the program counter
    ///
    void reset(NES_Address start_address);

 public:
    /// The interrupt types available to this CPU
    enum InterruptType {
        IRQ_INTERRUPT,
        NMI_INTERRUPT,
        BRK_INTERRUPT,
    };

    /// Reset using the given main bus to lookup a starting address.
    ///
    /// @param bus the main bus of the NES emulator
    ///
    inline void reset(MainBus &bus) { reset(read_address(bus, RESET_VECTOR)); }

    /// Interrupt the CPU.
    ///
    /// @param bus the main bus of the machine
    /// @param type the type of interrupt to issue
    ///
    /// TODO: Assuming sequential execution, for asynchronously calling this
    ///       with Execute, further work needed
    ///
    void interrupt(MainBus &bus, InterruptType type);

    /// Perform a full CPU cycle using and storing data in the given bus.
    ///
    /// @param bus the bus to read and write data from / to
    ///
    void cycle(MainBus &bus);

    /// Skip DMA cycles.
    ///
    /// 513 = 256 read + 256 write + 1 dummy read
    /// &1 -> +1 if on odd cycle
    ///
    inline void skip_DMA_cycles() { skip_cycles += 513 + (cycles & 1); }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "register_PC", json_integer(register_PC));
        json_object_set_new(rootJ, "register_SP", json_integer(register_SP));
        json_object_set_new(rootJ, "register_A", json_integer(register_A));
        json_object_set_new(rootJ, "register_X", json_integer(register_X));
        json_object_set_new(rootJ, "register_Y", json_integer(register_Y));
        json_object_set_new(rootJ, "flags", json_integer(flags.byte));
        json_object_set_new(rootJ, "skip_cycles", json_integer(skip_cycles));
        json_object_set_new(rootJ, "cycles", json_integer(cycles));
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load register_PC
        json_t* register_PC_ = json_object_get(rootJ, "register_PC");
        if (register_PC_)
            register_PC = json_integer_value(register_PC_);
        // load register_SP
        json_t* register_SP_ = json_object_get(rootJ, "register_SP");
        if (register_SP_)
            register_SP = json_integer_value(register_SP_);
        // load register_A
        json_t* register_A_ = json_object_get(rootJ, "register_A");
        if (register_A_)
            register_A = json_integer_value(register_A_);
        // load register_X
        json_t* register_X_ = json_object_get(rootJ, "register_X");
        if (register_X_)
            register_X = json_integer_value(register_X_);
        // load register_Y
        json_t* register_Y_ = json_object_get(rootJ, "register_Y");
        if (register_Y_)
            register_Y = json_integer_value(register_Y_);
        // load flags
        json_t* flags_ = json_object_get(rootJ, "flags");
        if (flags_)
            flags.byte = json_integer_value(flags_);
        // load skip_cycles
        json_t* skip_cycles_ = json_object_get(rootJ, "skip_cycles");
        if (skip_cycles_)
            skip_cycles = json_integer_value(skip_cycles_);
        // load cycles
        json_t* cycles_ = json_object_get(rootJ, "cycles");
        if (cycles_)
            cycles = json_integer_value(cycles_);
    }
};

}  // namespace NES

#endif  // NES_CPU_HPP

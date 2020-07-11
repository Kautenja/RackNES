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
    union {
        struct {
            bool N : 1,
                 V : 1,
                   : 1,
                 B : 1,
                 D : 1,
                 I : 1,
                 Z : 1,
                 C : 1;
        } bits;
        NES_Byte byte;
    } flags = {.byte = 0b00110100};

    /// The number of cycles to skip
    int skip_cycles = 0;
    /// The number of cycles the CPU has run
    int cycles = 0;

    /// Set the zero and negative flags based on the given value.
    ///
    /// @param value the value to set the zero and negative flags using
    ///
    inline void set_ZN(NES_Byte value) {
        // set zero high if the value is not zero
        flags.bits.Z = !value;
        // set negative high if the most significant bit is 1
        flags.bits.N = value & 0x80;
    }

    /// Read a 16-bit address from the bus given an address.
    ///
    /// @param bus the bus to read data from
    /// @param address the address in memory to read an address from
    /// @return the 16-bit address located at the given memory address
    ///
    inline NES_Address read_address(MainBus &bus, NES_Address address) const {
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

    // -----------------------------------------------------------------------
    // MARK: Implied Instructions
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    // MARK: Type 0 Instructions
    // -----------------------------------------------------------------------

    inline void BIT(MainBus &bus, NES_Byte opcode) {
        NES_Address operand = bus.read(type0_address(bus, opcode));
        flags.bits.Z = !(register_A & operand);
        flags.bits.V = operand & 0x40;
        flags.bits.N = operand & 0x80;
    }

    inline void STY(MainBus &bus, NES_Byte opcode) {
        bus.write(type0_address(bus, opcode), register_Y);
    }

    inline void LDY(MainBus &bus, NES_Byte opcode) {
        register_Y = bus.read(type0_address(bus, opcode));
        set_ZN(register_Y);
    }

    inline void CPY(MainBus &bus, NES_Byte opcode) {
        NES_Address diff = register_Y - bus.read(type0_address(bus, opcode));
        flags.bits.C = !(diff & 0x100);
        set_ZN(diff);
    }

    inline void CPX(MainBus &bus, NES_Byte opcode) {
        NES_Address diff = register_X - bus.read(type0_address(bus, opcode));
        flags.bits.C = !(diff & 0x100);
        set_ZN(diff);
    }

    // -----------------------------------------------------------------------
    // MARK: Type 1 Instructions
    // -----------------------------------------------------------------------

    inline void ORA(MainBus &bus, NES_Byte opcode) {
        register_A |= bus.read(type1_address(bus, opcode));
        set_ZN(register_A);
    }

    inline void AND(MainBus &bus, NES_Byte opcode) {
        register_A &= bus.read(type1_address(bus, opcode));
        set_ZN(register_A);
    }

    inline void EOR(MainBus &bus, NES_Byte opcode) {
        register_A ^= bus.read(type1_address(bus, opcode));
        set_ZN(register_A);
    }

    inline void ADC(MainBus &bus, NES_Byte opcode) {
        NES_Byte operand = bus.read(type1_address(bus, opcode));
        NES_Address sum = register_A + operand + flags.bits.C;
        // Carry forward or UNSIGNED overflow
        flags.bits.C = sum & 0x100;
        // SIGNED overflow, would only happen if the sign of sum is
        // different from BOTH the operands
        flags.bits.V = (register_A ^ sum) & (operand ^ sum) & 0x80;
        register_A = static_cast<NES_Byte>(sum);
        set_ZN(register_A);
    }

    inline void STA(MainBus &bus, NES_Byte opcode) {
        bus.write(type1_address(bus, opcode, true), register_A);
    }

    inline void LDA(MainBus &bus, NES_Byte opcode) {
        register_A = bus.read(type1_address(bus, opcode));
        set_ZN(register_A);
    }

    inline void CMP(MainBus &bus, NES_Byte opcode) {
        NES_Address diff = register_A - bus.read(type1_address(bus, opcode));
        flags.bits.C = !(diff & 0x100);
        set_ZN(diff);
    }

    inline void SBC(MainBus &bus, NES_Byte opcode) {
        // High carry means "no borrow", thus negate and subtract
        NES_Address subtrahend = bus.read(type1_address(bus, opcode));
        NES_Address diff = register_A - subtrahend - !flags.bits.C;
        // if the ninth bit is 1, the resulting number is negative => borrow => low carry
        flags.bits.C = !(diff & 0x100);
        // Same as ADC, except instead of the subtrahend,
        // substitute with it's one complement
        flags.bits.V = (register_A ^ diff) & (~subtrahend ^ diff) & 0x80;
        register_A = diff;
        set_ZN(diff);
    }

    // -----------------------------------------------------------------------
    // MARK: Type 2 Instructions
    // -----------------------------------------------------------------------

    inline void ASL(MainBus &bus, NES_Byte opcode) {
        auto address_mode = static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
        auto address = type2_address(bus, opcode);
        if (address_mode == AddressMode2::Accumulator) {
            // auto prev_C = flags.bits.C;
            flags.bits.C = register_A & 0x80;
            register_A <<= 1;
            //If Rotating, set the bit-0 to the the previous carry
            // register_A = register_A | (prev_C && 0);
            set_ZN(register_A);
        } else {
            // auto prev_C = flags.bits.C;
            NES_Address operand = bus.read(address);
            flags.bits.C = operand & 0x80;
            // operand = operand << 1 | (prev_C && 0);
            set_ZN(operand);
            bus.write(address, operand);
        }
    }

    inline void ROL(MainBus &bus, NES_Byte opcode) {
        auto address_mode = static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
        auto address = type2_address(bus, opcode);
        if (address_mode == AddressMode2::Accumulator) {
            auto prev_C = flags.bits.C;
            flags.bits.C = register_A & 0x80;
            register_A <<= 1;
            // If Rotating, set the bit-0 to the the previous carry
            register_A = register_A | prev_C;
            set_ZN(register_A);
        } else {
            auto prev_C = flags.bits.C;
            NES_Address operand = bus.read(address);
            flags.bits.C = operand & 0x80;
            // If Rotating, set the bit-0 to the the previous carry
            operand = operand << 1 | prev_C;
            set_ZN(operand);
            bus.write(address, operand);
        }
    }

    inline void LSR(MainBus &bus, NES_Byte opcode) {
        auto address_mode = static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
        auto address = type2_address(bus, opcode);
        if (address_mode == AddressMode2::Accumulator) {
            // auto prev_C = flags.bits.C;
            flags.bits.C = register_A & 1;
            register_A >>= 1;
            //If Rotating, set the bit-7 to the previous carry
            // register_A = register_A | (prev_C && 0) << 7;
            set_ZN(register_A);
        } else {
            // auto prev_C = flags.bits.C;
            NES_Address operand = bus.read(address);
            flags.bits.C = operand & 1;
            // operand = operand >> 1 | (prev_C && 0) << 7;
            set_ZN(operand);
            bus.write(address, operand);
        }
    }

    inline void ROR(MainBus &bus, NES_Byte opcode, bool is_accum) {
        auto address_mode = static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
        auto address = type2_address(bus, opcode);
        if (address_mode == AddressMode2::Accumulator) {
            auto prev_C = flags.bits.C;
            flags.bits.C = register_A & 1;
            register_A >>= 1;
            // If Rotating, set the bit-7 to the previous carry
            register_A = register_A | prev_C << 7;
            set_ZN(register_A);
        } else {
            auto prev_C = flags.bits.C;
            NES_Address operand = bus.read(address);
            flags.bits.C = operand & 1;
            // If Rotating, set the bit-7 to the previous carry
            operand = operand >> 1 | prev_C << 7;
            set_ZN(operand);
            bus.write(address, operand);
        }
    }

    inline void STX(MainBus &bus, NES_Byte opcode) {
        bus.write(type2_address(bus, opcode, true), register_X);
    }

    inline void LDX(MainBus &bus, NES_Byte opcode) {
        register_X = bus.read(type2_address(bus, opcode, true));
        set_ZN(register_X);
    }

    inline void DEC(MainBus &bus, NES_Byte opcode) {
        NES_Address address = type2_address(bus, opcode);
        auto tmp = bus.read(address) - 1;
        set_ZN(tmp);
        bus.write(address, tmp);
    }

    inline void INC(MainBus &bus, NES_Byte opcode) {
        NES_Address address = type2_address(bus, opcode);
        auto tmp = bus.read(address) + 1;
        set_ZN(tmp);
        bus.write(address, tmp);
    }

    /// The flag to check for a branch operation.
    enum class BranchFlagType: NES_Byte {
        NEGATIVE,
        OVERFLOW,
        CARRY,
        ZERO,
    };

    /// Execute a branch instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    inline void branch(MainBus &bus, NES_Byte opcode) {
        // a mask for checking the status bit of the opcode
        static constexpr NES_Byte STATUS_BIT_MASK = 0b00100000;
        // the number of bits to shift the opcode to the right to get flag type
        static constexpr auto FLAG_TYPE_SHIFTS = 6;
        // set branch to true if the given condition is met by the given flag
        switch (static_cast<BranchFlagType>(opcode >> FLAG_TYPE_SHIFTS)) {
        case BranchFlagType::NEGATIVE: {  // use XNOR to set
            if (!(static_cast<bool>(opcode & STATUS_BIT_MASK) ^ flags.bits.N))
                goto branch_to_newPC;
            break;
        }
        case BranchFlagType::OVERFLOW: {  // use XNOR to set
            if (!(static_cast<bool>(opcode & STATUS_BIT_MASK) ^ flags.bits.V))
                goto branch_to_newPC;
            break;
        }
        case BranchFlagType::CARRY: {  // use XNOR to set
            if (!(static_cast<bool>(opcode & STATUS_BIT_MASK) ^ flags.bits.C))
                goto branch_to_newPC;
            break;
        }
        case BranchFlagType::ZERO: {  // use XNOR to set
            if (!(static_cast<bool>(opcode & STATUS_BIT_MASK) ^ flags.bits.Z))
                goto branch_to_newPC;
            break;
        }
        default: break;
        }
        ++register_PC;
        return;
    branch_to_newPC:
        int8_t offset = bus.read(register_PC++);
        ++skip_cycles;
        auto newPC = static_cast<NES_Address>(register_PC + offset);
        set_page_crossed(register_PC, newPC, 2);
        register_PC = newPC;
        return;
    }

    /// a mask for determining addressing mode from opcode
    static constexpr auto ADRESS_MODE_MASK = 0x1c;
    /// the number of shifts to turn masked address mode into an enumeration
    static constexpr auto ADDRESS_MODE_SHIFT = 2;

    /// Addressing modes for type 1 instructions:
    /// ORA, AND, EOR, ADC, STA, LDA, CMP, SBC
    enum class AddressMode1: NES_Byte {
        IndexedIndirectX,
        ZeroPage,
        Immediate,
        Absolute,
        IndirectY,
        IndexedX,
        AbsoluteY,
        AbsoluteX,
    };

    /// @brief Return an address for a type 1 instruction:
    /// ORA, AND, EOR, ADC, STA, LDA, CMP, SBC
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @param is_STA whether the opcode is STA
    ///
    NES_Address type1_address(MainBus &bus, NES_Byte opcode, bool is_STA = false) {
        NES_Address location = 0;
        switch (static_cast<AddressMode1>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case AddressMode1::IndexedIndirectX: {
            NES_Byte zero_address = register_X + bus.read(register_PC++);
            // Addresses wrap in zero page mode, thus pass through a mask
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            break;
        }
        case AddressMode1::ZeroPage:
            location = bus.read(register_PC++);
            break;
        case AddressMode1::Immediate:
            location = register_PC++;
            break;
        case AddressMode1::Absolute:
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        case AddressMode1::IndirectY: {
            NES_Byte zero_address = bus.read(register_PC++);
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            if (!is_STA) set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        }
        case AddressMode1::IndexedX:
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        case AddressMode1::AbsoluteY:
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (!is_STA) set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        case AddressMode1::AbsoluteX:
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (!is_STA) set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        }
        return location;
    }

    /// Addressing modes for type 2 instructions:
    /// ASL, ROL, LSR, ROR, STX, LDX, DEC, INC
    /// and type 0 instructions:
    /// BIT, STY, LDY, CPY, CPX
    enum class AddressMode2: NES_Byte {
        Immediate,
        ZeroPage,
        Accumulator,
        Absolute,
        Indexed         = 5,
        AbsoluteIndexed = 7,
    };

    /// @brief Return an address for a type 2 instruction:
    /// ASL, ROL, LSR, ROR, STX, LDX, DEC, INC
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @param is_LDX_or_STX whether the opcode is LDX or STX
    ///
    NES_Address type2_address(MainBus &bus, NES_Byte opcode, bool is_LDX_or_STX = false) {
        NES_Address location = 0;
        switch (static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case AddressMode2::Immediate:
            location = register_PC++;
            break;
        case AddressMode2::ZeroPage:
            location = bus.read(register_PC++);
            break;
        case AddressMode2::Accumulator:
            break;
        case AddressMode2::Absolute:
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        case AddressMode2::Indexed:
            location = bus.read(register_PC++);
            // The mask wraps address around zero page
            location = (location + (is_LDX_or_STX ? register_Y : register_X)) & 0xff;
            break;
        case AddressMode2::AbsoluteIndexed:
            location = read_address(bus, register_PC);
            register_PC += 2;
            auto index = is_LDX_or_STX ? register_Y : register_X;
            set_page_crossed(location, location + index);
            location += index;
            break;
        }
        return location;
    }

    /// @brief Return an address for a type 0 instruction:
    /// BIT, STY, LDY, CPY, CPX
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    ///
    NES_Address type0_address(MainBus &bus, NES_Byte opcode) {
        NES_Address location = 0;
        switch (static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case AddressMode2::Immediate:
            location = register_PC++;
            break;
        case AddressMode2::ZeroPage:
            location = bus.read(register_PC++);
            break;
        case AddressMode2::Accumulator:
            break;
        case AddressMode2::Absolute:
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        case AddressMode2::Indexed:
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        case AddressMode2::AbsoluteIndexed:
            location = read_address(bus, register_PC);
            register_PC += 2;
            set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        }
        return location;
    }

    /// Execute a type 1 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type1(MainBus &bus, NES_Byte opcode) {
        if ((opcode & INSTRUCTION_MODE_MASK) != 0x1) return false;
        switch (static_cast<Operation1>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
        case Operation1::ORA: ORA(bus, opcode); break;
        case Operation1::AND: AND(bus, opcode); break;
        case Operation1::EOR: EOR(bus, opcode); break;
        case Operation1::ADC: ADC(bus, opcode); break;
        case Operation1::STA: STA(bus, opcode); break;
        case Operation1::LDA: LDA(bus, opcode); break;
        case Operation1::CMP: CMP(bus, opcode); break;
        case Operation1::SBC: SBC(bus, opcode); break;
        default: return false;
        }
        return true;
    }

    /// Execute a type 2 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type2(MainBus &bus, NES_Byte opcode);

    /// Execute a type 0 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type0(MainBus &bus, NES_Byte opcode) {
        if ((opcode & INSTRUCTION_MODE_MASK) != 0x0) return false;
        switch (static_cast<Operation0>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
        case Operation0::BIT: BIT(bus, opcode); break;
        case Operation0::STY: STY(bus, opcode); break;
        case Operation0::LDY: LDY(bus, opcode); break;
        case Operation0::CPY: CPY(bus, opcode); break;
        case Operation0::CPX: CPX(bus, opcode); break;
        default: return false;
        }
        return true;
    }

    /// Decode and execute the given opcode using the given bus.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool decode_execute(MainBus &bus, NES_Byte opcode);

    /// Reset the emulator using the given starting address.
    ///
    /// @param start_address the starting address for the program counter
    ///
    inline void reset(NES_Address start_address) {
        register_PC = start_address;
        register_SP = 0xfd;
        register_A = 0;
        register_X = 0;
        register_Y = 0;
        flags.byte = 0b00110100;
        skip_cycles = 0;
        cycles = 0;
    }

 public:
    /// The interrupt types available for this CPU
    enum InterruptType {
        IRQ_INTERRUPT,
        NMI_INTERRUPT,
        BRK_INTERRUPT,
    };

    /// @brief Reset using the given main bus to lookup a starting address.
    ///
    /// @param bus the main bus of the NES emulator
    ///
    inline void reset(MainBus &bus) { reset(read_address(bus, RESET_VECTOR)); }

    /// @brief Interrupt the CPU.
    ///
    /// @param bus the main bus of the machine
    /// @param type the type of interrupt to issue
    ///
    void interrupt(MainBus &bus, InterruptType type) {
        if (flags.bits.I && type != NMI_INTERRUPT && type != BRK_INTERRUPT)
            return;
        // Add one if BRK, a quirk of 6502
        if (type == BRK_INTERRUPT)
            ++register_PC;
        // push values on to the stack
        push_stack(bus, register_PC >> 8);
        push_stack(bus, register_PC);
        auto brk = static_cast<NES_Byte>(type == BRK_INTERRUPT) << 4;
        push_stack(bus, flags.byte | 0b00100000 | brk);
        // set the interrupt flag
        flags.bits.I = true;
        // handle the kind of interrupt
        switch (type) {
            case IRQ_INTERRUPT:
            case BRK_INTERRUPT:
                register_PC = read_address(bus, IRQ_VECTOR);
                break;
            case NMI_INTERRUPT:
                register_PC = read_address(bus, NMI_VECTOR);
                break;
        }
        // add the number of cycles to handle the interrupt
        skip_cycles += 7;
    }

    /// @brief Perform a full cycle
    ///
    /// @param bus the bus to read and write data from / to
    ///
    void cycle(MainBus &bus) {
        // increment the number of cycles
        ++cycles;
        // if in a skip cycle, return
        if (skip_cycles-- > 1) return;
        // reset the number of skip cycles to 0
        skip_cycles = 0;
        // read the opcode from the bus and lookup the number of cycles
        NES_Byte op = bus.read(register_PC++);
        // Using short-circuit evaluation, call the other function only if the
        // first failed. ExecuteImplied must be called first and ExecuteBranch
        // must be before ExecuteType0
        if (decode_execute(bus, op) || type1(bus, op) || type2(bus, op) || type0(bus, op))
            skip_cycles += OPERATION_CYCLES[op];
        else
            LOG(Error) << "failed to execute opcode: " << std::hex << +op << std::endl;
    }

    /// @brief Skip DMA cycles.
    ///
    /// 513 = 256 read + 256 write + 1 dummy read
    /// &1 -> +1 if on odd cycle
    ///
    inline void skip_DMA_cycles() { skip_cycles += 513 + (cycles & 1); }

    /// @brief Convert the object's state to a JSON object.
    json_t* dataToJson() const {
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

    /// @brief Load the object's state from a JSON object.
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

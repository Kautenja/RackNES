//  Program:      nes-py
//  File:         cpu.cpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "cpu.hpp"
#include "log.hpp"

namespace NES {

bool CPU::decode_execute(NES_Byte opcode, MainBus &bus) {
    switch (static_cast<OpcodeTable>(opcode)) {
    case OpcodeTable::BRK: {
        interrupt(bus, BRK_INTERRUPT);
        break;
    }
    // case OpcodeTable::ORA__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::ORA__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::ASL__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::PHP: {
        push_stack(bus, flags.byte);
        break;
    }
    // case OpcodeTable::ORA__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::ASL__ACCUMULATOR: {
    //     break;
    // }
    // case OpcodeTable::ORA__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::ASL__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BPL: {
    //     break;
    // }
    // case OpcodeTable::ORA__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::ORA__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::ASL__ZERO_PAGE_X: {
    //     break;
    // }
    case OpcodeTable::CLC: {
        flags.bits.C = false;
        break;
    }
    // case OpcodeTable::ORA__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::ORA__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::ASL__ABSOLUTE_X: {
    //     break;
    // }
    case OpcodeTable::JSR: {
        // Push address of next instruction - 1, thus register_PC + 1
        // instead of register_PC + 2 since register_PC and
        // register_PC + 1 are address of subroutine
        push_stack(bus, static_cast<NES_Byte>((register_PC + 1) >> 8));
        push_stack(bus, static_cast<NES_Byte>(register_PC + 1));
        register_PC = read_address(bus, register_PC);
        break;
    }
    // case OpcodeTable::AND__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::BIT__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::AND__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::ROL__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::PLP: {
        flags.byte = pop_stack(bus);
        break;
    }
    // case OpcodeTable::AND__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::ROL__ACCUMULATOR: {
    //     break;
    // }
    // case OpcodeTable::BIT__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::AND__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::ROL__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BMI: {
    //     break;
    // }
    // case OpcodeTable::AND__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::AND__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::ROL__ZERO_PAGE_X: {
    //     break;
    // }
    case OpcodeTable::SEC: {
        flags.bits.C = true;
        break;
    }
    // case OpcodeTable::AND__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::AND__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::ROL__ABSOLUTE_X: {
    //     break;
    // }
    case OpcodeTable::RTI: {
        flags.byte = pop_stack(bus);
        register_PC = pop_stack(bus);
        register_PC |= pop_stack(bus) << 8;
        break;
    }
    // case OpcodeTable::EOR__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::EOR__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::LSR__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::PHA: {
        push_stack(bus, register_A);
        break;
    }
    // case OpcodeTable::EOR__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::LSR__ACCUMULATOR: {
    //     break;
    // }
    case OpcodeTable::JMP__ABSOLUTE: {
        register_PC = read_address(bus, register_PC);
        break;
    }
    // case OpcodeTable::EOR__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::LSR__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BVC: {
    //     break;
    // }
    // case OpcodeTable::EOR__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::EOR__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::LSR__ZERO_PAGE_X: {
    //     break;
    // }
    case OpcodeTable::CLI: {
        flags.bits.I = false;
        break;
    }
    // case OpcodeTable::EOR__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::EOR__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::LSR__ABSOLUTE_X: {
    //     break;
    // }
    case OpcodeTable::RTS: {
        register_PC = pop_stack(bus);
        register_PC |= pop_stack(bus) << 8;
        ++register_PC;
        break;
    }
    // case OpcodeTable::ADC__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::ADC__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::ROR__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::PLA: {
        register_A = pop_stack(bus);
        set_ZN(register_A);
        break;
    }
    // case OpcodeTable::ADC__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::ROR__ACCUMULATOR: {
    //     break;
    // }
    case OpcodeTable::JMP__INDIRECT: {
        NES_Address location = read_address(bus, register_PC);
        // 6502 has a bug such that the when the vector of an indirect
        // address begins at the last byte of a page, the second byte
        // is fetched from the beginning of that page rather than the
        // beginning of the next
        // Recreating here:
        NES_Address Page = location & 0xff00;
        register_PC = bus.read(location) | bus.read(Page | ((location + 1) & 0xff)) << 8;
        break;
    }
    // case OpcodeTable::ADC__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::ROR__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BVS: {
    //     break;
    // }
    // case OpcodeTable::ADC__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::ADC__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::ROR__ZERO_PAGE_X: {
    //     break;
    // }
    case OpcodeTable::SEI: {
        flags.bits.I = true;
        break;
    }
    // case OpcodeTable::ADC__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::ADC__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::ROR__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::STA__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::STY__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::STA__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::STX__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::DEY: {
        --register_Y;
        set_ZN(register_Y);
        break;
    }
    case OpcodeTable::TXA: {
        register_A = register_X;
        set_ZN(register_A);
        break;
    }
    // case OpcodeTable::STY__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::STA__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::STX__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BCC: {
    //     break;
    // }
    // case OpcodeTable::STA__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::STY__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::STA__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::STX__ZERO_PAGE_Y: {
    //     break;
    // }
    case OpcodeTable::TYA: {
        register_A = register_Y;
        set_ZN(register_A);
        break;
    }
    // case OpcodeTable::STA__ABSOLUTE_Y: {
    //     break;
    // }
    case OpcodeTable::TXS: {
        register_SP = register_X;
        break;
    }
    // case OpcodeTable::STA__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::LDY__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::LDA__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::LDX__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::LDY__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::LDA__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::LDX__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::TAY: {
        register_Y = register_A;
        set_ZN(register_Y);
        break;
    }
    // case OpcodeTable::LDA__IMMEDIATE: {
    //     break;
    // }
    case OpcodeTable::TAX: {
        register_X = register_A;
        set_ZN(register_X);
        break;
    }
    // case OpcodeTable::LDY__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::LDA__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::LDX__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BCS: {
    //     break;
    // }
    // case OpcodeTable::LDA__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::LDY__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::LDA__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::LDX__ZERO_PAGE_Y: {
    //     break;
    // }
    case OpcodeTable::CLV: {
        flags.bits.V = false;
        break;
    }
    // case OpcodeTable::LDA__ABSOLUTE_Y: {
    //     break;
    // }
    case OpcodeTable::TSX: {
        register_X = register_SP;
        set_ZN(register_X);
        break;
    }
    // case OpcodeTable::LDY__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::LDA__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::LDX__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::CPY__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::CMP__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::CPY__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::CMP__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::DEC__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::INY: {
        ++register_Y;
        set_ZN(register_Y);
        break;
    }
    // case OpcodeTable::CMP__IMMEDIATE: {
    //     break;
    // }
    case OpcodeTable::DEX: {
        --register_X;
        set_ZN(register_X);
        break;
    }
    // case OpcodeTable::CPY__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::CMP__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::DEC__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BNE: {
    //     break;
    // }
    // case OpcodeTable::CMP__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::CMP__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::DEC__ZERO_PAGE_X: {
    //     break;
    // }
    case OpcodeTable::CLD: {
        flags.bits.D = false;
        break;
    }
    // case OpcodeTable::CMP__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::CMP__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::DEC__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::CPX__IMMEDIATE: {
    //     break;
    // }
    // case OpcodeTable::SBC__INDIRECT_X: {
    //     break;
    // }
    // case OpcodeTable::CPX__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::SBC__ZERO_PAGE: {
    //     break;
    // }
    // case OpcodeTable::INC__ZERO_PAGE: {
    //     break;
    // }
    case OpcodeTable::INX: {
        ++register_X;
        set_ZN(register_X);
        break;
    }
    // case OpcodeTable::SBC__IMMEDIATE: {
    //     break;
    // }
    case OpcodeTable::NOP: {
        break;
    }
    // case OpcodeTable::CPX__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::SBC__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::INC__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::BEQ: {
    //     break;
    // }
    // case OpcodeTable::SBC__INDIRECT_Y: {
    //     break;
    // }
    // case OpcodeTable::SBC__ZERO_PAGE_X: {
    //     break;
    // }
    // case OpcodeTable::INC__ZERO_PAGE_X: {
    //     break;
    // }
    case OpcodeTable::SED: {
        flags.bits.D = true;
        break;
    }
    // case OpcodeTable::SBC__ABSOLUTE_Y: {
    //     break;
    // }
    // case OpcodeTable::SBC__ABSOLUTE_X: {
    //     break;
    // }
    // case OpcodeTable::INC__ABSOLUTE_X: {
    //     break;
    // }
    default: return false;
    }
    return true;
}

bool CPU::branch(MainBus &bus, NES_Byte opcode) {
    static constexpr NES_Byte BRANCH_INSTRUCTION_MASK = 0x1f;
    static constexpr NES_Byte BRANCH_INSTRUCTION_MASK_RESULT = 0x10;
    if ((opcode & BRANCH_INSTRUCTION_MASK) != BRANCH_INSTRUCTION_MASK_RESULT)
        return false;

    // initialize branch to the status bit
    static constexpr NES_Byte STATUS_BIT_MASK = 0b00100000;
    bool branch = opcode & STATUS_BIT_MASK;
    // the number of bits to shift the opcode to the right to get the flag type
    static constexpr auto FLAG_TYPE_SHIFTS = 6;
    // set branch to true if the given condition is met by the given flag
    switch (static_cast<BranchFlagType>(opcode >> FLAG_TYPE_SHIFTS)) {
        case BranchFlagType::NEGATIVE: {  // use XNOR to set
            branch = !(branch ^ flags.bits.N);
            break;
        }
        case BranchFlagType::OVERFLOW: {  // use XNOR to set
            branch = !(branch ^ flags.bits.V);
            break;
        }
        case BranchFlagType::CARRY: {  // use XNOR to set
            branch = !(branch ^ flags.bits.C);
            break;
        }
        case BranchFlagType::ZERO: {  // use XNOR to set
            branch = !(branch ^ flags.bits.Z);
            break;
        }
        default: return false;
    }
    if (branch) {  // set program counter to branch location
        int8_t offset = bus.read(register_PC++);
        ++skip_cycles;
        auto newPC = static_cast<NES_Address>(register_PC + offset);
        set_page_crossed(register_PC, newPC, 2);
        register_PC = newPC;
    } else {  // increment program counter
        ++register_PC;
    }
    return true;
}

bool CPU::type0(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 0x0)
        return false;

    NES_Address location = 0;
    switch (static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case M2_IMMEDIATE: {
            location = register_PC++;
            break;
        }
        case M2_ZERO_PAGE: {
            location = bus.read(register_PC++);
            break;
        }
        case M2_ABSOLUTE: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        }
        case M2_INDEXED: {
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        }
        case M2_ABSOLUTE_INDEXED: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        }
        default: return false;
    }
    switch (static_cast<Operation0>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
        case BIT: {
            NES_Address operand = bus.read(location);
            flags.bits.Z = !(register_A & operand);
            flags.bits.V = operand & 0x40;
            flags.bits.N = operand & 0x80;
            break;
        }
        case STY: {
            bus.write(location, register_Y);
            break;
        }
        case LDY: {
            register_Y = bus.read(location);
            set_ZN(register_Y);
            break;
        }
        case CPY: {
            NES_Address diff = register_Y - bus.read(location);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
            break;
        }
        case CPX: {
            NES_Address diff = register_X - bus.read(location);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::type1(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 0x1)
        return false;
    // Location of the operand, could be in RAM
    NES_Address location = 0;
    auto op = static_cast<Operation1>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
    switch (static_cast<AddrMode1>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case M1_INDEXED_INDIRECT_X: {
            NES_Byte zero_address = register_X + bus.read(register_PC++);
            // Addresses wrap in zero page mode, thus pass through a mask
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            break;
        }
        case M1_ZERO_PAGE: {
            location = bus.read(register_PC++);
            break;
        }
        case M1_IMMEDIATE: {
            location = register_PC++;
            break;
        }
        case M1_ABSOLUTE: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        }
        case M1_INDIRECT_Y: {
            NES_Byte zero_address = bus.read(register_PC++);
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            if (op != STA)
                set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        }
        case M1_INDEXED_X: {
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        }
        case M1_ABSOLUTE_Y: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (op != STA)
                set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        }
        case M1_ABSOLUTE_X: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (op != STA)
                set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        }
        default: return false;
    }

    switch (op) {
        case ORA: {
            register_A |= bus.read(location);
            set_ZN(register_A);
            break;
        }
        case AND: {
            register_A &= bus.read(location);
            set_ZN(register_A);
            break;
        }
        case EOR: {
            register_A ^= bus.read(location);
            set_ZN(register_A);
            break;
        }
        case ADC: {
            NES_Byte operand = bus.read(location);
            NES_Address sum = register_A + operand + flags.bits.C;
            //Carry forward or UNSIGNED overflow
            flags.bits.C = sum & 0x100;
            //SIGNED overflow, would only happen if the sign of sum is
            //different from BOTH the operands
            flags.bits.V = (register_A ^ sum) & (operand ^ sum) & 0x80;
            register_A = static_cast<NES_Byte>(sum);
            set_ZN(register_A);
            break;
        }
        case STA: {
            bus.write(location, register_A);
            break;
        }
        case LDA: {
            register_A = bus.read(location);
            set_ZN(register_A);
            break;
        }
        case CMP: {
            NES_Address diff = register_A - bus.read(location);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
            break;
        }
        case SBC: {
            //High carry means "no borrow", thus negate and subtract
            NES_Address subtrahend = bus.read(location),
                     diff = register_A - subtrahend - !flags.bits.C;
            //if the ninth bit is 1, the resulting number is negative => borrow => low carry
            flags.bits.C = !(diff & 0x100);
            //Same as ADC, except instead of the subtrahend,
            //substitute with it's one complement
            flags.bits.V = (register_A ^ diff) & (~subtrahend ^ diff) & 0x80;
            register_A = diff;
            set_ZN(diff);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::type2(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 2)
        return false;

    NES_Address location = 0;
    auto op = static_cast<Operation2>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
    auto address_mode = static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
    switch (address_mode) {
        case M2_IMMEDIATE: {
            location = register_PC++;
            break;
        }
        case M2_ZERO_PAGE: {
            location = bus.read(register_PC++);
            break;
        }
        case M2_ACCUMULATOR: {
            break;
        }
        case M2_ABSOLUTE: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        }
        case M2_INDEXED: {
            location = bus.read(register_PC++);
            NES_Byte index;
            if (op == LDX || op == STX)
                index = register_Y;
            else
                index = register_X;
            //The mask wraps address around zero page
            location = (location + index) & 0xff;
            break;
        }
        case M2_ABSOLUTE_INDEXED: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            NES_Byte index;
            if (op == LDX || op == STX)
                index = register_Y;
            else
                index = register_X;
            set_page_crossed(location, location + index);
            location += index;
            break;
        }
        default: return false;
    }

    NES_Address operand = 0;
    switch (op) {
        case ASL:
        case ROL:
            if (address_mode == M2_ACCUMULATOR) {
                auto prev_C = flags.bits.C;
                flags.bits.C = register_A & 0x80;
                register_A <<= 1;
                //If Rotating, set the bit-0 to the the previous carry
                register_A = register_A | (prev_C && (op == ROL));
                set_ZN(register_A);
            } else {
                auto prev_C = flags.bits.C;
                operand = bus.read(location);
                flags.bits.C = operand & 0x80;
                operand = operand << 1 | (prev_C && (op == ROL));
                set_ZN(operand);
                bus.write(location, operand);
            }
            break;
        case LSR:
        case ROR:
            if (address_mode == M2_ACCUMULATOR) {
                auto prev_C = flags.bits.C;
                flags.bits.C = register_A & 1;
                register_A >>= 1;
                //If Rotating, set the bit-7 to the previous carry
                register_A = register_A | (prev_C && (op == ROR)) << 7;
                set_ZN(register_A);
            } else {
                auto prev_C = flags.bits.C;
                operand = bus.read(location);
                flags.bits.C = operand & 1;
                operand = operand >> 1 | (prev_C && (op == ROR)) << 7;
                set_ZN(operand);
                bus.write(location, operand);
            }
            break;
        case STX: {
            bus.write(location, register_X);
            break;
        }
        case LDX: {
            register_X = bus.read(location);
            set_ZN(register_X);
            break;
        }
        case DEC: {
            auto tmp = bus.read(location) - 1;
            set_ZN(tmp);
            bus.write(location, tmp);
            break;
        }
        case INC: {
            auto tmp = bus.read(location) + 1;
            set_ZN(tmp);
            bus.write(location, tmp);
            break;
        }
        default: return false;
    }
    return true;
}

void CPU::reset(NES_Address start_address) {
    register_PC = start_address;
    register_SP = 0xfd;
    register_A = 0;
    register_X = 0;
    register_Y = 0;
    flags.byte = 0b00110100;
    skip_cycles = 0;
    cycles = 0;
}

void CPU::interrupt(MainBus &bus, InterruptType type) {
    if (flags.bits.I && type != NMI_INTERRUPT && type != BRK_INTERRUPT)
        return;
    // Add one if BRK, a quirk of 6502
    if (type == BRK_INTERRUPT)
        ++register_PC;
    // push values on to the stack
    push_stack(bus, register_PC >> 8);
    push_stack(bus, register_PC);
    push_stack(bus, flags.byte | 0b00100000 | (type == BRK_INTERRUPT) << 4);
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

void CPU::cycle(MainBus &bus) {
    // increment the number of cycles
    ++cycles;
    // if in a skip cycle, return
    if (skip_cycles-- > 1)
        return;
    // reset the number of skip cycles to 0
    skip_cycles = 0;
    // read the opcode from the bus and lookup the number of cycles
    NES_Byte op = bus.read(register_PC++);
    // Using short-circuit evaluation, call the other function only if the
    // first failed. ExecuteImplied must be called first and ExecuteBranch
    // must be before ExecuteType0
    if (decode_execute(op, bus) || branch(bus, op) || type1(bus, op) || type2(bus, op) || type0(bus, op))
        skip_cycles += OPERATION_CYCLES[op];
    else
        LOG(Error) << "failed to execute opcode: " << std::hex << +op << std::endl;
}

}  // namespace NES

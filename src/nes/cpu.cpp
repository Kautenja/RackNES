//  Program:      nes-py
//  File:         cpu.cpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "cpu.hpp"
#include "log.hpp"

namespace NES {

bool CPU::type1(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 0x1)
        return false;
    // Location of the operand, could be in RAM
    auto op = static_cast<Operation1>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
    NES_Address address = type1_address(bus, opcode, op == Operation1::STA);
    switch (op) {
        case Operation1::ORA: {
            register_A |= bus.read(address);
            set_ZN(register_A);
            break;
        }
        case Operation1::AND: {
            register_A &= bus.read(address);
            set_ZN(register_A);
            break;
        }
        case Operation1::EOR: {
            register_A ^= bus.read(address);
            set_ZN(register_A);
            break;
        }
        case Operation1::ADC: {
            NES_Byte operand = bus.read(address);
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
        case Operation1::STA: {
            bus.write(address, register_A);
            break;
        }
        case Operation1::LDA: {
            register_A = bus.read(address);
            set_ZN(register_A);
            break;
        }
        case Operation1::CMP: {
            NES_Address diff = register_A - bus.read(address);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
            break;
        }
        case Operation1::SBC: {
            //High carry means "no borrow", thus negate and subtract
            NES_Address subtrahend = bus.read(address),
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

    auto op = static_cast<Operation2>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
    NES_Address address = type2_address(bus, opcode, op == Operation2::LDX || op == Operation2::STX);
    auto address_mode = static_cast<AddressMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
    NES_Address operand = 0;
    switch (op) {
        case Operation2::ASL:
        case Operation2::ROL:
            if (address_mode == AddressMode2::Accumulator) {
                auto prev_C = flags.bits.C;
                flags.bits.C = register_A & 0x80;
                register_A <<= 1;
                //If Rotating, set the bit-0 to the the previous carry
                register_A = register_A | (prev_C && (op == Operation2::ROL));
                set_ZN(register_A);
            } else {
                auto prev_C = flags.bits.C;
                operand = bus.read(address);
                flags.bits.C = operand & 0x80;
                operand = operand << 1 | (prev_C && (op == Operation2::ROL));
                set_ZN(operand);
                bus.write(address, operand);
            }
            break;
        case Operation2::LSR:
        case Operation2::ROR:
            if (address_mode == AddressMode2::Accumulator) {
                auto prev_C = flags.bits.C;
                flags.bits.C = register_A & 1;
                register_A >>= 1;
                //If Rotating, set the bit-7 to the previous carry
                register_A = register_A | (prev_C && (op == Operation2::ROR)) << 7;
                set_ZN(register_A);
            } else {
                auto prev_C = flags.bits.C;
                operand = bus.read(address);
                flags.bits.C = operand & 1;
                operand = operand >> 1 | (prev_C && (op == Operation2::ROR)) << 7;
                set_ZN(operand);
                bus.write(address, operand);
            }
            break;
        case Operation2::STX: {
            bus.write(address, register_X);
            break;
        }
        case Operation2::LDX: {
            register_X = bus.read(address);
            set_ZN(register_X);
            break;
        }
        case Operation2::DEC: {
            auto tmp = bus.read(address) - 1;
            set_ZN(tmp);
            bus.write(address, tmp);
            break;
        }
        case Operation2::INC: {
            auto tmp = bus.read(address) + 1;
            set_ZN(tmp);
            bus.write(address, tmp);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::type0(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 0x0)
        return false;

    NES_Address address = type0_address(bus, opcode);
    switch (static_cast<Operation0>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
        case Operation0::BIT: {
            NES_Address operand = bus.read(address);
            flags.bits.Z = !(register_A & operand);
            flags.bits.V = operand & 0x40;
            flags.bits.N = operand & 0x80;
            break;
        }
        case Operation0::STY: {
            bus.write(address, register_Y);
            break;
        }
        case Operation0::LDY: {
            register_Y = bus.read(address);
            set_ZN(register_Y);
            break;
        }
        case Operation0::CPY: {
            NES_Address diff = register_Y - bus.read(address);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
            break;
        }
        case Operation0::CPX: {
            NES_Address diff = register_X - bus.read(address);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::decode_execute(MainBus &bus, NES_Byte opcode) {
    // NES_Byte address_mode = (opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT;
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
    case OpcodeTable::BPL: {  // branch on result plus
        branch(bus, opcode);
        break;
    }
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
    case OpcodeTable::BMI: {  // branch on result minus
        branch(bus, opcode);
        break;
    }
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
    case OpcodeTable::BVC: {  // branch on overflow clear
        branch(bus, opcode);
        break;
    }
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
        NES_Address address = read_address(bus, register_PC);
        // 6502 has a bug such that the when the vector of an indirect
        // address begins at the last byte of a page, the second byte
        // is fetched from the beginning of that page rather than the
        // beginning of the next
        // Recreating here:
        NES_Address Page = address & 0xff00;
        register_PC = bus.read(address) | bus.read(Page | ((address + 1) & 0xff)) << 8;
        break;
    }
    // case OpcodeTable::ADC__ABSOLUTE: {
    //     break;
    // }
    // case OpcodeTable::ROR__ABSOLUTE: {
    //     break;
    // }
    case OpcodeTable::BVS: {  // branch on overflow set
        branch(bus, opcode);
        break;
    }
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
    case OpcodeTable::BCC: {  // branch on carry clear
        branch(bus, opcode);
        break;
    }
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
    case OpcodeTable::BCS: {  // branch on carry set
        branch(bus, opcode);
        break;
    }
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
    case OpcodeTable::BNE: {  // branch on result not zero
        branch(bus, opcode);
        break;
    }
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
    case OpcodeTable::BEQ: {  // branch on result zero
        branch(bus, opcode);
        break;
    }
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

}  // namespace NES

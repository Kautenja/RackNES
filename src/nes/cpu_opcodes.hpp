//  Program:      nes-py
//  File:         cpu_opcodes.hpp
//  Description:  This file defines relevant CPU opcodes
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_CPU_OPCODES_HPP
#define NES_CPU_OPCODES_HPP

#include "common.hpp"

namespace NES {

const auto INSTRUCTION_MODE_MASK = 0x3;

const auto OPERATION_MASK = 0xe0;
const auto OPERATION_SHIFT = 5;

const auto ADRESS_MODE_MASK = 0x1c;
const auto ADDRESS_MODE_SHIFT = 2;

const auto BRANCH_INSTRUCTION_MASK = 0x1f;
const auto BRANCH_INSTRUCTION_MASK_RESULT = 0x10;
const auto BRANCH_CONDITION_MASK = 0x20;
const auto BRANCH_ON_FLAG_SHIFT = 6;

const auto NMI_VECTOR = 0xfffa;
const auto RESET_VECTOR = 0xfffc;
const auto IRQ_VECTOR = 0xfffe;

enum BranchOnFlag {
    NEGATIVE_,
    OVERFLOW_,
    CARRY_,
    ZERO_,
};

enum Operation1 {
    ORA,
    AND,
    EOR,
    ADC,
    STA,
    LDA,
    CMP,
    SBC,
};

enum AddrMode1 {
    M1_INDEXED_INDIRECT_X,
    M1_ZERO_PAGE,
    M1_IMMEDIATE,
    M1_ABSOLUTE,
    M1_INDIRECT_Y,
    M1_INDEXED_X,
    M1_ABSOLUTE_Y,
    M1_ABSOLUTE_X,
};

enum Operation2 {
    ASL,
    ROL,
    LSR,
    ROR,
    STX,
    LDX,
    DEC,
    INC,
};

enum AddrMode2 {
    M2_IMMEDIATE,
    M2_ZERO_PAGE,
    M2_ACCUMULATOR,
    M2_ABSOLUTE,
    M2_INDEXED          = 5,
    M2_ABSOLUTE_INDEXED = 7,
};

enum Operation0 {
    BIT  = 1,
    STY  = 4,
    LDY,
    CPY,
    CPX,
};

/// Implied mode opcodes
enum OperationImplied {
    BRK  = 0x00,
    PHP  = 0x08,
    CLC  = 0x18,
    JSR  = 0x20,
    PLP  = 0x28,
    SEC  = 0x38,
    RTI  = 0x40,
    PHA  = 0x48,
    JMP  = 0x4C,
    CLI  = 0x58,
    RTS  = 0x60,
    PLA  = 0x68,
    JMPI = 0x6C,  // JMP indirect
    SEI  = 0x78,
    DEY  = 0x88,
    TXA  = 0x8a,
    TYA  = 0x98,
    TXS  = 0x9a,
    TAY  = 0xa8,
    TAX  = 0xaa,
    CLV  = 0xb8,
    TSX  = 0xba,
    INY  = 0xc8,
    DEX  = 0xca,
    CLD  = 0xd8,
    INX  = 0xe8,
    NOP  = 0xea,
    SED  = 0xf8,
};

/// The opcodes of the MOS6502 CPU used in the Nintendo Entertainment System.
/// Opcodes are sorted in increasing order of code. Enums are coded as the
/// three character opcode followed by an optional `__<mode>` symbol indicating
/// the mode of the operation as one of:
///
/// | Mode          | Symbol        |
/// |:--------------|:--------------|
/// | Absolute      | __ABSOLUTE    |
/// | Absolute, X   | __ABSOLUTE_X  |
/// | Absolute, Y   | __ABSOLUTE_Y  |
/// | Accumulator   | __ACCUMULATOR |
/// | Immediate     | __IMMEDIATE   |
/// | Indirect      | __INDIRECT    |
/// | (Indirect, X) | __INDIRECT_X  |
/// | (Indirect), Y | __INDIRECT_Y  |
/// | Zero Page     | __ZERO_PAGE   |
/// | Zero Page, X  | __ZERO_PAGE_X |
/// | Zero Page, Y  | __ZERO_PAGE_Y |
///
enum class OpcodeTable: uint8_t {
    BRK               = 0x00,
    ORA__INDIRECT_X   = 0x01,
    ORA__ZERO_PAGE    = 0x05,
    ASL__ZERO_PAGE    = 0x06,
    PHP               = 0x08,
    ORA__IMMEDIATE    = 0x09,
    ASL__ACCUMULATOR  = 0x0A,
    ORA__ABSOLUTE     = 0x0D,
    ASL__ABSOLUTE     = 0x0E,
    BPL               = 0x10,
    ORA__INDIRECT_Y   = 0x11,
    ORA__ZERO_PAGE_X  = 0x15,
    ASL__ZERO_PAGE_X  = 0x16,
    CLC               = 0x18,
    ORA__ABSOLUTE_Y   = 0x19,
    ORA__ABSOLUTE_X   = 0x1D,
    ASL__ABSOLUTE_X   = 0x1E,
    JSR               = 0x20,
    AND__INDIRECT_X   = 0x21,
    BIT__ZERO_PAGE    = 0x24,
    AND__ZERO_PAGE    = 0x25,
    ROL__ZERO_PAGE    = 0x26,
    PLP               = 0x28,
    AND__IMMEDIATE    = 0x29,
    ROL__ACCUMULATOR  = 0x2A,
    BIT__ABSOLUTE     = 0x2C,
    AND__ABSOLUTE     = 0x2D,
    ROL__ABSOLUTE     = 0x2E,
    BMI               = 0x30,
    AND__INDIRECT_Y   = 0x31,
    AND__ZERO_PAGE_X  = 0x35,
    ROL__ZERO_PAGE_X  = 0x36,
    SEC               = 0x38,
    AND__ABSOLUTE_Y   = 0x39,
    AND__ABSOLUTE_X   = 0x3D,
    ROL__ABSOLUTE_X   = 0x3E,
    RTI               = 0x40,
    EOR__INDIRECT_X   = 0x41,
    EOR__ZERO_PAGE    = 0x45,
    LSR__ZERO_PAGE    = 0x46,
    PHA               = 0x48,
    EOR__IMMEDIATE    = 0x49,
    LSR__ACCUMULATOR  = 0x4A,
    JMP__ABSOLUTE     = 0x4C,
    EOR__ABSOLUTE     = 0x4D,
    LSR__ABSOLUTE     = 0x4E,
    BVC               = 0x50,
    EOR__INDIRECT_Y   = 0x51,
    EOR__ZERO_PAGE_X  = 0x55,
    LSR__ZERO_PAGE_X  = 0x56,
    CLI               = 0x58,
    EOR__ABSOLUTE_Y   = 0x59,
    EOR__ABSOLUTE_X   = 0x5D,
    LSR__ABSOLUTE_X   = 0x5E,
    RTS               = 0x60,
    ADC__INDIRECT_X   = 0x61,
    ADC__ZERO_PAGE    = 0x65,
    ROR__ZERO_PAGE    = 0x66,
    PLA               = 0x68,
    ADC__IMMEDIATE    = 0x69,
    ROR__ACCUMULATOR  = 0x6A,
    JMP__INDIRECT     = 0x6C,
    ADC__ABSOLUTE     = 0x6D,
    ROR__ABSOLUTE     = 0x6E,
    BVS               = 0x70,
    ADC__INDIRECT_Y   = 0x71,
    ADC__ZERO_PAGE_X  = 0x75,
    ROR__ZERO_PAGE_X  = 0x76,
    SEI               = 0x78,
    ADC__ABSOLUTE_Y   = 0x79,
    ADC__ABSOLUTE_X   = 0x7D,
    ROR__ABSOLUTE_X   = 0x7E,
    STA__INDIRECT_X   = 0x81,
    STY__ZERO_PAGE    = 0x84,
    STA__ZERO_PAGE    = 0x85,
    STX__ZERO_PAGE    = 0x86,
    DEY               = 0x88,
    TXA               = 0x8A,
    STY__ABSOLUTE     = 0x8C,
    STA__ABSOLUTE     = 0x8D,
    STX__ABSOLUTE     = 0x8E,
    BCC               = 0x90,
    STA__INDIRECT_Y   = 0x91,
    STY__ZERO_PAGE_X  = 0x94,
    STA__ZERO_PAGE_X  = 0x95,
    STX__ZERO_PAGE_Y  = 0x96,
    TYA               = 0x98,
    STA__ABSOLUTE_Y   = 0x99,
    TXS               = 0x9A,
    STA__ABSOLUTE_X   = 0x9D,
    LDY__IMMEDIATE    = 0xA0,
    LDA__INDIRECT_X   = 0xA1,
    LDX__IMMEDIATE    = 0xA2,
    LDY__ZERO_PAGE    = 0xA4,
    LDA__ZERO_PAGE    = 0xA5,
    LDX__ZERO_PAGE    = 0xA6,
    TAY               = 0xA8,
    LDA__IMMEDIATE    = 0xA9,
    TAX               = 0xAA,
    LDY__ABSOLUTE     = 0xAC,
    LDA__ABSOLUTE     = 0xAD,
    LDX__ABSOLUTE     = 0xAE,
    BCS               = 0xB0,
    LDA__INDIRECT_Y   = 0xB1,
    LDY__ZERO_PAGE_X  = 0xB4,
    LDA__ZERO_PAGE_X  = 0xB5,
    LDX__ZERO_PAGE_Y  = 0xB6,
    CLV               = 0xB8,
    LDA__ABSOLUTE_Y   = 0xB9,
    TSX               = 0xBA,
    LDY__ABSOLUTE_X   = 0xBC,
    LDA__ABSOLUTE_X   = 0xBD,
    LDX__ABSOLUTE_Y   = 0xBE,
    CPY__IMMEDIATE    = 0xC0,
    CMP__INDIRECT_X   = 0xC1,
    CPY__ZERO_PAGE    = 0xC4,
    CMP__ZERO_PAGE    = 0xC5,
    DEC__ZERO_PAGE    = 0xC6,
    INY               = 0xC8,
    CMP__IMMEDIATE    = 0xC9,
    DEX               = 0xCA,
    CPY__ABSOLUTE     = 0xCC,
    CMP__ABSOLUTE     = 0xCD,
    DEC__ABSOLUTE     = 0xCE,
    BNE               = 0xD0,
    CMP__INDIRECT_Y   = 0xD1,
    CMP__ZERO_PAGE_X  = 0xD5,
    DEC__ZERO_PAGE_X  = 0xD6,
    CLD               = 0xD8,
    CMP__ABSOLUTE_Y   = 0xD9,
    CMP__ABSOLUTE_X   = 0xDD,
    DEC__ABSOLUTE_X   = 0xDE,
    CPX__IMMEDIATE    = 0xE0,
    SBC__INDIRECT_X   = 0xE1,
    CPX__ZERO_PAGE    = 0xE4,
    SBC__ZERO_PAGE    = 0xE5,
    INC__ZERO_PAGE    = 0xE6,
    INX               = 0xE8,
    SBC__IMMEDIATE    = 0xE9,
    NOP               = 0xEA,
    CPX__ABSOLUTE     = 0xEC,
    SBC__ABSOLUTE     = 0xED,
    INC__ABSOLUTE     = 0xEE,
    BEQ               = 0xF0,
    SBC__INDIRECT_Y   = 0xF1,
    SBC__ZERO_PAGE_X  = 0xF5,
    INC__ZERO_PAGE_X  = 0xF6,
    SED               = 0xF8,
    SBC__ABSOLUTE_Y   = 0xF9,
    SBC__ABSOLUTE_X   = 0xFD,
    INC__ABSOLUTE_X   = 0xFE,
};

/// A structure for working with the flags register
typedef union {
    struct {
        bool N : 1,
             V : 1,
             ONE : 1,
             B : 1,
             D : 1,
             I : 1,
             Z : 1,
             C : 1;
    } bits;
    NES_Byte byte;
} CPU_Flags;

/// a mapping of opcodes to the number of cycles used by the opcode.
const NES_Byte OPERATION_CYCLES[0x100] = {
//  0 1 2 3 4 5 6 7 8 9 A B C D E F
    0,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,// 0
    3,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,// 1
    6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,// 2
    3,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,// 3
    6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,// 4
    3,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,// 5
    6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,// 6
    3,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,// 7
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,// 8
    3,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,// 9
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,// A
    3,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,// B
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,// C
    3,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,// D
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,// E
    3,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7 // F
};

}  // namespace NES

#endif  // NES_CPU_OPCODES_HPP

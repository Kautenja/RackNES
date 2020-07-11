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

/// The opcodes on the MOS6502 (NES)
enum class OpcodeTable: uint8_t {
    ORA__INDIRECT_X   = 0x01,
    ORA__ZERO_PAGE    = 0x05,
    ASL__ZERO_PAGE    = 0x06,
    ORA__IMMEDIATE    = 0x09,
    ASL__ACCUMULATOR  = 0x0A,
    ORA__ABSOLUTE     = 0x0D,
    ASL__ABSOLUTE     = 0x0E,
    ORA__INDIRECT_Y   = 0x11,
    ORA__ZERO_PAGE_X  = 0x15,
    ASL__ZERO_PAGE_X  = 0x16,
    ORA__ABSOLUTE_Y   = 0x19,
    ORA__ABSOLUTE_X   = 0x1D,
    ASL__ABSOLUTE_X   = 0x1E,
    AND__INDIRECT_X   = 0x21,
    BIT__ZERO_PAGE    = 0x24,
    AND__ZERO_PAGE    = 0x25,
    ROL__ZERO_PAGE    = 0x26,
    AND__IMMEDIATE    = 0x29,
    ROL__ACCUMULATOR  = 0x2A,
    BIT__ABSOLUTE     = 0x2C,
    AND__ABSOLUTE     = 0x2D,
    ROL__ABSOLUTE     = 0x2E,
    AND__INDIRECT_Y   = 0x31,
    AND__ZERO_PAGE_X  = 0x35,
    ROL__ZERO_PAGE_X  = 0x36,
    AND__ABSOLUTE_Y   = 0x39,
    AND__ABSOLUTE_X   = 0x3D,
    ROL__ABSOLUTE_X   = 0x3E,
    EOR__INDIRECT_X   = 0x41,
    EOR__ZERO_PAGE    = 0x45,
    LSR__ZERO_PAGE    = 0x46,
    EOR__IMMEDIATE    = 0x49,
    LSR__ACCUMULATOR  = 0x4A,
    JMP__ABSOLUTE     = 0x4C,
    EOR__ABSOLUTE     = 0x4D,
    LSR__ABSOLUTE     = 0x4E,
    EOR__INDIRECT_Y   = 0x51,
    EOR__ZERO_PAGE_X  = 0x55,
    LSR__ZERO_PAGE_X  = 0x56,
    EOR__ABSOLUTE_Y   = 0x59,
    EOR__ABSOLUTE_X   = 0x5D,
    LSR__ABSOLUTE_X   = 0x5E,
    ADC__INDIRECT_X   = 0x61,
    ADC__ZERO_PAGE    = 0x65,
    ROR__ZERO_PAGE    = 0x66,
    ADC__IMMEDIATE    = 0x69,
    ROR__ACCUMULATOR  = 0x6A,
    JMP__INDIRECT     = 0x6C,
    ADC__ABSOLUTE     = 0x6D,
    ROR__ABSOLUTE     = 0x6E,
    ADC__INDIRECT_Y   = 0x71,
    ADC__ZERO_PAGE_X  = 0x75,
    ROR__ZERO_PAGE_X  = 0x76,
    ADC__ABSOLUTE_Y   = 0x79,
    ADC__ABSOLUTE_X   = 0x7D,
    ROR__ABSOLUTE_X   = 0x7E,
    STA__INDIRECT_X   = 0x81,
    STY__ZERO_PAGE    = 0x84,
    STA__ZERO_PAGE    = 0x85,
    STX__ZERO_PAGE    = 0x86,
    STY__ABSOLUTE     = 0x8C,
    STA__ABSOLUTE     = 0x8D,
    STX__ABSOLUTE     = 0x8E,
    STA__INDIRECT_Y   = 0x91,
    STY__ZERO_PAGE_X  = 0x94,
    STA__ZERO_PAGE_X  = 0x95,
    STX__ZERO_PAGE_Y  = 0x96,
    STA__ABSOLUTE_Y   = 0x99,
    STA__ABSOLUTE_X   = 0x9D,
    LDY__IMMEDIATE    = 0xA0,
    LDA__INDIRECT_X   = 0xA1,
    LDX__IMMEDIATE    = 0xA2,
    LDY__ZERO_PAGE    = 0xA4,
    LDA__ZERO_PAGE    = 0xA5,
    LDX__ZERO_PAGE    = 0xA6,
    LDA__IMMEDIATE    = 0xA9,
    LDY__ABSOLUTE     = 0xAC,
    LDA__ABSOLUTE     = 0xAD,
    LDX__ABSOLUTE     = 0xAE,
    LDA__INDIRECT_Y   = 0xB1,
    LDY__ZERO_PAGE_X  = 0xB4,
    LDA__ZERO_PAGE_X  = 0xB5,
    LDX__ZERO_PAGE_Y  = 0xB6,
    LDA__ABSOLUTE_Y   = 0xB9,
    LDY__ABSOLUTE_X   = 0xBC,
    LDA__ABSOLUTE_X   = 0xBD,
    LDX__ABSOLUTE_Y   = 0xBE,
    CPY__IMMEDIATE    = 0xC0,
    CMP__INDIRECT_X   = 0xC1,
    CPY__ZERO_PAGE    = 0xC4,
    CMP__ZERO_PAGE    = 0xC5,
    DEC__ZERO_PAGE    = 0xC6,
    CMP__IMMEDIATE    = 0xC9,
    CPY__ABSOLUTE     = 0xCC,
    CMP__ABSOLUTE     = 0xCD,
    DEC__ABSOLUTE     = 0xCE,
    CMP__INDIRECT_Y   = 0xD1,
    CMP__ZERO_PAGE_X  = 0xD5,
    DEC__ZERO_PAGE_X  = 0xD6,
    CMP__ABSOLUTE_Y   = 0xD9,
    CMP__ABSOLUTE_X   = 0xDD,
    DEC__ABSOLUTE_X   = 0xDE,
    CPX__IMMEDIATE    = 0xE0,
    SBC__INDIRECT_X   = 0xE1,
    CPX__ZERO_PAGE    = 0xE4,
    SBC__ZERO_PAGE    = 0xE5,
    INC__ZERO_PAGE    = 0xE6,
    SBC__IMMEDIATE    = 0xE9,
    CPX__ABSOLUTE     = 0xEC,
    SBC__ABSOLUTE     = 0xED,
    INC__ABSOLUTE     = 0xEE,
    SBC__INDIRECT_Y   = 0xF1,
    SBC__ZERO_PAGE_X  = 0xF5,
    INC__ZERO_PAGE_X  = 0xF6,
    SBC__ABSOLUTE_Y   = 0xF9,
    SBC__ABSOLUTE_X   = 0xFD,
    INC__ABSOLUTE_X   = 0xFE
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

/// a mapping of opcodes to the number of cycles used by the opcode. 0 implies
/// an unused opcode.
const NES_Byte OPERATION_CYCLES[0x100] = {
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};

}  // namespace NES

#endif  // NES_CPU_OPCODES_HPP

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

static constexpr auto INSTRUCTION_MODE_MASK = 0x3;

static constexpr auto OPERATION_MASK = 0xe0;
static constexpr auto OPERATION_SHIFT = 5;

static constexpr auto ADRESS_MODE_MASK = 0x1c;
static constexpr auto ADDRESS_MODE_SHIFT = 2;

static constexpr auto BRANCH_INSTRUCTION_MASK = 0x1f;
static constexpr auto BRANCH_INSTRUCTION_MASK_RESULT = 0x10;
static constexpr auto BRANCH_CONDITION_MASK = 0x20;
static constexpr auto BRANCH_ON_FLAG_SHIFT = 6;

static constexpr auto NMI_VECTOR = 0xfffa;
static constexpr auto RESET_VECTOR = 0xfffc;
static constexpr auto IRQ_VECTOR = 0xfffe;

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
    BRK = 0x00,
    PHP = 0x08,
    CLC = 0x18,
    JSR = 0x20,
    PLP = 0x28,
    SEC = 0x38,
    RTI = 0x40,
    PHA = 0x48,
    JMP  = 0x4C,
    CLI = 0x58,
    RTS = 0x60,
    PLA = 0x68,
    JMPI = 0x6C,  // JMP indirect
    SEI = 0x78,
    DEY = 0x88,
    TXA = 0x8a,
    TYA = 0x98,
    TXS = 0x9a,
    TAY = 0xa8,
    TAX = 0xaa,
    CLV = 0xb8,
    TSX = 0xba,
    INY = 0xc8,
    DEX = 0xca,
    CLD = 0xd8,
    INX = 0xe8,
    NOP = 0xea,
    SED = 0xf8,
};

/// a mapping of opcodes to the number of cycles used by the opcode.
static constexpr NES_Byte OPERATION_CYCLES[256] = {
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

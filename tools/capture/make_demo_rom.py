#!/usr/bin/env python3
"""Build an original silent NROM-128 title card; Python standard library only.

Copyright 2026 Arhythmetic Units. SPDX-License-Identifier: GPL-3.0-or-later
Original pixel lettering/artwork: CC-BY-NC-ND-4.0; see ../../LICENSE.md.
No external fonts, game code, graphics, or assembler are used.
"""
import argparse
from pathlib import Path
import struct

# Original five-column lettering, seven rows per glyph.
GLYPHS = {
    'A': ['01110','10001','10001','11111','10001','10001','10001'],
    'C': ['01111','10000','10000','10000','10000','10000','01111'],
    'D': ['11110','10001','10001','10001','10001','10001','11110'],
    'E': ['11111','10000','10000','11110','10000','10000','11111'],
    'G': ['01111','10000','10000','10111','10001','10001','01110'],
    'H': ['10001','10001','10001','11111','10001','10001','10001'],
    'I': ['11111','00100','00100','00100','00100','00100','11111'],
    'K': ['10001','10010','10100','11000','10100','10010','10001'],
    'L': ['10000','10000','10000','10000','10000','10000','11111'],
    'M': ['10001','11011','10101','10101','10001','10001','10001'],
    'N': ['10001','11001','11001','10101','10011','10011','10001'],
    'O': ['01110','10001','10001','10001','10001','10001','01110'],
    'R': ['11110','10001','10001','11110','10100','10010','10001'],
    'S': ['01111','10000','10000','01110','00001','00001','11110'],
    'T': ['11111','00100','00100','00100','00100','00100','00100'],
    'U': ['10001','10001','10001','10001','10001','10001','01110'],
    'V': ['10001','10001','10001','10001','10001','01010','00100'],
    'Y': ['10001','10001','01010','00100','00100','00100','00100'],
    ' ': ['00000'] * 7,
}


def build_rom():
    pixels = [[0] * 256 for _ in range(240)]

    def rectangle(x, y, w, h, color):
        for row in range(y, y + h):
            pixels[row][x:x + w] = [color] * w

    def title(text, y, scale, color):
        x = (256 - (len(text) * 6 - 1) * scale) // 2
        for character in text:
            for row, bits in enumerate(GLYPHS[character]):
                for col, bit in enumerate(bits):
                    if bit == '1':
                        rectangle(x + col * scale, y + row * scale, scale, scale, color)
            x += 6 * scale

    rectangle(16, 20, 224, 2, 2)
    title('ARHYTHMETIC', 36, 2, 1)
    title('UNITS', 57, 2, 1)
    title('RACKNES', 92, 3, 2)
    # An original stepped pulse motif, rendered by the NES background plane.
    for x in range(32, 224):
        y = 143 if ((x - 32) // 24) % 2 == 0 else 161
        rectangle(x, y, 1, 3, 1)
        if (x - 32) % 24 == 0 and x > 32:
            rectangle(x, 143, 2, 21, 1)
    title('LIVE NES', 186, 1, 1)
    title('VOLTAGE CONTROL', 203, 1, 3)
    rectangle(16, 220, 224, 2, 2)

    tiles, nametable = [], bytearray()
    for ty in range(30):
        for tx in range(32):
            tile = bytes(sum(((pixels[ty * 8 + y][tx * 8 + x] >> plane) & 1)
                             << (7 - x) for x in range(8))
                         for plane in range(2) for y in range(8))
            if tile not in tiles:
                tiles.append(tile)
            nametable.append(tiles.index(tile))
    if len(tiles) > 256:
        raise ValueError('Title card exceeds one background pattern table')
    nametable.extend(bytes(64))  # All attribute quadrants use palette zero.
    chr_rom = b''.join(tiles).ljust(8192, b'\0')
    code = bytearray()

    def emit(*values):
        code.extend(values)

    def store(address, value):
        emit(0xA9, value, 0x8D, address & 255, address >> 8)  # LDA #; STA abs

    def wait_vblank():
        emit(0x2C, 0x02, 0x20, 0x10, 0xFB)  # BIT $2002; BPL back to BIT

    emit(0x78, 0xD8, 0xA2, 0xFF, 0x9A)  # SEI; CLD; LDX #$FF; TXS
    for address in (0x2000, 0x2001, 0x4010, 0x4015):
        store(address, 0)
    store(0x4017, 0x40)  # Disable APU frame IRQ.
    emit(0x2C, 0x02, 0x20)  # Clear any initial vblank before two warmup frames.
    wait_vblank()
    wait_vblank()
    store(0x2006, 0x20)
    store(0x2006, 0)
    for value in nametable:
        store(0x2007, value)
    store(0x2006, 0x3F)
    store(0x2006, 0)
    for value in [0x0F, 0x30, 0x16, 0x00] * 8:
        store(0x2007, value)
    # Hide all sprites; enable both planes for this emulator's scroll reload.
    store(0x2003, 0)
    for _ in range(256):
        store(0x2004, 0xFF)
    store(0x2000, 0)
    store(0x2005, 0)
    store(0x2005, 0)
    store(0x0010, 0)  # Frame heartbeat checked by the capture harness.
    store(0x0011, 0xA7)  # Initialization marker.
    store(0x2001, 0x1E)  # Both planes enabled, including leftmost column.
    loop = 0x8000 + len(code)
    wait_vblank()
    emit(0xE6, 0x10)  # INC $10 once per vblank.
    emit(0x4C, loop & 255, loop >> 8)  # JMP loop
    handler = 0x8000 + len(code)
    emit(0x40)  # RTI for unused NMI/IRQ vectors.
    if len(code) > 16378:
        raise ValueError('Program overlaps interrupt vectors')
    prg = code.ljust(16378, b'\xEA') + struct.pack('<HHH', handler, 0x8000, handler)
    return b'NES\x1a\x01\x01' + bytes(10) + prg + chr_rom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    rom = build_rom()
    args.destination.write_bytes(rom)
    print(f'Wrote {args.destination}: {len(rom)} bytes, mapper 0, 16 KiB PRG / 8 KiB CHR')


if __name__ == '__main__':
    main()

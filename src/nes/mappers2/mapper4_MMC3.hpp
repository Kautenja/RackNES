// iNES mapper 4.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef MAPPERS2_MAPPER4_HPP
#define MAPPERS2_MAPPER4_HPP

#include "../rom.hpp"

namespace NES {

class Mapper4 final : public ROM::BaseMapper {
 public:
  void reset() override {
    regs.fill(0);
    reg_8000 = 0;

    horizontal_mirroring = true;
    irq_enabled          = false;
    irq_period           = 0;
    irq_counter          = 0;

    set_prg_map<8>(3, -1);
    apply();
  }

  void writePRG(uint16_t addr, uint8_t value) override {
    if (addr < 0x8000) {
      rom.writeRAM(addr - 0x6000, value);
    } else if (addr & 0x8000) {
      switch (addr & 0xE001) {
        case 0x8000: reg_8000 = value; break;
        case 0x8001: regs[reg_8000 & 7] = value; break;
        case 0xA000: horizontal_mirroring = value & 1; break;
        case 0xC000: irq_period = value; break;
        case 0xC001: irq_counter = 0; break;
        case 0xE000:
          // TODO: integrate with CPU
          // CPU::get().set_irq(false);
          irq_enabled = false;
          break;
        case 0xE001: irq_enabled = true; break;
      }

      apply();
    }
  }

  void scanline_counter() override {
    if (irq_counter == 0) {
      irq_counter = irq_period;
    } else {
      --irq_counter;
    }

    if (irq_enabled && irq_counter == 0) {
      // TODO: integrate with CPU
      // CPU::get().set_irq(true);
    }
  }

 private:
  void apply() {
    set_prg_map<8>(1, regs[7]);

    if (!(reg_8000 & (1 << 6))) {
      // PRG Mode 0
      set_prg_map<8>(0, regs[6]);
      set_prg_map<8>(2, -2);
    } else {
      // PRG Mode 1
      set_prg_map<8>(0, -2);
      set_prg_map<8>(2, regs[6]);
    }

    if (!(reg_8000 & (1 << 7))) {
      // CHR Mode 0
      set_chr_map<2>(0, regs[0] >> 1);
      set_chr_map<2>(1, regs[1] >> 1);

      for (size_t i = 0; i < 4; ++i) set_chr_map<1>(4 + i, regs[2 + i]);
    } else {
      // CHR Mode 1
      for (size_t i = 0; i < 4; ++i) set_chr_map<1>(i, regs[2 + i]);

      set_chr_map<2>(2, regs[0] >> 1);
      set_chr_map<2>(3, regs[1] >> 1);
    }

    // TODO: integrate with PPU
    // PPU::get().set_mirroring(horizontal_mirroring ? Horizontal : Vertical);
  }

  std::array<uint8_t, 8> regs     = {};
  uint8_t                reg_8000 = 0;

  bool horizontal_mirroring = false;

  bool    irq_enabled = false;
  uint8_t irq_period  = 0;
  uint8_t irq_counter = 0;
};

}  // namespace NES

#endif  // MAPPERS2_MAPPER4_HPP

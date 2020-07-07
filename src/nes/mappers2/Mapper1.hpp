// iNES mapper 1.
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

#ifndef MAPPERS2_MAPPER1_HPP
#define MAPPERS2_MAPPER1_HPP

#include "../base_mapper.hpp"

namespace NES {

class Mapper1 final : public BaseMapper {
 public:
  void reset() override {
    write_delay = 5;
    shift_reg   = 0;

    control    = 0x0C;
    chr_bank_0 = 0;
    chr_bank_1 = 0;
    prg_bank   = 0;

    apply();
  }

  void writePRG(uint16_t addr, uint8_t value) override {
    if (addr < 0x8000) {
      prg_ram[addr - 0x6000] = value;
    } else if (addr & 0x8000) {
      if (value & 0x80) {  // Reset
        control |= 0x0C;
        write_delay = 5;
        shift_reg   = 0;

        apply();
      } else {
        shift_reg = ((value & 1) << 4) | (shift_reg >> 1);
        --write_delay;

        if (write_delay == 0) {
          switch ((addr >> 13) & 3) {
            case 0: control = shift_reg; break;
            case 1: chr_bank_0 = shift_reg; break;
            case 2: chr_bank_1 = shift_reg; break;
            case 3: prg_bank = shift_reg; break;
          }

          write_delay = 5;
          shift_reg   = 0;

          apply();
        }
      }
    }
  }

 private:
  void apply() {
    if (control & 0b1000) {
      // 16KB PRG-ROM
      if (control & 0b100) {
        // Switchable or fixed to the first bank
        set_prg_map<16>(0, prg_bank & 0xF);
        set_prg_map<16>(1, 0xF);
      } else {
        // Fixed to the last bank or switchable
        set_prg_map<16>(0, 0);
        set_prg_map<16>(1, prg_bank & 0xF);
      }
    } else {
      // 32KB PRG-ROM
      set_prg_map<32>(0, (prg_bank & 0xF) >> 1);
    }

    if (control & 0b10000) {
      // 4KB CHR-ROM
      set_chr_map<4>(0, chr_bank_0);
      set_chr_map<4>(1, chr_bank_1);
    } else {
      // 8KB CHR-ROM
      set_chr_map<8>(0, chr_bank_0 >> 1);
    }

    // TODO: integrate with PPU
    // switch (control & 3) {
    //   case 0: PPU::get().set_mirroring(One_Screen_Low); break;
    //   case 1: PPU::get().set_mirroring(One_Screen_High); break;
    //   case 2: PPU::get().set_mirroring(Vertical); break;
    //   case 3: PPU::get().set_mirroring(Horizontal); break;
    // }
  }

  int     write_delay = 5;
  uint8_t shift_reg   = 0;

  uint8_t control    = 0x0C;
  uint8_t chr_bank_0 = 0;
  uint8_t chr_bank_1 = 0;
  uint8_t prg_bank   = 0;
};

}  // namespace NES

#endif  // MAPPERS2_MAPPER1_HPP

// iNES mapper 2.
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

#ifndef MAPPERS2_MAPPER2_HPP
#define MAPPERS2_MAPPER2_HPP

#include "../base_mapper.hpp"

namespace NES {

class Mapper2 final : public BaseMapper {
 public:
  inline void reset() override {
    set_prg_map<16>(0, 0);
    set_prg_map<16>(1, -1);
    set_chr_map<8>(0, 0);
  }

  inline void writePRG(uint16_t addr, uint8_t value) override {
    if (addr < 0x8000) {
      throw std::runtime_error("Mapper 2 does not have PRG-RAM");
    }
    mode = value;
    apply();
  }

 private:
  inline void apply() { set_prg_map<16>(0, mode); }

  int mode = 0;
};

}  // namespace NES

#endif  // MAPPERS2_MAPPER2_HPP

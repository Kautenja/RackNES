// iNES mapper 0.
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

#ifndef MAPPERS2_MAPPER0_HPP
#define MAPPERS2_MAPPER0_HPP

#include "../BaseMapper.hpp"

class Mapper0 final : public BaseMapper {
 public:
  void reset() override {
    set_prg_map<16>(0, 0);
    set_prg_map<16>(1, 1);
    set_chr_map<8>(0, 0);
  }
};

#endif  // MAPPERS2_MAPPER0_HPP

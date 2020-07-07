// An abstract base for iNES mappers.
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

#ifndef NES_BASE_MAPPER_HPP
#define NES_BASE_MAPPER_HPP

#include <array>
#include <vector>
#include <cstdint>
#include <utility>

namespace NES {

class BaseMapper {
 public:
  BaseMapper() { }
  virtual ~BaseMapper() { }

  virtual void reset() = 0;

  void set_prg_rom(std::vector<uint8_t>&& vec) {
    prg = std::move(vec);
  }

  void set_chr_rom(std::vector<uint8_t>&& vec) {
    chr = std::move(vec);
  }

  void set_prg_ram(std::vector<uint8_t>&& vec) {
    prg_ram = std::move(vec);
  }

  std::vector<uint8_t> get_prg_ram() const {
    return prg_ram;
  }

  uint8_t readPRG(uint16_t addr) const {
    if (addr >= 0x8000) {
      size_t slot     = (addr - 0x8000) / 0x2000;
      size_t prg_addr = (addr - 0x8000) % 0x2000;

      return prg[prg_map[slot] + prg_addr];
    } else {
      return prg_ram[addr - 0x6000];
    }
  }

  inline uint8_t readCHR(uint16_t addr) const {
    size_t slot     = addr / 0x400;
    size_t chr_addr = addr % 0x400;
    return chr[chr_map[slot] + chr_addr];
  }

  virtual inline void writePRG(uint16_t addr, uint8_t value) {
    prg_ram[addr] = value;
  }

  virtual inline void writeCHR(uint16_t addr, uint8_t value) {
    chr[addr] = value;
  }

  template <size_t size>
  void set_prg_map(size_t slot, int page) {
    constexpr size_t pages   = size / 8;
    constexpr size_t pages_b = size * 0x400;  // In bytes

    if (page < 0) {
      page = (static_cast<int>(prg.size()) / pages_b) + page;
    }

    for (size_t i = 0; i < pages; ++i) {
      prg_map[pages * slot + i] = ((pages_b * page) + 0x2000 * i) % prg.size();
    }
  }

  template <size_t size>
  void set_chr_map(size_t slot, int page) {
    constexpr size_t pages   = size;
    constexpr size_t pages_b = size * 0x400;  // In bytes

    for (size_t i = 0; i < size; ++i) {
      chr_map[pages * slot + i] = ((pages_b * page) + 0x400 * i) % chr.size();
    }
  }

  virtual inline void scanline_counter() { }

 protected:
  std::vector<uint8_t> prg;
  std::vector<uint8_t> prg_ram;
  std::vector<uint8_t> chr;

  std::array<std::size_t, 4> prg_map = {};
  std::array<std::size_t, 8> chr_map = {};
};

template void BaseMapper::set_prg_map<32>(size_t, int);
template void BaseMapper::set_prg_map<16>(size_t, int);
template void BaseMapper::set_prg_map<8> (size_t, int);

template void BaseMapper::set_chr_map<8>(size_t, int);
template void BaseMapper::set_chr_map<4>(size_t, int);
template void BaseMapper::set_chr_map<2>(size_t, int);
template void BaseMapper::set_chr_map<1>(size_t, int);

}  // namespace NES

#endif  // NES_BASE_MAPPER_HPP

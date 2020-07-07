
#ifndef MAPPERS2_MAPPER2_HPP
#define MAPPERS2_MAPPER2_HPP

#include "../BaseMapper.hpp"

class Mapper2 final : public BaseMapper {
 public:
  inline void reset() override {
    set_prg_map<16>(0, 0);
    set_prg_map<16>(1, -1);
    set_chr_map<8>(0, 0);
  }

  inline void prg_write(uint16_t addr, uint8_t value) override {
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

#endif  // MAPPERS2_MAPPER2_HPP

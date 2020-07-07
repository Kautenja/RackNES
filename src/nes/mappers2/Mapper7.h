
#ifndef MAPPERS2_MAPPER7_HPP
#define MAPPERS2_MAPPER7_HPP

#include "../BaseMapper.hpp"

class Mapper7 final : public BaseMapper {
 public:
  void reset() override {
    mode = 0;
    set_chr_map<8>(0, 0);
    apply();
  }

  void prg_write(uint16_t addr, uint8_t value) override {
    if (addr < 0x8000) {
      throw std::runtime_error("Mapper 7 does not have PRG-RAM");
    }
    mode = value;
    apply();
  }

 private:
  void apply() {
    set_prg_map<32>(0, mode & 0x0F);
    // TODO: integrate with PPU
    // PPU::get().set_mirroring((mode & 0x10) ? One_Screen_High : One_Screen_Low);
  }

  int mode = 0;
};

#endif  // MAPPERS2_MAPPER7_HPP

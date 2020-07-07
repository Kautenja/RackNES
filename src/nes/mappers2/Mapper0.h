
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

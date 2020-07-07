
#ifndef NES_BASE_MAPPER_HPP
#define NES_BASE_MAPPER_HPP

#include <array>
#include <vector>
#include <cstdint>

class BaseMapper {
 public:
  BaseMapper() { }
  virtual ~BaseMapper() { }

  virtual void reset() = 0;

  void set_prg_rom(std::vector<uint8_t>&&);
  void set_chr_rom(std::vector<uint8_t>&&);

  void set_prg_ram(std::vector<uint8_t>&&);
  std::vector<uint8_t> get_prg_ram() const;

  uint8_t prg_read(uint16_t) const;
  uint8_t chr_read(uint16_t) const;

  virtual void prg_write(uint16_t, uint8_t);
  virtual void chr_write(uint16_t, uint8_t);

  template <size_t> void set_prg_map(size_t, int);
  template <size_t> void set_chr_map(size_t, int);

  virtual void scanline_counter();

 protected:
  std::vector<uint8_t> prg;
  std::vector<uint8_t> prg_ram;
  std::vector<uint8_t> chr;

  std::array<std::size_t, 4> prg_map = {};
  std::array<std::size_t, 8> chr_map = {};
};

#endif  // NES_BASE_MAPPER_HPP

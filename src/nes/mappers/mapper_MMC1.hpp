//  Program:      nes-py
//  File:         mapper_MMC1.hpp
//  Description:  An implementation of the MMC1 mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_MAPPERS_MAPPER_MMC1_HPP
#define NES_MAPPERS_MAPPER_MMC1_HPP

#include <vector>
#include "../mapper.hpp"
#include "../log.hpp"

namespace NES {

/// The MMC1 mapper (mapper #1).
class MapperMMC1 : public Mapper {
 private:
    /// The mirroring callback on the PPU
    std::function<void(void)> mirroring_callback;
    /// the mirroring mode on the device
    NameTableMirroring mirroring;
    /// whether the cartridge uses character RAM
    bool has_character_ram;
    /// the mode for CHR ROM
    int mode_chr;
    /// the mode for PRG ROM
    int mode_prg;
    /// a temporary register
    NES_Byte temp_register;
    /// a write counter
    int write_counter;
    /// the PRG register
    NES_Byte register_prg;
    /// The first CHR register
    NES_Byte register_chr0;
    /// The second CHR register
    NES_Byte register_chr1;
    /// The first PRG bank
    std::size_t first_bank_prg;
    /// The second PRG bank
    std::size_t second_bank_prg;
    /// The first CHR bank
    std::size_t first_bank_chr;
    /// The second CHR bank
    std::size_t second_bank_chr;
    /// The character RAM on the cartridge
    std::vector<NES_Byte> character_ram;

    /// TODO: what does this do
    void calculatePRGPointers() {
        if (mode_prg <= 1) {  // 32KB changeable
            // equivalent to multiplying 0x8000 * (register_prg >> 1)
            first_bank_prg = 0x4000 * (register_prg & ~1);
            // add 16KB
            second_bank_prg = first_bank_prg + 0x4000;
        } else if (mode_prg == 2) {  // fix first switch second
            first_bank_prg = 0;
            second_bank_prg = first_bank_prg + 0x4000 * register_prg;
        } else {  // switch first fix second
            first_bank_prg = 0x4000 * register_prg;
            second_bank_prg = cartridge->getROM().size() - 0x4000;
        }
    }

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    /// @param mirroring_cb the callback to change mirroring modes on the PPU
    ///
    MapperMMC1(Cartridge* cart, std::function<void(void)> mirroring_cb) :
        Mapper(cart),
        mirroring_callback(mirroring_cb),
        mirroring(HORIZONTAL),
        mode_chr(0),
        mode_prg(3),
        temp_register(0),
        write_counter(0),
        register_prg(0),
        register_chr0(0),
        register_chr1(0),
        first_bank_prg(0),
        second_bank_prg(cart->getROM().size() - 0x4000),
        first_bank_chr(0),
        second_bank_chr(0) {
        if (cart->getVROM().size() == 0) {
            has_character_ram = true;
            character_ram.resize(0x2000);
            LOG(Info) << "Uses character RAM" << std::endl;
        } else {
            LOG(Info) << "Using CHR-ROM" << std::endl;
            has_character_ram = false;
            first_bank_chr = 0;
            second_bank_chr = 0x1000 * register_chr1;
        }
    }

    /// Return the name table mirroring mode of this mapper.
    inline NameTableMirroring getNameTableMirroring() override {
        return mirroring;
    }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    inline NES_Byte readPRG(NES_Address address) override {
        if (address < 0xc000)
            return cartridge->getROM()[first_bank_prg + (address & 0x3fff)];
        else
            return cartridge->getROM()[second_bank_prg + (address & 0x3fff)];
    }

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG(NES_Address address, NES_Byte value) override {
        if (!(value & 0x80)) {  // reset bit is NOT set
            temp_register = (temp_register >> 1) | ((value & 1) << 4);
            ++write_counter;

            if (write_counter == 5) {
                if (address <= 0x9fff) {
                    switch (temp_register & 0x3) {
                        case 0: { mirroring = ONE_SCREEN_LOWER;   break; }
                        case 1: { mirroring = ONE_SCREEN_HIGHER;  break; }
                        case 2: { mirroring = VERTICAL;           break; }
                        case 3: { mirroring = HORIZONTAL;         break; }
                    }
                    mirroring_callback();

                    mode_chr = (temp_register & 0x10) >> 4;
                    mode_prg = (temp_register & 0xc) >> 2;
                    calculatePRGPointers();

                    // Recalculate CHR pointers
                    if (mode_chr == 0) {  // one 8KB bank
                        // ignore last bit
                        first_bank_chr = 0x1000 * (register_chr0 | 1);
                        second_bank_chr = first_bank_chr + 0x1000;
                    } else {  // two 4KB banks
                        first_bank_chr = 0x1000 * register_chr0;
                        second_bank_chr = 0x1000 * register_chr1;
                    }
                } else if (address <= 0xbfff) {  // CHR Reg 0
                    register_chr0 = temp_register;
                    // OR 1 if 8KB mode
                    first_bank_chr = 0x1000 * (temp_register | (1 - mode_chr));
                    if (mode_chr == 0)
                        second_bank_chr = first_bank_chr + 0x1000;
                } else if (address <= 0xdfff) {
                    register_chr1 = temp_register;
                    if(mode_chr == 1)
                        second_bank_chr = 0x1000 * temp_register;
                } else {
                    // TODO: PRG-RAM
                    if ((temp_register & 0x10) == 0x10) {
                        LOG(Info) << "PRG-RAM activated" << std::endl;
                    }
                    temp_register &= 0xf;
                    register_prg = temp_register;
                    calculatePRGPointers();
                }

                temp_register = 0;
                write_counter = 0;
            }
        } else {  // reset
            temp_register = 0;
            write_counter = 0;
            mode_prg = 3;
            calculatePRGPointers();
        }
    }

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) override {
        if (has_character_ram)
            return character_ram[address];
        else if (address < 0x1000)
            return cartridge->getVROM()[first_bank_chr + address];
        else
            return cartridge->getVROM()[second_bank_chr + (address & 0xfff)];
    }

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writeCHR(NES_Address address, NES_Byte value) override {
        if (has_character_ram)
            character_ram[address] = value;
        else
            LOG(Info) << "Read-only CHR memory write attempt at " <<
                std::hex << address << std::endl;
    }
};

}  // namespace NES

#endif  // NES_MAPPERS_MAPPER_MMC1_HPP

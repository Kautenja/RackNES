//  Program:      nes-py
//  File:         controller.hpp
//  Description:  This class houses the logic and data for an NES controller
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "common.hpp"

namespace NES {

/// A standard NES controller.
class Controller {
 private:
    /// whether strobe is on
    bool is_strobe = true;
    /// the emulation of the buttons on the controller
    NES_Byte joypad_buttons = 0;
    /// the state of the buttons
    NES_Byte joypad_bits = 0;

 public:
    /// Return a pointer to the joypad buffer.
    inline NES_Byte* get_joypad_buffer() { return &joypad_buttons; }

    /// Write buttons to the virtual controller.
    ///
    /// @param buttons the button bitmap to write to the controller
    ///
    inline void write_buttons(NES_Byte buttons) { joypad_buttons = buttons; }

    /// Strobe the controller.
    inline void strobe(NES_Byte b) {
        is_strobe = (b & 1);
        if (!is_strobe) joypad_bits = joypad_buttons;
    }

    /// Read the controller state.
    ///
    /// @return a state from the controller
    ///
    inline NES_Byte read() {
        NES_Byte ret;
        if (is_strobe) {
            ret = (joypad_buttons & 1);
        } else {
            ret = (joypad_bits & 1);
            joypad_bits >>= 1;
        }
        return ret | 0x40;
    }
};

}  // namespace NES

#endif  // CONTROLLER_HPP

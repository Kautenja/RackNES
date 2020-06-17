//  Program:      nes-py
//  File:         controller.hpp
//  Description:  This class houses the logic and data for an NES controller
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_CONTROLLER_HPP
#define NES_CONTROLLER_HPP

#include <jansson.h>
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

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "is_strobe", json_boolean(is_strobe));
        json_object_set_new(rootJ, "joypad_buttons", json_integer(joypad_buttons));
        json_object_set_new(rootJ, "joypad_bits", json_integer(joypad_bits));
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load is_strobe
        {
            json_t* json_data = json_object_get(rootJ, "is_strobe");
            if (json_data) is_strobe = json_boolean_value(json_data);
        }
        // load joypad_buttons
        {
            json_t* json_data = json_object_get(rootJ, "joypad_buttons");
            if (json_data) joypad_buttons = json_boolean_value(json_data);
        }
        // load joypad_bits
        {
            json_t* json_data = json_object_get(rootJ, "joypad_bits");
            if (json_data) joypad_bits = json_boolean_value(json_data);
        }
    }
};

}  // namespace NES

#endif  // NES_CONTROLLER_HPP

// NES APU snapshot support with JSON serialization.
// Copyright 2020 Christian Kauten
// Nes_Snd_Emu 0.1.7. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.
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

#ifndef APU_SNAPSHOT_H
#define APU_SNAPSHOT_H

#include <string>
#include <cstring>
#include <vector>
#include <jansson.h>
#include "../../base64.h"
#include "blargg_common.h"

struct apu_snapshot_t {
    typedef BOOST::uint8_t byte;

    typedef byte env_t[3];
    /*struct env_t {
        byte delay;
        byte env;3
        byte written;
    };*/

    /// register $4000-$4013
    byte w40xx[0x14];
    /// enables
    byte w4015;
    /// mode
    byte w4017;
    BOOST::uint16_t delay;
    byte step;
    byte irq_flag;

    /// The square oscillator.
    struct square_t {
        BOOST::uint16_t delay;
        env_t env;
        byte length;
        byte phase;
        byte swp_delay;
        byte swp_reset;
        byte unused[1];

        /// Convert the object's state to a JSON object.
        json_t* dataToJson() {
            json_t* rootJ = json_object();
            json_object_set_new(rootJ, "delay", json_integer(delay));
            json_object_set_new(rootJ, "env[0]", json_integer(env[0]));
            json_object_set_new(rootJ, "env[1]", json_integer(env[1]));
            json_object_set_new(rootJ, "env[2]", json_integer(env[2]));
            json_object_set_new(rootJ, "length", json_integer(length));
            json_object_set_new(rootJ, "phase", json_integer(phase));
            json_object_set_new(rootJ, "swp_delay", json_integer(swp_delay));
            json_object_set_new(rootJ, "swp_reset", json_integer(swp_reset));
            return rootJ;
        }

        /// Load the object's state from a JSON object.
        void dataFromJson(json_t* rootJ) {
            // load delay
            {
                json_t* json_data = json_object_get(rootJ, "delay");
                if (json_data) delay = json_integer_value(json_data);
            }
            // load env[0]
            {
                json_t* json_data = json_object_get(rootJ, "env[0]");
                if (json_data) env[0] = json_integer_value(json_data);
            }
            // load env[1]
            {
                json_t* json_data = json_object_get(rootJ, "env[1]");
                if (json_data) env[1] = json_integer_value(json_data);
            }
            // load env[2]
            {
                json_t* json_data = json_object_get(rootJ, "env[2]");
                if (json_data) env[2] = json_integer_value(json_data);
            }
            // load length
            {
                json_t* json_data = json_object_get(rootJ, "length");
                if (json_data) length = json_integer_value(json_data);
            }
            // load phase
            {
                json_t* json_data = json_object_get(rootJ, "phase");
                if (json_data) phase = json_integer_value(json_data);
            }
            // load swp_delay
            {
                json_t* json_data = json_object_get(rootJ, "swp_delay");
                if (json_data) swp_delay = json_integer_value(json_data);
            }
            // load swp_reset
            {
                json_t* json_data = json_object_get(rootJ, "swp_reset");
                if (json_data) swp_reset = json_integer_value(json_data);
            }
        }
    } square1, square2;

    /// The triangle oscillator.
    struct triangle_t {
        BOOST::uint16_t delay;
        byte length;
        byte phase;
        byte linear_counter;
        byte linear_mode;

        /// Convert the object's state to a JSON object.
        json_t* dataToJson() {
            json_t* rootJ = json_object();
            json_object_set_new(rootJ, "delay", json_integer(delay));
            json_object_set_new(rootJ, "length", json_integer(length));
            json_object_set_new(rootJ, "phase", json_integer(phase));
            json_object_set_new(rootJ, "linear_counter", json_integer(linear_counter));
            json_object_set_new(rootJ, "linear_mode", json_integer(linear_mode));
            return rootJ;
        }

        /// Load the object's state from a JSON object.
        void dataFromJson(json_t* rootJ) {
            // load delay
            {
                json_t* json_data = json_object_get(rootJ, "delay");
                if (json_data) delay = json_integer_value(json_data);
            }
            // load length
            {
                json_t* json_data = json_object_get(rootJ, "length");
                if (json_data) length = json_integer_value(json_data);
            }
            // load phase
            {
                json_t* json_data = json_object_get(rootJ, "phase");
                if (json_data) phase = json_integer_value(json_data);
            }
            // load linear_counter
            {
                json_t* json_data = json_object_get(rootJ, "linear_counter");
                if (json_data) linear_counter = json_integer_value(json_data);
            }
            // load linear_mode
            {
                json_t* json_data = json_object_get(rootJ, "linear_mode");
                if (json_data) linear_mode = json_integer_value(json_data);
            }
        }
    } triangle;

    /// The noise oscillator.
    struct noise_t {
        BOOST::uint16_t delay;
        env_t env;
        byte length;
        BOOST::uint16_t shift_reg;

        /// Convert the object's state to a JSON object.
        json_t* dataToJson() {
            json_t* rootJ = json_object();
            json_object_set_new(rootJ, "delay", json_integer(delay));
            json_object_set_new(rootJ, "env[0]", json_integer(env[0]));
            json_object_set_new(rootJ, "env[1]", json_integer(env[1]));
            json_object_set_new(rootJ, "env[2]", json_integer(env[2]));
            json_object_set_new(rootJ, "length", json_integer(length));
            json_object_set_new(rootJ, "shift_reg", json_integer(shift_reg));
            return rootJ;
        }

        /// Load the object's state from a JSON object.
        void dataFromJson(json_t* rootJ) {
            // load delay
            {
                json_t* json_data = json_object_get(rootJ, "delay");
                if (json_data) delay = json_integer_value(json_data);
            }
            // load env[0]
            {
                json_t* json_data = json_object_get(rootJ, "env[0]");
                if (json_data) env[0] = json_integer_value(json_data);
            }
            // load env[1]
            {
                json_t* json_data = json_object_get(rootJ, "env[1]");
                if (json_data) env[1] = json_integer_value(json_data);
            }
            // load env[2]
            {
                json_t* json_data = json_object_get(rootJ, "env[2]");
                if (json_data) env[2] = json_integer_value(json_data);
            }
            // load length
            {
                json_t* json_data = json_object_get(rootJ, "length");
                if (json_data) length = json_integer_value(json_data);
            }
            // load shift_reg
            {
                json_t* json_data = json_object_get(rootJ, "shift_reg");
                if (json_data) shift_reg = json_integer_value(json_data);
            }
        }
    } noise;

    /// The DMC sampler.
    struct dmc_t {
        BOOST::uint16_t delay;
        BOOST::uint16_t remain;
        BOOST::uint16_t addr;
        byte buf;
        byte bits_remain;
        byte bits;
        byte buf_empty;
        byte silence;
        byte irq_flag;

        /// Convert the object's state to a JSON object.
        json_t* dataToJson() {
            json_t* rootJ = json_object();
            json_object_set_new(rootJ, "delay", json_integer(delay));
            json_object_set_new(rootJ, "remain", json_integer(remain));
            json_object_set_new(rootJ, "addr", json_integer(addr));
            json_object_set_new(rootJ, "buf", json_integer(buf));
            json_object_set_new(rootJ, "bits_remain", json_integer(bits_remain));
            json_object_set_new(rootJ, "bits", json_integer(bits));
            json_object_set_new(rootJ, "buf_empty", json_integer(buf_empty));
            json_object_set_new(rootJ, "silence", json_integer(silence));
            json_object_set_new(rootJ, "irq_flag", json_integer(irq_flag));
            return rootJ;
        }

        /// Load the object's state from a JSON object.
        void dataFromJson(json_t* rootJ) {
            // load delay
            {
                json_t* json_data = json_object_get(rootJ, "delay");
                if (json_data) delay = json_integer_value(json_data);
            }
            // load remain
            {
                json_t* json_data = json_object_get(rootJ, "remain");
                if (json_data) remain = json_integer_value(json_data);
            }
            // load addr
            {
                json_t* json_data = json_object_get(rootJ, "addr");
                if (json_data) addr = json_integer_value(json_data);
            }
            // load buf
            {
                json_t* json_data = json_object_get(rootJ, "buf");
                if (json_data) buf = json_integer_value(json_data);
            }
            // load bits_remain
            {
                json_t* json_data = json_object_get(rootJ, "bits_remain");
                if (json_data) bits_remain = json_integer_value(json_data);
            }
            // load bits
            {
                json_t* json_data = json_object_get(rootJ, "bits");
                if (json_data) bits = json_integer_value(json_data);
            }
            // load buf_empty
            {
                json_t* json_data = json_object_get(rootJ, "buf_empty");
                if (json_data) buf_empty = json_integer_value(json_data);
            }
            // load silence
            {
                json_t* json_data = json_object_get(rootJ, "silence");
                if (json_data) silence = json_integer_value(json_data);
            }
            // load irq_flag
            {
                json_t* json_data = json_object_get(rootJ, "irq_flag");
                if (json_data) irq_flag = json_integer_value(json_data);
            }
        }
    } dmc;

    enum { tag = 'APUR' };
    void swap();

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        {
            auto data_string = base64_encode(&w40xx[0], 0x14);
            json_object_set_new(rootJ, "w40xx", json_string(data_string.c_str()));
        }
        json_object_set_new(rootJ, "w4015", json_integer(w4015));
        json_object_set_new(rootJ, "w4017", json_integer(w4017));
        json_object_set_new(rootJ, "delay", json_integer(delay));
        json_object_set_new(rootJ, "step", json_integer(step));
        json_object_set_new(rootJ, "irq_flag", json_integer(irq_flag));
        json_object_set_new(rootJ, "square1", square1.dataToJson());
        json_object_set_new(rootJ, "square2", square2.dataToJson());
        json_object_set_new(rootJ, "triangle", triangle.dataToJson());
        json_object_set_new(rootJ, "noise", noise.dataToJson());
        json_object_set_new(rootJ, "dmc", dmc.dataToJson());
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load w40xx
        {
            json_t* json_data = json_object_get(rootJ, "w40xx");
            if (json_data) {
                std::string data_string = json_string_value(json_data);
                data_string = base64_decode(data_string);
                std::memcpy(&w40xx, data_string.c_str(), data_string.length());
            }
        }
        // load w4015
        {
            json_t* json_data = json_object_get(rootJ, "w4015");
            if (json_data) w4015 = json_integer_value(json_data);
        }
        // load w4017
        {
            json_t* json_data = json_object_get(rootJ, "w4017");
            if (json_data) w4017 = json_integer_value(json_data);
        }
        // load delay
        {
            json_t* json_data = json_object_get(rootJ, "delay");
            if (json_data) delay = json_integer_value(json_data);
        }
        // load step
        {
            json_t* json_data = json_object_get(rootJ, "step");
            if (json_data) step = json_integer_value(json_data);
        }
        // load irq_flag
        {
            json_t* json_data = json_object_get(rootJ, "irq_flag");
            if (json_data) irq_flag = json_integer_value(json_data);
        }
        // load square1
        {
            json_t* json_data = json_object_get(rootJ, "square1");
            if (json_data) square1.dataFromJson(json_data);
        }
        // load square2
        {
            json_t* json_data = json_object_get(rootJ, "square2");
            if (json_data) square2.dataFromJson(json_data);
        }
        // load triangle
        {
            json_t* json_data = json_object_get(rootJ, "triangle");
            if (json_data) triangle.dataFromJson(json_data);
        }
        // load noise
        {
            json_t* json_data = json_object_get(rootJ, "noise");
            if (json_data) noise.dataFromJson(json_data);
        }
        // load dmc
        {
            json_t* json_data = json_object_get(rootJ, "dmc");
            if (json_data) dmc.dataFromJson(json_data);
        }
    }
};
BOOST_STATIC_ASSERT( sizeof (apu_snapshot_t) == 72 );

#endif  // APU_SNAPSHOT_H

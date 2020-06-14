//  Program:      nes-py
//  File:         apu.hpp
//  Description:  This class houses the logic and data for an NES APU
//
//  Copyright (c) 2020 Christian Kauten. All rights reserved.
//

#ifndef APU_HPP
#define APU_HPP

#include "apu/Nes_Apu.h"

namespace NES {

/// The Audio Processing Unit (APU) of the NES.
class APU {
 public:
    /// The NES APU instance to synthesize sound with
    Nes_Apu apu;
    /// The BLIP buffer to render audio samples from
    Blip_Buffer buf;
    /// the number of elapsed frames (set by the emulator)
    int elapsed = 0;

    /// Initialize the APU.
    APU() {
        // set the audio sample rate
        buf.sample_rate(96000);
        // set the clock rate of the processor (NES processor)
        // CYCLES_PER_FRAME * FRAMES_PER_SECOND
        // 29781 * 60
        buf.clock_rate(1789773);
        apu.output(&buf);
    }

    /// Set the sample rate to a new value.
    ///
    /// @param value the frame rate, i.e., 96000Hz
    ///
    inline void set_sample_rate(int value) { buf.sample_rate(value); }

    /// Set the frame-rate to a new value.
    ///
    /// @param value the frame rate, i.e., 60FPS
    ///
    inline void set_frame_rate(float value) { buf.clock_rate(value * 29781); }

    /// Reset the APU.
    inline void reset() { apu.reset(); buf.clear(); }

    /// Read the value from the APU status register.
    inline uint8_t read_status() { return apu.read_status(elapsed); }

    /// Write a value from to APU registers.
    ///
    /// @param addr the address to write to
    /// @oaram value the value to write to the register
    ///
    inline void write(uint16_t addr, uint8_t value) {
        apu.write_register(elapsed, addr, value);
    }

    /// Run a step on the APU.
    inline void step() { apu.end_frame(elapsed); buf.end_frame(elapsed); }

    /// Return a 16-bit signed sample from the APU.
    inline int16_t get_sample() {
        // get a single sample from the BLIP buffer
        static constexpr int OUT_SIZE = 1;
        blip_sample_t outBuf[OUT_SIZE] = {0};
        if (buf.samples_avail() >= OUT_SIZE)
            buf.read_samples(outBuf, OUT_SIZE);
        return outBuf[0];
    }
};

}  // namespace NES

#endif // APU_HPP

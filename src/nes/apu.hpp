//  Program:      nes-py
//  File:         apu.hpp
//  Description:  This class houses the logic and data for an NES APU
//
//  Copyright (c) 2020 Christian Kauten. All rights reserved.
//

#ifndef NES_APU_HPP
#define NES_APU_HPP

#include <jansson.h>
#include "common.hpp"
#include "apu/Nes_Apu.h"
#include "apu/apu_snapshot.h"

namespace NES {

/// The Audio Processing Unit (APU) of the NES.
class APU {
 private:
    /// The BLIP buffer to render audio samples from
    Blip_Buffer buf;
    /// the number of elapsed cycles (set by the emulator)
    int cycles = 0;

 public:
    /// The NES APU instance to synthesize sound with
    Nes_Apu apu;

    /// Initialize the APU.
    APU() {
        // set the audio sample rate
        buf.sample_rate(96000);
        // set the clock rate of the processor (NES processor)
        // CYCLES_PER_FRAME * FRAMES_PER_SECOND
        // 29781 * 60
        buf.clock_rate(1789773);
        apu.output(&buf);
        apu.volume(2.0);
    }

    /// Copy data from another instance of APU.
    void copy_from(const APU &other) {
        cycles = other.cycles;
        apu_snapshot_t snapshot;
        other.apu.save_snapshot(&snapshot);
        apu.load_snapshot(snapshot);
    }

    /// Set the DMC Reader on the APU. The DMC Reader is a callback for reading
    /// audio samples from RAM for DMC playback.
    ///
    /// @param callback the callback function for reading RAM from memory
    ///
    inline void set_dmc_reader(RomReaderCallback callback) {
        apu.dmc_reader(callback);
    }

    /// Set the callback function for IRQ interrupting the CPU.
    ///
    /// @param callback the callback method that interrupts the CPU (IRQ)
    ///
    inline void set_irq_callback(APU_IRQ_InterruptCallback callback) {
        apu.irq_notifier(callback);
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
    inline NES_Byte read_status() { return apu.read_status(cycles); }

    /// Write a value from to APU registers.
    ///
    /// @param addr the address to write to
    /// @oaram value the value to write to the register
    ///
    inline void write(NES_Address addr, NES_Byte value) {
        apu.write_register(cycles, addr, value);
    }

    /// Run a cycle on the APU (increment number of elapsed cycles).
    inline void cycle() { ++cycles; }

    /// Run a step on the APU.
    inline void end_frame() {
        apu.end_frame(cycles);
        buf.end_frame(cycles);
        // reset the number of elapsed cycles back to 0
        cycles = 0;
    }

    /// Return a 16-bit signed sample from the APU.
    inline int16_t get_sample() {
        // get a single sample from the BLIP buffer
        static constexpr int OUT_SIZE = 1;
        blip_sample_t outBuf[OUT_SIZE] = {0};
        if (buf.samples_avail() >= OUT_SIZE)
            buf.read_samples(outBuf, OUT_SIZE);
        return outBuf[0];
    }

    /// Convert the object's state to a JSON object.
    json_t* dataToJson() {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "cycles", json_integer(cycles));
        apu_snapshot_t snapshot;
        apu.save_snapshot(&snapshot);
        json_object_set_new(rootJ, "apu", snapshot.dataToJson());
        return rootJ;
    }

    /// Load the object's state from a JSON object.
    void dataFromJson(json_t* rootJ) {
        // load cycles
        {
            json_t* json_data = json_object_get(rootJ, "cycles");
            if (json_data) cycles = json_integer_value(json_data);
        }
        // load apu
        {
            json_t* json_data = json_object_get(rootJ, "apu");
            if (json_data) {
                apu_snapshot_t snapshot;
                snapshot.dataFromJson(json_data);
                apu.load_snapshot(snapshot);
            }
        }
    }
};

}  // namespace NES

#endif // NES_APU_HPP

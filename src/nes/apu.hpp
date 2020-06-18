//  Program:      nes-py
//  File:         apu.hpp
//  Description:  This class houses the logic and data for an NES APU
//
//  Copyright (c) 2020 Christian Kauten. All rights reserved.
//

#ifndef NES_APU_HPP
#define NES_APU_HPP

#include <vector>
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
    /// The NES APU instance to synthesize sound with
    Nes_Apu apu;
    /// the number of elapsed cycles (set by the emulator)
    int cycles = 0;

 public:
    /// The default sample rate for the APU
    static constexpr uint32_t SAMPLE_RATE = 96000;
    /// The default volume
    static constexpr float VOLUME = 2.f;

    /// Initialize the APU.
    APU() {
        buf.sample_rate(SAMPLE_RATE);
        buf.clock_rate(CLOCK_RATE);
        apu.output(&buf);
        apu.volume(VOLUME);
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

    /// Set the volume to a new value.
    ///
    /// @param value the volume level
    ///
    inline void set_volume(float value = VOLUME) { apu.volume(value); }

    /// Set the sample rate to a new value.
    ///
    /// @param value the frame rate, i.e., 96000 Hz
    ///
    inline void set_sample_rate(uint32_t value = SAMPLE_RATE) {
        buf.sample_rate(value);
    }

    /// Set the clock-rate to a new value.
    ///
    /// @param value the clock rate, i.e., 1789773 CPS
    ///
    inline void set_clock_rate(uint64_t value = CLOCK_RATE) {
        buf.clock_rate(value);
    }

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
        if (buf.samples_avail() == 0) return 0;
        // copy the buffer to  a local vector and return the first sample
        std::vector<int16_t> output_buffer(buf.samples_avail());
        buf.read_samples(&output_buffer[0], buf.samples_avail());
        // usually there will only be one sample, but when the clock rate is
        // at particular values or very fast, the buffer will begin to grow
        // slowly. Because the audio turns to trash anyway, just ignore the
        // overflowing values :)
        return output_buffer[0];
        // TODO: is there a more elegant way of handling this funcation? this
        // buffer is not really necessary within the context of VCV Rack.
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

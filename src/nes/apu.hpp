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
    /// The BLIP buffers to render audio samples from
    Blip_Buffer buffer[Nes_Apu::osc_count];
    /// The NES APU instance to synthesize sound with
    Nes_Apu apu;

 public:
    /// the number of channels on the APU
    static constexpr std::size_t NUM_CHANNELS = Nes_Apu::osc_count;
    /// The default sample rate for the APU
    static constexpr uint32_t SAMPLE_RATE = 96000;

    /// @brief Initialize a new APU.
    APU() {
        for (std::size_t i = 0; i < Nes_Apu::osc_count; i++) {
            buffer[i].sample_rate(SAMPLE_RATE);
            buffer[i].clock_rate(CLOCK_RATE);
            apu.osc_output(i, &buffer[i]);
        }
    }

    /// @brief Copy data from another instance.
    ///
    /// @param other the other instance to copy the data from into this
    ///
    void copy_from(const APU &other) {
        apu_snapshot_t snapshot;
        other.apu.save_snapshot(&snapshot);
        apu.load_snapshot(snapshot);
    }

    /// @brief Set the DMC Reader on the APU. The DMC Reader is a callback for
    /// reading audio samples from RAM for DMC playback.
    ///
    /// @param callback the callback function for reading RAM from memory
    ///
    inline void set_dmc_reader(RomReaderCallback callback) {
        apu.dmc_reader(callback);
    }

    /// @brief Set the callback function for IRQ interrupting the CPU.
    ///
    /// @param callback the callback method that interrupts the CPU (IRQ)
    ///
    inline void set_irq_callback(APU_IRQ_InterruptCallback callback) {
        apu.irq_notifier(callback);
    }

    /// @brief Set the sample rate to a new value.
    ///
    /// @param value the frame rate, i.e., 96000 Hz
    ///
    inline void set_sample_rate(uint32_t value = SAMPLE_RATE) {
        for (std::size_t i = 0; i < Nes_Apu::osc_count; i++)
            buffer[i].sample_rate(value);
    }

    /// @brief Set the clock-rate to a new value.
    ///
    /// @param value the clock rate, i.e., 1789773 CPS
    ///
    inline void set_clock_rate(uint64_t value = CLOCK_RATE) {
        for (std::size_t i = 0; i < Nes_Apu::osc_count; i++)
            buffer[i].clock_rate(value);
    }

    /// @brief Reset the APU.
    inline void reset() {
        apu.reset();
        for (std::size_t i = 0; i < Nes_Apu::osc_count; i++)
            buffer[i].clear();
    }

    /// @brief Read the value from the APU status register.
    ///
    /// @returns the current value of the NES APU status register
    ///
    inline NES_Byte read_status() { return apu.read_status(1); }

    /// @brief Write a value from to APU registers.
    ///
    /// @param addr the address to write to
    /// @oaram value the value to write to the register
    ///
    inline void write(NES_Address addr, NES_Byte value) {
        apu.write_register(1, addr, value);
    }

    /// @brief Run a cycle on the APU (increment number of elapsed cycles).
    inline void cycle() {
        apu.end_frame(1);
        for (std::size_t i = 0; i < Nes_Apu::osc_count; i++)
            buffer[i].end_frame(1);
    }

    /// @brief Return a 16-bit signed sample from the APU.
    ///
    /// @param channel the channel to get a sample from
    /// @returns a 16-bit audio sample for the given channel
    ///
    inline int16_t get_sample(int channel) {
        if (buffer[channel].samples_avail() == 0) return 0;
        // copy the buffer to  a local vector and return the first sample
        std::vector<int16_t> output_buffer(buffer[channel].samples_avail());
        buffer[channel].read_samples(&output_buffer[0], buffer[channel].samples_avail());
        return output_buffer[0];
    }

    /// @brief Convert the object's state to a JSON object.
    ///
    /// @returns a JSON object with the serialized contents of this object
    ///
    json_t* dataToJson() const {
        json_t* rootJ = json_object();
        apu_snapshot_t snapshot;
        apu.save_snapshot(&snapshot);
        json_object_set_new(rootJ, "apu", snapshot.dataToJson());
        return rootJ;
    }

    /// @brief Load the object's state from a JSON object.
    ///
    /// @param rootJ a JSON object with the serialized contents of this object
    ///
    void dataFromJson(json_t* rootJ) {
        json_t* json_data = json_object_get(rootJ, "apu");
        if (json_data) {
            apu_snapshot_t snapshot;
            snapshot.dataFromJson(json_data);
            apu.load_snapshot(snapshot);
        }
    }
};

}  // namespace NES

#endif // NES_APU_HPP

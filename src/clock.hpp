// An oscillator that generates a simplified pulse wave for clock.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
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

#ifndef RACKNES_CLOCK_HPP
#define RACKNES_CLOCK_HPP

#include "rack.hpp"

/// An oscillator that generates a simplified pulse wave for clock.
struct Clock {
 private:
    /// the Min BLEP generator for anti-aliasing the pulse wave
    rack::dsp::MinBlepGenerator<16, 32, float> minBLEP;

    /// the current phase of the clock
    float phase = 0.0;
    /// the current frequency of the clock
    float freq = 120.f;
    /// the current width of the pulse in [0, 1]
    float pulseWidth = 0.5;
    /// the value from the last clock synchronization
    float lastSyncValue = 0.0;
    /// the output value of the clock
    float value = 0.0;

 public:
    /// the smallest pulse width value
    static constexpr float pulseWidthMin = 0.1f;
    /// the largest pulse width value
    static constexpr float pulseWidthMax = 0.9f;
    /// the smallest frequency for the clock
    static constexpr float frequencyMin = 0.f;
    /// the largest frequency for the clock
    static constexpr float frequencyMax = 20000.f;

    /// whether the oscillator is synced to another oscillator
    bool syncEnabled = false;
    /// whether the clock is running
    bool isRunning = true;

    /// Initialize a clock.
    explicit Clock(float pw = 0.5f) : pulseWidth(pw) { }

    /// Set the pitch of the oscillator to a new value.
    ///
    /// @param pitch the new pitch to set the oscillator to
    ///
    inline void setFrequency(float frequency) {
        freq = rack::clamp(frequency, frequencyMin, frequencyMax);
    }

    /// Set the width of the pulse wave to a new value.
    ///
    /// @param pulseWidth_ the new width to set the pulse wave to
    ///
    inline void setPulseWidth(float pulseWidth_) {
        pulseWidth = rack::clamp(pulseWidth_, pulseWidthMin, pulseWidthMax);
    }

    /// Return the pulse width of the oscillator.
    inline float getPulseWidth() const { return pulseWidth; }

    /// Return the phase of the oscillator.
    inline float getPhase() const { return phase; }

    /// Return the phase of the oscillator.
    inline float getFreq() const { return freq; }

    /// Return the value from the oscillator in the range [-1.0, 1.0].
    inline float getValue() const { return isRunning * value; }

    /// Return the value from the oscillator in the range [-10.0, 10.0].
    /// i.e., voltage according to Eurorack standards.
    inline float getVoltage() const { return 10.f * isRunning * value; }

    /// Reset the clock.
    inline void reset() {
        phase = 0.f;
        lastSyncValue = 0.f;
    }

    /// Process a sample for given change in time and sync value.
    ///
    /// @param deltaTime the change in time between samples
    /// @param syncValue the value of the oscillator to sync to
    ///
    void process(float deltaTime, float syncValue = 0) {
        // Advance phase
        float deltaPhase = rack::clamp(freq * deltaTime, 1e-6f, 0.5f);
        phase += deltaPhase;
        // detect half phase
        if ((phase - deltaPhase) < pulseWidth && phase >= pulseWidth) {
            float crossing  = -(phase - pulseWidth) / deltaPhase;
            minBLEP.insertDiscontinuity(crossing, -2.f);
        }
        // Reset phase if at end of cycle
        if (phase >= 1.0) {
            phase -= 1.0;
            float crossing = -phase / deltaPhase;
            minBLEP.insertDiscontinuity(crossing, 2.f);
        }
        // sync the oscillator to the input oscillator
        if (syncEnabled) {
            float deltaSync = syncValue - lastSyncValue;
            float syncCrossing = -lastSyncValue / deltaSync;
            lastSyncValue = syncValue;
            bool sync = (0.f < syncCrossing) && (syncCrossing <= 1.f) && (syncValue >= 0.f);
            float newPhase = sync ? (1.f - syncCrossing) * deltaPhase : phase;
            float p = syncCrossing - 1.f;
            float x0 = (phase < pulseWidth) ? 1.0 : -1.0;
            float x1 = (newPhase < pulseWidth) ? 1.0 : -1.0;
            minBLEP.insertDiscontinuity(p, x1 - x0);
            phase = newPhase;
        }
        // set the value of the oscillator
        value = (phase < pulseWidth) ? 1.0 : -1.0;
        value += minBLEP.process();
        value = (value + 1.f) / 2.f;
    }
};

#endif  // RACKNES_CLOCK_HPP

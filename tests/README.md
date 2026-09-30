# Focused Regression Checks

These checks run the actual module processing and serialization code against
Rack's headers and library. They use assertions rather than a separate test
framework. No game ROM or graphical Rack session is required.

## Run

From the repository root, using the prepared Rack tree at `../..`:

```shell
make -C tests -j2
```

For another Rack 2 SDK or prepared source tree:

```shell
make -C tests RACK_DIR=/absolute/path/to/Rack-SDK
```

The default compiler flags enable AddressSanitizer and UndefinedBehaviorSanitizer
and stop on the first sanitizer error. A matching C++11 compiler, sanitizer
runtime, Rack headers, dependency headers, and `libRack` are required. The
Makefile sets the library search path for macOS and Linux. On Windows, use
MSYS2 MINGW64 with `SANITIZERS=` and put the matching Rack runtime directory
on `PATH`; the SDK supplies only the import library. Keep the compiler's C++
runtime ahead of the Rack runtime on `PATH`.

Outputs and dependency files stay in ignored `tests/.build/`. If you change
compiler, SDK, or sanitizer flags, rebuild with `make -C tests -B`. For a compiler
without sanitizer support, `SANITIZERS=` disables instrumentation; report that
limitation when recording results.

When `BUILD` points elsewhere, `check` still creates `tests/.build/` for the
temporary ROM fixtures before running the suites.

## Continuous Integration

[Rack plugin and regression tests](../.github/workflows/rack-tests.yml) builds
the plugin and runs both headless fixtures on Linux x64, macOS arm64, and
Windows x64. It runs on pull requests, pushes to `master`, and
manual dispatch. Rack SDK 2.6.3 archives and the Windows runtime are verified
against SHA-256 checksums, following Fourier's workflow. The Windows installer
is extracted without running it. These matrix runs disable sanitizers.

[Coverage and sanitizers](../.github/workflows/instrumentation.yml) runs separate
Linux Clang 18 jobs for AddressSanitizer/UndefinedBehaviorSanitizer and LLVM
coverage on pull requests, `master` pushes, and manual dispatch. The jobs use
fresh temporary build directories, retain logs and reports for 14 days, and
fail on compiler, assertion, or sanitizer errors. Coverage reports focus on
`RackNES.cpp`, `CVGenie.cpp`, `GameMaps.hpp`, and `theme.hpp`; they exclude the
bundled emulator libraries, Rack SDK, and test fixtures. The prebuilt Rack
library is not instrumented. Coverage describes these focused checks, not
complete emulator or UI validation.

## Coverage

-   CV Genie: inactive and invalid selections, UI selection handoffs, map changes,
    randomize/reset, finite and non-finite voltage inputs, nonzero endpoints,
    ordinary and descending toggles, one write per edge across buffer flips,
    saved toggle state, malformed JSON, and oversized location arrays.
-   Patch compatibility: restore the existing Genie data from
    `patches/debugCVGenie.vcv`, preserving its game and location indices. The
    fixture's audio settings and ROM path are not used. Generated legacy JSON
    also exercises absent toggle fields and invalid selections.
-   Catalog: all ten maps pass menu enumeration, selection, address bounds,
    endpoint writes, toggle lifetime, disconnect, randomization, and JSON round
    trips. Restored selections produce the expected writes, and disconnects
    clear both expander buffers. New maps also reject duplicate names/addresses
    and have independently reviewed anchors.
-   Mapping: the five Mario Enemy Heading indices resolve to `0x0046`--`0x004A`
    and no longer overwrite Enemy 5 Type. The address source is
    [the SMB disassembly](https://6502disassembly.com/nes-smb/SuperMarioBros.html#SymEnemy_MovingDir).
-   RackNES: empty-emulator serialization, snapshot release on SAVE, reset,
    patch restoration, and destruction; preserving the display after an invalid
    ROM path; and expander RAM boundaries and row precedence. Controller checks
    restore held buttons and partially read streams and ignore malformed bytes.
    Synthetic cartridge headers check NES 2.0 format detection, legacy byte-8
    handling, and rejection of high mapper IDs without aliasing IDs 0--3 or
    continuing patch restoration in an empty or active emulator. CHR-ROM
    fixtures round-trip empty CHR RAM through mapper JSON for IDs 0--2 and
    check MMC1's initial CHR banks, odd/even bank selections, and transitions
    between 4 KiB and 8 KiB modes using distinct patterns in each 4 KiB bank.
    Adapted nes-py cases check mirroring independently of battery flags and
    CNROM CHR-bank bounds, read-only CHR, legacy register restoration, and
    unchanged PRG windows. Four-screen decoding is tested only as metadata;
    four-screen VRAM support is still absent.
-   Playback safety: Blip_Buffer bulk, one-sample, stereo, and direct-reader
    paths agree on positive and negative PCM, including overlapping buffer
    compaction. PPU reset clears sprite-hit status and stale buffered reads.
    The five-channel CPU-program audio fixture below runs in the default suite.

The Genie fixture uses a real Rack `Module` neighbor with two explicit expander
buffers. Tests connect input channels as Rack's engine would: `setChannels()`
alone cannot connect a disconnected port. The RackNES fixture creates a headless
Rack engine context for the module's sample-rate initialization.

These checks do not test rendering, a live UI session, or every game's response
to memory edits. They do not replace listening and gameplay checks when changing
CPU/PPU/APU timing or audio conversion.

## Focused Audio Characterization

A CPU-program fixture compares NROM and CNROM integer PCM for all
five voices, including looping DMC while the CPU writes CHR bank selections.
It checks 2,000 host samples at each of 44.1, 48, 96, and 192 kHz, at nominal
CPU speed with the existing integer cycle loop, using both the core's default
Blip clock and RackNES's fixed 768,000 Hz Blip clock. Both mapper runs must
match sample by sample, produce nonzero output on every channel, and generate
the same number of frame callbacks. It also prints a reproducible PCM
fingerprint for before/after comparisons on the same toolchain.

This fixture previously exposed overlapping copies and negative signed shifts
in Blip_Buffer, followed by uninitialized PPU edge-visibility flags. Those
defects are fixed, and `make -C tests` now includes the audio run under the
configured sanitizers. From the repository root on macOS with the prepared
Rack tree, run the full suite or isolate playback:

```shell
make -C tests -j2
(cd tests && DYLD_LIBRARY_PATH=../../.. .build/racknes --audio-only)
```

For an uninstrumented comparison, keep the default sanitizer build intact
and build to a separate ignored directory:

```shell
make -C tests BUILD=.build/audio SANITIZERS= .build/audio/racknes
(cd tests && DYLD_LIBRARY_PATH=../../.. .build/audio/racknes --audio-only)
```

Use `LD_LIBRARY_PATH` on Linux, the Rack runtime in `PATH` on Windows, and
the same external `RACK_DIR` and runtime directory for both builds if needed.
The generator is in `check_graphics_audio_preservation()` in `racknes.cpp`;
it removes its temporary ROM on success. The fixture leaves reset-default
rendering enabled with zero-filled CHR; it does not configure graphics through
PPU registers. It does not cover clock modulation, channel/MIX routing, listening,
or audio changes caused by a game's response to corrected graphics behavior.

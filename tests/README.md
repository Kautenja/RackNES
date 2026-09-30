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
    MMC1 work-RAM checks cover all 8 KiB with and without a battery, DMA page
    reads, PRG bank changes, cartridge replacement, and current/legacy snapshot
    restoration. An original CPU program stages room and sprite bytes in
    cartridge RAM and transfers them to the PPU and internal RAM, exercising
    the missing-memory defect investigated for Metroid #26 without a game ROM.
    Adapted nes-py cases check mirroring independently of battery flags and
    CNROM CHR-bank bounds, read-only CHR, legacy register restoration, and
    unchanged PRG windows. Four-screen decoding is tested only as metadata;
    four-screen VRAM support is still absent.
-   AxROM: 1/2/4/8 PRG banks, all byte write values, both one-screen pages,
    CHR RAM, old-bank bus conflicts, strict image rejection, malformed mapper
    JSON, cartridge clones after source destruction, live/backup Rack patch
    restoration, CPU/module reset, and DMC callbacks across distinct PRG banks.

-   Palette/NTSC: all 256 palette byte values decode to six-bit colors. Full
    602-by-240 filtered frames match a direct filter reference for all 64 colors
    with each combination of upper bits, through both PPUDATA writes and legacy
    palette JSON restoration. This synthetic backdrop fixture reproduces an
    unsafe filter-input path found while investigating issue #45; it does not
    establish correct Bubble Bobble gameplay.
-   Playback safety: Blip_Buffer bulk, one-sample, stereo, and direct-reader
    paths agree on positive and negative PCM, including overlapping buffer
    compaction. PPU reset clears sprite-hit status and stale buffered reads.
    The five-channel CPU-program audio fixture below runs in the default suite.

The Genie fixture uses a real Rack `Module` neighbor with two explicit expander
buffers. Tests connect input channels as Rack's engine would: `setChannels()`
alone cannot connect a disconnected port. The RackNES fixture creates a headless
Rack engine context for the module's sample-rate initialization.

These checks do not test live UI rendering or every game's response
to memory edits. They do not replace listening and gameplay checks when changing
CPU/PPU/APU timing or audio conversion.

## Focused Audio Characterization

A CPU-program fixture compares NROM, CNROM, AxROM, and MMC1 integer PCM for all
five voices, including looping DMC while the CPU writes bank selections.
AxROM repeats identical code/sample data in four PRG banks, switching from
bank 0 to bank 3; a separate DMC callback test reads distinct bank markers.
It checks 2,000 host samples at each of 44.1, 48, 96, and 192 kHz, at nominal
CPU speed with the existing integer cycle loop, using both the core's default
Blip clock and RackNES's fixed 768,000 Hz Blip clock. All four mapper runs must
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
The generator is `make_audio_image()` in `racknes.cpp`; the PCM check
removes its temporary ROM on success. The fixture leaves reset-default
rendering enabled with zero-filled CHR; it does not configure graphics through
PPU registers. This PCM comparison does not cover clock modulation, channel/MIX
routing, listening, or audio changes caused by a game's response to corrected graphics behavior.

## MMC1 Host And Game Replays

The default suite also runs `check_mmc1_host()` against the actual Rack module:
44.1/48 kHz sample-rate notifications, minimum/normal/maximum clock, clock CV,
all 32 individual-output connection combinations and MIX exclusion, Hang,
coincident SAVE/RESET/LOAD, independent live/backup patches, five-voice playback
after LOAD, failed/successful ROM replacement, and module reset. A synthetic
snapshot with IRQs permitted verifies that APU reconstruction does not modify
the restored CPU or stack. Legacy CHR snapshots retain partial serial writes
while rebuilding derived windows from registers. These checks require no games.

For optional game evidence, build the same test executable and supply your own
ROM and an existing output directory. The repository contains no game ROMs.
From `tests/`, with the Rack library in the platform's runtime search path:

```shell
.build/racknes --replay "$ROM" "$EVIDENCE_DIR/game" 48000 1500 replay-input.txt
.build/racknes --replay "$ROM" "$EVIDENCE_DIR/game-44100" 44100 1500 replay-input.txt
.build/racknes --replay-roundtrip "$ROM" "$EVIDENCE_DIR/game-restore" 48000 1500 replay-input.txt
```

Use `DYLD_LIBRARY_PATH="$RACK_DIR"` on macOS or the runtime paths described above.
Each input line contains a completed video-frame count and two decimal
controller bitmaps in the existing A/B/Select/Start/Up/Down/Left/Right order.
The supplied sequence presses Start after frames 180, 300, and 420, moves right
at 600, adds A at 720, and releases at 780. The runner uses normal emulation
speed and RackNES's fixed 768,000 Hz Blip clock. It exports paired 256-by-240
palette-index PGM and 602-by-240 filtered PPM frames every 60 video frames,
plus state JSON and interleaved five-channel signed 16-bit little-endian PCM
(`.s16le`, channels SQ1/SQ2/TRI/NOI/DMC). JSON contains the supplied ROM path;
keep captures outside tracked source. PGM maximum value is 63, not 255.

Every exported frame checks color bounds and exact agreement between the raw
pixels filtered at either alternating burst phase and the production output.
The diagnostic accessors are synchronous engine-thread reads; they are not a
UI synchronization interface. `--replay-roundtrip` saves at frame 1200, restores
at 1260, and asserts cartridge/CPU/PPU/RAM restoration. Compare its frame 1500
with uninterrupted frame 1440 when no later controller inputs intervene. Audio
snapshots do not preserve every phase/buffer and are not sample-exact loops.

For an independent unfiltered reference, build a Nestopia libretro core and run
from a writable evidence directory, passing paths to the repository helpers:

```shell
python3 "$REPO/tests/reference_nestopia.py" "$NESTOPIA_CORE" "$ROM" \
    "$EVIDENCE_DIR/reference" "$REPO/tests/replay-input.txt" 1500
```

This optional standard-library helper uses NTSC, no cropping/filter, and the
core's raw palette to export the same 0--63 indices; it does not compare emphasis
bits, which RackNES does not implement. It downloads nothing. Reference core
revision, fixture identities, observed differences, and actual validation are
recorded in [spec 005](../specs/005-mmc1-ntsc-regressions.md).

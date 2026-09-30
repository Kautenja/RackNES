### 1.0.0 (2020-06-14)

-   support for the first four cartridge mappers
-   2 player support
-   frame-rate-based clock control
-   clock output based on frame-rate
-   APU that works at all sample rates

### 1.1.0 (2020-06-19)

-   clock controls CPU rate directly
-   OS dialogs shown when ROMs fail to load
-   fix clock to properly output pulses at the frame rate (PW=50%)

### 1.1.1 (2020-06-19)

-   ix issue where backup / restore was freezing the emulation state

### 1.1.2 (2020-06-19)

-   resolve potential memory leak: deallocate emulator and backup state when
    module is removed from the rack

### 1.1.3 (2020-06-20)

-   resolve bug where RackNES would cause a segmentation fault when attempting
    to load a ROM from a JSON state where the ROM file pointed to is invalid

### 1.1.4 (2020-06-21)

-   resolve bug where RackNES would crash when the clock frequency was set to
    absolute minimum value through CV

### 1.1.5 (2020-06-28)

-   fix some aesthetic issues in the plugin.json file

### 1.1.6 (2020-06-30)

-   fix audio crackling / popping (BlipBuffer now runs locked at 768kHz)

### 1.2.0 (2020-07-01)

-   mixer (individual channel outputs + mix output)

### 1.3.0 (2020-07-02)

-   NTSC emulation

### 1.4.0 (2020-07-03)

-   ROM files can be loaded by dropping them onto the module

### 1.4.1 (2020-07-09)

-   improve ROM file loading and storage
-   fix mapper number calculation to include the 4-high bits in the 12-bit
    format
-   fix location of manual

### 1.4.2 (2020-07-11)

-   fix cycle durations to be more accurate

### 1.4.3 (TBD)

-   CV acquisition at _1/16x_ sample rate
-   support for dark mode (the NES display no longer dims)
-   fix logic for changing the sample rate

### 2.0.0 (2022-02-20)

-   support for Rackv2
-   pre-release of CV genie

### 2.1.0 (2022-02-26)

-   theme support
    -   new dark theme

### 2.2.0 (Unreleased)

-   Add standard MMC3B/C mapper 4 banking, banked CHR RAM, RAM protection,
    four-screen mirroring, filtered PPU A12 IRQs, and validated mapper state.
    Combine mapper/APU IRQ levels at CPU instruction boundaries, prioritize
    latched NMI, correct interrupt stack status, and refresh restored APU IRQs.
    Fix three PPU snapshot flags and handle cartridge-ROM/disabled-RAM OAM
    DMA safely. IRQ-driven software may change behavior; audio synthesis,
    buffer clocks, and the five-channel PCM reference remain unchanged.

-   Add Import SRAM and Export SRAM for explicit, reviewed 8 KiB battery-RAM
    layouts on mappers 0--3. Use bounded generation-checked engine/UI transfers,
    exact-size validation, and temporary-file replacement. Preserve SAVE/LOAD
    snapshots and legacy patch RAM; decline ambiguous or banked save layouts.
    Initialize the APU snapshot wrapper so omitted fields do not serialize
    uninitialized stack bytes.

-   Add mapper 9 (MMC2 / PxROM): bounded PRG/CHR banks, post-read CHR latches,
    mirroring, 8 KiB PRG RAM, validated mapper state, and cartridge clones.
    Reuse fetched pattern bytes for MMC2 rendering and preserve them in
    snapshots. Initialize bus-owned cartridge RAM on ROM replacement; saved
    patch RAM still restores afterward. Document the supported image subset.

-   Restore MMC1 CHR windows from saved registers instead of stale derived
    offsets. Add MMC1 audio, host controls, routing, and game replay regression
    coverage, including CPU/stack preservation during APU reconstruction.

-   Add mapper 7 (AxROM / AOROM): bounded 32 KiB PRG banking, one-screen
    mirroring, 8 KiB CHR RAM, validated JSON restoration, and independent
    cartridge clones. Validate supported image layouts before loading;
    NES 2.0 submapper 2 includes bus conflicts. Preserve base NES audio.
    Safely serialize empty bus RAM and scanline sprite lists during snapshots.

-   Expose MMC1 work RAM without requiring a battery flag, fixing discarded
    cartridge RAM writes needed by Metroid (#26). Preserve the existing
    snapshot format and safely restore legacy empty work RAM. Metroid gameplay
    listening verification remains pending.

-   Mask rendered palette colors to six bits before NTSC conversion, preventing
    out-of-range filter lookups and corrupted frames when palette writes or
    legacy snapshots contain upper bits. Add full-frame regression coverage
    for all 256 palette byte values while investigating issue #45.

-   Fix overlapping sample-buffer copies and undefined negative-value shifts
    in Blip_Buffer. Initialize PPU edge visibility, sprite-hit status, and the
    PPUDATA buffer on reset. Run the five-channel audio fixture in the default
    regression suite, including sanitizers where available.

-   Follow Rack's global light/dark panel preference for RackNES, CV Genie,
    and browser previews, including live theme changes. Replace the Plugin
    Theme menu and stop reading the legacy `RackNES.json` preference. Require
    Rack 2.4 or newer; saved patches remain compatible.

-   Safely serialize empty CHR RAM on NROM, MMC1, and UxROM cartridges without
    changing saved JSON. Correct MMC1's initial CHR-ROM bank pair and 8 KiB bank
    alignment when selecting banks or changing CHR modes.

-   Adapt nes-py's mirroring decoding so battery flags do not alter nametable
    layout, and bound CNROM CHR-bank reads to available graphics banks. Preserve
    PRG mapping, audio scheduling, and existing mapper JSON fields. Four-screen
    rendering is supported by the new MMC3 mapper; CNROM bus conflicts remain
    unsupported.

-   Restore controller button and serial-stream bytes from patch JSON correctly,
    ignoring malformed byte values. Decode high mapper bits only for NES 2.0
    headers, prevent unsupported high mapper IDs from aliasing IDs 0--3, and
    stop patch restoration when cartridge loading rejects an unsupported mapper.

-   Add curated CV Genie maps for Mega Man 1--2, Castlevania 1--2, Contra,
    Metroid, Ninja Gaiden, and Nintendo Tetris. Preserve existing saved map
    indices. Replace manual address tables with a generated supported-game
    list; record mapping sources and cartridge limitations for contributors.

-   Add CV Genie row port names, assignment ranges and trigger modes in menus,
    and full assignment hover help. Clarify RackNES frame-clock and MIX routing
    tooltips.
-   Safely construct RackNES browser previews without a module, and release
    display images on widget deletion and graphics-context teardown.

-   Rebrand RackNES and CV Genie as Arhythmetic Units, including panel and
    manual artwork. Preserve existing plugin and module slugs for saved patches.

-   Make CV Genie unassigned rows and menus safe before game selection, clear
    stale assignments on game changes, and validate restored game/row indices.
-   Clamp continuous Genie CV to 0--10 V, ignore non-finite signals, and toggle
    between each element's actual endpoints. Reset toggles on reassignment and
    preserve their choices in patches, with defaults for older patches.
-   Correct the five Super Mario Bros. Enemy Heading addresses without changing
    saved element indices. Clear unused expander messages and bound RAM writes.
-   Release RackNES snapshots with JSON reference counting, safely serialize an
    empty emulator, and preserve the current display when a ROM load fails.
-   Update the manuals to reflect these fixes and add focused SDK-backed
    regression checks.

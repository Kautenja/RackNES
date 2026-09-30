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

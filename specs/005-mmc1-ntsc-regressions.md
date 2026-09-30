# MMC1 And NTSC Regression Verification

Resolve [#26: Metroid MMC1 failure][metroid] and
[#45: Bubble Bobble rendering artifacts][bubble] with reproducible game-level
evidence and focused fixes where needed. An old report is not resolved merely
because nearby code has changed.

Status: IN PROGRESS

Created: September 30, 2026

## Goal And Behavior

Metroid should reach its title screen and gameplay with correct assets and
working audio. Bubble Bobble should render its title and gameplay without the
reported corruption. Normal composite color fringing is distinct from missing
or incorrectly selected tiles; compare like-for-like rendering settings.

A SAVE/LOAD round trip during gameplay must preserve graphics and sound.
Invalid ROM replacement must retain the running game and display.

## Evidence And Investigation

The #45 discussion suspects MMC1 as well as the NTSC filter. The #26 report
also includes audio failure, and its comment says adding the filter did not
change Metroid's rendering. Treat these as observations, not proven causes.
Current [MMC1](../src/nes/mappers/mapper1_MMC1.hpp) fixes CHR-ROM startup and
8 KiB bank alignment; [tests](../tests/racknes.cpp) exercise synthetic CHR-ROM
banks. Neither establishes correct behavior for either reported ROM revision
or covers every CHR-RAM board configuration.

1.  Record ROM revision, SHA-256, header, mapper/submapper, PRG/CHR/RAM sizes,
    region, and provenance. Use user-supplied games outside tracked source.
    Request missing revision/reproduction details in the issue if necessary;
    record an unavailable fixture as blocked evidence, not success.
2.  Capture current Rack behavior at normal clock and 48 kHz from cold load,
    title screen, and gameplay with an explicit controller sequence. Record
    Rack/plugin revision, platform, elapsed emulated frames, and audio routing.
3.  Compare palette-index pixels before the NTSC filter and the final filtered
    frame at the same emulated frame. Use test/debug capture outside the hot
    path; do not disable production NTSC filtering as a supposed fix.
4.  Compare against an identified reference emulator with matching ROM,
    controller sequence, timing, and palette/filter settings where applicable.
    If raw pixels are wrong, isolate mapper/PPU behavior first. If only the
    filtered image is wrong, inspect input ranges, stride, dimensions, output
    format, and filter state in [PPU](../src/nes/ppu.cpp) and
    [display integration](../src/widget/display.hpp).
5.  Reduce each confirmed defect to a redistributable synthetic fixture that
    fails before the fix. Share MMC1 work with [spec 001](001-nes-py-integration.md)
    and retain its audio, timing, attribution, and JSON compatibility contracts.
    Avoid a whole-core port to fix one bounded regression.

## Requirements And Non-Goals

Preserve the five audio channels, MIX exclusion, host sample rates, frame-clock
behavior, controller acquisition, hang, and save/reset/restore ordering.
Exercise ROM replacement and patch restoration as well as cold starts; legacy
snapshots may contain bank offsets not corrected by a cold-load fix.

No new mapper family, palette-CV feature, public filter bypass, speculative
NTSC-library rewrite, or screenshot redraw belongs to this investigation.
A test fixture may expose internal pixels without changing the product UI.

## Acceptance Criteria

- [x] Each issue has an independent reproduction and result record, including
    fixture identity and cold-load/title/gameplay observations.
- [x] Confirmed defects have deterministic failing-before/passing-after tests
    at the mapper, PPU, filter, or host boundary actually responsible.
- [x] Both graphics stages have been compared; #45's original attribution is
    confirmed or corrected with evidence rather than repeated as fact.
- [ ] Metroid audio and all five synthetic audio voices pass at 44.1 and
    48 kHz; gameplay listening, channel/MIX routing, and clock extremes are
    checked separately from executable regressions.
- [x] SAVE/LOAD, legacy patch restore, hang, reset, valid replacement, and
    failed replacement preserve their documented behavior.
- [ ] Close each issue only after its reported symptoms are verified resolved.
    If already fixed, cite the responsible change when established and add
    the missing regression evidence; no new production edit is required.
    If one remains reproducible, leave that issue open even if the other passes.

## Validation And Completion Evidence

From the repository root with the same prepared Rack tree for both builds
and the [documented prerequisites](../CONTRIBUTING.md#development-and-testing):

```shell
make -j4
make -C tests -j2
git diff --check
```

Run `make -C manual` and inspect the PDFs if behavior documentation changes.
Add exact capture/replay commands here when fixtures are implemented; no
unimplemented runner is presumed to exist. Follow the production capture guide
if a visible widget or runtime SVG changes, which is not currently planned.

Record dates, commits, fixture hashes, reference-emulator versions, screenshots
or artifact locations, listening observations, executed commands, and skipped
checks here. The existing synthetic checks and this planning pass are not a
manual Rack validation. Archive only after both issue dispositions have evidence.

### September 30, 2026: Metroid Work RAM Fix

Investigated #26 from source revision `261b548`. The cartridge bus gated all
`$6000`--`$7FFF` accesses on the iNES battery flag. Metroid uses volatile work
RAM: the [disassembly memory map][metroid-memory] places room buffers at
`$6000`/`$6400`, player state at `$6875`, and intro sprite data at `$6E00` and
`$6EA0`. Its [startup routine][metroid-startup] clears `$6000`--`$7FFF`.
This establishes a missing-memory defect relevant to the reported assets;
it does not establish that every reported graphics/audio symptom is resolved.

An original synthetic iNES fixture uses mapper 1, 128 KiB PRG, CHR RAM, and
byte 8 equal to zero. With battery flag clear, the unpatched regression aborts
at `bus.read(0x6000) == 0xA5` after writing `0xA5`. With the patch it passes.
MMC1 now supplies the bus's conventional 8 KiB work RAM independently of
battery presence. The bus allocates fresh storage on cartridge replacement.
Existing `extended_ram` JSON stays unchanged; empty legacy or invalid-sized
RAM data initializes zero-filled mapper-sized storage instead of shrinking it.
Other mapper RAM-presence rules are unchanged. Board-specific RAM sizes and
MMC1 PRG-RAM disable behavior remain outside this fix.

Validation on macOS arm64, Apple Clang 21.0.0, and the prepared Rack 2.6.0 tree:

```shell
# From the fix worktree, with RACK_DIR pointing to the prepared Rack tree:
make -C tests -j2 RACK_DIR="$RACK_DIR"
make -j4 RACK_DIR="$RACK_DIR"
git diff --check
```

-   Before the patch: existing suites pass; the new work-RAM assertion fails.
-   After the patch: AddressSanitizer/UndefinedBehaviorSanitizer suites pass,
    including full-window RAM patterns, DMA pages, eight PRG banks, mapper
    reset, fresh storage, current snapshots, and empty/short/mistyped legacy
    data. An original CPU program transfers cartridge RAM to PPU nametable
    memory and internal sprite staging RAM; full emulator restore preserves
    work RAM and accepts the old empty representation safely.
-   The five-channel NROM/CNROM audio fixtures match their baseline at 44.1,
    48, 96, and 192 kHz: `f5146c03e0a6ceb2` at the 1,789,773 Hz Blip clock and
    `d8915d435c9cf9a9` at 768,000 Hz.
-   Plugin build and `git diff --check` pass. Compiler warnings remain in
    existing Rack API usage and bundled code; no new build failure occurred.
-   No user-supplied Metroid ROM was available for this pass. No commercial ROM
    or copied game code is included. Actual title/gameplay captures, reference
    emulator comparison, Metroid listening, Rack UI/MIX routing, clock extremes,
    and manual SAVE/LOAD/hang/reset checks were not run. #26 remains open.
    #45 was not investigated; this shared spec remains in progress.

[metroid-memory]: https://github.com/nmikstas/metroid-disassembly/blob/master/Source_Files/Metroid_Defines.asm
[metroid-startup]: https://github.com/nmikstas/metroid-disassembly/blob/master/Source_Files/Bank07.asm

### September 30, 2026: Issue #45 Palette-Range Fix

Investigated from revision `261b548` on macOS arm64 with the prepared Rack
2.6.0 tree and Apple Clang. The issue screenshot shows a recognizable Bubble
Bobble title with severe color corruption. No Bubble Bobble ROM revision or
fixture was available for this run, so the original game's title, gameplay,
and reference-emulator comparison remain unverified. Issue #26 was not tested.

The NTSC integration introduced in `6d3ce77` passes palette bytes directly to
`nes_ntsc_blit`. The configured filter has 64 entries and does not mask its
input. `PictureBus::read_palette()` previously returned all eight stored bits,
allowing input values above 63 to read beyond the filter table. This establishes
a concrete unsafe integration path, but not that Bubble Bobble exercises it.
The [PPU palette reference][palette-reference] documents six-bit color values.

Added `check_ntsc_palette_range()` in [tests/ntsc.hpp](../tests/ntsc.hpp), using
no game ROM. With the production code unchanged, an initial full-frame test
passed for ordinary colors and failed with `NTSC mismatch: high bits 0x40,
restored 0`. It compares every output pixel against the bundled composite
filter given a known six-bit input image. The failure is reproducible after
cold reset and independent of MMC1. The final fixture also checks every byte
value at the palette-read boundary and repeats full-frame checks with legacy
JSON palettes containing upper bits.

Masking `read_palette()` to `0x3F` makes all eight frame comparisons pass:
64 colors, four combinations of upper bits, and two input paths (PPUDATA and
legacy palette JSON). The output is 602 by 240 pixels. The patch leaves palette
storage, saved JSON keys/bytes, CPU-visible reads, filter configuration, display
buffer ownership, and emulator scheduling unchanged. It adds only a bounded
bit mask to rendering, with no allocation or synchronization changes.

Validation from the isolated worktree root, with `RACK_DIR` set to the same
absolute prepared Rack tree for both commands:

```shell
make -j4 RACK_DIR="$RACK_DIR"
make -C tests -j2 RACK_DIR="$RACK_DIR"
git diff --check
```

-   Plugin build passed on macOS arm64 (Rack 2.6.0).
-   CV Genie, RackNES, and the audio fixture passed with AddressSanitizer and
    UndefinedBehaviorSanitizer. The prebuilt Rack library is not instrumented.
-   Five-channel NROM/CNROM PCM fingerprints match the pre-fix baseline:
    `f5146c03e0a6ceb2` at the 1,789,773 Hz Blip clock and `d8915d435c9cf9a9`
    at 768,000 Hz, over 44.1, 48, 96, and 192 kHz host rates.
-   No manual Rack session, Bubble Bobble/Metroid gameplay or listening,
    game SAVE/LOAD, clock-extreme, or cross-platform checks were performed.
    Existing headless snapshot, invalid-load, controller, and mapper checks
    passed but do not substitute for these missing game-level checks.
-   Keep this spec active and issue #45 open pending the original game check;
    the palette fix alone does not satisfy the combined acceptance criteria.

[palette-reference]: https://www.nesdev.org/wiki/PPU_palettes

[metroid]: https://github.com/Kautenja/RackNES/issues/26
[bubble]: https://github.com/Kautenja/RackNES/issues/45

### September 30, 2026: Integration And Game Verification

The initial status check examined only `techreport` at `4e11320` and missed
completed work on two other branches. Integrated `826e707` from
`codex/metroid-mmc1` and `7938975` from `codex/issue-45-ntsc`, preserving their
separate evidence above. Both fixes are now together on `codex/mmc1-regressions`.
Concurrent MMC2/SRAM work in the original checkout was not modified or committed.

Added three focused corrections during verification:

-   Rebuild MMC1 CHR offsets from its saved registers. A synthetic legacy state
    with stale offsets failed before the correction and passes afterward in
    4/8 KiB modes, including continuation of a partial serial-register write.
    JSON keys remain compatible. This does not add CHR-RAM board variants or
    general validation of malformed mapper states.
-   Zero-initialize the first-party APU snapshot before serialization. The
    bundled snapshot writer does not fill triangle phase, so repeated saves
    previously emitted different stack bytes in that JSON field. The host
    failed-replacement equality assertion caught this. Actual oscillator and
    Blip phase preservation remains limited; no sample-exact looping claim.
-   Suppress IRQ servicing while reconstructing the APU from a snapshot.
    Bubble Bobble SAVE/LOAD changed CPU PC/SP, flags, skipped cycles, and stack
    bytes because register reconstruction notified a changed IRQ schedule.
    An original synthetic snapshot with CPU flags zero and `$4017 = $C0`
    independently failed CPU-state equality before the fix. Afterward the
    synthetic check and both game round trips pass. Live IRQ behavior is
    unchanged; the broader IRQ scheduling work remains in spec 001.

#### Fixture Identity And Reference

At the user's request, obtained two public archive images from
`oldies.ndlp.info/roms/nes/` on September 30, 2026. ROMs, extracted game data,
snapshots, and screenshots remain outside tracked source. The archive filenames
are not proof of an original issue reporter's revision. These hashes identify
exactly what was tested:

| Fixture | Bytes | SHA-256 |
| --- | --- | --- |
| `Metroid.nes` | 131088 | `c5eea06e1e1128b576bd789f1a4f63bb154d6d31579c2f319382fb77a72d34a6` |
| Original `bubble bobble.nes` | 262288 | `00b2c4b4caf3f1e78ec8e4ce93d43fff55db0451f28016119b468ce302fc63e6` |
| Normalized Bubble Bobble test image | 262160 | `ef05b55feb1c2506d67e1d2db94ad0464f214ccf7b6debef992fffdbca028895` |

Metroid's header is `4e45531a0800110000004e4920312e33`: legacy iNES mapper 1,
128 KiB PRG, CHR RAM, no battery or trainer. Its PRG SHA-1 is
`fdbfc7871962f72a1ef57e5a7e456164fb93430b`, CRC32 `70080810`; the reference
core's database identifies this payload as NTSC NES-SNROM with 8 KiB work RAM.
The legacy header has no submapper and retains an old tool signature.

Bubble Bobble's source header is `4e45531a0810114469736b4475646521`, with
`DiskDude!` contamination setting spurious mapper bits and 128 trailing bytes.
For this test only, zeroed header bytes 7--15 and retained exactly the declared
128 KiB PRG plus 128 KiB CHR payload. The resulting header is
`4e45531a081011000000000000000000`: legacy mapper 1, no battery/trainer,
no submapper. Payload SHA-1 `587f3361e9ddec66cbf3bfb416208ead693ca82d` and
CRC32 `C3D576EA` are unchanged. Both cores used this same normalized image,
forced/assumed NTSC; the exact regional revision was not independently verified.
The production ROM parser was not changed to accept contaminated headers.

Primary reference: [Nestopia libretro][nestopia-reference], version 2.0.0,
source archive revision `8f00f50`, built on macOS arm64 with
`make -C libretro -j4`. Used no crop, no filter, and its raw palette to compare
0--63 color indices. The optional [reference helper](../tests/reference_nestopia.py)
contains the settings and input mapping. A preliminary JSNES 2.1.0 comparison
had missing sprite details and was not used as the final reference.

#### Reproduction And Observations

Used the committed [controller sequence](../tests/replay-input.txt) for both
cores. Cold-load replays run 1500 completed video frames at nominal CPU speed,
with the RackNES 768,000 Hz Blip clock, separately at 44.1 and 48 kHz. Start
pulses after frames 180/300/420 reach the game menus and gameplay; right and A
exercise movement/jumping after frames 600/720. Headless recording uses all
five individual PCM voices rather than a sound-device MIX connection.

-   Metroid: title, menu, room tiles, Samus, and enemies render. Raw frames
    120, 240, and 600 match Nestopia exactly. A separate negative-control
    executable reverting only MMC1 RAM availability reproduces missing title
    stars and gameplay filled with repeated background tiles; the fixed image
    restores them. Later movement diverges in scrolling/timing from the
    reference (8074 differing pixels at frame 1200), so this is not a claim
    of complete PPU/CPU accuracy.
-   Bubble Bobble: animated title, selection menu, intro, and first-round
    gameplay render without the reported large-scale color corruption.
    The game stores `0xFF` in palette RAM (observed at frame 1200), establishing
    that it actually exercises the unsafe NTSC-input path. The synthetic
    failing-before test covers the same upper-bit condition. Raw/reference
    differences remain around sprite positioning/timing (370 pixels at frame
    1200); broad mapper/PPU accuracy is not claimed.
-   For every exported game frame, the complete production NTSC output matches
    the bundled filter applied independently to the paired raw pixels at the
    appropriate alternating burst phase. Every raw color is in 0--63. No filter
    bypass or library rewrite was introduced.
-   Both games produce pulse 1, pulse 2, triangle, and noise PCM at both sample
    rates; DMC remains silent in this selected gameplay sequence. The synthetic
    MMC1 fixture exercises all five voices, including DMC, and matches
    NROM/CNROM/AxROM sample by sample at 44.1/48/96/192 kHz. Its two fingerprints
    remain `f5146c03e0a6ceb2` and `d8915d435c9cf9a9`.
-   Game round trips save at frame 1200, restore at 1260, and run to 1500.
    Cartridge, CPU, PPU, and RAM restore exactly. For both games the final raw
    frame is byte-identical to uninterrupted frame 1440. Audio resumes; this
    does not assert sample-identical continuation. Normal-run raw frame 1500
    is also identical between the 44.1 and 48 kHz runs for each game.
-   Expanded actual-module assertions cover minimum/normal/maximum clock,
    clock CV, live host-rate notifications, all 32 channel connection/MIX
    combinations, Hang holding CPU/PPU and outputs, simultaneous SAVE/RESET/LOAD,
    independent live/backup patch data, fresh and failed replacement, and reset.
-   An isolated real Rack Free 2.6.0 session loaded the built plugin and reopened
    a patch containing both gameplay snapshots. Visually checked the two
    production displays. No audio device was connected. UI button interaction
    could not be verified after computer-use lost window access; deterministic
    module/replay checks above supply the control evidence, not a claimed
    complete manual session.

Reproducible capture commands and output formats are in
[tests/README.md](../tests/README.md#mmc1-host-and-game-replays). Build/check from
the isolated worktree with an absolute `RACK_DIR` for the prepared Rack tree:

```shell
make -j4 RACK_DIR="$RACK_DIR"
make -C tests -j2 RACK_DIR="$RACK_DIR"
git diff --check
```

These passed on macOS arm64 with Apple Clang 21 and Rack 2.6.0. ASan/UBSan
instrument the test executable and emulator, not the prebuilt Rack library.
No other platform was built. No manual source, widget, or runtime SVG changed,
so manual PDF/capture regeneration was not required.

Local evidence is retained in the worktree's ignored `tests/.build/spec005/`
directory: build/test logs, before/after comparisons, reference/game captures,
and replay output. The original downloads and raw PCM are in the temporary
`racknes-005-games` fixture directory. No private absolute paths or game assets
are committed.

Implementation and executable game verification are complete for these
fixtures. Keep the spec active and both issues open pending a gameplay listening
check and final issue disposition. Nonzero PCM is not evidence that a human
heard correct Metroid music; the original issue's audio claim must not be
silently marked resolved. No listening pass was performed in this session.

[nestopia-reference]: https://github.com/libretro/nestopia/tree/8f00f50

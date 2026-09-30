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

- [ ] Each issue has an independent reproduction and result record, including
    fixture identity and cold-load/title/gameplay observations.
- [ ] Confirmed defects have deterministic failing-before/passing-after tests
    at the mapper, PPU, filter, or host boundary actually responsible.
- [ ] Both graphics stages have been compared; #45's original attribution is
    confirmed or corrected with evidence rather than repeated as fact.
- [ ] Metroid audio and all five synthetic audio voices pass at 44.1 and
    48 kHz; gameplay listening, channel/MIX routing, and clock extremes are
    checked separately from executable regressions.
- [ ] SAVE/LOAD, legacy patch restore, hang, reset, valid replacement, and
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

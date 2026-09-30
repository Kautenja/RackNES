# MMC1 And NTSC Regression Verification

Resolve [#26: Metroid MMC1 failure][metroid] and
[#45: Bubble Bobble rendering artifacts][bubble] with reproducible game-level
evidence and focused fixes where needed. An old report is not resolved merely
because nearby code has changed.

Status: PLANNED

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

[metroid]: https://github.com/Kautenja/RackNES/issues/26
[bubble]: https://github.com/Kautenja/RackNES/issues/45

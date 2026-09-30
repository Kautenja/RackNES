# MMC5 Mapper Implementation

Implement and validate a bounded mapper 5 (MMC5/ExROM) subset in RackNES,
adapting the reviewed nes-py implementation while preserving RackNES's base
APU, musical timing, and saved patches. Finish implementation by updating
the user manual and technical report with the tested capabilities and limits.
This specification does not register mapper 5 or claim current support.

Status: PLANNED

Created: September 30, 2026

## Ownership And Baseline

This is the MMC5 implementation sub-spec of
[001: nes-py Emulator Integration](001-nes-py-integration.md). Spec 001
continues to own issues #1 and #31, general cartridge parsing, and shared
audio/state compatibility policy. Completing this increment does not close
those issues or the [MMC1 game regressions](005-mmc1-ntsc-regressions.md).
Keep MMC5 decisions, validation results, and remaining limitations here.

-   RackNES baseline: commit `2cea414`, supporting mapper IDs 0 through 4,
    7, and 9. MMC3 supplies independent mapper/APU IRQ levels and PPU address
    observation, but its simplified A12 address stream is not a complete
    record of PPU reads.
-   Porting reference: nes-py commit
    `301da52f7f75de380e6e195fd36621c3d5b03757`, specifically its
    [MMC5 implementation][upstream-mmc5], [interface][upstream-header], and
    [regression tests][upstream-tests]. Preserve applicable attribution.
-   Hardware references: [NESdev MMC5][mmc5], [ExROM board layouts][exrom],
    and [IRQ acknowledgement][irq]. Record any reference corrections and
    hardware/test-ROM evidence before departing from these contracts.

The upstream code is a starting point, not a conformance oracle. It inflates
nonzero PRG RAM below 64 KiB to 64 KiB, uses a 32-tile-read scanline heuristic,
and switches background banks through a short fetch counter. Its vertical
split registers have no renderer, and its expansion audio writes are no-ops.
Do not copy those approximations into an unrestricted MMC5 support claim.

## Goal And Examples

An accepted cartridge must boot with deterministic PRG/CHR mapping, route
banked RAM consistently through every bus consumer, render the included MMC5
graphics features, and deliver scanline interrupts without disturbing audio.

1.  A program maps writable RAM at `$6000` and in a high PRG window, changes
    banks, and locks writes. CPU reads, OAM DMA, DMC reads, and CV Genie bus
    writes must observe the same mapping and protection rules.
2.  A background tile uses a different CHR bank from a sprite. Extended
    attributes change that tile's palette and graphics without changing the
    sprite or leaking attributes into the next tile.
3.  A scanline match sets pending status while MMC5 IRQ output is disabled.
    Enabling it exposes the pending level; acknowledging MMC5 leaves an
    independent APU IRQ pending and does not consume an NMI.
4.  A snapshot between nametable and pattern fetches resumes with identical
    subsequent pixels, RAM, IRQ timing, and base-APU output. Restoring a
    backup must not retain pointers or callbacks into the replaced mapper.
5.  A rejected ROM or malformed MMC5 snapshot leaves the current cartridge
    and backup usable. A cartridge using deferred expansion audio or vertical
    split rendering is not advertised as fully compatible merely because its
    header and mapper number are accepted.

## Scope And Difficulty

| Area | Difficulty | Required Outcome |
| --- | --- | --- |
| PRG/CHR registers and multiplier | Moderate | All included modes, masks, alignment, and boundary cases have deterministic checks. |
| Banked RAM and CPU bus integration | High | One authoritative RAM store; CPU, DMA, DMC, and CV writes agree. |
| Nametables, ExRAM, and fetch context | High | Actual fetch phases select the correct background/sprite data and attributes. |
| Scanline detection and IRQs | High | Read-sequence and idle detection drive an independent level IRQ. |
| Snapshot and legacy compatibility | High | Complete MMC5 state and safe restoration without changing older mapper states. |
| Documentation and Rack validation | Moderate | Manual/report claims match demonstrated support and explicit omissions. |

Included features are PRG banking, CHR banking, protected banked PRG RAM,
1 KiB ExRAM and its four modes, per-nametable routing, fill mode, extended
attributes, the multiplier, and scanline IRQs. The first implementation is
NTSC only and must describe itself as a partial MMC5 implementation.

Deferred features are vertical split rendering, MMC5 pulse/PCM sound and PCM
IRQs, other MMC5 variants or undocumented behavior without evidence, general
NES 2.0 parser expansion, and new SRAM-file layouts. Do not add audio ports,
change the base APU or mixer, port nes-py's scheduler, or undertake an unrelated
PPU rewrite. Limited fetch changes necessary for MMC5 are in scope.

Raw SRAM import/export remains disabled for mapper 5 under the policy of
[spec 004](archive/004-sram-import-export.md). Banked cartridge RAM must still
survive full snapshots. Header acceptance cannot detect all runtime uses of
deferred features; document that distinction rather than promising a reliable
load-time rejection of every incompatible game.

## Implementation Requirements

### Cartridge And Board Policy

Before enabling mapper 5 in the factory, freeze a reviewed board/profile table
here. For each accepted layout, record PRG ROM and CHR ROM/RAM size limits,
PRG RAM chip capacities, chip-select/bank wiring, volatile/nonvolatile split,
battery policy, and how iNES or NES 2.0 identifies it. Include test fixtures
for the smallest and largest accepted sizes and for every accepted RAM layout.

Target ordinary ExROM layouts described by the board reference; do not infer
that every possible combination of sizes is valid. Explicitly decide the
legacy iNES unspecified-RAM default. NES 2.0 declarations must not silently
gain RAM. Reject ambiguous or unsupported layouts, mixed memory configurations
without a defined mapping, unsupported submappers/timing/console types, and
truncated payloads through the existing transactional load path. General
parser extensions remain governed by spec 001.

This profile table is a prerequisite, not a permission to guess board wiring
from a ROM title. In particular, do not adopt upstream's automatic 64 KiB RAM
allocation or assume all smaller chips behave like a linear modulo array.

### CPU Registers And Banked Memory

-   Add narrowly decoded expansion reads/writes for MMC5 registers and ExRAM.
    Preserve the direction-dependent base APU/controller registers, including
    `$4017`. Unhandled reads must follow an explicit, tested bus policy.
-   Implement the four PRG modes, all bank selectors, the fixed-ROM final
    window, ROM/RAM selection where available, and both RAM unlock registers.
    Test ignored address bits, alignment, out-of-range bank selection, and
    protection in both low and high RAM windows against the chosen profiles.
-   Replace fixed-array bypasses where MMC5 needs mapper-owned RAM access.
    Keep a single authoritative RAM buffer; do not shadow it in the main bus.
    Preserve the established behavior and saved JSON of existing mappers.
-   Route DMA, DMC, and ordinary CPU accesses through the current mapping.
    A cached pointer to the old `$6000` bank is not sufficient. Check DMA from
    expansion space and any read side effects at that seam as well.
-   Implement the multiplier and defined readback/status behavior. Document
    and test reset defaults, reserved bits, and the treatment of unsupported
    split/audio registers without claiming their deferred functions work.

### CHR, Nametables, And ExRAM

Implement all four CHR bank granularities with correct alignment and upper-bit
latching. Audit background/sprite register selection for 8x8 and 8x16 sprites,
including the last-written register-set behavior where applicable. Do not
assume that writing a background register selects that set forever. CHR RAM
must either follow the reviewed banking contract or be excluded explicitly
from the accepted profile table.

Route each nametable quadrant to the selected CIRAM page, ExRAM, or fill source.
Retain shared CIRAM ownership in the picture bus instead of copying upstream's
private nametable RAM. Cover tile bytes, attribute bytes, mirrored addresses,
fill palette replication, and ignored writes. Specify and test the CPU/PPU
access truth table for each ExRAM mode, including rendering-time restrictions
and mode changes. Extended attributes must retain the right tile context
through the subsequent attribute and pattern fetches.

The current renderer's repeated per-pixel reads and MMC3's A12-only observation
addresses are not valid MMC5 fetch input. Establish a bounded PPU event seam
that carries the actual address and required read/fetch context. Distinguish
background, sprite, dummy, and CPU `$2007` accesses; preserve physical read
ordering and idle intervals. Cached rendering must not generate extra mapper
reads. Test pre-render, visible lines, scrolling, rendering enable transitions,
and both sprite sizes. Keep MMC2 latch and MMC3 IRQ regressions passing.

### Scanline IRQ And Scheduling

Use the MMC5 documented PPU-read sequence detector rather than counting calls
to the renderer or assuming every 32 tile reads is a scanline. Cover repeated
nametable reads, the following read that detects a line, first-line behavior,
counter comparison, and loss of in-frame status after the documented CPU idle
interval. Confirm edge cases against independent expected traces, not only
upstream's synthetic scanline helper. See [the hardware description][mmc5].

Represent scanline pending state separately from output enable. Reading IRQ
status acknowledges MMC5; changing enable must preserve the defined pending
semantics. Feed the existing `irqPending()` level path instead of injecting
one-shot CPU interrupts. Verify simultaneous MMC5, APU frame, DMC, and NMI
conditions and selective acknowledgement. See [IRQ behavior][irq].

Add a mapper CPU-clock hook if needed for idle detection. It must run once per
emulated CPU cycle, including DMA/stall cycles, at a documented phase relative
to PPU events. Preserve the existing CPU/PPU/APU ratios, frame-clock output,
audio buffer clock, and Hang behavior. No allocation, logging, blocking, or
file access may be introduced into the per-cycle hooks.

### State And Ownership

Serialize all bank/mode/protection registers, allocated RAM, ExRAM, fill and
multiplier state, CHR selection/latches, scanline detector history, idle count,
pending/enable state, and any PPU fetch context needed to resume. Recompute
derived mappings safely after restoration. Specify reset, ROM replacement,
backup/restore, and patch-load behavior separately.

Validate types, sizes, indices, and buffer lengths before mutating live state.
New state fields need a documented version/default policy; old mapper patches
must remain readable. Clone operations must rebind the destination ROM and
callbacks and must not alias mutable RAM. Exercise destruction and replacement
under sanitizers, including a failed load with an existing backup.

## Delivery Order

1.  Record the accepted profile table and hardware/upstream differences.
    Establish failing deterministic banking and bus fixtures.
2.  Implement register decoding, PRG/CHR banking, and authoritative RAM access.
3.  Integrate nametable/ExRAM routing, real fetch context, and scanline IRQs.
4.  Complete reset/snapshot ownership and run compatibility/audio checks.
    Enable factory acceptance only for profiles whose gates pass.
5.  Run the manual Rack checks below and record limitations. Then update the
    manual and whitepaper as the final implementation deliverable.
6.  Record commands, results, platform, revisions, and unverified checks here.
    Mark COMPLETE and archive only after the required acceptance gates pass;
    keep partial implementation IN PROGRESS otherwise.

## Acceptance And Validation

Use the existing standalone harness in [tests](../tests/README.md), not a new
test framework. Add focused MMC5 fixtures to its normal `check` target so CI
executes them. Generated fixtures must use original or redistributable code.
Record provenance, revision, hash, and expected results for external ROMs.

- [ ] The profile table is concrete; accepted and rejected headers, payload
    boundaries, RAM wiring, and allocation limits have deterministic checks.
- [ ] All PRG/CHR modes, RAM protection, multiplier, nametable sources, fill,
    and ExRAM mode transitions pass independent expected-value checks.
- [ ] CPU programs exercise the real buses, DMA, DMC, and high-window RAM.
    Register-only unit checks are insufficient.
- [ ] PPU fetch traces and rendered fixtures cover sprite/background banking,
    extended attributes, scroll boundaries, dummy/CPU reads, and idle gaps.
- [ ] IRQ traces cover pending while masked, enabling a pending source,
    acknowledgement, rendering transitions, DMA stalls, and competing IRQ/NMI.
- [ ] Reset, backup/restore, patch JSON, malformed state, and failed ROM loads
    preserve ownership and state; existing mapper patches still restore.
- [ ] Existing mapper 0-4, 7, and 9 regressions pass, particularly MMC2 fetch
    latches and MMC3 IRQs affected by shared PPU changes.
- [ ] Existing PCM fingerprints remain unchanged. Extend the base-APU fixture
    to MMC5 and check banked DMC/IRQ cases at 44.1, 48, 96, and 192 kHz.
    Preserve five individual channels, MIX exclusion, frame timing, clock
    extremes, sample-rate transitions, and Hang/resume. No expansion-audio
    fidelity claim follows from matching the five base channels.
- [ ] In Rack, boot a legally available identified MMC5 fixture; verify its
    graphics, base audio, IRQ behavior, save/restore, patch reopening, and ROM
    replacement at 44.1 and 48 kHz. Record listening/display observations
    separately from automated checks and list unsupported features exercised.
- [ ] Final documentation accurately describes the supported subset, and
    rendered manuals/report have been inspected.

From the repository root, use a prepared Rack tree or configure `RACK_DIR`
as described in [the contributor guide](../CONTRIBUTING.md#configure-the-rack-sdk).
The C++ checks use the same SDK as the plugin; documentation needs the TeX
prerequisites in the [manual guide](../manual/README.md) and
[report guide](../whitepaper/README.md).

```shell
make -j4
make -C tests -j2
make -C manual
make -C whitepaper
make -C whitepaper source
git diff --check
```

These are implementation validation commands, not results of writing this
specification. If source packaging changes, extract and compile the archive
independently using the report guide. Record missing prerequisites and skipped
manual checks explicitly; neither a build nor a ROM boot proves compatibility
with every MMC5 game.

## Final Manual And Whitepaper Updates

After implementation and validation, update the supported-mapper descriptions
in [README](../README.md), [the contributor architecture](../CONTRIBUTING.md#architecture),
and the [RackNES manual](../manual/RackNES). List accepted board/header limits,
graphics features, snapshot behavior, disabled SRAM-file interchange, and
deferred vertical split and expansion audio. Update any affected CV Genie
compatibility text without adding unverified game-map claims. Add an accurate
[changelog](../CHANGELOG.md) entry; do not imply a published release.

Update the canonical [manuscript](../whitepaper/racknes.tex), its
[source evidence](../whitepaper/sources.md), and [report overview](../whitepaper/README.md).
Extend the dated mapper addendum with MMC5's memory, fetch, IRQ, and audio
boundaries. Preserve the historical implementation revision unless intentionally
revising that account. Keep citation metadata consistent if the manuscript
version or publication status changes. Build and inspect the PDFs; keep
generated files in ignored build directories. Refresh production screenshots
only if visible UI changes require them under the manual guide.

Finish by updating spec 001 and the [spec index](README.md) with verified
progress and remaining gates. Documentation must distinguish accepted headers,
implemented hardware features, tested software, and deferred functionality.

[upstream-mmc5]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/mappers/mapper_MMC5.cpp
[upstream-header]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/include/nes_emu/mappers/mapper_MMC5.hpp
[upstream-tests]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/test/nes_emu/mappers/test_mapper_MMC5.cpp
[mmc5]: https://www.nesdev.org/wiki/MMC5
[exrom]: https://www.nesdev.org/wiki/ExROM
[irq]: https://www.nesdev.org/wiki/IRQ

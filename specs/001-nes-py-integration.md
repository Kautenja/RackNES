# nes-py Emulator Integration

This specification defines a selective integration of nes-py's newer native
emulator into RackNES. The objective is broader cartridge compatibility and
targeted correctness improvements while retaining RackNES's musical timing,
five-channel APU, NTSC display, CV behavior, and saved patches. This document
tracks implementation work; the port remains incomplete and is not validated
in Rack. See the partial progress record below for completed local fixes.

Status: IN PROGRESS

Created: September 30, 2026

## Current Progress

Reviewed September 30, 2026 against committed revision `4e11320`:

-   Mapper IDs 0 through 3 and 7 are committed. The AxROM increment records
    passing plugin, sanitizer, audio, state, and documentation checks below;
    manual Rack/gameplay/listening validation remains open.
-   MMC2 (mapper 9) and its PPU fetch changes are local work in progress,
    outside this specification update. They have no recorded mapper-specific
    regression or Rack validation and are not a supported-mapper claim.
-   Mappers 4, 5, and 69, general NES 2.0 support, and the remaining ownership,
    CPU/IRQ, state, audio, and manual gates are incomplete. Issues #1 and #31
    remain open; retain `Status: IN PROGRESS`.

## GitHub Issue Tracking

This specification owns [#1: NES Mappers][issue-1] and
[#31: NES 2.0 Support][issue-31]. Planning and partial parser fixes do not
resolve either issue. Track game-specific failures in
[005: MMC1 And NTSC Regression Verification](005-mmc1-ntsc-regressions.md).

The following issue gates can be completed before the entire integration:

- [ ] For #1, implement and validate both remaining requested families,
    AxROM/AOROM (7) and MMC3 (4), including banks, mirroring, IRQs where
    applicable, audio, legacy patches, and snapshot restoration. IDs 5, 9,
    and 69 remain goals of this spec but are not required to close #1.
    Link the separate Metroid investigation instead of claiming that mapper
    registration resolves #26.
- [ ] For #31, implement checked NES 2.0 ROM/RAM size decoding, including
    exponent/multiplier ROM sizes, submapper/console/timing policy, and exact
    payload validation under the contracts below. Test zero RAM, volatile
    and nonvolatile RAM, malformed/truncated files, and allocation limits.
    Unsupported boards must fail explicitly rather than run with guessed RAM.
- [ ] Inventory exact Pulsar and PR8 releases from #31: record source URLs,
    hashes, headers, mapper/submapper, and required RAM banking before coding
    board support. Do not assume parser changes alone make them work. If a
    tracker requires a board outside the current matrix, record and implement
    the additional contract here before claiming the issue is resolved.
- [ ] Run Pulsar and PR8 in Rack: reach the editor, create and play a short
    pattern, exercise song/pattern memory, and save/load and reopen a patch.
    Record actual audio and display results at 44.1 and 48 kHz. Check NTRQ
    and cajoNES as regression candidates; their 2020 success report is not
    current validation. Keep ROM files outside tracked source unless their
    redistribution terms explicitly permit inclusion.
- [ ] Document the supported NES 2.0 subset and remaining restrictions in the
    manual and README. Close #31 only when both the format gates and named
    tracker checks pass; support for every NES 2.0 board is not implied.

For example, loading a supported tracker must allocate its declared memory;
a truncated payload or unsupported board must leave the current game and
snapshot intact with a useful load error. Battery-file interchange is owned
by [004: SRAM Import And Export](archive/004-sram-import-export.md), not this parser
increment. Apply the build, regression, and manual validation commands below
and record results against each issue gate.

[issue-1]: https://github.com/Kautenja/RackNES/issues/1
[issue-31]: https://github.com/Kautenja/RackNES/issues/31

## Baseline And Evidence

Prepared September 30, 2026 against these source revisions:

-   RackNES: `b783a8e473f00242773e3fccbd4ed9e1a6ae17d5`, manifest version
    `2.2.0`. The emulator factory implements mapper IDs 0 through 3.
-   nes-py: [commit 301da52f7f75de380e6e195fd36621c3d5b03757][upstream], the
    `master` tip retrieved for this review. Pin implementation comparisons to
    this revision rather than a moving branch or package version.
-   At preparation, the working tree contained independent RackNES/CV Genie
    fixes and untracked standalone regression checks in `tests/`. Those changes
    are outside this specification commit. Reconcile with their final committed
    form before implementation; do not overwrite or silently absorb them.
    The standalone harness is now tracked and includes a `check` target.

The authoritative inventory is the [upstream mapper factory][factory] and
source, not the larger mapper queue in the gym-nes umbrella repository.
There are nine registered IDs at this revision, five more than RackNES.
Upstream's older `docs/native-mapper-capability-gaps.md` still describes only
IDs 0 through 3 and should not be used as the current support list.

Relevant RackNES sources are [the emulator][local-emulator], [APU wrapper][apu],
[ROM parser][rom], [cartridge factory][cartridge], [main bus][main-bus],
[picture bus][picture-bus], [PPU][ppu], and [module integration][module].
Upstream evidence links below are pinned to the reviewed revision.

## Integration Decision

Port the cartridge metadata, mapper abstractions, mapper implementations, and
required CPU/PPU fixes into `src/nes/`. Keep RackNES's public emulator methods
and host integration. Do not replace the emulator with nes-py's frame-stepping
runtime, add a Python dependency, or import its Gymnasium, vector environment,
observation, packaging, or windowing layers.

The absence of upstream audio does not prevent mapper integration. It makes
the bus, interrupts, DMC reads, and scheduler integration mandatory work. In
particular, [nes-py's bus][upstream-bus] omits base APU writes at
`$4000-$4013` and `$4017`; copying it unchanged would disable sound. Its
[emulator][upstream-emulator] also has no APU step or audio snapshot.

Treat base NES audio preservation and cartridge expansion audio as separate
acceptance conditions:

-   All nine target mapper IDs must retain RackNES's existing five base NES
    voices, including DMC playback through the active PRG mapping.
-   This integration does not add MMC5 pulse/PCM or Sunsoft 5B synthesis.
    Mapper 5 and mapper 69 ship as explicitly partial support, with those
    sound limitations visible in the manual and cartridge information shown
    after load. Do not describe them as full audio compatibility.
-   Full expansion audio is a follow-up requiring its own synthesis, state,
    timing, gain, and output-routing design. It is not a prerequisite for
    the first mapper integration, and must not silently change the five
    existing channel outputs or mix semantics.

No module slug, parameter/port/light ID, controller bit order, CV Genie game
ID, or memory-location index changes are part of this work. Keep C++11 and the
existing Rack build. Do not update the historical technical report's source
revision or citation metadata as a side effect.

## Change Inventory

### Mapper Coverage

The following is the target matrix, not a claim that every board variant or
game using an ID works. Port matching synthetic tests from the [upstream
native tests][upstream-tests], then verify through RackNES's complete emulator.

| ID | Family | Integration Work | Required Evidence And Limits |
| --- | --- | --- | --- |
| 0 | NROM | Adopt bounded bank/memory helpers while retaining legacy state loading. | 16/32 KiB PRG, CHR ROM/RAM, mirroring, RAM and snapshot round trips. |
| 1 | MMC1 / SxROM | Adapt upstream SxROM implementation to existing MMC1 state fields. | Serial writes, reset bit, PRG/CHR modes, PRG RAM, mirroring, and restore during an incomplete serial write. No blanket claim for all SxROM variants. |
| 2 | UNROM / UxROM | Adopt upstream bank handling without changing saved mapper identity. | Switchable/fixed PRG banks, CHR RAM, out-of-range bank handling. Upstream UxROM does not emulate bus conflicts; preserve and document that limitation. |
| 3 | CNROM | Adopt bounded CHR bank selection and board conflict policy. | PRG mapping, CHR switching, read-only CHR, and explicit conflict-policy tests. |
| 7 | AxROM / AOROM | Add 32 KiB PRG switching and mapper-controlled one-screen mirroring. | Both nametable pages, CHR RAM, bank boundaries, clone and JSON restoration. Implement first as the simplest new mapper. |
| 9 | MMC2 / PxROM | Add PRG/CHR banks and PPU-read-driven latches. | The triggering CHR read returns the old bank's byte; the latch affects subsequent reads. Test both latches and state between trigger and observation. |
| 4 | MMC3 / TxROM | Add PRG/CHR inversion, RAM protection, mirroring, filtered A12 observations, and IRQ state. | Include the latest sprite-fetch fixes, IRQ reload/disable/re-enable, four-screen override, and split-screen stability with moving/hidden sprites. Upstream models common MMC3B behavior, not every revision. |
| 69 | Sunsoft FME-7 / 5B | Add command decoding, banked ROM/RAM at `$6000`, mirroring, CPU-cycle IRQ counter, and audio-register storage. | Counter wrap/acknowledge, RAM enable, DMC bank reads, and all stored sound registers. Expansion sound remains absent. |
| 5 | MMC5 / ExROM | Add expansion registers, PRG/CHR modes, RAM protection, ExRAM/nametable/fill routing, multiplier, and scanline IRQ behavior. | Exercise background/sprite CHR selection, ExRAM, read-to-clear IRQ status, and snapshots. Upstream lacks vertical split rendering and pulse/PCM audio. |

For MMC3, the [current PPU implementation][upstream-ppu] and fixes
[`c5231f8`][mmc3-fix] and [`da7702c`][mmc3-stabilize] are part of the port.
Do not stop at the initial mapper implementation: mapper-visible sprite fetches
must occur even when no visible sprite pixels need drawing, and pixel rendering
must not emit duplicate fetch observations. The upstream A12 filter counts
observations; it is not evidence of a fully cycle-accurate hardware bus.

### RackNES Manual And Mapper Presentation

The [RackNES user manual][racknes-manual] includes a "Supported cartridge
mappers" table in [ROMs and compatibility][manual-roms]. It currently lists
IDs 0 through 3 and 7 by ID and name at committed revision `4e11320`. Update
that table alongside each newly enabled mapper, checking it against
[the cartridge factory][cartridge] and validation
evidence. Keep the README compatibility summary aligned. The target matrix
above describes planned work; it must not become a user-facing support claim
before implementation passes its gates.

Present mapper IDs, family names, and relevant board, rendering, and audio
limitations together so partial support is clear. Preserve the distinction
between mapper support and guaranteed game compatibility, and between RackNES
cartridge compatibility and CV Genie's separate game-memory maps.

As the table grows, reassess its presentation in the rendered manual. A compact
table may remain sufficient for nine IDs; if limitations crowd the rows or the
table no longer fits comfortably, consider a multipage table with repeated
headers or a short ID/name summary linked to detailed compatibility notes.
Choose based on readability and ease of maintenance, without shrinking text
to force a fit. Record the choice here during implementation and follow the
[manual guide][manual-guide] for builds and visual review, including page
breaks, navigation, and keeping each limitation associated with its mapper.

### Other Fixes And Improvements

| Area | Decision | Scope And Acceptance |
| --- | --- | --- |
| Cartridge parsing | Port with additional validation. | Explicit iNES/NES 2.0 decoding, 12-bit mapper ID, submapper, PRG/CHR ROM and RAM sizes, four-screen priority, and exact payload reads. Reject unsafe sizes before allocation. |
| Mapper ownership | Adapt. | Owned mapper lifetime, explicit cartridge association, mapper RAM, cloning, and reattachment after load/restore. Retain RackNES JSON support, absent upstream. |
| Bus routing | Adapt. | Expansion and mapper RAM hooks, nametable delegation, mirroring updates, and CPU/PPU observations while retaining every APU route. |
| CPU correctness | Port selectively. | Explicit status-byte handling for PHP/PLP/RTI/BRK/IRQ/NMI and undocumented `$DF` DCP absolute,X, with opcode timing and flag tests. Preserve numeric JSON flag representation. |
| CPU decoder optimization | Defer until correctness passes. | Preclassified instruction dispatch is optional; do not confuse it with the required opcode/status fixes. |
| Direct PRG/CHR pages | Defer. | Require valid lifetime/invalidation on bank writes, RAM writes, load, reset, and restore, and proof that mapper read side effects and DMC routing are preserved. |
| Sprite prefetch/background tile caching | Defer optional acceleration. | Port the fetch semantics needed for mapper correctness first. MMC2/MMC3/MMC5 observations must remain equivalent with caches disabled or enabled. |
| CPU instruction batching | Exclude from initial integration. | Upstream batches without an APU and gates batching on mapper CPU hooks, which is insufficient for RackNES's audio/IRQ/DMC events. |
| Snapshot lifecycle | Adapt concepts only. | Upstream snapshots are same-process objects without RackNES JSON or audio; they cannot replace persistent patch data. |
| Controller and display APIs | Retain RackNES behavior. | Upstream controller reads are substantially the same. Keep two ports, existing button ordering and RackNES NTSC conversion/output dimensions. |

The old scanline-length correction to 341 cycles is already present in
RackNES. Do not count it as a newly imported fix. Compare behavior and tests,
not merely file movement or renamed methods, when documenting improvements.

## Core Integration Contracts

### Cartridge Metadata And Ownership

Use a mapper ID type at least 16 bits wide throughout parsing, dispatch, state,
and diagnostics. The factory enum now has a 16-bit underlying type;
high mapper IDs must never alias IDs 0 through 3. Read NES 2.0 high bits only
when the header actually declares that format.

Adapt [upstream cartridge parsing][upstream-cartridge] and bank helpers with
checked shifts, multiplication, file-length checks, and allocation limits.
The exponent/multiplier size encoding needs explicit overflow checks beyond
the upstream implementation. Distinguish malformed data, unsupported mapper,
unsupported variant/region, and resource failure. No exception may escape a
Rack processing callback. Failure must preserve the active cartridge, audio,
display, and backup; loading with no active cartridge must remain safe.

Define the initial supported format as standard NTSC NES/Famicom iNES and
validated NES 2.0 boards for the listed IDs. Reject trainers, PAL/Dendy-only
timing, unsupported console types and unimplemented submappers with a useful
reason. Do not import upstream's permissive parsing switch as a promise that
these features work. Record compatibility changes for files the old parser
accepted without implementing their hardware. Battery-backed memory is part
of patch state; adding a separate battery-save file service is out of scope.

Each live cartridge owns its mapper and ROM storage. Buses reference that
live mapper; immutable ROM storage may be shared with snapshots only with
explicit lifetime ownership. Clone/restore must rebind the cartridge pointer,
bus pointers, IRQ handlers, mirroring, and any derived bank pointers. Never
copy callbacks capturing the source emulator. Test restoring into a different
emulator and destroying/replacing the source, not just same-instance restore.

Separate cold cartridge insertion, NES reset, and Rack module reset. Preserve
the existing NES reset behavior for IDs 0 through 3; do not clear their writable
RAM or bank state simply because upstream objects are easier to reconstruct.
Specify and test each new mapper's reset behavior. Module reset removes the
cartridge and backup, disconnects bus references, and leaves safe empty-device
state; successful ROM replacement starts fresh mapper/APU state.

### CPU And Picture Bus

Use these routing boundaries, including direction-dependent `$4017`:

| Address | Read | Write |
| --- | --- | --- |
| `$0000-$1FFF` | Mirrored 2 KiB internal RAM. | Same internal RAM; CV Genie retains direct bounded access to this storage. |
| `$2000-$3FFF` | Mirrored PPU registers. | Mirrored PPU registers. |
| `$4000-$4013` | Existing base APU/open-bus policy. | Base APU register writes, including DMC. |
| `$4014` | Existing unmapped-read policy. | OAM DMA; retain CPU stalls and safe source reads. |
| `$4015` | Base APU status and its side effects. | Base APU channel enables. |
| `$4016` | Controller 1. | Strobe both controllers. |
| `$4017` | Controller 2. | Base APU frame counter. |
| `$4018-$401F` | Existing unused-register policy. | Existing unused-register policy. |
| `$4020-$5FFF` | Mapper expansion hook when handled. | Mapper expansion hook when handled. |
| `$6000-$7FFF` | Mapper-controlled PRG RAM or ROM. | Mapper-controlled RAM/register behavior and protection. |
| `$8000-$FFFF` | Active mapper PRG mapping. | Mapper registers or mapper-mapped RAM. |

DMC reads must use the same current PRG mapping as CPU reads; never retain a
pointer to the bank selected when a sample began. Test switching banks during
playback, DMC address wrap, loop/end IRQ, and replacement/restore. Audit DMC
stall behavior separately: retaining a reader callback does not establish
hardware-accurate DMC DMA timing. No accuracy claim may exceed the tests.

The picture bus must support four-screen storage and mapper-owned nametable
routing before enabling MMC3/MMC5. Dispatch PPU address observations before
the access, read observations after resolving the value, and write observations
after storage, as required by [the upstream mapper interface][mapper-api].
Keep MMC2's post-read latch behavior. Synchronize changed mirroring before
the next affected PPU access, including after restore.

### Cycle Scheduling And Interrupts

Retain `Emulator::cycle(callback)` as the host entry point. One call advances
three PPU cycles, the required mapper CPU-cycle hook, one CPU cycle, one APU
cycle, and mirroring synchronization. Mapper timing continues during CPU
instruction/OAM stalls. Freeze all emulated devices together when the host
hang control prevents stepping. Test the ordering at boundaries where mapper,
PPU, CPU, and APU events coincide.

Preserve the current 29,781-cycle frame callback and approximately 50% frame
clock duty cycle. Do not replace the frame output with nes-py's blocking
`step()` or silently retime it to a new PPU frame convention. RackNES currently
uses a loop comparing an integer index with `getClockSpeed() / sampleRate`;
retain its observed cycle quantization in this work. A fractional clock
accumulator would be a separate audible behavior change.

Keep the fixed 768,000 Hz Blip clock, host sample-rate setting, and clock
control range from `CLOCK_RATE / 16` to `CLOCK_RATE * 16`. CPU speed modulation
must not be wired into `APU::set_clock_rate()` as an incidental porting change.
Do not batch APU writes at frame boundaries. Retain the current timestamp-1,
one-cycle `end_frame(1)` convention until an independently tested scheduling
change is justified.

IRQ integration is a required correctness gate, not a callback concatenation:

1.  Track mapper IRQ and APU IRQ independently and combine pending sources at
    CPU instruction boundaries. NMI remains a separate source with defined
    priority. Acknowledging one IRQ source must not clear another.
2.  Retain a pending mapper request while the CPU interrupt-disable flag is
    set; deliver it when eligible unless its device acknowledges/disables it.
    Add mapper assertion/deassertion state where needed. Upstream's immediate
    `cpu.interrupt()` callback can discard a masked request and is not a
    sufficient combined-IRQ contract.
3.  Audit the base APU notifier against [Nes_Apu.h][nes-apu]. It reports that
    the earliest IRQ time may have changed, not that an IRQ must be delivered
    immediately. Use pending state/`earliest_irq()` in the APU's time domain;
    do not read `$4015` just to poll IRQs because that acknowledges state.
4.  Test APU frame IRQ, DMC IRQ, mapper IRQ, simultaneous sources, masked then
    unmasked IRQ, NMI coincidence, DMA stalls, and state restoration with a
    pending interrupt. Isolate intentional timing corrections in review and
    document their audible compatibility impact.

### Audio And Expansion-Sound Boundary

Keep `APU::NUM_CHANNELS == 5`, channel order, voltage scaling, level controls,
and exclusion of connected individual outputs from the mix. Verify all five
voices through actual CPU bus writes, not only direct APU calls.

For mapper 69, preserve the upstream 5B register select latch and register
bytes in mapper JSON. For mapper 5, document the unsupported `$5000-$5015`
audio behavior; do not route these addresses to the base APU or describe
ignored writes as synthesized sound. Provide a static mapper capability
description to the UI/manual, without per-sample warnings or logs.

RackNES already bundles [Nes_Fme7_Apu][fme7-apu], plus VRC6 and Namco sound
sources. Their presence is not integration or full chip support. The bundled
FME-7 implementation explicitly lacks noise and envelope synthesis. Reusing
it alone would still leave Sunsoft 5B incomplete; there is no bundled MMC5
synthesizer in the reviewed tree. A future expansion-audio spec must establish
complete supported modes, timestamped writes, state restoration, headroom,
clock/sample-rate behavior, and a routing/UI policy before enabling sound.

### Saved Patches And Snapshots

Keep the module's `emulator` and `backup` objects and the existing nested
`cartridge`, `controllers[0]`, `controllers[1]`, `bus`, `picture_bus`, `cpu`,
`ppu`, and `apu` meanings. Add an emulator `state_version` integer for the new
schema; absence means the legacy schema. Reject unknown newer versions safely.
New-version patches need not load in older RackNES, but old patches must load
in the new implementation.

Implement an explicit legacy migration rather than relying on similarly named
upstream members:

-   Preserve `cartridge.rom_path` and existing mapper-0-through-3 JSON keys.
    Recompute derived bank windows from validated state. Translate old MMC1
    shift-register/write-count fields into SxROM state without losing an
    in-progress serial write.
-   Move legacy `bus.extended_ram` bytes into mapper-owned PRG RAM using the
    former address mapping, while `bus.ram` remains the CV Genie-visible
    internal RAM. Define new-state precedence explicitly; reject inconsistent
    sizes instead of truncating, aliasing, or silently losing data.
-   Preserve CPU status as a numeric byte and controller state as byte values,
    including held buttons and a partially read serial stream. Retain the
    controller loader's corrected integer decoding in migration coverage,
    with malformed byte values left unchanged.
-   Serialize each new mapper's registers, writable PRG/CHR/nametable/ExRAM,
    protection state, IRQ counters/pending state, PPU latch/filter history,
    and expansion-register state. Save emulation frame phase and scheduler
    state introduced by this integration. Derived pointers/caches are rebuilt.
-   Retain the base APU snapshot. Specify audio-buffer restoration explicitly:
    flush stale queued output and reconstruct synthesis state for both the
    reference restore path and the new path. Test resumed audio against that
    policy; do not claim sample-exact continuation from legacy JSON, which
    does not contain complete Blip buffer history or emulator frame phase.
    Missing legacy phase uses a documented deterministic default of zero.
-   Validate JSON types, numeric ranges, decoded Base64 sizes, ROM identity,
    and mapper compatibility before changing the live instance. For new
    states, record a ROM content fingerprint computed during load; legacy
    states without one retain path-based loading. Missing/changed ROM or
    invalid state returns an error without a partial restore or crash.

Retain save, reset, then restore ordering when triggers coincide. Preserve
legacy keys needed by stored backups as well as the live emulator object.
Use Jansson reference ownership correctly, including every failure path.
Upstream's C++ clone snapshots are useful lifecycle references, not substitutes
for this migration or a portable state format.

### Real-Time And Display Constraints

New per-cycle mapper paths must be bounded and allocation-free. Allocate RAM
and wiring during cartridge preparation; prohibit hot-path file access,
blocking, logging, and drawing. Do not introduce lazy caches that allocate
while running. The current APU sample accessor allocates a vector and drains
all available samples while returning only the first; characterize that
behavior and do not silently replace it with a different resampler.

Existing ROM loading and JSON snapshots can occur in processing, so this port
must not claim full real-time safety. Record their measured cost as a known
limitation. Any allocation removal must preserve observable audio behavior
and be reviewed separately from mapper logic.

Retain the palette-index-to-`nes_ntsc` rendering path and the existing filtered
screen dimensions. Upstream's 256-by-240 RGB output is not a drop-in buffer for
the Rack widget. Preserve existing display handoffs where untouched. If new
load-status/capability information or buffers cross engine/UI boundaries,
define ownership and a synchronized handoff; shared flags or strings alone
are not a synchronization design.

## Implementation Sequence

Each stage is a separate reviewable change with its tests and evidence. Do not
enable a mapper in the factory before its audio and JSON checks pass.

1.  **Characterization and provenance.** Reconcile the independent working
    tree changes, pin upstream, inventory changed native files, and capture
    mapper-0-through-3 state/audio/display fixtures. Add focused C++11 checks
    using synthetic ROMs; adapt upstream test intent without importing its
    entire Catch2/CMake/Python infrastructure. Preserve upstream attribution
    and MIT notice alongside existing [project/dependency notices][license].
2.  **Parser, ownership, and legacy migration.** Introduce validated metadata,
    wide mapper IDs, bank helpers, mapper-owned RAM, explicit rebinding, and
    legacy state conversion. Keep only IDs 0 through 3 enabled initially.
3.  **Audio-aware bus and IRQ hooks.** Implement the routing table, safe DMA
    handling, PPU hooks, four-screen storage, mirroring synchronization, and
    combined IRQ contract. Demonstrate base APU/DMC preservation before adding
    IRQ-capable cartridges. Port CPU status fixes and `$DF` with tests.
4.  **AxROM and MMC2.** Add banking/mirroring, latch timing, JSON, and complete
    emulator tests. This establishes new-mapper lifecycle coverage.
5.  **MMC3.** Port the corrected sprite-fetch sequence with A12/IRQ tests and
    split-screen fixtures. Keep correctness independent of optional caching.
6.  **FME-7 and MMC5.** Add CPU-cycle IRQ and expansion/nametable routing,
    register/state tests, and explicit partial-support information. Expansion
    audio and MMC5 vertical splits remain deferred as described above.
7.  **Integration and documentation.** Run the complete matrix below, update
    the RackNES manual's mapper table and limitations, review its presentation
    as described above, align the README and changelog, and record
    actual platforms/fixtures. No release version, tag, push, or publication
    is implied by implementation of this specification.
8.  **Optional measured optimization.** Consider direct pages and PPU caches
    only after the correctness baseline is accepted. Keep a reference path
    for differential checks. CPU batching remains separate follow-up work.

## Validation And Acceptance

### Deterministic Regression Matrix

Generate small legal ROM fixtures with known bank patterns and programs.
Record generator revision, ROM SHA-256, mapper/submapper/header, input script,
clock settings, host rate, compiler flags, and expected results. Port useful
cases from upstream's `test_cartridge.cpp`, `test_cpu.cpp`,
`test_mapper_hooks.cpp`, `test_main_bus.cpp`, `test_picture_bus.cpp`, per-mapper
tests, and PPU sprite/background tests. Upstream is a comparison source for
non-audio behavior, not an audio oracle or proof of hardware accuracy.

| Check | Passing Evidence |
| --- | --- |
| Loading | Bad magic, short header/payload, zero PRG, overflowing sizes, high mapper IDs, unsupported variants/regions, and replacement failure cause no crash, alias, leak, or active-state loss. |
| Banks and buses | All mapper-matrix cases pass; expansion writes cannot swallow base APU writes; DMA never dereferences a null or stale page. |
| CPU/PPU | Status stack bytes, `$DF` flags/memory/cycle count, mapper event traces, MMC2 latch ordering, MMC3 sprite-fetch splits, and MMC5 routing match specified expectations. |
| Base sound | CPU programs exercise both pulses, triangle, noise, and DMC on every target mapper; scripted register traces and PCM are compared with the characterized base engine. Intentional IRQ/status changes have explicit new expectations. |
| Timing | Test 44.1, 48, 96, and 192 kHz host rates at clock minimum, unity, maximum, and modulated CV; check frame pulses, bounded stepping, finite output, and sample-rate changes during playback. No unexplained audio/timing change on IDs 0 through 3. |
| Interrupts | Independent APU/mapper pending state, masking, acknowledgements, NMI priority, DMA overlap, DMC bank switches, and restore near an IRQ boundary pass. |
| State | Legacy live/backup JSON for IDs 0 through 3 loads; every target mapper round-trips modified RAM/banks/latches/IRQs; malformed state leaves the live instance intact; source destruction after clone exposes no dangling references. |
| Host controls | Save/reset/load coincidence, hang/resume, module reset, both controllers, CV divider/gate thresholds, and connected-output exclusion remain compatible. |
| CV Genie | Right adjacency, eight address/value pairs, address-zero sentinel, buffer flips, detached/disconnected inputs, bounded internal RAM writes, and saved selections remain correct. |
| Lifecycle | Repeated load/replace/remove/save/restore and sample-rate changes pass AddressSanitizer/UndefinedBehaviorSanitizer checks without failures attributable to the port. |

Require both exact deterministic assertions and listening checks. For unchanged
APU workloads, compare integer PCM on the same build/toolchain. Where synthesis
startup or intentional IRQ corrections change output, define a focused expected
trace and measured comparison before accepting the difference; do not simply
relax a failing golden test.

### Commands And Test Deliverables

Extend the tracked standalone `tests/Makefile` so that `check` runs the new
emulator cases as well as the host checks. The harness was untracked at the
reviewed baseline; it now includes the focused checks recorded below, not the
complete acceptance matrix. A small assertion-based runner is sufficient.
Do not add a root `make test` fiction or rely on `-DTEST`.

From the repository root, with a prepared Rack 2 tree at `../..`, a C++11
compiler, Jansson/Rack headers and libraries, and supported sanitizers:

```shell
make
make -C tests check
git diff --check
```

For an external SDK, replace the example path with its absolute location and
use it for both builds (the tests directory has a different default relative
path):

```shell
make RACK_DIR=/absolute/path/to/Rack-SDK
make -C tests check RACK_DIR=/absolute/path/to/Rack-SDK
```

The final implementation must document fixture generation and any extra test
commands alongside its checks. Where manual sources change, run
`make -C manual` with the prerequisites in [the manual guide][manual-guide]
and inspect the generated PDFs. Record missing SDK, TeX, sanitizer, or platform
prerequisites as skipped checks, not successful results.

### Rack Session And Performance

Run a real Rack session using redistributable homebrew fixtures or user-supplied
ROMs outside the repository. Exercise every listed mapper, module creation,
panel preview, light/dark themes, NTSC display, controls, routing, CV Genie,
patch save/reopen, unavailable-ROM reload, replacement failure, and rate
changes. Check sound and display while the new IRQ-capable games run. A plugin
build or upstream test pass does not replace this session.

For optional optimization, use the same machine, compiler/options, fixtures,
input scripts, clock speeds, and sample rates before and after. Include base
APU and NTSC costs, warm up, and take at least five runs per workload. Report
per-host-sample processing time/distribution and overruns, plus frame throughput
where useful. Explain regressions and measurement noise. Upstream's headless
benchmarks cannot establish a RackNES speedup.

### Completion Gates

- [ ] Provenance and per-file port decisions are recorded at pinned revisions.
- [ ] IDs 0 through 3 preserve patch compatibility and characterized musical
      behavior, with intentional correctness differences documented.
- [ ] IDs 4, 5, 7, 9, and 69 pass mapper, full-emulator, audio, IRQ, and state
      checks before registration is enabled.
- [ ] MMC5 vertical-split/audio and Sunsoft 5B audio limitations are explicit
      in capability information and user documentation.
- [ ] Persistent state migration, Jansson ownership, and pointer rebinding
      pass malformed-input and lifecycle checks.
- [ ] Plugin build, executable regressions, manual Rack observations, and any
      performance results are reported separately with reproducible fixtures.
- [ ] Manuals/changelog and license attribution reflect implemented behavior;
      no commercial ROMs or generated build products are committed.
- [ ] The RackNES manual's supported-mapper table agrees with enabled factory
      IDs and verified limitations; the README summary agrees with the manual.
      The expanded presentation is reviewed in the rendered PDF, with the
      layout choice and review results recorded here.

## Specification Review Record

The initial specification was based on source inspection, a pinned upstream
checkout, and comparison of the current native implementations and tests. No
upstream code was imported, and no plugin build, executable emulator regression,
performance benchmark, or manual Rack session was performed for this
documentation-only change. Implementation gates above remain open.
Documentation validation checks relative links, pinned upstream source paths,
command prerequisites, Markdown formatting, and the complete specification
diff before commit.

### Partial Progress: September 30, 2026

Implemented a small local correctness pass without importing upstream code:

-   Controller JSON restores integer button and serial-stream bytes, preserving
    held buttons and partially consumed reads. Non-integers and values outside
    0--255 leave the corresponding byte unchanged.
-   Mapper dispatch uses a 16-bit ID. High mapper bits are read only when
    header byte 7 matches the NES 2.0 marker `(byte & 0x0C) == 0x08`.
    Legacy iNES byte 8 no longer contributes mapper bits; NES 2.0 IDs
    `0x100`--`0x103` are rejected instead of aliasing supported IDs 0--3.
-   Patch restoration returns failure immediately if cartridge loading rejects
    the mapper, before dereferencing or restoring cartridge state.
-   NROM, MMC1, and UxROM serialize empty CHR RAM using `vector::data()` without
    indexing an empty vector. The `character_ram` key and empty Base64 string
    remain unchanged.
-   MMC1 CHR-ROM startup maps banks 0 and 1 into the two 4 KiB windows. Both
    register-write paths clear the low bank bit in 8 KiB mode, rather than
    setting it. Independent 4 KiB selection remains intact. This intentionally
    corrects graphics after insertion and bank/mode writes; it does not migrate
    previously serialized derived bank offsets.
-   Reconciled the specification with the now-tracked assertion harness and
    updated the changelog and test coverage notes. All integration completion
    gates remain open; enabled mapper IDs remain 0--3.

Validation on macOS arm64 with Apple Clang 21 and the prepared Rack tree at
`../..`, from the repository root:

-   The new controller stream assertion failed before the production fix.
-   The new MMC1 startup assertion failed before its fix. After fixing CHR
    mapping, UndefinedBehaviorSanitizer reproduced the empty-vector reference
    in mapper serialization before the `data()` replacements.
-   `make -C tests -j2`: passed CV Genie and RackNES checks with AddressSanitizer
    and UndefinedBehaviorSanitizer. Added focused controller and synthetic
    cartridge-header cases, including unsupported-mapper restoration into empty
    and active emulators. CHR-ROM fixtures also verify empty-RAM mapper JSON
    round trips for IDs 0--2 and MMC1 startup, odd/even selections, and 4/8 KiB
    mode transitions. Generated ROMs stay in ignored `tests/.build/` and are
    removed after successful checks.
-   `make -j4`: plugin build passed. Existing Rack SDK deprecation warnings
    remain.
-   `git diff --check`: passed.
-   No manual Rack session, listening/display comparison, other-platform build,
    full emulator characterization, or performance measurement was performed.

Remaining work includes PRG/CHR bank bounds, full parser/size/submapper/region
validation, ownership and callback rebinding, transactional state migration,
CPU opcode/status fixes, audio-aware bus/DMA/IRQ changes, all five new mappers,
and the complete audio/timing/state acceptance matrix. The load-failure guard
is not a general malformed-state validator or a complete transactional restore
implementation. No claim of complete NES 2.0 support is made by the mapper-ID
correction.

### Selective Upstream Graphics Port: September 30, 2026

Compared the clean local nes-py checkout at pinned revision
`301da52f7f75de380e6e195fd36621c3d5b03757` against RackNES `42b99d8` and
adapted two graphics-only fixes:

| Upstream Source | Local Decision |
| --- | --- |
| `nes_emu/src/nes_emu/cartridge.cpp` | Adapt explicit mirroring-bit decoding to `ROM::getNameTableMirroring()`: the battery bit is independent and four-screen metadata has priority. No parser or RAM ownership rewrite. |
| `nes_emu/include/nes_emu/mapper_bank.hpp` and `mappers/mapper_CNROM.hpp` | Adapt complete-bank counting, bank wrapping, and the empty-memory guard to CNROM CHR reads. Keep RackNES's two-bit write latch and raw `select_chr` JSON field; resolve it on reads, including legacy restores. No direct pages, PRG changes, or new board variants. |
| `nes_emu/test/nes_emu/test_cartridge.cpp` and `mappers/test_mapper_CNROM.cpp` | Adapt mirroring and available-bank cases into the existing assertion runner. Extend with 0/1/2/4 CHR banks, all byte write values, read-only CHR, legacy JSON, and PRG-window checks. |

Retained original file attribution and added the upstream MIT notice and exact
source inventory to `docs/licenses/THIRD-PARTY.txt`. No CPU, APU, DMC callback,
main-bus routing, scheduler, sample conversion, or module output routing changes
are part of this port. The PPU still lacks four-screen storage/routing; correct
metadata alone is not four-screen support. CNROM still does not model bus
conflicts or gain CHR-RAM support; absent CHR data reads as zero.

Validation uses macOS arm64, Apple Clang 21, and the same prepared Rack tree as
the earlier pass. The new mirroring assertion failed before the fix; after
that correction, UBSan reproduced CNROM's empty-CHR read before its guard.
`make -C tests -j2` passes the default ASan/UBSan regressions, and `make -j4`
builds the plugin. `git diff --check` passes. Existing SDK warnings remain.

The new optional `--audio-only` fixture runs actual CPU bus writes for both
pulses, triangle, noise, and looping DMC, comparing NROM and CNROM PCM at
44.1/48/96/192 kHz for 2,000 samples per rate at nominal CPU speed. Before and
after this port, the uninstrumented core-default-clock PCM fingerprint was
`f5146c03e0a6ceb2`; every channel was nonzero and NROM/CNROM samples matched
exactly. The fixture also checks both mappers with RackNES's 768,000 Hz Blip
clock (matching PCM, fingerprint `d8915d435c9cf9a9`). The fixture leaves
reset-default rendering enabled with zero-filled CHR; the earlier description
of rendering as disabled was incorrect. No listening or manual Rack session
was run.
This evidence does not guarantee identical game audio when a game responds to
corrected PPU behavior. Reproduction commands are in `tests/README.md`.

The audio fixture exposed a pre-existing ASan `memcpy-param-overlap` failure
in `Blip_Buffer::remove_samples()` before any production port changes. At that
stage, bundled audio code remained untouched and only the separate
`SANITIZERS=` comparison passed. The follow-up below resolves the instrumented
playback failures. Wider PRG/CHR bank validation,
state migration, cloning/rebinding, full four-screen support, all new mappers,
and the complete audio/timing acceptance matrix remain open.

### Playback Sanitizer Fixes: September 30, 2026

Fixed the pre-existing failures reached by the five-channel fixture:

-   `Blip_Buffer::remove_samples()` always uses `memmove` when compacting its
    buffer. The old branch incorrectly selected `memcpy` for overlapping
    ranges. Sample counts, clearing, filtering, and scheduling stay unchanged.
-   Mono, stereo, and `Blip_Reader` scale signed deltas by multiplication rather
    than left-shifting negative values. The unsigned 16-bit buffer minus
    `0x7F7F`, scaled by `2^15`, fits even a 32-bit `long`. This preserves the
    intended arithmetic without undefined signed shifts.
-   Adapted the four missing reset assignments from the pinned nes-py
    `nes_emu/src/nes_emu/ppu.cpp`: initialize edge visibility, clear sprite-hit
    status, and zero the buffered PPUDATA byte. Keep RackNES's existing
    rendering-enable defaults, OAM contents, RAM, NTSC path, and reset timing.

The Blip fixes necessarily touch the bundled library because both defects
occur inside its sample reader; changing the host wrapper cannot make those
operations defined. Original LGPL notices are preserved. The PPU adaptation
is recorded in the existing MIT provenance inventory.

Validation on macOS arm64 with Apple Clang 21 and the prepared Rack tree:

-   Reproduced the original overlapping-copy failure before editing, then the
    uninitialized PPU boolean and negative signed shift as playback progressed.
-   `make -C tests -j2`: passed CV Genie, RackNES, and the newly mandatory
    `--audio-only` run with ASan and UBSan. Direct sample-reader checks compare
    bulk, single-sample, stereo, and `Blip_Reader` results on positive and
    negative PCM; PPU checks cover initial and repeated reset status/buffering.
-   `make -j4`: plugin build passed; existing SDK warnings remain.
-   `make -C tests BUILD=.build/audio SANITIZERS= .build/audio/racknes`, then
    `(cd tests && DYLD_LIBRARY_PATH=../../.. .build/audio/racknes --audio-only)`:
    passed. Both sanitized and uninstrumented fingerprints equal the recorded
    pre-fix values `f5146c03e0a6ceb2` and `d8915d435c9cf9a9` at their respective
    Blip clocks. These results cover the existing four sample-rate workloads,
    not all possible games, clock modulation, or mixer configurations.
-   `git diff --check`: passed. No manual Rack/listening session or other
    platform build was performed. No remaining sanitizer failure was observed
    in these workloads; broader spec 001 completion gates remain open.

### AxROM Integration: September 30, 2026

Mapper 7 is now enabled alongside IDs 0--3. This bounded implementation moves
AxROM ahead of the general parser/IRQ migration: AxROM has no IRQ or expansion
sound and fits the existing PRG/CHR and mirroring interfaces. The broader
implementation sequence remains required for IRQ-capable mappers. This does
not complete issue #1 (MMC3 and manual validation remain), issue #31, or this
specification.

Implemented contracts:

-   Adapt upstream `mapper_AxROM.hpp/.cpp`, bank-selection semantics, and test
    cases from the pinned revision; retain the MIT notice/source inventory.
-   Map one 32 KiB PRG window using bits 0--2, wrapping within complete banks.
    Bit 4 selects the lower/upper nametable page; fixed 8 KiB CHR RAM is
    writable. Switching and reads add no allocation, file I/O, or IRQ hooks.
-   Before legacy ROM construction, validate mapper-7 magic, complete header,
    exact file size and supported memory layout: power-of-two 32--256 KiB PRG,
    no CHR ROM, battery RAM, trainer, or four-screen request. Accept clean
    NTSC iNES headers (RAM-size byte 0/1) or NES 2.0 submappers 0/1/2 with
    explicit 8 KiB volatile CHR RAM, no PRG RAM, and no extra devices. Reject
    other layouts, extended size encodings, regions and submappers.
-   iNES and NES 2.0 submappers 0/1 follow upstream's no-conflict policy;
    submapper 2 ANDs each write with the byte in the old PRG bank. Legacy
    iNES cannot identify every board's conflict behavior.
-   Serialize the latch and full CHR RAM. Reject malformed mapper JSON before
    replacing an active game; derive mirroring from the restored latch even
    if saved picture-bus page offsets disagree. Preserve old mapper JSON.
-   Rebind AxROM cartridge copies to their own ROM storage and an explicitly
    supplied callback. Clones survive source destruction and own independent
    CHR RAM. CPU reset retains mapper state; ROM replacement initializes it;
    Rack module initialization removes the cartridge and SAVE slot.
-   Replace empty-vector element access in bus-RAM and scanline-sprite
    serialization with `data()` so mapper-7 full snapshots are well-defined.
    No APU implementation, scheduler, DMC callback, or MIX routing changes.

Validation on macOS arm64, Apple Clang 21, prepared Rack tree:

-   `make -C tests -j2`: ASan/UBSan pass. Tests cover 1/2/4/8 PRG banks, all
    byte write values, PRG-window boundaries, both nametable pages, CHR RAM,
    conflict variants, rejected images, malformed state, source destruction,
    CPU reset/ROM replacement, Rack SAVE/LOAD and independent live/backup
    patch restoration. A real APU DMC callback checks distinct PRG bank data.
-   The CPU-program audio fixture compares NROM/CNROM/AxROM sample by sample,
    including both pulse voices, triangle, noise and looping DMC. AxROM
    switches from bank 0 to bank 3 with identical code/sample data in four
    banks. All channels are nonzero and frame counts agree at 44.1, 48, 96,
    and 192 kHz (2,000 host samples each), with Blip clocks 1,789,773 Hz and
    768,000 Hz. Fingerprints remain `f5146c03e0a6ceb2` and
    `d8915d435c9cf9a9`, respectively.
-   `make -j4`: plugin build passes; existing SDK warnings remain.
-   `make -C manual`, `make -C whitepaper`, `make -C whitepaper source`:
    successful. Updated both manuals' mapper coverage and added a dated
    whitepaper paragraph with evidence in `whitepaper/sources.md`, preserving
    the historical source revision. Native editor compilation also passes.
    Rendered PDFs reviewed for table/page flow and references.
-   `git diff --check`: passes. No manual Rack/gameplay/listening session,
    other-platform build, or clock-extreme performance claim.

Remaining work includes mappers 4/5/9/69, general NES 2.0 parsing and legacy
mapper bank bounds, full-state validation/migration, generic clone/callback
ownership (including the unused native `Emulator::copy_from` path), picture
bus fixes and four-screen storage, CPU/IRQ contracts, and the wider audio and
manual acceptance matrix. The new validation covers mapper-7 state fields;
it does not make arbitrary CPU/PPU/APU or bus JSON safe. PPU flag serialization
and the pre-existing snapshot omissions still limit deterministic continuation.

### MMC2 Work In Progress: September 30, 2026

Source inspection found an uncommitted `src/nes/mappers/mapper9_MMC2.hpp`
and related changes in the cartridge factory, ROM mapper interface, picture
bus, emulator restoration, and PPU. This documentation commit does not include
those implementation changes or establish that they pass acceptance checks.

The draft adds bounded PRG/CHR bank selection, post-read FD/FE latches,
mapper-controlled mirroring, a restricted NTSC image validator, mapper JSON
validation, and cartridge-copy rebinding. The PPU draft caches background and
sprite pattern bytes for latch-sensitive reads and adds `chr_latch_fetches`
snapshot fields. Factory registration is present locally, but the required
mapper-specific audio and JSON evidence is still missing.

Before accepting this increment:

-   Add deterministic checks for both latch trigger ranges and old-bank read
    ordering, PRG/CHR boundaries, mirroring, PRG RAM, rejected images, and
    malformed mapper/fetch JSON without losing the active game or backup.
-   Verify background and sprite fetch ordering, hidden/covered sprites,
    reset, and mid-fetch restoration through the complete emulator. Check
    legacy mapper rendering and snapshots for regressions.
-   Run clone/source-destruction, five-channel audio/DMC, host SAVE/LOAD, and
    patch-reopen checks. Record plugin and sanitizer results using the
    commands above, then perform the required manual Rack checks.
-   Reconcile upstream attribution and user documentation with verified
    mapper-9 behavior before publishing a compatibility claim.

This spec-only update checks local links, referenced paths, existing validation
commands, and the complete documentation diff with `git diff --check`.
No plugin build, executable regression, manual Rack session, or performance
measurement was run for this update; earlier results remain attached to their
implementation increments above.

### MMC2 Integration: September 30, 2026

Mapper 9 (MMC2 / PxROM) is now enabled alongside IDs 0--3 and 7. Adapted
`mapper_MMC2.hpp/.cpp`, bank behavior, PPU fetch structure and MMC2 test cases
from the pinned nes-py revision; retained attribution and the MIT notice.
No mapper IRQ, expansion sound, generic address-observation framework or APU
rewrite is needed for this mapper.

Implemented contracts:

-   Switchable 8 KiB PRG at `$8000`, with the final three banks fixed at
    `$A000/$C000/$E000`. Mask PRG registers to four bits and CHR registers to
    five bits; wrap bank selections within complete available banks.
-   Four CHR registers feed independent lower/upper 4 KiB windows. Exact
    `$0FD8/$0FE8` and ranged `$1FD8-$1FDF/$1FE8-$1FEF` reads select FD/FE
    latches. The triggering read returns the old bank's byte. Observation and
    transition occur inside one `readCHR` call, so there is no pending latch
    phase between bus observation and reading. CHR writes do not latch.
-   The picture bus exposes a mapper capability for read-sensitive CHR.
    Only MMC2 uses the new rendering path: background pattern bytes are
    fetched once per tile row and sprite patterns once per selected row in
    OAM order, including clipped/covered sprites. Background fetches continue
    behind left-edge clipping. Both sprite heights and vertical flips are
    handled. Other mappers retain their existing pixel-read behavior.
-   Preserve fetched bytes in the new `ppu.chr_latch_fetches` JSON object so
    restoration does not repeat side-effecting reads. Validate these new
    fields before loading an emulator; older snapshots without the object
    retain the old mapper behavior. Mapper JSON stores PRG/CHR registers,
    mirroring and both latch states, with strict type/range checks before
    cartridge replacement. Recompute bus mirroring from restored mapper state.
-   Explicitly rebind MMC2 cartridge clones to copied ROM and a destination
    mirroring callback. The 8 KiB PRG RAM uses the existing CPU bus and
    `bus.extended_ram` JSON field, including DMA-page access. Fresh cartridge
    attachment now zeroes/resizes this bus RAM for all mappers instead of
    retaining a prior cartridge's bytes; JSON restoration follows attachment.
-   Accept exact-size NTSC iNES or NES 2.0 submapper-0 images with power-of-two
    32--128 KiB PRG ROM, 8--128 KiB CHR ROM, and 8 KiB PRG RAM. NES 2.0 must
    explicitly specify volatile or battery-backed PRG RAM consistent with
    the battery flag. Reject CHR RAM, trainers, four-screen layouts, extra
    devices, extended size encodings and unsupported regions/submappers.
    Battery RAM persists through Rack patch/SAVE state, not external files.

Validation on macOS arm64, Apple Clang 21, prepared Rack tree:

-   `make -C tests -j2`: ASan/UBSan pass. New checks cover all byte writes,
    4/8/16 PRG banks and 2/4/8/16/32 CHR banks, trigger boundaries and
    old-bank ordering, mirrors, PRG RAM, malformed images/state, clone source
    destruction, and independent live/backup Rack patch restoration.
    PPU checks preserve fetched bytes across snapshots and exercise clipped
    backgrounds, covered/hidden sprites, both heights, vertical flips and
    buffered PPUDATA. A synthetic CPU program verifies actual mapper writes,
    PPUDATA latch reads, PRG selection and PRG RAM through emulator callbacks.
-   The four-mapper NROM/CNROM/AxROM/MMC2 audio workload passes with all five
    voices nonzero and identical PCM/frame counts at 44.1/48/96/192 kHz,
    2,000 samples each, nominal emulation speed, and both Blip clocks.
    MMC2 switches code from PRG bank 0 to 3 while DMC reads its fixed windows;
    the test verifies the final PRG register. Fingerprints stay
    `f5146c03e0a6ceb2` (1,789,773 Hz) and `d8915d435c9cf9a9` (768,000 Hz).
-   `make -j4`: plugin build passes. Existing Rack SDK warnings remain.
-   Repeated the plugin build and ASan/UBSan suite from a clean source snapshot
    containing only this mapper change, excluding concurrent SRAM work:
    `make -C "$snapshot" -j4 RACK_DIR="$rack_dir"` and
    `make -C "$snapshot/tests" -j2 RACK_DIR="$rack_dir"`. Both pass with the
    same prepared Rack tree and unchanged PCM fingerprints.
-   `make -C manual`, `make -C whitepaper`, and `make -C whitepaper source`:
    pass. Both manuals list mapper 9 and its limits; the whitepaper's dated
    addendum and evidence inventory cover MMC2 without changing the historical
    source revision. Native editor compilation also passes. Rendered tables,
    page flow and references reviewed; no overfull boxes or unresolved final
    references. `git diff --check` and changed Markdown link checks pass.

Limits: the renderer still uses its coarse scanline model (sprite rows fetch
at the existing end-of-scanline evaluation), not hardware-exact PPU fetch
cycles. No commercial-game, manual Rack/listening, other-platform, or
clock-extreme validation was performed. The complete bus/PPU/JSON/clone and
IRQ migrations remain open, as do mappers 4/5/69, broad NES 2.0 support and
Pulsar/PR8 validation. Existing full-state omissions and PPU flag serialization
still limit deterministic continuation; validation of the new fetch fields
does not validate all older CPU/PPU/APU or bus JSON. This specification and
its associated issue gates remain IN PROGRESS.

### MMC3 Integration: September 30, 2026

Mapper 4 (standard MMC3B/C TxROM) is now implemented. This extends the MMC2
work above and the independently completed SRAM ownership migration. The
specification remains IN PROGRESS; enabling this mapper does not close the
broader hardware timing, state, manual validation, or mapper 5/69 gates.

Implemented contracts:

-   Adapt nes-py's pinned MMC3 bank/register behavior with 8 KiB PRG windows,
    both PRG/CHR inversion modes, aligned 2 KiB CHR pairs, two six-bit PRG
    bank registers, and bounded 1 KiB CHR banks. CHR RAM is banked too.
    Cartridge clones rebind ROM/callback ownership and retain mapper state.
-   Enable/write-protect PRG RAM independently of its saved capacity. Disabled
    reads use the existing zero/open-bus approximation. OAM DMA reads current
    cartridge ROM banks or a preallocated zero page for unmapped/disabled
    ranges, avoiding null-pointer DMA. Allocate four-screen nametable storage;
    mapper mirroring writes cannot override a four-screen board.
-   Accept exact-size NTSC iNES and NES 2.0 submapper-0 images: power-of-two
    32--512 KiB PRG ROM, 8--256 KiB CHR ROM or 8 KiB CHR RAM, and 8 KiB PRG
    RAM. NES 2.0 requires explicit RAM sizes consistent with battery metadata.
    Reject trainers, MMC6/other submappers, mixed CHR ROM/RAM, extended sizes,
    extra devices and alternate timing. Mapper 4 SRAM file transfers remain
    disabled; Rack patches and SAVE snapshots retain its RAM.
-   Emit timed PPU address observations separately from pixel reads. Include
    background, attribute/nametable, pre-render, sprite and empty sprite slots,
    even with one layer hidden. Capture sprite addresses in OAM order, including
    flipped 8x16 rows. CPU PPUADDR/PPUDATA operations observe address changes
    without manufacturing elapsed dots. Keep MMC2 post-read latch behavior.
-   Use a conservative ten-PPU-dot low filter before rising A12 clocks the
    MMC3B/C reload/decrement counter. Short fetch gaps do not double-clock;
    reversed tables and mixed-table sprites have explicit regression cases.
    This replaces upstream's read-observation count with elapsed time, but
    remains an approximation: real hardware uses M2 edges. CPU/PPU phase,
    odd-frame anomalies, MMC3A and MC-ACC behavior are not claimed accurate.
-   Keep mapper IRQ asserted while CPU I is set, until acknowledgement at
    `$E000`; `$E001` enables future counter events. Poll mapper and APU levels
    together at instruction boundaries, with edge-latched NMI priority and
    DMA/instruction stalls respected. Acknowledging either device cannot clear
    the other. The APU notifier only signals schedule changes and is no longer
    an immediate CPU interrupt callback. Frame/DMC status is polled through
    `earliest_irq()` without acknowledging `$4015`.
-   Correct PHP/PLP/RTI/BRK/IRQ/NMI stack status encoding without reinterpreting
    legacy numeric CPU JSON flags. Save pending NMI, mapper registers/RAM/IRQ
    and partial A12-filter history, and sprite fetch addresses. Validate new
    mapper/fetch fields before replacement. Fix three PPU snapshot booleans
    previously serialized from `scanline`. Refresh bundled APU IRQ scheduling
    after snapshot flag restoration; the prior code left a pending frame IRQ
    invisible when the DMC schedule was unchanged.

Validation on macOS arm64 with Apple Clang 21 and the prepared Rack tree:

-   `make -C tests -j2`: ASan/UBSan passes. Tests cover all register bytes,
    supported PRG/CHR bank counts, protection, DMA bank reads, mirroring,
    header/state rejection, source destruction after clone, partial-filter
    restoration, zero reload, enable/disable/acknowledge, and both sprite
    sizes. A synthetic CPU program services raster IRQs through the full
    emulator and round-trips live/SAVE Rack patch state.
-   Focused CPU/APU checks pass for masked IRQ retention, NMI priority, OAM
    stalls, PHP/PLP/BRK/RTI status bytes, independent frame/DMC/mapper pending
    state and acknowledgements, restored APU IRQs, and DMC refills after PRG
    bank changes.
-   Five-mapper PCM/frame comparison passes at 44.1/48/96/192 kHz, 2,000 host
    samples each, nominal emulation speed and both Blip clocks. All five voices
    are nonzero. MMC3 changes CHR registers while DMC reads the fixed window;
    the separate DMC test switches its live PRG bank. Fingerprints remain
    `f5146c03e0a6ceb2` (1,789,773 Hz) and `d8915d435c9cf9a9` (768,000 Hz).
-   `make -j4`, `make -C manual`, `make -C whitepaper`, and
    `make -C whitepaper source` pass. Native whitepaper compilation passes.
    The manuals and dated whitepaper addendum describe the supported subset
    and limits; the historical manuscript revision stays pinned. Rendered
    pages, links and `git diff --check` are reviewed before commit.

Compatibility: CPU interrupt and stack-status corrections affect all mappers;
IRQ-dependent programs may now behave or sound differently. The characterized
non-interrupt audio workload remains identical. No oscillator, resampling,
Blip clock, host cycle quantization or frame-output timing was changed.

Remaining: hardware-exact M2/PPU timing, commercial-game and manual Rack/listening
checks, other platforms and clock extremes, broader parser/JSON validation,
complete emulator clone rebinding, omitted full-state fields and audio-buffer
continuation, DMC DMA timing, and mappers 5/69. These remain open rather than
being inferred from successful synthetic tests.

[upstream]: https://github.com/Kautenja/nes-py/tree/301da52f7f75de380e6e195fd36621c3d5b03757
[factory]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/mapper_factory.cpp
[upstream-bus]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/main_bus.cpp
[upstream-emulator]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/emulator.cpp
[upstream-cartridge]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/cartridge.cpp
[mapper-api]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/include/nes_emu/mapper.hpp
[upstream-ppu]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/ppu.cpp
[upstream-tests]: https://github.com/Kautenja/nes-py/tree/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/test/nes_emu
[mmc3-fix]: https://github.com/Kautenja/nes-py/commit/c5231f8
[mmc3-stabilize]: https://github.com/Kautenja/nes-py/commit/da7702c
[local-emulator]: ../src/nes/emulator.hpp
[apu]: ../src/nes/apu.hpp
[rom]: ../src/nes/rom.hpp
[cartridge]: ../src/nes/cartridge.hpp
[main-bus]: ../src/nes/main_bus.hpp
[picture-bus]: ../src/nes/picture_bus.hpp
[ppu]: ../src/nes/ppu.cpp
[module]: ../src/RackNES.cpp
[nes-apu]: ../src/nes/apu/Nes_Apu.h
[fme7-apu]: ../src/nes/apu/Nes_Fme7_Apu.cpp
[license]: ../LICENSING.md
[manual-guide]: ../manual/README.md
[racknes-manual]: ../manual/RackNES/manual.tex
[manual-roms]: ../manual/RackNES/sections/roms.tex

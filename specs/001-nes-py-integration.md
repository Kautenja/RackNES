# nes-py Emulator Integration

This specification defines a selective integration of nes-py's newer native
emulator into RackNES. The objective is broader cartridge compatibility and
targeted correctness improvements while retaining RackNES's musical timing,
five-channel APU, NTSC display, CV behavior, and saved patches. This document
proposes implementation work; it does not claim that the port is implemented
or validated in Rack.

Status: PLANNED

Created: September 30, 2026

## Baseline And Evidence

Prepared September 30, 2026 against these source revisions:

-   RackNES: `b783a8e473f00242773e3fccbd4ed9e1a6ae17d5`, manifest version
    `2.2.0`. The emulator factory implements mapper IDs 0 through 3.
-   nes-py: [commit 301da52f7f75de380e6e195fd36621c3d5b03757][upstream], the
    `master` tip retrieved for this review. Pin implementation comparisons to
    this revision rather than a moving branch or package version.
-   The working tree already contains independent RackNES/CV Genie fixes and
    untracked standalone regression checks in `tests/`. Those changes are
    outside this specification commit. Reconcile with their final committed
    form before implementation; do not overwrite or silently absorb them.

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
and diagnostics. RackNES's current factory enum has an 8-bit underlying type;
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
    including held buttons and a partially read serial stream. Correct the
    existing controller loader's boolean conversion of byte fields as part
    of migration coverage.
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
    user-facing mapper/limitation documentation and changelog, and record
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

For implementation, extend or reconcile the existing in-progress standalone
`tests/Makefile` so that `check` runs the new emulator cases as well as the host
checks. At the reviewed committed baseline that harness is not tracked; the
commands below are implementation acceptance requirements, not claims that a
test suite already exists in the committed repository. A small assertion-based
runner is sufficient. Do not add a root `make test` fiction or rely on `-DTEST`.

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

## Specification Review Record

This specification was based on source inspection, a pinned upstream checkout,
and comparison of the current native implementations and tests. No upstream
code was imported, and no plugin build, executable emulator regression,
performance benchmark, or manual Rack session was performed for this
documentation-only change. Implementation gates above remain open.
Documentation validation checks relative links, pinned upstream source paths,
command prerequisites, Markdown formatting, and the complete specification
diff before commit.

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
[license]: ../LICENSE.md
[manual-guide]: ../manual/README.md

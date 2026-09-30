# SRAM Import And Export

Implement [#52: Serialization of SRAM][issue] so a user can compose in another
emulator and bring cartridge save data into RackNES for processing.

Status: COMPLETE

Created: September 30, 2026

Completed: September 30, 2026

## Goal And Behavior

Add **Import SRAM...** and **Export SRAM...** to RackNES's context menu for
supported cartridges. A raw `.sav` file contains persistent cartridge RAM,
not CPU/PPU/APU state or a RackNES snapshot. Export produces a coherent copy;
import changes only the cartridge's persistent memory. The user can then use
the game's normal load command or NES reset if the game needs it.

For example, export a tracker song from a compatible emulator, import the
matching `.sav` into the same ROM revision in RackNES, and play that song.
An incorrect file length leaves the running game and all snapshots unchanged.

## Current Code And Dependencies

Before this increment, [MainBus](../../src/nes/main_bus.hpp) owned fixed 8 KiB
extended RAM;
[ROM](../../src/nes/rom.hpp) gates it on the battery flag. That array is already
stored in patch JSON, but the module has no external save-file service.
[Spec 001](../001-nes-py-integration.md) moves RAM ownership to cartridge mappers
and defines NES 2.0 volatile/nonvolatile sizes. Reuse that metadata and memory
access boundary; do not make raw MainBus storage the public file interface.

First support reviewed, unambiguous 8 KiB battery-RAM boards among mapper IDs
0--3. Explicitly decline volatile-only, unsupported, or ambiguous layouts.
Add larger/banked layouts only after spec 001 provides their memory contracts
and an interchange fixture establishes byte order. Do not block the initial
8 KiB feature on every future mapper. Tracker coverage depends on each
tracker's actual board support, not just its header being recognized.

## Requirements

-   Preserve existing module IDs, `emulator`/`backup` JSON, and legacy RAM
    restoration. Import does not replace ROMs, reset the NES automatically,
    or alter existing snapshots; loading an older snapshot can undo an import.
    Explain that behavior in menu help and the manual.
-   Use exact-size raw bytes with no private header. Never truncate, pad,
    reinterpret a full save state, or import into the wrong memory domain.
    Report the expected size when rejecting a file. Raw files do not identify
    their ROM, so document that users must match game and revision.
-   Keep dialogs, disk reads/writes, allocation for file handling, and error
    presentation off the engine thread. Preallocate bounded transfer storage
    for each supported layout. The engine copies only the supported RAM size
    at a defined process boundary and never waits for a UI/worker operation.
-   Specify request/result ownership, queue capacity, and overflow behavior
    before implementation. Use a bounded handoff with explicit publication;
    an unsynchronized shared path, flag, vector, or pointer is insufficient.
    Publish an acknowledgement before reusing a buffer.
-   Bind requests to a cartridge generation. Cancel stale work on ROM
    replacement, patch restoration, module reset, or destruction. No worker
    may dereference a deleted module. Import/export requests while hung must
    still complete at the normal control boundary.
-   Validate the entire import before applying it. File errors, stale requests,
    and cancellation leave live RAM unchanged. Export obtains one engine
    snapshot before writing and uses a temporary sibling file plus replacement
    so a failed write does not destroy a previous save. Use the ordinary save
    dialog's overwrite confirmation.
-   Add manual instructions and changelog coverage. Do not claim all existing
    processing is real-time safe: ROM loading and JSON snapshots already have
    separate engine-thread limitations.

## Non-Goals

Automatic battery persistence, emulator-specific save-state formats, volatile
RAM export, arbitrary memory editing, new mappers, and multiple snapshot slots
are outside this increment.

## Acceptance Criteria

- [x] Exact-size import/export round-trips all bytes with a synthetic fixture;
    CPU, PPU, APU, controllers, mapper registers, and backups stay unchanged.
- [x] Empty, short, long, unreadable, and unsupported files fail safely;
    failed exports preserve an existing destination.
- [x] Concurrent request, hang, replacement, reset, restore, and destruction
    cases obey the bounded handoff and never apply to a stale cartridge.
- [x] Legacy patches retain their RAM and load normally after the feature.
- [x] An identified external emulator's raw save imports into RackNES and a
    RackNES export loads there, with tool versions, ROM hash, size, and
    observed game/tracker data recorded. A self-round-trip is insufficient.
- [x] Headless menu checks pass for empty cartridges and browser previews;
    documentation states supported layouts and the distinction from SAVE/LOAD
    snapshots. Native dialog verification is retained as issue #52 follow-up,
    per the completion decision below.

## Validation And Completion Evidence

Implement checks in the [existing assertion harness](../../tests/README.md).
From the repository root with the prepared Rack tree at `../..` and the
[documented prerequisites](../../CONTRIBUTING.md#development-and-testing):

```shell
make -j4
make -C tests -j2
make -C manual
git diff --check
```

Use the same absolute `RACK_DIR` for both C++ builds when using an external
SDK. Inspect the rendered manual and perform the interoperability and Rack
checks above. Record date, commits, fixtures, platforms, commands, results,
and limitations here before marking complete and archiving. The implementation
and validation results are recorded below; issue #52 remains open for native
dialog checks and requester feedback.

[issue]: https://github.com/Kautenja/RackNES/issues/52

## Transfer Contract

The UI and engine share one preallocated 8,192-byte mailbox per module.
Capacity is one operation, including its dialog and file handling; additional
requests fail as busy. States are Idle, Preparing (UI owns bytes), Pending
(engine owns bytes), and Done (UI owns the acknowledged result). Release/acquire
publication transfers ownership; only the UI returns Done to Idle. The engine
never waits, allocates, opens a file, or presents errors for this service.

The engine services the mailbox after divided CV controls and expander writes,
before Hang returns. An emulator generation changes on ROM load, reset,
restoration, or removal. Each request captures the published generation before
opening its dialog; the engine rejects mismatches before touching RAM. The UI
also checks generation before committing an exported file. Module destruction
closes a shared service object; menu callbacks retain only that object, never
a module pointer. File operations run synchronously on the UI thread; there is
no worker and no worker join on the engine thread. Export completion linearizes
at the final generation check before atomic replacement.

Cartridge mappers own the fixed legacy RAM window, while `bus.extended_ram`
remains the serialized compatibility field. The initial file boundary requires
clean NTSC headers, a declared single 8 KiB battery PRG-RAM domain, supported
ROM sizes, no trainer/four-screen layout, no CHR NVRAM or extra devices, and
mapper 0--3. Ambiguous legacy headers without an explicit RAM size are declined.
This is a narrow prerequisite slice of spec 001, not its general memory/parser
migration or support for banked SRAM.

## Implementation And Validation: September 30, 2026

Implemented the mapper-owned fixed RAM window and exact-size transfer API,
SRAM context menu, shared single-slot service, generation invalidation, UI-only
file operations, and atomic sibling-file replacement. The existing
`emulator`, `backup`, and `bus.extended_ram` JSON fields retain their meanings.
Legacy RAM restoration now checks the decoded length before copying; malformed
lengths cannot resize the CPU-visible memory window. Mapper copy constructors
copy the new RAM array from the source rather than from an uninitialized self.
The legacy general clone/callback ownership limitations of spec 001 are not
claimed fixed by this increment.

The new exact-state regression exposed an existing uninitialized serialized
APU byte: the bundled snapshot writer omits triangle phase. Value-initialize
the first-party snapshot wrapper before filling it. This gives that omitted
field a deterministic zero; it does not add triangle-phase restoration or
resolve the other historical snapshot omissions. No bundled audio library
implementation was changed for SRAM support.

Header eligibility follows [iNES RAM metadata][ines] and the distinction
between fixed and banked RAM on [MMC1 boards][mmc1]. Legacy RAM-size byte zero
is deliberately declined because it does not explicitly establish the save
size. NES 2.0 requires byte 10 equal to `0x70`, byte 11 equal to zero with CHR
ROM or `0x07` without it, submapper zero, and no extended ROM-size fields or
extra devices. CHR NVRAM and mixed volatile/persistent PRG RAM are declined.
This does not expand the loader's general NES 2.0 compatibility.

Validation used macOS arm64, Apple Clang 21, and the prepared Rack Free 2.6.0
tree at `../..`. Work was uncommitted; the shared checkout advanced to
`8013d8f` for the independent MMC2 work during this task and also contained
ongoing spec 001 edits. Those changes were preserved. No commit or push had
been requested at this initial implementation stage; commit preparation and
completion are recorded below.

-   `make -j4`: passed. Existing Rack SDK deprecation warnings remain.
-   `make -C tests -j2`: passed with AddressSanitizer and
    UndefinedBehaviorSanitizer. The new assertions cover IDs 0--3 with iNES
    and NES 2.0 headers, every save byte, unchanged other serialized state,
    unchanged backups, LOAD undo, legacy JSON, invalid lengths, rejected
    layouts, one-slot overflow and acknowledgement, 1,000 concurrent
    import/export pairs, Hang, reset/replacement/restore/removal, and retained
    UI service objects after destruction in Preparing, Pending, and Done.
-   File assertions cover empty/short/long/missing input, overwrite, stale
    export, nonexistent parent, replacement failure, and a forced partial
    write using POSIX `RLIMIT_FSIZE`. The existing regular-file destination
    survived the forced failure byte for byte. No Windows or Linux run was
    performed; the Windows UTF-16 path and replacement branch remains untested.
-   Audio assertions pass with unchanged fingerprints `f5146c03e0a6ceb2`
    (1,789,773 Hz Blip clock) and `d8915d435c9cf9a9` (768,000 Hz), covering
    the existing 44.1/48/96/192 kHz workloads. No performance claim is made.
-   `make -C manual`: passed. Reviewed the 14-page RackNES PDF, including the
    full SRAM page, contents, page flow and references. No panel geometry or
    runtime SVG changed, so panel captures were not regenerated.
-   Headless menu enumeration passed for a supported cartridge, an empty
    module, and a null preview service. An isolated Rack Free 2.6.0 session
    at 48 kHz loaded the original test ROM and displayed the production panel
    with Hang set. Native dialog actions could not be exercised: computer-use
    read the window but repeated click attempts returned `noWindowsAvailable`.
    The temporary test process was closed. An earlier isolated Rack Pro 2.6.3
    attempt required activation; the ordinary user profile was not changed.
-   `git diff --check`: passed. At this stage, no issue was closed and the spec
    remained IN PROGRESS pending native dialog/overwrite/cancel checks. The
    later completion decision below transfers those checks to issue #52.

### Independent Save Interchange

Built [FCEUmm][fceumm] at
`7a542dab1e87679921962a9f056186eca425c0c2`, reporting `(SVN) 7a542da`, with
`make -f Makefile.libretro -j4 platform=osx arch=arm64`. The optional
[interchange driver](../../tests/sram_interop.py) uses its actual libretro
`RETRO_MEMORY_SAVE_RAM` domain, which reports 8,192 bytes. It is independent
of RackNES's cartridge storage and serialization implementation.

From `tests/`, after the assertion build:

```shell
DYLD_LIBRARY_PATH=../../.. python3 sram_interop.py /tmp/racknes-004-fceumm/fceumm_libretro.dylib
```

The script generates an original mapper-1 homebrew song-loader ROM. FCEUmm
executes it with a synthetic song record in SRAM, exports the raw memory
through the frontend, and RackNES imports the file and executes the ROM.
RackNES's export is then loaded into a newly loaded FCEUmm cartridge. Both
cores must load these sixteen note numbers into CPU RAM at `$0010--$001F`:

```text
48 52 55 60 55 52 48 43 45 48 52 57 52 48 45 40
```

All 8,192 bytes match in both directions, including the nonuniform remainder
of the save. Hashes printed by the passing run:

-   ROM SHA-256:
    `6b1390548acf4b13662837f8ac5efc3c0a26de1c70d478cbebd3202cc6d22b18`
-   Save SHA-256:
    `37e9236597251c2cb7b63bf0d60ef0b7b4245dc776ee6c4d655ff357283af685`

Fixtures remain in ignored `tests/.build/sram-interop/`. This establishes raw
format and CPU-visible byte order for the synthetic song loader, not tested
compatibility with any commercial game or third-party tracker. Tracker support
still depends on its actual mapper and memory layout.

[ines]: https://www.nesdev.org/wiki/INES
[mmc1]: https://www.nesdev.org/wiki/INES_Mapper_001
[fceumm]: https://github.com/libretro/libretro-fceumm/tree/7a542dab1e87679921962a9f056186eca425c0c2

### Commit Preparation: September 30, 2026

At the user's request, isolated the SRAM changes from the concurrent MMC3
work, including overlapping edits in shared source files. Exported the exact
prospective commit based on `8013d8f` to a temporary source directory and
validated it independently with the same absolute Rack SDK path for both
builds:

```shell
make -j4 RACK_DIR=/absolute/path/to/Rack
make -C tests -j2 RACK_DIR=/absolute/path/to/Rack
```

Both passed, including ASan/UBSan and the existing audio fingerprints. The
FCEUmm interchange script also passed against this isolated version with the
same ROM/save hashes and observed notes recorded above. Reviewed the complete
SRAM-only diff and ran `git diff --cached --check` before committing. Native
file-dialog verification remained open at that commit, which did not mark the
spec complete or include unrelated MMC3 work.

## Completion Decision: September 30, 2026

The implementation was committed as `82bab6a` and pushed to `origin/techreport`.
At the user's explicit request, mark this implementation spec COMPLETE and
archive it. [Issue #52][issue] remains the place to track requester feedback,
tracker-specific compatibility, and native dialog/overwrite/cancel checks.
The remaining interactive check is no longer a prerequisite for archiving this
spec; no additional manual test or release publication is claimed.

The delivered scope is raw, exact-size 8 KiB SRAM interchange for the reviewed
mapper 0--3 layouts, with the regression and independent FCEUmm evidence above.
Larger/banked layouts and additional tracker support still require their own
memory contracts and validation. The issue update invites the requester to
try their workflow and report the tracker, emulator, save size, and observed
behavior. Keep the issue open while collecting that feedback.

Archival validation is documentation-only: update inbound and relative links,
check that local targets exist, and review the complete diff with
`git diff --check`. The successful implementation builds and tests above
remain the source validation evidence; no C++ rebuild is needed for this move.

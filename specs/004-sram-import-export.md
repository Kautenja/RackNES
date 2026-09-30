# SRAM Import And Export

Implement [#52: Serialization of SRAM][issue] so a user can compose in another
emulator and bring cartridge save data into RackNES for processing.

Status: PLANNED

Created: September 30, 2026

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

[MainBus](../src/nes/main_bus.hpp) currently owns fixed 8 KiB extended RAM;
[ROM](../src/nes/rom.hpp) gates it on the battery flag. That array is already
stored in patch JSON, but the module has no external save-file service.
[Spec 001](001-nes-py-integration.md) moves RAM ownership to cartridge mappers
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

- [ ] Exact-size import/export round-trips all bytes with a synthetic fixture;
    CPU, PPU, APU, controllers, mapper registers, and backups stay unchanged.
- [ ] Empty, short, long, unreadable, and unsupported files fail safely;
    failed exports preserve an existing destination.
- [ ] Concurrent request, hang, replacement, reset, restore, and destruction
    cases obey the bounded handoff and never apply to a stale cartridge.
- [ ] Legacy patches retain their RAM and load normally after the feature.
- [ ] An identified external emulator's raw save imports into RackNES and a
    RackNES export loads there, with tool versions, ROM hash, size, and
    observed game/tracker data recorded. A self-round-trip is insufficient.
- [ ] Menus work with empty cartridges and browser previews. Documentation
    states supported layouts and the distinction from SAVE/LOAD snapshots.

## Validation And Completion Evidence

Implement checks in the [existing assertion harness](../tests/README.md).
From the repository root with the prepared Rack tree at `../..` and the
[documented prerequisites](../CONTRIBUTING.md#development-and-testing):

```shell
make -j4
make -C tests -j2
make -C manual
git diff --check
```

Use the same absolute `RACK_DIR` for both C++ builds when using an external
SDK. Inspect the rendered manual and perform the interoperability and Rack
checks above. Record date, commits, fixtures, platforms, commands, results,
and limitations here before marking complete and archiving. Planning has not
implemented or validated SRAM interchange; keep #52 open until its gates pass.

[issue]: https://github.com/Kautenja/RackNES/issues/52

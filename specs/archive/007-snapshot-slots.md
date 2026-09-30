# Multiple Snapshot Slots

This archived proposal for [#12: Additional Save States][issue] described
capturing and recalling several game moments using the existing SAVE/LOAD
controls and a voltage-controlled slot selector.

Status: ABANDONED

Created: September 30, 2026

Abandoned: September 30, 2026

## Disposition

Multiple snapshot slots are not planned. The maintainer chose to abandon
this proposal and close #12 as not planned. The original design and unchecked
acceptance criteria remain below for historical context; archival does not
claim implementation or validation of the feature.

Archival checks on September 30, 2026: relative link targets, the archive
path, and abandoned status passed scripted checks; `git diff --check` passed.
Builds, regression tests, and manual Rack checks were not run for this
documentation-only disposition.

## Goal And Behavior

Provide eight slots, numbered 1--8. A context-menu selection chooses the slot
when the new slot CV input is unpatched. When connected, a clamped 0--10 V
input selects `min(7, floor(8 * voltage / 10))` as the zero-based slot index.
Non-finite input falls back to the saved menu selection. Show the effective
slot and whether it is occupied without reading mutable JSON from the UI.

For example, save the title screen in slot 1 and a gameplay phrase in slot 2.
With 0 V select slot 1; with 1.25 V select slot 2. LOAD restores the selected
moment. Loading an empty slot is a no-op and does not reset the running game.
Selecting a slot alone neither saves nor loads it.

## Current Code And Dependencies

[RackNES.cpp](../../src/RackNES.cpp) owns one reference-counted `backup` JSON
object. It acquires snapshot controls every 16 samples, in save/reset/restore
order. Successful ROM replacement and Rack module reset clear the backup;
NES reset does not. Patch JSON stores `emulator` and optional `backup`.

[Spec 001](../001-nes-py-integration.md) owns core snapshot validation, migration,
and transactional restoration. Reuse that work rather than adding a second
emulator-state parser. Slot selection and storage can be implemented first;
safe restore and malformed-state acceptance gates must pass before shipping.
This is larger than a small menu change and is not required for the SRAM file
feature in [spec 004](../004-sram-import-export.md).

## Requirements

-   Keep eight independently owned snapshots. Select the effective slot once
    per existing control-acquisition tick, before SAVE, RESET, and LOAD. All
    coincident triggers act on that same slot in the current order. A save
    failure preserves the old slot; no code may weaken Jansson ownership.
-   Default menu selection to slot 1. Queue menu edits to the engine, publish
    bounded occupancy/selection status back to the UI, and keep dialogs,
    drawing, and file I/O out of processing. No UI callback may mutate a live
    snapshot or inspect JSON owned by the engine.
-   Append the slot CV input ID after existing input IDs. Do not renumber
    parameters, ports, or lights. Preserve panel width and current control
    positions; review a production panel layout for the new jack and slot
    indicator before implementation. If that layout cannot remain readable,
    revise this spec before changing the panel width or adopting an expander.
-   Keep existing SAVE/LOAD buttons and gates, divider, hang behavior, trigger
    thresholds, and save/reset/restore ordering. Connected slot CV overrides
    the menu only while connected; disconnect returns to its saved selection.
-   Define additive module JSON fields `snapshot_slots_version: 1`,
    `snapshot_slots` (eight object-or-null entries), and `selected_slot` (0--7).
    New-format data takes precedence when its version is recognized. Legacy
    `backup` alone migrates into slot 1; the remaining slots start empty.
    For new saves, retain `backup` as a copy of slot 1 for older readers.
    Older RackNES cannot preserve slots 2--8 and must not be advertised as
    supporting the new feature.
-   Validate versions, array length, element types, selected index, and each
    state's cartridge association before publication. Unknown versions or
    malformed new data must not silently fall back to a stale `backup`.
    Reject a malformed bank atomically while preserving the active bank;
    a fresh module retains its safe empty bank. Document the error to the user.
-   Successful ROM replacement clears all slots. Failed replacement preserves
    all slots. Rack module reset clears slots and selection; NES reset leaves
    them intact. Reject a slot associated with another ROM before mutating
    the emulator. Patch reopen must preserve every valid slot and selection.
-   Bound total snapshot storage. Characterize worst-case serialized sizes
    for supported cartridges, choose and document an explicit byte budget
    before implementation, and enforce it on capture and patch restoration.
    Eight entries alone do not bound attacker-supplied or mapper-dependent
    payload sizes. A budget failure preserves the previous bank and live game.
-   Existing captures/restores allocate and serialize on the engine thread.
    Do not multiply that work by eight per trigger or claim real-time safety.
    Capture only the selected slot; bound bookkeeping, and record measured
    capture/restore pauses with identified workloads. Off-thread snapshotting
    requires a separate safe state-transfer design, not concurrent JSON access.

## Phases And Non-Goals

1.  Implement bounded slot ownership, legacy migration, context-menu selection,
    status publication, and deterministic trigger tests.
2.  Add the slot CV input and visible indicator with compatibility checks,
    production captures, manual changes, and actual Rack validation.

New expander modules, automatic sequencing, sample-accurate audio loops,
external save-state interchange, and unlimited banks are outside this spec.
A menu-only phase is useful but does not finish the CV-controlled acceptance
criteria or close #12.

## Acceptance Criteria

- [ ] All eight slots retain distinct CPU/RAM/APU state and restore only when
    LOAD is triggered; empty LOAD and selection alone are no-ops.
- [ ] CV boundaries, out-of-range/non-finite values, connection changes, hang,
    and coincident SAVE/RESET/LOAD follow the defined one-tick selection rule.
- [ ] Legacy patches migrate; new banks round-trip; corrupt, oversized,
    mismatched-ROM, and unknown-version banks fail without live-state loss.
- [ ] Replacement/reset/destruction and repeated overwrite release every JSON
    reference once, under the existing sanitizer checks.
- [ ] Existing input IDs, module width, controls, and old cable destinations
    remain intact. Both themes and module browser previews work.
- [ ] In Rack, a sequenced CV selects at least three occupied slots and LOAD
    recalls the intended moments with sound; save/reopen preserves them.
    Document timing limitations, measured pauses, and the memory budget.
- [ ] Both phases and documentation pass before #12 is closed as implemented.

## Validation And Completion Evidence

Extend [the existing harness](../../tests/README.md). From the repository root
with the prepared Rack tree at `../..`, the [build prerequisites][build],
and [production capture prerequisites][capture]:

```shell
make -j4
make -C tests -j2
make -C tools/capture screenshots
make -C manual
git diff --check
```

Inspect refreshed panels and rendered PDFs. Check a real Rack session at
44.1 and 48 kHz, including the clock extremes, five individual outputs, MIX,
SAVE/LOAD, CV selection, patch reopen, and both reset types. Record dates,
commits, fixture hashes, actual commands/platforms, byte budget, timing
measurements, manual results, and limitations here before archival. Planning
does not establish implementation, real-time safety, or game compatibility.

[issue]: https://github.com/Kautenja/RackNES/issues/12
[build]: ../../CONTRIBUTING.md#development-and-testing
[capture]: ../../tools/capture/README.md

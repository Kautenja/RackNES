# CV Genie Readback

Implement [#51: Read functionality for CV Genie][issue] with an Output Genie
that observes internal RAM and the base 2A03 APU's last written register values.
This also owns the remaining output work from [#6: CV RAM Manipulation][origin].

Status: PLANNED

Created: September 30, 2026

## Goal And Behavior

A user selects a game-memory value such as player speed and receives CV as the
value changes. A game-independent 2A03 register menu exposes the values the
program writes to its sound chip. An internal-RAM address mode allows observing
unmapped homebrew without creating a curated game map.

For example, byte values 0, 128, and 255 produce 0 V, approximately 5.02 V,
and 10 V. Observing a register never acknowledges an IRQ, advances a controller
stream, changes audio, or triggers another emulated bus side effect.

## Current Code And Dependencies

[CVGenie.cpp](../src/CVGenie.cpp) contains unfinished Output Genie code, but
[plugin.cpp](../src/plugin.cpp) does not register it. Processing currently
supports only the Input Genie write path. Existing scaffolding is not proof
of a working output module or safe read protocol.

Input Genie must remain directly to RackNES's right with its eight existing
address/value pairs and address-zero sentinel unchanged. Place Output Genie
directly to RackNES's left, permitting both expanders simultaneously. Reserve
the existing unregistered `OutputGenie` slug for the completed module, and
verify the widget, metadata, and resource names before registration.

Coordinate emulator interfaces with [spec 001](001-nes-py-integration.md).
Readback of 2A03 registers is not Sunsoft/MMC5 expansion-audio synthesis or
support for any particular game's mapper.

## Requirements

-   Provide eight independent output rows. Each row selects a curated internal
    RAM entry, a typed internal address `0x0000`--`0x07FF`, or a named writable
    base APU register. Use explicit source kinds and validity flags so address
    zero is a valid read without changing Input Genie's sentinel.
-   Expose a read-only **2A03 Registers** source menu independently of games.
    Valid selections cover implemented registers in `$4000`--`$4013`, plus
    `$4015` and `$4017`; exclude unused `$4009` and `$400D`. Show address and
    name, and describe values as last writes, not instantaneous waveforms,
    audible pitch, hardware-readable status, or calibrated 1 V/octave CV.
-   Mirror writes in first-party APU integration before forwarding each write
    once to the sound library. Never call CPU bus reads or `read_status()` to
    sample these sources. Internal RAM reads likewise use bounded direct
    observation, not a generic side-effecting bus accessor.
-   Track validity per mirrored register. Cold load/reset initializes the
    mirror consistently with the actual APU reset policy; absent legacy data
    is unknown until written unless exact reconstruction is established.
    Serialize validated mirror bytes and validity for new snapshots. A missing
    legacy field must not invent values or prevent the patch from loading.
-   Sample at the existing 16-host-sample control cadence and hold voltages
    between updates. Use `10 * byte / 255` V for every source in this first
    version, with tooltips documenting raw-byte scaling. Hang holds the last
    published values; disconnect, missing ROM, invalid source, and reset clear
    affected output channels to 0 V. Reconnection cannot replay stale data.
-   Design separate, fixed-size, versioned request and response buffers on the
    left expander link. Document producer/consumer ownership, Rack flips,
    latency, validity, and disconnect handling before enabling the model.
    Budget eight bounded observations per update, without allocation, blocking,
    logging, or UI access on the engine path. UI selections require an explicit
    handoff like the existing Input Genie selection mechanism.
-   Preserve all existing slugs, IDs, game indices, input behavior, and patch
    JSON meanings. Save Output Genie source kinds and stable identifiers with
    bounds checks; corrupt/unknown selections become unassigned safely.
-   Provide both panel themes, browser-preview safety, port names, tooltips,
    manifest registration, manual instructions, and production captures.

## Phases And Non-Goals

1.  Add and test observation APIs and APU write mirrors, including old/new
    snapshot behavior. Do not register the incomplete output widget.
2.  Implement the left-side protocol, eight RAM outputs, address selection,
    persistence, and disconnect/reset behavior. Verify simultaneous Input
    Genie writes before enabling the module.
3.  Add the 2A03 menu and mirror sources, finish panels/manuals, and validate
    audio equivalence with readback enabled and disabled.

Arbitrary writes, full CPU-address-space peeks, memory scanning, derived note
or pitch CV, expansion sound registers, expander chains, and new mappers are
outside this spec. New mapper support remains in spec 001.

## Acceptance Criteria

- [ ] RAM endpoints including address zero and all byte-to-voltage endpoints
    pass deterministic checks; out-of-range and malformed selections are safe.
- [ ] Eight independent rows and both expanders operate through actual buffer
    flips with documented update cadence and no stale messages after detach.
- [ ] Identical CPU fixtures yield identical APU/IRQ/controller behavior and
    PCM with observation enabled and disabled. Each register write is forwarded
    exactly once. Reading `$4015` as an observation never clears IRQ status.
- [ ] Cold load, reset, hang, ROM replacement, patch reopen, and SAVE/LOAD
    preserve the specified mirror, output, and selection behavior.
- [ ] An existing legacy Input Genie patch is unchanged, and the new output
    patch schema rejects invalid types, addresses, indices, and versions safely.
- [ ] A real Rack patch uses RAM and 2A03 outputs to modulate other modules;
    both themes, previews, adjacency, audio, and save/reopen are verified.
- [ ] #51 remains open until RAM/address and 2A03 phases both pass. Closing #6
    as consolidated is administrative and does not claim readback is implemented.

## Validation And Completion Evidence

Extend the [existing tests](../tests/README.md), including protocol fixtures
and CPU-program audio comparisons. From the repository root with the prepared
Rack tree at `../..` and the [build](../CONTRIBUTING.md#development-and-testing)
and [capture prerequisites](../tools/capture/README.md):

```shell
make -j4
make -C tests -j2
make -C tools/capture screenshots
make -C manual
git diff --check
```

Extend the production capture tooling to include Output Genie before running
its screenshot acceptance check; the current command alone does not capture
an unimplemented module. Inspect all generated panels and rendered manuals.
Record dates, commits, fixture identities, measured message latency, actual
commands, platforms, manual observations, and limitations here. No readback
implementation or validation has been completed by this planning change.

[issue]: https://github.com/Kautenja/RackNES/issues/51
[origin]: https://github.com/Kautenja/RackNES/issues/6

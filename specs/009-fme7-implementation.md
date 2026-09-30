# FME-7 And Sunsoft 5B Mapper Implementation

Implement a bounded mapper 69 subset using nes-py's FME-7 banking and register
logic, with RackNES-specific bus, IRQ, and snapshot integration. Preserve the
five base NES audio channels. Finish implementation by updating the manual
and whitepaper with verified support and the missing Sunsoft 5B expansion
sound. This specification adds no runtime mapper support.

Status: PLANNED

Created: September 30, 2026

## Ownership And Evidence

This is the mapper 69 implementation sub-spec of
[001: nes-py Emulator Integration](001-nes-py-integration.md). Spec 001 owns
the general parser, audio/state compatibility policy, and issues #1 and #31.
Keep mapper 69 decisions and completion evidence here; completing this spec
does not close those broader issues.

-   RackNES baseline: `42947ae`, with runtime mapper IDs 0 through 4, 7, and 9.
    [Spec 008](008-mmc5-implementation.md) plans MMC5 but is not implemented.
-   Upstream reference: nes-py commit
    `301da52f7f75de380e6e195fd36621c3d5b03757`, its
    [FME-7 implementation][upstream], [interface][upstream-header], and
    [tests][upstream-tests]. Preserve attribution and dependency notices.
-   Hardware references: [Sunsoft FME-7][hardware], [Sunsoft 5B audio][sound],
    and [IRQ acknowledgement][irq]. Use hardware evidence to resolve
    disagreements rather than treating upstream tests as a complete oracle.

Upstream already implements command-selected banks, four mirroring modes,
ROM/RAM selection at `$6000`, a decrementing counter, and storage for 16 sound
register bytes. Its callback-based IRQ delivery needs adaptation to RackNES's
level IRQ interface. Its disabled-RAM reads return zero, and it applies a
six-bit PRG mask without distinguishing chip variants. Audit these policies.

The current [main bus](../src/nes/main_bus.hpp) directly accesses a fixed
8 KiB RAM array at `$6000`; both ordinary reads and DMA bypass mapper banking.
The [emulator](../src/nes/emulator.hpp) has no mapper CPU-cycle hook. Unlike
MMC5, this increment needs no new PPU fetch detector or nametable source.
It can be implemented before MMC5. If spec 008 supplies shared RAM/clock
interfaces first, reuse them and test both mappers instead of duplicating them.

## Goal And Behavior Examples

1.  Select a command once, then issue several parameter writes. Each write
    updates that command without changing the command latch; sound-register
    selection remains independent.
2.  Map ROM at `$6000`, then RAM bank A, RAM bank B, and disabled RAM. CPU
    reads and OAM DMA see the selected source. Writes cannot alter ROM or
    disabled RAM, and switching away does not destroy RAM contents.
3.  Enable a counter containing zero. Its next clock wraps to `$FFFF` and
    asserts an enabled IRQ. The CPU interrupt-disable flag delays servicing;
    it does not discard the mapper request. Any IRQ-control write acknowledges
    the mapper without acknowledging the APU.
4.  Save between command selection and its parameter write, or one cycle
    before counter underflow. Restore and obtain the same subsequent bank
    changes, interrupt trace, and base-APU output.
5.  Load software that writes Sunsoft 5B sound registers. Those bytes survive
    snapshots, but no extra sound is generated. Accepted mapper 69 headers
    must not be presented as proof of a complete 5B soundtrack.

## Scope And Difficulty

| Area | Difficulty | Required Outcome |
| --- | --- | --- |
| Command registers, PRG/CHR banks, mirroring | Moderate | Adapt existing upstream logic with boundary and alias checks. |
| `$6000` ROM/RAM and DMA routing | High | Remove fixed-window assumptions while preserving older mappers. |
| CPU-cycle counter and IRQ integration | High | Clock during CPU stalls; preserve level IRQs and device acknowledgement. |
| Snapshot and clone ownership | Moderate | Preserve every latch, RAM bank, and counter without stale references. |
| Sound-register storage | Low | Round-trip register state while leaving the audio engine unchanged. |
| Documentation and Rack checks | Moderate | Clearly separate banking/IRQ support from absent expansion sound. |

Include PRG/CHR banking, all four mirroring modes, banked PRG RAM, CPU-cycle
IRQs, full mapper state, and stored Sunsoft 5B register writes. Exclude 5B
synthesis, new audio ports, general PAL/Dendy support, other Sunsoft mappers,
external IRQ-pin devices, and arbitrary board/header extensions. Preserve
existing CPU/PPU/APU scheduling and the five-channel output/MIX contract.

Raw mapper 69 SRAM-file interchange stays disabled under
[spec 004](archive/004-sram-import-export.md); full snapshots still preserve
its RAM. No new release, game-map entry, or commercial-game compatibility claim
follows from factory registration.

## Implementation Requirements

### Initial Cartridge Subset

Use the existing pre-load validation path in [the factory](../src/nes/cartridge.hpp)
to accept only the following initial subset. Confirm bank aliasing against
these layouts before enabling them; document any evidence-driven restriction
here rather than silently broadening acceptance.

| Property | Initial Policy |
| --- | --- |
| Format | Clean iNES or NES 2.0 mapper 69, submapper 0, ordinary NTSC NES/Famicom. |
| PRG ROM | Power-of-two sizes from 32 through 256 KiB. Larger FME-7 layouts are deferred. |
| CHR | Power-of-two CHR ROM from 8 through 256 KiB, or exactly 8 KiB writable CHR RAM. Reject simultaneous CHR ROM/RAM and CHR NVRAM. |
| Legacy PRG RAM | iNES byte 8 equal to 0 or 1 uses a documented 8 KiB compatibility default; reject other sizes initially. |
| NES 2.0 PRG RAM | Explicitly zero, 8 KiB, or 32 KiB in one volatile or nonvolatile allocation. Reject mixed allocations and inconsistent battery declarations. |
| Other flags | Reject trainers, four-screen boards, alternate consoles/timing, unsupported reserved fields, and exponent/multiplier ROM sizes. |
| Payload | Validate exact declared payload size with bounded arithmetic before allocation. |

The 32 KiB RAM layout is an explicit banked-memory profile, not a claim that
every original board carries that memory. NES 2.0 zero RAM must remain absent.
Document iNES battery handling separately from the ambiguous RAM-size default.
The initial PRG limit avoids claiming the larger FME-7-only address range as
Sunsoft 5B support. See [chip differences][hardware].

Rejected files must leave the current cartridge and backup intact with a useful
load error. Do not expand the general NES 2.0 parser merely to accept mapper 69.

### Banking And Bus Access

Port command selection and parameter decoding across their full mirrored
address ranges. Test all command bits, repeated parameter writes, register
boundaries, ignored bits, and the independent sound latch. PRG reads from
register-write addresses must continue to read ROM.

-   Provide eight 1 KiB CHR windows, three switchable 8 KiB high PRG windows,
    and the final fixed PRG bank. Apply bank masks and size aliasing safely;
    CHR RAM must honor banking and CHR ROM must ignore writes.
-   Command 8 selects ROM or RAM for `$6000-$7FFF`. RAM enable gates RAM
    accesses only; it must not disable a ROM-selected window. Test the full
    ROM/RAM/enable combination table, absent RAM, and multiple RAM banks.
-   Route CPU and DMA access to this window through mapper methods. Keep one
    authoritative RAM allocation and default methods preserving old mappers.
    A raw legacy RAM pointer is insufficient for ROM-selected or banked reads.
    Review cartridge cloning and legacy `bus.extended_ram` serialization at
    the same time so they cannot overwrite or duplicate the new RAM state.
-   Keep disabled/absent RAM reads at the current deterministic zero bus
    approximation for this increment; writes do nothing. Real open-bus
    emulation is deferred and must be listed as a compatibility limitation.
-   Verify DMC fetches across switched high PRG windows and wrap boundaries;
    DMC does not use the `$6000` window. CV Genie writes through the bus must
    obey selected RAM/protection without changing its maps or expander protocol.

Test mirroring through actual CIRAM aliases rather than enum names alone:
command C values 0, 1, 2, and 3 select page patterns `[A,B,A,B]`, `[A,A,B,B]`,
`[A,A,A,A]`, and `[B,B,B,B]` respectively, matching the reviewed upstream
implementation. Preserve mirroring callbacks when cloning/restoring; stale
picture-bus JSON must not override mapper-owned routing.

### Counter, IRQ, And Reset

Implement command D acknowledgement and independent counter/output enable
bits. Commands E/F replace the respective byte of the live 16-bit counter,
preserving the other byte; there is no separate reload latch. Count modulo
65536 and assert on enabled underflow, retaining the level until acknowledged.
Test all four enable combinations, continued counting while IRQ output is
disabled, partial counter writes, wrap, repeated wraps, and acknowledgement
while leaving counting enabled. See [IRQ acknowledgement][irq].

Add or reuse a no-op-by-default mapper clock hook called once per emulated CPU
cycle, including OAM DMA and existing DMC/CPU stalls. Define its phase relative
to bus writes and IRQ sampling in the implementation and tests. The current
CPU executes instruction effects in batches; do not claim transistor-accurate
write/underflow coincidence. The hardware reference also leaves that coincidence
unresolved. Record the chosen deterministic ordering and its limit.

Feed `irqPending()` into the existing mapper/APU level combination; do not port
upstream's immediate interrupt callback. Test CPU masking/unmasking, NMI priority,
and simultaneous mapper, APU frame, and DMC requests. Acknowledging one source
must not clear another. Hang freezes all emulated clocks; emulation speed changes
the counter in CPU cycles, not host samples. Preserve frame-output cadence and
the APU buffer clock. No per-cycle allocation, blocking, logging, or I/O.

Document and test power-on defaults separately from soft reset and snapshot
restoration. Do not automatically reload the counter or erase cartridge RAM on
soft reset merely because mapper construction initializes them. Successful ROM
replacement must detach the old IRQ source and callbacks; a failed replacement
must preserve the original cartridge's IRQ state and connections.

### Audio Boundary And State

Store the 5B address latch and all 16 upstream register bytes in mapper state.
Audit selection/masking against the audio reference, record any deliberate
storage-only simplification, and test every slot plus invalid input handling.
Do not route writes into the base APU or enable the bundled
[Nes_Fme7_Apu](../src/nes/apu/Nes_Fme7_Apu.cpp): its noise/envelope support is
incomplete. A future synthesis feature needs its own timing, mixer, routing,
state, and fidelity specification under spec 001.

Serialize command selection, raw bank/control values, mirroring, all allocated
RAM, IRQ counter/enables/pending state, and sound storage. Rebuild derived bank
offsets after validated restoration. Clones must rebind the destination ROM
and callbacks and must not share mutable memory. Validate JSON types, ranges,
buffer lengths, and any state version before mutating live state. Preserve
legacy mapper snapshots and module identifiers. Exercise malformed state,
missing ROMs, repeated backup/restore, and replacement under sanitizers.

## Delivery And Acceptance

Implement in this order: validated headers and banking fixtures; mapper bus
access and DMA; cycle/IRQ integration; state and base-audio regressions; manual
Rack checks; final documentation. Add focused checks to the existing
[test harness](../tests/README.md) and its default `check` target. Preserve
upstream attribution without adding its test framework or Python runtime.

- [ ] Accepted size extremes, zero/banked RAM, and every rejected layout have
    deterministic loader checks with unchanged live/backup state on failure.
- [ ] Every command, address mirror, PRG/CHR boundary, RAM mode, and CIRAM
    alias passes expected-value checks independent of copied upstream logic.
- [ ] Executable CPU fixtures cover switched reads/writes, ROM/RAM OAM DMA,
    banked DMC fetches, and actual IRQ servicing, not just direct mapper calls.
- [ ] Counter traces cover 0, 1, and `$FFFF`, both byte-write orders, masking,
    acknowledgement, stalled cycles, pending restoration, and competing IRQ/NMI.
- [ ] Snapshots restore all RAM and sound bytes, both command latches, and the
    exact counter phase; clone destruction/replacement exposes no stale references.
- [ ] Existing mapper 0-4, 7, and 9 checks pass. If MMC5 is implemented first,
    its tests pass too, including any shared clock/RAM changes.
- [ ] Existing PCM fingerprints remain unchanged. Extend the base-APU fixture
    to mapper 69 at 44.1, 48, 96, and 192 kHz; cover DMC and IRQ-driven sound,
    five individual outputs, MIX exclusion, clock extremes, host-rate changes,
    Hang/resume, and save/reset/load ordering. Stored 5B writes add no sound.
- [ ] In Rack, use identified legal fixtures to check boot/rendering, scrolling,
    base audio, IRQ-driven behavior, RAM, snapshots, patch reopening, and ROM
    replacement at 44.1 and 48 kHz. Record fixture hashes/provenance, Rack/SDK,
    platform, clock settings, and listening/display observations. Do not
    redistribute commercial ROMs or infer whole-game support from a boot.
- [ ] Manual/whitepaper updates below are complete and rendered PDFs inspected.

From the repository root, use one matching Rack SDK or prepared Rack tree as
described in [the contributor guide](../CONTRIBUTING.md#configure-the-rack-sdk).
Use the normal sanitizer configuration; report unavailable instrumentation.
TeX prerequisites are in the [manual](../manual/README.md) and
[report](../whitepaper/README.md) guides.

```shell
make -j4
make -C tests -j2
make -C manual
make -C whitepaper
make -C whitepaper source
git diff --check
```

These are future implementation checks, not results of writing this spec.
Keep dated results, implementation revisions, decisions, and unresolved checks
here. Remain IN PROGRESS if required checks are unavailable or fail; mark
COMPLETE and archive only after all gates pass. If source packaging changes,
also extract and compile its archive independently per the report guide.

## Final Manual And Whitepaper Updates

At the end of implementation, update [README](../README.md), the
[contributor architecture](../CONTRIBUTING.md#architecture), and the
[RackNES manual](../manual/RackNES) with mapper 69's accepted headers/layouts,
banking, IRQs, snapshot behavior, SRAM-file restriction, and timing/open-bus
limits. Explicitly state that Sunsoft 5B expansion audio is absent and that
its register storage is not synthesis. Update affected CV Genie compatibility
text without inventing new game support, and add an unreleased
[changelog](../CHANGELOG.md) entry.

Extend the dated mapper addendum in [the manuscript](../whitepaper/racknes.tex),
update [source evidence](../whitepaper/sources.md), and align the
[report overview](../whitepaper/README.md). Explain the CPU-cycle hook, bus
ownership, level IRQs, and audio boundary using verified implementation
evidence. Preserve the pinned historical account unless intentionally revising
it; keep citation metadata aligned if manuscript version/status changes.

Build and visually review the affected PDFs. Keep generated products ignored;
refresh production screenshots only if visible UI changes require them.
Finally update spec 001 and the [spec index](README.md) with verified progress
and remaining restrictions, without claiming a release or full 5B support.

[upstream]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/src/nes_emu/mappers/mapper_FME7.cpp
[upstream-header]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/include/nes_emu/mappers/mapper_FME7.hpp
[upstream-tests]: https://github.com/Kautenja/nes-py/blob/301da52f7f75de380e6e195fd36621c3d5b03757/nes_emu/test/nes_emu/mappers/test_mapper_FME7.cpp
[hardware]: https://www.nesdev.org/wiki/Sunsoft_FME-7
[sound]: https://www.nesdev.org/wiki/Sunsoft_5B_audio
[irq]: https://www.nesdev.org/wiki/IRQ

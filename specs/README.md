# Implementation Specifications

These specifications track planned and active work. A specification is not
an implementation or a release promise. Preserve issue-specific acceptance
evidence in the owning spec, then archive it using [the agent workflow][flow].

## Active Work And Issue Ownership

| Spec | Scope | GitHub Issues | Order |
| --- | --- | --- | --- |
| [001](001-nes-py-integration.md) | Cartridge parsing, mapper integration, audio/state compatibility | [#1][i1], [#31][i31] | Deliver parser/tracker and mapper gates as separate increments. |
| [005](005-mmc1-ntsc-regressions.md) | Metroid and Bubble Bobble reproduction and fixes | [#26][i26], [#45][i45] | Reproduce first; potentially the smallest closures. |
| [008](008-mmc5-implementation.md) | MMC5 implementation subset, validation, and final manual/report updates | Under 001 | Freeze board profiles before bus/PPU integration; preserve base audio. |
| [009](009-fme7-implementation.md) | FME-7 / Sunsoft 5B mapper subset, validation, and final manual/report updates | Under 001 | Integrate banked RAM and CPU-cycle IRQs; defer expansion sound. |

Specs 002 and 003 are complete and retained in [the archive](archive/).
[Spec 004](archive/004-sram-import-export.md) is complete and archived;
[#52][i52] remains open for SRAM workflow feedback and native dialog checks.
[Spec 006](archive/006-cv-genie-readback.md) and
[spec 007](archive/007-snapshot-slots.md) are abandoned and archived.
Specs 008 and 009 are the requested MMC5 and FME-7 implementation sub-specs of
001. Keep general parser and audio/state policy in 001 rather than duplicating
its ownership; record mapper-specific implementation evidence in each sub-spec.

## Issue Dispositions

The active implementation issues above remain open until their acceptance
gates pass. The September 30, 2026 decisions also establish:

-   **[#52: Serialization of SRAM][i52]: implementation complete.** Raw 8 KiB
    SRAM import/export is committed on `techreport`. Spec 004 is archived;
    keep the issue open for requester feedback, tracker compatibility, and
    native dialog verification. This is not a release-availability claim.

-   **[#12: Additional Save States][i12]: not planned.** Multiple snapshot
    slots are outside the current plan. Spec 007 is abandoned and archived
    for historical context; this disposition does not claim implementation.
-   **[#51: CV Genie Readback][i51]: not planned.** Spec 006 is abandoned and
    archived. A new output module, safe RAM/APU observation, expander protocol,
    snapshot compatibility, and UI/manual validation add implementation and
    maintenance scope outside the current plan. This does not claim readback
    is implemented or technically impossible.
-   **[#6: CV RAM Manipulation][i6]: read scope not planned.** Input Genie
    implements the write portion. The remaining read portion was consolidated
    into #51 and is now dropped with spec 006; memory CV outputs are unavailable.
-   **[#24: NES Advantage][i24]: not planned.** A/B gates can already receive
    external clocks or LFOs. Repeated Start pulses can approximate the
    controller's pause-based slow behavior where a game supports it. Keep
    these patchable controls rather than add dedicated turbo/slow modes.
    Pause-based slow behavior is game-dependent.
-   **[#5: CV Palette Manipulation][i5]: not planned.** Palette CV is feasible,
    but it adds a visual-control interface and display synchronization work
    outside the compatibility, tracker interchange, and musical-control
    priorities above. Revisit if a concrete scope and contribution emerge.

These are maintenance decisions, not claims of technical impossibility.
Do not close unresolved game bugs because a fixture is unavailable, or mark a
feature completed merely because its spec has been committed.

[flow]: ../AGENTS.md#planning-and-completion
[i1]: https://github.com/Kautenja/RackNES/issues/1
[i5]: https://github.com/Kautenja/RackNES/issues/5
[i6]: https://github.com/Kautenja/RackNES/issues/6
[i12]: https://github.com/Kautenja/RackNES/issues/12
[i24]: https://github.com/Kautenja/RackNES/issues/24
[i26]: https://github.com/Kautenja/RackNES/issues/26
[i31]: https://github.com/Kautenja/RackNES/issues/31
[i45]: https://github.com/Kautenja/RackNES/issues/45
[i51]: https://github.com/Kautenja/RackNES/issues/51
[i52]: https://github.com/Kautenja/RackNES/issues/52

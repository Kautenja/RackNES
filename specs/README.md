# Implementation Specifications

These specifications track planned and active work. A specification is not
an implementation or a release promise. Preserve issue-specific acceptance
evidence in the owning spec, then archive it using [the agent workflow][flow].

## Active Work And Issue Ownership

| Spec | Scope | GitHub Issues | Order |
| --- | --- | --- | --- |
| [001](001-nes-py-integration.md) | Cartridge parsing, mapper integration, audio/state compatibility | [#1][i1], [#31][i31] | Deliver parser/tracker and mapper gates as separate increments. |
| [004](004-sram-import-export.md) | Raw battery-save import/export | [#52][i52] | Start with unambiguous supported 8 KiB layouts; coordinate RAM ownership with 001. |
| [005](005-mmc1-ntsc-regressions.md) | Metroid and Bubble Bobble reproduction and fixes | [#26][i26], [#45][i45] | Reproduce first; potentially the smallest closures. |

Specs 002 and 003 are complete and retained in [the archive](archive/).
[Spec 006](archive/006-cv-genie-readback.md) and
[spec 007](archive/007-snapshot-slots.md) are abandoned and archived.
Do not create another parser or mapper spec that duplicates 001.

## Issue Dispositions

The September 30, 2026 triage keeps the five implementation issues above
open until their acceptance gates pass. It makes these scope decisions:

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

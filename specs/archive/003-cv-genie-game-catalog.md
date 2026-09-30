# CV Genie Game Catalog

Created: 2026-09-30
Status: COMPLETE

## Goal

Expand CV Genie's useful coverage of familiar NES games without claiming a
measured percentage of the library. Replace the manual's exhaustive address
tables with a generated list of available games and practical usage guidance.

## Selection And Scope

Keep Super Mario Bros. and The Legend of Zelda. Add Mega Man, Mega Man 2,
Castlevania, Castlevania II: Simon's Quest, Contra, Metroid, Ninja Gaiden,
and Tetris (Nintendo). This is an editorial shortlist of recognizable series,
action games, and a puzzle staple, constrained by available RAM documentation
and the current mapper 0--3 emulator. It is not a popularity survey or a claim
of 90% coverage. Prioritize a handful of useful controls over exhaustive maps.

Super Mario Bros. 2 and 3, Mega Man 3--6, Kirby's Adventure, and Super C need
MMC3 (mapper 4); Punch-Out needs MMC2 (9); Castlevania III and Battletoads need
other unsupported mappers. Keep those out of the supported menu. DuckTales,
Zelda II, Dr. Mario, Kid Icarus, and early arcade titles are follow-up candidates
when sufficiently clear address/range evidence is available. Do not add obscure
titles just to inflate the count.

## Behavior Examples

-   With Mega Man selected, 0--10 V controls a documented health/weapon-energy
    byte within 0--28 rather than sweeping every possible byte value.
-   With Contra selected, each player's weapon selection stays within the five
    base weapons; the label states that the rapid-fire flag is cleared.
-   An existing Mario patch still restores game 0, element 6 to address $0086.
-   The manual lists games; row menus and hover help supply assignment ranges
    and modes directly from the implementation.

## Requirements

-   Preserve all existing game IDs, location order, names, endpoints, and modes.
    Append new games; never sort serialized IDs alphabetically.
-   Record sources, cartridge scope, range decisions, and validation limitations
    in a contributor reference. New entries must fit one byte at nonzero
    internal RAM addresses below $0800. Avoid packed decimal, pointer, sparse
    enum, multi-byte, uncertain, or banked-memory writes.
-   Use documented coordinates, timers, bounded counters, and dense enums.
    Coordinate writes can cross walls; timer writes can freeze game behavior.
    Document this without claiming gameplay validation that was not performed.
-   Generate the supported-game list from the shipped catalog, with an offline
    check mode. No network, allocation, or new parsing in audio processing.
-   Update README, manuals, and changelog together. Keep the version unreleased.

## Non-Goals

New mappers, ROM detection, Game Genie ROM patches, arbitrary-address editing,
new write modes, a universal website scraper, and exhaustive playtesting of
commercial ROMs are outside this change. No broad emulator or UI rewrite.

## Acceptance And Validation

-   Ten game maps are selectable; every new entry has traceable evidence.
-   Automated checks cover all maps' bounds, menu enumeration and actions,
    CV endpoints, disconnects, saved selections, and the legacy patch fixture.
-   Compare the complete original Mario/Zelda arrays against the base revision.
-   Build the plugin and both manuals; inspect the rendered manual pages.
-   Check generated list freshness, links, and the complete diff before commit.

Run from the repository root using the existing prepared Rack tree:

```shell
python3 tools/update_game_list.py --check
make -j2
make -C tests -j2
make -C manual
git diff --check
```

## Completion Evidence

Completed: 2026-09-30.

-   Added eight maps / 54 controls (ten maps / 162 controls total). Sources and
    per-game range decisions are in [the mapping reference](../../docs/game-maps.md).
    Chose curated static entries and an offline manual-list generator; a bulk
    website scraper cannot infer packed fields, revision scope, or safe ranges.
-   Preserved both legacy arrays byte-for-byte against the pre-change header:
    all 108 definitions, names, ordering, ranges, and modes match. IDs remain
    Mario 0 and Zelda 1; new IDs are explicitly appended as 2--9.
-   `python3 tools/update_game_list.py --check`: passed. Added the check to the
    existing publication workflow; generated names are alphabetized only in
    the manual, never in serialized game order.
-   `make -j2`: passed on macOS arm64, Apple Clang 21, prepared Rack 2.6.0 tree.
    Existing Rack-header literal-operator deprecation warnings remain.
-   `make -C tests -j2`: both SDK-backed suites passed with AddressSanitizer and
    UndefinedBehaviorSanitizer. Catalog checks cover all 162 entries, both new
    toggles, game menu-item actions, endpoints, disconnects, randomization,
    saved selection round trips, bounds, and new-map duplicate detection.
    The original debug patch compatibility check also passed.
-   Follow-up validation on 2026-09-30 exercises the actual menu population,
    checking all ten labels, serialized IDs, checkmarks, and selection actions.
    Every restored assignment produces its expected endpoint write, including
    the next toggle edge; disconnects clear both expander buffers. The menu
    population helper preserves the existing UI behavior and runs without a
    graphical scene. `make -j2` and `make -C tests -j2` passed with the same
    toolchain and sanitizers above.
-   `make -C manual`: both manuals passed. After final CV Genie prose edits,
    `make -C manual/CVGenie` passed again; the final LaTeX pass has no warnings.
    Inspected rendered pages using Poppler PNGs, including all changed pages
    at readable resolution. RackNES has 13 pages; CV Genie has 10 pages, with
    its game list on one page. PDF metadata, outline, language, XMP, text, and
    resolved-reference checks passed using the existing CI criteria.
-   `git diff --check`, local documentation path checks, generated-list check,
    and review of the complete task diff passed. User/concurrent whitepaper
    and RackNES figure work is outside this change and was not staged.
-   No commercial-ROM gameplay, live Rack UI/audio session, Windows build, or
    Linux build was performed. Address evidence and byte-processing regression
    checks are not proof of every game's response. Mapper expansion and the
    deferred candidates remain outside this completed scope.

# RackNES Agent Instructions

RackNES is a VCV Rack 2 plugin that turns an NES emulator into a
voltage-controlled musical instrument. It includes RackNES and the CV Genie
input expander. Preserve emulation behavior, musical timing, responsive
audio and displays, and compatibility with users' saved patches.

## Start Here

Read the relevant sources before editing:

-   [README.md](README.md): project overview and user manual links.
-   [Contributor guide](CONTRIBUTING.md): environment setup and pull requests.
-   [Architecture](CONTRIBUTING.md#architecture): source map, processing,
    ownership, and compatibility.
-   [Development And Testing](CONTRIBUTING.md#development-and-testing):
    build commands, regression coverage, CI, and manual checks.
-   [Manual Figures](CONTRIBUTING.md#manual-figures): production captures
    and panel references.
-   [plugin.json](plugin.json): plugin identity, registered modules, and version.
-   [CHANGELOG.md](CHANGELOG.md): historical behavior and compatibility fixes.
-   [LICENSING.md](LICENSING.md): source, visual-asset, and dependency terms.
-   [C++ Style Guide](docs/style-guides/cpp.md): required for C++ source,
    headers, and regression checks.
-   [Markdown Style Guide](docs/style-guides/markdown.md): required for
    documentation changes.
-   [Technical report guide](whitepaper/README.md): manuscript structure,
    evidence, citation metadata, and build commands.

These instructions and the contributor guide adapt Fourier's working
principles to RackNES. They are self-contained; Fourier is not a build
dependency. Keep shared setup, architecture, and validation guidance in
`CONTRIBUTING.md`, and agent-specific workflow in this file.

## Working In This Repository

-   Inspect `git status --short` and relevant diffs before editing. Preserve
    existing user work, including changes in files needed for the task.
-   Implement one coherent requested change at a time. Read nearby code
    before choosing an implementation, and avoid unrelated formatting,
    dependency upgrades, or new development infrastructure.
-   Follow the style guides above and surrounding conventions. Preserve
    existing names and local conventions in adapted third-party code rather
    than reformatting whole files.
-   Keep emulator logic in `src/nes/` independent of Rack widgets. Put host
    processing and UI integration in the module and widget code.
-   Preserve file-level attribution and dependency license notices. The
    emulator, audio library, NTSC filter, and Base64 implementation have
    upstream origins recorded in the README and source headers. Do not
    replace their notices with a generic project header.
-   Prefer fixes in first-party integration code over edits to bundled
    libraries. Change library code only when the requested fix requires it,
    and document the reason. Do not edit generated build products.
-   Keep source, manifest metadata, resources, patches, and manuals aligned
    when behavior or interfaces change.
-   Commit only when requested or included in the task. Push, publish, and
    release only when requested. Keep credentials and private local paths
    out of tracked files.

## Compatibility

Follow the [architecture guidance](CONTRIBUTING.md#architecture) when changing
module identities, saved JSON, game maps, or expander messages. Do not renumber
existing Rack IDs, rename slugs, or reorder serialized game/location indices
without an intentional compatibility plan and verification with existing
patches. Preserve Input Genie's placement to RackNES's right, eight address/value
pairs, and address-zero sentinel. Review buffer ownership, message flips,
memory bounds, and disconnected inputs together.

## Correctness And Real-Time Behavior

Emulation and CV processing run on Rack's audio engine thread. Keep per-sample
work bounded and avoid adding allocation, blocking, file I/O, logging, or drawing
to that path. The current implementation already loads ROMs and serializes
save states from processing code; do not claim it is fully real-time safe.
Document limitations touched by a change and avoid broad unsolicited rewrites.

Preserve or explicitly test changes to:

-   CPU/PPU/APU scheduling, interrupts, mapper behavior, and frame-clock output.
-   The relationship between emulation speed, the APU buffer clock, and the
    host sample rate, including clock extremes and sample-rate changes.
-   CV acquisition at the existing divider, controller button order, gate
    thresholds, and save/reset/restore ordering when triggers coincide.
-   Hang behavior, reset behavior, ROM replacement, and state restoration.
-   Five individual audio channels and the mix output's exclusion of channels
    whose individual output ports are connected.

Review engine/UI handoffs whenever changing the screen buffer, ROM-path signals,
or error flags. A shared flag or string is not a synchronization contract.
Check Jansson ownership and error paths when changing save states or patch JSON;
existing code is not evidence that every ownership pattern is correct.

Behavior changes need evidence at the appropriate seam. Reproduce bugs when
practical, and add focused deterministic regression checks where feasible.
Do not weaken assertions or hide failures. Performance claims require comparable
before/after workloads, compiler settings, repeated measurements, and a meaningful
effect; a successful build is not evidence of a speedup.

## Development And Validation

Follow [Development And Testing](CONTRIBUTING.md#development-and-testing)
for prerequisites, commands, regression coverage, and manual Rack checks.
Validate source changes with a Rack build and the relevant regression or
manual checks. There is no root `make test` target; use `make -C tests` with
the same Rack SDK as the plugin build. Report missing prerequisites explicitly,
and do not introduce a test framework for a small unrelated change.

Documentation-only changes need link, path, command, and diff checks rather
than a mandatory C++ build. Run `git diff --check` and review the complete diff
before committing. Distinguish a successful build, executable regression
checks, and an actual manual Rack session in completion reports.

## Manuals And Technical Report

Follow [Manual Figures](CONTRIBUTING.md#manual-figures) and the
[manual guide](manual/README.md) for panel references, production screenshots,
PDF builds, and visual review. Refresh captures after visible widget or runtime
SVG changes, using the production widgets and original branded ROM fixture.
Do not replace screenshots with manually maintained illustrations. Ordinary
manual builds use committed screenshots without Rack or a graphical session.
Keep intermediate files and compiled manuals in ignored build folders.

Follow [whitepaper/README.md](whitepaper/README.md) for report edits. Keep the
manuscript, bibliography, and diagram in the canonical standalone
`whitepaper/racknes.tex`, and tie claims to `whitepaper/sources.md`. Preserve
the documented source revision unless intentionally updating the implementation
account. Keep `CITATION.cff`, `whitepaper/CITATION.bib`, and the README citation
consistent with the manuscript's actual version and publication status.

```shell
make -C whitepaper
make -C whitepaper source
```

These targets create `.build/racknes.pdf` and `.build/racknes-source.tar.gz`
inside `whitepaper/`. Inspect the rendered report after typesetting changes;
when changing source packaging, extract and compile the archive independently.

## Releases And Completion

For an explicitly requested release, align `plugin.json`, `CHANGELOG.md`, and
affected manuals, preserving plugin and module identities. Validate the plugin
and documentation before tagging or publishing, and report which platforms were
actually checked. Verify that the release tag resolves to the intended commit
and manifest version; do not move an already published tag to incorporate a fix.

The manifest and README expect GitHub release assets named `RackNES.pdf` and
`CVGenie.pdf`. Build and check these assets when preparing a release. The
[user-manual workflow](.github/workflows/manuals.yml) builds PDFs and attaches
them on published-release events or explicit dispatch for an existing release.
See [manual/README.md](manual/README.md) for tag requirements. GitHub publication
and VCV Library submission are separate actions. Verify current VCV requirements
when a submission is requested, and do not claim availability in Rack merely
because a tag, release, or update request exists.

Small changes can be planned in chat. For substantial work, create a durable
specification when useful or requested, with behavior examples, acceptance
criteria, and exact validation commands. Avoid duplicate progress diaries or
making a specification a prerequisite for every edit.

Finish with a concise summary of the changes, validation actually run, and
unresolved failures or skipped checks. Include the commit and push result when
those actions were requested.

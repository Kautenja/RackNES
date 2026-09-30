# RackNES Agent Instructions

RackNES is a VCV Rack 2 plugin that turns an NES emulator into a
voltage-controlled musical instrument. It includes RackNES and the CV Genie
input expander. Preserve emulation behavior, musical timing, responsive
audio and displays, and compatibility with users' saved patches.

## Start Here

Read the relevant sources before editing:

-   [README.md](README.md): project overview and user manual links.
-   [plugin.json](plugin.json): plugin identity, registered modules, and version.
-   [CHANGELOG.md](CHANGELOG.md): historical behavior and compatibility fixes.
-   [LICENSING.md](LICENSING.md): source, visual-asset, and dependency terms.
-   [C++ Style Guide](docs/style-guides/cpp.md): required for C++ source,
    headers, and regression checks.
-   [Markdown Style Guide](docs/style-guides/markdown.md): required for
    documentation changes.
-   [Technical report guide](whitepaper/README.md): manuscript structure,
    evidence, citation metadata, and build commands.

This guide takes its working principles from Fourier's agent instructions,
adapted to the files and workflows in RackNES. It is self-contained; Fourier
is not a build dependency. There is no separate contributor guide. Focused
SDK-backed regression checks are described in [tests/README.md](tests/README.md);
the user-manual CI workflow is described in [manual/README.md](manual/README.md).

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

## Source Map And Compatibility

-   `src/plugin.cpp` and `src/plugin.hpp`: plugin initialization and model
    registration. The plugin slug is `KautenjaDSP-RackNES`; the registered
    module slugs are `RackNES` and `InputGenie`. Output Genie code is
    unfinished and is not registered as a module.
-   `src/RackNES.cpp`: controls, audio processing, emulator ownership,
    save/restore, patch JSON, expander consumption, and panel integration.
-   `src/CVGenie.cpp` and `src/GameMaps.hpp`: expander processing, game-specific
    RAM maps, selectors, and saved game/location indices.
-   `src/nes/emulator.hpp`: CPU, PPU, APU, controller, and bus coordination.
    The other `src/nes/` headers and sources implement those components,
    cartridge loading, and mappers. The cartridge factory currently supports
    mapper IDs 0 through 3: NROM, MMC1, UNROM, and CNROM.
-   `src/nes/apu.hpp`: RackNES audio wrapper; `src/nes/apu/` contains the
    bundled sound-emulation code. `src/nes/ntsc/` contains the NTSC filter.
-   `src/widget/display.hpp`, `src/components.hpp`, and `src/theme.hpp`:
    display rendering, custom controls, and panel themes. `res/` holds the
    panel and component SVG assets.
-   `manual/RackNES/` and `manual/CVGenie/`: LaTeX user manuals and artwork.
    `patches/debugCVGenie.vcv` is an existing integration/debug patch.
-   `whitepaper/racknes.tex`: standalone technical report, with supporting
    evidence recorded in `whitepaper/sources.md`.

Do not renumber existing parameter, port, or light IDs, rename slugs, or change
saved JSON meanings without an intentional compatibility plan and verification
with existing patches. Game IDs and memory-location indices are serialized;
reordering the maps can silently change a restored patch's behavior.

Preserve the CV Genie adjacency and message contract: Input Genie sits directly
to RackNES's right and sends eight address/value pairs through Rack's expander
buffers. Address zero denotes an empty message. Review producer/consumer
ownership, message flips, memory bounds, and disconnected inputs together.

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

The root [Makefile](Makefile) delegates to VCV Rack's `plugin.mk`. Use a Rack 2
SDK or prepared Rack source tree with the required headers, libraries, and
build tools. `RACK_DIR` defaults to `../..`, matching a checkout under
`Rack/plugins/RackNES`. An external SDK can be selected explicitly:

```shell
make RACK_DIR=/absolute/path/to/Rack-SDK
```

From the repository root with the default layout:

```shell
make
make dist
```

`make` builds the platform plugin library; `make dist` packages it under `dist/`.
Packaging requires the tools used by the selected SDK's `plugin.mk`, including
`jq` and `zstd`. `make install` additionally copies a package into the Rack user
plugins directory; use it when local installation is part of the task.
`make clean` removes plugin build and distribution outputs. Reuse the same `RACK_DIR`
override for all targets when using an external SDK.

There is no root `make test` target. Run the focused SDK-backed checks with
`make -C tests`; [tests/README.md](tests/README.md) documents their scope and
prerequisites. The plugin Makefile's `-DTEST` flag does not run tests. Validate
source changes with a Rack build and the relevant regression or manual checks,
reporting missing SDKs or other prerequisites explicitly. Do not introduce a
test framework merely to complete a small unrelated change.

For affected integration behavior, check in Rack:

-   Module creation, panel preview, light/dark themes, and display rendering.
-   ROM loading and replacement, invalid files, unsupported mappers, and patch
    reload when the original ROM is unavailable.
-   Both controllers, clock knob/CV extremes, frame-clock output, channel/mix
    routing, and host sample-rate changes.
-   Save/load, simultaneous save/reset/load, hang, module reset, and patch
    save/reopen.
-   CV Genie attached/detached, game/location selection, continuous and toggle
    inputs, and restored selections. Use the debug patch where useful.

Use user-provided ROMs or redistributable test/homebrew fixtures. Do not add
commercial ROMs to the repository. Record the fixtures and settings used so
that checks can be reproduced.

Documentation-only changes need link, path, command, and diff checks rather
than a mandatory C++ build. Run `git diff --check` and review the complete diff
before committing. Distinguish a successful build, executable regression
checks, and an actual manual Rack session in completion reports.

## Manuals And Technical Report

Keep user-facing documentation in the existing manual directories. Edit SVG
sources for artwork and regenerate the corresponding PDF assets when needed;
verify that panel positions and numbered references still match the module.
Follow the active manual makefiles and any shared local style/build sources;
do not assume Fourier's layout or screenshot exporter is available here.

With the TeX tools required by the active recipes (`pdflatex`, BibTeX, and
`latexmk` where used) and the packages declared by the manuals installed:

```shell
make -C manual/RackNES
make -C manual/CVGenie
```

Each manual is written to its own `.build/manual.pdf`. The shared build uses
`latexmk`, disables shell escape, and fails on compilation errors. See
[manual/README.md](manual/README.md) for prerequisites and the combined build.
Review compiler output and inspect the generated PDF for missing content,
unresolved references, and layout errors. Compile directly with errors visible
when diagnosing a failure. Keep intermediate files and compiled manuals in
ignored build folders.

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

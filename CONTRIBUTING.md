# Contributing To RackNES

Help improve RackNES and CV Genie through bug reports, documentation, tests,
and code. This guide takes you from a fresh checkout to a tested contribution
and explains the architecture and compatibility requirements used in review.

-   [Set up your environment](#set-up-your-environment)
-   [Understand the architecture](#architecture)
-   [Build and test](#development-and-testing)
-   [Choose validation for your change](#choosing-validation)
-   [Update manual figures](#manual-figures)
-   [Prepare a release and VCV update](#prepare-a-release-and-vcv-update)
-   [Submit a pull request](#submit-a-pull-request)

## Before You Start

Search the [existing issues][issues] before reporting a bug or proposing a
feature. Include the Rack and plugin versions, operating system and CPU
architecture, sample rate, reproduction steps, and expected versus observed
behavior. For ROM-dependent problems, identify the game or homebrew revision,
mapper, and relevant settings. A minimal patch or screenshot helps; list any
additional plugins or files it requires. Do not upload commercial ROMs.
Discuss substantial behavior or interface changes in an issue before starting.

Read the [README](README.md), [changelog](CHANGELOG.md), and relevant
[C++][cpp-style] or [Markdown][markdown-style] style guide before editing.
Preserve attribution and the separate source, artwork, and dependency terms
in [LICENSING.md](LICENSING.md). If you use a coding agent, have it follow
[AGENTS.md](AGENTS.md).

## Set Up Your Environment

| Work | Required Tools |
| --- | --- |
| Rack plugin | Git, GNU Make, C/C++ compilers, `jq`, and a compatible Rack 2 SDK or prepared Rack source tree |
| Headless regression checks | C++11 compiler, matching Rack headers and library, and sanitizer runtimes for the default flags |
| Plugin packaging | Plugin build tools plus the SDK's packaging tools, including `zstd` |
| Interactive checks | A Rack 2 installation matching the plugin's platform and architecture |
| User manuals and technical report | Make, `latexmk`, `pdflatex`, and the packages declared by the document sources |
| Panel captures | Rack build dependencies, an OpenGL desktop session, Python 3, and Pillow |

Markdown-only contributions do not require a C++ or TeX toolchain. The current
regression suites require Rack even though they run without its graphical UI.

### Get The Source

From your projects directory, clone the repository. To contribute through a
pull request, fork it on GitHub first and substitute your fork's clone URL:

```shell
git clone https://github.com/Kautenja/RackNES.git
cd RackNES
git switch -c docs/contributor-setup
```

Replace the example branch name with one describing your change. Run the
remaining commands from the repository root unless stated otherwise.

### Configure The Rack SDK

Use a Rack 2 SDK for your operating system and CPU architecture, or a prepared
Rack source tree. The [build CI](.github/workflows/rack-tests.yml) records the
platform toolchains and pinned SDK downloads used by this repository. An
installed Rack application alone does not provide the complete SDK.

Set an absolute SDK path so commands in subdirectories resolve the same tree.
Replace this placeholder with your extracted SDK location:

```shell
export RACK_DIR=/absolute/path/to/Rack-SDK
test -f "$RACK_DIR/plugin.mk"
test -d "$RACK_DIR/include"
test -d "$RACK_DIR/dep/include"
```

The SDK must also include the matching Rack library. Keep `RACK_DIR` set for
subsequent Make commands in the same shell. In the default
`Rack/plugins/RackNES` layout, omit the export to use the prepared Rack tree:
the root Makefile defaults to `../..`, and the test and capture makefiles
resolve that tree from their own directories.

On Windows, follow the MSYS2 MINGW64 toolchain used by CI. Headless tests also
need the matching Rack runtime DLL; see the [test guide](tests/README.md) for
runtime search paths and sanitizer limitations.

## Architecture

### Source Map And Compatibility

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
-   `specs/NNN-feature-name.md`: numbered specifications for planned or active
    work; `specs/archive/` retains completed or abandoned specs and their
    evidence. Follow [Planning And Completion](AGENTS.md#planning-and-completion)
    for the format and lifecycle.

Do not renumber existing parameter, port, or light IDs, rename slugs, or change
saved JSON meanings without an intentional compatibility plan and verification
with existing patches. Game IDs and memory-location indices are serialized;
reordering the maps can silently change a restored patch's behavior.

Preserve the CV Genie adjacency and message contract: Input Genie sits directly
to RackNES's right and sends eight address/value pairs through Rack's expander
buffers. Address zero denotes an empty message. Review producer/consumer
ownership, message flips, memory bounds, and disconnected inputs together.

### Processing And Ownership

Rack calls `RackNES::process()` on its audio engine thread. The module handles
pending ROM loads, acquires controls every 16 host samples, consumes expander
messages every sample, advances the emulator, and updates clock and audio
outputs. Hang returns after control and expander processing, holding outputs
while those inputs continue to work. Coincident snapshot controls run in
save, reset, then restore order.

The emulator coordinates CPU, PPU, APU, controllers, and cartridge mapping.
Clock modulation changes emulation speed; preserve its relationship to host
sample rate and the APU buffer clock. Five individual audio outputs feed MIX
only when their own output ports are disconnected. Changes to scheduling,
interrupts, sample conversion, or routing need listening and gameplay checks
as well as focused regression evidence.

Keep emulator logic independent of Rack widgets. Host processing belongs in
module code, and drawing belongs in widgets. Review ownership whenever changing
screen buffers, ROM-path signals, or error flags; a shared flag or string is
not a synchronization contract. Check Jansson reference ownership and failure
paths when changing snapshots or patch JSON.

Avoid adding allocation, blocking, file I/O, logging, or drawing to per-sample
processing. The existing path already loads ROMs and serializes snapshots;
it is not fully real-time safe. Document limitations touched by a change.
Prefer fixes in first-party integration code over bundled libraries, and
explain any necessary library changes while retaining upstream notices.

### Build Boundaries

The root [Makefile](Makefile) delegates compilation and packaging to Rack's
`plugin.mk`. It explicitly includes sources in `src/`, `src/nes/`,
`src/nes/mappers/`, `src/nes/apu/`, and C sources in `src/nes/ntsc/`.
Do not assume a new nested directory will be compiled automatically.

The [test Makefile](tests/Makefile) builds assertion-based executables against
real Rack headers and `libRack`. There is no separate test framework or root
`make test` target. The plugin's `-DTEST` flag does not run tests.

## Development And Testing

### Rack Plugin Build

With the SDK configured above:

```shell
make
make dist
```

`make` builds `plugin.dylib`, `plugin.so`, or `plugin.dll` for the selected
platform. `make dist` packages the plugin under `dist/`. When you want to
install your local build into Rack's user plugins directory, run:

```shell
make install
```

Restart Rack to load the rebuilt plugin. `make clean` removes the plugin
build and distribution outputs; it does not clean the separate test or
publication builds. Preserve any local experiment results before cleanup.
Use the same SDK and architecture for build, packaging, and installation.

### Headless Regression Checks

Build and execute both suites with the configured SDK:

```shell
make -C tests -j2
```

The default flags enable AddressSanitizer and UndefinedBehaviorSanitizer.
For a compiler without those runtimes, use `SANITIZERS=` and report the
missing instrumentation. Rebuild when changing compiler, SDK, or flags:

```shell
make -C tests -B -j2 SANITIZERS=
```

Outputs stay in ignored `tests/.build/`. The [test guide](tests/README.md)
describes exact prerequisites and coverage: CV Genie selections, voltage
conversion, toggles, expander messages, saved indices, malformed JSON, and
RackNES snapshot ownership and ROM-load failure handling. The existing
`patches/debugCVGenie.vcv` supplies saved Genie data for compatibility checks.
No game ROM or graphical session is required by these suites.

Add focused deterministic checks at the behavior boundary when practical.
Reproduce a bug before fixing it, and do not weaken assertions to pass a
change. These fixtures do not validate the entire emulator, rendering,
audio output, or every game's response to memory edits.

### Continuous Integration

The [Rack workflow](.github/workflows/rack-tests.yml) builds the plugin and
runs headless checks on Linux x64, macOS arm64, and Windows x64. Its matrix
disables sanitizers. The separate
[instrumentation workflow](.github/workflows/instrumentation.yml) runs Linux
Clang sanitizer and coverage jobs. Coverage focuses on the exercised Rack
integration sources, excluding the bundled libraries and prebuilt Rack host.
Both test workflows run on pull requests and pushes to `master` (including
merges), with manual dispatch available. Tag pushes do not rerun tests; create
release tags from tested commits on `master`.
See the test guide for triggers, SDK versions, and report artifacts.

The [publication workflow](.github/workflows/manuals.yml) builds and validates
the manuals and technical report. Automated checks supplement manual review;
a passing build does not establish gameplay, audio, or visual correctness.

### Choosing Validation

| Change | Relevant Validation |
| --- | --- |
| Markdown guidance | Check links, paths, commands, and the complete diff; run `git diff --check` |
| Module or emulator behavior | Build the plugin, run relevant regression checks, and exercise affected behavior in Rack |
| Patch JSON or game maps | Check legacy and malformed data, saved game/location indices, and existing patches |
| Timing or audio | Check clock extremes, sample-rate changes, frame output, controllers, and channel/MIX routing with recorded fixtures |
| Panels, controls, or runtime SVGs | Check both themes and module preview; refresh and inspect production captures and affected manuals |
| Manual or report content/layout | Build the affected publications and inspect rendered PDFs and references |
| Source archive packaging | Extract the archive into an empty directory and compile it independently |

For integration changes, exercise the affected items in Rack:

-   Module creation, preview, light/dark themes, and display rendering.
-   ROM loading and replacement, invalid files, unsupported mappers, and
    patch reload when the original ROM is unavailable.
-   Both controllers, clock knob/CV extremes, frame-clock output, five
    individual channels, MIX routing, and host sample-rate changes.
-   Save/load, coincident save/reset/load, hang, module reset, and patch reopen.
-   CV Genie attached/detached, game/location selection, disconnected inputs,
    continuous and toggle inputs, and restored selections.

Use user-provided ROMs or redistributable test/homebrew fixtures. Record the
fixture revision, mapper, Rack/SDK version, platform, sample rate, settings,
and observations so another contributor can reproduce the check. The debug
patch may need local ROMs or other plugins during an interactive session.

Performance claims require matched before/after workloads, compiler flags,
repeated measurements, and reported uncertainty. A successful build or a
single timing result is not evidence of a speedup.

## Manual Figures

User-facing documentation lives in `manual/RackNES/` and `manual/CVGenie/`.
Each `figures/panel-layout.tex` uses shared TikZ primitives; align control
positions and numbered explanations with the production widget constructors.
The cover and README share each module's tracked `img/Panel.png`.

After visible widget or runtime SVG changes, follow the
[capture guide](tools/capture/README.md), then run these commands sequentially:

```shell
make -C tools/capture screenshots
make -C manual
```

The capture tool uses production widgets and an original branded mapper-0 ROM.
It requires a native desktop session and the dependencies listed in its guide.
Inspect both themes and the two cropped light-theme PNGs before committing.
The static silent ROM verifies display capture, not audio or gameplay.
Preserve existing screenshots and report an unverified refresh if native
rendering is unavailable; do not substitute a mockup.

Ordinary manual builds consume committed screenshots and need no Rack SDK,
ROM generation, or graphical session. Follow the [manual guide](manual/README.md)
for shared typography, PDF checks, and release assets. Each manual builds to
its own `.build/manual.pdf`. Keep intermediate captures and compiled PDFs
in ignored build directories.

## Technical Report And Releases

Follow the [report guide](whitepaper/README.md) for manuscript edits,
evidence, and citation metadata. The canonical standalone source is
`whitepaper/racknes.tex`; `whitepaper/sources.md` records supporting evidence.
Preserve its documented implementation revision unless intentionally updating
the account, and keep the README citation, `CITATION.cff`, and
`whitepaper/CITATION.bib` aligned with the actual manuscript status.

```shell
make -C whitepaper
make -C whitepaper source
```

These create `whitepaper/.build/racknes.pdf` and
`whitepaper/.build/racknes-source.tar.gz`. Inspect the rendered report after
typesetting changes and independently compile the extracted archive when
changing source packaging.

For a release, align `plugin.json`, `CHANGELOG.md`, and affected manuals,
validate code and publications, and verify the tag's commit and manifest
version. Do not move an already published tag to incorporate a fix.
The [manual workflow guide](manual/README.md#ci-and-release-assets) describes
publication of `RackNES.pdf`, `CVGenie.pdf`, and `RackNES-whitepaper.pdf`.
A tag push builds artifacts but does not attach them to a release.
GitHub publication and VCV Library submission are separate actions; verify
current submission requirements and the distributed revision before claiming
availability in Rack.

### Prepare A Release And VCV Update

Release preparation does not publish a GitHub release or notify VCV. Keep the
changelog entry marked `Unreleased` until choosing the publication date.
Before publishing, use the intended release checkout and configured Rack SDK:

```shell
make -B -j4
make dist
make -C tests -B -j2
make -C manual
make -C whitepaper
git diff --check
```

Inspect the package in `dist/` for the matching `plugin.json`, plugin binary,
runtime resources, and license files. Review the PDFs and perform the
[manual Rack checks](#choosing-validation). Record the platforms, SDK version,
results, and skipped checks in the release notes. Confirm the Rack matrix,
instrumentation, and publication workflows pass on the release revision;
feature-branch pushes alone do not trigger these workflows. Use a pull request
or an eligible workflow dispatch before release. The current Rack matrix covers
Linux x64, macOS arm64, and Windows x64; it does not validate macOS x64.

For publication, date the changelog and integrate the final release content
into `master`, which the public changelog and report links reference. Create
a new `v<version>` tag at the tested release commit. Verify the tag resolves to
the intended full commit hash and that its `plugin.json` version matches.
Never reuse or move a published tag. Publish the GitHub release with its change
summary and validation limitations. Confirm its assets include `RackNES.pdf`,
`CVGenie.pdf`, and `RackNES-whitepaper.pdf`, and that the README and manifest
download links resolve to the new manuals. The PDF workflow rejects tags that
disagree with the manifest; see the [asset workflow](manual/README.md#ci-and-release-assets)
for dispatch and upload details.

VCV's [open-source update instructions][vcv-updates] require a comment with the
new version and full commit hash in the plugin's permanent thread,
[VCVRack/library#650][vcv-thread]. Reuse that thread even when it is closed;
maintainers reopen it. Do not open a duplicate issue or submit a branch name
instead of a commit. Review the current [manifest requirements][vcv-manifest]
and [plugin guidelines][vcv-guidelines] before submission. Preserve the plugin
slug `KautenjaDSP-RackNES` and module slugs `RackNES` and `InputGenie` across
branding changes.

After pushing the final release commit, obtain its identifiers with:

```shell
jq -r .version plugin.json
git rev-parse HEAD
```

Use those values in the update comment, linking the commit and release notes.
For 2.2.0, mention the Arhythmetic Units branding, CV Genie selection/CV/patch
fixes, RackNES snapshot and display-resource fixes, and revised manuals.
The planned nes-py integration is not included. Wait for VCV's build result
and verify the Library's version before announcing availability there; a
GitHub release does not update the VCV Library automatically.

## Submit A Pull Request

1.  Keep one coherent change per contribution. Update affected documentation,
    resources, and patches alongside behavior changes. Avoid unrelated
    formatting or dependency upgrades, and retain upstream license notices.
2.  Review the intended changes from the repository root:

    ```shell
    git diff --check
    git diff
    git status --short
    ```

3.  Commit only the intended files, push your branch to your fork, and open a
    pull request against `master`. Describe the problem, resulting behavior,
    related issue, and validation commands and results.
4.  Include OS, compiler, Rack/SDK versions, and fixture details when relevant.
    Include screenshots for visual changes. Distinguish a successful build,
    executable regressions, and an actual Rack session; state skipped checks
    and unresolved failures explicitly.

[issues]: https://github.com/Kautenja/RackNES/issues
[cpp-style]: docs/style-guides/cpp.md
[markdown-style]: docs/style-guides/markdown.md
[vcv-updates]: https://github.com/VCVRack/library#pushing-an-update
[vcv-thread]: https://github.com/VCVRack/library/issues/650
[vcv-manifest]: https://vcvrack.com/manual/Manifest
[vcv-guidelines]: https://vcvrack.com/manual/PluginLicensing#vcv-plugin-ethics-guidelines

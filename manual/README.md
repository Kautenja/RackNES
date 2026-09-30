# RackNES User Manuals

The RackNES and CV Genie manuals describe the current implementation and
retain the original KautenjaDSP logos, panel artwork, red accents, and
Helvetica typography. The version in each `manual.tex` follows `plugin.json`.
Do not document planned features as available controls.

## Build

From the repository root:

```shell
make -C manual
```

Or build either manual separately:

```shell
make -C manual/RackNES
make -C manual/CVGenie
```

Outputs are `RackNES/.build/manual.pdf` and `CVGenie/.build/manual.pdf`, relative
to this directory. No Rack SDK, plugin build, ROM, or running Rack instance is
needed. Install a TeX distribution with `latexmk`, `pdflatex`, the recommended
fonts, and the packages used by `latex/kautenjadsp-manual.sty`. On Ubuntu, the
workflow uses `texlive-latex-extra`, `texlive-fonts-recommended`, `cm-super`, and `latexmk`.
Poppler (`pdfinfo`, `pdftoppm`) supports PDF inspection.

The shared rules disable shell escape, propagate compilation errors, and run
LaTeX until references settle. Sources and graphics are read in place.
Successful builds clean auxiliary files, leaving the PDF; failed builds retain
logs for diagnosis. `make -C manual clean` removes generated PDFs and auxiliaries
only. `LATEXMK` and `BUILD` can be overridden for individual manual builds; use
separate output directories for the two manuals.

The former `build/` directories may contain copied source and graphics. Do not
reuse those as `BUILD`: stale copies can shadow the current source during TeX
lookup. The default `.build/` avoids this legacy layout, and the shared build
rejects an output directory containing a copied `manual.tex`.

## Source Layout

-   `latex/kautenjadsp-manual.sty`: shared type, covers, headers, navigation, metadata.
-   `latex/manual.mk`: shared build and cleanup rules.
-   `RackNES/manual.tex`, `CVGenie/manual.tex`: identity, version, section order.
-   Each `sections/` directory: operating guide and reference content.
-   Each `img/` directory: original logos and panel illustrations, read in place.

Keep the annotated panel numbers aligned with their explanations. The supplied
panel artwork is an illustration, not a new runtime screenshot. Preserve the
KautenjaDSP identity and the visual-asset license terms in the root `LICENSE.md`.
The white paper is an independent publication and is not changed by these builds.

## Verify Content Against The Code

Read executable behavior rather than relying only on comments. This pass found
several places where comments or the former manuals overstated the behavior.

| Manual topic | Implementation to inspect |
| --- | --- |
| Knob ranges, defaults, gates, 16-sample acquisition | `src/RackNES.cpp`: constructor, `CVButtonTrigger`, `processCV` |
| Clock scaling and whole-cycle execution | `src/RackNES.cpp`: `getClockSpeed`, `process`; `src/nes/common.hpp` |
| CLK waveform, snapshot contents, ROM restoration | `src/nes/emulator.hpp` |
| Audio scaling and buffered sample handling | `src/nes/emulator.hpp`: `get_audio_voltage`; `src/nes/apu.hpp` |
| Voice removal from MIX, gains, held outputs | `src/RackNES.cpp`: `process` |
| ROM menus, drag/drop, exact error messages | `src/RackNES.cpp`: `ROMMenuItem`, `RackNESWidget` |
| Supported mappers | `src/nes/cartridge.hpp`: `Cartridge::create` |
| ROM validation and stored path | `src/nes/rom.hpp`, `src/nes/emulator.hpp` |
| Genie placement, writes, toggles, persistence, selection menus | `src/CVGenie.cpp`; `src/RackNES.cpp`: `processExpanders` |
| Every map entry, endpoint, flag, and duplicate address | `src/GameMaps.hpp` |
| Available modules; no Output Genie | `src/plugin.cpp`, `plugin.json` |
| Theme scope and persistence | `src/theme.hpp` |

The Genie tables preserve source order and contain all 53 Mario and 55 Zelda
entries. Compare names, hexadecimal addresses, decimal endpoints, and toggle
flags when editing them. In particular, preserve these current limitations in
the prose until the implementation changes:

-   A connected Unassigned row, row menu before game selection, or Randomize
    before selection can access an invalid map index.
-   Selecting a different game retains old row indices.
-   Toggles write 0/1 regardless of their endpoint metadata and are not serialized.
-   All five Mario Enemy Heading entries share `0x001A` with Enemy 5 Type.
-   Continuous Genie voltage is not clamped to 0--10 V.
-   Hang holds output voltages while control and Genie processing continue.
-   Snapshot serialization omits the frame counter and queued audio samples.
-   CPU cycles per Rack sample round upward; the APU conversion clock is set
    separately to 768000 in the module constructor. Clock CV is not calibrated
    audio pitch tracking or an external synchronization input.

This manual update does not fix these behaviors. Patch ideas are suggestions
based on the implementation, not claims of testing every game or ROM revision.

## Review The PDFs

After content, artwork, or shared-style changes, build both manuals and inspect
all pages at a readable size. For example:

```shell
pdfinfo manual/RackNES/.build/manual.pdf
mkdir -p /tmp/racknes-manual-review
pdftoppm -scale-to 1400 -png manual/RackNES/.build/manual.pdf /tmp/racknes-manual-review/racknes
pdftoppm -scale-to 1400 -png manual/CVGenie/.build/manual.pdf /tmp/racknes-manual-review/genie
```

Check the cover, contents, tables across page breaks, artwork labels, headings,
footers, links, and PDF outline. Confirm no clipped text, blank spill pages,
undefined references, or unresolved overfull boxes. Metadata includes title,
version, language, publisher, and XMP; font encoding and Unicode maps support
search and copying. These features are not PDF/A or PDF/UA certification.
Keep review images and generated PDFs untracked.

## CI And Release Assets

[User manuals](../.github/workflows/manuals.yml) builds both PDFs on relevant
pull requests and pushes to `master`, version-tag pushes, published releases,
and manual dispatch. It checks version agreement with the manifest, readable
PDFs, navigation, metadata, and resolved references, then retains a downloadable
`user-manuals` artifact for 14 days. Source changes also trigger the build so a
reviewer can consider whether the manual needs a corresponding update; CI does
not prove behavioral accuracy or replace visual inspection.

Only a published-release event or manual dispatch attaches assets to a release.
The separate upload job has write permission; ordinary build jobs have read
permission. Asset names are **RackNES.pdf** and **CVGenie.pdf**, matching the
existing README and manifest links. Rebuilding a release replaces assets with
those names. A tag push alone builds an artifact without publishing assets.

To rebuild an existing release, run the workflow with its tag. That tag must
already contain this shared manual layout; historical tags using the old build
are not supported by this workflow. The release must already exist. If a
release is created by another workflow using `GITHUB_TOKEN`, dispatch this
workflow manually when the release event does not start a run.

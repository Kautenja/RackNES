# RackNES User Manuals

The RackNES and CV Genie manuals describe the current implementation and
use Arhythmetic Units branding with the original panel artwork, red accents, and
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
fonts, and the packages used by `latex/arhythmetic-manual.sty`. On Ubuntu, the
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

-   `latex/arhythmetic-manual.sty`: shared type, covers, headers, navigation, metadata.
-   `latex/manual.mk`: shared build and cleanup rules.
-   `RackNES/manual.tex`, `CVGenie/manual.tex`: identity, version, section order.
-   Each `sections/` directory: operating guide and reference content.
-   Each `img/` directory: module logos, brand wordmarks, and panel
    illustrations, read in place.

Keep the annotated panel numbers aligned with their explanations. The supplied
panel artwork is an illustration, not a new runtime screenshot. Preserve the
Arhythmetic Units identity and the visual-asset license terms in the root
`LICENSE.md`.
The white paper is an independent publication and is not changed by these builds.

The `ArhythmeticUnits.pdf` wordmarks are copied from Fourier's manual assets.
The panel logo outlines in `../res/ArhythmeticUnits.svg` come from Fourier's
`src/rack_extensions/panel_artwork.hpp` at revision
`12ac3236e332b7d0a01b2d750fa07a0849132341`. The same outlines are embedded
in the four runtime panels and six manual/README illustrations. Keep those
copies aligned; retain the CV Genie collaborator credit alongside the brand.

After editing the illustration SVGs, regenerate their PDFs from the repository
root with CairoSVG (2.9.1 was used for this branding update):

```shell
for module in RackNES CVGenie; do
    for asset in "$module-Module" "$module-Manual" Plugin; do
        cairosvg "manual/$module/img/$asset.svg" -o "manual/$module/img/$asset.pdf"
    done
done
cairosvg manual/CVGenie/img/Plugin.svg -o manual/CVGenie/img/Plugin.png
```

CairoSVG requires Cairo. On macOS, a Homebrew Cairo installation may require
`DYLD_FALLBACK_LIBRARY_PATH=/opt/homebrew/lib` when running these commands.
These exports are tracked artwork assets; compiled manuals remain ignored.

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
flags when editing them. CV Genie validates selections, clamps continuous
voltage to 0--10 V, and toggles between the mapped endpoints. Its saved JSON
retains the existing game and location indices, with an optional per-row
`Toggle State` field. The Mario Enemy Heading entries use `0x0046` through
`0x004A`, following `Enemy_MovingDir` plus the slot index in the
[SMB disassembly](https://6502disassembly.com/nes-smb/SuperMarioBros.html#SymEnemy_MovingDir).

Some operating distinctions still matter: Hang holds outputs while controls
and Genie writes continue; snapshots omit the frame counter and queued audio;
and clock modulation changes execution speed rather than synchronizing to
an external clock or providing calibrated pitch tracking. Keep these
explanations practical and separate from setup instructions.

The focused checks in `tests/` cover selection handling, byte conversion,
toggles, patch compatibility, and RackNES snapshot ownership. Patch ideas
are suggestions, not claims of testing every game or ROM revision.

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

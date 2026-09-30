# Manual Panel Assets

Replace duplicated panel illustrations with instructional LaTeX wireframes
and screenshots rendered from the production Rack widgets. Preserve all
module behavior, identifiers, saved patches, and existing artwork notices.

Status: COMPLETE

Created: September 30, 2026

## Delivery

Work on the current `techreport` branch. Commit and push this specification
first, then complete and commit/push each item in order. Record validation
with each item. Move this document to `specs/archive/` on completion.

## Work Items

- [x] **1. Panel reference diagrams.** Replace both annotated panel images
    with native TikZ wireframes, showing the spatial control groups and the
    existing numbered explanations. Keep the NES display conceptual. Read
    widget positions from `src/RackNES.cpp` and `src/CVGenie.cpp`; remove the
    two obsolete `*-Manual.svg` / `*-Manual.pdf` pairs once unreferenced.
    Build both manuals, inspect the diagrams and page flow, and run
    `git diff --check` before committing and pushing.
- [x] **2. Production widget captures.** Add a small Rack-backed capture tool
    using the real module/widget constructors and drawing code, following
    Fourier's inspector approach without depending on Fourier. Generate one
    reviewed light-theme PNG per module for covers and README reuse; also
    inspect dark rendering. Run a deterministic original mapper-0 NES ROM
    through RackNES processing at 48 kHz before capture. Include reproducible
    source and explicit licensing for the ROM; use original Arhythmetic Units
    text/pixel art without commercial game content. Keep processing and
    drawing sequential in this tool, with no audio device or personal Rack
    settings. Validate ROM loading, nonblank rendered video, capture geometry,
    build failure handling, the plugin build and focused regression checks.
    Commit and push the tool, fixture sources, screenshots, and evidence.
- [x] **3. Reference migration and cleanup.** Switch covers and README images
    to the reviewed PNGs. Remove remaining obsolete `Plugin.*` and
    `*-Module.*` panel illustrations after checking all tracked references.
    Document exact capture prerequisites/commands, fixture settings, licenses,
    and when to refresh diagrams versus screenshots. Align contributor and
    CI instructions. Build both manuals from the checked-in assets without
    running Rack, inspect every page, check links and `git diff --check`,
    archive this completed spec, then commit and push.

## Acceptance Criteria

-   Each module has one maintained production screenshot, shared by its cover
    and README; spatial reference diagrams remain editable LaTeX.
-   Captures include actual controls, fonts, SVG panels, and live display
    rendering. No composited substitute screen or manually drawn panel image.
-   RackNES visibly runs the supplied original ROM. CV Genie shows its honest
    unassigned state; the fixture does not imply a supported game map.
-   Ordinary manual builds need TeX and the committed assets, not Rack or ROM
    generation. Capture tooling is optional and fails clearly without its
    graphical desktop/SDK prerequisites.
-   No production emulation, routing, widget behavior, or patch format changes.
    A capture harness is not a full interactive Rack/audio validation session.

## Validation Commands

Run from the repository root (capture commands will be recorded in item 2):

```shell
make -C manual
make
make -C tests
git diff --check
```

Use `pdfinfo`, `pdftotext`, and `pdftoppm` to check PDF content and rendering.
Record the actual Rack build, platform, fixture settings, and skipped checks.

## Completion Evidence

Specification created after inspecting the clean working tree, manual build,
asset references, widget coordinates, and Fourier's production-widget capture
and TikZ approach. Local Rack source and TeX tools are available.

Item 1 complete: both manuals built with `make -C manual` (13 pages each).
Rendered and visually inspected both panel pages at 1500 px: all 12 RackNES
references and both Genie groups fit beside their explanations. No overfull
boxes; references resolved on the final LaTeX pass. Removed four obsolete
annotated assets and updated the interim export instructions.

Item 2 complete: `make -C tools/capture screenshots` compiled the production
widgets and emulator against Rack v2.6.0 on macOS 26.6.2 arm64. Both themes
were visually inspected. Light PNGs are 1140 x 760 (RackNES) and 360 x 760
(Genie), native 2x crops. The original ROM produced 120 heartbeat frames and
781 RGB colors after 96,000 samples at 48 kHz. Repeated ROM generation and
captures were byte-identical on this machine. ROM SHA-256:
`cfe4b89947e6526b7b028ecfd67710ccc82e3af8c3abfe9f4d3e978eefa6a1e2`.

Missing-ROM capture returned error 3, and malformed crop geometry failed
before replacing assets. The initial sandboxed GLFW run stalled; desktop
access resolved it and the stalled process was stopped. Complete component
cache validation caught the need for more render passes. The fixture enables
both PPU planes with off-screen sprites to accommodate the current emulator's
scroll reload conditions; no production source was changed.

`make -j4` succeeded (existing plugin build current); `make -C tests -j4`
passed both focused suites. No interactive Rack/audio-device session or
Linux/Windows capture validation was performed. Tool instructions and fixture
licensing are in `tools/capture/README.md` and `LICENSING.md`.

Item 3 complete: covers and README share the two reviewed `Panel.png` files.
Removed nine remaining panel illustration files (13 obsolete assets removed
across items 1 and 3). Updated the manual guide, contributor instructions, and
CI paths/prerequisites. Branding/logo assets remain in their existing scope.

`make -C manual` built both 13-page manuals using committed PNGs and TikZ,
without invoking Rack. All 26 pages were rendered at 1400 px and visually
reviewed, including covers, control maps, tables, and colophons. PDF metadata,
outline, language, version, and resolved references passed the existing CI
checks locally. No overfull boxes appeared. Local documentation links and
README image paths exist; no active references to removed panel assets remain.
`git diff --check` passed. The hosted GitHub workflow was not run on this branch.

All three items are complete. Archived after the spec-first commit `93c5490`,
wireframe commit `5310d09`, and capture commit `29156e5`; each was pushed to
`origin/techreport`. This archive and reference migration form the final item
commit. Unrelated concurrent repository changes were preserved.

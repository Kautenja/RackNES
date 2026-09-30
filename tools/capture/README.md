# Production Panel Captures

This optional desktop tool renders the real RackNES and Input Genie widgets
against Rack 2. It uses the production module code, SVGs, controls, fonts, and
NES/NTSC display path. Fourier's native panel inspector inspired the harness;
Fourier is not a dependency. It opens no audio device and loads no user patch.

## Generate

Prerequisites: a prepared Rack 2 source tree or SDK with matching native
`libRack`, C/C++ compilers, an active OpenGL desktop session, Python 3, and
Pillow. Tested on macOS arm64; Linux is supported by the makefile but untested.
Windows capture is not implemented. Ordinary manual builds need none of these
capture dependencies.

From the repository root, with the normal `Rack/plugins/RackNES` layout:

```shell
make -C tools/capture screenshots
```

For another SDK or Python environment:

```shell
make -C tools/capture screenshots RACK_DIR=/absolute/path/to/Rack-SDK PYTHON=/absolute/path/to/python3
```

Use the same SDK/architecture for every build. Remove the ignored
`tools/capture/.build/` directory before changing SDK or compiler flags.
The target compiles the harness, generates the original ROM, renders both
light and dark themes, and losslessly crops the light captures to the two
tracked `manual/*/img/Panel.png` assets. Inspect both PNGs before committing.
`make -C tools/capture capture` renders without overwriting tracked PNGs.

Captures use an 800 by 420 logical-pixel canvas, with a module at (10, 20).
RackNES is 570 by 380 and Genie is 180 by 380. Native framebuffer density is
preserved (2x on the reviewed desktop). The exporter rejects unexpected
geometry rather than resizing. Review the crop and TikZ diagram whenever
module dimensions change. No UI content is painted into the captured image.

The harness uses a private `.build/user` asset path, Rack's global panel preference,
and newly created modules with default parameters and disconnected ports.
It processes RackNES for 96,000 samples at 48 kHz, sequentially before drawing,
then validates ROM initialization, a frame heartbeat, nonblank video, every
component framebuffer, and GL success. It waits up to 160 draw passes for
Rack's component caches; incomplete rendering fails the command. Genie shows
No Game Selected and eight Unassigned rows. The ROM has no Genie game map.
The harness checks light and dark defaults at widget construction, toggles
Rack's global preference light/dark/light on existing modules and null-module
browser previews, and verifies their selected production SVGs. It also
checks display image reuse, retry after allocation failure, cleanup on context
events, recreation, and deletion. Context events are simulated against the
live renderer; this is not a full OS graphics-context replacement test.
Additional Genie captures show a long assignment's hover help and its menu.
These use a saved Mario map selection without running a commercial ROM.

On macOS, a sandbox without desktop access can prevent GLFW from initializing
or stall in the OS window service; run the capture in a normal desktop terminal
or permit desktop access. A successful inspector run is not an interactive
Rack/audio-device test. Font rasterization and native pixel density can differ
across machines; reproducibility means the same fixture/settings and real
rendering path, not identical pixels on every OS.

## Original ROM

`make_demo_rom.py` uses only Python's standard library to emit a 24,592-byte
iNES mapper-0 cartridge: 16 KiB PRG-ROM and 8 KiB CHR-ROM, horizontal mirroring,
no battery, trainer, sound, input handling, or external assets. The generated
`.build/arhythmetic-units.nes` can also be loaded manually into RackNES.

The title card contains original pixel lettering and a stepped pulse motif.
It is a static visual fixture, not a game or an audio demonstration. The 6502
program initializes the PPU, uploads the nametable/palette, and increments RAM
`$10` once per vblank; `$11 = $A7` marks completed initialization. It enables
both rendering planes with all sprite Y positions set off-screen: RackNES's
current PPU requires both flags for scroll-address reloads. The fixture follows
that existing behavior without modifying the emulator. Register reference:
[NESdev PPU programmer reference](https://www.nesdev.org/wiki/PPU_programmer_reference).

Build the ROM without Rack or Pillow, from the repository root:

```shell
python3 tools/capture/make_demo_rom.py /tmp/arhythmetic-units.nes
```

Code is GPL-3.0-or-later. The original title-card pixel art/lettering and
rendered panel assets use the project's CC-BY-NC-ND-4.0 visual-asset terms;
see [LICENSING.md](../../LICENSING.md). No commercial game ROM, font, or graphics
are included. Distributing the combined ROM requires respecting both scopes.

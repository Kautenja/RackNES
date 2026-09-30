# RackNES Licensing

RackNES uses separate licenses for source code, visual assets, and bundled
dependencies. These terms apply to their respective materials; they are not
alternative licenses for the entire repository. The standard software license
text is in [LICENSE](LICENSE), kept separate from this scope guide for
GitHub license detection. This layout does not change the license terms.

## Source Code

RackNES source code in `src/` and capture tooling in `tools/capture/` are
licensed under the GNU General Public License, version 3 or (at your option)
any later version (`GPL-3.0-or-later`). This is the software license declared
in [plugin.json](plugin.json). Existing Christian Kauten and contributor
copyright notices remain in effect; individual files record their years
and authors. Bundled dependencies have the terms described below.

The NES emulator derives from [SimpleNES](https://github.com/amhndu/SimpleNES)
by Amish Naidu and contributors, under GPL version 3. Bundled third-party
code retains its existing copyright and license notices. See
[third-party notices](docs/licenses/THIRD-PARTY.txt) for component-specific terms.

## Visual Assets

The module visual designs and KautenjaDSP logo and icon are copyright
2020-2024 Christian Kauten. They are licensed under Creative Commons
Attribution-NonCommercial-NoDerivatives 4.0 International
(`CC-BY-NC-ND-4.0`). This covers the project graphics in `res/` and `manual/`.

The Arhythmetic Units logo and icon are copyright 2025-2026 Arhythmetic Units
under the same license. The original demo ROM pixel lettering and pulse
artwork in `tools/capture/make_demo_rom.py` use these visual-asset terms too;
the ROM program and generator use the source-code license above. The branding
in `res/` and `manual/` is adapted from the
[Fourier artwork](https://github.com/Kautenja/ArhythmeticUnits-Fourier).

The license permits sharing the licensed artwork for noncommercial purposes
with attribution. It does not permit sharing adapted artwork or commercial
use under the licensed rights. See the complete
[CC BY-NC-ND 4.0 text](docs/licenses/CC-BY-NC-ND-4.0.txt) for the terms, exceptions,
and limitations. The artwork license does not replace the source-code license.

## Bundled Dependencies

The [third-party notices](docs/licenses/THIRD-PARTY.txt) cover the bundled
components, including the LGPL audio and video libraries and the zlib-licensed
Base64 implementation. The license text, this scope guide, and `docs/licenses/`
are included in plugin packages by the [Makefile](Makefile). The source-code
and visual-asset split follows [Fourier's licensing terms](https://github.com/Kautenja/ArhythmeticUnits-Fourier/blob/main/LICENSE.md);
RackNES also ships the notices and license texts for its bundled dependencies.

# RackNES

**An NES as if it were designed by Bob Moog.** A voltage-controlled Nintendo
Entertainment System emulator for VCV Rack 2, by **Arhythmetic Units**. Play
the controllers with gates, reshape game time with CV, and turn saved moments
into musical patterns. Add **CV Genie** to put game memory under voltage control.

[![Latest GitHub Release][ReleaseBadge]][LatestRelease]
[![VCV Library: Rack 2][VCVBadge]][VCVLibrary]
[![Source License: GPL-3.0-or-later][LicenseBadge]](LICENSE.md)

**[Get it on VCV Library][VCVLibrary]** |
[RackNES manual (PDF)][RackNES] | [CV Genie manual (PDF)][CVGenie] |
[Changelog](CHANGELOG.md)

Read the [**technical report (PDF)**][Whitepaper] for the architecture and
implementation details. [BibTeX citation](whitepaper/CITATION.bib).

## RackNES

The game supplies the music, sound effects, and visuals. Your patch controls
how it plays: slow down a sequence, revisit a sound, or send each NES voice
through its own effects chain.

<p align="center">
  <img alt="RackNES panel with the Arhythmetic Units demo on its NES display"
       src="manual/RackNES/img/Panel.png" height="380px">
</p>

### Features

-   **Control game time.** Change emulation speed with the clock knob and
    CV attenuverter. Use the frame-clock output to drive downstream modules.
-   **Play with gates.** Automate both NES controllers, reset the game, or
    hold execution with Hang.
-   **Revisit a moment.** SAVE captures one emulator snapshot; LOAD returns
    to it. Repeated triggers can retrace a phrase or sound effect while you
    change the controls around it.
-   **Split the soundtrack.** Mix the two pulse voices, triangle, noise,
    and DMC sample channel, or patch their five individual outputs for
    separate processing. Audio follows Rack's host sample rate.
-   **Watch the game.** The built-in display uses an NES NTSC filter;
    light and dark panels follow Rack's **View > Use dark panels if available**
    setting.

[Explore the RackNES manual][RackNES]

## CV Genie

Go beyond the controllers. CV Genie is a Game Genie-inspired input expander,
developed by [@anlexmatos][anlexmatos], that writes CV into game memory.
Its eight rows expose named game elements from ten built-in maps, including
**Super Mario Bros.**, **The Legend of Zelda**, **Mega Man**, **Castlevania**,
**Contra**, **Metroid**, **Ninja Gaiden**, and **Tetris**. See the CV Genie
manual for the full game list and cartridge limitations.

<p align="center">
  <img alt="CV Genie input expander with a game selector and eight assignment rows"
       src="manual/CVGenie/img/Panel.png" height="380px">
</p>

-   **Choose what to control.** Select the running game, then assign a game
    element to each row, such as player position or an inventory value.
-   **Modulate or toggle.** Use 0-10 V for continuous entries and rising
    triggers for entries marked as toggles. Each element has its own range.
-   **Connect by placement.** Put **CV Genie (Input)** immediately to the
    right of RackNES, touching it in the same row.

The game selector chooses a memory map; it does not load or detect a ROM.
CV Genie provides named assignments rather than typed Game Genie codes or
an arbitrary-address editor. The available expander writes memory; it has
no game-memory CV outputs.

[Explore the CV Genie manual][CVGenie]

## Get Started

1.  With VCV Rack 2.4 or newer installed, sign in to your VCV account and add the
    modules from the [VCV Library][VCVLibrary]. In Rack's **Library** menu,
    sign in, choose **Update all**, and restart after the download.
2.  Add **RackNES** from the module browser. Connect **MIX** to your audio
    output through a mixer or attenuator, starting at a low listening level.
    Leave the clock and channel controls at their defaults.
3.  Right-click RackNES and choose **Load ROM**, or drop a `.nes` file
    onto the panel. Supply your own compatible ROM; the module starts empty.
4.  Use Player 1 **Start**, **Select**, directions, and **A/B** to reach a
    part of the game that makes sound. Panel buttons and input gates act
    as the NES controllers.
5.  Press **SAVE** at an interesting moment, let the game continue, then
    press **LOAD** to return to it.

This README describes the current checkout. VCV Library builds and the
latest-release PDF manuals can lag behind it; see the [changelog](CHANGELOG.md)
for unreleased changes and the [manual guide](manual/README.md) for matching
local documentation.

### ROM Compatibility

RackNES supports iNES ROMs using cartridge mapper IDs **0 (NROM)**,
**1 (MMC1)**, **2 (UNROM)**, and **3 (CNROM)**. Mapper support does not
ensure every game or modified ROM works. Extract archives before loading,
and keep the original ROM at its saved path when reopening a Rack patch.
CV Genie's curated game maps are separate from RackNES's broader ROM support.

### Try A First Patch

| Try This | Patch It Like This |
| --- | --- |
| Give percussion its own effects | Keep MIX connected and route NOI through a filter or delay. Connecting NOI removes that voice from MIX. |
| Automate a controller | Send a repeating gate to Player 1 A. High holds the button; low releases it. |
| Repeat a game moment | Press SAVE, then send slow triggers to LOAD. Each trigger restores the one stored machine state. |
| Drive a sequencer from the game | Route CLK through a clock divider. Changing emulation speed changes the frame-clock output too. |
| Move a game element with CV | Load Super Mario Bros., attach CV Genie, select the matching map, and assign Player Horizontal Screen Position to a row. Sweep a 0-10 V source slowly. |

The clock CV input changes execution speed; it does not synchronize to
incoming tempo pulses or provide calibrated pitch tracking. SAVE/LOAD
recalls machine state, so it does not produce sample-accurate audio loops.
The manuals explain timing, routing, and game-map behavior in detail.

## Support

See [SUPPORT.md](SUPPORT.md) for troubleshooting, questions, bug reports,
and feature requests. Include your Rack and plugin versions, operating
system, and steps to reproduce a problem; identify the ROM revision when
relevant.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for environment setup, architecture,
builds, regression checks, and pull request guidance. The
[C++](docs/style-guides/cpp.md) and [Markdown](docs/style-guides/markdown.md)
style guides cover coding and documentation conventions. Coding agents
should also follow [AGENTS.md](AGENTS.md).

## Building The Manuals

The [manual guide](manual/README.md) covers prerequisites, the shared LaTeX
build, PDF review, and release workflow. With Make, `latexmk`, `pdflatex`,
and the guide's TeX packages installed, run from the repository root:

```shell
make -C manual
```

The PDFs are written to `manual/RackNES/.build/manual.pdf` and
`manual/CVGenie/.build/manual.pdf`. Ordinary builds use committed screenshots
and need no Rack SDK or graphical session. The manuals cover the current
controls and limitations, including the supported CV Genie games. Assignment
details live in the module menus and hover help.

<details>
<summary><strong>Panel Figures And Production Captures</strong></summary>

Panel guides use LaTeX wireframes; covers and the images above share captures
of the production widgets. The [capture guide](tools/capture/README.md) explains
how to regenerate them with the original Arhythmetic Units demo ROM.
That silent title-card fixture supplies the branded display shown above.

</details>

## Citation

The technical report, [**RackNES: A Voltage-Controlled NES Emulator as a Musical
Instrument**][Whitepaper], describes the project's foundations, related
work, architecture, and implementation. Its [standalone LaTeX source](whitepaper/racknes.tex)
includes the bibliography and architecture diagram. [Download the latest-release
PDF][Whitepaper] or follow the [local build instructions](whitepaper/README.md).

To cite the project and its implementation account, use the report entry below.
GitHub's **Cite this repository** button uses the preferred citation in
[CITATION.cff](CITATION.cff); [CITATION.bib](whitepaper/CITATION.bib) provides the
same BibTeX entry.

```bibtex
@techreport{kauten2026racknes,
  author      = {Kauten, Christian},
  title       = {{RackNES}: A Voltage-Controlled {NES} Emulator as a Musical Instrument},
  institution = {Arhythmetic Units},
  year        = {2026},
  month       = sep,
  type        = {Technical report},
  note        = {Manuscript version 1, dated September 30, 2026; repository manuscript},
  url         = {https://github.com/Kautenja/RackNES/tree/master/whitepaper},
}
```

When referencing the software, link to this repository and identify the version
or commit used. The report documents RackNES 2.1.0 at source revision
[`e8c99bf86295318724ef62ad40a17ba6897be696`](https://github.com/Kautenja/RackNES/tree/e8c99bf86295318724ef62ad40a17ba6897be696).
It is currently a repository manuscript without an assigned DOI or conference
publication status.

## Acknowledgments

RackNES builds on the work of these projects:

-   [SimpleNES][SimpleNES], by Amish Naidu and contributors: the emulator core.
-   [Nes_Snd_Emu][Nes_Snd_Emu] and [Blip_Buffer][Blip_Buffer], by Shay Green
    (blargg): NES audio emulation and band-limited sound synthesis.
-   [nes_ntsc][nes_ntsc], by Shay Green (blargg): the NTSC video filter.
-   [cpp-base64][cpp-base64], by René Nyffenegger: Base64 encoding and decoding.

The [technical report](whitepaper/README.md) cites these sources and records
the emulator's nes-py lineage. See the [bundled notices](docs/licenses/THIRD-PARTY.txt)
for component licenses and attribution.

## Contributors

Many thanks to [@anlexmatos][anlexmatos] for developing the **CV Genie**
expander module.

## License

RackNES source code is licensed under [GPL-3.0-or-later](LICENSE-GPLv3.txt).
The module graphics and branding in `res/` and `manual/` are separately licensed under
[CC BY-NC-ND 4.0](docs/licenses/CC-BY-NC-ND-4.0.txt). Bundled dependencies retain
their own license notices.

See [LICENSE.md](LICENSE.md) for the scope of each license and
[third-party notices](docs/licenses/THIRD-PARTY.txt) for bundled components.
The software license does not grant the same permissions for the artwork.

[ReleaseBadge]: https://img.shields.io/github/v/release/Kautenja/RackNES?label=GitHub%20release
[LatestRelease]: https://github.com/Kautenja/RackNES/releases/latest
[VCVBadge]: https://img.shields.io/badge/VCV-Rack%202-0099dd
[VCVLibrary]: https://library.vcvrack.com/KautenjaDSP-RackNES
[LicenseBadge]: https://img.shields.io/badge/source%20license-GPL--3.0--or--later-blue
[RackNES]: https://github.com/Kautenja/RackNES/releases/latest/download/RackNES.pdf
[CVGenie]: https://github.com/Kautenja/RackNES/releases/latest/download/CVGenie.pdf
[Whitepaper]: https://github.com/Kautenja/RackNES/releases/latest/download/RackNES-whitepaper.pdf
[anlexmatos]: https://github.com/anlexmatos
[SimpleNES]: https://github.com/amhndu/SimpleNES
[Nes_Snd_Emu]: https://www.slack.net/~ant/libs/audio.html#Nes_Snd_Emu
[Blip_Buffer]: https://www.slack.net/~ant/libs/audio.html#Blip_Buffer
[nes_ntsc]: https://slack.net/~ant/libs/ntsc.html#nes_ntsc
[cpp-base64]: https://github.com/ReneNyffenegger/cpp-base64

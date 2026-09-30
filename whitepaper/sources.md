# Evidence and source notes

This file accompanies *RackNES: A Voltage-Controlled NES Emulator as a Musical
Instrument*. It records the provenance and limits of the manuscript's claims.
Literature and documentation were checked on September 30, 2026. Implementation
claims refer to RackNES version 2.1.0 at commit
[`e8c99bf86295318724ef62ad40a17ba6897be696`][revision], rather than to a moving
branch or the current versions of its dependencies.

The account uses source inspection, project documentation, primary research
papers, and documentation published by the relevant authors or projects. No
emulator runs, cartridge trials, benchmarks, listening tests, or user studies
were performed for this report. Equations describing control scaling and loop
counts are deductions from the source. Patch recipes are proposed uses, not
records of observed performances. Compilation, rendered-page inspection, and
citation consistency checks establish document integrity only.

## Literature provenance and claim scope

The keys below match the manuscript's bibliography. URLs identify the evidence
used to check metadata and claims; a DOI is supplied only where verified.

| Key | Verified reference and primary evidence | Supported use and limits |
| --- | --- | --- |
| `racknes` | Christian Kauten and contributors. *RackNES*, version 2.1.0. [Pinned source][revision], [manifest][manifest], [changelog][changelog], and [license statement][license]. | The manifest supplies the version; the changelog dates 1.0.0 to June 14, 2020, the Rack 2 port to February 20, 2022, and 2.1.0 to February 26, 2022. These are project records, not independent release verification. The pinned revision is the report's inspected artifact, not a claim that every line dates to the 2.1.0 release. |
| `moog1965` | Robert A. Moog. “Voltage Controlled Electronic Music Modules.” *Journal of the Audio Engineering Society* 13(3), 200–206, 1965. [AES publisher record](https://aes.org/publications/elibrary-page/?id=1204). | The abstract supports voltage-controlled oscillator, amplifier, and filter modules and an exponential voltage/frequency relationship. It does not establish a historical priority claim for RackNES or a calibrated pitch response in this implementation. No DOI is shown in the record. |
| `hunt2002` | Andy D. Hunt, Marcelo M. Wanderley, and Matthew Paradis. “The importance of Parameter Mapping in Electronic Instrument Design.” *NIME*, Dublin, 88–93, 2002. [Proceedings record](https://nime.org/proc/nime2002_hunt/); [DOI 10.5281/zenodo.1176424](https://doi.org/10.5281/zenodo.1176424). | Supports treating the mapping between control inputs and synthesis/system parameters as part of an instrument's identity. The manuscript applies this idea analytically; it does not borrow the paper's empirical findings as evidence about RackNES. |
| `collins2007` | Karen Collins. “In the Loop: Creativity and Constraint in 8-bit Video Game Audio.” *Twentieth-Century Music* 4(2), 209–227, 2007. [Cambridge publisher record](https://www.cambridge.org/core/services/aop-cambridge-core/content/view/S1478572208000510); [DOI 10.1017/S1478572208000510](https://doi.org/10.1017/S1478572208000510). | Supports the relationship between technological constraints, compositional choices, and looping in early game audio. Cite the 2007 journal issue. The inspected Cambridge page also labels online publication September 1, 2007, while its copyright line says 2008; the article PDF carries 2008 copyright. This is a metadata distinction, not evidence of a different work. An online-publication date of 2008 was not established by the inspected page. |
| `goncalves2011` | André Gonçalves. “Towards a Voltage-Controlled Computer Control and Interaction Beyond an Embedded System.” *NIME*, Oslo, 92–95, 2011. [Proceedings record](https://nime.org/proc/nime2011_goncalves/index.html), [paper](https://www.nime.org/proceedings/2011/nime2011_092.pdf), [DOI 10.5281/zenodo.1178035](https://doi.org/10.5281/zenodo.1178035). | The ADDAC system supplies a precedent for programmable computation integrated into voltage-controlled synthesis and computer communication. It is an embedded hardware system, not a console emulator. Preserve the distinction when connecting it to RackNES. The PDF prints the author's surname with a cedilla; the proceedings metadata drops it. |
| `soundcraft2013` | Mark Cerqueira, Spencer Salazar, and Ge Wang. “SoundCraft: Transducing StarCraft 2.” *NIME*, 243–247, 2013. [Paper](https://www.nime.org/proceedings/2013/nime2013_146.pdf); [DOI 10.5281/zenodo.1178492](https://doi.org/10.5281/zenodo.1178492); [authors' repository](https://github.com/markcerqueira/soundCraft). | A custom game map exports state/events, a Ruby parser sends OSC, and ChucK supplies musical interpretation. Supports game processes as compositional material and game-state mapping as established practice. It does not document emulated NES audio or prove anything about RackNES performance. |
| `mcalpine2018` | Kenneth B. McAlpine. *Bits and Pieces: A History of Chiptunes*. Oxford University Press. [Publisher record](https://academic.oup.com/book/6699), [introduction abstract](https://academic.oup.com/book/6699/chapter-abstract/150754567), [copyright page](https://academic.oup.com/book/6699/chapter-abstract/150754465), [DOI 10.1093/oso/9780190496098.001.0001](https://doi.org/10.1093/oso/9780190496098.001.0001). | The introduction supports hardware constraints shaping sound and working practices, the role of new software interfaces, and chiptune as live performance. The publisher gives November 22, 2018 online and December 27, 2018 print publication; the copyright and cataloging text say 2019. The key follows the publisher's 2018 publication date. Chapter-specific claims beyond inspected abstracts should be checked against the chapter before inclusion. |
| `lsdj` | Johan Kotlinski. *Little Sound Dj v9.2.6: Operating Manual*, May 9, 2021. [Author-hosted manual](https://www.littlesounddj.com/lsd/latest/documentation/LSDj_9_2_6.pdf); [project page](https://www.littlesounddj.com/lsd/index.php). | The cover verifies author/date. Sections 1.1 and 1.3 describe Game Boy sound channels and the song–chain–phrase–note tracker hierarchy. Useful as an example of purpose-built composition software for a console. It is not an NES program or a musical modification of an existing game. |
| `olson2016` | Ben Olson. “Transforming 8-Bit Video Games into Musical Interfaces via Reverse Engineering and Augmentation.” *NIME*, Brisbane, 73–77, 2016. [Paper](https://www.nime.org/proceedings/2016/nime2016_paper0016.pdf); [DOI 10.5281/zenodo.1176100](https://doi.org/10.5281/zenodo.1176100). | Section 3 describes Emstrument observing game RAM through Lua and sending MIDI. It explicitly does not generate or manipulate in-game audio. This is a close predecessor for emulator-based musical interfaces and reverse-engineered state mappings. Cite it as related practice, not as a comparison baseline or evidence that RackNES is first. |
| `megamachine` | LOOK MUM NO COMPUTER. “Gameboy Megamachine,” undated [creator project documentation](https://www.lookmumnocomputer.com/gameboy-megamachine). | Part 4 documents external-voltage button-matrix actuation and a CPU-modulator connection that changes execution speed. Supports the hardware antecedent for controller and clock intervention. The manuscript does not infer a publication year, precise console count, quantitative behavior, or priority from this page. |
| `nesclock` | NESdev contributors. “Cycle reference chart.” [NESdev technical reference](https://www.nesdev.org/wiki/Cycle_reference_chart). | Supports the NTSC nominal CPU frequency and 3:1 PPU-to-CPU relationship. The indexed page identified revision 22030 and includes conditional frame timing and PAL differences. It describes hardware/reference behavior; RackNES's integer frame counter is separately documented below. |
| `nesmixer` | NESdev contributors. “APU Mixer.” [NESdev technical reference](https://www.nesdev.org/wiki/APU_Mixer); supporting [APU overview](https://www.nesdev.org/wiki/NES_APU). | Supports nonlinear mixing and interaction among NES channels. It does not establish audible error magnitude for RackNES's separated-voice mix. Direct NESdev page opens encountered HTTP 403 during research; substantive indexed text returned by web search was inspected for both the cycle chart and mixer, not merely a result title. This access limitation prevents claiming a fresh direct-page revision audit. |
| `simplenes` | Amish Kumar Naidu and contributors. *SimpleNES*. [Project repository](https://github.com/amhndu/SimpleNES); [maintainer profile](https://github.com/amhndu). | Attribution and general C++ NES-emulator provenance. RackNES's own [license statement][license] explicitly records adaptation. Current upstream features and compatibility are not transferred to the pinned RackNES implementation. |
| `nespy` | Christian Kauten and contributors. *nes-py*. [Project repository](https://github.com/Kautenja/nes-py); [native CPU source](https://github.com/Kautenja/nes-py/blob/master/nes_emu/src/nes_emu/cpu.cpp). | Historical adaptation between SimpleNES and RackNES, supported by the import evidence below. The current nes-py implementation and planned integration are not the core inspected for this report. No exact imported upstream revision has been established. |
| `green` | Shay Green. “Blargg's Audio Libraries.” [Author documentation](https://www.slack.net/~ant/libs/audio.html); additional [algorithm explanation](https://www.slack.net/~ant/bl-synth/). | Documents Nes_Snd_Emu and Blip_Buffer: timed amplitude changes, source-clock/output-rate conversion, and band-limited synthesis. The bundled source also identifies Green and Nes_Snd_Emu 0.1.7. Dependency capability is not a claim that RackNES's host integration preserves every generated sample or achieves hardware fidelity. |
| `ntsc` | Shay Green. “Blargg's NTSC Libraries.” [Author documentation](https://www.slack.net/~ant/libs/ntsc.html); bundled [readme][ntsc-readme]. | Attribution and intended image-filtering function. The bundled readme identifies nes_ntsc 0.2.2. Performance numbers in the upstream readme concern other systems and are not used as RackNES measurements. |
| `rack` | VCV. “Plugin API Guide.” [Official VCV Rack manual](https://vcvrack.com/manual/PluginGuide). | Documents module processing, ports, state serialization, and double-buffered expander messages. The current guide explains a one-engine-frame message handoff, but is not a pinned historical Rack binary or an end-to-end latency measurement. The report uses the actual RackNES calls as implementation evidence. |
| `base64` | René Nyffenegger. *cpp-base64*. [Author repository](https://github.com/ReneNyffenegger/cpp-base64); bundled [source header][base64]. | Encoding/decoding provenance. The bundled header identifies version 2.rc.04 and author copyright. No general snapshot-portability, security, or integrity guarantee follows from using Base64. |

The bibliography deliberately omits unrelated emulator benchmarks and sources
that were consulted only as discovery aids. A literature review establishes
context and antecedents; it does not require a comparative evaluation of these
systems. The manuscript's three-layer vocabulary and proposed patch recipes
are the report's synthesis, not terminology attributed to the cited works.

## Emulator And Library Attribution

RackNES's [initial emulator import](https://github.com/Kautenja/RackNES/commit/1743db436a8da773069e61d82039e8e09b896129),
dated June 14, 2020, already contains `Program: nes-py` source headers. The
[acknowledgment added the same day](https://github.com/Kautenja/RackNES/commit/c19b6256f971a2ee2640f1c748a44614bd95aacc)
credits SimpleNES and Nes_Snd_Emu. Together with nes-py's own SimpleNES
attribution and the matching PPU structure, these support the lineage
SimpleNES -> nes-py -> RackNES. These RackNES commits do not identify the
exact SimpleNES or nes-py revision imported.

The audio and video libraries have separate authorship: Shay Green (blargg)
is credited in the bundled Nes_Snd_Emu, Blip_Buffer, and nes_ntsc headers.
The `green` and `ntsc` references cite his author-hosted documentation;
current library descriptions do not change the versions identified in the
bundled source. License notices and full texts are distributed separately
from scholarly citations; see [LICENSING.md](../LICENSING.md) and the
[component notices](../docs/licenses/THIRD-PARTY.txt).

## Mapper Updates: September 30, 2026

The manuscript's dated mapper addendum describes implementation through
[`2cea414`](https://github.com/Kautenja/RackNES/commit/2cea414), reviewed at
[`62cbf7a`](https://github.com/Kautenja/RackNES/commit/62cbf7a). The rest of the
implementation account remains pinned to `e8c99bf86295`; this addendum does
not retroactively attribute AxROM, MMC2, or MMC3 to that revision. The
development factory supports IDs 0--4, 7, and 9; this is not a release claim.

| Increment | Committed Implementation | Evidence Boundary |
| --- | --- | --- |
| AxROM / AOROM (7) | [4e11320](https://github.com/Kautenja/RackNES/commit/4e11320) | Banking, one-screen mirroring, header variants, state, DMC, and base-audio fixtures. |
| MMC2 / PxROM (9) | [8013d8f](https://github.com/Kautenja/RackNES/commit/8013d8f) | Post-read latches, saved fetches, bounded cartridge layouts, state, and base-audio fixtures. |
| MMC3B/C / TxROM (4) | [2cea414](https://github.com/Kautenja/RackNES/commit/2cea414) | Banking, RAM, four-screen storage, approximate A12 timing, shared CPU/APU IRQs, state, and base-audio fixtures. |

The `supports()` validators in the three mapper headers below establish the
documented PRG/CHR limits, power-of-two sizes, NTSC-only policy, exact payloads,
and explicit NES 2.0 RAM requirements. These checks do not implement the full
NES 2.0 format or demonstrate Pulsar/PR8 operation.

-   [AxROM implementation](../src/nes/mappers/mapper7_AxROM.hpp) and
    [cartridge factory](../src/nes/cartridge.hpp): 32 KiB banking, one-screen
    pages, accepted header layouts, bus conflicts, and cartridge rebinding.
-   Upstream: nes-py `301da52f7f75de380e6e195fd36621c3d5b03757`,
    `mapper_AxROM.hpp/.cpp`, `mapper_bank.hpp`, and `test_mapper_AxROM.cpp`;
    [license and attribution](../docs/licenses/THIRD-PARTY.txt).
-   [AxROM checks](../tests/axrom.hpp) and
    [audio comparison](../tests/racknes.cpp): synthetic memory/state/DMC
    checks and 2,000 samples per host rate (44.1, 48, 96, 192 kHz), with
    1,789,773 Hz and 768,000 Hz Blip clocks at nominal emulation speed.
    The AxROM audio image repeats the reference program/sample data in four
    PRG banks; a separate DMC check reads distinct bank markers.
-   [MMC2 implementation](../src/nes/mappers/mapper9_MMC2.hpp),
    [PPU fetch handling](../src/nes/ppu.cpp), and
    [MMC2 regressions](../tests/mmc2.hpp): old-bank trigger bytes, independent
    latch ranges, PRG RAM, mirroring, cached pattern bytes in snapshots,
    clipped/covered sprites and flipped rows. Adapted from the same upstream
    revision's `mapper_MMC2.hpp/.cpp`, PPU fetch structure, and MMC2 tests.
    RackNES combines observation and latch transition in one CHR read;
    sprite prefetch remains at its existing scanline boundary, not a claim
    of hardware-exact PPU timing. Earlier mappers retain their pixel-read path.
    MMC2 joins the same four-rate/two-clock PCM comparison, with switchable
    code banks and fixed DMC windows.
-   [MMC3 implementation](../src/nes/mappers/mapper4_MMC3.hpp) and
    [regressions](../tests/mmc3.hpp): banking, banked CHR RAM, RAM protection,
    four-screen storage, IRQ acknowledgement/reload, timed sprite/dummy
    fetches, masked IRQs, NMI priority, DMA stalls, and patch restoration.
    Banking/register behavior adapts the pinned nes-py MMC3 implementation.
    The [NESdev MMC3 reference](https://www.nesdev.org/wiki/MMC3) describes
    hardware filtering through M2 edges; RackNES uses a conservative ten-dot
    low filter and does not reproduce sub-cycle M2 alignment or MMC3A IRQs.
-   [CPU](../src/nes/cpu.cpp), [emulator](../src/nes/emulator.hpp), and
    [APU wrapper](../src/nes/apu.hpp): poll independent APU/mapper IRQ levels
    without reading/acknowledging status. The bundled notifier reports schedule
    changes; it is no longer treated as an immediate CPU interrupt.
    [Snapshot refresh](../src/nes/apu/apu_snapshot.cpp) repairs stale restored
    IRQ scheduling. Stack flags use hardware positions while legacy CPU JSON
    keeps its prior bitfield encoding. Synthesis and buffer timing remain fixed.
    MMC3 joins the same PCM comparison with CHR bank writes and fixed DMC data.
-   [Spec 001](../specs/001-nes-py-integration.md) records exact validation
    commands and limitations. The cited mapper runs used macOS arm64 and
    Apple Clang 21. No new performance measurements, commercial game tests,
    manual listening, clock-extreme, or other-platform results are claimed.
-   [SRAM policy](../specs/archive/004-sram-import-export.md) and
    [ROM eligibility](../src/nes/rom.hpp): external save interchange remains
    restricted to reviewed mapper 0--3 layouts. Mapper snapshots and raw
    SRAM files have different coverage; new mapper RAM in a patch does not
    imply file interchange or complete audio-buffer continuation.
-   [MMC5/spec 008](../specs/008-mmc5-implementation.md) and
    [FME-7/spec 009](../specs/009-fme7-implementation.md) are plans, not
    implemented capabilities. The factory still rejects these mapper IDs.
    Their expansion sound is absent; bundled sound-chip code alone does not
    establish synthesis or routing support.

## Implementation evidence at the pinned revision

Each link below resolves to the specified commit. Function names and line
anchors identify the relevant code; when comments and executed expressions
differ, the expressions determine the account.

| Claim or boundary | Evidence | Interpretation and limits |
| --- | --- | --- |
| Coupled console components and callbacks | [`Emulator::Emulator`][emulator-callbacks], members immediately above it | CPU, PPU, APU, controllers, buses, and cartridge belong to one emulator. Register callbacks connect them; the DMC reader calls the main bus. This is an ownership/connection description, not a timing-accuracy claim. |
| Implemented cartridge subset | [`Cartridge::MapperID` and `Cartridge::create`][cartridge] | The switch handles 0/NROM, 1/MMC1, 2/UNROM, and 3/CNROM only. Bundled additional sound-chip files do not establish routed expansion-audio support. |
| Parameter ranges and clock equation | [`RackNES` constructor][rack-constructor], [`RackNES::getClockSpeed`][clock-speed], [constants][constants] | The exponent is the knob plus attenuverted input voltage divided by five, clipped to −4…4. `CLOCK_RATE` is 1,789,773. The result is converted to `uint64_t`; a 5 V input change at unity attenuverter adds one exponent unit until clipping. This is not a standard 1 V/octave input. |
| Per-sample cycle loop and upward rounding | [`RackNES::process`][process] | A `size_t` index starts at zero and compares against `getClockSpeed() / args.sampleRate`. For a stable positive quotient it executes its ceiling number of iterations, subject to finite-precision formation of the quotient. There is no fractional remainder accumulator. The derived excess-rate bound is algebraic, not measured. |
| PPU/CPU/APU advancement | [`Emulator::cycle`][cycle], [`APU::cycle`][apu-cycle] | Three PPU calls, one CPU call, and one APU-wrapper call occur per emulator iteration. The wrapper calls `end_frame(1)` on the APU and buffers. These library frames denote elapsed sound-emulation time, not full video frames. |
| Distinct control cadences | [Constructor divider setting][rack-constructor], [`processCV`][process-cv], [`process`][process] | Controller and save/reset/load/hang acquisition is divided by 16. Rate CV is read in the loop condition; expander consumption occurs each host sample. Therefore no single “CV update rate” describes the whole module. Missing a sufficiently short between-acquisition pulse is a source-level possibility, not an observed failure rate. |
| Frame-derived output and image publication | [`Emulator::is_clock_high`][clock-high], [`Emulator::cycle`][cycle], [constants][constants], [`RackNES::copyScreen`][copy-screen], [`RackNES::process`][process] | The emulator's separate counter wraps at 29,781 and requests a screen copy. Its half-period test drives a 0/10 V gate. This counter is distinct from PPU scanline state. The output metadata says “CPU clock,” but the executed expression is frame-derived. |
| NTSC rendering path | [`PPU::cycle`][ppu-render], [`PPU::get_screen_buffer`][ppu-json], [`RackNES::copyScreen`][copy-screen], [display widget][display] | The PPU calls the NTSC blitter; the module copies the filtered buffer; the widget displays that copy. This identifies dataflow, not a guarantee about display synchronization or visual fidelity. |
| Five individual sound buffers | [`APU::APU`][apu-constructor], [`Nes_Apu::Nes_Apu`][nes-apu] | `osc_output` assigns a separate Blip_Buffer to each of the five base oscillators. The supplied voices are two pulses, triangle, noise, and DMC. |
| Fixed sound-conversion clock | [Constructor][rack-constructor], [`Emulator::set_clock_rate`][emulator-rate], [`APU::set_clock_rate`][apu-rate], [`RackNES::onSampleRateChange`][sample-rate] | The constructor changes buffer source-clock conversion to 768,000 Hz. Host sample-rate changes update buffer output rate. Clock CV changes iteration count without updating the 768 kHz conversion setting. The changelog records the historical motivation; the report does not adopt its improvement claim as a new result. |
| Audio extraction drains generated data | [`APU::get_sample`][apu-sample] | Empty buffer returns zero. Otherwise a temporary vector is sized for all available samples, `read_samples` consumes them, and only element zero is returned. The consequences depend on availability and execution rate; no numerical dropout, distortion, or aliasing measure is claimed. |
| Voltage scaling and connection-dependent mix | [`Emulator::get_audio_voltage`][audio-voltage], [`RackNES::process`][process] | The code multiplies returned signed samples by 10/32767, then by channel gain. Only unconnected individual outputs contribute to the mix, which receives master gain. The variable name `Vpp` does not establish a measured peak-to-peak level. The arithmetic is a linear sum of separated outputs. |
| Save/reset/load ordering and hang behavior | [`RackNES::processCV`][process-cv], [`RackNES::process`][process] | Save precedes reset, which precedes load, followed by controller updates. CV and expander processing precede the hang return. Hang skips stepping and output writes, so it retains prior output values instead of explicitly writing silence. |
| Backup and host-patch serialization | [`RackNES::dataToJson` / `dataFromJson`][rack-json], [`Emulator::dataToJson` / `dataFromJson`][emulator-json] | There is one emulator backup JSON object; host state records current emulator and a deep copy of the backup. The emulator serializer includes cartridge, both controllers, buses, CPU, PPU, and APU. The report does not describe this as complete state restoration. |
| Snapshot omissions and file dependence | [`Emulator::dataToJson` / `dataFromJson`][emulator-json], [`APU::dataToJson` / `dataFromJson`][apu-json], [`ROM::dataToJson`][rom-json], [`ROM` file operations][rom-load] | Emulator `cycles` and Blip_Buffer contents are absent. Restoration validates the stored ROM path and calls `load_game`, which resets components before loading stored fields. The APU reset clears its buffers; recall does not restore the previous rendered queue. Serialized ROM data therefore do not remove the path/file dependency of this restore path. |
| Incorrectly sourced PPU status fields | [`PPU::dataToJson`][ppu-json], corresponding `dataFromJson` immediately below | `is_even_frame`, `is_vblank`, and `is_sprite_zero_hit` are each serialized with `json_boolean(scanline)`, rather than their matching member values. This directly limits the snapshot contract; the report makes no claim about how frequently a particular cartridge exposes the defect. PPU image buffers are also absent from this serializer. |
| Variable work within the sample callback | [`RackNES::process`][process], [`processCV`][process-cv], [`APU::get_sample`][apu-sample], [`Emulator::dataFromJson`][emulator-json], [`ROM` file operations][rom-load] | Calls reachable in this path include cartridge file access, JSON state operations, temporary audio-vector allocation, the rate-dependent cycle loop, and image copies. Source establishes these operations; it cannot establish their elapsed cost, underrun rate, or a hard real-time guarantee. |
| CV Genie eight-lane transport | [`CVGenie::process`][genie-process], [`RackNES::processExpanders`][expanders], [buffer allocation][rack-constructor] | Input Genie requires RackNES immediately to its left. It writes eight `uint16_t` address/value pairs into the producer buffer and requests a flip. RackNES consumes pairs, directly writes its byte RAM, and clears each consumed address to zero. Zero is a transport sentinel, so address zero is not expressible as a write in this protocol. |
| CV Genie numeric and toggle semantics | [`GameParameter`][game-parameter], [`CVGenie::process`][genie-process] | The explicit `toggle` flag chooses a Schmitt-trigger path that flips local Boolean state and transmits 0 or 1; it does not map that Boolean through configured endpoints. The numeric path rescales 0–10 V using stored byte endpoints, including reversed ranges, without an explicit clamp. Two-valued ranges alone do not select toggle behavior. Thus neither labels nor endpoints establish validated game semantics. |
| Direct RAM writes and scope of memory maps | [`MainBus::get_memory_buffer`][ram], [`RackNES::processExpanders`][expanders], [`GameMap` tables][game-maps] | The consumer writes into the internal RAM array, rather than intercepting cartridge PRG reads. Tables name Super Mario Bros. and The Legend of Zelda; Zelda entries include triangle frequency, song, voice positions, note durations, rhythm, and volume. Names reflect the map authors' interpretation; no cartridge checksum/revision validation or runtime semantic validation was performed. |
| Input-only registered expansion | [`CVGenie::process`][genie-process], [`RackNES::processExpanders`][expanders], [`init`][plugin-init], [manifest][manifest] | Output processing branches are commented out and `modelOutputGenie` is not registered. Some output-widget/model code remains in the file, but it does not make a usable memory-to-CV module available in the plugin. |

The snapshot omissions listed here are specific observed omissions, not an
exhaustive certification of every serialized component. Likewise, the memory
map discussion establishes available labels and executed mapping rules; it
does not endorse every address or byte value as correct for every ROM.

[revision]: https://github.com/Kautenja/RackNES/tree/e8c99bf86295318724ef62ad40a17ba6897be696
[manifest]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/plugin.json
[changelog]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/CHANGELOG.md
[license]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/LICENSE.md
[rack-constructor]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L129
[clock-speed]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L236
[process-cv]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L248
[expanders]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L309
[process]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L340
[copy-screen]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L231
[sample-rate]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L384
[rack-json]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/RackNES.cpp#L399
[constants]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/common.hpp#L33
[emulator-callbacks]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/emulator.hpp#L59
[clock-high]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/emulator.hpp#L118
[emulator-rate]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/emulator.hpp#L170
[audio-voltage]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/emulator.hpp#L249
[cycle]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/emulator.hpp#L272
[emulator-json]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/emulator.hpp#L315
[cartridge]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/cartridge.hpp#L35
[apu-constructor]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/apu.hpp#L34
[apu-rate]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/apu.hpp#L73
[apu-cycle]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/apu.hpp#L110
[apu-sample]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/apu.hpp#L121
[apu-json]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/apu.hpp#L133
[nes-apu]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/apu/Nes_Apu.cpp#L17
[ppu-json]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/ppu.hpp#L193
[ppu-render]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/ppu.cpp#L263
[display]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/widget/display.hpp#L63
[rom-json]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/rom.hpp#L272
[rom-load]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/rom.hpp#L149
[ram]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/main_bus.hpp#L125
[genie-process]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/CVGenie.cpp#L75
[game-parameter]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/GameMaps.hpp#L22
[game-maps]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/GameMaps.hpp#L58
[plugin-init]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/plugin.cpp#L25
[ntsc-readme]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/nes/ntsc/readme.txt
[base64]: https://github.com/Kautenja/RackNES/blob/e8c99bf86295318724ef62ad40a17ba6897be696/src/base64.cpp

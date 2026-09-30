# RackNES: A Voltage-Controlled NES Emulator as a Musical Instrument

A technical report by Christian Kauten (Arhythmetic Units), manuscript version 1,
dated September 30, 2026. It describes RackNES's foundations, architecture, musical
interaction, and implementation lessons for future engineers and researchers.
The report is a repository manuscript; it has no assigned DOI or conference
publication status.

## Read and build

[racknes.tex](racknes.tex) is the canonical manuscript. The prose, bibliography,
and architecture diagram are contained in that one file, which can be opened in
the Codex LaTeX editor with its PDF preview. The native editor does not require a
terminal TeX installation.

For a command-line PDF build, install `latexmk`, `pdflatex`, and the packages
listed in the manuscript preamble, then run from the repository root:

```shell
make -C whitepaper
```

The output is [`.build/racknes.pdf`](.build/racknes.pdf). The build disables shell
escape and only typesets the paper. This is a design and implementation report;
it contains no comparative experiments or benchmark results.

To package the standalone TeX source for sharing:

```shell
make -C whitepaper source
```

Extract `.build/racknes-source.tar.gz` into an empty directory and compile with:

```shell
latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' racknes.tex
```

`make -C whitepaper clean` removes the local `.build/` directory, including the
PDF, compilation logs, and source archive. The archive target does not publish or
submit the manuscript.

## Sources and maintenance

The implementation account describes RackNES 2.1.0 at source revision
[`e8c99bf86295318724ef62ad40a17ba6897be696`](https://github.com/Kautenja/RackNES/tree/e8c99bf86295318724ef62ad40a17ba6897be696).
[sources.md](sources.md) records the literature and source-code evidence behind
the manuscript. Keep claims tied to that evidence when revising the paper, and
update the source revision if the implementation account changes.

Maintain the manuscript in `racknes.tex` so that the native editor and the
command-line build use the same complete source. Keep literature references in
its inline bibliography. [CITATION.bib](CITATION.bib) is the citation for this
report itself, rather than the report's research bibliography.

## Cite

[CITATION.bib](CITATION.bib), the repository [CITATION.cff](../CITATION.cff), and
the [project README](../README.md#citation) provide the same preferred report
citation. Identify the software version or commit used when citing the code.
After a public deposit, update all three entries with its actual identifier and
URL, and keep the manuscript version, title, author, institution, and date in
agreement.

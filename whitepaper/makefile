# Typeset the standalone manuscript without running the RackNES software.
.DEFAULT_GOAL := all
.PHONY: all pdf source clean

LATEXMK ?= latexmk
TEX_SOURCE := racknes.tex
PDF := .build/racknes.pdf
SOURCE_ARCHIVE := .build/racknes-source.tar.gz

all: pdf

pdf: $(PDF)

$(PDF): $(TEX_SOURCE) makefile | .build
	$(LATEXMK) -pdf -pdflatex='pdflatex -no-shell-escape %O %S' -interaction=nonstopmode -halt-on-error -outdir=.build $(TEX_SOURCE)

source: $(SOURCE_ARCHIVE)

$(SOURCE_ARCHIVE): $(TEX_SOURCE) makefile | .build
	COPYFILE_DISABLE=1 tar -czf $@ $(TEX_SOURCE)

.build:
	mkdir -p $@

clean:
	rm -rf .build
